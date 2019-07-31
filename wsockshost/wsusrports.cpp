
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#include "wstring.h"

#ifdef WIN32
#include <shlwapi.h>
#include <Iphlpapi.h>
#endif

int WsocksHostImpl::WSHSetupUscPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	return 0;
}
int WsocksHostImpl::WSHLoadIPAclFile(WsocksApp *pmod, const char *fpath,IPACL **alist)
{
    FILE *fp = fopen(fpath, "rt");
    if ( !fp )
    {
        WSHLogError(pmod,"Failed opening IPACL file (%s)",fpath);
        return FALSE;
    }
    IPACL *facl = *alist;
    char rbuf[1024];
    int lno = 0;
    while ( fgets(rbuf, sizeof(rbuf), fp) )
    {
        lno ++;
        if ( !strncmp(rbuf, "//", 2) )
            continue;
        char *inptr=0;
        for( int i=0; (inptr=strtoke(i?0:rbuf,",\r\n"))!=NULL; i++ )
        {
            switch ( i%2 )
            {
            case 0:
				// blank line
				if(!*inptr)
				{
					i=-1;
					continue;
				}
				else
                {
					IPACL *pacl=(IPACL*)calloc(1,sizeof(IPACL));
					pacl->NextIPAcl=facl; facl=pacl;
					strcpy(pacl->Ip,inptr);
					pacl->lIp=inet_addr(facl->Ip);
				#ifdef WS_IPACL_DENY
					pacl->action='D';
				#else
					pacl->action='P';
				#endif
                }
                break;
            case 1:
                strcpy(facl->Mask,inptr);
                facl->lMask=inet_addr(facl->Mask);
                break;
            }
        }
        if (facl&&
            (!facl->lIp)||(!facl->lMask))
        {
            WSHLogError(pmod,"Incomplete ACL in (%s) at line %d",fpath,lno);
            fclose(fp);
            return FALSE;
        }
    }
    fclose(fp);
    *alist=facl;
    return TRUE;
}
int WsocksHostImpl::WSHCfgUscPort(WsocksApp *pmod, int PortNo)
{
    char cfgcpy[80];
    strcpy(cfgcpy,pmod->UscPort[PortNo].IPAclCfg);
    char *inptr=cfgcpy;
    for( int i=0; (inptr=strtok(i?0:inptr,","))!=NULL; i++ )
    {
        switch ( i %2 )
        {
        case 0:
            if (PathFileExists(inptr))
            {
                if(!WSHLoadIPAclFile(pmod,inptr,&pmod->UscPort[PortNo].IPAclList))
					return FALSE;
                i++;
                break;
            }
            else
            {
                IPACL *pacl=(IPACL*)calloc(1,sizeof(IPACL));
                pacl->NextIPAcl=pmod->UscPort[PortNo].IPAclList;
                strcpy(pacl->Ip,inptr);
                pacl->lIp=inet_addr(pacl->Ip);
				if(pacl->lIp==INADDR_NONE)
				{
					WSHLogError(pmod,"ACL file (%s) not found.",cfgcpy);
					free(pacl);
					return FALSE;
				}
			#ifdef WS_IPACL_DENY
				pacl->action='D';
			#else
				pacl->action='P';
			#endif
				pmod->UscPort[PortNo].IPAclList = pacl;
            }                        
            break;
        case 1:
            strcpy(pmod->UscPort[PortNo].IPAclList->Mask,inptr);
            pmod->UscPort[PortNo].IPAclList->lMask=
                inet_addr(pmod->UscPort[PortNo].IPAclList->Mask);
            break;
        }
    }
    if (pmod->UscPort[PortNo].IPAclList&&
        ((!pmod->UscPort[PortNo].IPAclList->lIp)||(!pmod->UscPort[PortNo].IPAclList->lMask)))
    {
        WSHLogError(pmod,"Incomplete ACL for Usc Port %d",PortNo);
        return FALSE;
    }
    return(TRUE);
}
int WsocksHostImpl::WSHOpenUscPort(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN local_sin;  // Local socket - internet style 

#ifdef WS_OIO
	int SndBuf = 0;
#else
	int SndBuf = pmod->UscPort[PortNo].BlockSize*8192;
#endif
	
	struct
	{
		int l_onoff;
		int l_linger;
	} linger;
	
    if (!WSHCfgUscPort(pmod,PortNo))
	{
        pmod->UscPort[PortNo].InUse=FALSE;
		return(FALSE);
	}

	#ifdef WSOCKS_SSL
	// Most apps won't need SSL support, so don't iniitialize the gateway until we open the first USC or CON port
	if(pmod->UscPort[PortNo].Encrypted==2)
	{
		if((!sslg)&&(ReloadCertificates(true)<0))
		{
			pmod->UscPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d SSLInit() failed",PortNo);
			return(FALSE);
		}
	}
	#endif

	//pmod->UscPort[PortNo].Sock = socket(AF_INET, SOCK_STREAM, 0);
	pmod->UscPort[PortNo].Sock = WSHSocket(pmod,WS_USC,PortNo);
	if (pmod->UscPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d socket() failed",PortNo);
		return(FALSE);
	}
#ifdef _DEBUG_THREADS
	WSHLogEvent(pmod,"DEBUG Create USC%d / %d",PortNo,((WSPort*)pmod->UscPort[PortNo].Sock)->sd);
#endif
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->UscPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d ioctlsocket(FIONBIO) failed",PortNo);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->UscPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d setsockopt(SO_REUSEADDR) failed",PortNo);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
	}

	linger.l_onoff=1;
	linger.l_linger=0;

	if (WSHSetSockOpt(pmod->UscPort[PortNo].Sock, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof(linger)) == SOCKET_ERROR) 
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d setsockopt(SO_LINGER) failed",PortNo);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->UscPort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d setsockopt(SO_SNDBUF,%d) failed",PortNo,SndBuf);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->UscPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d setsockopt(SO_RCVBUF,%d) failed",PortNo,SndBuf);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
	}

	local_sin.sin_family=AF_INET;
	local_sin.sin_port=htons(pmod->UscPort[PortNo].Port);

	if (strcmp(pmod->UscPort[PortNo].LocalIP,"AUTO")==0)
		local_sin.sin_addr.s_addr=INADDR_ANY;
	else
		local_sin.sin_addr.s_addr=inet_addr(pmod->UscPort[PortNo].LocalIP);

   //  Associate an address with a socket. (bind)
         
   //if (bind( pmod->UscPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
	if (WSHBindPort( pmod->UscPort[PortNo].Sock, &local_sin.sin_addr, pmod->UscPort[PortNo].Port) == SOCKET_ERROR) 
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d bind(%s:%d) failed",PortNo,pmod->UscPort[PortNo].LocalIP,pmod->UscPort[PortNo].Port);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
   }

	// Retrieve bind port
	SOCKADDR_IN laddr;
	int lalen=sizeof(SOCKADDR_IN);
	WSHGetSockName(pmod->UscPort[PortNo].Sock,(SOCKADDR*)&laddr,&lalen);
	pmod->UscPort[PortNo].bindPort=ntohs(laddr.sin_port);
	if(local_sin.sin_addr.s_addr==INADDR_ANY)
	{
	#ifdef WIN32
		// Enumerate NICs (Requires Iphlpapi.lib)
		ULONG blen=0;
		GetAdaptersInfo(0,&blen);
		PIP_ADAPTER_INFO pAdapterInfo=(PIP_ADAPTER_INFO)new char[blen];
		memset(pAdapterInfo,0,blen);
		GetAdaptersInfo(pAdapterInfo,&blen);
		for(PIP_ADAPTER_INFO pnic=pAdapterInfo;pnic;pnic=pnic->Next)
		{
			strncpy(pmod->UscPort[PortNo].bindIP,pnic->IpAddressList.IpAddress.String,20);
			pmod->UscPort[PortNo].bindIP[19]=0;
			if(pmod->UscPort[PortNo].bindIP[0])
				break;
		}
		delete pAdapterInfo;
	#else
		strncpy(pmod->UscPort[PortNo].bindIP,inet_ntoa(local_sin.sin_addr),20);
	#endif
	}
	else
		strncpy(pmod->UscPort[PortNo].bindIP,inet_ntoa(local_sin.sin_addr),20);

	if (WSHListen( pmod->UscPort[PortNo].Sock, MAX_PENDING_CONNECTS ) == SOCKET_ERROR) 
	{
		pmod->UscPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d listen(%d) failed",PortNo,MAX_PENDING_CONNECTS);
		WSHCloseUscPort(pmod,PortNo);
		return(FALSE);
	}

	pmod->UscPort[PortNo].SockActive=TRUE;
	WSHUpdatePort(pmod,WS_USC,PortNo);
