/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 8;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

Address* toAddress(char addr[6]) {
    string a;
    for (int i =0; i < (sizeof(addr)/sizeof(*addr)); i++) {
        a += (to_string((int)addr[i]) + ":");
    }
    a.pop_back();
    return new Address(a);
}

Address* toAddress(int id, short port) {
    char addr[6];
    memcpy(&addr[0], &id, sizeof(int));
    memcpy(&addr[4], &port, sizeof(int));
    string a;
    for (int i =0; i < (sizeof(addr)/sizeof(*addr)); i++) {
        a += (to_string((int)addr[i]) + ":");
    }
    a.pop_back();
    return new Address(a);
}

int getId(char addr[6]) {
    return (int) addr[0];
}

void printNodeStatus(Member* memberNode, int id, int port, string info);

/**
 * create Message Zone:
 * create different type of message.
 **/
MessageHdr* createMessageHeader(MsgTypes type, char fromaddr[6], size_t msgsize) {
    MessageHdr *msg;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = type;
    memcpy(&msg->msgsize, &msgsize, sizeof(size_t));
    memcpy(msg->fromaddr, fromaddr, sizeof(char[6]));
    return msg;
}

MessageHdr* createJoinqMessage(MsgTypes type, char nodeAddr[6],long heartbeat) {
    size_t msgsize = sizeof(MessageHdr) + sizeof(JoinReqCnt) + 4;
	MessageHdr *msg = createMessageHeader(JOINREQ, nodeAddr, msgsize);

    JoinReqCnt *joinReq = (JoinReqCnt *) (msg+1);
    memcpy(joinReq->addr, nodeAddr, sizeof(char[6]));
    memcpy(&joinReq->heartbeat, &heartbeat, sizeof(long));
    return msg;
}

void buildPeer(PeerInfo* peer, char a[6], long heartbeat) {
    memcpy(&peer->heartbeat, &heartbeat, sizeof(long));
    memcpy(peer->addr, a, sizeof(char[6]));
}

MessageHdr* createJoinpMessage(Member* nodeInfo) {
    bool success = true;
    int peerNum =  nodeInfo->memberList.size()-1;
    size_t msgsize = sizeof(MessageHdr) + sizeof(JoinRepCnt) + sizeof(PeerInfo) * (peerNum) + 14;
    
	MessageHdr *msg = createMessageHeader(JOINREP, nodeInfo->addr.addr, msgsize);

    JoinRepCnt *joinRep = (JoinRepCnt *) (msg+1);
    memcpy(&joinRep->success, &success, sizeof(true));
    memcpy(&joinRep->rcmdNum, &peerNum, sizeof(int));
    PeerInfo *peer = (PeerInfo*) joinRep+1;
    for (int i = 0; i < peerNum ; i++) {
        MemberListEntry entry = nodeInfo->memberList[i];
        Address* addr = toAddress(entry.id, entry.port);
        // buildPeer(peer, addr->addr, entry.heartbeat);
        memcpy(&peer->heartbeat, &entry.heartbeat, sizeof(long));
        memcpy(peer->addr, addr->addr, sizeof(char[6]));
        peer++;
        delete addr;
    }
    return msg;
}

MessageHdr* createMkPrRequestMessage(Member* nodeInfo) {
    // create MKPRREQ message:
    size_t msgsize = sizeof(MessageHdr) + sizeof(MkPrReqCnt) + sizeof(PeerInfo) + sizeof(long) + sizeof(char[6]) + 12;
	MessageHdr *msg = createMessageHeader(MKPRREQ, nodeInfo->addr.addr, msgsize);

    // attach peer info after content
    MkPrReqCnt *mkPrReq = (MkPrReqCnt *) (msg+1);
    mkPrReq->dummy = true;
    PeerInfo *peer = (PeerInfo*) mkPrReq+1;
    // buildPeer(peer, nodeInfo->addr.addr, nodeInfo->heartbeat);
    memcpy(&peer->heartbeat, &nodeInfo->heartbeat, sizeof(long));
    memcpy(peer->addr, &nodeInfo->addr, sizeof(char[6]));
    return msg;
}

