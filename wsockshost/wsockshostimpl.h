
#ifndef WSOCKSHOSTIMPL_H
#define WSOCKSHOSTIMPL_H

#include "sysmonc.h"
#include "wsportimpl.h"
#include "lbmonitor.h"
#ifdef WSOCKS_SSL
#include "SSLGateway.h"
#endif

#pragma pack(push,1)

// A host may run multiple threads
class WsocksThread
{
public:
	WsocksThread();
	~WsocksThread();
	int Lock();
	void Unlock();

	class WsocksHostImpl *phost;
	string name;
	DWORD tid;
	#ifdef WIN32
	HANDLE thnd;
	#else
	pthread_t thnd;
	#endif
	bool abort;
	WSAPPLIST appList;
	WSAPPLIST unloadList;
	WSAPPLIST reloadList;
	APPCONFIGLIST loadList;
	HANDLE mutex;
	DWORD lastTickWarn;
	DWORD lastTickErr;
	int threadAbortTimeout;
	int restartCnt;
	DWORD startTime;
	WsocksApp *activeApp;

	char muxtrail[32];
	char *muxtptr;
	string cfpath;
	FILE *cfp;
};
typedef map<string,WsocksThread *> WSTHREADMAP;
typedef map<DWORD,WsocksThread *> WSTHREADIDMAP;
#define DEF_THREAD_ABORT_TIMEOUT 15

// OS-specific implementatino
#ifdef WIN32

