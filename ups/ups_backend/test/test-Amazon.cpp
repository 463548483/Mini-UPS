// #include <gtest/gtest.h>
// #include <gmock/gmock.h>
#include "../Server/BaseServer.hpp"

// using ::testing::Return;
// using ::testing::_; // Matcher for parameters

// class MockServer: public BaseServer{
//     public:
//     MOCK_METHOD2(processAmazonUpsQuery,void(pqxx::connection *,AUCommand &))
// }

// class MockAUCommand: public AUCommand{
//     public:
//     MOCK_METHOD(const AUQueryUpsid &,queryupsid,());
// }

// class MockAUQueryUpsid: public AUQueryUpsid{
//     public:
//     MOCK_METHOD0(int64_t,upsid,());
// }


// TEST(QueryUpsidTest, Fill) {
//     MockAUQueryUpsid auQueryUpsid;
//     MockAUCommand auCommand;
//     ON_CALL(auCommand, queryupsid()).WillByDefault(Return(1001));
// }

int main(int argc, char **argv)
{
//   testing::InitGoogleMock(&argc, argv);

//   // Runs all tests using Google Test.
//   return RUN_ALL_TESTS();
    BaseServer baseServer("127.0.0.1","12345",0,10);
    //baseServer.findTrucks();
    baseServer.amazonCommunicate();
}