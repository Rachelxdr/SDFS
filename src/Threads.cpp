#include "../inc/Node.h"

/**
 * 
 * runUdpServer: Enqueue each heartbeat it receives
 *
 **/
void *runUdpServer(void *udpSocket) 
{
	// acquire UDP object
	UdpSocket* udp;
	udp = (UdpSocket*) udpSocket;
	udp->bindServer(PORT);
	pthread_exit(NULL);
}

void testMessages(UdpSocket* udp)
{
	sleep(2);
	for (int j = 0; j < 4; j++) {
		udp->sendMessage("127.0.0.1", PORT, "test message "+to_string(j));
	}
	sleep(1);
}

/**
 * 
 * runSenderThread: 
 * 1. handle messages in queue
 * 2. merge membership list
 * 3. prepare to send heartbeating
 * 4. do gossiping
 *
 **/
void *runSenderThread(void *node) 
{
	// acquire node object
	Node *nodeOwn = (Node *) node;

	nodeOwn->activeRunning = true;

	// step: joining to the group -> just heartbeating to introducer
	Member introducer(INTRODUCER, PORT);
	nodeOwn->joinSystem(introducer);

	while (nodeOwn->activeRunning) {
		
		// 1. deepcopy and handle queue, and
		// 2. merge membership list
		nodeOwn->listenForHeartbeats();
		
		// Volunteerily leave
		if(nodeOwn->activeRunning == false){
			pthread_exit(NULL);
		}

		//add failure detection in between listening and sending out heartbeats
		nodeOwn->failureDetection();
		
		// keep heartbeating
		nodeOwn->localTimestamp++;
		nodeOwn->heartbeatCounter++;
		nodeOwn->updateNodeHeartbeatAndTime();
		
		// 3. prepare to send heartbeating, and 
		// 4. do gossiping
		nodeOwn->heartbeatToNode();

		// for debugging
		//nodeOwn->debugMembershipList();
		time_t endTimestamp;
		time(&endTimestamp);
		double diff = difftime(endTimestamp, nodeOwn->startTimestamp);
		nodeOwn->computeAndPrintBW(diff);
#ifdef LOG_VERBOSE
		cout << endl;
#endif
		if (nodeOwn->prepareToSwitch) {
			cout << "[SWITCH] I am going to swtich my mode in " << T_switch << "s" << endl;
			nodeOwn->SwitchMyMode();
		} else {
			usleep(T_period);
		}
	}

	pthread_exit(NULL);
}