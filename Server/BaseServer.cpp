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
#include <cassert>
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
#define AMAZON_HOST "vcm-25794.vm.duke.edu"
#define AMAZON_PORT "9999"

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

// indicate if UConnected is received
bool worldIdReady = false;
mutex worldIdReadyMutex;
UConnected connectedResp;

// record the sequence numbers from UResponses which are already processed
unordered_set<int64_t> alreadyAckToWorld;
mutex alreadyAckMutex;

BaseServer::BaseServer(const char *_hostname, 
    const char *_port, int _backlog, int _threadNum) 
    : sock(nullptr), worldSock(nullptr), worldIn(nullptr), worldOut(nullptr), 
    amazonSock(nullptr), backlog(_backlog), threadNum(_threadNum), init(threadNum) {	
	// create, bind, and listen to a socket
	setupServer(_hostname, _port);
    // &db=new Database();
    // db.setup();
}

void BaseServer::setupServer(const char *hostname, const char *port) {
	// create, bind, and listen to a socket
    // this socket is used for communication between frontend and backend
    // e.g. change address
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
    connectToSimWorld();

	// get world id
    setWorldId(getWorldIdFromSim());

    // connect to amazon
    connectToAmazon();
}

void BaseServer::connectToSimWorld() {
    struct addrinfo host_info;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    worldSock = new MySocket(host_info, SIMWORLD_UPS_HOST, SIMWORLD_UPS_PORT);
    worldSock->connectSocket();
    // create socket in/out file stream
    worldOut = new FileOutputStream(worldSock->getSocketFd());
    worldIn = new FileInputStream(worldSock->getSocketFd());
}

void BaseServer::connectToAmazon() {
    // create socket between amazon and ups
    struct addrinfo host_info;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    amazonSock = new MySocket(host_info, AMAZON_HOST, AMAZON_PORT);
    amazonSock->connectSocket();
    // create socket in/out file stream
    amazonOut = new FileOutputStream(amazonSock->getSocketFd());
    amazonIn = new FileInputStream(amazonSock->getSocketFd());
    cout << "Successfully connect to amazon\n";

    // help the amazon connect to the world
    while (true) {
        int64_t seqnumCopy = seqnum.fetch_add(1);
        cout << "Send world id to amazon\n";
        sendWorldIdToAmazon(worldId, seqnumCopy);

        cout << "Wait for response from amazon\n";
        AUResponses resp;
        bool status = recvMesgFrom<AUResponses>(resp, amazonIn);
        if (!status) {
            cerr << "Receive AUResponses from amazon failed. Probably of wrong format.\n";
            throw ConnectToAmazonFailure();
        }
        
        // check if ack is correctly returned
        assert(resp.acks_size() > 0);
        assert(seqnumCopy == resp.acks(0));

        // if received error message, resend the world id
        if (resp.errors_size() != 0) {
            cout << "Received " << resp.errors_size() << " errors.\n";
            for (int i = 0; i < resp.errors_size(); ++i) {
                cout << resp.errors(i).err() << endl;
                ackToAmazon(resp.errors(i).seqnum());
            }
        } 
        // if no error
        else {
            cout << "Amazon connected to the world!\n";
            
            // send back ack
            assert(resp.has_seqnum());
            ackToAmazon(resp.seqnum());
            break;
        }
        cout << "Sending world id again\n";
    }
}

void BaseServer::receiveUConnected() {
    cout << "waiting for world id from sim world.....\n";
    int status = recvMesgFrom<UConnected>(connectedResp, worldIn);
    if (status) {
        worldIdReadyMutex.lock();
        worldIdReady = true;
        worldIdReadyMutex.unlock();
    } else {
        exitWithError("Can't receive world id from sim world.");
    }
}

