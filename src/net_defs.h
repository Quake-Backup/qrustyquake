// Copyright (C) 1996-1997  Id Software, Inc.
// Copyright (C) 2005-2012  O.Sezer <sezero@users.sourceforge.net>
// GPLv3 See LICENSE for details.

#ifndef __NET_DEFS_H
#define __NET_DEFS_H

struct qsockaddr
{
	short qsa_family;
	unsigned char qsa_data[14];
};

/**

This is the network info/connection protocol.  It is used to find Quake
servers, get info about them, and connect to them.  Once connected, the
Quake game protocol (documented elsewhere) is used.


General notes:
	game_name is currently always "QUAKE", but is there so this same protocol
		can be used for future games as well; can you say Quake2?

CCREQ_CONNECT
		string	game_name		"QUAKE"
		byte	net_protocol_version	NET_PROTOCOL_VERSION

CCREQ_SERVER_INFO
		string	game_name		"QUAKE"
		byte	net_protocol_version	NET_PROTOCOL_VERSION

CCREQ_PLAYER_INFO
		byte	player_number

CCREQ_RULE_INFO
		string	rule

CCREP_ACCEPT
		long	port

CCREP_REJECT
		string	reason

CCREP_SERVER_INFO
		string	server_address
		string	host_name
		string	level_name
		byte	current_players
		byte	max_players
		byte	protocol_version	NET_PROTOCOL_VERSION

CCREP_PLAYER_INFO
		byte	player_number
		string	name
		long	colors
		long	frags
		long	connect_time
		string	address

CCREP_RULE_INFO
		string	rule
		string	value

	note:
		There are two address forms used above.  The short form is just a
		port number.  The address that goes along with the port is defined as
		"whatever address you receive this reponse from".  This lets us use
		the host OS to solve the problem of multiple host addresses (possibly
		with no routing between them); the host will use the right address
		when we reply to the inbound connection request.  The long from is
		a full address and port in a string.  It is used for returning the
		address of a server that is not running locally.

**/


typedef struct qsocket_s
{
	struct qsocket_s	*next;
	double		connecttime;
	double		lastMessageTime;
	double		lastSendTime;

	qboolean	disconnected;
	qboolean	canSend;
	qboolean	sendNext;

	int		driver;
	int		landriver;
	sys_socket_t	socket;
	void		*driverdata;

	unsigned int	ackSequence;
	unsigned int	sendSequence;
	unsigned int	unreliableSendSequence;
	int		sendMessageLength;
	byte		sendMessage [NET_MAXMESSAGE];

	unsigned int	receiveSequence;
	unsigned int	unreliableReceiveSequence;
	int		receiveMessageLength;
	byte		receiveMessage [NET_MAXMESSAGE];

	struct qsockaddr	addr;
	char		address[NET_NAMELEN];

} qsocket_t;

extern qsocket_t	*net_activeSockets;
extern qsocket_t	*net_freeSockets;
extern int		net_numsockets;

typedef struct
{
	const char	*name;
	qboolean	initialized;
	sys_socket_t	controlSock;
	sys_socket_t	(*Init) ();
	void		(*Shutdown) ();
	void		(*Listen) (qboolean state);
	sys_socket_t	(*Open_Socket) (int port);
	int		(*Close_Socket) (sys_socket_t socketid);
	int		(*Connect) (sys_socket_t socketid, struct qsockaddr *addr);
	sys_socket_t	(*CheckNewConnections) ();
	int		(*Read) (sys_socket_t socketid, byte *buf, int len, struct qsockaddr *addr);
	int		(*Write) (sys_socket_t socketid, byte *buf, int len, struct qsockaddr *addr);
	int		(*Broadcast) (sys_socket_t socketid, byte *buf, int len);
	const char *	(*AddrToString) (struct qsockaddr *addr);
	int		(*StringToAddr) (const char *string, struct qsockaddr *addr);
	int		(*GetSocketAddr) (sys_socket_t socketid, struct qsockaddr *addr);
	int		(*GetNameFromAddr) (struct qsockaddr *addr, char *name);
	int		(*GetAddrFromName) (const char *name, struct qsockaddr *addr);
	int		(*AddrCompare) (struct qsockaddr *addr1, struct qsockaddr *addr2);
	int		(*GetSocketPort) (struct qsockaddr *addr);
	int		(*SetSocketPort) (struct qsockaddr *addr, int port);
} net_landriver_t;

extern net_landriver_t	net_landrivers[];
extern const int	net_numlandrivers;

typedef struct
{
	const char	*name;
	qboolean	initialized;
	int		(*Init) ();
	void		(*Listen) (qboolean state);
	void		(*SearchForHosts) (qboolean xmit);
	qsocket_t	*(*Connect) (const char *host);
	qsocket_t	*(*CheckNewConnections) ();
	int		(*QGetMessage) (qsocket_t *sock);
	int		(*QSendMessage) (qsocket_t *sock, sizebuf_t *data);
	int		(*SendUnreliableMessage) (qsocket_t *sock, sizebuf_t *data);
	qboolean	(*CanSendMessage) (qsocket_t *sock);
	qboolean	(*CanSendUnreliableMessage) (qsocket_t *sock);
	void		(*Close) (qsocket_t *sock);
	void		(*Shutdown) ();
} net_driver_t;

extern net_driver_t	net_drivers[];
extern const int	net_numdrivers;



extern int		net_driverlevel;

extern int		messagesSent;
extern int		messagesReceived;
extern int		unreliableMessagesSent;
extern int		unreliableMessagesReceived;

qsocket_t *NET_NewQSocket ();
void NET_FreeQSocket(qsocket_t *);
double SetNetTime();



typedef struct
{
	char	name[16];
	char	map[16];
	char	cname[32];
	int		users;
	int		maxusers;
	int		driver;
	int		ldriver;
	struct qsockaddr addr;
} hostcache_t;

extern int hostCacheCount;
extern hostcache_t hostcache[HOSTCACHESIZE];


typedef struct _PollProcedure
{
	struct _PollProcedure	*next;
	double				nextTime;
	void				(*procedure)(void *);
	void				*arg;
} PollProcedure;

void SchedulePollProcedure(PollProcedure *pp, double timeOffset);

#endif