MessageHdr* createMkPrResponseMessage(Member* nodeInfo) {
    // create MKPRREP message:
    size_t msgsize = sizeof(MessageHdr) + sizeof(MkPrRepCnt) + sizeof(PeerInfo) + sizeof(long) + sizeof(char[6]) + 12;
	MessageHdr *msg = createMessageHeader(MKPRREP, nodeInfo->addr.addr, msgsize);

    MkPrRepCnt *mkPrRep = (MkPrRepCnt *) (msg+1);
    mkPrRep->success = true;
    // attach peer info after content
    PeerInfo *peer = (PeerInfo*) mkPrRep+1;
    buildPeer(peer, nodeInfo->addr.addr, nodeInfo->heartbeat);
    return msg;
}

MessageHdr* createRlPrRequestMessage(Member* nodeInfo, char applyaddr[6]) {
    // create RLPRREQ message:
    size_t msgsize = sizeof(MessageHdr) + sizeof(RlPrReqCnt) + sizeof(long) + sizeof(char[8]) * 2;
	MessageHdr *msg = createMessageHeader(RLPRREQ, nodeInfo->addr.addr, msgsize);

    RlPrReqCnt *rlPrReq = (RlPrReqCnt *) (msg+1);
    memcpy(rlPrReq->fromaddr, nodeInfo->addr.addr, sizeof(char[6]));
    memcpy(rlPrReq->toaddr, applyaddr, sizeof(char[6]));
    memcpy(&rlPrReq->heartbeat, &nodeInfo->heartbeat, sizeof(long));
    return msg;
}

MessageHdr* createRlPrResponseMessage(Member* nodeInfo, char toaddr[6], long heartbeat) {
    // create RLPRREQ message:
    size_t msgsize = sizeof(MessageHdr) + sizeof(RlPrRepCnt) + sizeof(bool) + sizeof(char[6]) + sizeof(long) + 12;
	MessageHdr *msg = createMessageHeader(RLPRREP, nodeInfo->addr.addr, msgsize);

    RlPrRepCnt *rlPrReq = (RlPrRepCnt *) (msg+1);
    bool success = true;
    memcpy(&rlPrReq->success, &success, sizeof(bool));
    memcpy(&rlPrReq->heartbeat, &nodeInfo->heartbeat, sizeof(long));
    memcpy(rlPrReq->toaddr, toaddr, sizeof(char[6]));
    return msg;
}

MessageHdr* receiveMessage(void *env, char *data) {
    MessageHdr *msg;
    size_t hdrsize = sizeof(MessageHdr);
    msg = (MessageHdr *) malloc(hdrsize);
    memcpy(msg, data, hdrsize);
    size_t msgsize = msg->msgsize; 
    msg = (MessageHdr *) realloc(msg, msgsize); 
    memcpy((char*)(msg+1), data + hdrsize, msgsize - hdrsize); 
    return msg;
}


/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        MessageHdr *msg = createJoinqMessage(JOINREQ, memberNode->addr.addr, memberNode->heartbeat);

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif
        size_t size = sizeof(MessageHdr) + sizeof(JoinReqCnt) + 1; 
        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, size);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

void addPeer(Member* env, char addr[6], long heartbeat) {
    MemberListEntry newMember = MemberListEntry(*addr, addr[4], heartbeat, time(0));
    env->memberList.push_back(newMember);
}

void removePeer(Member* member, char addr[6]) {
    cout << "node " << member->addr.getAddress() << " is removing node" << endl;
    if (member->memberList.size() == 0) {
        return;
    }
    cout << "-3" << endl;
    int toRemove = 0;
    for (int i = 0; i < member->memberList.size(); i++) {
        cout << i << endl;
        int id = getId(addr);
        if (member->memberList[i].id == id) {
            toRemove = i;
            break;
        }
    }
    cout << "-1" << endl;
    member->memberList.erase(member->memberList.begin() + toRemove);
    cout << "-2" << endl;
}

void doReplacePeer(Member* nodeInfo, EmulNet *emulNet, char applyAddr[6]) {
    MemberListEntry entry = nodeInfo->memberList.back();
    nodeInfo->memberList.pop_back();
    MessageHdr* msg = createRlPrRequestMessage(nodeInfo, applyAddr);

    Address* toaddr = toAddress(entry.id, entry.port);
    emulNet->ENsend(&nodeInfo->addr, toaddr, (char *)msg, msg->msgsize);
    delete toaddr;
}

