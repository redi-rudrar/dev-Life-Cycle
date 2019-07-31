// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// http://msdn.microsoft.com/officeui.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// vsctest3.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "vsctest3.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "vsctest3Doc.h"
#include "LeftView.h"
#include "vsctest3View.h"
#include "LoginDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma pack(push,8)
// 5000 Global login request
typedef struct tdMsg5000Rec
{
	char ServiceName[64];   // in case of bad routing we can see this in the logs
	char Domain[/*DOMAIN_LEN*/16];
	char User[/*USER_LEN*/16];
	char Account[/*ACCOUNT_LEN*/16];
	char Oms[/*DOMAIN_LEN*/16];
	short BuildType; // this is the product code - so that the same cannot log into wrong domains  //0 = Normal, 1 = DirectTrade, 2 = ABN Trade; 3 = B of A
	int ProductType; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver
	int BuildNo;
	int LineID;
	int VerNo;       // the fe version number – I think current is 9
	short DocSignVersion; // should be in 5001
	char LogonFlags;  // first time or not – used as a bool for multiple account login 
	char EncryptionType;    // 0-Plain text, 1-MD5
	char ClientIP[20];      // IP of client for which this message is on behalf
	int ClientPort;         // The client connecting port isn’t very useful, so maybe the WSOCKS USR portno
	int PassLen;
	char Spare[76];
}  MSG5000REC; //256 +ePassword

// 5001 Global login response
typedef struct tdMsg5001Rec
{
	char ServiceName[64];
	char Domain[/*DOMAIN_LEN*/16];
	char User[/*USER_LEN*/16];
	char Account[/*ACCOUNT_LEN*/16];
	int ProductType; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver
	char LoginOk;
	char RejectReason[80];
	BYTE  cEnable[32][32];  // Entitlements (union with RejectReason?)
	int LineID;
	short DocSignVersion;
	char passwdStatus;            // 0 - OK, 1 - Expiring, 2 - Expired
	int PwdExpireDate;// date the password expires
	char ClientIP[20];      // Copied from msg 5000
	int ClientPort;         // Copied from msg 5000
	char Spare[20];
}  MSG5001REC; // 1280 bytes
#pragma pack(pop)

// Cvsctest3App

BEGIN_MESSAGE_MAP(Cvsctest3App, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &Cvsctest3App::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &Cvsctest3App::OnFileOpen)
END_MESSAGE_MAP()


// Cvsctest3App construction

