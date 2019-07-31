// IQMatrix.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "IQMatrix.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "IQMatrixDoc.h"
#include "IQMatrixView.h"
#include ".\iqmatrix.h"
#include "LoadAppsDlg.h"
#include "SMConsole.h"

#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static LoadAppsDlg *adlg=0;

// CIQMatrixApp

BEGIN_MESSAGE_MAP(CIQMatrixApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE,OnUpdateDisable)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS,OnUpdateDisable)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT,OnUpdateDisable)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW,OnUpdateDisable)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_SETUP,OnUpdateDisable)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_EDIT_EDITSETUP, OnEditEditsetup)
	ON_COMMAND(ID_EDIT_RELOADSETUP, OnEditReloadsetup)
	ON_UPDATE_COMMAND_UI(ID_EDIT_RELOADSETUP, OnUpdateEditReloadsetup)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ERRORLOG, OnUpdateEditErrorlog)
	ON_UPDATE_COMMAND_UI(ID_EDIT_EVENTLOG, OnUpdateEditEventlog)
	ON_COMMAND(ID_EDIT_EVENTLOG, OnEditEventlog)
	ON_COMMAND(ID_EDIT_ERRORLOG, OnEditErrorlog)
	ON_COMMAND(ID_EDIT_LOADNEWAPPS, OnEditLoadnewapps)
	ON_COMMAND(ID_EDIT_SYSMONCONSOLE, OnEditSysmonconsole)
END_MESSAGE_MAP()


// CIQMatrixApp construction

CIQMatrixApp::CIQMatrixApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	guiThread=GetCurrentThreadId();
	uiInitialized=false;
	uiShutdown=false;
	smc=0;
	__HOST_BUILD_DATE__=__DATE__;
	__HOST_BUILD_TIME__=__TIME__;
}
void CIQMatrixApp::OnUpdateDisable(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(false);
}

// The one and only CIQMatrixApp object

CIQMatrixApp theApp;

// Once per second timer callback
static void CALLBACK OneSecondProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	return theApp.OneSecondProc(hwnd,uMsg,idEvent,dwTime);
}
void CIQMatrixApp::OneSecondProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if(idEvent==0x01)
	{
		// No longer needed with waitable timer
		//WSHCheckTime();
		// Run through all the app views and update connection list
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				POSITION vpos=pdoc->GetFirstViewPosition();
				while(vpos)
				{
					CIQMatrixView *pview=(CIQMatrixView *)pdoc->GetNextView(vpos);
					pview->WSDisplayStatus(pview->m_hWnd,0,0x01,WSHTime);
				}
			}
		}
	}
}

