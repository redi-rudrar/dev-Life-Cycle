
#ifndef _VSDIST_H
#define _VSDIST_H

// General Design
// Each qsa-xxx "ViewServer" app receives drops for one application instance or
// application instances in the chain for one trading path.
// The qsa-xxx app persists, parses, and writes an update journal
// describing what occured in the FIX. For example:
// 
// [update journal]
// 1 alias QO C:\run\iqapi\qsa-order\qsa-order_20100716.fix
// 2 alias QOM C:\run\iqapi\qsa-order\qsa-order_manual_20100716.fix
// 3 alias QE C:\run\iqapi\qsa-exec\qsa-exec_20100716.fix
// 4 new CLOID1 100 QO 0
// 5 assoc CLOID1 ROOT1 ACCT1 DOM1 LOB1
// 6 new CLOID2 200 QO 200
// 7 assoc CLOID2 ROOT1 ACCT2 LOB2
// 8 new CLOID3 200 QO 400
// 9 assoc CLOID3 ROOT2 ACCT3
// 10 conf CLOID1 100 QO 600
// 11 conf CLOID2 200 QO 800
// 12 term CLOID1 0 QO 1000 (detail location: alias offset)
// 13 conf CLOID3 200 QO 1200
// 14 fill CLOID2 100 QE 0
// 15 fill CLOID2 100 QE 100
// 16 term CLOID2 200 QE 200
// 17 fill CLOID3 200 QE 300
// 18 term CLOID3 200 QE 400
// 
// The qsa-xxx only writes details to files (no orders).
// The central gbear server only writes orders (no details).
// The long detail file paths are given a short alias.
// Associations are made from order to 0+ groupings
// (i.e. parent order, accounts, domains, lines of businesses, etc.).
// new, conf, fill, and term order actions get notified to the
// full network of associations.
//
// The center gbear server should positively or negatively ack
// single or groups of messages. This allows for replay after application
// crash and restart.
//
// [update reply]
// 1 ack 1
// 2 ack 2
// 3 ack 3-9
// 4 ack 10-18
//
// Corrections and back office modifications should be supported.
// 
// 19 bust CLOID2 100 QE 500
// 20 correct CLOID3 QE 300 (old) 100 QE 600 (new)
// 21 disassoc CLOID2 ACCT2 LOB2
// 22 assoc CLOID2 ACCT3
//
// It may be useful for debugging to write the update journal to a file,
// but it's not necessary since the zmap already saves the last read
// point from the detail file, and the journal is just a high-level
// translation of the details.
// 
struct JPAGE
{
	int lstart;			// start line number
	int lend;			// end line number
	int pbeg;			// beginning offset in page
	int pend;			// end offset in page
	int lsent;			// last sent line number
	DWORD timeSent;		// last send time
	int nxmit;			// number of times transmitted
	char page[8192];	// page buffer
	LONGLONG rend;		// end read location of last entry in page
	JPAGE *next;		// list of pages
};

class VSDistJournal
{
public:
	VSDistJournal();
	~VSDistJournal();
	int Init(const char *alias);
	int Shutdown();

	//// 1 alias QO C:\run\iqapi\qsa-order\qsa-order_20100716.fix
	//int AddDetAlias(const char *DetAlias, const char *dfpath);
	//// 4 new CLOID1 100 QO 0
	//int AddNewOrder(const char *ClOrdID, int OrderQty, const char *DetAlias, LONGLONG off);
	//// 5 assoc CLOID1 ROOT1 ACCT1 DOM1 LOB1
	//int AddAssoc(const char *ClOrdID, const char *AssocList);
	//// 10 conf CLOID1 100 QO 600
	//int AddConfirm(const char *ClOrdID, int OrderQty, const char *DetAlias, LONGLONG off);
	//// 12 term CLOID1 0 QO 1000 (detail location: alias offset)
	//int AddTerm(const char *ClOrdID, int CumQty, const char *DetAlias, LONGLONG off);
	//// 14 fill CLOID2 100 QE 0
	//int AddFill(const char *ClOrdID, int LastQty, const char *DetAlias, LONGLONG off);

	char locAlias[16];
	JPAGE *firstPage,*lastPage;
	int AddLine(int lidx, const char *lptr, LONGLONG rpos);
	int RemLine(int lstart, int lend, LONGLONG& rpos);

	// Multi-thread safe
	HANDLE mutex;
	inline int Lock()
	{
		return (WaitForSingleObject(mutex,INFINITE)==WAIT_OBJECT_0)?0:-1;
	}
	inline int Unlock()
	{
		return ReleaseMutex(mutex)?0:-1;
	}

	inline LONGLONG GetMemUsage(){ return pmem +bmem; }
	inline void SetBackupMemUsage(LONGLONG val){ bmem=val; }
	inline int GetCount(){ return lcnt; }

protected:
	int lcnt;				// end line number
	LONGLONG pmem;			// page mem usage
	LONGLONG bmem;			// backup memusage
	#ifdef DEBUG_JOURNAL
	string fpath;			// debug vsdist journal
	FILE *fp;
	int next;
	friend class ViewServer;
	#endif
};

// Clients and servers must implement this interface
class VSDistNotify
{
public:
	virtual void VSDNotifyError(int rc, const char *emsg)=0;
	virtual void VSDNotifyEvent(int rc, const char *emsg)=0;
	// Journal page transfer
	virtual void VSDNotifyJPageRequest(void *udata, DWORD rid, JPAGE *jpage, bool retransmit)=0;
	virtual void VSDNotifyJpageReply(void *udata, int rc, DWORD rid, int lstart, int lend)=0;
	// Detail file alias
	virtual void VSDNotifyAliasRequest(void *udata, DWORD rid, const char *prefix, const char *dpath, int proto, int wsdate)=0;
};

// Clients and servers must instantiate this object
class VSDistCodec
{
public:
	VSDistCodec();
	~VSDistCodec();

	int Init(VSDistNotify *pnotify);
	int Shutdown();

	// For clients
	int DecodeReply(const char *rptr, int rlen, void *udata);
	// Journal page transfer
	int EncodeJPageRequest(char *&rptr, int rlen, DWORD rid, JPAGE *jpage, bool retransmit);
	int EncodeAliasRequest(char *&rptr, int rlen, DWORD rid, const char *prefix, const char *dpath, int proto, int wsdate);
	// TODO: Add schema definition message

	// For servers
	int DecodeRequest(const char *rptr, int rlen, void *udata);
	int EncodeJPageReply(char *&rptr, int rlen, int rc, DWORD rid, int lstart, int lend);

protected:
	VSDistNotify *pnotify;
	#ifdef DEBUG_JOURNAL
	string fpath;			// debug vshub journal
	FILE *fp;
	#endif
};

#endif//_VSDIST_H
