
#include "stdafx.h"
#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#ifdef WIN32
#include <shlwapi.h>
#include <windowsx.h>
#include <Iphlpapi.h>
#endif

int WsocksHostImpl::WSHSetupUmcPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	return 0;
}
int WsocksHostImpl::WSHOpenUmcPort(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->UmcPort[PortNo].BlockSize*8192;
	
	struct
	{
		int l_onoff;
		int l_linger;
	} linger;
	
	//pmod->UmcPort[PortNo].Sock = socket(AF_INET, SOCK_STREAM, 0);
	pmod->UmcPort[PortNo].Sock = WSHSocket(pmod,WS_UMC,PortNo);
	if (pmod->UmcPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->UmcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket socket() failed");
		return(FALSE);
	}
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->UmcPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->UmcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket ioctlsocket() failed");
		WSHCloseUmcPort(pmod,PortNo);
		return(FALSE);
	}

	linger.l_onoff=1;
	linger.l_linger=0;
	if (WSHSetSockOpt(pmod->UmcPort[PortNo].Sock, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof(linger)) == SOCKET_ERROR) 
	{
		pmod->UmcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket setsockopt() failed");
		WSHCloseUmcPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->UmcPort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->UmcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket setsockopt() failed");
		WSHCloseUmcPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->UmcPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->UmcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket setsockopt() failed");
		WSHCloseUmcPort(pmod,PortNo);
		return(FALSE);
	}

	local_sin.sin_family=AF_INET;
	local_sin.sin_port=htons(pmod->UmcPort[PortNo].Port);

	if (strcmp(pmod->UmcPort[PortNo].LocalIP,"AUTO")==0)
		local_sin.sin_addr.s_addr=INADDR_ANY;
	else
		local_sin.sin_addr.s_addr=inet_addr(pmod->UmcPort[PortNo].LocalIP);

	// Dynamic bind on port range
	if((!pmod->UmcPort[PortNo].Port)&&(pmod->UmcPort[PortNo].dynPortBegin>0)&&(pmod->UmcPort[PortNo].dynPortEnd>=pmod->UmcPort[PortNo].dynPortBegin))
	{
		int wsfalse = 0;
		if (WSHSetSockOpt(pmod->UmcPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wsfalse), sizeof(int)) == SOCKET_ERROR) 
		{
			pmod->UmcPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket setsockopt(SO_REUSEADDR=0) failed");
			WSHCloseUmcPort(pmod,PortNo);
			return(FALSE);
		}
		while((!pmod->UmcPort[PortNo].Port)&&(pmod->UmcPort[PortNo].dynPortBegin<pmod->UmcPort[PortNo].dynPortEnd))
		{
			if (WSHBindPort( pmod->UmcPort[PortNo].Sock, &local_sin.sin_addr, pmod->UmcPort[PortNo].dynPortBegin) != SOCKET_ERROR) 
				pmod->UmcPort[PortNo].Port=pmod->UmcPort[PortNo].dynPortBegin;
			else
			{
				int lerr=WSAGetLastError();
				if((lerr==WSAEADDRINUSE)||
				   (lerr==WSAEACCES))//Windows Server 2008 err code
					pmod->UmcPort[PortNo].dynPortBegin++;
				else
				{
					pmod->UmcPort[PortNo].dynPortBegin=pmod->UmcPort[PortNo].dynPortEnd;
					break;
				}
			}
		}
		if(pmod->UmcPort[PortNo].dynPortBegin>=pmod->UmcPort[PortNo].dynPortEnd)
		{
			pmod->UmcPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket dynamic bind(%d) failed", PortNo);
			WSHCloseUmcPort(pmod,PortNo);
			return(FALSE);
		}
		// Special Multi-NIC bind port synchronization for lbmonitor
		if((pmod->pcfg->aname=="LBSysmon")&&
		   (_stricmp(pmod->UmcPort[PortNo].LocalIP,"AUTO"))&&
		   (_stricmp(pmod->UmcPort[PortNo].LocalIP,"127.0.0.1")))
		{
			for(int i=0;i<pmod->NO_OF_UMC_PORTS;i++)
			{
				if((i==PortNo)||(!pmod->UmcPort[i].InUse))
					continue;
				if((!_stricmp(pmod->UmcPort[i].LocalIP,"AUTO"))||
				   (!_stricmp(pmod->UmcPort[i].LocalIP,"0.0.0.0"))||
				   (!_stricmp(pmod->UmcPort[i].LocalIP,"127.0.0.1")))
				   continue;
				if(pmod->UmcPort[i].Port!=pmod->UmcPort[PortNo].Port)
				{
					if(pmod->UmcPort[i].SockActive)
						WSHCloseUmcPort(pmod,i);
					pmod->UmcPort[i].Port=pmod->UmcPort[i].dynPortBegin=pmod->UmcPort[PortNo].Port;
					WSHLogEvent(pmod,"UMC%d: Bind sync UMC%d to port %d",PortNo,i,pmod->UmcPort[i].dynPortBegin);
				}
			}
		}
	}
   //  Associate an address with a socket. (bind)
	else
	{
		int wstrue = 1;
		if (WSHSetSockOpt(pmod->UmcPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
		{
			pmod->UmcPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket setsockopt(SO_REUSEADDR=1) failed");
			WSHCloseUmcPort(pmod,PortNo);
			return(FALSE);
		}
	         
		//if (bind( pmod->UmcPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
		if (WSHBindPort( pmod->UmcPort[PortNo].Sock, &local_sin.sin_addr, pmod->UmcPort[PortNo].Port) == SOCKET_ERROR) 
		{
			// Special Multi-NIC bind port synchronization for lbmonitor
			if(((WSAGetLastError()==WSAEADDRINUSE)||
				(WSAGetLastError()==WSAEACCES))&&//Windows Server 2008 err code
			   (pmod->pcfg->aname=="LBSysmon")&&
			   (_stricmp(pmod->UmcPort[PortNo].LocalIP,"AUTO"))&&
			   (_stricmp(pmod->UmcPort[PortNo].LocalIP,"127.0.0.1")))
			{
				pmod->UmcPort[PortNo].dynPortBegin++;
				WSHCloseUmcPort(pmod,PortNo);
				// Close all synced ports and try next port in range
				for(int i=0;i<pmod->NO_OF_UMC_PORTS;i++)
				{
					if((i==PortNo)||(!pmod->UmcPort[i].InUse))
						continue;
					if((!_stricmp(pmod->UmcPort[i].LocalIP,"AUTO"))||
					   (!_stricmp(pmod->UmcPort[i].LocalIP,"0.0.0.0"))||
					   (!_stricmp(pmod->UmcPort[i].LocalIP,"127.0.0.1")))
					   continue;
					if(pmod->UmcPort[i].SockActive)
						WSHCloseUmcPort(pmod,i);
					WSHLogEvent(pmod,"UMC%d: Bind sync %d failed, reset UMC%d begin port %d",PortNo,pmod->UmcPort[i].Port,i,pmod->UmcPort[PortNo].dynPortBegin);
					pmod->UmcPort[i].Port=0;
					pmod->UmcPort[i].dynPortBegin=pmod->UmcPort[PortNo].dynPortBegin;
				}
				return(FALSE);
			}

			pmod->UmcPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UmcSocket(%d) bind(%s:%d) failed",PortNo,pmod->UmcPort[PortNo].LocalIP,pmod->UmcPort[PortNo].Port);
			WSHCloseUmcPort(pmod,PortNo);
			return(FALSE);
		}
	}

	// Retrieve bind port
	SOCKADDR_IN laddr;
	int lalen=sizeof(SOCKADDR_IN);
	WSHGetSockName(pmod->UmcPort[PortNo].Sock,(SOCKADDR*)&laddr,&lalen);
	pmod->UmcPort[PortNo].bindPort=ntohs(laddr.sin_port);
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
			strncpy(pmod->UmcPort[PortNo].bindIP,pnic->IpAddressList.IpAddress.String,20);
			pmod->UmcPort[PortNo].bindIP[19]=0;
			if(pmod->UmcPort[PortNo].bindIP[0])
				break;
		}
		delete pAdapterInfo;
	#else
		strncpy(pmod->UmcPort[PortNo].bindIP,inet_ntoa(local_sin.sin_addr),20);
	#endif
	}
	else
		strncpy(pmod->UmcPort[PortNo].bindIP,inet_ntoa(local_sin.sin_addr),20);

   if (WSHListen( pmod->UmcPort[PortNo].Sock, MAX_PENDING_CONNECTS ) == SOCKET_ERROR) 
	{
		pmod->UmcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UMC%d listen(%d) failed",PortNo,MAX_PENDING_CONNECTS);
		WSHCloseUmcPort(pmod,PortNo);
		return(FALSE);
	}

	pmod->UmcPort[PortNo].SockActive=TRUE;
	WSHUpdatePort(pmod,WS_UMC,PortNo);
	return(TRUE);
}
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseUmcPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UmcPort[PortNo].InUse)
		return -1;
	if(pmod->UmcPort[PortNo].SockActive)
		pmod->UmcPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUmcPort(WsocksApp *pmod, int PortNo)
{
	WSHClosePort(pmod->UmcPort[PortNo].Sock);
	// Don't zero out UMC detail pointer values
	UMCPORT tmport;
	memcpy(&tmport.DetPtr,&pmod->UmcPort[PortNo].DetPtr,11*sizeof(void*));
	memset(&pmod->UmcPort[PortNo].SockActive,0
		,sizeof(UMCPORT)-(int)((char *)&pmod->UmcPort[PortNo].SockActive-(char *)&pmod->UmcPort[PortNo]));
	memcpy(&pmod->UmcPort[PortNo].DetPtr,&tmport.DetPtr,11*sizeof(void*));
	WSHUpdatePort(pmod,WS_UMC,PortNo);
	return (TRUE);
}

