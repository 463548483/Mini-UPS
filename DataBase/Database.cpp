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

	// clean all existing tables or types
    // cleanTables(C);

    // create tables
    // createTables(C);

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
    string createEnum = "CREATE TYPE order_status AS ENUM ('open', 'executed', 'canceled');";
    string createAccount = "CREATE TABLE ACCOUNT (\
    account_id     varchar(40)   NOT NULL,\
    balance        float         NOT NULL,\
    CONSTRAINT ACCOUNTID_PK PRIMARY KEY (ACCOUNT_ID)\
    )";
    string createOrders = "CREATE TABLE ORDERS (\
    id            serial        NOT NULL,\
    order_id      int           NOT NULL,\
    status        order_status  NOT NULL,\
    symbol        varchar(40)   NOT NULL,\
    amount        float         NOT NULL,\
    price         float         NOT NULL,\
    time          bigint        NOT NULL,\
    account_id    varchar(40)   NOT NULL,\
    CONSTRAINT ID_PK PRIMARY KEY (ID),\
    CONSTRAINT ACCOUNTIDFK FOREIGN KEY (ACCOUNT_ID) REFERENCES ACCOUNT(ACCOUNT_ID) ON DELETE SET NULL ON UPDATE CASCADE\
    )";
    string createPosition = "CREATE TABLE POSITION (\
    account_id     varchar(40)   NOT NULL,\
    symbol         varchar(40)   NOT NULL,\
    amount         float         NOT NULL,\
    CONSTRAINT ACCOUNTIDSYM_PK PRIMARY KEY (ACCOUNT_ID, SYMBOL)\
    )";

    /* Execute SQL query */
    W.exec(createEnum);
    W.exec(createAccount);
    W.exec(createOrders);
    W.exec(createPosition);
    W.commit();
    cout << "All tables created successfully" << endl;
}