#ifndef PROCESS_MEMORY_COUNTERS
// From <psapi.h>
typedef struct _PROCESS_MEMORY_COUNTERS {  
	DWORD cb;  
	DWORD PageFaultCount;  
	SIZE_T PeakWorkingSetSize;  
	SIZE_T WorkingSetSize;  
	SIZE_T QuotaPeakPagedPoolUsage;  
	SIZE_T QuotaPagedPoolUsage;  
	SIZE_T QuotaPeakNonPagedPoolUsage;  
	SIZE_T QuotaNonPagedPoolUsage;  
	SIZE_T PagefileUsage;  
	SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;
#endif

#if defined(WS_OIO)||defined(WS_OTPP)
struct OVERLAPBLOCK
{
	WSOVERLAPPED ovls[128];
	OVERLAPBLOCK *prev;
	OVERLAPBLOCK *next;
};
#endif

#else//!WIN32

#ifndef _CONSOLE
#define WSAECONNABORTED ECONNABORTED
#define WSAESOCKTNOSUPPORT EAFNOSUPPORT
#define WSAENOBUFS ENOBUFS
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAECONNRESET ECONNRESET
#define WSAECONNREFUSED ECONNREFUSED
#define WSAEISCONN EISCONN
#endif

#endif//!WIN32

typedef map<string,WSPort*> LBSMAP;
typedef list<WSPort*> LBCLIST;

#ifdef WS_OTMP
#define WS_NTMP 25
struct THREADPOOL
{
	HANDLE thnd;
	bool shutdown;
	int nports;
	WSPort *ports[WS_NTMP];
	int tsizes[WS_NTMP];
	char *tbufs[WS_NTMP];
	BUFFER TBuffers[WS_NTMP];
	DWORD RecvBuffLimits[WS_NTMP];
	THREADPOOL *next;
};
#endif

class WsocksHostImpl 
	:public WsocksHost
	,public SysCmdNotify
	#ifdef WSOCKS_SSL
	,public SSLGatewayNotify
	#endif
{
public:
	WsocksHostImpl();
	virtual ~WsocksHostImpl();

	// WsocksHost Implementation
public:
	// Two-phase initialization
	virtual int WSHInitHost(const char *logPath);
	//virtual int WSHInitSingleHost(WsocksApp *pmod, const char *aname);
	virtual int WSHCleanupHost();

	// Operations (downcalls)
	// This should be called once per second
	virtual void WSHCheckTime(void);
	// This should be called when idle
	virtual int WSHIdle(ULONGLONG tstop);
	// This should be called when a connection times out and is bounced
	//virtual void WSHLogSendTimeout(int PortType, int PortNo);
	// This should be called once per second
	//virtual void WSHSendMonitorData(WsocksApp *pmod, int PortNo);

	// These must be implemented to log the host app events
	virtual void WSHostLogEvent(const char *format,...);
	virtual void WSHostLogError(const char *format,...);
	virtual void WSHostLogEventVA(const char *format, va_list& alist);
	virtual void WSHostLogErrorVA(const char *format, va_list& alist);
	virtual int WSHCloseApp(WsocksApp *pmod);
	// Network aliasing
	virtual bool WSHTranslateAlias(char *ipstr, int len);
	#ifdef WIN32
	// Debugging
	virtual int WSHGenerateDump(WsocksApp *pmod, DWORD tid, EXCEPTION_POINTERS *pException=0);
	#endif
	#ifdef WSOCKS_SSL
	// SSL certificates
	int ReloadCertificates(bool server);
	#endif

protected:
	// Notifications (upcalls)
	// App loading
	virtual int WSHAppLoading(WsocksApp *pmod)=0;
	virtual int WSHAppLoadFailed(WsocksApp *pmod)=0;
	virtual int WSHAppLoaded(WsocksApp *pmod)=0;
	virtual int WSHAppUnloaded(WsocksApp *pmod)=0;

	// Port creation
	virtual int WSHPortCreated(WsocksApp *pmod, WSPort *pport)=0;
	virtual int WSHPortModified(WsocksApp *pmod, WSPort *pport)=0;
	virtual bool WSHIsPortAvailable(unsigned short bport);
	virtual int WSHPortDeleted(WsocksApp *pmod, WSPort *pport)=0;

	// Date and time
	virtual long WSHDiffTime(long tnew, long told);

	// Logging
	virtual int WSHLoggedEvent(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedError(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedDebug(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedRetry(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedRecover(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHHostLoggedEvent(const char *estr)=0;
	virtual int WSHHostLoggedError(const char *estr)=0;

	// Show/hide UI window
	virtual int WSHShowWindow(int nCmdShow)=0;

	// Operations (implementation of WsocksApp downcalls)
	// Run control
	virtual int WSHPreInitSocks(WsocksApp *pmod);
	virtual int WSHInitSocks(WsocksApp *pmod);
	virtual int WSHSuspend(WsocksApp *pmod);
	virtual int WSHResume(WsocksApp *pmod);
	virtual int WSHCloseSocks(WsocksApp *pmod);
	//virtual int WSHExitApp(WsocksApp *pmod, int ecode);
	//virtual int WSHExitHost(int ecode);
	virtual int WSHSyncLoop(WsocksApp *pmod);
	#ifdef WS_OIO
	virtual int WSHIocpLoop(WsocksApp *pmod, DWORD timeout);
	virtual void CancelOverlap(WsocksApp *pmod, WSOVERLAPPED *povl);
	#else
	virtual int WSHAsyncLoop(WsocksApp *pmod);
	#endif
	virtual int WSHWaitMutex(int mutexIdx, int timeout);
	virtual int WSHReleaseMutex(int mutexIdx);
	virtual int WSHBusyThreadReport(WsocksApp *pmod, DWORD estEndTime);
	virtual int WSHRestart(WsocksApp *pmod, int timeout);

	// Logging
	virtual void WSHResetErrorCnt(WsocksApp *pmod);
	virtual void WSHLogEvent(WsocksApp *pmod, const char *format,...);
	virtual void WSHLogError(WsocksApp *pmod, const char *format,...);
	virtual void WSHLogDebug(WsocksApp *pmod, const char *format,...);
	virtual void WSHLogRetry(WsocksApp *pmod, const char *format,...);
	virtual void WSHLogRecover(WsocksApp *pmod, const char *format,...);
	virtual void WSHLogEventVA(WsocksApp *pmod, const char *format, va_list& alist);
	virtual void WSHLogErrorVA(WsocksApp *pmod, const char *format, va_list& alist);
	virtual void WSHLogDebugVA(WsocksApp *pmod, const char *format, va_list& alist);
	virtual void WSHLogRetryVA(WsocksApp *pmod, const char *format, va_list& alist);
	virtual void WSHLogRecoverVA(WsocksApp *pmod, const char *format, va_list& alist);
	virtual void WSHLogEventStr(WsocksApp *pmod, const char *str);
	virtual void WSHLogErrorStr(WsocksApp *pmod, const char *str);

	// Status
	virtual void WSHSetAppHead(WsocksApp *pmod, const char *heading)=0;
	virtual void WSHSetAppStatus(WsocksApp *pmod, const char *status)=0;
	virtual const char *WSHSizeStr(WsocksApp *pmod, __int64 bigsize, bool byteUnits);
	virtual int WSHGetAppHead(WsocksApp *pmod, char *heading, int hlen)=0;
	virtual int WSHGetAppStatus(WsocksApp *pmod, char *status, int slen)=0;
    virtual int WSHGetHostBuildDate();
    virtual int WSHGetHostBuildTime();

	// High resolution timer
	virtual ULONGLONG WSHGetTimerCount(WsocksApp *pmod);
	virtual DWORD WSHDiffTimerCount(WsocksApp *pmod, ULONGLONG t1, ULONGLONG t2);

	// System resources
	virtual void WSHCheckSystemResourcesAvailable();
	#ifdef WIN32
	virtual void WSHGetSystemResourcesAvailable(struct _PROCESS_MEMORY_COUNTERS *pProcessMemoryCounters);
	#endif

	// Dynamic port creation
	virtual int WSHSetMaxPorts(WsocksApp *pmod, WSPortType PortType, int NumPorts);
	virtual WSPort *WSHCreatePort(WsocksApp *pmod, WSPortType PortType, int PortNo);
	virtual WSPort *WSHGetPort(WsocksApp *pmod, WSPortType PortType, int PortNo);
	virtual int WSHGetPorts(WsocksApp *pmod, WSPortType PortType, WSPORTLIST& portList);
	virtual int WSHUpdatePort(WsocksApp *pmod, WSPortType PortType, int PortNo);
	virtual int WSHDeletePort(WsocksApp *pmod, WSPortType PortType, int PortNo);
	virtual int WSHLockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send);
	virtual int WSHUnlockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send);

	// Sysmon commands
	virtual int WSHGetTagValues(WsocksApp *pmod, TVMAP& tvmap, const char *parm, int plen);
	virtual string WSHGetValue(WsocksApp *pmod, TVMAP& tvmap, string tag);
	virtual int WSHSetTagValues(WsocksApp *pmod, TVMAP& tvmap, char *parm, int plen);
	virtual int WSHSetValue(WsocksApp *pmod, TVMAP& tvmap, string tag, string value);
	virtual int WSHDelTag(WsocksApp *pmod, TVMAP& tvmap, string tag);
	virtual int WSHSysmonResp(WsocksApp *pmod, int UmrPortNo, int reqid, string cmd, const char *parm, int plen);

	// CON ports
	virtual int WSHSetupConPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenConPort(WsocksApp *pmod, int PortNo);
	virtual int WSHConConnect(WsocksApp *pmod, int PortNo);
	virtual int WSHConSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	virtual int WSHConSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets);
	virtual int WSHConSendBuffNew(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend);
	virtual int WSHCloseConPort(WsocksApp *pmod, int PortNo);
	virtual int WSHBeginUpload(WsocksApp *pmod, const char *spath, const char *DeliverTo, const char *OnBehalfOf,
							   int rid, const char *fpath, WSPortType PortType, int PortNo, 
							   LONGLONG begin);
	virtual int WSHSendFile(WsocksApp *pmod, const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
						    int rid, const char *fpath, WSPortType PortType, int PortNo,
							LONGLONG begin, LONGLONG rsize, LONGLONG tsize, DWORD ssize, bool multi);
	virtual int WSHRecvFile(WsocksApp *pmod, const char *Msg, int MsgLen, WSPortType PortType, int PortNo,
							int *rid, char *fkey, int klen, char *lpath, int llen, char *DeliverTo, WORD dlen, char *OnBehalfOf, WORD blen);

	// USC ports
	virtual int WSHSetupUscPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHCfgUscPort(WsocksApp *pmod, int PortNo);
	virtual int WSHOpenUscPort(WsocksApp *pmod, int PortNo);
	virtual int WSHCloseUscPort(WsocksApp *pmod, int PortNo);

	// USR ports
	virtual int WSHSetupUsrPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHUsrSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	virtual int WSHUsrSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets);
	virtual int WSHCloseUsrPort(WsocksApp *pmod, int PortNo);

	// UMC ports
	virtual int WSHSetupUmcPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenUmcPort(WsocksApp *pmod, int PortNo);
	virtual int WSHCloseUmcPort(WsocksApp *pmod, int PortNo);

	// UMR ports
	virtual int WSHSetupUmrPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHUmrSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	virtual int WSHUmrSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets);
	virtual int WSHCloseUmrPort(WsocksApp *pmod, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CONPORT_1_0& cport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CONPORT_1_1& cport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, USCPORT_1_0& uport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, USCPORT_1_1& uport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, USRPORT_1_0& rport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, USRPORT_1_1& rport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, FILEPORT_1_0& fport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, FILEPORT_1_1& fport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UMCPORT_1_0& uport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UMCPORT_1_1& uport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UMRPORT_1_0& rport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UMRPORT_1_1& rport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CGDPORT_1_0& cport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CGDPORT_1_1& cport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UGCPORT_1_0& uport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UGCPORT_1_1& uport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UGRPORT_1_0& rport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, UGRPORT_1_1& rport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CTOPORT_1_0& oport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CTOPORT_1_1& oport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CTIPORT_1_0& iport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, CTIPORT_1_1& iport, int PortNo);
	//virtual int WSHPortChanged(WsocksApp *pmod, OTHERPORT_1_0& oport, int PortNo);

	// FILE ports
	virtual int WSHMakeLocalDirs(WsocksApp *pmod, const char *lpath);
	#ifdef WS_FILE_SERVER
	virtual int WSHSetupFilePorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenFilePort(WsocksApp *pmod, int PortNo);
	virtual int WSHFileConnect(WsocksApp *pmod, int PortNo);
	virtual int WSHCloseFilePort(WsocksApp *pmod, int PortNo);

	virtual BOOL WSHReplaceFile(WsocksApp *pmod, const char *spath, const char *dpath);
	virtual BOOL WSHTransmitFile(WsocksApp *pmod, LPCTSTR lpLocalFile, LPCTSTR lpRemoteFile, 
								 const FILETIME *lpCreationTime, 
								 const FILETIME *lpLastAccessTime, 
								 const FILETIME *lpLastWriteTime, FileOptions *opts);
	virtual BOOL WSHDownloadFile(WsocksApp *pmod, LPCTSTR lpRemoteFile, LPCTSTR lpLocalFile, FileOptions *opts);
	virtual HANDLE WSHFindFirstFile(WsocksApp *pmod, LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, FileOptions *opts);
	virtual BOOL WSHFindNextFile(WsocksApp *pmod, HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
	virtual BOOL WSHFindClose(WsocksApp *pmod, HANDLE hFindFile);
	virtual int WSHPathFileExists(WsocksApp *pmod, LPCTSTR pszPath, FileOptions *opts);
	virtual int WSHPathIsDirectory(WsocksApp *pmod, LPCTSTR pszPath, FileOptions *opts);
	virtual BOOL WSHCreateDirectory(WsocksApp *pmod, LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes, FileOptions *opts);
	virtual BOOL WSHDeleteFile(WsocksApp *pmod, LPCTSTR pszPath, BOOL bOverrideAttributes, FileOptions *opts);
	virtual int WSHRecurseDelete(WsocksApp *pmod, const char *tdir);
	#endif

	#ifdef WS_GUARANTEED
	// CGD ports
	virtual void WSHGetCgdId(WsocksApp *pmod, int PortNo, GDID *GDId);
	virtual int WSHSetupCgdPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenCgdPort(WsocksApp *pmod, int PortNo);
	virtual int WSHCgdConnect(WsocksApp *pmod, int PortNo);
	virtual int WSHCgdSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo);
	virtual int WSHCloseCgdPort(WsocksApp *pmod, int PortNo);

	// UGC ports
	virtual void WSHGetUgcId(WsocksApp *pmod, int PortNo, char SessionId[20]);
	virtual int WSHSetupUgcPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenUgcPort(WsocksApp *pmod, int PortNo);
	virtual int WSHUgcSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, WORD LineId);
	virtual int WSHCloseUgcPort(WsocksApp *pmod, int PortNo);

	// UGR ports
	virtual int WSHSetupUgrPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHUgrSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, unsigned int GDLogId);
	virtual int WSHUgrSendNGMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo);
	virtual int WSHCloseUgrPort(WsocksApp *pmod, int PortNo);
	#endif

	// CTO ports
	virtual int WSHSetupCtoPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenCtoPort(WsocksApp *pmod, int PortNo);
	virtual int WSHCtoSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets);
	virtual int WSHCloseCtoPort(WsocksApp *pmod, int PortNo);

	// CTI ports
	virtual int WSHSetupCtiPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenCtiPort(WsocksApp *pmod, int PortNo);
	virtual int WSHCloseCtiPort(WsocksApp *pmod, int PortNo);

	// OTHER ports
	virtual int WSHSetupOtherPorts(WsocksApp *pmod, int SMode, int PortNo=-1);
	virtual int WSHOpenOtherPort(WsocksApp *pmod, int PortNo);
	virtual int WSHCloseOtherPort(WsocksApp *pmod, int PortNo);

	// Recording
	virtual int WSHOpenRecording(WsocksApp *pmod, WSRECORDING *wsr, HWND parent, int Type, int PortNo);
	virtual void WSHCloseRecording(WsocksApp *pmod, WSRECORDING *wsr, int Type, int PortNo);
	virtual int WSHRecord(WsocksApp *pmod, WSRECORDING *wsr, const char *buf, int len, BOOL send);
	virtual int WSHCleanRecordings(WsocksApp *pmod, int ndays, const char *extlist, BOOL prompt);

	// Notifications (implementation of WsocksApp upcalls)
	// CON ports
	virtual int WSHDecompressed(WsocksApp *pmod, int PortType,int PortNo,char *DecompBuff,int DecompSize,char PxySrc[PROXYCOMPRESS_MAX]);

	// UMR ports
	virtual int WSHUmrMsgReady(WsocksApp *pmod, int PortNo);
	virtual int WSHUmrStripMsg(WsocksApp *pmod, int PortNo);
	//virtual int WSHUmrSendHealth(WsocksApp *pmod, int PortNo, int MsgDate, int MsgTime);
	//virtual int WSHUmrSetHealth(WsocksApp *pmod, int index, int code, const char *desc);
	virtual int WSHSysmonFeed(WsocksApp *pmod, int UmrPortNo, string feed, const char *buf, int blen);

	#ifdef WS_FILE_SERVER
	// FILE ports
	virtual int WSHFileMsgReady(WsocksApp *pmod, int PortNo);
	virtual int WSHFileStripMsg(WsocksApp *pmod, int PortNo);
	virtual int WSHBeforeFileSend(WsocksApp *pmod, int PortNo);
	#endif

	// Implementation-specific
