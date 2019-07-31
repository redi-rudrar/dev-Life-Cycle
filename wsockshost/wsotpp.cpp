
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

#ifdef WS_OTPP

// Only one app thread will be in WSHSyncLoop at any one time
int WsocksHostImpl::WSHSyncLoop(WsocksApp *pmod)
{
	int Status,i,StillActive;

	//LastTickDiv=dwTime-LastTickCnt;
	//LastTickCnt=dwTime;
	#ifdef WS_PRE_TICK
	pmod->WSPreTick();
	#endif

	// Suspend/Resume
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

	int tid=GetCurrentThreadId();
	// Accept connections
	for(i=0;i<pmod->NO_OF_USC_PORTS;i++)
	{
		if(!pmod->UscPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_USC,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->UscPort[i].InUse)
		{
			UnlockPort(pmod,WS_USC,i,false);
			continue;
		}
		pmod->UscPort[i].activeThread=tid;
		if((!pmod->UscPort[i].ConnectHold)&&(!pmod->UscPort[i].SockActive))
			WSHOpenUscPort(pmod,i);
		if((pmod->UscPort[i].ConnectHold)&&(pmod->UscPort[i].SockActive))
		{
			_WSHCloseUscPort(pmod,i);
			// Why drop all accepted users?
			//for(j=0;j<pmod->NO_OF_USR_PORTS;j++)
			//{
			//	if((pmod->UsrPort[j].SockActive)&&(pmod->UsrPort[j].UscPort==i))
			//		_WSHCloseUsrPort(pmod,j);
			//}
			pmod->UscPort[i].activeThread=0;
			UnlockPort(pmod,WS_USC,i,false);
			continue;
		}
		if(pmod->UscPort[i].appClosed)
		{
			pmod->UscPort[i].appClosed=false;
			_WSHCloseUscPort(pmod,i); 
			pmod->UscPort[i].activeThread=0;
			UnlockPort(pmod,WS_USC,i,false);
			continue;
		}
		if(pmod->UscPort[i].SockActive)
			WSHUsrAccept(pmod,i);
		pmod->UscPort[i].activeThread=0;
		UnlockPort(pmod,WS_USC,i,false);
	}

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_UGC_PORTS;i++)
	{
		if(!pmod->UgcPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_UGC,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->UgcPort[i].InUse)
		{
			UnlockPort(pmod,WS_UGC,i,false);
			continue;
		}
		pmod->UgcPort[i].activeThread=tid;
		if((!pmod->UgcPort[i].ConnectHold)&&(!pmod->UgcPort[i].SockActive))
			WSHOpenUgcPort(pmod,i);
		if((pmod->UgcPort[i].ConnectHold)&&(pmod->UgcPort[i].SockActive))
		{
			_WSHCloseUgcPort(pmod,i);
			// Must drop all accept users because depends on UGC send
			for(int j=0;j<pmod->NO_OF_UGR_PORTS;j++)
			{
				if((pmod->UgrPort[j].SockActive)&&(pmod->UgrPort[j].UgcPort==i))
					_WSHCloseUgrPort(pmod,j);
			}
			pmod->UgcPort[i].activeThread=0;
			UnlockPort(pmod,WS_UGC,i,false);
			for(int j=0;j<pmod->NO_OF_UGR_PORTS;j++)
			{
				if(pmod->UgrPort[j].pthread)
					_WSHWaitUgrThreadExit(pmod,j);
			}
			continue;
		}
		if(pmod->UgcPort[i].appClosed)
		{
			pmod->UgcPort[i].appClosed=false;
			_WSHCloseUgcPort(pmod,i); 
			pmod->UgcPort[i].activeThread=0;
			UnlockPort(pmod,WS_UGC,i,false);
			continue;
		}
		if(pmod->UgcPort[i].SockActive)
			WSHUgrAccept(pmod,i);
		pmod->UgcPort[i].activeThread=0;
		UnlockPort(pmod,WS_UGC,i,false);
	}
	#endif //#ifdef WS_GUARANTEED

	#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_UMC_PORTS;i++)
	{
		if(!pmod->UmcPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_UMC,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->UmcPort[i].InUse)
		{
			UnlockPort(pmod,WS_UMC,i,false);
			continue;
		}
		pmod->UmcPort[i].activeThread=tid;
		if((!pmod->UmcPort[i].ConnectHold)&&(!pmod->UmcPort[i].SockActive))
			WSHOpenUmcPort(pmod,i);
		if((pmod->UmcPort[i].ConnectHold)&&(pmod->UmcPort[i].SockActive))
		{
			_WSHCloseUmcPort(pmod,i);
			// Why drop all accepted users?
			//for(j=0;j<pmod->NO_OF_UMR_PORTS;j++)
			//{
			//	if((pmod->UmrPort[j].SockActive)&&(pmod->UmrPort[j].UmcPort==i))
			//		_WSHCloseUmrPort(pmod,j);
			//}
			pmod->UmcPort[i].activeThread=0;
			UnlockPort(pmod,WS_UMC,i,false);
			continue;
		}
		if(pmod->UmcPort[i].appClosed)
		{
			pmod->UmcPort[i].appClosed=false;
			_WSHCloseUmcPort(pmod,i); 
			pmod->UmcPort[i].activeThread=0;
			UnlockPort(pmod,WS_UMC,i,false);
			continue;
		}
		if(pmod->UmcPort[i].SockActive)
			WSHUmrAccept(pmod,i);
		pmod->UmcPort[i].activeThread=0;
		UnlockPort(pmod,WS_UMC,i,false);
	}
	#endif //#ifdef WS_MONITOR

	// Make connections
	for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		if(!pmod->ConPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_CON,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->ConPort[i].InUse)
		{
			UnlockPort(pmod,WS_CON,i,false);
			continue;
		}
		pmod->ConPort[i].recvThread=tid;
		//printf("DEBUG CON%d active %d\n",i,tid);

 		if((!pmod->ConPort[i].ConnectHold)&&(!pmod->ConPort[i].SockOpen))
			WSHOpenConPort(pmod,i);
		if((pmod->ConPort[i].ConnectHold)&&(pmod->ConPort[i].SockOpen))
		{
			_WSHCloseConPort(pmod,i);
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
			_WSHWaitConThreadExit(pmod,i);
			continue;
		}
		// Give the app one last shot to read the rest of the data
		if(pmod->ConPort[i].peerClosed)
		{
			while(pmod->WSConMsgReady(i))
				pmod->WSConStripMsg(i);
			_WSHCloseConPort(pmod,i); 
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
			_WSHWaitConThreadExit(pmod,i);
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->ConPort[i].appClosed)
		{
			LockPort(pmod,WS_CON,i,true);
			pmod->ConPort[i].sendThread=tid;
			WSHConSend(pmod,i,true);
			pmod->ConPort[i].sendThread=0;
			UnlockPort(pmod,WS_CON,i,true);

			_WSHCloseConPort(pmod,i); 
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
			_WSHWaitConThreadExit(pmod,i);
			continue;
		}
		else if(pmod->ConPort[i].ReconCount)
		{
			Status=WSHFinishedConnect(pmod->ConPort[i].Sock);
			if(Status>0)
			{
				WSHConConnected(pmod,i);
				pmod->ConPort[i].ReconCount=0;
			}
			else
			{
				// Connection timeout
				if((Status<0)||
				   ((GetTickCount() -(DWORD)pmod->ConPort[i].ReconCount)<86400000)) // tick count may have wrapped
				{
					_WSHCloseConPort(pmod,i);
					pmod->ConPort[i].recvThread=0;
					UnlockPort(pmod,WS_CON,i,false);
					_WSHWaitConThreadExit(pmod,i);
					continue;
				}
			}
		}
		#ifdef WS_LOOPBACK
		WSPort *pport=(WSPort*)pmod->ConPort[i].Sock;
		if((pport)&&(pport->lbaConnReset))
		{
			_WSHCloseConPort(pmod,i); 
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
			_WSHWaitConThreadExit(pmod,i);
			continue;
		}
		#endif
		pmod->ConPort[i].recvThread=0;
		UnlockPort(pmod,WS_CON,i,false);
	}

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if(!pmod->CgdPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_CGD,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->CgdPort[i].InUse)
		{
			UnlockPort(pmod,WS_CGD,i,false);
			continue;
		}
		pmod->CgdPort[i].recvThread=tid;
		if((!pmod->CgdPort[i].ConnectHold)&&(!pmod->CgdPort[i].SockOpen))
			WSHOpenCgdPort(pmod,i);
		if((pmod->CgdPort[i].ConnectHold)&&(pmod->CgdPort[i].SockOpen))
		{
			_WSHCloseCgdPort(pmod,i);
			pmod->CgdPort[i].recvThread=0;
			UnlockPort(pmod,WS_CGD,i,false);
			_WSHWaitCgdThreadExit(pmod,i);
			continue;
		}
		// Give the app one last shot to read the rest of the data
		if(pmod->CgdPort[i].peerClosed)
		{
			while(pmod->WSCgdMsgReady(i))
				pmod->WSCgdStripMsg(i);
			_WSHCloseCgdPort(pmod,i);
			pmod->CgdPort[i].recvThread=0;
			UnlockPort(pmod,WS_CGD,i,false);
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->CgdPort[i].appClosed)
		{
			LockPort(pmod,WS_CGD,i,true);
			pmod->CgdPort[i].sendThread=tid;
			WSHCgdSend(pmod,i,true);
			pmod->CgdPort[i].sendThread=0;
			UnlockPort(pmod,WS_CGD,i,true);

			_WSHCloseCgdPort(pmod,i); 
			pmod->CgdPort[i].recvThread=0;
			UnlockPort(pmod,WS_CGD,i,false);
			_WSHWaitCgdThreadExit(pmod,i);
			continue;
		}
		else if(pmod->CgdPort[i].ReconCount)
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
					_WSHCloseCgdPort(pmod,i);
					pmod->CgdPort[i].recvThread=0;
					UnlockPort(pmod,WS_CGD,i,false);
					_WSHWaitCgdThreadExit(pmod,i);
					continue;
				}
			}
		}
		pmod->CgdPort[i].recvThread=0;
		UnlockPort(pmod,WS_CGD,i,false);
	}
	#endif //#ifdef WS_GUARANTEED

	#ifdef WS_FILE_SERVER
    for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
    {
		if(!pmod->FilePort[i].InUse)
			continue;
		if(LockPort(pmod,WS_FIL,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->FilePort[i].InUse)
		{
			UnlockPort(pmod,WS_FIL,i,false);
			continue;
		}
		pmod->FilePort[i].recvThread=tid;
		// Connect file ports
		if((!pmod->FilePort[i].ConnectHold)&&(!pmod->FilePort[i].SockOpen))
			WSHOpenFilePort(pmod,i);
		if((pmod->FilePort[i].ConnectHold)&&(pmod->FilePort[i].SockOpen))
		{
			_WSHCloseFilePort(pmod,i);
			pmod->FilePort[i].recvThread=0;
			UnlockPort(pmod,WS_FIL,i,false);
			_WSHWaitFileThreadExit(pmod,i);
			continue;
		}
		// Give the app one last shot to read the rest of the data
		if(pmod->FilePort[i].peerClosed)
		{
			while(pmod->WSFileMsgReady(i))
				pmod->WSFileStripMsg(i);
			_WSHCloseFilePort(pmod,i);
			pmod->FilePort[i].recvThread=0;
			UnlockPort(pmod,WS_FIL,i,false);
			_WSHWaitFileThreadExit(pmod,i);
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->FilePort[i].appClosed)
		{
			LockPort(pmod,WS_FIL,i,true);
			pmod->FilePort[i].sendThread=tid;
			WSHFileSend(pmod,i,true);
			pmod->FilePort[i].sendThread=0;
			UnlockPort(pmod,WS_FIL,i,true);

			_WSHCloseFilePort(pmod,i); 
			pmod->FilePort[i].recvThread=0;
			UnlockPort(pmod,WS_FIL,i,false);
			_WSHWaitFileThreadExit(pmod,i);
			continue;
		}
		else if(pmod->FilePort[i].ReconCount)
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
					_WSHCloseFilePort(pmod,i);
					pmod->FilePort[i].recvThread=0;
					UnlockPort(pmod,WS_FIL,i,false);
					_WSHWaitFileThreadExit(pmod,i);
					continue;
				}
			}
		}
		pmod->FilePort[i].recvThread=0;
		UnlockPort(pmod,WS_FIL,i,false);
	}
	#endif //#ifdef WS_FILE_SERVER

	#ifdef WS_CAST
	for(i=0;i<pmod->NO_OF_CTO_PORTS;i++)
	{
		if(!pmod->CtoPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_CTO,i,true,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->CtoPort[i].InUse)
		{
			UnlockPort(pmod,WS_CTO,i,true);
			continue;
		}
		pmod->CtoPort[i].sendThread=tid;
		if((!pmod->CtoPort[i].ConnectHold)&&(!pmod->CtoPort[i].SockActive))
			WSHOpenCtoPort(pmod,i);
		if((pmod->CtoPort[i].ConnectHold)&&(pmod->CtoPort[i].SockActive))
		{
			_WSHCloseCtoPort(pmod,i);
			pmod->CtoPort[i].sendThread=0;
			UnlockPort(pmod,WS_CTO,i,true);
			continue;
		}
		if(pmod->CtoPort[i].appClosed)
		{
			_WSHCloseCtoPort(pmod,i); 
			pmod->CtoPort[i].sendThread=0;
			UnlockPort(pmod,WS_CTO,i,true);
			continue;
		}
		pmod->CtoPort[i].sendThread=0;
		UnlockPort(pmod,WS_CTO,i,true);
	}
	for(i=0;i<pmod->NO_OF_CTI_PORTS;i++)
	{
		if(!pmod->CtiPort[i].InUse)
			continue;
		if(LockPort(pmod,WS_CTI,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->CtiPort[i].InUse)
		{
			UnlockPort(pmod,WS_CTI,i,false);
			continue;
		}
		pmod->CtiPort[i].recvThread=tid;
		if((!pmod->CtiPort[i].ConnectHold)&&(!pmod->CtiPort[i].SockActive))
			WSHOpenCtiPort(pmod,i);
		if((pmod->CtiPort[i].ConnectHold)&&(pmod->CtiPort[i].SockActive))
		{
			_WSHCloseCtiPort(pmod,i);
			pmod->CtiPort[i].recvThread=0;
			UnlockPort(pmod,WS_CTI,i,false);
			_WSHWaitCtiThreadExit(pmod,i);
			continue;
		}
		if(pmod->CtiPort[i].appClosed)
		{
			_WSHCloseCtiPort(pmod,i); 
			pmod->CtiPort[i].recvThread=0;
			UnlockPort(pmod,WS_CTI,i,false);
			_WSHWaitCtiThreadExit(pmod,i);
			continue;
		}
		pmod->CtiPort[i].recvThread=0;
		UnlockPort(pmod,WS_CTI,i,false);
	}
	#endif //#ifdef WS_CAST

	// Receive timeout and send buffered data
	for(i=0;i<pmod->NO_OF_USR_PORTS;i++)
	{
		if(!pmod->UsrPort[i].Sock) // Account for TimeTillClose
			continue;
		if(LockPort(pmod,WS_USR,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->UsrPort[i].Sock)
		{
			UnlockPort(pmod,WS_USR,i,false);
			continue;
		}
		pmod->UsrPort[i].recvThread=tid;
		// Give the app one last shot to read the rest of the data
		if(pmod->UsrPort[i].peerClosed)
		{
			while(pmod->WSUsrMsgReady(i))
				pmod->WSUsrStripMsg(i);
			_WSHCloseUsrPort(pmod,i);
			pmod->UsrPort[i].recvThread=0;
			UnlockPort(pmod,WS_USR,i,false);
			_WSHWaitUsrThreadExit(pmod,i);
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->UsrPort[i].appClosed)
		{
			LockPort(pmod,WS_USR,i,true);
			pmod->UsrPort[i].sendThread=tid;
			WSHUsrSend(pmod,i,true);
			pmod->UsrPort[i].sendThread=0;
			UnlockPort(pmod,WS_USR,i,true);

			_WSHCloseUsrPort(pmod,i); 
			pmod->UsrPort[i].recvThread=0;
			UnlockPort(pmod,WS_USR,i,false);
			_WSHWaitUsrThreadExit(pmod,i);
			continue;
		}
		#ifdef WS_LOOPBACK
		WSPort *pport=(WSPort*)pmod->UsrPort[i].Sock;
		if((pport)&&(pport->lbaConnReset))
		{
			_WSHCloseUsrPort(pmod,i); 
			pmod->UsrPort[i].recvThread=0;
			UnlockPort(pmod,WS_USR,i,false);
			_WSHWaitUsrThreadExit(pmod,i);
			continue;
		}
		#endif

		// Bounce when the peer is receiving too slowly
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
					_WSHCloseUsrPort(pmod,i);
					pmod->UsrPort[i].recvThread=0;
					UnlockPort(pmod,WS_USR,i,false);
					_WSHWaitUsrThreadExit(pmod,i);
					continue;
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
		pmod->UsrPort[i].recvThread=0;
		UnlockPort(pmod,WS_USR,i,false);

		if(LockPort(pmod,WS_USR,i,true,500)==WAIT_OBJECT_0)
		{
			if(pmod->UsrPort[i].Sock)
			{
				pmod->UsrPort[i].sendThread=tid;
				//pmod->UsrPort[i].SinceLastBeatCount++;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->UsrPort[i].Sock)->sd,&pmod->UsrPort[i].pendOvlSendBeg,&pmod->UsrPort[i].pendOvlSendEnd);
				pmod->WSBeforeUsrSend(i);
				WSHUsrSend(pmod,i,true);
				pmod->UsrPort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_USR,i,true);
		}
	}

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if(!pmod->UgrPort[i].SockActive)
			continue;
		if(LockPort(pmod,WS_UGR,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->UgrPort[i].SockActive)
		{
			UnlockPort(pmod,WS_UGR,i,false);
			continue;
		}
		pmod->UgrPort[i].recvThread=tid;
		// Give the app one last shot to read the rest of the data
		if(pmod->UgrPort[i].peerClosed)
		{
			while(pmod->WSUgrMsgReady(i))
				pmod->WSUgrStripMsg(i);
			_WSHCloseUgrPort(pmod,i);				
			pmod->UgrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UGR,i,false);
			_WSHWaitUgrThreadExit(pmod,i);
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->UgrPort[i].appClosed)
		{
			LockPort(pmod,WS_UGR,i,true);
			pmod->UgrPort[i].sendThread=tid;
			WSHUgrSend(pmod,i,true);
			pmod->UgrPort[i].sendThread=0;
			UnlockPort(pmod,WS_UGR,i,true);

			_WSHCloseUgrPort(pmod,i); 
			pmod->UgrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UGR,i,false);
			_WSHWaitUgrThreadExit(pmod,i);
			continue;
		}

		// Bounce when the peer is receiving too slowly
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
					_WSHCloseUgrPort(pmod,i);
					pmod->UgrPort[i].recvThread=0;
					UnlockPort(pmod,WS_UGR,i,false);
					_WSHWaitUgrThreadExit(pmod,i);
					continue;
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
		pmod->UgrPort[i].recvThread=0;
		UnlockPort(pmod,WS_UGR,i,false);

		if(LockPort(pmod,WS_UGR,i,true,50)==WAIT_OBJECT_0)
		{
			if(pmod->UgrPort[i].SockActive)
			{
				pmod->UgrPort[i].sendThread=tid;
				//pmod->UgrPort[i].SinceLastBeatCount++;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->UgrPort[i].Sock)->sd,&pmod->UgrPort[i].pendOvlSendBeg,&pmod->UgrPort[i].pendOvlSendEnd);
				_WSHBeforeUgrSend(pmod,i);
				WSHUgrSend(pmod,i,true);
				pmod->UgrPort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_UGR,i,true);
		}
	}
	#endif //#ifdef WS_GUARANTEED

	#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	{
		if(!pmod->UmrPort[i].SockActive)
			continue;
		if(LockPort(pmod,WS_UMR,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->UmrPort[i].SockActive)
		{
			UnlockPort(pmod,WS_UMR,i,false);
			continue;
		}
		pmod->UmrPort[i].recvThread=tid;
		// Give the app one last shot to read the rest of the data
		if(pmod->UmrPort[i].peerClosed)
		{
			while(pmod->WSUmrMsgReady(i))
				pmod->WSUmrStripMsg(i);
			_WSHCloseUmrPort(pmod,i);
			pmod->UmrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UMR,i,false);
			_WSHWaitUmrThreadExit(pmod,i);
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->UmrPort[i].appClosed)
		{
			LockPort(pmod,WS_UMR,i,true);
			pmod->UmrPort[i].sendThread=tid;
			WSHUmrSend(pmod,i,true);
			pmod->UmrPort[i].sendThread=0;
			UnlockPort(pmod,WS_UMR,i,true);

			_WSHCloseUmrPort(pmod,i); 
			pmod->UmrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UMR,i,false);
			_WSHWaitUmrThreadExit(pmod,i);
			continue;
		}
		#ifdef WS_LOOPBACK
		WSPort *pport=(WSPort*)pmod->UmrPort[i].Sock;
		if((pport)&&(pport->lbaConnReset))
		{
			_WSHCloseUmrPort(pmod,i); 
			pmod->UmrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UMR,i,false);
			_WSHWaitUmrThreadExit(pmod,i);
			continue;
		}
		#endif

		// Bounce when the peer is receiving too slowly
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
					_WSHCloseUmrPort(pmod,i);
					pmod->UmrPort[i].recvThread=0;
					UnlockPort(pmod,WS_UMR,i,false);
					_WSHWaitUmrThreadExit(pmod,i);
					continue;
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
		pmod->UmrPort[i].recvThread=0;
		UnlockPort(pmod,WS_UMR,i,false);

		if(LockPort(pmod,WS_UMR,i,true,50)==WAIT_OBJECT_0)
		{
			if(pmod->UmrPort[i].SockActive)
			{
				pmod->UmrPort[i].sendThread=tid;
				//pmod->UmrPort[i].SinceLastBeatCount++;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->UmrPort[i].Sock)->sd,&pmod->UmrPort[i].pendOvlSendBeg,&pmod->UmrPort[i].pendOvlSendEnd);
				pmod->WSBeforeUmrSend(i);
				WSHUmrSend(pmod,i,true);
				pmod->UmrPort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_UMR,i,true);
		}
	}
	#endif //#ifdef WS_MONITOR

	for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		if((!pmod->ConPort[i].InUse)||(!pmod->ConPort[i].SockConnected))
			continue;
		// Lock the port
		if(LockPort(pmod,WS_CON,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->ConPort[i].SockConnected)
		{
			UnlockPort(pmod,WS_CON,i,false);
			continue;
		}
		pmod->ConPort[i].recvThread=tid;
		//printf("DEBUG CON%d active %d\n",i,tid);
		// Bounce when not enough data when there should be
		if(pmod->ConPort[i].Bounce)
		{
			DWORD tnow=GetTickCount();
			if((pmod->ConPort[i].LastDataTime)&&(tnow -pmod->ConPort[i].LastDataTime>pmod->ConPort[i].Bounce))
			{
				int hhmm=WSHTime/100;
				if(((!pmod->ConPort[i].BounceStart)||(hhmm>=pmod->ConPort[i].BounceStart))&&
				   ((!pmod->ConPort[i].BounceEnd)||(hhmm<pmod->ConPort[i].BounceEnd)))
				{
					WSHLogError(pmod,"!WSOCKS: ConPort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->ConPort[i].LastDataTime));
					_WSHCloseConPort(pmod,i);
					pmod->ConPort[i].recvThread=0;
					UnlockPort(pmod,WS_CON,i,false);
					_WSHWaitConThreadExit(pmod,i);
					continue;
				}
			}
		}
		pmod->ConPort[i].recvThread=0;
		UnlockPort(pmod,WS_CON,i,false);

		if(LockPort(pmod,WS_CON,i,true,500)==WAIT_OBJECT_0)
		{
			if(pmod->ConPort[i].SockConnected)
			{
				pmod->ConPort[i].sendThread=tid;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->ConPort[i].Sock)->sd,&pmod->ConPort[i].pendOvlSendBeg,&pmod->ConPort[i].pendOvlSendEnd);
				pmod->WSBeforeConSend(i);
				WSHConSend(pmod,i,true);
				pmod->ConPort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_CON,i,true);
		}
	}

	#ifdef WS_FILE_SERVER
    for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
    {
		if((!pmod->FilePort[i].InUse)||(!pmod->FilePort[i].SockConnected))
			continue;
		if(LockPort(pmod,WS_FIL,i,false,50)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->FilePort[i].SockConnected)
		{
			UnlockPort(pmod,WS_FIL,i,false);
			continue;
		}
		pmod->FilePort[i].recvThread=tid;
		// Bounce when not enough data when there should be
		if(pmod->FilePort[i].Bounce)
		{
			DWORD tnow=GetTickCount();
			if((pmod->FilePort[i].LastDataTime)&&(tnow -pmod->FilePort[i].LastDataTime>pmod->FilePort[i].Bounce))
			{
				int hhmm=WSHTime/100;
				if(((!pmod->FilePort[i].BounceStart)||(hhmm>=pmod->FilePort[i].BounceStart))&&
				   ((!pmod->FilePort[i].BounceEnd)||(hhmm<pmod->FilePort[i].BounceEnd)))
				{
					WSHLogError(pmod,"!WSOCKS: FilePort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->FilePort[i].LastDataTime));
					_WSHCloseFilePort(pmod,i);
					pmod->FilePort[i].recvThread=0;
					UnlockPort(pmod,WS_FIL,i,false);
					_WSHWaitFileThreadExit(pmod,i);
					continue;
				}
			}
		}
		pmod->FilePort[i].recvThread=0;
		UnlockPort(pmod,WS_FIL,i,false);

		if(LockPort(pmod,WS_FIL,i,true,50)==WAIT_OBJECT_0)
		{
			if(pmod->FilePort[i].SockConnected)
			{
				pmod->FilePort[i].sendThread=tid;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->FilePort[i].Sock)->sd,&pmod->FilePort[i].pendOvlSendBeg,&pmod->FilePort[i].pendOvlSendEnd);
				pmod->WSBeforeFileSend(i);
				WSHFileSend(pmod,i,true);
				pmod->FilePort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_FIL,i,true);
		}
    }
	#endif //#ifdef WS_FILE_SERVER

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if((!pmod->CgdPort[i].InUse)||(!pmod->CgdPort[i].SockConnected))
			continue;
		if(LockPort(pmod,WS_CGD,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->CgdPort[i].SockConnected)
		{
			UnlockPort(pmod,WS_CGD,i,false);
			continue;
		}
		pmod->CgdPort[i].recvThread=tid;
		// Bounce when not enough data when there should be
		if(pmod->CgdPort[i].Bounce)
		{
			DWORD tnow=GetTickCount();
			if((pmod->CgdPort[i].LastDataTime)&&(tnow -pmod->CgdPort[i].LastDataTime>pmod->CgdPort[i].Bounce))
			{
				int hhmm=WSHTime/100;
				if(((!pmod->CgdPort[i].BounceStart)||(hhmm>=pmod->CgdPort[i].BounceStart))&&
				   ((!pmod->CgdPort[i].BounceEnd)||(hhmm<pmod->CgdPort[i].BounceEnd)))
				{
					WSHLogError(pmod,"!WSOCKS: CgdPort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->CgdPort[i].LastDataTime));
					_WSHCloseCgdPort(pmod,i);
					pmod->CgdPort[i].recvThread=0;
					UnlockPort(pmod,WS_CGD,i,false);
					_WSHWaitCgdThreadExit(pmod,i);
					continue;
				}
			}
		}
		pmod->CgdPort[i].recvThread=0;
		UnlockPort(pmod,WS_CGD,i,false);

		if(LockPort(pmod,WS_CGD,i,true,500)==WAIT_OBJECT_0)
		{
			if(pmod->CgdPort[i].SockConnected)
			{
				pmod->CgdPort[i].sendThread=tid;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->CgdPort[i].Sock)->sd,&pmod->CgdPort[i].pendOvlSendBeg,&pmod->CgdPort[i].pendOvlSendEnd);
				_WSHBeforeCgdSend(pmod,i);
				WSHCgdSend(pmod,i,true);
				pmod->CgdPort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_CGD,i,true);
		}
	}
	#endif //#ifdef WS_GUARANTEED

	for(i=0;i<pmod->NO_OF_CTO_PORTS;i++)
	{
		if((!pmod->CtoPort[i].InUse)||(!pmod->CtoPort[i].SockActive))
			continue;
		if(LockPort(pmod,WS_CTO,i,true,50)==WAIT_OBJECT_0)
		{
			if(pmod->CtoPort[i].SockActive)
			{
				pmod->CtoPort[i].sendThread=tid;
				WSHFinishOverlapSend(pmod,((WSPort*)pmod->CtoPort[i].Sock)->sd,&pmod->CtoPort[i].pendOvlSendBeg,&pmod->CtoPort[i].pendOvlSendEnd);
				//pmod->WSBeforeCtoSend(i);
				pmod->CtoPort[i].sendThread=0;
			}
			UnlockPort(pmod,WS_CTO,i,true);
		}
	}

	for(i=0;i<pmod->NO_OF_CTI_PORTS;i++)
	{
		if((!pmod->CtiPort[i].InUse)||(!pmod->CtiPort[i].SockActive))
			continue;
		// Lock the port
		if(LockPort(pmod,WS_CTI,i,false,500)!=WAIT_OBJECT_0)
			continue;
		if(!pmod->CtiPort[i].SockActive)
		{
			UnlockPort(pmod,WS_CTI,i,false);
			continue;
		}
		pmod->CtiPort[i].recvThread=tid;

		// Bounce when not enough data when there should be
		if(pmod->CtiPort[i].Bounce)
		{
			DWORD tnow=GetTickCount();
			if((pmod->CtiPort[i].LastDataTime)&&(tnow -pmod->CtiPort[i].LastDataTime>pmod->CtiPort[i].Bounce))
			{
				int hhmm=WSHTime/100;
				if(((!pmod->CtiPort[i].BounceStart)||(hhmm>=pmod->CtiPort[i].BounceStart))&&
				   ((!pmod->CtiPort[i].BounceEnd)||(hhmm<pmod->CtiPort[i].BounceEnd)))
				{
					WSHLogError(pmod,"!WSOCKS: CtiPort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->CtiPort[i].LastDataTime));
					_WSHCloseCtiPort(pmod,i);
					pmod->CtiPort[i].recvThread=0;
					UnlockPort(pmod,WS_CTI,i,false);
					_WSHWaitCtiThreadExit(pmod,i);
					continue;
				}
			}
		}
		pmod->CtiPort[i].recvThread=0;
		UnlockPort(pmod,WS_CTI,i,false);
	}
	return 0;
}

