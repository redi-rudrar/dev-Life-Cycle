
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

void WsocksHostImpl::fileLogError(WsocksApp *pmod, char *Error)
{
	//WSHWaitMutex(0x02,INFINITE);
	if (WSHDate!=pmod->fpErrDate)
	{
	#ifdef WIN32
		if(pmod->errhnd!=INVALID_FILE_VALUE)
			CloseHandle(pmod->errhnd);
	#else
		if(pmod->errhnd)
			fclose(pmod->errhnd);
	#endif
		pmod->errhnd=INVALID_FILE_VALUE;
	}
	if(pmod->errhnd==INVALID_FILE_VALUE)
	{
		char lpath[MAX_PATH];
	#ifdef WIN32
        if(pmod->pcfg->shareHostLog)
		{
			sprintf(lpath,"%s\\%sher.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			pmod->WSErrorLogPath=lpath;
		}
		else
		{
			#ifdef CLSERVER_EXPORT
			sprintf(lpath,"%s\\export%serr.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#else
			sprintf(lpath,"%s\\%serr.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#endif
			pmod->WSErrorLogPath=lpath;
		}
		if((pmod->errhnd=CreateFile(pmod->WSErrorLogPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_ALWAYS,0,0))==INVALID_HANDLE_VALUE)
		{
			//WSHReleaseMutex(0x02);
			return;
		}
		SetFilePointer(pmod->errhnd,0,0,FILE_END);
	#else
		if(pmod->pcfg->shareHostLog)
		{
			sprintf(lpath,"%s/%sher.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			pmod->WSErrorLogPath=lpath;
		}
		else
		{
			#ifdef CLSERVER_EXPORT
			sprintf(lpath,"%s/export%serr.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#else
			sprintf(lpath,"%s/%serr.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#endif
			pmod->WSErrorLogPath=lpath;
		}
		if((pmod->errhnd=fopen(pmod->WSErrorLogPath.c_str(),"at"))==0)
		{
			//WSHReleaseMutex(0x02);
			return;
		}
		fseek(pmod->errhnd,0,SEEK_END);
	#endif
		pmod->fpErrDate=WSHDate;
	}

#ifdef WIN32
	__int64 coff=SetFilePointer(pmod->errhnd,0,0,FILE_END);
	DWORD wbytes=0;
	if(!WriteFile(pmod->errhnd,Error,(int)strlen(Error),&wbytes,0))
	{
		//WSHReleaseMutex(0x02);
		return;	
	}
#else
	__int64 coff=fseeko(pmod->errhnd,0,SEEK_END);
	if(fwrite(Error,(int)strlen(Error),1,pmod->errhnd)!=1)
		return;
#endif
	//WSHReleaseMutex(0x02);
	if(lbmon)
		lbmon->LBMLoggedError(pmod,coff,Error);
	//FlushFileBuffers(pmod->errhnd);
}
void WsocksHostImpl::fileLogEvent(WsocksApp *pmod, char *Event)
{
	//WSHWaitMutex(0x02,INFINITE);
	if (WSHDate!=pmod->fpEveDate)
	{
	#ifdef WIN32
		if(pmod->evehnd!=INVALID_FILE_VALUE)
			CloseHandle(pmod->evehnd);
	#else
		if(pmod->evehnd)
			fclose(pmod->evehnd);
	#endif
		pmod->evehnd=INVALID_FILE_VALUE;
	}
	if(pmod->evehnd==INVALID_FILE_VALUE)
	{
		char lpath[MAX_PATH];
	#ifdef WIN32
		if(pmod->pcfg->shareHostLog)
		{
			sprintf(lpath,"%s\\%shev.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			pmod->WSEventLogPath=lpath;
		}
		else
		{
			#ifdef CLSERVER_EXPORT
			sprintf(lpath,"%s\\export%seve.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#else
			sprintf(lpath,"%s\\%seve.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#endif
			pmod->WSEventLogPath=lpath;
		}
		if((pmod->evehnd=CreateFile(pmod->WSEventLogPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_ALWAYS,0,0))==INVALID_HANDLE_VALUE)
		{
			//WSHReleaseMutex(0x02);
			return;
		}
		SetFilePointer(pmod->evehnd,0,0,FILE_END);
	#else
		if(pmod->pcfg->shareHostLog)
		{
			sprintf(lpath,"%s/%shev.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			pmod->WSEventLogPath=lpath;
		}
		else
		{
			#ifdef CLSERVER_EXPORT
			sprintf(lpath,"%s/export%seve.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#else
			sprintf(lpath,"%s/%seve.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
			#endif
			pmod->WSEventLogPath=lpath;
		}
		if((pmod->evehnd=fopen(pmod->WSEventLogPath.c_str(),"at"))==0)
		{
			//WSHReleaseMutex(0x02);
			return;
		}
		fseek(pmod->evehnd,0,SEEK_END);
	#endif
		pmod->fpEveDate=WSHDate;
	}

#ifdef WIN32
	__int64 coff=SetFilePointer(pmod->evehnd,0,0,FILE_END);
	DWORD wbytes=0;
	if(!WriteFile(pmod->evehnd,Event,(int)strlen(Event),&wbytes,0))
	{
		//WSHReleaseMutex(0x02);
		return;
	}
#else
	__int64 coff=fseeko(pmod->evehnd,0,SEEK_END);
	if(fwrite(Event,(int)strlen(Event),1,pmod->evehnd)!=1)
		return;
#endif
	//WSHReleaseMutex(0x02);
	if(lbmon)
		lbmon->LBMLoggedEvent(pmod,coff,Event);
	//FlushFileBuffers(pmod->evehnd);
}
void WsocksHostImpl::WSHLogError(WsocksApp *pmod, const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHLogErrorVA(pmod,format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHLogErrorVA(WsocksApp *pmod, const char *format, va_list& ptr)
{
	// Create a UUID for this error for THE EYE
#ifdef WIN32
	UUID uuid;
	unsigned char *suuid;
	UuidCreateSequential(&uuid);
	UuidToString(&uuid, &suuid);
#else
	unsigned char uuid[16]={0};
	unsigned char *suuid=uuid;
#endif

	char buf[2048];
	//WSErrorCount++;
	vsnprintf_s ( buf, sizeof(buf), _TRUNCATE, format, ptr ) ;
	char Tempstr[2048]={0},*tptr=Tempstr;
	sprintf(tptr,"%8s:%c%c@%6s | ",pmod->pcfg->aname.c_str(),WSHcDate[6],WSHcDate[7],WSHcTime); tptr+=strlen(tptr);
	strncpy(tptr,buf,1900); tptr+=strlen(tptr); char *uptr=tptr;
	sprintf(tptr," {%s}",suuid); tptr+=strlen(tptr);
	strcpy(tptr,"\r\n"); tptr+=2;
	_ASSERT(tptr -Tempstr<2047);
	Tempstr[2047]=0;
#ifdef WIN32
	RpcStringFree(&suuid);
#endif

	fileLogError(pmod,Tempstr);
	strcpy(uptr,"\r\n"); // hide the uuid from the eventlog and UI view
	fileLogEvent(pmod,Tempstr);
	*uptr=0;
	WSHLoggedError(pmod,Tempstr);
}
void WsocksHostImpl::WSHLogErrorStr(WsocksApp *pmod, const char *str)
{
	// Create a UUID for this error for THE EYE
#ifdef WIN32
	UUID uuid;
	unsigned char *suuid;
	UuidCreateSequential(&uuid);
	UuidToString(&uuid, &suuid);
#else
	unsigned char uuid[16]={0};
	unsigned char *suuid=uuid;
#endif

	char Tempstr[2048]={0},*tptr=Tempstr;
	sprintf(tptr,"%8s:%c%c@%6s | ",pmod->pcfg->aname.c_str(),WSHcDate[6],WSHcDate[7],WSHcTime); tptr+=strlen(tptr);
	strncpy(tptr,str,1900); tptr+=strlen(tptr); char *uptr=tptr;
	sprintf(tptr," {%s}",suuid); tptr+=strlen(tptr);
	strcpy(tptr,"\r\n"); tptr+=2;
	_ASSERT(tptr -Tempstr<2047);
	Tempstr[2047]=0;
#ifdef WIN32
	RpcStringFree(&suuid);
#endif

	fileLogError(pmod,Tempstr);
	strcpy(uptr,"\r\n"); // hide the uuid from the eventlog and UI view
	fileLogEvent(pmod,Tempstr);
	*uptr=0;
	WSHLoggedError(pmod,Tempstr);
}

void WsocksHostImpl::WSHLogEvent(WsocksApp *pmod, const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHLogEventVA(pmod,format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHLogEventVA(WsocksApp *pmod, const char *format, va_list& ptr)
{
	char buf[2048]={0};
	vsnprintf_s ( buf, sizeof(buf), _TRUNCATE, format, ptr ) ;

	char Tempstr[2048]={0},*tptr=Tempstr;
	sprintf(tptr,"%8s:%c%c@%6s | ",pmod->pcfg->aname.c_str(),WSHcDate[6],WSHcDate[7],WSHcTime); tptr+=strlen(tptr);
	strncat(tptr,buf, 2000); tptr+=strlen(tptr);
	strcat(tptr,"\r\n"); tptr+=2;
	_ASSERT(strlen(Tempstr)<2047);
	Tempstr[2047]=0;
	__int64 coff=0;
	fileLogEvent(pmod,Tempstr);
	tptr-=2; *tptr=0;
	WSHLoggedEvent(pmod,Tempstr);
}
void WsocksHostImpl::WSHLogEventStr(WsocksApp *pmod, const char *str)
{
	char Tempstr[2048]={0},*tptr=Tempstr;
	sprintf(tptr,"%8s:%c%c@%6s | ",pmod->pcfg->aname.c_str(),WSHcDate[6],WSHcDate[7],WSHcTime); tptr+=strlen(tptr);
	strncat(tptr,str, 2000); tptr+=strlen(tptr);
	strcat(tptr,"\r\n"); tptr+=2;
	_ASSERT(strlen(Tempstr)<2047);
	Tempstr[2047]=0;
	__int64 coff=0;
	fileLogEvent(pmod,Tempstr);
	tptr-=2; *tptr=0;
	WSHLoggedEvent(pmod,Tempstr);
}

void WsocksHostImpl::WSHLogDebug(WsocksApp *pmod, const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHLogDebugVA(pmod,format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHLogDebugVA(WsocksApp *pmod, const char *format, va_list& ptr)
{
	_ASSERT(false);
	char lpath[MAX_PATH];
#ifdef WIN32
	sprintf(lpath,"%s\\%sdbg.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
#else
	sprintf(lpath,"%s/%sdbg.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
#endif
	pmod->WSDebugLogPath=lpath;
}

void WsocksHostImpl::WSHLogRetry(WsocksApp *pmod, const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHLogRetryVA(pmod,format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHLogRetryVA(WsocksApp *pmod, const char *format, va_list& ptr)
{
	_ASSERT(false);
	char lpath[MAX_PATH];
#ifdef WIN32
	sprintf(lpath,"%s\\%sretry.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
#else
	sprintf(lpath,"%s/%sretry.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
#endif
	pmod->WSRetryLogPath=lpath;
}

void WsocksHostImpl::WSHLogRecover(WsocksApp *pmod, const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHLogRecoverVA(pmod,format,alist);
	va_end(alist);
}
void WsocksHostImpl::WSHLogRecoverVA(WsocksApp *pmod, const char *format, va_list& ptr)
{
	_ASSERT(false);
	char lpath[MAX_PATH];
#ifdef WIN32
	sprintf(lpath,"%s\\%srec.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
#else
	sprintf(lpath,"%s/%srec.txt",pmod->pcfg->LogPath.c_str(),WSHcDate);
#endif
	pmod->WSRecoverLogPath=lpath;
}
#ifdef WSOCKS_SSL
void WsocksHostImpl::SSLLogError(const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHostLogErrorVA(format,alist);
    va_end(alist);
}
void WsocksHostImpl::SSLLogEvent(const char *format, ...)
{
	va_list alist;
	va_start(alist,format);
	WSHostLogEventVA(format,alist);
    va_end(alist);
}
#endif
