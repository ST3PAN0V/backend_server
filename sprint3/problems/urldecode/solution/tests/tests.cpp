#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello%20World !"sv) == "Hello World !"s);
    BOOST_TEST(UrlDecode("Hello+World !"sv) == "Hello World !"s);
    BOOST_TEST(UrlDecode("Hello Artem"sv) == "Hello Artem"s);
    BOOST_TEST(UrlDecode("Hello%202World !"sv) == ""s);
    BOOST_TEST(UrlDecode("Hello%2"sv) == ""s);
}