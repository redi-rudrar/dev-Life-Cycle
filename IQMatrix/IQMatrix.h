// IQMatrix.h : main header file for the IQMatrix application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#include "resource.h"       // main symbols

// CIQMatrixApp:
// See IQMatrix.cpp for the implementation of this class
//
#define UM_PORT_CREATED		(WM_USER +1)
#define UM_PORT_MODIFIED	(WM_USER +2)
#define UM_PORT_DELETED		(WM_USER +3)
#define UM_LOG_EVENT		(WM_USER +4)
#define UM_LOG_ERROR		(WM_USER +5)
#define UM_LOG_DEBUG		(WM_USER +6)
#define UM_LOG_RETRY		(WM_USER +7)
#define UM_LOG_RECOVER		(WM_USER +8)
#define UM_SETAPP_HEADING	(WM_USER +9)
#define UM_APP_LOADING		(WM_USER +10)
#define UM_APP_LOADED		(WM_USER +11)
#define UM_APP_UNLOADED		(WM_USER +12)
#define UM_SETAPP_STATUS	(WM_USER +13)
#define UM_LOG_HOSTEVENT	(WM_USER +14)
#define UM_LOG_HOSTERROR	(WM_USER +15)

class CIQMatrixApp : public CWinApp, public WsocksHostImpl
{
public:
	CIQMatrixApp();
	DWORD guiThread;
	bool uiInitialized;
	bool uiShutdown;
	friend class SMConsoleDlg;
	class SMConsoleDlg *smc;

	virtual void WSHSetAppHead(WsocksApp *pApp, const char *heading);
	virtual void WSHSetAppStatus(WsocksApp *pApp, const char *status);
	virtual int WSHGetAppHead(WsocksApp *pApp, char *heading, int hlen);
	virtual int WSHGetAppStatus(WsocksApp *pApp, char *status, int slen);
	//class CIQMatrixDoc *GetDocument(WsocksApp *pApp);
	int WSHExitApp(WsocksApp *pApp, int ecode);
	int WSHExitHost(int ecode);

	// Notifications
	virtual int WSHAppLoading(WsocksApp *pApp);
	virtual int WSHAppLoadFailed(WsocksApp *pApp);
	virtual int WSHAppLoaded(WsocksApp *pApp);
	virtual int WSHAppUnloaded(WsocksApp *pApp);
	virtual int WSHPortCreated(WsocksApp *pApp, WSPort *pport);
	virtual int WSHPortModified(WsocksApp *pApp, WSPort *pport);
	virtual int WSHPortDeleted(WsocksApp *pmod, WSPort *pport);
	virtual int WSHLoggedEvent(WsocksApp *pApp, const char *estr);
	virtual int WSHLoggedError(WsocksApp *pApp, const char *estr);
	virtual int WSHLoggedDebug(WsocksApp *pApp, const char *estr);
	virtual int WSHLoggedRetry(WsocksApp *pApp, const char *estr);
	virtual int WSHLoggedRecover(WsocksApp *pApp, const char *estr);
	virtual int WSHHostLoggedEvent(const char *estr);
	virtual int WSHHostLoggedError(const char *estr);
	virtual int WSHShowWindow(int nCmdShow);

// Overrides
public:
	virtual BOOL InitInstance();
	void OneSecondProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	inline int WSHLoadAppIni(const char *appList=0, APPCONFIGLIST *puiList=0) 
	{ 
		return __super::WSHLoadAppIni(appList,puiList);
	}
	inline WsocksApp *WSHGetApp(const string& aname)
	{
		return __super::WSHGetApp(aname);
	}
	inline int WSHQueueApp(AppConfig *pcfg)
	{
		return __super::WSHQueueApp(pcfg);
	}
	inline int WSHReloadApp(WsocksApp *pApp)
	{
		return __super::WSHReloadApp(pApp);
	}
	inline int WSHDequeueApp(WsocksApp *pApp)
	{
		return __super::WSHDequeueApp(pApp);
	}

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
	afx_msg void OnEditEditsetup();
	afx_msg void OnEditReloadsetup();
	afx_msg void OnUpdateEditReloadsetup(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditErrorlog(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditEventlog(CCmdUI *pCmdUI);
	afx_msg void OnEditEventlog();
	afx_msg void OnEditErrorlog();
	afx_msg void OnUpdateDisable(CCmdUI *pCmdUI);
	afx_msg void OnEditLoadnewapps();
	afx_msg void OnEditSysmonconsole();
};

extern CIQMatrixApp theApp;