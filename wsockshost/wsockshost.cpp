#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#include "wstring.h"

// OS-specific implementation
#ifdef WIN32
#include <shlwapi.h>
#include <crtdbg.h>
#include <time.h>

#define WS_LOOPBACK

#ifdef _DEBUG
// To help debug deadlocks, uncomment these functions and set 'chkmux' at mutex creation time.
//int WsocksHostImpl::mcnt=0;
//HANDLE WsocksHostImpl::chkmux=0;
DWORD WsocksHostImpl::WaitForSingleObject(HANDLE hMutex, DWORD dwMilliseconds)
{
	_ASSERT(hMutex);
	int rc=::WaitForSingleObject(hMutex,dwMilliseconds);
	//if((rc==WAIT_OBJECT_0)&&(chkmux==hMutex))
	//{
	//	_ASSERT(mcnt>=0);
	//	mcnt++;
	//}
	return rc;
}
BOOL WsocksHostImpl::ReleaseMutex(HANDLE hMutex)
{
	_ASSERT(hMutex);
	//if(chkmux==hMutex)
	//{
	//	_ASSERT(mcnt>0);
	//	mcnt--;
	//}
	return ::ReleaseMutex(hMutex);
}
void WsocksHostImpl::DeleteMutex(HANDLE hMutex)
{
	_ASSERT(hMutex);
	//if(chkmux==hMutex)
	//{
	//	_ASSERT(mcnt>=0);
	//}
	::CloseHandle(hMutex);
}
#endif
int WsocksHostImpl::LockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send, DWORD timeout)
{
	if(!pmod)
		return WAIT_FAILED;
	HANDLE *pmux=0;
	DWORD *prcnt=0;
	char pstr[8]={0};
	switch(PortType)
	{
	case WS_CON: if((PortNo<0)||(PortNo>=pmod->NO_OF_CON_PORTS)) break;
				 pmux=send ?&pmod->ConPort[PortNo].smutex :&pmod->ConPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->ConPort[PortNo].smutcnt :&pmod->ConPort[PortNo].rmutcnt; pstr[0]='C'; break;
	case WS_USC: if((PortNo<0)||(PortNo>=pmod->NO_OF_USC_PORTS)) break;
				 pmux=&pmod->UscPort[PortNo].tmutex; pstr[0]='U'; break;
	case WS_USR: if((PortNo<0)||(PortNo>=pmod->NO_OF_USR_PORTS)) break;
				 pmux=send ?&pmod->UsrPort[PortNo].smutex :&pmod->UsrPort[PortNo].rmutex;
				 prcnt=send ?&pmod->UsrPort[PortNo].smutcnt :&pmod->UsrPort[PortNo].rmutcnt; pstr[0]='u'; break;
	case WS_UMC: if((PortNo<0)||(PortNo>=pmod->NO_OF_UMC_PORTS)) break;
				 pmux=&pmod->UmcPort[PortNo].tmutex; pstr[0]='M'; break;
	case WS_UMR: if((PortNo<0)||(PortNo>=pmod->NO_OF_UMR_PORTS)) break;
				 pmux=send ?&pmod->UmrPort[PortNo].smutex :&pmod->UmrPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->UmrPort[PortNo].smutcnt :&pmod->UmrPort[PortNo].rmutcnt; pstr[0]='m'; break;
	case WS_FIL: if((PortNo<0)||(PortNo>=pmod->NO_OF_FILE_PORTS)) break;
				 pmux=send ?&pmod->FilePort[PortNo].smutex :&pmod->FilePort[PortNo].rmutex; 
				 prcnt=send ?&pmod->FilePort[PortNo].smutcnt :&pmod->FilePort[PortNo].rmutcnt; pstr[0]='F'; break;
	case WS_CGD: if((PortNo<0)||(PortNo>=pmod->NO_OF_CGD_PORTS)) break;
				 pmux=send ?&pmod->CgdPort[PortNo].smutex :&pmod->CgdPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->CgdPort[PortNo].smutcnt :&pmod->CgdPort[PortNo].rmutcnt; pstr[0]='G'; break;
	case WS_UGC: if((PortNo<0)||(PortNo>=pmod->NO_OF_UGC_PORTS)) break;
				 pmux=&pmod->UgcPort[PortNo].tmutex; pstr[0]='G'; break;
	case WS_UGR: if((PortNo<0)||(PortNo>=pmod->NO_OF_UGR_PORTS)) break;
				 pmux=send ?&pmod->UgrPort[PortNo].smutex :&pmod->UgrPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->UgrPort[PortNo].smutcnt :&pmod->UgrPort[PortNo].rmutcnt; pstr[0]='g'; break;
	case WS_CTO: if((PortNo<0)||(PortNo>=pmod->NO_OF_CTO_PORTS)) break;
				 pmux=&pmod->CtoPort[PortNo].smutex;
				 prcnt=send ?&pmod->CtoPort[PortNo].smutcnt :0; pstr[0]='O'; break;
	case WS_CTI: if((PortNo<0)||(PortNo>=pmod->NO_OF_CTI_PORTS)) break;
				 pmux=&pmod->CtiPort[PortNo].rmutex;
				 prcnt=send ?0 :&pmod->CtiPort[PortNo].rmutcnt; pstr[0]='I'; break;
	default: return WAIT_FAILED;
	};
	if((!pmux)||(!*pmux))
		return WAIT_FAILED;
	int rc=WAIT_FAILED;
	if(prcnt)
	{
		#ifdef MUX_CRIT_SECTION
		EnterCriticalSection(&lockMux);
		#else
		WaitForSingleObject(lockMux,INFINITE);
		#endif
		(*prcnt)++;
		// Leave a bread crumb
		DWORD tid=GetCurrentThreadId();
		WSTHREADIDMAP::iterator tit=threadIdMap.find(tid);
		WsocksThread *pthread=0;
		int plen=0;
		if(tit!=threadIdMap.end())
		{
			pthread=tit->second;
			_ASSERT(pthread->tid==tid);
			sprintf(pstr +1,"%c%d",send?'s':'r',PortNo);
			plen=(int)strlen(pstr);
			if(pthread->muxtptr +plen<pthread->muxtrail +sizeof(pthread->muxtrail))
			{
				strcpy(pthread->muxtptr,pstr); pthread->muxtptr+=plen;
			}
		}
		#ifdef MUX_CRIT_SECTION
		LeaveCriticalSection(&lockMux);
		#else
		ReleaseMutex(lockMux);
		#endif

		rc=WaitForSingleObject(*pmux,timeout);
		if(rc!=WAIT_OBJECT_0)
		{
		#ifdef MUX_CRIT_SECTION
			EnterCriticalSection(&lockMux);
		#else
			WaitForSingleObject(lockMux,INFINITE);
		#endif
			if(((*prcnt)&0x0FFFFFFF)>0)
				(*prcnt)--;
			else
				_ASSERT(false);
			// Pick up bread crumb
			if(pthread)
			{
				pthread->muxtptr-=plen; memset(pthread->muxtrail,0,plen);
			}
			#ifdef MUX_CRIT_SECTION
			LeaveCriticalSection(&lockMux);
			#else
			ReleaseMutex(lockMux);
			#endif
		}
	}
	else
		rc=WaitForSingleObject(*pmux,timeout);
	return rc;
}
int WsocksHostImpl::UnlockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send)
{
	if(!pmod)
		return WAIT_FAILED;
	HANDLE *pmux=0;
	DWORD *prcnt=0;
	char pstr[8]={0};
	switch(PortType)
	{
	case WS_CON: pmux=send ?&pmod->ConPort[PortNo].smutex :&pmod->ConPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->ConPort[PortNo].smutcnt :&pmod->ConPort[PortNo].rmutcnt; pstr[0]='C'; break;
	case WS_USC: pmux=&pmod->UscPort[PortNo].tmutex; pstr[0]='U'; break;
	case WS_USR: pmux=send ?&pmod->UsrPort[PortNo].smutex :&pmod->UsrPort[PortNo].rmutex;
				 prcnt=send ?&pmod->UsrPort[PortNo].smutcnt :&pmod->UsrPort[PortNo].rmutcnt; pstr[0]='u'; break;
	case WS_UMC: pmux=&pmod->UmcPort[PortNo].tmutex; pstr[0]='M'; break;
	case WS_UMR: pmux=send ?&pmod->UmrPort[PortNo].smutex :&pmod->UmrPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->UmrPort[PortNo].smutcnt :&pmod->UmrPort[PortNo].rmutcnt; pstr[0]='m'; break;
	case WS_FIL: pmux=send ?&pmod->FilePort[PortNo].smutex :&pmod->FilePort[PortNo].rmutex; 
				 prcnt=send ?&pmod->FilePort[PortNo].smutcnt :&pmod->FilePort[PortNo].rmutcnt; pstr[0]='F'; break;
	case WS_CGD: pmux=send ?&pmod->CgdPort[PortNo].smutex :&pmod->CgdPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->CgdPort[PortNo].smutcnt :&pmod->CgdPort[PortNo].rmutcnt; pstr[0]='G'; break;
	case WS_UGC: pmux=&pmod->UgcPort[PortNo].tmutex; pstr[0]='G'; break;
	case WS_UGR: pmux=send ?&pmod->UgrPort[PortNo].smutex :&pmod->UgrPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->UgrPort[PortNo].smutcnt :&pmod->UgrPort[PortNo].rmutcnt; pstr[0]='g'; break;
	case WS_CTO: pmux=&pmod->CtoPort[PortNo].smutex;
				 prcnt=send ?&pmod->CtoPort[PortNo].smutcnt :0; pstr[0]='O'; break;
	case WS_CTI: pmux=&pmod->CtiPort[PortNo].rmutex;
				 prcnt=send ?0 :&pmod->CtiPort[PortNo].rmutcnt; pstr[0]='I'; break;
	default: return WAIT_FAILED;
	};
	if(!*pmux)
		return WAIT_FAILED;
	if(prcnt)
	{
		#ifdef MUX_CRIT_SECTION
		EnterCriticalSection(&lockMux);
		#else
		WaitForSingleObject(lockMux,INFINITE);
		#endif
		// Pick up bread crumb
		DWORD tid=GetCurrentThreadId();
		WSTHREADIDMAP::iterator tit=threadIdMap.find(tid);
		if(tit!=threadIdMap.end())
		{
			WsocksThread *pthread=tit->second;
			_ASSERT(pthread->tid==tid);
			sprintf(pstr +1,"%c%d",send?'s':'r',PortNo);
			int plen=(int)strlen(pstr);
			if((pthread->muxtptr>=pthread->muxtrail +plen)&&(!strcmp(pthread->muxtptr -plen,pstr)))
			{
				pthread->muxtptr-=plen; memset(pthread->muxtptr,0,plen);
			}
			else
				_ASSERT(false);
		}
		if(((*prcnt)&0x0FFFFFFF)>0)
			(*prcnt)--;
		else
			;//_ASSERT(false);
		// Delayed mutex close
		if(*prcnt==0x80000000)
		{
			DeleteMutex(*pmux); *pmux=0; *prcnt=0;
		}
		else
		{
			if(!ReleaseMutex(*pmux))
				_ASSERT(false);
		}
		#ifdef MUX_CRIT_SECTION
		LeaveCriticalSection(&lockMux);
		#else
		ReleaseMutex(lockMux);
		#endif
	}
	else
		if(!ReleaseMutex(*pmux))
			_ASSERT(false);
	return 0;
}
void WsocksHostImpl::DeletePortMutex(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send)
{
	HANDLE *pmux=0;
	DWORD *prcnt=0;
	char pstr[8]={0};
	switch(PortType)
	{
	case WS_CON: pmux=send ?&pmod->ConPort[PortNo].smutex :&pmod->ConPort[PortNo].rmutex; pstr[0]='C'; break;
	case WS_USC: pmux=&pmod->UscPort[PortNo].tmutex; pstr[0]='U'; break;
	case WS_USR: pmux=send ?&pmod->UsrPort[PortNo].smutex :&pmod->UsrPort[PortNo].rmutex;
				 prcnt=send ?&pmod->UsrPort[PortNo].smutcnt :&pmod->UsrPort[PortNo].rmutcnt; pstr[0]='u'; break;
	case WS_UMC: pmux=&pmod->UmcPort[PortNo].tmutex; pstr[0]='M'; break;
	case WS_UMR: pmux=send ?&pmod->UmrPort[PortNo].smutex :&pmod->UmrPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->UmrPort[PortNo].smutcnt :&pmod->UmrPort[PortNo].rmutcnt; pstr[0]='m'; break;
	case WS_FIL: pmux=send ?&pmod->FilePort[PortNo].smutex :&pmod->FilePort[PortNo].rmutex; pstr[0]='F'; break;
	case WS_CGD: pmux=send ?&pmod->CgdPort[PortNo].smutex :&pmod->CgdPort[PortNo].rmutex; pstr[0]='G'; break;
	case WS_UGC: pmux=&pmod->UgcPort[PortNo].tmutex; pstr[0]='G'; break;
	case WS_UGR: pmux=send ?&pmod->UgrPort[PortNo].smutex :&pmod->UgrPort[PortNo].rmutex; 
				 prcnt=send ?&pmod->UgrPort[PortNo].smutcnt :&pmod->UgrPort[PortNo].rmutcnt; pstr[0]='g'; break;
	case WS_CTO: pmux=&pmod->CtoPort[PortNo].smutex; pstr[0]='O'; break;
	case WS_CTI: pmux=&pmod->CtiPort[PortNo].rmutex; pstr[0]='I'; break;
	default: return;
	};
	if(!*pmux)
		return;
	if(prcnt)
	{
		#ifdef MUX_CRIT_SECTION
		EnterCriticalSection(&lockMux);
		#else
		WaitForSingleObject(lockMux,INFINITE);
		#endif
		// Pick up bread crumb
		DWORD tid=GetCurrentThreadId();
		WSTHREADIDMAP::iterator tit=threadIdMap.find(tid);
		if(tit!=threadIdMap.end())
		{
			WsocksThread *pthread=tit->second;
			_ASSERT(pthread->tid==tid);
			sprintf(pstr +1,"%c%d",send?'s':'r',PortNo);
			int plen=(int)strlen(pstr);
			if((pthread->muxtptr>=pthread->muxtrail +plen)&&(!strcmp(pthread->muxtptr -plen,pstr)))
			{
				pthread->muxtptr-=plen; memset(pthread->muxtptr,0,plen);
			}
			else
				_ASSERT(false);
		}
		if(((*prcnt)&0x0FFFFFFF)>0)
			(*prcnt)--;
		else
			_ASSERT(false);
		// Delayed mutex close
		if(*prcnt>0)
		{
			*prcnt|=0x80000000;
			if(!ReleaseMutex(*pmux))
				_ASSERT(false);
		}
		else
		{
			DeleteMutex(*pmux); *pmux=0; *prcnt=0;
		}
		#ifdef MUX_CRIT_SECTION
		LeaveCriticalSection(&lockMux);
		#else
		ReleaseMutex(lockMux);
		#endif
	}
	else
	{
		DeleteMutex(*pmux); *pmux=0; 
	}
}
bool WsocksHostImpl::WSHIsPortAvailable(unsigned short bport)
{
	SOCKET tfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(tfd==INVALID_SOCKET)
	{
		closesocket(tfd);
		return false;
	}
	int rval=0;
	if(setsockopt(tfd,SOL_SOCKET,SO_REUSEADDR,(char *)(&rval),sizeof(int))==SOCKET_ERROR) 
	{
		closesocket(tfd);
		return false;
	}
	SOCKADDR_IN baddr;
	baddr.sin_family=AF_INET;
	baddr.sin_addr.s_addr=INADDR_ANY;
	baddr.sin_port=htons(bport);
	if(bind(tfd,(SOCKADDR*)&baddr,bport)==SOCKET_ERROR) 
	{
		closesocket(tfd);
		return false;
	}
	closesocket(tfd);
	return true;
}

WsocksHostImpl::WsocksHostImpl()
{
	hostInitialized=0;
	CONNSTAT_TOTALS=false;

	memset(hostRunDir,0,MAX_PATH);
	memset(hostLogDir,0,MAX_PATH);
	WSHDate=0;
	WSHTime=0;
	WSHlTime=0;
	memset(WSHcDate,0,9);
	memset(WSHcTime,0,9);
	WSHDayOfWeek=0;
	#ifdef MUX_CRIT_SECTION
	InitializeCriticalSection(&dirMutex);
	#else
	dirMutex=0;
	#endif

	memset(&WSMemStat,0,sizeof(MEMORYSTATUS));
	WSMemPersUsedWarnLevel=0;
	WSMemAvailWarnLevel=0;
	LastTimeWeCheckedDiskSpace=0;

	thisProc=(HANDLE)-1;
	psapi=0; 
	pGetProcessMemoryInfo=0;
	memset(&WSProcessMemoryCounters,0,sizeof(PROCESS_MEMORY_COUNTERS));
	WSDiskSpaceAvailable=0;

	WSErrorCount=0;
	LastErrorCnt=0;
	hostErrFile=0;
	hostErrFileDate=0;
	hostEveFile=0;
	hostEveFileDate=0;
	WSNoOfUserPortsUsed=0;
	memset(AppStatusStr,0,256);

	LastTickDiv=0;
	LastIdle=0;
	__HOST_BUILD_DATE__=__DATE__;
	__HOST_BUILD_TIME__=__TIME__;

	#ifdef WS_FILE_SERVER
	WSFileTransferReady=false;
	WSFileXferInitCnt=0;
	fileTransfers=0;
	fileRecommits=0;
	#endif

	//dstVerified=false;

	lbmCfg=0;
	lbmon=0;

	#if defined(WS_OIO)||defined(WS_OTPP)
	#ifdef OVLMUX_CRIT_SECTION
	InitializeCriticalSection(&ovlMutex);
	#else
	ovlMutex=CreateMutex(0,false,0);
	#endif
	ovlBlocks=0;
	freeOvlList=0;
	#endif

	#ifdef LBAMUX_CRIT_SECTION
	InitializeCriticalSection(&lbaMutex);
	#else
	lbaMutex=CreateMutex(0,false,0);
	#endif
	#ifdef MUX_CRIT_SECTION
	for(int i=0;i<8;i++)
		InitializeCriticalSection(&gmutexes[i]);
	#else
	for(int i=0;i<8;i++)
		gmutexes[i]=CreateMutex(0,false,0);
	#endif
	stoptm=false;
	tmhnd=0;
	lbmon=0;
	#ifdef MUX_CRIT_SECTION
	InitializeCriticalSection(&lockMux);
	#else
	lockMux=CreateMutex(0,false,0);
	#endif

	#ifdef WS_OTMP
	conPools=0;
	usrPools=0;
	umrPools=0;
	#endif

	bAppInProcessOfExiting = false;

	#ifdef WSOCKS_SSL
	// TODO: add config for this
	SSLPW="kliserver";
	CAFILE="C:\\certs\\servercert.pem";
	CERTFILE="C:\\certs\\server.pem";
	sslg=0;
	#endif

	#ifdef WAITABLE_TIMERS
	oneSecTimer=0;
	#endif
}
WsocksHostImpl::~WsocksHostImpl()
{
	#ifdef MUX_CRIT_SECTION
	DeleteCriticalSection(&lockMux);
	#else
	if(lockMux)
	{
		CloseHandle(lockMux); lockMux=0;
	}
	#endif
	for(int i=0;i<8;i++)
	{
	#ifdef MUX_CRIT_SECTION
		DeleteCriticalSection(&gmutexes[i]);
	#else
		if(gmutexes[i]!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(gmutexes[i]); gmutexes[i]=INVALID_HANDLE_VALUE;
		}
	#endif
	}
	#ifdef LBAMUX_CRIT_SECTION
	DeleteCriticalSection(&lbaMutex);
	#else
	if(lbaMutex)
	{
		CloseHandle(lbaMutex); lbaMutex=0;
	}
	#endif
	#if defined(WS_OIO)||defined(WS_OTPP)
	#ifdef OVLMUX_CRIT_SECTION
	DeleteCriticalSection(&ovlMutex);
	#else
	if(ovlMutex)
	{
		CloseHandle(ovlMutex); ovlMutex=0;
	}
	#endif
	#endif
}
int WsocksHostImpl::WSHInitHost(const char *logPath)
{
	// This only disables _ASSERT macros in debug builds.
	//_CrtSetReportMode(_CRT_ASSERT, 0);

	#ifndef MUX_CRIT_SECTION
	dirMutex=CreateMutex(0,false,0);
	#endif
	GetCurrentDirectory(MAX_PATH,hostRunDir);
	if(logPath)
		strcpy(hostLogDir,logPath);
	else
		sprintf(hostLogDir,"%s\\logs",hostRunDir);
	CreateDirectory(hostLogDir,0);
	WSHCheckTime();
	WSHostLogEvent("--------------------------------------------------------------------------------");
	char ipath[MAX_PATH]={0};
	GetModuleFileName(0,ipath,MAX_PATH);

	char smon[8];
	int year=0,mon=0,day=0;
	sscanf(__HOST_BUILD_DATE__,"%s %d %d",smon,&day,&year);
	static const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
	char *mptr=(char*)strstr(months,smon);
	if(mptr)
		mon=(int)(mptr -months)/3+1;
	int bdate=(year*10000)+(mon*100)+(day);

	int hh=0,mm=0,ss=0;
	sscanf(__HOST_BUILD_TIME__,"%d:%d:%d",&hh,&mm,&ss);
	int btime=(hh*10000)+(mm*100)+(ss);

	WSHostLogEvent("Initializing WsocksHost (%s) built %08d-%06d...",ipath,bdate,btime);
	#ifdef WS_WINMM
	if(timeBeginPeriod(1)!=TIMERR_NOERROR)
		return -1;
	#endif
	#if defined(WS_OIO)||defined(WS_OTPP)
	if(InitOverlap()<0)
		return -1;
	#endif
	if(ccodec.Init()<0)
		return -1;
	//if(fcodec.Init()<0)
	//	return -1;

	// Default network aliases
	netmap["DAL"]="100.100.0.";
	netmap["DAL2"]="100.101.0.";
	netmap["CHI"]="100.100.1.";
	netmap["CHI2"]="100.101.1.";
	netmap["SCS"]="171.185.250.";
	netmap["SCS2"]="171.185.251.";
	netmap["DEV"]="171.130.66.";
	netmap["DEV2"]="171.130.67.";

#ifdef WIN32
	int argc=0;
	WCHAR **argv=CommandLineToArgvW(str2w(GetCommandLine()),&argc);
	for(int i=1;i<argc;i++)
	{
		const char *arg=w2str(argv[i]);
		if((!_stricmp(arg,"/startdelay"))&&(i<argc-1))
		{
			arg=w2str(argv[++i]);
			SleepEx(atol(arg)*1000,true);
		}
		else if((!_stricmp(arg,"/startwait"))&&(i<argc-2))
		{
			arg=w2str(argv[++i]);
			int pid=atoi(arg);
			arg=w2str(argv[++i]);
			int maxWait=atoi(arg);
			HANDLE phnd=OpenProcess(PROCESS_TERMINATE|SYNCHRONIZE,false,pid);
			if(phnd)
			{
				WSHostLogEvent("Waiting up to %d seconds for process %d to exit...",maxWait,pid);
				if(WaitForSingleObject(phnd,maxWait*1000)==WAIT_TIMEOUT)
				{
					// Need to kill it
					WSHostLogError("Previous process %d timed out--Terminating...",pid);
					if(TerminateProcess(phnd,9))
						WSHostLogError("Process %d terminated.",pid);
					else
						WSHostLogError("Failed terminating process %d!",pid);
				}
				else
					WSHostLogEvent("Previous process %d just exited.",pid);
				CloseHandle(phnd);
			}
			else 
				WSHostLogEvent("Previous process %d already exited.",pid);
		}
	}
#endif

	if(WSHCreateMonitorApp()<0)
		return -1;
	if(WSHLoadAppIni()<0)
		return -1;
	if(WSHCreateThreadMonitor()<0)
		return -1;
	hostInitialized=true;
	return 0; 
}
//int WsocksHostImpl::WSHInitSingleHost(WsocksApp *pmod, const char *aname)
//{
//	dirMutex=CreateMutex(0,false,0);
//	GetCurrentDirectory(MAX_PATH,hostRunDir);
//	WSHCheckTime();
//	WSHostLogEvent("--------------------------------------------------------------------------------");
//	char ipath[MAX_PATH]={0};
//	GetModuleFileName(0,ipath,MAX_PATH);
//	WIN32_FIND_DATA fdata;
//	HANDLE fhnd=FindFirstFile(ipath,&fdata);
//	if(fhnd!=INVALID_HANDLE_VALUE)
//		FindClose(fhnd);
//	SYSTEMTIME tsys;
//	FileTimeToSystemTime(&fdata.ftCreationTime,&tsys);
//	WSHostLogEvent("Initializing WsocksHost (%s) built %04d%02d%02d-%02d:%02d:%02d...",
//		ipath,tsys.wYear,tsys.wMonth,tsys.wDay,tsys.wHour,tsys.wMinute,tsys.wSecond);
//
//	static AppConfig acfg,*pcfg=&acfg;
//	acfg.ver=WS_APPVERS_1_0;
//	acfg.aclass=aname;
//	acfg.aname=aname;
//	acfg.asyncMode=0;
//	char mpath[MAX_PATH]={0};
//	GetModuleFileName(0,mpath,MAX_PATH);
//	acfg.DllPath=mpath;
//	char rdir[MAX_PATH]={0};
//	GetCurrentDirectory(MAX_PATH,rdir);
//	char cpath[MAX_PATH]={0};
//	sprintf(cpath,"%s\\setup\\ports.txt",rdir);
//	acfg.ConfigPath=cpath;
//	acfg.RunPath=rdir;
//	acfg.RunCmd=mpath;
//	char lpath[MAX_PATH]={0};
//	sprintf(lpath,"%s\\logs",rdir);
//	acfg.LogPath=lpath;
//	acfg.Menu="";
//	acfg.Enabled=true;
//	acfg.TargetThreads="T1";
//	acfg.reloadCode=0x01;
//	acfg.wshLoopback=false;
//	acfg.threadAbortTimeout=DEF_THREAD_ABORT_TIMEOUT;
//
//	pmod->dllhnd=GetModuleHandle(0);
//	pmod->pcfg=pcfg;
//	pmod->phost=this;
//	pmod->pGetAppInterface=0;
//	WSHAppLoading(pmod);
//
//	// App two-phase init
//	WaitForSingleObject(dirMutex,INFINITE);
//	SetCurrentDirectory(pmod->pcfg->RunPath.c_str());
//	if(pmod->WSHInitModule(pcfg,this)<0)
//	{
//		WSHostLogError("WSHInitModule for app(%s) failed.",pmod->pcfg->aname.c_str());
//		WSHDequeueApp(pmod);
//		SetCurrentDirectory(hostRunDir);
//		ReleaseMutex(dirMutex);
//		WSHAppLoadFailed(pmod);
//		return -1;
//	}
//	SetCurrentDirectory(hostRunDir);
//	ReleaseMutex(dirMutex);
//	pmod->initialized=1;
//	appMap[pmod->pcfg->aname]=pmod;
//	WSHostLogEvent("Loaded app(%s).",pmod->pcfg->aname.c_str());
//	WSHAppLoaded(pmod);
//
//	// Assign app to thread
//	WsocksThread *pthread=WSHCreateProcThread("T1",acfg.threadAbortTimeout);
//	if(!pthread)
//	{
//		WSHostLogError("Failed creating thread(T1)!");
//		return -1;
//	}
//	pthread->Lock();
//	pthread->appList.push_back(pmod);
//	pthread->Unlock();
//	hostInitialized=true;
//	return 0; 
//}
int WsocksHostImpl::WSHCleanupHost()
{ 
	WSHostLogEvent("Cleanup WsocksHost.");
	#ifdef WAITABLE_TIMERS
	if(oneSecTimer)
	{
		CancelWaitableTimer(oneSecTimer); oneSecTimer=0;
	}
	#endif
	if(tmhnd)
	{
		stoptm=true;
		WaitForSingleObject(tmhnd,INFINITE);
	}
	for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();)
	{
		WsocksApp *pmod=ait->second; 
		ait=appMap.erase(ait);
		WSHDequeueApp(pmod);
	}

	WSHWaitMutex(0x03,INFINITE);
	WSTHREADMAP thmap;
	thmap.swap(threadMap);
	for(WSTHREADMAP::iterator hit=thmap.begin();hit!=thmap.end();hit++)
		hit->second->abort=true;
	threadIdMap.clear();
	WSHReleaseMutex(0x03);
	for(WSTHREADMAP::iterator hit=thmap.begin();hit!=thmap.end();hit++)
	{
		WsocksThread *pthread=hit->second;
		// Thread abort will take care of stuck threads, but WSHBusyThreadReport threads should not be killed
		if(pthread->thnd)
		{
			WaitForSingleObject(pthread->thnd,INFINITE);
			CloseHandle(pthread->thnd);
			delete pthread;
		}
	}

	#ifdef WSOCKS_SSL
	if(sslg)
	{
		//sslg->ShutDown();			//DT10153 LDL 20111013 
		sslg->SSLCleanup();
		delete sslg; sslg=0;
	}
	#endif
	#if defined(WS_OIO)||defined(WS_OTPP)
	CleanupOverlap();
	#endif
	ccodec.Shutdown();
	//fcodec.Shutdown();
	hostInitialized=false;
	for(APPCONFIGLIST::iterator cit=configs.begin();cit!=configs.end();cit++)
		delete *cit;
	configs.clear();
	if(lbmCfg)
	{
		delete lbmCfg; lbmCfg=0;
	}
	if(psapi)
	{
		FreeLibrary(psapi); psapi=0;
	}
	if(thisProc)
	{
		CloseHandle(thisProc); thisProc=0;
	}
	#ifdef MUX_CRIT_SECTION
	DeleteCriticalSection(&dirMutex);
	#else
	if(dirMutex)
	{
		CloseHandle(dirMutex); dirMutex=0;
	}
	#endif
	return 0; 
}
int WsocksHostImpl::WSHGetHostBuildDate()
{
    int year;
    int day;
    char smonth[4];
    sscanf(__HOST_BUILD_DATE__, "%s %d %d", smonth, &day, &year);
    char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int month = ((int)(strstr(months, smonth) - months) / 3) + 1;
    return((year * 10000) + (month * 100) + (day));
}
int WsocksHostImpl::WSHGetHostBuildTime()
{
    int hour;
    int minute;
    int second;
    sscanf(__HOST_BUILD_TIME__, "%02d:%02d:%02d", &hour, &minute, &second);
    return((hour * 10000) + (minute * 100) + (second));
}