public:
	// Threads
	int WSHThreadProc(WsocksThread *pthread);
	int WSHThreadMonitor();
#ifdef WIN32
	int WSHThreadCrashed(WsocksThread *pthread, bool restart, EXCEPTION_POINTERS *pException=0);
#else
	int WSHThreadCrashed(WsocksThread *pthread, bool restart);
#endif
	void WSHThreadExit(WsocksThread *pthread, int ecode);
	#ifdef WS_FILE_SERVER
	static LRESULT CALLBACK cbProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	int PIFileTransferThread(WSFileTransfer *wft);
	int FileTransferThread(WSFileTransfer *wft);
	#ifdef WS_FILE_SMPROXY
	int PIFileTransferThread2(WSFileTransfer2 *wft);
	#endif
	#endif
	int WSHCreateThreadMonitor();
	WsocksThread *WSHCreateProcThread(const char *tname, int threadAbortTimeout);
	inline WSTHREADMAP::iterator threadBegin(){return threadMap.begin();}
	inline WSTHREADMAP::iterator threadEnd(){return threadMap.end();}
#ifdef WIN32
	int WSHCleanThreadReports(WsocksApp *pmod, int ndays, int maxsize);
#endif

	// UI support
	char AppStatusStr[256];
	int LastErrorCnt;
	ULONGLONG LastIdle;
	int WSNoOfUserPortsUsed;
	bool hostInitialized;
	const char *__HOST_BUILD_DATE__;
	const char *__HOST_BUILD_TIME__;

	#ifdef WS_OTMP
	THREADPOOL *conPools;
	THREADPOOL *usrPools;
	THREADPOOL *umrPools;
	#endif

	#ifdef WS_OTPP
	int WSHConReadThread(WsocksApp *pmod, int PortNo);
	int WSHUsrReadThread(WsocksApp *pmod, int PortNo);
	int WSHUmrReadThread(WsocksApp *pmod, int PortNo);
	int WSHFileReadThread(WsocksApp *pmod, int PortNo);
	#ifdef WS_GUARANTEED
	int WSHCgdReadThread(WsocksApp *pmod, int PortNo);
	int WSHUgrReadThread(WsocksApp *pmod, int PortNo);
	#endif
	int WSHCtiReadThread(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTMP
	HANDLE WSHPoolConReadThread(WsocksApp *pmod, WSPort *pport);
	int WSHConReadPoolThread(WsocksApp *pmod, THREADPOOL *tpool);
	HANDLE WSHPoolUsrReadThread(WsocksApp *pmod, WSPort *pport);
	int WSHUsrReadPoolThread(WsocksApp *pmod, THREADPOOL *tpool);
	HANDLE WSHPoolUmrReadThread(WsocksApp *pmod, WSPort *pport);
	int WSHUmrReadPoolThread(WsocksApp *pmod, THREADPOOL *tpool);
	#endif
	#endif

	bool SetNetAlias(const char *alias, const char *val);

protected:
	friend class WsocksHost;
	// App loading
	char hostRunDir[MAX_PATH];
	char hostLogDir[MAX_PATH];
	WSAPPMAP appMap;
	APPCONFIGLIST configs;

	int WSHLoadAppIni(const char *appList=0, APPCONFIGLIST *puiList=0);
	int WSHQueueApp(AppConfig *pcfg);
	WsocksApp *WSHLoadApp(AppConfig *pcfg);
	int WSHValidateApp(WsocksApp *pmod, char reason[256]);
	int WSHReloadApp(WsocksApp *pmod);
	WsocksApp *WSHGetApp(const string& aname);
	int WSHDequeueApp(WsocksApp *pmod);
	int WSHUnloadApp(WsocksApp *pmod);
	int WSHCreateMonitorApp();

	// Port creation
	int WSHSetupSocks(WsocksApp *pmod);

	// Date and time
	int WSHDate;
	int WSHTime;
	int WSHlTime;
	char WSHcDate[9];
	char WSHcTime[9];
	int WSHDayOfWeek;
	#ifdef MUX_CRIT_SECTION
	CRITICAL_SECTION dirMutex;
	#else
	HANDLE dirMutex;
	#endif
	int LastTickDiv;
	void WSDateChanging(WsocksApp *pmod);
	void WSTimeChanging(WsocksApp *pmod);
	int WSHDateChange();
	void WSHGetDate_Time(char *cDate,char *cTime,int *wday);
	void WSHGetTime(char *cTime);
	int WSHTimeChange();

	// Operations (implementation of WsocksApp downcalls)
	// Run control
	#ifdef MUX_CRIT_SECTION
	CRITICAL_SECTION gmutexes[8];
	#else
	HANDLE gmutexes[8];
	#endif
	WSTHREADMAP threadMap;
	WSTHREADIDMAP threadIdMap;
	bool stoptm;
	HANDLE tmhnd;

	// Logging
	int WSErrorCount;
	FILE *hostErrFile;
	long hostErrFileDate;
	FILE *hostEveFile;
	long hostEveFileDate;
	void hostFileLogError(char *Error);
	void hostFileLogEvent(char *Event);
	void fileLogError(WsocksApp *pmod, char *Error);
	void fileLogEvent(WsocksApp *pmod, char *Event);

	// Status
	bool CONNSTAT_TOTALS;

	// High resolution timer
	#if defined(WS_MIPS)&&defined(WIN32)
	virtual void WSHCalcMips(WsocksApp *pmod);
	virtual DWORD WSHMipsDiff(WsocksApp *pmod, ULONGLONG start, ULONGLONG stop);
	#endif

	// Dynamic port creation
	WSPort *WSHNewPort(WsocksApp *pmod, WSPortType PortType, int PortNo);

	// Socket primitives
	virtual SOCKET_T WSHSocket(WsocksApp *pmod, WSPortType PortType, int PortNo);
	virtual int WSHBindPort(WSPort *pport, IN_ADDR *iface, int port);
	virtual int WSHListen(WSPort *pport, int backlog);
	virtual SOCKET_T WSHAccept(WsocksApp *pmod, WSPort *pport, WSPortType accPortType, int accPortNo, struct sockaddr FAR * addr, int FAR * addrlen);
	virtual int WSHIoctlSocket(WSPort *pport, long cmd, u_long FAR *argp);
	virtual int WSHSetSockOpt(WSPort *pport, int level, int optname, const char FAR * optval, int optlen);
	virtual int WSHConnectPort(WSPort *pport, IN_ADDR *dest, int port);
	virtual int WSHFinishedConnect(WSPort *pport);
	virtual int WSHSelect(LONGLONG nfds, ws_fd_set FAR * readfds, ws_fd_set FAR * writefds, ws_fd_set FAR *exceptfds, const struct timeval FAR * timeout);
	virtual int WSHSendPort(WSPort *pport, const char *wbuf, int wlen, int flags);
	virtual int WSHRecv(WSPort *pport, char FAR * buf, int len, int flags);
	virtual int WSHSendToPort(WSPort *pport, const char *wbuf, int wlen, int flags, const struct sockaddr FAR *to, int tolen);
	virtual int WSHRecvFrom(WSPort *pport, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen);
	virtual int WSHClosePort(WSPort *pport);
	virtual int WSHGetPeerName(WSPort *pport, SOCKADDR *paddr, int *palen);
	virtual int WSHGetSockName(WSPort *pport, SOCKADDR *laddr, int *lalen);

	// Thunks
	inline int WSHBindPort(SOCKET_T sport, IN_ADDR *iface, int port){
		return WSHBindPort((WSPort *)sport,iface,port);
	}
	inline int WSHListen(SOCKET_T sport, int backlog){
		return WSHListen((WSPort *)sport,backlog);
	}
	inline SOCKET_T WSHAccept(WsocksApp *pApp, SOCKET_T sport, WSPortType accPortType, int accPortNo, struct sockaddr FAR * addr, int FAR * addrlen){
		return WSHAccept(pApp,(WSPort *)sport,accPortType,accPortNo,addr,addrlen);
	}
	inline int WSHIoctlSocket(SOCKET_T sport, long cmd, u_long FAR *argp){
		return WSHIoctlSocket((WSPort *)sport,cmd,argp);
	}
	inline int WSHSetSockOpt(SOCKET_T sport, int level, int optname, const char FAR * optval, int optlen){
		return WSHSetSockOpt((WSPort *)sport,level,optname,optval,optlen);
	}
	inline int WSHConnectPort(SOCKET_T sport, IN_ADDR *dest, int port){
		return WSHConnectPort((WSPort *)sport,dest,port);
	}
	inline int WSHFinishedConnect(SOCKET_T sport){
		return WSHFinishedConnect((WSPort *)sport);
	}
	inline int WSHSendPort(SOCKET_T sport, const char *wbuf, int wlen, int flags){
		return WSHSendPort((WSPort *)sport,wbuf,wlen,flags);
	}
	inline int WSHRecv(SOCKET_T sport, char FAR * buf, int len, int flags){
		return WSHRecv((WSPort *)sport,buf,len,flags);
	}
	inline int WSHSendToPort(SOCKET_T sport, const char *wbuf, int wlen, int flags, const struct sockaddr FAR *to, int tolen){
		return WSHSendToPort((WSPort *)sport,wbuf,wlen,flags,to,tolen);
	}
	inline int WSHRecvFrom(SOCKET_T sport, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen){
		return WSHRecvFrom((WSPort *)sport,buf,len,flags,from,fromlen);
	}
	inline int WSHClosePort(SOCKET_T sport){
		return WSHClosePort((WSPort *)sport);
	}
	inline int WSHGetPeerName(SOCKET_T sport, SOCKADDR *paddr, int *palen){
		return WSHGetPeerName((WSPort *)sport,paddr,palen);
	}
	virtual int WSHGetSockName(SOCKET_T sport, SOCKADDR *laddr, int *lalen){
		return WSHGetSockName((WSPort *)sport,laddr,lalen);
	}

	// CON ports
	void SetReconnectTime(DWORD MinReconnectDelay, DWORD MaxReconnectDelay, DWORD MinReconnectReset,
						  DWORD& ReconnectDelay, DWORD& ReconnectTime, DWORD& ConnectTime);
	int WSHConConnected(WsocksApp *pmod, int PortNo);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHConRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHConRead(WsocksApp *pmod, int PortNo);
	#endif
	int WSHConSend(WsocksApp *pmod, int PortNo, bool flush);
	int _WSHCloseConPort(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitConThreadExit(WsocksApp *pmod, int PortNo);
	#endif
	#ifdef WS_OTMP
	void WSHWaitConThreadPoolsExit();
	#endif

	// USC ports
	int _WSHCloseUscPort(WsocksApp *pmod, int PortNo);
	// USR ports
	int WSHLoadIPAclFile(WsocksApp *pmod, const char *fpath,IPACL **alist);
	int WSHUsrAccept(WsocksApp *pmod, int PortNo);
	int WSHUsrAuthorize(WsocksApp *pmod, int UscPortNo, SOCKADDR_IN *raddr);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHUsrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHUsrRead(WsocksApp *pmod, int PortNo);
	#endif
	int WSHUsrSend(WsocksApp *pmod, int PortNo, bool flush);
	int _WSHCloseUsrPort(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitUsrThreadExit(WsocksApp *pmod, int PortNo);
	#endif
	#ifdef WS_OTMP
	void WSHWaitUsrThreadPoolsExit();
	#endif

	// UMC ports
	int _WSHCloseUmcPort(WsocksApp *pmod, int PortNo);
	// UMR ports
	int WSHUmrAccept(WsocksApp *pmod, int PortNo);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHUmrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHUmrRead(WsocksApp *pmod, int PortNo);
	#endif
	int WSHUmrSend(WsocksApp *pmod, int PortNo, bool flush);
	int _WSHCloseUmrPort(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitUmrThreadExit(WsocksApp *pmod, int PortNo);
	#endif
	#ifdef WS_OTMP
	void WSHWaitUmrThreadPoolsExit();
	#endif

	// FILE ports
	WSFILEMAP fileMap;
	WSFPMAP fpMap;
	#ifdef WS_FILE_SERVER
	bool WSFileTransferReady;
	int WSFileXferInitCnt;
	WSFileTransfer *fileTransfers;
	WSFileRecommit *fileRecommits;

	int WSHFileTransferClean(WsocksApp *pmod);
	int WSHLoadUncommittedFiles(WsocksApp *pmod, const char *cdir);
	int WSHRecommitCheck(WsocksApp *pmod);
	BOOL WSHCancelFile(WsocksApp *pmod, WSFileTransfer *wft);
	BOOL WSHCloseFile(WsocksApp *pmod, HANDLE hObject, BOOL writeBack);
	int WSHCommitFileTransfer(WsocksApp *pmod, int PortNo);
	WSFileTransfer *WSHCreateFileAsync(WsocksApp *pmod, LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
												   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, 
												   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, FileOptions *opts);
	int WSHFileConnected(WsocksApp *pmod, int PortNo);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHFileRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHFileRead(WsocksApp *pmod, int PortNo);
	#endif
	int WSHFileSend(WsocksApp *pmod, int PortNo, bool flush);
	int WSHFileSendBuffNew(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend);
	int WSHFileSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo);
	int WSHFileTransferConnect(WsocksApp *pmod, int PortNo);
	int WSHFileTransferLoop(WsocksApp *pmod);
	WSFileTransfer *WSHfopenAsync(WsocksApp *pmod, const char *filename, const char *mode, FileOptions *opts);
	int WSHGetFileCachePath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs);
	int WSHGetFileCommitPath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs);
	int WSHGetFileKeepPath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs);
	int WSHGetFileLocalPath(WsocksApp *pmod, const char *root, const char *rpath, char *lpath, BOOL makeDirs);
	int WSHGetFileTempPath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs);
	int WSHGetFileTransferPort(WsocksApp *pmod, const char *lpFileName, BOOL showerr);
	WSFileTransfer *WSHOpenFileAsync(WsocksApp *pmod, LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle, FileOptions *opts);
	int WSHRecvFileTransfer(WsocksApp *pmod, int PortNo);
	int WSHRequestFile(WsocksApp *pmod, LPCSTR lpFileName, WSFileTransfer *wft, FileOptions *opts);
	int WSHS5FileConnect(WsocksApp *pmod, int PortNo);
	BOOL WSHSetFileTime(WsocksApp *pmod, HANDLE hFile, const FILETIME *lpCreationTime, const FILETIME *lpLastAccessTime, const FILETIME *lpLastWriteTime);
	int WSHWaitFile(WsocksApp *pmod, WSFileTransfer *wft, int timeout, HANDLE *fhnd);
	int _WSHCloseFilePort(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitFileThreadExit(WsocksApp *pmod, int PortNo);
	#endif
	#endif

	#ifdef WS_GUARANTEED
	// CGD ports
	int _WSHBeforeCgdSend(WsocksApp *pmod, int PortNo);
	int WSHCgdConnected(WsocksApp *pmod, int PortNo);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHCgdRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHCgdRead(WsocksApp *pmod, int PortNo);
	#endif
	int WSHCgdSend(WsocksApp *pmod, int PortNo, bool flush);
	int WSHReadCgdInLog(WsocksApp *pmod, int PortNo);
	int WSHWriteCgdInLog(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *Msg,int PortNo);
	int WSHReadCgdOutLog(WsocksApp *pmod, int PortNo);
	int WSHWriteCgdOutLog(WsocksApp *pmod, char *Block,WORD BlockLen,int PortNo);
	int WSHCgdSendLogin(WsocksApp *pmod, int PortNo);
	int WSHCfgCgdPort(WsocksApp *pmod, int PortNo);
	void WSHResetCgdId(WsocksApp *pmod, int PortNo);
	int WSHCgdSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend);
	int WSHCgdSendBlock(WsocksApp *pmod, char *Block,WORD BlockLen,int PortNo);
	int _WSHCloseCgdPort(WsocksApp *pmod, int PortNo);
	int WSHCgdMsgReady(WsocksApp *pmod, int PortNo);
	int WSHCgdStripMsg(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitCgdThreadExit(WsocksApp *pmod, int PortNo);
	#endif

	// UGC ports
	void WSHResetUgcId(WsocksApp *pmod, int PortNo);
	int WSHReadUgcOutLog(WsocksApp *pmod, int PortNo,GDLINE *GDLine);
	int WSHWriteUgcOutLog(WsocksApp *pmod, GDLINE *GDLine,char *Block,WORD BlockLen,int PortNo);
	int WSHUgcSendGap(WsocksApp *pmod, int PortNo,WORD LineId,int NextOutLogId);
	BOOL WSHUgcPortHasLine(WsocksApp *pmod, int PortNo,WORD LineId);
	int WSHUgcGenPortLine(WsocksApp *pmod, UGCPORT *UgcPort,int PortNo,char *pline,int len);
	int WSHCfgUgcPort(WsocksApp *pmod, int PortNo);
	int _WSHCloseUgcPort(WsocksApp *pmod, int PortNo);

	// UGR ports
	int _WSHBeforeUgrSend(WsocksApp *pmod, int PortNo);
	int WSHUgrAccept(WsocksApp *pmod, int PortNo);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHUgrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHUgrRead(WsocksApp *pmod, int PortNo);
	#endif
	int WSHUgrSend(WsocksApp *pmod, int PortNo, bool flush);
	int WSHReadUgrInLog(WsocksApp *pmod, int PortNo,const char *ClientId);
	int WSHWriteUgrInLog(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *Msg,int PortNo);
	int WSHUgrSendBlock(WsocksApp *pmod, char *Block,WORD BlockLen,int PortNo);
	int WSHUgrSendLoginReply(WsocksApp *pmod, int PortNo);
	int WSHUgrAuthorize(WsocksApp *pmod, int PortNo,GDID *GDId);
	int _WSHCloseUgrPort(WsocksApp *pmod, int PortNo);
	int WSHUgrMsgReady(WsocksApp *pmod, int PortNo);
	int WSHUgrStripMsg(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitUgrThreadExit(WsocksApp *pmod, int PortNo);
	#endif

	FILEINDEX *WSHCreateFileIndex(WsocksApp *pmod, const char *fpath, int initsize , FILE *fGDLog);
	void WSHCloseFileIndex(WsocksApp *pmod, FILEINDEX *findex);
	FIDX *WSHAddToFileIndex(WsocksApp *pmod, FILEINDEX *findex, unsigned index, unsigned long offset, unsigned int len);
	BOOL WSHOpenIndexFile(WsocksApp *pmod, FILEINDEX *findex);
	BOOL WSHReadFILEBlocks(WsocksApp *pmod, FILEINDEX *findex, char *pBuffer, unsigned int *pSize, unsigned int FromIndex, unsigned int *pCount);
	BOOL WSHReadFileBlocks(WsocksApp *pmod, FILEINDEX *findex, char *pBuffer, unsigned int *pSize);
	BOOL WSHReadOneFileBlock(WsocksApp *pmod, FILEINDEX *findex, int bidx, char *pBuffer, unsigned int *pSize);
	int WSHRolloverCGD(WsocksApp *pmod, int PortNo,const char *NewSessionId);
	WORD WSHGDPack(WsocksApp *pmod, char **NewBlock,int GDType,WORD MsgID,WORD MsgLen,char *MsgOut,unsigned int NextGDOutLogId,WORD LineId);
	GDACL *WSHParseGDAcl(WsocksApp *pmod, const char *gdcfg);
	#endif

	// CTO ports
	int WSHCtoSend(WsocksApp *pmod, int PortNo);
	int _WSHCloseCtoPort(WsocksApp *pmod, int PortNo);

	// CTI ports
	int WSHCfgCtiPort(WsocksApp *pmod, int PortNo);
	#if defined(WS_OIO)||defined(WS_OTPP)
	int WSHCtiRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes);
	#else
	int WSHCtiRead(WsocksApp *pmod, int PortNo);
	#endif
	int _WSHCloseCtiPort(WsocksApp *pmod, int PortNo);
	#ifdef WS_OTPP
	void _WSHWaitCtiThreadExit(WsocksApp *pmod, int PortNo);
	#endif

	// OTHER ports
	// Recording

	// SOCKS5
	int WSHS5Login(WsocksApp *pmod, int PortNo);
	int WSHS5Connect(WsocksApp *pmod, int PortNo);

	// Encryption
	int mtcompress(char *dest,unsigned int *destLen,const char *source,unsigned int sourceLen);
	int mtuncompress(char *dest,unsigned int *destLen,const char *source,unsigned int sourceLen);
	int WSHResetCryptKeys(WsocksApp *pmod, int PortNo,int PortType);
	void WSHEncrypt(WsocksApp *pmod, char *EncryptBuff,unsigned int *EncryptSize,
					char *DecryptBuff,unsigned int DecryptSize,
					int PortNo,int PortType);
	void WSHDecrypt(WsocksApp *pmod, char *DecryptBuff,unsigned int *DecryptSize,
					char *EncryptBuff,unsigned int EncryptSize,
					int PortNo,int PortType);

	// System resources
#ifdef WIN32
	MEMORYSTATUS WSMemStat;
	unsigned int WSMemPersUsedWarnLevel;
	unsigned int WSMemAvailWarnLevel;
	int LastTimeWeCheckedDiskSpace;
	HANDLE thisProc;
	HMODULE psapi; 
	BOOL(WINAPI *pGetProcessMemoryInfo)(HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD);
	PROCESS_MEMORY_COUNTERS WSProcessMemoryCounters;
	int WSDiskSpaceAvailable;
#endif

	// Loopback
	#ifdef LBAMUX_CRIT_SECTION
	CRITICAL_SECTION lbaMutex;
	#else
	HANDLE lbaMutex;
	#endif
	LBSMAP lbsMap;
	LBCLIST lbcList;
	int LBConnect(WsocksApp *pmod, WSPort *pport);

	// I/O completion ports
	#ifdef WS_OIO
	HANDLE ovlMutex;
	OVERLAPBLOCK *ovlBlocks;
	WSOVERLAPPED *freeOvlList;

	int AllocOverlapBlock();
	int InitOverlap();
	int CleanupOverlap();
	WSOVERLAPPED *AllocOverlap(WSOVERLAPPED **pendOvlList);
	int FreeOverlap(WSOVERLAPPED **ppendOvlList, WSOVERLAPPED *povl);
	int WSHWaitOverlapCancel(WsocksApp *pmod);

	int WSHConIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	int WSHUsrIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	int WSHUmrIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	#ifdef WS_FILE_SERVER
	int WSHFileIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	#endif
	#ifdef WS_GUARANTEED
	int WSHCgdIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	int WSHUgrIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	#endif
	int WSHCtiIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl=0);
	int WSHSendNonBlock(WsocksApp *pmod, int PortType, int PortNo, SOCKET_T socket, LPVOID buffer, DWORD bytesToWrite);
	int WSHSendToNonBlock(WsocksApp *pmod, int PortType, int PortNo, SOCKET_T socket, LPVOID buffer, DWORD bytesToWrite, SOCKADDR *taddr, int tlen);
	#elif defined(WS_OTPP)
