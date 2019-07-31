f// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// http://msdn.microsoft.com/officeui.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// vsctest3View.cpp : implementation of the Cvsctest3View class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "vsctest3.h"
#endif

#include "vsctest3Doc.h"
#include "vsctest3View.h"
#include "wstring.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "FixFilterDlg.h"
#include "BinaryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum IndexValueCols
{
	IVCOL_APPSYSTEM,
	IVCOL_KEY,
	IVCOL_POSITION,
	IVCOL_ORDER,
	IVCOL_COUNT
};
enum SummaryCols
{
	SCOL_TEXT,
	SCOL_SAMPLES,
	SCOL_LOW,
	SCOL_HIGH,
	SCOL_COUNT
};
enum FixCols
{
	//FCOL_HIST,
	FCOL_POSITION,
	FCOL_FIXLEN,
	FCOL_FIXMSG,
	FCOL_COUNT
};
enum ResultCols // BTR
{
	//RCOL_HIST,
	RCOL_FIXLEN,
	RCOL_ACCOUNT,
	RCOL_ACTION,
	RCOL_AON,
	RCOL_AAVGPX,
	RCOL_BOFAEXECUTIONSERVICESDOMAIN,
	RCOL_BOFAEXECUTIONSERVICESORDER,
	RCOL_CANCELLED,
	RCOL_CANCELLED_BY_EXCHANGE,
	RCOL_CANCEL_REQUESTED,
	RCOL_DISCRETIONARYAMOUNT,
	RCOL_DOMAIN,
	RCOL_DONOTCLEAR,
	RCOL_ECNID,
	RCOL_FIXLINEID,
	RCOL_ECNORDERNO,
	RCOL_EDITEDBYBO,
	RCOL_EXSYMBOL_CURRENCYCODE,
	RCOL_EXSYMBOL_EXCHANGE,
	RCOL_EXSYMBOL_SYMBOL,
	RCOL_EXSYMBOL_REGION,
	RCOL_EXT,
	RCOL_FILEDATE,
	RCOL_FOK,
	RCOL_ARCANOW,
	RCOL_LASTDATE,
	RCOL_LASTSTATUS,
	RCOL_LASTTIME,
	RCOL_LASTPX,
	RCOL_LASTSHARES,
	RCOL_EXESHARES,
	RCOL_OPENSHARES,
	RCOL_PREVOPENSHARES,
	RCOL_CLOSEDSHARES,
	RCOL_MASTERSLAVE,
	RCOL_MMID,
	RCOL_ONCLOSE,
	RCOL_ONOPEN,
	RCOL_ORDERNO,
	RCOL_CANCELMEMO,
	RCOL_ORDERWASCANCELREPLACED,
	RCOL_PEGBEST,
	RCOL_PEGLAST,
	RCOL_PEGMMID,
	RCOL_PEGMKT,
	//RCOL_PEGORDER,
	RCOL_PEGPRIME,
	RCOL_PENDING,
	RCOL_PREFECN,
	RCOL_PREFMMID,
	RCOL_PREVORDERNUMBER,
	RCOL_PRICE,
	RCOL_REPLACEREQUEST,
	RCOL_RESERVEDSHARES,
	RCOL_SHARES,
	RCOL_STOPAMOUNT,
	RCOL_STOPTYPE,
	RCOL_TRADEDETREF,
	RCOL_TYPE,
	RCOL_USER,
	RCOL_USERTYPE,
	RCOL_WORKORDERREFNO,
	RCOL_ECNPARSERORDERNO,
	RCOL_LINKORDERNO,
	RCOL_MEMO,
	RCOL_OATSCONNECTIONID,
	RCOL_OATSMANUALTICKET,
	RCOL_OATSNONDIRECTEDORDER,
	RCOL_ORDERTYPE,
	RCOL_SHORTEXEMPTCODE,
	RCOL_COMPLEXORDER,
	RCOL_COMPLEXORDERPARENT,
	RCOL_ORDERSTAGINGORDER,
	RCOL_ORDERSTAGINGPARENT,
	RCOL_NOTHELDORDER,
	RCOL_NOTOATSREPORTABLE,
	RCOL_OVDSPCLHNDLING,
	RCOL_TRADERGIVEUP,
	RCOL_WRKSPCLHNDLING,
	RCOL_STRATEGYNAME,
	RCOL_OVERRIDECOMMS,
	RCOL_COMMISSIONOVERRIDETYPE,
	RCOL_COMMISSIONOVERRIDE,
	RCOL_TRANTYPECODE,
	RCOL_REDIDESTINATION,
	RCOL_CURRENCY,
	RCOL_SETTLEMENT_CURRENCY,
	RCOL_PRODUCT_TYPE,
	RCOL_REGION,
	RCOL_CLOSEPOSITION,
	RCOL_POSITION,
	RCOL_COUNT
};
enum AccountsCals
{
	//ACOL_HIST,
	ACOL_LEN,
	ACOL_ACCOUNT,
	ACOL_DOMAIN,
	ACOL_POSITION,
	ACOL_COUNT
};

#pragma pack(push,8)
#include "dbcommon.h"
#ifdef LINK_TRADERLOOK
 // Big placelog: shoud be 960 bytes
typedef struct tdPlaceLogItem2
{
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponceTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , recieved from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
    // New one from trader-dev
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem2 *NextPlaceLogItem;
} PLACELOGITEM2;
#endif
#pragma pack(pop)

VSDBQUERY Cvsctest3View::newq;
#ifdef SPLIT_ORD_DET
Cvsctest3View *Cvsctest3View::newOrdDetView=0;
#endif
// Cvsctest3View

IMPLEMENT_DYNCREATE(Cvsctest3View, CListView)

BEGIN_MESSAGE_MAP(Cvsctest3View, CListView)
	ON_WM_STYLECHANGED()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_NOTIFY_REFLECT(NM_DBLCLK, &Cvsctest3View::OnNMDblclk)
	ON_COMMAND(IDC_SAVEALL, &Cvsctest3View::OnSaveAll)
	ON_COMMAND(IDC_SAVE, &Cvsctest3View::OnSave)
	ON_COMMAND(IDC_QUERYALL, &Cvsctest3View::OnQueryAll)
	ON_COMMAND(IDC_NOTEPAD, &Cvsctest3View::OnNotepad)
	ON_COMMAND(IDC_TEXTPAD, &Cvsctest3View::OnTextpad)
	ON_COMMAND(IDC_EXCEL, &Cvsctest3View::OnExcel)
	ON_COMMAND(IDC_SUMMARIZE_INDEX, &Cvsctest3View::OnSummarizeIndex)
	ON_COMMAND(IDC_SUMMARIZE_ORDERS, &Cvsctest3View::OnSummarizeOrders)
	ON_COMMAND(IDC_QUERY_ROOTORDERID, &Cvsctest3View::OnQueryRootOrderID)
	ON_COMMAND(IDC_QUERY_FIRSTCLORDID, &Cvsctest3View::OnQueryFirstClOrdID)
	ON_COMMAND(IDC_QUERY_DETAILS, &Cvsctest3View::OnQueryDetails)
	ON_COMMAND(IDC_SUMMARIZE_DETAILS, &Cvsctest3View::OnSummarizeDetails)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, &Cvsctest3View::OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, &Cvsctest3View::OnHdnItemclick)
	ON_COMMAND(ID_KEY, &Cvsctest3View::OnKey)
	ON_UPDATE_COMMAND_UI(ID_KEY, &Cvsctest3View::OnUpdateKey)
	ON_WM_KEYDOWN()
	//ON_COMMAND(ID_DATE, &Cvsctest3View::OnDate)
	ON_UPDATE_COMMAND_UI(ID_DATE, &Cvsctest3View::OnUpdateDate)
	ON_UPDATE_COMMAND_UI(ID_FINDALL, &Cvsctest3View::OnUpdateFindall)
	ON_COMMAND(ID_FINDALL, &Cvsctest3View::OnFindall)
	ON_COMMAND(IDC_FIXFILTER, &Cvsctest3View::OnFixFilter)
	ON_COMMAND(IDC_12AM, &Cvsctest3View::On12AM)
	ON_COMMAND(IDC_1AM, &Cvsctest3View::On1AM)
	ON_COMMAND(IDC_2AM, &Cvsctest3View::On2AM)
	ON_COMMAND(IDC_3AM, &Cvsctest3View::On3AM)
	ON_COMMAND(IDC_4AM, &Cvsctest3View::On4AM)
	ON_COMMAND(IDC_5AM, &Cvsctest3View::On5AM)
	ON_COMMAND(IDC_6AM, &Cvsctest3View::On6AM)
	ON_COMMAND(IDC_7AM, &Cvsctest3View::On7AM)
	ON_COMMAND(IDC_8AM, &Cvsctest3View::On8AM)
	ON_COMMAND(IDC_9AM, &Cvsctest3View::On9AM)
	ON_COMMAND(IDC_10AM, &Cvsctest3View::On10AM)
	ON_COMMAND(IDC_11AM, &Cvsctest3View::On11AM)
	ON_COMMAND(IDC_12PM, &Cvsctest3View::On12PM)
	ON_COMMAND(IDC_1PM, &Cvsctest3View::On1PM)
	ON_COMMAND(IDC_2PM, &Cvsctest3View::On2PM)
	ON_COMMAND(IDC_3PM, &Cvsctest3View::On3PM)
	ON_COMMAND(IDC_4PM, &Cvsctest3View::On4PM)
	ON_COMMAND(IDC_5PM, &Cvsctest3View::On5PM)
	ON_COMMAND(IDC_6PM, &Cvsctest3View::On6PM)
	ON_COMMAND(IDC_7PM, &Cvsctest3View::On7PM)
	ON_COMMAND(IDC_8PM, &Cvsctest3View::On8PM)
	ON_COMMAND(IDC_9PM, &Cvsctest3View::On9PM)
	ON_COMMAND(IDC_10PM, &Cvsctest3View::On10PM)
	ON_COMMAND(IDC_11PM, &Cvsctest3View::On11PM)
	ON_COMMAND(ID_REMOVE_ORDER, &Cvsctest3View::OnRemoveOrder)
	ON_COMMAND(ID_REFRESH_QUERY, &Cvsctest3View::OnRefreshQuery)
	ON_COMMAND(IDC_TRACE_ORDER, &Cvsctest3View::OnTraceTheseOrders)
	ON_COMMAND(IDC_TRACE_ORDER_ALL, &Cvsctest3View::OnTraceOrderAll)
	ON_COMMAND(ID_HIDE_POSITIONS, &Cvsctest3View::OnHidePositions)
	#ifdef SPLIT_ORD_DET
	ON_NOTIFY_REFLECT(NM_CLICK, &Cvsctest3View::OnNMClick)
	#endif
END_MESSAGE_MAP()

// Cvsctest3View construction/destruction

Cvsctest3View::Cvsctest3View()
	:connNotify(0)
	,vDocTemplate(0)
	,tpdlg(0)
	,osortcol(OCOL_ORDERLOC +1)
	,btrsortcol(RCOL_POSITION +1)
	,fixsortcol(1)
	,curq()
	,smsg()
	,moreToGet(false)
	,eventlog(false)
	,datRequested(false)
	,psaveAll(0)
	,m_findall(false)
	,indexSearchStart(-1)
	,savedOnce(false)
	#ifdef SPLIT_ORD_DET
	,ordDetView(0)
	,detDocTemplate(0)
	#endif
{
	// TODO: add construction code here
}

Cvsctest3View::~Cvsctest3View()
{
}

BOOL Cvsctest3View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CListView::PreCreateWindow(cs);
}

void Cvsctest3View::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().
	CListCtrl& clc=GetListCtrl();
	clc.ModifyStyle(0,LVS_REPORT);
	clc.ModifyStyle(0,LVS_SHOWSELALWAYS);
	DWORD estyle=clc.GetExtendedStyle();
	estyle&=~LVS_EX_TRACKSELECT;
	estyle|=LVS_EX_FULLROWSELECT;
	estyle|=LVS_EX_SUBITEMIMAGES;
	clc.SetExtendedStyle(estyle);
	clc.SetImageList(&theApp.ilist,LVSIL_NORMAL);
	if((newq.select!="SUMMARY")&&(newq.from!="INDICES"))
		clc.SetImageList(&theApp.ilist,LVSIL_SMALL); // required for report view
	Cvsctest3Doc *pdoc=GetDocument();

	#ifdef TIME_CONVERT
	if((newq.asys=="CLSERVER")||(newq.asys=="RTOATS")||(newq.asys=="TWIST"))
		newq.isgmt=false;
	else
		newq.isgmt=true;
	#endif
	// Index browse view
	if(newq.from=="INDICES")
	{
		LVCOLUMN ivcols[]={
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"App System",0,IVCOL_APPSYSTEM,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,200,"Key",0,IVCOL_KEY,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"Pos",0,IVCOL_POSITION,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,90,"OrderLoc",0,IVCOL_ORDER,0,0},
			{0,0,0,0,0},
		};
		for(int i=0;ivcols[i].mask;i++)
			clc.InsertColumn(i,&ivcols[i]);

		GetParent()->GetParent()->SetWindowText("Indices");	// ChildFrm doesn't have FWS_ADDTOTITLE
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

		curq=newq;
		theApp.AddNotify(this);
		connNotify=&theApp;
	}
	// Orders view
	#ifdef SPLIT_ORD_DET
	else if((newq.from=="ORDERS")||(newq.from=="ORDERS/DETAILS"))
	#else
	else if(newq.from=="ORDERS")
	#endif
	{
		if(newq.select=="SUMMARY")
		{
			LVCOLUMN scols[]={
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,200,"Summary",0,SCOL_TEXT,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,860,"Sample",0,SCOL_SAMPLES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,150,"Low",0,SCOL_LOW,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,150,"High",0,SCOL_HIGH,0,0},
				{0,0,0,0,0},
			};
			for(int i=0;scols[i].mask;i++)
				clc.InsertColumn(i,&scols[i]);

			curq=newq;
			CString title;
			title.Format("Summary %s",curq.where);
			GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE

			for(list<string>::iterator sit=newq.results.begin();sit!=newq.results.end();sit++)
			{
				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
				{
					clc.SetItemText(nidx,SCOL_TEXT,sit->c_str()); sit++;
					if(sit==newq.results.end()) break;
					clc.SetItemText(nidx,SCOL_SAMPLES,sit->c_str()); sit++;
					if(sit==newq.results.end()) break;
					clc.SetItemText(nidx,SCOL_LOW,sit->c_str()); sit++;
					if(sit==newq.results.end()) break;
					clc.SetItemText(nidx,SCOL_HIGH,sit->c_str());
				}
			}
			newq.results.clear();
		}
		else
		{
			LVCOLUMN ocols[]={
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"AppInstID",0,OCOL_APPINSTID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,200,"ClOrdID",0,OCOL_CLORDID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,200,"RootOrderID",0,OCOL_ROOTORDERID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,200,"FirstClOrdID",0,OCOL_FIRSTCLORDID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Symbol",0,OCOL_SYMBOL,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,80,"Account",0,OCOL_ACCOUNT,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,200,"EcnOrderID",0,OCOL_ECNORDERID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,80,"ClientID",0,OCOL_CLIENTID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,80,"Price",0,OCOL_PRICE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,30,"Side",0,OCOL_SIDE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"OrderQty",0,OCOL_ORDERQTY,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"CumQty",0,OCOL_CUMQTY,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"FillQty",0,OCOL_FILLQTY,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Term",0,OCOL_TERM,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,30,"HighMsgType",0,OCOL_HIGHMSGTYPE,0,0},
				//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_IMAGE|LVCF_FMT,LVCFMT_IMAGE,30,"HighExecType",0,OCOL_HIGHEXECTYPE,3,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,30,"HighExecType",0,OCOL_HIGHEXECTYPE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,150,"TransactTime",0,OCOL_TRANSACTTIME,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,80,"OrderLoc",0,OCOL_ORDERLOC,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,220,"Connection",0,OCOL_CONNECTION,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,120,"ClParentOrderID",0,OCOL_CLPARENTORDERID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OrderDate",0,OCOL_ORDERDATE,0,0},
				//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,120,"RoutedOrderID",0,OCOL_ROUTEDORDERID,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"RoutingBroker",0,OCOL_ROUTINGBROKER,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"SecurityType",0,OCOL_SECURITYTYPE,0,0},
				{0,0,0,0,0},
			};
			for(int i=0;ocols[i].mask;i++)
				clc.InsertColumn(i,&ocols[i]);

			#ifdef SPLIT_ORD_DET
			vDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
				RUNTIME_CLASS(Cvsctest3Doc),
				RUNTIME_CLASS(CChildFrame3),
				RUNTIME_CLASS(Cvsctest3View));
			detDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
				RUNTIME_CLASS(Cvsctest3Doc),
				RUNTIME_CLASS(CChildFrame2),
				RUNTIME_CLASS(Cvsctest3View));
			#else
			vDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
				RUNTIME_CLASS(Cvsctest3Doc),
				RUNTIME_CLASS(CChildFrame2),
				RUNTIME_CLASS(Cvsctest3View));
			#endif
			theApp.AddNotify(this);
			connNotify=&theApp;

			curq=newq;
			// Preload orders
			if(curq.select=="TRACEORDERS")
			{
				CString title;
				title.Format("Trace(%s*) %s",curq.asys,curq.where);
				#ifdef SPLIT_ORD_DET
				if(curq.from=="ORDERS/DETAILS")
				{
					curq.from="ORDERS";
					GetParent()->GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE
					newOrdDetView=this;
				}
				else
				#endif
				GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE
				curq.results.swap(newq.results);
				const char **pstr=new const char *[curq.nresults];
				memset(pstr,0,curq.nresults*sizeof(const char *));
				int i=0;
				for(list<string>::iterator rit=curq.results.begin();rit!=curq.results.end();rit++)
					pstr[i++]=rit->c_str();
				VSCNotifySqlDsvReply((VSCNotify*)this,0,curq.rid,curq.nresults,curq.nresults,true,0x07,pstr,curq.nresults,false,0,0);
				delete pstr;
				curq.results.clear();		
			}
			// Query orders
			else
			{
				CString title;
				title.Format("Orders %s",curq.where);
				#ifdef SPLIT_ORD_DET
				if(curq.from=="ORDERS/DETAILS")
				{
					curq.from="ORDERS";
					GetParent()->GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE
					newOrdDetView=this;
				}
				else
				#endif
				GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE
				curq.wresults=500; curq.nresults=0;
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
			}
		}
		#ifdef SPLIT_ORD_DET
		if(newq.from=="ORDERS/DETAILS")
			newq.from="DETAILS/ORDERS";
		#endif
	}
	// Details view
	#ifdef SPLIT_ORD_DET
	else if((newq.from=="DETAILS")||(newq.from=="DETAILS/ORDERS"))
	#else
	else if(newq.from=="DETAILS")
	#endif
	{
		if(newq.select=="SUMMARY")
		{
			LVCOLUMN scols[]={
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,160,"Summary",0,SCOL_TEXT,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,860,"Sample",0,SCOL_SAMPLES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,150,"Low",0,SCOL_LOW,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,150,"High",0,SCOL_HIGH,0,0},
				{0,0,0,0,0},
			};
			for(int i=0;scols[i].mask;i++)
				clc.InsertColumn(i,&scols[i]);

			curq=newq;
			CString title;
			title.Format("Summary %s",curq.where);
			GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE

			for(list<string>::iterator sit=newq.results.begin();sit!=newq.results.end();sit++)
			{
				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
				{
					clc.SetItemText(nidx,SCOL_TEXT,sit->c_str()); sit++;
					if(sit==newq.results.end()) break;
					clc.SetItemText(nidx,SCOL_SAMPLES,sit->c_str()); sit++;
					if(sit==newq.results.end()) break;
					clc.SetItemText(nidx,SCOL_LOW,sit->c_str()); sit++;
					if(sit==newq.results.end()) break;
					clc.SetItemText(nidx,SCOL_HIGH,sit->c_str());
				}
			}
			newq.results.clear();
			return;
		}
		if((newq.asys=="CLSERVER")||(theApp.btrdetails))
		{
			LVCOLUMN rcolsBtr[]={
				//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,20,"Hist",0,RCOL_HIST,0,0},                         
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Len",0,RCOL_FIXLEN,0,0},                        
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Account",0,RCOL_ACCOUNT,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Action",0,RCOL_ACTION,0,0},                     
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"AON",0,RCOL_AON,0,0},                        
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"AvgPx",0,RCOL_AAVGPX,0,0},                      
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"BofaExecutionServicesDomain",0,RCOL_BOFAEXECUTIONSERVICESDOMAIN,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"BofaExecutionServicesOrder",0,RCOL_BOFAEXECUTIONSERVICESORDER,0,0}, 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Canceled",0,RCOL_CANCELLED,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"CancelledByExchange",0,RCOL_CANCELLED_BY_EXCHANGE,0,0},        
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"CancelRequested",0,RCOL_CANCEL_REQUESTED,0,0},            
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"DiscretionaryAmount",0,RCOL_DISCRETIONARYAMOUNT,0,0},        
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Domain",0,RCOL_DOMAIN,0,0},                     
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"DoNotClear",0,RCOL_DONOTCLEAR,0,0},                 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ECNId",0,RCOL_ECNID,0,0},                      
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"FIXLineID",0,RCOL_FIXLINEID,0,0},                      				
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,80,"ECNOrderNo",0,RCOL_ECNORDERNO,0,0},                 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"EditedByBo",0,RCOL_EDITEDBYBO,0,0},                 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ExSymbol.CurrencyCode",0,RCOL_EXSYMBOL_CURRENCYCODE,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ExSymbol.Exchange",0,RCOL_EXSYMBOL_EXCHANGE,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ExSymbol.Symbol",0,RCOL_EXSYMBOL_SYMBOL,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ExSymbol.Region",0,RCOL_EXSYMBOL_REGION,0,0},    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Ext",0,RCOL_EXT,0,0},                        
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"FileDate",0,RCOL_FILEDATE,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"FOK",0,RCOL_FOK,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"IOC_ArcaNow",0,RCOL_ARCANOW,0,0},                
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"LastDate",0,RCOL_LASTDATE,0,0},                   
				//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_IMAGE|LVCF_FMT,LVCFMT_IMAGE,60,"LastStatus",0,RCOL_LASTSTATUS,3,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"LastStatus",0,RCOL_LASTSTATUS,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"LastTime",0,RCOL_LASTTIME,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"LastPx",0,RCOL_LASTPX,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"LastShares",0,RCOL_LASTSHARES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ExeShares",0,RCOL_EXESHARES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OpenShares",0,RCOL_OPENSHARES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PrevOpenShares",0,RCOL_PREVOPENSHARES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ClosedShares",0,RCOL_CLOSEDSHARES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"MasterSlave",0,RCOL_MASTERSLAVE,0,0},                
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"MMId",0,RCOL_MMID,0,0},                       
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OnClose",0,RCOL_ONCLOSE,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OnOpen",0,RCOL_ONOPEN,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,120,"OrderNo",0,RCOL_ORDERNO,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,120,"CancelMemo",0,RCOL_CANCELMEMO,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OrderWasCancelReplaced",0,RCOL_ORDERWASCANCELREPLACED,0,0},     
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PegBest",0,RCOL_PEGBEST,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PegLast",0,RCOL_PEGLAST,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PegMid",0,RCOL_PEGMMID,0,0},                     
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PegMkt",0,RCOL_PEGMKT,0,0},                     
				//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PegOrder",0,RCOL_PEGORDER,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PegPrime",0,RCOL_PEGPRIME,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Pending",0,RCOL_PENDING,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PrefECN",0,RCOL_PREFECN,0,0},                    
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PrefMMID",0,RCOL_PREFMMID,0,0},                   
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"PrevOrderNumber",0,RCOL_PREVORDERNUMBER,0,0},            
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Price",0,RCOL_PRICE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ReplaceRequest",0,RCOL_REPLACEREQUEST,0,0},             
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ReservedShares",0,RCOL_RESERVEDSHARES,0,0},  
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Shares",0,RCOL_SHARES,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"StopAmount",0,RCOL_STOPAMOUNT,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"StopType",0,RCOL_STOPTYPE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"TradeDetRef",0,RCOL_TRADEDETREF,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Type",0,RCOL_TYPE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"User",0,RCOL_USER,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"UserType",0,RCOL_USERTYPE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"WorkOrderRefNo",0,RCOL_WORKORDERREFNO,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,80,"ECNParserOrderNo",0,RCOL_ECNPARSERORDERNO,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"LinkOrderNo",0,RCOL_LINKORDERNO,0,0}, 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Memo",0,RCOL_MEMO,0,0}, 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OATSConnectionID",0,RCOL_OATSCONNECTIONID,0,0}, 
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OatsManualTicket",0,RCOL_OATSMANUALTICKET,0,0},           
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OatsNonDirectedOrder",0,RCOL_OATSNONDIRECTEDORDER,0,0},       
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OrderType",0,RCOL_ORDERTYPE,0,0},                  
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ShortExemptCode",0,RCOL_SHORTEXEMPTCODE,0,0},            
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ComplexOrder",0,RCOL_COMPLEXORDER,0,0},               
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ComplexOrderParent",0,RCOL_COMPLEXORDERPARENT,0,0},         
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OrderStagingOrder",0,RCOL_ORDERSTAGINGORDER,0,0},               
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OrderStagingParent",0,RCOL_ORDERSTAGINGPARENT,0,0},         
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"NotHeldOrder",0,RCOL_NOTHELDORDER,0,0},               
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"NotOATSReportable",0,RCOL_NOTOATSREPORTABLE,0,0},          
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OVDSpclHndling",0,RCOL_OVDSPCLHNDLING,0,0},             
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"TraderGiveUp",0,RCOL_TRADERGIVEUP,0,0},               
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"WRKSpclHndling",0,RCOL_WRKSPCLHNDLING,0,0},             
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"StrategyName",0,RCOL_STRATEGYNAME,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"OverrideComms",0,RCOL_OVERRIDECOMMS,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"CommissionOverrideType",0,RCOL_COMMISSIONOVERRIDETYPE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"CommissionOverride",0,RCOL_COMMISSIONOVERRIDE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"TranTypeCode",0,RCOL_TRANTYPECODE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"RediDestination",0,RCOL_REDIDESTINATION,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Currency",0,RCOL_CURRENCY,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"SCurrency",0,RCOL_SETTLEMENT_CURRENCY,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ProductType",0,RCOL_PRODUCT_TYPE,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Region",0,RCOL_REGION,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"ClosePosition",0,RCOL_CLOSEPOSITION,0,0},
				{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"Pos",0,RCOL_POSITION,0,0},
				{0,0,0,0,0},
			};
			for(int i=0;rcolsBtr[i].mask;i++)
				clc.InsertColumn(i,&rcolsBtr[i]);

			//LVCOLUMN rcolsAcct[]={
			//	{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,40,"Hist",0,ACOL_HIST,0,0},
			//	{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Len",0,ACOL_LEN,0,0},
			//	{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Account",0,ACOL_ACCOUNT,0,0},
			//	{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Domain",0,ACOL_DOMAIN,0,0},
			//	{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"Pos",0,ACOL_POSITION,0,0},
			//	{0,0,0,0,0},
			//};

			//for(int i=0;rcolsAcct[i].mask;i++)
			//	m_results.InsertColumn(i,&rcolsAcct[i]);
		}
		else
		{
			curq=newq;
			CreateFixColumns();
		}

		// Issue query for the new view
		curq=newq;
		CString title;
		title.Format("Details %s",curq.where);
		GetParent()->SetWindowText(title);	// ChildFrm2 doesn't have FWS_ADDTOTITLE
	#ifdef SPLIT_ORD_DET
		detDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
			RUNTIME_CLASS(Cvsctest3Doc),
			RUNTIME_CLASS(CChildFrame2),
			RUNTIME_CLASS(Cvsctest3View));
	#else
		vDocTemplate = new CMultiDocTemplate(IDR_vsctest3TYPE,
			RUNTIME_CLASS(Cvsctest3Doc),
			RUNTIME_CLASS(CChildFrame2),
			RUNTIME_CLASS(Cvsctest3View));
	#endif

		theApp.AddNotify(this);
		connNotify=&theApp;
	#ifdef SPLIT_ORD_DET
		if(newq.from=="DETAILS/ORDERS")
		{
			ordDetView=newOrdDetView; newOrdDetView->ordDetView=this;
			curq.select=curq.from=curq.where="";
			return;
		}
	#endif
		curq.wresults=500; curq.nresults=0;
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
			connNotify->VSCNotifyDatRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		else
		{
			if((theApp.allfixtags)||(eventlog))
				curq.select="*";
			else if(curq.asys=="RTOATS")
				curq.select="*";
			else
				curq.select=theApp.fixfilter.c_str();
			connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		}
	}
}
int Cvsctest3View::CreateFixColumns()
{
	CListCtrl& clc=GetListCtrl();
	CHeaderCtrl *chc=clc.GetHeaderCtrl();
	clc.DeleteAllItems();
	for(int i=chc->GetItemCount() -1;i>=0;i--)
		clc.DeleteColumn(i);

	if(curq.asys=="RTOATS")
	{
		LVCOLUMN fcol={LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"Pos",0,FCOL_POSITION,0,0};
		clc.InsertColumn(0,&fcol);
		for(int i=1;i<=70;i++)
		{
			CString hstr;
			hstr.Format("%d",i);
			LVCOLUMN ocol={LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,(LPSTR)(const char *)hstr,0,i,0,0};
			switch(i)
			{
			case 4:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 24:
				ocol.cx=120;
				break;
			};
			clc.InsertColumn(i,&ocol);
		}
	}
	else if(theApp.allfixtags)
	{
		LVCOLUMN fcols[]={
			//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,20,"Hist",0,FCOL_HIST,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"Pos",0,FCOL_POSITION,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,"Len",0,FCOL_FIXLEN,0,0},
			//{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_IMAGE|LVCF_FMT,LVCFMT_IMAGE,1000,"FIX",0,FCOL_FIXMSG,11,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,1000,"FIX",0,FCOL_FIXMSG,0,0},
			{0,0,0,0,0},
		};
		for(int i=0;fcols[i].mask;i++)
			clc.InsertColumn(i,&fcols[i]);
	}
	else
	{
		LVCOLUMN pcol={LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT,LVCFMT_RIGHT,60,"Pos",0,FCOL_POSITION,0,0};
		clc.InsertColumn(0,&pcol);
		int cno=1;
		for(const char *fptr=theApp.fixfilter.c_str();fptr;fptr=strchr(fptr,','))
		{
			if(*fptr==',') fptr++;
			int tno=atoi(fptr);
			if(tno>0)
			{
				char cname[16]={0};
				sprintf(cname,"%d",tno);
				LVCOLUMN fcol={LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,60,cname,0,cno,0,0};
				switch(tno)
				{
				case 35:
				case 54: 
				case 150: fcol.cx=30; break;
				case 11:
				case 41: 
				case 17:
				case 37: fcol.cx=150; break;
				case 52:
				case 60: fcol.cx=130; break;
				case 58: fcol.cx=200; break;
				};
				clc.InsertColumn(cno,&fcol);
				cno++;
			}
		}
	}
	return 0;
}
void Cvsctest3View::PostNcDestroy()
{
	Cvsctest3Doc *pdoc=GetDocument();
	pdoc->RemoveView(this);
	theApp.RemNotify(this);
	if(vDocTemplate)
	{
		delete vDocTemplate; vDocTemplate=0;
	}
#ifdef SPLIT_ORD_DET
	if(detDocTemplate)
	{
		delete detDocTemplate; detDocTemplate=0;
	}
	for(list<char*>::iterator bit=curq.bresults.begin();bit!=curq.bresults.end();bit++)
		delete *bit;
	curq.bresults.clear();
#endif
}

