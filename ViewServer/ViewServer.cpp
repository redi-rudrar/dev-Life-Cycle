// ViewServer.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "wstring.h"
#include "vsquery.h"
#include "iqflow.h"
#include <time.h>
#ifndef WIN32
#include <sys/stat.h>
#endif
#include "IQClServer.h" //DT9044
#include "wstring.h"
#include <Shlwapi.h>
#ifdef REDIOLEDBCON
#include "mcfilter.h"
#endif

#pragma pack(push,8)
// 5000 Global login request
typedef struct tdMsg5000Rec
{
	char ServiceName[64];   // in case of bad routing we can see this in the logs
	char Domain[/*DOMAIN_LEN*/16];
	char User[/*USER_LEN*/16];
	char Account[/*ACCOUNT_LEN*/16];
	char Oms[/*DOMAIN_LEN*/16];
	short BuildType; // this is the product code - so that the same cannot log into wrong domains  //0 = Normal, 1 = DirectTrade, 2 = ABN Trade; 3 = B of A
	int ProductType; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver
	int BuildNo;
	int LineID;
	int VerNo;       // the fe version number - I think current is 9
	short DocSignVersion; // should be in 5001
	char LogonFlags;  // first time or not - used as a bool for multiple account login 
	char EncryptionType;    // 0-Plain text, 1-MD5
	char ClientIP[20];      // IP of client for which this message is on behalf
	int ClientPort;         // The client connecting port isn't very useful, so maybe the WSOCKS USR portno
	int PassLen;
	char Spare[76];
}  MSG5000REC; //256 +ePassword

// 5001 Global login response
typedef struct tdMsg5001Rec
{
	char ServiceName[64];
	char Domain[/*DOMAIN_LEN*/16];
	char User[/*USER_LEN*/16];
	char Account[/*ACCOUNT_LEN*/16];
	int ProductType; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver
	char LoginOk;
	char RejectReason[80];
	BYTE  cEnable[32][32];  // Entitlements (union with RejectReason?)
	int LineID;
	short DocSignVersion;
	char passwdStatus;            // 0 - OK, 1 - Expiring, 2 - Expired
	int PwdExpireDate;// date the password expires
	char ClientIP[20];      // Copied from msg 5000
	int ClientPort;         // Copied from msg 5000
	char Spare[20];
}  MSG5001REC; // 1280 bytes
#pragma pack(pop)

//DT9044: Help w/ a little granularity on the playback files
int WildCardCompare(const char *wildcardStr, const char *matchStr)
{
	const char *cp = NULL;
	const char *mp = NULL;

	while ((*matchStr) && (*wildcardStr != '*'))
	{
		if ((*wildcardStr != *matchStr) && (*wildcardStr != '?'))
			return 0;
 		wildcardStr++;
		matchStr++;
	}

	while (*matchStr)
	{
		if (*wildcardStr == '*')
		{
			if (!*++wildcardStr)
				return 1;
			mp = wildcardStr;
			cp = matchStr+1;
		}
		else if ((*wildcardStr == *matchStr) || (*wildcardStr == '?'))
		{
			wildcardStr++;
			matchStr++;
		}
		else
		{
			wildcardStr = mp;
			matchStr = cp++;
		}
	}

	while (*wildcardStr == '*')
	{
		wildcardStr++;
	}
	return !*wildcardStr;
}

#ifdef HOLD_IQ_CHILD_ORDERS
class HeldDetail
{
public:
	HeldDetail()
		:pfix(0)
		,UscPortNo(-1)
		,doffset(-1)
		,proto(0)
	{
	}
	~HeldDetail()
	{
		if(pfix)
		{
			delete pfix; pfix=0;
		}
	}

	FIXINFO *pfix;
	int UscPortNo;
	LONGLONG doffset;
	int proto;
};
#endif

#ifdef REDIOLEDBCON
CComModule ViewServer::ComModule;
#endif

ViewServer::ViewServer()
	// public items
	// protected items
	:HEADING()
	,MAX_USR_TIME(20)
	,MAX_CON_TIME(990)
	,SERVER_EXIT_TIME(203000)
	,VSDIST_HUB(false)
	,MAX_DISTJ_SIZE(100)
	,FILLS_FILE_DIR()
	,LOG_QUERIES(0)
	,LOG_BROWSING(0)
	,IQFLOW(false)
	,IQCLFLOW(false) //DT9044
	,REGION()
	,REGION_PASS()
	,RETRANS_TIMEOUT(10000)
	,WSDATE(0)
	,KEEP_DAYS(-1)
#ifdef IQDNS
	,SAM_REPORT_TIME(0)
	,SAM_UPLOAD_DIR("X:\\DnsLogs")
#endif
#ifdef MULTI_DAY_HIST
	,HISTIMP_TIME(0)
	,HISTIMP_DIST_PATHS()
	,HISTIMP_HUB_PATHS()
	,HISTORIC_ONDEMAND(true)
	,HISTORIC_BEGINDATE(0)
	, importHistoricFiles(false)
#endif
#ifdef REDIOLEDBCON
	,IMPORT_REDIBUS(false)
	,nextTaskId(0)
	,REDIBUS_USER("u705758")
	,REDIBUS_PASS("Abc123")
	,REDIBUS_HEARTBEAT(5)
	,REDIDESTINATION_PATH("P:\\dafsoft\\import\\RediDestination.csv")
	,ALGOSTRATEGY_PATH("P:\\dafsoft\\import\\AlgoStrategy.csv")
	,REDIOLEDB_PATH()
	,LOAD_DONE_AWAY_FILES(false)
	//,DONE_AWAY_ACCOUNTS_PATH("P:\\dafsoft\\import\\DoneAwayAccounts.csv")
	//,DONE_AWAY_USERS_PATH("setup\\DoneAwayUsers.ini")
	//,DONE_AWAY_USER_TAG(0)
	,TRANSLATE_DONE_AWAY_USER(true)
	,TRANSLATE_DONE_AWAY_ACCOUNT(true)
	,RTOATS_IMPORT_TIME(0)
	,TUSERMAP_PATH()
	,TUSERDOMAIN_INI()
	,REDIBUS_BOUNCE_MISSED_HBS(true)
	,CON_URL_LIST()
	,runNowTaskList()
#endif
	,FIX_HOURS_BEGIN(0)
	,FIX_HOURS_END(0)
	,TRADE_BUST_ERRORS(true)
	,QUERY_TIMEZONE("Eastern Standard Time")
	,conStats()
	,uscStats()
	,lastTimeChange(0)
	,lastBusyTick(0)
	,taskUser()
	,taskmgr()
	,odb()
	,dbLoaded(false)
	,dbLoadCxl(false)
#ifdef WIN32
	,dbLoadThread(0)
#else
	,dbLoadThread()
#endif
	,ocnt(0)
	,pcnt(0)
	,oterm(0)
	,mrulist()
#ifdef DEBUG_ORDER_KEYS
	,okfp(0)
#endif
#ifdef MULTI_DAY_REPLAY
	,maxFixMsgs(200000000) //200M limit
#endif
	,ffp(0)
	,aimap()
	,asmap()
	,asbrowse(0)
	,matchList()
	,pm(0)
	,fm(0)
	,am(0)
	#ifdef GS_RTECHOATS
	,em(0)
	,cm(0)
	,rm(0)
	,km(0)
	#endif
#ifdef HOLD_IQ_CHILD_ORDERS
	,hm(0)
#endif
	,acmap()
	,dcmap()
#ifdef FIX_SERVER
	,dsmap()
#endif
	,onceErrSet()
#ifdef SPECTRUM
	,zmapReset(false)
#endif
#ifdef REDIOLEDBCON
	,TACMAP_SYSTEM("CLSERVER")
	,TACMAP_PREFIX()
	,tacmap()
	,symExchMap()
	,symIsinMap()
	,lastMarketMap()
	,rediDestMap()
	,algoStrategyMap()
	// DEV-12136: Default broker code behavior
	,DEFAULT_ROUTEBROKER_ON_ORDERS("")
	,DEFAULT_ROUTEBROKER_ON_TICKETS("")
	,DEFAULT_ROUTEBROKER_ON_DONEAWAYS("")
	//,doneAwayUserMap()
	//,doneAwayAccountMap()
#endif
	,UnkClOrdIDSeqNo(0)
	,QueryTimeZoneBias(0)
	// DEV-18992
	,emitTagMap()
	,emitTagReverseMap()
	,emitClientOrderMap()
	,emitSeqno(0)
	,TRADE_REGION()
	,TRADE_MARKET_OFFSET(0)
	,histImports()
{
	mutex=CreateMutex(0,false,0);
	memset(ohist,0,sizeof(ohist));
#ifdef TEST_HASH_DIST
	hhist=new DWORD[1024*1024]; // 1 million buckets
	memset(hhist,0,1024*1024*sizeof(DWORD));
#endif
	memset(ffpath,0,sizeof(ffpath));
	mrumux=CreateMutex(0,false,0);
	asmux=CreateMutex(0,false,0);
	iqmux=CreateMutex(0,false,0);
#ifdef TASK_THREAD_POOL
	#ifdef WIN32
	taskThreadAbort=false;
	memset(taskThreads,0,sizeof(taskThreads));
	taskIocpPort=0;
	#endif
#endif
	memset(&TradeRegion,0,sizeof(TradeRegion));
#ifdef REDIOLEDBCON
	InitializeCriticalSection(&rediOleMux);
#endif
}
ViewServer::~ViewServer()
{
#ifdef WIN32
	if(mutex)
	{
	#ifdef _DEBUG
		WaitForSingleObject(mutex,INFINITE);
	#else
		WaitForSingleObject(mutex,1000);
	#endif
		CloseHandle(mutex); mutex=0;
	}
	if(iqmux)
	{
		CloseHandle(iqmux); iqmux=0;
	}
	if(asmux)
	{
		CloseHandle(asmux); asmux=0;
	}
	if(mrumux)
	{
		CloseHandle(mrumux); mrumux=0;
	}
	#ifdef REDIOLEDBCON
	DeleteCriticalSection(&rediOleMux);
	#endif
#else
	if(mutex)
	{
	#ifdef _DEBUG
		WaitForSingleObject(mutex,INFINITE);
	#else
		WaitForSingleObject(mutex,1000);
	#endif
		DeleteMutex(mutex); mutex=0;
	}
	if(iqmux)
	{
		DeleteMutex(iqmux); iqmux=0;
	}
	if(asmux)
	{
		DeleteMutex(asmux); asmux=0;
	}
	if(mrumux)
	{
		DeleteMutex(mrumux); mrumux=0;
	}
#endif
#ifdef TEST_HASH_DIST
	delete hhist; hhist=0;
#endif
}

//ViewServer* myvs=0;

// IQMatrix DLL interface
#if defined(WIN32)||defined(_CONSOLE)
WsocksApp * __stdcall GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#else
void * __stdcall GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#endif
{
        if(version!=WSHOST_INTERFACE_VERSION)
            return 0;
        if(!_stricmp(aclass,VIEWSERVER_APP_CLASS))
        {
			ViewServer *pmod=new ViewServer;
			// This looks like a debug code left in
			//myvs=pmod;
			pmod->pFreeAppInterface=FreeAppInterface;
			return pmod;
        }
        return 0;
}
#if defined(WIN32)||defined(_CONSOLE)
void __stdcall FreeAppInterface(WsocksApp *pmod)
{
#else
void __stdcall FreeAppInterface(void *amod)
{
        WsocksApp *pmod=(WsocksApp *)amod;
#endif
        if(!_stricmp(pmod->pcfg->aclass.c_str(),VIEWSERVER_APP_CLASS))
        {
			delete (ViewServer*)pmod;
        }
}
// WsocksApi
// The wsocksapi provides transport, time, and sysmon
#ifdef WIN32
DWORD WINAPI _BootDBLoadThread(LPVOID arg)
{
	ViewServer *pmod=(ViewServer*)arg;
	return pmod->DBLoadThread();
}
#else
void *_BootDBLoadThread(void *arg)
{
	ViewServer *pmod=(ViewServer*)arg;
	return (void*)(PTRCAST)pmod->DBLoadThread();
}
#endif
#ifdef TASK_THREAD_POOL
#ifdef WIN32
DWORD WINAPI _BootTaskThread(LPVOID arg)
{
	ViewServer *pmod=(ViewServer*)arg;
	return pmod->TaskThread();
}
#endif
#endif

int ViewServer::WSHInitModule(AppConfig *config, class WsocksHost *apphost)
{
	__WSDATE__=__DATE__;
	__WSTIME__=__TIME__;
	NO_OF_CON_PORTS=100;
	NO_OF_FILE_PORTS=10;
	NO_OF_USC_PORTS=50;
	NO_OF_USR_PORTS=200;
	NO_OF_UGC_PORTS=20;
	NO_OF_UGR_PORTS=20;
#ifdef REDIOLEDBCON
	_pAtlModule=&ComModule;
#endif
#ifdef AWAY_FIXSERVER
	NO_OF_UGC_PORTS=10;
	NO_OF_UGR_PORTS=10;
#endif

	// 2013
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","TWIST version."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Fix for memory exception with aoit iterator in vsquery2."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Added setup.ini::IMPORT_REDIBUS and redibus.ini."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Convert FIX to BTR for TWIST query by CL."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Added twist account to domain lookup."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Added modifyorder sysmon command."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Added CON$SYMBLIST$ port for symbol exchange."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Import RTPOSITION file as positions."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Compile out all complex query code."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Live TWIST BTR updates."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Set position orders' orderdate to import date."));
	config->featList.push_back(AppFeature(20130605,111101,0,"bao","Support RpxFakeApi SQL queries."));
	config->featList.push_back(AppFeature(20130626,165631,0,"bao","Added REDIBUS_USER REDIBUS_PASS to setup.ini"));
	config->featList.push_back(AppFeature(20130702,151810,0,"bao","Added REDIBUS_HEARTBEAT to setup.ini"));
	config->featList.push_back(AppFeature(20130716,161432,0,"bao","Better simultaneous queries"));
	config->featList.push_back(AppFeature(20130717,122558,0,"bao","Prefix RTPOSITION ClOrdID with RTPOS_."));
	config->featList.push_back(AppFeature(20130725,141024,0,"bao","Use 'SDK' appid instead of 'eredi'."));
	config->featList.push_back(AppFeature(20130802,171013,0,"bao","Daily checking for bus changes."));
	config->featList.push_back(AppFeature(20130814,105759,0,"bao","Fix to daily checking for bus changes."));
	config->featList.push_back(AppFeature(20130815,102800,0,"bao","Support yyyymmdd for current date in redibus.ini."));
	config->featList.push_back(AppFeature(20130903,163536,0,"bao","Added setup.ini::FIX_HOURS_BEGIN and FIX_HOURS_END."));
	config->featList.push_back(AppFeature(20130917,170914,0,"bao","Added last capacity(29), last market(30), and openclosed(77) from Redi drop."));
	config->featList.push_back(AppFeature(20130917,170914,0,"bao","RED-87: Added MktBidPrice(645), and MktOfferPrice(646) from Redi drop."));
	config->featList.push_back(AppFeature(20130923,164515,0,"bao","Learn Redi accounts from message 6314 from Twist server."));
	config->featList.push_back(AppFeature(20130926,111800,96,"bao","RED-96: Enrich Redi drop with ISIN code from NexusGateway."));
	config->featList.push_back(AppFeature(20130927,153818,84,"bao","RED-84: Redi liquidity A,R,X"));
	config->featList.push_back(AppFeature(20130930,143504,91,"bao","RED-91: Redi cam codes"));
	config->featList.push_back(AppFeature(20131002,164511,0,"bao","Don't put tag 58(Text) into BTR.TradeRec.Memo"));
	config->featList.push_back(AppFeature(20131007,170612,83,"bao","DEV-83: Put tag 143 into RED-## BTR.TradeRecB.RediDestination"));
	config->featList.push_back(AppFeature(20131008,145832,69,"bao","DEV-69: Read LastMarketDestination.txt"));
	config->featList.push_back(AppFeature(20131016,161232,84,"bao","RED-84: Get liquidity from tag 8023."));
	config->featList.push_back(AppFeature(20131016,161232,645,"bao","DEV-645: Index RoutingBroker for CL OATS cancel/replace."));
	config->featList.push_back(AppFeature(20131016,161232,645,"bao","DEV-645: Always set TradeRec.Pending."));
	config->featList.push_back(AppFeature(20131016,161232,650,"bao","DEV-650: Set TradeRec.StopAmount from tag 99."));
	config->featList.push_back(AppFeature(20131016,161232,650,"bao","DEV-650: Set TradeRec.OnClose from tag 40=5,A,B."));
	config->featList.push_back(AppFeature(20131016,161232,650,"bao","DEV-650: Set TradeRec.Gtc from tag 59=1,5,6."));
	config->featList.push_back(AppFeature(20131016,161232,650,"bao","DEV-650: Set TradeRec.OptExpireDate, OptType, and OptStrikePrice from tags 200,201,202,205."));
	config->featList.push_back(AppFeature(20131016,161232,650,"bao","DEV-650: Set TradeRecB.NotOATSReportable from tag 113=N."));
	config->featList.push_back(AppFeature(20131024,140753,920,"bao","DEV-920: Set TradeRecB.CMTA from tag 8439."));
	config->featList.push_back(AppFeature(20131024,140753,961,"bao","DEV-961: Set TradeRecA.Notes from short locate 5700."));
	config->featList.push_back(AppFeature(20131024,140753,1041,"bao","DEV-1041: RediDestination.csv."));
	config->featList.push_back(AppFeature(20131024,140753,39,"bao","DEV-39: Set TradeRecB.Portfolio from tag 8440."));
	config->featList.push_back(AppFeature(20131028,180157,1240,"bao","DEV-1240: Set TradeRecB.RediSpreadTrade from tag 167=MLEG."));
	config->featList.push_back(AppFeature(20131106,164904,1383,"bao","DEV-1383: Uppercase domain instance names."));
	config->featList.push_back(AppFeature(20131107,122234,1426,"bao","DEV-1426: Allow FIX messages missing tag 11."));
	config->featList.push_back(AppFeature(20131108,115213,1426,"bao","DEV-1426: Don't send busts and corrections to CL"));
	config->featList.push_back(AppFeature(20131111,170927,1356,"bao","DEV-1356: Ticket orders"));
	config->featList.push_back(AppFeature(20131112,121325,1381,"bao","DEV-1381: GMTToEST fix for standard time."));
	config->featList.push_back(AppFeature(20131112,173512,0,"bao","Fix for binary query on ACCOUNTS table."));
	config->featList.push_back(AppFeature(20131113,145252,1381,"bao","DEV-1381: Send unique EcnOrderID to CL for multi-leg orders."));
	config->featList.push_back(AppFeature(20131114,113648,1381,"bao","DEV-1381: Allow 21-character options on ExSymbol.Symbol."));
	config->featList.push_back(AppFeature(20131126,153718,1615,"bao","DEV-1615: Use previous ECNOrderNo for TradeRecA.CancelMemo on option spreads."));
	config->featList.push_back(AppFeature(20131204,150220,1364,"bao","DEV-1364: Enable ECNId lookup on live query results."));
	config->featList.push_back(AppFeature(20131205,142357,2097,"bao","DEV-2097: Set TradeRec.DoNotClear on AWAY orders except when 113=Y."));
	config->featList.push_back(AppFeature(20131211,141813,1369,"bao","DEV-1369: Set TradeRec.Type=2 for 40=S."));
	config->featList.push_back(AppFeature(20131213,114633,83,"bao","DEV-83: Upload RediDestination.csv to remote entry for redibus.ini DESTINATIONINFO."));
	config->featList.push_back(AppFeature(20131213,151540,1369,"bao","DEV-1369: Mark equity leg of option spread as market order with no price."));
	config->featList.push_back(AppFeature(20131216,144725,2604,"bao","DEV-2604: Added setup.ini::REDIDESTINATION_PATH."));
	config->featList.push_back(AppFeature(20131217,164607,0,"bao","Put alternative destinations into the RediOleDB connection string."));
	config->featList.push_back(AppFeature(20131218,163553,0,"bao","Fix for importing historic .ext and .zmap files."));
	config->featList.push_back(AppFeature(20131218,163553,0,"bao","Do not save appsystems.ini for Redi."));
	config->featList.push_back(AppFeature(20131220,120659,0,"bao","Change cam code tags to GS-compliant production tags."));
	config->featList.push_back(AppFeature(20131223,151616,2763,"bao","DEV-2763: Added setup.ini::REDIBUS_BOUNCE_MISSED_HBS."));
	config->featList.push_back(AppFeature(20131224,102623,0,"bao","Fix for importing historic .ext and .zmap files."));
	config->featList.push_back(AppFeature(20131227,155506,0,"bao","Don't put international symbols into the ISIN map."));
	config->featList.push_back(AppFeature(20131230,175845,2763,"bao","DEV-2763: Fixed heartbeat bounce log message."));
	config->featList.push_back(AppFeature(20131231,144725,2905,"bao","DEV-2905: Use 30-second idle timeout for completion on RTSYM queries."));
	config->featList.push_back(AppFeature(20131231,155208,0,"bao","Added setup.ini::CON#_URL for $REDIOLEDB$ destination override."));
	config->featList.push_back(AppFeature(20140109,152251,0,"bao","Truncate historic .dat files on import."));
	config->featList.push_back(AppFeature(20140109,152251,3103,"bao","DEV-3103: Delay load of historic indices."));
	config->featList.push_back(AppFeature(20140122,180913,0,"bao","Handle BTR conversion for short exempt (54=6) orders."));
	config->featList.push_back(AppFeature(20140123,173605,3284,"bao","DEV-3284: Added redibustask sysmon console command"));
	config->featList.push_back(AppFeature(20140124,173134,3284,"bao","DEV-3284: Added reloadredibusini sysmon console command"));
	config->featList.push_back(AppFeature(20140206,94916,0,"bao","Fix false error when block reads split between CR and LF"));
	config->featList.push_back(AppFeature(20140218,152401,4130,"bao","DEV-4130: Set previous BTR.TradeRecA.EcnOrderNo on cancelled reports."));
	config->featList.push_back(AppFeature(20140228,155433,0,"bao","Minor query fix for setup.ini::INDEX_ORDERDATE:0."));
	config->featList.push_back(AppFeature(20140228,155433,0,"bao","Query performance enhancement for MLEG orders."));
	config->featList.push_back(AppFeature(20140228,155433,4436,"bao","DEV-4436: IQ doneaway LastPrice."));
	config->featList.push_back(AppFeature(20140228,155433,4570,"bao","DEV-4570: Added DoneAwayAccounts.csv and DoneAwayUsers.ini."));
	config->featList.push_back(AppFeature(20140304,100017,4570,"bao","DEV-4570: Added setup.ini::LOAD_DONE_AWAY_FILES"));
	config->featList.push_back(AppFeature(20140304,100017,0,"bao","Added setup.ini::HISTORIC_BEGINDATE"));
	config->featList.push_back(AppFeature(20140306,151148,4438,"bao","DEV-4438: Set 76=PrefMMID for inbound doneaways"));
	config->featList.push_back(AppFeature(20140307,141738,3781,"bao","DEV-3781: Key FirstClOrdID, RootOrderID, and ClientID for OATS files"));
	config->featList.push_back(AppFeature(20140307,141738,3781,"bao","DEV-3781: Added setup.ini::RTOATS_IMPORT_TIME to unhold CON$RTOATS$ ports"));
	config->featList.push_back(AppFeature(20140310,174150,0,"bao","Fix for MLEG confirm before unsolicited reject with different tag 11 and 41 values"));
	config->featList.push_back(AppFeature(20140310,174150,0,"bao","Fix for hist instance for day after DST takes effect."));
	#ifdef MULTI_DAY_ITEMLOC
	config->featList.push_back(AppFeature(20140311,90856,4762,"bao","DEV-4762: Fix under MULTI_DAY_ITEMLOC."));
	#endif
	config->featList.push_back(AppFeature(20140311,172055,4770,"bao","DEV-4770: Accept msg 5000 logins."));
	config->featList.push_back(AppFeature(20140318,144338,4881,"bao","DEV-4881: Truncate historic .csv files on import to hist instance."));
	config->featList.push_back(AppFeature(20140318,144338,4881,"bao","DEV-4881: Added filereport sysmon console command."));
	config->featList.push_back(AppFeature(20140318,144338,4881,"bao","DEV-4881: Prefix $ to RTPOSITION instances and pull positions by firm."));
	config->featList.push_back(AppFeature(20140404,152530,4836,"bao","DEV-4836: Set TradeRecA.Memo from tag 17303."));
	config->featList.push_back(AppFeature(20140409,195600,5577,"bao","DEV-5577: Added $RTAWAY$ protocol."));
	config->featList.push_back(AppFeature(20140410,161557,5577,"bao","DEV-5577: Added optional setup.ini::DONE_AWAY_USER_TAG"));
	config->featList.push_back(AppFeature(20140414,180026,5577,"bao","DEV-5577: Moved all doneway lookups to setup.ini::DONE_AWAY_LOOKUP"));
	config->featList.push_back(AppFeature(20140417,151835,5577,"bao","DEV-5577: Enable DONE_AWAY_USER_TAG=50 lookup over GD BTR connection"));
	config->featList.push_back(AppFeature(20140421,110537,5726,"bao","DEV-5726: Fix FIXDropProxy connection sequence gap issue"));
	config->featList.push_back(AppFeature(20140425,170249,5996,"bao","DEV-5996: Added setup.ini::TUSERMAP_PATH"));
	config->featList.push_back(AppFeature(20140425,170249,5966,"bao","DEV-5966: Added setup.ini::TACMAP_SYSTEM and TACMAP_PREFIX."));
	config->featList.push_back(AppFeature(20140429,155058,5577,"bao","DEV-5577: Make doneaway tag 17 unique for TRIAD"));
	config->featList.push_back(AppFeature(20140514,163024,6004,"bao","DEV-6004: Fixed query response for system with no instances."));
	config->featList.push_back(AppFeature(20140520,171729,6538,"bao","DEV-6538: Added setup.ini::TUSERMAP_PATH and TUSERDOMAIN_INI"));
	config->featList.push_back(AppFeature(20140523,171420,6516,"bao","DEV-6516: Fix stopped updates to CL and OG"));
	config->featList.push_back(AppFeature(20140609,135107,6862,"bao","DEV-6862: Added $RTECHOATS$ for drop missing tag 41."));
	config->featList.push_back(AppFeature(20140612,173848,6695,"bao","DEV-6695: Identify RTECHOATS done-away executions."));
	config->featList.push_back(AppFeature(20140618,102354,0,"bao","Tweak gap fill messaging."));
	config->featList.push_back(AppFeature(20140619,94014,6684,"bao","DEV-6684: Send settlement to CL."));
	config->featList.push_back(AppFeature(20140623,133514,7117,"bao","DEV-7117: Send busts and corrections to CL."));
	config->featList.push_back(AppFeature(20140707,171438,7240,"bao","DEV-7240: RTECHOATS support."));
	config->featList.push_back(AppFeature(20140815,154106,0,"bao","Harden ReadDetailThread against crash when the source file is corrupted."));
	config->featList.push_back(AppFeature(20140815,154106,8465,"bao","DEV-8465: Expose algo strategy and commission."));
	config->featList.push_back(AppFeature(20140815,154106,0,"bao","Fix accounting on total order quantity at instance level."));
	config->featList.push_back(AppFeature(20140819,122706,0,"amaralj","Start of DEV-8880"));
	config->featList.push_back(AppFeature(20140825,100926,0,"bao","32-bit Redibus build for BROKERALGO2 query."));
	config->featList.push_back(AppFeature(20140910,92326,0,"bao","Exclude doneaway shares from instance totals."));
	config->featList.push_back(AppFeature(20140910,92326,0,"bao","Set TradeRec.Shares to pending quantity."));
	config->featList.push_back(AppFeature(20141008,121841,7947,"bao","OATS override dbcommon.h."));
	config->featList.push_back(AppFeature(20141008,121841,0,"rapp","Support ExecBrokerMPID."));
	config->featList.push_back(AppFeature(20141008,121841,3808,"rapp","DEV-3808: Support OATS overrides tags."));
	config->featList.push_back(AppFeature(20150127,205718,13076,"bao","DEV-13076: Handle queries for trades after GMT midnight."));
	config->featList.push_back(AppFeature(20150306,174000,0,"bao","CLEANUP: Tag 9961 mis-numbered."));
	config->featList.push_back(AppFeature(20150602,160713,16679,"bao","DEV-16679: Set tag 76 to PrefECN and tag 30 to MMId."));
	config->featList.push_back(AppFeature(20150605,122019,16679,"bao","DEV-16679: Enrich bust and correct log message."));
	config->featList.push_back(AppFeature(20150617,152210,16679,"bao","DEV-16679: Rolled back tag 76 and 30 change."));
	config->featList.push_back(AppFeature(20151223,102900,22191,"bao","DEV-22191: Drop product type and currency for allocations 2."));
	config->featList.push_back(AppFeature(20160105,140200,22191,"bao","DEV-22191: Process tag 442 for future spread."));
	config->featList.push_back(AppFeature(20160122,153600,22005,"bao","DEV-22005: Fix thread crash on cancel after GMT midnight."));
	config->featList.push_back(AppFeature(20160209,105500,23542,"bao","DEV-23542: Load production futures from coverage tool for testing."));
	config->featList.push_back(AppFeature(20160219,164200,23769,"bao","DEV-23769: Generate TSXList-yyyymmdd.txt file from RTSYM_EQ.csv."));
	config->featList.push_back(AppFeature(20160222,122300,23788,"bao","DEV-23788: Report OATS for all international symbols with US equivalents."));
	config->featList.push_back(AppFeature(20160226,94700,23769,"bao","DEV-23769: Preserve STAGING extension for TSXList-yyyymmdd.txt."));
	config->featList.push_back(AppFeature(20160510,110700,23769,"bao","DEV-22005: Fix zero BTR FileDate on live details."));
	config->featList.push_back(AppFeature(20160510,180600,23769,"bao","DEV-22005: Fix mem leak when HAVE_FUSION_IO:1."));
	config->featList.push_back(AppFeature(20160515,100000,23769,"bao","DEV-22005: Prevent CRT Invalid parameter detected on BTR::ECNParserOrderNo and BTR::CancelMemo."));
	config->featList.push_back(AppFeature(20160518,222200,25004,"bao","DEV-25004: Enable VSDist Rollover Past Midnight."));
	config->featList.push_back(AppFeature(20160525,143300,25062,"bao","DEV-25062: Add region to offset index date."));
	config->featList.push_back(AppFeature(20160527,104600,25077,"bao","DEV-25077: Import historical data more than once per day."));
	config->featList.push_back(AppFeature(20160616,175000,25320,"bao","DEV-25320: Harden against string overflow."));
	config->featList.push_back(AppFeature(20160711,135100,25320,"bao","DEV-25320: Temporarily enable DEBUG_JOURNAL to try and catch cancel/replace issue"));
	config->featList.push_back(AppFeature(20160719,160500,25320,"bao","DEV-25320: Fix false-positive jpage gap error under DEBUG_JOURNAL"));
	config->featList.push_back(AppFeature(20160719,170400,25320,"bao","DEV-25320: Set FirstClOrdID to tag 41 even when the value hasn't been seen before"));
	config->featList.push_back(AppFeature(20160831,163500,26159,"bao","DEV-26159: Accept 77=1 for closing position"));
	config->featList.push_back(AppFeature(20161020,163500,26766,"jon","DEV-26766: set BTR.TRB.ECNExecID (27 chars) to Cl to support allocations"));
	config->featList.push_back(AppFeature(20161201,143500,27137,"jon","DEV-27137: Support 40 character ExecIDs to Cl to support allocations"));
	config->featList.push_back(AppFeature(20170124,92500,27628,"bao","DEV-27628: Disable inbound fix doneaway account translations"));
	config->featList.push_back(AppFeature(20170524,92500,27628,"jon","DEV-27318: Persist client memo field (25 characters) on allocation requests"));
	config->featList.push_back(AppFeature(20170612,155800,29121,"bao","DEV-29121: Don't swap order ids for cancel reject and order rejects"));
	config->featList.push_back(AppFeature(20171128, 178800, 1099, "jon", "REDICP-1099: Add MiFID II Fields"));
	config->featList.push_back(AppFeature(20180108, 178800, 1204, "jon", "REDICP-1099: Add MiFID II Fields (Order Ack fields)"));
	config->featList.push_back(AppFeature(20180129, 178800, 1159, "jon", "REDICP-1159: Support milliseconds timestamp"));
	config->featList.push_back(AppFeature(20180425, 123100, 1347, "jon", "REDICP-1347: REDI Principle Trades (Agency Reported as Principle)"));
#ifdef IQDEVTEST
	config->featList.push_back(AppFeature(0,0,0,"bao","IQDEVTEST build: Do not use for production!!!"));
	#endif

	WSLogEvent("Starting VSDB %s %s",__WSDATE__,__WSTIME__);
#ifdef WIN32
	int argc=0;
	WCHAR **argv=CommandLineToArgvW(str2w(GetCommandLine()),&argc);
	for(int i=1;i<argc;i++)
	{
		const char *arg=w2str(argv[i]);
		if((!_stricmp(arg,"/d"))&&(i<argc-1))
		{
			WSDATE=atoi(w2str(argv[++i]));
			WSLogEvent("Loading previous date %08d only.",WSDATE);
		}
		// This is handled in latest iqmatrix
		//else if((!_stricmp(arg,"/startdelay"))&&(i<argc-1))
		//{
		//	arg=w2str(argv[++i]);
		//	SleepEx(atol(arg)*1000,true);
		//}
		//else if((!_stricmp(arg,"/startwait"))&&(i<argc-2))
		//{
		//	arg=w2str(argv[++i]);
		//	int pid=atoi(arg);
		//	arg=w2str(argv[++i]);
		//	int maxWait=atoi(arg);
		//	HANDLE phnd=OpenProcess(PROCESS_TERMINATE|SYNCHRONIZE,false,pid);
		//	if(phnd)
		//	{
		//		WSLogEvent("Waiting up to %d seconds for process %d to exit...",maxWait,pid);
		//		WSBusyThreadReport(GetTickCount() +maxWait*1000);
		//		if(WaitForSingleObject(phnd,maxWait*1000)==WAIT_TIMEOUT)
		//		{
		//			// Need to kill it
		//			WSLogError("Previous process %d timed out--Terminating...",pid);
		//			if(TerminateProcess(phnd,9))
		//				WSLogError("Process %d terminated.",pid);
		//			else
		//				WSLogError("Failed terminating process %d!",pid);
		//		}
		//		else
		//			WSLogEvent("Previous process %d just exited.",pid);
		//		WSBusyThreadReport();
		//		CloseHandle(phnd);
		//	}
		//	else 
		//		WSLogEvent("Previous process %d already exited.",pid);
		//}
	}
#else
	// TODO: unfinished WSHInitModule /startwait
#endif

	// Task manager
	taskUser.authed=true;
	if(taskmgr.Startup(NO_OF_USR_PORTS,256)<0)
		return -1;
	// setup.ini parameters
	REGION=pcfg->aname.c_str();
	if(LoadSetupIni()<0)
		return -1;

#ifdef TASK_THREAD_POOL
	#ifdef WIN32
	taskIocpPort=CreateIoCompletionPort(INVALID_HANDLE_VALUE,0,(ULONG_PTR)this,4);
	for(int i=0;i<4;i++)
	{
		taskThreadAbort=false;
		DWORD tid;
		taskThreads[i]=CreateThread(0,0,_BootTaskThread,this,0,&tid);
	}
	#endif
#endif

	// TCP/IP
	WSSetAppStatus((char*)"ViewServer 1.0:",(char*)"Initializing Ports");
	WSResetErrorCnt();
	if(WSInitSocks()<0)
		return -1;
	WSSuspend();
	WSSetAppHead((char*)HEADING.c_str());
	// Buffering limits to prevent mem overflow
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].RemoteIP[0])
			continue;
		ConPort[i].RecvBuffLimit=100*1024; // 100MB
		ConPort[i].SendBuffLimit=100*1024; // 100MB
	#ifdef REDIOLEDBCON
		if(!strncmp(ConPort[i].CfgNote,"$REDIOLEDB$",11))
			ConPort[i].ShellPort=true;
	#endif
	}
	for(int i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(!UscPort[i].LocalIP[0])
			continue;
		UscPort[i].RecvBuffLimit=100*1024; // 100MB
		UscPort[i].SendBuffLimit=100*1024; // 100MB
	}

	int QueryTZNextDate=CalcTPlusDate(WSDATE?WSDATE:WSDate,+1,false);
	GetTimeZoneBias(QUERY_TIMEZONE.c_str());
	GetTradeRegion(TRADE_REGION.c_str(),TRADE_MARKET_OFFSET);

#ifdef REDIOLEDBCON
	if(IMPORT_REDIBUS)
		LoadRedibusIni();
	if(VSDIST_HUB)
	{
		LoadRediDestCsv();
		LoadAlgoStrategyCsv();
		LoadLastMarketDestTxt();
		LoadTUserMapCsv();
	}
	if(LOAD_DONE_AWAY_FILES)
	{		
		LoadDoneAwayUsersIni();
		LoadDoneAwayAccountsCsv();
	}
#endif
	// Order database thread
	dbLoaded=dbLoadCxl=false;
#ifdef WIN32
	DWORD tid=0;
	dbLoadThread=CreateThread(0,0,_BootDBLoadThread,this,0,&tid);
	if(!dbLoadThread)
	{
		WSLogError("Failed creating _BootLoadDBThread!");
		return -1;
	}
#else
	pthread_attr_t attr;
	int Result=pthread_attr_init( &attr );
	Result=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE );
	if(pthread_create(&dbLoadThread,&attr,_BootDBLoadThread,this) || !dbLoadThread)
	{
		WSLogError("Failed creating _BootLoadDBThread!");
		return -1;
	}
#endif
	return 0;
}
int ViewServer::DBLoadThread()
{
	WSSetAppStatus((char*)"ViewServer 1.0:",(char*)"Initializing...");

	if(dbLoadCxl)
		return -1;
	// Start date
	int wsdate=WSDATE;
	if(!wsdate)
	{
	#ifdef WIN32
		SYSTEMTIME tsys,tloc;
		GetSystemTime(&tsys);
		SystemTimeToTzSpecificLocalTime(&TradeRegion,&tsys,&tloc);
		wsdate=(tloc.wYear*10000) +(tloc.wMonth*100) +(tloc.wDay);
	#else
		wsdate=::WSDate();
	#endif
	}
	char hstr[256]={0};
	if(WSDATE)
		sprintf(hstr,"Initializing %s for %08d (READONLY)",HEADING.c_str(),wsdate);
#ifdef MULTI_DAY_HIST
	else if(odb.HISTORIC_DAYS>0)
		sprintf(hstr,"Initializing %s for %d days (HISTORIC)",HEADING.c_str(),odb.HISTORIC_DAYS);
#endif
	else
		sprintf(hstr,"Initializing %s for %08d",HEADING.c_str(),wsdate);
	WSSetAppHead(hstr);
	WSSetAppStatus((char*)"ViewServer 1.0:",(char*)"Loading database...");
	// Order database
	char dbname[MAX_PATH]={0};
	sprintf(dbname,"VSDB_%08d.dat",wsdate);
	// Locking is required because ODBIndexOrder will be called
	Lock();
	// TODO: Add setup.ini setting
	char dbdir[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(dbdir,"%s\\data",pcfg->RunPath.c_str());
	#else
	sprintf(dbdir,"%s/data",pcfg->RunPath.c_str());
	#endif
	CreateDirectory(dbdir,0);
	//if(odb.Init(this,dbdir,dbname,wsdate,WSDATE?true:false,dbLoadCxl)<0)
	//{
	//	Unlock();
	//	return -1;
	//}
	WSSetAppStatus((char*)"ViewServer 1.0:",(char*)"odb Init...");
	odb.Init(this);
	char dbpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(dbpath,"%s\\%s",dbdir,dbname);
	#else
	sprintf(dbpath,"%s/%s",dbdir,dbname);
	#endif
	odb.fpath=dbpath;
	odb.wsdate=wsdate;
#ifdef MULTI_DAY_HIST
	odb.readonly=(WSDATE||odb.HISTORIC_DAYS>0)?true:false;
#else
	odb.readonly=WSDATE?true:false;
#endif

	//DT9044: The detail files need to be opened first to read the IQ AccountRecs from the dist side upon restarts
	if(IQCLFLOW)
	{
		if(OpenDetailFiles()<0)
			return -1;
	}

	if(LoadSystemConfig()<0)
		return -1;
	#ifdef DEBUG_ORDER_KEYS
	if(okfp)
	{
		fclose(okfp); okfp=0;
	}
	// Wet the line
	VSOrder *porder=odb.AllocOrder();
	odb.FreeOrder(porder); porder=0;
	#endif
	Unlock();
	#ifdef NOMAP_TERM_ORDERS
	DWORD otot=ocnt +oterm;
	int tperc=oterm*100/(otot?otot:1);
	WSLogEvent("Loaded %d database orders, %d terminated (%d%%)",otot,oterm,tperc);
	#else
	WSLogEvent("Loaded %d database orders",ocnt);
	#endif

	if(!IQCLFLOW)  //DT9044
	{
		if(OpenDetailFiles()<0)
			return -1;
	}
	WSLogEvent("OpenDetailFiles(), success calling WSResume()");

	WSResume();
	WSSetAppStatus((char*)"ViewServer 1.0:",(char*)"Running...");
	return 0;
}
int ViewServer::WSHCleanupModule()
{
#ifdef SPECTRUM
	SaveAppSystemsIni();
#endif

#ifdef _DEBUG
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].InUse)
			continue;
		ConPort[i].ConnectHold=true;
		WSCloseConPort(i);
	}
	for(int i=0;i<NO_OF_USR_PORTS;i++)
	{
		if(!UsrPort[i].SockActive)
			continue;
		WSCloseUsrPort(i);
	}
	if(dbLoadThread)
	{
		dbLoadCxl=true;
	#if defined(WIN32)||defined(_CONSOLE)
		#ifdef _DEBUG
		WaitForSingleObject((HANDLE)(PTRCAST)dbLoadThread,INFINITE);
		#else
		WaitForSingleObject((HANDLE)(PTRCAST)dbLoadThread,1000);
		#endif
	#else
		void *rc=0;
		pthread_join(dbLoadThread,&rc);
	#endif
	#ifdef WIN32
		CloseHandle(dbLoadThread); dbLoadThread=0;
	#endif
	}

#ifdef TASK_THREAD_POOL
	#ifdef WIN32
	taskThreadAbort=true;
	for(int i=0;i<4;i++)
	{
		if(taskThreads[i])
		{
		#ifdef _DEBUG
			WaitForSingleObject(taskThreads[i],INFINITE);
		#else
			WaitForSingleObject(taskThreads[i],1000);
		#endif
			CloseHandle(taskThreads[i]); taskThreads[i]=0;
		}
	}
	if(taskIocpPort)
	{
		CloseHandle(taskIocpPort); taskIocpPort=0;
	}
	#endif
#endif

	for(DCMAP::iterator dit=dcmap.begin();dit!=dcmap.end();dit++)
	{
		DropClient *pdc=dit->second;
		pdc->Shutdown();
		delete pdc;
	}
	dcmap.clear();
#ifdef FIX_SERVER
	for(DSMAP::iterator dit=dsmap.begin();dit!=dsmap.end();dit++)
	{
		DropServer *pds=dit->second;
		pds->Shutdown();
		delete pds;
	}
	dsmap.clear();
#endif

	CloseDetailFiles();
	mrulist.clear();
    WSCloseSocks();
	if(ffp)
	{
		fclose(ffp); ffp=0;
	}
	odb.Shutdown();
	taskmgr.Shutdown();

	for(ACCTMAP::iterator acit=acmap.begin();acit!=acmap.end();acit++)
	{
		Account *pacc=acit->second;
		if((pacc->AcctType!="AppSystem")&&(pacc->AcctType!="AppInstance"))
			delete pacc;
	}
	acmap.clear();
	aimap.clear();
	if(asbrowse)
	{
		delete asbrowse; asbrowse=0;
	}
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		delete asys;
	}
	asmap.clear();
#else
	// In production, don't chance a hung process
	CloseDetailFiles();
	_exit(0);
#endif
    return 0;
}

static void btrim(char *Value)
{
     char *s, *d;

     d = s = Value;								   /* point d to string base  */
     while (*s && (*s == ' ' || *s == '\"'))       /* skip over first spaces */
          ++s;
     
	 while ((*d++ = *s++) != '\0');                /* copy until null        */

     d = s = Value;
     
	 s += (strlen(Value) - 1);
     if (s <= d)
        return;
     
	 while (s >= d && (*s == '\0' || *s == ' ' || *s == '\"' || *s == '\t' || *s == '\r' || *s == '\n')) 
	 {    /* find end of string */
         *s = '\0';
         --s;
     }
}
int ViewServer::LoadSetupIni()
{
	char ErrText[1024]={0};
	HEADING=pcfg->aname;
	char fpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(fpath,"%s\\setup\\setup.ini",pcfg->RunPath.c_str());
	#else
	sprintf(fpath,"%s/setup/setup.ini",pcfg->RunPath.c_str());
	#endif
	list<pair<int,string>> nCON_URL_LIST;
	list<HistImport> nhistImports;
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			if((fp=fopen(fpath,"wt"))==NULL)
			{
				sprintf(ErrText,"IQ-b3-e0: Could not open %s",fpath);
				goto Error;
			}
			fprintf(fp,"HEADING:%s\n",HEADING.c_str());
			fprintf(fp,"MAX_USR_TIME:%d\n",MAX_USR_TIME);
			fprintf(fp,"MAX_CON_TIME:%d\n",MAX_CON_TIME);
			fprintf(fp,"SERVER_EXIT_TIME:%d\n",SERVER_EXIT_TIME);
			fprintf(fp,"ORDER_CACHE_SIZE:%d\n",odb.ORDER_CACHE_SIZE);
			fprintf(fp,"VSDIST_HUB:%d\n",VSDIST_HUB?1:0);
			fprintf(fp,"MAX_DISTJ_SIZE:%d\n",MAX_DISTJ_SIZE);
			fprintf(fp,"INDEX_ROOTORDERID:%d\n",odb.INDEX_ROOTORDERID?1:0);
			fprintf(fp,"INDEX_FIRSTCLORDID:%d\n",odb.INDEX_FIRSTCLORDID?1:0);
			fprintf(fp,"INDEX_CLPARENTORDERID:%d\n",odb.INDEX_CLPARENTORDERID?1:0);
			fprintf(fp,"INDEX_SYMBOL:%d\n",odb.INDEX_SYMBOL?1:0);
			fprintf(fp,"INDEX_ACCOUNT:%d\n",odb.INDEX_ACCOUNT?1:0);
			fprintf(fp,"INDEX_ECNORDERID:%d\n",odb.INDEX_ECNORDERID?1:0);
			fprintf(fp,"INDEX_CLIENTID:%d\n",odb.INDEX_CLIENTID?1:0);
			fprintf(fp,"INDEX_TRANSACTTIME:%d\n",odb.INDEX_TRANSACTTIME?1:0);
			fprintf(fp,"INDEX_FILLED_ORDERS:%d\n",odb.INDEX_FILLED_ORDERS?1:0);
			fprintf(fp,"INDEX_OPEN_ORDERS:%d\n",odb.INDEX_OPEN_ORDERS?1:0);
			fprintf(fp,"INDEX_CONNECTIONS:%d\n",odb.INDEX_CONNECTIONS?1:0);
		#ifdef MULTI_DAY
			fprintf(fp,"INDEX_ORDERDATE:%d\n",odb.INDEX_ORDERDATE?1:0);
		#endif
			fprintf(fp,"INDEX_AUXKEY:%d\n",odb.INDEX_AUXKEYS); //DT9398
			//DT9491- Case sensitivity
			fprintf(fp,"INDEX_CLORDID_CS:%d\n",odb.INDEX_CLORDID_CS?1:0);
			fprintf(fp,"INDEX_ROOTORDERID_CS:%d\n",odb.INDEX_ROOTORDERID_CS?1:0);
			fprintf(fp,"INDEX_FIRSTCLORDID_CS:%d\n",odb.INDEX_FIRSTCLORDID_CS?1:0);
			fprintf(fp,"INDEX_SYMBOL_CS:%d\n",odb.INDEX_SYMBOL_CS?1:0);
			fprintf(fp,"INDEX_ACCOUNT_CS:%d\n",odb.INDEX_ACCOUNT_CS?1:0);
			fprintf(fp,"INDEX_ECNORDERID_CS:%d\n",odb.INDEX_ECNORDERID_CS?1:0);
			fprintf(fp,"INDEX_CLIENTID_CS:%d\n",odb.INDEX_CLIENTID_CS?1:0);
			fprintf(fp,"INDEX_CONNECTIONS_CS:%d\n",odb.INDEX_CONNECTIONS_CS?1:0);
			fprintf(fp,"INDEX_CLPARENTORDERID_CS:%d\n",odb.INDEX_CLPARENTORDERID_CS?1:0);
			fprintf(fp,"INDEX_AUXKEYS_CS:%d\n",odb.INDEX_AUXKEYS_CS?1:0);
			fprintf(fp,"//AUXKEY_NAME:\n");
			fprintf(fp,"//AUXKEY_TAG:\n");
			fprintf(fp,"FILLS_FILE_DIR:%s\n",FILLS_FILE_DIR.c_str());
			fprintf(fp,"HAVE_FUSION_IO:%d\n",odb.HAVE_FUSION_IO?1:0);
			fprintf(fp,"LOG_QUERIES:%d\n",LOG_QUERIES);
			fprintf(fp,"LOG_BROWSING:%d\n",LOG_BROWSING);
			fprintf(fp,"IQFLOW:%d\n",IQFLOW?1:0);
			fprintf(fp,"IQCLFLOW:%d\n",IQCLFLOW?1:0); //DT9044
			fprintf(fp,"REGION:%s\n",pcfg->aname.c_str());
			fprintf(fp,"REGION_PASS:%s\n",REGION_PASS.c_str());
			fprintf(fp,"RETRANS_TIMEOUT:%d\n",RETRANS_TIMEOUT);
			fprintf(fp,"KEEP_DAYS:%d\n",KEEP_DAYS);
		#ifdef IQDNS
			fprintf(fp,"SAM_REPORT_TIME:%d\n",SAM_REPORT_TIME);
			fprintf(fp,"SAM_UPLOAD_DIR:%s\n",SAM_UPLOAD_DIR.c_str());
		#endif
		#ifdef MULTI_DAY_HIST
			fprintf(fp,"HISTORIC_DAYS:%d\n",odb.HISTORIC_DAYS);
			fprintf(fp,"HISTIMP_TIME:%06d\n",HISTIMP_TIME);
			fprintf(fp,"HISTIMP_DIST_PATHS:%s\n",HISTIMP_DIST_PATHS.c_str());
			fprintf(fp,"HISTIMP_HUB_PATHS:%s\n",HISTIMP_HUB_PATHS.c_str());
			fprintf(fp,"HISTORIC_ONDEMAND:%d\n",HISTORIC_ONDEMAND?1:0);
			fprintf(fp,"HISTORIC_BEGINDATE:%d\n",HISTORIC_BEGINDATE);
		#endif
		#ifdef REDIOLEDBCON
			fprintf(fp,"IMPORT_REDIBUS:%d\n",IMPORT_REDIBUS?1:0);
			fprintf(fp,"REDIBUS_USER:%s\n",REDIBUS_USER.c_str());
			fprintf(fp,"REDIBUS_PASS:%s\n",REDIBUS_PASS.c_str());
			fprintf(fp,"REDIBUS_HEARTBEAT:%d\n",REDIBUS_HEARTBEAT);
			fprintf(fp,"REDIDESTINATION_PATH:%s\n",REDIDESTINATION_PATH.c_str());
			fprintf(fp,"ALGOSTRATEGY_PATH:%s\n",ALGOSTRATEGY_PATH.c_str());
			fprintf(fp,"REDIOLEDB_PATH:%s\n",REDIOLEDB_PATH.c_str());
			fprintf(fp,"LOAD_DONE_AWAY_FILES:%d\n",LOAD_DONE_AWAY_FILES);
			//fprintf(fp,"DONE_AWAY_ACCOUNTS_PATH:%s\n",DONE_AWAY_ACCOUNTS_PATH.c_str());
			//fprintf(fp,"DONE_AWAY_USERS_PATH:%s\n",DONE_AWAY_USERS_PATH.c_str());
			//fprintf(fp,"DONE_AWAY_USER_TAG:%d\n",DONE_AWAY_USER_TAG);
			fprintf(fp,"RTOATS_IMPORT_TIME:%d\n",RTOATS_IMPORT_TIME);
			fprintf(fp,"TUSERMAP_PATH:%s\n",TUSERMAP_PATH.c_str());
			fprintf(fp,"TUSERDOMAIN_INI:%s\n",TUSERDOMAIN_INI.c_str());			
		#endif
			fprintf(fp,"FIX_HOURS_BEGIN:%06d\n",FIX_HOURS_BEGIN);
			fprintf(fp,"FIX_HOURS_END:%06d\n",FIX_HOURS_END);
			fprintf(fp,"TRADE_BUST_ERRORS:%d\n",TRADE_BUST_ERRORS?1:0);
			// DEV-12136: Default broker code behavior
			fprintf(fp,"DEFAULT_ROUTEBROKER_ON_ORDERS:%s\n",DEFAULT_ROUTEBROKER_ON_ORDERS.c_str());
			fprintf(fp,"DEFAULT_ROUTEBROKER_ON_TICKETS:%s\n",DEFAULT_ROUTEBROKER_ON_TICKETS.c_str());
			fprintf(fp,"DEFAULT_ROUTEBROKER_ON_DONEAWAYS:%s\n",DEFAULT_ROUTEBROKER_ON_DONEAWAYS.c_str());
			fprintf(fp,"QUERY_TIMEZONE:%s\n",QUERY_TIMEZONE.c_str());
			fprintf(fp,"TRADE_REGION:%s\n",TRADE_REGION.c_str());
			fprintf(fp,"TRADE_MARKET_OFFSET:%d\n",TRADE_MARKET_OFFSET);
			fclose(fp);
			return 0;
		}
		sprintf(ErrText,"IQ-b3-e0: Could not open %s",fpath);
		goto Error;
	}
	{//scope
	int lno=0;
	char rbuf[1024]={0};
	char sname[1024]={0};
	char value[1024]={0};
	while(fgets(rbuf,1023,fp)!=NULL)
	{
		lno++;
        if(strncmp(rbuf,"//",2)==0)
            continue;
		char *Inptr;
		if((Inptr=strtok(rbuf,":\n\r"))==NULL)
			continue;
		if(Inptr[0]==0)
			continue;
		strcpy(sname,Inptr);
		btrim(value);
		if((Inptr=strtok(NULL,"\n\r"))==NULL)
		{
			sprintf(ErrText,"setup.ini: Empty value at line %d!",lno);
			goto Error;
		}
		strcpy(value,Inptr);
		btrim(value);

        if(strcmp(sname,"HEADING")==0)
		{
			HEADING=value;
		}
		else if(strcmp(sname,"MAX_USR_TIME")==0)
		{
			MAX_USR_TIME=atoi(value);
		}
		else if(strcmp(sname,"MAX_CON_TIME")==0)
		{
			MAX_CON_TIME=atoi(value);
		}
		else if(strcmp(sname,"SERVER_EXIT_TIME")==0)
		{
			SERVER_EXIT_TIME=atoi(value);
		}
		else if(strcmp(sname,"ORDER_CACHE_SIZE")==0)
		{
			odb.ORDER_CACHE_SIZE=atoi(value);
		}
		else if(strcmp(sname,"VSDIST_HUB")==0)
		{
			VSDIST_HUB=atoi(value)?true:false;
		}
		else if(strcmp(sname,"MAX_DISTJ_SIZE")==0)
		{
			MAX_DISTJ_SIZE=atoi(value);
		}
		else if(strcmp(sname,"INDEX_ROOTORDERID")==0)
		{
			odb.INDEX_ROOTORDERID=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_FIRSTCLORDID")==0)
		{
			odb.INDEX_FIRSTCLORDID=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_CLPARENTORDERID")==0)
		{
			odb.INDEX_CLPARENTORDERID=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_SYMBOL")==0)
		{
			odb.INDEX_SYMBOL=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_ACCOUNT")==0)
		{
			odb.INDEX_ACCOUNT=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_ECNORDERID")==0)
		{
			odb.INDEX_ECNORDERID=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_CLIENTID")==0)
		{
			odb.INDEX_CLIENTID=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_TRANSACTTIME")==0)
		{
			odb.INDEX_TRANSACTTIME=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_FILLED_ORDERS")==0)
		{
			odb.INDEX_FILLED_ORDERS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_OPEN_ORDERS")==0)
		{
			odb.INDEX_OPEN_ORDERS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_CONNECTIONS")==0)
		{
			odb.INDEX_CONNECTIONS=atoi(value)?true:false;
		}		
	#ifdef MULTI_DAY
		else if(strcmp(sname,"INDEX_ORDERDATE")==0)
		{
			odb.INDEX_ORDERDATE=atoi(value)?true:false;
		}		
	#endif
		//DT9398
		else if(strcmp(sname,"INDEX_AUXKEY")==0)
		{
			odb.INDEX_AUXKEYS=atoi(value);
		}		
		else if(strcmp(sname,"AUXKEY_NAME")==0)
		{
			char *tok=strtoke(value,",");
			int i=0;
			while(tok && i<AUX_KEYS_MAX_NUM)
			{
				odb.AUXKEY_NAMES[i]=tok;
				tok=strtoke(0,",");
				i++;
			}
		}		
		else if(strcmp(sname,"AUXKEY_TAG")==0)
		{
			char *tok=strtoke(value,",");
			int i=0;
			while(tok && i<AUX_KEYS_MAX_NUM)
			{
				odb.AUXKEY_TAGS[i]=atoi(tok);
				tok=strtoke(0,",");
				i++;
			}
		}
	#ifdef AUXKEY_EXPR
		else if(strncmp(sname,"AUXKEY_NAME",11)==0)
		{
			int i=atoi(sname+11);
			if((i>0)&&(i<AUX_KEYS_MAX_NUM))
			{
				if(i>(int)odb.INDEX_AUXKEYS)
					odb.INDEX_AUXKEYS=i;
				odb.AUXKEY_NAMES[i-1]=value;
			}
			else
			{
				sprintf(ErrText,"Invalid AUXKEY_NAME# index(%s)!",sname);
				goto Error;
			}
		}
		else if(strncmp(sname,"AUXKEY_ORD_EXPR",15)==0)
		{
			int i=atoi(sname+15);
			if((i>0)&&(i<AUX_KEYS_MAX_NUM))
			{
				if(i>(int)odb.INDEX_AUXKEYS)
					odb.INDEX_AUXKEYS=i;
				odb.AUXKEY_ORD_EXPR[i-1]=value;
			}
			else
			{
				sprintf(ErrText,"Invalid AUXKEY_ORD_EXPR# index(%s)!",sname);
				goto Error;
			}
		}
		else if(strncmp(sname,"AUXKEY_EXEC_EXPR",16)==0)
		{
			int i=atoi(sname+16);
			if((i>0)&&(i<AUX_KEYS_MAX_NUM))
			{
				if(i>(int)odb.INDEX_AUXKEYS)
					odb.INDEX_AUXKEYS=i;
				odb.AUXKEY_EXEC_EXPR[i-1]=value;
			}
			else
			{
				sprintf(ErrText,"Invalid AUXKEY_EXEC_EXPR# index(%s)!",sname);
				goto Error;
			}
		}
	#endif
		else if(strcmp(sname,"FILLS_FILE_DIR")==0)
		{
			FILLS_FILE_DIR=value;
		}
		else if(strcmp(sname,"HAVE_FUSION_IO")==0)
		{
			odb.HAVE_FUSION_IO=atoi(value)?true:false;
		}
		else if(strcmp(sname,"LOG_QUERIES")==0)
		{
			LOG_QUERIES=atoi(value);
		}
		else if(strcmp(sname,"LOG_BROWSING")==0)
		{
			LOG_BROWSING=atoi(value);
		}
		else if(strcmp(sname,"IQFLOW")==0)
		{
			IQFLOW=atoi(value)?true:false;
		}
		else if(strcmp(sname,"IQCLFLOW")==0)
		{
			IQCLFLOW=atoi(value)?true:false;
		}
		else if(strcmp(sname,"REGION")==0)
		{
			REGION=value;
		}
		else if(strcmp(sname,"REGION_PASS")==0)
		{
			REGION_PASS=value;
		}
		else if(strcmp(sname,"RETRANS_TIMEOUT")==0)
		{
			RETRANS_TIMEOUT=atoi(value);
		}
		else if(strcmp(sname,"KEEP_DAYS")==0)
		{
			KEEP_DAYS=atoi(value);
		}
	#ifdef IQDNS
		else if(strcmp(sname,"SAM_REPORT_TIME")==0)
		{
			SAM_REPORT_TIME=atoi(value);
		}		
		else if(strcmp(sname,"SAM_UPLOAD_DIR")==0)
		{
			SAM_UPLOAD_DIR=value;
		}
	#endif
	#ifdef MULTI_DAY_HIST
		else if(strcmp(sname,"HISTORIC_DAYS")==0)
		{
			odb.HISTORIC_DAYS=atoi(value);
			WSLogEvent("HISTORIC_DAYS: %d",odb.HISTORIC_DAYS);
		}
		else if(strcmp(sname,"HISTIMP_TIME")==0)
		{
			HISTIMP_TIME=atoi(value);
			WSLogEvent("HISTIMP_TIME: %d", HISTIMP_TIME);
		}
		else if(strcmp(sname,"HISTIMP_DIST_PATHS")==0)
		{
			HISTIMP_DIST_PATHS=value;
			WSLogEvent("HISTIMP_DIST_PATHS: %s", HISTIMP_DIST_PATHS.c_str());
		}
		else if(strcmp(sname,"HISTIMP_HUB_PATHS")==0)
		{
			HISTIMP_HUB_PATHS=value;
			WSLogEvent("HISTIMP_HUB_PATHS: %s", HISTIMP_HUB_PATHS.c_str());
		}
		else if(strcmp(sname,"HISTIMP_INSTANCE")==0)
		{
			HistImport import;
			int col=0;
			for(const char *tok=strtoke(value,",");tok;tok=strtoke(0,","))
			{
				switch(++col)
				{
				case 1: import.HISTIMP_TIME=atoi(tok); break;
				case 2: import.HISTIMP_DIST_PATH=tok; break;
				case 3: import.HISTIMP_HUB_PATH=tok; break;
				};
			}
			nhistImports.push_back(import);
			WSLogEvent("HISTIMP_INSTANCE HISTIMP_TIME: %d HISTIMP_DIST_PATH:%s HISTIMP_HUB_PATH:%s", nhistImports.back().HISTIMP_TIME, nhistImports.back().HISTIMP_DIST_PATH.c_str(), nhistImports.back().HISTIMP_HUB_PATH.c_str());
		}
		else if(strcmp(sname,"HISTORIC_ONDEMAND")==0)
		{
			HISTORIC_ONDEMAND=atoi(value)?true:false;
			WSLogEvent("HISTORIC_ONDEMAND: %d", HISTORIC_ONDEMAND);
		}
		else if(strcmp(sname,"HISTORIC_BEGINDATE")==0)
		{
			HISTORIC_BEGINDATE=atoi(value);
			WSLogEvent("HISTORIC_BEGINDATE: %d", HISTORIC_BEGINDATE);
		}
	#endif
	#ifdef REDIOLEDBCON
		else if(strcmp(sname,"IMPORT_REDIBUS")==0)
		{
			IMPORT_REDIBUS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"REDIBUS_USER")==0)
		{
			REDIBUS_USER=value;
		}
		else if(strcmp(sname,"REDIBUS_PASS")==0)
		{
			REDIBUS_PASS=value;
		}
		else if(strcmp(sname,"REDIBUS_HEARTBEAT")==0)
		{
			REDIBUS_HEARTBEAT=atoi(value);
		}
		else if(strcmp(sname,"REDIDESTINATION_PATH")==0)
		{
			REDIDESTINATION_PATH=value;
		}
		else if(strcmp(sname,"ALGOSTRATEGY_PATH")==0)
		{
			ALGOSTRATEGY_PATH=value;
		}
		else if(strcmp(sname,"REDIOLEDB_PATH")==0)
		{
			REDIOLEDB_PATH=value;
		}
		else if(strcmp(sname,"LOAD_DONE_AWAY_FILES")==0)
		{
			LOAD_DONE_AWAY_FILES=atoi(value)?true:false;
		}
		else if(strcmp(sname,"TRANSLATE_DONE_AWAY_USER")==0)
		{
			TRANSLATE_DONE_AWAY_USER=atoi(value)?true:false;
		}
		else if(strcmp(sname,"TRANSLATE_DONE_AWAY_ACCOUNT")==0)
		{
			TRANSLATE_DONE_AWAY_ACCOUNT=atoi(value)?true:false;
		}
		//else if(strcmp(sname,"DONE_AWAY_ACCOUNTS_PATH")==0)
		//{
		//	DONE_AWAY_ACCOUNTS_PATH=value;
		//}
		//else if(strcmp(sname,"DONE_AWAY_USERS_PATH")==0)
		//{
		//	DONE_AWAY_USERS_PATH=value;
		//}
		//else if(strcmp(sname,"DONE_AWAY_USER_TAG")==0)
		//{
		//	DONE_AWAY_USER_TAG=atoi(value);
		//}
		else if(strcmp(sname,"DONE_AWAY_LOOKUP")==0)
		{
			DoneAwayLookup dalu;
			string scid;
			int col=0;
			for(const char *tok=strtoke(value,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)
				{
				case 1: scid=tok; break;
				case 2: dalu.DONE_AWAY_USER_TAG=atoi(tok); break;
				case 3: dalu.DONE_AWAY_USERS_PATH=tok; break;
				case 4: dalu.DONE_AWAY_ACCOUNTS_PATH=tok; break;
				};
			}
			damap.insert(pair<string,DoneAwayLookup>(scid,dalu));
		}
		else if(strcmp(sname,"RTOATS_IMPORT_TIME")==0)
		{
			RTOATS_IMPORT_TIME=atoi(value);
			taskmgr.DelScheduledTask(&taskUser,TASK_IMPORT_RTOATS);
			if(RTOATS_IMPORT_TIME>0)
			{
				extern int _stdcall _RTOatsTask(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
				bool weekdays[]={0,1,1,1,1,1,0};
				if(taskmgr.AddScheduledTask(&taskUser,TASK_IMPORT_RTOATS,this,_RTOatsTask,0,RTOATS_IMPORT_TIME,true,true,weekdays,WSDate,WSTime))
					WSLogEvent("RTOATS_IMPORT_TIME task scheduled at %06d",RTOATS_IMPORT_TIME);
			}
		}
		else if(strcmp(sname,"TUSERMAP_PATH")==0)
		{
			TUSERMAP_PATH=value;
		}		
		else if(strcmp(sname,"TUSERDOMAIN_INI")==0)
		{
			TUSERDOMAIN_INI=value;
		}				
		else if(strcmp(sname,"REDIBUS_BOUNCE_MISSED_HBS")==0)
		{
			REDIBUS_BOUNCE_MISSED_HBS=atoi(value)?true:false;
		}
		else if(!strncmp(sname,"CON",3)&&(strrcmp(sname,"_URL")))
		{
			int cport=atoi(sname +3);
			if((cport>=0)&&(cport<NO_OF_CON_PORTS))
				nCON_URL_LIST.push_back(pair<int,string>(cport,value));
		}
		else if(strcmp(sname,"TACMAP_SYSTEM")==0)
		{
			TACMAP_SYSTEM=value;
		}
		else if(strcmp(sname,"TACMAP_PREFIX")==0)
		{
			TACMAP_PREFIX=value;
		}
	#endif
		else if(strcmp(sname,"FIX_HOURS_BEGIN")==0)
		{
			FIX_HOURS_BEGIN=atoi(value);
		}
		else if(strcmp(sname,"FIX_HOURS_END")==0)
		{
			FIX_HOURS_END=atoi(value);
		}		
		else if(strcmp(sname,"TRADE_BUST_ERRORS")==0)
		{
			TRADE_BUST_ERRORS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"QUERY_TIMEZONE")==0)
		{
			QUERY_TIMEZONE=value;
		}
		else if(strcmp(sname,"TRADE_REGION")==0)
		{
			TRADE_REGION=value;
		}
		else if(strcmp(sname,"TRADE_MARKET_OFFSET")==0)
		{
			TRADE_MARKET_OFFSET=atoi(value);
		}
		//DT9491
		else if(strcmp(sname,"INDEX_CLORDID_CS")==0)
		{
			odb.INDEX_CLORDID_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_ROOTORDERID_CS")==0)
		{
			odb.INDEX_ROOTORDERID_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_FIRSTCLORDID_CS")==0)
		{
			odb.INDEX_FIRSTCLORDID_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_SYMBOL_CS")==0)
		{
			odb.INDEX_SYMBOL_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_ACCOUNT_CS")==0)
		{
			odb.INDEX_ACCOUNT_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_ECNORDERID_CS")==0)
		{
			odb.INDEX_ECNORDERID_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_CLIENTID_CS")==0)
		{
			odb.INDEX_CLIENTID_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_CONNECTIONS_CS")==0)
		{
			odb.INDEX_CONNECTIONS_CS=atoi(value)?true:false;
		}		
		else if(strcmp(sname,"INDEX_CLPARENTORDERID_CS")==0)
		{
			odb.INDEX_CLPARENTORDERID_CS=atoi(value)?true:false;
		}
		else if(strcmp(sname,"INDEX_AUXKEYS_CS")==0)
		{
			odb.INDEX_AUXKEYS_CS=atoi(value)?true:false;
		}
		// DEV-12136: Default broker code behavior
		else if(strcmp(sname,"DEFAULT_ROUTEBROKER_ON_ORDERS")==0)
		{
			DEFAULT_ROUTEBROKER_ON_ORDERS=value;
		}
		else if(strcmp(sname,"DEFAULT_ROUTEBROKER_ON_TICKETS")==0)
		{
			DEFAULT_ROUTEBROKER_ON_TICKETS=value;
		}
		else if(strcmp(sname,"DEFAULT_ROUTEBROKER_ON_DONEAWAYS")==0)
		{
			DEFAULT_ROUTEBROKER_ON_DONEAWAYS=value;
		}
		else
		{
			sprintf(ErrText,"Unknown setup.ini setting(%s)!",sname);
			goto Error;
		}
	}
	fclose(fp);
	CON_URL_LIST.clear();
	CON_URL_LIST.swap(nCON_URL_LIST);
	histImports.clear();
	histImports.swap(nhistImports);

	if(FILLS_FILE_DIR.empty())
	{
		char fpath[MAX_PATH]={0};
		#ifdef WIN32
		sprintf(fpath,"%s\\data",pcfg->RunPath.c_str());
		#else
		sprintf(fpath,"%s/data",pcfg->RunPath.c_str());
		#endif
		FILLS_FILE_DIR=fpath;
	}
	if((IQFLOW)&&(!VSDIST_HUB))
		LoadMatchFile();
	if((VSDIST_HUB)&&(!odb.HAVE_FUSION_IO))
		WSLogError("WARNING: HAVE_FUSION_IO=0 with VSDIST_HUB=1 has not been tested for production use, and is only safe for dev testing!");

	WSLogEvent("Loaded %s",fpath);
	}//scope
	return 0;
Error:
	if(fp)
		fclose(fp);
	WSLogError(ErrText);
	return -1;
}
AppSystem *ViewServer::CreateSystem(APPSYSMAP& nasmap, const char *sname, int& scnt)
{
	string skey=(string)sname +"*";
	APPSYSMAP::iterator ait=nasmap.find(skey);
	if(ait!=nasmap.end())
		return ait->second;
	AppSystem *nasys=new AppSystem;
	nasys->sname=sname;
	nasys->skey=skey;
	nasmap[skey]=nasys; scnt++;
	return nasys;
}
int ViewServer::LoadSystemConfig()
{
	if(!VSDIST_HUB)
		return true;

	// Read the new configuration, but don't make changes until all files read
	char ErrText[1024]={0};
	APPSYSMAP nasmap_new;
	APPINSTMAP naimap_new;
	ACCTMAP nacmap_new;

    bool new_startup = (asmap.size() == 0);     // Mark if this function is called during a re-start
    APPSYSMAP& nasmap = (new_startup || importHistoricFiles ) ? asmap : nasmap_new;
    APPINSTMAP& naimap = (new_startup || importHistoricFiles) ? aimap : naimap_new;
    ACCTMAP& nacmap = (new_startup || importHistoricFiles ) ? acmap : nacmap_new;

	if(ReadAppSystemsIni(nasmap,naimap)<0)
		goto Error;
	if(ReadTopologyXml(nasmap,naimap)<0)
		goto Error;

	// Add new systems and instances first so they can be 'Parentof' targets
	for(APPSYSMAP::iterator ait=nasmap.begin();ait!=nasmap.end();ait++)
	{
		AppSystem *nasys=ait->second;
		nasys->AcctName=nasys->sname;
		nasys->AcctType="AppSystem";
		nacmap[nasys->AcctName]=nasys;
		for(APPINSTMAP::iterator iit=nasys->imap.begin();iit!=nasys->imap.end();iit++)
		{
			AppInstance *nainst=iit->second;
			if(strrcmp(nainst->iname.c_str(),"*"))
				continue;
			char aicname[256]={0};
			sprintf(aicname,"%s/%s",nasys->sname.c_str(),iit->second->iname.c_str());
			nainst->AcctName=aicname;
			nainst->AcctType="AppInstance";
			nainst->AcctAddParent(nasys);
			nacmap[nainst->AcctName]=nainst;
		}
	}
	WSLogEvent("Pre-loaded %d system and instance accounts.",(int)nacmap.size());
	if(ReadAccountsIni(nacmap)<0)
		goto Error;

	int scnt=(int)nasmap.size(),sload=0;
	int icnt=(int)naimap.size(),iload=0;
	int acnt=(int)nacmap.size(),aload=0;
	// Default system for unknown instances (if not in configuration)
	if(VSDIST_HUB)
		CreateSystem(nasmap,"UNKNOWN",scnt);

	// Can only add new systems and instances. Remove not allowed!
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator nait=nasmap.begin();nait!=nasmap.end();nait++)
	{
		AppSystem *nasys=nait->second;
		// Initialize new app instances only
		APPSYSMAP::iterator ait=asmap.find(nasys->skey);
		if(new_startup || ait==asmap.end() || importHistoricFiles)
		{
			// TODO: What if this takes too long?
			char dbdir[MAX_PATH]={0};
			#ifdef WIN32
			sprintf(dbdir,"%s\\data",pcfg->RunPath.c_str());
			#else
			sprintf(dbdir,"%s/data",pcfg->RunPath.c_str());
			#endif
			char dbname[MAX_PATH]={0};
		#ifdef MULTI_DAY_HIST
			if(odb.HISTORIC_DAYS>0)
			{
				nasys->odb.CopySettings(this->odb, nasys->sname.c_str());
				int ndays = odb.HISTORIC_DAYS;
				if (importHistoricFiles)
				{
					int sdate = CalcTPlusDate(odb.wsdate, 0);
					sprintf(dbname, "%s_%08d.dat", nasys->sname.c_str(), sdate);
					OrderDB *pdb = new OrderDB();
					pdb->Init(this);
					pdb->CopySettings(this->odb, nasys->sname.c_str());
					nasys->odblist.push_back(pdb);
					if (HISTORIC_ONDEMAND)
					{
						pdb->DelayInit(this, dbdir, dbname, sdate, odb.readonly, dbLoadCxl, nasys);
					}
					else if (pdb->Init(this, dbdir, dbname, sdate, odb.readonly, dbLoadCxl, nasys) < 0)
					{
						nasys->odblist.pop_back();
						delete pdb; pdb = 0;
					}
				}
				else
				{
					for (int d = 1; (ndays > 0) && (d <= odb.HISTORIC_DAYS); d++)
					{
						int sdate = CalcTPlusDate(odb.wsdate, -d);
						sprintf(dbname, "%s_%08d.dat", nasys->sname.c_str(), sdate);
						OrderDB *pdb = new OrderDB();
						pdb->Init(this);
						pdb->CopySettings(this->odb, nasys->sname.c_str());
						nasys->odblist.push_back(pdb);
						if (HISTORIC_ONDEMAND)
						{
							pdb->DelayInit(this, dbdir, dbname, sdate, odb.readonly, dbLoadCxl, nasys);
							ndays--;
						}
						else if (pdb->Init(this, dbdir, dbname, sdate, odb.readonly, dbLoadCxl, nasys) < 0)
						{
							nasys->odblist.pop_back();
							delete pdb; pdb = 0;
						}
						else
						{
							ndays--;
							//// Wet the line
							//VSOrder *porder=pdb->AllocOrder();
							//pdb->FreeOrder(porder); porder=0;
						}
					}
				}
			}
			else
			{
				sprintf(dbname,"%s_%08d.dat",nasys->sname.c_str(),odb.wsdate);		
				nasys->odb.CopySettings(this->odb,nasys->sname.c_str());
				if(nasys->odb.Init(this,dbdir,dbname,odb.wsdate,odb.readonly,dbLoadCxl,nasys)<0)
				{
					WSLogError("Failed OrderDB::Init(%s,%08d)!",nasys->sname.c_str(),odb.wsdate);
					continue;
				}
				// Wet the line
				VSOrder *porder=nasys->odb.AllocOrder();
				nasys->odb.FreeOrder(porder); porder=0;
			}
		#else
			sprintf(dbname,"%s_%08d.dat",nasys->sname.c_str(),odb.wsdate);		
			nasys->odb.CopySettings(this->odb,nasys->sname.c_str());
			// Speed load for debugging
			//if(nasys->sname!="CC")
			//	continue;
			if(nasys->odb.Init(this,dbdir,dbname,odb.wsdate,odb.readonly,dbLoadCxl,nasys)<0)
			{
				WSLogError("Failed OrderDB::Init(%s,%08d)!",nasys->sname.c_str(),odb.wsdate);
				continue;
			}
			// Wet the line
			VSOrder *porder=nasys->odb.AllocOrder();
			nasys->odb.FreeOrder(porder); porder=0;
		#endif
			WSLogEvent("Initialized AppSystem(%s)",nasys->sname.c_str()); 
			sload++; iload+=(int)nasys->imap.size();
		}
	}

	// Merge old systems and instances into the new list before we swap
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end()&&!new_startup&&!importHistoricFiles;ait++)
	{
		AppSystem *asys=ait->second;
		APPSYSMAP::iterator nait=nasmap.find(asys->skey);
		// Old system isn't in new config--copy
		if(nait==nasmap.end())
		{
			nasmap[asys->skey]=asys;
			nacmap[asys->sname]=asys;
			for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
			{
				AppInstance *ainst=iit->second;
				naimap[iit->first]=ainst;
				char aicname[256]={0};
				sprintf(aicname,"%s/%s",asys->sname.c_str(),ainst->iname.c_str());
				nacmap[aicname]=ainst;
			}
			WSLogError("Warning: AppSystem(%s) is missing from the new configuration, but will not be deleted!",
				asys->sname.c_str());
		}
		// Old system is in new config--merge
		else
		{
			AppSystem *nasys=nait->second;
			// Add new instances to old system
			int idiff=(int)nasys->imap.size() -(int)asys->imap.size();
			if(idiff>0)
				iload+=idiff;
			for(APPINSTMAP::iterator niit=nasys->imap.begin();niit!=nasys->imap.end();)
			{
				AppInstance *nainst=niit->second;
				if(asys->imap.find(nainst->iname)==asys->imap.end())
				{
					asys->imap[nainst->iname]=nainst;
					#ifdef WIN32
					niit=nasys->imap.erase(niit); // prevent deletion with 'nasys'
					#else
					nasys->imap.erase(niit++);
					#endif

				}
				else
					niit++;
			}
			// Point to the old system, and delete the new
			nait->second=asys;
			// New systems and instances may be referenced in the new accounts map
			ACCTMAP::iterator nacit=nacmap.find(nasys->sname);
			if(nacit!=nacmap.end())
			{
				nacit->second=asys;
				for(APPINSTMAP::iterator niit=nasys->imap.begin();niit!=nasys->imap.end();niit++)
				{
					AppInstance *nainst=niit->second;
					char aicname[256]={0};
					sprintf(aicname,"%s/%s",nasys->sname.c_str(),nainst->iname.c_str());
					nacit=nacmap.find(aicname);
					if(nacit!=nacmap.end())
					{
						ACCTMAP::iterator acit=acmap.find(aicname);
						if(acit!=acmap.end())
							nacit->second=acit->second;
					}
				}
			}
			delete nasys;
		}
	}

	// Pre-generate system browse list
	if(asbrowse) delete asbrowse;
	int nsys=(int)nasmap.size();
	asbrowse=new const char *[nsys];
	int i=0;
	for(APPSYSMAP::iterator ait=nasmap.begin();ait!=nasmap.end();ait++)
	{
		AppSystem *nasys=ait->second;
		asbrowse[i++]=nasys->sname.c_str();
		nasys->PregenBrowse();
	}

	// Can only add accounts. Remove not allowed!
	for(ACCTMAP::iterator nait=nacmap.begin();nait!=nacmap.end();nait++)
	{
		Account *nacc=nait->second;
		// Initialize new accounts only
		ACCTMAP::iterator ait=acmap.find(nacc->AcctName);
		if(new_startup || ait==acmap.end() || importHistoricFiles)
		{
			if((nacc->AcctType!="AppSystem")&&(nacc->AcctType!="AppInstance"))
				WSLogEvent("Loaded account(%s,%s).",nacc->AcctName.c_str(),nacc->AcctType.c_str());
			aload++;
		}
	}
	// Merge old accounts into the new list before we swap
	for(ACCTMAP::iterator ait=acmap.begin();ait!=acmap.end()&&!new_startup;ait++)
	{
		Account *pacc=ait->second;
		ACCTMAP::iterator nait=nacmap.find(pacc->AcctName);
		// Old account isn't in new config--copy
		if(nait==nacmap.end())
		{
			nacmap[pacc->AcctName]=pacc;
			WSLogError("Warning: Account(%s,%s) is missing from the new configuration, but will not be deleted!",
				pacc->AcctName.c_str(),pacc->AcctType.c_str());
		}
		// Old account is in new config--merge
		else
		{
			Account *nacc=nait->second;
			// Point to the old account, and delete the new
			nait->second=pacc;
			if((nacc->AcctType!="AppSystem")&&(nacc->AcctType!="AppInstance"))
				delete nacc;
		}
	}

	//// Pre-generate account browse list
	//int nacc=(int)acmap.size();
	//if(acbrowse) delete acbrowse;
	//acbrowse=new const char *[nacc];
	//memset(acbrowse,0,nacc*sizeof(const char*));
	//i=0;
	//for(ACCTMAP::iterator ait=nacmap.begin();ait!=nacmap.end();ait++)
	//{
	//	Account *acc=ait->second;
	//	acbrowse[i++]=acc->AcctName.c_str();
	//}

	// Swap the config
    if(!new_startup && !importHistoricFiles)
	{
        asmap.swap(nasmap);
        nasmap.clear();
        aimap.swap(naimap);
        naimap.clear();
        acmap.swap(nacmap);
        nacmap.clear();
    }

	ReleaseMutex(asmux);
	WSLogEvent("Loaded %d new systems, %d new instances, %d new accounts.",sload,iload,aload);
	return 0;

Error:
	WSLogError(ErrText);
	for(APPSYSMAP::iterator ait=nasmap.begin();ait!=nasmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		delete asys;
	}
	nasmap.clear();
	naimap.clear();
	for(ACCTMAP::iterator acit=nacmap.begin();acit!=nacmap.end();acit++)
	{
		Account *pacc=acit->second;
		if((pacc->AcctType!="AppSystem")&&(pacc->AcctType!="AppInstance"))
			delete pacc;
	}
	nacmap.clear();
	return -1;
}
int ViewServer::ReadAppSystemsIni(APPSYSMAP& nasmap, APPINSTMAP& naimap)
{
	char ErrText[1024]={0};
	char fpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(fpath,"%s\\setup\\appsystems.ini",pcfg->RunPath.c_str());
	#else
	sprintf(fpath,"%s/setup/appsystems.ini",pcfg->RunPath.c_str());
	#endif
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			if((fp=fopen(fpath,"wt"))==NULL)
			{
				sprintf(ErrText,"IQ-b3-e0: Could not open %s",fpath);
				goto Error;
			}
			fprintf(fp,"// View Server AppInstance Configuration File\n"
				"// Format:\n"
				"// [AppClass]\n"
				"// AppInst1\n"
				"// AppInst2\n"
				"// ...\n"
				"// AppInstN\n");
			fclose(fp);
			return 0;
		}
		sprintf(ErrText,"IQ-b3-e0: Could not open %s",fpath);
		goto Error;
	}
	{//scope
	int lno=0;
	char rbuf[1024]={0};
	AppSystem *nasys=0;
	int scnt=0,icnt=0;
    char currSystemName[128];
	while(fgets(rbuf,1023,fp)!=NULL)
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n'))
			continue;
        else if(strncmp(rptr,"//",2)==0)
            continue;
		char *rend;
		for(rend=rptr;(*rend)&&(*rend!='\r')&&(*rend!='\n');rend++)
			;
		if((*rend!='\r')||(*rend!='\n'))
			*rend=0;

		if(*rptr=='[')
		{
			rptr++;
			// Remove closing bracket
			char *nend=(char*)strechr(rptr,']',rend);
			if(!nend)
			{
				sprintf(ErrText,"Missing ] at line %d!",lno);
				goto Error;
			}
			// Remove trailing spaces
			*nend=0; rend=nend;
			for(;(rend>rptr)&&(isspace(rend[-1]));rend--)
				rend[-1]=0;
			_strupr(rptr);
			// New system
			nasys=CreateSystem(nasmap,rptr,scnt);
            strcpy(currSystemName, rptr);
		}
		else if(nasys)
		{
			// Remove trailing comments
			char *cptr=(char*)strstr(rptr,"//");
			if(cptr) 
			{
				*cptr=0; rend=cptr;
			}
			// Remove trailing spaces
			for(;(rend>rptr)&&(isspace(rend[-1]));rend--)
				rend[-1]=0;
			// All instance names uppercase
			_strupr(rptr);
			// New instance
			AppInstance *nainst=0;
            if(strcmp(currSystemName, "CLSERVER") == 0)
                nainst = CreateDnsDomain(-1, rptr, nasys);
            else
                nainst = nasys->CreateInstance(rptr,icnt);

            if(nainst)
                naimap[nainst->iname]=nainst;
		}
		else
		{
			sprintf(ErrText,"Parse appsystems.ini failure at line %d!",lno);
			goto Error;
		}
	}
	fclose(fp);
	WSLogEvent("Read %d systems, %d instances from %s",scnt,icnt,fpath);
	}//scope
	return 0;
Error:
	if(fp)
		fclose(fp);
	WSLogError(ErrText);
	for(APPSYSMAP::iterator ait=nasmap.begin();ait!=nasmap.end();ait++)
	{
		AppSystem *nasys=ait->second;
		delete nasys;
	}
	nasmap.clear();
	naimap.clear();
	return -1;
}
                                                                                          
void AddDomainNamesToAppSystemsIni(FILE* fp_temp, APPINSTMAP& aimap)
{
    for(APPINSTMAP::iterator iter = aimap.begin(); iter != aimap.end(); iter++)
    {
        if(iter->second->asys->sname.compare("CLSERVER") == 0)
        {
            char buff[128] = {0};
            strncpy(buff, iter->first.c_str(), 127);
            int size = strlen(buff);
            bool save = true;
            for(int i=0; i<size; i++)
            {
                int iv = buff[i];
                save = ((iv>=48 && iv<=57)  ||  // Numerical
                        (iv>=65 && iv<=90)  ||  // Upper case
                        (iv>=97 && iv<=122) ||  // Lower case
                        (iv==45)            ||  // -
                        (iv==46)            ||  // .
                        (iv==95));              // _
                if(save == false)
                    break;
            }
            if(save)
                fprintf(fp_temp, "%s\n", buff);
        }
    }
}

int ViewServer::SaveAppSystemsIni()
{
	char ErrText[1024]={0};
    char fpath[MAX_PATH]={0}, fpath_temp[MAX_PATH]={0}, fpath_save[MAX_PATH]={0};
#ifdef WIN32
    sprintf(fpath,"%s\\setup\\appsystems.ini",pcfg->RunPath.c_str());
    sprintf(fpath_temp,"%s\\setup\\appsystems_temp.ini",pcfg->RunPath.c_str());
    sprintf(fpath_save,"%s\\setup\\appsystems_save.ini",pcfg->RunPath.c_str());
#else
	sprintf(fpath,"%s/setup/appsystems.ini",pcfg->RunPath.c_str());
	sprintf(fpath_temp,"%s/setup/appsystems_temp.ini",pcfg->RunPath.c_str());
	sprintf(fpath_save,"%s/setup/appsystems_save.ini",pcfg->RunPath.c_str());
#endif
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
	    WSLogError("SaveAppSystemsIni can not open %s for updating",fpath);
	    return -1;
    }

	FILE *fp_temp=fopen(fpath_temp,"wt");
	if(!fp_temp)
	{
        fclose(fp);
	    WSLogError("SaveAppSystemsIni can not open temporary AppSystemIni file %s",fpath);
	    return -1;
	}

    int lno=0;
    char rbuf[1024]={0}, rbuf_orig[1024]={0};
    int scnt=0,icnt=0;
    char currSystemName[128] = {0};
    while(fgets(rbuf,1023,fp)!=NULL)
    {
        strcpy(rbuf_orig, rbuf);
        char *rptr;
        for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
            ;
        if((!*rptr)||(*rptr=='\r')||(*rptr=='\n'))
            continue;
        char *rend;
        for(rend=rptr;(*rend)&&(*rend!='\r')&&(*rend!='\n');rend++)
            ;
        if((*rend!='\r')||(*rend!='\n'))
            *rend=0;

        if(*rptr=='[')
        {
            rptr++;
            // Remove closing bracket
            char *nend=(char*)strechr(rptr,']',rend);
            if(!nend)
            {
                WSLogError("AppSystems.ini is Missing ] at line %d!",lno);
                fclose(fp);
                fclose(fp_temp);
                return -1;
            }

            // Remove trailing spaces
            *nend=0; rend=nend;
            for(;(rend>rptr)&&(isspace(rend[-1]));rend--)
                rend[-1]=0;
            _strupr(rptr);

            if(strcmp(currSystemName, "CLSERVER") == 0)
            {
                // Save DNS domain names if the marking line is not found for CLSERVER
                AddDomainNamesToAppSystemsIni(fp_temp, aimap);
            }
            strcpy(currSystemName, rptr);
            fprintf(fp_temp, "\n");
        }
        else
        {
            if(strcmp(currSystemName, "CLSERVER") == 0)
                continue;
        }
        fprintf(fp_temp, "%s", rbuf_orig);
    }
    if(strcmp(currSystemName, "CLSERVER") == 0)  
    {
        // Save DNS domain names if CLSERVER is the last AppSystem in the file
        AddDomainNamesToAppSystemsIni(fp_temp, aimap);
    }

    fclose(fp);
    fclose(fp_temp);

#ifdef WIN32
    DeleteFile(fpath_save);
    MoveFile(fpath, fpath_save);
    MoveFile(fpath_temp, fpath);
#else
    unlink(fpath_save);
    rename(fpath, fpath_save);
    rename(fpath_temp, fpath);
#endif

	return 0;
}

class TopologySession
{
public:
	TopologySession()
		:localAppName()
		,localAppType()
		,localSessionName()
		,remoteAppName()
		,remoteAppType()
		,remoteSessionName()
		,facingAspect()
	{
	}

	string localAppName;
	string localAppType;
	string localSessionName;
	string remoteAppName;
	string remoteAppType;
	string remoteSessionName;
	string facingAspect;
};
int ViewServer::ReadTopologyXml(APPSYSMAP& nasmap, APPINSTMAP& naimap)
{
	char ErrText[1024]={0};
	char fpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(fpath,"%s\\setup\\topology.xml",pcfg->RunPath.c_str());
	#else
	sprintf(fpath,"%s/setup/topology.xml",pcfg->RunPath.c_str());
	#endif
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			if((fp=fopen(fpath,"wt"))==NULL)
			{
				sprintf(ErrText,"Could not open %s",fpath);
				goto Error;
			}
			fclose(fp);
			return 0;
		}
		sprintf(ErrText,"Could not open %s",fpath);
		goto Error;
	}
	{//scope
	int lno=0;
	char rbuf[1024]={0};
	AppSystem *nasys=0;
	int scnt=0,icnt=0;
	TopologySession fit;
	while(fgets(rbuf,1023,fp)!=NULL)
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n'))
			continue;
        else if(strncmp(rptr,"//",2)==0)
            continue;
		char *rend;
		for(rend=(char*)rptr;(*rend)&&(*rend!='\r')&&(*rend!='\n');rend++)
			;
		if((*rend!='\r')||(*rend!='\n'))
			*rend=0;

		// This code corresponded to the "topology_PROD.xml" dated Aug 11
		// Ex: <Session localAppName="XFM_APR1" localAppType="XFM" localSessionName="PUBHUB" remoteAppName="PUBHUB_GUI" remoteAppType="PUBHUB" remoteSessionName="XFM_APR1" facingAspect="CHILD"/>
		//// Instead of using full XML parser, just assume each session on a separate line
		//if(!strncmp(rptr,"<Session ",9))
		//{
		//	TopologySession ts;
		//	// Scan session attributes
		//	for(rptr+=9;rptr<rend;)
		//	{
		//		// Skip spaces
		//		for(;(rptr<rend)&&(isspace(*rptr));rptr++)
		//			;
		//		if(rptr>=rend)
		//			break;
		//		// Attribute
		//		const char *attr=rptr;
		//		for(;(rptr<rend)&&(*rptr!='=');rptr++)
		//			;
		//		if(rptr>=rend)
		//			break;
		//		// Equals quoted value
		//		_ASSERT(*rptr=='=');
		//		*rptr=0; 
		//		for(rptr++;(rptr<rend)&&(*rptr!='\"');rptr++)
		//			;
		//		if(rptr>=rend)
		//			break;
		//		// Value
		//		char *val=rptr+1;
		//		for(rptr++;(rptr<rend)&&(*rptr!='\"');rptr++)
		//			;
		//		*rptr=0;
		//		// Close quote and space
		//		for(rptr++;(rptr<rend)&&(isspace(*rptr));rptr++)
		//			;
		//		if(!_stricmp(attr,"localAppName"))
		//		{
		//			_strupr(val);
		//			ts.localAppName=val;
		//		}
		//		else if(!_stricmp(attr,"localAppType"))
		//		{
		//			_strupr(val);
		//			ts.localAppType=val;
		//		}
		//		else if(!_stricmp(attr,"localSessionName"))
		//			ts.localSessionName=val;
		//		else if(!_stricmp(attr,"remoteAppName"))
		//			ts.remoteAppName=val;
		//		else if(!_stricmp(attr,"remoteAppType"))
		//			ts.remoteAppType=val;
		//		else if(!_stricmp(attr,"remoteSessionName"))
		//			ts.remoteSessionName=val;
		//		else if(!_stricmp(attr,"facingAspect"))
		//			ts.facingAspect=val;
		//	}
		//	// New system
		//	nasys=CreateSystem(nasmap,ts.localAppType.c_str(),scnt);
		//	if(nasys)
		//	{
		//		// New instance
		//		AppInstance *nainst=nasys->CreateInstance(ts.localAppName.c_str(),icnt);
		//		if(nainst)
		//			naimap[nainst->iname]=nainst;
		//	}
		//}
		// This code corresponded to the "topology_prod_hosts-2010-11-08.xml" dated Nov 8
		// Ex:
        //<file iontype="com.ml.ett.goms.elvis.config.InstanceConfigItem" name="cca-ax1-g.prd.etsd.ml.com.CC_CAD1" type="Instance">
        //    <field name="name">CC_CAD1</field>
        //    <field name="type">CCA</field>
        //    <field name="path">/cca-ax001g/apps/ccaprod/CC_CAD1</field>
        //    <field name="host">cca-ax1-g.prd.etsd.ml.com</field>
        //    <field name="adminPort">12005</field>
        //    <field name="version">4.0.20</field>
        //</file>
		char *tptr=strstr(rptr,"<field name=\"name\">");
		if(tptr)
		{
			rptr=tptr +19;
			char *val=rptr;
			for(rptr++;(rptr<rend)&&(*rptr!='<');rptr++)
				;
			*rptr=0;
			_strupr(val);
			fit.localAppName=val;
			fit.localAppType.clear();
			fit.localSessionName.clear();
			fit.remoteAppName.clear();
			fit.remoteAppType.clear();
			fit.remoteSessionName.clear();
			fit.facingAspect.clear();
		}
		tptr=strstr(rptr,"<field name=\"type\">");
		if(tptr)
		{
			rptr=tptr +19;
			char *val=rptr;
			for(rptr++;(rptr<rend)&&(*rptr!='<');rptr++)
				;
			*rptr=0;
			_strupr(val);
			fit.localAppType=val;

			// New system
			nasys=CreateSystem(nasmap,fit.localAppType.c_str(),scnt);
			if(nasys)
			{
				// New instance
				AppInstance *nainst=nasys->CreateInstance(fit.localAppName.c_str(),icnt);
				if(nainst)
					naimap[nainst->iname]=nainst;
			}
		}
	}
	WSLogEvent("Read %d systems, %d instances from %s",scnt,icnt,fpath);
	}//scope
	return 0;
Error:
	if(fp)
		fclose(fp);
	WSLogError(ErrText);
	for(APPSYSMAP::iterator ait=nasmap.begin();ait!=nasmap.end();ait++)
	{
		AppSystem *nasys=ait->second;
		delete nasys;
	}
	nasmap.clear();
	naimap.clear();
	return -1;
}
AppSystem *ViewServer::GetAppSystem(const char *AppInstID)
{
	WaitForSingleObject(asmux,INFINITE);
	if(strrcmp(AppInstID,"*"))
	{
		APPSYSMAP::iterator ait=asmap.find(AppInstID);
		if(ait!=asmap.end())
		{
			AppSystem *asys=ait->second;
			ReleaseMutex(asmux);
			return asys;
		}
	}
	else
	{
		APPINSTMAP::iterator iit=aimap.find(AppInstID);
		if(iit!=aimap.end())
		{
			AppInstance *ainst=iit->second;
			ReleaseMutex(asmux);
			return ainst->asys;
		}
	}
	ReleaseMutex(asmux);
	return 0;
}
AppInstance *ViewServer::GetAppInstance(const char *AppInstID)
{
	WaitForSingleObject(asmux,INFINITE);
	APPINSTMAP::iterator iit=aimap.find(AppInstID);
	if(iit!=aimap.end())
	{
		AppInstance *ainst=iit->second;
		ReleaseMutex(asmux);
		return ainst;
	}
	ReleaseMutex(asmux);
	return 0;
}
Account *ViewServer::CreateAccount(ACCTMAP& nacmap, const char *aname, int& acnt)
{
	ACCTMAP::iterator acit=nacmap.find(aname);
	if(acit!=nacmap.end())
		return acit->second;
	Account *nacc=new Account;
	nacc->AcctName=aname;
	nacmap[aname]=nacc; acnt++;
	return nacc;
}
int ViewServer::ReadAccountsIni(ACCTMAP& nacmap)
{
	char ErrText[1024]={0};
	char fpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(fpath,"%s\\setup\\accounts.ini",pcfg->RunPath.c_str());
	#else
	sprintf(fpath,"%s/setup/accounts.ini",pcfg->RunPath.c_str());
	#endif
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			if((fp=fopen(fpath,"wt"))==NULL)
			{
				sprintf(ErrText,"IQ-b3-e0: Could not open %s",fpath);
				goto Error;
			}
			fprintf(fp,"// View Server Accounts Configuration File\n"
				"// Format:\n"
				"// [Account]\n"
				"// Match:<where_clause>\n"
				"// Parentof:<child_account>\n"
				"// ...\n");
			fclose(fp);
			return 0;
		}
		sprintf(ErrText,"IQ-b3-e0: Could not open %s",fpath);
		goto Error;
	}
	int lno=0;
	char rbuf[1024]={0};
	Account *acc=0;
	int acnt=0;
	while(fgets(rbuf,1023,fp)!=NULL)
	{
		lno++;
		const char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n'))
			continue;
        else if(strncmp(rptr,"//",2)==0)
            continue;
		char *rend;
		for(rend=(char*)rptr;(*rend)&&(*rend!='\r')&&(*rend!='\n');rend++)
			;
		if((*rend!='\r')||(*rend!='\n'))
			*rend=0;

		if(*rptr=='[')
		{
			rptr++;
			char *nend=(char*)strechr(rptr,']',rend);
			if(!nend)
			{
				sprintf(ErrText,"Missing ] at line %d!",lno);
				goto Error;
			}
			// remove trailing spaces
			*nend=0; rend=nend;
			for(;(rend>rptr)&&(isspace(rend[-1]));rend--)
				rend[-1]=0;
			// New account
			acc=CreateAccount(nacmap,rptr,acnt);
		}
		else if(acc)
		{
			// remove trailing comments
			char *cptr=(char*)strstr(rptr,"//");
			if(cptr) 
			{
				*cptr=0; rend=cptr;
			}
			// remove trailing spaces
			for(;(rend>rptr)&&(isspace(rend[-1]));rend--)
				rend[-1]=0;
			const char *attr=rptr;
			if(!*attr)
			{
				sprintf(ErrText,"Missing ':' at line %d!",lno);
				goto Error;
			}
			char *val=(char*)strchr(attr,':');
			if(!val)
			{
				sprintf(ErrText,"Empty value for (%s) at line %d!",attr,lno);
				goto Error;
			}
			*val=0; val++;
			if(!_stricmp(attr,"Type"))
			{
				acc->AcctType=val;
			}
			else if(!_stricmp(attr,"Match"))
			{
				acc->mlist.push_back(val);
			}
			else if(!_stricmp(attr,"Parentof"))
			{
				ACCTMAP::iterator ait=nacmap.find(val);
				if(ait==nacmap.end())
				{
					sprintf(ErrText,"Unknown Parentof(%s) at line %d!",val,lno);
					goto Error;
				}
				Account *child=ait->second;
				child->AcctAddParent(acc);
			}
			else
			{
				sprintf(ErrText,"Unknown attribute(%s) at line %d!",attr,lno);
				goto Error;
			}
		}
		else
		{
			sprintf(ErrText,"Parse accounts.ini failure at line %d!",lno);
			goto Error;
		}
	}
	fclose(fp);
	WSLogEvent("Read %d accounts from %s",acnt,fpath);
	return 0;

Error:
	if(fp)
		fclose(fp);
	WSLogError(ErrText);
	for(ACCTMAP::iterator ait=nacmap.begin();ait!=nacmap.end();ait++)
	{
		Account *acc=ait->second;
		delete acc;
	}
	nacmap.clear();
	return -1;
}
Account *ViewServer::GetAccount(const char *aname)
{
	WaitForSingleObject(asmux,INFINITE);
	ACCTMAP::iterator ait=acmap.find(aname);
	if(ait!=acmap.end())
	{
		Account *acc=ait->second;
		ReleaseMutex(asmux);
		return acc;
	}
	ReleaseMutex(asmux);
	return 0;
}

void ViewServer::WSDateChange()
{
	conStats.ResetAll();
	uscStats.ResetAll();
	onceErrSet.clear();
	UnkClOrdIDSeqNo=0;
	if((!WSDATE)&&(odb.wsdate)&&(odb.wsdate!=WSDate))
	{
		if(ffp)
		{
			fclose(ffp); ffp=0;
		}

		char fpath[MAX_PATH]={0};
		WriteHistogram(fpath);
		memset(ohist,0,sizeof(ohist));
		// Locking is required because ODBIndexOrder will be called
		_ASSERT(false); // we really don't want the server to run overnight
		//Lock();
		//odb.omap.clear();
		//odb.Shutdown();
		//char dbname[MAX_PATH]={0};
		//sprintf(dbname,"VSDB_%08d.dat",WSDate);
		//bool dbLoadCxl=false;
		//if(odb.Init(this,pcfg->RunPath.c_str(),dbname,WSDate,false,dbLoadCxl)<0)
		//	WSLogError("Failed OrderDB::Init for new day(%08d)!",WSDate);
		//Unlock();
		//WSLogEvent("Loaded %d database orders",odb.omap.size());
		for(int i=0;i<NO_OF_USR_PORTS;i++)
		{
			if(!UsrPort[i].SockActive)
				continue;
			if(UsrPort[i].DetPtr==(void*)PROTO_CLBACKUP)
				CLBackupDateChange(i);
		}
	}
	if(!WSDATE)
		PurgeOldDays();
}
void ViewServer::WSTimeChange()
{
	if(VSDIST_HUB)
	{
		odb.UpdateExtFile();
	}
#ifdef SPECTRUM
	else
	{
		odb.UpdateZmapFile(zmapReset);
		if(zmapReset)
		{
			CloseDetailFiles();
			OpenDetailFiles();
			zmapReset=false;
		}
	}
#endif

#ifdef REDIOLEDBCON
	RediTimeChange();
#endif

	DWORD tnow=GetTickCount();
	if(!lastTimeChange) lastTimeChange=tnow;
	DWORD tdiff=tnow -lastTimeChange;
	if(tdiff==0) tdiff=1000;
	lastTimeChange=tnow;

	char hbuf[128];
	memset(hbuf,0,128);
	// CON Heartbeats and Statistics
	FixStats lastConStats=conStats;
	conStats.ResetAll();
	conStats.totMsgCnt=lastConStats.totMsgCnt;
	conStats.maxMsgCnt=lastConStats.maxMsgCnt;
	conStats.cntMsgCnt=lastConStats.cntMsgCnt;
	conStats.lastSendTime=lastConStats.lastSendTime;
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
		{
			// Nag about disconnected sessions
			if((!strncmp(ConPort[i].CfgNote,"$FIX$",5))&&(ConPort[i].InUse)&&
			   (FIX_HOURS_BEGIN>0)&&(WSTime>=FIX_HOURS_BEGIN)&&(FIX_HOURS_END>0)&&(WSTime<FIX_HOURS_END)&&(WSTime%5==0))
				WSLogError("CON%d: FIX client unconnected to %s:%d for (%s) between [%06d,%06d)!",
					i,ConPort[i].RemoteIP,ConPort[i].Port,ConPort[i].CfgNote,FIX_HOURS_BEGIN,FIX_HOURS_END);
			continue;
		}
		WSLockPort(WS_CON,i,true);
		if(!ConPort[i].SockConnected)
			continue;
		if((ConPort[i].DetPtr==(void*)PROTO_MLFIXLOGS)||
		   (ConPort[i].DetPtr==(void*)PROTO_MLPUBHUB)||
		   (ConPort[i].DetPtr==(void*)PROTO_ITF)||
		   (ConPort[i].DetPtr==(void*)PROTO_IQOMS)||
		   (ConPort[i].DetPtr==(void*)PROTO_IQFIXS)||
		   (ConPort[i].DetPtr==(void*)PROTO_TJMLOG)||
		   (ConPort[i].DetPtr==(void*)PROTO_FIXDROP)||
		   (ConPort[i].DetPtr==(void*)PROTO_TBACKUP)||
		   (ConPort[i].DetPtr==(void*)PROTO_FULLLOG)||
		   (ConPort[i].DetPtr==(void*)PROTO_TJM)||
		   (ConPort[i].DetPtr==(void*)PROTO_MLPH_EMEA)||
		   (ConPort[i].DetPtr==(void*)PROTO_MLPH_CC)||
		   (ConPort[i].DetPtr==(void*)PROTO_OMS_CBOE))
		{
			FixStats *cstats=(FixStats *)ConPort[i].DetPtr3;
			if(cstats)
			{
				FixStats csnap=*cstats;
				cstats->cntMsgCnt++; cstats->SetAverages(); cstats->msgCnt=0;
				DWORD totincr=csnap.msgCnt;
				if(tdiff>1000)
				{
					csnap.msgCnt=csnap.msgCnt*1000/tdiff;
				}
				if(csnap.msgCnt>csnap.maxMsgCnt) 
					cstats->maxMsgCnt=csnap.maxMsgCnt=csnap.msgCnt;				
				sprintf(ConPort[i].OnLineStatusText,"%s mps, %s max, %s tot",
					SizeStr(csnap.msgCnt,false),SizeStr(csnap.maxMsgCnt,false),SizeStr(csnap.totMsgCnt,false));
				conStats+=csnap; 
				conStats.totMsgCnt+=(totincr -csnap.msgCnt);				
			}
			if(ConPort[i].DetPtr==(void*)PROTO_FIXDROP)
			{
				DropClient *pdc=(DropClient*)ConPort[i].DetPtr5;
				if(pdc)
				{
					// Session heartbeat
					//if(!pdc->passive)
					//{
						if((pdc->IsLoggedIn())&&(tnow -pdc->ohblast>=pdc->ohbinterval))
							pdc->Heartbeat(0);
					//}
					char *cptr=ConPort[i].OnLineStatusText; cptr+=strlen(cptr);
					sprintf(cptr,", in=%d, out=%d, wait=%d",pdc->iseq,pdc->oseq,pdc->WaitSize());
				}
			}
			else if(ConPort[i].DetPtr==(void*)PROTO_TBACKUP)
			{
				ConPort[i].BeatsOut++;
				WSConSendMsg(16,128,hbuf,i);
			}
		}
		else if((ConPort[i].DetPtr==(void*)PROTO_VSCLIENT)||
				(ConPort[i].DetPtr==(void*)PROTO_DNS_REPL)||
				(ConPort[i].DetPtr==(void*)PROTO_DNS_MGR)||
				(ConPort[i].DetPtr==(void*)PROTO_SYMBLIST))
		{
			ConPort[i].BeatsOut++;
			WSConSendMsg(16,128,hbuf,i);

			//FixStats *cstats=(FixStats *)ConPort[i].DetPtr3;
			//if(cstats)
			//{
			//	DWORD totincr=cstats->msgCnt;
			//	if(tdiff>1000)
			//	{
			//		cstats->msgCnt=cstats->msgCnt*1000/tdiff;
			//	}
			//	if(cstats->msgCnt>cstats->maxMsgCnt) 
			//		cstats->maxMsgCnt=cstats->msgCnt;
			//	cstats->cntMsgCnt++; cstats->SetAverages(); 
			//	sprintf(ConPort[i].OnLineStatusText,"%s mps ",SizeStr(cstats->msgCnt,false));
			//	conStats+=*cstats;
			//	conStats.totMsgCnt+=(totincr -cstats->msgCnt);
			//	cstats->msgCnt=0;
			//}
		}
		else if(ConPort[i].DetPtr==(void*)PROTO_VSDIST)
		{
			ConPort[i].BeatsOut++;
			WSConSendMsg(16,128,hbuf,i);

			// Send buffered updates
			int lstart=0,lend=0;
			FixStats *cstats=(FixStats *)ConPort[i].DetPtr3;
			int UscPortNo=(int)(PTRCAST)ConPort[i].DetPtr4;
			if((UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS))
			{
				VSDistCodec *vsd=(VSDistCodec *)ConPort[i].DetPtr6;
				VSDistJournal *dj=(VSDistJournal*)UscPort[UscPortNo].DetPtr5;
				if((cstats)&&(vsd)&&(dj))
				{
					// Throttle out hold at 50MB
					dj->Lock();
					dj->SetBackupMemUsage(ConPort[i].OutBuffer.Size);
					char wbuf[12288],*wptr=wbuf;
					DWORD& rid=(DWORD&)ConPort[i].DetPtr5;
					if(dj->firstPage)
						lstart=dj->firstPage->lstart;
					if(dj->lastPage)
						lend=dj->lastPage->lend;
					if((ConPort[i].OutBuffer.Size<50*1024*1024)&&(!ConPort[i].SendTimeOut))
					{
						for(JPAGE *jpage=dj->firstPage;jpage;jpage=jpage->next)
						{
							// Only retransmit after 3 seconds no ACK
							bool retransmit=false;
							if(jpage->lsent>=jpage->lend)
							{
							#ifdef RETRANS_JOURNAL
								if((RETRANS_TIMEOUT<1)||(tnow -jpage->timeSent<(DWORD)RETRANS_TIMEOUT))
									continue;
								retransmit=true;
								#ifdef _DEBUG
								WSLogEvent("CON%d: Retransmitting(%d) jpage [%d,%d] after %d ms",
									i,jpage->nxmit+1,jpage->lstart,jpage->lend,tnow -jpage->timeSent);
								#endif
							#else
								continue;
							#endif
							}
							if(vsd->EncodeJPageRequest(wptr,sizeof(wbuf),++rid,jpage,retransmit)>0)
							{
							#ifdef DEBUG_JOURNAL
								WSLogEvent("CON%d: DEBUG Jpage(%d,%d) %d bytes",i,jpage->lstart,jpage->lend,(int)(wptr -wbuf));
								if((jpage->lstart!=dj->next)&&(jpage->lsent+1!=dj->next))
									WSLogError("CON%d: DEBUG positive jpage gap [%d,%d)!",i,dj->next,jpage->lstart);
								dj->next=jpage->lend+1;
							#endif
								WSConSendMsg(100,(int)(wptr -wbuf),wbuf,i);
								jpage->timeSent=tnow; 
								jpage->nxmit++;
								jpage->lsent=jpage->lend;
								int mcnt=jpage->lend -jpage->lstart +1;
								cstats->msgCnt+=mcnt; cstats->totMsgCnt+=mcnt;
							}
							else
								_ASSERT(false);
							wptr=wbuf;
						}
					}
					dj->Unlock();

					DWORD totincr=cstats->msgCnt;
					if(tdiff>1000)
					{
						cstats->msgCnt=cstats->msgCnt*1000/tdiff;
					}
					if(cstats->msgCnt>cstats->maxMsgCnt) 
						cstats->maxMsgCnt=cstats->msgCnt;
					cstats->cntMsgCnt++; cstats->SetAverages(); 
					int lperc=lstart>0 ?(int)((PTRCAST)lstart*100/(lend?lend:1)) :100;
					sprintf(ConPort[i].OnLineStatusText,"%s mps [%d,%d] %d%%",
						SizeStr(cstats->msgCnt,false),lstart?lstart:dj->GetCount(),lend?lend:dj->GetCount(),lperc);
					conStats+=*cstats;
					conStats.totMsgCnt+=(totincr -cstats->msgCnt);
					cstats->msgCnt=0;
				}
			}
		}
		WSUnlockPort(WS_CON,i,true);
	}
	if(conStats.msgCnt>conStats.maxMsgCnt)
		conStats.maxMsgCnt=conStats.msgCnt;
	conStats.SetAverages();
	sprintf(ConPort[NO_OF_CON_PORTS].OnLineStatusText,"%s mps, %s max, %s tot",
		SizeStr(conStats.msgCnt,false),SizeStr(conStats.maxMsgCnt,false),SizeStr(conStats.totMsgCnt,false));

	// USR Heartbeats and Statistics
	for(int i=0;i<NO_OF_USR_PORTS;i++)
	{
		if(!UsrPort[i].SockActive)
			continue;
		WSLockPort(WS_USR,i,false);
		if(!UsrPort[i].SockActive)
			continue;
		if((UsrPort[i].DetPtr==(void*)PROTO_MLFIXLOGS)||
		   (UsrPort[i].DetPtr==(void*)PROTO_MLPUBHUB)||
		   (UsrPort[i].DetPtr==(void*)PROTO_ITF)||
		   (UsrPort[i].DetPtr==(void*)PROTO_IQOMS)||
		   (UsrPort[i].DetPtr==(void*)PROTO_IQFIXS)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TJMLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FIXDROP)||
		   (UsrPort[i].DetPtr==(void*)PROTO_CLDROP)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TBACKUP)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TJM)||
		   (UsrPort[i].DetPtr==(void*)PROTO_MLPH_EMEA)||
		   (UsrPort[i].DetPtr==(void*)PROTO_MLPH_CC)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FIXS_PLACELOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FIXS_CXLLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FIXS_RPTLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FIXS_EVELOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_CLS_EVENTLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TRADER_PLACELOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TRADER_CXLLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TRADER_RPTLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TRADER_EVELOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FULLLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_RTOATS)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TWISTLOG)||
		   (UsrPort[i].DetPtr==(void*)PROTO_TWIST)||
		   (UsrPort[i].DetPtr==(void*)PROTO_OMS_CBOE)||
		   (UsrPort[i].DetPtr==(void*)PROTO_FIX)||
		   (UsrPort[i].DetPtr==(void*)PROTO_CLS_BTR)||
		   (UsrPort[i].DetPtr==(void*)PROTO_CSV)||
		   (UsrPort[i].DetPtr==(void*)PROTO_RTECHOATS))
		{
			FixStats *ustats=(FixStats *)UsrPort[i].DetPtr3;
			if(ustats)
			{
				DWORD totincr=ustats->msgCnt;
				if(tdiff>1000)
				{
					ustats->msgCnt=ustats->msgCnt*1000/tdiff;
				}
				if(ustats->msgCnt>ustats->maxMsgCnt) 
					ustats->maxMsgCnt=ustats->msgCnt;
				ustats->cntMsgCnt++; ustats->SetAverages(); 
				sprintf(UsrPort[i].Status,"%s Bps, %s max, %s tot",
					SizeStr(ustats->msgCnt,true),SizeStr(ustats->maxMsgCnt,true),SizeStr(ustats->totMsgCnt,true));
				ustats->msgCnt=0;
			}
			if(UsrPort[i].DetPtr==(void*)PROTO_CLDROP)
			{
				UsrPort[i].BeatsOut++;
				WSUsrSendMsg(16,128,hbuf,i);
			}
		}
		else if(UsrPort[i].DetPtr==(void*)PROTO_CLBACKUP)
		{
			CLBackupTimeChange(i);
			UsrPort[i].BeatsOut++;
			WSUsrSendMsg(16,128,hbuf,i);
		}
		else if(UsrPort[i].DetPtr==(void*)PROTO_VSCLIENT)
		{
			UsrPort[i].BeatsOut++;
			WSUsrSendMsg(16,128,hbuf,i);

			FixStats *ustats=(FixStats *)UsrPort[i].DetPtr3;
			FeedUser *fuser=(FeedUser*)UsrPort[i].DetPtr9;
			if((ustats)&&(fuser))
			{
				DWORD totincr=ustats->msgCnt;
				if(tdiff>1000)
				{
					ustats->msgCnt=ustats->msgCnt*1000/tdiff;
				}
				if(ustats->msgCnt>ustats->maxMsgCnt) 
					ustats->maxMsgCnt=ustats->msgCnt;
				ustats->cntMsgCnt++; ustats->SetAverages(); 
				sprintf(UsrPort[i].Status,"%s mps, %s %08d-%006d",
					SizeStr(ustats->msgCnt,false),fuser->user.c_str(),fuser->loginDate,fuser->loginTime);
				ustats->msgCnt=0;
			}
		}
		else if(UsrPort[i].DetPtr==(void*)PROTO_VSDIST)
		{
			UsrPort[i].BeatsOut++;
			WSUsrSendMsg(16,128,hbuf,i);

			FixStats *ustats=(FixStats *)UsrPort[i].DetPtr3;
			if(ustats)
			{
				DWORD totincr=ustats->msgCnt;
				if(tdiff>1000)
				{
					ustats->msgCnt=ustats->msgCnt*1000/tdiff;
				}
				if(ustats->msgCnt>ustats->maxMsgCnt) 
					ustats->maxMsgCnt=ustats->msgCnt;
				ustats->cntMsgCnt++; ustats->SetAverages(); 
				int& lstart=(int&)UsrPort[i].DetPtr4;
				int& lend=(int&)UsrPort[i].DetPtr5;
				int lperc=(int)(lstart>0 ?(PTRCAST)lstart*100/(lend?lend:1) :100);
				sprintf(UsrPort[i].Status,"%s mps [%d,%d] %d%%",SizeStr(ustats->msgCnt,false),lstart,lend,lperc);
				lstart=lend=0;
				ustats->msgCnt=0;
			}
		}
	#ifdef FIX_SERVER
		if(UsrPort[i].DetPtr==(void*)PROTO_FIX)
		{
			DropServer *pds=(DropServer*)UsrPort[i].DetPtr5;
			if(pds)
			{
				// Session heartbeat
				//if(!pds->passive)
				//{
					if((pds->IsLoggedIn())&&(tnow -pds->ohblast>=pds->ohbinterval))
						pds->Heartbeat(0);
				//}
				char *cptr=UsrPort[i].Status; cptr+=strlen(cptr);
				sprintf(cptr,", in=%d, out=%d, wait=%d",pds->iseq,pds->oseq,pds->WaitSize());
			}
		}
	#endif
		WSUnlockPort(WS_USR,i,false);
	}

	// UMR heartbeats
	for(int i=0;i<NO_OF_UMR_PORTS;i++)
	{
		if(!UmrPort[i].SockActive)
			continue;
		UmrPort[i].BeatsOut++;
		WSUmrSendMsg(16,128,hbuf,i);
	}

	// UGR heartbeats
	for(int i=0;i<NO_OF_UGR_PORTS;i++)
	{
		if(!UgrPort[i].SockActive)
			continue;
		UgrPort[i].BeatsOut++;
		WSUgrSendNGMsg(16,128,hbuf,i);
	}

	// Title caption stats (USC totals)
	FixStats lastUscStats=uscStats;
	uscStats.ResetAll();
	uscStats.totMsgCnt=lastUscStats.totMsgCnt;
	uscStats.maxMsgCnt=lastUscStats.maxMsgCnt;
	uscStats.cntMsgCnt=lastUscStats.cntMsgCnt;
	uscStats.lastSendTime=lastUscStats.lastSendTime;
	LONGLONG rtot=0,wtot=0,jsize=0,jmax=0;
	for(int i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(!UscPort[i].InUse)
			continue;
		WSLockPort(WS_USC,i,false);
		if(!UscPort[i].InUse)
			continue;
		if((UscPort[i].DetPtr==(void*)PROTO_MLFIXLOGS)||
		   (UscPort[i].DetPtr==(void*)PROTO_MLPUBHUB)||
		   (UscPort[i].DetPtr==(void*)PROTO_VSDIST)||
		   (UscPort[i].DetPtr==(void*)PROTO_ITF)||
		   (UscPort[i].DetPtr==(void*)PROTO_IQOMS)||
		   (UscPort[i].DetPtr==(void*)PROTO_IQFIXS)||
		   (UscPort[i].DetPtr==(void*)PROTO_TJMLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXDROP)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLDROP)||
		   (UscPort[i].DetPtr==(void*)PROTO_TBACKUP)||
		   (UscPort[i].DetPtr==(void*)PROTO_TJM)||
		   (UscPort[i].DetPtr==(void*)PROTO_MLPH_EMEA)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLBACKUP)||
		   (UscPort[i].DetPtr==(void*)PROTO_MLPH_CC)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_PLACELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_CXLLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_RPTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_EVELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLS_EVENTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_PLACELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_CXLLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_RPTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_EVELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FULLLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_RTOATS)||
		   (UscPort[i].DetPtr==(void*)PROTO_TWISTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TWIST)||
		   (UscPort[i].DetPtr==(void*)PROTO_OMS_CBOE)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIX)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLS_BTR)||
		   (UscPort[i].DetPtr==(void*)PROTO_CSV)||
		   (UscPort[i].DetPtr==(void*)PROTO_RTECHOATS))
		{
			FixStats *ustats=(FixStats *)UscPort[i].DetPtr3;
			if(ustats)
			{
				FixStats usnap=*ustats; 
				ustats->cntMsgCnt++; ustats->SetAverages(); ustats->msgCnt=0;
				DWORD totincr=usnap.msgCnt;
				if(tdiff>1000)
				{
					usnap.msgCnt=usnap.msgCnt*1000/tdiff;
				}
				if(usnap.msgCnt>usnap.maxMsgCnt) 
					ustats->maxMsgCnt=usnap.maxMsgCnt=usnap.msgCnt;
				int uperc=0;
				VSDetailFile *dfile=(VSDetailFile*)UscPort[i].DetPtr1;
				if(dfile)
				{
					#ifdef WIN32
					uperc=(int)(dfile->rend.QuadPart*100/(dfile->fend.QuadPart?dfile->fend.QuadPart:1));
					rtot+=dfile->rend.QuadPart; wtot+=dfile->fend.QuadPart;
					#else
					uperc=(int)(dfile->rend*100/(dfile->fend?dfile->fend:1));
					rtot+=dfile->rend; wtot+=dfile->fend;
					#endif
				}
				sprintf(UscPort[i].Status,"%s mps (%d%%), %s max, %s tot",
					SizeStr(usnap.msgCnt,false),uperc,SizeStr(usnap.maxMsgCnt,false),SizeStr(usnap.totMsgCnt,false));
				uscStats+=usnap;
				uscStats.totMsgCnt+=(totincr -usnap.msgCnt);				

				VSDistJournal *dj=(VSDistJournal *)UscPort[i].DetPtr5;
				if(dj)
				{
					jsize+=dj->GetMemUsage();
					jmax+=(MAX_DISTJ_SIZE*1024*1024);// +(50*1024*1024);
				}
			}
		#ifndef TASK_THREAD_POOL
		#ifdef _DEBUG
			if((!odb.HAVE_FUSION_IO)&&(UscPort[i].DetPtr!=(void*)PROTO_CLS_BTR))
			{
				// For testing MRU cache
				VSMruCache *pmru=(VSMruCache*)UscPort[i].DetPtr4;
				if(pmru)
				{
					int hperc=(int)(pmru->nhit*100/(pmru->nsearch?pmru->nsearch:1));
					sprintf(UscPort[i].Status +strlen(UscPort[i].Status)," [%d%% hit]",hperc);
				}
			}
		#endif
		#endif
			// Nag about disconnected sessions
			if((UscPort[i].DetPtr==(void*)PROTO_FIX)&&
			   (FIX_HOURS_BEGIN>0)&&(WSTime>=FIX_HOURS_BEGIN)&&(FIX_HOURS_END>0)&&(WSTime<FIX_HOURS_END)&&(WSTime%5==0))
			{
				bool found=false;
				for(int j=0;j<NO_OF_USR_PORTS;j++)
				{
					if((UsrPort[j].SockActive)&&(UsrPort[j].UscPort==i))
					{
						found=true;
						break;
					}
				}
				if(!found)
					WSLogError("USC%d: No FIX clients connected at %s:%d for (%s) between [%06d,%06d)!",
						i,UscPort[i].LocalIP,UscPort[i].Port,UscPort[i].CfgNote,FIX_HOURS_BEGIN,FIX_HOURS_END);
			}
		}
		WSUnlockPort(WS_USC,i,false);
	}
	if(uscStats.msgCnt>uscStats.maxMsgCnt)
		uscStats.maxMsgCnt=uscStats.msgCnt;
	uscStats.SetAverages();
	int uperc=(int)(rtot*100/(wtot?wtot:1));
	sprintf(UscPort[NO_OF_USC_PORTS].Status,"%s mps (%d%%), %s max, %s tot",
		SizeStr(uscStats.msgCnt,false),uperc,SizeStr(uscStats.maxMsgCnt,false),SizeStr(uscStats.totMsgCnt,false));

	string rstr;
	if(odb.readonly)
		rstr=" (READONLY)";
	char capstat[1024]={0};
#ifdef MULTI_DAY_HIST
	if((VSDIST_HUB)&&(odb.HISTORIC_DAYS>0))
	{
		rstr=" (HISTORIC)";
		LONGLONG omem=odb.GetMemOrderSize();
		int operc=(int)(omem*100/(odb.ORDER_CACHE_SIZE?odb.ORDER_CACHE_SIZE*1024*1024:1));
		sprintf(capstat,"%s%s: PTot=%s   OTot=%s   OTerm=%s   OCache=%s (%d%%)",
			HEADING.c_str(),rstr.c_str(),
			SizeStr(pcnt,false),
		#ifdef NOMAP_TERM_ORDERS
			SizeStr(ocnt +oterm,false),
		#else
			SizeStr(ocnt,false),
		#endif
			SizeStr(oterm,false),
			SizeStr(omem),operc);
	}
	else
#endif
	if(VSDIST_HUB)
	{
		LONGLONG omem=odb.GetMemOrderSize();
		int operc=odb.ORDER_CACHE_SIZE ?(int)(omem*100/(odb.ORDER_CACHE_SIZE*1024*1024)):0;
		sprintf(capstat,"%s%s: TotMsgs=%s (%d%%)   Msg/Sec=%s   Max=%s   PTot=%s   OTot=%s   OTerm=%s   OCache=%s (%d%%)",
			HEADING.c_str(),rstr.c_str(),
			SizeStr(uscStats.totMsgCnt,false),uperc,SizeStr(uscStats.msgCnt,false),SizeStr(uscStats.maxMsgCnt,false),
			SizeStr(pcnt,false),
		#ifdef NOMAP_TERM_ORDERS
			SizeStr(ocnt +oterm,false),
		#else
			SizeStr(ocnt,false),
		#endif
			SizeStr(oterm,false),
			SizeStr(omem),operc);
	}
	else
	{
		int jperc=(int)(jsize*100/(jmax?jmax:1));
		sprintf(capstat,"%s%s: TotMsgs=%s (%d%%)   Msg/Sec=%s   Max=%s   DistJ=%s (%d%%)",
			HEADING.c_str(),rstr.c_str(),
			SizeStr(uscStats.totMsgCnt,false),uperc,SizeStr(uscStats.msgCnt,false),SizeStr(uscStats.maxMsgCnt,false),
			SizeStr(jsize,true),jperc);
	}
	WSSetAppHead(capstat);
	conStats.msgCnt=0;
	uscStats.msgCnt=0;

#ifdef IQDNS
	if((SAM_REPORT_TIME>0)&&(VSDIST_HUB)&&(WSTime==SAM_REPORT_TIME))
	{
		char fpath[MAX_PATH]={0};
		IQSAMReport(fpath,true,0,0);
		SAMReport(fpath,true,0,0);
	}
#endif

#ifdef MULTI_DAY_HIST
	if(odb.HISTORIC_DAYS>0)
	{
		if ((HISTIMP_TIME > 0) && (HISTIMP_TIME == WSTime))
		{
			ImportHistoricFiles(WSDate); 
			importHistoricFiles = true;
			DBLoadThread();
		}
		for (list<HistImport>::iterator hit = histImports.begin(); hit != histImports.end(); hit++)
		{
			HistImport& import = *hit;
			if ((import.HISTIMP_TIME > 0) && (import.HISTIMP_TIME == WSTime))
			{
				ImportHistoricFiles(WSDate, &import);
				importHistoricFiles = true;
				DBLoadThread();
			}
		}
	}
#endif

	// Auto-shutdown during the minute of exit
	if((SERVER_EXIT_TIME>0)&&(WSTime>=SERVER_EXIT_TIME)&&(WSTime/100==SERVER_EXIT_TIME/100))
	{
		WSLogEvent("SERVER_EXIT_TIME expired.");

		//WSExit(0);
		// Trying this instead
		int pid = _getpid();
		char buff[256];
		sprintf(buff, "taskkill /pid %d /f", pid);
		system(buff);
	}
}

// Log file playback
struct PlayThreadData
{
	ViewServer *pmod;
	int PortNo;
	int proto;
	string fmatch;
	bool cxl;
	int rstart;
	float playRate;
};
#ifdef WIN32
static DWORD WINAPI _BootPlayMLThread(LPVOID arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	int rc=ptd->pmod->PlayMLLogs(ptd->PortNo,ptd->proto,ptd->fmatch.c_str(),ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return rc;
}
static DWORD WINAPI _BootPlayCldropThread(LPVOID arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	int rc=ptd->pmod->PlayCldropLogs(ptd->PortNo,ptd->proto,ptd->fmatch.c_str(),ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return rc;
}
static DWORD WINAPI _BootPlayTraderThread(LPVOID arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	char sdir[MAX_PATH]={0},*sptr=0;
	GetFullPathNameEx(ptd->fmatch.c_str(),MAX_PATH,sdir,&sptr);
	if(sptr) sptr[-1]=0;
	else return -1;
	int rc=ptd->pmod->PlayTraderLogs(ptd->PortNo,ptd->proto,sdir,sptr,ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return rc;
}
static DWORD WINAPI _BootPlayCoverageToolThread(LPVOID arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	char sdir[MAX_PATH]={0},*sptr=0;
	GetFullPathNameEx(ptd->fmatch.c_str(),MAX_PATH,sdir,&sptr);
	if(sptr) sptr[-1]=0;
	else return -1;
	int rc=ptd->pmod->PlayCoverageToolExport(ptd->PortNo,ptd->proto,sdir,sptr,ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return rc;
}
#ifdef IQSMP
static DWORD WINAPI _BootPlaySmpThread(LPVOID arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	char sdir[MAX_PATH]={0},*sptr=0;
	GetFullPathNameEx(ptd->fmatch.c_str(),MAX_PATH,sdir,&sptr);
	if(sptr) sptr[-1]=0;
	else return -1;
	int rc=ptd->pmod->PlaySmpLogs(ptd->PortNo,ptd->proto,sdir,sptr,ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return rc;
}
#endif
#else
static void *_BootPlayThread(void *arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	int rc=ptd->pmod->PlayMLLogs(ptd->PortNo,ptd->proto,ptd->fmatch.c_str(),ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return (void*)(PTRCAST)rc;
}
static void *_BootPlayCldropThread(void *arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	int rc=ptd->pmod->PlayCldropLogs(ptd->PortNo,ptd->proto,ptd->fmatch.c_str(),ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return (void*)(PTRCAST)rc;
}
static void *_BootPlayTraderThread(void *arg)
{
	PlayThreadData *ptd=(PlayThreadData *)arg;
	SleepEx(100,true); // wait a little on startup to allow UI thread to refresh
	char sdir[MAX_PATH]={0},*sptr=0;
	GetFullPathNameEx(ptd->fmatch.c_str(),MAX_PATH,sdir,&sptr);
	if(sptr) sptr[-1]=0;
	else return (void*)-1;
	int rc=ptd->pmod->PlayTraderLogs(ptd->PortNo,ptd->proto,sdir,sptr,ptd->cxl,ptd->rstart,ptd->playRate);
	delete ptd;
	return (void*)(PTRCAST)rc;
}
#endif
#ifdef AWAY_FIXSERVER
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
#endif

#ifdef REDIOLEDBCON
#pragma pack(push,1)
// Request All Symbols by Asset Type
typedef struct tdMsg345Rec
{
    unsigned char chAssetType;
    char szReserved[31];
} MSG345REC;    // 32 bytes

// Symbol Info
typedef struct tdMsg346Rec
{
    char szSymbol[SYMBOL_LEN-2];
    unsigned char chAssetType;
    unsigned char chIQMC;
    char szCurrency[4];
    unsigned int nLotQuantity;
    unsigned char chFormatCode;
    union
    {
            char chFlags;
            struct
            {
                    bool bStart             : 1;
                    bool bEnd               : 1;                    
            };
                
    };
	char szISIN[13];
	char szReserved[19];
} MSG346REC;    // 64 bytes
#pragma pack(pop)
#endif

void ViewServer::WSConOpened(int PortNo)
{
	WSLogError("CON%d connected to %s:%d...",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
	// Log file playback
	if((!strncmp(ConPort[PortNo].CfgNote,"$MLFIXLOGS$",11))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$MLPUBHUB$",10))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$ITF$",5))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$IQOMS$",7))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$IQFIXS$",8))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TJMLOG$",8))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$CLDROP$",8))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$FULLLOG$",9))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TJM$",5))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$MLPH_EMEA$",11))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$CLBACKUP$",10))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$MLPH_CC$",9))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$FIXS_PLACELOG$",15))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$FIXS_CXLLOG$",13))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$FIXS_RPTLOG$",13))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$FIXS_EVELOG$",13))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$CLS_EVENTLOG$",14))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TRADER_PLACELOG$",17))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TRADER_CXLLOG$",15))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TRADER_RPTLOG$",15))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TRADER_EVELOG$",15))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$RTOATS$",8))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TWISTLOG$",10))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$TWIST$",7))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$OMS_CBOE$",10))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$CLS_BTR$",9))||
	   (!strncmp(ConPort[PortNo].CfgNote,"$COVERAGETOOL$",14)))
	{
		if(VSDIST_HUB)
		{
			WSLogError("CON%d: VSDIST_HUB must be set to 0 for CON playback ports! Holding port.",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr=(void*)PROTO_MLFIXLOGS;
		ConPort[PortNo].DetPtr3=new FixStats;
		PlayThreadData *ptd=new PlayThreadData;
		ptd->pmod=this;
		ptd->PortNo=PortNo;
		ptd->rstart=0;
		ptd->playRate=1.0;
		char fmatch[1024]={0};
		if(!strncmp(ConPort[PortNo].CfgNote,"$MLFIXLOGS$",11))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +11);
			ptd->proto=PROTO_MLFIXLOGS;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$MLPUBHUB$",10))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +10);
			ptd->proto=PROTO_MLPUBHUB;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$ITF$",5))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +5);
			ptd->proto=PROTO_ITF;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$IQOMS$",7))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +7);
			ptd->proto=PROTO_IQOMS;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$IQFIXS$",8))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +8);
			ptd->proto=PROTO_IQFIXS;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TJMLOG$",8))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +8);
			ptd->proto=PROTO_TJMLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$CLDROP$",8))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +8);
			ptd->proto=PROTO_CLDROP;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$FULLLOG$",9))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +9);
			ptd->proto=PROTO_FULLLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TJM$",5))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +5);
			ptd->proto=PROTO_TJM;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$MLPH_EMEA$",11))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +11);
			ptd->proto=PROTO_MLPH_EMEA;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$CLBACKUP$",10))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +10);
			ptd->proto=PROTO_CLBACKUP;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$MLPH_CC$",9))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +9);
			ptd->proto=PROTO_MLPH_CC;
		}
	#ifdef IQSMP
		else if(!strncmp(ConPort[PortNo].CfgNote,"$FIXS_PLACELOG$",15))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +15);
			ptd->proto=PROTO_FIXS_PLACELOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$FIXS_CXLLOG$",13))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +13);
			ptd->proto=PROTO_FIXS_CXLLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$FIXS_RPTLOG$",13))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +13);
			ptd->proto=PROTO_FIXS_RPTLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$FIXS_EVELOG$",13))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +13);
			ptd->proto=PROTO_FIXS_EVELOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$CLS_EVENTLOG$",14))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +14);
			ptd->proto=PROTO_CLS_EVENTLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TRADER_PLACELOG$",17))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +17);
			ptd->proto=PROTO_TRADER_PLACELOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TRADER_CXLLOG$",15))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +15);
			ptd->proto=PROTO_TRADER_CXLLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TRADER_RPTLOG$",15))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +15);
			ptd->proto=PROTO_TRADER_RPTLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TRADER_EVELOG$",15))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +15);
			ptd->proto=PROTO_TRADER_EVELOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$RTOATS$",8))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +8);
			ptd->proto=PROTO_RTOATS;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TWISTLOG$",10))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +10);
			ptd->proto=PROTO_TWISTLOG;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$TWIST$",7))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +7);
			ptd->proto=PROTO_TWIST;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$OMS_CBOE$",10))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +10);
			ptd->proto=PROTO_OMS_CBOE;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$CLS_BTR$",9))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +9);
			ptd->proto=PROTO_CLS_BTR;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$RTECHOATS$",11))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +11);
			ptd->proto=PROTO_RTECHOATS;
		}
		else if(!strncmp(ConPort[PortNo].CfgNote,"$COVERAGETOOL$",14))
		{
			strcpy(fmatch,ConPort[PortNo].CfgNote +14);
			ptd->proto=PROTO_COVERAGETOOL;
		}
	#endif
		// Playback from file
		if(!strncmp(fmatch,"PLAY$",5))
		{
			memmove(fmatch,fmatch+5,strlen(fmatch+5)+1);
		#ifdef IQSMP
			// Optional AppInstID override
			const char *dptr=strchr(fmatch,'$');
			if(dptr)
			{
				char smpaid[80]={0};
				strncpy(smpaid,fmatch,dptr -fmatch); smpaid[(int)(dptr -fmatch)]=0;
				memmove(fmatch,dptr +1,strlen(dptr));
				// Inject an ID line we know how to parse out to override the AppInstID
				if(ptd->proto==PROTO_FULLLOG)
				{
					char smpidmsg[80]={0};
					sprintf(smpidmsg,"8=FIX.4.2|9=0|34=0|11505=%s|52=00000000-00:00:00|10=0|\r\n",smpaid);
					WSConSendBuff(strlen(smpidmsg),smpidmsg,PortNo,1);
				}
				else if(ptd->proto==PROTO_OMS_CBOE)
				{
					char smpidmsg[80]={0};
					sprintf(smpidmsg,"A:0:0:0:8=FIX.4.2\0019=0\00134=0\00111505=%s\00152=00000000-00:00:00\00110=0\001\r\n",smpaid);
					WSConSendBuff(strlen(smpidmsg),smpidmsg,PortNo,1);
				}
				else if((ptd->proto==PROTO_CLS_EVENTLOG)||(ptd->proto==PROTO_FIXS_EVELOG)||(ptd->proto==PROTO_TRADER_EVELOG))
				{
					char smpidmsg[80]={0};
					sprintf(smpidmsg,"APPINST:%s\r\nBUILD:00000000-000000\r\n",smpaid);
					WSConSendBuff(strlen(smpidmsg),smpidmsg,PortNo,1);
				}
				else if((ptd->proto==PROTO_FIXS_PLACELOG)||(ptd->proto==PROTO_FIXS_CXLLOG)||(ptd->proto==PROTO_FIXS_RPTLOG)||
						(ptd->proto==PROTO_TRADER_PLACELOG)||(ptd->proto==PROTO_TRADER_CXLLOG)||(ptd->proto==PROTO_TRADER_RPTLOG))
				{
					APPINSTMARKER aim;
					memset(&aim,0,sizeof(APPINSTMARKER));
					aim.marker=0xFEFEFEFE;
					char *aptr=strchr(smpaid,',');
					if(aptr)
					{
						*aptr=0;
						aim.buildDate=atoi(aptr+1);
						if(aptr[5]=='_') aim.buildTime=atoi(aptr+6); // e.g. 2041_0
						else aim.buildTime=atoi(aptr+10); // e.g. yyyymmdd-hhmmss
					}
					strcpy(aim.AppInstID,smpaid);
					WSConSendBuff(sizeof(APPINSTMARKER),(char*)&aim,PortNo,1);
				}
			}
		#endif
		#ifdef AWAY_FIXSERVER
			if(ptd->proto==PROTO_CLS_BTR)
			{
				MSGHEADER GDMsgHeader;
				GDHEADER GDHeader;
				GDID GDId;
				GDLOGIN GDLogin;
				// TODO: Figure out how to read this info
				GDMsgHeader.MsgID=GD_MSG_SEND;
				GDMsgHeader.MsgLen=sizeof(GDHEADER)+sizeof(GDID)+sizeof(GDLOGIN);
				GDId.LineId=1401;
				strcpy(GDId.LineName,"FIXSERVER");
				strcpy(GDId.ClientId,"TIAD");
				//GDId.LineId=1402;
				//strcpy(GDId.LineName,"RJLCAP");
				//strcpy(GDId.ClientId,"MEZWARE1");
				strcpy(GDId.SessionId,WScDate);
				GDHeader.Type=GD_TYPE_LOGIN;
				GDHeader.SourceId=0;
				GDHeader.GDLineId=1401;//pmod->CgdPort[PortNo].GDId.LineId;
				GDHeader.Reserved=0;
				GDHeader.GDLogId=0;//pmod->CgdPort[PortNo].NextGDInLogId;
				GDLogin.NextSendId=0;
				GDLogin.LastRecvId=0;
				WSConSendBuff(sizeof(MSGHEADER),(char*)&GDMsgHeader,PortNo,0);
				WSConSendBuff(sizeof(GDHEADER),(char*)&GDHeader,PortNo,0);
				WSConSendBuff(sizeof(GDID),(char*)&GDId,PortNo,0);
				WSConSendBuff(sizeof(GDLOGIN),(char*)&GDLogin,PortNo,1);
			}
		#endif
			if(fmatch[1]==';')
				fmatch[1]=':';
			char *rstart=strchr(fmatch,'$');
			if(rstart)
			{
				*rstart=0; ptd->rstart=atoi(rstart+1);
				rstart=strchr(rstart+1,'x');
				if(rstart)
				{
					float prate=(float)atof(rstart+1);
					if(prate>0)
						ptd->playRate=prate;
				}
			}
			ptd->fmatch=fmatch;
			ptd->cxl=false;
			ConPort[PortNo].DetPtr5=&ptd->cxl;
		#ifdef WIN32
			DWORD tid=0;
			if((ptd->proto==PROTO_CLDROP)||(ptd->proto==PROTO_CLBACKUP))
				ConPort[PortNo].DetPtr7=CreateThread(0,0,_BootPlayCldropThread,ptd,0,&tid);
			else if(ptd->proto==PROTO_FULLLOG)
				ConPort[PortNo].DetPtr7=CreateThread(0,0,_BootPlayTraderThread,ptd,0,&tid);
			else if(ptd->proto==PROTO_COVERAGETOOL)
				ConPort[PortNo].DetPtr7=CreateThread(0,0,_BootPlayCoverageToolThread,ptd,0,&tid);
			#ifdef IQSMP
			else if((ptd->proto==PROTO_IQFIXS)||
					(ptd->proto==PROTO_FIXS_PLACELOG)||(ptd->proto==PROTO_FIXS_CXLLOG)||(ptd->proto==PROTO_FIXS_RPTLOG)||(ptd->proto==PROTO_FIXS_EVELOG)||
					(ptd->proto==PROTO_CLS_EVENTLOG)||(ptd->proto==PROTO_CLS_BTR)||
					(ptd->proto==PROTO_TRADER_PLACELOG)||(ptd->proto==PROTO_TRADER_CXLLOG)||(ptd->proto==PROTO_TRADER_RPTLOG)||(ptd->proto==PROTO_TRADER_EVELOG)||
					(ptd->proto==PROTO_RTOATS)||(ptd->proto==PROTO_TWISTLOG)||(ptd->proto==PROTO_TWIST)||(ptd->proto==PROTO_OMS_CBOE)||(ptd->proto==PROTO_RTECHOATS))
				ConPort[PortNo].DetPtr7=CreateThread(0,0,_BootPlaySmpThread,ptd,0,&tid);
			#endif
			else
				ConPort[PortNo].DetPtr7=CreateThread(0,0,_BootPlayMLThread,ptd,0,&tid);
		#else
			pthread_t *thnd=new pthread_t;
			ConPort[PortNo].DetPtr7=thnd;
			if((ptd->proto==PROTO_CLDROP)||(ptd->proto==PROTO_CLBACKUP))
				pthread_create(thnd,0,_BootPlayCldropThread,ptd);
			else if(ptd->proto==PROTO_FULLLOG)
				pthread_create(thnd,0,_BootPlayTraderThread,ptd);
			else
				pthread_create(thnd,0,_BootPlayMLThread,ptd);
		#endif
		}
		// Otherwise, CON port connection
		else
		{
			int UscPortNo=atoi(fmatch);
			if((!isdigit(fmatch[0]))||(UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
			{
				WSLogError("CON%d: Invalid USC portno (%s)!",PortNo,fmatch);
				ConPort[PortNo].ConnectHold=true;
				return;
			}
			ConPort[PortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;
		}
	}
	else if(!strncmp(ConPort[PortNo].CfgNote,"$FIXDROP$",9))
	{
		// Session ids
		ConPort[PortNo].DetPtr=(void*)PROTO_FIXDROP;
		ConPort[PortNo].DetPtr3=new FixStats;
		int UscPortNo=-1;
		char sess[256]={0};
		strcpy(sess,ConPort[PortNo].CfgNote +9);
		bool passive=false;
		char *sptr=strchr(sess,'$');
		if(sptr)
		{
			*sptr=0; UscPortNo=atoi(sptr+1);
		}
		if(strrcmp(sess,",PASSIVE"))
		{
			sess[strlen(sess) -8]=0;
			passive=true;
		}
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		{
			WSLogError("CON%d: Invalid USC portno after $FIXDROP$!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		DropClient *pdc=0;
		DCMAP::iterator dit=dcmap.find(PortNo);
		if(dit==dcmap.end())
		{
			pdc=new DropClient;
			if(pdc->Init(sess,0,0,passive,this,(void*)(PTRCAST)MAKELONG(PortNo,WS_CON))<0)
			{
				delete pdc;
				WSLogError("CON%d: Invalid session ids after $FIXDROP$!",PortNo);
				ConPort[PortNo].ConnectHold=true;
				return;
			}
			dcmap[PortNo]=pdc;
		}
		else
			pdc=dit->second;
		ConPort[PortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;
		ConPort[PortNo].DetPtr5=pdc;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
			pdc->GetLastSeqNos(dfile->fpath.c_str());
			int i;
			for(i=0;i<pdc->iseq;i++)
			{
				DropClient::INBLOCK gap;
				if(pdc->GetGap(gap,i)>0)
					WSLogEvent("Gap detected [%d,%d)",gap.begin,gap.end);
				else
					break;
			}
			if(!i)
				WSLogEvent("No gaps detected");
		}
		if(pdc->Login(30)<0)
		{
			WSLogError("CON%d: Failed Login!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
	}
	else if(!strncmp(ConPort[PortNo].CfgNote,"$TBACKUP$",9))
	{
		// Session ids
		ConPort[PortNo].DetPtr=(void*)PROTO_TBACKUP;
		ConPort[PortNo].DetPtr3=new FixStats;
		int UscPortNo=atoi(ConPort[PortNo].CfgNote +9);
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		{
			WSLogError("CON%d: Invalid USC portno after $TBACKUP$!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		if(TBackupOpened(PortNo)<0)
		{
			WSLogError("CON%d: TBackupOpened failed!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;
	}
	// Connection to central order server
	else if(!strncmp(ConPort[PortNo].CfgNote,"$VSDIST$",8))
	{
		if(VSDIST_HUB)
		{
			WSLogError("CON%d: VSDIST_HUB must be set to 0 for CON$VSDIST$ ports! Holding port.",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr=(void*)PROTO_VSDIST;
		ConPort[PortNo].DetPtr3=new FixStats;
		int UscPortNo=atoi(ConPort[PortNo].CfgNote +8);
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		{
			WSLogError("CON%d: Invalid USC portno after $VSDIST$!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;

		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(!dfile)
		{
			WSLogError("CON%d: VSDetailFile not found for USC%d!",PortNo,UscPortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		VSDistJournal *dj=(VSDistJournal *)UscPort[UscPortNo].DetPtr5;
		if(!dj)
		{
			WSLogError("CON%d: VSDistJournal not found for USC%d!",PortNo,UscPortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		// Reset the send time for all JPAGEs
		dj->Lock();
		for(JPAGE *jpage=dj->firstPage;jpage;jpage=jpage->next)
			jpage->timeSent=0;
		dj->Unlock();

		VSDistCodec *vsd=new VSDistCodec;
		if(vsd->Init(this)<0)
		{
			WSLogError("CON%d: Failed to initialize VSDistCodec!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr6=vsd;
		DWORD& rid=(DWORD&)ConPort[PortNo].DetPtr5;

		char aline[1024]={0},dbpath[MAX_PATH]={0},*dfptr=0;
		GetFullPathNameEx(dfile->fpath.c_str(),sizeof(dbpath),dbpath,&dfptr);
		if(dfptr) dfptr[-1]=0;
		char wbuf[1024],*wptr=wbuf;
		if(vsd->EncodeAliasRequest(wptr,sizeof(wbuf),++rid,dfile->prefix.c_str(),dbpath,(int)dfile->proto,dfile->wsdate)>0)
		{
			WSConSendMsg(100,(int)(wptr -wbuf),wbuf,PortNo);
		}
		else
			_ASSERT(false);
	}
	// Connection to T-1 or VSDB for other region
	else if(!strncmp(ConPort[PortNo].CfgNote,"$VSCLIENT$",10))
	{
		if(!VSDIST_HUB)
		{
			WSLogError("CON%d: VSDIST_HUB must be set to 1 for CON$VSCLIENT$ ports! Holding port.",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr=(void*)PROTO_VSCLIENT;
		//ConPort[PortNo].DetPtr3=new FixStats;
		ConPort[PortNo].DetPtr5=new VSQUERYMAP;
		VSCodec *vsc=new VSCodec;
		if(vsc->Init(this)<0)
		{
			delete vsc;
			WSLogError("CON%d: Failed to initialize VSCodec!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr6=vsc;

		char rbuf[256],*rptr=rbuf;
		int& rid=(int&)ConPort[PortNo].DetPtr4;
		if(vsc->EncodeLoginRequest2(rptr,sizeof(rbuf),rid,REGION.c_str(),REGION_PASS.c_str())>0)
		{
			WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
			rid++;
		}
		else
		{
			WSLogError("CON%d: Failed EncodeLoginRequest",PortNo);
			ConPort[PortNo].ConnectHold=true;
		}
	}
#ifdef IQDNS
	// DNS replication
	else if(!strncmp(ConPort[PortNo].CfgNote,"$DNS_REPL$",10))
	{
		if(!VSDIST_HUB)
		{
			WSLogError("CON%d: VSDIST_HUB must be set to 1 for CON$DNS_REPL$ ports! Holding port.",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr=(void*)PROTO_DNS_REPL;
		ConPort[PortNo].DetPtr3=new FixStats;
		if(DnsReplOpened(PortNo)<0)
		{
			WSLogError("CON%d: DnsReplOpened failed!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
	}
	// DNS load balance
	else if(!strncmp(ConPort[PortNo].CfgNote,"$DNS_MGR$",9))
	{
		if(!VSDIST_HUB)
		{
			WSLogError("CON%d: VSDIST_HUB must be set to 1 for CON$DNS_MGR$ ports! Holding port.",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		ConPort[PortNo].DetPtr=(void*)PROTO_DNS_MGR;
		ConPort[PortNo].DetPtr3=new FixStats;
		if(DnsMgrOpened(PortNo)<0)
		{
			WSLogError("CON%d: DnsMgrOpened failed!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
	}
#endif
#ifdef REDIOLEDBCON
	// Connection to central order server
	else if(!strncmp(ConPort[PortNo].CfgNote,"$REDIOLEDB$",11))
	{
		ConPort[PortNo].DetPtr=(void*)PROTO_REDIOLEDB;
	}
	else if(!strncmp(ConPort[PortNo].CfgNote,"$SYMBLIST$",10))
	{
		ConPort[PortNo].DetPtr=(void*)PROTO_SYMBLIST;
		ConPort[PortNo].DetPtr3=new FixStats;
		// Send block request for all equities
		MSG345REC Msg345Rec;
		memset(&Msg345Rec,0,sizeof(MSG345REC));
		Msg345Rec.chAssetType=IQAST_EQUITY;
		WSConSendMsg(345,sizeof(MSG345REC),(char*)&Msg345Rec,PortNo);
		// Send block request for all futures
		memset(&Msg345Rec,0,sizeof(MSG345REC));
		Msg345Rec.chAssetType=IQAST_FUTURE;
		WSConSendMsg(345,sizeof(MSG345REC),(char*)&Msg345Rec,PortNo);	
	}
#endif
	else
		ConPort[PortNo].DetPtr=(void*)PROTO_UNKNOWN;
}
int ViewServer::WSConMsgReady(int PortNo)
{
	int Size=ConPort[PortNo].InBuffer.Size;
	if(Size<1)
		return 0;	
	DWORD& lastReadyStart=(DWORD&)ConPort[PortNo].lastReadyStart;
	DWORD tlast=lastReadyStart;
	DWORD tnow=GetTickCount();

	lastReadyStart=0;
	if((MAX_CON_TIME>0)&&(tlast)&&((int)(tnow -tlast)>=MAX_CON_TIME))
		return(0);
	lastReadyStart=tlast?tlast:tnow;

	Protocol proto=(Protocol)(PTRCAST)ConPort[PortNo].DetPtr;
	if((proto==PROTO_MLFIXLOGS)||(proto==PROTO_MLPUBHUB)||(proto==PROTO_ITF)||(proto==PROTO_MLPH_EMEA)||(proto==PROTO_MLPH_CC)||
	   (proto==PROTO_IQOMS)||(proto==PROTO_IQFIXS)||(proto==PROTO_TJMLOG)||(proto==PROTO_FULLLOG)||(proto==PROTO_TJM))
	{
		int UscPortNo=(int)(PTRCAST)ConPort[PortNo].DetPtr4;
		if((!ConPort[PortNo].DetPtr7)&&((UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS)))
		{
			Size=ConPort[PortNo].InBuffer.LocalSize;
			VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
			if(dfile)
			{
			#ifndef NO_FIX_WRITES
				LONGLONG doff=-1;
				odb.WriteDetailBlock(dfile,ConPort[PortNo].InBuffer.Block,Size,doff);
			#endif
			}
			ConPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
			ConPort[PortNo].PacketsIn++;
			FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
			if(cstats)
			{
				cstats->msgCnt+=Size; cstats->totMsgCnt+=Size;
			}
			return 1;
		}
		return 0;
	}
	else if((proto==PROTO_VSCLIENT)||(proto==PROTO_VSDIST)||(proto==PROTO_TBACKUP)||
			(proto==PROTO_DNS_REPL)||(proto==PROTO_DNS_MGR)||(proto==PROTO_SYMBLIST))
	{
		Size=ConPort[PortNo].InBuffer.LocalSize;
		if(Size<sizeof(MSGHEADER))
			return 0;
		MSGHEADER *MsgHeader=(MSGHEADER*)ConPort[PortNo].InBuffer.Block;
		if(Size<(int)sizeof(MSGHEADER) +MsgHeader->MsgLen)
			return 0;
		char *Msg=(char *)MsgHeader +sizeof(MSGHEADER);
		ConPort[PortNo].DetPtr1=(void*)(sizeof(MSGHEADER) +MsgHeader->MsgLen);
		if(TranslateConMsg(MsgHeader,Msg,PortNo)<0)
		{
			WSCloseConPort(PortNo);
			return -1;
		}
		ConPort[PortNo].PacketsIn++;
		return 1;
	}
	else if(proto==PROTO_FIXDROP)
	{
		DropClient *pdc=(DropClient*)ConPort[PortNo].DetPtr5;
		if(pdc)
		{
			Size=pdc->DecodeFix(ConPort[PortNo].InBuffer.Block,Size);
			if(Size<1)
				return 0;
			ConPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
			return 1;
		}
		return 0;
	}
	else
	{
		ConPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
		ConPort[PortNo].PacketsIn++;
		return 1;
	}
}
int ViewServer::WSConStripMsg(int PortNo)
{
	int Size=(int)(PTRCAST)ConPort[PortNo].DetPtr1;
	while(Size>0)
	{
		int slen=Size;
		if(slen>ConPort[PortNo].BlockSize*1024)
			slen=ConPort[PortNo].BlockSize*1024;
		if(WSStripBuff(&ConPort[PortNo].InBuffer,slen)<0)
		{
			WSCloseConPort(PortNo);
			return -1;
		}
		Size-=slen;
	}
	return 0;
}
int ViewServer::WSBeforeConSend(int PortNo)
{
	return 0;
}
void ViewServer::WSConClosed(int PortNo)
{
	WSLogError("CON%d disconnected from %s:%d.",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
	Protocol proto=(Protocol)(PTRCAST)ConPort[PortNo].DetPtr;
	if((proto==PROTO_MLFIXLOGS)||(proto==PROTO_MLPUBHUB)||(proto==PROTO_ITF)||(proto==PROTO_MLPH_EMEA)||(proto==PROTO_MLPH_CC)||
	   (proto==PROTO_IQOMS)||(proto==PROTO_IQFIXS)||(proto==PROTO_TJMLOG)||(proto==PROTO_FULLLOG)||(proto==PROTO_TJM))
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
		//ConPort[PortNo].ConnectHold=true;
		HANDLE thnd=(HANDLE)ConPort[PortNo].DetPtr7;
		if(thnd)
		{
			// Don't wait INFINITE for the play thread, because it can wait on this ConPort for send.
			// Only tell the play thread to cancel if it's still running.
			if(WaitForSingleObject(thnd,100)!=WAIT_OBJECT_0)
			{
				bool *pcxl=(bool *)ConPort[PortNo].DetPtr5;
				if(pcxl)
				{
					*pcxl=true; ConPort[PortNo].DetPtr5=0;
				}
			}
			#ifdef WIN32
			CloseHandle(thnd); ConPort[PortNo].DetPtr7=0;
			#else
			DeleteMutex(thnd); ConPort[PortNo].DetPtr7=0;
			#endif
		}
	}
	else if(proto==PROTO_VSDIST)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
		VSDistCodec *vsd=(VSDistCodec *)ConPort[PortNo].DetPtr6;
		if(vsd)
		{
			vsd->Shutdown();
			delete vsd; ConPort[PortNo].DetPtr6=0;
		}
	}
	else if(proto==PROTO_FIXDROP)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
	}
	else if(proto==PROTO_TBACKUP)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
		TBackupClosed(PortNo);
	}
	else if(proto==PROTO_FULLLOG)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
	}
	else if(proto==PROTO_VSCLIENT)
	{
		//FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		//if(cstats)
		//{
		//	delete cstats; ConPort[PortNo].DetPtr3=0;
		//}
		VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
		if(rtmap)
		{
			delete rtmap; ConPort[PortNo].DetPtr5=0;
		}
		VSCodec *vsc=(VSCodec *)ConPort[PortNo].DetPtr6;
		if(vsc)
		{
			vsc->Shutdown();
			delete vsc; ConPort[PortNo].DetPtr6=0;
		}
		APPSYSMAP *ramap=(APPSYSMAP *)ConPort[PortNo].DetPtr7;
		if(ramap)
		{
			for(APPSYSMAP::iterator ait=ramap->begin();ait!=ramap->end();ait++)
			{
				AppSystem *asys=ait->second;
				delete asys;
			}
			ramap->clear();
			delete ramap; ConPort[PortNo].DetPtr7=0;
		}
	}
#ifdef IQDNS
	else if(proto==PROTO_DNS_REPL)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
		DnsReplClosed(PortNo);
	}
	else if(proto==PROTO_DNS_MGR)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
		DnsMgrClosed(PortNo);
	}
#endif
#ifdef REDIOLEDBCON
	else if(proto==PROTO_REDIOLEDB)
	{
		REDIHEARTBEAT *redihb=(REDIHEARTBEAT*)ConPort[PortNo].DetPtr3;
		if(redihb)
		{
			delete redihb; ConPort[PortNo].DetPtr3=0;
		}
	}
#endif
#ifdef AWAY_FIXSERVER
	else if(proto==PROTO_CLS_BTR)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
	}
#endif
#ifdef REDIOLEDBCON
	else if(proto==PROTO_SYMBLIST)
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			delete cstats; ConPort[PortNo].DetPtr3=0;
		}
	}
#endif
}
void ViewServer::WSFileOpened(int PortNo)
{
	WSLogError("FILE%d connected to %s:%d...",PortNo,FilePort[PortNo].RemoteIP,FilePort[PortNo].Port);
}
void ViewServer::WSFileClosed(int PortNo)
{
	WSLogError("FILE%d disconnected from %s:%d.",PortNo,FilePort[PortNo].RemoteIP,FilePort[PortNo].Port);
}
struct BootReadDetailThreadData
{
	ViewServer *pmod;
	VSDetailFile *dfile;
};
#ifdef WIN32
static DWORD WINAPI _BootReadDetailThread(LPVOID arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadDetailThread(ptd->dfile);
	delete ptd;
	return rc;
}
static DWORD WINAPI _BootReadMsgThread(LPVOID arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadMsgThread(ptd->dfile);
	delete ptd;
	return rc;
}
#ifdef IQSMP
static DWORD WINAPI _BootReadSmpThread(LPVOID arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadSmpThread(ptd->dfile);
	delete ptd;
	return rc;
}
#endif
#ifdef AWAY_FIXSERVER
static DWORD WINAPI _BootReadAwayThread(LPVOID arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadAwayThread(ptd->dfile);
	delete ptd;
	return rc;
}
#endif
static DWORD WINAPI _BootReadCsvThread(LPVOID arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadCsvThread(ptd->dfile);
	delete ptd;
	return rc;
}
#else
static void *_BootReadDetailThread(void *arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadDetailThread(ptd->dfile);
	delete ptd;
	return (void*)(PTRCAST)rc;
}
static void *_BootReadMsgThread(void *arg)
{
	BootReadDetailThreadData *ptd=(BootReadDetailThreadData *)arg;
	int rc=ptd->pmod->ReadMsgThread(ptd->dfile);
	delete ptd;
	return (void*)(PTRCAST)rc;
}
#endif
void ViewServer::WSUsrOpened(int PortNo)
{
	WSLogEvent("USR%d connected from %s...",PortNo,UsrPort[PortNo].RemoteIP);
	// For receiving TCP FIX drops
	if((!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLFIXLOGS$",11))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLPUBHUB$",10))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$ITF$",5))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$IQOMS$",7))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$IQFIXS$",8))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TJMLOG$",8))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXDROP$",9))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLDROP$",8))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TBACKUP$",9))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TJM$",5))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLPH_EMEA$",11))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLPH_CC$",9))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_PLACELOG$",15))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_CXLLOG$",13))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_RPTLOG$",13))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_EVELOG$",13))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLS_EVENTLOG$",14))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_PLACELOG$",17))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_CXLLOG$",15))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_RPTLOG$",15))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_EVELOG$",15))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FULLLOG$",9))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$RTOATS$",8))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TWISTLOG$",10))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TWIST$",7))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$OMS_CBOE$",10))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLS_BTR$",9))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$RTAWAY$",8))||
	   (!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$RTECHOATS$",11)))
	{
		if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLFIXLOGS$",11))
			UsrPort[PortNo].DetPtr=(void*)PROTO_MLFIXLOGS;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLPUBHUB$",10))
			UsrPort[PortNo].DetPtr=(void*)PROTO_MLPUBHUB;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$ITF$",5))
			UsrPort[PortNo].DetPtr=(void*)PROTO_ITF;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$IQOMS$",7))
			UsrPort[PortNo].DetPtr=(void*)PROTO_IQOMS;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$IQFIXS$",8))
			UsrPort[PortNo].DetPtr=(void*)PROTO_IQFIXS;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TJMLOG$",8))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TJMLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXDROP$",9))
			UsrPort[PortNo].DetPtr=(void*)PROTO_FIXDROP;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLDROP$",8))
			UsrPort[PortNo].DetPtr=(void*)PROTO_CLDROP;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TBACKUP$",9))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TBACKUP;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TJM$",5))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TJM;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLPH_EMEA$",11))
			UsrPort[PortNo].DetPtr=(void*)PROTO_MLPH_EMEA;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLBACKUP$",10))
			UsrPort[PortNo].DetPtr=(void*)PROTO_CLBACKUP;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$MLPH_CC$",9))
			UsrPort[PortNo].DetPtr=(void*)PROTO_MLPH_CC;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_PLACELOG$",15))
			UsrPort[PortNo].DetPtr=(void*)PROTO_FIXS_PLACELOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_CXLLOG$",13))
			UsrPort[PortNo].DetPtr=(void*)PROTO_FIXS_CXLLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_RPTLOG$",13))
			UsrPort[PortNo].DetPtr=(void*)PROTO_FIXS_RPTLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIXS_EVELOG$",13))
			UsrPort[PortNo].DetPtr=(void*)PROTO_FIXS_EVELOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLS_EVENTLOG$",14))
			UsrPort[PortNo].DetPtr=(void*)PROTO_CLS_EVENTLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_PLACELOG$",17))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TRADER_PLACELOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_CXLLOG$",15))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TRADER_CXLLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_RPTLOG$",15))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TRADER_RPTLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TRADER_EVELOG$",15))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TRADER_EVELOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FULLLOG$",9))
			UsrPort[PortNo].DetPtr=(void*)PROTO_FULLLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$RTOATS$",8))
			UsrPort[PortNo].DetPtr=(void*)PROTO_RTOATS;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TWISTLOG$",10))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TWISTLOG;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$TWIST$",7))
			UsrPort[PortNo].DetPtr=(void*)PROTO_TWIST;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$OMS_CBOE$",10))
			UsrPort[PortNo].DetPtr=(void*)PROTO_OMS_CBOE;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLS_BTR$",9))
			UsrPort[PortNo].DetPtr=(void*)PROTO_CLS_BTR;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$RTAWAY$",8))
			UsrPort[PortNo].DetPtr=(void*)PROTO_RTAWAY;
		else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$RTECHOATS$",11))
			UsrPort[PortNo].DetPtr=(void*)PROTO_RTECHOATS;
		UsrPort[PortNo].DetPtr3=new FixStats;
		// Only allow one USR connection per USC port
		for(int i=0;i<NO_OF_USR_PORTS;i++)
		{
			if(!UsrPort[i].SockActive)
				continue;
			if(i==PortNo)
				continue;
			if(UsrPort[i].UscPort==UsrPort[PortNo].UscPort)
			{
				WSLogError("USR%d: USC%d already has a USR connection.",PortNo,UsrPort[PortNo].UscPort);
				UsrPort[PortNo].TimeTillClose=5;
				UsrPort[PortNo].SockActive=0;
				return;
			}
		}
	}
	// Thin server queries
	else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$VSCLIENT$",10))
	{
		UsrPort[PortNo].DetPtr=(void*)PROTO_VSCLIENT;
//		if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote, "$VSCLIENT$NEWBTR", 14))
		UsrPort[PortNo].DetPtr8 = (void*)1;
		UsrPort[PortNo].DetPtr3=new FixStats;
		VSCodec *vsc=new VSCodec;
		if(vsc->Init(this)<0)
		{
			delete vsc;
			WSLogError("USR%d: Failed to initialize VSCodec!",PortNo);
			WSCloseUsrPort(PortNo);
			return;
		}
		UsrPort[PortNo].DetPtr6=vsc;
		FeedUser *fuser=taskmgr.AddUser(WS_USR,PortNo);
		if(!fuser)
		{
			WSLogError("USR%d: Failed to add task user!",PortNo);
			WSCloseUsrPort(PortNo);
			return;
		}
		fuser->authed=false;
		fuser->waitAuth=true;
		UsrPort[PortNo].DetPtr9=fuser;
	}
	// Distributed journal updates
	else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$VSDIST$",8))
	{
		UsrPort[PortNo].DetPtr=(void*)PROTO_VSDIST;
		UsrPort[PortNo].DetPtr3=new FixStats;
		VSDistCodec *vsd=new VSDistCodec;
		if(vsd->Init(this)<0)
		{
			WSLogError("USR%d: Failed to initialize VSDistCodec!",PortNo);
			WSCloseUsrPort(PortNo);
			return;
		}
		UsrPort[PortNo].DetPtr6=vsd;
	}
	else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$CLBACKUP$",10))
	{
		// Session ids
		UsrPort[PortNo].DetPtr=(void*)PROTO_CLBACKUP;
		UsrPort[PortNo].DetPtr3=new FixStats;
		if(CLBackupOpened(PortNo)<0)
		{
			WSLogError("USR%d: TBackupOpened failed!",PortNo);
			WSCloseUsrPort(PortNo);
			return;
		}
	}
#ifdef FIX_SERVER
	else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$FIX$",5))
	{
		// Session ids
		UsrPort[PortNo].DetPtr=(void*)PROTO_FIX;
		UsrPort[PortNo].DetPtr3=new FixStats;
		int UscPortNo=-1;
		char sess[256]={0};
		strcpy(sess,UscPort[UsrPort[PortNo].UscPort].CfgNote +5);
		bool passive=false;
		char *sptr=strchr(sess,'$');
		if(sptr)
		{
			*sptr=0; UscPortNo=atoi(sptr+1);
		}
		if(strrcmp(sess,",PASSIVE"))
		{
			sess[strlen(sess) -8]=0;
			passive=true;
		}
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		{
			WSLogError("CON%d: Invalid USC portno after $FIX$!",PortNo);
			ConPort[PortNo].ConnectHold=true;
			return;
		}
		DropServer *pds=0;
		DSMAP::iterator dit=dsmap.find(PortNo);
		if(dit==dsmap.end())
		{
			pds=new DropServer;
			if(pds->Init(sess,0,0,passive,this,(void*)(PTRCAST)MAKELONG(PortNo,WS_USR))<0)
			{
				delete pds;
				WSLogError("USR%d: Invalid session ids after $FIXDROP$!",PortNo);
				UsrPort[PortNo].TimeTillClose=5;
				UsrPort[PortNo].SockActive=0;
				return;
			}
			dsmap[PortNo]=pds;
		}
		else
			pds=dit->second;
		UsrPort[PortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;
		UsrPort[PortNo].DetPtr5=pds;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
			pds->GetLastSeqNos(dfile->fpath.c_str());
			int i;
			for(i=0;i<pds->iseq;i++)
			{
				DropServer::INBLOCK gap;
				if(pds->GetGap(gap,i)>0)
					WSLogEvent("Gap detected [%d,%d)",gap.begin,gap.end);
				else
					break;
			}
			if(!i)
				WSLogEvent("No gaps detected");
		}
	}
#endif
	else if(!strncmp(UscPort[UsrPort[PortNo].UscPort].CfgNote,"$EMIT$",6))
	{
		UsrPort[PortNo].DetPtr=(void*)PROTO_EMIT;
	}
	else
		UsrPort[PortNo].DetPtr=(void*)PROTO_UNKNOWN;
}
int ViewServer::WSUsrMsgReady(int PortNo)
{
	int Size=UsrPort[PortNo].InBuffer.Size;
	if(Size<1)
		return 0;
	DWORD& lastReadyStart=(DWORD&)UsrPort[PortNo].lastReadyStart;
	DWORD tlast=lastReadyStart;
	DWORD tnow=GetTickCount();

	lastReadyStart=0;
	if((MAX_USR_TIME>0)&&(tlast)&&((int)(tnow -tlast)>=MAX_USR_TIME))
		return(0);
	lastReadyStart=tlast?tlast:tnow;

	Protocol proto=(Protocol)(PTRCAST)UsrPort[PortNo].DetPtr;
	if((proto==PROTO_MLFIXLOGS)||(proto==PROTO_MLPUBHUB)||(proto==PROTO_ITF)||
	   (proto==PROTO_MLPH_EMEA)||(proto==PROTO_MLPH_CC)||
	   (proto==PROTO_IQOMS)||(proto==PROTO_IQFIXS)||(proto==PROTO_TJMLOG)||
	   (proto==PROTO_FIXDROP)||(proto==PROTO_TBACKUP)||(proto==PROTO_TJM)||(proto==PROTO_FULLLOG)||
	   (proto==PROTO_FIXS_PLACELOG)||(proto==PROTO_FIXS_CXLLOG)||(proto==PROTO_FIXS_RPTLOG)||(proto==PROTO_FIXS_EVELOG)||
	   (proto==PROTO_CLS_EVENTLOG)||
	   (proto==PROTO_TRADER_PLACELOG)||(proto==PROTO_TRADER_CXLLOG)||(proto==PROTO_TRADER_RPTLOG)||(proto==PROTO_TRADER_EVELOG)||
	   (proto==PROTO_RTOATS)||(proto==PROTO_TWISTLOG)||(proto==PROTO_TWIST)||(proto==PROTO_RTECHOATS)||
	   (proto==PROTO_OMS_CBOE)||(proto==PROTO_CLS_BTR))
	{
		Size=UsrPort[PortNo].InBuffer.LocalSize;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UsrPort[PortNo].UscPort].DetPtr1;
		if(dfile)
		{
		#ifdef NO_FIX_WRITES
			// Go straight to journal
			int UscPortNo=dfile->portno;
			FixStats *ustats=(FixStats*)UscPort[UscPortNo].DetPtr3;
			LONGLONG doffset=ustats->totMsgCnt;
			FIXINFO fi;
			fi.noSession=true;
			fi.supressChkErr=true;
			const char *fend=UsrPort[PortNo].InBuffer.Block +UsrPort[PortNo].InBuffer.LocalSize;
			const char *fptr;
			for(fptr=UsrPort[PortNo].InBuffer.Block;fptr<fend;)
			{
				const char *fnext=strendl(fptr,fend);
				if(!fnext)
					break;
				int flen=(int)(fnext -fptr);
				fi.Reset();
				flen=fi.FixMsgReady((char*)fptr,flen);
				if(flen>0)
				{
					WSLockPort(WS_USC,UscPortNo,false);
					int rc=HandleUscFixMsg(&fi,UscPortNo,ustats,doffset,proto);
					WSUnlockPort(WS_USC,UscPortNo,false);
					if(rc==-2) // throttle
						break;
				}
				else
					break;
				for(fptr=fnext;(fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'));fptr++)
					;
			}
			Size=(int)(fptr -UsrPort[PortNo].InBuffer.Block);
			if(Size<1)
				return 0;
		#else
			LONGLONG doff=-1;
			odb.WriteDetailBlock(dfile,UsrPort[PortNo].InBuffer.Block,Size,doff);
		#endif
		}
		UsrPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
		UsrPort[PortNo].PacketsIn++;
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt+=Size; ustats->totMsgCnt+=Size;
		}
		return 1;
	}
	// MSGHEADER protocols
	else if((proto==PROTO_VSCLIENT)||(proto==PROTO_VSDIST)||(proto==PROTO_CLDROP)||(proto==PROTO_CLBACKUP))
	{
		Size=UsrPort[PortNo].InBuffer.LocalSize;
		if(Size<sizeof(MSGHEADER))
			return 0;
		MSGHEADER *MsgHeader=(MSGHEADER*)UsrPort[PortNo].InBuffer.Block;
		if(Size<(int)sizeof(MSGHEADER) +MsgHeader->MsgLen)
			return 0;
		char *Msg=(char *)MsgHeader +sizeof(MSGHEADER);
		UsrPort[PortNo].DetPtr1=(void*)(sizeof(MSGHEADER) +MsgHeader->MsgLen);
		if(TranslateUsrMsg(MsgHeader,Msg,PortNo)<0)
		{
			WSCloseUsrPort(PortNo);
			return -1;
		}
		UsrPort[PortNo].PacketsIn++;
		return 1;
	}
#ifdef FIX_SERVER
	else if(proto==PROTO_FIX)
	{
		DropServer *pds=(DropServer*)UsrPort[PortNo].DetPtr5;
		if(pds)
		{
			Size=pds->DecodeFix(UsrPort[PortNo].InBuffer.Block,Size);
			if(Size<1)
				return 0;
			UsrPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
			return 1;
		}
		return 0;
	}
#endif
	else if(proto==PROTO_EMIT)
	{
		Size=EmitMsgReady(PortNo);
		if(Size>0)
		{
			HandleEmitRequest(PortNo,Size);
			UsrPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
			UsrPort[PortNo].PacketsIn++;
			return 1;
		}
		return 0;
	}
	else
	{
		UsrPort[PortNo].DetPtr1=(void*)(PTRCAST)Size;
		UsrPort[PortNo].PacketsIn++;
		return 1;
	}
}
int ViewServer::WSUsrStripMsg(int PortNo)
{
	int Size=(int)(PTRCAST)UsrPort[PortNo].DetPtr1;
	while(Size>0)
	{
		int slen=Size;
		if(slen>UscPort[UsrPort[PortNo].UscPort].BlockSize*1024)
			slen=UscPort[UsrPort[PortNo].UscPort].BlockSize*1024;
		if(WSStripBuff(&UsrPort[PortNo].InBuffer,slen)<0)
		{
			WSCloseUsrPort(PortNo);
			return -1;
		}
		Size-=slen;
	}
	return 0;
}
int ViewServer::WSBeforeUsrSend(int PortNo)
{
	return 0;
}
void ViewServer::WSUsrClosed(int PortNo)
{
	WSLogEvent("USR%d disconnected from %s.",PortNo,UsrPort[PortNo].RemoteIP);
	Protocol proto=(Protocol)(PTRCAST)UsrPort[PortNo].DetPtr;
	if((proto==PROTO_MLFIXLOGS)||(proto==PROTO_MLPUBHUB)||(proto==PROTO_ITF)||
	   (proto==PROTO_MLPH_EMEA)||(proto==PROTO_MLPH_CC)||
	   (proto==PROTO_IQOMS)||(proto==PROTO_IQFIXS)||(proto==PROTO_TJMLOG)||
	   (proto==PROTO_FIXDROP)||(proto==PROTO_CLDROP)||(proto==PROTO_TJM)||
	   (proto==PROTO_FIXS_PLACELOG)||(proto==PROTO_FIXS_CXLLOG)||(proto==PROTO_FIXS_RPTLOG)||(proto==PROTO_FIXS_EVELOG)||
	   (proto==PROTO_CLS_EVENTLOG)||
	   (proto==PROTO_TRADER_PLACELOG)||(proto==PROTO_TRADER_CXLLOG)||(proto==PROTO_TRADER_RPTLOG)||(proto==PROTO_TRADER_EVELOG)||
	   (proto==PROTO_FULLLOG)||(proto==PROTO_RTOATS)||(proto==PROTO_TWISTLOG)||(proto==PROTO_TWIST)||(proto==PROTO_RTECHOATS)||
	   (proto==PROTO_OMS_CBOE)||(proto==PROTO_FIX)||(proto==PROTO_CLS_BTR))
	{
		//VSOrder *&pendWritesFirst=(VSOrder *&)UsrPort[PortNo].DetPtr4;
		//VSOrder *&pendWritesLast=(VSOrder *&)UsrPort[PortNo].DetPtr5;
		//odb.FreeOrders(pendWritesFirst,pendWritesLast);
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			delete ustats; UsrPort[PortNo].DetPtr3=0;
		}
	}
	else if(proto==PROTO_VSCLIENT)
	{
		//WSLogEvent("USR%d disconnected from %s. (vsclient)",PortNo,UsrPort[PortNo].RemoteIP);
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			delete ustats; UsrPort[PortNo].DetPtr3=0;
		}
		FeedUser *fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
		if(fuser)
		{
			USERENTITLELIST *elist=(USERENTITLELIST *)fuser->DetPtr3;
			if(elist)
			{
				delete elist; fuser->DetPtr3=0;
			}
			taskmgr.RemUser(WS_USR,PortNo);
			UsrPort[PortNo].DetPtr9=0;
		}
		VSQUERYMAP *qmap=(VSQUERYMAP*)UsrPort[PortNo].DetPtr4;
		if(qmap)
		{
			for(VSQUERYMAP::iterator qit=qmap->begin();qit!=qmap->end();qit++)
				delete qit->second;
			delete qmap; UsrPort[PortNo].DetPtr4=0;
		}
		VSCodec *vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
		if(vsc)
		{
			vsc->Shutdown();
			delete vsc; UsrPort[PortNo].DetPtr6=0;
		}
		//WSLogEvent("USR%d disconnected from %s. (vsclient-done)",PortNo,UsrPort[PortNo].RemoteIP);
	}
	else if(proto==PROTO_VSDIST)
	{
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			delete ustats; UsrPort[PortNo].DetPtr3=0;
		}
		VSDistCodec *vsd=(VSDistCodec *)UsrPort[PortNo].DetPtr6;
		if(vsd)
		{
			vsd->Shutdown();
			delete vsd; UsrPort[PortNo].DetPtr6=0;
		}
	}
	else if(proto==PROTO_CLBACKUP)
	{
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			delete ustats; UsrPort[PortNo].DetPtr3=0;
		}
		CLBackupClosed(PortNo);
	}
	else if(proto==PROTO_EMIT)
	{
		EmitPortClosed(PortNo);
	}
	//WSLogEvent("USR%d disconnected from %s. (done)",PortNo,UsrPort[PortNo].RemoteIP);
}

void ViewServer::WSUgrOpened(int PortNo)
{
	WSLogEvent("UGR%d connected from %s...",PortNo,UgrPort[PortNo].RemoteIP);
	// For receiving TCP FIX drops
	if(!strncmp(UgcPort[UgrPort[PortNo].UgcPort].CfgNote,"$IQFIXS$",8))
	{
		UgrPort[PortNo].DetPtr=(void*)PROTO_IQFIXS;
		int UscPortNo=atoi(UgcPort[UgrPort[PortNo].UgcPort].CfgNote +8);
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		{
			WSLogError("UGC%d: Invalid USC portno after $IQFIXS$!",UgrPort[PortNo].UgcPort);
			UgcPort[UgrPort[PortNo].UgcPort].ConnectHold=true;
			return;
		}
		UgrPort[PortNo].DetPtr4=(void*)(LONGLONG)UscPortNo;
		UgrPort[PortNo].DetPtr3=new FixStats;
		// GD ports already only allow one UGR connection per UGC line
	}
#ifdef AWAY_FIXSERVER
	// For receiving TCP FIX drops
	else if(!strncmp(UgcPort[UgrPort[PortNo].UgcPort].CfgNote,"$CLS_BTR$",9))
	{
		UgrPort[PortNo].DetPtr=(void*)PROTO_CLS_BTR;
		int UscPortNo=atoi(UgcPort[UgrPort[PortNo].UgcPort].CfgNote +9);
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		{
			WSLogError("UGC%d: Invalid USC portno after $IQFIXS$!",UgrPort[PortNo].UgcPort);
			UgcPort[UgrPort[PortNo].UgcPort].ConnectHold=true;
			return;
		}
		UgrPort[PortNo].DetPtr4=(void*)(LONGLONG)UscPortNo;
		UgrPort[PortNo].DetPtr3=new FixStats;
		// GD ports already only allow one UGR connection per UGC line
	}
#endif
}
int ViewServer::WSTranslateUgrMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo, WORD LineId)
{
	Protocol proto=(Protocol)(LONGLONG)UgrPort[PortNo].DetPtr;
	if(proto==PROTO_IQFIXS)
	{
		int Size=MsgHeader->MsgLen;
		int UscPortNo=(int)(LONGLONG)UgrPort[PortNo].DetPtr4;
		if((UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS))
		{
			VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
			if(dfile)
			{
			#ifndef NO_FIX_WRITES
				LONGLONG doff=-1;
				odb.WriteDetailBlock(dfile,Msg,Size,doff);
			#endif
			}
		}
		//UgrPort[PortNo].DetPtr1=(void*)(LONGLONG)(sizeof(MSGHEADER) +Size);
		//UgrPort[PortNo].PacketsIn++;
		FixStats *ustats=(FixStats *)UgrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt+=Size; ustats->totMsgCnt+=Size;
		}

		if(MsgHeader->MsgID==16)
			UgrPort[PortNo].BeatsIn++;
		return 1;
	}
#ifdef AWAY_FIXSERVER
	else if(proto==PROTO_CLS_BTR)
	{
		return TranslateAwayMsg(MsgHeader,Msg,PortNo);
	}
#endif
	return 0;
}
void ViewServer::WSUgrClosed(int PortNo)
{
	WSLogEvent("UGR%d disconnected from %s.",PortNo,UgrPort[PortNo].RemoteIP);
	Protocol proto=(Protocol)(LONGLONG)UsrPort[PortNo].DetPtr;
	if(proto==PROTO_IQFIXS)
	{
		FixStats *ustats=(FixStats *)UgrPort[PortNo].DetPtr3;
		if(ustats)
		{
			delete ustats; UgrPort[PortNo].DetPtr3=0;
		}
	}
#ifdef AWAY_FIXSERVER
	else if(proto==PROTO_CLS_BTR)
	{
		FixStats *ustats=(FixStats *)UgrPort[PortNo].DetPtr3;
		if(ustats)
		{
			delete ustats; UgrPort[PortNo].DetPtr3=0;
		}
	}
#endif
}

int ViewServer::TranslateConMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	switch(MsgHeader->MsgID)
	{
	case 1: // $VSCLIENT$
	{
		VSCodec *vsc=(VSCodec *)ConPort[PortNo].DetPtr6;
		if(vsc)
			vsc->DecodeReply(Msg,MsgHeader->MsgLen,(void*)(LONGLONG)MAKELONG(PortNo,WS_CON));
		break;
	}
	case 16: // Heartbeat
		ConPort[PortNo].BeatsIn++;
		break;
	case 100: // VSDist feed
	{
		VSDistCodec *vsd=(VSDistCodec *)ConPort[PortNo].DetPtr6;
		if(vsd)
			vsd->DecodeReply(Msg,MsgHeader->MsgLen,(void*)(PTRCAST)MAKELONG(PortNo,WS_CON)); // will call NotifyXXXReply
		break;
	}
	// TBACKUP replication messages
	case 472: // VersionRequest
	case 474: // FileTransferRequest
	case 475: // NewExeReady
	case 477: // FileTransferDone
	case 478: // FileTransferDoneAck
	case 479: // GenerateErrorEventLogsRequest
	case 495: // InitBackup
	case 500: // FileNotification
	case 950: // LiveBackupMessage
	case 822: // ReceiveTextFile
	case 1822: // ReceiveTextFile
	{
		HandleConTbackupMsg(MsgHeader,Msg,PortNo);
		break;
	}
#ifdef IQDNS
	case 21: //reqADDSERVER: // New iqclient
	case 24: // Iqclient logout elsewhere
	case 632: // AccountRec
	case 932: // VARSID message from web
	case 960: // UserEntitlement
	case 1960: // New user entitlement
	case 964: // User account entitlement
	case 969: // Domain Tree Replicated with Logic Parent //REPLACED WITH 972
	case 972: // Domain Tree Replicated with Logic Parent and OmsServerName
	case 970: // User document
	case 971: // Domain document
	case 981: // Domain query
	case 997: // OmsServer IP Notification
	case 998: // Clserver login
	case 999: // Remove user entitlements
	case 1102: // Iqclient load report
	case 1957: // Password expiration
	case 6314: // Twist domains, accounts, and users
	{
		HandleConDnsReplMsg(MsgHeader,Msg,PortNo);
		break;
	}
#endif
#ifdef REDIOLEDBCON
	// Nexus server exchange info
	case 346:
	{
		if(MsgHeader->MsgLen<sizeof(MSG346REC))
			break;
		MSG346REC Msg346Rec;
		memcpy(&Msg346Rec,Msg,sizeof(MSG346REC));
		IQMarketCenter mc=(IQMarketCenter)Msg346Rec.chIQMC;
		char exch=0,reg=0;
		IQCspApi::MarketCode(mc,exch,reg);
		WORD exchreg=MAKEWORD(reg,exch);
		// Ignore internationals for now
		if((exch!=0)&&(exch!=21))
		{
			symExchMap[Msg346Rec.szSymbol]=exchreg;
			if(Msg346Rec.szISIN[0])
				symIsinMap[Msg346Rec.szSymbol]=Msg346Rec.szISIN;
		}

		if(Msg346Rec.bEnd)
			WSLogEvent("CON%d: Completed Nexus symbol details for %s.",PortNo,
				Msg346Rec.chAssetType==(char)IQAST_EQUITY?"equities":"futures");

		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			cstats->msgCnt++; cstats->totMsgCnt++;
		}
		break;
	}
	case 910:
		break;
#endif
	};
	return 0;
}
int ViewServer::TranslateUsrMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	switch(MsgHeader->MsgID)
	{
	case 16: // Heartbeat
		UsrPort[PortNo].BeatsIn++;
		break;
	case 1: // 'VSCodec' request
	{
		VSCodec *vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
		if(vsc)
		{
			vsc->DecodeRequest(Msg,MsgHeader->MsgLen,(void*)(PTRCAST)MAKELONG(PortNo,WS_USR)); // will call NotifyXXXRequest
		}
		break;
	}
	case 100: // 'VSDistCodec' request
	{
		VSDistCodec *vsd=(VSDistCodec *)UsrPort[PortNo].DetPtr6;
		if(vsd)
		{
			if(vsd->DecodeRequest(Msg,MsgHeader->MsgLen,(void*)(PTRCAST)MAKELONG(PortNo,WS_USR))<0) // will call NotifyXXXRequest
				WSLogError("USR%d: DecodeRequest(100) failed on %d bytes!",PortNo,MsgHeader->MsgLen);
		}
		break;
	}
	case 752: // CLSERVER drop messages
	case 756:
	case 758:
	case 759:
	{
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UsrPort[PortNo].UscPort].DetPtr1;
		if(dfile)
		{
			LONGLONG doff=-1;
			odb.WriteDetailBlock(dfile,Msg -sizeof(MSGHEADER),sizeof(MSGHEADER) +MsgHeader->MsgLen,doff);
			break;
		}
	}
	// CLBACKUP replication messages
	case 707:
	case 719:
//	case 822:
	case 826:
	case 950:
	case 951:
	{
		HandleUsrCLbackupMsg(MsgHeader,Msg,PortNo);
		break;
	}
	case 101: // RpxFakeAPI sql query
	case 103:
	{
		HandleUsrRpxFakeAPIMsg(MsgHeader,Msg,PortNo);
		break;
	}
	case 5000: // Login from IQATS
	{
		MSG5000REC Msg5000Rec;
		memcpy(&Msg5000Rec,Msg,sizeof(MSG5000REC));
		MSG5001REC Msg5001Rec;
		memset(&Msg5001Rec,0,sizeof(MSG5001REC));
		strcpy(Msg5001Rec.ServiceName,Msg5000Rec.ServiceName);
		strcpy(Msg5001Rec.Domain,Msg5000Rec.Domain);
		strcpy(Msg5001Rec.User,Msg5000Rec.User);
		strcpy(Msg5001Rec.Account,Msg5000Rec.Account);
		Msg5001Rec.ProductType=Msg5000Rec.ProductType;
		Msg5001Rec.LineID=Msg5000Rec.LineID;
		Msg5001Rec.DocSignVersion=Msg5000Rec.DocSignVersion;
		strcpy(Msg5001Rec.ClientIP,Msg5000Rec.ClientIP);
		Msg5001Rec.ClientPort=Msg5000Rec.ClientPort;
		if(!strcmp(Msg5000Rec.ServiceName,"VSDB"))
			Msg5001Rec.LoginOk=1;
		else
			strcpy(Msg5001Rec.RejectReason,"Invalid ServiceName");
		WSUsrSendMsg(5001,sizeof(MSG5001REC),(char*)&Msg5001Rec,PortNo);
		break;
	}
	};
	return 0;
}

int ViewServer::OpenDetailFiles()
{
	list<VSDetailFile *> dlist;
	int i;
	if(VSDIST_HUB)
	{
	#ifdef MULTI_DAY_HIST
		if(odb.HISTORIC_DAYS>0)
		{
			int ndays=odb.HISTORIC_DAYS;
			if (importHistoricFiles)
			{
				int sdate = CalcTPlusDate(odb.wsdate, 0);
				if (!odb.OpenExtFile(sdate, HISTORIC_ONDEMAND))
					odb.CloseExtFile();
				importHistoricFiles = false;
			}
			else
			{
				for (int d = 1; (ndays > 0) && (d <= odb.HISTORIC_DAYS); d++)
				{
					int sdate = CalcTPlusDate(odb.wsdate, -d);
					if (sdate < HISTORIC_BEGINDATE)
						break;
					if (!odb.OpenExtFile(sdate, HISTORIC_ONDEMAND))
						odb.CloseExtFile();
				}
			}
		}
		else
		{
			if(odb.OpenExtFile(odb.wsdate,false)<0)
				return -1;
		}
	#else
		if(odb.OpenExtFile()<0)
			return -1;
	#endif
	}
	for(i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(!UscPort[i].InUse)
			continue;
		int proto=PROTO_UNKNOWN;
		char *prefix=0;
		// FIX drops
		if((!strncmp(UscPort[i].CfgNote,"$MLFIXLOGS$",11))||
		   (!strncmp(UscPort[i].CfgNote,"$MLPUBHUB$",10))||
		   (!strncmp(UscPort[i].CfgNote,"$ITF$",5))||
		   (!strncmp(UscPort[i].CfgNote,"$IQOMS$",7))||
		   (!strncmp(UscPort[i].CfgNote,"$IQFIXS$",8))||
		   (!strncmp(UscPort[i].CfgNote,"$TJMLOG$",8))||
		   (!strncmp(UscPort[i].CfgNote,"$FIXDROP$",9))||
		   (!strncmp(UscPort[i].CfgNote,"$CLDROP$",8))||
		   (!strncmp(UscPort[i].CfgNote,"$TBACKUP$",9))||
		   (!strncmp(UscPort[i].CfgNote,"$TJM$",5))||
		   (!strncmp(UscPort[i].CfgNote,"$MLPH_EMEA$",11))||
		   (!strncmp(UscPort[i].CfgNote,"$CLBACKUP$",10))||
		   (!strncmp(UscPort[i].CfgNote,"$MLPH_CC$",9))||
		   (!strncmp(UscPort[i].CfgNote,"$FIXS_PLACELOG$",15))||
		   (!strncmp(UscPort[i].CfgNote,"$FIXS_CXLLOG$",13))||
		   (!strncmp(UscPort[i].CfgNote,"$FIXS_RPTLOG$",13))||
		   (!strncmp(UscPort[i].CfgNote,"$FIXS_EVELOG$",13))||
		   (!strncmp(UscPort[i].CfgNote,"$CLS_EVENTLOG$",14))||
		   (!strncmp(UscPort[i].CfgNote,"$TRADER_PLACELOG$",17))||
		   (!strncmp(UscPort[i].CfgNote,"$TRADER_CXLLOG$",15))||
		   (!strncmp(UscPort[i].CfgNote,"$TRADER_RPTLOG$",15))||
		   (!strncmp(UscPort[i].CfgNote,"$TRADER_EVELOG$",15))||
		   (!strncmp(UscPort[i].CfgNote,"$FULLLOG$",9))||
		   (!strncmp(UscPort[i].CfgNote,"$RTOATS$",8))||
		   (!strncmp(UscPort[i].CfgNote,"$TWISTLOG$",10))||
		   (!strncmp(UscPort[i].CfgNote,"$TWIST$",7))||
		   (!strncmp(UscPort[i].CfgNote,"$OMS_CBOE$",10))||
		   (!strncmp(UscPort[i].CfgNote,"$CLS_BTR$",9))||
		   (!strncmp(UscPort[i].CfgNote,"$RTPOSITION$",12))||
		   (!strncmp(UscPort[i].CfgNote,"$RTAUDIT$",9))||
		   (!strncmp(UscPort[i].CfgNote,"$RTAWAY$",8))||
		   (!strncmp(UscPort[i].CfgNote,"$RTECHOATS$",11)))
		{
			if(VSDIST_HUB)
			{
				WSLogError("USC%d: VSDIST_HUB must be set to 0 for USC detail drop ports! Disabling port.",i);
				UscPort[i].InUse=0;
				continue;
			}
			if(!strncmp(UscPort[i].CfgNote,"$MLFIXLOGS$",11))
			{
				proto=PROTO_MLFIXLOGS;
				prefix=UscPort[i].CfgNote +11;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$MLPUBHUB$",10))
			{
				proto=PROTO_MLPUBHUB;
				prefix=UscPort[i].CfgNote +10;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$ITF$",5))
			{
				proto=PROTO_ITF;
				prefix=UscPort[i].CfgNote +5;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$IQOMS$",7))
			{
				proto=PROTO_IQOMS;
				prefix=UscPort[i].CfgNote +7;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$IQFIXS$",8))
			{
				proto=PROTO_IQFIXS;
				prefix=UscPort[i].CfgNote +8;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TJMLOG$",8))
			{
				proto=PROTO_TJMLOG;
				prefix=UscPort[i].CfgNote +8;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$FIXDROP$",9))
			{
				proto=PROTO_FIXDROP;
				prefix=UscPort[i].CfgNote +9;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$CLDROP$",8))
			{
				proto=PROTO_CLDROP;
				prefix=UscPort[i].CfgNote +8;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TBACKUP$",9))
			{
				proto=PROTO_TBACKUP;
				prefix=UscPort[i].CfgNote +9;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TJM$",5))
			{
				proto=PROTO_TJM;
				prefix=UscPort[i].CfgNote +5;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$MLPH_EMEA$",11))
			{
				proto=PROTO_MLPH_EMEA;
				prefix=UscPort[i].CfgNote +11;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$CLBACKUP$",10))
			{
				proto=PROTO_CLBACKUP;
				prefix=UscPort[i].CfgNote +10;
				CLBackupInit(i,prefix);
			}
			else if(!strncmp(UscPort[i].CfgNote,"$MLPH_CC$",9))
			{
				proto=PROTO_MLPH_CC;
				prefix=UscPort[i].CfgNote +9;
			}
		#ifdef IQSMP
			else if(!strncmp(UscPort[i].CfgNote,"$FIXS_PLACELOG$",15))
			{
				proto=PROTO_FIXS_PLACELOG;
				prefix=UscPort[i].CfgNote +15;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$FIXS_RPTLOG$",13))
			{
				proto=PROTO_FIXS_RPTLOG;
				prefix=UscPort[i].CfgNote +13;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$FIXS_CXLLOG$",13))
			{
				proto=PROTO_FIXS_CXLLOG;
				prefix=UscPort[i].CfgNote +13;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$FIXS_EVELOG$",13))
			{
				proto=PROTO_FIXS_EVELOG;
				prefix=UscPort[i].CfgNote +13;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$CLS_EVENTLOG$",14))
			{
				proto=PROTO_CLS_EVENTLOG;
				prefix=UscPort[i].CfgNote +14;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TRADER_PLACELOG$",17))
			{
				proto=PROTO_TRADER_PLACELOG;
				prefix=UscPort[i].CfgNote +17;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TRADER_CXLLOG$",15))
			{
				proto=PROTO_TRADER_CXLLOG;
				prefix=UscPort[i].CfgNote +15;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TRADER_RPTLOG$",15))
			{
				proto=PROTO_TRADER_RPTLOG;
				prefix=UscPort[i].CfgNote +15;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TRADER_EVELOG$",15))
			{
				proto=PROTO_TRADER_EVELOG;
				prefix=UscPort[i].CfgNote +15;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$FULLLOG$",9))
			{
				proto=PROTO_FULLLOG;
				prefix=UscPort[i].CfgNote +9;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$RTOATS$",8))
			{
				proto=PROTO_RTOATS;
				prefix=UscPort[i].CfgNote +8;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TWISTLOG$",10))
			{
				proto=PROTO_TWISTLOG;
				prefix=UscPort[i].CfgNote +10;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$TWIST$",7))
			{
				proto=PROTO_TWIST;
				prefix=UscPort[i].CfgNote +7;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$OMS_CBOE$",10))
			{
				proto=PROTO_OMS_CBOE;
				prefix=UscPort[i].CfgNote +10;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$RTECHOATS$",11))
			{
				proto=PROTO_RTECHOATS;
				prefix=UscPort[i].CfgNote +11;
			}
		#endif
		#ifdef AWAY_FIXSERVER
			else if(!strncmp(UscPort[i].CfgNote,"$CLS_BTR$",9))
			{
				proto=PROTO_CLS_BTR;
				prefix=UscPort[i].CfgNote +9;
				// CON port number that aggregates the drop
				char *cptr=strchr(prefix,'$');
				if(cptr)
				{
					*cptr=0; UscPort[i].DetPtr4=(void*)atoi(cptr +1);
				}
			}
		#endif
			else if(!strncmp(UscPort[i].CfgNote,"$RTPOSITION$",12))
			{
				proto=PROTO_CSV;
				prefix=UscPort[i].CfgNote +12;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$RTAUDIT$",9))
			{
				proto=PROTO_CSV;
				prefix=UscPort[i].CfgNote +9;
			}
			else if(!strncmp(UscPort[i].CfgNote,"$RTAWAY$",8))
			{
				proto=PROTO_RTAWAY;
				prefix=UscPort[i].CfgNote +8;
				// CON port number that aggregates the drop
				char *cptr=strchr(prefix,'$');
				if(cptr)
				{
					*cptr=0; UscPort[i].DetPtr4=(void*)atoi(cptr +1);
				}
			}
			if(!*prefix)
			{
				WSLogError("USC%d: CfgNote has blank detail file prefix! Disabling port.",i);
				UscPort[i].InUse=0;
				continue;
			}
			char dpath[MAX_PATH]={0},*dptr=0;
			if(prefix[1]==';')
				prefix[1]=':';
			GetFullPathNameEx(prefix,sizeof(dpath),dpath,&dptr);
			if(dptr>dpath) 
			{
				prefix=dptr; dptr[-1]=0;
			}
			else if(dptr==dpath)
				strcpy(dpath,pcfg->RunPath.c_str());
			else
			{
				WSLogError("USC%d: Invalid detail file path(%s)! Disabling port.",i,prefix);
				UscPort[i].InUse=0;
				continue;
			}

			if(strlen(prefix)>15)
			{
				WSLogError("USC%d: File prefix may not exceed 15 characters! Disabling port.",i);
				UscPort[i].InUse=0;
				continue;
			}
		#ifdef MULTI_DAY_HIST
			VSDetailFile *dfile=odb.OpenDetailFile(prefix,i,proto,dpath,false,odb.wsdate);
		#else
			VSDetailFile *dfile=odb.OpenDetailFile(prefix,i,proto,dpath,false);
		#endif
			if(!dfile)
			{
				WSLogError("USC%d: Failed opening detail file(%s)! Disabling port.",i,prefix);
				UscPort[i].InUse=0;
				continue;
			}
			dlist.push_back(dfile);
			//VSMruCache *pmru=new VSMruCache;
			//WaitForSingleObject(mrumux,INFINITE);
			//mrulist.push_back(pmru);
			//ReleaseMutex(mrumux);

			UscPort[i].DetPtr=(void*)(PTRCAST)proto;
			UscPort[i].DetPtr1=dfile;
			UscPort[i].DetPtr3=new FixStats;
			//UscPort[i].DetPtr4=pmru;
			VSDistJournal *dj=new VSDistJournal;
			if(dj->Init(prefix)<0)
			{
				WSLogError("USC%d: Failed VSDistJournal::Init! Disabling port.",i);
				UscPort[i].InUse=0;
				continue;
			}

			UscPort[i].DetPtr5=dj;
		}
		else if(!strncmp(UscPort[i].CfgNote,"$FIX$",5))
		{
			UscPort[i].DetPtr=(void*)PROTO_FIX;
			UscPort[i].DetPtr3=new FixStats;
		}
		// Distributed journal connections
		else if(!strncmp(UscPort[i].CfgNote,"$VSDIST$",8))
		{
			if(!VSDIST_HUB)
			{
				WSLogError("USC%d: VSDIST_HUB must be set to 1 for USC$VSDIST$ ports! Disabling port.",i);
				UscPort[i].InUse=0;
				continue;
			}
			VSMruCache *pmru=0;
			if(!odb.HAVE_FUSION_IO)
			{
				pmru=new VSMruCache;
				WaitForSingleObject(mrumux,INFINITE);
				mrulist.push_back(pmru);
				ReleaseMutex(mrumux);
			}

			const char *prefix=UscPort[i].CfgNote +8;
		#ifdef MULTI_DAY_HIST
			VSDetailFile *dfile=odb.GetDetailFile(prefix,odb.wsdate);
		#else
			VSDetailFile *dfile=odb.GetDetailFile(prefix);
		#endif
			if(dfile)
			{
				WSLogEvent("USC%d: Detail file(%s) opened...",i,dfile->fpath.c_str());
				UscPort[i].DetPtr1=dfile;
			}
			else
				WSLogEvent("USC%d: Detail file(%s) not found.",i,prefix);

			UscPort[i].DetPtr=(void*)PROTO_VSDIST;
			UscPort[i].DetPtr3=new FixStats;
			if(!odb.HAVE_FUSION_IO)
				UscPort[i].DetPtr4=pmru;
			continue;
		}
		else
			continue;
	}
	if(!VSDIST_HUB)
	{
		if(odb.OpenZmapFile()<0)
			return -1;
		if(IQFLOW)
		{
			pm=new map<string,string>;
			fm=new map<string,string>;
			am=new map<string,string>;
		#ifdef HOLD_IQ_CHILD_ORDERS
			hm=new multimap<string,HeldDetail *>;
		#endif
		}
		else
		{
			fm=new map<string,string>;
			#ifdef GS_RTECHOATS
			em=new map<string,string>;
			cm=new map<string,string>;
			rm=new map<string,string>;
			km=new map<string,string>;
			#endif
		}
	}

	#ifndef NO_FIX_WRITES
	// Don't start read threads until we've read the zmap to tell us where we left off
	for(list<VSDetailFile *>::iterator dit=dlist.begin();dit!=dlist.end();dit++)
	{
		VSDetailFile *dfile=*dit;
		BootReadDetailThreadData *ptd=new BootReadDetailThreadData;
		ptd->pmod=this;
		ptd->dfile=dfile;
		DWORD tid=0;
	#ifdef WIN32
		if((dfile->proto==PROTO_CLDROP)||(dfile->proto==PROTO_CLBACKUP))
			dfile->rthnd=CreateThread(0,0,_BootReadMsgThread,ptd,0,&tid);
		#ifdef IQSMP
		else if((dfile->proto==PROTO_FIXS_PLACELOG)||(dfile->proto==PROTO_FIXS_CXLLOG)||(dfile->proto==PROTO_FIXS_RPTLOG)||(dfile->proto==PROTO_FIXS_EVELOG)||
				(dfile->proto==PROTO_CLS_EVENTLOG)||
				(dfile->proto==PROTO_TRADER_PLACELOG)||(dfile->proto==PROTO_TRADER_CXLLOG)||(dfile->proto==PROTO_TRADER_RPTLOG)||(dfile->proto==PROTO_TRADER_EVELOG)||
				(dfile->proto==PROTO_RTOATS))
			dfile->rthnd=CreateThread(0,0,_BootReadSmpThread,ptd,0,&tid);
		#endif
		#ifdef AWAY_FIXSERVER
		else if(dfile->proto==PROTO_CLS_BTR)
			dfile->rthnd=CreateThread(0,0,_BootReadAwayThread,ptd,0,&tid);
		#endif
		else if(dfile->proto==PROTO_CSV)
			dfile->rthnd=CreateThread(0,0,_BootReadCsvThread,ptd,0,&tid);
		else
			dfile->rthnd=CreateThread(0,0,_BootReadDetailThread,ptd,0,&tid);
	#else
		if((dfile->proto==PROTO_CLDROP)||(dfile->proto==PROTO_CLBACKUP))
			pthread_create(&dfile->rthnd,0,_BootReadMsgThread,ptd);
		else
			pthread_create(&dfile->rthnd,0,_BootReadDetailThread,ptd);
	#endif
		if(!dfile->rthnd)
		{
			delete ptd;
			WSLogError("USC%d: Failed starting ReadDetailThread for file(%s)! Disabling port.",i,dfile->prefix.c_str());
			UscPort[i].InUse=0;
			continue;
		}
	}
	dlist.clear();
	#endif
	return 0;
}
int ViewServer::CloseDetailFiles()
{
	dbLoaded=false;
	if(VSDIST_HUB)
		odb.CloseExtFile();
	else
		odb.CloseZmapFile();
	for(int i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(!UscPort[i].InUse)
			continue;
		if((UscPort[i].DetPtr==(void*)PROTO_MLFIXLOGS)||
		   (UscPort[i].DetPtr==(void*)PROTO_MLPUBHUB)||
		   (UscPort[i].DetPtr==(void*)PROTO_ITF)||
		   (UscPort[i].DetPtr==(void*)PROTO_IQOMS)||
		   (UscPort[i].DetPtr==(void*)PROTO_IQFIXS)||
		   (UscPort[i].DetPtr==(void*)PROTO_TJMLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXDROP)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLDROP)||
		   (UscPort[i].DetPtr==(void*)PROTO_TBACKUP)||
		   (UscPort[i].DetPtr==(void*)PROTO_TJM)||
		   (UscPort[i].DetPtr==(void*)PROTO_MLPH_EMEA)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLBACKUP)||
		   (UscPort[i].DetPtr==(void*)PROTO_MLPH_CC)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_PLACELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_CXLLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_RPTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIXS_EVELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLS_EVENTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_PLACELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_CXLLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_RPTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TRADER_EVELOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_RTOATS)||
		   (UscPort[i].DetPtr==(void*)PROTO_TWISTLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_TWIST)||
		   (UscPort[i].DetPtr==(void*)PROTO_OMS_CBOE)||
		   (UscPort[i].DetPtr==(void*)PROTO_FULLLOG)||
		   (UscPort[i].DetPtr==(void*)PROTO_FIX)||
		   (UscPort[i].DetPtr==(void*)PROTO_CLS_BTR)||
		   (UscPort[i].DetPtr==(void*)PROTO_RTECHOATS))
		{
			VSDetailFile *dfile=(VSDetailFile *)UscPort[i].DetPtr1;
			if(dfile)
			{
				odb.CloseDetailFile(dfile);
				UscPort[i].DetPtr1=0;
			}
		#ifdef IQSMP
			if(UscPort[i].DetPtr2)
			{
				delete (char*)UscPort[i].DetPtr2; UscPort[i].DetPtr2=0;
			}
		#endif
			FixStats *ustats=(FixStats*)UscPort[i].DetPtr3;
			if(ustats)
			{
				delete ustats; UscPort[i].DetPtr3=0;
			}
			//VSMruCache *pmru=(VSMruCache*)UscPort[i].DetPtr4;
			//if(pmru)
			//{
			//	WaitForSingleObject(mrumux,INFINITE);
			//	for(list<VSMruCache*>::iterator mit=mrulist.begin();mit!=mrulist.end();mit++)
			//	{
			//		if(*mit==pmru)
			//		{
			//			mit=mrulist.erase(mit);
			//			break;
			//		}
			//	}
			//	ReleaseMutex(mrumux);
			//	delete pmru; UscPort[i].DetPtr4=0;
			//}
			VSDistJournal *dj=(VSDistJournal *)UscPort[i].DetPtr5;
			if(dj)
			{
				dj->Shutdown();
				delete dj; UscPort[i].DetPtr5=0;
			}
			if(UscPort[i].DetPtr==(void*)PROTO_CLBACKUP)
				CLBackupShutdown(i);
		#ifdef IQSMP
			else if((UscPort[i].DetPtr==(void*)PROTO_FIXS_PLACELOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_FIXS_CXLLOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_FIXS_RPTLOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_FIXS_EVELOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_CLS_EVENTLOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_TRADER_PLACELOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_TRADER_CXLLOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_TRADER_RPTLOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_TRADER_EVELOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_RTOATS)||
				   (UscPort[i].DetPtr==(void*)PROTO_TWISTLOG)||
				   (UscPort[i].DetPtr==(void*)PROTO_TWIST)||
				   (UscPort[i].DetPtr==(void*)PROTO_OMS_CBOE)||
				   (UscPort[i].DetPtr==(void*)PROTO_RTECHOATS))
				SmpShutdown(i);
		#endif
		}
		else if(UscPort[i].DetPtr==(void*)PROTO_VSDIST)
		{
			VSDetailFile *dfile=(VSDetailFile *)UscPort[i].DetPtr1;
			if(dfile)
			{
				odb.CloseDetailFile(dfile);
				UscPort[i].DetPtr1=0;
			}
			FixStats *ustats=(FixStats*)UscPort[i].DetPtr3;
			if(ustats)
			{
				delete ustats; UscPort[i].DetPtr3=0;
			}
		#ifndef TASK_THREAD_POOL
			if(!odb.HAVE_FUSION_IO)
			{
				VSMruCache *pmru=(VSMruCache*)UscPort[i].DetPtr4;
				if(pmru)
				{
					WaitForSingleObject(mrumux,INFINITE);
					for(list<VSMruCache*>::iterator mit=mrulist.begin();mit!=mrulist.end();mit++)
					{
						if(*mit==pmru)
						{
							#ifdef WIN32					
							mit=mrulist.erase(mit);
							#else
							mrulist.erase(mit++);
							#endif
							break;
						}
					}
					ReleaseMutex(mrumux);
					delete pmru; UscPort[i].DetPtr4=0;
				}
			}
		#endif
		}
	}

	// PROTO_IQOMS, PROTO_IQFIXS
	if(!VSDIST_HUB)
	{
		if(IQFLOW)
		{
			if(pm)
			{
				delete pm; pm=0;
			}
			if(fm)
			{
				delete fm; fm=0;
			}
			if(am)
			{
				delete am; am=0;
			}
		#ifdef HOLD_IQ_CHILD_ORDERS
			if(hm)
			{
				for(multimap<string,HeldDetail *>::iterator hit=hm->begin();hit!=hm->end();hit++)
					delete hit->second;
				hm->clear();
				delete hm; hm=0;
			}
		#endif
		}
		else
		{
			if(fm)
			{
				delete fm; fm=0;
			}
			#ifdef GS_RTECHOATS
			if(em)
			{
				delete em; em=0;
			}
			if(cm)
			{
				delete cm; cm=0;
			}
			if(rm)
			{
				delete rm; rm=0;
			}
			if(km)
			{
				delete km; km=0;
			}
			#endif
		}
	}
	return 0;
}
int ViewServer::PubHubFixSubstr(const char *&fptr, int& flen)
{
	const char *fendl=fptr +flen;
	static const char *mtypes[]={
		"NewOrderSingle",
		"OrderAccepted",
		"OrderRejected",
		"Fill",
		"PartialFill",
		"ReplaceRequest",
		"PendingReplace",
		"OrderReplaced",
		"CancelRequest",
		"PendingCancel",
		"OrderCanceled",
		"DoneForDay",
		"CancelReject",
		0
	};
	const char *iptr=strstr(fptr,"::");
	if(iptr) iptr+=2;
	else iptr=fptr;
	while(iptr<fendl)
	{
		for(;(*iptr)&&(isspace(*iptr));iptr++)
			;
		const char *bptr=strechr(iptr,'[',fendl);
		if(!bptr)
			break;
		bptr++;
		const char *eptr=strechr(bptr,']',fendl);
		if(!eptr) 
			eptr=fendl;
		for(int m=0;mtypes[m];m++)
		{
			if(!strincmp(iptr,mtypes[m],(int)strlen(mtypes[m])))
			{
				fptr=bptr; flen=(int)(eptr -bptr);
				return 0;
			}
		}
		iptr=eptr+1;
		if(iptr<fendl)
		{
			if(!strncmp(iptr," -- ",4)) iptr+=4;
			else if(*iptr==':') iptr++;
		}
	}
	return -1;
}
#ifdef WIN32
#ifdef NO_OVL_READ_DETAILS
// Windows no overlapped I/O
int ViewServer::ReadDetailThread(VSDetailFile *dfile)
{
#define READBLOCKSIZE (512*1024) // 512K tried 256K,1M,2M,4M; this is the best value
	char *rbuf=new char[READBLOCKSIZE],*rptr=rbuf;
	dfile->LockRead();
	FixStats *ustats=(FixStats *)UscPort[dfile->portno].DetPtr3;
	if(!ustats)
	{
		_ASSERT(false);
		dfile->UnlockRead();
		return -1;
	}
	while(dfile->rhnd!=INVALID_FILE_VALUE)
	{
		dfile->UnlockRead();
		// This is the only function to modify 'dfile->rend.QuadPart' so we don't need to lock read.
		// Mutex write against VSOrder::WriteDetailBlock.
		dfile->LockWrite();
		if(dfile->rend.QuadPart>=dfile->fend.QuadPart)
		{
			dfile->UnlockWrite();
			SleepEx(100,true);
			dfile->LockRead();
			continue;
		}
		// Only reference the end of write value while we have it locked
		LONGLONG ravail=dfile->fend.QuadPart -dfile->rend.QuadPart;
		LONGLONG rllen=ravail;
		int rused=(int)(rptr -rbuf);
		if(rllen>(READBLOCKSIZE -rused))
			rllen=(READBLOCKSIZE -rused);
		DWORD rlen=(DWORD)rllen;
		dfile->UnlockWrite();

		// Read a big chunk of details 
		// Mutex read against VSOrder::ReadDetail
		dfile->LockRead();
		OVERLAPPED *povl=0;
		// Read is already pending
		//WSLogEvent("DEBUG Read[%I64d,%I64d) max %I64d",dfile->rend.QuadPart,dfile->rend.QuadPart +rlen,dfile->fend.QuadPart);
		SetFilePointer(dfile->rhnd,dfile->rend.LowPart,(PLONG)&dfile->rend.HighPart,FILE_BEGIN);
		DWORD rbytes=0;
		int rc=ReadFile(dfile->rhnd,rptr,rlen,&rbytes,0);
		if(rc<0)
		{
			#ifdef WIN32
			WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),dfile->rend.QuadPart,rlen);
			#else
			WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),dfile->rend.QuadPart,rlen);
			#endif
			dfile->UnlockRead();
			break;
		}
		bool lastAvail=false;
		if(ravail -rlen<=0)
			lastAvail=true;
		dfile->UnlockRead(); // No need to lock the file while we're parsing

		// Process each detail
		FIXINFO fi;
		memset(&fi,0,sizeof(FIXINFO));
		// Check 'proto' to determine how to parse
		if(dfile->proto==PROTO_MLFIXLOGS)
			fi.FIXDELIM=';';
		else if(dfile->proto==PROTO_TJMLOG)
			fi.FIXDELIM=' ';
		else if((dfile->proto==PROTO_MLPH_EMEA)||(dfile->proto==PROTO_MLPH_CC))
		{
			fi.FIXDELIM=';';
			fi.FIXDELIM2=' ';
		}
		else
			fi.FIXDELIM=0x01;
		fi.noSession=true;
		fi.supressChkErr=true;
		const char *fend=rbuf +rbytes;
		const char *fptr;
		for(fptr=rbuf;fptr<fend;)
		{
			const char *fnext=strechr(fptr,0xA,fend);
			if(!fnext)
			{
				//WSLogEvent("DEBUG need more at %I64d",dfile->rend.QuadPart +(int)(fptr -rbuf));
				if(lastAvail)
					SleepEx(100,true); // prevent max CPU when there's no more to read
				break; // Read more from file
			}
			fi.Reset();
			int flen=fi.FixMsgReady((char*)fptr,(dfile->proto==PROTO_TJMLOG)?(int)(fnext -fptr -1):(int)(fnext -fptr));
			if(flen<1)
			{
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d!",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld!",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#endif
				flen=(int)(fnext -fptr);
			}
			if((dfile->proto!=PROTO_ITF)&&(fi.ntags<5))
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d! Possibly marked with wrong protocol.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld! Possibly marked with wrong protocol.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#endif
			else if((dfile->proto==PROTO_ITF)&&(!fi.GetTag(35)))
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d! Missing tag 35.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld! Missing tag 35.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#endif
			else
			{
				// TODO: If we want 100% accurate counts, then we'll have to mutex 'ustats' against WSTimeChange
				WSLockPort(WS_USC,UscPortNo,false);
				HandleUscFixMsg(&fi,dfile->portno,ustats,dfile->rend.QuadPart +(int)(fptr -rbuf),dfile->proto);
				WSUnlockPort(WS_USC,UscPortNo,false);
			}
			fptr+=flen +1;
		}
		if(fptr>rbuf)
		{
			dfile->rend.QuadPart+=(fptr -rbuf); 
			odb.SetZdirty();
			rptr=rbuf;
		}
		else
			rptr=rbuf;
		dfile->LockRead();
	}
	dfile->UnlockRead();
	delete rbuf;
	return 0;
}
#else//!NO_OVL_READ_DETAILS
// Windows with overlapped I/O
int ViewServer::ReadDetailThread(VSDetailFile *dfile)
{
#define READBLOCKSIZE (512*1024) // 512K tried 256K,1M,2M,4M; this is the best value
	// In order to maximize CPU on this thread, always have an overlapping read while we're processing
	OVERLAPPED ovls[2];
	char *rbufs[2];
	char *rbuf=new char[READBLOCKSIZE*2],*rptr=rbuf,*rbend=rbuf +READBLOCKSIZE*2;
	if(!rbuf)
	{
		_ASSERT(false);
		return -1;
	}
	memset(rbuf,0,READBLOCKSIZE*2);
	for(int i=0;i<2;i++)
	{
		memset(&ovls[i],0,sizeof(OVERLAPPED));
		rbufs[i]=new char[READBLOCKSIZE];
		if(!rbufs[i])
		{
			_ASSERT(false);
			return -1;
		}
		memset(rbufs[i],0,READBLOCKSIZE);
	}
	int pendidx=-1;
	dfile->LockRead();
	// Since we're double-buffering, 'rnext' doesn't always equal 'dfile->rend'
	LARGE_INTEGER rnext;
	rnext.QuadPart=dfile->rend.QuadPart;
	FixStats *ustats=(FixStats *)UscPort[dfile->portno].DetPtr3;
	if(!ustats)
	{
		_ASSERT(false);
		dfile->UnlockRead();
		return -1;
	}
	while(dfile->rhnd!=INVALID_FILE_VALUE)
	{
		dfile->UnlockRead();
		// 'rnext' is local so we don't need to lock read.
		// Mutex write against VSOrder::WriteDetailBlock.
		dfile->LockWrite();
	#ifdef IQSMP
		if(dfile->rend.QuadPart>=dfile->fend.QuadPart)
		// Can't remember why I thought this might be necessary
		//if(rnext.QuadPart>=dfile->fend.QuadPart)
	#else
		if(rnext.QuadPart>=dfile->fend.QuadPart)
	#endif
		{
			dfile->UnlockWrite();
			SleepEx(100,true);
			dfile->LockRead();
			continue;
		}
		// Only reference the end of write value while we have it locked
		LONGLONG ravail=dfile->fend.QuadPart -rnext.QuadPart;
		dfile->UnlockWrite();

		// Read a big chunk of details 
		// Mutex read against VSOrder::ReadDetail
		dfile->LockRead();
		OVERLAPPED *povl=0;
		// Read is already pending
		//char dbuf[1024]={0};
		DWORD rbytes=0;
		if(pendidx<0)
		{
			// Issue first read: don't exceed half of 'rbuf' or space available 
			LONGLONG rllen=ravail;
			DWORD rleftover=(DWORD)(rptr -rbuf);
			if(rllen>READBLOCKSIZE)
				rllen=READBLOCKSIZE;
			if(rllen>(READBLOCKSIZE*2 -rleftover))
				rllen=(READBLOCKSIZE*2 -rleftover);
			if(rllen>0)
			{
				DWORD rlen=(DWORD)rllen;
				ravail-=rlen;
				//sprintf(dbuf,"DEBUG: Read1[%I64d,%I64d), pend=%d\r\n",rnext.QuadPart,rnext.QuadPart +rlen,0);
				//OutputDebugString(dbuf);
				DWORD rbytes=0;
				pendidx=0;
				povl=&ovls[pendidx];
				povl->Offset=rnext.LowPart;
				povl->OffsetHigh=rnext.HighPart;
				rnext.QuadPart+=rlen;
				int rc=ReadFile(dfile->rhnd,rbufs[pendidx],rlen,&rbytes,povl);
				int err=GetLastError();
				if((rc<0)&&(err!=ERROR_IO_PENDING))
				{
					#ifdef WIN32
					WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen);
					#else
					WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen);
					#endif
					dfile->UnlockRead();
					break;
				}
				pendidx=0;
			}
		}
		// Wait for pending read
		if(pendidx>=0)
		{
			povl=&ovls[pendidx];
			rbytes=0;
			GetOverlappedResult(dfile->rhnd,povl,&rbytes,true);
			if(rbytes>0)
			{
				_ASSERT(rptr +rbytes<=rbend);
				memcpy(rptr,rbufs[pendidx],rbytes); rptr+=rbytes; 
			}
		}
		// Issue next overlapped read while we're processing: don't exceed half of 'rbuf' or space available 
		if(ravail>0)
		{
			LONGLONG rllen2=ravail;
			DWORD rleftover=(DWORD)(rptr -rbuf);
			if(rllen2>READBLOCKSIZE)
				rllen2=READBLOCKSIZE;
			if(rllen2>READBLOCKSIZE*2 -rleftover)
				rllen2=READBLOCKSIZE*2 -rleftover;
			if(rllen2>0)
			{
				DWORD rlen2=(DWORD)rllen2;
				ravail-=rlen2;
				DWORD rbytes2=0;
				pendidx=(pendidx +1)%2;
				//sprintf(dbuf,"DEBUG: Read2[%I64d,%I64d), pend=%d\r\n",rnext.QuadPart,rnext.QuadPart +rlen2,pendidx);
				//OutputDebugString(dbuf);
				povl=&ovls[pendidx];
				povl->Offset=rnext.LowPart;
				povl->OffsetHigh=rnext.HighPart;
				rnext.QuadPart+=rlen2;
				int rc=ReadFile(dfile->rhnd,rbufs[pendidx],rlen2,&rbytes2,povl);
				int err=GetLastError();
				if((rc<0)&&(err!=ERROR_IO_PENDING))
				{
					#ifdef WIN32
					WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen2);
					#else
					WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen2);
					#endif
					dfile->UnlockRead();
					break;
				}
			}
			else
				pendidx=-1;
		}
		else
			pendidx=-1;
		dfile->UnlockRead(); // No need to lock the file while we're parsing

		// FIX protocol
		FIXINFO fi;
		memset(&fi,0,sizeof(FIXINFO));
		if(dfile->proto==PROTO_MLFIXLOGS)
			fi.FIXDELIM=';';
		else if(dfile->proto==PROTO_TJMLOG)
			fi.FIXDELIM=' ';
		else if((dfile->proto==PROTO_TBACKUP)||(dfile->proto==PROTO_FULLLOG))
			fi.FIXDELIM='|';
		else if((dfile->proto==PROTO_MLPH_EMEA)||(dfile->proto==PROTO_MLPH_CC))
		{
			fi.FIXDELIM=';';
			fi.FIXDELIM2=' ';
		}
		else if(dfile->proto==PROTO_TWISTLOG)
			fi.FIXDELIM='_';
		else
			fi.FIXDELIM=0x01;
		fi.noSession=true;
		fi.supressChkErr=true;
		const char *fend=rptr;
		const char *fptr;
		for(fptr=rbuf;fptr<fend;)
		{
			const char *fendl=0,*fstart=fptr;
			int flen=0;
			// Tag 10 delimiter between FIX messsages
			if(dfile->proto==PROTO_IQFIXS)
			{
				static const char estr[]={0x01,'1','0','=',0};
				fendl=strestr(fptr,estr,fend);
				if(!fendl)
				{
					if(pendidx<0)
						SleepEx(100,true); // prevent max CPU when there's no more to read or rbuf full
					break; // Read more from file
				}
				if(fendl)
					fendl=strechr(fendl+4,0x01,fend);
				if(fendl)
					fendl++;
				else
				{
					if(pendidx<0)
						SleepEx(100,true); // prevent max CPU when there's no more to read or rbuf full
					break; // Read more from file
				}
				flen=(int)(fendl -fptr);
			}
			// CRLF delimiter between messages
			else
			{
				// Read blocks might happen to have been split between CR and LF, so skip any leading LFs (or CRs)
				for(;(*fptr=='\r')||(*fptr=='\n');fptr++)
					;
				fendl=strendl(fptr,fend);
				if(!fendl)
				{
					if(pendidx<0)
						SleepEx(100,true); // prevent max CPU when there's no more to read or rbuf full
					break; // Read more from file
				}
				if((fendl>fptr)&&(fendl[-1]=='\r')) // CRLF
					fendl--;
				flen=(int)(fendl -fptr);
				// Skip IQ OMS FIX msg header
				if((dfile->proto==PROTO_IQOMS)||(dfile->proto==PROTO_OMS_CBOE))
				{
					for(int d=0;d<4;d++)
					{
						const char *nptr=strechr(fptr,':',fptr +32);
						if(!nptr)
							break;
						flen-=(int)(nptr +1 -fptr);
						fptr=nptr+1;
					}
				}
				// Walk backwards from last 0x1D0A to previous 0x1D
				else if(dfile->proto==PROTO_ITF)
				{
					const char *iptr;
					for(iptr=fendl -2;(iptr>=fptr)&&(*iptr!=0x1d);iptr--)
						;
					fptr=iptr+1; fendl--;
				}
				else if(dfile->proto==PROTO_TJMLOG)
				{
					const char *nptr=strechr(fptr,'[',fendl);
					if(nptr)
						fptr=nptr+1;
					flen=(int)(fendl -fptr -1);
				}
				// Skip full log FIX msg header
				else if((dfile->proto==PROTO_TBACKUP)||(dfile->proto==PROTO_FIXDROP)||(dfile->proto==PROTO_FULLLOG)||(dfile->proto==PROTO_TWIST)||(dfile->proto==PROTO_RTAWAY))
				{
					for(int d=0;d<3;d++)
					{
						const char *nptr=strechr(fptr,':',fptr +32);
						if(!nptr)
							break;
						flen-=(int)(nptr +1 -fptr);
						fptr=nptr+1;
					}
				}
				// FIX is in [brackets]
				// Is this really the same as MLFIXLOGS or should it be a new protocol CCFIXLOGS?
				else if(dfile->proto==PROTO_MLPH_CC)
				{
					if(PubHubFixSubstr(fptr,flen)<0)
					{
						((char*)fendl)[-1]=0;
						WSLogEvent("No known PUBHUB tags found in [%s]",fptr);
					}
					fi.FIXDELIM2=' ';
				}
			#ifdef IQSMP
				// Skip TWIST log header
				// 07/06/2012  09:46:21.996 <== 
				// 07/06/2012  09:46:22.009 ==> 
				else if(dfile->proto==PROTO_TWISTLOG)
				{
					if(flen>=29)
					{
						fptr+=29; flen-=29;
					}
				}
			#endif
			}

			fi.Reset();
			flen=fi.FixMsgReady((char*)fptr,flen);
			if(flen<1)
			{
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d!",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld!",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#endif
				flen=(int)(fendl -fptr);
			}
			if((dfile->proto!=PROTO_ITF)&&(dfile->proto!=PROTO_MLPH_EMEA)&&(dfile->proto!=PROTO_MLPH_CC)&&(fi.ntags<5))
			{
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d! Possibly marked with wrong protocol.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld! Possibly marked with wrong protocol.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#endif
			}
			else if((dfile->proto==PROTO_ITF)&&(!fi.GetTag(35)))
			{
				// R. Mandell confirmed sporatic invalid messages (780/day out of 25 million)
				#ifdef WIN32
				WSLogEvent("Failed parsing FIX in data file(%s) at %I64d! Missing tag 35.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#else
				WSLogEvent("Failed parsing FIX in data file(%s) at %lld! Missing tag 35.",dfile->prefix.c_str(),dfile->rend.QuadPart +(int)(fptr -rbuf));
				#endif
			}
			else
			{
				// TODO: If we want 100% accurate counts, then we'll have to mutex 'ustats' against WSTimeChange
				WSLockPort(WS_USC,dfile->portno,false);
				HandleUscFixMsg(&fi,dfile->portno,ustats,dfile->rend.QuadPart +(int)(fptr -rbuf),dfile->proto);
				WSUnlockPort(WS_USC,dfile->portno,false);
			}

			//if(dfile->proto==PROTO_IQFIXS)
			//	fptr+=flen;
			//else if(dfile->proto==PROTO_ITF)
			//{
			//	fptr+=flen+2; // what happens if this doesn't reach 'fendl'?
			//	_ASSERT(fptr<=fend);
			//}
			//else
			//{
			//	fptr+=flen;
			//	while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
			//		fptr++;
			//	_ASSERT(fptr<=fend);
			//}
			fptr=fendl;
			if((dfile->proto==PROTO_ITF)&&(fptr<fend)) // 0x1d
				fptr++;
			while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
				fptr++;
			_ASSERT(fptr<=fend);
		}

		// Preserve leftovers
		rptr=rbuf;
		if(fptr>fend)
		{
			_ASSERT(false);
			fptr=fend;
		}
		int bleft=(int)(fend -fptr);
		if(fptr>rbuf)
		{
			// This is the only function that modifies 'dfile->rend', so we don't need to lock read.
			_ASSERT((rbuf<=fptr)&&(fptr<=fend));
			dfile->rend.QuadPart+=(fptr -rbuf); 
			odb.SetZdirty();
			//sprintf(dbuf,"DEBUG: leftover %d [%I64d,%d), pend=%d\r\n",bleft,dfile->rend.QuadPart,dfile->rend.QuadPart +bleft,pendidx);
			//OutputDebugString(dbuf);
			memmove(rptr,fptr,bleft);
		#ifdef _DEBUG
			memset(rptr +bleft,0,rbend -rptr -bleft);
		#endif
		}
		rptr+=bleft;
		dfile->LockRead();
	}
	dfile->UnlockRead();
	for(int i=0;i<2;i++)
		delete rbufs[i];
	delete rbuf;
	return 0;
}
#endif//!NO_OVL_READ_DETAILS
#else//!WIN32
// Linux
int ViewServer::ReadDetailThread(VSDetailFile *dfile)
{
#define READBLOCKSIZE (512*1024) // 512K tried 256K,1M,2M,4M; this is the best value
	char *rbuf=new char[READBLOCKSIZE],*rptr=rbuf;
	dfile->LockRead();
	FixStats *ustats=(FixStats *)UscPort[dfile->portno].DetPtr3;
	if(!ustats)
	{
		_ASSERT(false);
		dfile->UnlockRead();
		return -1;
	}
	while(dfile->rhnd!=INVALID_FILE_VALUE)
	{
		dfile->UnlockRead();
		// This is the only function to modify 'dfile->rend.QuadPart' so we don't need to lock read.
		// Mutex write against VSOrder::WriteDetailBlock.
		dfile->LockWrite();
		if(dfile->rend>=dfile->fend)
		{
			dfile->UnlockWrite();
			SleepEx(100,true);
			dfile->LockRead();
			continue;
		}
		// Only reference the end of write value while we have it locked
		LONGLONG ravail=dfile->fend -dfile->rend;
		LONGLONG rllen=ravail;
		int rused=(int)(rptr -rbuf);
		if(rllen>(READBLOCKSIZE -rused))
			rllen=(READBLOCKSIZE -rused);
		DWORD rlen=(DWORD)rllen;
		dfile->UnlockWrite();

		// Read a big chunk of details 
		// Mutex read against VSOrder::ReadDetail
		dfile->LockRead();
		// Read is already pending
		//WSLogEvent("DEBUG Read[%I64d,%I64d) max %I64d",dfile->rend,dfile->rend +rlen,dfile->fend);
		fseeko(dfile->rhnd,dfile->rend,SEEK_SET);

		DWORD rbytes=0;
		rbytes=(DWORD)fread(rptr,1,rlen,dfile->rhnd);
		if(rbytes!=rlen)
		{
			#ifdef WIN32
			WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",
			#else
			WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",
			#endif
				dfile->prefix.c_str(),GetLastError(),dfile->rend,rlen);
			dfile->UnlockRead();
			break;
		}
		bool lastAvail=false;
		if(ravail -rlen<=0)
			lastAvail=true;
		dfile->UnlockRead(); // No need to lock the file while we're parsing

		// Process each detail
		FIXINFO fi;
		memset(&fi,0,sizeof(FIXINFO));
		// Check 'proto' to determine how to parse
		if(dfile->proto==PROTO_MLFIXLOGS)
			fi.FIXDELIM=';';
		else if(dfile->proto==PROTO_TJMLOG)
			fi.FIXDELIM=' ';
		else if((dfile->proto==PROTO_TBACKUP)||(dfile->proto==PROTO_FULLLOG))
			fi.FIXDELIM='|';
		else if((dfile->proto==PROTO_MLPH_EMEA)||(dfile->proto==PROTO_MLPH_CC))
		{
			fi.FIXDELIM=';';
			fi.FIXDELIM2=' ';
		}
		else
			fi.FIXDELIM=0x01;
		fi.noSession=true;
		fi.supressChkErr=true;
		const char *fend=rbuf +rbytes;
		const char *fptr;
		for(fptr=rbuf;fptr<fend;)
		{
			const char *fnext=strechr(fptr,0xA,fend);
			if(!fnext)
			{
				static long buffErrorSize=5000;
				if((fend-fptr)>buffErrorSize)
				{
					WSLogError("Read more than %lld bytes without detecting an end of FIX msg indicator (0xA)",fend-fptr);
					buffErrorSize*=10;
				}

				//WSLogEvent("DEBUG need more at %I64d",dfile->rend +(int)(fptr -rbuf));
				if(lastAvail)
					SleepEx(100,true); // prevent max CPU when there's no more to read
				break; // Read more from file
			}
			fi.Reset();
			int flen=fi.FixMsgReady((char*)fptr,(dfile->proto==PROTO_TJMLOG)?(int)(fnext -fptr -1):(int)(fnext -fptr));
			if(flen<1)
			{
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d!",dfile->prefix.c_str(),dfile->rend +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld!",dfile->prefix.c_str(),dfile->rend +(int)(fptr -rbuf));
				#endif
				flen=(int)(fnext -fptr);
			}
			if((dfile->proto!=PROTO_ITF)&&(fi.ntags<5))
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d! Possibly marked with wrong protocol.",dfile->prefix.c_str(),dfile->rend +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld! Possibly marked with wrong protocol.",dfile->prefix.c_str(),dfile->rend +(int)(fptr -rbuf));
				#endif
			else if((dfile->proto==PROTO_ITF)&&(!fi.GetTag(35)))
				#ifdef WIN32
				WSLogError("Failed parsing FIX in data file(%s) at %I64d! Missing tag 35.",dfile->prefix.c_str(),dfile->rend +(int)(fptr -rbuf));
				#else
				WSLogError("Failed parsing FIX in data file(%s) at %lld! Missing tag 35.",dfile->prefix.c_str(),dfile->rend +(int)(fptr -rbuf));
				#endif
			else
			{
				// TODO: If we want 100% accurate counts, then we'll have to mutex 'ustats' against WSTimeChange
				WSLockPort(WS_USC,dfile->portno,false);
				HandleUscFixMsg(&fi,dfile->portno,ustats,dfile->rend +(int)(fptr -rbuf),dfile->proto);
				WSUnlockPort(WS_USC,dfile->portno,false);
			}
			fptr+=flen +1;
		}
		if(fptr>rbuf)
		{
			dfile->rend+=(fptr -rbuf); 
			odb.SetZdirty();
			rptr=rbuf;
		}
		else
			rptr=rbuf;
		dfile->LockRead();
	}
	dfile->UnlockRead();
	delete rbuf;
	return 0;
}
#endif//!WIN32

int ViewServer::HandleUscFixMsg(FIXINFO *pfix, int UscPortNo, FixStats *pstats, LONGLONG doffset, int proto)
{
	_ASSERT((pfix)&&(UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS)&&(pstats));
	// Stats
	pstats->msgCnt++; pstats->totMsgCnt++;
	if(pfix->ntags>0)
	{
		pstats->totFixTag+=pfix->ntags; pstats->cntFixTag++;
		if((DWORD)pfix->ntags>pstats->maxFixTag) 
			pstats->maxFixTag=pfix->ntags;
	}
	if(pfix->llen>0)
	{		
		pstats->totFixLen+=pfix->llen; pstats->cntFixLen++;
		if((DWORD)pfix->llen>pstats->maxFixLen) 
			pstats->maxFixLen=pfix->llen;
	}
	// Timing 10 channels, 2M msgs: ~155K mps max if we return here

	// MsgType
	char mtype=pfix->TagChar(35);
	switch(mtype)
	{
	case 'D': pstats->totFix35_D++; break;
	case 'F': pstats->totFix35_F++; break;
	case 'G': pstats->totFix35_G++; break;
	case '8': pstats->totFix35_8++; break;
	case '3': break;
	case '0': 
	case '1':
	case '2':
	case '4':
	case '5':
	case 'A':
	case 'Q':
		return 0; 
	};

	// Unique ClOrdID
	string t11505=pfix->TagStr(11505);
	string t11=pfix->TagStr(11);
	char t49[16]={0},t50[16]={0},t56[16]={0},t57[16]={0};
	strncpy(t49,pfix->TagStr(49),sizeof(t49)-1);
	t49[sizeof(t49)-1]=0;
	strncpy(t50,pfix->TagStr(50),sizeof(t50)-1);
	t50[sizeof(t50)-1]=0;
	strncpy(t56,pfix->TagStr(56),sizeof(t56)-1);
	t56[sizeof(t56)-1]=0;
	strncpy(t57,pfix->TagStr(57),sizeof(t57)-1);
	t57[sizeof(t57)-1]=0;

	//DT9874
	char etype=0;
	if(proto==PROTO_MLPUBHUB) 
	{
		etype=pfix->TagChar(39);
	}
	else
	{
		etype=pfix->TagChar(150);
		char restateTag = pfix->TagChar(378);
		if(etype=='D' && restateTag=='1')
			etype= pfix->TagChar(39);
		if((!etype)||(etype=='F')) 
			etype=pfix->TagChar(39);
	}
	if(mtype=='8')
	{
		// Cancelled  count
		if(etype=='4')
			pstats->totFix150_4++;
	}
	int klen=(int)t11.length();
	pstats->totKeyLen+=klen; pstats->cntKeyLen++;
	if(klen>(int)pstats->maxKeyLen)
		pstats->maxKeyLen=klen;
	// Timing 10 channels, 2M msgs: ~151K mps max if we return here

	VSDistJournal *dj=(VSDistJournal *)UscPort[UscPortNo].DetPtr5;
	if(!dj)
		return -1;
	// Don't exceed per-journal memory limit.
	// Can do about 1.6M msgs/100MB journal memory
	if(dj->GetMemUsage()>=MAX_DISTJ_SIZE*1024*1024)
	{
	#ifdef NO_FIX_WRITES
		// Okay to block ReadDetailThread indefinitely, but not WSUsrMsgReady
		static DWORD wstart=GetTickCount(),wnext=wstart +1000;
		DWORD tnow=GetTickCount();
		if(tnow>=wnext)
		{
			WSLogError("USC%d: MAX_DISTJ_SIZE(%d MB) exceeded-Throttling journal send.",UscPortNo,MAX_DISTJ_SIZE);
			while(wnext<=tnow)
				wnext+=1000;
		}
		return -2;
	#else
		DWORD wstart=GetTickCount(),wnext=wstart +1000;
		while(dj->GetMemUsage()>=MAX_DISTJ_SIZE*1024*1024)
		{
			// Let WSTimeChange get a breath
			WSUnlockPort(WS_USC,UscPortNo,false);
			DWORD tnow=GetTickCount();
			if(tnow>=wnext)
			{
				WSLogError("USC%d: MAX_DISTJ_SIZE(%d MB) exceeded-Pausing journal send.",UscPortNo,MAX_DISTJ_SIZE);
				while(wnext<=tnow)
					wnext+=1000;
			}
			SleepEx(100,true);
			WSLockPort(WS_USC,UscPortNo,false);
		}
	#endif
	}
	// TagStr supports no more than 16 strings on stack so copy to stack variables
	char AppInstID[20]={0};			// AppInstID(11505)
	char ClOrdID[40]={0};			// ClOrdID(11)
	char RootOrderID[40]={0};		// tag 70129
	char FirstClOrdID[40]={0};		// tag 5055
	char ClParentOrderID[40]={0};	// tag 5035
	char Symbol[32]={0};			// Symbol(55,65)
	double Price=0.0;
	char Side=0;
	char SymSfx[12]={0};			// Symbol(55,65)
	char Account[20]={0};			// Account(1)
	char EcnOrderID[40]={0};		// EcnOrderID(37)
	char ClientID[24]={0};			// ClientID(109)
	char Connection[48]={0};		// Connection
	char AuxKey[AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN]={0}; // AuxKeys  DT9398
	strncpy(AppInstID,pfix->TagStr(11505),sizeof(AppInstID)-1);
	AppInstID[sizeof(AppInstID)-1]=0;
	strncpy(ClOrdID,pfix->TagStr(11),sizeof(ClOrdID)-1);
	ClOrdID[sizeof(ClOrdID)-1]=0;
	char OrigClOrdID[40]={0};		// OrigClOrdID(41)
#ifdef HOLD_IQ_CHILD_ORDERS
	bool checkHeldDetails=false;
#endif
	bool isTicket=false;
	bool isAway=false;
	strncpy(OrigClOrdID,pfix->TagStr(41),sizeof(OrigClOrdID));
	OrigClOrdID[sizeof(OrigClOrdID)-1]=0;
	// IQ OMS parent-child linking
	if((proto==PROTO_IQOMS)||(proto==PROTO_IQFIXS))
	{
		_ASSERT(IQFLOW);
		WaitForSingleObject(iqmux,INFINITE);
		// FirstClOrdID by ClOrdID->FirstClOrdID map
		strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
		FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
		map<string,string>::iterator fit=fm->find(ClOrdID);
		if(fit!=fm->end())
			strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
		else
		{
			const char *t41=pfix->TagStr(41);
			if(*t41)
			{
				fit=fm->find(t41);
				if(fit!=fm->end())
				{
					strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
				}
			}
			fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
		#ifdef HOLD_IQ_CHILD_ORDERS
			checkHeldDetails=true;
		#endif
		}
		// AppInstID by ClOrdID->AppInstID map or 2007/2005 match
		const char *t2007=pfix->TagStr(2007);
		const char *t2005=pfix->TagStr(2005);

		//DT10009
		if(!am)
		{
			ReleaseMutex(iqmux);
			WSLogEvent("am is null, is IQFLOW on?");
			return 0;
		}
		map<string,string>::iterator ait=am->find(ClOrdID);
		if(ait!=am->end())
			strncpy(AppInstID,ait->second.c_str(),sizeof(AppInstID)-1);
		else
		{
			if((*t2007)||(*t2005))
			{
				MatchEntry *pmatch=GetMatch(*t2007 ?t2007:t2005);
				if(pmatch)
					strncpy(AppInstID,pmatch->key.c_str(),sizeof(AppInstID)-1);
				else
					strncpy(AppInstID,"UNKNOWN",sizeof(AppInstID)-1);
			}
			else
				strncpy(AppInstID,pfix->TagStr(49),sizeof(AppInstID)-1);
		}
		AppInstID[sizeof(AppInstID)-1]=0;
		am->insert(pair<string,string>(ClOrdID,AppInstID));
		// Ignore session level messages
		if(!*ClOrdID)
		{
			ReleaseMutex(iqmux);
			return 0;
		}
		else if(!*AppInstID)
		{
			ReleaseMutex(iqmux);
			WSLogEvent("USC%d: Missing 'AppInstID' at offset %I64d",UscPortNo,doffset);
			return 0;
		}
		// ClParentOrderID by last 2007/2005 value
		if(*t2007)
		{
			const char *rptr=strrchr(t2007,'%');
			if(rptr)
			{
				strncpy(ClParentOrderID,rptr+1,sizeof(ClParentOrderID)-1);
				ClParentOrderID[sizeof(ClParentOrderID)-1]=0;
				pm->insert(pair<string,string>(ClOrdID,ClParentOrderID));
			}
			// Invalid 2007 format; copy the real value from the tag 11 map (should be none of these)
			else
			{
				map<string,string>::iterator cit=pm->find(ClOrdID);
				if(cit!=pm->end())
				{
					strncpy(ClParentOrderID,cit->second.c_str(),sizeof(ClParentOrderID)-1);
					ClParentOrderID[sizeof(ClParentOrderID)-1]=0;
				}
			}
		}
		else if(*t2005)
		{
			const char *rptr=strrchr(t2005,'%');
			if(rptr)
			{
				strncpy(ClParentOrderID,rptr+1,sizeof(ClParentOrderID)-1);
				ClParentOrderID[sizeof(ClParentOrderID)-1]=0;
				pm->insert(pair<string,string>(ClOrdID,ClParentOrderID));
			}
			// Invalid 2005 format; copy the real value from the tag 11 map (there are lots of these)
			else
			{
				map<string,string>::iterator cit=pm->find(ClOrdID);
				if(cit!=pm->end())
				{
					strncpy(ClParentOrderID,cit->second.c_str(),sizeof(ClParentOrderID)-1);
					ClParentOrderID[sizeof(ClParentOrderID)-1]=0;
				}
			}
		}
		// RootOrderID by ClParentOrderID->FirstClOrdID lookup
		// Must play back parent orders before child orders because the FIX doesn't have 70129 on child orders
		if(*ClParentOrderID)
		{
			map<string,string>::iterator fit=fm->find(ClParentOrderID);
			if(fit!=fm->end())
			{
				strncpy(RootOrderID,fit->second.c_str(),sizeof(RootOrderID)-1);
				RootOrderID[sizeof(RootOrderID)-1]=0;
			}
			// Placeholder parent order
			else
			{
			#ifdef HOLD_IQ_CHILD_ORDERS
				sprintf(RootOrderID,"[%s]",ClParentOrderID);
				HeldDetail *phd=new HeldDetail;
				phd->pfix=new FIXINFO;
				phd->pfix->Reset();
				phd->pfix->noSession=pfix->noSession;
				phd->pfix->supressChkErr=pfix->supressChkErr;
				phd->pfix->slen=pfix->llen;
				phd->pfix->sbuf=new char[phd->pfix->slen];
				memcpy(phd->pfix->sbuf,pfix->fbuf,pfix->llen);
				phd->pfix->FixMsgReady(phd->pfix->sbuf,phd->pfix->slen);
				phd->UscPortNo=UscPortNo;
				phd->doffset=doffset;
				phd->proto=proto;
				multimap<string,HeldDetail*>::iterator hit=hm->find(RootOrderID);
				if(hit==hm->end())
					hm->insert(pair<string,HeldDetail*>(RootOrderID,phd));
				else
					hm->insert(hit,pair<string,HeldDetail*>(RootOrderID,phd)); // LIFO order
				ReleaseMutex(iqmux);
				return 0;
			#else
				sprintf(RootOrderID,"[%s]",ClParentOrderID);
			#endif
			}
		}
		ReleaseMutex(iqmux);
	}
	else if((proto==PROTO_TJMLOG)||(proto==PROTO_FIXDROP)||(proto==PROTO_CLDROP)||(proto==PROTO_CLBACKUP)||
			(proto==PROTO_TBACKUP)||(proto==PROTO_TJM)||(proto==PROTO_FULLLOG)||(proto==PROTO_OMS_CBOE))
	{
		if(proto==PROTO_TJM)
		{
			strncpy(AppInstID,pfix->TagStr(49),sizeof(AppInstID)-1);
			AppInstID[sizeof(AppInstID)-1]=0;
		}
	#ifdef IQSMP
		else if(proto==PROTO_FULLLOG)
		{
			// AppInstID max 19 chars!!!
			char taid[256]={0};
			// Injected AppInstID override line: 8=FIX.4.2|9=0|34=0|11505=MLCHFIX1|52=00000000-00:00:00|10=0|
			if((!t11505.empty())&&(pfix->TagInt(9)==0))
			{
				char *smpaid=new char[20];
				strncpy(smpaid,t11505.c_str(),19);
				smpaid[19]=0;
				if(UscPort[UscPortNo].DetPtr2)
					delete (char*)UscPort[UscPortNo].DetPtr2;
				UscPort[UscPortNo].DetPtr2=smpaid;
				return 0;
			}
			// AppInstID overridden
			else if(UscPort[UscPortNo].DetPtr2)
			{
				strncpy(AppInstID,(char*)UscPort[UscPortNo].DetPtr2,sizeof(AppInstID)-1);
				AppInstID[sizeof(AppInstID)-1]=0;
			}
			else
			{
				if((mtype=='8')||(mtype=='9')||(mtype=='3'))
				{
					sprintf(taid,"%s.%s",pfix->TagStr(56),pfix->TagStr(49));
					if(strlen(taid)<20)
						strcpy(AppInstID,taid);
					// Rightmost 9 characters of each
					else
					{
						char *aptr=AppInstID;
						strcpy(taid,pfix->TagStr(56));
						if(strlen(taid)>9) memmove(taid,taid +strlen(taid) -9,10);
						sprintf(aptr,"%s.",taid); aptr+=strlen(aptr);
						strcpy(taid,pfix->TagStr(49));
						if(strlen(taid)>9) memmove(taid,taid +strlen(taid) -9,10);
						sprintf(aptr,"%s",taid); aptr+=strlen(aptr);
					}
				}
				else
				{
					sprintf(taid,"%s.%s",pfix->TagStr(49),pfix->TagStr(56));
					if(strlen(taid)<20)
						strcpy(AppInstID,taid);
					// Rightmost 9 characters of each
					else
					{
						char *aptr=AppInstID;
						strcpy(taid,pfix->TagStr(49));
						if(strlen(taid)>9) memmove(taid,taid +strlen(taid) -9,10);
						sprintf(aptr,"%s.",taid); aptr+=strlen(aptr);
						strcpy(taid,pfix->TagStr(56));
						if(strlen(taid)>9) memmove(taid,taid +strlen(taid) -9,10);
						sprintf(aptr,"%s",taid); aptr+=strlen(aptr);
					}
				}
			}

			// Child FIXServer sends back 2005
			if(pfix->GetTag(2005))
				strncpy(RootOrderID,pfix->TagStr(2005),sizeof(RootOrderID)-1);
			// but gets 70129 from ML
			else
				strncpy(RootOrderID,pfix->TagStr(70129),sizeof(RootOrderID)-1);
			RootOrderID[sizeof(RootOrderID)-1]=0;
			const char *pptr=strchr(RootOrderID,'%');
			if(pptr) pptr=strchr(pptr+1,'%');
			if(pptr) pptr=strchr(pptr+1,'%');
			if(pptr)
			{
				memmove(RootOrderID,pptr +1,strlen(pptr +1) +1);
				strcpy(ClParentOrderID,RootOrderID);
			}
			// Parent FIXServer receives wrong value from ML so ignore it
			else
			{
				strcpy(RootOrderID,"");
				strncpy(ClientID,pfix->TagStr(129),sizeof(ClientID)-1);
				ClientID[sizeof(ClientID)-1]=0;
			}
		}
		else if(proto==PROTO_OMS_CBOE)
		{
			// AppInstID max 19 chars!!!
			char taid[256]={0};
			// Injected AppInstID override line: 8=FIX.4.2|9=0|34=0|11505=MLCHFIX1|52=00000000-00:00:00|10=0|
			if((!t11505.empty())&&(pfix->TagInt(9)==0))
			{
				char *smpaid=new char[20];
				strncpy(smpaid,t11505.c_str(),19);
				smpaid[19]=0;
				if(UscPort[UscPortNo].DetPtr2)
					delete (char*)UscPort[UscPortNo].DetPtr2;
				UscPort[UscPortNo].DetPtr2=smpaid;
				return 0;
			}
			// AppInstID overridden
			else if(UscPort[UscPortNo].DetPtr2)
			{
				strncpy(AppInstID,(char*)UscPort[UscPortNo].DetPtr2,sizeof(AppInstID)-1);
				AppInstID[sizeof(AppInstID)-1]=0;
			}
		}
	#endif
		else
		{
			if((mtype=='8')||(mtype=='9')||(mtype=='3'))
				strncpy(AppInstID,pfix->TagStr(56),sizeof(AppInstID)-1);
			else
				strncpy(AppInstID,pfix->TagStr(49),sizeof(AppInstID)-1);
			AppInstID[sizeof(AppInstID)-1]=0;
		}
		// Ignore session level messages
		if(!*ClOrdID)
			return 0;
		else if(!*AppInstID)
		{
			WSLogEvent("USC%d: Missing 'AppInstID' at offset %I64d",UscPortNo,doffset);
			return 0;
		}
		//char sym[32]={0};
		//strcpy(sym,pfix->TagStr(55));
		//const char *t65=pfix->TagStr(65);
		//if(*t65)
		//	sprintf(sym+strlen(sym),".%s",t65);
		//strncpy(RootOrderID,sym,sizeof(RootOrderID)-1);
		//RootOrderID[sizeof(RootOrderID)-1]=0;
		// Link C/R chains
		strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
		FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
		map<string,string>::iterator fit=fm->find(ClOrdID);
		if(fit!=fm->end())
		{
			strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
			FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
		}
		else
		{
			const char *t41=pfix->TagStr(41);
			if(*t41)
			{
				fit=fm->find(t41);
				if(fit!=fm->end())
				{
					strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
				}
			}
			fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
		}
	#ifdef IQSMP
		// Parent FIXServer receives wrong value from ML so fix it here
		if(proto==PROTO_FULLLOG)
		{
			if(!RootOrderID[0])
				strcpy(RootOrderID,FirstClOrdID);
		}
	#endif
	}
#ifdef IQSMP
	else if(proto==PROTO_TWISTLOG)
	{
		strcpy(AppInstID,"TWIST");
		//strncpy(RootOrderID,ClOrdID,sizeof(RootOrderID)-1);
		//RootOrderID[sizeof(RootOrderID)-1]=0;
	}
	#ifdef GS_RTECHOATS
	else if(proto==PROTO_RTECHOATS)
	{
		if((mtype=='8')||(mtype=='9')||(mtype=='3'))
			strcpy(AppInstID,t49);
		else
			strcpy(AppInstID,t56);
		strcpy(ClientID,pfix->TagStr(50));
		// Oops: tag 109 is not unique
		//if(((etype=='0')||(etype=='5'))&&(pfix->GetTag(109)))
		//	strcpy(ClOrdID,pfix->TagStr(109));
		strcpy(EcnOrderID,pfix->TagStr(37));
		// DEV-7240: Legacy GS gcore sends a new fake tag 11 for every single report
		char ClOrdIDKey[40];
		sprintf(ClOrdIDKey,"%s.%s",pfix->TagStr(37),pfix->TagStr(pfix->GetTag(10600)?10600:1));
		if(ClOrdIDKey[0])
		{
			map<string,string>::iterator cit=cm->find(ClOrdIDKey);
			if(cit==cm->end())
				cm->insert(pair<string,string>(ClOrdIDKey,ClOrdID));
			else
			{
				if((etype=='0')||(etype=='5'))
					cit->second=ClOrdID;
				else
					strncpy(ClOrdID,cit->second.c_str(),sizeof(ClOrdID)-1);
			}
		}
		//else if(pfix->GetTag(109))
		//	strcpy(ClOrdID,pfix->TagStr(109));
		// Option and future spreads
		if((!stricmp(pfix->TagStr(167),"MLEG"))||
		   (!stricmp(pfix->TagStr(442),"2"))||(!stricmp(pfix->TagStr(442),"3")))
			strcpy(RootOrderID,ClOrdID);
		// Link cancel/replace chain
		strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
		map<string,string>::iterator fit=fm->find(ClOrdID);
		if(fit!=fm->end())
			strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
		else
		{
			const char *t41=pfix->TagStr(41);
			if(*t41)
			{
				fit=fm->find(t41);
				if(fit!=fm->end())
					strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
			}
			// Legacy GS gcore doesn't send tag 41
			else
			{
				if(ClOrdIDKey[0])
				{
					// The very first order in the cancel/replace chain
					map<string,string>::iterator eit=em->find(ClOrdIDKey);
					if(eit!=em->end())
						strncpy(FirstClOrdID,eit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
						em->insert(pair<string,string>(ClOrdIDKey,FirstClOrdID));
					}
				}
			}
			fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
		}
		// Tickets and child orders
		if(!RootOrderID[0])
		{
			strncpy(RootOrderID,pfix->TagStr(198),sizeof(RootOrderID)-1);
			if(RootOrderID[0])
			{
				if((RootOrderID[0])&&(!strcmp(RootOrderID,EcnOrderID)))
				{
					isTicket=true;
					km->insert(pair<string,string>(pfix->TagStr(109),ClOrdID));
				}
				// Handle replace of ticket: same 198 and 109, but different 37 from 198.
				else if(etype=='5')
				{
					map<string,string>::iterator kit=km->find(pfix->TagStr(109));
					if(kit!=km->end())
					{
						isTicket=true;
						strncpy(OrigClOrdID,kit->second.c_str(),sizeof(OrigClOrdID));
						strcpy(FirstClOrdID,OrigClOrdID);
						fit=fm->find(OrigClOrdID);
						if(fit!=fm->end())
							strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}
				}
				rm->insert(pair<string,string>(FirstClOrdID,RootOrderID));
			}
			// Tag 198 missing on cancel replace messages
			else
			{
				map<string,string>::iterator rit=rm->find(FirstClOrdID);
				if(rit!=rm->end())
					strcpy(RootOrderID,rit->second.c_str());
			}
		}
		
		// DEV-6695: Identify done-away executions
		if((etype=='2')&&(!EcnOrderID[0]))
		{
			if(proto==PROTO_RTECHOATS)
			{
				isAway=true;
			}
		}
	}
	#endif
	else if((proto==PROTO_TWIST)||(proto==PROTO_RTECHOATS))
	{
		if((mtype=='8')||(mtype=='9')||(mtype=='3'))
			strcpy(AppInstID,t49);
		else
			strcpy(AppInstID,t56);
		strcpy(ClientID,pfix->TagStr(50));
		// Option and future spreads
		if((!stricmp(pfix->TagStr(167),"MLEG"))||
		   (!stricmp(pfix->TagStr(442),"2"))||(!stricmp(pfix->TagStr(442),"3")))
		{
			strcpy(RootOrderID,ClOrdID);
			//strcat(ClOrdID,"_");
			//strcat(ClOrdID,pfix->TagStr(37));
		}
		// Link cancel/replace chain
		strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
		FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
		map<string,string>::iterator fit=fm->find(ClOrdID);
		if(fit!=fm->end())
		{
			strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
			FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
		}
		else
		{
			const char *t41=pfix->TagStr(41);
			if(*t41)
			{
				fit=fm->find(t41);
				if(fit!=fm->end())
				{
					strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
				}
				else
				{
					strncpy(FirstClOrdID,t41,sizeof(FirstClOrdID)-1);
					FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
				}
			}
			fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
		}
		// Tickets
		if(!RootOrderID[0])
		{
			strncpy(RootOrderID,pfix->TagStr(198),sizeof(RootOrderID)-1);
			RootOrderID[sizeof(RootOrderID)-1]=0;
		}
	}
#endif
	else
	{
		// SORT AppInstID from tag 5010 instead of 11505
		if((proto==PROTO_MLPUBHUB)||(proto==PROTO_MLFIXLOGS)||(proto==PROTO_ITF)||(proto==PROTO_MLPH_EMEA)||(proto==PROTO_MLPH_CC))
		{
			const char *t5010=pfix->TagStr(5010);
			if(*t5010)
			{
				strncpy(AppInstID,t5010,sizeof(AppInstID)-1);
				AppInstID[sizeof(AppInstID)-1]=0;
			}
		}
		// Ignore session level messages
		if(!*ClOrdID)
		{
			strncpy(ClOrdID,"UNKCLORDID",sizeof(AppInstID)-1);
			ClOrdID[sizeof(ClOrdID)-1]=0;
		}
		if(!*AppInstID)
		{
			strncpy(AppInstID,"UNKNOWN",sizeof(AppInstID)-1);
			AppInstID[sizeof(AppInstID)-1]=0;
		}
		strncpy(RootOrderID,pfix->TagStr(70129),sizeof(RootOrderID)-1);
		RootOrderID[sizeof(RootOrderID)-1]=0;
		strncpy(FirstClOrdID,pfix->TagStr(5055),sizeof(FirstClOrdID)-1);
		FirstClOrdID[sizeof(FirstClOrdID)-1]=0;
		strncpy(ClParentOrderID,pfix->TagStr(5035),sizeof(ClParentOrderID)-1);
		ClParentOrderID[sizeof(ClParentOrderID)-1]=0;
	}
	strncpy(Symbol,pfix->TagStr(55),sizeof(Symbol)-1);
	Symbol[sizeof(Symbol)-1]=0;
	strncpy(SymSfx,pfix->TagStr(65),sizeof(SymSfx)-1);
	SymSfx[sizeof(SymSfx)-1]=0;
	if((proto==PROTO_TWIST)||(proto==PROTO_RTECHOATS))
	{
		const char *SecurityID=pfix->TagStr(48);
		if(*SecurityID)
		{
			if((strlen(SecurityID)>20)&&(SecurityID[20]!='0')&&(!isspace(SecurityID[20]!='0')))
				WSLogError("USC%d: !!!! SYMBOL OVERFLOW [%s],[%s] - Call and Correct Manually(%s,%.4f,%d)",UscPortNo,AppInstID,ClOrdID,SecurityID,pfix->TagFloat(6),pfix->TagInt(32));
			strncpy(Symbol,SecurityID,sizeof(Symbol)-1);
			Symbol[sizeof(Symbol)-1]=0;
		}
	}
	if(*SymSfx)
		sprintf(Symbol+strlen(Symbol),".%s",SymSfx);
	Price=pfix->TagFloat(44);
	Side=pfix->TagChar(54);
	strncpy(Account,pfix->TagStr(pfix->GetTag(10600)?10600:1),sizeof(Account)-1);
	Account[sizeof(Account)-1]=0;
	strncpy(EcnOrderID,pfix->TagStr(37),sizeof(EcnOrderID)-1);
	EcnOrderID[sizeof(EcnOrderID)-1]=0;
	if(!ClientID[0])
	{
		strncpy(ClientID,pfix->TagStr(109),sizeof(ClientID)-1);
		ClientID[sizeof(ClientID)-1]=0;
	}
	// TODO: Add private time conversion functions
	const char *TimestampStr=pfix->TagStr(60);
	if(!*TimestampStr)
		TimestampStr=pfix->TagStr(52);
	DWORD wsdate=0,hr=0,min=0,sec=0,ms=0;
	sscanf(TimestampStr,"%08d-%02d:%02d:%02d.%03d",&wsdate,&hr,&min,&sec,&ms);
	LONGLONG Timestamp=((LONGLONG)wsdate*1000000000) +(hr*10000000) +(min*100000) +(sec*1000) +ms;
	int OrderDate=0;
#ifdef MULTI_DAY
	OrderDate=(int)(Timestamp/1000000000);
#endif
	LONGLONG dend=doffset +pfix->llen +1;
	if((proto==PROTO_MLPUBHUB)||(proto==PROTO_MLFIXLOGS)||(proto==PROTO_ITF)||(proto==PROTO_MLPH_EMEA)||(proto==PROTO_MLPH_CC))
		sprintf(Connection,"%s.%s",pfix->TagStr(73145),pfix->TagStr(73146));
#ifdef IQSMP
	else if(proto==PROTO_FULLLOG)
	{
		if((mtype=='8')||(mtype=='9')||(mtype=='3'))
			sprintf(Connection,"%s.%s",t56,t50);
		else
			sprintf(Connection,"%s.%s",t49,pfix->TagStr(115) ?pfix->TagStr(115) :pfix->TagStr(109));
	}
	else if((proto==PROTO_TWISTLOG)||(proto==PROTO_TWIST)||(proto==PROTO_RTECHOATS))
	{
		if((mtype=='8')||(mtype=='9')||(mtype=='3'))
			//sprintf(Connection,"%s.%s",pfix->TagStr(128),pfix->TagStr(115));
			sprintf(Connection,"%s.%s",pfix->TagStr(56),pfix->TagStr(49));
		else
			//sprintf(Connection,"%s.%s",pfix->TagStr(115),pfix->TagStr(128));
			sprintf(Connection,"%s.%s",pfix->TagStr(49),pfix->TagStr(56));
	}
	else if(proto==PROTO_OMS_CBOE)
	{
		strncpy(Account,pfix->TagStr(440),sizeof(Account)-1);
		Account[sizeof(Account)-1]=0;
		if(pfix->GetTag(128))
			strcpy(ClientID,pfix->TagStr(128));
		else if(pfix->GetTag(115))
			strcpy(ClientID,pfix->TagStr(115));
	}
#endif
	else
	{
		if((mtype=='8')||(mtype=='9')||(mtype=='3'))
			sprintf(Connection,"%s.%s.%s.%s",t56,t57,t49,t50);
		else
			sprintf(Connection,"%s.%s.%s.%s",t49,t50,t56,t57);
	}
	for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
	{
		if(odb.AUXKEY_TAGS[i] && pfix->TagStr(odb.AUXKEY_TAGS[i]))
		{
			// DEV-12136: Default broker code behavior
#ifdef GS_RTECHOATS
			if((odb.AUXKEY_NAMES[i]=="RoutingBroker") && (strlen(pfix->TagStr(odb.AUXKEY_TAGS[i]))==0))
				sprintf_s(AuxKey,sizeof(AuxKey)-1,"%s%s%c",AuxKey,DefaultRouteBroker(isTicket,isAway),AUX_KEYS_DELIM);
			else
#endif
				sprintf_s(AuxKey,sizeof(AuxKey)-1,"%s%s%c",AuxKey,pfix->TagStr(odb.AUXKEY_TAGS[i]),AUX_KEYS_DELIM);
		}
	#ifdef AUXKEY_EXPR
		// Evaluate the expression indices
		else
		{
			if((mtype!='8')&&(mtype!='9')&&(mtype!='3'))	
			{
				if(odb.AUXKEY_ORD_EXPR[i].empty())
					sprintf_s(AuxKey,sizeof(AuxKey)-1,"%s%c",AuxKey,AUX_KEYS_DELIM);
				else
				{
					ExprTok etok;
					memset(etok.sval,0,sizeof(etok.sval));
					const char *ebeg=odb.AUXKEY_ORD_EXPR[i].c_str(),*eend=ebeg +strlen(ebeg);
					etok.EvalExpr(ebeg,eend,pfix);
					sprintf_s(AuxKey,sizeof(AuxKey)-1,"%s%s%c",AuxKey,etok.sval,AUX_KEYS_DELIM);
				}
			}
			if((mtype=='8')||(mtype=='9')||(mtype=='3'))	
			{
				if(odb.AUXKEY_EXEC_EXPR[i].empty())
					sprintf_s(AuxKey,sizeof(AuxKey)-1,"%s%c",AuxKey,AUX_KEYS_DELIM);
				else
				{
					ExprTok etok;
					memset(etok.sval,0,sizeof(etok.sval));
					const char *ebeg=odb.AUXKEY_EXEC_EXPR[i].c_str(),*eend=ebeg +strlen(ebeg);
					etok.EvalExpr(ebeg,eend,pfix);
					sprintf_s(AuxKey,sizeof(AuxKey)-1,"%s%s%c",AuxKey,etok.sval,AUX_KEYS_DELIM);
				}
			}
		}
	#endif
	}

	if(proto==PROTO_TWIST)
	{
		char ExecTransType=pfix->TagChar(20);
		char emsg[256]={0};
		char SideStr[2]={Side,0};
		// Correction
		if(ExecTransType=='2')
		{
			sprintf(emsg,"USC%d: !!!! TRADE CORRECTED [%s],[%s],[%s],[%s],[%s] - Call and Correct Manually(%s,%s,%.4f,%d)",
				UscPortNo,AppInstID,ClientID,Account,ClOrdID,pfix->TagStr(109),SideStr,Symbol,pfix->TagFloat(6),pfix->TagInt(32));
			if(TRADE_BUST_ERRORS) WSLogError(emsg);
			else WSLogEvent(emsg);
		}
		// Bust
		else if(ExecTransType=='1')
		{
			sprintf(emsg,"USC%d: !!!! TRADE BUSTED[%s],[%s],[%s],[%s],[%s] - Call and Correct Manually(%s,%s,%.4f,%d)",
				UscPortNo,AppInstID,ClientID,Account,ClOrdID,pfix->TagStr(109),SideStr,Symbol,pfix->TagFloat(6),pfix->TagInt(32));
			if(TRADE_BUST_ERRORS) WSLogError(emsg);
			else WSLogEvent(emsg);
		}
	}

	if(proto==PROTO_RTAWAY)
	{
		if((mtype!='D')&&(mtype!='G'))
			return 0;
		DropClient *pdc=0;
		int ConPortNo=(int)(LONGLONG)UscPort[UscPortNo].DetPtr4;
		if((ConPortNo>=0)&&(ConPortNo<NO_OF_CON_PORTS)&&(ConPort[ConPortNo].DetPtr==(void*)PROTO_FIXDROP))
			pdc=(DropClient *)ConPort[ConPortNo].DetPtr5;
		// Create drop client so we can queue up outgoing messages
		if(!pdc)
		{
			// Find the FIX client session
			DCMAP::iterator dit=dcmap.find(ConPortNo);
			if(dit==dcmap.end())
			{
				pdc=new DropClient;
				int UscPortNo=-1;
				char sess[256]={0};
				strcpy(sess,ConPort[ConPortNo].CfgNote +9);
				bool passive=false;
				char *sptr=strchr(sess,'$');
				if(sptr)
				{
					*sptr=0; UscPortNo=atoi(sptr+1);
				}
				if(strrcmp(sess,",PASSIVE"))
				{
					sess[strlen(sess) -8]=0;
					passive=true;
				}
				if(pdc->Init(sess,0,0,passive,this,(void*)(PTRCAST)MAKELONG(ConPortNo,WS_CON))<0)
				{
					delete pdc; pdc=0;
				}
				else
				{
					dcmap[ConPortNo]=pdc;
					ConPort[ConPortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;
					ConPort[ConPortNo].DetPtr5=pdc;
				}
			}
			else
				pdc=dit->second;
		}

		// Build the FIX message that goes to Redi Apollo away connection
		FIXINFO dfx;
		memset(&dfx,0,sizeof(FIXINFO));
		dfx.FIXDELIM=0x01;
		// Copy all 'pfix' tags to 'dfx'
		for(int i=0;i<pfix->ntags;i++)
		{
			int tno=pfix->tags[i].no;
			switch(tno)
			{
			case 9: tno=0; break;
			case 10: tno=0; break;
			case 34: tno=0; break;
			case 43: tno=0; break;
			case 52: tno=0; break;
			case 98: tno=0; break;
			case 101: tno=76; break;
			// Restore drop proxy tags moved to 2900 range and drop the 2900 tags
			case 2900: tno=0; break; // INCOMING or OUTGOING
			case 2901: tno=0; break; // iq domain
			case 2902: tno=0; break; // iq account
			case 2903: tno=0; break; // iq user
			case 2904: tno=0; break; // tag 34
			case 2905: tno=52; break;
			case 2906: tno=60; break;
			case 2907: tno=0; break; // tag 49
			case 2908: tno=50; break;
			case 2909: tno=0; break; // tag 56
			case 2910: tno=57; break;
			case 2911: tno=0; break; // tag 8
			};
			if(tno>0)
				dfx.SetTag(tno,&pfix->tags[i]);
		}

		// Use the doneaway translations for this client
		DoneAwayLookup *pdalu=0;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
			DAMAP::iterator dit=damap.find(dfile->prefix);
			if(dit!=damap.end())
				pdalu=&dit->second;
		}

		// Turn the new and replace order message into an execution
		dfx.SetTag(8,"FIX.4.2");
		// DoneAwayAccounts.csv
		const char *account=dfx.TagStr(pfix->GetTag(10600)?10600:1);
		if((pdalu)&&(TRANSLATE_DONE_AWAY_ACCOUNT))
		{
			map<string,string>::iterator dait=pdalu->doneAwayAccountMap.find(account);
			if(dait!=pdalu->doneAwayAccountMap.end())
				account=dait->second.c_str();
		}
		dfx.SetTag(1,account);
		dfx.SetTag(6,dfx.TagStr(44));
		#ifdef _DEBUG
		string clordid=dfx.TagStr(11);
		char cloid[32]={0};
		sprintf(cloid,"%s_%d",clordid.c_str(),pdc->oseq+1);
		dfx.SetTag(11,cloid);
		#endif
		dfx.SetTag(14,dfx.TagStr(38));
		char execid[32]={0};
		sprintf(execid,"%s-%d",dfx.GetTag(11)->val,pdc->oseq+1);
		dfx.SetTag(17,execid);
		//dfx.SetTag(17,dfx.GetTag(11));
		dfx.SetTag(20,'0');
		dfx.SetTag(31,dfx.TagStr(44));
		dfx.SetTag(32,dfx.TagStr(38));
		dfx.SetTag(35,'8');
		#ifdef _DEBUG
		char ordid[32]={0};
		sprintf(ordid,"%s.%d",clordid.c_str(),pdc->oseq+1);
		dfx.SetTag(37,ordid);
		#else
		dfx.SetTag(37,dfx.GetTag(11));
		#endif
		dfx.SetTag(39,'2');
		//DEV-4336: The price shows up in Message Monitor when we send 
		// as limit order executions with last price in price tag
		dfx.SetTag(40,'2');
		if(!dfx.GetTag(44))
			dfx.SetTag(44,'0.00');

		const char *user="";
		if(pdalu)
		{
			if(pdalu->DONE_AWAY_USER_TAG>0)
				user=dfx.TagStr(pdalu->DONE_AWAY_USER_TAG);
			else
			{
				// DoneAwayUsers.ini
				user=dfx.TagStr(50); // domain
				if(TRANSLATE_DONE_AWAY_USER)
				{
					map<string,string>::iterator duit=pdalu->doneAwayUserMap.find(user);
					if(duit==pdalu->doneAwayUserMap.end())
					{
						duit=pdalu->doneAwayUserMap.find("default");
						if(duit!=pdalu->doneAwayUserMap.end())
							user=duit->second.c_str();
					}
					else
						user=duit->second.c_str();
				}
			}
		}
		dfx.SetTag(50,user);
		dfx.SetTag(10500,user);

		dfx.SetTag(59,'0');
		if((!dfx.GetTag(109))&&(dfx.GetTag(57)))
			dfx.SetTag(109,dfx.GetTag(57));
		// DoneAway clearing
		if(stricmp(dfx.TagStr(100),"AWAY")!=0)
			dfx.SetTag(113,'Y'); // doneaway type Floor (to be cleared)
		else
			dfx.SetTag(113,'N'); // doneaway type Quick Insert
		dfx.SetTag(100,"AWAY");
		dfx.SetTag(150,'2');
		dfx.SetTag(151,'0');

		// SendFixMsg will encode and add to the FIX log, even when the session isn't connected
		if(pdc)
			pdc->SendFixMsg(dfx);
		else
		{
			char sstr[2]={dfx.TagChar(54),0};
			WSLogError("CON%d FIX session not found to drop (%s,%s,%s,%s,%s,%s,%.4f,%d)",ConPortNo,
				dfx.TagStr(50),dfx.TagStr(pfix->GetTag(10600)?10600:1),dfx.TagStr(11),dfx.TagStr(17),
				sstr,dfx.TagStr(55),dfx.TagFloat(31),dfx.TagInt(32));
		}
		return 0;
	}

	#ifndef NO_EFILLS_FILE
	if(!*ffpath)
		#ifdef WIN32
		sprintf(ffpath,"%s\\efills_%08d.fix",FILLS_FILE_DIR.c_str(),WSDate);
		#else
		sprintf(ffpath,"%s/efills_%08d.fix",FILLS_FILE_DIR.c_str(),WSDate);
		#endif
	#endif
	AddJournal(dj,pfix->fbuf,pfix->llen,doffset,dend,
			   AppInstID,ClOrdID,RootOrderID,
			   FirstClOrdID,ClParentOrderID,Symbol,
			   Price,Side,
			   Account,EcnOrderID,ClientID,
			   Connection,OrderDate,AuxKey,
			   mtype,etype,pfix->TagInt(38),pfix->TagInt(14),pfix->TagInt(151), 
			   Timestamp,OrigClOrdID,pfix->TagStr(17),pfix->TagInt(32),ffpath);

#ifdef HOLD_IQ_CHILD_ORDERS
	if((checkHeldDetails)&&(hm))
	{
		char sRootOrderID[40]={0};
		sprintf(sRootOrderID,"[%s]",ClOrdID);
		FixStats tstats;
		list<HeldDetail *> hlist;
		WaitForSingleObject(iqmux,INFINITE);
		for(multimap<string,HeldDetail *>::iterator hit=hm->find(sRootOrderID);
			(hit!=hm->end())&&(hit->first==sRootOrderID);
			#ifdef WIN32
			hit=hm->erase(hit))
			#else
			hm->erase(hit++))
			#endif
			hlist.push_front(hit->second); // invert LIFO order to FIFO
		ReleaseMutex(iqmux);
		for(list<HeldDetail *>::iterator hit=hlist.begin();hit!=hlist.end();hit++)
		{
			HeldDetail *phd=*hit;
			WSLockPort(WS_USC,phd->UscPortNo,false);
			HandleUscFixMsg(phd->pfix,phd->UscPortNo,&tstats,phd->doffset,phd->proto);
			WSUnlockPort(WS_USC,phd->UscPortNo,false);
			delete phd;
		}
		hlist.clear();
	}
#endif
	return 0;
}
int ViewServer::AddJournal(VSDistJournal *dj, const char *dptr, int dlen, LONGLONG doffset, LONGLONG dend,
						   const char *AppInstID, const char *ClOrdID, const char *RootOrderID,
						   const char *FirstClOrdID, const char *ClParentOrderID, const char *Symbol,
						   double Price, char Side,
						   const char *Account, const char *EcnOrderID, const char *ClientID,
						   const char *Connection, int OrderDate, const char *AuxKey,
						   char mtype, char etype, int OrderQty, int CumQty, int LeavesQty, 
						   LONGLONG Timestamp, const char *OrigClOrdID, const char *ExecID,
						   int LastQty, const char *ffpath)
{
	// Don't compact this encoding if it sacrifices debug-ability
	// It would have been nice to be able to use comma delimiter, but some orderids have commas in them.
	// TODO: Seems like there should be less error-prone way to do this
	char lbuf[1024]={0};
	char PriceStr[64]={0};
	sprintf(PriceStr,"%.4f",Price);
	char *decptr=strchr(PriceStr,'.');
	if(decptr)
	{
		for(char *pptr=decptr +strlen(decptr) -1;(pptr>decptr+2)&&(*pptr=='0');pptr--)
			*pptr=0;
	}
	char SideStr[2]={Side,0};
	switch(mtype)
	{
	case 'D': // New order
		#ifdef WIN32
		sprintf(lbuf,"new\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%d\1",
		#else
		sprintf(lbuf,"new\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%d\1",
		#endif
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
			OrderQty);
		dj->AddLine(-1,lbuf,dend); 
		break;
	case 'F': // Cancel request
		#ifdef WIN32
		sprintf(lbuf,"canc\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%s\1",
		#else
		sprintf(lbuf,"canc\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%s\1",
		#endif
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
			OrigClOrdID);
		dj->AddLine(-1,lbuf,dend); 
		break;
	case 'G': // Replace request
		#ifdef WIN32
		sprintf(lbuf,"repl\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%s\1%d\1",
		#else
		sprintf(lbuf,"repl\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%s\1%d\1",
		#endif
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
			OrigClOrdID,OrderQty);
		dj->AddLine(-1,lbuf,dend); 
		break;
	case '3': // Session reject
		#ifdef WIN32
		sprintf(lbuf,"srej\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1",
		#else
		sprintf(lbuf,"srej\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1",
		#endif
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey);
		dj->AddLine(-1,lbuf,dend); 
		break;
	case '8': // Execution
	{
		// Fills
		#ifndef NO_EFILLS_FILE
		if(LastQty>0)
		{
			Lock();
			if(!ffp)
			{
				ffp=fopen(ffpath,"wt");
				if(!ffp)
					WSLogError("Failed opening %s for write!",ffpath);
			}
			if(ffp)
				fwrite(dptr,1,dlen,ffp);
			Unlock();
		}
		#endif
		switch(etype)
		{
		case '0': // Confirmed
			#ifdef WIN32
			sprintf(lbuf,"conf\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%d\1",
			#else
			sprintf(lbuf,"conf\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%d\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				OrderQty);
			dj->AddLine(-1,lbuf,dend); 
			break;
		case '5': // Replaced
			#ifdef WIN32
			sprintf(lbuf,"rpld\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%d\1%s\1",
			#else
			sprintf(lbuf,"rpld\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%d\1%s\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				OrderQty,OrigClOrdID);
			dj->AddLine(-1,lbuf,dend); 
			break;
		case '1': // Part-fill
			#ifdef WIN32
			sprintf(lbuf,"fill\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%s\1%d\1%d\1%d\1%d\1",
			#else
			sprintf(lbuf,"fill\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%s\1%d\1%d\1%d\1%d\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				ExecID,LastQty,OrderQty,CumQty,LeavesQty);
			dj->AddLine(-1,lbuf,dend); 
			break;
		case '2': // Filled
			#ifdef WIN32
			sprintf(lbuf,"filled\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%s\1%d\1%d\1%d\1",
			#else
			sprintf(lbuf,"filled\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%08d\1%s\1%s\1%s\1%d\1%d\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				ExecID,LastQty,OrderQty,CumQty);
			dj->AddLine(-1,lbuf,dend); 
			break;
		case '3': // Done for day
			#ifdef WIN32
			sprintf(lbuf,"dfd\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%d\1",
			#else
			sprintf(lbuf,"dfd\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%d\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				CumQty);
			dj->AddLine(-1,lbuf,dend); 
			break;
		case '4': // Cancelled
			#ifdef WIN32
			sprintf(lbuf,"cxld\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%d\1%s\1",
			#else
			sprintf(lbuf,"cxld\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%d\1%s\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				CumQty,OrigClOrdID);
			dj->AddLine(-1,lbuf,dend); 
			// Previous order is cancelled too
			break;
		case '8': // Rejected
			#ifdef WIN32
			sprintf(lbuf,"rej\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%d\1",
			#else
			sprintf(lbuf,"rej\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%d\1",
			#endif
				AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
				CumQty);
			dj->AddLine(-1,lbuf,dend); 
			break;
		};
		break;
	}
	case '9': // Cancel or replace reject
		#ifdef WIN32
		sprintf(lbuf,"canrej\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%s\1",
		#else
		sprintf(lbuf,"canrej\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1%s\1",
		#endif
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
			OrigClOrdID);
		dj->AddLine(-1,lbuf,dend); 
		break;
#ifdef MULTI_RECTYPES
	case 0x04: // IQ Account
		sprintf(lbuf,"iqaccount\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1",
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey);
		dj->AddLine(-1,lbuf,dend); 
		break;
	case 0x05: // IQ Position
		sprintf(lbuf,"iqpos\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1%s\1%d\1%d\1%d\1",
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey,
			ExecID,LastQty,OrderQty,CumQty);
		dj->AddLine(-1,lbuf,dend); 
		break;
#endif
	default: // General detail reference
		#ifdef WIN32
		sprintf(lbuf,"det\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%I64d\1%s\1%I64d\1%d\1%s\1%08d\1%s\1",
		#else
		sprintf(lbuf,"det\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%lld\1%s\1%lld\1%d\1%s\1%08d\1%s\1",
		#endif
			AppInstID,ClOrdID,RootOrderID,FirstClOrdID,ClParentOrderID,Symbol,PriceStr,SideStr,Account,EcnOrderID,ClientID,Timestamp,dj->locAlias,doffset,dlen,Connection,OrderDate,AuxKey);
		dj->AddLine(-1,lbuf,dend); 
	};
	return 0;
}

// OrderDBNotify
void ViewServer::ODBError(int ecode, const char *emsg)
{
	_ASSERT(emsg);
	WSLogError(emsg);
}
void ViewServer::ODBEvent(int ecode, const char *emsg)
{
	_ASSERT(emsg);
	WSLogEvent(emsg);
}
#ifdef TEST_HASH_DIST
static unsigned long HashStr(const char *str)
{
	// Copied from GNU's hash_fun.h
	unsigned long __h = 0;
	for ( ; *str; ++str)
	__h = 5*__h + *str;
	return __h%(1024*1024);
}
#endif
#ifdef MULTI_DAY_HIST
void ViewServer::ODBIndexOrder(AppSystem *asys, OrderDB *pdb, VSOrder *porder, LONGLONG offset)
#else
void ViewServer::ODBIndexOrder(AppSystem *asys, VSOrder *porder, LONGLONG offset)
#endif
{
	_ASSERT((porder)&&(offset>=0));
	// Only need to index orders if there's no disk persistence
//#ifndef USE_DISK_INDEX
	const char *okey=porder->GetOrderKey();
	// Only count unique orders
	char ch1=toupper(okey[0]),ch2=toupper(okey[1]);
	if((ch1=='M')&&(ch2=='L'))
	{
		ch1=okey[2]; ch2=okey[3];
	}
	int ich1=26,ich2=26;
	if((ch1>='A')&&(ch1<='Z'))
		ich1=ch1 -'A';
	if((ch2>='A')&&(ch2<='Z'))
		ich2=ch2 -'A';
	ohist[ich1][ich2]++;
	#ifdef TEST_HASH_DIST
	unsigned long hval=HashStr(okey);
	hhist[hval]++;
	#endif
	#ifdef DEBUG_ORDER_KEYS
	if(okfp)
		#ifdef WIN32
		fprintf(okfp,"%s,%s,%I64d\n",porder->GetRootOrderID(),okey,offset);
		#else
		fprintf(okfp,"%s,%s,%lld\n",porder->GetRootOrderID(),okey,offset);
		#endif
	#endif

#ifndef MULTI_DAY_HIST
	OrderDB *pdb=&asys->odb;
#endif
	ITEMLOC iloc=offset;
#ifdef MULTI_DAY_ITEMLOC
	iloc.wsdate=pdb->wsdate;
#endif
	// For loading one order to debug
	//if(strcmp(okey,"HFEC0CD3088X"))
	//{
	//	pdb->FreeOrder(porder);
	//	return;
	//}
	// Don't store the 'porder' memory location pointer in the parent/child map, 
	// because it can be unloaded in the master order map.

	// For historic database, index all orders and positions by database date, but preserve the real order date 
	//if(pdb->HISTORIC_DAYS>0)
	//{
	//	int odate=porder->GetOrderDate();
	//	if(odate!=pdb->wsdate)
	//	{
	//		porder->SetOrderDate(pdb->wsdate);
	//		pdb->AssociateOrder(porder,offset,ocnt,pcnt);
	//		//porder->SetOrderDate(odate);
	//		//pdb->WriteOrder(porder,offset);
	//	}
	//}
	//else
		pdb->AssociateOrder(porder,iloc,ocnt,pcnt);
	// Add to AppInstance order set for query
	const char *AppInstID=porder->GetAppInstID();
	AppInstance *ainst=0;
	APPINSTMAP::iterator iit=asys->imap.find(AppInstID);
	if(iit==asys->imap.end())
	{
		WaitForSingleObject(asmux,INFINITE);
		WSLogError("AppInstance(%s) under AppSystem(%s) not found.",AppInstID,asys->sname.c_str());
		int icnt=0;
		ainst=asys->CreateInstance(AppInstID,icnt);
		if(ainst)
		{
			aimap[ainst->iname]=ainst;
			asys->PregenBrowse();
			WSLogEvent("Created AppInstance(%s) under AppSystem(UNKNOWN)",ainst->iname.c_str());
		}
		else
			WSLogError("Failed creating AppInstance(%s) under AppSystem(UNKNOWN)!",AppInstID);
		ReleaseMutex(asmux);
	}
	else
		ainst=iit->second;
	if(ainst)
	{
		ainst->oset.insert(iloc);
		if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(porder->GetAuxKey(),"AWAY",4)))
		{
			ainst->AcctAddOrderQty(porder->GetOrderQty());
			ainst->AcctAddCumQty(porder->GetCumQty());
			ainst->AcctAddFillQty(porder->GetFillQty());
		}
		ainst->AcctSetUpdateTime(WSTime);
		ainst->AcctAddMsgCnt(porder->GetNumDetails());
	}

	if(pdb->INDEX_FILLED_ORDERS)
	{
		if(porder->GetFillQty()>0)
		{
			VSINDEX::iterator eit=pdb->fomap.find(okey);
		#ifdef USE_DISK_INDEX
			eit=pdb->fomap.insert(okey,iloc);
		#else
			if(eit==pdb->fomap.end())
				eit=pdb->fomap.insert(okey,iloc);
			//else
			//	eit=pdb->fomap.insert(eit,okey,iloc);
		#endif
		}
	}

	string skey=okey;
	// Evict orders when they terminate
	if(porder->IsTerminated())
	{
		oterm++;
		pdb->FreeOrder(porder); porder=0;
	#ifdef NOMAP_TERM_ORDERS
		return;
	#endif
	}
	// Evict orders when our order cache limit exceeded
	else if(pdb->GetMemOrderSize()>=(pdb->ORDER_CACHE_SIZE*1024*1024))
	{
		pdb->FreeOrder(porder); porder=0;
	}
	pdb->omap.insert(skey,iloc);
	if(pdb->omap.size()%100==0)
		WSBusyThreadReport();
//#endif//USE_DISK_INDEX
}
void ViewServer::ODBBusyLoad(LONGLONG offset, int lperc)
{
	DWORD tnow=GetTickCount();
	if((tnow -lastBusyTick>=5000)||(lperc==100))
	{
		lastBusyTick=tnow;
		WSLogEvent("Loaded %s bytes (%d%%)...",SizeStr(offset),lperc);
	}
	WSBusyThreadReport();
}
void ViewServer::ODBOrderUnloaded(VSOrder *porder, LONGLONG offset)
{
	if(!UscPort)
		return;
	if(!odb.HAVE_FUSION_IO)
	{
		//WSLogEvent("DEBUG ODBOrderUnloaded(%s)",porder->GetOrderKey());
		//const char *okey=porder->GetOrderKey();
		//for(int i=0;i<NO_OF_USC_PORTS;i++)
		//{
		//	if(!UscPort[i].InUse)
		//		continue;
		//	if(UscPort[i].DetPtr==(void*)PROTO_VSDIST)
		//	{
		//		VSMruCache *pmru=(VSMruCache*)UscPort[i].DetPtr4;
		//		if(pmru)
		//			//pmru->RemItemByKey(okey);
		//			pmru->RemItemByVal(porder);
		//	}
		//}
		WaitForSingleObject(mrumux,INFINITE);
		for(list<VSMruCache*>::iterator mit=mrulist.begin();mit!=mrulist.end();mit++)
		{
			VSMruCache *pmru=*mit;
			pmru->RemItemByVal(porder);
		}
		ReleaseMutex(mrumux);
	}
}

// Returns number of milliseconds since start of day from localtime
DWORD ViewServer::GetPlayTime()
{
#ifdef WIN32
	SYSTEMTIME tloc;
	GetLocalTime(&tloc);
	DWORD rts=(tloc.wHour*3600000) +(tloc.wMinute*60000) +(tloc.wSecond*1000) +(tloc.wMilliseconds);
	return rts;
#else
	DWORD rts=::WSTime();
	return rts*1000;
#endif
}
// Returns number of milliseconds since start of day from hhmmss<mmm>
DWORD ViewServer::GetPlayTime(DWORD hms)
{
	DWORD rts=hms%1000; hms/=1000;
	rts+=(hms/10000)*3600000 +((hms%10000)/100)*60000 +(hms%100)*1000;
	return rts;
}
// Returns number of milliseconds since start of day from yyyymmdd-hh:mm:ss[.mmm]
DWORD ViewServer::GetPlayTime(const char *fixts)
{
	DWORD hms=myatoi(fixts+9)*10000000 +myatoi(fixts+12)*100000 +myatoi(fixts+15)*1000;
	if(fixts[17]=='.')
		hms+=myatoi(fixts+18);
	DWORD rts=GetPlayTime(hms);
	return rts;
}
// Returns hhmmssmmm from playback time
DWORD ViewServer::RevPlayTime(DWORD pts, char *pbuf)
{
	int ms=pts%1000; pts/=1000;
	int hh=pts/3600; pts-=(hh*3600);
	int mm=pts/60; pts-=(mm*60);
	int ss=pts; pts=0;
	if(pbuf)
		sprintf(pbuf,"%02d:%02d:%02d.%03d",hh,mm,ss,ms);
	DWORD hms=(hh*10000000) +(mm*100000) +(ss*1000) +ms;
	return hms;
}
// For testing from log files
int ViewServer::PlayMLLogs(int PortNo, int proto, const char *fmatch, bool& cxl, int rstart, float playRate)
{
	_ASSERT((PortNo>=0)&&(fmatch)&&(*fmatch));
	map<string,string> fmap;
	char fdir[MAX_PATH],*fname=0;
	GetFullPathNameEx(fmatch,MAX_PATH,fdir,&fname);
	if(fname) *fname=0;
#if defined(WIN32)||defined(_CONSOLE)
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(fmatch,&fdata);
	if(fhnd==INVALID_FILE_VALUE)
		return -1;
	do{
		if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
			;
		else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			;
		else
		{
			char fpath[MAX_PATH]={0},fkey[MAX_PATH]={0};
			sprintf(fpath,"%s%s",fdir,fdata.cFileName);

			//DT9044: A little help for the playbacks
			if(!WildCardCompare(fmatch, fpath))
				continue;

			// Ex: process orders before execs
			// exec_5005_101_20100803143158.672.itf_.1.processed
			// order_5005_101_201008031550010.795.itf_.1.processed
			if(proto==PROTO_ITF)
			{
				if(!strincmp(fdata.cFileName,"order_",5))
				{
					const char *dptr=strstr(fdata.cFileName,"2011");
					if(!dptr)
						dptr=strstr(fdata.cFileName,"2010");
					if(dptr)
					{
						// Ex: The files names from Unity were changed!
						// order_5008_200_1_20101130.itf_.1.2.processed
						char *fptr=fkey;
						if(dptr[8]=='.')
						{
							memcpy(fptr,dptr,8); fptr+=8; dptr+=8;
							FILETIME lft;
							FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&lft);
							SYSTEMTIME tsys;
							FileTimeToSystemTime(&lft,&tsys);
							sprintf(fptr,"%02d%02d%02d",tsys.wHour,tsys.wMinute,tsys.wSecond); fptr+=6;
						}
						else
						{
							memcpy(fptr,dptr,14); fptr+=14; dptr+=14;
						}
						sprintf(fptr,".order"); fptr+=strlen(fptr);
						const char *iptr=strstr(dptr,".itf");
						if(iptr)
						{
							int dlen=(int)(iptr -dptr);
							strncpy(fptr,dptr,dlen); fptr+=dlen; *fptr=0;
						}
					}
				}
				else if(!strincmp(fdata.cFileName,"exec_",4))
				{
					const char *dptr=strstr(fdata.cFileName,"2011");
					if(!dptr)
						dptr=strstr(fdata.cFileName,"2010");
					if(dptr)
					{
						char *fptr=fkey;
						// Ex: The files names from Unity were changed!
						// exec_5008_345_1_20101130.itf_.1.2.processed
						if(dptr[8]=='.')
						{
							memcpy(fptr,dptr,8); fptr+=8; dptr+=8;
							SYSTEMTIME tsys;
							FileTimeToSystemTime(&fdata.ftLastWriteTime,&tsys);
							sprintf(fptr,"%02d%02d%02d",tsys.wHour,tsys.wMinute,tsys.wSecond); fptr+=6;
						}
						else
						{
							memcpy(fptr,dptr,14); fptr+=14; dptr+=14;
						}
						sprintf(fptr,".orxc"); fptr+=strlen(fptr);
						const char *iptr=strstr(dptr,".itf");
						if(iptr)
						{
							int dlen=(int)(iptr -dptr);
							strncpy(fptr,dptr,dlen); fptr+=dlen; *fptr=0;
						}
					}
				}
				if(!fkey[0])
					strcpy(fkey,fdata.cFileName);
			}
			// parent orders before child orders
			else if(proto==PROTO_IQOMS)
			{
				if(!strincmp(fdata.cFileName,"parent",6))
					sprintf(fkey,"1.%s",fdata.cFileName);
				else if(!strincmp(fdata.cFileName,"child",5))
					sprintf(fkey,"2.%s",fdata.cFileName);
				if(!fkey[0])
					strcpy(fkey,fdata.cFileName);
			}
			// Ex: .10 should be processed after .9
			// qsa-order_UNITY10.order.002008.21940.20100603.9
			// qsa-order_UNITY10.order.002008.21940.20100603.10
			else
			{
				strcpy(fkey,fdata.cFileName);
				char *nptr=strrchr(fkey,'.');
				if(nptr)
					sprintf(nptr+1,"%02d",atoi(nptr +1)); // to sort the extension number correctly
			}
			fmap[fkey]=fpath;
		}
	}while(FindNextFile(fhnd,&fdata));
	FindClose(fhnd);
#else//!WIN32
	DIR *pdir=opendir(fdir);
	if(!pdir)
		return -1;
	dirent *fdata=0;
	do{
		fdata=readdir(pdir);
		if(fdata)
		{
			if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
				continue;
			char fpath[MAX_PATH]={0},fkey[MAX_PATH]={0};
			sprintf(fpath,"%s%s",fdir,fdata->d_name);
			struct stat fst;
			stat(fdata->d_name,&fst);
			if(fst.st_mode&S_IFDIR)
				continue;

			//DT9044: A little help for the playbacks
			if(!WildCardCompare(fmatch, fpath))
				continue;

			// Ex: process orders before execs
			// exec_5005_101_20100803143158.672.itf_.1.processed
			// order_5005_101_201008031550010.795.itf_.1.processed
			if(proto==PROTO_ITF)
			{
				if(!strincmp(fdata->d_name,"order_",5))
				{
					char sdate[5];
					strncpy(sdate,WScDate,4); sdate[4]=0;
					const char *dptr=strstr(fdata->d_name,sdate);
					if(dptr)
					{
						char *fptr=fkey;
						memcpy(fptr,dptr,14); fptr+=14; dptr+=14;
						sprintf(fptr,".order"); fptr+=strlen(fptr);
						const char *iptr=strstr(dptr,".itf");
						if(iptr)
						{
							int dlen=(int)(iptr -dptr);
							strncpy(fptr,dptr,dlen); fptr+=dlen; *fptr=0;
						}
					}
				}
				else if(!strincmp(fdata->d_name,"exec_",4))
				{
					char sdate[5];
					strncpy(sdate,WScDate,4); sdate[4]=0;
					const char *dptr=strstr(fdata->d_name,sdate);
					if(dptr)
					{
						char *fptr=fkey;
						memcpy(fptr,dptr,14); fptr+=14; dptr+=14;
						sprintf(fptr,".orxc"); fptr+=strlen(fptr);
						const char *iptr=strstr(dptr,".itf");
						if(iptr)
						{
							int dlen=(int)(iptr -dptr);
							strncpy(fptr,dptr,dlen); fptr+=dlen; *fptr=0;
						}
					}
				}
				if(!fkey[0])
					strcpy(fkey,fdata->d_name);
			}
			// parent orders before child orders
			else if(proto==PROTO_IQOMS)
			{
				if(!strincmp(fdata->d_name,"parent",6))
					sprintf(fkey,"1.%s",fdata->d_name);
				else if(!strincmp(fdata->d_name,"child",5))
					sprintf(fkey,"2.%s",fdata->d_name);
				if(!fkey[0])
					strcpy(fkey,fdata->d_name);
			}
			// Ex: .10 should be processed after .9
			// qsa-order_UNITY10.order.002008.21940.20100603.9
			// qsa-order_UNITY10.order.002008.21940.20100603.10
			else if((proto==PROTO_MLFIXLOGS)||(proto==PROTO_MLPUBHUB)||(proto==PROTO_MLPH_CC))
			{
				strcpy(fkey,fdata->d_name);
				char *nptr=strrchr(fkey,'.');
				if(nptr)
					sprintf(nptr+1,"%02d",atoi(nptr +1)); // to sort the extension number correctly
			}
			else
			{
				strcpy(fkey,fdata->d_name);
			}
			fmap[fkey]=fpath;
		}
	}while(fdata);
	closedir(pdir);
#endif//!WIN32

	if(rstart>0)
		WSLogEvent("CON%d: Realtime playback relative to %06d...",PortNo,rstart);
	DWORD pstart=GetPlayTime(); // real playback start time
	DWORD vstart=GetPlayTime(rstart*1000); // virtual playback start time
	DWORD vtime=vstart; // current virtual time
	DWORD lnext=vstart; // next eventlog virtual time
	int fcnt=0;
	for(map<string,string>::iterator fit=fmap.begin();(fit!=fmap.end())&&(!cxl);fit++)
	{
		const char *fpath=fit->second.c_str();
		FILE *fp=fopen(fpath,"rb");
		if(fp)
		{
			fcnt++;
			fseeko(fp,0,SEEK_END);
			int fsize=ftell(fp);
			fseeko(fp,0,SEEK_SET);
			int lwtime=0;
		#ifdef WIN32
			HANDLE fhnd=FindFirstFile(fpath,&fdata);
			if(fhnd!=INVALID_FILE_VALUE)
			{
				FindClose(fhnd);
				SYSTEMTIME tsys;
				FileTimeToSystemTime(&fdata.ftLastWriteTime,&tsys);
				lwtime=tsys.wHour*10000 +tsys.wMinute*100 +tsys.wSecond;
			}
		#else
			struct stat fst;
			stat(fpath,&fst);
			struct tm *ltm=localtime(&fst.st_ctime);
			if(ltm)
				lwtime=ltm->tm_hour*10000 +ltm->tm_min*100 +ltm->tm_sec;
		#endif
			WSLogEvent("CON%d: Playing %s (%s bytes) (lastmod %06d)...",PortNo,fpath,SizeStr(fsize),lwtime);

			// Gap debugging only
			DropClient dc;
			dc.ScanLastSeqNos(fpath);
			int i;
			for(i=0;i<dc.iseq;i++)
			{
				DropClient::INBLOCK gap;
				if(dc.GetGap(gap,i)>0)
					WSLogEvent("Gap detected [%d,%d]",gap.begin,gap.end);
				else
					break;
			}
			if(!i)
				WSLogEvent("No gaps detected");

			// Maximum timestamp to wait for during realtime playback
			DWORD fmax=0;
			if(rstart>0)
			{
				if(proto==PROTO_IQOMS)
					fmax=GetPlayTime(161500000); // 16:15 EDT
				else
				{
					#ifdef WIN32
					const char *fname=strrchr(fpath,'\\');
					#else
					const char *fname=strrchr(fpath,'/');
					#endif
					if(fname) fname++;
					else fname=fpath;
					const char *dptr=strstr(fname,"2011");
					if(!dptr)
						dptr=strstr(fname,"2010");
					if(dptr)
					{
						// Ex: The files names from Unity were changed!
						// exec_5008_345_1_20101130.itf_.1.2.processed
						if(dptr[8]=='.')
						{
							DWORD fts=lwtime *1000;
							fmax=GetPlayTime(fts) +3600000; // 1 hour after file time
						}
						else
						{
							DWORD fts=myatoi(dptr +8)*1000;
							fmax=GetPlayTime(fts) +3600000; // 1 hour after file time
						}
					}
				}
			}

			char rbuf[4096]={0};
		#if defined(MULTIPLY_FIX)||defined(MULTI_DAY_REPLAY)
			char mbuf[4096]={0};
		#endif
			int lno=0;
			while(fgets(rbuf,sizeof(rbuf),fp)&&(!cxl))
			{
				lno++;
				const char *rptr;
				for(rptr=rbuf;*rptr==' ';rptr++)
					;
				if((!*rptr)||(*rptr=='\r')||(*rptr=='\n'))
					continue;
				if(!strncmp(rptr,"//",2))
					continue;
				while((ConPort[PortNo].SockConnected)&&(!cxl)&&
					((ConPort[PortNo].OutBuffer.Size +ConPort[PortNo].IOCPSendBytes>=ConPort[PortNo].OutBuffer.MaxSize*1000)|| // throttle on port buffer
					(ConPort[PortNo].SendTimeOut>0))) // send "Retry" condition
					SleepEx(50,true);
				int rlen=(int)strlen(rptr);
				if((!ConPort[PortNo].SockConnected)||(ConPort[PortNo].appClosed))
				{
					WSLogEvent("CON%d: Playback aborted",PortNo);
					return -1;
				}
				if(proto==PROTO_ITF)
				{
					const char *iend=rptr +rlen -1;
					const char *iptr;
					for(iptr=iend -2;(iptr>rptr)&&(*iptr!=0x1d);iptr--)
						;
					if(iptr<=rptr)
					{
						WSLogError("CON%d: Skipping line %d",PortNo,lno);
						continue;
					}
					iptr++;
					// Move rptr to begin of FIX message, but send the entire ITF 'rbuf'
					//rlen=(int)(iend -iptr);
					//if(rlen>=sizeof(rbuf))
					//{
					//	_ASSERT(false);
					//	rlen=sizeof(rbuf) -1;
					//}
					//memmove(rbuf,iptr,rlen); strcpy(rbuf +rlen -1,"\n");
					rptr=iptr;
				}
			#if defined(_DEBUG)&&defined(FIX_SERVER)
				// Only for testing TJM playback to active FIX port
				//if(proto==PROTO_TJM)
				//{
				//	rptr=strchr(rptr,':');
				//	if(rptr) rptr=strchr(rptr+1,':');
				//	if(rptr) rptr=strchr(rptr+1,':');
				//	rptr++;
				//	rlen=strlen(rptr) -1;
				//}
			#endif
				static const char t60str[5]={0x01,'6','0','=',0};
				// Realtime throttle
				if(rstart>0)
				{
					DWORD rts=GetPlayTime();
					DWORD rdiff=rts -pstart;
					vtime=vstart +(int)(rdiff*playRate);
					if(vtime>lnext)
					{
						char vbuf[32]={0};
						RevPlayTime(vtime,vbuf);
						WSLogEvent("CON%d: Playback virtual time %s",PortNo,vbuf);
						while(lnext<vtime)
							lnext+=(int)(1000*playRate);
					}
					DWORD fts=0;
					// Play by OMS header time
					if(proto==PROTO_IQOMS)
					{
						fts=GetPlayTime(atoi(rbuf+2)*1000);						
					}
					// Play by tag 60
					else
					{
						const char *t60=strstr(rptr,t60str);
						if(t60)
							fts=GetPlayTime(t60 +4);						
					}
					if(fts>0)
					{
						char fbuf[32]={0};
						RevPlayTime(fts,fbuf);
						if((fts>=fmax)&&(vtime<fts))
							WSLogEvent("CON%d: Not waiting for FIX msg time %s that is out of realtime range",PortNo,fbuf);
						// If virtual time < fix msg time, then wait
						while((fts<fmax)&&(vtime<fts))
						{
							SleepEx(1,true);
							rts=GetPlayTime();
							rdiff=rts -pstart;
							vtime=vstart +(int)(rdiff*playRate);
							if(vtime>lnext)
							{
								char vbuf[32]={0};
								RevPlayTime(vtime,vbuf);
								WSLogEvent("CON%d: Playback virtual time %s, next FIX msg at %s",PortNo,vbuf,fbuf);
								while(lnext<vtime)
									lnext+=(int)(1000*playRate);
							}
						}
					}
				}
			#ifdef MULTI_DAY_REPLAY
				int mmdd=0;
				const char *dptr=strstr(rbuf,t60str);
				if(dptr)
					mmdd=myatoi(dptr+8);
				char *mptr=mbuf;
				for(const char *rptr=rbuf;*rptr;)
				{
					if(*rptr==0x01)
					{
						*mptr=*rptr; mptr++; rptr++;
						int tlen=0;
						if(!strncmp(rptr,"11=",3))
							tlen=3;
						else if(!strncmp(rptr,"37=",3))
							tlen=3;
						else if(!strncmp(rptr,"39=",3))
							tlen=3;
						else if(!strncmp(rptr,"41=",3))
							tlen=3;
						else if(!strncmp(rptr,"5055=",5))
							tlen=5;
						else if(!strncmp(rptr,"70129=",6))
							tlen=6;
						if(tlen>0)
						{
							memcpy(mptr,rptr,tlen); mptr+=tlen; rptr+=tlen;
							sprintf(mptr,"%04d.",mmdd); mptr+=5;
						}
					}
					for(;(*rptr)&&(*rptr!=0x01);rptr++)
					{
						*mptr=*rptr; mptr++;
					}
					_ASSERT(mptr -mbuf<sizeof(mbuf));
				}
				*mptr=0;
				WSConSendBuff((int)(mptr -mbuf),mbuf,PortNo,1);
			#else
				WSConSendBuff(rlen,(char*)rptr,PortNo,1);
			#endif
				FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
				if(cstats)
				{
					cstats->msgCnt++; cstats->totMsgCnt++;
				}
			#ifdef MULTIPLY_FIX
				for(int m=0;m<MULTIPLY_FIX-1;m++)
				{
					char *mptr=mbuf;
					for(const char *rptr=rbuf;*rptr;)
					{
						bool addsfx=false;
						if(*rptr==0x01)
						{
							*mptr=*rptr; mptr++; rptr++;
							if((!strncmp(rptr,"11=",3))||
							(!strncmp(rptr,"41=",3))||
							(!strncmp(rptr,"5055=",5))||
							(!strncmp(rptr,"37=",5)))
								addsfx=true;
						}
						for(;(*rptr)&&(*rptr!=0x01);rptr++)
						{
							*mptr=*rptr; mptr++;
						}
						if(addsfx)
						{
							sprintf(mptr,"M%d",m+2); mptr+=strlen(mptr);
						}

					}
					*mptr=0;
					WSConSendBuff((int)(mptr -mbuf),mbuf,PortNo,1);
				}
			#endif
			}
			fclose(fp);
		}
////BAOHACK first two files only for MLFIXLOGS to play 2M msgs
//if((fcnt>=10)&&(stristr(fpath,"UNITY")))
//	break;
	}
	// Threads that initiate overlapped sends should not exit before send completion.
	// Otherwise, if no subsequent sends are issued (to get WSAECONNRESET), 
	// then those overlapped operations will never complete!
	if((ConPort[PortNo].SockConnected)&&(ConPort[PortNo].NumOvlSends>0))
	{
		WSLogEvent("CON%d: Waiting for send completion...",PortNo);
		while((ConPort[PortNo].SockConnected)&&(ConPort[PortNo].IOCPSendBytes>0))
			SleepEx(100,true);
	}
	WSLogEvent("CON%d: Playback complete",PortNo);
////BAOHACK auto-chain playback
//if(PortNo<10)
//	ConPort[PortNo+1].ConnectHold=false;
	return 0;
}
// For testing from full log files
int ViewServer::PlayTraderLogs(int PortNo, int proto, const char *fdir, const char *fmatch, bool& cxl, int rstart, float playRate)
{
#if defined(WIN32)||defined(_CONSOLE)
	WIN32_FIND_DATA fdata;
	char pmatch[MAX_PATH]={0};
	sprintf(pmatch,"%s\\%s",fdir,fmatch);
	// BFS
	HANDLE fhnd=FindFirstFile(pmatch,&fdata);
	if(fhnd!=INVALID_FILE_VALUE)
	{
		do{
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				;
			else
			{
				char fpath[MAX_PATH]={0};
				sprintf(fpath,"%s\\%s",fdir,fdata.cFileName);
				PlayMLLogs(PortNo,proto,fpath,cxl,rstart,playRate);
			}
		} while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}

	sprintf(pmatch,"%s\\*.*",fdir);
	fhnd=FindFirstFile(pmatch,&fdata);
	if(fhnd!=INVALID_FILE_VALUE)
	{
		do{
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				char spath[MAX_PATH]={0};
				sprintf(spath,"%s\\%s",fdir,fdata.cFileName);
				PlayTraderLogs(PortNo,proto,spath,fmatch,cxl,rstart,playRate);
			}
		} while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
#else
	DIR *pdir=opendir(fdir);
	if(pdir)
	{
		dirent *fdata=0;
		do{
			fdata=readdir(pdir);
			if(fdata)
			{
				if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
					continue;
				char fpath[MAX_PATH]={0};
				sprintf(fpath,"%s/%s",fdir,fdata->d_name);
				PlayMLLogs(PortNo,proto,fpath,cxl,rstart,playRate);
			}
		}while(fdata);
		closedir(pdir);
	}

	pdir=opendir(fdir);
	if(pdir)
	{
		dirent *fdata=0;
		do{
			fdata=readdir(pdir);
			if(fdata)
			{
				if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
					continue;
				char spath[MAX_PATH]={0};
				sprintf(spath,"%s/%s",fdir,fdata->d_name);
				PlayTraderLogs(PortNo,proto,spath,fmatch,cxl,rstart,playRate);
			}
		}while(fdata);
		closedir(pdir);
	}
#endif
	return 0;
}

#ifdef IQSMP
int ViewServer::PlaySmpLogs(int PortNo, int proto, const char *fdir, const char *fmatch, bool& cxl, int rstart, float playRate)
{
	#if defined(WIN32)||defined(_CONSOLE)
	WIN32_FIND_DATA fdata;
	char pmatch[MAX_PATH]={0};
	sprintf(pmatch,"%s\\%s",fdir,fmatch);
	// BFS
	HANDLE fhnd=FindFirstFile(pmatch,&fdata);
	if(fhnd!=INVALID_FILE_VALUE)
	{
		do{
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				;
			else
			{
				char fpath[MAX_PATH]={0};
				sprintf(fpath,"%s\\%s",fdir,fdata.cFileName);
				FILE *fp=fopen(fpath,"rb");
				if(!fp)
				{
					WSLogError("Failed opening %s!",fpath);
					continue;
				}
				WSLogEvent("Playing %s...",fpath);
				char rbuf[4096]={0};
				int rbytes,tbytes=0;
			//#ifdef _DEBUG
				// Only for testing active FIX replay
				if((proto==PROTO_TWIST)||(proto==PROTO_RTECHOATS))
				{
					while((fgets(rbuf,sizeof(rbuf),fp)))
					{
						if(!strstr(rbuf,"I:"))
							continue;
						if(/*(strstr(rbuf,"35=2"))||(strstr(rbuf,"35=3"))||(strstr(rbuf,"35=4"))||*/(strstr(rbuf,"35=5")))
							continue;
						rbytes=strlen(rbuf) +2;
						const char *rptr=strchr(rbuf,':');
						if(rptr) rptr=strchr(rptr +1,':');
						if(rptr) rptr=strchr(rptr +1,':');
						if(rptr) rptr++;
						WSConSendBuff(strlen(rptr) -2,(char*)rptr,PortNo,1); tbytes+=rbytes;
					}
				}
			#ifdef FIX_SERVER
				else if(proto==PROTO_IQFIXS)
				{
					while((fgets(rbuf,sizeof(rbuf),fp)))
					{
						// Only for testing FIXSERVER playback to active FIX port
						//if(!strstr(rbuf,"INLOG"))
						//	continue;
						rbytes=strlen(rbuf) +2;
						const char *rptr=strchr(rbuf,':');
						if(rptr) rptr=strchr(rptr +1,':');
						if(rptr) rptr++;
						int rlen=strlen(rptr) -2;
						//for(char *dptr=(char*)rptr;dptr<rptr+rlen;dptr++)
						//{
						//	if(*dptr=='|') *dptr=0x01;
						//}
						//WSConSendBuff(rlen,(char*)rptr,PortNo,1); tbytes+=rbytes;
						FIXINFO ifx;
						memset(&ifx,0,sizeof(FIXINFO));
						ifx.FIXDELIM='|';
						rlen=ifx.FixMsgReady((char*)rptr,rlen);
						if(rlen>0)
						{
							char t35=ifx.TagChar(35);
							const char *t57=ifx.TagStr(57);
							int& oseq=(int&)ConPort[PortNo].DetPtr6;
							// Only for testing FIXSERVER playback to active FIX port
							//if((((oseq==0)&&(t35=='A'))||(t35=='D')||(t35=='F')||(t35=='G'))&&(*t57))
							{
								ifx.SetTag(34,++oseq);
								char tstr[32]={0};
								sprintf(tstr,"%08d-%02d:%02d:%02d",WSDate,WSTime/10000,(WSTime%10000)/100,WSTime%100);
								ifx.SetTag(52,tstr);
								ifx.FIXDELIM=0x01;
								ifx.Merge();
								WSConSendBuff(ifx.llen,ifx.fbuf,PortNo,1); tbytes+=ifx.llen;
								delete ifx.sbuf; ifx.sbuf=0; ifx.ssize=0;
							}
						}
					}
				}
			#endif
				else
			//#endif
				{
					while((rbytes=fread(rbuf,1,sizeof(rbuf),fp))>0)
					{
						WSConSendBuff(rbytes,rbuf,PortNo,1); tbytes+=rbytes;
					}
				}
				fclose(fp);
				WSLogEvent("Played %s bytes from %s.",SizeStr(tbytes),fpath);
			}
		} while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
	else
		WSLogError("No files matched %s!",pmatch);
	#endif
	return 0;
}
#endif

int ViewServer::PlayCoverageToolExport(int PortNo, int proto, const char *fdir, const char *fmatch, bool& cxl, int rstart, float playRate)
{
#if defined(WIN32)||defined(_CONSOLE)
	WIN32_FIND_DATA fdata;
	char pmatch[MAX_PATH]={0};
	sprintf(pmatch,"%s\\%s",fdir,fmatch);
	// BFS
	HANDLE fhnd=FindFirstFile(pmatch,&fdata);
	if(fhnd!=INVALID_FILE_VALUE)
	{
		do{
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				;
			else
			{
				char fpath[MAX_PATH]={0};
				sprintf(fpath,"%s\\%s",fdir,fdata.cFileName);
				PlayCoverageToolExport(PortNo,fpath,cxl);
			}
		} while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
#endif
	return 0;
}
static const char *GetField(vector<const char *>& row, map<string,int>& headers, const char *field)
{
	map<string,int>::iterator hit=headers.find(field);
	if(hit==headers.end())
		return "";
	int col=hit->second;
	if(col>=(int)row.size())
		return "";
	const char *val=row[col];
	if(!strcmp(val,"null"))
		return "";
	return val;
}
// Translate the .csv to FIX
int ViewServer::PlayCoverageToolExport(int PortNo, const char *fpath, bool& cxl)
{
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
		return -1;
	int lno=0;
	char rbuf[2048]={0};
	map<string,int> headers;
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		vector<const char *> row;
		for(char *tok=strtoke(rbuf,",");tok;tok=strtoke(0,","))
		{
			for(char *tptr=tok+strlen(tok);(tptr>tok)&&(isspace(tptr[-1]));tptr--)
				tptr[-1]=0;
			row.push_back(tok);
		}
		// Allow columns to be in any order
		if((headers.empty())&&(!_stricmp(row[0],"Time")))
		{
			for(int col=0;col<(int)row.size();col++)
				headers[row[col]]=col;
			continue;
		}

		char fbuf[2048]={0},*fptr=fbuf;
		sprintf(fptr,"8=FIX.4.2\0019=000\00135=8\00149=IQDROPFUT\00156=REDIRPT\00134=%d\001",lno); 
		fptr+=strlen(fptr);

		const char *transTime=GetField(row,headers,"Time");
		strcpy(fptr,"60="); fptr+=3;
		memcpy(fptr,transTime,4); fptr+=4;
		memcpy(fptr,transTime+5,2); fptr+=2;
		memcpy(fptr,transTime+8,2); fptr+=2;
		fptr[0]='-'; fptr++;
		memcpy(fptr,transTime+11,8); fptr+=8;
		fptr[0]=0x01; fptr++;

		sprintf(fptr,"50=%s\00110500=%s\001",GetField(row,headers,"User"),GetField(row,headers,"User")); 
		fptr+=strlen(fptr);

		sprintf(fptr,"55=%s\001",GetField(row,headers,"Symbol")); 
		fptr+=strlen(fptr);

		const char *side=GetField(row,headers,"Side");
		char *sidestr="";
		if(!_stricmp(side,"Buy"))
			sidestr="1";
		else if(!_stricmp(side,"Sell"))
			sidestr="2";
		else if(!_stricmp(side,"Sell Short"))
			sidestr="5";
		sprintf(fptr,"54=%s\001",sidestr); 
		fptr+=strlen(fptr);

		const char *prtype=GetField(row,headers,"Price Type");
		const char *prtypestr="";
		if(!_stricmp(prtype,"Market"))
			prtypestr="1";
		else if(!_stricmp(prtype,"Limit"))
			prtypestr="2";
		else if(!_stricmp(prtype,"Pegged"))
			prtypestr="1";
		else if(!_stricmp(prtype,"Stop"))
			prtypestr="1";
		else if(!_stricmp(prtype,"StopLimit"))
			prtypestr="2";
		sprintf(fptr,"40=%s\001",prtypestr); 
		fptr+=strlen(fptr);

		const char *limitprice=GetField(row,headers,"Limit Price");
		if((limitprice[0])&&(strcmp(limitprice,"0.0")))
		{
			sprintf(fptr,"44=%s\001",limitprice); 
			fptr+=strlen(fptr);
		}

		sprintf(fptr,"38=%d\001",atoi(GetField(row,headers,"Orig Qty"))); 
		fptr+=strlen(fptr);

		sprintf(fptr,"15=USD\001"); 
		fptr+=strlen(fptr);

		const char *execprice=GetField(row,headers,"Exec Prc");
		if((execprice[0])&&(strcmp(execprice,"0.0")))
		{
			sprintf(fptr,"6=%.04f\001",atof(execprice)); 
			fptr+=strlen(fptr);
		}

		const char *execqty=GetField(row,headers,"Exec Qty");
		if((execqty[0])&&(strcmp(execqty,"0.0")))
		{
			sprintf(fptr,"14=%d\001",atoi(execqty)); 
			fptr+=strlen(fptr);
		}

		const char *leavesqty=GetField(row,headers,"Lvs");
		if((leavesqty[0])&&(strcmp(leavesqty,"0.0")))
		{
			sprintf(fptr,"151=%d\001",atoi(leavesqty)); 
			fptr+=strlen(fptr);
		}

		const char *strategy=GetField(row,headers,"Algo Name");
		if((strategy[0])&&(strcmp(strategy,"NONE")))
		{
			sprintf(fptr,"847=%s\001",strategy); 
			fptr+=strlen(fptr);
		}

		const char *status=GetField(row,headers,"Status");
		const char *statusstr="";
		if(!_stricmp(status,"Complete"))
			statusstr="2";
		else if(!_stricmp(status,"Canceled"))
			statusstr="4";
		else if(!_stricmp(status,"PartComplete"))
			statusstr="1";
		else if(!_stricmp(status,"Open"))
			statusstr="0";
		else if(!_stricmp(status,"Expired"))
			statusstr="3";
		else if(!_stricmp(status,"Partial"))
			statusstr="1";
		else if(!_stricmp(status,"PendCancel"))
			statusstr="6";
		else if(!_stricmp(status,"Rejected"))
			statusstr="8";
		sprintf(fptr,"150=%s\001",statusstr); 
		fptr+=strlen(fptr);

		const char *exchange=GetField(row,headers,"Exchange");
		if((!_stricmp(exchange,"MBRK"))||(!exchange[0]))
			exchange=GetField(row,headers,"Prim Exch");
		sprintf(fptr,"100=%s\001",exchange); 
		fptr+=strlen(fptr);

		sprintf(fptr,"109=%s%03d\001",GetField(row,headers,"Branch Code"),atoi(GetField(row,headers,"Branch Sequence"))); 
		fptr+=strlen(fptr);

		sprintf(fptr,"1=%s\001",GetField(row,headers,"Account")); 
		fptr+=strlen(fptr);

		const char *broker=GetField(row,headers,"Broker Code");
		if((broker[0])&&(strcmp(broker,"NA")))
		{
			sprintf(fptr,"76=%s\001",broker); 
			fptr+=strlen(fptr);
		}

		char prodtype[128]={0};
		strcpy(prodtype,GetField(row,headers,"Prod Type"));
		_strupr(prodtype);
		sprintf(fptr,"1595=%s\001",prodtype); 
		fptr+=strlen(fptr);
		if(!stricmp(prodtype,"FUTURE"))
		{
			sprintf(fptr,"207=FUTURE\001"); 
			fptr+=strlen(fptr);
		}

		sprintf(fptr,"10583=%s\001",GetField(row,headers,"Region Group")); 
		fptr+=strlen(fptr);
		
		sprintf(fptr,"11=%04d%02d%02d%s%s%s\001",
			atoi(transTime),atoi(transTime+5),atoi(transTime+8),
			GetField(row,headers,"Core Id (OMS)"),GetField(row,headers,"Line Id (OMS)"),GetField(row,headers,"Line Seq (OMS)")); 
		fptr+=strlen(fptr);

		const char *refkey=GetField(row,headers,"Ref Key");
		const char *rkptr=strchr(refkey,'-');
		if(rkptr)
		{
			// There are orders from the next day (odd/even) in this file
			int tdate=(atoi(transTime)*10000)+(atoi(transTime+5)*100)+(atoi(transTime+8));
			int rdate=atoi(rkptr+1);
			if(rdate!=tdate)
				continue;
		}
		sprintf(fptr,"37=%s\001",refkey);
		fptr+=strlen(fptr);

		sprintf(fptr,"17=%s-%d\001",refkey,lno);
		fptr+=strlen(fptr);

		sprintf(fptr,"58=%s\001",GetField(row,headers,"Security Desc")); 
		fptr+=strlen(fptr);

		sprintf(fptr,"143=%s\001",GetField(row,headers,"Prim Exch")); 
		fptr+=strlen(fptr);

		const char *tif=GetField(row,headers,"Time-In-Force");
		const char *tifstr="";
		if(!_stricmp(tif,"CUSTOM"))
			tifstr="";
		else if(!_stricmp(tif,"DAY"))
			tifstr="0";
		else if(!_stricmp(tif,"GTC"))
			tifstr="1";
		else if(!_stricmp(tif,"IOC"))
			tifstr="3";
		sprintf(fptr,"59=%s\001",tifstr); 
		fptr+=strlen(fptr);

		sprintf(fptr,"10=0\001");
		fptr+=strlen(fptr);

		int len=0;
		const char *t35=strstr(fbuf,"35=");
		if(t35)
			len=strlen(t35);
		char lstr[8]={0};
		sprintf(lstr,"%03d",len);
		char *t9=strstr(fbuf,"9=");
		if(t9)
			memcpy(t9+2,lstr,3);

		ExpandCoverageToolOrder(PortNo,fbuf,(int)(fptr -fbuf));
	}
	fclose(fp);
	return 0;
}
// Fill in life-cycle messages
int ViewServer::ExpandCoverageToolOrder(int PortNo, char *fbuf, int flen)
{
	FIXINFO ifix;
	ifix.FIXDELIM=0x01;
	ifix.noSession=1;
	ifix.Reset();
	flen=ifix.FixMsgReady(fbuf,flen);
	if(flen<1)
		return -1;
	char t150=ifix.TagChar(150);
	// Nothing needed before confirmed and rejected events
	if((t150=='0')||(t150=='8'))
		return 0;

	// Generate a confirm
	char t17[128]={0};
	int t17sub=0;
	int cqty=0,lqty=ifix.TagInt(38);

	FIXINFO cfix;
	cfix.Copy(&ifix,true);
	cfix.SetTag(150,'0');
	sprintf(t17,"%s-%d",cfix.TagStr(17),++t17sub);
	cfix.SetTag(17,t17);
	cfix.SetTag(14,cqty);
	cfix.SetTag(151,lqty);
	cfix.Merge();
	WSConSendBuff(cfix.slen,cfix.sbuf,PortNo,0);
	WSConSendBuff(2,"\r\n",PortNo,1);

	// Generate intermediate part-fills
	int cumqty=ifix.TagInt(14);
	int fincr=100;
	if(cumqty<10)
		fincr=1;
	else if(cumqty<100)
		fincr=10;
	int filltot=cumqty;
	if((t150=='1')||(t150=='2'))
	{
		if(filltot>=fincr)
			filltot-=fincr;
		else
		{
			fincr=filltot;
			filltot=0;
		}
	}
	while(cqty<filltot)
	{
		FIXINFO ffix;
		ffix.Copy(&ifix,true);
		ffix.SetTag(150,'1');
		int fqty=fincr;
		if(cqty +fqty>cumqty)
			fqty=cumqty -cqty;
		ffix.SetTag(31,ifix.TagStr(6));
		ffix.SetTag(32,fqty);
		cqty+=fqty;
		ffix.SetTag(14,cqty);
		lqty-=fqty;
		ffix.SetTag(151,lqty);
		sprintf(t17,"%s-%d",ffix.TagStr(17),++t17sub);
		ffix.SetTag(17,t17);
		ffix.Merge();
		WSConSendBuff(ffix.slen,ffix.sbuf,PortNo,0);
		WSConSendBuff(2,"\r\n",PortNo,1);
	}
	if((t150=='1')||(t150=='2'))
	{
		ifix.fbuf=0;
		int fqty=fincr;
		if(cqty +fqty>cumqty)
			fqty=cumqty -cqty;
		ifix.SetTag(31,ifix.TagStr(6));
		ifix.SetTag(32,fqty);
		cqty+=fqty;
		ifix.SetTag(14,cqty);
		lqty-=fqty;
		ifix.SetTag(151,lqty);
		ifix.Merge();
	}
	WSConSendBuff(ifix.llen,ifix.fbuf,PortNo,0);
	WSConSendBuff(2,"\r\n",PortNo,1);
	return 0;
}

void ViewServer::VSDNotifyError(int rc, const char *emsg)
{
	WSLogError(emsg);
}
void ViewServer::VSDNotifyEvent(int rc, const char *emsg)
{
	WSLogEvent(emsg);
}
// Finds or creates an order in memory only
VSOrder *ViewServer::CreateOrder(int UscPortNo, AppSystem *asys, const char *okey, bool& isnew, ITEMLOC& oloc)
{
	isnew=false;
	VSOrder *porder=0;
#ifndef TASK_THREAD_POOL
	if(!odb.HAVE_FUSION_IO)
	{
		// Check MRU cache first
		VSMruCache *pmru=(VSMruCache*)UscPort[UscPortNo].DetPtr4;
		if(pmru)
		{
			char mrukey[256]={0};
			sprintf(mrukey,"%s.%s",asys->sname.c_str(),okey);
			porder=pmru->GetItem(mrukey);
			if(porder)
			{
				oloc=porder->GetOffset();
				_ASSERT(oloc>=0);
				_ASSERT(porder->GetOrderKey()[0]);
				return porder;
			}
		}
	}
#endif
	// Only use the map when it's not in the MRU
	OrderDB *pdb=asys ?&asys->odb :&this->odb;
	VSINDEX::iterator oit=pdb->omap.find(okey);
	// New order
	if(oit==pdb->omap.end())
	{
		isnew=true;
		porder=pdb->AllocOrder();
		if(!porder)
		{
			Unlock();
			WSLogError("Failed allocating order(%s)!",okey);
			return 0;
		}
		porder->SetOrderKey(okey);
		// Only count unique orders
		char ch1=toupper(okey[0]),ch2=toupper(okey[1]);
		int ich1=26,ich2=26;
		if((ch1>='A')&&(ch1<='Z'))
			ich1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			ich2=ch2 -'A';
		ohist[ich1][ich2]++;
	}
	#ifndef NO_DB_WRITES
	// Existing order
	else
	{
		// Read from database if not in memory
		porder=pdb->ReadOrder(oit.second);
		if(!porder)
		{
			Unlock();
			#ifdef WIN32
			WSLogError("Failed reading order(%s) from loc(%I64d)",okey,(LONGLONG)oit.second);
			#else
			WSLogError("Failed reading order(%s) from loc(%lld)",okey,(LONGLONG)oit.second);
			#endif
			return 0;
		}
		oloc=oit.second;
	}
	#endif
	_ASSERT(porder->GetOrderKey()[0]);
	return porder;
}
int ViewServer::CommitOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc)
{
	Lock();
	const char *okey=porder->GetOrderKey();
	_ASSERT(okey[0]);
	OrderDB *pdb=asys ?&asys->odb :&this->odb;
	ITEMLOC offset=porder->GetOffset();
#ifdef NO_DB_WRITES
	pdb->omap.insert(okey,offset);
#else
#ifdef VALIDATE_DBWRITES
	// Prevent writing an order from a different system
	if(asys->imap.find(porder->GetAppInstID())==asys->imap.end())
	{
		WSLogError("Prevented write of order(%s) into wrong system(%s)!",okey,asys->sname.c_str());
		Unlock();
		return -1;
	}
#endif
	// For new orders, make sure to return the committed location
	if(offset<(ITEMLOC)0)
	{
		if(pdb->WriteOrder(porder,offset)<0)
		{
			#ifdef WIN32
			WSLogError("Failed committing order(%s) to loc(%I64d)",okey,(LONGLONG)offset);
			#else
			WSLogError("Failed committing order(%s) to loc(%lld)",okey,(LONGLONG)offset);
			#endif
			Unlock();
			return -1;
		}
		oloc=offset;
		pdb->omap.insert(okey,offset);
	}
	// Commit existing order
	else
	{
		if(porder->IsDirty())
		{
			if(pdb->WriteOrder(porder,offset)<0)
			{
				#ifdef WIN32
				WSLogError("Failed committing order(%s) to loc(%I64d)",okey,(LONGLONG)offset);
				#else
				WSLogError("Failed committing order(%s) to loc(%lld)",okey,(LONGLONG)offset);
				#endif
				Unlock();
				return -1;
			}
		}
		_ASSERT(oloc==offset);
	}
#endif
	// Add to AppInstance order set for query
	const char *AppInstID=porder->GetAppInstID();
	APPINSTMAP::iterator iit=asys->imap.find(AppInstID);
	if(iit!=asys->imap.end())
	{
		AppInstance *ainst=iit->second;
		ainst->oset.insert(oloc);
	}
#ifndef TASK_THREAD_POOL
	if(!odb.HAVE_FUSION_IO)
	{
		// Add to the MRU
		if((UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS))
		{
			VSMruCache *pmru=(VSMruCache*)UscPort[UscPortNo].DetPtr4;
			if((pmru)&&(porder))
			{
				char mrukey[256]={0};
				sprintf(mrukey,"%s.%s",asys->sname.c_str(),okey);
				pmru->AddItem(mrukey,porder);
			}
		}
	}
#endif
	Unlock();
	return 0;
}
int ViewServer::UnloadOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc)
{
	OrderDB *pdb=asys ?&asys->odb :&this->odb;
	pdb->FreeOrder(porder);
	return 0;
}
// 'okey' should include date prefix like 'yyyymmdd.<clordid>'
// 'wsdate' is the order database date
int ViewServer::ModifyOrder(const char *appinstid, const char *okey, int wsdate,
		const char *account, const char *clientid, const char *symbol,
		const char *nappinstid, const char *newaccount, const char *newrootorderid, const char *newfirstclordid, 
		const char *newsymbol)
{
	if((!appinstid)||(!okey))
		return -1;
	AppInstance *ainst=GetAppInstance(appinstid);
	if((!ainst)||(!ainst->asys))
		return -1;
	if(!wsdate)
		wsdate=WSDATE ?WSDATE :WSDate;
	OrderDB *sdb=ainst->asys->GetDB(wsdate);
	if(!sdb)
		sdb=ainst->asys->GetDB(WSDate);
	if(!sdb)
		return -1;

	// All orders in the appinst for the date
	if(!strcmp(okey,"*"))
	{
		list<string> olist;
		for(VSINDEX::iterator oit=sdb->omap.begin();oit!=sdb->omap.end();oit++)
		{
			okey=oit.first.c_str();
			if(odb.HISTORIC_DAYS>0)
			{
				int odate=atoi(okey);
				if((wsdate)&&(odate!=wsdate))
					continue;
			}
			olist.push_back(okey);
		}
		int rc=0;
		for(list<string>::iterator sit=olist.begin();sit!=olist.end();sit++)
		{
			if(ModifyOrder(appinstid,sit->c_str(),wsdate,
				account,clientid,symbol,
				nappinstid,newaccount,newrootorderid,newfirstclordid,
				newsymbol)>0)
				rc++;
		}
		return rc;
	}
	// Single order
	VSINDEX::iterator oit=sdb->omap.find(okey);
	if(oit==sdb->omap.end())
		return -1;

	AppInstance *nainst=0;
	if((nappinstid)&&(*nappinstid))
	{
		nainst=GetAppInstance(nappinstid);
		if(!nainst)
			return -1;
	}
	else
		nainst=ainst;

	VSOrder *porder=sdb->ReadOrder(oit.second);
	if(!porder)
		return -1;

	// Match filters
	// The order must currently be associated with old instance specified
	if(stricmp(porder->GetAppInstID(),appinstid))
		return -1;
	if((account)&&(*account)&&(stricmp(porder->GetAccount(),account)))
		return -1;
	if((clientid)&&(*clientid)&&(stricmp(porder->GetClientID(),clientid)))
		return -1;
	if((symbol)&&(*symbol)&&(stricmp(porder->GetSymbol(),symbol)))
		return -1;

	bool save=false;
	ITEMLOC oloc=porder->GetOffset();
	// Change app instance
	if(nainst!=ainst)
	{
		// Must be within same system for now
		if(nainst->asys!=ainst->asys)
			return -1;
		OrderDB *ndb=nainst->asys->GetDB(wsdate);
		if(!ndb)
			ndb=nainst->asys->GetDB(WSDate);
		if(!ndb)
			return -1;
		ITEMLOCSET::iterator oit=ainst->oset.find(oloc);
		if(oit!=ainst->oset.end())
			ainst->oset.erase(oit);
		porder->SetAppInstID(nappinstid); save=true;
		// CommitOrder will associate it to the new instance
		//nainst->oset.insert(oloc);
	}
	// Change account
	if((newaccount)&&(*newaccount))
	{
		if(stricmp(porder->GetAccount(),newaccount))
		{
			sdb->DisAssociateAccount(porder);
			porder->SetAccount(newaccount); save=true;
			sdb->AssociateAccount(porder,oloc);
		}
	}
	// Change parent tree
	if((newrootorderid)&&(*newrootorderid))
	{
		if(stricmp(porder->GetRootOrderID(),newrootorderid))
		{
			sdb->DisAssociateRootOrderID(porder);
			porder->SetRootOrderID(newrootorderid); save=true;
			sdb->AssociateRootOrderID(porder,oloc);
		}
	}
	// Change cancel/replace chain
	if((newfirstclordid)&&(*newfirstclordid))
	{
		if(stricmp(porder->GetFirstClOrdID(),newfirstclordid))
		{
			sdb->DisAssociateFirstClOrdID(porder);
			porder->SetFirstClOrdID(newfirstclordid); save=true;
			sdb->AssociateFirstClOrdID(porder,oloc);
		}
	}
	// Change symbol (i.e. to match OATS or clearing firm symbology)
	if((newsymbol)&&(*newsymbol))
	{
		if(stricmp(porder->GetSymbol(),newsymbol))
		{
			sdb->DisAssociateSymbol(porder);
			porder->SetSymbol(newsymbol); save=true;
			sdb->AssociateSymbol(porder,oloc);
		}
	}
	if(save)
		CommitOrder(-1,nainst->asys,porder,oloc);
	sdb->FreeOrder(porder); porder=0;
	return 1;
}
// Journal page transfer
void ViewServer::VSDNotifyJPageRequest(void *udata, DWORD rid, JPAGE *jpage, bool retransmit)
{
	if(!udata || !jpage)
	{
		WSLogError("VSDNotifyJPageRequest(), udata or jpage is null");
		return;
	}
	_ASSERT(VSDIST_HUB);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS))
		return;	
	//#ifdef _DEBUG
	//WSLogEvent("USR%d: VSDNotifyJPageRequest(%d,%d,%d,%d)",PortNo,rid,jpage->lstart,jpage->lend,retransmit);
	//#endif
	VSDistCodec *vsd=(VSDistCodec *)UsrPort[PortNo].DetPtr6;
	if(!vsd)
		return;
	// Stats
	int UscPortNo=UsrPort[PortNo].UscPort;
	FixStats *pstats=(FixStats *)UscPort[UscPortNo].DetPtr3;
	if(!pstats)
		return;
	int& lstart=(int&)UsrPort[PortNo].DetPtr4;
	int& lend=(int&)UsrPort[PortNo].DetPtr5;
	// If we already got the whole page, skip to the ack
	if((lstart>0)&&(jpage->lend<lstart))
	{
	#ifndef RETRANS_JOURNAL
		WSLogEvent("Previously ACKed page [%d,%d] < [%d,%d] re-ACKed.",jpage->lstart,jpage->lend,lstart,lend);
	#endif
		goto send_ack;
	}
	if((!lstart)||(jpage->lstart<lstart))
		lstart=jpage->lstart;
	if(jpage->lend>lend)
		lend=jpage->lend;

	{//scope
	char *pptr=jpage->page +jpage->pbeg;
	char *args[32];
	for(int lidx=jpage->lstart;lidx<=jpage->lend;lidx++)
	{
		char *nptr=pptr +strlen(pptr) +1;
	#ifdef MULTI_DAY_REPLAY
		if(pstats->totMsgCnt>=maxFixMsgs)
			break;
	#endif
		pstats->msgCnt++; pstats->totMsgCnt++;
		// Parse message
		char *tok=strtoke(pptr,"\1");
		if(!tok)
			continue;
		const char *mtype=tok;
		int aidx=0;
		memset(args,0,sizeof(args));
		for(tok=strtoke(0,"\1");tok;tok=strtoke(0,"\1"))
			args[aidx++]=tok;
		_ASSERT(aidx<=32);

		//DT9044: Handle IQ AccountRecs
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
#ifndef MULTI_RECTYPES
	#ifdef MULTI_DAY
		if((dfile->proto==PROTO_CLBACKUP) && args[17] && (strcmp(args[17],VSDB_ACCOUNTREC_IND)==0))
	#else
		if((dfile->proto==PROTO_CLBACKUP) && args[16] && (strcmp(args[16],VSDB_ACCOUNTREC_IND)==0))
	#endif
		{
			bool result=ClBackupAccountRec(PortNo,aidx,args,lidx);
			pptr=nptr;
			continue;
		}
#endif

		// 9 common order arguments
		if(aidx<9)
		{
			WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
			pptr=nptr;
			continue;
		}
		int cidx=0;
		char *AppInstID=args[cidx++];
		if((!AppInstID)||(!*AppInstID))
		{
			WSLogError("USR%d: Missing 'AppInstID' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		_strupr(AppInstID);
		char *ClOrdID=args[cidx++];
		char FakeClOrdID[256]={0};
		if((!ClOrdID)||(!*ClOrdID))
		{
			sprintf(FakeClOrdID,"UNK_%s_%d",AppInstID,++UnkClOrdIDSeqNo);
			ClOrdID=FakeClOrdID;
			// Sometimes, a session reject will also trigger this error
			WSLogError("USR%d: Missing 'ClOrdID' on journal entry(%d)! Assigned(%s)",PortNo,lidx,FakeClOrdID);
			//pptr=nptr;
			//continue;
		}
		if(!odb.INDEX_CLORDID_CS) //DT9491
			_strupr(ClOrdID);
		char *RootOrderID=args[cidx++];
		if(!RootOrderID)
		{
			WSLogEvent("USR%d: Missing 'RootOrderID' on journal entry(%d).",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_ROOTORDERID_CS) //DT9491
			_strupr(RootOrderID);
		char *FirstClOrdID=args[cidx++];
		if(!FirstClOrdID)
		{
			WSLogEvent("USR%d: Missing 'FirstClOrdID' on journal entry(%d).",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_FIRSTCLORDID_CS) //DT9491
			_strupr(FirstClOrdID);
		char *ClParentOrderID=args[cidx++];
		if(!ClParentOrderID)
		{
			WSLogEvent("USR%d: Missing 'ClParentOrderID' on journal entry(%d).",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_CLPARENTORDERID_CS) //DT9491
			_strupr(ClParentOrderID);
		char *Symbol=args[cidx++];
		if(!Symbol)
		{
			WSLogError("USR%d: Missing 'Symbol' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_SYMBOL_CS) //DT9491
			_strupr(Symbol);
		char *PriceStr=args[cidx++];
		if(!PriceStr)
		{
			WSLogError("USR%d: Missing 'Price' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		double Price=atof(PriceStr);
		char *Side=args[cidx++];
		if(!Side)
		{
			WSLogError("USR%d: Missing 'Side' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		_strupr(Side);
		char *Account=args[cidx++];
		if(!Account)
		{
			WSLogError("USR%d: Missing 'Account' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_ACCOUNT_CS) //DT9491
			_strupr(Account);
		char *EcnOrderID=args[cidx++];
		if(!EcnOrderID)
		{
			WSLogError("USR%d: Missing 'EcnOrderID' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_ECNORDERID_CS) //DT9491
			_strupr(EcnOrderID);
		char *ClientID=args[cidx++];
		if(!ClientID)
		{
			WSLogError("USR%d: Missing 'ClientID' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_CLIENTID_CS) //DT9491
			_strupr(ClientID);
		char *TransactTimeStr=args[cidx++];
		if(!TransactTimeStr)
		{
			WSLogError("USR%d: Missing 'TransactTime' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		LONGLONG TransactTime=myatoi64(TransactTimeStr);
	#ifdef IQSMP
		//if(strlen(TransactTimeStr)==14))
		//	TransactTime*=1000; // Add milliseconds
		// Remove milliseconds to reduce number of time indices
		int tslen=strlen(TransactTimeStr);
		if(tslen>14)
		{
			for(int d=14;d<tslen;d++)
				TransactTime/=10;
		}
	#endif
		char *locAlias=args[cidx++];
		if((!locAlias)||(!*locAlias))
		{
			WSLogError("USR%d: Missing 'locAlias' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		_strupr(locAlias);
		if(strlen(locAlias)>15)
		{
			WSLogError("USR%d: 'locAlias' must not exceed 15 characters on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if((!args[cidx])||(!*args[cidx]))
		{
			WSLogError("USR%d: Missing 'doff' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		LONGLONG doff=myatoi64(args[cidx++]);
		if(doff<0)
		{
			#ifdef WIN32
			WSLogError("USR%d: Invalid doff(%I64d) on journal entry(%d)!",PortNo,doff,lidx);
			#else
			WSLogError("USR%d: Invalid doff(%lld) on journal entry(%d)!",PortNo,doff,lidx);
			#endif
			pptr=nptr;
			continue;
		}
		if((!args[cidx])||(!*args[cidx]))
		{
			WSLogError("USR%d: Missing 'dlen' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		int dlen=myatoi(args[cidx++]);
		if(dlen<0)
		{
			WSLogError("USR%d: Invalid dlen(%d) on journal entry(%d)!",PortNo,dlen,lidx);
			pptr=nptr;
			continue;
		}
		char *Connection=args[cidx++];
		if(!Connection)
		{
			WSLogError("USR%d: Missing 'Connection' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_CONNECTIONS_CS) //DT9491
			_strupr(Connection);
		int OrderDate=myatoi(args[cidx++]);
		if(OrderDate</*19800101*/0)
		{
			WSLogError("USR%d: Missing 'OrderDate' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		//DT9398
		char *AuxKey=args[cidx++];
		if(!AuxKey)
		{
			WSLogError("USR%d: Missing 'AuxKey' on journal entry(%d)!",PortNo,lidx);
			pptr=nptr;
			continue;
		}
		if(!odb.INDEX_AUXKEYS_CS) //DT9491
			_strupr(AuxKey);
		char okey[256]={0};
	#ifdef MULTI_DAY
		if(odb.INDEX_ORDERDATE)
			sprintf(okey,"%08d.%s",OrderDate,ClOrdID);
		else
			strcpy(okey,ClOrdID);
	#else
		strcpy(okey,ClOrdID);
	#endif

	#ifdef REDIOLEDBCON
		// Allow moving REDIRPT and REDIDW orders into domain instances
		char OverrideAppInstID[64]={0};
		if(!TACMAP_PREFIX.empty())
		{
			sprintf(OverrideAppInstID,"%s%s",TACMAP_PREFIX.c_str(),AppInstID);
			AppInstID=OverrideAppInstID;
		}
		TACCTMAP::iterator ait=tacmap.find(Account);
		if(ait!=tacmap.end())
		{
			string domain=ait->second.first;
			//if(!stricmp(AppInstID,"$RTPOSITION"))
			//	sprintf(OverrideAppInstID,"$%s",domain.c_str());
			//else
			//	strcpy(OverrideAppInstID,domain.c_str());
			sprintf(OverrideAppInstID,"%s%s",TACMAP_PREFIX.c_str(),domain.c_str());
			//_strupr(OverrideAppInstID);
			AppInstID=OverrideAppInstID;
		}
		char ckey[32]={0};
		strcpy(ckey,ClientID);
		_strupr(ckey);
		TUSERMAP::iterator uit=tusermap.find(ckey);
		if(uit!=tusermap.end())
		{
			string domain=uit->second;
			sprintf(OverrideAppInstID,"%s%s",TACMAP_PREFIX.c_str(),domain.c_str());
			AppInstID=OverrideAppInstID;
		}
	#endif

		bool isnew=false,fill=false,acct=false;
		AppSystem *asys=GetAppSystem(AppInstID);
		if(!asys)
		{
			WaitForSingleObject(asmux,INFINITE);
			// Assume 2-character instances are SORT
			if(strlen(AppInstID)==2)
			{
				WSLogError("App system for (%s) not found--Assuming SORT.",AppInstID);
				asys=GetAppSystem("SORT*");
				if(!asys)
				{
					WSLogError("USR%d: AppSystem(%s) for (%s) not found!",PortNo,"SORT",AppInstID);
					ReleaseMutex(asmux);
					pptr=nptr;
					continue;
				}
			}
			else
			{
				WSLogError("AppSystem for AppInstance(%s) not found.",AppInstID);
				asys=GetAppSystem("UNKNOWN*");
				if(!asys)
				{
					WSLogError("USR%d: AppSystem(%s) for (%s) not found!",PortNo,"UNKNOWN",AppInstID);
					ReleaseMutex(asmux);
					pptr=nptr;
					continue;
				}
			}
			int icnt=0;
			AppInstance *ainst=asys->CreateInstance(AppInstID,icnt);
			if(ainst)
			{
				aimap[ainst->iname]=ainst;
				asys->PregenBrowse();
				WSLogEvent("Created AppInstance(%s) under AppSystem(UNKNOWN)",ainst->iname.c_str());
			}
			ReleaseMutex(asmux);
		}

		// Minimize mutex complexity by locking out one system at a time
		OrderDB *pdb=&asys->odb;
		pdb->Lock();
		AppInstance *ainst=0;
		APPINSTMAP::iterator aiit=asys->imap.find(AppInstID);
		if(aiit==asys->imap.end())
		{
			pdb->Unlock();
			WSLogError("USR%d: AppInstance(%s) of (%s) not found!",PortNo,AppInstID,asys->sname.c_str());
			pptr=nptr;
			continue;
		}
		ainst=aiit->second;
		ainst->AcctAddMsgCnt(1);
		ainst->AcctSetUpdateTime(WSTime);
		// DEV-22005: Override with session start date
		if(OrderDate!=pdb->wsdate)
		{
			OrderDate=pdb->wsdate;
			if(odb.INDEX_ORDERDATE)
				sprintf(okey,"%08d.%s",OrderDate,ClOrdID);
		}
		ITEMLOC oloc=-1;
		VSOrder *porder=CreateOrder(UscPortNo,asys,okey,isnew,oloc);
		if(!porder)
		{
			pdb->Unlock();
			WSLogError("USR%d: Failed CreateOrder(%s)",PortNo,okey);
			pptr=nptr;
			continue;
		}
		// Allow addition of missing key values, but don't change a key once it's indexed
		bool needAccount=*porder->GetAccount()?false:true;
		bool needFirstClOrdID=*porder->GetFirstClOrdID()?false:true;
		bool needClParentOrderID=*porder->GetClParentOrderID()?false:true;
		bool needEcnOrderID=*porder->GetEcnOrderID()?false:true;
		bool needClientID=*porder->GetClientID()?false:true;
		bool needTransactTime=porder->GetTransactTime()?false:true;
	#ifdef MULTI_DAY
		bool needOrderDate=porder->GetOrderDate()?false:true;
	#endif
		bool needAuxKey=porder->NeedAuxKey(pdb->INDEX_AUXKEYS); //DT9398
		if(isnew)
		{
			porder->SetAppInstID(AppInstID);
			porder->SetClOrdID(ClOrdID);
			porder->SetRootOrderID(RootOrderID);
			porder->SetFirstClOrdID(FirstClOrdID);
			porder->SetClParentOrderID(ClParentOrderID);
			porder->SetSymbol(Symbol);
			porder->SetPrice(Price);
			porder->SetSide(Side[0]);
			porder->SetAccount(Account);
			porder->SetEcnOrderID(EcnOrderID);
			porder->SetClientID(ClientID);
			porder->SetTransactTime(TransactTime);
			porder->SetConnection(Connection);
		#ifdef MULTI_DAY
			porder->SetOrderDate(OrderDate);
		#endif
			porder->SetAuxKeys(AuxKey,pdb->INDEX_AUXKEYS); //DT9398 
		}
		else
		{
			if(needAccount)
				porder->SetAccount(Account);
			if(needFirstClOrdID)
				porder->SetFirstClOrdID(FirstClOrdID);
			if(needClParentOrderID)
				porder->SetClParentOrderID(ClParentOrderID);
			if(needEcnOrderID)
				porder->SetEcnOrderID(EcnOrderID);
			if(needClientID)
				porder->SetClientID(ClientID);
			if(needTransactTime)
				porder->SetTransactTime(TransactTime);
		#ifdef MULTI_DAY
			if(needOrderDate)
				porder->SetOrderDate(OrderDate);
		#endif
			if(needAuxKey)
				porder->SetAuxKeys(AuxKey,pdb->INDEX_AUXKEYS); //DT9398 
		}

		//DT9044 - Check for updated detail
		if(dfile && dfile->proto==PROTO_CLBACKUP)
		{
			if(ClBackupCheckForDetailUpdate(pdb,porder,UscPortNo,dfile,AppInstID,ClOrdID,AuxKey,asys,ainst,acct,oloc,doff))
			{
				pdb->Unlock();
				pptr=nptr;
				continue;
			}
		}

		int didx=porder->AddDetail(doff,UscPortNo,dlen);
		if(didx<0)
		{
			VSDetExt *pde=pdb->AllocDetExt();
			if(pde)
			{
				strncpy(pde->OrderKey,porder->GetOrderKey(),sizeof(pde->OrderKey));
				pde->OrderKey[sizeof(pde->OrderKey)]=0;
				pde->dirty=true;
				LONGLONG eoff=-1;
				pdb->WriteDetExt(pde,eoff);
				porder->AddDetExt(pde);
				didx=porder->AddDetail(doff,UscPortNo,dlen);
			}
		}
		if(didx<0)
		{
			pdb->Unlock();
			#ifdef WIN32
			WSLogError("USR%d: Failed adding detail offset(%I64d) to order(%s)",PortNo,doff,okey);
			#else
			WSLogError("USR%d: Failed adding detail offset(%lld) to order(%s)",PortNo,doff,okey);
			#endif
			pptr=nptr;
			continue;
		}

	//#ifdef _DEBUG
	//	// Re-read and validate the detail
	//	// This slows down the process immensely and should only be used for debugging
	//	VSDetailFile *dfile=(VSDetailFile *)UscPort[UscPortNo].DetPtr1;
	//	if(dfile->proto==PROTO_ITF)
	//	{
	//		char *dptr=new char[dlen+2];
	//		pdb->ReadDetail(dfile,doff-1,dptr,dlen+1);
	//		if(dptr[0]!=0x1d || !isdigit(dptr[1]))
	//		{
	//			_ASSERT(false);
	//			pdb->Unlock();
	//			WSLogError("USR%d: Corrupt detail offset(%I64d) on order(%s)",PortNo,doff,okey);
	//			pptr=nptr;
	//			continue;
	//		}
	//		FIXINFO ifix;
	//		ifix.FIXDELIM=0x01;
	//		ifix.noSession=true;
	//		ifix.supressChkErr=true;
	//		ifix.FixMsgReady(dptr+1,dlen);
	//		const char *cloid=ifix.TagStr(11);
	//		if(!*cloid)
	//		{
	//			_ASSERT(false);
	//			pdb->Unlock();
	//			WSLogError("USR%d: Corrupt detail offset(%I64d) on order(%s)",PortNo,doff,okey);
	//			pptr=nptr;
	//			continue;
	//		}
	//		delete dptr; dptr=0;
	//	}
	//#endif

		// Message arguments
		if(!strcmp(mtype,"new"))
		{
			// Args
			if(aidx -cidx!=1)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetPrice(Price);
			int OrderQty=myatoi(args[cidx++]);
			porder->SetOrderQty(OrderQty);
			porder->SetHighMsgType('D');
			// Stats
			// Redi doesn't drop order requests
			//if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			//{
			//	ainst->AcctAddOrderQty(porder->GetOrderQty()); acct=true;
			//}
			pstats->totFix35_D++;
			const char *okey=ClOrdID;
			int klen=(int)strlen(okey);
			pstats->totKeyLen+=klen; pstats->cntKeyLen++;
			if(klen>(int)pstats->maxKeyLen)
				pstats->maxKeyLen=klen;
		}
		else if(!strcmp(mtype,"canc"))
		{
			// Args
			if(aidx -cidx!=1)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			char *OrigClOrdID=args[cidx++];
			if(!odb.INDEX_CLORDID_CS) //DT9491
				_strupr(OrigClOrdID);
			porder->SetHighMsgType('F');
			#ifdef SPECTRUM
			porder->SetHighExecType('6');
			#endif
			// Stats
			pstats->totFix35_F++;
			const char *okey=ClOrdID;
			int klen=(int)strlen(okey);
			pstats->totKeyLen+=klen; pstats->cntKeyLen++;
			if(klen>(int)pstats->maxKeyLen)
				pstats->maxKeyLen=klen;
			// Do we need to associate with OrigClOrdID?
		}
		else if(!strcmp(mtype,"repl"))
		{
			// Args
			if(aidx -cidx!=2)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetPrice(Price);
			char *OrigClOrdID=args[cidx++];
			if(!odb.INDEX_CLORDID_CS) //DT9491
				_strupr(OrigClOrdID);
			int OrderQty=myatoi(args[cidx++]);
			porder->SetOrderQty(OrderQty);
			porder->SetHighMsgType('G');
			#ifdef SPECTRUM
			porder->SetHighExecType('E');
			#endif
			// Stats
			// Redi doesn't drop order requests
			//if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			//{
			//	ainst->AcctAddOrderQty(porder->GetOrderQty()); acct=true;
			//}
			pstats->totFix35_G++;
			const char *okey=ClOrdID;
			int klen=(int)strlen(okey);
			pstats->totKeyLen+=klen; pstats->cntKeyLen++;
			if(klen>(int)pstats->maxKeyLen)
				pstats->maxKeyLen=klen;
			// Do we need to associate with OrigClOrdID?
		}
		else if(!strcmp(mtype,"conf"))
		{
			// Args
			if(aidx -cidx!=1)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetPrice(Price);
			int OrderQty=myatoi(args[cidx++]);
			porder->SetOrderQty(OrderQty);
			// Stats
			if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			{
				ainst->AcctAddOrderQty(porder->GetOrderQty()); acct=true;
			}
			pstats->totFix35_8++;
			porder->SetHighExecType('0');
			// If we have unconfirmed order set, then remove this order from it here.
		}
		else if(!strcmp(mtype,"rpld"))
		{
			// Args
			if(aidx -cidx!=2)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetPrice(Price);
			int OrderQty=myatoi(args[cidx++]);
			char *OrigClOrdID=args[cidx++];
			if(!odb.INDEX_CLORDID_CS) //DT9491
				_strupr(OrigClOrdID);
			porder->SetOrderQty(OrderQty);
			// Stats
			if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			{
				ainst->AcctAddOrderQty(porder->GetOrderQty()); acct=true;
			}
			pstats->totFix35_8++;
			// Previous order replaced
			porder->SetHighExecType('5');
			if(*OrigClOrdID)
			{
				if(odb.INDEX_CLORDID_CS ? !strcmp(OrigClOrdID,ClOrdID) : !_stricmp(OrigClOrdID,ClOrdID)) //DT9491
				{
					WSLogEvent("USR%d: OrigClOrdID(%s) should not be the same as ClOrdID on rpld journal entry(%d).",PortNo,OrigClOrdID,lidx);
					// TODO: Fix this at app source.
					// Alternate trick is to store the OrigClOrdID in VSOrder and use that value instead
				}
				else
				{
					char ookey[256]={0};
				#ifdef MULTI_DAY
					// TODO: What if the original order date is the previous EST day
					if(odb.INDEX_ORDERDATE)
						sprintf(ookey,"%08d.%s",OrderDate,OrigClOrdID);
					else
						strcpy(ookey,OrigClOrdID);
				#else
					strcpy(ookey,OrigClOrdID);
				#endif
					ITEMLOC ooloc=-1;
					bool oisnew=false;
					VSOrder *oorder=CreateOrder(UscPortNo,asys,ookey,oisnew,ooloc);
					_ASSERT(oorder!=porder);
					if((oorder)&&(oorder!=porder))
					{
						if(!oorder->IsTerminated())
						{
							// Don't change a key once it's indexed
							if(oisnew)
							{
								oorder->SetAppInstID(AppInstID);
								oorder->SetClOrdID(OrigClOrdID);
								oorder->SetSymbol(Symbol);
								oorder->SetRootOrderID(RootOrderID);
								//oorder->SetFirstClOrdID(FirstClOrdID);
								//oorder->SetClParentOrderID(ClParentOrderID);
								oorder->SetAccount(Account);
								oorder->SetEcnOrderID(EcnOrderID);
								oorder->SetClientID(ClientID);
								oorder->SetTransactTime(TransactTime);
								oorder->SetConnection(Connection);
								oorder->SetAuxKeys(AuxKey,pdb->INDEX_AUXKEYS); //DT9398
							}
							oorder->SetTerminated(true); oterm++;
							oorder->SetHighExecType('5');
							if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
							{
								ainst->AcctAddOrderQty(-oorder->GetOrderQty()); acct=true;
							}
							if(oorder->IsDirty())
								CommitOrder(UscPortNo,asys,oorder,ooloc);
							CallTasks(oorder,asys,ainst,UscPortNo,-1);
							if(oisnew)
							{
								// Indices and tasks for placeholder order
								if(pdb->AssociateOrder(oorder,ooloc,ocnt,pcnt)>0)
								{
									// New child for any order
								}
							}

							VSINDEX::iterator ooit=pdb->oomap.find(ookey);
							if(ooit!=pdb->oomap.end())
								pdb->oomap.erase(ooit);
							#ifdef UNLOAD_TERM_ORDERS
							UnloadOrder(UscPortNo,asys,oorder,ooloc); oorder=0;
							#endif
						}				
						if((oorder)&&(pdb->HAVE_FUSION_IO))
							UnloadOrder(UscPortNo,asys,oorder,ooloc);
					}
				}
			}
			// If we have unconfirmed order set, then remove this order from it here.
		}
		else if(!strcmp(mtype,"fill"))
		{
			// Args
			if(aidx -cidx!=5)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			const char *ExecID=args[cidx++];
			int LastQty=myatoi(args[cidx++]);
			int OrderQty=myatoi(args[cidx++]);
			int CumQty=myatoi(args[cidx++]);
			int LeavesQty=myatoi(args[cidx++]);
			porder->SetOrderQty(OrderQty);
			int lcqty=porder->GetCumQty();
			porder->SetCumQty(CumQty);
			porder->AddFillQty(LastQty);
			if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			{
				ainst->AcctAddFillQty(LastQty);
				ainst->AcctAddCumQty(CumQty -lcqty); acct=true;
			}
			porder->SetHighExecType('1');

			#ifndef NO_EFILLS_FILE
			Lock();
			if(!ffp)
			{
				#ifdef WIN32
				sprintf(ffpath,"%s\\ofills_%08d.csv",FILLS_FILE_DIR.c_str(),WSDate);
				#else
				sprintf(ffpath,"%s/ofills_%08d.csv",FILLS_FILE_DIR.c_str(),WSDate);
				#endif
				ffp=fopen(ffpath,"wt");
				if(ffp)
					fprintf(ffp,"AppInstID,RootOrderID,ClOrdID,Account,EcnOrderID,ExecID,Symbol,LastQty,FillQty,OrderQty,CumQty,LeavesQty,\n");
				else
					WSLogError("Failed opening %s for write!",ffpath);
			}
			if(ffp)
				fprintf(ffp,"%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,\n",
					AppInstID,RootOrderID,ClOrdID,Account,EcnOrderID,ExecID,Symbol,
					LastQty,porder->GetFillQty(),OrderQty,CumQty,LeavesQty);
			Unlock();
			#endif
			// Stats
			pstats->totFix35_8++;
			// Call any fill tasks here
			fill=true;
		}
		else if(!strcmp(mtype,"filled"))
		{
			// Args
			if(aidx -cidx!=4)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			const char *ExecID=args[cidx++];
			int LastQty=myatoi(args[cidx++]);
			int OrderQty=myatoi(args[cidx++]);
			porder->SetOrderQty(OrderQty);
			int CumQty=myatoi(args[cidx++]);
			int lcqty=porder->GetCumQty();
			porder->SetCumQty(CumQty);
			porder->AddFillQty(LastQty);
			if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			{
				ainst->AcctAddFillQty(LastQty);
				ainst->AcctAddCumQty(CumQty -lcqty); acct=true;
			}
			// Redi doesn't drop order requests and away executions don't get confirms
			//if((odb.AUXKEY_NAMES[0]=="RoutingBroker")&&(!strncmp(AuxKey,"AWAY",4)))
			//{
			//	ainst->AcctAddOrderQty(OrderQty); acct=true;
			//}
			porder->SetHighExecType('2');
			// If we have open order set, then remove this order from it here.
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}

			#ifndef NO_EFILLS_FILE
			Lock();
			if(!ffp)
			{
				#ifdef WIN32
				sprintf(ffpath,"%s\\ofills_%08d.csv",FILLS_FILE_DIR.c_str(),WSDate);
				#else
				sprintf(ffpath,"%s/ofills_%08d.csv",FILLS_FILE_DIR.c_str(),WSDate);
				#endif
				ffp=fopen(ffpath,"wt");
				if(ffp)
					fprintf(ffp,"AppInstID,RootOrderID,ClOrdID,Account,EcnOrderID,ExecID,Symbol,LastQty,FillQty,OrderQty,CumQty,LeavesQty,\n");
				else
					WSLogError("Failed opening %s for write!",ffpath);
			}
			if(ffp)
				fprintf(ffp,"%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,\n",
					AppInstID,RootOrderID,ClOrdID,Account,EcnOrderID,ExecID,Symbol,
					LastQty,porder->GetFillQty(),porder->GetOrderQty(),CumQty,porder->GetOrderQty() -porder->GetCumQty());
			Unlock();
			#endif
			// Stats
			pstats->totFix35_8++;
			// Call any fill tasks here
			fill=true;
		}
		else if(!strcmp(mtype,"dfd"))
		{
			// Args
			if(aidx -cidx!=1)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			int CumQty=myatoi(args[cidx++]);
			porder->SetCumQty(CumQty);
			porder->SetHighExecType('3');
			// Stats
			pstats->totFix35_8++;
			// If we have open order set, then remove this order from it here.
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}
		}
		else if(!strcmp(mtype,"cxld"))
		{
			// Args
			if(aidx -cidx!=2)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			int CumQty=myatoi(args[cidx++]);
			char *OrigClOrdID=args[cidx++];
			if(!odb.INDEX_CLORDID_CS) //DT9491
				_strupr(OrigClOrdID);
			porder->SetCumQty(CumQty);
			porder->SetHighExecType('4');
			// Stats
			pstats->totFix35_8++;
			pstats->totFix150_4++;
			// If we have open order set, then remove this order from it here.
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}
			// Previous order cancelled
			if(*OrigClOrdID)
			{
				if(odb.INDEX_CLORDID_CS ? !strcmp(OrigClOrdID,ClOrdID) : !_stricmp(OrigClOrdID,ClOrdID)) //DT9491
				{
					// Spams the log
					//WSLogEvent("USR%d: OrigClOrdID(%s) should not be the same as ClOrdID on cxld journal entry(%d).",PortNo,OrigClOrdID,lidx);
					// TODO: Fix this at app source.
					// Alternate trick is to store the OrigClOrdID in VSOrder and use that value instead
				}
				else
				{
					char ookey[256]={0};
				#ifdef MULTI_DAY
					// TODO: What if the original order date is the previous EST day
					if(odb.INDEX_ORDERDATE)
						sprintf(ookey,"%08d.%s",OrderDate,OrigClOrdID);
					else
						strcpy(ookey,OrigClOrdID);
				#else
					strcpy(ookey,OrigClOrdID);
				#endif
					ITEMLOC ooloc=-1;
					bool oisnew=false;
					VSOrder *oorder=CreateOrder(UscPortNo,asys,ookey,oisnew,ooloc);
					_ASSERT(oorder!=porder);
					if((oorder)&&(oorder!=porder))
					{
						if(!oorder->IsTerminated())
						{
							// Don't change a key once it's indexed
							if(oisnew)
							{
								oorder->SetAppInstID(AppInstID);
								oorder->SetClOrdID(OrigClOrdID);
								oorder->SetSymbol(Symbol);
								oorder->SetRootOrderID(RootOrderID);
								//oorder->SetFirstClOrdID(FirstClOrdID);
								//oorder->SetClParentOrderID(ClParentOrderID);
								oorder->SetAccount(Account);
								oorder->SetEcnOrderID(EcnOrderID);
								oorder->SetClientID(ClientID);
								oorder->SetTransactTime(TransactTime);
								oorder->SetConnection(Connection);
								oorder->SetAuxKeys(AuxKey,pdb->INDEX_AUXKEYS); //DT9398
							}
							oorder->SetTerminated(true); oterm++;
							oorder->SetHighExecType('4');
							if(oorder->IsDirty())
								CommitOrder(UscPortNo,asys,oorder,ooloc);
							CallTasks(oorder,asys,ainst,UscPortNo,-1);
							if(oisnew)
							{
								// Indices and tasks for placeholder order
								if(pdb->AssociateOrder(oorder,ooloc,ocnt,pcnt)>0)
								{
									// New child for any order
								}
							}

							VSINDEX::iterator ooit=pdb->oomap.find(ookey);
							if(ooit!=pdb->oomap.end())
								pdb->oomap.erase(ooit);
							#ifdef UNLOAD_TERM_ORDERS
							UnloadOrder(UscPortNo,asys,oorder,ooloc); oorder=0;
							#endif
						}
						if((oorder)&&(odb.HAVE_FUSION_IO))
							UnloadOrder(UscPortNo,asys,oorder,ooloc);
					}
				}
			}
		}
		else if(!strcmp(mtype,"rej"))
		{
			// Args
			if(aidx -cidx!=1)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			int CumQty=myatoi(args[cidx++]);
			porder->SetCumQty(CumQty);
			porder->SetHighExecType('8');
			// Stats
			pstats->totFix35_8++;
			// If we have open order set, then remove this order from it here.
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}
		}
		else if(!strcmp(mtype,"canrej"))
		{
			// Args
			if(aidx -cidx!=1)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			char *OrigClOrdID=args[cidx++];
			if(!odb.INDEX_CLORDID_CS) //DT9491
				_strupr(OrigClOrdID);
			porder->SetHighExecType('9');
			// Stats
			pstats->totFix35_8++;
			// Not sure if this warrants being separate from "term"
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}
		}
		else if(!strcmp(mtype,"srej"))
		{
			// Args
			if(aidx -cidx!=0)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetHighExecType('8');
			// Stats
			pstats->totFix35_8++;
			// Not sure if this warrants being separate from "term"
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}
		}
		// TODO: Support bust/correct
	#ifdef MULTI_RECTYPES
		else if(!strcmp(mtype,"iqaccount"))
		{
			// Args
			if(aidx -cidx!=0)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetType(0x04);
			// Stats
			//pstats->totFix35_8++;
			// If we have unconfirmed order set, then remove this order from it here.
		}
		else if(!strcmp(mtype,"iqpos"))
		{
			// Args
			if(aidx -cidx!=4)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			porder->SetPrice(Price);
			porder->SetSide(Side[0]);
			porder->SetType(0x05);
			const char *ExecID=args[cidx++];
			int LastQty=myatoi(args[cidx++]);
			int OrderQty=myatoi(args[cidx++]);
			if(OrderQty>0)
			{
				porder->SetOrderQty(OrderQty);
				porder->SetSide('1');
			}
			else if(OrderQty<0)
			{
				porder->SetOrderQty(-OrderQty);
				porder->SetSide('2');
			}
			int CumQty=myatoi(args[cidx++]);
			int lcqty=porder->GetCumQty();
			porder->SetCumQty(CumQty);
			porder->AddFillQty(LastQty);
			if((odb.AUXKEY_NAMES[0]!="RoutingBroker")||(strncmp(AuxKey,"AWAY",4)))
			{
				ainst->AcctAddFillQty(LastQty);
				ainst->AcctAddCumQty(CumQty -lcqty);
			}
			acct=true;
			//porder->SetHighExecType('2');
			porder->SetHighExecType('B'); // calculated so positions don't show up in filled orders query
			// If we have open order set, then remove this order from it here.
			if(!porder->IsTerminated())
			{
				porder->SetTerminated(true); oterm++;
				VSINDEX::iterator ooit=pdb->oomap.find(okey);
				if(ooit!=pdb->oomap.end())
					pdb->oomap.erase(ooit);
			}
			// Stats
			//pstats->totFix35_8++;
		}
	#endif
		else if(!strcmp(mtype,"det"))
		{
			// Args
			if(aidx -cidx!=0)
			{
				pdb->Unlock();
				WSLogError("USR%d: Incorrect number of '%s' fields(%d) on journal entry(%d).",PortNo,mtype,aidx,lidx);
				pptr=nptr;
				continue;
			}
			// Stats
			// Catch all other details
		}
		else
		{
			WSLogError("USR%d: Unknown mtype(%s) on journal entry(%d)",PortNo,mtype,lidx);
			_ASSERT(false);
		}
		// Commit and free the order as needed
		if(porder)
		{
			if(porder->IsDirty())
				CommitOrder(UscPortNo,asys,porder,oloc);
			// Call tasks
			CallTasks(porder,asys,ainst,UscPortNo,didx);

		#ifdef TASK_THREAD_POOL
			// Unload must be done by TaskThread
			// When we have fusion I/O, the disk copy is as good as memory
			if(pdb->HAVE_FUSION_IO)
				fhint.unload=true;
			#ifdef UNLOAD_TERM_ORDERS
			// Evict orders when they terminate
			else if(porder->IsTerminated())
				fhint.unload=true;
			#endif
		#endif
			// It is critical to only associate orders as few times as possible for best performance 
			// in order to optimize multimap insert
			if(isnew)
			{
				// Indices
				if(pdb->AssociateOrder(porder,oloc,ocnt,pcnt)>0)
				{
					// New child for any order
				}
			}
			// Make up for missing Account and FirstClOrdID tags in some messages
			else
			{
				if((needAccount)&&(*Account))
					pdb->AssociateAccount(porder,oloc);;
				if((needFirstClOrdID)&&(*FirstClOrdID))
					pdb->AssociateFirstClOrdID(porder,oloc);
				if((needClParentOrderID)&&(*ClParentOrderID))
					pdb->AssociateClParentOrderID(porder,oloc);
				if((needEcnOrderID)&&(*EcnOrderID))
					pdb->AssociateEcnOrderID(porder,oloc);
				if((needClientID)&&(*ClientID))
					pdb->AssociateClientID(porder,oloc);
				if((needTransactTime)&&(TransactTime>0))
					pdb->AssociateTransactTime(porder,oloc);
			#ifdef MULTI_DAY
				if((needOrderDate)&&(OrderDate>0))
					pdb->AssociateOrderDate(porder,oloc);
			#endif
				if((needAuxKey)&&(*AuxKey))
					pdb->AssociateAuxKey(porder,oloc);
			}
			
			if(acct)
			{
				FeedHint achint={asys,ainst,porder,-1,0,PROTO_UNKNOWN};
				taskmgr.CallTasks("ACCOUNTS",TASK_SQL_ACCOUNTS,&achint);
			}

			if((fill)&&(pdb->INDEX_FILLED_ORDERS))
			{
				// Multimap for filled orders
				bool found=false;
				VSINDEX::iterator eit=pdb->fomap.find(okey);
				//for(;(eit!=pdb->fomap.end())&&(eit.first==ClOrdID);eit++)
				//{
				//	if(eit.second.off==oloc.off)
				//	{
				//		found=true;
				//		break;
				//	}
				//}
				if(!found)
				{
				#ifdef USE_DISK_INDEX
					eit=pdb->fomap.insert(okey,oloc);
				#else
					if(eit==pdb->fomap.end())
						eit=pdb->fomap.insert(okey,oloc);
					// Only add the order once (not multimap behavior)
					//else
					//	eit=pdb->fomap.insert(eit,ClOrdID,oit.second);
				#endif
				}
			}

		#ifndef TASK_THREAD_POOL
			// When we have fusion I/O, the disk copy is as good as memory
			if(pdb->HAVE_FUSION_IO)
				UnloadOrder(UscPortNo,asys,porder,oloc);
			#ifdef UNLOAD_TERM_ORDERS
			// Evict orders when they terminate
			else if(porder->IsTerminated())
				UnloadOrder(UscPortNo,asys,porder,oloc);
			#endif
			//pdb->SetZdirty(); is this needed?
		#endif
		}
		pdb->Unlock();
		pptr=nptr;
	}//for(int lidx
	}//scope

send_ack:
	// Ack the page
	char rbuf[1024],*rptr=rbuf;
	if(vsd->EncodeJPageReply(rptr,sizeof(rbuf),0,rid,jpage->lstart,jpage->lend)>0)
	{
		//#ifdef _DEBUG
		//WSLogEvent("USR%d: EncodeJPageReply(rc=%d,rid=%d,lstart=%d,lend=%d) %d bytes",
		//	PortNo,0,rid,jpage->lstart,jpage->lend,rptr -rbuf);
		//#endif
		WSUsrSendMsg(100,(WORD)(rptr -rbuf),rbuf,PortNo);
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			int mcnt=jpage->lend -jpage->lstart +1;
			ustats->msgCnt+=mcnt; ustats->totMsgCnt+=mcnt;
		}
	}
	else
		WSLogError("USR%d: EncodeJPageReply(rc=%d,rid=%d,lstart=%d,lend=%d) %d bytes failed to encode!",
			PortNo,0,rid,jpage->lstart,jpage->lend,rptr -rbuf);
	//odb.SetZdirty(); is this needed?
	//WSLogEvent("USR%d: VSDNotifyJPageRequest(%d,%d,%d,%d).done",PortNo,rid,jpage->lstart,jpage->lend,retransmit);
}
void ViewServer::VSDNotifyJpageReply(void *udata, int rc, DWORD rid, int lstart, int lend)
{
	_ASSERT((udata)&&(lstart>=0)&&(lend>=0));
	_ASSERT(!VSDIST_HUB);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS))
		return;
	int UscPortNo=(int)(PTRCAST)ConPort[PortNo].DetPtr4;
	if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		return;
	//#ifdef _DEBUG
	//WSLogEvent("CON%d: VSDNotifyJpageReply(%d,%d,%d)",PortNo,rid,lstart,lend);
	//#endif
	VSDetailFile *dfile=(VSDetailFile *)UscPort[UscPortNo].DetPtr1;
	VSDistJournal *dj=(VSDistJournal*)UscPort[UscPortNo].DetPtr5;
	if((dfile)&&(dj))
	{
		LONGLONG rpos=0;
		dj->RemLine(lstart,lend,rpos);
		if(rpos>0)
		{
		#ifdef WIN32
			if(rpos>dfile->aend.QuadPart)
			{
				dfile->aend.QuadPart=rpos;
				odb.SetZdirty();
			}
		#else
			if(rpos>dfile->aend)
			{
				dfile->aend=rpos;
				odb.SetZdirty();
			}
		#endif
		}
	}
	// Process journal ack
}
void ViewServer::VSDNotifyAliasRequest(void *udata, DWORD rid, const char *prefix, const char *dpath, int proto, int wsdate)
{
	_ASSERT((udata)&&(prefix)&&(*prefix)&&(dpath)&&(*dpath));
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS))
		return;
	int UscPortNo=UsrPort[PortNo].UscPort;
	if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
		return;
	WSLogEvent("USR%d: Alias(%s,%s,%d)",PortNo,prefix,dpath,proto);
	VSDetailFile *dfile=(VSDetailFile *)UscPort[UscPortNo].DetPtr1;
	if(!dfile)
	{
	#ifdef MULTI_DAY_HIST
		VSDetailFile *dfile=odb.OpenDetailFile(prefix,UscPortNo,proto,dpath,true,wsdate);
	#else
		VSDetailFile *dfile=odb.OpenDetailFile(prefix,UscPortNo,proto,dpath,true);
	#endif
		UscPort[UscPortNo].DetPtr1=dfile;
		odb.SetEdirty();
	}
}

// DropClientNotify
void ViewServer::DCSendMsg(void *udata, const char *msg, int mlen)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortType==WS_CON)&&(PortNo>=0)&&(PortNo<NO_OF_CON_PORTS))
	{
		DropClient *pdc=(DropClient*)ConPort[PortNo].DetPtr5;
		if(!pdc)
			return;
		WSConSendBuff(mlen,(char*)msg,PortNo,1);
	}
#ifdef FIX_SERVER
	else if((PortType==WS_USR)&&(PortNo>=0)&&(PortNo<NO_OF_USR_PORTS))
	{
		DropServer *pds=(DropServer*)UsrPort[PortNo].DetPtr5;
		if(!pds)
			return;
		WSUsrSendBuff(mlen,(char*)msg,PortNo,1);
	}
#endif
}
void ViewServer::DCLoggedIn(void *udata, int hbinterval)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortType==WS_CON)&&(PortNo>=0)&&(PortNo<NO_OF_CON_PORTS))
	{
		DropClient *pdc=(DropClient*)ConPort[PortNo].DetPtr5;
		if(!pdc)
			return;
		WSLogEvent("CON%d: Logged in as %s; lastIn=%d, lastOut=%d",
			PortNo,pdc->GetConnID().c_str(),pdc->iseq,pdc->oseq);
	}
#ifdef FIX_SERVER
	else if((PortType==WS_USR)&&(PortNo>=0)&&(PortNo<NO_OF_USR_PORTS))
	{
		DropServer *pds=(DropServer*)UsrPort[PortNo].DetPtr5;
		if(!pds)
			return;
		WSLogEvent("USR%d: Logged in as %s; lastIn=%d, lastOut=%d",
			PortNo,pds->GetConnID().c_str(),pds->iseq,pds->oseq);
	}
#endif
}
void ViewServer::DCLoggedOut(void *udata, const char *text)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortType==WS_CON)&&(PortNo>=0)&&(PortNo<NO_OF_CON_PORTS))
	{
		DropClient *pdc=(DropClient*)ConPort[PortNo].DetPtr5;
		if(!pdc)
			return;
		WSLogEvent("CON%d: Logged out from %s: %s",PortNo,pdc->GetConnID().c_str(),text);
		ConPort[PortNo].ConnectHold=true;
	}
#ifdef FIX_SERVER
	else if((PortType==WS_USR)&&(PortNo>=0)&&(PortNo<NO_OF_USR_PORTS))
	{
		DropServer *pds=(DropServer*)UsrPort[PortNo].DetPtr5;
		if(!pds)
			return;
		WSLogEvent("USR%d: Logged out from %s: %s",PortNo,pds->GetConnID().c_str(),text);
		UsrPort[PortNo].TimeTillClose=5;
		UsrPort[PortNo].SockActive=0;
	}
#endif
}
LONGLONG ViewServer::DCWriteFix(void *udata, char mdir, FIXINFO *pfix)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortType==WS_CON)&&(PortNo>=0)&&(PortNo<NO_OF_CON_PORTS))
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			cstats->msgCnt++; cstats->totMsgCnt++;
		}
		if(pfix->TagChar(35)=='0')
		{
			if(mdir=='I') ConPort[PortNo].BeatsIn++;
			else if(mdir=='O') ConPort[PortNo].BeatsOut++;
		}
		DropClient *pdc=(DropClient*)ConPort[PortNo].DetPtr5;
		if(!pdc)
			return -1;

		// Write the dropped FIX message
		int UscPortNo=(int)(PTRCAST)ConPort[PortNo].DetPtr4;
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
			return -1;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		LONGLONG doff=-1;
		if(dfile)
		{
		#ifndef NO_FIX_WRITES
			// Full log format
			char wbuf[2048]={0},*wptr=wbuf;
			sprintf(wptr,"%c:%06d:%04d:",mdir,WSTime,pfix->llen); wptr+=strlen(wptr);
			memcpy(wptr,pfix->fbuf,pfix->llen); wptr+=pfix->llen;
			#ifdef WIN32
			strcpy(wptr,"\r\n"); wptr+=2;
			#else
			strcpy(wptr,"\n"); wptr++;
			#endif
			odb.WriteDetailBlock(dfile,wbuf,(int)(wptr -wbuf),doff);
		#endif
		}
		UsrPort[PortNo].DetPtr1=(void*)(PTRCAST)pfix->llen;
		UsrPort[PortNo].PacketsIn++;
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt+=pfix->llen; ustats->totMsgCnt+=pfix->llen;
		}

		// DEV-18992: Return Emit order responses
		ReturnEmitReponse(*pfix);
		return doff;
	}
#ifdef FIX_SERVER
	else if((PortType==WS_USR)&&(PortNo>=0)&&(PortNo<NO_OF_USR_PORTS))
	{
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt++; ustats->totMsgCnt++;
		}
		if(pfix->TagChar(35)=='0')
		{
			if(mdir=='I') UsrPort[PortNo].BeatsIn++;
			else if(mdir=='O') UsrPort[PortNo].BeatsOut++;
		}
		DropServer *pds=(DropServer*)UsrPort[PortNo].DetPtr5;
		if(!pds)
			return -1;

		// Write the dropped FIX message
		int UscPortNo=(int)(PTRCAST)UsrPort[PortNo].DetPtr4;
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
			return -1;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		LONGLONG doff=-1;
		if(dfile)
		{
		#ifndef NO_FIX_WRITES
			// Full log format
			char wbuf[2048]={0},*wptr=wbuf;
			sprintf(wptr,"%c:%06d:%04d:",mdir,WSTime,pfix->llen); wptr+=strlen(wptr);
			memcpy(wptr,pfix->fbuf,pfix->llen); wptr+=pfix->llen;
			#ifdef WIN32
			strcpy(wptr,"\r\n"); wptr+=2;
			#else
			strcpy(wptr,"\n"); wptr++;
			#endif
			odb.WriteDetailBlock(dfile,wbuf,(int)(wptr -wbuf),doff);
		#endif
		}
		UsrPort[PortNo].DetPtr1=(void*)(PTRCAST)pfix->llen;
		UsrPort[PortNo].PacketsIn++;
		ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt+=pfix->llen; ustats->totMsgCnt+=pfix->llen;
		}
		return doff;
	}
#endif
	return -1;
}
LONGLONG ViewServer::DCReadFix(void *udata, LONGLONG roff, char& mdir, FIXINFO *pfix)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	if((PortType==WS_CON)&&(PortNo>=0)&&(PortNo<NO_OF_CON_PORTS))
	{
		FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
		if(cstats)
		{
			cstats->msgCnt++; cstats->totMsgCnt++;
		}
		DropClient *pdc=(DropClient*)ConPort[PortNo].DetPtr5;
		if(!pdc)
			return 0;

		// Read the requested FIX message
		int UscPortNo=(int)(PTRCAST)ConPort[PortNo].DetPtr4;
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
			return 0;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
		#ifndef NO_FIX_WRITES
			// Full log format
			char rbuf[2048]={0};
			DWORD rbytes=odb.ReadDetail(dfile,roff,rbuf,sizeof(rbuf));
			if(rbytes<1)
				return 0;
			const char *rend=rbuf +rbytes;
			const char *rptr;
			for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
				;
			if(!*rptr)
				return 0;
			char *eptr=(char*)strendl(rptr,rend);
			if(eptr) *eptr=0;
			const char *nptr;
			for(nptr=eptr+1;(nptr<rend)&&((*nptr=='\r')||(*nptr=='\n'));nptr++)
				;
			LONGLONG noff=roff +(int)(nptr -rbuf);
			mdir=rptr[0];
			for(int d=0;d<3;d++)
			{
				const char *nptr=strechr(rptr,':',rptr +32);
				if(!nptr)
					return 0;
				rptr=nptr+1;
			}
			int flen=(int)(eptr -rptr);
			if(flen>pfix->ssize)
				flen=pfix->ssize;
			memcpy(pfix->sbuf,rptr,flen); pfix->slen=flen;
			pfix->Reset();
			pfix->FixMsgReady(pfix->sbuf,flen);
			return noff;
		#endif
		}
	}
#ifdef FIX_SERVER
	else if((PortType==WS_USR)&&(PortNo>=0)&&(PortNo<NO_OF_USR_PORTS))
	{
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt++; ustats->totMsgCnt++;
		}
		DropServer *pds=(DropServer*)UsrPort[PortNo].DetPtr5;
		if(!pds)
			return 0;

		// Read the requested FIX message
		int UscPortNo=(int)(PTRCAST)UsrPort[PortNo].DetPtr4;
		if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS))
			return 0;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
		#ifndef NO_FIX_WRITES
			// Full log format
			char rbuf[2048]={0};
			DWORD rbytes=odb.ReadDetail(dfile,roff,rbuf,sizeof(rbuf));
			if(rbytes<1)
				return 0;
			const char *rend=rbuf +rbytes;
			const char *rptr;
			for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
				;
			if(!*rptr)
				return 0;
			char *eptr=(char*)strendl(rptr,rend);
			if(eptr) *eptr=0;
			const char *nptr;
			for(nptr=eptr+1;(nptr<rend)&&((*nptr=='\r')||(*nptr=='\n'));nptr++)
				;
			LONGLONG noff=roff +(int)(nptr -rbuf);
			mdir=rptr[0];
			for(int d=0;d<3;d++)
			{
				const char *nptr=strechr(rptr,':',rptr +32);
				if(!nptr)
					return 0;
				rptr=nptr+1;
			}
			int flen=(int)(eptr -rptr);
			if(flen>pfix->ssize)
				flen=pfix->ssize;
			memcpy(pfix->sbuf,rptr,flen); pfix->slen=flen;
			pfix->Reset();
			pfix->FixMsgReady(pfix->sbuf,flen);
			return noff;
		#endif
		}
	}
#endif
	return 0;
}
void ViewServer::WSLogErrorOnce(const char *fmt,...)
{
	char tbuf[2048]={0};
	va_list alist;
	va_start(alist,fmt);
	vsprintf_s(tbuf,sizeof(tbuf),fmt,alist);
	va_end(alist);
	if(onceErrSet.find(tbuf)!=onceErrSet.end())
		return;
	onceErrSet.insert(tbuf);
	WSLogError(tbuf);
}

//int ViewServer::RecurseDelete(const char *dpath)
//{
//	char dmatch[MAX_PATH]={0};
//	sprintf(dmatch,"%s\\*",dpath);
//	WIN32_FIND_DATA fdata;
//	HANDLE fhnd=FindFirstFile(dmatch,&fdata);
//	if(fhnd==INVALID_FILE_VALUE)
//		return 0;
//	do
//	{
//		if((!strcmp(fdata.cFileName,"."))||(!strcmp(fdata.cFileName,"..")))
//			;
//		else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
//		{
//			char ddir[MAX_PATH]={0};
//			sprintf(ddir,"%s\\%s",dpath,fdata.cFileName);
//			RecurseDelete(ddir);
//		}
//		else
//		{
//			char fpath[MAX_PATH]={0};
//			sprintf(fpath,"%s\\%s",dpath,fdata.cFileName);
//			DeleteFile(fpath);
//		}
//	}while(FindNextFile(fhnd,&fdata));
//	FindClose(fhnd);
//	RemoveDirectory(dpath);
//	return 0;
//}
int ViewServer::CalcTPlusDate(int wsdate, int n, bool skipWeekends)
{
	// Convert wsdate to time_t
	tm ltm;
	memset(&ltm,0,sizeof(tm));
	ltm.tm_year=wsdate/10000 -1900;
	ltm.tm_mon=(wsdate%10000)/100 -1;
	ltm.tm_mday=wsdate%100;
	ltm.tm_hour=3; // Set to 3am for day after DST changes
	ltm.tm_isdst=-1;
	time_t wtm=mktime(&ltm);
	if(!wtm)
		return 0;
	// Add or subtract n business days
	while(n!=0)
	{
		if(n>0) 
		{
			wtm+=86400; ltm.tm_wday++;
			if(ltm.tm_wday>6) ltm.tm_wday=0;
			// 0-Sunday,...,6-Saturday
			if(skipWeekends)
			{
				if((ltm.tm_wday>0)&&(ltm.tm_wday<6))
					n--;
			}
			else
				n--;
		}
		else 
		{
			wtm-=86400; ltm.tm_wday--;
			if(ltm.tm_wday<0) ltm.tm_wday=6;
			// 0-Sunday,...,6-Saturday
			if(skipWeekends)
			{
				if((ltm.tm_wday>0)&&(ltm.tm_wday<6))
					n++;
			}
			else
				n++;
		}
	}
	tm *ptm=localtime(&wtm);
	if(ptm)
		wsdate=((ptm->tm_year +1900)*10000)+((ptm->tm_mon+1)*100)+(ptm->tm_mday);
	return wsdate;
}
int ViewServer::PurgeOldDays()
{
	if(KEEP_DAYS<0)
		return 0;
	int kdate=CalcTPlusDate(WSDate,-KEEP_DAYS);
	WSLogEvent("Purging data older than %08d...",kdate);
#if defined(WIN32)||defined(_CONSOLE)
	char dmatch[MAX_PATH]={0};
	sprintf(dmatch,"%s\\data\\*",pcfg->RunPath.c_str());
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(dmatch,&fdata);
	if(fhnd==INVALID_FILE_VALUE)
		return 0;
	do
	{
		if((!strcmp(fdata.cFileName,"."))||(!strcmp(fdata.cFileName,"..")))
			;
		else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			;
		else
		{
			// Must be 'yyyymmdd' exactly
			int ndigits=0,ddate=0;
			const char *cptr=strchr(fdata.cFileName,'_');
			if(cptr)
			{
				cptr++;
				int i;
				for(i=0;i<8;i++)
				{
					if(isdigit(cptr[i]))
						ndigits++;
				}
				if(VSDIST_HUB)
				{
					if((!strcmp(cptr+i,".dat"))||
					   (!strcmp(cptr+i,".ext"))||
					   (!strcmp(cptr+i,".csv")))
						ndigits++;
				}
				else
				{
					if((!strcmp(cptr+i,".fix"))||
					   (!strcmp(cptr+i,".msg"))||
					   (!strcmp(cptr+i,".dat"))||
					   (!strcmp(cptr+i,".zmap")))
						ndigits++;
				}
				ddate=atol(cptr);
			}
			if((ndigits==9)&&(ddate<kdate))
			{
				char dpath[MAX_PATH]={0};
				sprintf(dpath,"%s\\data\\%s",pcfg->RunPath.c_str(),fdata.cFileName);
				WSLogEvent("Purging %s...",dpath);
				if(!DeleteFile(dpath))
					WSLogError("Failed deleteing %s",dpath);
			}
			else
				WSLogEvent("Keeping %s\\data\\%s.",pcfg->RunPath.c_str(),fdata.cFileName);
		}
	}while(FindNextFile(fhnd,&fdata));
	FindClose(fhnd);
#else
	char fdir[MAX_PATH]={0};
	sprintf(fdir,"%s/data",pcfg->RunPath.c_str());
	DIR *pdir=opendir(fdir);
	if(!pdir)
		return -1;
	dirent *fdata=0;
	do{
		fdata=readdir(pdir);
		if(fdata)
		{
			if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
				continue;
			char fpath[MAX_PATH]={0},fkey[MAX_PATH]={0};
			sprintf(fpath,"%s%s",fdir,fdata->d_name);
			// Must be 'yyyymmdd' exactly
			int ndigits=0,ddate=0;
			const char *cptr=strchr(fdata->d_name,'_');
			if(cptr)
			{
				cptr++;
				int i;
				for(i=0;i<8;i++)
				{
					if(isdigit(cptr[i]))
						ndigits++;
				}
				if(VSDIST_HUB)
				{
					if((!strcmp(cptr+i,".dat"))||
					   (!strcmp(cptr+i,".ext"))||
					   (!strcmp(cptr+i,".csv")))
						ndigits++;
				}
				else
				{
					if((!strcmp(cptr+i,".fix"))||
					   (!strcmp(cptr+i,".msg"))||
					   (!strcmp(cptr+i,".dat"))||
					   (!strcmp(cptr+i,".zmap")))
						ndigits++;
				}
				ddate=atol(cptr);
			}
			if((ndigits==9)&&(ddate<kdate))
			{
				char dpath[MAX_PATH]={0};
				sprintf(dpath,"%s/data/%s",pcfg->RunPath.c_str(),fdata->d_name);
				WSLogEvent("Purging %s...",dpath);
				unlink(dpath);
				if(PathFileExists(dpath))
					WSLogError("Failed deleteing %s",dpath);
			}
			else
				WSLogEvent("Keeping %s/data/%s.",pcfg->RunPath.c_str(),fdata->d_name);
		}
	}while(fdata);
	closedir(pdir);
#endif
	return 0;
}

bool ViewServer::CallTasks(VSOrder* order,AppSystem *asys,AppInstance* ainst,int UscPortNo,int didx)
{
	if(!order || !ainst || !asys)
		return false;

#ifdef TASK_THREAD_POOL
	FeedHint ofhint={asys,ainst,oorder,didx,0,PROTO_UNKNOWN,true,UscPortNo};
	taskmgr.CallTasks("ORDERS",TASK_SQL_ORDERS,&ofhint); oorder=0;
#else
	FeedHint ofhint={asys,ainst,order,didx,0,PROTO_UNKNOWN};
	taskmgr.CallTasks(order->GetClOrdID(),TASK_CLORDID,&ofhint);
	taskmgr.CallTasks(order->GetClParentOrderID(),TASK_PARENTCLORDID,&ofhint);
	taskmgr.CallTasks(order->GetAppInstID(),TASK_APPINST,&ofhint);
	taskmgr.CallTasks(asys->skey,TASK_APPSYS,&ofhint);
	taskmgr.CallTasks(order->GetSymbol(),TASK_SYMBOL,&ofhint);
	taskmgr.CallTasks(order->GetRootOrderID(),TASK_ROOTORDERID,&ofhint);
	taskmgr.CallTasks(order->GetFirstClOrdID(),TASK_FIRSTCLORDID,&ofhint);
	taskmgr.CallTasks(order->GetAccount(),TASK_ACCOUNT,&ofhint);
	taskmgr.CallTasks(order->GetEcnOrderID(),TASK_ECNORDERID,&ofhint);
	taskmgr.CallTasks(order->GetClientID(),TASK_CLIENTID,&ofhint);

	if(ofhint.pfix)
	{
		delete ofhint.pfix;
		ofhint.pfix=0;
	}
#endif

	return true;
}

#ifdef MULTI_DAY_HIST
OrderDB *AppSystem::GetDB(int wsdate)
{
	if(wsdate==odb.wsdate)
		return &odb;
	for(list<OrderDB *>::iterator dit=odblist.begin();dit!=odblist.end();dit++)
	{
		if((*dit)->wsdate==wsdate)
			return *dit;
	}
	return 0;
}
int ViewServer::ImportHistoricFiles(int wsdate)
{
	if(odb.HISTORIC_DAYS<=0)
		return -1;
	int nfiles=0;
	WSLogEvent("Importing %08d to history...",wsdate);
	char fmatch[MAX_PATH]={0},spath[MAX_PATH]={0},dpath[MAX_PATH]={0};
	// Import dist files
	char *hpcpy=new char[HISTIMP_DIST_PATHS.length()+1];
	strcpy(hpcpy,HISTIMP_DIST_PATHS.c_str());
	for(const char *hpath=strtok(hpcpy,",");hpath;hpath=strtok(0,","))
	{
		sprintf(fmatch,"%s\\*_%08d.*",hpath,wsdate);
		WIN32_FIND_DATA fdata;
		HANDLE fhnd=FindFirstFile(fmatch,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			do
			{
				if(!(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					// We have to merge .zmap files, so don't move them
					if(strrcmp(fdata.cFileName,".zmap"))
						continue;
					sprintf(spath,"%s\\%s",hpath,fdata.cFileName);
					sprintf(dpath,"%s\\data\\%s",pcfg->RunPath.c_str(),fdata.cFileName);
					// Same drive
					if((spath[1]==':')&&(dpath[1]==':')&&(spath[0]==dpath[0]))
					{
						if(MoveFile(spath,dpath))
						{
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed moving (%s) to (%s)!",spath,dpath);
					}
					// Different drives
					else
					{
						if(CopyFile(spath,dpath,true))
						{
							DeleteFile(spath);
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed copying (%s) to (%s)!",spath,dpath);
					}

					// Truncate .csv files to save disk space
					if(strrcmp(fdata.cFileName,".csv"))
						OrderDB::TruncateCsvFile(dpath);
				}
			}while(FindNextFile(fhnd,&fdata));
			FindClose(fhnd);
		}

		// We have to merge the .zmap files
		sprintf(spath,"%s\\VSDB_%08d.zmap",hpath,wsdate);
		FILE *rfp=fopen(spath,"rt");
		if(rfp)
		{
			sprintf(dpath,"%s\\data\\VSDB_%08d.zmap",pcfg->RunPath.c_str(),wsdate);
			// Open for append to merge entries
			FILE *wfp=fopen(dpath,"at");
			if(wfp)
			{
				char rbuf[1024]={0};
				while(fgets(rbuf,sizeof(rbuf),rfp))
					fprintf(wfp,"%s",rbuf);
				fclose(wfp); nfiles++;
				fclose(rfp);
				DeleteFile(spath);
			}
			else
				WSLogError("Failed writing %s!",dpath);
		}
	}
	delete hpcpy;

	// Import hub files
	hpcpy=new char[HISTIMP_HUB_PATHS.length()+1];
	strcpy(hpcpy,HISTIMP_HUB_PATHS.c_str());
	for(const char *hpath=strtok(hpcpy,",");hpath;hpath=strtok(0,","))
	{
		sprintf(fmatch,"%s\\*_%08d.*",hpath,wsdate);
		WIN32_FIND_DATA fdata;
		HANDLE fhnd=FindFirstFile(fmatch,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			do
			{
				if(!(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					// We have to merge .ext files, so don't move them
					if(strrcmp(fdata.cFileName,".ext"))
						continue;
					sprintf(spath,"%s\\%s",hpath,fdata.cFileName);
					sprintf(dpath,"%s\\data\\%s",pcfg->RunPath.c_str(),fdata.cFileName);
					// Same drive
					if((spath[1]==':')&&(dpath[1]==':')&&(spath[0]==dpath[0]))
					{
						if(MoveFile(spath,dpath))
						{
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed moving (%s) to (%s)!",spath,dpath);
					}
					// Different drives
					else
					{
						if(CopyFile(spath,dpath,true))
						{
							DeleteFile(spath);
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed copying (%s) to (%s)!",spath,dpath);
					}

					// Truncate .dat files to save disk space
					if(strrcmp(fdata.cFileName,".dat"))
						OrderDB::TruncateDatFile(dpath);
				}
			}while(FindNextFile(fhnd,&fdata));
			FindClose(fhnd);
		}

		// We have to fix the new directory path in the .ext file
		sprintf(spath,"%s\\VSDB_%08d.ext",hpath,wsdate);
		FILE *rfp=fopen(spath,"rt");
		if(rfp)
		{
			sprintf(dpath,"%s\\data\\VSDB_%08d.ext",pcfg->RunPath.c_str(),wsdate);
			// Open for append to merge entries
			FILE *wfp=fopen(dpath,"at");
			if(wfp)
			{
				char rbuf[1024]={0};
				while(fgets(rbuf,sizeof(rbuf),rfp))
				{
					char *eptr=(char *)strendl(rbuf,rbuf +strlen(rbuf));
					if(eptr) *eptr=0;
					if((!strncmp(rbuf,"//",2))||(!*rbuf)||(*rbuf=='\r')||(*rbuf=='\n'))
						fprintf(wfp,"%s\n",rbuf);
					else
					{
						int col=0;
						for(const char *tok=strtoke(rbuf,",");tok;tok=strtoke(0,","))
						{
							switch(++col)
							{
							case 3: fprintf(wfp,"%s\\data,",pcfg->RunPath.c_str()); break;
							default: fprintf(wfp,"%s,",tok); break;
							};
						}
						fprintf(wfp,"\n");
					}
				}
				fclose(wfp); nfiles++;
				fclose(rfp);
				DeleteFile(spath);
			}
			else
				WSLogError("Failed writing %s!",dpath);
		}
	}
	delete hpcpy;

	WSLogEvent("Imported %d files",nfiles);
	return nfiles;
}
int ViewServer::ImportHistoricFiles(int wsdate, HistImport *import)
{
	if(odb.HISTORIC_DAYS<=0)
		return -1;
	int nfiles=0;
	WSLogEvent("Importing %08d,%s,%s to history...",wsdate,import->HISTIMP_DIST_PATH.c_str(),import->HISTIMP_HUB_PATH.c_str());
	char fmatch[MAX_PATH]={0},spath[MAX_PATH]={0},dpath[MAX_PATH]={0};
	// Import dist files
	{
		const char *hpath=import->HISTIMP_DIST_PATH.c_str();
		sprintf(fmatch,"%s\\*_%08d.*",hpath,wsdate);
		WIN32_FIND_DATA fdata;
		HANDLE fhnd=FindFirstFile(fmatch,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			do
			{
				if(!(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					// We have to merge .zmap files, so don't move them
					if(strrcmp(fdata.cFileName,".zmap"))
						continue;
					sprintf(spath,"%s\\%s",hpath,fdata.cFileName);
					sprintf(dpath,"%s\\data\\%s",pcfg->RunPath.c_str(),fdata.cFileName);
					// Same drive
					if((spath[1]==':')&&(dpath[1]==':')&&(spath[0]==dpath[0]))
					{
						if(MoveFile(spath,dpath))
						{
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed moving (%s) to (%s)!",spath,dpath);
					}
					// Different drives
					else
					{
						if(CopyFile(spath,dpath,true))
						{
							DeleteFile(spath);
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed copying (%s) to (%s)!",spath,dpath);
					}

					// Truncate .csv files to save disk space
					if(strrcmp(fdata.cFileName,".csv"))
						OrderDB::TruncateCsvFile(dpath);
				}
			}while(FindNextFile(fhnd,&fdata));
			FindClose(fhnd);
		}

		// We have to merge the .zmap files
		sprintf(spath,"%s\\VSDB_%08d.zmap",hpath,wsdate);
		FILE *rfp=fopen(spath,"rt");
		if(rfp)
		{
			sprintf(dpath,"%s\\data\\VSDB_%08d.zmap",pcfg->RunPath.c_str(),wsdate);
			// Open for append to merge entries
			FILE *wfp=fopen(dpath,"at");
			if(wfp)
			{
				char rbuf[1024]={0};
				while(fgets(rbuf,sizeof(rbuf),rfp))
					fprintf(wfp,"%s",rbuf);
				fclose(wfp); nfiles++;
				fclose(rfp);
				DeleteFile(spath);
			}
			else
				WSLogError("Failed writing %s!",dpath);
		}
	}

	// Import hub files
	{
		const char *hpath=import->HISTIMP_HUB_PATH.c_str();
		sprintf(fmatch,"%s\\*_%08d.*",hpath,wsdate);
		WIN32_FIND_DATA fdata;
		HANDLE fhnd=FindFirstFile(fmatch,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			do
			{
				if(!(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					// We have to merge .ext files, so don't move them
					if(strrcmp(fdata.cFileName,".ext"))
						continue;
					sprintf(spath,"%s\\%s",hpath,fdata.cFileName);
					sprintf(dpath,"%s\\data\\%s",pcfg->RunPath.c_str(),fdata.cFileName);
					// Same drive
					if((spath[1]==':')&&(dpath[1]==':')&&(spath[0]==dpath[0]))
					{
						if(MoveFile(spath,dpath))
						{
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed moving (%s) to (%s)!",spath,dpath);
					}
					// Different drives
					else
					{
						if(CopyFile(spath,dpath,true))
						{
							DeleteFile(spath);
							WSLogEvent("Moved (%s) to (%s)",spath,dpath); nfiles++;
						}
						else
							WSLogError("Failed copying (%s) to (%s)!",spath,dpath);
					}

					// Truncate .dat files to save disk space
					if(strrcmp(fdata.cFileName,".dat"))
						OrderDB::TruncateDatFile(dpath);
				}
			}while(FindNextFile(fhnd,&fdata));
			FindClose(fhnd);
		}

		// We have to fix the new directory path in the .ext file
		sprintf(spath,"%s\\VSDB_%08d.ext",hpath,wsdate);
		FILE *rfp=fopen(spath,"rt");
		if(rfp)
		{
			sprintf(dpath,"%s\\data\\VSDB_%08d.ext",pcfg->RunPath.c_str(),wsdate);
			// Open for append to merge entries
			FILE *wfp=fopen(dpath,"at");
			if(wfp)
			{
				char rbuf[1024]={0};
				while(fgets(rbuf,sizeof(rbuf),rfp))
				{
					char *eptr=(char *)strendl(rbuf,rbuf +strlen(rbuf));
					if(eptr) *eptr=0;
					if((!strncmp(rbuf,"//",2))||(!*rbuf)||(*rbuf=='\r')||(*rbuf=='\n'))
						fprintf(wfp,"%s\n",rbuf);
					else
					{
						int col=0;
						for(const char *tok=strtoke(rbuf,",");tok;tok=strtoke(0,","))
						{
							switch(++col)
							{
							case 3: fprintf(wfp,"%s\\data,",pcfg->RunPath.c_str()); break;
							default: fprintf(wfp,"%s,",tok); break;
							};
						}
						fprintf(wfp,"\n");
					}
				}
				fclose(wfp); nfiles++;
				fclose(rfp);
				DeleteFile(spath);
			}
			else
				WSLogError("Failed writing %s!",dpath);
		}
	}
	WSLogEvent("Imported %d files",nfiles);
	return nfiles;
}
#endif

#ifdef REDIOLEDBCON
int ViewServer::LoadLastMarketDestTxt()
{
	const char *fpath="setup\\LastMarketDestination.txt";
	if(!PathFileExists(fpath))
		return 0;
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		WSLogError("Failed opening %s!",fpath);
		return -1;
	}
	map<string,string> newLastMarketMap;
	int lno=0;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;isspace(*rptr);rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n')||(*rptr=='#')||(!strncmp(rptr,"//",2)))
			continue;
		char *eptr=(char *)strendl(rptr,rptr +strlen(rptr));
		if(eptr) *eptr=0;
		if(!strchr(rptr,'='))
		{
			WSLogError("Missing = at line %d!",lno);
			continue;
		}
		const char *dest=strtok(rptr,"=");
		const char *translation=strtok(0,"=");
		newLastMarketMap[dest]=translation;
	}
	fclose(fp);
	lastMarketMap.clear();
	lastMarketMap.swap(newLastMarketMap);
	WSLogEvent("Loaded %d translations from %s.",lastMarketMap.size(),fpath);
	return 0;
}
int ViewServer::LoadRediDestCsv()
{
	const char *fpath=REDIDESTINATION_PATH.c_str();
	if(!PathFileExists(fpath))
		fpath="RediDestination.csv";
	if(!PathFileExists(fpath))
		return 0;
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		WSLogError("Failed opening %s!",fpath);
		return -1;
	}
	map<string,pair<string,string>> newRediDestMap;
	int lno=0;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;isspace(*rptr);rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n')||(*rptr=='#')||(!strncmp(rptr,"//",2)))
			continue;
		char *eptr=(char *)strendl(rptr,rptr +strlen(rptr));
		if(eptr) *eptr=0;
		const char *enumId=strtoke(rptr,",");
		const char *shortAlias=strtoke(0,",");
		const char *longAlias=strtoke(0,",");
		const char *broker=strtoke(0,",");
		const char *translation=strtoke(0,",");
		const char *execbrkmpid=strtoke(0,",");
		if (!execbrkmpid) {
			execbrkmpid = ""; //make_pair requires !null
		}
		if(!stricmp(enumId,"DestinationEnumId"))
			continue;
		// TODO: code review ExecBrokerMpid to the map
		newRediDestMap[translation]= std::make_pair(longAlias,execbrkmpid);
	}
	fclose(fp);
	rediDestMap.clear();
	rediDestMap.swap(newRediDestMap);
	WSLogEvent("Loaded %d destinations from %s.",rediDestMap.size(),fpath);
	return 0;
}
int ViewServer::LoadAlgoStrategyCsv()
{
	const char *fpath=ALGOSTRATEGY_PATH.c_str();
	if(!PathFileExists(fpath))
		fpath="AlgoStrategy.csv";
	if(!PathFileExists(fpath))
		return 0;
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		WSLogError("Failed opening %s!",fpath);
		return -1;
	}
	map<string,string> newAlgoStrategyMap;
	int lno=0;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;isspace(*rptr);rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n')||(*rptr=='#')||(!strncmp(rptr,"//",2)))
			continue;
		char *eptr=(char *)strendl(rptr,rptr +strlen(rptr));
		if(eptr) *eptr=0;
		const char *row=strtoke(rptr,",");
		const char *action=strtoke(0,",");
		const char *bookmark=strtoke(0,",");
		const char *broker=strtoke(0,",");
		const char *algorithm=strtoke(0,",");
		const char *displayname=strtoke(0,",");
		if(!stricmp(broker,"Broker"))
			continue;
		char akey[128]={0};
		sprintf(akey,"%s.%s",broker,algorithm);
		newAlgoStrategyMap[akey]=displayname;
	}
	fclose(fp);
	algoStrategyMap.clear();
	algoStrategyMap.swap(newAlgoStrategyMap);
	WSLogEvent("Loaded %d strategies from %s.",algoStrategyMap.size(),fpath);
	return 0;
}
#endif

const char* ViewServer::DefaultRouteBroker(bool isTicket,bool isAway)
{
	if(isTicket)
		return DEFAULT_ROUTEBROKER_ON_TICKETS.c_str();
	if(isAway)
		return DEFAULT_ROUTEBROKER_ON_DONEAWAYS.c_str();
	return DEFAULT_ROUTEBROKER_ON_ORDERS.c_str();
}
#ifndef REG_TZI_FORMAT
typedef struct _REG_TZI_FORMAT
{
	LONG Bias;
	LONG StandardBias;
	LONG DaylightBias;
	SYSTEMTIME StandardDate;
	SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;
#endif

int ViewServer::GetTimeZoneBias(const char *timeZone)
{
	QueryTimeZoneBias=0;
	if(!*timeZone)
		return 0;
	bool found=false;
	HKEY hkey;
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",0,KEY_READ,&hkey)==ERROR_SUCCESS)
	{
		char subkeyName[256]={0};
		for(DWORD dwIndex=0;::RegEnumKey(hkey,dwIndex,subkeyName,sizeof(subkeyName))==ERROR_SUCCESS;dwIndex++)
		{
			if(!stricmp(subkeyName,timeZone))
			{
				HKEY tzkey;
				if(::RegOpenKeyEx(hkey,subkeyName,0,KEY_READ,&tzkey)==ERROR_SUCCESS)
				{
					REG_TZI_FORMAT timeZoneInfo;
					memset(&timeZoneInfo,0,sizeof(timeZoneInfo));
					DWORD infoLen=sizeof(timeZoneInfo);
					DWORD dwType;
					if(::RegQueryValueEx(tzkey,"TZI",0,&dwType,(LPBYTE)&timeZoneInfo,&infoLen)==ERROR_SUCCESS)
					{
						QueryTimeZoneBias=timeZoneInfo.Bias;
						found=true;
					}
					::RegCloseKey(tzkey);
				}
			}
		}
		::RegCloseKey(hkey);
	}
	if(found)
		WSLogEvent("Time zone(%s) bias determined to be %d",timeZone,QueryTimeZoneBias);
	else
		WSLogError("Time zone(%s) bias undetermined!",timeZone);
	return QueryTimeZoneBias;
}

static void strwcpy(WCHAR *wstr, const char *str)
{
	if((wstr)&&(str))
	{
		while(*str)
		{
			*wstr=(WCHAR)(*str);
			wstr++; str++;
		}
	}
	*wstr=0;
}

int ViewServer::GetTradeRegion(const char *tradeRegion, int tradeMarketOffset)
{
	const char *timeZone="";
	if(!_stricmp(tradeRegion,"HK"))
		timeZone="China Standard Time";
	else if(!_stricmp(tradeRegion,"LN"))
		timeZone="GMT Standard Time";
	else if(!_stricmp(tradeRegion,"FUT"))
		timeZone="Eastern Standard Time";
	else if((!tradeRegion[0])||(!_stricmp(tradeRegion,"US")))
		timeZone="Eastern Standard Time";

	if(!*timeZone)
		return 0;
	bool found=false;
	HKEY hkey;
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",0,KEY_READ,&hkey)==ERROR_SUCCESS)
	{
		char subkeyName[256]={0};
		for(DWORD dwIndex=0;::RegEnumKey(hkey,dwIndex,subkeyName,sizeof(subkeyName))==ERROR_SUCCESS;dwIndex++)
		{
			if(!stricmp(subkeyName,timeZone))
			{
				HKEY tzkey;
				if(::RegOpenKeyEx(hkey,subkeyName,0,KEY_READ,&tzkey)==ERROR_SUCCESS)
				{
					REG_TZI_FORMAT timeZoneInfo;
					memset(&timeZoneInfo,0,sizeof(timeZoneInfo));
					DWORD infoLen=sizeof(timeZoneInfo);
					DWORD dwType;
					if(::RegQueryValueEx(tzkey,"TZI",0,&dwType,(LPBYTE)&timeZoneInfo,&infoLen)==ERROR_SUCCESS)
					{
						strwcpy(TradeRegion.StandardName,subkeyName);
						TradeRegion.StandardDate=timeZoneInfo.StandardDate;
						TradeRegion.StandardBias=timeZoneInfo.StandardBias;
						char daylightName[256]={0};
						DWORD dlnLen=sizeof(daylightName);
						if(::RegQueryValueEx(tzkey,"Dlt",0,&dwType,(LPBYTE)daylightName,&dlnLen)==ERROR_SUCCESS)
							strwcpy(TradeRegion.DaylightName,daylightName);
						TradeRegion.DaylightDate=timeZoneInfo.DaylightDate;
						TradeRegion.DaylightBias=timeZoneInfo.DaylightBias;
						TradeRegion.Bias=timeZoneInfo.Bias;
						found=true;
					}
					::RegCloseKey(tzkey);
				}
			}
		}
		::RegCloseKey(hkey);
	}
	// Add minutes to align with desired regional start of day (FUT and HK)
	TradeRegion.Bias-=tradeMarketOffset;
	if(found)
		WSLogEvent("Trade region(%s) bias determined to be %d",timeZone,TradeRegion.Bias);
	else
		WSLogError("Trade region(%s) bias undetermined!",timeZone);
	return TradeRegion.Bias;
}
