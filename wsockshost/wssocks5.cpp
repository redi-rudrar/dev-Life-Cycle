
#include "stdafx.h"
#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

int WsocksHostImpl::WSHS5Login(WsocksApp *pmod, int PortNo)
{
	char Buff[1024];
	sprintf(Buff,"%c%c%c",pmod->ConPort[PortNo].S5Version,1,pmod->ConPort[PortNo].S5Methode);
	pmod->ConPort[PortNo].S5Status=10;
	pmod->WSConSendBuffNew(3,Buff,PortNo,1,TRUE);
	//pmod->WSLogEvent("SOCKS5 DEBUG WSHS5Login: Sent Version=%d, EncryptMethod=%d",pmod->ConPort[PortNo].S5Version,pmod->ConPort[PortNo].S5Methode);// BAOHACK
	return(TRUE);
}

int WsocksHostImpl::WSHS5Connect(WsocksApp *pmod, int PortNo)
{
	char Buff[1024];
	SOCKADDR_IN remote_sin;
	
	remote_sin.sin_addr.s_addr=inet_addr(pmod->ConPort[PortNo].RemoteIP);
	remote_sin.sin_port=htons(pmod->ConPort[PortNo].Port);

	sprintf(Buff,"%c%c%c%c",pmod->ConPort[PortNo].S5Version,1,0,1);
	pmod->ConPort[PortNo].S5Status=20;
	pmod->WSConSendBuffNew(4,Buff,PortNo,0,TRUE);
	pmod->WSConSendBuffNew(4,(char*)&remote_sin.sin_addr.s_addr,PortNo,0,TRUE);
	pmod->WSConSendBuffNew(2,(char*)&remote_sin.sin_port,PortNo,1,TRUE);
	//pmod->WSLogEvent("SOCKS5 DEBUG WSHS5Connect: Sent Ip=%s, Port=%d",pmod->ConPort[PortNo].RemoteIP,pmod->ConPort[PortNo].Port);// BAOHACK
	return(TRUE);
}
