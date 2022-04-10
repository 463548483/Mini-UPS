#ifndef TRUCK_H
#define TRUCK_H
#include <string>

#include "SQLObject.hpp"
using namespace std;
static const char * enum_str[] = {"idle", "travelling", "arrive warehouse", "delivering"};
enum status_t { idle, travelling, arrive_warehouse, delivering };

class Truck : virtual public SQLObject {
 private:
  int truckId;
  status_t status;
  int x;
  int y;

 public:
  Truck(status_t status, int x, int y) :
      SQLObject("TRUCKS"), status(status), x(x), y(y) {}
  int getTruckId() { return truckId; }
  int getX() { return x; }
  int getY() { return y; }
  status_t getStatus() { return status; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (status,x,y) values ('" << enum_str[status]
       << "'," << x << "," << y << ");";
    return ss.str();
  }
  ~Truck() {}
};
#endif