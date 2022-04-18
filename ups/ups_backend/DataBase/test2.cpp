#include <iostream>
#include <pqxx/pqxx>
using namespace std;
using namespace pqxx;

void myTransaction(connection *C) {
    /* Create a transactional object. */
    transaction<serializable, read_write> W(*C);

    /* Execute SQL query */
    W.exec("UPDATE tbl_Employee SET EmpName ='Lucy' WHERE EmpID=1;");
    W.exec("UPDATE tbl_Employee SET EmpName ='Eve' WHERE EmpID=4;");
    W.commit();
    cout << "Player value changed successfully with serialization transaction" << endl;
}

int main(int argc, char const *argv[]) {
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
          	return 1;
        }
    } catch (const std::exception &e){
        cerr << e.what() << std::endl;
        return 1;
    }

    /* Execute SQL query */
    bool retry = true;
    while (retry) {
        try {
            myTransaction(C);
            retry = false;
        }
        catch (const pqxx::serialization_failure &e) {
            cout << "Serialization failure!!!\n";
            // exit(EXIT_FAILURE);
        }
        catch (const exception &e) {
            cerr << e.what() << std::endl;
            retry = false;
        }
    }
    
    return 0;
}
