
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
#include "wstring.h"
#include <sys/stat.h>

#ifdef WS_FILE_SERVER
#pragma pack(push,8)
#ifdef WS_FILE_SERVER
//598: Request File
typedef struct tdMsg598Rec
{
    char MsgVersion;                // 1,2
    char FilePath[MAX_PATH];        // Virtual path
    DWORD FileSize;                 // Current size
    FILETIME FileCreateTime;        // Current create date
    FILETIME FileModifyTime;        // Current modify date
    char FileVersion[8];            // Match version
    int op;                         // 1-CreateFile, 2-DeleteFile, 
                                    // 3-FindFirstFile, 4-FindNextFile, 5-FindClose
                                    // 6-PathFileExists, 7-PathIsDirectory
    unsigned int dwDesiredAccess;  // Use CreateFile symantics
    unsigned int dwShareMode;
    unsigned int dwCreationDisposition;
    unsigned int dwFlagsAndAttributes;
    union
	{
		int MatchFlags;
		struct
		{
			unsigned short MatchSize  : 1; //Match by size
			unsigned short MatchCDate : 1; //Match by creation date
			unsigned short MatchMDate : 1; //Match by modify date
			unsigned short MatchVer   : 1; //Match by file version
		};
	};

    char User[16];                  // Security credentials
    char PassWord[16];
    HANDLE vhnd;                    // Virtual handle
    int kbps;                       // How fast we want it
} MSG598REC;

//599: Request File Response
typedef struct tdMsg599Rec
{
    char MsgVersion;                // 1,2
    int FileOffset;                 // Offset in file of packet data
    int FileSize;                   // Total file size
    char DataType;                  // 1-start, 2-data, 3-status,
                                    // 4,5-findfirst, 6-exists|isdir
	u_int FileHandle;				// Virtual file handle
    union
	{
        struct // DataType=0x01     // Starts file transfer
        {
            char FilePath[MAX_PATH];
            WIN32_FIND_DATAA fdata;
			BOOL uptodate; // v2 only
        };
        struct // DataType=0x02     // Transfer file data
        {
            int DataLen;
            char FileData[1024];
        };
        struct // DataType=0x03     // Error code for ACKs
        {
            int ErrorCode;
            char ErrorStr[256];
			BOOL uptodate2; // v2 only
        };
        struct // DataType=0x04     // FindFirst/FindNext data
        {
            int eof;
            WIN32_FIND_DATAA fdata2;
        };
        struct // DataType=0x06     // File existence or file is dir
        {
            char FilePath2[MAX_PATH];
            int exists;
        };
	};
} MSG599REC;
#endif //#ifdef WS_FILE_SERVER
#pragma pack(pop)

int WsocksHostImpl::WSHSetupFilePorts(WsocksApp *pmod, int SMode, int PortNo)
{ 
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
		StartPort=0; EndPort=pmod->NO_OF_FILE_PORTS;
	}
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=StartPort;i<=EndPort;i++)
		{
			memset(&pmod->FilePort[i],0,sizeof(FILEPORT));
			pmod->FilePort[i].BlockSize=WS_DEF_BLOCK_SIZE;
		#ifdef WS_DECLINING_RECONNECT
			pmod->FilePort[i].MinReconnectDelay=MIN_RECONNECT_DELAY;
			pmod->FilePort[i].MaxReconnectDelay=MAX_RECONNECT_DELAY;
			pmod->FilePort[i].MinReconnectReset=MIN_RECONNECT_RESET;
			pmod->FilePort[i].ReconnectDelay=0;
			pmod->FilePort[i].ReconnectTime=0;
			pmod->FilePort[i].ConnectTime=0;
		#endif
			pmod->FilePort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
			pmod->FilePort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		}
		break;
	case WS_OPEN: // Open
		for (i=StartPort;i<=EndPort;i++)
		{
			if(strlen(pmod->FilePort[i].LocalIP)>0)
			{
				if(pmod->FilePort[i].InUse)
					continue;
				pmod->FilePort[i].rmutex=CreateMutex(0,false,0);
				pmod->FilePort[i].smutex=CreateMutex(0,false,0);
				pmod->FilePort[i].recvThread=0;
				pmod->FilePort[i].sendThread=0;
				WSOpenBuffLimit(&pmod->FilePort[i].OutBuffer,pmod->FilePort[i].BlockSize,pmod->FilePort[i].SendBuffLimit);
				WSOpenBuffLimit(&pmod->FilePort[i].InBuffer,pmod->FilePort[i].BlockSize,pmod->FilePort[i].RecvBuffLimit);
			#ifdef WS_COMPRESS
				WSOpenBuffLimit(&pmod->FilePort[i].OutCompBuffer,pmod->FilePort[i].BlockSize,pmod->FilePort[i].SendBuffLimit);
				WSOpenBuffLimit(&pmod->FilePort[i].InCompBuffer,pmod->FilePort[i].BlockSize,pmod->FilePort[i].RecvBuffLimit);
			#endif
				pmod->FilePort[i].InUse=TRUE;
				//WSHCreatePort(pmod,WS_FIL,i);
				//AddConListItem(GetDispItem(WS_FIL,i));
			}
		}
		//if(!Single)
		//	AddConListItem(GetDispItem(WS_FIL_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=StartPort;i<=EndPort;i++)
		{
			WSHCloseRecording(pmod,&pmod->FilePort[i].Recording,WS_FIL,i);
			_WSHCloseFilePort(pmod,i);
			pmod->FilePort[i].recvThread=0;
			pmod->FilePort[i].sendThread=0;
			if(pmod->FilePort[i].smutex)
			{
				DeleteMutex(pmod->FilePort[i].smutex); pmod->FilePort[i].smutex=0;
			}
			if(pmod->FilePort[i].rmutex)
			{
				DeleteMutex(pmod->FilePort[i].rmutex); pmod->FilePort[i].rmutex=0;
			}
			if(pmod->FilePort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_FIL,i));
				pmod->FilePort[i].InUse=FALSE;
			}
			#ifdef WS_OTPP
			_WSHWaitFileThreadExit(pmod,i);
			#endif
		}
		break;
	}
	return(TRUE);
}
int WsocksHostImpl::WSHOpenFilePort(WsocksApp *pmod, int PortNo)
{ 
#ifdef WS_DECLINING_RECONNECT
	if((pmod->FilePort[PortNo].ReconnectTime)&&(GetTickCount()<pmod->FilePort[PortNo].ReconnectTime))
		return FALSE;
#endif

	// Don't even create a socket if none of the IPs are active
	bool enabled=false;
	for(int i=0;i<pmod->FilePort[PortNo].AltIPCount;i++)
	{
		if(pmod->FilePort[PortNo].AltRemoteIPOn[i])
		{
			enabled=true;
			break;
		}
	}
	if(!enabled)
	{
	#ifdef WS_DECLINING_RECONNECT
		pmod->FilePort[PortNo].ReconnectTime=0;
	#endif
		return FALSE;
	}

	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->FilePort[PortNo].BlockSize*8192;

	struct
	{
		int l_onoff;
		int l_linger;
	} linger;
	
	//pmod->FilePort[PortNo].Sock = socket(AF_INET, SOCK_STREAM, 0);
	pmod->FilePort[PortNo].Sock = WSHSocket(pmod,WS_FIL,PortNo);
	if (pmod->FilePort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->FilePort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket socket() failed with error=%d",errno);
		perror(NULL);
		return(FALSE);
	}
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->FilePort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->FilePort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket ioctlsocket() failed");
		_WSHCloseFilePort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->FilePort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->FilePort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket setsockopt() failed");
		_WSHCloseFilePort(pmod,PortNo);
		return(FALSE);
	}

	linger.l_onoff=1;
	linger.l_linger=0;
	if (WSHSetSockOpt(pmod->FilePort[PortNo].Sock, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof(linger)) == SOCKET_ERROR) 
	{
		pmod->FilePort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket setsockopt() failed");
		_WSHCloseFilePort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->FilePort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->FilePort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket setsockopt() failed");
		_WSHCloseFilePort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->FilePort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->FilePort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket setsockopt() failed");
		_WSHCloseFilePort(pmod,PortNo);
		return(FALSE);
	}

	// There's no evidence this helps
//#ifdef WS_REALTIMESEND
//	// Disable Nagle
//	unsigned int nagleoff=1;
//	if (WSHSetSockOpt(pmod->FilePort[PortNo].Sock, IPPROTO_TCP, TCP_NODELAY, (char *)(&nagleoff), sizeof(nagleoff)) == SOCKET_ERROR) 
//	{
//		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : FileSocket setsockopt(TCP_NODELAY) failed");
//		_WSHCloseFilePort(pmod,PortNo);
//		return(FALSE);
//	}
//#endif

	// Connect sockets will automatically bind an unused port. Otherwise, when the dest is
	// loopback or a local interface, it may accidentally bind the dest port.
	if (strcmp(pmod->FilePort[PortNo].LocalIP,"AUTO")!=0)
	{
		// Retrieve the local IP address and TCP Port number
		local_sin.sin_family=AF_INET;
		local_sin.sin_port=INADDR_ANY;
		local_sin.sin_addr.s_addr=inet_addr(pmod->FilePort[PortNo].LocalIP);

		//  Associate an address with a socket. (bind)
		//if (bind( pmod->FilePort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
		if (WSHBindPort( pmod->FilePort[PortNo].Sock, &local_sin.sin_addr, INADDR_ANY) == SOCKET_ERROR) 
		{
			pmod->FilePort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : ConSocket bind() failed");
			_WSHCloseFilePort(pmod,PortNo);
			return(FALSE);
		}
	}
	WSHFileConnect(pmod,PortNo);
	pmod->FilePort[PortNo].SockOpen=TRUE;
	return(TRUE);
}
int WsocksHostImpl::WSHFileSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{ 
	MSGHEADER MsgOutHeader;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_FILE_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->FilePort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	MsgOutHeader.MsgID=MsgID;
	MsgOutHeader.MsgLen=MsgLen;
	LockPort(pmod,WS_FIL,PortNo,true);
	int lastSend=pmod->FilePort[PortNo].sendThread;
	pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();
	if(pmod->FilePort[PortNo].SockConnected)
	{
		if((pmod->FilePort[PortNo].OutBuffer.Busy)&&(pmod->FilePort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: FIL%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->FilePort[PortNo].OutBuffer.Busy);
		}
		int lastBusy=pmod->FilePort[PortNo].OutBuffer.Busy;
		pmod->FilePort[PortNo].OutBuffer.Busy=pmod->FilePort[PortNo].sendThread;
		//Send Header
		if(!WSWriteBuff(&pmod->FilePort[PortNo].OutBuffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->FilePort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_FIL,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(&pmod->FilePort[PortNo].OutBuffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->FilePort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_FIL,PortNo,true);
				return(FALSE);
			}
		}
		pmod->FilePort[PortNo].PacketsOut++;
	#ifdef WS_REALTIMESEND
		WSHFileSend(pmod,PortNo,false);
	#endif
		pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
		pmod->FilePort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_FIL,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->FilePort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_FIL,PortNo,true);
		return(FALSE);
	}
}
int WsocksHostImpl::WSHFileSendBuffNew(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend)
{ 
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_FILE_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->FilePort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_FIL,PortNo,true);
	int lastSend=pmod->FilePort[PortNo].sendThread;
	pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->FilePort[PortNo].SockConnected)||(ForceSend))
	{
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if((pmod->FilePort[PortNo].OutBuffer.Busy)&&(pmod->FilePort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
			{
				_ASSERT(false);
				WSHLogError(pmod,"!CRASH: FIL%d OutBuffer.Busy detected a possible thread %d crash.",
					PortNo,pmod->FilePort[PortNo].OutBuffer.Busy);
				pmod->FilePort[PortNo].OutBuffer.Busy=0;
			}
			int lastBusy=pmod->FilePort[PortNo].OutBuffer.Busy;
			pmod->FilePort[PortNo].OutBuffer.Busy=pmod->FilePort[PortNo].sendThread;
			if(!WSWriteBuff(&pmod->FilePort[PortNo].OutBuffer,MsgOut,MsgLen))
			{
				pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->FilePort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_FIL,PortNo,true);
				return(FALSE);
			}
			pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
		}
		pmod->FilePort[PortNo].PacketsOut+=Packets;
	#ifdef WS_REALTIMESEND
		if((Packets)||(ForceSend))
			WSHFileSend(pmod,PortNo,false);
	#endif
		pmod->FilePort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_FIL,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->FilePort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_FIL,PortNo,true);
		return(FALSE);
	}
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseFilePort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->FilePort[PortNo].InUse)
		return -1;
	if(pmod->FilePort[PortNo].SockConnected)
		pmod->FilePort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseFilePort(WsocksApp *pmod, int PortNo)
{ 
	int lastRecv=0,lastSend=0;
	if(pmod->FilePort[PortNo].rmutex)
	{
		LockPort(pmod,WS_FIL,PortNo,false);
		lastRecv=pmod->FilePort[PortNo].recvThread;
		pmod->FilePort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->FilePort[PortNo].pendingClose)
		{
			pmod->FilePort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_FIL,PortNo,false);
			return 0;
		}
		pmod->FilePort[PortNo].pendingClose=true;
	}
	if(pmod->FilePort[PortNo].smutex)
	{
		LockPort(pmod,WS_FIL,PortNo,true);
		lastSend=pmod->FilePort[PortNo].sendThread;
		pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->FilePort[PortNo].pendOvlRecvList; pmod->FilePort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->FilePort[PortNo].pendOvlSendList; pmod->FilePort[PortNo].pendOvlSendList=0;
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

	//ResetSendTimeout(WS_FIL,PortNo);
	if(pmod->FilePort[PortNo].Sock != 0)
	{
		WSHClosePort(pmod->FilePort[PortNo].Sock);
	}
	WSHCloseRecording(pmod,&pmod->FilePort[PortNo].Recording,WS_FIL,PortNo);
	WSCloseBuff(&pmod->FilePort[PortNo].InBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->FilePort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutCompBuffer);
	#endif
    // Fail all non-terminated transfers on the closed port
    for ( WSFileTransfer *wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->PortNo == PortNo )
        {
            if(wft->status != WSF_SUCCESS &&
               wft->status != WSF_FAILED &&
               wft->status != WSF_CANCELLED &&
               wft->status != WSF_TIMEOUT )
            {
                wft->status = WSF_FAILED;
                if(wft->progWnd)
                    PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
            }
        }
    }
	if(pmod->FilePort[PortNo].SockConnected)
	{
		pmod->FilePort[PortNo].SockConnected=0;
	    pmod->WSFileClosed(PortNo); // Use this setting if you DONT HAVE LOOPED CLOSES and want to see DetPtr's in WSFileClosed
	}
    memset(&pmod->FilePort[PortNo].SockOpen,0
		,sizeof(FILEPORT)-(int)((char *)&pmod->FilePort[PortNo].SockOpen-(char *)&pmod->FilePort[PortNo]));
	pmod->FilePort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->FilePort[PortNo].MinReconnectDelay,pmod->FilePort[PortNo].MaxReconnectDelay,pmod->FilePort[PortNo].MinReconnectReset,
		pmod->FilePort[PortNo].ReconnectDelay,pmod->FilePort[PortNo].ReconnectTime,pmod->FilePort[PortNo].ConnectTime);
	#endif
	if(pmod->FilePort[PortNo].smutex)
	{
		pmod->FilePort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_FIL,PortNo,true);
	}
	if(pmod->FilePort[PortNo].rmutex)
	{
		pmod->FilePort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_FIL,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_FIL,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseFilePort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->FilePort[PortNo].InUse)
		return -1;
	if(pmod->FilePort[PortNo].SockConnected)
		pmod->FilePort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseFilePort(WsocksApp *pmod, int PortNo)
{ 
	int lastRecv=0,lastSend=0;
	if(pmod->FilePort[PortNo].rmutex)
	{
		LockPort(pmod,WS_FIL,PortNo,false);
		lastRecv=pmod->FilePort[PortNo].recvThread;
		pmod->FilePort[PortNo].recvThread=GetCurrentThreadId();
	}
	if(pmod->FilePort[PortNo].smutex)
	{
		LockPort(pmod,WS_FIL,PortNo,true);
		lastSend=pmod->FilePort[PortNo].sendThread;
		pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_FIL,PortNo);
	if(pmod->FilePort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->FilePort[PortNo].Sock)->sd;
		WSHClosePort(pmod->FilePort[PortNo].Sock);
		WSHFinishOverlapSend(pmod,sd,&pmod->FilePort[PortNo].pendOvlSendBeg,&pmod->FilePort[PortNo].pendOvlSendEnd);
	}
	WSHCloseRecording(pmod,&pmod->FilePort[PortNo].Recording,WS_FIL,PortNo);
	WSCloseBuff(&pmod->FilePort[PortNo].InBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->FilePort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutCompBuffer);
	#endif
    // Fail all non-terminated transfers on the closed port
    for ( WSFileTransfer *wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->PortNo == PortNo )
        {
            if(wft->status != WSF_SUCCESS &&
               wft->status != WSF_FAILED &&
               wft->status != WSF_CANCELLED &&
               wft->status != WSF_TIMEOUT )
            {
                wft->status = WSF_FAILED;
                if(wft->progWnd)
                    PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
            }
        }
    }
	if(pmod->FilePort[PortNo].SockConnected)
	{
		pmod->FilePort[PortNo].SockConnected=0;
	    pmod->WSFileClosed(PortNo); // Use this setting if you DONT HAVE LOOPED CLOSES and want to see DetPtr's in WSFileClosed
	}

	HANDLE pthread=pmod->FilePort[PortNo].pthread;
	memset(&pmod->FilePort[PortNo].SockOpen,0
		,sizeof(FILEPORT)-(int)((char *)&pmod->FilePort[PortNo].SockOpen-(char *)&pmod->FilePort[PortNo]));
	pmod->FilePort[PortNo].pthread=pthread;
	pmod->FilePort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->FilePort[PortNo].MinReconnectDelay,pmod->FilePort[PortNo].MaxReconnectDelay,pmod->FilePort[PortNo].MinReconnectReset,
		pmod->FilePort[PortNo].ReconnectDelay,pmod->FilePort[PortNo].ReconnectTime,pmod->FilePort[PortNo].ConnectTime);
	#endif
	if(pmod->FilePort[PortNo].smutex)
	{
		pmod->FilePort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_FIL,PortNo,true);
	}
	if(pmod->FilePort[PortNo].rmutex)
	{
		pmod->FilePort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_FIL,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_FIL,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitFileThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->FilePort[PortNo].pthread)
	{
		#ifdef _DEBUG
		WaitForSingleObject(pmod->FilePort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->FilePort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->FilePort[PortNo].pthread); pmod->FilePort[PortNo].pthread=0;
	}
}
int WsocksHostImpl::WSHFileReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->FilePort[PortNo].Sock;
	int tsize=pmod->FilePort[PortNo].InBuffer.BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->FilePort[PortNo].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->FilePort[PortNo].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->FilePort[PortNo].SockConnected)
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before lockiing the InBuffer and presenting it to the app.
		while((pmod->FilePort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
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
		pmod->FilePort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port
		if((TBuffer.Size>0)&&(pmod->FilePort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_FIL,PortNo,false);
			if(pmod->FilePort[PortNo].SockConnected)
			{
				int lastRecv=pmod->FilePort[PortNo].recvThread;
				pmod->FilePort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->FilePort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->FilePort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->FilePort[PortNo].InBuffer.BlockSize*1024;
					WSHFileRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->FilePort[PortNo].TBufferSize=TBuffer.Size;
				pmod->FilePort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_FIL,PortNo,false);
		}
		// Single-threaded apps need WSFileMsgReady called here when the progress dialog is up
		if(!pmod->pcfg->asyncMode)
		{
			HANDLE progwnd=0;
			for(WSFileTransfer *wft=fileTransfers;wft;wft=wft->next)
			{
				if(wft->progWnd)
				{
					progwnd=wft->progWnd;
					break;
				}
			}
		#ifdef WS_FILE_SMPROXY
			// No dialog up, but transfer in progress
			if(pmod->FilePort[PortNo].DetPtr5)
			{
				WSFileTransfer2 *wft=(WSFileTransfer2 *)pmod->FilePort[PortNo].DetPtr5;
				progwnd=wft->doneEvent;
				if(LockPort(pmod,WS_FIL,PortNo,true,50)==WAIT_OBJECT_0)
				{
					WSHFileSend(pmod,PortNo,true);
					UnlockPort(pmod,WS_FIL,PortNo,true);
				}
			}
		#endif
			while(progwnd)
			{
				// We must lock, because WSHSyncLoop or WSHAsyncLoop normally call WSHFileMsgReady
				if(LockPort(pmod,WS_FIL,PortNo,false,50)!=WAIT_OBJECT_0)
					break;
				if(!pmod->FilePort[PortNo].InUse)
				{
					UnlockPort(pmod,WS_FIL,PortNo,false);
					break;
				}
				pmod->FilePort[PortNo].recvThread=tid;

				while(WSHFileMsgReady(pmod,PortNo))
					WSHFileStripMsg(pmod,PortNo);

				pmod->FilePort[PortNo].recvThread=0;
				UnlockPort(pmod,WS_FIL,PortNo,false);
				break;
			}
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->FilePort[PortNo].Sock))
		pmod->FilePort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#else//!WS_OIO
int WsocksHostImpl::WSHCloseFilePort(WsocksApp *pmod, int PortNo)
{ 
	if(pmod->FilePort[PortNo].rmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_FIL,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->FilePort[PortNo].recvThread)&&(pmod->FilePort[PortNo].recvThread!=tid))
		{
			UnlockPort(pmod,WS_FIL,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_FIL,PortNo,false);
		}
		pmod->FilePort[PortNo].recvThread=tid;
	}
	//ResetSendTimeout(WS_FIL,PortNo);
	if(pmod->FilePort[PortNo].Sock != 0)
		WSHClosePort(pmod->FilePort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->FilePort[PortNo].Recording,WS_FIL,PortNo);
	WSCloseBuff(&pmod->FilePort[PortNo].InBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->FilePort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutCompBuffer);
	#endif
    // Fail all non-terminated transfers on the closed port
    for ( WSFileTransfer *wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->PortNo == PortNo )
        {
            if(wft->status != WSF_SUCCESS &&
               wft->status != WSF_FAILED &&
               wft->status != WSF_CANCELLED &&
               wft->status != WSF_TIMEOUT )
            {
                wft->status = WSF_FAILED;
                if(wft->progWnd)
                    PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
            }
        }
    }
	if(pmod->FilePort[PortNo].SockConnected)
	    pmod->WSFileClosed(PortNo); // Use this setting if you DONT HAVE LOOPED CLOSES and want to see DetPtr's in WSFileClosed
    memset(&pmod->FilePort[PortNo].SockOpen,0
		,sizeof(FILEPORT)-(int)((char *)&pmod->FilePort[PortNo].SockOpen-(char *)&pmod->FilePort[PortNo]));
	pmod->FilePort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->FilePort[PortNo].MinReconnectDelay,pmod->FilePort[PortNo].MaxReconnectDelay,pmod->FilePort[PortNo].MinReconnectReset,
		pmod->FilePort[PortNo].ReconnectDelay,pmod->FilePort[PortNo].ReconnectTime,pmod->FilePort[PortNo].ConnectTime);
	#endif
	if(pmod->FilePort[PortNo].rmutex)
	{
		pmod->FilePort[PortNo].recvThread=0;
		UnlockPort(pmod,WS_FIL,PortNo,false);
	}
	//WSHPortChanged(pmod,pmod->FilePort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO

#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHFileRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	pmod->FilePort[i].LastDataTime=GetTickCount();
	pmod->FilePort[i].BytesIn+=bytes;
	pmod->FilePort[i].BlocksIn++;

	// SOCKS5 protocol
	if((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status>=10)&&(pmod->FilePort[i].S5Status<100))
	{
		_ASSERT(false);//untested
		bool s5err=false;
		switch (pmod->FilePort[i].S5Status)
		{
		case 10:
			if(pmod->FilePort[i].InBuffer.LocalSize>=2)
			{
				int Len=2;
				int Version=pmod->FilePort[i].InBuffer.Block[0];
				int Methode=pmod->FilePort[i].InBuffer.Block[1];
				if ((Version!=pmod->FilePort[i].S5Version)||(Methode!=pmod->FilePort[i].S5Methode))
				{
					s5err=true;
					break;
				}
				WSStripBuff(&pmod->FilePort[i].InBuffer, Len);
				WSHS5Connect(pmod,i);
				break;
			}
			break;
		case 20:
			if(pmod->FilePort[i].InBuffer.LocalSize>=4)
			{
				unsigned int Len=4;
				int Version=pmod->FilePort[i].InBuffer.Block[0];
				int Reply=pmod->FilePort[i].InBuffer.Block[1];
				int AdressType=pmod->FilePort[i].InBuffer.Block[3];
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
				if(pmod->FilePort[i].InBuffer.LocalSize<Len)
					break;
				WSStripBuff(&pmod->FilePort[i].InBuffer, Len);
				pmod->FilePort[i].S5Status=100;
				WSHFileConnected(pmod,i);
				break;
			}
			break;
		};
		if(s5err)
		{
			WSHCloseFilePort(pmod,i);
			WSHLogError(pmod,"Socks5 Failed on FIL%d",i);
		}
	}