// 'aname' is for reload of one app's config only
int WsocksHostImpl::WSHLoadAppIni(const char *appList, APPCONFIGLIST *puiList)
{
	const char *fpath=".\\setup\\app.ini";
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		WSHostLogError("Failed opening \"%s\"!",fpath);
		return -1;
	}

	APPCONFIGLIST nmods;
	AppConfig *pcfg=0;
	char rbuf[1024]={0},errstr[256];
	int lno=0;
	while(fgets(rbuf,1024,fp))
	{
		lno++;
		// Skip leading whitespace
		const char *rptr;
		for(rptr=rbuf;(*rptr)&&((*rptr==' ')||(*rptr=='\t'));rptr++)
			;
		if((!rptr[0])||(!strncmp(rptr,"//",2))||(rptr[0]=='\r')||(rptr[0]=='\n'))
			continue;
		// Ignore trailing whitespace
		for(const char *eptr=rptr +strlen(rptr) -1;(eptr>=rptr)&&((*eptr==' ')||(*eptr=='\t'));eptr--)
			;

		// App section
		if(rptr[0]=='[')
		{
			char *eptr=(char*)strchr(rptr +1,']');
			if(eptr) *eptr=0;
			char uname[128]={0};
			strncpy(uname,rptr +1,127); uname[127]=0;
			_strupr(uname);
			if(AppConfig::FindConfig(nmods,uname))
			{
				sprintf(errstr,"Duplicate app(%s) at line %d",uname,lno);
				goto error;
			}
			pcfg=new AppConfig;
			pcfg->line=lno;
			pcfg->aname=uname;
			nmods.push_back(pcfg);
			continue;
		}
		// Parse tag:value lines
		const char *tag=strtoke((char*)rptr,":\r\n");
		if(!tag)
		{
			sprintf(errstr,"Missing tag at line %d",lno);
			goto error;
		}
		const char *value=strtoke(0,"\r\n");
		if(!value)
		{
			sprintf(errstr,"Missing value at line %d",lno);
			goto error;
		}

		if(!_stricmp(tag,"VERSION"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			int high=atol(value);
			int low=0;
			const char *dptr=strchr(value,'.');
			if(dptr)
				low=atol(dptr+1);
			pcfg->ver=(WSAppVersion)MAKEWORD(low,high);
		}
		else if(!_stricmp(tag,"ENABLED"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->Enabled=atoi(value)?true:false;
		}
		else if(!_stricmp(tag,"APPCLASS"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->aclass=value;
		}
		else if(!_stricmp(tag,"DLLPATH"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
		#ifdef _DEBUG
			char *dptr=(char*)strrchr(value,'.');
			if(dptr)
			{
				char dpath[MAX_PATH]={0};
				strncpy(dpath,value,dptr -value);
				sprintf(dpath +(dptr -value),"D%s",dptr);
				if(PathFileExists(dpath))
				{
					strcpy((char*)value,dpath);
					WSHostLogEvent("Selecting debug binary (%s) for (%s)",dpath,pcfg->aname.c_str());
				}
			}
		#endif
			pcfg->DllPath=value;
		}
		else if(!_stricmp(tag,"RUNPATH"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->RunPath=value;
		}
		else if(!_stricmp(tag,"RUNCMD"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->RunCmd=value;
		}
		else if(!_stricmp(tag,"LOGPATH"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->LogPath=value;
		}
		else if(!_stricmp(tag,"PORTS_TXT"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->ConfigPath=value;
		}
		else if(!_stricmp(tag,"MENU"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->Menu=value;
		}
		else if(!_stricmp(tag,"ASYNC_MODE"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->asyncMode=atoi(value)?true:false;
		}
		else if(!_stricmp(tag,"WSH_LOOPBACK"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
		#ifdef WS_LOOPBACK
			pcfg->wshLoopback=atoi(value)?true:false;
		#else
			if(atol(value))
			{
				sprintf(errstr,"WSH_LOOPBACK at line %d not supported.",lno);
				goto error;
			}
		#endif
		}
		else if(!_stricmp(tag,"TARGET_THREADS"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->TargetThreads=value;
		}
		//else if(!_stricmp(tag,"SYSMON_PORT"))
		//{
		//	if(!pcfg)
		//	{
		//		sprintf(errstr,"No module defined for setting at line %d",lno);
		//		goto error;
		//	}
		//	pcfg->sysmonPort=atoi(value)?true:false;
		//}
		else if(!_stricmp(tag,"CRITICAL_APP"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->criticalApp=atoi(value)?true:false;
		}
		else if(!_stricmp(tag,"THREAD_ABORT_TIMEOUT"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->threadAbortTimeout=atoi(value);
		}
		else if(!_stricmp(tag,"SYSMON_INTERFACES"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			if(!pcfg->broadAddrs.empty())
				pcfg->broadAddrs+=",";
			pcfg->broadAddrs+=value;
		}
		#ifdef WSOCKS_SSL
		else if(!_stricmp(tag,"SSLPW"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->SSLPW=value;
		}
		else if(!_stricmp(tag,"CAFILE"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->CAFILE=value;
		}
		else if(!_stricmp(tag,"CERTFILE"))
		{
			if(!pcfg)
			{
				sprintf(errstr,"No module defined for setting at line %d",lno);
				goto error;
			}
			pcfg->CERTFILE=value;
		}
		#endif
		else
		{
			sprintf(errstr,"Unknown setting (%s) at line %d",tag,lno);
			goto error;
		}
	}
	fclose(fp);

	// Restrict to apps only in the list when supplied
	if(appList)
	{
		set<string> appSet;
		char *appListCopy=_strdup(appList);
		for(char *tok=strtok(appListCopy,",");tok;tok=strtok(0,","))
			appSet.insert(tok);
		free(appListCopy);
		for(APPCONFIGLIST::iterator cit=nmods.begin();cit!=nmods.end();)
		{
			AppConfig *ncfg=*cit;
			AppConfig *pcfg=AppConfig::FindConfig(configs,ncfg->aname);
			if(appSet.find(ncfg->aname)==appSet.end())
			{
				delete ncfg;
				cit=nmods.erase(cit);
			}
			else
				cit++;
		}
	}
	// For app.ini UI
	if(puiList)
	{
		puiList->swap(nmods);
		return 0;
	}
	WSHostLogEvent("Loaded %d app definitions from %s",nmods.size(),fpath);

	// Minimize unload and reload of apps
	for(APPCONFIGLIST::iterator cit=nmods.begin();cit!=nmods.end();cit++)
	{
		AppConfig *ncfg=*cit;
		WsocksApp *pmod=WSHGetApp(ncfg->aname);
		// Unload or skip
		if(!ncfg->Enabled)
		{
			WSHostLogEvent("App(%s) not enabled.",ncfg->aname.c_str());
			if(pmod)
				WSHDequeueApp(pmod);
		}
		// Reload
		else if(pmod)
		{
			WSHReloadApp(pmod);
		}
		// New load
		else
		{
			WSHQueueApp(ncfg);
		}
		// Old configurations may be deleted
		if(pmod)
			pmod->pcfg=ncfg;
	}
	for(APPCONFIGLIST::iterator cit=configs.begin();cit!=configs.end();cit++)
	{
		AppConfig *pcfg=*cit;
		AppConfig *ncfg=AppConfig::FindConfig(nmods,pcfg->aname);
		if(ncfg)
			delete *cit;
		else
			nmods.push_back(pcfg);
	}
	configs.clear();
	configs.swap(nmods);
	SetCurrentDirectory(hostRunDir);
	return 0;

error:
	WSHostLogError("Failed loading app.ini: %s",errstr);
	for(APPCONFIGLIST::iterator cit=nmods.begin();cit!=nmods.end();cit++)
	{
		AppConfig *pcfg=*cit;
		delete pcfg;
	}
	return -1;
}
int WsocksHostImpl::WSHQueueApp(AppConfig *pcfg)
{ 
	// Already loaded
	WsocksApp *pmod=WSHGetApp(pcfg->aname);
	if(pmod)
		return 0;
	// Assign app to one of the threads for loading
	char tbuf[256]={0};
	strcpy(tbuf,pcfg->TargetThreads.c_str());
	int nthreads=0;
	for(char *tok=strtok((char*)tbuf," ,\r\n");tok;tok=strtok(0," ,\r\n"))
	{
		WsocksThread *pthread=0;
		WSHWaitMutex(0x03,INFINITE);
		WSTHREADMAP::iterator tit=threadMap.find(tok);
		if(tit==threadMap.end())
		{
			pthread=WSHCreateProcThread(tok,pcfg->threadAbortTimeout);
			if(!pthread)
			{
				WSHReleaseMutex(0x03);
				WSHostLogError("Failed creating thread(%s)!",tok);
				continue;
			}
		}
		else
			pthread=tit->second;
		WSHReleaseMutex(0x03);
		pthread->Lock();
		pthread->loadList.push_back(pcfg);
		pthread->Unlock();
		return 0;
	}
	return -1;
}
int WsocksHostImpl::WSHReloadApp(WsocksApp *pmod)
{
	// Assign app to one of the threads
	char tbuf[256]={0};
	strcpy(tbuf,pmod->pcfg->TargetThreads.c_str());
	for(char *tok=strtok((char*)tbuf," ,\r\n");tok;tok=strtok(0," ,\r\n"))
	{
		WsocksThread *pthread=0;
		WSHWaitMutex(0x03,INFINITE);
		WSTHREADMAP::iterator tit=threadMap.find(tok);
		if(tit==threadMap.end())
		{
			pthread=WSHCreateProcThread(tok,pmod->pcfg->threadAbortTimeout);
			if(!pthread)
			{
				WSHReleaseMutex(0x03);
				WSHostLogError("Failed creating thread(%s)!",tok);
				continue;
			}
		}
		else
			pthread=tit->second;
		WSHReleaseMutex(0x03);
		// Schedule reload from one thread only
		WSHostLogEvent("Reloading app(%s)...",pmod->pcfg->aname.c_str());
		WSAPPMAP::iterator ait=appMap.find(pmod->pcfg->aname);
		if(ait!=appMap.end())
			appMap.erase(ait);
		pthread->Lock();
		pthread->reloadList.push_back(pmod);
		pthread->Unlock();
		return 0;
	}
	return -1;
}
WsocksApp *WsocksHostImpl::WSHLoadApp(AppConfig *pcfg)
{ 
	// Already loaded
	WsocksApp *pmod=WSHGetApp(pcfg->aname);
	if(pmod)
		return pmod;

	HMODULE hmod=0;
	GETAPPINTERFACE pGetAppInterface=0;
	// Built-in loopback app
	if(pcfg->loopback)
	{
		hmod=pcfg->lbDllHnd;
		pGetAppInterface=(GETAPPINTERFACE)pcfg->plbGETAPPINTERFACE;
	}
	else
	{
		// Load interface functions from DLL
		hmod=LoadLibrary(pcfg->DllPath.c_str());
		if(!hmod)
		{
			if(!PathFileExists(pcfg->DllPath.c_str()))
				WSHostLogError("DLLPATH (%s) not found for (%s).",pcfg->DllPath.c_str(),pcfg->aname.c_str());
			else
				WSHostLogError("DLLPATH (%s) failed loading for (%s). Check DLL dependencies.", pcfg->DllPath.c_str(), pcfg->aname.c_str());

			return 0;
		}

		pGetAppInterface=(GETAPPINTERFACE)GetProcAddress(hmod,WSHOST_EXPORT_FUNC);
		if(!pGetAppInterface)
		{
			WSHostLogError("(%s) not exported from (%s).",WSHOST_EXPORT_FUNC,pcfg->DllPath.c_str());
			return 0;
		}
	}

	// App creation
	pmod=pGetAppInterface(hmod,pcfg->ver,pcfg->aclass.c_str(),pcfg->aname.c_str());
	if(!pmod)
	{
		WSHostLogError("GetAppInterface for app(%s) failed from (%s).",pcfg->aname.c_str(),pcfg->DllPath.c_str());
		return 0;
	}
	pmod->dllhnd=hmod;
	pmod->pcfg=pcfg;
	pmod->phost=this;
	pmod->pGetAppInterface=pGetAppInterface;
	pmod->NO_OF_UMC_PORTS=1;
	WSHAppLoading(pmod);

	// App compile validation
	char reason[256]={0};
	if(WSHValidateApp(pmod,reason)<0)
	{
		WSHostLogError("WSValidateModule for app(%s) failed: %s.",pcfg->aname.c_str(),reason);
		WSHDequeueApp(pmod);
		WSHAppLoadFailed(pmod);
		return 0;
	}
	// Auto-cleanup of minidump and thread report files
	if(!pmod->pcfg->loopback)
		WSHCleanThreadReports(pmod,7,1024);

	// App two-phase init
	#ifdef MUX_CRIT_SECTION
	EnterCriticalSection(&dirMutex);
	#else
	WaitForSingleObject(dirMutex,INFINITE);
	#endif
	SetCurrentDirectory(pcfg->RunPath.c_str());
	if(pmod->WSHInitModule(pcfg,this)<0)
	{
		WSHostLogError("WSHInitModule for app(%s) failed.",pcfg->aname.c_str());
		WSHDequeueApp(pmod);
		SetCurrentDirectory(hostRunDir);
		#ifdef MUX_CRIT_SECTION
		LeaveCriticalSection(&dirMutex);
		#else
		ReleaseMutex(dirMutex);
		#endif
		WSHAppLoadFailed(pmod);
		pmod->initialized=-1;
		return 0;
	}
	SetCurrentDirectory(hostRunDir);
	#ifdef MUX_CRIT_SECTION
	LeaveCriticalSection(&dirMutex);
	#else
	ReleaseMutex(dirMutex);
	#endif

	// Update the app build date and time after initialization
	if(pmod->__WSDATE__)
	{
		char smon[8];
		int year=0,mon=0,day=0;
		sscanf(pmod->__WSDATE__,"%s %d %d",smon,&day,&year);
		static const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
		char *mptr=(char*)strstr(months,smon);
		if(mptr)
			mon=(int)(mptr -months)/3+1;
		pmod->buildDate=(year*10000)+(mon*100)+(day);
	}
	if(pmod->__WSTIME__)
	{
		int hh=0,mm=0,ss=0;
		sscanf(pmod->__WSTIME__,"%d:%d:%d",&hh,&mm,&ss);
		pmod->buildTime=(hh*10000)+(mm*100)+(ss);
	}
	WSHostLogEvent("WSHInitModule for app(%s) reported build %s-%s.",
		pcfg->aname.c_str(),pmod->__WSDATE__,pmod->__WSTIME__);

	pmod->initialized=1;
	appMap[pcfg->aname]=pmod;
	//WSHostLogEvent("Loaded app(%s).",pcfg->aname.c_str());
	WSHAppLoaded(pmod);

	// Assign app to all threads
	char tbuf[256]={0};
	strcpy(tbuf,pcfg->TargetThreads.c_str());
	int nthreads=0;
	for(char *tok=strtok((char*)tbuf," ,\r\n");tok;tok=strtok(0," ,\r\n"))
	{
		nthreads++;
		WsocksThread *pthread=0;
		WSHWaitMutex(0x03,INFINITE);
		WSTHREADMAP::iterator tit=threadMap.find(tok);
		if(tit==threadMap.end())
		{
			pthread=WSHCreateProcThread(tok,pcfg->threadAbortTimeout);
			if(!pthread)
			{
				WSHReleaseMutex(0x03);
				WSHostLogError("Failed creating thread(%s)!",tok);
				continue;
			}
		}
		else
			pthread=tit->second;
		WSHReleaseMutex(0x03);
		pthread->Lock();
		pthread->appList.push_back(pmod);
		pthread->Unlock();
	}
	if((nthreads>1)&&(!pcfg->asyncMode))
		WSHostLogError("App(%s) is assigned to %d threads, but ASYNC_MODE is 0.",
			pcfg->aname.c_str(),nthreads);
	return pmod;
}
int WsocksHostImpl::WSHValidateApp(WsocksApp *pmod, char reason[256])
{
	// Build date/time
	pmod->buildDate=pmod->WSValidateModule("BuildDate");
	pmod->buildTime=pmod->WSValidateModule("BuildTime");
	if(pmod->buildDate<20090713)
	{
		sprintf(reason,"%s failed BuildDate challenge with %d response (expected>=%d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,20090713);
		return -1;
	}
	if(pmod->buildTime<0)
	{
		sprintf(reason,"%s failed BuildTime challenge with %d response (expected>=0)!",
			pmod->pcfg->aname.c_str(),pmod->buildTime);
		return -1;
	}
	int rc=0;
	if((rc=pmod->WSValidateModule("WSOCKSAPI_DATE"))!=WSOCKSAPI_DATE)
	{
		sprintf(reason,"%s (built %08d-%06d) failed WSOCKSAPI_DATE challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,WSOCKSAPI_DATE);
		return -1;
	}
	// Port versions
	if((pmod->CON_PORT_VERSION!=0x0100)&&
	   (pmod->CON_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CON_PORT_VERSION check with %d value (expected 1.0, 1.1, 1.2, or 1.3)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->CON_PORT_VERSION);
		return -1;
	}
	if((pmod->USC_PORT_VERSION!=0x0100)&&
	   (pmod->USC_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed USC_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->USC_PORT_VERSION);
		return -1;
	}
	if((pmod->USR_PORT_VERSION!=0x0100)&&
	   (pmod->USR_PORT_VERSION!=0x0101)&&
	   (pmod->USR_PORT_VERSION!=0x0200))
	{
		sprintf(reason,"%s (built %08d-%06d) failed USR_PORT_VERSION check with %d value (expected 1.0, 1.1, or 2.0)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->USR_PORT_VERSION);
		return -1;
	}
	if((pmod->FIL_PORT_VERSION!=0x0100)&&
	   (pmod->FIL_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed FIL_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->FIL_PORT_VERSION);
		return -1;
	}
	if((pmod->UMC_PORT_VERSION!=0x0100)&&
	   (pmod->UMC_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UMC_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->UMC_PORT_VERSION);
		return -1;
	}
	if((pmod->UMR_PORT_VERSION!=0x0100)&&
	   (pmod->UMR_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UMR_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->UMR_PORT_VERSION);
		return -1;
	}
	if((pmod->CGD_PORT_VERSION!=0x0100)&&
	   (pmod->CGD_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CGD_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->CGD_PORT_VERSION);
		return -1;
	}
	if((pmod->UGC_PORT_VERSION!=0x0100)&&
	   (pmod->UGC_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UGC_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->UGC_PORT_VERSION);
		return -1;
	}
	if((pmod->UGR_PORT_VERSION!=0x0100)&&
	   (pmod->UGR_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UGR_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->UGR_PORT_VERSION);
		return -1;
	}
	if((pmod->CTO_PORT_VERSION!=0x0100)&&
	   (pmod->CTO_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CTO_PORT_VERSION check with %d value (expected 1.0)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->CTO_PORT_VERSION);
		return -1;
	}
	if((pmod->CTI_PORT_VERSION!=0x0100)&&
	   (pmod->CTI_PORT_VERSION!=0x0101))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CTI_PORT_VERSION check with %d value (expected 1.0 or 1.1)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->CTI_PORT_VERSION);
		return -1;
	}
	if(pmod->OTHER_PORT_VERSION!=0x0100)
	{
		sprintf(reason,"%s (built %08d-%06d) failed OTHER_PORT_VERSION check with %d value (expected 1.0)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,pmod->OTHER_PORT_VERSION);
		return -1;
	}
	// App size
	if((rc=pmod->WSValidateModule("WsocksApp"))!=sizeof(WsocksApp))
	{
		sprintf(reason,"%s (built %08d-%06d) failed WsocksApp challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(WsocksApp));
		return -1;
	}
	if((rc=pmod->WSValidateModule("AppConfig"))!=sizeof(AppConfig))
	{
		sprintf(reason,"%s (built %08d-%06d) failed AppConfig challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(AppConfig));
		return -1;
	}
	// Port sizes
	if((rc=pmod->WSValidateModule("CONPORT"))!=sizeof(CONPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CONPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(CONPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("USCPORT"))!=sizeof(USCPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed USCPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(USCPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("USRPORT"))!=sizeof(USRPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed USRPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(USRPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("FILEPORT"))!=sizeof(FILEPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed FILEPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(FILEPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("UMCPORT"))!=sizeof(UMCPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UMCPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(UMCPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("UMRPORT"))!=sizeof(UMRPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UMRPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(UMRPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("CGDPORT"))!=sizeof(CGDPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CGDPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(CGDPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("UGCPORT"))!=sizeof(UGCPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UGCPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(UGCPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("UGRPORT"))!=sizeof(UGRPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed UGRPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(UGRPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("CTOPORT"))!=sizeof(CTOPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CTOPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(CTOPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("CTIPORT"))!=sizeof(CTIPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed CTIPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(CTIPORT));
		return -1;
	}
	if((rc=pmod->WSValidateModule("OTHERPORT"))!=sizeof(OTHERPORT))
	{
		sprintf(reason,"%s (built %08d-%06d) failed OTHERPORT challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(OTHERPORT));
		return -1;
	}
	// Compile defs
	if((rc=pmod->WSValidateModule("WS_MAX_BLOCK_SIZE"))!=WS_MAX_BLOCK_SIZE)
	{
		sprintf(reason,"%s (built %08d-%06d) failed WS_MAX_BLOCK_SIZE challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,WS_MAX_BLOCK_SIZE);
		return -1;
	}
	if((rc=pmod->WSValidateModule("WS_MAX_ALT_PORTS"))!=WS_MAX_ALT_PORTS)
	{
		sprintf(reason,"%s (built %08d-%06d) failed WS_MAX_ALT_PORTS challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,WS_MAX_ALT_PORTS);
		return -1;
	}
	#ifdef WS_FILE_SERVER
	if(!pmod->WSValidateModule("WS_FILE_SERVER"))
	{
		sprintf(reason,"%s (built %08d-%06d) failed WS_FILE_SERVER challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,1);
		return -1;
	}
	#endif
	#ifdef WS_GUARANTEED
	if(!pmod->WSValidateModule("WS_GUARANTEED"))
	{
		sprintf(reason,"%s (built %08d-%06d) failed WS_GUARANTEED challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,1);
		return -1;
	}
	#endif
	#ifdef WS_OIO
	if(!pmod->WSValidateModule("WS_OIO"))
	{
		sprintf(reason,"%s (built %08d-%06d) failed WS_OIO challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,1);
		return -1;
	}
	#endif
	// Struct sizes
	if((rc=pmod->WSValidateModule("WSRECORDING"))!=sizeof(WSRECORDING))
	{
		sprintf(reason,"%s (built %08d-%06d) failed WSRECORDING challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(WSRECORDING));
		return -1;
	}
	if((rc=pmod->WSValidateModule("IPACL"))!=sizeof(IPACL))
	{
		sprintf(reason,"%s (built %08d-%06d) failed IPACL challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(IPACL));
		return -1;
	}
	#ifdef WS_OIO
	if((rc=pmod->WSValidateModule("WSOVERLAPPED"))!=sizeof(WSOVERLAPPED))
	{
		sprintf(reason,"%s (built %08d-%06d) failed WSOVERLAPPED challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(WSOVERLAPPED));
		return -1;
	}
	#endif
	if((rc=pmod->WSValidateModule("IPACLMAP"))!=sizeof(IPACLMAP))
	{
		sprintf(reason,"%s (built %08d-%06d) failed IPACLMAP challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(IPACLMAP));
		return -1;
	}
	if((rc=pmod->WSValidateModule("GDID"))!=sizeof(GDID))
	{
		sprintf(reason,"%s (built %08d-%06d) failed GDID challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(GDID));
		return -1;
	}
	if((rc=pmod->WSValidateModule("GDLOGIN"))!=sizeof(GDLOGIN))
	{
		sprintf(reason,"%s (built %08d-%06d) failed GDLOGIN challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(GDLOGIN));
		return -1;
	}
	if((rc=pmod->WSValidateModule("GDACL"))!=sizeof(GDACL))
	{
		sprintf(reason,"%s (built %08d-%06d) failed GDACL challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(GDACL));
		return -1;
	}
	if((rc=pmod->WSValidateModule("FIDX"))!=sizeof(FIDX))
	{
		sprintf(reason,"%s (built %08d-%06d) failed FIDX challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(FIDX));
		return -1;
	}
	if((rc=pmod->WSValidateModule("FILEINDEX"))!=sizeof(FILEINDEX))
	{
		sprintf(reason,"%s (built %08d-%06d) failed FILEINDEX challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(FILEINDEX));
		return -1;
	}
	if((rc=pmod->WSValidateModule("GDLINE"))!=sizeof(GDLINE))
	{
		sprintf(reason,"%s (built %08d-%06d) failed GDLINE challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,sizeof(GDLINE));
		return -1;
	}
#ifdef VS2003
	int studiover=2003;
#elif defined(VS2005)
	int studiover=2005;
#elif defined(VS2010)
	int studiover=2010;
#elif defined(_CONSOLE)
	int studiover=2003;
#elif !defined(WIN32)
	int studiover=323;//G++ 3.2.3
#else
	#error A VisualStudio compiler version must be defined.
#endif
	if((rc=pmod->WSValidateModule("VisualStudio"))!=studiover)
	{
		sprintf(reason,"%s (built %08d-%06d) failed VisualStudio challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,studiover);
		return -1;
	}
#ifdef BIT64
	int cpubits=64;
#else
	int cpubits=32;
#endif
	if((rc=pmod->WSValidateModule("CpuBits"))!=cpubits)
	{
		sprintf(reason,"%s (built %08d-%06d) failed CpuBits challenge with %d response (expected %d)!",
			pmod->pcfg->aname.c_str(),pmod->buildDate,pmod->buildTime,rc,cpubits);
		return -1;
	}
	return 0;
}
WsocksApp *WsocksHostImpl::WSHGetApp(const string& aname)
{
	WSAPPMAP::iterator ait=appMap.find(aname);
	if(ait==appMap.end())
		return 0;
	return ait->second;
}
int WsocksHostImpl::WSHDequeueApp(WsocksApp *pmod)
{
	if(pmod->threadSignal&0x80000000)
		return 0;
	pmod->threadSignal|=0x80000000;
	// Remove app from all threads
	char tbuf[256]={0};
	strcpy(tbuf,pmod->pcfg->TargetThreads.c_str());
	WsocksThread *uthread=0;
	for(char *tok=strtok((char*)tbuf," ,\r\n");tok;tok=strtok(0," ,\r\n"))
	{
		WsocksThread *pthread=0;
		WSHWaitMutex(0x03,INFINITE);
		WSTHREADMAP::iterator tit=threadMap.find(tok);
		if(tit==threadMap.end())
		{
			WSHReleaseMutex(0x03);
			continue;
		}
		pthread=tit->second;
		WSHReleaseMutex(0x03);
		pthread->Lock();
	#ifndef WS_OIO
		for(WSAPPLIST::iterator ait=pthread->appList.begin();ait!=pthread->appList.end();)
		{
			WsocksApp *amod=*ait;
			if(amod==pmod)
				ait=pthread->appList.erase(ait);
			else
				ait++;
		}
	#endif
		if(!uthread)
			uthread=pthread;
		pthread->Unlock();
	}
	// Schedule unload from one thread only
	if(uthread)
	{
		WSAPPMAP::iterator ait=appMap.find(pmod->pcfg->aname);
		if(ait!=appMap.end())
			appMap.erase(ait);
		uthread->Lock();
		uthread->unloadList.push_back(pmod);
		uthread->Unlock();
	}
	return 0;
}
int WsocksHostImpl::WSHUnloadApp(WsocksApp *pmod)
{
	bAppInProcessOfExiting = true;

	pmod->threadSignal|=0x80000000;
	string aname=pmod->pcfg->aname;

	// Remove app from all threads
	char tbuf[256]={0};
	strcpy(tbuf,pmod->pcfg->TargetThreads.c_str());
	for(char *tok=strtok((char*)tbuf," ,\r\n");tok;tok=strtok(0," ,\r\n"))
	{
		WsocksThread *pthread=0;
		WSHWaitMutex(0x03,INFINITE);
		WSTHREADMAP::iterator tit=threadMap.find(tok);		
		if(tit==threadMap.end())
		{
			WSHReleaseMutex(0x03);
			continue;
		}
		pthread=tit->second;
		WSHReleaseMutex(0x03);
		pthread->Lock();
		for(WSAPPLIST::iterator ait=pthread->appList.begin();ait!=pthread->appList.end();)
		{
			WsocksApp *amod=*ait;
			if(amod==pmod)
				ait=pthread->appList.erase(ait);
			else
				ait++;
		}
		if(pthread->appList.empty())
			pthread->abort=true;
		pthread->Unlock();
	}

	if(pmod->initialized>0)
	{
		WSHostLogEvent("Unloading app(%s)...",aname.c_str());
		#ifdef MUX_CRIT_SECTION
		EnterCriticalSection(&dirMutex);
		#else
		WaitForSingleObject(dirMutex,INFINITE);
		#endif
		SetCurrentDirectory(pmod->pcfg->RunPath.c_str());
		pmod->WSHCleanupModule();
		SetCurrentDirectory(hostRunDir);
		#ifdef MUX_CRIT_SECTION
		LeaveCriticalSection(&dirMutex);
		#else
		ReleaseMutex(dirMutex);
		#endif
		WSHostLogEvent("Unloaded app(%s).",aname.c_str());
		pmod->initialized=0;
	}
	WSHAppUnloaded(pmod);
	bool exithost=pmod->pcfg->criticalApp;

	FREEAPPINTERFACE pFreeFunc=pmod->pFreeAppInterface;
	if(pFreeFunc)
		pFreeFunc(pmod);

	// If the app is marked as critical, then exit the host too when it exits
	if(exithost)
		WSHExitHost(0);

	bAppInProcessOfExiting = false;

	return 0; 
}
int WsocksHostImpl::WSHCreateMonitorApp()
{
	AppConfig *pcfg=new AppConfig;
	pcfg->aname="LBSysmon";
	pcfg->ver=WS_APPVERS_1_0;
	pcfg->Enabled=true;
	pcfg->aclass="LBSysmon";
	char mpath[MAX_PATH]={0};
	GetModuleFileName(0,mpath,MAX_PATH);
	pcfg->DllPath=mpath;
	GetCurrentDirectory(MAX_PATH,mpath);
	pcfg->RunPath=mpath;
	sprintf(mpath,"%s\\logs",pcfg->RunPath.c_str());
	pcfg->LogPath=mpath;
	sprintf(mpath,"%s\\setup\\app.ini",pcfg->RunPath.c_str());
	pcfg->ConfigPath=mpath;
	pcfg->RunCmd="LBSysmon";
	pcfg->asyncMode=0;
	pcfg->TargetThreads="LBMon";
	pcfg->wshLoopback=true;
	pcfg->loopback=true;
    pcfg->shareHostLog=true;
	pcfg->lbDllHnd=GetModuleHandle(0);
	pcfg->plbGETAPPINTERFACE=LBMonitor_GetAppInterface;
	pcfg->threadAbortTimeout=60;//DEF_THREAD_ABORT_TIMEOUT;

	WCHAR *cmdl=GetCommandLineW();
	int argc=0;
	LPWSTR *argv=CommandLineToArgvW(cmdl,&argc);
	for(int i=1;i<argc;i++)
	{
		const char *arg=w2str(argv[i]);
		if(!strncmp(arg,"/smp:",5))
			pcfg->sysmonAddr=arg+5;
	}

	lbmCfg=pcfg;
	lbmon=(LBMonitor*)WSHLoadApp(pcfg);
	return lbmon?0:-1;
}
//int WsocksHostImpl::WSPortsCfg()
//{
//	_ASSERT(false);
//	return 0;
//}
//int WsocksHostImpl::WSSetupSocks()
//{
//	return WSHSetupSocks();
//}
void WsocksHostImpl::WSHResetErrorCnt(WsocksApp *pmod)
{
}
int WsocksHostImpl::WSHPreInitSocks(WsocksApp *pmod)
{
	if(pmod->WSInitCnt>0)
		return 0;
	//SMSInit();

	if(pmod->NO_OF_CON_PORTS+1>0)
	{
		pmod->ConPort=new CONPORT[pmod->NO_OF_CON_PORTS+1];
		memset(pmod->ConPort,0,(pmod->NO_OF_CON_PORTS+1)*sizeof(CONPORT));
	}
	if(pmod->NO_OF_USC_PORTS+1>0)
	{
		pmod->UscPort=new USCPORT[pmod->NO_OF_USC_PORTS+1];
		memset(pmod->UscPort,0,(pmod->NO_OF_USC_PORTS+1)*sizeof(USCPORT));
	}
	if(pmod->NO_OF_USR_PORTS+1>0)
	{
		pmod->UsrPort=new USRPORT[pmod->NO_OF_USR_PORTS+1];
		memset(pmod->UsrPort,0,(pmod->NO_OF_USR_PORTS+1)*sizeof(USRPORT));
	}
	if(pmod->NO_OF_FILE_PORTS+1>0)
	{
		pmod->FilePort=new FILEPORT[pmod->NO_OF_FILE_PORTS+1];
		memset(pmod->FilePort,0,(pmod->NO_OF_FILE_PORTS+1)*sizeof(FILEPORT));
	}
	if(pmod->NO_OF_UMC_PORTS+1>0)
	{
		pmod->UmcPort=new UMCPORT[pmod->NO_OF_UMC_PORTS+1];
		memset(pmod->UmcPort,0,(pmod->NO_OF_UMC_PORTS+1)*sizeof(UMCPORT));
	}
	if(pmod->NO_OF_UMR_PORTS+1>0)
	{
		pmod->UmrPort=new UMRPORT[pmod->NO_OF_UMR_PORTS+1];
		memset(pmod->UmrPort,0,(pmod->NO_OF_UMR_PORTS+1)*sizeof(UMRPORT));
	}
	if(pmod->NO_OF_CGD_PORTS+1>0)
	{
		pmod->CgdPort=new CGDPORT[pmod->NO_OF_CGD_PORTS+1];
		memset(pmod->CgdPort,0,(pmod->NO_OF_CGD_PORTS+1)*sizeof(CGDPORT));
	}
	if(pmod->NO_OF_UGC_PORTS+1>0)
	{
		pmod->UgcPort=new UGCPORT[pmod->NO_OF_UGC_PORTS+1];
		memset(pmod->UgcPort,0,(pmod->NO_OF_UGC_PORTS+1)*sizeof(UGCPORT));
	}
	if(pmod->NO_OF_UGR_PORTS+1>0)
	{
		pmod->UgrPort=new UGRPORT[pmod->NO_OF_UGR_PORTS+1];
		memset(pmod->UgrPort,0,(pmod->NO_OF_UGR_PORTS+1)*sizeof(UGRPORT));
	}
	if(pmod->NO_OF_CTO_PORTS+1>0)
	{
		pmod->CtoPort=new CTOPORT[pmod->NO_OF_CTO_PORTS+1];
		memset(pmod->CtoPort,0,(pmod->NO_OF_CTO_PORTS+1)*sizeof(CTOPORT));
	}
	if(pmod->NO_OF_CTI_PORTS+1>0)
	{
		pmod->CtiPort=new CTIPORT[pmod->NO_OF_CTI_PORTS+1];
		memset(pmod->CtiPort,0,(pmod->NO_OF_CTI_PORTS+1)*sizeof(CTIPORT));
	}
	if(pmod->NO_OF_OTHER_PORTS+1>0)
	{
		pmod->OtherPort=new OTHERPORT[pmod->NO_OF_OTHER_PORTS+1];
		memset(pmod->OtherPort,0,(pmod->NO_OF_OTHER_PORTS+1)*sizeof(OTHERPORT));
	}

	pmod->WSDate=WSHDate;
	pmod->WSTime=WSHTime;
	memcpy(pmod->WScDate,WSHcDate,9);
	memcpy(pmod->WScTime,WSHcTime,9);

	CreateDirectory(pmod->pcfg->LogPath.c_str(),0);

#ifdef UNIX
	//sigevent SigEvent;
	//itimerspec iTimerSpec;
	//timer_t StatusTimer;
	//timer_t WSHTimer;
#else
    WSADATA WSAData;
	int status;
#endif
	//int i;

	//WSInitCnt++;
	//if(WSInitCnt>1)
	//	return 0;

	pmod->WSSuspendMode=0;
#if defined(WS_MIPS)&&defined(WIN32)
	WSHCalcMips(pmod);
	WSHLogEvent(pmod,"Initializing Ports:PerFreq(%I64d),WSMips(%I64d),WSMaxIdleMips(%I64d)"
		,pmod->WSPerformanceFreq.QuadPart,pmod->WSMips,pmod->WSMaxIdleMips);
//#elif defined(WS_WINMM)
//	timeBeginPeriod(1);
//#endif
#endif
	WSHLogEvent(pmod,"Initializing Ports"	);
	//WSCheckDST();

#ifndef UNIX
#ifdef WS_LOGERRORS
	_mkdir ("logs");
#endif
	if ((status = WSAStartup(257, &WSAData)) != 0) // 275 = MAKEWORD(1,1)
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Starting WSocks");
		//PostMessage(pmod->WShWnd, WM_QUIT, 0, 0);
		WSHExitApp(pmod,99);
	}
#endif
	//for(i=0;i<WS_MAX_DIALOGS;i++)
	//	WSDlgPosOpen[i]=TRUE;

	WSHSetupConPorts(pmod,WS_INIT);
#ifdef WS_GUARANTEED
	WSHSetupCgdPorts(pmod,WS_INIT);
#endif
	WSHSetupUsrPorts(pmod,WS_INIT);
#ifdef WS_GUARANTEED
	WSHSetupUgrPorts(pmod,WS_INIT);
#endif
#ifdef WS_MONITOR
	WSHSetupUmrPorts(pmod,WS_INIT);
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
	WSHSetupCtoPorts(pmod,WS_INIT);
	WSHSetupCtiPorts(pmod,WS_INIT);
#endif //#ifdef WS_CAST
#ifdef WS_OTHER
	WSHSetupOtherPorts(pmod,WS_INIT);
#endif
#ifdef WS_FILE_SERVER
	WSHSetupFilePorts(pmod,WS_INIT);
#endif
	int rc=pmod->WSPortsCfg();
	if(rc<0)
		return -1;
	else if((!rc)&&(WSHSetupSocks(pmod)<0))
		return -1;
	if(lbmon) lbmon->LBMAppSetupSocks(pmod);

#ifdef WS_OIO
	pmod->hIOPort=::CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
	if(!pmod->hIOPort) 
	{
		WSHLogError(pmod,"Unable to create hIOPort completion port.");
		return -1;
	}
#endif

#ifdef WS_FILE_SERVER
	WSHSetupFilePorts(pmod,WS_OPEN);
    fileTransfers = 0;
	fileRecommits = 0;
	WSHLoadUncommittedFiles(pmod,0);
	memset(&pmod->WSFileOptions, 0, sizeof(FileOptions));
	pmod->WSFileOptions.xfersPerTick = FILE_XFERS_PER_TICK;
	pmod->WSFileOptions.commitTimeout = COMMIT_TIMEOUT;
	pmod->WSFileOptions.waitTimeout = INFINITE;
	pmod->WSFileOptions.autoCloseProgressDialog = true;
	//pmod->WSFileOptions.useLocalFirst = false;
	//pmod->WSFileOptions.useFileServer = true;
	//pmod->WSFileOptions.useWindowsNetwork = false;
	pmod->WSFileOptions.localMaster = false;
	pmod->WSFileOptions.keepLocal = false;
	pmod->WSFileOptions.cacheLocal = false;
	pmod->WSFileOptions.guaranteeCommit = false;
	WSFileTransferReady = true;
#endif

//#ifdef UNIX
//	memset((char*)&SigEvent,0,sizeof(sigevent));
//	SigEvent.sigev_notify=SIGEV_SIGNAL;
//	SigEvent.sigev_signo=SIGUSR1;
//	SigEvent.sigev_value.sival_int=2;
//	timer_create(CLOCK_REALTIME,&SigEvent,&StatusTimer);
//	sigset(SIGUSR1,WSDisplayStatus);
//	iTimerSpec.it_interval.tv_sec=1;
//	iTimerSpec.it_interval.tv_nsec=0;
//	iTimerSpec.it_value.tv_sec=1;
//	iTimerSpec.it_value.tv_nsec=0;
//	timer_settime(StatusTimer,0,&iTimerSpec,NULL);
//
//	memset((char*)&SigEvent,0,sizeof(sigevent));
//	SigEvent.sigev_notify=SIGEV_SIGNAL;
//	SigEvent.sigev_signo=SIGUSR2;
//	SigEvent.sigev_value.sival_int=2;
//	timer_create(CLOCK_REALTIME,&SigEvent,&WSHTimer);
//	sigset(SIGUSR2,WSHTimerProc);
//	iTimerSpec.it_interval.tv_sec=0;
//	iTimerSpec.it_interval.tv_nsec=WS_TIMER_INTERVAL*1000;
//	iTimerSpec.it_value.tv_sec=0;
//	iTimerSpec.it_value.tv_nsec=WS_TIMER_INTERVAL*1000;
//	timer_settime(WSHTimer,0,&iTimerSpec,NULL);
//#else
//	#ifdef WS_WINDOWS_CE
//		WSpMainWnd->SetTimer(1,STATUS_UPDATE,(TIMERPROC)WSDisplayStatus);
//		WSpMainWnd->SetTimer(0,WS_TIMER_INTERVAL,(TIMERPROC)WSHTimerProc);
//	#else
//		SetTimer(pmod->WShWnd,1,STATUS_UPDATE,(TIMERPROC)WSDisplayStatus);
//		SetTimer(pmod->WShWnd,0,WS_TIMER_INTERVAL,(TIMERPROC)WSHTimerProc);
//	#endif
//#endif
//
//	WSMemPersUsedWarnLevel = 0;
//	WSMemAvailWarnLevel=20000;
//	WSErrorCount=0;
//
//	LastTickCnt=0;
	pmod->WSInitCnt++;
	return(TRUE);
}
int WsocksHostImpl::WSHInitSocks(WsocksApp *pmod)
{
	if(WSHPreInitSocks(pmod)<0)
		return -1;
	WSHLogEvent(pmod,"Opening Ports");

	WSHSetupConPorts(pmod,WS_OPEN);
#ifdef WS_GUARANTEED
    WSHSetupCgdPorts(pmod,WS_OPEN);
#endif
	WSHSetupUsrPorts(pmod,WS_OPEN);
#ifdef WS_GUARANTEED
    WSHSetupUgrPorts(pmod,WS_OPEN);
#endif
#ifdef WS_MONITOR
	WSHSetupUmrPorts(pmod,WS_OPEN);
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
	WSHSetupCtoPorts(pmod,WS_OPEN);
	WSHSetupCtiPorts(pmod,WS_OPEN);
#endif //#ifdef WS_CAST
#ifdef WS_OTHER
	WSHSetupOtherPorts(pmod,WS_OPEN);
#endif
	return(TRUE);
}
int WsocksHostImpl::WSHSetupSocks(WsocksApp *pmod)
{
	char instr[256];
	char Tempstr[256];
	char *inptr;
	char port_type;
	char PortStr[10];
	int PortNo=-1;
	char port_ip[50];
	char port_port[10];
	char port_out[50];
	FILE *infile;
	int Roaming;
	int i,line=0;

	// Override with ports.txt file
	if((infile=fopen(pmod->pcfg->ConfigPath.c_str(),"rt"))==NULL)
	{
		WSHLogError(pmod,"Failed to open \"%s\"!",pmod->pcfg->ConfigPath.c_str());
		return -1;
	}

	while(fgets(instr,255,infile)!=NULL)
	{
		line++;
		Roaming = FALSE;
		if((inptr=strtok(instr,":\r\n"))==NULL)
		{
			continue;
		}

		if (((instr[0] == '/')&&(instr[1] == '/'))
				|| (instr[0] == '#')
				|| (instr[0] == '!'))
			continue;

		port_type=inptr[0];
		strcpy(PortStr,&inptr[1]);
		PortNo=atoi(PortStr);

		if((inptr=strtok(NULL,":\r\n"))==NULL)
		{
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.txt at line %d",line);
			break;
		}
		strcpy(port_ip,inptr);

		if((inptr=strtok(NULL,":\r\n"))==NULL)
		{
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.tx at line %d",line);
			break;
		}
		strcpy(port_port,inptr);

		switch(port_type)
		{
		case 'c':
		case 'C':
			if((inptr=strtok(NULL,":\r\n"))==NULL)
			{
				WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.txt at line %d",line);
				break;
			}
			strcpy(port_out,inptr);

			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CON_PORTS))
			{
				pmod->WSCreatePort(WS_CON,PortNo);
				strcpy(pmod->ConPort[PortNo].LocalIP,port_ip);
				pmod->ConPort[PortNo].Port=atoi(port_port);
				int oport=pmod->ConPort[PortNo].Port;
				char *pptr=strchr(port_out,'/');
				if(pptr)
				{
					*pptr=0; oport=atoi(pptr +1);
				}
				strcpy(pmod->ConPort[PortNo].RemoteIP,port_out);
				pmod->ConPort[PortNo].AltRemotePort[pmod->ConPort[PortNo].AltIPCount]=oport;
				strcpy(pmod->ConPort[PortNo].AltRemoteIP[pmod->ConPort[PortNo].AltIPCount++],port_out);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->ConPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->ConPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='L')
						pmod->ConPort[PortNo].ChokeSize=atoi(&inptr[1]);
#ifdef WS_COMPRESS
					if(inptr[0]=='C')
					{
						pmod->ConPort[PortNo].Compressed=TRUE;
						if(isdigit(inptr[1]))
							pmod->ConPort[PortNo].CompressType=inptr[1]-'0';
					}
#ifdef WS_ENCRYPTED
					if(inptr[0]=='E')
					{
						pmod->ConPort[PortNo].Encrypted=TRUE;
						pmod->ConPort[PortNo].Compressed=TRUE;
						if(isdigit(inptr[1]))
						{
							pmod->ConPort[PortNo].Encrypted=inptr[1]-'0';
							#ifdef WSOCKS_SSL
							if(pmod->ConPort[PortNo].Encrypted==2)
								pmod->ConPort[PortNo].Compressed=FALSE;
							#endif
						}
					}
#endif
#endif
					if(inptr[0]=='5') 
					{
						strncpy(pmod->ConPort[PortNo].S5RemoteIP,&inptr[1], 19);
						pmod->ConPort[PortNo].S5RemoteIP[19] = 0;
						pmod->ConPort[PortNo].S5Port=1080;
						pmod->ConPort[PortNo].S5Connect=TRUE;
						pmod->ConPort[PortNo].S5Methode=00;
						pmod->ConPort[PortNo].S5Version=5;
					}
					if(inptr[0]=='6') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->ConPort[PortNo].S5Port=atoi(Tempstr);
					}
					if(inptr[0]=='7') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->ConPort[PortNo].S5Methode=atoi(Tempstr);
					}
					if(inptr[0]=='8') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->ConPort[PortNo].S5Version=atoi(Tempstr);
					}
					if(inptr[0]=='N') 
					{
						strncpy(pmod->ConPort[PortNo].CfgNote,&inptr[1], 255);
						pmod->ConPort[PortNo].CfgNote[255] = 0;
					}
					if(inptr[0]=='R') 
						Roaming=TRUE;
					if(inptr[0]=='B')
					{
						pmod->ConPort[PortNo].Bounce=atol(&inptr[1]);
						if((pmod->ConPort[PortNo].Bounce!=0)&&(pmod->ConPort[PortNo].Bounce<500))
							pmod->ConPort[PortNo].Bounce=500;
					}
					if(inptr[0]=='<')
						pmod->ConPort[PortNo].BounceStart=atol(&inptr[1]);
					if(inptr[0]=='>')
						pmod->ConPort[PortNo].BounceEnd=atol(&inptr[1]);
					if(inptr[0]=='A')
					{
						if(pmod->ConPort[PortNo].AltIPCount<WS_MAX_ALT_PORTS)
						{
							int oport=pmod->ConPort[PortNo].Port;
							char *pptr=strchr(&inptr[1],'/');
							if(pptr)
							{
								*pptr=0; oport=atoi(pptr +1);
							}
							pmod->ConPort[PortNo].AltRemotePort[pmod->ConPort[PortNo].AltIPCount]=oport;
							strcpy(pmod->ConPort[PortNo].AltRemoteIP[pmod->ConPort[PortNo].AltIPCount++]
								,&inptr[1]);
						}
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->ConPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
				#ifdef WS_DECLINING_RECONNECT
					if(inptr[0]=='Y')
					{
						const char *line=strtoke(&inptr[1],",");
						if(line)
							pmod->ConPort[PortNo].MinReconnectDelay=atol(line);
						line=strtoke(0,",");
						if(line)
							pmod->ConPort[PortNo].MaxReconnectDelay=atol(line);
						line=strtoke(0,",");
						if(line)
							pmod->ConPort[PortNo].MinReconnectReset=atol(line);
					}
				#endif
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d,%d",&pmod->ConPort[PortNo].RecvBuffLimit,&pmod->ConPort[PortNo].SendBuffLimit);
					}
				}
				if(Roaming)
				{
					for (i=0;i<pmod->ConPort[PortNo].AltIPCount;i++)
					{
						pmod->ConPort[PortNo].AltRemoteIPOn[i]=TRUE;
					}
				}
				else
					pmod->ConPort[PortNo].AltRemoteIPOn[0]=TRUE;
				pmod->ConPort[PortNo].CurrentAltIP = -1;
				#ifdef WS_REALTIMESEND
				pmod->ConPort[PortNo].ImmediateSendLimit=pmod->ConPort[PortNo].BlockSize*1024;
				#endif
			}
			break;
#ifdef WS_GUARANTEED
		case 'd':
		case 'D':
			if((inptr=strtok(NULL,":\r\n"))==NULL)
			{
				WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.txt at line %d",line);
				break;
			}
			strcpy(port_out,inptr);

			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CGD_PORTS))
			{
				pmod->WSCreatePort(WS_CGD,PortNo);
				strcpy(pmod->CgdPort[PortNo].LocalIP,port_ip);
				pmod->CgdPort[PortNo].Port=atoi(port_port);
				strcpy(pmod->CgdPort[PortNo].RemoteIP,port_out);
				strcpy(pmod->CgdPort[PortNo].AltRemoteIP[pmod->CgdPort[PortNo].AltIPCount++]
					,port_out);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->CgdPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->CgdPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='L')
						pmod->CgdPort[PortNo].ChokeSize=atoi(&inptr[1]);
#ifdef WS_COMPRESS
					if(inptr[0]=='C')
						pmod->CgdPort[PortNo].Compressed=TRUE;
#ifdef WS_ENCRYPTED
					if(inptr[0]=='E')
					{
						pmod->CgdPort[PortNo].Encrypted=TRUE;
						pmod->CgdPort[PortNo].Compressed=TRUE;
					}
#endif
#endif
					if(inptr[0]=='5') 
					{
						strncpy(pmod->CgdPort[PortNo].S5RemoteIP,&inptr[1], 19);
						pmod->CgdPort[PortNo].S5RemoteIP[19] = 0;
						pmod->CgdPort[PortNo].S5Port=1080;
						pmod->CgdPort[PortNo].S5Connect=TRUE;
						pmod->CgdPort[PortNo].S5Methode=00;
						pmod->CgdPort[PortNo].S5Version=5;
					}
					if(inptr[0]=='6') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->CgdPort[PortNo].S5Port=atoi(Tempstr);
					}
					if(inptr[0]=='7') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->CgdPort[PortNo].S5Methode=atoi(Tempstr);
					}
					if(inptr[0]=='8') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->CgdPort[PortNo].S5Version=atoi(Tempstr);
					}
					if(inptr[0]=='N') 
					{
						strncpy(pmod->CgdPort[PortNo].CfgNote,&inptr[1], 79);
						pmod->CgdPort[PortNo].CfgNote[79] = 0;
					}
					if(inptr[0]=='R') 
						Roaming=TRUE;
					if(inptr[0]=='B')
					{
						pmod->CgdPort[PortNo].Bounce=atol(&inptr[1]);
						if((pmod->CgdPort[PortNo].Bounce!=0)&&(pmod->CgdPort[PortNo].Bounce<500))
							pmod->CgdPort[PortNo].Bounce=500;
					}
					if(inptr[0]=='<')
						pmod->CgdPort[PortNo].BounceStart=atol(&inptr[1]);
					if(inptr[0]=='>')
						pmod->CgdPort[PortNo].BounceEnd=atol(&inptr[1]);
					if(inptr[0]=='A')
					{
						if(pmod->CgdPort[PortNo].AltIPCount<WS_MAX_ALT_PORTS)
						{
							strcpy(pmod->CgdPort[PortNo].AltRemoteIP[pmod->CgdPort[PortNo].AltIPCount++]
								,&inptr[1]);
						}
					}
					if(inptr[0]=='G') 
					{
						strncpy(pmod->CgdPort[PortNo].GDCfg,&inptr[1], 127);
						pmod->CgdPort[PortNo].GDCfg[127] = 0;
					}
					if(inptr[0]=='J') 
						pmod->CgdPort[PortNo].GDGap=atoi(&inptr[1]);
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->CgdPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
				#ifdef WS_DECLINING_RECONNECT
					if(inptr[0]=='Y')
					{
						const char *line=strtoke(&inptr[1],",");
						if(line)
							pmod->CgdPort[PortNo].MinReconnectDelay=atol(line);
						line=strtoke(0,",");
						if(line)
							pmod->CgdPort[PortNo].MaxReconnectDelay=atol(line);
						line=strtoke(0,",");
						if(line)
							pmod->CgdPort[PortNo].MinReconnectReset=atol(line);
					}
				#endif
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d,%d",&pmod->CgdPort[PortNo].RecvBuffLimit,&pmod->CgdPort[PortNo].SendBuffLimit);
					}
				}
				if(Roaming)
				{
					for (i=0;i<pmod->CgdPort[PortNo].AltIPCount;i++)
					{
						pmod->CgdPort[PortNo].AltRemoteIPOn[i]=TRUE;
					}
				}
				else
					pmod->CgdPort[PortNo].AltRemoteIPOn[0]=TRUE;
				pmod->CgdPort[PortNo].CurrentAltIP = -1;
				#ifdef WS_REALTIMESEND
				pmod->CgdPort[PortNo].ImmediateSendLimit=pmod->CgdPort[PortNo].BlockSize*1024;
				#endif
			}
			break;
#endif //#ifdef WS_GUARANTEED
		case 'u':
		case 'U':
			if(PortNo==0)
			{
				WSHLogError(pmod,"USC0 has been deprecated by Sysmon and is no longer supported.");
			}
			else if((PortNo>=1)&&(PortNo<pmod->NO_OF_USC_PORTS))
			{
				pmod->WSCreatePort(WS_USC,PortNo);
				strcpy(pmod->UscPort[PortNo].LocalIP,port_ip);
				pmod->UscPort[PortNo].Port=atoi(port_port);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->UscPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->UscPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='N') 
					{
						strncpy(pmod->UscPort[PortNo].CfgNote,&inptr[1], 255);
						pmod->UscPort[PortNo].CfgNote[255] = 0;
					}
#ifdef WS_COMPRESS
					if(inptr[0]=='C')
					{
						pmod->UscPort[PortNo].Compressed=TRUE;
						if(isdigit(inptr[1]))
							pmod->UscPort[PortNo].CompressType=inptr[1]-'0';
					}
#ifdef WS_ENCRYPTED
					if(inptr[0]=='E')
					{
						pmod->UscPort[PortNo].Encrypted=TRUE;
						pmod->UscPort[PortNo].Compressed=TRUE;
						if(isdigit(inptr[1]))
						{
							pmod->UscPort[PortNo].Encrypted=inptr[1]-'0';
							#ifdef WSOCKS_SSL
							if(pmod->UscPort[PortNo].Encrypted==2)
								pmod->UscPort[PortNo].Compressed=FALSE;
							#endif
						}
					}
#endif
#endif
					if(inptr[0]=='T')
					{
						pmod->UscPort[PortNo].TimeOut=atoi(&inptr[1]);
						if(!pmod->UscPort[PortNo].TimeOutSize)
							pmod->UscPort[PortNo].TimeOutSize=1000000;
					}
					if(inptr[0]=='S')
					{
						pmod->UscPort[PortNo].TimeOutSize=atoi(&inptr[1]);
						if(!pmod->UscPort[PortNo].TimeOut)
							pmod->UscPort[PortNo].TimeOut=60;
					}
					if(inptr[0]=='P')
					{
						pmod->UscPort[PortNo].TimeOutPeriod=atoi(&inptr[1]);
						if(!pmod->UscPort[PortNo].TimeOut)
							pmod->UscPort[PortNo].TimeOut=60;
						if(!pmod->UscPort[PortNo].TimeOutSize)
							pmod->UscPort[PortNo].TimeOutSize=1000000;
					}
					if(inptr[0]=='I')
					{
						strncpy(pmod->UscPort[PortNo].IPAclCfg,&inptr[1],sizeof(pmod->UscPort[PortNo].IPAclCfg)-1);
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->UscPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d,%d",&pmod->UscPort[PortNo].RecvBuffLimit,&pmod->UscPort[PortNo].SendBuffLimit);
					}
				}
				#ifdef WS_REALTIMESEND
				pmod->UscPort[PortNo].ImmediateSendLimit=pmod->UscPort[PortNo].BlockSize*1024;
				#endif
			}
			break;
#ifdef WS_GUARANTEED
		case 'g':
		case 'G':
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_UGC_PORTS))
			{
				pmod->WSCreatePort(WS_UGC,PortNo);
				strcpy(pmod->UgcPort[PortNo].LocalIP,port_ip);
				pmod->UgcPort[PortNo].Port=atoi(port_port);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->UgcPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->UgcPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='N') 
					{
						strncpy(pmod->UgcPort[PortNo].CfgNote,&inptr[1], 79);
						pmod->UgcPort[PortNo].CfgNote[79] = 0;
					}
#ifdef WS_COMPRESS
					if(inptr[0]=='C')
						pmod->UgcPort[PortNo].Compressed=TRUE;
#ifdef WS_ENCRYPTED
					if(inptr[0]=='E')
					{
						pmod->UgcPort[PortNo].Encrypted=TRUE;
						pmod->UgcPort[PortNo].Compressed=TRUE;
					}
#endif
#endif
					if(inptr[0]=='T')
					{
						pmod->UgcPort[PortNo].TimeOut=atoi(&inptr[1]);
						if(!pmod->UgcPort[PortNo].TimeOutSize)
							pmod->UgcPort[PortNo].TimeOutSize=1000000;
					}
					if(inptr[0]=='S')
					{
						pmod->UgcPort[PortNo].TimeOutSize=atoi(&inptr[1]);
						if(!pmod->UgcPort[PortNo].TimeOut)
							pmod->UgcPort[PortNo].TimeOut=60;
					}
					if(inptr[0]=='P')
					{
						pmod->UgcPort[PortNo].TimeOutPeriod=atoi(&inptr[1]);
						if(!pmod->UgcPort[PortNo].TimeOut)
							pmod->UgcPort[PortNo].TimeOut=60;
						if(!pmod->UgcPort[PortNo].TimeOutSize)
							pmod->UgcPort[PortNo].TimeOutSize=1000000;
					}
					if(inptr[0]=='G') 
					{
						strncpy(pmod->UgcPort[PortNo].GDCfg,&inptr[1], 127);
						pmod->UgcPort[PortNo].GDCfg[127] = 0;
					}
					if(inptr[0]=='J') 
						pmod->UgcPort[PortNo].GDGap=atoi(&inptr[1]);
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->UgcPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d,%d",&pmod->UgcPort[PortNo].RecvBuffLimit,&pmod->UgcPort[PortNo].SendBuffLimit);
					}
				}
				#ifdef WS_REALTIMESEND
				pmod->UgcPort[PortNo].ImmediateSendLimit=pmod->UgcPort[PortNo].BlockSize*1024;
				#endif
			}
			break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
		case 'm':
		case 'M':
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_UMC_PORTS))
			{
				pmod->WSCreatePort(WS_UMC,PortNo);
				strcpy(pmod->UmcPort[PortNo].LocalIP,port_ip);
				pmod->UmcPort[PortNo].Port=atoi(port_port);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->UmcPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->UmcPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='N') 
					{
						strncpy(pmod->UmcPort[PortNo].CfgNote,&inptr[1], 79);
						pmod->UmcPort[PortNo].CfgNote[79] = 0;
					}
#ifdef WS_COMPRESS
					if(inptr[0]=='C')
						pmod->UmcPort[PortNo].Compressed=TRUE;
#ifdef WS_ENCRYPTED
					if(inptr[0]=='E')
					{
						pmod->UmcPort[PortNo].Encrypted=TRUE;
						pmod->UmcPort[PortNo].Compressed=TRUE;
					}
#endif
#endif
					if(inptr[0]=='T')
					{
						pmod->UmcPort[PortNo].TimeOut=atoi(&inptr[1]);
						if(!pmod->UmcPort[PortNo].TimeOutSize)
							pmod->UmcPort[PortNo].TimeOutSize=1000000;
					}
					if(inptr[0]=='S')
					{
						pmod->UmcPort[PortNo].TimeOutSize=atoi(&inptr[1]);
						if(!pmod->UmcPort[PortNo].TimeOut)
							pmod->UmcPort[PortNo].TimeOut=60;
					}
					if(inptr[0]=='P')
					{
						pmod->UmcPort[PortNo].TimeOutPeriod=atoi(&inptr[1]);
						if(!pmod->UmcPort[PortNo].TimeOut)
							pmod->UmcPort[PortNo].TimeOut=60;
						if(!pmod->UmcPort[PortNo].TimeOutSize)
							pmod->UmcPort[PortNo].TimeOutSize=1000000;
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->UmcPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d,%d",&pmod->UmcPort[PortNo].RecvBuffLimit,&pmod->UmcPort[PortNo].SendBuffLimit);
					}
				}
				#ifdef WS_REALTIMESEND
				pmod->UmcPort[PortNo].ImmediateSendLimit=pmod->UmcPort[PortNo].BlockSize*1024;
				#endif
			}
			break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
		case 'o':
		case 'O':
			if((inptr=strtok(NULL,":\r\n"))==NULL)
			{
				WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.txt at line %d",line);
				break;
			}
			strcpy(port_out,inptr);

			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CTO_PORTS))
			{
				pmod->WSCreatePort(WS_CTO,PortNo);
				strcpy(pmod->CtoPort[PortNo].LocalIP,port_ip);
				pmod->CtoPort[PortNo].Port=atoi(port_port);
				strcpy(pmod->CtoPort[PortNo].RemoteIP,port_out);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->CtoPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->CtoPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='N') 
					{
						strncpy(pmod->CtoPort[PortNo].CfgNote,&inptr[1], 79);
						pmod->CtoPort[PortNo].CfgNote[79] = 0;
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->CtoPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d",&pmod->CtoPort[PortNo].SendBuffLimit);
					}
				}
			}
			break;
		case 'i':
		case 'I':
			if((inptr=strtok(NULL,":\r\n"))==NULL)
			{
				WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.txt at line %d",line);
				break;
			}
			strcpy(port_out,inptr);

			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CTI_PORTS))
			{
				pmod->WSCreatePort(WS_CTI,PortNo);
				strcpy(pmod->CtiPort[PortNo].LocalIP,port_ip);
				pmod->CtiPort[PortNo].Port=atoi(port_port);
				strcpy(pmod->CtiPort[PortNo].RemoteIP,port_out);
				strcpy(pmod->CtiPort[PortNo].AltRemoteIP[pmod->CtiPort[PortNo].AltIPCount++]
					,port_out);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->CtiPort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->CtiPort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='N') 
					{
						strncpy(pmod->CtiPort[PortNo].CfgNote,&inptr[1], 255);
						pmod->CtiPort[PortNo].CfgNote[255] = 0;
					}
					if(inptr[0]=='R') 
						Roaming=TRUE;
					if(inptr[0]=='B')
					{
						pmod->CtiPort[PortNo].Bounce=atol(&inptr[1]);
						if((pmod->CtiPort[PortNo].Bounce!=0)&&(pmod->CtiPort[PortNo].Bounce<500))
							pmod->CtiPort[PortNo].Bounce=500;
					}
					if(inptr[0]=='<')
						pmod->CtiPort[PortNo].BounceStart=atol(&inptr[1]);
					if(inptr[0]=='>')
						pmod->CtiPort[PortNo].BounceEnd=atol(&inptr[1]);
					if(inptr[0]=='A')
					{
						if(pmod->CtiPort[PortNo].AltIPCount<WS_MAX_ALT_PORTS)
						{
							strcpy(pmod->CtiPort[PortNo].AltRemoteIP[pmod->CtiPort[PortNo].AltIPCount++]
								,&inptr[1]);
						}
					}
					if(inptr[0]=='I')
					{
						strncpy(pmod->CtiPort[PortNo].IPAclCfg,&inptr[1],sizeof(pmod->CtiPort[PortNo].IPAclCfg)-1);
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->CtiPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
				#ifdef WS_CTI_PREFIX_PACKET_LEN
					if(inptr[0]=='P')
					{
						pmod->CtiPort[PortNo].PrefixPacketLen = 1;
					}
				#endif
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d",&pmod->CtiPort[PortNo].RecvBuffLimit);
					}
				}
				if(Roaming)
				{
					for (i=0;i<pmod->CtiPort[PortNo].AltIPCount;i++)
					{
						pmod->CtiPort[PortNo].AltRemoteIPOn[i]=TRUE;
					}
				}
				else
					pmod->CtiPort[PortNo].AltRemoteIPOn[0]=TRUE;
				pmod->CtiPort[PortNo].CurrentAltIP = -1;
			}
			break;
#endif //#ifdef WS_CAST
#ifdef WS_FILE_SERVER
		case 'f':
		case 'F':
			if((inptr=strtok(NULL,":\r\n"))==NULL)
			{
				WSHLogError(pmod,"!WSOCKS: FATAL ERROR : Incorrect Format in ports.txt at line %d",line);
				break;
			}
			strcpy(port_out,inptr);

			if((PortNo>=0)&&(PortNo<pmod->NO_OF_FILE_PORTS))
			{
				pmod->WSCreatePort(WS_FIL,PortNo);
				strcpy(pmod->FilePort[PortNo].LocalIP,port_ip);
				pmod->FilePort[PortNo].Port=atoi(port_port);
				strcpy(pmod->FilePort[PortNo].RemoteIP,port_out);
				strcpy(pmod->FilePort[PortNo].AltRemoteIP[pmod->FilePort[PortNo].AltIPCount++]
					,port_out);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='H')
						pmod->FilePort[PortNo].ConnectHold=TRUE;
					if(inptr[0]=='K')
						pmod->FilePort[PortNo].BlockSize=atoi(&inptr[1]);
					if(inptr[0]=='L')
						pmod->FilePort[PortNo].ChokeSize=atoi(&inptr[1]);
#ifdef WS_COMPRESS
					if(inptr[0]=='C')
						pmod->FilePort[PortNo].Compressed=TRUE;
#ifdef WS_ENCRYPTED
					if(inptr[0]=='E')
					{
						pmod->FilePort[PortNo].Encrypted=TRUE;
						pmod->FilePort[PortNo].Compressed=TRUE;
					}
#endif
#endif
					if(inptr[0]=='5') 
					{
						strncpy(pmod->FilePort[PortNo].S5RemoteIP,&inptr[1], 19);
						pmod->FilePort[PortNo].S5RemoteIP[19] = 0;
						pmod->FilePort[PortNo].S5Port=1080;
						pmod->FilePort[PortNo].S5Connect=TRUE;
						pmod->FilePort[PortNo].S5Methode=00;
						pmod->FilePort[PortNo].S5Version=5;
					}
					if(inptr[0]=='6') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->FilePort[PortNo].S5Port=atoi(Tempstr);
					}
					if(inptr[0]=='7') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->FilePort[PortNo].S5Methode=atoi(Tempstr);
					}
					if(inptr[0]=='8') 
					{
						strncpy(Tempstr,&inptr[1], 10);
						Tempstr[10]=0;
						pmod->FilePort[PortNo].S5Version=atoi(Tempstr);
					}
					if(inptr[0]=='N') 
					{
						strncpy(pmod->FilePort[PortNo].CfgNote,&inptr[1], 79);
						pmod->FilePort[PortNo].CfgNote[79] = 0;
					}
					if(inptr[0]=='R') 
						Roaming=TRUE;
					if(inptr[0]=='B')
					{
						pmod->FilePort[PortNo].Bounce=atol(&inptr[1]);
						if((pmod->FilePort[PortNo].Bounce!=0)&&(pmod->FilePort[PortNo].Bounce<500))
							pmod->FilePort[PortNo].Bounce=500;
					}
					if(inptr[0]=='<')
						pmod->FilePort[PortNo].BounceStart=atol(&inptr[1]);
					if(inptr[0]=='>')
						pmod->FilePort[PortNo].BounceEnd=atol(&inptr[1]);
					if(inptr[0]=='A')
					{
						if(pmod->FilePort[PortNo].AltIPCount<WS_MAX_ALT_PORTS)
						{
							strcpy(pmod->FilePort[PortNo].AltRemoteIP[pmod->FilePort[PortNo].AltIPCount++]
								,&inptr[1]);
						}
					}
					if(inptr[0]=='D')
					{
						strcpy(pmod->FilePort[PortNo].DriveList,&inptr[1]);
						_strupr(pmod->FilePort[PortNo].DriveList);
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->FilePort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
				#ifdef WS_DECLINING_RECONNECT
					if(inptr[0]=='Y')
					{
						const char *line=strtoke(&inptr[1],",");
						if(line)
							pmod->FilePort[PortNo].MinReconnectDelay=atol(line);
						line=strtoke(0,",");
						if(line)
							pmod->FilePort[PortNo].MaxReconnectDelay=atol(line);
						line=strtoke(0,",");
						if(line)
							pmod->FilePort[PortNo].MinReconnectReset=atol(line);
					}
				#endif
					if(inptr[0]=='Z') 
					{
						sscanf(&inptr[1],"%d,%d",&pmod->FilePort[PortNo].RecvBuffLimit,&pmod->FilePort[PortNo].SendBuffLimit);
					}
				}
				if(Roaming)
				{
					for (i=0;i<pmod->FilePort[PortNo].AltIPCount;i++)
					{
						pmod->FilePort[PortNo].AltRemoteIPOn[i]=TRUE;
					}
				}
				else
					pmod->FilePort[PortNo].AltRemoteIPOn[0]=TRUE;
				pmod->FilePort[PortNo].CurrentAltIP = -1;
				#ifdef WS_REALTIMESEND
				pmod->FilePort[PortNo].ImmediateSendLimit=pmod->FilePort[PortNo].BlockSize*1024;
				#endif
			}
			break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_OTHER
		case 'z':
		case 'Z':
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_OTHER_PORTS))
			{
				pmod->WSCreatePort(WS_OTH,PortNo);
				strcpy(pmod->OtherPort[PortNo].RemoteIP,port_ip);
				strcpy(pmod->OtherPort[PortNo].AltRemoteIP[pmod->OtherPort[PortNo].AltIPCount++]
					,port_ip);
				pmod->OtherPort[PortNo].Port=atoi(port_port);
				while((inptr=strtok(NULL,":\r\n"))!=NULL)
				{
					if(inptr[0]=='N') 
					{
						strncpy(pmod->OtherPort[PortNo].Note,&inptr[1], 49);
						pmod->OtherPort[PortNo].Note[49] = 0;
					}
					if(inptr[0]=='A')
					{
						if(pmod->OtherPort[PortNo].AltIPCount<WS_MAX_ALT_PORTS)
						{
							strcpy(pmod->OtherPort[PortNo].AltRemoteIP[pmod->OtherPort[PortNo].AltIPCount++]
								,&inptr[1]);
						}
					}
					if(inptr[0]=='M')
					{
						const char *line=strtoke(&inptr[1],",");
						for(int i=0;(i<8)&&(line);i++)
						{
							strncpy(pmod->OtherPort[PortNo].MonitorGroups[i],line,7);
							line=strtoke(0,",");
						}
					}
				}
			}
			break;
#endif //#ifdef WS_OTHER
		}
	}
	fclose(infile);
	return(TRUE);
}
int WsocksHostImpl::WSHSuspend(WsocksApp *pmod)
{
	int i;

	switch(pmod->WSSuspendMode)
	{
	case 0:
		for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
		{
			if(!pmod->ConPort[i].ConnectHold)
			{
				pmod->ConPort[i].ConnectHold=TRUE;
				pmod->ConPort[i].PortWasActiveBeforeSuspend=TRUE;
			}
			else
				pmod->ConPort[i].PortWasActiveBeforeSuspend=FALSE;
		}
		for(i=0;i<pmod->NO_OF_USC_PORTS;i++)
		{
			if(!pmod->UscPort[i].ConnectHold)
			{
				pmod->UscPort[i].ConnectHold=TRUE;
				pmod->UscPort[i].PortWasActiveBeforeSuspend=TRUE;
			}
			else
				pmod->UscPort[i].PortWasActiveBeforeSuspend=FALSE;
		}
#ifdef WS_GUARANTEED      
		for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
		{
			if(!pmod->CgdPort[i].ConnectHold)
			{
				pmod->CgdPort[i].ConnectHold=TRUE;
				pmod->CgdPort[i].PortWasActiveBeforeSuspend=TRUE;
			}
			else
				pmod->CgdPort[i].PortWasActiveBeforeSuspend=FALSE;
		}
		for(i=0;i<pmod->NO_OF_UGC_PORTS;i++)
		{
			if(!pmod->UgcPort[i].ConnectHold)
			{
				pmod->UgcPort[i].ConnectHold=TRUE;
				pmod->UgcPort[i].PortWasActiveBeforeSuspend=TRUE;
			}
			else
				pmod->UgcPort[i].PortWasActiveBeforeSuspend=FALSE;
		}
#endif
		pmod->WSSuspendMode=1;
		break;
	case 2:
		return TRUE;
	}
	return FALSE;
}
int WsocksHostImpl::WSHResume(WsocksApp *pmod)
{
	int i;

	if(pmod->WSSuspendMode > 0)
	{
		for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
		{
			if(pmod->ConPort[i].PortWasActiveBeforeSuspend)
			{
				pmod->ConPort[i].ConnectHold=FALSE;
			}
		}
		for(i=0;i<pmod->NO_OF_USC_PORTS;i++)
		{
			if(pmod->UscPort[i].PortWasActiveBeforeSuspend)
			{
				pmod->UscPort[i].ConnectHold=FALSE;
			}
		}
#ifdef WS_GUARANTEED      
		for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
		{
			if(pmod->CgdPort[i].PortWasActiveBeforeSuspend)
			{
				pmod->CgdPort[i].ConnectHold=FALSE;
			}
		}
		for(i=0;i<pmod->NO_OF_UGC_PORTS;i++)
		{
			if(pmod->UgcPort[i].PortWasActiveBeforeSuspend)
			{
				pmod->UgcPort[i].ConnectHold=FALSE;
			}
		}
#endif
		pmod->WSSuspendMode=0;
	}
	return 0;
}
int WsocksHostImpl::WSHCloseSocks(WsocksApp *pmod)
{
	if(pmod->WSInitCnt<=0)
		return 0;
	WSHLogEvent(pmod,"==========================CLOSING WSOCKS==========================");
	//SMSCleanup();

	if(lbmon)
	{
		if(pmod->pcfg->aclass=="LBSysmon")
			lbmon=0;
		else
			lbmon->LBMAppCloseSocks(pmod);
	}
#ifdef WS_FILE_SERVER
	WSHSetupFilePorts(pmod,WS_CLOSE);
	WSHFileTransferClean(pmod);
#endif
#ifdef WS_CAST
	WSHSetupCtoPorts(pmod,WS_CLOSE);
	WSHSetupCtiPorts(pmod,WS_CLOSE);
#endif //#ifdef WS_CAST
	WSHSetupConPorts(pmod,WS_CLOSE);
	WSHSetupUsrPorts(pmod,WS_CLOSE);
#ifdef WS_MONITOR
	WSHSetupUmrPorts(pmod,WS_CLOSE);
#endif //#ifdef WS_MONITOR
#ifdef WS_GUARANTEED
    WSHSetupCgdPorts(pmod,WS_CLOSE);
    WSHSetupUgrPorts(pmod,WS_CLOSE);
	for(int i=0;i<pmod->NO_OF_UGC_PORTS;i++)
	{
		while(pmod->UgcPort[i].GDLines)
		{
			GDLINE *old=pmod->UgcPort[i].GDLines;
			pmod->UgcPort[i].GDLines=pmod->UgcPort[i].GDLines->NextGDLine;
			free(old);
		}
	}
#endif
	pmod->WSInitCnt=0;

#ifdef WS_OIO
	//WSFlushCompletions();
	WSHWaitOverlapCancel(pmod);
	if(pmod->hIOPort != NULL)
	{
		::CloseHandle(pmod->hIOPort); 
		pmod->hIOPort = NULL;
	}
#endif

	int nports=(int)pmod->portMap.size();
	for(int i=0;i<nports;i++)
	{
		WSPort *pport=pmod->portMap.begin()->second;
		if(WSHDeletePort(pmod,pport->PortType,pport->PortNo)<0)
			_ASSERT(false);
	}

	if(pmod->ConPort) 
	{
		delete pmod->ConPort; pmod->ConPort=0;
	}
	if(pmod->UscPort) 
	{
		delete pmod->UscPort; pmod->UscPort=0;
	}
	if(pmod->UsrPort) 
	{
		delete pmod->UsrPort; pmod->UsrPort=0;
	}
	if(pmod->FilePort) 
	{
		delete pmod->FilePort; pmod->FilePort=0;
	}
	if(pmod->UmcPort) 
	{
		delete pmod->UmcPort; pmod->UmcPort=0;
	}
	if(pmod->UmrPort) 
	{
		delete pmod->UmrPort; pmod->UmrPort=0;
	}
	if(pmod->CgdPort) 
	{
		delete pmod->CgdPort; pmod->CgdPort=0;
	}
	if(pmod->UgcPort) 
	{
		delete pmod->UgcPort; pmod->UgcPort=0;
	}
	if(pmod->UgrPort) 
	{
		delete pmod->UgrPort; pmod->UgrPort=0;
	}
	if(pmod->CtoPort) 
	{
		delete pmod->CtoPort; pmod->CtoPort=0;
	}
	if(pmod->CtiPort) 
	{
		delete pmod->CtiPort; pmod->CtiPort=0;
	}
	if(pmod->OtherPort) 
	{
		delete pmod->OtherPort; pmod->OtherPort=0;
	}
	return(TRUE);
}

void WsocksHostImpl::hostFileLogError(char *Error)
{
	if (WSHDate!=hostErrFileDate)
	{
		if(hostErrFile)
			fclose(hostErrFile);
		hostErrFile=NULL;
	}
	if(!hostErrFile)
	{
		char fpath[MAX_PATH]={0};
		sprintf(fpath,"%s\\%sher.txt",hostLogDir,WSHcDate);
		if((hostErrFile=fopen(fpath,"a"))==NULL)
			return;
		hostErrFileDate=WSHDate;
	}

	if(fprintf(hostErrFile,"%s\n",Error)<0)
		return;
	fflush(hostErrFile);
}
void WsocksHostImpl::hostFileLogEvent(char *Event)
{
	if (WSHDate!=hostEveFileDate)
	{
		if(hostEveFile)
			fclose(hostEveFile);
		hostEveFile=NULL;
	}
	if(!hostEveFile)
	{
		char fpath[MAX_PATH]={0};
		sprintf(fpath,"%s\\%shev.txt",hostLogDir,WSHcDate);
		if((hostEveFile=fopen(fpath,"a"))==NULL)
			return;
		hostEveFileDate=WSHDate;
	}

	if(fprintf(hostEveFile,"%s\n",Event)<0)
		return;
	fflush(hostEveFile);
}
void WsocksHostImpl::WSHostLogError(const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHostLogErrorVA(format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHostLogErrorVA(const char *format, va_list& ptr)
{
	// Create a UUID for this error
	UUID uuid;
	unsigned char *suuid;
	UuidCreateSequential(&uuid);
	UuidToString(&uuid, &suuid);

	char buf[2048];
	//WSErrorCount++;
	vsnprintf_s ( buf, sizeof(buf), _TRUNCATE, format, ptr ) ;

	char Tempstr[2048],*tptr=Tempstr;
	sprintf(tptr,"%8s:%c%c@%6s | ","",WSHcDate[6],WSHcDate[7],WSHcTime); tptr+=strlen(tptr);
	strncpy(tptr,buf,1900);  tptr+=strlen(tptr);
	sprintf(tptr," {%s}",suuid); tptr+=strlen(tptr);
	*tptr=0;
	_ASSERT(tptr -Tempstr<2047);
	RpcStringFree(&suuid);

	//__int64 coff=0;
	//if(pmod->fpErr)
	//	fgetpos(pmod->fpErr,&coff);
	hostFileLogError(Tempstr);
	hostFileLogEvent(Tempstr);

	//if(lbmon)
	//{
	//	char mbuf[2048+256]={0},*mptr=mbuf;
	//	int mlen=sizeof(mbuf);
	//	for(int i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	//	{
	//		if(!pmod->UmrPort[i].SockActive)
	//			continue;
	//		SysmonFlags *smflags=(SysmonFlags*)pmod->UmrPort[i].DetPtr3;
	//		if((smflags)&&(smflags->wantError))
	//		{
	//			_ASSERT(false);//untested
	//			if(!*mbuf)
	//			{
	//				pmod->fcodec->EncodeLog(mptr,mlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(), 
	//					pmod->WSErrorLogPath.c_str(),false,coff,Tempstr,strlen(Tempstr)+1);
	//			}
	//			WSHUmrSendMsg(pmod,1110,strlen(Tempstr)+1,Tempstr,i);
	//		}
	//	}
	//}

	WSHHostLoggedError(Tempstr);
}
void WsocksHostImpl::WSHostLogEvent(const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHostLogEventVA(format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHostLogEventVA(const char *format, va_list& ptr)
{
	char buf[2048];
	vsnprintf_s ( buf, sizeof(buf), _TRUNCATE, format, ptr ) ;

	char Tempstr[2048];
	sprintf(Tempstr,"%8s:%c%c@%6s | ","",WSHcDate[6],WSHcDate[7],WSHcTime);
	strncat(Tempstr,buf, 2000);
	Tempstr[2021] = 0;

	hostFileLogEvent(Tempstr);

	//if(lbmon)
	//{
	//	char mbuf[2048+256]={0},*mptr=mbuf;
	//	int mlen=sizeof(mbuf);
	//	for(int i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	//	{
	//		if(!pmod->UmrPort[i].SockActive)
	//			continue;
	//		SysmonFlags *smflags=(SysmonFlags*)pmod->UmrPort[i].DetPtr3;
	//		if((smflags)&&(smflags->wantEvent))
	//		{
	//			_ASSERT(false);//untested
	//			if(!*mbuf)
	//			{
	//				pmod->fcodec->EncodeLog(mptr,mlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(), 
	//					pmod->WSEventLogPath.c_str(),false,coff,Tempstr,strlen(Tempstr)+1);
	//			}
	//			WSHUmrSendMsg(pmod,1110,strlen(Tempstr)+1,Tempstr,i);
	//		}
	//	}
	//}

	WSHHostLoggedEvent(Tempstr);
}
int WsocksHostImpl::WSHIdle(ULONGLONG tstop)
{
	for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
	{
		WsocksApp *pmod=ait->second; ait++;
		pmod->threadIdleStop=tstop;
	}
	return 0;
}
//void WsocksHostImpl::WSHLogSendTimeout(int PortType, int PortNo)
//{
//return;
//	WsocksApp *pmod=0;
//	CONPORT *ConPort=pmod->ConPort;
//	USCPORT *UscPort=pmod->UscPort;
//	USRPORT *UsrPort=pmod->UsrPort;
//	FILEPORT *FilePort=pmod->FilePort;
//	UMCPORT *UmcPort=pmod->UmcPort;
//	UMRPORT *UmrPort=pmod->UmrPort;
//	CGDPORT *CgdPort=pmod->CgdPort;
//	UGCPORT *UgcPort=pmod->UgcPort;
//	UGRPORT *UgrPort=pmod->UgrPort;
//	CTOPORT *CtoPort=pmod->CtoPort;
//	CTIPORT *CtiPort=pmod->CtiPort;
//	OTHERPORT *OtherPort=pmod->OtherPort;
//	int *Ctr=0;
//	switch(PortType)
//	{
//	case WS_CON: Ctr=&pmod->ConPort[PortNo].SendTimeOut; break;
//	case WS_USR: Ctr=&pmod->UsrPort[PortNo].SendTimeOut; break;
//#ifdef WS_CAST
//	case WS_CTO: Ctr=&pmod->CtoPort[PortNo].SendTimeOut; break;
//#endif
//#ifdef WS_MONITOR
//	case WS_UMR: Ctr=&UmrPort[PortNo].SendTimeOut; break;
//#endif
//#ifdef WS_GUARANTEED
//	case WS_UGR: Ctr=&pmod->UgrPort[PortNo].SendTimeOut; break;
//	case WS_CGD: Ctr=&pmod->CgdPort[PortNo].SendTimeOut; break;
//#endif
//#ifdef WS_FILE_SERVER
//	case WS_FIL: Ctr=&pmod->FilePort[PortNo].SendTimeOut; break;
//#endif
//	}
//	if(Ctr)
//	{
//#ifdef WS_RETRY_LOG
//		if((WSEnableRetryLog)&&(*Ctr>0))
//		{
//			LV_FINDINFO lvfi;
//			memset(&lvfi,0,sizeof(LV_FINDINFO));
//			lvfi.flags=LVFI_PARAM;
//			lvfi.lParam=GetDispItem(PortType,PortNo);
//			int iItem=ListView_FindItem(WShConList,-1,&lvfi);
//			char pstr[32]={0},istr[32]={0},nstr[256]={0},sstr[256]={0},rstr[256]={0},tstr[256]={0};
//			if(iItem>=0)
//			{
//				ListView_GetItemText(WShConList,iItem,0,pstr,31);
//				ListView_GetItemText(WShConList,iItem,1,istr,31);
//				ListView_GetItemText(WShConList,iItem,2,nstr,255);
//				ListView_GetItemText(WShConList,iItem,3,sstr,255);
//				ListView_GetItemText(WShConList,iItem,4,rstr,255);
//				ListView_GetItemText(WShConList,iItem,5,tstr,255);
//			}
//			WSLogRetry("\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%d\"",pstr,istr,nstr,sstr,rstr,tstr,*Ctr);
//		}
//#endif
//	}
//}

// Upcalls from WsocksApp
int WsocksHostImpl::WSHCloseApp(WsocksApp *pmod)
{
	WSHDequeueApp(pmod);
	return 0;
}
//int WsocksHostImpl::WSHExitApp(WsocksApp *pmod, int ecode)
//{
//	if(IsWindow(pmod->WShWnd))
//		PostMessage(pmod->WShWnd,WM_CLOSE,0,0);
//	return 0;
//}
WSPort *WsocksHostImpl::WSHNewPort(WsocksApp *pmod, WSPortType PortType, int PortNo)
{
	WSPort *pport=new WSPort;
	if(!pport)
		return 0;
	pport->pmod=pmod;
	pport->PortVers=WS_VERS_1_0;
	pport->PortType=PortType;
	pport->PortNo=PortNo;
	pport->SetName();
	pport->sd=INVALID_SOCKET;
	WSHPortCreated(pmod,pport);
	return pport;
}
WSPort *WsocksHostImpl::WSHCreatePort(WsocksApp *pmod, WSPortType PortType, int PortNo)
{
	WSPort *pport=(WSPort *)WSHNewPort(pmod,PortType,PortNo);
	if(pport)
	{
		switch(PortType)
		{
		case WS_CON: 
			if(!WSHGetPort(pmod,WS_CON_TOT,pmod->NO_OF_CON_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_CON_PORTS,WS_CON_TOT)]=WSHNewPort(pmod,WS_CON_TOT,pmod->NO_OF_CON_PORTS);
			break;
		case WS_USC:
		case WS_USR:
			if(!WSHGetPort(pmod,WS_USR_TOT,pmod->NO_OF_USR_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_USR_PORTS,WS_USR_TOT)]=WSHNewPort(pmod,WS_USR_TOT,pmod->NO_OF_USR_PORTS);
			break;
		case WS_FIL:
			if(!WSHGetPort(pmod,WS_FIL_TOT,pmod->NO_OF_FILE_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_FILE_PORTS,WS_FIL_TOT)]=WSHNewPort(pmod,WS_FIL_TOT,pmod->NO_OF_FILE_PORTS);
			break;
		case WS_UMC:
		case WS_UMR:
			//if(!WSHGetPort(pmod,WS_UMR_TOT,pmod->NO_OF_UMR_PORTS))
			//	pmod->portMap[MAKELONG(pmod->NO_OF_UMR_PORTS,WS_UMR_TOT)]=WSHNewPort(pmod,WS_UMR_TOT,pmod->NO_OF_UMR_PORTS);
			break;
		case WS_CGD:
			if(!WSHGetPort(pmod,WS_CGD_TOT,pmod->NO_OF_CGD_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_CGD_PORTS,WS_CGD_TOT)]=WSHNewPort(pmod,WS_CGD_TOT,pmod->NO_OF_CGD_PORTS);
			break;
		case WS_UGC:
		case WS_UGR:
			if(!WSHGetPort(pmod,WS_UGR_TOT,pmod->NO_OF_UGR_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_UGR_PORTS,WS_UGR_TOT)]=WSHNewPort(pmod,WS_UGR_TOT,pmod->NO_OF_UGR_PORTS);
			break;
		case WS_CTO:
			if(!WSHGetPort(pmod,WS_CTO_TOT,pmod->NO_OF_CTO_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_CTO_PORTS,WS_CTO_TOT)]=WSHNewPort(pmod,WS_CTO_TOT,pmod->NO_OF_CTO_PORTS);
			break;
		case WS_CTI:
			if(!WSHGetPort(pmod,WS_CTI_TOT,pmod->NO_OF_CTI_PORTS))
				pmod->portMap[MAKELONG(pmod->NO_OF_CTI_PORTS,WS_CTI_TOT)]=WSHNewPort(pmod,WS_CTI_TOT,pmod->NO_OF_CTI_PORTS);
			break;
		case WS_OTH:
			//if(!WSHGetPort(pmod,WS_OTH_TOT,pmod->NO_OF_OTHER_PORTS))
			//	pmod->portMap[MAKELONG(pmod->NO_OF_OTHER_PORTS,WS_OTH_TOT)]=WSHNewPort(pmod,WS_OTH_TOT,pmod->NO_OF_OTHER_PORTS);
			break;
		}
        if(pmod->portMap[MAKELONG(PortNo,PortType)])
            delete(pmod->portMap[MAKELONG(PortNo,PortType)]);
		pmod->portMap[MAKELONG(PortNo,PortType)]=pport;
	}
	return pport;
}
int WsocksHostImpl::WSHSetMaxPorts(WsocksApp *pmod, WSPortType PortType, int NumPorts)
{
	if((PortType<0)||(PortType>=WS_TYPE_COUNT)||(NumPorts<0))
		return -1;
	//portLimits[PortType]=NumPorts;
	_ASSERT(false);
	return 0;
}
WSPort *WsocksHostImpl::WSHGetPort(WsocksApp *pmod, WSPortType PortType, int PortNo)
{
	WSPORTMAP::iterator pit=pmod->portMap.find(MAKELONG(PortNo,PortType));
	if(pit==pmod->portMap.end())
		return 0;
	return pit->second;
}
int WsocksHostImpl::WSHGetPorts(WsocksApp *pmod, WSPortType PortType, WSPORTLIST& portList)
{
	for(WSPORTMAP::iterator pit=pmod->portMap.begin();pit!=pmod->portMap.end();pit++)
	{
		WSPort *pport=pit->second;
		if((PortType==WS_UNKNOWN)||(pport->PortType==PortType))
			portList.push_back(pport);
	}
	return 0;
}
int WsocksHostImpl::WSHUpdatePort(WsocksApp *pmod, WSPortType PortType, int PortNo)
{
	WSPort *pport=WSHGetPort(pmod,PortType,PortNo);
	if(!pport)
		return -1;
	WSHPortModified(pmod,pport);
	//switch(PortType)
	//{
	//case WS_CON: WSHPortChanged(pmod,pmod->ConPort[PortNo],PortNo); break;
	//case WS_USC: WSHPortChanged(pmod,pmod->UscPort[PortNo],PortNo); break;
	//case WS_USR: WSHPortChanged(pmod,pmod->UsrPort[PortNo],PortNo); break;
	//case WS_FIL: WSHPortChanged(pmod,pmod->FilePort[PortNo],PortNo); break;
	//case WS_UMC: WSHPortChanged(pmod,pmod->UmcPort[PortNo],PortNo); break;
	//case WS_UMR: WSHPortChanged(pmod,pmod->UmrPort[PortNo],PortNo); break;
	//case WS_CGD: WSHPortChanged(pmod,pmod->CgdPort[PortNo],PortNo); break;
	//case WS_UGC: WSHPortChanged(pmod,pmod->UgcPort[PortNo],PortNo); break;
	//case WS_UGR: WSHPortChanged(pmod,pmod->UgrPort[PortNo],PortNo); break;
	//case WS_CTI: WSHPortChanged(pmod,pmod->CtiPort[PortNo],PortNo); break;
	//case WS_CTO: WSHPortChanged(pmod,pmod->CtoPort[PortNo],PortNo); break;
	//case WS_OTH: WSHPortChanged(pmod,pmod->OtherPort[PortNo],PortNo); break;
	//default: return -1;
	//};
	// Notify lbmonitor when ports disappear or are dynamically created
	if(lbmon)
		lbmon->LBMPortChanged(pmod,pport);
	return 0;
}
int WsocksHostImpl::WSHDeletePort(WsocksApp *pmod, WSPortType PortType, int PortNo)
{
	WSPORTMAP::iterator pit=pmod->portMap.find(MAKELONG(PortNo,PortType));
	if(pit==pmod->portMap.end())
		return -1;
	WSPort *pport=pit->second;
	WSHPortDeleted(pmod,pport);
	pmod->portMap.erase(pit);
	delete pport;
	return 0;
}
int WsocksHostImpl::WSHLockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send)
{
	LockPort(pmod,PortType,PortNo,send);
	return 0;
}
int WsocksHostImpl::WSHUnlockPort(WsocksApp *pmod, WSPortType PortType, int PortNo, bool send)
{
	UnlockPort(pmod,PortType,PortNo,send);
	return 0;
}

SOCKET_T WsocksHostImpl::WSHSocket(WsocksApp *pmod, WSPortType PortType, int PortNo)
{
	WSPort *pport=pmod->WSGetPort(PortType,PortNo);
	if(!pport)
		return INVALID_SOCKET_T;
	int type=0,proto=0;
	switch(PortType)
	{
	case WS_CON:
	case WS_USC:
	case WS_USR:
	case WS_FIL:
	case WS_UMC:
	case WS_UMR:
	case WS_CGD:
	case WS_UGC:
	case WS_UGR:
		type=SOCK_STREAM;
		proto=IPPROTO_TCP;
		break;
	case WS_CTO:
	case WS_CTI:
		type=SOCK_DGRAM;
		proto=IPPROTO_UDP;
		break;
	default:
		return INVALID_SOCKET_T;
	}
	pport->sd=socket(AF_INET,type,proto);
	if(pport->sd==INVALID_SOCKET)
		return INVALID_SOCKET_T;
	return (SOCKET_T)pport;
}
int WsocksHostImpl::WSHIoctlSocket(WSPort *pport, long cmd, u_long FAR *argp)
{
	return ioctlsocket(pport->sd,cmd,argp);
}
int WsocksHostImpl::WSHSetSockOpt(WSPort *pport, int level, int optname, const char FAR * optval, int optlen)
{
	return setsockopt(pport->sd,level,optname,optval,optlen);
}
int WsocksHostImpl::WSHBindPort(WSPort *pport, IN_ADDR *iface, int port)
{
	SOCKADDR_IN baddr;
	baddr.sin_family=AF_INET;
	baddr.sin_addr.s_addr=iface->s_addr;
	baddr.sin_port=htons(port);
	if(bind(pport->sd,(SOCKADDR*)&baddr,sizeof(SOCKADDR))<0)
		return -1;

	int balen=sizeof(SOCKADDR_IN);
	if(!getsockname(pport->sd,(SOCKADDR*)&baddr,&balen))
	{
		#ifdef WS_LOOPBACK
		if(((pport->pmod->pcfg->wshLoopback)||(pport->PortType==WS_UMC))&&
		   (ntohl(iface->s_addr)==INADDR_LOOPBACK))
		{
			#ifdef LBAMUX_CRIT_SECTION
			EnterCriticalSection(&lbaMutex);
			#else
			WaitForSingleObject(lbaMutex,INFINITE);
			#endif
			char lbaKey[32]={0};
			if(ntohl(iface->s_addr)==INADDR_ANY)
				sprintf(lbaKey,"127.0.0.1:%d",ntohs(baddr.sin_port));
			else
				sprintf(lbaKey,"%s:%d",inet_ntoa(baddr.sin_addr),ntohs(baddr.sin_port));
			pport->lbaKey=lbaKey;			
			lbsMap[lbaKey]=pport;
			// Check pending loopback connect list
			for(LBCLIST::iterator cit=lbcList.begin();cit!=lbcList.end();)
			{
				WSPort *cport=*cit;
				if(!LBConnect(pport->pmod,cport))
					cit=lbcList.erase(cit);
				else
					cit++;
			}
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
		}
		#endif
	}
	return 0;
}
int WsocksHostImpl::WSHListen(WSPort *pport, int backlog)
{
	WSHPortModified(pport->pmod,pport);
	return listen(pport->sd,backlog);
}
SOCKET_T WsocksHostImpl::WSHAccept(WsocksApp *pmod, WSPort *pport, WSPortType accPortType, int accPortNo, struct sockaddr FAR * addr, int FAR * addrlen)
{
	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	_ASSERT(pport);
	if(!pport->lbaKey.empty())
	{
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		if(!pport->lbaPending.empty())
		{
			_ASSERT((pport->PortType==WS_USC)||(pport->PortType==WS_UMC));
			WSPORTSET::iterator cit=pport->lbaPending.begin();
			WSPort *cport=*cit;
			pport->lbaPending.erase(cit);

			//WSPort *aport=new WSPort;
			WSPort *aport=pmod->WSCreatePort(accPortType,accPortNo);
			aport->pmod=pport->pmod;
			aport->PortVers=pport->PortVers;
			aport->PortType=accPortType;
			aport->PortNo=accPortNo;
			aport->SetName();
			aport->sd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			aport->lbaKey=pport->lbaKey;
			BUFFER *abuf=new BUFFER;
			if(pport->PortType==WS_USC)
				WSOpenBuffLimit(abuf,pmod->UscPort[pport->PortNo].BlockSize,pmod->UscPort[pport->PortNo].RecvBuffLimit);
			else if(pport->PortType==WS_UMC)
				WSOpenBuffLimit(abuf,pmod->UmcPort[pport->PortNo].BlockSize,pmod->UmcPort[pport->PortNo].RecvBuffLimit);
			aport->lbaBuffer=abuf;
			aport->lbaPeer=cport;
			BUFFER *cbuf=new BUFFER;
			if(pport->PortType==WS_USC)
				WSOpenBuffLimit(cbuf,pmod->UscPort[pport->PortNo].BlockSize,pmod->UscPort[pport->PortNo].RecvBuffLimit);
			else if(pport->PortType==WS_UMC)
				WSOpenBuffLimit(cbuf,pmod->UmcPort[pport->PortNo].BlockSize,pmod->UmcPort[pport->PortNo].RecvBuffLimit);
			cport->lbaBuffer=cbuf;
			cport->lbaPeer=aport;
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			WSHLogEvent(pport->pmod,"%s accepted internally...",aport->PortName.c_str());
			WSHPortModified(aport->pmod,aport);

			if((addr)&&(addrlen)&&(*addrlen>=(int)sizeof(SOCKADDR_IN)))
			{
				SOCKADDR_IN iaddr;
				iaddr.sin_family=AF_INET;
				iaddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
				iaddr.sin_port=0;
				memcpy(addr,&iaddr,sizeof(SOCKADDR_IN));
				*addrlen=sizeof(SOCKADDR_IN);
			}
			return (SOCKET_T)aport;
		}
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
	}
	#endif

	SOCKET asd=accept(pport->sd,addr,addrlen);
	if(asd==INVALID_SOCKET)
		return INVALID_SOCKET_T;
    // deny the connection (out of USR ports)
    if(accPortNo<0)
    {
        closesocket(asd);
        return INVALID_SOCKET_T;
    }
	// determine which user channel is available
	//WSPort *aport=new WSPort;
	WSPort *aport=pmod->WSCreatePort(accPortType,accPortNo);
	aport->pmod=pport->pmod;
	aport->PortVers=pport->PortVers;
	aport->PortType=accPortType;
	aport->PortNo=accPortNo;
	aport->SetName();
	aport->sd=asd;
	WSHPortModified(aport->pmod,aport);
	return (SOCKET_T)aport;
}
int WsocksHostImpl::WSHSelect(LONGLONG nfds, ws_fd_set FAR * readfds, ws_fd_set FAR * writefds, ws_fd_set FAR *exceptfds, const struct timeval FAR * timeout)
{
	fd_set rds,wds,eds,*prds=0,*pwds=0,*peds=0;
	if(readfds)
	{
		FD_ZERO(&rds);
		for(WORD i=0;i<readfds->fd_count;i++)
		{
			WSPort *pport=(WSPort *)readfds->fd_array[i];
			_ASSERT(pport);
		#ifdef WS_LOOPBACK
			// Short-circuit loopback
			if(!pport->lbaKey.empty())
			{
				#ifdef LBAMUX_CRIT_SECTION
				EnterCriticalSection(&lbaMutex);
				#else
				WaitForSingleObject(lbaMutex,INFINITE);
				#endif
				// Pending accept?
				if(!pport->lbaPending.empty())
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(lbaMutex);
					#endif
					WS_FD_ZERO(readfds); WS_FD_SET((SOCKET_T)pport,readfds);
					if(writefds) WS_FD_ZERO(writefds);
					if(exceptfds) WS_FD_ZERO(exceptfds);
					return 1;
				}
				// Reset by peer or ready to read?
				else if((pport->lbaPeer)||(pport->lbaConnReset))
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(lbaMutex);
					#endif
					WS_FD_ZERO(readfds);
					if(writefds) WS_FD_ZERO(writefds);
					if(exceptfds) WS_FD_ZERO(exceptfds);
					if((pport->lbaBuffer->Size>0)||(pport->lbaConnReset))
					{
						WS_FD_SET((SOCKET_T)pport,readfds);
						return 1;
					}
					return 0;
				}
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				continue;
			}
		#endif
			FD_SET(pport->sd,&rds); prds=&rds;
		}
	#ifdef WS_LOOPBACK
		if(rds.fd_count<1)
			return 0;
	#endif
	}
	if(writefds)
	{
		FD_ZERO(&wds);
		for(WORD i=0;i<writefds->fd_count;i++)
		{
			WSPort *pport=(WSPort *)writefds->fd_array[i];
			FD_SET(pport->sd,&wds); pwds=&wds;
		}
	}
	if(exceptfds)
	{
		FD_ZERO(&eds);
		for(WORD i=0;i<exceptfds->fd_count;i++)
		{
			WSPort *pport=(WSPort *)exceptfds->fd_array[i];
			FD_SET(pport->sd,&eds); peds=&eds;
		}
	}
	int rc=select((int)nfds,prds,pwds,peds,timeout);
	if(rc>0)
	{
		if(readfds)
		{
			ws_fd_set tfd;
			WS_FD_ZERO(&tfd);
			for(WORD i=0;i<readfds->fd_count;i++)
			{
				WSPort *pport=(WSPort *)readfds->fd_array[i];
				if(FD_ISSET(pport->sd,prds))
                    WS_FD_SET((SOCKET_T)pport,&tfd);
			}
			*readfds=tfd;
		}
		if(writefds)
		{
			ws_fd_set tfd;
			WS_FD_ZERO(&tfd);
			for(WORD i=0;i<writefds->fd_count;i++)
			{
				WSPort *pport=(WSPort *)writefds->fd_array[i];
				if(FD_ISSET(pport->sd,pwds))
                    WS_FD_SET((SOCKET_T)pport,&tfd);
			}
			*writefds=tfd;
		}
		if(exceptfds)
		{
			ws_fd_set tfd;
			WS_FD_ZERO(&tfd);
			for(WORD i=0;i<exceptfds->fd_count;i++)
			{
				WSPort *pport=(WSPort *)exceptfds->fd_array[i];
				if(FD_ISSET(pport->sd,peds))
                    WS_FD_SET((SOCKET_T)pport,&tfd);
			}
			*exceptfds=tfd;
		}
	}
	else
	{
		if(readfds) WS_FD_ZERO(readfds);
		if(writefds) WS_FD_ZERO(writefds);
		if(exceptfds) WS_FD_ZERO(exceptfds);
	}
	return rc;
}
#ifdef WS_LOOPBACK
int WsocksHostImpl::LBConnect(WsocksApp *pmod, WSPort *pport)
{
	LBSMAP::iterator lit=lbsMap.find(pport->lbaKey);
	if(lit!=lbsMap.end())
	{
		WSPort *sport=lit->second;
		sport->lbaPending.insert(pport);
		return 0;
	}
	return -1;
}
#endif
int WsocksHostImpl::WSHConnectPort(WSPort *pport, IN_ADDR *dest, int port)
{
	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	if((pport->pmod->pcfg->wshLoopback)&&(ntohl(dest->s_addr)==INADDR_LOOPBACK))
	{
		//WSHLogEvent(pport->pmod,"%s connecting internally...",pport->PortName.c_str());
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		char lbaKey[32]={0};
		sprintf(lbaKey,"%s:%d",inet_ntoa(*dest),port);
		pport->lbaKey=lbaKey;

		if(LBConnect(pport->pmod,pport)<0)
			lbcList.push_back(pport);
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
		WSASetLastError(WSAEWOULDBLOCK);
		return -1;
	}
	#endif

	memset(&pport->caddr,0,sizeof(SOCKADDR_IN));
	pport->caddr.sin_family=AF_INET;
	pport->caddr.sin_addr.s_addr=dest->s_addr;
	pport->caddr.sin_port=htons(port);
	WSHPortModified(pport->pmod,pport);
	return connect(pport->sd,(SOCKADDR*)&pport->caddr,sizeof(SOCKADDR));
}
int WsocksHostImpl::WSHFinishedConnect(WSPort *pport)
{
	#ifdef WS_LOOPBACK
		// Short-circuit loopback
		_ASSERT(pport);
		if(!pport->lbaKey.empty())
		{
			#ifdef LBAMUX_CRIT_SECTION
			EnterCriticalSection(&lbaMutex);
			#else
			WaitForSingleObject(lbaMutex,INFINITE);
			#endif
			if(pport->lbaPeer)
			{
				// ConOpened should occur after UxrOpened
				if(((pport->lbaPeer->PortType==WS_USR)&&(pport->lbaPeer->pmod->UsrPort[pport->lbaPeer->PortNo].SockActive))||
				   ((pport->lbaPeer->PortType==WS_UMR)&&(pport->lbaPeer->pmod->UmrPort[pport->lbaPeer->PortNo].SockActive)))
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(lbaMutex);
					#endif
					return 1;
				}
			}
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			return 0;
		}
	#endif

        // Check the real socket
		struct timeval TimeVal={0,0};
        fd_set Fd_Set;
        FD_ZERO(&Fd_Set);
        FD_SET(pport->sd,&Fd_Set);
        select((int)pport->sd+1,NULL,&Fd_Set,0,(timeval*)&TimeVal);
        int Status=FD_ISSET(pport->sd,&Fd_Set);
	#ifndef _CONSOLE
		// Linux code needs extra checks
        if(Status>0)
        {
			connect(pport->sd,(SOCKADDR*)&pport->caddr,sizeof(SOCKADDR_IN));
			if(WSAGetLastError()==WSAECONNREFUSED)
				Status=-1;
			else if(WSAGetLastError()!=WSAEISCONN)
				Status=0;
        }
	#endif
        return Status;
}
int WsocksHostImpl::WSHSendPort(WSPort *pport, const char *wbuf, int wlen, int flags)
{
	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	_ASSERT(pport);
	if((pport)&&(!pport->lbaKey.empty()))
	{
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		if(pport->lbaPeer)
		{
			if(!pport->lbaPeer->lbaBuffer)
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAENOBUFS);
				return -1;
			}
			if(pport->lbaPeer->lbaBuffer->Size>(1*1024*1024))
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAEWOULDBLOCK);
				return 0;
			}
			int sbytes=0;
			while(wlen>0)
			{
				int wbytes=wlen;
				if(wbytes>((int)pport->lbaPeer->lbaBuffer->BlockSize)*1024)
					wbytes=((int)pport->lbaPeer->lbaBuffer->BlockSize)*1024;
				if(WSWriteBuff(pport->lbaPeer->lbaBuffer,(char*)wbuf,wbytes)<0)
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(lbaMutex);
					#endif
					WSASetLastError(WSAENOBUFS);
					return -1;
				}
				sbytes+=wbytes; wlen-=wbytes;
			}
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			return sbytes;
		}
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
	}
	#endif

	return send(pport->sd,wbuf,wlen,flags);
}
int WsocksHostImpl::WSHRecv(WSPort *pport, char FAR * buf, int len, int flags)
{
	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	_ASSERT(pport);
	if((pport)&&(!pport->lbaKey.empty()))
	{
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		if(pport->lbaConnReset)
		{
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			WSASetLastError(WSAECONNRESET);
			return -1;
		}
		if(pport->lbaPeer)
		{
			if(!pport->lbaBuffer)
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAENOBUFS);
				return -1;
			}
			if(pport->lbaBuffer->Size<=0)
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAEWOULDBLOCK);
				return -1;
			}
			int wbytes=0;
			while(pport->lbaBuffer->Size>0)
			{
				int rbytes=pport->lbaBuffer->LocalSize;
				if(rbytes>((int)pport->lbaBuffer->BlockSize)*1024)
					rbytes=((int)pport->lbaBuffer->BlockSize)*1024;
				if(rbytes>len)
					rbytes=len;
				memcpy(buf,pport->lbaBuffer->Block,rbytes);
				if(WSStripBuff(pport->lbaBuffer,rbytes)<0)
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(&lbaMutex);
					#endif
					WSASetLastError(WSAENOBUFS);
					return -1;
				}
				wbytes+=rbytes; buf+=rbytes; len-=rbytes;
			}
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			return wbytes;
		}
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
	}
	#endif

	return recv(pport->sd,buf,len,flags);
}
int WsocksHostImpl::WSHSendToPort(WSPort *pport, const char *wbuf, int wlen, int flags, const struct sockaddr FAR *to, int tolen)
{
	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	_ASSERT(pport);
	if((pport)&&(!pport->lbaKey.empty()))
	{
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		if(pport->lbaPeer)
		{
			if(!pport->lbaPeer->lbaBuffer)
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAENOBUFS);
				return -1;
			}
			if(pport->lbaPeer->lbaBuffer->Size>(1*1024*1024))
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAEWOULDBLOCK);
				return 0;
			}
			int sbytes=0;
			while(wlen>0)
			{
				int wbytes=wlen;
				if(wbytes>((int)pport->lbaPeer->lbaBuffer->BlockSize)*1024)
					wbytes=((int)pport->lbaPeer->lbaBuffer->BlockSize)*1024;
				if(WSWriteBuff(pport->lbaPeer->lbaBuffer,(char*)wbuf,wbytes)<0)
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(lbaMutex);
					#endif
					WSASetLastError(WSAENOBUFS);
					return -1;
				}
				sbytes+=wbytes; wlen-=wbytes;
			}
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			return sbytes;
		}
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
	}
	#endif

	return sendto(pport->sd,wbuf,wlen,flags,to,tolen);
}
int WsocksHostImpl::WSHRecvFrom(WSPort *pport, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen)
{
	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	_ASSERT(pport);
	if((pport)&&(!pport->lbaKey.empty()))
	{
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		if(pport->lbaConnReset)
		{
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			WSASetLastError(WSAECONNRESET);
			return -1;
		}
		if(pport->lbaPeer)
		{
			if(!pport->lbaBuffer)
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAENOBUFS);
				return -1;
			}
			if(pport->lbaBuffer->Size<=0)
			{
				#ifdef LBAMUX_CRIT_SECTION
				LeaveCriticalSection(&lbaMutex);
				#else
				ReleaseMutex(lbaMutex);
				#endif
				WSASetLastError(WSAEWOULDBLOCK);
				return -1;
			}
			int wbytes=0;
			while(pport->lbaBuffer->Size>0)
			{
				int rbytes=pport->lbaBuffer->LocalSize;
				if(rbytes>((int)pport->lbaBuffer->BlockSize)*1024)
					rbytes=((int)pport->lbaBuffer->BlockSize)*1024;
				if(rbytes>len)
					rbytes=len;
				memcpy(buf,pport->lbaBuffer->Block,rbytes);
				if(WSStripBuff(pport->lbaBuffer,rbytes)<0)
				{
					#ifdef LBAMUX_CRIT_SECTION
					LeaveCriticalSection(&lbaMutex);
					#else
					ReleaseMutex(lbaMutex);
					#endif
					WSASetLastError(WSAENOBUFS);
					return -1;
				}
				wbytes+=rbytes; buf+=rbytes; len-=rbytes;
			}
			#ifdef LBAMUX_CRIT_SECTION
			LeaveCriticalSection(&lbaMutex);
			#else
			ReleaseMutex(lbaMutex);
			#endif
			return wbytes;
		}
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
	}
	#endif

	return recvfrom(pport->sd,buf,len,flags,from,fromlen);
}
int WsocksHostImpl::WSHClosePort(WSPort *pport)
{
	if(!pport)
		return -1;
	if(pport->sd!=INVALID_SOCKET)
	{
		closesocket(pport->sd); pport->sd=INVALID_SOCKET;
	}
	WSHPortModified(pport->pmod,pport);

	#ifdef WS_LOOPBACK
	// Short-circuit loopback
	if((pport)&&(!pport->lbaKey.empty()))
	{
		#ifdef LBAMUX_CRIT_SECTION
		EnterCriticalSection(&lbaMutex);
		#else
		WaitForSingleObject(lbaMutex,INFINITE);
		#endif
		for(LBSMAP::iterator lit=lbsMap.begin();lit!=lbsMap.end();lit++)
		{
			WSPort *sport=lit->second;
			if(sport==pport)
			{
				lbsMap.erase(lit);
				break;
			}
		}
		for(LBCLIST::iterator cit=lbcList.begin();cit!=lbcList.end();cit++)
		{
			WSPort *cport=*cit;
			if(cport==pport)
			{
				lbcList.erase(cit);
				break;
			}
		}
		if(pport->lbaBuffer)
		{
			WSCloseBuff(pport->lbaBuffer); 
			delete pport->lbaBuffer; pport->lbaBuffer=0;
		}
		if(pport->lbaPeer)
		{
			pport->lbaPeer->lbaPeer=0;
			pport->lbaPeer->lbaConnReset=true;
			pport->lbaPeer=0;
		}
		pport->lbaConnReset=false;
		#ifdef LBAMUX_CRIT_SECTION
		LeaveCriticalSection(&lbaMutex);
		#else
		ReleaseMutex(lbaMutex);
		#endif
	}
	#endif
	return 0;
}
int WsocksHostImpl::WSHGetPeerName(WSPort *pport, SOCKADDR *paddr, int *palen)
{
	if(!pport)
		return -1;
	if(pport->sd!=INVALID_SOCKET)
	{
		return getpeername(pport->sd,paddr,palen);
	}
	return -1;
}
int WsocksHostImpl::WSHGetSockName(WSPort *pport, SOCKADDR *laddr, int *lalen)
{
	if(!pport)
		return -1;
	if(pport->sd!=INVALID_SOCKET)
	{
		return getsockname(pport->sd,laddr,lalen); pport->sd=INVALID_SOCKET;
	}
	return -1;
}