void Cvsctest3View::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void Cvsctest3View::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	//theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
	CListCtrl& clc=GetListCtrl();
	int sidx=-1;
	POSITION pos=clc.GetFirstSelectedItemPosition();
	if(pos)
		sidx=clc.GetNextSelectedItem(pos);
	HMENU hmenu = CreatePopupMenu();
	UINT mflags;
	if(curq.from=="INDICES")
	{
		if(strstr(curq.where,"TransactTime"))
		{
			mflags=MF_STRING;
			if(clc.GetItemCount()<=0)
				mflags|=MF_GRAYED;
			AppendMenu(hmenu,mflags,IDC_12AM,"12 am");
			AppendMenu(hmenu,mflags,IDC_1AM,"1 am");
			AppendMenu(hmenu,mflags,IDC_2AM,"2 am");
			AppendMenu(hmenu,mflags,IDC_3AM,"3 am");
			AppendMenu(hmenu,mflags,IDC_4AM,"4 am");
			AppendMenu(hmenu,mflags,IDC_5AM,"5 am");
			AppendMenu(hmenu,mflags,IDC_6AM,"6 am");
			AppendMenu(hmenu,mflags,IDC_7AM,"7 am");
			AppendMenu(hmenu,mflags,IDC_8AM,"8 am");
			AppendMenu(hmenu, MF_SEPARATOR,0,0);
			AppendMenu(hmenu,mflags,IDC_9AM,"9 am");
			AppendMenu(hmenu,mflags,IDC_10AM,"10 am");
			AppendMenu(hmenu,mflags,IDC_11AM,"11 am");
			AppendMenu(hmenu,mflags,IDC_12PM,"12 pm");
			AppendMenu(hmenu,mflags,IDC_1PM,"1 pm");
			AppendMenu(hmenu,mflags,IDC_2PM,"2 pm");
			AppendMenu(hmenu,mflags,IDC_3PM,"3 pm");
			AppendMenu(hmenu,mflags,IDC_4PM,"4 pm");
			AppendMenu(hmenu, MF_SEPARATOR,0,0);
			AppendMenu(hmenu,mflags,IDC_5PM,"5 pm");
			AppendMenu(hmenu,mflags,IDC_6PM,"6 pm");
			AppendMenu(hmenu,mflags,IDC_7PM,"7 pm");
			AppendMenu(hmenu,mflags,IDC_8PM,"8 pm");
			AppendMenu(hmenu,mflags,IDC_9PM,"9 pm");
			AppendMenu(hmenu,mflags,IDC_10PM,"10 pm");
			AppendMenu(hmenu,mflags,IDC_11PM,"11 pm");
		}
		else
		{
			AppendMenu(hmenu, MF_STRING, IDC_SAVEALL,"Save &all");
			AppendMenu(hmenu, MF_STRING, IDC_SAVE,"Save &selected");
			AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_NOTEPAD,"&Notepad query.txt");
			AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_TEXTPAD,"&Textpad query.txt");
			AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_EXCEL,"&Excel query.txt");
			AppendMenu(hmenu, MF_STRING |(moreToGet)?0:MF_GRAYED, IDC_QUERYALL,"Query all to &file");

			//AppendMenu(hmenu, MF_SEPARATOR,0,0);
			//AppendMenu(hmenu,MF_STRING,IDC_,"");
			//AppendMenu(hmenu,MF_STRING,IDC_,"");
			//AppendMenu(hmenu,MF_STRING,IDC_,"");

			AppendMenu(hmenu, MF_SEPARATOR,0,0);
			AppendMenu(hmenu,MF_STRING,IDC_TRACE_ORDER_ALL,"Trace this key all systems...");
		}
	}
	else if(curq.from=="ORDERS")
	{
		int sidx=-1;
		POSITION pos=clc.GetFirstSelectedItemPosition();
		if(pos)
			sidx=clc.GetNextSelectedItem(pos);
		AppendMenu(hmenu, MF_STRING, IDC_SAVEALL,"Save &all");
		AppendMenu(hmenu, MF_STRING, IDC_SAVE,"Save &selected");
		AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_NOTEPAD,"&Notepad query.txt");
		AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_TEXTPAD,"&Textpad query.txt");
		AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_EXCEL,"&Excel query.txt");

		if(curq.select!="SUMMARY")
		{
			AppendMenu(hmenu, MF_STRING |(moreToGet)?0:MF_GRAYED, IDC_QUERYALL,"Query all to &file");
			mflags=MF_STRING;
			if(curq.select.IsEmpty()) mflags|=MF_GRAYED;
			AppendMenu(hmenu, mflags, ID_REFRESH_QUERY,"&Refresh query");

			AppendMenu(hmenu, MF_SEPARATOR,0,0);
			AppendMenu(hmenu,MF_STRING,IDC_SUMMARIZE_ORDERS,"S&ummarize orders");
			mflags=MF_STRING;
			if((sidx<0)||(clc.GetItemText(sidx,OCOL_ROOTORDERID).IsEmpty())) mflags|=MF_GRAYED;
			else if(strstr(curq.where,"RootOrderID=="))  mflags|=MF_GRAYED;
			AppendMenu(hmenu,mflags,IDC_QUERY_ROOTORDERID,"Parent/Child family");
			mflags=MF_STRING;
			if((sidx<0)||(clc.GetItemText(sidx,OCOL_FIRSTCLORDID).IsEmpty())) mflags|=MF_GRAYED;
			else if(strstr(curq.where,"FirstClOrdID=="))  mflags|=MF_GRAYED;
			AppendMenu(hmenu,mflags,IDC_QUERY_FIRSTCLORDID,"Cancel/Replace chain");
			AppendMenu(hmenu,MF_STRING,IDC_QUERY_DETAILS,"Same query on &DETAILS");
			AppendMenu(hmenu,MF_STRING,IDC_TRACE_ORDER,"Trace these orders");
			mflags=MF_STRING;
			if(sidx<0) mflags|=MF_GRAYED;
			AppendMenu(hmenu,mflags,IDC_TRACE_ORDER_ALL,"Trace this order all systems...");
		}

		AppendMenu(hmenu, MF_SEPARATOR,0,0);
		mflags=MF_STRING;
		if(sidx<0) mflags|=MF_GRAYED;
		AppendMenu(hmenu,mflags,ID_REMOVE_ORDER,"Remove result from table");
		mflags=MF_STRING;
		if((curq.asys!="CLSERVER")&&(curq.asys!="TWIST")) mflags|=MF_GRAYED;
		AppendMenu(hmenu,mflags,ID_HIDE_POSITIONS,"Remove positions from table");
	}
	else if(curq.from=="DETAILS")
	{
		int sidx=-1;
		POSITION pos=clc.GetFirstSelectedItemPosition();
		if(pos)
			sidx=clc.GetNextSelectedItem(pos);
		AppendMenu(hmenu, MF_STRING, IDC_SAVEALL,"Save &all");
		AppendMenu(hmenu, MF_STRING, IDC_SAVE,"Save &selected");
		AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_NOTEPAD,"&Notepad query.txt");
		AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_TEXTPAD,"&Textpad query.txt");
		AppendMenu(hmenu, MF_STRING |(savedOnce)?0:MF_GRAYED, IDC_EXCEL,"&Excel query.txt");

		if(curq.select!="SUMMARY")
		{
			AppendMenu(hmenu, MF_STRING |(moreToGet)?0:MF_GRAYED, IDC_QUERYALL,"Query all to &file");
			mflags=MF_STRING;
			if(curq.select.IsEmpty()) mflags|=MF_GRAYED;
			AppendMenu(hmenu, mflags, ID_REFRESH_QUERY,"&Refresh query");

			AppendMenu(hmenu, MF_SEPARATOR,0,0);
			mflags=MF_STRING;
			if(curq.asys=="RTOATS") mflags|=MF_GRAYED;
			else if(curq.asys=="EVENTLOG") mflags|=MF_GRAYED;
			if(!theApp.allfixtags) mflags|=MF_CHECKED;
			AppendMenu(hmenu,mflags,IDC_FIXFILTER,"FIX &Filter...");
			mflags=MF_STRING;
			if(curq.asys=="RTOATS") mflags|=MF_GRAYED;
			else if(curq.asys=="EVENTLOG") mflags|=MF_GRAYED;
			AppendMenu(hmenu,mflags,IDC_SUMMARIZE_DETAILS,"Su&mmarize details");
		}

		AppendMenu(hmenu, MF_SEPARATOR,0,0);
		mflags=MF_STRING;
		if(sidx<0) mflags|=MF_GRAYED;
		AppendMenu(hmenu,mflags,ID_REMOVE_ORDER,"Remove result from table");
	}

	TrackPopupMenu(hmenu, TPM_LEFTALIGN |TPM_TOPALIGN |TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, 0);
}

int CALLBACK _SortOrders(LPARAM e1, LPARAM e2, LPARAM hint)
{
	Cvsctest3View *pview=(Cvsctest3View *)hint;
	return pview->SortOrders(e1,e2,pview->osortcol);
}
int Cvsctest3View::SortOrders(LPARAM e1, LPARAM e2, LPARAM hint)
{
	int osortcol=(int)hint;
	if(osortcol<0)
		return SortOrders(e2,e1,-osortcol);

	CListCtrl& clc=GetListCtrl();
	LVFINDINFO lvfi;
	memset(&lvfi,0,sizeof(LVFINDINFO));
	lvfi.flags=LVFI_PARAM;
	lvfi.lParam=e1;
	int sidx1=clc.FindItem(&lvfi);
	lvfi.lParam=e2;
	int sidx2=clc.FindItem(&lvfi);
	if((sidx1<0)||(sidx2<0))
		return 0;
	char t1[256]={0},t2[256]={0};
	clc.GetItemText(sidx1,osortcol -1,t1,sizeof(t1));
	clc.GetItemText(sidx2,osortcol -1,t2,sizeof(t2));
	switch(osortcol -1)
	{
	case OCOL_APPINSTID:
	case OCOL_CLORDID:
	case OCOL_ROOTORDERID:
	case OCOL_FIRSTCLORDID:
	case OCOL_SYMBOL:
	case OCOL_SIDE:
	case OCOL_ACCOUNT:
	case OCOL_ECNORDERID:
	case OCOL_CLIENTID:
	case OCOL_TERM:
	case OCOL_TRANSACTTIME:
	case OCOL_CONNECTION:
	case OCOL_CLPARENTORDERID:
	case OCOL_ORDERDATE:
	//case OCOL_ROUTEDORDERID:
	case OCOL_ROUTINGBROKER:
	case OCOL_SECURITYTYPE:
		return _stricmp(t1,t2);
	case OCOL_PRICE:
	{
		double f1=atof(t1),f2=atof(t2);
		if(f1>f2) return +1;
		else if(f1<f2) return -1;
		else return 0;
	}
	case OCOL_ORDERQTY:
	case OCOL_CUMQTY:
	case OCOL_FILLQTY:
	case OCOL_ORDERLOC:
		return (int)(_atoi64(t1) -_atoi64(t2));
	case OCOL_HIGHMSGTYPE:
		return theApp.GetMsgTypeTier(t1[0]) -theApp.GetMsgTypeTier(t2[0]);
	case OCOL_HIGHEXECTYPE:
		return theApp.GetExecTypeTier(t1[0]) -theApp.GetExecTypeTier(t2[0]);
	};
	return 0;
}
int CALLBACK _SortBTR(LPARAM e1, LPARAM e2, LPARAM hint)
{
	Cvsctest3View *pview=(Cvsctest3View *)hint;
	return pview->SortBTR(e1,e2,pview->btrsortcol);
}
int Cvsctest3View::SortBTR(LPARAM e1, LPARAM e2, LPARAM hint)
{
	int btrsortcol=(int)hint;
	if(btrsortcol<0)
		return SortBTR(e2,e1,-btrsortcol);

	CListCtrl& clc=GetListCtrl();
	LVFINDINFO lvfi;
	memset(&lvfi,0,sizeof(LVFINDINFO));
	lvfi.flags=LVFI_PARAM;
	lvfi.lParam=e1;
	int sidx1=clc.FindItem(&lvfi);
	lvfi.lParam=e2;
	int sidx2=clc.FindItem(&lvfi);
	if((sidx1<0)||(sidx2<0))
		return 0;
	char t1[256]={0},t2[256]={0};
	clc.GetItemText(sidx1,btrsortcol -1,t1,sizeof(t1));
	clc.GetItemText(sidx2,btrsortcol -1,t2,sizeof(t2));
	switch(btrsortcol -1)
	{
	case RCOL_FIXLEN:
	case RCOL_ACCOUNT:
	case RCOL_ACTION:
	case RCOL_AON:
	case RCOL_BOFAEXECUTIONSERVICESDOMAIN:
	case RCOL_BOFAEXECUTIONSERVICESORDER:
	case RCOL_CANCELLED:
	case RCOL_CANCELLED_BY_EXCHANGE:
	case RCOL_CANCEL_REQUESTED:
	case RCOL_DISCRETIONARYAMOUNT:
	case RCOL_DOMAIN:
	case RCOL_DONOTCLEAR:
	case RCOL_ECNID:
	case RCOL_FIXLINEID:
	case RCOL_ECNORDERNO:
	case RCOL_EDITEDBYBO:
	case RCOL_EXSYMBOL_CURRENCYCODE:
	case RCOL_EXSYMBOL_EXCHANGE:
	case RCOL_EXSYMBOL_SYMBOL:
	case RCOL_EXSYMBOL_REGION:
	case RCOL_EXT:
	case RCOL_FILEDATE:
	case RCOL_FOK:
	case RCOL_ARCANOW:
	case RCOL_LASTDATE:
	case RCOL_LASTSTATUS:
	case RCOL_LASTTIME:
	case RCOL_MASTERSLAVE:
	case RCOL_MMID:
	case RCOL_ONCLOSE:
	case RCOL_ONOPEN:
	case RCOL_ORDERNO:
	case RCOL_CANCELMEMO:
	case RCOL_ORDERWASCANCELREPLACED:
	case RCOL_PEGBEST:
	case RCOL_PEGLAST:
	case RCOL_PEGMMID:
	case RCOL_PEGMKT:
	//case RCOL_PEGORDER:
	case RCOL_PENDING:
	case RCOL_PREFECN:
	case RCOL_PREFMMID:
	case RCOL_PREVORDERNUMBER:
	case RCOL_REPLACEREQUEST:
	case RCOL_STOPAMOUNT:
	case RCOL_STOPTYPE:
	case RCOL_TRADEDETREF:
	case RCOL_TYPE:
	case RCOL_USER:
	case RCOL_USERTYPE:
	case RCOL_WORKORDERREFNO:
	case RCOL_ECNPARSERORDERNO:
	case RCOL_LINKORDERNO:
	case RCOL_MEMO:
	case RCOL_OATSCONNECTIONID:
	case RCOL_OATSMANUALTICKET:
	case RCOL_OATSNONDIRECTEDORDER:
	case RCOL_ORDERTYPE:
	case RCOL_SHORTEXEMPTCODE:
	case RCOL_COMPLEXORDER:
	case RCOL_COMPLEXORDERPARENT:
	case RCOL_ORDERSTAGINGORDER:
	case RCOL_ORDERSTAGINGPARENT:
	case RCOL_NOTHELDORDER:
	case RCOL_NOTOATSREPORTABLE:
	case RCOL_OVDSPCLHNDLING:
	case RCOL_TRADERGIVEUP:
	case RCOL_WRKSPCLHNDLING:
	case RCOL_STRATEGYNAME:
	case RCOL_OVERRIDECOMMS:
	case RCOL_COMMISSIONOVERRIDETYPE:
	case RCOL_REDIDESTINATION:
	case RCOL_CURRENCY:
	case RCOL_SETTLEMENT_CURRENCY:
	case RCOL_PRODUCT_TYPE:
	case RCOL_REGION:
		return _stricmp(t1,t2);
	case RCOL_AAVGPX:
	case RCOL_LASTPX:
	case RCOL_PEGPRIME:
	case RCOL_PRICE:
	case RCOL_COMMISSIONOVERRIDE:
	{
		double f1=atof(t1),f2=atof(t2);
		if(f1>f2) return +1;
		else if(f1<f2) return -1;
		else return 0;
	}
	case RCOL_LASTSHARES:
	case RCOL_EXESHARES:
	case RCOL_OPENSHARES:
	case RCOL_PREVOPENSHARES:
	case RCOL_CLOSEDSHARES:
	case RCOL_RESERVEDSHARES:
	case RCOL_SHARES:
	case RCOL_TRANTYPECODE:
	case RCOL_POSITION:
	case RCOL_CLOSEPOSITION:
		return (int)(_atoi64(t1) -_atoi64(t2));
	//case OCOL_HIGHMSGTYPE:
	//	return theApp.GetMsgTypeTier(t1[0]) -theApp.GetMsgTypeTier(t2[0]);
	//case OCOL_HIGHEXECTYPE:
	//	return theApp.GetExecTypeTier(t1[0]) -theApp.GetExecTypeTier(t2[0]);
	};
	return 0;
}
int CALLBACK _SortFIX(LPARAM e1, LPARAM e2, LPARAM hint)
{
	Cvsctest3View *pview=(Cvsctest3View *)hint;
	return pview->SortFIX(e1,e2,pview->fixsortcol);
}
int Cvsctest3View::SortFIX(LPARAM e1, LPARAM e2, LPARAM hint)
{
	int fixsortcol=(int)hint;
	if(fixsortcol<0)
		return SortFIX(e2,e1,-fixsortcol);

	CListCtrl& clc=GetListCtrl();
	LVFINDINFO lvfi;
	memset(&lvfi,0,sizeof(LVFINDINFO));
	lvfi.flags=LVFI_PARAM;
	lvfi.lParam=e1;
	int sidx1=clc.FindItem(&lvfi);
	lvfi.lParam=e2;
	int sidx2=clc.FindItem(&lvfi);
	if((sidx1<0)||(sidx2<0))
		return 0;
	CHeaderCtrl *chc=clc.GetHeaderCtrl();
	if(fixsortcol>chc->GetItemCount())
		fixsortcol=chc->GetItemCount();
	char t1[256]={0},t2[256]={0};
	clc.GetItemText(sidx1,fixsortcol -1,t1,sizeof(t1));
	clc.GetItemText(sidx2,fixsortcol -1,t2,sizeof(t2));

	HDITEM hdi;
	memset(&hdi,0,sizeof(HDITEM));
	hdi.mask=HDI_TEXT;
	char hstr[256]={0};
	hdi.pszText=hstr;
	hdi.cchTextMax=sizeof(hstr);
	int tno=0,cmp=0;
	if((fixsortcol>0)&&(fixsortcol -1<chc->GetItemCount()))
	{
		chc->GetItem(fixsortcol -1,&hdi);
		if(!strcmp(hstr,"Pos"))
			tno=-1;
		else
			tno=atoi(hstr);
	}
	switch(tno)
	{
	case 6:
	case 31:
	case 44:
	case 99:
	{
		double f1=atof(t1),f2=atof(t2);
		if(f1>f2) return +1;
		else if(f1<f2) return -1;
		break;
	}
	case -1:
	case 14:
	case 32:
	case 38:
	case 54:
	case 111:
	case 151:
		cmp=(int)(_atoi64(t1) -_atoi64(t2));
		if(cmp!=0) return cmp;
		break;
	case 35:
		cmp=theApp.GetMsgTypeTier(t1[0]) -theApp.GetMsgTypeTier(t2[0]);
		if(cmp!=0) return cmp;
		break;
	case 150:
		cmp=theApp.GetExecTypeTier(t1[0]) -theApp.GetExecTypeTier(t2[0]);
		if(cmp!=0) return cmp;
		break;
	default:
		cmp=_stricmp(t1,t2);
		if(cmp!=0) return cmp;
		break;
	};
	if(fixsortcol>1)
	{
		// When the two items have same value, 2nd tier sort by detail position
		clc.GetItemText(sidx1,0,t1,sizeof(t1));
		clc.GetItemText(sidx2,0,t2,sizeof(t2));
		return atoi(t1) -atoi(t2);
	}
	return 0;
}
void Cvsctest3View::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: Add your control notification handler code here
	CListCtrl& clc=GetListCtrl();
	if(curq.from=="ORDERS")
	{
		if(osortcol==phdr->iItem +1)
			osortcol=-osortcol;
		else
			osortcol=phdr->iItem +1;
		// Remove ... before sorting
		if(moreToGet)
		{
			int lidx=clc.GetItemCount() -1;
			if(lidx>=0)
				clc.DeleteItem(lidx);
		}

		clc.SortItems(_SortOrders,(DWORD_PTR)this);

		if(moreToGet)
		{
			LV_ITEM lvi;
			memset(&lvi,0,sizeof(LV_ITEM));
			lvi.mask=LVIF_IMAGE;
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();
			int nidx=clc.InsertItem(&lvi);
			if(nidx>=0)
				clc.SetItemText(nidx,OCOL_CLORDID,"...");
		}
	}
	else if(curq.from=="DETAILS")
	{
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
		{
			if(btrsortcol==phdr->iItem +1)
				btrsortcol=-btrsortcol;
			else
				btrsortcol=phdr->iItem +1;
			// Remove ... before sorting
			if(moreToGet)
			{
				int lidx=clc.GetItemCount() -1;
				if(lidx>=0)
					clc.DeleteItem(lidx);
			}

			clc.SortItems(_SortBTR,(DWORD_PTR)this);

			if(moreToGet)
			{
				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.mask=LVIF_IMAGE;
				lvi.iImage=-1;
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
					clc.SetItemText(nidx,RCOL_ACCOUNT,"...");
			}
		}
		else if((!theApp.allfixtags)&&(!eventlog))
		{
			if(fixsortcol==phdr->iItem +1)
				fixsortcol=-fixsortcol;
			else
				fixsortcol=phdr->iItem +1;
			// Remove ... before sorting
			if(moreToGet)
			{
				int lidx=clc.GetItemCount() -1;
				if(lidx>=0)
					clc.DeleteItem(lidx);
			}

			clc.SortItems(_SortFIX,(DWORD_PTR)this);

			if(moreToGet)
			{
				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.mask=LVIF_IMAGE;
				lvi.iImage=-1;
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
					clc.SetItemText(nidx,0,"...");
			}
		}
	}

	*pResult = 0;
}

