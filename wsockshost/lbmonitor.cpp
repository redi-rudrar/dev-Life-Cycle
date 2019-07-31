
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#include "ps.h"
#include "wstring.h"
#ifdef WIN32
#include <shlwapi.h>
#endif

#pragma pack(push,8)
typedef struct tdMsg1104Rec //Monitor commands
{
	int cmd;
	int Date;
	int Time;
	union{
		char MonitorGroups[8][16]; // cmd=0x01
		int  NoArg; // cmd=0x02 (use 1105)
		struct { // cmd=0x03
			int PortType;
			int PortNo;
			char ResetGroup[16];
		} ResetInfo;
		struct { // cmd=0x04,0x08,0x0A
			int PortType;
			int PortNo;
			int Len; // Var len data follows after 1104, not after len (0x04 only)
			BOOL Reset;
		} UpdateInfo;
		DWORD UpdateInterval; // cmd=0x05
		struct {
			BOOL LogError;
			BOOL LogWarn;
			BOOL LogDebug;
			BOOL ErrorHist;
			BOOL WarnHist;
			BOOL DebugHist;
		} LogInfo;
		struct { // cmd=0x09
			char User[16];
		} LoginInfo;
		int  OnOff; // cme=0x0C
		char Reserved[256];
	};
} MSG1104REC;

typedef struct tdMsg1105Rec //Generic Monitor Data
{
	int cmd; // 0x00(notify),0x01(nosupp),0x02(frame),0x03(closed),0x04(mem usage),0x05(disk&mem)
	int PortType;
	int PortNo;
	int Date;
	int Time;
	char BuildDate[12];
	char BuildTime[9];
	int Len;
	// Variable length
} MSG1105REC;
#pragma pack(pop)

int LBMonitor::nextLBMonPort=8010;
LBMonitor::LBMonitor()
	:taskmgr()
	,proxyAuthPort(-1)
#ifdef FIX_TASK_CALL
	,elist()
#endif
{
	memset(uuid,0,sizeof(uuid));
#ifdef FIX_TASK_CALL
	InitializeCriticalSection(&emutex);
#endif
}
LBMonitor::~LBMonitor()
{
#ifdef FIX_TASK_CALL
	for(list<char*>::iterator eit=elist.begin();eit!=elist.end();eit++)
		delete *eit;
	elist.clear();
	DeleteCriticalSection(&emutex);
#endif
}

int LBMonitor::WSHInitModule(AppConfig *config, class WsocksHost *apphost)
{
	NO_OF_CON_PORTS=64;
	NO_OF_CTO_PORTS=16;
	NO_OF_UMC_PORTS=16;
	NO_OF_UMR_PORTS=32;
#ifdef WIN32
	UuidCreate((UUID*)uuid);
#endif
	if(WSInitSocks()<0)
		return -1;
	WSSetAppHead((char*)pcfg->aname.c_str());
	if(taskmgr.Startup(NO_OF_UMR_PORTS,8)<0)
		return -1;
	if(!ccodec)
	{
		ccodec=new SysCmdCodec;
		if(ccodec->Init()<0)
			return -1;
	}
	if(!fcodec)
	{
		fcodec=new SysFeedCodec;
		if(fcodec->Init()<0)
			return -1;
	}
	scmds.Init(this,ccodec/*,NO_OF_UMR_PORTS*/);
	return 0;
}
int LBMonitor::WSHCleanupModule()
{
	scmds.Shutdown();
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		WsocksApp *pmod=(WsocksApp*)ConPort[i].DetPtr;
		if(!pmod)
			continue;
		ConPort[i].DetPtr=0;
		LBMAppCloseSocks(pmod);
	}
	WSCloseSocks();
	if(ccodec)
	{
		delete ccodec; ccodec=0;
	}
	if(fcodec)
	{
		delete fcodec; fcodec=0;
	}
	taskmgr.Shutdown();
	return 0;
}

