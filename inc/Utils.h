#ifndef UTILS_H
#define UTILS_H
#include <iostream>
#include <string>
#include <vector>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using std::string;
using std::vector;

vector<string> splitString(string s, string delimiter);
string getIP();
string getIP(const char * host);

#endif //UDPSOCKET_H
