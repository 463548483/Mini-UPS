#include <iostream>
#include <cstdlib>
#include <sstream>
#include "utils.hpp"
using namespace std;

// check if the string is a pure number, positive integer
bool isValidNumber(const string &line) {
    if (line.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] < '0' || line[i] > '9')
            return false;
        if (line[i] == '0' && line.size() > 1 && i == 0) {
            return false;
        }
    }
    return true;
}

// sequence of one or more base-10 digits
bool isValidAccountNum(const string &line) {
    if (line.size() == 0)
        return false;
    for (char c : line) {
        if (c < '0' || c > '9')
            return false;
    }
    return true;
}

// sequence of one or more alphabets and numbers
bool isValidSymbol(const string &line) {
    if (line.size() == 0)
        return false;
    for (char c : line) {
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            continue;
        else
            return false;
    }
    return true;
}

// check if the string is a valid fractional
bool isValidPrice(const string &line, bool mustPositive, double *result) {
    if (line.size() == 0)
        return false;

    size_t remain = 0;
    *result = stod(line, &remain);
    if (remain != line.size() || (mustPositive && *result < 0))
        return false;
    return true;
}

// check if the string is a valid integer
bool isValidInteger(const string &line, bool mustPositive, int *result) {
    if (line.size() == 0)
        return false;

    size_t remain = 0;
    *result = stoi(line, &remain);
    if (remain != line.size() || (mustPositive && *result < 0))
        return false;
    return true;
}

time_t getTimeStamp() {
    return time (NULL);
}

void exitWithError(string errmsg) {
    cerr << errmsg << endl;
    exit(EXIT_FAILURE);
}