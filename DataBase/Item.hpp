#ifndef ITEM_H
#define ITEM_H
#include <string>

#include "SQLObject.hpp"

class Item : virtual public SQLObject {
 private:
  int itemId;
  std::string description;
  int amount;
  int packageid;

 public:
  Item(int id, const std::string & description, int amount, int packageid = 0) :
      SQLObject("items"),
      itemId(id),
      description(description),
      amount(amount),
      packageid(packageid) {}
  int getItemId() { return itemId; }
  const std::string & getDescription() const { return description; }
  int getAmount() { return amount; }
  int getPackageid() { return packageid; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (itemId,description,amount,packageid) values ("
       << itemId << ",'" << description << "'," << amount << "," ;
    if (packageid==0){
      ss<<"NULL"<<");";
    }
    else{
      ss<<packageid<<");";
    }
    return ss.str();
  }
};

#endif