void checkMakePeerStatus(Member* nodeInfo, EmulNet *emulNet, Address* procNode) {
    //  int peerNum =  nodeInfo->memberList.size();
    // if (peerNum == MAX_PEERS) {
    //     MemberListEntry entry = nodeInfo->memberList.back();
    //     nodeInfo->memberList.pop_back();
    //     MessageHdr* msg = createRlPrRequestMessage(nodeInfo, procNode);
    //     Address* toaddr = toAddress(entry.id, entry.port);
    //     emulNet->ENsend(&nodeInfo->addr, toaddr, (char *)msg, msg->msgsize);
    //     delete toaddr;
    //     cout << "the memberlist size is " << nodeInfo->memberList.size() << endl;
    // }
}

void doJoinResponse(Member* nodeInfo, EmulNet *emulNet, char addr[6], long heartbeat) {
    Address* toaddr = toAddress(addr);
    addPeer(nodeInfo, addr, heartbeat);
    MessageHdr* sendMsg = createJoinpMessage(nodeInfo);
    emulNet->ENsend(&nodeInfo->addr, toaddr, (char *) sendMsg, sendMsg->msgsize);
    cout <<"Node " << toaddr->getAddress() << " want to join the group, heartbeat: " << dec << heartbeat << endl;
    //printNodeStatus(nodeInfo, 1, 0, " handle join request finished ");
    free(sendMsg);
    delete toaddr;
}

void handleJoinReq(Member* nodeInfo, EmulNet *emulNet, JoinReqCnt *joinReq) {
    Address* toaddr = toAddress(joinReq->addr);
    Address* hostaddr = &nodeInfo->addr;
    //printNodeStatus(nodeInfo, 1, 0, " handle join request start ");
    if (nodeInfo->memberList.size() == MAX_PEERS) {
        cout << "Host " << hostaddr->getAddress() << " reach max peer, do replace peer" << endl; 
        doReplacePeer(nodeInfo, emulNet, joinReq->addr);
    } else {
        doJoinResponse(nodeInfo, emulNet, joinReq->addr, joinReq->heartbeat);
    }
    delete toaddr;
}

void handleJoinRep(Member* nodeInfo, EmulNet *emulNet, JoinRepCnt *joinRep) {
    //printNodeStatus(nodeInfo, 1, 0, " handle join response start ");
    bool success = joinRep->success;
    cout << "start join response" << endl;
    if (success) {
        MessageHdr* sendMsg = createMkPrRequestMessage(nodeInfo);
        PeerInfo* peer = (PeerInfo*) joinRep+1;
        for (int i = 0; i < joinRep->rcmdNum; i++) {
            Address* toaddr = toAddress(peer->addr);
            cout << "Make peer from "<< nodeInfo->addr.getAddress() << " to " << toaddr->getAddress() << ", the heartbeat is " << peer->heartbeat << endl;
            emulNet->ENsend(&nodeInfo->addr, toaddr, (char *)sendMsg, sendMsg->msgsize);
            peer++;  
            delete toaddr;
        } 
        free(sendMsg);
    }
    cout << "finish join response" << endl;
    //printNodeStatus(nodeInfo, 1, 0, " handle join response finished ");
}

void handleReplacePeerReq(Member* nodeInfo, EmulNet *emulNet, RlPrReqCnt *rlprReqCnt, char fromaddr[6]) {
    //printNodeStatus(nodeInfo, 1, 0, " handle make peer request start ");
    MessageHdr* msg = createRlPrResponseMessage(nodeInfo, rlprReqCnt->toaddr, rlprReqCnt->heartbeat);
    removePeer(nodeInfo, rlprReqCnt->toaddr);

    Address* fromaddrBig = toAddress(fromaddr);
    Address* toaddrBig = toAddress(((RlPrRepCnt*)(msg+1))->toaddr);
    cout << "To address is " << toaddrBig->getAddress() <<endl;
    cout << "send the rl peer response to " << fromaddrBig->getAddress() << endl;
    cout << "toaddrBig # " << (char*) (((RlPrRepCnt*)(msg+1))->toaddr) << endl;

    emulNet->ENsend(&nodeInfo->addr, fromaddrBig, (char *)msg, msg->msgsize);
    cout << "send the rl peer response end" << endl;
    delete fromaddrBig;
    delete toaddrBig;
    //printNodeStatus(nodeInfo, 1, 0, " handle make peer request finished ");
}

void handleReplacePeerRep(Member* nodeInfo, EmulNet *emulNet, RlPrRepCnt *rlprRepCnt) {
    cout << "handleReplacePeerRep" << endl;
    Address* toAddr = toAddress(rlprRepCnt->toaddr);
    cout << "To address " << toAddr->getAddress() << endl;
    cout << "success " << rlprRepCnt->success << endl;
    cout << "heartbeat " << rlprRepCnt->heartbeat << endl;
    doJoinResponse(nodeInfo, emulNet, rlprRepCnt->toaddr, rlprRepCnt->heartbeat);
    delete toAddr;
}

