#include <string>

class Truck {
 private:
  Truck() {}
  int truckId;
  int x;
  int y;
  enum status_t { idle, travelling, arrive_warehouse, delivering };
  status_t status;

 public:
  Truck(int truckid, int x, int y, status_t status) :
      truckId(truckid), x(x), y(y), status(status) {}
  int truckId() { return truckId; }
  int x() { return x; }
  int y() { return y; }
  status_t status() { return status; }
};