#ifdef WS_COMPRESS
	// Decompression
	if(pmod->FilePort[i].Compressed&&(!((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status<100))))
	{
		WSHWaitMutex(0x01,INFINITE);
		char *szTemp=pmod->FileRead_szTemp;
		char *szDecompBuff=pmod->FileRead_szDecompBuff;
	#ifdef WS_ENCRYPTED
		char *szDecryptBuff=pmod->FileRead_szDecryptBuff;
		unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
	#endif
		if(!WSWriteBuff(&pmod->FilePort[i].InCompBuffer,(char*)buf,bytes))
		{
			WSHReleaseMutex(0x01);
			return(FALSE);
		}
GetNextBlock:
		if(pmod->FilePort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
		{
			unsigned int *CompSize=(unsigned int *)pmod->FilePort[i].InCompBuffer.Block;
			unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

			if(pmod->FilePort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
			{
			#ifdef WS_ENCRYPTED
				// Decryption
				if(pmod->FilePort[i].EncryptionOn)
				{
					WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
						,&pmod->FilePort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_FIL);
				}
				else
				{
					memcpy(szDecryptBuff,&pmod->FilePort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
					DecryptSize=*CompSize;
				}
				if(uncompress(szDecompBuff,&DecompSize
					,szDecryptBuff,DecryptSize))
			#else
				if(uncompress(szDecompBuff,&DecompSize
					,&pmod->FilePort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
			#endif
				{
					WSHReleaseMutex(0x01);
					WSHCloseFilePort(pmod,i);
					return(FALSE);
				}
				WSHRecord(pmod,&pmod->FilePort[i].Recording,szDecompBuff,DecompSize,false);
				WSStripBuff(&pmod->FilePort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
				if(!WSWriteBuff(&pmod->FilePort[i].InBuffer,szDecompBuff,DecompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseFilePort(pmod,i);
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
		WSHRecord(pmod,&pmod->FilePort[i].Recording,(char*)buf,bytes,false);
		if(!WSWriteBuff(&pmod->FilePort[i].InBuffer,(char*)buf,bytes))
		{
			WSHCloseFilePort(pmod,i);
			return(FALSE);
		}
	}
	return TRUE;
}
#else//!WS_OIO
int WsocksHostImpl::WSHFileRead(WsocksApp *pmod, int PortNo)
{
	//static char szTemp[(WS_MAX_BLOCK_SIZE*1024)];
	char *szTemp=pmod->FileRead_szTemp;
#ifdef WS_COMPRESS
	//static char szDecompBuff[(WS_MAX_BLOCK_SIZE*1024)];
	char *szDecompBuff=pmod->FileRead_szDecompBuff;
#ifdef WS_ENCRYPTED
	//static char szDecryptBuff[(WS_MAX_BLOCK_SIZE*1024)];
	char *szDecryptBuff=pmod->FileRead_szDecryptBuff;
	unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
#endif
#endif
	int status;             // Status Code 
	int i=PortNo;

	if((pmod->FilePort[i].InBuffer.Busy)&&(pmod->FilePort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: FIL%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->FilePort[i].InBuffer.Busy);
		pmod->FilePort[i].InBuffer.Busy=0;
	}
	pmod->FilePort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common FileRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
#ifdef WS_COMPRESS
	if(pmod->FilePort[i].Compressed&&(!((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status<100))))
	{
		status = WSHRecv(pmod->FilePort[i].Sock, szTemp, ((pmod->FilePort[i].BlockSize-1)*1024), NO_FLAGS_SET );
		if (status>0) 
		{
			pmod->FilePort[i].LastDataTime=GetTickCount();
			pmod->FilePort[i].BytesIn+=status;
			pmod->FilePort[i].BlocksIn++;

			if(!WSWriteBuff(&pmod->FilePort[i].InCompBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				pmod->FilePort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
GetNextBlock:
			if(pmod->FilePort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
			{
				unsigned int *CompSize=(unsigned int *)pmod->FilePort[i].InCompBuffer.Block;
				unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

				if(pmod->FilePort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
				{
#ifdef WS_ENCRYPTED
					if(pmod->FilePort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->FilePort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_FIL);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->FilePort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize
						,szDecryptBuff,DecryptSize))
#else
					if(uncompress(szDecompBuff,&DecompSize
						,&pmod->FilePort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
#endif
					{
						WSHReleaseMutex(0x01);
						WSHCloseFilePort(pmod,i);
						return(FALSE);
					}
					WSHRecord(pmod,&pmod->FilePort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->FilePort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
					if(!WSWriteBuff(&pmod->FilePort[i].InBuffer,szDecompBuff,DecompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseFilePort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
			}
			pmod->FilePort[i].InBuffer.Busy=lastBusy;
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->FilePort[i].InBuffer.Size>0)
			{
				while(pmod->WSFileMsgReady(i))
					pmod->WSFileStripMsg(i);
			}
			WSHCloseFilePort(pmod,i);
			return(FALSE);
		}
	}
	else
#endif
	{
		status = WSHRecv(pmod->FilePort[i].Sock, szTemp, ((pmod->FilePort[i].BlockSize-1)*1024), NO_FLAGS_SET );

		if (status>0) 
		{
    		pmod->FilePort[i].LastDataTime=GetTickCount();
			pmod->FilePort[i].BytesIn+=status;
			if(status==132||status==8)
			{
				if(*((WORD *)szTemp)!=16)
				{
					pmod->FilePort[i].BlocksIn++;
				}
			}
			else
			{
				pmod->FilePort[i].BlocksIn++;
			}
			WSHRecord(pmod,&pmod->FilePort[i].Recording,szTemp,status,false);
			if(!WSWriteBuff(&pmod->FilePort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseFilePort(pmod,i);
				return(FALSE);
			}
			pmod->FilePort[i].InBuffer.Busy=lastBusy;
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->FilePort[i].InBuffer.Size>0)
			{
				while(pmod->WSFileMsgReady(i))
					pmod->WSFileStripMsg(i);
			}
			WSHCloseFilePort(pmod,i);
			return(FALSE);
		}
	}
	WSHReleaseMutex(0x01);
	pmod->FilePort[i].InBuffer.Busy=lastBusy;
	return(TRUE);
}
#endif//!WS_OIO

int WsocksHostImpl::WSHBeforeFileSend(WsocksApp *pmod, int PortNo)
{
	return(0);
}
int WsocksHostImpl::WSHFileMsgReady(WsocksApp *pmod, int PortNo)
{
	//MSGHEADER MsgHeader;

	//if(pmod->FilePort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
	//{
	//	return(0);
	//}
	//memcpy((char *)&MsgHeader,pmod->FilePort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	//if(pmod->FilePort[PortNo].InBuffer.Size<(sizeof(MSGHEADER)+MsgHeader.MsgLen))
	//{
	//	return(0);
	//}
	//return(1);
	return WSHRecvFileTransfer(pmod,PortNo);
}
int WsocksHostImpl::WSHFileStripMsg(WsocksApp *pmod, int PortNo)
{
	//MSGHEADER *MsgHeader;
	//char *Msg;

	//MsgHeader = (MSGHEADER*) malloc(sizeof(MSGHEADER));
	//memcpy((char *)MsgHeader,pmod->FilePort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	//WSStripBuff(&pmod->FilePort[PortNo].InBuffer,sizeof(MSGHEADER));
	//Msg = (char*) malloc(MsgHeader->MsgLen);
	//memcpy(Msg,pmod->FilePort[PortNo].InBuffer.Block,MsgHeader->MsgLen);
	//WSStripBuff(&pmod->FilePort[PortNo].InBuffer,MsgHeader->MsgLen);
	//pmod->FilePort[PortNo].PacketsIn++;

	//switch(MsgHeader->MsgID)
	//{
	//case 16:
	//    pmod->FilePort[PortNo].PacketsIn-=1;
	//    pmod->FilePort[PortNo].BeatsIn++;
 //       break;
	//// File transfer
 //   case 599:
	//{
 //       break;
	//}
	//default:
	//	_ASSERT(false);
 //   }
	//free(Msg);
	//free(MsgHeader);
	return(0);
}

int WsocksHostImpl::WSHMakeLocalDirs(WsocksApp *pmod, const char *lpath)
{
    for ( const char *lptr=lpath; lptr; )
    {
		#ifdef WIN32
        const char *nptr = strchr(lptr, '\\');
		#else
        const char *nptr = strchr(lptr, '/');
		#endif
        if ( !nptr )
	{
	    nptr=strchr(lptr,'/');
	    if(!nptr)
                break;
	}
        char dpath[MAX_PATH];
        memset(dpath, 0, sizeof(dpath));
        memcpy(dpath, lpath, nptr -lpath);
        CreateDirectory(dpath, 0);
        lptr = nptr +1;
    }
    return 0;
}
// Determine a local location from virtual path
int WsocksHostImpl::WSHGetFileLocalPath(WsocksApp *pmod, const char *root, const char *rpath, char *lpath, BOOL makeDirs)
{
    char tpath[MAX_PATH];
	memset(tpath, 0, sizeof(tpath));
    char *tname = 0;
    GetFullPathNameEx(rpath, sizeof(tpath), tpath, &tname);
    int tlen = (int)strlen(tpath);
    if ( tpath[1] == ':' )
    {
        //MoveMemory(tpath, tpath +2, tlen -1);
        tpath[1] = toupper(tpath[0]); tpath[0] = '/';
    }

	#ifdef WIN32
    sprintf(lpath, "%s\\%s%s", pmod->pcfg->RunPath.c_str(), root, tpath);    
	#else
    sprintf(lpath, "%s/%s%s", pmod->pcfg->RunPath.c_str(), root, tpath);    
	#endif
	char fpath[MAX_PATH];
    char *fname =0;
    GetFullPathNameEx(lpath, sizeof(fpath), fpath, &fname);
    strcpy(lpath, fpath);
    if ( makeDirs )
        return WSHMakeLocalDirs(pmod,lpath);
    return 0;
}
// Determine a temp location from virtual path
int WsocksHostImpl::WSHGetFileTempPath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs)
{
	return WSHGetFileLocalPath(pmod,"NFSTemp", rpath, lpath, makeDirs);
}
// Determine a cache location from virtual path
int WsocksHostImpl::WSHGetFileCachePath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs)
{
	return WSHGetFileLocalPath(pmod,"NFSCache", rpath, lpath, makeDirs);
}
// Determine a persistent location from virtual path
int WsocksHostImpl::WSHGetFileKeepPath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs)
{
	return WSHGetFileLocalPath(pmod,"NFSKeep", rpath, lpath, makeDirs);
}
// Determine a commit location from virtual path
int WsocksHostImpl::WSHGetFileCommitPath(WsocksApp *pmod, const char *rpath, char *lpath, BOOL makeDirs)
{
	return WSHGetFileLocalPath(pmod,"NFSCommit", rpath, lpath, makeDirs);
}

// Determine the port number by F-port DriveList
int WsocksHostImpl::WSHGetFileTransferPort(WsocksApp *pmod, const char *lpFileName, BOOL showerr)
{
	if ( !WSFileTransferReady )
		return -1;
	if ( lpFileName[1] != ':' )
	{
		if ( showerr )
			WSHLogError(pmod,"!WSOCKS: ERROR : WSGetFileTransferPort: No drive letter to transfer \"%s\"!.", lpFileName);
		return -1;
	}
	char drive = toupper(lpFileName[0]);
	// Preference connected F ports first
	int i;
	for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
	{
		if ( pmod->FilePort[i].SockConnected && strchr(pmod->FilePort[i].DriveList, drive) )
			return i;
	}
	for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
	{
		if ( strchr(pmod->FilePort[i].DriveList, drive) )
			return i;
	}
	if ( showerr )
	    WSHLogError(pmod,"!WSOCKS: ERROR : WSGetFileTransferPort: No F ports for drive %c: to transfer \"%s\"!.", drive,lpFileName);
	return -1;
}

int WsocksHostImpl::WSHFileConnect(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN remote_sin; // Remote socket addr - internet style 
	int cnt;

	remote_sin.sin_family=AF_INET;
	remote_sin.sin_port=htons(pmod->FilePort[PortNo].Port);
	strcpy(pmod->FilePort[PortNo].Note, pmod->FilePort[PortNo].CfgNote);
	char *nptr = pmod->FilePort[PortNo].Note +strlen(pmod->FilePort[PortNo].Note);
	if ( pmod->FilePort[PortNo].DriveList[0] )
	{
		*nptr=','; nptr++;
		for ( const char *dptr=pmod->FilePort[PortNo].DriveList; *dptr; dptr++ )
		{
			*nptr=*dptr; nptr++;
			*nptr=':'; nptr++;
			*nptr=0;
		}
	}
	cnt = 0;

	do
	{
		pmod->FilePort[PortNo].CurrentAltIP++;
		if(pmod->FilePort[PortNo].CurrentAltIP>=pmod->FilePort[PortNo].AltIPCount)
			pmod->FilePort[PortNo].CurrentAltIP=0;
		strcpy(pmod->FilePort[PortNo].RemoteIP,pmod->FilePort[PortNo].AltRemoteIP[pmod->FilePort[PortNo].CurrentAltIP]);
		if (cnt > pmod->FilePort[PortNo].AltIPCount+1) {
			strcpy(pmod->FilePort[PortNo].RemoteIP, "");
			break;// none !
		}
		cnt++;

	}while(!pmod->FilePort[PortNo].AltRemoteIPOn[pmod->FilePort[PortNo].CurrentAltIP]);

	if(pmod->FilePort[PortNo].S5Connect)
	{
		remote_sin.sin_addr.s_addr=inet_addr(pmod->FilePort[PortNo].S5RemoteIP);
		remote_sin.sin_port=htons(pmod->FilePort[PortNo].S5Port);
	}
	else
	{
		remote_sin.sin_addr.s_addr=inet_addr(pmod->FilePort[PortNo].RemoteIP);
		remote_sin.sin_port=htons(pmod->FilePort[PortNo].Port);
	}
	
	//WSFileConnecting(PortNo);
    //if (bind( pmod->FilePort[PortNo].Sock, (struct sockaddr *) &remote_sin, sizeof(sockaddr)) == SOCKET_ERROR)  
    if (WSHConnectPort( pmod->FilePort[PortNo].Sock, &remote_sin.sin_addr, pmod->FilePort[PortNo].Port) == SOCKET_ERROR)  
	{
		int err=WSAGetLastError();
		_ASSERT(err=WSAEWOULDBLOCK);
		//pmod->FilePort[PortNo].ReconCount=WS_CONOUT_RECON/(WS_TIMER_INTERVAL?WS_TIMER_INTERVAL:1)+1;
		pmod->FilePort[PortNo].ReconCount=(GetTickCount() +WS_CONOUT_RECON);
		WSHUpdatePort(pmod,WS_FIL,PortNo);
		return(FALSE);
	}
	WSHFileConnected(pmod,PortNo);
	return(TRUE);
}
#ifdef WS_OTPP
struct FileReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
DWORD WINAPI _BootFileReadThread(LPVOID arg)
{
	FileReadThreadData *ptd=(FileReadThreadData*)arg;
	int rc=ptd->aimpl->WSHFileReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	return rc;
}
#endif
int WsocksHostImpl::WSHFileConnected(WsocksApp *pmod, int PortNo)
{
	WSCloseBuff(&pmod->FilePort[PortNo].InBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutBuffer);
	WSOpenBuffLimit(&pmod->FilePort[PortNo].OutBuffer,pmod->FilePort[PortNo].BlockSize,pmod->FilePort[PortNo].SendBuffLimit);
	WSOpenBuffLimit(&pmod->FilePort[PortNo].InBuffer,pmod->FilePort[PortNo].BlockSize,pmod->FilePort[PortNo].RecvBuffLimit);
#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->FilePort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->FilePort[PortNo].OutCompBuffer);
	WSOpenBuffLimit(&pmod->FilePort[PortNo].OutCompBuffer,pmod->FilePort[PortNo].BlockSize,pmod->FilePort[PortNo].SendBuffLimit);
	WSOpenBuffLimit(&pmod->FilePort[PortNo].InCompBuffer,pmod->FilePort[PortNo].BlockSize,pmod->FilePort[PortNo].RecvBuffLimit);
#endif
	pmod->FilePort[PortNo].SinceLastBeatCount=0;
	if (pmod->FilePort[PortNo].S5Connect)
	{
		if (pmod->FilePort[PortNo].S5Status<100)
		{
			WSHS5Login(pmod,PortNo);
			return(FALSE);
		}
	}
#ifdef WS_DECLINING_RECONNECT
	pmod->FilePort[PortNo].ConnectTime=GetTickCount();
#endif
	pmod->FilePort[PortNo].SockConnected=TRUE;
	pmod->FilePort[PortNo].ReconCount=0;
	UuidCreate((UUID*)pmod->FilePort[PortNo].Uuid);
	pmod->FilePort[PortNo].LastDataTime=GetTickCount();
	WSHUpdatePort(pmod,WS_FIL,PortNo);
	if(pmod->FilePort[PortNo].DoRecOnOpen)
	{
		pmod->WSOpenRecording(&pmod->FilePort[PortNo].Recording,pmod->WShWnd,WS_FIL,PortNo);
		pmod->FilePort[PortNo].DoRecOnOpen=pmod->FilePort[PortNo].Recording.DoRec;
	}
#ifdef WS_FILE_SMPROXY
	if(!strncmp(pmod->FilePort[PortNo].CfgNote,"$SMP$",5))
	{
		int& rid=(int&)pmod->FilePort[PortNo].DetPtr4;
		char mbuf[1024]={0},*mptr=mbuf,szParams[128];
		sprintf_s(szParams, "User=%s|Pass=|",pmod->pcfg->aname.c_str());
		ccodec.Encode(mptr,sizeof(mbuf),true,++rid,"login",szParams,0);
		WSHFileSendMsg(pmod,1108,(int)(mptr -mbuf),mbuf,PortNo);
	}
#endif
	pmod->WSFileOpened(PortNo);

#ifdef WS_OIO
	if(pmod->initialized>0)
	{
		WSPort *pport=(WSPort*)pmod->FilePort[PortNo].Sock;
		::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0);
		for(int o=0;o<WS_OVERLAP_MAX;o++)
		{
			if(WSHFileIocpBegin(pmod,PortNo)<0)
				return FALSE;
		}
	}
#elif defined(WS_OTPP)
	if(pmod->initialized>0)
	{
		WSPort *pport=(WSPort*)pmod->FilePort[PortNo].Sock;
		#ifdef WS_LOOPBACK
		if((pport)&&(!pport->lbaPeer))
		{
		#endif
			DWORD tid=0;
			FileReadThreadData *ptd=new FileReadThreadData;
			ptd->aimpl=this;
			ptd->pport=pport;
			pmod->FilePort[PortNo].pthread=CreateThread(0,0,_BootFileReadThread,ptd,0,&tid);
			if(!pmod->FilePort[PortNo].pthread)
			{
				WSHLogError(pmod,"FIL%d: Failed creating read thread: %d!",PortNo,GetLastError());
				_WSHCloseFilePort(pmod,PortNo);
				pmod->FilePort[PortNo].ConnectHold=true;
			}
		#ifdef WS_LOOPBACK
		}
		#endif
	}
#endif
	return (TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHFileIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	WSPort *pport=(WSPort*)pmod->FilePort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->FilePort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->FilePort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->FilePort[PortNo].InBuffer.BlockSize*1024;
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
		FreeOverlap(&pmod->FilePort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#endif
// File transfer could be called from WinMain so WSTimerProc may not have been
// called to connect the ports yet.
int WsocksHostImpl::WSHFileTransferConnect(WsocksApp *pmod, int PortNo)
{
	WSHLockPort(pmod,WS_FIL,PortNo,false);
	if((pmod->FilePort[PortNo].InUse)&&(!pmod->FilePort[PortNo].ConnectHold)&&(!pmod->FilePort[PortNo].SockOpen))
		WSHOpenFilePort(pmod,PortNo);
    if ( !pmod->FilePort[PortNo].SockConnected )
    {
		while((DWORD)pmod->FilePort[PortNo].ReconCount>0)
		{
            ws_fd_set Fd_Set;
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->FilePort[PortNo].Sock,&Fd_Set);
            timeval TimeVal = {0, 100000};
            WSHSelect((PTRCAST)pmod->FilePort[PortNo].Sock+10,NULL,&Fd_Set,NULL,(timeval*)&TimeVal);
			int Status=FD_ISSET(pmod->FilePort[PortNo].Sock,&Fd_Set);
			if(Status)
			{
				WSHFileConnected(pmod,PortNo);
				pmod->FilePort[PortNo].ReconCount=0;
			}
			else
			{
				// Cases to think about: G=GetTickCount current time, T=timeout time
				// ........ G ... T (normal timeout not expired)
				// ............. TG (normal timeout expired)
				// T ............ G (timeout wrapped, but timeout not expired)
				// G ............ T (time wrapped and timeout expired)
				// T G ............ (both wrapped and timeout expired)
				// This only works because it's a subtraction of two unsigned values,
				// and we assume no timeout set > 1 day (86,400 sec x 1000ms)
				if((GetTickCount() -(DWORD)pmod->FilePort[PortNo].ReconCount)<86400000) // tick count may have wrapped
				{
					WSHUnlockPort(pmod,WS_FIL,PortNo,false);
					_WSHCloseFilePort(pmod,PortNo);
					return -1;
				}
			}
		}
    }
    if ( !pmod->FilePort[PortNo].SockConnected )
	{
		WSHUnlockPort(pmod,WS_FIL,PortNo,false);
        return -1;
	}
	WSHUnlockPort(pmod,WS_FIL,PortNo,false);
    return 0;
}
int WsocksHostImpl::WSHRequestFile(WsocksApp *pmod, LPCSTR lpFileName, WSFileTransfer *wft, FileOptions *opts)
{
    // Request file transfer here
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x01;
    strncpy(Msg598Rec.FilePath, lpFileName, sizeof(Msg598Rec.FilePath) -1);
	Msg598Rec.FileSize = wft->fdata.nFileSizeLow;
	Msg598Rec.FileCreateTime = wft->fdata.ftCreationTime;
	Msg598Rec.FileModifyTime = wft->fdata.ftLastWriteTime;
    Msg598Rec.kbps = opts->xfersPerTick;

    Msg598Rec.dwDesiredAccess = wft->dwDesiredAccess;
    Msg598Rec.dwShareMode = wft->dwShareMode;
    Msg598Rec.dwCreationDisposition = wft->dwCreationDisposition;
    Msg598Rec.dwFlagsAndAttributes = wft->dwFlagsAndAttributes;

	// One transfer on a port at a time
	int PortNo = opts->PortNo;
    pmod->FilePort[PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, PortNo) )
    {
		pmod->FilePort[PortNo].FileTransfer = 0;
        wft->status = WSF_FAILED;
        WSHLogEvent(pmod,"WSOCKS: Failed requesting file transfer of \"%s\"!", wft->remotePath);
        return -1;
    }

    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: File transfer of \"%s\" requested on Port %d...", wft->remotePath, PortNo);
#endif
	return 0;
}
// A single file transfer over file port pre-initialization
static DWORD CALLBACK BootPIFileTransferThread(LPVOID arg)
{
	WSFileTransfer *wft=(WSFileTransfer *)arg;
	WsocksHostImpl *phost=(WsocksHostImpl*)wft->pmod->phost;
	int rc=phost->PIFileTransferThread(wft);
	return rc;
}
int WsocksHostImpl::PIFileTransferThread(WSFileTransfer *wft)
{
	WsocksApp *pmod=wft->pmod;
	int PortNo=wft->PortNo;
	if((PortNo<0)||(PortNo>=pmod->NO_OF_FILE_PORTS))
	{
		wft->status=WSF_FAILED;
		goto failed;
	}
	if(!pmod->FilePort[PortNo].SockConnected)
	{
		wft->status=WSF_FAILED;
		goto failed;
	}
	LockPort(pmod,WS_FIL,PortNo,false);
	int lastRecv=pmod->FilePort[PortNo].recvThread;
	pmod->FilePort[PortNo].recvThread=GetCurrentThreadId();
	LockPort(pmod,WS_FIL,PortNo,true);
	int lastSend=pmod->FilePort[PortNo].sendThread;
	pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();
	char rbuf[1024]={0};
	DWORD txfer=INFINITE;
	while((wft->status!=WSF_SUCCESS)&&
		  (wft->status!=WSF_FAILED)&&
		  (wft->status!=WSF_CANCELLED)&&
		  (wft->status!=WSF_TIMEOUT))
	{
		// Give the app one last shot to read the rest of the data
		if(pmod->FilePort[PortNo].peerClosed)
		{
			while(pmod->WSFileMsgReady(PortNo))
				pmod->WSFileStripMsg(PortNo);
			wft->status=WSF_FAILED;
			goto failed;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->FilePort[PortNo].appClosed)
		{
			WSHFileSend(pmod,PortNo,true);
			wft->status=WSF_FAILED;
			goto failed;
		}
		// Transfer data for this port only
		else if(pmod->FilePort[PortNo].SockConnected)
		{
			int rlen=WSHRecv(pmod->FilePort[PortNo].Sock,rbuf,sizeof(rbuf),0);
			if(rlen<=0)
			{
				if(WSAGetLastError()==WSAEWOULDBLOCK)
				{
					if((txfer!=INFINITE)&&(GetTickCount()>=txfer))
					{
						wft->status = WSF_TIMEOUT;
						goto failed;
					}
					continue;
				}
				pmod->FilePort[PortNo].peerClosed=true;
				continue;
			}
			WSHFileRead(pmod,PortNo,rbuf,rlen);
			while(WSHFileMsgReady(pmod,PortNo))
				WSHFileStripMsg(pmod,PortNo);

			if((wft)&&(wft->status == WSF_COMMIT_INPROGRESS)&&
			   (pmod->FilePort[PortNo].OutBuffer.Size<10*1024*1024))
				WSHCommitFileTransfer(pmod,PortNo);

			WSHFileSend(pmod,PortNo,true);
		}
	}
	if(pmod->FilePort[PortNo].SockConnected)
	{
		WSHFileSend(pmod,PortNo,true);
		// Wait for last send for pre-init case
		SleepEx(100,true);
		_WSHCloseFilePort(pmod,PortNo);
		// Allow immediate reconnect
		pmod->FilePort[PortNo].ReconnectTime=0;
	}
	pmod->FilePort[PortNo].sendThread=lastSend;
	UnlockPort(pmod,WS_FIL,PortNo,true);
	pmod->FilePort[PortNo].recvThread=lastRecv;
	UnlockPort(pmod,WS_FIL,PortNo,false);
	return 0;
failed:
	if(pmod->FilePort[PortNo].SockConnected)
		_WSHCloseFilePort(pmod,PortNo);
	if(wft->progWnd)
		PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
	pmod->FilePort[PortNo].sendThread=lastSend;
	UnlockPort(pmod,WS_FIL,PortNo,true);
	pmod->FilePort[PortNo].recvThread=lastRecv;
	UnlockPort(pmod,WS_FIL,PortNo,false);
	return -1;
}
#ifdef WS_FILE_SMPROXY
static DWORD CALLBACK BootPIFileTransferThread2(LPVOID arg)
{
	WSFileTransfer2 *wft=(WSFileTransfer2 *)arg;
	WsocksHostImpl *phost=(WsocksHostImpl*)wft->pmod->phost;
	int rc=phost->PIFileTransferThread2(wft);
	return rc;
}
int WsocksHostImpl::PIFileTransferThread2(WSFileTransfer2 *wft)
{
	WsocksApp *pmod=wft->pmod;
	int PortNo=wft->PortNo;
	if((PortNo<0)||(PortNo>=pmod->NO_OF_FILE_PORTS))
	{
		wft->status=WSF_FAILED;
		goto failed;
	}
	if(!pmod->FilePort[PortNo].SockConnected)
	{
		wft->status=WSF_FAILED;
		goto failed;
	}
	LockPort(pmod,WS_FIL,PortNo,false);
	int lastRecv=pmod->FilePort[PortNo].recvThread;
	pmod->FilePort[PortNo].recvThread=GetCurrentThreadId();
	LockPort(pmod,WS_FIL,PortNo,true);
	int lastSend=pmod->FilePort[PortNo].sendThread;
	pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();
	char rbuf[1024]={0};
	DWORD txfer=INFINITE;
	while((wft->status!=WSF_SUCCESS)&&
		  (wft->status!=WSF_FAILED)&&
		  (wft->status!=WSF_CANCELLED)&&
		  (wft->status!=WSF_TIMEOUT))
	{
		// Give the app one last shot to read the rest of the data
		if(pmod->FilePort[PortNo].peerClosed)
		{
			while(pmod->WSFileMsgReady(PortNo))
				pmod->WSFileStripMsg(PortNo);
			wft->status=WSF_FAILED;
			goto failed;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->FilePort[PortNo].appClosed)
		{
			WSHFileSend(pmod,PortNo,true);
			wft->status=WSF_FAILED;
			goto failed;
		}
		// Transfer data for this port only
		else if(pmod->FilePort[PortNo].SockConnected)
		{
			int rlen=WSHRecv(pmod->FilePort[PortNo].Sock,rbuf,sizeof(rbuf),0);
			if(rlen<=0)
			{
				if(WSAGetLastError()==WSAEWOULDBLOCK)
				{
					if((txfer!=INFINITE)&&(GetTickCount()>=txfer))
					{
						wft->status = WSF_TIMEOUT;
						goto failed;
					}
					continue;
				}
				pmod->FilePort[PortNo].peerClosed=true;
				continue;
			}
			WSHFileRead(pmod,PortNo,rbuf,rlen);
			while(WSHFileMsgReady(pmod,PortNo))
				WSHFileStripMsg(pmod,PortNo);

			WSHFileSend(pmod,PortNo,true);
		}
	}
	if(pmod->FilePort[PortNo].SockConnected)
	{
		WSHFileSend(pmod,PortNo,true);
		// Wait for last send for pre-init case
		SleepEx(100,true);
		_WSHCloseFilePort(pmod,PortNo);
		// Allow immediate reconnect
		pmod->FilePort[PortNo].ReconnectTime=0;
	}
	pmod->FilePort[PortNo].sendThread=lastSend;
	UnlockPort(pmod,WS_FIL,PortNo,true);
	pmod->FilePort[PortNo].recvThread=lastRecv;
	UnlockPort(pmod,WS_FIL,PortNo,false);
	return 0;
failed:
	if(pmod->FilePort[PortNo].SockConnected)
		_WSHCloseFilePort(pmod,PortNo);
	//if(wft->progWnd)
	//	PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
	pmod->FilePort[PortNo].sendThread=lastSend;
	UnlockPort(pmod,WS_FIL,PortNo,true);
	pmod->FilePort[PortNo].recvThread=lastRecv;
	UnlockPort(pmod,WS_FIL,PortNo,false);
	return -1;
}
#endif//WS_FILE_SMPROXY

// A single file transfer over file port post-initialization
static DWORD CALLBACK BootFileTransferThread(LPVOID arg)
{
	WSFileTransfer *wft=(WSFileTransfer *)arg;
	WsocksHostImpl *phost=(WsocksHostImpl*)wft->pmod->phost;
	int rc=phost->FileTransferThread(wft);
	return rc;
}
int WsocksHostImpl::FileTransferThread(WSFileTransfer *wft)
{
	WsocksApp *pmod=wft->pmod;
	int PortNo=wft->PortNo;
	if((PortNo<0)||(PortNo>=pmod->NO_OF_FILE_PORTS))
	{
		wft->status=WSF_FAILED;
		goto failed;
	}
	if(!pmod->FilePort[PortNo].SockConnected)
	{
		wft->status=WSF_FAILED;
		goto failed;
	}
	char rbuf[1024]={0};
	DWORD txfer=INFINITE;
	while((wft->status!=WSF_SUCCESS)&&
		  (wft->status!=WSF_FAILED)&&
		  (wft->status!=WSF_CANCELLED)&&
		  (wft->status!=WSF_TIMEOUT))
	{
		// Give the app one last shot to read the rest of the data
		if(pmod->FilePort[PortNo].peerClosed)
		{
			LockPort(pmod,WS_FIL,PortNo,false);
			int lastRecv=pmod->FilePort[PortNo].recvThread;
			pmod->FilePort[PortNo].recvThread=GetCurrentThreadId();

			while(pmod->WSFileMsgReady(PortNo))
				pmod->WSFileStripMsg(PortNo);

			pmod->FilePort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_FIL,PortNo,false);
			wft->status=WSF_FAILED;
			goto failed;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->FilePort[PortNo].appClosed)
		{
			LockPort(pmod,WS_FIL,PortNo,true);
			int lastSend=pmod->FilePort[PortNo].sendThread;
			pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();

			WSHFileSend(pmod,PortNo,true);

			pmod->FilePort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_FIL,PortNo,true);
			wft->status=WSF_FAILED;
			goto failed;
		}
		// Transfer data for this port only
		else if(pmod->FilePort[PortNo].SockConnected)
		{
			// Ouch: this really only works if the app is multi-threaded
			//int rlen=WSHRecv(pmod->FilePort[PortNo].Sock,rbuf,sizeof(rbuf),0);
			//if(rlen<=0)
			//{
			//	if(WSAGetLastError()==WSAEWOULDBLOCK)
			//	{
			//		if((txfer!=INFINITE)&&(GetTickCount()>=txfer))
			//		{
			//			wft->status = WSF_TIMEOUT;
			//			goto failed;
			//		}
			//		continue;
			//	}
			//	pmod->FilePort[PortNo].peerClosed=true;
			//	continue;
			//}
			//WSHFileRead(pmod,PortNo,rbuf,rlen);
			//while(WSHFileMsgReady(pmod,PortNo))
			//	WSHFileStripMsg(pmod,PortNo);
			if((wft)&&(wft->status == WSF_COMMIT_INPROGRESS)&&
			   (pmod->FilePort[PortNo].OutBuffer.Size<10*1024*1024))
				WSHCommitFileTransfer(pmod,PortNo);

			LockPort(pmod,WS_FIL,PortNo,true);
			int lastSend=pmod->FilePort[PortNo].sendThread;
			pmod->FilePort[PortNo].sendThread=GetCurrentThreadId();

			WSHFileSend(pmod,PortNo,true);

			pmod->FilePort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_FIL,PortNo,true);
		}
	}
	return 0;
failed:
	if(wft->progWnd)
		PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
	return -1;
}
// Just like CreateFile, but with some extras at the end
WSFileTransfer *WsocksHostImpl::WSHCreateFileAsync(WsocksApp *pmod, LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
												   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, 
												   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, FileOptions *opts)
{
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,lpFileName, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	// Guaranteed write-only
	bool gdWriteOnly = false;
	if ( (dwDesiredAccess &GENERIC_WRITE) && 
		 !(dwDesiredAccess &GENERIC_READ) &&
		  opts->guaranteeCommit )
		gdWriteOnly = true;

	// One transfer on a port at a time
	int PortNo = opts->PortNo;
	if ( pmod->FilePort[PortNo].FileTransfer && !gdWriteOnly )
	{
		WSHLogError(pmod,"!WSOCKS: ERROR : WSCreateFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpFileName);
		return 0;
	}
	if ( WSHFileTransferConnect(pmod,PortNo) && !gdWriteOnly )
	{
		WSHLogError(pmod,"!WSOCKS: ERROR : WSCreateFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpFileName);
		return 0;
	}

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSCreateFile Creating Buffer");
        return 0;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
	wft->pmod=pmod;
    strcpy(wft->remotePath, lpFileName);
    wft->api = FTAPI_CREATEFILE;
	memcpy(&wft->opts, opts, sizeof(FileOptions));
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    WSHGetFileTempPath(pmod,lpFileName, wft->tempPath, true);
    if ( opts->keepLocal )
        WSHGetFileKeepPath(pmod,lpFileName, wft->localPath, true);
	else if ( opts->cacheLocal )
        WSHGetFileCachePath(pmod,lpFileName, wft->localPath, true);
	wft->gdWriteOnly = gdWriteOnly;
    wft->dwDesiredAccess = dwDesiredAccess;
    wft->dwShareMode = dwShareMode;
    wft->dwCreationDisposition = dwCreationDisposition;
    wft->dwFlagsAndAttributes = dwFlagsAndAttributes;
	if ( *wft->localPath )
	{
		HANDLE fhnd = FindFirstFile(wft->localPath, &wft->fdata);
		if ( fhnd != INVALID_HANDLE_VALUE )
			FindClose(fhnd);
	}

    wft->thnd = CreateFile(wft->tempPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if ( wft->thnd == INVALID_HANDLE_VALUE )
    {
        int err = GetLastError();
        WSHLogError(pmod,"!WSOCKS: ERROR : WSCreateFile: Failed opening temp file \"%s\" with error %d to transfer \"%s\"", wft->tempPath, err, lpFileName);
        free(wft);
        return 0;
    }
    wft->next = fileTransfers; fileTransfers = wft;

	// No need to request anything if we're not writing
	if ( opts->localMaster && PathFileExists(wft->localPath) )
	{
		wft->uptodate = true;
		if ( wft->dwDesiredAccess &GENERIC_WRITE )
			wft->status = WSF_WAIT_COMMIT;
		else
			wft->status = WSF_SUCCESS;
	}
	else if ( WSHRequestFile(pmod,lpFileName, wft, opts) && !gdWriteOnly )
		return 0;
    return wft;
}
// Just like fopen, but with some extras at the end
WSFileTransfer *WsocksHostImpl::WSHfopenAsync(WsocksApp *pmod, const char *filename, const char *mode, FileOptions *opts)
{
	// Translate mode to CreateFile symantics
	DWORD dwDesiredAccess = 0;
	DWORD dwShareMode = 0;
	DWORD dwCreationDisposition = 0;
	DWORD dwFlagsAndAttributes = 0;
    WORD fbits = 0;
#define FBITS_READ      0x0001
#define FBITS_WRITE     0x0002
#define FBITS_APPEND    0x0004
#define FBITS_PLUS      0x0010
#define FBITS_TEXT      0x0100
#define FBITS_BINARY    0x0200
#define FBITS_COMMIT    0x1000
#define FBITS_NOCOMMIT  0x2000
    for ( const char *mptr=mode; *mptr; mptr++ )
    {
        switch ( *mptr )
        {
        case 'r': fbits |= FBITS_READ; break;
        case 'w': fbits |= FBITS_WRITE; break;
        case 'a': fbits |= FBITS_APPEND; break;
        case '+': fbits |= FBITS_PLUS; break;
        case 't': fbits |= FBITS_TEXT; break;
        case 'b': fbits |= FBITS_BINARY; break;
        case 'c': fbits |= FBITS_COMMIT; break;
        case 'n': fbits |= FBITS_NOCOMMIT; break;
        }
    }
    if ( (fbits &FBITS_READ) && (fbits &FBITS_PLUS) )
    {
        dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = OPEN_ALWAYS;
        dwFlagsAndAttributes = 0;
    }
    else if ( (fbits &FBITS_WRITE) && (fbits &FBITS_PLUS) )
    {
        dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = OPEN_EXISTING;
        dwFlagsAndAttributes = 0;
    }
    else if ( (fbits &FBITS_APPEND) && (fbits &FBITS_PLUS) )
    {
        dwDesiredAccess = GENERIC_READ |GENERIC_WRITE |GENERIC_APPEND;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = OPEN_ALWAYS;
        dwFlagsAndAttributes = 0;
    }
    else if ( (fbits &FBITS_READ) && (fbits &FBITS_WRITE) )
    {
        dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = OPEN_ALWAYS;
        dwFlagsAndAttributes = 0;
    }
    else if ( fbits &FBITS_READ )
    {
        dwDesiredAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = OPEN_EXISTING;
        dwFlagsAndAttributes = 0;
    }
    else if ( fbits &FBITS_WRITE )
    {
        dwDesiredAccess = GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = CREATE_ALWAYS;
        dwFlagsAndAttributes = 0;
    }
    else if ( fbits &FBITS_APPEND )
    {
        dwDesiredAccess = GENERIC_WRITE |GENERIC_APPEND;
        dwShareMode = FILE_SHARE_READ;
        dwCreationDisposition = OPEN_ALWAYS;
        dwFlagsAndAttributes = 0;
    }

	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,filename, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	// Guaranteed write-only
	bool gdWriteOnly = false;
	if ( (dwDesiredAccess &GENERIC_WRITE) && 
		 !(dwDesiredAccess &GENERIC_READ) &&
		  opts->guaranteeCommit )
		gdWriteOnly = true;

    // One transfer on a port at a time
	int PortNo = opts->PortNo;
    if ( pmod->FilePort[PortNo].FileTransfer && !gdWriteOnly )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSfopen: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, filename);
        return 0;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) && !gdWriteOnly )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSfopen: FilePort %d not connected to transfer \"%s\"!", PortNo, filename);
        return 0;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSfopen Creating Buffer");
        return 0;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, filename);
    strcpy(wft->mode, mode);
    wft->api = FTAPI_FOPEN;
	memcpy(&wft->opts, opts, sizeof(FileOptions));
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    WSHGetFileTempPath(pmod,filename, wft->tempPath, true);
    if ( opts->keepLocal )
        WSHGetFileKeepPath(pmod,filename, wft->localPath, true);
	else if ( opts->cacheLocal )
        WSHGetFileCachePath(pmod,filename, wft->localPath, true);
	wft->gdWriteOnly = gdWriteOnly;
    wft->dwDesiredAccess = dwDesiredAccess;
    wft->dwShareMode = dwShareMode;
    wft->dwCreationDisposition = dwCreationDisposition;
    wft->dwFlagsAndAttributes = dwFlagsAndAttributes;
	if ( *wft->localPath )
	{
		HANDLE fhnd = FindFirstFile(wft->localPath, &wft->fdata);
		if ( fhnd != INVALID_HANDLE_VALUE )
			FindClose(fhnd);
	}

    wft->thnd = CreateFile(wft->tempPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if ( wft->thnd == INVALID_HANDLE_VALUE )
    {
        int err = GetLastError();
        WSHLogError(pmod,"!WSOCKS: ERROR : WSfopen: Failed opening temp file \"%s\" with error %d to transfer \"%s\"", wft->tempPath, err, filename);
        free(wft);
        return 0;
    }
    wft->next = fileTransfers; fileTransfers = wft;

	// No need to request anything if we're not writing
	if ( opts->localMaster && PathFileExists(wft->localPath) )
	{
		wft->uptodate = true;
		if ( wft->dwDesiredAccess &GENERIC_WRITE )
			wft->status = WSF_WAIT_COMMIT;
		else
			wft->status = WSF_SUCCESS;
	}
	else if ( WSHRequestFile(pmod,filename, wft, opts) && !gdWriteOnly )
		return 0;
    return wft;
}
// Just like OpenFile, but with some extras at the end
WSFileTransfer *WsocksHostImpl::WSHOpenFileAsync(WsocksApp *pmod, LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle, FileOptions *opts)
{
	// Translate open style to CreateFile symantics
	DWORD dwDesiredAccess = 0;
	DWORD dwShareMode = 0;
	DWORD dwCreationDisposition = 0;
	DWORD dwFlagsAndAttributes = 0;
    // We don't support these options for now
    if ( (uStyle &OF_CANCEL)||(uStyle &OF_DELETE)||(uStyle &OF_EXIST)||
         (uStyle &OF_PARSE)||(uStyle &OF_PROMPT)||(uStyle &OF_REOPEN)||
         (uStyle &OF_VERIFY)||(uStyle &OF_SHARE_COMPAT) )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSOpenFile doesn't support all uStyle options: %08X", uStyle);
        return 0;
    }
    if ( uStyle &OF_CREATE )
        dwCreationDisposition |= CREATE_ALWAYS;
    else
        dwCreationDisposition = OPEN_ALWAYS;
    if ( uStyle &OF_READ )
        dwDesiredAccess |= GENERIC_READ;
    if ( uStyle &OF_READWRITE )
        dwDesiredAccess |= GENERIC_READ |GENERIC_WRITE;
    if ( uStyle &OF_SHARE_DENY_NONE )
        dwShareMode |= FILE_SHARE_READ |FILE_SHARE_WRITE;
    if ( uStyle &OF_SHARE_DENY_READ )
        dwShareMode |= FILE_SHARE_WRITE;
    if ( uStyle &OF_SHARE_DENY_WRITE )
        dwShareMode |= FILE_SHARE_READ;
    if ( uStyle &OF_WRITE )
        dwDesiredAccess |= GENERIC_WRITE;

	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,lpFileName, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	// Guaranteed write-only
	bool gdWriteOnly = false;
	if ( (dwDesiredAccess &GENERIC_WRITE) && 
		 !(dwDesiredAccess &GENERIC_READ) &&
		  opts->guaranteeCommit )
		gdWriteOnly = true;

    // One transfer on a port at a time
	int PortNo = opts->PortNo;
    if ( pmod->FilePort[PortNo].FileTransfer && !gdWriteOnly )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSOpenFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpFileName);
        return 0;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) && !gdWriteOnly )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSOpenFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpFileName);
        return 0;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSOpenFile Creating Buffer");
        return 0;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, lpFileName);
    if ( lpReOpenBuff )
        wft->ReOpenBuff = *lpReOpenBuff;
    wft->uStyle = uStyle;
    wft->api = FTAPI_OPENFILE;
	memcpy(&wft->opts, opts, sizeof(FileOptions));
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    WSHGetFileTempPath(pmod,lpFileName, wft->tempPath, true);
    if ( opts->keepLocal )
        WSHGetFileKeepPath(pmod,lpFileName, wft->localPath, true);
	else if ( opts->cacheLocal )
        WSHGetFileCachePath(pmod,lpFileName, wft->localPath, true);
	wft->gdWriteOnly = gdWriteOnly;
    wft->dwDesiredAccess = dwDesiredAccess;
    wft->dwShareMode = dwShareMode;
    wft->dwCreationDisposition = dwCreationDisposition;
    wft->dwFlagsAndAttributes = dwFlagsAndAttributes;
	if ( *wft->localPath )
	{
		HANDLE fhnd = FindFirstFile(wft->localPath, &wft->fdata);
		if ( fhnd != INVALID_HANDLE_VALUE )
			FindClose(fhnd);
	}

    wft->thnd = CreateFile(wft->tempPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if ( wft->thnd == INVALID_HANDLE_VALUE )
    {
        int err = GetLastError();
        WSHLogError(pmod,"!WSOCKS: ERROR : WSOpenFile: Failed opening temp file \"%s\" with error %d to transfer \"%s\"", wft->tempPath, err, lpFileName);
        free(wft);
        return 0;
    }
    wft->next = fileTransfers; fileTransfers = wft;

	// No need to request anything if we're not writing
	if ( opts->localMaster && PathFileExists(wft->localPath) )
	{
		wft->uptodate = true;
		if ( wft->dwDesiredAccess &GENERIC_WRITE )
			wft->status = WSF_WAIT_COMMIT;
		else
			wft->status = WSF_SUCCESS;
	}
	else if ( WSHRequestFile(pmod,lpFileName, wft, opts) && !gdWriteOnly )
		return 0;
    return wft;
}

static LRESULT CALLBACK cbProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return WsocksHostImpl::cbProgress(hDlg,message,wParam,lParam);
}
//static void CALLBACK cbTimer(HWND hDlg, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
//{
//}
// No longer in winuser.h in VS2010
#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif
LRESULT CALLBACK WsocksHostImpl::cbProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_INITDIALOG:
        {
        WSFileTransfer *wft = (WSFileTransfer *)lParam;
		WsocksApp *pmod=wft->pmod;
        SetWindowLong(hDlg, GWL_USERDATA, (DWORD)(PTRCAST)wft);
        wft->progWnd = hDlg;
        // Center the dialog within the parent window
        HWND parent = GetParent(hDlg);
        if ( !parent ) parent = GetDesktopWindow();
        RECT crect, prect;
        GetWindowRect(hDlg, &crect);
        GetWindowRect(parent, &prect);
        int cx = crect.right -crect.left;
        int cy = crect.bottom -crect.top;
        crect.left = prect.left +(prect.right -prect.left -cx) /2; crect.right = crect.left +cx;
        crect.top = prect.top +(prect.bottom -prect.top -cy) /2; crect.bottom = crect.top +cy;
        MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
		// The FileTransferThread may have already completed for small files
        if(wft->opts.autoCloseProgressDialog)
        {
			switch(wft->status)
			{
			case WSF_SUCCESS:
			case WSF_FAILED:
			case WSF_CANCELLED:
			case WSF_TIMEOUT:
				wft->progWnd = 0;
				EndDialog(hDlg, IDOK);
				break;
			};
        }
		switch(wft->status)
		{
		// Commit timeout
		case WSF_COMMIT_INPROGRESS:
		case WSF_WAIT_COMMIT:
		case WSF_WAIT_COMMIT_ACK:
			// WSHRecvFileTransfer already tracks commit timeout
			//SetTimer(hDlg,0x01,wft->opts.commitTimeout*1000,0);
			break;
		// Download timeout
		default:
			SetTimer(hDlg,0x01,wft->opts.waitTimeout*1000,0);
		};
        return FALSE;
        }

    case WM_PAINT:
        {
        WSFileTransfer *wft = (WSFileTransfer *)(PTRCAST)GetWindowLong(hDlg, GWL_USERDATA);
		WsocksApp *pmod=wft->pmod;
        // We only have room for the rightmost 24 characters of the filepath
        char rpath[32];
        memset(rpath, 0, sizeof(rpath));
        int rplen = (int)strlen(wft->remotePath);
        if ( rplen > 24 )
            memcpy(rpath, wft->remotePath +rplen -24, 24);
        else
            memcpy(rpath, wft->remotePath, rplen);
        char msg[1024];
        memset(msg, 0, sizeof(msg));
        const char *tstr = "Download";
        int bytes = wft->recvd;
        if ( wft->commitAttempted )
        {
            tstr = "Commit";
            bytes = wft->sent;
        }
        switch ( wft->status )
        {
        case WSF_INIT:
			sprintf(msg, "Connecting %s:%d...", 
				pmod->FilePort[wft->PortNo].RemoteIP, pmod->FilePort[wft->PortNo].Port);
            break;
        case WSF_REQUESTED:
            sprintf(msg, "Requested \"%s\"...", rpath);
            break;
        case WSF_RECV_INPROGRESS:
            sprintf(msg, "Downloading \"%s\" (%s/%s) bytes...", 
                rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        case WSF_WAIT_COMMIT:
            sprintf(msg, "Download complete awaiting commit for \"%s\" (%s/%s) bytes.", 
                rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        case WSF_COMMIT_INPROGRESS:
            sprintf(msg, "Committing \"%s\" (%s/%s) bytes...", 
                rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        case WSF_WAIT_COMMIT_ACK:
            sprintf(msg, "Waiting commit ack \"%s\" (%s/%s) bytes...", 
                rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        case WSF_SUCCESS:
            sprintf(msg, "%s complete for \"%s\" (%s/%s) bytes.", 
                tstr, rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        case WSF_FAILED:
            sprintf(msg, "%s failed for \"%s\" (%s/%s) bytes!", 
                tstr, rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        case WSF_CANCELLED:
            sprintf(msg, "%s cancelled for \"%s\" (%s/%s) bytes.", 
                tstr, rpath, pmod->SizeStr(bytes), pmod->SizeStr(wft->fsize));
            break;
        }
        SetDlgItemText(hDlg, IDC_MSG, msg);

        // Show the percentage complete
        int perc = 0;
        if ( wft->commitAttempted )
            perc = wft->fsize ?(int)((__int64)wft->sent *100 /wft->fsize) :0;
        else
            perc = wft->fsize ?(int)((__int64)wft->recvd *100 /wft->fsize) :0;
        if ( perc >= 100 ) perc = 100;
        char pstr[32];
        memset(pstr, 0, sizeof(pstr));
        sprintf(pstr, "%d%%", perc);
        SetDlgItemText(hDlg, IDC_PERCENT, pstr);

        // Paint the progress bar
        HWND prog = GetDlgItem(hDlg, IDC_PROGRESS);
        RECT prect;
        GetWindowRect(prog, &prect);
        POINT p1={prect.left, prect.top}, p2={prect.right, prect.bottom};
        ScreenToClient(hDlg, &p1); ScreenToClient(hDlg, &p2);
        int pwid = (p2.x -p1.x) *perc /100;
        HDC hdc = GetDC(hDlg);
        SelectObject(hdc, GetStockObject(WHITE_BRUSH));
        Rectangle(hdc, p1.x, p1.y, p1.x +pwid, p2.y);
        if ( wft->status == WSF_FAILED || wft->status == WSF_CANCELLED )
            SelectObject(hdc, GetStockObject(GRAY_BRUSH));
        else
            SelectObject(hdc, GetStockObject(BLACK_BRUSH));
        Rectangle(hdc, p1.x +pwid, p1.y, p2.x, p2.y);
        ReleaseDC(hDlg, hdc);
        prect.left = p1.x; prect.top = p1.y; prect.right = p2.x; prect.bottom = p2.y;
        ValidateRect(hDlg, &prect);
        }
        break;

	case WM_COMMAND:
	{
        WSFileTransfer *wft = (WSFileTransfer *)(PTRCAST)GetWindowLong(hDlg, GWL_USERDATA);
		WsocksApp *pmod=wft->pmod;
		if (LOWORD(wParam) == IDCANCEL) 
		{
            if ( wft->status == WSF_INIT ||
				 wft->status == WSF_REQUESTED ||
                 wft->status == WSF_RECV_INPROGRESS ||
                 wft->status == WSF_COMMIT_INPROGRESS ||
                 wft->status == WSF_WAIT_COMMIT_ACK )
            {
                char qmsg[1024];
                memset(qmsg, 0, sizeof(qmsg));
                sprintf(qmsg, "Cancel file transfer of \"%s\"?", wft->remotePath);
                if ( MessageBox(hDlg, qmsg, "Cancel Verification", MB_ICONQUESTION |MB_YESNO) == IDYES )
                {
					if((pmod)&&(pmod->phost))
						((WsocksHostImpl*)pmod->phost)->WSHCancelFile(pmod,wft);
                    wft->progWnd = 0;
					KillTimer(hDlg,0x01);
			        EndDialog(hDlg, IDCANCEL);
                }
            }
            else
            {
                wft->progWnd = 0;
				KillTimer(hDlg,0x01);
			    EndDialog(hDlg, IDOK);
            }
			return TRUE;
		}
        else if (LOWORD(wParam) == WS_XFER_STATUS) 
        {
            // If we're done, then change the Cancel button to Close
            switch ( wft->status )
            {
            case WSF_WAIT_COMMIT:
            case WSF_SUCCESS:
            case WSF_FAILED:
            case WSF_CANCELLED:
            case WSF_TIMEOUT:
                SetDlgItemText(hDlg, IDCANCEL, "Close");
                if ( wft->opts.autoCloseProgressDialog )
                {
                    wft->progWnd = 0;
					KillTimer(hDlg,0x01);
			        EndDialog(hDlg, IDOK);
                }
                break;
            }
            // Invalidate the progress bar for painting
            HWND prog = GetDlgItem(hDlg, IDC_PROGRESS);
            RECT prect;
            GetWindowRect(prog, &prect);
            POINT p1={prect.left, prect.top}, p2={prect.right, prect.bottom};
            ScreenToClient(hDlg, &p1); ScreenToClient(hDlg, &p2);
            prect.left = p1.x; prect.top = p1.y; prect.right = p2.x; prect.bottom = p2.y;
            InvalidateRect(hDlg, &prect, false);
            return TRUE;
        }
	}

	case WM_TIMER:
	{
		WSFileTransfer *wft = (WSFileTransfer *)(PTRCAST)GetWindowLong(hDlg, GWL_USERDATA);
		WsocksApp *pmod=wft->pmod;
		// Don't abort if we just crossed the finish line
        switch ( wft->status )
        {
        case WSF_SUCCESS:
        case WSF_FAILED:
        case WSF_CANCELLED:
        case WSF_TIMEOUT:
			break;
		default:
			KillTimer(hDlg,0x01);
			// Send timeout status to fileserver so it knows we're aborting
			if((pmod)&&(pmod->phost))
				((WsocksHostImpl*)pmod->phost)->WSHCancelFile(pmod,wft);
			wft->status = WSF_TIMEOUT;
			wft->progWnd = 0;
			EndDialog(hDlg,IDCANCEL);
		};
		break;
	}
	};
    return FALSE;
}

int WsocksHostImpl::WSHS5FileConnect(WsocksApp *pmod, int PortNo)
{
	char Buff[1024];
	SOCKADDR_IN remote_sin;
	
	remote_sin.sin_addr.s_addr=inet_addr(pmod->FilePort[PortNo].RemoteIP);
	remote_sin.sin_port=htons(pmod->FilePort[PortNo].Port);

	sprintf(Buff,"%c%c%c%c",pmod->FilePort[PortNo].S5Version,1,0,1);
	WSHFileSendBuffNew(pmod,4,Buff,PortNo,0,TRUE);
	WSHFileSendBuffNew(pmod,4,(char*)&remote_sin.sin_addr.s_addr,PortNo,0,TRUE);
	WSHFileSendBuffNew(pmod,2,(char*)&remote_sin.sin_port,PortNo,1,TRUE);
	pmod->FilePort[PortNo].S5Status=20;
	return(TRUE);
}

// Duplicate of code in WSTimerProc to be called by WSWaitFile,
// when the timeout is not-infinite. This will allow the file transfer
// to completely block the caller from additional timer notifications.
// For example, clserver cannot tolerate additional WSConMsgReady when
// it has initiated a file transfer from within WSTranslateMsg.
int WsocksHostImpl::WSHFileTransferLoop(WsocksApp *pmod)
{
#if !defined(WS_OIO)&&!defined(WS_OTPP)
	fd_set Fd_Set;
	timeval TimeVal={0,0};
	int Status;
	unsigned int PreCompBlock=WS_COMP_BLOCK_LEN;
#ifdef WS_COMPRESS
	int CompSize=0;
	unsigned int CompBuffSize=0;
#ifdef WS_ENCRYPTED
	//static char CompBuff[(WS_MAX_BLOCK_SIZE*1024)*2];
	char *CompBuff=pmod->FileTransfer_CompBuff;
	unsigned int EncryptSize;
#endif
#endif
    int SendBlockSize=0;
	int SendSize=0;

//#ifdef WS_OIO
//	WSFlushCompletions();
//#endif

    for ( int i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
    {
		// Connect file ports
		if((pmod->FilePort[i].InUse)&&(!pmod->FilePort[i].ConnectHold)&&(!pmod->FilePort[i].SockOpen))
			WSHOpenFilePort(pmod,i);
		if((pmod->FilePort[i].InUse)&&(pmod->FilePort[i].ConnectHold)&&(pmod->FilePort[i].SockOpen))
			_WSHCloseFilePort(pmod,i);
		if(pmod->FilePort[i].ReconCount)
		{
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->FilePort[i].Sock,&Fd_Set);
			timeval TimeVal = {0, 0};
			WSHSelect(pmod->FilePort[i].Sock+1,NULL,&Fd_Set,NULL,(timeval*)&TimeVal);
			Status=FD_ISSET(pmod->FilePort[i].Sock,&Fd_Set);
			if(Status)
			{
				WSHFileConnected(pmod,i);
				pmod->FilePort[i].ReconCount=0;
			}
			else
			{
				pmod->FilePort[i].ReconCount--;
				if(pmod->FilePort[i].ReconCount<=1)
				{
					_WSHCloseFilePort(pmod,i);
				}
			}
		}
	}
    for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
    {
		// Recv and send file port data
        if ((!pmod->FilePort[i].SockConnected)&&
            !((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status>0)) )
            continue;
		if((pmod->FilePort[i].SockConnected)||((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status>=10)&&(pmod->FilePort[i].S5Status<100)))
		{
File_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->FilePort[i].Sock,&Fd_Set);
			WSHSelect(pmod->FilePort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=FD_ISSET(pmod->FilePort[i].Sock,&Fd_Set);
			if(Status)
			{
				if(!WSHFileRead(pmod,i))
					continue;
				else
					goto File_Read_More;
			}
			// Read All Pending Messages
			if(!pmod->FilePort[i].InBuffer.Busy)
			{	
				pmod->FilePort[i].InBuffer.Busy=GetCurrentThreadId();
				if((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status>=10)&&(pmod->FilePort[i].S5Status<100))
				{
					switch (pmod->FilePort[i].S5Status)
					{
						case 10:
							if(pmod->FilePort[i].InBuffer.LocalSize>=2)
							{
								int Len=2;
								int Version=pmod->FilePort[i].InBuffer.Block[0];
								int Methode=pmod->FilePort[i].InBuffer.Block[1];
								if ((Version!=pmod->FilePort[i].S5Version)||(Methode!=pmod->FilePort[i].S5Methode))
									goto S5FileError;
								WSStripBuff(&pmod->FilePort[i].InBuffer, Len);
								WSHS5FileConnect(pmod,i);
								break;
							}
							break;
						case 20:
							if(pmod->FilePort[i].InBuffer.LocalSize>=4)
							{
								unsigned int Len=4;
								int Version=pmod->FilePort[i].InBuffer.Block[0];
								int Reply=pmod->FilePort[i].InBuffer.Block[1];
								int AdressType=pmod->FilePort[i].InBuffer.Block[3];
								switch(AdressType)
								{
									case 1:
										Len+=4; break;
									default:
									goto S5FileError;
								}
								// AddPort Len
								Len+=2;
								if(pmod->FilePort[i].InBuffer.LocalSize<Len)
									break;
								WSStripBuff(&pmod->FilePort[i].InBuffer, Len);
								pmod->FilePort[i].S5Status=100;
								WSHFileConnected(pmod,i);
								break;
							}
							break;
					}
					goto S5FileDone;
S5FileError:
								_WSHCloseFilePort(pmod,i);
								WSHLogError(pmod,"Socks5 Failed on FIL%d",i);
								continue;
S5FileDone:;
				}
				else
				{
					WSFileTransfer *wft = pmod->FilePort[i].FileTransfer;
					if(wft)
					{
						/*
						if ( wft->status == WSF_COMMIT_INPROGRESS )
							WSHCommitFileTransfer(pmod,i);                       
						else
						{
							for ( int f=0; f<pmod->WSFileOptions.xfersPerTick; f++ )
							{
								if ( wft->status == WSF_COMMIT_INPROGRESS ||
									 WSHRecvFileTransfer(pmod,i)<=0 )
									break;
							}
						}
						*/
						for ( int f=0; f<pmod->WSFileOptions.xfersPerTick; f++ )
						{
							if ( WSHRecvFileTransfer(pmod,i)<=0 )
								break;
						}
						if ( wft->status == WSF_COMMIT_INPROGRESS )
							WSHCommitFileTransfer(pmod,i);                       
					}
					else
					{
//						long tt=GetTickCount();
						while(pmod->WSFileMsgReady(i))
						{
//							if (abs(GetTickCount()-tt) > WS_TIMER_INTERVAL*2)
//								break;
							pmod->WSFileStripMsg(i);
						}
					}
				}
				pmod->FilePort[i].InBuffer.Busy=lastBusy;
			}
			if(!((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status<100)))
				pmod->WSBeforeFileSend(i);
			// Send as much data out as posible
			if((pmod->FilePort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
				||
				(pmod->FilePort[i].OutCompBuffer.LocalSize>0)
#endif
				)
			{
				if(!pmod->FilePort[i].OutBuffer.Busy)
				{
					pmod->FilePort[i].OutBuffer.Busy=GetCurrentThreadId();
					pmod->FilePort[i].SendTimeOut=pmod->FilePort[i].SendTimeOut%1000;
File_Send_More:
#ifdef WS_COMPRESS
					if(pmod->FilePort[i].Compressed&&(!((pmod->FilePort[i].S5Connect)&&(pmod->FilePort[i].S5Status<100))))
					{
						if(pmod->FilePort[i].OutCompBuffer.LocalSize<=0)
						{
							if(pmod->FilePort[i].OutBuffer.LocalSize<=0)
								goto File_Send_Done;
							if(pmod->FilePort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
								CompSize=((PreCompBlock*1024)*99/100);
							else
								CompSize=pmod->FilePort[i].OutBuffer.LocalSize;
							CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
#ifdef WS_ENCRYPTED
							mtcompress(CompBuff,&CompBuffSize
								,pmod->FilePort[i].OutBuffer.Block,CompSize);
							if(pmod->FilePort[i].EncryptionOn)
							{
								WSHEncrypt(pmod,&pmod->FilePort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
									,CompBuff,CompBuffSize,i,WS_FIL);
							}
							else
							{
								EncryptSize=CompBuffSize;
								memcpy(&pmod->FilePort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
							}
							*((unsigned int *)&pmod->FilePort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->FilePort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
							mtcompress(&pmod->FilePort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->FilePort[i].OutBuffer.Block,CompSize);
							*((unsigned int *)&pmod->FilePort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->FilePort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
							WSStripBuff(&pmod->FilePort[i].OutBuffer, CompSize);
						}
						if(pmod->FilePort[i].ChokeSize)
						{
							if((pmod->FilePort[i].ChokeSize-pmod->FilePort[i].LastChokeSize) < pmod->FilePort[i].OutCompBuffer.LocalSize)
								SendBlockSize=(pmod->FilePort[i].ChokeSize-pmod->FilePort[i].LastChokeSize);
							else
								SendBlockSize=pmod->FilePort[i].OutCompBuffer.LocalSize;
						}
						else
							SendBlockSize=pmod->FilePort[i].OutCompBuffer.LocalSize;
						if(SendBlockSize)
						{
				#if defined(WS_OIO)||defined(WS_OTPP)
							Status=WSHSendNonBlock(pmod, WS_FIL, i, pmod->FilePort[i].Sock, pmod->FilePort[i].OutCompBuffer.Block, SendBlockSize);
				#else
							Status=WSHSendPort(pmod->FilePort[i].Sock, pmod->FilePort[i].OutCompBuffer.Block, SendBlockSize, NO_FLAGS_SET );
				#endif
							if(Status>0)
							{
								//ResetSendTimeout(WS_FIL,i);
								WSStripBuff(&pmod->FilePort[i].OutCompBuffer, Status);
								pmod->FilePort[i].BytesOut+=Status;
								pmod->FilePort[i].BlocksOut++;
								pmod->FilePort[i].SendTimeOut=0;
								pmod->FilePort[i].LastChokeSize+=Status;
								goto File_Send_More;
							}
							else //retry's due to Port Busy
							{
								int lerr=WSAGetLastError();
								if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
									pmod->FilePort[i].SendTimeOut++;
								else
								{
									if(lerr==WSAECONNRESET)
										WSHLogEvent(pmod,"!WSOCKS: FilePort[%d][%s] Reset by Peer",i,pmod->FilePort[i].CfgNote);
									else
										WSHLogEvent(pmod,"!WSOCKS: FilePort[%d][%s] Send Failed: %d",i,pmod->FilePort[i].CfgNote,lerr);
									_WSHCloseFilePort(pmod,i);
								}
							}
						}
					}
					else
#endif
					{
						if(pmod->FilePort[i].OutBuffer.LocalSize>((unsigned int)((pmod->FilePort[i].BlockSize-1)*1024)))
							SendSize=((pmod->FilePort[i].BlockSize-1)*1024);
						else
							SendSize=pmod->FilePort[i].OutBuffer.LocalSize;
						if(pmod->FilePort[i].ChokeSize)
						{
							if((int)(pmod->FilePort[i].ChokeSize-pmod->FilePort[i].LastChokeSize) < SendSize)
								SendBlockSize=(pmod->FilePort[i].ChokeSize-pmod->FilePort[i].LastChokeSize);
							else
								SendBlockSize=SendSize;
						}
						else
							SendBlockSize=SendSize;
						if(SendBlockSize)
						{
				#if defined(WS_OIO)||defined(WS_OTPP)
							Status=WSHSendNonBlock(pmod, WS_FIL, i, pmod->FilePort[i].Sock, pmod->FilePort[i].OutBuffer.Block, SendBlockSize);
				#else
							Status=WSHSendPort(pmod->FilePort[i].Sock, pmod->FilePort[i].OutBuffer.Block, SendBlockSize, NO_FLAGS_SET );
				#endif
							if(Status>0)
							{
								//ResetSendTimeout(WS_FIL,i);
								WSStripBuff(&pmod->FilePort[i].OutBuffer, Status);
								pmod->FilePort[i].BytesOut+=Status;
								pmod->FilePort[i].BlocksOut++;
								pmod->FilePort[i].SendTimeOut=0;
								pmod->FilePort[i].LastChokeSize+=Status;
								if(Status<SendSize)
									goto File_Send_Done;
								if (pmod->FilePort[i].OutBuffer.Size>0)
									goto File_Send_More;
							}
							else //retry's due to Port Busy
							{
								int lerr=WSAGetLastError();
								if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
									pmod->FilePort[i].SendTimeOut++;
								else
								{
									if(lerr==WSAECONNRESET)
										WSHLogEvent(pmod,"!WSOCKS: FilePort[%d][%s] Reset by Peer",i,pmod->FilePort[i].CfgNote);
									else
										WSHLogEvent(pmod,"!WSOCKS: FilePort[%d][%s] Send Failed: %d",i,pmod->FilePort[i].CfgNote,lerr);
									_WSHCloseFilePort(pmod,i);
								}
							}
						}
					}
File_Send_Done:
					pmod->FilePort[i].OutBuffer.Busy=lastBusy;
				}
				else // retry's due to BuffBusy
				{
					pmod->FilePort[i].SendTimeOut++;
				}
				if(pmod->FilePort[i].SendTimeOut>WS_FILE_TIMEOUT)
				{
					_WSHCloseFilePort(pmod,i);
				}
			}
//			pmod->FilePort[i].SinceLastBeatCount++;
			if(pmod->FilePort[i].Bounce)
			{
				DWORD l=GetTickCount();
				if(pmod->FilePort[i].LastDataTime)
				{
					if((l-pmod->FilePort[i].LastDataTime)>pmod->FilePort[i].Bounce)
					{
						while (TRUE) // Added to please KMA and not use a GOTO  :-)
						{
							long x = (WSHTime/100);

							if ((pmod->FilePort[i].BounceStart)&&((WSHTime/100)<pmod->FilePort[i].BounceStart))
								break;

							if ((pmod->FilePort[i].BounceEnd)&&((WSHTime/100)>pmod->FilePort[i].BounceEnd))
								break;

							WSHLogError(pmod,"!WSOCKS: Conport[%d], was Bounced last data receved %ldms ago",i,(l-pmod->FilePort[i].LastDataTime));
							_WSHCloseFilePort(pmod,i);
							break;
						}
					}
				}
				else
					pmod->FilePort[i].LastDataTime=l;
			}
		}
    }
#endif//!WS_OIO
	return 0;
}

// A dynamic dialog item
#define DLGITEM(idx,tlen) \
	DWORD DI ## idx ## style; \
	DWORD DI ## idx ## dwExtendedStyle; \
	short DI ## idx ## x; \
	short DI ## idx ## y; \
	short DI ## idx ## cx; \
	short DI ## idx ## cy; \
	WORD  DI ## idx ## id; \
	WORD  DI ## idx ## Class[2]; \
	WORD  DI ## idx ## Text[ ## tlen ## ]; \
	WORD  DI ## idx ## CreationData;
// Dynamic dialog item with classname
#define DLGITEM2(idx,clen,tlen) \
	DWORD DI ## idx ## style; \
	DWORD DI ## idx ## dwExtendedStyle; \
	short DI ## idx ## x; \
	short DI ## idx ## y; \
	short DI ## idx ## cx; \
	short DI ## idx ## cy; \
	WORD  DI ## idx ## id; \
	WORD  DI ## idx ## Class[ ## clen ## ]; \
	WORD  DI ## idx ## Text[ ## tlen ## ]; \
	WORD  DI ## idx ## CreationData;

// Wait for the transfer to finish
int WsocksHostImpl::WSHWaitFile(WsocksApp *pmod, WSFileTransfer *wft, int timeout, HANDLE *fhnd)
{
	// The progress dialog
	static struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title[23];
		WORD PointSize;
		WORD Font[14];
		DLGITEM(1,28)
		DLGITEM(2,7)
		DLGITEM(3,7)
		DLGITEM(4,5)
	} WSProgressTemplate = {DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT ,0,4,0,0,298,50,0,0, L"File Transfer Progress", 8,L"MS Sans Serif"
							,WS_VISIBLE,0,7,11,230,8,IDC_MSG,{0xffff,0x0082}, L"Downloading <file_path>...",0
							,SS_BLACKRECT,0,7,23,230,20,IDC_PROGRESS,{0xffff,0x0082},L"Static",0
							,WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,0,241,7,50,14,IDCANCEL,{0xffff,0x0080},L"Cancel",0
							,WS_VISIBLE,0,241,29,50,8,IDC_PERCENT,{0xffff,0x0082},L"100%",0
							};

	// If local master and the file already exists, then we never sent a file request
	//if ( wft->opts.localMaster && wft->uptodate )
	//	;
 //   // Wait for file transfer to complete
 //   // Use progress dialog to block UI thread, but the dialog box's
 //   // window function to send the WM_TIMER
	//else if ( timeout == INFINITE )
	//{
	//	DialogBoxIndirectParam(pmod->WShInst, (LPDLGTEMPLATE)&WSProgressTemplate, 
	//		pmod->WShWnd, (DLGPROC)::cbProgress, (LPARAM)wft);
	//}
	//// Otherwise, wait till the timeout and send WM_TIMER manually
	//else
	//{
	//	DWORD tstart = GetTickCount();
	//	DWORD lcheck = tstart;
	//	while ( wft->status == WSF_INIT ||
	//			wft->status == WSF_REQUESTED ||
	//			wft->status == WSF_RECV_INPROGRESS ||
	//			wft->status == WSF_COMMIT_INPROGRESS )
	//	{
	//		DWORD tnow = GetTickCount();
	//		if ( !timeout || (int)(tnow -tstart)>timeout )
	//			return WSF_TIMEOUT;
	//		SleepEx(WS_TIMER_INTERVAL, true);
	//		if ( tnow -lcheck > 1000 )
	//		{
	//			WSHCheckTime(); lcheck = tnow;
	//		}
	//		//WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
	//		WSHFileTransferLoop(pmod);
	//	}
	//}
	if(pmod->initialized>0)
		wft->xthnd=CreateThread(0,0,BootFileTransferThread,wft,0,&wft->xtid);
	else
		wft->xthnd=CreateThread(0,0,BootPIFileTransferThread,wft,0,&wft->xtid);
	// Implementation now uses separate thread for xfer
	DialogBoxIndirectParam(pmod->WShInst, (LPDLGTEMPLATE)&WSProgressTemplate, 
		pmod->WShWnd, (DLGPROC)::cbProgress, (LPARAM)wft);

	// Set the file's time to the remote time received in start packet
	if ( wft->thnd != INVALID_HANDLE_VALUE )
		SetFileTime(wft->thnd, &wft->fdata.ftCreationTime,
			&wft->fdata.ftLastAccessTime, &wft->fdata.ftLastWriteTime);

    // If we're writing, then return a handle to write the temp file
    if ( wft->dwDesiredAccess &GENERIC_WRITE ) 
    {
        switch ( wft->api )
        {
        case FTAPI_FOPEN:
            if ( wft->thnd != INVALID_HANDLE_VALUE )
            {
                CloseHandle(wft->thnd); wft->thnd = INVALID_HANDLE_VALUE;
            }
			// Our local copy is up-to-date, so we have to copy our
			// persistent copy to the temp location for write.
			if ( wft->uptodate )
			{
				if ( !CopyFile(wft->localPath, wft->tempPath, false) )
					wft->status = WSF_FAILED;
			}
			if ( wft->status != WSF_FAILED )
			{
				*fhnd = wft->fhnd = fopen(wft->tempPath, wft->mode);
				if ( !wft->fhnd ) 
				{
					wft->status = WSF_FAILED;
					wft->fhnd = INVALID_HANDLE_VALUE;
				}
			}
            break;
        case FTAPI_OPENFILE:
            if ( wft->thnd != INVALID_HANDLE_VALUE )
            {
                CloseHandle(wft->thnd); wft->thnd = INVALID_HANDLE_VALUE;
            }
			// Our local copy is up-to-date, so we have to copy our
			// persistent copy to the temp location for write.
			if ( wft->uptodate )
			{
				if ( !CopyFile(wft->localPath, wft->tempPath, false) )
					wft->status = WSF_FAILED;
			}
			if ( wft->status != WSF_FAILED )
			{
				*fhnd = wft->fhnd = (HANDLE)(PTRCAST)OpenFile(wft->tempPath, &wft->ReOpenBuff, wft->uStyle);
				if ( wft->fhnd == (HANDLE)HFILE_ERROR ) 
				{
					wft->status = WSF_FAILED;
					wft->fhnd = INVALID_HANDLE_VALUE;
				}
			}
            break;
        case FTAPI_TRANSMITFILE:
        case FTAPI_PUTFILE:
            if ( wft->thnd != INVALID_HANDLE_VALUE )
            {
                CloseHandle(wft->thnd); wft->thnd = INVALID_HANDLE_VALUE;
            }
            *fhnd = wft->fhnd;
            break;
        case FTAPI_CREATEFILE:
        case FTAPI_GETFILE:
        default:
            *fhnd = wft->fhnd = wft->thnd; wft->thnd = INVALID_HANDLE_VALUE;
            break;
        }
        if ( (wft->status == WSF_FAILED ||
              wft->status == WSF_CANCELLED ||
              wft->status == WSF_TIMEOUT) && !wft->gdWriteOnly )
            return wft->status;
        return WSF_SUCCESS;
    }
    // Close the temp file for write
    if ( wft->thnd != INVALID_HANDLE_VALUE )
    {
        CloseHandle(wft->thnd); wft->thnd = INVALID_HANDLE_VALUE;
    }

    // Rename temp file on success or delete on failure
    switch ( wft->status )
    {
    case WSF_SUCCESS:
        // There might already be one there... back it up
		if ( wft->uptodate )
		{
		#ifdef FILE_SERVER_EVENTS
			WSHLogEvent(pmod,"WSOCKS: File \"%s\" (%s bytes) up-to-date.", 
				wft->remotePath, pmod->SizeStr(wft->fsize));
		#endif
		}
		else
		{
			if ( wft->opts.keepLocal || wft->opts.cacheLocal )
				WSHReplaceFile(pmod,wft->tempPath, wft->localPath);
			else
				strcpy(wft->localPath, wft->tempPath);
		#ifdef FILE_SERVER_EVENTS
			WSHLogEvent(pmod,"WSOCKS: File transfer of \"%s\" (%s bytes) completed.", 
				wft->remotePath, pmod->SizeStr(wft->fsize));
		#endif
		}
        break;
    case WSF_FAILED:
        DeleteFile(wft->tempPath);
        WSHLogError(pmod,"!WSOCKS: File transfer of \"%s\" (%s/%s) failed!.", 
            wft->remotePath, wft->status == WSF_COMMIT_INPROGRESS ?pmod->SizeStr(wft->sent) :pmod->SizeStr(wft->recvd), pmod->SizeStr(wft->fsize));
        strcpy(wft->localPath, wft->remotePath);
        if ( wft->opts.keepLocal || wft->opts.cacheLocal )
        {
		#ifdef FILE_SERVER_EVENTS
            WSHLogEvent(pmod,"!WSOCKS: Using local copy at \"%s\".", wft->localPath);
		#endif
            break;
        }
        return wft->status;
    case WSF_CANCELLED:
        DeleteFile(wft->tempPath);
        WSHLogError(pmod,"!WSOCKS: File transfer of \"%s\" (%s/%s) cancelled..", 
            wft->remotePath, wft->status == WSF_COMMIT_INPROGRESS ?pmod->SizeStr(wft->sent) :pmod->SizeStr(wft->recvd), pmod->SizeStr(wft->fsize));
        if ( wft->opts.keepLocal || wft->opts.cacheLocal )
        {
		#ifdef FILE_SERVER_EVENTS
            WSHLogError(pmod,"!WSOCKS: Using local copy at \"%s\".", wft->localPath);
		#endif
            break;
        }
        return wft->status;
    }

    // Try opening the local file, even on failure to use any previous copy
    if ( !*wft->localPath )
        strcpy(wft->localPath, wft->remotePath);
    if ( *wft->localPath )
    {
        // Make sure to open with caller's parameters
        switch ( wft->api )
        {
        case FTAPI_FOPEN:
            *fhnd = wft->fhnd = (HANDLE)fopen(wft->localPath, wft->mode);
            if ( !wft->fhnd ) wft->fhnd = INVALID_HANDLE_VALUE;
            break;
        case FTAPI_OPENFILE:
            *fhnd = wft->fhnd = (HANDLE)(PTRCAST)OpenFile(wft->localPath, &wft->ReOpenBuff, wft->uStyle);
            if ( wft->fhnd == (HANDLE)HFILE_ERROR ) wft->fhnd = INVALID_HANDLE_VALUE;
            break;
        case FTAPI_CREATEFILE:
        case FTAPI_GETFILE:
        case FTAPI_TRANSMITFILE:
        case FTAPI_PUTFILE:
        default:
            *fhnd = wft->fhnd = CreateFile(
                wft->localPath, wft->dwDesiredAccess, wft->dwShareMode, 
                &wft->SecurityAttributes, wft->dwCreationDisposition, wft->dwFlagsAndAttributes, 
                wft->hTemplateFile);
        }
    }
	if ( wft->xthnd )
		WaitForSingleObject(wft->xthnd,100);
    return wft->status;
}

// Cancel the transfer: wsfhnd=pmod->FilePort[].FileTransfer
BOOL WsocksHostImpl::WSHCancelFile(WsocksApp *pmod, WSFileTransfer *wft)
{
    if ( wft )
    {
        if ( wft->status == WSF_SUCCESS ||
             wft->status == WSF_FAILED ||
             wft->status == WSF_CANCELLED ||
             wft->status == WSF_TIMEOUT )
        {
            WSHLogError(pmod,"WSOCKS: Cancel too late for \"%s\".", wft->remotePath);
            return FALSE;
        }
	#ifdef FILE_SERVER_EVENTS
        WSHLogEvent(pmod,"WSOCKS: Cancel file transfer of \"%s\".", wft->remotePath);
	#endif
        wft->status = WSF_CANCELLED;
        if (wft->progWnd)
            PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);

        // Send a recv ack
        MSG599REC Msg599Rec;
        memset(&Msg599Rec, 0, sizeof(Msg599Rec));
        Msg599Rec.MsgVersion = 2;
        Msg599Rec.FileOffset = wft->sent;
        Msg599Rec.FileSize = wft->fsize;
        Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
        Msg599Rec.DataType = 0x03;
        Msg599Rec.ErrorCode = ERROR_CANCELLED;
        //strcpy(Msg599Rec.ErrorStr, strerror(Msg599Rec.ErrorCode));
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, Msg599Rec.ErrorCode, 0, 
            Msg599Rec.ErrorStr, sizeof(Msg599Rec.ErrorStr), 0);
        WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);
    }
    return TRUE;
}

// Right now, we never return WSFileTransfer.
BOOL WsocksHostImpl::WSHCloseFile(WsocksApp *pmod, HANDLE hObject, BOOL writeBack)
{
	// The progress dialog
	static struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title[23];
		WORD PointSize;
		WORD Font[14];
		DLGITEM(1,28)
		DLGITEM(2,7)
		DLGITEM(3,7)
		DLGITEM(4,5)
	} WSProgressTemplate = {DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT ,0,4,0,0,298,50,0,0, L"File Transfer Progress", 8,L"MS Sans Serif"
							,WS_VISIBLE,0,7,11,230,8,IDC_MSG,{0xffff,0x0082}, L"Downloading <file_path>...",0
							,SS_BLACKRECT,0,7,23,230,20,IDC_PROGRESS,{0xffff,0x0082},L"Static",0
							,WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,0,241,7,50,14,IDCANCEL,{0xffff,0x0080},L"Cancel",0
							,WS_VISIBLE,0,241,29,50,8,IDC_PERCENT,{0xffff,0x0082},L"100%",0
							};

	WSFileTransfer *wft;
    for ( wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->fhnd == hObject ||
             wft == hObject )
            break;
    }
    if ( !wft )
        return FALSE;

    if ( wft->thnd != INVALID_HANDLE_VALUE )
    {
        CloseHandle(wft->thnd); wft->thnd = INVALID_HANDLE_VALUE;
    }
    if ( wft->xthnd != INVALID_HANDLE_VALUE )
    {
        CloseHandle(wft->xthnd); wft->xthnd = 0; wft->xtid=0;
    }
    switch ( wft->api )
    {
    case FTAPI_FOPEN:
        if ( wft->fhnd != INVALID_HANDLE_VALUE )
        {
            fclose((FILE *)wft->fhnd); wft->fhnd = INVALID_HANDLE_VALUE;
        }
        break;
    case FTAPI_OPENFILE:
        if ( wft->fhnd != INVALID_HANDLE_VALUE )
        {
            _lclose((HFILE)(PTRCAST)wft->fhnd); wft->fhnd = INVALID_HANDLE_VALUE;
        }
        break;
    case FTAPI_TRANSMITFILE:
    case FTAPI_PUTFILE:
        break;
    case FTAPI_CREATEFILE:
    case FTAPI_GETFILE:
    default:
        if ( wft->fhnd != INVALID_HANDLE_VALUE )
        {
            CloseHandle(wft->fhnd); wft->fhnd = INVALID_HANDLE_VALUE;
        }
    }

	// LL 20080318 DT3239 - if we opened for write while request was aborted,
	// we need to progress past the WSF_REQUESTED state for server to
	// properly release the transfer resources
	if ((wft->dwDesiredAccess & GENERIC_WRITE) &&
		(wft->status == WSF_REQUESTED) && !writeBack)
	{
		bool bChanged;
		DWORD dwTick = ::GetTickCount();
		do
		{
			SleepEx(WS_TIMER_INTERVAL, TRUE);
			WSHFileTransferLoop(pmod);
			bChanged = (wft->status != WSF_REQUESTED) &&
				(wft->status != WSF_RECV_INPROGRESS);
		} while (!bChanged && ((::GetTickCount() - dwTick) < 15000));

		// close the port if it took too long to wait
		if (!bChanged)
			_WSHCloseFilePort(pmod,wft->PortNo);
	}

	// We opened for write, so the fileserver is waiting for us
    if ( (wft->dwDesiredAccess &GENERIC_WRITE) &&
		 (wft->status == WSF_WAIT_COMMIT || wft->gdWriteOnly) )
    {
		// Cancel the commit
		if ( !writeBack )
		{
            MSG599REC Msg599Rec;
            memset(&Msg599Rec, 0, sizeof(Msg599Rec));
            Msg599Rec.MsgVersion = 2;
            Msg599Rec.FileOffset = 0;
            Msg599Rec.FileSize = wft->fsize;
            Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
            Msg599Rec.DataType = 0x03;
            Msg599Rec.ErrorCode = 0;
            wft->status = WSF_SUCCESS;
            WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);

            if ( pmod->FilePort[wft->PortNo].FileTransfer == wft )
                pmod->FilePort[wft->PortNo].FileTransfer = 0;
			DeleteFile(wft->tempPath);
            return TRUE;
		}

		// Move to commit directory when guaranteed
		bool commitDir = false;
		if ( wft->opts.guaranteeCommit )
		{
			char cpath[MAX_PATH];
			memset(cpath, 0, sizeof(cpath));
			WSHGetFileCommitPath(pmod,wft->remotePath, cpath, true);
			if ( wft->api == FTAPI_TRANSMITFILE ||
				 wft->api == FTAPI_PUTFILE )
			{
				if ( CopyFile(wft->localPath, cpath, false) )
				{
					strcpy(wft->tempPath, cpath);
					commitDir = true;
				}
				// Failed gdWriteOnly stops out here
				if ( wft->status != WSF_WAIT_COMMIT )
				{
					if ( pmod->FilePort[wft->PortNo].FileTransfer == wft )
						pmod->FilePort[wft->PortNo].FileTransfer = 0;
					return FALSE;
				}
			}
			else
			{
				if ( WSHReplaceFile(pmod,wft->tempPath, cpath) )
				{
					strcpy(wft->tempPath, cpath);
					commitDir = true;
				}
			}
		}

		// We never requested the file, so do a write-only transmit
		if ( wft->opts.localMaster && wft->uptodate )
		{
            if ( pmod->FilePort[wft->PortNo].FileTransfer == wft )
                pmod->FilePort[wft->PortNo].FileTransfer = 0;
			FileOptions fopts=wft->opts;
			fopts.localMaster = false;
			fopts.keepLocal = false;
			fopts.cacheLocal = false;
			int rc = WSHTransmitFile(pmod,wft->tempPath, wft->remotePath, 
				&wft->fdata.ftCreationTime, &wft->fdata.ftLastAccessTime, 
				&wft->fdata.ftLastWriteTime, &fopts);
			if ( !commitDir )
				DeleteFile(wft->tempPath);
			return rc;
		}

		// Open the temp file
        wft->commitAttempted = true;
        if ( (wft->api != FTAPI_TRANSMITFILE &&
			  wft->api != FTAPI_PUTFILE) ||
			 commitDir )
		{
			if ( wft->fhnd != INVALID_HANDLE_VALUE )
				CloseHandle(wft->fhnd);
            wft->fhnd = CreateFile(wft->tempPath, GENERIC_READ, FILE_SHARE_READ,
                0, OPEN_EXISTING, 0, 0);
		}
		// Get the file times unless it's been set to something else
		if ( wft->fhnd != INVALID_HANDLE_VALUE && !wft->sftCalled )
			GetFileTime(wft->fhnd, &wft->fdata.ftCreationTime, 
				&wft->fdata.ftLastAccessTime, &wft->fdata.ftLastWriteTime);

        // Failed starting commit
        if ( wft->fhnd == INVALID_HANDLE_VALUE )
        {
            MSG599REC Msg599Rec;
            memset(&Msg599Rec, 0, sizeof(Msg599Rec));
            Msg599Rec.MsgVersion = 2;
            Msg599Rec.FileOffset = 0;
            Msg599Rec.FileSize = wft->fsize;
            Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
            Msg599Rec.DataType = 0x03;
            Msg599Rec.ErrorCode = GetLastError();
            strcpy(Msg599Rec.ErrorStr, strerror(Msg599Rec.ErrorCode));
            wft->status = WSF_FAILED;
            WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);

            WSHLogError(pmod,"WSOCKS: Failed committing \"%s\" to file server!", wft->remotePath);
            if ( pmod->FilePort[wft->PortNo].FileTransfer == wft )
                pmod->FilePort[wft->PortNo].FileTransfer = 0;
			if ( !commitDir )
				DeleteFile(wft->tempPath);
            return TRUE;
        }
        // Begin the commit
        else
        {
            wft->fsize = GetFileSize(wft->fhnd, 0);
            wft->status = WSF_COMMIT_INPROGRESS;
            wft->sent = 0;
            MSG599REC Msg599Rec;
            memset(&Msg599Rec, 0, sizeof(MSG599REC));
            Msg599Rec.MsgVersion = 2;
            Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
            Msg599Rec.FileOffset = 0;
            Msg599Rec.FileSize = wft->fsize;
            Msg599Rec.DataType = 0x01;
            Msg599Rec.fdata.ftCreationTime = wft->fdata.ftCreationTime;
            Msg599Rec.fdata.ftLastAccessTime = wft->fdata.ftLastAccessTime;
            Msg599Rec.fdata.ftLastWriteTime = wft->fdata.ftLastWriteTime;
            strcpy(Msg599Rec.FilePath, wft->remotePath);
            // For now, no change of attributes allowed on commit
            //memcpy(&Msg599Rec.fdata, &pfe->fdata, sizeof(WIN32_FIND_DATA));
            WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);

		#ifdef FILE_SERVER_EVENTS
            WSHLogEvent(pmod,"WSOCKS: Committing \"%s\" (%s bytes) to file server...", 
                wft->remotePath, pmod->SizeStr(wft->fsize));
		#endif
            // Must be modal, because WSCloseFile shouldn't return until 
            // the commit has completed.
            int rc = DialogBoxIndirectParam(pmod->WShInst, (LPDLGTEMPLATE)&WSProgressTemplate, 
				pmod->WShWnd, (DLGPROC)::cbProgress, (LPARAM)wft) == IDOK ?TRUE :FALSE;
            if ( pmod->FilePort[wft->PortNo].FileTransfer == wft )
                pmod->FilePort[wft->PortNo].FileTransfer = 0;
			if ( !rc || !commitDir )
				DeleteFile(wft->tempPath);
            return rc;
        }
		// Never gets here
    }
    else
    {
        if ( pmod->FilePort[wft->PortNo].FileTransfer == wft )
            pmod->FilePort[wft->PortNo].FileTransfer = 0;
        DeleteFile(wft->tempPath);
    }
    return TRUE;
}
int WsocksHostImpl::WSHRecvFileTransfer(WsocksApp *pmod, int PortNo)
{
    WSFileTransfer *wft = pmod->FilePort[PortNo].FileTransfer;
	// Tired of waiting for an ack from fileserver
    if((wft)&&(wft->status == WSF_WAIT_COMMIT_ACK))
    {
		if(!wft->waitCommitTimer)
			wft->waitCommitTimer=GetTickCount();
        else if ( GetTickCount() -wft->waitCommitTimer >= (DWORD)wft->opts.commitTimeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[wft->PortNo].FileTransfer = 0;
            WSHLogError(pmod,"!WSOCKS: Commit timeout for \"%s\"!", wft->remotePath);
            if (wft->progWnd)
                PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
        }
    }
	//else if((wft)&&(wft->status == WSF_COMMIT_INPROGRESS))
	//	WSHCommitFileTransfer(pmod,PortNo);                       

    if(pmod->FilePort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
        return 0;
	MSGHEADER MsgHeader;
	memcpy((char *)&MsgHeader,pmod->FilePort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(pmod->FilePort[PortNo].InBuffer.Size<(sizeof(MSGHEADER)+MsgHeader.MsgLen))
		return 0;

	WSStripBuff(&pmod->FilePort[PortNo].InBuffer,sizeof(MSGHEADER));
	char *Msg = (char*) GlobalAllocPtr(GPTR,MsgHeader.MsgLen);
	memcpy(Msg,pmod->FilePort[PortNo].InBuffer.Block,MsgHeader.MsgLen);
	WSStripBuff(&pmod->FilePort[PortNo].InBuffer,MsgHeader.MsgLen);
	pmod->FilePort[PortNo].PacketsIn++;

    int rc = -1;
    switch ( MsgHeader.MsgID )
    {
    case 16:
		pmod->FilePort[PortNo].BeatsIn++;
		break;

    case 599:
        {
        MSG599REC *Msg599Rec = (MSG599REC *)Msg;
		if(!wft)
			break;
        if ( Msg599Rec->MsgVersion == 1 || Msg599Rec->MsgVersion == 2 )
        {
            // Download start packet
            if ( Msg599Rec->DataType == 0x01 )
            {
                wft->status = WSF_RECV_INPROGRESS;
                wft->fsize = Msg599Rec->fdata.nFileSizeLow;
                wft->vhnd = (HANDLE)(PTRCAST)Msg599Rec->FileHandle;
                memcpy(&wft->fdata, &Msg599Rec->fdata, sizeof(WIN32_FIND_DATA));
                rc = 1;
				// We have an up-to-date file, so skip the recv
				if ( Msg599Rec->MsgVersion == 2 && Msg599Rec->uptodate )
                {
                    MSG599REC Msg599Rec;
                    memset(&Msg599Rec, 0, sizeof(Msg599Rec));
                    Msg599Rec.MsgVersion = 2;
                    Msg599Rec.FileOffset = wft->sent;
                    Msg599Rec.FileSize = wft->fsize;
                    Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
                    Msg599Rec.DataType = 0x03;
                    Msg599Rec.ErrorCode = 0;
					Msg599Rec.uptodate2 = 1;
                    WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);
                    if ( wft->dwDesiredAccess &GENERIC_WRITE )
                    {
                        wft->status = WSF_WAIT_COMMIT;
					#ifdef FILE_SERVER_EVENTS
                        WSHLogEvent(pmod,"WSOCKS: File transfer of \"%s\" (%s bytes) completed.",
                            wft->remotePath, pmod->SizeStr(wft->recvd));
					#endif
                    }
                    else
                    {
                        wft->status = WSF_SUCCESS;
                        pmod->FilePort[wft->PortNo].FileTransfer = 0;
                    }
                    rc = 0;

					wft->uptodate = true;
					if ( wft->thnd != INVALID_HANDLE_VALUE )
					{
						CloseHandle(wft->thnd); wft->thnd = INVALID_HANDLE_VALUE;
						DeleteFile(wft->tempPath);
					}
                }
            }
            // Download data packet
            else if ( Msg599Rec->DataType == 0x02 )
            {
                if ( Msg599Rec->DataLen == 0 )
                {
                    // Send a recv ack if we didn't cancel
                    if ( wft->status == WSF_CANCELLED )
                    {
                        pmod->FilePort[wft->PortNo].FileTransfer = 0;
                        rc = -1;
                    }
                    else
                    {
                        MSG599REC Msg599Rec;
                        memset(&Msg599Rec, 0, sizeof(Msg599Rec));
                        Msg599Rec.MsgVersion = 2;
                        Msg599Rec.FileOffset = wft->sent;
                        Msg599Rec.FileSize = wft->fsize;
                        Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
                        Msg599Rec.DataType = 0x03;
                        Msg599Rec.ErrorCode = 0;
                        WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);
                        if ( wft->dwDesiredAccess &GENERIC_WRITE )
                        {
                            wft->status = WSF_WAIT_COMMIT;
						#ifdef FILE_SERVER_EVENTS
                            WSHLogEvent(pmod,"WSOCKS: File transfer of \"%s\" (%s bytes) completed.",
                                wft->remotePath, pmod->SizeStr(wft->recvd));
						#endif
                        }
                        else
                        {
                            wft->status = WSF_SUCCESS;
                            pmod->FilePort[wft->PortNo].FileTransfer = 0;
                        }
                        rc = 0;
                    }
                }
                else
                {
                    wft->recvd += Msg599Rec->DataLen;
                    DWORD wbytes = 0;
                    WriteFile(wft->thnd, Msg599Rec->FileData, Msg599Rec->DataLen, &wbytes, 0);
                    rc = 1;
                }
            }
            // Status packet
            else if ( Msg599Rec->DataType == 0x03 )
            {
                if ( wft->api == FTAPI_CREATEFILE ||
					 wft->api == FTAPI_GETFILE ||
                     wft->api == FTAPI_TRANSMITFILE ||
					 wft->api == FTAPI_PUTFILE ||
                     wft->api == FTAPI_FOPEN ||
                     wft->api == FTAPI_OPENFILE )
                {
                    // Download or commit failed
                    if ( Msg599Rec->ErrorCode )
                    {
                        wft->status = WSF_FAILED;
                        pmod->FilePort[wft->PortNo].FileTransfer = 0;
                        if ( wft->commitAttempted )
                            WSHLogError(pmod,"!WSOCKS: Commit failed  for \"%s\" with error %d=%s.", 
                                wft->remotePath, Msg599Rec->ErrorCode, Msg599Rec->ErrorStr);
                        else
                            WSHLogError(pmod,"!WSOCKS: Download failed for \"%s\" with error %d=%s.", 
                                wft->remotePath, Msg599Rec->ErrorCode, Msg599Rec->ErrorStr);
                        rc = -1;
                    }
                    // Commit succeeded
                    else 
                    {
                        if ( wft->status == WSF_WAIT_COMMIT_ACK ||
							 wft->status == WSF_COMMIT_INPROGRESS )
                        {
						#ifdef FILE_SERVER_EVENTS
							if ( wft->status == WSF_COMMIT_INPROGRESS )
								WSHLogEvent(pmod,"WSOCKS: Commit of \"%s\" (%s bytes) up-to-date.", 
									wft->remotePath, pmod->SizeStr(wft->fsize));
							else
								WSHLogEvent(pmod,"WSOCKS: Commit of \"%s\" (%s bytes) completed.", 
									wft->remotePath, pmod->SizeStr(wft->fsize));
						#endif
                            wft->status = WSF_SUCCESS;
                            pmod->FilePort[wft->PortNo].FileTransfer = 0;
                            if ( wft->opts.keepLocal || wft->opts.cacheLocal )
							{
								if ( wft->api == FTAPI_TRANSMITFILE ||
									 wft->api == FTAPI_PUTFILE )
									DeleteFile(wft->tempPath);
								else
									WSHReplaceFile(pmod,wft->tempPath, wft->localPath);
							}
                            else
                            {
								if ( wft->api != FTAPI_TRANSMITFILE &&
									 wft->api != FTAPI_PUTFILE )
									DeleteFile(wft->localPath);
                                DeleteFile(wft->tempPath);
                            }
                        }
                        rc = 0;
                    }
                }
                // FindClose result
                else if ( wft->api == FTAPI_FINDFIRSTFILE )
                {
                    if ( Msg599Rec->ErrorCode )
                    {
                        wft->status = WSF_FAILED;
                        pmod->FilePort[wft->PortNo].FileTransfer = 0;
                        rc = -1;
                    }
                    else
                    {
                        wft->status = WSF_SUCCESS;
                        pmod->FilePort[wft->PortNo].FileTransfer = 0;
                        rc = 0;
                    }
                }
            }
            // FindFirst/FindNext data
            else if ( Msg599Rec->DataType == 0x04 )
            {
                wft->status = WSF_RECV_INPROGRESS;
                wft->vhnd = (HANDLE)(PTRCAST)Msg599Rec->FileHandle;
                if ( Msg599Rec->eof )
                    wft->status = WSF_SUCCESS;
                else
                    memcpy(&wft->fdata, &Msg599Rec->fdata2, sizeof(WIN32_FIND_DATA));
                rc = 0;
            }
            // Existence packet
            else if ( Msg599Rec->DataType == 0x06 )
            {
                wft->status = WSF_SUCCESS;
                wft->exists = Msg599Rec->exists ?true :false;
                pmod->FilePort[wft->PortNo].FileTransfer = 0;
                rc = 0;
            }
        }
        if (wft->progWnd)
            PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
        }
        break;
#ifdef WS_FILE_SMPROXY
	// Smproxy protocol
	case 1109:
	{
		WSDecodeHint hint={WS_FIL,PortNo,pmod};
		const char *mptr=Msg;
		ccodec.Decode(mptr,MsgHeader.MsgLen,false,this,&hint);
		break;
	}
	case 1111:
	{
		int rid=0;
		char fkey[256]={0};
		char fpath[MAX_PATH]={0};
		char dtid[256]={0},oboi[256]={0};
		if(!WSHRecvFile(pmod,Msg,MsgHeader.MsgLen,WS_FIL,PortNo,
				&rid,fkey,sizeof(fkey),fpath,MAX_PATH,dtid,sizeof(dtid),oboi,sizeof(oboi)))
		{
			//ShellExecute(WShWnd,"open","notepad.exe",fpath,0,SW_SHOW);
		}
		break;
	}
#endif
    }

	GlobalFreePtr(Msg);
    return rc;
}
int WsocksHostImpl::WSHCommitFileTransfer(WsocksApp *pmod, int PortNo)
{
    WSFileTransfer *wft = pmod->FilePort[PortNo].FileTransfer;
    if ( !wft )
        return -1;
    for ( int i=0; i<pmod->WSFileOptions.xfersPerTick; i++ )
    {
        // If the source file is still open (WSTransmitFile), then
        // the size may grow past when we determined the size
        //if ( wft->status == WSF_COMMIT_INPROGRESS && wft->sent<=wft->fsize )
        if ( wft->status == WSF_COMMIT_INPROGRESS )
        {
            MSG599REC Msg599Rec;
            memset(&Msg599Rec, 0, sizeof(Msg599Rec));
            Msg599Rec.MsgVersion = 2;
            Msg599Rec.FileOffset = wft->sent;
            Msg599Rec.FileSize = wft->fsize;
            Msg599Rec.FileHandle = (u_int)(PTRCAST)wft->vhnd;
            DWORD rbytes = 0;
            if ( !ReadFile(wft->fhnd, Msg599Rec.FileData, sizeof(Msg599Rec.FileData), &rbytes, 0) )
            {
                Msg599Rec.DataType = 0x03;
                //memcpy(&Msg599Rec.fdata, &wft->pfe->fdata, sizeof(WIN32_FIND_DATA));
                Msg599Rec.ErrorCode = GetLastError();
                strcpy(Msg599Rec.ErrorStr, strerror(Msg599Rec.ErrorCode));
                wft->status = WSF_FAILED;
                pmod->FilePort[wft->PortNo].FileTransfer = 0;
                CloseHandle(wft->fhnd); wft->fhnd = INVALID_HANDLE_VALUE;
                WSHLogError(pmod,"WSOCKS: Commit of \"%s\" (offset=%d) failed!", 
                    wft->remotePath, wft->sent);
            }
            else
            {
                Msg599Rec.DataType = 0x02;
                Msg599Rec.DataLen = rbytes;
                wft->sent += rbytes;
                // We're done sending the file
                if ( !rbytes && 
                     wft->sent >= wft->fsize )
                {
                    wft->status = WSF_WAIT_COMMIT_ACK;
                    CloseHandle(wft->fhnd); wft->fhnd = INVALID_HANDLE_VALUE;
				#ifdef FILE_SERVER_EVENTS
                    //WSHLogEvent(pmod,"WSOCKS: Waiting commit ack for \"%s\" (%s bytes).", 
                    //    wft->remotePath, pmod->SizeStr(wft->fsize));
				#endif
                }
            }
            WSHFileSendMsg(pmod,599, sizeof(MSG599REC), (char *)&Msg599Rec, wft->PortNo);
            if (wft->progWnd)
                PostMessage(wft->progWnd, WM_COMMAND, WS_XFER_STATUS, 0);
        }
    }
    return 0;
}
int WsocksHostImpl::WSHFileTransferClean(WsocksApp *pmod)
{
    if ( !WSFileTransferReady )
        return 0;
    WSFileTransfer *last = 0;
    for ( WSFileTransfer *wft=fileTransfers; wft; )
    {
        // This time diff isn't the number of seconds diff, but can tell
        // us whether the record is at least an hour old
        long tdiff = 0;
        if ( WSHDate > wft->reqDate )
        {
            tdiff = (WSHDate -wft->reqDate) *240000;
            tdiff -= wft->reqTime; tdiff += WSHTime;
        }
        else if ( WSHDate == wft->reqDate )
            tdiff = WSHTime -wft->reqTime;
        // else Time got set back

        // Clean up all completed transfers too
        if ( wft->status == WSF_SUCCESS || wft->status == WSF_FAILED ||
             wft->status == WSF_CANCELLED || wft->status == WSF_TIMEOUT ||
             tdiff > 10000 )
        {
            WSFileTransfer *old = wft; wft=wft->next;
            if ( last ) last->next = old->next;
            else fileTransfers = old->next;
            free(old);
            continue;
        }
        last = wft; wft=wft->next;
    }
    return 0;
}

//HANDLE WsocksHostImpl::WSHCreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
//                    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, 
//                    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, 
//                    FileOptions *opts)
//{
//	if ( !opts ) 
//		opts = &pmod->WSFileOptions;
//    if ( !WSFileTransferReady )
//    {
//        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSCreateFile for \"%s\"!", lpFileName);
//        return INVALID_HANDLE_VALUE;
//    }
//    WSFileTransfer *wft=WSCreateFileAsync(lpFileName, dwDesiredAccess, 
//        dwShareMode, lpSecurityAttributes, dwCreationDisposition, 
//        dwFlagsAndAttributes, hTemplateFile, opts);
//    if ( !wft )
//        return INVALID_HANDLE_VALUE;
//    HANDLE *fhnd = 0;
//    if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&fhnd) != WSF_SUCCESS )
//    {
//        WSHCloseFile(pmod,wft, false);
//        return INVALID_HANDLE_VALUE;
//    }
//    return fhnd;
//}
//BOOL WsocksHostImpl::WSHCloseHandle(HANDLE hObject, BOOL commit)
//{
//    if ( WSHCloseFile(pmod,hObject, commit) )
//        return TRUE;
//    return CloseHandle(hObject);
//}
BOOL WsocksHostImpl::WSHSetFileTime(WsocksApp *pmod, HANDLE hFile, const FILETIME *lpCreationTime, const FILETIME *lpLastAccessTime, const FILETIME *lpLastWriteTime)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSSetFileTime for \"%X\"!", hFile);
        return -1;
    }
	WSFileTransfer *wft;
    for ( wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->fhnd == hFile ||
             wft == hFile )
            break;
    }
    if ( !wft || pmod->FilePort[wft->PortNo].FileTransfer != wft ||
         !(wft->dwDesiredAccess &GENERIC_WRITE) ||
         wft->status == WSF_SUCCESS ||
         wft->status == WSF_FAILED ||
         wft->status == WSF_CANCELLED ||
         wft->status == WSF_TIMEOUT )
        return FALSE;
    memcpy(&wft->fdata.ftCreationTime, lpCreationTime, sizeof(FILETIME));
    memcpy(&wft->fdata.ftLastAccessTime, lpLastAccessTime, sizeof(FILETIME));
    memcpy(&wft->fdata.ftLastWriteTime, lpLastWriteTime, sizeof(FILETIME));
    wft->sftCalled = TRUE;
    return TRUE;
}