int WsocksHostImpl::WSHSetupUmrPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=0;i<=pmod->NO_OF_UMC_PORTS;i++)
		{
			if((PortNo>=0)&&(i!=PortNo))
				continue;
			memset(&pmod->UmcPort[i],0,sizeof(UMCPORT));
			pmod->UmcPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
			pmod->UmcPort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
			pmod->UmcPort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		}
		for (i=0;i<=pmod->NO_OF_UMR_PORTS;i++)
		{
			if((PortNo>=0)&&(i!=PortNo))
				continue;
			memset(&pmod->UmrPort[i],0,sizeof(UMRPORT));
		}
		break;
	case WS_OPEN: // Open
		for (i=0;i<pmod->NO_OF_UMC_PORTS;i++)
		{
			if((PortNo>=0)&&(i!=PortNo))
				continue;
			if(strlen(pmod->UmcPort[i].LocalIP)>0)
			{
				//AddConListItem(GetDispItem(WS_UMC,i));
			#ifdef WS_COMPRESS
				if((pmod->UmcPort[i].InUse)&&(!pmod->UmcPort[i].Compressed))
				{
					WSHLogError(pmod,"UMC%d not configured for compression. Setting to compressed.",i);
					pmod->UmcPort[i].Compressed=TRUE;
				}
			#else
				WSHLogError(pmod,"UMC%d configured, but app not built with compression. Disabling port!",i);
				pmod->UmcPort[i].InUse=FALSE;
			#endif
				pmod->UmcPort[i].tmutex=CreateMutex(0,false,0);
				pmod->UmcPort[i].activeThread=0;
				pmod->UmcPort[i].InUse=TRUE;
			}
		}
		break;
	case WS_CLOSE: // close
		for (i=0;i<=pmod->NO_OF_UMC_PORTS;i++)
		{
			if((PortNo>=0)&&(i!=PortNo))
				continue;
			if(pmod->UmcPort[i].SockActive)
				WSHCloseUmcPort(pmod,i);
			if(pmod->UmcPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_UMC,i));
				pmod->UmcPort[i].InUse=FALSE;
			}
			pmod->UmcPort[i].activeThread=0;
			if(pmod->UmcPort[i].tmutex)
			{
				//DeleteMutex(pmod->UmcPort[i].tmutex); pmod->UmcPort[i].tmutex=0;
				DeletePortMutex(pmod,WS_UMC,i,false);
			}
		}
		for (i=0;i<=pmod->NO_OF_UMR_PORTS;i++)
		{
			if((PortNo>=0)&&(i!=PortNo))
				continue;
			if(pmod->UmrPort[i].SockActive)
			{
				_WSHCloseUmrPort(pmod,i);
				#ifdef WS_OTPP
				_WSHWaitUmrThreadExit(pmod,i);
				#endif
			}
		}
		#ifdef WS_OTMP
		WSHWaitUmrThreadPoolsExit();
		#endif
		break;
	}
	return(TRUE);
}
#ifdef WS_OTPP
struct UmrReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
#ifdef WIN32
DWORD WINAPI _BootUmrReadThread(LPVOID arg)
{
	UmrReadThreadData *ptd=(UmrReadThreadData*)arg;
	int rc=ptd->aimpl->WSHUmrReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	return rc;
}
#ifdef WS_OTMP
struct UmrReadPoolThreadData
{
	WsocksHostImpl *aimpl;
	WsocksApp *pmod;
	THREADPOOL *tpool;
};
DWORD WINAPI _BootUmrReadPoolThread(LPVOID arg)
{
	UmrReadPoolThreadData *ptd=(UmrReadPoolThreadData*)arg;
	WsocksHostImpl *aimpl=ptd->aimpl;
	WsocksApp *pmod=ptd->pmod;
	THREADPOOL *tpool=ptd->tpool;
	delete ptd;
	int rc=aimpl->WSHUmrReadPoolThread(pmod,tpool);
	return rc;
}
HANDLE WsocksHostImpl::WSHPoolUmrReadThread(WsocksApp *pmod, WSPort *pport)
{
	THREADPOOL *tpool=umrPools;
	if((!tpool)||(tpool->nports>=WS_NTMP))
	{
		tpool=new THREADPOOL;
		memset(tpool,0,sizeof(THREADPOOL));
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
		tpool->next=umrPools; umrPools=tpool;
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif

		DWORD tid=0;
		UmrReadPoolThreadData *ptd=new UmrReadPoolThreadData;
		ptd->aimpl=this;
		ptd->pmod=pmod;
		ptd->tpool=tpool;
		tpool->thnd=CreateThread(0,0,_BootUmrReadPoolThread,ptd,0,&tid);
		if(!tpool->thnd)
		{
			delete ptd;
			return 0;
		}
	}
	tpool->ports[tpool->nports++]=pport;
	return tpool->thnd;
}
void WsocksHostImpl::WSHWaitUmrThreadPoolsExit()
{
	while(umrPools)
	{
		THREADPOOL *tpool=umrPools;
		tpool->shutdown=true;
		WaitForSingleObject(tpool->thnd,INFINITE);
		CloseHandle(tpool->thnd); tpool->thnd=0;
		umrPools=umrPools->next;
		delete tpool;
	}
}
#endif//WS_OTMP
#else//!WIN32
void *_BootUmrReadThread(void *arg)
{
	UmrReadThreadData *ptd=(UmrReadThreadData*)arg;
	int rc=ptd->aimpl->WSHUmrReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
}
#endif//!WIN32
#endif
int WsocksHostImpl::WSHUmrAccept(WsocksApp *pmod, int PortNo)
{
	SOCKET_T TSock;			// temp socket to hold connection while we
							// determine what port it was for 
	SOCKADDR_IN AccSin;    // Accept socket address - internet style 
	int AccSinLen;        // Accept socket address length 
	int i;
	int j=PortNo;

	ws_fd_set rds;
	WS_FD_ZERO(&rds);
	WS_FD_SET(pmod->UmcPort[j].Sock,&rds);
	struct timeval TimeVal={0,0};
	int nrds=WSHSelect((PTRCAST)pmod->UmcPort[j].Sock+1,&rds,NULL,NULL,(timeval*)&TimeVal);
	if(nrds<1)
		return FALSE;
	// determine which user channel is available
	for (i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	{
		if((!pmod->UmrPort[i].SockActive)&&(pmod->UmrPort[i].TimeTillClose<=0)
			#ifdef WS_OIO
			&&(!pmod->UmrPort[i].pendingClose)
			#endif
			&&(!pmod->UmrPort[i].rmutcnt)&&(!pmod->UmrPort[i].smutcnt)
			)
			break;
	}
	if (i>=pmod->NO_OF_UMR_PORTS) // no more ports avaliable
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : No more user ports avaliable");
		// Accept the incoming connection with port -1 to drop it
		AccSinLen = sizeof( AccSin );
		TSock = WSHAccept(pmod,pmod->UscPort[j].Sock,WS_UMR,-1,(struct sockaddr *) &AccSin,
			(int *) &AccSinLen );
		return(FALSE);
	}
	pmod->UmrPort[i].UmcPort=j;

    AccSinLen = sizeof( AccSin );
    //TSock = accept(pmod->UmcPort[j].Sock,(struct sockaddr *) &AccSin,
    TSock = WSHAccept(pmod,pmod->UmcPort[j].Sock,WS_UMR,i,(struct sockaddr *) &AccSin,
        (int *) &AccSinLen );

	if (TSock == INVALID_SOCKET_T)
		return(FALSE);

#if !defined(WIN32)&&!defined(_CONSOLE)
	// On Windows, the accepted socket inherits async behavior from listen socket,
	// but on Linux, it doesn't inherit fcntl attributes
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(TSock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d WSUsrAccept  USR%d ioctlsocket(!FIONBIO) failed",j,i);
		WSHClosePort(TSock);
		return(FALSE);
	}
#endif

	//// determine which user channel is available
	//for (i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	//{
	//	if((!pmod->UmrPort[i].SockActive)&&(pmod->UmrPort[i].TimeTillClose<=0))
	//		break;
	//}
	//if (i>=pmod->NO_OF_UMR_PORTS) // no more ports avaliable
	//{
	//	WSHLogError(pmod,"!WSOCKS: FATAL ERROR : No more user ports avaliable");
 //       WSHClosePort( TSock );
	//	return(FALSE);
	//}
#ifdef WS_REALTIMESEND
	pmod->UmrPort[i].ImmediateSendLimit=pmod->UmcPort[pmod->UmrPort[i].UmcPort].ImmediateSendLimit;
#endif

    // DONT MOVE THIS LINE OR THE BUFFER SIZE IN THE NEXT LINE WILL FAIL
	//pmod->UmrPort[i].UmcPort=j;

	WSOpenBuffLimit(&pmod->UmrPort[i].InBuffer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize,pmod->UmcPort[pmod->UmrPort[i].UmcPort].RecvBuffLimit);
	WSOpenBuffLimit(&pmod->UmrPort[i].OutBuffer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize,pmod->UmcPort[pmod->UmrPort[i].UmcPort].SendBuffLimit);
#ifdef WS_COMPRESS
	WSOpenBuffLimit(&pmod->UmrPort[i].InCompBuffer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize,pmod->UmcPort[pmod->UmrPort[i].UmcPort].RecvBuffLimit);
	WSOpenBuffLimit(&pmod->UmrPort[i].OutCompBuffer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize,pmod->UmcPort[pmod->UmrPort[i].UmcPort].SendBuffLimit);
#endif
#ifdef WS_ENCRYPTED
	if(pmod->UmcPort[PortNo].Encrypted)
	{
		pmod->UmrPort[i].EncryptionOn=1;
		// Default encryption keys
		*((__int64*)(&pmod->UmrPort[i].pw[0]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UmrPort[i].pw[8]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UmrPort[i].pw[16]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UmrPort[i].Iiv[0]))=0x0123456780ABCDEF;
		*((__int64*)(&pmod->UmrPort[i].Oiv[0]))=0x0123456780ABCDEF;
		WSHResetCryptKeys(pmod,i,WS_UMR);
	}
#endif
	pmod->UmrPort[i].rmutcnt=0;
	pmod->UmrPort[i].rmutex=CreateMutex(0,false,0);
	pmod->UmrPort[i].smutcnt=0;
	pmod->UmrPort[i].smutex=CreateMutex(0,false,0);
	pmod->UmrPort[i].recvThread=0;
	pmod->UmrPort[i].sendThread=0;
	pmod->UmrPort[i].Sock=TSock;
	strcpy(pmod->UmrPort[i].Note,pmod->UmcPort[j].CfgNote);
	pmod->UmrPort[i].SockActive=TRUE;
	LockPort(pmod,WS_UMR,i,false);
	LockPort(pmod,WS_UMR,i,true);
	#ifdef WIN32
	sprintf(pmod->UmrPort[i].RemoteIP,"%d.%d.%d.%d"
		,AccSin.sin_addr.S_un.S_un_b.s_b1
		,AccSin.sin_addr.S_un.S_un_b.s_b2
		,AccSin.sin_addr.S_un.S_un_b.s_b3
		,AccSin.sin_addr.S_un.S_un_b.s_b4);
	#else
	strcpy(pmod->UmrPort[i].RemoteIP,inet_ntoa(AccSin.sin_addr));
	#endif
	//AddConListItem(GetDispItem(WS_UMR,i));
#ifdef WIN32
	UuidCreate((UUID*)pmod->UmrPort[i].Uuid);
#endif
#ifdef WS_LOOPBACK
	// Detect loopback connections and remember the corresponding port
	if(!strcmp(pmod->UmrPort[i].RemoteIP,"127.0.0.1"))
	{
		for(int c=0;c<pmod->NO_OF_CON_PORTS;c++)
		{
			if((pmod->ConPort[c].SockConnected)&&
			   (pmod->ConPort[c].Port==pmod->UmcPort[pmod->UmrPort[i].UmcPort].Port)&&
			   (!strcmp(pmod->ConPort[c].RemoteIP,"127.0.0.1")))
			{
				SOCKADDR_IN paddr,laddr;
				int palen=sizeof(SOCKADDR_IN),lalen=sizeof(SOCKADDR_IN);
				WSHGetPeerName(pmod->ConPort[c].Sock,(SOCKADDR*)&paddr,&palen);
				WSHGetSockName(pmod->UmrPort[i].Sock,(SOCKADDR*)&laddr,&lalen);
				if((paddr.sin_addr.s_addr==laddr.sin_addr.s_addr)&&
				   (paddr.sin_port==laddr.sin_port))
				{
					if((pmod->UmcPort[pmod->UmrPort[i].UmcPort].Compressed)||(pmod->ConPort[c].Compressed))
						WSHLogError(pmod,"Loopback connection from USR%d to CON%d detected, but cannot be optimized due to compression.",PortNo,c);
					else
					{
						pmod->UmrPort[i].DetPtr=(void*)(PTRCAST)MAKELONG(c,127);
						pmod->ConPort[c].DetPtr=(void*)(PTRCAST)MAKELONG(PortNo,127);
						WSHLogEvent(pmod,"Loopback connection from USR%d to CON%d detected.",PortNo,c);
					}
				}
			}
		}
	}
#endif
	if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].Recording.DoRec)
		WSHOpenRecording(pmod,&pmod->UmrPort[i].Recording,0,WS_UMR,i);
	pmod->WSUmrOpened(i);
	UnlockPort(pmod,WS_UMR,i,true);
	UnlockPort(pmod,WS_UMR,i,false);
	WSHUpdatePort(pmod,WS_UMR,i);

