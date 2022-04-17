#ifndef PACKAGE_H
#define PACKAGE_H
#include <string>

#include "SQLObject.hpp"

static const char * package_enum_str[] = {"delivered",
                                          "delivering",
                                          "wait for loading",
                                          "wait for pickup"};
enum package_status_t { delivered, out_for_deliver, wait_for_loading, wait_for_pickup };

class Package : virtual public SQLObject {
 private:
  int trackingNum;
  int warehouseId;
  package_status_t status;
  int destX;
  int destY;
  int truckId;
  int accountId;

 public:
  Package(int trackingNum,
          int warehouseId,
          package_status_t status,
          int accountId = -1,
          int truckId = -1,
          int destX = INT32_MAX,
          int destY = INT32_MAX
          ) :
      SQLObject("PACKAGES"),
      trackingNum(trackingNum),
      warehouseId(warehouseId),
      status(status),
      destX(destX),
      destY(destY),
      truckId(truckId),
      accountId(accountId) {}

  int getTrakingNum() { return trackingNum; }
  int getDestX() { return destX; }
  int getDestY() { return destY; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName
       << " (destX,destY,truckId,accountId,warehouseId,status,trackingNum) values (";
    if (destX == INT32_MAX && destY == INT32_MAX) {
      ss << "NULL,NULL,";
    }
    else {
      ss << destX << "," << destY << ",";
    }
    if (truckId == -1) {
      ss << "NULL,";
    }
    else {
      ss << truckId << ",";
    }
    if (accountId == -1) {
      ss << "NULL,";
    }
    else {
      ss << accountId << ",";
    }
    ss << warehouseId << ","
       << "'" << package_enum_str[status] << "'," << trackingNum << ");";
    return ss.str();
  }
};

#endif