void WsocksHostImpl::WSDateChanging(WsocksApp *pmod)
{
#ifdef WS_FILE_SERVER
	WSHFileTransferClean(pmod);
#endif
#ifdef WS_GUARANTEED
	for(int i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if(!pmod->CgdPort[i].InUse)
			continue;
		if(atoi(pmod->CgdPort[i].GDId.SessionId)==pmod->WSDate)
			continue;
		WSHResetCgdId(pmod,i);
	}
	for(int i=0;i<pmod->NO_OF_UGC_PORTS;i++)
	{
		if(!pmod->UgcPort[i].InUse)
			continue;
		if(atoi(pmod->UgcPort[i].SessionId)==pmod->WSDate)
			continue;
		WSHResetUgcId(pmod,i);
	}
#endif
}
void WsocksHostImpl::WSTimeChanging(WsocksApp *pmod)
{
#ifdef WS_FILE_SERVER
	WSHRecommitCheck(pmod);
#endif

	int i,j,MainAltIP;
	for (i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		if(pmod->ConPort[i].SockConnected)
			pmod->ConPort[i].SinceLastBeatCount+=100;
        pmod->ConPort[i].LastChokeSize=0;
	}
	for(i=0;i<pmod->NO_OF_USR_PORTS;i++)
	{
		if(pmod->UsrPort[i].TimeTillClose>0)
		{
			if((--pmod->UsrPort[i].TimeTillClose)<=0)
			{
				pmod->UsrPort[i].TimeTillClose=0;
				pmod->UsrPort[i].SockActive=1;
				WSHCloseUsrPort(pmod,i);
			}
		}
	}
#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	{
		if(pmod->UmrPort[i].TimeTillClose>0)
		{
			if((--pmod->UmrPort[i].TimeTillClose)<=0)
			{
				pmod->UmrPort[i].TimeTillClose=0;
				pmod->UmrPort[i].SockActive=1;
				WSHCloseUmrPort(pmod,i);
			}
		}
	}
