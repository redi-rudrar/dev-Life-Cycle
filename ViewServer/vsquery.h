
// SQL-like order query
#ifndef _VSQUERY_H
#define _VSQUERY_H

#include "bfix.h"
#include "vsorder.h"
#include "feedtask.h"
#include "ViewServerProto.h"
using namespace viewserver;

#ifdef MULTI_DAY_ITEMLOC
typedef set<ITEMLOC> ITEMLOCSET;
#else
typedef set<LONGLONG> ITEMLOCSET;
#endif

// VSQuery::NarrowScope pre-parses query where conditions that map to data indices
#include "exprtok.h"
typedef list<ExprTok*> CONDLIST;
struct EvalHint
{
	VSOrder *porder;
	FIXINFO *pfix;
	list<string> AuxKeyNames;  //DT9398
	class Account *pacc;
#ifdef IQDNS
	class IQUser *puser;
	struct tdVarsIdRec *vrec;
	class IQDomain *pdom;
	class IQDomain *pdomt;
	struct tdDnsInfoEx *psrv;
#endif
};
class VSQueryScope
{
public:
	VSQueryScope()
		:etok()
		,EqOrderLoc(-1)
		,CondRegion()
		,CondAppSystem()
		,CondAppInstID()
		,CondClOrdID()
		,CondRootOrderID()
		,CondFirstClOrdID()
		,CondClParentOrderID()
		,CondSymbol()
		//,CondPrice()
		//,CondSide()
		,CondAccount()
		,CondEcnOrderID()
		,CondClientID()
		,CondHighMsgType()
		,CondHighExecType()
		,CondOrderQty()
		,CondCumQty()
		,CondFillQty()
		,CondOpenOrder()
		,CondTermOrder()
		,CondOrderLoc()
		,CondTransactTime()
		,CondConnection()
	#ifdef MULTI_DAY
		,CondOrderDate()
	#endif
		,CondAcctName()
		,CondAcctType()
	#ifdef IQDNS
		,CondUserDomain()
		,CondUserName()
		,CondVarsidKey()
		,CondDomainRoot()
		//,CondHost()
	#endif
	{
	#ifdef MULTI_DAY
		memset(multikey,0,sizeof(multikey));
	#endif
		//DT9398
		for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
		{
			CondAuxKeys[i].erase(CondAuxKeys[i].begin(),CondAuxKeys[i].end());
		}
	}

	inline int ValueInAuxKeys(const char* val)
	{
		int result=-1;
		for(list<string>::iterator itr=AuxKeyNames.begin();itr!=AuxKeyNames.end();itr++)
		{
			result++;
			if(!strcmp(val,itr->c_str()))
				return result;
		}
		return -1;
	};
	inline int CondAuxKeySize(int idx)
	{
		if(idx>AUX_KEYS_MAX_NUM)
			return 0;
		return (int)CondAuxKeys[idx].size();
	};
	inline CONDLIST* CondAuxKeySearch(int& AuxKeyIndex)
	{
		for(unsigned int i=0;i<AUX_KEYS_MAX_NUM;i++)
		{
			if(CondAuxKeySize(i))
			{
				AuxKeyIndex=i;
				return &CondAuxKeys[i];
			}
		}
		return 0;
	};

	ExprTok etok;
	LONGLONG EqOrderLoc;		// OrderLoc==
	list<string> AuxKeyNames;   // DT9398
	// Count on number of conditions per attribute.
	CONDLIST CondRegion;
	CONDLIST CondAppSystem;
	CONDLIST CondAppInstID;
	CONDLIST CondClOrdID;
	CONDLIST CondRootOrderID;
	CONDLIST CondFirstClOrdID;
	CONDLIST CondClParentOrderID;
	CONDLIST CondSymbol;
	//CONDLIST CondPrice;
	//CONDLIST CondSide;
	CONDLIST CondAccount;
	CONDLIST CondEcnOrderID;
	CONDLIST CondClientID;
	CONDLIST CondHighMsgType;
	CONDLIST CondHighExecType;
	CONDLIST CondOrderQty;
	CONDLIST CondCumQty;
	CONDLIST CondFillQty;
	CONDLIST CondOpenOrder;
	CONDLIST CondTermOrder;
	CONDLIST CondOrderLoc;
	CONDLIST CondTransactTime;
	CONDLIST CondConnection;
#ifdef MULTI_DAY
	CONDLIST CondOrderDate;
#endif
	CONDLIST CondAuxKeys[AUX_KEYS_MAX_NUM]; //DT9398
	CONDLIST CondAcctName;
	CONDLIST CondAcctType;
#ifdef IQDNS
	CONDLIST CondUserDomain;
	CONDLIST CondUserName;
	CONDLIST CondVarsidKey;
	CONDLIST CondDomainRoot;
	//CONDLIST CondHost;
#endif
#ifdef MULTI_DAY
	char multikey[256];
#endif

