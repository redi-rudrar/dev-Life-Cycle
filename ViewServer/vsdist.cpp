
#include "stdafx.h"
#include "vsdefs.h"
#include "vsdist.h"
#include "wstring.h"

VSDistJournal::VSDistJournal()
	:firstPage(0)
	,lastPage(0)
	,lcnt(0)
	,pmem(0)
	,bmem(0)
	#ifdef DEBUG_JOURNAL
	,fpath()
	,fp(0)
	,next(1)
	#endif
{
	memset(locAlias,0,sizeof(locAlias));
	mutex=CreateMutex(0,false,0);
}
VSDistJournal::~VSDistJournal()
{
	if(mutex)
	#ifdef _DEBUG
		WaitForSingleObject(mutex,INFINITE);
	#else
		WaitForSingleObject(mutex,1000);
	#endif

	Shutdown();

	if(mutex)
	{
	#ifdef WIN32
		CloseHandle(mutex); mutex=0;
	#else
		DeleteMutex(mutex); mutex=0;
	#endif
	}
}
int VSDistJournal::Init(const char *alias)
{
	strncpy(locAlias,alias,sizeof(locAlias)-1); locAlias[sizeof(locAlias)-1]=0;
	#ifdef DEBUG_JOURNAL
	char spath[MAX_PATH]={0};
	sprintf(spath,"journal_%s.txt",alias);
	fpath=spath;
	fp=fopen(spath,"wt");
	#endif
	return 0;
}
int VSDistJournal::Shutdown()
{
	Lock();
	#ifdef DEBUG_JOURNAL
	if(fp)
	{
		fclose(fp); fp=0; fpath="";
	}
	#endif
	while(firstPage)
	{
		JPAGE *dpage=firstPage;
		firstPage=firstPage->next;
		delete dpage;
	}
	lastPage=0; pmem=0;
	Unlock();
	return 0;
}
int VSDistJournal::AddLine(int lidx, const char *lptr, LONGLONG rend)
{
	if(!lptr)
		return -1;
	int llen=(int)strlen(lptr)+1;
	// Need a new page
	Lock();
	if(lidx<0)
		lidx=++lcnt;
	else if(lidx>lcnt)
	{
		_ASSERT(false);
		lcnt=lidx;
	}
	if((!lastPage)||(lastPage->pend +llen>sizeof(lastPage->page)))
	{
		JPAGE *npage=new JPAGE;
		if(!npage)
		{
			Unlock();
			return -1;
		}
		pmem+=sizeof(JPAGE);
		memset(npage,0,sizeof(JPAGE));
		if(lastPage)
		{
			lastPage->next=npage; lastPage=npage;
		}
		else
			firstPage=lastPage=npage;
	#ifdef DEBUG_JOURNAL
		if(fp)
			fprintf(fp,"AddPage(%d)\n",lidx);
	#endif
	}
	// Add the line
	_ASSERT(lastPage);
	memcpy(lastPage->page +lastPage->pend,lptr,llen); lastPage->pend+=llen;
	if(rend>lastPage->rend)
		lastPage->rend=rend;
	if(lastPage->lstart<1)
		lastPage->lstart=lastPage->lend=lidx;
	if(lidx>lastPage->lend)
		lastPage->lend=lidx;
	#ifdef DEBUG_JOURNAL
	if(fp)
		//fprintf(fp,"%d,%I64d,%s\n",lastPage->lend,rend,lptr);
		fprintf(fp,"%s\n",lptr);
	#endif
	Unlock();
	return 0;
}
int VSDistJournal::RemLine(int lstart, int lend, LONGLONG& rend)
{
	if((lstart<1)||(lend<1))
		return -1;
	JPAGE *prev=0;
	int lidx=0;
	Lock();
	for(JPAGE *jpage=firstPage;jpage;)
	{
	#ifdef RETRANS_JOURNAL
		// We can get acks out of order, because we may retransmit
		if(jpage->lend<lstart)
		{
			prev=jpage; jpage=jpage->next;
			continue;
		}
		// The list is maintained in strictly increasing order
		else if(jpage->lstart>lend)
			break;
	#else
		// We shouldn't get out-of-order nor duplicate acks, but this case 
		// might trigger when server takes more than 3 seconds to ACK.
		if((jpage->lstart>lend)||(jpage->lend<lstart))
		{
			prev=jpage; jpage=jpage->next;
			continue;
		}
	#endif
		const char *pptr=jpage->page +jpage->pbeg;
		const char *pend=jpage->page +jpage->pend;
		for(lidx=jpage->lstart;(lidx<=lend)&&(lidx<=jpage->lend)&&(pptr<pend);lidx++)
		{
			const char *nptr=strechr(pptr,0,pend);
			if(nptr) nptr++;
			else nptr=pend;
			if((lidx>=lstart)&&(lidx<=lend))
			{
				jpage->pbeg=(int)(nptr -jpage->page);
				jpage->lstart++;
			}
			pptr=nptr;
		}
		// Page complete ACKed
		if((jpage->lstart>jpage->lend)||(jpage->pbeg>=jpage->pend))
		{
			JPAGE *dpage=jpage;
		#ifdef DEBUG_JOURNAL
			if(fp)
				fprintf(fp,"RemPage(%d,%d)\n",lstart,lend);
		#endif
			jpage=jpage->next;
		#ifdef RETRANS_JOURNAL
			if(prev)
				prev->next=jpage;
			else
				firstPage=jpage;
		#else
			if(firstPage==dpage)
				firstPage=jpage;
		#endif
			if(lastPage==dpage)
				lastPage=jpage?jpage:prev;
			rend=dpage->rend;
			delete dpage;
			if(pmem>=sizeof(JPAGE))
				pmem-=sizeof(JPAGE);
			if(lidx>lend)
				break;
		}
		jpage->timeSent=0; // Allow resend of the rest of this page
		prev=jpage; jpage=jpage->next;
	}
	Unlock();
	return 0;
}


