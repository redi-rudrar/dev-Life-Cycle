// wsocksapi.h : C++ wsocks interface
//
#ifndef _WSOCKSAPI_H
#define _WSOCKSAPI_H
//
// THE INFORMATION CONTAINED HEREIN IS PROVIDED <93>AS IS<94> AND NO PERSON OR ENTITY ASSOCIATED WITH THE
// INSTAQUOTE MARKET DATA API MAKES ANY REPRESENTATION OR WARRANTY, EXPRESS OR IMPLIED, AS TO THE
// INSTAQUOTE MARKET DATA API (OR THE RESULTS TO BE OBTAINED BY THE USE THEREOF) OR ANY OTHER MATTER
// AND EACH SUCH PERSON AND ENTITY SPECIFICALLY DISCLAIMS ANY WARRANTY OF ORIGINALITY, ACCURACY,
// COMPLETENESS, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  SUCH PERSONS AND ENTITIES
// DO NOT WARRANT THAT THE INSTAQUOTE MARKET DATA API WILL CONFORM TO ANY DESCRIPTION THEREOF OR
// BE FREE OF ERRORS.  THE ENTIRE RISK OF ANY USE OF THE INSTAQUOTE MARKET DATA API IS ASSUMED BY THE USER.
//
// NO PERSON OR ENTITY ASSOCIATED WITH THE INSTAQUOTE MARKET DATA API SHALL HAVE ANY LIABILITY FOR
// DAMAGE OF ANY KIND ARISING IN ANY MANNER OUT OF OR IN CONNECTION WITH ANY USER<92>S USE OF (OR ANY
// INABILITY TO USE) THE INSTAQUOTE MARKET DATA API, WHETHER DIRECT, INDIRECT, INCIDENTAL, SPECIAL
// OR CONSEQUENTIAL (INCLUDING, WITHOUT LIMITATION, LOSS OF DATA, LOSS OF USE, CLAIMS OF THIRD PARTIES
// OR LOST PROFITS OR REVENUES OR OTHER ECONOMIC LOSS), WHETHER IN TORT (INCLUDING NEGLIGENCE AND STRICT
// LIABILITY), CONTRACT OR OTHERWISE, WHETHER OR NOT ANY SUCH PERSON OR ENTITY HAS BEEN ADVISED OF,
// OR OTHERWISE MIGHT HAVE ANTICIPATED THE POSSIBILITY OF, SUCH DAMAGES.
//
// No proprietary or ownership interest of any kind is granted with respect to the INSTAQUOTE MARKET DATA API
// (or any rights therein).
//
// Copyright © 2009 Banc of America Securities, LLC. All rights reserved.
//
#ifdef __cplusplus

#ifdef WIN32
#include <winsock2.h>
#ifdef MAKE_DLL
	#ifdef MAKE_WSOCKSAPI
	#define WSAPILINK __declspec(dllexport)
	#else
	#define WSAPILINK __declspec(dllimport)
	#endif
#else
	#ifndef WSAPILINK
	#define WSAPILINK
	#endif
#endif
#else
#include "stdafx.h"
#endif

#include <string>
#include <list>
#include <map>
#include <set>
using namespace std; 
#include "wsbuffer.h"

// Compile settings
#define WSOCKSAPI
#define WS_MAX_BLOCK_SIZE 128
#define WS_MAX_ALT_PORTS 20
#define PROXYCOMPRESS_MAX 8
#ifdef WIN32
#define WS_FILE_SERVER //Only on windows for now
#else
#undef WS_FILE_SERVER
#endif
#define WS_GUARANTEED //Not fully implemented yet
#undef WS_OIO // I/O completion thread pool
#define WS_OTPP // one-thread per port
#define WS_OTMP // one-thread many ports
// else async thread pool (not fully implemented yet)
typedef unsigned int DES_KS[16][2];
#define USER_TIMER_ID 2
#if defined(WIN32)&&!defined(BIT64)
#define WSOCKS_SSL
#endif

// Port types
enum WSPortType
{
	WS_UNKNOWN=-1,
	WS_CON=0,
	WS_USR=1,
	WS_OTH=2,
	WS_CON_TOT=3,
	WS_USR_TOT=4,
	WS_CTO=5,
	WS_CTO_TOT=6,
	WS_CTI=7,
	WS_CTI_TOT=8,
	WS_USC=9,
	WS_UMC=10,
	WS_UMR=11,
	WS_UGR_TOT=12,
	WS_UGR=13,
	WS_UGC=14,
	WS_CGD_TOT=15,
	WS_CGD=16,
	WS_FIL=17,
	WS_FIL_TOT=18,
	WS_BRT=19,
	WS_BRT_TOT=20,
	WS_BRR=21,
	WS_BRR_TOT=22,
	WS_TYPE_COUNT
};

#pragma pack(push,8)

// Port raw data recording
typedef struct tdRecording
{
	char RecordPath[MAX_PATH];
	BOOL DoRec;
	FILE *RecOutHnd;
	FILE *RecInHnd;
} WSRECORDING;

// Port IP access control list
typedef struct tdIPAcl
{
    char Ip[20];
    char Mask[20];
    unsigned int lIp;
    unsigned int lMask;
	char action;
    struct tdIPAcl *NextIPAcl;
} IPACL;

#if defined(WS_OIO)||defined(WS_OTPP)
// Socket size
#ifdef BIT64
typedef void *SOCKET_T; // 64-bit on Windows or Linux
#elif defined(WIN32)
typedef SOCKET SOCKET_T; // 32-bit on Windows
#elif defined(_CONSOLE)
typedef ULONGLONG SOCKET_T; // 64-bit Linux compatibility on Windows
#else
typedef void *SOCKET_T; // 32-bit on Linux
#endif

#ifdef WIN32
#define INVALID_SOCKET_T (SOCKET_T)(-1)
#else
#define INVALID_SOCKET_T (SOCKET_T)(0)
#endif

#ifndef PTRCAST
#if defined(BIT64)||defined(_CONSOLE)||defined(WIN32)
#define PTRCAST ULONGLONG
#else
#define PTRCAST ULONG
#endif
#endif

#ifdef WIN32
// Overlapped I/O completion notification
typedef struct tdOvlBuf :public OVERLAPPED
{
	int PortType;
	int PortNo;
	int byteCount;
	bool Pending;
	bool Cancelled;
	bool RecvOp;
	WSABUF wsabuf;
	char *buf;
	DWORD bytes;
	DWORD flags;
	tdOvlBuf *prev;
	tdOvlBuf *next;
}WSOVERLAPPED;
#else
// Synchronous I/O only on Linux
typedef struct tdOvlBuf
{
	int PortType;
	int PortNo;
	int byteCount;
	bool Pending;
	bool Cancelled;
	bool RecvOp;
	//WSABUF wsabuf;
	char *buf;
	DWORD bytes;
	DWORD flags;
	tdOvlBuf *prev;
	tdOvlBuf *next;
}WSOVERLAPPED;
#endif
#endif

typedef struct tdConPort
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int CompressType;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[80];    
	DWORD Bounce; //Bounce Port if No Data in X milliseconds
	int BounceStart; //Dont Bounce Port before;
	int BounceEnd; //Dont Bounce Port after;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;	// 0	= offline
					// 10	= Login into Proxy
					// 20	= Connecting to Remote
					// 30	= Binding to Remote
					// 100+	= Online ready for data
	char OnLineStatusText[80];

	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
}CONPORT_1_0;

// Version 1.1 features:
// 1) Multi-threading
// 2) Low latency from immediate send threshold
// 3) Overlapped I/O with completion ports
struct CONPORT_1_1
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int CompressType;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[80];    
	DWORD Bounce;
	long BounceStart;
	long BounceEnd;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	HANDLE rmutex; // New for this version
	HANDLE smutex; // New for this version
	DWORD rmutcnt; // New for this version
	DWORD smutcnt; // New for this version
	int recvThread; // New for this version
	int sendThread; // New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	#ifndef WIN32
	void *sDetPtr;
	#endif
	BOOL DoRecOnOpen;		// Allow recording as soon as the port connects
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;
	char OnLineStatusText[80];

	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	DWORD lastReadyStart; // New for this version
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

// Version 1.2 features:
// 1) 256-byte CfgNote and Note
struct CONPORT_1_2
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int CompressType;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[256];    
	DWORD Bounce;
	long BounceStart;
	long BounceEnd;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	HANDLE rmutex; // New for this version
	HANDLE smutex; // New for this version
	DWORD rmutcnt; // New for this version
	DWORD smutcnt; // New for this version
	int recvThread; // New for this version
	int sendThread; // New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	#ifndef WIN32
	void *sDetPtr;
	#endif
	BOOL DoRecOnOpen;		// Allow recording as soon as the port connects
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;
	char OnLineStatusText[80];

	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[256];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	DWORD lastReadyStart; // New for this version
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

