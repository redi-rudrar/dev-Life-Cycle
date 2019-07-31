// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "IQMatrix.h"

#include "MainFrm.h"
#include ".\mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// Per Abri's request, remove "IQMatrix" from title to allow for more characters in minimized caption
	m_strTitle="";
	cs.cx=800; cs.cy=600;

	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers


LRESULT CMainFrame::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case UM_PORT_CREATED: 
	{
		theApp.WSHPortCreated((WsocksApp *)wParam,(WSPort*)lParam);
		return TRUE;
	}
	case UM_PORT_MODIFIED: 
	{
		theApp.WSHPortModified((WsocksApp *)wParam,(WSPort*)lParam);
		return TRUE;
	}
	case UM_PORT_DELETED: 
	{
		theApp.WSHPortDeleted((WsocksApp *)wParam,(WSPort*)lParam);
		return TRUE;
	}
	case UM_LOG_EVENT: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHLoggedEvent((WsocksApp *)wParam,ecpy);
		free(ecpy);
		return TRUE;
	}
	case UM_LOG_ERROR: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHLoggedError((WsocksApp *)wParam,ecpy);
		free(ecpy);
		return TRUE;
	}
	case UM_LOG_DEBUG: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHLoggedDebug((WsocksApp *)wParam,ecpy);
		free(ecpy);
		return TRUE;
	}
	case UM_LOG_RETRY: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHLoggedRetry((WsocksApp *)wParam,ecpy);
		free(ecpy);
		return TRUE;
	}
	case UM_LOG_RECOVER: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHLoggedRecover((WsocksApp *)wParam,ecpy);
		free(ecpy);
		return TRUE;
	}
	case UM_SETAPP_HEADING: 
	{
		char *hcpy=(char *)lParam;
		theApp.WSHSetAppHead((WsocksApp *)wParam,hcpy);
		free(hcpy);
		return TRUE;
	}
	case UM_SETAPP_STATUS: 
	{
		char *scpy=(char *)lParam;
		theApp.WSHSetAppStatus((WsocksApp *)wParam,scpy);
		free(scpy);
		return TRUE;
	}
	case UM_APP_LOADING: 
	{
		theApp.WSHAppLoading((WsocksApp *)wParam);
		return TRUE;
	}
	case UM_APP_LOADED: 
	{
		theApp.WSHAppLoaded((WsocksApp *)wParam);
		return TRUE;
	}
	case UM_APP_UNLOADED: 
	{
		theApp.WSHAppUnloaded((WsocksApp *)wParam);
		return TRUE;
	}
	case UM_LOG_HOSTEVENT: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHHostLoggedEvent(ecpy);
		free(ecpy);
		return TRUE;
	}
	case UM_LOG_HOSTERROR: 
	{
		char *ecpy=(char *)lParam;
		theApp.WSHHostLoggedError(ecpy);
		free(ecpy);
		return TRUE;
	}
	case WM_SYSCOMMAND:
	{
		if(wParam==SC_CLOSE)
		{
			//theApp.uiInitialized=false;
			//theApp.uiShutdown=true;
			return TRUE;
		}
		break;
	}
	};
#ifdef WS_OIO
	// Be alertable for the I/O completion threads
	SleepEx(0,true);
#endif

	return CMDIFrameWnd::DefWindowProc(message, wParam, lParam);
}
