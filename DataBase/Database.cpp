#ifndef DATABASE_H
#define DATABASE_H
#include "Database.hpp"

#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>


#define TRANS(name) #name

using namespace std;
using namespace pqxx;

atomic<int> ORDER_ID(0);

Database::Database() {
  setup();
}

void Database::setup() {
  connection * C = connectToDatabase();

  cout << "testing\n";
  // clean all existing tables or types
  cleanTables(C);

  // create tables
  createTables(C);
  
  // test insert
  SQLObject * truck1=new Truck(3,idle,0,0);
  insertTables(C,truck1);
  SQLObject * account1=new Account(123,"eaeeer");
  insertTables(C,account1);
  SQLObject * pkg1=new Package(3,-1,-1);
  insertTables(C,pkg1);
  SQLObject * item1=new Item(9,"cloth",3);
  insertTables(C,item1);


  C->disconnect();
  delete C;
}

Database::~Database() {
}

connection * Database::connectToDatabase() {
  //Establish a connection to the database
  //Parameters: database name, user name, user password
  connection * C;
  stringstream ss;
  ss << "host=" << HOST << " dbname=" << DATABASE << " user=" << USER
     << " password=" << PASSWORD;
  C = new connection(ss.str());

  if (C->is_open()) {
    cout << "Connect to database successfully: " << C->dbname() << endl;
  }
  else {
    throw DatabaseConnectionError();
  }

  return C;
}

void Database::dropATable(connection * C, string tableName) {
  /* Create a transactional object. */
  work W(*C);

  string sql = "DROP TABLE ";
  sql += tableName;
  sql += ";";

  /* Execute SQL query */
  try {
    W.exec(sql);
    W.commit();
    cout << "Table " << tableName << " dropped successfully" << endl;
  }
  catch (const undefined_table & e) {
    cout << "Table " << tableName << " doesn't exist. Ignoring the drop.\n";
  }
}

// drop all the existing tables
void Database::cleanTables(connection * C) {
  dropATable(C, "ITEMS");
  dropATable(C, "PACKAGES");
  dropATable(C, "ACCOUNT");
  dropATable(C, "TRUCKS");
  //dropATable(C, "account");

  // clean the enum type
  /* Create a transactional object. */
  work W(*C);

  string sql = "DROP TYPE truck_status;";

  /* Execute SQL query */
  try {
    W.exec(sql);
    W.commit();
    cout << "Enum type truck_status dropped successfully" << endl;
  }
  catch (const exception & e) {
    cout << "Enum type truck_status doesn't exist. Ignoring the drop.\n";
  }
}

void Database::createTables(connection * C) {
  /* Create a transactional object. */
  work W(*C);

  /* Create SQL statement */
  string createEnum = "CREATE TYPE truck_status AS ENUM ('idle', 'travelling', 'arrive warehouse', 'delivering');";
  string createTruck = "CREATE TABLE TRUCKS (\
    truckId       int           NOT NULL,\
    status        truck_status  NOT NULL,\
    x             int           NOT NULL,\
    y             int           NOT NULL,\
    CONSTRAINT TRUCKID_PK PRIMARY KEY (truckId)\
    )";

  string createAccount = "CREATE TABLE ACCOUNT (\
    accountId     int   NOT NULL,\
    password      varchar(40)         NOT NULL,\
    CONSTRAINT ACCOUNTID_PK PRIMARY KEY (accountId)\
    )";

  string createPackage = "CREATE TABLE PACKAGES (\
    packageId      int         NOT NULL,\
    destX          int         NOT NULL,\
    destY          int         NOT NULL,\
    truckId        int                 ,\
    accountId      int                 ,\
    CONSTRAINT PACKAGEID_PK PRIMARY KEY (packageId),\
    CONSTRAINT PACKAGE_TRUCKFK FOREIGN KEY (truckId) REFERENCES TRUCKS(truckId) ON DELETE SET NULL ON UPDATE CASCADE,\
    CONSTRAINT PACKAGE_ACCOUNTFK FOREIGN KEY (accountId) REFERENCES ACCOUNT(accountId) ON DELETE SET NULL ON UPDATE CASCADE\
    )";

  string createItem = "CREATE TABLE ITEMS (\
    itemId     int   NOT NULL,\
    description varchar(2000)   NOT NULL,\
    amount         int        NOT NULL,\
    packageid         int        ,\
    CONSTRAINT ITEMID_PK PRIMARY KEY (itemId),\
    CONSTRAINT ITEM_PACKAGEFK FOREIGN KEY (packageid) REFERENCES PACKAGES(packageId) ON DELETE SET NULL ON UPDATE CASCADE\
    )";

  /* Execute SQL query */
  W.exec(createEnum);
  W.exec(createTruck);
  W.exec(createAccount);
  W.exec(createPackage);
  W.exec(createItem);
  //W.exec(createSearchHisotry);
  W.commit();
  cout << "All tables created successfully" << endl;
}

void Database::insertTables(connection * C, SQLObject * object){
    work W(*C);
    W.exec(object->sql_insert());
    W.commit();
    cout << "One row inserted successfully" << endl;
}

#endif