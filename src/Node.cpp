#include "../inc/Node.h"


//add another function to Node class for failure detection
//call function before sender (heartbeat) after listenForHeartbeat

Node::Node()
{
	// create a udp object
	udpServent = new UdpSocket();
	localTimestamp = 0;
	heartbeatCounter = 0;
	time(&startTimestamp);
	// byteSent = 0;
	// byteReceived = 0;
	runningMode = ALL2ALL;
	activeRunning = false;
	prepareToSwitch = false;
	logWriter = new Logger(LOGGING_FILE_NAME);
	
}

Node::Node(ModeType mode)
{
	// create a udp object
	udpServent = new UdpSocket();
	localTimestamp = 0;
	heartbeatCounter = 0;
	time(&startTimestamp);
	// byteSent = 0;
	// byteReceived = 0;
	runningMode = mode;
	activeRunning = false;
	prepareToSwitch = false;
	logWriter = new Logger(LOGGING_FILE_NAME);
}

void Node::computeAndPrintBW(double diff)
{
#ifdef LOG_VERBOSE
	cout << "total " << udpServent->byteSent << " bytes sent" << endl;
	cout << "total " << udpServent->byteReceived << " bytes received" << endl;
	printf("elasped time is %.2f s\n", diff);
#endif
	if (diff > 0) {
		double bandwidth = udpServent->byteSent/diff;
		string message = "["+to_string(this->localTimestamp)+"] B/W usage: "+to_string(bandwidth)+" bytes/s";
#ifdef LOG_VERBOSE
		printf("%s\n", message.c_str());
#endif
		this->logWriter->printTheLog(BANDWIDTH, message);
	}
}

void Node::updateNodeHeartbeatAndTime()
{
	string startTime = ctime(&startTimestamp);
	startTime = startTime.substr(0, startTime.find("\n"));
	tuple<string, string, string> keyTuple(nodeInformation.ip, nodeInformation.port,startTime);
	tuple<int, int, int> valueTuple(heartbeatCounter, localTimestamp, 0);
	this->membershipList[keyTuple] = valueTuple;
}

string Node::populateMembershipMessage()
{
	//The string we send will be seperated line by line --> IP,PORT,HeartbeatCounter,FailFlag
	string mem_list_to_send = "";
	//Assume destination already exists in the membership list of this node, just a normal heartbeat
	switch (this->runningMode)
	{
		case GOSSIP:
			for (auto& element: this->membershipList) {
				tuple<string, string, string> keyTuple = element.first;
				tuple<int, int, int> valueTuple = element.second;
				mem_list_to_send += get<0>(keyTuple) + "," + get<1>(keyTuple) + "," + get<2>(keyTuple) + ","; 
				mem_list_to_send += to_string(get<0>(valueTuple)) + "," + to_string(get<2>(valueTuple)) + "\n";
			}
			break;
		
		default:
			string startTime = ctime(&startTimestamp);
			startTime = startTime.substr(0, startTime.find("\n"));
			mem_list_to_send += nodeInformation.ip + "," + nodeInformation.port + "," + startTime + ",";
			mem_list_to_send += to_string(heartbeatCounter) + "," + to_string(0) + "\n";
			break;
	}
	return mem_list_to_send;
}

string Node::populateIntroducerMembershipMessage(){
	string mem_list_to_send = "";
	for (auto& element: this->membershipList) {
		tuple<string, string, string> keyTuple = element.first;
		tuple<int, int, int> valueTuple = element.second;
		mem_list_to_send += get<0>(keyTuple) + "," + get<1>(keyTuple) + "," + get<2>(keyTuple) + ","; 
		mem_list_to_send += to_string(get<0>(valueTuple)) + "," + to_string(get<2>(valueTuple)) + "\n";
	}
	return mem_list_to_send;
}

/**
 * 
 * HeartbeatToNode: Sends a string version of the membership list to the receiving node. The receiving node will convert the string to
 * a <string, long> map where the key is the Addresss (IP + PORT) and value is the heartbeat counter. We then compare the Member.
 * 
 **/
