
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

int WsocksHostImpl::WSHSetupConPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	//switch(SMode)
	//{
	//case WS_INIT: return WSInitConPort(PortNo);
	//case WS_OPEN: return WSOpenConPort(PortNo);
	//case WS_CLOSE: return _WSHCloseConPort(pmod,PortNo);
	//};
	int i;	
	int StartPort=0;
	int EndPort=0;
	int Single=false;

	if(PortNo>=0)
	{
		StartPort=EndPort=PortNo;
		Single=true;
	}
	else
	{
		StartPort=0; EndPort=pmod->NO_OF_CON_PORTS;
	}
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=StartPort;i<=EndPort;i++)
		{
			memset(&pmod->ConPort[i],0,sizeof(CONPORT));
			pmod->ConPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
		#ifdef WS_DECLINING_RECONNECT
			pmod->ConPort[i].MinReconnectDelay=MIN_RECONNECT_DELAY;
			pmod->ConPort[i].MaxReconnectDelay=MAX_RECONNECT_DELAY;
			pmod->ConPort[i].MinReconnectReset=MIN_RECONNECT_RESET;
			pmod->ConPort[i].ReconnectDelay=0;
			pmod->ConPort[i].ReconnectTime=0;
			pmod->ConPort[i].ConnectTime=0;
		#endif
			pmod->ConPort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
			pmod->ConPort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		}
		break;
	case WS_OPEN: // Open
		for (i=StartPort;i<=EndPort;i++)
		{
			if(strlen(pmod->ConPort[i].LocalIP)>0)
			{
				if(pmod->ConPort[i].InUse)
					continue;
				pmod->ConPort[i].rmutex=CreateMutex(0,false,0);
				pmod->ConPort[i].smutex=CreateMutex(0,false,0);
				pmod->ConPort[i].recvThread=0;
				pmod->ConPort[i].sendThread=0;
				WSOpenBuffLimit(&pmod->ConPort[i].OutBuffer,pmod->ConPort[i].BlockSize,pmod->ConPort[i].SendBuffLimit);
				WSOpenBuffLimit(&pmod->ConPort[i].InBuffer,pmod->ConPort[i].BlockSize,pmod->ConPort[i].RecvBuffLimit);
			#ifdef WS_COMPRESS
				WSOpenBuffLimit(&pmod->ConPort[i].OutCompBuffer,pmod->ConPort[i].BlockSize,pmod->ConPort[i].SendBuffLimit);
				WSOpenBuffLimit(&pmod->ConPort[i].InCompBuffer,pmod->ConPort[i].BlockSize,pmod->ConPort[i].RecvBuffLimit);
			#endif
				pmod->ConPort[i].InUse=TRUE;
				//AddConListItem(GetDispItem(WS_CON,i));
			}
		}
		//if(!Single)
		//	AddConListItem(GetDispItem(WS_CON_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=StartPort;i<=EndPort;i++)
		{
			WSHCloseRecording(pmod,&pmod->ConPort[i].Recording,WS_CON,i);
			_WSHCloseConPort(pmod,i);
			pmod->ConPort[i].recvThread=0;
			pmod->ConPort[i].sendThread=0;
			if(pmod->ConPort[i].smutex)
			{
				DeleteMutex(pmod->ConPort[i].smutex); pmod->ConPort[i].smutex=0;
			}
			if(pmod->ConPort[i].rmutex)
			{
				DeleteMutex(pmod->ConPort[i].rmutex); pmod->ConPort[i].rmutex=0;
			}
			if(pmod->ConPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_CON,i));
				pmod->ConPort[i].InUse=FALSE;
			}
			#ifdef WS_OTPP
			_WSHWaitConThreadExit(pmod,i);
			#endif
		}
		#ifdef WS_OTMP
		WSHWaitConThreadPoolsExit();
		#endif
		break;
	}
	return(TRUE);
}
int WsocksHostImpl::WSHOpenConPort(WsocksApp *pmod, int PortNo)
{
#ifdef WS_DECLINING_RECONNECT
	if(pmod->ConPort[PortNo].ReconnectTime)
	{
		DWORD tnow=GetTickCount();
		if(tnow<pmod->ConPort[PortNo].ReconnectTime)
		{
			// The 32-bit tick count loops zero once every 49.7 days,
			// in which case we have a vary large ReconnectTime and small tnow
			if(pmod->ConPort[PortNo].ReconnectTime -tnow<=MAX_RECONNECT_DELAY)
				return FALSE;
		}
	}
#endif

	// Don't even create a socket if none of the IPs are active
	bool enabled=false;
	for(int i=0;i<pmod->ConPort[PortNo].AltIPCount;i++)
	{
		if(pmod->ConPort[PortNo].AltRemoteIPOn[i])
		{
			enabled=true;
			break;
		}
	}
	if(!enabled)
	{
	#ifdef WS_DECLINING_RECONNECT
		pmod->ConPort[PortNo].ReconnectTime=0;
	#endif
		return FALSE;
	}

	#ifdef WSOCKS_SSL
	// Most apps won't need SSL support, so don't iniitialize the gateway until we open the first USC or CON port
	if(pmod->ConPort[PortNo].Encrypted==2)
	{
		if((!sslg)&&(ReloadCertificates(false)<0))
		{
			pmod->ConPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d SSLInit() failed",PortNo);
			_WSHCloseConPort(pmod,PortNo);
			return(FALSE);
		}
	}
	#endif

	if(pmod->ConPort[PortNo].ShellPort)
	{
		pmod->ConPort[PortNo].Sock=(SOCKET_T)new char[sizeof(WSPort)];
		memset((char*)pmod->ConPort[PortNo].Sock,0,sizeof(WSPort));
		pmod->ConPort[PortNo].SockOpen=TRUE;
		WSHConConnect(pmod,PortNo);
		WSHUpdatePort(pmod,WS_CON,PortNo);
		return 0;
	}

	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->ConPort[PortNo].BlockSize*8192;

	struct
	{
		int l_onoff;
		int l_linger;
	} linger;
	
	//pmod->ConPort[PortNo].Sock = socket(AF_INET, SOCK_STREAM, 0);
	pmod->ConPort[PortNo].Sock = WSHSocket(pmod,WS_CON,PortNo);
	if (pmod->ConPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->ConPort[PortNo].Sock=0;
		pmod->ConPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d socket() failed with error=%d",PortNo,errno);
		perror(NULL);
		return(FALSE);
	}
#ifdef _DEBUG_THREADS
	WSHLogEvent(pmod,"DEBUG Create CON%d / %d",PortNo,((WSPort*)pmod->ConPort[PortNo].Sock)->sd);
#endif
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->ConPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->ConPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d ioctlsocket(FIONBIO) failed",PortNo);
		_WSHCloseConPort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->ConPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->ConPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d setsockopt(SO_REUSEADDR) failed",PortNo);
		_WSHCloseConPort(pmod,PortNo);
		return(FALSE);
	}

	linger.l_onoff=1;
	linger.l_linger=0;
	if (WSHSetSockOpt(pmod->ConPort[PortNo].Sock, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof(linger)) == SOCKET_ERROR) 
	{
		pmod->ConPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d setsockopt(SO_LINGER) failed",PortNo);
		_WSHCloseConPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->ConPort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->ConPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d setsockopt(SO_SNDBUF,%d) failed",PortNo,SndBuf);
		_WSHCloseConPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->ConPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->ConPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d setsockopt(SO_RCVBUF,%d) failed",PortNo,SndBuf);
		_WSHCloseConPort(pmod,PortNo);
		return(FALSE);
	}

	// There's no evidence this helps