#endif
#ifdef WS_GUARANTEED
	for (i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if(pmod->CgdPort[i].SockConnected)
			pmod->CgdPort[i].SinceLastBeatCount+=100;
        pmod->CgdPort[i].LastChokeSize=0;
	}
	for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if(pmod->UgrPort[i].TimeTillClose>0)
		{
			if((--pmod->UgrPort[i].TimeTillClose)<=0)
			{
				pmod->UgrPort[i].TimeTillClose=0;
				pmod->UgrPort[i].SockActive=1;
				WSHCloseUgrPort(pmod,i);
			}
		}
	}
#endif //#ifdef WS_GUARANTEED
#ifndef WS_NO_RESET
	if(WSHTime/100==600)
	{
		for (i=0;i<pmod->NO_OF_CON_PORTS;i++)
		{
			MainAltIP=0;
			for(j=0;j<pmod->ConPort[i].AltIPCount;j++)
			{
				if(pmod->ConPort[i].AltRemoteIPOn[j])
				{
					MainAltIP=j;
					break;
				}
			}
			if(pmod->ConPort[i].CurrentAltIP!=MainAltIP)
			{
				pmod->ConPort[i].CurrentAltIP=MainAltIP-1;
				if(pmod->ConPort[i].SockOpen)
				{
					WSHCloseConPort(pmod,i);
				}
			}
		}
#ifdef WS_CAST
		for (i=0;i<pmod->NO_OF_CTI_PORTS;i++)
		{
			MainAltIP=0;
			for(j=0;j<pmod->CtiPort[i].AltIPCount;j++)
			{
				if(pmod->CtiPort[i].AltRemoteIPOn[j])
				{
					MainAltIP=j;
					break;
				}
			}
			if(pmod->CtiPort[i].CurrentAltIP!=MainAltIP)
			{
				pmod->CtiPort[i].CurrentAltIP=MainAltIP-1;
				if(pmod->CtiPort[i].SockActive)
				{
					WSHCloseCtiPort(pmod,i);
				}
			}
		}
#endif //#ifdef WS_CAST
	}
