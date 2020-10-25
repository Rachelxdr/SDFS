#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "Messages.h"
#include "Modes.h"
#include "Member.h"
#include "UdpSocket.h"
#include "Logger.h"
#include "Utils.h"

using namespace std;

#define INTRODUCER "fa20-cs425-g02-01.cs.illinois.edu"
#define UDP_PORT "4950"


#define LOGGING_FILE_NAME "logs.txt"

// --- parameters (stay tuned) ---
#define T_period 500000 // in microseconds
#define T_timeout 3 // in T_period
#define T_cleanup 3 // in T_period
#define N_b 3 // how many nodes GOSSIP want to use
// ------

#define T_switch 3 // in seconds

void *runUdpServer(void *udpSocket);
void *runSenderThread(void *node);

class Node {
public:
	// (ip_addr, port_num, timestamp at insertion) -> (hb_count, timestamp, fail_flag)
	map<tuple<string, string, string>, tuple<int, int, int>> membershipList;
	Member nodeInformation;
	UdpSocket *udpServent;
	int localTimestamp;
	int heartbeatCounter;
	time_t startTimestamp;
	ModeType runningMode;
	Logger* logWriter;
	bool activeRunning;
	bool prepareToSwitch;

	Node();
	Node(ModeType mode);
	int heartbeatToNode();
	int joinSystem(Member introdcuer);
	int listenForHeartbeats();
	int failureDetection();
	void updateNodeHeartbeatAndTime();
	void computeAndPrintBandwidth(double diff);
	int requestSwitchingMode();
	int SwitchMyMode();
	void debugMembershipList();

private:
	string populateMembershipMessage();
	string populateIntroducerMembershipMessage();
	void readMessage(string message);
	void processHeartbeat(string message);
	vector<tuple<string,string, string>> getRandomNodesToGossipTo();
};

#endif //NODE_H