//#ifdef WS_REALTIMESEND
//	// Disable Nagle
//	unsigned int nagleoff=1;
//	if (WSHSetSockOpt(pmod->ConPort[PortNo].Sock, IPPROTO_TCP, TCP_NODELAY, (char *)(&nagleoff), sizeof(nagleoff)) == SOCKET_ERROR) 
//	{
//		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d setsockopt(TCP_NODELAY) failed",PortNo);
//		_WSHCloseConPort(pmod,PortNo);
//		return(FALSE);
//	}
//#endif

	// Connect sockets will automatically bind an unused port. Otherwise, when the dest is
	// loopback or a local interface, it may accidentally bind the dest port.
	if (strcmp(pmod->ConPort[PortNo].LocalIP,"AUTO")!=0)
	{
		// Retrieve the local IP address and TCP Port number
		local_sin.sin_family=AF_INET;
		local_sin.sin_port=INADDR_ANY;
		local_sin.sin_addr.s_addr=inet_addr(pmod->ConPort[PortNo].LocalIP);

	    //  Associate an address with a socket. (bind)
		//if (bind( pmod->ConPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
		if (WSHBindPort( pmod->ConPort[PortNo].Sock, &local_sin.sin_addr, INADDR_ANY) == SOCKET_ERROR) 
		{
			pmod->ConPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CON%d bind(INADDR_ANY) failed",PortNo);
			_WSHCloseConPort(pmod,PortNo);
			return(FALSE);
		}
	}
	WSHConConnect(pmod,PortNo);
	pmod->ConPort[PortNo].SockOpen=TRUE;
	WSHUpdatePort(pmod,WS_CON,PortNo);
	return(TRUE);
}
int WsocksHostImpl::WSHConConnect(WsocksApp *pmod, int PortNo)
{
	if(pmod->ConPort[PortNo].ShellPort)
	{
		pmod->WSConConnecting(PortNo);
		if(pmod->ConPort[PortNo].SockConnected)
			WSHConConnected(pmod,PortNo);
		return 0;
	}

	SOCKADDR_IN remote_sin; // Remote socket addr - internet style 
	int cnt;

	remote_sin.sin_family=AF_INET;
	remote_sin.sin_port=htons(pmod->ConPort[PortNo].Port);
	strcpy (pmod->ConPort[PortNo].Note, pmod->ConPort[PortNo].CfgNote);
	cnt = 0;

	do
	{
		pmod->ConPort[PortNo].CurrentAltIP++;
		if(pmod->ConPort[PortNo].CurrentAltIP>=pmod->ConPort[PortNo].AltIPCount)
			pmod->ConPort[PortNo].CurrentAltIP=0;
		strcpy(pmod->ConPort[PortNo].RemoteIP,pmod->ConPort[PortNo].AltRemoteIP[pmod->ConPort[PortNo].CurrentAltIP]);
		if(pmod->ConPort[PortNo].AltRemotePort[pmod->ConPort[PortNo].CurrentAltIP])
			pmod->ConPort[PortNo].Port=pmod->ConPort[PortNo].AltRemotePort[pmod->ConPort[PortNo].CurrentAltIP];
		if (cnt > pmod->ConPort[PortNo].AltIPCount+1) {
			strcpy(pmod->ConPort[PortNo].RemoteIP, "");
			pmod->ConPort[PortNo].Port=0;
			break;// none !
		}
		cnt++;

	}while(!pmod->ConPort[PortNo].AltRemoteIPOn[pmod->ConPort[PortNo].CurrentAltIP]);

	if(pmod->ConPort[PortNo].S5Connect)
	{
		remote_sin.sin_addr.s_addr=inet_addr(pmod->ConPort[PortNo].S5RemoteIP);
		remote_sin.sin_port=htons(pmod->ConPort[PortNo].S5Port);
	}
	else
	{
		remote_sin.sin_addr.s_addr=inet_addr(pmod->ConPort[PortNo].RemoteIP);
		remote_sin.sin_port=htons(pmod->ConPort[PortNo].Port);
	}
	
	pmod->WSConConnecting(PortNo);
	//if (connect( pmod->ConPort[PortNo].Sock, (struct sockaddr *) &remote_sin, sizeof(sockaddr)) == SOCKET_ERROR)  
	if (WSHConnectPort( pmod->ConPort[PortNo].Sock, &remote_sin.sin_addr, ntohs(remote_sin.sin_port)) == SOCKET_ERROR)  
	{
		//pmod->ConPort[PortNo].ReconCount=WS_CONOUT_RECON/(WS_TIMER_INTERVAL?WS_TIMER_INTERVAL:1)+1;
		pmod->ConPort[PortNo].ReconCount=(GetTickCount() +WS_CONOUT_RECON);
		WSHUpdatePort(pmod,WS_CON,PortNo);
		return(FALSE);
	}
	WSHConConnected(pmod,PortNo);
	return(TRUE);
}
int WsocksHostImpl::WSHConSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	MSGHEADER MsgOutHeader;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CON_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->ConPort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	MsgOutHeader.MsgID=MsgID;
	MsgOutHeader.MsgLen=MsgLen;
	LockPort(pmod,WS_CON,PortNo,true);
	int lastSend=pmod->ConPort[PortNo].sendThread;
	pmod->ConPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->ConPort[PortNo].SockConnected)
		#ifdef WS_OIO
		&&(!pmod->ConPort[PortNo].pendingClose)
		#endif
		)
	{
		if((pmod->ConPort[PortNo].OutBuffer.Busy)&&(pmod->ConPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: CON%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->ConPort[PortNo].OutBuffer.Busy);
			pmod->ConPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->ConPort[PortNo].OutBuffer.Busy;
		pmod->ConPort[PortNo].OutBuffer.Busy=pmod->ConPort[PortNo].sendThread;
#ifdef WS_LOOPBACK
		BUFFER *Buffer=&pmod->ConPort[PortNo].OutBuffer;
		int UsrPortNo=-1,UmrPortNo=-1;
		// Short circuit loopback connections
		WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
		if((pport)&&(pport->lbaPeer)&&(pport->lbaPeer->lbaBuffer))
		{
			_ASSERT((pport->lbaPeer->PortType==WS_USR)||(pport->lbaPeer->PortType==WS_UMR));
			if(pport->lbaPeer->PortType==WS_USR)
			{
				UsrPortNo=pport->lbaPeer->PortNo;
				Buffer=pport->lbaPeer->lbaBuffer;
			}
			else if(pport->lbaPeer->PortType==WS_UMR)
			{
				UmrPortNo=pport->lbaPeer->PortNo;
				Buffer=pport->lbaPeer->lbaBuffer;
			}
		}
		//Send Header
		if(!WSWriteBuff(Buffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->ConPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CON,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(Buffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->ConPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CON,PortNo,true);
				return(FALSE);
			}
		}
		pmod->ConPort[PortNo].PacketsOut++;
		if(UsrPortNo>=0)
		{
			WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),true);
			WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,MsgOut,MsgOutHeader.MsgLen,true);
			// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
			// but that was problematic during close of the ports. 
			// Instead, try unlocking the CON port before doing any USR port message handling.
			pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->ConPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CON,PortNo,true);
			LockPort(pport->lbaPeer->pmod,WS_USR,UsrPortNo,false);
		#if defined(WS_OIO)||defined(WS_OTPP)
			if(WSHUsrRead(pport->lbaPeer->pmod,UsrPortNo,(char*)&MsgOutHeader,sizeof(MSGHEADER))&&
			   WSHUsrRead(pport->lbaPeer->pmod,UsrPortNo,MsgOut,MsgOutHeader.MsgLen))
		#else
			WSHRecord(pmod,&pport->lbaPeer->pmod->UsrPort[UsrPortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),false);
			WSHRecord(pmod,&pport->lbaPeer->pmod->UsrPort[UsrPortNo].Recording,MsgOut,MsgOutHeader.MsgLen,false);
			if(WSHUsrRead(pport->lbaPeer->pmod,UsrPortNo))
		#endif
			{
				while((pport->lbaPeer)&&(pport->lbaPeer->pmod->WSUsrMsgReady(UsrPortNo)))
					pport->lbaPeer->pmod->WSUsrStripMsg(UsrPortNo);
			}
			UnlockPort(pport->lbaPeer->pmod,WS_USR,UsrPortNo,false);
			return(TRUE);
		}
		else if(UmrPortNo>=0)
		{
			WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),true);
			WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,MsgOut,MsgOutHeader.MsgLen,true);
			// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
			// but that was problematic during close of the ports. 
			// Instead, try unlocking the CON port before doing any UMR port message handling.
			pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->ConPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CON,PortNo,true);
			LockPort(pport->lbaPeer->pmod,WS_UMR,UmrPortNo,false);
		#if defined(WS_OIO)||defined(WS_OTPP)
			if(WSHUmrRead(pport->lbaPeer->pmod,UmrPortNo,(char*)&MsgOutHeader,sizeof(MSGHEADER))&&
			   WSHUmrRead(pport->lbaPeer->pmod,UmrPortNo,MsgOut,MsgOutHeader.MsgLen))
		#else
			WSHRecord(pmod,&pport->lbaPeer->pmod->UmrPort[UmrPortNo].Recording,(char*)&MsgOutHeader,sizeof(MSGHEADER),false);
			WSHRecord(pmod,&pport->lbaPeer->pmod->UmrPort[UmrPortNo].Recording,MsgOut,MsgOutHeader.MsgLen,false);
			if(WSHUmrRead(pport->lbaPeer->pmod,UmrPortNo))
		#endif
			{
				while((pport->lbaPeer)&&(pport->lbaPeer->pmod->WSUmrMsgReady(UmrPortNo)))
					pport->lbaPeer->pmod->WSUmrStripMsg(UmrPortNo);
			}
			UnlockPort(pport->lbaPeer->pmod,WS_UMR,UmrPortNo,false);
			return(TRUE);
		}
	#ifdef WS_REALTIMESEND
		else
			WSHConSend(pmod,PortNo,false);
	#endif
#else
		//Send Header
		if(!WSWriteBuff(&pmod->ConPort[PortNo].OutBuffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->ConPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CON,PortNo,false);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(&pmod->ConPort[PortNo].OutBuffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->ConPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CON,PortNo,false);
				return(FALSE);
			}
		}
		pmod->ConPort[PortNo].PacketsOut++;
#endif
		pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
		pmod->ConPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CON,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->ConPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CON,PortNo,true);
		return(FALSE);
	}
}
int WsocksHostImpl::WSHConSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets)
{
	return WSHConSendBuffNew(pmod,MsgLen,MsgOut,PortNo,Packets,FALSE);
}
int WsocksHostImpl::WSHConSendBuffNew(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CON_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->ConPort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_CON,PortNo,true);
	int lastSend=pmod->ConPort[PortNo].sendThread;
	pmod->ConPort[PortNo].sendThread=GetCurrentThreadId();
	if(((pmod->ConPort[PortNo].SockConnected)||(ForceSend))
		#ifdef WS_OIO
		&&(!pmod->ConPort[PortNo].pendingClose)
		#endif
		)
	{
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if((pmod->ConPort[PortNo].OutBuffer.Busy)&&(pmod->ConPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
			{
				_ASSERT(false);
				WSHLogError(pmod,"!CRASH: CON%d OutBuffer.Busy detected a possible thread %d crash.",
					PortNo,pmod->ConPort[PortNo].OutBuffer.Busy);
				pmod->ConPort[PortNo].OutBuffer.Busy=0;
			}
			int lastBusy=pmod->ConPort[PortNo].OutBuffer.Busy;
			pmod->ConPort[PortNo].OutBuffer.Busy=pmod->ConPort[PortNo].sendThread;
#ifdef WS_LOOPBACK
			BUFFER *Buffer=&pmod->ConPort[PortNo].OutBuffer;
			int UsrPortNo=-1,UmrPortNo=-1;
			// Short circuit loopback connections
			WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
			if((pport)&&(pport->lbaPeer)&&(pport->lbaPeer->lbaBuffer))
			{
				_ASSERT((pport->lbaPeer->PortType==WS_USR)||(pport->lbaPeer->PortType==WS_UMR));
				if(pport->lbaPeer->PortType==WS_USR)
				{
					UsrPortNo=pport->lbaPeer->PortNo;
					Buffer=pport->lbaPeer->lbaBuffer;
				}
				else if(pport->lbaPeer->PortType==WS_UMR)
				{
					UmrPortNo=pport->lbaPeer->PortNo;
					Buffer=pport->lbaPeer->lbaBuffer;
				}
			}
			if(!WSWriteBuff(Buffer,MsgOut,MsgLen))
			{
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->ConPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CON,PortNo,true);
				return(FALSE);
			}
			pmod->ConPort[PortNo].PacketsOut+=Packets;
			if(UsrPortNo>=0)
			{
				pmod->ConPort[PortNo].BytesOut+=MsgLen;
				WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,MsgOut,MsgLen,true);
				// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
				// but that was problematic during close of the ports. 
				// Instead, try unlocking the CON port before doing any USR port message handling.
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->ConPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CON,PortNo,true);
				LockPort(pport->lbaPeer->pmod,WS_USR,UsrPortNo,false);
			#if defined(WS_OIO)||defined(WS_OTPP)
				if(WSHUsrRead(pport->lbaPeer->pmod,UsrPortNo,MsgOut,MsgLen))
			#else
				WSHRecord(pport->lbaPeer->pmod,&pport->lbaPeer->pmod->UsrPort[UsrPortNo].Recording,MsgOut,MsgLen,false);
				if(WSHUsrRead(pport->lbaPeer->pmod,UsrPortNo))
			#endif
				{
					while((pport->lbaPeer)&&(pport->lbaPeer->pmod->WSUsrMsgReady(UsrPortNo)))
						pport->lbaPeer->pmod->WSUsrStripMsg(UsrPortNo);
				}
				UnlockPort(pport->lbaPeer->pmod,WS_USR,UsrPortNo,false);
				return(TRUE);
			}
			else if(UmrPortNo>=0)
			{
				WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,MsgOut,MsgLen,true);
				// To prevent deadlock, I tried using the same mutex for both sides of loopback connections,
				// but that was problematic during close of the ports. 
				// Instead, try unlocking the CON port before doing any UMR port message handling.
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->ConPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CON,PortNo,true);
				LockPort(pport->lbaPeer->pmod,WS_UMR,UmrPortNo,false);
			#if defined(WS_OIO)||defined(WS_OTPP)
				if(WSHUmrRead(pport->lbaPeer->pmod,UmrPortNo,MsgOut,MsgLen))
			#else
				WSHRecord(pport->lbaPeer->pmod,&pport->lbaPeer->pmod->UmrPort[UmrPortNo].Recording,MsgOut,MsgLen,false);
				if(WSHUmrRead(pport->lbaPeer->pmod,UmrPortNo))
			#endif
				{
					while((pport->lbaPeer)&&(pport->lbaPeer->pmod->WSUmrMsgReady(UmrPortNo)))
						pport->lbaPeer->pmod->WSUmrStripMsg(UmrPortNo);
				}
				UnlockPort(pport->lbaPeer->pmod,WS_UMR,UmrPortNo,false);
				return(TRUE);
			}
	#ifdef WS_REALTIMESEND
			else if((Packets)||(ForceSend))
				WSHConSend(pmod,PortNo,false);
	#endif
