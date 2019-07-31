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

// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "vsctest3.h"

#include "vsctest3Doc.h"
#include "LeftView.h"
#include "MainFrm.h"
#include "TraceOrder.h"
#include "ChildFrm.h"
#include "TraceView.h"
#include "vsctest3View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CLeftView::OnTvnSelchanged)
	ON_MESSAGE(WM_USER +2, &CLeftView::OnSetTitle)
	ON_MESSAGE(WM_USER +3, &CLeftView::OnTraceView)
	ON_WM_RBUTTONUP()
	ON_NOTIFY_REFLECT(NM_RCLICK, &CLeftView::OnNMRClick)
	ON_COMMAND(IDC_TRACE_ORDER, &CLeftView::OnTraceOrder)
	ON_COMMAND(IDC_TRACE_ORDER_ALL, &CLeftView::OnTraceOrderAll)
	ON_COMMAND(ID_KEY, &CLeftView::OnKey)
	ON_UPDATE_COMMAND_UI(ID_KEY, &CLeftView::OnUpdateKey)
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_DATE, &CLeftView::OnDate)
	ON_UPDATE_COMMAND_UI(ID_DATE, &CLeftView::OnUpdateDate)
END_MESSAGE_MAP()


// CLeftView construction/destruction

CLeftView::CLeftView()
	:connNotify(0)
	,selitem(0)
	,curq()
	,syscnt(0)
	,instcnt(0)
	,tqlist()
	,ftqlist()
	,todlg(0)
	,tDocTemplate(0)
	,vDocTemplate(0)
	,rsitem(0)
	,orderSchema()
	,auxIndexNames()
{
	// TODO: add construction code here
}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	return CTreeView::PreCreateWindow(cs);
}

void CLeftView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	tDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
		RUNTIME_CLASS(Cvsctest3Doc),
		RUNTIME_CLASS(CChildFrame2),
		RUNTIME_CLASS(CTraceView));
	#ifdef SPLIT_ORD_DET
	vDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
		RUNTIME_CLASS(Cvsctest3Doc),
		RUNTIME_CLASS(CChildFrame3),
		RUNTIME_CLASS(Cvsctest3View));
	#else
	vDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
		RUNTIME_CLASS(Cvsctest3Doc),
		RUNTIME_CLASS(CChildFrame2),
		RUNTIME_CLASS(Cvsctest3View));
	#endif

	// TODO: You may populate your TreeView with items by directly accessing
	//  its tree control through a call to GetTreeCtrl().
	CTreeCtrl& ctc=GetTreeCtrl();
	ctc.ModifyStyle(0,TVS_HASLINES);
	ctc.ModifyStyle(0,TVS_LINESATROOT);
	ctc.ModifyStyle(0,TVS_HASBUTTONS);

	theApp.AddNotify(this);
	connNotify=&theApp;
}


// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

Cvsctest3Doc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Cvsctest3Doc)));
	return (Cvsctest3Doc*)m_pDocument;
}
#endif //_DEBUG