//FILE *WsocksHostImpl::WSHfopen(const char *filename, const char *mode,
//              FileOptions *opts)
//{
//	if ( !opts ) 
//		opts = &pmod->WSFileOptions;
//    if ( !WSFileTransferReady )
//    {
//        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSfopen for \"%s\"!", filename);
//        return 0;
//    }
//    WSFileTransfer *wft=WSfopenAsync(filename, mode, opts);
//    if ( !wft )
//        return 0;
//    FILE *file = 0;
//    if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&file) != WSF_SUCCESS )
//    {
//        WSHCloseFile(pmod,wft, false);
//        return 0;
//    }
//    return file;
//}
//int WsocksHostImpl::WSHfclose(FILE *stream, BOOL commit)
//{
//    if ( WSHCloseFile(pmod,(HANDLE)stream, commit) )
//        return TRUE;
//    return fclose(stream);
//}

//HFILE WsocksHostImpl::WSHOpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle,
//                 FileOptions *opts)
//{
//	if ( !opts ) 
//		opts = &pmod->WSFileOptions;
//    if ( !WSFileTransferReady )
//    {
//        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSOpenFile for \"%s\"!", lpFileName);
//        return HFILE_ERROR;
//    }
//    WSFileTransfer *wft=WSOpenFileAsync(lpFileName, lpReOpenBuff, uStyle, opts);
//    if ( !wft )
//        return 0;
//    HFILE hFile = 0;
//    if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&hFile) != WSF_SUCCESS )
//    {
//        WSHCloseFile(pmod,wft, false);
//        return 0;
//    }
//    return hFile;
//}
//void WsocksHostImpl::WSH_lclose(HFILE hFile, BOOL commit)
//{
//    if ( WSHCloseFile(pmod,(HANDLE)hFile, commit) )
//        return;
//    _lclose(hFile);
//}

