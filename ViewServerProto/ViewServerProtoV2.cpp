// ViewServerProto.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "ViewServerProtoV2.h"
#ifdef WIN32
#include <crtdbg.h>
#endif
#include "md5.h"

VSCodecV2::VSCodecV2()
	:pnotify2(0)
{
}
VSCodecV2::~VSCodecV2()
{
}
int VSCodecV2::Init(VSCNotifyV2 *pnotify2)
{
	if(VSCodecV1::Init(pnotify2)<0)
		return -1;
	this->pnotify2=pnotify2;
	return 0;
}
int VSCodecV2::Shutdown()
{
	VSCodecV1::Shutdown();
	pnotify2=0;
	return 0;
}

// For clients
// Authenticate and authorize
int VSCodecV2::EncodeLoginRequest2(char *&rptr, int rlen, DWORD rid, const char *user, const char *pass)
{
	if(!pnotify2)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x0A;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// user
	short ulen=(short)strlen(user)+1;
	if(rend -rptr<2+ulen)
		return -1;
	memcpy(rptr,&ulen,2); rptr+=2;
	memcpy(rptr,user,ulen); rptr+=ulen;
	// phash
	if(rend -rptr<16)
		return -1;
	MD5_CTX hash;
	MD5Init(&hash);
	unsigned char pbuf[32],dbuf[16];
	memset(pbuf,0,sizeof(pbuf));
	strncpy((char*)pbuf,pass,sizeof(pbuf));
	MD5Update(&hash,pbuf,sizeof(pbuf));
	MD5Final(dbuf,&hash);
	memcpy(rptr,dbuf,16); rptr+=16;
	return (int)(rptr -rbegin);
}
// SQL request with TTL for server-to-server proxy of queries
int VSCodecV2::EncodeSqlRequest2(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
{
	if((!pnotify1)||(!select)||(!from))
		return -1;
	if(!where) where="";
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x0B;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// select
	short slen=(short)strlen(select)+1;
	if(rend -rptr<2 +slen)
		return -1;
	memcpy(rptr,&slen,2); rptr+=2;
	memcpy(rptr,select,slen); rptr+=slen;
	// from
	short flen=(short)strlen(from)+1;
	if(rend -rptr<2 +flen)
		return -1;
	memcpy(rptr,&flen,2); rptr+=2;
	memcpy(rptr,from,flen); rptr+=flen;
	// where
	short wlen=(short)strlen(where)+1;
	if(rend -rptr<2 +wlen)
		return -1;
	memcpy(rptr,&wlen,2); rptr+=2;
	memcpy(rptr,where,wlen); rptr+=wlen;
	// maxorders 
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&maxorders,4); rptr+=4;
	// hist
	if(rend -rptr<1)
		return -1;
	*rptr=hist?1:0; rptr++;
	// live
	if(rend -rptr<1)
		return -1;
	*rptr=live?1:0; rptr++;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	// ttl
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&ttl,4); rptr+=4;
	return (int)(rptr -rbegin);
}
// SqlIndex request with TTL for server-to-server proxy of queries
int VSCodecV2::EncodeSqlIndexRequest2(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
{
	if((!pnotify1)||(!select)||(!from))
		return -1;
	if(!where) where="";
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x0C;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// select
	short slen=(short)strlen(select)+1;
	if(rend -rptr<2 +slen)
		return -1;
	memcpy(rptr,&slen,2); rptr+=2;
	memcpy(rptr,select,slen); rptr+=slen;
	// from
	short flen=(short)strlen(from)+1;
	if(rend -rptr<2 +flen)
		return -1;
	memcpy(rptr,&flen,2); rptr+=2;
	memcpy(rptr,from,flen); rptr+=flen;
	// where
	short wlen=(short)strlen(where)+1;
	if(rend -rptr<2 +wlen)
		return -1;
	memcpy(rptr,&wlen,2); rptr+=2;
	memcpy(rptr,where,wlen); rptr+=wlen;
	// maxorders 
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&maxorders,4); rptr+=4;
	// unique
	if(rend -rptr<1)
		return -1;
	*rptr=unique?1:0; rptr++;
	// istart
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&istart,4); rptr+=4;
	// idir
	if(rend -rptr<1)
		return -1;
	*rptr=idir<0?-1:1; rptr++;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	// ttl
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&ttl,4); rptr+=4;
	return (int)(rptr -rbegin);
}
int VSCodecV2::DecodeReply(const char *rptr, int rlen, void *udata)
{
	if(!pnotify2)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	// EncodeLoginReply2
	if(cmd==0x0A)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// rc
		int rc=-1;
		if(rend -rptr<4)
			return -1;
		memcpy(&rc,rptr,4); rptr+=4;
		// relen
		if(rend -rptr<2)
			return -1;
		short relen=0;
		memcpy(&relen,rptr,2); rptr+=2;
		// reason
		if(rend -rptr<relen)
			return -1;
		const char *reason="";
		if(relen>0)
		{
			reason=rptr; rptr+=relen;
		}
		// nlen
		if(rend -rptr<2)
			return -1;
		short nlen=0;
		memcpy(&nlen,rptr,2); rptr+=2;
		// name
		if(rend -rptr<nlen)
			return -1;
		const char *name="";
		if(nlen>0)
		{
			name=rptr; rptr+=nlen;
		}
		// wsdate
		if(rend -rptr<4)
			return -1;
		int wsdate=0;
		memcpy(&wsdate,rptr,4); rptr+=4;

		pnotify2->VSCNotifyLoginReply2(udata,rc,rid,reason,name,wsdate);
		return (int)(rptr -rbegin);
	}
	else
	{
		rptr=rbegin;
		return VSCodecV1::DecodeReply(rptr,rlen,udata);
	}
	return (int)(rptr -rbegin);
}