#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->UmrPort[i].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
	::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0);
	for(int o=0;o<WS_OVERLAP_MAX;o++)
	{
		if(WSHUmrIocpBegin(pmod,i)<0)
			return FALSE;
	}
	#ifdef WS_LOOPBACK
	}
	#endif
#elif defined(WS_OTPP)
	WSPort *pport=(WSPort*)pmod->UmrPort[i].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
		DWORD tid=0;
		UmrReadThreadData *ptd=new UmrReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
	#ifdef WIN32
		#ifdef WS_OTMP
		if(pmod->pcfg->umrOtmp)
		{
			pmod->UmrPort[i].pthread=WSHPoolUmrReadThread(pmod,pport);
			if(!pmod->UmrPort[i].pthread)
			{
				WSHLogError(pmod,"UMR%d: Failed assigning read thread: %d!",i,GetLastError());
				_WSHCloseUmrPort(pmod,i);
			}
			return TRUE;
		}
		#endif
		pmod->UmrPort[i].pthread=CreateThread(0,0,_BootUmrReadThread,ptd,0,&tid);
		if(!pmod->UmrPort[i].pthread)
		{
			WSHLogError(pmod,"UMR%d: Failed creating read thread: %d!",i,GetLastError());
			_WSHCloseUmrPort(pmod,i);
		}
	#else
		pthread_create(&pmod->UmrPort[i].pthread,0,_BootUmrReadThread,ptd);
	#endif
	#ifdef WS_LOOPBACK
	}
	#endif