HANDLE WsocksHostImpl::WSHFindFirstFile(WsocksApp *pmod, LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData, FileOptions *opts)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSFindFirstFile for \"%s\"!", lpFileName);
        return INVALID_HANDLE_VALUE;
    }
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,lpFileName, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
    // One transfer on a port at a time
    if ( pmod->FilePort[PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSFindFirstFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpFileName);
        return INVALID_HANDLE_VALUE;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSFindFirstFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpFileName);
        return INVALID_HANDLE_VALUE;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSFindFirstFile Creating Buffer");
        return INVALID_HANDLE_VALUE;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
	wft->xthnd = 0;
	wft->xtid = 0;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, lpFileName);
    wft->api = FTAPI_FINDFIRSTFILE;
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    wft->next = fileTransfers; fileTransfers = wft;

    // Request find
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x03;
    strncpy(Msg598Rec.FilePath, lpFileName, sizeof(Msg598Rec.FilePath) -1);

    pmod->FilePort[PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, PortNo) )
    {
        wft->status = WSF_FAILED;
        pmod->FilePort[PortNo].FileTransfer = 0;
        WSHLogError(pmod,"WSOCKS: Failed requesting FindFirstFile for \"%s\"!", wft->remotePath);
        return INVALID_HANDLE_VALUE;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: FindFirstFile for \"%s\" requested on Port %d...", wft->remotePath, PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[PortNo].FileTransfer = 0;
            WSHLogError(pmod,"WSOCKS: Timeout requesting FindFirstFile for \"%s\"!", wft->remotePath);
            return INVALID_HANDLE_VALUE;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }
    pmod->FilePort[PortNo].FileTransfer = 0;

    // Return the first result
    if ( wft->status == WSF_RECV_INPROGRESS )
    {
        memcpy(lpFindFileData, &wft->fdata, sizeof(WIN32_FIND_DATA));
        return wft->vhnd;
    }
    pmod->FilePort[PortNo].FileTransfer = 0;
    return INVALID_HANDLE_VALUE;
}
BOOL WsocksHostImpl::WSHFindNextFile(WsocksApp *pmod, HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSFindNextFile for %X!", hFindFile);
        return FALSE;
    }
	WSFileTransfer *wft;
    for ( wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->vhnd == hFindFile )
            break;
    }
    if ( !wft )
        return FALSE;

    // One transfer on a port at a time
    if ( pmod->FilePort[wft->PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSFindNextFile: FilePort %d already in use, cannot transfer \"%s\"!.", wft->PortNo, wft->remotePath);
        return FALSE;
    }
    if ( !pmod->FilePort[wft->PortNo].SockConnected )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSFindNextFile: FilePort %d not connected to transfer \"%s\"!", wft->PortNo, wft->remotePath);
        return FALSE;
    }

    // Request next find
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x04;
    Msg598Rec.vhnd = wft->vhnd;

    pmod->FilePort[wft->PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, wft->PortNo) )
    {
        wft->status = WSF_FAILED;
        pmod->FilePort[wft->PortNo].FileTransfer = 0;
        WSHLogError(pmod,"WSOCKS: Failed requesting FindNextFile for \"%s\"!", wft->remotePath);
        return FALSE;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    //WSHLogEvent(pmod,"WSOCKS: FindNextFile for \"%s\" requested on Port %d...", wft->remotePath, wft->PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[wft->PortNo].FileTransfer = 0;
            WSHLogError(pmod,"WSOCKS: Timeout requesting FindNextFile for \"%s\"!", wft->remotePath);
            return FALSE;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }
    pmod->FilePort[wft->PortNo].FileTransfer = 0;

    // Return the next result
    if ( wft->status == WSF_RECV_INPROGRESS )
    {
        memcpy(lpFindFileData, &wft->fdata, sizeof(WIN32_FIND_DATA));
        return TRUE;
    }
    return FALSE;
}
BOOL WsocksHostImpl::WSHFindClose(WsocksApp *pmod, HANDLE hFindFile)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSFindClose for %X!", hFindFile);
        return FALSE;
    }
	WSFileTransfer *wft;
    for ( wft=fileTransfers; wft; wft=wft->next )
    {
        if ( wft->vhnd == hFindFile )
            break;
    }
    if ( !wft )
        return FALSE;

    // One transfer on a port at a time
    if ( pmod->FilePort[wft->PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSFindClose: FilePort %d already in use, cannot transfer \"%s\"!.", wft->PortNo, wft->remotePath);
        return FALSE;
    }
    if ( !pmod->FilePort[wft->PortNo].SockConnected )
    {
        pmod->FilePort[wft->PortNo].FileTransfer = 0;
        WSHLogError(pmod,"!WSOCKS: ERROR : WSFindClose: FilePort %d not connected to transfer \"%s\"!", wft->PortNo, wft->remotePath);
        return FALSE;
    }

    // Request find close
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x05;
    Msg598Rec.vhnd = wft->vhnd;

    pmod->FilePort[wft->PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, wft->PortNo) )
    {
        wft->status = WSF_FAILED;
        pmod->FilePort[wft->PortNo].FileTransfer = 0;
        WSHLogError(pmod,"WSOCKS: Failed requesting FindClose for \"%s\"!", wft->remotePath);
        return FALSE;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: FindClose for \"%s\" requested on Port %d...", wft->remotePath, wft->PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[wft->PortNo].FileTransfer = 0;
            WSHLogError(pmod,"WSOCKS: Timeout requesting FindClose for \"%s\"!", wft->remotePath);
            return FALSE;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }
    pmod->FilePort[wft->PortNo].FileTransfer = 0;

    return wft->status == WSF_SUCCESS ?TRUE :FALSE;
}