int LBMonitor::WSPortsCfg()
{
	// Broadcast CTO port
	WSCreatePort(WS_CTO,0);
	strcpy(CtoPort[0].LocalIP,"AUTO");
	//strcpy(CtoPort[0].RemoteIP,"224.2.1.15");
	strcpy(CtoPort[0].RemoteIP,"255.255.255.255");
	sprintf(CtoPort[0].CfgNote,"Sysmon");
	CtoPort[0].Port=8001;
	CtoPort[0].BlockSize=WS_MAX_BLOCK_SIZE;
	CtoPort[0].ConnectHold=true;

	// Loopback UMC port
	WSCreatePort(WS_UMC,1);
	strcpy(UmcPort[0].LocalIP,"127.0.0.1");
	UmcPort[0].Port=0;
	strcpy(UmcPort[0].CfgNote,"LBConsole");
	UmcPort[0].BlockSize=WS_MAX_BLOCK_SIZE;
	UmcPort[0].Compressed=false;
#ifdef WS_REALTIMESEND
	UmcPort[0].ImmediateSendLimit=UmcPort[0].BlockSize*1024;
#endif

	// Dynamic UMC port
	WSCreatePort(WS_UMC,0);
	UmcPort[1].Port=0;
	char iface[32]={0};
	strcpy(iface,pcfg->sysmonAddr.c_str());
	char *pstr=strchr(iface,':');
	if(pstr)
	{
		*pstr=0; UmcPort[1].Port=atoi(pstr +1);
	}
	strcpy(UmcPort[1].LocalIP,iface);
	// Limit dynamic range from [16000,17000).
	if(!UmcPort[1].Port)
	{
		//// We'll have to bind ourself until we find an open one
		//for(int bport=16000;bport<17000;bport++)
		//{
		//	if(phost->WSHIsPortAvailable(bport))
		//	{
		//		UmcPort[1].Port=bport;
		//		break;
		//	}
		//}
		// Better mutual port exclusion when two apps start at the same time on the same machine
		UmcPort[1].dynPortBegin=16000;
		UmcPort[1].dynPortEnd=17000;
	}
	strcpy(UmcPort[1].CfgNote,"Sysmon");
	UmcPort[1].BlockSize=WS_MAX_BLOCK_SIZE;
	UmcPort[1].Compressed=true;
#ifdef WS_REALTIMESEND
	UmcPort[1].ImmediateSendLimit=UmcPort[1].BlockSize*1024;
#endif
	UmcPort[1].ConnectHold=true;
	return 1;
}
void LBMonitor::WSConOpened(int PortNo)
{
	#ifdef WIN32
	WsocksApp *pmod=(WsocksApp*)(PTRCAST)ConPort[PortNo].BounceEnd;
	#else
	WsocksApp *pmod=(WsocksApp*)ConPort[PortNo].sDetPtr;
	#endif
	if(pmod)
	{
		WSLogEvent("CON%d connected to %s:%d for (%s)(%s)...",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port,
			pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
		ConPort[PortNo].DetPtr=pmod;
		char *apath=new char[pmod->pcfg->aclass.length() +1 +pmod->pcfg->aname.length() +1];
		sprintf(apath,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
		ConPort[PortNo].DetPtr4=apath;

		DWORD& rid=(DWORD&)ConPort[PortNo].DetPtr5;
		char mbuf[1024]={0},*mptr=mbuf;
		char parms[256]={0};
		sprintf(parms,"User=%s.%s|Pass=|",pcfg->aclass.c_str(),pcfg->aname.c_str());
		ccodec->Encode(mptr,sizeof(mbuf),true,++rid,"login",parms,0);
		WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,PortNo);

		// Tell clients about new apps
		for(int i=0;i<NO_OF_UMR_PORTS;i++)
		{
			if(!UmrPort[i].SockActive)
				continue;
			//SysmonFlags *umflags=(SysmonFlags*)UmrPort[PortNo].DetPtr3;
			FeedUser *fuser=(FeedUser *)UmrPort[i].DetPtr3;
			if(!fuser)
				continue;
			SysmonFlags *umflags=(SysmonFlags*)fuser->DetPtr;
			if((umflags)&&(umflags->appListRid))
			{
				char resp[1024]={0},*rptr=resp,parms[2048]={0};
				int rlen=sizeof(resp);
				sprintf(parms,"AppClass=%s|AppName=%s|AppPath=%s|SockConnected=Y|",
					pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),apath); 
				rptr=resp; rlen=sizeof(resp);
				ccodec->Encode(rptr,rlen,false,umflags->appListRid,"applist",parms,0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,i);
			}
		}
	}
	else
		WSLogEvent("CON%d connected to %s:%d)...",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
}
int LBMonitor::WSConMsgReady(int PortNo)
{
	if(ConPort[PortNo].InBuffer.Size<1)
		return 0;
	if(ConPort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
		return 0;
	// Apps that support loopback should strip the msg before handling
	// because any new reply will be handled inline
	MSGHEADER MsgHeader;
	memcpy(&MsgHeader,ConPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(ConPort[PortNo].InBuffer.Size<sizeof(MSGHEADER)+MsgHeader.MsgLen)
		return 0;
	char *Msg=new char[MsgHeader.MsgLen];
	memcpy(Msg,ConPort[PortNo].InBuffer.Block +sizeof(MSGHEADER),MsgHeader.MsgLen);
	if(!WSStripBuff(&ConPort[PortNo].InBuffer,sizeof(MSGHEADER) +MsgHeader.MsgLen))
	{
		WSCloseConPort(PortNo);
		return 0;
	}
	TranslateConMsg(&MsgHeader,Msg,PortNo);
	delete Msg;
	//ConPort[PortNo].DetPtr2=(void*)(sizeof(MSGHEADER)+MsgHeader.MsgLen);
	return 1;
}
int LBMonitor::WSConStripMsg(int PortNo)
{
	int size=(int)(PTRCAST)ConPort[PortNo].DetPtr2;
	while(size>0)
	{
		int bsize=size;
		if(bsize>ConPort[PortNo].BlockSize*1024)
			bsize=ConPort[PortNo].BlockSize*1024;
		if(!WSStripBuff(&ConPort[PortNo].InBuffer,bsize))
		{
			WSCloseConPort(PortNo);
			return -1;
		}
		size-=bsize;
	}
	return 0;
}
int LBMonitor::TranslateConMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	switch(MsgHeader->MsgID)
	{
	case 16:
		ConPort[PortNo].BeatsIn++;
		// Pass through to all clients
		for(int i=0;i<NO_OF_UMR_PORTS;i++)
		{
			if(!UmrPort[i].SockActive)
				continue;
			UmrPort[i].BeatsOut++;
			WSUmrSendMsg(16,MsgHeader->MsgLen,Msg,i);
		}
		break;
	// New sysmon response
	case 1109:
	{
		const char *mend=Msg +MsgHeader->MsgLen;
		while(Msg<mend)
		{
			if(ccodec->Decode((const char *&)Msg,(int)(mend -Msg),false,this,(void*)(PTRCAST)MAKELONG(PortNo,WS_CON))<0)
				return -1;
		}
		break;
	}
	// New sysmon feed
	case 1110:
	{
		// Don't fully decode the whole feed message; just check the header to determine the task to fire
		const char *mptr=Msg,*mend=Msg +MsgHeader->MsgLen;
		int fid=0;
		bool hist=false;
		char aclass[256]={0},aname[256]={0};
		fcodec->DecodeProxy(mptr,MsgHeader->MsgLen,fid,hist,aclass,sizeof(aclass),aname,sizeof(aname));
		SysmonTasks task=SMTASK_UNKNOWN;
		switch(fid)
		{
		case SMFNO_LVL1: task=hist ?SMTASK_LEVEL1_HIST :SMTASK_LEVEL1; break;
		case SMFNO_LVL2: task=hist ?SMTASK_LEVEL2_HIST :SMTASK_LEVEL2; break;
		case SMFNO_ERRORLOG: task=hist ?SMTASK_ERRORLOG_HIST :SMTASK_ERRORLOG; break;
		case SMFNO_EVENTLOG: task=hist ?SMTASK_EVENTLOG_HIST :SMTASK_EVENTLOG; break;
		case SMFNO_TRADEMON: task=hist ?SMTASK_TRADEMON_HIST :SMTASK_TRADEMON; break;
		case SMFNO_CUSTOM: task=hist ?SMTASK_CUSTOM_HIST :SMTASK_CUSTOM; break;
		default: return -1;
		};
		char skey[256]={0};
		sprintf(skey,"%s.%s",aclass,aname);
		_strupr(skey);
		SMFeedHint fhint={Msg,MsgHeader->MsgLen};
		taskmgr.CallTasks(skey,task,&fhint);
		break;
	}
	// File download [rid(4)|dlen(2)|deliverto(str)|blen(2)|onbehalfof(str)]|file data
	case 1111:
	{
		// Proxy header
		const char *mptr=Msg,*mend=Msg +MsgHeader->MsgLen;
		if(mend -mptr<4)
			return -1;
		int reqid=0;
		memcpy(&reqid,mptr,4); mptr+=4;
		if(mend -mptr<2)
			return -1;
		WORD dlen=0;
		memcpy(&dlen,mptr,2); mptr+=2;
		if(mend -mptr<dlen)
			return -1;
		const char *dtid=mptr; mptr+=dlen;
		if(mend -mptr<2)
			return -1;
		WORD blen=0;
		memcpy(&blen,mptr,2); mptr+=2;
		if(mend -mptr<blen)
			return -1;
		const char *oboi=mptr; mptr+=blen;	
		char *uptr=(char*)strrchr(dtid,'/');
        if(uptr)
        {
            for(uptr--; uptr > dtid; uptr--)
                if(uptr[0] == '/')
                    break;
        }
		if((uptr)&&(!strncmp(uptr,"/LBM.",5)))
		{
			*uptr=0; // Fast removal of "/LBM.#/LBSysmon.LBSysmon" from DeliverTo
			int UmrPortNo=atoi(uptr+5);
			if((UmrPortNo>=0)&&(UmrPortNo<NO_OF_UMR_PORTS)&&(UmrPort[UmrPortNo].SockActive))
			{
				FeedUser *fuser=(FeedUser*)UmrPort[UmrPortNo].DetPtr3;
				if(!fuser)
					break;
				SysmonFlags *umflags=(SysmonFlags *)fuser->DetPtr;
				if(!umflags)
					break;
				//// Drop file transfers from old connections
				//if(reqid<umflags->minRid)
				//	break;
				WSUmrSendMsg(1111,MsgHeader->MsgLen,Msg,UmrPortNo);
			}
		}
		break;
	}
	};
	return 0;
}
int LBMonitor::WSBeforeConSend(int PortNo)
{
	return 0;
}
void LBMonitor::WSConClosed(int PortNo)
{
	WSLogEvent("CON%d disconnected from %s:%d.",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
	SysmonFlags *cmflags=(SysmonFlags*)ConPort[PortNo].DetPtr3;
	if(cmflags)
	{
		delete cmflags; ConPort[PortNo].DetPtr3=0;
		// Remove feed requests for the closed app
		for(int i=0;i<NO_OF_UMR_PORTS;i++)
		{
			if(!UmrPort[i].SockActive)
				continue;
			SysmonFlags *umflags=(SysmonFlags*)UmrPort[i].DetPtr3;
			//if(umflags)
			//{
			//	for(SMFEEDMAP::iterator fit=umflags->feedMap.begin();fit!=umflags->feedMap.end();)
			//	{
			//		int ConPortNo=fit->first;
			//		if(ConPortNo==PortNo)
			//			#ifdef WIN32
			//			fit=umflags->feedMap.erase(fit);
			//			#else
			//			umflags->feedMap.erase(fit++);
			//			#endif
			//		else
			//			fit++;
			//	}
			//}
		}
	}

	char *apath=(char *)ConPort[PortNo].DetPtr4;
	if(apath)
	{
		char *aname=strchr(apath,'.'),*aclass=apath;
		if(aname)
		{
			*aname=0; aname++;
		}
		// Tell clients about closed apps
		for(int i=0;i<NO_OF_UMR_PORTS;i++)
		{
			if(!UmrPort[i].SockActive)
				continue;
			FeedUser *fuser=(FeedUser *)UmrPort[i].DetPtr3;
			if(!fuser)
				continue;
			SysmonFlags *umflags=(SysmonFlags*)fuser->DetPtr;
			if((umflags)&&(umflags->appListRid))
			{
				char parms[1024]={0},resp[1024]={0},*rptr=resp;
				int rlen=sizeof(resp);
				sprintf(parms,"AppClass=%s|AppName=%s|AppPath=%s.%s|SockConnected=N|",
					aclass,aname,aclass,aname); 
				rptr=resp; rlen=sizeof(resp);
				ccodec->Encode(rptr,rlen,false,umflags->appListRid,"applist",parms,0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,i);
			}
		}
		delete apath; ConPort[PortNo].DetPtr4=0;
	}
}

void LBMonitor::WSUmrOpened(int PortNo)
{
	WSLogEvent("UMR%d accepted from %s...",PortNo,UmrPort[PortNo].RemoteIP);
	FeedUser *fuser=taskmgr.AddUser(WS_UMR,PortNo);
	if(fuser)
	{
		SysmonFlags *umflags=new SysmonFlags;
		umflags->pmod=this;
		fuser->DetPtr=umflags;
		fuser->waitAuth=true;
		fuser->authed=false;
		UmrPort[PortNo].DetPtr3=fuser;
	}
}
int LBMonitor::WSUmrMsgReady(int PortNo)
{
	if(UmrPort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
		return 0;
	MSGHEADER MsgHeader;
	memcpy(&MsgHeader,UmrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(UmrPort[PortNo].InBuffer.Size<sizeof(MSGHEADER)+MsgHeader.MsgLen)
		return 0;
	UmrPort[PortNo].PacketsIn++;
	// Apps that support loopback should strip the msg before handling
	// because any new reply will be handled inline
	char *mcpy=new char[sizeof(MSGHEADER) +MsgHeader.MsgLen];
	memcpy(mcpy,UmrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER) +MsgHeader.MsgLen);
	if(!WSStripBuff(&UmrPort[PortNo].InBuffer,sizeof(MSGHEADER) +MsgHeader.MsgLen))
	{
		WSCloseConPort(PortNo);
		return 0;
	}
	TranslateUmrMsg((MSGHEADER*)mcpy,mcpy +sizeof(MSGHEADER),PortNo);
	delete mcpy;
	//UmrPort[PortNo].DetPtr2=(void*)(sizeof(MSGHEADER)+MsgHeader.MsgLen);
	return 1;
}
int LBMonitor::WSUmrStripMsg(int PortNo)
{
	return 0;
}
int LBMonitor::TranslateUmrMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	FeedUser *fuser=(FeedUser*)UmrPort[PortNo].DetPtr3;
	SysmonFlags *smflags=0;
	if(fuser)
		smflags=(SysmonFlags *)fuser->DetPtr;
	switch(MsgHeader->MsgID)
	{
	case 16:
		UmrPort[PortNo].BeatsIn++;
		break;
	// New sysmon requests
	case 1108:
	{
		const char *mend=Msg +MsgHeader->MsgLen;
		while(Msg<mend)
		{
			if(ccodec->Decode((const char *&)Msg,(int)(mend -Msg),true,this,(void*)(PTRCAST)MAKELONG(PortNo,WS_UMR))<0)
				return -1;
		}
		break;
	}
	// File upload
	case 1111:
	{
		if(!fuser->authed)
			break;
		// Proxy header
		const char *mptr=Msg,*mend=Msg +MsgHeader->MsgLen;
		if(mend -mptr<4)
			return -1;
		int rid=0;
		memcpy(&rid,mptr,4); mptr+=4;
		if(mend -mptr<2)
			return -1;
		WORD dlen=0;
		memcpy(&dlen,mptr,2); mptr+=2;
		if(mend -mptr<dlen)
			return -1;
		char dtid[256]={0};
		memcpy(dtid,mptr,dlen); mptr+=dlen;
		if(mend -mptr<2)
			return -1;
		WORD blen=0;
		memcpy(&blen,mptr,2); mptr+=2;
		if(mend -mptr<blen)
			return -1;
		char oboi[256]={0};
		memcpy(oboi,mptr,blen); mptr+=blen;
		int hlen=(int)(mptr -Msg);
		// Find the target app
		for(int i=0;i<NO_OF_CON_PORTS;i++)
		{
			if(!ConPort[i].SockConnected)
				continue;
			WsocksApp *pmod=(WsocksApp*)ConPort[i].DetPtr;
			if(!pmod)
				continue;
			char skey[256]={0};
			sprintf(skey,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
			if(!strcmp(dtid,skey))
			{
				// Append LBM.# to the OnBehalfOf
				// TODO: Come up with a faster way to do this
				char nhead[512]={0},*nptr=nhead;
                sprintf(oboi +strlen(oboi),"/LBM.%d/%s.%s",PortNo,pcfg->aclass.c_str(),pcfg->aname.c_str());
				memcpy(nptr,Msg,hlen -blen -2); nptr+=hlen -blen -2;
				blen=(int)strlen(oboi)+1;
				memcpy(nptr,&blen,2); nptr+=2;
				memcpy(nptr,oboi,blen); nptr+=blen;
				int nhlen=(int)(nptr -nhead),dlen=(int)(mend -mptr);
				char *nmsg=new char[nhlen +dlen];
				memcpy(nmsg,nhead,nhlen);
				memcpy(nmsg +nhlen,mptr,dlen);
				char fkey[256]={0};
				char fpath[MAX_PATH]={0};
				pmod->WSRecvFile(nmsg,nhlen +dlen,WS_UMR,0, // LBMonitor is connected to UMR0 port in application side.
					&rid,fkey,sizeof(fkey),fpath,MAX_PATH,dtid,sizeof(dtid),oboi,sizeof(oboi));
				delete nmsg;
				break;
			}
		}
		break;
	}
	// Proxy authorize response
	case 1113:
	{
		const char *mend=Msg +MsgHeader->MsgLen;
		while(Msg<mend)
		{
			if(ccodec->Decode((const char *&)Msg,(int)(mend -Msg),false,this,(void*)(PTRCAST)MAKELONG(PortNo,WS_UMR))<0)
				return -1;
		}
		break;
	}
	};
	return 0;
}
int LBMonitor::WSBeforeUmrSend(int PortNo)
{
	return 0;
}
void LBMonitor::WSUmrClosed(int PortNo)
{
	WSLogEvent("UMR%d disconnected from %s.",PortNo,UmrPort[PortNo].RemoteIP);
	FeedUser *fuser=(FeedUser*)UmrPort[PortNo].DetPtr3;
	if(fuser)
	{
		if(!strncmp(fuser->user.c_str(),"smproxy.",8))
		{
			proxyAuthPort=-1;
			WSLogEvent("UMR%d: lost proxy authorizer",PortNo);
			// Find another smproxy to proxy authorize
			for(int i=0;i<NO_OF_UMR_PORTS;i++)
			{
				if(!UmrPort[i].SockActive)
					continue;
				FeedUser *suser=(FeedUser*)UmrPort[i].DetPtr3;
				if((suser)&&(!strncmp(suser->user.c_str(),"smproxy.",8)))
				{
					proxyAuthPort=i;
					WSLogEvent("UMR%d: designated as proxy authorizer",i);
					break;
				}
			}
		}
		SysmonFlags *smflags=(SysmonFlags *)fuser->DetPtr;
		if(smflags)
		{
			delete smflags; fuser->DetPtr=0;
		}
		taskmgr.RemUser(WS_UMR,PortNo);
	}
}

int LBMonitor::EnumNICs(list<string>& nics)
{
	char fpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(fpath,"%s\\enumnics.txt",pcfg->RunPath.c_str());
	#else
	sprintf(fpath,"%s/enumnics.txt",pcfg->RunPath.c_str());
	#endif
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		char cmd[256]={0};
		sprintf(cmd,"ipconfig /all > %s",fpath);
		system(cmd);
		fp=fopen(fpath,"rt");
		if(!fp)
			return -1;
	}
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		if(strstr(rbuf,"IP Address. . . . . . . . . . . . :")||
		   strstr(rbuf,"IPv4 Address. . . . . . . . . . . :")||
		   strstr(rbuf,"IPv6 Address. . . . . . . . . . . :"))
		{
			const char *nic=strchr(rbuf,':');
			if(nic) 
			{
				char *rptr;
				for(rptr=(char*)nic+2;(*rptr)&&((*rptr=='.')||(isdigit(*rptr)));rptr++)
					;
				*rptr=0;
				nics.push_back(nic +2);
			}
		}
	}
	fclose(fp);
	return 0;
}
int LBMonitor::FindNIC(list<string>& nics, const char *iface, char nic[32])
{
	memset(nic,0,32);
	char icpy[256]={0};
	strncpy(icpy,iface,sizeof(icpy)-1);
	icpy[sizeof(icpy)-1]=0;
	// Optional port
	int port=8001;
	char *iptr=strchr(icpy,':');
	if(iptr)
	{
		*iptr=0; iptr++; port=atoi(iptr);
	}
	else
		iptr=icpy;
	// Optional subnet mask
	IN_ADDR mask;
	mask.s_addr=INADDR_NONE;
	iptr=strchr(iptr,'/');
	if(iptr)
	{
		*iptr=0; mask.s_addr=inet_addr(iptr+1);
	}
	IN_ADDR ifip;
	ifip.s_addr=inet_addr(icpy);
	for(list<string>::iterator nit=nics.begin();nit!=nics.end();nit++)
	{
		string snic=*nit;
		IN_ADDR sip;
		sip.s_addr=inet_addr(snic.c_str());
		if((sip.s_addr&mask.s_addr)==(ifip.s_addr&mask.s_addr))
		{
			sprintf(nic,"%s:%d",snic.c_str(),port);
			return 0;
		}
	}
	sprintf(nic,"%s:%d",icpy,port);
	return -1;
}
int LBMonitor::LBMAppSetupSocks(WsocksApp *pmod)
{
	if(pmod->pcfg->aclass=="SMConsole")
		return 0;
	if(!pmod->ccodec)
	{
		pmod->ccodec=new SysCmdCodec;
		if(pmod->ccodec->Init()<0)
			return -1;
	}
	if(!pmod->fcodec)
	{
		pmod->fcodec=new SysFeedCodec;
		if(pmod->fcodec->Init()<0)
			return -1;
	}
	if(pmod==this)
		return 0;
	char head[256]={0};
	sprintf(head,"LBSysmon for %s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
	WSSetAppHead(head);

	// Make sure the app has the minimum number of ports for monitoring multiple NICs
	if(pmod->NO_OF_UMC_PORTS<2)
	{
		int oldNoOfUmcPorts=pmod->NO_OF_UMC_PORTS;
		UMCPORT *oldUmc=pmod->UmcPort;
		pmod->NO_OF_UMC_PORTS=2;
		pmod->UmcPort=new UMCPORT[pmod->NO_OF_UMC_PORTS+1];
		memset(pmod->UmcPort,0,(pmod->NO_OF_UMC_PORTS+1)*sizeof(UMCPORT));
		if(oldNoOfUmcPorts>0)
			memcpy(pmod->UmcPort,oldUmc,oldNoOfUmcPorts *sizeof(UMCPORT));
		delete oldUmc;
	}
	if(pmod->NO_OF_UMR_PORTS<10)
	{
		pmod->NO_OF_UMR_PORTS=10;
		delete pmod->UmrPort;
		pmod->UmrPort=new UMRPORT[pmod->NO_OF_UMR_PORTS+1];
		memset(pmod->UmrPort,0,(pmod->NO_OF_UMR_PORTS+1)*sizeof(UMRPORT));
	}

	// If the app is configured for monitoring, only the first port is used
	int mport=-1;
	for(int i=0;i<pmod->NO_OF_UMC_PORTS;i++)
	{
		if(pmod->UmcPort[i].LocalIP[0])
		{
			if(mport<0)
			{
				mport=i;
				// LBMonitor's UMC port could already be bound, so it may need to be bounced
				// Use smproxy's first UMC port number unless overridden
				if((!pcfg->sysmonAddr.empty())&&(pcfg->criticalApp))
				{
					char astr[64]={0};
					sprintf(astr,"%s:%d",pmod->UmcPort[i].LocalIP,pmod->UmcPort[i].Port);
					pcfg->sysmonAddr=astr;
					if((UmcPort[1].Port!=pmod->UmcPort[i].Port)||(strcmp(UmcPort[1].LocalIP,UmcPort[i].LocalIP)))
					{
						char *pstr=strchr(astr,':');
						if(pstr)
						{
							*pstr=0; UmcPort[1].Port=atoi(pstr +1);
						}
						strcpy(UmcPort[1].LocalIP,astr);
						pmod->UmcPort[0].Compressed=0;
						WSLogEvent("Using critcial app's M%d interface %s:%d",i,UmcPort[1].LocalIP,UmcPort[1].Port);
						if(UmcPort[1].bindPort)
							WSCloseUmcPort(1);
					}
				}
				// The first app's UMC port becomes a loopback port
				strcpy(pmod->UmcPort[i].LocalIP,"127.0.0.1");
				pmod->UmcPort[i].Port=nextLBMonPort++;
				break;
			}
			// Leave the other ports for the app to handle itself
			//// All other app UMC ports are disabled
			//else
			//	strcpy(pmod->UmcPort[i].LocalIP,"");
		}
	}
	// If the app isn't configured for monitoring, make one port
	if(mport<0)
	{
		strcpy(pmod->UmcPort[0].LocalIP,"127.0.0.1");
		pmod->UmcPort[0].Port=nextLBMonPort++;
		pmod->UmcPort[0].BlockSize=WS_MAX_BLOCK_SIZE;
		pmod->WSCreatePort(WS_UMC,0);
		if(!UmcPort[0].InUse)
			WSSetupUmrPorts(WS_OPEN,0);
	}

	// If the app is configured for multi-interface monitoring, make multiple CTO ports	and UMC ports
	if(pmod->pcfg->broadAddrs.empty())
		UmcPort[1].ConnectHold=false;
	else
	{
		list<string> nics;
		EnumNICs(nics);

		char istr[256]={0};
		strcpy(istr,pmod->pcfg->broadAddrs.c_str());
		for(const char *iface=strtok(istr,",");iface;iface=strtok(0,","))
		{
			if(!_stricmp(iface,"AUTO"))
				continue;
			// Ex iface values:
			// AUTO:8002
			// 171.130.66.81
			// 171.130.66.81:8002
			// 171.130.66.255:8002/255.255.255.0
			char nic[32]={0};
			FindNIC(nics,iface,nic);
			int i;
			for(i=0;i<NO_OF_CTO_PORTS;i++)
			{
				// Override the default CTO port
				if(!strcmp(CtoPort[i].LocalIP,"AUTO"))
				{
					char *iptr=strchr(nic,':');
					if(iptr)
					{
						*iptr=0; CtoPort[i].Port=atoi(iptr+1);
					}
					strcpy(CtoPort[i].LocalIP,nic);
					break;
				}
				// Add new ports
				else if(!CtoPort[i].InUse)
				{
					// Broadcast CTO port
					WSCreatePort(WS_CTO,i);
					CtoPort[i].Port=8001;
					char *iptr=strchr(nic,':');
					if(iptr)
					{
						*iptr=0; CtoPort[i].Port=atoi(iptr+1);
					}
					strcpy(CtoPort[i].LocalIP,nic);
					//strcpy(CtoPort[0].RemoteIP,"224.2.1.15");
					strcpy(CtoPort[i].RemoteIP,"255.255.255.255");
					sprintf(CtoPort[i].CfgNote,"Sysmon");
					CtoPort[i].BlockSize=WS_MAX_BLOCK_SIZE;
					CtoPort[i].ConnectHold=false;
					WSSetupCtoPorts(WS_OPEN,i);
					break;
				}
			}
			// See if we're already listening on this interface
			int UmcPortNo=-1;
			for(i=0;i<NO_OF_UMC_PORTS;i++)
			{
				if(!UmcPort[i].InUse)
				{
					if(UmcPortNo<0) UmcPortNo=i; 
					continue;
				}
				// App-configured M0 should be replaced by app.ini explicit interface
				else if((i==1)&&(!_stricmp(UmcPort[i].LocalIP,"AUTO")))
				{
					if(UmcPortNo<0) UmcPortNo=i; 
					continue;
				}
				else if(!_stricmp(UmcPort[i].LocalIP,iface))
				{
					UmcPort[i].ConnectHold=false;
					break;
				}
			}
			if((i>=NO_OF_UMC_PORTS)&&(UmcPortNo>=0)&&(UmcPortNo<NO_OF_UMC_PORTS))
			{
				if(UmcPort[UmcPortNo].SockActive)
					WSCloseUmcPort(UmcPortNo);
				strcpy(UmcPort[UmcPortNo].LocalIP,iface);
				if(!pcfg->broadAddrs.empty())
					pcfg->broadAddrs+=",";
				pcfg->broadAddrs+=iface;
				if(UmcPortNo>1)
				{
					WSCreatePort(WS_UMC,UmcPortNo);
					UmcPort[UmcPortNo].Port=UmcPort[1].Port;
					strcpy(UmcPort[UmcPortNo].CfgNote,"Sysmon");
					UmcPort[UmcPortNo].BlockSize=WS_MAX_BLOCK_SIZE;
					UmcPort[UmcPortNo].Compressed=true;
				#ifdef WS_REALTIMESEND
					UmcPort[UmcPortNo].ImmediateSendLimit=UmcPort[1].BlockSize*1024;
				#endif
					UmcPort[UmcPortNo].dynPortBegin=16000;
					UmcPort[UmcPortNo].dynPortEnd=17000;
					if(!UmcPort[UmcPortNo].InUse)
						WSSetupUmrPorts(WS_OPEN,UmcPortNo);
				}
				UmcPort[UmcPortNo].ConnectHold=false;
			}
		}
	}
	for(int i=0;i<NO_OF_CTO_PORTS;i++)
	{
		if(!CtoPort[i].InUse)
			continue;
		if(CtoPort[i].ConnectHold)
			CtoPort[i].ConnectHold=false;
	}

	int PortNo=-1;
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		// Open CON port
		if((PortNo<0)&&((!ConPort[i].InUse)||(ConPort[i].ConnectHold)))
			PortNo=i;
		// Already connected
		else if((ConPort[i].InUse)&&(ConPort[i].DetPtr==pmod))
		{
			PortNo=i;
			break;
		}
	}
	if(PortNo<0)
		return -1;
	this->WSCreatePort(WS_CON,PortNo);
	strcpy(ConPort[PortNo].LocalIP,"127.0.0.1");
	strcpy(ConPort[PortNo].RemoteIP,"127.0.0.1");
	sprintf(ConPort[PortNo].CfgNote,"%s/%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
	strcpy(ConPort[PortNo].AltRemoteIP[0],"127.0.0.1");
	ConPort[PortNo].AltRemoteIPOn[0]=1;
	ConPort[PortNo].AltIPCount=1;
	ConPort[PortNo].CurrentAltIP=-1;
	ConPort[PortNo].Port=pmod->UmcPort[0].Port;
	#ifdef WIN32
	//REDICP-1917 
	ConPort[PortNo].BounceEnd=(LONG_PTR)(PTRCAST)pmod;
	#else
	ConPort[PortNo].sDetPtr=pmod;
	#endif
	if(ConPort[PortNo].ConnectHold)
		ConPort[PortNo].ConnectHold=false;
	else
	{
		ConPort[PortNo].BlockSize=WS_MAX_BLOCK_SIZE;
		phost->WSHSetupConPorts(this,WS_OPEN,PortNo);
	}
	return 0;
}
int LBMonitor::LBMAppCloseSocks(WsocksApp *pmod)
{
	if(pmod->ccodec)
	{
		delete pmod->ccodec; pmod->ccodec=0;
	}
	if(pmod->fcodec)
	{
		delete pmod->fcodec; pmod->fcodec=0;
	}
	if(pmod==this)
		return 0;
	int PortNo=-1;
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if((ConPort[i].InUse)&&
			#ifdef WIN32
		   ((ConPort[i].DetPtr==pmod)||(ConPort[i].BounceEnd==(LONGLONG)pmod)))
			#else
		   ((ConPort[i].DetPtr==pmod)||(ConPort[i].sDetPtr==pmod)))
			#endif
		{
			PortNo=i;
			break;
		}
	}
	if(PortNo<0)
		return -1;
	// This function is called by the app thread, but the LBMon thread may be using this CON port at the same time.
	//phost->WSHSetupConPorts(this,WS_CLOSE,PortNo);
	//this->WSDeletePort(WS_CON,PortNo);
	// Just put the port on hold and reuse it later.
	ConPort[PortNo].ConnectHold=true;

	// Tell clients about closed apps
	// Sending here causes a thread dependency between the closing app's thread and the LBMon thread.
	//for(int i=0;i<NO_OF_UMR_PORTS;i++)
	//{
	//	if(!UmrPort[i].SockActive)
	//		continue;
	//	FeedUser *fuser=(FeedUser *)UmrPort[i].DetPtr3;
	//	if(!fuser)
	//		continue;
	//	SysmonFlags *umflags=(SysmonFlags*)fuser->DetPtr;
	//	if((umflags)&&(umflags->appListRid))
	//	{
	//		char parms[1024]={0},resp[1024]={0},*rptr=resp;
	//		int rlen=sizeof(resp);
	//		sprintf(parms,"AppClass=%s|AppName=%s|AppPath=%s.%s|SockConnected=N|",
	//			pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),
	//			pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str()); 
	//		rptr=resp; rlen=sizeof(resp);
	//		ccodec->Encode(rptr,rlen,false,umflags->appListRid,"applist",parms,0);
	//		WSUmrSendMsg(1109,(int)(rptr -resp),resp,i);
	//	}
	//}
	return 0;
}
int LBMonitor::LBMPortChanged(WsocksApp *pmod, WSPort *pport)
{
	if((pport->PortType!=WS_USR)&&
	   (pport->PortType!=WS_UMR)&&
	   (pport->PortType!=WS_UGR)&&
       (pport->PortType!=WS_OTH))
		return 0;
	// Generate sysmon feed on the single port for registered apps
	char skey[256]={0};
	sprintf(skey,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
	_strupr(skey);
	TASKITEM *titem=taskmgr.FindUserItem(skey);
	if(titem)
	{
		char resp[1024]={0},*rptr=resp;
		int rlen=sizeof(resp);
		bool lvl1=false,lvl2=false;
		//for(TASKMAP::iterator tit=titem->taskMap.begin();tit!=titem->taskMap.end();tit++)
		for(TASKENTRY *ptask=titem->ptasks;ptask;ptask=ptask->inext)
		{
			//TASKENTRY *ptask=tit.second;
			// Generate level 1 feed
			if((!lvl1)&&(ptask->taskid==SMTASK_LEVEL1))
			{
				//if(tnow -pmod->lvl1UpdateTime>=pmod->lvl1Interval)
				{
					HANDLE ihnd=(HANDLE)(PTRCAST)MAKELONG(pport->PortNo,pport->PortType+1);
					//char heading[256]={0},status[256]={0};
					//phost->WSHGetAppHead(pmod,heading,sizeof(heading));
					//phost->WSHGetAppStatus(pmod,status,sizeof(status));
					if(!pmod->fcodec)
						return -1;
					if(pmod->fcodec->EncodeLvl1(rptr,rlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),0,0,pmod,false,ihnd,true)>0)
					{
						if(rptr>resp)
						{
						#ifdef FIX_TASK_CALL
							// Only call tasks from LBMon thread, not external threads
							EnterCriticalSection(&emutex);
							int elen=(int)(rptr -resp),slen=(int)strlen(skey) +1;
							char *ebuf=new char[12 +slen +elen],*eptr=ebuf;
							int ecode=SMTASK_LEVEL1;
							memcpy(eptr,&ecode,4); eptr+=4;
							memcpy(eptr,&slen,4); eptr+=4;
							memcpy(eptr,skey,slen); eptr+=slen;
							memcpy(eptr,&elen,4); eptr+=4;
							memcpy(eptr,resp,elen);
							elist.push_back(ebuf);
							LeaveCriticalSection(&emutex);
						#else
							SMFeedHint fhint={resp,(int)(rptr -resp)};
							taskmgr.CallTasks(skey,SMTASK_LEVEL1,&fhint);
						#endif
							rptr=resp;
						}
					}
					//pmod->lvl1UpdateTime=tnow;
				}
				lvl1=true;
			}
			// Generate level 2 feed
			if((!lvl2)&&(ptask->taskid==SMTASK_LEVEL2))
			{
				//if(tnow -pmod->lvl2UpdateTime>=pmod->lvl2Interval)
				{
					HANDLE ihnd=(HANDLE)(PTRCAST)MAKELONG(pport->PortNo,pport->PortType+1);
					//char heading[256]={0},status[256]={0};
					//phost->WSHGetAppHead(pmod,heading,sizeof(heading));
					//phost->WSHGetAppStatus(pmod,status,sizeof(status));
					if(!pmod->fcodec)
						return -1;
					if(pmod->fcodec->EncodeLvl2(rptr,rlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),0,0,pmod,false,ihnd,true)>0)
					{
						if(rptr>resp)
						{
						#ifdef FIX_TASK_CALL
							// Only call tasks from LBMon thread, not external threads
							EnterCriticalSection(&emutex);
							int elen=(int)(rptr -resp),slen=(int)strlen(skey) +1;
							char *ebuf=new char[12 +slen +elen],*eptr=ebuf;
							int ecode=SMTASK_LEVEL2;
							memcpy(eptr,&ecode,4); eptr+=4;
							memcpy(eptr,&slen,4); eptr+=4;
							memcpy(eptr,skey,slen); eptr+=slen;
							memcpy(eptr,&elen,4); eptr+=4;
							memcpy(eptr,resp,elen);
							elist.push_back(ebuf);
							LeaveCriticalSection(&emutex);
						#else
							SMFeedHint fhint={resp,(int)(rptr -resp)};
							taskmgr.CallTasks(skey,SMTASK_LEVEL2,&fhint);
						#endif
							rptr=resp;
						}
					}
					//pmod->lvl2UpdateTime=tnow;
				}
				lvl2=true;
			}
			if((lvl1)&&(lvl2))
				break;
		}
	}
	return 0;
}
void LBMonitor::WSTimeChange()
{
	DWORD tnow=GetTickCount();
	// Common commands
	scmds.CheckThreadedCmd();

	// Heartbeats and sysmon feed
	char hbeat[128];
	memset(hbeat,0,128);
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		ConPort[i].BeatsOut++;
		WSConSendMsg(16,128,hbeat,i);

		WsocksApp *pmod=(WsocksApp*)ConPort[i].DetPtr;
		if(pmod)
		{
			// Generate sysmon feed on all ports for registered apps
			char skey[256]={0};
			sprintf(skey,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
			_strupr(skey);
			TASKITEM *titem=taskmgr.FindUserItem(skey);
			if(titem)
			{
				char resp[1024]={0},*rptr=resp;
				int rlen=sizeof(resp);
				bool lvl1=false,lvl2=false;
				//for(TASKMAP::iterator tit=titem->taskMap.begin();tit!=titem->taskMap.end();tit++)
				for(TASKENTRY *ptask=titem->ptasks;ptask;ptask=ptask->inext)
				{
					//TASKENTRY *ptask=tit.second;
					// Generate level 1 feed
					if((!lvl1)&&(ptask->taskid==SMTASK_LEVEL1))
					{
						if(tnow -pmod->lvl1UpdateTime>=pmod->lvl1Interval)
						{
							HANDLE ihnd=0;
							char heading[256]={0},status[256]={0};
							phost->WSHGetAppHead(pmod,heading,sizeof(heading));
							phost->WSHGetAppStatus(pmod,status,sizeof(status));
							if(!pmod->fcodec)
								break;
							while(pmod->fcodec->EncodeLvl1(rptr,rlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),heading,status,pmod,false,ihnd,false)>0)
							{
								if(rptr>resp)
								{
									SMFeedHint fhint={resp,(int)(rptr -resp)};
									taskmgr.CallTasks(skey,SMTASK_LEVEL1,&fhint);
									rptr=resp;
								}
							}
							pmod->lvl1UpdateTime=tnow;
						}
						lvl1=true;
					}
					// Generate level 2 feed
					if((!lvl2)&&(ptask->taskid==SMTASK_LEVEL2))
					{
						if(tnow -pmod->lvl2UpdateTime>=pmod->lvl2Interval)
						{
							HANDLE ihnd=0;
							char heading[256]={0},status[256]={0};
							phost->WSHGetAppHead(pmod,heading,sizeof(heading));
							phost->WSHGetAppStatus(pmod,status,sizeof(status));
							if(!pmod->fcodec)
								break;
							while(pmod->fcodec->EncodeLvl2(rptr,rlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),heading,status,pmod,false,ihnd,false)>0)
							{
								if(rptr>resp)
								{
									SMFeedHint fhint={resp,(int)(rptr -resp)};
									taskmgr.CallTasks(skey,SMTASK_LEVEL2,&fhint);
									rptr=resp;
								}
							}
							pmod->lvl2UpdateTime=tnow;
						}
						lvl2=true;
					}
					if((lvl1)&&(lvl2))
						break;
				}
			}
		}
	}
	// We're passing through WsocksApp heartbeats now
	//for(int i=0;i<NO_OF_UMR_PORTS;i++)
	//{
	//	if(!UmrPort[i].SockActive)
	//		continue;
	//	UmrPort[i].BeatsOut++;
	//	WSUmrSendMsg(16,128,hbeat,i);
	//}

	// Broadcast sysmon port right after port opened and once every few seconds
	// Don't advertise the loopback interface used for local sysmon console
	for(int i=1;i<NO_OF_UMC_PORTS;i++)
	{
		if(!UmcPort[i].SockActive)
			continue;
		char ibuf[256]={0},*iptr=ibuf;
		MSG1112REC Msg1112Rec;
		memset(&Msg1112Rec,0,sizeof(MSG1112REC));
		memcpy(Msg1112Rec.bindIP,&UmcPort[i].bindIP,20);
		Msg1112Rec.bindPort=UmcPort[i].bindPort;
		memcpy(Msg1112Rec.uuid,uuid,16);
		MSGHEADER MsgHeader={1112,sizeof(MSG1112REC)};
		memcpy(iptr,&MsgHeader,4); iptr+=4;
		memcpy(iptr,&Msg1112Rec,sizeof(MSG1112REC)); iptr+=sizeof(MSG1112REC);
		for(int j=0;j<NO_OF_CTO_PORTS;j++)
		{
			if(!CtoPort[j].SockActive)
			{
				// Unhold CTO port every 10 seconds (after bind failure)
				if((CtoPort[j].InUse)&&(CtoPort[j].ConnectHold)&&(WSTime%10==0))
					CtoPort[j].ConnectHold=false;
				continue;
			}
			if((strcmp(CtoPort[j].LocalIP,"AUTO"))&&(strcmp(CtoPort[j].LocalIP,Msg1112Rec.bindIP)))
				continue;
			DWORD& tlast=(DWORD&)CtoPort[j].DetPtr3;
			if(tnow -tlast>=5000)
			{
				WSCtoSendBuff((int)(iptr -ibuf),ibuf,j,1);
				CtoPort[j].BeatsOut++;
				tlast=tnow;
			}
		}
	}
}
#ifdef FIX_TASK_CALL
int LBMonitor::WSPreTick()
{
	EnterCriticalSection(&emutex);
	for(list<char*>::iterator eit=elist.begin();eit!=elist.end();eit++)
	{
		char *ebuf=*eit,*eptr=ebuf,*skey=0;
		int ecode=0,elen=0,slen=0;
		memcpy(&ecode,eptr,4); eptr+=4;
		memcpy(&slen,eptr,4); eptr+=4;
		skey=eptr; eptr+=slen;
		memcpy(&elen,eptr,4); eptr+=4;
		SMFeedHint fhint={eptr,elen};
		taskmgr.CallTasks(skey,ecode,&fhint);
		delete ebuf;
	}
	elist.clear();
	LeaveCriticalSection(&emutex);
	return 0;
}
#endif