// Version 1.3 features:
// 1) ShellPort and ShellDetPtr to allow app to use 3rd party connection, but display as CON port
struct CONPORT_1_3
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	BOOL AltRemoteIPOn[WS_MAX_ALT_PORTS];
	u_short AltRemotePort[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int CompressType;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[256];    
	DWORD Bounce;
	//REDICP - 1917
	LONG_PTR BounceStart;
	LONG_PTR BounceEnd;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	HANDLE rmutex; // New for this version
	HANDLE smutex; // New for this version
	DWORD rmutcnt; // New for this version
	DWORD smutcnt; // New for this version
	int recvThread; // New for this version
	int sendThread; // New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	#ifndef WIN32
	void *sDetPtr;
	#endif
	BOOL DoRecOnOpen;		// Allow recording as soon as the port connects
	BOOL ShellPort;
	void *ShellDetPtr;
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;
	char OnLineStatusText[80];

	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[256];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	DWORD lastReadyStart; // New for this version
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

struct CONPORT_2_0
{
public:
	// These attributes persist across connections
	// Status
	int InUse;				//Enable
	int ConnectHold;		//Auto-connect suspended
	// Where
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	int AltIPCount;
	int CurrentAltIP;		//Current AltRemoteIP used
	u_short Port;
	// Encryption
	int Compressed;			//Compressed flag
	int CompressType;		//Compression method: 0-zlib, 1-feedprox
	int Encrypted;			//Encryption method: 1-des3
	// Socks 5 proxy
	char S5RemoteIP[20];	//Proxy IP
	int S5Port;				//Proxy port
	int S5Connect;			//Proxy protocol
	int S5Methode;			//Proxy encryption method
	int S5Version;			//Proxy version
	// Declinging reconnect
	DWORD MinReconnectDelay;	//Minimum wait between reconnects (100 ms)
	DWORD MaxReconnectDelay;	//Maximum wait between reconnects (5 sec)
	DWORD MinReconnectReset;	//Time after which reconnect delay resets to MinReconnectDelay (30 sec)
	// Buffering
	int BlockSize;			//Default 16, max 128 (Kbytes)
    int ChokeSize;
	char CfgNote[80];
	DWORD Bounce;			//Bounce Port if No Data in X milliseconds
	long BounceStart;		//Dont Bounce Port before;
	long BounceEnd;			//Dont Bounce Port after;
	// Latency
	int ImmediateSendLimit;	//Sends when this number of outgoing bytes buffered
	BOOL DoRecOnOpen;		// Allow recording as soon as the port connects

	// These are cleared between connections
	// Status
	char SockOpen;			//Connect in progress or connected
	char SockConnected;		//Connection established
	char Note[80];			//Note text
	char OnLineStatusText[80]; //Status text
	char Uuid[16];			//Unique connection ID
	// Where
	char LocalIP[20];		//Current bind interface
	char RemoteIP[20];		//Current destination
	// Buffering
	BUFFER InBuffer;		//Incoming buffer
	BUFFER OutBuffer;		//Outgoing buffer
	// App storage
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	// Implementation
	struct CONIMPL_2_0 *pimpl;
};

typedef struct tdUscPort
{
	char LocalIP[20];
	u_short Port;
    SOCKET_T Sock;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int CompressType;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
    char IPAclCfg[128];
    IPACL *IPAclList;
	char MonitorGroups[8][16];
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
	HWND setupDlg;
	char Status[80];
	BOOL CfgNoteDirty;
}USCPORT_1_0;

struct USCPORT_1_1
{
	char LocalIP[20];
	u_short Port;
	u_short bindPort; //New for this version
	char bindIP[20]; //New for this version
    SOCKET_T Sock;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int CompressType;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
    char IPAclCfg[128];
    IPACL *IPAclList;
	char MonitorGroups[8][16];
	HANDLE tmutex; //New for this version
	int activeThread; //New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	bool appClosed; // New for this version
	#ifdef WS_OIO
	bool pendingClose; // New for this version
	#endif
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
	HWND setupDlg;
	char Status[80];
	BOOL CfgNoteDirty;
};

struct USCPORT_1_2
{
	char LocalIP[20];
	u_short Port;
	u_short bindPort; //New for this version
	char bindIP[20]; //New for this version
    SOCKET_T Sock;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[256];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int CompressType;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
    char IPAclCfg[128];
    IPACL *IPAclList;
	char MonitorGroups[8][16];
	HANDLE tmutex; //New for this version
	int activeThread; //New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	bool appClosed; // New for this version
	#ifdef WS_OIO
	bool pendingClose; // New for this version
	#endif
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
	HWND setupDlg;
	char Status[80];
	BOOL CfgNoteDirty;
};

typedef struct tdUsrPort
{
	char SockActive;
	char RemoteIP[20];
	char LocalIP[20];
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	int UscPort;
	SOCKET_T Sock;
	unsigned long Seconds;
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long SinceOpenTimer;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	WSRECORDING Recording;
	HWND setupDlg;
	unsigned long TimeTillClose;
	time_t TimeOutStart;
}USRPORT_1_0;

struct USRPORT_1_1 :public USRPORT_1_0
{
	// All below are new for this version
	HANDLE rmutex;
	HANDLE smutex;
	DWORD rmutcnt;
	DWORD smutcnt;
	int recvThread;
	int sendThread;
	DWORD lastReadyStart;
	int ImmediateSendLimit;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes;
	bool peerClosed;
	bool appClosed;
	bool pendingClose;
	WSOVERLAPPED *pendOvlRecvList;
	WSOVERLAPPED *pendOvlSendList;
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread;
	#else
	pthread_t pthread;
	#endif
	int TBufferSize;
	bool peerClosed;
	bool appClosed;
	WSOVERLAPPED *pendOvlSendBeg;
	WSOVERLAPPED *pendOvlSendEnd;
	unsigned long IOCPSendBytes;
	int NumOvlSends;
	#endif
};

struct USRPORT_2_0 :public USRPORT_1_1
{
	char ClientIP[20];
	int ClientPort;
};

struct USRPORT_2_1
{
	char SockActive;
	char RemoteIP[20];
	char LocalIP[20];
	char Note[256];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	int UscPort;
	SOCKET_T Sock;
	unsigned long Seconds;
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long SinceOpenTimer;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	WSRECORDING Recording;
	HWND setupDlg;
	unsigned long TimeTillClose;
	time_t TimeOutStart;
	// 1.1
	HANDLE rmutex;
	HANDLE smutex;
	DWORD rmutcnt;
	DWORD smutcnt;
	int recvThread;
	int sendThread;
	DWORD lastReadyStart;
	int ImmediateSendLimit;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes;
	bool peerClosed;
	bool appClosed;
	bool pendingClose;
	WSOVERLAPPED *pendOvlRecvList;
	WSOVERLAPPED *pendOvlSendList;
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread;
	#else
	pthread_t pthread;
	#endif
	int TBufferSize;
	bool peerClosed;
	bool appClosed;
	WSOVERLAPPED *pendOvlSendBeg;
	WSOVERLAPPED *pendOvlSendEnd;
	unsigned long IOCPSendBytes;
	int NumOvlSends;
	#endif
	// 2.0
	char ClientIP[20];
	int ClientPort;
};

typedef struct tdFileOptions
{
	int PortNo;                         // F-port
	int xfersPerTick;					// Number of 1k packets per tick
	int commitTimeout;					// Milliseconds to wait for commit ack
	int waitTimeout;					// Milliseconds to wait for transfer
	BOOL autoCloseProgressDialog;       // Close the progress dialog after xfer

	//BOOL useLocalFirst;                 // Open persistent files first
	//BOOL useFileServer;                 // Use the file server on F ports
	//BOOL useWindowsNetwork;             // Use Windows Networking last
	BOOL localMaster;                   // The local copy is the master

	BOOL keepLocal;                     // Keep a persistent copy
	BOOL cacheLocal;                    // Keep a cache copy
	BOOL guaranteeCommit;               // Guarantee commit for the app
} FileOptions;

// WSFileTransfer.status codes
typedef enum tdTransferStatus
{
	WSF_INIT=0,
    WSF_REQUESTED,
    WSF_RECV_INPROGRESS,
    WSF_WAIT_COMMIT,
    WSF_COMMIT_INPROGRESS,
    WSF_WAIT_COMMIT_ACK,
    WSF_SUCCESS,
    WSF_FAILED,
    WSF_CANCELLED,
    WSF_TIMEOUT,
} TransferStatus;

typedef enum tdFileTransferAPI
{
    FTAPI_UNKNOWN,
    FTAPI_CREATEFILE,
    FTAPI_FOPEN,
    FTAPI_OPENFILE,
    FTAPI_FINDFIRSTFILE,
    FTAPI_PATHFILEEXISTS,
    FTAPI_PATHISDIRECTORY,
    FTAPI_CREATEDIRECTORY,
    FTAPI_TRANSMITFILE,
	FTAPI_GETFILE,
	FTAPI_PUTFILE,
} FileTransferAPI;

typedef struct tdFileTransfer
{
    struct tdFileTransfer *next;
    char remotePath[MAX_PATH];          // Virtual file path
    char tempPath[MAX_PATH];            // Temp or commit file path
    char localPath[MAX_PATH];           // Cache or keep file path
    DWORD dwDesiredAccess;              // Createfile parameters
    DWORD dwShareMode;
#ifdef WIN32
    SECURITY_ATTRIBUTES SecurityAttributes;
#endif
    DWORD dwCreationDisposition;
    DWORD dwFlagsAndAttributes;
    HANDLE hTemplateFile;
    FileTransferAPI api;                // Which API the app called
    char mode[16];                      // WSfopen mode
#ifdef WIN32
    OFSTRUCT ReOpenBuff;                // WSOpenFile struct
#endif
    UINT uStyle;                        // WSOpenFile style
	FileOptions opts;                   // File transfer options

    HANDLE thnd;                        // Temp file handle
    HANDLE fhnd;                        // App file handle
    HANDLE vhnd;                        // Remote virtual file handle
    int PortNo;                         // Transfer port
    TransferStatus status;              // WSF_xxx status
    DWORD fsize;                        // File size
    DWORD recvd;                        // Bytes received
    HWND progWnd;                       // Progress dialog window
    BOOL commitAttempted;               // Commit attempted
    DWORD sent;                         // Bytes sent
    int reqDate;                       // Date of request
    int reqTime;                       // Time of request
    DWORD waitCommitTimer;              // Commit wait tick count
#ifdef WIN32
    WIN32_FIND_DATAA fdata;              // Remote file times
#endif
    BOOL exists;                        // Remote file exists
    BOOL sftCalled;                     // WSSetFileTime was called
	BOOL uptodate;                      // The file is up-to-date with server
	BOOL gdWriteOnly;                   // Write-only w/ guaranteed commit
	HANDLE xthnd;						// File transfer thread handle
	DWORD xtid;							// File transfer thread id
	class WsocksApp *pmod;
} WSFileTransfer;

typedef struct tdFileTransfer_2_0
{
	bool download;	// Transfer direction 
	WORD proto;		// Transfer protocol
	string fkey;	// Transfer file key
	string fpath;	// Remote file path
	DWORD rfp;		// Remote file handle
	string tpath;	// Local file path
	#ifdef WIN32
	HANDLE lfp;		// Local file handle
	#else
	FILE *lfp;		// Local file handle
	#endif
	LONGLONG tsize; // Total file size
	LONGLONG begin;	// Begin offset
	LONGLONG rsize; // Requested file size
	LONGLONG off;	// Current file offset
    #ifdef WIN32
    FILETIME ftCreation;
    FILETIME ftLastAccess;
    FILETIME ftLastWrite;
    #endif
    int rid;
    WSPortType PortType;
    int PortNo;
    bool multi;
    string OnBehalfOf;
    string errmsg;
} WSFileTransfer_2_0;

typedef struct tdFileRecommit
{
	struct tdFileRecommit *next;
	char remotePath[MAX_PATH];
	char localPath[MAX_PATH];
	long lastSend;
} WSFileRecommit;

typedef struct tdFilePort
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[80];    
	DWORD Bounce; //Bounce Port if No Data in X milliseconds
	long BounceStart; //Dont Bounce Port before;
	long BounceEnd; //Dont Bounce Port after;
	int PortWasActiveBeforeSuspend;
	char DriveList[32];
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;	// 0	= offline
					// 10	= Login into Proxy
					// 20	= Connecting to Remote
					// 30	= Binding to Remote
					// 100+	= Online ready for data
	char OnLineStatusText[80];

	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	WSFileTransfer *FileTransfer;
}FILEPORT_1_0;

