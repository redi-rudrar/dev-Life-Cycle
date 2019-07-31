
#ifndef _UNITTEST_VSCODECV3_H
#define _UNITTEST_VSCODECV3_H

#include "ViewServerProto.h"
#include "UnitTest_VSCodecV2.h"

// For methods with more than 10 parameters
class mockVSCNotifyV3
{
public:
	struct mREPLYHEADER3
	{
		void *udata;
		int rc;
		DWORD rid;
		int endcnt;
		int totcnt;
	};
	virtual void mVSCNotifyDatReply(mREPLYHEADER3 *pheader, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
};
class MockVSCNotifyV3 : public VSCNotify, public MockVSCNotifyV2, public mockVSCNotifyV3
{
public:
	// VSCNotify
	MOCK_METHOD9(VSCNotifyDatRequest, void(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter));
	//MOCK_METHOD13(VSCNotifyDatReply, void(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
	MOCK_METHOD9(mVSCNotifyDatReply, void(mREPLYHEADER3 *pheader, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
	virtual void VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{
		mREPLYHEADER3 hdr={udata,rc,rid,endcnt,totcnt};
		mVSCNotifyDatReply(&hdr,hist,proto,pstrlen,pstr,nstr,more,iter,reason);
	}

	// MockVSCNotifyV2
	virtual void VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
	{ MockVSCNotifyV2::VSCNotifyLoginRequest2(udata,rid,user,phash,phlen); }
	virtual void VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
	{ MockVSCNotifyV2::VSCNotifyLoginReply2(udata,rid,rid,reason,name,wsdate); }
	virtual void VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
	{ MockVSCNotifyV2::VSCNotifySqlRequest2(udata,rid,select,from,where,maxorders,hist,live,iter,ttl); }
	virtual void VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
	{ MockVSCNotifyV2::VSCNotifySqlIndexRequest2(udata,rid,select,from,where,maxorders,unique,istart,idir,iter,ttl); }

	// MockVSCNotifyV1
	virtual void VSCNotifyError(int rc, const char *emsg)
	{ MockVSCNotifyV2::VSCNotifyError(rc,emsg); }
	virtual void VSCNotifyEvent(int rc, const char *emsg)
	{ MockVSCNotifyV2::VSCNotifyEvent(rc,emsg); }
	virtual void VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
	{ MockVSCNotifyV2::VSCNotifyLoginRequest(udata,rid,user,phash,phlen); }
	virtual void VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
	{ MockVSCNotifyV2::VSCNotifyLoginReply(udata,rc,rid,reason); }
	virtual void VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
	{ MockVSCNotifyV2::VSCNotifySqlRequest(udata,rid,select,from,where,maxorders,hist,live,iter); }
	virtual void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV2::VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nfix,more,iter,reason); }
	virtual void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV2::VSCNotifySqlDsvReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstr,nstr,more,iter,reason); }
	virtual void VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
	{ MockVSCNotifyV2::VSCNotifySqlIndexRequest(udata,rid,select,from,where,maxorders,unique,istart,idir,iter); }
	virtual void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV2::VSCNotifySqlIndexReply(udata,rc,rid,endcnt,totcnt,proto,pstr,nstr,more,iter,reason); }
	virtual void VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
	{ MockVSCNotifyV2::VSCNotifyCancelRequest(udata,rid,iter); }
	virtual void VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
	{ MockVSCNotifyV2::VSCNotifyDescribeRequest(udata,rid,from,maxrows,iter); }
	virtual void VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV2::VSCNotifyDescribeReply(udata,rc,rid,endcnt,totcnt,pstr,nstr,more,iter,reason); }
};

#endif//_UNITTEST_VSCODECV2_H