#ifdef _DEBUG_THREADS
	WSHLogEvent(pmod,"DEBUG Active USC%d / %d",PortNo,((WSPort*)pmod->UscPort[PortNo].Sock)->sd);
#endif
	return(TRUE);
}
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseUscPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UscPort[PortNo].InUse)
		return -1;
	if(pmod->UscPort[PortNo].SockActive)
		pmod->UscPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUscPort(WsocksApp *pmod, int PortNo)
{
    while ( pmod->UscPort[PortNo].IPAclList )
    {
        IPACL *pacl=pmod->UscPort[PortNo].IPAclList;
        pmod->UscPort[PortNo].IPAclList=pmod->UscPort[PortNo].IPAclList->NextIPAcl;
        free(pacl);
    }
	WSHClosePort(pmod->UscPort[PortNo].Sock);
	// Don't zero out USC detail pointer values
	USCPORT tuport;
	memcpy(&tuport.DetPtr,&pmod->UscPort[PortNo].DetPtr,11*sizeof(void*));
	memset(&pmod->UscPort[PortNo].SockActive,0
		,sizeof(USCPORT)-(int)((char *)&pmod->UscPort[PortNo].SockActive-(char *)&pmod->UscPort[PortNo]));
	memcpy(&pmod->UscPort[PortNo].DetPtr,&tuport.DetPtr,11*sizeof(void*));
	return (TRUE);
}

int WsocksHostImpl::WSHSetupUsrPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=0;i<=pmod->NO_OF_USC_PORTS;i++)
		{
			memset(&pmod->UscPort[i],0,sizeof(USCPORT));
			pmod->UscPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
			pmod->UscPort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
			pmod->UscPort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		}
		for (i=0;i<=pmod->NO_OF_USR_PORTS;i++)
		{
			memset(&pmod->UsrPort[i],0,sizeof(USRPORT));
		}
		break;
	case WS_OPEN: // Open
		for (i=0;i<pmod->NO_OF_USC_PORTS;i++)
		{
			if(strlen(pmod->UscPort[i].LocalIP)>0)
			{
				//AddConListItem(GetDispItem(WS_USC,i));
				pmod->UscPort[i].tmutex=CreateMutex(0,false,0);
				pmod->UscPort[i].activeThread=0;
				pmod->UscPort[i].InUse=TRUE;
			}
		}
		//AddConListItem(GetDispItem(WS_USR_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=0;i<=pmod->NO_OF_USC_PORTS;i++)
		{
			if(pmod->UscPort[i].SockActive)
				WSHCloseUscPort(pmod,i);
			if(pmod->UscPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_USC,i));
				pmod->UscPort[i].InUse=FALSE;
			}
			pmod->UscPort[i].activeThread=0;
			if(pmod->UscPort[i].tmutex)
			{
				//DeleteMutex(pmod->UscPort[i].tmutex); pmod->UscPort[i].tmutex=0;
				DeletePortMutex(pmod,WS_USC,i,false);
			}
		}
		for (i=0;i<=pmod->NO_OF_USR_PORTS;i++)
		{
			if(pmod->UsrPort[i].Sock)// Account for TimeTillClose
			{
				_WSHCloseUsrPort(pmod,i);
				#ifdef WS_OTPP
				_WSHWaitUsrThreadExit(pmod,i);
				#endif
			}
		}
		#ifdef WS_OTMP
		WSHWaitUsrThreadPoolsExit();
		#endif
		break;
	}
	return(TRUE);
}
int WsocksHostImpl::WSHUsrSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	MSGHEADER MsgOutHeader;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_USR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->UscPort[pmod->UsrPort[PortNo].UscPort].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	MsgOutHeader.MsgID=MsgID;
	MsgOutHeader.MsgLen=MsgLen;
	LockPort(pmod,WS_USR,PortNo,true);
	int lastSend=pmod->UsrPort[PortNo].sendThread;
	pmod->UsrPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->UsrPort[PortNo].SockActive)
		#if defined(WS_OIO)||defined(WS_OTPP)
		&&(!pmod->UsrPort[PortNo].peerClosed)
		#endif
		)
	{
		if((pmod->UsrPort[PortNo].OutBuffer.Busy)&&(pmod->UsrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: USR%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->UsrPort[PortNo].OutBuffer.Busy);
			pmod->UsrPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->UsrPort[PortNo].OutBuffer.Busy;
		pmod->UsrPort[PortNo].OutBuffer.Busy=pmod->UsrPort[PortNo].sendThread;
#ifdef WS_LOOPBACK
		BUFFER *Buffer=&pmod->UsrPort[PortNo].OutBuffer;
		int ConPortNo=-1;
		// Short circuit loopback connections
		WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
		if((pport->lbaPeer)&&(pport->lbaPeer->lbaBuffer))
		{
			_ASSERT(pport->lbaPeer->PortType==WS_CON);
			ConPortNo=pport->lbaPeer->PortNo;
			Buffer=pport->lbaPeer->lbaBuffer;
		}
		//Send Header
		if(!WSWriteBuff(Buffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UsrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_USR,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(Buffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UsrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_USR,PortNo,true);
				return(FALSE);
			}
		}
		pmod->UsrPort[PortNo].PacketsOut++;
		if(ConPortNo>=0)
		{
			pmod->UsrPort[PortNo].BytesOut+=sizeof(MSGHEADER) +MsgOutHeader.MsgLen;
			WSHRecord(pmod,&pmod->UsrPort[PortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),true);
			WSHRecord(pmod,&pmod->UsrPort[PortNo].Recording,MsgOut,MsgOutHeader.MsgLen,true);
			// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
			// but that was problematic during close of the ports. 
			// Instead, try unlocking the USR port before doing any CON port message handling.
			pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UsrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_USR,PortNo,true);
			LockPort(pport->lbaPeer->pmod,WS_CON,ConPortNo,false);
		#if defined(WS_OIO)||defined(WS_OTPP)
			if(WSHConRead(pport->lbaPeer->pmod,ConPortNo,(char*)&MsgOutHeader,sizeof(MSGHEADER))&&
			   WSHConRead(pport->lbaPeer->pmod,ConPortNo,MsgOut,MsgOutHeader.MsgLen))
		#else
			WSHRecord(pport->lbaPeer->pmod,&pport->lbaPeer->pmod->ConPort[ConPortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),false);
			WSHRecord(pport->lbaPeer->pmod,&pport->lbaPeer->pmod->ConPort[ConPortNo].Recording,MsgOut,MsgOutHeader.MsgLen,false);
			if(WSHConRead(pport->lbaPeer->pmod,ConPortNo))
		#endif
			{
				while((pport->lbaPeer)&&(pport->lbaPeer->pmod->WSConMsgReady(ConPortNo)))
					pport->lbaPeer->pmod->WSConStripMsg(ConPortNo);
			}
			UnlockPort(pport->lbaPeer->pmod,WS_CON,ConPortNo,false);
			return(TRUE);
		}
	#ifdef WS_REALTIMESEND
		else
			WSHUsrSend(pmod,PortNo,false);
	#endif
#else
		//Send Header
		if(!WSWriteBuff(&pmod->UsrPort[PortNo].OutBuffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UsrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_USR,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(&pmod->UsrPort[PortNo].OutBuffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UsrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_USR,PortNo,true);
				return(FALSE);
			}
		}
		pmod->UsrPort[PortNo].PacketsOut++;
	#ifdef WS_REALTIMESEND
		WSHUsrSend(pmod,PortNo);
	#endif
#endif
		pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
		pmod->UsrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_USR,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->UsrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_USR,PortNo,true);
		return(FALSE);
	}
}
int WsocksHostImpl::WSHUsrSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_USR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->UscPort[pmod->UsrPort[PortNo].UscPort].BlockSize*1024))
	{
		_ASSERT(false);
		return(FALSE);
	}
	LockPort(pmod,WS_USR,PortNo,true);
	int lastSend=pmod->UsrPort[PortNo].sendThread;
	pmod->UsrPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->UsrPort[PortNo].SockActive)
		#if defined(WS_OIO)||defined(WS_OTPP)
		&&(!pmod->UsrPort[PortNo].peerClosed)
		#endif
		)
	{
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if((pmod->UsrPort[PortNo].OutBuffer.Busy)&&(pmod->UsrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
			{
				_ASSERT(false);
				WSHLogError(pmod,"!CRASH: USR%d OutBuffer.Busy detected a possible thread %d crash.",
					PortNo,pmod->UsrPort[PortNo].OutBuffer.Busy);
				pmod->UsrPort[PortNo].OutBuffer.Busy=0;
			}
			int lastBusy=pmod->UsrPort[PortNo].OutBuffer.Busy;
			pmod->UsrPort[PortNo].OutBuffer.Busy=pmod->UsrPort[PortNo].sendThread;
#ifdef WS_LOOPBACK
			BUFFER *Buffer=&pmod->UsrPort[PortNo].OutBuffer;
			int ConPortNo=-1;
			// Short circuit loopback connections
			WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
			if((pport->lbaPeer)&&(pport->lbaPeer->lbaBuffer))
			{
				_ASSERT(pport->lbaPeer->PortType==WS_CON);
				ConPortNo=pport->lbaPeer->PortNo;
				Buffer=pport->lbaPeer->lbaBuffer;
			}
			if(WSWriteBuff(Buffer,MsgOut,MsgLen)==0)
			{
				pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UsrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_USR,PortNo,true);
				return(FALSE);
			}
			pmod->UsrPort[PortNo].PacketsOut++;
			if(ConPortNo>=0)
			{
				pmod->UsrPort[PortNo].BytesOut+=MsgLen;
				WSHRecord(pmod,&pmod->UsrPort[PortNo].Recording,MsgOut,MsgLen,true);
				// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
				// but that was problematic during close of the ports. 
				// Instead, try unlocking the USR port before doing any CON port message handling.
				pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UsrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_USR,PortNo,true);
				LockPort(pport->lbaPeer->pmod,WS_CON,ConPortNo,false);
			#if defined(WS_OIO)||defined(WS_OTPP)
				if(WSHConRead(pport->lbaPeer->pmod,ConPortNo,MsgOut,MsgLen))
			#else
				WSHRecord(pport->lbaPeer->pmod,&pport->lbaPeer->pmod->ConPort[ConPortNo].Recording,MsgOut,MsgLen,false);
				if(WSHConRead(pport->lbaPeer->pmod,ConPortNo))
			#endif
				{
					while((pport->lbaPeer)&&(pport->lbaPeer->pmod->WSConMsgReady(ConPortNo)))
						pport->lbaPeer->pmod->WSConStripMsg(ConPortNo);
				}
				UnlockPort(pport->lbaPeer->pmod,WS_CON,ConPortNo,false);
				return(TRUE);
			}
	#ifdef WS_REALTIMESEND
			else if(Packets)
				WSHUsrSend(pmod,PortNo,false);
	#endif
#else
			if(WSWriteBuff(&pmod->UsrPort[PortNo].OutBuffer,MsgOut,MsgLen)==0)
			{
				pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UsrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_USR,PortNo,true);
				return(FALSE);
			}
			pmod->UsrPort[PortNo].PacketsOut+=Packets;
	#ifdef WS_REALTIMESEND
			if(Packets)
				WSHUsrSend(pmod,PortNo);
	#endif