#endif
	return(TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHUmrIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	// Issue overlapped recvs
	WSPort *pport=(WSPort*)pmod->UmrPort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->UmrPort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->UmrPort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->UmrPort[PortNo].InBuffer.BlockSize*1024;
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
		FreeOverlap(&pmod->UmrPort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHUmrReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->UmrPort[PortNo].Sock;
	int tsize=pmod->UmrPort[PortNo].InBuffer.BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->UmrPort[PortNo].SockActive)
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before lockiing the InBuffer and presenting it to the app.
		while((pmod->UmrPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
		{
			int tbytes=recv(pport->sd,tbuf,tsize,0);
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
		pmod->UmrPort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port
		if((TBuffer.Size>0)&&(pmod->UmrPort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_UMR,PortNo,false);
			int lastRecv=pmod->UmrPort[PortNo].recvThread;
			if(pmod->UmrPort[PortNo].SockActive)
			{
				pmod->UmrPort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->UmrPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->UmrPort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->UmrPort[PortNo].InBuffer.BlockSize*1024;
					WSHUmrRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->UmrPort[PortNo].TBufferSize=TBuffer.Size;
				pmod->UmrPort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_UMR,PortNo,false);
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->UmrPort[PortNo].Sock))
		pmod->UmrPort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#ifdef WS_OTMP
int WsocksHostImpl::WSHUmrReadPoolThread(WsocksApp *pmod, THREADPOOL *tpool)
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
			if(!pmod->UmrPort[PortNo].Sock)
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
				tsize=pmod->UmrPort[PortNo].InBuffer.BlockSize*1024;
				tbuf=new char[tsize];
				RecvBuffLimit=(DWORD)pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].RecvBuffLimit*1024;
				if(!RecvBuffLimit)
					RecvBuffLimit=ULONG_MAX;
				WSOpenBuffLimit(&TBuffer,pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].BlockSize,RecvBuffLimit/1024);
			}
			// Port loop
			int err=0;
			while(pmod->UmrPort[PortNo].Sock)// Account for TimeTillClose
			{
				// This implementation increases throughput by reading from the socket as long as data is 
				// available (up to some limit) before locking the InBuffer and presenting it to the app.
				while((pmod->UmrPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
				{
					int tbytes=recv(pport->sd,tbuf,tsize,0);
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
				pmod->UmrPort[PortNo].TBufferSize=TBuffer.Size;
				// Save what we've pulled off the port
				if((TBuffer.Size>0)&&(pmod->UmrPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					LockPort(pmod,WS_UMR,PortNo,false);
					if(pmod->UmrPort[PortNo].Sock)
					{
						int lastRecv=pmod->UmrPort[PortNo].recvThread;
						pmod->UmrPort[PortNo].recvThread=tid;
						while((TBuffer.Size>0)&&(pmod->UmrPort[PortNo].InBuffer.Size<RecvBuffLimit))
						{
							DWORD rsize=TBuffer.LocalSize;
							if(rsize>pmod->UmrPort[PortNo].InBuffer.BlockSize*1024)
								rsize=pmod->UmrPort[PortNo].InBuffer.BlockSize*1024;
							WSHUmrRead(pmod,PortNo,TBuffer.Block,rsize);
							if(!WSStripBuff(&TBuffer,rsize))
							{
								err=WSAENOBUFS; _ASSERT(false);
								break;
							}
						}
						pmod->UmrPort[PortNo].TBufferSize=TBuffer.Size;
						pmod->UmrPort[PortNo].recvThread=lastRecv;
					}
					else
						err=WSAECONNABORTED;
					UnlockPort(pmod,WS_UMR,PortNo,false);
				}
				// This loop is kept a 'while' instead of 'if' to preserve 
				// 'break' statements above, but it really functions like an 'if'
				// so we can handle the next port
				break; 
			}
			// One-time cleanup
			if((err)&&(err!=WSAEWOULDBLOCK))
			{
				if((err!=WSAECONNABORTED)&&(pmod->UmrPort[PortNo].Sock))
					pmod->UmrPort[PortNo].peerClosed=true;
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
int WsocksHostImpl::WSHUmrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	pmod->UmrPort[i].BytesIn+=bytes;
	pmod->UmrPort[i].BlocksIn++;
#ifdef WS_LOOPBACK
	WSPort *pport=(WSPort*)pmod->UmrPort[PortNo].Sock;
	if((pport)&&(pport->lbaPeer))
	{
		while(pport->lbaBuffer->Size>0)
		{
			unsigned int size=pport->lbaBuffer->LocalSize;
			if(size>pmod->UmrPort[i].InBuffer.BlockSize*1024)
				size=pmod->UmrPort[i].InBuffer.BlockSize*1024;
			if(!WSWriteBuff(&pmod->UmrPort[i].InBuffer,pport->lbaBuffer->Block,size))
				return(FALSE);
			WSStripBuff(pport->lbaBuffer,size);
		}
		return(TRUE);
	}
#endif
#ifdef WS_COMPRESS
	if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].Compressed)
	{
		WSHWaitMutex(0x01,INFINITE);
		char *szTemp=pmod->UmrRead_szTemp;
		char *szDecompBuff=pmod->UmrRead_szDecompBuff;
	#ifdef WS_ENCRYPTED
		char *szDecryptBuff=pmod->UmrRead_szDecryptBuff;
		unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
	#endif
		if(!WSWriteBuff(&pmod->UmrPort[i].InCompBuffer,(char*)buf,bytes))
		{
			WSHReleaseMutex(0x01);
			return(FALSE);
		}
GetNextBlock:
		if(pmod->UmrPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
		{
			unsigned int *CompSize=(unsigned int*)pmod->UmrPort[i].InCompBuffer.Block;
			unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

			if(pmod->UmrPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
			{
			#ifdef WS_ENCRYPTED
				if(pmod->UmrPort[i].EncryptionOn)
				{
					WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
						,&pmod->UmrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_UMR);
				}
				else
				{
					memcpy(szDecryptBuff,&pmod->UmrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
					DecryptSize=*CompSize;
				}
				if(uncompress(szDecompBuff,&DecompSize
					,szDecryptBuff,DecryptSize))
			#else
				if(uncompress(szDecompBuff,&DecompSize
					,&pmod->UmrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
			#endif
				{
					WSHReleaseMutex(0x01);
					WSHCloseUmrPort(pmod,i);
					return(FALSE);
				}

				WSStripBuff(&pmod->UmrPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
				if(!WSWriteBuff(&pmod->UmrPort[i].InBuffer,szDecompBuff,DecompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUmrPort(pmod,i);
					return(FALSE);
				}
				goto GetNextBlock;
			}
		}
		WSHReleaseMutex(0x01);
	}
	else
#endif
	{
		if(!WSWriteBuff(&pmod->UmrPort[i].InBuffer,(char*)buf,bytes))
		{
			WSHCloseUmrPort(pmod,i);
			return(FALSE);
		}
	}
	return(TRUE);
}
#else//!WS_OIO
int WsocksHostImpl::WSHUmrRead(WsocksApp *pmod, int PortNo)
{
	//static char szTemp[(WS_MAX_BLOCK_SIZE*1024)];
	char *szTemp=pmod->UmrRead_szTemp;
#ifdef WS_COMPRESS
	//static char szDecompBuff[(WS_MAX_BLOCK_SIZE*1024)];
	char *szDecompBuff=pmod->UmrRead_szDecompBuff;
#ifdef WS_ENCRYPTED
	//static char szDecryptBuff[(WS_MAX_BLOCK_SIZE*1024)];
	char *szDecryptBuff=pmod->UmrRead_szDecryptBuff;
	unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
#endif
#endif
	int status;             // Status Code 
	int i=PortNo;

	if((pmod->UmrPort[i].InBuffer.Busy)&&(pmod->UmrPort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: UMR%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->UmrPort[i].InBuffer.Busy);
		pmod->UmrPort[i].InBuffer.Busy=0;
	}
	pmod->UmrPort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common UmrRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
#ifdef WS_COMPRESS
	if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].Compressed)
	{
		status = WSHRecv(pmod->UmrPort[i].Sock, szTemp, ((pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize-1)*1024), NO_FLAGS_SET );
		if (status>0) 
		{
			pmod->UmrPort[i].BytesIn+=status;
			pmod->UmrPort[i].BlocksIn++;

			if(!WSWriteBuff(&pmod->UmrPort[i].InCompBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				pmod->UmrPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
GetNextBlock:
			if(pmod->UmrPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
			{
				unsigned int *CompSize=(unsigned int*)pmod->UmrPort[i].InCompBuffer.Block;
				unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

				if(pmod->UmrPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
				{
#ifdef WS_ENCRYPTED
					if(pmod->UmrPort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->UmrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_UMR);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->UmrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize
						,szDecryptBuff,DecryptSize))
#else
					if(uncompress(szDecompBuff,&DecompSize
						,&pmod->UmrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
#endif
					{
						WSHReleaseMutex(0x01);
						WSHCloseUmrPort(pmod,i);
						return(FALSE);
					}

					WSStripBuff(&pmod->UmrPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
					if(!WSWriteBuff(&pmod->UmrPort[i].InBuffer,szDecompBuff,DecompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUmrPort(pmod,i);
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
			if(pmod->UmrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUmrMsgReady(i))
					pmod->WSUmrStripMsg(i);
			}
			WSHCloseUmrPort(pmod,i);
			return(FALSE);
		}
	}
	else
#endif
	{
		status = WSHRecv(pmod->UmrPort[i].Sock, szTemp, ((pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize-1)*1024), NO_FLAGS_SET );

		if (status>0) 
		{
			pmod->UmrPort[i].BytesIn+=status;
			if(status==132||status==8)
			{
				if(*((WORD *)szTemp)!=16)
				{
					pmod->UmrPort[i].BlocksIn++;
				}
			}
			else
			{
				pmod->UmrPort[i].BlocksIn++;
			}
			if(!WSWriteBuff(&pmod->UmrPort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseUmrPort(pmod,i);
				return(FALSE);
			}
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->UmrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUmrMsgReady(i))
					pmod->WSUmrStripMsg(i);
			}
			WSHCloseUmrPort(pmod,i);
			return(FALSE);
		}
	}
	WSHReleaseMutex(0x01);
	pmod->UmrPort[i].InBuffer.Busy=lastBusy;
	return(TRUE);
}
#endif//!WS_OIO
int WsocksHostImpl::WSHUmrSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	MSGHEADER MsgOutHeader;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UMR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	MsgOutHeader.MsgID=MsgID;
	MsgOutHeader.MsgLen=MsgLen;
	LockPort(pmod,WS_UMR,PortNo,true);
	int lastSend=pmod->UmrPort[PortNo].sendThread;
	pmod->UmrPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->UmrPort[PortNo].SockActive)
		#if defined(WS_OIO)||defined(WS_OTPP)
		&&(!pmod->UmrPort[PortNo].peerClosed)
		#endif
		)
	{
		if((pmod->UmrPort[PortNo].OutBuffer.Busy)&&(pmod->UmrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: UMR%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->UmrPort[PortNo].OutBuffer.Busy);
			pmod->UmrPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->UmrPort[PortNo].OutBuffer.Busy;
		pmod->UmrPort[PortNo].OutBuffer.Busy=pmod->UmrPort[PortNo].sendThread;
#ifdef WS_LOOPBACK
		BUFFER *Buffer=&pmod->UmrPort[PortNo].OutBuffer;
		int ConPortNo=-1;
		// Short circuit loopback connections
		WSPort *pport=(WSPort*)pmod->UmrPort[PortNo].Sock;
		if((pport->lbaPeer)&&(pport->lbaPeer->lbaBuffer))
		{
			_ASSERT(pport->lbaPeer->PortType==WS_CON);
			ConPortNo=pport->lbaPeer->PortNo;
			Buffer=pport->lbaPeer->lbaBuffer;
		}
		//Send Header
		if(!WSWriteBuff(Buffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UmrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_UMR,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(Buffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UmrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_UMR,PortNo,true);
				return(FALSE);
			}
		}
		pmod->UmrPort[PortNo].PacketsOut++;
		if(ConPortNo>=0)
		{
			pmod->UmrPort[PortNo].BytesOut+=sizeof(MSGHEADER) +MsgOutHeader.MsgLen;
			WSHRecord(pmod,&pmod->UmrPort[PortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),true);
			WSHRecord(pmod,&pmod->UmrPort[PortNo].Recording,MsgOut,MsgOutHeader.MsgLen,true);
			// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
			// but that was problematic during close of the ports. 
			// Instead, try unlocking the UMR port before doing any CON port message handling.
			pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UmrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_UMR,PortNo,true);
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
			WSHUmrSend(pmod,PortNo,false);
	#endif
#else
		//Send Header
		if(!WSWriteBuff(&pmod->UmrPort[PortNo].OutBuffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UmrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_UMR,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(&pmod->UmrPort[PortNo].OutBuffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UmrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_UMR,PortNo,true);
				return(FALSE);
			}
		}
		pmod->UmrPort[PortNo].PacketsOut++;
	#ifdef WS_REALTIMESEND
		WSHUmrSend(pmod,PortNo,false);
	#endif
#endif
		pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
		pmod->UmrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_UMR,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->UmrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_UMR,PortNo,true);
		return(FALSE);
	}
}
int WsocksHostImpl::WSHUmrSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UMR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_UMR,PortNo,true);
	int lastSend=pmod->UmrPort[PortNo].sendThread;
	pmod->UmrPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->UmrPort[PortNo].SockActive)
		#if defined(WS_OIO)||defined(WS_OTPP)
		&&(!pmod->UmrPort[PortNo].peerClosed)
		#endif
		)
	{
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if((pmod->UmrPort[PortNo].OutBuffer.Busy)&&(pmod->UmrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
			{
				_ASSERT(false);
				WSHLogError(pmod,"!CRASH: UMR%d OutBuffer.Busy detected a possible thread %d crash.",
					PortNo,pmod->UmrPort[PortNo].OutBuffer.Busy);
				pmod->UmrPort[PortNo].OutBuffer.Busy=0;
			}
			int lastBusy=pmod->UmrPort[PortNo].OutBuffer.Busy;
			pmod->UmrPort[PortNo].OutBuffer.Busy=pmod->UmrPort[PortNo].sendThread;
#ifdef WS_LOOPBACK
			BUFFER *Buffer=&pmod->UmrPort[PortNo].OutBuffer;
			int ConPortNo=-1;
			// Short circuit loopback connections
			WSPort *pport=(WSPort*)pmod->UmrPort[PortNo].Sock;
			if((pport->lbaPeer)&&(pport->lbaPeer->lbaBuffer))
			{
				_ASSERT(pport->lbaPeer->PortType==WS_CON);
				ConPortNo=pport->lbaPeer->PortNo;
				Buffer=pport->lbaPeer->lbaBuffer;
			}
			if(WSWriteBuff(Buffer,MsgOut,MsgLen)==0)
			{
				pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UmrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_UMR,PortNo,true);
				return(FALSE);
			}
			pmod->UmrPort[PortNo].PacketsOut+=Packets;
			if(ConPortNo>=0)
			{
				pmod->UmrPort[PortNo].BytesOut+=MsgLen;
				WSHRecord(pmod,&pmod->UmrPort[PortNo].Recording,MsgOut,MsgLen,true);
				// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
				// but that was problematic during close of the ports. 
				// Instead, try unlocking the UMR port before doing any CON port message handling.
				pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UmrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_UMR,PortNo,true);
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
				WSHUmrSend(pmod,PortNo,false);
	#endif
#else
			if(WSWriteBuff(&pmod->UmrPort[PortNo].OutBuffer,MsgOut,MsgLen)==0)
			{
				pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UmrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_UMR,PortNo,true);
				return(FALSE);
			}
			pmod->UmrPort[PortNo].PacketsOut+=Packets;
	#ifdef WS_REALTIMESEND
			if(Packets)
				WSHUmrSend(pmod,PortNo);
	#endif
#endif
			pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
		}
	#ifdef WS_REALTIMESEND
		WSHUmrSend(pmod,PortNo,false);
	#endif
		pmod->UmrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_UMR,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->UmrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_UMR,PortNo,true);
		return(FALSE);
	}
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseUmrPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UmrPort[PortNo].SockActive)
		return -1;
	if(pmod->UmrPort[PortNo].SockActive)
		pmod->UmrPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUmrPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->UmrPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_UMR,PortNo,false);
		lastRecv=pmod->UmrPort[PortNo].recvThread;
		pmod->UmrPort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->UmrPort[PortNo].pendingClose)
		{
			pmod->UmrPort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_UMR,PortNo,false);
			return 0;
		}
		pmod->UmrPort[PortNo].pendingClose=true;
	}
	if(pmod->UmrPort[PortNo].smutex)
	{
		LockPort(pmod,WS_UMR,PortNo,true);
		lastSend=pmod->UmrPort[PortNo].sendThread;
		pmod->UmrPort[PortNo].sendThread=GetCurrentThreadId();
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->UmrPort[PortNo].pendOvlRecvList; pmod->UmrPort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->UmrPort[PortNo].pendOvlSendList; pmod->UmrPort[PortNo].pendOvlSendList=0;
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

	//ResetSendTimeout(WS_UMR,PortNo);
	pmod->UmrPort[PortNo].SockActive=0;
	pmod->WSUmrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_UMR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_UMR,PortNo));
	if(pmod->UmrPort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->UmrPort[PortNo].Sock)->sd;
		WSHClosePort(pmod->UmrPort[PortNo].Sock);
		WSHFinishOverlapSend(pmod,sd,&pmod->UmrPort[PortNo].pendOvlSendList);
	}

	WSHCloseRecording(pmod,&pmod->UmrPort[PortNo].Recording,WS_UMR,PortNo);
	WSCloseBuff(&pmod->UmrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UmrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UmrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UmrPort[PortNo].OutCompBuffer);
	#endif
	HANDLE srm=0,ssm=0;
	DWORD srcnt=0,sscnt=0;
	if(pmod->UmrPort[PortNo].smutex)
	{
		ssm=pmod->UmrPort[PortNo].smutex;
		pmod->UmrPort[PortNo].sendThread=lastSend;
		//DeleteMutex(pmod->UmrPort[PortNo].smutex); pmod->UmrPort[PortNo].smutex=0;
		DeletePortMutex(pmod,WS_UMR,PortNo,true);
		sscnt=pmod->UmrPort[PortNo].smutcnt;
	}
	if(pmod->UmrPort[PortNo].rmutex)
	{
		srm=pmod->UmrPort[PortNo].rmutex;
		pmod->UmrPort[PortNo].recvThread=lastRecv;
		//DeleteMutex(pmod->UmrPort[PortNo].rmutex); pmod->UmrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_UMR,PortNo,false);
		srcnt=pmod->UmrPort[PortNo].rmutcnt;
	}
	memset(&pmod->UmrPort[PortNo],0,sizeof(UMRPORT));
	if(sscnt)
	{
		pmod->UmrPort[PortNo].smutex=ssm;
		pmod->UmrPort[PortNo].smutcnt=sscnt;
	}
	if(srcnt)
	{
		pmod->UmrPort[PortNo].rmutex=srm;
		pmod->UmrPort[PortNo].rmutcnt=srcnt;
	}
	WSHUpdatePort(pmod,WS_UMR,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseUmrPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UmrPort[PortNo].SockActive)
		return -1;
	if(pmod->UmrPort[PortNo].SockActive)
		pmod->UmrPort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseUmrPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->UmrPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_UMR,PortNo,false);
		lastRecv=pmod->UmrPort[PortNo].recvThread;
		pmod->UmrPort[PortNo].recvThread=GetCurrentThreadId();
	}
	if(pmod->UmrPort[PortNo].smutex)
	{
		LockPort(pmod,WS_UMR,PortNo,true);
		lastSend=pmod->UmrPort[PortNo].sendThread;
		pmod->UmrPort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_UMR,PortNo);
	pmod->UmrPort[PortNo].SockActive=0;
	pmod->WSUmrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_UMR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_UMR,PortNo));
	WSHClosePort(pmod->UmrPort[PortNo].Sock);

	#ifdef WIN32
	HANDLE pthread=pmod->UmrPort[PortNo].pthread;
	#else
	pthread_t pthread=pmod->UmrPort[PortNo].pthread;
	#endif
	WSHCloseRecording(pmod,&pmod->UmrPort[PortNo].Recording,WS_UMR,PortNo);
	WSCloseBuff(&pmod->UmrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UmrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UmrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UmrPort[PortNo].OutCompBuffer);
	#endif
	HANDLE srm=0,ssm=0;
	DWORD srcnt=0,sscnt=0;
	if(pmod->UmrPort[PortNo].smutex)
	{
		ssm=pmod->UmrPort[PortNo].smutex;
		pmod->UmrPort[PortNo].sendThread=lastSend;
		//DeleteMutex(pmod->UmrPort[PortNo].smutex); pmod->UmrPort[PortNo].smutex=0;
		DeletePortMutex(pmod,WS_UMR,PortNo,true);
		sscnt=pmod->UmrPort[PortNo].smutcnt;
	}
	if(pmod->UmrPort[PortNo].rmutex)
	{
		srm=pmod->UmrPort[PortNo].rmutex;
		pmod->UmrPort[PortNo].recvThread=lastRecv;
		//DeleteMutex(pmod->UmrPort[PortNo].rmutex); pmod->UmrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_UMR,PortNo,false);
		srcnt=pmod->UmrPort[PortNo].rmutcnt;
	}
	memset(&pmod->UmrPort[PortNo],0,sizeof(UMRPORT));
	pmod->UmrPort[PortNo].pthread=pthread;
	if(sscnt)
	{
		pmod->UmrPort[PortNo].smutex=ssm;
		pmod->UmrPort[PortNo].smutcnt=sscnt;
	}
	if(srcnt)
	{
		pmod->UmrPort[PortNo].rmutex=srm;
		pmod->UmrPort[PortNo].rmutcnt=srcnt;
	}
	WSHUpdatePort(pmod,WS_UMR,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitUmrThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->UmrPort[PortNo].pthread)
	{
	#ifdef WIN32
		#ifdef WS_OTMP
		if(pmod->pcfg->umrOtmp)
		{
			pmod->UmrPort[PortNo].pthread=0;
			return;
		}
		#endif
		#ifdef _DEBUG
		WaitForSingleObject(pmod->UmrPort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->UmrPort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->UmrPort[PortNo].pthread); pmod->UmrPort[PortNo].pthread=0;
	#else
		void *rc=0;
		pthread_join(pmod->UmrPort[PortNo].pthread,&rc);
	#endif
	}
}
#else//!WS_OIO
int WsocksHostImpl::WSHCloseUmrPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->UmrPort[PortNo].rmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_UMR,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->UmrPort[PortNo].recvThread)&&(pmod->UmrPort[PortNo].recvThread!=tid))
		{
			UnlockPort(pmod,WS_UMR,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_UMR,PortNo,false);
		}
		pmod->UmrPort[PortNo].recvThread=tid;
	}
	//ResetSendTimeout(WS_UMR,PortNo);
	pmod->UmrPort[PortNo].SockActive=0;
	pmod->WSUmrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_UMR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_UMR,PortNo));
	WSHClosePort(pmod->UmrPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->UmrPort[PortNo].Recording,WS_UMR,PortNo);
	WSCloseBuff(&pmod->UmrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UmrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UmrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UmrPort[PortNo].OutCompBuffer);
	#endif
	pmod->UmrPort[PortNo].recvThread=0;
	if(pmod->UmrPort[PortNo].rmutex)
	{
		//DeleteMutex(pmod->UmrPort[PortNo].rmutex); pmod->UmrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_UMR,PortNo,false);
	}
	memset(&pmod->UmrPort[PortNo],0,sizeof(UMRPORT));
	//WSHPortChanged(pmod,pmod->UmrPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO

int WsocksHostImpl::WSHUmrSend(WsocksApp *pmod, int PortNo, bool flush)
{
	int nsent=0;
	if(!pmod->UmrPort[PortNo].SockActive)
		return -1;

	bool compressed=false;
	int lastBusy=pmod->UmrPort[PortNo].OutBuffer.Busy;
	pmod->UmrPort[PortNo].OutBuffer.Busy=pmod->UmrPort[PortNo].sendThread;
	pmod->UmrPort[PortNo].SendTimeOut=0;
	BUFFER *sendBuffer=&pmod->UmrPort[PortNo].OutBuffer;
#ifdef WS_COMPRESS
	if(pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].Compressed)
	{
		compressed=true;
		sendBuffer=&pmod->UmrPort[PortNo].OutCompBuffer;
		// Compress as much as possible to fill one send block
		while((pmod->UmrPort[PortNo].OutBuffer.Size>0)&&
			  ((flush)||((int)pmod->UmrPort[PortNo].OutBuffer.Size>=pmod->UmrPort[PortNo].ImmediateSendLimit)))
		{
			unsigned int CompSize=pmod->UmrPort[PortNo].OutBuffer.LocalSize;
			if(CompSize>((WS_COMP_BLOCK_LEN*1024)*99/100))
				CompSize=((WS_COMP_BLOCK_LEN*1024)*99/100);
			if(CompSize>0)
			{
				WSHWaitMutex(0x01,INFINITE);
				char *CompBuff=pmod->SyncLoop_CompBuff;
				unsigned int CompBuffSize=sizeof(pmod->SyncLoop_CompBuff);
				mtcompress(CompBuff,&CompBuffSize,pmod->UmrPort[PortNo].OutBuffer.Block,CompSize);
				// Compression +encryption
				#ifdef WS_ENCRYPTED
				char *EncryptBuff=CompBuff;
				unsigned int EncryptSize=CompBuffSize;
				if(pmod->UmrPort[PortNo].EncryptionOn)
				{
					EncryptBuff=pmod->SyncLoop_EncryptBuff;
					EncryptSize=sizeof(pmod->SyncLoop_EncryptBuff);
					WSHEncrypt(pmod,EncryptBuff,&EncryptSize,CompBuff,CompBuffSize,PortNo,WS_UMR);
				}
				WSWriteBuff(sendBuffer,(char*)&EncryptSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,EncryptBuff,EncryptSize);
				// Compression only
				#else//!WS_ENCRYPTED
				WSWriteBuff(sendBuffer,(char*)&CompBuffSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,CompBuff,CompBuffSize);
				#endif//WS_ENCRYPTED
				WSHRecord(pmod,&pmod->UmrPort[PortNo].Recording,pmod->UmrPort[PortNo].OutBuffer.Block,CompSize,true);
				WSStripBuff(&pmod->UmrPort[PortNo].OutBuffer,CompSize);
				WSHReleaseMutex(0x01);
			}
		}
	}
