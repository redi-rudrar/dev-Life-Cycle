#pragma once


// CTraceView view

class CTraceView : public CTreeView
	//,public VSCNotify
{
	DECLARE_DYNCREATE(CTraceView)

	//// VSCNotify
	//void VSCNotifyError(int rc, const char *emsg);
	//void VSCNotifyEvent(int rc, const char *emsg);
	//// For EncodeLoginRequest
	//void VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen);
	//void VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason);
	//// For EncodeSqlRequest
	//void VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);
	//void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason);
	//void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	//void VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter);
	//void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	//// For EncodeCancelRequest
	//void VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter);
	//// For EncodeDescribeRequest
	//void VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter);
	//void VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

	//// For EncodeLoginRequest2
	//void VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen);
	//void VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate);
	//// For EncodeSqlRequest2
	//void VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl);
	//// For EncodeSqlIndexRequest2
	//void VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl);

	//void VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);
	//void VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

protected:
	CTraceView();           // protected constructor used by dynamic creation
	virtual ~CTraceView();

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

	//class VSCNotify *connNotify;
	VSDBQUERY curq;
	static VSDBQUERY newq;
	static list<VSDBQUERY *> ftqlist;
	list<class TraceApp *> alist;
	HTREEITEM rselitem;
	int LoadTraceResults();
	int MatchOrders(list<class TRACEORDER *>& plist, list<class TraceApp *>& alist);
	int GroupParents();
	int LaunchTextViewer(const char *fpath);

	CMultiDocTemplate* vDocTemplate;
	bool autoRClick;
	void OnQueryCRChain(int idx);
	void OnQueryPCChain(int idx);
 	void OnQueryOrder(int idx);
	void OnGroup(int idx);
	void OnSave(bool all);

	CString smsg;

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
	afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnQueryCRChain1();
	afx_msg void OnQueryCRChain2();
	afx_msg void OnQueryCRChain3();
	afx_msg void OnQueryCRChain4();
	afx_msg void OnQueryCRChain5();
	afx_msg void OnQueryCRChain6();
	afx_msg void OnQueryCRChain7();
	afx_msg void OnQueryCRChain8();
	afx_msg void OnQueryCRChain9();
	afx_msg void OnQueryCRChain10();
	afx_msg void OnQueryPCChain1();
	afx_msg void OnQueryPCChain2();
	afx_msg void OnQueryPCChain3();
	afx_msg void OnQueryPCChain4();
	afx_msg void OnQueryPCChain5();
	afx_msg void OnQueryPCChain6();
	afx_msg void OnQueryPCChain7();
	afx_msg void OnQueryPCChain8();
	afx_msg void OnQueryPCChain9();
	afx_msg void OnQueryPCChain10();
	afx_msg void OnQueryOrder1();
	afx_msg void OnQueryOrder2();
	afx_msg void OnQueryOrder3();
	afx_msg void OnQueryOrder4();
	afx_msg void OnQueryOrder5();
	afx_msg void OnQueryOrder6();
	afx_msg void OnQueryOrder7();
	afx_msg void OnQueryOrder8();
	afx_msg void OnQueryOrder9();
	afx_msg void OnQueryOrder10();
	afx_msg void OnDestroy();
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnExpandAll();
	afx_msg void OnCollapseAll();
	afx_msg void OnKey();
	afx_msg void OnUpdateKey(CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	afx_msg void OnGroup1();
	afx_msg void OnGroup2();
	afx_msg void OnGroup3();
	afx_msg void OnGroup4();
	afx_msg void OnGroup5();
	afx_msg void OnGroup6();
	afx_msg void OnGroup7();
	afx_msg void OnGroup8();
	afx_msg void OnGroup9();
	afx_msg void OnGroup10();
	afx_msg void OnGroup11();
	afx_msg void OnGroup12();
	afx_msg void OnGroup13();
	afx_msg void OnGroup14();
	afx_msg void OnGroup15();
	afx_msg void OnGroup16();
	afx_msg void OnGroup17();
	afx_msg void OnGroup18();
	afx_msg void OnGroup19();
	afx_msg void OnGroup20();
	afx_msg void OnGroup21();
	afx_msg void OnGroup22();
	afx_msg void OnGroup23();
	afx_msg void OnGroupNone();
	afx_msg void OnGroupReorder();
	afx_msg void OnRemoveFromTrace();
	afx_msg void OnSaveTrace();
	afx_msg void OnSaveTraceAll();
	#ifdef OWNER_PAINT
	virtual void OnPaint();
	#endif
	afx_msg void OnTraceToOrdersView();
};


