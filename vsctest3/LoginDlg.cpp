// LoginDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vsctest3.h"
#include "LoginDlg.h"
#include ".\logindlg.h"
#include "wsocksapi.h"

#pragma pack(push,1)

struct CREDREC
{
	WORD marker;
	char dest[64];
	char user[32];
	char pass[32];
	int lastDate;
	int lastTime;
};
struct CREDREC2	:public CREDREC
{
	char tunnel[256];
};
struct CREDREC3	:public CREDREC2
{
	char gateway[32];
};

#pragma pack(pop)

CREDMAP CLoginDlg::credmap;
int CREDMAP::GetDestAlias(const char *cdest, string& dest, string& alias)
{
	char tdest[80]={0};
	strcpy(tdest,cdest);
	char *aptr=strchr(tdest,'-');
	if(aptr)
	{
		*aptr=0;
		for(char *dptr=aptr -1;(dptr>dest)&&(isspace(*dptr));dptr--)
			*dptr=0;
		for(aptr++;(*aptr)&&(isspace(*aptr));aptr++)
			;
		alias=aptr;
	}
	dest=tdest;
	return 0;
}
int CREDMAP::Save()
{
	FILE *fp=fopen("setup\\cred.dat","wb");
	if(!fp)
		return -1;
	for(CREDMAP::iterator cit=begin();cit!=end();cit++)
	{
		CRED& cred=cit->second;
		CREDREC3 crec;
		memset(&crec,0,sizeof(CREDREC3));
		crec.marker=0xFCFC;
		strncpy(crec.dest,cred.dest.c_str(),sizeof(crec.dest));
		strncpy(crec.user,cred.user.c_str(),sizeof(crec.user));
		strncpy(crec.pass,cred.pass.c_str(),sizeof(crec.pass));
		crec.lastDate=cred.lastDate;
		crec.lastTime=cred.lastTime;
		strncpy(crec.tunnel,cred.tunnel.c_str(),sizeof(crec.tunnel));
		strncpy(crec.gateway,cred.gateway.c_str(),sizeof(crec.gateway));
		fwrite(&crec,1,sizeof(CREDREC3),fp);
	}
	fclose(fp);
	return 0;
}
int CREDMAP::Load()
{
	FILE *fp=fopen("setup\\cred.dat","r+b");
	if(!fp)
		return -1;
	CREDREC3 crec;
	while(fp)
	{
		if(fread(&crec,1,sizeof(CREDREC),fp)==sizeof(CREDREC))
		{
			if(crec.marker==0xFEFE)
			{
				CRED cred;
				cred.dest=crec.dest;
				cred.user=crec.user;
				cred.pass=crec.pass;
				cred.lastDate=crec.lastDate;
				cred.lastTime=crec.lastTime;
				string dest,alias;
				GetDestAlias(cred.dest.c_str(),dest,alias);
				(*this)[dest]=cred;
			}
			else if(crec.marker==0xFDFD)
			{
				fread(&crec.tunnel,1,sizeof(crec.tunnel),fp);
				CRED cred;
				cred.dest=crec.dest;
				cred.user=crec.user;
				cred.pass=crec.pass;
				cred.lastDate=crec.lastDate;
				cred.lastTime=crec.lastTime;
				cred.tunnel=crec.tunnel;
				string dest,alias;
				GetDestAlias(cred.dest.c_str(),dest,alias);
				(*this)[dest]=cred;
			}
			else if(crec.marker==0xFCFC)
			{
				fread(&crec.tunnel,1,sizeof(crec.tunnel),fp);
				fread(&crec.gateway,1,sizeof(crec.gateway),fp);
				CRED cred;
				cred.dest=crec.dest;
				cred.user=crec.user;
				cred.pass=crec.pass;
				cred.lastDate=crec.lastDate;
				cred.lastTime=crec.lastTime;
				cred.tunnel=crec.tunnel;
				cred.gateway=crec.gateway;
				string dest,alias;
				GetDestAlias(cred.dest.c_str(),dest,alias);
				(*this)[dest]=cred;
			}
			else
			{
				fclose(fp);
				char spath[MAX_PATH]={0},dpath[MAX_PATH]={0},*sptr=0,*dptr=0;
				GetFullPathName("setup\\cred.dat",sizeof(spath),spath,&sptr);
				GetFullPathName("setup\\cred.dat.bak",sizeof(dpath),dpath,&dptr);
				CopyFile(spath,dpath,false);
				return -1;
			}
		}
		else
			break;
	}
	fclose(fp);
	return 0;
}