#endif
			pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
		}
		pmod->UsrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_USR,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->UsrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_USR,PortNo,true);
		return(FALSE);
	}
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseUsrPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UsrPort[PortNo].Sock)
		return -1;
	if((pmod->UsrPort[PortNo].SockActive)||(pmod->UsrPort[PortNo].TimeTillClose))
		pmod->UsrPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUsrPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->UsrPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_USR,PortNo,false);
		lastRecv=pmod->UsrPort[PortNo].recvThread;
		pmod->UsrPort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->UsrPort[PortNo].pendingClose)
		{
			pmod->UsrPort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_USR,PortNo,false);
			return 0;
		}
		pmod->UsrPort[PortNo].pendingClose=true;
	}
	if(pmod->UsrPort[PortNo].smutex)
	{
		LockPort(pmod,WS_USR,PortNo,true);
		lastSend=pmod->UsrPort[PortNo].sendThread;
		pmod->UsrPort[PortNo].sendThread=GetCurrentThreadId();
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->UsrPort[PortNo].pendOvlRecvList; pmod->UsrPort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->UsrPort[PortNo].pendOvlSendList; pmod->UsrPort[PortNo].pendOvlSendList=0;
	if((pendOvlRecvList)||(pendOvlSendList))
	{		
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
		if(pendOvlRecvList)
		{
			WSOVERLAPPED *lastRecv=0;
			for(WSOVERLAPPED *povl=pendOvlRecvList;povl;povl=povl->next)
			{
				povl->Cancelled=true;
				lastRecv=povl;
			}
			lastRecv->next=pmod->cxlOvlList;
			if(pmod->cxlOvlList) pmod->cxlOvlList->prev=lastRecv;
			pmod->cxlOvlList=pendOvlRecvList;
		}
		if(pendOvlSendList)
		{
			WSOVERLAPPED *lastSend=0;
			for(WSOVERLAPPED *povl=pendOvlSendList;povl;povl=povl->next)
			{
				povl->Cancelled=true;
				lastSend=povl;
			}
			lastSend->next=pmod->cxlOvlList;
			if(pmod->cxlOvlList) pmod->cxlOvlList->prev=lastSend;
			pmod->cxlOvlList=pendOvlSendList;
		}
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif
	}

	//ResetSendTimeout(WS_USR,PortNo);
	pmod->UsrPort[PortNo].SockActive=0;
	pmod->WSUsrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_USR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_USR,PortNo));
	WSHClosePort(pmod->UsrPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->UsrPort[PortNo].Recording,WS_USR,PortNo);
	WSCloseBuff(&pmod->UsrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UsrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UsrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UsrPort[PortNo].OutCompBuffer);
	#endif
	HANDLE srm=0,ssm=0;
	DWORD srcnt=0,sscnt=0;
	if(pmod->UsrPort[PortNo].smutex)
	{
		ssm=pmod->UsrPort[PortNo].smutex;
		pmod->UsrPort[PortNo].sendThread=lastSend;
		//DeleteMutex(pmod->UsrPort[PortNo].smutex); pmod->UsrPort[PortNo].smutex=0;
		DeletePortMutex(pmod,WS_USR,PortNo,true);
		sscnt=pmod->UsrPort[PortNo].smutcnt;
	}
	if(pmod->UsrPort[PortNo].rmutex)
	{
		srm=pmod->UsrPort[PortNo].rmutex;
		pmod->UsrPort[PortNo].recvThread=lastRecv;
		//DeleteMutex(pmod->UsrPort[PortNo].rmutex); pmod->UsrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_USR,PortNo,false);
		srcnt=pmod->UsrPort[PortNo].rmutcnt;
	}
	memset(&pmod->UsrPort[PortNo],0,sizeof(USRPORT));
	if(sscnt)
	{
		pmod->UsrPort[PortNo].smutex=ssm;
		pmod->UsrPort[PortNo].smutcnt=sscnt;
	}
	if(srcnt)
	{
		pmod->UsrPort[PortNo].rmutex=srm;
		pmod->UsrPort[PortNo].rmutcnt=srcnt;
	}
	WSHUpdatePort(pmod,WS_USR,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseUsrPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UsrPort[PortNo].Sock)
		return -1;
	if((pmod->UsrPort[PortNo].SockActive)||(pmod->UsrPort[PortNo].TimeTillClose))
		pmod->UsrPort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseUsrPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->UsrPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_USR,PortNo,false);
		lastRecv=pmod->UsrPort[PortNo].recvThread;
		pmod->UsrPort[PortNo].recvThread=GetCurrentThreadId();
	}
	if(pmod->UsrPort[PortNo].smutex)
	{
		LockPort(pmod,WS_USR,PortNo,true);
		lastSend=pmod->UsrPort[PortNo].sendThread;
		pmod->UsrPort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_USR,PortNo);
	pmod->UsrPort[PortNo].SockActive=0;
	pmod->WSUsrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_USR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_USR,PortNo));
	if(pmod->UsrPort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->UsrPort[PortNo].Sock)->sd;
		#ifdef WSOCKS_SSL
		if(pmod->UscPort[pmod->UsrPort[PortNo].UscPort].Encrypted == 2)
		{
			SSLInfo *pSSLInfo=((WSPort*)pmod->UsrPort[PortNo].Sock)->pSSLInfo;
			WSHClosePort(pmod->UsrPort[PortNo].Sock); pmod->UsrPort[PortNo].Sock=0;
			if(pSSLInfo)
				SSLGateway::instance()->SSLRemove(pSSLInfo);
		}
		else
		{
			WSHClosePort(pmod->UsrPort[PortNo].Sock); pmod->UsrPort[PortNo].Sock=0;
		}
		#else
		WSHClosePort(pmod->UsrPort[PortNo].Sock); pmod->UsrPort[PortNo].Sock=0;
		#endif
		WSHFinishOverlapSend(pmod,sd,&pmod->UsrPort[PortNo].pendOvlSendBeg,&pmod->UsrPort[PortNo].pendOvlSendEnd);
	}

	#ifdef WIN32
	HANDLE pthread=pmod->UsrPort[PortNo].pthread;
	#else
	pthread_t pthread=pmod->UsrPort[PortNo].pthread;
	#endif
	WSHCloseRecording(pmod,&pmod->UsrPort[PortNo].Recording,WS_USR,PortNo);
	WSCloseBuff(&pmod->UsrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UsrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UsrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UsrPort[PortNo].OutCompBuffer);
	#endif
	HANDLE srm=0,ssm=0;
	DWORD srcnt=0,sscnt=0;
	if(pmod->UsrPort[PortNo].smutex)
	{
		ssm=pmod->UsrPort[PortNo].smutex;
		pmod->UsrPort[PortNo].sendThread=lastSend;
		//DeleteMutex(pmod->UsrPort[PortNo].smutex); pmod->UsrPort[PortNo].smutex=0;
		DeletePortMutex(pmod,WS_USR,PortNo,true);
		sscnt=pmod->UsrPort[PortNo].smutcnt;
	}
	if(pmod->UsrPort[PortNo].rmutex)
	{
		srm=pmod->UsrPort[PortNo].rmutex;
		pmod->UsrPort[PortNo].recvThread=lastRecv;
		//DeleteMutex(pmod->UsrPort[PortNo].rmutex); pmod->UsrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_USR,PortNo,false);
		srcnt=pmod->UsrPort[PortNo].rmutcnt;
	}
	memset(&pmod->UsrPort[PortNo],0,sizeof(USRPORT));
	pmod->UsrPort[PortNo].pthread=pthread;
	if(sscnt)
	{
		pmod->UsrPort[PortNo].smutex=ssm;
		pmod->UsrPort[PortNo].smutcnt=sscnt;
	}
	if(srcnt)
	{
		pmod->UsrPort[PortNo].rmutex=srm;
		pmod->UsrPort[PortNo].rmutcnt=srcnt;
	}
	WSHUpdatePort(pmod,WS_USR,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitUsrThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->UsrPort[PortNo].pthread)
	{
	#ifdef WIN32
		#ifdef WS_OTMP
		if(pmod->pcfg->usrOtmp)
		{
			pmod->UsrPort[PortNo].pthread=0;
			return;
		}
		#endif
		#ifdef _DEBUG
		WaitForSingleObject(pmod->UsrPort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->UsrPort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->UsrPort[PortNo].pthread); pmod->UsrPort[PortNo].pthread=0;
	#else
		void *rc=0;
		pthread_join(pmod->UsrPort[PortNo].pthread,&rc);
	#endif
	}
}
#ifdef WIN32
struct UsrReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
DWORD WINAPI _BootUsrReadThread(LPVOID arg)
{
	UsrReadThreadData *ptd=(UsrReadThreadData*)arg;
	WsocksHostImpl *aimpl=ptd->aimpl;
	WsocksApp *pmod=ptd->pport->pmod;
	int PortNo=ptd->pport->PortNo;
	delete ptd;
	int rc=aimpl->WSHUsrReadThread(pmod,PortNo);
	return rc;
}
#ifdef WS_OTMP
struct UsrReadPoolThreadData
{
	WsocksHostImpl *aimpl;
	WsocksApp *pmod;
	THREADPOOL *tpool;
};
DWORD WINAPI _BootUsrReadPoolThread(LPVOID arg)
{
	UsrReadPoolThreadData *ptd=(UsrReadPoolThreadData*)arg;
	WsocksHostImpl *aimpl=ptd->aimpl;
	WsocksApp *pmod=ptd->pmod;
	THREADPOOL *tpool=ptd->tpool;
	delete ptd;
	int rc=aimpl->WSHUsrReadPoolThread(pmod,tpool);
	return rc;
}
HANDLE WsocksHostImpl::WSHPoolUsrReadThread(WsocksApp *pmod, WSPort *pport)
{
	THREADPOOL *tpool=usrPools;
	if((!tpool)||(tpool->nports>=WS_NTMP))
	{
		tpool=new THREADPOOL;
		memset(tpool,0,sizeof(THREADPOOL));
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
		tpool->next=usrPools; usrPools=tpool;
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif

		DWORD tid=0;
		UsrReadPoolThreadData *ptd=new UsrReadPoolThreadData;
		ptd->aimpl=this;
		ptd->pmod=pmod;
		ptd->tpool=tpool;
		tpool->thnd=CreateThread(0,0,_BootUsrReadPoolThread,ptd,0,&tid);
		if(!tpool->thnd)
		{
			delete ptd;
			return 0;
		}
	}
	tpool->ports[tpool->nports++]=pport;
	return tpool->thnd;
}
void WsocksHostImpl::WSHWaitUsrThreadPoolsExit()
{
	while(usrPools)
	{
		THREADPOOL *tpool=usrPools;
		tpool->shutdown=true;
		WaitForSingleObject(tpool->thnd,INFINITE);
		CloseHandle(tpool->thnd); tpool->thnd=0;
		usrPools=usrPools->next;
		delete tpool;
	}
}
#endif//WS_OTMP
#else//!WIN32
struct UsrReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
void *_BootUsrReadThread(void *arg)
{
	UsrReadThreadData *ptd=(UsrReadThreadData*)arg;
	WsocksHostImpl *aimpl=ptd->aimpl;
	WsocksApp *pmod=ptd->pport->pmod;
	int PortNo=ptd->pport->PortNo;
	delete ptd;
	int rc=aimpl->WSHUsrReadThread(pmod,PortNo);
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
}
#endif
#else//!WS_OIO
int WsocksHostImpl::WSHCloseUsrPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->UsrPort[PortNo].rmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_USR,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->UsrPort[PortNo].recvThread)&&(pmod->UsrPort[PortNo].recvThread!=tid))
		{
			UnlockPort(pmod,WS_USR,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_USR,PortNo,false);
		}
		pmod->UsrPort[PortNo].recvThread=tid;
	}
	//ResetSendTimeout(WS_USR,PortNo);
	pmod->UsrPort[PortNo].SockActive=0;
	pmod->WSUsrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_USR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_USR,PortNo));
	WSHClosePort(pmod->UsrPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->UsrPort[PortNo].Recording,WS_USR,PortNo);
	WSCloseBuff(&pmod->UsrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UsrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UsrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UsrPort[PortNo].OutCompBuffer);
	#endif
	pmod->UsrPort[PortNo].recvThread=0;
	if(pmod->UsrPort[PortNo].rmutex)
	{
		//DeleteMutex(pmod->UsrPort[PortNo].rmutex); pmod->UsrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_USR,PortNo,false);
	}
	memset(&pmod->UsrPort[PortNo],0,sizeof(USRPORT));
	//WSHPortChanged(pmod,pmod->UsrPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO
