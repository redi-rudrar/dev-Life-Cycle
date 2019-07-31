// 
// dnsapi.h : main header file for the dnsapi DLL
//
#ifndef _DNSAPI_H
#define _DNSAPI_H

#ifdef MAKE_DLL
#ifdef MAKE_DNSAPI
#define DNSEXPORT __declspec(dllexport)
#else
#define DNSEXPORT __declspec(dllimport)
#endif
#else
#define DNSEXPORT
#endif

// API data types
#pragma pack(push,8)

enum ServiceStatus
{
SVS_UNKNOWN,
SVS_DISABLED,
SVS_OFFLINE,
SVS_ONLINE,
SVS_COUNT,
};

typedef struct
{
	char szHost[20];
	char szPort[8];
	WORD wCpu;
	WORD wMemory;
	WORD wTypeServer;
	WORD wNrusers;
	WORD wOS;
	WORD wIqversion;
	WORD wNropenquotes;
	char szDomain[9];
	char ui[9];
} DNSINFO;

typedef struct tdDnsInfoEx :public DNSINFO
{
	DWORD dwIqversion;
	char reserved[64];
} DNSINFOEX;

#include "dbcommon.h"
typedef struct tdUserDoc
{
	USERDOCREC UserDocRec;
	bool EntitleDelete;  // Use only diring re-entitle
	int PortType;
	int PortNo;
	char Uuid[16];
}USERDOC,*pUSERDOC;

typedef struct tdDomainDoc
{
	DOMAINDOCREC DomainDocRec;
	bool EntitleDelete;  // Use only diring re-entitle
	int PortType;
	int PortNo;
	char Uuid[16];
}DOMAINDOC,*pDOMAINDOC,**ppDOMAINDOC;

// Callback interface
class IqdnsNotify
{
public:
	IqdnsNotify()
		:wantCompress(true)
		,wantEncrypt(true)
	{
	}
	// Called to make a connection to IQ
	virtual int ConnectDNS(void *idata, struct sockaddr_in *addr, int alen)=0;
	// Called to drop a connection from IQ
	virtual int DisconnectDNS(void *idata, struct sockaddr_in *addr, int alen)=0;
	// Called to send a message to IQ
	virtual int SendDNS(void *idata, const char *buf, int blen, bool compressed)=0;
	// Called to wait for message response
	virtual int WaitDNS(void *idata)=0;

	// Notifies an error condition
	virtual int NotifyErrorDNS(void *idata, int ecode, const char *emsg)=0;
	// Notifies an heartbeat
	virtual int NotifyHeartbeatDNS(void *idata)=0;

	// Client (void *idata, InstaQuote, iqclientapi, feedproxapi)
	virtual int NotifyService(void *idata, const char *svcgroup, const char *svcname, const char *ip, int port){return 0;}
	virtual int NotifyLogin(void *idata, const char *Domain, const char *User, bool LoginOk, const char *RejectReason){return 0;}
	virtual int NotifyChangePassword(void *idata, const char *Domain, const char *User, int expire, const char *notice){return 0;}

	// Authorizer (void *idata, Iqclient, Iqserver, FIX server, OMS, MDVendor server)
	virtual int NotifyAuthResult(void *idata, const char *Domain, const char *User, unsigned char cEnable[32][32], const char *RejectReason){return 0;}
	virtual int NotifyLoginElsewhere(void *idata, const char *Domain, const char *User, UUID *Uuid){return 0;}

	// Entitlement Manager (void *idata, Clserver)
	virtual int NotifyDocumentSigned(void *idata, DOMAINDOC *pdoc){return 0;}
	virtual int NotifyChangePassword(void *idata, const char *Domain, const char *User, const char *OldPass, const char *NewPass){return 0;}

	// Primary and Backup DNS
	virtual int NotifyDomainBegin(void *idata){return 0;}
	virtual int NotifyDomain(void *idata, const char *Domain){return 0;}
	virtual int NotifyDomainEnd(void *idata){return 0;}

	virtual int NotifyAccountBegin(void *idata, const char *Domain){return 0;}
	virtual int NotifyAccount(void *idata, const char *Domain, const char *Account){return 0;}
	virtual int NotifyAccountEnd(void *idata, const char *Domain){return 0;}

