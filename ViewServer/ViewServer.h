// ViewServer.h : main header file for the ViewServer DLL
//
#ifndef _VIEWSERVER_H
#define _VIEWSERVER_H

#include "wsocksapi.h"
#include "feedtask.h"
#include "vsorder.h"
#include "mrucache.h"
#include "bfix.h"
#include "ViewServerProto.h"
#include "fixstats.h"
#include "exprtok.h"
#include "vsdist.h"
#include "vsquery.h"
#include "sfix.h"
#include "vsdefs.h"
#ifdef REDIOLEDBCON
#include "vsoledb.h"
#endif
using namespace viewserver;

#if !defined(WIN32)&&!defined(_CONSOLE)
#include <sys/types.h>
#include <dirent.h>
#endif

#ifdef MULTI_DAY_ITEMLOC
typedef set<ITEMLOC> ITEMLOCSET;
#else
typedef set<LONGLONG> ITEMLOCSET;
#endif

//DT9044
#define VSDB_ACCOUNTREC_IND  "$$ACCOUNT"


// An account can be an appsystems.ini-defined account, an AppInstance, or AppSystem
typedef list<class Account*> ACCTLIST;
typedef map<string,Account*> ACCTMAP;
typedef list<string> ACCTMATCHLIST;
class Account
{
public:
	Account()
		:AcctName()
		,AcctType()
		,AcctOrderQty(0)
		,AcctCumQty(0)
		,AcctFillQty(0)
		,adirty(false)
		,plist()
		,mlist()
	#ifdef IQDNS
		,iqptr(0)
	#endif
		,AcctUpdateTime(0)
		,AcctMsgCnt(0)
	{
	}
#ifdef IQDNS
	~Account();
#endif

	//inline const string AcctGetName(){ return AcctName; }
	//inline bool AcctIsDirty(){ return adirty; }
	//inline ACCTLIST::iterator pbegin(){ return plist.begin(); }
	//inline ACCTLIST::iterator pend(){ return plist.end(); }
	//inline ACCTMATCHLIST::iterator mbegin(){ return mlist.begin(); }
	//inline ACCTMATCHLIST::iterator mend(){ return mlist.end(); }
	//inline LONGLONG AcctGetOrderQty(){ return AcctOrderQty; }
	//inline LONGLONG AcctGetCumQty(){ return AcctCumQty; }
	//inline LONGLONG AcctGetFillQty(){ return AcctFillQty; }

