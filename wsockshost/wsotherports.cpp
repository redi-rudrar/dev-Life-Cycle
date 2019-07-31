
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

int WsocksHostImpl::WSHSetupOtherPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=0;i<=pmod->NO_OF_OTHER_PORTS;i++)
		{
			memset(&pmod->OtherPort[i],0,sizeof(OTHERPORT));
		}
		break;
	case WS_OPEN: // Open
		for (i=0;i<pmod->NO_OF_OTHER_PORTS;i++)
		{
			if(strlen(pmod->OtherPort[i].RemoteIP)>0)
			{
				pmod->OtherPort[i].InUse=TRUE;
				//AddConListItem(GetDispItem(WS_OTH,i));
			}
		}
		break;
	case WS_CLOSE: // close
		for (i=0;i<=pmod->NO_OF_OTHER_PORTS;i++)
		{
			WSHCloseOtherPort(pmod,i);
			if(pmod->OtherPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_OTH,i));
				pmod->OtherPort[i].InUse=FALSE;
			}
		}
		break;
	}
	return(TRUE);
}
int WsocksHostImpl::WSHOpenOtherPort(WsocksApp *pmod, int PortNo)
{
	WSPort *pport=WSHCreatePort(pmod,WS_OTH,PortNo);
	if(pport)
	{
		//AddConListItem(GetDispItem(WS_OTH,PortNo));
		pmod->OtherPort[PortNo].InUse=1;
		WSHUpdatePort(pmod,WS_OTH,PortNo);
		pmod->WSOtherOpened(PortNo);
	}
	return PortNo;
}
int WsocksHostImpl::WSHCloseOtherPort(WsocksApp *pmod, int PortNo)
{
	//DeleteConListItem(GetDispItem(WS_OTH,PortNo));
	pmod->WSOtherClosed(PortNo);
	pmod->OtherPort[PortNo].InUse=0;
    WSHUpdatePort(pmod, WS_OTH, PortNo);
	// Allow reuse of the Z ports and resolve close for multi-threaded apps
	//WSHDeletePort(pmod,WS_OTH,PortNo);
	memset(&pmod->OtherPort[PortNo],0,sizeof(OTHERPORT));
	return 0;
}
