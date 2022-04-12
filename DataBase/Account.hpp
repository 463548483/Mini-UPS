#ifndef ACCOUNT_H
#define ACCOUNT_H
#include <string>

#include "SQLObject.hpp"

class Account : virtual public SQLObject {
 private:
  unsigned long accountId;
  std::string username;
  std::string password;

 public:
  Account(unsigned long accountId, const std::string &username, const std::string &password) :
      SQLObject("ACCOUNT"), accountId(accountId), username(username), password(password) {}
  const unsigned long getAccountId() const { return accountId; }
  const std::string & getPassword() const { return password; }
  const std::string & getUsername() const { return username; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (accountId,username,password) values (" << accountId << ",'"
       << username << "','" << password << "');";
    return ss.str();
  }
};

#endif