#ifdef VS2010
// Copied from Nexus
static void InvalidParameterHandler(const wchar_t* pwszExpression, const wchar_t* pwszFunction, const wchar_t* pwszFile, unsigned int unLine, uintptr_t pReserved)
{
	char szExpression[80] = { 0 }, szFunction[80] = { 0 }, szFile[_MAX_PATH] = { 0 };
	size_t nSize;
	wcstombs_s(&nSize, szExpression, sizeof(szExpression), pwszExpression, _TRUNCATE);
	wcstombs_s(&nSize, szFunction, sizeof(szFunction), pwszFunction, _TRUNCATE);
	wcstombs_s(&nSize, szFile, sizeof(szFile), pwszFile, _TRUNCATE);

	theApp.WSHostLogEvent("CRT Invalid parameter detected in function %s. File: %s Line: %d.", szFunction, szFile, unLine);
	theApp.WSHostLogEvent("Expression: %s.", szExpression);
	//g_pNexusServer->GenerateDump();
}
#endif
// CIQMatrixApp initialization
BOOL CIQMatrixApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_IQMatrixTYPE,
		RUNTIME_CLASS(CIQMatrixDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CIQMatrixView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	//if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	#ifdef VS2010
	// Disable Dr. Watson and catch CRT exceptions
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = InvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);
	#endif

	// Creates apps and assigns to threads (if any)
	uiInitialized=true;
	if(WSHInitHost(0)<0)
	{
		char fpath[MAX_PATH]={0};
		sprintf(fpath,"%s\\logs\\%shev.txt",hostRunDir,WSHcDate);
		if(PathFileExists(fpath))
			ShellExecute(m_pMainWnd->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
		return FALSE;
	}
	m_pMainWnd->SetTimer(0x01,1000,(TIMERPROC)::OneSecondProc);

	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
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
public:
	CString m_version;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
, m_version(_T(""))
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_VERSION, m_version);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CIQMatrixApp::OnAppAbout()
{
	char ipath[MAX_PATH]={0};
	GetModuleFileName(0,ipath,MAX_PATH);
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(ipath,&fdata);
	if(fhnd!=INVALID_HANDLE_VALUE)
		FindClose(fhnd);
	FILETIME lft;
	FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&lft);
	SYSTEMTIME tsys;
	FileTimeToSystemTime(&lft,&tsys);
	CAboutDlg aboutDlg;
	aboutDlg.m_version.Format("IQMatrix 1.0\nBuild %04d%02d%02d-%02d:%02d:%02d",
		tsys.wYear,tsys.wMonth,tsys.wDay,tsys.wHour,tsys.wMinute,tsys.wSecond);
	aboutDlg.DoModal();
}

// CIQMatrixApp message handlers
int CIQMatrixApp::ExitInstance()
{
	uiInitialized=false;
	uiShutdown=true;
	if(smc)
	{
		delete smc; smc=0;
	}
	WSHCleanupHost();

	return __super::ExitInstance();
}