#endif//WS_COMPRESS
	// Send as much as possible
	while((sendBuffer->Size>0)&&((flush)||(compressed)||((int)sendBuffer->Size>=pmod->UmrPort[PortNo].ImmediateSendLimit)))
	{
		int size=sendBuffer->LocalSize;
		if(size>pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].BlockSize*1024)
			size=pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->UmrPort[PortNo].Sock;
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=WSHSendNonBlock(pmod,WS_UMR,PortNo,(SOCKET_T)pport,sendBuffer->Block,size);
		#else
		int rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_UMR_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->UmrPort[PortNo].SendTimeOut)
				{
					pmod->UmrPort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->UmrPort[PortNo].SendTimeOut<WS_UMR_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: UMR%d [%s] Reset by Peer",PortNo,pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: UMR%d [%s] Send Failed: %d",PortNo,pmod->UmcPort[pmod->UmrPort[PortNo].UmcPort].CfgNote,lerr);
				WSHCloseUmrPort(pmod,PortNo);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		if(!compressed)
			WSHRecord(pmod,&pmod->UmrPort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->UmrPort[PortNo].BytesOut+=rc;
		pmod->UmrPort[PortNo].BlocksOut++;
		pmod->UmrPort[PortNo].SendTimeOut=0;
		nsent+=rc;
	}
	pmod->UmrPort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}

