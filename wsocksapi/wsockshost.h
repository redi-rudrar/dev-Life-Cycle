
#ifndef WSOCKSHOST_H
#define WSOCKSHOST_H

#pragma pack(push,1)

// Purse interface class that defines the up and downcalls between WsocksApp and WsocksHost
class WsocksHost
{
public:
	// These must be called by the subclass host app
	// Two-phase initialization
	virtual int WSHInitHost(const char *runPath)=0;
	//virtual int WSHInitSingleHost(WsocksApp *pmod, const char *aname)=0;
	virtual int WSHCleanupHost()=0;

	// Operations (downcalls)
	// This should be called once per second (no longer required to be called with waitable timer)
	virtual void WSHCheckTime(void)=0;
	// This should be called when idle
	virtual int WSHIdle(ULONGLONG tstop)=0;
	// This should be called when a connection times out and is bounced
	//virtual void WSHLogSendTimeout(int PortType, int PortNo)=0;
	// This should be called once per second
	//virtual void WSHSendMonitorData(WsocksApp *pmod, int PortNo)=0;

	// These must be implemented to log the host app events
	virtual void WSHostLogEvent(const char *format,...)=0;
	virtual void WSHostLogError(const char *format,...)=0;
	virtual void WSHostLogEventVA(const char *format, va_list& alist)=0;
	virtual void WSHostLogErrorVA(const char *format, va_list& alist)=0;
	virtual int WSHCloseApp(WsocksApp *pmod)=0;
	// Network aliasing
	virtual bool WSHTranslateAlias(char *ipstr, int len)=0;
	#ifdef WIN32
	// Debugging
	virtual int WSHGenerateDump(WsocksApp *pmod, DWORD tid, EXCEPTION_POINTERS *pException=0)=0;
	#endif

	// Notifications (upcalls)
	// App loading
	virtual int WSHAppLoading(WsocksApp *pmod)=0;
	virtual int WSHAppLoadFailed(WsocksApp *pmod)=0;
	virtual int WSHAppLoaded(WsocksApp *pmod)=0;
	virtual int WSHAppUnloaded(WsocksApp *pmod)=0;

	// Port creation
	virtual int WSHPortCreated(WsocksApp *pmod, WSPort *pport)=0;
	virtual int WSHPortModified(WsocksApp *pmod, WSPort *pport)=0;
	virtual bool WSHIsPortAvailable(unsigned short bport)=0;

	// Date and time
	virtual long WSHDiffTime(long tnew, long told)=0;

	// System resources
	virtual void WSHCheckSystemResourcesAvailable()=0;
	#ifdef WIN32
	virtual void WSHGetSystemResourcesAvailable(struct _PROCESS_MEMORY_COUNTERS *pProcessMemoryCounters)=0;
	#endif