#else
			if(!WSWriteBuff(&pmod->ConPort[PortNo].OutBuffer,MsgOut,MsgLen))
			{
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->ConPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CON,PortNo,true);
				return(FALSE);
			}
			pmod->ConPort[PortNo].PacketsOut+=Packets;
	#ifdef WS_REALTIMESEND
			if((Packets)||(ForceSend))
				WSHConSend(pmod,PortNo,false);
	#endif
#endif
			pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->ConPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_CON,PortNo,true);
			return(TRUE);
		}
		pmod->ConPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CON,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->ConPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CON,PortNo,true);
		return(FALSE);
	}
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseConPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CON_PORTS))
		return -1;
	if(!pmod->ConPort[PortNo].InUse)
		return -1;
	if(pmod->ConPort[PortNo].SockConnected)
		pmod->ConPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseConPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->ConPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_CON,PortNo,false);
		lastRecv=pmod->ConPort[PortNo].recvThread;
		pmod->ConPort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->ConPort[PortNo].pendingClose)
		{
			pmod->ConPort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_CON,PortNo,false);
			return 0;
		}
		pmod->ConPort[PortNo].pendingClose=true;
	}
	if(pmod->ConPort[PortNo].smutex)
	{
		LockPort(pmod,WS_CON,PortNo,true);
		lastSend=pmod->ConPort[PortNo].sendThread;
		pmod->ConPort[PortNo].sendThread=GetCurrentThreadId();
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->ConPort[PortNo].pendOvlRecvList; pmod->ConPort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->ConPort[PortNo].pendOvlSendList; pmod->ConPort[PortNo].pendOvlSendList=0;
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

	//ResetSendTimeout(WS_CON,PortNo);
	if(pmod->ConPort[PortNo].Sock != 0)
		WSHClosePort(pmod->ConPort[PortNo].Sock);
	if(pmod->ConPort[PortNo].SockConnected)
	{
		pmod->ConPort[PortNo].SockConnected=0;
		pmod->WSConClosed(PortNo); // Use this setting if you DONT HAVE LOOPED CLOSES and want to see DetPtr's in WSConClosed
	}

	WSHCloseRecording(pmod,&pmod->ConPort[PortNo].Recording,WS_CON,PortNo);
	WSCloseBuff(&pmod->ConPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->ConPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutCompBuffer);
	#endif
	memset(&pmod->ConPort[PortNo].SockOpen,0
		,sizeof(CONPORT)-(int)((char *)&pmod->ConPort[PortNo].SockOpen-(char *)&pmod->ConPort[PortNo]));
	pmod->ConPort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->ConPort[PortNo].MinReconnectDelay,pmod->ConPort[PortNo].MaxReconnectDelay,pmod->ConPort[PortNo].MinReconnectReset,
		pmod->ConPort[PortNo].ReconnectDelay,pmod->ConPort[PortNo].ReconnectTime,pmod->ConPort[PortNo].ConnectTime);
	#endif
	if(pmod->ConPort[PortNo].smutex)
	{
		pmod->ConPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CON,PortNo,true);
	}
	if(pmod->ConPort[PortNo].rmutex)
	{
		pmod->ConPort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_CON,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_CON,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseConPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CON_PORTS))
		return -1;
	if(!pmod->ConPort[PortNo].InUse)
		return -1;
	if(pmod->ConPort[PortNo].SockOpen)
		pmod->ConPort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseConPort(WsocksApp *pmod, int PortNo)
{
	pmod->WSConClosing(PortNo);
	int lastRecv=0,lastSend=0;
	if(pmod->ConPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_CON,PortNo,false);
		lastRecv=pmod->ConPort[PortNo].recvThread;
		pmod->ConPort[PortNo].recvThread=GetCurrentThreadId();
	}
	if(pmod->ConPort[PortNo].smutex)
	{
		LockPort(pmod,WS_CON,PortNo,true);
		lastSend=pmod->ConPort[PortNo].sendThread;
		pmod->ConPort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_CON,PortNo);
	if((!pmod->ConPort[PortNo].ShellPort)&&(pmod->ConPort[PortNo].Sock != 0))
	{
		SOCKET sd=((WSPort*)pmod->ConPort[PortNo].Sock)->sd;
		#ifdef WSOCKS_SSL
		if(pmod->ConPort[PortNo].Encrypted == 2 && pmod->ConPort[PortNo].Sock != 0)
		{
			SSLInfo *pSSLInfo=((WSPort*)pmod->ConPort[PortNo].Sock)->pSSLInfo;
			WSHClosePort(pmod->ConPort[PortNo].Sock); pmod->ConPort[PortNo].Sock=0;
			if(pSSLInfo)
				SSLGateway::instance()->SSLRemove(pSSLInfo);
		}
		else
		{
			WSHClosePort(pmod->ConPort[PortNo].Sock); pmod->ConPort[PortNo].Sock=0;
		}
		#else
		WSHClosePort(pmod->ConPort[PortNo].Sock);
		#endif
		WSHFinishOverlapSend(pmod,sd,&pmod->ConPort[PortNo].pendOvlSendBeg,&pmod->ConPort[PortNo].pendOvlSendEnd);
	}
	if(pmod->ConPort[PortNo].SockConnected)
	{
		pmod->ConPort[PortNo].SockConnected=0;
		pmod->WSConClosed(PortNo); // Use this setting if you DONT HAVE LOOPED CLOSES and want to see DetPtr's in WSConClosed
	}

	#ifdef WIN32
	HANDLE pthread=pmod->ConPort[PortNo].pthread;
	#else
	pthread_t pthread=pmod->ConPort[PortNo].pthread;
	#endif
	WSHCloseRecording(pmod,&pmod->ConPort[PortNo].Recording,WS_CON,PortNo);
	WSCloseBuff(&pmod->ConPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->ConPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutCompBuffer);
	#endif
	memset(&pmod->ConPort[PortNo].SockOpen,0
		,sizeof(CONPORT)-(int)((char *)&pmod->ConPort[PortNo].SockOpen-(char *)&pmod->ConPort[PortNo]));
	pmod->ConPort[PortNo].pthread=pthread;
	pmod->ConPort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->ConPort[PortNo].MinReconnectDelay,pmod->ConPort[PortNo].MaxReconnectDelay,pmod->ConPort[PortNo].MinReconnectReset,
		pmod->ConPort[PortNo].ReconnectDelay,pmod->ConPort[PortNo].ReconnectTime,pmod->ConPort[PortNo].ConnectTime);
	#endif
	if(pmod->ConPort[PortNo].smutex)
	{
		pmod->ConPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CON,PortNo,true);
	}
	if(pmod->ConPort[PortNo].rmutex)
	{
		pmod->ConPort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_CON,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_CON,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitConThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->ConPort[PortNo].pthread)
	{
	#ifdef WIN32
		#ifdef WS_OTMP
		if(pmod->pcfg->conOtmp)
		{
			pmod->ConPort[PortNo].pthread=0;
			return;
		}
		#endif
		#ifdef _DEBUG
		WaitForSingleObject(pmod->ConPort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->ConPort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->ConPort[PortNo].pthread); pmod->ConPort[PortNo].pthread=0;
	#else
		void *rc=0;
		pthread_join(pmod->ConPort[PortNo].pthread,&rc);
	#endif
	}
}
#ifdef WIN32
struct ConReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
DWORD WINAPI _BootConReadThread(LPVOID arg)
{
	ConReadThreadData *ptd=(ConReadThreadData*)arg;
	int rc=ptd->aimpl->WSHConReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	return rc;
}
#ifdef WS_OTMP
struct ConReadPoolThreadData
{
	WsocksHostImpl *aimpl;
	WsocksApp *pmod;
	THREADPOOL *tpool;
};
DWORD WINAPI _BootConReadPoolThread(LPVOID arg)
{
	ConReadPoolThreadData *ptd=(ConReadPoolThreadData*)arg;
	WsocksHostImpl *aimpl=ptd->aimpl;
	WsocksApp *pmod=ptd->pmod;
	THREADPOOL *tpool=ptd->tpool;
	delete ptd;
	int rc=aimpl->WSHConReadPoolThread(pmod,tpool);
	return rc;
}
HANDLE WsocksHostImpl::WSHPoolConReadThread(WsocksApp *pmod, WSPort *pport)
{
	THREADPOOL *tpool=conPools;
	if((!tpool)||(tpool->nports>=WS_NTMP))
	{
		tpool=new THREADPOOL;
		memset(tpool,0,sizeof(THREADPOOL));
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
		tpool->next=conPools; conPools=tpool;
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif

		DWORD tid=0;
		ConReadPoolThreadData *ptd=new ConReadPoolThreadData;
		ptd->aimpl=this;
		ptd->pmod=pmod;
		ptd->tpool=tpool;
		tpool->thnd=CreateThread(0,0,_BootConReadPoolThread,ptd,0,&tid);
		if(!tpool->thnd)
		{
			delete ptd;
			return 0;
		}
	}
	tpool->ports[tpool->nports++]=pport;
	return tpool->thnd;
}
void WsocksHostImpl::WSHWaitConThreadPoolsExit()
{
	while(conPools)
	{
		THREADPOOL *tpool=conPools;
		tpool->shutdown=true;
		WaitForSingleObject(tpool->thnd,INFINITE);
		CloseHandle(tpool->thnd); tpool->thnd=0;
		conPools=conPools->next;
		delete tpool;
	}
}
#endif//WS_OMTP
#else//!WIN32
struct ConReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
void *_BootConReadThread(void *arg)
{
	ConReadThreadData *ptd=(ConReadThreadData*)arg;
	int rc=ptd->aimpl->WSHConReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
}
#endif
#else//!WS_OIO
int WsocksHostImpl::WSHCloseConPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->ConPort[PortNo].tmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_CON,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->ConPort[PortNo].activeThread)&&(pmod->ConPort[PortNo].activeThread!=tid))
		{
			UnlockPort(pmod,WS_CON,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_CON,PortNo,false);
		}
		pmod->ConPort[PortNo].activeThread=tid;
	}
	//ResetSendTimeout(WS_CON,PortNo);
	if(pmod->ConPort[PortNo].Sock != 0)
		WSHClosePort(pmod->ConPort[PortNo].Sock);
	if(pmod->ConPort[PortNo].SockConnected)
		pmod->WSConClosed(PortNo); // Use this setting if you DONT HAVE LOOPED CLOSES and want to see DetPtr's in WSConClosed
	WSHCloseRecording(pmod,&pmod->ConPort[PortNo].Recording,WS_CON,PortNo);
	WSCloseBuff(&pmod->ConPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->ConPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutCompBuffer);
	#endif
	memset(&pmod->ConPort[PortNo].SockOpen,0
		,sizeof(CONPORT)-(int)((char *)&pmod->ConPort[PortNo].SockOpen-(char *)&pmod->ConPort[PortNo]));
	pmod->ConPort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->ConPort[PortNo].MinReconnectDelay,pmod->ConPort[PortNo].MaxReconnectDelay,pmod->ConPort[PortNo].MinReconnectReset,
		pmod->ConPort[PortNo].ReconnectDelay,pmod->ConPort[PortNo].ReconnectTime,pmod->ConPort[PortNo].ConnectTime);
	#endif
	if(pmod->ConPort[PortNo].tmutex)
	{
		pmod->ConPort[PortNo].activeThread=0;
		UnlockPort(pmod,WS_CON,PortNo,false);
	}
	//WSHPortChanged(pmod,pmod->ConPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO
int WsocksHostImpl::WSHConConnected(WsocksApp *pmod, int PortNo)
{
	WSCloseBuff(&pmod->ConPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutBuffer);
	WSOpenBuffLimit(&pmod->ConPort[PortNo].OutBuffer,pmod->ConPort[PortNo].BlockSize,pmod->ConPort[PortNo].SendBuffLimit);
	WSOpenBuffLimit(&pmod->ConPort[PortNo].InBuffer,pmod->ConPort[PortNo].BlockSize,pmod->ConPort[PortNo].RecvBuffLimit);
#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->ConPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->ConPort[PortNo].OutCompBuffer);
	WSOpenBuffLimit(&pmod->ConPort[PortNo].OutCompBuffer,pmod->ConPort[PortNo].BlockSize,pmod->ConPort[PortNo].SendBuffLimit);
	WSOpenBuffLimit(&pmod->ConPort[PortNo].InCompBuffer,pmod->ConPort[PortNo].BlockSize,pmod->ConPort[PortNo].RecvBuffLimit);
#endif
	pmod->ConPort[PortNo].SinceLastBeatCount=0;
	if (pmod->ConPort[PortNo].S5Connect)
	{
		if (pmod->ConPort[PortNo].S5Status<100)
		{
			WSHS5Login(pmod,PortNo);
			pmod->ConPort[PortNo].SockConnected=TRUE;
			pmod->ConPort[PortNo].ReconCount=0;
			goto start_read;
		}
	}
#ifdef WS_DECLINING_RECONNECT
	pmod->ConPort[PortNo].ConnectTime=GetTickCount();
#endif
	pmod->ConPort[PortNo].SockConnected=TRUE;
	pmod->ConPort[PortNo].ReconCount=0;
#ifdef WIN32
	UuidCreate((UUID*)pmod->ConPort[PortNo].Uuid);
#endif
	pmod->ConPort[PortNo].LastDataTime=GetTickCount();
	WSHUpdatePort(pmod,WS_CON,PortNo);
	if(pmod->ConPort[PortNo].DoRecOnOpen)
	{
		pmod->WSOpenRecording(&pmod->ConPort[PortNo].Recording,pmod->WShWnd,WS_CON,PortNo);
		pmod->ConPort[PortNo].DoRecOnOpen=pmod->ConPort[PortNo].Recording.DoRec;
	}
	#ifdef WSOCKS_SSL
    if(pmod->ConPort[PortNo].Encrypted==2)
    {
		#ifdef VS2010
		((WSPort*)pmod->ConPort[PortNo].Sock)->pSSLInfo=SSLGateway::instance()->ClientSetup(((WSPort*)pmod->ConPort[PortNo].Sock)->sd);
		#else
		((WSPort*)pmod->ConPort[PortNo].Sock)->pSSLInfo=SSLGateway::instance()->ClientSetup((int)((WSPort*)pmod->ConPort[PortNo].Sock)->sd);
		#endif
		if(((WSPort*)pmod->ConPort[PortNo].Sock)->pSSLInfo)
			SSLGateway::instance()->SSLConnect(((WSPort*)pmod->ConPort[PortNo].Sock)->pSSLInfo);
    }
	#endif
	pmod->WSConOpened(PortNo);

	if(pmod->ConPort[PortNo].ShellPort)
		return 0;
	if(pmod->ConPort[PortNo].S5Connect)
		return 0;
start_read:
#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
	if(!::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0))
		return FALSE;
	for(int o=0;o<WS_OVERLAP_MAX;o++)
	{
		if(WSHConIocpBegin(pmod,PortNo)<0)
			return FALSE;
	}
	#ifdef WS_LOOPBACK
	}
	#endif
#elif defined(WS_OTPP)
	WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
	#ifdef WIN32
		#ifdef WS_OTMP
		if(pmod->pcfg->conOtmp)
		{
			pmod->ConPort[PortNo].pthread=WSHPoolConReadThread(pmod,pport);
			if(!pmod->ConPort[PortNo].pthread)
			{
				WSHLogError(pmod,"CON%d: Failed assigning read thread: %d!",PortNo,GetLastError());
				_WSHCloseConPort(pmod,PortNo);
				pmod->ConPort[PortNo].ConnectHold=true;
			}
			return TRUE;
		}
		#endif
		DWORD tid=0;
		ConReadThreadData *ptd=new ConReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
		pmod->ConPort[PortNo].pthread=CreateThread(0,0,_BootConReadThread,ptd,0,&tid);
		if(!pmod->ConPort[PortNo].pthread)
		{
			WSHLogError(pmod,"CON%d: Failed creating read thread: %d!",PortNo,GetLastError());
			_WSHCloseConPort(pmod,PortNo);
			pmod->ConPort[PortNo].ConnectHold=true;
		}
	#else
		DWORD tid=0;
		ConReadThreadData *ptd=new ConReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
		pthread_create(&pmod->ConPort[PortNo].pthread,0,_BootConReadThread,ptd);
	#endif
	#ifdef WS_LOOPBACK
	}
	#endif
