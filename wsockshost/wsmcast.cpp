
#include "stdafx.h"
#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#ifdef WIN32
#include <shlwapi.h>
#include <windowsx.h>
#endif

int WsocksHostImpl::WSHSetupCtoPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=0;i<=pmod->NO_OF_CTO_PORTS;i++)
		{
			memset(&pmod->CtoPort[i],0,sizeof(CTOPORT));
			pmod->CtoPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
		#ifdef WS_OIO
			pmod->CtoPort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		#endif
		}
		break;
	case WS_OPEN: // Open
		for (i=0;i<pmod->NO_OF_CTO_PORTS;i++)
		{
			if(strlen(pmod->CtoPort[i].LocalIP)>0)
			{
				pmod->CtoPort[i].smutex=CreateMutex(0,false,0);
				pmod->CtoPort[i].sendThread=0;
				WSOpenBuffLimit(&pmod->CtoPort[i].OutBuffer,pmod->CtoPort[i].BlockSize,pmod->CtoPort[i].SendBuffLimit);
				pmod->CtoPort[i].InUse=TRUE;
				//AddConListItem(GetDispItem(WS_CTO,i));
			}
		}
		//AddConListItem(GetDispItem(WS_CTO_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=0;i<=pmod->NO_OF_CTO_PORTS;i++)
		{
			_WSHCloseCtoPort(pmod,i);
			pmod->CtoPort[i].sendThread=0;
			if(pmod->CtoPort[i].smutex)
			{
				DeleteMutex(pmod->CtoPort[i].smutex); pmod->CtoPort[i].smutex=0;
			}
			if(pmod->CtoPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_CTO,i));
				pmod->CtoPort[i].InUse=FALSE;
			}
		}
		break;
	}
	return(TRUE);
}
int WsocksHostImpl::WSHOpenCtoPort(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->CtoPort[PortNo].BlockSize*8192;
	int one = 1;

	int cnt;

	//pmod->CtoPort[PortNo].Sock = socket(AF_INET, SOCK_DGRAM, 0);
	pmod->CtoPort[PortNo].Sock = WSHSocket(pmod,WS_CTO,PortNo);
	if (pmod->CtoPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->CtoPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket socket() failed with error=%d",errno);
		perror(NULL);
		return(FALSE);
	}
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->CtoPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->CtoPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket ioctlsocket() failed");
		_WSHCloseCtoPort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->CtoPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt() failed");
		_WSHCloseCtoPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->CtoPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt() failed");
		_WSHCloseCtoPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->CtoPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt() failed");
		_WSHCloseCtoPort(pmod,PortNo);
		return(FALSE);
	}

	//   Retrieve the local IP address and TCP Port number
    
	local_sin.sin_family=AF_INET;
	local_sin.sin_port=INADDR_ANY;

	if (strcmp(pmod->CtoPort[PortNo].LocalIP,"AUTO")==0)
		local_sin.sin_addr.s_addr=INADDR_ANY;
	else
		local_sin.sin_addr.s_addr=inet_addr(pmod->CtoPort[PortNo].LocalIP);

   //  Associate an address with a socket. (bind)
         
	//if (bind( pmod->CtoPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
	if (WSHBindPort( pmod->CtoPort[PortNo].Sock, &local_sin.sin_addr, INADDR_ANY) == SOCKET_ERROR) 
	{
		pmod->CtoPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket bind(%s) failed",inet_ntoa(local_sin.sin_addr));
		_WSHCloseCtoPort(pmod,PortNo);
		return(FALSE);
	}

	strcpy (pmod->CtoPort[PortNo].Note, pmod->CtoPort[PortNo].CfgNote);
	cnt = 0;

	// UDP broadcast instead of multicast
	if(!strcmp(pmod->CtoPort[PortNo].RemoteIP,"255.255.255.255"))
	{
		if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, SOL_SOCKET, SO_BROADCAST,(char *)(&one), sizeof(int)) == SOCKET_ERROR) 
		{
			pmod->CtoPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt(SO_BROADCAST) failed");
			_WSHCloseCtoPort(pmod,PortNo);
			return(FALSE);
		}

		pmod->CtoPort[PortNo].cast_addr.sin_family = AF_INET;
		pmod->CtoPort[PortNo].cast_addr.sin_port=htons(pmod->CtoPort[PortNo].Port);
		pmod->CtoPort[PortNo].cast_addr.sin_addr.s_addr=INADDR_ANY;
	}
	// Multicast
	else if(!strncmp(pmod->CtoPort[PortNo].RemoteIP,"224.",4))
	{
		if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, IPPROTO_IP, IP_MULTICAST_TTL,(char *)(&one), sizeof(int)) == SOCKET_ERROR) 
		{
			pmod->CtoPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt(IP_MULTICAST_TTL) failed");
			_WSHCloseCtoPort(pmod,PortNo);
			return(FALSE);
		}
		struct tdmreq
		{
		struct in_addr imr_multiaddr;  //The multicast group to join 
		struct in_addr imr_interface;  //The interface to join on
		} mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(pmod->CtoPort[PortNo].RemoteIP);
		mreq.imr_interface.s_addr = local_sin.sin_addr.s_addr;
		if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) 
		{
			pmod->CtoPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt(IP_ADD_MEMBERSHIP) failed");
			_WSHCloseCtoPort(pmod,PortNo);
			return(FALSE);
		}

		pmod->CtoPort[PortNo].cast_addr.sin_family = AF_INET;
		pmod->CtoPort[PortNo].cast_addr.sin_port=htons(pmod->CtoPort[PortNo].Port);
		pmod->CtoPort[PortNo].cast_addr.sin_addr.s_addr=inet_addr(pmod->CtoPort[PortNo].RemoteIP);

	#ifdef WS_CAST_LOOPBACK
		int mloop=1;
		if (WSHSetSockOpt(pmod->CtoPort[PortNo].Sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&mloop, sizeof(mloop)) == SOCKET_ERROR) 
		{
			pmod->CtoPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtoSocket setsockopt(IP_MULTICAST_LOOP) failed");
			_WSHCloseCtoPort(pmod,PortNo);
			return(FALSE);
		}
	#endif
	}
	// UDP unicast
	else
	{
		pmod->CtoPort[PortNo].cast_addr.sin_family = AF_INET;
		pmod->CtoPort[PortNo].cast_addr.sin_port=htons(pmod->CtoPort[PortNo].Port);
		pmod->CtoPort[PortNo].cast_addr.sin_addr.s_addr=inet_addr(pmod->CtoPort[PortNo].RemoteIP);
	}
	WSCloseBuff(&pmod->CtoPort[PortNo].OutBuffer);
	WSOpenBuffLimit(&pmod->CtoPort[PortNo].OutBuffer,pmod->CtoPort[PortNo].BlockSize,pmod->CtoPort[PortNo].SendBuffLimit);
	pmod->CtoPort[PortNo].SockActive=TRUE;
