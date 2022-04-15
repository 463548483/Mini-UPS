#include "../DataBase/Database.hpp"
#include <iostream>
#include <pqxx/pqxx>
using namespace std;
using namespace pqxx;

int main(int argc, char const *argv[]) {
    Database db;

    db.setup();
    return 0;
}
