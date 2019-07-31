
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

#if !defined(WS_OIO)&&!defined(WS_OTPP)
#error This code isnt ready yet!!!

// Linux doesn't have WinSock2 overlapped I/O
int WsocksHostImpl::WSHAsyncLoop(WsocksApp *pmod)
{
	int tid=GetCurrentThreadId();
	struct timeval TimeVal={0,0};
	fd_set Fd_Set;
	int Status;
	int SendSize=0;
	int SendBlockSize=0;
#ifdef WS_COMPRESS
	int CompSize=0;
	unsigned int CompBuffSize=0;
#endif
	unsigned int PreCompBlock=WS_COMP_BLOCK_LEN;
#ifdef WS_COMPRESS
	char CompBuff[(WS_MAX_BLOCK_SIZE*1024)*2]={0};
	#ifdef WS_ENCRYPTED
	unsigned int EncryptSize=0;
	#endif
#endif

	// CON ports
	for(int i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		// Lock the port
		WaitForSingleObject(pmod->ConPort[i].tmutex,INFINITE);
		if((!pmod->ConPort[i].SockConnected)||(pmod->ConPort[i].activeThread!=0))
		{
			ReleaseMutex(pmod->ConPort[i].tmutex);
			continue;
		}
		pmod->ConPort[i].activeThread=tid;
		//printf("DEBUG CON%d active %d\n",i,tid);
		ReleaseMutex(pmod->ConPort[i].tmutex);

Con_Read_More:
		WS_FD_ZERO(&Fd_Set);
		WS_FD_SET(pmod->ConPort[i].Sock,&Fd_Set);
		WSHSelect(pmod->ConPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
		Status=WS_FD_ISSET(pmod->ConPort[i].Sock,&Fd_Set);
		if(Status)
		{
			// Prevent infinite loop reading
        	DWORD& lastReadyStart=pmod->ConPort[i].lastReadyStart;
			DWORD tlast=lastReadyStart;
			DWORD tnow=GetTickCount();
			lastReadyStart=0;
			if((!tlast)||((int)(tnow -tlast)<200))
			{
			    lastReadyStart=tlast?tlast:tnow;
				if(!WSHConRead(pmod,i))
					goto Con_Read_Done;
				else
					goto Con_Read_More;
			}
		}

		// Read All Pending Messages
		if(!pmod->ConPort[i].InBuffer.Busy)
		{	
			pmod->ConPort[i].InBuffer.Busy=GetCurrentThreadId();
			if((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status>=10)&&(pmod->ConPort[i].S5Status<100))
			{
				switch (pmod->ConPort[i].S5Status)
				{
					case 10:
						if(pmod->ConPort[i].InBuffer.LocalSize>=2)
						{
							int Len=2;
							int Version=pmod->ConPort[i].InBuffer.Block[0];
							int Methode=pmod->ConPort[i].InBuffer.Block[1];
							if ((Version!=pmod->ConPort[i].S5Version)||(Methode!=pmod->ConPort[i].S5Methode))
								goto S5Error;
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
							switch(AdressType)
							{
								case 1:
									Len+=4; break;
								default:
								goto S5Error;
							}
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
				}
				goto S5Done;
S5Error:
							WSHCloseConPort(pmod,i);
							WSHLogError(pmod,"Socks5 Failed on CON%d",i);
							break;
S5Done:;
			}
			else
			{
				while(pmod->WSConMsgReady(i))
				{
					pmod->WSConStripMsg(i);
				}
			}
			pmod->ConPort[i].InBuffer.Busy=0;
		}
Con_Read_Done:
		if(!((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status<100)))
			pmod->WSBeforeConSend(i);
		// Send as much data out as posible
		if((pmod->ConPort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
			||
			(pmod->ConPort[i].OutCompBuffer.LocalSize>0)
#endif
			)
		{
			if((pmod->ConPort[i].OutBuffer.Busy)&&(pmod->ConPort[i].OutBuffer.Busy!=GetCurrentThreadId())
			{
				WSHLogError(pmod,"!CRASH: CON%d OutBuffer.Busy detected a possible thread %d crash.",
					i,pmod->ConPort[i].OutBuffer.Busy);
				_ASSERT(FALSE);
				pmod->ConPort[i].OutBuffer.Busy=0;
			}
			if(!pmod->ConPort[i].OutBuffer.Busy)
			{
				pmod->ConPort[i].OutBuffer.Busy=GetCurrentThreadId();
				pmod->ConPort[i].SendTimeOut=pmod->ConPort[i].SendTimeOut%1000;
Con_Send_More:
#ifdef WS_COMPRESS
				if(((pmod->ConPort[i].Compressed)||(pmod->ConPort[i].CompressType>0))&&(!((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status<100))))
				{
					if(pmod->ConPort[i].OutCompBuffer.LocalSize<=0)
					{
						if(pmod->ConPort[i].OutBuffer.LocalSize<=0)
							goto Con_Send_Done;
						if(pmod->ConPort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
							CompSize=((PreCompBlock*1024)*99/100);
						else
							CompSize=pmod->ConPort[i].OutBuffer.LocalSize;
						CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
#ifdef WS_ENCRYPTED
						mtcompress(CompBuff,&CompBuffSize
							,pmod->ConPort[i].OutBuffer.Block,CompSize);
						if(pmod->ConPort[i].EncryptionOn)
						{
							WSHEncrypt(pmod,&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
								,CompBuff,CompBuffSize,i,WS_CON);
						}
						else
						{
							EncryptSize=CompBuffSize;
							memcpy(&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
						}
						*((unsigned int *)&pmod->ConPort[i].OutCompBuffer.Block[0])=EncryptSize;
						pmod->ConPort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
						if(pmod->ConPort[i].CompressType==2)
							cscompress(&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->ConPort[i].OutBuffer.Block,(unsigned int*)&CompSize,0);
						else
							mtcompress(&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->ConPort[i].OutBuffer.Block,CompSize);
						*((unsigned int*)&pmod->ConPort[i].OutCompBuffer.Block[0])=CompBuffSize;
						pmod->ConPort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
						WSHRecord(pmod,&pmod->ConPort[i].Recording,pmod->ConPort[i].OutBuffer.Block,CompSize,true);
						WSStripBuff(&pmod->ConPort[i].OutBuffer, CompSize);
					}
                        if(pmod->ConPort[i].ChokeSize)
                        {
                            if((pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize) < pmod->ConPort[i].OutCompBuffer.LocalSize)
                                SendBlockSize=(pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize);
                            else
                                SendBlockSize=pmod->ConPort[i].OutCompBuffer.LocalSize;
                        }
                        else
                            SendBlockSize=pmod->ConPort[i].OutCompBuffer.LocalSize;
                        if(SendBlockSize)
                        {
					    Status=WSHSendPort(pmod->ConPort[i].Sock, pmod->ConPort[i].OutCompBuffer.Block
						    , SendBlockSize, NO_FLAGS_SET );
					    if(Status>0)
					    {
							//ResetSendTimeout(WS_CON,i);
							WSHRecord(pmod,&pmod->ConPort[i].Recording,pmod->ConPort[i].OutBuffer.Block,Status,true);
						    WSStripBuff(&pmod->ConPort[i].OutCompBuffer, Status);
						    pmod->ConPort[i].BytesOut+=Status;
						    pmod->ConPort[i].BlocksOut++;
						    pmod->ConPort[i].SendTimeOut=0;
                                pmod->ConPort[i].LastChokeSize+=Status;
						    goto Con_Send_More;
					    }
					    else //retry's due to Port Busy
					    {
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->ConPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Reset by Peer",i,pmod->ConPort[i].CfgNote);
								else
									WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Send Failed: %d",i,pmod->ConPort[i].CfgNote,lerr);
								WSHCloseConPort(pmod,i);
							}
					    }
                        }
				}
				else
#endif
				{
					if(pmod->ConPort[i].OutBuffer.LocalSize>((unsigned int)((pmod->ConPort[i].BlockSize-1)*1024)))
						SendSize=((pmod->ConPort[i].BlockSize-1)*1024);
					else
						SendSize=pmod->ConPort[i].OutBuffer.LocalSize;
                        if(pmod->ConPort[i].ChokeSize)
                        {
                            if((int)(pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize) < SendSize)
                                SendBlockSize=(pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize);
                            else
                                SendBlockSize=SendSize;
                        }
                        else
                            SendBlockSize=SendSize;
                        if(SendBlockSize)
                        {
					    Status=WSHSendPort(pmod->ConPort[i].Sock, pmod->ConPort[i].OutBuffer.Block, SendBlockSize, NO_FLAGS_SET );
					    if(Status>0)
					    {
							//ResetSendTimeout(WS_CON,i);
							WSHRecord(pmod,&pmod->ConPort[i].Recording,pmod->ConPort[i].OutBuffer.Block,Status,true);
						    WSStripBuff(&pmod->ConPort[i].OutBuffer, Status);
						    pmod->ConPort[i].BytesOut+=Status;
						    pmod->ConPort[i].BlocksOut++;
						    pmod->ConPort[i].SendTimeOut=0;
                                pmod->ConPort[i].LastChokeSize+=Status;
						    if(Status<SendSize)
							    goto Con_Send_Done;
						    if (pmod->ConPort[i].OutBuffer.Size>0)
							    goto Con_Send_More;
					    }
					    else //retry's due to Port Busy
					    {
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->ConPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Reset by Peer",i,pmod->ConPort[i].CfgNote);
								else
									WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Send Failed: %d",i,pmod->ConPort[i].CfgNote,lerr);
								WSHCloseConPort(pmod,i);
							}
					    }
                        }
				}
Con_Send_Done:
				pmod->ConPort[i].OutBuffer.Busy=0;
			}
			else // retry's due to BuffBusy
			{
				pmod->ConPort[i].SendTimeOut++;
			}
			if(pmod->ConPort[i].SendTimeOut>WS_CON_TIMEOUT)
			{
				WSHCloseConPort(pmod,i);
			}
		}
//			pmod->ConPort[i].SinceLastBeatCount++;
		if(pmod->ConPort[i].Bounce)
		{
			DWORD l=GetTickCount();
			if(pmod->ConPort[i].LastDataTime)
			{
				if((l-pmod->ConPort[i].LastDataTime)>pmod->ConPort[i].Bounce)
				{
					while (TRUE) // Added to please KMA and not use a GOTO  :-)
					{
						int x = (WSTime/100);

						if ((pmod->ConPort[i].BounceStart)&&((WSTime/100)<pmod->ConPort[i].BounceStart))
							break;

						if ((pmod->ConPort[i].BounceEnd)&&((WSTime/100)>pmod->ConPort[i].BounceEnd))
							break;

						WSHLogError(pmod,"!WSOCKS: ConPort[%d], was Bounced last data receved %ldms ago",i,(l-pmod->ConPort[i].LastDataTime));
						WSHCloseConPort(pmod,i);
						break;
					}
				}
			}
			else
				pmod->ConPort[i].LastDataTime=l;
		}

		// Unlock the port
		WaitForSingleObject(pmod->ConPort[i].tmutex,INFINITE);
		pmod->ConPort[i].activeThread=0;
		ReleaseMutex(pmod->ConPort[i].tmutex);
	}

	// USR ports
	for(int i=0;i<pmod->NO_OF_USR_PORTS;i++)
	{
		if(!pmod->UsrPort[i].SockActive)
			continue;
		// Lock the port
		WaitForSingleObject(pmod->UsrPort[i].tmutex,INFINITE);
		if(pmod->UsrPort[i].activeThread!=0)
		{
			ReleaseMutex(pmod->UsrPort[i].tmutex);
			continue;
		}
		pmod->UsrPort[i].activeThread=tid;
		ReleaseMutex(pmod->UsrPort[i].tmutex);

		// Unlock the port
		WaitForSingleObject(pmod->UsrPort[i].tmutex,INFINITE);
		pmod->UsrPort[i].activeThread=0;
		ReleaseMutex(pmod->UsrPort[i].tmutex);
	}
	return 0;
}

int WsocksHostImpl::WSHSyncLoop(WsocksApp *pmod)
{
	//struct {
	//	long tv_sec;         // seconds 
	//	long tv_usec;        // and microseconds 
	//} TimeVal={0,0};
	struct timeval TimeVal={0,0};
	fd_set Fd_Set;
	int Status;
//	unsigned long LongStatus;
	int i;
	int SendSize=0;
	int SendBlockSize=0;
#ifdef WS_COMPRESS
	int CompSize=0;
	unsigned int CompBuffSize=0;
	#ifdef WS_ENCRYPTED
	//static char CompBuff[(WS_MAX_BLOCK_SIZE*1024)*2]={0};
	char *CompBuff=pmod->SyncLoop_CompBuff;
	unsigned int EncryptSize=0;
	#endif
#endif
#ifdef WS_CAST
	SOCKADDR_IN cast_sin;  
#endif//#ifdef WS_CAST
	int StillActive;
	unsigned int PreCompBlock=WS_COMP_BLOCK_LEN;

	//LastTickDiv=dwTime-LastTickCnt;
	//LastTickCnt=dwTime;

#ifdef WS_PRE_TICK
	pmod->WSPreTick();
#endif

	if (pmod->WSSuspendMode==1)
	{
		StillActive=FALSE;

		for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
		{
			if(pmod->ConPort[i].SockConnected)
			{
				StillActive=TRUE;
				break;
			}
		}
		for(i=0;i<pmod->NO_OF_USR_PORTS;i++)
		{
			if(pmod->UsrPort[i].SockActive)
			{
				StillActive=TRUE;
				break;
			}
		}
#ifdef WS_MONITOR
		for(i=0;i<pmod->NO_OF_UMR_PORTS;i++)
		{
			if(pmod->UmrPort[i].SockActive)
			{
				StillActive=TRUE;
				break;
			}
		}
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
		for(i=0;i<pmod->NO_OF_CTO_PORTS;i++)
		{
			if(pmod->CtoPort[i].SockActive)
			{
				StillActive=TRUE;
				break;
			}
		}
		for(i=0;i<pmod->NO_OF_CTI_PORTS;i++)
		{
			if(pmod->CtiPort[i].SockActive)
			{
				StillActive=TRUE;
				break;
			}
		}
#endif //#ifdef WS_CAST
		if(!StillActive)
			pmod->WSSuspendMode=2;
	}

	for(i=0;i<pmod->NO_OF_USC_PORTS;i++)
	{
		if((pmod->UscPort[i].InUse)&&(!pmod->UscPort[i].ConnectHold)&&(!pmod->UscPort[i].SockActive))
			WSHOpenUscPort(pmod,i);
		if((pmod->UscPort[i].InUse)&&(pmod->UscPort[i].ConnectHold)&&(pmod->UscPort[i].SockActive))
		{
			WSHCloseUscPort(pmod,i);
			// Why drop all accepted users?
			//for(j=0;j<pmod->NO_OF_USR_PORTS;j++)
			//{
			//	if((pmod->UsrPort[j].SockActive)&&(pmod->UsrPort[j].UscPort==i))
			//		WSHCloseUsrPort(pmod,j);
			//}
		}
		if(pmod->UscPort[i].SockActive)
		{
			WSHUsrAccept(pmod,i);
		}
	}
#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_UGC_PORTS;i++)
	{
		if((pmod->UgcPort[i].InUse)&&(!pmod->UgcPort[i].ConnectHold)&&(!pmod->UgcPort[i].SockActive))
			WSHOpenUgcPort(pmod,i);
		if((pmod->UgcPort[i].InUse)&&(pmod->UgcPort[i].ConnectHold)&&(pmod->UgcPort[i].SockActive))
		{
			WSHCloseUgcPort(pmod,i);
			// Why drop all accepted users?
			//for(j=0;j<pmod->NO_OF_UGR_PORTS;j++)
			//{
			//	if((pmod->UgrPort[j].SockActive)&&(pmod->UgrPort[j].UgcPort==i))
			//		WSHCloseUgrPort(pmod,j);
			//}
		}
		if(pmod->UgcPort[i].SockActive)
		{
			WSHUgrAccept(pmod,i);
		}
	}
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_UMC_PORTS;i++)
	{
		if((pmod->UmcPort[i].InUse)&&(!pmod->UmcPort[i].ConnectHold)&&(!pmod->UmcPort[i].SockActive))
			WSHOpenUmcPort(pmod,i);
		if((pmod->UmcPort[i].InUse)&&(pmod->UmcPort[i].ConnectHold)&&(pmod->UmcPort[i].SockActive))
		{
			WSHCloseUmcPort(pmod,i);
			// Why drop all accepted users?
			//for(j=0;j<pmod->NO_OF_UMR_PORTS;j++)
			//{
			//	if((pmod->UmrPort[j].SockActive)&&(pmod->UmrPort[j].UmcPort==i))
			//		WSHCloseUmrPort(pmod,j);
			//}
		}
		if(pmod->UmcPort[i].SockActive)
		{
			WSHUmrAccept(pmod,i);
		}
	}
#endif //#ifdef WS_MONITOR
	int tid=GetCurrentThreadId();
	for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		// Lock the port
		WaitForSingleObject(pmod->ConPort[i].tmutex,INFINITE);
		if(pmod->ConPort[i].activeThread!=0)
		{
			ReleaseMutex(pmod->ConPort[i].tmutex);
			continue;
		}
		pmod->ConPort[i].activeThread=tid;
		//printf("DEBUG CON%d active %d\n",i,tid);
		ReleaseMutex(pmod->ConPort[i].tmutex);

 		if((pmod->ConPort[i].InUse)&&(!pmod->ConPort[i].ConnectHold)&&(!pmod->ConPort[i].SockOpen))
			WSHOpenConPort(pmod,i);
		if((pmod->ConPort[i].InUse)&&(pmod->ConPort[i].ConnectHold)&&(pmod->ConPort[i].SockOpen))
			WSHCloseConPort(pmod,i);
		if(pmod->ConPort[i].ReconCount)
		{
			Status=WSHFinishedConnect(pmod->ConPort[i].Sock);
			if(Status>0)
			{
				WSHConConnected(pmod,i);
				pmod->ConPort[i].ReconCount=0;
			}
			else
			{
				// The old code was a terrible way of tracking timeout
				//pmod->ConPort[i].ReconCount--;
				//if((pmod->ConPort[i].ReconCount<=1)||(Status<0))
				if((Status<0)||
				   ((GetTickCount() -(DWORD)pmod->ConPort[i].ReconCount)<86400000)) // tick count may have wrapped
				{
					WSHCloseConPort(pmod,i);
				}
			}
		}

		// Unlock the port
		WaitForSingleObject(pmod->ConPort[i].tmutex,INFINITE);
		pmod->ConPort[i].activeThread=0;
		ReleaseMutex(pmod->ConPort[i].tmutex);
	}

#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if((pmod->CgdPort[i].InUse)&&(!pmod->CgdPort[i].ConnectHold)&&(!pmod->CgdPort[i].SockOpen))
			WSHOpenCgdPort(pmod,i);
		if((pmod->CgdPort[i].InUse)&&(pmod->CgdPort[i].ConnectHold)&&(pmod->CgdPort[i].SockOpen))
			WSHCloseCgdPort(pmod,i);
		if(pmod->CgdPort[i].ReconCount)
		{
			Status=WSHFinishedConnect(pmod->CgdPort[i].Sock);
			if(Status>0)
			{
				WSHCgdConnected(pmod,i);
				pmod->CgdPort[i].ReconCount=0;
			}
			else
			{
				//pmod->CgdPort[i].ReconCount--;
				//if((pmod->CgdPort[i].ReconCount<=1)||(Status<0))
				if((Status<0)||
				   ((GetTickCount() -(DWORD)pmod->CgdPort[i].ReconCount)<86400000)) // tick count may have wrapped
				{
					WSHCloseCgdPort(pmod,i);
				}
			}
		}
	}
#endif //#ifdef WS_GUARANTEED
#ifdef WS_FILE_SERVER
    for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
    {
		// Connect file ports
		if((pmod->FilePort[i].InUse)&&(!pmod->FilePort[i].ConnectHold)&&(!pmod->FilePort[i].SockOpen))
			WSHOpenFilePort(pmod,i);
		if((pmod->FilePort[i].InUse)&&(pmod->FilePort[i].ConnectHold)&&(pmod->FilePort[i].SockOpen))
			WSHCloseFilePort(pmod,i);
		if(pmod->FilePort[i].ReconCount)
		{
			Status=WSHFinishedConnect(pmod->FilePort[i].Sock);
			if(Status>0)
			{
				WSHFileConnected(pmod,i);
				pmod->FilePort[i].ReconCount=0;
			}
			else
			{
				//pmod->FilePort[i].ReconCount--;
				//if((pmod->FilePort[i].ReconCount<=1)||(Status<0))
				if((Status<0)||
				   ((GetTickCount() -(DWORD)pmod->FilePort[i].ReconCount)<86400000)) // tick count may have wrapped
				{
					WSHCloseFilePort(pmod,i);
				}
			}
		}
	}
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_CAST
	for(i=0;i<pmod->NO_OF_CTO_PORTS;i++)
	{
		if((pmod->CtoPort[i].InUse)&&(!pmod->CtoPort[i].ConnectHold)&&(!pmod->CtoPort[i].SockActive))
			WSHOpenCtoPort(pmod,i);
		if((pmod->CtoPort[i].InUse)&&(pmod->CtoPort[i].ConnectHold)&&(pmod->CtoPort[i].SockActive))
			WSHCloseCtoPort(pmod,i);
	}
	for(i=0;i<pmod->NO_OF_CTI_PORTS;i++)
	{
		if((pmod->CtiPort[i].InUse)&&(!pmod->CtiPort[i].ConnectHold)&&(!pmod->CtiPort[i].SockActive))
			WSHOpenCtiPort(pmod,i);
		if((pmod->CtiPort[i].InUse)&&(pmod->CtiPort[i].ConnectHold)&&(pmod->CtiPort[i].SockActive))
			WSHCloseCtiPort(pmod,i);
	}
#endif //#ifdef WS_CAST

	for(i=0;i<pmod->NO_OF_USR_PORTS;i++)
	{
		if(pmod->UsrPort[i].SockActive)
		{
Usr_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->UsrPort[i].Sock,&Fd_Set);
			WSHSelect(pmod->UsrPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=WS_FD_ISSET(pmod->UsrPort[i].Sock,&Fd_Set);
			if(Status)
			{
				// Prevent infinite loop reading
        		DWORD& lastReadyStart=pmod->UsrPort[i].lastReadyStart;
				DWORD tlast=lastReadyStart;
				DWORD tnow=GetTickCount();
				lastReadyStart=0;
				if((!tlast)||((int)(tnow -tlast)<200))
				{
					lastReadyStart=tlast?tlast:tnow;
					if(!WSHUsrRead(pmod,i))
						goto Usr_Read_Done;
					else
						goto Usr_Read_More;
				}
			}

			// Read All Pending Messages
			if(!pmod->UsrPort[i].InBuffer.Busy)
			{
				pmod->UsrPort[i].InBuffer.Busy=GetCurrentThreadId();
				while(pmod->WSUsrMsgReady(i))
				{
					pmod->WSUsrStripMsg(i);
				}
				pmod->UsrPort[i].InBuffer.Busy=0;
			}
Usr_Read_Done:
			pmod->WSBeforeUsrSend(i);
			// Send as much data out as posible
			if((pmod->UsrPort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
				||(pmod->UsrPort[i].OutCompBuffer.LocalSize>0)
#endif
				)
			{
				if((pmod->UsrPort[i].OutBuffer.Busy)&&(pmod->UsrPort[i].OutBuffer.Busy!=GetCurrentThreadId())
				{
					WSHLogError(pmod,"!CRASH: USR%d OutBuffer.Busy detected a possible thread %d crash.",
						i,pmod->UsrPort[i].OutBuffer.Busy);
					_ASSERT(FALSE);
					pmod->UsrPort[i].OutBuffer.Busy=0;
				}
				if(!pmod->UsrPort[i].OutBuffer.Busy)
				{
					pmod->UsrPort[i].OutBuffer.Busy=GetCurrentThreadId();
Usr_Send_More:
#ifdef WS_COMPRESS
					if(pmod->UscPort[pmod->UsrPort[i].UscPort].Compressed)
					{
						if(pmod->UsrPort[i].OutCompBuffer.LocalSize<=0)
						{
							if(pmod->UsrPort[i].OutBuffer.LocalSize<=0)
								goto Usr_Send_Done;
							if(pmod->UsrPort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
								CompSize=((PreCompBlock*1024)*99/100);
							else
								CompSize=pmod->UsrPort[i].OutBuffer.LocalSize;
							CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
	#ifdef WS_ENCRYPTED
							mtcompress(CompBuff,&CompBuffSize
								,pmod->UsrPort[i].OutBuffer.Block,CompSize);
							if(pmod->UsrPort[i].EncryptionOn)
							{
								WSHEncrypt(pmod,&pmod->UsrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
									,CompBuff,CompBuffSize,i,WS_USR);
							}
							else
							{
								EncryptSize=CompBuffSize;
								memcpy(&pmod->UsrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
							}
							*((unsigned int *)&pmod->UsrPort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->UsrPort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
	#else
							mtcompress(&pmod->UsrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->UsrPort[i].OutBuffer.Block,CompSize);
							*((unsigned int*)&pmod->UsrPort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->UsrPort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
	#endif
							WSHRecord(pmod,&pmod->UsrPort[i].Recording,pmod->UsrPort[i].OutBuffer.Block,CompSize,true);
							WSStripBuff(&pmod->UsrPort[i].OutBuffer, CompSize);
						//#ifdef _DEBUG
						//	// Compression statistics
						//	static __int64 itot=0,otot=0,rtime=0;
						//	itot+=CompSize;
						//	otot+=CompBuffSize;
						//	DWORD tnow=GetTickCount();
						//	if(tnow -rtime>=1000)
						//	{
						//		char dstr[256];
						//		int perc=(int)((__int64)otot*100/(itot?itot:1));
						//		sprintf(dstr,"WSOCKS COMPRESSED: %I64d to %I64d (%d%%)\n",itot,otot,perc);
						//		OutputDebugString(dstr);
						//		rtime=tnow;
						//	}
						//#endif
						}
						//else if(pmod->UscPort[pmod->UsrPort[i].UscPort].CompressType==2)
						//{
						//	if(pmod->UsrPort[i].OutCompBuffer.LocalSize<=0)
						//	{
						//		if(pmod->UsrPort[i].OutBuffer.LocalSize<=0)
						//			goto Usr_Send_Done;
						//		CompSize=pmod->UsrPort[i].OutBuffer.LocalSize;
						//		if(CompSize>(WS_MAX_BLOCK_SIZE -1)*1024)
						//			CompSize=(WS_MAX_BLOCK_SIZE -1)*1024;
						//		CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2 -8;
						//		// Note that Comstock compression will compress up to the last message
						//		// so CompSize will be returned slightly smaller.
						//		cscompress(&pmod->UsrPort[i].OutCompBuffer.Block[4+sizeof(unsigned int)],&CompBuffSize
						//			,pmod->UsrPort[i].OutBuffer.Block,(unsigned int*)&CompSize,false);
						//		if(!CompSize)
						//		{
						//			pmod->UsrPort[i].OutBuffer.Busy=0;
						//			continue;
						//		}
						//		memset(pmod->UsrPort[i].OutCompBuffer.Block,0xFD,4);
						//		memcpy(pmod->UsrPort[i].OutCompBuffer.Block +4,&CompBuffSize,sizeof(unsigned int));
						//		pmod->UsrPort[i].OutCompBuffer.LocalSize=4+sizeof(unsigned int)+CompBuffSize;
						//		WSHRecord(pmod,&pmod->UsrPort[i].Recording,pmod->UsrPort[i].OutBuffer.Block,CompSize,true);
						//		WSStripBuff(&pmod->UsrPort[i].OutBuffer, CompSize);
						//	//#ifdef _DEBUG
						//	//	// Compression statistics
						//	//	static __int64 itot=0,otot=0,rtime=0;
						//	//	itot+=CompSize;
						//	//	otot+=CompBuffSize;
						//	//	DWORD tnow=GetTickCount();
						//	//	if(tnow -rtime>=1000)
						//	//	{
						//	//		char dstr[256];
						//	//		int perc=(int)((__int64)otot*100/(itot?itot:1));
						//	//		sprintf(dstr,"WSOCKS COMPRESSED: %I64d to %I64d (%d%%)\n",itot,otot,perc);
						//	//		OutputDebugString(dstr);
						//	//		rtime=tnow;
						//	//	}
						//	//#endif
						//	}
						//}
						Status=WSHSendPort(pmod->UsrPort[i].Sock, pmod->UsrPort[i].OutCompBuffer.Block
							, pmod->UsrPort[i].OutCompBuffer.LocalSize, NO_FLAGS_SET );
						if(Status>0)
						{
							//ResetSendTimeout(WS_USR,i);
							WSStripBuff(&pmod->UsrPort[i].OutCompBuffer, Status);
							pmod->UsrPort[i].BytesOut+=Status;
							pmod->UsrPort[i].BlocksOut++;
							pmod->UsrPort[i].SendTimeOut=0;
							goto Usr_Send_More;
						}
						else //retry's due to Port Busy
						{
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->UsrPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: UsrPort[%d][%s] Reset by Peer",i,pmod->UsrPort[i].Note);
								else
									WSHLogEvent(pmod,"!WSOCKS: UsrPort[%d][%s] Send Failed: %d",i,pmod->UsrPort[i].Note,lerr);
								WSHCloseUsrPort(pmod,i);
							}
						}
					}
					else
#endif
					{ 
						if(pmod->UsrPort[i].OutBuffer.LocalSize>((unsigned int)((pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize-1)*1024)))
							SendSize=((pmod->UscPort[pmod->UsrPort[i].UscPort].BlockSize-1)*1024);
						else
							SendSize=pmod->UsrPort[i].OutBuffer.LocalSize;
						Status=WSHSendPort(pmod->UsrPort[i].Sock, pmod->UsrPort[i].OutBuffer.Block, SendSize, NO_FLAGS_SET );
						if(Status>0)
						{
							//ResetSendTimeout(WS_USR,i);
							WSHRecord(pmod,&pmod->UsrPort[i].Recording,pmod->UsrPort[i].OutBuffer.Block,Status,true);
							WSStripBuff(&pmod->UsrPort[i].OutBuffer, Status);
							pmod->UsrPort[i].BytesOut+=Status;
							pmod->UsrPort[i].BlocksOut++;
							pmod->UsrPort[i].SendTimeOut=0;
							if(Status<SendSize)
								goto Usr_Send_Done;
							if (pmod->UsrPort[i].OutBuffer.Size>0)
								goto Usr_Send_More;
						}
						else //retry's due to Port Busy
						{
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->UsrPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: UsrPort[%d][%s] Reset by Peer",i,pmod->UsrPort[i].Note);
								else
									WSHLogEvent(pmod,"!WSOCKS: UsrPort[%d][%s] Send Failed: %d",i,pmod->UsrPort[i].Note,lerr);
								WSHCloseUsrPort(pmod,i);
							}
						}
					}
Usr_Send_Done:
					pmod->UsrPort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->UsrPort[i].SendTimeOut++;
				}
				if(pmod->UsrPort[i].SendTimeOut>WS_USR_TIMEOUT)
				{
					WSHLogError(pmod,"!WSOCKS: UsrPort[%d][%s] - Dropped - after %ld Retry's]"
						,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].SendTimeOut);
					WSHCloseUsrPort(pmod,i);
				}
				if(pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutSize)
				{
					if((pmod->UsrPort[i].SinceOpenTimer > pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut)
						&&(pmod->UsrPort[i].OutBuffer.Size > pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutSize))
					{
						if((!pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod)||
						   ((pmod->UsrPort[i].TimeOutStart)&&(time(0) -pmod->UsrPort[i].TimeOutStart>=pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod)))
						{
							if(pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod)
								WSHLogError(pmod,"!WSOCKS: UsrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld for %ld[%ld] Seconds after %ld[%ld] Seconds Since Login"
									,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].RemoteIP,pmod->UsrPort[i].OutBuffer.Size
									,time(0) -pmod->UsrPort[i].TimeOutStart,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod
									,pmod->UsrPort[i].SinceOpenTimer,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut);
							else
								WSHLogError(pmod,"!WSOCKS: UsrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
									,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].RemoteIP,pmod->UsrPort[i].OutBuffer.Size
									,pmod->UsrPort[i].SinceOpenTimer,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut);
							WSHCloseUsrPort(pmod,i);
						}
						else if(!pmod->UsrPort[i].TimeOutStart)
						{
							pmod->UsrPort[i].TimeOutStart=time(0);
							WSHLogEvent(pmod,"!WSOCKS: UsrPort[%d][%s][%s] - Timer Started[%ld] - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
								,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].RemoteIP,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod,pmod->UsrPort[i].OutBuffer.Size
								,pmod->UsrPort[i].SinceOpenTimer,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut);
						}
					}
					else
						pmod->UsrPort[i].TimeOutStart=0;
				}
			}
			pmod->UsrPort[i].SinceLastBeatCount++;
		}
	}

#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if(pmod->UgrPort[i].SockActive)
		{
Ugr_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->UgrPort[i].Sock,&Fd_Set);
			WSHSelect(pmod->UgrPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=WS_FD_ISSET(pmod->UgrPort[i].Sock,&Fd_Set);
			if(Status)
			{
				if(!WSUgrRead(i))
					continue;
				else
					goto Ugr_Read_More;
			}

			// Read All Pending Messages
			if(!pmod->UgrPort[i].InBuffer.Busy)
			{
				pmod->UgrPort[i].InBuffer.Busy=GetCurrentThreadId();
				while(WSUgrMsgReady(i))
				{
					WSUgrStripMsg(i);
				}
				pmod->UgrPort[i].InBuffer.Busy=0;
			}
			_WSHBeforeUgrSend(pmod,i);
			// Send as much data out as posible
			if((pmod->UgrPort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
				||(pmod->UgrPort[i].OutCompBuffer.LocalSize>0)
#endif
				)
			{
				if(!pmod->UgrPort[i].OutBuffer.Busy)
				{
					pmod->UgrPort[i].OutBuffer.Busy=GetCurrentThreadId();
Ugr_Send_More:
#ifdef WS_COMPRESS
					if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].Compressed)
					{
						if(pmod->UgrPort[i].OutCompBuffer.LocalSize<=0)
						{
							if(pmod->UgrPort[i].OutBuffer.LocalSize<=0)
								goto Ugr_Send_Done;
							if(pmod->UgrPort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
								CompSize=((PreCompBlock*1024)*99/100);
							else
								CompSize=pmod->UgrPort[i].OutBuffer.LocalSize;
							CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
#ifdef WS_ENCRYPTED
							mtcompress(CompBuff,&CompBuffSize
								,pmod->UgrPort[i].OutBuffer.Block,CompSize);
							if(pmod->UgrPort[i].EncryptionOn)
							{
								WSHEncrypt(pmod,&pmod->UgrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
									,CompBuff,CompBuffSize,i,WS_UGR);
							}
							else
							{
								EncryptSize=CompBuffSize;
								memcpy(&pmod->UgrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
							}
							*((unsigned int*)&pmod->UgrPort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->UgrPort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
							mtcompress(&pmod->UgrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->UgrPort[i].OutBuffer.Block,CompSize);
							*((unsigned int*)&pmod->UgrPort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->UgrPort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
							WSHRecord(pmod,&pmod->UgrPort[i].Recording,pmod->UgrPort[i].OutBuffer.Block,CompSize,true);
							WSStripBuff(&pmod->UgrPort[i].OutBuffer, CompSize);
						}
						Status=WSHSendPort(pmod->UgrPort[i].Sock, pmod->UgrPort[i].OutCompBuffer.Block
							, pmod->UgrPort[i].OutCompBuffer.LocalSize, NO_FLAGS_SET );
						if(Status>0)
						{
							//ResetSendTimeout(WS_UGR,i);
							WSStripBuff(&pmod->UgrPort[i].OutCompBuffer, Status);
							pmod->UgrPort[i].BytesOut+=Status;
							pmod->UgrPort[i].BlocksOut++;
							pmod->UgrPort[i].SendTimeOut=0;
							goto Ugr_Send_More;
						}
						else //retry's due to Port Busy
						{
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->UgrPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: UgrPort[%d][%s] Reset by Peer",i,pmod->UgrPort[i].Note);
								else
									WSHLogEvent(pmod,"!WSOCKS: UgrPort[%d][%s] Send Failed: %d",i,pmod->UgrPort[i].Note,lerr);
								WSHCloseUgrPort(pmod,i);
							}
						}
					}
					else
#endif
					{ 
						if(pmod->UgrPort[i].OutBuffer.LocalSize>((unsigned int)((pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize-1)*1024)))
							SendSize=((pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize-1)*1024);
						else
							SendSize=pmod->UgrPort[i].OutBuffer.LocalSize;
						Status=WSHSendPort(pmod->UgrPort[i].Sock, pmod->UgrPort[i].OutBuffer.Block, SendSize, NO_FLAGS_SET );
						if(Status>0)
						{
							//ResetSendTimeout(WS_UGR,i);
							WSHRecord(pmod,&pmod->UgrPort[i].Recording,pmod->UgrPort[i].OutBuffer.Block,Status,true);
							WSStripBuff(&pmod->UgrPort[i].OutBuffer, Status);
							pmod->UgrPort[i].BytesOut+=Status;
							pmod->UgrPort[i].BlocksOut++;
							pmod->UgrPort[i].SendTimeOut=0;
							if(Status<SendSize)
								goto Ugr_Send_Done;
							if (pmod->UgrPort[i].OutBuffer.Size>0)
								goto Ugr_Send_More;
						}
						else //retry's due to Port Busy
						{
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->UgrPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: UgrPort[%d][%s] Reset by Peer",i,pmod->UgrPort[i].Note);
								else
									WSHLogEvent(pmod,"!WSOCKS: UgrPort[%d][%s] Send Failed: %d",i,pmod->UgrPort[i].Note,lerr);
								WSHCloseUgrPort(pmod,i);
							}
						}
					}
Ugr_Send_Done:
					pmod->UgrPort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->UgrPort[i].SendTimeOut++;
				}
				if(pmod->UgrPort[i].SendTimeOut>WS_USR_TIMEOUT)
				{
					WSHLogError(pmod,"!WSOCKS: UgrPort[%d][%s] - Dropped - after %ld Retry's]"
						,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].SendTimeOut);
					WSHCloseUgrPort(pmod,i);
				}
				if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutSize)
				{
					if((pmod->UgrPort[i].SinceOpenTimer > pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut)
						&&(pmod->UgrPort[i].OutBuffer.Size > pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutSize))
					{
						if((!pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod)||
						   ((pmod->UgrPort[i].TimeOutStart)&&(time(0) -pmod->UgrPort[i].TimeOutStart>=pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod)))
						{
							if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod)
								WSHLogError(pmod,"!WSOCKS: UgrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld for %ld[%ld] Seconds after %ld[%ld] Seconds Since Login"
									,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].RemoteIP,pmod->UgrPort[i].OutBuffer.Size
									,time(0) -pmod->UgrPort[i].TimeOutStart,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod
									,pmod->UgrPort[i].SinceOpenTimer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut);
							else
								WSHLogError(pmod,"!WSOCKS: UgrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
									,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].RemoteIP,pmod->UgrPort[i].OutBuffer.Size
									,pmod->UgrPort[i].SinceOpenTimer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut);
							WSHCloseUgrPort(pmod,i);
						}
						else if(!pmod->UgrPort[i].TimeOutStart)
						{
							pmod->UgrPort[i].TimeOutStart=time(0);
							WSHLogEvent(pmod,"!WSOCKS: UgrPort[%d][%s][%s] - Timer Started[%ld] - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
								,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].RemoteIP,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod,pmod->UgrPort[i].OutBuffer.Size
								,pmod->UgrPort[i].SinceOpenTimer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut);
						}
					}
					else
						pmod->UgrPort[i].TimeOutStart=0;
				}
			}
			pmod->UgrPort[i].SinceLastBeatCount++;
		}
	}