	static const char *GetEqStrVal(ExprTok *cond);
#ifdef MULTI_DAY
	static LONGLONG GetEqIntVal(ExprTok *cond);
#endif
};

// VSQuery.results item can store a FIX message or DSV string
class VSQueryResult
{
public:
	VSQueryResult()
		:rfix()
		,obuf(0)
        ,DataLength(0)
		//,hist(false)
		,rtime(0)
	{
	}
	~VSQueryResult()
	{
		if(rfix.fci)
		{
			delete rfix.fci; rfix.fci=0;
		}
		if(rfix.sbuf)
		{
			delete rfix.sbuf; rfix.sbuf=0; rfix.slen=0;
		}
		if(obuf)
		{
			delete obuf; obuf=0;
		}
	}

	FIXINFO rfix;			// FIX msg reference
	char *obuf;				// Order result buffer
    int DataLength;         // Data length for binary data
	//bool hist;			// Historic or live result (not needed because we have separate hresults and lresults)
	DWORD rtime;			// Result time
};
typedef list<VSQueryResult*> VSRESULTLIST;

typedef list<class AppSystem*> APPSYSLIST;
typedef list<class AppInstance*> APPINSTLIST;
typedef map<string,class AppSystem*> APPSYSMAP;
typedef map<string,class Account*> ACCTMAP;
#ifdef REDIOLEDBCON
typedef map<string,pair<string,class Account*>> TACCTMAP;
typedef map<string,string> TUSERMAP;
#endif
typedef list<class Account*> ACCTLIST;
#ifdef IQDNS
typedef map<string,class IQUser *> USERMAP;
typedef map<string,struct tdVarsIdRec *> VARSIDMAP;
typedef list<string> DOMLIST;
typedef map<string,struct tdDnsInfoEx> IQSMAP;
#endif

//DT10472
typedef enum _KEYTYPE
{
        KEY_UNKNOWN=0,
        KEY_APPSYSTEM=1,
        KEY_APPINSTID=2,
        KEY_CLORDID=3,
        KEY_ROOTORDERID=4,
        KEY_FIRSTCLORDID=5,
        KEY_CLPARENTORDERID=6,
        KEY_SYMBOL=7,
        KEY_ACCOUNT=8,
        KEY_ECNORDERID=9,
        KEY_CLIENTID=10,
        KEY_HIGHMSGTYPE=11,
        KEY_HIGHEXECTYPE=12,
        KEY_ORDERQTY=13,
        KEY_CUMQTY=14,
        KEY_FILLQTY=15,
        KEY_OPENORDER=16,
        KEY_TERM=17,
        KEY_TRANSACTTIME=18,
        KEY_CONNECTION=19,
        KEY_ORDERLOC=20,
        KEY_AUXKEY1=30,
        KEY_AUXKEY2=31,
        KEY_AUXKEY3=32,
        KEY_AUXKEY4=33,
        KEY_AUXKEY5=34,
        KEY_AUXKEY6=35,
} KEYTYPE;

typedef map<string,ITEMLOCSET> CQRESULTLISTSET;

typedef enum _WHERE_OPS
{
        OPS_UNKNOWN=0,
        OPS_AND=1,
        OPS_OR=2,
        OPS_NONE=3,
        OPS_NOT_AND=4,
        OPS_NOT_OR=5,
} WHERE_OPS;

typedef enum _QUERY_DATA_TYPE
{
        QDT_NON_BINARY=0,
        QDT_BINARY_BTR=1,
        QDT_BINARY_IQOS=2,
		#ifdef IQSMP
		QDT_BINARY_OTHER=3
		#endif
} QUERY_DATA_TYPE;

class WhereSegment
{
public:
        WhereSegment(string seg, WHERE_OPS op, APPSYSMAP* asmap);
        string Segment;
        WHERE_OPS oper;
        CQRESULTLISTSET cqResultSet;
};

class VSQueryLive;
typedef map<TASKENTRY*,VSQueryLive*> LiveQueryMap;
//DT10472