#endif//WS_OTPP
	return (TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHConIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	// Issue overlapped recvs
	WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->ConPort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->ConPort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->ConPort[PortNo].InBuffer.BlockSize*1024;
	povl->bytes=0;
	povl->flags=0;

	int rc=WSARecv(pport->sd,&povl->wsabuf,1,&povl->bytes,&povl->flags,povl,0);

	//#ifdef _DEBUG
	//{
	//char dbgmsg[1024]={0};
	//sprintf(dbgmsg,"%06d: DEBUG WSHConIocpBegin(9,%08x)=%d,%d\r\n",WSTime,povl,rc,WSAGetLastError());
	//OutputDebugString(dbgmsg);
	//}
	//#endif

	if((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING))
		povl->Pending=true;
	else if(!rc)
		povl->Pending=false;
	else
	{
		delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
		FreeOverlap(&pmod->ConPort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHConReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
	int tsize=pmod->ConPort[PortNo].BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->ConPort[PortNo].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->ConPort[PortNo].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->ConPort[PortNo].SockConnected)
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before lockiing the InBuffer and presenting it to the app.
		while((pmod->ConPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
		{
			#ifdef WSOCKS_SSL
			int tbytes=-1;
			if((pmod->ConPort[PortNo].Encrypted==2)&&
			   ((!pmod->ConPort[PortNo].S5Connect)||(pmod->ConPort[PortNo].S5Status>=100)))
			{
				// SSL handshake not completed on non-blocking socket, try again
				if(SSLGateway::instance()->IsActivated(pport->pSSLInfo)==false)
				{
					if(SSLGateway::instance()->SSLConnect(pport->pSSLInfo)==false)
						tbytes=0;
					else
					{
						tbytes=-1; WSASetLastError(WSAEWOULDBLOCK);
					}
				}
				else
				{
					tbytes=SSLGateway::instance()->SSLRecv(pport->pSSLInfo,tbuf,tsize);
					//if(tbytes<0)
					//	tbytes=0;
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
		pmod->ConPort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port. 200MB compressed data may inflate to 800MB uncompressed
		// data, so don't decompress if the hold limit is reached.
		if((TBuffer.Size>0)&&(pmod->ConPort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_CON,PortNo,false);
			if(pmod->ConPort[PortNo].SockConnected)
			{
				int lastRecv=pmod->ConPort[PortNo].recvThread;
				pmod->ConPort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->ConPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->ConPort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->ConPort[PortNo].InBuffer.BlockSize*1024;
					WSHConRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->ConPort[PortNo].TBufferSize=TBuffer.Size;
				pmod->ConPort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_CON,PortNo,false);
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->ConPort[PortNo].Sock))
		pmod->ConPort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#ifdef WS_OTMP
int WsocksHostImpl::WSHConReadPoolThread(WsocksApp *pmod, THREADPOOL *tpool)
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
			if(!pmod->ConPort[PortNo].Sock)
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
				tsize=pmod->ConPort[PortNo].InBuffer.BlockSize*1024;
				tbuf=new char[tsize];
				RecvBuffLimit=(DWORD)pmod->ConPort[PortNo].RecvBuffLimit*1024;
				if(!RecvBuffLimit)
					RecvBuffLimit=ULONG_MAX;
				WSOpenBuffLimit(&TBuffer,pmod->ConPort[PortNo].BlockSize,RecvBuffLimit/1024);
			}
			// Port loop
			int err=0;
			while(pmod->ConPort[PortNo].Sock)// Account for TimeTillClose
			{
				// This implementation increases throughput by reading from the socket as long as data is 
				// available (up to some limit) before locking the InBuffer and presenting it to the app.
				while((pmod->ConPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
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
				pmod->ConPort[PortNo].TBufferSize=TBuffer.Size;
				// Save what we've pulled off the port
				if((TBuffer.Size>0)&&(pmod->ConPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					LockPort(pmod,WS_CON,PortNo,false);
					if(pmod->ConPort[PortNo].Sock)
					{
						int lastRecv=pmod->ConPort[PortNo].recvThread;
						pmod->ConPort[PortNo].recvThread=tid;
						while((TBuffer.Size>0)&&(pmod->ConPort[PortNo].InBuffer.Size<RecvBuffLimit))
						{
							DWORD rsize=TBuffer.LocalSize;
							if(rsize>pmod->ConPort[PortNo].InBuffer.BlockSize*1024)
								rsize=pmod->ConPort[PortNo].InBuffer.BlockSize*1024;
							WSHConRead(pmod,PortNo,TBuffer.Block,rsize);
							if(!WSStripBuff(&TBuffer,rsize))
							{
								err=WSAENOBUFS; _ASSERT(false);
								break;
							}
						}
						pmod->ConPort[PortNo].TBufferSize=TBuffer.Size;
						pmod->ConPort[PortNo].recvThread=lastRecv;
					}
					else
						err=WSAECONNABORTED;
					UnlockPort(pmod,WS_CON,PortNo,false);
				}
				// This loop is kept a 'while' instead of 'if' to preserve 
				// 'break' statements above, but it really functions like an 'if'
				// so we can handle the next port
				break; 
			}
			// One-time cleanup
			if((err)&&(err!=WSAEWOULDBLOCK))
			{
				if((err!=WSAECONNABORTED)&&(pmod->ConPort[PortNo].Sock))
					pmod->ConPort[PortNo].peerClosed=true;
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
int WsocksHostImpl::WSHConRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	pmod->ConPort[i].LastDataTime=GetTickCount();
	pmod->ConPort[i].BytesIn+=bytes;
	pmod->ConPort[i].BlocksIn++;

#ifdef WS_LOOPBACK
	// Loopback short-circuit
	WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
	if((pport)&&(pport->lbaPeer))
	{
		while(pport->lbaBuffer->Size>0)
		{
			unsigned int size=pport->lbaBuffer->LocalSize;
			if(size>pmod->ConPort[i].InBuffer.BlockSize*1024)
				size=pmod->ConPort[i].InBuffer.BlockSize*1024;
			if(!WSWriteBuff(&pmod->ConPort[i].InBuffer,pport->lbaBuffer->Block,size))
				return(FALSE);
			WSStripBuff(pport->lbaBuffer,size);
		}
		return(TRUE);
	}
#endif
	// SOCKS5 protocol
	if((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status>=10)&&(pmod->ConPort[i].S5Status<100))
	{
		WSWriteBuff(&pmod->ConPort[i].InBuffer,(char*)buf,bytes);
		bool s5err=false;
		switch (pmod->ConPort[i].S5Status)
		{
		case 10:
			if(pmod->ConPort[i].InBuffer.LocalSize>=2)
			{
				int Len=2;
				int Version=pmod->ConPort[i].InBuffer.Block[0];
				int Methode=pmod->ConPort[i].InBuffer.Block[1];
				//pmod->WSLogEvent("SOCKS5 DEBUG 10: Received Version=%d, EncryptMethod=%d",Version,Methode);// BAOHACK
				if ((Version!=pmod->ConPort[i].S5Version)||(Methode!=pmod->ConPort[i].S5Methode))
				{
					s5err=true;
					break;
				}
				WSStripBuff(&pmod->ConPort[i].InBuffer, Len);
				WSHS5Connect(pmod,i);
				break;
			}
			break;
		case 20:
			if(pmod->ConPort[i].InBuffer.LocalSize>=4)
			{
				unsigned int Len=4;
				int Version=pmod->ConPort[i].InBuffer.Block[0];
				int Reply=pmod->ConPort[i].InBuffer.Block[1];
				int AdressType=pmod->ConPort[i].InBuffer.Block[3];
				//pmod->WSLogEvent("SOCKS5 DEBUG 20: Received Version=%d, Reply=%d, AddressType=%d",Version,Reply,AdressType);// BAOHACK
				switch(AdressType)
				{
				case 1:
					Len+=4; break;
				default:
					s5err=true;
					break;
				};
				// AddPort Len
				Len+=2;
				if(pmod->ConPort[i].InBuffer.LocalSize<Len)
					break;
				WSStripBuff(&pmod->ConPort[i].InBuffer, Len);
				pmod->ConPort[i].S5Status=100;
				WSHConConnected(pmod,i);
				break;
			}
			break;
		};
		if(s5err)
		{
			WSHCloseConPort(pmod,i);
			WSHLogError(pmod,"Socks5 Failed on CON%d",i);
		}
	}
#ifdef WS_COMPRESS
	// Decompression
	else if((pmod->ConPort[i].Compressed))//||(pmod->ConPort[i].CompressType>0))
	{
		WSHWaitMutex(0x01,INFINITE);
		char *szTemp=pmod->ConRead_szTemp;
		char *szDecompBuff=pmod->ConRead_szDecompBuff;
		#ifdef WS_ENCRYPTED
		char *szDecryptBuff=pmod->ConRead_szDecryptBuff;
		unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
		#endif
		char *PxySrc=pmod->ConRead_PxySrc;

		if(!WSWriteBuff(&pmod->ConPort[i].InCompBuffer,(char*)buf,bytes))
		{
			WSHReleaseMutex(0x01);
			return(FALSE);
		}
GetNextBlock:
		if(pmod->ConPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
		{
			unsigned int *CompSize=(unsigned int*)pmod->ConPort[i].InCompBuffer.Block,CompType=*CompSize;
			unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);
			#ifdef BIT64
			unsigned long lCompSize=*CompSize;
			unsigned long lDecompSize=DecompSize;
			#endif

			//WS_PROXYCOMPRESS
			int HeadLen=sizeof(unsigned int);
			memset(PxySrc,0xFF,PROXYCOMPRESS_MAX);
			if(CompType==0xFFFFFFFF)
			{
				WSHReleaseMutex(0x01);
				WSHLogError(pmod,"CON%d to %s:%d no longer supports 0xFFFFFFFF C1 compression. Halting port!",
					i,pmod->ConPort[i].RemoteIP,pmod->ConPort[i].Port);
				WSHCloseConPort(pmod,i);
				pmod->ConPort[i].ConnectHold=true;
				return FALSE;
			}
			// C2 compression
			else if(CompType==0xFEFEFEFE)
			{
				if(pmod->ConPort[i].InCompBuffer.LocalSize>=8)
				{
					CompSize=(unsigned int*)(pmod->ConPort[i].InCompBuffer.Block+4);
					HeadLen=8;
					memset(PxySrc,0xFE,PROXYCOMPRESS_MAX);
					if(pmod->ConPort[i].InCompBuffer.BlockSize*2*1024<(*CompSize)+HeadLen)
					{
						WSHLogError(pmod,"CON%d block size(%dK) is too small for packet size %d. Halting port!",
							i,pmod->ConPort[i].InCompBuffer.BlockSize,(*CompSize)+HeadLen);
						WSHCloseConPort(pmod,i);
						pmod->ConPort[i].ConnectHold=true;
						return FALSE;
					}
				}
			}
			// C3 compression 0xFCFCFCFC
			// C4 compression
			else if(CompType==0xFBFBFBFB)
			{
				if(pmod->ConPort[i].InCompBuffer.LocalSize>=8)
				{
					CompSize=(unsigned int*)(pmod->ConPort[i].InCompBuffer.Block+4);
					HeadLen=8;
					memset(PxySrc,0xFB,PROXYCOMPRESS_MAX);
					if(pmod->ConPort[i].InCompBuffer.BlockSize*2*1024<(*CompSize)+HeadLen)
					{
						WSHLogError(pmod,"CON%d block size(%dK) is too small for packet size %d. Halting port!",
							i,pmod->ConPort[i].InCompBuffer.BlockSize,(*CompSize)+HeadLen);
						WSHCloseConPort(pmod,i);
						pmod->ConPort[i].ConnectHold=true;
						return FALSE;
					}
				}
			}

			if(pmod->ConPort[i].InCompBuffer.LocalSize>=((*CompSize)+HeadLen))
			{
				// No zlib decompression necessary for C4
				if(CompType==0xFBFBFBFB)
				{
					memcpy(szDecompBuff,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize);
					DecompSize=*CompSize;
					WSHRecord(pmod,&pmod->ConPort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->ConPort[i].InCompBuffer,((*CompSize)+HeadLen));
					// The app callback must call WSWriteBuff for InBuffer
					if(!pmod->WSDecompressed(WS_CON,i,szDecompBuff,DecompSize,PxySrc))
					{
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
				else
				{
				#ifdef WS_ENCRYPTED
					// Decryption
					if(pmod->ConPort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize,i,WS_CON);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize,szDecryptBuff,DecryptSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
				#else
					if(pmod->ConPort[i].CompressType==2)
					{
						if(csuncompress(szDecompBuff,&DecompSize,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],CompSize))
						{
							WSHReleaseMutex(0x01);
							WSHCloseConPort(pmod,i);
							return(FALSE);
						}
					}
					else if(uncompress(szDecompBuff,&DecompSize,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize))
					{
						_ASSERT(false);
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
				#endif
					WSHRecord(pmod,&pmod->ConPort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->ConPort[i].InCompBuffer,((*CompSize)+HeadLen));
					// The app callback must call WSWriteBuff for InBuffer
					if(!pmod->WSDecompressed(WS_CON,i,szDecompBuff,DecompSize,PxySrc))
					{
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
			}
		}
		WSHReleaseMutex(0x01);
	}
	else
#endif//WS_COMPRESS
	{
		WSHRecord(pmod,&pmod->ConPort[i].Recording,buf,bytes,false);
		if(!WSWriteBuff(&pmod->ConPort[i].InBuffer,(char*)buf,bytes))
		{
			WSHCloseConPort(pmod,i);
			return(FALSE);
		}
	}
	return TRUE;
}
#else//!WS_OIO
int WsocksHostImpl::WSHConRead(WsocksApp *pmod, int PortNo)
{
	char *szTemp=pmod->ConRead_szTemp;
#ifdef WS_COMPRESS
	char *szDecompBuff=pmod->ConRead_szDecompBuff;
#ifdef WS_ENCRYPTED
	char *szDecryptBuff=pmod->ConRead_szDecryptBuff;
	unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
#endif
	char *PxySrc=pmod->ConRead_PxySrc;
#endif
	int status;             // Status Code 
	int i=PortNo;

	if((pmod->ConPort[i].InBuffer.Busy)&&(pmod->ConPort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: CON%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->ConPort[i].InBuffer.Busy);
		pmod->ConPort[i].InBuffer.Busy=0;
	}
	pmod->ConPort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common ConRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
#ifdef WS_LOOPBACK
	WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
	if((pport)&&(pport->lbaPeer))
	{
		while(pport->lbaBuffer->Size>0)
		{
			unsigned int size=pport->lbaBuffer->LocalSize;
			if(size>pmod->ConPort[i].InBuffer.BlockSize*1024)
				size=pmod->ConPort[i].InBuffer.BlockSize*1024;
			if(!WSWriteBuff(&pmod->ConPort[i].InBuffer,pport->lbaBuffer->Block,size))
			{
				WSHReleaseMutex(0x01);
				pmod->ConPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
			WSStripBuff(pport->lbaBuffer,size);
		}
		WSHReleaseMutex(0x01);
		pmod->ConPort[i].InBuffer.Busy=lastBusy;
		return(TRUE);
	}
#endif
#ifdef WS_COMPRESS
	if(pmod->ConPort[i].Compressed&&(!((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status<100))))
	{
		status = WSHRecv(pmod->ConPort[i].Sock, szTemp, ((pmod->ConPort[i].BlockSize-1)*1024), NO_FLAGS_SET );
		if (status>0) 
		{
			pmod->ConPort[i].LastDataTime=GetTickCount();
			pmod->ConPort[i].BytesIn+=status;
			pmod->ConPort[i].BlocksIn++;

			if(!WSWriteBuff(&pmod->ConPort[i].InCompBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				pmod->ConPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
GetNextBlock:
			if(pmod->ConPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
			{
				unsigned int *CompSize=(unsigned int*)pmod->ConPort[i].InCompBuffer.Block;
				unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);
			#ifdef BIT64
				unsigned long lCompSize=*CompSize;
				unsigned long lDecompSize=DecompSize;
			#endif

				//WS_PROXYCOMPRESS
				int HeadLen=sizeof(unsigned int);
				memset(PxySrc,0xFF,PROXYCOMPRESS_MAX);
				if(*CompSize==0xFFFFFFFF)
				{
					WSHReleaseMutex(0x01);
					WSHLogError(pmod,"CON%d to %s:%d no longer supports 0xFFFFFFFF C1 compression. Halting port!",
						i,pmod->ConPort[i].RemoteIP,pmod->ConPort[i].Port);
					WSHCloseConPort(pmod,i);
					pmod->ConPort[i].ConnectHold=true;
					return FALSE;
				}
				else if(*CompSize==0xFEFEFEFE)
				{
					if(pmod->ConPort[i].InCompBuffer.LocalSize>=8)
					{
						CompSize=(unsigned int*)(pmod->ConPort[i].InCompBuffer.Block+4);
						HeadLen=8;
						memset(PxySrc,0xFE,PROXYCOMPRESS_MAX);
						if(pmod->ConPort[i].InCompBuffer.BlockSize*2*1024<(*CompSize)+HeadLen)
						{
							WSHLogError(pmod,"CON%d block size(%dK) is too small for packet size %d. Halting port!",
								i,pmod->ConPort[i].InCompBuffer.BlockSize,(*CompSize)+HeadLen);
							WSHCloseConPort(pmod,i);
							pmod->ConPort[i].ConnectHold=true;
							return FALSE;
						}
					}
				}
				if(pmod->ConPort[i].InCompBuffer.LocalSize>=((*CompSize)+HeadLen))
				{
		#ifdef WS_ENCRYPTED
					if(pmod->ConPort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize,i,WS_CON);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize,szDecryptBuff,DecryptSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
		#else
					if(pmod->ConPort[i].CompressType==2)
					{
						if(csuncompress(szDecompBuff,&DecompSize,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],CompSize))
						{
							WSHReleaseMutex(0x01);
							WSHCloseConPort(pmod,i);
							return(FALSE);
						}
					}
					else if(uncompress(szDecompBuff,&DecompSize,&pmod->ConPort[i].InCompBuffer.Block[HeadLen],*CompSize))
					{
						_ASSERT(false);
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
		#endif
					WSHRecord(pmod,&pmod->ConPort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->ConPort[i].InCompBuffer,((*CompSize)+HeadLen));
					// The app callback must call WSWriteBuff for InBuffer
					if(!pmod->WSDecompressed(WS_CON,i,szDecompBuff,DecompSize,PxySrc))
					{
						WSHReleaseMutex(0x01);
						WSHCloseConPort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
			}
			pmod->ConPort[i].InBuffer.Busy=lastBusy;
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->ConPort[i].InBuffer.Size>0)
			{
				while(pmod->WSConMsgReady(i))
					pmod->WSConStripMsg(i);
			}
			WSHCloseConPort(pmod,i);
			return(FALSE);
		}
	}
	else
#endif//WS_COMPRESS
	{
		status = WSHRecv(pmod->ConPort[i].Sock, szTemp, ((pmod->ConPort[i].BlockSize-1)*1024), NO_FLAGS_SET );

		if (status>0) 
		{
    		pmod->ConPort[i].LastDataTime=GetTickCount();
			pmod->ConPort[i].BytesIn+=status;
			if(status==132||status==8)
			{
				if(*((WORD *)szTemp)!=16)
				{
					pmod->ConPort[i].BlocksIn++;
				}
			}
			else
			{
				pmod->ConPort[i].BlocksIn++;
			}
			WSHRecord(pmod,&pmod->ConPort[i].Recording,szTemp,status,false);
			if(!WSWriteBuff(&pmod->ConPort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseConPort(pmod,i);
				return(FALSE);
			}
			pmod->ConPort[i].InBuffer.Busy=lastBusy;
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->ConPort[i].InBuffer.Size>0)
			{
				while(pmod->WSConMsgReady(i))
					pmod->WSConStripMsg(i);
			}
			WSHCloseConPort(pmod,i);
			return(FALSE);
		}
	}
	WSHReleaseMutex(0x01);
	pmod->ConPort[i].InBuffer.Busy=lastBusy;
	return(TRUE);
}
#endif//!WS_OIO

void WsocksHostImpl::SetReconnectTime(DWORD MinReconnectDelay, DWORD MaxReconnectDelay, DWORD MinReconnectReset,
									  DWORD& ReconnectDelay, DWORD& ReconnectTime, DWORD& ConnectTime)
{
	DWORD tnow=GetTickCount();
	// We've stayed connected long enough to reset to minimum reconnect delay
	if((ConnectTime)&&((int)(tnow -ConnectTime)>=MinReconnectReset))
	{
		ReconnectDelay=MinReconnectDelay;
		ReconnectTime=tnow +ReconnectDelay;
		ConnectTime=0;
	}
	// Connection failed or not stayed connected long enough, so increase the delay
	else
	{
		if(ReconnectDelay>0)
		{
			ReconnectDelay*=2;
			if(ReconnectDelay>MaxReconnectDelay)
				ReconnectDelay=MaxReconnectDelay;
		}
		else
			ReconnectDelay=MinReconnectDelay;
		ReconnectTime=tnow +ReconnectDelay;
		ConnectTime=0;
	}
}

#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHSendNonBlock(WsocksApp *pmod, int PortType, int PortNo, SOCKET_T socket, LPVOID buffer, DWORD bytesToWrite)
{
	WSPort *pport=(WSPort *)socket;
	switch(PortType)
	{
	case WS_CON:
	{
		#ifdef WSOCKS_SSL
		if((pmod->ConPort[PortNo].Encrypted==2)&&((!pmod->ConPort[PortNo].S5Connect)||(pmod->ConPort[PortNo].S5Status>=100)))
			return SSLGateway::instance()->SSLSend(pport->pSSLInfo,(char*)buffer,bytesToWrite);
		#endif
		if(pmod->ConPort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
	#ifdef WIN32
		WSOVERLAPPED *povl=AllocOverlap(&pmod->ConPort[PortNo].pendOvlSendBeg,&pmod->ConPort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->ConPort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=false;
		povl->Cancelled=false;
		povl->RecvOp=0;		
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASend(pport->sd,&povl->wsabuf,1,&povl->bytes,0,(LPWSAOVERLAPPED)povl,0);
	//#ifdef _DEBUG
	//	char dbuf[256]={0};
	//	sprintf(dbuf,"WSHSendNonBlock(%x,%d)=>%d,%d\r\n",povl,povl->wsabuf.len,rc,WSAGetLastError());
	//	OutputDebugString(dbuf);
	//#endif
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			pmod->ConPort[PortNo].IOCPSendBytes+=bytesToWrite;
			if(rc<0)
				pmod->ConPort[PortNo].SendTimeOut=GetTickCount();
			return bytesToWrite;
		}
		// Usually WSAECONNRESET or WSAENOBUFS
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->ConPort[PortNo].pendOvlSendBeg,&pmod->ConPort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		DWORD tsent=0;
		while(tsent<bytesToWrite)
		{
			int rc=send(pport->sd,(char *)buffer +tsent,bytesToWrite -tsent,0);
			if(rc<1)
				break;
			tsent+=rc;
		}
		return tsent;
	#endif
	}
	case WS_USR:
	{
		#ifdef WSOCKS_SSL
		if(pmod->UscPort[pmod->UsrPort[PortNo].UscPort].Encrypted==2)
			return SSLGateway::instance()->SSLSend(pport->pSSLInfo,(char*)buffer,bytesToWrite);
		#endif
		if(pmod->UsrPort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
	#ifdef WIN32
		WSOVERLAPPED *povl=AllocOverlap(&pmod->UsrPort[PortNo].pendOvlSendBeg,&pmod->UsrPort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->UsrPort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=false;
		povl->Cancelled=false;
		povl->RecvOp=0;
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASend(pport->sd,&povl->wsabuf,1,&povl->bytes,0,(LPWSAOVERLAPPED)povl,0);
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			return bytesToWrite;
		}
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->UsrPort[PortNo].pendOvlSendBeg,&pmod->UsrPort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		DWORD tsent=0;
		while(tsent<bytesToWrite)
		{
			int rc=send(pport->sd,(char *)buffer +tsent,bytesToWrite -tsent,0);
			if(rc<1)
				break;
			tsent+=rc;
		}
		return tsent;
	#endif
	}
#ifdef WS_GUARANTEED
	case WS_CGD:
	{
		if(pmod->CgdPort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
	#ifdef WIN32
		WSOVERLAPPED *povl=AllocOverlap(&pmod->CgdPort[PortNo].pendOvlSendBeg,&pmod->CgdPort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->CgdPort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=false;
		povl->Cancelled=false;
		povl->RecvOp=0;
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASend(pport->sd,&povl->wsabuf,1,&povl->bytes,0,(LPWSAOVERLAPPED)povl,0);
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			return bytesToWrite;
		}
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->CgdPort[PortNo].pendOvlSendBeg,&pmod->CgdPort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		DWORD tsent=0;
		while(tsent<bytesToWrite)
		{
			int rc=send(pport->sd,(char *)buffer +tsent,bytesToWrite -tsent,0);
			if(rc<1)
				break;
			tsent+=rc;
		}
		return tsent;
	#endif
	}
	case WS_UGR:
	{
		if(pmod->UgrPort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
	#ifdef WIN32
		WSOVERLAPPED *povl=AllocOverlap(&pmod->UgrPort[PortNo].pendOvlSendBeg,&pmod->UgrPort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->UgrPort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=false;
		povl->Cancelled=false;
		povl->RecvOp=0;
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASend(pport->sd,&povl->wsabuf,1,&povl->bytes,0,(LPWSAOVERLAPPED)povl,0);
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			return bytesToWrite;
		}
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->UgrPort[PortNo].pendOvlSendBeg,&pmod->UgrPort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		DWORD tsent=0;
		while(tsent<bytesToWrite)
		{
			int rc=send(pport->sd,(char *)buffer +tsent,bytesToWrite -tsent,0);
			if(rc<1)
				break;
			tsent+=rc;
		}
		return tsent;
	#endif
	}
#endif
#ifdef WS_FILE_SERVER
	case WS_FIL:
	{
		if(pmod->FilePort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
	#ifdef WIN32
		WSOVERLAPPED *povl=AllocOverlap(&pmod->FilePort[PortNo].pendOvlSendBeg,&pmod->FilePort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->FilePort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=false;
		povl->Cancelled=false;
		povl->RecvOp=0;
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASend(pport->sd,&povl->wsabuf,1,&povl->bytes,0,(LPWSAOVERLAPPED)povl,0);
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			return bytesToWrite;
		}
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->FilePort[PortNo].pendOvlSendBeg,&pmod->FilePort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		DWORD tsent=0;
		while(tsent<bytesToWrite)
		{
			int rc=send(pport->sd,(char *)buffer +tsent,bytesToWrite -tsent,0);
			if(rc<1)
				break;
			tsent+=rc;
		}
		return tsent;
	#endif
	}
#endif
#ifdef WS_MONITOR
	case WS_UMR:
	{
		if(pmod->UmrPort[PortNo].NumOvlSends>=32)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
	#ifdef WIN32
		WSOVERLAPPED *povl=AllocOverlap(&pmod->UmrPort[PortNo].pendOvlSendBeg,&pmod->UmrPort[PortNo].pendOvlSendEnd);
		if(!povl)
		{
			WSASetLastError(WSAENOBUFS);
			return -1;
		}
		pmod->UmrPort[PortNo].NumOvlSends++;
		povl->PortType=PortType;
		povl->PortNo=PortNo;
		povl->Pending=false;
		povl->Cancelled=false;
		povl->RecvOp=0;
		povl->buf=new char[bytesToWrite];
		povl->wsabuf.buf=povl->buf;
		povl->wsabuf.len=bytesToWrite;
		memcpy(povl->buf,buffer,bytesToWrite);
		int rc=WSASend(pport->sd,&povl->wsabuf,1,&povl->bytes,0,(LPWSAOVERLAPPED)povl,0);
		if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
		{
			povl->Pending=true;
			return bytesToWrite;
		}
		else
		{
			delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
			FreeOverlap(&pmod->UmrPort[PortNo].pendOvlSendBeg,&pmod->UmrPort[PortNo].pendOvlSendEnd,povl);
			return SOCKET_ERROR;
		}
	#else
		DWORD tsent=0;
		while(tsent<bytesToWrite)
		{
			int rc=send(pport->sd,(char *)buffer +tsent,bytesToWrite -tsent,0);
			if(rc<1)
				break;
			tsent+=rc;
		}
		return tsent;
	#endif
	}
#endif
	default:
		WSASetLastError(WSAESOCKTNOSUPPORT);
		return SOCKET_ERROR;
	};
}
#endif//WS_OIO
#ifdef WS_OTPP
int WsocksHostImpl::WSHFinishOverlapSend(WsocksApp *pmod, SOCKET socket, WSOVERLAPPED **pendOvlBeg, WSOVERLAPPED **pendOvlEnd)
{
	if((!pmod)||(!socket)||(!pendOvlBeg)||(!pendOvlEnd))
		return -1;
#ifdef WIN32
	// Check all pending operations because completion signals can be out of order
	WSOVERLAPPED *pbeg=*pendOvlBeg,*pend=*pendOvlEnd;
	*pendOvlBeg=*pendOvlEnd=0;
	int errPortType=-1,errPortNo=-1;
	for(WSOVERLAPPED *povl=pbeg;povl;)
	{
		int PortNo=povl->PortNo;
		DWORD bytes=0,flags=0;
		BOOL rc=WSAGetOverlappedResult(socket,povl,&bytes,false,&flags);
		int lerr=WSAGetLastError();
		// Still in progress
		if((!rc)&&(lerr==ERROR_IO_INCOMPLETE))
		{
			povl=povl->next;
			continue;
		}
		// Completed (rc=TRUE, bytes>0) or cancelled (WSAENOTSOCK or ERROR_OPERATION_ABORTED)
		else
		{
			WSOVERLAPPED *dovl=povl; povl=povl->next;
			// If the thread that initiated an overlapped send exits, 
			// then all of it's overlapped operations are cancelled.
			if((!rc)&&(lerr))
			{
				errPortType=dovl->PortType;
				errPortNo=dovl->PortNo;
			}
			switch(dovl->PortType)
			{
			case WS_CON:
				if(pmod->ConPort[PortNo].NumOvlSends>0)
					pmod->ConPort[PortNo].NumOvlSends--;
				if(bytes>pmod->ConPort[PortNo].IOCPSendBytes)
					pmod->ConPort[PortNo].IOCPSendBytes=0;
				else
					pmod->ConPort[PortNo].IOCPSendBytes-=bytes;
				break;
			case WS_USR:
				if(pmod->UsrPort[PortNo].NumOvlSends>0)
					pmod->UsrPort[PortNo].NumOvlSends--;
				if(bytes>pmod->UsrPort[PortNo].IOCPSendBytes)
					pmod->UsrPort[PortNo].IOCPSendBytes=0;
				else
					pmod->UsrPort[PortNo].IOCPSendBytes-=bytes;
				break;
		#ifdef WS_GUARANTEED
			case WS_CGD:
				if(pmod->CgdPort[PortNo].NumOvlSends>0)
					pmod->CgdPort[PortNo].NumOvlSends--;
				if(bytes>pmod->CgdPort[PortNo].IOCPSendBytes)
					pmod->CgdPort[PortNo].IOCPSendBytes=0;
				else
					pmod->CgdPort[PortNo].IOCPSendBytes-=bytes;
				break;
			case WS_UGR:
				if(pmod->UgrPort[PortNo].NumOvlSends>0)
					pmod->UgrPort[PortNo].NumOvlSends--;
				if(bytes>pmod->UgrPort[PortNo].IOCPSendBytes)
					pmod->UgrPort[PortNo].IOCPSendBytes=0;
				else
					pmod->UgrPort[PortNo].IOCPSendBytes-=bytes;
				break;
		#endif
		#ifdef WS_FILE_SERVER
			case WS_FIL:
				if(pmod->FilePort[PortNo].NumOvlSends>0)
					pmod->FilePort[PortNo].NumOvlSends--;
				if(bytes>pmod->FilePort[PortNo].IOCPSendBytes)
					pmod->FilePort[PortNo].IOCPSendBytes=0;
				else
					pmod->FilePort[PortNo].IOCPSendBytes-=bytes;
				break;
		#endif
		#ifdef WS_MONITOR
			case WS_UMR:
				if(pmod->UmrPort[PortNo].NumOvlSends>0)
					pmod->UmrPort[PortNo].NumOvlSends--;
				if(bytes>pmod->UmrPort[PortNo].IOCPSendBytes)
					pmod->UmrPort[PortNo].IOCPSendBytes=0;
				else
					pmod->UmrPort[PortNo].IOCPSendBytes-=bytes;
				break;
		#endif
		#ifdef WS_CAST
			case WS_CTO:
				if(pmod->CtoPort[PortNo].NumOvlSends>0)
					pmod->CtoPort[PortNo].NumOvlSends--;
				if(bytes>pmod->CtoPort[PortNo].IOCPSendBytes)
					pmod->CtoPort[PortNo].IOCPSendBytes=0;
				else
					pmod->CtoPort[PortNo].IOCPSendBytes-=bytes;
				break;
		#endif
			};
			delete dovl->buf; dovl->wsabuf.buf=dovl->buf=0; dovl->wsabuf.len=0;
			FreeOverlap(&pbeg,&pend,dovl);
		#ifdef _DEBUG
			if((!pbeg)&&(dovl->PortType==WS_CON))
				_ASSERT(pmod->ConPort[PortNo].NumOvlSends==0);
		#endif
		}
	}

	// We detected cancelled overlapped operations caused by thread exit
	if((errPortType>=0)&&(errPortNo>=0))
	{
		switch(errPortType)
		{
		case WS_CON: 
			if((!pmod->ConPort[errPortNo].peerClosed)&&(!pmod->ConPort[errPortNo].appClosed))
			{
				WSHLogEvent(pmod,"WSOCKS: CON%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseConPort(pmod,errPortNo); // Purposely not _WSHCloseConPort
				return -1;
			}
			break;
		case WS_USR:
			if((!pmod->UsrPort[errPortNo].peerClosed)&&(!pmod->UsrPort[errPortNo].appClosed))
			{
				WSHLogEvent(pmod,"WSOCKS: USR%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseUsrPort(pmod,errPortNo); // Purposely not _WSHCloseUsrPort
				return -1;
			}
			break;
	#ifdef WS_GUARANTEED
		case WS_CGD:
			if((!pmod->CgdPort[errPortNo].peerClosed)&&(!pmod->CgdPort[errPortNo].appClosed))
			{
				WSHLogEvent(pmod,"WSOCKS: CGD%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseCgdPort(pmod,errPortNo); // Purposely not _WSHCloseCgdPort
				return -1;
			}
			break;
		case WS_UGR:
			if((!pmod->UgrPort[errPortNo].peerClosed)&&(!pmod->UgrPort[errPortNo].appClosed))
			{
				WSHLogEvent(pmod,"WSOCKS: UGR%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseUgrPort(pmod,errPortNo); // Purposely not _WSHCloseUgrPort
				return -1;
			}
			break;
	#endif
	#ifdef WS_FILE_SERVER
		case WS_FIL:
			if((!pmod->FilePort[errPortNo].peerClosed)&&(!pmod->FilePort[errPortNo].appClosed))
			{
				WSHLogEvent(pmod,"WSOCKS: FIL%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);
				WSHCloseFilePort(pmod,errPortNo); // Purposely not _WSHCloseFilePort
				return -1;
			}
			break;
	#endif
	#ifdef WS_MONITOR
		case WS_UMR:
			if((!pmod->UmrPort[errPortNo].peerClosed)&&(!pmod->UmrPort[errPortNo].appClosed))
			{
				WSHLogEvent(pmod,"WSOCKS: UMR%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseUmrPort(pmod,errPortNo); // Purposely not _WSHCloseUmrPort
				return -1;
			}
			break;
	#endif
	#ifdef WS_CAST
		case WS_CTO:
			if(!pmod->CtoPort[errPortNo].appClosed)
			{
				WSHLogEvent(pmod,"WSOCKS: CTO%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseCtoPort(pmod,errPortNo); // Purposely not _WSHCloseCtoPort
				return -1;
			}
			break;
		case WS_CTI:
			if(!pmod->CtiPort[errPortNo].appClosed)
			{
				WSHLogEvent(pmod,"WSOCKS: CTI%d: A thread that sent on this port has exited before send completion! Closing port.",errPortNo);			
				WSHCloseCtiPort(pmod,errPortNo); // Purposely not _WSHCloseCtoPort
				return -1;
			}
			break;
	#endif
		};
	}
	*pendOvlBeg=pbeg; *pendOvlEnd=pend;
#else
	// Not doing overlapped I/O on Linux
#endif
	return 0;
}
#endif

int WsocksHostImpl::WSHConSend(WsocksApp *pmod, int PortNo, bool flush)
{
	int nsent=0;
	// Socks5 protocol exchange not compressed
	bool socks5=false,compressed=false;
	if((pmod->ConPort[PortNo].S5Connect)&&(pmod->ConPort[PortNo].S5Status>=10)&&(pmod->ConPort[PortNo].S5Status<100))
		socks5=true;
	else if(!pmod->ConPort[PortNo].SockConnected)
		return -1;

	#ifdef WSOCKS_SSL
	// Wait for SSL handshake to complete before sending anything
	if((pmod->ConPort[PortNo].Encrypted==2)&&(!socks5))
	{
		if(SSLGateway::instance()->IsActivated(((WSPort*)pmod->ConPort[PortNo].Sock)->pSSLInfo)==false)
			return 0;
	}
	#endif

	int lastBusy=pmod->ConPort[PortNo].OutBuffer.Busy;
	pmod->ConPort[PortNo].OutBuffer.Busy=pmod->ConPort[PortNo].sendThread;
	//pmod->ConPort[PortNo].SendTimeOut=0;
	BUFFER *sendBuffer=&pmod->ConPort[PortNo].OutBuffer;
#ifdef WS_COMPRESS
	if((!socks5)&&((pmod->ConPort[PortNo].Compressed)||(pmod->ConPort[PortNo].CompressType>0)))
	{
		compressed=true;
		sendBuffer=&pmod->ConPort[PortNo].OutCompBuffer;
		// Compress as much as possible to fill one send block
		while((pmod->ConPort[PortNo].OutBuffer.Size>0)&&
			  ((flush)||((int)pmod->ConPort[PortNo].OutBuffer.Size>=pmod->ConPort[PortNo].ImmediateSendLimit)))
		{
			unsigned int CompSize=pmod->ConPort[PortNo].OutBuffer.LocalSize;
			if(CompSize>((WS_COMP_BLOCK_LEN*1024)*99/100))
				CompSize=((WS_COMP_BLOCK_LEN*1024)*99/100);
			if(CompSize>0)
			{
				WSHWaitMutex(0x01,INFINITE);
				char *CompBuff=pmod->SyncLoop_CompBuff;
				unsigned int CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
				mtcompress(CompBuff,&CompBuffSize,pmod->ConPort[PortNo].OutBuffer.Block,CompSize);
				// Compression +encryption
				#ifdef WS_ENCRYPTED
				char *EncryptBuff=CompBuff;
				unsigned int EncryptSize=CompBuffSize;
				if(pmod->ConPort[PortNo].EncryptionOn)
				{
					EncryptBuff=pmod->SyncLoop_EncryptBuff;
					EncryptSize=sizeof(pmod->SyncLoop_EncryptBuff);
					WSHEncrypt(pmod,EncryptBuff,&EncryptSize,CompBuff,CompBuffSize,PortNo,WS_CON);
				}
				WSWriteBuff(sendBuffer,(char*)&EncryptSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,EncryptBuff,EncryptSize);
				//Optimized for compression only
				#else//!WS_ENCRYPTED
				WSWriteBuff(sendBuffer,(char*)&CompBuffSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,CompBuff,CompBuffSize);
				#endif//WS_ENCRYPTED
				WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,pmod->ConPort[PortNo].OutBuffer.Block,CompSize,true);
				WSStripBuff(&pmod->ConPort[PortNo].OutBuffer,CompSize);
				WSHReleaseMutex(0x01);
			}
		}
	}
#endif//WS_COMPRESS
	// Send as much as possible or up to the choke limit
	while((sendBuffer->Size>0)&&((flush)||(compressed)||((int)sendBuffer->Size>=pmod->ConPort[PortNo].ImmediateSendLimit)))
	{
		int size=sendBuffer->LocalSize;
		if((pmod->ConPort[PortNo].ChokeSize>0)&&(pmod->ConPort[PortNo].ChokeSize -(int)pmod->ConPort[PortNo].LastChokeSize<size))
		{
			size=(pmod->ConPort[PortNo].ChokeSize-pmod->ConPort[PortNo].LastChokeSize);
			if(size<=0)
				break;
		}
		if(size>pmod->ConPort[PortNo].BlockSize*1024)
			size=pmod->ConPort[PortNo].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->ConPort[PortNo].Sock;
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=WSHSendNonBlock(pmod,WS_CON,PortNo,(SOCKET_T)pport,sendBuffer->Block,size);
		#else
		int rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_CON_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->ConPort[PortNo].SendTimeOut)
				{					
					pmod->ConPort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->ConPort[PortNo].SendTimeOut<WS_CON_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: CON%d [%s] Reset by Peer",PortNo,pmod->ConPort[PortNo].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: CON%d [%s] Send Failed: %d",PortNo,pmod->ConPort[PortNo].CfgNote,lerr);
				WSHCloseConPort(pmod,PortNo);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		if(!compressed)
			WSHRecord(pmod,&pmod->ConPort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->ConPort[PortNo].BytesOut+=rc;
		pmod->ConPort[PortNo].BlocksOut++;
		pmod->ConPort[PortNo].SendTimeOut=0;
		pmod->ConPort[PortNo].LastChokeSize+=rc;
		nsent+=rc;
	}
	pmod->ConPort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}