// note that we need timeout and retransmission here
int64_t BaseServer::getWorldIdFromSim() {
    vector<int> xs = {0,23,323,4,32,4,5,4,4};
    vector<int> ys = {5,1,379,324,3,45,6,34,3};
    // vector<int> xs = {0,23,323};
    // vector<int> ys = {5,1,379};

    UConnect ucon;
    ucon.set_isamazon(false);
    connection *C = db.connectToDatabase();

    for (size_t i = 0; i < xs.size(); ++i) {
        UInitTruck *truck = ucon.add_trucks();
        truck->set_id(i + 1);
        truck->set_x(xs[i]);
        truck->set_y(ys[i]);

        // store the truck in the database
        SQLObject *truckObj = new Truck(loading, xs[i], ys[i]);
        db.insertTables(C, truckObj);
        delete truckObj;
    }

    // this thread waits for the UConnected response
    group.run([=]{receiveUConnected();});

    while (true) {
        bool isReady = false;
        worldIdReadyMutex.lock();
        isReady = worldIdReady;
        worldIdReadyMutex.unlock();
        if (isReady) {
            break;
        }
        sendMesgTo<UConnect>(ucon, worldOut);
        sleep(2);
    }

    if (connectedResp.result() == "connected!") {
        cout << "Connected to world: " << connectedResp.worldid() << endl;
    } else {
        cout << "Something went wrong when receiving world id from sim world" << endl;
    }

    
    return connectedResp.worldid();
}

BaseServer::~BaseServer() {
    delete sock;
    delete worldSock;
    delete amazonSock;
    delete worldIn;
    delete worldOut;
    delete amazonIn;
    delete amazonOut;
}

void BaseServer::launch() {
	// daemonize();

    // one thread for receiving responses from sim world
    group.run([=]{simWorldCommunicate();});
    // one or more threads for communicating with amazon
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

    // cout << "Sending test command\n";
    // sendTestCommand();

    while (1) {
        cout << "Waiting for responses\n";
        UResponses resp;

        // bool status = recvMesgFrom<UResponses>(resp, worldIn);
        cout << "Constructing test response\n";
        bool status = true;
        getTestResponse(resp);

        if (status) {
            // get all sequence numbers
            vector<int64_t> seqnums = extractSeqNums(resp);
            // send back acks
            ackToWorld(seqnums);

            // displaying responses
            // displayUResponses(resp);

            // upon receiving different responses, update the database, and do other related stuff
            // do stuff related to completions
            processCompletions(C, resp);
            // do stuff related to delivered
            processDelivered(C, resp);
            // do stuff related to truckstatus
            processTruckStatus(C, resp);
            // do stuff related to error
            processErrors(resp);
            // do stuff related to acks
            processAcks(resp);
            // If the world set finished, then we would not receive responses from the world again, just exit the while loop
            if (resp.has_finished() && resp.finished()) {
                break;
            }
        } else {
            cerr << "The message received is of wrong format. Can't parse it. So this would be skipped.\n";
        }

        // cout << "Results of errCommands and errFreeCommands:\n";
        // for (auto cmd : errorFreeCmds) {
        //     cout << cmd << ' ';
        // }
        // cout << endl;
        // for (auto p : errorCmds) {
        //     cout << p.first << ": " << p.second << ", ";
        // }
        // cout << endl;
        break;
    }

    google::protobuf::ShutdownProtobufLibrary();
	C->disconnect();
	delete C;
}

bool BaseServer::checkIfAlreadyProcessed(int64_t seq) {
    bool processed = false;
    alreadyAckMutex.lock();
    // if this response has already been processed
    if (alreadyAckToWorld.find(seq) != alreadyAckToWorld.end()) {
        processed = true;
    } else {
        alreadyAckToWorld.insert(seq);
    }
    alreadyAckMutex.unlock();

    return processed;
}

void BaseServer::processErrors(UResponses &resp) {
    for (int i = 0; i < resp.error_size(); ++i) {
        const UErr &err = resp.error(i);

        bool processed = checkIfAlreadyProcessed(err.seqnum());
        if (processed)
            continue;

        // display the error
        cerr << err.err() << " when dealing with request " << err.originseqnum() << endl;
    }
}

void BaseServer::processAcks(UResponses &resp) {
    waitAckMutex.lock();
    // add sequence number to error commands
    for (int i = 0; i < resp.error_size(); ++i) {
        int64_t seq = resp.error(i).originseqnum();
        errorCmds[seq] = resp.error(i).err();
    }

    // remove the corresponding sequence number in unackeds
    for (int i = 0; i < resp.acks_size(); ++i) {
        // if has indeed not been acked
        if (unackeds.find(resp.acks(i)) != unackeds.end())
            unackeds.erase(resp.acks(i));
        
        // add sequence number to error free commands
        if (errorCmds.find(resp.acks(i)) == errorCmds.end())
            errorFreeCmds.insert(resp.acks(i));
    }

    waitAckMutex.unlock();
}

