#include "BaseServer.hpp"
#include "../DataBase/Database.hpp"
#include "../Utils/utils.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iomanip>
#include <sys/time.h>
#include <sstream>
#include <fstream>
#include <cmath>
#include <mutex>
#include <thread>
#include <boost/thread.hpp>
#include <fcntl.h>
#include <chrono>
#include <atomic>

using namespace std;
using namespace pqxx;
using namespace tbb;
using namespace std::chrono;
using namespace google::protobuf::io;

#define MAX_LIMIT 65536
#define TIME_OUT_LIMIT 1
#define SIMWORLD_UPS_HOST "vcm-25032.vm.duke.edu"
#define SIMWORLD_UPS_PORT "12345"

struct requestInfo {
    int64_t seqnum;
    UCommands ucom;
    time_t recvTime;
};

class MyCmp {
public:
    bool operator()(const requestInfo &r1, const requestInfo &r2) const {
        return r1.recvTime > r2.recvTime;
    }
};

// for timeout and retransmission
std::atomic<int64_t> seqnum(0);
priority_queue<requestInfo, vector<requestInfo>, MyCmp> waitAcks;
unordered_set<int64_t> unackeds; 
mutex waitAckMutex;

// records of the results of requested commands
unordered_map<int64_t, string> errorCmds;
unordered_set<int64_t> errorFreeCmds;

BaseServer::BaseServer(const char *_hostname, 
    const char *_port, int _backlog, int _threadNum) 
    : sock(nullptr), worldSock(nullptr), worldIn(nullptr), worldOut(nullptr), 
    amazonSock(nullptr), backlog(_backlog), threadNum(_threadNum), init(threadNum) {	
	// create, bind, and listen to a socket
	setupServer(_hostname, _port);
}

void BaseServer::setupServer(const char *hostname, const char *port) {
	// create, bind, and listen to a socket
	struct addrinfo host_info;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

	sock = new MySocket(host_info, hostname, port);
	setSocket(sock);
	sock->bindSocket();
	sock->listenSocket(backlog);

    // connect to sim world
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    worldSock = new MySocket(host_info, SIMWORLD_UPS_HOST, SIMWORLD_UPS_PORT);
    worldSock->connectSocket();
    // create socket in/out file stream
    worldOut = new FileOutputStream(worldSock->getSocketFd());
    worldIn = new FileInputStream(worldSock->getSocketFd());
	// get world id
    setWorldId(getWorldIdFromSim());

    // connect to amazon

}

int64_t BaseServer::getWorldIdFromSim() {
    vector<int> xs = {0,23,323};
    vector<int> ys = {5,1,379};

    UConnect ucon;
    ucon.set_isamazon(false);

    for (size_t i = 0; i < 3; ++i) {
        UInitTruck *truck = ucon.add_trucks();
        truck->set_id(i);
        truck->set_x(xs[i]);
        truck->set_y(ys[i]);
    }
    
    sendMesgTo<UConnect>(ucon, worldOut);

    // wait for response
    cout << "waiting for world id from sim world.....\n";
    UConnected resp;
    recvMesgFrom<UConnected>(resp, worldIn);

    if (resp.result() == "connected!") {
        cout << "Connected to world: " << resp.worldid() << endl;
    } else {
        cout << "Something went wrong with receiving world id from sim world: " << resp.worldid();
    }

    google::protobuf::ShutdownProtobufLibrary();
    return resp.worldid();
}

BaseServer::~BaseServer() {
    delete sock;
    delete worldSock;
    delete amazonSock;
    delete worldIn;
    delete worldOut;
}

void BaseServer::launch() {
	// daemonize();

    // one thread for receiving responses from sim world
    group.run([=]{simWorldCommunicate();});
    // one thread for communicating with amazon
    group.run([=]{amazonCommunicate();});
    // one thread for implementing timeout and retransmission mechanism
    group.run([=]{timeoutAndRetransmission();});

	group.wait();
}

void BaseServer::timeoutAndRetransmission() {
    while (1) {
        // wait for several seconds
        sleep(1);

        // check if there're requests timeout
        waitAckMutex.lock();
        while (!waitAcks.empty() &&
            time(nullptr) - waitAcks.top().recvTime > TIME_OUT_LIMIT) {
            // if this request has already been acked, just remove it
            if (unackeds.find(waitAcks.top().seqnum) == unackeds.end()) {
                waitAcks.pop();
                continue;
            }
            
            requestInfo req = waitAcks.top();
            waitAcks.pop();

            // retransmit the request
            cout << "Request with sequence #: " << req.seqnum << " timed out. Retransmitting...\n";
            sendMesgTo<UCommands>(req.ucom, worldOut);

            // put the request back with new timestamp
            req.recvTime = time(nullptr);
            waitAcks.push(req);
        }
        waitAckMutex.unlock();
    }
}

