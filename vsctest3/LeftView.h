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

// LeftView.h : interface of the CLeftView class
//


#pragma once

class Cvsctest3Doc;

// Tree view of app instances by class
class CLeftView : public CTreeView
	,public VSCNotify
{
protected: // create from serialization only
	CLeftView();
	DECLARE_DYNCREATE(CLeftView)

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
	Cvsctest3Doc* GetDocument();

// Operations
public:

// Overrides
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	virtual ~CLeftView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	class VSCNotify *connNotify;
	HTREEITEM selitem;
	VSDBQUERY curq;
	int syscnt;
	int instcnt;
	list<VSDBQUERY *> tqlist;
	list<VSDBQUERY *> ftqlist;
	CMultiDocTemplate *tDocTemplate;
	CMultiDocTemplate *vDocTemplate;
	class CTraceOrder *todlg;
	HTREEITEM rsitem;
	int TraceOrder(list<string>& slist, const char *orderid, const char *idtype,
		const char *symbol, const char *side, double price, int shares, int wsdate);

	list<string> orderSchema;
	list<string> auxIndexNames;

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnSetTitle(WPARAM wParam, LPARAM lParam);
	afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTraceOrder();
	afx_msg void OnTraceOrderAll();
	afx_msg LRESULT OnTraceView(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKey();
	afx_msg void OnUpdateKey(CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDate();
	afx_msg void OnUpdateDate(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline Cvsctest3Doc* CLeftView::GetDocument()
   { return reinterpret_cast<Cvsctest3Doc*>(m_pDocument); }
#endif

