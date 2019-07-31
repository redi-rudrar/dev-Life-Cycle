// ViewServerProtoV1.cpp : Interface for first Nov 11 release
//

#include "stdafx.h"
#include "ViewServerProtoV1.h"
#include <crtdbg.h>
#include "md5.h"

VSCodecV1::VSCodecV1()
	:pnotify1(0)
{
}
VSCodecV1::~VSCodecV1()
{
}
int VSCodecV1::Init(VSCNotifyV1 *pnotify1)
{
	this->pnotify1=pnotify1;
	return 0;
}
int VSCodecV1::Shutdown()
{
	pnotify1=0;
	return 0;
}
// Authenticate and authorize
int VSCodecV1::EncodeLoginRequest(char *&rptr, int rlen, DWORD rid, const char *user, const char *pass)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x07;
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
// SQL-like query with any combination of tags
int VSCodecV1::EncodeSqlRequest(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	if((!pnotify1)||(!select)||(!from))
		return -1;
	if(!where) where="";
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x04;
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
	return (int)(rptr -rbegin);
}
// SQL-like query with any combination of tags
int VSCodecV1::EncodeSqlIndexRequest(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
{
	if((!pnotify1)||(!select)||(!from))
		return -1;
	if(!where) where="";
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x05;
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
	return (int)(rptr -rbegin);
}
int VSCodecV1::EncodeCancelRequest(char *&rptr, int rlen, DWORD rid, LONGLONG iter)
{
	if(!iter)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x06;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	return (int)(rptr -rbegin);
}
int VSCodecV1::EncodeDescribeRequest(char *&rptr, int rlen, DWORD rid, const char *from, int maxrows, LONGLONG iter)
{
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x09;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// from
	short flen=(short)strlen(from)+1;
	if(rend -rptr<2 +flen)
		return -1;
	memcpy(rptr,&flen,2); rptr+=2;
	memcpy(rptr,from,flen); rptr+=flen;
	// maxrows 
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&maxrows,4); rptr+=4;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	return (int)(rptr -rbegin);
}
int VSCodecV1::DecodeRequest(const char *rptr, int rlen, void *udata)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	// cmds 0x01,0x02,0x03 retired
	// EncodeSqlRequest
	if(cmd==0x04)
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

		pnotify1->VSCNotifySqlRequest(udata,rid,select,from,where,maxorders,hist,live,iter);
	}
	// EncodeSqlIndexRequest
	else if(cmd==0x05)
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

		pnotify1->VSCNotifySqlIndexRequest(udata,rid,select,from,where,maxorders,unique,istart,idir,iter);
	}
	// EncodeCancelRequest
	else if(cmd==0x06)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// iter
		if(rend -rptr<8)
			return -1;
		LONGLONG iter;
		memcpy(&iter,rptr,8); rptr+=8;

		pnotify1->VSCNotifyCancelRequest(udata,rid,iter);
	}
	// EncodeLoginRequest
	else if(cmd==0x07)
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

		pnotify1->VSCNotifyLoginRequest(udata,rid,user,phash,16);
	}
	// EncodeDescribeRequest
	else if(cmd==0x09)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
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
		// maxrows
		if(rend -rptr<4)
			return -1;
		int maxrows=0;
		memcpy(&maxrows,rptr,4); rptr+=4;
		// iter
		if(rend -rptr<8)
			return -1;
		LONGLONG iter;
		memcpy(&iter,rptr,8); rptr+=8;

		pnotify1->VSCNotifyDescribeRequest(udata,rid,from,maxrows,iter);
	}
	else
	{
		_ASSERT(false);
		return -1;
	}
	return (int)(rptr -rbegin);
}