int CIQMatrixApp::WSHPortCreated(WsocksApp *pApp, WSPort *pport)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
			m_pMainWnd->PostMessage(UM_PORT_CREATED,(WPARAM)pApp,(LPARAM)pport);
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x01,(CObject*)pport);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHPortModified(WsocksApp *pApp, WSPort *pport)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
			m_pMainWnd->PostMessage(UM_PORT_MODIFIED,(WPARAM)pApp,(LPARAM)pport);
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x02,(CObject*)pport);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHPortDeleted(WsocksApp *pApp, WSPort *pport)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
			m_pMainWnd->PostMessage(UM_PORT_DELETED,(WPARAM)pApp,(LPARAM)MAKELONG(pport->PortNo,pport->PortType));
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x0B,(CObject*)pport);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHLoggedEvent(WsocksApp *pApp, const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_EVENT,(WPARAM)pApp,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}
			else
				m_pMainWnd->PostMessage(UM_LOG_EVENT,(WPARAM)pApp,(LPARAM)ecpy);
		}
	}
	else
	{
		// SMConsole is rendered via SMConsoleDlg
		if(pApp->pcfg->aclass=="SMConsole")
		{
			if(smc)
				smc->WSHLoggedEvent(estr);
			return 0;
		}

		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x04,(CObject*)estr);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHLoggedError(WsocksApp *pApp, const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_ERROR,(WPARAM)pApp,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}
			else
				m_pMainWnd->PostMessage(UM_LOG_ERROR,(WPARAM)pApp,(LPARAM)ecpy);
		}
	}
	else
	{
		// SMConsole is rendered via SMConsoleDlg
		if(pApp->pcfg->aclass=="SMConsole")
		{
			if(smc)
				smc->WSHLoggedError(estr);
			return 0;
		}

		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x05,(CObject*)estr);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHLoggedDebug(WsocksApp *pApp, const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_DEBUG,(WPARAM)pApp,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}
			else
				m_pMainWnd->PostMessage(UM_LOG_DEBUG,(WPARAM)pApp,(LPARAM)ecpy);
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x06,(CObject*)estr);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHLoggedRetry(WsocksApp *pApp, const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_RETRY,(WPARAM)pApp,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}
			else
				m_pMainWnd->PostMessage(UM_LOG_RETRY,(WPARAM)pApp,(LPARAM)ecpy);			
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x07,(CObject*)estr);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHLoggedRecover(WsocksApp *pApp, const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_RECOVER,(WPARAM)pApp,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}
			else
				m_pMainWnd->PostMessage(UM_LOG_RECOVER,(WPARAM)pApp,(LPARAM)ecpy);			
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x08,(CObject*)estr);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHHostLoggedError(const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_HOSTERROR,0,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}		
			else
				m_pMainWnd->PostMessage(UM_LOG_HOSTERROR,0,(LPARAM)ecpy);
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod->pcfg->aclass=="LBSysmon")
					pdoc->UpdateAllViews(0,0x05,(CObject*)estr);
			}
		}
	}
	return 0;
}
int CIQMatrixApp::WSHHostLoggedEvent(const char *estr)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *ecpy=_strdup(estr);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_LOG_HOSTEVENT,0,(LPARAM)ecpy);
				if(val == 0)
					free(ecpy);
			}
			else
				m_pMainWnd->PostMessage(UM_LOG_HOSTEVENT,0,(LPARAM)ecpy);
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod->pcfg->aclass=="LBSysmon")
					pdoc->UpdateAllViews(0,0x04,(CObject*)estr);
			}
		}
	}
	return 0;
}
void CIQMatrixApp::WSHSetAppHead(WsocksApp *pApp, const char *heading)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *hcpy=_strdup(heading);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_SETAPP_HEADING,0,(LPARAM)hcpy);
				if(val == 0)
					free(hcpy);
			}
			else
				m_pMainWnd->PostMessage(UM_SETAPP_HEADING,(WPARAM)pApp,(LPARAM)hcpy);
			
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
				{
					CString hstr;
					hstr.Format("%d %s",GetCurrentProcessId(),heading);
					pdoc->heading=hstr;
					pdoc->UpdateAllViews(0,0x09,(CObject*)(const char*)hstr);
				}
			}
		}
	}
}
void CIQMatrixApp::WSHSetAppStatus(WsocksApp *pApp, const char *status)
{
	if(GetCurrentThreadId()!=guiThread)
	{
		if((m_pMainWnd)&&(uiInitialized))
		{
			char *scpy=_strdup(status);
			if(bAppInProcessOfExiting)
			{
				LRESULT val = m_pMainWnd->SendMessage(UM_SETAPP_STATUS,0,(LPARAM)scpy);
				if(val == 0)
					free(scpy);
			}
			else
				m_pMainWnd->PostMessage(UM_SETAPP_STATUS,(WPARAM)pApp,(LPARAM)scpy);			
		}
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
				{
					pdoc->status=status;
					pdoc->UpdateAllViews(0,0x0A,(CObject*)status);
				}
			}
		}
	}
}
int CIQMatrixApp::WSHGetAppHead(WsocksApp *pApp, char *heading, int hlen)
{
	POSITION pos=GetFirstDocTemplatePosition();
	while(pos)
	{
		CDocTemplate *cdt=GetNextDocTemplate(pos);
		POSITION dpos=cdt->GetFirstDocPosition();
		while(dpos)
		{
			CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
			if(pdoc->pmod==pApp)
			{
				strncpy(heading,pdoc->heading.c_str(),hlen);
				heading[hlen -1]=0;
				return 0;
			}
		}
	}
	return -1;
}
int CIQMatrixApp::WSHGetAppStatus(WsocksApp *pApp, char *status, int slen)
{
	POSITION pos=GetFirstDocTemplatePosition();
	while(pos)
	{
		CDocTemplate *cdt=GetNextDocTemplate(pos);
		POSITION dpos=cdt->GetFirstDocPosition();
		while(dpos)
		{
			CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
			if(pdoc->pmod==pApp)
			{
				strncpy(status,pdoc->status.c_str(),slen);
				status[slen -1]=0;
				return 0;
			}
		}
	}
	return -1;
}
int CIQMatrixApp::WSHAppLoading(WsocksApp *pApp)
{
	// SMConsole is rendered via SMConsoleDlg
	if(pApp->pcfg->aclass=="SMConsole")
		return 0;
	if(GetCurrentThreadId()!=guiThread)
	{
		while((!uiShutdown)&&(!uiInitialized))
			SleepEx(100,true);
		if((m_pMainWnd)&&(uiInitialized)&&(!uiShutdown))
			m_pMainWnd->SendMessage(UM_APP_LOADING,(WPARAM)pApp,0);
	}
	else
	{
		// App document must be created from GUI thread
		CIQMatrixDoc::nextDocApp=pApp;
		if(!PathFileExists(pApp->pcfg->ConfigPath.c_str()))
		{
			WSHostLogError("ConfigPath(%s) for app(%s) not found!",pApp->pcfg->ConfigPath.c_str(),pApp->pcfg->aname.c_str());
			return -1;
		}
		CIQMatrixDoc *pdoc=(CIQMatrixDoc*)theApp.OpenDocumentFile(pApp->pcfg->ConfigPath.c_str());
		if(!pdoc)
		{
			WSHostLogError("OpenDocumentFile(%s) for app(%s) failed!",pApp->pcfg->DllPath.c_str(),pApp->pcfg->aname.c_str());
			return -1;
		}
	}
	return 0;
}
int CIQMatrixApp::WSHAppLoadFailed(WsocksApp *pApp)
{
	char fpath[MAX_PATH]={0};
	sprintf(fpath,"%s",pApp->WSEventLogPath.c_str());
	if(PathFileExists(fpath))
		ShellExecute(m_pMainWnd->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);

	sprintf(fpath,"%s\\logs\\%shev.txt",hostRunDir,WSHcDate);
	if(PathFileExists(fpath))
		ShellExecute(m_pMainWnd->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
	return 0;
}
int CIQMatrixApp::WSHAppLoaded(WsocksApp *pApp)
{
	// SMConsole is rendered via SMConsoleDlg
	if(pApp->pcfg->aclass=="SMConsole")
		return 0;
	if(GetCurrentThreadId()!=guiThread)
	{
		while((!uiShutdown)&&(!uiInitialized))
			SleepEx(100,true);
		if((m_pMainWnd)&&(uiInitialized)&&(!uiShutdown))
			m_pMainWnd->SendMessage(UM_APP_LOADED,(WPARAM)pApp,0);
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->UpdateAllViews(0,0x03,0);
			}
		}
	}
	if(adlg)
		adlg->UpdateApp(pApp);
	return 0;
}
int CIQMatrixApp::WSHAppUnloaded(WsocksApp *pApp)
{
	// SMConsole is rendered via SMConsoleDlg
	if(pApp->pcfg->aclass=="SMConsole")
		return 0;
	if(GetCurrentThreadId()!=guiThread)
	{
		while((!uiShutdown)&&(!uiInitialized))
			SleepEx(100,true);
		if((m_pMainWnd)&&(uiInitialized)&&(!uiShutdown))
			m_pMainWnd->SendMessage(UM_APP_UNLOADED,(WPARAM)pApp,0);
	}
	else
	{
		POSITION pos=GetFirstDocTemplatePosition();
		while(pos)
		{
			CDocTemplate *cdt=GetNextDocTemplate(pos);
			POSITION dpos=cdt->GetFirstDocPosition();
			while(dpos)
			{
				CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
				if(pdoc->pmod==pApp)
					pdoc->OnCloseDocument();
			}
		}
	}
	if(adlg)
		adlg->UpdateApp(pApp);
	return 0;
}
//CIQMatrixDoc *CIQMatrixApp::GetDocument(WsocksApp *pApp)
//{
//	POSITION pos=GetFirstDocTemplatePosition();
//	while(pos)
//	{
//		CDocTemplate *cdt=GetNextDocTemplate(pos);
//		POSITION dpos=cdt->GetFirstDocPosition();
//		while(dpos)
//		{
//			CIQMatrixDoc *pdoc=(CIQMatrixDoc *)cdt->GetNextDoc(dpos);
//			if(pdoc->pmod==pApp)
//				return pdoc;
//		}
//	}
//	return 0;
//}
// WSHExitApp is only called for app-initiated exits
int CIQMatrixApp::WSHExitApp(WsocksApp *pmod, int ecode)
{
	// SMConsole is rendered via SMConsoleDlg
	if(pmod->pcfg->aclass=="SMConsole")
	{
		if(smc)
			smc->PostMessage(WM_CLOSE,0,0);
		return 0;
	}
	
	if(IsWindow(pmod->WShWnd))
		PostMessage(GetParent(pmod->WShWnd),WM_CLOSE,0,0);

	return 0;
}
int CIQMatrixApp::WSHExitHost(int ecode)
{
	if(m_pMainWnd)
		m_pMainWnd->PostMessage(WM_CLOSE,0,0);

	return 0;
}

