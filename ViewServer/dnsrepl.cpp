												   
#include "stdafx.h"
#include "ViewServer.h"

#ifdef IQDNS

#pragma pack(push,8)
#include "MESSAGE.H"

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

typedef struct tdUserAcc
{
	USERACCREC UserAccRec;
	bool EntitleDelete;  // Use only during re-entitle
}USERACC,*pUSERACC;

typedef struct tdpeekmessage
{
	char szSymbol[80];
	char szSymbol1[20];
	long lStartDate;
	long lEndDate;
	long lStartTime;
	long lEndTime;
	long period;
	HANDLE hSession;
	HWND hWnd;
} PEEKMESSAGE;

typedef struct tdServerInfo
{
	DNSINFOEX DnsInfo;
	int UserCount;
	bool InetServer;
	char res[3];
	bool DirServer;
	bool RemoteServer;
	bool GroupServer;
	char res2[2];
	char szGroups[256];
} SERVERINFO,*pSERVERINFO;
#pragma pack(pop)

class IQUser
{
public:
	IQUser()
		:AccountList()
		,EntitleDelete(false)
		,LastFeedIP(0)
	{
		memset(&Msg1960Rec,0,sizeof(MSG1960REC));
	}
	~IQUser()
	{
		AccountList.clear();
	}

	MSG1960REC Msg1960Rec;
	list<USERACC> AccountList;
//	bool LoginSaved;
	bool EntitleDelete;  // Use only diring re-entitle
//	long UnsuccessfulLogins;
//	long LastBadLoginTime;
//	bool EnhancedSecurity;
//	unsigned char PasswordLifetime;// bumber of days a password may live
//#ifdef MD5_PASSWORD
//	unsigned char MD5Password[16];
//#endif
	long LastFeedIP;
};

// Extension to AppInstance (probably should be subclass)
typedef map<string,IQUser *> USERMAP;
class IQDomain
{
public:
	IQDomain()
		:umap()
		,acmap()
	{
		memset(&Msg972Rec,0,sizeof(MSG972REC));
	}
	~IQDomain()
	{
		for(USERMAP::iterator uit=umap.begin();uit!=umap.end();uit++)
			delete uit->second;
		umap.clear();
		acmap.clear();
	}

	MSG972REC Msg972Rec;
	USERMAP umap;
	ACCTMAP acmap;
};

// Extension to AppSystem (probably should be subclass)
typedef multimap<string,AppInstance*> DOMRELMAP;
typedef map<string,DNSINFOEX> IQSMAP;
class DnsSystem
{
public:
	DnsSystem()
		:rpmap()
		,epmap()
		,varsIdMap()
		,smap()
	{
	}
	~DnsSystem()
	{
		for(map<string,VARSIDREC*>::iterator vit=varsIdMap.begin();vit!=varsIdMap.end();vit++)
			delete vit->second;
		varsIdMap.clear();
		epmap.clear();
		rpmap.clear();
	}

	DOMRELMAP rpmap; // Real parent map
	DOMRELMAP epmap; // Entitlement parent map
	map<string,VARSIDREC*> varsIdMap;
	IQSMAP smap;
};