#ifdef WIN32
	#ifdef OVLMUX_CRIT_SECTION
	CRITICAL_SECTION ovlMutex;
	#else
	HANDLE ovlMutex;
	#endif
	OVERLAPBLOCK *ovlBlocks;
	WSOVERLAPPED *freeOvlList;
#endif

	int AllocOverlapBlock();
	int InitOverlap();
	int CleanupOverlap();
	//WSOVERLAPPED *AllocOverlap(WSOVERLAPPED **pendOvlList);
	WSOVERLAPPED *AllocOverlap(WSOVERLAPPED **pendOvlBeg, WSOVERLAPPED **pendOvlEnd);
	//int FreeOverlap(WSOVERLAPPED **ppendOvlList, WSOVERLAPPED *povl);
	int FreeOverlap(WSOVERLAPPED **ppendOvlBeg, WSOVERLAPPED **ppendOvlEnd, WSOVERLAPPED *povl);

	int WSHSendNonBlock(WsocksApp *pmod, int PortType, int PortNo, SOCKET_T socket, LPVOID buffer, DWORD bytesToWrite);
	int WSHSendToNonBlock(WsocksApp *pmod, int PortType, int PortNo, SOCKET_T socket, LPVOID buffer, DWORD bytesToWrite, SOCKADDR *taddr, int tlen);
	//int WSHFinishOverlapSend(WsocksApp *pmod, SOCKET socket, WSOVERLAPPED **pendOvlList);
	int WSHFinishOverlapSend(WsocksApp *pmod, SOCKET socket, WSOVERLAPPED **pendOvlBeg, WSOVERLAPPED **pendOvlEnd);
	#endif

