
#ifndef WSOCKSIMPL_H
#define WSOCKSIMPL_H

// Legacy defines
#define WS_INIT 0
#define WS_OPEN 1
#define WS_CLOSE 2

#ifdef WIN32
#define WS_ENCRYPTED
// Define to use a critical section for lockMux instead of a mutex
#define MUX_CRIT_SECTION
#define OVLMUX_CRIT_SECTION
#define LBAMUX_CRIT_SECTION
#else
// No 64-bit DES3 source code available
#undef WS_ENCRYPTED
#endif
#define WS_COMPRESS
#define WS_MAX_BLOCK_SIZE 128
#define WS_MONITOR
#ifdef WS_FILE_SERVER
#define WS_FILE_TIMEOUT 6000
#define FILE_XFERS_PER_TICK 32
#endif
#define WS_CAST
#define WS_OTHER
#define WS_CONOUT_RECON 5000
#undef WS_SENDS_PER_BEAT //Retired
#undef WS_MIPS
#define WS_WINMM
#define MAX_PENDING_CONNECTS 4
#define WS_PRE_TICK
#define WS_LOOPBACK
#define WS_DECLINING_RECONNECT
#define WS_REALTIMESEND
#define WS_FILE_SMPROXY

// Actual milliseconds instead of legacy send count
#define WS_CON_TIMEOUT 60000
#define WS_USR_TIMEOUT 60000
#define WS_UMR_TIMEOUT 5000
#define WS_CTO_TIMEOUT 5000
#define WS_CTI_PREFIX_PACKET_LEN

#undef WS_LOW_LATENCY
// When defined, we get sub-millisecond latency, but the trade-off is high CPU utilization all the time.
// Without, we get 4 ms round trip (without loopback shortcut) within same app.
#ifdef WS_LOW_LATENCY
#define WS_TIMER_INTERVAL	0
#else
#define WS_TIMER_INTERVAL	1
#endif
#define WAITABLE_TIMERS

extern void WSLogEvent(const char *Event, ... );
extern void WSLogError (const char *format, ... );

#if defined(WIN32)&&!defined(BIT64)
	#define ws_fd_set fd_set
	#define WS_FD_ZERO FD_ZERO
	#define WS_FD_SET FD_SET
	#define WS_FD_ISSET FD_ISSET
#else//!WIN32
	typedef struct ws_fd_set {
			u_int fd_count;               /* how many are SET? */
			SOCKET_T  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
	} ws_fd_set;
	#define WS_FD_SET(fd, set) do { \
		if (((ws_fd_set FAR *)(set))->fd_count < FD_SETSIZE) \
			((ws_fd_set FAR *)(set))->fd_array[((ws_fd_set FAR *)(set))->fd_count++]=(fd);\
	} while(0)

	#define WS_FD_ZERO(set) (((ws_fd_set FAR *)(set))->fd_count=0)

	#define WS_FD_ISSET(fd, set) __WSFDIsSet((SOCKET_T)(fd), (fd_set FAR *)(set))
#endif//WIN32

#ifndef FIONBIO
#include <asm/ioctls.h>
#endif

#pragma pack(push,1)

// Flow control buffer
class WSBuffer
{
public:
	WSBuffer();
	~WSBuffer();

	virtual int WSOpenBuff(int blksize);
	virtual int WSWriteBuff(char *wbuf, int wlen);
	virtual int WSStripBuff(int wlen);
	virtual int WSCloseBuff();

protected:
	BUFFER buffer;
};

// WSFileTransfer.status codes
// Length of time to wait for commit ack
#define COMMIT_TIMEOUT 300
#define GENERIC_APPEND (0x01000000L)
// File transfer progress dialog controls
#define WS_LISTBOX			(pmod->WS_USER+1)
#define WS_LB_CONLIST		(pmod->WS_USER+2)
#define WS_IDC_PORTNO		(pmod->WS_USER+3)
#define WS_XFER_STATUS      (pmod->WS_USER+4)
#define WS_IDC_SESSIONID    (pmod->WS_USER+5)
#define WS_IDC_GDCFG		(pmod->WS_USER+6)
#define WS_IDC_EDIT		    (pmod->WS_USER+7)
#define WS_IDC_IP			(pmod->WS_USER+8)

#define IDC_MSG             (pmod->WS_USER+2910)
#define IDC_PERCENT         (pmod->WS_USER+292)
#define IDC_PROGRESS        (pmod->WS_USER+293)

#define STATUS_UPDATE 1000
#define WS_LOG_WIDTH 90
#define WS_DEF_BLOCK_SIZE (16)
#define WS_COMP_BLOCK_LEN (15)
#if WS_DEF_BLOCK_SIZE<2
#error WS_MAX_BLOCK_SIZE must be >= 2!
#endif
#define WS_DEF_RECV_BUFF_LIMIT (200*1024) // #Kilobytes default 200MB
#define WS_DEF_SEND_BUFF_LIMIT (1024*1024) // #Kilobytes default 1GB

