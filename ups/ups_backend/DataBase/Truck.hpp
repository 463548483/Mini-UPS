#ifndef TRUCK_H
#define TRUCK_H
#include <string>

#include "SQLObject.hpp"
using namespace std;
static const char * truck_enum_str[] = {"idle", "traveling", "arrive warehouse", "loading", "delivering"};
enum truck_status_t { idle, traveling, arrive_warehouse, loading, delivering };

class Truck : virtual public SQLObject {
 private:
  int truckId;
  truck_status_t status;
  int x;
  int y;

 public:
  Truck(truck_status_t status, int x, int y) :
      SQLObject("TRUCKS"), status(status), x(x), y(y) {}
  int getTruckId() { return truckId; }
  int getX() { return x; }
  int getY() { return y; }
  truck_status_t getStatus() { return status; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (status,x,y) values ('" << truck_enum_str[status]
       << "'," << x << "," << y << ");";
    return ss.str();
  }
  ~Truck() {}
};
#endif