	// Logging
	virtual int WSHLoggedEvent(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedError(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedDebug(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedRetry(WsocksApp *pmod, const char *estr)=0;
	virtual int WSHLoggedRecover(WsocksApp *pmod, const char *estr)=0;
	virtual void WSHLogEventStr(WsocksApp *pmod, const char *str)=0;
	virtual void WSHLogErrorStr(WsocksApp *pmod, const char *str)=0;
	virtual int WSHHostLoggedEvent(const char *estr)=0;
	virtual int WSHHostLoggedError(const char *estr)=0;


	// Operations (implementation of WsocksApp downcalls)
	// Run control
	virtual int WSHPreInitSocks(WsocksApp *pmod)=0;
	virtual int WSHInitSocks(WsocksApp *pmod)=0;
	virtual int WSHSuspend(WsocksApp *pmod)=0;
	virtual int WSHResume(WsocksApp *pmod)=0;
	virtual int WSHCloseSocks(WsocksApp *pmod)=0;
	virtual int WSHExitApp(WsocksApp *pmod, int ecode)=0;
	virtual int WSHExitHost(int ecode)=0;
	virtual int WSHSyncLoop(WsocksApp *pmod)=0;
	#ifdef WS_OIO
	virtual int WSHIocpLoop(WsocksApp *pmod, DWORD timeout)=0;
	#else
	virtual int WSHAsyncLoop(WsocksApp *pmod)=0;
	#endif
	virtual int WSHWaitMutex(int mutexIdx, int timeout)=0;
	virtual int WSHReleaseMutex(int mutexIdx)=0;
	virtual int WSHBusyThreadReport(WsocksApp *pmod, DWORD estEndTime)=0;
	virtual int WSHRestart(WsocksApp *pmod, int timeout)=0;
	static int WSHCreateGuiHost(WsocksApp *pmod, AppConfig *pcfg, bool lbmonitor);
	static int WSHDestroyGuiHost(WsocksApp *pmod);

	// Logging
	virtual void WSHResetErrorCnt(WsocksApp *pmod)=0;
	virtual void WSHLogEvent(WsocksApp *pmod, const char *format,...)=0;
	virtual void WSHLogError(WsocksApp *pmod, const char *format,...)=0;
	virtual void WSHLogDebug(WsocksApp *pmod, const char *format,...)=0;
	virtual void WSHLogRetry(WsocksApp *pmod, const char *format,...)=0;
	virtual void WSHLogRecover(WsocksApp *pmod, const char *format,...)=0;
	virtual void WSHLogEventVA(WsocksApp *pmod, const char *format, va_list& alist)=0;
	virtual void WSHLogErrorVA(WsocksApp *pmod, const char *format, va_list& alist)=0;
	virtual void WSHLogDebugVA(WsocksApp *pmod, const char *format, va_list& alist)=0;
	virtual void WSHLogRetryVA(WsocksApp *pmod, const char *format, va_list& alist)=0;
	virtual void WSHLogRecoverVA(WsocksApp *pmod, const char *format, va_list& alist)=0;

	// Status
	virtual void WSHSetAppHead(WsocksApp *pmod, const char *heading)=0;
	virtual void WSHSetAppStatus(WsocksApp *pmod, const char *status)=0;
	virtual const char *WSHSizeStr(WsocksApp *pmod, __int64 bigsize, bool byteUnits)=0;
	virtual int WSHGetAppHead(WsocksApp *pmod, char *heading, int hlen)=0;
	virtual int WSHGetAppStatus(WsocksApp *pmod, char *status, int slen)=0;
    virtual int WSHGetHostBuildDate()=0;
    virtual int WSHGetHostBuildTime()=0;

	// High resolution timer
	virtual ULONGLONG WSHGetTimerCount(WsocksApp *pmod)=0;
	virtual DWORD WSHDiffTimerCount(WsocksApp *pmod, ULONGLONG t1, ULONGLONG t2)=0;

	// Dynamic port creation
	virtual int WSHSetMaxPorts(WsocksApp *pmod, WSPortType PortType, int NumPorts)=0;
	virtual WSPort *WSHCreatePort(WsocksApp *pmod, WSPortType PortType, int PortNo)=0;
	virtual WSPort *WSHGetPort(WsocksApp *pmod, WSPortType PortType, int PortNo)=0;
	virtual int WSHGetPorts(WsocksApp *pmod, WSPortType PortType, WSPORTLIST& portList)=0;
	virtual int WSHUpdatePort(WsocksApp *pmod, WSPortType PortType, int PortNo)=0;
	virtual int WSHDeletePort(WsocksApp *pmod, WSPortType PortType, int PortNo)=0;
	virtual int WSHLockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send)=0;
	virtual int WSHUnlockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send)=0;

	// Sysmon commands
	virtual int WSHGetTagValues(WsocksApp *pmod, TVMAP& tvmap, const char *parm, int plen)=0;
	virtual string WSHGetValue(WsocksApp *pmod, TVMAP& tvmap, string tag)=0;
	virtual int WSHSetTagValues(WsocksApp *pmod, TVMAP& tvmap, char *parm, int plen)=0;
	virtual int WSHSetValue(WsocksApp *pmod, TVMAP& tvmap, string tag, string value)=0;
	virtual int WSHDelTag(WsocksApp *pmod, TVMAP& tvmap, string tag)=0;
	virtual int WSHSysmonResp(WsocksApp *pmod, int UmrPortNo, int reqid, string cmd, const char *parm, int plen)=0;

	// Socket primitives
	//virtual SOCKET WSHSocket(WsocksApp *pmod, WSPortType PortType, int PortNo)=0;
	//virtual int WSHBindPort(WSPort *pport, IN_ADDR *iface, int port)=0;
	//virtual int WSHListen(WSPort *pport, int backlog)=0;
	//virtual SOCKET WSHAccept(WsocksApp *pmod, WSPort *pport, WSPortType accPortType, int accPortNo, struct sockaddr FAR * addr, int FAR * addrlen)=0;
	//virtual int WSHIoctlSocket(WSPort *pport, long cmd, u_long FAR *argp)=0;
	//virtual int WSHSetSockOpt(WSPort *pport, int level, int optname, const char FAR * optval, int optlen)=0;
	//virtual int WSHConnectPort(WSPort *pport, IN_ADDR *dest, int port)=0;
	//virtual int WSHFinishedConnect(WSPort *pport)=0;
	//virtual int WSHSelect(int nfds, fd_set FAR * readfds, fd_set FAR * writefds, fd_set FAR *exceptfds, const struct timeval FAR * timeout)=0;
	//virtual int WSHSendPort(WSPort *pport, const char *wbuf, int wlen, int flags)=0;
	//virtual int WSHRecv(WSPort *pport, char FAR * buf, int len, int flags)=0;
	//virtual int WSHSendToPort(WSPort *pport, const char *wbuf, int wlen, int flags, const struct sockaddr FAR *to, int tolen)=0;
	//virtual int WSHRecvFrom(WSPort *pport, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen)=0;
	//virtual int WSHClosePort(WSPort *pport)=0;
	//virtual int WSHGetPeerName(WSPort *pport, SOCKADDR *paddr, int *palen)=0;
	//virtual int WSHGetSockName(WSPort *pport, SOCKADDR *laddr, int *lalen)=0;

	// CON ports
	virtual int WSHSetupConPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenConPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHConConnect(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHConSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)=0;
	virtual int WSHConSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)=0;
	virtual int WSHConSendBuffNew(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend)=0;
	virtual int WSHCloseConPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHBeginUpload(WsocksApp *pmod, const char *spath, const char *DeliverTo, const char *OnBehalfOf,
							   int rid, const char *fpath, WSPortType PortType, int PortNo, 
							   LONGLONG begin)=0;
	virtual int WSHSendFile(WsocksApp *pmod, const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
						    int rid, const char *fpath, WSPortType PortType, int PortNo,
							LONGLONG begin, LONGLONG rsize, LONGLONG tsize, DWORD ssize, bool multi)=0;
	virtual int WSHRecvFile(WsocksApp *pmod, const char *Msg, int MsgLen, WSPortType PortType, int PortNo,
							int *rid, char *fkey, int klen, char *lpath, int llen, char *DeliverTo, WORD dlen, char *OnBehalfOf, WORD blen)=0;