int Node::heartbeatToNode()
{
	// 3. prepare to send heartbeating, and 
	string mem_list_to_send = populateMembershipMessage();
	vector<tuple<string,string,string>> targetNodes = getRandomNodesToGossipTo();
	
	//Now we have messages ready to send, need to invoke UDP client to send 
#ifdef LOG_VERBOSE
	cout << "pick " << targetNodes.size() << " of " << this->membershipList.size()-1;
	cout << " members" << endl;
#endif
	
	// 4. do gossiping
	for (uint i=0; i<targetNodes.size(); i++) {
		//cout << targetNodes[i].first << "/" << targetNodes[i].second << endl;
		Member destination(get<0>(targetNodes[i]), get<1>(targetNodes[i]));

		string message = "["+to_string(this->localTimestamp)+"] node "+destination.ip+"/"+destination.port+"/"+get<2>(targetNodes[i]);
#ifdef LOG_VERBOSE
		cout << "[Gossip]" << message.c_str() << endl;
#endif
		this->logWriter->printTheLog(GOSSIPTO, message);

		//cout << mem_list_to_send.size() << " Bytes sent..." << endl;
		// byteSent += mem_list_to_send.size();
		Messages msg(HEARTBEAT, mem_list_to_send);
		udpServent->sendMessage(destination.ip, destination.port, msg.toString());

	}
	
	return 0;
}

int Node::failureDetection(){
	//1. check local membership list for any timestamps whose curr_time - timestamp > T_fail
	//2. If yes, mark node as local failure, update fail flag to 1 and update timestamp to current time
	//3. for already failed nodes, check to see if curr_time - time stamp > T_cleanup
	//4. If yes, remove node from membership list
	vector<tuple<string,string,string>> removedVec;
	for(auto& element: this->membershipList){
		tuple<string,string,string> keyTuple = element.first;
		tuple<int, int, int> valueTuple = element.second;
#ifdef LOG_VERBOSE
		cout << "checking " << get<0>(keyTuple) << "/" << get<1>(keyTuple) << "/" << get<2>(keyTuple) << endl;
#endif
		if ((get<0>(keyTuple).compare(nodeInformation.ip) == 0) && (get<1>(keyTuple).compare(nodeInformation.port) == 0)) {
			// do not check itself
#ifdef LOG_VERBOSE
			cout << "skip it" << endl;
#endif	
			continue;
		}
		if(get<2>(valueTuple) == 0){
			if(localTimestamp - get<1>(valueTuple) > T_timeout){
				//cout << "Got " << get<0>(keyTuple) << "/" << get<1>(keyTuple) << "/" << get<2>(keyTuple) << endl;
				//cout << "local time " << localTimestamp << " vs. " << get<1>(valueTuple) << endl;
				get<1>(this->membershipList[keyTuple]) = localTimestamp;
				get<2>(this->membershipList[keyTuple]) = 1;
				
				string message = "["+to_string(this->localTimestamp)+"] node "+get<0>(keyTuple)+"/"+get<1>(keyTuple)+"/"+get<2>(keyTuple)+": Local Failure";
				cout << "[FAIL]" << message.c_str() << endl;
				this->logWriter->printTheLog(FAIL, message);
			}
		}
		else{
			if(localTimestamp - get<1>(valueTuple) > T_cleanup){
				// core dumped happened here; bug fix
				auto iter = this->membershipList.find(keyTuple);
				if (iter != this->membershipList.end()) {
					//cout << "Got " << get<0>(iter->first) << "/" << get<1>(iter->first) << "/" << get<2>(iter->first);
					//cout << " with " << to_string(get<0>(iter->second)) << "/";
					//cout << to_string(get<1>(iter->second)) << "/";
					//cout << to_string(get<2>(iter->second)) << endl;
					//cout << this->membershipList[keyTuple]
					//this->membershipList.erase(iter);
					removedVec.push_back(keyTuple);
				}
			}
		}
	}

	// O(c*n) operation, but it ensures safety
	for (uint i=0; i<removedVec.size(); i++) {
		auto iter = this->membershipList.find(removedVec[i]);
		if (iter != this->membershipList.end()) {
			this->membershipList.erase(iter);

			string message = "["+to_string(this->localTimestamp)+"] node "+get<0>(removedVec[i])+"/"+get<1>(removedVec[i])+"/"+get<2>(removedVec[i])+": REMOVED FROM LOCAL MEMBERSHIP LIST";
			cout << "[REMOVE]" << message.c_str() << endl;
			this->logWriter->printTheLog(REMOVE, message);

			//this->debugMembershipList();
		}
	}
	return 0;
}


