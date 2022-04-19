#ifndef BASESERVER_H
#define BASESERVER_H

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <unistd.h>

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "../DataBase/Database.hpp"
#include "../protos/ups_amazon.pb.h"
#include "../protos/world_amazon.pb.h"
#include "../protos/world_ups.pb.h"
#include "BaseSocket.hpp"

class ConnectToAmazonFailure : public std::exception {
 public:
  virtual const char * what() const noexcept {
    return "After sending world id to amazon, didn't receive correct response.";
  }
};

class BaseServer {
 public:
  // listening socket, for accepting connection
  MySocket * sock;
  // communicating with the world
  MySocket * worldSock;
  google::protobuf::io::FileInputStream * worldIn;
  google::protobuf::io::FileOutputStream * worldOut;
  // communicating with Amazon
  MySocket * amazonSock;
  google::protobuf::io::FileInputStream * amazonIn;
  google::protobuf::io::FileOutputStream * amazonOut;
  int backlog;
  int threadNum;
  tbb::task_scheduler_init init;
  tbb::task_group group;
  Database db;
  int64_t worldId;

 public:
  BaseServer(const char * _hostname, const char * _port, int _backlog, int _threadNum);
  ~BaseServer();
  void connectToSimWorld();
  void connectToAmazon();
  void receiveUConnected();
  int64_t getWorldIdFromSim();
  void sendTestCommand();
  void getTestResponse(UResponses & resp);
  std::vector<int64_t> extractSeqNums(UResponses & resp);
  void displayUResponses(UResponses & resp);
  void processErrors(UResponses & resp);
  void processAcks(UResponses & resp);
  void processCompletions(pqxx::connection * C, UResponses & resp);
  void processDelivered(pqxx::connection * C, UResponses & resp);
  void processTruckStatus(pqxx::connection * C, UResponses & resp);
  bool getCorrespondTruckStatus(const std::string & status, truck_status_t & newStatus);

  // interfaces of request to world
  void requestPickUpToWorld(int truckid, int whid, int64_t seqnum);
  void requestDeliverToWorld(int truckid,
                             const vector<UDeliveryLocation> & packages,
                             int64_t seqnum);
  void requestQueryToWorld(int truckids, int64_t seqnums);
  void adjustSimSpeed(unsigned int simspeed);
  void requestDisconnectToWorld();
  void addToWaitAcks(int64_t seqnum, UCommands ucom);
  void ackToWorld(std::vector<int64_t> acks);
  bool checkIfAlreadyProcessed(int64_t seq);

  // interfaces to amazon
  void sendWorldIdToAmazon(int64_t worldid, int64_t seqnum);
  void notifyArrivalToAmazon(int truckid, int64_t trackingnum, int64_t seqnum);
  void notifyDeliveredToAmazon(int64_t trackingnum, int64_t seqnum);
  void sendErrToAmazon(std::string err, int64_t originseqnum, int64_t seqnum);
  void ackToAmazon(int64_t ack);

  void setupServer(const char * _hostname, const char * _port);

  void frontendCommunicate();
  void simWorldCommunicate();
  void amazonCommunicate();
  void timeoutAndRetransmission();
  void launch();
  void daemonize();

  //interface for process amazon request
  void processAmazonPickup(pqxx::connection * C, AUCommand & aResq);
  void processAmazonLoaded(pqxx::connection * C, AUCommand & aResq);
  void processAmazonUpsQuery(pqxx::connection * C, AUCommand & aResq);
  bool waitWorldProcess(int64_t seqNum);
  int findTrucks();
  void getTestAUCommand(AUCommand & acommd);

  // getter
  int getBacklog() const;
  const MySocket * getSocket() const;
  int64_t getWorldId() const;

  // setter
  void setBacklog(int);
  void setSocket(MySocket *);
  void setWorldId(int64_t);
};

#endif /* BASESERVER_H */
