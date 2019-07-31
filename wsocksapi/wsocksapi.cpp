// C++ implementation

#include "stdafx.h"
//#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"

#ifdef WIN32
LARGE_INTEGER WsocksApp::WSPerformanceFreq={0,0};
ULONGLONG WsocksApp::WSMips=0;
ULONGLONG WsocksApp::WSMaxIdleMips=0;
ULONGLONG WsocksApp::WSIdleExtraCycles=0;
#endif//WIN32

static unsigned long wstrue_ul = 1;
static int wstrue = 1;
static int wsfalse = 0;

WsocksApp::WsocksApp()
{
	WS_USER=1000;
	// This must match the wsocksapi.h typedef version
	CON_PORT_VERSION=0x0101;
	USC_PORT_VERSION=0x0101;
	USR_PORT_VERSION=0x0200;
	FIL_PORT_VERSION=0x0101;
	UMC_PORT_VERSION=0x0101;
	UMR_PORT_VERSION=0x0101;
	CGD_PORT_VERSION=0x0101;
	UGC_PORT_VERSION=0x0101;
	UGR_PORT_VERSION=0x0101;
	CTO_PORT_VERSION=0x0101;
	CTI_PORT_VERSION=0x0101;
	OTHER_PORT_VERSION=0x0100;

	NO_OF_CON_PORTS=0;
	NO_OF_USC_PORTS=0;
	NO_OF_USR_PORTS=0;
	NO_OF_FILE_PORTS=0;
	NO_OF_UMC_PORTS=0;
	NO_OF_UMR_PORTS=0;
	NO_OF_CGD_PORTS=0;
	NO_OF_UGC_PORTS=0;
	NO_OF_UGR_PORTS=0;
	NO_OF_CTO_PORTS=0;
	NO_OF_CTI_PORTS=0;
	NO_OF_OTHER_PORTS=0;
	NO_OF_BRT_PORTS=0;
	NO_OF_BRR_PORTS=0;

	ConPort=0;
	UscPort=0;
	UsrPort=0;
	FilePort=0;
	UmcPort=0;
	UmrPort=0;
	CgdPort=0;
	UgcPort=0;
	UgrPort=0;
	CtoPort=0;
	CtiPort=0;
	OtherPort=0;

	buildDate=0;
	buildTime=0;
	__WSDATE__=__DATE__;
	__WSTIME__=__TIME__;
	dllhnd=0;
	initialized=false;
	threadSignal=0x02;
	threadIdleStop=0;
	WSDate=0;
	WSTime=0;
	WSlTime=0;
	memset(WScDate,0,9);
	memset(WScTime,0,9);
	WSDayOfWeek=-1;
	WSInitCnt=0;
	WSSuspendMode=0;
	hIOPort=0;

	// GUI-implementation
	WShWnd=0;
	WShInst=0;
	WShDlgError=0;
	WShDlgEvent=0;
	WShDlgDebug=0;
	WSShowDlgDebug=0;
	WShDlgRecover=0;
	WSShowDlgRecover=0;
	WShDlgConList=0;
	WShConList=0;
	#ifdef WIN32
	memset(&WShDlgConList_LastRect,0,sizeof(RECT));
	#endif
	memset(AppVerStr,0,256);

	errhnd=evehnd=INVALID_FILE_VALUE;
	fpErrDate=fpEveDate=0;
	WSErrorLogPath="";
	WSEventLogPath="";
	WSDebugLogPath="";
	WSRetryLogPath="";
	WSRecoverLogPath="";

	#ifdef WS_FILE_SERVER
	memset(&WShDlgConList_LastRect,0,sizeof(RECT));
	WSFileXferInitCnt=0;
	WSFileTransferReady=FALSE;
	memset(&WSFileOptions,0,sizeof(FileOptions));
	fileTransfers=0;
	fileRecommits=0;
	#endif

	#ifdef WS_GUARANTEED
	WSUgcNoGap=0;
	#endif

	appMutex=CreateMutex(0,false,0);
	activeThread=0;
	#ifdef WS_OIO
	cxlOvlList=0;
	#endif

	SizeStr_zidx=0;
	memset(SizeStr_zstrs,0,sizeof(SizeStr_zstrs));

	lastTickDiff=0;
	lastTimeChange=0;

	// Compression and Encryption
	memset(SyncLoop_CompBuff,0,WS_MAX_BLOCK_SIZE*2048);	
	memset(SyncLoop_EncryptBuff,0,WS_MAX_BLOCK_SIZE*2048);	
	memset(ConRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(ConRead_szDecompBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(ConRead_szDecryptBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(ConRead_PxySrc,0,PROXYCOMPRESS_MAX);
	memset(UsrRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UsrRead_szDecompBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UsrRead_szDecryptBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(CtiRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UmrRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UmrRead_szDecompBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UmrRead_szDecryptBuff,0,WS_MAX_BLOCK_SIZE*1024);
	#ifdef WS_FILE_SERVER
	memset(FileRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(FileRead_szDecompBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(FileRead_szDecryptBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(FileTransfer_CompBuff,0,WS_MAX_BLOCK_SIZE*2048);
	#endif
	#ifdef WS_GUARANTEED
	memset(CgdRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(CgdRead_szDecompBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(CgdRead_szDecryptBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UgrRead_szTemp,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UgrRead_szDecompBuff,0,WS_MAX_BLOCK_SIZE*1024);
	memset(UgrRead_szDecryptBuff,0,WS_MAX_BLOCK_SIZE*1024);
	#endif

	ccodec=0;
	fcodec=0;
	lvl1Interval=1000;
	lvl1UpdateTime=0;
	lvl2Interval=1000;
	lvl2UpdateTime=0;
}
WsocksApp::~WsocksApp()
{
	if(appMutex)
	{
		DeleteMutex(appMutex); appMutex=0;
	}
}

// Built-in challenge response to validate versions
int WsocksApp::WSValidateModule(const char *challenge)
{
	// Last API date
	if(!strcmp(challenge,"WSOCKSAPI_DATE"))
		return WSOCKSAPI_DATE;
	// App size
	else if(!strcmp(challenge,"WsocksApp"))
		return sizeof(WsocksApp);
	else if(!strcmp(challenge,"AppConfig"))
		return sizeof(AppConfig);
	// When this function is called, __WSDATE__ and __WSTIME__ are set to the
	// default wsocksapi.lib build time. The app build time is set after WSHInitModule.
	else if(!strcmp(challenge,"BuildDate"))
	{
		char smon[8];
		int year=0,mon=0,day=0;
		sscanf(__WSDATE__,"%s %d %d",smon,&day,&year);
		static const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
		char *mptr=(char*)strstr(months,smon);
		if(mptr)
			mon=(int)(mptr -months)/3+1;
		return (year*10000)+(mon*100)+(day);
	}
	else if(!strcmp(challenge,"BuildTime"))
	{
		int hh=0,mm=0,ss=0;
		sscanf(__WSTIME__,"%d:%d:%d",&hh,&mm,&ss);
		return (hh*10000)+(mm*100)+(ss);
	}
	// Port sizes
	else if(!strcmp(challenge,"CONPORT"))
		return sizeof(CONPORT);
	else if(!strcmp(challenge,"USCPORT"))
		return sizeof(USCPORT);
	else if(!strcmp(challenge,"USRPORT"))
		return sizeof(USRPORT);
	else if(!strcmp(challenge,"FILEPORT"))
		return sizeof(FILEPORT);
	else if(!strcmp(challenge,"UMCPORT"))
		return sizeof(UMCPORT);
	else if(!strcmp(challenge,"UMRPORT"))
		return sizeof(UMRPORT);
	else if(!strcmp(challenge,"CGDPORT"))
		return sizeof(CGDPORT);
	else if(!strcmp(challenge,"UGCPORT"))
		return sizeof(UGCPORT);
	else if(!strcmp(challenge,"UGRPORT"))
		return sizeof(UGRPORT);
	else if(!strcmp(challenge,"CTOPORT"))
		return sizeof(CTOPORT);
	else if(!strcmp(challenge,"CTIPORT"))
		return sizeof(CTIPORT);
	else if(!strcmp(challenge,"OTHERPORT"))
		return sizeof(OTHERPORT);
	// Compile defs
	else if(!strcmp(challenge,"WS_MAX_BLOCK_SIZE"))
		return WS_MAX_BLOCK_SIZE;
	else if(!strcmp(challenge,"WS_MAX_ALT_PORTS"))
		return WS_MAX_ALT_PORTS;
	else if(!strcmp(challenge,"WS_FILE_SERVER"))
		#ifdef WS_FILE_SERVER
		return 1;
		#else
		return 0;
		#endif
	else if(!strcmp(challenge,"WS_GUARANTEED"))
		#ifdef WS_GUARANTEED
		return 1;
		#else
		return 0;
		#endif
	else if(!strcmp(challenge,"WS_OIO"))
		#ifdef WS_OIO
		return 1;
		#else
		return 0;
		#endif
	// Struct sizes
	else if(!strcmp(challenge,"WSRECORDING"))
		return sizeof(WSRECORDING);
	else if(!strcmp(challenge,"IPACL"))
		return sizeof(IPACL);
	else if(!strcmp(challenge,"WSOVERLAPPED"))
		#ifdef WS_OIO
		return sizeof(WSOVERLAPPED);
		#else
		return 0;
		#endif
	else if(!strcmp(challenge,"IPACLMAP"))
		return sizeof(IPACLMAP);
	else if(!strcmp(challenge,"GDID"))
		return sizeof(GDID);
	else if(!strcmp(challenge,"GDLOGIN"))
		return sizeof(GDLOGIN);
	else if(!strcmp(challenge,"GDACL"))
		return sizeof(GDACL);
	else if(!strcmp(challenge,"FIDX"))
		return sizeof(FIDX);
	else if(!strcmp(challenge,"FILEINDEX"))
		return sizeof(FILEINDEX);
	else if(!strcmp(challenge,"GDLINE"))
		return sizeof(GDLINE);
	else if(!strcmp(challenge,"VisualStudio"))
	{
	#ifdef VS2003
		return 2003;
	#elif defined(VS2005)
		return 2005;
	#elif defined(VS2010)
		return 2010;
	#elif defined(_CONSOLE)
		return 2003;
	#elif defined(GXX_323)
		return 323;
	#else
		#error A VisualStudio compiler version must be defined.
	#endif
	}
	else if(!strcmp(challenge,"CpuBits"))
	{
	#ifdef BIT64
		return 64;
	#else
		return 32;
	#endif
	}
	return -1;
}
bool WsocksApp::WSInitialized()
{
	return WSInitCnt>0?true:false;
}
int WsocksApp::WSPreInitSocks()
{
	return phost->WSHPreInitSocks(this);
}
int WsocksApp::WSInitSocks()
{
	return phost->WSHInitSocks(this);
}
int WsocksApp::WSSuspend()
{
	return phost->WSHSuspend(this);
}
int WsocksApp::WSResume()
{
	return phost->WSHResume(this);
}
int WsocksApp::WSCloseSocks()
{
	return phost->WSHCloseSocks(this);
}
int WsocksApp::WSExit(int ecode)
{
	return phost->WSHExitApp(this,ecode);
}
int WsocksApp::WSSyncLoop()
{
	return phost->WSHSyncLoop(this);
}
#ifdef WS_OIO
int WsocksApp::WSIocpLoop(int timeout)
{
	return phost->WSHIocpLoop(this,timeout);
}
#else
int WsocksApp::WSAsyncLoop()
{
	return phost->WSHAsyncLoop(this);
}
#endif

ULONGLONG WsocksApp::WSGetTimerCount()
{
	return phost->WSHGetTimerCount(this); 
}
DWORD WsocksApp::WSDiffTimerCount(ULONGLONG t1, ULONGLONG t2)
{
	return phost->WSHDiffTimerCount(this,t1,t2);
}
void WsocksApp::WSCheckSystemResourcesAvailable()
{
	return phost->WSHCheckSystemResourcesAvailable();
}
#ifdef WIN32
void WsocksApp::WSGetSystemResourcesAvailable(struct _PROCESS_MEMORY_COUNTERS *pProcessMemoryCounters)
{
	return phost->WSHGetSystemResourcesAvailable(pProcessMemoryCounters);
}
#endif
const char *WsocksApp::SizeStr(__int64 bigsize, bool byteUnits)
{
	return phost->WSHSizeStr(this,bigsize,byteUnits);
}

void WsocksApp::WSResetErrorCnt(void)
{
	phost->WSHResetErrorCnt(this);
}
void WsocksApp::WSLogEvent(const char *fmt,...)
{
	if(!phost)
		return;
	va_list alist;
	va_start(alist,fmt);
	phost->WSHLogEventVA(this,fmt,alist);
	va_end(alist);
}
void WsocksApp::WSLogError(const char *fmt,...)
{
	if(!phost)
		return;
	va_list alist;
	va_start(alist,fmt);
	phost->WSHLogErrorVA(this,fmt,alist);
	va_end(alist);
}
void WsocksApp::WSLogDebug(const char *fmt,...)
{
	if(!phost)
		return;
	va_list alist;
	va_start(alist,fmt);
	phost->WSHLogDebugVA(this,fmt,alist);
	va_end(alist);
}
void WsocksApp::WSLogRetry(const char *fmt,...)
{
	if(!phost)
		return;
	va_list alist;
	va_start(alist,fmt);
	phost->WSHLogRetryVA(this,fmt,alist);
	va_end(alist);
}
void WsocksApp::WSLogRecover(const char *fmt,...)
{
	if(!phost)
		return;
	va_list alist;
	va_start(alist,fmt);
	phost->WSHLogRecoverVA(this,fmt,alist);
	va_end(alist);
}
void WsocksApp::WSLogEventStr(const char *str)
{
	if(!phost)
		return;
	phost->WSHLogEventStr(this,str);
}
void WsocksApp::WSLogErrorStr(const char *str)
{
	if(!phost)
		return;
	phost->WSHLogErrorStr(this,str);
}
// This works but isn't the best solution
//// Returns a new string when necessary, otherwise null
//char *WsocksApp::WSLogCheckString(const char *str)
//{
//	if(strchr(str,'%'))
//	{
//		int alen=strlen(str) +64; // max 64 % sign replacements should be reasonable
//		char *tfmt=new char[alen];
//		char *tfptr=tfmt;
//		const char *tfend=tfmt +alen;
//		const char *fptr=str;
//		// Replace % with %%
//		for(const char *pptr=strchr(fptr,'%');(tfptr<tfend)&&(pptr)&&(*fptr);)
//		{
//			int clen=(int)(pptr - fptr);
//			memcpy(tfptr,fptr,clen); tfptr+=clen; 
//			memcpy(tfptr,"%%",3); tfptr+=2;
//			fptr+=clen+1; pptr=strchr(fptr,'%');
//		}
//		// Copy the rest
//		if((tfptr +strlen(fptr) +1<tfend)&&(*fptr))
//			strcpy(tfptr,fptr);
//		return tfmt;
//	}
//	return 0;
//}
//void WsocksApp::WSLogFreeString(char *tstr)
//{
//	if(tstr)
//		delete tstr;
//}
void WsocksApp::WSLogEventVA(const char *fmt, va_list& alist)
{
	if(!phost)
		return;
	phost->WSHLogEventVA(this,fmt,alist);
}
void WsocksApp::WSLogErrorVA(const char *fmt, va_list& alist)
{
	if(!phost)
		return;
	phost->WSHLogErrorVA(this,fmt,alist);
}
void WsocksApp::WSLogDebugVA(const char *fmt, va_list& alist)
{
	if(!phost)
		return;
	phost->WSHLogDebugVA(this,fmt,alist);
}
void WsocksApp::WSLogRetryVA(const char *fmt, va_list& alist)
{
	if(!phost)
		return;
	phost->WSHLogRetryVA(this,fmt,alist);
}
void WsocksApp::WSLogRecoverVA(const char *fmt, va_list& alist)
{
	if(!phost)
		return;
	phost->WSHLogRecoverVA(this,fmt,alist);
}
void WsocksApp::WSSetAppHead(char *Head)
{
	phost->WSHSetAppHead(this,Head);
}
void WsocksApp::WSSetAppStatus(char *Appver, char *Status)
{
	if(Appver)
	{
		strncpy(AppVerStr,Appver,sizeof(AppVerStr)-1);
		AppVerStr[sizeof(AppVerStr)-1]=0;
	}
	phost->WSHSetAppStatus(this,Status);
}
int WsocksApp::WSGetAppHead(char *heading, int hlen)
{
    return(phost->WSHGetAppHead(this, heading, hlen));
}
int WsocksApp::WSGetAppStatus(char *status, int slen)
{
    return(phost->WSHGetAppStatus(this, status, slen));
}
int WsocksApp::WSGetHostBuildDate()
{
    return(phost->WSHGetHostBuildDate());
}
int WsocksApp::WSGetHostBuildTime()
{
    return(phost->WSHGetHostBuildTime());
}

int WsocksApp::WSSetMaxPorts(WSPortType PortType, int NumPorts)
{
	return phost->WSHSetMaxPorts(this,PortType,NumPorts);
}
WSPort *WsocksApp::WSCreatePort(WSPortType PortType, int PortNo)
{
	return phost->WSHCreatePort(this,PortType,PortNo);
}
WSPort *WsocksApp::WSGetPort(WSPortType PortType, int PortNo)
{
	return phost->WSHGetPort(this,PortType,PortNo);
}
int WsocksApp::WSGetPorts(WSPortType PortType, WSPORTLIST& portList)
{
	return phost->WSHGetPorts(this,PortType,portList);
}
int WsocksApp::WSUpdatePort(WSPortType PortType, int PortNo)
{
	return phost->WSHUpdatePort(this,PortType,PortNo);
}
int WsocksApp::WSDeletePort(WSPortType PortType, int PortNo)
{
	return phost->WSHDeletePort(this,PortType,PortNo);
}
int WsocksApp::WSLockPort(WSPortType PortType, int PortNo, bool send)
{
	return phost->WSHLockPort(this,PortType,PortNo,send);
}
int WsocksApp::WSUnlockPort(WSPortType PortType, int PortNo, bool send)
{
	return phost->WSHUnlockPort(this,PortType,PortNo,send);
}

int WsocksApp::WSGetTagValues(TVMAP& tvmap, const char *parm, int plen)
{
	return phost->WSHGetTagValues(this,tvmap,parm,plen);
}
string WsocksApp::WSGetValue(TVMAP& tvmap, string tag)
{
	return phost->WSHGetValue(this,tvmap,tag);
}
int WsocksApp::WSSetTagValues(TVMAP& tvmap, char *parm, int plen)
{
	return phost->WSHSetTagValues(this,tvmap,parm,plen);
}
int WsocksApp::WSSetValue(TVMAP& tvmap, string tag, string value)
{
	return phost->WSHSetValue(this,tvmap,tag,value);
}
int WsocksApp::WSDelTag(TVMAP& tvmap, string tag)
{
	return phost->WSHDelTag(this,tvmap,tag);
}
int WsocksApp::WSSysmonResp(int UmrPortNo, int reqid, string cmd, const char *parm, int plen)
{
	return phost->WSHSysmonResp(this,UmrPortNo,reqid,cmd,parm,plen);
}

int WsocksApp::WSSetupConPorts(int SMode, int PortNo)
{
	return phost->WSHSetupConPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenConPort(int PortNo)
{
	return phost->WSHOpenConPort(this,PortNo);
}
int WsocksApp::WSConConnect(int PortNo)
{
	return phost->WSHConConnect(this,PortNo);
}
int WsocksApp::WSConSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	return phost->WSHConSendMsg(this,MsgID,MsgLen,MsgOut,PortNo);
}
int WsocksApp::WSConSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	return phost->WSHConSendBuff(this,MsgLen,MsgOut,PortNo,Packets);
}
int WsocksApp::WSConSendBuffNew(int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend)
{
	return phost->WSHConSendBuffNew(this,MsgLen,MsgOut,PortNo,Packets,ForceSend);
}
int WsocksApp::WSCloseConPort(int PortNo)
{
	return phost->WSHCloseConPort(this,PortNo);
}
int WsocksApp::WSDecompressed(int PortType,int PortNo,char *DecompBuff,int DecompSize,char PxySrc[PROXYCOMPRESS_MAX])
{
	return phost->WSHDecompressed(this,PortType,PortNo,DecompBuff,DecompSize,PxySrc);
}

int WsocksApp::WSSetupUscPorts(int SMode, int PortNo)
{
	return phost->WSHSetupUscPorts(this,SMode,PortNo);
}
int WsocksApp::WSCfgUscPort(int PortNo)
{
	return phost->WSHCfgUscPort(this,PortNo);
}
int WsocksApp::WSOpenUscPort(int PortNo)
{
	return phost->WSHOpenUscPort(this,PortNo);
}
int WsocksApp::WSCloseUscPort(int PortNo)
{
	return phost->WSHCloseUscPort(this,PortNo);
}

int WsocksApp::WSSetupUsrPorts(int SMode, int PortNo)
{
	return phost->WSHSetupUsrPorts(this,SMode,PortNo);
}
int WsocksApp::WSUsrSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	return phost->WSHUsrSendMsg(this,MsgID,MsgLen,MsgOut,PortNo);
}
int WsocksApp::WSUsrSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	return phost->WSHUsrSendBuff(this,MsgLen,MsgOut,PortNo,Packets);
}
int WsocksApp::WSCloseUsrPort(int PortNo)
{
	return phost->WSHCloseUsrPort(this,PortNo);
}

#ifdef WS_FILE_SERVER
int WsocksApp::WSSetupFilePorts(int SMode, int PortNo)
{
	return phost->WSHSetupFilePorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenFilePort(int PortNo)
{
	return phost->WSHOpenFilePort(this,PortNo);
}
int WsocksApp::WSFileConnect(int PortNo)
{
	return phost->WSHFileConnect(this,PortNo);
}
int WsocksApp::WSCloseFilePort(int PortNo)
{
	return phost->WSHCloseFilePort(this,PortNo);
}
#endif

int WsocksApp::WSSetupUmcPorts(int SMode, int PortNo)
{
	return phost->WSHSetupUmcPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenUmcPort(int PortNo)
{
	return phost->WSHOpenUmcPort(this,PortNo);
}
int WsocksApp::WSCloseUmcPort(int PortNo)
{
	return phost->WSHCloseUmcPort(this,PortNo);
}

int WsocksApp::WSSetupUmrPorts(int SMode, int PortNo)
{
	return phost->WSHSetupUmrPorts(this,SMode,PortNo);
}
int WsocksApp::WSUmrSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	return phost->WSHUmrSendMsg(this,MsgID,MsgLen,MsgOut,PortNo);
}
int WsocksApp::WSUmrSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	return phost->WSHUmrSendBuff(this,MsgLen,MsgOut,PortNo,Packets);
}
int WsocksApp::WSCloseUmrPort(int PortNo)
{
	return phost->WSHCloseUmrPort(this,PortNo);
}

#ifdef WS_GUARANTEED
void WsocksApp::WSGetCgdId(int PortNo, GDID *GDId)
{
	phost->WSHGetCgdId(this,PortNo,GDId);
}
int WsocksApp::WSSetupCgdPorts(int SMode, int PortNo)
{
	return phost->WSHSetupCgdPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenCgdPort(int PortNo)
{
	return phost->WSHOpenCgdPort(this,PortNo);
}
int WsocksApp::WSCgdConnect(int PortNo)
{
	return phost->WSHCgdConnect(this,PortNo);
}
int WsocksApp::WSCgdSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	return phost->WSHCgdSendMsg(this,MsgID,MsgLen,MsgOut,PortNo);
}
int WsocksApp::WSCloseCgdPort(int PortNo)
{
	return phost->WSHCloseCgdPort(this,PortNo);
}
int WsocksApp::WSCgdMsgReady(int PortNo)
{
	return phost->WSHCgdMsgReady(this,PortNo);
}
int WsocksApp::WSCgdStripMsg(int PortNo)
{
	return phost->WSHCgdStripMsg(this,PortNo);
}

int WsocksApp::WSSetupUgcPorts(int SMode, int PortNo)
{
	return phost->WSHSetupUgcPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenUgcPort(int PortNo)
{
	return phost->WSHOpenUgcPort(this,PortNo);
}
int WsocksApp::WSUgcSendMsg(WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, WORD LineId)
{
	return phost->WSHUgcSendMsg(this,MsgID,MsgLen,MsgOut,PortNo,LineId);
}
int WsocksApp::WSCloseUgcPort(int PortNo)
{
	return phost->WSHCloseUgcPort(this,PortNo);
}
int WsocksApp::WSCfgUgcPort(int PortNo)
{
	return phost->WSHCfgUgcPort(this,PortNo);
}

int WsocksApp::WSSetupUgrPorts(int SMode, int PortNo)
{
	return phost->WSHSetupUgrPorts(this,SMode,PortNo);
}
int WsocksApp::WSUgrSendMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo,unsigned int GDLogId)
{
	return phost->WSHUgrSendMsg(this,MsgID,MsgLen,MsgOut,PortNo,GDLogId);
}
int WsocksApp::WSUgrSendNGMsg(WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	return phost->WSHUgrSendNGMsg(this,MsgID,MsgLen,MsgOut,PortNo);
}
int WsocksApp::WSCloseUgrPort(int PortNo)
{
	return phost->WSHCloseUgrPort(this,PortNo);
}
void WsocksApp::WSGetUgcId(int PortNo, char SessionId[20])
{
	phost->WSHGetUgcId(this,PortNo,SessionId);
}
int WsocksApp::WSUgrMsgReady(int PortNo)
{
	return phost->WSHUgrMsgReady(this,PortNo);
}
int WsocksApp::WSUgrStripMsg(int PortNo)
{
	return phost->WSHUgrStripMsg(this,PortNo);
}
#endif

int WsocksApp::WSSetupCtoPorts(int SMode, int PortNo)
{
	return phost->WSHSetupCtoPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenCtoPort(int PortNo)
{
	return phost->WSHOpenCtoPort(this,PortNo);
}
int WsocksApp::WSCtoSendBuff(int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	return phost->WSHCtoSendBuff(this,MsgLen,MsgOut,PortNo,Packets);
}
int WsocksApp::WSCloseCtoPort(int PortNo)
{
	return phost->WSHCloseCtoPort(this,PortNo);
}

int WsocksApp::WSSetupCtiPorts(int SMode, int PortNo)
{
	return phost->WSHSetupCtiPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenCtiPort(int PortNo)
{
	return phost->WSHOpenCtiPort(this,PortNo);
}
int WsocksApp::WSCloseCtiPort(int PortNo)
{
	return phost->WSHCloseCtiPort(this,PortNo);
}

int WsocksApp::WSSetupOtherPorts(int SMode, int PortNo)
{
	return phost->WSHSetupOtherPorts(this,SMode,PortNo);
}
int WsocksApp::WSOpenOtherPort(int PortNo)
{
	return phost->WSHOpenOtherPort(this,PortNo);
}
int WsocksApp::WSCloseOtherPort(int PortNo)
{
	return phost->WSHCloseOtherPort(this,PortNo);
}

int WsocksApp::WSMakeLocalDirs(const char *lpath)
{
	return phost->WSHMakeLocalDirs(this,lpath);
}
int WsocksApp::WSBeginUpload(const char *spath, const char *DeliverTo, const char *OnBehalfOf, 
							 int rid, const char *fpath, WSPortType PortType, int PortNo, LONGLONG begin)
{
	return phost->WSHBeginUpload(this,spath,DeliverTo,OnBehalfOf,rid,fpath,PortType,PortNo,begin);
}
int WsocksApp::WSSendFile(const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
						  int rid, const char *fpath, WSPortType PortType, int PortNo,
						  LONGLONG begin, LONGLONG rsize, LONGLONG tsize, DWORD ssize, bool multi)
{
	return phost->WSHSendFile(this,fkey,DeliverTo,OnBehalfOf,rid,fpath,PortType,PortNo,begin,rsize,tsize,ssize,multi);
}
int WsocksApp::WSRecvFile(const char *Msg, int MsgLen, WSPortType PortType, int PortNo,
						  int *rid, char *fkey, int klen, char *lpath, int llen, char *DeliverTo, WORD dlen, char *OnBehalfOf, WORD blen)
{
	return phost->WSHRecvFile(this,Msg,MsgLen,PortType,PortNo,rid,fkey,klen,lpath,llen,DeliverTo,dlen,OnBehalfOf,blen);
}
#ifdef WS_FILE_SERVER
BOOL WsocksApp::WSReplaceFile(const char *spath, const char *dpath)
{
	return phost->WSHReplaceFile(this,spath,dpath);
}
int WsocksApp::WSFileMsgReady(int PortNo)
{
	return phost->WSHFileMsgReady(this,PortNo);
}
int WsocksApp::WSFileStripMsg(int PortNo)
{
	return phost->WSHFileStripMsg(this,PortNo);
}
int WsocksApp::WSBeforeFileSend(int PortNo)
{
	return phost->WSHBeforeFileSend(this,PortNo);
}
#endif//WS_FILE_SERVER

long WsocksApp::WSDiffTime(long tnew, long told)
{
	return phost->WSHDiffTime(tnew,told);
}

int WsocksApp::WSUmrMsgReady(int PortNo)
{
	return phost->WSHUmrMsgReady(this,PortNo);
}
int WsocksApp::WSUmrStripMsg(int PortNo)
{
	return phost->WSHUmrStripMsg(this,PortNo);
}
//int WsocksApp::WSUmrSendHealth(int PortNo, int MsgDate, int MsgTime)
//{
//	return phost->WSHUmrSendHealth(this,PortNo,MsgDate,MsgTime);
//}
//int WsocksApp::WSUmrSetHealth(int index, int code, const char *desc)
//{
//	return phost->WSHUmrSetHealth(this,index,code,desc);
//}
int WsocksApp::WSSysmonFeed(int UmrPortNo, string feed, const char *buf, int blen)
{
	return phost->WSHSysmonFeed(this,UmrPortNo,feed,buf,blen);
}

//int WsocksApp::WSPortChanged(CONPORT_1_0& cport, int PortNo)
//{
//	return phost->WSHPortChanged(this,cport,PortNo);
//}
//int WsocksApp::WSPortChanged(CONPORT_1_1& cport, int PortNo)
//{
//	return phost->WSHPortChanged(this,cport,PortNo);
//}
//int WsocksApp::WSPortChanged(USCPORT_1_0& uport, int PortNo)
//{
//	return phost->WSHPortChanged(this,uport,PortNo);
//}
//int WsocksApp::WSPortChanged(USCPORT_1_1& uport, int PortNo)
//{
//	return phost->WSHPortChanged(this,uport,PortNo);
//}
//int WsocksApp::WSPortChanged(USRPORT_1_0& rport, int PortNo)
//{
//	return phost->WSHPortChanged(this,rport,PortNo);
//}
//int WsocksApp::WSPortChanged(USRPORT_1_1& rport, int PortNo)
//{
//	return phost->WSHPortChanged(this,rport,PortNo);
//}
//int WsocksApp::WSPortChanged(FILEPORT_1_0& fport, int PortNo)
//{
//	return phost->WSHPortChanged(this,fport,PortNo);
//}
//int WsocksApp::WSPortChanged(FILEPORT_1_1& fport, int PortNo)
//{
//	return phost->WSHPortChanged(this,fport,PortNo);
//}
//int WsocksApp::WSPortChanged(UMCPORT_1_0& uport, int PortNo)
//{
//	return phost->WSHPortChanged(this,uport,PortNo);
//}
//int WsocksApp::WSPortChanged(UMCPORT_1_1& uport, int PortNo)
//{
//	return phost->WSHPortChanged(this,uport,PortNo);
//}
//int WsocksApp::WSPortChanged(UMRPORT_1_0& rport, int PortNo)
//{
//	return phost->WSHPortChanged(this,rport,PortNo);
//}
//int WsocksApp::WSPortChanged(UMRPORT_1_1& rport, int PortNo)
//{
//	return phost->WSHPortChanged(this,rport,PortNo);
//}
//int WsocksApp::WSPortChanged(CGDPORT_1_0& cport, int PortNo)
//{
//	return phost->WSHPortChanged(this,cport,PortNo);
//}
//int WsocksApp::WSPortChanged(CGDPORT_1_1& cport, int PortNo)
//{
//	return phost->WSHPortChanged(this,cport,PortNo);
//}
//int WsocksApp::WSPortChanged(UGCPORT_1_0& uport, int PortNo)
//{
//	return phost->WSHPortChanged(this,uport,PortNo);
//}
//int WsocksApp::WSPortChanged(UGCPORT_1_1& uport, int PortNo)
//{
//	return phost->WSHPortChanged(this,uport,PortNo);
//}
//int WsocksApp::WSPortChanged(UGRPORT_1_0& rport, int PortNo)
//{
//	return phost->WSHPortChanged(this,rport,PortNo);
//}
//int WsocksApp::WSPortChanged(UGRPORT_1_1& rport, int PortNo)
//{
//	return phost->WSHPortChanged(this,rport,PortNo);
//}
//int WsocksApp::WSPortChanged(CTOPORT_1_0& oport, int PortNo)
//{
//	return phost->WSHPortChanged(this,oport,PortNo);
//}
//int WsocksApp::WSPortChanged(CTOPORT_1_1& oport, int PortNo)
//{
//	return phost->WSHPortChanged(this,oport,PortNo);
//}
//int WsocksApp::WSPortChanged(CTIPORT_1_0& iport, int PortNo)
//{
//	return phost->WSHPortChanged(this,iport,PortNo);
//}
//int WsocksApp::WSPortChanged(CTIPORT_1_1& iport, int PortNo)
//{
//	return phost->WSHPortChanged(this,iport,PortNo);
//}
//int WsocksApp::WSPortChanged(OTHERPORT_1_0& oport, int PortNo)
//{
//	return phost->WSHPortChanged(this,oport,PortNo);
//}

#ifdef WS_FILE_SERVER
BOOL WsocksApp::WSTransmitFile(LPCTSTR lpLocalFile, LPCTSTR lpRemoteFile, 
							   const FILETIME *lpCreationTime, 
							   const FILETIME *lpLastAccessTime, 
							   const FILETIME *lpLastWriteTime, FileOptions *opts)
{
	return phost->WSHTransmitFile(this,lpLocalFile,lpRemoteFile,lpCreationTime,lpLastAccessTime,lpLastWriteTime,opts);
}
BOOL WsocksApp::WSDownloadFile(LPCTSTR lpRemoteFile, LPCTSTR lpLocalFile, FileOptions *opts)
{
	return phost->WSHDownloadFile(this,lpRemoteFile,lpLocalFile,opts);
}
HANDLE WsocksApp::WSFindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, FileOptions *opts)
{
	return phost->WSHFindFirstFile(this,lpFileName,lpFindFileData,opts);
}
BOOL WsocksApp::WSFindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
	return phost->WSHFindNextFile(this,hFindFile,lpFindFileData);
}
BOOL WsocksApp::WSFindClose(HANDLE hFindFile)
{
	return phost->WSHFindClose(this,hFindFile);
}
int WsocksApp::WSPathFileExists(LPCTSTR pszPath, FileOptions *opts)
{
	return phost->WSHPathFileExists(this,pszPath,opts);
}
int WsocksApp::WSPathIsDirectory(LPCTSTR pszPath, FileOptions *opts)
{
	return phost->WSHPathIsDirectory(this,pszPath,opts);
}
BOOL WsocksApp::WSCreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes, FileOptions *opts)
{
	return phost->WSHCreateDirectory(this,lpPathName,lpSecurityAttributes,opts);
}
BOOL WsocksApp::WSDeleteFile(LPCTSTR pszPath, BOOL bOverrideAttributes, FileOptions *opts)
{
	return phost->WSHDeleteFile(this,pszPath,bOverrideAttributes,opts);
}
#endif

int WsocksApp::WSOpenRecording(WSRECORDING *wsr, HWND parent, int Type, int PortNo)
{
	return phost->WSHOpenRecording(this,wsr,parent,Type,PortNo);
}
void WsocksApp::WSCloseRecording(WSRECORDING *wsr, int Type, int PortNo)
{
	return phost->WSHCloseRecording(this,wsr,Type,PortNo);
}
int WsocksApp::WSRecord(WSRECORDING *wsr, const char *buf, int len, BOOL send)
{
	return phost->WSHRecord(this,wsr,buf,len,send);
}
int WsocksApp::WSBusyThreadReport(DWORD estEndTime)
{
	return phost->WSHBusyThreadReport(this,estEndTime);
}
int WsocksApp::WSRestart(int timeout)
{
	return phost->WSHRestart(this,timeout);
}
int WsocksApp::WSResetCryptKeys(int PortType, int PortNo)
{
	return phost->WSHResetCryptKeys(this,PortType,PortNo);
}
int WsocksApp::WSBackupApp(char bpath[MAX_PATH], char emsg[256], bool overwrite)
{
	return phost->WSHBackupApp(this,bpath,emsg,overwrite);
}
int WsocksApp::WSRestoreApp(char bpath[MAX_PATH], char emsg[256])
{
	return phost->WSHRestoreApp(this,bpath,emsg);
}
bool WsocksApp::WSTranslateAlias(char *ipstr, int len)
{
	return phost->WSHTranslateAlias(ipstr,len);
}
#ifdef WIN32
// Debugging
int WsocksApp::WSGenerateDump(DWORD tid, EXCEPTION_POINTERS *pException)
{
	return phost->WSHGenerateDump(this,tid,pException);
}
#endif
