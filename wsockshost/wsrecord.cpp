
#include "stdafx.h"
#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#if defined(WIN32)||defined(_CONSOLE)
#include <shlwapi.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif

int WsocksHostImpl::WSHOpenRecording(WsocksApp *pmod, WSRECORDING *wsr, HWND parent, int Type, int PortNo)
{
	if (!wsr)
		return -1;
	const char *tstr="";
	switch ( Type )
	{
	case WS_CON: tstr="CON"; break;
	case WS_CGD: tstr="CGD"; break;
	case WS_USR: tstr="USR"; break;
	case WS_UGR: tstr="UGR"; break;
	case WS_FIL: tstr="FIL"; break;
	case WS_CTI: tstr="CTI"; break;
	case WS_CTO: tstr="CTO"; break;
	}

	//sprintf(wsr->RecordPath, "logs/%s%d", tstr, PortNo);
	CreateDirectory("TCP_Recordings",0);
	char udesc[MAX_PATH]={0};
	switch ( Type )
	{
	case WS_CON: 
	{
		sprintf(udesc,"CON%03d-%06d-%s",PortNo,WSHTime,pmod->ConPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
	case WS_USR: 
	{
		sprintf(udesc,"USR%03d-%06d-%s",PortNo,WSHTime,pmod->UsrPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
#ifdef WS_FILE_SERVER
	case WS_FIL: 
	{
		sprintf(udesc,"FIL%03d-%06d-%s",PortNo,WSHTime,pmod->FilePort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
#endif
#ifdef WS_MONITOR
	case WS_CGD: 
	{
		sprintf(udesc,"CGD%03d-%06d-%s",PortNo,WSHTime,pmod->CgdPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
	case WS_UMR: 
	{
		sprintf(udesc,"UMR%03d-%06d-%s",PortNo,WSHTime,pmod->UmrPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
#endif
#ifdef WS_GUARANTEED
	case WS_UGR: 
	{
		sprintf(udesc,"UGR%03d-%06d-%s",PortNo,WSHTime,pmod->UgrPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
#endif
#ifdef WS_CAST
	case WS_CTI:
	{
		sprintf(udesc,"CTI%03d-%06d-%s",PortNo,WSHTime,pmod->CtiPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
	case WS_CTO:
	{
		sprintf(udesc,"CTO%03d-%06d-%s",PortNo,WSHTime,pmod->CtoPort[PortNo].RemoteIP);
		for(char *uptr=udesc +14;*uptr;uptr++)
		{
			if(*uptr=='.')
				*uptr='_';
		}
		#ifdef WIN32
		sprintf(wsr->RecordPath,"TCP_Recordings\\%s",udesc);
		#else
		sprintf(wsr->RecordPath,"TCP_Recordings/%s",udesc);
		#endif
		break;
	}
#endif
	default:
		#ifdef WIN32
		sprintf(wsr->RecordPath, "TCP_Recordings\\%s%d", tstr, PortNo);
		#else
		sprintf(wsr->RecordPath, "TCP_Recordings/%s%d", tstr, PortNo);
		#endif
	};

	char *eptr=strrchr(wsr->RecordPath,'.');
	if((eptr)&&(_stricmp(eptr,".OUT")==0||_stricmp(eptr,".IN")==0))
		*eptr=0;

	char opath[MAX_PATH];
	sprintf(opath, "%s.OUT", wsr->RecordPath);
	char ipath[MAX_PATH];
	sprintf(ipath, "%s.IN", wsr->RecordPath);
	wsr->DoRec=FALSE;
	if((parent)&&(PathFileExists(opath)||PathFileExists(ipath)))
	{
	//	char Tempstr[1024];
	//#ifdef WIN32
	//	sprintf(Tempstr,"Overwrite \"%s\" and \"%s\"?",opath,ipath);
	//	if(MessageBox(parent,Tempstr,"Confirm Overwrite",MB_ICONQUESTION|MB_YESNO)==IDYES)
	//		wsr->DoRec=TRUE;
	//#else
	//	sprintf(Tempstr,"Overwrite \"%s\" and \"%s\"? y/n",opath,ipath);
	//	printf(Tempstr);
	//	unsigned ch=getchar(); printf("%c\n",ch);
	//	if((ch=='Y')||(ch=='y'))
	//		wsr->DoRec=TRUE;
	//#endif
		WSHLogEvent(pmod,"Overwriting existing files \"%s\" and \"%s\"...",opath,ipath);
		wsr->DoRec=TRUE;
	}
	else
		wsr->DoRec=TRUE;
	if(wsr->DoRec)
	{
		wsr->RecOutHnd=fopen(opath,"wb");
		if(!wsr->RecOutHnd)
		{
			wsr->DoRec=FALSE;
			WSHLogError(pmod,"!WSOCKS: %s%d Failed opening (%s) for recording!",tstr,PortNo,opath);
		}
		else
			WSHLogEvent(pmod,"!WSOCKS: %s%d Recording sends to (%s)...",tstr,PortNo,opath);

		wsr->RecInHnd=fopen(ipath,"wb");
		if(!wsr->RecInHnd)
		{
			fclose(wsr->RecOutHnd); 
			wsr->RecOutHnd=0;
			wsr->DoRec=FALSE;
			WSHLogError(pmod,"!WSOCKS: %s%d Failed opening (%s) for recording!",tstr,PortNo,ipath);
		}
		else
			WSHLogEvent(pmod,"!WSOCKS: %s%d Recording recvs to (%s)...",tstr,PortNo,ipath);
	}
	return 0;
}

void WsocksHostImpl::WSHCloseRecording(WsocksApp *pmod, WSRECORDING *wsr, int Type, int PortNo)
{
	if (!wsr)
		return;
	const char *tstr="";
	switch ( Type )
	{
	case WS_CON: tstr="CON"; break;
	case WS_CGD: tstr="CGD"; break;
	case WS_USR: tstr="USR"; break;
	case WS_UGR: tstr="UGR"; break;
	case WS_FIL: tstr="FIL"; break;
	case WS_CTI: tstr="CTI"; break;
	case WS_CTO: tstr="CTO"; break;
	}

	if(wsr->DoRec)
	{
		wsr->DoRec=FALSE;
		if(wsr->RecOutHnd)
		{
			fclose(wsr->RecOutHnd);
			wsr->RecOutHnd=0;
		}
		if(wsr->RecInHnd)
		{
			fclose(wsr->RecInHnd);
			wsr->RecInHnd=0;
		}
		WSHLogEvent(pmod,"!WSOCKS: %s%d Recording Stopped.",tstr,PortNo);
	}
}

int WsocksHostImpl::WSHRecord(WsocksApp *pmod, WSRECORDING *wsr, const char *buf, int len, BOOL send)
{
	if (!wsr||!wsr->DoRec)
		return -1;
	if (send)
	{
		if (!wsr->RecOutHnd)
			return -1;
		if ((int)fwrite(buf,1,len,wsr->RecOutHnd)!=len)
			return -1;
	}
	else
	{
		if (!wsr->RecInHnd)
			return -1;
		if ((int)fwrite(buf,1,len,wsr->RecInHnd)!=len)
			return -1;
	}
	return 0;
}

int WsocksHostImpl::WSHCleanRecordings(WsocksApp *pmod, int ndays, const char *extlist, BOOL prompt)
{
	time_t tnow=time(0);
	struct tm ltm=*localtime(&tnow);
	ltm.tm_mday-=ndays;
	mktime(&ltm);
	int ddate=((ltm.tm_year +1900)*10000) +((ltm.tm_mon +1)*100) + (ltm.tm_mday);
	char wmsg[256]={0};
	sprintf(wmsg,"Confirm cleaning up TCP_Recordings on or before %08d?",ddate);
#if defined(WIN32)||defined(_CONSOLE)
	if((prompt)&&(MessageBox(pmod->WShWnd,wmsg,"Confirm Cleanup",MB_ICONQUESTION|MB_YESNO)!=IDYES))
		return -1;
	WSHLogEvent(pmod,"Cleaning up TCP_Recordings on or before %08d...",ddate);
	int ndel=0;
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
				int fdate=(ftsys.wYear*10000) +(ftsys.wMonth*100) +(ftsys.wDay);
				char *ext=strrchr(fdata.cFileName,'.');
				bool extmatch=false;
				if(ext)
				{
					if(extlist)
					{
						for(const char *estr=extlist;*estr;estr+=strlen(estr) +1)
						{
							if(!_stricmp(ext,estr))
							{
								extmatch=true;
								break;
							}
						}
					}
					else
					{
						if((!_stricmp(ext,".IN")||!_stricmp(ext,".OUT")||!_stricmp(ext,".RPT")||!_stricmp(ext,".DMP")))
							extmatch=true;
					}
				}
				if((extmatch)&&(fdate<=ddate))
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
			}
		}while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
	WSHLogEvent(pmod,"Cleaned up %d files",ndel);

	WSHCleanThreadReports(pmod,ndays,1024);
#else//!WIN32
	WSHLogEvent(pmod,"Cleaning up TCP_Recordings on or before %08d...",ddate);
	int ndel=0;
	DIR *pdir=opendir("TCP_Recordings");
	if(pdir)
	{
		dirent *fdata=0;
		do{
			fdata=readdir(pdir);
			if(fdata)
			{
				if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
					continue;
				int fdate=0;
				struct stat fst;
				stat(fdata->d_name,&fst);
				struct tm *ltm=localtime(&fst.st_ctime);
				if(ltm)
					fdate=(1900 +ltm->tm_year)*10000 +(ltm->tm_mon+1)*100 +ltm->tm_mday;
				char *ext=strrchr(fdata->d_name,'.');
				if((ext)&&(!_stricmp(ext,".IN")||!_stricmp(ext,".OUT")||!_stricmp(ext,".RPT"))&&(fdate<=ddate))
				{
					char fpath[MAX_PATH]={0};
					sprintf(fpath,"TCP_Recordings/%s",fdata->d_name);
					unlink(fpath);
					if(!PathFileExists(fpath))
					{
						ndel++;
						WSHLogEvent(pmod,"Deleted (%s), last written %08d",fdata->d_name,fdate);
					}
					else
						WSHLogError(pmod,"Failed deleting (%s)",fdata->d_name);
				}
			}
		}while(fdata);
		closedir(pdir);
	}
	WSHLogEvent(pmod,"Cleaned up %d files",ndel);
#endif//!WIN32
	return 0;
}

