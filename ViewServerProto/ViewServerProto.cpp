// ViewServerProto.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "ViewServerProto.h"
#ifdef WIN32
#include <crtdbg.h>
#endif


VSCodec::VSCodec()
	:pnotify(0)
{
}
VSCodec::~VSCodec()
{
}
int VSCodec::Init(VSCNotify *pnotify)
{
	if(VSCodecV2::Init(pnotify)<0)
		return -1;
	this->pnotify=pnotify;
	return 0;
}
int VSCodec::Shutdown()
{
	VSCodecV2::Shutdown();
	pnotify=0;
	return 0;
}

int VSCodec::DecodeReply(const char *rptr, int rlen, void *udata)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	if(cmd==0x30) //EncodeDatReply
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

		const void **pstr=0;
		if(nstr>0)
		{
			pstr=new const void *[nstr];
			if(!pstr)
				return -1;
		}
		short flen=0;
		for(int i=0;i<nstr;i++)
		{
			// flen
			if(rend -rptr<2)
			{
				if(pstr) delete pstr;
				return -1;
			}
			memcpy(&flen,rptr,2); rptr+=2;
			// dat 
			if(rend -rptr<flen)
			{
				if(pstr) delete pstr;
				return -1;
			}
			pstr[i]=rptr; rptr+=flen;
		}
		pnotify->VSCNotifyDatReply(udata,rc,rid,endcnt,totcnt,hist,proto,flen,pstr,nstr,more,iter,reason);
		if(pstr)
			delete[] pstr;
		return (int)(rptr -rbegin);
	}
	else
	{
		rptr=rbegin;
		return VSCodecV2::DecodeReply(rptr,rlen,udata);
	}
	return (int)(rptr -rbegin);
}

int VSCodec::EncodeDatRequest(char *&rptr, int rlen, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	if((!pnotify)||(!select)||(!from))
		return -1;
	if(!where) where="";
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x30;
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

int VSCodec::DecodeRequest(const char *rptr, int rlen, void *udata)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	if(cmd==0x30) // EncodeDatRequest
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

		pnotify->VSCNotifyDatRequest(udata,rid,select,from,where,maxorders,hist,live,iter);
	}
	else
	{
		rptr=rbegin;
		return VSCodecV2::DecodeRequest(rptr,rlen,udata);
	}
	return (int)(rptr -rbegin);
}

int VSCodec::EncodeDatReply(char *&rptr, int rlen, int rc, DWORD rid, int endcnt, int totcnt, bool hist, 
							char proto, short* pstrlen, const void **pstr, int& nstr, bool more, LONGLONG iter, const char *reason)
{
	if(!pnotify)
		return -1;

	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x30;
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
	// proto
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
		if(rend -rptr < pstrlen[i])
		{
			// not enough room, set the more indicator
			if(i<1)
				return -1;
			*pmore=1; nstr=i; memcpy(pnstr,&nstr,4);
			break;
		}
		memcpy(rptr,&pstrlen[i],2); rptr+=2;
		if(pstr[i])
			memcpy(rptr,pstr[i],pstrlen[i]); 
		else
			_ASSERT(false);
		rptr+=pstrlen[i];
	}
	return (int)(rptr -rbegin);
}