#ifdef _DEBUG
	// For debugging deadlocks
	//static int mcnt;
	//static HANDLE chkmux;
	DWORD WaitForSingleObject(HANDLE hMutex, DWORD dwMilliseconds);
	BOOL ReleaseMutex(HANDLE hMutex);
	void DeleteMutex(HANDLE hMutex);
#endif
	#ifdef MUX_CRIT_SECTION
	CRITICAL_SECTION lockMux;
	#else
	HANDLE lockMux; // a mutex to protect port mutex reference counts
	#endif
	int LockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send, DWORD timeout=INFINITE);
	int UnlockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send);
	void DeletePortMutex(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send);

	// Loopback sysmon proxy
	AppConfig *lbmCfg;
	LBMonitorNotify *lbmon;
	// SysCmdNotify
	SysCmdCodec ccodec;
	//SysFeedCodec fcodec;
	void NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata);
	void NotifyResponse(int reqid, string cmd, const char *parm, int plen, void *udata);

	bool bAppInProcessOfExiting;

	// Misc app notifications
	int WSHOpenThreadReportFile(WsocksThread *pthread, WsocksApp *pmod);
	int WSHCloseThreadReportFile(WsocksThread *pthread, WsocksApp *pmod);
	int WSHBackupApp(WsocksApp *pmod, char bpath[MAX_PATH], char emsg[256], bool overwrite);
	int WSHRestoreApp(WsocksApp *pmod, char bpath[MAX_PATH], char emsg[256]);

	// Network aliases
	map<string,string> netmap;

	#ifdef WSOCKS_SSL
	string SSLPW;
	string CAFILE;
	string CERTFILE;
	class SSLGateway *sslg;
	// SSLGatewayNotify
	void SSLLogError(const char *format, ...);
	void SSLLogEvent(const char *format, ...);
	#endif

	#ifdef WAITABLE_TIMERS
	HANDLE oneSecTimer;
	#endif
};

extern void strLogError(char *Error);
extern void strLogEvent(const char *Event, bool display);
extern void WSbtrim(char *Value);
#ifdef WIN32
extern bool RectsOverlap(RECT& r1, RECT& r2);
#endif

#pragma pack(pop)

#endif//WSOCKSHOSTIMPL_H