struct FILEPORT_1_1
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[80];    
	DWORD Bounce;
	long BounceStart;
	long BounceEnd;
	int PortWasActiveBeforeSuspend;
	char DriveList[32];
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	HANDLE rmutex; // New for this version
	HANDLE smutex; // New for this version
	DWORD rmutcnt; // New for this version
	DWORD smutcnt; // New for this version
	int recvThread; //New for this version
	int sendThread; //New for this version
	int ImmediateSendLimit; //New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	BOOL DoRecOnOpen;		// Allow recording as soon as the port connects
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;
	char OnLineStatusText[80];
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	WSFileTransfer *FileTransfer;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

typedef struct tdUmcPort
{
	char LocalIP[20];
	u_short Port;
    SOCKET_T Sock;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
	char MonitorGroups[8][16];
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	
	unsigned long Seconds;

	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
}UMCPORT_1_0;

struct UMCPORT_1_1
{
	char LocalIP[20];
	u_short Port;
	u_short bindPort; //New for this version
	char bindIP[20]; //New for this version
    SOCKET_T Sock;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
	char MonitorGroups[8][16];
	HANDLE tmutex; //New for this version
	int activeThread; //New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	bool appClosed; // New for this version
	#ifdef WS_OIO
	bool pendingClose; // New for this version
	#endif
	u_short dynPortBegin; 
	u_short dynPortEnd;
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
	char Status[80]; // New for this version
};

typedef struct tdUmrPort
{
	char SockActive;
	char RemoteIP[20];
	char LocalIP[20];
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	int UmcPort;
	SOCKET_T Sock;
	unsigned long Seconds;
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long SinceOpenTimer;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	char MonitorGroups[8][16];
	unsigned long TimeTillClose;
	time_t TimeOutStart;
	WSRECORDING Recording;
}UMRPORT_1_0;

struct UMRPORT_1_1 :public UMRPORT_1_0
{
	// All below are new for this version
	HANDLE rmutex;
	HANDLE smutex;
	DWORD rmutcnt;
	DWORD smutcnt;
	int recvThread;
	int sendThread;
	DWORD lastReadyStart;
	int ImmediateSendLimit;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes;
	bool peerClosed;
	bool appClosed;
	bool pendingClose;
	WSOVERLAPPED *pendOvlRecvList;
	WSOVERLAPPED *pendOvlSendList;
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread;
	#else
	pthread_t pthread;
	#endif
	int TBufferSize;
	bool peerClosed;
	bool appClosed;
	WSOVERLAPPED *pendOvlSendBeg;
	WSOVERLAPPED *pendOvlSendEnd;
	unsigned long IOCPSendBytes;
	int NumOvlSends;
	#endif
};

typedef struct tdGDId
{
    WORD LineId;
    char LineName[20];
    char ClientId[40];
    char SessionId[20];
} GDID;

typedef struct tdGDLogin
{
    unsigned int NextSendId;
    unsigned int LastRecvId;
} GDLOGIN;

typedef struct tdGDAcl
{
    WORD LineId;
    char LineName[20];
    char ClientId[40];
    char Ip[20];
    char Mask[20];
    unsigned int lIp;
    unsigned int lMask;
    struct tdGDAcl *NextGDAcl;
} GDACL;

typedef struct tdFileIdx
{
    unsigned long Offset;               // Offset in file
    unsigned int Length;               // Block size
}FIDX;

typedef struct tdFileIndex
{
    char fpath[MAX_PATH];               // Indexed file path
#ifdef WIN32
    HANDLE fhnd;                        // Indexed file handle if CreateFile was Used
#else
	FILE *fhnd;
#endif
    FILE *fGDLog;                       // Indexed file pointer if fopen was Used outside of Index
    unsigned long ReadIdx;              // Start block for WSReadFileBlocks
    unsigned long MinOffset;            // Minimum index offset
    unsigned long MaxOffset;            // Maximum index offset

    FIDX *Indexes;                      // Index table
    unsigned int Size;                 // Index table size
    unsigned int Count;                // Number of indexes
}FILEINDEX;

typedef struct tdGDLine
{
    WORD LineId;
    char LineName[20];
	unsigned int NextGDOutLogId;
	unsigned long NextGDOutLogOffset;
    char fGDOutLogName[256];
	FILE *fGDOutLog;
	FILEINDEX *GDOutLogFileIndex;
    struct tdGDLine *NextGDLine;
} GDLINE;

