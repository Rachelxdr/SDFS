#ifndef MEMBER_H
#define MEMBER_H

#include <iostream>
#include <string>

#include "Utils.h"

using std::string;
using std::to_string;

class Member {
public:
	string ip;
	string port;
	int timestamp;
	int heartbeatCounter;
	int failed_flag;
	Member();
	Member(string nodeIp, string nodePort, int nodeTimestamp, int heartbeatCounter);
	Member(string nodeIp, string nodePort);
	Member(string nodeIp, string nodePort, int nodeTimestamp);
	string toString();
};


#endif //MEMBER_H