#define NO_FLAGS_SET         0
#ifndef MIN_RECONNECT_DELAY
#define MIN_RECONNECT_DELAY 100
#endif
#ifndef MAX_RECONNECT_DELAY
#define MAX_RECONNECT_DELAY 5000
#endif
#ifndef MIN_RECONNECT_RESET
#define MIN_RECONNECT_RESET 30000
#endif

// For multi-thread exclusion, restricting max concurrent overlapped operations per socket to 1
// maintains socket mutual exclusion in the app threads.
#define WS_OVERLAP_MAX 1

#ifdef WS_GUARANTEED
typedef struct tdGDHeader
{
	unsigned short Type; //see below
    unsigned short SourceId;
    unsigned short GDLineId;
    unsigned short Reserved;
	unsigned long GDLogId;
}GDHEADER;

#define GD_MSG_SEND     1024
#define GD_TYPE_DATA    1
#define GD_TYPE_LOGIN   3
#define GD_TYPE_SYNC    4
#define GD_TYPE_NOGD    5
#define GD_TYPE_GAP     6
#endif//WS_GUARANTEED



// Interface for generic data transformation and handling
class WSCodec
{
public:
	virtual int Init()=0;
	virtual int Encode(char *ebuf, int *elen, const char *dbuf, int dlen, void *context)=0;
	virtual int Decode(char *dbuf, int *dlen, const char *ebuf, int elen, void *context)=0;
	virtual int Cleanup()=0;

protected:
	void *context;
};
typedef list<WSCodec *> WSCODECLIST;

// Connection negotiation. New connections are validated (e.g. ipfilter and handshake) 
// by the connector class before being passed to the app.
class WSPort;
enum WSConnectionState
{
	WSCS_UNKNOWN,
	WSCS_CONNECTING,
	WSCS_CLIENT_NEGOTIATE,
	WSCS_CONNECTED,
	WSCS_ACCEPTING,
	WSCS_SERVER_NEGOTIATE,
	WSCS_ACCEPTED,
	WSCS_COUNT,
};
class WSConnector
{
public:
	// Notifications
	virtual int WSAccepting(WSPort *lport, WSPort *aport, void *context)=0;
	virtual int WSConnecting(WSPort *cport, void *context)=0;

protected:
	WSConnectionState state;
	void *context;
};
typedef list<WSConnector *> WSCONNECTORLIST;

// Versioning
enum WSPortVersion
{
	WS_VERS_UNKNOWN=0,
	WS_VERS_1_0=0x0100,
	WS_VERS_COUNT
};
typedef set<WSPort*> WSPORTSET;
class WSPort
{
public:
	WSPort()
		:PortName()
		,pmod(0)
		,PortType(WS_UNKNOWN)
		,PortNo(-1)
		,PortVers(WS_VERS_UNKNOWN)
		,codecList()
		,connectorList()
		,sd(INVALID_SOCKET)
		,lbaKey()
		,lbaPeer(0)
		,lbaConnReset(false)
		,lbaBuffer(0)
		,lbaPending()
		#ifdef WSOCKS_SSL
		,pSSLInfo(0)
		#endif
	{
//		memset(&oneport,0,sizeof(oneport));
	}
	inline void SetName()
	{
		char pname[16]={0};
		switch(PortType)
		{
		case WS_CON: sprintf(pname,"CON%d",PortNo); break;
		case WS_CON_TOT: sprintf(pname,"CON_TOT"); break;
		case WS_USC: sprintf(pname,"USC%d",PortNo); break;
		case WS_USR: sprintf(pname,"USR%d",PortNo); break;
		case WS_USR_TOT: sprintf(pname,"USR_TOT"); break;
		case WS_FIL: sprintf(pname,"FIL%d",PortNo); break;
		case WS_FIL_TOT: sprintf(pname,"FIL_TOT"); break;
		case WS_UMC: sprintf(pname,"UMC%d",PortNo); break;
		case WS_UMR: sprintf(pname,"UMR%d",PortNo); break;
		case WS_CGD: sprintf(pname,"CGD%d",PortNo); break;
		case WS_CGD_TOT: sprintf(pname,"CGD_TOT"); break;
		case WS_UGC: sprintf(pname,"UGC%d",PortNo); break;
		case WS_UGR: sprintf(pname,"UGR%d",PortNo); break;
		case WS_UGR_TOT: sprintf(pname,"UGR_TOT"); break;
		case WS_CTO: sprintf(pname,"CTO%d",PortNo); break;
		case WS_CTO_TOT: sprintf(pname,"CTO_TOT"); break;
		case WS_CTI: sprintf(pname,"CTI%d",PortNo); break;
		case WS_CTI_TOT: sprintf(pname,"CTI_TOT"); break;
		case WS_OTH: sprintf(pname,"OTH%d",PortNo); break;
		case WS_BRT: sprintf(pname,"BRT%d",PortNo); break;
		case WS_BRT_TOT: sprintf(pname,"BRT_TOT"); break;
		case WS_BRR: sprintf(pname,"BRR%d",PortNo); break;
		case WS_BRR_TOT: sprintf(pname,"BRR_TOT"); break;
		default: sprintf(pname,"UNK%d",PortNo);
		};
		PortName=pname;
	}