#endif
#ifdef WS_FILE_SERVER
	for (i=0;i<pmod->NO_OF_FILE_PORTS;i++)
	{
		if(pmod->FilePort[i].SockConnected)
		{
			char text[128]={0};
			MSGHEADER MsgHeader;
			MsgHeader.MsgID = 16;	
			MsgHeader.MsgLen = 128;
			WSWriteBuff(&pmod->FilePort[i].OutBuffer,(char *)&MsgHeader,sizeof(MsgHeader));
			WSWriteBuff(&pmod->FilePort[i].OutBuffer,text,sizeof(text));
			pmod->FilePort[i].BeatsOut++;
			pmod->FilePort[i].LastChokeSize=0;
		}
	}
#endif //#ifdef WS_FILE_SERVER
//#ifdef WS_MONITOR
//	WSHSendMonitorData(pmod,-1);
//#endif
}

#include <direct.h>
void WsocksHostImpl::WSHCheckSystemResourcesAvailable()
{
#ifndef UNIX
	ZeroMemory (&WSMemStat, sizeof (MEMORYSTATUS));
   
	WSMemStat.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus (&WSMemStat);

	if (WSMemPersUsedWarnLevel && WSMemStat.dwMemoryLoad > WSMemPersUsedWarnLevel)
	{
		WSHostLogError("!CRASH: MEMORY LOW: Percentage used is %d%%",WSMemStat.dwMemoryLoad);
		// Ops wants these errors in the app's log so they get them on TheEye
		for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
		{
			WsocksApp *pmod=ait->second;
			if((pmod->pcfg->criticalApp)&&(!pmod->pcfg->loopback))
				WSHLogError(pmod,"!CRASH: MEMORY LOW: Percentage used is %d%%",WSMemStat.dwMemoryLoad);
		}
	}

	WSMemStat.dwAvailPhys /= 1024;

	if (WSMemAvailWarnLevel && WSMemStat.dwAvailPhys < WSMemAvailWarnLevel)
	{
		WSHostLogError("!CRASH: MEMORY LOW: Available is %dKb",WSMemStat.dwAvailPhys);
		for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
		{
			WsocksApp *pmod=ait->second;
			if((pmod->pcfg->criticalApp)&&(!pmod->pcfg->loopback))
				WSHLogError(pmod,"!CRASH: MEMORY LOW: Available is %dKb",WSMemStat.dwAvailPhys);
		}
	}

	if((!pGetProcessMemoryInfo)&&(thisProc==(HANDLE)-1))
	{
		thisProc=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,GetCurrentProcessId());
		psapi=LoadLibrary("psapi.dll");
		if(psapi)
			pGetProcessMemoryInfo=
				(BOOL(WINAPI *)(HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD))
				GetProcAddress(psapi,"GetProcessMemoryInfo");
	}
	if(pGetProcessMemoryInfo)
	{
		memset(&WSProcessMemoryCounters,0,sizeof(PROCESS_MEMORY_COUNTERS));
		WSProcessMemoryCounters.cb=sizeof(PROCESS_MEMORY_COUNTERS);
		pGetProcessMemoryInfo(thisProc,&WSProcessMemoryCounters,sizeof(PROCESS_MEMORY_COUNTERS));
		__int64 totmem=WSProcessMemoryCounters.WorkingSetSize/1024;
		if (WSMemAvailWarnLevel && totmem >= 1.5*1024*1024)
		{
			WSHostLogError("!CRASH: PROCESS MEMORY HIGH: Used %dKb",totmem);
			for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
			{
				WsocksApp *pmod=ait->second;
				if((pmod->pcfg->criticalApp)&&(!pmod->pcfg->loopback))
					WSHLogError(pmod,"!CRASH: PROCESS MEMORY HIGH: Used %dKb",totmem);
			}
		}
	}

	ULARGE_INTEGER i64FreeBytesToCaller;
	ULARGE_INTEGER i64TotalBytes;
	ULARGE_INTEGER i64FreeBytes;

	char dir[1001];
	_getcwd(dir, 1000);

	GetDiskFreeSpaceEx(dir, &i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes);

	WSDiskSpaceAvailable = (unsigned long)(i64FreeBytesToCaller.QuadPart / 1024);
	WSDiskSpaceAvailable /= 1024;

	long tlastErr=0;
	// Under 1GB, warn first time and on every quarter hour after
	if(WSDiskSpaceAvailable<=1024)
	{
		if((!tlastErr)||(WSHTime%1500==0))
		{
			WSHostLogError("!CRASH: DISK SPACE RUNNING LOW at %dM!!!!!!!!!!!!!",WSDiskSpaceAvailable);
			for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
			{
				WsocksApp *pmod=ait->second;
				if((pmod->pcfg->criticalApp)&&(!pmod->pcfg->loopback))
					WSHLogError(pmod,"!CRASH: DISK SPACE RUNNING LOW at %dM!!!!!!!!!!!!!",WSDiskSpaceAvailable);
			}
			tlastErr=WSHTime;
		}
	}
	// Under 3GB, warn first time and on every hour after
	else if(WSDiskSpaceAvailable<=3*1024)
	{
		if((!tlastErr)||(WSHTime%10000==0))
		{
			WSHostLogError("!CRASH: DISK SPACE RUNNING LOW at %dM!!!!!!!!!!!!!",WSDiskSpaceAvailable);
			for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
			{
				WsocksApp *pmod=ait->second;
				if((pmod->pcfg->criticalApp)&&(!pmod->pcfg->loopback))
					WSHLogError(pmod,"!CRASH: DISK SPACE RUNNING LOW at %dM!!!!!!!!!!!!!",WSDiskSpaceAvailable);
			}
			tlastErr=WSHTime;
		}
	}
	else
		tlastErr=0;