// Cvsctest3View diagnostics

#ifdef _DEBUG
void Cvsctest3View::AssertValid() const
{
	CListView::AssertValid();
}

void Cvsctest3View::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

Cvsctest3Doc* Cvsctest3View::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Cvsctest3Doc)));
	return (Cvsctest3Doc*)m_pDocument;
}
#endif //_DEBUG


// Cvsctest3View message handlers
void Cvsctest3View::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window	
	CListView::OnStyleChanged(nStyleType,lpStyleStruct);	
}

// VSCNotify
void Cvsctest3View::VSCNotifyError(int rc, const char *emsg)
{
}
void Cvsctest3View::VSCNotifyEvent(int rc, const char *emsg)
{
}
// For EncodeLoginRequest
void Cvsctest3View::VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
}
void Cvsctest3View::VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
{
}
// For EncodeSqlRequest
void Cvsctest3View::VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	// Orders or details request
	if((psaveAll)&&(psaveAll->query.select==select)&&(psaveAll->query.from==from)&&(psaveAll->query.where==where))
		psaveAll->query.rid=rid;
	else if((curq.select==select)&&(curq.from==from)&&(curq.where==where)&&(!curq.nresults))
	{
		curq.rid=rid;
		moreToGet=false;
	#ifdef SPLIT_ORD_DET
		if(curq.from=="DETAILS")
		{
			CListCtrl& clc=GetListCtrl();
			clc.DeleteAllItems();
		}
		for(list<char*>::iterator bit=curq.bresults.begin();bit!=curq.bresults.end();bit++)
			delete *bit;
		curq.bresults.clear();
	#endif
	}
}
void Cvsctest3View::VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
{
	// Orders query results
	if((psaveAll)&&(rid==psaveAll->query.rid))
	{
		// FIX format
		for(int i=0;i<nfix;i++)
		{
			DWORD wbytes=0;
			WriteFile(psaveAll->fhnd,pfix[i].fbuf,pfix[i].llen,&wbytes,0);
			WriteFile(psaveAll->fhnd,"\n",1,&wbytes,0);
			//psaveAll->cnt++;
			psaveAll->query.nresults++;
		}
		if((hist)&&(!more))
		{
			psaveAll->query.iter=iter;

			//rtmap.erase(rit);
			if(iter>0)
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,psaveAll->query.select,psaveAll->query.from,psaveAll->query.where,psaveAll->query.maxorders,psaveAll->query.hist,psaveAll->query.live,psaveAll->query.iter);
			else
			{
				CString Tmp;
				Tmp.Format("Total %d items\r\n",/*psaveAll->cnt*/psaveAll->query.nresults);
				DWORD wbytes=0;
				WriteFile(psaveAll->fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
				CloseHandle(psaveAll->fhnd); psaveAll->fhnd=INVALID_HANDLE_VALUE;
				LaunchFixViewer(psaveAll->fpath.c_str());
				delete psaveAll; psaveAll=0;
				//VIEW_SIZE=500;
			}
		}
		return;
	}

	if(rid!=curq.rid)
		return;
	CListCtrl& clc=GetListCtrl();

	if((proto==/*PROTO_CLS_EVENTLOG*/0x1A)||
	   (proto==/*PROTO_FIXS_EVELOG*/0x19)||
	   (proto==/*PROTO_TRADER_EVELOG*/0x1E))
	{
		//theApp.allfixtags=true;
		eventlog=true;
		if(curq.nresults<1)
		{
			CreateFixColumns();
			curq.asys="EVENTLOG";
		}
	}

	// CSV format
	if(curq.asys=="RTOATS")
	{
		char vstr[2048]={0};
		for(int i=0;i<nfix;i++)
		{
			LV_ITEM lvi;
			memset(&lvi,0,sizeof(LV_ITEM));
			lvi.mask=LVIF_IMAGE;
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();
			int nidx=clc.InsertItem(&lvi);
			if(nidx>=0)
			{
				sprintf(vstr,"%d",/*endcnt -nfix +i +1*/nidx+1);
				clc.SetItemText(nidx,FCOL_POSITION,vstr);
				int flen=pfix[i].llen;
				if(flen>2044) flen=2044;
				memcpy(vstr,pfix[i].fbuf,flen); vstr[flen]=0;
				const char *tok=strtoke(vstr,",");
				for(int c=1;(tok)&&(c<=70);c++,tok=strtoke(0,","))
				{
					if(*tok)
					{
						clc.SetItemText(nidx,c,tok);
					}
				}
				clc.SetItemData(nidx,(DWORD_PTR)nidx +1);
				curq.nresults++;
				if(curq.wresults>0) curq.wresults--; 
			}
		}
	}
	// Show entire FIX message
	else if((theApp.allfixtags)||(eventlog))
	{
		char vstr[2048]={0};
		for(int i=0;i<nfix;i++)
		{
			LV_ITEM lvi;
			memset(&lvi,0,sizeof(LV_ITEM));
			lvi.mask=LVIF_IMAGE;
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();
			int nidx=clc.InsertItem(&lvi);
			if(nidx>=0)
			{
				//sprintf(tmp,"%s (%d)",hist?"H":"L",rid);
				//clc.SetItemText(nidx,FCOL_HIST,tmp);
				sprintf(vstr,"%d",/*endcnt -nfix +i +1*/nidx+1);
				clc.SetItemText(nidx,FCOL_POSITION,vstr);
				sprintf(vstr,"%d",pfix[i].llen);
				clc.SetItemText(nidx,FCOL_FIXLEN,vstr);
				int flen=pfix[i].llen;
				if(flen>2044) flen=2044;
				memcpy(vstr,pfix[i].fbuf,flen); vstr[flen]=0;
				strcpy(vstr+2044,"...");
				for(char *dptr=vstr;*dptr;dptr++)
				{
					if(*dptr==0x01) *dptr='|';
				}
				clc.SetItemText(nidx,FCOL_FIXMSG,vstr);
				clc.SetItemData(nidx,(DWORD_PTR)nidx +1);
				char highMsgType=pfix[i].TagChar(35);
				char highExecType=pfix[i].TagChar(150);
				if(!highExecType) highExecType=pfix[i].TagChar(39);
				int img=theApp.OrderImage(highMsgType,highExecType,0,0);
				clc.SetItem(nidx,FCOL_FIXMSG,LVIF_IMAGE,0,img,0,0,0,0);
				//clc.SetItem(nidx,0,LVIF_IMAGE,0,-1,0,0,0,0); // Blank out the image in the first column
				// Aggregates
				if(curq.asys=="EVENTLOG")
				{
					// Clserver log
					if(strstr(vstr,"Filled]"))
					{
						const char *sptr=strstr(vstr,"Shares[");
						if(sptr)
						{
							const char *pptr=strstr(sptr,"Price[");
							if(pptr)
							{
								int fqty=atoi(sptr +7);
								double fpx=atof(pptr +6);
								if(fqty>0)
								{
									curq.fillQty+=fqty;
									curq.fillSum+=(fpx *fqty);
								}
							}
						}
					}
				}
				// FIX string
				else
				{
					int fqty=pfix[i].TagInt(32);
					if(fqty>0)
					{
						double fpx=pfix[i].TagFloat(31);
						curq.fillQty+=fqty;
						curq.fillSum+=(fpx *fqty);
					}
				}
				curq.nresults++;
				if(curq.wresults>0) curq.wresults--; 
			}
		}
	}
	// Filtered FIX tags only
	else
	{
		for(int i=0;i<nfix;i++)
		{
			LV_ITEM lvi;
			memset(&lvi,0,sizeof(LV_ITEM));
			lvi.mask=LVIF_IMAGE;
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();
			int nidx=clc.InsertItem(&lvi);
			if(nidx>=0)
			{
				char vstr[32]={0};
				sprintf(vstr,"%d",nidx +1);
				clc.SetItemText(nidx,0,vstr);
				int cno=1;
				for(const char *fptr=theApp.fixfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno>0)
					{
						const char *tstr=pfix[i].TagStr(tno);
						switch(tno)
						{
						case 54:
							if(!strcmp(tstr,"1")) tstr="B";
							else if(!strcmp(tstr,"2")) tstr="SL";
							else if(!strcmp(tstr,"5")) tstr="SS";
							else if(!strcmp(tstr,"6")) tstr="SX";
							break;
						case 150:
						case 39:
						{
							char highMsgType=pfix[i].TagChar(35);
							char highExecType=pfix[i].TagChar(150);
							if(!highExecType) highExecType=pfix[i].TagChar(39);
							int img=theApp.OrderImage(highMsgType,highExecType,0,0);
							clc.SetItem(nidx,FCOL_FIXMSG,LVIF_IMAGE,0,img,0,0,0,0);
							//clc.SetItem(nidx,0,LVIF_IMAGE,0,-1,0,0,0,0); // Blank out the image in the first column
							break;
						}
						};
						clc.SetItemText(nidx,cno,tstr);
						cno++;
					}
				}
				clc.SetItemData(nidx,(DWORD_PTR)nidx +1);
				// Aggregates
				int fqty=pfix[i].TagInt(32);
				if(fqty>0)
				{
					double fpx=pfix[i].TagFloat(31);
					curq.fillQty+=fqty;
					curq.fillSum+=(fpx *fqty);
				}
				curq.nresults++;
				if(curq.wresults>0) curq.wresults--; 
			}
		}
	}

	if((!more)&&(hist))
	{
		// Auto-size to contents
		CHeaderCtrl *chc=clc.GetHeaderCtrl();
		for(int i=0;i<chc->GetItemCount();i++)
			clc.SetColumnWidth(i,LVSCW_AUTOSIZE);
		// Add extra for empty bitmap in first column
		int width0=clc.GetColumnWidth(0);
		clc.SetColumnWidth(0,width0 +16);

		if((!theApp.allfixtags)&&(!eventlog))
			clc.SortItems(_SortFIX,(DWORD_PTR)this);
		if(iter)
		{
			curq.iter=iter;
			moreToGet=true;

			LV_ITEM lvi;
			memset(&lvi,0,sizeof(LV_ITEM));
			lvi.mask=LVIF_IMAGE;
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();
			int nidx=clc.InsertItem(&lvi);
			if(nidx>=0)
				clc.SetItemText(nidx,FCOL_POSITION,"...");

			int eperc=endcnt*100/(totcnt?totcnt:1);
			smsg.Format("Browsed %u details, searched %d%% (%u/%u), filled %d, avgpx %.4f, more",
				clc.GetItemCount() -1,eperc,endcnt,totcnt,curq.fillQty,curq.fillSum/(curq.fillQty?curq.fillQty:1));
			theApp.pMainFrame->SetStatus(1,smsg);
			curq.wresults=500;
		}
		else
		{
			moreToGet=false;
			smsg.Format("Browsed all %u details of %u, filled %d, avgpx %.4f",
				clc.GetItemCount(),totcnt,curq.fillQty,curq.fillSum/(curq.fillQty?curq.fillQty:1));
			theApp.pMainFrame->SetStatus(1,smsg);

			// Now get the binary records
			if((!datRequested)&&((curq.asys=="FIXSERVER")||(curq.asys=="TRADER")))
			{
				datRequested=true;
				connNotify->VSCNotifyDatRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
			}
		}
	}
}
LONGLONG Cvsctest3View::GMTToEST(LONGLONG gts, SYSTEMTIME& lst, CString& stz)
{
	memset(&lst,0,sizeof(SYSTEMTIME));
	stz="";
	// This only works when client machine is on EST
	//SYSTEMTIME gst;
	//gst.wMilliseconds=gts%1000; gts/=1000;
	//gst.wSecond=gts%100; gts/=100;
	//gst.wMinute=gts%100; gts/=100;
	//gst.wHour=gts%100; gts/=100;
	//gst.wDay=gts%100; gts/=100;
	//gst.wMonth=gts%100; gts/=100;
	//gst.wYear=gts%10000;
	//gst.wDayOfWeek=0;
	//FILETIME gft,lft;
	//SystemTimeToFileTime(&gst,&gft);
	//FileTimeToLocalFileTime(&gft,&lft);
	//FileTimeToSystemTime(&lft,&lst);

	struct tm gst;
	int wMilliseconds=0;
	if(gts>=(LONGLONG)99991231000000)
	{
		wMilliseconds=gts%1000; gts/=1000;
	}
	gst.tm_sec=gts%100; gts/=100;
	gst.tm_min=gts%100; gts/=100;
	gst.tm_hour=gts%100; gts/=100;
	gst.tm_mday=gts%100; gts/=100;
	gst.tm_mon=gts%100 -1; gts/=100;
	gst.tm_year=gts%10000 -1900;
	gst.tm_isdst=-1;
	time_t tt=mktime(&gst);
	if(tt<=0)
		return 0;
	tt-=4*3600;
	if(!gst.tm_isdst)
		tt-=3600;
	gst=*localtime(&tt);
	#ifdef TIME_CONVERT
	stz=(gst.tm_isdst)?"EDT":"EST";
	#endif

	lst.wYear=gst.tm_year +1900;
	lst.wMonth=gst.tm_mon +1;
	lst.wDay=gst.tm_mday;
	lst.wHour=gst.tm_hour;
	lst.wMinute=gst.tm_min;
	lst.wSecond=gst.tm_sec;
	lst.wMilliseconds=wMilliseconds;

	LONGLONG lts=(lst.wYear*10000) +(lst.wMonth*100) +(lst.wDay); lts*=1000000;
	lts+=(lst.wHour*10000) +(lst.wMinute*100) +(lst.wSecond); lts*=1000;
	lts+=lst.wMilliseconds;
	return lts;
}
LONGLONG Cvsctest3View::ESTToGMT(LONGLONG gts, SYSTEMTIME& lst, CString& stz)
{
	memset(&lst,0,sizeof(SYSTEMTIME));
	stz="";

	struct tm gst;
	int wMilliseconds=0;
	if(gts>=(LONGLONG)99991231000000)
	{
		wMilliseconds=gts%1000; gts/=1000;
	}
	gst.tm_sec=gts%100; gts/=100;
	gst.tm_min=gts%100; gts/=100;
	gst.tm_hour=gts%100; gts/=100;
	gst.tm_mday=gts%100; gts/=100;
	gst.tm_mon=gts%100 -1; gts/=100;
	gst.tm_year=gts%10000 -1900;
	gst.tm_isdst=-1;
	time_t tt=mktime(&gst);
	if(tt<=0)
		return 0;
	tt+=4*3600;
	if(!gst.tm_isdst)
		tt+=3600;

	gst=*localtime(&tt);
	stz="GMT";

	lst.wYear=gst.tm_year +1900;
	lst.wMonth=gst.tm_mon +1;
	lst.wDay=gst.tm_mday;
	lst.wHour=gst.tm_hour;
	lst.wMinute=gst.tm_min;
	lst.wSecond=gst.tm_sec;
	lst.wMilliseconds=wMilliseconds;

	LONGLONG lts=(lst.wYear*10000) +(lst.wMonth*100) +(lst.wDay); lts*=1000000;
	lts+=(lst.wHour*10000) +(lst.wMinute*100) +(lst.wSecond); lts*=1000;
	lts+=lst.wMilliseconds;
	return lts;
}
void Cvsctest3View::VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// Orders query results
	if((psaveAll)&&(rid==psaveAll->query.rid))
	{
		// DSV format
		for(int i=0;i<nstr;i++)
		{
			CString Tmp="";
			const char *tok=strtoke((char*)pstr[i],"\1");
			for(int c=0;(tok)/*&&(c<psaveAll->ncols)*/;c++)
			{
				// Quote any value that has a comma in it
				if(strchr(tok,','))
					Tmp+=(CString)"\""+tok+(CString)"\",";
				else
					Tmp+=tok+(CString)",";
				if(tok)
					tok=strtoke(0,"\1");
				//vpages[curpage].endidx++;
			}
			Tmp+="\r\n";
			DWORD wbytes=0;
			WriteFile(psaveAll->fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
			//psaveAll->cnt++;
			psaveAll->query.nresults++;
		}
		if((hist)&&(!more))
		{
			psaveAll->query.iter=iter;

			//rtmap.erase(rit);
			if(iter>0)
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,psaveAll->query.rid,psaveAll->query.select,psaveAll->query.from,psaveAll->query.where,psaveAll->query.maxorders,psaveAll->query.hist,psaveAll->query.live,psaveAll->query.iter);
			else
			{
				CString Tmp;
				Tmp.Format("Total %d items\r\n",/*psaveAll->cnt*/psaveAll->query.nresults);
				DWORD wbytes=0;
				WriteFile(psaveAll->fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
				CloseHandle(psaveAll->fhnd); psaveAll->fhnd=INVALID_HANDLE_VALUE;
				LaunchTextViewer(psaveAll->fpath.c_str());
				delete psaveAll; psaveAll=0;
				//VIEW_SIZE=50;
			}
		}
		return;
	}

	if(rid!=curq.rid)
		return;
	CListCtrl& clc=GetListCtrl();
	for(int i=0;i<nstr;i++)
	{
		LV_ITEM lvi;
		memset(&lvi,0,sizeof(LV_ITEM));
		lvi.mask=LVIF_IMAGE;
		lvi.iImage=-1;
		lvi.iItem=clc.GetItemCount();	
		int nidx=clc.InsertItem(&lvi);
		if(nidx>=0)
		{
			int col=0,ordQty=0,fillQty=0;
			char highMsgType=0,highExecType=0;
			for(const char *tok=strtoke((char*)pstr[i],"\1");tok;tok=strtoke(0,"\1"))
			{
				if(col>=OCOL_COUNT)
					break;
				else if(col==OCOL_TRANSACTTIME)
				{
					char kstr[32]={0};
					memset(kstr,0,sizeof(kstr));
					__int64 kval=_atoi64(tok);
					if(kval>0)
					{
						#ifdef TIME_CONVERT
						if(curq.isgmt)
						{
							SYSTEMTIME lst;
							CString stz;
							GMTToEST(kval,lst,stz);
							sprintf(kstr,"%04d%02d%02d-%02d%02d%02d.%03d %s",
								lst.wYear,lst.wMonth,lst.wDay,
								lst.wHour,lst.wMinute,lst.wSecond,
								lst.wMilliseconds,stz);
						}
						else
						#endif
						{
							memcpy(kstr,tok,8); kstr[8]='-';
							memcpy(kstr +9,tok +8,6); 
							if(tok[14])
							{
								kstr[15]='.'; memcpy(kstr +16,tok +14,3); 
							}
							#ifdef TIME_CONVERT
							strcpy(kstr +19," EDT");
							#endif
						}
					}
					clc.SetItemText(nidx,col,kstr);
				}
				// Remove trailing zeros after decimal down to two
				else if(col==OCOL_PRICE)
				{
					char kstr[32]={0};
					strcpy(kstr,tok);
					char *dptr=strchr(kstr,'.');
					if(dptr)
					{
						for(char *kptr=kstr +strlen(kstr) -1;(kptr>dptr +2)&&(*kptr=='0');kptr--)
							*kptr=0;
					}
					clc.SetItemText(nidx,col,kstr);
				}
				else if(col==OCOL_SIDE)
				{
					if(!strcmp(tok,"1")) tok="B";
					else if(!strcmp(tok,"2")) tok="SL";
					else if(!strcmp(tok,"5")) tok="SS";
					else if(!strcmp(tok,"6")) tok="SX";
					clc.SetItemText(nidx,col,tok);
				}
				else if(col==OCOL_HIGHMSGTYPE)
				{
					highMsgType=*tok;
					clc.SetItemText(nidx,col,tok);
				}
				else if(col==OCOL_HIGHEXECTYPE)
				{
					highExecType=*tok;
					clc.SetItemText(nidx,col,tok);
				}
				else if(col==OCOL_ORDERQTY)
				{
					ordQty=atoi(tok);
					clc.SetItemText(nidx,col,tok);
				}
				else if(col==OCOL_FILLQTY)
				{
					fillQty=atoi(tok);
					clc.SetItemText(nidx,col,tok);
				}
				else
				{
					clc.SetItemText(nidx,col,tok);
				}
				//// Aggregates
				//if(col==OCOL_ORDERQTY)
				//{
				//	int oqty=atoi(tok);
				//	if(oqty>0)
				//		curq.ordQty+=oqty;
				//}
				//else if(col==OCOL_FILLQTY)
				//{
				//	int fqty=atoi(tok);
				//	if(fqty>0)
				//		curq.fillQty+=fqty;
				//}
				col++;
			}
			clc.SetItemData(nidx,(DWORD_PTR)nidx +1);
			int img=theApp.OrderImage(highMsgType,highExecType,ordQty,fillQty);
			clc.SetItem(nidx,OCOL_HIGHEXECTYPE,LVIF_IMAGE,0,img,0,0,0,0);
			//clc.SetItem(nidx,0,LVIF_IMAGE,0,-1,0,0,0,0); // Blank out the image in the first column
			// If this is a CLSERVER position, show the $ icon in the first column
			if((curq.asys=="CLSERVER")||(curq.asys=="TWIST"))
			{
				if((clc.GetItemText(nidx,OCOL_HIGHMSGTYPE).IsEmpty())&&
				   (clc.GetItemText(nidx,OCOL_HIGHEXECTYPE)=="B")&&
				   (clc.GetItemText(nidx,OCOL_FILLQTY)=="0"))
					clc.SetItem(nidx,0,LVIF_IMAGE,0,12,0,0,0,0);
			}
			curq.nresults++;
			if(curq.wresults>0) curq.wresults--;
		}
	}

	if((!more)&&(hist))
	{
		bool autoContinue=false;
		if(iter)
		{
			curq.iter=iter;
			moreToGet=true;

			int eperc=endcnt*100/(totcnt?totcnt:1);
			// Auto-continue minute browses and when we've received less than 500 results for this block of results
			if(((strstr(curq.where,"TransactTime>="))&&(strstr(curq.where,"TransactTime<")))||(curq.wresults>0))
			{
				smsg.Format("Browsed %u orders, searched %d%% (%u/%u)...",clc.GetItemCount(),eperc,endcnt,totcnt);
				//smsg.Format("Browsed %u orders, searched %d%% (%u/%u), %d ordered, %d filled",
				//	clc.GetItemCount() -1,eperc,endcnt,totcnt,curq.ordQty,curq.fillQty);

				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.mask=LVIF_IMAGE;
				lvi.iImage=-1;
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
					clc.SetItemText(nidx,OCOL_CLORDID,"...");

				clc.SetItemState(nidx,LVIS_SELECTED,LVIS_SELECTED);
				NMITEMACTIVATE ItemActivate;
				memset(&ItemActivate,0,sizeof(NMITEMACTIVATE));
				ItemActivate.iItem=nidx;
				LRESULT Result=0;
				#ifdef SPLIT_ORD_DET
				OnNMClick((NMHDR *)&ItemActivate,&Result);
				#else
				OnNMDblclk((NMHDR *)&ItemActivate,&Result);
				#endif
				autoContinue=true;
			}
			else
			{
				smsg.Format("Browsed %u orders, searched %d%% (%u/%u), more",clc.GetItemCount(),eperc,endcnt,totcnt);
				//if(curq.from!="TRACEORDERS")
				//	clc.SortItems(_SortOrders,(DWORD_PTR)this);

				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.mask=LVIF_IMAGE;
				lvi.iImage=-1;
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
					clc.SetItemText(nidx,OCOL_CLORDID,"...");

				curq.wresults=500;
			}
			theApp.pMainFrame->SetStatus(1,smsg);
		}
		else
		{
			moreToGet=false;
			smsg.Format("Browsed all %u orders of %u",clc.GetItemCount(),totcnt);
			//smsg.Format("Browsed all %u orders of %u, %d ordered, %d filled",
			//	clc.GetItemCount(),totcnt,curq.ordQty,curq.fillQty);
			theApp.pMainFrame->SetStatus(1,smsg);

			#ifdef SPLIT_ORD_DET
			// Select single orders
			if(clc.GetItemCount()==1)
			{
				clc.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);
				//clc.PostMessage(WM_USER +4,0,0);
				NMITEMACTIVATE ItemActivate;
				memset(&ItemActivate,0,sizeof(NMITEMACTIVATE));
				ItemActivate.iItem=0;
				LRESULT Result=0;
				OnNMClick((NMHDR *)&ItemActivate,&Result);
			}
			#endif
		}

		if(!autoContinue)
		{
			// Auto-size to contents
			CHeaderCtrl *chc=clc.GetHeaderCtrl();
			for(int i=0;i<chc->GetItemCount();i++)
				clc.SetColumnWidth(i,LVSCW_AUTOSIZE);
			// Add extra for empty bitmap in first column
			int width0=clc.GetColumnWidth(0);
			clc.SetColumnWidth(0,width0 +16);
		}
	}
}
void Cvsctest3View::VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
{
	if(strcmp(from,"INDICES"))
		return;
	if((psaveAll)&&(psaveAll->query.select==select)&&(psaveAll->query.from==from)&&(psaveAll->query.where==where))
		psaveAll->query.rid=rid;
	else
	{
		if(rid==curq.rid)
		{
			curq.iter=iter;
			moreToGet=false;
		}
		else if((curq.from.IsEmpty())||(curq.from=="INDICES"))
		{
			// A different index has been selected in left view
			CListCtrl& clc=GetListCtrl();
			clc.DeleteAllItems();
			curq.rid=rid;
			curq.select=select;
			curq.from=from;
			curq.where=where;
			curq.maxorders=maxorders;
			curq.unique=unique;
			curq.istart=istart;
			curq.idir=idir;
			curq.iter=iter;
			curq.wresults=500; curq.nresults=0;
		#ifdef SPLIT_ORD_DET
			for(list<char*>::iterator bit=curq.bresults.begin();bit!=curq.bresults.end();bit++)
				delete *bit;
			curq.bresults.clear();
		#endif
			moreToGet=false;
		}
	}
}
void Cvsctest3View::VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// Index browse results:
	if((psaveAll)&&(rid==psaveAll->query.rid))
	{
		// CSV format
		CString Tmp;
		for(int i=0;i<nstr;i++)
		{
			Tmp="";
			const char *tok=strtoke((char*)pstr[i],"\1");
			for(int c=0;(tok)&&(c<IVCOL_COUNT);c++)
			{
				if(c==IVCOL_KEY)
				{
					// Quote any value that has a comma in it
					if(strchr(tok,','))
						Tmp+=(CString)"\""+tok+(CString)"\"";
					else
						Tmp+=tok;
				}
				tok=strtoke(0,"\1");
			}
			Tmp+="\r\n";
			DWORD wbytes=0;
			WriteFile(psaveAll->fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
			//psaveAll->cnt++;
			psaveAll->query.nresults++;
		}
		if(!more)
		{
			psaveAll->query.istart=endcnt +1;
			psaveAll->query.iter=iter;

			//rtmap.erase(rit);
			if((endcnt<totcnt)&&(iter))
				connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,psaveAll->query.rid,psaveAll->query.select,psaveAll->query.from,psaveAll->query.where,psaveAll->query.maxorders,psaveAll->query.unique,psaveAll->query.istart,psaveAll->query.idir,psaveAll->query.iter);
			else
			{
				CString Tmp;
				Tmp.Format("Total %d items\r\n",/*psaveAll->cnt*/psaveAll->query.nresults);
				DWORD wbytes=0;
				WriteFile(psaveAll->fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
				CloseHandle(psaveAll->fhnd); psaveAll->fhnd=INVALID_HANDLE_VALUE;
				LaunchTextViewer(psaveAll->fpath.c_str());
				delete psaveAll; psaveAll=0;
				//VIEW_SIZE=50;
			}
		}
		return;
	}

	if(rid!=curq.rid)
		return;
	CListCtrl& clc=GetListCtrl();
	char tsmin[32]={0};
	strcpy(tsmin,"00000000-0000");
	// Look for last minute separator
	if(strstr(curq.where,"TransactTime"))
	{
		for(int i=clc.GetItemCount()-1;i>=0;i--)
		{
			CString text=clc.GetItemText(i,IVCOL_KEY);
			if(text.Left(3)=="---")
			{
				memcpy(tsmin +9,(const char *)text +4,2);
				memcpy(tsmin +11,(const char *)text +7,2);
				break;
			}
			else if(!strncmp(tsmin,"00000000",8))
				memcpy(tsmin,(const char*)text,8);
		}
	}

	for(int i=0;i<nstr;i++)
	{
		LV_ITEM lvi;
		memset(&lvi,0,sizeof(LV_ITEM));
		lvi.iItem=clc.GetItemCount();
		int nidx=clc.InsertItem(&lvi);
		if(nidx>=0)
		{
			const char *asys="",*key="",*pos="",*order="";
			//const char *orderid="",*sym="",*side="",*price="",*shares="",*time="";
			int col=0;
			for(const char *tok=strtoke((char*)pstr[i],"\1");tok;tok=strtoke(0,"\1"))
			{
				switch(++col)
				{
				case 1: asys=tok; break;
				case 2: key=tok; break;
				case 3: pos=tok; break;
				case 4: order=tok; break;
				};
			}
			if(curq.asys.IsEmpty())
			{
				curq.asys=asys;
				#ifdef TIME_CONVERT
				if((curq.asys=="CLSERVER")||(curq.asys=="RTOATS")||(curq.asys=="TWIST"))
					curq.isgmt=false;
				#endif
			}
			if(strstr(curq.where,"TransactTime"))
			{
				#ifdef TIME_CONVERT
				if(curq.isgmt)
				{
					char kstr[32]={0};
					__int64 kval=_atoi64(key);
					SYSTEMTIME lst;
					CString stz;
					GMTToEST(kval,lst,stz);
					sprintf(kstr,"%04d%02d%02d-%02d%02d%02d.%03d %s",
						lst.wYear,lst.wMonth,lst.wDay,
						lst.wHour,lst.wMinute,lst.wSecond,
						lst.wMilliseconds,stz);
					clc.SetItemText(nidx,IVCOL_KEY,kstr);
				}
				else
				#endif
				{
					char kstr[32]={0};
					memset(kstr,0,sizeof(kstr));
					memcpy(kstr,key,8); kstr[8]='-';
					memcpy(kstr +9,key +8,6); 
					if(key[14])
					{
						kstr[15]='.'; memcpy(kstr +16,key +14,3); 
					}
					#ifdef TIME_CONVERT
					strcpy(kstr +19," EDT");
					#endif
					// Minute separator
					if(strncmp(tsmin,kstr,13)<0)
					{
						strncpy(tsmin,kstr,13);
						char mstr[16]={0};
						strcpy(mstr,"--- 00:00 ---");
						memcpy(mstr +4,kstr +9,2);
						memcpy(mstr +7,kstr +11,2);
						clc.SetItemText(nidx,IVCOL_KEY,mstr);
						clc.SetItemData(nidx,atoi(kstr));

						memset(&lvi,0,sizeof(LV_ITEM));
						lvi.iItem=clc.GetItemCount();
						nidx=clc.InsertItem(&lvi);
						if(nidx<0)
							break;
					}
					clc.SetItemText(nidx,IVCOL_APPSYSTEM,asys);
					clc.SetItemText(nidx,IVCOL_KEY,kstr);
				}
			}
			else
			{
				clc.SetItemText(nidx,IVCOL_APPSYSTEM,asys);
				clc.SetItemText(nidx,IVCOL_KEY,key);
			}
			clc.SetItemText(nidx,IVCOL_POSITION,pos);
			clc.SetItemText(nidx,IVCOL_ORDER,order);
			curq.nresults++;
			if(curq.wresults>0) curq.wresults--; 
		}
	}

	if(!more)
	{
		if(endcnt<totcnt)
		{
			curq.istart=endcnt +1;
			curq.iter=iter;
			moreToGet=true;

			LV_ITEM lvi;
			memset(&lvi,0,sizeof(LV_ITEM));
			lvi.mask=LVIF_IMAGE;
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();
			int nidx=clc.InsertItem(&lvi);
			if(nidx>=0)
				clc.SetItemText(nidx,IVCOL_KEY,"...");

			int eperc=endcnt*100/(totcnt?totcnt:1);
			smsg.Format("Browsed %u keys, searched %d%% (%u/%u), more",clc.GetItemCount() -1,eperc,endcnt,totcnt);
			theApp.pMainFrame->SetStatus(1,smsg);
			curq.wresults=500;

			if((theApp.browseAllTransactTimes)&&(strstr(curq.where,"TransactTime")))
			{
				NMITEMACTIVATE ItemActivate;
				memset(&ItemActivate,0,sizeof(NMITEMACTIVATE));
				ItemActivate.iItem=nidx;
				LRESULT Result=0;
				OnNMClick((NMHDR *)&ItemActivate,&Result);
			}
		}
		else
		{
			moreToGet=false;
			smsg.Format("Browsed all %u unique keys of %u",clc.GetItemCount(),totcnt);
			theApp.pMainFrame->SetStatus(1,smsg);
		}
		// Continue the index search
		if(indexSearchStart>=0)
			PostMessage(WM_COMMAND,ID_KEY,0);
	}
}
// For EncodeCancelRequest
void Cvsctest3View::VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
{
}
// For EncodeDescribeRequest
void Cvsctest3View::VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
{
}
void Cvsctest3View::VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// TODO:
}