int WsocksHostImpl::WSHGetTagValues(WsocksApp *pmod, TVMAP& tvmap, const char *parm, int plen)
{
	return (pmod)&&(pmod->ccodec) ?pmod->ccodec->GetTagValues(tvmap,parm,plen) :-1;
}
string WsocksHostImpl::WSHGetValue(WsocksApp *pmod, TVMAP& tvmap, string tag)
{
	return (pmod)&&(pmod->ccodec) ?pmod->ccodec->GetValue(tvmap,tag) :"";
}
int WsocksHostImpl::WSHSetTagValues(WsocksApp *pmod, TVMAP& tvmap, char *parm, int plen)
{
	return (pmod)&&(pmod->ccodec) ?pmod->ccodec->SetTagValues(tvmap,parm,plen) :-1;
}
int WsocksHostImpl::WSHSetValue(WsocksApp *pmod, TVMAP& tvmap, string tag, string value)
{
	return (pmod)&&(pmod->ccodec) ?pmod->ccodec->SetValue(tvmap,tag,value) :-1;
}
int WsocksHostImpl::WSHDelTag(WsocksApp *pmod, TVMAP& tvmap, string tag)
{
	return (pmod)&&(pmod->ccodec) ?pmod->ccodec->DelTag(tvmap,tag) :-1;
}
int WsocksHostImpl::WSHSysmonFeed(WsocksApp *pmod, int UmrPortNo, string feed, const char *buf, int blen)
{
	MSGHEADER MsgHeader={1110,1+blen};
	WSHUmrSendBuff(pmod,sizeof(MSGHEADER),(char*)&MsgHeader,UmrPortNo,0);
	char feedno=SMFNO_CUSTOM;
	WSHUmrSendBuff(pmod,1,&feedno,UmrPortNo,0);
	WSHUmrSendBuff(pmod,blen,(char*)buf,UmrPortNo,1);
	return 0;
}
int WsocksHostImpl::WSHSysmonResp(WsocksApp *pmod, int UmrPortNo, int reqid, string cmd, const char *parm, int plen)
{
	if((!pmod)||(!pmod->ccodec))
		return -1;
	char mbuf[2048]={0},*mptr=mbuf;
	int mlen=sizeof(mbuf);
	pmod->ccodec->Encode(mptr,mlen,false,reqid,cmd,parm,plen);
	WSHUmrSendMsg(pmod,1109,(int)(mptr -mbuf),mbuf,UmrPortNo);
	return 0;
}

