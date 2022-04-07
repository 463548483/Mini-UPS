#include <iostream>
#include <fstream>
#include <string>
#include "world_ups.pb.h"
#include "../Server/BaseSocket.hpp"
#include "../Utils/utils.hpp"

using namespace std;

int main(int argc, char const *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    UCommands ucom;
    ucom.add_acks(1);
    ucom.add_acks(2);

    UGoPickup *pick = ucom.add_pickups();
    UGoDeliver *deli = ucom.add_deliveries();
    UQuery *quer = ucom.add_queries();

    pick->set_truckid(23);
    pick->set_whid(12);
    pick->set_seqnum(23633);

    deli->set_truckid(10);
    deli->set_seqnum(2368);
    UDeliveryLocation *pack = deli->add_packages();
    pack->set_packageid(127);
    pack->set_x(0);
    pack->set_y(0);

    quer->set_truckid(23);
    quer->set_seqnum(273409);

    struct addrinfo host_info;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    try {
        MySocket sock(host_info, "vcm-25032.vm.duke.edu", "5555");
        sock.connectSocket();

        google::protobuf::io::FileOutputStream *out = new google::protobuf::io::FileOutputStream(sock.getSocketFd());
        cout << "===============sending message==============\n";
        sendMesgTo<UCommands>(ucom, out);
        cout << "===============sending message done==============\n";

        delete out;
        google::protobuf::ShutdownProtobufLibrary();
    }
    catch(const std::exception& e) {
        exitWithError(e.what());
    }

    return 0;
}