typedef struct tdCgdPort
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[80];    
    char GDCfg[128];
	DWORD Bounce; //Bounce Port if No Data in X milliseconds
	long BounceStart; //Dont Bounce Port before;
	long BounceEnd; //Dont Bounce Port after;
	int PortWasActiveBeforeSuspend;
    GDID GDId;
	unsigned int NextGDOutLogId;
	unsigned long NextGDOutLogOffset;
    char fGDOutLogName[256];
    FILE *fGDOutLog;
    FILEINDEX *GDOutLogFileIndex;
    int GDGap;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;	// 0	= offline
					// 10	= Login into Proxy
					// 20	= Connecting to Remote
					// 30	= Binding to Remote
					// 100+	= Online ready for data
	char OnLineStatusText[80];

	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	unsigned int NextGDInLogId;
	unsigned long NextGDInLogOffset;
	unsigned int LastGDSendLogId;
	char fGDInLogName[256];
	FILE *fGDInLog;
	BOOL GDLoginAck;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
}CGDPORT_1_0;

struct CGDPORT_1_1
{
	// remember to change to size of the memset in WSConClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	char S5RemoteIP[20];
	int S5Port;
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int Compressed;
	int Encrypted;
	int S5Connect;
	int S5Methode;
	int S5Version;
	int BlockSize;
    int ChokeSize;
	char CfgNote[80];    
    char GDCfg[128];
	DWORD Bounce;
	long BounceStart;
	long BounceEnd;
	int PortWasActiveBeforeSuspend;
    GDID GDId;
	unsigned int NextGDOutLogId;
	unsigned long NextGDOutLogOffset;
    char fGDOutLogName[256];
    FILE *fGDOutLog;
    FILEINDEX *GDOutLogFileIndex;
    int GDGap;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	DWORD MinReconnectDelay;
	DWORD MaxReconnectDelay;
	DWORD MinReconnectReset;
	DWORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	HANDLE rmutex; // New for this version
	HANDLE smutex; // New for this version
	DWORD rmutcnt; // New for this version
	DWORD smutcnt; // New for this version
	int recvThread; //New for this version
	int sendThread; //New for this version
	int ImmediateSendLimit; //New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	BOOL DoRecOnOpen;		// Allow recording as soon as the port connects
	// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
	char SockOpen;
	char SockConnected;
	char S5Status;
	char OnLineStatusText[80];
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	unsigned int NextGDInLogId;
	unsigned long NextGDInLogOffset;
	unsigned int LastGDSendLogId;
	char fGDInLogName[256];
	FILE *fGDInLog;
	BOOL GDLoginAck;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread;
	#else
	pthread_t pthread;
	#endif
	int TBufferSize;
	bool peerClosed;
	bool appClosed;
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

typedef struct tdUgcPort
{
	char LocalIP[20];
	u_short Port;
    SOCKET_T Sock;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
    char GDCfg[128];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
    GDACL *GDAclList;
    GDLINE *GDLines;
    int GDGap;
	char MonitorGroups[8][16];
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	
	unsigned long Seconds;

	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
    char SessionId[20];
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
}UGCPORT_1_0;

struct UGCPORT_1_1
{
	char LocalIP[20];
	u_short Port;
    SOCKET_T Sock;
	u_short bindPort; //New for this version
	char bindIP[20]; //New for this version
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
    char GDCfg[128];
	unsigned long TimeOutSize;
	unsigned short TimeOut;
	unsigned short TimeOutPeriod;
	int Compressed;
	int Encrypted;
	int PortWasActiveBeforeSuspend;
    GDACL *GDAclList;
    GDLINE *GDLines;
    int GDGap;
	char MonitorGroups[8][16];
	HANDLE tmutex; //New for this version
	int activeThread; //New for this version
	int ImmediateSendLimit; // New for this version
	int RecvBuffLimit; // New for this version
	int SendBuffLimit; // New for this version
	bool appClosed; // New for this version
	#ifdef WS_OIO
	bool pendingClose; // New for this version
	#endif
	// evert thing ,including SockActive and below,gets reset during WSUscClosePort();
	char SockActive;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long InBuffer_Size;
	unsigned long OutBuffer_Size;
    char SessionId[20];
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	WSRECORDING Recording;
	char Status[80];
};

typedef struct tdUgrPort
{
	char SockActive;
	char RemoteIP[20];
	char LocalIP[20];
	char Note[80];
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	int UgcPort;
	SOCKET_T Sock;
	unsigned long Seconds;
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long SinceOpenTimer;
    GDID GDId;
	unsigned int NextGDOutLogId;
	unsigned long NextGDInLogId;
	unsigned int NextGDInLogOffset;
    char fGDInLogName[256];
	FILE *fGDInLog;
	FILEINDEX *GDInLogFileIndex;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	BUFFER OutBuffer;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;
	WSRECORDING Recording;
	HWND setupDlg;
	unsigned long TimeTillClose;
	time_t TimeOutStart;
}UGRPORT_1_0;

struct UGRPORT_1_1: public UGRPORT_1_0
{
	// All below are new for this version
	HANDLE rmutex;
	HANDLE smutex;
	DWORD rmutcnt;
	DWORD smutcnt;
	int recvThread;
	int sendThread;
	DWORD lastReadyStart;
	int ImmediateSendLimit;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes;
	int RecvBuffLimit;
	int SendBuffLimit;
	bool peerClosed;
	bool appClosed;
	bool pendingClose;
	WSOVERLAPPED *pendOvlRecvList;
	WSOVERLAPPED *pendOvlSendList;
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread;
	#else
	pthread_t pthread;
	#endif
	int TBufferSize;
	int RecvBuffLimit;
	int SendBuffLimit;
	bool peerClosed;
	bool appClosed;
	WSOVERLAPPED *pendOvlSendBeg;
	WSOVERLAPPED *pendOvlSendEnd;
	unsigned long IOCPSendBytes;
	int NumOvlSends;
	#endif
};

typedef struct tdCtoPort
{
	// remember to change to size of the memset in WSCtiClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	SOCKADDR_IN cast_addr;
	u_short Port;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	int PortWasActiveBeforeSuspend;
	char MonitorGroups[8][16];
	// evert thing ,including SockActive and below,gets reset during WSCtoClosePort();
	char SockActive;
	char OnLineStatusText[80];

	char Note[80];
	char Status[80];
	int SendTimeOut;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER OutBuffer;
}CTOPORT_1_0;

struct CTOPORT_1_1
{
	// remember to change to size of the memset in WSCtiClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	SOCKADDR_IN cast_addr;
	u_short Port;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	int PortWasActiveBeforeSuspend;
	char MonitorGroups[8][16];
	HANDLE smutex; //New for this version
	DWORD smutcnt; // New for this version
	int sendThread; //New for this version
	int SendBuffLimit; // New for this version
	// evert thing ,including SockActive and below,gets reset during WSCtoClosePort();
	char SockActive;
	char OnLineStatusText[80];