Cvsctest3App::Cvsctest3App()
	:USER()
	,PASS()
	,notifyList()
	,pMainFrame(0)
	,fixfilter("35,150,11,41,17,37,55,65,54,44,38,60,76,14,151,31,32,58")
	,allfixtags(false)
	,btrdetails(false)
	,browseAllTransactTimes(true)
	,clientregion("")
	,psortlist()
	,ilist()
	#ifdef SPLIT_ORD_DET
	,bdlg(0)
	#endif
	,m_startAlias()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("vsctest3.AppID.NoVersion"));

	// TODO: add construction code here,
	psortlist.push_back(MAKEWORD(0,OCOL_CLIENTID));
	psortlist.push_back(MAKEWORD(0,OCOL_ACCOUNT));
	psortlist.push_back(MAKEWORD(1,OCOL_APPINSTID));
	psortlist.push_back(MAKEWORD(0,OCOL_CONNECTION));
	psortlist.push_back(MAKEWORD(1,OCOL_ORDERDATE));

	psortlist.push_back(MAKEWORD(0,OCOL_TERM));
	psortlist.push_back(MAKEWORD(0,OCOL_TRANSACTTIME));
	psortlist.push_back(MAKEWORD(0,OCOL_HIGHMSGTYPE));
	psortlist.push_back(MAKEWORD(0,OCOL_HIGHEXECTYPE));
	psortlist.push_back(MAKEWORD(0,OCOL_SYMBOL));
	psortlist.push_back(MAKEWORD(0,OCOL_SIDE));
	psortlist.push_back(MAKEWORD(0,OCOL_PRICE));

	psortlist.push_back(MAKEWORD(0,OCOL_ROOTORDERID));
	psortlist.push_back(MAKEWORD(0,OCOL_CLPARENTORDERID));
	psortlist.push_back(MAKEWORD(0,OCOL_FIRSTCLORDID));
	psortlist.push_back(MAKEWORD(0,OCOL_CLORDID));
	psortlist.push_back(MAKEWORD(0,OCOL_ECNORDERID));
	//psortlist.push_back(MAKEWORD(0,OCOL_ROUTEDORDERID));
	psortlist.push_back(MAKEWORD(0,OCOL_ROUTINGBROKER));
	psortlist.push_back(MAKEWORD(0,OCOL_SECURITYTYPE));

	psortlist.push_back(MAKEWORD(0,OCOL_ORDERQTY));
	psortlist.push_back(MAKEWORD(0,OCOL_CUMQTY));
	psortlist.push_back(MAKEWORD(0,OCOL_FILLQTY));
	psortlist.push_back(MAKEWORD(0,OCOL_ORDERLOC));
	// Place all significant initialization in InitInstance
}
void Cvsctest3App::LoadOrderImages()
{
	ilist.Create(16,16,ILC_COLOR4,16,0);
	HICON hNone			  = LoadIcon(MAKEINTRESOURCE(IDI_NONE		   ));
	HICON hAcceptBid      = LoadIcon(MAKEINTRESOURCE(IDI_ACCEPTBID     ));
	HICON hCalculated     = LoadIcon(MAKEINTRESOURCE(IDI_CALCULATED    ));
	HICON hCanceled       = LoadIcon(MAKEINTRESOURCE(IDI_CANCELED      ));
	HICON hCancelPending  = LoadIcon(MAKEINTRESOURCE(IDI_CANCELPENDING ));
	HICON hConfirmed      = LoadIcon(MAKEINTRESOURCE(IDI_CONFIRMED     ));
	HICON hDoneForDay     = LoadIcon(MAKEINTRESOURCE(IDI_DONEFORDAY    ));
	HICON hExpired        = LoadIcon(MAKEINTRESOURCE(IDI_EXPIRED       ));
	HICON hFilled         = LoadIcon(MAKEINTRESOURCE(IDI_FILLED        ));
	HICON hPartFill       = LoadIcon(MAKEINTRESOURCE(IDI_PARTFILL      ));
	HICON hPendingNew     = LoadIcon(MAKEINTRESOURCE(IDI_PENDINGNEW    ));
	HICON hPendingReplace = LoadIcon(MAKEINTRESOURCE(IDI_PENDINGREPLACE));
	HICON hRejected       = LoadIcon(MAKEINTRESOURCE(IDI_REJECTED      ));
	HICON hReplaced       = LoadIcon(MAKEINTRESOURCE(IDI_REPLACED      ));
	HICON hStopped        = LoadIcon(MAKEINTRESOURCE(IDI_STOPPED       ));
	HICON hSuspended      = LoadIcon(MAKEINTRESOURCE(IDI_SUSPENDED     ));
	ilist.Add(hNone);
	ilist.Add(hConfirmed);
	ilist.Add(hPartFill);
	ilist.Add(hFilled);
	ilist.Add(hDoneForDay);
	ilist.Add(hCanceled);
	ilist.Add(hReplaced);
	ilist.Add(hCancelPending);
	ilist.Add(hStopped);
	ilist.Add(hRejected);
	ilist.Add(hSuspended);
	ilist.Add(hPendingNew);
	ilist.Add(hCalculated);
	ilist.Add(hExpired);
	ilist.Add(hAcceptBid);
	ilist.Add(hPendingReplace);
}
int Cvsctest3App::OrderImage(char highMsgType, char highExecType, int ordQty, int fillQty)
{
	int idx=0;
	switch(highExecType)
	{
	case '0': idx=1; break;
	case '1': idx=2; break;
	case '2': idx=3; break;
	case '3': idx=4; break;
	case '4': idx=5; break;
	case '5': idx=6; break;
	case '6': idx=7; break;
	case '7': idx=8; break;									case '8': idx=9; break;
	case '9': idx=10; break;
	case 'A': idx=11; break;
	case 'B': idx=12; break;
	case 'C': idx=13; break;
	case 'D': idx=14; break;
	case 'E': idx=15; break;
	default:
		switch(highMsgType)
		{
		case 'D': idx=11; break;
		case 'G': idx=15; break;
		case 'F': idx=7; break;
		case '9': idx=9; break;
		};
	};
	if(fillQty>0)
	{
		idx=2;
		if((ordQty>0)&&(fillQty>=ordQty))
			idx=3;
	}
	return idx;
}
char Cvsctest3App::GetMsgTypeTier(char mtype)
{
	char tval=0;
	switch(mtype)
	{
	case 'D': tval=1; break;
	case 'G': tval=2; break;
	case 'F': tval=3; break;
	case '8': tval=4; break;
	case '9': tval=5; break;
	default: tval=6;
	};
	return tval;
}
char Cvsctest3App::GetExecTypeTier(char etype)
{
	char tval=0;
	switch(etype)
	{
	case '0': tval=1; break;
	case '5': tval=2; break;
	case '1': tval=3; break;
	case '3': 
	case '4': 
	case '8': 
	case '9': tval=4; break;
	case '2': tval=5; break;
	default: tval=6;
	};
	return tval;
}

