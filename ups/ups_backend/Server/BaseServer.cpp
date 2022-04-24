#include "BaseServer.hpp"

#include <boost/thread.hpp>
#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <sys/time.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../Utils/utils.hpp"

using json = nlohmann::json;
using namespace std;
using namespace pqxx;
using namespace tbb;
using namespace std::chrono;
using namespace google::protobuf::io;

#define MAX_LIMIT 65536
#define TIME_OUT_LIMIT 1
#define SIMWORLD_UPS_HOST "vcm-26825.vm.duke.edu"
// #define SIMWORLD_UPS_PORT "23456"
#define SIMWORLD_UPS_PORT "12345"
#define AMAZON_HOST "vcm-26766.vm.duke.edu"
#define AMAZON_PORT "8999"

struct requestInfo {
  int64_t seqnum;
  UCommands ucom;
  time_t recvTime;
};

class MyCmp {
 public:
  bool operator()(const requestInfo & r1, const requestInfo & r2) const {
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

// map for packages need to find trucks, key=warehouseId, value=list of trackingNum
unordered_map<int, list<int64_t> > warehousePkgMap;
mutex findTruckMutex;
unordered_set<int64_t> amazonReq;
mutex amazonReqMutex;
unordered_map<int, vector<UDeliveryLocation> > truckPackageMap;

// initial trucks location
vector<int> truckInitX = {0, -23, 323, 4, 32, 4, 50, 4, 4};
vector<int> truckInitY = {20, 1, 379, 324, 32, 45, 26, 34, 3};

BaseServer::BaseServer(const char * _hostname,
                       const char * _port,
                       int _backlog,
                       int _threadNum) :
    sock(nullptr),
    worldSock(nullptr),
    worldIn(nullptr),
    worldOut(nullptr),
    amazonSock(nullptr),
    backlog(_backlog),
    threadNum(_threadNum),
    init(threadNum) {
  // create, bind, and listen to a socket
  setupServer(_hostname, _port);
}

void BaseServer::setupServer(const char * hostname, const char * port) {
  // create, bind, and listen to a socket
  // this socket is used for communication between frontend and backend
  // e.g. change address
  struct addrinfo host_info;

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  sock = new MySocket(host_info, hostname, port);
  setSocket(sock);
  sock->bindSocket();
  sock->listenSocket(backlog);

  // connect to sim world
  connectToSimWorld();
  cout << "Successfully connected to simulation world\n";

  // get world id
  setWorldId(getWorldIdFromSim());

  // connect to amazon
  connectToAmazon();
}

void BaseServer::connectToSimWorld() {
  struct addrinfo host_info;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
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
  host_info.ai_family = AF_UNSPEC;
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
    AUCommand resp;
    try {
      bool status = recvMesgFrom<AUCommand>(resp, amazonIn);
      cout << "Received " << resp.ShortDebugString() << endl;
      if (!status) {
        cerr << "Receive AUCommand from amazon failed. Probably of wrong format.\n";
        throw ConnectToAmazonFailure();
      }

      // check if ack is correctly returned
      if (resp.acks_size() <= 0) {
        cerr << "Receive AUCommand doesn't contain ACK\n";
        throw ConnectToAmazonFailure();
      }
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
        if (!resp.has_seqnum()) {
          cerr << "Receive AUCommand doesn't contain seqnum, unable to ack\n";
          throw ConnectToAmazonFailure();
        }
        ackToAmazon(resp.seqnum());
        break;
      }
      cout << "Sending world id again\n";
      break;
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
    }
  }
}

void BaseServer::receiveUConnected() {
  cout << "waiting for world id from sim world.....\n";
  int status = recvMesgFrom<UConnected>(connectedResp, worldIn);
  if (status) {
    worldIdReadyMutex.lock();
    worldIdReady = true;
    worldIdReadyMutex.unlock();
  }
  else {
    exitWithError("Can't receive world id from sim world.");
  }
}

