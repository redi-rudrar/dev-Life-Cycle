
#ifndef _SFIX_H
#define _SFIX_H

#include "bfix.h"

#include <string>
#include <list>
using namespace std;

// FIX drop copy client session
// FIX Session notification
class DropClientNotify
{
public:
	virtual void DCSendMsg(void *udata, const char *msg, int mlen)=0;
	virtual void DCLoggedIn(void *udata, int hbinterval)=0;
	virtual void DCLoggedOut(void *udata, const char *text)=0;
	virtual LONGLONG DCWriteFix(void *udata, char mdir, FIXINFO *pfix)=0;
	virtual LONGLONG DCReadFix(void *udata, LONGLONG roff, char& mdir, FIXINFO *pfix)=0;
};
class DropClient
{
public:
	DropClient();
	~DropClient();

	int Init(const char *sids, int iseq, int oseq, bool passive, DropClientNotify *pnotify, void *udata);
	void Shutdown();
	int GetLastSeqNos(const char *fpath);
	int ScanLastSeqNos(const char *fpath);

	int Login(int hbsec);
	int Logout(const char *text);
	int Heartbeat(const char *text);
	int ResendReq(int start, int stop);
	int GapFill(int start, int stop);
	int SeqReset(int nstop);
	int ResendReply(int start, int stop);
	virtual int DecodeFix(const char *sfix, int slen);
	string GetConnID();
	inline int WaitSize(){ return (int)msgq.size(); }
	inline bool IsLoggedIn(){ return loginTime?true:false; }
	virtual int SendFixMsg(FIXINFO& ifx);

	string scid;			// SenderCompID
	string ssid;			// SenderSubID
	string tcid;			// TargetCompID
	string tsid;			// TargetSubID
	int loginTime;			// login time
	bool passive;			// not a sequence-enforced session
	int iseq;				// last in seq no
	int oseq;				// last out seq no
	bool dirty;				// seq no changed
	DWORD ihbinterval;		// incoming heartbeat interval
	DWORD ihblast;			// last incoming hb time
	DWORD ohbinterval;		// outgoing heartbeat interval
	DWORD ohblast;			// last outgoing hb time

	// Multi-gap handling
	struct INBLOCK
	{
		int begin;	// inclusive
		int end;	// exclusive
	};
	typedef list<INBLOCK> INLIST;
	INLIST inlist;
	DWORD waitgapfill;
	enum TrackInCode
	{
	TRKI_UNKNOWN,
	TRKI_IN_SEQ,
	TRKI_NEW_GAP,
	TRKI_REPLAYED,
	TRKI_GAP_CLOSED
	};
	TrackInCode TrackIn(int mseq);
	int GetGap(INBLOCK& gap, int skip);

protected:
	DropClientNotify *pnotify;
	void *udata;
	CRITICAL_SECTION mux;
	map<int,LONGLONG> replayMap;

	typedef pair<int,char*> QMSG;
	list<QMSG> msgq;		// queued messages
	virtual int EncodeFixMsg(char *&rptr, int rlen, FIXINFO& ifx);
};

#ifdef FIX_SERVER
class DropServer :public DropClient
{
public:
	DropServer();

	virtual int DecodeFix(const char *sfix, int slen);

protected:
	virtual int EncodeFixMsg(char *&rptr, int rlen, FIXINFO& ifx);
};
#endif

#endif//_SFIX_H