	//inline void AcctSetName(const char *aname)
	//{
	//	if(!strcmp(AcctName.c_str(),aname))
	//	{
	//		AcctName=aname; adirty=true;
	//	}
	//}
	//inline void SetDirty(bool tf)
	//{
	//	adirty=tf;
	//}
	inline void AcctAddParent(Account *aparent)
	{
		for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
		{
			if((*pit)==aparent)
				return;
		}
		plist.push_back(aparent);
	}
	inline void AcctRemParent(Account *aparent)
	{
		for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
		{
			if((*pit)==aparent)
			{
				#ifdef WIN32
				pit=plist.erase(pit);
				#else
				plist.erase(pit++);
				#endif
				return;
			}
		}
	}
	inline void AcctAddOrderQty(int qty)
	{
		if(qty!=0) // pos or neg
		{
			AcctOrderQty+=qty; adirty=true;
			for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
				(*pit)->AcctAddOrderQty(qty);
		}
	}
	inline void AcctAddCumQty(int qty)
	{
		if(qty!=0) // pos or neg
		{
			AcctCumQty+=qty; adirty=true;
			for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
				(*pit)->AcctAddCumQty(qty);
		}
	}
	inline void AcctAddFillQty(int qty)
	{
		if(qty!=0) // pos or neg
		{
			AcctFillQty+=qty; adirty=true;
			for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
				(*pit)->AcctAddFillQty(qty);
		}
	}
	inline void AcctSetUpdateTime(int wstime)
	{
		if(wstime!=0)
		{
			AcctUpdateTime=wstime; adirty=true;
			for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
				(*pit)->AcctSetUpdateTime(wstime);
		}
	}
	inline void AcctAddMsgCnt(int qty)
	{
		if(qty!=0) // pos or neg
		{
			AcctMsgCnt+=qty; adirty=true;
			for(ACCTLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
				(*pit)->AcctAddMsgCnt(qty);
		}
	}

public:
	string AcctName;
	string AcctType;
	LONGLONG AcctOrderQty;
	LONGLONG AcctCumQty;
	LONGLONG AcctFillQty;
#ifdef IQDNS
	void *iqptr;
#endif
	int AcctUpdateTime;
	DWORD AcctMsgCnt;

	bool adirty;
	ACCTLIST plist;
	ACCTMATCHLIST mlist;
};

class AppInstance
	:public Account
{
public:
	AppInstance()
		:iname()
		,oset()
		,asys(0)
	#ifdef IQDNS
		,iqptr(0)
	#endif
	{
	}
#ifdef IQDNS
	~AppInstance();
#endif

	string iname;
	ITEMLOCSET oset;
	AppSystem *asys;
#ifdef IQDNS
	void *iqptr;
#endif
};
typedef map<string,AppInstance*> APPINSTMAP;

class AppSystem
	:public Account
{
public:
	AppSystem()
		:sname()
		,skey()
		,imap()
		,odb()
		,aibrowse(0)
	#ifdef IQDNS
		,iqptr(0)
	#endif
	#ifdef MULTI_DAY_HIST
		,odblist()
	#endif
	{
	}
	~AppSystem()
	{
	#ifdef IQDNS
		FreeIqptr();
	#endif
		if(aibrowse)
		{
			delete aibrowse; aibrowse=0;
		}
		for(APPINSTMAP::iterator iit=imap.begin();iit!=imap.end();iit++)
		{
			AppInstance *ainst=iit->second;
			delete ainst;
		}
		imap.clear();
	}

	AppInstance *CreateInstance(const char *iname, int& icnt)
	{
		APPINSTMAP::iterator iit=imap.find(iname);
		if(iit!=imap.end())
			return iit->second;
		AppInstance *nainst=new AppInstance;
		nainst->iname=iname;
		nainst->asys=this;
		imap[iname]=nainst; icnt++;
		return nainst;
	}
	int PregenBrowse()
	{
		if(aibrowse) delete aibrowse;
		int ninst=(int)imap.size();
		aibrowse=new const char *[ninst +1];
		int i=0;
		aibrowse[i++]=skey.c_str();
		for(APPINSTMAP::iterator iit=imap.begin();iit!=imap.end();iit++)
			aibrowse[i++]=iit->second->iname.c_str();
		return 0;
	}

	string sname;			// Name
	string skey;
	APPINSTMAP imap;		// Instances
	OrderDB odb;
	const char **aibrowse;
#ifdef IQDNS
	void *iqptr;
	void FreeIqptr();
#endif
#ifdef MULTI_DAY_HIST
	list<OrderDB *> odblist;
	OrderDB *GetDB(int wsdate);
#endif
};
typedef list<AppSystem*> APPSYSLIST;
typedef map<string,AppSystem*> APPSYSMAP;

typedef pair<string,string> USERENTITLEMENT;
typedef list<USERENTITLEMENT> USERENTITLELIST;

#ifdef IQSMP
struct APPINSTMARKER
{
	DWORD marker;
	char AppInstID[20];
	int buildDate;
	int buildTime;
}; // 32 bytes
#endif

// Gummy Bear Server
class ViewServer 
	:public WsocksApp		// TCP/IP
	,public OrderDBNotify	// Order database
	,public VSCNotify		// Client query
	,public ExprTokNotify	// Parse error
	,public VSDistNotify	// Distributed update
	,public DropClientNotify// Drop copy client
	#ifdef REDIOLEDBCON
	,public CQueryChangeEventHandler
	#endif
{
public:
	ViewServer();
	~ViewServer();

	// Protocols we support
	enum Protocol
	{
		PROTO_UNKNOWN,
		PROTO_FIX,			// $FIX$: Real FIX: 0x01 delimiter, no LF
		PROTO_MLFIXLOGS,	// $MLFIXLOGS$: ML log files with ';' delimiter and LF
		PROTO_VSCLIENT,		// $VSCLIENT$: 'VSCodec' client query
		PROTO_MLPUBHUB,		// $MLPUBHUB$: ML log files with 0x01 delimiter and LF
		PROTO_VSDIST,		// $VSDIST$: 'VSDistCodec' server journal
		PROTO_ITF,			// $ITF$: ITF Fix encoding
		PROTO_DSV,			// Delimiter-separated value
		PROTO_IQOMS,		// $IQOMS$: InstaQuote ML OMS .fix file
		PROTO_IQFIXS,		// $IQFIXS$: InstaQuote FIX server
		PROTO_TJMLOG,		// $TJMLOG$: TJM sample log files
		PROTO_FIXDROP,		// $FIXDROP$: FIX sessioned drop copy
		PROTO_CLDROP,		// $CLDROP$: CLSERVER drop copy
		PROTO_TBACKUP,		// $TBACKUP$: TRADER backup drop
		PROTO_FULLLOG,		// $FULLLOG$: TRADER fulllog playback
		PROTO_TJM,			// $TJM$: TJM production log files
		PROTO_MLPH_EMEA,	// $MLPH_EMEA$: ML log files with 0x01 delimiter followed by space
		PROTO_DNS_REPL,		// $DNS_REPL$: DNS replication
		PROTO_DNS_MGR,		// $DNS_MGR$: DNS manager (iqclient load balance) connection
		PROTO_CLBACKUP,		// $CLBACKUP$: CLSERVER backup
		PROTO_MLPH_CC,		// $MLPH_CC$: ML log files with ';' delimiter followed by space
		PROTO_DAT, 			// DT9044: Dat value
		PROTO_FIXS_PLACELOG,// $FIXS_PLACELOG$
		PROTO_FIXS_CXLLOG,	// $FIXS_CXLLOG$
		PROTO_FIXS_RPTLOG,	// $FIXS_RPTLOG$
		PROTO_FIXS_EVELOG,	// $FIX_EVELOG$
		PROTO_CLS_EVENTLOG,	// $CLS_EVENTLOG$
		PROTO_TRADER_PLACELOG,// $TRADER_PLACELOG$
		PROTO_TRADER_CXLLOG,// $TRADER_CXLLOG$
		PROTO_TRADER_RPTLOG,// $TRADER_RPTLOG$
		PROTO_TRADER_EVELOG,// $TRADER_EVELOG$
		PROTO_RTOATS,		// $RTOATS$
		PROTO_TWISTLOG,		// $TWISTLOG$
		PROTO_OMS_CBOE,		// $OMS_CBOE$
		PROTO_TWIST,		// $TWIST$
		PROTO_REDIOLEDB,	// $REDIOLEDB$
		PROTO_CLS_BTR,		// $CLS_BTR$
		PROTO_SYMBLIST,		// $SYMBLIST$
		PROTO_CSV,			// $RTPOSITION$, $RTAUDIT$
		PROTO_RTAWAY,		// $RTAWAY$
		PROTO_RTECHOATS,	// $RTECHOATS$
		PROTO_EMIT,			// $EMIT$
		PROTO_COVERAGETOOL, // $COVERAGETOOL$
		PROTO_COUNT
	};

	// WsocksApp
	int WSHInitModule(AppConfig *config, class WsocksHost *apphost);
	int WSHCleanupModule();

	void WSDateChange();
	void WSTimeChange();

    void WSConOpened(int PortNo);
    int WSConMsgReady(int PortNo);
    int WSConStripMsg(int PortNo);
    int WSBeforeConSend(int PortNo);
    void WSConClosed(int PortNo);
#ifdef REDIOLEDBCON
	void WSConConnecting(int PortNo);
	void WSConClosing(int PortNo);
#endif

    void WSFileOpened(int PortNo);
    void WSFileClosed(int PortNo);

	void WSUsrOpened(int PortNo);
    int WSUsrMsgReady(int PortNo);
    int WSUsrStripMsg(int PortNo);
    int WSBeforeUsrSend(int PortNo);
    void WSUsrClosed(int PortNo);

	void WSUgrOpened(int PortNo);
    void WSUgrClosed(int PortNo);
	int WSTranslateUgrMsg(struct tdMsgHeader *MsgHeader, char *Msg, int PortNo, WORD LineId);

	int WSSysmonMsg(int UmrPortNo, int reqid, string cmd, const char *parm, int plen, void *udata);
	int WSSysmonHelp(int UmrPortNo, int reqid, string cmd, TVMAP& tvmap, string& oboi, string& dtid, TVMAP& rvmap);

	// OrderDBNotify
	void ODBError(int ecode, const char *emsg);
	void ODBEvent(int ecode, const char *emsg);
	//void ODBOrderCommitted(int rc, VSOrder *porder);
#ifdef MULTI_DAY_HIST
	void ODBIndexOrder(AppSystem *asys, OrderDB *pdb, VSOrder *porder, LONGLONG offset);
#else
	void ODBIndexOrder(AppSystem *asys, VSOrder *porder, LONGLONG offset);
#endif
	void ODBBusyLoad(LONGLONG offset, int lperc);
	void ODBOrderUnloaded(VSOrder *porder, LONGLONG offset);

	// VSCNotify
	// (vsquery.cpp)
	void VSCNotifyError(int rc, const char *emsg);
	void VSCNotifyEvent(int rc, const char *emsg);
	// For EncodeLoginRequest
	void VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen);
	void VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason);
	// For EncodeSqlRequest
	void VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxordersws, bool hist, bool live, LONGLONG iter);
	void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason);
	void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	// For EncodeSqlIndexRequest
	void VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxordersws, bool unique, int istart, char idir, LONGLONG iter);
	void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	// For EncodeCancelRequest
	void VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter);
	// For EncodeDescribeRequest
	void VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter);
	void VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	// (vsquery2.cpp)
	// For EncodeLoginRequest2
	void VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen);
	void VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate);
	// For EncodeSqlRequest2
	void VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl);
	// For EncodeSqlIndexRequest2
	void VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl);
	
	//DT9044
	void VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);
	void VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

	// ExprTokNotify
	void ExprTokError(int rc, const char *emsg);
	bool GetValue(const char *var, class ExprTok *etok, void *hint);
	#ifdef IQDNS
	bool GetUserValue(const char *var, class ExprTok *vtok, void *hint);
	bool GetVarsidValue(const char *var, class ExprTok *vtok, void *hint);
	bool GetDomrelValue(const char *var, class ExprTok *vtok, void *hint);
	bool GetDomtreeValue(const char *var, class ExprTok *vtok, void *hint);
	bool GetServerValue(const char *var, class ExprTok *vtok, void *hint);
	#endif
	bool IsInSet(const char *set, const char *val, void *hint);

	// VSDistNotify
	void VSDNotifyError(int rc, const char *emsg);
	void VSDNotifyEvent(int rc, const char *emsg);
	// Journal page transfer
	void VSDNotifyJPageRequest(void *udata, DWORD rid, JPAGE *jpage, bool retransmit);
	void VSDNotifyJpageReply(void *udata, int rc, DWORD rid, int lstart, int lend);
	void VSDNotifyAliasRequest(void *udata, DWORD rid, const char *prefix, const char *dpath, int proto, int wsdate);

	// DropClientNotify
	void DCSendMsg(void *udata, const char *msg, int mlen);
	void DCLoggedIn(void *udata, int hbinterval);
	void DCLoggedOut(void *udata, const char *text);
	LONGLONG DCWriteFix(void *udata, char mdir, FIXINFO *pfix);
	LONGLONG DCReadFix(void *udata, LONGLONG roff, char& mdir, FIXINFO *pfix);

