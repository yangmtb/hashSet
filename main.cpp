#include "main.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <regex>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>

using std::string;
using std::transform;
using std::ifstream;
using std::cerr;
using std::cout;
using std::vector;
using std::sort;
using std::pair;
using std::regex;
using std::stoi;
using std::to_string;
using std::getline;
using std::fill;
using std::endl;

namespace {

vector<pair64> hashSet;
string hashesLocation { "/hashes.txt" };
uint16_t port { 9998 };
bool dryRun { false };

void loadHashes()
{
    const regex md5_re { "^[A-Fa-f0-9]{32}$" };
    uint32_t hashCount { 0 };
    ifstream infile { hashesLocation.c_str() };
    try {
        hashSet.reserve(50000000);
    } catch (std::bad_alloc &) {
        cerr << "couldn't reserve enough memory" << endl;
        exit(EXIT_FAILURE);
    }
    if (not infile) {
        cerr << "couldn't open hashes file" << hashesLocation << endl;
        exit(EXIT_FAILURE);
    }

    while (infile) {
        string line;
        getline(infile, line);
        transform(line.begin(), line.end(), line.begin(), ::toupper);
        if (line.size() < 76) {
            cout << "line: " << line << endl;
            continue;
        }
        string tmp {line, 44, 32};
        static uint16_t errCount = 0;
        if (!regex_match(tmp.cbegin(), tmp.cend(), md5_re)) {
            cout << "not match: " << tmp << endl;
            if (10 == ++errCount) {
                exit(EXIT_FAILURE);
            }
            continue;
        }

        try {
            hashSet.emplace_back(toPair64(tmp));
            //hashSet.emplace_back(toPair64(line));
            hashCount++;
            if (0 == hashCount % 1000000) {
                cout << "loaded " << to_string(hashCount/1000000) << " million hashes." << endl;
                //break;
            }
        } catch (std::bad_alloc &) {
            cerr << "couldn't allocate enough memory." << endl;
            exit(EXIT_FAILURE);
        }
    }
    cout << "read in " << to_string(hashCount) << " hashes." << endl;
    infile.close();
    sort(hashSet.begin(), hashSet.end());
    if (hashSet.size() > 1) {
        pair64 foo { hashSet.at(0) };
        for (auto it = (hashSet.cbegin()+1); it != hashSet.cend(); ++it) {
            if (foo == *it) {
                cout << "==" << endl;
                hashSet.erase(it);
            }
            foo = *it;
        }
    }
}

void daemonize()
{
    const auto pid = fork();
    if (0 > pid) {
        cerr << "couldn't fork!" << endl;
        exit(EXIT_FAILURE);
    } else if (0 < pid) {
        //cout << "0 < pid" << endl;
        exit(EXIT_SUCCESS);
    }
    cout << "daemon started." << endl;
    umask(0);
    if (0 > setsid()) {
        cerr << "couldn't set sid." << endl;
        exit(EXIT_FAILURE);
    }
    if (0 > chdir("/")) {
        cerr << "couldn't chdir to root." << endl;
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

auto makeSocket()
{
    sockaddr_in server;
    memset(static_cast<void *>(&server), 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    const auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        cerr << "couldn't create a server socket." << endl;
        exit(EXIT_FAILURE);
    }
    if (0 > bind(sock, reinterpret_cast<sockaddr *>(&server), sizeof(server))) {
        cerr << "couldn't bind to port." << endl;
        exit(EXIT_FAILURE);
    }
    if (0 > listen(sock, 20)) {
        cerr << "couldn't listen for clients." << endl;
        exit(EXIT_FAILURE);
    }
    return sock;
}

}

const vector<pair64> &hashes { hashSet };

int main()
{
    /*string line ("\"0000002D9D62AEBE1E0E9DB6C4C4C7C16A163D2C\",\"1D6EBB5A789ABD108FF578263E1F40F3\",\"FFFFFFFF\",\"_sfx_0024._p\",4109,21000,\"358\",\"\"");
    const regex md5_re { ".*\"[A-Fa-f0-9]{32}\".*" };
    if (!regex_match(line.cbegin(), line.cend(), md5_re)) {
        cout << "not match: " << line << endl;
    } else {
        cout << "    match. " << line << endl;
    }
    //return 0;*/
    if (dryRun) {
        daemonize();
    }
    hashesLocation = "/home/yang/NSRLFile.txt";
    cout << "begin load hashes." << endl;
    loadHashes();
    cout << "end load: " << hashesLocation << endl;
    int32_t clientSock { 0 };
    int32_t svrSock { makeSocket() };
    sockaddr_in client;
    sockaddr *clientAddr = reinterpret_cast<sockaddr *>(&client);
    socklen_t clientLength { sizeof(client) };

    //signal(SIGHLD, SIG_IGN);

    while (true) {
        if (0 > (clientSock = accept(svrSock, clientAddr, &clientLength))) {
            cerr << "accept err" << endl;
            continue;
        }
        string ipaddr { inet_ntoa(client.sin_addr) };
        cout << "accepted a client :" << ipaddr << endl;
        if (0 == fork()) {
            handleClient(clientSock);
            if (-1 == close(clientSock)) {
                cerr << "couldn't close client: " << ipaddr << endl;
            } else {
                cout << "closed client: " << ipaddr << endl;
            }
            return 0;
        } else {
            if (-1 == close(clientSock)) {
                cerr << "parent couldn't close client: " << ipaddr << endl;
            }
        }
    }
    return EXIT_SUCCESS;
}