// The one and only Cvsctest3App object

Cvsctest3App theApp;


// Cvsctest3App initialization

BOOL Cvsctest3App::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	LoadOrderImages();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
		RUNTIME_CLASS(Cvsctest3Doc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CLeftView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd

	LoadSetupIni();
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if(cmdInfo.m_nShellCommand==cmdInfo.FileOpen)
	{
		m_startAlias=cmdInfo.m_strFileName;
		cmdInfo.m_nShellCommand=cmdInfo.FileNew;
		cmdInfo.m_strFileName="";
	}
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

    AppConfig acfg;
    acfg.aclass="vsctest3";
    acfg.aname="vsctest3";
    acfg.ver=(WSAppVersion)WSHOST_INTERFACE_VERSION;
    acfg.TargetThreads="VT1";
    if(theApp.WSCreateGuiHost(&acfg,false)<0)
		return FALSE;

	pMainFrame->PostMessage(WM_COMMAND,ID_FILE_OPEN,0);
	return TRUE;
}

int Cvsctest3App::ExitInstance()
{
	//TODO: handle additional resources you may have added
	theApp.WSDestroyGuiHost();
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// Cvsctest3App message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void Cvsctest3App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void Cvsctest3App::OnFileOpen()
{
	if(ConPort[0].SockConnected)
		ConPort[0].ConnectHold=true;
	else
	{
		CLoginDlg ldlg;
		ldlg.m_selalias=m_startAlias.c_str(); m_startAlias="";
		ldlg.DoModal();
	}
}

// Cvsctest3App customization load/save methods

void Cvsctest3App::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void Cvsctest3App::LoadCustomState()
{
}

void Cvsctest3App::SaveCustomState()
{
}

static void btrim(char *Value)
{
     char *s, *d;

     d = s = Value;								   /* point d to string base  */
     while (*s && (*s == ' ' || *s == '\"'))       /* skip over first spaces */
          ++s;
     
	 while ((*d++ = *s++) != '\0');                /* copy until null        */

     d = s = Value;
     
	 s += (strlen(Value) - 1);
     if (s <= d)
        return;
     
	 while (s >= d && (*s == '\0' || *s == ' ' || *s == '\"' || *s == '\t' || *s == '\r' || *s == '\n')) 
	 {    /* find end of string */
         *s = '\0';
         --s;
     }
}
int Cvsctest3App::LoadSetupIni()
{
	char ErrText[1024]={0};
	//HEADING=pcfg->aname;
	char fpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(fpath,"%s\\setup\\setup.ini",pcfg?pcfg->RunPath.c_str():".");
	#else
	sprintf(fpath,"%s/setup/setup.ini",pcfg?pcfg->RunPath.c_str():".");
	#endif
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			if((fp=fopen(fpath,"wt"))==NULL)
			{
				sprintf(ErrText,"Could not open %s",fpath);
				goto Error;
			}
			//fprintf(fp,"HEADING:%s\n",HEADING.c_str());
			fprintf(fp,"FIXFILTER:%s\n",fixfilter.c_str());
			fprintf(fp,"ALLFIXTAGS:%d\n",allfixtags?1:0);
			fprintf(fp,"BTR:%d\n",btrdetails?1:0);
			fclose(fp);
			return 0;
		}
		sprintf(ErrText,"Could not open %s",fpath);
		goto Error;
	}
	{//scope
	int lno=0;
	char rbuf[1024]={0};
	char sname[1024]={0};
	char value[1024]={0};
	while(fgets(rbuf,1023,fp)!=NULL)
	{
		lno++;
        if(strncmp(rbuf,"//",2)==0)
            continue;
		char *Inptr;
		if((Inptr=strtok(rbuf,":\n\r"))==NULL)
			continue;
		if(Inptr[0]==0)
			continue;
		strcpy(sname,Inptr);
		btrim(value);
		if((Inptr=strtok(NULL,"\n\r"))==NULL)
		{
			sprintf(ErrText,"setup.ini: Empty value at line %d!",lno);
			goto Error;
		}
		strcpy(value,Inptr);
		btrim(value);

		//if(strcmp(sname,"HEADING")==0)
		//{
		//	HEADING=value;
		//}
		//else 
		if(strcmp(sname,"FIXFILTER")==0)
		{
			fixfilter=value;
		}
		else if(strcmp(sname,"ALLFIXTAGS")==0)
		{
			allfixtags=atoi(value)?true:false;
		}
		else if(strcmp(sname,"BTR")==0)
		{
			btrdetails=atoi(value)?true:false;
		}		
		else if(strcmp(sname,"BROWSE_ALL_TRANSACT_TIMES")==0)
		{
			browseAllTransactTimes=atoi(value)?true:false;
		}
		else if(strcmp(sname,"CLIENT_REGION")==0)
		{
			clientregion=value;
		}		
		else
		{
			sprintf(ErrText,"Unknown setup.ini setting(%s)!",sname);
			goto Error;
		}
	}
	fclose(fp);

	WSLogEvent("Loaded %s",fpath);
	}//scope
	return 0;
