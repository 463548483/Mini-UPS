#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <ctime>
#include <pqxx/pqxx>
#include <string>

#include "Truck.hpp"
#include "Account.hpp"
#include "Item.hpp"
#include "Package.hpp"

#define HOST "vcm-26069.vm.duke.edu"
#define DATABASE "postgres"
#define USER "postgres"
#define PASSWORD "@zxcvbnm123"

class SQLObject;
class Truck;
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

  // pqxx::connection *getConnection();
};

#endif