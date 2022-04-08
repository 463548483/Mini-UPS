#include <string>

class Item {
 private:
  Item() {}
  int itemId;
  std::string description;
  int amount;
  int packageid;

 public:
  Item(int id, const std::string & description, int amount, int packageid) :
      itemId(itemId), description(description), amount(amount), packageid(packageid) {}
  int itemId(){return itemId;}
  const std::string & description() const { return description; }
  int amount() { return amount; }
  int packageid() { return packageid; }
};