int WsocksHostImpl::WSHPathFileExists(WsocksApp *pmod, LPCTSTR pszPath, FileOptions *opts)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSPathFileExists for \"%s\"!", pszPath);
        return -1;
    }
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,pszPath, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
    // One transfer on a port at a time
    if ( pmod->FilePort[PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSPathFileExists: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, pszPath);
        return -1;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSPathFileExists: FilePort %d not connected to transfer \"%s\"!", PortNo, pszPath);
        return -1;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSPathFileExists Creating Buffer");
        return -1;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, pszPath);
    wft->api = FTAPI_PATHFILEEXISTS;
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    wft->next = fileTransfers; fileTransfers = wft;

    // Request existence check
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x06;
    strncpy(Msg598Rec.FilePath, pszPath, sizeof(Msg598Rec.FilePath) -1);

    pmod->FilePort[PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, PortNo) )
    {
        wft->status = WSF_FAILED;
        pmod->FilePort[PortNo].FileTransfer = 0;
        WSHLogError(pmod,"WSOCKS: Failed requesting PathFileExists for \"%s\"!", wft->remotePath);
        return -1;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: PathFileExists for \"%s\" requested on Port %d...", wft->remotePath, PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[PortNo].FileTransfer = 0;
            WSHLogError(pmod,"WSOCKS: Timeout requesting PathFileExists for \"%s\"!", wft->remotePath);
            return -1;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }
    pmod->FilePort[PortNo].FileTransfer = 0;

    // Return the results
    if ( wft->status == WSF_SUCCESS )
        return wft->exists ?1 :0;     
    return -1;
}

BOOL WsocksHostImpl::WSHDeleteFile(WsocksApp *pmod, LPCTSTR pszPath, BOOL bOverrideAttributes, FileOptions *opts)
{
   if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSPathFileExists for \"%s\"!", pszPath);
        return false;
    }
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,pszPath, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
    // One transfer on a port at a time
    if ( pmod->FilePort[PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSPathFileExists: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, pszPath);
        return false;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSPathFileExists: FilePort %d not connected to transfer \"%s\"!", PortNo, pszPath);
        return false;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSPathFileExists Creating Buffer");
        return false;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, pszPath);
    wft->api = FTAPI_PATHFILEEXISTS;
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    wft->next = fileTransfers; 
	fileTransfers = wft;

    // Request existence check
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x02;
    strncpy(Msg598Rec.FilePath, pszPath, sizeof(Msg598Rec.FilePath) -1);

    pmod->FilePort[PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, PortNo) )
    {
        wft->status = WSF_FAILED;
        WSHLogError(pmod,"WSOCKS: Failed requesting PathFileExists for \"%s\"!", wft->remotePath);
        return false;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: PathFileExists for \"%s\" requested on Port %d...", wft->remotePath, PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            WSHLogError(pmod,"WSOCKS: Timeout requesting PathFileExists for \"%s\"!", wft->remotePath);
            return false;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }

    // Return the results
    if ( wft->status == WSF_SUCCESS )
        return wft->exists ? true : false;     
    return false;
}

int WsocksHostImpl::WSHPathIsDirectory(WsocksApp *pmod, LPCTSTR pszPath, FileOptions *opts)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSPathIsDirectory for \"%s\"!", pszPath);
        return -1;
    }
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,pszPath, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
    // One transfer on a port at a time
    if ( pmod->FilePort[PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSPathIsDirectory: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, pszPath);
        return -1;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSPathIsDirectory: FilePort %d not connected to transfer \"%s\"!", PortNo, pszPath);
        return -1;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSPathIsDirectory Creating Buffer");
        return -1;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, pszPath);
    wft->api = FTAPI_PATHISDIRECTORY;
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    wft->next = fileTransfers; fileTransfers = wft;

    // Request existence check
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x07;
    strncpy(Msg598Rec.FilePath, pszPath, sizeof(Msg598Rec.FilePath) -1);

    pmod->FilePort[PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, PortNo) )
    {
        wft->status = WSF_FAILED;
        pmod->FilePort[PortNo].FileTransfer = 0;
        WSHLogError(pmod,"WSOCKS: Failed requesting PathIsDirectory for \"%s\"!", wft->remotePath);
        return -1;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: PathIsDirectory for \"%s\" requested on Port %d...", wft->remotePath, PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[PortNo].FileTransfer = 0;
            WSHLogError(pmod,"WSOCKS: Timeout requesting PathIsDirectory for \"%s\"!", wft->remotePath);
            return -1;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }
    pmod->FilePort[PortNo].FileTransfer = 0;

    // Return the results
    if ( wft->status == WSF_SUCCESS )
        return wft->exists ?1 :0;     
    return -1;
}

BOOL WsocksHostImpl::WSHCreateDirectory(WsocksApp *pmod, LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes, FileOptions *opts)
{
    if ( !WSFileTransferReady )
    {
        WSHLogError(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSCreateDirectory for \"%s\"!", lpPathName);
        return FALSE;
    }
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,lpPathName, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
    // One transfer on a port at a time
    if ( pmod->FilePort[PortNo].FileTransfer )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSCreateDirectory: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpPathName);
        return FALSE;
    }
    if ( WSHFileTransferConnect(pmod,PortNo) )
    {
        WSHLogError(pmod,"!WSOCKS: ERROR : WSCreateDirectory: FilePort %d not connected to transfer \"%s\"!", PortNo, lpPathName);
        return FALSE;
    }

    // Make a wrapper for our handle
    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
    if ( !wft )
    {
        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSCreateDirectory Creating Buffer");
        return FALSE;
    }
    wft->thnd = INVALID_HANDLE_VALUE;
    wft->fhnd = INVALID_HANDLE_VALUE;
    wft->PortNo = PortNo;
    strcpy(wft->remotePath, lpPathName);
    wft->api = FTAPI_CREATEDIRECTORY;
    wft->reqDate = WSHDate;
    wft->reqTime = WSHTime;
    wft->next = fileTransfers; fileTransfers = wft;

    // Request existence check
    MSG598REC Msg598Rec;
    memset(&Msg598Rec, 0, sizeof(MSG598REC));
    Msg598Rec.MsgVersion = 2;
    Msg598Rec.op = 0x08;
    strncpy(Msg598Rec.FilePath, lpPathName, sizeof(Msg598Rec.FilePath) -1);

    pmod->FilePort[PortNo].FileTransfer = wft;
	if ( !WSHFileSendMsg(pmod,598, sizeof(MSG598REC), (char *)&Msg598Rec, PortNo) )
    {
        wft->status = WSF_FAILED;
        pmod->FilePort[PortNo].FileTransfer = 0;
        WSHLogError(pmod,"WSOCKS: Failed requesting CreateDirectory for \"%s\"!", wft->remotePath);
        return FALSE;
    }
    wft->status = WSF_REQUESTED;
#ifdef FILE_SERVER_EVENTS
    WSHLogEvent(pmod,"WSOCKS: CreateDirectory for \"%s\" requested on Port %d...", wft->remotePath, PortNo);