#ifdef REDIOLEDBCON
	void OnRowChange(CQuery *pSender, HROW hRow, DBREASON eReason, DBEVENTPHASE ePhase);
#endif

	// Message handlers
	int TranslateConMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo);
	int TranslateUsrMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo);
	int HandleUscFixMsg(FIXINFO *pfix, int UscPortNo, FixStats *pstats, LONGLONG doffset, int proto);
	int HandleUscClserverMsg(const MSGHEADER *MsgHeader, char *Msg, int UscPortNo, FixStats *pstats, LONGLONG doffset, int proto);
	int HandleConTbackupMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo);
	#ifdef IQDNS
	int HandleConDnsReplMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo);
	#endif
	int HandleUsrCLbackupMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo);

	// Callbacks
	int PlayMLLogs(int PortNo, int proto, const char *fmatch, bool& cxl, int rstart, float playRate);
	int PlayCldropLogs(int PortNo, int proto, const char *fmatch, bool& cxl, int rstart, float playRate);
	int PlayTraderLogs(int PortNo, int proto, const char *fdir, const char *fmatch, bool& cxl, int rstart, float playRate);
	#ifdef IQSMP
	int PlaySmpLogs(int PortNo, int proto, const char *fdir, const char *fmatch, bool& cxl, int rstart, float playRate);
	#endif
	int PlayCoverageToolExport(int PortNo, int proto, const char *fdir, const char *fmatch, bool& cxl, int rstart, float playRate);
	int PlayCoverageToolExport(int PortNo, const char *fpath, bool& cxl);
	int ExpandCoverageToolOrder(int PortNo, char *fbuf, int flen);
	int SendSqlDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendSqlOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int SendSqlAccountsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int ReadDetailThread(VSDetailFile *dfile);
	int ReadMsgThread(VSDetailFile *dfile);
	#ifdef IQSMP
	int ReadSmpThread(VSDetailFile *dfile);
	#endif
	#ifdef AWAY_FIXSERVER
	int ReadAwayThread(VSDetailFile *dfile);
	#endif
	int DBLoadThread();