void CIQMatrixApp::OnEditEditsetup()
{
	ShellExecute(m_pMainWnd->m_hWnd,"open","notepad.exe","setup\\app.ini",0,SW_SHOW);
}
void CIQMatrixApp::OnUpdateEditReloadsetup(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(false);
}
void CIQMatrixApp::OnEditReloadsetup()
{
	//WSHLoadAppIni();
}

void CIQMatrixApp::OnUpdateEditErrorlog(CCmdUI *pCmdUI)
{
	char fpath[MAX_PATH]={0};
	sprintf(fpath,"logs\\%sher.txt",WSHcDate);
	pCmdUI->SetText(fpath);
}
void CIQMatrixApp::OnEditErrorlog()
{
	char fpath[MAX_PATH]={0};
	sprintf(fpath,"logs\\%sher.txt",WSHcDate);
	ShellExecute(m_pMainWnd->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
}
void CIQMatrixApp::OnUpdateEditEventlog(CCmdUI *pCmdUI)
{
	char fpath[MAX_PATH]={0};
	sprintf(fpath,"logs\\%shev.txt",WSHcDate);
	pCmdUI->SetText(fpath);
}
void CIQMatrixApp::OnEditEventlog()
{
	char fpath[MAX_PATH]={0};
	sprintf(fpath,"logs\\%shev.txt",WSHcDate);
	ShellExecute(m_pMainWnd->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
}

void CIQMatrixApp::OnEditLoadnewapps()
{
	if(!adlg)
	{
		adlg=new LoadAppsDlg;
		adlg->Create(IDD_LOADAPPS);
	}
	adlg->ShowWindow(SW_RESTORE);
}

void CIQMatrixApp::OnEditSysmonconsole()
{
	if(!smc)
	{
		smc=new SMConsoleDlg;
		smc->phost=this;
		smc->Create(IDD_SMCONSOLE);
	}
	smc->ShowWindow(SW_RESTORE);
}

int CIQMatrixApp::WSHShowWindow(int nCmdShow)
{
	if(nCmdShow==SW_RESTORE)
	{
		if(m_pMainWnd)
		{
			m_pMainWnd->ShowWindow(nCmdShow);
			m_pMainWnd->BringWindowToTop();
		}
	}
	else if(nCmdShow==SW_MINIMIZE)
	{
		if(m_pMainWnd)
			m_pMainWnd->ShowWindow(nCmdShow);
	}
	else if(nCmdShow==SW_MAXIMIZE)
	{
		if(m_pMainWnd)
		{
			m_pMainWnd->ShowWindow(nCmdShow);
			m_pMainWnd->BringWindowToTop();
		}
	}
	return 0;
}