int Node::joinSystem(Member introducer) 
{
	string mem_list_to_send = populateMembershipMessage();
	Messages msg(JOIN, mem_list_to_send);

	string message = "["+to_string(this->localTimestamp)+"] sent a request to "+introducer.ip+"/"+introducer.port+", I am "+nodeInformation.ip+"/"+nodeInformation.port;
	cout << "[JOIN]" << message.c_str() << endl;
	this->logWriter->printTheLog(JOINGROUP, message);

	udpServent->sendMessage(introducer.ip, introducer.port, msg.toString());
	return 0;
}

int Node::requestSwitchingMode()
{
	string message = nodeInformation.ip+","+nodeInformation.port;
	Messages msg(SWREQ, message);
	for(auto& element: this->membershipList) {
		tuple<string,string,string> keyTuple = element.first;
		//tuple<int, int, int> valueTuple = element.second;
		cout << "[SWITCH] sent a request to " << get<0>(keyTuple) << "/" << get<1>(keyTuple) << endl;
		udpServent->sendMessage(get<0>(keyTuple), get<1>(keyTuple), msg.toString());
	}
	return 0;
}

int Node::SwitchMyMode() 
{
	// wait for a while
	sleep(T_switch);
	// empty all messages
	udpServent->qMessages = queue<string>();
	switch (this->runningMode) {
		case GOSSIP: {
			this->runningMode = ALL2ALL;
			cout << "[SWITCH] === from gossip to all-to-all ===" << endl;
			break;
		}
		case ALL2ALL: {
			this->runningMode = GOSSIP;
			cout << "[SWITCH] === from all-to-all to gossip ===" << endl;
			break;
		}
		default:
			break;
	}
	// finishing up
	prepareToSwitch = false;
	return 0;
}
 
int Node::listenForHeartbeats() 
{
	//look in queue for any strings --> if non empty, we have received a message and need to check the membership list

	// 1. deepcopy and handle queue
	queue<string> qCopy(udpServent->qMessages);
	udpServent->qMessages = queue<string>();

	int size = qCopy.size();
	//cout << "Got " << size << " messages in the queue" << endl;
	//cout << "checking queue size " << nodeOwn->udpServent->qMessages.size() << endl;

	// 2. merge membership list
	for (int j = 0; j < size; j++) {
		//cout << qCopy.front() << endl;
		readMessage(qCopy.front());

		// Volunteerily leave
		if(this->activeRunning == false){
			return 0;
		}
		// byteReceived += qCopy.front().size();
		qCopy.pop();
	}

	return 0;
}

void Node::debugMembershipList()
{
	cout << "Membership list [" << this->membershipList.size() << "]:" << endl;
	cout << "IP/Port/JoinedTime:Heartbeat/LocalTimestamp/FailFlag" << endl;
	string message = "";
	for (auto& element: this->membershipList) {
		tuple<string,string,string> keyTuple = element.first;
		tuple<int, int, int> valueTuple = element.second;

		message += get<0>(keyTuple)+"/"+get<1>(keyTuple)+"/"+get<2>(keyTuple);
		message += ": "+to_string(get<0>(valueTuple))+"/"+to_string(get<1>(valueTuple))+"/"+to_string(get<2>(valueTuple))+"\n";
	}
	cout << message.c_str() << endl;
	this->logWriter->printTheLog(MEMBERS, message);
}