// CLeftView message handlers
// VSCNotify
void CLeftView::VSCNotifyError(int rc, const char *emsg)
{
}
void CLeftView::VSCNotifyEvent(int rc, const char *emsg)
{
}
// For EncodeLoginRequest
void CLeftView::VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
}
void CLeftView::VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
{
}
// For EncodeSqlRequest
void CLeftView::VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
}
void CLeftView::VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
{
	// TODO:
}
// New view creation must be done from UI thread
LRESULT CLeftView::OnTraceView(WPARAM wParam, LPARAM lParam)
{
	if(ftqlist.empty())
		return false;
	VSDBQUERY *tquery=ftqlist.front();
	if(!tquery)
		return false;

	CTraceView::newq.from="TRACEORDERS";
	char wclause[256]={0};
	strcpy(wclause,tquery->where);
	const char *wptr=strstr(wclause,"AppInstID=='");
	if(wptr)
	{
		wptr=strstr(wptr +12,"AND");
		if(wptr)
			memmove(wclause,wptr +4,strlen(wptr +4) +1);
	}
	CTraceView::newq.where=wclause;
	CTraceView::newq.traceSide=tquery->traceSide;
	CTraceView::newq.tracePrice=tquery->tracePrice;
	CTraceView::newq.traceShares=tquery->traceShares;
	CTraceView::newq.traceOrder=tquery->traceOrder;
	CTraceView::newq.traceIdType=tquery->traceIdType;

	int totcnt=0;
	for(list<VSDBQUERY *>::iterator qit=ftqlist.begin();qit!=ftqlist.end();qit++)
		totcnt+=(*qit)->nresults;
	if(totcnt<1)
	{
		CString emsg;
		emsg.Format("No results on trace WHERE %s",CTraceView::newq.where);
		CTraceView::newq.from="";
		CTraceView::newq.where="";
		for(list<VSDBQUERY *>::iterator qit=ftqlist.begin();qit!=ftqlist.end();qit++)
			delete *qit;
		ftqlist.clear();
		MessageBox(emsg,"Trace Orders",MB_ICONINFORMATION);
		return false;
	}

	CTraceView::ftqlist.swap(ftqlist);
	Cvsctest3Doc *pdoc=GetDocument();
	CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(tDocTemplate);
	CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("TraceOrder",0);
	pdoc->SetDocTemplate(ldt);

	CTraceView::newq.from="";
	return true;
}
void CLeftView::VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// Trace answers
	for(list<VSDBQUERY *>::iterator qit=tqlist.begin();qit!=tqlist.end();qit++)
	{
		VSDBQUERY *tquery=*qit;
		if(tquery->rid==rid)
		{
			for(int i=0;i<nstr;i++)
			{
				tquery->results.push_back(pstr[i]);
				tquery->nresults++;
			}

			if((!more)&&(hist))
			{
				// Auto-continue
				if(iter)
				{
					tquery->iter=iter;
					connNotify->VSCNotifySqlRequest((VSCNotify*)this,tquery->rid,tquery->select,tquery->from,tquery->where,tquery->maxorders,tquery->hist,tquery->live,tquery->iter);		
				}
				// This query done
				else
				{
					tqlist.erase(qit);
					ftqlist.push_back(tquery);
					// All done
					if(tqlist.empty())
						PostMessage(WM_USER +3,0,0);
					// Next query
					else
					{
						tquery=tqlist.front();
						connNotify->VSCNotifySqlRequest((VSCNotify*)this,tquery->rid,tquery->select,tquery->from,tquery->where,tquery->maxorders,tquery->hist,tquery->live,tquery->iter);		
					}
				}
			}
			break;
		}
	}
}
void CLeftView::VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
{
	if(!strcmp(from,"APPSYSTEMS"))
	{
		curq.rid=rid;
		curq.select=select;
		curq.from=from;
		curq.where=where;
		curq.maxorders=maxorders;
		curq.unique=unique;
		curq.istart=istart;
		curq.idir=idir;
		curq.iter=iter;
	}
	else if(!strcmp(from,"APPINSTANCE"))
	{
		CTreeCtrl& ctc=GetTreeCtrl();
		for(HTREEITEM aitem=ctc.GetRootItem();aitem;aitem=ctc.GetNextSiblingItem(aitem))
		{
			VSDBQUERY *iquery=(VSDBQUERY*)ctc.GetItemData(aitem);
			if((iquery)&&(iquery->rid==rid))
			{
				iquery->select=select;
				iquery->from=from;
				iquery->where=where;
				iquery->maxorders=maxorders;
				iquery->unique=unique;
				iquery->istart=istart;
				iquery->idir=idir;
				iquery->iter=iter;
				iquery->asys=ctc.GetItemText(aitem);
				iquery->nresults=0;
				break;
			}
		}
	}
}
static bool listfind(list<string>& slist, const char *val)
{
	if(slist.empty())
		return true;
	for(list<string>::iterator sit=slist.begin();sit!=slist.end();sit++)
	{
		if(!_stricmp(sit->c_str(),val))
			return true;
	}
	return false;
}
void CLeftView::VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// This gets crowded. Find a better way to convey the info.
	//if(!selitem)
	//	return;
	//CTreeCtrl& ctc=GetTreeCtrl();
	//CString selname=ctc.GetItemText(selitem);
	//if(strchr(selname,'('))
	//{
	//	if(!strstr(selname,"tot"))
	//	{
	//		CString tstr;
	//		tstr.Format(", tot %s",theApp.SizeStr(totcnt));
	//		selname+=tstr;
	//		ctc.SetItemText(selitem,selname);
	//	}
	//}
	CTreeCtrl& ctc=GetTreeCtrl();
	if(curq.from=="APPSYSTEMS")
	{
		if(rid!=curq.rid)
			return;
		CTreeCtrl& ctc=GetTreeCtrl();
		ctc.DeleteAllItems();
		for(int i=0;i<nstr;i++)
		{
			HTREEITEM aitem=ctc.InsertItem(pstr[i]);
			if(aitem)
			{
				char wclause[256]={0};
				sprintf(wclause,"AppSystem=='%s'",pstr[i]);
				VSDBQUERY *iquery=new VSDBQUERY;
				iquery->select="*";
				iquery->from="APPINSTANCE";
				iquery->where=wclause;
				iquery->rid=curq.rid +1 +i;
				ctc.SetItemData(aitem,(DWORD_PTR)iquery);
				connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,iquery->rid,iquery->select,iquery->from,iquery->where,0,true,0,0,0);
				syscnt++;

				CString smsg;
				smsg.Format("%d systems, %d instances",syscnt,instcnt);
				theApp.pMainFrame->SetStatus(0,smsg);
			}
		}
		curq.select="";
		curq.from="";
		curq.where="";
	}
	else
	{
		for(HTREEITEM aitem=ctc.GetRootItem();aitem;aitem=ctc.GetNextSiblingItem(aitem))
		{
			VSDBQUERY *iquery=(VSDBQUERY*)ctc.GetItemData(aitem);
			if((iquery)&&(iquery->rid==rid))
			{
				for(int i=0;i<nstr;i++)
				{
					HTREEITEM iitem=ctc.InsertItem(pstr[i],aitem);
					if(iitem)
					{
						// Only show indices found in the schema
						if(listfind(orderSchema,"OrderDate"))
							ctc.InsertItem("OrderDate(60)",iitem);
						if(listfind(orderSchema,"ClOrdID"))
							ctc.InsertItem("ClOrdID(11)",iitem);
						if(listfind(orderSchema,"RootOrderID"))
							ctc.InsertItem("RootOrderID(70129)",iitem);
						if(listfind(orderSchema,"FirstClOrdID"))
							ctc.InsertItem("FirstClOrdID(5055)",iitem);
						if(listfind(orderSchema,"ClParentOrderID"))
							ctc.InsertItem("ClParentOrderID(5035)",iitem);
						if(listfind(orderSchema,"Symbol"))
							ctc.InsertItem("Symbol(55,65)",iitem);
						if(listfind(orderSchema,"Account"))
							ctc.InsertItem("Account(1)",iitem);
						if(listfind(orderSchema,"EcnOrderID"))
							ctc.InsertItem("EcnOrderID(37)",iitem);
						if(listfind(orderSchema,"ClientID"))
							ctc.InsertItem("ClientID(109)",iitem);
						if(listfind(orderSchema,"TransactTime"))
							ctc.InsertItem("TransactTime(60)",iitem);
						if(listfind(orderSchema,"FillQty"))
							ctc.InsertItem("Filled Orders(14)...",iitem);
						if(listfind(orderSchema,"Term"))
							ctc.InsertItem("Open Orders(39)...",iitem);
						if(listfind(orderSchema,"ClOrdID"))
							ctc.InsertItem("Rejected Orders(39)...",iitem);
						if(listfind(orderSchema,"Connection"))
							ctc.InsertItem("Connection(73145,73146|49,50,56,57)",iitem);
						// Aux keys
						//ctc.InsertItem("RoutedOrderID(2100)",iitem);
						for(list<string>::iterator ait=auxIndexNames.begin();ait!=auxIndexNames.end();ait++)
							ctc.InsertItem(ait->c_str(),iitem);
						ctc.Expand(iitem,TVE_COLLAPSE);
						instcnt++;

						CString smsg;
						smsg.Format("%d systems, %d instances",syscnt,instcnt);
						theApp.pMainFrame->SetStatus(0,smsg);
					}
				}
				// For the [+] boxes to show up
				ctc.Expand(aitem,TVE_EXPAND);
				ctc.Expand(aitem,TVE_COLLAPSE);
				break;
			}
		}
	}
}
// For EncodeCancelRequest
void CLeftView::VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
{
}
// For EncodeDescribeRequest
void CLeftView::VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
{
	if((curq.select=="DESCRIBE")&&(curq.from==from)&&(curq.where=="")&&(!curq.nresults))
	{
		curq.rid=rid;
		orderSchema.clear();
		auxIndexNames.clear();
	}
}
void CLeftView::VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	if(rid!=curq.rid)
		return;
	// CSV format
	for(int i=0;i<nstr;i++)
	{
		const char *tok=strtoke((char*)pstr[i],",");
		if((!strcmp(tok,"Column"))||(!strcmp(tok,"AppInstID")))
			continue;
		string colname,desc;
		for(int c=0;tok;c++)
		{
			if(c==0)
				colname=tok;
			else if(c==3)
				desc=tok;
			tok=strtoke(0,",");
		}
		orderSchema.push_back(colname);
		if((colname!="ClOrdID")&&
		   (colname!="RootOrderID")&&
		   (colname!="FirstClOrdID")&&
		   (colname!="Symbol")&&
		   (colname!="Price")&&
		   (colname!="Side")&&
		   (colname!="Account")&&
		   (colname!="EcnOrderID")&&
		   (colname!="ClientID")&&
		   (colname!="OrderQty")&&
		   (colname!="CumQty")&&
		   (colname!="FillQty")&&
		   (colname!="Term")&&
		   (colname!="HighMsgType")&&
		   (colname!="HighExecType")&&
		   (colname!="TransactTime")&&
		   (colname!="OrderLoc")&&
		   (colname!="Connection")&&
		   (colname!="ClParentOrderID")&&
		   (colname!="OrderDate"))
		   auxIndexNames.push_back(colname +"(" +desc +")");
	}
	if(!more)
	{
		curq.select="";
		curq.from="";
		connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,0,"*","APPSYSTEMS","",0,true,0,0,0);
	}
}

