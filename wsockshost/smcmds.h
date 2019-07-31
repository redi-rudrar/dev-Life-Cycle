
#ifndef _SMCMDS_H
#define _SMCMDS_H
// Symon common commands handler for LBMonitor and SMProxy
#include "wsocksapi.h"
#include "sysmonc.h"

class SMCmdsNotify
{
public:
	virtual int SMCSendMsg(WORD MsgID, WORD MsgLen, char *MsgOut, WSPortType PortType, int PortNo)=0;
	virtual int SMCBeginUpload(const char *spath, const char *DeliverTo, const char *OnBehalfOf,
							   int rid, const char *fpath, WSPortType PortType, int PortNo, 
							   LONGLONG begin=0)=0;
	virtual int SMCSendFile(const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
						    int rid, const char *fpath, WSPortType PortType, int PortNo,
							LONGLONG begin=0, LONGLONG rsize=0, LONGLONG tsize=0, DWORD ssize=0, bool multi=false)=0;
	virtual bool SMCSendFinished(WSPortType PortType, int PortNo, int threshold)=0;
};
class SMCmds
{
public:
	// Two-phase init
	SMCmds();
	~SMCmds();
	int Init(SMCmdsNotify *cnotify, SysCmdCodec *ccodec);
	int Shutdown();

	// Call this to handle SysCmdNotify requests for common commands
	int SMCNotifyRequest(int reqid, string cmd, const char *parm, int plen, WSPortType PortType, int PortNo);
	void CheckThreadedCmd();

	// Threaded commands
	struct WSThreadedCmdInfo
	{
		int cmdid;
		string cmd;
		int reqid;
		char *parm;
		int plen;
		WSPortType PortType;
		int PortNo;
		string dtid;
		string oboi;

		DWORD tstart;
		DWORD timeout;
		bool doneCommandHandling;
        bool doneTransferring;
		string fileKey;
		char fpath[MAX_PATH];
		LONGLONG fileBegin;
		LONGLONG fileOffset;
		LONGLONG fileBytes;
        DWORD bytesMaxRate;
		char resp[1024],*rptr;
        SMCmds *pSMCmds;

		//class WsocksHostImpl *phost;
		SMCmds *pmod;
		DWORD tid;
		#ifdef WIN32
		HANDLE thnd;
		#else
		pthread_t thnd;
		#endif

		#ifdef WIN32
		// Wildcard file transfer
		bool multiFile;
		char mdir[MAX_PATH];
		int mdlen;
		bool recurse;
		char fileList[MAX_PATH];
		#endif
	};
	int WSHThreadedCmdHandler(SMCmds *pmod, WSThreadedCmdInfo *pdata);
    #ifdef WIN32
    static DWORD WINAPI BootThreadedCmd(LPVOID arg);
    #else
    static void *BootThreadedCmd(void *arg);
    #endif

protected:
	SMCmdsNotify *cnotify;
	SysCmdCodec *ccodec;

	// Convenience
	int SMUSysmonResp(int UmrPortNo, int reqid, string cmd, const char *parm, int plen);
	// Threaded commands
	typedef list<WSThreadedCmdInfo *> WSTCMDLIST;
	WSTCMDLIST WSThreadedCmdList;
	int BeginThreadedCmd(int cmdid, string cmd, int reqid, string dtid, string oboi, 
						 const char *parm, int plen, 
						 WSPortType PortType, int PortNo, DWORD timeout);
    #ifdef WIN32
	static int MultiFileSend(WSThreadedCmdInfo *pdata, const char *fdir, const char *fmatch, bool recurse, DWORD& msprev, DWORD& sentLastFile);
	static int FileListSend(WSThreadedCmdInfo *pdata, const char *fileList);
	#endif
};

#endif//_SMCMDS_H
