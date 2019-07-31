// ViewServerProto.h : main header file for the ViewServerProto DLL
//
#ifndef _VIEWSERVERPROTOV1_H
#define _VIEWSERVERPROTOV1_H

#include "bfix.h"

// Define to retire proof-of-concept queries 09/22/10
//#define RETIRE_POC_QUERIES

// Clients and servers must implement this interface
class VSCNotifyV1
{
public:
	virtual void VSCNotifyError(int rc, const char *emsg)=0;
	virtual void VSCNotifyEvent(int rc, const char *emsg)=0;
	// For EncodeLoginRequest
	virtual void VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)=0;
	virtual void VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)=0;
	// For EncodeSqlRequest
	virtual void VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)=0;
	virtual void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)=0;
	virtual void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
	// For EncodeSqlIndexRequest
	virtual void VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)=0;
	virtual void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
	// For EncodeCancelRequest
	virtual void VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)=0;
	// For EncodeDescribeRequest
	virtual void VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)=0;
	virtual void VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
};

// Clients and servers must instantiate this object
class VSCodecV1
{
public:
	VSCodecV1();
	~VSCodecV1();

	virtual int Init(VSCNotifyV1 *pnotify1);
	virtual int Shutdown();

	// For clients
	virtual int DecodeReply(const char *rptr, int rlen, void *udata);
	// Authenticate and authorize
	int EncodeLoginRequest(char *&rptr, int rlen, DWORD rid, const char *user, const char *pass);
	// SQL-like FIX query
	int EncodeSqlRequest(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);
	// SQL-like index query that supports browsing
	int EncodeSqlIndexRequest(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter);
	// Cancel 'more' operation
	int EncodeCancelRequest(char *&rptr, int rlen, DWORD rid, LONGLONG iter);
	// SQL describe virtual tables query
	int EncodeDescribeRequest(char *&rptr, int rlen, DWORD rid, const char *from, int maxrows, LONGLONG iter);

	// For servers
	virtual int DecodeRequest(const char *rptr, int rlen, void *udata);
	int EncodeLoginReply(char *&rptr, int rlen, int rc, DWORD rid, const char *reason);
	int EncodeSqlFixReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int& nfix, bool more, LONGLONG iter, const char *reason);
	int EncodeSqlDsvReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int& nstr, bool more, LONGLONG iter, const char *reason);
	int EncodeSqlIndexReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int& nstr, bool more, LONGLONG iter, const char *reason);
	int EncodeDescribeReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int& nstr, bool more, LONGLONG iter, const char *reason);

protected:
	VSCNotifyV1 *pnotify1;
};

#endif//_VIEWSERVERPROTOV1_H