#endif //#ifdef WS_GUARANTEED

#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	{
		if(pmod->UmrPort[i].SockActive)
		{
Umr_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->UmrPort[i].Sock,&Fd_Set);
			WSHSelect(pmod->UmrPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=WS_FD_ISSET(pmod->UmrPort[i].Sock,&Fd_Set);
			if(Status)
			{
				if(!WSHUmrRead(pmod,i))
					continue;
				else
					goto Umr_Read_More;
			}

			// Read All Pending Messages
			if(!pmod->UmrPort[i].InBuffer.Busy)
			{
				pmod->UmrPort[i].InBuffer.Busy=GetCurrentThreadId();
				while(pmod->WSUmrMsgReady(i))
				{
					pmod->WSUmrStripMsg(i);
				}
				pmod->UmrPort[i].InBuffer.Busy=0;
			}
			pmod->WSBeforeUmrSend(i);
			// Send as much data out as posible
			if((pmod->UmrPort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
				||(pmod->UmrPort[i].OutCompBuffer.LocalSize>0)
#endif
				)
			{
				if(!pmod->UmrPort[i].OutBuffer.Busy)
				{
					pmod->UmrPort[i].OutBuffer.Busy=GetCurrentThreadId();
Umr_Send_More:
#ifdef WS_COMPRESS
					if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].Compressed)
					{
						if(pmod->UmrPort[i].OutCompBuffer.LocalSize<=0)
						{
							if(pmod->UmrPort[i].OutBuffer.LocalSize<=0)
								goto Umr_Send_Done;
							if(pmod->UmrPort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
								CompSize=((PreCompBlock*1024)*99/100);
							else
								CompSize=pmod->UmrPort[i].OutBuffer.LocalSize;
							CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
#ifdef WS_ENCRYPTED
							mtcompress(CompBuff,&CompBuffSize
								,pmod->UmrPort[i].OutBuffer.Block,CompSize);
							if(pmod->UmrPort[i].EncryptionOn)
							{
								WSHEncrypt(pmod,&pmod->UmrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
									,CompBuff,CompBuffSize,i,WS_UMR);
							}
							else
							{
								EncryptSize=CompBuffSize;
								memcpy(&pmod->UmrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
							}
							*((unsigned int*)&pmod->UmrPort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->UmrPort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
							mtcompress(&pmod->UmrPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->UmrPort[i].OutBuffer.Block,CompSize);
							*((unsigned int*)&pmod->UmrPort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->UmrPort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
							WSStripBuff(&pmod->UmrPort[i].OutBuffer, CompSize);
						}
						Status=WSHSendPort(pmod->UmrPort[i].Sock, pmod->UmrPort[i].OutCompBuffer.Block, pmod->UmrPort[i].OutCompBuffer.LocalSize, NO_FLAGS_SET );
						if(Status>0)
						{
							//ResetSendTimeout(WS_UMR,i);
							WSStripBuff(&pmod->UmrPort[i].OutCompBuffer, Status);
							pmod->UmrPort[i].BytesOut+=Status;
							pmod->UmrPort[i].BlocksOut++;
							pmod->UmrPort[i].SendTimeOut=0;
							goto Umr_Send_More;
						}
						else //retry's due to Port Busy
						{
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->UmrPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: UmrPort[%d][%s] Reset by Peer",i,pmod->UmrPort[i].Note);
								else
									WSHLogEvent(pmod,"!WSOCKS: UmrPort[%d][%s] Send Failed: %d",i,pmod->UmrPort[i].Note,lerr);
								WSHCloseUmrPort(pmod,i);
							}
						}
					}
					else
#endif
					{
						if(pmod->UmrPort[i].OutBuffer.LocalSize>(unsigned int)((pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize-1)*1024))
							SendSize=((pmod->UmcPort[pmod->UmrPort[i].UmcPort].BlockSize-1)*1024);
						else
							SendSize=pmod->UmrPort[i].OutBuffer.LocalSize;
						Status=WSHSendPort(pmod->UmrPort[i].Sock, pmod->UmrPort[i].OutBuffer.Block, SendSize, NO_FLAGS_SET );
						if(Status>0)
						{
							//ResetSendTimeout(WS_UMR,i);
							WSStripBuff(&pmod->UmrPort[i].OutBuffer, Status);
							pmod->UmrPort[i].BytesOut+=Status;
							pmod->UmrPort[i].BlocksOut++;
							pmod->UmrPort[i].SendTimeOut=0;
							if(Status<SendSize)
								goto Umr_Send_Done;
							if (pmod->UmrPort[i].OutBuffer.Size>0)
								goto Umr_Send_More;
						}
						else //retry's due to Port Busy
						{
							int lerr=WSAGetLastError();
							if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
								pmod->UmrPort[i].SendTimeOut++;
							else
							{
								if(lerr==WSAECONNRESET)
									WSHLogEvent(pmod,"!WSOCKS: UmrPort[%d][%s] Reset by Peer",i,pmod->UmrPort[i].Note);
								else
									WSHLogEvent(pmod,"!WSOCKS: UmrPort[%d][%s] Send Failed: %d",i,pmod->UmrPort[i].Note,lerr);
								WSHCloseUmrPort(pmod,i);
							}
						}
					}
Umr_Send_Done:
					pmod->UmrPort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->UmrPort[i].SendTimeOut++;
				}
				if(pmod->UmrPort[i].SendTimeOut>WS_UMR_TIMEOUT)
				{
					WSHLogError(pmod,"!WSOCKS: UmrPort[%d][%s] - Dropped - after %ld Retry's]"
						,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].SendTimeOut);
					WSHCloseUmrPort(pmod,i);
				}
				if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutSize)
				{
					if((pmod->UmrPort[i].SinceOpenTimer > pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut)
						&&(pmod->UmrPort[i].OutBuffer.Size > pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutSize))
					{
						if((!pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod)||
						   ((pmod->UmrPort[i].TimeOutStart)&&(time(0) -pmod->UmrPort[i].TimeOutStart>=pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod)))
						{
							if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod)
								WSHLogError(pmod,"!WSOCKS: UmrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld for %ld[%ld] Seconds after %ld[%ld] Seconds Since Login"
									,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].RemoteIP,pmod->UmrPort[i].OutBuffer.Size
									,time(0) -pmod->UmrPort[i].TimeOutStart,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod
									,pmod->UmrPort[i].SinceOpenTimer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut);
							else
								WSHLogError(pmod,"!WSOCKS: UmrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
									,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].RemoteIP,pmod->UmrPort[i].OutBuffer.Size
									,pmod->UmrPort[i].SinceOpenTimer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut);
							WSHCloseUmrPort(pmod,i);
						}
						else if(!pmod->UmrPort[i].TimeOutStart)
						{
							pmod->UmrPort[i].TimeOutStart=time(0);
							WSHLogEvent(pmod,"!WSOCKS: UmrPort[%d][%s][%s] - Timer Started[%ld] - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
								,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].RemoteIP,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod,pmod->UmrPort[i].OutBuffer.Size
								,pmod->UmrPort[i].SinceOpenTimer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut);
						}
					}
					else
						pmod->UmrPort[i].TimeOutStart=0;
				}
			}
			pmod->UmrPort[i].SinceLastBeatCount++;
		}
	}
