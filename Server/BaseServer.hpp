#ifndef BASESERVER_H
#define BASESERVER_H

#include "BaseSocket.hpp"
#include "../DataBase/Database.hpp"
#include "../protos/world_ups.pb.h"
#include "../protos/ups_amazon.pb.h"
#include <pqxx/pqxx>
#include <exception>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <memory>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

class FormatError : public std::exception {
public:    
    virtual const char* what() const noexcept {
       return "The data received is of wrong format.";
    }
};

class FormatErrorLength : public std::exception {
public:    
    virtual const char* what() const noexcept {
       return "The number in the first line does not match the length of the xml received.";
    }
};

class BaseServer {
private:
    // listening socket, for accepting connection
    MySocket *sock;
    // communicating with the world
    MySocket *worldSock;
    google::protobuf::io::FileInputStream *worldIn;
    google::protobuf::io::FileOutputStream *worldOut;
    // communicating with Amazon
    MySocket *amazonSock;
    google::protobuf::io::FileInputStream *amazonIn;
    google::protobuf::io::FileOutputStream *amazonOut;
    int backlog;
    int threadNum;
    tbb::task_scheduler_init init;
	tbb::task_group group;
    Database db;
    int64_t worldId;
public:
    BaseServer(const char *_hostname, 
        const char *_port, int _backlog, int _threadNum);
    ~BaseServer();
    int64_t getWorldIdFromSim();
    void sendTestCommand();
    std::vector<int64_t> extractSeqNums(UResponses &resp);
    void displayUResponses(UResponses &resp);
    void processErrors(UResponses &resp);
    void processAcks(UResponses &resp);
    void processCompletions(UResponses &resp);
    void processDelivered(UResponses &resp);
    void processTruckStatus(UResponses &resp);

    // interfaces of request to world
    void requestPickUpToWorld(int truckid, int whid, int64_t seqnum);
    void requestDeliverToWorld(int truckid, std::vector<int64_t> packageids, 
        std::vector<int> xs, std::vector<int> ys, int64_t seqnum);
    void requestQueryToWorld(int truckids, int64_t seqnums);
    void adjustSimSpeed(unsigned int simspeed);
    void requestDisconnectToWorld();
    void addToWaitAcks(int64_t seqnum, UCommands ucom);
    void ackToWorld(int64_t ack);

    // interfaces to amazon
    void sendWorldIdToAmazon(int64_t worldid, int64_t seqnum);
    void notifyArrivalToAmazon(int truckid, int64_t packageid, int64_t seqnum);
    void notifyDeliveredToAmazon(int64_t packageid, int64_t seqnum);
    void sendErrToAmazon(std::string err, int64_t originseqnum, int64_t seqnum);
    void sendPickUpResponseToAmazon(int64_t packageid, int64_t seqnum);
    void ackToAmazon(int64_t ack);

    //interface for process amazon request
    void processAmazonPickup(AUCommand &aResq);
    void processAmazonLoaded(AUCommand &aResq);
    void processAmazonChangeAdd(AUCommand &aResq);

    void setupServer(const char *_hostname, const char *_port);
    
    void simWorldCommunicate();
    void amazonCommunicate();
    void timeoutAndRetransmission();
    void launch();
    void daemonize();
    
    // getter 
    int getBacklog() const;
    const MySocket *getSocket() const;
    int64_t getWorldId() const;

    // setter
    void setBacklog(int);
    void setSocket(MySocket *);
    void setWorldId(int64_t);
};


#endif /* BASESERVER_H */