// logic of communication with sim world
void BaseServer::amazonCommunicate() {
    // big while loop
    // while (1) {
        // wait for notification

        // processing order and update database
        
    // }
}

// logic of communication with sim world
void BaseServer::simWorldCommunicate() {
	// std::cout << boost::this_thread::get_id() << endl;
	// connect to database, need a new connection for each thread
	connection *C = db.connectToDatabase();

    // set sim world speed, default is 100
    adjustSimSpeed(100);

    cout << "Sending test command\n";
    sendTestCommand();

    while (1) {
        cout << "Waiting for responses\n";
        UResponses resp;
        bool status = recvMesgFrom<UResponses>(resp, worldIn);

        if (status) {
            // get all sequence numbers
            vector<int64_t> seqnums = extractSeqNums(resp);
            // send back acks
            for (int64_t seqnum : seqnums) {
                ackToWorld(seqnum);
            }

            // displaying responses
            displayUResponses(resp);

            // upon receiving different responses, update the database, and do other related stuff
            // do stuff related to completions
            processCompletions(resp);
            // do stuff related to delivered
            processDelivered(resp);
            // do stuff related to truckstatus
            processTruckStatus(resp);
            // do stuff related to error
            processErrors(resp);
            // do stuff related to acks
            processAcks(resp);
            // If the world set finished, then we would not receive responses from the world again, just exit the while loop
            if (resp.finished()) {
                break;
            }
        } else {
            cerr << "The message received is of wrong format. Can't parse it. So this would be skipped.\n";
        }

        google::protobuf::ShutdownProtobufLibrary();
    }

	C->disconnect();
	delete C;
}

void BaseServer::processErrors(UResponses &resp) {
    for (int i = 0; i < resp.error_size(); ++i) {
        const UErr &err = resp.error(i);
        // if this response has already been processed, skip it
        if (unackeds.find(err.seqnum()) == unackeds.end()) {
            continue;
        }

        // display the error
        cerr << err.err() << " when dealing with request " << err.originseqnum() << endl;
    }
}

void BaseServer::processAcks(UResponses &resp) {
    waitAckMutex.lock();
    // remove the corresponding sequence number in unackeds
    for (int i = 0; i < resp.acks_size(); ++i) {
        // if has indeed not been acked
        if (unackeds.find(resp.acks(i)) != unackeds.end())
            unackeds.erase(resp.acks(i));
    }
    waitAckMutex.unlock();
}

void BaseServer::processTruckStatus(UResponses &resp) {
    // update the truckStatus
    for (int i = 0; i < resp.truckstatus_size(); ++i) {
        const UTruck &truck = resp.truckstatus(i);
        // if this response has already been processed, skip it
        if (unackeds.find(truck.seqnum()) == unackeds.end()) {
            continue;
        }

        // update truck table
        
    }
}

void BaseServer::processDelivered(UResponses &resp) {
    for (int i = 0; i < resp.delivered_size(); ++i) {
        const UDeliveryMade &delivered = resp.delivered(i);
        // if this response has already been processed, skip it
        if (unackeds.find(delivered.seqnum()) == unackeds.end()) {
            continue;
        }
        
        // notify amazon the package has been delivered
        notifyDeliveredToAmazon(delivered.packageid(), seqnum.fetch_add(1));

        // update the package table, setting the status to be "delivered"

    }
}

void BaseServer::processCompletions(UResponses &resp) {
    for (int i = 0; i < resp.completions_size(); ++i) {
        const UFinished &finished = resp.completions(i);
        // if this response has already been processed, skip it
        if (unackeds.find(finished.seqnum()) == unackeds.end()) {
            continue;
        }

        string status = finished.status();

        // completion for pickup
        if (status == "ARRIVE WAREHOUSE") {
            // query the database to get all packages whose state is "wait for pickup"
            // and truck id is the correct one
            vector<int64_t> packageIds;

            // notify amazon the truck has arrived
            for (size_t i = 0; i < packageIds.size(); ++i)
                notifyArrivalToAmazon(finished.truckid(), packageIds[i], seqnum.fetch_add(1));

            // update package table, setting status to "wait for loading"
        }

        // update truck table in database
    }
}

