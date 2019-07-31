// Proprietary file-based order database class
#ifndef _VSORDER_H
#define _VSORDER_H

#include <string>
#include <set>
#include <map>
using namespace std;

#ifdef USE_DISK_INDEX
#include "DiskIndex.h"
#else
#include "memindex.h"
#endif
#include "timeidx.h"

//DT9398
#define AUX_KEYS_MAX_NUM   6
#define AUX_KEYS_MAX_LEN   40
#define AUX_KEYS_DELIM     (0x28)

#pragma pack(push,1)

#ifndef INVALID_FILE_VALUE
#ifdef WIN32
#define INVALID_FILE_VALUE INVALID_HANDLE_VALUE
#else
#define INVALID_FILE_VALUE 0
#endif
#endif

class VSDetailFileNotify
{
public:
	virtual void VSDFError(int rc, const char *emsg)=0;
	virtual void VSDFEvent(int rc, const char *emsg)=0;
};
class VSDetailFile
{
public:
	VSDetailFile();
	~VSDetailFile();

	string fpath;		// File path
	string prefix;		// File key
	int wsdate;			// transaction date
	bool readonly;		// read-only
	short portno;		// USC portno
	char proto;			// protocol

	// File attributes
#ifdef WIN32
	HANDLE whnd;
	HANDLE rhnd;
	LARGE_INTEGER fsize,fend,aend;
#else
	FILE *whnd;
	FILE *rhnd;
	LONGLONG fsize,fend,aend;
#endif
	int GrowFile(LONGLONG minSize);

	// Read thread
#ifdef WIN32
	HANDLE rthnd;
	LARGE_INTEGER rend;
#else
	pthread_t rthnd;
	LONGLONG rend;
#endif

	// Two-phase initialization
	int Init(VSDetailFileNotify *pnotify, const char *dbpath, const char *prefix, int wsdate, bool readonly, short portno, char proto);
	void Shutdown();

	inline int LockWrite()
	{
		return (WaitForSingleObject(wmutex,INFINITE)==WAIT_OBJECT_0)?0:-1;
	}
	inline int UnlockWrite()
	{
		return ReleaseMutex(wmutex)?0:-1;
	}
	inline int LockRead()
	{
		return (WaitForSingleObject(rmutex,INFINITE)==WAIT_OBJECT_0)?0:-1;
	}
	inline int UnlockRead()
	{
		return ReleaseMutex(rmutex)?0:-1;
	}

protected:
	friend class OrderDB;

	// Multi-thread safe
	HANDLE rmutex;
	HANDLE wmutex;

	VSDetailFileNotify *pnotify;
};
typedef map<string,VSDetailFile*> DETAILFILEMAP;

// The in-memory VSOrder can be both parent and child. One for each FIX order 
// (not aggregated order for replace, cancels, etc.)
struct DETAILLOC
{
	LONGLONG offset;
	unsigned short flen;
	unsigned short portno;
};
// To support unlimited number of details per order, we'll have an extension record
#define VSDETEXT_RECSIZE 452
class VSDetExt
{
public:
	VSDetExt()
	{
		Reset();
	}
	inline void Reset()
	{
		// public items
		marker=0;
		rtype=0;
		rlen=0;
		OrderKey[0]=0;
		startIdx=0;
		dcnt=0;
		memset(ddata,0,sizeof(ddata));
		noff=-1;
		// protected items
		offset=-1;
		dirty=false;
		nextDetExt=0;
	}

	// The attributes in this section must be VSDETEXT_RECSIZE bytes
	unsigned int marker;		// 0xFDFDFDFD
	int rtype;					// 3-detext(v1)
	int rlen;					// Tot record length
	char OrderKey[40];			// ClOrdID(11)
	int startIdx;
	int dcnt;
	DETAILLOC ddata[32];
	LONGLONG noff;				// Next VSDetExt record offset

public:
	VSDetExt *nextDetExt;
	LONGLONG offset;
	bool dirty;
};

#define VSORDER_RECSIZE (452+AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN) //DT9398

struct VSRECHEAD
{
	unsigned int marker;		// 0xFDFDFDFD
	int rtype;
	int rlen;					// Tot record length including header
};