int WsocksHostImpl::WSHUsrAuthorize(WsocksApp *pmod, int UscPortNo, SOCKADDR_IN *raddr)
{
    if (!pmod->UscPort[UscPortNo].IPAclList)
		// Everyone permitted if no ACL set up
        return(TRUE);
    DWORD remoteIp=raddr->sin_addr.s_addr;
    for ( IPACL *IPAcl=pmod->UscPort[UscPortNo].IPAclList; IPAcl; IPAcl=IPAcl->NextIPAcl )
    {
        if ( IPAcl->lIp && (remoteIp &IPAcl->lMask)!=(IPAcl->lIp &IPAcl->lMask) )
            continue;
		if( IPAcl->action=='D' )
			return FALSE;
		else if( IPAcl->action=='P' )
			return TRUE;
    }
#ifdef WS_IPACL_DENY
	// Permitted unless explicitly denied
    return(TRUE);
#else
	// Denied unless explicitly permitted
    return(FALSE);
#endif
}
int WsocksHostImpl::WSHUsrAccept(WsocksApp *pmod, int PortNo)
{
	SOCKET_T TSock;			// temp socket to hold connection while we
							// determine what port it was for 
	SOCKADDR_IN AccSin;    // Accept socket address - internet style 
	int AccSinLen;        // Accept socket address length 
	int i;
	int j=PortNo;

	ws_fd_set rds;
	WS_FD_ZERO(&rds);
	WS_FD_SET(pmod->UscPort[j].Sock,&rds);
	struct timeval TimeVal={0,0};
	int nrds=WSHSelect((PTRCAST)pmod->UscPort[j].Sock+1,&rds,NULL,NULL,(timeval*)&TimeVal);
	if(nrds<1)
		return FALSE;
#ifdef _DEBUG_THREADS
	int lfd=((WSPort*)pmod->UscPort[j].Sock)->sd;
	WSHLogEvent(pmod,"DEBUG select ready %d",lfd);
#endif
	// determine which user channel is available
	for (i=0;i<pmod->NO_OF_USR_PORTS;i++)
	{
		if((!pmod->UsrPort[i].SockActive)&&(pmod->UsrPort[i].TimeTillClose<=0)
			#ifdef WS_OIO
			&&(!pmod->UsrPort[i].pendingClose)
			#endif
			&&(!pmod->UsrPort[i].rmutcnt)&&(!pmod->UsrPort[i].smutcnt)
			)
			break;
	}
	if (i>=pmod->NO_OF_USR_PORTS) // no more ports avaliable
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : No more USR ports (max %d) avaliable",pmod->NO_OF_USR_PORTS);
		// Accept the incoming connection with port -1 to drop it
		AccSinLen = sizeof( AccSin );
		TSock = WSHAccept(pmod,pmod->UscPort[j].Sock,WS_USR,-1,(struct sockaddr *) &AccSin,
			(int *) &AccSinLen );
		return(FALSE);
	}
	pmod->UsrPort[i].UscPort=j;

	AccSinLen = sizeof( AccSin );
	//TSock = accept(pmod->UscPort[j].Sock,(struct sockaddr *) &AccSin,
	TSock = WSHAccept(pmod,pmod->UscPort[j].Sock,WS_USR,i,(struct sockaddr *) &AccSin,
		(int *) &AccSinLen );

	if (TSock == INVALID_SOCKET_T)
	{
		if(i!=-1)
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d accept() failed",j);
		return(FALSE);
	}
