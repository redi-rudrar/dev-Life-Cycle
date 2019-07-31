// TraceView.cpp : implementation file
//

#include "stdafx.h"
#include "vsctest3.h"
#include "TraceView.h"
#ifndef SHARED_HANDLERS
#include "vsctest3.h"
#endif
#include "vsctest3Doc.h"
#include "MainFrm.h"
#include "vsctest3View.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "GroupOrderDlg.h"

typedef list<class TRACEORDER *> ORDERLIST;
class TraceApp
{
public:
	TraceApp(const string& asys);
	~TraceApp();

	string asys;
	TRACEORDER **sorders;
	int totcnt;
	int LoadOrders(VSDBQUERY *tquery);
};

class TRACEORDER
{
public:
	TRACEORDER()
		:str(0)
		,ainst(0)
		,clordid(0)
		,rootorderid(0)
		,firstclordid(0)
		,symbol(0)
		,account(0)
		,ecnordid(0)
		,clientid(0)
		,price(0)
		,side(0)
		,orderqty(0)
		,cumqty(0)
		,fillqty(0)
		,term(0)
		,highmsgtype(0)
		,highexectype(0)
		,transacttime(0)
		,orderloc(0)
		,connection(0)
		,orderdate(0)
		,clparentorderid(0)
		,routedorderid(0)
		// C/R chain
		,crnext(0)
		,crprev(0)
		,rtnext(0)
		,rtprevList()
		,vsoats(0)
		// P/C chain
		,parent(0)
		,childList()
	{
		memset(asys,0,sizeof(asys));
	}

	char asys[32];
	char *str;
	const char *ainst;
	const char *clordid;
	const char *rootorderid;
	const char *firstclordid;
	const char *symbol;
	const char *account;
	const char *ecnordid;
	const char *clientid;
	const char *price;
	const char *side;
	const char *orderqty;
	const char *cumqty;
	const char *fillqty;
	const char *term;
	const char *highmsgtype;
	const char *highexectype;
	const char *transacttime;
	const char *orderloc;
	const char *connection;
	const char *orderdate;
	const char *clparentorderid;
	const char *routedorderid;
	const char *securitytype;
	// C/R chain
	TRACEORDER *crnext;
	TRACEORDER *crprev;
	TRACEORDER *rtnext;
	ORDERLIST rtprevList;
	TRACEORDER *vsoats;
	// P/C chain
	TRACEORDER *parent;
	ORDERLIST childList;
};

// CTraceView
list<VSDBQUERY *> CTraceView::ftqlist;
VSDBQUERY CTraceView::newq;

IMPLEMENT_DYNCREATE(CTraceView, CTreeView)

CTraceView::CTraceView()
	:curq()
	,alist()
	,rselitem(0)
	//,connNotify(0)
	,vDocTemplate(0)
	,autoRClick(false)
{
}

CTraceView::~CTraceView()
{
}

BEGIN_MESSAGE_MAP(CTraceView, CTreeView)
	ON_NOTIFY_REFLECT(NM_RCLICK, &CTraceView::OnNMRClick)
	ON_COMMAND(ID_FIRST_CRCHAIN +0, &CTraceView::OnQueryCRChain1)
	ON_COMMAND(ID_FIRST_CRCHAIN +1, &CTraceView::OnQueryCRChain2)
	ON_COMMAND(ID_FIRST_CRCHAIN +2, &CTraceView::OnQueryCRChain3)
	ON_COMMAND(ID_FIRST_CRCHAIN +3, &CTraceView::OnQueryCRChain4)
	ON_COMMAND(ID_FIRST_CRCHAIN +4, &CTraceView::OnQueryCRChain5)
	ON_COMMAND(ID_FIRST_CRCHAIN +5, &CTraceView::OnQueryCRChain6)
	ON_COMMAND(ID_FIRST_CRCHAIN +6, &CTraceView::OnQueryCRChain7)
	ON_COMMAND(ID_FIRST_CRCHAIN +7, &CTraceView::OnQueryCRChain8)
	ON_COMMAND(ID_FIRST_CRCHAIN +8, &CTraceView::OnQueryCRChain9)
	ON_COMMAND(ID_FIRST_CRCHAIN +9, &CTraceView::OnQueryCRChain10)
	ON_COMMAND(ID_FIRST_PCCHAIN +0, &CTraceView::OnQueryPCChain1)
	ON_COMMAND(ID_FIRST_PCCHAIN +1, &CTraceView::OnQueryPCChain2)
	ON_COMMAND(ID_FIRST_PCCHAIN +2, &CTraceView::OnQueryPCChain3)
	ON_COMMAND(ID_FIRST_PCCHAIN +3, &CTraceView::OnQueryPCChain4)
	ON_COMMAND(ID_FIRST_PCCHAIN +4, &CTraceView::OnQueryPCChain5)
	ON_COMMAND(ID_FIRST_PCCHAIN +5, &CTraceView::OnQueryPCChain6)
	ON_COMMAND(ID_FIRST_PCCHAIN +6, &CTraceView::OnQueryPCChain7)
	ON_COMMAND(ID_FIRST_PCCHAIN +7, &CTraceView::OnQueryPCChain8)
	ON_COMMAND(ID_FIRST_PCCHAIN +8, &CTraceView::OnQueryPCChain9)
	ON_COMMAND(ID_FIRST_PCCHAIN +9, &CTraceView::OnQueryPCChain10)
	ON_COMMAND(ID_FIRST_ORDER +0, &CTraceView::OnQueryOrder1)
	ON_COMMAND(ID_FIRST_ORDER +1, &CTraceView::OnQueryOrder2)
	ON_COMMAND(ID_FIRST_ORDER +2, &CTraceView::OnQueryOrder3)
	ON_COMMAND(ID_FIRST_ORDER +3, &CTraceView::OnQueryOrder4)
	ON_COMMAND(ID_FIRST_ORDER +4, &CTraceView::OnQueryOrder5)
	ON_COMMAND(ID_FIRST_ORDER +5, &CTraceView::OnQueryOrder6)
	ON_COMMAND(ID_FIRST_ORDER +6, &CTraceView::OnQueryOrder7)
	ON_COMMAND(ID_FIRST_ORDER +7, &CTraceView::OnQueryOrder8)
	ON_COMMAND(ID_FIRST_ORDER +8, &CTraceView::OnQueryOrder9)
	ON_COMMAND(ID_FIRST_ORDER +9, &CTraceView::OnQueryOrder10)
	ON_WM_DESTROY()
	ON_COMMAND(ID_EXPAND_ALL, &CTraceView::OnExpandAll)
	ON_COMMAND(ID_COLLAPSE_ALL, &CTraceView::OnCollapseAll)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CTraceView::OnNMDblclk)
	ON_COMMAND(ID_KEY, &CTraceView::OnKey)
	ON_UPDATE_COMMAND_UI(ID_KEY, &CTraceView::OnUpdateKey)
	ON_WM_KEYDOWN()
	//ON_COMMAND(ID_GROUP_APPINSTID, &CTraceView::OnGroupAppInstID)
	//ON_COMMAND(ID_GROUP_SIDE, &CTraceView::OnGroupSide)
	//ON_COMMAND(ID_GROUP_PRICE, &CTraceView::OnGroupPrice)
	ON_COMMAND(ID_FIRST_GROUP +0, &CTraceView::OnGroup1)
	ON_COMMAND(ID_FIRST_GROUP +1, &CTraceView::OnGroup2)
	ON_COMMAND(ID_FIRST_GROUP +2, &CTraceView::OnGroup3)
	ON_COMMAND(ID_FIRST_GROUP +3, &CTraceView::OnGroup4)
	ON_COMMAND(ID_FIRST_GROUP +4, &CTraceView::OnGroup5)
	ON_COMMAND(ID_FIRST_GROUP +5, &CTraceView::OnGroup6)
	ON_COMMAND(ID_FIRST_GROUP +6, &CTraceView::OnGroup7)
	ON_COMMAND(ID_FIRST_GROUP +7, &CTraceView::OnGroup8)
	ON_COMMAND(ID_FIRST_GROUP +8, &CTraceView::OnGroup9)
	ON_COMMAND(ID_FIRST_GROUP +9, &CTraceView::OnGroup10)
	ON_COMMAND(ID_FIRST_GROUP +10, &CTraceView::OnGroup11)
	ON_COMMAND(ID_FIRST_GROUP +11, &CTraceView::OnGroup12)
	ON_COMMAND(ID_FIRST_GROUP +12, &CTraceView::OnGroup13)
	ON_COMMAND(ID_FIRST_GROUP +13, &CTraceView::OnGroup14)
	ON_COMMAND(ID_FIRST_GROUP +14, &CTraceView::OnGroup15)
	ON_COMMAND(ID_FIRST_GROUP +15, &CTraceView::OnGroup16)
	ON_COMMAND(ID_FIRST_GROUP +16, &CTraceView::OnGroup17)
	ON_COMMAND(ID_FIRST_GROUP +17, &CTraceView::OnGroup18)
	ON_COMMAND(ID_FIRST_GROUP +18, &CTraceView::OnGroup19)
	ON_COMMAND(ID_FIRST_GROUP +19, &CTraceView::OnGroup20)
	ON_COMMAND(ID_FIRST_GROUP +20, &CTraceView::OnGroup21)
	ON_COMMAND(ID_FIRST_GROUP +21, &CTraceView::OnGroup22)
	ON_COMMAND(ID_FIRST_GROUP +22, &CTraceView::OnGroup23)
	ON_COMMAND(ID_GROUP_NONE, &CTraceView::OnGroupNone)
	ON_COMMAND(ID_GROUP_REORDER, &CTraceView::OnGroupReorder)
	ON_COMMAND(ID_REMOVE_FROM_TRACE, &CTraceView::OnRemoveFromTrace)
	ON_COMMAND(ID_SAVE_TRACE, &CTraceView::OnSaveTrace)
	ON_COMMAND(ID_SAVE_TRACE_ALL, &CTraceView::OnSaveTraceAll)
	ON_COMMAND(ID_TRACE_TO_ORDERS_VIEW, &CTraceView::OnTraceToOrdersView)
	#ifdef OWNER_PAINT
	ON_WM_PAINT()
	#endif
END_MESSAGE_MAP()


// CTraceView diagnostics

#ifdef _DEBUG
void CTraceView::AssertValid() const
{
	CTreeView::AssertValid();
}

#ifndef _WIN32_WCE
void CTraceView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif
#endif //_DEBUG

