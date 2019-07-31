
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

WsocksThread::WsocksThread()
	:phost(0)
	,name()
	,tid(0)
	,thnd(0)
	,abort(false)
	,appList()
	,unloadList()
	,reloadList()
	,loadList()
	,lastTickWarn(0)
	,lastTickErr(0)
	,threadAbortTimeout(DEF_THREAD_ABORT_TIMEOUT)
	,restartCnt(0)
	,startTime(0)
	,cfp(0)
	,activeApp(0)
{
	mutex=CreateMutex(0,false,0);
	memset(muxtrail,0,sizeof(muxtrail));
	muxtptr=muxtrail;
}
WsocksThread::~WsocksThread()
{
	Lock();
	#ifdef WIN32
	CloseHandle(mutex); mutex=0;
	#else
	DeleteMutex(mutex); mutex=0;
	#endif
}
int WsocksThread::Lock()
{
	if(!mutex)
		return -1;
	WaitForSingleObject(mutex,INFINITE);
	return 0;
}
void WsocksThread::Unlock()
{
	ReleaseMutex(mutex);
}

#ifdef WAITABLE_TIMERS
extern void CALLBACK OneSecondProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
#endif

#ifdef WIN32
static DWORD WINAPI BootThread(LPVOID arg)
#else
static void *BootThread(void *arg)
#endif
{
	WsocksThread *pthread=(WsocksThread *)arg;
	WsocksHostImpl *phost=pthread->phost;
	int rc=0;
	#ifndef _DEBUG
	__try{
	#endif

	rc=phost->WSHThreadProc(pthread);
	phost->WSHThreadExit(pthread,rc);

	#ifndef _DEBUG
	}
	__except(phost->WSHThreadCrashed(pthread,true,GetExceptionInformation())){
	}
	#endif
#ifdef WIN32
	return rc;
#else
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
#endif
}
#ifdef WIN32
int WsocksHostImpl::WSHThreadCrashed(WsocksThread *pthread, bool restart, EXCEPTION_POINTERS *pException)
#else
int WsocksHostImpl::WSHThreadCrashed(WsocksThread *pthread, bool restart)
#endif
{
	int lerr=GetLastError();
	string tname=pthread->name;
	WSHostLogError("Thread(%s) crashed: muxtrail(%s)!",tname.c_str(),pthread->muxtrail);
	// Copy to app's error log
	WsocksApp *pmod=pthread->activeApp;
	//for(WSAPPLIST::iterator ait=pthread->appList.begin();ait!=pthread->appList.end();ait++)
	//{
	//	WsocksApp *smod=*ait;
	//	if(smod->activeThread==pthread)
	//	{
	//		pmod=smod;
	//		break;
	//	}
	//}
	// Attempt to close ports on the bread crumb trail
	if(pmod)
	{
		pmod->WSLogError("Thread(%s) crashed: muxtrail(%s)!",tname.c_str(),pthread->muxtrail);
		SetLastError(lerr);
	#ifdef WIN32
		WSHGenerateDump(pmod,pthread->tid,pException);
	#endif
		WSHOpenThreadReportFile(pthread,pmod);
		if(!strncmp(pthread->muxtrail,"Cr",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CON_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_CON,PortNo);
				}
				WSHostLogError("App(%s): CON%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("CON%d: Closed after crashed thread.",PortNo);
				_WSHCloseConPort(pmod,PortNo);
			#ifdef WIN32
				if(pmod->ConPort[PortNo].smutex)
				{
					CloseHandle(pmod->ConPort[PortNo].smutex);
					pmod->ConPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->ConPort[PortNo].rmutex)
				{
					CloseHandle(pmod->ConPort[PortNo].rmutex);
					pmod->ConPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#else
				if(pmod->ConPort[PortNo].smutex)
				{
					DeleteMutex(pmod->ConPort[PortNo].smutex);
					pmod->ConPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->ConPort[PortNo].rmutex)
				{
					DeleteMutex(pmod->ConPort[PortNo].rmutex);
					pmod->ConPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#endif
			}
		}
	#ifdef WS_FILE_SERVER
		else if(!strncmp(pthread->muxtrail,"Fr",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_FILE_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_FIL,PortNo);
				}
				WSHostLogError("App(%s): FIL%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("FIL%d: Closed after crashed thread.",PortNo);
				_WSHCloseFilePort(pmod,PortNo);
				if(pmod->FilePort[PortNo].smutex)
				{
					CloseHandle(pmod->FilePort[PortNo].smutex);
					pmod->FilePort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->FilePort[PortNo].rmutex)
				{
					CloseHandle(pmod->FilePort[PortNo].rmutex);
					pmod->FilePort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			}
		}
	#endif
		else if(!strncmp(pthread->muxtrail,"ur",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_USR_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_USR,PortNo);
				}
				WSHostLogError("App(%s): USR%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("USR%d: Closed after crashed thread.",PortNo);
				_WSHCloseUsrPort(pmod,PortNo);
			#ifdef WIN32
				if(pmod->UsrPort[PortNo].smutex)
				{
					CloseHandle(pmod->UsrPort[PortNo].smutex);
					pmod->UsrPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->UsrPort[PortNo].rmutex)
				{
					CloseHandle(pmod->UsrPort[PortNo].rmutex);
					pmod->UsrPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#else
				if(pmod->UsrPort[PortNo].smutex)
				{
					DeleteMutex(pmod->UsrPort[PortNo].smutex);
					pmod->UsrPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->UsrPort[PortNo].rmutex)
				{
					DeleteMutex(pmod->UsrPort[PortNo].rmutex);
					pmod->UsrPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#endif
			}
		}
		else if(!strncmp(pthread->muxtrail,"mr",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_UMR_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_UMR,PortNo);
				}
				WSHostLogError("App(%s): UMR%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("UMR%d: Closed after crashed thread.",PortNo);
				_WSHCloseUmrPort(pmod,PortNo);
			#ifdef WIN32
				if(pmod->UmrPort[PortNo].smutex)
				{
					CloseHandle(pmod->UmrPort[PortNo].smutex);
					pmod->UmrPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->UmrPort[PortNo].rmutex)
				{
					CloseHandle(pmod->UmrPort[PortNo].rmutex);
					pmod->UmrPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#else
				if(pmod->UmrPort[PortNo].smutex)
				{
					DeleteMutex(pmod->UmrPort[PortNo].smutex);
					pmod->UmrPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->UmrPort[PortNo].rmutex)
				{
					DeleteMutex(pmod->UmrPort[PortNo].rmutex);
					pmod->UmrPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#endif
			}
		}
	#ifdef WS_GUARANTEED
		else if(!strncmp(pthread->muxtrail,"Dr",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CGD_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_CGD,PortNo);
				}
				WSHostLogError("App(%s): CGD%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("CGD%d: Closed after crashed thread.",PortNo);
				_WSHCloseCgdPort(pmod,PortNo);
			#ifdef WIN32
				if(pmod->CgdPort[PortNo].smutex)
				{
					CloseHandle(pmod->CgdPort[PortNo].smutex);
					pmod->CgdPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->CgdPort[PortNo].rmutex)
				{
					CloseHandle(pmod->CgdPort[PortNo].rmutex);
					pmod->CgdPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#else
				if(pmod->CgdPort[PortNo].smutex)
				{
					DeleteMutex(pmod->CgdPort[PortNo].smutex);
					pmod->CgdPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->CgdPort[PortNo].rmutex)
				{
					DeleteMutex(pmod->CgdPort[PortNo].rmutex);
					pmod->CgdPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#endif
			}
		}
		else if(!strncmp(pthread->muxtrail,"gr",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_UGR_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_UGR,PortNo);
				}
				WSHostLogError("App(%s): UGR%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("UGR%d: Closed after crashed thread.",PortNo);
				_WSHCloseUgrPort(pmod,PortNo);
			#ifdef WIN32
				if(pmod->UgrPort[PortNo].smutex)
				{
					CloseHandle(pmod->UgrPort[PortNo].smutex);
					pmod->UgrPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->UgrPort[PortNo].rmutex)
				{
					CloseHandle(pmod->UgrPort[PortNo].rmutex);
					pmod->UgrPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#else
				if(pmod->UgrPort[PortNo].smutex)
				{
					DeleteMutex(pmod->UgrPort[PortNo].smutex);
					pmod->UgrPort[PortNo].smutex=CreateMutex(0,false,0);
				}
				if(pmod->UgrPort[PortNo].rmutex)
				{
					DeleteMutex(pmod->UgrPort[PortNo].rmutex);
					pmod->UgrPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			#endif
			}
		}
	#endif
		else if(!strncmp(pthread->muxtrail,"Os",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CTO_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_CTO,PortNo);
				}
				WSHostLogError("App(%s): CTO%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("CTO%d: Closed after crashed thread.",PortNo);
				_WSHCloseCtoPort(pmod,PortNo);
				if(pmod->CtoPort[PortNo].smutex)
				{
					#ifdef WIN32
					CloseHandle(pmod->CtoPort[PortNo].smutex);
					#else
					DeleteMutex(pmod->CtoPort[PortNo].smutex);
					#endif
					pmod->CtoPort[PortNo].smutex=CreateMutex(0,false,0);
				}
			}
		}
		else if(!strncmp(pthread->muxtrail,"Ir",2))
		{
			int PortNo=atoi(pthread->muxtrail+2);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CTI_PORTS))
			{
				if(pthread->cfp)
				{
					SetLastError(lerr);
					pmod->WSThreadCrashed(pthread->cfp,WS_CTI,PortNo);
				}
				WSHostLogError("App(%s): CTI%d: Closed after crashed thread.",pmod->pcfg->aname.c_str(),PortNo);
				pmod->WSLogError("CTI%d: Closed after crashed thread.",PortNo);
				_WSHCloseCtiPort(pmod,PortNo);
				if(pmod->CtiPort[PortNo].rmutex)
				{
					#ifdef WIN32
					CloseHandle(pmod->CtiPort[PortNo].rmutex);
					#else
					DeleteMutex(pmod->CtiPort[PortNo].rmutex);
					#endif
					pmod->CtiPort[PortNo].rmutex=CreateMutex(0,false,0);
				}
			}
		}
		else if(pthread->cfp)
		{
			SetLastError(lerr);
			pmod->WSThreadCrashed(pthread->cfp,WS_UNKNOWN,-1);
		}
		WSHCloseThreadReportFile(pthread,pmod);
	}

	int threadAbortTimeout=pthread->threadAbortTimeout;
	int restartCnt=pthread->restartCnt;
	this->WSHThreadExit(pthread,69);
	#ifndef WIN32
	void *rc=0;
	pthread_join(pthread->thnd,&rc);
	#endif
	WSAPPLIST appList;
	pthread->Lock();
	appList.swap(pthread->appList);
	pthread->Unlock();
	for(WSAPPLIST::iterator ait=appList.begin();ait!=appList.end();ait++)
	{
		WsocksApp *pmod=*ait;
		if(pmod->activeThread==pthread)
			pmod->activeThread=0;
	}

	if(restart)
	{
		pthread=this->WSHCreateProcThread(tname.c_str(),threadAbortTimeout);
		if(pthread)
		{
			pthread->restartCnt=restartCnt +1;
			WSHostLogError("Thread(%s) was restarted %d time(s).",tname.c_str(),pthread->restartCnt);
			if(pmod)
				pmod->WSLogError("Thread(%s) was restarted %d time(s).",tname.c_str(),pthread->restartCnt);
			pthread->Lock();
			pthread->appList.swap(appList);
			pthread->Unlock();
		}
		else
			WSHostLogError("Thread(%s) failed to restart!",tname.c_str());
	}
	else
		WSHostLogError("Thread(%s) not restarted.",tname.c_str());
	return EXCEPTION_EXECUTE_HANDLER;
}
WsocksThread *WsocksHostImpl::WSHCreateProcThread(const char *tname, int threadAbortTimeout)
{
	WsocksThread *pthread=new WsocksThread;
	pthread->name=tname;
	pthread->phost=this;
	pthread->lastTickWarn=pthread->lastTickErr=GetTickCount();
	pthread->threadAbortTimeout=threadAbortTimeout;
#ifdef WIN32
	pthread->thnd=CreateThread(0,0,BootThread,pthread,0,&pthread->tid);
#else
	pthread_create(&pthread->thnd,0,BootThread,pthread);
#endif
	if(!pthread->thnd)
	{
		delete pthread;
		return 0;
	}
	WSHWaitMutex(0x03,INFINITE);
	threadMap[pthread->name]=pthread;
	threadIdMap[pthread->tid]=pthread;
	WSHReleaseMutex(0x03);
	return pthread; 
}
int WsocksHostImpl::WSHThreadProc(WsocksThread *pthread)
{ 
	WSHostLogEvent("Thread %s started (THREAD_ABORT_TIMEOUT=%d)...",
		pthread->name.c_str(),pthread->threadAbortTimeout,pthread->appList.size());
	WORD acnt=0;
	DWORD trest=0;
	pthread->startTime=WSHTime;
	while(true)
	{
		pthread->Lock();
		if(!pthread->phost->hostInitialized)
		{
			pthread->Unlock();
			if(pthread->abort)
				break;
			SleepEx(100,true);
			continue;
		}
		pthread->lastTickWarn=pthread->lastTickErr=GetTickCount();
		// Unload apps	
		while(!pthread->unloadList.empty())
		{
			WsocksApp *umod=pthread->unloadList.front();
			WaitForSingleObject(umod->appMutex,INFINITE);
			if(!umod->activeThread)
			{
				umod->activeThread=pthread;
				ReleaseMutex(umod->appMutex);
				pthread->unloadList.pop_front();
				WSHUnloadApp(umod);
			}
			else
				ReleaseMutex(umod->appMutex);
		}
		// Reload apps	
		while(!pthread->reloadList.empty())
		{
			WsocksApp *umod=pthread->reloadList.front();
			WaitForSingleObject(umod->appMutex,INFINITE);
			if(!umod->activeThread)
			{
				umod->activeThread=pthread;
				ReleaseMutex(umod->appMutex);
				pthread->reloadList.pop_front();
				AppConfig *pcfg=umod->pcfg;
				WSHUnloadApp(umod);
				WSHQueueApp(pcfg);
			}
			else
				ReleaseMutex(umod->appMutex);
		}
		// Load new apps
		while(!pthread->loadList.empty())
		{
			AppConfig *pcfg=pthread->loadList.front();
			pthread->loadList.pop_front();
			WsocksApp *nmod=WSHLoadApp(pcfg);

		#ifdef WAITABLE_TIMERS
			// Waitable timer must be created by a thread that will enter alertable state
			if(!oneSecTimer)
			{
				oneSecTimer=CreateWaitableTimer(0,true,0);
				if(oneSecTimer)
				{
					SYSTEMTIME tsys;
					GetSystemTime(&tsys);
					//tsys.wSecond++;
					//tsys.wMilliseconds=0;
					FILETIME fsys;
					SystemTimeToFileTime(&tsys,&fsys);
					LARGE_INTEGER li;
					li.LowPart=fsys.dwLowDateTime;
					li.HighPart=fsys.dwHighDateTime;
					SetWaitableTimer(oneSecTimer,&li,1000,::OneSecondProc,this,false);
				}
			}
		#endif
		}
		if(pthread->abort)
		{
			pthread->Unlock();
			break;
		}
		if(!pthread->appList.size())
		{
			pthread->Unlock();
			SleepEx(1000,true);
			continue;
		}
		WsocksApp *pmod=pthread->appList.front();
		if(pthread->appList.size()>1)
		{
			pthread->appList.pop_front();
			pthread->appList.push_back(pmod);
		}
		pthread->activeApp=pmod;
		pthread->Unlock();

		// App closing
		if(pmod->threadSignal&0x80000000)
			continue;
		// Mutli-thread exclusion
		bool thisThread=false;
		WaitForSingleObject(pmod->appMutex,INFINITE);
		if(!pmod->activeThread)
		{
			pmod->activeThread=pthread; thisThread=true;
		}
		ReleaseMutex(pmod->appMutex);

		// Date change
		if((thisThread)&&(pmod->threadSignal&0x02))
		{
			pmod->WSDate=WSHDate;
			pmod->WSTime=WSHTime;
			memcpy(pmod->WScDate,WSHcDate,9);
			WSDateChanging(pmod);
			pmod->WSDateChange();
			pmod->threadSignal&=~0x02;
		}
		// Time change
		if((thisThread)&&(pmod->threadSignal&0x01))
		{
			DWORD tnow=GetTickCount();
			pmod->lastTickDiff=pmod->lastTimeChange ?(tnow -pmod->lastTimeChange) :0;
			pmod->lastTimeChange=tnow;
			pmod->WSTime=WSHTime;
			memcpy(pmod->WScTime,WSHcTime,9);
			WSTimeChanging(pmod);
			pmod->WSTimeChange();
			pmod->threadSignal&=~0x01;
		}

		// Normal app loop
	#ifdef WS_OIO
		WSHIocpLoop(pmod,WS_TIMER_INTERVAL);
		if(thisThread)
			WSHSyncLoop(pmod);
	#elif defined(WS_OTPP)
		if(thisThread)
		{
			WSHSyncLoop(pmod);
			// In case there's only one thread configured,
			// we must perform WSAsyncLoop for I/O processing
			WSHAsyncLoop(pmod);
		}
		else if(pmod->pcfg->asyncMode)
			WSHAsyncLoop(pmod);
	#else
		if(thisThread)
			WSHSyncLoop(pmod);
		else if(pmod->pcfg->asyncMode)
			WSHAsyncLoop(pmod);
	#endif

		// Idle time
		if((thisThread)&&(pmod->threadIdleStop))
		{
			pmod->WSIdle(pmod->threadIdleStop);
			pmod->threadIdleStop=0;
		}

		// Mutli-thread exclusion
		if(thisThread)
		{
			WaitForSingleObject(pmod->appMutex,INFINITE);
			pmod->activeThread=0;
			ReleaseMutex(pmod->appMutex);
		}
		pthread->activeApp=0;

		#ifndef WS_OIO
		// Only rest after going through all apps
		if(++acnt>=pthread->appList.size())
		{
			acnt=0;
			if(WS_TIMER_INTERVAL>0)
				SleepEx(WS_TIMER_INTERVAL,true);
			//// Keep the CPU down by resting a few times a second
			//else
			//{
			//	DWORD tnow=GetTickCount();
			//	if(tnow -trest>=50)
			//	{
			//		trest=tnow;
			//		SleepEx(1,true);
			//	}
			//}
		}
		#endif
	}
	WSHostLogEvent("Thread %s exit.",pthread->name.c_str());
	return 0; 
}
void WsocksHostImpl::WSHThreadExit(WsocksThread *pthread, int ecode)
{
	WSHWaitMutex(0x03,INFINITE);
	WSTHREADIDMAP::iterator tiit=threadIdMap.find(pthread->tid);
	if(tiit!=threadIdMap.end())
		threadIdMap.erase(tiit);
	WSTHREADMAP::iterator tit=threadMap.find(pthread->name);
	if(tit!=threadMap.end())
	{
		threadMap.erase(tit);
		if(pthread->thnd) CloseHandle(pthread->thnd);
		delete pthread;
	}
	WSHReleaseMutex(0x03);
}
int WsocksHostImpl::WSHOpenThreadReportFile(WsocksThread *pthread, WsocksApp *pmod)
{
	if((!pthread)||(!pmod))
		return -1;
	char cpath[MAX_PATH]={0};
	sprintf(cpath,"%s\\TCP_Recordings",pmod->pcfg->RunPath.c_str());
	CreateDirectory(cpath,0);
	sprintf(cpath,"%s\\TCP_Recordings\\THREAD-%s-%06d.RPT",
		pmod->pcfg->RunPath.c_str(),pthread->name.c_str(),WSHTime);
	pthread->cfp=fopen(cpath,"ab");
	if(!pthread->cfp)
		return -1;
	pthread->cfpath=cpath;
	return 0;
}
int WsocksHostImpl::WSHCloseThreadReportFile(WsocksThread *pthread, WsocksApp *pmod)
{
	if((!pthread)||(!pthread->cfp)||(!pthread->cfp))
		return -1;
	int fsize=ftell(pthread->cfp);
	fclose(pthread->cfp); pthread->cfp=0;
	if(fsize<1)
		DeleteFile(pthread->cfpath.c_str());
	else
	{
		WSHostLogError("Thread report(%s) created (%d bytes).",pthread->cfpath.c_str(),fsize);
		pmod->WSLogError("Thread report(%s) created (%d bytes).",pthread->cfpath.c_str(),fsize);
		pthread->cfpath.clear();
	}
	return 0;
}

#ifdef WIN32
int WsocksHostImpl::WSHCleanThreadReports(WsocksApp *pmod, int ndays, int maxsize)
{
	time_t tnow=time(0);
	struct tm ltm=*localtime(&tnow);
	ltm.tm_mday-=ndays;
	mktime(&ltm);
	int ddate=((ltm.tm_year +1900)*10000) +((ltm.tm_mon +1)*100) + (ltm.tm_mday);
	WSHLogEvent(pmod,"Cleaning up TCP_Recordings .DMP and .RPT files on or before %08d and > %dMB total...",ddate,maxsize);
	int ndel=0;
	multimap<int,WIN32_FIND_DATA> keepMap;
	__int64 keepSize=0;
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile("TCP_Recordings\\*.*",&fdata);
	if(fhnd!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				;
			else
			{
				SYSTEMTIME ftsys;
				FileTimeToSystemTime(&fdata.ftLastWriteTime,&ftsys);
				bool keep=false;
				int fdate=(ftsys.wYear*10000) +(ftsys.wMonth*100) +(ftsys.wDay);
				char *ext=strrchr(fdata.cFileName,'.');
				bool extmatch=false;
				if(ext)
				{
					if(!_stricmp(ext,".RPT")||!_stricmp(ext,".DMP"))
						extmatch=true;
				}
				if(extmatch)
				{
					if(fdate<=ddate)
					{
						char fpath[MAX_PATH]={0};
						sprintf(fpath,"TCP_Recordings\\%s",fdata.cFileName);
						if(DeleteFile(fpath))
						{
							ndel++;
							WSHLogEvent(pmod,"Deleted (%s), last written %08d",fpath,fdate);
						}
						else
							WSHLogError(pmod,"Failed deleting (%s)",fpath);
					}
					else
						keep=true;
				}

				if(keep)
				{
					fdata.ftLastWriteTime.dwHighDateTime=fdate;
					fdata.ftLastWriteTime.dwLowDateTime=(ftsys.wHour) +(ftsys.wMinute) +(ftsys.wSecond);
					multimap<int,WIN32_FIND_DATA>::iterator kit;
					// Ascending by date+time
					for(kit=keepMap.begin();
						(kit!=keepMap.end())&&((kit->first<fdate)||
											  ((kit->first==fdate)&&(kit->second.ftLastWriteTime.dwLowDateTime<fdata.ftLastWriteTime.dwLowDateTime)));
						kit++)
						;
					if(kit==keepMap.end())
						keepMap.insert(pair<int,WIN32_FIND_DATA>(fdate,fdata));
					else
						keepMap.insert(kit,pair<int,WIN32_FIND_DATA>(fdate,fdata));
					LARGE_INTEGER fsize;
					fsize.HighPart=fdata.nFileSizeHigh;
					fsize.LowPart=fdata.nFileSizeLow;
					keepSize+=fsize.QuadPart;
				}
			}
		}while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
	// Keep at most 100MB
	if(keepSize>=maxsize*1024*1024)
	{
		for(multimap<int,WIN32_FIND_DATA>::iterator kit=keepMap.begin();kit!=keepMap.end();kit++)
		{
			WIN32_FIND_DATA fdata=kit->second;
			char fpath[MAX_PATH]={0};
			sprintf(fpath,"TCP_Recordings\\%s",fdata.cFileName);
			if(DeleteFile(fpath))
			{
				ndel++;
				WSHLogEvent(pmod,"Deleted (%s), last written %08d",fpath,kit->first);

				LARGE_INTEGER fsize;
				fsize.HighPart=fdata.nFileSizeHigh;
				fsize.LowPart=fdata.nFileSizeLow;
				keepSize-=fsize.QuadPart;
				if(keepSize<maxsize*1024*1024)
					break;
			}
			else
				WSHLogError(pmod,"Failed deleting (%s)",fpath);
		}
	}
	keepMap.clear();
	WSHLogEvent(pmod,"Cleaned up %d files",ndel);
	return 0;
}

// Copied from Nexus
#define STRSAFE_NO_DEPRECATE
#include <DbgHelp.h>
#include <Strsafe.h>
#include <Shlwapi.h>
int WsocksHostImpl::WSHGenerateDump(WsocksApp *pmod, DWORD tid, EXCEPTION_POINTERS *pException)
{	
	#ifdef VS2010
	// Note that DbgHelp.dll should be marked as delayed-load DLL
	char dhpath[MAX_PATH]={0};
	GetSystemDirectory(dhpath,sizeof(dhpath));
	strcat_s(dhpath,sizeof(dhpath),"\\DbgHelp.dll");
	if(PathFileExists(dhpath))
	{
		TCHAR szFileName[_MAX_PATH] = { 0 }; 	
		DWORD dwBufferSize = _MAX_PATH;	
		MINIDUMP_EXCEPTION_INFORMATION miniDumpExceptionInformation = { tid, pException, TRUE };
	
		StringCchPrintf(szFileName, _MAX_PATH, "%s\\TCP_Recordings", pmod->pcfg->RunPath.c_str());
		CreateDirectory((LPCSTR)szFileName, NULL);
		
		StringCchPrintf((STRSAFE_LPSTR)szFileName, _MAX_PATH, "%s\\TCP_Recordings\\B%08d-%06d-E%08d-%06d-P%ld-T%ld.dmp", 
			pmod->pcfg->RunPath.c_str(), pmod->buildDate, pmod->buildTime, 
			pmod->WSDate, pmod->WSTime, GetCurrentProcessId(), tid);
		HANDLE hDumpFile = CreateFile((LPCSTR)szFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	
		WSHLogError(pmod,"Generating memory dump '%s'...", szFileName); 
		BOOL bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithThreadInfo, pException ? &miniDumpExceptionInformation : 0, 0, 0);
		// MiniDumpWithThreadInfo not supported prior to DbgHelp 6.1
		if(!bMiniDumpSuccessful)
			bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithHandleData, pException ? &miniDumpExceptionInformation : 0, 0, 0);

		CloseHandle(hDumpFile);
	}
	#endif
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif//WIN32

#ifdef WIN32
static DWORD WINAPI BootThreadMonitor(LPVOID arg)
#else
static void *BootThreadMonitor(void *arg)
#endif
{
	int rc=0;
	WsocksHostImpl *phost=(WsocksHostImpl*)arg;
	phost->WSHostLogEvent("ThreadMonitor started...");
	try{
	rc=phost->WSHThreadMonitor();
	}
	catch(...){
		phost->WSHostLogError("ThreadMonitor crashed!");
		if(phost->hostInitialized)
			phost->WSHCreateThreadMonitor();
	}
	phost->WSHostLogEvent("ThreadMonitor exit.");
	#ifdef WIN32
	return rc;
	#else
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void *)(PTRCAST)rc;
	#endif
}
int WsocksHostImpl::WSHThreadMonitor()
{
	while(!stoptm)
	{
		if(!hostInitialized)
		{
			SleepEx(100,true);
			continue;
		}
		WSHWaitMutex(0x03,INFINITE);
		DWORD tnow=GetTickCount();
		list<WsocksThread *> tlist;
		for(WSTHREADMAP::iterator tit=threadMap.begin();tit!=threadMap.end();tit++)
			tlist.push_back(tit->second);
		WSHReleaseMutex(0x03);
		for(list<WsocksThread*>::iterator ptit=tlist.begin();ptit!=tlist.end();)
		{
			WsocksThread *pthread=*ptit;
			if((pthread)&&(pthread->lastTickErr)&&(tnow>pthread->lastTickErr)&&
			   (pthread->threadAbortTimeout>0)&&((int)(tnow -pthread->lastTickErr)>=(pthread->threadAbortTimeout*1000)))
			{
				// If LBMon thread is hung there is no way to remotely kill threads
				if(pthread->name=="LBMon")
				{
				#ifdef _DEBUG
					WSHostLogError("Thread(%s) last tick was %d ms ago, exceeding THREAD_ABORT_TIMEOUT(%d): muxtrail(%s)!",
						pthread->name.c_str(),tnow -pthread->lastTickErr,pthread->threadAbortTimeout,pthread->muxtrail);
					pthread->lastTickErr=tnow;
				#else
					// Restarting the thread never seems to resolve the problem.
					// Kill (and restart) the thread
					bool restart=(pthread->appList.size()>0?true:false);
					if(restart)
						WSHostLogError("Thread(%s) last tick was %d ms ago, exceeding THREAD_ABORT_TIMEOUT(%d): muxtrail(%s) ! Restarting thread...",
							pthread->name.c_str(),tnow -pthread->lastTickErr,pthread->threadAbortTimeout,pthread->muxtrail);
					else
						WSHostLogError("Thread(%s) last tick was %d ms ago, exceeding THREAD_ABORT_TIMEOUT(%d): muxtrail(%s)! Not restarting.",
							pthread->name.c_str(),tnow -pthread->lastTickErr,pthread->threadAbortTimeout,pthread->muxtrail);
					pthread->lastTickErr=tnow;
					string tname=pthread->name;
					int threadAbortTimeout=pthread->threadAbortTimeout;
					int restartCnt=pthread->restartCnt;
					HANDLE thnd=pthread->thnd;
					ptit=tlist.erase(ptit);
					WSAPPLIST appList;
					pthread->Lock();
					appList.swap(pthread->appList);
					pthread->Unlock();
					for(WSAPPLIST::iterator ait=appList.begin();ait!=appList.end();ait++)
					{
						WsocksApp *pmod=*ait;
						if(pmod->activeThread==pthread)
							pmod->activeThread=0;
					}
				#ifdef WIN32
					BOOL rc=TerminateThread(thnd,69);
					WSHThreadExit(pthread,69);
				#else
					BOOL rc=-1;
					pthread_join(&thnd,(void*)&rc);
				#endif
					pthread=0;
					if(rc)
					{
						if(restart)
						{
							pthread=WSHCreateProcThread(tname.c_str(),threadAbortTimeout);
							if(pthread)
							{
								pthread->restartCnt=restartCnt +1;
								WSHostLogError("Thread(%s) was terminated and restarted %d time(s).",
									tname.c_str(),pthread->restartCnt);
								pthread->Lock();
								pthread->appList.swap(appList);
								pthread->Unlock();
							}
							else
								WSHostLogError("Thread(%s) was terminated but failed to restart!",tname.c_str());
						}
					}
					else
						WSHostLogError("Failed terminating thread(%s)!",tname.c_str());
					continue;
					//for(WSAPPLIST::iterator ait=pthread->appList.begin();ait!=pthread->appList.end();ait++)
					//{
					//	WsocksApp *pmod=*ait;
					//	if(pmod->pcfg->criticalApp)
					//	{
					//		SleepEx(1000,true); // wait for host error log to write
					//		#ifdef WIN32
					//		char lpath[MAX_PATH];
					//		sprintf(lpath,"%s\\%seve.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
					//		ShellExecute(pmod->WShWnd,"open","notepad.exe",lpath,0,SW_SHOW);
					//		sprintf(lpath,"%s\\%sher.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
					//		ShellExecute(pmod->WShWnd,"open","notepad.exe",lpath,0,SW_SHOW);
					//		#endif
					//		_exit(9);
					//		break;
					//	}
					//}
				#endif
				}
				// After discussion with ops, make remote kill and restart manual.
				DWORD tdiff=tnow -pthread->lastTickErr;
				WSHostLogError("Thread(%s) last tick was %d ms ago, exceeding THREAD_ABORT_TIMEOUT(%d): muxtrail(%s)!",
					pthread->name.c_str(),tdiff,pthread->threadAbortTimeout,pthread->muxtrail);
				// Copy to app's error log
				for(WSAPPLIST::iterator ait=pthread->appList.begin();ait!=pthread->appList.end();ait++)
				{
					WsocksApp *pmod=*ait;
					if(pthread->activeApp==pmod)
					{
						pmod->WSLogError("Thread(%s) last tick was %d ms ago, exceeding THREAD_ABORT_TIMEOUT(%d): muxtrail(%s)!",
							pthread->name.c_str(),tdiff,pthread->threadAbortTimeout,pthread->muxtrail);
						// Decode the ports involved
						WSHOpenThreadReportFile(pthread,pmod);
						if(!strncmp(pthread->muxtrail,"Cr",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_CON_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_CON,PortNo,tdiff);
						}
					#ifdef WS_FILE_SERVER
						else if(!strncmp(pthread->muxtrail,"Fr",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_FILE_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_FIL,PortNo,tdiff);
						}
					#endif
						else if(!strncmp(pthread->muxtrail,"ur",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_USR_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_USR,PortNo,tdiff);
						}
						else if(!strncmp(pthread->muxtrail,"mr",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_UMR_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_UMR,PortNo,tdiff);
						}
					#ifdef WS_GUARANTEED
						else if(!strncmp(pthread->muxtrail,"Dr",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_CGD_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_CGD,PortNo,tdiff);
						}
						else if(!strncmp(pthread->muxtrail,"gr",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_UGR_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_UGR,PortNo,tdiff);
						}
					#endif
						else if(!strncmp(pthread->muxtrail,"Os",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_CTO_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_CTO,PortNo,tdiff);
						}
						else if(!strncmp(pthread->muxtrail,"Ir",2))
						{
							int PortNo=atoi(pthread->muxtrail+2);
							if((PortNo>=0)&&(PortNo<pmod->NO_OF_CTI_PORTS)&&(pthread->cfp))
								pmod->WSThreadTimeout(pthread->cfp,WS_CTI,PortNo,tdiff);
						}
						else if(pthread->cfp)
							pmod->WSThreadTimeout(pthread->cfp,WS_UNKNOWN,-1,tdiff);
						WSHCloseThreadReportFile(pthread,pmod);
						break;
					}
				}
				pthread->lastTickErr=tnow;
			}
			else if((pthread)&&(pthread->lastTickWarn)&&(tnow>pthread->lastTickWarn)&&(tnow -pthread->lastTickWarn>=3000))
			{
				WSHostLogEvent("Thread(%s) last tick was %d ms ago!",pthread->name.c_str(),tnow -pthread->lastTickErr);
				pthread->lastTickWarn=tnow;
			}
			ptit++;
		}
		tlist.clear();
		SleepEx(1000,true);
	}
	tmhnd=0;
	return 0;
}
int WsocksHostImpl::WSHCreateThreadMonitor()
{
#ifdef WIN32
	DWORD tid=0;
	tmhnd=CreateThread(0,0,BootThreadMonitor,this,0,&tid);
	if(!tmhnd)
		return -1;
	return 0; 
#else
	pthread_t thnd;
	if(pthread_create(&thnd,0,BootThreadMonitor,this))
		return -1;
	return 0;
#endif
}
int WsocksHostImpl::WSHBusyThreadReport(WsocksApp *pmod, DWORD estEndTime)
{
	DWORD tid=GetCurrentThreadId();
	WSHWaitMutex(0x03,INFINITE);
	for(WSTHREADMAP::iterator tit=threadMap.begin();tit!=threadMap.end();tit++)
	{
		WsocksThread *pthread=tit->second;
		if(pthread->tid==tid)
		{
			pthread->lastTickWarn=pthread->lastTickErr=estEndTime ?estEndTime :GetTickCount();
			WSHReleaseMutex(0x03);
			return 0;
		}
	}
	WSHReleaseMutex(0x03);
	return -1;
}