#ifdef _DEBUG_THREADS
	WSHLogEvent(pmod,"DEBUG Create USR%d / %d",PortNo,((WSPort*)TSock)->sd);
#endif

#if !defined(WIN32)&&!defined(_CONSOLE)
	// On Windows, the accepted socket inherits async behavior from listen socket,
	// but on Linux, it doesn't inherit fcntl attributes
	if (WSHIoctlSocket(TSock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d WSUsrAccept  USR%d ioctlsocket(!FIONBIO) failed",j,i);
		WSHClosePort(TSock);
		return(FALSE);
	}
#endif

#ifdef WS_REALTIMESEND
#ifdef WIN32
	// Disable Nagle
	unsigned int nagleoff=1;
	if (WSHSetSockOpt(TSock, IPPROTO_TCP, TCP_NODELAY, (char *)(&nagleoff), sizeof(nagleoff)) == SOCKET_ERROR) 
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d WSUsrAccept USR%d setsockopt(TCP_NODELAY) failed",j,i);
        WSHClosePort( TSock );
		return(FALSE);
	}
#else
	// Not really needed
#endif
	pmod->UsrPort[i].ImmediateSendLimit=pmod->UscPort[pmod->UsrPort[i].UscPort].ImmediateSendLimit;
#endif

	if(!WSHUsrAuthorize(pmod,PortNo,&AccSin))
	{
		static DWORD lastip=0;
		static DWORD tlast=0;
		DWORD tnow=GetTickCount();
		if((AccSin.sin_addr.s_addr!=lastip)||(tnow -tlast>=3600000))
		{
			WSHLogError(pmod,"Unauthorized connection USC%d dropped from USR%d %s:%d",PortNo,i,inet_ntoa(AccSin.sin_addr),ntohs(AccSin.sin_port));
			lastip=AccSin.sin_addr.s_addr;
			tlast=tnow;
		}
		WSHClosePort(TSock);
		return(FALSE);
	}

	//// determine which user channel is available
	//for (i=0;i<pmod->NO_OF_USR_PORTS;i++)
	//{
	//	if((!pmod->UsrPort[i].SockActive)&&(pmod->UsrPort[i].TimeTillClose<=0))
	//		break;
	//}
	//if (i>=pmod->NO_OF_USR_PORTS) // no more ports avaliable
	//{
	//	WSHLogError(pmod,"!WSOCKS: FATAL ERROR : No more user ports avaliable");
 //       WSHClosePort( TSock );
	//	return(FALSE);
	//}

    // DONT MOVE THIS LINE OR THE BUFFER SIZE IN THE NEXT LINE WILL FAIL
	//pmod->UsrPort[i].UscPort=j;

	WSOpenBuffLimit(&pmod->UsrPort[i].InBuffer,pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize,pmod->UscPort[pmod->UsrPort[i].UscPort].RecvBuffLimit);
	WSOpenBuffLimit(&pmod->UsrPort[i].OutBuffer,pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize,pmod->UscPort[pmod->UsrPort[i].UscPort].SendBuffLimit);
#ifdef WS_COMPRESS
	WSOpenBuffLimit(&pmod->UsrPort[i].InCompBuffer,pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize,pmod->UscPort[pmod->UsrPort[i].UscPort].RecvBuffLimit);
	WSOpenBuffLimit(&pmod->UsrPort[i].OutCompBuffer,pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize,pmod->UscPort[pmod->UsrPort[i].UscPort].SendBuffLimit);
#endif
#ifdef WS_ENCRYPTED
	#ifdef WSOCKS_SSL
    if(pmod->UscPort[PortNo].Encrypted==2)
	{
		#ifdef VS2010
        ((WSPort*)TSock)->pSSLInfo=SSLGateway::instance()->ServerSetup(((WSPort*)TSock)->sd);
		#else
        ((WSPort*)TSock)->pSSLInfo=SSLGateway::instance()->ServerSetup((int)((WSPort*)TSock)->sd);
		#endif
		if(SSLGateway::instance()->SSLAccept(((WSPort*)TSock)->pSSLInfo)==false)
		{
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : SSLAccept failed from USC%d %s:%d",PortNo,inet_ntoa(AccSin.sin_addr),ntohs(AccSin.sin_port));
			WSHClosePort(TSock);
			return(FALSE);
		}
	}
	else
	#endif
	if(pmod->UscPort[PortNo].Encrypted)
	{
		pmod->UsrPort[i].EncryptionOn=1;
		// Default encryption keys
		*((__int64*)(&pmod->UsrPort[i].pw[0]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UsrPort[i].pw[8]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UsrPort[i].pw[16]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UsrPort[i].Iiv[0]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UsrPort[i].Oiv[0]))=0x0123456780ABCDEF;
		WSHResetCryptKeys(pmod,i,WS_USR);
	}
#endif
	pmod->UsrPort[i].rmutcnt=0;
	pmod->UsrPort[i].rmutex=CreateMutex(0,false,0);
	pmod->UsrPort[i].smutcnt=0;
	pmod->UsrPort[i].smutex=CreateMutex(0,false,0);
	pmod->UsrPort[i].recvThread=0;
	pmod->UsrPort[i].sendThread=0;
	pmod->UsrPort[i].Sock=TSock;
	strcpy(pmod->UsrPort[i].Note,pmod->UscPort[j].CfgNote);
	pmod->UsrPort[i].SockActive=TRUE;
	LockPort(pmod,WS_USR,i,false);
	LockPort(pmod,WS_USR,i,true);
	#ifdef WIN32
	sprintf(pmod->UsrPort[i].RemoteIP,"%d.%d.%d.%d"
		,AccSin.sin_addr.S_un.S_un_b.s_b1
		,AccSin.sin_addr.S_un.S_un_b.s_b2
		,AccSin.sin_addr.S_un.S_un_b.s_b3
		,AccSin.sin_addr.S_un.S_un_b.s_b4);
	#else
	strcpy(pmod->UsrPort[i].RemoteIP,inet_ntoa(AccSin.sin_addr));
	#endif
	//AddConListItem(GetDispItem(WS_USR,i));
#ifdef WIN32
	UuidCreate((UUID*)pmod->UsrPort[i].Uuid);
