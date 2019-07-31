
#include "stdafx.h"
#include "smdlgapi.h"
#include "wstring.h"

#pragma pack(push,8)
typedef struct tdMsgHeader
{
	WORD MsgID;
	WORD MsgLen;
} MSGHEADER;
#pragma pack(pop)

SMDlgProvider::SMDlgProvider()
	:name()
	,dlgapi(0)
	,fpath()
	,dllhnd(0)
	,pGetDlgInterface(0)
	,pFreeDlgInterface(0)
{
}
SMDlgApi::SMDlgApi()
	:ccodec(0)
	,dsnotify(0)
	,pmap()
{
}
// Built-in challenge response to validate versions
int SMDlgProvider::SMDPValidateModule(const char *challenge)
{
	// Last API date
	if(!strcmp(challenge,"SMDLGAPI_DATE"))
		return SMDLGAPI_DATE;
	// App size
	else if(!strcmp(challenge,"SMDlgProvider"))
		return sizeof(SMDlgProvider);
	return -1;
}
// For servers wanting dialog services
int SMDlgApi::InitDlgServer(string oboi, SMDlgServerNotify *dsnotify, SysCmdCodec *ccodec, const char *pdir)
{
	this->oboi=oboi;
	this->dsnotify=dsnotify;
	this->ccodec=ccodec;
	return LoadDlgProviders(pdir);
}
int SMDlgApi::ShutdownDlgServer()
{
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		FreeDlgProvider(provider);
	}
	pmap.clear();
	dsnotify=0;
	ccodec=0;
	return 0;
}
int SMDlgApi::ValidateApp(SMDlgProvider *provider, char reason[256])
{
	int rc=0;
	// Last API date
	if((rc=provider->SMDPValidateModule("SMDLGAPI_DATE"))!=SMDLGAPI_DATE)
	{
		sprintf(reason,"%s failed SMDLGAPI_DATE challenge with %d response (expected %d)!",
			provider->name.c_str(),rc,SMDLGAPI_DATE);
		return -1;
	}
	// App size
	if((rc=provider->SMDPValidateModule("SMDlgProvider"))!=sizeof(SMDlgProvider))
	{
		sprintf(reason,"%s failed SMDlgProvider challenge with %d response (expected %d)!",
			provider->name.c_str(),rc,sizeof(SMDlgProvider));
		return -1;
	}
	return 0;
}
int SMDlgApi::LoadDlgProviders(const char *pdir)
{
	char fmatch[MAX_PATH]={0};
	sprintf(fmatch,"%s\\*.dll",pdir);
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(fmatch,&fdata);
	if(fhnd!=INVALID_HANDLE_VALUE)
	{
		do{
			if((!strcmp(fdata.cFileName,"."))||(!strcmp(fdata.cFileName,"..")))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				;
			else
			{
				char dllpath[MAX_PATH]={0};
				sprintf(dllpath,"%s\\%s",pdir,fdata.cFileName);
				#ifdef _DEBUG
				if(!stristr(fdata.cFileName,"D.dll"))
					continue;
				#else
				if(stristr(fdata.cFileName,"D.dll"))
					continue;
				#endif
				HMODULE dllhnd=LoadLibrary(dllpath);
				if(!dllhnd)
				{
					dsnotify->SMDSLogError("Failed LoadLibrary(%s)--Check dependencies!",dllpath);
					continue;
				}
				GETDLGINTERFACE pGetDlgInterface=(GETDLGINTERFACE)GetProcAddress(dllhnd,SMDLGAPI_EXPORT_FUNC);
				if(!pGetDlgInterface)
				{
					dsnotify->SMDSLogError("Failed GetProcAddress(%s) from %s!",SMDLGAPI_EXPORT_FUNC,dllpath);
					continue;
				}
				SMDlgProvider *provider=pGetDlgInterface(dllhnd,SMDLGAPI_INTERFACE_VERSION,this);
				if(!provider)
				{
					dsnotify->SMDSLogError("Failed GetDlgInterface() from %s!",dllpath);
					continue;
				}
				provider->dllhnd=dllhnd;
				provider->pGetDlgInterface=pGetDlgInterface;
				if(provider->name.empty())
				{
					//FreeDlgProvider(provider);
					FreeLibrary(dllhnd);
					dsnotify->SMDSLogError("Blank provider name from %s!",dllpath);
					continue;
				}
				char reason[256]={0};
				if(ValidateApp(provider,reason)<0)
				{
					// The pointer table might mismatch so don't risk a crash
					//FreeDlgProvider(provider);
					FreeLibrary(dllhnd);
					dsnotify->SMDSLogError("SMDPValidateModule(%s) failed: %s!",provider->name.c_str(),reason);
					continue;
				}
				pmap[provider->name]=provider;
			}
		}while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
	return 0;
}
int SMDlgApi::FreeDlgProvider(SMDlgProvider *provider)
{
	provider->SMDPDisconnected(dsnotify);
	HMODULE dllhnd=provider->dllhnd;
	if(provider->pFreeDlgInterface)
	{
		FREEDLGINTERFACE pFreeDlgInterface=provider->pFreeDlgInterface;
		pFreeDlgInterface(provider);
	}
	if(dllhnd)
		FreeLibrary(dllhnd);
	return 0;
}
int SMDlgApi::FindDlgProvider(string aclass, string aname, string apath, string cmd)
{
	if(!dsnotify)
		return -1;
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,cmd))
			return provider->SMDPOpenDialog(dsnotify,oboi,apath,cmd);
	}
	return -1;
}
int SMDlgApi::NotifyResponse(string apath, string cmd, const char *resp, int rlen)
{
	if(!dsnotify)
		return -1;
	char aclass[256]={0},aname[256]={0};
	const char *optr=strrchr(apath.c_str(),'/');
	if(!optr) optr=apath.c_str();
    else optr++;
	strcpy(aclass,optr);
	char *aptr=strchr(aclass,'.');
	if(aptr)
	{
		strcpy(aname,aptr +1); *aptr=0;
	}
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,cmd))
		{
			provider->SMDPNotifyResponse(dsnotify,apath,cmd,resp,rlen);
			return 0;
		}
	}
	return -1;
}
int SMDlgApi::NotifyFile(string apath, int rc, string fkey, const char *fpath)
{
	if(!dsnotify)
		return -1;
	char aclass[256]={0},aname[256]={0};
	const char *optr=strrchr(apath.c_str(),'/');
	if(!optr) optr=apath.c_str();
    else optr++;
	strcpy(aclass,optr);
	char *aptr=strchr(aclass,'.');
	if(aptr)
	{
		strcpy(aname,aptr +1); *aptr=0;
	}
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,"downloadfile"))
			return provider->SMDPNotifyFile(dsnotify,apath,rc,fkey,fpath);
	}
	return -1;
}
int SMDlgApi::NotifyDisconnected(string apath, string cmd)
{
	if(!dsnotify)
		return -1;
	char aclass[256]={0},aname[256]={0};
	const char *optr=strrchr(apath.c_str(),'/');
	if(!optr) optr=apath.c_str();
    else optr++;
	strcpy(aclass,optr);
	char *aptr=strchr(aclass,'.');
	if(aptr)
	{
		strcpy(aname,aptr +1); *aptr=0;
	}
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,cmd))
			return provider->SMDPDisconnected(dsnotify);
	}
	return -1;
}