#ifdef TASK_THREAD_POOL
	int TaskThread();
	int PostTaskThread(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
#endif
	bool AddLiveQuery(VSQuery* pquery,FeedUser *fuser);
#ifdef REDIOLEDBCON
	int RediConnectThread(class RediConnectThreadData *ptd);
	int ImportRediOleDBLog(int wsdate, int tstart, int tstop);
	int RedibusTask(struct REDIBUSTASK *ptd);
	int RedibusTask(FILE *fp, const char *cmd, const char *taskid, const char *datetime);
	int SendRediPositions(const char *fpath);
	int ReadCsvThread(VSDetailFile *dfile);
	int HandleUsrRpxFakeAPIMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo);
	int NotifyRediBusInsert(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
	int RTOatsTask();
#endif

	bool CallTasks(VSOrder* order,AppSystem *asys,AppInstance* ainst,int UscPortNo,int didx=-1);
	AppSystem *GetAppSystem(const char *AppInstID);

protected:
	// Multi-thread safe
	HANDLE mutex;
	inline int Lock()
	{
		return (WaitForSingleObject(mutex,INFINITE)==WAIT_OBJECT_0)?0:-1;
	}
	inline int Unlock()
	{
		return ReleaseMutex(mutex)?0:-1;
	}

	struct DoneAwayLookup
	{
		string DONE_AWAY_USERS_PATH;
		string DONE_AWAY_ACCOUNTS_PATH;
		int DONE_AWAY_USER_TAG;
		map<string,string> doneAwayUserMap;
		map<string,string> doneAwayAccountMap;
	};
	typedef map<string,DoneAwayLookup> DAMAP;

	struct HistImport
	{
		int HISTIMP_TIME;			// Time to import
		string HISTIMP_DIST_PATH;	// Where to get .fix or .msg files
		string HISTIMP_HUB_PATH;	// Where to get .dat files
	};

	// setup.ini parameters
	string HEADING;			// Title caption heading
	int MAX_USR_TIME;		// Max time(ms) for consecutive WSUsrMsgReady
	int MAX_CON_TIME;		// Max time(ms) for consecutive WSConMsgReady
	int SERVER_EXIT_TIME;	// HHMMSS time to exit
	int WSDATE;				// Override date
	bool VSDIST_HUB;		// 1 for Gbear hub
	int MAX_DISTJ_SIZE;		// Max size(MB) for each VSDistJournal
	string FILLS_FILE_DIR;  // Directory for ffpath
	int LOG_QUERIES;		// 1 to log new and historic queries, 2 adds live queries
	int LOG_BROWSING;		// 1 to log browsing
	bool IQFLOW;			// 1 for IQ MLALGO flow
	bool IQCLFLOW;          // 1 for IQ ClServer flow //DT9044
	string REGION;			// Database region name
	string REGION_PASS;		// Database region password for peer login
	int RETRANS_TIMEOUT;	// Time(ms) to retransmit
	int KEEP_DAYS;			// Number of days of files to keep
#ifdef IQDNS
	int SAM_REPORT_TIME;	// Daily SAM report generation time
	string SAM_UPLOAD_DIR;	// Upload location
#endif
#ifdef MULTI_DAY_HIST
	int HISTIMP_TIME;		// Time to import
	string HISTIMP_DIST_PATHS; // Where to get .fix or .msg files
	string HISTIMP_HUB_PATHS;// Where to get .dat files
	bool HISTORIC_ONDEMAND; // On-demand load of indices
	int HISTORIC_BEGINDATE;	// Earliest date for history
	bool importHistoricFiles;//This flag is set when current day Historical file is imported and loaded into memory
#endif
#ifdef REDIOLEDBCON
	bool IMPORT_REDIBUS;	// 1 to schedule import tasks from redibus.ini
	int nextTaskId;
	string REDIBUS_USER;
	string REDIBUS_PASS;
	int REDIBUS_HEARTBEAT;
	string REDIDESTINATION_PATH;
	string ALGOSTRATEGY_PATH;
	bool REDIBUS_BOUNCE_MISSED_HBS;
	string REDIOLEDB_PATH;
	bool LOAD_DONE_AWAY_FILES;
	bool TRANSLATE_DONE_AWAY_USER;
	bool TRANSLATE_DONE_AWAY_ACCOUNT;
	//string DONE_AWAY_USERS_PATH;
	//string DONE_AWAY_ACCOUNTS_PATH;
	//int DONE_AWAY_USER_TAG;
	int RTOATS_IMPORT_TIME;
	string TUSERMAP_PATH;
	string TUSERDOMAIN_INI;
	list<pair<int,string>> CON_URL_LIST;
	list<SCHEDENTRY*> runNowTaskList;
#endif
	int FIX_HOURS_BEGIN;	// [hhhmmss start time for reporting disconnected sessions
	int FIX_HOURS_END;		// hhmmss) end time for reporting disconnected sessions
	bool TRADE_BUST_ERRORS;
	string QUERY_TIMEZONE;
	int LoadSetupIni();
	// Stats
	FixStats conStats;		// CON feed statistics
	FixStats uscStats;		// USC feed statistics
	DWORD lastTimeChange;
	DWORD lastBusyTick;
	// DEV-12136: Default broker code behavior
	string DEFAULT_ROUTEBROKER_ON_ORDERS;
	string DEFAULT_ROUTEBROKER_ON_TICKETS;
	string DEFAULT_ROUTEBROKER_ON_DONEAWAYS;
	const char* DefaultRouteBroker(bool isTicket,bool isAway);
	// Tasks
	enum ViewServerTasks
	{
		TASK_UNKNOWN=0,
		TASK_ROOTORDER,		// retired
		TASK_LISTCHILDREN,	// retired
		TASK_ORDERDETAILS,	// retired
		TASK_SQL_DETAILS,	// Virtual DETAILS table update
		TASK_SQL_ORDERS,	// Virtual ORDERS table update
		TASK_SQL_ACCOUNTS,	// Virtual ACCOUNTS table update
		TASK_SQL_USERS,		// Virtual USERS table update
		TASK_SQL_VARSID,	// Virtual VARSID table update
		TASK_SQL_DOMREL,	// Virtual DOMREL table update
		TASK_SQL_DOMTREE,	// Virtual DOMTREE table update
		TASK_SQL_SERVERS,	// Virtual SERVERS table update
		TASK_RESERVED,
		TASK_CLORDID,
		TASK_PARENTCLORDID,
		TASK_APPINST,
		TASK_APPSYS,
		TASK_SYMBOL,
		TASK_ROOTORDERID,
		TASK_FIRSTCLORDID,
		TASK_ACCOUNT,
		TASK_ECNORDERID,
		TASK_CLIENTID,
		TASK_REDIBUS,
		TASK_IMPORT_RTOATS,
		TASK_COUNT,
	};
	// For SendxxxLive 'hint'
	struct FeedHint
	{
		// For SendRootOrderLive, SendOrderDetailsLive, SendSqlDetailsLive, SendSqlOrdersLive
		AppSystem *asys;
		AppInstance *ainst;
		VSOrder *porder;
		int didx;
		FIXINFO *pfix;
		char proto;
	#ifdef TASK_THREAD_POOL
		bool unload;
		int unldUscPortNo;
		ITEMLOC unldLoc;
	#endif
	};
	// Tasking
	FeedUser taskUser;		// Internal task user
	FeedTaskMgr taskmgr;	// Task manager

	// Order database
	OrderDB odb;
	bool dbLoaded;
	bool dbLoadCxl;
#ifdef WIN32
	HANDLE dbLoadThread;
#else
	pthread_t dbLoadThread;
#endif
	DWORD ocnt;
	DWORD pcnt;
	DWORD oterm;
	VSOrder *CreateOrder(int UscPortNo, AppSystem *asys, const char *okey, bool& isnew, ITEMLOC& oloc);
	int CommitOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc);
	int UnloadOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc);
	int ModifyOrder(const char *appinstid, const char *okey, int wsdate,
		const char *account, const char *clientid, const char *symbol,
		const char *nappinstid, const char *newaccount, const char *newrootorderid, const char *newfirstclordid,
		const char *newsymbol);

	// Detail files
	HANDLE mrumux;
	list<VSMruCache*> mrulist;
	int OpenDetailFiles();
	int CloseDetailFiles();
	int LoadFixDetail(FeedHint *fhint);

	// Sysmon commands
	int Restart();
	int ShowUserTask(FILE *fp);
	int ShowUserList(FILE *fp);
	int SqlQuery(FILE *fp, const char *select, const char *from, const char *where, int UmrPortNo);
	int ShowDistJournal(FILE *fp);