#else
#endif
#ifdef WS_LOOPBACK
	// Detect loopback connections and remember the corresponding port
	if(!strcmp(pmod->UsrPort[i].RemoteIP,"127.0.0.1"))
	{
		for(int c=0;c<pmod->NO_OF_CON_PORTS;c++)
		{
			if((pmod->ConPort[c].SockConnected)&&
			   (pmod->ConPort[c].Port==pmod->UscPort[pmod->UsrPort[i].UscPort].Port)&&
			   (!strcmp(pmod->ConPort[c].RemoteIP,"127.0.0.1")))
			{
				SOCKADDR_IN paddr,laddr;
				int palen=sizeof(SOCKADDR_IN),lalen=sizeof(SOCKADDR_IN);
				WSHGetPeerName(pmod->ConPort[c].Sock,(SOCKADDR*)&paddr,&palen);
				WSHGetSockName(pmod->UsrPort[i].Sock,(SOCKADDR*)&laddr,&lalen);
				if((paddr.sin_addr.s_addr==laddr.sin_addr.s_addr)&&
				   (paddr.sin_port==laddr.sin_port))
				{
					if((pmod->UscPort[pmod->UsrPort[i].UscPort].Compressed)||(pmod->ConPort[c].Compressed))
						WSHLogError(pmod,"Loopback connection from USR%d to CON%d detected, but cannot be optimized due to compression.",PortNo,c);
					else
					{
						pmod->UsrPort[i].DetPtr=(void*)(PTRCAST)MAKELONG(c,127);
						pmod->ConPort[c].DetPtr=(void*)(PTRCAST)MAKELONG(PortNo,127);
						WSHLogEvent(pmod,"Loopback connection from USR%d to CON%d detected.",PortNo,c);
					}
				}
			}
		}
	}
#endif
	if(pmod->UscPort[pmod->UsrPort[i].UscPort].Recording.DoRec)
		WSHOpenRecording(pmod,&pmod->UsrPort[i].Recording,0,WS_USR,i);
	pmod->WSUsrOpened(i);
	UnlockPort(pmod,WS_USR,i,true);
	UnlockPort(pmod,WS_USR,i,false);
	WSHUpdatePort(pmod,WS_USR,i);

#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->UsrPort[i].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
	::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0);
	for(int o=0;o<WS_OVERLAP_MAX;o++)
	{
		if(WSHUsrIocpBegin(pmod,i)<0)
			return FALSE;
	}
	#ifdef WS_LOOPBACK
	}
	#endif
#elif defined(WS_OTPP)
	WSPort *pport=(WSPort*)pmod->UsrPort[i].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
	#ifdef WIN32
		#ifdef WS_OTMP
		if(pmod->pcfg->usrOtmp)
		{
			pmod->UsrPort[i].pthread=WSHPoolUsrReadThread(pmod,pport);
			if(!pmod->UsrPort[i].pthread)
			{
				WSHLogError(pmod,"USR%d: Failed assigning read thread: %d!",i,GetLastError());
				_WSHCloseUsrPort(pmod,i);
			}
			return TRUE;
		}
		#endif
		DWORD tid=0;
		UsrReadThreadData *ptd=new UsrReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
		pmod->UsrPort[i].pthread=CreateThread(0,0,_BootUsrReadThread,ptd,0,&tid);
		if(!pmod->UsrPort[i].pthread)
		{
			WSHLogError(pmod,"USR%d: Failed creating read thread: %d!",i,GetLastError());
			_WSHCloseUsrPort(pmod,i);
		}
	#else
		DWORD tid=0;
		UsrReadThreadData *ptd=new UsrReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
		pthread_create(&pmod->UsrPort[i].pthread,0,_BootUsrReadThread,ptd);
	#endif
	#ifdef WS_LOOPBACK
	}
	#endif