Error:
	if(fp)
		fclose(fp);
	WSLogError(ErrText);
	return -1;
}
int Cvsctest3App::SaveSetupIni()
{
	char ErrText[1024]={0};
	//HEADING=pcfg->aname;
	char sdir[MAX_PATH]={0},fpath[MAX_PATH]={0},wpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(sdir,"%s\\setup",pcfg?pcfg->RunPath.c_str():".");
	sprintf(fpath,"%s\\setup\\setup.ini",pcfg?pcfg->RunPath.c_str():".");
	sprintf(wpath,"%s\\setup\\setup.ini.tmp",pcfg?pcfg->RunPath.c_str():".");
	#else
	sprintf(sdir,"%s/setup",pcfg?pcfg->RunPath.c_str():".");
	sprintf(fpath,"%s/setup/setup.ini",pcfg?pcfg->RunPath.c_str():".");
	sprintf(fpath,"%s/setup/setup.ini.tmp",pcfg?pcfg->RunPath.c_str():".");
	#endif
	CreateDirectory(sdir,0);
	FILE *wfp=fopen(wpath,"wt");
	if(!wfp)
	{
		sprintf(ErrText,"Could not write %s",wpath);
		goto Error;
	}
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			if((fp=fopen(fpath,"wt"))==NULL)
			{
				sprintf(ErrText,"Could not open %s",fpath);
				goto Error;
			}
			//fprintf(fp,"HEADING:%s\n",HEADING.c_str());
			fprintf(fp,"FIXFILTER:%s\n",fixfilter.c_str());
			fclose(fp);
			return 0;
		}
		sprintf(ErrText,"Could not open %s",fpath);
		goto Error;
	}
	{//scope
	int lno=0;
	char rbuf[1024]={0};
	char sname[1024]={0};
	char value[1024]={0};
	while(fgets(rbuf,1023,fp)!=NULL)
	{
		string line=rbuf;
		lno++;
        if(strncmp(rbuf,"//",2)==0)
            goto save_line;
		char *Inptr;
		if((Inptr=strtok(rbuf,":\n\r"))==NULL)
			goto save_line;
		if(Inptr[0]==0)
			goto save_line;
		strcpy(sname,Inptr);
		btrim(value);
		if((Inptr=strtok(NULL,"\n\r"))==NULL)
		{
			sprintf(ErrText,"setup.ini: Empty value at line %d!",lno);
			goto Error;
		}
		strcpy(value,Inptr);
		btrim(value);

		if(strcmp(sname,"FIXFILTER")==0)
		{
			fprintf(wfp,"FIXFILTER:%s\n",fixfilter.c_str());
			continue;
		}
		else if(strcmp(sname,"ALLFIXTAGS")==0)
		{
			fprintf(wfp,"ALLFIXTAGS:%d\n",allfixtags?1:0);
			continue;
		}
		else if(strcmp(sname,"BTR")==0)
		{
			fprintf(wfp,"BTR:%d\n",btrdetails?1:0);
			continue;
		}
		else if(strcmp(sname,"BROWSE_ALL_TRANSACT_TIMES")==0)
		{
			fprintf(wfp,"BROWSE_ALL_TRANSACT_TIMES:%d\n",browseAllTransactTimes?1:0);
			continue;
		}
		else if(strcmp(sname,"CLIENT_REGION")==0)
		{
			fprintf(wfp,"CLIENT_REGION:%s\n",clientregion.c_str());
			continue;
		}
save_line:
		fprintf(wfp,"%s",line.c_str());
	}
	fclose(fp);
	fclose(wfp);
	if(CopyFile(wpath,fpath,false))
		DeleteFile(wpath);

	WSLogEvent("Saved %s",fpath);
	}//scope
	return 0;
Error:
	if(fp)
		fclose(fp);
	if(wfp)
		fclose(wfp);
	WSLogError(ErrText);
	return -1;
}