// For EncodeLoginRequest2
void Cvsctest3View::VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
}
struct LAYOUT_NODE
{
	char name[80];
	int x;
	int y;
};
void Cvsctest3View::VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
{
	if(!rc)
	{
		CListCtrl& clc=GetListCtrl();
		clc.DeleteAllItems();
		// Didn't quite work out nicely
		//// We can't refresh the query but allow the user a chance to save the info
		//if(curq.from!="INDICES")
		//{
		//	CString title;
		//	title.Format("STALE %s",curq.where);
		//	GetParent()->SetWindowText(title);
		//}
		curq.rid=0;
		curq.select="";
		curq.from="";
		curq.where="";
		if(psaveAll)
		{
			delete psaveAll; psaveAll=0;
		}
		indexSearchStart=-1;
		savedOnce=false;
	}
}
// For EncodeSqlRequest2
void Cvsctest3View::VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
{
}
// For EncodeSqlIndexRequest2
void Cvsctest3View::VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
{
}

void Cvsctest3View::VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	// BTR details request
	if((psaveAll)&&(psaveAll->query.select==select)&&(psaveAll->query.from==from)&&(psaveAll->query.where==where))
		psaveAll->query.rid=rid;
	else if((curq.select==select)&&(curq.from==from)&&(curq.where==where)&&(!curq.nresults))
	{
		curq.rid=rid;
		moreToGet=false;
	#ifdef SPLIT_ORD_DET
		if(curq.from=="DETAILS")
		{
			CListCtrl& clc=GetListCtrl();
			clc.DeleteAllItems();
		}
		for(list<char*>::iterator bit=curq.bresults.begin();bit!=curq.bresults.end();bit++)
			delete *bit;
		curq.bresults.clear();
	#endif
	}
}
void Cvsctest3View::VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// Orders query results
	if((psaveAll)&&(rid==psaveAll->query.rid))	
	{
		if((curq.asys=="CLSERVER")||(theApp.btrdetails)) // Pass CLSERVER details for conversion
			ClserverSaveReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstrlen,pstr,nstr,more,iter,reason);
		else
		{
			// BTR format
			for(int i=0;i<nstr;i++)
			{
				DWORD wbytes=0;
				WriteFile(psaveAll->fhnd,pstr[i],pstrlen,&wbytes,0);
				//psaveAll->cnt++;
				psaveAll->query.nresults++;
			}
		}
		if((hist)&&(!more))
		{
			psaveAll->query.iter=iter;

			//rtmap.erase(rit);
			if(iter>0)
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,psaveAll->query.select,psaveAll->query.from,psaveAll->query.where,psaveAll->query.maxorders,psaveAll->query.hist,psaveAll->query.live,psaveAll->query.iter);
			else
			{
				CString Tmp;
				Tmp.Format("Total %d items\r\n",/*psaveAll->cnt*/psaveAll->query.nresults);
				DWORD wbytes=0;
				WriteFile(psaveAll->fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
				CloseHandle(psaveAll->fhnd); psaveAll->fhnd=INVALID_HANDLE_VALUE;
				//LaunchFixViewer(psaveAll->fpath.c_str());
				LaunchTextViewer(psaveAll->fpath.c_str());
				delete psaveAll; psaveAll=0;
				//VIEW_SIZE=500;
			}
		}
		return;
	}

	if(rid!=curq.rid)
		return;
	switch(proto)
	{
	case /*PROTO_CLDROP*/0x0C:
	case /*PROTO_DAT*/0x15:
		ClserverReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstrlen,pstr,nstr,more,iter,reason);
		break;
	case /*PROTO_FIXS_PLACELOG*/0x16:
	case /*PROTO_FIXS_CXLLOG*/0x17:
	case /*PROTO_FIXS_RPTLOG*/0x18:
	case /*PROTO_TRADER_PLACELOG*/0x1B:
	case /*PROTO_TRADER_CXLLOG*/0x1C:
	case /*PROTO_TRADER_RPTLOG*/0x1D:
		FIXServerReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstrlen,pstr,nstr,more,iter,reason);
		break;
	};
	return;
}
void Cvsctest3View::ClserverReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	_ASSERT((proto==/*PROTO_CLDROP*/0x0C)||(proto==0x15));

#define EXCHANGE_INDEX	0x1b	// 27
#define EXCHANGE_BB		0x0b	// 11
#define EXCHANGE_OPRA	0x0c	// 12
#define EXCHANGE_OPTION	0x0c	// 12
#define EXCHANGE_NYSE	0x0d	// 13
#define EXCHANGE_AMEX	0x0e	// 14
#define EXCHANGE_NASDAQ	0x0f	// 15
#define EXCHANGE_BOND	0x03
#define EXCHANGE_DUMMY	0x04

#define EXCHANGE_SPREAD	22

#define EXCHANGE_INTERNATIONAL 0x11 // 17
#define EXCHANGE_A4_UNKNOWN	100
#define EXCHANGE_UNKNOWN 99