	string PortName;
	WsocksApp *pmod;
	WSPortType PortType;
	int PortNo;
	WSPortVersion PortVers;
	WSCODECLIST codecList;
	WSCONNECTORLIST connectorList;
	SOCKET sd;

	string lbaKey;
	WSPORTSET lbaPending;
	WSPort *lbaPeer;
	bool lbaConnReset;
	BUFFER *lbaBuffer;
	SOCKADDR_IN caddr;
	#ifdef WSOCKS_SSL
	struct SSLInfo *pSSLInfo;
	#endif
};

// Compression
class ZlibCompress :public WSCodec
{
public:
	ZlibCompress();

	virtual int Init();
	virtual int Encode(char *ebuf, int *elen, const char *dbuf, int dlen);
	virtual int Decode(char *dbuf, int *dlen, const char *ebuf, int elen);
	virtual int Cleanup();

protected:
	WSBuffer InCompBuffer;
	WSBuffer OutCompBuffer;
};

// Encryption
class Des3Encrypt :public WSCodec
{
public:
	Des3Encrypt();

	virtual int Init();
	virtual int Encode(char *ebuf, int *elen, const char *dbuf, int dlen);
	virtual int Decode(char *dbuf, int *dlen, const char *ebuf, int elen);
	virtual int Cleanup();

protected:
	WSBuffer InCompBuffer;
	WSBuffer OutCompBuffer;
};

//// Buffers asynchronous notifications for synchronous notifications
//class WSSyncBuffer :public WSCodec
//{
//public:
//	WSSyncBuffer();
//
//	virtual int Init();
//	virtual int Encode(char *ebuf, int *elen, const char *dbuf, int dlen);
//	virtual int Decode(char *dbuf, int *dlen, const char *ebuf, int elen);
//	virtual int Cleanup();
//
//protected:
//};
//
// Raw recording capability
class WSRecorder :public WSCodec
{
public:
	WSRecorder();

	virtual int Init();
	virtual int Encode(char *ebuf, int *elen, const char *dbuf, int dlen);
	virtual int Decode(char *dbuf, int *dlen, const char *ebuf, int elen);
	virtual int Cleanup();

protected:
};

// Bounces connections that don't have activity
class WSPortBouncer
{
public:
	WSPortBouncer();

protected:
};

struct WSALTDEST
{
	char IP[20];
	int Port;
};
typedef list<WSALTDEST> ALTDESTLIST;

// Socks5 proxy protocol
class Socks5 :public WSConnector
{
public:
	Socks5();
	virtual int WSAccepting(WSPort *lport, WSPort *aport, void *context);
	virtual int WSConnecting(WSPort *cport, void *context);

protected:
};

// IP filtering
typedef list<IPACL> IPACLLIST;
class IpFilter :public WSConnector
{
public:
	IpFilter();
	virtual int WSAccepting(WSPort *lport, WSPort *aport, void *context);
	virtual int WSConnecting(WSPort *cport, void *context);

protected:
	IPACLLIST aclList;
};

// IQ handshake protocol. Exchanges version, compression, encryption, features, etc.
class IQHandshake :public WSConnector
{
public:
	IQHandshake();
	virtual int WSAccepting(WSPort *lport, WSPort *aport, void *context);
	virtual int WSConnecting(WSPort *cport, void *context);

protected:
};

#define WSTEMPFILESDIR "SysmonTempFiles"
typedef WSFileTransfer_2_0 WSTempFile;
typedef map<string,WSTempFile *> WSFILEMAP;
typedef map<string,WSTempFile *> WSFPMAP;

#ifdef WS_FILE_SMPROXY
struct WSDecodeHint
{
	WSPortType PortType;
	int PortNo;
	WsocksApp *pmod;
};

struct WSFileTransfer2
{
	class WsocksApp *pmod;
	int PortNo;
	bool download;
	string localFile;
	string remoteFile;
    TransferStatus status;              // WSF_xxx status
	HANDLE xthnd;						// File transfer thread handle
	DWORD xtid;							// File transfer thread id
	HANDLE doneEvent;
};
#endif

#pragma pack(pop)

#endif//WSOCKSIMPL_H