class VSOrder
{
public:
	VSOrder()
	{
		Reset();
	}

	// Basic order info
	inline void Reset()
	{
		// VSORDER_RECSIZE items
		memset(&marker,0,VSORDER_RECSIZE);
		rtype=0;
		rlen=0;
		AppInstID[0]=0;
		ClOrdID[0]=0;
		RootOrderID[0]=0;
		FirstClOrdID[0]=0;
		ClParentOrderID[0]=0;
		Symbol[0]=0;
		Price=0.0;
		Side=0;
		Account[0]=0;
		EcnOrderID[0]=0;
		ClientID[0]=0;
		OrderQty=0;
		CumQty=0;
		FillQty=0;
		term=0;
		HighMsgType=0;
#ifdef SPECTRUM
		HighExecType='A';
#else
		HighExecType=0;
#endif
		TransactTime=0;
		Connection[0]=0;
	#ifdef MULTI_DAY
		OrderDate=0;
	#endif
		//DT9398
		memset(AuxKey,0,AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN);
		dcnt=0;
		memset(ddata,0,sizeof(ddata));
		noff=-1;
		// Memory-only items
	#ifdef MULTI_DAY_ITEMLOC
		offset.wsdate=0;
	#endif
		offset=-1;
	#ifdef WIN32
		memset(&ovl,0,sizeof(OVERLAPPED));
	#endif
		freeNext=0;
		pendNext=0;
		dirty=false;
		pendWrite=false;
		deFirst=deLast=0;
		OrderKey[0]=0;
	}

	inline const char *GetRootOrderID(){ return RootOrderID; }
	inline const char *GetFirstClOrdID(){ return FirstClOrdID; }
	inline const char *GetClParentOrderID(){ return ClParentOrderID; }
	inline const char *GetOrderKey(){ return OrderKey; }
	inline const char *GetAppInstID(){ return AppInstID; }
	inline const char *GetClOrdID(){ return ClOrdID; }
	inline const char *GetSymbol(){ return Symbol; }
	inline double GetPrice(){ return Price; }
	inline char GetSide(){ return Side; }
	inline const char *GetAccount(){ return Account; }
	inline const char *GetEcnOrderID(){ return EcnOrderID; }
	inline const char *GetClientID(){ return ClientID; }
	inline int GetOrderQty(){ return OrderQty; }
	inline int GetCumQty(){ return CumQty; }
	inline int GetFillQty(){ return FillQty; }
	inline char GetHighMsgType(){ return HighMsgType; }
	inline char GetHighExecType(){ return HighExecType; }
	inline LONGLONG GetTransactTime(){ return TransactTime; }
	inline const char *GetConnection(){ return Connection; }
#ifdef MULTI_DAY
	int GetOrderDate(){ return OrderDate; }
	//int GetLocaleOrderDate(int bias);
#else
	inline int GetOrderDate(){ return 0; }
	//inline int GetLocaleOrderDate(int bias){ return 0; }
#endif
#ifdef MULTI_RECTYPES
	inline int GetType(){ return rtype; }
#endif
	//DT9398
	inline const char *GetAuxKey(unsigned int i){ return (AuxKey+AUX_KEYS_MAX_LEN*i); }  
	inline const char *GetAuxKey() { return AuxKey; }  
	inline int GetNumDetails()
	{
		if(dcnt>sizeof(ddata)/sizeof(DETAILLOC))
		{
			_ASSERT(false);
			return sizeof(ddata)/sizeof(DETAILLOC);
		}
		int tcnt=dcnt;
		for(VSDetExt *pde=deFirst;pde;pde=pde->nextDetExt)
			tcnt+=pde->dcnt;
		return tcnt; 
	}
	inline LONGLONG GetDetail(int i, unsigned short& UscPortNo, unsigned short& flen)
	{ 
		if(i<0)
			return -1;
		else if(i<sizeof(ddata)/sizeof(DETAILLOC))
		{
			UscPortNo=ddata[i].portno;
			flen=ddata[i].flen;
			return ddata[i].offset;
		}
		else
		{
			for(VSDetExt *pde=deFirst;pde;pde=pde->nextDetExt)
			{
				if((pde->startIdx<=i)&&(pde->startIdx +(int)(sizeof(pde->ddata)/sizeof(DETAILLOC))>i))
				{
					int didx=i -pde->startIdx;
					UscPortNo=pde->ddata[didx].portno;
					flen=pde->ddata[didx].flen;
					return pde->ddata[didx].offset;
				}
			}
			return -1;
		}
	}
	//DT9044
	inline bool UpdateDetailOffset(int i, LONGLONG offset)
	{ 
		if(i<0)
			return false;
		else if(i<sizeof(ddata)/sizeof(DETAILLOC))
		{
			ddata[i].offset=offset;
			dirty=true;
			return true;
		}
		else
		{
			for(VSDetExt *pde=deFirst;pde;pde=pde->nextDetExt)
			{
				if((pde->startIdx<=i)&&(pde->startIdx +(int)(sizeof(pde->ddata)/sizeof(DETAILLOC))>i))
				{
					int didx=i -pde->startIdx;
					pde->ddata[didx].offset=offset;
					dirty=true;
					return true;
				}
			}
			return false;
		}
	}
	inline bool IsTerminated(){ return term ?true :false; }
	inline bool IsDirty(){ return dirty; }
	inline ITEMLOC GetOffset(){ return offset; }