#endif
}
void WsocksHostImpl::WSHGetSystemResourcesAvailable(PROCESS_MEMORY_COUNTERS *pProcessMemoryCounters)
{
	memcpy(pProcessMemoryCounters,&WSProcessMemoryCounters,sizeof(PROCESS_MEMORY_COUNTERS));
}

//int WsocksHostImpl::WSHCheckDST(WsocksApp *pmod)
//{
//	// Old DST 20070401 02:59:59
//	struct tm odstBefore;
//    odstBefore.tm_sec=59;     /* seconds after the minute - [0,59] */
//    odstBefore.tm_min=59;     /* minutes after the hour - [0,59] */
//    odstBefore.tm_hour=2;    /* hours since midnight - [0,23] */
//    odstBefore.tm_mday=1;    /* day of the month - [1,31] */
//    odstBefore.tm_mon=3;     /* months since January - [0,11] */
//    odstBefore.tm_year=2007 -1900;    /* years since 1900 */
//    odstBefore.tm_wday=0;    /* days since Sunday - [0,6] */
//    odstBefore.tm_yday=0;    /* days since January 1 - [0,365] */
//    odstBefore.tm_isdst=-1;   /* daylight savings time flag */
//	time_t ottBefore=mktime(&odstBefore);
//
//	// 20070401 03:00:00
//	time_t ottBegin=ottBefore +1;
//
//	// 20071028 02:59:59
//	struct tm odstEnd;
//    odstEnd.tm_sec=59;     /* seconds after the minute - [0,59] */
//    odstEnd.tm_min=59;     /* minutes after the hour - [0,59] */
//    odstEnd.tm_hour=0;    /* hours since midnight - [0,23] */
//    odstEnd.tm_mday=28;    /* day of the month - [1,31] */
//    odstEnd.tm_mon=9;     /* months since January - [0,11] */
//    odstEnd.tm_year=2007 -1900;    /* years since 1900 */
//    odstEnd.tm_wday=0;    /* days since Sunday - [0,6] */
//    odstEnd.tm_yday=0;    /* days since January 1 - [0,365] */
//    odstEnd.tm_isdst=-1;   /* daylight savings time flag */
//	time_t ottEnd=mktime(&odstEnd);
//	ottEnd+=3600;
//
//	// 20071028 03:00:00
//	time_t ottAfter=ottEnd +1;
//
//	// New DST 20070311 02:59:59
//	struct tm ndstBefore;
//    ndstBefore.tm_sec=59;     /* seconds after the minute - [0,59] */
//    ndstBefore.tm_min=59;     /* minutes after the hour - [0,59] */
//    ndstBefore.tm_hour=2;    /* hours since midnight - [0,23] */
//    ndstBefore.tm_mday=11;    /* day of the month - [1,31] */
//    ndstBefore.tm_mon=2;     /* months since January - [0,11] */
//    ndstBefore.tm_year=2007 -1900;    /* years since 1900 */
//    ndstBefore.tm_wday=0;    /* days since Sunday - [0,6] */
//    ndstBefore.tm_yday=0;    /* days since January 1 - [0,365] */
//    ndstBefore.tm_isdst=-1;   /* daylight savings time flag */
//	time_t nttBefore=mktime(&ndstBefore);
//
//	// 20070311 03:00:00
//	time_t nttBegin=nttBefore +1;
//
//	// 20071104 02:59:59
//	struct tm ndstEnd; // inclusive
//    ndstEnd.tm_sec=59;     /* seconds after the minute - [0,59] */
//    ndstEnd.tm_min=59;     /* minutes after the hour - [0,59] */
//    ndstEnd.tm_hour=0;    /* hours since midnight - [0,23] */
//    ndstEnd.tm_mday=4;    /* day of the month - [1,31] */
//    ndstEnd.tm_mon=10;     /* months since January - [0,11] */
//    ndstEnd.tm_year=2007 -1900;    /* years since 1900 */
//    ndstEnd.tm_wday=0;    /* days since Sunday - [0,6] */
//    ndstEnd.tm_yday=0;    /* days since January 1 - [0,365] */
//    ndstEnd.tm_isdst=-1;   /* daylight savings time flag */
//	time_t nttEnd=mktime(&ndstEnd);
//	nttEnd+=3600;
//
//	// 20071104 03:00:00
//	time_t nttAfter=nttEnd +1;
//
//	// Old DST 20070331 18:00:00
//	struct tm odstPre6;
//    odstPre6.tm_sec=0;     /* seconds after the minute - [0,59] */
//    odstPre6.tm_min=0;     /* minutes after the hour - [0,59] */
//    odstPre6.tm_hour=18;    /* hours since midnight - [0,23] */
//    odstPre6.tm_mday=31;    /* day of the month - [1,31] */
//    odstPre6.tm_mon=2;     /* months since January - [0,11] */
//    odstPre6.tm_year=2007 -1900;    /* years since 1900 */
//    odstPre6.tm_wday=0;    /* days since Sunday - [0,6] */
//    odstPre6.tm_yday=0;    /* days since January 1 - [0,365] */
//    odstPre6.tm_isdst=-1;   /* daylight savings time flag */
//	time_t ottPre6=mktime(&odstPre6);
//
//	// Old DST 20070401 06:00:00
//	struct tm odstPost6;
//    odstPost6.tm_sec=0;     /* seconds after the minute - [0,59] */
//    odstPost6.tm_min=0;     /* minutes after the hour - [0,59] */
//    odstPost6.tm_hour=6;    /* hours since midnight - [0,23] */
//    odstPost6.tm_mday=1;    /* day of the month - [1,31] */
//    odstPost6.tm_mon=3;     /* months since January - [0,11] */
//    odstPost6.tm_year=2007 -1900;    /* years since 1900 */
//    odstPost6.tm_wday=0;    /* days since Sunday - [0,6] */
//    odstPost6.tm_yday=0;    /* days since January 1 - [0,365] */
//    odstPost6.tm_isdst=-1;   /* daylight savings time flag */
//	time_t ottPost6=mktime(&odstPost6);
//
//	// New DST 20070310 18:00:00
//	struct tm ndstPre6;
//    ndstPre6.tm_sec=0;     /* seconds after the minute - [0,59] */
//    ndstPre6.tm_min=0;     /* minutes after the hour - [0,59] */
//    ndstPre6.tm_hour=18;    /* hours since midnight - [0,23] */
//    ndstPre6.tm_mday=10;    /* day of the month - [1,31] */
//    ndstPre6.tm_mon=2;     /* months since January - [0,11] */
//    ndstPre6.tm_year=2007 -1900;    /* years since 1900 */
//    ndstPre6.tm_wday=0;    /* days since Sunday - [0,6] */
//    ndstPre6.tm_yday=0;    /* days since January 1 - [0,365] */
//    ndstPre6.tm_isdst=-1;   /* daylight savings time flag */
//	time_t nttPre6=mktime(&ndstPre6);
//
//	// New DST 20070311 06:00:00
//	struct tm ndstPost6;
//    ndstPost6.tm_sec=0;     /* seconds after the minute - [0,59] */
//    ndstPost6.tm_min=0;     /* minutes after the hour - [0,59] */
//    ndstPost6.tm_hour=6;    /* hours since midnight - [0,23] */
//    ndstPost6.tm_mday=11;    /* day of the month - [1,31] */
//    ndstPost6.tm_mon=2;     /* months since January - [0,11] */
//    ndstPost6.tm_year=2007 -1900;    /* years since 1900 */
//    ndstPost6.tm_wday=0;    /* days since Sunday - [0,6] */
//    ndstPost6.tm_yday=0;    /* days since January 1 - [0,365] */
//    ndstPost6.tm_isdst=-1;   /* daylight savings time flag */
//	time_t nttPost6=mktime(&ndstPost6);
//
//	// Old DST 20071027 20:00:00
//	struct tm odstPre8;
//    odstPre8.tm_sec=0;     /* seconds after the minute - [0,59] */
//    odstPre8.tm_min=0;     /* minutes after the hour - [0,59] */
//    odstPre8.tm_hour=20;    /* hours since midnight - [0,23] */
//    odstPre8.tm_mday=27;    /* day of the month - [1,31] */
//    odstPre8.tm_mon=9;     /* months since January - [0,11] */
//    odstPre8.tm_year=2007 -1900;    /* years since 1900 */
//    odstPre8.tm_wday=0;    /* days since Sunday - [0,6] */
//    odstPre8.tm_yday=0;    /* days since January 1 - [0,365] */
//    odstPre8.tm_isdst=-1;   /* daylight savings time flag */
//	time_t ottPre8=mktime(&odstPre8);
//
//	// Old DST 20071028 08:00:00
//	struct tm odstPost8;
//    odstPost8.tm_sec=0;     /* seconds after the minute - [0,59] */
//    odstPost8.tm_min=0;     /* minutes after the hour - [0,59] */
//    odstPost8.tm_hour=8;    /* hours since midnight - [0,23] */
//    odstPost8.tm_mday=28;    /* day of the month - [1,31] */
//    odstPost8.tm_mon=9;     /* months since January - [0,11] */
//    odstPost8.tm_year=2007 -1900;    /* years since 1900 */
//    odstPost8.tm_wday=0;    /* days since Sunday - [0,6] */
//    odstPost8.tm_yday=0;    /* days since January 1 - [0,365] */
//    odstPost8.tm_isdst=-1;   /* daylight savings time flag */
//	time_t ottPost8=mktime(&odstPost8);
//
//	// New DST 20071103 20:00:00
//	struct tm ndstPre8;
//    ndstPre8.tm_sec=0;     /* seconds after the minute - [0,59] */
//    ndstPre8.tm_min=0;     /* minutes after the hour - [0,59] */
//    ndstPre8.tm_hour=20;    /* hours since midnight - [0,23] */
//    ndstPre8.tm_mday=3;    /* day of the month - [1,31] */
//    ndstPre8.tm_mon=10;     /* months since January - [0,11] */
//    ndstPre8.tm_year=2007 -1900;    /* years since 1900 */
//    ndstPre8.tm_wday=0;    /* days since Sunday - [0,6] */
//    ndstPre8.tm_yday=0;    /* days since January 1 - [0,365] */
//    ndstPre8.tm_isdst=-1;   /* daylight savings time flag */
//	time_t nttPre8=mktime(&ndstPre8);
//
//	// New DST 20071104 08:00:00
//	struct tm ndstPost8;
//    ndstPost8.tm_sec=0;     /* seconds after the minute - [0,59] */
//    ndstPost8.tm_min=0;     /* minutes after the hour - [0,59] */
//    ndstPost8.tm_hour=8;    /* hours since midnight - [0,23] */
//    ndstPost8.tm_mday=4;    /* day of the month - [1,31] */
//    ndstPost8.tm_mon=10;     /* months since January - [0,11] */
//    ndstPost8.tm_year=2007 -1900;    /* years since 1900 */
//    ndstPost8.tm_wday=0;    /* days since Sunday - [0,6] */
//    ndstPost8.tm_yday=0;    /* days since January 1 - [0,365] */
//    ndstPost8.tm_isdst=-1;   /* daylight savings time flag */
//	time_t nttPost8=mktime(&ndstPost8);
//
//	// Check tm_isdst flags from ttime values
//	tm oltmBefore,oltmBegin,oltmEnd,oltmAfter;
//	oltmBefore=*localtime(&ottBefore);
//	oltmBegin=*localtime(&ottBegin);
//	oltmEnd=*localtime(&ottEnd);
//	oltmAfter=*localtime(&ottAfter);
//
//	tm nltmBefore,nltmBegin,nltmEnd,nltmAfter;
//	nltmBefore=*localtime(&nttBefore);
//	nltmBegin=*localtime(&nttBegin);
//	nltmEnd=*localtime(&nttEnd);
//	nltmAfter=*localtime(&nttAfter);
//
//	DWORD ottDiff626=ottPost6 -ottPre6;
//	DWORD nttDiff626=nttPost6 -nttPre6;
//	DWORD ottDiff828=ottPost8 -ottPre8;
//	DWORD nttDiff828=nttPost8 -nttPre8;
//
//	int oldDstCnt=0,newDstCnt=0;
//	if(!oltmBegin.tm_isdst)
//		WSHostLogError("oltmBegin.tm_isdst not set!");
//	if(!oltmEnd.tm_isdst)
//		WSHostLogError("oltmEnd.tm_isdst not set!");
//	if(oltmBefore.tm_isdst)
//		newDstCnt++;
//	else
//		oldDstCnt++;
//	if(oltmAfter.tm_isdst)
//		newDstCnt++;
//	else
//		oldDstCnt++;
//
//	if(nltmBegin.tm_isdst)
//		newDstCnt++;
//	else
//		oldDstCnt++;
//	if(nltmEnd.tm_isdst)
//		newDstCnt++;
//	else
//		oldDstCnt++;
//	if(nltmBefore.tm_isdst)
//		WSHostLogError("nltmBefore.tm_isdst is set!");
//	if(nltmAfter.tm_isdst)
//		WSHostLogError("nltmAfter.tm_isdst is set!");
//
//	// All tests should indicate old or new DST entirely
//	if((oldDstCnt!=0)&&(newDstCnt!=0))
//		WSHostLogError("OldDst: %d, NewDst: %d",oldDstCnt,newDstCnt);	
//	if(oldDstCnt>newDstCnt)
//	{
//		WSHostLogError("Old DST localtime!");
//		if(ottDiff626!=11*3600)
//			WSHostLogError("OldDST: ottDiff626=%d, expected %d",ottDiff626,11*3600);
//		if(ottDiff828!=13*3600)
//			WSHostLogError("OldDST: ottDiff828=%d, expected %d",ottDiff828,13*3600);
//		return -1;
//	}
//	else
//	{
//		if(WSDate/10000==2007)
//		{
//			WSHostLogEvent("New DST localtime.");
//			if(nttDiff626!=11*3600)
//				WSHostLogError("NewDST: nttDiff626=%d, expected %d",nttDiff626,11*3600);
//			if(nttDiff828!=13*3600)
//				WSHostLogError("NewDST: nttDiff828=%d, expected %d",nttDiff828,13*3600);
//		}
//		dstVerified=true;
//	}
//	return 0;
//}