void handleMakePeerReq(Member* nodeInfo, EmulNet *emulNet, MkPrReqCnt *mkpeerReq, char fromaddr[6]) {
    //printNodeStatus(nodeInfo, 1, 0, " handle make peer request start ");
    PeerInfo* peer = (PeerInfo*) mkpeerReq + 1;
    Address* toaddr = toAddress(peer->addr);
    addPeer(nodeInfo, peer->addr, peer->heartbeat);
    MessageHdr* sendMsg = createMkPrResponseMessage(nodeInfo);
    emulNet->ENsend(&nodeInfo->addr, toaddr, (char *)sendMsg, sendMsg->msgsize);
    delete toaddr;
    free(sendMsg);
    //printNodeStatus(nodeInfo, 1, 0, " handle make peer request finished ");
}

void handleMakePeerRep(Member* nodeInfo, MkPrRepCnt *mkpeerRep) {
    //printNodeStatus(nodeInfo, 1, 0, " handle make peer response start");
    PeerInfo* peer = (PeerInfo*) mkpeerRep + 1;
    addPeer(nodeInfo, peer->addr, peer->heartbeat);
    //printNodeStatus(nodeInfo, 1, 0, " handle make peer response finished");
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 * JOINREQ the join group request, if the host reach the max peer staus, send RLPRREQ.
 * JOINREP
 * RLPRREQ
 * RLPRREP
 * MKPRREQ
 * MKPRREP
 */
bool MP1Node::recvCallBack(void *env, char *data, int size) {
	/*
	 * Your code goes here
	 */
    MessageHdr *msg = receiveMessage(env, data);
    if (msg->msgType == JOINREQ) {
        cout << "JOINREQ" << endl;
        handleJoinReq((Member *) env, emulNet, (JoinReqCnt *) (msg + 1));
    } else if (msg->msgType == JOINREP) {
        cout << "JOINREP" << endl;
        JoinRepCnt *joinRep = (JoinRepCnt *) (msg + 1);
        handleJoinRep((Member *) env, emulNet, joinRep);
        memberNode->inGroup = joinRep->success;
    } else if (msg->msgType == RLPRREQ) {
        cout << "RLPRREQ" << endl;
        handleReplacePeerReq((Member *) env, emulNet, (RlPrReqCnt *) (msg + 1), msg->fromaddr);
    } else if (msg->msgType == RLPRREP) {
        cout << "RLPRREP" << endl;
        handleReplacePeerRep((Member *) env, emulNet, (RlPrRepCnt *) (msg + 1));
    } else if (msg->msgType == MKPRREQ) {
        cout << "MKPRREQ" << endl;
        handleMakePeerReq((Member *) env, emulNet, (MkPrReqCnt *) (msg + 1), msg->fromaddr);
    } else if (msg->msgType == MKPRREP) {
        cout << "MKPRREP" << endl;
        handleMakePeerRep((Member *) env, (MkPrRepCnt *) (msg + 1));
    } else if (msg->msgType == GOSSIP) {
        cout << "GOSSIP" << endl;
        GossipCnt *gossip = (GossipCnt *) (msg + 1);
        int nodeNum = gossip->nodeNum;
    }
    free(msg);
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
    // cout << "member in group: "<< memberNode->inGroup << ", address is " << memberNode->addr.getAddress() << 
    // " size of member: " << memberNode->memberList.size() << endl;
    std::vector<MemberListEntry> memberList = memberNode->memberList;
    for (std::vector<MemberListEntry>::iterator it = memberList.begin() ; it != memberList.end(); ++it) {
        MemberListEntry m = *it;
        
    }

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void printNodeStatus(Member* memberNode, int id, int port, string info)
{
    Address *addr = toAddress(id, port);
    bool sameNode = memcmp(memberNode->addr.addr, addr->addr, 6) == 0;
    // cout << "trace node, " << memberNode->addr.getAddress() << " " << addr->getAddress() << " " << endl; 
    if (sameNode) {
        cout << "Track the node: " << addr->getAddress() << " " << info;
        cout << "neighbor are";
        for (int i = 0; i < memberNode->memberList.size(); i++) {
            cout << ", " << memberNode->memberList[i].id;
        }
        cout << endl;
    }
    delete addr;
}