// CTraceView message handlers
void CTraceView::OnInitialUpdate()
{
	__super::OnInitialUpdate();

	// Add your specialized code here and/or call the base class
	CTreeCtrl& ctc=GetTreeCtrl();
	ctc.ModifyStyle(0,TVS_HASLINES);
	ctc.ModifyStyle(0,TVS_LINESATROOT);
	ctc.ModifyStyle(0,TVS_HASBUTTONS);
	ctc.SetImageList(&theApp.ilist,TVSIL_NORMAL);

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
	const char *tsys="ALL";
	if(ftqlist.size()==1)
		tsys=ftqlist.front()->asys;
	CString title;
	title.Format("Trace(%s*) %s",tsys,curq.where);
	curq.asys=tsys;
	GetParent()->SetWindowText(title);
	LoadTraceResults();
}
void CTraceView::OnDestroy()
{
	// Add your message handler code here
	for(list<TraceApp*>::iterator ait=alist.begin();ait!=alist.end();ait++)
		delete *ait;
	alist.clear();

	CTreeView::OnDestroy();
}
//// VSCNotify
//void CTraceView::VSCNotifyError(int rc, const char *emsg)
//{
//}
//void CTraceView::VSCNotifyEvent(int rc, const char *emsg)
//{
//}
//// For EncodeLoginRequest
//void CTraceView::VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
//{
//}
//void CTraceView::VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
//{
//}
//// For EncodeSqlRequest
//void CTraceView::VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
//{
//}
//
//void CTraceView::VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
//{
//}
//void CTraceView::VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
//{
//}
//void CTraceView::VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
//{
//}
//void CTraceView::VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
//{
//}
//// For EncodeCancelRequest
//void CTraceView::VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
//{
//}
//// For EncodeDescribeRequest
//void CTraceView::VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
//{
//}
//void CTraceView::VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
//{
//}
//
//// For EncodeLoginRequest2
//void CTraceView::VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
//{
//}
//void CTraceView::VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
//{
//}
//// For EncodeSqlRequest2
//void CTraceView::VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
//{
//}
//// For EncodeSqlIndexRequest2
//void CTraceView::VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
//{
//}
//
//void CTraceView::VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
//{
//}
//void CTraceView::VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
//{
//}