#ifdef WIN32
	UuidCreate((UUID*)pmod->CtoPort[PortNo].Uuid);
#endif
	WSHUpdatePort(pmod,WS_CTO,PortNo);
	pmod->WSCtoOpened(PortNo);

#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->CtoPort[PortNo].Sock;
	if(!::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0))
		return FALSE;
#endif
	return(TRUE);
}
// This implementation of send won't fragment nor aggregate packets
int WsocksHostImpl::WSHCtoSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CTO_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>pmod->CtoPort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_CTO,PortNo,true);
	int lastSend=pmod->CtoPort[PortNo].sendThread;
	pmod->CtoPort[PortNo].sendThread=GetCurrentThreadId();
	if(pmod->CtoPort[PortNo].SockActive)
	{
		if((pmod->CtoPort[PortNo].OutBuffer.Busy)&&(pmod->CtoPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: CTO%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->CtoPort[PortNo].OutBuffer.Busy);
			pmod->CtoPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->CtoPort[PortNo].OutBuffer.Busy;
		pmod->CtoPort[PortNo].OutBuffer.Busy=pmod->CtoPort[PortNo].sendThread;
		//Send Data
		if(!WSWriteBuff(&pmod->CtoPort[PortNo].OutBuffer,MsgOut,MsgLen))
		{
			pmod->CtoPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->CtoPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CTO,PortNo,true);
			return(FALSE);
		}
		pmod->WSBeforeCtoSend(PortNo);
		WSHCtoSend(pmod,PortNo);
		pmod->CtoPort[PortNo].OutBuffer.Busy=lastBusy;
		pmod->CtoPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CTO,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->CtoPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CTO,PortNo,true);
		return(FALSE);
	}
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseCtoPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CTO_PORTS))
		return -1;
	if(!pmod->CtoPort[PortNo].InUse)
		return -1;
	if(pmod->CtoPort[PortNo].SockActive)
		pmod->CtoPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseCtoPort(WsocksApp *pmod, int PortNo)
{
	int lastSend=0;
	if(pmod->CtoPort[PortNo].smutex)
	{
		LockPort(pmod,WS_CTO,PortNo,true);
		lastSend=pmod->CtoPort[PortNo].sendThread;
		pmod->CtoPort[PortNo].sendThread=GetCurrentThreadId();
		if(pmod->CtoPort[PortNo].pendingClose)
		{
			pmod->CtoPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CTO,PortNo,true);
			return 0;
		}
		pmod->CtoPort[PortNo].pendingClose=true;
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->CtoPort[PortNo].pendOvlRecvList; pmod->CtoPort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->CtoPort[PortNo].pendOvlSendList; pmod->CtoPort[PortNo].pendOvlSendList=0;
	if(pendOvlSendList)
	{		
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
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

	//ResetSendTimeout(WS_CTO,PortNo);
	if(pmod->CtoPort[PortNo].Sock != 0)
		WSHClosePort(pmod->CtoPort[PortNo].Sock);
	WSCloseBuff(&pmod->CtoPort[PortNo].OutBuffer);
	memset(&pmod->CtoPort[PortNo].SockActive,0
		,sizeof(CTOPORT)-(int)((char *)&pmod->CtoPort[PortNo].SockActive-(char *)&pmod->CtoPort[PortNo]));
	if(pmod->CtoPort[PortNo].SockActive)
	{
		pmod->CtoPort[PortNo].SockActive=0;
		pmod->WSCtoClosed(PortNo);
	}
	if(pmod->CtoPort[PortNo].smutex)
	{
		pmod->CtoPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CTO,PortNo,true);
	}
	WSHUpdatePort(pmod,WS_CTO,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseCtoPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CTO_PORTS))
		return -1;
	if(!pmod->CtoPort[PortNo].InUse)
		return -1;
	if(pmod->CtoPort[PortNo].SockActive)
		pmod->CtoPort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseCtoPort(WsocksApp *pmod, int PortNo)
{
	int lastSend=0;
	if(pmod->CtoPort[PortNo].smutex)
	{
		LockPort(pmod,WS_CTO,PortNo,true);
		lastSend=pmod->CtoPort[PortNo].sendThread;
		pmod->CtoPort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_CTO,PortNo);
	if(pmod->CtoPort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->CtoPort[PortNo].Sock)->sd;
		WSHClosePort(pmod->CtoPort[PortNo].Sock);
		WSHFinishOverlapSend(pmod,sd,&pmod->CtoPort[PortNo].pendOvlSendBeg,&pmod->CtoPort[PortNo].pendOvlSendEnd);
	}
	WSCloseBuff(&pmod->CtoPort[PortNo].OutBuffer);
	if(pmod->CtoPort[PortNo].SockActive)
	{
		pmod->CtoPort[PortNo].SockActive=0;
		pmod->WSCtoClosed(PortNo);
	}

	memset(&pmod->CtoPort[PortNo].SockActive,0
		,sizeof(CTOPORT)-(int)((char *)&pmod->CtoPort[PortNo].SockActive-(char *)&pmod->CtoPort[PortNo]));
	if(pmod->CtoPort[PortNo].smutex)
	{
		pmod->CtoPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CTO,PortNo,true);
	}
	WSHUpdatePort(pmod,WS_CTO,PortNo);
	return (TRUE);
}
#else//!WS_OIO
int WsocksHostImpl::WSHCloseCtoPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->CtoPort[PortNo].smutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_CTO,PortNo,true);
		DWORD tid=GetCurrentThreadId();
		while((pmod->CtoPort[PortNo].sendThread)&&(pmod->CtoPort[PortNo].sendThread!=tid))
		{
			UnlockPort(pmod,WS_CTO,PortNo,true);
			SleepEx(100,true);
			LockPort(pmod,WS_CTO,PortNo,true);
		}
		pmod->CtoPort[PortNo].sendThread=tid;
	}
	//ResetSendTimeout(WS_CTO,PortNo);
	if(pmod->CtoPort[PortNo].Sock != 0)
		WSHClosePort(pmod->CtoPort[PortNo].Sock);
	WSCloseBuff(&pmod->CtoPort[PortNo].OutBuffer);
	memset(&pmod->CtoPort[PortNo].SockActive,0
		,sizeof(CTOPORT)-(int)((char *)&pmod->CtoPort[PortNo].SockActive-(char *)&pmod->CtoPort[PortNo]));
	pmod->WSCtoClosed(PortNo);

	if(pmod->CtoPort[PortNo].smutex)
	{
		pmod->CtoPort[PortNo].sendThread=0;
		UnlockPort(pmod,WS_CTO,PortNo,true);
	}
	//WSHPortChanged(pmod,pmod->CtoPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO

