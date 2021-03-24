/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define MAX_PEERS 3

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
	// JOINREQ : request to join the group
    JOINREQ,

	// JOINREP : response for join request, send the recommend list to the join friend. 
	// if the the number of the host's friend hits the max peer, host should unfriend one 
	// of its friend before it sends back the response to the applier. And if the host
	// in the maximum friend state, the recommend list only include the unfriend node.
    JOINREP,

	// RLPRREQ : I unfriend you and you will wait for the an other's make peer request. And please
	// reserve the last occupation for the node's request. if no one send the request to you,
	// please send the make peer request to me.
	RLPRREQ,

	// RLPRREP : OK, I unfriend you and I will wait for the node's make peer request. if I didn't 
	// receive the request in the timeout, I will reply back the MKPRREQ again. Please send the 
	// join response back to the origin node.
	RLPRREP,

	// MKPRREQ: request to become a peer, 
	// if the node is already in the group, it send the MKPRREQ request to make the peer 
	// relationship with other. The MKPRREQ should follow by the recommend list after
	// receiving the JOINREP response.
	MKPRREQ, 

	// MKPRREP: response, yes or no (yes if my peer number < MAX_PEER)
	// if the receiver in the maximum friend state, it pick one of its friend,
	// unfriend the node and the the RLPRREQ to it. if the receive connected the node
	// already, it will response negative to the sender.
	MKPRREP,  

	GOSSIP,
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct PeerInfo {
	long heartbeat;
	char addr[6];
}PeerInfo;


typedef struct GossipCnt {
	int nodeNum;
	PeerInfo peerInfos[];
}GossipCnt;

typedef struct JoinReqCnt {
	long heartbeat;
	char addr[6];	
}JoinReqCnt;


typedef struct JoinRepCnt {
	bool success;
	int rcmdNum;
}JoinRepCnt;

// need one PeerInfo
typedef struct MkPrReqCnt {
	bool dummy;
}MkPrReqCnt;

// need one PeerInfo
typedef struct MkPrRepCnt {
	bool success;
}MkPrRepCnt;

typedef struct RlPrReqCnt {
	char fromaddr[6];
	long heartbeat;
	char toaddr[6];
}RlPrReqCnt;

typedef struct RlPrRepCnt {
	bool success;
	char toaddr[6];
	long heartbeat;
}RlPrRepCnt;

typedef struct MessageHdr {
	enum MsgTypes msgType;
	char fromaddr[6];
	size_t msgsize;
}MessageHdr;


/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];
	void addToGroup(char addr[6], long heartbeat);

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
