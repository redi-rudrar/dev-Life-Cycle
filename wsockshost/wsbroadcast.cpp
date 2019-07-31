
#include "stdafx.h"
#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

#include "BroadcastTransmitter.h"
#include "BroadcastReceiver.h"
#include "BroadcastReceiverManager.h"
#include "BroadcastSequencer.h"

#ifdef WS_BROADCAST_RECEIVER
static int __stdcall _WSBrrSendPatch(const char *buf, int blen, void *udata, int gapsize)
{
	pair<WsocksApp *,int> *pdata=(pair<WsocksApp *,int> *)udata;
	if((pdata->second>=0)&&(pdata->second<=pdata->first->NO_OF_CON_PORTS))
	{
		pdata->first->ConPort[pdata->second].broadcastRepairCnt+=gapsize;
		pdata->first->ConPort[pdata->second].PacketsOut+=gapsize;
		pdata->first->ConPort[pdata->second].BeatsOut+=gapsize;
		if(pdata->first->WSConSendBuff(blen,(char*)buf,pdata->second,1)<0)
			return -1;
		return blen;
	}
	return -1;
}
#endif

#ifdef WS_BROADCAST_TRANSMITTER
int WsocksHostImpl::WSHSetupBrtPorts(int SMode, int PortNo)
{
	int i;

    try
    {
	    switch (SMode)
	    {
	    case WS_INIT: // init
		    for (i = 0; i <= NO_OF_BRT_PORTS; i++)
		    {
			    memset(&BrtPort[i], 0, sizeof(BRTPORT));
		    }
		    break;
	    case WS_OPEN: // Open
		    for (i = 0; i < NO_OF_BRT_PORTS;i++)
		    {
				if(BrtPort[i].InUse)
				{
					if((BrtPort[i].PatchPort<0)||(BrtPort[i].PatchPort>=NO_OF_USC_PORTS))
					{
						pmod->WSLogError("!WSOCKS: FATAL ERROR : Patch port USC%d out of range for broadcast port BRT%d",BrtPort[i].PatchPort,i);
						continue;
					}
					try
					{
						BroadcastPatchServer *patchServer = UscPort[BrtPort[i].PatchPort].broadcastPatchServer;
						if(0 == patchServer)
						{
							pmod->WSLogError("!WSOCKS: FATAL ERROR : Patch port USC%d not found for broadcast port BRT%d",BrtPort[i].PatchPort,i);
							continue;
						}
						BrtPort[i].transmitter = new BroadcastTransmitter(patchServer,BrtPort[i].RemoteIP,BrtPort[i].Port,BrtPort[i].Ttl,BrtPort[i].LocalIP);
					}
					catch(GeneralException &e)
					{
						pmod->WSLogError("!WSOCKS: FATAL ERROR: transmitter port %d: %s",i,e.what());
						BrtPort[i].InUse=0;
					}
				#ifndef WS_DISPOFF
				#ifndef UNIX
				    AddConListItem(GetDispItem(WS_BRT,i));
				#endif
				#endif// NOT WS_DISPOFF
				}
		    }
		#ifndef WS_DISPOFF
		#ifndef UNIX
		    AddConListItem(GetDispItem(WS_BRT_TOT, 0));
		#endif
		#endif// NOT WS_DISPOFF
		    break;
	    case WS_CLOSE: // close
		    for (i = 0; i <= NO_OF_BRT_PORTS;i++)
		    {
			    if(BrtPort[i].InUse)
			    {
					if(BrtPort[i].SockActive)
						WSCloseBrtPort(i);
					if(BrtPort[i].transmitter)
					{
						delete BrtPort[i].transmitter; BrtPort[i].transmitter=0;
					}
				#ifndef WS_DISPOFF
				#ifndef UNIX
				    DeleteConListItem(GetDispItem(WS_BRT,i));
				#endif
				#endif// NOT WS_DISPOFF
				    BrtPort[i].InUse=FALSE;
			    }
		    }
		    break;
	    }
    }
    catch(GeneralException &e)
    {
        pmod->WSLogError("%s %s", (SMode == 0 ? "INIT" : SMode == 1 ? "OPEN" : "CLOSE"), e.what());
        return FALSE;
    }
    return(TRUE);
	return 0;
}