int WsocksHostImpl::WSHSetupCtiPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=0;i<=pmod->NO_OF_CTI_PORTS;i++)
		{
			memset(&pmod->CtiPort[i],0,sizeof(CTIPORT));
			pmod->CtiPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
			pmod->CtiPort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
		}
		break;
	case WS_OPEN: // Open
		for (i=0;i<pmod->NO_OF_CTI_PORTS;i++)
		{
			if(strlen(pmod->CtiPort[i].LocalIP)>0)
			{
				pmod->CtiPort[i].rmutex=CreateMutex(0,false,0);
				pmod->CtiPort[i].recvThread=0;
				WSOpenBuffLimit(&pmod->CtiPort[i].InBuffer,pmod->CtiPort[i].BlockSize,pmod->CtiPort[i].RecvBuffLimit);
				pmod->CtiPort[i].InUse=TRUE;
				//AddConListItem(GetDispItem(WS_CTI,i));
			}
		}
		//AddConListItem(GetDispItem(WS_CTI_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=0;i<=pmod->NO_OF_CTI_PORTS;i++)
		{
			WSHCloseRecording(pmod,&pmod->CtiPort[i].Recording,WS_CTI,i);
			_WSHCloseCtiPort(pmod,i);
			pmod->CtiPort[i].recvThread=0;
			if(pmod->CtiPort[i].rmutex)
			{
				DeleteMutex(pmod->CtiPort[i].rmutex); pmod->CtiPort[i].rmutex=0;
			}
			if(pmod->CtiPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_CTI,i));
				pmod->CtiPort[i].InUse=FALSE;
			}
			#ifdef WS_OTPP
			_WSHWaitCtiThreadExit(pmod,i);
			#endif
		}
		break;
	}
	return(TRUE);
}
static int CompareIpAcl(const void *e1, const void *e2)
{
	IPACL *pacl1=(IPACL *)e1;
	IPACL *pacl2=(IPACL *)e2;
	// The search item will have a zero mask
	DWORD mask=pacl1->lMask;
	if(!mask) mask=pacl2->lMask;
	return (pacl1->lIp &mask) -(pacl2->lIp &mask);
}
int WsocksHostImpl::WSHCfgCtiPort(WsocksApp *pmod, int PortNo)
{
    char cfgcpy[80];
    strcpy(cfgcpy,pmod->CtiPort[PortNo].IPAclCfg);
    char *inptr=cfgcpy;
    for( int i=0; (inptr=strtok(i?0:inptr,","))!=NULL; i++ )
    {
        switch ( i %2 )
        {
        case 0:
            if (PathFileExists(inptr))
            {
                if(!WSHLoadIPAclFile(pmod,inptr,&pmod->CtiPort[PortNo].IPAclList))
					return FALSE;
                i++;
                break;
            }
            else
            {
                IPACL *pacl=(IPACL*)calloc(1,sizeof(IPACL));
                pacl->NextIPAcl=pmod->CtiPort[PortNo].IPAclList;
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
				pmod->CtiPort[PortNo].IPAclList = pacl;
            }                        
            break;
        case 1:
            strcpy(pmod->CtiPort[PortNo].IPAclList->Mask,inptr);
            pmod->CtiPort[PortNo].IPAclList->lMask=
                inet_addr(pmod->CtiPort[PortNo].IPAclList->Mask);
            break;
        }
    }
    if (pmod->CtiPort[PortNo].IPAclList&&
        ((!pmod->CtiPort[PortNo].IPAclList->lIp)||(!pmod->CtiPort[PortNo].IPAclList->lMask)))
    {
        WSHLogError(pmod,"Incomplete ACL for Cti Port %d",PortNo);
        return FALSE;
    }

	// Make an IP map to accelerate lookups
	// We can't include CMap or STL map, so use qsort/bsearch
	if(pmod->CtiPort[PortNo].IPAclList)
	{
		IPACLMAP *amap=&pmod->CtiPort[PortNo].IPAclMap;
		for(IPACL *pacl=pmod->CtiPort[PortNo].IPAclList;pacl;pacl=pacl->NextIPAcl)
		{
			if(amap->cnt>=amap->size)
			{
				int nsize=amap->size*2;
				if(!nsize) nsize=8;
				amap->Acls=(IPACL*)realloc(amap->Acls,nsize*sizeof(IPACL));
				if(!amap->Acls)
				{
					WSHLogError(pmod,"Failed growing ACL map for %d bytes on Cti Port %d!",nsize*sizeof(IPACL),PortNo);
					return FALSE;
				}
				memset(&amap->Acls[amap->cnt],0,(amap->size-amap->cnt)*sizeof(IPACL));
				amap->size=nsize;
			}
			IPACL *macl=&amap->Acls[amap->cnt++];
			memcpy(macl,pacl,sizeof(IPACL));
			// Nicely sort in canonical order
			macl->lIp=htonl(macl->lIp);
			macl->lMask=htonl(macl->lMask);
			macl->NextIPAcl=0;
		}
		qsort(amap->Acls,amap->cnt,sizeof(IPACL),CompareIpAcl);
	}
    return(TRUE);
}
#ifdef WS_OTPP
struct CtiReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
#ifdef WIN32
DWORD WINAPI _BootCtiReadThread(LPVOID arg)
{
	CtiReadThreadData *ptd=(CtiReadThreadData*)arg;
	int rc=ptd->aimpl->WSHCtiReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	return rc;
}
#else
void *_BootCtiReadThread(void *arg)
{
	CtiReadThreadData *ptd=(CtiReadThreadData*)arg;
	int rc=ptd->aimpl->WSHCtiReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
}
#endif
#endif
int WsocksHostImpl::WSHOpenCtiPort(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->CtiPort[PortNo].BlockSize*8192;

	int cnt;

    if (!WSHCfgCtiPort(pmod,PortNo))
	{
        pmod->CtiPort[PortNo].InUse=FALSE;
		return(FALSE);
	}
	//pmod->CtiPort[PortNo].Sock = socket(AF_INET, SOCK_DGRAM, 0);
	pmod->CtiPort[PortNo].Sock = WSHSocket(pmod,WS_CTI,PortNo);
	if (pmod->CtiPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->CtiPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket socket() failed with error=%d",errno);
		perror(NULL);
		return(FALSE);
	}
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->CtiPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->CtiPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket ioctlsocket() failed");
		_WSHCloseCtiPort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->CtiPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->CtiPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket setsockopt() failed");
		_WSHCloseCtiPort(pmod,PortNo);
		return(FALSE);
	}

	SndBuf = 5242880;
	if (WSHSetSockOpt(pmod->CtiPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->CtiPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket setsockopt() failed");
		_WSHCloseCtiPort(pmod,PortNo);
		return(FALSE);
	}
	SndBuf = pmod->CtiPort[PortNo].BlockSize*8192;

	//   Retrieve the local IP address and TCP Port number
    
	local_sin.sin_family=AF_INET;
	local_sin.sin_port=htons(pmod->CtiPort[PortNo].Port);

	if (strcmp(pmod->CtiPort[PortNo].LocalIP,"AUTO")==0)
		local_sin.sin_addr.s_addr=INADDR_ANY;
	else
		local_sin.sin_addr.s_addr=inet_addr(pmod->CtiPort[PortNo].LocalIP);

   //  Associate an address with a socket. (bind)
         
	//if (bind( pmod->CtiPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
	if (WSHBindPort( pmod->CtiPort[PortNo].Sock, &local_sin.sin_addr, pmod->CtiPort[PortNo].Port) == SOCKET_ERROR) 
	{
		//pmod->CtiPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket bind() failed");
		_WSHCloseCtiPort(pmod,PortNo);
		return(FALSE);
	}

	strcpy (pmod->CtiPort[PortNo].Note, pmod->CtiPort[PortNo].CfgNote);
	cnt = 0;

	do
	{
		pmod->CtiPort[PortNo].CurrentAltIP++;
		if(pmod->CtiPort[PortNo].CurrentAltIP>=pmod->CtiPort[PortNo].AltIPCount)
			pmod->CtiPort[PortNo].CurrentAltIP=0;
		strcpy(pmod->CtiPort[PortNo].RemoteIP,pmod->CtiPort[PortNo].AltRemoteIP[pmod->CtiPort[PortNo].CurrentAltIP]);
		if (cnt > pmod->CtiPort[PortNo].AltIPCount+1) {
			strcpy(pmod->CtiPort[PortNo].RemoteIP, "");
			break;// none !
		}
		cnt++;

	}while(!pmod->CtiPort[PortNo].AltRemoteIPOn[pmod->CtiPort[PortNo].CurrentAltIP]);

	// UDP broadcast instead of multicast
	if(!strcmp(pmod->CtiPort[PortNo].RemoteIP,"0.0.0.0"))
	{
		int one=1;
		if (WSHSetSockOpt(pmod->CtiPort[PortNo].Sock, SOL_SOCKET, SO_BROADCAST, (char*)&one, sizeof(one)) == SOCKET_ERROR) 
		{
			//pmod->CtiPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket setsockopt(SO_BROADCAST) failed");
			_WSHCloseCtiPort(pmod,PortNo);
			return(FALSE);
		}
	}
	else
	{
		struct tdmreq
		{
		struct in_addr imr_multiaddr;  //The multicast group to join 
		struct in_addr imr_interface;  //The interface to join on
		} mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(pmod->CtiPort[PortNo].RemoteIP);
		mreq.imr_interface.s_addr = local_sin.sin_addr.s_addr;
		if (WSHSetSockOpt(pmod->CtiPort[PortNo].Sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) 
		{
			//pmod->CtiPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket setsockopt(IP_ADD_MEMBERSHIP) failed");
			_WSHCloseCtiPort(pmod,PortNo);
			return(FALSE);
		}
	#ifdef WS_CAST_LOOPBACK
		int mloop=1;
		if (WSHSetSockOpt(pmod->CtiPort[PortNo].Sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&mloop, sizeof(mloop)) == SOCKET_ERROR) 
		{
			//pmod->CtiPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket setsockopt(IP_MULTICAST_LOOP) failed");
			_WSHCloseCtiPort(pmod,PortNo);
			return(FALSE);
		}
	#endif
	}

	WSCloseBuff(&pmod->CtiPort[PortNo].InBuffer);
	WSOpenBuffLimit(&pmod->CtiPort[PortNo].InBuffer,pmod->CtiPort[PortNo].BlockSize,pmod->CtiPort[PortNo].RecvBuffLimit);
	pmod->CtiPort[PortNo].SockActive=TRUE;
#ifdef WIN32
	UuidCreate((UUID*)pmod->CtiPort[PortNo].Uuid);
#endif
	pmod->CtiPort[PortNo].LastDataTime=GetTickCount();
	WSHUpdatePort(pmod,WS_CTI,PortNo);
	pmod->WSCtiOpened(PortNo);

#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->CtiPort[PortNo].Sock;
	::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0);
	for(int o=0;o<WS_OVERLAP_MAX;o++)
	{
		if(WSHCtiIocpBegin(pmod,PortNo)<0)
			return FALSE;
	}
#elif defined(WS_OTPP)
	WSPort *pport=(WSPort*)pmod->CtiPort[PortNo].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
		DWORD tid=0;
		CtiReadThreadData *ptd=new CtiReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
	#ifdef WIN32
		pmod->CtiPort[PortNo].pthread=CreateThread(0,0,_BootCtiReadThread,ptd,0,&tid);
		if(!pmod->CtiPort[PortNo].pthread)
		{
			WSHLogError(pmod,"CTI%d: Failed creating read thread: %d!",PortNo,GetLastError());
			_WSHCloseCtiPort(pmod,PortNo);
			pmod->CtiPort[PortNo].ConnectHold=true;
		}
	#else
		pthread_create(&pmod->CtiPort[PortNo].pthread,0,_BootCtiReadThread,ptd);
	#endif
	#ifdef WS_LOOPBACK
	}
	#endif