#define EXCHANGE_FUTURES 0x10	// 16
#define EXCHANGE_MINI_FUTURES 0x1C	// 28
#define EXCHANGE_CBOT_FUTURES 19

	CListCtrl& clc=GetListCtrl();
	char vstr[4096]={0};
	for(int i=0;i<nstr;i++)
	{
		switch(pstrlen)
		{
		case sizeof(BIGTRADEREC):
		{
			LVITEM lvi;
			memset(&lvi,0,sizeof(LVITEM));
			lvi.mask=LVIF_IMAGE; 
			lvi.iImage=-1;
			lvi.iItem=clc.GetItemCount();	
			int nidx=clc.InsertItem(&lvi);

			if(nidx>=0)
			{
				//clc.SetItemText(nidx,RCOL_HIST,hist?"H":"L");
				_snprintf(vstr,sizeof(vstr)-1,"%d",pstrlen);
				clc.SetItemText(nidx,RCOL_FIXLEN,vstr);
				sprintf(vstr,"%d",/*vpages[curpage].endidx++*/nidx+1);
				clc.SetItemText(nidx,RCOL_POSITION,vstr);

			#ifdef LINK_TRADERLOOK
				BIGTRADEREC* btr=new BIGTRADEREC;
				_ASSERT(pstrlen==sizeof(BIGTRADEREC));
				memcpy(btr,pstr[i],sizeof(BIGTRADEREC));
				clc.SetItemData(nidx,(DWORD_PTR)btr);
			#else
				#ifdef SPLIT_ORD_DET
				BIGTRADEREC* btr=new BIGTRADEREC;
				memcpy(btr,pstr[i],sizeof(BIGTRADEREC));
				curq.bresults.push_back((char*)btr);
				clc.SetItemData(nidx,(DWORD_PTR)btr);
				#else
				BIGTRADEREC* btr=(BIGTRADEREC*)pstr[i];
				clc.SetItemData(nidx,(DWORD_PTR)nidx +1);
				#endif
			#endif

				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.Account);
				clc.SetItemText(nidx,RCOL_ACCOUNT,vstr);

				switch(btr->TradeRec.Action) {
					case 1: sprintf(vstr,"Buy"); break;
					case 2: sprintf(vstr,"Sell"); break;
					case 3: sprintf(vstr,"Short"); break;
					default: sprintf(vstr,"Unknown"); break;
				}
				clc.SetItemText(nidx,RCOL_ACTION,vstr);

				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.AON?'1':'0');
				clc.SetItemText(nidx,RCOL_AON,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.AvgPrice);
				clc.SetItemText(nidx,RCOL_AAVGPX,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.BofaExecutionServicesDomain?'1':'0');
				clc.SetItemText(nidx,RCOL_BOFAEXECUTIONSERVICESDOMAIN,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.BofaExecutionServicesOrder?'1':'0');
				clc.SetItemText(nidx,RCOL_BOFAEXECUTIONSERVICESORDER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.Canceled);
				clc.SetItemText(nidx,RCOL_CANCELLED,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.CancelledByExchange?'1':'0');
				clc.SetItemText(nidx,RCOL_CANCELLED_BY_EXCHANGE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.CancelRequested?'1':'0');
				clc.SetItemText(nidx,RCOL_CANCEL_REQUESTED,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%.f",btr->TradeRec.DiscretionaryAmount);
				clc.SetItemText(nidx,RCOL_DISCRETIONARYAMOUNT,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.Domain);
				clc.SetItemText(nidx,RCOL_DOMAIN,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.DoNotClear?'1':'0');
				clc.SetItemText(nidx,RCOL_DONOTCLEAR,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.ECNId);
				clc.SetItemText(nidx,RCOL_ECNID,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRecA.FIXLineId);
				clc.SetItemText(nidx,RCOL_FIXLINEID,vstr);				
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.ECNOrderNo);
				clc.SetItemText(nidx,RCOL_ECNORDERNO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.EditedByBo?'1':'0');
				clc.SetItemText(nidx,RCOL_EDITEDBYBO,vstr);
						
				//switch(btr->TradeRec.ExSymbol.CurrencyCode) {
				//	case CURRENCY_USD: sprintf(vstr,"USD"); break;
				//	case CURRENCY_CAD: sprintf(vstr,"CAD"); break;
				//	default: sprintf(vstr,"Unknown"); break;
				//}
				clc.SetItemText(nidx,RCOL_EXSYMBOL_CURRENCYCODE,vstr);

				switch(btr->TradeRec.ExSymbol.Exchange) {
					case EXCHANGE_INDEX: sprintf(vstr,"Index"); break;
					case EXCHANGE_BB:  sprintf(vstr,"BB"); break;
					case EXCHANGE_OPRA: sprintf(vstr,"Option/Opra"); break;
//							case EXCHANGE_OPTION: sprintf(vstr,"Option"); break;
					case EXCHANGE_NYSE: sprintf(vstr,"NYSE"); break;
					case EXCHANGE_AMEX: sprintf(vstr,"AMEX"); break;
					case EXCHANGE_NASDAQ: sprintf(vstr,"NASDAQ"); break;
					case EXCHANGE_BOND: sprintf(vstr,"BOND"); break;
					case EXCHANGE_DUMMY: sprintf(vstr,"DUMMY"); break;
					case EXCHANGE_SPREAD: sprintf(vstr,"SPREAD"); break;
					case EXCHANGE_INTERNATIONAL: sprintf(vstr,"International"); break;
					case EXCHANGE_A4_UNKNOWN: sprintf(vstr,"A4_Unknown"); break;
					case EXCHANGE_UNKNOWN: sprintf(vstr,"Unknown"); break;
					case EXCHANGE_FUTURES: sprintf(vstr,"futures"); break;
					case EXCHANGE_MINI_FUTURES: sprintf(vstr,"Mini fut"); break;
					case EXCHANGE_CBOT_FUTURES: sprintf(vstr,"CBOT fut"); break;
					default: sprintf(vstr,"Unknown"); break;
				}
				clc.SetItemText(nidx,RCOL_EXSYMBOL_EXCHANGE,vstr);

				if(btr->TradeRec.ExSymbol.Region)
					_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.ExSymbol.Region);
				else
					vstr[0]=0;
				clc.SetItemText(nidx,RCOL_EXSYMBOL_REGION,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.ExSymbol.Symbol);
				clc.SetItemText(nidx,RCOL_EXSYMBOL_SYMBOL,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.Ext?'1':'0');
				clc.SetItemText(nidx,RCOL_EXT,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.FileDate);
				clc.SetItemText(nidx,RCOL_FILEDATE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.FOK?'1':'0');
				clc.SetItemText(nidx,RCOL_FOK,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.IOC_ArcaNow?'1':'0');
				clc.SetItemText(nidx,RCOL_ARCANOW,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.LastDate);
				clc.SetItemText(nidx,RCOL_LASTDATE,vstr);

				char highMsgType=0,highExecType=0;
				switch(btr->TradeRec.LastStatus) {
					case TRADE_STOPPED: sprintf(vstr,"Stopped"); highExecType='7'; break;
					case WILL_LOCK: sprintf(vstr,"Will lock"); break;
					case CANCEL_SENT: sprintf(vstr,"Cxl Sent"); highMsgType='F'; break;
					case CANCEL_RECEIVED: sprintf(vstr,"Cxl Received"); highMsgType='F'; break;
					case CANCEL_PLACED: sprintf(vstr,"Cxl Placed"); highMsgType='F'; break;
					case TRADE_SENT: sprintf(vstr,"Trade Sent"); break;
					case TRADE_RECEIVED: 
						if(btr->TradeRec.ReplaceRequest)
						{
							sprintf(vstr,"Rpl Received"); highMsgType='G';
						}
						else
						{
							sprintf(vstr,"Received"); highMsgType='D'; 
						}
						break;
					case TRADE_PLACED: 
						if(btr->TradeRec.ReplaceRequest)
						{
							sprintf(vstr,"Rpl Placed"); highMsgType='G';
						}
						else
						{
							sprintf(vstr,"Placed"); highMsgType='D'; 
						}
						break;
					case TRADE_CONFIRMED: sprintf(vstr,"Confirmed"); highMsgType='D'; highExecType='0'; break;
					case TRADE_PARTFILL: sprintf(vstr,"PartFill"); highExecType='1'; break;
					case TRADE_FILLED: sprintf(vstr,"Filled"); highExecType='2'; break;
					case TRADE_EXPIRED: sprintf(vstr,"Expired"); highExecType='3'; break;
					case TRADE_CANCELED: sprintf(vstr,"Cancelled"); highExecType='4'; break;
					case TRADE_CANCELPENDING: sprintf(vstr,"Cxl Pending"); highExecType='6'; break;
					case TRADE_REJECTED : sprintf(vstr,"Rejected"); highExecType='8'; break;
					case 100:
					case 104: sprintf(vstr,"Cxl Reject"); highExecType='9'; break;
					default: sprintf(vstr,"Unknown"); break;
				}
				if(btr->TradeRec.CancelRequested)
				{
					sprintf(vstr,"Cxl Placed"); highMsgType='F'; highExecType=0;
				}
				clc.SetItemText(nidx,RCOL_LASTSTATUS,vstr);

				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.LastTime);
				clc.SetItemText(nidx,RCOL_LASTTIME,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.LastPrice);
				clc.SetItemText(nidx,RCOL_LASTPX,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.LastShares);
				clc.SetItemText(nidx,RCOL_LASTSHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.ExeShares);
				clc.SetItemText(nidx,RCOL_EXESHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.OpenShares);
				clc.SetItemText(nidx,RCOL_OPENSHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.PrevOpenShares);
				clc.SetItemText(nidx,RCOL_PREVOPENSHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.ClosedShares);
				clc.SetItemText(nidx,RCOL_CLOSEDSHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.MasterSlave);
				clc.SetItemText(nidx,RCOL_MASTERSLAVE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.MMId);
				clc.SetItemText(nidx,RCOL_MMID,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OnClose?'1':'0');
				clc.SetItemText(nidx,RCOL_ONCLOSE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OnOpen?'1':'0');
				clc.SetItemText(nidx,RCOL_ONOPEN,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.OrderNo);
				clc.SetItemText(nidx,RCOL_ORDERNO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.CancelMemo);
				clc.SetItemText(nidx,RCOL_CANCELMEMO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OrderWasCancelReplaced?'1':'0');
				clc.SetItemText(nidx,RCOL_ORDERWASCANCELREPLACED,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegBest?'1':'0');
				clc.SetItemText(nidx,RCOL_PEGBEST,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegLast?'1':'0');
				clc.SetItemText(nidx,RCOL_PEGLAST,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegMid?'1':'0');
				clc.SetItemText(nidx,RCOL_PEGMMID,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegMkt?'1':'0');
				clc.SetItemText(nidx,RCOL_PEGMKT,vstr);
				//if(btr->TradeRec.PegOrder)
				//	_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegOrder);
				//else
				//	vstr[0]=0;
				//clc.SetItemText(nidx,RCOL_PEGORDER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegPrime?'1':'0');
				clc.SetItemText(nidx,RCOL_PEGPRIME,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.Pending);
				clc.SetItemText(nidx,RCOL_PENDING,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.PrefECN);
				clc.SetItemText(nidx,RCOL_PREFECN,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.PrefMMID);
				clc.SetItemText(nidx,RCOL_PREFMMID,vstr);
				//_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.PrevOrderNumber);
				clc.SetItemText(nidx,RCOL_PREVORDERNUMBER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.Price);
				clc.SetItemText(nidx,RCOL_PRICE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.ReplaceRequest?'1':'0');
				clc.SetItemText(nidx,RCOL_REPLACEREQUEST,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.ReservedShares);
				clc.SetItemText(nidx,RCOL_RESERVEDSHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.Shares);
				clc.SetItemText(nidx,RCOL_SHARES,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.StopAmount);
				clc.SetItemText(nidx,RCOL_STOPAMOUNT,vstr);
				if(btr->TradeRec.StopType)
					_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.StopType);
				else
					vstr[0]=0;
				clc.SetItemText(nidx,RCOL_STOPTYPE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.TradeDetRef);
				clc.SetItemText(nidx,RCOL_TRADEDETREF,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.Type==1?"Mkt":"Lmt");
				clc.SetItemText(nidx,RCOL_TYPE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.User);
				clc.SetItemText(nidx,RCOL_USER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.UserType);
				clc.SetItemText(nidx,RCOL_USERTYPE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.WorkOrderRefNo);
				clc.SetItemText(nidx,RCOL_WORKORDERREFNO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.ECNParserOrderNo);
				clc.SetItemText(nidx,RCOL_ECNPARSERORDERNO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.LinkOrderNo);
				clc.SetItemText(nidx,RCOL_LINKORDERNO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.Memo);
				clc.SetItemText(nidx,RCOL_MEMO,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.nullByte?"":btr->TradeRecA.OATSConnectionID);
				clc.SetItemText(nidx,RCOL_OATSCONNECTIONID,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecA.OatsManualTicket?'1':'0');
				clc.SetItemText(nidx,RCOL_OATSMANUALTICKET,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecA.OatsNonDirectedOrder?'1':'0');
				clc.SetItemText(nidx,RCOL_OATSNONDIRECTEDORDER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRecA.OrderType);
				clc.SetItemText(nidx,RCOL_ORDERTYPE,vstr);
				if(btr->TradeRecA.ShortExemptCode)
					_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecA.ShortExemptCode);
				else
					vstr[0]=0;
				clc.SetItemText(nidx,RCOL_SHORTEXEMPTCODE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.ComplexOrder?'1':'0');
				clc.SetItemText(nidx,RCOL_COMPLEXORDER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.ComplexOrderParent?'1':'0');
				clc.SetItemText(nidx,RCOL_COMPLEXORDERPARENT,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.OrderStagingOrder?'1':'0');
				clc.SetItemText(nidx,RCOL_ORDERSTAGINGORDER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.OrderStagingParent?'1':'0');
				clc.SetItemText(nidx,RCOL_ORDERSTAGINGPARENT,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.NotHeldOrder?'1':'0');
				clc.SetItemText(nidx,RCOL_NOTHELDORDER,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.NotOATSReportable?'1':'0');
				clc.SetItemText(nidx,RCOL_NOTOATSREPORTABLE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.OVDSpclHndling?'1':'0');
				clc.SetItemText(nidx,RCOL_OVDSPCLHNDLING,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.TraderGiveUp);
				clc.SetItemText(nidx,RCOL_TRADERGIVEUP,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.WRKSpclHndling?'1':'0');
				clc.SetItemText(nidx,RCOL_WRKSPCLHNDLING,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.StrategyName);
				clc.SetItemText(nidx,RCOL_STRATEGYNAME,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OverrideComms?'1':'0');
				clc.SetItemText(nidx,RCOL_OVERRIDECOMMS,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.CommissionOverrideType);
				clc.SetItemText(nidx,RCOL_COMMISSIONOVERRIDETYPE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRecB.CommissionOverride);
				clc.SetItemText(nidx,RCOL_COMMISSIONOVERRIDE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.TranTypeCode);
				clc.SetItemText(nidx,RCOL_TRANTYPECODE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.RediDestination);
				clc.SetItemText(nidx,RCOL_REDIDESTINATION,vstr);

				_snprintf(vstr,sizeof(vstr)-1,"%d",nidx+1);
				clc.SetItemText(nidx,RCOL_POSITION,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.Currency);
				clc.SetItemText(nidx,RCOL_CURRENCY,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.SettlementCurrency);
				clc.SetItemText(nidx,RCOL_SETTLEMENT_CURRENCY,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRecA.ProductType);
				clc.SetItemText(nidx,RCOL_PRODUCT_TYPE,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRecA.RegionGroup);
				clc.SetItemText(nidx,RCOL_REGION,vstr);
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.ClosePosition);
				clc.SetItemText(nidx,RCOL_CLOSEPOSITION,vstr);
				int img=theApp.OrderImage(highMsgType,highExecType,0,0);
				clc.SetItem(nidx,RCOL_LASTSTATUS,LVIF_IMAGE,0,img,0,0,0,0);
				//clc.SetItem(nidx,0,LVIF_IMAGE,0,-1,0,0,0,0); // Blank out the image in the first column

				// Aggregates
				if(btr->TradeRec.LastShares>0)
				{
					curq.fillQty+=btr->TradeRec.LastShares;
					curq.fillSum+=(btr->TradeRec.LastPrice*btr->TradeRec.LastShares);
				}

				curq.nresults++;
				if(curq.wresults>0) curq.wresults--; 
				for(int col=0;col<RCOL_COUNT;col++)
				{
					CString text=clc.GetItemText(nidx,col);
				}
			}
			break;
		}
		// VSDB never really stores the ACCOUNTREC, just the account names
		//case sizeof(ACCOUNTREC):
		//{
		//	_ASSERT(false);//untested
		//	LVITEM lvi;
		//	memset(&lvi,0,sizeof(LVITEM));
		//	lvi.mask=0;
		//	lvi.iItem=clc.GetItemCount();
		//	int nidx=clc.InsertItem(&lvi);

		//	if(nidx>=0)
		//	{
		//		//clc.SetItemText(nidx,ACOL_HIST,hist?"H":"L");
		//		_snprintf(vstr,sizeof(vstr)-1,"%d",pstrlen);
		//		clc.SetItemText(nidx,ACOL_LEN,vstr);
		//		sprintf(vstr,"%d",/*vpages[curpage].endidx++*/nidx+1);
		//		clc.SetItemText(nidx,ACOL_POSITION,vstr);

		//		ACCOUNTREC* ac=(ACCOUNTREC*)pstr[i];

		//		_snprintf(vstr,sizeof(vstr)-1,"%s",ac->Account);
		//		clc.SetItemText(nidx,ACOL_ACCOUNT,vstr);
		//		_snprintf(vstr,sizeof(vstr)-1,"%s",ac->Domain);
		//		clc.SetItemText(nidx,ACOL_DOMAIN,vstr);
		//	}
		//	break;
		//}
		}; // End switch
	}

	if((!more)&&(hist))
	{
		// Auto-size to contents
		CHeaderCtrl *chc=clc.GetHeaderCtrl();
		for(int i=0;i<chc->GetItemCount();i++)
			clc.SetColumnWidth(i,LVSCW_AUTOSIZE);
		// Add extra for empty bitmap in first column
		int width0=clc.GetColumnWidth(0);
		clc.SetColumnWidth(0,width0 +16);

		if(iter)
		{
			curq.iter=iter;
			moreToGet=true;

			int eperc=endcnt*100/(totcnt?totcnt:1);
			// Auto-continue when we've received less than 500 results for this block of results
			if(curq.wresults>0)
			{
				smsg.Format("Browsed %u details, searched %d%% (%u/%u), filled %d, avgpx %.4f, ...",
					clc.GetItemCount() -1,eperc,endcnt,totcnt,curq.fillQty,curq.fillSum/(curq.fillQty?curq.fillQty:1));
				theApp.pMainFrame->SetStatus(1,smsg);

				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.mask=LVIF_IMAGE;
				lvi.iImage=-1;
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
					clc.SetItemText(nidx,RCOL_ACCOUNT,"...");

				clc.SetItemState(nidx,LVIS_SELECTED,LVIS_SELECTED);
				NMITEMACTIVATE ItemActivate;
				memset(&ItemActivate,0,sizeof(NMITEMACTIVATE));
				ItemActivate.iItem=nidx;
				LRESULT Result=0;
				#ifdef SPLIT_ORD_DET
				OnNMClick((NMHDR *)&ItemActivate,&Result);
				#else
				OnNMDblclk((NMHDR *)&ItemActivate,&Result);
				#endif
			}
			else
			{
				clc.SortItems(_SortBTR,(DWORD_PTR)this);

				LV_ITEM lvi;
				memset(&lvi,0,sizeof(LV_ITEM));
				lvi.mask=LVIF_IMAGE;
				lvi.iImage=-1;
				lvi.iItem=clc.GetItemCount();
				int nidx=clc.InsertItem(&lvi);
				if(nidx>=0)
					clc.SetItemText(nidx,RCOL_ACCOUNT,"...");

				smsg.Format("Browsed %u details, searched %d%% (%u/%u), filled %d, avgpx %.4f, more",
					clc.GetItemCount() -1,eperc,endcnt,totcnt,curq.fillQty,curq.fillSum/(curq.fillQty?curq.fillQty:1));
				theApp.pMainFrame->SetStatus(1,smsg);
				curq.wresults=500;
			}
		}
		else
		{
			clc.SortItems(_SortBTR,(DWORD_PTR)this);
			moreToGet=false;
			smsg.Format("Browsed all %u details of %u, filled %d, avgpx %.4f",
				clc.GetItemCount(),totcnt,curq.fillQty,curq.fillSum/(curq.fillQty?curq.fillQty:1));
			theApp.pMainFrame->SetStatus(1,smsg);
		}
	}
}
void Cvsctest3View::ClserverSaveReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	_ASSERT((proto==/*PROTO_CLDROP*/0x0C)||(proto==0x15));
	if((!psaveAll)||(rid!=psaveAll->query.rid))
		return;

#define EXCHANGE_INDEX	0x1b	// 27
#define EXCHANGE_BB		0x0b	// 11
#define EXCHANGE_OPRA	0x0c	// 12
#define EXCHANGE_OPTION	0x0c	// 12
#define EXCHANGE_NYSE	0x0d	// 13
#define EXCHANGE_AMEX	0x0e	// 14
#define EXCHANGE_NASDAQ	0x0f	// 15
#define EXCHANGE_BOND	0x03
#define EXCHANGE_DUMMY	0x04

#define EXCHANGE_SPREAD	22

#define EXCHANGE_INTERNATIONAL 0x11 // 17
#define EXCHANGE_A4_UNKNOWN	100
#define EXCHANGE_UNKNOWN 99


#define EXCHANGE_FUTURES 0x10	// 16
#define EXCHANGE_MINI_FUTURES 0x1C	// 28
#define EXCHANGE_CBOT_FUTURES 19

	CListCtrl& clc=GetListCtrl();
	char wbuf[4096]={0},*wptr=wbuf,vstr[4096]={0};
	for(int i=0;i<nstr;i++)
	{
		switch(pstrlen)
		{
		case sizeof(BIGTRADEREC):
		{
			wptr=wbuf;
			//clc.SetItemText(nidx,RCOL_HIST,hist?"H":"L");
			_snprintf(vstr,sizeof(vstr)-1,"%d",pstrlen);
			//clc.SetItemText(nidx,RCOL_FIXLEN,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			//sprintf(vstr,"%d",/*vpages[curpage].endidx++*/nidx+1);
			//clc.SetItemText(nidx,RCOL_POSITION,vstr);

		#ifdef LINK_TRADERLOOK
			BIGTRADEREC* btr=new BIGTRADEREC;
		//	_ASSERT(pstrlen==sizeof(BIGTRADEREC));
		//	memcpy(btr,pstr[i],sizeof(BIGTRADEREC));
		//	clc.SetItemData(nidx,(DWORD_PTR)btr);
		#else
		//	clc.SetItemData(nidx,(DWORD_PTR)nidx +1);
			BIGTRADEREC* btr=(BIGTRADEREC *)pstr[i];
		#endif

			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.Account);
			//clc.SetItemText(nidx,RCOL_ACCOUNT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			switch(btr->TradeRec.Action) {
				case 1: sprintf(vstr,"Buy"); break;
				case 2: sprintf(vstr,"Sell"); break;
				case 3: sprintf(vstr,"Short"); break;
				default: sprintf(vstr,"Unknown"); break;
			}
			//clc.SetItemText(nidx,RCOL_ACTION,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.AON?'1':'0');
			//clc.SetItemText(nidx,RCOL_AON,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.AvgPrice);
			//clc.SetItemText(nidx,RCOL_AAVGPX,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.BofaExecutionServicesDomain?'1':'0');
			//clc.SetItemText(nidx,RCOL_BOFAEXECUTIONSERVICESDOMAIN,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.BofaExecutionServicesOrder?'1':'0');
			//clc.SetItemText(nidx,RCOL_BOFAEXECUTIONSERVICESORDER,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.Canceled);
			//clc.SetItemText(nidx,RCOL_CANCELLED,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.CancelledByExchange?'1':'0');
			//clc.SetItemText(nidx,RCOL_CANCELLED_BY_EXCHANGE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.CancelRequested?'1':'0');
			//clc.SetItemText(nidx,RCOL_CANCEL_REQUESTED,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%.f",btr->TradeRec.DiscretionaryAmount);
			//clc.SetItemText(nidx,RCOL_DISCRETIONARYAMOUNT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.Domain);
			//clc.SetItemText(nidx,RCOL_DOMAIN,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.DoNotClear?'1':'0');
			//clc.SetItemText(nidx,RCOL_DONOTCLEAR,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.ECNId);
			//clc.SetItemText(nidx,RCOL_ECNID,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRecA.FIXLineId);
			//clc.SetItemText(nidx,RCOL_FIXLINEID,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);			
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.ECNOrderNo);
			//clc.SetItemText(nidx,RCOL_ECNORDERNO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.EditedByBo?'1':'0');
			//clc.SetItemText(nidx,RCOL_EDITEDBYBO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
						
			//switch(btr->TradeRec.ExSymbol.CurrencyCode) {
			//	case CURRENCY_USD: sprintf(vstr,"USD"); break;
			//	case CURRENCY_CAD: sprintf(vstr,"CAD"); break;
			//	default: sprintf(vstr,"Unknown"); break;
			//}
			//clc.SetItemText(nidx,RCOL_EXSYMBOL_CURRENCYCODE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			switch(btr->TradeRec.ExSymbol.Exchange) {
				case EXCHANGE_INDEX: sprintf(vstr,"Index"); break;
				case EXCHANGE_BB:  sprintf(vstr,"BB"); break;
				case EXCHANGE_OPRA: sprintf(vstr,"Option/Opra"); break;