	// USC ports
	virtual int WSHSetupUscPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHCfgUscPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHOpenUscPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCloseUscPort(WsocksApp *pmod, int PortNo)=0;

	// USR ports
	virtual int WSHSetupUsrPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHUsrSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)=0;
	virtual int WSHUsrSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)=0;
	virtual int WSHCloseUsrPort(WsocksApp *pmod, int PortNo)=0;

	// UMC ports
	virtual int WSHSetupUmcPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenUmcPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCloseUmcPort(WsocksApp *pmod, int PortNo)=0;

	// UMR ports
	virtual int WSHSetupUmrPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHUmrSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)=0;
	virtual int WSHUmrSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)=0;
	virtual int WSHCloseUmrPort(WsocksApp *pmod, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CONPORT_1_0& cport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CONPORT_1_1& cport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, USCPORT_1_0& uport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, USCPORT_1_1& uport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, USRPORT_1_0& rport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, USRPORT_1_1& rport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, FILEPORT_1_0& fport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, FILEPORT_1_1& fport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UMCPORT_1_0& uport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UMCPORT_1_1& uport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UMRPORT_1_0& rport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UMRPORT_1_1& rport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CGDPORT_1_0& cport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CGDPORT_1_1& cport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UGCPORT_1_0& uport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UGCPORT_1_1& uport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UGRPORT_1_0& rport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, UGRPORT_1_1& rport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CTOPORT_1_0& oport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CTOPORT_1_1& oport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CTIPORT_1_0& iport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, CTIPORT_1_1& iport, int PortNo)=0;
	//virtual int WSHPortChanged(WsocksApp *pmod, OTHERPORT_1_0& oport, int PortNo)=0;

	// FILE ports
	virtual int WSHMakeLocalDirs(WsocksApp *pmod, const char *lpath)=0;
	#ifdef WS_FILE_SERVER
	virtual int WSHSetupFilePorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenFilePort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHFileConnect(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCloseFilePort(WsocksApp *pmod, int PortNo)=0;

	virtual BOOL WSHReplaceFile(WsocksApp *pmod, const char *spath, const char *dpath)=0;
	virtual BOOL WSHTransmitFile(WsocksApp *pmod, LPCTSTR lpLocalFile, LPCTSTR lpRemoteFile, 
						const FILETIME *lpCreationTime, 
						const FILETIME *lpLastAccessTime, 
						const FILETIME *lpLastWriteTime, FileOptions *opts)=0;
	virtual BOOL WSHDownloadFile(WsocksApp *pmod, LPCTSTR lpRemoteFile, LPCTSTR lpLocalFile, FileOptions *opts)=0;
	virtual HANDLE WSHFindFirstFile(WsocksApp *pmod, LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, FileOptions *opts)=0;
	virtual BOOL WSHFindNextFile(WsocksApp *pmod, HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)=0;
	virtual BOOL WSHFindClose(WsocksApp *pmod, HANDLE hFindFile)=0;
	virtual int WSHPathFileExists(WsocksApp *pmod, LPCTSTR pszPath, FileOptions *opts)=0;
	virtual int WSHPathIsDirectory(WsocksApp *pmod, LPCTSTR pszPath, FileOptions *opts)=0;
	virtual BOOL WSHCreateDirectory(WsocksApp *pmod, LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes, FileOptions *opts)=0;
	virtual BOOL WSHDeleteFile(WsocksApp *pmod, LPCTSTR pszPath, BOOL bOverrideAttributes, FileOptions *opts)=0;
	virtual int WSHRecurseDelete(WsocksApp *pmod, const char *tdir)=0;
	#endif

	#ifdef WS_GUARANTEED
	// CGD ports
	virtual void WSHGetCgdId(WsocksApp *pmod, int PortNo, GDID *GDId)=0;
	virtual int WSHSetupCgdPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenCgdPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCgdConnect(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCgdSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo)=0;
	virtual int WSHCloseCgdPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCgdMsgReady(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCgdStripMsg(WsocksApp *pmod, int PortNo)=0;

	// UGC ports
	virtual void WSHGetUgcId(WsocksApp *pmod, int PortNo, char SessionId[20])=0;
	virtual int WSHSetupUgcPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenUgcPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHUgcSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, WORD LineId)=0;
	virtual int WSHCloseUgcPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCfgUgcPort(WsocksApp *pmod, int PortNo)=0;

	// UGR ports
	virtual int WSHSetupUgrPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHUgrSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, unsigned int GDLogId)=0;
	virtual int WSHUgrSendNGMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo)=0;
	virtual int WSHCloseUgrPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHUgrMsgReady(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHUgrStripMsg(WsocksApp *pmod, int PortNo)=0;
	#endif

	// CTO ports
	virtual int WSHSetupCtoPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenCtoPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCtoSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)=0;
	virtual int WSHCloseCtoPort(WsocksApp *pmod, int PortNo)=0;

	// CTI ports
	virtual int WSHSetupCtiPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenCtiPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCloseCtiPort(WsocksApp *pmod, int PortNo)=0;

	// OTHER ports
	virtual int WSHSetupOtherPorts(WsocksApp *pmod, int SMode, int PortNo=-1)=0;
	virtual int WSHOpenOtherPort(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHCloseOtherPort(WsocksApp *pmod, int PortNo)=0;

	// Recording
	virtual int WSHOpenRecording(WsocksApp *pmod, WSRECORDING *wsr, HWND parent, int Type, int PortNo)=0;
	virtual void WSHCloseRecording(WsocksApp *pmod, WSRECORDING *wsr, int Type, int PortNo)=0;
	virtual int WSHRecord(WsocksApp *pmod, WSRECORDING *wsr, const char *buf, int len, BOOL send)=0;
	virtual int WSHCleanRecordings(WsocksApp *pmod, int ndays, const char *extlist, BOOL prompt)=0;

	// Notifications (implementation of WsocksApp upcalls)
	// CON ports
	virtual int WSHDecompressed(WsocksApp *pmod, int PortType,int PortNo,char *DecompBuff,int DecompSize,char PxySrc[PROXYCOMPRESS_MAX])=0;

	// UMR ports
	virtual int WSHUmrMsgReady(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHUmrStripMsg(WsocksApp *pmod, int PortNo)=0;
	//virtual int WSHUmrSendHealth(WsocksApp *pmod, int PortNo, int MsgDate, int MsgTime)=0;
	//virtual int WSHUmrSetHealth(WsocksApp *pmod, int index, int code, const char *desc)=0;
	virtual int WSHSysmonFeed(WsocksApp *pmod, int UmrPortNo, string feed, const char *buf, int blen)=0;

	#ifdef WS_FILE_SERVER
	// FILE ports
	virtual int WSHFileMsgReady(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHFileStripMsg(WsocksApp *pmod, int PortNo)=0;
	virtual int WSHBeforeFileSend(WsocksApp *pmod, int PortNo)=0;
	#endif

	// Encryption
	virtual int WSHResetCryptKeys(WsocksApp *pmod, int PortNo,int PortType)=0;

	// Misc app notifications
	virtual int WSHBackupApp(WsocksApp *pmod, char bpath[MAX_PATH], char emsg[256], bool overwrite)=0;
	virtual int WSHRestoreApp(WsocksApp *pmod, char bpath[MAX_PATH], char emsg[256])=0;

	// Show/hide UI window
	virtual int WSHShowWindow(int nCmdShow)=0;
};

#ifdef WIN32
extern BOOL DeleteMutex(HANDLE hMutex);
#else
// DeleteMutex is defined in stdafx.h for Linux
#endif

#pragma pack(pop)

#endif//WSOCKSHOST_H