// For EncodeLoginRequest2
void CLeftView::VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
}
// Setting the title must be done from UI thread
LRESULT CLeftView::OnSetTitle(WPARAM wParam, LPARAM lParam)
{
	Cvsctest3Doc *pdoc=GetDocument();
	char *tstr=(char *)lParam;
	pdoc->SetTitle(tstr);
	delete tstr;
	return true;
}
void CLeftView::VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
{
	char *tstr=new char[256];
	if(!rc)
	{
		CTreeCtrl& ctc=GetTreeCtrl();
		ctc.DeleteAllItems();
		selitem=0;
		curq.rid=0;
		curq.select="";
		curq.from="";
		curq.where="";
		syscnt=0;
		instcnt=0;
		sprintf(tstr,"Connected to %s (%08d)",name,wsdate);

		// Discover dynamic indices
		curq.select="DESCRIBE";
		curq.from="ORDERS";
		connNotify->VSCNotifyDescribeRequest((VSCNotify*)this,0,"ORDERS",0,0);
	}
	else
		sprintf(tstr,"Disconnected");
	PostMessage(WM_USER +2,0,(LPARAM)tstr);
}
// For EncodeSqlRequest2
void CLeftView::VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
{
	// Link with list of orders
	if(!strcmp(from,"LINKTRACEORDERSONLY"))
	{
		VSDBQUERY *tquery=(VSDBQUERY*)iter; _ASSERT(tquery);
		ftqlist.push_back(tquery);
		OnTraceView(0,0);
	}
	// Link from index or single order
	else if(!strcmp(from,"LINKTRACEORDERS"))
	{
		char orderid[128]={0},idtype[32]={0},symbol[64]={0};
		strcpy(idtype,"ClOrdID");
		if(strncmp(where,"Symbol==",8))
		{
			const char *cptr=where;
			const char *eptr=strstr(cptr,"==");
			if(eptr)
			{
				strncpy(idtype,cptr,(int)(eptr -cptr));
				if((!strcmp(idtype,"Filled Orders"))||(!strcmp(idtype,"Open Orders"))||(!strcmp(idtype,"Rejected Orders")))
					strcpy(idtype,"ClOrdID");
				cptr=eptr +3;
				eptr=strchr(cptr,'\'');
				if(!eptr) eptr=cptr +strlen(cptr);
				memcpy(orderid,cptr,(int)(eptr -cptr));
				orderid[(int)(eptr -cptr)]=0;
			}
		}
		const char *sptr=strstr(where,"Symbol==");
		if(sptr)
		{
			sptr+=9;
			const char *eptr=strchr(sptr,'\'');
			if(!eptr) eptr=sptr +strlen(sptr);
			memcpy(symbol,sptr,(int)(eptr -sptr));
			symbol[(int)(eptr -sptr)]=0;
		}
		if(!todlg)
			todlg=new CTraceOrder;
		CTreeCtrl& ctc=GetTreeCtrl();
		todlg->slist.clear();
		todlg->slist.push_back("ALL");
		for(HTREEITEM hitem=ctc.GetRootItem();hitem;hitem=ctc.GetNextSiblingItem(hitem))
			todlg->slist.push_back((const char *)ctc.GetItemText(hitem));
		todlg->idtype=idtype;
		todlg->m_orderid=orderid;
		todlg->m_symbol=symbol;
		todlg->side="ANY";
		if(!todlg->m_date)
		{
			CString dstr=theApp.pMainFrame->GetDateText();
			if(!dstr.IsEmpty())
				todlg->m_date=atoi(dstr);
		}
		if(todlg->DoModal()==IDOK)
			TraceOrder(todlg->slist,todlg->m_orderid,todlg->idtype.c_str(),
				todlg->m_symbol,todlg->side.c_str(),todlg->m_price,todlg->m_shares,todlg->m_date);
	}
}
// For EncodeSqlIndexRequest2
void CLeftView::VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
{
}
void CLeftView::VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
}
void CLeftView::VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
}