#endif

    // This should be a fast operation, so don't bother with a progress dialog
    int timeout = 3000;
    DWORD tstart = GetTickCount();
    DWORD lcheck = tstart;
    while ( wft->status == WSF_REQUESTED )
    {
        DWORD tnow = GetTickCount();
        if ( !timeout || (int)(tnow -tstart)>timeout )
        {
			// Send timeout status to fileserver so it knows we're aborting
			WSHCancelFile(pmod,wft);
            wft->status = WSF_TIMEOUT;
            pmod->FilePort[PortNo].FileTransfer = 0;
            WSHLogError(pmod,"WSOCKS: Timeout requesting CreateDirectory for \"%s\"!", wft->remotePath);
            return FALSE;
        }
        SleepEx(WS_TIMER_INTERVAL, true);
        if ( tnow -lcheck > 1000 )
        {
            WSHCheckTime(); lcheck = tnow;
        }
        //WSTimerProc(pmod->WShWnd, WM_TIMER, 0, WSlTime);
		WSHFileTransferLoop(pmod);
    }
    pmod->FilePort[PortNo].FileTransfer = 0;

    // Return the results
    if ( wft->status == WSF_SUCCESS )
    {
        if ( !wft->exists )
            WSHLogError(pmod,"WSOCKS: Failed CreateDirecotry for \"%s\"!", wft->remotePath);
        return wft->exists ?TRUE :FALSE;     
    }
    return FALSE;
}

BOOL WsocksHostImpl::WSHTransmitFile(WsocksApp *pmod, LPCTSTR lpLocalFile, LPCTSTR lpRemoteFile, 
									 const FILETIME *lpCreationTime, 
									 const FILETIME *lpLastAccessTime, 
									 const FILETIME *lpLastWriteTime, FileOptions *opts)
{
    if ( !WSFileTransferReady )
    {
        WSHLogEvent(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSTransmitFile for \"%s\"!", lpLocalFile);
        return FALSE;
    }
#ifdef WS_FILE_SMPROXY
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,lpRemoteFile, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
	if(!strncmp(pmod->FilePort[PortNo].CfgNote,"$SMP$",5))
	{		
		WSHLockPort(pmod,WS_FIL,PortNo,true);
		// One transfer on a port at a time
		if((pmod->FilePort[PortNo].FileTransfer)||(pmod->FilePort[PortNo].DetPtr5))
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			WSHLogEvent(pmod,"!WSOCKS: ERROR : WSHTransmitFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpRemoteFile);
			return 0;
		}
		if ( WSHFileTransferConnect(pmod,PortNo) )
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			WSHLogEvent(pmod,"!WSOCKS: ERROR : WSHTransmitFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpRemoteFile);
			return 0;
		}

		WSFileTransfer2 *wft=new WSFileTransfer2;
		wft->pmod=pmod;
		wft->PortNo=PortNo;
		wft->download=false;
		wft->localFile=lpLocalFile;
		wft->remoteFile=lpRemoteFile;
		wft->status=WSF_INIT;
		wft->xthnd=0; wft->xtid=0;
		wft->doneEvent=CreateEvent(0,false,false,0);
		pmod->FilePort[PortNo].DetPtr5=wft;

		// We can't send this request until applist reply
		if(pmod->FilePort[PortNo].OnLineStatusText[0])
		{
			// Single file download request
			int& rid=(int&)pmod->FilePort[PortNo].DetPtr4;
			char mbuf[1024]={0},*mptr=mbuf;
			char parms[1024];
			sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|SrcPath=%s|FilePath=%s|Overwrite=Y|",
				pmod->pcfg->aname.c_str(),pmod->FilePort[PortNo].OnLineStatusText,lpLocalFile,lpRemoteFile);
			mptr=mbuf;
			ccodec.Encode(mptr,sizeof(mbuf),true,++rid,"uploadfile",parms,0);
			WSHFileSendMsg(pmod,1108,(int)(mptr -mbuf),mbuf,PortNo); 
			wft->status=WSF_REQUESTED;
			// The SyncLoop thread is the only one to flush port sends. 
			// If that thread calls WSTransmitFile, then the port send won't get flushed without us doing it here.
			WSHFileSend(pmod,PortNo,true);
		}
		WSHUnlockPort(pmod,WS_FIL,PortNo,true);
		// Create file transfer thread for WSPreInitSocks case
		if(!pmod->initialized)
			wft->xthnd=CreateThread(0,0,BootPIFileTransferThread2,wft,0,&wft->xtid);
		// Wait for transfer completion. No more progress dialog.
		#ifdef _DEBUG
		int rc=WaitForSingleObject(wft->doneEvent,INFINITE);
		#else
		int rc=WaitForSingleObject(wft->doneEvent,opts->waitTimeout*1000);
		#endif
		if(rc!=WAIT_OBJECT_0)
			wft->status=WSF_TIMEOUT;
		pmod->FilePort[PortNo].DetPtr5=0; 
		// Wait for WSPreInitSocks transfer thread completion
		if(wft->xthnd)
		{
			#ifdef _DEBUG
			WaitForSingleObject(wft->xthnd,INFINITE);
			#else
			WaitForSingleObject(wft->xthnd,3000);
			#endif
			CloseHandle(wft->xthnd); wft->xthnd=0;
		}
		CloseHandle(wft->doneEvent); wft->doneEvent=0;
		rc=(wft->status==WSF_SUCCESS)?true:false;
		delete wft;
		return rc;
	}
	else
#endif
	{
		WSHLockPort(pmod,WS_FIL,PortNo,true);
		WSFileTransfer *wft=WSHCreateFileAsync(pmod,lpRemoteFile, GENERIC_WRITE, 
			FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0, opts);
		if ( !wft )
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			return FALSE;
		}
		wft->api = FTAPI_TRANSMITFILE;
		wft->fhnd = CreateFile(lpLocalFile, GENERIC_READ, FILE_SHARE_READ |FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		if ( wft->fhnd == INVALID_HANDLE_VALUE )
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			WSHCloseFile(pmod,wft, false);
			return FALSE;
		}
		WSHUnlockPort(pmod,WS_FIL,PortNo,true);
		strncpy(wft->localPath, lpLocalFile, sizeof(wft->localPath) -1);
		HANDLE *fhnd = 0;
		if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&fhnd) != WSF_SUCCESS )
		{
			WSHCloseFile(pmod,wft, false);
			return FALSE;
		}
		WSHLockPort(pmod,WS_FIL,PortNo,true);
		WSHSetFileTime(pmod,wft, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
		WSHCloseFile(pmod,wft, true);
		WSHUnlockPort(pmod,WS_FIL,PortNo,true);
	}
    return TRUE;
}
BOOL WsocksHostImpl::WSHDownloadFile(WsocksApp *pmod, LPCTSTR lpRemoteFile, LPCTSTR lpLocalFile, FileOptions *opts)
{
    if ( !WSFileTransferReady )
    {
        WSHLogEvent(pmod,"!WSOCKS: WSPreInitSocks wasn't called before WSHDownloadFile for \"%s\"!", lpLocalFile);
        return FALSE;
    }
#ifdef WS_FILE_SMPROXY
	if ( !opts ) 
		opts = &pmod->WSFileOptions;
	// Determine port by DriveList
	if ( opts->PortNo < 0 )
	{
		opts->PortNo = WSHGetFileTransferPort(pmod,lpRemoteFile, true);
		if ( opts->PortNo < 0 )
			return 0;
	}
	int PortNo = opts->PortNo;
	if(!strncmp(pmod->FilePort[PortNo].CfgNote,"$SMP$",5))
	{		
		WSHLockPort(pmod,WS_FIL,PortNo,true);
		// One transfer on a port at a time
		if((pmod->FilePort[PortNo].FileTransfer)||(pmod->FilePort[PortNo].DetPtr5))
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			WSHLogEvent(pmod,"!WSOCKS: ERROR : WSHDownloadFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpRemoteFile);
			return 0;
		}
		if ( WSHFileTransferConnect(pmod,PortNo) )
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			WSHLogEvent(pmod,"!WSOCKS: ERROR : WSHDownloadFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpRemoteFile);
			return 0;
		}

		WSFileTransfer2 *wft=new WSFileTransfer2;
		wft->pmod=pmod;
		wft->PortNo=PortNo;
		wft->download=true;
		wft->localFile=lpLocalFile;
		wft->remoteFile=lpRemoteFile;
		wft->status=WSF_INIT;
		wft->xthnd=0; wft->xtid=0;
		wft->doneEvent=CreateEvent(0,false,false,0);
		pmod->FilePort[PortNo].DetPtr5=wft;

		// We can't send this request until applist reply
		if(pmod->FilePort[PortNo].OnLineStatusText[0])
		{
			// Single file download request
			int& rid=(int&)pmod->FilePort[PortNo].DetPtr4;
			char mbuf[1024]={0},*mptr=mbuf;
			char parms[1024];
			sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|FileKey=%s|FilePath=%s|",
				pmod->pcfg->aname.c_str(),pmod->FilePort[PortNo].OnLineStatusText,lpLocalFile,lpRemoteFile);
			mptr=mbuf;
			ccodec.Encode(mptr,sizeof(mbuf),true,++rid,"downloadfile",parms,0);
			WSHFileSendMsg(pmod,1108,(int)(mptr -mbuf),mbuf,PortNo); 
			wft->status=WSF_REQUESTED;
			// The SyncLoop thread is the only one to flush port sends. 
			// If that thread calls WSTransmitFile, then the port send won't get flushed without us doing it here.
			WSHFileSend(pmod,PortNo,true);
		}
		WSHUnlockPort(pmod,WS_FIL,PortNo,true);
		// Create file transfer thread for WSPreInitSocks case
		if(!pmod->initialized)
			wft->xthnd=CreateThread(0,0,BootPIFileTransferThread2,wft,0,&wft->xtid);
		// Wait for transfer completion. No more progress dialog.
		#ifdef _DEBUG
		int rc=WaitForSingleObject(wft->doneEvent,INFINITE);
		#else
		int rc=WaitForSingleObject(wft->doneEvent,opts->waitTimeout*1000);
		#endif
		if(rc!=WAIT_OBJECT_0)
			wft->status=WSF_TIMEOUT;
		pmod->FilePort[PortNo].DetPtr5=0; 
		// Wait for WSPreInitSocks transfer thread completion
		if(wft->xthnd)
		{
			#ifdef _DEBUG
			WaitForSingleObject(wft->xthnd,INFINITE);
			#else
			WaitForSingleObject(wft->xthnd,3000);
			#endif
			CloseHandle(wft->xthnd); wft->xthnd=0;
		}
		CloseHandle(wft->doneEvent); wft->doneEvent=0;
		rc=(wft->status==WSF_SUCCESS)?true:false;
		delete wft;
		return rc;
	}
	else
#endif
	{
		WSHLockPort(pmod,WS_FIL,PortNo,true);
		if(opts)
			opts->keepLocal = 1;
		WSFileTransfer *wft=WSHCreateFileAsync(pmod,lpRemoteFile,GENERIC_READ, 
			FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0, opts);
		if ( !wft )
		{
			WSHUnlockPort(pmod,WS_FIL,PortNo,true);
			return FALSE;
		}
		wft->api = FTAPI_GETFILE;
		strncpy(wft->localPath, lpLocalFile, sizeof(wft->localPath) -1);
		WSHUnlockPort(pmod,WS_FIL,PortNo,true);
		HANDLE *fhnd = 0;
		if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&fhnd) != WSF_SUCCESS )
		{
			WSHCloseFile(pmod,wft, false);
			return FALSE;
		}
		WSHLockPort(pmod,WS_FIL,PortNo,true);
		WSHCloseFile(pmod,wft, true);
		WSHUnlockPort(pmod,WS_FIL,PortNo,true);
	}
	return TRUE;
}
// Breadth-first search delete of all files and directories
int WsocksHostImpl::WSHRecurseDelete(WsocksApp *pmod, const char *tdir)
{
	// Don't recurse delete anything outside of the cache dirs!!!
	// An accidental C:\ or blank may cause a catastrophe!!!
	if ( strncmp(tdir+1, ":\\NFS", 5) )
		return -1;

    char match[MAX_PATH];
    memset(match, 0, sizeof(match));
    sprintf(match, "%s\\*.*", tdir);
    WIN32_FIND_DATAA fdata;
    HANDLE fhnd = FindFirstFile(match, &fdata);
    if ( fhnd == INVALID_HANDLE_VALUE )
        return 0;
    do
    {
        if ( !strcmp(fdata.cFileName, ".") || !strcmp(fdata.cFileName, "..") )
            ;
        else if ( fdata.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY )
        {
            char sdir[MAX_PATH];
            memset(sdir, 0, sizeof(sdir));
            sprintf(sdir, "%s\\%s", tdir, fdata.cFileName);
            WSHRecurseDelete(pmod,sdir);
            RemoveDirectory(sdir);
        }
        else
        {
            char fpath[MAX_PATH];
            memset(fpath, 0, sizeof(fpath));
            sprintf(fpath, "%s\\%s", tdir, fdata.cFileName);
            DeleteFile(fpath);
        }
    } while ( FindNextFile(fhnd, &fdata) );
    FindClose(fhnd);
    RemoveDirectory(tdir);
    return 0;
}
//BOOL WsocksHostImpl::WSHCleanCache()
//{
//	char tpath[MAX_PATH];
//	memset(tpath, 0, sizeof(tpath));
//	WSHGetFileTempPath(pmod,"", tpath, false);
//	if ( WSHRecurseDelete(pmod,tpath) )
//		return FALSE;
//	return TRUE;
//}

int WsocksHostImpl::WSHRecommitCheck(WsocksApp *pmod)
{
	// Block nested WM_TIMER calls
	static bool recurseBlock=false;
	if (recurseBlock)
		return -1;
	recurseBlock=true;
	// Once per minute, load newly failed commits
	if ( WSHTime%100==0 )
		WSHLoadUncommittedFiles(pmod,0);
	// Each second try to clear the list
	WSFileRecommit *lfr=0;
	for ( WSFileRecommit *wfr=fileRecommits; wfr; )
	{
		bool deleted=false;
		if ( WSHTime -wfr->lastSend >= 100 )
		{
			FileOptions fopts=pmod->WSFileOptions;
			fopts.PortNo = WSHGetFileTransferPort(pmod,wfr->remotePath, false);
			if ( fopts.PortNo>=0 )
			{
				// The port is connected and there are no file transfers in progress
				if ((pmod->FilePort[fopts.PortNo].SockConnected)&&
					(!pmod->FilePort[fopts.PortNo].FileTransfer))
				{
					WIN32_FIND_DATAA fdata;
					HANDLE fhnd = FindFirstFile(wfr->localPath, &fdata);
					if ( fhnd != INVALID_HANDLE_VALUE )
					{
						FindClose(fhnd);
					#ifdef FILE_SERVER_EVENTS
						WSHLogEvent(pmod,"!WSOCKS: Re-commiting \"%s\"...",wfr->remotePath);
					#endif
						if ( WSHTransmitFile(pmod,wfr->localPath, wfr->remotePath, 
								&fdata.ftCreationTime, &fdata.ftLastAccessTime, &fdata.ftLastWriteTime, &fopts) )
						{
							if ( !DeleteFile(wfr->localPath) )
								WSHLogError(pmod,"!WSOCKS: Failed deleting re-commited file \"%s\"!",wfr->localPath);
							if ( lfr )
								lfr->next=wfr->next;
							else
								fileRecommits=wfr->next;
							WSFileRecommit *old=wfr;
							wfr=wfr->next;
							delete old;
							deleted=true;
						}
						else
						{
							WSHLogError(pmod,"!WSOCKS: Failed re-commiting \"%s\"!",wfr->localPath);
							wfr->lastSend=WSHTime;
						}
					}
					// The file might not be there if we transferred it after it got on the recommit list
					else
					{
						//WSHLogError(pmod,"!WSOCKS: Failed getting times for \"%s\"!",wfr->localPath);
						if ( lfr )
							lfr->next=wfr->next;
						else
							fileRecommits=wfr->next;
						WSFileRecommit *old=wfr;
						wfr=wfr->next;
						delete old;
						deleted=true;
					}
				}			
			}
		}
		if (!deleted)
			wfr=wfr->next;
	}
	recurseBlock=false;
	return 0;
}

// On start, scan the NFSCommit subtree for left-over failed commits
int WsocksHostImpl::WSHLoadUncommittedFiles(WsocksApp *pmod, const char *cdir)
{
	char cpath[MAX_PATH];
	memset(cpath, 0, sizeof(cpath));
	WSHGetFileCommitPath(pmod,"", cpath, false);
	int cplen = (int)strlen(cpath);
	// Start at the app's commit directory
	if ( !cdir )
		cdir = cpath;
	// Add to end of list
	WSFileRecommit *lfr;
	for ( lfr=fileRecommits; lfr && lfr->next; lfr=lfr->next )
		;
	// Add all files to the commit dir.
	char cmatch[MAX_PATH];
	memset(cmatch, 0, sizeof(cmatch));
	sprintf(cmatch, "%s\\*.*", cdir);
	WIN32_FIND_DATAA fdata;
	HANDLE fhnd = FindFirstFile(cmatch, &fdata);
	if ( fhnd == INVALID_HANDLE_VALUE )
		return 0;
	do
	{
		if ( !strcmp(fdata.cFileName, ".") || !strcmp(fdata.cFileName, "..") )
			;
		else if ( fdata.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY )
		{
			char sdir[MAX_PATH];
			memset(sdir, 0, sizeof(sdir));
			sprintf(sdir, "%s\\%s", cdir, fdata.cFileName);
			WSHLoadUncommittedFiles(pmod,sdir);
		}
		else
		{
			char lpath[MAX_PATH];
			memset(lpath, 0, sizeof(lpath));
			sprintf(lpath, "%s\\%s", cdir, fdata.cFileName);
			bool found=false;
			// Don't add currently-committing files
			for ( int i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
			{
				WSFileTransfer *wft=pmod->FilePort[i].FileTransfer;
				if ( wft && !_stricmp(wft->localPath, lpath) )
				{
					found=true;
					break;
				}
			}
			// Don't add to list more than once
			for ( WSFileRecommit *sfr=fileRecommits; sfr && sfr->next; sfr=sfr->next )
			{
				if ( !_stricmp(sfr->localPath, lpath) )
				{
					found=true;
					break;
				}
			}
			if ( !found )
			{
				WSFileRecommit *wfr=(WSFileRecommit *)calloc(1,sizeof(WSFileRecommit));
				if ( wfr )
				{
					strcpy(wfr->localPath, lpath);
					strncpy(wfr->remotePath, wfr->localPath +cplen, sizeof(wfr->remotePath) -1);
					wfr->remotePath[0]=wfr->remotePath[1]; wfr->remotePath[1]=':';
					if ( lfr )
					{
						lfr->next=wfr; lfr=wfr;
					}
					else
						lfr=fileRecommits=wfr;
				}
				else
					WSHLogError(pmod,"!WSOCKS: Failed extending buffer to re-commit \"%s\"!",fdata.cFileName);
			}
		}
	} while ( FindNextFile(fhnd, &fdata) );
	FindClose(fhnd);
	return 0;
}

//// Downloads a file	to the NFS directory tree
//BOOL WsocksHostImpl::WSHGetFile(LPCTSTR lpFileName, LPTSTR localPath, FileOptions *opts)
//{
//	// Must specify keepLocal or cacheLocal
//	if ( !opts || (!opts->keepLocal && !opts->cacheLocal) ) 
//	{
//        WSHLogError(pmod,"!WSOCKS: ERROR : WSGetFile: Requires keep or cache option to transfer \"%s\"", lpFileName);
//		return FALSE;
//	}
//
//	// Determine port by DriveList
//	if ( opts->PortNo < 0 )
//	{
//		opts->PortNo = WSHGetFileTransferPort(pmod,lpFileName, true);
//		if ( opts->PortNo < 0 )
//			return FALSE;
//	}
//	int PortNo = opts->PortNo;
//
//    // Make a wrapper for our handle
//    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
//    if ( !wft )
//    {
//        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSGetFile Creating Buffer");
//        return FALSE;
//    }
//    wft->thnd = INVALID_HANDLE_VALUE;
//    wft->fhnd = INVALID_HANDLE_VALUE;
//    wft->PortNo = PortNo;
//    strcpy(wft->remotePath, lpFileName);
//    wft->api = FTAPI_GETFILE;
//	memcpy(&wft->opts, opts, sizeof(FileOptions));
//    wft->reqDate = WSHDate;
//    wft->reqTime = WSHTime;
//	wft->gdWriteOnly = false;
//    wft->dwDesiredAccess = GENERIC_READ;
//    wft->dwShareMode = FILE_SHARE_READ |FILE_SHARE_WRITE;
//    wft->dwCreationDisposition = OPEN_EXISTING;
//    wft->dwFlagsAndAttributes = 0;
//    WSHGetFileTempPath(pmod,lpFileName, wft->tempPath, true);
//    if ( opts->keepLocal )
//        WSHGetFileKeepPath(pmod,lpFileName, wft->localPath, true);
//	else if ( opts->cacheLocal )
//        WSHGetFileCachePath(pmod,lpFileName, wft->localPath, true);
//    wft->next = fileTransfers; fileTransfers = wft;
//
//	// Create the temp file
//    wft->thnd = CreateFile(wft->tempPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
//    if ( wft->thnd == INVALID_HANDLE_VALUE )
//    {
//        int err = GetLastError();
//		wft->status = WSF_FAILED;
//        WSHLogError(pmod,"!WSOCKS: ERROR : WSGetFile: Failed opening temp file \"%s\" with error %d to transfer \"%s\"", wft->tempPath, err, lpFileName);
//        return FALSE;
//    }
//
//	// Get the timestamps if we already have a persistent copy
//	if ( *wft->localPath )
//	{
//		HANDLE fhnd = FindFirstFile(wft->localPath, &wft->fdata);
//		if ( fhnd != INVALID_HANDLE_VALUE )
//		{
//			FindClose(fhnd);
//			// No need to do anything if we already have it
//			if ( opts->localMaster )
//			{
//				wft->status = WSF_SUCCESS;
//			#ifdef FILE_SERVER_EVENTS
//				WSHLogEvent(pmod,"WSOCKS: WSGetFile: Using local master for \"%s\"", lpFileName);
//			#endif
//				strcpy(localPath, wft->localPath);
//				DeleteFile(wft->tempPath);
//				return TRUE;
//			}
//		}
//	}
//
//	// One transfer on a port at a time
//	if ( pmod->FilePort[PortNo].FileTransfer )
//	{
//		wft->status = WSF_FAILED;
//		WSHLogError(pmod,"!WSOCKS: ERROR : WSGetFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpFileName);
//		return FALSE;
//	}
//	if ( WSHFileTransferConnect(pmod,PortNo) )
//	{
//		wft->status = WSF_FAILED;
//		WSHLogError(pmod,"!WSOCKS: ERROR : WSGetFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpFileName);
//		return FALSE;
//	}
//
//	// Ask the fileserver for it
//	if ( WSHRequestFile(pmod,lpFileName, wft, opts)  )
//		return FALSE;
//
//	// Wait for the download
//    HANDLE fhnd = 0;
//    if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&fhnd) != WSF_SUCCESS )
//    {
//        WSHCloseFile(pmod,wft, false);
//        return FALSE;
//    }
//
//	// Return the persistent location
//	strcpy(localPath, wft->localPath);
//    if ( !WSHCloseFile(pmod,wft, false) )
//		return FALSE;
//    return TRUE;
//}
//// Uploads a file
//BOOL WsocksHostImpl::WSHPutFile(LPCTSTR lpFileName, FileOptions *opts)
//{
//	FileOptions fopts = pmod->WSFileOptions;
//	if ( opts )
//		fopts = *opts;
//	char localPath[MAX_PATH];
//	memset(localPath, 0, sizeof(localPath));
//	bool found = false;
//	// See if there's a keep copy
//	WSHGetFileKeepPath(pmod,lpFileName, localPath, false);
//	if ( PathFileExists(localPath) )
//	{
//		found = true;
//		opts->keepLocal = true;
//	}
//	else 
//	{
//		// If the caller specified keep, then it must be there
//		if ( opts->keepLocal )
//		{
//			WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: Persistent keep copy not found to transfer \"%s\"", lpFileName);
//			return FALSE;
//		}
//		// See if there's a cache copy
//		WSHGetFileCachePath(pmod,lpFileName, localPath, false);
//		if ( PathFileExists(localPath) )
//		{
//			found = true;
//			opts->cacheLocal = true;
//		}
//		else if ( opts->cacheLocal )
//		{
//			WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: Persistent cache copy not found to transfer \"%s\"", lpFileName);
//			return FALSE;
//		}
//	}
//	// There must be a local copy
//	if ( !found )
//	{
//        WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: No persistent copy of \"%s\"", lpFileName);
//		return FALSE;
//	}
//
//	// Determine port by DriveList
//	if ( opts->PortNo < 0 )
//	{
//		opts->PortNo = WSHGetFileTransferPort(pmod,lpFileName, true);
//		if ( opts->PortNo < 0 )
//			return FALSE;
//	}
//	int PortNo = opts->PortNo;
//
//    // Make a wrapper for our handle
//    WSFileTransfer *wft = (WSFileTransfer *)calloc(sizeof(WSFileTransfer), 1);
//    if ( !wft )
//    {
//        WSHLogError(pmod,"!WSOCKS: FATAL ERROR : WSGetFile Creating Buffer");
//        return FALSE;
//    }
//    wft->thnd = INVALID_HANDLE_VALUE;
//    wft->fhnd = INVALID_HANDLE_VALUE;
//    wft->PortNo = PortNo;
//    strcpy(wft->remotePath, lpFileName);
//    wft->api = FTAPI_PUTFILE;
//	memcpy(&wft->opts, opts, sizeof(FileOptions));
//    wft->reqDate = WSHDate;
//    wft->reqTime = WSHTime;
//	wft->gdWriteOnly = false;
//	wft->dwDesiredAccess = GENERIC_WRITE;
//	wft->dwShareMode = FILE_SHARE_READ;
//	wft->dwCreationDisposition = CREATE_ALWAYS;
//	wft->dwFlagsAndAttributes = 0;
//    WSHGetFileTempPath(pmod,lpFileName, wft->tempPath, true);
//    if ( opts->keepLocal )
//        WSHGetFileKeepPath(pmod,lpFileName, wft->localPath, true);
//	else if ( opts->cacheLocal )
//        WSHGetFileCachePath(pmod,lpFileName, wft->localPath, true);
//    wft->next = fileTransfers; fileTransfers = wft;
//
//	/*
//	// Create the temp file
//    wft->thnd = CreateFile(wft->tempPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
//    if ( wft->thnd == INVALID_HANDLE_VALUE )
//    {
//        int err = GetLastError();
//		WSHCloseFile(pmod,wft, false);
//        WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: Failed opening temp file \"%s\" with error %d to transfer \"%s\"", wft->tempPath, err, lpFileName);
//        return FALSE;
//    }
//	*/
//
//	// Open the local file and get timestamps
//    wft->fhnd = CreateFile(wft->localPath, GENERIC_READ, FILE_SHARE_READ |FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
//    if ( wft->fhnd == INVALID_HANDLE_VALUE )
//    {
//		int err = GetLastError();
//		WSHCloseFile(pmod,wft, false);
//        WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: Failed opening persistent file \"%s\" with error %d to transfer \"%s\"", wft->localPath, err, lpFileName);
//        return FALSE;
//    }
//	GetFileTime(wft->fhnd, &wft->fdata.ftCreationTime, &wft->fdata.ftLastAccessTime, &wft->fdata.ftLastWriteTime);
//
//	// One transfer on a port at a time
//	if ( pmod->FilePort[PortNo].FileTransfer )
//	{
//		wft->status = WSF_FAILED;
//		WSHCloseFile(pmod,wft, false);
//		WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: FilePort %d already in use, cannot transfer \"%s\"!.", PortNo, lpFileName);
//		return FALSE;
//	}
//	if ( WSHFileTransferConnect(pmod,PortNo) )
//	{
//		wft->status = WSF_FAILED;
//		WSHCloseFile(pmod,wft, false);
//		WSHLogError(pmod,"!WSOCKS: ERROR : WSPutFile: FilePort %d not connected to transfer \"%s\"!", PortNo, lpFileName);
//		return FALSE;
//	}
//
//	// Ask the fileserver to take it
//	if ( WSHRequestFile(pmod,lpFileName, wft, opts)  )
//	{
//		WSHCloseFile(pmod,wft, false);
//		return FALSE;
//	}
//
//	// Wait for the upload
//    HANDLE fhnd = 0;
//    if ( WSHWaitFile(pmod,wft, opts->waitTimeout, (HANDLE *)&fhnd) != WSF_SUCCESS )
//    {
//		WSHCloseFile(pmod,wft, false);
//        return FALSE;
//    }
//    //WSSetFileTime(wft, &wft->fdata.ftCreationTime, &wft->fdata.ftLastAccessTime, &wft->fdata.ftLastWriteTime);
//    if ( !WSHCloseFile(pmod,wft, true) )
//		return FALSE;
//	return TRUE;
//}

BOOL WsocksHostImpl::WSHReplaceFile(WsocksApp *pmod, const char *spath, const char *dpath)
{
    // Must be full path
    if ( dpath[1] != ':' || spath[1] != ':' )
	{
        if(!MoveFile(spath, dpath))
		{
            DeleteFile(dpath);
            if ( !MoveFile(spath, dpath) )
            {
                WSHLogError(pmod,"Failed moving \"%s\" to \"%s\".", spath, dpath);
                return FALSE;
            }
		}
		return TRUE;
	}
    // Movefile only works when dest is on same volume, 
    // but is much faster than copyfile.
    if ( toupper(dpath[0]) == toupper(spath[0]) )
    {
        if ( !MoveFile(spath, dpath) )
        {
            // May have failed due to dpath permissions
            DeleteFile(dpath);
            if ( !MoveFile(spath, dpath) )
            {
                WSHLogError(pmod,"Failed moving \"%s\" to \"%s\".", spath, dpath);
                return FALSE;
            }
        }
    }
    // Must use CopyFile when on different volumes
    else
    {
        if ( !CopyFile(spath, dpath, false) )
        {
            WSHLogError(pmod,"Failed moving \"%s\" to \"%s\".", spath, dpath);
            return FALSE;
        }
        else
        {
            if ( !DeleteFile(spath) )
                WSHLogEvent(pmod,"Failed deleting \"%s\" after move to \"%s\".", spath, dpath);
        }
    }
    return TRUE;
}

int WsocksHostImpl::WSHFileSend(WsocksApp *pmod, int PortNo, bool flush)
{
	int nsent=0;
	// Socks5 protocol exchange not compressed
	bool socks5=false,compressed=false;
	if((pmod->FilePort[PortNo].S5Connect)&&(pmod->FilePort[PortNo].S5Status>=10)&&(pmod->FilePort[PortNo].S5Status<100))
		socks5=true;
	else if(!pmod->FilePort[PortNo].SockConnected)
		return -1;

	int lastBusy=pmod->FilePort[PortNo].OutBuffer.Busy;
	pmod->FilePort[PortNo].OutBuffer.Busy=pmod->FilePort[PortNo].sendThread;
	pmod->FilePort[PortNo].SendTimeOut=0;
	BUFFER *sendBuffer=&pmod->FilePort[PortNo].OutBuffer;
#ifdef WS_COMPRESS
	if((!socks5)&&(pmod->FilePort[PortNo].Compressed))
	{
		compressed=true;
		sendBuffer=&pmod->FilePort[PortNo].OutCompBuffer;
		// Compress as much as possible to fill one send block
		while((pmod->FilePort[PortNo].OutBuffer.Size>0)&&
			  ((flush)||((int)pmod->FilePort[PortNo].OutBuffer.Size>=pmod->FilePort[PortNo].ImmediateSendLimit)))
		{
			unsigned int CompSize=pmod->FilePort[PortNo].OutBuffer.LocalSize;
			if(CompSize>((WS_COMP_BLOCK_LEN*1024)*99/100))
				CompSize=((WS_COMP_BLOCK_LEN*1024)*99/100);
			if(CompSize>0)
			{
				WSHWaitMutex(0x01,INFINITE);
				char *CompBuff=pmod->SyncLoop_CompBuff;
				unsigned int CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
				mtcompress(CompBuff,&CompBuffSize,pmod->FilePort[PortNo].OutBuffer.Block,CompSize);
				// Compression +encryption
				#ifdef WS_ENCRYPTED
				char *EncryptBuff=CompBuff;
				unsigned int EncryptSize=CompBuffSize;
				if(pmod->FilePort[PortNo].EncryptionOn)
				{
					EncryptBuff=pmod->SyncLoop_EncryptBuff;
					EncryptSize=sizeof(pmod->SyncLoop_EncryptBuff);
					WSHEncrypt(pmod,EncryptBuff,&EncryptSize,CompBuff,CompBuffSize,PortNo,WS_FIL);
				}
				WSWriteBuff(sendBuffer,(char*)&EncryptSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,EncryptBuff,EncryptSize);
				//Optimized for compression only
				#else//!WS_ENCRYPTED
				WSWriteBuff(sendBuffer,(char*)&CompBuffSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,CompBuff,CompBuffSize);
				#endif//WS_ENCRYPTED
				WSHRecord(pmod,&pmod->FilePort[PortNo].Recording,pmod->FilePort[PortNo].OutBuffer.Block,CompSize,true);
				WSStripBuff(&pmod->FilePort[PortNo].OutBuffer,CompSize);
				WSHReleaseMutex(0x01);
			}
		}
	}
#endif//WS_COMPRESS
	// Send as much as possible or up to the choke limit
	while((sendBuffer->Size>0)&&((flush)||(compressed)||((int)sendBuffer->Size>=pmod->FilePort[PortNo].ImmediateSendLimit)))
	{
		int size=sendBuffer->LocalSize;
		if((pmod->FilePort[PortNo].ChokeSize>0)&&(pmod->FilePort[PortNo].ChokeSize -(int)pmod->FilePort[PortNo].LastChokeSize<size))
		{
			size=(pmod->FilePort[PortNo].ChokeSize-pmod->FilePort[PortNo].LastChokeSize);
			if(size<=0)
				break;
		}
		if(size>pmod->FilePort[PortNo].BlockSize*1024)
			size=pmod->FilePort[PortNo].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->FilePort[PortNo].Sock;
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=0;
		if(pmod->initialized>0)
			rc=WSHSendNonBlock(pmod,WS_FIL,PortNo,(SOCKET_T)pport,sendBuffer->Block,size);
		else
			rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#else
		int rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_FILE_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->FilePort[PortNo].SendTimeOut)
				{					
					pmod->FilePort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->FilePort[PortNo].SendTimeOut<WS_CON_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: FILE%d [%s] Reset by Peer",PortNo,pmod->FilePort[PortNo].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: FILE%d [%s] Send Failed: %d",PortNo,pmod->FilePort[PortNo].CfgNote,lerr);
				WSHCloseFilePort(pmod,PortNo);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		if(!compressed)
			WSHRecord(pmod,&pmod->FilePort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->FilePort[PortNo].BytesOut+=rc;
		pmod->FilePort[PortNo].BlocksOut++;
		pmod->FilePort[PortNo].SendTimeOut=0;
		pmod->FilePort[PortNo].LastChokeSize+=rc;
		nsent+=rc;
	}
	pmod->FilePort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}

#else//!WS_FILE_SERVER

int WsocksHostImpl::WSHMakeLocalDirs(WsocksApp *pmod, const char *lpath)
{
    for ( const char *lptr=lpath; lptr; )
    {
		#ifdef WIN32
        const char *nptr = strchr(lptr, '\\');
		#else
        const char *nptr = strchr(lptr, '/');
		#endif
        if ( !nptr )
            break;
        char dpath[MAX_PATH];
        memset(dpath, 0, sizeof(dpath));
        memcpy(dpath, lpath, nptr -lpath);
        CreateDirectory(dpath, 0);
        lptr = nptr +1;
    }
    return 0;
}
#endif//WS_FILE_SERVER

int WsocksHostImpl::WSHBeginUpload(WsocksApp *pmod, const char *spath, const char *DeliverTo, const char *OnBehalfOf,
								   int rid, const char *fpath, WSPortType PortType, int PortNo, 
								   LONGLONG begin)
{
	// Set up a temporary upload file
	char fkey[256]={0};
	const char *fname=0;
	for(fname=fpath +strlen(fpath);
		(fname>=fpath)&&(*fname!='\\')&&(*fname!='/');
		fname--)
		;
	if((*fname=='\\')||(*fname=='/'))
		fname++;
	else
		fname=fpath;
	sprintf(fkey,"%s.upl",fname);
    const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+DeliverTo+"/"+fkey);
	CreateDirectory(WSTEMPFILESDIR,0);
	char tpath[MAX_PATH]={0};
	// Change the temp file location to support simultaneous uploads using the same FileKey 
	// to multiple apps running in the same IQMatrix
#ifdef WIN32
	sprintf(tpath,".\\%s\\%s_%s_%s",WSTEMPFILESDIR,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fkey);
	HANDLE lfp=CreateFile(tpath,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
	if(lfp==INVALID_FILE_VALUE)
		return -1;
	LARGE_INTEGER fsize;
	fsize.QuadPart=0;
	fsize.LowPart=GetFileSize(lfp,(LPDWORD)&fsize.HighPart);
#else
	sprintf(tpath,"./%s/%s_%s_%s",WSTEMPFILESDIR,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fkey);
	FILE *lfp=fopen(tpath,"wb");
	if(lfp==INVALID_FILE_VALUE)
		return -1;
	fseeko(lfp,0,SEEK_END);
	LONGLONG fsize=ftello(lfp);
	fseeko(lfp,0,SEEK_SET);
#endif

	WSTempFile *pfile=new WSTempFile;
	pfile->download=false;
	pfile->rfp=0;
	pfile->fkey=fkey;
	pfile->tpath=tpath;
	pfile->fpath=fpath;
	pfile->lfp=lfp;
	pfile->begin=0;
	pfile->rsize=0;
	pfile->tsize=0;
	pfile->off=0;
    pfile->rid=rid;
    pfile->PortType=PortType;
    pfile->PortNo=PortNo;
    pfile->OnBehalfOf=OnBehalfOf;
    pfile->multi=false;
	fileMap[fmkey]=pfile;

	// Begin upload
	char hbuf[512]={0},*hptr=hbuf;
	memcpy(hptr,&rid,4); hptr+=4;
	WORD dlen=(WORD)strlen(DeliverTo) +1;
	memcpy(hptr,&dlen,2); hptr+=2;
	memcpy(hptr,DeliverTo,dlen); hptr+=dlen;
	WORD blen=(WORD)strlen(OnBehalfOf) +1;
	memcpy(hptr,&blen,2); hptr+=2;
	memcpy(hptr,OnBehalfOf,blen); hptr+=blen;

	char wbuf[8*1024],*wptr=wbuf,*wend=wbuf+sizeof(wbuf);
	memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
	WORD op=0x0A;
	memcpy(wptr,&op,2); wptr+=2;
	// File key
	WORD klen=(WORD)strlen(fkey) +1;
	memcpy(wptr,&klen,2); wptr+=2;
	memcpy(wptr,fkey,klen); wptr+=klen;
	// SrcPath
	WORD slen=(WORD)strlen(spath)+1;
	memcpy(wptr,&slen,2); wptr+=2;
	memcpy(wptr,spath,slen); wptr+=slen;
	// Begin
	memcpy(wptr,&begin,8); wptr+=8;
	switch(PortType)
	{
	case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	default: _ASSERT(false); return -1;
	};
	_ASSERT(wptr<=wend);
	return 0;
}
// Supports files > 2GB and multi-part sends
int WsocksHostImpl::WSHSendFile(WsocksApp *pmod, const char *fkey, const char *DeliverTo, const char *OnBehalfOf,
								int rid, const char *fpath, WSPortType PortType, int PortNo,
								LONGLONG begin, LONGLONG rsize, LONGLONG tsize, DWORD ssize, bool multi)
{
    char errmsg[1024];
    memset(errmsg,0,sizeof(errmsg));
	if(begin<0) begin=0;
#ifdef WIN32
	HANDLE fhnd=CreateFile(fpath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,0,0);
    if(fhnd==INVALID_FILE_VALUE)
    {
        char buf[256];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,GetLastError(),0,buf,sizeof(buf),0);
        size_t blen(strlen(buf));
        while(buf[0]&&!isgraph(buf[blen -1]))
            buf[--blen]=0; // FormatMessage puts \r\n at the end.
        sprintf_s(errmsg,"Opening the file(%s) failed(%s).",fpath,buf);
    }
	LARGE_INTEGER fsize;
    fsize.QuadPart=0;
    if(!errmsg[0]&&!GetFileSizeEx(fhnd,&fsize))
    {
        char buf[256];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,GetLastError(),0,buf,sizeof(buf),0);
        size_t blen(strlen(buf));
        while(buf[0]&&!isgraph(buf[blen -1]))
            buf[--blen]=0;
        sprintf_s(errmsg,"Getting the file size(%s) failed(%s).",fpath,buf);
    }
	if(tsize<1) tsize=fsize.QuadPart;
	if(rsize<1) rsize=tsize -begin;
	LARGE_INTEGER off;
	off.QuadPart=begin;
    if(!errmsg[0]&&!SetFilePointerEx(fhnd,off,0,FILE_BEGIN))
    {
        char buf[256];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,GetLastError(),0,buf,sizeof(buf),0);
        size_t blen(strlen(buf));
        while(buf[0]&&!isgraph(buf[blen -1]))
            buf[--blen]=0;
        sprintf_s(errmsg,"Setting the file pointer(%s) failed(%s).",fpath,buf);
    }