	char Note[80];
	char Status[80];
	int SendTimeOut;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER OutBuffer;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; // New for this version
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

typedef struct tdIpAclMap
{
	IPACL *Acls;
	int size;
	int cnt;
} IPACLMAP;

typedef struct tdCtiPort
{
	// remember to change to size of the memset in WSCtiClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	DWORD Bounce; //Bounce Port if No Data in X milliseconds
	long BounceStart; //Dont Bounce Port before;
	long BounceEnd; //Dont Bounce Port after;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
    char IPAclCfg[128];
    IPACL *IPAclList;
	IPACLMAP IPAclMap;
	char MonitorGroups[8][16];
	int PrefixPacketLen;
	// evert thing ,including SockActive and below,gets reset during WSCtiClosePort();
	char SockActive;
	char SockConnected;
	char OnLineStatusText[80];

	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;

	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	HWND setupDlg;
}CTIPORT_1_0;

struct CTIPORT_1_1
{
	// remember to change to size of the memset in WSCtiClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[80];
	DWORD Bounce; //Bounce Port if No Data in X milliseconds
	long BounceStart; //Dont Bounce Port before;
	long BounceEnd; //Dont Bounce Port after;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
    char IPAclCfg[128];
    IPACL *IPAclList;
	IPACLMAP IPAclMap;
	char MonitorGroups[8][16];
	int PrefixPacketLen;
	HANDLE rmutex; //New for this version
	DWORD rmutcnt; // New for this version
	int recvThread; //New for this version
	int RecvBuffLimit; // New for this version
	// evert thing ,including SockActive and below,gets reset during WSCtiClosePort();
	char SockActive;
	char SockConnected;
	char OnLineStatusText[80];
	long ReconCount;
	char Note[80];
	char Status[80];
	int SendTimeOut;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	HWND setupDlg;
	#ifdef WS_OIO
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	DWORD lastReadyStart; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	DWORD lastReadyStart; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

struct CTIPORT_1_2
{
	// remember to change to size of the memset in WSCtiClosePort() if u add or change to this non reset block
	char LocalIP[20];
	char RemoteIP[20];
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	int AltIPCount;
	int CurrentAltIP;
	u_short Port;
	int ConnectHold;
	int InUse;
	int BlockSize;
	char CfgNote[256];
	DWORD Bounce; //Bounce Port if No Data in X milliseconds
	long BounceStart; //Dont Bounce Port before;
	long BounceEnd; //Dont Bounce Port after;
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
    char IPAclCfg[128];
    IPACL *IPAclList;
	IPACLMAP IPAclMap;
	char MonitorGroups[8][16];
	int PrefixPacketLen;
	HANDLE rmutex; //New for this version
	DWORD rmutcnt; // New for this version
	int recvThread; //New for this version
	int RecvBuffLimit; // New for this version
	// evert thing ,including SockActive and below,gets reset during WSCtiClosePort();
	char SockActive;
	char SockConnected;
	char OnLineStatusText[80];
	long ReconCount;
	char Note[256];
	char Status[80];
	int SendTimeOut;
	DWORD LastDataTime;
	SOCKET_T Sock;
	unsigned long Seconds;
	__int64 BytesIn;
	__int64 BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	__int64 BytesOut;
	__int64 BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	BUFFER InBuffer;
	HWND setupDlg;
	#ifdef WS_OIO
	bool appClosed; // New for this version
	bool pendingClose; // New for this version
	WSOVERLAPPED *pendOvlRecvList; // New for this version
	WSOVERLAPPED *pendOvlSendList; // New for this version
	DWORD lastReadyStart; // New for this version
	#elif defined(WS_OTPP)
	#ifdef WIN32
	HANDLE pthread; // New for this version
	#else
	pthread_t pthread;
	#endif
	int TBufferSize; // New for this version
	bool peerClosed; // New for this version
	bool appClosed; // New for this version
	DWORD lastReadyStart; // New for this version
	WSOVERLAPPED *pendOvlSendBeg; // New for this version
	WSOVERLAPPED *pendOvlSendEnd; // New for this version
	unsigned long IOCPSendBytes; // New for this version
	int NumOvlSends; // New for this version
	#endif
};

typedef struct tdOtherPort
{
	char RemoteIP[20];
	u_short Port;
	char AltRemoteIP[WS_MAX_ALT_PORTS][20];
	int AltRemoteIPOn[WS_MAX_ALT_PORTS];
	int AltIPCount;
	int InUse;
	char Note[50];
	char Status[50];
	char Recieved[50];
	char Transmit[50];
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	void *DetPtr4;
	void *DetPtr5;
	void *DetPtr6;
	void *DetPtr7;
	void *DetPtr8;
	void *DetPtr9;
	void *DetPtr10;
	char Uuid[16];
	char MonitorGroups[8][16];
}OTHERPORT_1_0;

#pragma pack(pop)
#pragma pack(push,1)

// The maximum supported version
typedef CONPORT_1_3 CONPORT;
typedef USCPORT_1_2 USCPORT;
typedef USRPORT_2_1 USRPORT;
typedef FILEPORT_1_1 FILEPORT;
typedef UMCPORT_1_1 UMCPORT;
typedef UMRPORT_1_1 UMRPORT;
typedef CGDPORT_1_1 CGDPORT;
typedef UGCPORT_1_1 UGCPORT;
typedef UGRPORT_1_1 UGRPORT;
typedef CTOPORT_1_1 CTOPORT;
typedef CTIPORT_1_2 CTIPORT;
typedef OTHERPORT_1_0 OTHERPORT;

// Dynamic ports
class WSPort;
typedef list<WSPort*> WSPORTLIST;
typedef map<DWORD,WSPort*> WSPORTMAP;

// App (GetAppInterface and WsocksApp) versioning
enum WSAppVersion
{
	WS_APPVERS_UNKNOWN=0,
	WS_APPVERS_1_0=0x0100,
	WS_APPVERS_COUNT=2
};

// app.ini configuration
class AppConfig;
typedef list<AppConfig *> APPCONFIGLIST;
class AppFeature
{
public:
	AppFeature(int wsdate, int wstime, int dtno, string author, string desc)
	{
		this->wsdate=wsdate;
		this->wstime=wstime;
		this->devTrackNo=dtno;
		this->author=author;
		this->desc=desc;
	}

	int wsdate;
	int wstime;
	int devTrackNo;
	string author;
	string desc;
};
class AppConfig
{
public:
	AppConfig()
		:line(0)
		,aclass()
		,aname()
		,asyncMode(false)
		,DllPath()
		,ConfigPath()
		,RunPath()
		,LogPath()
		,Menu()
		,ver(WS_APPVERS_UNKNOWN)
		,Enabled(false)
		,TargetThreads()
		,reloadCode(0x01)
		,wshLoopback(false)
		,sysmonAddr("AUTO")
		,broadAddrs()
		,loopback(false)
        ,shareHostLog(false)
		,lbDllHnd(0)
		,plbGETAPPINTERFACE(0)
		,criticalApp(true)
		,threadAbortTimeout(15)
		#ifdef WS_OTMP
		,conOtmp(false)
		,usrOtmp(false)
		,umrOtmp(false)
		#endif
		#ifdef WSOCKS_SSL
		,SSLPW()
		,CAFILE()
		,CERTFILE()
		#endif
	{
	}
	static AppConfig *FindConfig(APPCONFIGLIST& configs, const string Name)
	{
		for(APPCONFIGLIST::iterator ait=configs.begin();ait!=configs.end();ait++)
		{
			AppConfig *pcfg=*ait;
			if(pcfg->aname==Name)
				return pcfg;
		}
		return 0;
	}
	bool operator==(const AppConfig& acfg)
	{
		return _stricmp(aname.c_str(),acfg.aname.c_str())?false:true;
	}

	int line;
	WSAppVersion ver;
	string aclass;
	string aname;
	bool asyncMode;
	string DllPath;
	string ConfigPath;
	string RunPath;
	string RunCmd;
	string LogPath;
	string Menu;
	bool Enabled;
	string TargetThreads;
	int reloadCode;
	bool wshLoopback;
	string sysmonAddr; // not in app.ini
	string broadAddrs; // not in app.ini
	bool criticalApp;
	int threadAbortTimeout;
	#ifdef WS_OTMP
	bool conOtmp;
	bool usrOtmp;
	bool umrOtmp;
	#endif
	list<AppFeature> featList;
	#ifdef WSOCKS_SSL
	string SSLPW;
	string CAFILE;
	string CERTFILE;
	#endif

