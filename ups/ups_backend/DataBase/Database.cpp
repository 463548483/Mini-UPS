#ifndef DATABASE_H
#define DATABASE_H
#include "Database.hpp"

#include <atomic>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>

#define TRANS(name) #name

using namespace std;
using namespace pqxx;

Database::Database() {
  setup();
}

void Database::setup() {
  connection * C = connectToDatabase();

  // cout << "testing\n";
  // clean all existing tables or types
  cleanTables(C);

  // create tables
  createTables(C);

  // test insert
  // SQLObject *warehouse1 = new WarehouseInfo(1, 23, 23);
  // insertTables(C, warehouse1);
  // SQLObject *warehouse2 = new WarehouseInfo(2, -23, -23);
  // insertTables(C, warehouse2);
  
  // SQLObject *
  // SQLObject *account1 = new Account("")
  // SQLObject * truck1 = new Truck(idle, 0, 0);
  // insertTables(C, truck1);
  // SQLObject * account1 = new Account("eaeeer");
  // insertTables(C, account1);

  // SQLObject * warehouse1 = new WarehouseInfo(1, 3, 3);
  // insertTables(C, warehouse1);
  // SQLObject * pkg1 = new Package(1231, 1, delivered,123345);
  // insertTables(C, pkg1);
  // SQLObject * item1 = new Item("cloth", 3, 1231);
  // insertTables(C, item1);

  // updateTruck(C, delivering, 9, 9, 1);
  // updatePackage(C, out_for_deliver, 10, 10, 1231);
  // updatePackage(C, out_for_deliver, 1, 1231);
  // updatePackage(C, wait_for_loading, 1, 1231);
  // string st = queryPackageStatus(C, 1231);
  // cout << st << " status for pkg" << endl;
  // int truckIds = queryAvailableTrucksPerDistance(C, 1);
  // cout << "find truck " << truckIds << endl;

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
  dropATable(C, "SEARCHHISTORY");
  dropATable(C, "PACKAGES");
  dropATable(C, "WAREHOUSES");
  dropATable(C, "ACCOUNT");
  dropATable(C, "TRUCKS");
  

  // clean the enum type
  /* Create a transactional object. */
  work W(*C);

  string sql1 = "DROP TYPE if exists truck_status;";
  string sql2 = "DROP TYPE if exists package_status;";

  /* Execute SQL query */
  try {
    W.exec(sql1);
    W.exec(sql2);
    W.commit();
    cout << "Enum type truck_status dropped successfully" << endl;
  }
  catch (const exception & e) {
    cout << e.what() << endl;
  }
}