//							case EXCHANGE_OPTION: sprintf(vstr,"Option"); break;
				case EXCHANGE_NYSE: sprintf(vstr,"NYSE"); break;
				case EXCHANGE_AMEX: sprintf(vstr,"AMEX"); break;
				case EXCHANGE_NASDAQ: sprintf(vstr,"NASDAQ"); break;
				case EXCHANGE_BOND: sprintf(vstr,"BOND"); break;
				case EXCHANGE_DUMMY: sprintf(vstr,"DUMMY"); break;
				case EXCHANGE_SPREAD: sprintf(vstr,"SPREAD"); break;
				case EXCHANGE_INTERNATIONAL: sprintf(vstr,"International"); break;
				case EXCHANGE_A4_UNKNOWN: sprintf(vstr,"A4_Unknown"); break;
				case EXCHANGE_UNKNOWN: sprintf(vstr,"Unknown"); break;
				case EXCHANGE_FUTURES: sprintf(vstr,"futures"); break;
				case EXCHANGE_MINI_FUTURES: sprintf(vstr,"Mini fut"); break;
				case EXCHANGE_CBOT_FUTURES: sprintf(vstr,"CBOT fut"); break;
				default: sprintf(vstr,"Unknown"); break;
			}
			//clc.SetItemText(nidx,RCOL_EXSYMBOL_EXCHANGE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.ExSymbol.Symbol);
			//clc.SetItemText(nidx,RCOL_EXSYMBOL_SYMBOL,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			if(btr->TradeRec.ExSymbol.Region)
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.ExSymbol.Region);
			else
				vstr[0]=0;
			//clc.SetItemText(nidx,RCOL_EXSYMBOL_REGION,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.Ext?'1':'0');
			//clc.SetItemText(nidx,RCOL_EXT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.FileDate);
			//clc.SetItemText(nidx,RCOL_FILEDATE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.FOK?'1':'0');
			//clc.SetItemText(nidx,RCOL_FOK,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.IOC_ArcaNow?'1':'0');
			//clc.SetItemText(nidx,RCOL_ARCANOW,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.LastDate);
			//clc.SetItemText(nidx,RCOL_LASTDATE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			char highMsgType=0,highExecType=0;
			switch(btr->TradeRec.LastStatus) {
				case TRADE_STOPPED: sprintf(vstr,"Stopped"); highExecType='7'; break;
				case WILL_LOCK: sprintf(vstr,"Will lock"); break;
				case CANCEL_SENT: sprintf(vstr,"Cxl Sent"); highMsgType='F'; break;
				case CANCEL_RECEIVED: sprintf(vstr,"Cxl Received"); highMsgType='F'; break;
				case CANCEL_PLACED: sprintf(vstr,"Cxl Placed"); highMsgType='F'; break;
				case TRADE_SENT: sprintf(vstr,"Trade Sent"); break;
				case TRADE_RECEIVED: 
					if(btr->TradeRec.ReplaceRequest)
					{
						sprintf(vstr,"Rpl Received"); highMsgType='G';
					}
					else
					{
						sprintf(vstr,"Received"); highMsgType='D'; 
					}
					break;
				case TRADE_PLACED: 
					if(btr->TradeRec.ReplaceRequest)
					{
						sprintf(vstr,"Rpl Placed"); highMsgType='G';
					}
					else
					{
						sprintf(vstr,"Placed"); highMsgType='D'; 
					}
					break;
				case TRADE_CONFIRMED: sprintf(vstr,"Confirmed"); highMsgType='D'; highExecType='0'; break;
				case TRADE_PARTFILL: sprintf(vstr,"PartFill"); highExecType='1'; break;
				case TRADE_FILLED: sprintf(vstr,"Filled"); highExecType='2'; break;
				case TRADE_EXPIRED: sprintf(vstr,"Expired"); highExecType='3'; break;
				case TRADE_CANCELED: sprintf(vstr,"Cancelled"); highExecType='4'; break;
				case TRADE_CANCELPENDING: sprintf(vstr,"Cxl Pending"); highExecType='6'; break;
				case TRADE_REJECTED : sprintf(vstr,"Rejected"); highExecType='8'; break;
				default: sprintf(vstr,"Unknown"); break;
			}
			if(btr->TradeRec.CancelRequested)
			{
				sprintf(vstr,"Cxl Placed"); highMsgType='F'; highExecType=0;
			}
			//clc.SetItemText(nidx,RCOL_LASTSTATUS,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.LastTime);
			//clc.SetItemText(nidx,RCOL_LASTTIME,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.LastPrice);
			//clc.SetItemText(nidx,RCOL_LASTPX,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.LastShares);
			//clc.SetItemText(nidx,RCOL_LASTSHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.ExeShares);
			//clc.SetItemText(nidx,RCOL_EXESHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.OpenShares);
			//clc.SetItemText(nidx,RCOL_OPENSHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.PrevOpenShares);
			//clc.SetItemText(nidx,RCOL_PREVOPENSHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.ClosedShares);
			//clc.SetItemText(nidx,RCOL_CLOSEDSHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.MasterSlave);
			//clc.SetItemText(nidx,RCOL_MASTERSLAVE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.MMId);
			//clc.SetItemText(nidx,RCOL_MMID,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OnClose?'1':'0');
			//clc.SetItemText(nidx,RCOL_ONCLOSE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OnOpen?'1':'0');
			//clc.SetItemText(nidx,RCOL_ONOPEN,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.OrderNo);
			//clc.SetItemText(nidx,RCOL_ORDERNO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.CancelMemo);
			//clc.SetItemText(nidx,RCOL_CANCELMEMO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OrderWasCancelReplaced?'1':'0');
			//clc.SetItemText(nidx,RCOL_ORDERWASCANCELREPLACED,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegBest?'1':'0');
			//clc.SetItemText(nidx,RCOL_PEGBEST,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegLast?'1':'0');
			//clc.SetItemText(nidx,RCOL_PEGLAST,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegMid?'1':'0');
			//clc.SetItemText(nidx,RCOL_PEGMMID,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegMkt?'1':'0');
			//clc.SetItemText(nidx,RCOL_PEGMKT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			//if(btr->TradeRec.PegOrder)
			//	_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegOrder);
			//else
			//	vstr[0]=0;
			//clc.SetItemText(nidx,RCOL_PEGORDER,vstr);
			//sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.PegPrime?'1':'0');
			//clc.SetItemText(nidx,RCOL_PEGPRIME,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.Pending);
			//clc.SetItemText(nidx,RCOL_PENDING,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.PrefECN);
			//clc.SetItemText(nidx,RCOL_PREFECN,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.PrefMMID);
			//clc.SetItemText(nidx,RCOL_PREFMMID,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			//_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.PrevOrderNumber);
			//clc.SetItemText(nidx,RCOL_PREVORDERNUMBER,vstr);
			*vstr=0;
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.Price);
			//clc.SetItemText(nidx,RCOL_PRICE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.ReplaceRequest?'1':'0');
			//clc.SetItemText(nidx,RCOL_REPLACEREQUEST,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.ReservedShares);
			//clc.SetItemText(nidx,RCOL_RESERVEDSHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.Shares);
			//clc.SetItemText(nidx,RCOL_SHARES,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRec.StopAmount);
			//clc.SetItemText(nidx,RCOL_STOPAMOUNT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			if(btr->TradeRec.StopType)
				_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.StopType);
			else
				vstr[0]=0;
			//clc.SetItemText(nidx,RCOL_STOPTYPE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.TradeDetRef);
			//clc.SetItemText(nidx,RCOL_TRADEDETREF,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.Type==1?"Mkt":"Lmt");
			//clc.SetItemText(nidx,RCOL_TYPE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRec.User);
			//clc.SetItemText(nidx,RCOL_USER,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.UserType);
			//clc.SetItemText(nidx,RCOL_USERTYPE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%ld",btr->TradeRec.WorkOrderRefNo);
			//clc.SetItemText(nidx,RCOL_WORKORDERREFNO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.ECNParserOrderNo);
			//clc.SetItemText(nidx,RCOL_ECNPARSERORDERNO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.LinkOrderNo);
			//clc.SetItemText(nidx,RCOL_LINKORDERNO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.Memo);
			//clc.SetItemText(nidx,RCOL_MEMO,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecA.nullByte?"":btr->TradeRecA.OATSConnectionID);
			//clc.SetItemText(nidx,RCOL_OATSCONNECTIONID,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecA.OatsManualTicket?'1':'0');
			//clc.SetItemText(nidx,RCOL_OATSMANUALTICKET,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecA.OatsNonDirectedOrder?'1':'0');
			//clc.SetItemText(nidx,RCOL_OATSNONDIRECTEDORDER,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRecA.OrderType);
			//clc.SetItemText(nidx,RCOL_ORDERTYPE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			if(btr->TradeRecA.ShortExemptCode)
				_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecA.ShortExemptCode);
			else
				vstr[0]=0;
			//clc.SetItemText(nidx,RCOL_SHORTEXEMPTCODE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.ComplexOrder?'1':'0');
			//clc.SetItemText(nidx,RCOL_COMPLEXORDER,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.ComplexOrderParent?'1':'0');
			//clc.SetItemText(nidx,RCOL_COMPLEXORDERPARENT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.OrderStagingOrder?'1':'0');
			//clc.SetItemText(nidx,RCOL_ORDERSTAGINGORDER,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.OrderStagingParent?'1':'0');
			//clc.SetItemText(nidx,RCOL_ORDERSTAGINGPARENT,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.NotHeldOrder?'1':'0');
			//clc.SetItemText(nidx,RCOL_NOTHELDORDER,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.NotOATSReportable?'1':'0');
			//clc.SetItemText(nidx,RCOL_NOTOATSREPORTABLE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.OVDSpclHndling?'1':'0');
			//clc.SetItemText(nidx,RCOL_OVDSPCLHNDLING,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.TraderGiveUp);
			//clc.SetItemText(nidx,RCOL_TRADERGIVEUP,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.WRKSpclHndling?'1':'0');
			//clc.SetItemText(nidx,RCOL_WRKSPCLHNDLING,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.StrategyName);
			//clc.SetItemText(nidx,RCOL_STRATEGYNAME,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRec.OverrideComms?'1':'0');
			//clc.SetItemText(nidx,RCOL_OVERRIDECOMMS,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%d",btr->TradeRec.CommissionOverrideType);
			//clc.SetItemText(nidx,RCOL_COMMISSIONOVERRIDETYPE,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			_snprintf(vstr,sizeof(vstr)-1,"%.4f",btr->TradeRecB.CommissionOverride);
			//clc.SetItemText(nidx,RCOL_COMMISSIONOVERRIDE,vstr);
			_snprintf(vstr,sizeof(vstr)-1,"%c",btr->TradeRecB.TranTypeCode);
			_snprintf(vstr,sizeof(vstr)-1,"%s",btr->TradeRecB.RediDestination);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

			_snprintf(vstr,sizeof(vstr)-1,"%d",psaveAll->query.nresults+1);
			//clc.SetItemText(nidx,RCOL_POSITION,vstr);
			sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
			//int img=theApp.OrderImage(highMsgType,highExecType,0,0);
			//clc.SetItem(nidx,RCOL_LASTSTATUS,LVIF_IMAGE,0,img,0,0,0,0);
			//clc.SetItem(nidx,0,LVIF_IMAGE,0,-1,0,0,0,0); // Blank out the image in the first column
			strcpy(wptr,"\r\n"); wptr+=2;
			DWORD wbytes=0;
			WriteFile(psaveAll->fhnd,wbuf,(int)(wptr -wbuf),&wbytes,0);
			//psaveAll->cnt++;
			psaveAll->query.nresults++;

			// Aggregates
			if(btr->TradeRec.LastShares>0)
			{
				psaveAll->query.fillQty+=btr->TradeRec.LastShares;
				psaveAll->query.fillSum+=(btr->TradeRec.LastPrice*btr->TradeRec.LastShares);
			}

			if(psaveAll->query.wresults>0) psaveAll->query.wresults--; 
			break;
		}
		// VSDB never really stores the ACCOUNTREC, just the account names
		//case sizeof(ACCOUNTREC):
		//{
		//	_ASSERT(false);//untested
		//	wptr=wbuf;
		//	//clc.SetItemText(nidx,ACOL_HIST,hist?"H":"L");
		//	//_snprintf(vstr,sizeof(vstr)-1,"%d",pstrlen);
		//	//clc.SetItemText(nidx,ACOL_LEN,vstr);
		//	//sprintf(vstr,"%d",/*vpages[curpage].endidx++*/nidx+1);
		//	//clc.SetItemText(nidx,ACOL_POSITION,vstr);

		//	ACCOUNTREC* ac=(ACCOUNTREC*)pstr[i];

		//	_snprintf(vstr,sizeof(vstr)-1,"%s",ac->Account);
		//	//clc.SetItemText(nidx,ACOL_ACCOUNT,vstr);
		//	sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);
		//	_snprintf(vstr,sizeof(vstr)-1,"%s",ac->Domain);
		//	//clc.SetItemText(nidx,ACOL_DOMAIN,vstr);
		//	sprintf(wptr,"%s,",vstr); wptr+=strlen(wptr);

		//	strcpy(wptr,"\r\n"); wptr+=2;
		//	DWORD wbytes=0;
		//	WriteFile(psaveAll->fhnd,wbuf,(int)(wptr -wbuf),&wbytes,0);
		//	//psaveAll->cnt++;
		//	psaveAll->query.nresults++;

		//	if(psaveAll->query.wresults>0) psaveAll->query.wresults--; 
		//	break;
		//}
		}; // End switch
	}
}
void Cvsctest3View::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CListCtrl& clc=GetListCtrl();
	if((pNMItemActivate->iItem<0)||(pNMItemActivate->iItem>=clc.GetItemCount()))
		return;
	if(curq.select=="SUMMARY")
		return;
	// Index query
	if(curq.from=="INDICES")
	{
		CString selname=clc.GetItemText(pNMItemActivate->iItem,IVCOL_KEY);
		// Get more (should we auto-scroll when ... visible?)
		if(selname=="...")
		{
		#ifndef SPLIT_ORD_DET
			clc.DeleteItem(pNMItemActivate->iItem);
			connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.unique,curq.istart,curq.idir,curq.iter);
		#endif
		}
		else
		{
			newq=curq;
			newq.select="*";
			newq.from="ORDERS";
			char wclause[1024]={0};
			strcpy(wclause,newq.where);
			char iname[64]={0};
			char *iptr=strstr(wclause,"IndexName==");
			if(iptr)
			{
				iptr+=12;
				char *eptr=strchr(iptr,'\'');
				if(!eptr) eptr=iptr +strlen(iptr);
				memcpy(iname,iptr,(int)(eptr -iptr));
				iname[(int)(eptr -iptr)]=0;
			}
			char *aptr=strstr(wclause,"AppInstID==");
			if(aptr)
			{
				char *eptr=strchr(aptr+12,'\'');
				if(eptr) eptr++;
				else eptr=aptr +strlen(aptr);
				memcpy(wclause,aptr,(int)(eptr -aptr));
				wclause[(int)(eptr -aptr)]=0;
			}

			if(!_stricmp(iname,"TransactTime"))
			{
				#ifdef TIME_CONVERT
				if(curq.isgmt)
				{
					SYSTEMTIME lst;
					CString stz;
					int wsdate=0,wstime=0,ms=0;
					sscanf((const char*)selname,"%08d-%06d.%03d",&wsdate,&wstime,&ms);
					LONGLONG kval=wsdate; kval*=1000000;
					kval+=wstime; kval*=1000;
					kval+=ms;
					ESTToGMT(kval,lst,stz);
					selname.Format("%04d%02d%02d%02d%02d%02d%03d",
								lst.wYear,lst.wMonth,lst.wDay,
								lst.wHour,lst.wMinute,lst.wSecond,
								lst.wMilliseconds);
				}
				else
				#endif
				{
					// All orders in the minute
					if(selname.Left(3)=="---")
					{
						int tsdate=clc.GetItemData(pNMItemActivate->iItem);
						LONGLONG bmin=tsdate; bmin*=1000000;
						bmin+=atoi((const char*)selname +4)*10000 +atoi((const char*)selname +7)*100;
						struct tm btm;
						memset(&btm,0,sizeof(tm));
						btm.tm_year=tsdate/10000 -1900;
						btm.tm_mon=(tsdate%10000)/100 -1;
						btm.tm_mday=tsdate%100;
						btm.tm_hour=atoi((const char*)selname +4);
						btm.tm_min=atoi((const char*)selname +7);
						btm.tm_isdst=-1;
						time_t btime=mktime(&btm);

						btime+=60;
						btm=*localtime(&btime);
						LONGLONG emin=(btm.tm_year +1900)*10000 +(btm.tm_mon +1)*100 +(btm.tm_mday); emin*=1000000;
						emin+=(btm.tm_hour*10000) +(btm.tm_min*100) +(btm.tm_sec);
						sprintf(wclause +strlen(wclause)," AND %s>=%I64d AND %s<%I64d",iname,bmin,iname,emin);
					}
					else
					{
						//int wsdate=0,wstime=0,ms=0;
						//sscanf((const char*)selname,"%08d-%06d.%03d",&wsdate,&wstime,&ms);
						//selname.Format("%08d%06d%03d",wsdate,wstime,ms);
						int wsdate=0,wstime=0;
						sscanf((const char*)selname,"%08d-%06d",&wsdate,&wstime);
						selname.Format("%08d%06d",wsdate,wstime);
						sprintf(wclause +strlen(wclause)," AND %s=='%s'",iname,selname);
					}
				}
			}
			else if((!_stricmp(iname,"Open Orders"))||(!_stricmp(iname,"Filled Orders")))
			{
				strcpy(iname,"ClOrdID");
				sprintf(wclause +strlen(wclause)," AND %s=='%s'",iname,selname);
			}
			else if(!_stricmp(iname,"OrderDate"))
				sprintf(wclause +strlen(wclause)," AND %s==%s",iname,selname);
			else
				sprintf(wclause +strlen(wclause)," AND %s=='%s'",iname,selname);
			CString dstr=theApp.pMainFrame->GetDateText();
			if((!dstr.IsEmpty())&&(_stricmp(iname,"OrderDate")))
				sprintf(wclause +strlen(wclause)," AND OrderDate==%s",dstr);
			if(!theApp.clientregion.empty())
				sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
			newq.where=wclause;
			newq.unique=false;
			newq.istart=0;
			newq.hist=true;
			newq.live=false;
			newq.iter=0;
			CString asys=clc.GetItemText(pNMItemActivate->iItem,IVCOL_APPSYSTEM);
			if(!asys.IsEmpty())
				newq.asys=asys;

			//theApp.m_pMainWnd->PostMessage(WM_COMMAND,ID_VIEW_NEW,0);
			Cvsctest3Doc *pdoc=GetDocument();
			CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
			CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow(selname,0);
			pdoc->SetDocTemplate(ldt);

			newq.select="";
			newq.from="";
			newq.where="";
		}
	}
#ifdef SPLIT_ORD_DET
	else if(curq.from=="DETAILS")
	{
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
		{
			char *btr=(char*)clc.GetItemData(pNMItemActivate->iItem);
			if(btr)
			{
				if(!theApp.bdlg)
				{
					theApp.bdlg=new CBinaryDlg;
					theApp.bdlg->Create(MAKEINTRESOURCE(IDD_BINARY),this);
				}
				theApp.bdlg->heading.Format("%s_%s",
					clc.GetItemText(pNMItemActivate->iItem,RCOL_DOMAIN),
					clc.GetItemText(pNMItemActivate->iItem,RCOL_ORDERNO));
				theApp.bdlg->qbuf=btr;
				theApp.bdlg->qlen=sizeof(BIGTRADEREC);
				theApp.bdlg->PostMessage(WM_USER +5,0,0);
				theApp.bdlg->ShowWindow(SW_RESTORE);
			}
		}
	}
#else
	// Orders query
	else if(curq.from=="ORDERS")
	{
		CString appinstid=clc.GetItemText(pNMItemActivate->iItem,OCOL_APPINSTID);
		CString selname=clc.GetItemText(pNMItemActivate->iItem,OCOL_CLORDID);
		// Get more (should we auto-scroll when ... visible?)
		if(selname=="...")
		{
			clc.DeleteItem(pNMItemActivate->iItem);
			connNotify->VSCNotifySqlRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		}
		else
		{
			newq=curq;
			newq.select="*";
			newq.from="DETAILS";
			char wclause[1024]={0};
			//strcpy(wclause,newq.where);
			//char *aptr=strstr(wclause,"AppInstID==");
			//if(aptr)
			//{
			//	char *eptr=strchr(aptr+12,'\'');
			//	if(eptr) eptr++;
			//	else eptr=aptr +strlen(aptr);
			//	memcpy(wclause,aptr,(int)(eptr -aptr));
			//	wclause[(int)(eptr -aptr)]=0;
			//}
			//sprintf(wclause +strlen(wclause)," AND ClOrdID=='%s'",selname);
			sprintf(wclause,"AppInstID=='%s' AND ClOrdID=='%s'",appinstid,selname);
			newq.where=wclause;
			newq.unique=false;
			newq.istart=0;
			newq.hist=true;
			newq.live=false;
			newq.iter=0;

			//theApp.m_pMainWnd->PostMessage(WM_COMMAND,ID_VIEW_NEW,0);
			Cvsctest3Doc *pdoc=GetDocument();
			CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
			CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow(selname,0);
			pdoc->SetDocTemplate(ldt);

			newq.select="";
			newq.from="";
			newq.where="";
		}
	}
	else if(curq.from=="DETAILS")
	{
		// Get more (should we auto-scroll when ... visible?)
		CString selname;
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
			selname=clc.GetItemText(pNMItemActivate->iItem,RCOL_ACCOUNT);
		else
			selname=clc.GetItemText(pNMItemActivate->iItem,FCOL_POSITION);
		if(selname=="...")
		{
			clc.DeleteItem(pNMItemActivate->iItem);
			if((curq.asys=="CLSERVER")||(theApp.btrdetails))
				connNotify->VSCNotifyDatRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
			else if((curq.asys=="FIXSERVER")||(curq.asys=="TRADER"))
			{
				connNotify->VSCNotifyDatRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
			}
			else
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		}
		else
		{
		}
	}
#endif
	*pResult = 0;
}

