// ViewServerProto.h : main header file for the ViewServerProto DLL
//
#ifndef _VIEWSERVERPROTOV2_H
#define _VIEWSERVERPROTOV2_H

#include "ViewServerProtoV1.h"

// Post-Nov 11
class VSCNotifyV2
	:public VSCNotifyV1
{
public:
	// For EncodeLoginRequest2
	virtual void VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)=0;
	virtual void VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)=0;
	// For EncodeSqlRequest2
	virtual void VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)=0;
	// For EncodeSqlIndexRequest2
	virtual void VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)=0;
};

class VSCodecV2
	:public VSCodecV1
{
public:
	VSCodecV2();
	~VSCodecV2();

	int Init(VSCNotifyV2 *pnotify2);
	int Shutdown();

	// For clients
	int DecodeReply(const char *rptr, int rlen, void *udata);
	// Server-to-server login
	int EncodeLoginRequest2(char *&rptr, int rlen, DWORD rid, const char *user, const char *pass);
	// SQL request with TTL for server-to-server proxy of queries
	int EncodeSqlRequest2(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl);
	// SqlIndex request with TTL for server-to-server proxy of queries
	int EncodeSqlIndexRequest2(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl);

	// For servers
	int DecodeRequest(const char *rptr, int rlen, void *udata);
	int EncodeLoginReply2(char *&rptr, int rlen, int rc, DWORD rid, const char *reason, const char *name, int wsdate);

protected:
	VSCNotifyV2 *pnotify2;
};

#endif//_VIEWSERVERPROTOV2_H