VSDistCodec::VSDistCodec()
	:pnotify(0)
	#ifdef DEBUG_JOURNAL
	,fpath()
	,fp(0)
	#endif
{
}
VSDistCodec::~VSDistCodec()
{
}

int VSDistCodec::Init(VSDistNotify *pnotify)
{
	if(!pnotify)
		return -1;
	this->pnotify=pnotify;
	return 0;
}
int VSDistCodec::Shutdown()
{
	#ifdef DEBUG_JOURNAL
	if(fp)
	{
		fclose(fp); fp=0; fpath="";
	}
	#endif
	pnotify=0;
	return 0;
}

// Journal page transfer
int VSDistCodec::EncodeJPageRequest(char *&rptr, int rlen, DWORD rid, JPAGE *jpage, bool retransmit)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x01;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// jpage
	// lstart
	if(rend -rptr<4)
		return -1;
#ifdef RETRANS_JOURNAL
	int lstart=jpage->lstart;
	if(!retransmit)
		lstart=jpage->lsent ?jpage->lsent+1 :jpage->lstart;
#else
	int lstart=jpage->lsent ?jpage->lsent+1 :jpage->lstart;
#endif
	memcpy(rptr,&lstart,4); rptr+=4;
	// lend
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&jpage->lend,4); rptr+=4;
	// Find the start location of 'lsent'+1 (not the same as 'lstart' when un-acked)
	const char *pptr=jpage->page +jpage->pbeg;
	const char *pend=jpage->page +jpage->pend;
	for(int lidx=jpage->lstart;(lidx<=jpage->lsent)&&(lidx<=jpage->lend)&&(pptr<pend);lidx++)
	{
		const char *nptr=strechr(pptr,0,pend);
		if(nptr) nptr++;
		else nptr=pend;
		pptr=nptr;
	}
	// plen
	int plen=(int)(pend -pptr);
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&plen,4); rptr+=4;
	// page
	if(rend -rptr<plen)
		return -1;
	memcpy(rptr,pptr,plen); rptr+=plen;
	// retransmit
	if(rend -rptr<1)
		return -1;
	*rptr=retransmit?1:0; rptr++;
	return (int)(rptr -rbegin);
}
// Detail file alias
int VSDistCodec::EncodeAliasRequest(char *&rptr, int rlen, DWORD rid, const char *prefix, const char *dpath, int proto, int wsdate)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x02;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// prefix
	short plen=(short)strlen(prefix) +1;
	if(rend -rptr<2 +plen)
		return -1;
	memcpy(rptr,&plen,2); rptr+=2;
	memcpy(rptr,prefix,plen); rptr+=plen;
	// dpath
	short dlen=(short)strlen(dpath) +1;
	if(rend -rptr<2 +dlen)
		return -1;
	memcpy(rptr,&dlen,2); rptr+=2;
	memcpy(rptr,dpath,dlen); rptr+=dlen;
	// proto
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&proto,4); rptr+=4;
	// wsdate
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&wsdate,4); rptr+=4;
	return (int)(rptr -rbegin);
}
int VSDistCodec::DecodeRequest(const char *rptr, int rlen, void *udata)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	// EncodeJPageRequest
	if(cmd==0x01)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// jpage
		JPAGE jpage;
		memset(&jpage,0,sizeof(JPAGE));
		// lstart
		if(rend -rptr<4)
			return -1;
		int lstart=0;
		memcpy(&lstart,rptr,4); rptr+=4;
		// lend
		if(rend -rptr<4)
			return -1;
		int lend=0;
		memcpy(&lend,rptr,4); rptr+=4;
		// plen
		if(rend -rptr<4)
			return -1;
		int plen=0;
		memcpy(&plen,rptr,4); rptr+=4;
		if((plen<0)||(plen>sizeof(jpage.page)))
			return -1;
		// page
		if(rend -rptr<plen)
			return -1;
		memcpy(jpage.page,rptr,plen); rptr+=plen;
		// retransmit
		bool retransmit=false;
		if(rend -rptr<1)
			return -1;
		retransmit=*rptr?true:false; rptr++;

		jpage.lstart=lstart;
		jpage.lend=lend;
		jpage.pbeg=0;
		jpage.pend=plen;
	#ifdef DEBUG_JOURNAL
		if(!fp)
		{
			int UscPort=LOWORD((PTRCAST)udata);
			char spath[MAX_PATH]={0};
			sprintf(spath,"journal_%03d.txt",UscPort);
			fpath=spath;
			fp=fopen(spath,"wt");
		}
		if(fp)
		{
			fprintf(fp,"AddPage(%d,%d)\n",jpage.lstart,jpage.lend);
			char *pptr=jpage.page +jpage.pbeg;
			for(int i=jpage.lstart;i<=jpage.lend;i++)
			{
				char *nptr=pptr +strlen(pptr) +1;
				//fprintf(fp,"%d,%s\n",i,pptr);
				fprintf(fp,"%s\n",pptr);
				pptr=nptr;
			}
		}
	#endif
		pnotify->VSDNotifyJPageRequest(udata,rid,&jpage,retransmit);
	}
	// EncodeAliasRequest
	else if(cmd==0x02)
	{
		// rid
		if(rend -rptr<4)
			return -1;
		DWORD rid=0;
		memcpy(&rid,rptr,4); rptr+=4;
		// prefix
		if(rend -rptr<2)
			return -1;
		short plen=0;
		memcpy(&plen,rptr,2); rptr+=2;
		if(rend -rptr<plen)
			return -1;
		char prefix[256]={0};
		memcpy(prefix,rptr,plen); rptr+=plen;
		// dpath
		if(rend -rptr<2)
			return -1;
		short dlen=0;
		memcpy(&dlen,rptr,2); rptr+=2;
		if(rend -rptr<dlen)
			return -1;
		char dpath[MAX_PATH]={0};
		memcpy(dpath,rptr,dlen); rptr+=dlen;
		// proto
		if(rend -rptr<4)
			return -1;
		int proto=0;
		memcpy(&proto,rptr,4); rptr+=4;
		// wsdate
		if(rend -rptr<4)
			return -1;
		int wsdate=0;
		memcpy(&wsdate,rptr,4); rptr+=4;
		pnotify->VSDNotifyAliasRequest(udata,rid,prefix,dpath,proto,wsdate);
	}
	else
	{
		_ASSERT(false);
		return -1;
	}
	return (int)(rptr -rbegin);
}