void BaseServer::processTruckStatus(connection *C, UResponses &resp) {
    // update the truckStatus
    for (int i = 0; i < resp.truckstatus_size(); ++i) {
        const UTruck &truck = resp.truckstatus(i);
        
        bool processed = checkIfAlreadyProcessed(truck.seqnum());
        if (processed)
            continue;
        // update truck table
        truck_status_t status;
        if (!getCorrespondTruckStatus(truck.status(), status)) {
            cerr << "Received unknown truck status: " << truck.status() << endl;
            continue;
        }
           
        db.updateTruck(C, status, truck.x(), truck.y(), truck.truckid());
    }
}

void BaseServer::processDelivered(connection *C, UResponses &resp) {
    for (int i = 0; i < resp.delivered_size(); ++i) {
        const UDeliveryMade &deliveryMade = resp.delivered(i);
        
        bool processed = checkIfAlreadyProcessed(deliveryMade.seqnum());
        if (processed)
            continue;
        
        // notify amazon the package has been delivered
        notifyDeliveredToAmazon(deliveryMade.packageid(), seqnum.fetch_add(1));

        // update the package table, setting the status to be "delivered"
        db.updatePackage(C, delivered, deliveryMade.packageid());
    }
}

void BaseServer::processCompletions(connection *C, UResponses &resp) {
    for (int i = 0; i < resp.completions_size(); ++i) {
        const UFinished &finished = resp.completions(i);
        
        bool processed = checkIfAlreadyProcessed(finished.seqnum());
        if (processed)
            continue;

        string status = finished.status();
        cout << "While processing completions, truck status is received: " << status << endl;

        // completion for pickup
        if (status == "ARRIVE WAREHOUSE") {
            // query the database to get the warehouse id
            int warehouseId = db.queryWarehouseId(C, finished.x(), finished.y());

            // query the database to get all packages whose state is "wait for pickup"
            // and truck id is the correct one, warehouse id is the correct one
            vector<int64_t> trackingNums = db.queryTrackingNumToPickUp(C, finished.truckid(), warehouseId);

            // notify amazon the truck has arrived
            // update package table, setting status to "wait for loading"
            for (size_t i = 0; i < trackingNums.size(); ++i) {
                notifyArrivalToAmazon(finished.truckid(), trackingNums[i], seqnum.fetch_add(1));
                db.updatePackage(C, wait_for_loading, trackingNums[i]);
            }
        }

        // update truck table in database
        truck_status_t newStatus;
        if (!getCorrespondTruckStatus(status, newStatus)) {
            cerr << "Received unknown truck status: " << status << endl;
            continue;
        }
        db.updateTruck(C, newStatus, finished.x(), finished.y(), finished.truckid());
    }
}

bool BaseServer::getCorrespondTruckStatus(const string &status, truck_status_t &newStatus) {
    if (status == "IDLE")
        newStatus = idle;
    else if (status == "TRAVELING")
        newStatus = traveling;
    else if (status == "ARRIVE WAREHOUSE")
        newStatus = arrive_warehouse;
    else if (status == "LOADING")
        newStatus = loading;
    else if (status == "DELIVERING")
        newStatus = delivering;
    else 
        return false;
    
    return true;
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
    requestQueryToWorld(2, seqnum.fetch_add(1));
    requestQueryToWorld(1, seqnum.fetch_add(1));
    requestQueryToWorld(3, seqnum.fetch_add(1));
}

void BaseServer::getTestResponse(UResponses &resp) {
    UFinished *finished = resp.add_completions();
    finished->set_truckid(7);
    finished->set_x(999);
    finished->set_y(999);
    finished->set_status("ARRIVE WAREHOUSE");
    finished->set_seqnum(233);

    finished = resp.add_completions();
    finished->set_truckid(5);
    finished->set_x(-999);
    finished->set_y(-999);
    finished->set_status("IDLE");
    finished->set_seqnum(466);
    
    // UTruck *truck = resp.add_truckstatus();
    // truck->set_truckid(6);
    // truck->set_status("ARRIVE WAREHOUSE");
    // truck->set_x(0);
    // truck->set_y(0);
    // truck->set_seqnum(233);

    // truck = resp.add_truckstatus();
    // truck->set_truckid(9);
    // truck->set_status("TRAVELING");
    // truck->set_x(-1);
    // truck->set_y(-1);
    // truck->set_seqnum(677);
    
    // UErr *err = resp.add_error();
    // err->set_err("Testing error1!!!");
    // err->set_originseqnum(333);
    // err->set_seqnum(355);

    // err = resp.add_error();
    // err->set_err("Testing error2!!!");
    // err->set_originseqnum(23);
    // err->set_seqnum(87);

    // resp.add_acks(100);
    // resp.add_acks(333);
    // resp.add_acks(23);
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
}