// Cvsctest3App message handlers
int Cvsctest3App::WSHInitModule(AppConfig *config, class WsocksHost *apphost)
{
	__WSDATE__=__DATE__;
	__WSTIME__=__TIME__;

	NO_OF_CON_PORTS=1;

	if(WSInitSocks()<0)
		return -1;
	return 0;
}
int Cvsctest3App::WSPortsCfg()
{
	WSCreatePort(WS_CON,0);
	strcpy(ConPort[0].LocalIP,"AUTO");
	strcpy(ConPort[0].AltRemoteIP[0],"127.0.0.1");
	ConPort[0].AltRemoteIPOn[0]=true;
	ConPort[0].Port=4522;
	ConPort[0].AltIPCount=1;
	ConPort[0].Compressed=true;
	ConPort[0].BlockSize=128;
	ConPort[0].ConnectHold=true;
	return 1;
}
int Cvsctest3App::WSHCleanupModule()
{
	return 0;
}
void Cvsctest3App::WSConOpened(int PortNo)
{
	WSLogEvent("CON%d: Connected to %s:%d...",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);

	//ConPort[PortNo].DetPtr=(void*)PROTO_VSCLIENT;
	VSCodec *vsc=new VSCodec;
	if(vsc->Init(&theApp)<0)
	{
		WSLogError("CON%d: Failed to initialize VSCodec",PortNo);
		ConPort[PortNo].ConnectHold=true;
		return;
	}
	ConPort[PortNo].DetPtr3=vsc;
	char rbuf[256],*rptr=rbuf;
	int& rid=(int&)ConPort[PortNo].DetPtr4;
	// IQATS
	if(!strncmp(ConPort[PortNo].CfgNote,"$IQATS$",7))
	{
		MSG5000REC Msg5000Rec;
		memset(&Msg5000Rec,0,sizeof(MSG5000REC));
		strcpy(Msg5000Rec.ServiceName,"VSDB");
		strcpy(Msg5000Rec.Domain,"");
		strcpy(Msg5000Rec.User,"");
		strcpy(Msg5000Rec.Account,"");
		Msg5000Rec.ProductType=0x80; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver
		Msg5000Rec.BuildNo=buildDate;
		Msg5000Rec.LineID=0;
		Msg5000Rec.EncryptionType=0;    // 0-Plain text, 1-MD5
		memmove(ConPort[PortNo].CfgNote,ConPort[PortNo].CfgNote+7,strlen(ConPort[PortNo].CfgNote+7)+1);
		if(!strncmp(ConPort[PortNo].CfgNote,"$TUNNEL$",8))
		{
			strcpy(Msg5000Rec.ServiceName,"SMP");
			char *nptr=strchr(ConPort[PortNo].CfgNote+8,'/');
			if(nptr)
			{
				*nptr=0; nptr++;
				strcpy(Msg5000Rec.ClientIP,ConPort[PortNo].CfgNote+8);
				memmove(ConPort[PortNo].CfgNote+8,nptr,strlen(nptr)+1);
			}
			else
				strcpy(Msg5000Rec.ClientIP,ConPort[PortNo].CfgNote+8);
		}
		else
			strcpy(Msg5000Rec.ClientIP,ConPort[PortNo].CfgNote);
		char *pptr=strchr(Msg5000Rec.ClientIP,':');
		if(pptr)
		{
			*pptr=0; Msg5000Rec.ClientPort=atoi(pptr+1);
		}
		Msg5000Rec.PassLen=0;
		WSConSendMsg(5000,sizeof(MSG5000REC),(char*)&Msg5000Rec,PortNo);
	}
	// SysmonProxy
	else if(!strncmp(ConPort[PortNo].CfgNote,"$TUNNEL$",8))
	{
		char parms[256]={0};
		sprintf_s(parms,"User=%s|Pass=|",USER.c_str());
		SysCmdCodec *ccodec=new SysCmdCodec();
		ccodec->Init();
		ConPort[PortNo].DetPtr5=ccodec;
		if(!ccodec->Encode(rptr,sizeof(rbuf),true,rid,"login",parms,0))
		{
			WSConSendMsg(1108,(int)(rptr -rbuf),rbuf,PortNo);
			rid++;
		}
		else
		{
			WSLogError("CON%d: Failed EncodeLoginRequest",PortNo);
			ConPort[PortNo].ConnectHold=true;
		}
	}
	// ViewServer
	else
	{
		if(vsc->EncodeLoginRequest2(rptr,sizeof(rbuf),rid,USER.c_str(),PASS.c_str())>0)
		{
			WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
			rid++;
		}
		else
		{
			WSLogError("CON%d: Failed EncodeLoginRequest",PortNo);
			ConPort[PortNo].ConnectHold=true;
		}
	}
}
int Cvsctest3App::WSConMsgReady(int PortNo)
{
	int Size=ConPort[PortNo].InBuffer.LocalSize;
	if(Size<sizeof(MSGHEADER))
		return 0;
	MSGHEADER *MsgHeader=(MSGHEADER *)ConPort[PortNo].InBuffer.Block;
	if(Size<(int)sizeof(MSGHEADER) +MsgHeader->MsgLen)
		return 0;
	char *Msg=(char *)MsgHeader +sizeof(MSGHEADER);
	TranslateConMsg(MsgHeader,Msg,PortNo);
	return 1;
}
int Cvsctest3App::WSConStripMsg(int PortNo)
{
	MSGHEADER *MsgHeader=(MSGHEADER *)ConPort[PortNo].InBuffer.Block;
	const char *Msg=(const char *)MsgHeader +sizeof(MSGHEADER);
	WSStripBuff(&ConPort[PortNo].InBuffer,sizeof(MSGHEADER) +MsgHeader->MsgLen);
	return 0;
}
void Cvsctest3App::WSConClosed(int PortNo)
{
	WSLogEvent("CON%d: Disconnected from %s:%d.",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
	ConPort[PortNo].ConnectHold=true;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr3;
	if(vsc)
	{
		char rbuf[1024]={0},*rptr=rbuf;
		int rlen=sizeof(rbuf);
		int& crid=(int&)ConPort[PortNo].DetPtr4;
		vsc->EncodeLoginReply2(rptr,rlen,-1,crid++,"Disconnected","",WSDate);
		vsc->DecodeReply(rbuf,(int)(rptr -rbuf),(void*)(LONGLONG)MAKELONG(PortNo,WS_CON));
		delete vsc; ConPort[PortNo].DetPtr3;
	}
	if(pMainFrame)
		pMainFrame->PostMessage(WM_COMMAND,ID_FILE_OPEN,0);
}

void Cvsctest3App::WSTimeChange()
{
	char hbuf[128];
	memset(hbuf,0,sizeof(hbuf));
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		WSConSendMsg(16,sizeof(hbuf),hbuf,i);
	}
}
int Cvsctest3App::TranslateConMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	switch(MsgHeader->MsgID)
	{
	case 16:
		ConPort[PortNo].BeatsIn++;
		return 0;
	case 1: // reply
	{
		VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr3;
		if(vsc)
			vsc->DecodeReply(Msg,MsgHeader->MsgLen,(void*)(LONGLONG)MAKELONG(PortNo,WS_CON)); // will call NotifyRootOrderReply
		break;
	}
	case 1109: // tunnel reply
	{
		SysCmdCodec *ccodec=(SysCmdCodec*)ConPort[PortNo].DetPtr5;
		if(ccodec)
		{
			const char *mptr=Msg;
			ccodec->Decode(mptr,MsgHeader->MsgLen,false,this,(void*)(LONGLONG)MAKELONG(PortNo,WS_CON));
		}
		break;
	}
	case 5001:
	{
		MSG5001REC Msg5001Rec;
		memcpy(&Msg5001Rec,Msg,sizeof(MSG5001REC));
		if(Msg5001Rec.LoginOk)
		{
			// Continue the tunnel protocol
			if(!strncmp(ConPort[PortNo].CfgNote,"$TUNNEL$",8))
				WSConOpened(PortNo);
			else
			{
				WSLogEvent("CON%d: Logged in",PortNo);
				VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr3;
				if(!vsc)
					return -1;
				char rbuf[256],*rptr=rbuf,parms[256]={0};
				int& rid=(int&)ConPort[PortNo].DetPtr4;
				if(vsc->EncodeLoginRequest2(rptr,sizeof(rbuf),rid,USER.c_str(),PASS.c_str())>0)
				{
					WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
					rid++;
				}
				else
				{
					WSLogError("CON%d: Failed VSCodec::EncodeLoginRequest2",PortNo);
					ConPort[PortNo].ConnectHold=true;
				}
			}
		}
		else
		{
			WSLogError("CON%d: Failed 5000 login: %s",PortNo,Msg5001Rec.RejectReason);
			ConPort[PortNo].ConnectHold=true;
			::AfxMessageBox(Msg5001Rec.RejectReason);
		}
		break;
	}
	};
	return 0;
}

