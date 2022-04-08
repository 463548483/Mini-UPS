#include <iostream>
#include <sstream>
#include <mutex>
#include <atomic>
#include "Database.hpp"

using namespace std;
using namespace pqxx;

atomic<int> ORDER_ID(0);

Database::Database() {
    setup();
}

void Database::setup() {
	connection *C = connectToDatabase();

    cout << "testing\n";
	// clean all existing tables or types
    cleanTables(C);

    // create tables
    createTables(C);

    C->disconnect();
    delete C;
}

Database::~Database() {

}

connection *Database::connectToDatabase() {
    //Establish a connection to the database
	//Parameters: database name, user name, user password
	connection *C;
    stringstream ss;
    ss << "host=" << HOST << " dbname=" << DATABASE << " user=" << USER << " password=" << PASSWORD;
    C = new connection(ss.str());
    
	if (C->is_open()) {
		cout << "Connect to database successfully: " << C->dbname() << endl;
	} else {
		throw DatabaseConnectionError();
	}

    return C;
}

void Database::dropATable(connection *C, string tableName) {
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
    catch (const undefined_table &e) {
        cout << "Table " << tableName << " doesn't exist. Ignoring the drop.\n";
    }
}

// drop all the existing tables
void Database::cleanTables(connection *C) {
    dropATable(C, "orders");
    dropATable(C, "position");
    dropATable(C, "account");

    // clean the enum type
    /* Create a transactional object. */
    work W(*C);

    string sql = "DROP TYPE order_status;";

    /* Execute SQL query */
    try {
        W.exec(sql);
        W.commit();
        cout << "Enum type order_status dropped successfully" << endl;
    }
    catch (const exception &e) {
        cout << "Enum type order_status doesn't exist. Ignoring the drop.\n";
    }
}

void Database::createTables(connection *C) {
    /* Create a transactional object. */
    work W(*C);

    /* Create SQL statement */
    string createEnum = "CREATE TYPE truck_status AS ENUM ('idle', 'travelling', 'arrive warehouse', 'delivering');";
    string createTruck = "CREATE TABLE TRUCKS (\
    truckId       int           NOT NULL,\
    truck_status  status        NOT NULL,\
    symbol        varchar(40)   NOT NULL,\
    x             int           NOT NULL,\
    y             int           NOT NULL,\
    CONSTRAINT TRUCKID_PK PRIMARY KEY (truckId)\
    )";
    string createPackage = "CREATE TABLE PACKAGES (\
    packageId      int         NOT NULL,\
    truckId        int         NOT NULL,\
    userId         int                 ,\
    destX          int         NOT NULL,\
    destY          int         NOT NULL,\
    CONSTRAINT PACKAGEID_PK PRIMARY KEY (packageId),\
    CONSTRAINT PACKAGEIDFK FOREIGN KEY (truckId) REFERENCES TRUCKS(truckId) ON DELETE SET NULL ON UPDATE CASCADE\
    CONSTRAINT PACKAGEIDFK FOREIGN KEY (truckId) REFERENCES TRUCKS(truckId) ON DELETE SET NULL ON UPDATE CASCADE\
    )";
    string createUser = "CREATE TABLE USER (\
    userId     int   NOT NULL,\
    password        varchar(40)         NOT NULL,\
    CONSTRAINT USERID_PK PRIMARY KEY (userId)\
    )";

    string createItem = "CREATE TABLE ITEMS (\
    itemId     int   NOT NULL,\
    description         varchar(2000)   NOT NULL,\
    amount         int        NOT NULL,\
    packageid         int        NOT NULL,\
    CONSTRAINT ITEMID_PK PRIMARY KEY (itemId)\
    CONSTRAINT ITEMIDFK FOREIGN KEY (packageid) REFERENCES ACCOUNT(ACCOUNT_ID) ON DELETE SET NULL ON UPDATE CASCADE\
    )";

    /* Execute SQL query */
    W.exec(createEnum);
    W.exec(createAccount);
    W.exec(createOrders);
    W.exec(createPosition);
    W.commit();
    cout << "All tables created successfully" << endl;
}