void WsocksHostImpl::WSHGetTime(char *cTime)
{
#ifdef UNIX
	timeb Timeb;
	tm *TM;

	ftime(&Timeb);
	TM=localtime(&(Timeb.time));
	sprintf(cTime,"%02d:%02d:%02d",TM->tm_hour,TM->tm_min,TM->tm_sec);
#else
	GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_FORCE24HOURFORMAT
		,NULL,"HH:mm:ss",cTime,9);
#endif
}
void WsocksHostImpl::WSHGetDate_Time(char *cDate,char *cTime,int *wday)
{
#ifdef UNIX
	timeb Timeb;
	tm *TM;

	ftime(&Timeb);
	TM=localtime(&(Timeb.time));
	sprintf(cDate,"%04d%02d%02d",(1900+TM->tm_year),(TM->tm_mon+1),TM->tm_mday);
	sprintf(cTime,"%02d%02d%02d",TM->tm_hour,TM->tm_min,TM->tm_sec);
    *wday=TM->tm_wday;
#else
    SYSTEMTIME lsys;
    GetLocalTime(&lsys);
	GetDateFormat(LOCALE_SYSTEM_DEFAULT,NULL
			,&lsys,"yyyyMMdd",cDate,9);
	GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_FORCE24HOURFORMAT
		,&lsys,"HHmmss",cTime,7);
    *wday=lsys.wDayOfWeek;
#endif

#ifdef WS_FORCEDATE
	if (WSForceDate && *WSForceDate)
	{
		strcpy(cDate, WSForceDate);
	}
#endif
}
int WsocksHostImpl::WSHDateChange()
{
	for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
	{
		WsocksApp *pmod=ait->second;
		pmod->threadSignal|=0x02;
	}
	return 0;
}
int WsocksHostImpl::WSHTimeChange()
{
	for(WSAPPMAP::iterator ait=appMap.begin();ait!=appMap.end();ait++)
	{
		WsocksApp *pmod=ait->second;
		pmod->threadSignal|=0x01;
	}
	return 0;
}
void WsocksHostImpl::WSHCheckTime(void)
{
	long lTempDate;
	long lTempTime;
//	int i;

#ifndef WS_NO_RESET
//	int j;
//	int MainAltIP;
#endif

	int DoTimeChange=false;

	WSHGetDate_Time(WSHcDate,WSHcTime,&WSHDayOfWeek);
	WSHlTime = (WSHcTime[0]-'0') * 10*3600 + 
              (WSHcTime[1]-'0') * 3600 + 
              (WSHcTime[2]-'0') * 10*60 + 
              (WSHcTime[3]-'0') * 60 + 
              (WSHcTime[4]-'0') * 10 +
              (WSHcTime[5]-'0');

	lTempDate=WSHDate;
	WSHDate=atol(WSHcDate);

	lTempTime=WSHTime;
	WSHTime=atol(WSHcTime);

	if(lTempDate!=WSHDate)
	{
#ifdef WS_FILE_SERVER
//        WSHFileTransferClean();
#endif
		//dstVerified=false;
		WSHDateChange();
		DoTimeChange=true;
	}
	else if(lTempTime!=WSHTime)
		DoTimeChange=true;

	if(DoTimeChange)
	{
#ifndef UNIX
#ifndef WS_DISPOFF
		{
			int t;

			t  =WSHTime / 100;
			
			if (t != LastTimeWeCheckedDiskSpace)
			{
				WSHCheckSystemResourcesAvailable();	// so we can scream to the eye			
			}
			LastTimeWeCheckedDiskSpace = t;
 
		}
#endif
#endif

#ifdef KMA_DEBUG_204
		static ULONGLONG LastTimeIn;
		ULONGLONG ullTimeStart=WSGetTimerCount();
#endif
		//if(!dstVerified)
		//{
		//	// Every 5 minutes
		//	if(WSHTime%500==0)
		//		WSHCheckDST();
		//}
		WSHTimeChange();
#ifdef WS_FILE_SERVER
//		WSRecommitCheck();
#endif
#if defined(WS_MONITOR)&&defined(WS_ORDERBOOK)
		WSBookPublish();
#endif

#ifdef KMA_DEBUG_204
		if (ullTimeStart)
		{
			ULONGLONG StopTime=ullTimeStart+(WSPerformanceFreq.QuadPart*16/1000);
			ULONGLONG adder=(WSPerformanceFreq.QuadPart*16/1000);
			WSHostLogEvent(" In:  %I64d    Stop: %I64d   : Perf Fre = %I64d   Adder: %I64d", ullTimeStart,  StopTime, WSPerformanceFreq.QuadPart, adder);
		}
#endif
	}
}

int WsocksHostImpl::WSHWaitMutex(int mutexIdx, int timeout)
{
	if((mutexIdx<1)||(mutexIdx>8))
		return -1;
#ifdef MUX_CRIT_SECTION
	EnterCriticalSection(&gmutexes[mutexIdx-1]);
	return WAIT_OBJECT_0;
#else
	return WaitForSingleObject(gmutexes[mutexIdx-1],timeout);
#endif
}
int WsocksHostImpl::WSHReleaseMutex(int mutexIdx)
{
	if((mutexIdx<1)||(mutexIdx>8))
		return -1;
#ifdef MUX_CRIT_SECTION
	LeaveCriticalSection(&gmutexes[mutexIdx-1]);
	return 0;
#else
	return ReleaseMutex(gmutexes[mutexIdx-1]);
#endif
}
const char *WsocksHostImpl::WSHSizeStr(WsocksApp *pmod, __int64 bigsize, bool byteUnits)
{
	__int64 kil = byteUnits ?1024 :1000;
    __int64 meg = kil *kil;
    __int64 gig = meg *kil;
    //static __thread int zidx = -1;
    char *zstrs[8]={pmod->SizeStr_zstrs[0],pmod->SizeStr_zstrs[1],pmod->SizeStr_zstrs[2],pmod->SizeStr_zstrs[3],pmod->SizeStr_zstrs[4],pmod->SizeStr_zstrs[5],pmod->SizeStr_zstrs[6],pmod->SizeStr_zstrs[7]};
    int& zidx=pmod->SizeStr_zidx;
    zidx = (zidx +1) %8;
    char *zstr = zstrs[zidx];
    memset(zstr, 0, sizeof(zstr));
    if ( bigsize > gig )
    {
        double frem = (double)(bigsize %gig);
        frem /= (double)gig;
        int irem = (int)(frem *10);
        sprintf(zstr, "%d.%dG", (int)(bigsize /gig), irem);
    }
    else if ( bigsize >= meg )
    {
        double frem = (double)(bigsize %meg);
        frem /= (double)meg;
        int irem = (int)(frem *10);
        sprintf(zstr, "%d.%dM", (int)(bigsize /meg), irem);
    }
    else if ( bigsize >= kil )
    {
        double frem = (double)(bigsize %kil);
        frem /= (double)kil;
        int irem = (int)(frem *10);
        sprintf(zstr, "%d.%dK", (int)(bigsize /kil), irem);
    }
    else
        sprintf(zstr, "%d", bigsize);
    return zstr;
}
long WsocksHostImpl::WSHDiffTime(long tnew, long told)
{
	int nhh=tnew/10000; tnew%=10000;
	int nmm=tnew/100; tnew%=100;
	int nss=tnew;
	long nts=(nhh*3600)+(nmm*60)+(nss);
	int ohh=told/10000; told%=10000;
	int omm=told/100; told%=100;
	int oss=told;
	long ots=(ohh*3600)+(omm*60)+(oss);
	return nts -ots;
}
ULONGLONG WsocksHostImpl::WSHGetTimerCount(WsocksApp *pmod)
{
#ifdef WS_MIPS
#define WS_MIPSTime( ullTime ) {_asm RDTSC _asm mov DWORD PTR ullTime[0], eax _asm mov DWORD PTR ullTime[4], edx}
	ULONGLONG l; 
	WS_MIPSTime(l); 
	return l; 
#elif defined(WS_WINMM)
	return timeGetTime();
#else
	return GetTickCount();
#endif
}
DWORD WsocksHostImpl::WSHDiffTimerCount(WsocksApp *pmod, ULONGLONG t1, ULONGLONG t2)
{
#ifdef WS_MIPS
	return WSHMipsDiff(pmod,t1,t2);
#elif defined(WS_WINMM)
	return (DWORD)(t2 -t1);
#else
	return (DWORD)(t2 -t1);
#endif
}
#ifdef WS_MIPS
void WsocksHostImpl::WSHCalcMips(WsocksApp *pmod)
{
	ULONGLONG StartMIPS=0;
	ULONGLONG EndMIPS;
	ULONG StartTick=GetTickCount();
	ULONG EndTick;
	while(true)
	{
		if(!StartMIPS)
		{
			if(StartTick==GetTickCount())
				continue;
			StartTick=GetTickCount();
			StartMIPS=WSHGetTimerCount(pmod);
		}
		else
		{	
			EndTick=GetTickCount();
			if(WS_TIMER_INTERVAL>0)
			{
				if(EndTick<StartTick+WS_TIMER_INTERVAL)// sample over one tick
					continue;
				EndMIPS=WSHGetTimerCount(pmod);
				pmod->WSMaxIdleMips=(EndMIPS-StartMIPS);// idle up to one tick
			}
			else
			{
				if(EndTick<StartTick+1000)// sample over full second
					continue;
				EndMIPS=WSHGetTimerCount(pmod);
				pmod->WSMaxIdleMips=(EndMIPS-StartMIPS)*15/100; // 15ms idle
			}
			pmod->WSMips=pmod->WSMaxIdleMips*1000/(EndTick-StartTick);
			break;
		}
	}
	QueryPerformanceFrequency( (LARGE_INTEGER*)&pmod->WSPerformanceFreq);
//ULONGLONG chk1=WSHGetTimerCount(pmod);
//ULONGLONG chk2=WSHGetTimerCount(pmod);
//printf("DEBUG [%I64d,%I64d)=%I64d=>%d us\n",chk1,chk2,chk2-chk1,WSHMipsDiff(pmod,chk1,chk2));
}
DWORD WsocksHostImpl::WSHMipsDiff(WsocksApp *pmod, ULONGLONG start, ULONGLONG stop)
{
	double div=(double)(LONGLONG)(stop -start)/(double)(LONGLONG)pmod->WSMips; 
	//double div=(double)(LONGLONG)(stop -start)/(double)(LONGLONG)WSPerformanceFreq.QuadPart; 
	return (DWORD)(div*1000000); 
}
#endif//WS_MIPS

#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::AllocOverlapBlock()
{
	OVERLAPBLOCK *pblock=new OVERLAPBLOCK;
	if(!pblock)
		return -1;
	memset(pblock,0,sizeof(OVERLAPBLOCK));

	#ifdef OVLMUX_CRIT_SECTION
	EnterCriticalSection(&ovlMutex);
	#else
	WaitForSingleObject(ovlMutex,INFINITE);
	#endif
	pblock->next=ovlBlocks;
	if(ovlBlocks) ovlBlocks->prev=pblock;
	ovlBlocks=pblock;

	for(int i=0;i<128;i++)
	{
		WSOVERLAPPED *povl=&pblock->ovls[i];
		povl->next=freeOvlList;
		if(freeOvlList) freeOvlList->prev=povl;
		freeOvlList=povl;
	}
	#ifdef OVLMUX_CRIT_SECTION
	LeaveCriticalSection(&ovlMutex);
	#else
	ReleaseMutex(ovlMutex);
	#endif
	return 0;
}
int WsocksHostImpl::InitOverlap()
{
	#ifdef OVLMUX_CRIT_SECTION
	EnterCriticalSection(&ovlMutex);
	#else
	WaitForSingleObject(ovlMutex,INFINITE);
	#endif
	if(AllocOverlapBlock()<0)
		return -1;
	#ifdef OVLMUX_CRIT_SECTION
	LeaveCriticalSection(&ovlMutex);
	#else
	ReleaseMutex(ovlMutex);
	#endif
	return 0;
}
int WsocksHostImpl::CleanupOverlap()
{
	#ifdef OVLMUX_CRIT_SECTION
	EnterCriticalSection(&ovlMutex);
	#else
	WaitForSingleObject(ovlMutex,INFINITE);
	#endif
	freeOvlList=0;
	while(ovlBlocks)
	{
		OVERLAPBLOCK *pblock=ovlBlocks;
		ovlBlocks=ovlBlocks->next;
		if(ovlBlocks) ovlBlocks->prev=0;
		// Last chance to clean up abandoned operations
		for(int i=0;i<128;i++)
		{
			WSOVERLAPPED *povl=&pblock->ovls[i];
			if(povl->buf)
			{
				delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			}
		}
		delete pblock;
	}
	#ifdef OVLMUX_CRIT_SECTION
	LeaveCriticalSection(&ovlMutex);
	#else
	ReleaseMutex(ovlMutex);
	#endif
	return 0;
}
//WSOVERLAPPED *WsocksHostImpl::AllocOverlap(WSOVERLAPPED **ppendOvlList)
//{
//	// Take from global free list
//	#ifdef OVLMUX_CRIT_SECTION
//	EnterCriticalSection(&ovlMutex);
//	#else
//	WaitForSingleObject(ovlMutex,INFINITE);
//	#endif
//	if(!freeOvlList)
//	{
//		if(AllocOverlapBlock()<0)
//		{
//			#ifdef OVLMUX_CRIT_SECTION
//			LeaveCriticalSection(&ovlMutex);
//			#else
//			ReleaseMutex(ovlMutex);
//			#endif
//			return 0;
//		}
//	}
//	WSOVERLAPPED *povl=freeOvlList;
//	freeOvlList=freeOvlList->next;
//	if(freeOvlList) freeOvlList->prev=0;
//	povl->next=0;
//
//	// Put on port pending list
//	if(ppendOvlList)
//	{
//		povl->next=*ppendOvlList;
//		if(*ppendOvlList) (*ppendOvlList)->prev=povl;
//		*ppendOvlList=povl;
//	}
//	#ifdef OVLMUX_CRIT_SECTION
//	LeaveCriticalSection(&ovlMutex);
//	#else
//	ReleaseMutex(ovlMutex);
//	#endif
//	return povl;
//}
WSOVERLAPPED *WsocksHostImpl::AllocOverlap(WSOVERLAPPED **ppendOvlBeg, WSOVERLAPPED **ppendOvlEnd)
{
	// Take from global free list
	_ASSERT((ppendOvlBeg)&&(ppendOvlEnd));
	#ifdef OVLMUX_CRIT_SECTION
	EnterCriticalSection(&ovlMutex);
	#else
	WaitForSingleObject(ovlMutex,INFINITE);
	#endif
	if(!freeOvlList)
	{
		if(AllocOverlapBlock()<0)
		{
			#ifdef OVLMUX_CRIT_SECTION
			LeaveCriticalSection(&ovlMutex);
			#else
			ReleaseMutex(ovlMutex);
			#endif
			return 0;
		}
	}
	WSOVERLAPPED *povl=freeOvlList;
	freeOvlList=freeOvlList->next;
	if(freeOvlList) freeOvlList->prev=0;
	povl->next=0;

	// Append to end of port pending list
	if(*ppendOvlEnd)
	{
		povl->prev=*ppendOvlEnd; (*ppendOvlEnd)->next=povl;
		*ppendOvlEnd=povl;
	}
	else
		*ppendOvlBeg=*ppendOvlEnd=povl;
	#ifdef OVLMUX_CRIT_SECTION
	LeaveCriticalSection(&ovlMutex);
	#else
	ReleaseMutex(ovlMutex);
	#endif
	return povl;
}
//int WsocksHostImpl::FreeOverlap(WSOVERLAPPED **ppendOvlList, WSOVERLAPPED *povl)
//{
//	// Remove from port list
//	#ifdef OVLMUX_CRIT_SECTION
//	EnterCriticalSection(&ovlMutex);
//	#else
//	WaitForSingleObject(ovlMutex,INFINITE);
//	#endif
//	_ASSERT(ppendOvlList);
//	if((ppendOvlList)&&(*ppendOvlList==povl))
//		*ppendOvlList=povl->next;
//	if(povl->prev) povl->prev->next=povl->next;
//	if(povl->next) povl->next->prev=povl->prev;
//
//	// Put back on global free list
//	povl->prev=0; povl->next=freeOvlList;
//	if(freeOvlList) freeOvlList->prev=povl;
//	freeOvlList=povl;
//	#ifdef OVLMUX_CRIT_SECTION
//	LeaveCriticalSection(&ovlMutex);
//	#else
//	ReleaseMutex(ovlMutex);
//	#endif
//	return 0;
//}
int WsocksHostImpl::FreeOverlap(WSOVERLAPPED **ppendOvlBeg, WSOVERLAPPED **ppendOvlEnd, WSOVERLAPPED *povl)
{
	// Remove from port list
	#ifdef OVLMUX_CRIT_SECTION
	EnterCriticalSection(&ovlMutex);
	#else
	WaitForSingleObject(ovlMutex,INFINITE);
	#endif
	_ASSERT((ppendOvlBeg)&&(ppendOvlEnd));
	if((ppendOvlBeg)&&(*ppendOvlBeg==povl))
	{
		*ppendOvlBeg=povl->next;
		if(!*ppendOvlBeg)
			*ppendOvlEnd=0;
	}
	if((ppendOvlEnd)&&(*ppendOvlEnd==povl))
	{
		*ppendOvlEnd=povl->prev;
		if(!*ppendOvlEnd)
			*ppendOvlBeg=0;
	}
	if(povl->prev) povl->prev->next=povl->next;
	if(povl->next) povl->next->prev=povl->prev;

	// Put back on global free list
	povl->prev=0; povl->next=freeOvlList;
	if(freeOvlList) freeOvlList->prev=povl;
	freeOvlList=povl;
	#ifdef OVLMUX_CRIT_SECTION
	LeaveCriticalSection(&ovlMutex);
	#else
	ReleaseMutex(ovlMutex);
	#endif
	return 0;
}
#ifndef WS_OTPP
int WsocksHostImpl::WSHWaitOverlapCancel(WsocksApp *pmod)
{
#ifdef _DEBUG
	while(pmod->cxlOvlList)
#else
	for(int i=0;i<10;i++)
#endif
	{
		if(pmod->cxlOvlList)
			WSHIocpLoop(pmod,100);
	}
	return 0;
}
#endif//WS_OTPP
#endif//WS_OIO||WS_OTPP

