
#ifndef _UNITTEST_VSCODECV2_H
#define _UNITTEST_VSCODECV2_H

#include "ViewServerProtoV2.h"
#include "UnitTest_VSCodecV1.h"

// For methods with more than 10 parameters
class mockVSCNotifyV2
{
public:
	struct mREPLYHEADER2
	{
		void *udata;
		DWORD rid;
	};
	virtual void mVSCNotifySqlIndexRequest2(mREPLYHEADER2 *pheader, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)=0;
};
class MockVSCNotifyV2 : public VSCNotifyV2, public MockVSCNotifyV1, public mockVSCNotifyV2
{
public:
	// VSCNotifyV2
	MOCK_METHOD5(VSCNotifyLoginRequest2, void(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen));
	MOCK_METHOD6(VSCNotifyLoginReply2, void(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate));
	MOCK_METHOD10(VSCNotifySqlRequest2, void(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl));
	//MOCK_METHOD11(VSCNotifySqlIndexRequest2, void(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl));
	MOCK_METHOD10(mVSCNotifySqlIndexRequest2, void(mREPLYHEADER2 *pheader, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl));
	virtual void VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
	{
		mREPLYHEADER2 hdr={udata,rid};
		mVSCNotifySqlIndexRequest2(&hdr,select,from,where,maxorders,unique,istart,idir,iter,ttl);
	}

	// MockVSCNotifyV1
	virtual void VSCNotifyError(int rc, const char *emsg)
	{ MockVSCNotifyV1::VSCNotifyError(rc,emsg); }
	virtual void VSCNotifyEvent(int rc, const char *emsg)
	{ MockVSCNotifyV1::VSCNotifyEvent(rc,emsg); }
	virtual void VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
	{ MockVSCNotifyV1::VSCNotifyLoginRequest(udata,rid,user,phash,phlen); }
	virtual void VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
	{ MockVSCNotifyV1::VSCNotifyLoginReply(udata,rc,rid,reason); }
	virtual void VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
	{ MockVSCNotifyV1::VSCNotifySqlRequest(udata,rid,select,from,where,maxorders,hist,live,iter); }
	virtual void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV1::VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nfix,more,iter,reason); }
	virtual void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV1::VSCNotifySqlDsvReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstr,nstr,more,iter,reason); }
	virtual void VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
	{ MockVSCNotifyV1::VSCNotifySqlIndexRequest(udata,rid,select,from,where,maxorders,unique,istart,idir,iter); }
	virtual void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV1::VSCNotifySqlIndexReply(udata,rc,rid,endcnt,totcnt,proto,pstr,nstr,more,iter,reason); }
	virtual void VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
	{ MockVSCNotifyV1::VSCNotifyCancelRequest(udata,rid,iter); }
	virtual void VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
	{ MockVSCNotifyV1::VSCNotifyDescribeRequest(udata,rid,from,maxrows,iter); }
	virtual void VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{ MockVSCNotifyV1::VSCNotifyDescribeReply(udata,rc,rid,endcnt,totcnt,pstr,nstr,more,iter,reason); }
};

#endif//_UNITTEST_VSCODECV2_H