// Generic SQL-like query that supports searching enormous data population and paging enormous result sets
class VSQuery
{
public:
	VSQuery()
		// Connection info
		:pmod(0)
		,fuser(0)
		,rid(0)
		,PortType(-1)
		,PortNo(-1)
		,fp(0)
		// Codec optimization
		,vsc(0)
		// Query details
		,select()
		,from()
		,where()
		,unique(false)
		,maxorders(0)
		,maxbrowse(0)
		,hist(false)
		,live(false)
		,ttl(1)
		,tstart(0)
		// Result iteration
		,qscope()
		,hresults()
		,lresults()
		,rcnt(0)
		,tpos(0)
		,spos(0)
		,supos(0)
		,suval()
		,kmatch()
		,kts(0)
		,koffset(-1)
	#ifdef MULTI_DAY
		,kdate(0)
	#endif
		,morehist(true)
		,eiter(0)
		,tcontinue(0)
		,scontinue(0)
		,asys(0)
		,ainst(0)
		,pidx(0)
		,midx(0)
		,tidx(0)
	#ifdef MULTI_DAY
		,didx(0)
	#endif
		,ailist()
		,iit()
		,miit()
		,tiit()
	#ifdef MULTI_DAY
		,diit()
		,odset()
	#endif
		,aiit()
		,aoit()
		,asit()
		,acc(0)
		,acit()
		,aclist()
		,aclit()
		,ptask(0)
		// Query forwarding
		,FwdPortType(-1)
		,FwdPortNo(-1)
		,ikey1(0)
		,crid(0)
		,uiter(0)
	#ifdef IQDNS
		,puser(0)
		,uit()
		,vrec(0)
		,vit()
		,pdom(0)
		,dit()
		,dlist()
		,psrv(0)
		,sit()
	#endif
		,binarydatareq(false) //DT9044
        ,binarydatatype(QDT_NON_BINARY)
		,binarydatalen(0) //DT9044
		,CaseSen(false)  //DT9491
        ,complexquery(false)
		,tzbias(0)
	{
		memset(rbuf,0,sizeof(rbuf));
		memset(efix,0,sizeof(efix));
	}
	~VSQuery();
	static int NarrowScope(VSQueryScope *qscope, const char *where, ExprTokNotify *pnotify, char reason[256]);
	static int RecurseScope(VSQueryScope *qscope, ExprTok *etok, ExprTokNotify *pnotify, char reason[256]);
	int FilterOrder(VSOrder *porder, ExprTokNotify *pnotify);
	int FilterFixResult(FIXINFO *pfix, bool hist, ExprTokNotify *pnotify, bool checkWhere=true);
	#ifdef IQSMP
	int FilterSmpResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify);
	#endif
	int FilterCldropResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify);
	int FilterClbackupResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify);
//#ifdef IQDNS
//	int FilterDnsReplResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify);
//#endif
	int FilterOrderResult(VSOrder *porder, bool hist, ExprTokNotify *pnotify);
	int FilterAccount(Account *acc, ExprTokNotify *pnotify);
	int FilterAccountResult(Account *acc, bool hist, ExprTokNotify *pnotify);
	#ifdef IQDNS
	int FilterUser(class IQUser *puser, ExprTokNotify *pnotify);
	int FilterUserResult(class IQUser *puser, bool hist, ExprTokNotify *pnotify);
	int FilterVarsid(struct tdVarsIdRec *vrec, ExprTokNotify *pnotify);
	int FilterVarsidResult(struct tdVarsIdRec *vrec, bool hist, ExprTokNotify *pnotify);
	int FilterDomrel(class IQDomain *pdom, ExprTokNotify *pnotify);
	int FilterDomrelResult(class IQDomain *pdom, bool hist, ExprTokNotify *pnotify);
	int FilterDomtree(class IQDomain *pdom, ExprTokNotify *pnotify);
	int FilterDomtreeResult(class IQDomain *pdom, bool hist, ExprTokNotify *pnotify);
	int FilterServers(struct tdDnsInfoEx *psrv, ExprTokNotify *pnotify);
	int FilterServersResult(struct tdDnsInfoEx *psrv, bool hist, ExprTokNotify *pnotify);
	#endif
	int ProxyFixResult(bool hist, int proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason);
	int ProxyDsvResult(bool hist, int proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason);
	//DT9044
	int ProxyDatResult(bool hist, int proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason);

	class ViewServer *pmod;
	// Connection info
	FeedUser *fuser;		// Query user
	int rid;				// Query request id
	int PortType;			// User port type
	int PortNo;				// User port number
	FILE *fp;				// WS_UMR result file
	// Codec optimization
	VSCodec *vsc;			// Protocol codec
	char rbuf[16380];		// Codec buffer
	FIXINFO efix[20];
	// Query details
	string select;			// Select clause
	string from;			// From clause
	string where;			// Where clause
	bool unique;
	int maxorders;			// Query page limit for this iteration
	int maxbrowse;
	bool hist;				// Historic
	bool live;				// Live
	int ttl;				// Time to live
	DWORD tstart;			// Query start time
	bool binarydatareq;     // DT9044:Binary data request
    QUERY_DATA_TYPE binarydatatype;
	short binarydatalen;    // DT9044:Binary data result length
	// Result iteration
	VSQueryScope qscope;	// Query dependencies
	VSRESULTLIST hresults;	// Historic results
	VSRESULTLIST lresults;	// Live results (no buffering supported)
	int rcnt;				// Total number of results
	int tpos;				// Last position to be searched
	int spos;				// Position of last row searched
	int supos;				// First position of last unique value searched
	string suval;			// Last unique row value corresponding to 'supos'
	string kmatch;			// String index key match value
	LONGLONG kts;			// Time index match value
	LONGLONG koffset;		// Offset match value
	int tzbias;				// Locale time zone bias