// This is the default monitor message handler for apps
struct SMHINT
{
	WsocksApp *pmod;
	WSPortType PortType;
	int PortNo;
};
void WsocksHostImpl::NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata)
{
	SMHINT *phint=(SMHINT *)udata;
	phint->pmod->WSSysmonMsg(phint->PortNo,reqid,cmd,parm,plen,(void*)(PTRCAST)MAKELONG(phint->PortNo,phint->PortType));
}
void WsocksHostImpl::NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata)
{
#ifdef WS_FILE_SMPROXY
	WSDecodeHint *phint=(WSDecodeHint *)udata;
	int PortType=phint->PortType;
	int PortNo=phint->PortNo;
	WsocksApp *pmod=phint->pmod;
	if(PortType==WS_FIL)
	{
		TVMAP tvmap;
		if(ccodec.GetTagValues(tvmap,resp,rlen)<0)
			return;
		string dtid=ccodec.GetValue(tvmap,"DeliverTo");
		string oboi=ccodec.GetValue(tvmap,"OnBehalfOf");
		#ifdef _DEBUG
		size_t _lenResp;
		_lenResp = strlen(resp);
		#endif
		if(cmd=="login")
		{
			string rc=ccodec.GetValue(tvmap,"rc");
			if(rc=="0")
			{
				string text=ccodec.GetValue(tvmap,"Text");
				WSHLogEvent(pmod,"FILE%d: login: %s",PortNo,text.c_str());

				int& rid=(int&)pmod->FilePort[PortNo].DetPtr4;
				char mbuf[1024]={0},*mptr=mbuf;
				char parms[1024];
				sprintf_s(parms, "OnBehalfOf=%s|Hist=Y|Live=N|",pmod->pcfg->aname.c_str());
				ccodec.Encode(mptr,sizeof(mbuf),true,++rid,"applist",parms,0);
				WSHFileSendMsg(pmod,1108,(int)(mptr -mbuf),mbuf,PortNo);
			}
			else
			{
				string reason=ccodec.GetValue(tvmap,"Reason");
				WSHLogEvent(pmod,"FILE%d: Failed login: %s",PortNo,reason.c_str());
			}
		}
		else if(cmd=="applist")
		{
			string apath=ccodec.GetValue(tvmap, "AppPath");
			strcpy(pmod->FilePort[PortNo].OnLineStatusText,apath.c_str());
			// PreInitSocks case: we can't send download request until we have a DeliverTo
			WSFileTransfer2 *wft=(WSFileTransfer2 *)pmod->FilePort[PortNo].DetPtr5;
			if((wft)&&(wft->status==WSF_INIT))
			{
				int& rid=(int&)pmod->FilePort[PortNo].DetPtr4;
				char mbuf[1024]={0},*mptr=mbuf;
				char parms[1024];
				// Single file download request
				if(wft->download)
				{
					sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|FileKey=%s|FilePath=%s|",
						pmod->pcfg->aname.c_str(),apath.c_str(),wft->localFile.c_str(),wft->remoteFile.c_str());
					ccodec.Encode(mptr,sizeof(mbuf),true,++rid,"downloadfile",parms,0);
					WSHFileSendMsg(pmod,1108,(int)(mptr -mbuf),mbuf,PortNo); 
				}
				// Single file upload request
				else
				{
					sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|SrcPath=%s|FilePath=%s|Overwrite=Y|",
						pmod->pcfg->aname.c_str(),apath.c_str(),wft->localFile.c_str(),wft->remoteFile.c_str());
					ccodec.Encode(mptr,sizeof(mbuf),true,++rid,"uploadfile",parms,0);
					WSHFileSendMsg(pmod,1108,(int)(mptr -mbuf),mbuf,PortNo); 
				}
			}
		}
		else if(cmd=="uploadfile")
		{
			string rc=ccodec.GetValue(tvmap,"rc");
			//string text=ccodec.GetValue(tvmap, "Text");
			//string reason=ccodec.GetValue(tvmap, "Reason");
			//WSHLogEvent(pmod,"Upload: %s: %s",rc.c_str(),text.empty()?reason.c_str():text.c_str());
			WSFileTransfer2 *wft=(WSFileTransfer2 *)pmod->FilePort[PortNo].DetPtr5;
			if(wft)
			{
				if(rc=="0")
					wft->status=WSF_SUCCESS;
				else
					wft->status=WSF_FAILED;
				SetEvent(wft->doneEvent);
			}
		}
		else if(cmd=="downloadfile")
		{
			string rc=ccodec.GetValue(tvmap,"rc");
			//string filekey=ccodec.GetValue(tvmap, "FileKey");
			//string reason=ccodec.GetValue(tvmap, "Reason");
			//WSHLogEvent(pmod,"Download: %s: %s",rc.c_str(),filekey.empty()?reason.c_str():filekey.c_str());
			WSFileTransfer2 *wft=(WSFileTransfer2 *)pmod->FilePort[PortNo].DetPtr5;
			if(wft)
			{
				if(rc=="0")
					wft->status=WSF_SUCCESS;
				else
					wft->status=WSF_FAILED;
				SetEvent(wft->doneEvent);
			}
		}
		return;
	}
#endif
	_ASSERT(false); // An app shouldn't get a response from UMR message
}
int WsocksHostImpl::WSHUmrMsgReady(WsocksApp *pmod, int PortNo)
{
	if(pmod->UmrPort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
		return 0;
	MSGHEADER MsgHeader;
	memcpy((char *)&MsgHeader,pmod->UmrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(pmod->UmrPort[PortNo].InBuffer.Size<(sizeof(MSGHEADER)+MsgHeader.MsgLen))
		return(0);
	pmod->UmrPort[PortNo].PacketsIn++;
	// Apps that support loopback should strip the msg before handling
	// because any new reply will be handled inline
	char *mcpy=new char[sizeof(MSGHEADER) +MsgHeader.MsgLen];
	memcpy(mcpy,pmod->UmrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER) +MsgHeader.MsgLen);
	if(!WSStripBuff(&pmod->UmrPort[PortNo].InBuffer,sizeof(MSGHEADER) +MsgHeader.MsgLen))
	{
		delete mcpy;
		WSHCloseUmrPort(pmod,PortNo);
		return -1;
	}
	switch(MsgHeader.MsgID)
	{
	case 16:
		pmod->UmrPort[PortNo].BeatsIn++;
		break;
	// New sysmon requests
	case 1108:
	{
		const char *Msg=mcpy +sizeof(MSGHEADER);
		const char *mend=Msg +MsgHeader.MsgLen;
		SMHINT smhint={pmod,WS_UMR,PortNo};
		while(Msg<mend)
		{
			if((!pmod->ccodec)||(pmod->ccodec->Decode((const char *&)Msg,MsgHeader.MsgLen,true,this,&smhint)<0))
				return -1;
		}
		break;
	}
	};
	delete mcpy;
	return(1);
}
int WsocksHostImpl::WSHUmrStripMsg(WsocksApp *pmod, int PortNo)
{
	return 0;
}
