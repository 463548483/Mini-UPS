#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "utils.hpp"
using namespace std;

int main(int argc, char const *argv[]) {
    double a = 1000000.0;
    double b = 10034.2;
    a += b;
    cout << a << endl;

    if (fabs(a - 1010034.2) < 1e-5)
        cout << "yes\n";

    stringstream ss;
    ss << fixed << a << ' ' << a;
    cout << ss.str() << endl;
    // double result = 0;
    // cout << isValidPrice("-123.21", false, &result) << endl;
    // cout << isValidPrice("0", false, &result) << endl;
    // cout << isValidPrice("0", true, &result) << endl;
    // cout << isValidPrice("-123..21", false, &result) << endl;
    // cout << isValidPrice("-123.21", true, &result) << endl;
    // cout << isValidPrice("123.21", false, &result) << endl;
    // cout << isValidPrice("123.21aaxj", false, &result) << endl;
    return 0;
}