// CLoginDlg dialog
IMPLEMENT_DYNAMIC(CLoginDlg, CDialog)
CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
	, m_user("")
	, m_pass("")
	, m_server("&Server ip:port - alias\nLast connect:")
	, m_selalias()
	, m_tunnel("")
	, m_gateway("")
{
}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USER, m_user);
	DDX_Text(pDX, IDC_PASS, m_pass);
	DDX_Text(pDX, IDC_SERVER, m_server);
	DDX_Control(pDX, IDC_IPADDR, m_ipaddr);
	DDX_Text(pDX, IDC_TUNNEL, m_tunnel);
	DDX_Text(pDX, IDC_GATEWAY, m_gateway);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
//	ON_WM_CREATE()
ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
ON_CBN_SELCHANGE(IDC_IPADDR, OnCbnSelchangeIpaddr)
ON_CBN_EDITCHANGE(IDC_IPADDR, OnCbnEditchangeIpaddr)
ON_BN_CLICKED(IDC_DELETE, OnBnClickedDelete)
END_MESSAGE_MAP()


// CLoginDlg message handlers
BOOL CLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Load destinations
	//string selstr=theApp.lastdest;
	//for(int i=0;i<NO_OF_CON_PORTS;i++)
	//{
	//	if(!ConPort[i].InUse)
	//		continue;
	//	if((ConPort[i].AltRemoteIPOn[0])&&(ConPort[i].Port))
	//	{
	//		char dest[32]={0};
	//		sprintf(dest,"%s:%d",ConPort[i].AltRemoteIP[0],ConPort[i].Port);
	//		m_ipaddr.AddString(dest);
	//		if(selstr.empty())
	//			selstr=dest;
	//		// Pre-load empty one for new destinations
	//		if(credmap.find(dest)==credmap.end())
	//		{
	//			CRED cred;
	//			cred.dest=dest;
	//			cred.user="";
	//			cred.pass="";
	//			credmap[cred.dest]=cred;
	//		}
	//	}
	//}
	//credmap.Save();
	CRED *scred=0;
	credmap.Load();
	for(int i=0;i<theApp.NO_OF_CON_PORTS;i++)
	{
		if(!theApp.ConPort[i].InUse)
			continue;
		if(theApp.ConPort[i].SockConnected)
			theApp.ConPort[i].ConnectHold=true;
	}

	for(CREDMAP::iterator cit=credmap.begin();cit!=credmap.end();cit++)
	{
		CRED& cred=cit->second;
		m_ipaddr.AddString(cred.dest.c_str());
		// Select credential by alias name
		if(!m_selalias.IsEmpty())
		{
			const char *alias=strchr(cred.dest.c_str(),'-');
			for(;(alias)&&((*alias=='-')||(isspace(*alias)));alias++)
				;
			if((alias)&&(!m_selalias.CompareNoCase(alias)))
				scred=&cred;
		}
		// Last used credential
		else
		{
			if((!scred)||(cred.lastDate>scred->lastDate)||
			   ((cred.lastDate==scred->lastDate)&&(cred.lastTime>scred->lastTime)))
				scred=&cred;
		}
	}

	// Select credential from cache
	if(scred)
	{
		m_ipaddr.SelectString(-1,scred->dest.c_str());
		m_tunnel=scred->tunnel.c_str();
		m_gateway=scred->gateway.c_str();
		OnCbnSelchangeIpaddr();
	}
	UpdateData(false);

	if((scred)&&(!m_selalias.IsEmpty()))
		PostMessage(WM_COMMAND,IDOK);
	return 0;
}