void Database::createTables(connection * C) {
  /* Create a transactional object. */
  work W(*C);

  /* Create SQL statement */
  string createEnumTruck = "CREATE TYPE truck_status AS ENUM ('idle', 'traveling', "
                           "'arrive warehouse', 'loading', 'delivering');";
  string createEnumPackage = "CREATE TYPE package_status AS ENUM ('delivered', "
                             "'delivering', 'wait for loading', 'wait for pickup');";
  string createTruck = "CREATE TABLE TRUCKS (\
    truckId       SERIAL        PRIMARY KEY,\
    status        truck_status  NOT NULL,\
    x             int           NOT NULL,\
    y             int           NOT NULL\
    );";

  string createAccount = "CREATE TABLE ACCOUNT (\
    accountId     BIGSERIAL              NOT NULL,\
    username      varchar(40)         NOT NULL,\
    CONSTRAINT ACCOUNTID_PK PRIMARY KEY (accountId)\
    );";

  string createWarehouse = "CREATE TABLE WAREHOUSES (\
    warehouseId     int         NOT NULL,\
    x               int         NOT NULL,\
    y               int         NOT NULL,\
    CONSTRAINT WAREHOUSEID_PK PRIMARY KEY (warehouseId)\
    );";

  string createPackage = "CREATE TABLE PACKAGES (\
    trackingNum    bigint             PRIMARY KEY,\
    destX          int             ,\
    destY          int             ,\
    truckId        int             ,\
    accountId      bigint             ,\
    warehouseId    int             NOT NULL,\
    status         package_status  NOT NULL,\
    CONSTRAINT PACKAGE_TRUCKFK FOREIGN KEY (truckId) REFERENCES TRUCKS(truckId) ON DELETE SET NULL ON UPDATE CASCADE,\
    CONSTRAINT PACKAGE_WAREHOUSEFK FOREIGN KEY (warehouseId) REFERENCES WAREHOUSES(warehouseId) ON DELETE SET NULL ON UPDATE CASCADE,\
    CONSTRAINT PACKAGE_ACCOUNTFK FOREIGN KEY (accountId) REFERENCES ACCOUNT(accountId) ON DELETE SET NULL ON UPDATE CASCADE\
    );";

  string createItem = "CREATE TABLE ITEMS (\
    itemId         BIGSERIAL          PRIMARY KEY,\
    description    varchar(2000)   NOT NULL,\
    amount         int             NOT NULL,\
    trackingNum    bigint             NOT NULL,\
    CONSTRAINT ITEM_PACKAGEFK FOREIGN KEY (trackingNum) REFERENCES PACKAGES(trackingNum) ON DELETE SET NULL ON UPDATE CASCADE\
    );";

  string createSearchHistory = "CREATE TABLE SEARCHHISTORY (\
    id             SERIAL   PRIMARY KEY,\
    accountId      bigint   NOT NULL,\
    trackingNum    bigint   NOT NULL,\
    CONSTRAINT HISTORY_PACKAGEFK FOREIGN KEY (trackingNum) REFERENCES PACKAGES(trackingNum) ON DELETE SET NULL ON UPDATE CASCADE,\
    CONSTRAINT HISTORY_ACCOUNT FOREIGN KEY (accountid) REFERENCES ACCOUNT(accountid) ON DELETE SET NULL ON UPDATE CASCADE\
    );";

  /* Execute SQL query */
  try {
    W.exec(createEnumTruck);
    W.exec(createEnumPackage);
    W.exec(createTruck);
    W.exec(createAccount);
    W.exec(createWarehouse);
    W.exec(createPackage);
    W.exec(createItem);
    

    W.exec(createSearchHistory);
    W.commit();
    cout << "All tables created successfully" << endl;
  }
  catch (const exception & e) {
    cout << e.what() << endl;
  }
}

void Database::insertTables(connection * C, SQLObject * object) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      T.exec(object->sql_insert());
      T.commit();
      cout << object->getTableName() << " one row inserted successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
    catch (const pqxx::foreign_key_violation & e){
      if (object->getTableName()=="PACKAGES"){
        Package * package=dynamic_cast<Package*>(object);
        (*package).accountId=0;
        cout<<package->sql_insert()<<endl;
        while (true){
          transaction<serializable, read_write> T(*C);
          try {
            T.exec(package->sql_insert());
            T.commit();
            cout << object->getTableName() << " one row inserted successfully" << endl;
            break;
          }
          catch (const pqxx::serialization_failure & e) {
            cout << e.what() << endl;
          }          
        }

      }
      return;
    }
  }
}

void Database::updateTruck(connection * C,
                           truck_status_t status,
                           int x,
                           int y,
                           int truckId) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "update trucks set status='" << truck_enum_str[status] << "', x=" << x
         << ", y=" << y << " where truckid=" << truckId << ";";
      T.exec(ss.str());
      T.commit();
      cout << "trucks one row update successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}

void Database::updateTruck(connection * C, truck_status_t status, int truckId) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "update trucks set status='" << truck_enum_str[status]
         << "' where truckid=" << truckId << ";";
      T.exec(ss.str());
      T.commit();
      cout << "trucks one row update successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}

void Database::updatePackage(connection * C,
                             int destX,
                             int destY,
                             int64_t trackingNum) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "update packages set destX=" << destX << ", destY=" << destY
         << " where trackingNum=" << trackingNum << ";";
      T.exec(ss.str());
      T.commit();
      cout << "packages one row update successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}


void Database::updatePackage(connection * C,
                             package_status_t status,
                             int destX,
                             int destY,
                             int64_t trackingNum) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "update packages set status='" << package_enum_str[status]
         << "', destX=" << destX << ", destY=" << destY
         << " where trackingNum=" << trackingNum << ";";
      T.exec(ss.str());
      T.commit();
      cout << "packages one row update successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}

