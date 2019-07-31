
#ifndef _UNITTEST_VSCODECV1_H
#define _UNITTEST_VSCODECV1_H

#include "ViewServerProtoV1.h"

// For methods with more than 10 parameters
class mockVSCNotifyV1
{
public:
	struct mREPLYHEADER
	{
		void *udata;
		int rc;
		DWORD rid;
		int endcnt;
		int totcnt;
	};
	virtual void mVSCNotifySqlFixReply(mREPLYHEADER *pheader, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)=0;
	virtual void mVSCNotifySqlDsvReply(mREPLYHEADER *pheader, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
	virtual void mVSCNotifySqlIndexReply(mREPLYHEADER *pheader, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)=0;
};
class MockVSCNotifyV1 : public VSCNotifyV1, public mockVSCNotifyV1
{
public:
	MOCK_METHOD2(VSCNotifyError, void(int rc, const char *emsg));
	MOCK_METHOD2(VSCNotifyEvent, void(int rc, const char *emsg));
	MOCK_METHOD5(VSCNotifyLoginRequest, void(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen));
	MOCK_METHOD4(VSCNotifyLoginReply, void(void *udata, int rc, DWORD rid, const char *reason));
	MOCK_METHOD9(VSCNotifySqlRequest, void(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter));
	//MOCK_METHOD12(VSCNotifySqlFixReply, void(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason));
	MOCK_METHOD8(mVSCNotifySqlFixReply, void(mREPLYHEADER *pheader, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason));
	virtual void VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
	{
		mREPLYHEADER hdr={udata,rc,rid,endcnt,totcnt};
		mVSCNotifySqlFixReply(&hdr,hist,proto,pfix,nfix,more,iter,reason);
	}
	//MOCK_METHOD12(VSCNotifySqlDsvReply, void(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
	MOCK_METHOD8(mVSCNotifySqlDsvReply, void(mREPLYHEADER *pheader, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
	virtual void VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{
		mREPLYHEADER hdr={udata,rc,rid,endcnt,totcnt};
		mVSCNotifySqlDsvReply(&hdr,hist,proto,pstr,nstr,more,iter,reason);
	}
	MOCK_METHOD10(VSCNotifySqlIndexRequest, void(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter));
	//MOCK_METHOD11(VSCNotifySqlIndexReply, void(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
	MOCK_METHOD7(mVSCNotifySqlIndexReply, void(mREPLYHEADER *pheader, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
	virtual void VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
	{
		mREPLYHEADER hdr={udata,rc,rid,endcnt,totcnt};
		mVSCNotifySqlIndexReply(&hdr,proto,pstr,nstr,more,iter,reason);
	}
	MOCK_METHOD3(VSCNotifyCancelRequest, void(void *udata, DWORD rid, LONGLONG iter));
	MOCK_METHOD5(VSCNotifyDescribeRequest, void(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter));
	MOCK_METHOD10(VSCNotifyDescribeReply, void(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason));
};

#endif//_UNITTEST_VSCODECV1_H
