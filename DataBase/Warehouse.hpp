#ifndef WAREHOUSE_H
#define WAREHOUSE_H
#include <string>

#include "SQLObject.hpp"

class Warehouse : virtual public SQLObject {
 private:
  int warehouseId;
  int x;
  int y;

 public:
  Warehouse(int warehouseId, int x, int y) :
      SQLObject("WAREHOUSES"),warehouseId(warehouseId), x(x), y(y) {}
  int getWarehouseId() { return warehouseId; }
  int getX() { return x; }
  int getY() { return y; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (warehouseId,x,y) values (" << warehouseId
       << "," << x << "," << y << ");";
    return ss.str();
  }
};

#endif