	inline void SetRootOrderID(const char *sval)
	{
		_ASSERT(strlen(sval)<sizeof(RootOrderID));
		if((*sval)&&(strcmp(sval,RootOrderID)))
		{
			strncpy(RootOrderID,sval,sizeof(RootOrderID)); dirty=true;
			RootOrderID[sizeof(RootOrderID) -1]=0;
		}
	}
	inline void SetFirstClOrdID(const char *sval)
	{
		_ASSERT(strlen(sval)<sizeof(FirstClOrdID));
		if((*sval)&&(strcmp(sval,FirstClOrdID)))
		{
			strncpy(FirstClOrdID,sval,sizeof(FirstClOrdID)); dirty=true;
			FirstClOrdID[sizeof(FirstClOrdID) -1]=0;
		}
	}
	inline void SetClParentOrderID(const char *sval)
	{
		_ASSERT(strlen(sval)<sizeof(ClParentOrderID));
		if((*sval)&&(strcmp(sval,ClParentOrderID)))
		{
			strncpy(ClParentOrderID,sval,sizeof(ClParentOrderID)); dirty=true;
			ClParentOrderID[sizeof(ClParentOrderID) -1]=0;
		}
	}
	inline void SetOrderKey(const char *okey)
	{
		_ASSERT(strlen(okey)<sizeof(OrderKey));
		if((*okey)&&(strcmp(okey,OrderKey)))
		{
			strncpy(OrderKey,okey,sizeof(OrderKey)); dirty=true;
			OrderKey[sizeof(OrderKey) -1]=0;
		}
	}
	inline void SetAppInstID(const char *aid)
	{
		_ASSERT(strlen(aid)<sizeof(AppInstID));
		if((*aid)&&(_stricmp(aid,AppInstID)))
		{
			strncpy(AppInstID,aid,sizeof(AppInstID)); dirty=true;
			AppInstID[sizeof(AppInstID) -1]=0;
		}
	}
	inline void SetClientID(const char *cid)
	{
		_ASSERT(strlen(cid)<sizeof(ClientID));
		if((*cid)&&(strcmp(cid,ClientID)))
		{
			strncpy(ClientID,cid,sizeof(ClientID)); dirty=true;
			ClientID[sizeof(ClientID) -1]=0;
		}
	}
	inline void SetClOrdID(const char *cloid)
	{
		_ASSERT(strlen(cloid)<sizeof(ClOrdID));
		if((*cloid)&&(strcmp(cloid,ClOrdID)))
		{
			strncpy(ClOrdID,cloid,sizeof(ClOrdID)); dirty=true;
			ClOrdID[sizeof(ClOrdID) -1]=0;
		}
	}
	inline void SetSymbol(const char *sym)
	{
		_ASSERT(strlen(sym)<sizeof(Symbol));
		if((*sym)&&(strcmp(sym,Symbol)))
		{
			strncpy(Symbol,sym,sizeof(Symbol)); dirty=true;
			Symbol[sizeof(Symbol) -1]=0;
		}
	}
	inline void SetPrice(double price)
	{
		if(price!=Price)
		{
			Price=price; dirty=true;
		}
	}
	inline void SetSide(char side)
	{
		if((side)&&(side!=Side))
		{
			Side=side; dirty=true;
		}
	}
	inline void SetAccount(const char *acct)
	{
		_ASSERT(strlen(acct)<sizeof(Account));
		if((*acct)&&(strcmp(acct,Account)))
		{
			strncpy(Account,acct,sizeof(Account)); dirty=true;
			Account[sizeof(Account) -1]=0;
		}
	}
	inline void SetEcnOrderID(const char *ecnoid)
	{
		_ASSERT(strlen(ecnoid)<sizeof(EcnOrderID));
		if((*ecnoid)&&(strcmp(ecnoid,EcnOrderID)))
		{
			strncpy(EcnOrderID,ecnoid,sizeof(EcnOrderID)); dirty=true;
			EcnOrderID[sizeof(EcnOrderID) -1]=0;
		}
	}
	inline void SetOrderQty(int qty)
	{
		if((qty>0)&&(qty!=OrderQty))
		{
			OrderQty=qty; dirty=true;
		}
	}
	inline void SetCumQty(int qty)
	{
		if((qty>0)&&(qty>CumQty))
		{
			CumQty=qty; dirty=true;
		}
	}
	inline void AddFillQty(int qty)
	{
		if(qty>0)
		{
			FillQty+=qty; dirty=true;
		}
	}
	static inline char GetMsgTypeTier(char mtype)
	{
		char tval=0;
		switch(mtype)
		{
		case 'D': tval=1; break;
		case 'G': tval=2; break;
		case 'F': tval=3; break;
		};
		return tval;
	}
	inline void SetHighMsgType(char mtype)
	{
		if(GetMsgTypeTier(mtype)>GetMsgTypeTier(HighMsgType)) // D,F,G
		{
			HighMsgType=mtype; dirty=true;
		}
	}
	static inline char GetExecTypeTier(char etype)
	{
		char tval=0;
		switch(etype)
		{
		case '0': tval=1; break;
		case '5': tval=2; break;
		case '1': tval=3; break;
		case '3': 
		case '4': 
		case '8': 
		case '9': tval=4; break;
		case '2': tval=5; break;
		};
		return tval;
	}
	inline void SetHighExecType(char etype)
	{
		char hval=GetExecTypeTier(HighExecType);
		char eval=GetExecTypeTier(etype);
		if(eval>=hval)
		{
			HighExecType=etype; dirty=true;
		}
	}
	inline void SetTransactTime(LONGLONG ts)
	{
		if((ts>0)&&((!TransactTime)||(ts<TransactTime)))
		{
			TransactTime=ts; dirty=true;
		}
	}
	inline void SetConnection(const char *conn)
	{
		_ASSERT(strlen(conn)<sizeof(Connection));
		if((*conn)&&(strcmp(conn,Connection)))
		{
			strncpy(Connection,conn,sizeof(Connection)); dirty=true;
			Connection[sizeof(Connection) -1]=0;
		}
	}
	//DT9398
	inline bool NeedAuxKey(unsigned int NumAuxKeys)
	{
		_ASSERT(NumAuxKeys<=AUX_KEYS_MAX_NUM);
		for(unsigned int i=0;i<NumAuxKeys;i++)
		{
			if(!AuxKey[i*AUX_KEYS_MAX_LEN])
				return true;
		}
		return false;
	}
	inline void SetAuxKey(unsigned int i, const char *akey)
	{
		_ASSERT(i<=AUX_KEYS_MAX_NUM);
		_ASSERT(strlen(akey)<AUX_KEYS_MAX_LEN);
		if((*akey)&&(strcmp(akey,AuxKey+AUX_KEYS_MAX_LEN*i)))
		{
			memset(AuxKey+AUX_KEYS_MAX_LEN*i,0,AUX_KEYS_MAX_LEN);
			strncpy(AuxKey+AUX_KEYS_MAX_LEN*i,akey,AUX_KEYS_MAX_LEN-1); dirty=true;
		}
	}
	inline void SetAuxKeys(const char *akey, unsigned int NumAuxKeys)
	{
		_ASSERT(NumAuxKeys<=AUX_KEYS_MAX_NUM);
		if(!akey[0])
			return;
		char tmp[AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN]={0};
		memcpy(tmp,akey,AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN-1);
		char* beg=tmp;
		unsigned int i=0;
		while(char* end=strchr(beg,AUX_KEYS_DELIM))
		{
			if(i>=NumAuxKeys)
				break;
			*end=0;
			SetAuxKey(i++,beg);
			beg=end+1;
		}
	}
	inline int AddDetExt(VSDetExt *pde)
	{
		if(deLast)
		{
			pde->startIdx=deLast->startIdx +sizeof(deLast->ddata)/sizeof(DETAILLOC);
			deLast->nextDetExt=pde; 
			if(pde->offset!=deLast->noff)
			{
				deLast->noff=pde->offset; dirty=deLast->dirty=true; 
			}
			deLast=pde;
		}
		else
		{
			pde->startIdx=sizeof(ddata)/sizeof(DETAILLOC);
			deFirst=deLast=pde;
			if(pde->offset!=noff)
			{
				noff=pde->offset; dirty=true;
			}
		}
		return 0;
	}
	inline int AddDetail(LONGLONG offset, unsigned short UscPortNo, unsigned short flen)
	{
		if(deLast)
		{
			if(deLast->dcnt>=sizeof(deLast->ddata)/sizeof(DETAILLOC))
				return -1;
			int didx=deLast->dcnt;
			deLast->ddata[didx].offset=offset;
			deLast->ddata[didx].portno=UscPortNo;
			deLast->ddata[didx].flen=flen;
			dirty=deLast->dirty=true; deLast->dcnt++;
			return deLast->startIdx +didx;
		}
		else
		{
			if(dcnt>=sizeof(ddata)/sizeof(DETAILLOC))
				return -1;
			int didx=dcnt;
			ddata[didx].offset=offset;
			ddata[didx].portno=UscPortNo;
			ddata[didx].flen=flen;
			dirty=true; dcnt++;
			return didx;
		}
	}
	inline void SetTerminated(bool tf)
	{
		if(tf!=(term?true:false))
		{
			term=tf; dirty=true;
		}
	}
#ifdef MULTI_DAY
	inline void SetOrderDate(int odate)
	{
		if(odate!=OrderDate)
		{
			OrderDate=odate; dirty=true;
		}
	}
#endif
#ifdef MULTI_RECTYPES
	inline void SetType(int otype)
	{
		if(rtype!=otype)
		{
			rtype=otype; dirty=true;
		}
	}
#endif

protected:
	friend class OrderDB;

