#ifndef ACCOUNT_H
#define ACCOUNT_H
#include <string>

#include "SQLObject.hpp"

class Account : virtual public SQLObject {
 private:
  unsigned long accountId;
  std::string password;

 public:
  Account(unsigned long accountId, const std::string & password) :
      SQLObject("account"), accountId(accountId), password(password) {}
  const unsigned long getAccountId() const { return accountId; }
  const std::string & getPassword() const { return password; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (accountId,password) values (" << accountId << ",'"
       << password << "');";
    return ss.str();
  }
};

#endif
