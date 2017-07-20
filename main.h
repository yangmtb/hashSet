#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <string>
#include <sys/types.h>
#include <utility>
#include <cstdint>

using pair64 = std::pair<uint64_t, uint64_t>;

void handleClient(const int32_t);
pair64 toPair64(std::string);
std::string fromPair64(pair64);
bool operator<(const pair64 &lhs, const pair64 &rhs);
bool operator==(const pair64 &lhs, const pair64 &rhs);
bool operator>(const pair64 &lhs, const pair64 &rhs);

#endif // MAIN_H_INCLUDED