int SMDlgApi::FindDlgProvider(string aclass, string aname, int msgid)
{
	if(!dsnotify)
		return -1;
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,msgid))
			return provider->SMDPOpenDialog(dsnotify,aclass,aname,msgid);
	}
	return -1;
}
int SMDlgApi::NotifyMsg(string apath, MSGHEADER *MsgHeader, char *Msg)
{
	if(!dsnotify)
		return -1;
	char aclass[256]={0},aname[256]={0};
	const char *optr=strrchr(apath.c_str(),'/');
	if(!optr) optr=apath.c_str();
    else optr++;
	strcpy(aclass,optr);
	char *aptr=strchr(aclass,'.');
	if(aptr)
	{
		strcpy(aname,aptr +1); *aptr=0;
	}
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,MsgHeader->MsgID))
			return provider->SMDPNotifyMsg(dsnotify,aclass,aname,MsgHeader,Msg);
	}
	return -1;
}
int SMDlgApi::NotifyDisconnected(string aclass, string aname, int msgid)
{
	if(!dsnotify)
		return -1;
	for(PROVDERMAP::iterator pit=pmap.begin();pit!=pmap.end();pit++)
	{
		SMDlgProvider *provider=pit->second;
		if(provider->SMDPIsProvider(aclass,aname,msgid))
			return provider->SMDPDisconnected(dsnotify);
	}
	return -1;
}