#endif
	return(TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHCtiIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	// Issue overlapped recvs
	WSPort *pport=(WSPort*)pmod->CtiPort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->CtiPort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->CtiPort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->CtiPort[PortNo].InBuffer.BlockSize*1024;
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
		FreeOverlap(&pmod->CtiPort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#endif
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseCtiPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CTI_PORTS))
		return -1;
	if(!pmod->CtiPort[PortNo].InUse)
		return -1;
	if(pmod->CtiPort[PortNo].SockActive)
		pmod->CtiPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseCtiPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0;
	if(pmod->CtiPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_CTI,PortNo,false);
		lastRecv=pmod->CtiPort[PortNo].recvThread;
		pmod->CtiPort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->CtiPort[PortNo].pendingClose)
		{
			pmod->CtiPort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_CTI,PortNo,false);
			return 0;
		}
		pmod->CtiPort[PortNo].pendingClose=true;
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->CtiPort[PortNo].pendOvlRecvList; pmod->CtiPort[PortNo].pendOvlRecvList=0;
	if(pendOvlRecvList)
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
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif
	}

	//ResetSendTimeout(WS_CTI,PortNo);
	if(pmod->CtiPort[PortNo].Sock != 0)
		WSHClosePort(pmod->CtiPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->CtiPort[PortNo].Recording,WS_CTI,PortNo);
	WSCloseBuff(&pmod->CtiPort[PortNo].InBuffer);
	if(pmod->CtiPort[PortNo].SockActive)
	{
		pmod->CtiPort[PortNo].SockActive=0;
		pmod->WSCtiClosed(PortNo);
	}
	memset(&pmod->CtiPort[PortNo].SockActive,0
		,sizeof(CTIPORT)-(int)((char *)&pmod->CtiPort[PortNo].SockActive-(char *)&pmod->CtiPort[PortNo]));
	if(pmod->CtiPort[PortNo].rmutex)
	{
		pmod->CtiPort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_CTI,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_CTI,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseCtiPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CTI_PORTS))
		return -1;
	if(!pmod->CtiPort[PortNo].InUse)
		return -1;
	if(pmod->CtiPort[PortNo].SockActive)
		pmod->CtiPort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseCtiPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0;
	if(pmod->CtiPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_CTI,PortNo,false);
		lastRecv=pmod->CtiPort[PortNo].recvThread;
		pmod->CtiPort[PortNo].recvThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_CTI,PortNo);
	if(pmod->CtiPort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->CtiPort[PortNo].Sock)->sd;
		// Send drop multicast membership on port close (though the O/S should do it automatically on socket close)
		if((pmod->CtiPort[PortNo].RemoteIP[0])&&(strcmp(pmod->CtiPort[PortNo].RemoteIP,"0.0.0.0")))
		{
			//   Retrieve the local IP address and TCP Port number
			SOCKADDR_IN local_sin;  // Local socket - internet style    
			local_sin.sin_family=AF_INET;
			local_sin.sin_port=htons(pmod->CtiPort[PortNo].Port);
			if (strcmp(pmod->CtiPort[PortNo].LocalIP,"AUTO")==0)
				local_sin.sin_addr.s_addr=INADDR_ANY;
			else
				local_sin.sin_addr.s_addr=inet_addr(pmod->CtiPort[PortNo].LocalIP);

			struct tdmreq
			{
			struct in_addr imr_multiaddr;  //The multicast group to join 
			struct in_addr imr_interface;  //The interface to join on
			} mreq;
			mreq.imr_multiaddr.s_addr = inet_addr(pmod->CtiPort[PortNo].RemoteIP);
			mreq.imr_interface.s_addr = local_sin.sin_addr.s_addr;
			if (WSHSetSockOpt(pmod->CtiPort[PortNo].Sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) 
				WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CtiSocket setsockopt(IP_DROP_MEMBERSHIP) failed");
		}
		WSHClosePort(pmod->CtiPort[PortNo].Sock);
		WSHFinishOverlapSend(pmod,sd,&pmod->CtiPort[PortNo].pendOvlSendBeg,&pmod->CtiPort[PortNo].pendOvlSendEnd);
	}
	WSHCloseRecording(pmod,&pmod->CtiPort[PortNo].Recording,WS_CTI,PortNo);
	WSCloseBuff(&pmod->CtiPort[PortNo].InBuffer);
	if(pmod->CtiPort[PortNo].SockActive)
	{
		pmod->CtiPort[PortNo].SockActive=0;
		pmod->WSCtiClosed(PortNo);
	}

	#ifdef WIN32
	HANDLE pthread=pmod->CtiPort[PortNo].pthread;
	#else
	pthread_t pthread=pmod->CtiPort[PortNo].pthread;
	#endif
	memset(&pmod->CtiPort[PortNo].SockActive,0
		,sizeof(CTIPORT)-(int)((char *)&pmod->CtiPort[PortNo].SockActive-(char *)&pmod->CtiPort[PortNo]));
	pmod->CtiPort[PortNo].pthread=pthread;
	if(pmod->CtiPort[PortNo].rmutex)
	{
		pmod->CtiPort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_CTI,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_CTI,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitCtiThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->CtiPort[PortNo].pthread)
	{
	#ifdef WIN32
		#ifdef _DEBUG
		WaitForSingleObject(pmod->CtiPort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->CtiPort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->CtiPort[PortNo].pthread); pmod->CtiPort[PortNo].pthread=0;
	#else
		void *rc=0;
		pthread_join(pmod->CtiPort[PortNo].pthread,&rc);
	#endif
	}
}
int WsocksHostImpl::WSHCtiReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->CtiPort[PortNo].Sock;
	int tsize=pmod->CtiPort[PortNo].InBuffer.BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->CtiPort[PortNo].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->CtiPort[PortNo].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->CtiPort[PortNo].SockActive)
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before lockiing the InBuffer and presenting it to the app.
		while((pmod->CtiPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
		{
			SOCKADDR_IN sa;
			int salen=sizeof(SOCKADDR_IN);
			int tbytes=recvfrom(pport->sd,tbuf,tsize,0,(SOCKADDR*)&sa,&salen);
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
			if(!pmod->WSCtiFilter(&sa,PortNo))
				continue;
			#ifdef WS_CTI_PREFIX_PACKET_LEN
			if(!WSWriteBuff(&TBuffer,(char*)&tbytes,sizeof(int)))
			{
				err=WSAENOBUFS; _ASSERT(false);
				break;
			}
			#endif
			if(!WSWriteBuff(&TBuffer,tbuf,tbytes))
			{
				err=WSAENOBUFS; _ASSERT(false);
				break;
			}
		}
		pmod->CtiPort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port
		if((TBuffer.Size>0)&&(pmod->CtiPort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_CTI,PortNo,false);
			int lastRecv=pmod->CtiPort[PortNo].recvThread;
			if(pmod->CtiPort[PortNo].SockActive)
			{
				pmod->CtiPort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->CtiPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->CtiPort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->CtiPort[PortNo].InBuffer.BlockSize*1024;
					WSHCtiRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->CtiPort[PortNo].TBufferSize=TBuffer.Size;
				pmod->CtiPort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_CTI,PortNo,false);
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->CtiPort[PortNo].Sock))
		pmod->CtiPort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#else//!WS_OIO
int WsocksHostImpl::WSHCloseCtiPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->CtiPort[PortNo].rmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_CTI,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->CtiPort[PortNo].recvThread)&&(pmod->CtiPort[PortNo].recvThread!=tid))
		{
			UnlockPort(pmod,WS_CTI,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_CTI,PortNo,false);
		}
		pmod->CtiPort[PortNo].recvThread=tid;
	}
	//ResetSendTimeout(WS_CTI,PortNo);
	if(pmod->CtiPort[PortNo].Sock != 0)
		WSHClosePort(pmod->CtiPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->CtiPort[PortNo].Recording,WS_CTI,PortNo);
	WSCloseBuff(&pmod->CtiPort[PortNo].InBuffer);
	pmod->WSCtiClosed(PortNo);
	memset(&pmod->CtiPort[PortNo].SockActive,0
		,sizeof(CTIPORT)-(int)((char *)&pmod->CtiPort[PortNo].SockActive-(char *)&pmod->CtiPort[PortNo]));
	if(pmod->CtiPort[PortNo].rmutex)
	{
		pmod->CtiPort[PortNo].recvThread=0;
		UnlockPort(pmod,WS_CTI,PortNo,false);
	}
	//WSHPortChanged(pmod,pmod->CtiPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO
#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHCtiRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	pmod->CtiPort[i].BytesIn+=bytes;
	pmod->CtiPort[i].BlocksIn++;
	pmod->CtiPort[i].LastDataTime=GetTickCount();

	// If we add a MSGHEADER to the recording, we could playback the CTI cast
	WSHRecord(pmod,&pmod->CtiPort[i].Recording,buf,bytes,false);
	//#ifdef WS_CTI_PREFIX_PACKET_LEN
	////if (pmod->CtiPort[i].PrefixPacketLen)
	//{
	//	if(!WSWriteBuff(&pmod->CtiPort[i].InBuffer,(char*)&bytes,sizeof(int)))
	//	{
	//		WSHCloseCtiPort(pmod,i);
	//		return(FALSE);
	//	}
	//}
	//#endif
	if(!WSWriteBuff(&pmod->CtiPort[i].InBuffer,(char*)buf,bytes))
	{
		WSHCloseCtiPort(pmod,i);
		return(FALSE);
	}
	return(TRUE);
}
#else//!WS_OIO
int WsocksHostImpl::WSHCtiRead(WsocksApp *pmod, int PortNo)
{
	//static char szTemp[(WS_MAX_BLOCK_SIZE*1024)];
	char *szTemp=pmod->CtiRead_szTemp;
	int status;             // Status Code 
	int i=PortNo;
	struct sockaddr_in SourceAddr;
	int SourceAddrSize = sizeof(SourceAddr);
	
	if((pmod->CtiPort[i].InBuffer.Busy)&&(pmod->CtiPort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: CTI%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->CtiPort[i].InBuffer.Busy);
		pmod->CtiPort[i].InBuffer.Busy=0;
	}
	pmod->CtiPort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common CtiRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
	status = WSHRecvFrom(pmod->CtiPort[i].Sock, szTemp, ((pmod->CtiPort[i].BlockSize-1)*1024), NO_FLAGS_SET
		,(struct sockaddr *)&SourceAddr,&SourceAddrSize);
	if (status==SOCKET_ERROR)
	{
		int Err=WSAGetLastError();
		WSHReleaseMutex(0x01);
        WSHLogError(pmod,"CTIREAD FAILED on Port %d with Error %d -- Port Bounced",PortNo,Err);
		WSHCloseCtiPort(pmod,i);
		return FALSE;
	}

	if (status>0) 
	{
		// If we add a MSGHEADER to the recording, we could playback the CTI cast
		WSHRecord(pmod,&pmod->CtiPort[i].Recording,szTemp,status,false);
		pmod->CtiPort[i].LastDataTime=GetTickCount();
		if(pmod->WSCtiFilter(&SourceAddr,PortNo))
		{
			pmod->CtiPort[i].BytesIn+=status;
			pmod->CtiPort[i].BlocksIn++;
		#ifdef WS_CTI_PREFIX_PACKET_LEN
			//if (pmod->CtiPort[i].PrefixPacketLen)
			{
				if(!WSWriteBuff(&pmod->CtiPort[i].InBuffer,(char*)&status,sizeof(int)))
				{
					WSHReleaseMutex(0x01);
					WSHCloseCtiPort(pmod,i);
					return(FALSE);
				}
			}
		#endif
			if(!WSWriteBuff(&pmod->CtiPort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseCtiPort(pmod,i);
				return(FALSE);
			}
		}
	}
	else
	{
		WSHReleaseMutex(0x01);
		WSHCloseCtiPort(pmod,i);
		return(FALSE);
	}
	WSHReleaseMutex(0x01);
	pmod->CtiPort[i].InBuffer.Busy=lastBusy;
	return(TRUE);
}
#endif//!WS_OIO

#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHSendToNonBlock(WsocksApp *pmod, int PortType, int PortNo, SOCKET_T socket, LPVOID buffer, DWORD bytesToWrite, SOCKADDR *taddr, int tlen)
{
	switch(PortType)
	{
	case WS_CTO:
	{
		WSPort *pport=(WSPort *)socket;
	#ifdef WIN32
		if(pmod->CtoPort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		WSOVERLAPPED *povl=AllocOverlap(&pmod->CtoPort[PortNo].pendOvlSendBeg,&pmod->CtoPort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->CtoPort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=true;
		povl->Cancelled=false;
		povl->RecvOp=0;
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASendTo(pport->sd,&povl->wsabuf,1,&povl->bytes,0,taddr,tlen,(LPWSAOVERLAPPED)povl,0);
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			return bytesToWrite;
		}
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->CtoPort[PortNo].pendOvlSendBeg,&pmod->CtoPort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		int tsent=sendto(pport->sd,(char *)buffer,bytesToWrite,0,taddr,tlen);
		if(tsent<1)
			return SOCKET_ERROR;
		return tsent;
	#endif
	}
	default:
		WSASetLastError(WSAESOCKTNOSUPPORT);
		return SOCKET_ERROR;
	}
}
#endif//WS_OIO

// WSHCtoSendBuff is always immediate
int WsocksHostImpl::WSHCtoSend(WsocksApp *pmod, int PortNo)
{
	int nsent=0;
	if(!pmod->CtoPort[PortNo].SockActive)
		return -1;

	int lastBusy=pmod->CtoPort[PortNo].OutBuffer.Busy;
	pmod->CtoPort[PortNo].OutBuffer.Busy=pmod->CtoPort[PortNo].sendThread;
	pmod->CtoPort[PortNo].SendTimeOut=0;

	// Send as much as possible or up to the choke limit
	BUFFER *sendBuffer=&pmod->CtoPort[PortNo].OutBuffer;
	while(sendBuffer->Size>0)
	{
		int size=sendBuffer->LocalSize;
		if(size>pmod->CtoPort[PortNo].BlockSize*1024)
			size=pmod->CtoPort[PortNo].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->CtoPort[PortNo].Sock;
		SOCKADDR_IN cast_sin;
		cast_sin.sin_family = AF_INET;
		cast_sin.sin_port=htons(pmod->CtoPort[PortNo].Port);
		cast_sin.sin_addr.s_addr=inet_addr(pmod->CtoPort[PortNo].RemoteIP);
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=WSHSendToNonBlock(pmod,WS_CTO,PortNo,(SOCKET_T)pport,sendBuffer->Block,size,(struct sockaddr *)&cast_sin,sizeof(sockaddr_in));
		#else
		int rc=WSHSendToPort(pport,sendBuffer->Block,size,0,(struct sockaddr *)&cast_sin,sizeof(sockaddr_in));
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_CTO_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->CtoPort[PortNo].SendTimeOut)
				{
					pmod->CtoPort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->CtoPort[PortNo].SendTimeOut<WS_CTO_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: CTO%d [%s] Reset by Peer",PortNo,pmod->CtoPort[PortNo].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: CTO%d [%s] Send Failed: %d",PortNo,pmod->CtoPort[PortNo].CfgNote,lerr);
				WSHCloseCtoPort(pmod,PortNo);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->CtoPort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		//WSHRecord(pmod,&pmod->CtoPort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->CtoPort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->CtoPort[PortNo].BytesOut+=rc;
		pmod->CtoPort[PortNo].BlocksOut++;
		pmod->CtoPort[PortNo].SendTimeOut=0;
		nsent+=rc;
	}
	pmod->CtoPort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}
