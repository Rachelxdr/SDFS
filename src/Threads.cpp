#include "../inc/Node.h"
#include "../inc/TcpSocket.h"

void *runUdpServer(void *udpSocket)
{
	UdpSocket* udp;
	udp = (UdpSocket*) udpSocket;
	udp->bindServer();
	pthread_exit(NULL);
}

void testMessages(UdpSocket* udp)
{
	sleep(2);
	for (int j = 0; j < 4; j++) {
		udp->sendMessage("127.0.0.1", UDP_PORT, "test message "+to_string(j));
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
	Node *nodeOwn;
	nodeOwn = (Node *) node;
	nodeOwn->activeRunning = true;

	// step: joining to the group -> just heartbeating to introducer
	Member introducer(getIP(INTRODUCER), UDP_PORT);
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
		time_t endTimestamp;
		time(&endTimestamp);
		double diff = difftime(endTimestamp, nodeOwn->startTimestamp);
		nodeOwn->computeAndPrintBandwidth(diff);
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
