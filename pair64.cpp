#include "main.h"
#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <stdexcept>

using std::pair;
using std::string;
using std::transform;
using std::make_pair;

string fromPair64(pair64 input)
{
    static string hexadecimal { "0123456789ABCDEF" };
    string left = "", right = "";
    uint64_t first = input.first;
    uint64_t second = input.second;
    for (int i = 0; i < 16; ++i) {
        left = hexadecimal[first & 0x0F] + left;
        right = hexadecimal[second & 0x0F] + right;
        first >>= 4;
        second >>= 4;
    }
    return left + right;
}

pair64 toPair64(string input)
{
    uint64_t left { 0 };
    uint64_t right { 0 };
    uint8_t val1 { 0 };
    uint8_t val2 { 0 };
    size_t index { 0 };
    char ch1 { 0 };
    char ch2 { 0 };
    transform(input.begin(), input.end(), input.begin(), ::tolower);
    for (index = 0; index < 16; ++index) {
        ch1 = input[index];
        ch2 = input[index+16];
        val1 = (ch1 >= '0' and ch1 <= '9') ? static_cast<uint8_t>(ch1 - '0') : static_cast<uint8_t>(ch1 - 'a') + 10;
        val2 = (ch2 >= '0' and ch2 <= '9') ? static_cast<uint8_t>(ch2 - '0') : static_cast<uint8_t>(ch2 - 'a') + 10;
        left = (left << 4) + val1;
        right = (right << 4) + val2;
    }
    return make_pair(left, right);
}

bool operator<(const pair64 &lhs, const pair64 &rhs)
{
    return (lhs.first < rhs.first) ? true : (lhs.first == rhs.first and lhs.second < rhs.second) ? true : false;
}

bool operator==(const pair64 &lhs, const pair64 &rhs)
{
    return (lhs.first == rhs.first) and (lhs.second == rhs.second);
}

bool operator>(const pair64 &lhs, const pair64 &rhs)
{
    return ((not(lhs < rhs)) and (not(lhs == rhs)));
}
