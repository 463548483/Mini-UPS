#ifndef WAREHOUSEINFO_H
#define WAREHOUSEINFO_H
#include <string>

#include "SQLObject.hpp"

class WarehouseInfo : virtual public SQLObject {
 private:
  int warehouseId;
  int x;
  int y;

 public:
  WarehouseInfo(int warehouseId, int x, int y) :
      SQLObject("WAREHOUSES"),warehouseId(warehouseId), x(x), y(y) {}
  int getWarehouseId() { return warehouseId; }
  int getX() { return x; }
  int getY() { return y; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (warehouseId,x,y) values (" << warehouseId
       << "," << x << "," << y << ") on conflict(warehouseid) do update set x=excluded.x, y=excluded.y;";
    return ss.str();
  }
};

#endif