void CLeftView::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	CTreeCtrl& ctc=GetTreeCtrl();
	if(!pNMTreeView->itemNew.hItem)
		return;
	selitem=pNMTreeView->itemNew.hItem;
	CString selname=ctc.GetItemText(pNMTreeView->itemNew.hItem);
	// Browse query when index selected
	if(strchr(selname,'('))
	{
		HTREEITEM htp=ctc.GetParentItem(pNMTreeView->itemNew.hItem);
		CString parname=ctc.GetItemText(htp);
		char wclause[256]={0},kclause[128]={0},kname[256]={0};
		strcpy(kname,selname);
		char *kptr=strchr(kname,'('); *kptr=0;
		sprintf(wclause,"IndexName=='%s' AND AppInstID=='%s'%s",kname,(const char*)parname,kclause);
		CString dstr=theApp.pMainFrame->GetDateText();
		if(!dstr.IsEmpty())
		{
			//if(!_stricmp(kname,"OrderDate"))
			//	sprintf(wclause +strlen(wclause)," AND DBOrderDate==%s",dstr);
			//else
			if(_stricmp(kname,"OrderDate"))
				sprintf(wclause +strlen(wclause)," AND OrderDate==%s",dstr);
			if(!theApp.clientregion.empty())
				sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		}
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,0,"Key","INDICES",wclause,500,true,0,0,0);

		// Auto-launch an orders view for filled and open orders
		if(!strcmp(kname,"Filled Orders"))
		{
			char asys[64]={0};
			for(HTREEITEM sitem=pNMTreeView->itemNew.hItem;sitem;sitem=ctc.GetParentItem(sitem))
			{
				CString text=ctc.GetItemText(sitem);
				strcpy(asys,text);
				char *aptr=strchr(asys,'*');
				if(aptr) 
				{
					*aptr=0; break;
				}
			}
			Cvsctest3View::newq.asys=asys;
			Cvsctest3View::newq.select="*";
			Cvsctest3View::newq.from="ORDERS/DETAILS";
			char wclause[256]={0};
			sprintf(wclause,"AppInstID=='%s' AND FillQty>0",(const char*)parname);
			CString dstr=theApp.pMainFrame->GetDateText();
			if(!dstr.IsEmpty())
				sprintf(wclause +strlen(wclause)," AND OrderDate==%s",dstr);
			if(!theApp.clientregion.empty())
				sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
			Cvsctest3View::newq.where=wclause;
			Cvsctest3View::newq.hist=true;
			#ifdef LIVE_DETAILS
			Cvsctest3View::newq.live=true;
			#else
			Cvsctest3View::newq.live=false;
			#endif
			Cvsctest3View::newq.maxorders=500;
			Cvsctest3View::newq.iter=0;
			Cvsctest3View::newq.nresults=0;

			Cvsctest3Doc *pdoc=GetDocument();
			CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
			CString tstr;
			tstr.Format("%s Filled Orders",asys);
			CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow(tstr,0);
			pdoc->SetDocTemplate(ldt);
		}
		else if(!strcmp(kname,"Open Orders"))
		{
			char asys[64]={0};
			for(HTREEITEM sitem=pNMTreeView->itemNew.hItem;sitem;sitem=ctc.GetParentItem(sitem))
			{
				CString text=ctc.GetItemText(sitem);
				strcpy(asys,text);
				char *aptr=strchr(asys,'*');
				if(aptr) 
				{
					*aptr=0; break;
				}
			}
			Cvsctest3View::newq.asys=asys;
			Cvsctest3View::newq.select="*";
			Cvsctest3View::newq.from="ORDERS/DETAILS";
			char wclause[256]={0};
			sprintf(wclause,"AppInstID=='%s' AND Term==FALSE",(const char*)parname);
			CString dstr=theApp.pMainFrame->GetDateText();
			if(!dstr.IsEmpty())
				sprintf(wclause +strlen(wclause)," AND OrderDate==%s",dstr);
			if(!theApp.clientregion.empty())
				sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
			Cvsctest3View::newq.where=wclause;
			Cvsctest3View::newq.hist=true;
			#ifdef LIVE_DETAILS
			Cvsctest3View::newq.live=true;
			#else
			Cvsctest3View::newq.live=false;
			#endif
			Cvsctest3View::newq.maxorders=500;
			Cvsctest3View::newq.iter=0;
			Cvsctest3View::newq.nresults=0;

			Cvsctest3Doc *pdoc=GetDocument();
			CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
			CString tstr;
			tstr.Format("%s Open Orders",asys);
			CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow(tstr,0);
			pdoc->SetDocTemplate(ldt);
		}
		else if(!strcmp(kname,"Rejected Orders"))
		{
			char asys[64]={0};
			for(HTREEITEM sitem=pNMTreeView->itemNew.hItem;sitem;sitem=ctc.GetParentItem(sitem))
			{
				CString text=ctc.GetItemText(sitem);
				strcpy(asys,text);
				char *aptr=strchr(asys,'*');
				if(aptr) 
				{
					*aptr=0; break;
				}
			}
			Cvsctest3View::newq.asys=asys;
			Cvsctest3View::newq.select="*";
			Cvsctest3View::newq.from="ORDERS/DETAILS";
			char wclause[256]={0};
			sprintf(wclause,"AppInstID=='%s' AND (HighExecType=='8' OR HighExecType=='9')",(const char*)parname);
			CString dstr=theApp.pMainFrame->GetDateText();
			if(!dstr.IsEmpty())
				sprintf(wclause +strlen(wclause)," AND OrderDate==%s",dstr);
			if(!theApp.clientregion.empty())
				sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
			Cvsctest3View::newq.where=wclause;
			Cvsctest3View::newq.hist=true;
			Cvsctest3View::newq.live=false;
			Cvsctest3View::newq.maxorders=500;
			Cvsctest3View::newq.iter=0;
			Cvsctest3View::newq.nresults=0;

			Cvsctest3Doc *pdoc=GetDocument();
			CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
			CString tstr;
			tstr.Format("%s Rejected Orders",asys);
			CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow(tstr,0);
			pdoc->SetDocTemplate(ldt);
		}
	}
	*pResult = 0;
}

