#include "../inc/Node.h"

vector<tuple<string,string, string>> Node::getRandomNodesToGossipTo()
{
    vector<tuple<string, string, string>> availableNodesInfo;
    vector<tuple<string, string, string>> selectedNodesInfo;
    vector<int> indexList;
    int availableNodes = 0;
    for(auto& element: this->membershipList){
        tuple<string, string, string> keyPair = element.first;
        tuple<int, int, int> valueTuple = element.second;
        //dont gossip to self or failed nodes
        if(get<0>(keyPair).compare(this->nodeInformation.ip) && (get<2>(valueTuple) != 1)){
            availableNodesInfo.push_back(keyPair);
            indexList.push_back(availableNodes++);
        }
    }
    switch (this->runningMode) {
        case GOSSIP: {
            srand(time(NULL));
            // N_b is a predefined number
            if (availableNodes <= N_b) return availableNodesInfo;
            int nodeCount = 0;
            while (nodeCount < N_b) {
                int randomNum = rand() % availableNodes;
                selectedNodesInfo.push_back(availableNodesInfo[indexList[randomNum]]);
                indexList.erase(indexList.begin() + randomNum);
                availableNodes--;
                nodeCount++;
            }
            return selectedNodesInfo;
        }
        //ALL2ALL
        default: {
            return availableNodesInfo;
        }
    }
}