	// Run-time created apps
	bool loopback;
    bool shareHostLog;
	HMODULE lbDllHnd;
	void *plbGETAPPINTERFACE;
};

class WsocksApp;
#define WSHOST_INTERFACE_VERSION 0x0100
#if defined(WIN32)||defined(_CONSOLE)
#ifdef BIT64
#define WSHOST_EXPORT_FUNC "?GetAppInterface@@YAPEAVWsocksApp@@PEAUHINSTANCE__@@GPEBD1@Z" // 64-bit signature
#else
#define WSHOST_EXPORT_FUNC "?GetAppInterface@@YGPAVWsocksApp@@PAUHINSTANCE__@@GPBD1@Z" // 32-bit signature
#endif
typedef WsocksApp *(__stdcall *GETAPPINTERFACE)(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
typedef void (__stdcall *FREEAPPINTERFACE)(WsocksApp *pmod);
#else//!WIN32
#define WSHOST_EXPORT_FUNC "GetAppInterface"
typedef void *(__stdcall *GETAPPINTERFACE)(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
typedef void (__stdcall *FREEAPPINTERFACE)(void *pmod);
#endif//!WIN32

typedef map<string,string> TVMAP;

// WSOCKS C++ application
// If you change the size of WsocksApp or add/remove any methods, you must update this date
#define WSOCKSAPI_DATE	20131112
class WsocksApp
{
public:
	WSAPILINK WsocksApp();
	WSAPILINK virtual ~WsocksApp();

	// IQMatrix interface
	AppConfig *pcfg;
	class WsocksHost *phost;
	HANDLE dllhnd;
	GETAPPINTERFACE pGetAppInterface;
	FREEAPPINTERFACE pFreeAppInterface;
	int buildDate;
	int buildTime;
	const char *__WSDATE__;
	const char *__WSTIME__;
	int initialized;
	DWORD threadSignal;
	ULONGLONG threadIdleStop;

	WSAPILINK virtual int WSHInitModule(AppConfig *config, class WsocksHost *apphost){return 0;}
	WSAPILINK virtual int WSHCleanupModule(){return 0;}
	WSAPILINK virtual int WSValidateModule(const char *challenge);

	// Wsocks attributes (legacy)
	// App time (app-specific to allow for playback emulation)
	int WSDate;
	int WSTime;
	int WSlTime;
	char WScDate[9];
	char WScTime[9];
	int WSDayOfWeek;

	// Port version
	int CON_PORT_VERSION;
	int USC_PORT_VERSION;
	int USR_PORT_VERSION;
	int FIL_PORT_VERSION;
	int UMC_PORT_VERSION;
	int UMR_PORT_VERSION;
	int CGD_PORT_VERSION;
	int UGC_PORT_VERSION;
	int UGR_PORT_VERSION;
	int CTO_PORT_VERSION;
	int CTI_PORT_VERSION;
	int OTHER_PORT_VERSION;
	// Port defines now dynamic
	int NO_OF_CON_PORTS;
	int NO_OF_USC_PORTS;
	int NO_OF_USR_PORTS;
	int NO_OF_FILE_PORTS;
	int NO_OF_UMC_PORTS;
	int NO_OF_UMR_PORTS;
	int NO_OF_CGD_PORTS;
	int NO_OF_UGC_PORTS;
	int NO_OF_UGR_PORTS;
	int NO_OF_CTO_PORTS;
	int NO_OF_CTI_PORTS;
	int NO_OF_OTHER_PORTS;
	int NO_OF_BRT_PORTS;
	int NO_OF_BRR_PORTS;
	// Port interface
	CONPORT *ConPort;
	USCPORT *UscPort;
	USRPORT *UsrPort;
	FILEPORT *FilePort;
	UMCPORT *UmcPort;
	UMRPORT *UmrPort;
	CGDPORT *CgdPort;
	UGCPORT *UgcPort;
	UGRPORT *UgrPort;
	CTOPORT *CtoPort;
	CTIPORT *CtiPort;
	OTHERPORT *OtherPort;

	// GUI defines now dynamic
	HWND WShWnd;
	HINSTANCE WShInst;
	int WS_USER;

	// Operations (app downcalls)
	// Run control
	WSAPILINK virtual bool WSInitialized();
	WSAPILINK virtual int WSPreInitSocks();
	WSAPILINK virtual int WSInitSocks();
	WSAPILINK virtual int WSPortsCfg(){return 0;}
	WSAPILINK virtual int WSSuspend();
	WSAPILINK virtual int WSResume();
	WSAPILINK virtual int WSCloseSocks();
	WSAPILINK virtual int WSExit(int ecode);
	WSAPILINK virtual int WSSyncLoop();
	#ifdef WS_OIO
	WSAPILINK virtual int WSIocpLoop(int timeout);
	#else
	WSAPILINK virtual int WSAsyncLoop();
	#endif
	WSAPILINK virtual int WSBusyThreadReport(DWORD estEndTime=0);
	WSAPILINK virtual int WSRestart(int timeout);

	// Date and time
	WSAPILINK long WSDiffTime(long tnew, long told);

	// Logging
	WSAPILINK virtual void WSResetErrorCnt(void);
	WSAPILINK virtual void WSLogEvent(const char *fmt,...);
	WSAPILINK virtual void WSLogError(const char *fmt,...);
	WSAPILINK virtual void WSLogDebug(const char *fmt,...);
	WSAPILINK virtual void WSLogRetry(const char *fmt,...);
	WSAPILINK virtual void WSLogRecover(const char *fmt,...);
	WSAPILINK virtual void WSLogEventVA(const char *fmt, va_list& alist);
	WSAPILINK virtual void WSLogErrorVA(const char *fmt, va_list& alist);
	WSAPILINK virtual void WSLogDebugVA(const char *fmt, va_list& alist);
	WSAPILINK virtual void WSLogRetryVA(const char *fmt, va_list& alist);
	WSAPILINK virtual void WSLogRecoverVA(const char *fmt, va_list& alist);
	WSAPILINK virtual void WSLogEventStr(const char *str);
	WSAPILINK virtual void WSLogErrorStr(const char *str);

	// Status
	WSAPILINK virtual void WSSetAppHead(char *Head);
	WSAPILINK virtual void WSSetAppStatus(char *Appver, char *Status);
	WSAPILINK virtual const char *SizeStr(__int64 bigsize, bool byteUnits=true);
    WSAPILINK virtual int WSGetAppHead(char *heading, int hlen);
    WSAPILINK virtual int WSGetAppStatus(char *status, int slen);
    WSAPILINK virtual int WSGetHostBuildDate();
    WSAPILINK virtual int WSGetHostBuildTime();

	// High resolution timer
	WSAPILINK ULONGLONG WSGetTimerCount();
	WSAPILINK DWORD WSDiffTimerCount(ULONGLONG t1, ULONGLONG t2);

	// System resources
	WSAPILINK void WSCheckSystemResourcesAvailable();
	WSAPILINK void WSGetSystemResourcesAvailable(struct _PROCESS_MEMORY_COUNTERS *pProcessMemoryCounters);

	// Dynamic port creation
	WSAPILINK virtual int WSSetMaxPorts(WSPortType PortType, int NumPorts);
	WSAPILINK virtual WSPort *WSCreatePort(WSPortType PortType, int PortNo=-1);
	WSAPILINK virtual WSPort *WSGetPort(WSPortType PortType, int PortNo);
	WSAPILINK virtual int WSGetPorts(WSPortType PortType, WSPORTLIST& portList);
	WSAPILINK virtual int WSUpdatePort(WSPortType PortType, int PortNo);
	WSAPILINK int WSDeletePort(WSPortType PortType, int PortNo);
	WSAPILINK virtual int WSLockPort(WSPortType PortType, int PortNo, bool send);
	WSAPILINK virtual int WSUnlockPort(WSPortType PortType, int PortNo, bool send);

	// Sysmon commands
	WSAPILINK virtual int WSGetTagValues(TVMAP& tvmap, const char *parm, int plen);
	WSAPILINK virtual string WSGetValue(TVMAP& tvmap, string tag);
	WSAPILINK virtual int WSSetTagValues(TVMAP& tvmap, char *parm, int plen);
	WSAPILINK virtual int WSSetValue(TVMAP& tvmap, string tag, string value);
	WSAPILINK virtual int WSDelTag(TVMAP& tvmap, string tag);
	WSAPILINK virtual int WSSysmonResp(int UmrPortNo, int reqid, string cmd, const char *parm, int plen);

	// CON ports
	WSAPILINK virtual int WSSetupConPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenConPort(int PortNo);
	WSAPILINK virtual int WSConConnect(int PortNo);
	WSAPILINK virtual int WSConSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	WSAPILINK virtual int WSConSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets);
	WSAPILINK virtual int WSConSendBuffNew(int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend);
	WSAPILINK virtual int WSCloseConPort(int PortNo);
	WSAPILINK virtual int WSBeginUpload(const char *spath, const char *DeliverTo, const char *OnBehalfOf,
										int rid, const char *fpath, WSPortType PortType, int PortNo, 
										LONGLONG begin=0);
	WSAPILINK virtual int WSSendFile(const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
									 int rid, const char *fpath, WSPortType PortType, int PortNo,
									 LONGLONG begin=0, LONGLONG rsize=0, LONGLONG tsize=0, DWORD ssize=0, bool multi=false);
	WSAPILINK virtual int WSRecvFile(const char *Msg, int MsgLen, WSPortType PortType, int PortNo,
									 int *rid, char *fkey, int klen, char *lpath, int llen, char *DeliverTo, WORD dlen, char *OnBehalfOf, WORD blen);

	// USC ports
	WSAPILINK virtual int WSSetupUscPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSCfgUscPort(int PortNo);
	WSAPILINK virtual int WSOpenUscPort(int PortNo);
	WSAPILINK virtual int WSCloseUscPort(int PortNo);

	// USR ports
	WSAPILINK virtual int WSSetupUsrPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSUsrSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	WSAPILINK virtual int WSUsrSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets);
	WSAPILINK virtual int WSCloseUsrPort(int PortNo);

	// UMC ports
	WSAPILINK virtual int WSSetupUmcPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenUmcPort(int PortNo);
	WSAPILINK virtual int WSCloseUmcPort(int PortNo);

	// UMR ports
	WSAPILINK virtual int WSSetupUmrPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSUmrSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	WSAPILINK virtual int WSUmrSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets);
	WSAPILINK virtual int WSCloseUmrPort(int PortNo);