// Many app threads can be in WSHAsyncLoop at once
int WsocksHostImpl::WSHAsyncLoop(WsocksApp *pmod)
{
	int tid=GetCurrentThreadId();
	int i;
	for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		if(!pmod->ConPort[i].SockConnected)
			continue;
		if(LockPort(pmod,WS_CON,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->ConPort[i].SockConnected)
		{
			pmod->ConPort[i].recvThread=tid;
			if(pmod->ConPort[i].InBuffer.Size>0)
			{
				while(pmod->WSConMsgReady(i))
					pmod->WSConStripMsg(i);
			}
			pmod->ConPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_CON,i,false);
	}

	for(i=0;i<pmod->NO_OF_USR_PORTS;i++)
	{
		if(!pmod->UsrPort[i].Sock) // Account for TimeTillClose
			continue;
		if(LockPort(pmod,WS_USR,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->UsrPort[i].Sock)
		{
			pmod->UsrPort[i].recvThread=tid;
			if(pmod->UsrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUsrMsgReady(i))
					pmod->WSUsrStripMsg(i);
			}
			pmod->UsrPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_USR,i,false);
	}

	for(i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	{
		if(!pmod->UmrPort[i].SockActive)
			continue;
		if(LockPort(pmod,WS_UMR,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->UmrPort[i].SockActive)
		{
			pmod->UmrPort[i].recvThread=tid;
			if(pmod->UmrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUmrMsgReady(i))
					pmod->WSUmrStripMsg(i);
			}
			pmod->UmrPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_UMR,i,false);
	}

	#ifdef WS_FILE_SERVER
	for(i=0;i<pmod->NO_OF_FILE_PORTS;i++)
	{
		if(!pmod->FilePort[i].SockConnected)
			continue;
		if(LockPort(pmod,WS_FIL,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->FilePort[i].SockConnected)
		{
			pmod->FilePort[i].recvThread=tid;
			if(pmod->FilePort[i].InBuffer.Size>0)
			{
				while(pmod->WSFileMsgReady(i))
					pmod->WSFileStripMsg(i);
			}
			pmod->FilePort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_FIL,i,false);
	}
	#endif

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if(!pmod->CgdPort[i].SockConnected)
			continue;
		if(LockPort(pmod,WS_CGD,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->CgdPort[i].SockConnected)
		{
			pmod->CgdPort[i].recvThread=tid;
			if(pmod->CgdPort[i].InBuffer.Size>0)
			{
				while(pmod->WSCgdMsgReady(i))
					pmod->WSCgdStripMsg(i);
			}
			pmod->CgdPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_CGD,i,false);
	}

	for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if(!pmod->UgrPort[i].SockActive)
			continue;
		if(LockPort(pmod,WS_UGR,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->UgrPort[i].SockActive)
		{
			pmod->UgrPort[i].recvThread=tid;
			if(pmod->UgrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUgrMsgReady(i))
					pmod->WSUgrStripMsg(i);
			}
			pmod->UgrPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_UGR,i,false);
	}
	#endif

	for(i=0;i<pmod->NO_OF_CTI_PORTS;i++)
	{
		if(!pmod->CtiPort[i].SockActive)
			continue;
		if(LockPort(pmod,WS_CTI,i,false,200)!=WAIT_OBJECT_0)
			continue;
		if(pmod->CtiPort[i].SockActive)
		{
			pmod->CtiPort[i].recvThread=tid;
			if(pmod->CtiPort[i].InBuffer.Size>0)
			{
				while(pmod->WSCtiMsgReady(i))
					pmod->WSCtiStripMsg(i);
			}
			pmod->CtiPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_CTI,i,false);
	}
	return 0;
}

#endif//WS_OTPP