#ifdef TEST_ALL_ORDERIDS
	int DbgLoadOids(FILE *fp);
#endif
	int ShowFileReport(FILE *fp);
	int ShowOrderMap(FILE *fp, AppSystem *asys);
	int ShowOrderMap(FILE *fp);
	int ShowParentMap(FILE *fp, AppSystem *asys);
	int ShowParentMap(FILE *fp);
	int ShowOrder(FILE *fp, const char *asname, const char *clordid, int odate);
	int ShowIndices(FILE *fp);
	int ShowOpenOrders(FILE *fp);
	int ShowFills(FILE *fp);
	int AddHistogram(DWORD shist[27][27], 
					DWORD ahist[27][27], 
					DWORD eohist[27][27], 
					DWORD fohist[27][27], 
					DWORD oohist[27][27], 
					DWORD aihist[27][27], 
					DWORD cnhist[27][27], 
					DWORD cphist[27][27], 
					DWORD akhist[27][27], 
					AppSystem *asys);
	int ShowIndices(FILE *fp, AppSystem *asys);
	int ShowOpenOrders(FILE *fp, AppSystem *asys);
	int ShowFills(FILE *fp, AppSystem *asys);
	int ReportOats(FILE *fp, const char *dllpath, const char *AppInstID);
	int ShowInstances(FILE *fp);
	int ShowAccounts(FILE *fp);
