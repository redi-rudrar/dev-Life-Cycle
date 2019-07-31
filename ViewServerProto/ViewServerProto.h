// ViewServerProto.h : main header file for the ViewServerProto DLL
//
#ifndef _VIEWSERVERPROTO_H
#define _VIEWSERVERPROTO_H

#include "ViewServerProtoV2.h"


class VSCNotify
	:public VSCNotifyV2
{
public:
	// For EncodeDatRequest
	virtual void VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)=0;
	virtual void VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
};

class VSCodec
	:public VSCodecV2
{
public:
	VSCodec();
	~VSCodec();

	int Init(VSCNotify *pnotify);
	int Shutdown();

	// For clients
	int DecodeReply(const char *rptr, int rlen, void *udata);
	int EncodeDatRequest(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter);

	// For servers
	int DecodeRequest(const char *rptr, int rlen, void *udata);
	int EncodeDatReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, bool hist,
        char proto, short* pstrlen, const void **pstr, int& nstr, bool more, LONGLONG iter, const char *reason);


protected:
	VSCNotify *pnotify;
};

#endif//_VIEWSERVERPROTO_H
