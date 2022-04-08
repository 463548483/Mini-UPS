#include <string>

class User {
 private:
  User() {}
  unsigned long userId;
  std::string password;

 public:
  User(unsigned long userId, const std::string & password) :
      userId(userId), password(password) {}
  const unsigned long userId() const { return userId; }
  const std::string & password() const { return password; }
};