	// FILE ports
	WSAPILINK int WSMakeLocalDirs(const char *lpath);
	#ifdef WS_FILE_SERVER
	FileOptions WSFileOptions;
	WSAPILINK virtual int WSSetupFilePorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenFilePort(int PortNo);
	WSAPILINK virtual int WSFileConnect(int PortNo);
	WSAPILINK virtual int WSCloseFilePort(int PortNo);

	WSAPILINK BOOL WSReplaceFile(const char *spath, const char *dpath);
	WSAPILINK BOOL WSTransmitFile(LPCTSTR lpLocalFile, LPCTSTR lpRemoteFile, 
								  const FILETIME *lpCreationTime, 
								  const FILETIME *lpLastAccessTime, 
								  const FILETIME *lpLastWriteTime, FileOptions *opts);
	WSAPILINK BOOL WSDownloadFile(LPCTSTR lpRemoteFile, LPCTSTR lpLocalFile, FileOptions *opts);

	WSAPILINK HANDLE WSFindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, FileOptions *opts);
	WSAPILINK BOOL WSFindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
	WSAPILINK BOOL WSFindClose(HANDLE hFindFile);
	WSAPILINK int WSPathFileExists(LPCTSTR pszPath, FileOptions *opts);
	WSAPILINK int WSPathIsDirectory(LPCTSTR pszPath, FileOptions *opts);
	WSAPILINK BOOL WSCreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes, FileOptions *opts);
	WSAPILINK BOOL WSDeleteFile(LPCTSTR pszPath, BOOL bOverrideAttributes, FileOptions *opts);
	WSAPILINK int WSRecurseDelete(const char *tdir);
	#endif

	#ifdef WS_GUARANTEED
	// CGD ports
	WSAPILINK virtual int WSSetupCgdPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenCgdPort(int PortNo);
	WSAPILINK virtual int WSCgdConnect(int PortNo);
	WSAPILINK virtual int WSCgdSendMsg(WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo);
	WSAPILINK virtual int WSCloseCgdPort(int PortNo);

	// UGC ports
	WSAPILINK virtual int WSSetupUgcPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenUgcPort(int PortNo);
	WSAPILINK virtual int WSUgcSendMsg(WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, WORD LineId);
	WSAPILINK virtual int WSCloseUgcPort(int PortNo);
	WSAPILINK virtual int WSCfgUgcPort(int PortNo);

	// UGR ports
	WSAPILINK virtual int WSSetupUgrPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSUgrSendMsg(WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, unsigned int GDLogId);
	WSAPILINK virtual int WSUgrSendNGMsg(WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo);
	WSAPILINK virtual int WSCloseUgrPort(int PortNo);
	#endif

	// CTO ports
	WSAPILINK virtual int WSSetupCtoPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenCtoPort(int PortNo);
	WSAPILINK virtual int WSCtoSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets);
	WSAPILINK virtual int WSCloseCtoPort(int PortNo);

	// CTI ports
	WSAPILINK virtual int WSSetupCtiPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenCtiPort(int PortNo);
	WSAPILINK virtual int WSCloseCtiPort(int PortNo);

	// OTHER ports
	WSAPILINK virtual int WSSetupOtherPorts(int SMode, int PortNo=-1);
	WSAPILINK virtual int WSOpenOtherPort(int PortNo);
	WSAPILINK virtual int WSCloseOtherPort(int PortNo);

	// Recording
	WSAPILINK virtual int WSOpenRecording(WSRECORDING *wsr, HWND parent, int Type, int PortNo);
	WSAPILINK virtual void WSCloseRecording(WSRECORDING *wsr, int Type, int PortNo);
	WSAPILINK virtual int WSRecord(WSRECORDING *wsr, const char *buf, int len, BOOL send);

	// Network aliasing
	WSAPILINK virtual bool WSTranslateAlias(char *ipstr, int len);

	#ifdef WIN32
	// Debugging
	WSAPILINK virtual int WSGenerateDump(DWORD tid, EXCEPTION_POINTERS *pException=0);
	#endif


	// App notifications (upcalls)
	// Date and time
	WSAPILINK virtual void WSDateChange(){}
	WSAPILINK virtual void WSTimeChange(){}
	WSAPILINK virtual int WSIdle(ULONGLONG StopTime){return 0;}
	WSAPILINK virtual int WSTickIdle(){return 0;}
	WSAPILINK virtual int WSPreTick(){return 0;}

	// CON ports
	WSAPILINK virtual void WSConConnecting(int PortNo){}
	WSAPILINK virtual void WSConOpened(int PortNo){}
	WSAPILINK virtual int WSConMsgReady(int PortNo){return 0;}
	WSAPILINK virtual int WSConStripMsg(int PortNo){return 0;}
	WSAPILINK virtual int WSBeforeConSend(int PortNo){return 0;}
	WSAPILINK virtual void WSConClosed(int PortNo){}
	WSAPILINK virtual int WSDecompressed(int PortType,int PortNo,char *DecompBuff,int DecompSize,char PxySrc[PROXYCOMPRESS_MAX]);
	WSAPILINK virtual void WSConClosing(int PortNo){}

	// USC ports
	WSAPILINK virtual void WSUscOpened(int PortNo){}
	WSAPILINK virtual void WSUscClosed(int PortNo){}

	// USR ports
	WSAPILINK virtual void WSUsrOpened(int PortNo){}
	WSAPILINK virtual int WSUsrMsgReady(int PortNo){return 0;}
	WSAPILINK virtual int WSUsrStripMsg(int PortNo){return 0;}
	WSAPILINK virtual int WSBeforeUsrSend(int PortNo){return 0;}
	WSAPILINK virtual void WSUsrClosed(int PortNo){}

	// UMC ports
	WSAPILINK virtual void WSUmcOpened(int PortNo){}
	WSAPILINK virtual void WSUmcClosed(int PortNo){}

	// UMR ports
	WSAPILINK virtual void WSUmrOpened(int PortNo){}
	WSAPILINK virtual int WSUmrMsgReady(int PortNo);
	WSAPILINK virtual int WSUmrStripMsg(int PortNo);
	WSAPILINK virtual int WSBeforeUmrSend(int PortNo){return 0;}
	//WSAPILINK virtual int WSUmrSendHealth(int PortNo, int MsgDate, int MsgTime);
	//WSAPILINK virtual int WSUmrSetHealth(int index, int code, const char *desc);
	WSAPILINK virtual void WSUmrClosed(int PortNo){}
	WSAPILINK virtual int WSSysmonMsg(int UmrPortNo, int reqid, string cmd, const char *parm, int plen, void *udata){return 0;}
	//WSAPILINK virtual void WSSysmonResponse(int reqid, string cmd, const char *resp, int rlen, void *udata){};
	//WSAPILINK virtual int WSRemoteCommand(int cmdid, string cmd, const char *parm, int plen, WSPortType PortType, int PortNo){return -1;}
	WSAPILINK virtual int WSSysmonFeed(int UmrPortNo, string feed, const char *buf, int blen);

	#ifdef WS_FILE_SERVER
	// FILE ports
	WSAPILINK virtual void WSFileOpened(int PortNo){}
	WSAPILINK virtual int WSFileMsgReady(int PortNo);
	WSAPILINK virtual int WSFileStripMsg(int PortNo);
	WSAPILINK virtual int WSBeforeFileSend(int PortNo);
	WSAPILINK virtual void WSFileClosed(int PortNo){}
    WSAPILINK virtual void WSFileTransfered(const WSFileTransfer_2_0 &TempFile){}
	#endif

	#ifdef WS_GUARANTEED
	// CGD ports
	WSAPILINK virtual void WSGetCgdId(int PortNo,GDID *GDId);
	WSAPILINK virtual void WSCgdConnecting(int PortNo){}
	WSAPILINK virtual void WSCgdOpened(int PortNo){}
	WSAPILINK virtual int WSCgdMsgReady(int PortNo);
	WSAPILINK virtual int WSCgdStripMsg(int PortNo);
	WSAPILINK virtual int WSBeforeCgdSend(int PortNo){return 0;}
	WSAPILINK virtual void WSCgdClosed(int PortNo){}
	WSAPILINK virtual int WSTranslateCgdMsg(struct tdMsgHeader *MsgHeader, char *Msg, int PortNo){return 0;}

	// UGC ports
	WSAPILINK virtual void WSGetUgcId(int PortNo, char SessionId[20]);
	WSAPILINK virtual void WSUgcOpened(int PortNo){}
	WSAPILINK virtual void WSUgcClosed(int PortNo){}

	// UGR ports
	WSAPILINK virtual void WSUgrOpened(int PortNo){}
	WSAPILINK virtual int WSUgrMsgReady(int PortNo);
	WSAPILINK virtual int WSUgrStripMsg(int PortNo);
	WSAPILINK virtual int WSBeforeUgrSend(int PortNo){return 0;}
	WSAPILINK virtual void WSUgrClosed(int PortNo){}
	WSAPILINK virtual int WSTranslateUgrMsg(struct tdMsgHeader *MsgHeader, char *Msg, int PortNo, WORD LineId){return 0;}
	#endif

	// CTO ports
	WSAPILINK virtual void WSCtoOpened(int PortNo){}
	WSAPILINK virtual int WSBeforeCtoSend(int PortNo){return 0;}
	WSAPILINK virtual void WSCtoClosed(int PortNo){}

	// CTI ports
	WSAPILINK virtual void WSCtiOpened(int PortNo){}
	WSAPILINK virtual int WSCtiMsgReady(int PortNo){return 0;}
	WSAPILINK virtual int WSCtiFilter(struct sockaddr_in *Addr,int PortNo){return 1;}
	WSAPILINK virtual int WSCtiStripMsg(int PortNo){return 0;}
	WSAPILINK virtual void WSCtiClosed(int PortNo){}

	// OTHER ports
	WSAPILINK virtual void WSOtherOpened(int PortNo){}
	WSAPILINK virtual void WSOtherClosed(int PortNo){}

	// Misc app notifications
	WSAPILINK virtual void WSThreadCrashed(FILE *fp, WSPortType PortType, int PortNo){}
	WSAPILINK virtual void WSThreadTimeout(FILE *fp, WSPortType PortType, int PortNo, int ms){}
	WSAPILINK virtual int WSBackupApp(char bpath[MAX_PATH], char emsg[256], bool overwrite);
	WSAPILINK virtual int WSRestoreApp(char bpath[MAX_PATH], char emsg[256]);

	// Implementation