void CLoginDlg::OnBnClickedOk()
{
	UpdateData(true);
	// Set login credentials
	theApp.USER=m_user;
	theApp.PASS=m_pass;

	// Add to credential cache
	CString cdest;
	m_ipaddr.GetWindowText(cdest);
	CRED cred;
	cred.dest=cdest;
	string dest,alias;
	CREDMAP::GetDestAlias(cdest,dest,alias);
	cred.user=m_user;
	cred.pass=m_pass;
	cred.lastDate=theApp.WSDate;
	cred.lastTime=theApp.WSTime;
	cred.tunnel=m_tunnel;
	cred.gateway=m_gateway;
	credmap[dest]=cred;
	credmap.Save();
	//theApp.lastdest=dest;

	// Add to extras
	//int selidx=m_ipaddr.FindString(-1,dest);
	//if(selidx<0)
	//	m_ipaddr.AddString(dest);
	int i=0;
	for(i=0;i<theApp.NO_OF_CON_PORTS;i++)
	{
		if(!theApp.ConPort[i].InUse)
			continue;
		if(theApp.ConPort[i].SockConnected)
			theApp.WSCloseConPort(i);
		//// Gateway connection
		//if(!m_gateway.IsEmpty())
		//{
		//	strncpy(theApp.ConPort[i].AltRemoteIP[0],(const char *)m_gateway,sizeof(theApp.ConPort[i].AltRemoteIP));
		//	theApp.ConPort[i].AltRemoteIPOn[0]=1;
		//	char *iptr=strchr(theApp.ConPort[i].AltRemoteIP[0],':');
		//	if(iptr) *iptr=0;
		//	theApp.ConPort[i].Port=atoi(iptr+1);
		//	theApp.ConPort[i].EncryptionOn=1;
		//	theApp.ConPort[i].Encrypted=2;
		//	theApp.ConPort[i].Compressed=false;
		//	sprintf(theApp.ConPort[i].CfgNote,"$IQATS$");
		//}
		//// Smproxy tunnel
		//if(!m_tunnel.IsEmpty())
		//{
		//	char dpath[256]={0},tnext[64]={0};
		//	sprintf(dpath,"%s/%s",m_tunnel,dest.c_str());
		//	if(strcmp(theApp.ConPort[i].CfgNote,"$IQATS$")!=0)
		//	{
		//		char *tptr=strchr(dpath,'/');
		//		if(tptr)
		//		{
		//			*tptr=0; strcpy(tnext,dpath);
		//			memmove(dpath,tptr+1,strlen(tptr+1)+1);
		//		}
		//		strncpy(theApp.ConPort[i].AltRemoteIP[0],tnext,sizeof(theApp.ConPort[i].AltRemoteIP));
		//		char *iptr=strchr(theApp.ConPort[i].AltRemoteIP[0],':');
		//		if(iptr) *iptr=0;
		//		theApp.ConPort[i].AltRemoteIPOn[0]=1;
		//		theApp.ConPort[i].Port=atoi(iptr+1);
		//		theApp.ConPort[i].EncryptionOn=0;
		//		theApp.ConPort[i].Encrypted=0;
		//		theApp.ConPort[i].Compressed=true;
		//	}
		//	sprintf(theApp.ConPort[i].CfgNote +strlen(theApp.ConPort[i].CfgNote),"$TUNNEL$%s",dpath);
		//}
		//// Direct connection
		//if(!theApp.ConPort[i].CfgNote[0])
		//{
		//	strncpy(theApp.ConPort[i].AltRemoteIP[0],dest.c_str(),sizeof(theApp.ConPort[i].AltRemoteIP));
		//	char *iptr=strchr(theApp.ConPort[i].AltRemoteIP[0],':');
		//	if(iptr) *iptr=0;
		//	theApp.ConPort[i].AltRemoteIPOn[0]=1;
		//	theApp.ConPort[i].Port=atoi(iptr+1);
		//	theApp.ConPort[i].EncryptionOn=0;
		//	theApp.ConPort[i].Encrypted=0;
		//	theApp.ConPort[i].Compressed=true;
		//}
		//else
		//	strcpy(theApp.ConPort[i].CfgNote +strlen(theApp.ConPort[i].CfgNote),dest.c_str());
		//theApp.ConPort[i].ConnectHold=false;
		char dpath[1024]={0},*dptr=dpath;
		if(!m_gateway.IsEmpty())
		{
			sprintf(dptr,"$IQATS$%s/",m_gateway); dptr+=strlen(dptr);
		}
		if(!m_tunnel.IsEmpty())
		{
			sprintf(dptr,"$TUNNEL$%s/",m_tunnel); dptr+=strlen(dptr);
		}
		sprintf(dptr,"%s",dest.c_str()); dptr+=strlen(dptr);

		dptr=strchr(dpath,'/');
		if(dptr) 
		{
			*dptr=0; dptr++;
		}
		else 
			dptr=dpath +strlen(dpath);		
		// Gateway first
		if(!strncmp(dpath,"$IQATS$",7))
		{
			strncpy(theApp.ConPort[i].AltRemoteIP[0],(const char *)dpath+7,sizeof(theApp.ConPort[i].AltRemoteIP));
			theApp.ConPort[i].AltRemoteIPOn[0]=1;
			char *iptr=strchr(theApp.ConPort[i].AltRemoteIP[0],':');
			if(iptr) 
			{
				*iptr=0; theApp.ConPort[i].Port=atoi(iptr+1);
			}
			theApp.ConPort[i].EncryptionOn=1;
			theApp.ConPort[i].Encrypted=2;
			theApp.ConPort[i].Compressed=false;
			sprintf(theApp.ConPort[i].CfgNote,"$IQATS$%s",dptr);
		}
		// Tunnel first
		else if(!strncmp(dpath,"$TUNNEL$",8))
		{
			strncpy(theApp.ConPort[i].AltRemoteIP[0],(const char *)dpath+8,sizeof(theApp.ConPort[i].AltRemoteIP));
			theApp.ConPort[i].AltRemoteIPOn[0]=1;
			char *iptr=strchr(theApp.ConPort[i].AltRemoteIP[0],':');
			if(iptr) 
			{
				*iptr=0; theApp.ConPort[i].Port=atoi(iptr+1);
			}
			theApp.ConPort[i].EncryptionOn=0;
			theApp.ConPort[i].Encrypted=0;
			theApp.ConPort[i].Compressed=true;
			sprintf(theApp.ConPort[i].CfgNote,"$TUNNEL$%s",dptr);
		}
		// Direct only
		else
		{
			strncpy(theApp.ConPort[i].AltRemoteIP[0],(const char *)dpath,sizeof(theApp.ConPort[i].AltRemoteIP));
			theApp.ConPort[i].AltRemoteIPOn[0]=1;
			char *iptr=strchr(theApp.ConPort[i].AltRemoteIP[0],':');
			if(iptr)
			{
				*iptr=0; theApp.ConPort[i].Port=atoi(iptr+1);
			}
			theApp.ConPort[i].EncryptionOn=0;
			theApp.ConPort[i].Encrypted=0;
			theApp.ConPort[i].Compressed=true;
			strcpy(theApp.ConPort[i].CfgNote,"");
		}
		theApp.ConPort[i].ConnectHold=false;

		OnOK();
		break;
	}
	if(i>=theApp.NO_OF_CON_PORTS)
		MessageBox("No CON ports available for connection!","Login Failed",MB_ICONERROR);
}

void CLoginDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void CLoginDlg::OnCbnSelchangeIpaddr()
{
	int selidx=m_ipaddr.GetCurSel();
	if(selidx>=0)
	{
		CString selstr;
		m_ipaddr.GetLBText(selidx,selstr);
		string dest,alias;
		CREDMAP::GetDestAlias(selstr,dest,alias);
		CREDMAP::iterator cit=credmap.find(dest);
		CRED cred;
		if(cit!=credmap.end())
			cred=cit->second;
		m_user=cred.user.c_str();
		m_pass=cred.pass.c_str();
		m_server.Format("&Server ip:port - alias\nLast connect: %08d-%02d:%02d:%02d",
			cred.lastDate,cred.lastTime/10000,(cred.lastTime%10000)/100,cred.lastTime%100);
		m_tunnel=cred.tunnel.c_str();
		m_gateway=cred.gateway.c_str();
		UpdateData(false);
	}
}

void CLoginDlg::OnCbnEditchangeIpaddr()
{
	CString selstr;
	m_ipaddr.GetWindowText(selstr);
	string dest,alias;
	CREDMAP::GetDestAlias(selstr,dest,alias);
	CREDMAP::iterator cit=credmap.find(dest);
	CRED cred;
	if(cit!=credmap.end())
		cred=cit->second;
	m_user=cred.user.c_str();
	m_pass=cred.pass.c_str();
	m_tunnel=cred.tunnel.c_str();
	m_gateway=cred.gateway.c_str();
	UpdateData(false);
}

void CLoginDlg::OnBnClickedDelete()
{
	int selidx=m_ipaddr.GetCurSel();
	if(selidx>=0)
	{
		CString selstr;
		m_ipaddr.GetLBText(selidx,selstr);
		CString wstr;
		wstr.Format("Delete the credential for (%s)?",selstr);
		if(MessageBox(wstr,"Confirm Delete",MB_ICONQUESTION|MB_YESNO)!=IDYES)
			return;
		m_ipaddr.DeleteString(selidx);
		string dest,alias;
		CREDMAP::GetDestAlias(selstr,dest,alias);
		CREDMAP::iterator cit=credmap.find(dest);
		if(cit!=credmap.end())
		{
			credmap.erase(cit);
			credmap.Save();
		}
	}
}