static CString DetailedNodeDisplay(TRACEORDER *torder, bool aggfill)
{
	CString display;
	CString mtstr;
	if(!strcmp(torder->highmsgtype,"D")) 
	{
		if(!strcmp(torder->asys,"RTOATS"))
		{
			if(*torder->rootorderid) mtstr="RT"; 
			else mtstr="NW";
		}
		else 
			mtstr="NW";
	}
	else if(!strcmp(torder->highmsgtype,"G")) mtstr="CR";
	else if(!strcmp(torder->highmsgtype,"F")) mtstr="CL";
	CString sstr;
	if(!strcmp(torder->side,"1")) sstr="B";
	else if(!strcmp(torder->side,"2")) sstr="SL";
	else if(!strcmp(torder->side,"5")) sstr="SS";
	else if(!strcmp(torder->side,"6")) sstr="SX";
	else sstr=torder->side;
	char price[32]={0};
	strcpy(price,torder->price);
	for(char *pptr=price +strlen(price) -1;
		(pptr>price)&&(*pptr=='0')&&(pptr[-2]!='.');
		pptr--)
		*pptr=0;
	if(!strcmp(price,"0.00"))
		strcpy(price,"MKT");
	CString pstr;
	//if(mtstr!="CL")
		pstr.Format(" %s x %s",price,torder->orderqty);
	CString fstr;
	int fillqty=0;
	if(aggfill)
	{
		for(TRACEORDER *norder=torder;norder;norder=norder->crnext)
		{
			if(strcmp(norder->fillqty,"0"))
				fillqty+=atoi(norder->fillqty);
			if(norder->crnext==norder)
			{
				_ASSERT(false); break;
			}
		}
	}
	else
		fillqty=atoi(torder->fillqty);
	if(fillqty>0)
		fstr.Format(" [F%d]",fillqty);
	char tstr[32]={0};
	if(torder->transacttime[0])
	{
		//sprintf(tstr,"%s-%s",torder->orderdate,torder->transacttime +8);
		strncpy(tstr,torder->transacttime,8); 
		if(torder->transacttime[8])
		{
			tstr[8]='-'; strncpy(tstr+9,torder->transacttime +8,6);
		}
	}
	else
		sprintf(tstr,"%s",torder->orderdate);
	display.Format("%s: %s %s %s%s%s | %s (%s.%s) %s",
		tstr,mtstr,sstr,torder->symbol,pstr,fstr,
		torder->clordid,torder->asys,torder->ainst,torder->routedorderid);
	return display;
}
static CString ShortNodeDisplay(TRACEORDER *torder)
{
	CString display;
	display.Format("%s (%s.%s) %s",torder->clordid,torder->asys,torder->ainst,torder->routedorderid);
	return display;
}
//static int _SortByRootOrderID(const void *e1, const void *e2)
//{
//	TRACEORDER *to1=*(TRACEORDER **)e1;
//	TRACEORDER *to2=*(TRACEORDER **)e2;
//	const char *r1=to1->rootorderid;
//	if(!*r1) r1=to1->clparentorderid;
//	if(!*r1) r1=to1->clordid;
//	const char *r2=to2->rootorderid;
//	if(!*r2) r2=to2->clparentorderid;
//	if(!*r2) r2=to2->clordid;
//	int cmp=strcmp(r1,r2);
//	if(cmp) return cmp;
//	cmp=strcmp(to1->transacttime,to2->transacttime);
//	if(cmp) return cmp;
//	return _atoi64(to1->orderloc) -_atoi64(to2->orderloc);
//}
static int _SortByFirstClOrdID(const void *e1, const void *e2)
{
	TRACEORDER *to1=*(TRACEORDER **)e1;
	TRACEORDER *to2=*(TRACEORDER **)e2;
	int cmp=strcmp(to1->firstclordid,to2->firstclordid);
	if(cmp) return cmp;
	cmp=strcmp(to1->ainst,to2->ainst);
	if(cmp) return cmp;
	cmp=strcmp(to1->transacttime,to2->transacttime);
	if(cmp) return cmp;
	__int64 l1=_atoi64(to1->orderloc);
	__int64 l2=_atoi64(to2->orderloc);
	if(l1>l2) return +1;
	else if(l1<l2) return -1;
	return 0;
}
static int _SortByTransactTime(const void *e1, const void *e2)
{
	TRACEORDER *to1=*(TRACEORDER **)e1;
	TRACEORDER *to2=*(TRACEORDER **)e2;
	int cmp=strcmp(to1->transacttime,to2->transacttime);
	if(cmp) return cmp;
	__int64 l1=_atoi64(to1->orderloc);
	__int64 l2=_atoi64(to2->orderloc);
	if(l1>l2) return +1;
	else if(l1<l2) return -1;
	return 0;
}
static int BreakTreeCycles(set<TRACEORDER*>& tset, TRACEORDER *torder)
{
	if(tset.find(torder)!=tset.end())
	{
		_ASSERT(false);
		return -1;
	}
	tset.insert(torder);

	// The same order can be routed to by more than one order, 
	// but on a single order's route path an order cannot occur more than once.
	set<TRACEORDER*> rtset;
	for(TRACEORDER *norder=torder;norder;)
	{
		if(rtset.find(norder->rtnext)!=rtset.end())
		{
			_ASSERT(false);
			TRACEORDER *porder=norder;
			norder=norder->rtnext;
			porder->rtnext=0;
		}
		else
		{
			rtset.insert(norder);
			norder=norder->rtnext;
		}
	}
	if(torder->vsoats)
	{
		if(rtset.find(torder->vsoats)!=rtset.end())
		{
			_ASSERT(false);
			torder->vsoats=0;
		}
		else
			rtset.insert(torder->vsoats);
	}
	for(set<TRACEORDER*>::iterator rit=rtset.begin();rit!=rtset.end();rit++)
		tset.insert(*rit);

	if(torder->crnext)
	{
		for(TRACEORDER *norder=torder;norder;)
		{
			if(tset.find(norder->crnext)!=tset.end())
			{
				_ASSERT(false);
				TRACEORDER *porder=norder;
				norder=norder->crnext;
				porder->crnext=0;
			}
			else
			{
				// Let the recursion on torder->crnext insert the nodes
				//tset.insert(norder);
				norder=norder->crnext;
			}
		}
		BreakTreeCycles(tset,torder->crnext);
	}

	for(ORDERLIST::iterator cit=torder->childList.begin();cit!=torder->childList.end();)
	{
		TRACEORDER *corder=*cit;
		if(tset.find(corder)!=tset.end())
		{
			_ASSERT(false);
			cit=torder->childList.erase(cit);
			BreakTreeCycles(tset,corder);
		}
		else
		{
			BreakTreeCycles(tset,corder);
			cit++;
		}
	}
	return 0;
}
int CTraceView::MatchOrders(ORDERLIST& plist, list<TraceApp*>& alist)
{
	int totcnt=0;
	for(list<TraceApp*>::iterator ait=alist.begin();ait!=alist.end();ait++)
		totcnt+=(*ait)->totcnt;
	TRACEORDER **sorders=new TRACEORDER *[totcnt];
	int i=0;
	for(list<TraceApp*>::iterator ait=alist.begin();ait!=alist.end();ait++)
	{
		for(int j=0;j<(*ait)->totcnt;j++)
		{
			TRACEORDER *torder=(*ait)->sorders[j];
			sorders[i]=torder; i++;
		}
	}

	// Match orders routed across instances
	for(i=0;i<totcnt;i++)
	{
		TRACEORDER *torder=sorders[i];
		for(int j=0;j<totcnt;j++)
		{
			TRACEORDER *norder=sorders[j];
			if(j==i) 
				continue;
			if(strcmp(norder->orderdate,torder->orderdate))
				continue;
			// Our purposes may be better served performing multiple searches to
			// give match priority to internal instances (i.e. FIXSERVER to FIXSERVER_C)
			//else if(!norder->rtprevList.empty()) // already matched in rt chain
			//	continue;
			if((!strcmp(torder->asys,"FIXSERVER"))&&(!strcmp(norder->asys,"FIXSERVER")))
			{
				if((!strrcmp(torder->ainst,"_C"))&&(strrcmp(norder->ainst,"_C")))
				{
					if(!strcmp(norder->clordid,torder->ecnordid))
					{
						torder->rtnext=norder; norder->rtprevList.push_back(torder); 
					}
				}
			}
			else if((!strcmp(torder->asys,"FIXSERVER"))&&(!strcmp(norder->asys,"CLSERVER")))
			{
				if(!strcmp(norder->clordid,torder->ecnordid))
				{
					torder->rtnext=norder; norder->rtprevList.push_back(torder); 
				}
			}
			else if((!strcmp(torder->asys,"FIXSERVER"))&&(!strcmp(norder->asys,"RTOATS")))
			{
				// NYSE CCG matching
				char ecnordid[32]={0};
				strcpy(ecnordid,norder->ecnordid);
				if(strchr(norder->ecnordid,'/'))
				{
					char *cptr=strchr(ecnordid,'/'); *cptr=0;
					cptr=strchr(ecnordid,'_');
					if((cptr)&&(cptr>ecnordid))
					{
						memmove(ecnordid+1,ecnordid,(int)(cptr -ecnordid));
						*ecnordid='_';
					}
				}
				if((!strcmp(ecnordid,torder->routedorderid))|| // child report
				   (!strcmp(norder->clordid,torder->clordid))) // parent report
				{
					torder->vsoats=norder; norder->rtprevList.push_back(torder); 
				}
			}
			else if((!strcmp(torder->asys,"CLSERVER"))&&(!strcmp(norder->asys,"TRADER")))
			{
				// BTR match
				if(strrcmp(norder->ainst,"_C"))
				{
					if(!strcmp(norder->clordid,torder->clordid))
					{
						torder->rtnext=norder; norder->rtprevList.push_back(torder); 
						// Breaking here allows a FIX match to be overridden by a BTR match,
						// but a BTR match will not be overriden by a FIX match
						break;
					}
				}
				// FIX match
				else
				{
					// NYSE CCG matching
					char clordid[32]={0};
					strcpy(clordid,norder->clordid);
					if(strchr(norder->clordid,'/'))
					{
						char *cptr=strchr(clordid,'/'); *cptr=0;
						cptr=strchr(clordid,' ');
						if(cptr)
						{
							memmove(clordid+1,clordid,(int)(cptr -clordid));
							*clordid='_';
						}
					}
					if(!strncmp(clordid,torder->ecnordid,strlen(torder->ecnordid)))
					{
						torder->rtnext=norder; norder->rtprevList.push_back(torder); 
					}
				}
			}
			else if((!strcmp(torder->asys,"TRADER"))&&(!strcmp(norder->asys,"TRADER")))
			{
				if((strrcmp(torder->ainst,"_C"))&&(!strrcmp(norder->ainst,"_C")))
				{
					// NYSE CCG matching
					char clordid[32]={0};
					strcpy(clordid,norder->clordid);
					if(strchr(norder->clordid,'/'))
					{
						char *cptr=strchr(clordid,'/'); *cptr=0;
						cptr=strchr(clordid,' ');
						if(cptr)
						{
							memmove(clordid+1,clordid,(int)(cptr -clordid));
							*clordid='_';
						}
					}
					if(!strcmp(clordid,torder->routedorderid))
					{
						torder->rtnext=norder; norder->rtprevList.push_back(torder); 
						break;
					}
				}
			}
			else if((!strcmp(torder->asys,"TWIST"))&&(!strcmp(norder->asys,"RTOATS")))
			{
				if(!strcmp(torder->clordid,norder->ecnordid))
				{
					torder->rtnext=norder; norder->rtprevList.push_back(torder); 
				}
			}
			// Match FIX to OATS
			else if((!strcmp(torder->asys,"RTECHOATS"))&&(!strcmp(norder->asys,"RTOATS")))
			{
				if((strrcmp(norder->clordid,torder->clordid))&&
				   (!isdigit(norder->clordid[strlen(norder->clordid) -strlen(torder->clordid) -1])))
				{
					torder->rtnext=norder; norder->rtprevList.push_back(torder); 
				}
			}
			// Match OATS OATS
			else if((!strcmp(torder->asys,"RTOATS"))&&(!strcmp(norder->asys,"RTOATS")))
			{
				// Match OG OATS to GS OATS
				if((strncmp(torder->clientid,"SLK_",4))&&
				   (!strncmp(norder->clientid,"SLK_",4))&&
				   (!strcmp(torder->side,norder->side))&&
				   (!strcmp(torder->symbol,norder->symbol))&&
				   (!strcmp(torder->price,norder->price)&&
				   (!strcmp(torder->orderqty,norder->orderqty))&&
				   (!strncmp(torder->transacttime,norder->transacttime,13))))
				{
					torder->rtnext=norder; norder->rtprevList.push_back(torder); 
				}
				// Match RT to NW/CR
				else if((!strcmp(torder->clientid,norder->clientid))&&
					(!strcmp(torder->clordid,norder->rootorderid)))
				{
					torder->rtnext=norder; norder->rtprevList.push_back(torder); 
				}
			}
		}
	}

	// Match child orders to parent orders
	for(i=0;i<totcnt;i++)
	{
		TRACEORDER *torder=sorders[i];
		// parents only
		if(!strcmp(torder->asys,"IQMLALGO"))
		{
			if(!strcmp(torder->account,"ALGOACC"))
				continue;
		}
		else if(!strcmp(torder->asys,"RTECHOATS"))
		{
			if((!*torder->rootorderid)||(strcmp(torder->rootorderid,torder->ecnordid)))
				continue;
		}
		else
		{
			if(*torder->clparentorderid)
				continue;
		}
		for(int j=0;j<totcnt;j++)
		{
			TRACEORDER *norder=sorders[j];
			if(j==i) 
				continue;
			else if(strcmp(norder->orderdate,torder->orderdate))
				continue;
			else if(norder->parent)	// parent already found
				continue;
			else if(norder->crprev) // already matched in C/R chain
				continue;
			else if(!norder->rtprevList.empty()) // already matched across instances
				continue;
			// children only
			if((*norder->clparentorderid)&&(!strcmp(norder->clparentorderid,torder->clordid)))
			{
				norder->parent=torder; torder->childList.push_back(norder);
			}
			else if((*norder->rootorderid)&&(!strcmp(norder->rootorderid,torder->ecnordid)))
			{
				norder->parent=torder; torder->childList.push_back(norder);
			}
		}
	}
	delete [] sorders;

	// Pull out parents (for order within instances)
	ORDERLIST ulist;
	for(list<TraceApp*>::iterator ait=alist.begin();ait!=alist.end();ait++)
	{
		TraceApp *tapp=*ait;
		for(i=0;i<tapp->totcnt;i++)
		{
			TRACEORDER& torder=*tapp->sorders[i];
			if((torder.parent)||(torder.crprev)||(!torder.rtprevList.empty()))
				continue;
			// If there's more than one instance (trace all systems)
			if(alist.size()>1)
			{
				// We're missing the FIXServer parent so group under UNMATCHED parent
				if((!strcmp(torder.asys,"CLSERVER"))||(!strcmp(torder.asys,"TRADER"))||(!strcmp(torder.asys,"RTOATS"))||
				   ((!strcmp(torder.asys,"FIXSERVER"))&&(strrcmp(torder.ainst,"_C"))))
				{
					ulist.push_back(&torder);
					continue;
				}
			}
			plist.push_back(&torder);
		}
	}

	// Instance for unmatched orders
	if(!ulist.empty())
	{
		TraceApp *uapp=new TraceApp("UNMATCHED");
		alist.push_back(uapp);
		uapp->totcnt=1;
		uapp->sorders=new TRACEORDER *[uapp->totcnt];
		memset(uapp->sorders,0,uapp->totcnt *sizeof(TRACEORDER*));
		TRACEORDER *uparent=new TRACEORDER;
		strcpy(uparent->asys,"UNMATCHED");
		uparent->str=new char[256];
		memset(uparent->str,0,256);
		char *sptr=uparent->str;
		uparent->ainst=sptr; strcpy(sptr,"Unmatched"); sptr+=strlen(sptr) +1;
		uparent->clordid=sptr; strcpy(sptr,"Uparent"); sptr+=strlen(sptr) +1;
		uparent->rootorderid=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->firstclordid=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->symbol=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->account=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->ecnordid=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->clientid=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->price=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->side=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->orderqty=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->cumqty=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->fillqty=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->term=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->highmsgtype=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->highexectype=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->transacttime=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->orderloc=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->connection=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->routedorderid=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->clparentorderid=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		uparent->orderdate=sptr; strcpy(sptr,""); sptr+=strlen(sptr) +1;
		_ASSERT(sptr -uparent->str<256);
		uapp->sorders[0]=uparent;
		for(ORDERLIST::iterator uit=ulist.begin();uit!=ulist.end();uit++)
		{
			TRACEORDER *uorder=*uit;
			uorder->parent=uparent; uparent->childList.push_back(uorder);
		}
		plist.push_back(uparent);
	}

	// Make sure there are no cycles in the tree that will cause infinite loop
	set<TRACEORDER*> tset;
	for(ORDERLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
		BreakTreeCycles(tset,*pit);
	return 0;
}
TraceApp::TraceApp(const string& asys)
{
	this->asys=asys;
	sorders=0;
	totcnt=0;
}
TraceApp::~TraceApp()
{
	for(int i=0;i<totcnt;i++)
	{
		delete sorders[i]->str;
		delete sorders[i];
	}
	delete [] sorders; sorders=0; totcnt=0;
}
int TraceApp::LoadOrders(VSDBQUERY *tquery)
{
	asys=tquery->asys;
	totcnt=tquery->results.size();
	sorders=new TRACEORDER*[totcnt]; 
	char tdstr[32]={0};
	if(tquery->traceDate>0)
		sprintf(tdstr,"%08d",tquery->traceDate);
	int i=0;
	for(list<string>::iterator rit=tquery->results.begin();rit!=tquery->results.end();rit++)
	{
		sorders[i]=new TRACEORDER();
		TRACEORDER& torder=*sorders[i++];
		torder.str=new char[rit->size() +1];
		strcpy(torder.str,rit->c_str());
		strcpy(torder.asys,tquery->asys);
		torder.ainst=torder.clordid=torder.ecnordid=torder.routedorderid=torder.clparentorderid="";
		char *bptr=torder.str;
		for(int c=0;c<OCOL_COUNT;c++)
		{
			char *eptr=strchr(bptr,0x01);
			if(eptr) *eptr=0;
			else break;
			switch(c)
			{
			case OCOL_APPINSTID: torder.ainst=bptr; break;
			case OCOL_CLORDID: torder.clordid=bptr; break;
			case OCOL_ROOTORDERID: torder.rootorderid=bptr; break;
			case OCOL_FIRSTCLORDID: torder.firstclordid=bptr; break;
			case OCOL_SYMBOL: torder.symbol=bptr; break;
			case OCOL_ACCOUNT: torder.account=bptr; break;
			case OCOL_ECNORDERID: torder.ecnordid=bptr; break;
			case OCOL_CLIENTID: torder.clientid=bptr; break;
			case OCOL_PRICE: torder.price=bptr; break;
			case OCOL_SIDE: torder.side=bptr; break;
			case OCOL_ORDERQTY: torder.orderqty=bptr; break;
			case OCOL_CUMQTY: torder.cumqty=bptr; break;
			case OCOL_FILLQTY: torder.fillqty=bptr; break;
			case OCOL_TERM: torder.term=bptr; break;
			case OCOL_HIGHMSGTYPE: torder.highmsgtype=bptr; break;
			case OCOL_HIGHEXECTYPE: torder.highexectype=bptr; break;
			case OCOL_TRANSACTTIME: torder.transacttime=bptr; break;
			case OCOL_ORDERLOC: torder.orderloc=bptr; break;
			case OCOL_CONNECTION: torder.connection=bptr; break;
			case OCOL_CLPARENTORDERID: torder.clparentorderid=bptr; break;
			case OCOL_ORDERDATE: torder.orderdate=bptr; break;
			//case OCOL_ROUTEDORDERID: torder.routedorderid=bptr; break;
			case OCOL_ROUTINGBROKER: torder.routedorderid=bptr; break;
			case OCOL_SECURITYTYPE: torder.securitytype=bptr; break;
			};
			bptr=eptr+1;
		}
		// Don't include cldrop overnight positions in view
		if(asys=="CLSERVER")
		{
			// e.g. PIN_20121126-1976
			if((tdstr[0])&&(strchr(torder.clordid,'_'))&&(strchr(torder.clordid,'-')))
			{
				if(!strstr(torder.clordid,tdstr))
				{
					i--; totcnt--;
				}
			}
		}
		// We only get execution reports from TWIST, so fix up highmsgtype
		else if(asys=="TWIST")
		{
			if(strcmp(torder.clordid,torder.firstclordid)!=0)
			{
				if((*torder.highexectype=='4')&&(!strcmp(torder.orderqty,"0")))
					torder.highmsgtype="F";
				else if((*torder.highexectype=='5')||(*torder.firstclordid))
					torder.highmsgtype="G";
			}
		}
	}

	if(totcnt>1)
	{
		// Match C/R chains
		qsort(sorders,totcnt,sizeof(TRACEORDER*),_SortByFirstClOrdID);
		for(int i=0;i<totcnt;i++)
		{
			TRACEORDER *torder=sorders[i];
			TRACEORDER *last=torder;
			for(int j=i+1;
				(j<totcnt)&&(!strcmp(sorders[j]->ainst,torder->ainst))&&
				(*sorders[j]->firstclordid)&&(!strcmp(sorders[j]->firstclordid,torder->firstclordid));
				j++)
			{
				TRACEORDER *norder=sorders[j];
				if((!norder->crprev)&&
				   ((!strcmp(norder->highmsgtype,"G"))||(!strcmp(norder->highmsgtype,"F"))||
				    (!strcmp(norder->highexectype,"5"))||(!strcmp(norder->highexectype,"6"))))
				{
					last->crnext=norder; norder->crprev=last; last=norder;
				}
				i=j;
			}
		}

		// Put back in time order
		qsort(sorders,totcnt,sizeof(TRACEORDER*),_SortByTransactTime);
	}
	return 0;
}
static int RollupChildDetails(TRACEORDER *porder, int& ocnt, int& posQty, int& rtQty, int& fillQty)
{
	ocnt++;
	if(porder->childList.empty())
	{
		for(TRACEORDER *norder=porder;norder;norder=norder->crnext)
		{
			// Don't include position quantity
			int oqty=atoi(norder->orderqty);
			if(oqty>0) 
			{
				if(*norder->highexectype=='B')
					posQty+=oqty;
				else
					rtQty+=oqty;
			}
			int fqty=atoi(norder->fillqty);
			if(fqty>0) fillQty+=fqty;
			if(norder->crnext==norder)
			{
				_ASSERT(false); break;
			}
		}
	}
	else
	{
		for(ORDERLIST::iterator cit=porder->childList.begin();cit!=porder->childList.end();cit++)
		{
			TRACEORDER *corder=*cit;
			RollupChildDetails(corder,ocnt,posQty,rtQty,fillQty);
		}
	}
	return 0;
}
// Group C/R chains vertically
// We repeat the parent NW so we can aggregate fill qty to top level
// |_NW B BAC 9.98 700
//		|_NW
//			|_CHILD ORDERS
//				|_NW B BAC 9.98 100
//		|_CR B BAC 9.99
//			|_CHILD ORDERS
//				|_NW B BAC 9.99 100
//		|_CL
// |_NW B BAC 9.99 1000
//		|_CHILD ORDERS
//			|_NW
static HTREEITEM RecurseLoadTree(CTreeCtrl& ctc, HTREEITEM pitem, TRACEORDER *porder)
{
	CString display;
	// Parent order has been replaced or cancelled
	if(porder->crnext)
	{
		//display.Format("%s [%s]",porder->transacttime +8,porder->firstclordid);
		display=DetailedNodeDisplay(porder,true);
		for(TRACEORDER *rorder=porder->rtnext;rorder;rorder=rorder->rtnext)
			display+=" => " +ShortNodeDisplay(rorder);
		if(porder->vsoats)
			display+=" => " +ShortNodeDisplay(porder->vsoats);
		pitem=ctc.InsertItem(display,pitem);
		ctc.SetItemData(pitem,(LPARAM)porder);
		int img=0,ordqty=0,fillqty=0;
		for(TRACEORDER *norder=porder;norder;norder=norder->crnext)
		{
			display=DetailedNodeDisplay(norder,false);
			for(TRACEORDER *rorder=norder->rtnext;rorder;rorder=rorder->rtnext)
				display+=" => " +ShortNodeDisplay(rorder);
			if(norder->vsoats)
				display+=" => " +ShortNodeDisplay(norder->vsoats);
			HTREEITEM cpitem=ctc.InsertItem(display,pitem);
			ctc.SetItemData(cpitem,(LPARAM)norder);
			img=theApp.OrderImage(*norder->highmsgtype,*norder->highexectype,atoi(norder->orderqty),atoi(norder->fillqty));
			// If this is a CLSERVER position, show the $ icon
			if((!strcmp(porder->asys,"CLSERVER"))||(!strcmp(porder->asys,"TWIST")))
			{
				if((!porder->highmsgtype[0])&&
				   (!strcmp(porder->highexectype,"B"))&&
				   (!strcmp(porder->fillqty,"0")))
					img=12;
			}
			ctc.SetItemImage(cpitem,img,img);
			if(atoi(norder->orderqty)>0) ordqty=atoi(norder->orderqty);
			fillqty+=atoi(norder->fillqty);
			if(!norder->childList.empty())
			{
				int ccnt=0,posQty=0,rtQty=0,fillQty=0;
				RollupChildDetails(norder,ccnt,posQty,rtQty,fillQty);
				display.Format("%d CHILD ORDERS, positions %d, shares routed %d, filled %d",ccnt-1,posQty,rtQty,fillQty);
				cpitem=ctc.InsertItem(display,cpitem);
				for(ORDERLIST::iterator cit=norder->childList.begin();cit!=norder->childList.end();cit++)
				{
					TRACEORDER *corder=*cit;
					RecurseLoadTree(ctc,cpitem,corder);
				}
				ctc.Expand(cpitem,TVE_EXPAND);
			}
			if(norder->crnext==norder)
			{
				_ASSERT(false); break;
			}
		}
		// Set the parent to status of last order in cancel/replace chain
		if(fillqty>0)
		{
			img=2;
			if(fillqty>=ordqty) img=3;
		}
		ctc.SetItemImage(pitem,img,img);
		return pitem;
	}
	// Single parent order
	else
	{
		display=DetailedNodeDisplay(porder,false);
		for(TRACEORDER *rorder=porder->rtnext;rorder;rorder=rorder->rtnext)
			display+=" => " +ShortNodeDisplay(rorder);
		if(porder->vsoats)
			display+=" => " +ShortNodeDisplay(porder->vsoats);
		HTREEITEM cpitem=ctc.InsertItem(display,pitem);
		ctc.SetItemData(cpitem,(LPARAM)porder);
		int img=theApp.OrderImage(*porder->highmsgtype,*porder->highexectype,atoi(porder->orderqty),atoi(porder->fillqty));
		// If this is a CLSERVER position, show the $ icon
		if((!strcmp(porder->asys,"CLSERVER"))||(!strcmp(porder->asys,"TWIST")))
		{
			if((!porder->highmsgtype[0])&&
				(!strcmp(porder->highexectype,"B"))&&
				(!strcmp(porder->fillqty,"0")))
				img=12;
		}
		ctc.SetItemImage(cpitem,img,img);
		if(!porder->childList.empty())
		{
			int ccnt=0,posQty=0,rtQty=0,fillQty=0;
			RollupChildDetails(porder,ccnt,posQty,rtQty,fillQty);
			display.Format("%d CHILD ORDERS, positions %d, shares routed %d, filled %d",ccnt-1,posQty,rtQty,fillQty);
			cpitem=ctc.InsertItem(display,cpitem);
			for(ORDERLIST::iterator cit=porder->childList.begin();cit!=porder->childList.end();cit++)
			{
				TRACEORDER *corder=*cit;
				RecurseLoadTree(ctc,cpitem,corder);
			}
			ctc.Expand(cpitem,TVE_EXPAND);
		}
		return cpitem;
	}
}
static TRACEORDER *RecurseFindOrder(TRACEORDER *porder, const char *orderid, const char *idtype)
{
	if((!strcmp(idtype,"ClOrdID"))&&(stristr(porder->clordid,orderid)))
		return porder;
	else if((!strcmp(idtype,"RootOrderID"))&&(stristr(porder->rootorderid,orderid)))
		return porder;
	else if((!strcmp(idtype,"FirstClOrdID"))&&(stristr(porder->firstclordid,orderid)))
		return porder;
	else if((!strcmp(idtype,"ClParentOrderID"))&&(stristr(porder->clparentorderid,orderid)))
		return porder;
	else if((!strcmp(idtype,"Account"))&&(stristr(porder->account,orderid)))
		return porder;
	else if((!strcmp(idtype,"EcnOrderID"))&&(stristr(porder->ecnordid,orderid)))
		return porder;
	else if((!strcmp(idtype,"ClientID"))&&(stristr(porder->clientid,orderid)))
		return porder;
	else if((!strcmp(idtype,"TransactTime"))&&(stristr(porder->transacttime,orderid)))
		return porder;
	else if((!strcmp(idtype,"Connection"))&&(stristr(porder->connection,orderid)))
		return porder;
	//else if((!strcmp(idtype,"RoutedOrderID"))&&(stristr(porder->routedorderid,orderid)))
	else if((!strcmp(idtype,"RoutingBroker"))&&(stristr(porder->routedorderid,orderid)))
		return porder;
	else if((!strcmp(idtype,"OrderDate"))&&(stristr(porder->orderdate,orderid)))
		return porder;

	for(TRACEORDER *norder=porder->rtnext;norder;norder=norder->rtnext)
	{
		if((!strcmp(idtype,"ClOrdID"))&&(stristr(norder->clordid,orderid)))
			return norder;
		else if((!strcmp(idtype,"RootOrderID"))&&(stristr(norder->rootorderid,orderid)))
			return norder;
		else if((!strcmp(idtype,"FirstClOrdID"))&&(stristr(norder->firstclordid,orderid)))
			return norder;
		else if((!strcmp(idtype,"ClParentOrderID"))&&(stristr(norder->clparentorderid,orderid)))
			return norder;
		else if((!strcmp(idtype,"Account"))&&(stristr(norder->account,orderid)))
			return norder;
		else if((!strcmp(idtype,"EcnOrderID"))&&(stristr(norder->ecnordid,orderid)))
			return norder;
		else if((!strcmp(idtype,"ClientID"))&&(stristr(norder->clientid,orderid)))
			return norder;
		else if((!strcmp(idtype,"TransactTime"))&&(stristr(norder->transacttime,orderid)))
			return norder;
		else if((!strcmp(idtype,"Connection"))&&(stristr(norder->connection,orderid)))
			return norder;
		//else if((!strcmp(idtype,"RoutedOrderID"))&&(stristr(norder->routedorderid,orderid)))
		else if((!strcmp(idtype,"RoutingBroker"))&&(stristr(norder->routedorderid,orderid)))
			return norder;
		else if((!strcmp(idtype,"OrderDate"))&&(stristr(norder->orderdate,orderid)))
			return norder;
		if(norder->rtnext==norder)
		{
			_ASSERT(false);	break;
		}
	}
	if(porder->vsoats)
	{
		if((!strcmp(idtype,"ClOrdID"))&&(stristr(porder->vsoats->clordid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"RootOrderID"))&&(stristr(porder->vsoats->rootorderid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"FirstClOrdID"))&&(stristr(porder->vsoats->firstclordid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"ClParentOrderID"))&&(stristr(porder->vsoats->clparentorderid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"Account"))&&(stristr(porder->vsoats->account,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"EcnOrderID"))&&(stristr(porder->vsoats->ecnordid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"ClientID"))&&(stristr(porder->vsoats->clientid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"TransactTime"))&&(stristr(porder->vsoats->transacttime,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"Connection"))&&(stristr(porder->vsoats->connection,orderid)))
			return porder->vsoats;
		//else if((!strcmp(idtype,"RoutedOrderID"))&&(stristr(porder->vsoats->routedorderid,orderid)))
		else if((!strcmp(idtype,"RoutingBroker"))&&(stristr(porder->vsoats->routedorderid,orderid)))
			return porder->vsoats;
		else if((!strcmp(idtype,"OrderDate"))&&(stristr(porder->vsoats->orderdate,orderid)))
			return porder->vsoats;
	}

	if(porder->crnext)
	{
		TRACEORDER *forder=RecurseFindOrder(porder->crnext,orderid,idtype);
		if(forder)
			return forder;
	}

	for(ORDERLIST::iterator cit=porder->childList.begin();cit!=porder->childList.end();cit++)
	{
		TRACEORDER *corder=*cit;
		TRACEORDER *forder=RecurseFindOrder(corder,orderid,idtype);
		if(forder)
			return forder;
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
int CTraceView::LoadTraceResults()
{
	int crid=0;
	for(list<VSDBQUERY *>::iterator qit=ftqlist.begin();qit!=ftqlist.end();qit++)
	{
		VSDBQUERY *tquery=*qit;
		TraceApp *tapp=new TraceApp((const char*)tquery->asys);
		tapp->LoadOrders(tquery);
		alist.push_back(tapp);
		if(!crid)
		{
			crid=tquery->rid; curq.rid=crid;
		}
		tquery->results.clear();
		delete tquery;
	}
	ftqlist.clear();

	ORDERLIST plist;
	MatchOrders(plist,alist);

	// Load the tree
	CTreeCtrl& ctc=GetTreeCtrl();
	int pcnt=0,totOrdQty=0,totPosQty=0,totRtQty=0,totFillQty=0;
	for(ORDERLIST::iterator pit=plist.begin();pit!=plist.end();pit++)
	{
		// Apply filter to root parent orders
		TRACEORDER *porder=*pit;
		string pside;
		if(!strcmp(porder->side,"1")) pside="B";
		else if(!strcmp(porder->side,"2")) pside="SL";
		else if(!strcmp(porder->side,"5")) pside="SS";
		else if(!strcmp(porder->side,"6")) pside="SX";
		if((!curq.traceSide.empty())&&(curq.traceSide!="ANY")&&(curq.traceSide!=pside))
			continue;
		if((curq.tracePrice!=0.0)&&(curq.tracePrice!=atof(porder->price)))
			continue;
		if((curq.traceShares!=0)&&(curq.traceShares!=atoi(porder->orderqty)))
			continue;
		if((!curq.traceOrder.empty())&&(!RecurseFindOrder(porder,curq.traceOrder.c_str(),curq.traceIdType.c_str())))
			continue;
		RecurseLoadTree(ctc,0,porder); pcnt++;

		int ordqty=0;
		for(TRACEORDER *norder=porder;norder;norder=norder->crnext)
		{
			if((!strcmp(norder->highmsgtype,"D"))||(!strcmp(norder->highmsgtype,"G")))
			{
				int oqty=atoi(porder->orderqty);
				if(oqty>0) ordqty=oqty;
			}
			if(norder->crnext==norder)
			{
				_ASSERT(false); break;
			}
		}
		if(ordqty>0) totOrdQty+=ordqty;
		int ccnt=0,posQty=0,rtQty=0,fillQty=0;
		RollupChildDetails(porder,ccnt,posQty,rtQty,fillQty);
		totPosQty+=posQty; totRtQty+=rtQty; totFillQty+=fillQty;
	}

	CString display;
	int fperc=0;
	if(totOrdQty>0) fperc=totFillQty*100/totOrdQty;
	else if(totRtQty>0) fperc=totFillQty*100/totRtQty;
	//display.Format("Total x %d [F%d]",totRtQty,totFillQty);
	display.Format("Total %d groups, %d orders, positions %d, shares ordered %d, routed %d, filled %d (%d%%)",
		0,pcnt,totPosQty,totOrdQty,totRtQty,totFillQty,fperc);
	ctc.InsertItem(display);
	smsg=display;
	theApp.pMainFrame->SetStatus(1,smsg);

	GroupParents();

	// Select order that contains the key
	if(!curq.traceOrder.empty())
		RecurseFindKey(ctc,0,curq.traceOrder.c_str(),true);
	// Select the totals line
	else
	{
		HTREEITEM pfirst=ctc.GetChildItem(0);
		if(pfirst)
		{
			ctc.SelectItem(pfirst);
			ctc.EnsureVisible(pfirst);
		}
	}

	plist.clear();
	// Don't delete TRACEORDERs because we've they are set as item data on the tree nodes
	//for(list<TraceApp*>::iterator ait=alist.begin();ait!=alist.end();ait++)
	//	delete *ait;
	//alist.clear();
	return 0;
}
static int CALLBACK _SortTreeParents(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	TRACEORDER *to1=(TRACEORDER *)lParam1;
	TRACEORDER *to2=(TRACEORDER *)lParam2;
	// Put the totals line at the top
	if((to1)&&(!to2))
		return +1;
	else if((!to1)&&(to2))
		return -1;
	int cmp=0;
	list<WORD>& psortlist=*(list<WORD>*)lParamSort;
	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
	{
		WORD psortcol=*sit;
		if(!LOBYTE(psortcol))
			continue;
		switch(HIBYTE(psortcol))
		{
		case OCOL_APPINSTID: cmp=strcmp(to1->ainst,to2->ainst); break;
		case OCOL_CLORDID: cmp=strcmp(to1->clordid,to2->clordid); break;
		case OCOL_ROOTORDERID: cmp=strcmp(to1->rootorderid,to2->rootorderid); break;
		case OCOL_FIRSTCLORDID: cmp=strcmp(to1->firstclordid,to2->firstclordid); break;
		case OCOL_SYMBOL: cmp=strcmp(to1->symbol,to2->symbol); break;
		case OCOL_ACCOUNT: cmp=strcmp(to1->account,to2->account); break;
		case OCOL_ECNORDERID: cmp=strcmp(to1->ecnordid,to2->ecnordid); break;
		case OCOL_CLIENTID: cmp=strcmp(to1->clientid,to2->clientid); break;
		case OCOL_PRICE: 
		{
			double px1=atof(to1->price);
			double px2=atof(to2->price);
			if(px1>px2) cmp=+1;
			else if(px1<px2) cmp=-1;
			break;
		}
		case OCOL_SIDE: cmp=strcmp(to1->side,to2->side); break;
		case OCOL_ORDERQTY: cmp=atoi(to1->orderqty) -atoi(to2->orderqty); break;
		case OCOL_CUMQTY: cmp=atoi(to1->cumqty) -atoi(to2->cumqty); break;
		case OCOL_FILLQTY: cmp=atoi(to1->fillqty) -atoi(to2->fillqty); break;
		case OCOL_TERM: cmp=strcmp(to1->term,to2->term); break;
		case OCOL_HIGHMSGTYPE: cmp=theApp.GetMsgTypeTier(*to1->highmsgtype) -theApp.GetMsgTypeTier(*to2->highmsgtype); break;
		case OCOL_HIGHEXECTYPE: cmp=theApp.GetExecTypeTier(*to1->highexectype) -theApp.GetExecTypeTier(*to2->highexectype); break;
		case OCOL_TRANSACTTIME: cmp=strcmp(to1->transacttime,to2->transacttime); break;
		case OCOL_ORDERLOC: 
		{
			__int64 l1=_atoi64(to1->orderloc);
			__int64 l2=_atoi64(to2->orderloc);
			if(l1>l2) cmp=+1;
			else if(l1<l2) cmp=-1;
			break;
		}
		case OCOL_CONNECTION: cmp=strcmp(to1->connection,to2->connection); break;
		case OCOL_CLPARENTORDERID: cmp=strcmp(to1->clparentorderid,to2->clparentorderid); break;
		case OCOL_ORDERDATE: cmp=strcmp(to1->orderdate,to2->orderdate); break;
		//case OCOL_ROUTEDORDERID: cmp=strcmp(to1->routedorderid,to2->routedorderid); break;
		case OCOL_ROUTINGBROKER: cmp=strcmp(to1->routedorderid,to2->routedorderid); break;
		case OCOL_SECURITYTYPE: cmp=strcmp(to1->securitytype,to2->securitytype); break;
		};
		if(cmp) return cmp;
	}
	// When all the same, sort by time then order
	// Sort by timestamp
	cmp=strcmp(to1->transacttime,to2->transacttime);
	if(cmp) return cmp;
	// Sort by order creation order
	__int64 l1=_atoi64(to1->orderloc);
	__int64 l2=_atoi64(to2->orderloc);
	if(l1>l2) cmp=+1;
	else if(l1<l2) cmp=-1;
	return cmp;
}
static HTREEITEM RecurseMoveChildren(CTreeCtrl& ctc, HTREEITEM pitem, HTREEITEM citem)
{
	CString text=ctc.GetItemText(citem);
	HTREEITEM npitem=ctc.InsertItem(text,0,0,pitem,0);
	ctc.SetItemData(npitem,ctc.GetItemData(citem));
	int img,simg;
	ctc.GetItemImage(citem,img,simg);
	ctc.SetItemImage(npitem,img,simg);
	for(HTREEITEM gcitem=ctc.GetChildItem(citem);gcitem;)
	{
		HTREEITEM nitem=ctc.GetNextSiblingItem(gcitem);
		RecurseMoveChildren(ctc,npitem,gcitem);
		gcitem=nitem;
	}
	if(text.Find("CHILD ORDERS")>=0)
		ctc.Expand(npitem,TVE_EXPAND);

	ctc.DeleteItem(citem);
	return npitem;
}
// |__Total
// |__<group>
//		|__# PARENT ORDERS shared routed #, filled #
//			|__ ...
// |__<group>
// ...
int CTraceView::GroupParents()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	// Remove grouping
	HTREEITEM prev=0;
	for(HTREEITEM pitem=ctc.GetChildItem(0);pitem;)
	{
		HTREEITEM titem=ctc.GetChildItem(pitem);
		if(!titem)
		{
			prev=pitem; pitem=ctc.GetNextSiblingItem(pitem); continue;
		}
		if(ctc.GetItemText(titem).Find("PARENT ORDERS")<0)
		{
			prev=pitem; pitem=ctc.GetNextSiblingItem(pitem); continue;
		}
		for(HTREEITEM citem=ctc.GetChildItem(titem);citem;)
		{
			HTREEITEM nitem=ctc.GetNextSiblingItem(citem);
			prev=RecurseMoveChildren(ctc,0,citem);
			citem=nitem;
		}
		HTREEITEM nitem=ctc.GetNextSiblingItem(pitem);
		ctc.DeleteItem(pitem);
		pitem=nitem;
	}

	// Sort top level of parent orders
	list<WORD>& psortlist=theApp.psortlist;
	TVSORTCB tvsort;
	tvsort.hParent=0;
	tvsort.lParam=(LPARAM)&psortlist;
	tvsort.lpfnCompare=_SortTreeParents;
	ctc.SortChildrenCB(&tvsort);

	int nsorts=0;
	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
		if(LOBYTE(*sit)) nsorts++;
	if(nsorts<1)
		return 0;
	// Group first level parents
	ctc.LockWindowUpdate();
	string lkey;
	prev=0;
	HTREEITEM lpitem=0;
	for(HTREEITEM pitem=ctc.GetChildItem(0);pitem;)
	{
		TRACEORDER *porder=(TRACEORDER *)ctc.GetItemData(pitem);
		if(!porder)
		{
			prev=pitem; pitem=ctc.GetNextSiblingItem(pitem); continue;
		}
		char pkey[1024]={0},*kptr=pkey;
		for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
		{
			WORD psortcol=*sit;
			if(!LOBYTE(psortcol))
				continue;
			if(*pkey) {*kptr=','; kptr++;}
			switch(HIBYTE(psortcol))
			{
			case OCOL_APPINSTID: strcpy(kptr,porder->ainst); kptr+=strlen(kptr); break;
			case OCOL_CLORDID: strcpy(kptr,porder->clordid); kptr+=strlen(kptr); break;
			case OCOL_ROOTORDERID: strcpy(kptr,porder->routedorderid); kptr+=strlen(kptr); break;
			case OCOL_FIRSTCLORDID: strcpy(kptr,porder->firstclordid); kptr+=strlen(kptr); break;
			case OCOL_SYMBOL: strcpy(kptr,porder->symbol); kptr+=strlen(kptr); break;
			case OCOL_ACCOUNT: strcpy(kptr,porder->account); kptr+=strlen(kptr); break;
			case OCOL_ECNORDERID: strcpy(kptr,porder->ecnordid); kptr+=strlen(kptr); break;
			case OCOL_CLIENTID: strcpy(kptr,porder->clientid); kptr+=strlen(kptr); break;
			case OCOL_PRICE: sprintf(kptr,"%.4f",atof(porder->price)); kptr+=strlen(kptr); break;
			case OCOL_SIDE: 
			{
				const char *sstr=porder->side;
				if(!strcmp(porder->side,"1")) sstr="B";
				else if(!strcmp(porder->side,"2")) sstr="SL";
				else if(!strcmp(porder->side,"5")) sstr="SS";
				else if(!strcmp(porder->side,"6")) sstr="SX";
				strcpy(kptr,sstr); kptr+=strlen(kptr);
				break;
			}
			case OCOL_ORDERQTY: strcpy(kptr,porder->orderqty); kptr+=strlen(kptr); break;
			case OCOL_CUMQTY: strcpy(kptr,porder->cumqty); kptr+=strlen(kptr); break;
			case OCOL_FILLQTY: strcpy(kptr,porder->fillqty); kptr+=strlen(kptr); break;
			case OCOL_TERM: strcpy(kptr,porder->term); kptr+=strlen(kptr); break;
			case OCOL_HIGHMSGTYPE: strcpy(kptr,porder->highmsgtype); kptr+=strlen(kptr); break;
			case OCOL_HIGHEXECTYPE: strcpy(kptr,porder->highexectype); kptr+=strlen(kptr); break;
			case OCOL_TRANSACTTIME: 
				memcpy(kptr,porder->transacttime,8); kptr[8]='-';
				memcpy(kptr +9,porder->transacttime +8,4); kptr[13]=0; 
				kptr+=strlen(kptr); 
				break;
			case OCOL_ORDERLOC: strcpy(kptr,porder->orderloc); kptr+=strlen(kptr); break;
			case OCOL_CONNECTION: strcpy(kptr,porder->connection); kptr+=strlen(kptr); break;
			case OCOL_CLPARENTORDERID: strcpy(kptr,porder->clparentorderid); kptr+=strlen(kptr); break;
			case OCOL_ORDERDATE: strcpy(kptr,porder->orderdate); kptr+=strlen(kptr); break;
			//case OCOL_ROUTEDORDERID: strcpy(kptr,porder->routedorderid); kptr+=strlen(kptr); break;
			case OCOL_ROUTINGBROKER: strcpy(kptr,porder->routedorderid); kptr+=strlen(kptr); break;
			case OCOL_SECURITYTYPE: strcpy(kptr,porder->securitytype); kptr+=strlen(kptr); break;
			};
		}
		if(!*pkey)
			strcpy(pkey,"<NULL>");
		// Make a new group node
		if(lkey!=pkey)
		{
			if(lpitem) ctc.Expand(lpitem,TVE_EXPAND);		
			if(prev) ctc.Expand(prev,TVE_EXPAND);
			prev=ctc.InsertItem(pkey,0,0,0,prev?prev:TVI_FIRST); lkey=pkey;
			lpitem=ctc.InsertItem("PARENT ORDERS",0,0,prev,0);
		}
		HTREEITEM nitem=ctc.GetNextSiblingItem(pitem);
		_ASSERT(lpitem); // catches infinite loops
		RecurseMoveChildren(ctc,lpitem,pitem);
		pitem=nitem;
	}
	if(lpitem) ctc.Expand(lpitem,TVE_EXPAND);		
	if(prev) ctc.Expand(prev,TVE_EXPAND);

	// Rollup parent orders
	int ngroups=0;
	for(HTREEITEM pitem=ctc.GetChildItem(0);pitem;pitem=ctc.GetNextSiblingItem(pitem))
	{
		HTREEITEM titem=ctc.GetChildItem(pitem);
		if(!titem)
			continue;
		_ASSERT(ctc.GetItemText(titem)=="PARENT ORDERS"); ngroups++;
		int pcnt=0,ccnt=0,posQty=0,rtQty=0,fillQty=0;
		for(HTREEITEM citem=ctc.GetChildItem(titem);citem;citem=ctc.GetNextSiblingItem(citem))
		{
			TRACEORDER *porder=(TRACEORDER*)ctc.GetItemData(citem); pcnt++;
			RollupChildDetails(porder,ccnt,posQty,rtQty,fillQty); 
		}
		CString display;
		display.Format("%d PARENT ORDERS, positions %d, shares routed %d, filled %d",pcnt,posQty,rtQty,fillQty);
		ctc.SetItemText(titem,display);
	}
	ctc.UnlockWindowUpdate();

	if(!strncmp(smsg,"Total ",6))
	{
		const char *gptr=strstr(smsg,"groups, ");
		if(gptr)
		{
			CString nsmsg;
			nsmsg.Format("Total %d groups, %s",ngroups,gptr +8);
			smsg=nsmsg;
			theApp.pMainFrame->SetStatus(1,smsg);
		}
	}
	HTREEITEM titem=ctc.GetChildItem(0);
	if(titem)
	{
		if(ctc.GetItemText(titem).Left(6)=="Total ")
			ctc.SetItemText(titem,smsg);
	}
	return ngroups;
}

void CTraceView::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = (LPNMTREEVIEW)pNMHDR;
	// Add your control notification handler code here
	*pResult = 0;
	POINT point;
	GetCursorPos(&point);
	CTreeCtrl& ctc=GetTreeCtrl();

	HMENU hmenu = CreatePopupMenu();
	//HTREEITEM sitem=ctc.GetSelectedItem();
	// This is the item over which the cursor is pointing--not necessarily the last selected item
	HTREEITEM sitem;
	//memcpy(&sitem,(char*)pNMHDR +16*5 -4,4);
	TVHITTESTINFO pht;
	memset(&pht,0,sizeof(TVHITTESTINFO));
	pht.pt=point;
	ctc.ScreenToClient(&pht.pt);
	ctc.HitTest(&pht);
	sitem=pht.hItem;
	if(!sitem)
		return;
	rselitem=sitem;
	CString selname;
	if(sitem)
		selname=ctc.GetItemText(sitem);

	DWORD mflags=MF_STRING;
	if(selname.IsEmpty())
		mflags|=MF_GRAYED;
	else if(selname.Left(5)=="Total")
		; // Enable to expand/contract all parents
	else if((!sitem)||(!ctc.GetChildItem(sitem)))
		mflags|=MF_GRAYED;
	AppendMenu(hmenu,mflags,ID_EXPAND_ALL,"Expand all");
	AppendMenu(hmenu,mflags,ID_COLLAPSE_ALL,"Collapse all");

	AppendMenu(hmenu,MF_SEPARATOR,0,0);
	HMENU gmenu=CreatePopupMenu();
	list<WORD>& psortlist=theApp.psortlist;
	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
	{
		WORD psortcol=*sit;
		const char *kstr="";
		switch(HIBYTE(psortcol))
		{
		case OCOL_APPINSTID: kstr="AppInstID"; break;
		case OCOL_CLORDID: kstr="ClOrdID"; break;
		case OCOL_ROOTORDERID: kstr="RootOrderID"; break;
		case OCOL_FIRSTCLORDID: kstr="FirstClOrdID"; break;
		case OCOL_SYMBOL: kstr="Symbol"; break;
		case OCOL_ACCOUNT: kstr="Account"; break;
		case OCOL_ECNORDERID: kstr="EcnOrderID"; break;
		case OCOL_CLIENTID: kstr="ClientID"; break;
		case OCOL_PRICE: kstr="Price"; break;
		case OCOL_SIDE: kstr="Side"; break;
		case OCOL_ORDERQTY: kstr="OrderQty"; break;
		case OCOL_CUMQTY: kstr="CumQty"; break;
		case OCOL_FILLQTY: kstr="FillQty"; break;
		case OCOL_TERM: kstr="Term"; break;
		case OCOL_HIGHMSGTYPE: kstr="HighMsgType"; break;
		case OCOL_HIGHEXECTYPE: kstr="HighExecType"; break;
		case OCOL_TRANSACTTIME: kstr="TransactTime (Minute)"; break;
		case OCOL_ORDERLOC: kstr="OrderLoc"; break;
		case OCOL_CONNECTION: kstr="Connection"; break;
		case OCOL_CLPARENTORDERID: kstr="ClParentOrderID"; break;
		case OCOL_ORDERDATE: kstr="OrderDate"; break;
		//case OCOL_ROUTEDORDERID: kstr="RoutedOrderID"; break;
		case OCOL_ROUTINGBROKER: kstr="RoutingBroker"; break;
		case OCOL_SECURITYTYPE: kstr="SecurityType"; break;
		};
		mflags=MF_STRING;
		if(LOBYTE(psortcol)) mflags|=MF_CHECKED;
		AppendMenu(gmenu,mflags,ID_FIRST_GROUP +HIBYTE(psortcol),kstr);
	}
	AppendMenu(gmenu,MF_SEPARATOR,0,0);
	AppendMenu(gmenu,MF_STRING,ID_GROUP_NONE,"None");
	AppendMenu(gmenu,MF_STRING,ID_GROUP_REORDER,"Reorder...");
	AppendMenu(hmenu,MF_POPUP,(UINT_PTR)gmenu,"Group by");
	AppendMenu(hmenu,MF_STRING,ID_REMOVE_FROM_TRACE,"Remove from trace...");
	AppendMenu(hmenu,MF_STRING,ID_SAVE_TRACE,"Save selected trace");
	AppendMenu(hmenu,MF_STRING,ID_SAVE_TRACE_ALL,"Save entire trace");

	AppendMenu(hmenu,MF_SEPARATOR,0,0);
	TRACEORDER *torder=(TRACEORDER*)ctc.GetItemData(sitem);
	if(torder)
	{
		CString menustr;
		int i=0;
		for(TRACEORDER *norder=torder;norder;norder=norder->rtnext)
		{
			DWORD mflags=MF_STRING;
			if(!*norder->clordid) mflags|=MF_GRAYED;
			menustr.Format("Order for %s (%s.%s)",norder->clordid,norder->asys,norder->ainst);
			AppendMenu(hmenu,mflags,ID_FIRST_ORDER +i,menustr); i++;
			if(norder->rtnext==norder)
			{
				_ASSERT(false); break;
			}
		}
		if(torder->vsoats)
		{
			DWORD mflags=MF_STRING;
			if(!*torder->vsoats->clordid) mflags|=MF_GRAYED;
			menustr.Format("Order for %s (%s.%s)",torder->vsoats->clordid,torder->vsoats->asys,torder->vsoats->ainst);
			AppendMenu(hmenu,mflags,ID_FIRST_ORDER +i,menustr); i++;
		}

		AppendMenu(hmenu,MF_SEPARATOR,0,0);
		i=0;
		for(TRACEORDER *norder=torder;norder;norder=norder->rtnext)
		{
			DWORD mflags=MF_STRING;
			if(!*norder->firstclordid) mflags|=MF_GRAYED;
			menustr.Format("Cancel/replace chain for %s (%s.%s)",norder->firstclordid,norder->asys,norder->ainst);
			AppendMenu(hmenu,mflags,ID_FIRST_CRCHAIN +i,menustr); i++;
			if(norder->rtnext==norder)
			{
				_ASSERT(false); break;
			}
		}
		if(torder->vsoats)
		{
			DWORD mflags=MF_STRING;
			if(!*torder->vsoats->firstclordid) mflags|=MF_GRAYED;
			menustr.Format("Cancel/replace chain for %s (%s.%s)",torder->vsoats->firstclordid,torder->vsoats->asys,torder->vsoats->ainst);
			AppendMenu(hmenu,mflags,ID_FIRST_CRCHAIN +i,menustr); i++;
		}

		AppendMenu(hmenu,MF_SEPARATOR,0,0);
		i=0;
		for(TRACEORDER *norder=torder;norder;norder=norder->rtnext)
		{
			DWORD mflags=MF_STRING;
			if(!*norder->rootorderid) mflags|=MF_GRAYED;
			menustr.Format("Parent/Child chain for %s (%s*)",norder->rootorderid,norder->asys);
			AppendMenu(hmenu,mflags,ID_FIRST_PCCHAIN +i,menustr); i++;
			if(norder->rtnext==norder)
			{
				_ASSERT(false); break;
			}
		}
		if(torder->vsoats)
		{
			DWORD mflags=MF_STRING;
			if(!*torder->vsoats->rootorderid) mflags|=MF_GRAYED;
			menustr.Format("Parent/Child chain for %s (%s*)",torder->vsoats->rootorderid,torder->vsoats->asys);
			AppendMenu(hmenu,mflags,ID_FIRST_PCCHAIN +i,menustr); i++;
		}
	}
	else
	{
		AppendMenu(hmenu,mflags,ID_TRACE_TO_ORDERS_VIEW,"Orders View...");
	}

	TrackPopupMenu(hmenu,TPM_LEFTALIGN |TPM_TOPALIGN |TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, 0);
}
void CTraceView::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Add your control notification handler code here
	autoRClick=true;
	char tnmbuf[16*5];
	memset(tnmbuf,0,sizeof(tnmbuf));
	memcpy(tnmbuf,pNMHDR,sizeof(NMHDR));
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=ctc.GetSelectedItem();
	memcpy(tnmbuf +16*5 -4,&sitem,4);
	pNMHDR=(NMHDR*)tnmbuf;
	OnNMRClick(pNMHDR,pResult);
	autoRClick=false;
	*pResult = 0;
}
void CTraceView::OnQueryCRChain(int idx)
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	if(!sitem)
		return;
	TRACEORDER *torder=(TRACEORDER*)ctc.GetItemData(sitem);
	if(!torder)
		return;
	int i=0;
	TRACEORDER *norder;
	for(norder=torder;norder;norder=norder->rtnext)
	{
		if(i==idx)
			break;
		i++;
	}
	if((!norder)&&(i==idx))
		norder=torder->vsoats;
	if(norder)
	{
		Cvsctest3View::newq=curq;
		Cvsctest3View::newq.asys=norder->asys;
		Cvsctest3View::newq.select="*";
		#ifdef SPLIT_ORD_DET
		Cvsctest3View::newq.from="ORDERS/DETAILS";
		#else
		Cvsctest3View::newq.from="ORDERS";
		#endif
		char wclause[1024]={0};
		//if(!strcmp(norder->asys,"RTOATS"))
		//	sprintf(wclause,"AppInstID=='%s' AND ClOrdID=='%s'",norder->ainst,norder->clordid);
		//else
			sprintf(wclause,"AppInstID=='%s' AND FirstClOrdID=='%s'",norder->ainst,norder->firstclordid);
		if(*norder->orderdate)
			sprintf(wclause +strlen(wclause)," AND OrderDate==%s",norder->orderdate);
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		Cvsctest3View::newq.where=wclause;
		Cvsctest3View::newq.unique=false;
		Cvsctest3View::newq.istart=0;
		Cvsctest3View::newq.hist=true;
		#ifdef LIVE_DETAILS
		Cvsctest3View::newq.live=true;
		#else
		Cvsctest3View::newq.live=false;
		#endif
		Cvsctest3View::newq.iter=0;

		Cvsctest3Doc *pdoc=(Cvsctest3Doc *)GetDocument();
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
		CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("By FirstClOrdID",0);
		pdoc->SetDocTemplate(ldt);

		Cvsctest3View::newq.select="";
		Cvsctest3View::newq.from="";
		Cvsctest3View::newq.where="";
	}
}
void CTraceView::OnQueryCRChain1(){	OnQueryCRChain(0); }
void CTraceView::OnQueryCRChain2(){	OnQueryCRChain(1); }
void CTraceView::OnQueryCRChain3(){	OnQueryCRChain(2); }
void CTraceView::OnQueryCRChain4(){	OnQueryCRChain(3); }
void CTraceView::OnQueryCRChain5(){	OnQueryCRChain(4); }
void CTraceView::OnQueryCRChain6(){	OnQueryCRChain(5); }
void CTraceView::OnQueryCRChain7(){	OnQueryCRChain(6); }
void CTraceView::OnQueryCRChain8(){	OnQueryCRChain(7); }
void CTraceView::OnQueryCRChain9(){	OnQueryCRChain(8); }
void CTraceView::OnQueryCRChain10(){ OnQueryCRChain(9); }
void CTraceView::OnQueryPCChain(int idx)
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	if(!sitem)
		return;
	TRACEORDER *torder=(TRACEORDER*)ctc.GetItemData(sitem);
	if(!torder)
		return;
	int i=0;
	TRACEORDER *norder;
	for(norder=torder;norder;norder=norder->rtnext)
	{
		if(i==idx)
			break;
		i++;
	}
	if((!norder)&&(i==idx))
		norder=torder->vsoats;
	if(norder)
	{
		Cvsctest3View::newq=curq;
		Cvsctest3View::newq.asys=norder->asys;
		Cvsctest3View::newq.select="*";
		#ifdef SPLIT_ORD_DET
		Cvsctest3View::newq.from="ORDERS/DETAILS";
		#else
		Cvsctest3View::newq.from="ORDERS";
		#endif
		char wclause[1024]={0};
		sprintf(wclause,"AppInstID=='%s*' AND RootOrderID=='%s'",norder->asys,norder->rootorderid);
		if(*norder->orderdate)
			sprintf(wclause +strlen(wclause)," AND OrderDate==%s",norder->orderdate);
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		Cvsctest3View::newq.where=wclause;
		Cvsctest3View::newq.unique=false;
		Cvsctest3View::newq.istart=0;
		Cvsctest3View::newq.hist=true;
		#ifdef LIVE_DETAILS
		Cvsctest3View::newq.live=true;
		#else
		Cvsctest3View::newq.live=false;
		#endif
		Cvsctest3View::newq.iter=0;

		Cvsctest3Doc *pdoc=(Cvsctest3Doc *)GetDocument();
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
		CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("By RootOrderID",0);
		pdoc->SetDocTemplate(ldt);

		Cvsctest3View::newq.select="";
		Cvsctest3View::newq.from="";
		Cvsctest3View::newq.where="";
	}
}
void CTraceView::OnQueryPCChain1(){	OnQueryPCChain(0); }
void CTraceView::OnQueryPCChain2(){	OnQueryPCChain(1); }
void CTraceView::OnQueryPCChain3(){	OnQueryPCChain(2); }
void CTraceView::OnQueryPCChain4(){	OnQueryPCChain(3); }
void CTraceView::OnQueryPCChain5(){	OnQueryPCChain(4); }
void CTraceView::OnQueryPCChain6(){	OnQueryPCChain(5); }
void CTraceView::OnQueryPCChain7(){	OnQueryPCChain(6); }
void CTraceView::OnQueryPCChain8(){	OnQueryPCChain(7); }
void CTraceView::OnQueryPCChain9(){	OnQueryPCChain(8); }
void CTraceView::OnQueryPCChain10(){ OnQueryPCChain(9); }
void CTraceView::OnQueryOrder(int idx)
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	if(!sitem)
		return;
	TRACEORDER *torder=(TRACEORDER*)ctc.GetItemData(sitem);
	if(!torder)
		return;
	int i=0;
	TRACEORDER *norder;
	for(norder=torder;norder;norder=norder->rtnext)
	{
		if(i==idx)
			break;
		i++;
	}
	if((!norder)&&(i==idx))
		norder=torder->vsoats;
	if(norder)
	{
		Cvsctest3View::newq=curq;
		Cvsctest3View::newq.asys=norder->asys;
		Cvsctest3View::newq.select="*";
		#ifdef SPLIT_ORD_DET
		Cvsctest3View::newq.from="ORDERS/DETAILS";
		#else
		Cvsctest3View::newq.from="ORDERS";
		#endif
		char wclause[1024]={0};
		sprintf(wclause,"AppInstID=='%s*' AND ClOrdID=='%s'",norder->asys,norder->clordid);
		if(*norder->orderdate)
			sprintf(wclause +strlen(wclause)," AND OrderDate==%s",norder->orderdate);
		if(!theApp.clientregion.empty())
			sprintf(wclause +strlen(wclause)," AND Region=='%s'",theApp.clientregion.c_str());
		Cvsctest3View::newq.where=wclause;
		Cvsctest3View::newq.unique=false;
		Cvsctest3View::newq.istart=0;
		Cvsctest3View::newq.hist=true;
		#ifdef LIVE_DETAILS
		Cvsctest3View::newq.live=true;
		#else
		Cvsctest3View::newq.live=false;
		#endif
		Cvsctest3View::newq.iter=0;

		Cvsctest3Doc *pdoc=(Cvsctest3Doc *)GetDocument();
		CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
		CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("By ClOrdID",0);
		pdoc->SetDocTemplate(ldt);

		Cvsctest3View::newq.select="";
		Cvsctest3View::newq.from="";
		Cvsctest3View::newq.where="";
	}
}
void CTraceView::OnQueryOrder1(){ OnQueryOrder(0); }
void CTraceView::OnQueryOrder2(){ OnQueryOrder(1); }
void CTraceView::OnQueryOrder3(){ OnQueryOrder(2); }
void CTraceView::OnQueryOrder4(){ OnQueryOrder(3); }
void CTraceView::OnQueryOrder5(){ OnQueryOrder(4); }
void CTraceView::OnQueryOrder6(){ OnQueryOrder(5); }
void CTraceView::OnQueryOrder7(){ OnQueryOrder(6); }
void CTraceView::OnQueryOrder8(){ OnQueryOrder(7); }
void CTraceView::OnQueryOrder9(){ OnQueryOrder(8); }
void CTraceView::OnQueryOrder10(){ OnQueryOrder(9); }
static void RecurseExpandAll(CTreeCtrl& ctc, HTREEITEM sitem, UINT expand)
{
	ctc.Expand(sitem,expand);
	for(HTREEITEM citem=ctc.GetChildItem(sitem);citem;citem=ctc.GetNextSiblingItem(citem))
		RecurseExpandAll(ctc,citem,expand);
}
void CTraceView::OnExpandAll()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	ctc.LockWindowUpdate();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	if(sitem)
	{
		CString selname=ctc.GetItemText(sitem);
		if(selname.Left(5)=="Total")
		{
			for(HTREEITEM pitem=ctc.GetChildItem(0);pitem;pitem=ctc.GetNextSiblingItem(pitem))
				RecurseExpandAll(ctc,pitem,TVE_EXPAND);
		}
		else
			RecurseExpandAll(ctc,sitem,TVE_EXPAND);
	}
	ctc.UnlockWindowUpdate();
}
void CTraceView::OnCollapseAll()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	ctc.LockWindowUpdate();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	if(sitem)
	{
		CString selname=ctc.GetItemText(sitem);
		if(selname.Left(5)=="Total")
		{
			for(HTREEITEM pitem=ctc.GetChildItem(0);pitem;pitem=ctc.GetNextSiblingItem(pitem))
				RecurseExpandAll(ctc,pitem,TVE_COLLAPSE);
		}
		else
			RecurseExpandAll(ctc,sitem,TVE_COLLAPSE);
	}
	ctc.UnlockWindowUpdate();
}
void CTraceView::OnUpdateKey(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}
void CTraceView::OnKey()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	CString key=theApp.pMainFrame->GetKeyText();
	key.MakeUpper();
	// Search the index
	if(curq.from=="TRACEORDERS")
	{
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
		if(fitem)
			smsg.Format("Matching item found");
		else
			smsg.Format("Not found");
		theApp.pMainFrame->SetStatus(1,smsg);
	}
}
// F3 on the list
void CTraceView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar==VK_F3)
		OnKey();

	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}
