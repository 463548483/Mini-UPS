#include "BaseServer.hpp"
#include "../DataBase/Database.hpp"
#include "../Utils/utils.hpp"
#include <string>
#include <vector>
#include <iomanip>
#include <sys/time.h>
#include <sstream>
#include <fstream>
#include <cmath>
#include <mutex>
#include <thread>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <boost/thread.hpp>
#include <fcntl.h>
#include <chrono>

using namespace std;
using namespace pqxx;
using namespace tbb;
using namespace std::chrono;
using namespace google::protobuf::io;

#define MAX_LIMIT 65536
#define SIMWORLD_UPS_HOST "vcm-25032.vm.duke.edu"
#define SIMWORLD_UPS_PORT "12345"

BaseServer::BaseServer(const char *_hostname, 
    const char *_port, int _backlog, int _threadNum) 
    : sock(nullptr), worldSock(nullptr), worldIn(nullptr), worldOut(nullptr), amazonSock(nullptr), backlog(_backlog), threadNum(_threadNum) {	
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
        cout << "Connected to world: " << resp.worldid();
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
	// initialize thread pool
	tbb::task_scheduler_init init(threadNum);
	task_group group;

    // one thread for communicating with sim world
    group.run([=]{simWorldCommunicate();});
    // one thread for communicating with amazon
    group.run([=]{amazonCommunicate();});

	group.wait();
}

// logic of communication with sim world
void BaseServer::simWorldCommunicate() {
	// std::cout << boost::this_thread::get_id() << endl;
	// connect to database, need a new connection for each thread
	connection *C = db.connectToDatabase();

    while (1) {
        cout << "Sending test command\n";
        sendTestCommand();

        cout << "Waiting for responses\n";
        UResponses resp;
        bool status = recvMesgFrom<UResponses>(resp, worldIn);

        if (status) {
            // processing
            displayUResponses(resp);
        } else {
            exitWithError("Failed to receive the google protocol buffer message\n");
        }
        break;
    }
	// UCommands cmd;
    // !!!!!!!!!!! change to worldIn
    // FileInputStream *in = new FileInputStream(client_sock->getSocketFd());
    // bool status = recvMesgFrom<UCommands>(cmd, in);
    // delete in;

    // // display message
    // if (status) {
    //     cout << "disconnect set or not: " << cmd.has_disconnect() << endl;
    //     cout << "ACKs: ";
    //     for (int i = 0; i < cmd.acks_size(); ++i) {
    //         cout << cmd.acks(i) << ' ';
    //     }
    //     cout << endl;
    //     cout << "Deliveries: truck id: " << cmd.deliveries(0).truckid() << 
    //     " , packages: x = " << cmd.deliveries(0).packages(0).x() << 
    //     " y = " << cmd.deliveries(0).packages(0).y() << endl;
    // } else {
    //     exitWithError("Failed to receive the google protocol buffer message\n");
    // }

	C->disconnect();
	delete C;
}

// logic of communication with sim world
void BaseServer::amazonCommunicate() {
    // big while loop
    // while (1) {
        // wait for notification

        // processing order and update database
        
    // }
}

void BaseServer::sendTestCommand() {
    UCommands ucom;
    
    UGoPickup *pick = ucom.add_pickups();
    pick->set_truckid(0);
    pick->set_whid(0);
    pick->set_seqnum(12);

    sendMesgTo<UCommands>(ucom, worldOut);

    google::protobuf::ShutdownProtobufLibrary();
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

    }

    if (resp.has_finished()) {
        
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