void CLeftView::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Add your control notification handler code here
	*pResult = 0;
	POINT point;
	GetCursorPos(&point);
	CTreeCtrl& ctc=GetTreeCtrl();

	HMENU hmenu = CreatePopupMenu();
	//HTREEITEM sitem=ctc.GetSelectedItem();
	// This is the item over which the cursor is pointing--not necessarily the last selected item
	HTREEITEM sitem;
	memcpy(&sitem,(char*)pNMHDR +16*5 -4,4);
	rsitem=sitem;
	DWORD mflags=MF_STRING;
	if(!sitem) mflags|=MF_GRAYED;
	if(!tqlist.empty()) mflags|=MF_GRAYED;
	AppendMenu(hmenu,mflags,IDC_TRACE_ORDER,"&Trace order this system...");
	mflags=MF_STRING;
	if(!tqlist.empty()) mflags|=MF_GRAYED;
	AppendMenu(hmenu,mflags,IDC_TRACE_ORDER_ALL,"&Trace order all systems...");

	TrackPopupMenu(hmenu, TPM_LEFTALIGN |TPM_TOPALIGN |TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, 0);
}
void CLeftView::OnTraceOrder()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=rsitem;//ctc.GetSelectedItem();
	if(!sitem)
		return;
	for(HTREEITEM parent=ctc.GetParentItem(sitem);parent;parent=ctc.GetParentItem(sitem))
		sitem=parent;
	if(!todlg)
	{
		todlg=new CTraceOrder;
		todlg->idtype="ClOrdID";
		todlg->side="ANY";
	}
	todlg->slist.clear();
	todlg->slist.push_back((const char *)ctc.GetItemText(sitem));
	if(todlg->DoModal()==IDOK)
		TraceOrder(todlg->slist,todlg->m_orderid,todlg->idtype.c_str(),
			todlg->m_symbol,todlg->side.c_str(),todlg->m_price,todlg->m_shares,todlg->m_date);
}
void CLeftView::OnTraceOrderAll()
{
	if(!todlg)
	{
		todlg=new CTraceOrder;
		todlg->idtype="ClOrdID";
		todlg->side="ANY";
	}
	CTreeCtrl& ctc=GetTreeCtrl();
	todlg->slist.clear();
	todlg->slist.push_back("ALL");
	for(HTREEITEM hitem=ctc.GetRootItem();hitem;hitem=ctc.GetNextSiblingItem(hitem))
		todlg->slist.push_back((const char *)ctc.GetItemText(hitem));
	if(todlg->DoModal()==IDOK)
		TraceOrder(todlg->slist,todlg->m_orderid,todlg->idtype.c_str(),
			todlg->m_symbol,todlg->side.c_str(),todlg->m_price,todlg->m_shares,todlg->m_date);
}
int CLeftView::TraceOrder(list<string>& slist, const char *orderid, const char *idtype,
						  const char *symbol, const char *side, double price, int shares, int wsdate)
{
	if((!strlen(orderid))&&(!strlen(symbol)))
		return -1;
	int i=0;
	for(list<string>::iterator sit=slist.begin();sit!=slist.end();sit++)
	{
		char wclause[1024]={0};
		sprintf(wclause,"AppInstID=='%s*' AND ",sit->c_str());
		if(strlen(orderid)>0)
			sprintf(wclause +strlen(wclause),"%s=='%s'",idtype,orderid);
		else if(strlen(symbol)>0)
		{
			// e.g. "AMT PRA" is traded "AMT.PRA"
			char tsym[32]={0};
			if((strchr(symbol,' '))&&(*sit!="RTOATS"))
			{
				strcpy(tsym,symbol);
				char *tptr=strchr(tsym,' ');
				if(tptr) *tptr='.';
				sprintf(wclause +strlen(wclause),"Symbol=='%s'",tsym);
			}
			// e.g. "AMT.PRA" is reported "AMT PRA"
			else if((strchr(symbol,'.'))&&(*sit=="RTOATS"))
			{
				strcpy(tsym,symbol);
				char *tptr=strchr(tsym,'.');
				if(tptr) *tptr=' ';
				sprintf(wclause +strlen(wclause),"Symbol=='%s'",tsym);
			}
			else
				sprintf(wclause +strlen(wclause),"Symbol=='%s'",symbol);
		}
		if(wsdate)
			sprintf(wclause +strlen(wclause)," AND OrderDate==%08d",wsdate);
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());

		VSDBQUERY *tquery=new VSDBQUERY;
		tquery->select="*";
		tquery->from="ORDERS";
		tquery->where=wclause;
		tquery->rid=curq.rid +i; i++;
		tquery->hist=true;
		tquery->live=false;
		tquery->iter=0;
		tquery->asys=sit->c_str();
		tquery->traceSide=side;
		tquery->tracePrice=price;
		tquery->traceShares=shares;
		tquery->traceOrder=orderid;
		tquery->traceIdType=idtype;
		tquery->traceDate=wsdate;
		tqlist.push_back(tquery);
	}
	if(!tqlist.empty())
	{
		VSDBQUERY *tquery=tqlist.front();
		connNotify->VSCNotifySqlRequest((VSCNotify*)this,tquery->rid,tquery->select,tquery->from,tquery->where,tquery->maxorders,tquery->hist,tquery->live,tquery->iter);		
	}
	return 0;
}
static HTREEITEM RecurseFindKey(CTreeCtrl& ctc, HTREEITEM sitem, const CString& key, bool includeItem)
{
	CString text=ctc.GetItemText(sitem);
	text.MakeUpper();
	// BFS partial match on the key
	if((includeItem)&&(text.Find(key)>=0))
	{
		ctc.SelectItem(sitem);
		ctc.EnsureVisible(sitem);
		return sitem;
	}
	for(HTREEITEM citem=ctc.GetChildItem(sitem);citem;citem=ctc.GetNextSiblingItem(citem))
	{
		HTREEITEM fitem=RecurseFindKey(ctc,citem,key,true);
		if(fitem)
			return fitem;
	}
	return 0;
}
void CLeftView::OnKey()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	CString key=theApp.pMainFrame->GetKeyText();
	key.MakeUpper();
	HTREEITEM fitem=0;
	HTREEITEM sitem=ctc.GetSelectedItem();

	// If an item is selected, start after the selected item
	bool includeItem=false;
	while(sitem)
	{
		// Selected item's children
		fitem=RecurseFindKey(ctc,sitem,key,includeItem);
		if(fitem)
			break;
		includeItem=true;
		// Selected item's siblings
		if(!fitem)
		{
			for(HTREEITEM nitem=ctc.GetNextSiblingItem(sitem);nitem;nitem=ctc.GetNextSiblingItem(nitem))
			{
				fitem=RecurseFindKey(ctc,nitem,key,true);
				if(fitem)
					break;
			}
		}
		if(fitem)
			break;
		// Parent's next sibling
		sitem=ctc.GetParentItem(sitem);
		if(sitem) sitem=ctc.GetNextSiblingItem(sitem);
	}
	// Last search at root to find before selected item
	if(!fitem)
		fitem=RecurseFindKey(ctc,0,key,true);
}
void CLeftView::OnUpdateKey(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}
// F3 on the list
void CLeftView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar==VK_F3)
		OnKey();

	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}
void CLeftView::OnDate()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=ctc.GetSelectedItem();
	if(sitem)
	{
		ctc.SelectItem(0);
		ctc.SelectItem(sitem);
	}
}
void CLeftView::OnUpdateDate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}
