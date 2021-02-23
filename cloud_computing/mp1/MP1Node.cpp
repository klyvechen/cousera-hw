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
    memcpy(&addr[4], &port, sizeof(short));
    string a;
    for (int i =0; i < (sizeof(addr)/sizeof(*addr)); i++) {
        a += (to_string((int)addr[i]) + ":");
    }
    a.pop_back();
    return new Address(a);
}

MessageHdr* replaceMeMessage(MsgTypes type, char* nodeAddr,long heartbeat) {

}

MessageHdr* createJoinqMessage(MsgTypes type, char* nodeAddr,long heartbeat) {
	MessageHdr *msg;
    size_t msgsize = sizeof(MessageHdr) + sizeof(JoinReqCnt) + 1;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = JOINREQ;
    // create JOINREQ message: format of data is {struct Address myaddr}
    JoinReqCnt *joinReq = (JoinReqCnt *) (msg+1);
    memcpy(joinReq->addr, nodeAddr, sizeof(char[6]));
    memcpy(&joinReq->heartbeat, &heartbeat, sizeof(long));

    return msg;
}

MessageHdr* createJoinpMessage(MsgTypes type, char* nodeAddr) {
	MessageHdr *msg;
    size_t msgsize = sizeof(MessageHdr) + sizeof(JoinRepCnt) + 1;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = JOINREP;
    // create JOINREQ message: format of data is {struct Address myaddr}
    JoinRepCnt *joinRep = (JoinRepCnt *) (msg+1);
    bool success = true;
    memcpy(&joinRep->success, &success, sizeof(true));
    return msg;
}

#define handleJoinMessage(type_t) ({ \
    size_t msgsize = sizeof(MessageHdr) + sizeof(type_t) + 1; \
    msg = (MessageHdr *) realloc(msg, msgsize * sizeof(char)); \
    memcpy((char*)(msg+1), data+sizeof(MessageHdr), msgsize); \
    })

MessageHdr* receiveMessage(char *data) {
    MessageHdr *msg;
    size_t hdrsize = sizeof(MessageHdr);
    msg = (MessageHdr *) malloc(hdrsize * sizeof(char));
    memcpy(msg, data, sizeof(MessageHdr));
    if (msg->msgType == JOINREQ) {
        handleJoinMessage(JoinReqCnt);
    } else if (msg->msgType == JOINREP) {
        handleJoinMessage(JoinRepCnt);
    } else if (msg->msgType == GOSSIP) {
        long nodeNum = (long)malloc(sizeof(long));
        memcpy(&nodeNum, data + sizeof(MessageHdr), sizeof(long));
        size_t hdrsize = sizeof(MessageHdr);
        size_t cntsize = sizeof(long) + sizeof(NodeStat) * nodeNum + 1;
        msg = (MessageHdr *) realloc(msg, (hdrsize + cntsize) * sizeof(char));
        memcpy((char *)(msg+1), data + sizeof(MessageHdr), cntsize);
    }
    
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

void MP1Node::addToGroup(char addr[6], long heartbeat) {
    MemberListEntry newMember = MemberListEntry(*addr, addr[4], heartbeat, time(0));
    memberNode->memberList.push_back(newMember);
}

void MP1Node::notifyJoined(Address* joinRepAddr) {
    MessageHdr* msg = createJoinpMessage(JOINREP, joinRepAddr->addr);
    emulNet->ENsend(&memberNode->addr, joinRepAddr, (char *)msg, sizeof(msg));
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
    MessageHdr *msg = receiveMessage(data);

    Address joinaddr = getJoinAddress();
    if (msg->msgType == JOINREQ) {
        JoinReqCnt *joinReq = (JoinReqCnt *) (msg + 1);
        addToGroup(joinReq->addr, joinReq->heartbeat);
        // string str(joinReq->addr);s
        Address *address = toAddress(joinReq->addr);
        notifyJoined(toAddress(joinReq->addr));
        cout <<"The join req is from " << toAddress(joinReq->addr)->getAddress()
        << ", heartbeat: " << dec << joinReq->heartbeat << endl;
    } else if (msg->msgType == JOINREP) {
        JoinRepCnt *joinRep = (JoinRepCnt *) (msg + 1);
        memberNode->inGroup = joinRep->success;
    } else if (msg->msgType == GOSSIP) {
        GossipCnt *gossip = (GossipCnt *) (msg + 1);
        int nodeNum = gossip->nodeNum;
    }

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
    cout << "member in group: "<< memberNode->inGroup << ", address is " << memberNode->addr.getAddress() << 
    " size of member: " << memberNode->memberList.size() << endl;
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
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