#if defined(WIN32)||defined(_CONSOLE)
WsocksApp * __stdcall LBMonitor_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#else
void * __stdcall LBMonitor_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#endif
{
        if(version!=WSHOST_INTERFACE_VERSION)
                return 0;
        if(!_stricmp(aclass,"LBSysmon"))
        {
                LBMonitor *pmod=new LBMonitor;
                pmod->pFreeAppInterface=LBMonitor_FreeAppInterface;
                return pmod;
        }
        return 0;
}
#if defined(WIN32)||defined(_CONSOLE)
void __stdcall LBMonitor_FreeAppInterface(WsocksApp *pmod)
{
#else
void __stdcall LBMonitor_FreeAppInterface(void *amod)
{
        WsocksApp *pmod=(WsocksApp *)amod;
#endif
		if(!_stricmp(pmod->pcfg->aclass.c_str(),"LBSysmon"))
        {
                delete (LBMonitor*)pmod;
        }
}

static int _stdcall _SendLvl1Quote(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	return umflags->pmod->SendLvl1Quote(fuser,skey,taskid,0,udata,hint);
}
int LBMonitor::SendLvl1Quote(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;	
	SMFeedHint *fhint=(SMFeedHint *)hint;
	WSUmrSendMsg(1110,fhint->rlen,fhint->rptr,fuser->PortNo);
	return 1;
}
static int _stdcall _SendLvl2Quote(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	return umflags->pmod->SendLvl2Quote(fuser,skey,taskid,0,udata,hint);
}
int LBMonitor::SendLvl2Quote(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	SMFeedHint *fhint=(SMFeedHint *)hint;
	WSUmrSendMsg(1110,fhint->rlen,fhint->rptr,fuser->PortNo);
	return 1;
}
static int _stdcall _SendErrorLog(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	return umflags->pmod->SendErrorLog(fuser,skey,taskid,0,udata,hint);
}
int LBMonitor::SendErrorLog(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	SMFeedHint *fhint=(SMFeedHint *)hint;
	WSUmrSendMsg(1110,fhint->rlen,fhint->rptr,fuser->PortNo);
	return 1;
}
static int _stdcall _SendEventLog(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	return umflags->pmod->SendEventLog(fuser,skey,taskid,0,udata,hint);
}
int LBMonitor::SendEventLog(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	SMFeedHint *fhint=(SMFeedHint *)hint;
	WSUmrSendMsg(1110,fhint->rlen,fhint->rptr,fuser->PortNo);
	return 1;
}
static int _stdcall _SendTrademon(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	return umflags->pmod->SendTrademon(fuser,skey,taskid,0,udata,hint);
}
int LBMonitor::SendTrademon(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	SMFeedHint *fhint=(SMFeedHint *)hint;
	WSUmrSendMsg(1110,fhint->rlen,fhint->rptr,fuser->PortNo);
	return 1;
}
static int _stdcall _SendCustom(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	return umflags->pmod->SendCustom(fuser,skey,taskid,0,udata,hint);
}
int LBMonitor::SendCustom(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	SysmonFlags *umflags=(SysmonFlags *)udata;
	SMFeedHint *fhint=(SMFeedHint *)hint;
	WSUmrSendMsg(1110,fhint->rlen,fhint->rptr,fuser->PortNo);
	return 1;
}
void LBMonitor::NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	if(PortType!=WS_UMR)
		return;
	TVMAP tvmap;
	if(ccodec->GetTagValues(tvmap,parm,plen)<0)
		return;

	FeedUser *fuser=(FeedUser*)UmrPort[PortNo].DetPtr3;
	if(!fuser)
		return;
	SysmonFlags *umflags=(SysmonFlags *)fuser->DetPtr;
	if(!umflags)
		return;
	//if(!umflags->minRid)
	//	umflags->minRid=reqid;

	char parms[2048 -128]={0},resp[2048]={0},*rptr=resp;
	int rlen=sizeof(resp);
	// Point-to-point
	string oboi=ccodec->GetValue(tvmap,"OnBehalfOf");
	string dtid=ccodec->GetValue(tvmap,"DeliverTo");
	if(dtid.empty())
	{
		// Point-to-point login
		if(cmd=="login")
		{
			string user=ccodec->GetValue(tvmap,"User");
			string pass=ccodec->GetValue(tvmap,"Pass");
			fuser->user=user;
			// Must always have user
			if(user.empty())
			{
				fuser->waitAuth=false;
				WSLogEvent("UMR%d: authorization failure for %s",PortNo,user.c_str());
				ccodec->Encode(rptr,rlen,false,reqid,cmd,"rc=-1|Reason=authorization failed|",0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				UmrPort[PortNo].TimeTillClose=5;
				UmrPort[PortNo].SockActive=false;
			}
			// Only SMConsoleOps and SMProxy will send blank password
			else if(pass.empty())
			{
				fuser->authed=true;
				fuser->waitAuth=false;
				WSLogEvent("UMR%d: logged in as %s",PortNo,user.c_str());
				if(!strncmp(user.c_str(),"smproxy.",8))
				{
					proxyAuthPort=PortNo;
					WSLogEvent("UMR%d: designated as proxy authorizer",PortNo);
				}

				char smon[8];
				int year=0,mon=0,day=0;
				sscanf(((WsocksHostImpl*)phost)->__HOST_BUILD_DATE__,"%s %d %d",smon,&day,&year);
				static const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
				char *mptr=(char*)strstr(months,smon);
				if(mptr)
					mon=(int)(mptr -months)/3+1;
				int bdate=(year*10000)+(mon*100)+(day);

				int hh=0,mm=0,ss=0;
				sscanf(((WsocksHostImpl*)phost)->__HOST_BUILD_TIME__,"%d:%d:%d",&hh,&mm,&ss);
				int btime=(hh*10000)+(mm*100)+(ss);
				char cdir[MAX_PATH]={0};
				GetCurrentDirectory(sizeof(cdir),cdir);

				sprintf(parms,"rc=0|Text=success|BuildDate=%08d|BuildTime=%06d|Pid=%d|RunCmd=%s|RunDir=%s|",
					bdate,btime,GetCurrentProcessId(),GetCommandLine(),cdir); 
				ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
				sprintf(UmrPort[PortNo].Status,"%s",fuser->user.c_str());
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			}
			// If there is an active smproxy connection, then proxy autorize
			else if(proxyAuthPort>=0)
			{
				fuser->waitAuth=true;
				ccodec->Encode(rptr,rlen,true,reqid,cmd,parm,0);
				WSUmrSendMsg(1113,(int)(rptr -resp),resp,proxyAuthPort);
			}
			else
			{
				fuser->waitAuth=false;
				WSLogEvent("UMR%d: authorization failure for %s",PortNo,user.c_str());
				ccodec->Encode(rptr,rlen,false,reqid,cmd,"rc=-1|Reason=authorization failed|",0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				UmrPort[PortNo].TimeTillClose=5;
				UmrPort[PortNo].SockActive=false;
			}
			return;
		}
		// Only login cmd allowed if unauthorized
		if(!fuser->authed)
		{
			_ASSERT(false);//untested
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,"rc=-1|Reason=Not logged in.|",0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		// Subscribe to all the apps this proxy knows about (point-to-point)
		if(cmd=="applist")
		{
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
			{
				int acnt=0;
				for(int i=0;i<NO_OF_CON_PORTS;i++)
				{
					if(!ConPort[i].SockConnected)
						continue;
					WsocksApp *pmod=(WsocksApp*)ConPort[i].DetPtr;
					if(!pmod)
						continue;
					if(pmod->pcfg->aclass=="SMConsole")
						continue;
					sprintf(parms,"AppClass=%s|AppName=%s|AppPath=%s.%s|SockConnected=Y|AppBuildDate=%08d|AppBuildTime=%06d|",
						pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),
						pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),
                        pmod->buildDate, pmod->buildTime);
					rptr=resp; rlen=sizeof(resp);
					ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
					acnt++;
				}
				if(!acnt)
				{
					ccodec->Encode(rptr,rlen,false,reqid,cmd,"rc=-1|Reason=No monitored apps|",0);
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				}
			}
			if(live=="Y")
				umflags->appListRid=reqid;
			return;
		}
		//// Feed update interval for this client
		//if(cmd=="UpdateInterval")
		//{
		//	_ASSERT(false);//untested
		//	int UpdateInterval=atol(ccodec->GetValue(tvmap,"Interval").c_str());
		//	if(UpdateInterval<=0)
		//	{
		//		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,"rc=-1|Reason=Invalid 'Interval' value.|",0);
		//		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		//	}
		//	umflags->lvl1Interval=UpdateInterval;
		//	ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,"rc=0|Text=Success|",0);
		//	WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		//	//WSHSendMonitorData(pmod,PortNo);
		//	return;
		//}
	}

	// Proxy all other commands to the target app or smproxy
	if(dtid.empty())
		return;
	// Find the target app
	WsocksApp *pmod=0,*pxymod=0;
	int pxyPort=-1;
	int i;
	for(i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		WsocksApp *cmod=(WsocksApp*)ConPort[i].DetPtr;
		char *apath=(char *)ConPort[i].DetPtr4;
		if((apath)&&(dtid==apath))
		{
			pmod=cmod;
			break;
		}
		else if(cmod->pcfg->aclass=="smproxy")
		{
			pxymod=cmod; pxyPort=i;
		}
	}
	// Nack
	if((!pmod)&&(!pxymod))
	{
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Undeliverable|",
			dtid.c_str(),oboi.c_str());
		rptr=resp;
		ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)strlen(resp),resp,PortNo);
		return;
	}
	int ConPortNo=i;
	char noboi[256]={0};
    sprintf(noboi,"%s/LBM.%d/%s.%s",oboi.c_str(),PortNo, pcfg->aclass.c_str(), pcfg->aname.c_str());

	TVMAP rvmap;
	// App identify request
	if((cmd=="ident")&&(pmod))
	{
		rvmap.clear();
		char vstr[256]={0};
		WSSetValue(rvmap,"rc","0");
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"AppClass",pmod->pcfg->aclass);
		WSSetValue(rvmap,"AppName",pmod->pcfg->aname);
		sprintf(vstr,"%08d",pmod->buildDate);
		WSSetValue(rvmap,"BuildDate",vstr);
		sprintf(vstr,"%06d",pmod->buildTime);
		WSSetValue(rvmap,"BuildTime",vstr);
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);
		return;
	}
	// App config request
	else if((cmd=="appconfig")&&(pmod))
	{
		rvmap.clear();
		char vstr[256]={0};
		WSSetValue(rvmap,"rc","0");
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"AppClass",pmod->pcfg->aclass);
		WSSetValue(rvmap,"AppName",pmod->pcfg->aname);
		sprintf(vstr,"%08d",pmod->buildDate);
		WSSetValue(rvmap,"BuildDate",vstr);
		sprintf(vstr,"%06d",pmod->buildTime);
		WSSetValue(rvmap,"BuildTime",vstr);
		WSSetValue(rvmap,"AsyncMode",pmod->pcfg->asyncMode?"Y":"N");
		WSSetValue(rvmap,"BuildTime",vstr);
		WSSetValue(rvmap,"DllPath",pmod->pcfg->DllPath.c_str());
		WSSetValue(rvmap,"ConfigPath",pmod->pcfg->ConfigPath.c_str());
		WSSetValue(rvmap,"RunPath",pmod->pcfg->RunPath.c_str());
		WSSetValue(rvmap,"RunCmd",pmod->pcfg->RunCmd.c_str());
		WSSetValue(rvmap,"LogPath",pmod->pcfg->LogPath.c_str());
		WSSetValue(rvmap,"Enabled",pmod->pcfg->Enabled?"Y":"N");
		WSSetValue(rvmap,"TargetThreads",pmod->pcfg->TargetThreads.c_str());
		WSSetValue(rvmap,"WshLoopback",pmod->pcfg->wshLoopback?"Y":"N");
		WSSetValue(rvmap,"SysmonAddr",pmod->pcfg->sysmonAddr.c_str());
		WSSetValue(rvmap,"BroadAddrs",pmod->pcfg->broadAddrs.c_str());
		WSSetValue(rvmap,"CriticalApp",pmod->pcfg->criticalApp?"Y":"N");
		sprintf(vstr,"%d",pmod->pcfg->threadAbortTimeout);
		WSSetValue(rvmap,"ThreadAbortTimeout",vstr);
		WSSetValue(rvmap,"Loopback",pmod->pcfg->loopback?"Y":"N");
		#ifdef WIN32
		FILE *fp=fopen("C:\\ThisServer.ini","rt");
		#else
		FILE *fp=fopen("~/ThisServer.ini","rt");
		#endif
		if(fp)
		{
			char rbuf[256]={0};
			while(fgets(rbuf,sizeof(rbuf),fp))
			{
				const char *rptr;
				for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
					;
				const char *rend=rbuf +strlen(rbuf);
				char *eptr=(char*)strendl(rptr,rend);
				if(eptr) *eptr=0;
				if(!strincmp(rptr,"IP-",3))
				{
					char *eptr=(char*)strchr(rptr,'=');
					if((eptr)&&(eptr[1]))
					{
						*eptr=0;
						char vname[32]={0};
						sprintf(vname,"ThisServer%s",rptr);
						WSSetValue(rvmap,vname,eptr+1);
					}
				}
			}
		}
		else
			WSSetValue(rvmap,"ThisServerIP-0","");
		sprintf(vstr,"%I64u",pmod->pcfg->featList.size());
		WSSetValue(rvmap,"FeatureCount",vstr);
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		for(list<AppFeature>::iterator fit=pmod->pcfg->featList.begin();fit!=pmod->pcfg->featList.end();fit++)
		{
			AppFeature& feat=*fit;
			if((feat.wsdate)||(feat.wstime))
			{
				sprintf(vstr,"%08d",feat.wsdate);
				WSSetValue(rvmap,"FeatDate",vstr);
				sprintf(vstr,"%06d",feat.wstime);
				WSSetValue(rvmap,"FeatTime",vstr);
			}
			else
			{
				sprintf(vstr,"%08d",pmod->buildDate);
				WSSetValue(rvmap,"FeatDate",vstr);
				sprintf(vstr,"%06d",pmod->buildTime);
				WSSetValue(rvmap,"FeatTime",vstr);
			}
			sprintf(vstr,"%d",feat.devTrackNo);
			WSSetValue(rvmap,"FeatDT",vstr);
			WSSetValue(rvmap,"FeatAuthor",feat.author);
			WSSetValue(rvmap,"FeatDesc",feat.desc);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(PortNo,reqid,cmd,parms,0);
		}
		return;
	}
	// Filter out common app and sysmon functionality
	// Some commands only work if we're directly connected to the app (i.e. pmod!=0)
	else if(cmd=="helpcommon")
	{
		// Begin help
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"Begin","Y");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// ident
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","ident");
		ccodec->SetValue(rvmap,"Desc","returns app class and name");
		ccodec->SetValue(rvmap,"NumParms","0");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","ident");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// appconfig
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","appconfig");
		ccodec->SetValue(rvmap,"Desc","returns app.ini info");
		ccodec->SetValue(rvmap,"NumParms","0");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","appconfig");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// subscribe
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","subscribe");
		WSSetValue(rvmap,"Desc","sysmon and EYE feeds");
		WSSetValue(rvmap,"NumParms","3");
			// Feed
			WSSetValue(rvmap,"Parm1","Feed");
			WSSetValue(rvmap,"ParmDef1","level1");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","level1/level2/errorlog/eventlog");
			// Hist
			WSSetValue(rvmap,"Parm2","Hist");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","Y/N");
			// Live
			WSSetValue(rvmap,"Parm3","Live");
			WSSetValue(rvmap,"ParmDef3","Y");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","Y/N");
		WSSetValue(rvmap,"NumSamples","2");
		WSSetValue(rvmap,"Sample1","subscribe Feed=level1/Live=Y");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetValue(rvmap,"Sample2","subscribe Feed=errorlog/Hist=Y/Live=Y");
		WSSetValue(rvmap,"SampleDesc2","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// exit
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","exit");
		WSSetValue(rvmap,"Desc","exit the app");
		WSSetValue(rvmap,"NumParms","2");
			// User
			WSSetValue(rvmap,"Parm1","User");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","your username");
			// Reason
			WSSetValue(rvmap,"Parm2","Reason");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","reason");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","exit user=/reason=");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// restart
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","restart");
		WSSetValue(rvmap,"Desc","restart the app");
		WSSetValue(rvmap,"NumParms","2");
			// User
			WSSetValue(rvmap,"Parm1","User");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","your username");
			// Reason
			WSSetValue(rvmap,"Parm2","Reason");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","reason");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","restart user=/reason=");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// threadlist
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","threadlist");
		WSSetValue(rvmap,"Desc","lists thread info");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","threadlist");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// threadkill
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","threadkill");
		WSSetValue(rvmap,"Desc","kills a WsocksApi thread");
		WSSetValue(rvmap,"NumParms","2");
			// Name
			WSSetValue(rvmap,"Parm1","Name");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","thread name reported by threadlist");
			// Restart
			WSSetValue(rvmap,"Parm2","Restart");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","Y/N");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","threadkill Name=T1/Restart=Y");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// bounce
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","bounce");
		WSSetValue(rvmap,"Desc","closes a port");
		WSSetValue(rvmap,"NumParms","1");
			// Port
			WSSetValue(rvmap,"Parm1","Port");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","i.e. C1,U1,u0,F0,M0,m0,D0,G1,g0,I2,O3");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","bounce Port=");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// set
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","set");
		WSSetValue(rvmap,"Desc","changes port config");
		WSSetValue(rvmap,"NumParms","19");
			// Port
			WSSetValue(rvmap,"Parm1","Port");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","i.e. C1,F0,U1,u0,D0,G1,g0,I2,O3");
			// Ip
			WSSetValue(rvmap,"Parm2","Ip");
			WSSetValue(rvmap,"ParmDef2","127.0.0.1");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","IP address");
			// Tcpport
			WSSetValue(rvmap,"Parm3","Tcpport");
			WSSetValue(rvmap,"ParmDef3","0");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","Port number");
			// Hold
			WSSetValue(rvmap,"Parm4","Hold");
			WSSetValue(rvmap,"ParmDef4","Y");
			WSSetValue(rvmap,"ParmOpt4","Y");
			WSSetValue(rvmap,"ParmDesc4","Y/N");
			// Rec
			WSSetValue(rvmap,"Parm5","Rec");
			WSSetValue(rvmap,"ParmDef5","Y");
			WSSetValue(rvmap,"ParmOpt5","Y");
			WSSetValue(rvmap,"ParmDesc5","Y/N");
			// Note
			WSSetValue(rvmap,"Parm6","Note");
			WSSetValue(rvmap,"ParmDef6","");
			WSSetValue(rvmap,"ParmOpt6","Y");
			WSSetValue(rvmap,"ParmDesc6","Config note");
			// Altip#
			WSSetValue(rvmap,"Parm7","Altip1");
			WSSetValue(rvmap,"ParmDef7","100.100.0.0");
			WSSetValue(rvmap,"ParmOpt7","Y");
			WSSetValue(rvmap,"ParmDesc7","Alternate IP");
			// Active#
			WSSetValue(rvmap,"Parm8","Active1");
			WSSetValue(rvmap,"ParmDef8","Y");
			WSSetValue(rvmap,"ParmOpt8","Y");
			WSSetValue(rvmap,"ParmDesc8","Y/N");
			// Current#
			WSSetValue(rvmap,"Parm9","Current1");
			WSSetValue(rvmap,"ParmDef9","Y");
			WSSetValue(rvmap,"ParmOpt9","Y");
			WSSetValue(rvmap,"ParmDesc9","Y");
			// Reset
			WSSetValue(rvmap,"Parm10","Reset");
			WSSetValue(rvmap,"ParmDef10","Y");
			WSSetValue(rvmap,"ParmOpt10","Y");
			WSSetValue(rvmap,"ParmDesc10","Y/N");
			// NewAltip
			WSSetValue(rvmap,"Parm11","NewAltip");
			WSSetValue(rvmap,"ParmDef11","127.0.0.1");
			WSSetValue(rvmap,"ParmOpt11","Y");
			WSSetValue(rvmap,"ParmDesc11","New altip");
			// Bounce
			WSSetValue(rvmap,"Parm12","Bounce");
			WSSetValue(rvmap,"ParmDef12","0");
			WSSetValue(rvmap,"ParmOpt12","Y");
			WSSetValue(rvmap,"ParmDesc12","Bounce timeout");
			// Bstart
			WSSetValue(rvmap,"Parm13","Bstart");
			WSSetValue(rvmap,"ParmDef13","0");
			WSSetValue(rvmap,"ParmOpt13","Y");
			WSSetValue(rvmap,"ParmDesc13","Bounce start time (inclusive)");
			// Bend
			WSSetValue(rvmap,"Parm14","Bend");
			WSSetValue(rvmap,"ParmDef14","0");
			WSSetValue(rvmap,"ParmOpt14","Y");
			WSSetValue(rvmap,"ParmDesc14","Bounce end time (exclusive)");
			// Drives
			WSSetValue(rvmap,"Parm15","Drives");
			WSSetValue(rvmap,"ParmDef15","PQX");
			WSSetValue(rvmap,"ParmOpt15","Y");
			WSSetValue(rvmap,"ParmDesc15","File port remote drives");
			// Reset
			WSSetValue(rvmap,"Parm16","Reset");
			WSSetValue(rvmap,"ParmDef16","Y");
			WSSetValue(rvmap,"ParmOpt16","Y");
			WSSetValue(rvmap,"ParmDesc16","Y/N");
            // Compressed
            WSSetValue(rvmap,"Parm17","Comp");
            WSSetValue(rvmap,"ParmDef17","");
            WSSetValue(rvmap,"ParmOpt17","Y");
            WSSetValue(rvmap,"ParmDesc17","Compressed");
            // Compress Type
            WSSetValue(rvmap,"Parm18","CompType");
            WSSetValue(rvmap,"ParmDef18","");
            WSSetValue(rvmap,"ParmOpt18","Y");
            WSSetValue(rvmap,"ParmDesc18","Compress Type");
            // Encryption Type
            WSSetValue(rvmap,"Parm19","EncryptType");
            WSSetValue(rvmap,"ParmDef19","");
            WSSetValue(rvmap,"ParmOpt19","Y");
            WSSetValue(rvmap,"ParmDesc19","Encryption Type");
		WSSetValue(rvmap,"NumSamples","5");
		WSSetValue(rvmap,"Sample1","set Port=/Ip=/Tcpport=/Active0=Y/Current0=Y/Reset=Y");
		WSSetValue(rvmap,"SampleDesc1","Change first ip and port number");
		//WSSetValue(rvmap,"Sample2","set Port=/Hold=Y/Reset=Y");
		//WSSetValue(rvmap,"SampleDesc2","Change hold");
		//WSSetValue(rvmap,"Sample3","set Port=/Note=/Reset=Y");
		//WSSetValue(rvmap,"SampleDesc3","Change note");
		WSSetValue(rvmap,"Sample2","set Port=/Altip1=/Active1=N/Current1=N/Reset=Y");
		WSSetValue(rvmap,"SampleDesc2","Change alternate ip(s)");
		WSSetValue(rvmap,"Sample3","set Port=/NewAltip=/Reset=Y");
		WSSetValue(rvmap,"SampleDesc3","Add new alternate ip and make it current");
		WSSetValue(rvmap,"Sample4","set Port=/Bounce=/Bstart=/Bend=/Reset=Y");
		WSSetValue(rvmap,"SampleDesc4","Change bounce parameters");
		WSSetValue(rvmap,"Sample5","set Port=/Drives=/Reset=Y");
		WSSetValue(rvmap,"SampleDesc5","Change remote drive list");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// backupapp
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","backupapp");
		WSSetValue(rvmap,"Desc","creates a backup of the app");
		WSSetValue(rvmap,"NumParms","1");
			// Path
			WSSetValue(rvmap,"Parm1","Path");
			WSSetValue(rvmap,"ParmDef1","Versions\\yyyymmdd_hhmmss");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","relative backup location");
		WSSetValue(rvmap,"NumSamples","2");
		WSSetValue(rvmap,"Sample1","backupapp Path=Versions\\\\yyyymmdd_hhmmss");
		WSSetValue(rvmap,"SampleDesc1","backup");
		WSSetValue(rvmap,"Sample2","backupapp Path=Versions\\\\Future_Staging");
		WSSetValue(rvmap,"SampleDesc2","upgrade");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// restoreapp
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","restoreapp");
		WSSetValue(rvmap,"Desc","restores a backup of the app");
		WSSetValue(rvmap,"NumParms","1");
			// Path
			WSSetValue(rvmap,"Parm1","Path");
			WSSetValue(rvmap,"ParmDef1","Versions\\Future_Staging");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","upgrade or rollback location");
		WSSetValue(rvmap,"NumSamples","2");
		WSSetValue(rvmap,"Sample1","restoreapp Path=Versions\\\\Future_Staging");
		WSSetValue(rvmap,"SampleDesc1","upgrade");
		WSSetValue(rvmap,"Sample2","restoreapp Path=Versions\\\\yyyymmdd_hhmmss");
		WSSetValue(rvmap,"SampleDesc2","rollback");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// addevent
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","addevent");
		WSSetValue(rvmap,"Desc","writes text to event log");
		WSSetValue(rvmap,"NumParms","1");
			// Text
			WSSetValue(rvmap,"Parm1","Text");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","text");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","addevent Text=");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// adderror
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","adderror");
		WSSetValue(rvmap,"Desc","writes text to error log");
		WSSetValue(rvmap,"NumParms","1");
			// Text
			WSSetValue(rvmap,"Parm1","Text");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","text");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","adderror Text=");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// netalias
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","netalias");
		WSSetValue(rvmap,"Desc","sets a network alias");
		WSSetValue(rvmap,"NumParms","3");
			// Alias
			WSSetValue(rvmap,"Parm1","Alias");
			WSSetValue(rvmap,"ParmDef1","");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","short alias text");
			// Text
			WSSetValue(rvmap,"Parm2","Text");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","value or empty to remove alias");
			// Clear
			WSSetValue(rvmap,"Parm3","Clear");
			WSSetValue(rvmap,"ParmDef3","Y");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","deletes all aliases");
		WSSetValue(rvmap,"NumSamples","2");
		WSSetValue(rvmap,"Sample1","netalias Alias=/Text=");
		WSSetValue(rvmap,"Sample2","netalias Clear=Y");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		// showwindow
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","showwindow");
		WSSetValue(rvmap,"Desc","controls iqmatrix ui window");
		WSSetValue(rvmap,"NumParms","3");
			// Restore
			WSSetValue(rvmap,"Parm1","Restore");
			WSSetValue(rvmap,"ParmDef1","Y");
			WSSetValue(rvmap,"ParmOpt1","Y");
			WSSetValue(rvmap,"ParmDesc1","restores window and bring to top");
			// Hide
			WSSetValue(rvmap,"Parm2","Hide");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","minimizes window");
			// Maximize
			WSSetValue(rvmap,"Parm3","Maximize");
			WSSetValue(rvmap,"ParmDef3","Y");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","maximizes window");
		WSSetValue(rvmap,"NumSamples","3");
		WSSetValue(rvmap,"Sample1","showwindow Restore=Y");
		WSSetValue(rvmap,"Sample2","showwindow Hide=Y");
		WSSetValue(rvmap,"Sample3","showwindow Maximize=Y");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);

		#ifdef WSOCKS_SSL
		// reloadcerts
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloadcerts");
		WSSetValue(rvmap,"Desc","reload SSL certificates");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloadcerts");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);
		#endif

		// common commands help
		scmds.SMCNotifyRequest(reqid,cmd,parm,plen,(WSPortType)PortType,PortNo);

		// end help
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"End","Y");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(PortNo,reqid,cmd,parms,0);
		return;
	}
	// For unsubscribe, send "Hist=N|Live=N|"
	else if(cmd=="subscribe")
	{
		string feed=ccodec->GetValue(tvmap,"Feed");
		// Must be valid feed target (i.e. aclass.aname)
		const char *dptr=strrchr(dtid.c_str(),'/');
		if(dptr) dptr++;
		else dptr=dtid.c_str();
		if(!strchr(dptr,'.'))
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Invalid subscribe 'DeliverTo'|",
				dtid.c_str(),oboi.c_str());
			rptr=resp;
			ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)strlen(resp),resp,PortNo);
			return;
		}
		char skey[256]={0};
		sprintf(skey,"%s",dptr);
		_strupr(skey);
		// Built-in health attributes (e.g. sysmon)
		if(feed=="level1")
		{
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
			{
                if(!taskmgr.FindTask(fuser,skey,SMTASK_LEVEL1_HIST))
                    taskmgr.AddTask(fuser,skey,SMTASK_LEVEL1_HIST,umflags,reqid,_SendLvl1Quote);
				if((pmod)&&(pmod->fcodec))
				{
					HANDLE ihnd=0;
					char heading[256]={0},status[256]={0};
					phost->WSHGetAppHead(pmod,heading,sizeof(heading));
					phost->WSHGetAppStatus(pmod,status,sizeof(status));
					while(pmod->fcodec->EncodeLvl1(rptr,rlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),heading,status,pmod,true,ihnd,false)>0)
					{
						if(rptr>resp)
						{
							SMFeedHint fhint={resp,(int)(rptr -resp)};
							_SendLvl1Quote(fuser,feed,SMTASK_LEVEL1,reqid,umflags,&fhint);
							rptr=resp;
						}
					}
					sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Feed=%s|Hist=Y|",
						dtid.c_str(),oboi.c_str(),feed.c_str());
					rptr=resp;
					ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)strlen(resp),resp,PortNo);
				}
			}
			else if(hist=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_LEVEL1_HIST);

			if(live=="Y")
			{
				string interval=ccodec->GetValue(tvmap,"Interval");
				int ival=atol(interval.c_str());
				if(ival<1000) ival=1000;
				if(pmod)
					pmod->lvl1Interval=ival;
                if(!taskmgr.FindTask(fuser,skey,SMTASK_LEVEL1))
                    taskmgr.AddTask(fuser,skey,SMTASK_LEVEL1,umflags,reqid,_SendLvl1Quote);
			}
			else if(live=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_LEVEL1);
			if(pmod)
				return;
		}
		// Detailed port attributes (e.g. mportclient)
		else if(feed=="level2")
		{
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
			{
                if(!taskmgr.FindTask(fuser,skey,SMTASK_LEVEL2_HIST))
                    taskmgr.AddTask(fuser,skey,SMTASK_LEVEL2_HIST,umflags,reqid,_SendLvl2Quote);
				if((pmod)&&(pmod->fcodec))
				{
					HANDLE ihnd=0;
					char heading[256]={0},status[256]={0};
					phost->WSHGetAppHead(pmod,heading,sizeof(heading));
					phost->WSHGetAppStatus(pmod,status,sizeof(status));
					while(pmod->fcodec->EncodeLvl2(rptr,rlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),heading,status,pmod,true,ihnd,false)>0)
					{
						if(rptr>resp)
						{
							SMFeedHint fhint={resp,(int)(rptr -resp)};
							_SendLvl2Quote(fuser,feed,SMTASK_LEVEL1,reqid,umflags,&fhint);
							rptr=resp;
						}
					}
					sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Feed=%s|Hist=Y|",
						dtid.c_str(),oboi.c_str(),feed.c_str());
					ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)strlen(resp),resp,PortNo);
				}
			}
			else if(hist=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_LEVEL2_HIST);

			if(live=="Y")
			{
				string interval=ccodec->GetValue(tvmap,"Interval");
				int ival=atol(interval.c_str());
				if(ival<1000) ival=1000;
				if(pmod)
					pmod->lvl2Interval=ival;
                if(!taskmgr.FindTask(fuser,skey,SMTASK_LEVEL2))
                    taskmgr.AddTask(fuser,skey,SMTASK_LEVEL2,umflags,reqid,_SendLvl2Quote);
			}
			else if(live=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_LEVEL2);
			if(pmod)
				return;
		}
		// Log feeds
		else if(feed=="errorlog")
		{
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
			{
                if(!taskmgr.FindTask(fuser,skey,SMTASK_ERRORLOG_HIST))
                    taskmgr.AddTask(fuser,skey,SMTASK_ERRORLOG_HIST,umflags,reqid,_SendErrorLog);
				if(pmod)
				{
					const char *fname=0;
					__int64 begin=_atoi64(ccodec->GetValue(tvmap,"begin").c_str());
					__int64 end=_atoi64(ccodec->GetValue(tvmap,"end").c_str());
					__int64 tot=0;
					int rc=SendLogHist(PortNo,pmod,pmod->WSErrorLogPath.c_str(),fname,begin,end,tot);
					#ifdef WIN32
					if(rc<0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%I64d|End=%I64d|Tot=%I64d|Reason=Failed opening log|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else if(rc>0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%I64d|End=%I64d|Tot=%I64d|Reason=More...|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%I64d|End=%I64d|Tot=%I64d|Text=Hist delivered|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					#else
					if(rc<0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%lld|End=%lld|Tot=%lld|Reason=Failed opening log|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else if(rc>0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%lld|End=%lld|Tot=%lld|Reason=More...|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%lld|End=%lld|Tot=%lld|Text=Hist delivered|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					#endif
					ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)strlen(resp),resp,PortNo);
				}
			}
			else if(hist=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_ERRORLOG_HIST);

			if(live=="Y")
            {
                if(!taskmgr.FindTask(fuser,skey,SMTASK_ERRORLOG))
                    taskmgr.AddTask(fuser,skey,SMTASK_ERRORLOG,umflags,reqid,_SendErrorLog);
            }
			else if(live=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_ERRORLOG);
			if(pmod)
				return;
		}
		else if(feed=="eventlog")
		{
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
			{
                if(!taskmgr.FindTask(fuser,skey,SMTASK_EVENTLOG_HIST))
                    taskmgr.AddTask(fuser,skey,SMTASK_EVENTLOG_HIST,umflags,reqid,_SendEventLog);
				if(pmod)
				{
					const char *fname=0;
					__int64 begin=_atoi64(ccodec->GetValue(tvmap,"begin").c_str());
					__int64 end=_atoi64(ccodec->GetValue(tvmap,"end").c_str());
					__int64 tot=0;
					int rc=SendLogHist(PortNo,pmod,pmod->WSEventLogPath.c_str(),fname,begin,end,tot);
					#ifdef WIN32
					if(rc<0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%I64d|End=%I64d|Tot=%I64d|Reason=Failed opening log|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else if(rc>0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%I64d|End=%I64d|Tot=%I64d|Reason=More...|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%I64d|End=%I64d|Tot=%I64d|Text=Hist delivered|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					#else
					if(rc<0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%lld|End=%lld|Tot=%lld|Reason=Failed opening log|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else if(rc>0)
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%lld|End=%lld|Tot=%lld|Reason=More...|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					else
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Feed=%s|Hist=Y|AppClass=%s|AppName=%s|FileKey=%s|Begin=%lld|End=%lld|Tot=%lld|Text=Hist delivered|",
							dtid.c_str(),oboi.c_str(),feed.c_str(),pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fname,begin,end,tot);
					#endif
					ccodec->Encode(rptr,rlen,false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)strlen(resp),resp,PortNo);
				}
			}
			else if(hist=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_EVENTLOG_HIST);

			if(live=="Y")
            {
                if(!taskmgr.FindTask(fuser,skey,SMTASK_EVENTLOG))
                    taskmgr.AddTask(fuser,skey,SMTASK_EVENTLOG,umflags,reqid,_SendEventLog);
            }
			else if(live=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_EVENTLOG);
			if(pmod)
				return;
		}
		else if(feed=="trademon")
		{
			// Drop through for app-specific feeds (e.g. "orders")
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
            {
                if(!taskmgr.FindTask(fuser,skey,SMTASK_TRADEMON_HIST))
                    taskmgr.AddTask(fuser,skey,SMTASK_TRADEMON_HIST,umflags,reqid,_SendTrademon);
            }
			else if(hist=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_TRADEMON_HIST);

			if(live=="Y")
            {
                if(!taskmgr.FindTask(fuser,skey,SMTASK_TRADEMON))
                    taskmgr.AddTask(fuser,skey,SMTASK_TRADEMON,umflags,reqid,_SendTrademon);
            }
			else if(live=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_TRADEMON);
			if(pmod)
				return;
		}
		else
		{
			// Drop through for app-specific feeds (e.g. "orders")
			string hist=ccodec->GetValue(tvmap,"Hist");
			string live=ccodec->GetValue(tvmap,"Live");
			if(hist=="Y")
				taskmgr.AddTask(fuser,skey,SMTASK_CUSTOM_HIST,umflags,reqid,_SendCustom);
			else if(hist=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_CUSTOM_HIST);

			if(live=="Y")
				taskmgr.AddTask(fuser,skey,SMTASK_CUSTOM,umflags,reqid,_SendCustom);
			else if(live=="N")
				taskmgr.DelTask(fuser,skey,SMTASK_CUSTOM);
			if(pmod)
				return;
		}
	}
	// Exit app
	else if((cmd=="exit")&&(pmod))
	{
		string user=ccodec->GetValue(tvmap,"User");
		if(user.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'User' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		string reason=ccodec->GetValue(tvmap,"Reason");
		if(reason.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Reason' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Exiting...|",
			dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		phost->WSHLogError(pmod,"Instructed to exit by %s/%s for %s!",oboi.c_str(),user.c_str(),reason.c_str());
		phost->WSHExitApp(pmod,86);
		return;
	}
	// Restart app
	else if((cmd=="restart")&&(pmod))
	{
		string user=ccodec->GetValue(tvmap,"User");
		if(user.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'User' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		string reason=ccodec->GetValue(tvmap,"Reason");
		if(reason.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Reason' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Exiting...|",
			dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		phost->WSHLogError(pmod,"Instructed to exit by %s/%s for %s!",oboi.c_str(),user.c_str(),reason.c_str());
		pmod->WSRestart(60);
		return;
	}
	// IQMatrix Thread Information
	else if(cmd=="threadlist")
	{
		WsocksHostImpl *phi=(WsocksHostImpl*)phost;
		for(WSTHREADMAP::iterator tit=phi->threadBegin();tit!=phi->threadEnd();tit++)
		{
			WsocksThread *pthread=tit->second;
			DWORD tnow=GetTickCount();
			int rc = 0;
			//Renuka
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|ThreadName=%s|Tid=%d|Abort=%d|AppList=%zd|UnloadList=%zd|ReloadList=%zd|LoadList=%zd|LastTickWarn=%l|LastTickErr=%d|AbortTimeout=%d|RestartCnt=%d|StartTime=%d|MuxTrail=%s|",
				dtid.c_str(),oboi.c_str(), rc,pthread->name.c_str(),pthread->tid,pthread->abort,
				pthread->appList.size(),pthread->unloadList.size(),pthread->reloadList.size(),pthread->loadList.size(),
				tnow -pthread->lastTickWarn,tnow -pthread->lastTickErr,pthread->threadAbortTimeout,pthread->restartCnt,pthread->startTime,pthread->muxtrail);
			rptr=resp;
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		}
        return;
	}
	else if(cmd=="threadkill")
	{
		string name=ccodec->GetValue(tvmap,"Name");
		string restart=ccodec->GetValue(tvmap,"Restart");
		WSLogEvent("USR%d: Threadkill(%s,%s) from (%s)",PortNo,name.c_str(),restart.c_str(),oboi.c_str());
		if(name.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Name' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		if(restart.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Restart' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		WsocksHostImpl *phi=(WsocksHostImpl*)phost;
		int rc=-1;
		char text[256]={0};
		sprintf(text,"Reason=Thread %s not found",name.c_str());
		for(WSTHREADMAP::iterator tit=phi->threadBegin();tit!=phi->threadEnd();tit++)
		{
			WsocksThread *pthread=tit->second;
			if(pthread->name==name)
			{
			#ifdef WIN32
				phi->WSHGenerateDump(pmod,pthread->tid,0);
				HANDLE thnd=pthread->thnd;
				rc=TerminateThread(thnd,69);
				if(rc)
				{
					rc=0;
					sprintf(text,"Text=Thread(%s) %d terminated",pthread->name.c_str(),pthread->tid);
					phi->WSHThreadCrashed(pthread,(restart=="N"||restart=="n")?false:true);
				}
			#else
				_ASSERT(false);//unfinished
			#endif
				break;
			}
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|%s|",
			dtid.c_str(),oboi.c_str(),rc,text);
		rptr=resp;
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
        return;
	}
	// Remote port dialogs
	else if((cmd=="bounce")&&(pmod))
	{
		string port=ccodec->GetValue(tvmap,"Port");
		WSLogEvent("USR%d: Port bounce(%s) from (%s)",PortNo,parm,oboi.c_str());
		if(port.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Port' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		WSPortType PortType=WS_UNKNOWN;
		int PortNo=-1;
		if(GetPortSpec(pmod,port.c_str(),PortType,PortNo)<0)
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Invalid 'Port' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		switch(PortType)
		{
		case WS_CON: pmod->WSCloseConPort(PortNo); break;
		case WS_USC: pmod->WSCloseUscPort(PortNo); break;
		case WS_USR: pmod->WSCloseUsrPort(PortNo); break;
		#ifdef WS_FILE_SERVER
		case WS_FIL: pmod->WSCloseFilePort(PortNo); break;
		#endif
		case WS_UMC: pmod->WSCloseUmcPort(PortNo); break;
		case WS_UMR: pmod->WSCloseUmrPort(PortNo); break;
		case WS_CGD: pmod->WSCloseCgdPort(PortNo); break;
		case WS_UGC: pmod->WSCloseUgcPort(PortNo); break;
		case WS_UGR: pmod->WSCloseUgrPort(PortNo); break;
		case WS_CTO: pmod->WSCloseCtoPort(PortNo); break;
		case WS_CTI: pmod->WSCloseCtiPort(PortNo); break;
		};
        return;
	}
	else if((cmd=="set")&&(pmod))
	{
		string port=ccodec->GetValue(tvmap,"Port");
		WSLogEvent("USR%d: Port set(%s) from (%s)",PortNo,parm,oboi.c_str());
		if(port.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Port' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		WSPortType PortType=WS_UNKNOWN;
		int CmdPortNo=-1;
		if(GetPortSpec(pmod,port.c_str(),PortType,CmdPortNo)<0)
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Invalid 'Port' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		string ip=ccodec->GetValue(tvmap,"Ip");
		if(ccodec->HasValue(tvmap,"Ip"))
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Ip' not supported for specified port.|",
				dtid.c_str(),oboi.c_str());
			rptr=resp;
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			char ipstr[256]={0};
			strncpy(ipstr,ip.c_str(),sizeof(ipstr));
			ipstr[sizeof(ipstr) -1]=0;
			phost->WSHTranslateAlias(ipstr,sizeof(ipstr));
			switch(PortType)
			{
			case WS_CON: 
				strncpy(pmod->ConPort[CmdPortNo].RemoteIP,ipstr,sizeof(pmod->ConPort[CmdPortNo].RemoteIP)-1);
				strncpy(pmod->ConPort[CmdPortNo].AltRemoteIP[0],ipstr,sizeof(pmod->ConPort[CmdPortNo].AltRemoteIP[0])-1);
				pmod->ConPort[CmdPortNo].AltRemoteIPOn[0]=true;
				break;
			case WS_USR: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			#ifdef WS_FILE_SERVER
			case WS_FIL:
				strncpy(pmod->FilePort[CmdPortNo].RemoteIP,ipstr,sizeof(pmod->FilePort[CmdPortNo].RemoteIP)-1);
				strncpy(pmod->FilePort[CmdPortNo].AltRemoteIP[0],ipstr,sizeof(pmod->FilePort[CmdPortNo].AltRemoteIP[0])-1);
				pmod->FilePort[CmdPortNo].AltRemoteIPOn[0]=true;
				break;
			#endif
			case WS_UMR: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			case WS_CGD:
				strncpy(pmod->CgdPort[CmdPortNo].RemoteIP,ipstr,sizeof(pmod->CgdPort[CmdPortNo].RemoteIP)-1);
				strncpy(pmod->CgdPort[CmdPortNo].AltRemoteIP[0],ipstr,sizeof(pmod->CgdPort[CmdPortNo].AltRemoteIP[0])-1);
				pmod->CgdPort[CmdPortNo].AltRemoteIPOn[0]=true;
				break;
			case WS_UGR: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			case WS_CTO:
				strncpy(pmod->CtoPort[CmdPortNo].RemoteIP,ipstr,sizeof(pmod->CtoPort[CmdPortNo].RemoteIP)-1);
				break;
			case WS_CTI: 
				strncpy(pmod->CtiPort[CmdPortNo].RemoteIP,ipstr,sizeof(pmod->CtiPort[CmdPortNo].RemoteIP)-1);
				strncpy(pmod->CtiPort[CmdPortNo].AltRemoteIP[0],ipstr,sizeof(pmod->CtiPort[CmdPortNo].AltRemoteIP[0])-1);
				pmod->CtiPort[CmdPortNo].AltRemoteIPOn[0]=true;
				break;
			default:
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		}
		string tcpport=ccodec->GetValue(tvmap,"Tcpport");
		if(!tcpport.empty())
		{
			int tval=atoi(tcpport.c_str());
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Tcpport' not supported for specified port.|",
				dtid.c_str(),oboi.c_str());
			rptr=resp;
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			switch(PortType)
			{
			case WS_CON: pmod->ConPort[CmdPortNo].Port=tval; break;
			case WS_USC: pmod->UscPort[CmdPortNo].Port=tval; break;
			case WS_USR: WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); return;
			#ifdef WS_FILE_SERVER
			case WS_FIL: pmod->FilePort[CmdPortNo].Port=tval; break;
			#endif
			case WS_UMC: pmod->UmcPort[CmdPortNo].Port=tval; break;
			case WS_UMR: WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); return;
			case WS_CGD: pmod->CgdPort[CmdPortNo].Port=tval; break;
			case WS_UGC: pmod->UgcPort[CmdPortNo].Port=tval; break;
			case WS_UGR: WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); return;
			case WS_CTO: pmod->CtoPort[CmdPortNo].Port=tval; break;
			case WS_CTI: pmod->CtiPort[CmdPortNo].Port=tval; break;
			default: WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); return;
			};
		}
		string hold=ccodec->GetValue(tvmap,"Hold");
		if(!hold.empty())
		{
			bool hval=(hold=="Y"||hold=="y")?true:false;
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Hold' not supported for specified port.|",
				dtid.c_str(),oboi.c_str());
			rptr=resp;
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			switch(PortType)
			{
			case WS_CON: 
				pmod->ConPort[CmdPortNo].ConnectHold=hval; 
				#ifdef WS_DECLINING_RECONNECT
				if(!pmod->ConPort[CmdPortNo].ConnectHold)
				{
					pmod->ConPort[CmdPortNo].ReconnectDelay=0;
					pmod->ConPort[CmdPortNo].ReconnectTime=0;
				}
				#endif
				break;
			case WS_USC: 
				pmod->UscPort[CmdPortNo].ConnectHold=hval; 
				pmod->UscPort[CmdPortNo].Status[0]=0;
				break;
			case WS_USR: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			#ifdef WS_FILE_SERVER
			case WS_FIL: 
				pmod->FilePort[CmdPortNo].ConnectHold=hval; 
				#ifdef WS_DECLINING_RECONNECT
				if(!pmod->FilePort[CmdPortNo].ConnectHold)
				{
					pmod->FilePort[CmdPortNo].ReconnectDelay=0;
					pmod->FilePort[CmdPortNo].ReconnectTime=0;
				}
				#endif
				break;
			#endif
			case WS_UMC: 
				pmod->UmcPort[CmdPortNo].ConnectHold=hval; 
				pmod->UmcPort[CmdPortNo].Status[0]=0;
				break;
			case WS_UMR: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			case WS_CGD: 
				pmod->CgdPort[CmdPortNo].ConnectHold=hval; 
				#ifdef WS_DECLINING_RECONNECT
				if(!pmod->CgdPort[CmdPortNo].ConnectHold)
				{
					pmod->CgdPort[CmdPortNo].ReconnectDelay=0;
					pmod->CgdPort[CmdPortNo].ReconnectTime=0;
				}
				#endif
				break;
			case WS_UGC: 
				pmod->UgcPort[CmdPortNo].ConnectHold=hval; 
				pmod->UgcPort[CmdPortNo].Status[0]=0;
				break;
			case WS_UGR: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			case WS_CTO: 
				pmod->CtoPort[CmdPortNo].ConnectHold=hval; 
				pmod->CtoPort[CmdPortNo].Status[0]=0;
				break;
			case WS_CTI: 
				pmod->CtiPort[CmdPortNo].ConnectHold=hval; 
				pmod->CtiPort[CmdPortNo].Status[0]=0;
				break;
			default:
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		}
		string rec=ccodec->GetValue(tvmap,"Rec");
		if(!rec.empty())
		{
			bool DoRec=(rec=="Y"||rec=="y")?true:false;
            string reset = ccodec->GetValue(tvmap,"reset");
            bool bReset = (reset == "Y" || reset == "y");
			switch(PortType)
			{
			case WS_CON:
                if(!bReset)
                {
				    if(DoRec)
					    pmod->WSOpenRecording(&pmod->ConPort[CmdPortNo].Recording,pmod->WShWnd,PortType,CmdPortNo);
				    else
					    pmod->WSCloseRecording(&pmod->ConPort[CmdPortNo].Recording,PortType,CmdPortNo);
                }
                pmod->ConPort[CmdPortNo].DoRecOnOpen = DoRec;
				break;
			case WS_USR:
				if(DoRec)
					pmod->WSOpenRecording(&pmod->UsrPort[CmdPortNo].Recording,pmod->WShWnd,PortType,CmdPortNo);
				else
					pmod->WSCloseRecording(&pmod->UsrPort[CmdPortNo].Recording,PortType,CmdPortNo);
				break;
			case WS_FIL:
                if(!bReset)
                {
				    if(DoRec)
					    pmod->WSOpenRecording(&pmod->FilePort[CmdPortNo].Recording,pmod->WShWnd,PortType,CmdPortNo);
				    else
					    pmod->WSCloseRecording(&pmod->FilePort[CmdPortNo].Recording,PortType,CmdPortNo);
                }
                pmod->FilePort[CmdPortNo].DoRecOnOpen = DoRec;
				break;
			case WS_UMR:
				if(DoRec)
					pmod->WSOpenRecording(&pmod->UmrPort[CmdPortNo].Recording,pmod->WShWnd,PortType,CmdPortNo);
				else
					pmod->WSCloseRecording(&pmod->UmrPort[CmdPortNo].Recording,PortType,CmdPortNo);
				break;
			case WS_CGD:
                if(!bReset)
                {
				    if(DoRec)
					    pmod->WSOpenRecording(&pmod->CgdPort[CmdPortNo].Recording,pmod->WShWnd,PortType,CmdPortNo);
				    else
					    pmod->WSCloseRecording(&pmod->CgdPort[CmdPortNo].Recording,PortType,CmdPortNo);
                }
                pmod->CgdPort[CmdPortNo].DoRecOnOpen = DoRec;
				break;
			case WS_UGR:
				if(DoRec)
					pmod->WSOpenRecording(&pmod->UgrPort[CmdPortNo].Recording,pmod->WShWnd,PortType,CmdPortNo);
				else
					pmod->WSCloseRecording(&pmod->UgrPort[CmdPortNo].Recording,PortType,CmdPortNo);
				break;
			case WS_USC:
				if(pmod->UscPort[CmdPortNo].Recording.DoRec!=(BOOL)DoRec)
				{
					pmod->UscPort[CmdPortNo].Recording.DoRec=DoRec;
					if(DoRec)
						pmod->WSLogEvent("Auto-Record USC%d ON",CmdPortNo);
					else
						pmod->WSLogEvent("Auto-Record USC%d OFF",CmdPortNo);
				}
				break;
			case WS_UMC:
				if(pmod->UmcPort[CmdPortNo].Recording.DoRec!=(BOOL)DoRec)
				{
					pmod->UmcPort[CmdPortNo].Recording.DoRec=DoRec;
					if(DoRec)
						pmod->WSLogEvent("Auto-Record UMC%d ON",CmdPortNo);
					else
						pmod->WSLogEvent("Auto-Record UMC%d OFF",CmdPortNo);
				}
				break;
			case WS_UGC:
				if(pmod->UgcPort[CmdPortNo].Recording.DoRec!=(BOOL)DoRec)
				{
					pmod->UgcPort[CmdPortNo].Recording.DoRec=DoRec;
					if(DoRec)
						pmod->WSLogEvent("Auto-Record UGC%d ON",CmdPortNo);
					else
						pmod->WSLogEvent("Auto-Record UGC%d OFF",CmdPortNo);
				}
				break;
			};
		}
		string note=ccodec->GetValue(tvmap,"Note");
		if(ccodec->HasValue(tvmap,"Note"))
		{
			switch(PortType)
			{
			case WS_CON: strncpy(pmod->ConPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->ConPort[CmdPortNo].CfgNote)-1); break;
			case WS_USC: strncpy(pmod->UscPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->UscPort[CmdPortNo].CfgNote)-1); break;
			case WS_USR: strncpy(pmod->UsrPort[CmdPortNo].Note,note.c_str(),sizeof(pmod->UsrPort[CmdPortNo].Note)-1); break;
			#ifdef WS_FILE_SERVER
			case WS_FIL: strncpy(pmod->FilePort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->FilePort[CmdPortNo].CfgNote)-1); break;
			#endif
			case WS_UMC: strncpy(pmod->UmcPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->UmcPort[CmdPortNo].CfgNote)-1); break;
			case WS_UMR: strncpy(pmod->UmrPort[CmdPortNo].Note,note.c_str(),sizeof(pmod->UmrPort[CmdPortNo].Note)-1); break;
			case WS_CGD: strncpy(pmod->CgdPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->CgdPort[CmdPortNo].CfgNote)-1); break;
			case WS_UGC: strncpy(pmod->UgcPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->UgcPort[CmdPortNo].CfgNote)-1); break;
			case WS_UGR: strncpy(pmod->UgrPort[CmdPortNo].Note,note.c_str(),sizeof(pmod->UgrPort[CmdPortNo].Note)-1); break;
			case WS_CTO: strncpy(pmod->CtoPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->CtoPort[CmdPortNo].CfgNote)-1); break;
			case WS_CTI: strncpy(pmod->CtiPort[CmdPortNo].CfgNote,note.c_str(),sizeof(pmod->CtiPort[CmdPortNo].CfgNote)-1); break;
			};
		}
        string compressed=ccodec->GetValue(tvmap,"Comp");
        if(!compressed.empty())
        {
            switch(PortType)
            {
                case WS_CON: pmod->ConPort[CmdPortNo].Compressed=compressed[0]=='Y'; break;
                case WS_USC: pmod->UscPort[CmdPortNo].Compressed=compressed[0]=='Y'; break;
                case WS_FIL: pmod->FilePort[CmdPortNo].Compressed=compressed[0]=='Y'; break;
                case WS_UMC: pmod->UmcPort[CmdPortNo].Compressed=compressed[0]=='Y'; break;
                case WS_CGD: pmod->CgdPort[CmdPortNo].Compressed=compressed[0]=='Y'; break;
                case WS_UGC: pmod->UgcPort[CmdPortNo].Compressed=compressed[0]=='Y'; break;
            }
        }
        string compressType=ccodec->GetValue(tvmap,"CompType");
        if(!compressType.empty())
        {
            switch(PortType)
            {
                case WS_CON: pmod->ConPort[CmdPortNo].CompressType=atoi(compressType.c_str()); break;
                case WS_USC: pmod->UscPort[CmdPortNo].CompressType=atoi(compressType.c_str()); break;
            }
        }
        string encryptType=ccodec->GetValue(tvmap,"EncryptType");
        if(!encryptType.empty())
        {
            switch(PortType)
            {
                case WS_CON: pmod->ConPort[CmdPortNo].Encrypted=atoi(encryptType.c_str()); break;
                case WS_USC: pmod->UscPort[CmdPortNo].Encrypted=atoi(encryptType.c_str()); break;
                case WS_FIL: pmod->FilePort[CmdPortNo].Encrypted=atoi(encryptType.c_str()); break;
                case WS_UMC: pmod->UmcPort[CmdPortNo].Encrypted=atoi(encryptType.c_str()); break;
                case WS_CGD: pmod->CgdPort[CmdPortNo].Encrypted=atoi(encryptType.c_str()); break;
                case WS_UGC: pmod->UgcPort[CmdPortNo].Encrypted=atoi(encryptType.c_str()); break;
            }
        }
		string bounce=ccodec->GetValue(tvmap,"Bounce");
		if(!bounce.empty())
		{
			int bval=atoi(bounce.c_str());
			switch(PortType)
			{
			case WS_CON: pmod->ConPort[CmdPortNo].Bounce=bval; break;
			#ifdef WS_FILE_SERVER
			case WS_FIL: pmod->FilePort[CmdPortNo].Bounce=bval; break;
			#endif
			case WS_CGD: pmod->CgdPort[CmdPortNo].Bounce=bval; break;
			case WS_CTI: pmod->CtiPort[CmdPortNo].Bounce=bval; break;
			default: 
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Bounce' not supported for specified port.|",
					dtid.c_str(),oboi.c_str());
				rptr=resp;
				ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		}
		string bstart=ccodec->GetValue(tvmap,"Bstart");
		if(!bstart.empty())
		{
			int bval=atoi(bstart.c_str());
			switch(PortType)
			{
			case WS_CON: pmod->ConPort[CmdPortNo].BounceStart=bval; break;
			#ifdef WS_FILE_SERVER
			case WS_FIL: pmod->FilePort[CmdPortNo].BounceStart=bval; break;
			#endif
			case WS_CGD: pmod->CgdPort[CmdPortNo].BounceStart=bval; break;
			case WS_CTI: pmod->CtiPort[CmdPortNo].BounceStart=bval; break;
			default: 
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Bstart' not supported for specified port.|",
					dtid.c_str(),oboi.c_str());
				rptr=resp;
				ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		}
		string bend=ccodec->GetValue(tvmap,"Bend");
		if(!bend.empty())
		{
			int bval=atoi(bend.c_str());
			switch(PortType)
			{
			case WS_CON: pmod->ConPort[CmdPortNo].BounceEnd=bval; break;
			#ifdef WS_FILE_SERVER
			case WS_FIL: pmod->FilePort[CmdPortNo].BounceEnd=bval; break;
			#endif
			case WS_CGD: pmod->CgdPort[CmdPortNo].BounceEnd=bval; break;
			case WS_CTI: pmod->CtiPort[CmdPortNo].BounceEnd=bval; break;
			default: 
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Bend' not supported for specified port.|",
					dtid.c_str(),oboi.c_str());
				rptr=resp;
				ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		}
		bool checkalts=false;
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
		{
			char pname[32]={0};
			sprintf(pname,"Altip%d",i);
			string altip=ccodec->GetValue(tvmap,pname);
			// empty used to delete
			if(ccodec->HasValue(tvmap,pname))
			{
				char altipstr[256]={0};
				strncpy(altipstr,altip.c_str(),sizeof(altipstr));
				altipstr[sizeof(altipstr) -1]=0;
				phost->WSHTranslateAlias(altipstr,sizeof(altipstr));
				switch(PortType)
				{
				case WS_CON: strncpy(pmod->ConPort[CmdPortNo].AltRemoteIP[i],altipstr,sizeof(pmod->ConPort[CmdPortNo].AltRemoteIP[i])-1); break;
				#ifdef WS_FILE_SERVER
				case WS_FIL: strncpy(pmod->FilePort[CmdPortNo].AltRemoteIP[i],altipstr,sizeof(pmod->FilePort[CmdPortNo].AltRemoteIP[i])-1); break;
				#endif
				case WS_CGD: strncpy(pmod->CgdPort[CmdPortNo].AltRemoteIP[i],altipstr,sizeof(pmod->CgdPort[CmdPortNo].AltRemoteIP[i])-1); break;
				case WS_CTI: strncpy(pmod->CtiPort[CmdPortNo].AltRemoteIP[i],altipstr,sizeof(pmod->CtiPort[CmdPortNo].AltRemoteIP[i])-1); break;
				default:
					sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Altip' not supported for specified port.|",
						dtid.c_str(),oboi.c_str());
					rptr=resp;
					ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
					return;
				};
				checkalts=true;
			}
			sprintf(pname,"Altport%d",i);
			string altport=ccodec->GetValue(tvmap,pname);
			if(!altport.empty())
			{
				int pval=atoi(altport.c_str());
				switch(PortType)
				{
				case WS_CON: 
					pmod->ConPort[CmdPortNo].AltRemotePort[i]=pval;
					break;
				default:
					sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Active' not supported for specified port.|",
						dtid.c_str(),oboi.c_str());
					rptr=resp;
					ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
					return;
				};
				checkalts=true;
			}
			sprintf(pname,"Active%d",i);
			string active=ccodec->GetValue(tvmap,pname);
			if(!active.empty())
			{
				bool aval=(active=="Y"||active=="y")?true:false;
				switch(PortType)
				{
				case WS_CON: 
					pmod->ConPort[CmdPortNo].AltRemoteIPOn[i]=aval; 
					break;
				#ifdef WS_FILE_SERVER
				case WS_FIL: 
					pmod->FilePort[CmdPortNo].AltRemoteIPOn[i]=aval; 
					break;
				#endif
				case WS_CGD: 
					pmod->CgdPort[CmdPortNo].AltRemoteIPOn[i]=aval; 
					break;
				case WS_CTI: 
					pmod->CtiPort[CmdPortNo].AltRemoteIPOn[i]=aval; 
					break;
				default:
					sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Active' not supported for specified port.|",
						dtid.c_str(),oboi.c_str());
					rptr=resp;
					ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
					return;
				};
				checkalts=true;
			}
			sprintf(pname,"Current%d",i);
			string current=ccodec->GetValue(tvmap,pname);
			if(!current.empty())
			{
				bool cval=(current=="Y"||current=="y")?true:false;
				if(cval)
				{
					switch(PortType)
					{
					case WS_CON: pmod->ConPort[CmdPortNo].CurrentAltIP=i-1; break;
					#ifdef WS_FILE_SERVER
					case WS_FIL: pmod->FilePort[CmdPortNo].CurrentAltIP=i-1; break;
					#endif
					case WS_CGD: pmod->CgdPort[CmdPortNo].CurrentAltIP=i-1; break;
					case WS_CTI: pmod->CtiPort[CmdPortNo].CurrentAltIP=i-1; break;
					default:
						sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Current' not supported for specified port.|",
							dtid.c_str(),oboi.c_str());
						rptr=resp;
						ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
						WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
						return;
					};
					checkalts=true;
				}
			}
		}
		if(checkalts)
		{
			// Remove empty alternates and adjust alt count
			int aidx=0;
			for(int i=0;i<WS_MAX_ALT_PORTS -1;i++)
			{
				switch(PortType)
				{
				case WS_CON: 
					if(pmod->ConPort[CmdPortNo].AltRemoteIP[i][0])
					{
						if(i>aidx)
						{
							strcpy(pmod->ConPort[CmdPortNo].AltRemoteIP[aidx],pmod->ConPort[CmdPortNo].AltRemoteIP[i]);
							pmod->ConPort[CmdPortNo].AltRemotePort[aidx]=pmod->ConPort[CmdPortNo].AltRemotePort[i];
							strcpy(pmod->ConPort[CmdPortNo].AltRemoteIP[i],"");
						}
						aidx++;
					}
					break;
				#ifdef WS_FILE_SERVER
				case WS_FIL: 
					if(pmod->FilePort[CmdPortNo].AltRemoteIP[i][0])
					{
						if(i>aidx)
						{
							strcpy(pmod->FilePort[CmdPortNo].AltRemoteIP[aidx],pmod->FilePort[CmdPortNo].AltRemoteIP[i]);
							strcpy(pmod->FilePort[CmdPortNo].AltRemoteIP[i],"");
						}
						aidx++;
					}
					break;
				#endif
				case WS_CGD: 
					if(pmod->CgdPort[CmdPortNo].AltRemoteIP[i][0])
					{
						if(i>aidx)
						{
							strcpy(pmod->CgdPort[CmdPortNo].AltRemoteIP[aidx],pmod->CgdPort[CmdPortNo].AltRemoteIP[i]);
							strcpy(pmod->CgdPort[CmdPortNo].AltRemoteIP[i],"");
						}
						aidx++;
					}
					break;
				case WS_CTI:
					if(pmod->CtiPort[CmdPortNo].AltRemoteIP[i][0])
					{
						if(i>aidx)
						{
							strcpy(pmod->CtiPort[CmdPortNo].AltRemoteIP[aidx],pmod->CtiPort[CmdPortNo].AltRemoteIP[i]);
							strcpy(pmod->CtiPort[CmdPortNo].AltRemoteIP[i],"");
						}
						aidx++;
					}
					break;
				};
			}
			switch(PortType)
			{
			case WS_CON: 
				if(pmod->ConPort[CmdPortNo].AltIPCount!=aidx)
				{
					pmod->ConPort[CmdPortNo].AltIPCount=aidx;
					if(pmod->ConPort[CmdPortNo].CurrentAltIP>=pmod->ConPort[CmdPortNo].AltIPCount)
						pmod->ConPort[CmdPortNo].CurrentAltIP=pmod->ConPort[CmdPortNo].AltIPCount -1;
				}
				break;
			#ifdef WS_FILE_SERVER
			case WS_FIL: 
				if(pmod->FilePort[CmdPortNo].AltIPCount!=aidx)
				{
					pmod->FilePort[CmdPortNo].AltIPCount=aidx;
					if(pmod->FilePort[CmdPortNo].CurrentAltIP>=pmod->FilePort[CmdPortNo].AltIPCount)
						pmod->FilePort[CmdPortNo].CurrentAltIP=pmod->FilePort[CmdPortNo].AltIPCount -1;
				}
				break;
			#endif
			case WS_CGD: 
				if(pmod->CgdPort[CmdPortNo].AltIPCount!=aidx)
				{
					pmod->CgdPort[CmdPortNo].AltIPCount=aidx;
					if(pmod->CgdPort[CmdPortNo].CurrentAltIP>=pmod->CgdPort[CmdPortNo].AltIPCount)
						pmod->CgdPort[CmdPortNo].CurrentAltIP=pmod->ConPort[CmdPortNo].AltIPCount -1;
				}
				break;
			case WS_CTI:
				if(pmod->CtiPort[CmdPortNo].AltIPCount!=aidx)
				{
					pmod->CtiPort[CmdPortNo].AltIPCount=aidx;
					if(pmod->CtiPort[CmdPortNo].CurrentAltIP>=pmod->CtiPort[CmdPortNo].AltIPCount)
						pmod->CtiPort[CmdPortNo].CurrentAltIP=pmod->CtiPort[CmdPortNo].AltIPCount -1;
				}
				break;
			}
		}
		string newaltip=ccodec->GetValue(tvmap,"NewAltip");
		if(ccodec->HasValue(tvmap,"NewAltip"))
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=No available alternative IPs.|",
				dtid.c_str(),oboi.c_str());
			rptr=resp;
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			char ipstr[256]={0};
			strncpy(ipstr,newaltip.c_str(),sizeof(ipstr));
			ipstr[sizeof(ipstr) -1]=0;
			phost->WSHTranslateAlias(ipstr,sizeof(ipstr));
			switch(PortType)
			{
			case WS_CON: 
			{
				int a;
				for(a=0;(a<WS_MAX_ALT_PORTS)&&(pmod->ConPort[CmdPortNo].AltRemoteIP[a][0]);a++)
					;
				if(a<WS_MAX_ALT_PORTS)
				{
					strncpy(pmod->ConPort[CmdPortNo].AltRemoteIP[a],ipstr,sizeof(pmod->ConPort[CmdPortNo].AltRemoteIP[a])-1);
					pmod->ConPort[CmdPortNo].AltRemoteIPOn[a]=true;
					pmod->ConPort[CmdPortNo].AltIPCount++;
					pmod->ConPort[CmdPortNo].CurrentAltIP=a -1;
				}
				else
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				break;
			}
			#ifdef WS_FILE_SERVER
			case WS_FIL:
			{
				int a;
				for(a=0;(a<WS_MAX_ALT_PORTS)&&(pmod->FilePort[CmdPortNo].AltRemoteIP[a][0]);a++)
					;
				if(a<WS_MAX_ALT_PORTS)
				{
					strncpy(pmod->FilePort[CmdPortNo].AltRemoteIP[a],ipstr,sizeof(pmod->FilePort[CmdPortNo].AltRemoteIP[a])-1);
					pmod->FilePort[CmdPortNo].AltRemoteIPOn[a]=true;
					pmod->FilePort[CmdPortNo].AltIPCount++;
					pmod->FilePort[CmdPortNo].CurrentAltIP=a -1;
				}
				else
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				break;
			}
			#endif
			case WS_CGD:
			{
				int a;
				for(a=0;(a<WS_MAX_ALT_PORTS)&&(pmod->CgdPort[CmdPortNo].AltRemoteIP[a][0]);a++)
					;
				if(a<WS_MAX_ALT_PORTS)
				{
					strncpy(pmod->CgdPort[CmdPortNo].AltRemoteIP[a],ipstr,sizeof(pmod->CgdPort[CmdPortNo].AltRemoteIP[a])-1);
					pmod->CgdPort[CmdPortNo].AltRemoteIPOn[a]=true;
					pmod->CgdPort[CmdPortNo].AltIPCount++;
					pmod->CgdPort[CmdPortNo].CurrentAltIP=a -1;
				}
				else
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				break;
			}
			case WS_CTI: 
			{
				int a;
				for(a=0;(a<WS_MAX_ALT_PORTS)&&(pmod->CtiPort[CmdPortNo].AltRemoteIP[a][0]);a++)
					;
				if(a<WS_MAX_ALT_PORTS)
				{
					strncpy(pmod->CtiPort[CmdPortNo].AltRemoteIP[a],ipstr,sizeof(pmod->CtiPort[CmdPortNo].AltRemoteIP[a])-1);
					pmod->CtiPort[CmdPortNo].AltRemoteIPOn[a]=true;
					pmod->CtiPort[CmdPortNo].AltIPCount++;
					pmod->CtiPort[CmdPortNo].CurrentAltIP=a -1;
				}
				break;
			}
			default:
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='NewAltip' not supported for specified port.|",
					dtid.c_str(),oboi.c_str());
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		}
		string drives=ccodec->GetValue(tvmap,"Drives");
		if(!drives.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='Bend' not supported for specified port.|",
				dtid.c_str(),oboi.c_str());
			rptr=resp;
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		#ifdef WS_FILE_SERVER
			switch(PortType)
			{
			case WS_FIL: strncpy(pmod->FilePort[CmdPortNo].DriveList,drives.c_str(),sizeof(pmod->FilePort[CmdPortNo].DriveList)-1); break;
			default: 
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
				return;
			};
		#else
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo); 
			return;
		#endif
		}
		// Reset should be handled last
		string reset=ccodec->GetValue(tvmap,"Reset");
		if((reset=="Y")||(reset=="y"))
		{
			switch(PortType)
			{
			case WS_CON: pmod->WSCloseConPort(CmdPortNo); break;
			case WS_USC: pmod->WSCloseUscPort(CmdPortNo); break;
			case WS_USR: pmod->WSCloseUsrPort(CmdPortNo); break;
			#ifdef WS_FILE_SERVER
			case WS_FIL: pmod->WSCloseFilePort(CmdPortNo); break;
			#endif
			case WS_UMC: pmod->WSCloseUmcPort(CmdPortNo); break;
			case WS_UMR: pmod->WSCloseUmrPort(CmdPortNo); break;
			case WS_CGD: pmod->WSCloseCgdPort(CmdPortNo); break;
			case WS_UGC: pmod->WSCloseUgcPort(CmdPortNo); break;
			case WS_UGR: pmod->WSCloseUgrPort(CmdPortNo); break;
			case WS_CTO: pmod->WSCloseCtoPort(CmdPortNo); break;
			case WS_CTI: pmod->WSCloseCtiPort(CmdPortNo); break;
			};
		}
        return;
	}
	else if((cmd=="backupapp")&&(pmod))
	{
		string path=ccodec->GetValue(tvmap,"Path");
		WSLogEvent("UMR%d: backupapp(%s) from (%s)",PortNo,parm,oboi.c_str());
		if(path.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Path' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		char bpath[MAX_PATH],emsg[256]={0};
		strcpy(bpath,path.c_str());
		int rc=pmod->WSBackupApp(bpath,emsg,true);
		if(rc<0)
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Reason=%s|",
				dtid.c_str(),oboi.c_str(),rc,emsg);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Text=%s|",
				dtid.c_str(),oboi.c_str(),rc,emsg);
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	else if((cmd=="restoreapp")&&(pmod))
	{
		string path=ccodec->GetValue(tvmap,"Path");
		WSLogEvent("UMR%d: restoreapp(%s) from (%s)",PortNo,parm,oboi.c_str());
		if(path.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Path' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		char bpath[MAX_PATH],emsg[256]={0};
		strcpy(bpath,path.c_str());
		int rc=pmod->WSRestoreApp(bpath,emsg);
		if(rc<0)
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Reason=%s|",
				dtid.c_str(),oboi.c_str(),rc,emsg);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Text=%s|",
				dtid.c_str(),oboi.c_str(),rc,emsg);
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	else if((cmd=="addevent")&&(pmod))
	{
		string text=ccodec->GetValue(tvmap,"Text");
		WSLogEvent("UMR%d: addevent(%s) from (%s)",PortNo,parm,oboi.c_str());
		if(text.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Text' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		pmod->WSLogEvent(text.c_str());
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|",dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	else if((cmd=="adderror")&&(pmod))
	{
		string text=ccodec->GetValue(tvmap,"Text");
		WSLogEvent("UMR%d: adderror(%s) from (%s)",PortNo,parm,oboi.c_str());
		if(text.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Text' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		pmod->WSLogError(text.c_str());
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|",dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	else if((cmd=="netalias")&&(pmod))
	{
		string clear=ccodec->GetValue(tvmap,"Clear");
		string alias=ccodec->GetValue(tvmap,"Alias");
		string text=ccodec->GetValue(tvmap,"Text");
		WSLogEvent("UMR%d: adderror(%s) from (%s)",PortNo,parm,oboi.c_str());
		if((clear=="Y")||(clear=="y"))
		{
			((WsocksHostImpl*)phost)->SetNetAlias("clear","all");
		}
		else
		{
			if(alias.empty())
			{
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Alias' parameter!|",
					dtid.c_str(),oboi.c_str());
				ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
				WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
				return;
			}
			((WsocksHostImpl*)phost)->SetNetAlias(alias.c_str(),text.c_str());
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|",dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	else if(cmd=="showwindow")
	{
		WSLogEvent("UMR%d: showwindow(%s) from (%s)",PortNo,parm,oboi.c_str());
		string restore=ccodec->GetValue(tvmap,"Restore");
		string hide=ccodec->GetValue(tvmap,"Hide");
		string maximize=ccodec->GetValue(tvmap,"Maximize");
		if((restore=="Y")||(restore=="y"))
			phost->WSHShowWindow(SW_RESTORE);
		else if((hide=="Y")||(hide=="y"))
			phost->WSHShowWindow(SW_MINIMIZE);
		else if((maximize=="Y")||(maximize=="y"))
			phost->WSHShowWindow(SW_MAXIMIZE);
		else
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'Restore', 'Hide', or 'Maximize' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
			return;
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|",dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	#ifdef WSOCKS_SSL
	else if(cmd=="reloadcerts")
	{
		WSLogEvent("UMR%d: reloadcerts(%s) from (%s)",PortNo,parm,oboi.c_str());
		((WsocksHostImpl*)phost)->ReloadCertificates(true);
		((WsocksHostImpl*)phost)->ReloadCertificates(false);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|",dtid.c_str(),oboi.c_str());
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		WSUmrSendMsg(1109,(int)(rptr -resp),resp,PortNo);
		return;
	}
	#endif
	// Common commands
	else
	{
		if(scmds.SMCNotifyRequest(reqid,cmd,parm,plen,(WSPortType)PortType,PortNo))
			return;
	}

	// Proxy the command to the app or smproxy
	if(!pmod)
		ConPortNo=pxyPort;
	char nparms[2048]={0};
	// DeliverTo stays the same since this is loopback (i.e. <dest> to <dest>)
	ccodec->SetValue(tvmap,"OnBehalfOf",noboi); // OnBehalfOf gets bigger (i.e. <src> to <src>/LBM.#)
	ccodec->SetTagValues(tvmap,nparms,sizeof(nparms));
	ccodec->Encode(rptr,sizeof(resp),true,reqid,cmd,nparms,(int)strlen(nparms));
	WSConSendMsg(1108,(int)(rptr -resp),resp,ConPortNo);
}
int LBMonitor::GetPortSpec(WsocksApp *pmod, const char *cptr, WSPortType& PortType, int& PortNo)
{
	PortType=WS_UNKNOWN; PortNo=-1;
	switch(*cptr)
	{
	case 'C': PortType=WS_CON; break;
	case 'U': PortType=WS_USC; break;
	case 'u': PortType=WS_USR; break;
	case 'F': PortType=WS_FIL; break;
	case 'M': PortType=WS_UMC; break;
	case 'm': PortType=WS_UMR; break;
	case 'D': PortType=WS_CGD; break;
	case 'G': PortType=WS_UGC; break;
	case 'g': PortType=WS_UGR; break;
	case 'I': PortType=WS_CTI; break;
	case 'O': PortType=WS_CTO; break;
	default: return -1;
	};
	cptr++;
	if(!isdigit(*cptr))
		return -1;
	PortNo=atoi(cptr);

	switch(PortType)
	{
	case WS_CON: 
		if((PortNo<0)||(PortNo>=pmod->NO_OF_CON_PORTS))
			return -1;
		break;
	case WS_USR:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_USR_PORTS))
			return -1;
		break;
	#ifdef WS_FILE_SERVER
	case WS_FIL:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_FILE_PORTS))
			return -1;
		break;
	#endif
	case WS_UMR:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_UMR_PORTS))
			return -1;
		break;
	case WS_CGD:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_CGD_PORTS))
			return -1;
		break;
	case WS_UGR:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_UGR_PORTS))
			return -1;
		break;
	case WS_CTO:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_CTO_PORTS))
			return -1;
		break;
	case WS_CTI:
		if((PortNo<0)||(PortNo>=pmod->NO_OF_CTI_PORTS))
			return -1;
		break;
	};
	return 0;
}
void LBMonitor::NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata)
{
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
	TVMAP tvmap;
	if(ccodec->GetTagValues(tvmap,resp,rlen)<0)
		return;
	if(PortType!=WS_CON)
	{
		// Proxy authorize response
		if((PortType==WS_UMR)&&(cmd=="login"))
		{
			for(int i=0;i<NO_OF_UMR_PORTS;i++)
			{
				if(!UmrPort[i].SockActive)
					continue;
				FeedUser *fuser=(FeedUser*)UmrPort[i].DetPtr3;
				string user=ccodec->GetValue(tvmap,"User");
				string appclass=ccodec->GetValue(tvmap,"AppClass");
				if((user==fuser->user)&&(fuser->waitAuth))
				{
					char resp[1024]={0},*rptr=resp;
                    char szParam[512];
					rlen=sizeof(resp);
                    string rc = ccodec->GetValue(tvmap, "rc");
                    if(rc!="0")
                    {
                        string reason = ccodec->GetValue(tvmap, "reason");
                        fuser->authed=false;
                        WSLogEvent("UMR%d: authorization failure (%s)", i, reason.c_str());
                        sprintf(szParam, "rc=%s|Reason=%s|", rc.c_str(), reason.c_str());
                        UmrPort[i].TimeTillClose = 5;
                        UmrPort[i].SockActive = false;
                    }
                    else
                    {
					    fuser->authed=true;
					    WSLogEvent("UMR%d: logged in as %s",i,user.c_str());
                        strcpy(szParam, "rc=0|Text=success|");
                    }
                    fuser->waitAuth=false;
                    ccodec->Encode(rptr,rlen,false,reqid,cmd,szParam,0);
					sprintf(UmrPort[i].Status,"%s",fuser->user.c_str());
					WSUmrSendMsg(1109,(int)(rptr -resp),resp,i);
					break;
				}
			}
		}
		return;
	}
    else
    {
        if(cmd=="login")
            return;
    }

	// Remove "/LBM.#/LBSysmon.LBSysmon" from DeliverTo
	string oboi=ccodec->GetValue(tvmap,"OnBehalfOf");
	string dtid=ccodec->GetValue(tvmap,"DeliverTo");
	const char *dptr=dtid.c_str();
	char *uptr=(char*)strrchr(dptr,'/');
    if(uptr)
    {
        int idx = (int)(uptr - dptr);
        dtid[idx] = '_';
        uptr = (char*)strrchr(dptr, '/');
        dtid[idx] = '/';
    }
	if((uptr)&&(!strncmp(uptr,"/LBM.",5)))
	{
		int UmrPortNo=atoi(uptr+5);
		if((UmrPortNo>=0)&&(UmrPortNo<NO_OF_UMR_PORTS)&&(UmrPort[UmrPortNo].SockActive))
		{
			FeedUser *fuser=(FeedUser*)UmrPort[UmrPortNo].DetPtr3;
			if(!fuser)
				goto bad_route;
			SysmonFlags *umflags=(SysmonFlags *)fuser->DetPtr;
			if(!umflags)
				goto bad_route;
			//// Drop responses from old connections
			//if(reqid<umflags->minRid)
			//	return;
			char ndtid[256]={0};
			strncpy(ndtid,dptr,uptr -dptr); ndtid[uptr -dptr]=0;
			ccodec->SetValue(tvmap,"DeliverTo",ndtid); // DeliverTo gets smaller (i.e. <src>/LBM.# to <src>)
			// OnBehalfOf stays the same since this is loopback (i.e. <dest> to <dest>)

			char mbuf[2048]={0},*mptr=mbuf,nresps[2048]={0},*pptr=nresps;
			ccodec->SetTagValues(tvmap,nresps,sizeof(nresps));
			ccodec->Encode(mptr,sizeof(mbuf),true,reqid,cmd,nresps,0);
			WSUmrSendMsg(1109,(int)(mptr -mbuf),mbuf,UmrPortNo);
			return;
		}
	}
bad_route:
	WSLogError("Response to (%s) from (%s) mis-routed!",dtid.c_str(),oboi.c_str());
}