vector<int64_t> BaseServer::extractSeqNums(UResponses &resp) {
    vector<int64_t> seqnums;
    for (int i = 0; i < resp.completions_size(); ++i) {
        seqnums.push_back(resp.completions(i).seqnum());
    }
    for (int i = 0; i < resp.delivered_size(); ++i) {
        seqnums.push_back(resp.delivered(i).seqnum());
    }
    for (int i = 0; i < resp.truckstatus_size(); ++i) {
        seqnums.push_back(resp.truckstatus(i).seqnum());
    }
    for (int i = 0; i < resp.error_size(); ++i) {
        seqnums.push_back(resp.error(i).seqnum());
    }
    return seqnums;
}

void BaseServer::sendTestCommand() {
    requestQueryToWorld(0, seqnum.fetch_add(1));
    requestQueryToWorld(1, seqnum.fetch_add(1));
    requestQueryToWorld(3, seqnum.fetch_add(1));
}

void BaseServer::displayUResponses(UResponses &resp) {
    if (resp.completions_size() != 0) {
        cout << resp.completions_size() << " completions received:\n";
        for (int i = 0; i < resp.completions_size(); ++i) {
            const UFinished &finished = resp.completions(i);
            cout << "Truck ID: " << finished.truckid() << ' ' <<
            "Truck location: (" << finished.x() << ", " << finished.y() << ") " <<
            "Truck status: " << finished.status() << endl;
        }
    }

    if (resp.delivered_size() != 0) {
        cout << resp.delivered_size() << " deliveries done:\n";
        for (int i = 0; i < resp.delivered_size(); ++i) {
            const UDeliveryMade &made = resp.delivered(i);
            cout << "Truck ID: " << made.truckid() << ' ' <<
            "Package ID: " << made.packageid() << ' ' << endl;
        }
    }

    if (resp.acks_size() != 0) {
        cout << resp.acks_size() << " acks received:\n";
        for (int i = 0; i < resp.acks_size(); ++i) {
            cout << "ACK: " << resp.acks(i) << endl;
        }
    }

    if (resp.has_finished()) {
        cout << "World closed connection\n";
    }

    if (resp.truckstatus_size() != 0) {
        cout << resp.truckstatus_size() << " truck status received:\n";
        for (int i = 0; i < resp.truckstatus_size(); ++i) {
            const UTruck &truck = resp.truckstatus(i);
            cout << "Truck ID: " << truck.truckid() << ' ' <<
            "Truck status: " << truck.status() << ' ' <<
            "Truck location: (" << truck.x() << ", " << truck.y() << ")" << endl;
        }
    }

    if (resp.error_size() != 0) {
        cout << resp.error_size() << " errors received:\n";
        for (int i = 0; i < resp.error_size(); ++i) {
            const UErr &err = resp.error(i);
            cout << "Error message: " << err.err() << ' ' <<
            "Origin sequence number: " << err.originseqnum() << endl;
        }
    }
}

void BaseServer::daemonize(){
    // fork, create a child process
    pid_t pid = fork();
    // exit the parent process, guaranteed not to be a process group leader
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        cerr << "During daemonize: First Fork failure\n";
        exit(EXIT_FAILURE);
    }
    // working on the child process
    // create a new session with no controlling tty
    pid_t sid = setsid();
    if (sid == -1) {
        cerr << "During daemonize: Create new session failure\n";
        exit(EXIT_FAILURE);
    }
    // point stdin/stdout/stderr to it
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    
    // change working directory to root
    chdir("/");
    // clear umask
    umask(0);
    // fork again
    pid = fork();
    // exit the parent process, guaranteed not to be a session leader
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        cerr << "During daemonize: Second Fork failure\n";
        exit(EXIT_FAILURE);
    }
}


void BaseServer::addToWaitAcks(int64_t seqnum, UCommands ucom) {
    waitAckMutex.lock();
    requestInfo req;
    req.seqnum = seqnum;
    req.ucom = ucom;
    req.recvTime = time(nullptr);
    waitAcks.push(req);
    unackeds.insert(seqnum);
    waitAckMutex.unlock();
}