int VSDistCodec::EncodeJPageReply(char *&rptr, int rlen, int rc, DWORD rid, int lstart, int lend)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0x01;
	memcpy(rptr,&cmd,4); rptr+=4;
	// rid
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rid,4); rptr+=4;
	// rc
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&rc,4); rptr+=4;
	// lstart
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&lstart,4); rptr+=4;
	// lend
	if(rend -rptr<4)
		return -1;
	memcpy(rptr,&lend,4); rptr+=4;
#ifdef DEBUG_JOURNAL
	if(fp)
		fprintf(fp,"RemPage(%d,%d)\n",lstart,lend);
#endif
	return (int)(rptr -rbegin);
}
int VSDistCodec::DecodeReply(const char *rptr, int rlen, void *udata)
{
	if(!pnotify)
		return -1;
	const char *rbegin=rptr,*rend=rbegin +rlen;
	// cmd
	if(rend -rptr<4)
		return -1;
	int cmd=0;
	memcpy(&cmd,rptr,4); rptr+=4;
	// EncodeJPageReply
	if(cmd==0x01)
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
		// lstart
		int lstart=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&lstart,rptr,4); rptr+=4;
		// lend
		int lend=0;
		if(rend -rptr<4)
			return -1;
		memcpy(&lend,rptr,4); rptr+=4;
		pnotify->VSDNotifyJpageReply(udata,rc,rid,lstart,lend);
		return (int)(rptr -rbegin);
	}
	else
	{
		_ASSERT(false);
		return -1;
	}
}