#ifdef IQDNS
	int SAMReport(char fpath[MAX_PATH], bool upload, const char *Domain, const char *User);
	int IQSAMReport(char fpath[MAX_PATH], bool upload, const char *Domain, const char *User);
#endif

	// Order histogram
	DWORD ohist[27][27];
#ifdef TEST_HASH_DIST
	DWORD *hhist;
#endif
	int WriteHistogram(char *fpath);

#ifdef DEBUG_ORDER_KEYS
	FILE *okfp;
#endif
#ifdef MULTI_DAY_REPLAY
	LONGLONG maxFixMsgs;
#endif
	int ImportCoverageToolOrders(const char *fpath);

	// Fills File
	char ffpath[MAX_PATH];
	FILE *ffp;

	// App Systems
	APPSYSMAP asmap;
	APPINSTMAP aimap;
	HANDLE asmux;
	const char **asbrowse;
	static AppSystem *CreateSystem(APPSYSMAP& nasmap, const char *sname, int& scnt);
	int ReadAppSystemsIni(APPSYSMAP& nasmap, APPINSTMAP& naimap);
    int SaveAppSystemsIni();
	int ReadTopologyXml(APPSYSMAP& nasmap, APPINSTMAP& naimap);
	int LoadSystemConfig();
	AppInstance *GetAppInstance(const char *AppInstID);

	// Realtime playback
	DWORD GetPlayTime();
	DWORD GetPlayTime(DWORD hms);
	DWORD GetPlayTime(const char *fixts);
	DWORD RevPlayTime(DWORD pts, char *pbuf);

	// Queries
	int SendSqlResult(VSQuery *pquery, int rc, int endpos, int totpos, bool hist, char proto, int& ndets, bool more, const char *reason);
	int SendSqlIndexResult(VSQuery *pquery, int rc, int endcnt, int totcnt, const char **pstr, int& ndets, bool more, const char *reason);
	int StartQuery(VSQuery *pquery);
	int ContinueDetailsQuery(VSQuery *pquery);
	int ContinueOrdersQuery(VSQuery *pquery);
	int StartAccountsQuery(VSQuery *pquery);
	int ContinueAccountsQuery(VSQuery *pquery);
#ifdef IQDNS
	int StartUsersQuery(VSQuery *pquery);
	int ContinueUsersQuery(VSQuery *pquery);
	int StartVarsidQuery(VSQuery *pquery);
	int ContinueVarsidQuery(VSQuery *pquery);
	int StartDomrelQuery(VSQuery *pquery);
	int ContinueDomrelQuery(VSQuery *pquery);
	int StartDomtreeQuery(VSQuery *pquery);
	int ContinueDomtreeQuery(VSQuery *pquery);
	int StartServersQuery(VSQuery *pquery);
	int ContinueServersQuery(VSQuery *pquery);
#endif
	int NextAppSystem(VSQuery *pquery, bool first);
	int StartForwardQuery(VSQuery *pquery, const char *AppInstID, int ConPortNo);
	int ContinueForwardQuery(VSQuery *pquery);
	int CancelForwardQuery(VSQuery *pquery);

	// IQ flow
	list<class MatchEntry*> matchList;
	int LoadMatchFile();
	class MatchEntry *GetMatch(const char *tparent);
	HANDLE iqmux;
	// Local maps used when there are missing FIX tags
	map<string,string> *pm; // ClParentOrderID map
	map<string,string> *fm; // FirstClOrdID map
	map<string,string> *am; // AppInstID map
	#ifdef GS_RTECHOATS
	map<string,string> *em; // ECNOrderID map
	map<string,string> *cm; // ClOrdID map
	map<string,string> *rm; // RootOrderID map
	map<string,string> *km; // Ticket order map
	#endif
#ifdef HOLD_IQ_CHILD_ORDERS
	multimap<string,class HeldDetail *> *hm; // Held details by ClParentOrderID
#endif

	// Entitlements
	USERENTITLELIST *MakeEntitlementList(char *estr);
	bool IsEntitled(FeedUser *fuser, const char *sname, const char *iname);

	// Accounts
	ACCTMAP acmap;
#ifdef REDIOLEDBCON
	string TACMAP_SYSTEM;
	string TACMAP_PREFIX;
	TACCTMAP tacmap;
	TUSERMAP tusermap;
#endif
	static Account *CreateAccount(ACCTMAP& nacmap, const char *aname, int& acnt);
	int ReadAccountsIni(ACCTMAP& nacmap);
	Account *GetAccount(const char *aname);

	// FIX drop copy sessions
	typedef map<int,class DropClient*> DCMAP;
	DCMAP dcmap;
#ifdef FIX_SERVER
	typedef map<int,class DropServer*> DSMAP;
	DSMAP dsmap;