void Database::updatePackage(connection * C,
                             package_status_t status,
                             int truckid,
                             int64_t trackingNum) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "update packages set status='" << package_enum_str[status]
         << "', truckid=" << truckid << " where trackingNum=" << trackingNum << ";";
      T.exec(ss.str());
      T.commit();
      cout << "packages one row update successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}

void Database::updatePackage(connection * C,
                             package_status_t status,
                             int64_t trackingNum) {
  
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "update packages set status='" << package_enum_str[status]
         << "' where trackingNum=" << trackingNum << ";";
      T.exec(ss.str());
      T.commit();
      cout << "packages one row update successfully" << endl;
      break;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}

string Database::queryPackageStatus(connection * C, int64_t trackingNum) {
  work W(*C);
  stringstream ss;
  ss << "select status from packages where trackingNum='" << trackingNum << "';";
  result r = W.exec(ss.str());
  return r.begin()[0].as<string>();
}

int Database::queryAvailableTrucksPerDistance(connection * C, int warehouseId) {
  while (true) {
    transaction<serializable, read_write> T(*C);
    try {
      stringstream ss;
      ss << "select truckid, (trucks.x-warehouses.x)^2+(trucks.y-warehouses.y)^2 as "
            "distance"
         << " from trucks, warehouses where warehouseId=" << warehouseId
         << " and trucks.status in ('idle','delivering','arrive warehouse')  order by "
            "distance;";
      result r = T.exec(ss.str());
      for (size_t i = 0; i < r.size(); i++) {
        int truckId = r[i][0].as<int>();
        cout<<"try truck"<<truckId<<endl;
        ss = stringstream();
        ss << "select * from packages where truckid=" << truckId
           << " and status in ('wait for loading', 'wait for pickup');";
        
        result check = T.exec(ss.str());
        cout<<"after query package under this truck"<<endl;
        if (check.size() == 0) {
          cout<<"updated the available truck"<<truckId<<endl;
          stringstream ss;
          ss << "update trucks set status='traveling' where truckid=" << truckId << ";";
          T.exec(ss.str());
          T.commit();
          cout<<"commited"<<endl;
          return truckId;
        }
      }
      return 0;
    }
    catch (const pqxx::serialization_failure & e) {
      cout << e.what() << endl;
    }
  }
}

bool Database::queryTruckAvailable(connection * C,int truckId) {
  work W(*C);
  stringstream ss;
  ss << "select * from trucks,packages where trucks.truckid="<<truckId<<" and packages.truckid="<<truckId<<" and packages.status in ('wait for pickup','wait for loading');";
  result r = W.exec(ss.str());
  if (r.size() > 0) {
    return false;
  }
  else {
    return true;
  }
}

bool Database::queryUpsid(connection * C, int64_t upsId) {
  work W(*C);
  stringstream ss;
  ss << "select * from account where accountId=" << upsId << ";";
  result r = W.exec(ss.str());
  if (r.size() > 0) {
    return true;
  }
  else {
    return false;
  }
}

vector<int64_t> Database::queryTrackingNumToPickUp(connection * C,
                                                   int truckid,
                                                   int warehouseid) {
  vector<int64_t> trackingNums;

  work W(*C);
  stringstream ss;
  ss << "select trackingnum from packages where truckid='" << truckid
     << "' and warehouseid='" << warehouseid << "' and status='wait for pickup';";
  result r = W.exec(ss.str());

  for (result::const_iterator it = r.begin(); it != r.end(); ++it) {
    trackingNums.push_back(it[0].as<int64_t>());
  }
  return trackingNums;
}

int Database::queryWarehouseId(connection * C, int x, int y) {
  work W(*C);
  stringstream ss;
  ss << "select warehouseid from warehouses where x=" << x << " and y=" << y << ";";
  result r = W.exec(ss.str());

  if (r.size() == 0) {
    throw WareHouseNotExist();
  }  
  
  return r.begin()[0].as<int>();
}

#endif
