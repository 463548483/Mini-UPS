#ifndef ITEM_H
#define ITEM_H
#include <string>

#include "SQLObject.hpp"

class Item : virtual public SQLObject {
 private:
  int itemId;
  std::string description;
  int amount;
  int packageId;

 public:
  Item(const std::string & description, int amount, int packageId = 0) :
      SQLObject("items"),
      description(description),
      amount(amount),
      packageId(packageId) {}
  int getItemId() { return itemId; }
  const std::string & getDescription() const { return description; }
  int getAmount() { return amount; }
  int getPackageid() { return packageId; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (description,amount,packageId) values ('"
      << description << "'," << amount << "," ;
    if (packageId==0){
      ss<<"NULL"<<");";
    }
    else{
      ss<<packageId<<");";
    }
    return ss.str();
  }
};

#endif