void Node::processHeartbeat(string message) {
	bool changed = false;
	vector<string> incomingMembershipList = splitString(message, "\n");
	vector<string> membershipListEntry;
	for(string list_entry: incomingMembershipList){
#ifdef LOG_VERBOSE
		cout << "handling with " << list_entry << endl;
#endif
		if (list_entry.size() == 0) {
			continue;
		}
		membershipListEntry.clear();
		membershipListEntry = splitString(list_entry, ",");
		if (membershipListEntry.size() != 5) {
			// circumvent craching
			continue;
		}

		int incomingHeartbeatCounter = stoi(membershipListEntry[3]);
		int failFlag = stoi(membershipListEntry[4]);
		tuple<string,string,string> mapKey(membershipListEntry[0], membershipListEntry[1], membershipListEntry[2]);

		if ((get<0>(mapKey).compare(nodeInformation.ip) == 0) && (get<1>(mapKey).compare(nodeInformation.port) == 0)) {
			// Volunteerily leave if you are marked as failed
			if(failFlag == 1){
				this->activeRunning = false;

				string message = "["+to_string(this->localTimestamp)+"] node "+this->nodeInformation.ip+"/"+this->nodeInformation.port+" is left";
				cout << "[VOLUNTARY LEAVE]" << message.c_str() << endl;
				this->logWriter->printTheLog(LEAVE, message);
				return;
			}
			
			// do not check itself heartbeat
#ifdef LOG_VERBOSE
			cout << "skip it" << endl;
#endif		
			continue;
		}

		map<tuple<string,string,string>, tuple<int, int, int>>::iterator it;
		it = this->membershipList.find(mapKey);
		if (it == this->membershipList.end() && failFlag == 0) {
			tuple<int, int, int> valueTuple(incomingHeartbeatCounter, localTimestamp, failFlag);
			this->membershipList[mapKey] = valueTuple;

			string message = "["+to_string(this->localTimestamp)+"] new node "+get<0>(mapKey)+"/"+get<1>(mapKey)+"/"+get<2>(mapKey)+" is joined";
			cout << "[JOIN]" << message.c_str() << endl;
			this->logWriter->printTheLog(JOINGROUP, message);
			changed = true;
		} else if(it != this->membershipList.end()) {
			// update heartbeat count and local timestamp if fail flag of node is not equal to 1. If it equals 1, we ignore it.
			if(get<2>(this->membershipList[mapKey]) != 1){
				//if incoming membership list has node with fail flag = 1, but fail flag in local membership list = 0, we have to set fail flag = 1 in local
				if(failFlag == 1){
					get<2>(this->membershipList[mapKey]) = 1;
					get<1>(this->membershipList[mapKey]) = localTimestamp;
					string message = "["+to_string(this->localTimestamp)+"] node "+get<0>(mapKey)+"/"+get<1>(mapKey)+"/"+get<2>(mapKey)+": Disseminated Failure";
					cout << "[FAIL]" << message.c_str() << endl;
					this->logWriter->printTheLog(FAIL, message);
				}
				else{
					int currentHeartbeatCounter = get<0>(this->membershipList[mapKey]);
					if(incomingHeartbeatCounter > currentHeartbeatCounter){
						get<0>(this->membershipList[mapKey]) = incomingHeartbeatCounter;
						get<1>(this->membershipList[mapKey]) = localTimestamp;
						// get<2>(this->membershipList[mapKey]) = failFlag;
						string message = "["+to_string(this->localTimestamp)+"] node "+get<0>(mapKey)+"/"+get<1>(mapKey)+"/"+get<2>(mapKey)+" from "+to_string(currentHeartbeatCounter)+" to "+to_string(incomingHeartbeatCounter);
#ifdef LOG_VERBOSE
								cout << "[UPDATE]" << message.c_str() << endl;
#endif
								this->logWriter->printTheLog(UPDATE, message);
							}
						}
						break;
					}
					default: { // ALL2ALL doesn't disseminate
						int currentHeartbeatCounter = get<0>(this->membershipList[mapKey]);
						if(incomingHeartbeatCounter > currentHeartbeatCounter){
							get<0>(this->membershipList[mapKey]) = incomingHeartbeatCounter;
							get<1>(this->membershipList[mapKey]) = localTimestamp;
							get<2>(this->membershipList[mapKey]) = failFlag;
							string message = "["+to_string(this->localTimestamp)+"] node "+get<0>(mapKey)+"/"+get<1>(mapKey)+"/"+get<2>(mapKey)+" from "+to_string(currentHeartbeatCounter)+" to "+to_string(incomingHeartbeatCounter);
#ifdef LOG_VERBOSE
							cout << "[UPDATE]" << message.c_str() << endl;
#endif
							this->logWriter->printTheLog(UPDATE, message);
						}
						break;
					}
				}
			} else {
				// TODO: we might need to bring failed node back once we have new heartbeat
			}	
		}		

	// If membership list changed in all-to-all, full membership list will be sent
	if(changed && this->runningMode == ALL2ALL){
		string mem_list_to_send = populateIntroducerMembershipMessage();
		vector<tuple<string,string,string>> targetNodes = getRandomNodesToGossipTo();

		for (uint i=0; i<targetNodes.size(); i++) {
			Member destination(get<0>(targetNodes[i]), get<1>(targetNodes[i]));

			string message = "["+to_string(this->localTimestamp)+"] node "+destination.ip+"/"+destination.port+"/"+get<2>(targetNodes[i]);
#ifdef LOG_VERBOSE
			cout << "[Gossip]" << message.c_str() << endl;
#endif
			this->logWriter->printTheLog(GOSSIPTO, message);

			Messages msg(HEARTBEAT, mem_list_to_send);
			udpServent->sendMessage(destination.ip, destination.port, msg.toString());

		}		
	}
}