// interfaces of request to world
void BaseServer::requestPickUpToWorld(int truckid, int whid, int64_t seqnum) {
    UCommands ucom;
    UGoPickup *pickup = ucom.add_pickups();
    pickup->set_truckid(truckid);
    pickup->set_whid(whid);
    pickup->set_seqnum(seqnum);

    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send pickup message to the world\n";
    } else {
        addToWaitAcks(seqnum, ucom);
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::requestDeliverToWorld(int truckid, std::vector<int64_t> packageids, 
    std::vector<int> xs, std::vector<int> ys, int64_t seqnum) {
    UCommands ucom;
    UGoDeliver *deliver = ucom.add_deliveries();
    deliver->set_truckid(truckid);

    for (size_t i = 0; i < packageids.size(); ++i) {
        UDeliveryLocation *loc = deliver->add_packages();
        loc->set_packageid(packageids[i]);
        loc->set_x(xs[i]);
        loc->set_y(ys[i]);
    }

    deliver->set_seqnum(seqnum);

    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send deliver message to the world\n";
    } else {
        addToWaitAcks(seqnum, ucom);
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::requestQueryToWorld(int truckid, int64_t seqnum) {
    UCommands ucom;
    UQuery *query = ucom.add_queries();
    query->set_truckid(truckid);
    query->set_seqnum(seqnum);
    
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send query message to the world\n";
    } else {
        addToWaitAcks(seqnum, ucom);
    }
    google::protobuf::ShutdownProtobufLibrary();
}

// note that adjust speed doesn't need sequence number
void BaseServer::adjustSimSpeed(unsigned int simspeed) {
    UCommands ucom;
    ucom.set_simspeed(simspeed);
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send sim speed to the world\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

// note that disconnect doesn't need sequence number
void BaseServer::requestDisconnectToWorld() {
    UCommands ucom;
    ucom.set_disconnect(true);
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send disconnect message to the world\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

// note that ACK doesn't need sequence number
void BaseServer::ackToWorld(int64_t ack) {
    UCommands ucom;
    ucom.add_acks(ack);
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send ack to the world\n";
    } 
    google::protobuf::ShutdownProtobufLibrary();
}

// interfaces of request to amazon
// note that for communication between amazon and ups,
// timeout and retransmission are not necessary
void BaseServer::sendWorldIdToAmazon(int64_t worldid, int64_t seqnum) {
    UACommand uacom;
    UASendWorldId *sendId = new UASendWorldId();
    sendId->set_worldid(worldid);
    sendId->set_seqnum(seqnum);
    uacom.set_allocated_sendid(sendId);
    
    int status = sendMesgTo<UACommand>(uacom, amazonOut);
    if (!status) {
        cerr << "Can't send world id to amazon\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::notifyArrivalToAmazon(int truckid, int64_t packageid, int64_t seqnum) {
    UACommand uacom;
    UAArrived *arrived = uacom.add_arrived();
    arrived->set_truckid(truckid);
    arrived->set_packageid(packageid);
    arrived->set_seqnum(seqnum);

    int status = sendMesgTo<UACommand>(uacom, amazonOut);
    if (!status) {
        cerr << "Can't notify amazon the truck arrival info\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::notifyDeliveredToAmazon(int64_t packageid, int64_t seqnum) {
    UACommand uacom;
    UADeliverOver *delover = uacom.add_deliverover();
    delover->set_packageid(packageid);
    delover->set_seqnum(seqnum);

    int status = sendMesgTo<UACommand>(uacom, amazonOut);
    if (!status) {
        cerr << "Can't notify amazon the delivery over info\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::sendErrToAmazon(std::string err, int64_t originseqnum, int64_t seqnum) {
    UAResponses uaresp;
    Err *error = uaresp.add_errors();
    error->set_err(err);
    error->set_originseqnum(originseqnum);
    error->set_seqnum(seqnum);

    int status = sendMesgTo<UAResponses>(uaresp, amazonOut);
    if (!status) {
        cerr << "Can't send error message to amazon\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::sendPickUpResponseToAmazon(int64_t packageid, int64_t seqnum) {
    UAResponses uaresp;
    UAPickupResponse *pickupResp = uaresp.add_pickupresp();
    pickupResp->set_packageid(packageid);
    pickupResp->set_seqnum(seqnum);

    int status = sendMesgTo<UAResponses>(uaresp, amazonOut);
    if (!status) {
        cerr << "Can't send pickup response info to amazon\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void BaseServer::ackToAmazon(int64_t ack) {
    UAResponses uaresp;
    uaresp.add_acks(ack);

    int status = sendMesgTo<UAResponses>(uaresp, amazonOut);
    if (!status) {
        cerr << "Can't send ack to amazon\n";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

// getter functions
int BaseServer::getBacklog() const {
    return backlog;
}
const MySocket *BaseServer::getSocket() const {
    return sock;
}
int64_t BaseServer::getWorldId() const {
    return worldId;
}

// setter functions
void BaseServer::setBacklog(int b) {
    backlog = b;
}
void BaseServer::setSocket(MySocket *s) {
    sock = s;
}
void BaseServer::setWorldId(int64_t id) {
    worldId = id;
}