int WsocksHostImpl::WSHBrtSendBuff(int MsgLen, char *MsgOut, int PortNo)
{
    if((PortNo>=0)&&(PortNo<NO_OF_BRT_PORTS))
    {
        BroadcastTransmitter *transmitter = BrtPort[PortNo].transmitter;
        if(transmitter)
        {
            try
            {
                return transmitter->SendData(MsgOut, MsgLen);
	        }
	        catch(GeneralException e)
	        {
		        pmod->WSLogError(e.what());
                return FALSE;
	        }
        }
		return TRUE;
    }
    return FALSE;
}
int WsocksHostImpl::WSHOpenBrtPort(int PortNo)
{
	if((PortNo>=0)&&(PortNo<=NO_OF_BRT_PORTS)&&
	   (BrtPort[PortNo].transmitter)&&(!BrtPort[PortNo].SockActive))
	{
		if(BrtPort[PortNo].ReconCount>0)
		{
			BrtPort[PortNo].ReconCount--;
			return FALSE;
		}
		try
		{
			BrtPort[PortNo].transmitter->Enable();
			BrtPort[PortNo].SockActive=TRUE;
			WSBrtOpened(PortNo);
			return TRUE;
		}
		catch(GeneralException &e)
		{
			BrtPort[PortNo].ReconCount=WS_BRT_RECON;
			pmod->WSLogError("Open of Brt[%d]: %s", PortNo, e.what());
		}
	}
	return FALSE;
}
int WsocksHostImpl::WSHCloseBrtPort(int PortNo)
{
	if((PortNo>=0)&&(PortNo<=NO_OF_BRT_PORTS)&&
	   (BrtPort[PortNo].transmitter)&&(BrtPort[PortNo].SockActive))
	{
		try
		{
			BrtPort[PortNo].transmitter->Disable();
			BrtPort[PortNo].SockActive=FALSE;
			BrtPort[PortNo].ReconCount=WS_BRT_RECON;
			WSBrtClosed(PortNo);
			return TRUE;
		}
		catch(GeneralException &e)
		{
			pmod->WSLogError("Open of Brt[%d]: %s", PortNo, e.what());
		}
	}
	return FALSE;
}
#endif//WS_BROADCAST_TRANSMITTER

#ifdef WS_BROADCAST_RECEIVER
static void _WSBrrReceive(int userData, char *buffer, int bufferSize)
{
	pair<WsocksApp *,int> *udata=(pair<WsocksApp *,int> *)userData;
	if(udata)
		udata->first->WSBrrRecvMsg(udata->second,buffer,bufferSize);
}