void Cvsctest3App::AddNotify(VSCNotify *pnotify)
{
	notifyList.push_back(pnotify);
}
void Cvsctest3App::RemNotify(VSCNotify *pnotify)
{
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
	{
		if(*nit==pnotify)
		{
			notifyList.erase(nit);
			break;
		}
	}
}

// VSCNotify
void Cvsctest3App::VSCNotifyError(int rc, const char *emsg)
{
	WSLogError(emsg);
}
void Cvsctest3App::VSCNotifyEvent(int rc, const char *emsg)
{
	WSLogEvent(emsg);
}
// For EncodeLoginRequest
void Cvsctest3App::VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
}
void Cvsctest3App::VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
{
}
// For EncodeSqlRequest
void Cvsctest3App::VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	VSCodec *vsc=(VSCodec *)theApp.ConPort[0].DetPtr3;
	if(!vsc)
		return;
	int& crid=(int&)theApp.ConPort[0].DetPtr4;
	if(!rid) rid=crid++;
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifySqlRequest(udata,rid,select,from,where,maxorders,hist,live,iter);

	char rbuf[1024]={0},*rptr=rbuf;
	if(vsc->EncodeSqlRequest(rptr,sizeof(rbuf),rid,select,from,where,maxorders,hist,live,iter)<0)
		theApp.WSLogError("Failed EncodeSqlRequest");
	else
		theApp.WSConSendMsg(1,(int)(rptr -rbuf),rbuf,0);
}
void Cvsctest3App::VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
{
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nfix,more,iter,reason);
}
void Cvsctest3App::VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifySqlDsvReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstr,nstr,more,iter,reason);
}
void Cvsctest3App::VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
{
	VSCodec *vsc=(VSCodec *)theApp.ConPort[0].DetPtr3;
	if(!vsc)
		return;
	DWORD& crid=(DWORD&)theApp.ConPort[0].DetPtr4;
	if(!rid) rid=crid++;
	else if(rid>=crid) crid=rid +1;
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifySqlIndexRequest(udata,rid,select,from,where,maxorders,unique,istart,idir,iter);

	char rbuf[1024]={0},*rptr=rbuf;
	if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),rid,select,from,where,maxorders,unique,istart,idir,iter)<0)
		theApp.WSLogError("Failed EncodeSqlIndexRequest");
	else
		theApp.WSConSendMsg(1,(int)(rptr -rbuf),rbuf,0);
}
void Cvsctest3App::VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifySqlIndexReply(udata,rc,rid,endcnt,totcnt,proto,pstr,nstr,more,iter,reason);
}
// For EncodeCancelRequest
void Cvsctest3App::VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
{
}
// For EncodeDescribeRequest
void Cvsctest3App::VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
{
	VSCodec *vsc=(VSCodec *)theApp.ConPort[0].DetPtr3;
	if(!vsc)
		return;
	DWORD& crid=(DWORD&)theApp.ConPort[0].DetPtr4;
	if(!rid) rid=crid++;
	else if(rid>=crid) crid=rid +1;
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifyDescribeRequest(udata,rid,from,maxrows,iter);

	char rbuf[1024]={0},*rptr=rbuf;
	if(vsc->EncodeDescribeRequest(rptr,sizeof(rbuf),rid,from,maxrows,iter)<0)
		theApp.WSLogError("Failed EncodeDescribeRequest");
	else
		theApp.WSConSendMsg(1,(int)(rptr -rbuf),rbuf,0);
}
void Cvsctest3App::VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifyDescribeReply(udata,rc,rid,endcnt,totcnt,pstr,nstr,more,iter,reason);
}