	// The data in this section should be no more than VSORDER_RECSIZE bytes
	unsigned int marker;		// 0xFDFDFDFD
	int rtype;					// 1-order(v1), 4-iqaccount, 5-iqpos
	int rlen;					// Tot record length
	char AppInstID[20];			// AppInstID(11505)
	char ClOrdID[40];			// ClOrdID(11)
	char RootOrderID[40];		// tag 70129
	char FirstClOrdID[40];		// tag 5055
	char ClParentOrderID[40];	// tag 5035
	char Symbol[32];			// Symbol(55,65)
	double Price;				// LmtPrice(44)
	char Account[20];			// Account(1)
	char EcnOrderID[40];		// EcnOrderID(37)
	char ClientID[24];			// ClientID(109)
	int OrderQty;				// OrderQty(38)
	int CumQty;					// CumQty(14)
	int FillQty;				// Sum LastQty(31)
	char term;					// 0-live, 1-terminated
	char HighMsgType;			// Highest MsgType value (eg DFG)
	char HighExecType;			// Highest ExecType value (eg 0,1,2,4)
	char Side;					// Side(54)
	LONGLONG TransactTime;		// Earliest TransactTime (yyyymmddhhmmss)
	char Connection[48];		// Connection
#ifdef MULTI_DAY
	int OrderDate;				// yyyymmdd
#endif
	//DT9398
	char AuxKey[AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN]; // Generic keys
	int dcnt;					// Number of details
	DETAILLOC ddata[4];			// Detail offsets (default 4 per order to conserve space)
	LONGLONG noff;				// Next VSDetExt record offset for extra details

public:
	VSDetExt *deFirst,*deLast;
protected:
	// Members in this section don't get written to main record on disk
	ITEMLOC offset;
#ifdef WIN32
	OVERLAPPED ovl;
#endif
	VSOrder *freeNext;
	VSOrder *pendNext;
	bool dirty;
	bool pendWrite;
	char OrderKey[52];
};

