add_test( HelloTest.BasicAssertions /home/zw255/erss-project-yd160-zw255/test/build/test-Amazon [==[--gtest_filter=HelloTest.BasicAssertions]==] --gtest_also_run_disabled_tests)
set_tests_properties( HelloTest.BasicAssertions PROPERTIES WORKING_DIRECTORY /home/zw255/erss-project-yd160-zw255/test/build)
set( test-Amazon_TESTS HelloTest.BasicAssertions)
