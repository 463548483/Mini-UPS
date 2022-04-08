#include <string>

class Package {
 private:
  Package() {}
  int packageId;
  int truckId;
  int userId;
  int destX;
  int destY;

 public:
  Package(int id, int truckid, int userid=NULL, int destX, int destY) :
      PackageId(PackageId), truckId(truckid), userId(userid), destX(destX), destY(destY)) {}
  int PackageId() { return PackageId; }
  int destX() { return destX; }
  int destY() { return destY; }
};