// Multi-map index on RootOrderID to many child ITEMLOCs
#ifdef USE_DISK_INDEX
typedef DiskIndex VSINDEX;
#else
typedef MemIndex VSINDEX;
#endif
typedef TimeIndex TSINDEX;
#ifdef MULTI_DAY
typedef multimap<int,ITEMLOC> DATEINDEX;
#endif

// OrderDB users must implement this interface
class OrderDBNotify
{
public:
	virtual void ODBError(int ecode, const char *emsg)=0;
	virtual void ODBEvent(int ecode, const char *emsg)=0;
	//// For async I/O completion
	//virtual void ODBOrderCommitted(int rc, VSOrder *porder)=0;
	// When loading the database
#ifdef MULTI_DAY_HIST
	virtual void ODBIndexOrder(class AppSystem *asys, OrderDB *pdb, VSOrder *porder, LONGLONG offset)=0;
#else
	virtual void ODBIndexOrder(class AppSystem *asys, VSOrder *porder, LONGLONG offset)=0;
#endif
	virtual void ODBBusyLoad(LONGLONG offset, int lperc)=0;
	virtual void ODBOrderUnloaded(VSOrder *porder, LONGLONG offset)=0;
};

// my copy of GNU hash_map doesn't support LONGLONG hashing
typedef map<ITEMLOC,VSOrder*> CACHEDORDERMAP;