int Cvsctest3View::LaunchFixViewer(const char *fpath)
{
	// Look in C:\Run first
	char vpath[MAX_PATH]={0},parms[1024]={0};
	sprintf(vpath,"C:\\run\\ETT FIX Viewer\\ETT FIX Viewer.exe");
	sprintf(parms,"-p ViewServer -f %s",fpath);
	if(PathFileExists(vpath))
	{
		ShellExecute(0,"open",vpath,parms,0,SW_SHOW);
		return 0;
	}
	// Then check %PATH% environment variable
	char path[1024]={0};
	GetEnvironmentVariable("$PATH$",path,sizeof(path));
	for(const char *dpath=strtok(path,";");dpath;dpath=strtok(0,";"))
	{
		sprintf(vpath,"%s\\ETT FIX Viewer.exe",dpath);
		if(PathFileExists(vpath))
		{
			ShellExecute(m_hWnd,"open",vpath,parms,0,SW_SHOW);
			return 0;
		}
	}
	// Default to notepad
	LaunchTextViewer(fpath);
	return 0;
}
int Cvsctest3View::LaunchTextViewer(const char *fpath)
{
	if(PathFileExists("C:\\Program Files\\TextPad 4\\textpad.exe"))
		ShellExecute(m_hWnd,"open","C:\\Program Files\\TextPad 4\\textpad.exe",fpath,0,SW_SHOW);
	else
		ShellExecute(m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
	return 0;
}
int Cvsctest3View::LaunchBtrViewer(const char *fpath)
{
	// Look in C:\Run first
	char vpath[MAX_PATH]={0};
	sprintf(vpath,"C:\\Run\\TraderLook\\TraderLook.exe");
	if(PathFileExists(vpath))
	{
		ShellExecute(0,"open",vpath,fpath,0,SW_SHOW);
		return 0;
	}
	// Then check %PATH% environment variable
	char path[1024]={0};
	GetEnvironmentVariable("$PATH$",path,sizeof(path));
	for(const char *dpath=strtok(path,";");dpath;dpath=strtok(0,";"))
	{
		sprintf(vpath,"%s\\TraderLook.exe",dpath);
		if(PathFileExists(vpath))
		{
			ShellExecute(m_hWnd,"open",vpath,fpath,0,SW_SHOW);
			return 0;
		}
	}
	// Default to HexPad?
	return 0;
}
void Cvsctest3View::OnSaveAll()
{
	CListCtrl& clc=GetListCtrl();
	for(int i=0;i<clc.GetItemCount();i++)
	{
		if((i==clc.GetItemCount() -1)&&(moreToGet))
			continue;
		clc.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);
	}
	OnSave();
}
void Cvsctest3View::OnSave()
{
	CListCtrl& clc=GetListCtrl();
	char sname[MAX_PATH], spath[MAX_PATH],*sptr;
	sprintf(sname,"%s\\SysmonTempFiles",theApp.pcfg->RunPath.c_str());
	CreateDirectory(sname,0);
	sprintf(sname,"%s\\SysmonTempFiles\\query.txt",theApp.pcfg->RunPath.c_str());
	memset(spath,0,sizeof(spath));
	GetFullPathName(sname,sizeof(spath),spath,&sptr);
	//if(PathFileExists(spath))
	//{
	//	CString wmsg;
	//	wmsg.Format("Overwrite \"%s\"?",spath);
	//	if(MessageBox(wmsg,"Confirm Overwrite",MB_ICONWARNING|MB_YESNO)!= IDYES)
	//		return;
	//}
	HANDLE fhnd=CreateFile(spath,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
	if(fhnd==INVALID_HANDLE_VALUE)
	{
		CString wmsg;
		wmsg.Format("Failed opening \"%s\"!",spath);
		MessageBox(wmsg,"Error",MB_ICONERROR);
		return;
	}	
	savedOnce=true;

	char qbuf[2048]={0};
	DWORD wbytes=0;
	if((curq.from!="DETAILS")||(curq.asys!="CLSERVER"))
	{
		sprintf(qbuf,"select %s from %s where %s\r\n",curq.select,curq.from,curq.where);
		WriteFile(fhnd,qbuf,(int)strlen(qbuf),&wbytes,0);
	}
	// Raw FIX only
	if(curq.from=="INDICES")
	{
		int cnt=0;
		char fbuf[1024]={0};
		POSITION pos=clc.GetFirstSelectedItemPosition();
		while(pos)
		{
			int nItem=clc.GetNextSelectedItem(pos);
			clc.GetItemText(nItem,IVCOL_KEY,fbuf,sizeof(fbuf)-3);
			char *fptr;
			for(fptr=fbuf;*fptr;fptr++)
			{
				if(*fptr=='|') *fptr=0x01;
			}
			strcpy(fptr,"\r\n"); fptr+=2;
			wbytes=0;
			WriteFile(fhnd,fbuf,(int)(fptr -fbuf),&wbytes,0); cnt++;
		}
		CString Tmp;
		Tmp.Format("Total %d items\r\n",cnt);
		DWORD wbytes=0;
		WriteFile(fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
		CloseHandle(fhnd);
		LaunchTextViewer(spath);
	}
	// CSV format
	else if((curq.from=="ORDERS")
		#ifndef LINK_TRADERLOOK
			||((curq.from=="DETAILS")&&((curq.asys=="CLSERVER")||(theApp.btrdetails)))
		#endif
			||((curq.from=="DETAILS")&&(!theApp.allfixtags)&&(!eventlog))
			)
	{
		CHeaderCtrl *chc=clc.GetHeaderCtrl();
		char *qptr=qbuf;
		for(int i=0;i<chc->GetItemCount();i++)
		{
			HDITEM hdi;
			memset(&hdi,0,sizeof(HDITEM));
			char hstr[256];
			hdi.cchTextMax=sizeof(hstr);
			hdi.pszText=hstr;
			hdi.mask=HDI_TEXT;
			chc->GetItem(i,&hdi);
			sprintf(qptr,"%s,",hstr); qptr+=strlen(qptr);
		}
		strcpy(qptr,"\r\n"); qptr+=2;
		wbytes=0;
		WriteFile(fhnd,qbuf,(int)strlen(qbuf),&wbytes,0);

		int cnt=0;
		POSITION pos=clc.GetFirstSelectedItemPosition();
		while(pos)
		{
			CString Tmp;
			int nItem=clc.GetNextSelectedItem(pos);
			for(int i=0;i<chc->GetItemCount();i++)
			{
				CString tbuf=clc.GetItemText(nItem,i);
				// Quote any value that has a comma in it
				if(strchr(tbuf,','))
					Tmp+=(CString)"\""+tbuf+(CString)"\",";
				else
					Tmp+=tbuf+(CString)",";
			}
			Tmp+="\r\n";
			wbytes=0;
			WriteFile(fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0); cnt++;
		}
		CString Tmp;
		Tmp.Format("Total %d items\r\n",cnt);
		DWORD wbytes=0;
		WriteFile(fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
		CloseHandle(fhnd);
		LaunchTextViewer(spath);
	}
	else if(curq.from=="DETAILS")
	{
	#ifdef LINK_TRADERLOOK
		if(curq.asys=="CLSERVER")
		{
			int cnt=0;
			PLACELOGITEM2 pli2;
			memset(&pli2,0,sizeof(PLACELOGITEM2));
			POSITION pos=clc.GetFirstSelectedItemPosition();
			while(pos)
			{
				int nItem=clc.GetNextSelectedItem(pos);
				char *btr=(char*)clc.GetItemData(nItem);
				memcpy(&pli2.BigTradeRec,btr,sizeof(BIGTRADEREC));
				if(btr)
				{
					wbytes=0;
					WriteFile(fhnd,&pli2,sizeof(PLACELOGITEM2),&wbytes,0); cnt++;
				}
			}
			CloseHandle(fhnd);
			LaunchBtrViewer(spath);
		}
		else
	#endif
		{
			int cnt=0;
			char fbuf[2048]={0};
			POSITION pos=clc.GetFirstSelectedItemPosition();
			while(pos)
			{
				int nItem=clc.GetNextSelectedItem(pos);
				clc.GetItemText(nItem,FCOL_FIXMSG,fbuf,sizeof(fbuf)-3);
				char *fptr;
				for(fptr=fbuf;*fptr;fptr++)
				{
					if(*fptr=='|') *fptr=0x01;
				}
				strcpy(fptr,"\r\n"); fptr+=2;
				wbytes=0;
				WriteFile(fhnd,fbuf,(int)(fptr -fbuf),&wbytes,0); cnt++;
			}
			CString Tmp;
			Tmp.Format("Total %d items\r\n",cnt);
			DWORD wbytes=0;
			WriteFile(fhnd,(const char *)Tmp,Tmp.GetLength(),&wbytes,0);
			CloseHandle(fhnd);
			if((curq.asys=="RTOATS")||(curq.asys=="EVENTLOG"))
				LaunchTextViewer(spath);
			else
				LaunchFixViewer(spath);
		}
	}
}
// Queries all results
void Cvsctest3View::OnQueryAll()
{
	if(!moreToGet)
		return;

	char sname[MAX_PATH], spath[MAX_PATH],*sptr;
	sprintf(sname,"%s\\SysmonTempFiles",theApp.pcfg->RunPath.c_str());
	CreateDirectory(sname,0);
	sprintf(sname,"%s\\SysmonTempFiles\\query.txt",theApp.pcfg->RunPath.c_str());
	memset(spath,0,sizeof(spath));
	GetFullPathName(sname,sizeof(spath),spath,&sptr);
	//if(PathFileExists(spath))
	//{
	//	CString wmsg;
	//	wmsg.Format("Overwrite \"%s\"?",spath);
	//	if(MessageBox(wmsg,"Confirm Overwrite",MB_ICONWARNING|MB_YESNO)!= IDYES)
	//		return;
	//}
	HANDLE fhnd=CreateFile(spath,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
	if(fhnd==INVALID_HANDLE_VALUE)
	{
		CString wmsg;
		wmsg.Format("Failed opening \"%s\"!",spath);
		MessageBox(wmsg,"Error",MB_ICONERROR);
		return;
	}	

	char qbuf[1024]={0};
	sprintf(qbuf,"select %s from %s where %s\r\n",curq.select,curq.from,curq.where);
	DWORD wbytes=0;
	WriteFile(fhnd,qbuf,(int)strlen(qbuf),&wbytes,0);

	if((curq.from=="ORDERS")||(curq.asys=="CLSERVER")||(theApp.btrdetails))
	{
		CListCtrl& clc=GetListCtrl();
		CHeaderCtrl *chc=clc.GetHeaderCtrl();
		char *qptr=qbuf;
		int end=OCOL_COUNT;
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
			end=chc->GetItemCount();
		for(int i=0;i<end;i++)
		{
			HDITEM hdi;
			memset(&hdi,0,sizeof(HDITEM));
			char tbuf[256];
			hdi.cchTextMax=sizeof(tbuf);
			hdi.pszText=tbuf;
			hdi.mask=HDI_TEXT;
			chc->GetItem(i,&hdi);
			sprintf(qptr,"%s,",tbuf); qptr+=strlen(qptr);
		}
		strcpy(qptr,"\r\n"); qptr+=2;
		wbytes=0;
		WriteFile(fhnd,qbuf,(int)strlen(qbuf),&wbytes,0);
	}

	//if(iter>0)
	//{
	//	SqlCancel(iter); iter=0;
	//}
	if(psaveAll)
		delete psaveAll;
	psaveAll=new VQSAVEALL;
	psaveAll->query=curq;
	psaveAll->query.rid=0;
	psaveAll->query.iter=0;
	psaveAll->fpath=spath;
	psaveAll->fhnd=fhnd;
	//psaveAll->cnt=0;
	psaveAll->query.nresults=0;
	//psaveAll->ncols=ncols;
	//psaveAll->tcol=tcol;
	//clc.DeleteAllItems();
	//VIEW_SIZE=1000;
	//OnQuery();
	if(curq.from=="INDICES")
		connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.unique,0,curq.idir,0);
	else if((curq.asys=="CLSERVER")||(theApp.btrdetails))
		connNotify->VSCNotifyDatRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,0);
	else
		connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,0);
}
void Cvsctest3View::OnNotepad()
{
	ShellExecute(m_hWnd,"open","notepad.exe","SysmonTempFiles\\query.txt",0,SW_SHOW);
}
void Cvsctest3View::OnTextpad()
{
	LaunchTextViewer("SysmonTempFiles\\query.txt");
}
void Cvsctest3View::OnExcel()
{
	if(PathFileExists("C:\\Program Files\\Microsoft Office\\Office12\\EXCEL.EXE"))
		ShellExecute(m_hWnd,"open","C:\\Program Files\\Microsoft Office\\Office12\\EXCEL.EXE","SysmonTempFiles\\query.txt",0,SW_SHOW);
	else
		ShellExecute(m_hWnd,"open","EXCEL.EXE","SysmonTempFiles\\query.txt",0,SW_SHOW);
}


void Cvsctest3View::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// Add your specialized code here and/or call the base class
	theApp.pMainFrame->SetStatus(1,smsg);

	__super::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void Cvsctest3View::OnQueryDetails()
{
	CListCtrl& clc=GetListCtrl();
	if(curq.from=="ORDERS")
	{
		newq=curq;
		newq.select="*";
		newq.from="DETAILS";
		char wclause[1024]={0};
		strcpy(wclause,newq.where);
		newq.where=wclause;
		newq.unique=false;
		newq.istart=0;
		newq.hist=true;
		#ifdef LIVE_DETAILS
		newq.live=true;
		#else
		newq.live=false;
		#endif
		newq.iter=0;

		Cvsctest3Doc *pdoc=GetDocument();
	#ifdef SPLIT_ORD_DET
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(detDocTemplate);
	#else
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
	#endif
		CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("All Details",0);
		pdoc->SetDocTemplate(ldt);

		newq.select="";
		newq.from="";
		newq.where="";
	}
}

void Cvsctest3View::OnSummarizeIndex()
{
	if(curq.from!="INDICES")
		return;
	// TODO: Analyze indices
}
struct ROLLUP
{
	int icnt;
	int orderQty;
	int cumQty;
	int fillQty;
};
void Cvsctest3View::OnSummarizeOrders()
{
	if(curq.from!="ORDERS")
		return;
	// Load results into new view
	newq=curq;
	newq.select="SUMMARY";
	newq.from="ORDERS";
	newq.where=curq.where;
	newq.unique=false;
	newq.istart=0;
	newq.hist=true;
	newq.live=false;
	newq.iter=0;

	// Analyze orders
	CListCtrl& clc=GetListCtrl();
	list<string> oslist;
	CString sstr;
	sstr.Format("Histogram %d orders",clc.GetItemCount());
	newq.results.push_back((string)(const char *)sstr);
	sstr.Format("FROM %s WHERE %s",curq.from,curq.where);
	newq.results.push_back((string)(const char *)sstr);
	newq.results.push_back("");
	newq.results.push_back("");

	map<string,ROLLUP> appinstset,rootorderset,firstclset;
	CHeaderCtrl *chc=clc.GetHeaderCtrl();
	for(int c=0;c<chc->GetItemCount();c++)
	{
		map<string,int> uset;
		CString low,high;
		for(int r=0;r<clc.GetItemCount();r++)
		{
			//// Ignore cancel orders
			//CString text=clc.GetItemText(r,OCOL_HIGHMSGTYPE);
			//if(text=="F")
			//	continue;
			CString text=clc.GetItemText(r,c);
			// Count unique values
			if(text.IsEmpty())
				continue;
			else if((c==OCOL_CLORDID)&&(text=="..."))
				continue;
			else if((c==OCOL_PRICE)&&(text=="0.00"))
				continue;
			else if(((c==OCOL_ORDERQTY)||(c==OCOL_CUMQTY)||(c==OCOL_FILLQTY))&&(text=="0"))
				continue;
			map<string,int>::iterator uit=uset.find((const char *)text);
			if(uit==uset.end())
				uset.insert(pair<string,int>((const char *)text,1));
			else
				uit->second++;
			// Hi/low
			if(c==OCOL_HIGHMSGTYPE)
			{
				if((low.IsEmpty())||(theApp.GetMsgTypeTier(*text)<theApp.GetMsgTypeTier(*low)))
					low=text;
				if((high.IsEmpty())||(theApp.GetMsgTypeTier(*text)>theApp.GetMsgTypeTier(*high)))
					high=text;
			}
			else if(c==OCOL_HIGHEXECTYPE)
			{
				if((low.IsEmpty())||(theApp.GetExecTypeTier(*text)<theApp.GetExecTypeTier(*low)))
					low=text;
				if((high.IsEmpty())||(theApp.GetExecTypeTier(*text)>theApp.GetExecTypeTier(*high)))
					high=text;
			}
			else
			{
				if((low.IsEmpty())||(text.CompareNoCase(low)<0))
					low=text;
				if((high.IsEmpty())||(text.CompareNoCase(high)>0))
					high=text;
			}
		}
		HDITEM hdi;
		memset(&hdi,0,sizeof(HDITEM));
		hdi.mask=HDI_TEXT;
		char hstr[256]={0};
		hdi.pszText=hstr;
		hdi.cchTextMax=sizeof(hstr);
		chc->GetItem(c,&hdi);
		sstr.Format("%d unique %ss",uset.size(),hstr);
		newq.results.push_back((string)(const char *)sstr);

		CString samples;
		map<string,int>::iterator uit=uset.begin();
		for(;(uit!=uset.end())&&(samples.GetLength()<128);uit++)
		{
			CString text;
			text.Format("%s(%d)",uit->first.c_str(),uit->second);
			if(!samples.IsEmpty())
				samples+=", ";
			samples+=(const char *)text;
		}
		if(uit!=uset.end())
			samples+="...";
		newq.results.push_back((string)(const char *)samples);
		newq.results.push_back((string)(const char *)low);
		newq.results.push_back((string)(const char *)high);

		if(c==OCOL_APPINSTID)
		{
			for(map<string,int>::iterator uit=uset.begin();uit!=uset.end();uit++)
			{
				ROLLUP rup={uit->second,0,0,0};
				appinstset.insert(pair<string,ROLLUP>(uit->first,rup));
			}
		}
		else if(c==OCOL_ROOTORDERID)
		{
			for(map<string,int>::iterator uit=uset.begin();uit!=uset.end();uit++)
			{
				ROLLUP rup={uit->second,0,0,0};
				rootorderset.insert(pair<string,ROLLUP>(uit->first,rup));
			}
		}
		else if(c==OCOL_FIRSTCLORDID)
		{
			for(map<string,int>::iterator uit=uset.begin();uit!=uset.end();uit++)
			{
				ROLLUP rup={uit->second,0,0,0};
				firstclset.insert(pair<string,ROLLUP>(uit->first,rup));
			}
		}
	}
	newq.results.push_back("");
	newq.results.push_back("");
	newq.results.push_back("");
	newq.results.push_back("");

	// Rollup tree
	if(appinstset.empty())
	{
		ROLLUP rup={0,0,0,0};
		appinstset.insert(pair<string,ROLLUP>("*",rup));
	}
	for(map<string,ROLLUP>::iterator ait=appinstset.begin();ait!=appinstset.end();ait++)
	{
		string appinstid=ait->first;
		ROLLUP& airu=ait->second;
		memset(&airu,0,sizeof(ROLLUP));
		int rcnt=0;
		if(rootorderset.empty())
		{
			ROLLUP rup={0,0,0,0};
			rootorderset.insert(pair<string,ROLLUP>("*",rup));
		}
		for(map<string,ROLLUP>::iterator rit=rootorderset.begin();rit!=rootorderset.end();rit++)
		{
			string rootorderid=rit->first;
			ROLLUP& roru=rit->second;
			memset(&roru,0,sizeof(ROLLUP));
			int fcnt=0;
			if(firstclset.empty())
			{
				ROLLUP rup={0,0,0,0};
				firstclset.insert(pair<string,ROLLUP>("*",rup));
			}
			for(map<string,ROLLUP>::iterator fit=firstclset.begin();fit!=firstclset.end();fit++)
			{
				string firstclordid=fit->first;
				ROLLUP& fcru=fit->second;
				memset(&fcru,0,sizeof(ROLLUP));
				int ocnt=0;
				LONGLONG fcruOloc=-1;
				bool fcterm=false;
				// TODO: Nested GetItemText too inefficient!!
				for(int r=0;r<clc.GetItemCount();r++)
				{
					CString text=clc.GetItemText(r,OCOL_APPINSTID);
					if((appinstid!="*")&&(text.CompareNoCase(appinstid.c_str())))
						continue;
					text=clc.GetItemText(r,OCOL_ROOTORDERID);
					if((rootorderid!="*")&&(text.CompareNoCase(rootorderid.c_str())))
						continue;
					text=clc.GetItemText(r,OCOL_FIRSTCLORDID);
					if((firstclordid!="*")&&(text.CompareNoCase(firstclordid.c_str())))
						continue;
					string msgtype=clc.GetItemText(r,OCOL_HIGHMSGTYPE);
					// Ignore rejected or cancel/rejected orders
					string highexectype=clc.GetItemText(r,OCOL_HIGHEXECTYPE);
					if((highexectype=="8")||(highexectype=="9"))
					{
						fcterm=true; continue;
					}
					ocnt++;
					text=clc.GetItemText(r,OCOL_ORDERQTY);
					int ordQty=atoi(text);
					text=clc.GetItemText(r,OCOL_CUMQTY);
					int cumQty=atoi(text);
					text=clc.GetItemText(r,OCOL_FILLQTY);
					int fillQty=atoi(text);
					text=clc.GetItemText(r,OCOL_ORDERLOC);
					LONGLONG oloc=_atoi64(text);
					// CLSERVER-style
					if((curq.asys=="CLSERVER")||(theApp.btrdetails)||(strrcmp(appinstid.c_str(),"_C")))
					{
						if(!fcterm)
							fcru.orderQty=ordQty +fcru.cumQty; // yuk
						fcru.cumQty+=cumQty;
						fcru.fillQty+=fillQty;
						if((highexectype=="4")||(highexectype=="2"))
							fcterm=true;
					}
					// OATS-style
					else if(curq.asys=="RTOATS")
					{
						text=clc.GetItemText(r,OCOL_CLPARENTORDERID);
						// Only count RT quantities
						if(!text.IsEmpty())
						{
							fcru.orderQty+=ordQty;
							fcru.cumQty+=cumQty;
							fcru.fillQty+=fillQty;
						}
					}
					// FIX-style
					else
					{
						// Latest qty in C/R chain
						if(oloc>fcruOloc)
						{
							if(ordQty>0)
								fcru.orderQty=ordQty;
							fcru.cumQty=cumQty;
							fcruOloc=oloc;
						}
						// Fills are not tracked FIX-style due to ViewServer behavior
						fcru.fillQty+=fillQty;
					}
				}
				if(ocnt>0)
				{
					int fperc=fcru.fillQty*100/(fcru.orderQty?fcru.orderQty:1);
					int rperc=fcru.cumQty*100/(fcru.orderQty?fcru.orderQty:1);
					newq.results.push_back("C/R Chain");
					sstr.Format("%s/%s/%s - %d ordered, %d filled (%d%%), %d reported (%d%%)",
						appinstid.c_str(),rootorderid.c_str(),firstclordid.c_str(),
						fcru.orderQty,fcru.fillQty,fperc,fcru.cumQty,rperc);
					newq.results.push_back((string)(const char *)sstr);
					newq.results.push_back("");
					sstr.Format("%d orders",ocnt);
					newq.results.push_back((string)(const char *)sstr);
					roru.orderQty+=fcru.orderQty;
					roru.cumQty+=fcru.cumQty;
					roru.fillQty+=fcru.fillQty;
					fcnt++;
				}
			}
			if(fcnt>0)
			{
				int fperc=roru.fillQty*100/(roru.orderQty?roru.orderQty:1);
				int rperc=roru.cumQty*100/(roru.orderQty?roru.orderQty:1);
				newq.results.push_back("P/C Chain");
				sstr.Format("%s/%s - %d ordered, %d filled (%d%%), %d reported (%d%%)",
					appinstid.c_str(),rootorderid.c_str(),
					roru.orderQty,roru.fillQty,fperc,roru.cumQty,rperc);
				newq.results.push_back((string)(const char *)sstr);
				newq.results.push_back("");
				sstr.Format("%d C/R chains",fcnt);
				newq.results.push_back((string)(const char *)sstr);
				airu.orderQty+=roru.orderQty;
				airu.cumQty+=roru.cumQty;
				airu.fillQty+=roru.fillQty;
				rcnt++;
			}
		}
		if(rcnt>0)
		{
			int fperc=airu.fillQty*100/(airu.orderQty?airu.orderQty:1);
			int rperc=airu.cumQty*100/(airu.orderQty?airu.orderQty:1);
			newq.results.push_back("AppInstance");
			sstr.Format("%s - %d ordered, %d filled (%d%%), %d reported (%d%%)",
				appinstid.c_str(),
				airu.orderQty,airu.fillQty,fperc,airu.cumQty,rperc);
			newq.results.push_back((string)(const char *)sstr);
			newq.results.push_back("");
			sstr.Format("%d P/C chains",rcnt);
			newq.results.push_back((string)(const char *)sstr);
			newq.results.push_back("");
			newq.results.push_back("");
			newq.results.push_back("");
			newq.results.push_back("");
		}
	}

	Cvsctest3Doc *pdoc=GetDocument();
	#ifdef SPLIT_ORD_DET
	CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(detDocTemplate);
	#else
	CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
	#endif
	CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("Summary",0);
	pdoc->SetDocTemplate(ldt);

	newq.select="";
	newq.from="";
	newq.where="";
}
void Cvsctest3View::OnSummarizeDetails()
{
	if(curq.from!="DETAILS")
		return;
	// Load results into new view
	newq=curq;
	newq.select="SUMMARY";
	newq.from="DETAILS";
	newq.where=curq.where;
	newq.unique=false;
	newq.istart=0;
	newq.hist=true;
	newq.live=false;
	newq.iter=0;

	// Analyze details
	CListCtrl& clc=GetListCtrl();
	list<string> oslist;
	CString sstr;
	sstr.Format("%d details",clc.GetItemCount());
	newq.results.push_back((string)(const char *)sstr);
	sstr.Format("FROM %s WHERE %s",curq.from,curq.where);
	newq.results.push_back((string)(const char *)sstr);
	newq.results.push_back("");
	newq.results.push_back("");

	CHeaderCtrl *chc=clc.GetHeaderCtrl();
	for(int c=FCOL_POSITION +1;c<chc->GetItemCount();c++)
	{
		HDITEM hdi;
		memset(&hdi,0,sizeof(HDITEM));
		hdi.mask=HDI_TEXT;
		char hstr[256]={0};
		hdi.pszText=hstr;
		hdi.cchTextMax=sizeof(hstr);
		chc->GetItem(c,&hdi);

		map<string,int> uset;
		CString low,high;
		for(int r=0;r<clc.GetItemCount();r++)
		{
			CString text=clc.GetItemText(r,c);
			// Count unique values
			if(text.IsEmpty())
				continue;
			map<string,int>::iterator uit=uset.find((const char *)text);
			if(uit==uset.end())
				uset.insert(pair<string,int>((const char *)text,1));
			else
				uit->second++;
			// Hi/Low
			if(!text.IsEmpty())
			{
				if(!strcmp(hstr,"35"))
				{
					if((low.IsEmpty())||(theApp.GetMsgTypeTier(*text)<theApp.GetMsgTypeTier(*low)))
						low=text;
					if((high.IsEmpty())||(theApp.GetMsgTypeTier(*text)>theApp.GetMsgTypeTier(*high)))
						high=text;
				}
				else if((!strcmp(hstr,"39"))||(!strcmp(hstr,"150")))
				{
					if((low.IsEmpty())||(theApp.GetExecTypeTier(*text)<theApp.GetExecTypeTier(*low)))
						low=text;
					if((high.IsEmpty())||(theApp.GetExecTypeTier(*text)>theApp.GetExecTypeTier(*high)))
						high=text;
				}
				else if((!strcmp(hstr,"14"))||(!strcmp(hstr,"32"))||(!strcmp(hstr,"38"))||(!strcmp(hstr,"111"))||(!strcmp(hstr,"151")))
				{
					int tval=atoi(text);
					if(tval!=0)
					{
						if((low.IsEmpty())||(tval<atoi(low)))
							low=text;
						if((high.IsEmpty())||(tval>atoi(high)))
							high=text;
					}
				}
				else if((!strcmp(hstr,"6"))||(!strcmp(hstr,"31"))||(!strcmp(hstr,"44"))||(!strcmp(hstr,"99")))
				{
					double tval=atof(text);
					if(tval!=0.0)
					{
						if((low.IsEmpty())||(tval<atoi(low)))
							low=text;
						if((high.IsEmpty())||(tval>atof(high)))
							high=text;
					}
				}
				else
				{
					if((low.IsEmpty())||(text.CompareNoCase(low)<0))
						low=text;
					if((high.IsEmpty())||(text.CompareNoCase(high)>0))
						high=text;
				}
			}
		}
		sstr.Format("%d unique %ss",uset.size(),hstr);
		newq.results.push_back((string)(const char *)sstr);
		CString samples;
		map<string,int>::iterator uit=uset.begin();
		for(;(uit!=uset.end())&&(samples.GetLength()<128);uit++)
		{
			CString text;
			text.Format("%s(%d)",uit->first.c_str(),uit->second);
			if(!samples.IsEmpty())
				samples+=", ";
			samples+=(const char*)text;
		}
		if(uit!=uset.end())
			samples+="...";
		newq.results.push_back((string)(const char *)samples);
		newq.results.push_back((string)(const char *)low);
		newq.results.push_back((string)(const char *)high);
	}

	Cvsctest3Doc *pdoc=GetDocument();
	#ifdef SPLIT_ORD_DET
	CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(detDocTemplate);
	#else
	CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
	#endif
	CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("Summary",0);
	pdoc->SetDocTemplate(ldt);

	newq.select="";
	newq.from="";
	newq.where="";
}

// Gets entire parent/child order chain for selected order
void Cvsctest3View::OnQueryRootOrderID()
{
	CListCtrl& clc=GetListCtrl();
	POSITION pos=clc.GetFirstSelectedItemPosition();
	if(!pos)
		return;
	int sidx=clc.GetNextSelectedItem(pos);
	if((sidx<0)||(sidx>=clc.GetItemCount()))
		return;
	CString rootorderid=clc.GetItemText(sidx,OCOL_ROOTORDERID);
	if(rootorderid.IsEmpty())
		return;
	CString orderdate=clc.GetItemText(sidx,OCOL_ORDERDATE);

	if(curq.from=="ORDERS")
	{
		newq=curq;
		newq.select="*";
		#ifdef SPLIT_ORD_DET
		newq.from="ORDERS/DETAILS";
		#else
		newq.from="ORDERS";
		#endif
		char wclause[1024]={0};
		strcpy(wclause,newq.where);
		char *aptr=strstr(wclause,"AppInstID==");
		if(aptr)
		{
			char *eptr=strchr(aptr+12,'\'');
			if(eptr) eptr++;
			else eptr=aptr +strlen(aptr);
			memcpy(wclause,aptr,(int)(eptr -aptr));
			wclause[(int)(eptr -aptr)]=0;
		}
		sprintf(wclause +strlen(wclause)," AND RootOrderID=='%s'",rootorderid);
		if(!orderdate.IsEmpty())
			sprintf(wclause +strlen(wclause)," AND OrderDate==%s",orderdate);
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		newq.where=wclause;
		newq.unique=false;
		newq.istart=0;
		newq.hist=true;
		#ifdef LIVE_DETAILS
		newq.live=true;
		#else
		newq.live=false;
		#endif
		newq.iter=0;

		Cvsctest3Doc *pdoc=GetDocument();
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
		CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("By RootOrderID",0);
		pdoc->SetDocTemplate(ldt);

		newq.select="";
		newq.from="";
		newq.where="";
	}
}
// Gets entire cancel/replace order chain for selected order
void Cvsctest3View::OnQueryFirstClOrdID()
{
	CListCtrl& clc=GetListCtrl();
	POSITION pos=clc.GetFirstSelectedItemPosition();
	if(!pos)
		return;
	int sidx=clc.GetNextSelectedItem(pos);
	if((sidx<0)||(sidx>=clc.GetItemCount()))
		return;
	//CString appinstid=clc.GetItemText(sidx,OCOL_APPINSTID);
	CString firstclordid=clc.GetItemText(sidx,OCOL_FIRSTCLORDID);
	if(firstclordid.IsEmpty())
		return;
	CString orderdate=clc.GetItemText(sidx,OCOL_ORDERDATE);

	if(curq.from=="ORDERS")
	{
		newq=curq;
		newq.select="*";
		#ifdef SPLIT_ORD_DET
		newq.from="ORDERS/DETAILS";
		#else
		newq.from="ORDERS";
		#endif
		char wclause[1024]={0};
		strcpy(wclause,newq.where);
		//if(appinstid.IsEmpty())
		//{
			char *aptr=strstr(wclause,"AppInstID==");
			if(aptr)
			{
				char *eptr=strchr(aptr+12,'\'');
				if(eptr) eptr++;
				else eptr=aptr +strlen(aptr);
				memcpy(wclause,aptr,(int)(eptr -aptr));
				wclause[(int)(eptr -aptr)]=0;
			}
		//}
		//else
		//	sprintf(wclause,"AppInstID=='%s'",(const char*)appinstid);
		sprintf(wclause +strlen(wclause)," AND FirstClOrdID=='%s'",firstclordid);
		if(!orderdate.IsEmpty())
			sprintf(wclause +strlen(wclause)," AND OrderDate==%s",orderdate);
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		newq.where=wclause;
		newq.unique=false;
		newq.istart=0;
		newq.hist=true;
		#ifdef LIVE_DETAILS
		newq.live=true;
		#else
		newq.live=false;
		#endif
		newq.iter=0;

		Cvsctest3Doc *pdoc=GetDocument();
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
		CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("By FirstClOrdID",0);
		pdoc->SetDocTemplate(ldt);

		newq.select="";
		newq.from="";
		newq.where="";
	}
}

void Cvsctest3View::OnKey()
{
	CListCtrl& clc=GetListCtrl();
	CString key=theApp.pMainFrame->GetKeyText();
	key.MakeUpper();
	// Search the index
	if(curq.from=="INDICES")
	{
		POSITION pos=clc.GetFirstSelectedItemPosition();
		while(pos)
		{
			int sidx=clc.GetNextSelectedItem(pos);
			clc.SetItemState(sidx,0,LVIS_SELECTED);
			// Index search always starts at beginning
			//indexSearchStart=sidx +1;
		}
		// See if the value occurs between visible part of the key
		bool lookForMore=true;
		CString last;
		if(indexSearchStart<0)
			indexSearchStart=0;
		for(int i=indexSearchStart;i<clc.GetItemCount();i++)
		{
			CString text=clc.GetItemText(i,IVCOL_KEY);
			text.MakeUpper();
			// Partial match on the key
			if(text.Find(key)>=0)
			{
				smsg.Format("Matching index found");
				clc.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);
				clc.EnsureVisible(i,false);
				lookForMore=false;
				break;
			}
			// Found where the key should be so stop
			else if(((last.IsEmpty())&&(key.Compare(text)<0))||
					((!last.IsEmpty())&&(last.Compare(text)<0)&&(key.Compare(text)<0)))
			{
				// These keys are not sorted so don't stop
				if((!stristr(curq.where,"ClOrdID"))&&(!stristr(curq.where,"OpenOrders"))&&(!stristr(curq.where,"FilledOrders")))
				{
					smsg.Format("Index not found");
					clc.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);
					clc.EnsureVisible(i,false);
					lookForMore=false;
					break;
				}
			}
			last=text;
		}
		indexSearchStart=-1;
		if((lookForMore)&&(!last.IsEmpty())&&(last.CompareNoCase(key)<0))
		{
			int i=clc.GetItemCount() -1;
			// Autoscroll the index
			if(moreToGet)
			{
				CString selname=clc.GetItemText(i,IVCOL_KEY);
				if(selname=="...")
					clc.DeleteItem(i);
				indexSearchStart=i;
				connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.unique,curq.istart,curq.idir,curq.iter);
				return;
			}
			else
			{
				smsg.Format("Index not found");
				clc.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);
				clc.EnsureVisible(i,false);
			}
		}
		theApp.pMainFrame->SetStatus(1,smsg);
	}
	// Search the pulled results (no autoscroll)
	else// if((curq.from=="ORDERS")||(curq.from=="DETAILS"))
	{
		int start=-1;
		// Unselect everything first
		POSITION pos=clc.GetFirstSelectedItemPosition();
		while(pos)
		{
			int sidx=clc.GetNextSelectedItem(pos);
			clc.SetItemState(sidx,0,LVIS_SELECTED);
			if((!m_findall)&&(start<0))
				start=sidx +1;
		}
		if(start<0)
			start=0;
		CHeaderCtrl *chc=clc.GetHeaderCtrl();
		int nsel=0;
		for(int i=start;i<clc.GetItemCount();i++)
		{
			bool found=false;
			for(int h=0;h<chc->GetItemCount();h++)
			{
				CString text=clc.GetItemText(i,h);
				text.MakeUpper();
				if(text.Find(key)>=0)
				{
					found=true;
					break;
				}
			}
			if(found)
			{
				clc.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED); nsel++;
				clc.EnsureVisible(i,false);
				if(!m_findall)
					break;
			}
		}
		CString tstr=curq.from;
		tstr.MakeLower();
		smsg.Format("Selected %u %s",nsel,tstr);
		theApp.pMainFrame->SetStatus(1,smsg);
	}
}
void Cvsctest3View::OnUpdateKey(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}
// F3 on the list
void Cvsctest3View::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar==VK_F3)
		OnKey();

	__super::OnKeyDown(nChar, nRepCnt, nFlags);