int ViewServer::DnsReplOpened(int ConPortNo)
{
	//ConPort[ConPortNo].DetPtr5=new DnsReplClient;
	MSG963REC Msg963Rec;
	strcpy(Msg963Rec.IP,"0.0.0.0");
	Msg963Rec.Port=0;
	strcpy(Msg963Rec.ServiceName,"DNS_REP_REQ_V1");
	WSConSendMsg(963,sizeof(MSG963REC),(char*)&Msg963Rec,ConPortNo);
	return 0;
}
int ViewServer::DnsReplClosed(int ConPortNo)
{
	//DnsReplClient *drc=(DnsReplClient *)ConPort[ConPortNo].DetPtr5;
	//if(drc)
	//{
	//	delete drc; ConPort[ConPortNo].DetPtr5=0;
	//}
	return 0;
}
int ViewServer::DnsMgrOpened(int ConPortNo)
{
	SERVERINFO Msg024Rec;
	memset(&Msg024Rec,0,sizeof(SERVERINFO));
	Msg024Rec.DnsInfo.wTypeServer=1;
	WSConSendMsg(21,sizeof(SERVERINFO),(char*)&Msg024Rec,ConPortNo);
	return 0;
}
int ViewServer::DnsMgrClosed(int ConPortNo)
{
	return 0;
}
// A Domain is an AppInstance under the CLSERVER system
AppInstance *ViewServer::CreateDnsDomain(int ConPortNo, const char *Domain, AppSystem *nasys)
{
	if(!*Domain)
	{
		WSLogError("CON%d: Cannot create DNS Domain/AppInstID with no name",ConPortNo);
		return 0;
	}
	if(strlen(Domain)>DOMAIN_LEN)
	{
		WSLogError("CON%d: Domain/AppInstID name is too long %s",ConPortNo,Domain);
		return 0;
	}
	//DT9044: If an AppSystem was passed in, then use that one.  This is used to support reloads of 
	// IQ AccountRecs upon restart.
	AppInstance *ainst=0;
	if(nasys)
	{
		APPINSTMAP::iterator aimap_itr=nasys->imap.find(Domain);
		if(aimap_itr!=nasys->imap.end())
		{
			ainst=aimap_itr->second;
		}
	}
	else
	{
		ainst=GetAppInstance(Domain);
	}
	if(ainst)
	{
		// This might happen if a domain is pre-configured in appsystems.ini
		if(!ainst->iqptr)
		{
			IQDomain *pdom=new IQDomain;
			strcpy(pdom->Msg972Rec.Domain,Domain);
			strcpy(pdom->Msg972Rec.Parent,Domain);
			strcpy(pdom->Msg972Rec.LogicParent,Domain);
			ainst->iqptr=pdom;
		}
		return ainst;
	}

	AppSystem *asys=GetAppSystem(Domain);
	if(!asys)
	{
		#ifdef DNS_INSTANCES
		string akey=TACMAP_SYSTEM +(string)"*";
		asys=GetAppSystem(akey.c_str());
		#else
		asys=GetAppSystem("CLSERVER*");
		#endif
		if(!asys)
		{
			#ifdef DNS_INSTANCES
			WSLogError("CON%d: AppSystem(%s) for Domain(%s) not found.",ConPortNo,TACMAP_SYSTEM.c_str(),Domain);
			#else
			WSLogError("CON%d: AppSystem(CLSERVER) for Domain(%s) not found.",ConPortNo,Domain);
			#endif
			ReleaseMutex(asmux);
			return 0;
		}
	}
	WaitForSingleObject(asmux,INFINITE);
	int icnt=0;
	ainst=asys->CreateInstance(Domain,icnt);
	if((ainst)&&(icnt>0))
	{
		IQDomain *pdom=new IQDomain;
		strcpy(pdom->Msg972Rec.Domain,Domain);
		strcpy(pdom->Msg972Rec.Parent,Domain);
		strcpy(pdom->Msg972Rec.LogicParent,Domain);
		ainst->iqptr=pdom;
		aimap[ainst->iname]=ainst;
		asys->PregenBrowse();
		char aicname[256]={0};
		sprintf(aicname,"%s/%s",asys->sname.c_str(),ainst->iname.c_str());
		ainst->AcctName=aicname;
		ainst->AcctType="AppInstance";
		ainst->AcctAddParent(asys);
		acmap[ainst->AcctName]=ainst;
		WSLogEvent("CON%d: Created Domain(%s) under AppSystem(CLSERVER)",ConPortNo,ainst->iname.c_str());
	}
	ReleaseMutex(asmux);
	// Make sure every domain has an entry in both maps
	AssociateDnsDomains(ainst,ainst,ainst);
	return ainst;
}
// A real domain ACCOUNTREC
Account *ViewServer::CreateDnsAccount(int ConPortNo, MSG632REC& Msg632Rec, AppSystem *asys) //DT9044
{
	char *Domain=Msg632Rec.AccountRec.Domain;
	char *DnsAccount=Msg632Rec.AccountRec.Account;
	if((!*Domain)||(!*DnsAccount))
	{
		WSLogEvent("CON%d: Domain or Account is empty in MSG632REC.",ConPortNo);
		return 0;
	}

	#ifdef DNS_INSTANCES
	char tacmapDomain[DOMAIN_LEN]={0};
	if(!TACMAP_SYSTEM.empty())
	{
		sprintf(tacmapDomain,"%s%s",TACMAP_PREFIX.c_str(),Domain); Domain=tacmapDomain;
	}
	#endif

	// Find user
	AppInstance *ainst=CreateDnsDomain(ConPortNo,Domain,asys); //DT9044
	if((!ainst)||(!ainst->iqptr))
	{
		WSLogError("CON%d: Domain(%s) for AccountRec(%s/%s) not found.",ConPortNo,Domain,Domain,DnsAccount);
		return 0;
	}
	IQDomain *iqd=(IQDomain*)ainst->iqptr;

	// Create domain accounts
	char akey[256]={0};
	_snprintf(akey,sizeof(akey)-1,"%s|%s",Domain,DnsAccount);
	ACCTMAP::iterator ait=acmap.find(akey);
	Account *pacc=0;
	if(ait==acmap.end())
	{
		Account *nacc=new Account;
		nacc->AcctName=DnsAccount;
		nacc->AcctType=Domain;
		acmap[akey]=nacc;
		iqd->acmap[DnsAccount]=nacc;
		#ifdef REDIOLEDBCON
		#ifdef DNS_INSTANCES
		if((ainst->asys)&&(ainst->asys->sname==TACMAP_SYSTEM))
		#else
		if((ainst->asys)&&(ainst->asys->sname=="TWIST"))
		#endif
		{
			pair<string,Account*> da;
			da.first=Domain;
			da.second=nacc;
			tacmap[DnsAccount]=da;
		}
		#endif
		pacc=nacc;
	}
	else
		pacc=ait->second;
	if(!pacc->iqptr)
		pacc->iqptr=new ACCOUNTREC;
	memcpy(pacc->iqptr,&Msg632Rec.AccountRec,sizeof(ACCOUNTREC));
#ifdef _DEBUG
	char groups[256]={0},*gptr=groups;
	for(int i=0;i<5;i++)
	{
		if(Msg632Rec.AccountRec.Group[i][0])
		{
			if(gptr>groups) 
			{
				*gptr=','; gptr++;
			}
			*gptr='#'; gptr++;
			strcpy(gptr,Msg632Rec.AccountRec.Group[i]); gptr+=strlen(gptr);
		}
	}
	if(pacc->adirty)
		WSLogEvent("CON%d: Created Account(%s/%s) Groups(%s) under Domain(%s)",ConPortNo,Domain,DnsAccount,groups,Domain);
#endif
	return pacc;
}
#ifdef DNS_INSTANCES
int ViewServer::CreateRediAccount(int ConPortNo, MSG632REC& Msg632Rec, AppSystem *asys) //DT9044
{
	char *Domain=Msg632Rec.AccountRec.Domain;
	char *DnsAccount=Msg632Rec.AccountRec.Account;
	if((!*Domain)||(!*DnsAccount))
	{
		WSLogEvent("CON%d: Domain or Account is empty in MSG632REC.",ConPortNo);
		return 0;
	}

	char tacmapDomain[DOMAIN_LEN+8]={0};
	sprintf(tacmapDomain,"%s%s",TACMAP_PREFIX.c_str(),Domain); Domain=tacmapDomain;

	// Only pre-configured domains (AppInstances) get Redi accounts assigned to them
	AppInstance *ainst=GetAppInstance(Domain);
	if(!ainst)
		return -1;
	// Create domain accounts
	if((ainst->asys)&&(ainst->asys->sname==TACMAP_SYSTEM))
	{
		pair<string,Account*> da;
		da.first=Domain;
		da.second=ainst;
		tacmap[DnsAccount]=da;
	}
	return 0;
}
#endif
// A user account entitlement. Allow entitlements for accounts that don't exist.
int ViewServer::CreateDnsUserEnt(int ConPortNo, USERACCREC& UserAccRec)
{
	char *Domain=UserAccRec.Domain;
	char *DnsAccount=UserAccRec.Account;
	// DnsAccount is blank for end of UserAcc entitlements list
	if(!*Domain)
	{
		_ASSERT(false);
		return -1;
	}

	// Find user
	AppInstance *ainst=GetAppInstance(Domain);
	if((!ainst)||(!ainst->iqptr))
	{
		WSLogError("CON%d: Domain(%s) for UserEnt(%s/%s/%s) not found.",
			ConPortNo,Domain,Domain,DnsAccount,UserAccRec.User);
		return 0;
	}
	IQDomain *iqd=(IQDomain*)ainst->iqptr;
	char ukey[256]={0};
	strcpy(ukey,UserAccRec.User); ukey[255]=0;
	_strlwr(ukey);
	USERMAP::iterator uit=iqd->umap.find(ukey);
	if(uit==iqd->umap.end())
	{
		WSLogError("CON%d: User(%s) for UserEnt(%s/%s/%s) not found.",
			ConPortNo,UserAccRec.User,Domain,DnsAccount,UserAccRec.User);
		return 0;
	}
	IQUser *puser=uit->second;

	// Drop deleted accounts
	if(!*DnsAccount)
	{
		for(list<USERACC>::iterator acit=puser->AccountList.begin();acit!=puser->AccountList.end();)
		{
			USERACC& uacc=*acit;
			if(uacc.EntitleDelete)
				acit=puser->AccountList.erase(acit);
			else
				acit++;
		}
	}

	// Entitle user
	list<USERACC>::iterator acit;
	for(acit=puser->AccountList.begin();acit!=puser->AccountList.end();acit++)
	{
		USERACC& uacc=*acit;
		if(!_stricmp(uacc.UserAccRec.Account,DnsAccount))
		{
			uacc.UserAccRec=UserAccRec;
			uacc.EntitleDelete=false;
			break;
		}
	}
	if(acit==puser->AccountList.end())
	{
		USERACC uacc;
		uacc.UserAccRec=UserAccRec;
		uacc.EntitleDelete=false;
		puser->AccountList.push_back(uacc);
	}
#ifdef _DEBUG
	WSLogEvent("Created UserEnt(%s/%s/%s) under Domain(%s)",
		UserAccRec.Domain,UserAccRec.Account,UserAccRec.User,Domain);
#endif

	// Don't create accounts from user entitlements, because they may not actually exist
	//// Create domain accounts
	//if(*DnsAccount=='@') // specific account entitlement
	//{
	//	DnsAccount++;
	//	if(!_stricmp(DnsAccount,"ALL"))
	//		return 0;
	//}
	//else if(*DnsAccount=='#') // groups?
	//{
	//	//if((!_stricmp(DnsAccount,"SUPER"))||(!_stricmp(DnsAccount,"SUPERD")))
	//	//	return 0;
	//	return 0;
	//}
	//else if(*DnsAccount=='^') // OMS server entitlement
	//	return 0;
	//char akey[256]={0};
	//sprintf(akey,"%s|%s",Domain,DnsAccount);
	////strcpy(aname,DnsAccount);

	//ACCTMAP::iterator ait=acmap.find(akey);
	//if(ait==acmap.end())
	//{
	//	Account *nacc=new Account;
	//	nacc->AcctName=DnsAccount;
	//	nacc->AcctType=Domain;
	//	acmap[akey]=nacc;
	//	iqd->acmap[DnsAccount]=nacc;
	//	WSLogEvent("CON%d: Created UserAccount(%s/%s) under Domain(%s)",ConPortNo,Domain,DnsAccount,Domain);
	//	return nacc;
	//}
	//else
	//	return ait->second;
	return 0;
}
int ViewServer::CreateDnsUser(int ConPortNo, MSG1960REC& Msg1960Rec)
{
	AppInstance *ainst=CreateDnsDomain(ConPortNo,Msg1960Rec.Domain);
	if(!ainst || !ainst->iqptr)
		return -1;

	// Not sure why this was added since !ainst->iqptr was just checked above
	//if(!ainst->iqptr) //Bao: app crashes in this function below b/c aisnt->iqptr was null.  this is just a guess
	//{
	//	IQDomain *pdom=new IQDomain;
	//	strcpy(pdom->Msg972Rec.Domain,Msg1960Rec.Domain);
	//	strcpy(pdom->Msg972Rec.Parent,Msg1960Rec.Domain);
	//	strcpy(pdom->Msg972Rec.LogicParent,Msg1960Rec.Domain);
	//	ainst->iqptr=pdom;
	//}

	// User map
	IQDomain *iqd=(IQDomain*)ainst->iqptr;

	// Delete all users not re-entitled
	if(!*Msg1960Rec.User)
	{
		if(strcmp(Msg1960Rec.Password,"EOD")==0)
		{
			for(USERMAP::iterator uit=iqd->umap.begin();uit!=iqd->umap.end();)
			{
				IQUser *puser=uit->second;
				if(puser->EntitleDelete)
				{
					WSLogEvent("CON%d: Deleted User(%s/%s) under Domain(%s)",
						ConPortNo,puser->Msg1960Rec.Domain,puser->Msg1960Rec.User,puser->Msg1960Rec.Domain);
					uit=iqd->umap.erase(uit);
					delete puser;
				}
				else
					uit++;
			}
			WSLogEvent("CON%d: All users received for Domain(%s)",ConPortNo,Msg1960Rec.Domain);
		}
		else
		{
			for(USERMAP::iterator uit=iqd->umap.begin();uit!=iqd->umap.end();uit++)
			{
				IQUser *puser=uit->second;
				puser->EntitleDelete=true;
			}
			
		}
		return 0;
	}

	char ukey[256]={0};
	strcpy(ukey,Msg1960Rec.User); ukey[255]=0;
	_strlwr(ukey);
	IQUser *puser=0;
	USERMAP::iterator uit=iqd->umap.find(ukey);
	if(uit==iqd->umap.end())
	{
		puser=new IQUser;
		puser->Msg1960Rec=Msg1960Rec;
		iqd->umap[ukey]=puser;
	#ifdef _DEBUG
		WSLogEvent("CON%d: Created User(%s/%s) under Domain(%s)",
			ConPortNo,Msg1960Rec.Domain,Msg1960Rec.User,Msg1960Rec.Domain);
	#endif
	}
	else
	{
		puser=uit->second;
		puser->Msg1960Rec=Msg1960Rec;
		// Mark all this user's accounts to be deleted
		for(list<USERACC>::iterator ait=puser->AccountList.begin();ait!=puser->AccountList.end();ait++)
		{
			USERACC& uacc=*ait;
			uacc.EntitleDelete=true;
		}
	}
	return 0;
}
AppInstance::~AppInstance()
{
	if(iqptr)
	{
		IQDomain *iqd=(IQDomain*)iqptr;
		delete iqd; iqptr=0;
	}
}
Account::~Account()
{
	if(iqptr)
	{
		ACCOUNTREC *arec=(ACCOUNTREC*)iqptr;
		delete arec; iqptr=0;
	}
}
int ViewServer::AssociateDnsDomains(AppInstance *pdom, AppInstance *dparent, AppInstance *eparent)
{
	_ASSERT(pdom);
	// Remove old associations first
	if(!pdom->asys->iqptr)
		pdom->asys->iqptr=new DnsSystem;
	DnsSystem *dns=(DnsSystem*)pdom->asys->iqptr;
	IQDomain *cdom=(IQDomain*)pdom->iqptr;
	// Real domain parent associations stored in the AppSystem
	if(dparent)
	{
		// Changed real parent
		//Bao: cdom (pdom->iqptr) was null once...
		if(cdom && _stricmp(cdom->Msg972Rec.LogicParent,dparent->iname.c_str()))
		{
			_ASSERT(false);//untested
			for(DOMRELMAP::iterator mit=dns->rpmap.find(cdom->Msg972Rec.LogicParent);
				(mit!=dns->rpmap.end())&&(mit->first==cdom->Msg972Rec.LogicParent);
				mit++)
			{
				if(mit->second==pdom)
				{
					dns->rpmap.erase(mit);
					break;
				}
			}
		}
		bool found=false;
		DOMRELMAP::iterator mit;
		for(mit=dns->rpmap.find(dparent->iname);
			(mit!=dns->rpmap.end())&&(mit->first==dparent->iname);
			mit++)
		{
			if(mit->second==pdom)
			{
				found=true;
				break;
			}
		}
		if(!found)
		{
			if(mit==dns->rpmap.end())
				dns->rpmap.insert(pair<string,AppInstance *>(dparent->iname,pdom));
			else
				dns->rpmap.insert(mit,pair<string,AppInstance *>(dparent->iname,pdom));
		}
	}
	// Entitlement parent associations stored in the AppSystem
	if(eparent)
	{
		// Changed entitlement parent
		if(cdom && _stricmp(cdom->Msg972Rec.Parent,eparent->iname.c_str()))
		{
			_ASSERT(false);//untested
			for(DOMRELMAP::iterator mit=dns->epmap.find(cdom->Msg972Rec.Parent);
				(mit!=dns->epmap.end())&&(mit->first==cdom->Msg972Rec.Parent);
				mit++)
			{
				if(mit->second==pdom)
				{
					dns->epmap.erase(mit);
					break;
				}
			}
		}
		bool found=false;
		DOMRELMAP::iterator mit;
		for(mit=dns->epmap.find(eparent->iname);
			(mit!=dns->epmap.end())&&(mit->first==eparent->iname);
			mit++)
		{
			if(mit->second==pdom)
			{
				found=true;
				break;
			}
		}
		if(!found)
		{
			if(mit==dns->epmap.end())
				dns->epmap.insert(pair<string,AppInstance *>(eparent->iname,pdom));
			else
				dns->epmap.insert(mit,pair<string,AppInstance *>(eparent->iname,pdom));
		}
	}
	return 0;
}
void AppSystem::FreeIqptr()
{
	if(iqptr)
	{
		DnsSystem *dns=(DnsSystem*)iqptr;
		delete dns; iqptr=0;
	}
}
int ViewServer::HandleConDnsReplMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	//DnsReplClient *drc=(DnsReplClient *)ConPort[PortNo].DetPtr5;
	//if(!drc)
	//	return -1;
	FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
	if(!cstats)
		return -1;

	#ifdef DNS_INSTANCES
	// Only the account messages handled when not SAM ViewServer
	if((TACMAP_SYSTEM!="CLSERVER")&&(MsgHeader->MsgID!=632)&&(MsgHeader->MsgID!=6314))
		return 0;
	#endif

	switch(MsgHeader->MsgID)
	{
	case 21: //reqADDSERVER: // New iqclient
	{
		DNSINFOEX Msg021Rec;
		if(MsgHeader->MsgLen<sizeof(DNSINFO))
			return -1;
		if(MsgHeader->MsgLen>=sizeof(DNSINFOEX))
			memcpy(&Msg021Rec,Msg,sizeof(DNSINFOEX));
		else
			memcpy(&Msg021Rec,Msg,sizeof(DNSINFO));
		// End of server list
		if(!*Msg021Rec.szHost)
			break;
		Msg021Rec.wTypeServer=21;

		AppSystem *asys=GetAppSystem("CLSERVER*");
		if(!asys)
			return -1;
		if(!asys->iqptr)
			asys->iqptr=new DnsSystem;
		DnsSystem *dns=(DnsSystem*)asys->iqptr;
		char skey[1024]={0};
		sprintf(skey,"%s|%s|%d",Msg021Rec.szHost,Msg021Rec.szDomain,Msg021Rec.wTypeServer);
		IQSMAP::iterator sit=dns->smap.find(skey);
		if(sit==dns->smap.end())
			dns->smap[skey]=Msg021Rec;
		else
			sit->second=Msg021Rec;
		break;
	}
	case 1102: // Iqclient load report
	{
		MSG1102REC Msg1102Rec;
		if(MsgHeader->MsgLen<sizeof(MSG1102REC))
			return -1;
		memcpy(&Msg1102Rec,Msg,sizeof(MSG1102REC));

		AppSystem *asys=GetAppSystem("CLSERVER*");
		if(!asys)
			return -1;
		if(!asys->iqptr)
			asys->iqptr=new DnsSystem;
		DnsSystem *dns=(DnsSystem*)asys->iqptr;
		char skey[1024]={0};
		sprintf(skey,"%s|%s|%d",Msg1102Rec.szHost,Msg1102Rec.szDomain,21);
		IQSMAP::iterator sit=dns->smap.find(skey);
		if(sit!=dns->smap.end())
		{
			DNSINFOEX& Msg021Rec=sit->second;
			Msg021Rec.wNrusers=Msg1102Rec.TotUsers;
			Msg021Rec.wNropenquotes=Msg1102Rec.TotQuotes;
		}
		break;
	}
	case 24: //reqLOGOUT: Iqclient logout elsewhere
	{
		if(ConPort[PortNo].DetPtr!=(void*)PROTO_DNS_MGR)
			break;
		PEEKMESSAGE Msg024Rec;
		if(MsgHeader->MsgLen<sizeof(PEEKMESSAGE))
			return -1;
		memcpy(&Msg024Rec,Msg,sizeof(PEEKMESSAGE));
		// The login domain may not be the domain where the user exists
		AppSystem *asys=GetAppSystem("CLSERVER*");
		if(!asys)
			return -1;
		else if(!asys->iqptr)
			return -1;
		DnsSystem *dns=(DnsSystem*)asys->iqptr;
		AppInstance *pdom=GetAppInstance(Msg024Rec.szSymbol +32);
		if(!pdom)
			return -1;
		IQDomain *iqd=(IQDomain*)pdom->iqptr;
		IQUser *puser=0;
		while((iqd)&&(!puser))
		{
			USERMAP::iterator uit=iqd->umap.find(Msg024Rec.szSymbol);
			if(uit!=iqd->umap.end())
			{
				IQUser *puser=uit->second;
				if(puser)
				{
					IN_ADDR ip;
					memcpy(&ip,Msg024Rec.szSymbol1 +16,4);
					puser->LastFeedIP=ip.s_addr;
					WSLogEvent("CON%d: User(%s/%s) logged into iqclient(%s)",
						PortNo,puser->Msg1960Rec.Domain,puser->Msg1960Rec.User,inet_ntoa(ip));
					break;
				}
			}
			// Walk parent domains
			if(!stricmp(iqd->Msg972Rec.Parent,iqd->Msg972Rec.Domain))
				break;
			pdom=GetAppInstance(iqd->Msg972Rec.Parent);
			if(!pdom)
				break;
			iqd=(IQDomain*)pdom->iqptr;
		}
		break;
	}
	case 632: // AccountRec
	{
		MSG632REC Msg632Rec;
		if(MsgHeader->MsgLen<sizeof(MSG632REC))
			return -1;
		memcpy(&Msg632Rec,Msg,sizeof(MSG632REC));
		_strupr(Msg632Rec.AccountRec.Domain);
		_strupr(Msg632Rec.AccountRec.Account);

		#ifdef DNS_INSTANCES
		if(TACMAP_SYSTEM=="CLSERVER")
			CreateDnsAccount(PortNo,Msg632Rec);
		else
			CreateRediAccount(PortNo,Msg632Rec);
		#else
		CreateDnsAccount(PortNo,Msg632Rec);
		#endif
		break;
	}
	case 932: // VARSID message from web
	{
		MSG932REC Msg932Rec;
		if(MsgHeader->MsgLen<sizeof(MSG932REC))
			return -1;
		memcpy(&Msg932Rec,Msg,sizeof(MSG932REC));

		AppSystem *asys=GetAppSystem("CLSERVER*");
		if(!asys)
			return -1;
		if(!asys->iqptr)
			asys->iqptr=new DnsSystem;
		DnsSystem *dns=(DnsSystem*)asys->iqptr;
		// Mark all for deletion
		if(Msg932Rec.ClientInfo.ListStart)
		{
			for(map<string,VARSIDREC*>::iterator vit=dns->varsIdMap.begin();vit!=dns->varsIdMap.end();vit++)
			{
				VARSIDREC *vrec=vit->second;
				vrec->Deleted=true;
			}
		}
		// Update and reset delete bit
		if((Msg932Rec.ClientInfo.DataItem)||(Msg932Rec.ClientInfo.UpdateItem))
		{
			char vkey[1024]={0};
			//sprintf(vkey,"%s_%08ld",Msg932Rec.VarsId.Domain,Msg932Rec.VarsId.Key);
			sprintf(vkey,"%s_%d",Msg932Rec.VarsId.Domain,Msg932Rec.VarsId.Key);
			map<string,VARSIDREC*>::iterator vit=dns->varsIdMap.find(vkey);
			if(vit==dns->varsIdMap.end())
			{
				VARSIDREC *vrec=new VARSIDREC;
				*vrec=Msg932Rec.VarsId;
				dns->varsIdMap[vkey]=vrec;
			}
			else
			{
				VARSIDREC *vrec=vit->second;
				// Check for delete bits from DB and delete if it's an update record - Bud 04/13/06 TD#NONEYEt
				if((Msg932Rec.ClientInfo.UpdateItem)&&(Msg932Rec.VarsId.DeletedFlags!=0))
				{
					delete vrec;
					dns->varsIdMap.erase(vit);
				}
				else
				{
					vrec->Deleted=false;
					*vrec=Msg932Rec.VarsId;
				}
			}
		}
		// Remove all with deleted bits set
		if(Msg932Rec.ClientInfo.ListEnd)
		{
			for(map<string,VARSIDREC*>::iterator vit=dns->varsIdMap.begin();vit!=dns->varsIdMap.end();)
			{
				VARSIDREC *vrec=vit->second;
				if(vrec->Deleted)
				{
					delete vrec;
					vit=dns->varsIdMap.erase(vit);
				}
				else
					vit++;
			}
		}
		break;
	}
	case 960: // UserEntitlement
	case 1960: // New user entitlement
	{
		MSG1960REC Msg1960Rec;
		if(MsgHeader->MsgID==1960)
		{
			if(MsgHeader->MsgLen<sizeof(MSG1960REC))
				return -1;
			memcpy(&Msg1960Rec,Msg,sizeof(MSG1960REC));
		}
		else
		{
			if(MsgHeader->MsgLen<sizeof(MSG960REC))
				return -1;
			memset(&Msg1960Rec,0,sizeof(MSG1960REC));
			memcpy(&Msg1960Rec,Msg,sizeof(MSG960REC));
		}

		CreateDnsUser(PortNo,Msg1960Rec);
		break;
	}
	case 964: // User account entitlement
	{
		USERACCREC Msg964Rec;
		if(MsgHeader->MsgLen<sizeof(USERACCREC))
			return -1;
		memcpy(&Msg964Rec,Msg,sizeof(USERACCREC));

		CreateDnsUserEnt(PortNo,Msg964Rec);
		break;
	}
	case 969: // Domain Tree Replicated with Logic Parent //REPLACED WITH 972
	{
		MSG969REC Msg969Rec;
		if(MsgHeader->MsgLen<sizeof(MSG969REC))
			return -1;
		memcpy(&Msg969Rec,Msg,sizeof(MSG969REC));

		AppInstance *pdom=CreateDnsDomain(PortNo,Msg969Rec.Domain);
		if(pdom)
		{
			IQDomain *iqd=(IQDomain*)pdom->iqptr;
			if(!iqd)
			{
				WSLogError("CreateDnsDomain() called, but iqptr is NULL");
				break;
			}
			memcpy(&iqd->Msg972Rec,&Msg969Rec,sizeof(MSG969REC));
			AppInstance *dparent=*Msg969Rec.LogicParent ?CreateDnsDomain(PortNo,Msg969Rec.LogicParent) :0;
			AppInstance *eparent=*Msg969Rec.Parent ?CreateDnsDomain(PortNo,Msg969Rec.Parent) :0;
			AssociateDnsDomains(pdom,dparent,eparent);
		}
		break;
	}
	case 972: // Domain Tree Replicated with Logic Parent and OmsServerName
	{
		MSG972REC Msg972Rec;
		if(MsgHeader->MsgLen<sizeof(MSG972REC))
			return -1;
		memcpy(&Msg972Rec,Msg,sizeof(MSG972REC));

		AppInstance *pdom=CreateDnsDomain(PortNo,Msg972Rec.Domain);
		if(pdom)
		{
			IQDomain *iqd=(IQDomain*)pdom->iqptr;
			memcpy(&iqd->Msg972Rec,&Msg972Rec,sizeof(MSG972REC));
			AppInstance *dparent=*Msg972Rec.LogicParent ?CreateDnsDomain(PortNo,Msg972Rec.LogicParent) :0;
			AppInstance *eparent=*Msg972Rec.Parent ?CreateDnsDomain(PortNo,Msg972Rec.Parent) :0;
			AssociateDnsDomains(pdom,dparent,eparent);
		}
		break;
	}
	case 970: // User document
	{
		MSG970REC Msg970Rec;
		if(MsgHeader->MsgLen<sizeof(MSG970REC))
			return -1;
		memcpy(&Msg970Rec,Msg,sizeof(MSG970REC));

		AppSystem *asys=GetAppSystem(Msg970Rec.UserDocRec.Domain);
		break;
	}
	case 971: // Domain document
	{
		MSG971REC Msg971Rec;
		if(MsgHeader->MsgLen<sizeof(MSG971REC))
			return -1;
		memcpy(&Msg971Rec,Msg,sizeof(MSG971REC));

		AppSystem *asys=GetAppSystem(Msg971Rec.DomainDocRec.Domain);
		break;
	}
	case 981: // Domain query
	{
		MSG981REC Msg981Rec;
		if(MsgHeader->MsgLen<sizeof(MSG981REC))
			return -1;
		memcpy(&Msg981Rec,Msg,sizeof(MSG981REC));

		AppSystem *asys=GetAppSystem(Msg981Rec.Domain);
		break;
	}
	case 997: // OmsServer IP Notification
	{
		MSG997REC Msg997Rec;
		if(MsgHeader->MsgLen<sizeof(MSG997REC))
			return -1;
		memcpy(&Msg997Rec,Msg,sizeof(MSG997REC));
		DNSINFOEX dnsi;
		memset(&dnsi,0,sizeof(DNSINFOEX));
		strcpy(dnsi.szHost,Msg997Rec.IP[0]);
		strncpy(dnsi.szDomain,Msg997Rec.OmsName,sizeof(dnsi.szDomain));
		dnsi.szDomain[sizeof(dnsi.szDomain)-1]=0;
		dnsi.wTypeServer=997;

		AppSystem *asys=GetAppSystem("CLSERVER*");
		if(!asys)
			return -1;
		if(!asys->iqptr)
			asys->iqptr=new DnsSystem;
		DnsSystem *dns=(DnsSystem*)asys->iqptr;
		char skey[1024]={0};
		sprintf(skey,"%s|%s|%d",dnsi.szHost,dnsi.szDomain,dnsi.wTypeServer);
		IQSMAP::iterator sit=dns->smap.find(skey);
		if(sit==dns->smap.end())
			dns->smap[skey]=dnsi;
		else
			sit->second=dnsi;
		break;
	}
	case 998: // Clserver login
	{
		MSG998REC Msg998Rec;
		if(MsgHeader->MsgLen<sizeof(MSG998REC))
			return -1;
		memcpy(&Msg998Rec,Msg,sizeof(MSG998REC));
		DNSINFOEX dnsi;
		memset(&dnsi,0,sizeof(DNSINFOEX));
		strcpy(dnsi.szHost,Msg998Rec.IP[0]);
		strncpy(dnsi.szDomain,Msg998Rec.Domain,sizeof(dnsi.szDomain));
		dnsi.szDomain[sizeof(dnsi.szDomain)-1]=0;
		dnsi.wTypeServer=998;

		AppSystem *asys=GetAppSystem("CLSERVER*");
		if(!asys)
			return -1;
		if(!asys->iqptr)
			asys->iqptr=new DnsSystem;
		DnsSystem *dns=(DnsSystem*)asys->iqptr;
		char skey[1024]={0};
		sprintf(skey,"%s|%s|%d",dnsi.szHost,dnsi.szDomain,dnsi.wTypeServer);
		IQSMAP::iterator sit=dns->smap.find(skey);
		if(sit==dns->smap.end())
			dns->smap[skey]=dnsi;
		else
			sit->second=dnsi;
		break;
	}
	case 999: // Remove user entitlements
	{
		MSG999REC Msg999Rec;
		if(MsgHeader->MsgLen<sizeof(MSG999REC))
			return -1;
		memcpy(&Msg999Rec,Msg,sizeof(MSG999REC));
		break;
	}
	case 1957: // Password expiration
	{
		break;
	}
	case 6314: // Twist domains, accounts, and users
	{
		map<string,string> tvmap;
		for(char *tok=strtok(Msg,",");tok;tok=strtok(0,","))
		{
			char *tag=tok,*val="";
			char *eptr=strchr(tok,'=');
			if(eptr)
			{
				*eptr=0; val=eptr+1;
			}
			tvmap[tag]=val;
		}
		map<string,string>::iterator tit=tvmap.find("cmd");
		if(tit!=tvmap.end())
		{
			// RTDOMAIN rows, not necessarily real domains in CL domain.ini
			if(tit->second=="AddDomain")
			{
			}
			// RTACCOUNT rows
			else if(tit->second=="AddAccount")
			{
				MSG632REC Msg632Rec;
				memset(&Msg632Rec,0,sizeof(MSG632REC));
				map<string,string>::iterator vit=tvmap.find("domain");
				if(vit!=tvmap.end())
					strncpy(Msg632Rec.AccountRec.Domain,vit->second.c_str(),sizeof(Msg632Rec.AccountRec.Domain));
				vit=tvmap.find("account");
				if(vit!=tvmap.end())
					strncpy(Msg632Rec.AccountRec.Account,vit->second.c_str(),sizeof(Msg632Rec.AccountRec.Account));
				vit=tvmap.find("Description");
				if(vit!=tvmap.end())
					strncpy(Msg632Rec.AccountRec.Notes,vit->second.c_str(),sizeof(Msg632Rec.AccountRec.Notes)-1);
				if((Msg632Rec.AccountRec.Domain[0])&&(Msg632Rec.AccountRec.Account[0]))
				{
					//char tKey[256]={0};
					//sprintf(tKey,"%16s,%16s",Msg632Rec.AccountRec.Domain,Msg632Rec.AccountRec.Account);
					//ACCOUNTMAP::iterator ait=AccountMap.find(tKey);
					//if(ait!=AccountMap.end())
					//{
					//	//MSG632REC *pAccountRec=&ait->second;
					//	//memcpy(pAccountRec,&Msg632Rec,sizeof(MSG632REC));
					//	if(_stricmp(ait->second.AccountRec.Domain,Msg632Rec.AccountRec.Domain))
					//		WSLogError("CON%d: Twist server reported account %s/%s, in conflict with existing account %s/%s",
					//			PortNo,Msg632Rec.AccountRec.Domain,Msg632Rec.AccountRec.Account,ait->second.AccountRec.Domain,ait->second.AccountRec.Account);
					//}
					//else
					//	AccountMap[tKey]=Msg632Rec;
					_strupr(Msg632Rec.AccountRec.Domain);
					_strupr(Msg632Rec.AccountRec.Account);
					#ifdef DNS_INSTANCES
					CreateRediAccount(PortNo,Msg632Rec);
					#else
					CreateDnsAccount(PortNo,Msg632Rec);
					#endif
				}
			}
			// RTUSER rows
			else if(tit->second=="AddUser")
			{
			}
		}
		break;
	}
	default:
	{
		_ASSERT(false);
		return -1;
	}
	};
	return 0;
}