#endif
	return(TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHUsrIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	// Issue overlapped recvs
	WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->UsrPort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->UsrPort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->UsrPort[PortNo].InBuffer.BlockSize*1024;
	povl->bytes=0;
	povl->flags=0;

	int rc=WSARecv(pport->sd,&povl->wsabuf,1,&povl->bytes,&povl->flags,povl,0);
	if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
	{
		povl->Pending=true;
	}
	else
	{
		delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
		FreeOverlap(&pmod->UsrPort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHUsrReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
	int tsize=pmod->UsrPort[PortNo].InBuffer.BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->UscPort[pmod->UsrPort[PortNo].UscPort].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->UscPort[pmod->UsrPort[PortNo].UscPort].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->UsrPort[PortNo].Sock)// Account for TimeTillClose
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before locking the InBuffer and presenting it to the app.
		while((pmod->UsrPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
		{
			#ifdef WSOCKS_SSL
			int tbytes=-1;
			if(pmod->UscPort[pmod->UsrPort[PortNo].UscPort].Encrypted==2)
			{
				// SSL handshake not done yet
				if(SSLGateway::instance()->IsActivated(pport->pSSLInfo)==false)
				{
					if(SSLGateway::instance()->SSLAccept(pport->pSSLInfo)==false)
						tbytes=0;
					else
					{
						tbytes=-1; WSASetLastError(WSAEWOULDBLOCK);
					}
				}
				else
				{
					// We need to check socket for readability (or close notification) first. 
					// Otherwise, SSLGateway::SSLRecv always returns>0 and we never get out of this loop
					struct timeval TimeVal={0,0};
					fd_set Fd_Set;
					WS_FD_ZERO(&Fd_Set);
					WS_FD_SET((SOCKET)pport,&Fd_Set);
					WSHSelect((LONGLONG)pport+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
					int Status=WS_FD_ISSET(pport,&Fd_Set);
					if(Status)
					{
						tbytes=SSLGateway::instance()->SSLRecv(pport->pSSLInfo,tbuf,tsize);
						//if(tbytes<0)
						//	tbytes=0;
					}
				}
			}
			else
				tbytes=recv(pport->sd,tbuf,tsize,0);
			#else
			int tbytes=recv(pport->sd,tbuf,tsize,0);
			#endif
			if(tbytes<0)
			{
				err=WSAGetLastError();
				break;
			}
			else if(!tbytes)
			{
				err=WSAECONNRESET;
				break;
			}
			if(!WSWriteBuff(&TBuffer,tbuf,tbytes))
			{
				err=WSAENOBUFS; _ASSERT(false);
				break;
			}
		}
		pmod->UsrPort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port
		if((TBuffer.Size>0)&&(pmod->UsrPort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_USR,PortNo,false);
			if(pmod->UsrPort[PortNo].Sock)
			{
				int lastRecv=pmod->UsrPort[PortNo].recvThread;
				pmod->UsrPort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->UsrPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->UsrPort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->UsrPort[PortNo].InBuffer.BlockSize*1024;
					WSHUsrRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->UsrPort[PortNo].TBufferSize=TBuffer.Size;
				pmod->UsrPort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_USR,PortNo,false);
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->UsrPort[PortNo].Sock))
		pmod->UsrPort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#ifdef WS_OTMP
int WsocksHostImpl::WSHUsrReadPoolThread(WsocksApp *pmod, THREADPOOL *tpool)
{
	DWORD tid=GetCurrentThreadId();
	while(!tpool->shutdown)
	{
		int nactive=0;
		for(int i=0;i<tpool->nports;i++)
		{
			WSPort *&pport=tpool->ports[i];
			if(!pport)
				continue;
			nactive++;
			int PortNo=pport->PortNo;
			int& tsize=tpool->tsizes[i];
			char *&tbuf=tpool->tbufs[i];
			BUFFER& TBuffer=tpool->TBuffers[i];
			DWORD& RecvBuffLimit=tpool->RecvBuffLimits[i];
			// Port closed by app
			if(!pmod->UsrPort[PortNo].Sock)
			{
				if(tbuf)
				{
					WSCloseBuff(&TBuffer); memset(&TBuffer,0,sizeof(BUFFER));
					delete tbuf; tbuf=0; tsize=0;
					RecvBuffLimit=0; pport=0;
				}
				continue;
			}
			// One-time initialization per port
			if(!tbuf)
			{
				tsize=pmod->UsrPort[PortNo].InBuffer.BlockSize*1024;
				tbuf=new char[tsize];
				RecvBuffLimit=(DWORD)pmod->UscPort[pmod->UsrPort[PortNo].UscPort].RecvBuffLimit*1024;
				if(!RecvBuffLimit)
					RecvBuffLimit=ULONG_MAX;
				WSOpenBuffLimit(&TBuffer,pmod->UscPort[pmod->UsrPort[PortNo].UscPort].BlockSize,RecvBuffLimit/1024);
			}
			// Port loop
			int err=0;
			while(pmod->UsrPort[PortNo].Sock)// Account for TimeTillClose
			{
				// This implementation increases throughput by reading from the socket as long as data is 
				// available (up to some limit) before locking the InBuffer and presenting it to the app.
				while((pmod->UsrPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
				{
					#ifdef WSOCKS_SSL
					int tbytes=-1;
					if(pmod->UscPort[pmod->UsrPort[PortNo].UscPort].Encrypted==2)
					{
						// SSL handshake not done yet
						if(SSLGateway::instance()->IsActivated(pport->pSSLInfo)==false)
						{
							if(SSLGateway::instance()->SSLAccept(pport->pSSLInfo)==false)
								tbytes=0;
							else
							{
								tbytes=-1; WSASetLastError(WSAEWOULDBLOCK);
							}
						}
						else
						{
							// We need to check socket for readability (or close notification) first. 
							// Otherwise, SSLGateway::SSLRecv always returns>0 and we never get out of this loop
							struct timeval TimeVal={0,0};
							fd_set Fd_Set;
							WS_FD_ZERO(&Fd_Set);
							WS_FD_SET((SOCKET)pport,&Fd_Set);
							WSHSelect((LONGLONG)pport+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
							int Status=WS_FD_ISSET(pport,&Fd_Set);
							if(Status)
							{
								tbytes=SSLGateway::instance()->SSLRecv(pport->pSSLInfo,tbuf,tsize);
								//if(tbytes<0)
								//	tbytes=0;
							}
						}
					}
					else
						tbytes=recv(pport->sd,tbuf,tsize,0);
					#else
					int tbytes=recv(pport->sd,tbuf,tsize,0);
					#endif
					if(tbytes<0)
					{
						err=WSAGetLastError();
						break;
					}
					else if(!tbytes)
					{
						err=WSAECONNRESET;
						break;
					}
					if(!WSWriteBuff(&TBuffer,tbuf,tbytes))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->UsrPort[PortNo].TBufferSize=TBuffer.Size;
				// Save what we've pulled off the port
				if((TBuffer.Size>0)&&(pmod->UsrPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					LockPort(pmod,WS_USR,PortNo,false);
					if(pmod->UsrPort[PortNo].Sock)
					{
						int lastRecv=pmod->UsrPort[PortNo].recvThread;
						pmod->UsrPort[PortNo].recvThread=tid;
						while((TBuffer.Size>0)&&(pmod->UsrPort[PortNo].InBuffer.Size<RecvBuffLimit))
						{
							DWORD rsize=TBuffer.LocalSize;
							if(rsize>pmod->UsrPort[PortNo].InBuffer.BlockSize*1024)
								rsize=pmod->UsrPort[PortNo].InBuffer.BlockSize*1024;
							WSHUsrRead(pmod,PortNo,TBuffer.Block,rsize);
							if(!WSStripBuff(&TBuffer,rsize))
							{
								err=WSAENOBUFS; _ASSERT(false);
								break;
							}
						}
						pmod->UsrPort[PortNo].TBufferSize=TBuffer.Size;
						pmod->UsrPort[PortNo].recvThread=lastRecv;
					}
					else
						err=WSAECONNABORTED;
					UnlockPort(pmod,WS_USR,PortNo,false);
				}
				// This loop is kept a 'while' instead of 'if' to preserve 
				// 'break' statements above, but it really functions like an 'if'
				// so we can handle the next port
				break; 
			}
			// One-time cleanup
			if((err)&&(err!=WSAEWOULDBLOCK))
			{
				if((err!=WSAECONNABORTED)&&(pmod->UsrPort[PortNo].Sock))
					pmod->UsrPort[PortNo].peerClosed=true;
				WSCloseBuff(&TBuffer); memset(&TBuffer,0,sizeof(BUFFER));
				delete tbuf; tbuf=0; tsize=0;
				RecvBuffLimit=0; pport=0;
			}
		}
		if((tpool->nports>=WS_NTMP)&&(nactive<1))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	return 0;
}
#endif//WS_OTMP
#endif//WS_OIO
#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHUsrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	//pmod->UsrPort[i].LastDataTime=GetTickCount();
	pmod->UsrPort[i].BytesIn+=bytes;
	pmod->UsrPort[i].BlocksIn++;

#ifdef WS_LOOPBACK
	WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
	if((pport)&&(pport->lbaPeer))
	{
		while(pport->lbaBuffer->Size>0)
		{
			unsigned int size=pport->lbaBuffer->LocalSize;
			if(size>pmod->UsrPort[i].InBuffer.BlockSize*1024)
				size=pmod->UsrPort[i].InBuffer.BlockSize*1024;
			if(!WSWriteBuff(&pmod->UsrPort[i].InBuffer,pport->lbaBuffer->Block,size))
				return(FALSE);
			WSStripBuff(pport->lbaBuffer,size);
		}
		return(TRUE);
	}
#endif
#ifdef WS_COMPRESS
	if((pmod->UscPort[pmod->UsrPort[i].UscPort].Compressed)||(pmod->UscPort[pmod->UsrPort[i].UscPort].CompressType>0))
	{
		WSHWaitMutex(0x01,INFINITE);
		char *szTemp=pmod->UsrRead_szTemp;
		char *szDecompBuff=pmod->UsrRead_szDecompBuff;
		#ifdef WS_ENCRYPTED
		char *szDecryptBuff=pmod->UsrRead_szDecryptBuff;
		unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
		#endif
		if(!WSWriteBuff(&pmod->UsrPort[i].InCompBuffer,(char*)buf,bytes))
		{
			WSHReleaseMutex(0x01);
			return(FALSE);
		}
GetNextBlock:
		if(pmod->UsrPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
		{
			unsigned int *CompSize=(unsigned int*)pmod->UsrPort[i].InCompBuffer.Block;
			unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

			if(pmod->UsrPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
			{
			#ifdef WS_ENCRYPTED
				if(pmod->UsrPort[i].EncryptionOn)
				{
					WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
						,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_USR);
				}
				else
				{
					memcpy(szDecryptBuff,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
					DecryptSize=*CompSize;
				}
				if(uncompress(szDecompBuff,&DecompSize
					,szDecryptBuff,DecryptSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUsrPort(pmod,i);
					return(FALSE);
				}
			#else
				if(pmod->UscPort[pmod->UsrPort[i].UscPort].CompressType==2)
				{
					if(csuncompress(szDecompBuff,&DecompSize
						,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],CompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUsrPort(pmod,i);
						return(FALSE);
					}
				}
				else if(uncompress(szDecompBuff,&DecompSize
					,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUsrPort(pmod,i);
					return(FALSE);
				}
			#endif

				WSHRecord(pmod,&pmod->UsrPort[i].Recording,szDecompBuff,DecompSize,false);
				WSStripBuff(&pmod->UsrPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
				if(!WSWriteBuff(&pmod->UsrPort[i].InBuffer,szDecompBuff,DecompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUsrPort(pmod,i);
					return(FALSE);
				}
				goto GetNextBlock;
			}
		}
		WSHReleaseMutex(0x01);
	}
	else
#endif//WS_COMPRESS
	{
		WSHRecord(pmod,&pmod->UsrPort[i].Recording,buf,bytes,false);
		if(!WSWriteBuff(&pmod->UsrPort[i].InBuffer,(char*)buf,bytes))
		{
			WSHCloseUsrPort(pmod,i);
			return(FALSE);
		}
	}
	return TRUE;
}
#else//!WS_OIO
int WsocksHostImpl::WSHUsrRead(WsocksApp *pmod, int PortNo)
{
	//static char szTemp[(WS_MAX_BLOCK_SIZE*1024)];
	char *szTemp=pmod->UsrRead_szTemp;
#ifdef WS_COMPRESS
	//static char szDecompBuff[(WS_MAX_BLOCK_SIZE*1024)];
	char *szDecompBuff=pmod->UsrRead_szDecompBuff;
#ifdef WS_ENCRYPTED
	//static char szDecryptBuff[(WS_MAX_BLOCK_SIZE*1024)];
	char *szDecryptBuff=pmod->UsrRead_szDecryptBuff;
	unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
#endif
#endif
	int status;             // Status Code 
	int i=PortNo;

	if((pmod->UsrPort[i].InBuffer.Busy)&&(pmod->UsrPort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: USR%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->UsrPort[i].InBuffer.Busy);
		pmod->UsrPort[i].InBuffer.Busy=0;
	}
	pmod->UsrPort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common UsrRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
#ifdef WS_LOOPBACK
	WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
	if((pport)&&(pport->lbaPeer))
	{
		while(pport->lbaBuffer->Size>0)
		{
			unsigned int size=pport->lbaBuffer->LocalSize;
			if(size>pmod->UsrPort[i].InBuffer.BlockSize*1024)
				size=pmod->UsrPort[i].InBuffer.BlockSize*1024;
			if(!WSWriteBuff(&pmod->UsrPort[i].InBuffer,pport->lbaBuffer->Block,size))
			{
				WSHReleaseMutex(0x01);
				pmod->UsrPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
			WSStripBuff(pport->lbaBuffer,size);
		}
		WSHReleaseMutex(0x01);
		pmod->UsrPort[i].InBuffer.Busy=lastBusy;
		return(TRUE);
	}
#endif
#ifdef WS_COMPRESS
	if((pmod->UscPort[pmod->UsrPort[i].UscPort].Compressed)||(pmod->UscPort[pmod->UsrPort[i].UscPort].CompressType>0))
	{
		status = WSHRecv(pmod->UsrPort[i].Sock, szTemp, ((pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize-1)*1024), NO_FLAGS_SET );
		if (status>0) 
		{
			pmod->UsrPort[i].BytesIn+=status;
			pmod->UsrPort[i].BlocksIn++;

			if(!WSWriteBuff(&pmod->UsrPort[i].InCompBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				pmod->UsrPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
GetNextBlock:
			if(pmod->UsrPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
			{
				unsigned int *CompSize=(unsigned int*)pmod->UsrPort[i].InCompBuffer.Block;
				unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

				if(pmod->UsrPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
				{
#ifdef WS_ENCRYPTED
					if(pmod->UsrPort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_USR);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize
						,szDecryptBuff,DecryptSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUsrPort(pmod,i);
						return(FALSE);
					}
#else
					if(pmod->UscPort[pmod->UsrPort[i].UscPort].CompressType==2)
					{
						if(csuncompress(szDecompBuff,&DecompSize
							,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],CompSize))
						{
							WSHReleaseMutex(0x01);
							WSHCloseUsrPort(pmod,i);
							return(FALSE);
						}
					}
					else if(uncompress(szDecompBuff,&DecompSize
						,&pmod->UsrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUsrPort(pmod,i);
						return(FALSE);
					}
#endif

					WSHRecord(pmod,&pmod->UsrPort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->UsrPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
					if(!WSWriteBuff(&pmod->UsrPort[i].InBuffer,szDecompBuff,DecompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUsrPort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
			}
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->UsrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUsrMsgReady(i))
					pmod->WSUsrStripMsg(i);
			}
			WSHCloseUsrPort(pmod,i);
			return(FALSE);
		}
	}
	else
#endif
	{
		status = WSHRecv(pmod->UsrPort[i].Sock, szTemp, ((pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize-1)*1024), NO_FLAGS_SET );

		if (status>0) 
		{
			pmod->UsrPort[i].BytesIn+=status;
			if(status==132||status==8)
			{
				if(*((WORD *)szTemp)!=16)
				{
					pmod->UsrPort[i].BlocksIn++;
				}
			}
			else
			{
				pmod->UsrPort[i].BlocksIn++;
			}
			WSHRecord(pmod,&pmod->UsrPort[i].Recording,szTemp,status,false);
			if(!WSWriteBuff(&pmod->UsrPort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseUsrPort(pmod,i);
				return(FALSE);
			}
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->UsrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUsrMsgReady(i))
					pmod->WSUsrStripMsg(i);
			}
			WSHCloseUsrPort(pmod,i);
			return(FALSE);
		}
	}
	WSHReleaseMutex(0x01);
	pmod->UsrPort[i].InBuffer.Busy=lastBusy;
	return(TRUE);
}
#endif//!WS_OIO

int WsocksHostImpl::WSHUsrSend(WsocksApp *pmod, int PortNo, bool flush)
{
	int nsent=0;
	if(!pmod->UsrPort[PortNo].SockActive)
		return -1;
	#ifdef WSOCKS_SSL
	// Wait for SSL handshake to complete before sending anything
	if(pmod->UscPort[pmod->UsrPort[PortNo].UscPort].Encrypted==2)
	{
		if(SSLGateway::instance()->IsActivated(((WSPort*)pmod->UsrPort[PortNo].Sock)->pSSLInfo)==false)
			return 0;
	}
	#endif

	bool compressed=false;
	int lastBusy=pmod->UsrPort[PortNo].OutBuffer.Busy;
	pmod->UsrPort[PortNo].OutBuffer.Busy=pmod->UsrPort[PortNo].sendThread;
	pmod->UsrPort[PortNo].SendTimeOut=0;
	BUFFER *sendBuffer=&pmod->UsrPort[PortNo].OutBuffer;
#ifdef WS_COMPRESS
	if(pmod->UscPort[pmod->UsrPort[PortNo].UscPort].Compressed)
	{
		compressed=true;
		sendBuffer=&pmod->UsrPort[PortNo].OutCompBuffer;
		// Compress as much as possible to fill one send block
		while((pmod->UsrPort[PortNo].OutBuffer.Size>0)&&
			  ((flush)||((int)pmod->UsrPort[PortNo].OutBuffer.Size>=pmod->UsrPort[PortNo].ImmediateSendLimit)))
		{
			unsigned int CompSize=pmod->UsrPort[PortNo].OutBuffer.LocalSize;
			if(CompSize>((WS_COMP_BLOCK_LEN*1024)*99/100))
				CompSize=((WS_COMP_BLOCK_LEN*1024)*99/100);
			if(CompSize>0)
			{
				WSHWaitMutex(0x01,INFINITE);
				char *CompBuff=pmod->SyncLoop_CompBuff;
				unsigned int CompBuffSize=sizeof(pmod->SyncLoop_CompBuff);
				mtcompress(CompBuff,&CompBuffSize,pmod->UsrPort[PortNo].OutBuffer.Block,CompSize);
				// Compression +encryption
				#ifdef WS_ENCRYPTED
				char *EncryptBuff=CompBuff;
				unsigned int EncryptSize=CompBuffSize;
				if(pmod->UsrPort[PortNo].EncryptionOn)
				{
					EncryptBuff=pmod->SyncLoop_EncryptBuff;
					EncryptSize=sizeof(pmod->SyncLoop_EncryptBuff);
					WSHEncrypt(pmod,EncryptBuff,&EncryptSize,CompBuff,CompBuffSize,PortNo,WS_USR);
				}
				WSWriteBuff(sendBuffer,(char*)&EncryptSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,EncryptBuff,EncryptSize);
				// Compression only
				#else//!WS_ENCRYPTED
				WSWriteBuff(sendBuffer,(char*)&CompBuffSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,CompBuff,CompBuffSize);
				#endif//WS_ENCRYPTED
				WSHRecord(pmod,&pmod->UsrPort[PortNo].Recording,pmod->UsrPort[PortNo].OutBuffer.Block,CompSize,true);
				WSStripBuff(&pmod->UsrPort[PortNo].OutBuffer,CompSize);
				WSHReleaseMutex(0x01);
			}
		}
	}
#endif//WS_COMPRESS
	// Send as much as possible
	while((sendBuffer->Size>0)&&((flush)||(compressed)||((int)sendBuffer->Size>=pmod->UsrPort[PortNo].ImmediateSendLimit)))
	{
		int size=sendBuffer->LocalSize;
		if(size>pmod->UscPort[pmod->UsrPort[PortNo].UscPort].BlockSize*1024)
			size=pmod->UscPort[pmod->UsrPort[PortNo].UscPort].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->UsrPort[PortNo].Sock;
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=WSHSendNonBlock(pmod,WS_USR,PortNo,(SOCKET_T)pport,sendBuffer->Block,size);
		#else
		int rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_USR_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->UsrPort[PortNo].SendTimeOut)
				{
					pmod->UsrPort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->UsrPort[PortNo].SendTimeOut<WS_USR_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: USR%d [%s] Reset by Peer",PortNo,pmod->UscPort[pmod->UsrPort[PortNo].UscPort].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: USR%d [%s] Send Failed: %d",PortNo,pmod->UscPort[pmod->UsrPort[PortNo].UscPort].CfgNote,lerr);
				WSHCloseUsrPort(pmod,PortNo);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		if(!compressed)
			WSHRecord(pmod,&pmod->UsrPort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->UsrPort[PortNo].BytesOut+=rc;
		pmod->UsrPort[PortNo].BlocksOut++;
		pmod->UsrPort[PortNo].SendTimeOut=0;
		nsent+=rc;
	}
	pmod->UsrPort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}