/**
 * given a string message which contains a membership list, we will take the string, split it by returns, and then split it by commas, to then compare the heartbeat counters
 * of each IP,PORT,timestamp tuple with the membership list of the receiving Node.
 * 
 * Found help on how to do string processing part of this at https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
 */
void Node::readMessage(string message){
	Messages msg(message);
	switch (msg.type) {
		case HEARTBEAT: 
		case JOINRESPONSE:{
			processHeartbeat(msg.payload);
			break;
		}


		case SWREQ: {
			// got a request, send an ack back
			vector<string> fields = splitString(msg.payload, ",");
			if (fields.size() == 2) {
				cout << "[SWITCH] got a request from "+fields[0]+"/"+fields[1] << endl;
				string messageReply = nodeInformation.ip+","+nodeInformation.port;
				Messages msgReply(SWRESP, messageReply);
				udpServent->sendMessage(fields[0], fields[1], msgReply.toString());

				prepareToSwitch = true;
			}
			break;
		}
		case SWRESP: {
			// got an ack
			vector<string> fields = splitString(msg.payload, ",");
			if (fields.size() == 2) {
				cout << "[SWITCH] got an ack from "+fields[0]+"/"+fields[1] << endl;
			}
			break;
		}

		case JOIN:{
			string introducerMembershipList;
			introducerMembershipList = populateIntroducerMembershipMessage();
			Messages response(JOINRESPONSE, introducerMembershipList);
			vector<string> fields = splitString(msg.payload, ",");
			if(fields.size() >= 2){
				udpServent->sendMessage(fields[0], fields[1], response.toString());
			}
			break;
		}

		default:
			break;
	}
	//debugMembershipList();
		
}

vector<string> Node::splitString(string s, string delimiter){
	vector<string> result;
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	string token;

	while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
		token = s.substr (pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		result.push_back (token);
	}

	result.push_back (s.substr (pos_start));
	return result;
}