public:
	//friend class WsocksHost;
	//friend class WsocksHostImpl;

	// Run control
	int WSInitCnt;
	int WSSuspendMode;	// 0 = Suspend off 
						// 1 = All new Connections Suspended 
						// 2 = All ports Suspended
	HANDLE hIOPort;

	// Date and time
	DWORD lastTickDiff;
	DWORD lastTimeChange;

	// Status
	char AppVerStr[256];
	char SizeStr_zstrs[8][40];
	int SizeStr_zidx;

	// Dynamic port creation
	WSPORTMAP portMap;

	// GUI
	HWND WShDlgError;
	HWND WShDlgEvent;
	HWND WShDlgDebug;
	int  WSShowDlgDebug;
	HWND WShDlgRecover;
	int  WSShowDlgRecover;
	HWND WShDlgConList;
	HWND WShConList;
#ifdef WIN32
	RECT WShDlgConList_LastRect;
#endif

	// Logging
#ifdef WIN32
	HANDLE errhnd,evehnd;
#else
	FILE *errhnd,*evehnd;
#endif
	int fpErrDate,fpEveDate;
	string WSErrorLogPath;
	string WSEventLogPath;
	string WSDebugLogPath;
	string WSRetryLogPath;
	string WSRecoverLogPath;

#ifdef WIN32
	// High-resolution timer
	static LARGE_INTEGER WSPerformanceFreq;
	static ULONGLONG WSMips;
	static ULONGLONG WSMaxIdleMips;
	static ULONGLONG WSIdleExtraCycles;
#endif

	#ifdef WS_FILE_SERVER
	// FILE ports
	BOOL WSFileTransferReady;
	int WSFileXferInitCnt;
	WSFileTransfer *fileTransfers;
	WSFileRecommit *fileRecommits;
	#endif

	#ifdef WS_GUARANTEED
	// GD ports
	BOOL WSUgcNoGap;
	#endif

	// Compression and Encryption
	char SyncLoop_CompBuff[WS_MAX_BLOCK_SIZE*2048];	
	char SyncLoop_EncryptBuff[WS_MAX_BLOCK_SIZE*2048];	
	char ConRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char ConRead_szDecompBuff[WS_MAX_BLOCK_SIZE*1024];
	char ConRead_szDecryptBuff[WS_MAX_BLOCK_SIZE*1024];
	char ConRead_PxySrc[PROXYCOMPRESS_MAX];
	char UsrRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char UsrRead_szDecompBuff[WS_MAX_BLOCK_SIZE*1024];
	char UsrRead_szDecryptBuff[WS_MAX_BLOCK_SIZE*1024];
	char CtiRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char UmrRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char UmrRead_szDecompBuff[WS_MAX_BLOCK_SIZE*1024];
	char UmrRead_szDecryptBuff[WS_MAX_BLOCK_SIZE*1024];
	#ifdef WS_FILE_SERVER
	char FileRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char FileRead_szDecompBuff[WS_MAX_BLOCK_SIZE*1024];
	char FileRead_szDecryptBuff[WS_MAX_BLOCK_SIZE*1024];
	char FileTransfer_CompBuff[WS_MAX_BLOCK_SIZE*2048];
	#endif
	#ifdef WS_GUARANTEED
	char CgdRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char CgdRead_szDecompBuff[WS_MAX_BLOCK_SIZE*1024];
	char CgdRead_szDecryptBuff[WS_MAX_BLOCK_SIZE*1024];
	char UgrRead_szTemp[WS_MAX_BLOCK_SIZE*1024];
	char UgrRead_szDecompBuff[WS_MAX_BLOCK_SIZE*1024];
	char UgrRead_szDecryptBuff[WS_MAX_BLOCK_SIZE*1024];
	char UgrAuth_lastErrStr[256];
	int UgrAuth_lastErrTime;
	#endif
	int WSResetCryptKeys(int PortType, int PortNo);

	// Multi-threading
	HANDLE appMutex;
	class WsocksThread *activeThread;
	#ifdef WS_OIO
	WSOVERLAPPED *cxlOvlList;
	#endif

	// Sysmon
	// Probably needs mutex for these codecs
	class SysCmdCodec *ccodec;
	class SysFeedCodec *fcodec;
	DWORD lvl1Interval;		// Level 1 feed interval
	DWORD lvl1UpdateTime;	// Last level 1 update time
	DWORD lvl2Interval;		// Level 2 feed interval
	DWORD lvl2UpdateTime;	// Last level 2 update time
};
typedef list<WsocksApp*> WSAPPLIST;
typedef map<string,WsocksApp*> WSAPPMAP;

class WsocksGuiApp :public WsocksApp
{
public:
	WSAPILINK virtual int WSCreateGuiHost(AppConfig *pcfg, bool lbmonitor);
	WSAPILINK virtual int WSDestroyGuiHost();
};

#pragma pack(pop)

extern "C"
{
#ifndef _DNSAPI_IMPL_H
typedef struct tdMsgHeader
{
	WORD MsgID;
	WORD MsgLen;
} MSGHEADER;
#endif

// From zlib2.lib
WSAPILINK int compress(char *dest,unsigned int *destLen,const char *source,unsigned int sourceLen);
WSAPILINK int uncompress(char *dest,unsigned int *destLen,const char *source,unsigned int sourceLen);

// From cscompress.lib
WSAPILINK int cscompress(char *dest,unsigned int *destLen,const char *source,unsigned int *sourceLen, int sendLast);
WSAPILINK int csuncompress(char *dest,unsigned int *destLen,const char *source,unsigned int *sourceLen);

// From des3.lib
#ifndef DES3_DECL
#define DES3DECL int pascal
#endif
WSAPILINK DES3DECL des_3EEEinit(UCHAR *key, int len, DES_KS ks1, DES_KS ks2, DES_KS ks3);
WSAPILINK DES3DECL des_cfb3EEEencode(UCHAR *inpb, UCHAR *outpb, SIZE_T len, UCHAR *iv64, DES_KS ks1, DES_KS ks2, DES_KS ks3);
WSAPILINK DES3DECL des_cfb3EEEdecode(UCHAR *inpb, UCHAR *outpb, SIZE_T len, UCHAR *iv64, DES_KS ks1, DES_KS ks2, DES_KS ks3);

};//extern "C"

#endif//__cplusplus
#endif//_WSOCKSAPI_H