#endif

	// FIX protocol help
	int PubHubFixSubstr(const char *&fptr, int& flen);

	// VSDist journal encoding
	int AddJournal(VSDistJournal *dj, const char *dptr, int dlen, LONGLONG doffset, LONGLONG dend,
				   const char *AppInstID, const char *ClOrdID, const char *RootOrderID,
				   const char *FirstClOrdID, const char *ClParentOrderID, const char *Symbol,
				   double Price, char Side,
				   const char *Account, const char *EcnOrderID, const char *ClientID,
				   const char *Connection, int OrderDate, const char *AuxKey,
				   char mtype, char etype, int OrderQty, int CumQty, int LeavesQty, 
				   LONGLONG Timestamp, const char *OrigClOrdID, const char *ExecID,
				   int LastQty, const char *ffpath);

	// TBackup client
	int TBackupOpened(int ConPortNo);
	int TBackupClosed(int ConPortNo);

	// CLBackup client
	int CLBackupInit(int UscPortNo,char* prefix);
	int CLBackupShutdown(int UscPortNo);
	int CLBackupOpened(int UsrPortNo);
	int CLBackupClosed(int UsrPortNo);
	void CLBackupDateChange(int UsrPortNo);
	void CLBackupTimeChange(int UsrPortNo);
	//DT9044
	bool ClBackupAccountRec(int PortNo,int aidx,char *args[32],int lidx);
	bool ClBackupCheckForDetailUpdate(OrderDB* pdb, VSOrder *porder, int UscPortNo,VSDetailFile* dfile,char* AppInstID, char*ClOrdID,
		char* AuxKey,AppSystem* asys, AppInstance* ainst,bool acct,ITEMLOC oloc,LONGLONG newdoff);

	// Remote regions
	APPSYSMAP *GetRemoteRegion(const char *Region, int& ConPortNo);
	AppSystem *GetRemoteSystem(const char *Region, const char *AppInstID, int& ConPortNo);
	AppInstance *GetRemoteInstance(const char *Region, const char *AppInstID, int& ConPortNo);

#ifdef TASK_THREAD_POOL
	#ifdef WIN32
	struct TASKOVL :public OVERLAPPED
	{
		FeedUser *fuser;
		string skey;
		uchar taskid;
		DWORD rid;
		VSQuery *pquery;
		FeedHint fhint;
	};
	bool taskThreadAbort;
	HANDLE taskIocpPort;
	HANDLE taskThreads[4];
	#endif
#endif

	set<string> onceErrSet;
	void WSLogErrorOnce(const char *fmt,...);

	// Auto-cleanup
	//static int RecurseDelete(const char *dpath);
	static int CalcTPlusDate(int wsdate, int n, bool skipWeekends=true);
	int PurgeOldDays();

	#ifdef IQDNS
	// DNS replication
	int DnsReplOpened(int ConPortNo);
	int DnsReplClosed(int ConPortNo);
	int DnsMgrOpened(int ConPortNo);
	int DnsMgrClosed(int ConPortNo);
	//DT9044: Added optional AppSystem to re-load Accounts upon restart
	AppInstance *CreateDnsDomain(int ConPortNo, const char *Domain, AppSystem *asys=0); 
	int CreateDnsUserEnt(int ConPortNo, struct tdUserAccRec& UserAccRec);
	int CreateDnsUser(int ConPortNo, struct tdMsg1960Rec& Msg1960Rec);
	int AssociateDnsDomains(AppInstance *pdom, AppInstance *dparent, AppInstance *eparent);
	static int SAMReportLine(FILE *fp, class IQUser *puser, const struct tdUserAccRec& uacc, const char *Domain, bool iqsam);
	int BFSEntDomains(list<string>& dlist, AppSystem *asys, const char *Domain);
	int BFSRealDomains(list<string>& dlist, AppSystem *asys, const char *Domain);

	//DT9044: Added optional AppSystem to re-load Accounts upon restart and make public
public:
	Account *CreateDnsAccount(int ConPortNo, struct tdMsg632Rec& Msg632Rec, AppSystem *asys=0);
	int CreateRediAccount(int ConPortNo, struct tdMsg632Rec& Msg632Rec, AppSystem *asys=0);
	#endif

	//DT9873
	int SingleSearchCondtion(VSQueryScope& qscope,const char*& key,int& AuxKeyIndex);
	bool DetermineBestSearchCond(VSQuery* pquery, VSQueryScope& qscope,OrderDB *pdb);

#ifdef SPECTRUM
	//DT10473
	bool ComplexQuery(VSQuery* pquery,VSQUERYMAP *qmap,VSQUERYMAP::iterator& qit,FeedUser *fuser);
	bool ComplexQuerySearch(VSQuery* pquery, KEYTYPE type,string& key,WhereSegment* wSegment=NULL);
	bool ComplexQuerySendResults(VSQuery*& pquery,VSQUERYMAP *qmap,VSQUERYMAP::iterator& qit);
	bool ComplexQueryContinueQuery(VSQuery* pquery,VSQUERYMAP *qmap,VSQUERYMAP::iterator& qit);
	bool ComplexQueryAddLive(VSQuery* pquery,FeedUser* fuser,uchar qt,string& key);
	bool zmapReset;
public:
	KEYTYPE ConditionMatch(const char*);
	uchar ConvertKeyType2TaskID(KEYTYPE keytype);
	int BuildWhereSegments(list<WhereSegment>& WhereList, string _where);
    int SendSqlComplexDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
    int SendSqlComplexOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
#else
	uchar ConvertKeyType2TaskID(KEYTYPE keytype);
#endif

#ifdef IQSMP
protected:
	// Support heterogeneous-protocol details
	int SmpShutdown(int UscPortNo);
	int SendResults(int& nfix, VSQuery *pquery, int proto, int& tsent, bool needMoreEnd, list<char*>& dlist);