int VSCodecV1::EncodeLoginReply(char *&rptr, int rlen, int rc, DWORD rid, const char *reason)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x07;
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
	return (int)(rptr -rbegin);
}
int VSCodecV1::EncodeSqlFixReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int& nfix, bool more, LONGLONG iter, const char *reason)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x04;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// rc
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rc,4); rptr+=4;
	// part
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&endcnt,4); rptr+=4;
	// nparts
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&totcnt,4); rptr+=4;
	// hist
	if(rend -rptr<1)
		return -1;
	*rptr=hist?1:0; rptr++;
	// proto TODO: Each detail could be different protocol, but this will work for now
	if(rend -rptr<1)
		return -1;
	*rptr=proto; rptr++;
	// more
	if(rend -rptr<1)
		return -1;
	char *pmore=rptr; *rptr=more?1:0; rptr++;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	// reason
	short relen=reason?(short)strlen(reason) +1:0;
	if(rend -rptr<2 +relen)
		return -1;
	memcpy(rptr,&relen,2); rptr+=2;
	if(reason)
	{
		memcpy(rptr,reason,relen); rptr+=relen;
	}
	// nfix
	if(rend -rptr<4)
		return -1;
	char *pnfix=rptr;
	memcpy(rptr,&nfix,4); rptr+=4;
	for(int i=0;i<nfix;i++)
	{
		// flen
		short flen=pfix[i].llen;
		if(rend -rptr<2+flen)
		{
			if(i<1)
				return -1;
			*pmore=1; nfix=i; memcpy(pnfix,&nfix,4);
			break;
		}
		memcpy(rptr,&flen,2); rptr+=2;
		// FIX msg
		memcpy(rptr,pfix[i].fbuf,flen); rptr+=flen;
	}
	return (int)(rptr -rbegin);
}
int VSCodecV1::EncodeSqlDsvReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int& nstr, bool more, LONGLONG iter, const char *reason)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x08;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// rc
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rc,4); rptr+=4;
	// part
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&endcnt,4); rptr+=4;
	// nparts
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&totcnt,4); rptr+=4;
	// hist
	if(rend -rptr<1)
		return -1;
	*rptr=hist?1:0; rptr++;
	// proto TODO: Each detail could be different protocol, but this will work for now
	if(rend -rptr<1)
		return -1;
	*rptr=proto; rptr++;
	// more
	if(rend -rptr<1)
		return -1;
	char *pmore=rptr; *rptr=more?1:0; rptr++;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	// reason
	short relen=reason?(short)strlen(reason) +1:0;
	if(rend -rptr<2 +relen)
		return -1;
	memcpy(rptr,&relen,2); rptr+=2;
	if(reason)
	{
		memcpy(rptr,reason,relen); rptr+=relen;
	}
	// nstr
	if(rend -rptr<4)
		return -1;
	char *pnstr=rptr;
	memcpy(rptr,&nstr,4); rptr+=4;
	for(int i=0;i<nstr;i++)
	{
		// flen
		short flen=(short)strlen(pstr[i])+1;
		if(rend -rptr<2+flen)
		{
			if(i<1)
				return -1;
			*pmore=1; nstr=i; memcpy(pnstr,&nstr,4);
			break;
		}
		memcpy(rptr,&flen,2); rptr+=2;
		// dsv string
		memcpy(rptr,pstr[i],flen); rptr+=flen;
	}
	return (int)(rptr -rbegin);
}
int VSCodecV1::EncodeSqlIndexReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int& nstr, bool more, LONGLONG iter, const char *reason)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x05;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// rc
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rc,4); rptr+=4;
	// part
	if(rend -rptr<4)
		return -1;
	char *ppart=rptr; memcpy(rptr,&endcnt,4); rptr+=4;
	// nparts
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&totcnt,4); rptr+=4;
	// proto TODO: Each detail could be different protocol, but this will work for now
	if(rend -rptr<1)
		return -1;
	*rptr=proto; rptr++;
	// more
	if(rend -rptr<1)
		return -1;
	char *pmore=rptr; *rptr=more?1:0; rptr++;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	// reason
	short relen=reason?(short)strlen(reason) +1:0;
	if(rend -rptr<2 +relen)
		return -1;
	memcpy(rptr,&relen,2); rptr+=2;
	if(reason)
	{
		memcpy(rptr,reason,relen); rptr+=relen;
	}
	// nstr
	if(rend -rptr<4)
		return -1;
	char *pnstr=rptr;
	memcpy(rptr,&nstr,4); rptr+=4;
	for(int i=0;i<nstr;i++)
	{
		// flen
		short flen=(short)strlen(pstr[i]) +1;
		if(rend -rptr<2+flen)
		{
			if(i<1)
				return -1;
			*ppart=endcnt -(nstr -i); *pmore=1; nstr=i; memcpy(pnstr,&nstr,4);
			break;
		}
		memcpy(rptr,&flen,2); rptr+=2;
		// FIX msg
		memcpy(rptr,pstr[i],flen); rptr+=flen;
	}
	return (int)(rptr -rbegin);
}
int VSCodecV1::EncodeDescribeReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int& nstr, bool more, LONGLONG iter, const char *reason)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x09;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// rc
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rc,4); rptr+=4;
	// part
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&endcnt,4); rptr+=4;
	// nparts
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&totcnt,4); rptr+=4;
	// more
	if(rend -rptr<1)
		return -1;
	char *pmore=rptr; *rptr=more?1:0; rptr++;
	// iter
	if(rend -rptr<8)
		return -1;
	memcpy(rptr,&iter,8); rptr+=8;
	// reason
	short relen=reason?(short)strlen(reason) +1:0;
	if(rend -rptr<2 +relen)
		return -1;
	memcpy(rptr,&relen,2); rptr+=2;
	if(reason)
	{
		memcpy(rptr,reason,relen); rptr+=relen;
	}
	// nstr
	if(rend -rptr<4)
		return -1;
	char *pnstr=rptr;
	memcpy(rptr,&nstr,4); rptr+=4;
	for(int i=0;i<nstr;i++)
	{
		// schema len
		short flen=(short)strlen(pstr[i]) +1;
		if(rend -rptr<2+flen)
		{
			if(i<1)
				return -1;
			*pmore=1; nstr=i; memcpy(pnstr,&nstr,4);
			break;
		}
		memcpy(rptr,&flen,2); rptr+=2;
		// schema desc
		memcpy(rptr,pstr[i],flen); rptr+=flen;
	}
	return (int)(rptr -rbegin);
}
int VSCodecV1::DecodeReply(const char *rptr, int rlen, void *udata)
{
	if(!pnotify1)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	// cmds 0x01,0x02,0x03 retired
	// EncodeSqlFixReply
	if(cmd==0x04)
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
		// part
		int endcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&endcnt,rptr,4); rptr+=4;
		// nparts
		int totcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&totcnt,rptr,4); rptr+=4;
		// hist
		if(rend -rptr<1)
			return -1;
		bool hist=*rptr?true:false; rptr++;
		// proto
		if(rend -rptr<1)
			return -1;
		char proto=*rptr; rptr++;
		// more
		bool more=false;
		if(rend -rptr<1)
			return -1;
		more=*rptr?true:false; rptr++;
		// iter
		LONGLONG iter=0;
		if(rend -rptr<8)
			return -1;
		memcpy(&iter,rptr,8); rptr+=8;
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
		// nfix
		int nfix=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&nfix,rptr,4); rptr+=4;
		if((nfix<0)||(nfix>256))
			return -1;
		FIXINFO *pfix=0;
		if(nfix>0)
		{
			pfix=new FIXINFO[nfix];
			if(!pfix)
				return -1;
		}
		for(int i=0;i<nfix;i++)
		{
			// flen
			if(rend -rptr<2)
			{
				delete pfix;
				return -1;
			}
			short flen=0;
			memcpy(&flen,rptr,2); rptr+=2;
			// FIX msg
			if(rend -rptr<flen)
			{
				delete pfix;
				return -1;
			}
			pfix[i].Reset();
			if((proto==0x02)||(proto==0x10))
			{
				pfix[i].FIXDELIM=';';
				pfix[i].FIXDELIM2=' ';
			}
			else if(proto==0x0A)
				pfix[i].FIXDELIM=' ';
			else if((proto==0x0D)||(proto==0x0E))
				pfix[i].FIXDELIM='|';
			else if(proto==0x20)
				pfix[i].FIXDELIM='_';
			else
				pfix[i].FIXDELIM=0x01;
			pfix[i].noSession=true;
			pfix[i].supressChkErr=true;
			if(pfix[i].FixMsgReady((char*)rptr,(proto==0x0A)?flen-1:flen)<0)
			{
				delete pfix;
				return -1;
			}
			rptr+=flen;
		}

		pnotify1->VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nfix,more,iter,reason);
		if(pfix)
			delete[] pfix;
		return (int)(rptr -rbegin);
	}
	// EncodeSqlIndexReply
	else if(cmd==0x05)
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
		// part
		int endcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&endcnt,rptr,4); rptr+=4;
		// nparts
		int totcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&totcnt,rptr,4); rptr+=4;
		// proto
		if(rend -rptr<1)
			return -1;
		char proto=*rptr; rptr++;
		// more
		bool more=false;
		if(rend -rptr<1)
			return -1;
		more=*rptr?true:false; rptr++;
		// iter
		LONGLONG iter=0;
		if(rend -rptr<8)
			return -1;
		memcpy(&iter,rptr,8); rptr+=8;
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
		// nstr
		int nstr=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&nstr,rptr,4); rptr+=4;
		const char **pstr=0;
		if(nstr>0)
		{
			pstr=new const char *[nstr];
			if(!pstr)
				return -1;
		}
		for(int i=0;i<nstr;i++)
		{
			// flen
			if(rend -rptr<2)
			{
				delete pstr;
				return -1;
			}
			short flen=0;
			memcpy(&flen,rptr,2); rptr+=2;
			// FIX msg
			if(rend -rptr<flen)
			{
				delete pstr;
				return -1;
			}
			pstr[i]=rptr; rptr+=flen;
		}

		pnotify1->VSCNotifySqlIndexReply(udata,rc,rid,endcnt,totcnt,proto,pstr,nstr,more,iter,reason);
		if(pstr)
			delete[] pstr;
		return (int)(rptr -rbegin);
	}
	// EncodeLoginReply
	else if(cmd==0x07)
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

		pnotify1->VSCNotifyLoginReply(udata,rc,rid,reason);
		return (int)(rptr -rbegin);
	}
	// EncodeSqlDsvReply
	else if(cmd==0x08)
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
		// part
		int endcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&endcnt,rptr,4); rptr+=4;
		// nparts
		int totcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&totcnt,rptr,4); rptr+=4;
		// hist
		if(rend -rptr<1)
			return -1;
		bool hist=*rptr?true:false; rptr++;
		// proto
		if(rend -rptr<1)
			return -1;
		char proto=*rptr; rptr++;
		// more
		bool more=false;
		if(rend -rptr<1)
			return -1;
		more=*rptr?true:false; rptr++;
		// iter
		LONGLONG iter=0;
		if(rend -rptr<8)
			return -1;
		memcpy(&iter,rptr,8); rptr+=8;
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
		// nstr
		int nstr=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&nstr,rptr,4); rptr+=4;
		if((nstr<0)||(nstr>256))
			return -1;
		const char **pstr=0;
		if(nstr>0)
		{
			pstr=new const char *[nstr];
			if(!pstr)
				return -1;
		}
		for(int i=0;i<nstr;i++)
		{
			// flen
			if(rend -rptr<2)
			{
				if(pstr) delete pstr;
				return -1;
			}
			short flen=0;
			memcpy(&flen,rptr,2); rptr+=2;
			// dsv string
			if(rend -rptr<flen)
			{
				if(pstr) delete pstr;
				return -1;
			}
			pstr[i]=rptr; rptr+=flen;
		}

		pnotify1->VSCNotifySqlDsvReply(udata,rc,rid,endcnt,totcnt,hist,proto,pstr,nstr,more,iter,reason);
		if(pstr)
			delete pstr;
		return (int)(rptr -rbegin);
	}
	// EncodeDescribeReply
	else if(cmd==0x09)
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
		// part
		int endcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&endcnt,rptr,4); rptr+=4;
		// nparts
		int totcnt=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&totcnt,rptr,4); rptr+=4;
		// more
		bool more=false;
		if(rend -rptr<1)
			return -1;
		more=*rptr?true:false; rptr++;
		// iter
		LONGLONG iter=0;
		if(rend -rptr<8)
			return -1;
		memcpy(&iter,rptr,8); rptr+=8;
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
		// nstr
		int nstr=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&nstr,rptr,4); rptr+=4;
		const char **pstr=0;
		if(nstr>0)
		{
			pstr=new const char *[nstr];
			if(!pstr)
				return -1;
		}
		for(int i=0;i<nstr;i++)
		{
			// flen
			if(rend -rptr<2)
			{
				delete pstr;
				return -1;
			}
			short flen=0;
			memcpy(&flen,rptr,2); rptr+=2;
			// FIX msg
			if(rend -rptr<flen)
			{
				delete pstr;
				return -1;
			}
			pstr[i]=rptr; rptr+=flen;
		}

		pnotify1->VSCNotifyDescribeReply(udata,rc,rid,endcnt,totcnt,pstr,nstr,more,iter,reason);
		if(pstr)
			delete[] pstr;
		return (int)(rptr -rbegin);
	}
	else
	{
		_ASSERT(false);
		return -1;
	}
}