// For EncodeLoginRequest2
void Cvsctest3App::VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
}
void Cvsctest3App::VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
{
	int PortNo=LOWORD(udata);
	if(rc<0)
		WSLogError("CON%d: Failed login: %s",PortNo,reason);
	else
		WSLogEvent("CON%d: Logged in",PortNo);

	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifyLoginReply2(udata,rc,rid,reason,name,wsdate);
}
// For EncodeSqlRequest2
void Cvsctest3App::VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
{
	VSCodec *vsc=(VSCodec *)theApp.ConPort[0].DetPtr3;
	if(!vsc)
		return;
	int& crid=(int&)theApp.ConPort[0].DetPtr4;
	if(!rid) rid=crid++;
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifySqlRequest2(udata,rid,select,from,where,maxorders,hist,live,iter,ttl);
}
// For EncodeSqlIndexRequest2
void Cvsctest3App::VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
{
}

void Cvsctest3App::VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	VSCodec *vsc=(VSCodec *)theApp.ConPort[0].DetPtr3;
	if(!vsc)
		return;
	int& crid=(int&)theApp.ConPort[0].DetPtr4;
	if(!rid) rid=crid++;
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifyDatRequest(udata,rid,select,from,where,maxorders,hist,live,iter);

	char rbuf[1024]={0},*rptr=rbuf;
	if(vsc->EncodeDatRequest(rptr,sizeof(rbuf),rid,select,from,where,maxorders,hist,live,iter)<0)
		theApp.WSLogError("Failed EncodeDatRequest");
	else
		theApp.WSConSendMsg(1,(int)(rptr -rbuf),rbuf,0);
}
void Cvsctest3App::VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	for(list<VSCNotify *>::iterator nit=notifyList.begin();nit!=notifyList.end();nit++)
		(*nit)->VSCNotifyDatReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstrlen,pstr,nstr,more,iter,reason);
}