// A flat-file, multi-thread-safe order database
class OrderDB
	:public VSDetailFileNotify
{
public:
	OrderDB();
	~OrderDB();
	
	string fpath;		// Single database file
	int wsdate;			// Data file date
	bool readonly;
	int ORDER_CACHE_SIZE;	// Max size(MB) of order cache
	bool INDEX_ROOTORDERID;	// 1 to index orders by rootorderid
	bool INDEX_FIRSTCLORDID;// 1 to index orders by firstclordid
	bool INDEX_SYMBOL;		// 1 to index orders by symbol
	bool INDEX_ACCOUNT;		// 1 to index orders by account
	bool INDEX_ECNORDERID;	// 1 to index orders by ecnorderid
	bool INDEX_CLIENTID;	// 1 to index clientid
	bool INDEX_TRANSACTTIME;// 1 to index timestamp
	bool INDEX_FILLED_ORDERS;// 1 to index filled orders
	bool INDEX_OPEN_ORDERS;	// 1 to index open orders
	bool INDEX_CONNECTIONS;	// 1 to index by SCID(49),SSID(50),TCID(56),TSID(57)
#ifdef MULTI_DAY
	bool INDEX_ORDERDATE;
#endif
#ifdef MULTI_DAY_HIST
	int HISTORIC_DAYS;		// Number of historic days to load
#endif
	//DT9398
	unsigned int INDEX_AUXKEYS;		// Number of Aux Keys
	string AUXKEY_NAMES[AUX_KEYS_MAX_NUM];		// Index name
	int AUXKEY_TAGS[AUX_KEYS_MAX_NUM];			// Index tag number
#ifdef AUXKEY_EXPR
	string AUXKEY_ORD_EXPR[AUX_KEYS_MAX_NUM];
	string AUXKEY_EXEC_EXPR[AUX_KEYS_MAX_NUM];
#endif
	bool HAVE_FUSION_IO;	// When we're running on fusion I/O drive
	bool INDEX_CLPARENTORDERID; // 1 to index orders by clparentorderid
	//DT9491- Case sensitivity
	bool INDEX_CLORDID_CS;
	bool INDEX_ROOTORDERID_CS; 
	bool INDEX_FIRSTCLORDID_CS;
	bool INDEX_SYMBOL_CS;
	bool INDEX_ACCOUNT_CS;
	bool INDEX_ECNORDERID_CS;
	bool INDEX_CLIENTID_CS;
	bool INDEX_CONNECTIONS_CS;
	bool INDEX_CLPARENTORDERID_CS;
	bool INDEX_AUXKEYS_CS;

	// TODO: Make the tag-based index.ini to be able to add indices without changing code
	VSINDEX omap;		// ClOrdID (tag 11) disk location map
	VSINDEX pmap;		// RootOrderID (tag 70129) multimap
	VSINDEX fmap;		// FirstClOrdID (tag 5055) multimap
	VSINDEX symmap;		// Symbol (tag 55,65) multimap
	VSINDEX acctmap;	// Account (tag 1) multimap
	VSINDEX omap2;		// OrderID (tag 37) multimap
	VSINDEX cimap;		// ClientID (tag 109) multimap
	TSINDEX tsmap;		// Time map
	VSINDEX fomap;		// Filled orders map
	VSINDEX oomap;		// Open orders map
	VSINDEX cnmap;		// Connections map
	VSINDEX cpmap;		// ClParentOrderID (tag 5035) multimap
#ifdef MULTI_DAY
	DATEINDEX odmap;		// OrderDate multimap
#endif
	VSINDEX akmap[AUX_KEYS_MAX_NUM];// AuxKey map //DT9398

	// Two-phase initialization
	int Init(OrderDBNotify *pnotify, const char *dbdir, const char *dbname, int wsdate, bool readonly, bool& cxlInit, AppSystem *asys);
	inline int Init(OrderDBNotify *pnotify){ this->pnotify=pnotify; return 0; }
	int DelayInit(OrderDBNotify *pnotify, const char *dbdir, const char *dbname, int wsdate, bool readonly, bool& cxlInit, AppSystem *asys);
	int DoDelayInit();
	void Shutdown();
	void CopySettings(const OrderDB& odb,const char* sname);
	static int TruncateDatFile(const char *datPath);
	static int TruncateCsvFile(const char *datPath);

	// VSOrder allocation
	VSOrder *AllocOrder();
	void FreeOrder(VSOrder *porder);
	//void FreeOrders(VSOrder*& pendWritesFirst, VSOrder*& pendWritesLast);
	int WriteOrder(VSOrder *porder, ITEMLOC& offset);
	VSOrder *ReadOrder(const ITEMLOC& offset);

	// Detail allocation
	VSDetExt *AllocDetExt();
	void FreeDetExt(VSDetExt *pde);
	int WriteDetExt(VSDetExt *pde, LONGLONG& offset);
	VSDetExt *ReadDetExt(LONGLONG offset);
	// VSDetailFileNotify
	void VSDFError(int rc, const char *emsg);
	void VSDFEvent(int rc, const char *emsg);
	static const char *SizeStr(__int64 bigsize, bool byteUnits);
	// Detail file
#ifdef MULTI_DAY_HIST
	VSDetailFile *OpenDetailFile(const char *prefix, int portno, int proto, const char *dpath, bool readOnly, int wsdate);
	VSDetailFile *GetDetailFile(const char *prefix, int wsdate);
	VSDetailFile *GetDetailFile(int UscPortNo, int wsdate);
	//int DetailFileCount(int wsdate);
	DETAILFILEMAP& GetDetailFiles(){ return dfmap; }
	inline void GetFileReportStats(HANDLE& fhnd, LARGE_INTEGER& fsize, LARGE_INTEGER& fend)
	{ fhnd=this->fhnd; fsize=this->fsize; fend=this->fend; }
#else
	VSDetailFile *OpenDetailFile(const char *prefix, int portno, int proto, const char *dpath, bool readOnly);
	VSDetailFile *GetDetailFile(const char *prefix);
#endif
	int CloseDetailFile(VSDetailFile *dfile);
	int WriteDetailBlock(VSDetailFile *dfile, const char *bptr, int blen, LONGLONG& offset);
	int ReadDetail(VSDetailFile *dfile, LONGLONG offset, char *fptr, int flen);

	// Zmap file (VSDIST_HUB=0 only)
	int OpenZmapFile();
	int UpdateZmapFile(bool zmapReset=false);
	int CloseZmapFile();
	inline void SetZdirty(){ zdirty=true; }
	// External file (VSDIST_HUB=1 only)
#ifdef MULTI_DAY_HIST
	int OpenExtFile(int wsdate, bool delayInit);
#else
	int OpenExtFile();
#endif
	int UpdateExtFile();
	int CloseExtFile();
	inline void SetEdirty(){ edirty=true; }

	// Stats
	inline int GetMaxOrders(){ return noblocks *8192; }
	inline int GetNumOrdersPend(){ return nopend; }
	inline int GetNumMemOrders(){ return nomem; }
	inline LONGLONG GetMemOrderSize(){ return somem; }
	inline int GetMaxDetails(){ return ndblocks *8192; }
	inline int GetNumDetailsPend(){ return ndpend; }
	inline int GetNumMemDetails(){ return ndmem; }

	int AssociateOrder(VSOrder *porder, const ITEMLOC& oloc, DWORD& totOrderCnt, DWORD& totRootCnt);
	VSOrder *GetLRUOrder(ITEMLOC& oloc);
	// Make up for tags that don't appear in all messages
	int AssociateAccount(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateRootOrderID(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateFirstClOrdID(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateEcnOrderID(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateClientID(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateClParentOrderID(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateTransactTime(VSOrder *porder, const ITEMLOC& oloc);
#ifdef MULTI_DAY
	int AssociateOrderDate(VSOrder *porder, const ITEMLOC& oloc);
#endif
	int AssociateAuxKey(VSOrder *porder, const ITEMLOC& oloc);
	int AssociateSymbol(VSOrder *porder, const ITEMLOC& oloc);
	//int DisAssociateOrder(VSOrder *porder);
	int DisAssociateAccount(VSOrder *porder);
	int DisAssociateRootOrderID(VSOrder *porder);
	int DisAssociateFirstClOrdID(VSOrder *porder);
	int DisAssociateSymbol(VSOrder *porder);

	// Multi-thread safe
	inline int Lock()
	{
		return (WaitForSingleObject(mutex,INFINITE)==WAIT_OBJECT_0)?0:-1;
	}
	inline int Unlock()
	{
		return ReleaseMutex(mutex)?0:-1;
	}

protected:
	// Multi-thread safe
	HANDLE mutex;
	// Async notification
	OrderDBNotify *pnotify;
	// Flat-file store
#ifdef WIN32
	HANDLE fhnd;
	LARGE_INTEGER fsize,fend;
#else
	FILE *fhnd;
	LONGLONG fsize,fend;
#endif
	int GrowFile(LONGLONG minSize);

	// VSOrder allocation
	struct VSORDERBLOCK
	{
		VSOrder orders[8192];
		VSORDERBLOCK *next;
	};
	VSORDERBLOCK *oblocks;
	VSOrder *freeOrders;
	int noblocks;
	int nomem;
	int nopend;
	static LONGLONG somem;

	// Detail files
	int ndblocks;
	int ndmem;
	int ndpend;
	DETAILFILEMAP dfmap;

	// SizeStr
	static char SizeStr_zstrs[8][64];
	static int SizeStr_zidx;

	// Zmap file
	char zpath[MAX_PATH];
	FILE *zfp;
	bool zdirty;

	// Emap file
	char epath[MAX_PATH];
	FILE *efp;
	bool edirty;

	// Order cache map when no fusion I/O
	CACHEDORDERMAP comap;

	// HISTORIC_ONDEMAND load
	void *delayInitArgs;
};// ## bytes

#pragma pack(pop)

#endif//_VSORDER_H