	virtual int NotifyUser(void *idata, const char *Domain, const char *User, const char *Password){return 0;}
	virtual int NotifyUserEntitleBegin(void *idata, const char *Domain, const char *User){return 0;}
	virtual int NotifyUserEntitle(void *idata, const char *Domain, const char *User, const char *Account){return 0;}
	virtual int NotifyUserEntitleEnd(void *idata, const char *Domain, const char *User){return 0;}

	virtual int NotifyDocBegin(void *idata, const char *Domain){return 0;}
	virtual int NotifyDoc(void *idata, const char *Domain, DOMAINDOC *pdoc){return 0;}
	virtual int NotifyDoc(void *idata, const char *Domain, USERDOC *pdoc){return 0;}
	virtual int NotifyDocEnd(void *idata, const char *Domain){return 0;}

	virtual int NotifyVarsidBegin(void *idata){return 0;}
	virtual int NotifyVarsid(void *idata, VARSIDREC *pVarsId){return 0;}
	virtual int NotifyVarsidEnd(void *idata){return 0;}

	// Encoding options
	bool wantCompress;
	bool wantEncrypt;
};
// InstaQuote DNS protocol
// Use this interface if you want to manage your own TCP connections.
class IqdnsApi
{
public:
	// Two-phase API initialization
	DNSEXPORT IqdnsApi();
	DNSEXPORT ~IqdnsApi();
	DNSEXPORT int Init(IqdnsNotify *pnotify, void *idata, bool compressed);
	DNSEXPORT int Shutdown();

	// Session
	DNSEXPORT int Heartbeat();
	DNSEXPORT int Disconnected();
	DNSEXPORT int RecvDNS(const char *buf, int blen, bool uncompressed);
	DNSEXPORT int CheckDone();

	// Client (InstaQuote, iqclientapi, feedproxapi)
	DNSEXPORT int SendLogin(const char *Domain, const char *User, const char *Password, int ProductType, int BuildNo, int LineID, const char *dnsCfgPath=0);

	// Authorizer (Iqclient, Iqserver, FIX server, OMS, MDVendor server)
	DNSEXPORT int PublishService(const char *ServiceClass, const char *ServiceName, ServiceStatus Status, const char *ip, int port);
	DNSEXPORT int LoadReport(const char *ServiceClass, const char *ServiceName, DNSINFOEX *dinfo);
	DNSEXPORT int Authorize(const char *Domain, const char *User, const char *Password, char EncryptionType, 
							int ProductType, int BuildNo, int LineID, char RejectReason[80]);
	DNSEXPORT int LogoutElsewhere(const char *Domain, const char *User, UUID *Uuid);

	// Entitlement Manager (Clserver)
	DNSEXPORT int CreateDomainBegin();
	DNSEXPORT int CreateDomain(const char *Domain);
	DNSEXPORT int CreateDomainEnd();

	DNSEXPORT int CreateAccountBegin(const char *Domain);
	DNSEXPORT int CreateAccount(const char *Domain, const char *Account);
	DNSEXPORT int CreateAccountEnd(const char *Domain);

	DNSEXPORT int CreateUser(const char *Domain, const char *User, const char *Password);
	DNSEXPORT int EntitleUserAccountBegin(const char *Domain, const char *User);
	DNSEXPORT int EntitleUserAccount(const char *Domain, const char *User, const char *Account);
	DNSEXPORT int EntitleUserAccountEnd(const char *Domain, const char *User);

	DNSEXPORT int CreateDocBegin(const char *Domain);
	DNSEXPORT int CreateDoc(const char *Domain, DOMAINDOC *pdoc);
	DNSEXPORT int CreateDoc(const char *Domain, USERDOC *pdoc);
	DNSEXPORT int CreateDocEnd(const char *Domain);

	DNSEXPORT int CreateVarsidBegin();
	DNSEXPORT int CreateVarsid(VARSIDREC *pVarsId);
	DNSEXPORT int CreateVarsidEnd();

	// Backup DNS
	DNSEXPORT int RequestReplication();

protected:
	class DnsapiImpl *aimpl;
};

#pragma pack(pop)

#endif//_DNSAPI_H