int WsocksHostImpl::WSHSetupBrrPorts(int SMode, int PortNo)
{
	int i;
	
    try
    {
        switch (SMode)
	    {
	    case WS_INIT: // init
		    for (i = 0; i <= NO_OF_BRR_PORTS; i++)
		    {
			    memset(&BrrPort[i], 0, sizeof(BRRPORT));
		    }
		    break;
	    case WS_OPEN: // Open
		    for (i = 0; i < NO_OF_BRR_PORTS;i++)
		    {
				if(!BrrPort[i].InUse)
					continue;
				if((BrrPort[i].PatchPort<0)||(BrrPort[i].PatchPort>=NO_OF_CON_PORTS))
				{
					pmod->WSLogError("!WSOCKS: FATAL ERROR : Patch port CON%d out of range for broadcast port BRR%d",BrrPort[i].PatchPort,i);
					continue;
				}
				try
				{
					if(!broadcastReceiverManager)
						broadcastReceiverManager=new BroadcastReceiverManager;
				}
				catch(GeneralException &e)
				{
					pmod->WSLogError("!WSOCKS: FATAL ERROR: Broadcast port %d: %s",i,e.what());
				}
			#ifndef WS_DISPOFF
			#ifndef UNIX
			    AddConListItem(GetDispItem(WS_BRR,i));
			#endif
			#endif// NOT WS_DISPOFF
		    }
    #ifndef WS_DISPOFF
    #ifndef UNIX
		    AddConListItem(GetDispItem(WS_BRR_TOT,0));
    #endif
    #endif// NOT WS_DISPOFF
		    break;
	    case WS_CLOSE: // close
		    for (i = 0; i <= NO_OF_BRR_PORTS;i++)
		    {
			    if(BrrPort[i].InUse)
			    {
					if(BrrPort[i].SockActive)
						WSCloseBrrPort(i);
				#ifndef WS_DISPOFF
				#ifndef UNIX
				    DeleteConListItem(GetDispItem(WS_BRR,i));
				#endif
				#endif// NOT WS_DISPOFF
				    BrrPort[i].InUse=FALSE;
			    }
		    }
			if(broadcastReceiverManager)
			{
				delete broadcastReceiverManager; broadcastReceiverManager=0;
			}
		    break;
	}
    }
    catch(GeneralException &e)
    {
        pmod->WSLogError("%s %s", (SMode == 0 ? "INIT" : SMode == 1 ? "OPEN" : "CLOSE"), e.what());
        return FALSE;
    }
	return(TRUE);
}
int WsocksHostImpl::WSHOpenBrrPort(int PortNo)
{
	if((PortNo>=0)&&(PortNo<=NO_OF_BRR_PORTS)&&(!BrrPort[PortNo].SockActive))
	{
		if(BrrPort[PortNo].ReconCount>0)
		{
			BrrPort[PortNo].ReconCount--;
			return FALSE;
		}
		try
		{
			pair<WsocksApp *,int> *rdata=new pair<WsocksApp *,int>;
			rdata->first=this;
			rdata->second=PortNo;
			BrrPort[PortNo].rdata=rdata;
			BrrPort[PortNo].receiver = broadcastReceiverManager->CreateReceiver(
				BrrPort[PortNo].RemoteIP,BrrPort[PortNo].Port,BrrPort[PortNo].LocalIP,_WSBrrReceive,(int)rdata,BrrPort[PortNo].FifoSizeLimit);
#ifdef NO_WS_BROADCAST_TRANSPORT
			BroadcastPatchClient *pclient=ConPort[BrrPort[PortNo].PatchPort].broadcastPatchClient;
			if(pclient)
			{
				uint32_t handle=inet_addr(BrrPort[PortNo].RemoteIP);
				// Lets BroadcastReceiver know where to send patch requests
				BrrPort[PortNo].receiver->SetPatch(ConPort[BrrPort[PortNo].PatchPort].broadcastPatchClient,handle);
				// Lets BroadcastPatchClient know where to route patch replies
				ConPort[BrrPort[PortNo].PatchPort].broadcastPatchClient->SetFIFO(handle,BrrPort[PortNo].receiver->GetSequencer()->GetPatchFIFO());
			}
			else
				pmod->WSLogError("Open of Brr[%d]: CON%d is not a repair connection!", PortNo,BrrPort[PortNo].PatchPort);
			BrrPort[PortNo].receiver->Enable();
#else
#error Only NO_WS_BROADCAST_TRANSPORT supported by WSOCKS!
#endif
			BrrPort[PortNo].SockActive=TRUE;
			WSBrrOpened(PortNo);
			return TRUE;
		}
		catch(GeneralException &e)
		{
			BrrPort[PortNo].ReconCount=WS_BRR_RECON;
			pmod->WSLogError("Open of Brr[%d]: %s", PortNo, e.what());
		}
	}
	return FALSE;
}
int WsocksHostImpl::WSHCloseBrrPort(int PortNo)
{
	if((PortNo>=0)&&(PortNo<=NO_OF_BRR_PORTS)&&(BrrPort[PortNo].SockActive))
	{
		try
		{
			BrrPort[PortNo].receiver->Disable();
			broadcastReceiverManager->DeleteReceiver((char*)BrrPort[PortNo].receiver->GetFullAddress());
			BrrPort[PortNo].receiver=0;
			pair<WsocksApp *,int> *rdata=(pair<WsocksApp *,int> *)BrrPort[PortNo].rdata;
			delete rdata;
			BrrPort[PortNo].SockActive=FALSE;
			BrrPort[PortNo].ReconCount=WS_BRR_RECON;
			WSBrrClosed(PortNo);
			return TRUE;
		}
		catch(GeneralException &e)
		{
			pmod->WSLogError("Close of Brr[%d]: %s", PortNo, e.what());
		}
	}
	return FALSE;
}
#endif//WS_BROADCAST_RECEIVER