#endif

#ifdef MULTI_DAY_HIST
	int ImportHistoricFiles(int wsdate);
	int ImportHistoricFiles(int wsdate, HistImport *import);
#endif

#ifdef REDIOLEDBCON
	static CComModule ComModule;
	CRITICAL_SECTION rediOleMux;
	map<string,WORD> symExchMap;
	map<string,string> symIsinMap;
	map<string,string> lastMarketMap;
	map<string,pair<string,string>> rediDestMap;
	map<string,string> algoStrategyMap;
	//map<string,string> doneAwayUserMap;
	//map<string,string> doneAwayAccountMap;
	DAMAP damap;
	struct REDIHEARTBEAT
	{
		char server[32];
		int serverPort;
		char status[16];
		char serverTime[32];
		char serverTimeZone[8];
		int disconCount;
		int staleTimeCount;
	};
	int RediTimeChange();
	int VSRediQuery(int ConPortNo, const char *select, int PortType, int PortNo, FILE *fp, void *udata);
	int VSWaitRediQuery(int ConPortNo, int timeout, int qid, bool callTimeChange);
	int VSOleDBMsgReady(int ConPortNo, char *Msg, int MsgLen, int PortType, int PortNo, FILE *fp, void *udata);
	int LoadRedibusIni();
	static LONGLONG GMTToEST(LONGLONG gts, SYSTEMTIME& tsys, string& stz);
	static LONGLONG ESTToGMT(LONGLONG gts, SYSTEMTIME& tsys, string& stz);
	void decodeTranTypeCode(struct tdBigTradeRec *btr, const char *capacity);
	void decodeRediDestination(struct tdBigTradeRec *btr, const char *rediDestination);
	void decodeAlgoStrategy(struct tdBigTradeRec *btr, const char *algorithm, const char *rediDestination);
	int decodeProductType(const char *productType);
	void decodeFutureExchange(const char *productType, int spreadCode, char& exchange, char& region);
	void getEcnParserOrderNo(struct tdBigTradeRec *btr, const char *t11, const char *t37);
	void getCancelMemo(struct tdBigTradeRec *btr, const char *t11, const char *t37);
	int ConvertFixToBTR(VSQueryResult *vsres);
	int ConvertFixToBTR_Old(VSQueryResult *vsres);
	int FixTags2BTR(VSQueryResult *vsres, struct tdBigTradeRec *btr, struct tdMiFID* miFID);
	int RediSqlLoadCat(map<string,struct REDITABLE*>& tmap, const char *fpath);
	int RediSqlDiff(FILE *fp, map<string,REDITABLE*>& tmap, map<string,struct REDITABLE*>& nmap);
	#ifdef _DEBUG
	int RediSqlScript(FILE *fp, const char *tables);
	int RediSqlLoad(FILE *fp, const char *tables);
	#ifdef IQDEVTEST
	int RediSqlInsertDnsDomains(FILE *fp);
	int RediSqlInsertDnsAccounts(FILE *fp);
	int RediSqlInsertDnsUsers(FILE *fp);
	#endif
	#endif
	#ifdef REDISQLOATS
	int RediSqlImportOats(FILE *fp, const char *tables);
	#endif
	int LoadLastMarketDestTxt();
	int LoadRediDestCsv();
	int LoadAlgoStrategyCsv();
	int GenRediDestinationCsv(const char *opath, const char *dpath);
	int GenTsxList(const char *opath, const char *dpath);
	int GenEUList(const char *opath, const char *dpath);
	int LoadDoneAwayUsersIni();
	int LoadDoneAwayAccountsCsv();
	int LoadTUserMapCsv();
	int QueryRediPositions(const char *dpath);
#endif

#ifdef AWAY_FIXSERVER
	int TranslateAwayMsg(MSGHEADER *MsgHeader, char *Msg, int UgrPortNo);
#endif

	// Missing ClOrdIDs
	int UnkClOrdIDSeqNo;

	int QueryTimeZoneBias;
	int GetTimeZoneBias(const char *timeZone);

	// DEV-18992
	map<string,int> emitTagMap;
	map<int,string> emitTagReverseMap;
	map<string,int> emitClientOrderMap;
	int emitSeqno;

	int EmitMsgReady(int PortNo);
	void EmitPortClosed(int PortNo);
	int LoadEmitFIXMapping();
	const string GenerateEmitClOrdID();
	int TranslateEmitMessage(FIXINFO& ifx, char *cmdbuf);
	int TranslateEmitResponse(FIXINFO& ifx, char *cmdbuf);
	void HandleEmitRequest(int PortNo, int Size);
	DropClient *GetDropClient(const char *sender, const char *target);
	int ReturnEmitReponse(FIXINFO& ifx);

	string TRADE_REGION;
	int TRADE_MARKET_OFFSET;
	TIME_ZONE_INFORMATION TradeRegion;
	int GetTradeRegion(const char *tradeRegion, int tradeMarketOffset);

	// DEV-25077
	list<HistImport> histImports;
};

// IQMatrix DLL interface
#define VIEWSERVER_APP_CLASS "ViewServer"
#if defined(WIN32)||defined(_CONSOLE)
__declspec(dllexport) WsocksApp * __stdcall GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
__declspec(dllexport) void __stdcall FreeAppInterface(WsocksApp *pmod);
#else
extern "C" void * __stdcall GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
extern "C" void __stdcall FreeAppInterface(void *amod);
#endif

#endif//_VIEWSERVER_H
