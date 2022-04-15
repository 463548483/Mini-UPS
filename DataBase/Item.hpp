#ifndef ITEM_H
#define ITEM_H
#include <string>

#include "SQLObject.hpp"

class Item : virtual public SQLObject {
 private:
  int itemId;
  std::string description;
  int amount;
  int trackingNum;

 public:
  Item(const std::string & description, int amount, int trackingNum) :
      SQLObject("ITEMS"),
      description(description),
      amount(amount),
      trackingNum(trackingNum) {}
  int getItemId() { return itemId; }
  const std::string & getDescription() const { return description; }
  int getAmount() { return amount; }
  int getTrackingNum() { return trackingNum; }
  std::string sql_insert() {
    stringstream ss;
    ss << "insert into " << tableName << " (description,amount,trackingNum) values ('"
       << description << "'," << amount << "," << trackingNum << ");";
    return ss.str();
  }
};

#endif