// For servers
int VSCodecV2::EncodeLoginReply2(char *&rptr, int rlen, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
{
	if(!pnotify2)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x0A;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// rc
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rc,4); rptr+=4;
	// reason
	short rtlen=(short)strlen(reason) +1;
	if(rend -rptr<2+rtlen)
		return -1;
	memcpy(rptr,&rtlen,2); rptr+=2;
	memcpy(rptr,reason,rtlen); rptr+=rtlen;
	// name
	short nlen=(short)strlen(name) +1;
	if(rend -rptr<2+nlen)
		return -1;
	memcpy(rptr,&nlen,2); rptr+=2;
	memcpy(rptr,name,nlen); rptr+=nlen;
	// wsdate
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&wsdate,4); rptr+=4;
	return (int)(rptr -rbegin);
}
int VSCodecV2::DecodeRequest(const char *rptr, int rlen, void *udata)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	// EncodeLoginRequest
	if(cmd==0x0A)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// ulen
		if(rend -rptr<2)
			return -1;
		short ulen=0;
		memcpy(&ulen,rptr,2); rptr+=2;
		// user
		if(rend -rptr<ulen)
			return -1;
		if(ulen>255)
			return -1;
		const char *user=rptr; rptr+=ulen;
		// phash
		if(rend -rptr<16)
			return -1;
		unsigned char phash[16];
		memset(phash,0,sizeof(phash));
		memcpy(phash,rptr,16);

		pnotify2->VSCNotifyLoginRequest2(udata,rid,user,phash,16);
	}
	// EncodeSqlRequest2
	else if(cmd==0x0B)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// slen
		if(rend -rptr<2)
			return -1;
		short slen=0;
		memcpy(&slen,rptr,2); rptr+=2;
		// select
		if(rend -rptr<slen)
			return -1;
		if(slen>255)
			return -1;
		const char *select=rptr; rptr+=slen;
		// flen
		if(rend -rptr<2)
			return -1;
		short flen=0;
		memcpy(&flen,rptr,2); rptr+=2;
		// from
		if(rend -rptr<flen)
			return -1;
		if(flen>255)
			return -1;
		const char *from=rptr; rptr+=flen;
		// wlen
		if(rend -rptr<2)
			return -1;
		short wlen=0;
		memcpy(&wlen,rptr,2); rptr+=2;
		// where
		if(rend -rptr<wlen)
			return -1;
		if(wlen>255)
			return -1;
		const char *where=rptr; rptr+=wlen;
		// maxorders
		if(rend -rptr<4)
			return -1;
		int maxorders=0;
		memcpy(&maxorders,rptr,4); rptr+=4;
		// hist
		if(rend -rptr<1)
			return -1;
		bool hist=*rptr?true:false; rptr++;
		// live
		if(rend -rptr<1)
			return -1;
		bool live=*rptr?true:false; rptr++;
		// iter
		if(rend -rptr<8)
			return -1;
		LONGLONG iter;
		memcpy(&iter,rptr,8); rptr+=8;
		// ttl
		if(rend -rptr<4)
			return -1;
		int ttl=0;
		memcpy(&ttl,rptr,4); rptr+=4;

		pnotify2->VSCNotifySqlRequest2(udata,rid,select,from,where,maxorders,hist,live,iter,ttl);
	}
	// EncodeSqlIndexRequest2
	else if(cmd==0x0C)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// slen
		if(rend -rptr<2)
			return -1;
		short slen=0;
		memcpy(&slen,rptr,2); rptr+=2;
		// select
		if(rend -rptr<slen)
			return -1;
		if(slen>255)
			return -1;
		const char *select=rptr; rptr+=slen;
		// flen
		if(rend -rptr<2)
			return -1;
		short flen=0;
		memcpy(&flen,rptr,2); rptr+=2;
		// from
		if(rend -rptr<flen)
			return -1;
		if(flen>255)
			return -1;
		const char *from=rptr; rptr+=flen;
		// wlen
		if(rend -rptr<2)
			return -1;
		short wlen=0;
		memcpy(&wlen,rptr,2); rptr+=2;
		// where
		if(rend -rptr<wlen)
			return -1;
		if(wlen>255)
			return -1;
		const char *where=rptr; rptr+=wlen;
		// maxorders
		if(rend -rptr<4)
			return -1;
		int maxorders=0;
		memcpy(&maxorders,rptr,4); rptr+=4;
		// unique
		if(rend -rptr<1)
			return -1;
		bool unique=*rptr?true:false; rptr++;
		// istart
		if(rend -rptr<4)
			return -1;
		int istart=-1;
		memcpy(&istart,rptr,4); rptr+=4;
		// idir
		if(rend -rptr<1)
			return -1;
		char idir=1;
		idir=*rptr; rptr++;
		// iter
		if(rend -rptr<8)
			return -1;
		LONGLONG iter;
		memcpy(&iter,rptr,8); rptr+=8;
		// ttl
		if(rend -rptr<4)
			return -1;
		int ttl=0;
		memcpy(&ttl,rptr,4); rptr+=4;

		pnotify2->VSCNotifySqlIndexRequest2(udata,rid,select,from,where,maxorders,unique,istart,idir,iter,ttl);
	}
	else
	{
		rptr=rbegin;
		return VSCodecV1::DecodeRequest(rptr,rlen,udata);
	}
	return (int)(rptr -rbegin);
}