#else
	FILE *fhnd=fopen(fpath,"rb");
	if(fhnd==INVALID_FILE_VALUE)
		return -1;
	fseeko(fhnd,0,SEEK_END);
	LONGLONG fsize=ftello(fhnd);
	if(tsize<1) tsize=fsize;
	if(rsize<1) rsize=tsize -begin;
	LONGLONG off=begin;
	fseeko(fhnd,off,SEEK_SET);
#endif
	// Proxy header
	char hbuf[512]={0},*hptr=hbuf;
	memcpy(hptr,&rid,4); hptr+=4;
	WORD dlen=(WORD)strlen(DeliverTo) +1;
	memcpy(hptr,&dlen,2); hptr+=2;
	memcpy(hptr,DeliverTo,dlen); hptr+=dlen;
	WORD blen=(WORD)strlen(OnBehalfOf) +1;
	memcpy(hptr,&blen,2); hptr+=2;
	memcpy(hptr,OnBehalfOf,blen); hptr+=blen;

	// Send header
	char wbuf[8*1024],*wptr=wbuf,*wend=wbuf+sizeof(wbuf);
	memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
	WORD op=0;
	// Multi-file transfer
	if(multi)
	{
		op=0x0E;
		if(!ssize) 
		{
		#ifdef WIN32
			LONGLONG lsz=fsize.QuadPart -begin;
		#else
			LONGLONG lsz=fsize;
		#endif
			if(lsz>ULONG_MAX)
				return -1;
			ssize=(DWORD)lsz;
		}
		WORD plen=(int)strlen(fpath) +1;
		memcpy(wptr,&op,2); wptr+=2;
		WORD klen=(WORD)strlen(fkey) +1;
		memcpy(wptr,&klen,2); wptr+=2; // File key
		memcpy(wptr,fkey,klen); wptr+=klen;
		memcpy(wptr,&fhnd,4); wptr+=4; // File handle
		memcpy(wptr,&begin,8); wptr+=8; // File offset
		memcpy(wptr,&rsize,8); wptr+=8; // Requested file size
		memcpy(wptr,&tsize,8); wptr+=8; // Total file size
		memcpy(wptr,&ssize,4); wptr+=4; // Partial send size
		memcpy(wptr,&plen,2); wptr+=2; // File path length
		memcpy(wptr,fpath,plen); wptr+=plen; // File path
		switch(PortType)
		{
		case WS_CON: 
			if(!pmod->ConPort[PortNo].SockConnected)
				return -1;
			WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_FIL: 
			if(!pmod->FilePort[PortNo].SockConnected)
				return -1;
			WSHFileSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_USR: 
			if(!pmod->UsrPort[PortNo].SockActive)
				return -1;
			WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_UMR: 
			if(!pmod->UmrPort[PortNo].SockActive)
				return -1;
			WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		default: _ASSERT(false); 
			#ifdef WIN32
			CloseHandle(fhnd); return -1;
			#else
			fclose(fhnd); return -1;
			#endif
		};
		_ASSERT(wptr<=wend);
	}
	// Single file transfer
	else
	{
		op=0x07;
		if(!ssize) 
		{
		#ifdef WIN32
			LONGLONG lsz=fsize.QuadPart -begin;
		#else
			LONGLONG lsz=fsize;
		#endif
			if(lsz>ULONG_MAX)
				return -1;
			ssize=(DWORD)lsz;
		}
		memcpy(wptr,&op,2); wptr+=2;
		WORD klen=(WORD)strlen(fkey) +1;
		memcpy(wptr,&klen,2); wptr+=2; // File key
		memcpy(wptr,fkey,klen); wptr+=klen;
		memcpy(wptr,&fhnd,4); wptr+=4; // File handle
		memcpy(wptr,&begin,8); wptr+=8; // File offset
		memcpy(wptr,&rsize,8); wptr+=8; // Requested file size
		memcpy(wptr,&tsize,8); wptr+=8; // Total file size
		memcpy(wptr,&ssize,4); wptr+=4; // Partial send size
		switch(PortType)
		{
		case WS_CON: 
			if(!pmod->ConPort[PortNo].SockConnected)
				return -1;
			WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_FIL: 
			if(!pmod->FilePort[PortNo].SockConnected)
				return -1;
			WSHFileSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_USR: 
			if(!pmod->UsrPort[PortNo].SockActive)
				return -1;
			WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_UMR: 
			if(!pmod->UmrPort[PortNo].SockActive)
				return -1;
			WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		default: _ASSERT(false); 
			#ifdef WIN32
			CloseHandle(fhnd); return -1;
			#else
			fclose(fhnd); return -1;
			#endif
		};
		_ASSERT(wptr<=wend);
	}
	// Send body
	for(LONGLONG off=begin;off<begin +ssize&&!errmsg[0];)
	{
		wptr=wbuf;
		memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
		op=0x08;
		memcpy(wptr,&op,2); wptr+=2;
		memcpy(wptr,&fhnd,4); wptr+=4; // File handle
		LONGLONG lwz=begin +ssize -off;
		DWORD wsize=(DWORD)lwz;
		if(lwz>wend -wptr)
			wsize=(DWORD)(wend -wptr);
	#ifdef WIN32
		DWORD rbytes=0;
		if(!ReadFile(fhnd,wptr,wsize,&rbytes,0)||(rbytes!=wsize))
        {
            char buf[256];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,GetLastError(),0,buf,sizeof(buf),0);
            size_t blen(strlen(buf));
            while(buf[0]&&!isgraph(buf[blen -1]))
                buf[--blen]=0;
            sprintf_s(errmsg,"Reading file(%s) from %lld for %u bytes failed(%s).",fpath,off,wsize,buf);
			break;
        }
	#else
		DWORD rbytes=(DWORD)fread(wptr,1,wsize,fhnd);
		if(rbytes!=wsize)
			break;
	#endif
		wptr+=wsize; off+=wsize;
		switch(PortType)
		{
		case WS_CON: 
			while(pmod->ConPort[PortNo].OutBuffer.Size>=100*1024*1024)
				SleepEx(100,true);
			WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_FIL: 
			while(pmod->FilePort[PortNo].OutBuffer.Size>=100*1024*1024)
				SleepEx(100,true);
			WSHFileSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_USR: 
			while(pmod->UsrPort[PortNo].OutBuffer.Size>=100*1024*1024)
				SleepEx(100,true);
			WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		case WS_UMR: 
			while(pmod->UmrPort[PortNo].OutBuffer.Size>=100*1024*1024)
				SleepEx(100,true);
			WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
		default: _ASSERT(false); 
			#ifdef WIN32
			CloseHandle(fhnd); return -1;
			#else
			fclose(fhnd); return -1;
			#endif
		};
		_ASSERT(wptr<=wend);
	}
	#ifdef WIN32
    FILETIME ftCreation;
    FILETIME ftLastAccess;
    FILETIME ftLastWrite;
    memset(&ftCreation, 0, sizeof(ftCreation));
    memset(&ftLastAccess, 0, sizeof(ftLastAccess));
    memset(&ftLastWrite, 0, sizeof(ftLastWrite));
    if(!errmsg[0]&&!GetFileTime(fhnd, &ftCreation, &ftLastAccess, &ftLastWrite))
    {
        char buf[256];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,GetLastError(),0,buf,sizeof(buf),0);
        size_t blen(strlen(buf));
        while(buf[0]&&!isgraph(buf[blen -1]))
            buf[--blen]=0;
        sprintf_s(errmsg,"Getting the file time(%s) failed(%s).",fpath,buf);
    }
    if(fhnd!=INVALID_FILE_VALUE)
    {
        CloseHandle(fhnd);
    }
	#else
	fclose(fhnd);
	#endif
    
    // Send File Time
    if(!errmsg[0])
    {
        wptr=wbuf;
	    memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
        op=0x0D;
	    memcpy(wptr,&op,2); wptr+=2;
	    memcpy(wptr,&fhnd,4); wptr+=4; // File handle
        #ifdef WIN32
        memcpy(wptr,&ftCreation.dwLowDateTime,4); wptr+=4;
        memcpy(wptr,&ftCreation.dwHighDateTime,4); wptr+=4;
        memcpy(wptr,&ftLastAccess.dwLowDateTime,4); wptr+=4;
        memcpy(wptr,&ftLastAccess.dwHighDateTime,4); wptr+=4;
        memcpy(wptr,&ftLastWrite.dwLowDateTime,4); wptr+=4;
        memcpy(wptr,&ftLastWrite.dwHighDateTime,4); wptr+=4;
        #endif
	    switch(PortType)
	    {
	    case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    case WS_FIL: WSHFileSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    default: _ASSERT(false); return -1;
	    };
	    _ASSERT(wptr<=wend);
    }
    if(errmsg[0])
    {
        // Download failure
	    wptr=wbuf;
	    memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
        op=0x10;
        memcpy(wptr,&op,2); wptr+=2;
        memcpy(wptr,&fhnd,4); wptr+=4; // File handle
        WORD elen((int)strlen(errmsg) +1);
        memcpy(wptr,&elen,2); wptr+=2;
        memcpy(wptr,errmsg,elen); wptr+=elen;
	    switch(PortType)
	    {
	    case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    case WS_FIL: WSHFileSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	    default: _ASSERT(false); return -1;
	    };
        _ASSERT(wptr<=wend);
        return -1;
    }
	// Send trailer
	wptr=wbuf;
	memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
	op=0x09;
	memcpy(wptr,&op,2); wptr+=2;
	memcpy(wptr,&fhnd,4); wptr+=4; // File handle
	switch(PortType)
	{
	case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	case WS_FIL: WSHFileSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
	default: _ASSERT(false); return -1;
	};
	_ASSERT(wptr<=wend);
	return 0;
}
// Bidirectional file transfer
int WsocksHostImpl::WSHRecvFile(WsocksApp *pmod, const char *Msg, int MsgLen, WSPortType PortType, int PortNo,
								int *rid, char *fkey, int klen, char *lpath, int llen, char *DeliverTo, WORD dlen, char *OnBehalfOf, WORD blen)
{
	CreateDirectory(WSTEMPFILESDIR,0);
	const char *mptr=Msg,*mend=Msg +MsgLen;
	// Proxy header
	if(mend -mptr<4)
		return -1;
	memcpy(rid,mptr,4); mptr+=4;
	if(mend -mptr<2)
		return -1;
	memcpy(&dlen,mptr,2); mptr+=2;
	if(mend -mptr<dlen)
		return -1;
	memcpy(DeliverTo,mptr,dlen); mptr+=dlen;
	if(mend -mptr<2)
		return -1;
	memcpy(&blen,mptr,2); mptr+=2;
	if(mend -mptr<blen)
		return -1;
	memcpy(OnBehalfOf,mptr,blen); mptr+=blen;
	// File portion
	if(mend -mptr<2)
		return -1;
	WORD op=0x00;
	if(mend -mptr<2)
		return -1;
	memcpy(&op,mptr,2); mptr+=2;
	WSTempFile *pfile=0;
	// Operations 0x07, 0x08, and 0x09 are common for download and upload
	// Start file transfer block (large files)
	if(op==0x07)
	{
		// File key
		WORD klen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&klen,mptr,2); mptr+=2;
		char fkey[256]={0};
		if((mend -mptr<klen)||(klen>255))
			return -1;
		memcpy(fkey,mptr,klen); fkey[klen]=0; mptr+=klen;
		if(mend -mptr<4)
			return -1;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+fkey);
		// File handle
		DWORD rfp=0;
		memcpy(&rfp,mptr,4); mptr+=4;
		if(mend -mptr<4)
			return -1;
		// File offset
		LONGLONG begin=0;
		if(mend -mptr<8)
			return -1;
		memcpy(&begin,mptr,8); mptr+=8;
		// Requested file size
		LONGLONG rsize=0;
		if(mend -mptr<8)
			return -1;
		memcpy(&rsize,mptr,8); mptr+=8;
		// Total file size
		LONGLONG tsize=0;
		if(mend -mptr<8)
			return -1;
		memcpy(&tsize,mptr,8); mptr+=8;
		// Partial send size (max 4GB block)
		DWORD ssize=0;
		if(mend -mptr<4)
			return -1;
		memcpy(&ssize,mptr,4); mptr+=4;

		WSTempFile *pfile=0;
		WSFILEMAP::iterator fit=fileMap.find(fmkey);
		if(fit==fileMap.end())
		{
		#ifdef WS_FILE_SMPROXY
			if((PortType!=WS_CON)&&(PortType!=WS_FIL))
		#else
			if(PortType!=WS_CON)
		#endif
			{
			#ifdef WIN32
				WSHLogError(pmod,"UMR%d: Unauthorized upload(%s) for %I64d bytes rejected.",PortNo,fkey,rsize);
			#else
				WSHLogError(pmod,"UMR%d: Unauthorized upload(%s) for %lld bytes rejected.",PortNo,fkey,rsize);
			#endif
				return -1;
			}
			// New download
			char tpath[MAX_PATH]={0};
			// Let the user control the download FileKey
			//sprintf(tpath,".\\%s\\%s_%s_%s",WSTEMPFILESDIR,pmod->pcfg->aclass.c_str(),pmod->pcfg->aname.c_str(),fkey);
            char errmsg[1024]={0};
		#ifdef WIN32
			if((fkey[1]==':')||(fkey[0]=='\\')||(fkey[0]=='/'))
				strcpy(tpath,fkey);
			else
				sprintf(tpath,".\\%s\\%s",WSTEMPFILESDIR,fkey);
			WSHMakeLocalDirs(pmod,tpath);
			HANDLE lfp=CreateFile(tpath,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
			if(lfp==INVALID_FILE_VALUE)
            {
                char buf[256];
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,GetLastError(),0,buf,sizeof(buf),0);
                size_t blen(strlen(buf));
                while(buf[0]&&!isgraph(buf[blen -1]))
                    buf[--blen]=0;
                sprintf_s(errmsg,"Opening the file(%s) failed(%s).",tpath,buf);
            }
		#else
			sprintf(tpath,"./%s/%s",WSTEMPFILESDIR,fkey);
			FILE *lfp=fopen(tpath,"wb");
			if(lfp==INVALID_FILE_VALUE)
				return -1;
		#endif
			pfile=new WSTempFile;
			pfile->proto=op;
			pfile->download=true;
			pfile->fkey=fkey;
			pfile->rfp=rfp;
			pfile->tpath=tpath;
			pfile->lfp=lfp;
			pfile->begin=begin;
			pfile->rsize=rsize;
			pfile->tsize=tsize;
			pfile->off=begin;
            #ifdef WIN32
            memset(&pfile->ftCreation, 0, sizeof(pfile->ftCreation));
            memset(&pfile->ftLastAccess, 0, sizeof(pfile->ftLastAccess));
            memset(&pfile->ftLastWrite, 0, sizeof(pfile->ftLastWrite));
            #endif
            pfile->rid=*rid;
            pfile->PortType=PortType;
            pfile->PortNo=PortNo;
            pfile->OnBehalfOf=OnBehalfOf;
            pfile->multi=false;
            #ifdef WIN32
            LARGE_INTEGER fsize;
            if(lfp!=INVALID_FILE_VALUE&&GetFileSizeEx(lfp,&fsize)&&fsize.QuadPart==begin)
            {
                // we want to append data to the existing file
                SetFilePointerEx(lfp,fsize,0,FILE_BEGIN);
            }
            #endif
            if(errmsg[0])
            {
                WSHLogError(pmod,"Download(%s,%s) failed: %s",fkey,tpath,errmsg);
                pfile->errmsg=errmsg;
                pmod->WSFileTransfered(*pfile);
                delete(pfile);
                return -1;
            }
			fileMap[fmkey]=pfile;
			#ifdef VS2010
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)rfp));
			#else
			char rstr[32]={0};
			sprintf(rstr,"%08X",rfp);
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
			#endif
			fpMap[fpkey]=pfile;
			// Don't leave empty space in local file when begin>0 for remote file
			//SetEndOfFile(lfp);
		}
		// Multi-part transfer
		else
		{
			pfile=fit->second;
			// Download
			if(pfile->download)
			{
				if(pfile->off!=begin)
					#ifdef WIN32
					WSHLogError(pmod,"CON%d: Download(%s) for %d bytes already in progress-restarting at %I64d.",
					#else
					WSHLogError(pmod,"CON%d: Download(%s) for %d bytes already in progress-restarting at %lld.",
					#endif
						PortNo,fkey,rsize,begin);
			}
			// Upload
			else
			{
				pfile->rsize=rsize;
				pfile->tsize=tsize;
			}
			// Seek to start of write
			if(pfile->lfp==INVALID_FILE_VALUE)
				return -1;
            // we don't always want to set file pointer to the download offset because if the download began from the middle of the file,
            // download offset is not the same as the file pointer(the file pointer is offset - original begin).
            if(pfile->off!=begin)
            {
		    #ifdef WIN32
			    LARGE_INTEGER off;
			    off.QuadPart=begin;
			    SetFilePointer(pfile->lfp,off.LowPart,&off.HighPart,FILE_BEGIN);
		    #else
			    LONGLONG off=begin;
			    fseeko(pfile->lfp,off,SEEK_SET);
		    #endif
                pfile->off=begin;
            }
			// Always keep the remote file handle up-to-date
			if(pfile->rfp!=rfp)
			{
				pfile->rfp=rfp;
				#ifdef VS2010
                const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)pfile->rfp));
				#else
				char rstr[32]={0};
				sprintf(rstr,"%08X",(PTRCAST)pfile->rfp);
                const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
				#endif
				fpMap[fpkey]=pfile;
			}
		}
		// Return parameters
		strncpy(fkey,pfile->fkey.c_str(),klen);
		strncpy(lpath,pfile->fpath.c_str(),llen);
        pmod->WSFileTransfered(*pfile);
		return 1;
	}
	// Body (large files)
	else if(op==0x08)
	{
		// File handle
		DWORD rfp=0;
		if(mend -mptr<4)
			return -1;
		memcpy(&rfp,mptr,4); mptr+=4;
		#ifdef VS2010
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)rfp));
		#else
		char rstr[32]={0};
		sprintf(rstr,"%08X",rfp);
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
		#endif
		WSFPMAP::iterator fit=fpMap.find(fpkey);
		if(fit==fpMap.end())
			return -1;
		pfile=fit->second;
		if(pfile->lfp==INVALID_FILE_VALUE)
			return -1;
		// File data
		DWORD wsize=(DWORD)(mend -mptr);
	#ifdef WIN32
		DWORD wbytes=0;
		if(!WriteFile(pfile->lfp,mptr,wsize,&wbytes,0)||(wbytes!=wsize))
			return -1;
	#else
		DWORD wbytes=(DWORD)fwrite(mptr,1,wsize,pfile->lfp);
		if(wbytes!=wsize)
			return -1;
	#endif
		pfile->off+=wbytes;
		// Return parameters
		strncpy(fkey,pfile->fkey.c_str(),klen);
		strncpy(lpath,pfile->fpath.c_str(),llen);
        pmod->WSFileTransfered(*pfile);
		return 1;
	}
    // File Time
    else if(op==0x0D)
    {
		// File handle
		DWORD rfp=0;
		if(mend -mptr<4)
			return -1;
		memcpy(&rfp,mptr,4); mptr+=4;
		#ifdef VS2010
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)rfp));
		#else
		char rstr[32]={0};
		sprintf(rstr,"%08X",rfp);
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
		#endif
		WSFPMAP::iterator fit=fpMap.find(fpkey);
		if(fit==fpMap.end())
			return -1;
		pfile=fit->second;
        if(mend -mptr<(4 * 2) * 3)
            return -1;
        #ifdef WIN32
        memcpy(&pfile->ftCreation.dwLowDateTime, mptr, 4); mptr += 4;
        memcpy(&pfile->ftCreation.dwHighDateTime, mptr, 4); mptr += 4;
        memcpy(&pfile->ftLastAccess.dwLowDateTime, mptr, 4); mptr += 4;
        memcpy(&pfile->ftLastAccess.dwHighDateTime, mptr, 4); mptr += 4;
        memcpy(&pfile->ftLastWrite.dwLowDateTime, mptr, 4); mptr += 4;
        memcpy(&pfile->ftLastWrite.dwHighDateTime, mptr, 4); mptr += 4;
        #endif
        return 1;
    }
	// Trailer (large files)
	else if(op==0x09)
	{
		// File handle
		DWORD rfp=0;
		if(mend -mptr<4)
			return -1;
		memcpy(&rfp,mptr,4);
		#ifdef VS2010
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)rfp));
		#else
		char rstr[32]={0};
		sprintf(rstr,"%08X",rfp);
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
		#endif
		WSFPMAP::iterator fit=fpMap.find(fpkey);
		if(fit==fpMap.end())
			return -1;
		pfile=fit->second;
		fpMap.erase(fit); pfile->rfp=0;
		if(pfile->lfp==INVALID_FILE_VALUE)
			return -1;
		// Return parameters
		strncpy(fkey,pfile->fkey.c_str(),klen);
		strncpy(lpath,pfile->tpath.c_str(),llen);
	#ifdef WS_FILE_SMPROXY
		const char *xtype=((PortType==WS_CON)||(PortType==WS_FIL))?"Download":"Upload";
	#else
		const char *xtype=(PortType==WS_CON)?"Download":"Upload";
	#endif
		// Multi-part transfer incomplete
		if(pfile->off<pfile->begin +pfile->rsize)
		{
		    // Upload block ack
		    if(!pfile->download)
		    {
			    char hbuf[512]={0},*hptr=hbuf;
			    memcpy(hptr,rid,4); hptr+=4;
			    WORD dlen=(WORD)strlen(OnBehalfOf) +1;
			    memcpy(hptr,&dlen,2); hptr+=2;
			    memcpy(hptr,OnBehalfOf,dlen); hptr+=dlen;
			    WORD blen=(WORD)strlen(DeliverTo) +1;
			    memcpy(hptr,&blen,2); hptr+=2;
			    memcpy(hptr,DeliverTo,blen); hptr+=blen;

			    char wbuf[8*1024],*wptr=wbuf,*wend=wbuf+sizeof(wbuf);
			    memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
			    WORD op=0x0B;
			    memcpy(wptr,&op,2); wptr+=2;
			    WORD klen=(WORD)strlen(fkey) +1;
			    memcpy(wptr,&klen,2); wptr+=2; // File key
			    memcpy(wptr,fkey,klen); wptr+=klen;			
			    WORD flen=(WORD)pfile->fpath.length()+1; // File path
			    memcpy(wptr,&flen,2); wptr+=2;			
			    memcpy(wptr,pfile->fpath.c_str(),flen); wptr+=flen;
                memcpy(wptr,&pfile->off,8); wptr+=8; // Last block offset
			    switch(PortType)
			    {
			    case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			    case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			    case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			    };
			    _ASSERT(wptr<=wend);
		    }
            pmod->WSFileTransfered(*pfile);
            return 1;
		}
	#ifdef WIN32
		SetEndOfFile(pfile->lfp);
        if((pfile->ftCreation.dwLowDateTime || pfile->ftCreation.dwHighDateTime) &&
            (pfile->ftLastAccess.dwLowDateTime || pfile->ftLastAccess.dwHighDateTime) &&
            (pfile->ftLastWrite.dwLowDateTime || pfile->ftLastWrite.dwLowDateTime))
        {
            SetFileTime(pfile->lfp, &pfile->ftCreation, &pfile->ftLastAccess, &pfile->ftLastWrite);
        }
		CloseHandle(pfile->lfp);
	#else
		//ftruncate(pfile->lfp->_file);
		fclose(pfile->lfp);
	#endif
		pfile->lfp=INVALID_FILE_VALUE;

        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+pfile->fkey);
		WSFILEMAP::iterator kit=fileMap.find(fmkey);
		if(kit!=fileMap.end())
			fileMap.erase(kit);

        char errmsg[1024]={0};
		// Got too much
        if(pfile->off>pfile->begin +pfile->rsize)
        {
			#ifdef WIN32
            sprintf_s(errmsg,"%s(%s,%s) extended at %I64d/%I64d bytes.",
			#else
            sprintf(errmsg,"%s(%s,%s) extended at %lld/%lld bytes.",
			#endif
				xtype,pfile->fkey.c_str(),pfile->tpath.c_str(),pfile->off,pfile->begin +pfile->rsize);
            WSHLogEvent(pmod,"%s",errmsg);
        }
		// Just right.
		else
        {
			#ifdef WIN32
			WSHLogEvent(pmod,"%s(%s,%s) completed at %I64d bytes.",
			#else
			WSHLogEvent(pmod,"%s(%s,%s) completed at %lld bytes.",
			#endif
				xtype,pfile->fkey.c_str(),pfile->tpath.c_str(),pfile->begin +pfile->rsize);
        }
		// Move to final destination
		if(!pfile->fpath.empty()&&!errmsg[0])
		{
		#if defined(WIN32)||defined(_CONSOLE)
			if(CopyFile(pfile->tpath.c_str(),pfile->fpath.c_str(),false))
			{
				WSHLogEvent(pmod,"Uploaded file moved to (%s).",pfile->fpath.c_str());
				DeleteFile(pfile->tpath.c_str());
			}
		#else
			char cmd[1024]={0};
			sprintf(cmd,"cp --reply=yes -f %s %s",pfile->tpath.c_str(),pfile->fpath.c_str());
			system(cmd);
			if(PathFileExists(pfile->fpath.c_str()))
			{
				WSHLogEvent(pmod,"Uploaded file moved to (%s).",pfile->fpath.c_str());
				unlink(pfile->tpath.c_str());
			}
		#endif
			else
			{
				int ecode=GetLastError();
                char reason[256]={0};
            #if defined(WIN32)||defined(_CONSOLE)
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,(DWORD)ecode,0,reason,sizeof(reason),0);
                size_t rlen=strlen(reason);
                while(reason[0]&&strchr("\r\n",reason[rlen-1]))
                    reason[--rlen]=0; // FormatMessage appends new line chars, so remove them
            #else
                sprintf(reason,"err=%d.",ecode);
            #endif
				sprintf(errmsg,"%s(%s,%s) failed copy to (%s): %s",
					xtype,pfile->fkey.c_str(),pfile->tpath.c_str(),pfile->fpath.c_str(),reason);
				WSHLogEvent(pmod,"%s",errmsg);
				pfile->tpath="";
			}
		}
		// Upload block ack
		if(!pfile->download)
		{
			char hbuf[512]={0},*hptr=hbuf;
			memcpy(hptr,rid,4); hptr+=4;
			WORD dlen=(WORD)strlen(OnBehalfOf) +1;
			memcpy(hptr,&dlen,2); hptr+=2;
			memcpy(hptr,OnBehalfOf,dlen); hptr+=dlen;
			WORD blen=(WORD)strlen(DeliverTo) +1;
			memcpy(hptr,&blen,2); hptr+=2;
			memcpy(hptr,DeliverTo,blen); hptr+=blen;

			char wbuf[8*1024],*wptr=wbuf,*wend=wbuf+sizeof(wbuf);
			memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
			WORD op=errmsg[0]?0x0F:0x0B;
			memcpy(wptr,&op,2); wptr+=2;
			WORD klen=(WORD)strlen(fkey) +1;
			memcpy(wptr,&klen,2); wptr+=2; // File key
			memcpy(wptr,fkey,klen); wptr+=klen;			
			WORD flen=(WORD)pfile->fpath.length()+1; // File path
			memcpy(wptr,&flen,2); wptr+=2;			
			memcpy(wptr,pfile->fpath.c_str(),flen); wptr+=flen;
            if(op==0x0F)
            {
                WORD elen=(WORD)strlen(errmsg) +1;
                memcpy(wptr,&elen,2); wptr+=2;
                memcpy(wptr,errmsg,elen); wptr+=elen; // Error message
            }
            else
            {
                memcpy(wptr,&pfile->off,8); wptr+=8; // Last block offset
            }
			switch(PortType)
			{
			case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			};
			_ASSERT(wptr<=wend);
		}
        pfile->errmsg = errmsg;
        pmod->WSFileTransfered(*pfile);
		delete pfile;
		return errmsg[0]?-1:0;
	}
    // Download failure acknowledge
    else if(op==0x10)
    {
		// File handle
		DWORD rfp=0;
		if(mend -mptr<4)
			return -1;
		memcpy(&rfp,mptr,4); mptr+=4;
		#ifdef VS2010
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)rfp));
		#else
		char rstr[32]={0};
		sprintf(rstr,"%08X",rfp);
        const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
		#endif
		WSFPMAP::iterator fit=fpMap.find(fpkey);
		if(fit==fpMap.end())
			return -1;
		pfile=fit->second;
		fpMap.erase(fit); pfile->rfp=0;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+pfile->fkey);
		WSFILEMAP::iterator kit=fileMap.find(fmkey);
		if(kit!=fileMap.end())
			fileMap.erase(kit);
		if(pfile->lfp==INVALID_FILE_VALUE)
			return -1;
