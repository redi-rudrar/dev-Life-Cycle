
#ifndef _LBMONITOR_H
#define _LBMONITOR_H

#include "sysmonc.h"
#include "smcmds.h"
#include "smtask.h"

#include <list>
#include <map>
using namespace std;

// Define to prevent deadlock between taskMgr mutex and port mutexes
#define FIX_TASK_CALL

// Sysmon client info
class SysmonFlags
{
public:
	SysmonFlags()
		:appListRid(0)
		,minRid(0)
		,pmod(0)
	{
	}

	int appListRid;			// Request id for applist feed
	int minRid;				// Minimum rid for which responses are proxied
	class LBMonitor *pmod;
};

// Common Sysmon commands
enum SysmonTasks
{
	SMTASK_UNKNOWN=0,
	SMTASK_APPLIST,
	SMTASK_EVENTLOG_HIST,
	SMTASK_EVENTLOG,
	SMTASK_ERRORLOG_HIST,
	SMTASK_ERRORLOG,
	SMTASK_LEVEL1_HIST,
	SMTASK_LEVEL1,
	SMTASK_LEVEL2_HIST,
	SMTASK_LEVEL2,
	SMTASK_TRADEMON_HIST,
	SMTASK_TRADEMON,
    SMTASK_HEARTBEAT_HIST,
    SMTASK_HEARTBEAT,
	SMTASK_CUSTOM_HIST,
	SMTASK_CUSTOM,
	SMTASK_COUNT,
};
struct SMFeedHint
{
	char *rptr;
	int rlen;
};

class LBMonitorNotify
{
public:
	// Called by wsockshost to notify app events
	virtual int LBMAppSetupSocks(WsocksApp *pmod)=0;
	virtual int LBMAppCloseSocks(WsocksApp *pmod)=0;
	virtual int LBMPortChanged(WsocksApp *pmod, WSPort *wsport)=0;
	// Called by wsockshost to fetch sysmon bind port
	virtual int LBMGetPort()=0;
	virtual int LBMGetLBPort()=0;
	// Called by wsockshost to notify log events
	virtual int LBMLoggedError(WsocksApp *pmod, __int64& off, const char *estr)=0;
	virtual int LBMLoggedEvent(WsocksApp *pmod, __int64& off, const char *estr)=0;
};

// LBMonitor is the encapsulized replacement for wsocks sysmon code
class LBMonitor 
	:public WsocksApp
	,public LBMonitorNotify
	,public SysCmdNotify
	,public SMCmdsNotify
{
public:
	LBMonitor();
	~LBMonitor();

	// WsocksApp
	int WSHInitModule(AppConfig *config, class WsocksHost *apphost);
	int WSHCleanupModule();

	int WSPortsCfg();
	void WSTimeChange();
#ifdef FIX_TASK_CALL
	int WSPreTick();
#endif

    void WSConOpened(int PortNo);
    int WSConMsgReady(int PortNo);
    int WSConStripMsg(int PortNo);
    int WSBeforeConSend(int PortNo);
    void WSConClosed(int PortNo);

	void WSUmrOpened(int PortNo);
    int WSUmrMsgReady(int PortNo);
    int WSUmrStripMsg(int PortNo);
    int WSBeforeUmrSend(int PortNo);
    void WSUmrClosed(int PortNo);

	// LBMonitorNotify
	int LBMAppSetupSocks(WsocksApp *pmod);
	int LBMAppCloseSocks(WsocksApp *pmod);
	int LBMPortChanged(WsocksApp *pmod, WSPort *pport);
	int LBMGetPort(){ return UmcPort[1].bindPort; }
	int LBMGetLBPort(){ return UmcPort[0].bindPort; }
	int LBMLoggedError(WsocksApp *pmod, __int64& off, const char *estr);
	int LBMLoggedEvent(WsocksApp *pmod, __int64& off, const char *estr);

	// SysCmdNotify
	void NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata);
	void NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata);

	// SMCmdsNotify
	inline int SMCSendMsg(WORD MsgID, WORD MsgLen, char *MsgOut, WSPortType PortType, int PortNo)
	{
		return WSUmrSendMsg(MsgID,MsgLen,MsgOut,PortNo);
	}
	int SMCBeginUpload(const char *spath, const char *DeliverTo, const char *OnBehalfOf,
						      int rid, const char *fpath, WSPortType PortType, int PortNo, 
							  LONGLONG begin=0);
	inline int SMCSendFile(const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
						   int rid, const char *fpath, WSPortType PortType, int PortNo,
						   LONGLONG begin=0, LONGLONG rsize=0, LONGLONG tsize=0, DWORD ssize=0, bool multi=false)
	{
		return WSSendFile(fkey,DeliverTo,OnBehalfOf,rid,fpath,PortType,PortNo,begin,rsize,tsize,ssize,multi);
	}
	bool SMCSendFinished(WSPortType PortType, int PortNo, int threshold);

public:
	// Feed callbacks
	int SendLvl1Quote(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendLvl2Quote(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendErrorLog(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendEventLog(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendTrademon(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendCustom(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);

protected:
	static int nextLBMonPort;	// Auto-port select
	SMTaskMgr taskmgr;			// Feed task manager
	SMCmds scmds;				// Common commands handler
	int proxyAuthPort;			// Proxy authorization
	unsigned char uuid[16];		// Unique Id for multi-NIC discovery
#ifdef FIX_TASK_CALL
	CRITICAL_SECTION emutex;
	list<char*> elist;			// Log and port change events that need to be handled by LBMon thread
#endif

	// Message handlers
	int TranslateConMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo);
	int TranslateUmrMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo);
	// Log feed
	int SendLogHist(int UmrPortNo, WsocksApp *pmod, const char *fpath, const char *&fname, __int64& begin, __int64& end, __int64& tot);
	// NIC enumeration and matching
	int EnumNICs(list<string>& nics);
	int FindNIC(list<string>& nics, const char *iface, char nic[32]);

	int GetPortSpec(WsocksApp *pmod, const char *cptr, WSPortType& PortType, int& PortNo);
};
#if defined(WIN32)||defined(_CONSOLE)
extern "C" WsocksApp* __stdcall LBMonitor_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
extern "C" void __stdcall LBMonitor_FreeAppInterface(WsocksApp *pmod);
#else
extern "C" void * __stdcall LBMonitor_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
extern "C" void __stdcall LBMonitor_FreeAppInterface(void *pmod);
#endif

#endif//_LBMONITOR_H