void CTraceView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// Add your specialized code here and/or call the base class
	theApp.pMainFrame->SetStatus(1,smsg);

	__super::OnActivateView(bActivate, pActivateView, pDeactiveView);
}
void CTraceView::OnGroup(int idx)
{
	list<WORD>& psortlist=theApp.psortlist;
	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
	{
		WORD& psortcol=*sit;
		if(HIBYTE(psortcol)==idx)
		{
			psortcol=MAKEWORD(LOBYTE(psortcol)?0:1,HIBYTE(psortcol));
			GroupParents();
			break;
		}
	}
}
void CTraceView::OnGroup1(){ OnGroup(0); }
void CTraceView::OnGroup2(){ OnGroup(1); }
void CTraceView::OnGroup3(){ OnGroup(2); }
void CTraceView::OnGroup4(){ OnGroup(3); }
void CTraceView::OnGroup5(){ OnGroup(4); }
void CTraceView::OnGroup6(){ OnGroup(5); }
void CTraceView::OnGroup7(){ OnGroup(6); }
void CTraceView::OnGroup8(){ OnGroup(7); }
void CTraceView::OnGroup9(){ OnGroup(8); }
void CTraceView::OnGroup10(){ OnGroup(9); }
void CTraceView::OnGroup11(){ OnGroup(10); }
void CTraceView::OnGroup12(){ OnGroup(11); }
void CTraceView::OnGroup13(){ OnGroup(12); }
void CTraceView::OnGroup14(){ OnGroup(13); }
void CTraceView::OnGroup15(){ OnGroup(14); }
void CTraceView::OnGroup16(){ OnGroup(15); }
void CTraceView::OnGroup17(){ OnGroup(16); }
void CTraceView::OnGroup18(){ OnGroup(17); }
void CTraceView::OnGroup19(){ OnGroup(18); }
void CTraceView::OnGroup20(){ OnGroup(19); }
void CTraceView::OnGroup21(){ OnGroup(20); }
void CTraceView::OnGroup22(){ OnGroup(21); }
void CTraceView::OnGroup23(){ OnGroup(22); }
void CTraceView::OnGroupNone()
{
	list<WORD>& psortlist=theApp.psortlist;
	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
	{
		WORD& psortcol=*sit;
		psortcol=MAKEWORD(0,HIBYTE(psortcol));
	}
	GroupParents();
}
void CTraceView::OnGroupReorder()
{
	CGroupOrderDlg gdlg;
	list<WORD>& psortlist=theApp.psortlist;
	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
		gdlg.psortlist.push_back(*sit);
	if(gdlg.DoModal()==IDOK)
	{
		psortlist.swap(gdlg.psortlist);
		GroupParents();
	}
}
void CTraceView::OnRemoveFromTrace()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	if(sitem)
	{
		CString selname=ctc.GetItemText(sitem);
		CString wmsg;
		wmsg.Format("Confirm remove \"%s\"?",selname);
		if(MessageBox(wmsg,"Remove From Trace",MB_YESNO|MB_ICONWARNING)!=IDYES)
			return;
		HTREEITEM pitem=ctc.GetParentItem(sitem);
		ctc.DeleteItem(sitem);
		// No more children left
		while(pitem)
		{
			HTREEITEM nitem=ctc.GetParentItem(pitem);
			if(!ctc.GetChildItem(pitem))
				ctc.DeleteItem(pitem);
			pitem=nitem;
		}
		GroupParents();
	}
}
static int RecurseSaveTrace(HANDLE fhnd, CTreeCtrl& ctc, HTREEITEM pitem, int indent)
{
	CString selname=ctc.GetItemText(pitem);
	DWORD wbytes=0;
	if(indent>0)
	{
		char ibuf[256];
		memset(ibuf,' ',sizeof(ibuf));
		if(indent>sizeof(ibuf))
			indent=sizeof(ibuf);
		WriteFile(fhnd,ibuf,indent,&wbytes,0);
	}
	WriteFile(fhnd,(const char *)selname,selname.GetLength(),&wbytes,0);
	WriteFile(fhnd,"\r\n",2,&wbytes,0);
	if(ctc.GetItemState(pitem,TVIS_EXPANDED)&TVIS_EXPANDED)
	{
		for(HTREEITEM citem=ctc.GetChildItem(pitem);citem;citem=ctc.GetNextSiblingItem(citem))
			RecurseSaveTrace(fhnd,ctc,citem,indent +4);
	}
	return 0;
}
void CTraceView::OnSave(bool all)
{
	CTreeCtrl& ctc=GetTreeCtrl();
	HTREEITEM sitem=rselitem;//ctc.GetSelectedItem();
	char sname[MAX_PATH], spath[MAX_PATH],*sptr;
	sprintf(sname,"%s\\SysmonTempFiles",theApp.pcfg->RunPath.c_str());
	CreateDirectory(sname,0);
	sprintf(sname,"%s\\SysmonTempFiles\\trace.txt",theApp.pcfg->RunPath.c_str());
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
	if((all)||(!sitem))
	{
		for(HTREEITEM pitem=ctc.GetChildItem(0);pitem;pitem=ctc.GetNextSiblingItem(pitem))
			RecurseSaveTrace(fhnd,ctc,pitem,0);
	}
	else
		RecurseSaveTrace(fhnd,ctc,sitem,0);
	CloseHandle(fhnd);
	LaunchTextViewer(spath);
}
void CTraceView::OnSaveTrace()
{
	OnSave(false);
}
void CTraceView::OnSaveTraceAll()
{
	OnSave(true);
}
int CTraceView::LaunchTextViewer(const char *fpath)
{
	if(PathFileExists("C:\\Program Files\\TextPad 4\\textpad.exe"))
		ShellExecute(m_hWnd,"open","C:\\Program Files\\TextPad 4\\textpad.exe",fpath,0,SW_SHOW);
	else
		ShellExecute(m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
	return 0;
}

#ifdef OWNER_PAINT
void CTraceView::OnPaint()
{
	//CTreeView::OnPaint();
	// Add your specialized code here and/or call the base class
	CPaintDC pdc(this);
	pdc.SetBkMode(TRANSPARENT);
	CBrush pbrush;
	pbrush.CreateSolidBrush(RGB(255,150,150));
	CBrush *defbrush=pdc.SelectObject(&pbrush);
	//CPen ppen;
	//ppen.CreatePen(PS_SOLID,1,RGB(220,0,0));
	//CPen *defpen=pdc.SelectObject(&ppen);
	CFont pfont;
	pfont.CreateFont(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Arial");
	CFont *deffont=pdc.SelectObject(&pfont);

	CTreeCtrl& ctc=GetTreeCtrl();
	for(HTREEITEM pitem=ctc.GetFirstVisibleItem();pitem;pitem=ctc.GetNextVisibleItem(pitem))
	{
		CString pname=ctc.GetItemText(pitem);	
		RECT irect;
		//ctc.GetItemRect(pitem,&irect,false);
		ctc.GetItemRect(pitem,&irect,true);
		pdc.FillRect(&irect,&pbrush);
		pdc.TextOut(irect.left,irect.top,pname,pname.GetLength());
	}

	pdc.SelectObject(deffont);
	//pdc.SelectObject(defpen);
	pdc.SelectObject(defbrush);
}
#endif

static int RecurseTraceToOrders(list<string>& results, CTreeCtrl& ctc, HTREEITEM pitem)
{
	if(pitem)
	{
		TRACEORDER *torder=(TRACEORDER*)ctc.GetItemData(pitem);
		if(torder)
		{
			char dbuf[1024]={0};
			sprintf(dbuf,"%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1",
				torder->ainst,
				torder->clordid,
				torder->rootorderid,
				torder->firstclordid,
				torder->symbol,
				torder->account,
				torder->ecnordid,
				torder->clientid,
				torder->price,
				torder->side,
				torder->orderqty,
				torder->cumqty,
				torder->fillqty,
				torder->term,
				torder->highmsgtype,
				torder->highexectype,
				torder->transacttime,
				torder->orderloc,
				torder->connection,
				torder->clparentorderid,
				torder->routedorderid,
				torder->orderdate);
			results.push_back(dbuf);
		}
	}
	for(HTREEITEM citem=ctc.GetChildItem(pitem);citem;citem=ctc.GetNextSiblingItem(citem))
		RecurseTraceToOrders(results,ctc,citem);
	return 0;
}
void CTraceView::OnTraceToOrdersView()
{
	CTreeCtrl& ctc=GetTreeCtrl();
	Cvsctest3View::newq=curq;
	Cvsctest3View::newq.asys=curq.asys;
	Cvsctest3View::newq.select="TRACEORDERS";
	Cvsctest3View::newq.from="ORDERS/DETAILS";
	Cvsctest3View::newq.where=curq.where;
	Cvsctest3View::newq.unique=false;
	Cvsctest3View::newq.istart=0;
	Cvsctest3View::newq.hist=true;
	Cvsctest3View::newq.live=false;
	Cvsctest3View::newq.iter=0;
	RecurseTraceToOrders(Cvsctest3View::newq.results,ctc,0);
	Cvsctest3View::newq.nresults=Cvsctest3View::newq.results.size();

	Cvsctest3Doc *pdoc=(Cvsctest3Doc *)GetDocument();
	CMultiDocTemplate *ldt=(CMultiDocTemplate*)pdoc->SetDocTemplate(vDocTemplate);
	CMDIChildWndEx *pchild=theApp.pMainFrame->CreateNewWindow("TraceOrder",0);
	pdoc->SetDocTemplate(ldt);

	Cvsctest3View::newq.select="";
	Cvsctest3View::newq.from="";
	Cvsctest3View::newq.where="";
}
