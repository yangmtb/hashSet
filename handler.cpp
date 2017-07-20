#include "main.h"
#include <algorithm>
#include <iostream>
#include <array>
#include <exception>
#include <inttypes.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>

#include <unistd.h>

using std::string;
using std::find;
using std::find_if;
using std::transform;
using std::vector;
using std::remove;
using std::exception;
using std::binary_search;
using std::pair;
using std::copy;
using std::back_inserter;
using std::array;
using std::fill;
using std::cerr;
using std::cout;
using std::endl;

extern const vector<pair64> &hashes;

namespace {
    class NetworkTimeout : public std::exception {
    public:
        virtual const char * what() const noexcept { return "network timeout"; }
    };

    class NetworkError : public std::exception {
    public:
        virtual const char * what() const noexcept { return "network error"; }
    };

    string readLine(const int32_t sockfd, int timeout = 15)
    {
        static vector<char> buffer;
        static array<char, 8192> rdbuf;
        struct pollfd pfd;
        struct timeval start;
        struct timeval now;
        time_t elapsedTime;
        ssize_t bytesReceived;
        constexpr auto MEGABYTE = 1 << 20;
        if (buffer.capacity() < MEGABYTE) {
            buffer.reserve(MEGABYTE);
        }
        auto iter = find(buffer.begin(), buffer.end(), '\n');
        if (iter != buffer.end()) {
            vector<char> newbuf(buffer.begin(), iter);
            buffer.erase(buffer.begin(), iter+1);
            buffer.erase(remove(newbuf.begin(), newbuf.end(), '\r'), newbuf.end());
            return string(newbuf.begin(), newbuf.end());
        }
        gettimeofday(&start, nullptr);
        now.tv_sec = start.tv_sec;
        now.tv_usec = start.tv_usec;
        elapsedTime = now.tv_sec - start.tv_sec;
        while ((elapsedTime < timeout)) {
            pfd.fd = sockfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            fill(rdbuf.begin(), rdbuf.end(), 0);
            if ((buffer.size() > MEGABYTE) || (-1 == poll(&pfd, 1, 1000)) || (pfd.revents & POLLERR) || (pfd.revents & POLLHUP) || (pfd.revents & POLLNVAL)) {
                cerr << "notwor error: ";
                if (buffer.size() > MEGABYTE) {
                    cerr << "buffer too large." << endl;
                } else if (pfd.revents & POLLERR) {
                    cerr << "pollerr." << endl;
                } else if (pfd.revents & POLLHUP) {
                    cerr << "pollhup." << endl;
                } else if (pfd.revents & POLLNVAL) {
                    cerr << "pollnval." << endl;
                } else {
                    cerr << "other." << endl;
                }
                throw NetworkError();
            }
            if (pfd.revents & POLLIN) {
                bytesReceived = recvfrom(sockfd, &rdbuf[0], rdbuf.size(), 0, NULL, 0);
                if (0 == bytesReceived) {
                    cerr << "read line on closed socket." << endl;
                    throw NetworkError();
                }
                copy(rdbuf.begin(), rdbuf.begin()+bytesReceived, back_inserter(buffer));
            }
            iter = find(buffer.begin(), buffer.end(), '\n');
            if (iter != buffer.end()) {
                string line(buffer.begin(), iter);
                if (line.at(line.size()-1) == '\r') {
                    line = string (line.begin(), line.end()-1);
                }
                buffer.erase(buffer.begin(), iter+1);
                return line;
            }
            gettimeofday(&now, nullptr);
            elapsedTime = now.tv_sec - start.tv_sec;
        }
        throw NetworkTimeout();
    }

    void writeLine(const int32_t sockfd, string &&line)
    {
        string output = line + "\r\n";
        const char *msg = output.c_str();
        if (-1 == send(sockfd, reinterpret_cast<const void *>(msg), output.size(), 0)) {
            cerr << "send err" << endl;
            throw NetworkError();
        }
    }

    auto tokenize(string &&line, char character = ' ')
    {
        vector<string> rv;
        transform(line.begin(), line.end(), line.begin(), ::toupper);
        auto begin = find_if(line.cbegin(), line.cend(), [&](auto x) { return x != character; });
        auto end = (begin != line.cend()) ? find(begin+1, line.cend(), character) : line.cend();
        while (begin != line.cend()) {
            rv.emplace_back(string { begin, end } );
            if (end == line.cend()) {
                break;
            }
            begin = find_if(end+1, line.cend(), [&](auto x) { return x != character; });
            end = (begin != line.cend()) ? find(begin+1, line.cend(), character) : line.cend();
        }
        return rv;
    }

    string generateResponse(vector<string>::const_iterator begin, vector<string>::const_iterator end)
    {
        string rv = "OK ";
        for (auto i = begin; i != end; ++i) {
            bool present = binary_search(hashes.cbegin(), hashes.cend(), toPair64(*i));
            rv += present ? "1" : "0";
        }
        return rv;
    }
}

void handleClient(const int32_t fd)
{
    enum class Command {
        Bye = 0,
        Query = 1,
        Unknown = 2
    };
    try {
        auto commands = tokenize(readLine(fd));
        while (true) {
            auto cmdString = commands.at(0);
            Command cmd = Command::Unknown;
            if (cmdString == "QUERY") {
                cmd = Command::Query;
            } else if (cmdString == "Byte") {
                cmd = Command::Bye;
            }
            switch (cmd) {
            case Command::Bye:
                return;
            case Command::Query:
                writeLine(fd, generateResponse(commands.begin()+1, commands.end()));
                break;
            case Command::Unknown:
                cerr << "unknown" << endl;
                break;
            }
            commands = tokenize(readLine(fd));
        }
    } catch (std::exception &) {
        cerr << "command err" << endl;
    }
}
