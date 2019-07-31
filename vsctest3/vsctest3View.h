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

// vsctest3View.h : interface of the Cvsctest3View class
//

#pragma once

// List view of index results 
class Cvsctest3View : public CListView
	,public VSCNotify
{
protected: // create from serialization only
	Cvsctest3View();
	DECLARE_DYNCREATE(Cvsctest3View)

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

// Attributes
public:
	Cvsctest3Doc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void PostNcDestroy();

// Implementation
public:
	virtual ~Cvsctest3View();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	class VSCNotify *connNotify;
	static VSDBQUERY newq;
	CMultiDocTemplate* vDocTemplate;
	#ifdef SPLIT_ORD_DET
	CMultiDocTemplate* detDocTemplate;
	#endif
	int osortcol;
	int btrsortcol;
	int fixsortcol;
	static LONGLONG GMTToEST(LONGLONG gts, SYSTEMTIME& tsys, CString& stz);
	static LONGLONG ESTToGMT(LONGLONG gts, SYSTEMTIME& tsys, CString& stz);

	CWnd *tpdlg;
	int LaunchFixViewer(const char *fpath);
	int LaunchTextViewer(const char *fpath);
	int LaunchBtrViewer(const char *fpath);
	int SortOrders(LPARAM e1, LPARAM e2, LPARAM hint);
	int SortBTR(LPARAM e1, LPARAM e2, LPARAM hint);
	int SortFIX(LPARAM e1, LPARAM e2, LPARAM hint);

protected:
	VSDBQUERY curq;
	CString smsg;
	bool moreToGet;
	bool eventlog;
	bool datRequested;
	struct VQSAVEALL
	{
		VSDBQUERY query;
		string fpath;
		HANDLE fhnd;
		//int cnt;
		//int ncols;
		//int tcol;
	};
	VQSAVEALL *psaveAll;
	bool m_findall;
	int indexSearchStart;
	bool savedOnce;
	int CreateFixColumns();

	void ClserverReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	void ClserverSaveReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	void FIXServerReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

	int tsdate;
	int BrowseTimestamp(int dbegin, int tbegin);

	#ifdef SPLIT_ORD_DET
	static Cvsctest3View *newOrdDetView;
	Cvsctest3View *ordDetView;
	class CBinaryDlg *bdlg;
	#endif

// Generated message map functions
protected:
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSaveAll();
	afx_msg void OnSave();
	afx_msg void OnQueryAll();
	afx_msg void OnNotepad();
	afx_msg void OnQueryDetails();
	afx_msg void OnQueryRootOrderID();
	afx_msg void OnQueryFirstClOrdID();
	afx_msg void OnTextpad();
	afx_msg void OnExcel();
	afx_msg void OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	afx_msg void OnKey();
	afx_msg void OnUpdateKey(CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//afx_msg void OnDate();
	afx_msg void OnUpdateDate(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFindall(CCmdUI *pCmdUI);
	afx_msg void OnFindall();
	afx_msg void OnFixFilter();
	afx_msg void OnSummarizeOrders();
	afx_msg void OnSummarizeDetails();
	afx_msg void OnSummarizeIndex();
	afx_msg void On12AM();
	afx_msg void On1AM();
	afx_msg void On2AM();
	afx_msg void On3AM();
	afx_msg void On4AM();
	afx_msg void On5AM();
	afx_msg void On6AM();
	afx_msg void On7AM();
	afx_msg void On8AM();
	afx_msg void On9AM();
	afx_msg void On10AM();
	afx_msg void On11AM();
	afx_msg void On12PM();
	afx_msg void On1PM();
	afx_msg void On2PM();
	afx_msg void On3PM();
	afx_msg void On4PM();
	afx_msg void On5PM();
	afx_msg void On6PM();
	afx_msg void On7PM();
	afx_msg void On8PM();
	afx_msg void On9PM();
	afx_msg void On10PM();
	afx_msg void On11PM();
	afx_msg void OnRemoveOrder();
	afx_msg void OnRefreshQuery();
	afx_msg void OnTraceTheseOrders();
	afx_msg void OnTraceOrderAll();
	afx_msg void OnHidePositions();
	#ifdef SPLIT_ORD_DET
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	#endif
};

#ifndef _DEBUG  // debug version in vsctest3View.cpp
inline Cvsctest3Doc* Cvsctest3View::GetDocument() const
   { return reinterpret_cast<Cvsctest3Doc*>(m_pDocument); }
#endif

enum OrderCols
{
	OCOL_APPINSTID,
	OCOL_CLORDID,
	OCOL_ROOTORDERID,
	OCOL_FIRSTCLORDID,
	OCOL_SYMBOL,
	OCOL_ACCOUNT,
	OCOL_ECNORDERID,
	OCOL_CLIENTID,
	OCOL_PRICE,
	OCOL_SIDE,
	OCOL_ORDERQTY,
	OCOL_CUMQTY,
	OCOL_FILLQTY,
	OCOL_TERM,
	OCOL_HIGHMSGTYPE,
	OCOL_HIGHEXECTYPE,
	OCOL_TRANSACTTIME,
	OCOL_ORDERLOC,
	OCOL_CONNECTION,
	OCOL_CLPARENTORDERID,
	OCOL_ORDERDATE,
	//OCOL_ROUTEDORDERID,
	OCOL_ROUTINGBROKER,
	OCOL_SECURITYTYPE,
	OCOL_COUNT
};
