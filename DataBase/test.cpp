#include <iostream>
#include <pqxx/pqxx>
#include <thread>
#include <cstdlib>
using namespace std;
using namespace pqxx;

connection *connectToDatabase() {
    //Allocate & initialize a Postgres connection object
    connection *C;

    try{
        //Establish a connection to the database
        //Parameters: database name, user name, user password
		C = new connection("dbname=serialtest user=postgres password=passw0rd");
        if (C->is_open()) {
        	cout << "Opened database successfully: " << C->dbname() << endl;
        } else {
          	cout << "Can't open database" << endl;
          	exit(EXIT_FAILURE);
        }
    } catch (const std::exception &e){
        cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return C;
}

void myTransaction1() {
    connection *C = connectToDatabase();

    /* Create a transactional object. */
    bool retry = true;
    while (retry) {
        try {
            transaction<serializable, read_write> W(*C);

            W.exec("UPDATE tbl_Employee SET EmpName ='Jeremy' WHERE EmpID=4;");
            while (1) {
                cout << "Press any key to continue: ";
                char c;
                cin >> c;
                break;
            }
            W.commit();
            retry = false;
        }
        catch(const pqxx::serialization_failure &e) {
            cout << "Serialization failure!!! Retrying...\n";
        } 
    }

    cout << "Player value changed to Jeremy successfully with serialization transaction" << endl;
    C->disconnect();
}

void myTransaction2() {
    connection *C = connectToDatabase();

    /* Create a transactional object. */
    bool retry = true;
    while (retry) {
        try {
            transaction<serializable, read_write> W(*C);

            W.exec("UPDATE tbl_Employee SET EmpName ='Tommy' WHERE EmpID=4;");
            while (1) {
                cout << "Press any key to continue: ";
                char c;
                cin >> c;
                break;
            }
            W.commit();
            retry = false;
        }
        catch(const pqxx::serialization_failure &e) {
            cout << "Serialization failure!!! Retrying...\n";
        } 
    }

    cout << "Player value changed to Tommy successfully with serialization transaction" << endl;
    C->disconnect();
}

int main(int argc, char const *argv[]) {
    vector<thread> threadArr;
    thread th1(myTransaction1);
    thread th2(myTransaction2);
    threadArr.push_back(move(th1));
    threadArr.push_back(move(th2));

    // wait for all threads finish their job
	for (thread &th : threadArr) {
        // If thread Object is Joinable then Join that thread.
        if (th.joinable())
            th.join();
    }

    return 0;
}