// Default backup/restore functions
int WsocksHostImpl::WSHBackupApp(WsocksApp *pmod, char bpath[MAX_PATH], char emsg[256], bool overwrite)
{
	emsg[0]=0;
	char bdir[MAX_PATH],*bptr=0;
	GetFullPathName(bpath,sizeof(bdir),bdir,&bptr);
	if(!bptr) bptr=bdir;
	char *dptr=(char*)stristr(bptr,"yyyymmdd");
	if(dptr)
	{
		char dstr[16];
		sprintf(dstr,"%08d",pmod->buildDate);
		memcpy(dptr,dstr,8);
	}
	dptr=(char*)stristr(bptr,"hhmmss");
	if(dptr)
	{
		char tstr[16];
		sprintf(tstr,"%06d",pmod->buildTime);
		memcpy(dptr,tstr,6);
	}
	strcpy(bpath,bdir);

	if(PathFileExists(bdir))
	{
		if(!overwrite)
			return 0;
	}
	else
	{
		WSHMakeLocalDirs(pmod,bdir);
		CreateDirectory(bdir,0);
	}
	// App DLL
	char spath[MAX_PATH]={0},dpath[MAX_PATH]={0};
	strcpy(spath,pmod->pcfg->DllPath.c_str());
	char mpath[MAX_PATH]={0},*mname=0;
	GetFullPathName(pmod->pcfg->DllPath.c_str(),sizeof(mpath),mpath,&mname);
	sprintf(dpath,"%s\\%s",bdir,mname);
	if(!CopyFile(spath,dpath,false))
    {
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
    }
	// IQMatrix host
	GetModuleFileName(0,spath,sizeof(spath));
	GetFullPathName(spath,sizeof(mpath),mpath,&mname);
	sprintf(dpath,"%s\\%s",bdir,mname);
	if(!CopyFile(spath,dpath,false))
    {
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
    }

	sprintf(mpath,"%s\\setup",bdir);
	CreateDirectory(mpath,0);
	// setup\app.ini
	sprintf(spath,"%s\\setup\\app.ini",pmod->pcfg->RunPath.c_str());
	sprintf(dpath,"%s\\setup\\app.ini",bdir);
	if(!CopyFile(spath,dpath,false))
    {
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
    }
	// ports.txt
	strcpy(spath,pmod->pcfg->ConfigPath.c_str());
	sprintf(dpath,"%s\\setup\\ports.txt",bdir);
	if(!CopyFile(spath,dpath,false))
    {
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
    }
	// setup.ini
	sprintf(spath,"%s\\setup\\setup.ini",pmod->pcfg->RunPath.c_str());
	sprintf(dpath,"%s\\setup\\setup.ini",bdir);
	if(!CopyFile(spath,dpath,false))
    {
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
    }
	if(emsg[0])
	{
		// There may be more than one copy failure, but we're only reporting the last one
		return -1;
	}
	sprintf(emsg,"Backup(%s) copied",bdir);
	return 1;
}
int WsocksHostImpl::WSHRestoreApp(WsocksApp *pmod, char bpath[MAX_PATH], char emsg[256])
{
	emsg[0]=0;
	char bdir[MAX_PATH],*bptr=0;
	GetFullPathName(bpath,sizeof(bdir),bdir,&bptr);
	if(!PathFileExists(bpath))
	{
		sprintf(emsg,"Backup(%s) not found",bpath);
		return -1;
	}
	strcpy(bpath,bdir);

	// App DLL
	char spath[MAX_PATH]={0},dpath[MAX_PATH]={0};
	char dllRestorePath[MAX_PATH]={0},exeRestorePath[MAX_PATH]={0};
	strcpy(spath,pmod->pcfg->DllPath.c_str());
	sprintf(dpath,"%s.%08d_%06d.restore",spath,WSHDate,WSHTime);
	strcpy(dllRestorePath,dpath);
	if(!MoveFile(spath,dpath))
	{
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
		sprintf(emsg,"Failed renaming (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
		return -1;
	}
	char mpath[MAX_PATH]={0},*mname=0;
	GetFullPathName(pmod->pcfg->DllPath.c_str(),sizeof(mpath),mpath,&mname);
	sprintf(dpath,"%s\\%s",bdir,mname);
	if(!CopyFile(dpath,spath,false))
	{
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",dpath,spath,szReason);
        WSHLogError(pmod, "%s", emsg);
        MoveFile(dllRestorePath,pmod->pcfg->DllPath.c_str());
        return -1;
    }

	// IQMatrix host
	GetModuleFileName(0,spath,sizeof(spath));
	sprintf(dpath,"%s.%08d_%06d.restore",spath,WSHDate,WSHTime);
	strcpy(exeRestorePath,dpath);
	if(!MoveFile(spath,dpath))
	{
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
		sprintf(emsg,"Failed renaming (%s) to (%s)!: %s",spath,dpath,szReason);
        WSHLogError(pmod, "%s", emsg);
        MoveFile(dllRestorePath,pmod->pcfg->DllPath.c_str());
		return -1;
	}
	GetFullPathName(spath,sizeof(mpath),mpath,&mname);
	sprintf(dpath,"%s\\%s",bdir,mname);
	if(!CopyFile(dpath,spath,false))
	{
        char szReason[256] = {0};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
        while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
            szReason[strlen(szReason) - 1] = 0;
        sprintf(emsg,"Failed copying (%s) to (%s)!: %s",dpath,spath,szReason);
        WSHLogError(pmod, "%s", emsg);
        MoveFile(dllRestorePath,pmod->pcfg->DllPath.c_str());
        MoveFile(exeRestorePath,spath);
        return -1;
	}

	sprintf(mpath,"%s\\setup",bdir);
	if(PathFileExists(mpath))
	{
		// setup\app.ini
		sprintf(spath,"%s\\setup\\app.ini",pmod->pcfg->RunPath.c_str());
		sprintf(dpath,"%s\\setup\\app.ini",bdir);
		if(!CopyFile(dpath,spath,false))
        {
            char szReason[256] = {0};
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
            while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
                szReason[strlen(szReason) - 1] = 0;
			sprintf(emsg,"Failed copying (%s) to (%s)!: %s",dpath,spath,szReason);
            WSHLogError(pmod, "%s", emsg);
        }
		// ports.txt
		strcpy(spath,pmod->pcfg->ConfigPath.c_str());
		sprintf(dpath,"%s\\setup\\ports.txt",bdir);
		if(!CopyFile(dpath,spath,false))
        {
            char szReason[256] = {0};
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
            while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
                szReason[strlen(szReason) - 1] = 0;
			sprintf(emsg,"Failed copying (%s) to (%s)!: %s",dpath,spath,szReason);
            WSHLogError(pmod, "%s", emsg);
        }
		// setup.ini
		sprintf(spath,"%s\\setup\\setup.ini",pmod->pcfg->RunPath.c_str());
		sprintf(dpath,"%s\\setup\\setup.ini",bdir);
		if(!CopyFile(dpath,spath,false))
        {
            char szReason[256] = {0};
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
            while(szReason[0] && strchr("\r\n", szReason[strlen(szReason) - 1]))
                szReason[strlen(szReason) - 1] = 0;
			sprintf(emsg,"Failed copying (%s) to (%s)!: %s",dpath,spath,szReason);
            WSHLogError(pmod, "%s", emsg);
        }
	}
	if(emsg[0])
	{
		// There may be more than one copy failure, but we're only reporting the last one
		// We're not restoring the dll and exe and setup files for now
		return -1;
	}
	sprintf(emsg,"Backup(%s) restored",bdir);
	return 1;
}
int WsocksHostImpl::WSHRestart(WsocksApp *pmod, int timeout)
{
    // Restart ourself
    char epath[MAX_PATH];
    GetModuleFileName(0,epath,sizeof(epath));
	char args[64]={0};
#ifdef _DEBUG
	sprintf(args,"/startwait %d %d",GetCurrentProcessId(),timeout);
#else
	// Max 5 seconds to exit
	sprintf(args,"/startwait %d %d",GetCurrentProcessId(),timeout);
#endif
    WSHLogEvent(pmod,"Restarting [%s %s]",epath,args);
    ShellExecute(pmod->WShWnd,"open",epath,args,0,SW_SHOW);
	pmod->WSExit(0);
	return 0;
}
bool WsocksHostImpl::SetNetAlias(const char *alias, const char *val)
{
	if((!strcmp(alias,"clear"))&&(!strcmp(val,"all")))
	{
		netmap.clear();
		return true;
	}

	map<string,string>::iterator ait=netmap.find(alias);
	if(ait==netmap.end())
		netmap[alias]=val;
	else
	{
		if((!val)||(!*val))
			netmap.erase(ait);
		else
			ait->second=val;
	}
	return true;
}
bool WsocksHostImpl::WSHTranslateAlias(char *ipstr, int len)
{
	if(!ipstr)
		return false;
	// Ex. dal 232 (space required)
	char *iptr=strchr(ipstr,' ');
	if(!iptr)
		return false;
	char nipstr[1024]={0};
	strncpy(nipstr,ipstr,iptr -ipstr);
	for(map<string,string>::iterator ait=netmap.begin();ait!=netmap.end();ait++)
	{
		if(!_stricmp(nipstr,ait->first.c_str()))
		{
			const char *val=ait->second.c_str();
		#ifdef WIN32
			sprintf_s(nipstr,sizeof(nipstr),"%s%s",val,iptr +1);
		#else
			sprintf(nipstr,"%s%s",val,iptr +1);
		#endif
			strncpy(ipstr,nipstr,len);
			return true;
		}
	}
	return false;
}
#ifdef WSOCKS_SSL
// SSL certificates
int WsocksHostImpl::ReloadCertificates(bool server)
{
	// Validate server certificate existence
	if(server)
	{
		bool valid=true;
		if(SSLPW.empty())
		{
			WSHostLogError("SSLPW is empty!"); valid=false;
		}
		if(!PathFileExists(CAFILE.c_str()))
		{
			WSHostLogError("SSL CAFILE(%s) not found!",CAFILE.c_str()); valid=false;
		}
		if(!PathFileExists(CERTFILE.c_str()))
		{
			WSHostLogError("SSL CERTFILE(%s) not found!",CERTFILE.c_str()); valid=false;
		}
		if(!valid)
			return -1;
	}

	SSLGateway *dslg=0;
	if(sslg)
	{
		// This will bomb current SSL connections! Delete it after the new gateway has been created.
		// TODO: Modify SSLGateway to add a reload certificate feature.
		sslg->SSLCleanup();
		//delete sslg; 
		dslg=sslg;
	}
	sslg=new SSLGateway;
	if(!sslg->SSLInit(this,SSLPW.c_str(),server?CAFILE.c_str():"",server?CERTFILE.c_str():""))
	{
		delete sslg; sslg=0;
		if(dslg)
			sslg=dslg;
		return -1;
	}
	WSHostLogEvent("SSL initialized");
	if(dslg)
	{
		delete dslg; dslg=0;
	}
	return 0;
}
#endif

int WsocksGuiApp::WSCreateGuiHost(AppConfig *pcfg, bool lbmonitor)
{
	return WsocksHost::WSHCreateGuiHost(this,pcfg,lbmonitor);
}
int WsocksGuiApp::WSDestroyGuiHost()
{
	return WsocksHost::WSHDestroyGuiHost(this);
}
// A stubbed class for GUIs built as exes
class WSHGuiHost
	:public WsocksHostImpl
{
public:
	WSHGuiHost()
		:pcfg(0)
		,useLbmonitor(false)
	{
	}
	// WsocksHostImpl
	int WSHInitHost(const char *logPath);
	virtual void WSHSetAppHead(WsocksApp *pApp, const char *heading);
	virtual void WSHSetAppStatus(WsocksApp *pApp, const char *status);
	virtual int WSHGetAppHead(WsocksApp *pApp, char *heading, int hlen);
	virtual int WSHGetAppStatus(WsocksApp *pApp, char *status, int slen);
	int WSHExitApp(WsocksApp *pApp, int ecode);
	int WSHExitHost(int ecode);

	// Notifications
	virtual int WSHAppLoading(WsocksApp *pApp){return 0;}
	virtual int WSHAppLoadFailed(WsocksApp *pApp){return 0;}
	virtual int WSHAppLoaded(WsocksApp *pApp){return 0;}
	virtual int WSHAppUnloaded(WsocksApp *pApp){return 0;}
	virtual int WSHPortCreated(WsocksApp *pApp, WSPort *pport){return 0;}
	virtual int WSHPortModified(WsocksApp *pApp, WSPort *pport){return 0;}
	virtual int WSHPortDeleted(WsocksApp *pmod, WSPort *pport){return 0;}
	virtual int WSHLoggedEvent(WsocksApp *pApp, const char *estr){return 0;}
	virtual int WSHLoggedError(WsocksApp *pApp, const char *estr){return 0;}
	virtual int WSHLoggedDebug(WsocksApp *pApp, const char *estr){return 0;}
	virtual int WSHLoggedRetry(WsocksApp *pApp, const char *estr){return 0;}
	virtual int WSHLoggedRecover(WsocksApp *pApp, const char *estr){return 0;}
	virtual int WSHHostLoggedEvent(const char *estr){return 0;}
	virtual int WSHHostLoggedError(const char *estr){return 0;}
	virtual int WSHShowWindow(int nCmdShow){return 0;}

	static class WsocksApp *ptheApp;		// Only for WSHGuiHost_GetAppInterface
	static class WSHGuiHost *ptheHost;		// Only for OneSecondProc
	AppConfig *pcfg;						// Only for WSHInitHost
	bool useLbmonitor;						// Only for WSHInitHost
};
WsocksApp *WSHGuiHost::ptheApp=0;
WSHGuiHost *WSHGuiHost::ptheHost=0;
#ifdef WIN32
void __stdcall WSHGuiHost_FreeAppInterface(WsocksApp *pmod)
{
#else
void __stdcall WSHGuiHost_FreeAppInterface(void *amod)
{
	WsocksApp *pmod=(WsocksApp *)amod;
#endif
	// The app was not dynamicly allocated from WSHInitModule, so nothing to free
}
#ifdef WIN32
WsocksApp * __stdcall WSHGuiHost_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#else
void * __stdcall WSHGuiHost_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#endif
{
	if(version!=WSHOST_INTERFACE_VERSION)
		return 0;
	if(!WSHGuiHost::ptheApp)
		return 0;
	WSHGuiHost::ptheApp->pFreeAppInterface=WSHGuiHost_FreeAppInterface;
	return WSHGuiHost::ptheApp;
}
// Once per second timer callback
#ifdef WAITABLE_TIMERS
void CALLBACK OneSecondProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
	WsocksHost *phost=(WsocksHost *)lpArgToCompletionRoutine;
	if(phost)
		phost->WSHCheckTime();
}
#else
static void CALLBACK OneSecondProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	// idEvent is ignored when hwnd=0
	//if(idEvent==0x01)
	if(WSHGuiHost::ptheHost)
		WSHGuiHost::ptheHost->WSHCheckTime();
}
#endif
int WsocksHost::WSHCreateGuiHost(WsocksApp *pmod, AppConfig *acfg, bool lbmonitor)
{
	char rdir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,rdir);

	AppConfig *pcfg=new AppConfig;
	pcfg->line=0;
	pcfg->aclass=acfg->aclass;
	pcfg->aname=acfg->aname;
	pcfg->RunPath=acfg->RunPath.empty() ?rdir :acfg->RunPath;
	pcfg->LogPath=acfg->LogPath.empty() ?(string)rdir +(string)"\\logs" :acfg->LogPath;
	pcfg->RunCmd=acfg->aname +(string)".exe";
	pcfg->Enabled=true;
	pcfg->ver=acfg->ver;
	pcfg->asyncMode=acfg->asyncMode;
	pcfg->TargetThreads=acfg->TargetThreads;
	pcfg->sysmonAddr=acfg->sysmonAddr;
	pcfg->conOtmp=acfg->conOtmp;
	pcfg->usrOtmp=acfg->usrOtmp;
	pcfg->umrOtmp=acfg->umrOtmp;
	#ifdef WSOCKS_SSL
	pcfg->SSLPW=acfg->SSLPW;
	pcfg->CAFILE=acfg->CAFILE;
	pcfg->CERTFILE=acfg->CERTFILE;
	#endif
	pcfg->criticalApp=true;
	pcfg->loopback=true;
    pcfg->shareHostLog=acfg->shareHostLog;
	pcfg->plbGETAPPINTERFACE=WSHGuiHost_GetAppInterface;
	// Copy back values we set internally
	acfg->line=pcfg->line;
	acfg->RunPath=pcfg->RunPath;
	acfg->LogPath=pcfg->LogPath;
	acfg->RunCmd=pcfg->RunCmd;
	acfg->Enabled=pcfg->Enabled;
	acfg->criticalApp=pcfg->criticalApp;
	acfg->loopback=pcfg->loopback;
	acfg->plbGETAPPINTERFACE=pcfg->plbGETAPPINTERFACE;

	WSHGuiHost *phost=new WSHGuiHost;
	phost->pcfg=pcfg;
	phost->useLbmonitor=lbmonitor;
	WSHGuiHost::ptheApp=pmod;
	WSHGuiHost::ptheHost=phost;
	if(phost->WSHInitHost(pcfg->LogPath.c_str())<0)
		return -1;
	pmod->phost=phost;

	// Wait for app to load by newly created app thread before continuing
	WsocksApp *nmod=phost->WSHGetApp(pcfg->aname);
	while((!nmod)||(!nmod->initialized))
	{
		SleepEx(100,true);
		nmod=phost->WSHGetApp(pcfg->aname);
		if(pmod->initialized<0)
		{
			char lpath[MAX_PATH];
			sprintf(lpath,"%s\\%seve.txt",pmod->pcfg->LogPath.c_str(),phost->WSHcDate);
			ShellExecute(pmod->WShWnd,"open","notepad.exe",lpath,0,SW_SHOW);
			sprintf(lpath,"%s\\%shev.txt",pmod->pcfg->LogPath.c_str(),phost->WSHcDate);
			ShellExecute(pmod->WShWnd,"open","notepad.exe",lpath,0,SW_SHOW);
			return -1;
		}
	}
    phost->__HOST_BUILD_DATE__=nmod->__WSDATE__;
    phost->__HOST_BUILD_TIME__=nmod->__WSTIME__;
	#ifdef WAITABLE_TIMERS
	// Timer will be created in WSHThreadProc
	#else
	::SetTimer(0,0x01,1000,(TIMERPROC)::OneSecondProc);
	#endif
	return 0;
}
int WsocksHost::WSHDestroyGuiHost(WsocksApp *pmod)
{
	if(pmod->phost)
	{
		WSHGuiHost *phost=(WSHGuiHost*)pmod->phost;
		#ifndef WAITABLE_TIMERS
		::KillTimer(0,0x01); // Unclear from documentation if this does anything when hwnd=0
		#endif
		phost->WSHCleanupHost();
		delete phost;
		WSHGuiHost::ptheApp=0;
		WSHGuiHost::ptheHost=0;
	}
	return 0;
}
int WSHGuiHost::WSHInitHost(const char *logPath)
{
	// This only disables _ASSERT macros in debug builds.
	//_CrtSetReportMode(_CRT_ASSERT, 0);

	#ifndef MUX_CRIT_SECTION
	dirMutex=CreateMutex(0,false,0);
	#endif
	GetCurrentDirectory(MAX_PATH,hostRunDir);
	if(hostLogDir)
		strcpy(hostLogDir,logPath);
	else
		sprintf(hostLogDir,"%s\\logs",hostRunDir);
	CreateDirectory(hostLogDir,0);
	WSHCheckTime();
	WSHostLogEvent("--------------------------------------------------------------------------------");
	char ipath[MAX_PATH]={0};
	GetModuleFileName(0,ipath,MAX_PATH);

	char smon[8];
	int year=0,mon=0,day=0;
	sscanf(__HOST_BUILD_DATE__,"%s %d %d",smon,&day,&year);
	const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
	char *mptr=(char*)strstr(months,smon);
	if(mptr)
		mon=(int)(mptr -months)/3+1;
	int bdate=(year*10000)+(mon*100)+(day);

	int hh=0,mm=0,ss=0;
	sscanf(__HOST_BUILD_TIME__,"%d:%d:%d",&hh,&mm,&ss);
	int btime=(hh*10000)+(mm*100)+(ss);

	WSHostLogEvent("Initializing WsocksHost (%s) built %08d-%06d...",ipath,bdate,btime);
	#ifdef WS_WINMM
	if(timeBeginPeriod(1)!=TIMERR_NOERROR)
		return -1;
	#endif
	#if defined(WS_OIO)||defined(WS_OTPP)
	if(InitOverlap()<0)
		return -1;
	#endif
	if(WsocksHostImpl::ccodec.Init()<0)
		return -1;
	//if(fcodec.Init()<0)
	//	return -1;

	// Default network aliases
	netmap["DAL"]="100.100.0.";
	netmap["DAL2"]="100.101.0.";
	netmap["CHI"]="100.100.1.";
	netmap["CHI2"]="100.101.1.";
	netmap["SCS"]="171.185.250.";
	netmap["SCS2"]="171.185.251.";
	netmap["DEV"]="171.130.66.";
	netmap["DEV2"]="171.130.67.";

#ifdef WIN32
	int argc=0;
	WCHAR **argv=CommandLineToArgvW(str2w(GetCommandLine()),&argc);
	for(int i=1;i<argc;i++)
	{
		const char *arg=w2str(argv[i]);
		if((!_stricmp(arg,"/startdelay"))&&(i<argc-1))
		{
			arg=w2str(argv[++i]);
			SleepEx(atol(arg)*1000,true);
		}
		else if((!_stricmp(arg,"/startwait"))&&(i<argc-2))
		{
			arg=w2str(argv[++i]);
			int pid=atoi(arg);
			arg=w2str(argv[++i]);
			int maxWait=atoi(arg);
			HANDLE phnd=OpenProcess(PROCESS_TERMINATE|SYNCHRONIZE,false,pid);
			if(phnd)
			{
				WSHostLogEvent("Waiting up to %d seconds for process %d to exit...",maxWait,pid);
				if(WaitForSingleObject(phnd,maxWait*1000)==WAIT_TIMEOUT)
				{
					// Need to kill it
					WSHostLogError("Previous process %d timed out--Terminating...",pid);
					if(TerminateProcess(phnd,9))
						WSHostLogError("Process %d terminated.",pid);
					else
						WSHostLogError("Failed terminating process %d!",pid);
				}
				else
					WSHostLogEvent("Previous process %d just exited.",pid);
				CloseHandle(phnd);
			}
			else 
				WSHostLogEvent("Previous process %d already exited.",pid);
		}
	}
#endif

	if((useLbmonitor)&&(WSHCreateMonitorApp()<0))
		return -1;
	// app.ini replaced by WSHCreateGuiHost(AppConfig)
	//if(WSHLoadAppIni()<0)
	//	return -1;
	APPCONFIGLIST nmods;
	nmods.push_back(pcfg);
	WSHQueueApp(pcfg);

	configs.clear();
	configs.swap(nmods);
	if(WSHCreateThreadMonitor()<0)
		return -1;
	hostInitialized=1;
	return 0; 
}
void WSHGuiHost::WSHSetAppHead(WsocksApp *pApp, const char *heading)
{
}
void WSHGuiHost::WSHSetAppStatus(WsocksApp *pApp, const char *status)
{
}
int WSHGuiHost::WSHGetAppHead(WsocksApp *pApp, char *heading, int hlen)
{
	return 0;
}
int WSHGuiHost::WSHGetAppStatus(WsocksApp *pApp, char *status, int slen)
{
	return 0;
}
int WSHGuiHost::WSHExitApp(WsocksApp *pApp, int ecode)
{
	return 0;
}
int WSHGuiHost::WSHExitHost(int ecode)
{
	return 0;
}

#endif//WIN32
