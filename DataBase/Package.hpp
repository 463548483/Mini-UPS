#ifndef PACKAGE_H
#define PACKAGE_H
#include <string>

#include "SQLObject.hpp"

class Package : virtual public SQLObject {
 private:
  int packageId;
  int destX;
  int destY;
  int truckId;
  int accountId;

 public:
  Package(int destX, int destY, int truckid = 0, int accountid = 0) :
      SQLObject("PACKAGES"),
      destX(destX),
      destY(destY),
      truckId(truckid),
      accountId(accountid) {}
  int getPackageId() { return packageId; }
  int getDestX() { return destX; }
  int getDestY() { return destY; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName
       << " (destX,destY,truckId,accountId) values ("
       << destX << "," << destY << ",";
    if (truckId==0){
      ss<<"NULL"<<",";
    }
    else{
      ss<<truckId<<",";
    }
    if (accountId==0){
      ss<<"NULL"<<");";
    }
    else{
      ss<<accountId<<");";
    }
    return ss.str();
  }
};

#endif