#ifdef SPLIT_ORD_DET
	if(((nChar==VK_UP)||(nChar==VK_DOWN)||(nChar==VK_RETURN)))
	{
		CListCtrl& clc=GetListCtrl();
		POSITION pos=clc.GetFirstSelectedItemPosition();
		if(pos)
		{
			int idx=(int)clc.GetNextSelectedItem(pos);
			if((curq.from=="ORDERS")||(curq.from=="DETAILS"))
			{
				NMITEMACTIVATE ItemActivate;
				memset(&ItemActivate,0,sizeof(NMITEMACTIVATE));
				ItemActivate.iItem=idx;
				LRESULT Result=0;
				OnNMClick((NMHDR *)&ItemActivate,&Result);
			}
			else if(curq.from=="INDICES")
			{
				CString text=clc.GetItemText(idx,IVCOL_KEY);
				NMITEMACTIVATE ItemActivate;
				memset(&ItemActivate,0,sizeof(NMITEMACTIVATE));
				ItemActivate.iItem=idx;
				LRESULT Result=0;
				if(text=="...")
					OnNMClick((NMHDR *)&ItemActivate,&Result);
				else if(nChar==VK_RETURN)
					OnNMDblclk((NMHDR *)&ItemActivate,&Result);
			}
		}
	}
#endif
}
//void Cvsctest3View::OnDate()
//{
//}
void Cvsctest3View::OnUpdateDate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}

// "Find All" disabled for index find
void Cvsctest3View::OnUpdateFindall(CCmdUI *pCmdUI)
{
	if(curq.from=="INDICES")
		pCmdUI->Enable(false);
	else
		pCmdUI->Enable(true);
	pCmdUI->SetCheck(m_findall);
}
void Cvsctest3View::OnFindall()
{
	m_findall=!theApp.pMainFrame->IsFindAll();
}
void Cvsctest3View::OnFixFilter()
{
	CFixFilterDlg ffdlg;
	ffdlg.m_text=theApp.fixfilter.c_str();
	ffdlg.m_alltags=theApp.allfixtags ?BST_CHECKED :BST_UNCHECKED;
	if(ffdlg.DoModal()==IDOK)
	{
		theApp.fixfilter=ffdlg.m_text;
		theApp.allfixtags=ffdlg.m_alltags==BST_CHECKED ?true: false;
		theApp.SaveSetupIni();
		CreateFixColumns();

		curq.wresults=500; curq.nresults=0;
		if(theApp.allfixtags)
			curq.select="*";
		else
			curq.select=theApp.fixfilter.c_str();
		connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
	}
}
int Cvsctest3View::BrowseTimestamp(int dbegin, int tbegin)
{
	CListCtrl& clc=GetListCtrl();
	newq=curq;
	newq.select="*";
	newq.from="INDICES";
	char wclause[1024]={0};
	strcpy(wclause,curq.where);
	char *aptr=strstr(wclause,"AppInstID==");
	if(aptr)
	{
		char *eptr=strchr(aptr+12,'\'');
		if(eptr) eptr++;
		else eptr=aptr +strlen(aptr);
		memcpy(wclause,aptr,(int)(eptr -aptr));
		wclause[(int)(eptr -aptr)]=0;
	}
	sprintf(wclause +strlen(wclause),"AND IndexName=='TransactTime' AND Key>='%08d%06d'",dbegin,tbegin);
	newq.where=wclause;
	newq.unique=true;
	newq.istart=0;
	newq.hist=true;
	newq.live=false;
	newq.iter=0;

	curq=newq;
	curq.wresults=500; curq.nresults=0;
	curq.istart=0;
	curq.iter=0;
	connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.unique,curq.istart,curq.idir,curq.iter);

	// Remove the start key because it messes up the query "..." continue
	char *wptr=strstr(wclause,"AND Key>=");
	if(wptr) *wptr=0;
	curq.where=wclause;

	newq.select="";
	newq.from="";
	newq.where="";
	return 0;
}
void Cvsctest3View::On12AM(){ BrowseTimestamp(tsdate,0); }
void Cvsctest3View::On1AM(){ BrowseTimestamp(tsdate,10000); }
void Cvsctest3View::On2AM(){ BrowseTimestamp(tsdate,20000); }
void Cvsctest3View::On3AM(){ BrowseTimestamp(tsdate,30000); }
void Cvsctest3View::On4AM(){ BrowseTimestamp(tsdate,40000); }
void Cvsctest3View::On5AM(){ BrowseTimestamp(tsdate,50000); }
void Cvsctest3View::On6AM(){ BrowseTimestamp(tsdate,60000); }
void Cvsctest3View::On7AM(){ BrowseTimestamp(tsdate,70000); }
void Cvsctest3View::On8AM(){ BrowseTimestamp(tsdate,80000); }
void Cvsctest3View::On9AM(){ BrowseTimestamp(tsdate,90000); }
void Cvsctest3View::On10AM(){ BrowseTimestamp(tsdate,100000); }
void Cvsctest3View::On11AM(){ BrowseTimestamp(tsdate,110000); }
void Cvsctest3View::On12PM(){ BrowseTimestamp(tsdate,120000); }
void Cvsctest3View::On1PM(){ BrowseTimestamp(tsdate,130000); }
void Cvsctest3View::On2PM(){ BrowseTimestamp(tsdate,140000); }
void Cvsctest3View::On3PM(){ BrowseTimestamp(tsdate,150000); }
void Cvsctest3View::On4PM(){ BrowseTimestamp(tsdate,160000); }
void Cvsctest3View::On5PM(){ BrowseTimestamp(tsdate,170000); }
void Cvsctest3View::On6PM(){ BrowseTimestamp(tsdate,180000); }
void Cvsctest3View::On7PM(){ BrowseTimestamp(tsdate,190000); }
void Cvsctest3View::On8PM(){ BrowseTimestamp(tsdate,200000); }
void Cvsctest3View::On9PM(){ BrowseTimestamp(tsdate,210000); }
void Cvsctest3View::On10PM(){ BrowseTimestamp(tsdate,220000); }
void Cvsctest3View::On11PM(){ BrowseTimestamp(tsdate,230000); }
void Cvsctest3View::OnRemoveOrder()
{
	CListCtrl& clc=GetListCtrl();
	list<int> ilist;
	POSITION pos=clc.GetFirstSelectedItemPosition();
	while(pos)
	{
		int sidx=clc.GetNextSelectedItem(pos);
		if(sidx>=0)
			ilist.push_back(sidx);
	}
	// Delete backwards from the bottom
	for(list<int>::reverse_iterator rit=ilist.rbegin();rit!=ilist.rend();rit++)
	{
		int idx=*rit;
		if(curq.from=="DETAILS")
		{
			if((curq.asys=="CLSERVER")||(curq.asys=="TWIST"))
			{
				double fpx=atof(clc.GetItemText(idx,RCOL_LASTPX));
				int fqty=atoi(clc.GetItemText(idx,RCOL_LASTSHARES));
				if(fqty>0)
				{
					curq.fillQty-=fqty;
					curq.fillSum-=(fpx *fqty);
				}
			}
			//else if(curq.asys=="RTOATS")
			//	;
			//else if((theApp.allfixtags)||(eventlog))
			//	;
			//else
			//	;// TODO: Update total when removing FIX detail
		}
		clc.DeleteItem(idx);
	}
	ilist.clear();

	if(curq.from=="DETAILS")
	{
		smsg.Format("Left %d details, filled %d, avgpx %.4f",
			clc.GetItemCount(),curq.fillQty,curq.fillSum/(curq.fillQty?curq.fillQty:1));
		theApp.pMainFrame->SetStatus(1,smsg);
	}
}
void Cvsctest3View::OnRefreshQuery()
{
	CListCtrl& clc=GetListCtrl();
	clc.DeleteAllItems();
	if(curq.from=="INDICES")
	{
		eventlog=false;
		curq.wresults=500; curq.nresults=0;
		connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.unique,curq.istart,curq.idir,curq.iter);
	}
	else if(curq.from=="ORDERS")
	{
		eventlog=false;
		curq.wresults=500; curq.nresults=0;
		connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
	}
	else if(curq.from=="DETAILS")
	{
		eventlog=false;
		curq.wresults=500; curq.nresults=0;
		curq.fillQty=0;
		curq.fillSum=0.0;
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
			connNotify->VSCNotifyDatRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		else
		{
			if((theApp.allfixtags)||(eventlog))
				curq.select="*";
			else if(curq.asys=="RTOATS")
				curq.select="*";
			else
				curq.select=theApp.fixfilter.c_str();
			connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		}
	}
}
void Cvsctest3View::OnTraceTheseOrders()
{
	if(curq.from=="ORDERS")
	{
		CListCtrl& clc=GetListCtrl();
		CHeaderCtrl *chc=clc.GetHeaderCtrl();
		int ncol=chc->GetItemCount();
		VSDBQUERY *tquery=new VSDBQUERY;
		tquery->asys=curq.asys;
		tquery->select="*";
		tquery->from="LINKTRACEORDERSONLY";
		tquery->where=curq.where;
		char obuf[1024]={0};
		for(int i=0;i<clc.GetItemCount();i++)
		{
			char *optr=obuf;
			const char *clordid=0;
			for(int c=0;c<ncol;c++)
			{
				CString tok=clc.GetItemText(i,c);
				if(c==OCOL_TRANSACTTIME)
				{
					memcpy(optr,(const char*)tok,8);
					strcpy(optr+8,(const char*)tok+9);
					strcat(optr,"\1"); optr+=strlen(optr);
				}
				else if(c==OCOL_CLORDID)
				{
					clordid=optr;
					sprintf(optr,"%s\1",tok); optr+=strlen(optr);
					// Not needed after "Remove positions from this view" menu item
					//// Don't include cldrop overnight positions in view
					//if((curq.asys=="CLSERVER")&&(strchr(tok,'_'))&&(strchr(tok,'-')))
					//{
					//	int odate=atoi(strchr(tok,'_') +1);
					//	if(odate>tquery->traceDate)
					//		tquery->traceDate=odate;
					//}
				}
				else
				{
					sprintf(optr,"%s\1",tok); optr+=strlen(optr);
				}
			}
			if(strncmp(clordid,"...",3))
			{
				string ostr=obuf;
				tquery->results.push_back(ostr); tquery->nresults++;
			}
		}
		connNotify->VSCNotifySqlRequest2((VSCNotify*)this,0,tquery->select,tquery->from,tquery->where,0,true,false,(LONGLONG)tquery,0);
	}
}
void Cvsctest3View::OnTraceOrderAll()
{
	if(curq.from=="ORDERS")
	{
		CListCtrl& clc=GetListCtrl();
		POSITION pos=clc.GetFirstSelectedItemPosition();
		if(pos)
		{
			int sidx=clc.GetNextSelectedItem(pos);
			if(sidx>=0)
			{
				CString clordid=clc.GetItemText(sidx,OCOL_CLORDID);
				CString symbol=clc.GetItemText(sidx,OCOL_SYMBOL);
				CString orderdate=clc.GetItemText(sidx,OCOL_ORDERDATE);
				CString wclause;
				if(!orderdate.IsEmpty())
					wclause.Format("ClOrdID=='%s' AND Symbol=='%s' AND OrderDate==%s",clordid,symbol,orderdate);
				else
					wclause.Format("ClOrdID=='%s' AND Symbol=='%s'",clordid,symbol);
				//if(!theApp.clientregion.empty())
				//	sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
				connNotify->VSCNotifySqlRequest2((VSCNotify*)this,0,"*","LINKTRACEORDERS",wclause,0,true,false,0,0);
			}
		}
	}
	else if(curq.from=="INDICES")
	{
		CListCtrl& clc=GetListCtrl();
		POSITION pos=clc.GetFirstSelectedItemPosition();
		if(pos)
		{
			int sidx=clc.GetNextSelectedItem(pos);
			if(sidx>=0)
			{
				CString key=clc.GetItemText(sidx,IVCOL_KEY);
				char wclause[1024]={0};
				strcpy(wclause,curq.where);
				char iname[64]={0};
				char *iptr=strstr(wclause,"IndexName==");
				if(iptr)
				{
					iptr+=12;
					char *eptr=strchr(iptr,'\'');
					if(!eptr) eptr=iptr +strlen(iptr);
					memcpy(iname,iptr,(int)(eptr -iptr));
					iname[(int)(eptr -iptr)]=0;
				}
				sprintf(wclause,"%s=='%s'",iname,key);
				connNotify->VSCNotifySqlRequest2((VSCNotify*)this,0,"*","LINKTRACEORDERS",wclause,0,true,false,0,0);
			}
		}
	}
}
void Cvsctest3View::OnHidePositions()
{
	if(((curq.asys!="CLSERVER")&&(curq.asys!="TWIST"))||(curq.from!="ORDERS"))
		return;
	CListCtrl& clc=GetListCtrl();
	for(int i=clc.GetItemCount()-1;i>=0;i--)
	{
		if((clc.GetItemText(i,OCOL_HIGHMSGTYPE).IsEmpty())&&
			(clc.GetItemText(i,OCOL_HIGHEXECTYPE)=="B")&&
			(clc.GetItemText(i,OCOL_FILLQTY)=="0"))
			clc.DeleteItem(i);
	}
	smsg.Format("Left %u orders",clc.GetItemCount());
	theApp.pMainFrame->SetStatus(1,smsg);
}
#ifdef SPLIT_ORD_DET
void Cvsctest3View::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	CListCtrl& clc=GetListCtrl();
	if((pNMItemActivate->iItem<0)||(pNMItemActivate->iItem>=clc.GetItemCount()))
		return;
	if(curq.from=="INDICES")
	{
		CString selname=clc.GetItemText(pNMItemActivate->iItem,IVCOL_KEY);
		// One-click query continue
		if(selname=="...")
		{
			clc.DeleteItem(pNMItemActivate->iItem);
			connNotify->VSCNotifySqlIndexRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.unique,curq.istart,curq.idir,curq.iter);
		}
		else if(strstr(curq.where,"=='OrderDate'"))
		{
			theApp.pMainFrame->SetDateText(selname);
		}
	}
	else if((curq.from=="ORDERS")&&(curq.select!="SUMMARY"))
	{
		CString appinstid=clc.GetItemText(pNMItemActivate->iItem,OCOL_APPINSTID);
		CString selname=clc.GetItemText(pNMItemActivate->iItem,OCOL_CLORDID);
		CString orderdate=clc.GetItemText(pNMItemActivate->iItem,OCOL_ORDERDATE);
		// One-click query continue
		if(selname=="...")
		{
			clc.DeleteItem(pNMItemActivate->iItem);
			connNotify->VSCNotifySqlRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		}
		// One-click detail query on orders
		else
		{
			_ASSERT(ordDetView);
			// Set the query for the attached detail view
			ordDetView->curq=curq;
			ordDetView->curq.select="*";
			ordDetView->curq.from="DETAILS";
			char wclause[1024]={0};
			sprintf(wclause,"AppInstID=='%s' AND ClOrdID=='%s'",appinstid,selname);
			if(!orderdate.IsEmpty())
				sprintf(wclause +strlen(wclause)," AND OrderDate==%s",orderdate);
			if(!theApp.clientregion.empty())
				sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
			ordDetView->curq.where=wclause;
			ordDetView->curq.unique=false;
			ordDetView->curq.istart=0;
			ordDetView->curq.hist=true;
			#ifdef LIVE_DETAILS
			ordDetView->curq.live=true;
			#else
			ordDetView->curq.live=false;
			#endif
			ordDetView->curq.iter=0;

			ordDetView->curq.wresults=500; ordDetView->curq.nresults=0;
			if((ordDetView->curq.asys=="CLSERVER")||(theApp.btrdetails))
				ordDetView->connNotify->VSCNotifyDatRequest((VSCNotify*)this,0,ordDetView->curq.select,ordDetView->curq.from,ordDetView->curq.where,ordDetView->curq.maxorders,ordDetView->curq.hist,ordDetView->curq.live,ordDetView->curq.iter);
			else
			{
				if((theApp.allfixtags)||(eventlog))
					ordDetView->curq.select="*";
				else if(curq.asys=="RTOATS")
					ordDetView->curq.select="*";
				else
					ordDetView->curq.select=theApp.fixfilter.c_str();
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,0,ordDetView->curq.select,ordDetView->curq.from,ordDetView->curq.where,ordDetView->curq.maxorders,ordDetView->curq.hist,ordDetView->curq.live,ordDetView->curq.iter);
			}
		}
	}
	else if(curq.from=="DETAILS")
	{
		CString selname;
		if((curq.asys=="CLSERVER")||(theApp.btrdetails))
			selname=clc.GetItemText(pNMItemActivate->iItem,RCOL_ACCOUNT);
		else
			selname=clc.GetItemText(pNMItemActivate->iItem,FCOL_POSITION);
		// One-click query continue
		if(selname=="...")
		{
			clc.DeleteItem(pNMItemActivate->iItem);
			if((curq.asys=="CLSERVER")||(theApp.btrdetails))
				connNotify->VSCNotifyDatRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
			else if((curq.asys=="FIXSERVER")||(curq.asys=="TRADER"))
			{
				connNotify->VSCNotifyDatRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
			}
			else
				connNotify->VSCNotifySqlRequest((VSCNotify*)this,curq.rid,curq.select,curq.from,curq.where,curq.maxorders,curq.hist,curq.live,curq.iter);
		}
		else
		{
			if((curq.asys=="CLSERVER")||(theApp.btrdetails))
			{
				char *btr=(char*)clc.GetItemData(pNMItemActivate->iItem);
				if((btr)&&(theApp.bdlg)&&(theApp.bdlg->IsWindowVisible()))
				{
					theApp.bdlg->heading.Format("%s_%s",
						clc.GetItemText(pNMItemActivate->iItem,RCOL_DOMAIN),
						clc.GetItemText(pNMItemActivate->iItem,RCOL_ORDERNO));
					theApp.bdlg->qbuf=btr;
					theApp.bdlg->qlen=sizeof(BIGTRADEREC);
					theApp.bdlg->PostMessage(WM_USER +5,0,0);
				}
			}
		}
	}
	*pResult = 0;
}
#endif