int main(int argc, char *argv[]) 
{
	pthread_t threads[2];
	int rc;
	Node *node;

	cout << "Mode: " << ALL2ALL << "->All-to-All, ";
	cout << GOSSIP << "->Gossip-style" << endl;
	if (argc < 2) {
		node = new Node();
	} else {
		ModeType mode = ALL2ALL;
		if (atoi(argv[1]) == 1) {
			mode = GOSSIP;
		}
		node = new Node(mode);
	}
	cout << "Running mode: " << node->runningMode << endl;
	cout << endl;

	char host[100] = {0};
	struct hostent *hp;

	if (gethostname(host, sizeof(host)) < 0) {
		cout << "error: gethostname" << endl;
		return 0;
	}
	
	if ((hp = gethostbyname(host)) == NULL) {
		cout << "error: gethostbyname" << endl;
		return 0;
	}

	if (hp->h_addr_list[0] == NULL) {
		cout << "error: no ip" << endl;
		return 0;
	}
	//cout << "hostname " << hp->h_name << endl;
	Member own(inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]), PORT, node->localTimestamp, node->heartbeatCounter);
	node->nodeInformation = own;
	cout << "Starting Node at " << node->nodeInformation.ip << "/";
	cout << node->nodeInformation.port << "..." << endl;

	// inserting its own into the list
	string startTime = ctime(& node -> startTimestamp);
	startTime = startTime.substr(0, startTime.find("\n"));
	tuple<string,string,string> mapKey(own.ip, own.port, startTime);
	tuple<int, int, int> valueTuple(own.timestamp, own.heartbeatCounter, 0);
	node->membershipList[mapKey] = valueTuple;
	node->debugMembershipList();

	int *ret;
	string cmd;
	bool joined = false;

	// listening server can run first regardless of running time commands
	if ((rc = pthread_create(&threads[0], NULL, runUdpServer, (void *)node->udpServent)) != 0) {
		cout << "Error:unable to create thread," << rc << endl;
		exit(-1);
	}

	while(1){
		cin >> cmd;

		// Deal with multiple cmd input
		if(cmd == "join"){
			if ((rc = pthread_create(&threads[1], NULL, runSenderThread, (void *)node)) != 0) {
				cout << "Error:unable to create thread," << rc << endl;
				exit(-1);
			}
			joined = true;
		} else if(cmd == "leave"){
			if(joined){
				node->activeRunning = false;
				pthread_join(threads[1], (void **)&ret);

				string message = "["+to_string(node->localTimestamp)+"] node "+node->nodeInformation.ip+"/"+node->nodeInformation.port+" is left";
				cout << "[LEAVE]" << message.c_str() << endl;
				node->logWriter->printTheLog(LEAVE, message);
				sleep(2); // wait for logging

				joined = false;
			}
		} else if(cmd == "id"){
			cout << "ID: (" << node->nodeInformation.ip << ", " << node->nodeInformation.port << ")" << endl;
		} else if(cmd == "member"){
			node->debugMembershipList();
		} else if(cmd == "switch") {
			if(joined){
				node->requestSwitchingMode();
			}
		} else if(cmd == "mode") {
			cout << "In " << node->runningMode << " mode" << endl;
		} else if(cmd == "exit"){
			cout << "exiting..." << endl;
			break;
		} else {
			cout << "[join] join to a group via fixed introducer" << endl;
			cout << "[leave] leave the group" << endl;
			cout << "[id] print id (IP/PORT)" << endl;
			cout << "[member] print all membership list" << endl;
			cout << "[switch] switch to other mode (All-to-All to Gossip, and vice versa)" << endl;
			cout << "[mode] show in 0/1 [All-to-All/Gossip] modes" << endl;
			cout << "[exit] terminate process" << endl;
		} // More command line interface if wanted 
	}

	pthread_kill(threads[0], SIGUSR1);
	if(joined){
		pthread_kill(threads[1], SIGUSR1);
	}

	pthread_exit(NULL);

	return 1;
}