int ViewServer::StartUsersQuery(VSQuery *pquery)
{
	const char *where=pquery->where.c_str();
	VSQueryScope& qscope=pquery->qscope;

	// Search all orders
	// Pre-process the where clause to narrow the search scope
	char reason[256]={0};
	if(pquery->NarrowScope(&pquery->qscope,where,this,reason)<0)
	{
		sprintf(reason,"Invalid 'where' clause.");
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	// User information only comes from CLSERVER system
	pquery->asys=GetAppSystem("CLSERVER*");
	if(!pquery->asys)
	{
		sprintf(reason,"No DNS information available.");
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	OrderDB *pdb=&pquery->asys->odb;
	pquery->ainst=0;
	pquery->puser=0;
	// Specific user
	if(qscope.CondUserName.size()>0)
	{
		ExprTok *etok=qscope.CondUserName.front();
		const char *UserName=qscope.GetEqStrVal(etok);
		if(qscope.CondUserDomain.size()!=1)
		{
			sprintf(reason,"'UserDomain' condition required with 'UserName' condition.");
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
		etok=qscope.CondUserDomain.front();
		const char *UserDomain=qscope.GetEqStrVal(etok);
		APPINSTMAP::iterator iit=pquery->asys->imap.find(UserDomain);
		if(iit==pquery->asys->imap.end())
		{
			sprintf(reason,"UserDomain(%s) not found.",UserDomain);
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
		pquery->ainst=iit->second;
		// Domain entitlement
		if(!IsEntitled(pquery->fuser,pquery->asys->sname.c_str(),pquery->ainst->iname.c_str()))
		{
			char reason[256]={0};
			sprintf(reason,"User not entitled to domain(%s/%s).",pquery->asys->sname.c_str(),pquery->ainst->iname.c_str());
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
		// Find the user
		IQDomain *iqd=(IQDomain*)pquery->ainst->iqptr;
		if(iqd)
		{
			char ukey[256]={0};
			strcpy(ukey,UserName); ukey[255]=0;
			_strlwr(ukey);
			pquery->uit=iqd->umap.find(ukey);
			if(pquery->uit!=iqd->umap.end())
				pquery->puser=pquery->uit->second;
		}
		if(pquery->puser)
		{
			pquery->eiter=0;
			pquery->FilterUserResult(pquery->puser,pquery->hist,this);
			int nenc=1;
			SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0);
			pquery->morehist=false;
			return 0;
		}
		else
		{
			sprintf(reason,"User(%s/%s) not found.",UserDomain,UserName);
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
	}

	// All users for specific domains
	WaitForSingleObject(asmux,INFINITE);
	if(qscope.CondUserDomain.size()>0)
	{
		for(CONDLIST::iterator cit=qscope.CondUserDomain.begin();cit!=qscope.CondUserDomain.end();cit++)
		{
			ExprTok *etok=*cit;
			const char *UserDomain=qscope.GetEqStrVal(etok);
			if(UserDomain)
			{
				APPINSTMAP::iterator iit=pquery->asys->imap.find(UserDomain);
				if(iit!=pquery->asys->imap.end())
				{
					AppInstance *ainst=iit->second;
					if(IsEntitled(pquery->fuser,pquery->asys->sname.c_str(),ainst->iname.c_str()))
						pquery->ailist.push_back(ainst);
				}
			}
		}
	}
	// All users for all domains
	else
	{
		for(APPINSTMAP::iterator iit=pquery->asys->imap.begin();iit!=pquery->asys->imap.end();iit++)
			pquery->ailist.push_back(iit->second);
	}
	ReleaseMutex(asmux);

	pquery->morehist=false;
	if(pquery->hist)
	{
		// We already found the single user
		if(pquery->puser)
			pquery->morehist=true;
		// Browse all users in domain list
		else
		{
			pquery->aiit=pquery->ailist.begin();
			if(pquery->aiit!=pquery->ailist.end())
			{
				pquery->ainst=*pquery->aiit;
				IQDomain *iqd=(IQDomain*)pquery->ainst->iqptr;
				if(iqd)
				{
					pquery->uit=iqd->umap.begin();
					if(pquery->uit!=iqd->umap.end())
						pquery->puser=pquery->uit->second;
				}
				pquery->morehist=true;
			}
		}
		if(!pquery->morehist)
		{
			pquery->eiter=0;
			int nenc=0;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
		pquery->spos=0; // 'supos' not used for SQL query
	}
	return 0;
}
// Virtual USERS table (colums are all attributes in VSOrder object)
int ViewServer::ContinueUsersQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				// Not every app instance has a user map
				// or we've reached the end of the current user map
				IQDomain *iqd=(IQDomain*)pquery->ainst->iqptr;
				if((!iqd)||(pquery->uit==iqd->umap.end()))
				{
					pquery->aiit++;
					if(pquery->aiit==pquery->ailist.end())
					{
						pquery->morehist=false;
						break;
					}
					pquery->ainst=*pquery->aiit;
					iqd=(IQDomain*)pquery->ainst->iqptr;
					if(iqd)
					{
						pquery->uit=iqd->umap.begin();
						if(pquery->uit!=iqd->umap.end())
							pquery->puser=pquery->uit->second;
					}
					continue;
				}
				_ASSERT(pquery->puser);
				if(pquery->FilterUser(pquery->puser,this)>0)
				{
					if(pquery->FilterUserResult(pquery->puser,pquery->hist,this)>0)
					{
						ndets++;
						// Send filled blocks as soon as possible
						if(ndets>=256)
						{
							int nenc=ndets;
							if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,true,0)>0)
							{
								tsent+=nenc; ndets-=nenc; needMoreEnd=true;
							}
							else
								ndets=0;
						}
					}
				}
				// User index
				pquery->uit++; pquery->spos++; pquery->scontinue++;
				if(pquery->uit!=iqd->umap.end())
					pquery->puser=pquery->uit->second;
			}
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}
	return 0;
}
int _stdcall _SendSqlUsersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	_ASSERT(false);//unfinished
	//int rc=pquery->pmod->SendSqlUsersLive(fuser,skey,taskid,rid,pquery,hint);
	//return rc;
	return 1;
}
// This function returns the value for one column in one row of the virtual USERS table
bool ViewServer::GetUserValue(const char *var, class ExprTok *vtok, void *hint)
{
	EvalHint *ehint=(EvalHint *)hint;
	IQUser *puser=ehint->puser;
	// MSG1960REC attributes
	if(!strcmp(var,"UserDomain"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,puser->Msg1960Rec.Domain,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"UserName"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,puser->Msg1960Rec.User,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Pro"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.Pro;
		return true;
	}
	else if(!strcmp(var,"VarsIdSet"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.VarsIdSet;
		return true;
	}
	else if(!strcmp(var,"EnableNonProVARSID"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.EnableNonProVARSID;
		return true;
	}
	else if(!strcmp(var,"PasswdLockOut"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.PasswdLockOut;
		return true;
	}
	else if(!strcmp(var,"EnhancedSecurity"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.EnhancedSecurity;
		return true;
	}
	else if(!strcmp(var,"UserDocAckNeeded"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.UserDocAckNeeded;
		return true;
	}
	else if(!strcmp(var,"UserAgreementNeeded"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.UserAgreementNeeded;
		return true;
	}
	else if(!strcmp(var,"UserDeleted"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.UserDeleted;
		return true;
	}
	else if(!strcmp(var,"FeedSuspended"))
	{
		vtok->vtype='B';
		vtok->bval=puser->Msg1960Rec.FeedSuspended;
		return true;
	}
	else if(!strcmp(var,"VarsId"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,puser->Msg1960Rec.VarsId,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Citizenship1"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,puser->Msg1960Rec.Citizenship1,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Citizenship2"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,puser->Msg1960Rec.Citizenship2,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"PasswordLifetime"))
	{
		vtok->vtype='S';
		sprintf(vtok->sval,"%d",puser->Msg1960Rec.PasswordLifetime);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"PwdExpireDate"))
	{
		vtok->vtype='S';
		sprintf(vtok->sval,"%08d",puser->Msg1960Rec.PwdExpireDate);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}	
	else if(!strcmp(var,"LastFeedIP"))
	{
		vtok->vtype='S';
		IN_ADDR ip;
		ip.s_addr=puser->LastFeedIP;
		sprintf(vtok->sval,"%s",inet_ntoa(ip));
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	return false;
}
// Evaluate where clause order conditions against a real user instance
int VSQuery::FilterUser(IQUser *puser, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={0,0,qscope.AuxKeyNames,0,puser};  //DT9398
	qscope.etok.EvalExprTree(0,&ehint);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause user conditions against the given user,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterUserResult(IQUser *puser, bool hist, ExprTokNotify *pnotify)
{
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[1024];
	memset(qresult->obuf,0,1024);
	char *rptr=qresult->obuf;
	const char *rend=rptr +1024;
	// Send all user attributes
	if(select=="*")
	{
		IN_ADDR ip;
		ip.s_addr=puser->LastFeedIP;
		sprintf(rptr,"%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%d\1%08d\1%s\1",
			// MSG1960REC attributes
			puser->Msg1960Rec.Domain,
			puser->Msg1960Rec.User,
			puser->Msg1960Rec.Pro?"TRUE":"FALSE",
			puser->Msg1960Rec.VarsIdSet?"TRUE":"FALSE",
			puser->Msg1960Rec.EnableNonProVARSID?"TRUE":"FALSE",
			puser->Msg1960Rec.PasswdLockOut?"TRUE":"FALSE",
			puser->Msg1960Rec.EnhancedSecurity?"TRUE":"FALSE",
			puser->Msg1960Rec.UserDocAckNeeded?"TRUE":"FALSE",
			puser->Msg1960Rec.UserAgreementNeeded?"TRUE":"FALSE",
			puser->Msg1960Rec.UserDeleted?"TRUE":"FALSE",
			puser->Msg1960Rec.FeedSuspended?"TRUE":"FALSE",
			puser->Msg1960Rec.VarsId,
			puser->Msg1960Rec.Citizenship1,
			puser->Msg1960Rec.Citizenship2,
			puser->Msg1960Rec.PasswordLifetime,
			puser->Msg1960Rec.PwdExpireDate,
			inet_ntoa(ip)
			);
		rptr+=strlen(rptr);
	}
	// Filter only desired attributes
	else
	{
		// Copy only the attributes in the select clause
		char sstr[1024]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			// MSG1960REC attributes
			if(!_stricmp(tok,"UserDomain"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.Domain); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"UserName"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.User); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Pro"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.Pro?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"VarsIdSet"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.VarsIdSet?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"EnableNonProVARSID"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.EnableNonProVARSID?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"PasswdLockOut"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.PasswdLockOut?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"EnhancedSecurity"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.EnhancedSecurity?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"UserDocAckNeeded"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.UserDocAckNeeded?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"UserAgreementNeeded"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.UserAgreementNeeded?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"UserDeleted"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.UserDeleted?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"FeedSuspended"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.FeedSuspended?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"VarsId"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.VarsId); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Citizenship1"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.Citizenship1); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Citizenship2"))
			{
				sprintf(rptr,"%s\1",puser->Msg1960Rec.Citizenship2); rptr+=strlen(rptr);
			}		
			else if(!strcmp(tok,"PasswordLifetime"))
			{
				sprintf(rptr,"%d\1",puser->Msg1960Rec.PasswordLifetime); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"PwdExpireDate"))
			{
				sprintf(rptr,"%08d\1",puser->Msg1960Rec.PwdExpireDate); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"LastFeedIP"))
			{
				IN_ADDR ip;
				ip.s_addr=puser->LastFeedIP;
				sprintf(rptr,"%s\1",inet_ntoa(ip)); rptr+=strlen(rptr);
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}

int ViewServer::StartVarsidQuery(VSQuery *pquery)
{
	const char *where=pquery->where.c_str();
	VSQueryScope& qscope=pquery->qscope;

	// Search all orders
	// Pre-process the where clause to narrow the search scope
	char reason[256]={0};
	if(pquery->NarrowScope(&pquery->qscope,where,this,reason)<0)
	{
		sprintf(reason,"Invalid 'where' clause.");
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	// Varsid information only comes from CLSERVER system
	pquery->asys=GetAppSystem("CLSERVER*");
	if((!pquery->asys)||(!pquery->asys->iqptr))
	{
		sprintf(reason,"No DNS information available.");
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	OrderDB *pdb=&pquery->asys->odb;
	DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
	pquery->ainst=0;
	pquery->puser=0;
	// Specific VARSID
	if(qscope.CondVarsidKey.size()>0)
	{
		ExprTok *etok=qscope.CondVarsidKey.front();
		const char *VarsidKey=qscope.GetEqStrVal(etok);
		// Domain entitlement
		if(!IsEntitled(pquery->fuser,pquery->asys->sname.c_str(),"VARSID"))
		{
			char reason[256]={0};
			sprintf(reason,"Varsid not entitled to domain(%s/VARSID).",pquery->asys->sname.c_str());
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
		// Find the varsid
		pquery->vit=dns->varsIdMap.find(VarsidKey);
		if(pquery->vit!=dns->varsIdMap.end())
			pquery->vrec=pquery->vit->second;
		if(pquery->vrec)
		{
			pquery->eiter=0;
			pquery->FilterVarsidResult(pquery->vrec,pquery->hist,this);
			int nenc=1;
			SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0);
			pquery->morehist=false;
			return 0;
		}
		else
		{
			sprintf(reason,"Varsid(%s) not found.",VarsidKey);
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
	}

	// All varsids
	pquery->morehist=false;
	if(pquery->hist)
	{
		// We already found the single user
		if(pquery->vrec)
			pquery->morehist=true;
		// Browse all users in domain list
		else
		{
			pquery->vit=dns->varsIdMap.begin();
			if(pquery->vit!=dns->varsIdMap.end())
			{
				pquery->vrec=pquery->vit->second;
				pquery->morehist=true;
			}
		}
		if(!pquery->morehist)
		{
			pquery->eiter=0;
			int nenc=0;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
		pquery->spos=0; // 'supos' not used for SQL query
	}
	return 0;
}
// Virtual VARSIDS table (colums are all attributes in VSOrder object)
int ViewServer::ContinueVarsidQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				// We've reached the end of the varsid map
				if(pquery->vit==dns->varsIdMap.end())
				{
					pquery->morehist=false;
					break;
				}
				_ASSERT(pquery->vrec);
				if(pquery->FilterVarsid(pquery->vrec,this)>0)
				{
					if(pquery->FilterVarsidResult(pquery->vrec,pquery->hist,this)>0)
					{
						ndets++;
						// Send filled blocks as soon as possible
						if(ndets>=256)
						{
							int nenc=ndets;
							if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,true,0)>0)
							{
								tsent+=nenc; ndets-=nenc; needMoreEnd=true;
							}
							else
								ndets=0;
						}
					}
				}
				// Varsid index
				pquery->vit++; pquery->spos++; pquery->scontinue++;
				if(pquery->vit!=dns->varsIdMap.end())
					pquery->vrec=pquery->vit->second;
			}
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}
	return 0;
}
int _stdcall _SendSqlVarsidLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	_ASSERT(false);//unfinished
	//int rc=pquery->pmod->SendSqlVarsidLive(fuser,skey,taskid,rid,pquery,hint);
	//return rc;
	return 1;
}
// This function returns the value for one column in one row of the virtual VARSID table
bool ViewServer::GetVarsidValue(const char *var, class ExprTok *vtok, void *hint)
{
	EvalHint *ehint=(EvalHint *)hint;
	VARSIDREC *vrec=ehint->vrec;
	// VARSIDREC attributes
	if(!strcmp(var,"Id"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Id,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Company"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Company,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Company2"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Company2,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Firm"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Firm,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"TaxId"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->TaxId,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"BrokerDealer"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->BrokerDealer,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"ShortName"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->ShortName,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"FirstName"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->FirstName,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"MiddleName"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->MiddleName,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"LastName"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->LastName,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Address1"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Address1,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Address2"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Address2,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Address3"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Address3,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Address4"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Address4,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"City"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->City,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"State"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->State,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Zip"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Zip,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Country"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Country,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Phone"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Phone,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"FAX"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->FAX,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Email"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->Email,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Created"))
	{
		vtok->vtype='S';
		sprintf(vtok->sval,"%08d",vrec->Created);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Modified"))
	{
		vtok->vtype='S';
		sprintf(vtok->sval,"%08d",vrec->Modified);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"ModifiedBy"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,vrec->ModifiedBy,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	return false;
}
// Evaluate where clause order conditions against a real user instance
int VSQuery::FilterVarsid(VARSIDREC *vrec, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={0,0,qscope.AuxKeyNames,0,0,vrec};  //DT9398
	qscope.etok.EvalExprTree(0,&ehint);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause user conditions against the given user,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterVarsidResult(VARSIDREC *vrec, bool hist, ExprTokNotify *pnotify)
{
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[1024];
	memset(qresult->obuf,0,1024);
	char *rptr=qresult->obuf;
	const char *rend=rptr +1024;
	// Send all user attributes
	if(select=="*")
	{
		sprintf(rptr,"%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%08d\1%08d\1%s\1",
			// VARSIDREC attributes
			vrec->Id,
			vrec->Company,
			vrec->Company2,
			vrec->Firm,
			vrec->TaxId,
			vrec->BrokerDealer,
			vrec->ShortName,
			vrec->FirstName,
			vrec->MiddleName,
			vrec->LastName,
			vrec->Address1,
			vrec->Address2,
			vrec->Address3,
			vrec->Address4,
			vrec->City,
			vrec->State,
			vrec->Zip,
			vrec->Country,
			vrec->Phone,
			vrec->FAX,
			vrec->Email,
			vrec->Created,
			vrec->Modified,
			vrec->ModifiedBy
			);
		rptr+=strlen(rptr);
	}
	// Filter only desired attributes
	else
	{
		// Copy only the attributes in the select clause
		char sstr[1024]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			// VARSIDREC attributes
			if(!strcmp(tok,"Id"))
			{
				sprintf(rptr,"%s\1",vrec->Id); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Company"))
			{
				sprintf(rptr,"%s\1",vrec->Company); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Company2"))
			{
				sprintf(rptr,"%s\1",vrec->Company2); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Firm"))
			{
				sprintf(rptr,"%s\1",vrec->Firm); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"TaxId"))
			{
				sprintf(rptr,"%s\1",vrec->TaxId); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"BrokerDealer"))
			{
				sprintf(rptr,"%s\1",vrec->BrokerDealer); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"ShortName"))
			{
				sprintf(rptr,"%s\1",vrec->ShortName); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"FirstName"))
			{
				sprintf(rptr,"%s\1",vrec->FirstName); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"MiddleName"))
			{
				sprintf(rptr,"%s\1",vrec->MiddleName); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"LastName"))
			{
				sprintf(rptr,"%s\1",vrec->LastName); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Address1"))
			{
				sprintf(rptr,"%s\1",vrec->Address1); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Address2"))
			{
				sprintf(rptr,"%s\1",vrec->Address2); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Address3"))
			{
				sprintf(rptr,"%s\1",vrec->Address3); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Address4"))
			{
				sprintf(rptr,"%s\1",vrec->Address4); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"City"))
			{
				sprintf(rptr,"%s\1",vrec->City); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"State"))
			{
				sprintf(rptr,"%s\1",vrec->State); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Zip"))
			{
				sprintf(rptr,"%s\1",vrec->Zip); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Country"))
			{
				sprintf(rptr,"%s\1",vrec->Country); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Phone"))
			{
				sprintf(rptr,"%s\1",vrec->Phone); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"FAX"))
			{
				sprintf(rptr,"%s\1",vrec->FAX); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Email"))
			{
				sprintf(rptr,"%s\1",vrec->Email); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Created"))
			{
				sprintf(rptr,"%08d\1",vrec->Created); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Modified"))
			{
				sprintf(rptr,"%08d\1",vrec->Modified); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"ModifiedBy"))
			{
				sprintf(rptr,"%s\1",vrec->ModifiedBy); rptr+=strlen(rptr);
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}

int ViewServer::StartDomrelQuery(VSQuery *pquery)
{
	const char *where=pquery->where.c_str();
	VSQueryScope& qscope=pquery->qscope;

	// Search all orders
	// Pre-process the where clause to narrow the search scope
	char reason[256]={0};
	if(pquery->NarrowScope(&pquery->qscope,where,this,reason)<0)
	{
		sprintf(reason,"Invalid 'where' clause.");
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	// Domrel information only comes from CLSERVER system
	pquery->asys=GetAppSystem("CLSERVER*");
	if((!pquery->asys)||(!pquery->asys->iqptr))
	{
		sprintf(reason,"No DNS information available.");
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	OrderDB *pdb=&pquery->asys->odb;
	DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
	// Domain entitlement
	if(!IsEntitled(pquery->fuser,pquery->asys->sname.c_str(),"DOMREL"))
	{
		sprintf(reason,"User not entitled to domain(%s/DOMREL).",pquery->asys->sname.c_str());
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}

	pquery->pdom=0;
	string DomainRoot="[ROOT]"; // default all domains
	if(qscope.CondDomainRoot.size()>0)
	{
		ExprTok *etok=qscope.CondDomainRoot.front();
		DomainRoot=qscope.GetEqStrVal(etok);
	}

	// All domains
	pquery->morehist=false;
	if(pquery->hist)
	{
		// Flattened domain list
		if(BFSEntDomains(pquery->dlist,pquery->asys,DomainRoot.c_str())<0)
		{
			sprintf(reason,"DomainRoot(%s/%s) not found.",pquery->asys->sname.c_str(),DomainRoot.c_str());
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
		pquery->dit=pquery->dlist.begin();
		if(pquery->dit!=pquery->dlist.end())
		{
			pquery->ainst=GetAppInstance(pquery->dit->c_str());
			if(pquery->ainst)
			{
				pquery->pdom=(IQDomain*)pquery->ainst->iqptr;
				pquery->morehist=true;
			}
		}
		if(!pquery->morehist)
		{
			pquery->eiter=0;
			int nenc=0;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
		pquery->spos=0; // 'supos' not used for SQL query
	}
	return 0;
}
// Virtual DOMREL table
int ViewServer::ContinueDomrelQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				// We've reached the end of the varsid map
				if(pquery->dit==pquery->dlist.end())
				{
					pquery->morehist=false;
					break;
				}
				_ASSERT(pquery->pdom);
				if(pquery->FilterDomrel(pquery->pdom,this)>0)
				{
					if(pquery->FilterDomrelResult(pquery->pdom,pquery->hist,this)>0)
					{
						ndets++;
						// Send filled blocks as soon as possible
						if(ndets>=256)
						{
							int nenc=ndets;
							if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,true,0)>0)
							{
								tsent+=nenc; ndets-=nenc; needMoreEnd=true;
							}
							else
								ndets=0;
						}
					}
				}
				// Domrel index
				pquery->dit++; pquery->spos++; pquery->scontinue++;
				if(pquery->dit!=pquery->dlist.end())
				{
					AppInstance *ainst=GetAppInstance(pquery->dit->c_str());
					pquery->pdom=ainst ?(IQDomain*)ainst->iqptr :0;
					_ASSERT(pquery->pdom);
				}
			}
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}
	return 0;
}
int _stdcall _SendSqlDomrelLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	_ASSERT(false);//unfinished
	//int rc=pquery->pmod->SendSqlDomrelLive(fuser,skey,taskid,rid,pquery,hint);
	//return rc;
	return 1;
}
// This function returns the value for one column in one row of the virtual VARSID table
bool ViewServer::GetDomrelValue(const char *var, class ExprTok *vtok, void *hint)
{
	EvalHint *ehint=(EvalHint *)hint;
	IQDomain *pdom=ehint->pdom;
	if(!strcmp(var,"EntParent"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,pdom->Msg972Rec.Parent,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Domain"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,pdom->Msg972Rec.Domain,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"RealParent"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,pdom->Msg972Rec.LogicParent,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	//else if(!strcmp(var,"OmsServer"))
	//{
	//	vtok->vtype='S';
	//	strncpy(vtok->sval,pdom->Msg972Rec.OmsServerName,sizeof(vtok->sval) -1);
	//	vtok->sval[sizeof(vtok->sval) -1]=0;
	//	return true;
	//}
	return false;
}
// Evaluate where clause order conditions against a real user instance
int VSQuery::FilterDomrel(IQDomain *pdom, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={0,0,qscope.AuxKeyNames,0,0,0,pdom}; //DT9398
	qscope.etok.EvalExprTree(0,&ehint);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause user conditions against the given domain,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterDomrelResult(IQDomain *pdom, bool hist, ExprTokNotify *pnotify)
{
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[256];
	memset(qresult->obuf,0,256);
	char *rptr=qresult->obuf;
	const char *rend=rptr +256;
	// Send all user attributes
	if(select=="*")
	{
		sprintf(rptr,"%s\1%s\1%s\1",
			pdom->Msg972Rec.Parent,
			pdom->Msg972Rec.Domain,
			pdom->Msg972Rec.LogicParent
			//pdom->Msg972Rec.OmsServerName
			);
		rptr+=strlen(rptr);
	}
	// Filter only desired attributes
	else
	{
		// Copy only the attributes in the select clause
		char sstr[256]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			if(!strcmp(tok,"EntParent"))
			{
				sprintf(rptr,"%s\1",pdom->Msg972Rec.Parent); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Domain"))
			{
				sprintf(rptr,"%s\1",pdom->Msg972Rec.Domain); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"RealParent"))
			{
				sprintf(rptr,"%s\1",pdom->Msg972Rec.LogicParent); rptr+=strlen(rptr);
			}
			//else if(!strcmp(tok,"OmsServer"))
			//{
			//	sprintf(rptr,"%s\1",pdom->Msg972Rec.OmsServerName); rptr+=strlen(rptr);
			//}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}
int ViewServer::StartDomtreeQuery(VSQuery *pquery)
{
	const char *where=pquery->where.c_str();
	VSQueryScope& qscope=pquery->qscope;

	// Search all orders
	// Pre-process the where clause to narrow the search scope
	char reason[256]={0};
	if(pquery->NarrowScope(&pquery->qscope,where,this,reason)<0)
	{
		sprintf(reason,"Invalid 'where' clause.");
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	// Domrel information only comes from CLSERVER system
	pquery->asys=GetAppSystem("CLSERVER*");
	if((!pquery->asys)||(!pquery->asys->iqptr))
	{
		sprintf(reason,"No DNS information available.");
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	OrderDB *pdb=&pquery->asys->odb;
	DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
	// Domain entitlement
	if(!IsEntitled(pquery->fuser,pquery->asys->sname.c_str(),"DOMREL"))
	{
		sprintf(reason,"User not entitled to domain(%s/DOMREL).",pquery->asys->sname.c_str());
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}

	pquery->pdom=0;
	string DomainRoot="[ROOT]"; // default all domains
	if(qscope.CondDomainRoot.size()>0)
	{
		ExprTok *etok=qscope.CondDomainRoot.front();
		DomainRoot=qscope.GetEqStrVal(etok);
	}

	// All domains
	pquery->morehist=false;
	if(pquery->hist)
	{
		// Flattened domain list
		if(BFSRealDomains(pquery->dlist,pquery->asys,DomainRoot.c_str())<0)
		{
			sprintf(reason,"DomainRoot(%s/%s) not found.",pquery->asys->sname.c_str(),DomainRoot.c_str());
			if(LOG_QUERIES>0)
				WSLogEvent("USR%d: %s",pquery->PortNo,reason);
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
			return -1;
		}
		pquery->dit=pquery->dlist.begin();
		if(pquery->dit!=pquery->dlist.end())
		{
			pquery->ainst=GetAppInstance(pquery->dit->c_str());
			if(pquery->ainst)
			{
				pquery->pdom=(IQDomain*)pquery->ainst->iqptr;
				pquery->morehist=true;
			}
		}
		if(!pquery->morehist)
		{
			pquery->eiter=0;
			int nenc=0;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
		pquery->spos=0; // 'supos' not used for SQL query
	}
	return 0;
}
// Virtual DOMTREE table
int ViewServer::ContinueDomtreeQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				// We've reached the end of the varsid map
				if(pquery->dit==pquery->dlist.end())
				{
					pquery->morehist=false;
					break;
				}
				_ASSERT(pquery->pdom);
				if(pquery->FilterDomtree(pquery->pdom,this)>0)
				{
					if(pquery->FilterDomtreeResult(pquery->pdom,pquery->hist,this)>0)
					{
						ndets++;
						// Send filled blocks as soon as possible
						if(ndets>=256)
						{
							int nenc=ndets;
							if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,true,0)>0)
							{
								tsent+=nenc; ndets-=nenc; needMoreEnd=true;
							}
							else
								ndets=0;
						}
					}
				}
				// Domtree index
				pquery->dit++; pquery->spos++; pquery->scontinue++;
				if(pquery->dit!=pquery->dlist.end())
				{
					AppInstance *ainst=GetAppInstance(pquery->dit->c_str());
					pquery->pdom=ainst ?(IQDomain*)ainst->iqptr :0;
					_ASSERT(pquery->pdom);
				}
			}
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}
	return 0;
}
int _stdcall _SendSqlDomtreeLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	_ASSERT(false);//unfinished
	//int rc=pquery->pmod->SendSqlDomtreeLive(fuser,skey,taskid,rid,pquery,hint);
	//return rc;
	return 1;
}
// This function returns the value for one column in one row of the virtual VARSID table
bool ViewServer::GetDomtreeValue(const char *var, class ExprTok *vtok, void *hint)
{
	return GetDomrelValue(var,vtok,hint);
}
// Evaluate where clause order conditions against a real user instance
int VSQuery::FilterDomtree(IQDomain *pdom, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={0,0,qscope.AuxKeyNames,0,0,0,pdom}; //DT9398
	qscope.etok.EvalExprTree(0,&ehint);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause user conditions against the given domain,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterDomtreeResult(IQDomain *pdom, bool hist, ExprTokNotify *pnotify)
{
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[256];
	memset(qresult->obuf,0,256);
	char *rptr=qresult->obuf;
	const char *rend=rptr +256;
	// Send all user attributes
	if(select=="*")
	{
		sprintf(rptr,"%s\1%s\1%s\1",
			pdom->Msg972Rec.LogicParent,
			pdom->Msg972Rec.Domain,
			pdom->Msg972Rec.Parent
			//pdom->Msg972Rec.OmsServerName
			);
		rptr+=strlen(rptr);
	}
	// Filter only desired attributes
	else
	{
		// Copy only the attributes in the select clause
		char sstr[256]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			if(!strcmp(tok,"RealParent"))
			{
				sprintf(rptr,"%s\1",pdom->Msg972Rec.LogicParent); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Domain"))
			{
				sprintf(rptr,"%s\1",pdom->Msg972Rec.Domain); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"EntParent"))
			{
				sprintf(rptr,"%s\1",pdom->Msg972Rec.Parent); rptr+=strlen(rptr);
			}
			//else if(!strcmp(tok,"OmsServer"))
			//{
			//	sprintf(rptr,"%s\1",pdom->Msg972Rec.OmsServerName); rptr+=strlen(rptr);
			//}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}

int ViewServer::BFSEntDomains(list<string>& dlist, AppSystem *asys, const char *Domain)
{
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	if(!dns)
		return -1;
	list<string> tlist;
	if(!strcmp(Domain,"[ROOT]"))
	{
		// Breadth-first search of root domains
		for(DOMRELMAP::iterator rit=dns->epmap.begin();rit!=dns->epmap.end();rit++)
		{
			AppInstance *ainst=rit->second;
			IQDomain *pdm=(IQDomain*)ainst->iqptr;
			if(pdm)
			{
				if((!*pdm->Msg972Rec.Parent)||(!_stricmp(pdm->Msg972Rec.Domain,pdm->Msg972Rec.Parent)))
				{
					dlist.push_back(ainst->iname);
					tlist.push_back(ainst->iname);
				}
			}
		}
	}
	else
	{
		if(dlist.empty())
			dlist.push_back(Domain);
		// Breadth-first search of all real children of the domain
		DOMRELMAP::iterator rit=dns->epmap.find(Domain);
		if(rit==dns->epmap.end())
			return -1;
		for(;
			(rit!=dns->epmap.end())&&(rit->first==Domain);
			rit++)
		{
			AppInstance *ainst=rit->second;
			if(ainst->iname==Domain) // Prevent infinite loops
				continue;
			dlist.push_back(ainst->iname);
			tlist.push_back(ainst->iname);
		}
	}
	for(list<string>::iterator tit=tlist.begin();tit!=tlist.end();tit++)
	{
		if(BFSEntDomains(dlist,asys,tit->c_str())<0)
			return -1;
	}
	return 0;
}
int ViewServer::BFSRealDomains(list<string>& dlist, AppSystem *asys, const char *Domain)
{
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	if(!dns)
		return -1;
	list<string> tlist;
	if(!strcmp(Domain,"[ROOT]"))
	{
		// Breadth-first search of root domains
		for(DOMRELMAP::iterator rit=dns->rpmap.begin();rit!=dns->rpmap.end();rit++)
		{
			AppInstance *ainst=rit->second;
			IQDomain *pdm=(IQDomain*)ainst->iqptr;
			if(pdm)
			{
				if((!*pdm->Msg972Rec.LogicParent)||(!_stricmp(pdm->Msg972Rec.Domain,pdm->Msg972Rec.LogicParent)))
				{
					dlist.push_back(ainst->iname);
					tlist.push_back(ainst->iname);
				}
			}
		}
	}
	else
	{
		if(dlist.empty())
			dlist.push_back(Domain);
		// Breadth-first search of all real children of the domain
		DOMRELMAP::iterator rit=dns->rpmap.find(Domain);
		if(rit==dns->rpmap.end())
			return -1;
		for(;
			(rit!=dns->rpmap.end())&&(rit->first==Domain);
			rit++)
		{
			AppInstance *ainst=rit->second;
			if(ainst->iname==Domain) // Prevent infinite loops
				continue;
			dlist.push_back(ainst->iname);
			tlist.push_back(ainst->iname);
		}
	}
	for(list<string>::iterator tit=tlist.begin();tit!=tlist.end();tit++)
	{
		if(BFSRealDomains(dlist,asys,tit->c_str())<0)
			return -1;
	}
	return 0;
}

int ViewServer::StartServersQuery(VSQuery *pquery)
{
	const char *where=pquery->where.c_str();
	VSQueryScope& qscope=pquery->qscope;

	// Search all orders
	// Pre-process the where clause to narrow the search scope
	char reason[256]={0};
	if(pquery->NarrowScope(&pquery->qscope,where,this,reason)<0)
	{
		sprintf(reason,"Invalid 'where' clause.");
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	// Domrel information only comes from CLSERVER system
	pquery->asys=GetAppSystem("CLSERVER*");
	if((!pquery->asys)||(!pquery->asys->iqptr))
	{
		sprintf(reason,"No DNS information available.");
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	OrderDB *pdb=&pquery->asys->odb;
	DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
	// Domain entitlement
	if(!IsEntitled(pquery->fuser,pquery->asys->sname.c_str(),"SERVERS"))
	{
		sprintf(reason,"User not entitled to domain(%s/DOMREL).",pquery->asys->sname.c_str());
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}

	pquery->psrv=0;
	//if(qscope.CondHost.size()>0)
	//{
	//	ExprTok *etok=qscope.CondHost.front();
	//	const char *Host=qscope.GetEqStrVal(etok);
	//	pquery->sit=dns->smap.find(Host);
	//	if(pquery->sit==dns->smap.end())
	//	{
	//		sprintf(reason,"Host(%s) not found.",Host);
	//		if(LOG_QUERIES>0)
	//			WSLogEvent("USR%d: %s",pquery->PortNo,reason);
	//		int nenc=0;
	//		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
	//		return -1;
	//	}
	//	pquery->psrv=&pquery->sit->second;
	//}

	pquery->morehist=false;
	if(pquery->hist)
	{
		// Single host
		if(pquery->psrv)
		{
			if(pquery->FilterServers(pquery->psrv,this)>0)
			{
				if(pquery->FilterServersResult(pquery->psrv,pquery->hist,this)>0)
				{
					int nenc=1;
					SendSqlResult(pquery,0,0,0,true,0,nenc,false,reason);
					return 0;
				}
			}
		}
		// All hosts
		else
		{
			pquery->sit=dns->smap.begin();
			if(pquery->sit!=dns->smap.end())
			{
				pquery->psrv=&pquery->sit->second;
				pquery->morehist=true;
			}
		}
		if(!pquery->morehist)
		{
			pquery->eiter=0;
			int nenc=0;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
		pquery->spos=0; // 'supos' not used for SQL query
	}
	return 0;
}
// Virtual DOMTREE table
int ViewServer::ContinueServersQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			DnsSystem *dns=(DnsSystem*)pquery->asys->iqptr;
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				// We've reached the end of the varsid map
				if(pquery->sit==dns->smap.end())
				{
					pquery->morehist=false;
					break;
				}
				_ASSERT(pquery->psrv);
				if(pquery->FilterServers(pquery->psrv,this)>0)
				{
					if(pquery->FilterServersResult(pquery->psrv,pquery->hist,this)>0)
					{
						ndets++;
						// Send filled blocks as soon as possible
						if(ndets>=256)
						{
							int nenc=ndets;
							if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,true,0)>0)
							{
								tsent+=nenc; ndets-=nenc; needMoreEnd=true;
							}
							else
								ndets=0;
						}
					}
				}
				// Servers index
				pquery->sit++; pquery->spos++; pquery->scontinue++;
				if(pquery->sit!=dns->smap.end())
					pquery->psrv=&pquery->sit->second;
			}
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}
	return 0;
}
int _stdcall _SendSqlServersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	_ASSERT(false);//unfinished
	//int rc=pquery->pmod->SendSqlServersLive(fuser,skey,taskid,rid,pquery,hint);
	//return rc;
	return 1;
}
// This function returns the value for one column in one row of the virtual SERVERS table
bool ViewServer::GetServerValue(const char *var, class ExprTok *vtok, void *hint)
{
	EvalHint *ehint=(EvalHint *)hint;
	DNSINFOEX *psrv=ehint->psrv;
	// DNSINFOEX attributes
	if(!strcmp(var,"Host"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,psrv->szHost,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Port"))
	{
		vtok->vtype='S';
		strncpy(vtok->sval,psrv->szPort,sizeof(vtok->sval) -1);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Cpu"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wCpu;
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Memory"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wMemory;
		return true;
	}
	else if(!strcmp(var,"TypeServer"))
	{
		//vtok->vtype='I';
		//vtok->ival=psrv->wTypeServer;
		vtok->vtype='S';
		switch(psrv->wTypeServer)
		{
		case 21: strcpy(vtok->sval,"IQS\1"); break;
		case 997: strcpy(vtok->sval,"OMS\1"); break;
		case 998: strcpy(vtok->sval,"CLS\1"); break;
		default: strcpy(vtok->sval,"unknown\1"); 
		};
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Nrusers"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wNrusers;
		return true;
	}
	else if(!strcmp(var,"OS"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wOS;
		return true;
	}
	else if(!strcmp(var,"Iqversion"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wIqversion;
		return true;
	}
	else if(!strcmp(var,"Nropenquotes"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wNropenquotes;
		return true;
	}
	else if(!strcmp(var,"Domain"))
	{
		vtok->vtype='S';
		sprintf(vtok->sval,"%d",psrv->wCpu);
		vtok->sval[sizeof(vtok->sval) -1]=0;
		return true;
	}
	else if(!strcmp(var,"Iqversion"))
	{
		vtok->vtype='I';
		vtok->ival=psrv->wIqversion;
		return true;
	}
	return false;
}
// Evaluate where clause order conditions against a real user instance
int VSQuery::FilterServers(DNSINFOEX *psrv, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={0,0,qscope.AuxKeyNames,0,0,0,0,0,psrv}; //DT9398
	qscope.etok.EvalExprTree(0,&ehint);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause user conditions against the given domain,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterServersResult(DNSINFOEX *psrv, bool hist, ExprTokNotify *pnotify)
{
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[256];
	memset(qresult->obuf,0,256);
	char *rptr=qresult->obuf;
	const char *rend=rptr +256;
	// Send all server attributes
	if(select=="*")
	{
		const char *tstr="";
		switch(psrv->wTypeServer)
		{
		case 21: tstr="IQS"; break;
		case 997: tstr="OMS"; break;
		case 998: tstr="CLS"; break;
		default: tstr="unknown"; 
		};
		sprintf(rptr,"%s\1%s\1%d\1%d\1%s\1%d\1%d\1%d\1%d\1%s\1%d\1",
			psrv->szHost,
			psrv->szPort,
			psrv->wCpu,
			psrv->wMemory,
			tstr,
			psrv->wNrusers,
			psrv->wOS,
			psrv->wIqversion,
			psrv->wNropenquotes,
			psrv->szDomain,
			psrv->wIqversion);
		rptr+=strlen(rptr);
	}
	// Filter only desired attributes
	else
	{
		// Copy only the attributes in the select clause
		char sstr[256]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			if(!strcmp(tok,"Host"))
			{
				sprintf(rptr,"%s\1",psrv->szHost); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Port"))
			{
				sprintf(rptr,"%s\1",psrv->szPort); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Cpu"))
			{
				sprintf(rptr,"%d\1",psrv->wCpu); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Memory"))
			{
				sprintf(rptr,"%d\1",psrv->wMemory); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"TypeServer"))
			{
				switch(psrv->wTypeServer)
				{
				case 21: strcpy(rptr,"IQS\1"); break;
				case 997: strcpy(rptr,"OMS\1"); break;
				case 998: strcpy(rptr,"CLS\1"); break;
				default: strcpy(rptr,"unknown\1"); 
				};
				rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Nrusers"))
			{
				sprintf(rptr,"%d\1",psrv->wNrusers); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"OS"))
			{
				sprintf(rptr,"%d\1",psrv->wOS); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Iqversion"))
			{
				sprintf(rptr,"%d\1",psrv->wIqversion); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Nropenquotes"))
			{
				sprintf(rptr,"%d\1",psrv->wNropenquotes); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Domain"))
			{
				sprintf(rptr,"%s\1",psrv->szDomain); rptr+=strlen(rptr);
			}
			else if(!strcmp(tok,"Iqversion"))
			{
				sprintf(rptr,"%d\1",psrv->wIqversion); rptr+=strlen(rptr);
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}

int ViewServer::SAMReportLine(FILE *fp, IQUser *puser, const USERACCREC& uacc, const char *Domain, bool iqsam)
{
	if(!iqsam)
	{
		if(*uacc.Account=='@') // specific domain entitlement
			return 0;
		else if(*uacc.Account=='#') // groups?
			return 0;
		else if(*uacc.Account=='^') // OMS server entitlement
			return 0;
	}
	char astr[32]={0};
	if(strchr(uacc.Account,','))
		sprintf(astr,"\"%s\"",uacc.Account);
	else
		strcpy(astr,uacc.Account);
	fprintf(fp,"%s,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d,\n",
		puser->Msg1960Rec.User,
		puser->Msg1960Rec.Domain,
		Domain,
		astr,
		uacc.UserAccEnts.Trader,
		uacc.UserAccEnts.ViewTrading,
		uacc.UserAccEnts.ViewAccount,
		uacc.UserAccEnts.CanAllocate,
		uacc.UserAccEnts.ViewAllocations,
		uacc.UserAccEnts.CanSetCommision,
		uacc.UserAccEnts.CanAllocateToAnyAcc);
	return 1;
}
int ViewServer::SAMReport(char fpath[MAX_PATH], bool upload, const char *Domain, const char *User)
{
	sprintf(fpath,"%s\\data\\sam_%08d.csv",pcfg->RunPath.c_str(),WSDate);
	FILE *fp=fopen(fpath,"wt");
	if(!fp)
	{
		WSLogError("Failed opening (%s) for SAM report!",fpath);
		return -1;
	}

	bool explain=false;
	if((Domain)&&(*Domain)&&(User)&&(*User))
		explain=true;

	WaitForSingleObject(asmux,INFINITE);
	_ASSERT(fp);
	fprintf(fp,"User,UserDomain,AccountDomain,Account,PlaceTrades,ViewRemoteTrading,ViewAccountBP,AllowAllocations,ViewAllocations,SetCommissions,AllocateToAnyAccount,\n");
	int nusers=0,nents=0;
	// All IQ domains
	AppSystem *asys=GetAppSystem("CLSERVER*");
	if(!asys)
	{
		WSLogError("IQSAM report missing AppSystem(CLSERVER).");
		ReleaseMutex(asmux);
		return -1;
	}
	else if(!asys->iqptr)
	{
		WSLogError("No IQ CLSERVER domain info received from DNS.");
		ReleaseMutex(asmux);
		return -1;
	}
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	int nerr=0;
	char tbuf[1024]={0};
	// Walk all domains
	bool once=false;
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		AppInstance *ainst=iit->second;
		IQDomain *iqd=(IQDomain*)ainst->iqptr;
		if(!iqd)
			continue;
		if((Domain)&&(*Domain)&&(_stricmp(iqd->Msg972Rec.Domain,Domain)))
			continue;
		// Walk all users
		for(USERMAP::iterator uit=iqd->umap.begin();uit!=iqd->umap.end();uit++)
		{
			IQUser *puser=uit->second;
			if((User)&&(*User)&&(_stricmp(puser->Msg1960Rec.User,User)))
				continue;
			//if(strincmp(puser->Msg1960Rec.User,"nbk",3))// Only NBK
			//	continue;
			nusers++;
			// First, build a list of domains entitled for the user
			list<string> dlist;
			if(explain)
			{
				sprintf(tbuf,"Building domain list for User(%s/%s)...",
					puser->Msg1960Rec.Domain,puser->Msg1960Rec.User);
				fprintf(fp,"NOTE: %s\n",tbuf);
			}
			dlist.push_back(puser->Msg1960Rec.Domain);
			if(explain)
			{
				sprintf(tbuf,"Domain(%s) added as default",puser->Msg1960Rec.Domain);
				fprintf(fp,"NOTE: %s\n",tbuf);
			}
			for(list<USERACC>::iterator acit=puser->AccountList.begin();acit!=puser->AccountList.end();acit++)
			{
				USERACC& uacc=*acit;
				if(*uacc.UserAccRec.Account=='@') // specific domain entitlement
				{
					// If there's an @ entitlement, then the user's domain is excluded
					if(!once)
					{
						once=true;
						if((!dlist.empty())&&(dlist.front()==puser->Msg1960Rec.Domain))
						{
							dlist.pop_front();
							if(explain)
							{
								sprintf(tbuf,"Domain(%s) removed by UserEnt(%s/%s/%s)",
									puser->Msg1960Rec.Domain,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
								fprintf(fp,"NOTE: %s\n",tbuf);
							}
						}
					}
					// All sub-domains of and including the user's domain
					if(!_stricmp(uacc.UserAccRec.Account,"@ALL"))
					{
						list<string> tlist;
						if(BFSEntDomains(tlist,asys,puser->Msg1960Rec.Domain)<0)
						{
							sprintf(tbuf,"Failed recursing Domain(%s)!",puser->Msg1960Rec.Domain); nerr++;
							WSLogEvent("SAM report: %s",tbuf);
							if(explain)
								fprintf(fp,"ERROR: %s\n",tbuf);
							_ASSERT(false);
						}
						else
						{
							for(list<string>::iterator tit=tlist.begin();tit!=tlist.end();tit++)
							{
								dlist.push_back(*tit);
								if(explain)
								{
									sprintf(tbuf,"Domain(%s) added by UserEnt(%s/%s/%s)",
										tit->c_str(),uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
									fprintf(fp,"NOTE: %s\n",tbuf);
								}
							}
							tlist.clear();
						}
					}
					// All sub-domains of the specified one
					else if(!strncmp(uacc.UserAccRec.Account,"@>",2))
					{
						DOMRELMAP::iterator rit=dns->rpmap.find(uacc.UserAccRec.Account +2);
						if(rit==dns->rpmap.end())
						{
							sprintf(tbuf,"Missing @>Domain(%s) for UserEnt(%s/%s/%s)",
								uacc.UserAccRec.Account +2,uacc.UserAccRec.User,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
							WSLogEvent("SAM report: %s",tbuf);
							if(explain)
								fprintf(fp,"NOTE: %s\n",tbuf);
						}
						else
						{
							list<string> tlist;
							if(BFSEntDomains(tlist,asys,uacc.UserAccRec.Account +2)<0)
							{
								sprintf(tbuf,"Failed recursing Domain(%s) for UserEnt(%s/%s/%s)",
									uacc.UserAccRec.Account +2,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User); nerr++;
								WSLogEvent("SAM report: %s",tbuf);
								if(explain)
									fprintf(fp,"ERROR: %s\n",tbuf);
							}
							else
							{
								for(list<string>::iterator tit=tlist.begin();tit!=tlist.end();tit++)
								{
									// Don't include the @domain
									if(_stricmp(tit->c_str(),uacc.UserAccRec.Account +2))
									{
										dlist.push_back(*tit);
										if(explain)
										{
											sprintf(tbuf,"Domain(%s) added due to UserEnt(%s/%s/%s)",
												tit->c_str(),uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
											fprintf(fp,"NOTE: %s\n",tbuf);
										}
									}
								}
								tlist.clear();
							}
						}
					}
					// Only the specified @domain
					else
					{
						DOMRELMAP::iterator rit=dns->rpmap.find(uacc.UserAccRec.Account +1);
						if(rit==dns->rpmap.end())
						{
							sprintf(tbuf,"Missing @Domain(%s) for UserEnt(%s/%s/%s)",
								uacc.UserAccRec.Account +1,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
							WSLogEvent("SAM report: %s",tbuf);
							if(explain)
								fprintf(fp,"NOTE: %s\n",tbuf);
						}
						else
						{
							dlist.push_back(uacc.UserAccRec.Account +1);
							if(explain)
							{
								sprintf(tbuf,"Domain(%s) added due to UserEnt(%s/%s/%s)",
									uacc.UserAccRec.Account +1,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
								fprintf(fp,"NOTE: %s\n",tbuf);
							}
						}
					}
				}
			}

			// Second, expand #SUPER and #groups accounts for all domains and
			// validate single accounts on each domain
			list<USERACC> alist;
			for(list<string>::iterator dit=dlist.begin();dit!=dlist.end();dit++)
			{
				const char *Domain=dit->c_str();
				if(explain)
				{
					sprintf(tbuf,"Building Domain(%s) account list...",Domain);
					fprintf(fp,"NOTE: %s\n",tbuf);
				}
				APPINSTMAP::iterator iit=asys->imap.find(Domain);
				if((iit==asys->imap.end())||(!iit->second->iqptr))
				{
					sprintf(tbuf,"Missing Domain(%s)",Domain);
					WSLogEvent("SAM report: %s",tbuf);
					if(explain)
						fprintf(fp,"NOTE: %s\n",tbuf);
					continue;
				}
				IQDomain *iqd=(IQDomain*)iit->second->iqptr;
				for(list<USERACC>::iterator acit=puser->AccountList.begin();acit!=puser->AccountList.end();acit++)
				{
					USERACC& uacc=*acit;
					if(*uacc.UserAccRec.Account=='@') // already handled
						;
					else if(*uacc.UserAccRec.Account=='#') // groups
					{
						// Every account in this domain
						if((!_stricmp(uacc.UserAccRec.Account,"#SUPER"))||(!_stricmp(uacc.UserAccRec.Account,"#SUPERD")))
						{
							for(ACCTMAP::iterator acit=iqd->acmap.begin();acit!=iqd->acmap.end();acit++)
							{
								ACCOUNTREC *arec=(ACCOUNTREC*)acit->second->iqptr;
								if(!arec)
									continue;
								// Override with #SUPER bits
								USERACC vacc=uacc;
								strcpy(vacc.UserAccRec.Domain,Domain);
								strcpy(vacc.UserAccRec.Account,arec->Account);
								alist.push_back(vacc);
								if(explain)
								{
									sprintf(tbuf,"Added all Account(%s/%s) for UserEnt(%s/%s/%s)",
										vacc.UserAccRec.Domain,vacc.UserAccRec.Account,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
									fprintf(fp,"NOTE: %s\n",tbuf);
								}
							}
						}
						// Expand #group accounts for this domain.
						else
						{
							// Add all accounts under the entitled domain that have the account group membership
							bool found=false;
							for(ACCTMAP::iterator acit=iqd->acmap.begin();acit!=iqd->acmap.end();acit++)
							{
								ACCOUNTREC *arec=(ACCOUNTREC*)acit->second->iqptr;
								if(!arec)
									continue;
								for(int i=0;i<5;i++)
								{
									if(*arec->Group[i])
									{
										if(!_stricmp(arec->Group[i],uacc.UserAccRec.Account +1))
										{
											found=true;
											// Override with #group bits
											USERACC vacc=uacc;
											strcpy(vacc.UserAccRec.Domain,Domain);
											strcpy(vacc.UserAccRec.Account,arec->Account);
											alist.push_back(vacc);
											if(explain)
											{
												sprintf(tbuf,"Added group Account(%s/%s) for UserEnt(%s/%s/%s)",
													vacc.UserAccRec.Domain,vacc.UserAccRec.Account,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
												fprintf(fp,"NOTE: %s\n",tbuf);
											}
											break;
										}
									}
								}
							}
							if(!found)
							{
								sprintf(tbuf,"Missing Group(%s/%s) for UserEnt(%s/%s/%s)",
									Domain,uacc.UserAccRec.Account,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
								WSLogEvent("SAM report: %s",tbuf);
								if(explain)
									fprintf(fp,"NOTE: %s\n",tbuf);
							}
						}
					}
					// Single account entitlement
					else
					{
						ACCTMAP::iterator acit=iqd->acmap.find(uacc.UserAccRec.Account);
						// Pre-configured accounts aren't IQ accounts
						if((acit==iqd->acmap.end())||(!acit->second->iqptr))
						{
							if(explain)
							{
								sprintf(tbuf,"Missing Account(%s/%s) for UserEnt(%s/%s/%s)",
									Domain,uacc.UserAccRec.Account,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
								fprintf(fp,"NOTE: %s\n",tbuf);
							}
						}
						else
						{
							ACCOUNTREC *arec=(ACCOUNTREC*)acit->second->iqptr;
							USERACC vacc=uacc;
							strcpy(vacc.UserAccRec.Domain,Domain);
							strcpy(vacc.UserAccRec.Account,arec->Account);
							alist.push_back(vacc);
							if(explain)
							{
								sprintf(tbuf,"Added Account(%s/%s) for UserEnt(%s/%s/%s)",
									Domain,uacc.UserAccRec.Account,uacc.UserAccRec.Domain,uacc.UserAccRec.Account,uacc.UserAccRec.User);
								fprintf(fp,"NOTE: %s\n",tbuf);
							}
						}
					}
				}
			}
			dlist.clear();

			// Third, report account entitlements
			if(explain)
			{
				sprintf(tbuf,"Reporting %d entitlements for User(%s/%s)...",
					alist.size(),puser->Msg1960Rec.Domain,puser->Msg1960Rec.User);
				fprintf(fp,"NOTE: %s\n",tbuf);
			}
			for(list<USERACC>::iterator acit=alist.begin();acit!=alist.end();acit++)
			{
				USERACC& uacc=*acit;
				if(SAMReportLine(fp,puser,uacc.UserAccRec,uacc.UserAccRec.Domain,false))
					nents++;
			}
			alist.clear();
		}
	}
	ReleaseMutex(asmux);
	fprintf(fp,"\nTotal %d users, %d account entitlements, %d errors\n",nusers,nents,nerr);
	fclose(fp);
	WSLogEvent("SAM reported to (%s).",fpath);

	if(upload)
	{
		FileOptions fopts=WSFileOptions;
		fopts.waitTimeout=3;
		WIN32_FIND_DATA fdata;
		memset(&fdata,0,sizeof(WIN32_FIND_DATA));
		HANDLE fhnd=FindFirstFile(fpath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
			FindClose(fhnd);
		char rpath[MAX_PATH]={0};
		sprintf(rpath,"%s\\sam_%08d.csv",SAM_UPLOAD_DIR.c_str(),WSDate);
		if(!WSTransmitFile(fpath,rpath,&fdata.ftCreationTime,&fdata.ftLastAccessTime,&fdata.ftLastWriteTime,&fopts))
			WSLogEvent("Failed uploading (%s) to file server.",rpath);
		else
			WSLogEvent("Uploaded user file (%s) %d bytes",rpath,fdata.nFileSizeLow);	
	}
	return 0;
}
// Non-expanded form for internal IQ use
int ViewServer::IQSAMReport(char fpath[MAX_PATH], bool upload, const char *Domain, const char *User)
{
	sprintf(fpath,"%s\\data\\iqsam_%08d.csv",pcfg->RunPath.c_str(),WSDate);
	FILE *fp=fopen(fpath,"wt");
	if(!fp)
	{
		WSLogError("Failed opening (%s) for IQSAM report!",fpath);
		return -1;
	}

	WaitForSingleObject(asmux,INFINITE);
	_ASSERT(fp);
	fprintf(fp,"User,UserDomain,AccountDomain,Account,PlaceTrades,ViewRemoteTrading,ViewAccountBP,AllowAllocations,ViewAllocations,SetCommissions,AllocateToAnyAccount,\n");
	int nusers=0,nents=0;
	// All IQ domains
	AppSystem *asys=GetAppSystem("CLSERVER*");
	if(!asys)
	{
		WSLogError("IQSAM report missing AppSystem(CLSERVER).");
		ReleaseMutex(asmux);
		return -1;
	}
	else if(!asys->iqptr)
	{
		WSLogError("No IQ CLSERVER domain info received from DNS.");
		ReleaseMutex(asmux);
		return -1;
	}
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	int nerr=0;
	set<string> mset;
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		AppInstance *ainst=iit->second;
		IQDomain *iqd=(IQDomain*)ainst->iqptr;
		if(!iqd)
			continue;
		if((Domain)&&(*Domain)&&(_stricmp(iqd->Msg972Rec.Domain,Domain)))
			continue;
		for(USERMAP::iterator uit=iqd->umap.begin();uit!=iqd->umap.end();uit++)
		{
			IQUser *puser=uit->second;
			if((User)&&(*User)&&(_stricmp(puser->Msg1960Rec.User,User)))
				continue;
			nusers++;
			// Report accounts for user
			const char *Domain=puser->Msg1960Rec.Domain;
			for(list<USERACC>::iterator acit=puser->AccountList.begin();acit!=puser->AccountList.end();acit++)
			{
				USERACC& uacc=*acit;
				if(SAMReportLine(fp,puser,uacc.UserAccRec,Domain,true))
					nents++;
			}
		}
	}
	ReleaseMutex(asmux);
	fprintf(fp,"\nTotal %d users, %d account entitlements, %d errors\n",nusers,nents,nerr);
	fclose(fp);
	WSLogEvent("IQSAM reported to (%s).",fpath);

	if(upload)
	{
		FileOptions fopts=WSFileOptions;
		fopts.waitTimeout=3;
		WIN32_FIND_DATA fdata;
		memset(&fdata,0,sizeof(WIN32_FIND_DATA));
		HANDLE fhnd=FindFirstFile(fpath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
			FindClose(fhnd);
		char rpath[MAX_PATH]={0};
		sprintf(rpath,"%s\\iqsam_%08d.csv",SAM_UPLOAD_DIR.c_str(),WSDate);
		if(!WSTransmitFile(fpath,rpath,&fdata.ftCreationTime,&fdata.ftLastAccessTime,&fdata.ftLastWriteTime,&fopts))
			WSLogEvent("Failed uploading (%s) to file server.",rpath);
		else
			WSLogEvent("Uploaded user file (%s) %d bytes",rpath,fdata.nFileSizeLow);	
	}
	return 0;
}

#if defined(_DEBUG)&&defined(IQDEVTEST)
int ViewServer::RediSqlInsertDnsDomains(FILE *fp)
{
	AppSystem *asys=GetAppSystem("CLSERVER*");
	if((!asys)||(!asys->iqptr))
		return -1;
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	// Temporary map of entitlement parent to child domains
	typedef multimap<string,IQDomain*> DMAP;
	DMAP dmap;
	short ndomid=20;
	map<string,int> firms;
	short nfid=1003;
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		DMAP::iterator dit;
		for(dit=dmap.find(pdom->Msg972Rec.Parent);
			(dit!=dmap.end())&&(dit->first==pdom->Msg972Rec.Parent)&&(_stricmp(dit->second->Msg972Rec.Parent,pdom->Msg972Rec.Domain)<=0);
			dit++)
			;
		if(dit==dmap.end())
			dmap.insert(pair<string,IQDomain*>(pdom->Msg972Rec.Parent,pdom));
		else
			dmap.insert(dit,pair<string,IQDomain*>(pdom->Msg972Rec.Parent,pdom));
		// Assign domain id
		short domid=ndomid++;
		char *pdomid=pdom->Msg972Rec.Domain +strlen(pdom->Msg972Rec.Domain) +1;
		memcpy(pdomid,&domid,2);
		// Lookup/assign firm id
		short firmid;
		const char *firm=pdom->Msg972Rec.Parent[0] ?pdom->Msg972Rec.Parent :pdom->Msg972Rec.Domain;
		map<string,int>::iterator fit=firms.find(firm);
		if(fit==firms.end())
		{
			firmid=++nfid;
			firms.insert(pair<string,int>(firm,firmid));
		}
		else
			firmid=fit->second;
		char *pfirm=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(pfirm,&firmid,2);
	}
	// Domain list
	fprintf(fp,"\nIQDOMAIN.csv\n");
	fprintf(fp,"RowKey,Id,Name,FirmId,ParentDomainId,AggUnitId,IsDeleted,\n");
	for(DMAP::iterator dit=dmap.begin();dit!=dmap.end();dit++)
	{
		IQDomain *pdom=dit->second;
		// Read domain id
		short domid;
		char *pdomain=pdom->Msg972Rec.Domain +strlen(pdom->Msg972Rec.Domain) +1;
		memcpy(&domid,pdomain,2);
		// Read firm id
		short firmid;
		char *pfirm=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pfirm,2);
		// Look up parent domain ids
		short parentid=0;
		if((pdom->Msg972Rec.Parent[0])&&(stricmp(pdom->Msg972Rec.Parent,pdom->Msg972Rec.Domain)))
		{
			for(DMAP::iterator pdit=dmap.begin();pdit!=dmap.end();pdit++)
			{
				IQDomain *parent=pdit->second;
				if(!parent) continue;
				if(!stricmp(parent->Msg972Rec.Domain,pdom->Msg972Rec.Parent))
				{
					char *pparentid=parent->Msg972Rec.Domain +strlen(parent->Msg972Rec.Domain) +1;
					memcpy(&parentid,pparentid,2);
					break;
				}
			}
		}
		//fprintf(fp,"INSERT INTO RTDOMAIN_TRANS(Id,Name,FirmId,ParentDomainId,AggUnitId)\n\tVALUES (%d,'%s',1003,%d,'%s')\n",
		//	domid,pdom->Msg972Rec.Domain,parentid,pdom->Msg972Rec.Domain);
		fprintf(fp,"%s.%d,%d,%s,%d,%d,%s,0,\n",
			pdom->Msg972Rec.Domain,firmid,domid,pdom->Msg972Rec.Domain,firmid,parentid,pdom->Msg972Rec.Domain);
	}
	// Domain's AggUnit
	fprintf(fp,"\nIQAGGUNIT.csv\n");
	fprintf(fp,"RowKey,AggUnitSynonym,AggUnitSynonymType,AggUnitType,FirmId,IsManual,IsPositionEligible,CreateUser,ModifiedUser,\n");
	for(DMAP::iterator dit=dmap.begin();dit!=dmap.end();dit++)
	{
		IQDomain *pdom=dit->second;
		// Read domain id
		short domid;
		char *pdomain=pdom->Msg972Rec.Domain +strlen(pdom->Msg972Rec.Domain) +1;
		memcpy(&domid,pdomain,2);
		// Read firm id
		short firmid;
		char *pfirm=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pfirm,2);
		fprintf(fp,"%s.1.%d,%s,1,3,%d,0,1,%s,%s,\n",
			pdom->Msg972Rec.Domain,firmid,pdom->Msg972Rec.Domain,firmid,"u705758","u705758");
	}
	// Parent Domains
	fprintf(fp,"\nIQFIRMS.csv\n");
	fprintf(fp,"firm,clr_firm,qsr_ind,clr_ind,attach_to_ind,backoff_dest,oe_clr_rpt,trade_rpt_ind,EndOfQuery,\n");
	for(map<string,int>::iterator fit=firms.begin();fit!=firms.end();fit++)
	{
		fprintf(fp,"%d,,,,,,,,,\n",fit->second);
	}
	return 0;
}
int ViewServer::RediSqlInsertDnsAccounts(FILE *fp)
{
	AppSystem *asys=GetAppSystem("CLSERVER*");
	if((!asys)||(!asys->iqptr))
		return -1;
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	// Account list
	fprintf(fp,"\nIQACCOUNT.csv\n");
	fprintf(fp,"RowKey,AccountSynonym,RtFirmId,DomainId,AccountId,CompanyCode,DtcBrkName,PrimaryContact,ReferenceNumber,IsDeleted,\n");
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		short firmid=1003;
		char *pparent=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pparent,2);
		short domid=0;
		char *pdomid=pdom->Msg972Rec.Domain +strlen(pdom->Msg972Rec.Domain) +1;
		memcpy(&domid,pdomid,2);
		for(ACCTMAP::iterator ait=pdom->acmap.begin();ait!=pdom->acmap.end();ait++)
		{
			ACCOUNTREC *arec=(ACCOUNTREC *)ait->second->iqptr;
			if(!arec) continue;
			//fprintf(fp,"INSERT INTO RTACCOUNT_TRANS(AccountSynonym,RtFirmId,DomainId,AccountId,DtcBrkName,IsDeleted)\n\tVALUES ('%s','2',%d,'%s')\n",
			//	arec->Account,domid,arec->LegalName);
			char aname[64]={0},*aptr=aname;
			for(char *asptr=arec->Account;*asptr;asptr++)
			{
				char ch=*asptr;
				if(ch=='.') ch=' ';
				else ch=toupper(ch);
				*aptr=ch; aptr++;
			}
			*aptr=0;
			char entity[256]={0};
			strcpy(entity,arec->Entity[0] ?arec->Entity :arec->LastName);
			while(strchr(entity,','))
				*strchr(entity,',')='.';
			fprintf(fp,"%s,%s,%d,%d,%s,%s,%s,%s,%s,0,\n",
				aname,aname,firmid,domid,aname,entity,arec->LegalName,
				arec->TelOffice[0] ?arec->TelOffice :arec->TelHome,arec->SSNId);
		}
	}
	// Domain/Account mapping
	fprintf(fp,"\nIQAGGUNITTOACCTMPG.csv\n");
	fprintf(fp,"RowKey,FirmId,AggUnitSynonym,AggUnitSynonymType,RowKeyAggUnit,AccountSynonym,AccountSynonymType,IsManual,\n");
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		short firmid=1003;
		char *pparent=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pparent,2);
		short domid=0;
		char *pdomid=pdom->Msg972Rec.Domain +strlen(pdom->Msg972Rec.Domain) +1;
		memcpy(&domid,pdomid,2);
		for(ACCTMAP::iterator ait=pdom->acmap.begin();ait!=pdom->acmap.end();ait++)
		{
			ACCOUNTREC *arec=(ACCOUNTREC *)ait->second->iqptr;
			if(!arec) continue;
			char aname[64]={0},*aptr=aname;
			for(char *asptr=arec->Account;*asptr;asptr++)
			{
				char ch=*asptr;
				if(ch=='.') ch=' ';
				else ch=toupper(ch);
				*aptr=ch; aptr++;
			}
			*aptr=0;
			fprintf(fp,"%s.1.%s.1,%d,%s,1,%s.1.%d,%s,1,0,\n",
				pdom->Msg972Rec.Domain,aname,firmid,pdom->Msg972Rec.Domain,pdom->Msg972Rec.Domain,firmid,aname);
		}
	}
	return 0;
}
struct ACGROUP
{
	short id;
	short firmid;
};
int ViewServer::RediSqlInsertDnsUsers(FILE *fp)
{
	AppSystem *asys=GetAppSystem("CLSERVER*");
	if((!asys)||(!asys->iqptr))
		return -1;
	DnsSystem *dns=(DnsSystem*)asys->iqptr;
	// Domain Users
	fprintf(fp,"\nIQUSER.csv\n");
	fprintf(fp,"UserId,FirmId,FirmName,ActivationDate,CreationDate,IsSupervisor,IsModel,Status,CustomerOf,REDIPlus,\n");
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		short firmid=1003;
		char *pparent=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pparent,2);
		for(USERMAP::iterator uit=pdom->umap.begin();uit!=pdom->umap.end();uit++)
		{
			IQUser *puser=uit->second;
			char uname[40]={0};
			sprintf(uname,"iq%s",puser->Msg1960Rec.User);
			fprintf(fp,"%s,%d,InstaQuote,CURRENT_TIMESTAMP,CURRENT_TIMESTAMP,0,0,Y,%s,N,\n",
				uname,firmid,puser->Msg1960Rec.Domain);
		}
	}
	// Account entitlements
	fprintf(fp,"\nIQAGGUNITTOUSER.csv\n");
	fprintf(fp,"RowKey,UserId,RowKeyAggUnit,ModifiedTime,\n");
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		short firmid=1003;
		char *pparent=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pparent,2);
		for(USERMAP::iterator uit=pdom->umap.begin();uit!=pdom->umap.end();uit++)
		{
			IQUser *puser=uit->second;
			char uname[40]={0};
			sprintf(uname,"iq%s",puser->Msg1960Rec.User);
			fprintf(fp,"%s.%s.1.%d,%s,%s.1.%d,CURRENT_TIMESTAMP,\n",
				uname,pdom->Msg972Rec.Domain,firmid,uname,pdom->Msg972Rec.Domain,firmid);
		}
	}
	// Account group list
	fprintf(fp,"\nIQACCOUNTGROUP.csv\n");
	//fprintf(fp,"Id,GroupName,FirmId,IsDeleted,ModifiedTime\n");
	map<string,ACGROUP> groups;
	int id=1;
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		short firmid=1003;
		char *pparent=pdom->Msg972Rec.Parent +strlen(pdom->Msg972Rec.Parent) +1;
		memcpy(&firmid,pparent,2);
		for(ACCTMAP::iterator ait=pdom->acmap.begin();ait!=pdom->acmap.end();ait++)
		{
			ACCOUNTREC *arec=(ACCOUNTREC *)ait->second->iqptr;
			if(!arec) continue;
			for(int i=0;i<5;i++)
			{
				if(arec->Group[i][0])
				{
					char agkey[256]={0};
					sprintf(agkey,"%s.%s",pdom->Msg972Rec.Parent,arec->Group[i]);
					if(groups.find(agkey)==groups.end())
					{
						ACGROUP acg={++id,firmid};
						groups.insert(pair<string,ACGROUP>(agkey,acg));
					}
				}
			}
		}
	}
	fprintf(fp,"Id,GroupName,FirmId,IsDeleted,ModifiedTime,\n");
	for(map<string,ACGROUP>::iterator git=groups.begin();git!=groups.end();git++)
	{
		//fprintf(fp,"INSERT INTO RTACCOUNTGROUP(Id,GroupName,FirmId,IsDeleted,ModifiedTime) VALUES(%d,'%s',%d,0,CURRENT_TIMESTAMP)\n",
		//	git->second.id,git->first.c_str(),git->second.firmid);
		fprintf(fp,"%d,'%s',%d,0,CURRENT_TIMESTAMP,\n",git->second.id,git->first.c_str(),git->second.firmid);
	}
	// Account groups
	fprintf(fp,"\nIQACCTGRPMPG.csv\n");
	fprintf(fp,"RowKey,AccountGroupId,AccountSynonym,IsDeleted,ModifiedTime,\n");
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		for(ACCTMAP::iterator ait=pdom->acmap.begin();ait!=pdom->acmap.end();ait++)
		{
			ACCOUNTREC *arec=(ACCOUNTREC *)ait->second->iqptr;
			if(!arec) continue;
			for(int i=0;i<5;i++)
			{
				if(arec->Group[i][0])
				{
					char agkey[256]={0};
					sprintf(agkey,"%s.%s",pdom->Msg972Rec.Parent,arec->Group[i]);
					int gid=0;
					map<string,ACGROUP>::iterator git=groups.find(agkey);
					if(git!=groups.end())
						gid=git->second.id;
					//fprintf(fp,"INSERT INTO RTACCTGRPMPG(RowKey,AccountGroupId,AccountSynonym,IsDeleted,ModifiedTime) VALUES(%s,%d,%s,0,CURRENT_TIMESTAMP)\n",
					//	arec->Group[i],gid,arec->Group[i]);
					fprintf(fp,"%s,%d,%s,0,CURRENT_TIMESTAMP,\n",
						arec->Account,gid,arec->Account);
				}
			}
		}
	}
	// Account group entitlement
	fprintf(fp,"\nIQUSERACCTGRP.csv\n");
	fprintf(fp,"RowKey,UserId,AccountGroupId,IsDeleted,ModifiedTime,\n");
	for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	{
		IQDomain *pdom=(IQDomain*)iit->second->iqptr;
		if(!pdom) continue;
		for(USERMAP::iterator uit=pdom->umap.begin();uit!=pdom->umap.end();uit++)
		{
			IQUser *puser=uit->second;
			for(list<USERACC>::iterator ait=puser->AccountList.begin();ait!=puser->AccountList.end();ait++)
			{
				USERACC& uacc=*ait;
				if(uacc.UserAccRec.Account[0]!='#')
					continue;
				int gid=0;
				char agkey[256]={0};
				sprintf(agkey,"%s.%s",pdom->Msg972Rec.Parent,uacc.UserAccRec.Account+1);
				map<string,ACGROUP>::iterator git=groups.find(agkey);
				if(git!=groups.end())
				{
					gid=git->second.id;
					fprintf(fp,"iq%s.%d,iq%s,%d,0,CURRENT_TIMESTAMP,\n",
						puser->Msg1960Rec.User,gid,puser->Msg1960Rec.User,gid);
				}
			}
		}
	}
	// Account entitlements
	fprintf(fp,"\nIQENT.csv\n");
	fprintf(fp,"Key,Type,UserID,Value1,Value2,Action,\n");
	// Use the SAM report instead, which has already expanded all #SUPER and #group entitlements
	//for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
	//{
	//	IQDomain *pdom=(IQDomain*)iit->second->iqptr;
	//	if(!pdom) continue;
	//	for(USERMAP::iterator uit=pdom->umap.begin();uit!=pdom->umap.end();uit++)
	//	{
	//		IQUser *puser=uit->second;
	//		for(list<USERACC>::iterator ait=puser->AccountList.begin();ait!=puser->AccountList.end();ait++)
	//		{
	//			USERACC& uacc=*ait;
	//			if((uacc.UserAccRec.Account[0]=='#')||(uacc.UserAccRec.Account[0]=='@'))
	//				continue;
	//			fprintf(fp,"ACCT.iq%s.%s,ACCT,iq%s,%s,%s,,\n",
	//				puser->Msg1960Rec.User,uacc.UserAccRec.Account,puser->Msg1960Rec.User,uacc.UserAccRec.Account,uacc.UserAccRec.Account);
	//		}
	//	}
	//}
	FILE *rfp=fopen("SysmonTempFiles\\sam.csv","rt");
	if(rfp)
	{
		char rbuf[1024]={0};
		while(fgets(rbuf,sizeof(rbuf),rfp))
		{
			const char *user="",*udomain="",*adomain="",*account="";
			int col=0;
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)			
				{
				case 1: user=tok; break;
				case 2: udomain=tok; break;
				case 3: adomain=tok; break;
				case 4: account=tok; break;
				};
				if(col>=4)
					break;
			}
			fprintf(fp,"ACCT.iq%s.%s,ACCT,iq%s,%s,%s,,\n",
				user,account,user,account,account);
		}
		fclose(rfp);
	}
	return 0;
}
#endif
#endif//IQDNS

