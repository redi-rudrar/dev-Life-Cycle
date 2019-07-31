
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

#ifdef WS_OIO

// Windows overlapped I/O using completion ports lowers latency and CPU.
void WsocksHostImpl::CancelOverlap(WsocksApp *pmod, WSOVERLAPPED *povl)
{
	#ifdef OVLMUX_CRIT_SECTION
	EnterCriticalSection(&ovlMutex);
	#else
	WaitForSingleObject(ovlMutex,INFINITE);
	#endif
	delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
	_ASSERT(pmod->cxlOvlList);
	FreeOverlap(&pmod->cxlOvlList,povl);
	#ifdef OVLMUX_CRIT_SECTION
	LeaveCriticalSection(&ovlMutex);
	#else
	ReleaseMutex(ovlMutex);
	#endif
}
int WsocksHostImpl::WSHIocpLoop(WsocksApp *pmod, DWORD timeout)
{
	int tid=GetCurrentThreadId();
    DWORD tstart=GetTickCount();
	// Delayed processing allows us to buffer data (up to a limit) as soon as it's available from the NIC
//#define CON_HOLD_LIMIT (400*1024*1024)
//#define USR_HOLD_LIMIT (10*1024*1024)
//	set<DWORD> readyPorts;
//	bool holdLimit=false;
	while(pmod->initialized>0)
//	while((pmod->initialized)&&(!holdLimit))
	{
		DWORD bytes=0;
		ULONG ckey=0;
		WSOVERLAPPED *povl=0;
		int rc=GetQueuedCompletionStatus(pmod->hIOPort,&bytes,&ckey,(LPOVERLAPPED*)&povl,timeout);
		WSPort *pport=(WSPort *)ckey;
		if((!pport)||(!povl))
		{
			_ASSERT(!povl);
			//if(!readyPorts.empty())
			//	break;
			return -1;
		}
		_ASSERT(povl);
		int PortNo=pport->PortNo;
		//#ifdef _DEBUG
		//{
		//	char dbgmsg[1024]={0};
		//	sprintf(dbgmsg,"%06d: DEBUG WSHIocpLoop(%d,%08x)=%d,%d,%d,%d\r\n",
		//		WSTime,PortNo,povl,rc,WSAGetLastError(),bytes,povl->bytes);
		//	OutputDebugString(dbgmsg);
		//}
		//#endif

		// Bytes transferred
		if(bytes>0)
		{
			// Overlapped WSARecv completion
			if(povl->RecvOp)
			{
				// Handle the message, then issue next overlapped recv
				switch(pport->PortType)
				{
				case WS_CON: 
				{
					//WSLogEvent("CON%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_CON,PortNo,false);
					pmod->ConPort[PortNo].recvThread=tid;
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->ConPort[PortNo].SockConnected)
					{
						if(WSHConRead(pmod,PortNo,povl->buf,bytes))
						{
							while(pmod->WSConMsgReady(PortNo))
								pmod->WSConStripMsg(PortNo);
							//readyPorts.insert(MAKELONG(PortNo,pport->PortType));
							//if(pmod->ConPort[PortNo].InBuffer.Size>=CON_HOLD_LIMIT)
							//	holdLimit=true;
						}
						WSHConIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->ConPort[PortNo].recvThread);
					pmod->ConPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_CON,PortNo,false);
					break;
				}
				case WS_USR: 
				{
					//WSLogEvent("USR%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_USR,PortNo,false);
					pmod->UsrPort[PortNo].recvThread=tid;					
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->UsrPort[PortNo].Sock) // Account for TimeTillClose
					{
						if(WSHUsrRead(pmod,PortNo,povl->buf,bytes))
						{
							while((pmod->UsrPort[PortNo].SockActive)&&(pmod->WSUsrMsgReady(PortNo)))
								pmod->WSUsrStripMsg(PortNo);
							//readyPorts.insert(MAKELONG(PortNo,pport->PortType));
							//if(pmod->UsrPort[PortNo].InBuffer.Size>=USR_HOLD_LIMIT)
							//	holdLimit=true;
						}
						WSHUsrIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->UsrPort[PortNo].recvThread);
					pmod->UsrPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_USR,PortNo,false);
					break;
				}
				case WS_UMR: 
				{
					//WSLogEvent("UMR%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_UMR,PortNo,false);
					pmod->UmrPort[PortNo].recvThread=tid;
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->UmrPort[PortNo].SockActive)
					{
						if(WSHUmrRead(pmod,PortNo,povl->buf,bytes))
						{
							while(pmod->WSUmrMsgReady(PortNo))
								pmod->WSUmrStripMsg(PortNo);
						}
						WSHUmrIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->UmrPort[PortNo].recvThread);
					pmod->UmrPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_UMR,PortNo,false);
					break;
				}
			#ifdef WS_FILE_SERVER
				case WS_FIL: 
				{
					//WSLogEvent("FIL%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_FIL,PortNo,false);
					pmod->FilePort[PortNo].recvThread=tid;
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->FilePort[PortNo].SockConnected)
					{
						if(WSHFileRead(pmod,PortNo,povl->buf,bytes))
						{
							while(pmod->WSFileMsgReady(PortNo))
								pmod->WSFileStripMsg(PortNo);
						}
						WSHFileIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->FilePort[PortNo].recvThread);
					pmod->FilePort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_FIL,PortNo,false);
					break;
				}
			#endif
			#ifdef WS_GUARANTEED
				case WS_CGD: 
				{
					//WSLogEvent("CGD%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_CGD,PortNo,false);
					pmod->CgdPort[PortNo].recvThread=tid;
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->CgdPort[PortNo].SockConnected)
					{
					#ifdef WS_COMPRESS
						if(pmod->CgdPort[PortNo].Compressed&&(!((pmod->CgdPort[PortNo].S5Connect)&&(pmod->CgdPort[PortNo].S5Status<100))))
							WSWriteBuff(&pmod->CgdPort[PortNo].InCompBuffer,povl->buf,bytes);
						else
					#endif
						{
							WSHRecord(pmod,&pmod->CgdPort[PortNo].Recording,povl->buf,bytes,false);
							WSWriteBuff(&pmod->CgdPort[PortNo].InBuffer,povl->buf,bytes);
						}
						pmod->CgdPort[PortNo].LastDataTime=GetTickCount();
						pmod->CgdPort[PortNo].BytesIn+=bytes;
						pmod->CgdPort[PortNo].BlocksIn++;
						if(WSHCgdRead(pmod,PortNo,povl->buf,bytes))
						{
							while(pmod->WSCgdMsgReady(PortNo))
								pmod->WSCgdStripMsg(PortNo);
						}
						WSHCgdIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->CgdPort[PortNo].recvThread);
					pmod->CgdPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_CGD,PortNo,false);
					break;
				}
				case WS_UGR: 
				{
					//WSLogEvent("UGR%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_UGR,PortNo,false);
					pmod->UgrPort[PortNo].recvThread=tid;
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->UgrPort[PortNo].SockActive)
					{
					#ifdef WS_COMPRESS
						if(pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].Compressed)
							WSWriteBuff(&pmod->UgrPort[PortNo].InCompBuffer,povl->buf,bytes);
						else
					#endif
						{
							WSHRecord(pmod,&pmod->UgrPort[PortNo].Recording,povl->buf,bytes,false);
							WSWriteBuff(&pmod->UgrPort[PortNo].InBuffer,povl->buf,bytes);
						}
						//pmod->UgrPort[PortNo].LastDataTime=GetTickCount();
						pmod->UgrPort[PortNo].BytesIn+=bytes;
						pmod->UgrPort[PortNo].BlocksIn++;
						if(WSHUgrRead(pmod,PortNo,povl->buf,bytes))
						{
							while(pmod->WSUgrMsgReady(PortNo))
								pmod->WSUgrStripMsg(PortNo);
						}
						WSHUgrIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->UgrPort[PortNo].recvThread);
					pmod->UgrPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_UGR,PortNo,false);
					break;
				}
			#endif
				case WS_CTI: 
				{
					//WSLogEvent("CTI%d: Recv %d bytes",PortNo,bytes);
					LockPort(pmod,WS_CTI,PortNo,false);
					pmod->CtiPort[PortNo].recvThread=tid;
					povl->Pending=false;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else if(pmod->CtiPort[PortNo].SockActive)
					{
						if(WSHCtiRead(pmod,PortNo,povl->buf,bytes))
						{
							while(pmod->WSCtiMsgReady(PortNo))
								pmod->WSCtiStripMsg(PortNo);
						}								
						WSHCtiIocpBegin(pmod,PortNo,povl);
					}
					_ASSERT(pmod->CtiPort[PortNo].recvThread);
					pmod->CtiPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_CTI,PortNo,false);
					break;
				}
				};
			}
			// Overlapped WSASend completion
			else
			{
				_ASSERT(bytes>0);
				switch(pport->PortType)
				{
				case WS_CON:
				{
					//WSLogEvent("CON%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_CON,PortNo,true);
					pmod->ConPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->ConPort[PortNo].pendOvlSendList,povl);
					}
					pmod->ConPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_CON,PortNo,true);
					break;
				}
				case WS_USR:
				{
					//WSLogEvent("USR%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_USR,PortNo,true);
					pmod->UsrPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UsrPort[PortNo].pendOvlSendList,povl);
					}
					pmod->UsrPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_USR,PortNo,true);
					break;
				}
				case WS_UMR:
				{
					//WSLogEvent("UMR%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_UMR,PortNo,true);
					pmod->UmrPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UmrPort[PortNo].pendOvlSendList,povl);
					}
					pmod->UmrPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_UMR,PortNo,true);
					break;
				}
			#ifdef WS_FILE_SERVER
				case WS_FIL:
				{
					//WSLogEvent("FILE%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_FIL,PortNo,true);
					pmod->FilePort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->FilePort[PortNo].pendOvlSendList,povl);
					}
					pmod->FilePort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_FIL,PortNo,true);
					break;
				}
			#endif
			#ifdef WS_GUARANTEED
				case WS_CGD:
				{
					//WSLogEvent("CGD%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_CGD,PortNo,true);
					pmod->CgdPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->CgdPort[PortNo].pendOvlSendList,povl);
					}
					pmod->CgdPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_CGD,PortNo,true);
					break;
				}
				case WS_UGR:
				{
					//WSLogEvent("UGR%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_UGR,PortNo,true);
					pmod->UgrPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UgrPort[PortNo].pendOvlSendList,povl);
					}
					pmod->UgrPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_UGR,PortNo,true);
					break;
				}
			#endif
				case WS_CTO:
				{
					//WSLogEvent("CTO%d: Sent %d bytes at %d",PortNo,bytes,(DWORD)WSGetTimerCount());
					LockPort(pmod,WS_CTO,PortNo,true);
					pmod->CtoPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->CtoPort[PortNo].pendOvlSendList,povl);
					}
					pmod->CtoPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_CTO,PortNo,true);
					break;
				}
				};
			}
		}
		// Overlapped WSASend or WSARecv cancellations from close by peer
		else
		{
			switch(pport->PortType)
			{
			case WS_CON: 
			{
				if(povl->RecvOp)
				{
					LockPort(pmod,WS_CON,PortNo,false);
					pmod->ConPort[PortNo].recvThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->ConPort[PortNo].SockConnected)
							pmod->ConPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->ConPort[PortNo].pendOvlRecvList,povl);
					}
					pmod->ConPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_CON,PortNo,false);
				}
				else
				{
					LockPort(pmod,WS_CON,PortNo,true);
					pmod->ConPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->ConPort[PortNo].SockConnected)
							pmod->ConPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->ConPort[PortNo].pendOvlSendList,povl);
					}
					pmod->ConPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_CON,PortNo,true);
				}
				break;
			}
			case WS_USR: 
			{
				if(povl->RecvOp)
				{
					LockPort(pmod,WS_USR,PortNo,false);
					pmod->UsrPort[PortNo].recvThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->UsrPort[PortNo].SockActive)
							pmod->UsrPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UsrPort[PortNo].pendOvlRecvList,povl);
					}
					pmod->UsrPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_USR,PortNo,false);
				}
				else
				{
					LockPort(pmod,WS_USR,PortNo,true);
					pmod->UsrPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->UsrPort[PortNo].SockActive)
							pmod->UsrPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UsrPort[PortNo].pendOvlSendList,povl);
					}
					pmod->UsrPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_USR,PortNo,true);
				}
				break;
			}
			case WS_UMR: 
			{
				if(povl->RecvOp)
				{
					LockPort(pmod,WS_UMR,PortNo,false);
					pmod->UmrPort[PortNo].recvThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->UmrPort[PortNo].SockActive)
							pmod->UmrPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UmrPort[PortNo].pendOvlRecvList,povl);
					}
					pmod->UmrPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_UMR,PortNo,false);
				}
				else
				{
					LockPort(pmod,WS_UMR,PortNo,true);
					pmod->UmrPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->UmrPort[PortNo].SockActive)
							pmod->UmrPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UmrPort[PortNo].pendOvlSendList,povl);
					}
					pmod->UmrPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_UMR,PortNo,true);
				}
				break;
			}
		#ifdef WS_FILE_SERVER
			case WS_FIL: 
			{
				if(povl->RecvOp)
				{
					LockPort(pmod,WS_FIL,PortNo,false);
					pmod->FilePort[PortNo].recvThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->FilePort[PortNo].SockConnected)
							pmod->FilePort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->FilePort[PortNo].pendOvlRecvList,povl);
					}
					pmod->FilePort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_FIL,PortNo,false);
				}
				else
				{
					LockPort(pmod,WS_FIL,PortNo,true);
					pmod->FilePort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->FilePort[PortNo].SockConnected)
							pmod->FilePort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->FilePort[PortNo].pendOvlSendList,povl);
					}
					pmod->FilePort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_FIL,PortNo,true);
				}
				break;
			}
		#endif
		#ifdef WS_GUARANTEED
			case WS_CGD: 
			{
				if(povl->RecvOp)
				{
					LockPort(pmod,WS_CGD,PortNo,false);
					pmod->CgdPort[PortNo].recvThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->CgdPort[PortNo].SockConnected)
							pmod->CgdPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->CgdPort[PortNo].pendOvlRecvList,povl);
					}
					pmod->CgdPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_CGD,PortNo,false);
				}
				else
				{
					LockPort(pmod,WS_CGD,PortNo,true);
					pmod->CgdPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->CgdPort[PortNo].SockConnected)
							pmod->CgdPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->CgdPort[PortNo].pendOvlSendList,povl);
					}
					pmod->CgdPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_CGD,PortNo,true);
				}
				break;
			}
			case WS_UGR: 
			{
				if(povl->RecvOp)
				{
					LockPort(pmod,WS_UGR,PortNo,false);
					pmod->UgrPort[PortNo].recvThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->UgrPort[PortNo].SockActive)
							pmod->UgrPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UgrPort[PortNo].pendOvlRecvList,povl);
					}
					pmod->UgrPort[PortNo].recvThread=0;
					UnlockPort(pmod,WS_UGR,PortNo,false);
				}
				else
				{
					LockPort(pmod,WS_UGR,PortNo,true);
					pmod->UgrPort[PortNo].sendThread=tid;
					if(povl->Cancelled)
						CancelOverlap(pmod,povl);
					else 
					{
						if(pmod->UgrPort[PortNo].SockActive)
							pmod->UgrPort[PortNo].peerClosed=true;
						delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
						FreeOverlap(&pmod->UgrPort[PortNo].pendOvlSendList,povl);
					}
					pmod->UgrPort[PortNo].sendThread=0;
					UnlockPort(pmod,WS_UGR,PortNo,true);
				}
				break;
			}
		#endif
			case WS_CTO: 
			{
				_ASSERT(!pmod->CtoPort[PortNo].SockActive);
				LockPort(pmod,WS_CTO,PortNo,true);
				pmod->CtoPort[PortNo].sendThread=tid;
				if(povl->Cancelled)
					CancelOverlap(pmod,povl);
				else 
				{
					delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
					FreeOverlap(&pmod->CtoPort[PortNo].pendOvlSendList,povl);
				}
				pmod->CtoPort[PortNo].sendThread=0;
				UnlockPort(pmod,WS_CTO,PortNo,true);
				break;
			}
			case WS_CTI: 
			{
				_ASSERT(!pmod->CtiPort[PortNo].SockActive);
				LockPort(pmod,WS_CTI,PortNo,false);
				pmod->CtiPort[PortNo].recvThread=tid;
				if(povl->Cancelled)
					CancelOverlap(pmod,povl);
				else 
				{
					delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
					FreeOverlap(&pmod->CtiPort[PortNo].pendOvlRecvList,povl);
				}
				pmod->CtiPort[PortNo].recvThread=0;
				UnlockPort(pmod,WS_CTI,PortNo,false);
				break;
			}
			};
		}
		//if(holdLimit)
		//	break;
		// Take a breath so WSHSyncLoop can run at least once per second
		DWORD tnow=GetTickCount();
		if(tnow -tstart>=200)
			break;
	}

	//for(set<DWORD>::iterator rit=readyPorts.begin();rit!=readyPorts.end();rit++)
	//{
	//	int PortType=HIWORD(*rit);
	//	int PortNo=LOWORD(*rit);
	//	switch(PortType)
	//	{
	//	case WS_CON:
	//	{
	//		LockPort(pmod,WS_CON,PortNo,false);
	//		pmod->ConPort[PortNo].recvThread=tid;
	//		while(pmod->WSConMsgReady(PortNo))
	//			pmod->WSConStripMsg(PortNo);
	//		_ASSERT(pmod->ConPort[PortNo].recvThread);
	//		pmod->ConPort[PortNo].recvThread=0;
	//		UnlockPort(pmod,WS_CON,PortNo,false);
	//		break;
	//	}
	//	case WS_USR:
	//	{
	//		LockPort(pmod,WS_USR,PortNo,false);
	//		pmod->UsrPort[PortNo].recvThread=tid;
	//		while(pmod->WSUsrMsgReady(PortNo))
	//			pmod->WSUsrStripMsg(PortNo);
	//		_ASSERT(pmod->UsrPort[PortNo].recvThread);
	//		pmod->UsrPort[PortNo].recvThread=0;
	//		UnlockPort(pmod,WS_USR,PortNo,false);
	//		break;
	//	}
	//	};
	//}
	//readyPorts.clear();
	return 0;
}

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
		if(LockPort(pmod,WS_USC,i,false,0)!=WAIT_OBJECT_0)
			continue;
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
		if(LockPort(pmod,WS_UGC,i,false,0)!=WAIT_OBJECT_0)
			continue;
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
		if(LockPort(pmod,WS_UMC,i,false,0)!=WAIT_OBJECT_0)
			continue;
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
		LockPort(pmod,WS_CON,i,false);
		pmod->ConPort[i].recvThread=tid;
		//printf("DEBUG CON%d active %d\n",i,tid);

 		if((!pmod->ConPort[i].ConnectHold)&&(!pmod->ConPort[i].SockOpen))
			WSHOpenConPort(pmod,i);
		if((pmod->ConPort[i].ConnectHold)&&(pmod->ConPort[i].SockOpen))
		{
			_WSHCloseConPort(pmod,i);
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
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
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->ConPort[i].appClosed)
		{
			WSHConSend(pmod,i,true);
			_WSHCloseConPort(pmod,i); 
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
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
		LockPort(pmod,WS_CGD,i,false);
		pmod->CgdPort[i].recvThread=tid;
		if((!pmod->CgdPort[i].ConnectHold)&&(!pmod->CgdPort[i].SockOpen))
			WSHOpenCgdPort(pmod,i);
		if((pmod->CgdPort[i].ConnectHold)&&(pmod->CgdPort[i].SockOpen))
		{
			_WSHCloseCgdPort(pmod,i);
			pmod->CgdPort[i].recvThread=0;
			UnlockPort(pmod,WS_CGD,i,false);
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
			WSHCgdSend(pmod,i,true);
			_WSHCloseCgdPort(pmod,i); 
			pmod->CgdPort[i].recvThread=0;
			UnlockPort(pmod,WS_CGD,i,false);
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
		LockPort(pmod,WS_FIL,i,false);
		pmod->FilePort[i].recvThread=tid;
		// Connect file ports
		if((!pmod->FilePort[i].ConnectHold)&&(!pmod->FilePort[i].SockOpen))
			WSHOpenFilePort(pmod,i);
		if((pmod->FilePort[i].ConnectHold)&&(pmod->FilePort[i].SockOpen))
		{
			_WSHCloseFilePort(pmod,i);
			pmod->FilePort[i].recvThread=0;
			UnlockPort(pmod,WS_FIL,i,false);
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
			continue;
		}
		// Give the peer one last shot to read buffered output
		else if(pmod->FilePort[i].appClosed)
		{
			WSHFileSend(pmod,i,true);
			_WSHCloseFilePort(pmod,i); 
			pmod->FilePort[i].recvThread=0;
			UnlockPort(pmod,WS_FIL,i,false);
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
		if(LockPort(pmod,WS_CTO,i,true,0)!=WAIT_OBJECT_0)
			continue;
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
		LockPort(pmod,WS_CTI,i,false);
		pmod->CtiPort[i].recvThread=tid;
		if((!pmod->CtiPort[i].ConnectHold)&&(!pmod->CtiPort[i].SockActive))
			WSHOpenCtiPort(pmod,i);
		if((pmod->CtiPort[i].ConnectHold)&&(pmod->CtiPort[i].SockActive))
		{
			_WSHCloseCtiPort(pmod,i);
			pmod->CtiPort[i].recvThread=0;
			UnlockPort(pmod,WS_CTI,i,false);
			continue;
		}
		if(pmod->CtiPort[i].appClosed)
		{
			_WSHCloseCtiPort(pmod,i); 
			pmod->CtiPort[i].recvThread=0;
			UnlockPort(pmod,WS_CTI,i,false);
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
		LockPort(pmod,WS_USR,i,false);
		{
			pmod->UsrPort[i].recvThread=tid;
			// Give the app one last shot to read the rest of the data
			if(pmod->UsrPort[i].peerClosed)
			{
				while(pmod->WSUsrMsgReady(i))
					pmod->WSUsrStripMsg(i);
				_WSHCloseUsrPort(pmod,i);
				pmod->UsrPort[i].recvThread=0;
				UnlockPort(pmod,WS_USR,i,false);
				continue;
			}
			// Give the peer one last shot to read buffered output
			else if(pmod->UsrPort[i].appClosed)
			{
				WSHUsrSend(pmod,i,true);
				_WSHCloseUsrPort(pmod,i); 
				UnlockPort(pmod,WS_USR,i,false);
				continue;
			}
			#ifdef WS_LOOPBACK
			WSPort *pport=(WSPort*)pmod->UsrPort[i].Sock;
			if((pport)&&(pport->lbaConnReset))
			{
				_WSHCloseUsrPort(pmod,i); 
				UnlockPort(pmod,WS_USR,i,false);
				continue;
			}
			#endif

			// Bounce when the peer is receiving too slowly
			if(pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutSize)
			{
				if((pmod->UsrPort[i].SinceOpenTimer > pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut)
					&&(pmod->UsrPort[i].IOCPSendBytes > pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutSize))
				{
					if((!pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod)||
						((pmod->UsrPort[i].TimeOutStart)&&(time(0) -pmod->UsrPort[i].TimeOutStart>=pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod)))
					{
						if(pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod)
							WSHLogError(pmod,"!WSOCKS: UsrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld for %ld[%ld] Seconds after %ld[%ld] Seconds Since Login"
								,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].RemoteIP,pmod->UsrPort[i].IOCPSendBytes
								,time(0) -pmod->UsrPort[i].TimeOutStart,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod
								,pmod->UsrPort[i].SinceOpenTimer,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut);
						else
							WSHLogError(pmod,"!WSOCKS: UsrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
							,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].RemoteIP,pmod->UsrPort[i].IOCPSendBytes
								,pmod->UsrPort[i].SinceOpenTimer,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut);
						_WSHCloseUsrPort(pmod,i);
						UnlockPort(pmod,WS_USR,i,false);
						continue;
					}
					else if(!pmod->UsrPort[i].TimeOutStart)
					{
						pmod->UsrPort[i].TimeOutStart=time(0);
						WSHLogEvent(pmod,"!WSOCKS: UsrPort[%d][%s][%s] - Timer Started[%ld] - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
							,i,pmod->UsrPort[i].Note,pmod->UsrPort[i].RemoteIP,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOutPeriod,pmod->UsrPort[i].IOCPSendBytes
							,pmod->UsrPort[i].SinceOpenTimer,pmod->UscPort[pmod->UsrPort[i].UscPort].TimeOut);
					}
				}
				else
					pmod->UsrPort[i].TimeOutStart=0;
			}
			pmod->UsrPort[i].recvThread=0;
		}
		UnlockPort(pmod,WS_USR,i,false);

		if(LockPort(pmod,WS_USR,i,true,0)==WAIT_OBJECT_0)
		{
			pmod->UsrPort[i].sendThread=tid;
			//pmod->UsrPort[i].SinceLastBeatCount++;
			pmod->WSBeforeUsrSend(i);
			WSHUsrSend(pmod,i,true);
			pmod->UsrPort[i].sendThread=0;
			UnlockPort(pmod,WS_USR,i,true);
		}
	}

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if(!pmod->UgrPort[i].SockActive)
			continue;
		LockPort(pmod,WS_UGR,i,false);
		{
			pmod->UgrPort[i].recvThread=tid;
			// Give the app one last shot to read the rest of the data
			if(pmod->UgrPort[i].peerClosed)
			{
				while(pmod->WSUgrMsgReady(i))
					pmod->WSUgrStripMsg(i);
				_WSHCloseUgrPort(pmod,i);				
				pmod->UgrPort[i].recvThread=0;
				UnlockPort(pmod,WS_UGR,i,false);
				continue;
			}
			// Give the peer one last shot to read buffered output
			else if(pmod->UgrPort[i].appClosed)
			{
				WSHUgrSend(pmod,i,true);
				_WSHCloseUgrPort(pmod,i); 
				pmod->UgrPort[i].recvThread=0;
				UnlockPort(pmod,WS_UGR,i,false);
				continue;
			}

			// Bounce when the peer is receiving too slowly
			if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutSize)
			{
				if((pmod->UgrPort[i].SinceOpenTimer > pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut)
					&&(pmod->UgrPort[i].IOCPSendBytes > pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutSize))
				{
					if((!pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod)||
						((pmod->UgrPort[i].TimeOutStart)&&(time(0) -pmod->UgrPort[i].TimeOutStart>=pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod)))
					{
						if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod)
							WSHLogError(pmod,"!WSOCKS: UgrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld for %ld[%ld] Seconds after %ld[%ld] Seconds Since Login"
								,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].RemoteIP,pmod->UgrPort[i].IOCPSendBytes
								,time(0) -pmod->UgrPort[i].TimeOutStart,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod
								,pmod->UgrPort[i].SinceOpenTimer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut);
						else
							WSHLogError(pmod,"!WSOCKS: UgrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
							,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].RemoteIP,pmod->UgrPort[i].IOCPSendBytes
								,pmod->UgrPort[i].SinceOpenTimer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut);
						_WSHCloseUgrPort(pmod,i);
						pmod->UgrPort[i].recvThread=0;
						UnlockPort(pmod,WS_UGR,i,false);
						continue;
					}
					else if(!pmod->UgrPort[i].TimeOutStart)
					{
						pmod->UgrPort[i].TimeOutStart=time(0);
						WSHLogEvent(pmod,"!WSOCKS: UgrPort[%d][%s][%s] - Timer Started[%ld] - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
							,i,pmod->UgrPort[i].Note,pmod->UgrPort[i].RemoteIP,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOutPeriod,pmod->UgrPort[i].IOCPSendBytes
							,pmod->UgrPort[i].SinceOpenTimer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].TimeOut);
					}
				}
				else
					pmod->UgrPort[i].TimeOutStart=0;
			}
		}
		pmod->UgrPort[i].recvThread=0;
		UnlockPort(pmod,WS_UGR,i,false);

		if(LockPort(pmod,WS_UGR,i,true,0)==WAIT_OBJECT_0)
		{
			pmod->UgrPort[i].sendThread=tid;
			//pmod->UgrPort[i].SinceLastBeatCount++;
			_WSHBeforeUgrSend(pmod,i);
			WSHUgrSend(pmod,i,false);
			pmod->UgrPort[i].sendThread=0;
			UnlockPort(pmod,WS_UGR,i,true);
		}
	}
	#endif //#ifdef WS_GUARANTEED

	#ifdef WS_MONITOR
	for(i=0;i<pmod->NO_OF_UMR_PORTS;i++)
	{
		if(!pmod->UmrPort[i].SockActive)
			continue;
		LockPort(pmod,WS_UMR,i,false);
		{
			pmod->UmrPort[i].recvThread=tid;
			// Give the app one last shot to read the rest of the data
			if(pmod->UmrPort[i].peerClosed)
			{
				while(pmod->WSUmrMsgReady(i))
					pmod->WSUmrStripMsg(i);
				_WSHCloseUmrPort(pmod,i);
				pmod->UmrPort[i].recvThread=0;
				UnlockPort(pmod,WS_UMR,i,false);
				continue;
			}
			// Give the peer one last shot to read buffered output
			else if(pmod->UmrPort[i].appClosed)
			{
				WSHUmrSend(pmod,i,true);
				_WSHCloseUmrPort(pmod,i); 
				pmod->UmrPort[i].recvThread=0;
				UnlockPort(pmod,WS_UMR,i,false);
				continue;
			}
			#ifdef WS_LOOPBACK
			WSPort *pport=(WSPort*)pmod->UmrPort[i].Sock;
			if((pport)&&(pport->lbaConnReset))
			{
				_WSHCloseUmrPort(pmod,i); 
				pmod->UmrPort[i].recvThread=0;
				UnlockPort(pmod,WS_UMR,i,false);
				continue;
			}
			#endif

			// Bounce when the peer is receiving too slowly
			if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutSize)
			{
				if((pmod->UmrPort[i].SinceOpenTimer > pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut)
					&&(pmod->UmrPort[i].IOCPSendBytes > pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutSize))
				{
					if((!pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod)||
						((pmod->UmrPort[i].TimeOutStart)&&(time(0) -pmod->UmrPort[i].TimeOutStart>=pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod)))
					{
						if(pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod)
							WSHLogError(pmod,"!WSOCKS: UmrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld for %ld[%ld] Seconds after %ld[%ld] Seconds Since Login"
								,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].RemoteIP,pmod->UmrPort[i].IOCPSendBytes
								,time(0) -pmod->UmrPort[i].TimeOutStart,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod
								,pmod->UmrPort[i].SinceOpenTimer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut);
						else
							WSHLogError(pmod,"!WSOCKS: UmrPort[%d][%s][%s] - Dropped - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
							,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].RemoteIP,pmod->UmrPort[i].IOCPSendBytes
								,pmod->UmrPort[i].SinceOpenTimer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut);
						_WSHCloseUmrPort(pmod,i);
						pmod->UmrPort[i].recvThread=0;
						UnlockPort(pmod,WS_UMR,i,false);
						continue;
					}
					else if(!pmod->UmrPort[i].TimeOutStart)
					{
						pmod->UmrPort[i].TimeOutStart=time(0);
						WSHLogEvent(pmod,"!WSOCKS: UmrPort[%d][%s][%s] - Timer Started[%ld] - Holding Buffer was %ld after %ld[%ld] Seconds Since Login"
							,i,pmod->UmrPort[i].Note,pmod->UmrPort[i].RemoteIP,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOutPeriod,pmod->UmrPort[i].IOCPSendBytes
							,pmod->UmrPort[i].SinceOpenTimer,pmod->UmcPort[pmod->UmrPort[i].UmcPort].TimeOut);
					}
				}
				else
					pmod->UmrPort[i].TimeOutStart=0;
			}
			pmod->UmrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UMR,i,false);
		}

		if(LockPort(pmod,WS_UMR,i,true,0)==WAIT_OBJECT_0)
		{
			pmod->UmrPort[i].sendThread=tid;
			//pmod->UmrPort[i].SinceLastBeatCount++;
			pmod->WSBeforeUmrSend(i);
			WSHUmrSend(pmod,i,true);
			pmod->UmrPort[i].recvThread=0;
			UnlockPort(pmod,WS_UMR,i,true);
		}
	}
	#endif //#ifdef WS_MONITOR

	for(i=0;i<pmod->NO_OF_CON_PORTS;i++)
	{
		if((!pmod->ConPort[i].InUse)||(!pmod->ConPort[i].SockConnected))
			continue;
		// Lock the port
		LockPort(pmod,WS_CON,i,false);
		{
			pmod->ConPort[i].recvThread=tid;
			//printf("DEBUG CON%d active %d\n",i,tid);
			// Bounce when not enough data when there should be
			if(pmod->ConPort[i].Bounce)
			{
				DWORD tnow=GetTickCount();
				if((pmod->ConPort[i].LastDataTime)&&(tnow -pmod->ConPort[i].LastDataTime>pmod->ConPort[i].Bounce))
				{
					int hhmm=WSTime/100;
					if(((!pmod->ConPort[i].BounceStart)||(hhmm>=pmod->ConPort[i].BounceStart))&&
					((!pmod->ConPort[i].BounceEnd)||(hhmm<pmod->ConPort[i].BounceEnd)))
					{
						WSHLogError(pmod,"!WSOCKS: ConPort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->ConPort[i].LastDataTime));
						_WSHCloseConPort(pmod,i);
						pmod->ConPort[i].recvThread=0;
						UnlockPort(pmod,WS_CON,i,false);
						continue;
					}
				}
				else
					pmod->ConPort[i].LastDataTime=tnow;
			}
			pmod->ConPort[i].recvThread=0;
			UnlockPort(pmod,WS_CON,i,false);
		}

		if(LockPort(pmod,WS_CON,i,true,0)==WAIT_OBJECT_0)
		{
			pmod->ConPort[i].sendThread=tid;
			pmod->WSBeforeConSend(i);
			WSHConSend(pmod,i,true);
			pmod->ConPort[i].sendThread=0;
			UnlockPort(pmod,WS_CON,i,true);
		}
	}

	#ifdef WS_FILE_SERVER
    for ( i=0; i<pmod->NO_OF_FILE_PORTS; i++ )
    {
		if((!pmod->FilePort[i].InUse)||(!pmod->FilePort[i].SockConnected))
			continue;
		LockPort(pmod,WS_FIL,i,false);
		{
			pmod->FilePort[i].recvThread=tid;
			// Bounce when not enough data when there should be
			if(pmod->FilePort[i].Bounce)
			{
				DWORD tnow=GetTickCount();
				if((pmod->FilePort[i].LastDataTime)&&(tnow -pmod->FilePort[i].LastDataTime>pmod->FilePort[i].Bounce))
				{
					int hhmm=WSTime/100;
					if(((!pmod->FilePort[i].BounceStart)||(hhmm>=pmod->FilePort[i].BounceStart))&&
					((!pmod->FilePort[i].BounceEnd)||(hhmm<pmod->FilePort[i].BounceEnd)))
					{
						WSHLogError(pmod,"!WSOCKS: FilePort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->FilePort[i].LastDataTime));
						_WSHCloseFilePort(pmod,i);
						pmod->FilePort[i].recvThread=0;
						UnlockPort(pmod,WS_FIL,i,false);
						continue;
					}
				}
				else
					pmod->FilePort[i].LastDataTime=tnow;
			}
			pmod->FilePort[i].recvThread=0;
			UnlockPort(pmod,WS_FIL,i,false);
		}

		if(LockPort(pmod,WS_FIL,i,true,0)==WAIT_OBJECT_0)
		{
			pmod->FilePort[i].sendThread=tid;
			pmod->WSBeforeFileSend(i);
			WSHFileSend(pmod,i,true);
			pmod->FilePort[i].sendThread=0;
			UnlockPort(pmod,WS_FIL,i,true);
		}
    }
	#endif //#ifdef WS_FILE_SERVER

	#ifdef WS_GUARANTEED
	for(i=0;i<pmod->NO_OF_CGD_PORTS;i++)
	{
		if((!pmod->CgdPort[i].InUse)||(!pmod->CgdPort[i].SockConnected))
			continue;
		LockPort(pmod,WS_CGD,i,false);
		{
			pmod->CgdPort[i].recvThread=tid;
			// Bounce when not enough data when there should be
			if(pmod->CgdPort[i].Bounce)
			{
				DWORD tnow=GetTickCount();
				if((pmod->CgdPort[i].LastDataTime)&&(tnow -pmod->CgdPort[i].LastDataTime>pmod->CgdPort[i].Bounce))
				{
					int hhmm=WSTime/100;
					if(((!pmod->CgdPort[i].BounceStart)||(hhmm>=pmod->CgdPort[i].BounceStart))&&
					((!pmod->CgdPort[i].BounceEnd)||(hhmm<pmod->CgdPort[i].BounceEnd)))
					{
						WSHLogError(pmod,"!WSOCKS: CgdPort[%d], was Bounced last data receved %ldms ago",i,(tnow -pmod->CgdPort[i].LastDataTime));
						_WSHCloseCgdPort(pmod,i);
						pmod->CgdPort[i].recvThread=0;
						UnlockPort(pmod,WS_CGD,i,false);
						continue;
					}
				}
				else
					pmod->CgdPort[i].LastDataTime=tnow;
			}
			pmod->CgdPort[i].recvThread=0;
			UnlockPort(pmod,WS_CGD,i,false);
		}

		if(LockPort(pmod,WS_CGD,i,true,0)==WAIT_OBJECT_0)
		{
			pmod->CgdPort[i].sendThread=tid;
			_WSBeforeCgdSend(pmod,i);
			WSHCgdSend(pmod,i,false);
			pmod->CgdPort[i].sendThread=0;
			UnlockPort(pmod,WS_CGD,i,true);
		}
	}
	#endif //#ifdef WS_GUARANTEED

	//for(i=0;i<pmod->NO_OF_CTO_PORTS;i++)
	//{
	//	if((!pmod->CtoPort[i].InUse)||(!pmod->CtoPort[i].SockActive))
	//		continue;
	//	if(LockPort(pmod,WS_CTO,i,true,0)==WAIT_OBJECT_0)
	//	{
	//		pmod->CtoPort[i].sendThread=tid;
	//		pmod->WSBeforeCtoSend(i);
	//		pmod->CtoPort[i].sendThread=0;
	//		UnlockPort(pmod,WS_CTO,i,true);
	//	}
	//}
	return 0;
}

#endif//WS_OIO