// SysCmdNotify
void Cvsctest3App::NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata)
{
}
void Cvsctest3App::NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata)
{
	int PortNo=LOWORD(udata);
	if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS)||(!ConPort[PortNo].SockConnected))
		return;
	if(!strncmp(ConPort[PortNo].CfgNote,"$TUNNEL$",8))
	{
		SysCmdCodec *ccodec=(SysCmdCodec *)ConPort[PortNo].DetPtr5;
		if(!ccodec)
			return;
		TVMAP tvmap;
		ccodec->GetTagValues(tvmap,resp,rlen);
		string rc=ccodec->GetValue(tvmap,"rc");
		string reason=ccodec->GetValue(tvmap,"reason");
		// Send tunnel command
		if(cmd=="login")
		{
			if(rc=="0")
				WSLogEvent("CON%d: Logged in at proxy",PortNo);
			else
			{
				WSLogError("CON%d: Failed login at proxy: %s)",PortNo,reason.c_str());
				ConPort[PortNo].ConnectHold=true;
				return;
			}

			char rbuf[256],*rptr=rbuf,parms[256]={0};
			int& rid=(int&)ConPort[PortNo].DetPtr4;
			char dpath[256]={0};
			strcpy(dpath,ConPort[PortNo].CfgNote +8);
			char *tptr=strchr(dpath,'/');
			if(tptr)
			{
				*tptr=0; sprintf(ConPort[PortNo].CfgNote,"$TUNNEL$%s",tptr+1);
			}
			else
				strcpy(ConPort[PortNo].CfgNote,"$TUNNEL$");
			int tport=0;
			tptr=strchr(dpath,':');
			if(tptr)
			{
				*tptr=0; tport=atoi(tptr+1);
			}
			sprintf(parms,"Ip=%s|Port=%d|OnBehalfOf=%s",dpath,tport,USER.c_str());
			if(!ccodec->Encode(rptr,sizeof(rbuf),true,rid,"tunnel",parms,0))
			{
				WSConSendMsg(1108,(int)(rptr -rbuf),rbuf,PortNo);
				rid++;
			}
			else
			{
				WSLogError("CON%d: Failed SysCmdCodec::Encode",PortNo);
				ConPort[PortNo].ConnectHold=true;
			}
			return;
		}
		else if(cmd=="tunnel")
		{
			if(rc=="0")
			{
				char rbuf[256],*rptr=rbuf,parms[256]={0};
				int& rid=(int&)ConPort[PortNo].DetPtr4;
				char dpath[256]={0};
				strcpy(dpath,ConPort[PortNo].CfgNote +8);
				// Next hop
				if(*dpath)
				{
					char *tptr=strchr(dpath,'/');
					if(tptr)
					{
						*tptr=0; sprintf(ConPort[PortNo].CfgNote,"$TUNNEL$%s",tptr+1);
					}
					else
						strcpy(ConPort[PortNo].CfgNote,"$TUNNEL$");
					int tport=0;
					tptr=strchr(dpath,':');
					if(tptr)
					{
						*tptr=0; tport=atoi(tptr+1);
					}
					sprintf(parms,"Ip=%s|Port=%d|OnBehalfOf=%s",dpath,tport,USER.c_str());
					if(!ccodec->Encode(rptr,sizeof(rbuf),true,rid,"tunnel",parms,0))
					{
						WSConSendMsg(1108,(int)(rptr -rbuf),rbuf,PortNo);
						rid++;
					}
					else
					{
						WSLogError("CON%d: Failed SysCmdCodec::Encode",PortNo);
						ConPort[PortNo].ConnectHold=true;
					}
					return;
				}
				WSLogEvent("CON%d: Logged in",PortNo);
				VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr3;
				if(!vsc)
					return;
				if(vsc->EncodeLoginRequest2(rptr,sizeof(rbuf),rid,USER.c_str(),PASS.c_str())>0)
				{
					WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
					rid++;
				}
				else
				{
					WSLogError("CON%d: Failed VSCodec::EncodeLoginRequest2",PortNo);
					ConPort[PortNo].ConnectHold=true;
				}
			}
			else
			{
				WSLogError("CON%d: Failed login: %s)",PortNo,reason.c_str());
				ConPort[PortNo].ConnectHold=true;
			}
			return;
		}
	}
}