// note that we need timeout and retransmission here
int64_t BaseServer::getWorldIdFromSim() {
  
  
  UConnect ucon;
  ucon.set_isamazon(false);
  connection * C = db.connectToDatabase();

  for (size_t i = 0; i < truckInitX.size(); ++i) {
    UInitTruck * truck = ucon.add_trucks();
    truck->set_id(i + 1);
    truck->set_x(truckInitX[i]);
    truck->set_y(truckInitY[i]);

    // store the truck in the database
    SQLObject * truckObj = new Truck(idle, truckInitX[i], truckInitY[i]);
    db.insertTables(C, truckObj);
    delete truckObj;
  }

  // this thread waits for the UConnected response
  group.run([=] { receiveUConnected(); });

  while (true) {
    bool isReady = false;
    worldIdReadyMutex.lock();
    isReady = worldIdReady;
    worldIdReadyMutex.unlock();
    if (isReady) {
      break;
    }
    cout << ucon.ShortDebugString() << endl;
    sendMesgTo<UConnect>(ucon, worldOut);
    sleep(2);
  }

  cout << connectedResp.ShortDebugString() << endl;
  if (connectedResp.result() == "connected!") {
    cout << "Connected to world: " << connectedResp.worldid() << endl;
  }
  else {
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

  // one thread for communicating with front-end
  group.run([=] { frontendCommunicate(); });
  // one thread for receiving responses from sim world
  group.run([=] { simWorldCommunicate(); });
  // one or more threads for communicating with amazon
  group.run([=] { amazonCommunicate(); });
  // one thread for implementing timeout and retransmission mechanism
  group.run([=] { timeoutAndRetransmission(); });
  // periodically update truck status
  group.run([=] { periodicalUpdateTruckStatus(); });

  group.wait();
}

void BaseServer::periodicalUpdateTruckStatus() {
  while (1) {
    sleep(2);

    for (int i = 0; i < (int)truckInitX.size(); ++i) {
      requestQueryToWorld(i + 1, seqnum.fetch_add(1));
    }
  }
}

void BaseServer::frontendCommunicate() {
  connection * C = db.connectToDatabase();

  while (1) {
    // accept connection
    MySocket * backendSock = sock->acceptConnection();
    cout << "Accepting connection from back-end\n";
    string req = backendSock->receiveData();
    // if really received data
    if (req.size() != 0) {
      json req_parsed = json::parse(req);

      // change address request
      if (req_parsed["request"] == "Change Address") {
        int truckId = req_parsed["truck_id"];
        vector<int64_t> packageIds = {req_parsed["tracking_num"]};
        vector<int> xs = {req_parsed["new_x"]};
        vector<int> ys = {req_parsed["new_y"]};
        vector<UDeliveryLocation> locs;
        for (size_t i = 0; i < packageIds.size(); i++) {
          UDeliveryLocation loc;
          loc.set_packageid(packageIds[i]);
          loc.set_x(xs[i]);
          loc.set_y(ys[i]);
          locs.push_back(loc);
        }

        int64_t seq = seqnum.fetch_add(1);
        requestDeliverToWorld(truckId, locs, seq);

        // send fake response
        // update the package table in database
        // for (auto loc : locs) {
        //   db.updatePackage(C, loc.x(), loc.y(), loc.packageid());
        // }

        // stringstream ss;
        // ss << "{\"result\": \"success\", \"info\": \"Address change successfully. You can refresh the page to see the change.\"}";
        // backendSock->sendData(ss.str());
        // wait for response from world
        while (1) {
          sleep(1);

          waitAckMutex.lock();
          if (errorCmds.find(seq) != errorCmds.end()) {
            stringstream ss;
            ss << "{\"result\": \"failure\", \"error\": \"" << errorCmds[seq] << "\"}";
            backendSock->sendData(ss.str());
            waitAckMutex.unlock();
            break;
          }
          else if (errorFreeCmds.find(seq) != errorFreeCmds.end()) {
            stringstream ss;
            ss << "{\"result\": \"success\", \"info\": \"Address change successfully..\"}";
            backendSock->sendData(ss.str());

            // update the package table in database
            for (auto loc : locs) {
              db.updatePackage(C, loc.x(), loc.y(), loc.packageid());
            }

            waitAckMutex.unlock();
            break;
          }
          waitAckMutex.unlock();
        }
      }
      else {
        cerr << "Unknown request. Ignoring it.\n";
      }
    }

    delete backendSock;
  }

  C->disconnect();
  delete C;
}

void BaseServer::timeoutAndRetransmission() {
  while (1) {
    // wait for several seconds
    sleep(4);

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
      // cout << "Request with sequence #: " << req.seqnum
      //      << " timed out. Retransmitting...\n";
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
  //mutiple thread for truck finding
  group.run([=] { findTrucks(); });
  group.run([=] { findTrucks(); });
  //group.run([=] { findTrucks(); });

  // big while loop
  connection * C = db.connectToDatabase();
  while (1) {
    try {
      cout << "Wait for Amazon command" << endl;
      AUCommand acommd;
      //getTestAUCommand(acommd);
      bool status = recvMesgFrom<AUCommand>(acommd, amazonIn);
      cout<<acommd.ShortDebugString()<<endl;
      if (!status) {
        cerr << "Receive AUCommand from amazon failed.\n";
        break;
      } else {
        processAmazonUpsQuery(C, acommd);
        processAmazonPickup(C, acommd);
        processAmazonLoaded(C, acommd);
      }
    }
    catch (const google::protobuf::FatalException & e){
      std::cerr << e.what() << '\n';
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
    }

  }
  cout << "End connection with Amazon" << endl;
  C->disconnect();
  delete C;
}

// logic of communication with sim world
void BaseServer::simWorldCommunicate() {
  // std::cout << boost::this_thread::get_id() << endl;
  // connect to database, need a new connection for each thread
  connection * C = db.connectToDatabase();

  // set sim world speed, default is 100
  adjustSimSpeed(10000);

  // cout << "Sending test command\n";
  // sendTestCommand();

  while (1) {
    cout << "Waiting for responses\n";
    UResponses resp;

    try {
      bool status = recvMesgFrom<UResponses>(resp, worldIn);
      // cout << "Constructing test response\n";
      // bool status = true;
      // getTestResponse(resp);

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
      }
      else {
        cerr << "The message received is of wrong format. Can't parse it. So this would "
                "be skipped.\n";
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
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
    }
  }
  // cout << "Finished the thread\n";
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
  }
  else {
    alreadyAckToWorld.insert(seq);
  }
  alreadyAckMutex.unlock();

  return processed;
}

void BaseServer::processErrors(UResponses & resp) {
  for (int i = 0; i < resp.error_size(); ++i) {
    const UErr & err = resp.error(i);

    bool processed = checkIfAlreadyProcessed(err.seqnum());
    if (processed)
      continue;

    // display the error
    cerr << err.err() << " when dealing with request " << err.originseqnum() << endl;
  }
}

void BaseServer::processAcks(UResponses & resp) {
  waitAckMutex.lock();
  // add sequence number to error commands
  for (int i = 0; i < resp.error_size(); ++i) {
    int64_t seq = resp.error(i).originseqnum();
    errorCmds[seq] = resp.error(i).err();
  }

  // remove the corresponding sequence number in unackeds
  for (int i = 0; i < resp.acks_size(); ++i) {
    // add sequence number to error free commands
    if (errorCmds.find(resp.acks(i)) == errorCmds.end()) {
      errorFreeCmds.insert(resp.acks(i));
    }
    // if has indeed not been acked
    if (unackeds.find(resp.acks(i)) != unackeds.end()) {
      unackeds.erase(resp.acks(i));
    }
  }

  waitAckMutex.unlock();
}

void BaseServer::processTruckStatus(connection * C, UResponses & resp) {
  // update the truckStatus
  for (int i = 0; i < resp.truckstatus_size(); ++i) {
    const UTruck & truck = resp.truckstatus(i);

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

void BaseServer::processDelivered(connection * C, UResponses & resp) {
  for (int i = 0; i < resp.delivered_size(); ++i) {
    const UDeliveryMade & deliveryMade = resp.delivered(i);

    bool processed = checkIfAlreadyProcessed(deliveryMade.seqnum());
    if (processed)
      continue;

    // notify amazon the package has been delivered
    notifyDeliveredToAmazon(deliveryMade.packageid(), seqnum.fetch_add(1));

    // update the package table, setting the status to be "delivered"
    db.updatePackage(C, delivered, deliveryMade.packageid());
  }
}

void BaseServer::processCompletions(connection * C, UResponses & resp) {
  for (int i = 0; i < resp.completions_size(); ++i) {
    const UFinished & finished = resp.completions(i);

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
      vector<int64_t> trackingNums =
          db.queryTrackingNumToPickUp(C, finished.truckid(), warehouseId);

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

bool BaseServer::getCorrespondTruckStatus(const string & status,
                                          truck_status_t & newStatus) {
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

vector<int64_t> BaseServer::extractSeqNums(UResponses & resp) {
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

void BaseServer::getTestResponse(UResponses & resp) {
  UFinished * finished = resp.add_completions();
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

void BaseServer::displayUResponses(UResponses & resp) {
  if (resp.completions_size() != 0) {
    cout << resp.completions_size() << " completions received:\n";
    for (int i = 0; i < resp.completions_size(); ++i) {
      const UFinished & finished = resp.completions(i);
      cout << "Truck ID: " << finished.truckid() << ' ' << "Truck location: ("
           << finished.x() << ", " << finished.y() << ") "
           << "Truck status: " << finished.status() << endl;
    }
  }

  if (resp.delivered_size() != 0) {
    cout << resp.delivered_size() << " deliveries done:\n";
    for (int i = 0; i < resp.delivered_size(); ++i) {
      const UDeliveryMade & made = resp.delivered(i);
      cout << "Truck ID: " << made.truckid() << ' ' << "Package ID: " << made.packageid()
           << ' ' << endl;
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
      const UTruck & truck = resp.truckstatus(i);
      cout << "Truck ID: " << truck.truckid() << ' ' << "Truck status: " << truck.status()
           << ' ' << "Truck location: (" << truck.x() << ", " << truck.y() << ")" << endl;
    }
  }

  if (resp.error_size() != 0) {
    cout << resp.error_size() << " errors received:\n";
    for (int i = 0; i < resp.error_size(); ++i) {
      const UErr & err = resp.error(i);
      cout << "Error message: " << err.err() << ' '
           << "Origin sequence number: " << err.originseqnum() << endl;
    }
  }
}

void BaseServer::daemonize() {
  // fork, create a child process
  pid_t pid = fork();
  // exit the parent process, guaranteed not to be a process group leader
  if (pid != 0) {
    exit(EXIT_SUCCESS);
  }
  else if (pid == -1) {
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
  }
  else if (pid == -1) {
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
  UGoPickup * pickup = ucom.add_pickups();
  pickup->set_truckid(truckid);
  pickup->set_whid(whid);
  pickup->set_seqnum(seqnum);

  int status = sendMesgTo<UCommands>(ucom, worldOut);
  if (!status) {
    cerr << "Can't send pickup message to the world\n";
  }
  else {
    addToWaitAcks(seqnum, ucom);
  }
}

void BaseServer::requestDeliverToWorld(int truckid,
                                       const vector<UDeliveryLocation> & packages,
                                       int64_t seqnum) {
  UCommands ucom;
  UGoDeliver * deliver = ucom.add_deliveries();
  deliver->set_truckid(truckid);

  for (size_t i = 0; i < packages.size(); ++i) {
    UDeliveryLocation * loc = deliver->add_packages();
    *loc = packages[i];
  }

  deliver->set_seqnum(seqnum);

  int status = sendMesgTo<UCommands>(ucom, worldOut);
  if (!status) {
    cerr << "Can't send deliver message to the world\n";
  }
  else {
    addToWaitAcks(seqnum, ucom);
  }
}

void BaseServer::requestQueryToWorld(int truckid, int64_t seqnum) {
  UCommands ucom;
  UQuery * query = ucom.add_queries();
  query->set_truckid(truckid);
  query->set_seqnum(seqnum);

  int status = sendMesgTo<UCommands>(ucom, worldOut);
  if (!status) {
    cerr << "Can't send query message to the world\n";
  }
  else {
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
  UASendWorldId * sendId = new UASendWorldId();
  sendId->set_worldid(worldid);
  sendId->set_seqnum(seqnum);
  uacom.set_allocated_sendid(sendId);
  cout << uacom.ShortDebugString() << endl;
  int status = sendMesgTo<UACommand>(uacom, amazonOut);
  if (!status) {
    cerr << "Can't send world id to amazon\n";
  }
}

void BaseServer::notifyArrivalToAmazon(int truckid, int64_t packageid, int64_t seqnum) {
  UACommand uacom;
  UAArrived * arrived = uacom.add_arrived();
  arrived->set_truckid(truckid);
  arrived->set_trackingnum(packageid);
  arrived->set_seqnum(seqnum);

  int status = sendMesgTo<UACommand>(uacom, amazonOut);
  cout << uacom.ShortDebugString()<<endl;
  if (!status) {
    cerr << "Can't notify amazon the truck arrival info\n";
  }
}

void BaseServer::notifyDeliveredToAmazon(int64_t packageid, int64_t seqnum) {
  UACommand uacom;
  UADeliverOver * delover = uacom.add_deliverover();
  delover->set_trackingnum(packageid);
  delover->set_seqnum(seqnum);

  int status = sendMesgTo<UACommand>(uacom, amazonOut);
  if (!status) {
    cerr << "Can't notify amazon the delivery over info\n";
  }
}

void BaseServer::sendErrToAmazon(std::string err, int64_t originseqnum, int64_t seqnum) {
  UACommand uaresp;
  Err * error = uaresp.add_errors();
  error->set_err(err);
  error->set_originseqnum(originseqnum);
  error->set_seqnum(seqnum);

  int status = sendMesgTo<UACommand>(uaresp, amazonOut);
  if (!status) {
    cerr << "Can't send error message to amazon\n";
  }
}

void BaseServer::ackToAmazon(int64_t ack) {
  UACommand uaresp;
  uaresp.add_acks(ack);

  int status = sendMesgTo<UACommand>(uaresp, amazonOut);
  if (!status) {
    cerr << "Can't send ack to amazon\n";
  }
}

bool BaseServer::waitWorldProcess(int64_t seqNum) {
  while (true) {
    if (unackeds.find(seqNum) != unackeds.end()) {
      sleep(10);
      continue;
    }
    else {
      cout << "Checking if the sequence number is in errorCmds or errorFreeCmds\n";
      if (errorCmds.find(seqNum) != errorCmds.end()) {
        cout << "The sequence number is in errorCmds\n";
        errorCmds.erase(seqNum);
        return false;
      }
      else {
        if (errorFreeCmds.find(seqNum) != errorFreeCmds.end()) {
          cout << "The sequence number is in errorFreeCmds\n";
          errorFreeCmds.erase(seqNum);
          return true;
        }
        else {
          cerr << "World Response either not in error or not in errorFree set";
          return false;
        }
      }
    }
  }
}

int BaseServer::findTrucks() {
  //update status for all trucks in database
  // list<int> truckIds = queryTrucks();
  // unordered_set<int64_t> querySet;
  // for (auto & truckId : truckIds) {
  //   int queryNum = seqnum.fetch_add(1);
  //   querySet.insert(queryNum);
  //   requestQueryToWorld(truckId, queryNum);
  // }
  //wait until truck data updated
  // for (auto & queryNum:querySet){
  //   waitWorldProcess(queryNum);
  // }
  //depend truck status through whether it has packages to pick up
  pqxx::connection * C = db.connectToDatabase();
  while (true) {
    if (warehousePkgMap.size() > 0) {
      findTruckMutex.lock();
      if (warehousePkgMap.size() == 0) {
        findTruckMutex.unlock();
        sleep(10);
        continue;
      }
      else {
        cout << "Ready to find a truck\n";
        unordered_map<int, list<int64_t> >::iterator it = warehousePkgMap.begin();
        int warehouseId = it->first;
        list<int64_t> trackingNums = it->second;
        warehousePkgMap.erase(it->first);
        cout << "After erasing from warehousePkgMap, it size is: "
             << warehousePkgMap.size() << endl;
        findTruckMutex.unlock();

        //find the closest truck if without other package to pick up
        while (true) {
          cout << "Query Truck" << endl;
          int closestTruckId = db.queryAvailableTrucksPerDistance(C, warehouseId);
          if (closestTruckId == 0) {
            cout << "cannot find available trucks currently" << endl;
            continue;
          }
          int queryNum = seqnum.fetch_add(1);
          //db.updateTruck(C, traveling, closestTruckId);
          cout << "Found the closest truck id: " << closestTruckId << endl;
          cout << "Resquest warehouse id: " << warehouseId << endl;
          requestPickUpToWorld(closestTruckId, warehouseId, queryNum);
          cout << "wait for world" << endl;
          if (waitWorldProcess(queryNum) == false) {
            cout << "Request truck pick up fail" << endl;
            continue;
          }
          for (auto const & trackingNum : trackingNums) {
            db.updatePackage(C, wait_for_loading, closestTruckId, trackingNum);
            notifyArrivalToAmazon(closestTruckId, trackingNum, seqnum.fetch_add(1));
          }
          break;
        }
      }
    }

    else {
      sleep(10);
    }
  }
}

void BaseServer::getTestAUCommand(AUCommand & aResq) {
  AURequestPickup * pickup = aResq.add_pickup();
  pickup->set_seqnum(1001);
  pickup->set_trackingnum(120393503);
  AProduct * aProduct = pickup->add_things();
  aProduct->set_id(12);
  aProduct->set_description("clothss");
  aProduct->set_count(3);
  AProduct * aProduct2 = pickup->add_things();
  aProduct2->set_id(13);
  aProduct2->set_description("ss");
  aProduct2->set_count(4);
  Warehouse * warehouse = new Warehouse();
  warehouse->set_id(99);
  warehouse->set_x(99);
  warehouse->set_y(99);
  pickup->set_allocated_wareinfo(warehouse);
  AURequestPickup * pickup2 = aResq.add_pickup();
  pickup2->set_seqnum(1002);
  pickup2->set_trackingnum(120393504);
  AProduct * aProduct3 = pickup2->add_things();
  aProduct3->set_id(12);
  aProduct3->set_description("cc");
  aProduct3->set_count(3);
  AProduct * aProduct4 = pickup2->add_things();
  aProduct4->set_id(13);
  aProduct4->set_description("dd");
  aProduct4->set_count(4);
  Warehouse * warehouse2 = new Warehouse();
  ;
  warehouse2->set_id(99);
  warehouse2->set_x(99);
  warehouse2->set_y(99);
  pickup2->set_allocated_wareinfo(warehouse);
  AULoadOver * loaded = aResq.add_packloaded();
  loaded->set_seqnum(120393504);
  loaded->set_truckid(1);
  UDeliveryLocation * loc = new UDeliveryLocation();
  loc->set_packageid(120393504);
  loc->set_x(101);
  loc->set_y(101);
  loaded->set_allocated_loc(loc);
  loaded->set_seqnum(1005);
  AUQueryUpsid * queryUpsid = new AUQueryUpsid();
  queryUpsid->set_upsid(19);
  queryUpsid->set_seqnum(1006);
  aResq.set_allocated_queryupsid(queryUpsid);
}

//process query upsid request from Amazon
void BaseServer::processAmazonUpsQuery(connection * C, AUCommand & aResq) {
  if (!aResq.has_queryupsid()) {
    return;
  }
  cout << "qeury for upsid" << endl;
  const AUQueryUpsid & queryUpsid = aResq.queryupsid();
  if (amazonReq.find(queryUpsid.seqnum()) != amazonReq.end()) {
    return;
  }
  amazonReq.insert(queryUpsid.seqnum());
  int64_t upsId = queryUpsid.upsid();
  if (!db.queryUpsid(C, upsId)) {
    sendErrToAmazon("cannot find this upsId", queryUpsid.seqnum(), seqnum.fetch_add(1));
  }
  else {
    ackToAmazon(queryUpsid.seqnum());
  }
}

// process pickup request from Amazon
void BaseServer::processAmazonPickup(connection * C, AUCommand & aResq) {
  cout << "During processing amazon pickup:\n";
  cout << "pickup size=" << aResq.pickup_size() << endl;
  cout << aResq.ShortDebugString() << endl;
  for (int i = 0; i < aResq.pickup_size(); i++) {
    const AURequestPickup & pickup = aResq.pickup(i);
    ackToAmazon(pickup.seqnum());
    if (amazonReq.find(pickup.seqnum()) != amazonReq.end()) {
      continue;
    }
    amazonReq.insert(pickup.seqnum());
    const int warehouseId = pickup.wareinfo().id();

    cout << "New warehouse info...\n";
    WarehouseInfo * newWarehouseInfo =
        new WarehouseInfo(warehouseId, pickup.wareinfo().x(), pickup.wareinfo().y());
    db.insertTables(C, newWarehouseInfo);
    delete newWarehouseInfo;

    cout << "New package info...\n";
    Package * newPackage =
        new Package(pickup.trackingnum(), warehouseId, wait_for_pickup, pickup.upsid());
    db.insertTables(C, newPackage);
    delete newPackage;

    for (int j = 0; j < pickup.things_size(); j++) {
      const AProduct & newProduct = pickup.things(j);
      cout << "New item info...\n";
      Item * newItem =
          new Item(newProduct.description(), newProduct.count(), pickup.trackingnum());
      db.insertTables(C, newItem);
      delete newItem;
    }
    findTruckMutex.lock();

    cout << "After updating table in database:\n";
    auto search = warehousePkgMap.find(warehouseId);
    if (search != warehousePkgMap.end()) {
      search->second.push_back(pickup.trackingnum());
    }
    else {
      list<int64_t> packagesToPickUp;
      packagesToPickUp.push_back(pickup.trackingnum());
      auto it=warehousePkgMap.end();
      warehousePkgMap.emplace_hint(it,warehouseId, packagesToPickUp);
    }
    findTruckMutex.unlock();
  }
  //findTruck() will handle the truck finding and notify to amazon
}

void BaseServer::processAmazonLoaded(connection * C, AUCommand & aResq) {
  cout << "process loaded size=" << aResq.packloaded_size() << endl;
  
  for (int i = 0; i < aResq.packloaded_size(); i++) {
    const AULoadOver & loadOver = aResq.packloaded(i);
    ackToAmazon(loadOver.seqnum());
    if (amazonReq.find(loadOver.seqnum()) != amazonReq.end()) {
      continue;
    }
    amazonReq.insert(loadOver.seqnum());
    
    cout << "Updating package and truck table...\n";
    db.updatePackage(C,
                     out_for_deliver,
                     loadOver.loc().x(),
                     loadOver.loc().y(),
                     loadOver.loc().packageid());
    db.updateTruck(
        C, arrive_warehouse, loadOver.loc().x(), loadOver.loc().y(), loadOver.truckid());
    
    cout << "Adding elements to truck package map...\n";
    unordered_map<int, vector<UDeliveryLocation> >::iterator it =
        truckPackageMap.find(loadOver.truckid());
    if (it != truckPackageMap.end()) {
      it->second.push_back(loadOver.loc());
    }
    else {
      vector<UDeliveryLocation> loadedPackages;
      loadedPackages.push_back(loadOver.loc());
      auto it=truckPackageMap.end();
      truckPackageMap.emplace_hint(it,loadOver.truckid(), loadedPackages);
    }
  }

  cout << "Fetching sequence numbers...\n";
  vector<int64_t> seqNums;
  vector<int> needToErase;
  for (auto const p : truckPackageMap) {
    if (db.queryTruckAvailable(C, p.first)){
      int64_t seqNum = seqnum.fetch_add(1);
      cout << "Requesting delivery to world...\n";
      requestDeliverToWorld(p.first, p.second, seqNum);
      cout << "Remove element from truckPackageMap...\n";
      needToErase.push_back(p.first);
      seqNums.push_back(seqNum);
    }
  }

  // erase from truckPackageMap
  for (auto seq : needToErase) {
    truckPackageMap.erase(seq);
  }

  cout << "Waiting for world processing...\n";
  for (auto const seqNum : seqNums) {
    waitWorldProcess(seqNum);
  }
  cout << "World processing done...\n";
}

// getter functions
int BaseServer::getBacklog() const {
  return backlog;
}
const MySocket * BaseServer::getSocket() const {
  return sock;
}
int64_t BaseServer::getWorldId() const {
  return worldId;
}

// setter functions
void BaseServer::setBacklog(int b) {
  backlog = b;
}
void BaseServer::setSocket(MySocket * s) {
  sock = s;
}
void BaseServer::setWorldId(int64_t id) {
  worldId = id;
}
