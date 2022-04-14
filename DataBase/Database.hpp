#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <ctime>
#include <pqxx/pqxx>
#include <string>

#include "Account.hpp"
#include "Item.hpp"
#include "Package.hpp"
#include "Truck.hpp"
#include "Warehouse.hpp"

#define HOST "localhost"
#define DATABASE "postgres"
#define USER "postgres"
#define PASSWORD "postgres"

class DatabaseConnectionError : public std::exception {
 public:
  virtual const char * what() const noexcept { return "Cannot connect to the database."; }
};

class Database {
 public:
  Database();
  ~Database();

  pqxx::connection * connectToDatabase();
  void setup();
  void dropATable(pqxx::connection *, std::string tableName);
  void cleanTables(pqxx::connection *);
  void createTables(pqxx::connection *);
  void insertTables(pqxx::connection * C, SQLObject * object);
  void updateTruck(pqxx::connection * C,
                   truck_status_t status,
                   int x,
                   int y,
                   int truckId);
  void updatePackage(pqxx::connection * C,
                     package_status_t status,
                     int x,
                     int y,
                     int trackingNum);
  void updatePackage(pqxx::connection * C,
                     package_status_t status,
                     int truckid,
                     int trackingNum);
  void updatePackage(pqxx::connection * C, package_status_t status, int trackingNum);
  string queryPackageStatus(pqxx::connection * C, int packageId);
  // pqxx::connection *getConnection();
};

#endif