#ifdef MULTI_DAY
	int kdate;				// OrderDate match value
#endif
	bool morehist;			// More historic to iterate
	int eiter;				// External iterator handle
	DWORD tcontinue;		// Last continue time
	int scontinue;			// Number searched since last continue
	AppSystem *asys;		// App system
	AppInstance *ainst;		// App instance
	VSINDEX *pidx;			// Single index
	VSINDEX *midx;			// Multi index
	TSINDEX *tidx;			// Time index
#ifdef MULTI_DAY
	DATEINDEX *didx;		// Date index
	ITEMLOCSET odset;		// Join set on OrderDate
#endif
	APPINSTLIST ailist;		// Additional app instances for AppInstID index
	VSINDEX::iterator iit;  // Internal hash iterator
	VSINDEX::m_iterator miit;// Internal multimap iterator
	TSINDEX::m_iterator tiit;// Internal timemap iterator
#ifdef MULTI_DAY
	DATEINDEX::iterator diit;//Internal date iterator
#endif
	APPINSTLIST::iterator aiit; // Internal AppInstID list iterator
	ITEMLOCSET::iterator aoit; // Internal AppInstID order set iterator
	APPSYSMAP::iterator asit; // Internal APPSYSLIST system iterator
	Account *acc;			// Account
	ACCTMAP::iterator acit; // Internal account map iterator
	ACCTLIST aclist;		// Specific account list
	ACCTLIST::iterator aclit;// Specific account list iterator
	TASKENTRY *ptask; 		// Live 'taskMgr' task
#ifdef IQDNS
	class IQUser *puser;	// Internal User
	USERMAP::iterator uit;	// Internal user map iterator
	struct tdVarsIdRec *vrec;// Internal Varsid
	VARSIDMAP::iterator vit;// Internal varsid map iterator
	class IQDomain *pdom;	// Internal domain
	DOMLIST::iterator dit;	// Internal domain list iterator
	DOMLIST dlist;			// Internal domain list
	struct tdDnsInfoEx *psrv;// Internal server
	IQSMAP::iterator sit;	// Internal server map iterator
#endif
	// Query forwarding
	int FwdPortType;
	int FwdPortNo;
	AppSystem *ikey1;
	int crid;
	int uiter;
	bool CaseSen;  //DT9491

	//DTxxxx
	bool LiveQueryKey(string& key, KEYTYPE& qt);
	bool SetLiveQueryKey(CONDLIST& cl, string& keyStr, KEYTYPE& qt, KEYTYPE Type);

//DT10472
public:
	bool complexquery;
	CQRESULTLISTSET cqResultSet;
	CQRESULTLISTSET::iterator cqResultSetItr;
	bool ComplexQueryResultAdd(CQRESULTLISTSET* cqrs, string appsys, LONGLONG& loc);
	bool ComplexQueryResultAdd(CQRESULTLISTSET* cqrs, string appsys, ITEMLOCSET* s);

	LiveQueryMap LiveQueries;
//DT10472
};

// Map external iterator value to VSQuery structure per client connection
class VSQUERYMAP :public map<int,VSQuery*>
{
public:
	VSQUERYMAP()
		:eiterNext(0)
		,rgname()
		,wsdate(0)
	{
	}

	int eiterNext;			// Next external iterator handle value
	string rgname;
	int wsdate;
};

#endif//_VSQUERY_H