int LBMonitor::SendLogHist(int UmrPortNo, WsocksApp *pmod, const char *fpath, const char *&fname, __int64& begin, __int64& end, __int64& tot)
{
	if(*fpath)
	{
		for(fname=fpath +strlen(fpath) -1;(fname>fpath);fname--)
		{
			if((*fname=='/')||(*fname=='\\'))
			{
				fname++;
				break;
			}
		}
	}
	else
		fname=fpath;

#ifdef WIN32
	HANDLE fhnd=CreateFile(fpath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
	if(fhnd==INVALID_HANDLE_VALUE)
		return -1;

	LARGE_INTEGER fsize;
	fsize.LowPart=GetFileSize(fhnd,(LPDWORD)&fsize.HighPart);
	if(!end)
		end=fsize.QuadPart;
	else if(end>fsize.QuadPart)
		end=fsize.QuadPart;
	tot=end;

	DWORD tsent=0;
	LARGE_INTEGER boff;
	for(boff.QuadPart=begin;boff.QuadPart<end;)
	{
		// Cap at 128KB at a time
		if(tsent>=128*1024)
		{
			end=boff.QuadPart;
			CloseHandle(fhnd);
			return +1;
		}
		char lbuf[4096]={0};
		__int64 eoff=boff.QuadPart +sizeof(lbuf) -256;
		if(eoff>end) eoff=end;
		SetFilePointer(fhnd,boff.LowPart,&boff.HighPart,FILE_BEGIN);
		DWORD llen=0;
		if(ReadFile(fhnd,lbuf,(DWORD)(eoff -boff.QuadPart),&llen,0)<0)
		{
			CloseHandle(fhnd);
			return -1;
		}
		char mbuf[4096]={0},*mptr=mbuf;
		int mlen=sizeof(mbuf);
		if(!pmod->fcodec)
			break;
		mlen=pmod->fcodec->EncodeLog(mptr,mlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(), 
			fname,true,boff.QuadPart,lbuf,llen);
		WSUmrSendMsg(1110,mlen,mbuf,UmrPortNo);
		boff.QuadPart+=llen;
		tsent+=llen;
	}
	CloseHandle(fhnd);
#else
	_ASSERT(false);//untested
	FILE *fhnd=fopen(fpath,"rb");
	if(fhnd==INVALID_FILE_VALUE)
		return -1;

	fseeko(fhnd,0,SEEK_END);
	LONGLONG fsize=ftello(fhnd);
	fseeko(fhnd,0,SEEK_SET);
	if(!end)
		end=fsize;
	else if(end>fsize)
		end=fsize;
	tot=end;


	DWORD tsent=0;
	for(LONGLONG boff=begin;boff<end;)
	{
		// Cap at 128KB at a time
		if(tsent>=128*1024)
		{
			end=boff;
			return +1;
		}
		char lbuf[4096]={0};
		__int64 eoff=boff +sizeof(lbuf) -256;
		if(eoff>end) eoff=end;
		fseeko(fhnd,boff,SEEK_SET);
		DWORD llen=(DWORD)fread(lbuf,1,(DWORD)(eoff -boff),fhnd);
		if(llen!=(DWORD)(eoff -boff))
			return -1;
		char mbuf[4096]={0},*mptr=mbuf;
		int mlen=sizeof(mbuf);
		mlen=pmod->fcodec->EncodeLog(mptr,mlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(), 
			fname,true,boff,lbuf,llen);
		WSUmrSendMsg(1110,mlen,mbuf,UmrPortNo);
		boff+=llen;
		tsent+=llen;
	}
	fclose(fhnd);
#endif
	return 0;
}
// New error for an app we're monitoring
int LBMonitor::LBMLoggedError(WsocksApp *pmod, __int64& off, const char *estr)
{
	char skey[256]={0};
	sprintf(skey,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
	_strupr(skey);
	TASKITEM *titem=taskmgr.FindUserItem(skey);
	if(!titem)
		return 0;
	char mbuf[2048+256]={0},*mptr=mbuf;
	int mlen=sizeof(mbuf);
	const char *fpath=pmod->WSErrorLogPath.c_str();
	const char *fname;
	for(fname=fpath +strlen(fpath) -1;(fname>fpath);fname--)
	{
		if((*fname=='/')||(*fname=='\\'))
		{
			fname++;
			break;
		}
	}
	if(!pmod->fcodec)
		return -1;
	mlen=pmod->fcodec->EncodeLog(mptr,mlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(), 
		fname,false,off,estr,(int)strlen(estr));
#ifdef FIX_TASK_CALL
	// Only call tasks from LBMon thread, not external threads
	EnterCriticalSection(&emutex);
	int elen=(int)(mptr -mbuf),slen=(int)strlen(skey) +1;
	char *ebuf=new char[12 +slen +elen],*eptr=ebuf;
	int ecode=SMTASK_ERRORLOG;
	memcpy(eptr,&ecode,4); eptr+=4;
	memcpy(eptr,&slen,4); eptr+=4;
	memcpy(eptr,skey,slen); eptr+=slen;
	memcpy(eptr,&elen,4); eptr+=4;
	memcpy(eptr,mbuf,elen);
	elist.push_back(ebuf);
	LeaveCriticalSection(&emutex);
#else
	SMFeedHint fhint={mbuf,(int)(mptr -mbuf)};
	taskmgr.CallTasks(skey,SMTASK_ERRORLOG,&fhint);
#endif
	return 0;
}
// New event for an app we're monitoring
int LBMonitor::LBMLoggedEvent(WsocksApp *pmod, __int64& off, const char *estr)
{
	char skey[256]={0};
	sprintf(skey,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
	_strupr(skey);
	TASKITEM *titem=taskmgr.FindUserItem(skey);
	if(!titem)
		return 0;
	char mbuf[2048+256]={0},*mptr=mbuf;
	int mlen=sizeof(mbuf);
	const char *fpath=pmod->WSEventLogPath.c_str();
	const char *fname;
	for(fname=fpath +strlen(fpath) -1;(fname>fpath);fname--)
	{
		if((*fname=='/')||(*fname=='\\'))
		{
			fname++;
			break;
		}
	}
	if(!pmod->fcodec)
		return -1;
	mlen=pmod->fcodec->EncodeLog(mptr,mlen,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(), 
		fname,false,off,estr,(int)strlen(estr));
#ifdef FIX_TASK_CALL
	// Only call tasks from LBMon thread, not external threads
	EnterCriticalSection(&emutex);
	int elen=(int)(mptr -mbuf),slen=(int)strlen(skey) +1;
	char *ebuf=new char[12 +slen +elen],*eptr=ebuf;
	int ecode=SMTASK_EVENTLOG;
	memcpy(eptr,&ecode,4); eptr+=4;
	memcpy(eptr,&slen,4); eptr+=4;
	memcpy(eptr,skey,slen); eptr+=slen;
	memcpy(eptr,&elen,4); eptr+=4;
	memcpy(eptr,mbuf,elen);
	elist.push_back(ebuf);
	LeaveCriticalSection(&emutex);
#else
	SMFeedHint fhint={mbuf,(int)(mptr -mbuf)};
	taskmgr.CallTasks(skey,SMTASK_EVENTLOG,&fhint);
#endif
	return 0;
}
int LBMonitor::SMCBeginUpload(const char *spath, const char *DeliverTo, const char *OnBehalfOf,
							  int rid, const char *fpath, WSPortType PortType, int PortNo, 
							  LONGLONG begin)
{
	// Find the target app
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		WsocksApp *pmod=(WsocksApp*)ConPort[i].DetPtr;
		if(!pmod)
			continue;
		char skey[256]={0};
		sprintf(skey,"%s.%s",pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str());
		if(!strcmp(OnBehalfOf,skey))
		{
			char ndtid[256]={0};
            sprintf(ndtid,"%s/LBM.%d/%s.%s",DeliverTo,PortNo,pcfg->aclass.c_str(),pcfg->aname.c_str());
			// We're pretending to be the app calling WSBeginUpload 
			// so UmrPortNo must be the app's UMR port that accepted the LBMon connection
			WSPort *pport=(WSPort*)ConPort[i].Sock;
			if((pport->lbaPeer)&&(pport->lbaPeer->PortType==WS_UMR))
			{
				int UmrPortNo=pport->lbaPeer->PortNo;
				return pmod->WSBeginUpload(spath,ndtid,OnBehalfOf,rid,fpath,PortType,UmrPortNo,begin);
			}
		}
	}
	return -1;
}
bool LBMonitor::SMCSendFinished(WSPortType PortType, int PortNo, int threshold)
{
	switch(PortType)
	{
	case WS_CON: 
		if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS)) return true;
		else if(!ConPort[PortNo].SockConnected) return true;
		else if(ConPort[PortNo].OutBuffer.Size +ConPort[PortNo].OutCompBuffer.Size>((DWORD)threshold*1024*1024)) return false;
		else return true;
	case WS_UMR: 
		if((PortNo<0)||(PortNo>=NO_OF_UMR_PORTS)) return true;
		else if(!UmrPort[PortNo].SockActive) return true;
		else if(UmrPort[PortNo].OutBuffer.Size +UmrPort[PortNo].OutCompBuffer.Size>((DWORD)threshold*1024*1024)) return false;
		else return true;
	};
	return true;
}