// note that adjust speed doesn't need sequence number
void BaseServer::adjustSimSpeed(unsigned int simspeed) {
    UCommands ucom;
    ucom.set_simspeed(simspeed);
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send sim speed to the world\n";
    }
}

// note that disconnect doesn't need sequence number
void BaseServer::requestDisconnectToWorld() {
    UCommands ucom;
    ucom.set_disconnect(true);
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send disconnect message to the world\n";
    }
}

// note that ACK doesn't need sequence number
void BaseServer::ackToWorld(vector<int64_t> acks) {
    UCommands ucom;
    for (int64_t ack : acks) {
        ucom.add_acks(ack);
    }
    int status = sendMesgTo<UCommands>(ucom, worldOut);
    if (!status) {
        cerr << "Can't send ack to the world\n";
    }
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
}

void BaseServer::notifyArrivalToAmazon(int truckid, int64_t trackingnum, int64_t seqnum) {
    UACommand uacom;
    UAArrived *arrived = uacom.add_arrived();
    arrived->set_truckid(truckid);
    arrived->set_trackingnum(trackingnum);
    arrived->set_seqnum(seqnum);

    int status = sendMesgTo<UACommand>(uacom, amazonOut);
    if (!status) {
        cerr << "Can't notify amazon the truck arrival info\n";
    }
}

void BaseServer::notifyDeliveredToAmazon(int64_t trackingnum, int64_t seqnum) {
    UACommand uacom;
    UADeliverOver *deliver = uacom.add_deliverover();
    deliver->set_trackingnum(trackingnum);
    deliver->set_seqnum(seqnum);

    int status = sendMesgTo<UACommand>(uacom, amazonOut);
    if (!status) {
        cerr << "Can't notify amazon the delivery over info\n";
    }
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
}

void BaseServer::ackToAmazon(int64_t ack) {
    UAResponses uaresp;
    uaresp.add_acks(ack);

    int status = sendMesgTo<UAResponses>(uaresp, amazonOut);
    if (!status) {
        cerr << "Can't send ack to amazon\n";
    }
}

int findTruck(int x,int y){
    //need update truck first?
    //query db to find closest available truck?
    return 0;
}

// // process request from Amazon
// void BaseServer::processAmazonPickup(AUCommand &aResq){
//     for (int i=0;i<aResq.pickup_size();i++){
//         AURequestPickup * pickup=aResq.pickup(i);
//         //if (pickup->has_upsid());
//         int truckId=findTruck(pickup->wareinfo().x,pickup->wareinfo().y);
//         Package * newPackage=new Package(,truckId,pickup->upsid());//need status, what if not have?
//         db.sql_insert(newPackage);
//         for (int j=0;j<pickup->things_size;j++){
//             AProduct * newProduct=pickup->things(j);
//             Item * newItem=new Item(newProduct->id,newProduct->description(),newProduct->count,newPackage->getPackageId());
//             db.sql_insert(newItem);
//         }
//         //sendPickupResponse
//         requestPickUpToWorld(truckid, pickup->wareinfo().id, seqnum);//what if this truck not available
//     }
// }
// void BaseServer::processAmazonLoaded(AUCommand &aResq){
//     for (int i=0;i<aResq.packloaded_size();i++){
//         AULoadOver * loadOver=aResq.packloaded(i);
//         //update status per packageid 
//         //update X, Y per packageid 
//         //update status per truckid

//         //sendLoadedResponse
        
//     }
//     requestDeliverToWorld(truckid, <>packageid, seqnum);//when to do that, all the package loaded?
// }
// void BaseServer::processAmazonChangeAdd(AUCommand &aResq){
//     for (int i=0;i<aResq.changeaddr_size();i++){
//         AUChangeAddress * changeAddress=aResq.changeaddr(i);
//         int destX=changeAddress->loc().x;
//         int destY=changeAddress->loc().y;
//         //query status per packageid
//         //if not available, send error
        
//         //else update X, Y per packageid         
//         //ackToAmazon();
//     }
// }


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