#ifdef WIN32
        CloseHandle(pfile->lfp);
#else
		fclose(pfile->lfp);
#endif
        pfile->lfp=INVALID_FILE_VALUE;

        if(mend -mptr<2)
            return -1;
        WORD elen;
        memcpy(&elen,mptr,2); mptr+=2;
        if(mend -mptr<elen)
            return -1;
        char errmsg[1024];
        memcpy(errmsg,mptr,elen); mptr+=elen;
        WSHLogError(pmod,"Download(%s,%s) failed: %s",pfile->fkey.c_str(),pfile->tpath.c_str(),errmsg);
        pfile->errmsg=errmsg;
        pmod->WSFileTransfered(*pfile);
        delete(pfile);
        return -1;
    }
	// Begin upload (large files)
	else if(op==0x0A)
	{
		// File key
		WORD klen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&klen,mptr,2); mptr+=2;
		char fkey[256]={0};
		if((mend -mptr<klen)||(klen>255))
			return -1;
		memcpy(fkey,mptr,klen); fkey[klen]=0; mptr+=klen;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+fkey);
		// SrcPath
		if(mend -mptr<2)
			return -1;
		int slen=0;
		memcpy(&slen,mptr,2); mptr+=2;
		char spath[MAX_PATH]={0};
		if(slen>MAX_PATH)
		{
			memcpy(spath,mptr,MAX_PATH); spath[MAX_PATH-1]=0;
		}
		else
			memcpy(spath,mptr,slen);
		mptr+=slen;
		// Begin
		if(mend -mptr<8)
			return -1;
		LONGLONG begin=0;
		memcpy(&begin,mptr,8); mptr+=8;

		WSTempFile *pfile=0;
		WSFILEMAP::iterator fit=fileMap.find(fmkey);
		// New upload
		if(fit==fileMap.end())
		{
		#ifdef WIN32
			LARGE_INTEGER fsize;
			fsize.QuadPart=0;
			WIN32_FIND_DATA fdata;
			HANDLE fhnd=FindFirstFile(spath,&fdata);
			if(fhnd!=INVALID_HANDLE_VALUE)
			{
				fsize.HighPart=fdata.nFileSizeHigh;
				fsize.LowPart=fdata.nFileSizeLow;
				FindClose(fhnd);
			}
		#else
			struct stat fst;
			stat(spath,&fst);
			LONGLONG fsize=fst.st_size;
		#endif

			pfile=new WSTempFile;
			pfile->proto=op;
			pfile->download=false;
			pfile->fkey=fkey;
			pfile->rfp=0;
			pfile->tpath=spath;
			pfile->lfp=INVALID_FILE_VALUE;
			pfile->begin=0;
			#ifdef WIN32
			pfile->rsize=fsize.QuadPart;
			pfile->tsize=fsize.QuadPart;
			#else
			pfile->rsize=fsize;
			pfile->tsize=fsize;
			#endif
			pfile->off=0;
            pfile->rid=*rid;
            pfile->PortType=PortType;
            pfile->PortNo=PortNo;
            pfile->OnBehalfOf=OnBehalfOf;
            pfile->multi=false;
			fileMap[fmkey]=pfile;
		}
		// Multi-part upload
		else
		{
			pfile=fit->second;
			pfile->off=begin;
		}

		// If the file is larger than 2MB, then send nicely (max 2MB/sec)
		LONGLONG sendBytes=pfile->tsize -begin;
		if(sendBytes>=2048*1024)
			sendBytes=2048*1024;
		if(WSHSendFile(pmod,fkey,OnBehalfOf,DeliverTo,*rid,spath,PortType,PortNo,begin,pfile->tsize,pfile->tsize,(DWORD)sendBytes,false)<0)
		{
			// Cancel upload 0x0C
			char hbuf[512]={0},*hptr=hbuf;
			memcpy(hptr,rid,4); hptr+=4;
			WORD dlen=(WORD)strlen(DeliverTo) +1;
			memcpy(hptr,&dlen,2); hptr+=2;
			memcpy(hptr,DeliverTo,dlen); hptr+=dlen;
			WORD blen=(WORD)strlen(OnBehalfOf) +1;
			memcpy(hptr,&blen,2); hptr+=2;
			memcpy(hptr,OnBehalfOf,blen); hptr+=blen;

			char wbuf[8*1024],*wptr=wbuf,*wend=wbuf+sizeof(wbuf);
			memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
			WORD op=0x0C;
			memcpy(wptr,&op,2); wptr+=2;
			WORD klen=(WORD)strlen(fkey) +1;
			memcpy(wptr,&klen,2); wptr+=2; // File key
			memcpy(wptr,fkey,klen); wptr+=klen;
			char reason[256]={0};
			strcpy(reason,"Upload failed");
			WORD rlen=(WORD)strlen(reason) +1;
			memcpy(wptr,&rlen,2); wptr+=2; // Reason
			memcpy(wptr,reason,rlen); wptr+=rlen;
			switch(PortType)
			{
			case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
			};
			_ASSERT(wptr<=wend);
			return -1;
		}
		return 1;
	}
	// Upload acknowledge (large files)
	else if(op==0x0B)
	{
		// File key
		WORD klen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&klen,mptr,2); mptr+=2;
		char fkey[256]={0};
		if((mend -mptr<klen)||(klen>255))
			return -1;
		memcpy(fkey,mptr,klen); fkey[klen]=0; mptr+=klen;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+fkey);
		// FilePath
		WORD flen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&flen,mptr,2); mptr+=2;
		char fpath[MAX_PATH]={0};
		if(flen>MAX_PATH)
		{
			memcpy(fpath,mptr,MAX_PATH); fpath[MAX_PATH-1]=0;
		}
		else
			memcpy(fpath,mptr,flen);
		mptr+=flen;
		// Last block offset
		if(mend -mptr<8)
			return -1;
		LONGLONG begin=0;
		memcpy(&begin,mptr,8); mptr+=8;

		WSFILEMAP::iterator fit=fileMap.find(fmkey);
		if(fit!=fileMap.end())
		{
			WSTempFile *pfile=fit->second;
			pfile->off=begin;
            pmod->WSFileTransfered(*pfile);
			// Multi-part upload incomplete
			if(pfile->off<pfile->begin +pfile->rsize)
			{
				// If the file is larger than 2MB, then send nicely (max 2MB/sec)
				LONGLONG sendBytes=pfile->begin +pfile->rsize -pfile->off;
				if(sendBytes>=2048*1024)
					sendBytes=2048*1024;
				if(WSHSendFile(pmod,fkey,OnBehalfOf,DeliverTo,*rid,pfile->tpath.c_str(),PortType,PortNo,pfile->off,pfile->rsize,pfile->tsize,(DWORD)sendBytes,false)<0)
				{
					// Cancel upload 0x0C
					char hbuf[512]={0},*hptr=hbuf;
					memcpy(hptr,rid,4); hptr+=4;
					WORD dlen=(WORD)strlen(DeliverTo) +1;
					memcpy(hptr,&dlen,2); hptr+=2;
					memcpy(hptr,DeliverTo,dlen); hptr+=dlen;
					WORD blen=(WORD)strlen(OnBehalfOf) +1;
					memcpy(hptr,&blen,2); hptr+=2;
					memcpy(hptr,OnBehalfOf,blen); hptr+=blen;

					char wbuf[8*1024],*wptr=wbuf,*wend=wbuf+sizeof(wbuf);
					memcpy(wptr,hbuf,hptr -hbuf); wptr+=(hptr -hbuf); // Proxy header
					WORD op=0x0C;
					memcpy(wptr,&op,2); wptr+=2;
					WORD klen=(WORD)strlen(fkey) +1;
					memcpy(wptr,&klen,2); wptr+=2; // File key
					memcpy(wptr,fkey,klen); wptr+=klen;
					char reason[256]={0};
					strcpy(reason,"Upload failed");
					WORD rlen=(WORD)strlen(reason) +1;
					memcpy(wptr,&rlen,2); wptr+=2; // Reason
					memcpy(wptr,reason,rlen); wptr+=rlen;
					switch(PortType)
					{
					case WS_CON: WSHConSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
					case WS_USR: WSHUsrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
					case WS_UMR: WSHUmrSendMsg(pmod,1111,(int)(wptr -wbuf),wbuf,PortNo); break;
					};
					_ASSERT(wptr<=wend);
					return -1;
				}
				return 1;
			}
			WSHLogEvent(pmod,"Upload(%s,%s) completed for %lld bytes.",fkey,fpath,begin);
			fileMap.erase(fit);
			if(pfile->lfp!=INVALID_FILE_VALUE)
			{
				#ifdef VS2010
                const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)pfile->lfp));
				#else
				char rstr[32]={0};
				sprintf(rstr,"%08X",(PTRCAST)pfile->lfp);
                const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
				#endif
				WSFPMAP::iterator fit=fpMap.find(fpkey);
				if(fit!=fpMap.end())
					fpMap.erase(fit);
				#ifdef WIN32
				CloseHandle(pfile->lfp);
				#else
				fclose(pfile->lfp);
				#endif
				pfile->lfp=INVALID_FILE_VALUE;
			}
			delete pfile;
            return 0;
		}
		return 1;
	}
    // Upload failure acknowledge
    else if(op==0x0F)
    {
        // File Key
        WORD klen=0;
        if(mend -mptr<2)
            return -1;
        memcpy(&klen,mptr,2); mptr+=2;
        char fkey[MAX_PATH]={0};
        if((mend -mptr<klen)||(klen>sizeof(fkey)))
            return -1;
        memcpy(fkey,mptr,klen); fkey[klen-1]=0; mptr+=klen;
        // File Path
        WORD flen=0;
        if(mend -mptr<2)
            return -1;
        memcpy(&flen,mptr,2); mptr+=2;
        char fpath[MAX_PATH]={0};
        if((mend -mptr<flen)||(flen>sizeof(fpath)))
            return -1;
        memcpy(fpath,mptr,flen); fpath[flen-1]=0; mptr+=flen;
        // Error Message
        WORD elen=0;
        if(mend -mptr<2)
            return -1;
        memcpy(&elen,mptr,2); mptr+=2;
        char errmsg[1024]={0};
        if((mend -mptr<elen)||(elen>sizeof(errmsg)))
            return -1;
        memcpy(errmsg,mptr,elen); errmsg[elen-1]=0; mptr+=elen;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+fkey);
        WSFILEMAP::iterator fit=fileMap.find(fmkey);
        if(fit==fileMap.end())
            return 1;
        WSTempFile *pfile=fit->second;
        fileMap.erase(fit);
		if(pfile->lfp!=INVALID_FILE_VALUE)
		{
			#ifdef VS2010
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)pfile->lfp));
			#else
			char rstr[32]={0};
			sprintf(rstr,"%08X",(PTRCAST)pfile->lfp);
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
			#endif
			WSFPMAP::iterator fit=fpMap.find(fpkey);
			if(fit!=fpMap.end())
				fpMap.erase(fit);
			#ifdef WIN32
			CloseHandle(pfile->lfp);
			#else
			fclose(pfile->lfp);
			#endif
			pfile->lfp=INVALID_FILE_VALUE;
		}
        WSHLogEvent(pmod,"Upload(%s,%s) failed: %s",fkey,fpath,errmsg);
        pfile->errmsg=errmsg;
        pmod->WSFileTransfered(*pfile);
        delete pfile;
        return 0;
    }
	// Cancel upload
	else if(op==0x0C)
	{
		// File key
		WORD klen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&klen,mptr,2); mptr+=2;
		char fkey[256]={0};
		if((mend -mptr<klen)||(klen>255))
			return -1;
		memcpy(fkey,mptr,klen); fkey[klen]=0; mptr+=klen;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+fkey);
		// Reason
		char reason[256]={0};
		WORD rlen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&rlen,mptr,2); mptr+=2;
		memcpy(reason,mptr,rlen); mptr+=rlen;

		WSFILEMAP::iterator kit=fileMap.find(fmkey);
		if(kit==fileMap.end())
			return -1;
		WSTempFile *pfile=kit->second;
		WSHLogEvent(pmod,"Upload(%s,%s) cancelled by peer with reason(%s).",fkey,pfile->fpath.c_str(),reason);
		fileMap.erase(kit);
		if(pfile->lfp!=INVALID_FILE_VALUE)
		{
			#ifdef VS2010
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)pfile->lfp));
			#else
			char rstr[32]={0};
			sprintf(rstr,"%08X",(PTRCAST)pfile->lfp);
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
			#endif
			WSFPMAP::iterator fit=fpMap.find(fpkey);
			if(fit!=fpMap.end())
				fpMap.erase(fit);
			#ifdef WIN32
			CloseHandle(pfile->lfp);
			DeleteFile(pfile->tpath.c_str());
			#else
			fclose(pfile->lfp);
			unlink(pfile->tpath.c_str());
			#endif
			pfile->lfp=INVALID_FILE_VALUE;
		}
		delete pfile;
		return 1;
	}
	// Operations 0x0E is common for download and upload
	// Start multi file transfer block (large files)
	if(op==0x0E)
	{
		// File key
		WORD klen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&klen,mptr,2); mptr+=2;
		char fkey[256]={0};
		if((mend -mptr<klen)||(klen>255))
			return -1;
		memcpy(fkey,mptr,klen); fkey[klen]=0; mptr+=klen;
		if(mend -mptr<4)
			return -1;
        const string fmkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+fkey);
		// File handle
		DWORD rfp=0;
		memcpy(&rfp,mptr,4); mptr+=4;
		if(mend -mptr<4)
			return -1;
		// File offset
		LONGLONG begin=0;
		if(mend -mptr<8)
			return -1;
		memcpy(&begin,mptr,8); mptr+=8;
		// Requested file size
		LONGLONG rsize=0;
		if(mend -mptr<8)
			return -1;
		memcpy(&rsize,mptr,8); mptr+=8;
		// Total file size
		LONGLONG tsize=0;
		if(mend -mptr<8)
			return -1;
		memcpy(&tsize,mptr,8); mptr+=8;
		// Partial send size (max 4GB block)
		DWORD ssize=0;
		if(mend -mptr<4)
			return -1;
		memcpy(&ssize,mptr,4); mptr+=4;
		// File path length
		WORD plen=0;
		if(mend -mptr<2)
			return -1;
		memcpy(&plen,mptr,2); mptr+=2;
		char rpath[MAX_PATH]={0};
		if(mend -mptr<plen)
			return -1;
		memcpy(rpath,mptr,plen); mptr+=plen;

		WSTempFile *pfile=0;
		WSFILEMAP::iterator fit=fileMap.find(fmkey);
		if(fit==fileMap.end())
		{
			if(PortType!=WS_CON)
			{
			#ifdef WIN32
				WSHLogError(pmod,"UMR%d: Unauthorized upload(%s) for %I64d bytes rejected.",PortNo,fkey,rsize);
			#else
				WSHLogError(pmod,"UMR%d: Unauthorized upload(%s) for %lld bytes rejected.",PortNo,fkey,rsize);
			#endif
				return -1;
			}
			// New download
			char tpath[MAX_PATH]={0},*tptr=0,ldir[MAX_PATH];
			strcpy(ldir,fkey);
			tptr=strrchr(ldir,'\\');
			if(!tptr) tptr=strrchr(ldir,'/');
			if(tptr) *tptr=0;
			if((strchr(ldir,'*'))||(strchr(ldir,'?')))
				*ldir=0;
			// Absolute local path
			if((ldir[1]==':')||(ldir[0]=='\\')||(ldir[0]=='/'))
				sprintf(tpath,"%s\\%s",ldir,rpath[1]==':'?rpath+2:rpath);
			// Relative local path to temp directory not run directory
			else
				sprintf(tpath,"%s\\%s\\%s",WSTEMPFILESDIR,ldir,rpath[1]==':'?rpath+2:rpath);
		#ifdef WIN32
			WSHMakeLocalDirs(pmod,tpath);
			HANDLE lfp=CreateFile(tpath,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
		#else
			sprintf(tpath,"./%s/%s",WSTEMPFILESDIR,fkey);
			FILE *lfp=fopen(tpath,"wb");
		#endif
			if(lfp==INVALID_FILE_VALUE)
				return -1;
			pfile=new WSTempFile;
			pfile->proto=op;
			pfile->download=true;
			pfile->fkey=fkey;
			pfile->rfp=rfp;
			pfile->tpath=tpath;
			pfile->lfp=lfp;
			pfile->begin=begin;
			pfile->rsize=rsize;
			pfile->tsize=tsize;
			pfile->off=begin;
            #ifdef WIN32
            memset(&pfile->ftCreation, 0, sizeof(pfile->ftCreation));
            memset(&pfile->ftLastAccess, 0, sizeof(pfile->ftLastAccess));
            memset(&pfile->ftLastWrite, 0, sizeof(pfile->ftLastWrite));
            #endif
            pfile->rid=*rid;
            pfile->PortType=PortType;
            pfile->PortNo=PortNo;
            pfile->OnBehalfOf=OnBehalfOf;
            pfile->multi=true;
			fileMap[fmkey]=pfile;
			#ifdef VS2010
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)rfp));
			#else
			char rstr[32]={0};
			sprintf(rstr,"%08X",rfp);
            const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
			#endif
			fpMap[fpkey]=pfile;
			// Don't leave empty space in local file when begin>0 for remote file
		}
		// Multi-part transfer
		else
		{
			pfile=fit->second;
			// Download
			if(pfile->download)
			{
				if(pfile->off!=begin)
					#ifdef WIN32
					WSHLogError(pmod,"CON%d: Download(%s) for %d bytes already in progress-restarting at %I64d.",
					#else
					WSHLogError(pmod,"CON%d: Download(%s) for %d bytes already in progress-restarting at %lld.",
					#endif
						PortNo,fkey,rsize,begin);
			}
			// Upload
			else
			{
				pfile->rsize=rsize;
				pfile->tsize=tsize;
			}
			// Seek to start of write
			if(pfile->lfp==INVALID_FILE_VALUE)
				return -1;
		#ifdef WIN32
			LARGE_INTEGER off;
			off.QuadPart=begin;
			SetFilePointer(pfile->lfp,off.LowPart,&off.HighPart,FILE_BEGIN);
		#else
			LONGLONG off=begin;
			fseeko(pfile->lfp,off,SEEK_SET);
		#endif
			pfile->off=begin;
			// Always keep the remote file handle up-to-date
			if(pfile->rfp!=rfp)
			{
				pfile->rfp=rfp;
				#ifdef VS2010
                const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+to_string((LONGLONG)pfile->rfp));
				#else
				char rstr[32]={0};
				sprintf(rstr,"%08X",(PTRCAST)pfile->rfp);
                const string fpkey(pmod->pcfg->aclass+"."+pmod->pcfg->aname+"/"+OnBehalfOf+"/"+(string)rstr);
				#endif
				fpMap[fpkey]=pfile;
			}
		}
		// Return parameters
		strncpy(fkey,pfile->fkey.c_str(),klen);
		strncpy(lpath,pfile->fpath.c_str(),llen);
        pmod->WSFileTransfered(*pfile);
		return 1;
	}
	return -1;
}
