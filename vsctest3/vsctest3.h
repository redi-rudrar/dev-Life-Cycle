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

// vsctest3.h : main header file for the vsctest3 application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "wsocksapi.h"
#include "ViewServerProto.h"
#include "sysmonc.h"
#include "sysmonc.h"

// Cvsctest3App:
// See vsctest3.cpp for the implementation of this class
//

class Cvsctest3App : public CWinAppEx
	,public WsocksGuiApp
	,public VSCNotify
	,public SysCmdNotify
{
public:
	Cvsctest3App();

	// WsocksGuiApp
	int WSHInitModule(AppConfig *config, class WsocksHost *apphost);
	int WSHCleanupModule();
	int WSPortsCfg();

	void WSConOpened(int PortNo);
	int WSConMsgReady(int PortNo);
	int WSConStripMsg(int PortNo);
	void WSConClosed(int PortNo);

	// VSCNotify
	void VSCNotifyError(int rc, const char *emsg);
	void VSCNotifyEvent(int rc, const char *emsg);
	// For EncodeLoginRequest
	void VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen);
	void VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason);
	// For EncodeSqlRequest
	void VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);
	void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason);
	void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	void VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter);
	void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	// For EncodeCancelRequest
	void VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter);
	// For EncodeDescribeRequest
	void VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter);
	void VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

	// For EncodeLoginRequest2
	void VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen);
	void VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate);
	// For EncodeSqlRequest2
	void VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl);
	// For EncodeSqlIndexRequest2
	void VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl);

	void VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);
	void VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

	// SysCmdNotify
	void NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata);
	void NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata);

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	UINT  m_nAppLook;
	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	string USER;
	string PASS;
	list<VSCNotify *> notifyList;
	void AddNotify(VSCNotify *pnotify);
	void RemNotify(VSCNotify *pnotify);
	class CMainFrame *pMainFrame;
	string fixfilter;
	bool allfixtags;
	bool btrdetails;
	bool browseAllTransactTimes;
	string clientregion;
	list<WORD> psortlist;
	CImageList ilist;
	void LoadOrderImages();
	int OrderImage(char highMsgType, char highExecType, int ordQty, int fillQty);
	static char GetMsgTypeTier(char mtype);
	static char GetExecTypeTier(char etype);
	#ifdef SPLIT_ORD_DET
	class CBinaryDlg *bdlg;
	#endif
	string m_startAlias;

	int LoadSetupIni();
	int SaveSetupIni();
	void WSTimeChange();
	int TranslateConMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo);

	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	DECLARE_MESSAGE_MAP()
};

class VSDBQUERY
{
public:
	VSDBQUERY()
		:rid(0)
		,select("")
		,from("")
		,where("")
		,maxorders(0)
		,unique(true)
		,istart(0)
		,idir(0)
		,hist(true)
		,live(false)
		,iter(0)
		,asys("")
		,nresults(0)
		,wresults(500)
		,results()
		#ifdef SPLIT_ORD_DET
		,bresults()
		#endif
		#ifdef TIME_CONVERT
		,isgmt(true)
		#endif
		,traceSide()
		,tracePrice(0.0)
		,traceShares(0)
		,traceOrder()
		,traceIdType()
		,traceDate(0)
		//,ordQty(0)
		,fillQty(0)
		,fillSum(0.0)
	{
	}
	// Query parms
	DWORD rid;
	CString select;
	CString from;
	CString where;
	int maxorders;
	bool unique;
	int istart;
	char idir;
	bool hist;
	bool live;
	LONGLONG iter;
	// Results
	CString asys;
	int nresults;
	int wresults;
	list<string> results;
	#ifdef SPLIT_ORD_DET
	list<char*> bresults;
	#endif
	#ifdef TIME_CONVERT
	bool isgmt;
	#endif
	// Trace parms
	string traceSide;
	double tracePrice;
	int traceShares;
	string traceOrder;
	string traceIdType;
	int traceDate;
	// Aggregates
	//int ordQty;
	int fillQty;
	double fillSum;
};

extern Cvsctest3App theApp;
