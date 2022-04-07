#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <pqxx/pqxx>
#include <string>
#include <ctime>

#define HOST "localhost"
#define DATABASE "postgres"
#define USER "postgres"
#define PASSWORD "postgres"

class DatabaseConnectionError : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "Cannot connect to the database.";
    }
};

class Database
{
public:
    Database();
    ~Database();

    pqxx::connection *connectToDatabase();
    void setup();
    void dropATable(pqxx::connection *, std::string tableName);
    void cleanTables(pqxx::connection *);
    void createTables(pqxx::connection *);

    // pqxx::connection *getConnection();
};

#endif