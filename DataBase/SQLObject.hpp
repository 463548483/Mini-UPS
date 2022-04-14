#ifndef SQLOBJECT_H
#define SQLOBJECT_H
#include <string>
using namespace std;

class SQLObject {
 public:
  const char * tableName;
  

 public:
  SQLObject(const char * tableName):tableName(tableName){}
  string getTableName(){ return tableName;}
  virtual string sql_insert()=0;
  virtual ~SQLObject(){}
};

#endif