#endif //#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		// Lock the port
		WaitForSingleObject(pmod->ConPort[i].tmutex,INFINITE);
		if(pmod->ConPort[i].activeThread!=0)
		{
			ReleaseMutex(pmod->ConPort[i].tmutex);
			continue;
		}
		pmod->ConPort[i].activeThread=tid;
		//printf("DEBUG CON%d active %d\n",i,tid);
		ReleaseMutex(pmod->ConPort[i].tmutex);

		if((pmod->ConPort[i].SockConnected)||((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status>=10)&&(pmod->ConPort[i].S5Status<100)))
		{
Con_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->ConPort[i].Sock,&Fd_Set);
			WSHSelect(pmod->ConPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=WS_FD_ISSET(pmod->ConPort[i].Sock,&Fd_Set);
			if(Status)
			{
				// Prevent infinite loop reading
        		DWORD& lastReadyStart=pmod->ConPort[i].lastReadyStart;
			    DWORD tlast=lastReadyStart;
			    DWORD tnow=GetTickCount();
			    lastReadyStart=0;
			    if((!tlast)||((int)(tnow -tlast)<200))
				{
			        lastReadyStart=tlast?tlast:tnow;
					if(!WSHConRead(pmod,i))
						goto Con_Read_Done;
					else
						goto Con_Read_More;
				}
			}
			// Read All Pending Messages
			if(!pmod->ConPort[i].InBuffer.Busy)
			{	
				pmod->ConPort[i].InBuffer.Busy=GetCurrentThreadId();
				if((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status>=10)&&(pmod->ConPort[i].S5Status<100))
				{
					switch (pmod->ConPort[i].S5Status)
					{
						case 10:
							if(pmod->ConPort[i].InBuffer.LocalSize>=2)
							{
								int Len=2;
								int Version=pmod->ConPort[i].InBuffer.Block[0];
								int Methode=pmod->ConPort[i].InBuffer.Block[1];
								if ((Version!=pmod->ConPort[i].S5Version)||(Methode!=pmod->ConPort[i].S5Methode))
									goto S5Error;
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
								switch(AdressType)
								{
									case 1:
										Len+=4; break;
									default:
									goto S5Error;
								}
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
					}
					goto S5Done;
S5Error:
								WSHCloseConPort(pmod,i);
								WSHLogError(pmod,"Socks5 Failed on CON%d",i);
								break;
S5Done:;
				}
				else
				{
					while(pmod->WSConMsgReady(i))
					{
						pmod->WSConStripMsg(i);
					}
				}
				pmod->ConPort[i].InBuffer.Busy=0;
			}
Con_Read_Done:
			if(!((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status<100)))
				pmod->WSBeforeConSend(i);
			// Send as much data out as posible
			if((pmod->ConPort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
				||
				(pmod->ConPort[i].OutCompBuffer.LocalSize>0)
#endif
				)
			{
				if((pmod->ConPort[i].OutBuffer.Busy)&&(pmod->ConPort[i].OutBuffer.Busy!=GetCurrentThreadId())
				{
					WSHLogError(pmod,"!CRASH: CON%d OutBuffer.Busy detected a possible thread %d crash.",
						i,pmod->ConPort[i].OutBuffer.Busy);
					_ASSERT(FALSE);
					pmod->ConPort[i].OutBuffer.Busy=0;
				}
				if(!pmod->ConPort[i].OutBuffer.Busy)
				{
					pmod->ConPort[i].OutBuffer.Busy=GetCurrentThreadId();
					pmod->ConPort[i].SendTimeOut=pmod->ConPort[i].SendTimeOut%1000;
Con_Send_More:
#ifdef WS_COMPRESS
					if(pmod->ConPort[i].Compressed&&(!((pmod->ConPort[i].S5Connect)&&(pmod->ConPort[i].S5Status<100))))
					{
						if(pmod->ConPort[i].OutCompBuffer.LocalSize<=0)
						{
							if(pmod->ConPort[i].OutBuffer.LocalSize<=0)
								goto Con_Send_Done;
							if(pmod->ConPort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
								CompSize=((PreCompBlock*1024)*99/100);
							else
								CompSize=pmod->ConPort[i].OutBuffer.LocalSize;
							CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
#ifdef WS_ENCRYPTED
							mtcompress(CompBuff,&CompBuffSize
								,pmod->ConPort[i].OutBuffer.Block,CompSize);
							if(pmod->ConPort[i].EncryptionOn)
							{
								WSHEncrypt(pmod,&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
									,CompBuff,CompBuffSize,i,WS_CON);
							}
							else
							{
								EncryptSize=CompBuffSize;
								memcpy(&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
							}
							*((unsigned int *)&pmod->ConPort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->ConPort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
							if(pmod->ConPort[i].CompressType==2)
								cscompress(&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
									,pmod->ConPort[i].OutBuffer.Block,(unsigned int*)&CompSize,0);
							else
								mtcompress(&pmod->ConPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
									,pmod->ConPort[i].OutBuffer.Block,CompSize);
							*((unsigned int*)&pmod->ConPort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->ConPort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
							WSHRecord(pmod,&pmod->ConPort[i].Recording,pmod->ConPort[i].OutBuffer.Block,CompSize,true);
							WSStripBuff(&pmod->ConPort[i].OutBuffer, CompSize);
						}
                        if(pmod->ConPort[i].ChokeSize)
                        {
                            if((pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize) < pmod->ConPort[i].OutCompBuffer.LocalSize)
                                SendBlockSize=(pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize);
                            else
                                SendBlockSize=pmod->ConPort[i].OutCompBuffer.LocalSize;
                        }
                        else
                            SendBlockSize=pmod->ConPort[i].OutCompBuffer.LocalSize;
                        if(SendBlockSize)
                        {
						    Status=WSHSendPort(pmod->ConPort[i].Sock, pmod->ConPort[i].OutCompBuffer.Block
							    , SendBlockSize, NO_FLAGS_SET );
						    if(Status>0)
						    {
								//ResetSendTimeout(WS_CON,i);
								WSHRecord(pmod,&pmod->ConPort[i].Recording,pmod->ConPort[i].OutBuffer.Block,Status,true);
							    WSStripBuff(&pmod->ConPort[i].OutCompBuffer, Status);
							    pmod->ConPort[i].BytesOut+=Status;
							    pmod->ConPort[i].BlocksOut++;
							    pmod->ConPort[i].SendTimeOut=0;
                                pmod->ConPort[i].LastChokeSize+=Status;
							    goto Con_Send_More;
						    }
						    else //retry's due to Port Busy
						    {
								int lerr=WSAGetLastError();
								if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
									pmod->ConPort[i].SendTimeOut++;
								else
								{
									if(lerr==WSAECONNRESET)
										WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Reset by Peer",i,pmod->ConPort[i].CfgNote);
									else
										WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Send Failed: %d",i,pmod->ConPort[i].CfgNote,lerr);
									WSHCloseConPort(pmod,i);
								}
						    }
                        }
					}
					else
#endif
					{
						if(pmod->ConPort[i].OutBuffer.LocalSize>((unsigned int)((pmod->ConPort[i].BlockSize-1)*1024)))
							SendSize=((pmod->ConPort[i].BlockSize-1)*1024);
						else
							SendSize=pmod->ConPort[i].OutBuffer.LocalSize;
                        if(pmod->ConPort[i].ChokeSize)
                        {
                            if((int)(pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize) < SendSize)
                                SendBlockSize=(pmod->ConPort[i].ChokeSize-pmod->ConPort[i].LastChokeSize);
                            else
                                SendBlockSize=SendSize;
                        }
                        else
                            SendBlockSize=SendSize;
                        if(SendBlockSize)
                        {
						    Status=WSHSendPort(pmod->ConPort[i].Sock, pmod->ConPort[i].OutBuffer.Block, SendBlockSize, NO_FLAGS_SET );
						    if(Status>0)
						    {
								//ResetSendTimeout(WS_CON,i);
								WSHRecord(pmod,&pmod->ConPort[i].Recording,pmod->ConPort[i].OutBuffer.Block,Status,true);
							    WSStripBuff(&pmod->ConPort[i].OutBuffer, Status);
							    pmod->ConPort[i].BytesOut+=Status;
							    pmod->ConPort[i].BlocksOut++;
							    pmod->ConPort[i].SendTimeOut=0;
                                pmod->ConPort[i].LastChokeSize+=Status;
							    if(Status<SendSize)
								    goto Con_Send_Done;
							    if (pmod->ConPort[i].OutBuffer.Size>0)
								    goto Con_Send_More;
						    }
						    else //retry's due to Port Busy
						    {
								int lerr=WSAGetLastError();
								if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
									pmod->ConPort[i].SendTimeOut++;
								else
								{
									if(lerr==WSAECONNRESET)
										WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Reset by Peer",i,pmod->ConPort[i].CfgNote);
									else
										WSHLogEvent(pmod,"!WSOCKS: ConPort[%d][%s] Send Failed: %d",i,pmod->ConPort[i].CfgNote,lerr);
									WSHCloseConPort(pmod,i);
								}
						    }
                        }
					}
Con_Send_Done:
					pmod->ConPort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->ConPort[i].SendTimeOut++;
				}
				if(pmod->ConPort[i].SendTimeOut>WS_CON_TIMEOUT)
				{
					WSHCloseConPort(pmod,i);
				}
			}
//			pmod->ConPort[i].SinceLastBeatCount++;
			if(pmod->ConPort[i].Bounce)
			{
				DWORD l=GetTickCount();
				if(pmod->ConPort[i].LastDataTime)
				{
					if((l-pmod->ConPort[i].LastDataTime)>pmod->ConPort[i].Bounce)
					{
						while (TRUE) // Added to please KMA and not use a GOTO  :-)
						{
							int x = (WSTime/100);

							if ((pmod->ConPort[i].BounceStart)&&((WSTime/100)<pmod->ConPort[i].BounceStart))
								break;

							if ((pmod->ConPort[i].BounceEnd)&&((WSTime/100)>pmod->ConPort[i].BounceEnd))
								break;

							WSHLogError(pmod,"!WSOCKS: ConPort[%d], was Bounced last data receved %ldms ago",i,(l-pmod->ConPort[i].LastDataTime));
							WSHCloseConPort(pmod,i);
							break;
						}
					}
				}
				else
					pmod->ConPort[i].LastDataTime=l;
			}
		}
		// Unlock the port
		WaitForSingleObject(pmod->ConPort[i].tmutex,INFINITE);
		pmod->ConPort[i].activeThread=0;
		ReleaseMutex(pmod->ConPort[i].tmutex);
	}
#ifdef WS_FILE_SERVER
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
			Status=WS_FD_ISSET(pmod->FilePort[i].Sock,&Fd_Set);
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
								WSHCloseFilePort(pmod,i);
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
							WSCommitFileTransfer(i);                       
						else
						{
							for ( int f=0; f<pmod->WSFileOptions.xfersPerTick; f++ )
							{
								if ( wft->status == WSF_COMMIT_INPROGRESS ||
									 WSRecvFileTransfer(i)<=0 )
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
				pmod->FilePort[i].InBuffer.Busy=0;
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
				if((pmod->FilePort[i].OutBuffer.Busy)&&(pmod->FilePort[i].OutBuffer.Busy!=GetCurrentThreadId())
				{
					WSHLogError(pmod,"!CRASH: FILE%d OutBuffer.Busy detected a possible thread %d crash.",
						i,pmod->FilePort[i].OutBuffer.Busy);
					_ASSERT(FALSE);
					pmod->FilePort[i].OutBuffer.Busy=0;
				}
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
							*((unsigned int*)&pmod->FilePort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->FilePort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
							mtcompress(&pmod->FilePort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->FilePort[i].OutBuffer.Block,CompSize);
							*((unsigned int*)&pmod->FilePort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->FilePort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
							WSHRecord(pmod,&pmod->FilePort[i].Recording,pmod->FilePort[i].OutBuffer.Block,CompSize,true);
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
							Status=WSHSendPort(pmod->FilePort[i].Sock, pmod->FilePort[i].OutCompBuffer.Block, SendBlockSize, NO_FLAGS_SET );
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
									WSHCloseFilePort(pmod,i);
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
							Status=WSHSendPort(pmod->FilePort[i].Sock, pmod->FilePort[i].OutBuffer.Block, SendBlockSize, NO_FLAGS_SET );
							if(Status>0)
							{
								//ResetSendTimeout(WS_FIL,i);
								WSHRecord(pmod,&pmod->FilePort[i].Recording,pmod->FilePort[i].OutBuffer.Block,Status,true);
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
									WSHCloseFilePort(pmod,i);
								}
							}
						}
					}
File_Send_Done:
					pmod->FilePort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->FilePort[i].SendTimeOut++;
				}
				if(pmod->FilePort[i].SendTimeOut>WS_FILE_TIMEOUT)
				{
					WSHCloseFilePort(pmod,i);
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
							int x = (WSTime/100);

							if ((pmod->FilePort[i].BounceStart)&&((WSTime/100)<pmod->FilePort[i].BounceStart))
								break;

							if ((pmod->FilePort[i].BounceEnd)&&((WSTime/100)>pmod->FilePort[i].BounceEnd))
								break;

							WSHLogError(pmod,"!WSOCKS: ConPort[%d], was Bounced last data receved %ldms ago",i,(l-pmod->FilePort[i].LastDataTime));
							WSHCloseFilePort(pmod,i);
							break;
						}
					}
				}
				else
					pmod->FilePort[i].LastDataTime=l;
			}
		}
    }
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if((pmod->CgdPort[i].SockConnected)||((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status>=10)&&(pmod->CgdPort[i].S5Status<100)))
		{
Cgd_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->CgdPort[i].Sock,&Fd_Set);
			WSHSelect(pmod->CgdPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=WS_FD_ISSET(pmod->CgdPort[i].Sock,&Fd_Set);
			if(Status)
			{
				if(!WSCgdRead(i))
					continue;
				else
					goto Cgd_Read_More;
			}
			// Read All Pending Messages
			if(!pmod->CgdPort[i].InBuffer.Busy)
			{
				pmod->CgdPort[i].InBuffer.Busy=GetCurrentThreadId();
				if((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status>=10)&&(pmod->CgdPort[i].S5Status<100))
				{
					switch (pmod->CgdPort[i].S5Status)
					{
						case 10:
							if(pmod->CgdPort[i].InBuffer.LocalSize>=2)
							{
								int Len=2;
								int Version=pmod->CgdPort[i].InBuffer.Block[0];
								int Methode=pmod->CgdPort[i].InBuffer.Block[1];
								if ((Version!=pmod->CgdPort[i].S5Version)||(Methode!=pmod->CgdPort[i].S5Methode))
									goto S5CgdError;
								WSStripBuff(&pmod->CgdPort[i].InBuffer, Len);
								WSHS5Connect(pmod,i);
								break;
							}
							break;
						case 20:
							if(pmod->CgdPort[i].InBuffer.LocalSize>=4)
							{
								unsigned int Len=4;
								int Version=pmod->CgdPort[i].InBuffer.Block[0];
								int Reply=pmod->CgdPort[i].InBuffer.Block[1];
								int AdressType=pmod->CgdPort[i].InBuffer.Block[3];
								switch(AdressType)
								{
									case 1:
										Len+=4; break;
									default:
									goto S5CgdError;
								}
								// AddPort Len
								Len+=2;
								if(pmod->CgdPort[i].InBuffer.LocalSize<Len)
									break;
								WSStripBuff(&pmod->CgdPort[i].InBuffer, Len);
								pmod->CgdPort[i].S5Status=100;
								pmod->WSCgdConnected(i);
								break;
							}
							break;
					}
					goto S5CgdDone;
S5CgdError:
								WSHCloseCgdPort(pmod,i);
								WSHLogError(pmod,"Socks5 Failed on CGD%d",i);
								break;
S5CgdDone:;
				}
				else
				{
					while(WSCgdMsgReady(i))
					{
						WSCgdStripMsg(i);
					}
				}
				pmod->CgdPort[i].InBuffer.Busy=0;
			}
			if(!((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status<100)))
				_WSHBeforeCgdSend(pmod,i);
			// Send as much data out as posible
			if((pmod->CgdPort[i].OutBuffer.LocalSize>0)
#ifdef WS_COMPRESS
				||
				(pmod->CgdPort[i].OutCompBuffer.LocalSize>0)
#endif
				)
			{
				if((pmod->CgdPort[i].OutBuffer.Busy)&&(pmod->CgdPort[i].OutBuffer.Busy!=GetCurrentThreadId())
				{
					WSHLogError(pmod,"!CRASH: CGD%d OutBuffer.Busy detected a possible thread %d crash.",
						i,pmod->CgdPort[i].OutBuffer.Busy);
					_ASSERT(FALSE);
					pmod->CgdPort[i].OutBuffer.Busy=0;
				}
				if(!pmod->CgdPort[i].OutBuffer.Busy)
				{
					pmod->CgdPort[i].OutBuffer.Busy=GetCurrentThreadId();
					pmod->CgdPort[i].SendTimeOut=pmod->CgdPort[i].SendTimeOut%1000;
Cgd_Send_More:
#ifdef WS_COMPRESS
					if(pmod->CgdPort[i].Compressed&&(!((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status<100))))
					{
						if(pmod->CgdPort[i].OutCompBuffer.LocalSize<=0)
						{
							if(pmod->CgdPort[i].OutBuffer.LocalSize<=0)
								goto Cgd_Send_Done;
							if(pmod->CgdPort[i].OutBuffer.LocalSize>((PreCompBlock*1024)*99/100))
								CompSize=((PreCompBlock*1024)*99/100);
							else
								CompSize=pmod->CgdPort[i].OutBuffer.LocalSize;
							CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
#ifdef WS_ENCRYPTED
							mtcompress(CompBuff,&CompBuffSize
								,pmod->CgdPort[i].OutBuffer.Block,CompSize);
							if(pmod->CgdPort[i].EncryptionOn)
							{
								WSHEncrypt(pmod,&pmod->CgdPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&EncryptSize
									,CompBuff,CompBuffSize,i,WS_CGD);
							}
							else
							{
								EncryptSize=CompBuffSize;
								memcpy(&pmod->CgdPort[i].OutCompBuffer.Block[sizeof(unsigned int)],CompBuff,EncryptSize);
							}
							*((unsigned int*)&pmod->CgdPort[i].OutCompBuffer.Block[0])=EncryptSize;
							pmod->CgdPort[i].OutCompBuffer.LocalSize=EncryptSize+sizeof(unsigned int);
#else
							mtcompress(&pmod->CgdPort[i].OutCompBuffer.Block[sizeof(unsigned int)],&CompBuffSize
								,pmod->CgdPort[i].OutBuffer.Block,CompSize);
							*((unsigned int*)&pmod->CgdPort[i].OutCompBuffer.Block[0])=CompBuffSize;
							pmod->CgdPort[i].OutCompBuffer.LocalSize=CompBuffSize+sizeof(unsigned int);
#endif
							WSStripBuff(&pmod->CgdPort[i].OutBuffer, CompSize);
						}
                        if(pmod->CgdPort[i].ChokeSize)
                        {
                            if((pmod->CgdPort[i].ChokeSize-pmod->CgdPort[i].LastChokeSize) < pmod->CgdPort[i].OutCompBuffer.LocalSize)
                                SendBlockSize=(pmod->CgdPort[i].ChokeSize-pmod->CgdPort[i].LastChokeSize);
                            else
                                SendBlockSize=pmod->CgdPort[i].OutCompBuffer.LocalSize;
                        }
                        else
                            SendBlockSize=pmod->CgdPort[i].OutCompBuffer.LocalSize;
                        if(SendBlockSize)
                        {
						    Status=WSHSendPort(pmod->CgdPort[i].Sock, pmod->CgdPort[i].OutCompBuffer.Block
							    , SendBlockSize, NO_FLAGS_SET );
						    if(Status>0)
						    {
								//ResetSendTimeout(WS_CGD,i);
							    WSStripBuff(&pmod->CgdPort[i].OutCompBuffer, Status);
							    pmod->CgdPort[i].BytesOut+=Status;
							    pmod->CgdPort[i].BlocksOut++;
							    pmod->CgdPort[i].SendTimeOut=0;
                                pmod->CgdPort[i].LastChokeSize+=Status;
							    goto Cgd_Send_More;
						    }
						    else //retry's due to Port Busy
						    {
								int lerr=WSAGetLastError();
								if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
									pmod->CgdPort[i].SendTimeOut++;
								else
								{
									if(lerr==WSAECONNRESET)
										WSHLogEvent(pmod,"!WSOCKS: CgdPort[%d][%s] Reset by Peer",i,pmod->CgdPort[i].CfgNote);
									else
										WSHLogEvent(pmod,"!WSOCKS: CgdPort[%d][%s] Send Failed: %d",i,pmod->CgdPort[i].CfgNote,lerr);
									WSHCloseCgdPort(pmod,i);
								}
						    }
                        }
					}
					else
#endif
					{
						if(pmod->CgdPort[i].OutBuffer.LocalSize>((unsigned int)((pmod->CgdPort[i].BlockSize-1)*1024)))
							SendSize=((pmod->CgdPort[i].BlockSize-1)*1024);
						else
							SendSize=pmod->CgdPort[i].OutBuffer.LocalSize;
                        if(pmod->CgdPort[i].ChokeSize)
                        {
                            if((int)(pmod->CgdPort[i].ChokeSize-pmod->CgdPort[i].LastChokeSize) < SendSize)
                                SendBlockSize=(pmod->CgdPort[i].ChokeSize-pmod->CgdPort[i].LastChokeSize);
                            else
                                SendBlockSize=SendSize;
                        }
                        else
                            SendBlockSize=SendSize;
                        if(SendBlockSize)
                        {
						    Status=WSHSendPort(pmod->CgdPort[i].Sock, pmod->CgdPort[i].OutBuffer.Block, SendBlockSize, NO_FLAGS_SET );
						    if(Status>0)
						    {
								//ResetSendTimeout(WS_CGD,i);
								WSHRecord(pmod,&pmod->CgdPort[i].Recording,pmod->CgdPort[i].OutBuffer.Block,Status,true);
							    WSStripBuff(&pmod->CgdPort[i].OutBuffer, Status);
							    pmod->CgdPort[i].BytesOut+=Status;
							    pmod->CgdPort[i].BlocksOut++;
							    pmod->CgdPort[i].SendTimeOut=0;
                                pmod->CgdPort[i].LastChokeSize+=Status;
							    if(Status<SendSize)
								    goto Cgd_Send_Done;
							    if (pmod->CgdPort[i].OutBuffer.Size>0)
								    goto Cgd_Send_More;
						    }
						    else //retry's due to Port Busy
						    {
								int lerr=WSAGetLastError();
								if((lerr==WSAEWOULDBLOCK)||(lerr==WSAENOBUFS))
									pmod->CgdPort[i].SendTimeOut++;
								else
								{
									if(lerr==WSAECONNRESET)
										WSHLogEvent(pmod,"!WSOCKS: CgdPort[%d][%s] Reset by Peer",i,pmod->CgdPort[i].CfgNote);
									else
										WSHLogEvent(pmod,"!WSOCKS: CgdPort[%d][%s] Send Failed: %d",i,pmod->CgdPort[i].CfgNote,lerr);
									WSHCloseCgdPort(pmod,i);
								}
						    }
                        }
					}
Cgd_Send_Done:
					pmod->CgdPort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->CgdPort[i].SendTimeOut++;
				}
				if(pmod->CgdPort[i].SendTimeOut>WS_CON_TIMEOUT)
				{
					WSHCloseCgdPort(pmod,i);
				}
			}
//			pmod->CgdPort[i].SinceLastBeatCount++;
			if(pmod->CgdPort[i].Bounce)
			{
				DWORD l=GetTickCount();
				if(pmod->CgdPort[i].LastDataTime)
				{
					if((l-pmod->CgdPort[i].LastDataTime)>pmod->CgdPort[i].Bounce)
					{
						while (TRUE) // Added to please KMA and not use a GOTO  :-)
						{
							int x = (WSTime/100);

							if ((pmod->CgdPort[i].BounceStart)&&((WSTime/100)<pmod->CgdPort[i].BounceStart))
								break;

							if ((pmod->CgdPort[i].BounceEnd)&&((WSTime/100)>pmod->CgdPort[i].BounceEnd))
								break;

							WSHLogError(pmod,"!WSOCKS: CgdPort[%d], was Bounced last data receved %ldms ago",i,(l-pmod->CgdPort[i].LastDataTime));
							WSHCloseCgdPort(pmod,i);
							break;
						}
					}
				}
				else
					pmod->CgdPort[i].LastDataTime=l;
			}
		}
	}
#endif //#ifdef WS_GUARANTEED
#ifdef WS_CAST
	for(i=0;i<pmod->NO_OF_CTI_PORTS;i++)
	{
		if(pmod->CtiPort[i].SockActive)
		{
Cti_Read_More:
			WS_FD_ZERO(&Fd_Set);
			WS_FD_SET(pmod->CtiPort[i].Sock,&Fd_Set);
			WSHSelect(pmod->CtiPort[i].Sock+1,&Fd_Set,NULL,NULL,(timeval*)&TimeVal);
			Status=WS_FD_ISSET(pmod->CtiPort[i].Sock,&Fd_Set);
			if(Status)
			{
				if(!WSHCtiRead(pmod,i))
					continue;
				else
					goto Cti_Read_More;
			}

			// Read All Pending Messages
			if(!pmod->CtiPort[i].InBuffer.Busy)
			{
				pmod->CtiPort[i].InBuffer.Busy=GetCurrentThreadId();
				while(pmod->WSCtiMsgReady(i))
				{
					pmod->WSCtiStripMsg(i);
				}
				pmod->CtiPort[i].InBuffer.Busy=0;
			}
			if(pmod->CtiPort[i].Bounce)
			{
				DWORD l=GetTickCount();
				if(pmod->CtiPort[i].LastDataTime)
				{
					if((l-pmod->CtiPort[i].LastDataTime)>pmod->CtiPort[i].Bounce)
					{
						while (TRUE) // Added to please KMA and not use a GOTO  :-)
						{
							if ((pmod->CtiPort[i].BounceStart)&&(WSTime/100<pmod->CtiPort[i].BounceStart))
								break;

							if ((pmod->CtiPort[i].BounceEnd)&&(WSTime/100>pmod->CtiPort[i].BounceEnd))
								break;

							WSHLogError(pmod,"!WSOCKS: CtiPort[%d], was Bounced last data receved %ldms ago",i,(l-pmod->CtiPort[i].LastDataTime));
							WSHCloseCtiPort(pmod,i);
							break;
						}
					}
				}
				else
					pmod->CtiPort[i].LastDataTime=l;
			}
		}
	}
	for(i=0;i<pmod->NO_OF_CTO_PORTS;i++)
	{
		if(pmod->CtoPort[i].SockActive)
		{
			// Send as much data out as posible
			if(pmod->CtoPort[i].OutBuffer.LocalSize>0)
			{
				if((pmod->CtoPort[i].OutBuffer.Busy)&&(pmod->CtoPort[i].OutBuffer.Busy!=GetCurrentThreadId())
				{
					WSHLogError(pmod,"!CRASH: CTI%d OutBuffer.Busy detected a possible thread %d crash.",
						i,pmod->CtoPort[i].OutBuffer.Busy);
					_ASSERT(FALSE);
					pmod->CtoPort[i].OutBuffer.Busy=0;
				}
				if(!pmod->CtoPort[i].OutBuffer.Busy)
				{
					pmod->CtoPort[i].OutBuffer.Busy=GetCurrentThreadId();
					pmod->CtoPort[i].SendTimeOut=pmod->CtoPort[i].SendTimeOut%1000;
Cto_Send_More:
					if(pmod->CtoPort[i].OutBuffer.LocalSize>((unsigned int)((pmod->CtoPort[i].BlockSize-1)*1024)))
						SendSize=((pmod->CtoPort[i].BlockSize-1)*1024);
					else
						SendSize=pmod->CtoPort[i].OutBuffer.LocalSize;
					cast_sin.sin_family = AF_INET;
					cast_sin.sin_port=htons(pmod->CtoPort[i].Port);
					cast_sin.sin_addr.s_addr=inet_addr(pmod->CtoPort[i].RemoteIP);

					Status=WSHSendToPort(pmod->CtoPort[i].Sock,pmod->CtoPort[i].OutBuffer.Block,SendSize,NO_FLAGS_SET
						,(struct sockaddr *)&cast_sin,sizeof(sockaddr_in));
					if(Status>0)
					{
						//ResetSendTimeout(WS_CTO,i);
						WSStripBuff(&pmod->CtoPort[i].OutBuffer, Status);
						pmod->CtoPort[i].BytesOut+=Status;
						pmod->CtoPort[i].BlocksOut++;
						pmod->CtoPort[i].SendTimeOut=0;
						if(Status<SendSize)
							goto Cto_Send_Done;
						if (pmod->CtoPort[i].OutBuffer.Size>0)
							goto Cto_Send_More;
					}
					else //retry's due to Port Busy
					{
int err=WSAGetLastError();
						pmod->CtoPort[i].SendTimeOut++;
					}
Cto_Send_Done:
					pmod->CtoPort[i].OutBuffer.Busy=0;
				}
				else // retry's due to BuffBusy
				{
					pmod->CtoPort[i].SendTimeOut++;
				}
				if(pmod->CtoPort[i].SendTimeOut>WS_CTO_TIMEOUT)
				{
					WSHCloseCtoPort(pmod,i);
				}
			}
		}
	}
#endif//#ifdef WS_CAST
	return 0;
}

#endif//!WS_OIO
