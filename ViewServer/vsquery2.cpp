
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "vsquery.h"
#include "wstring.h"
#include "md5.h"
//DT9383
#include "vsquerylive.h"

extern int _stdcall _SendSqlDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlAccountsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
//DT10472
extern int _stdcall _SendSqlComplexDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlComplexOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);

#ifdef IQDNS
extern int _stdcall _SendSqlUsersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlVarsidLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlDomrelLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlDomtreeLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlServersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
#endif

// For EncodeLoginRequest2
void ViewServer::VSCNotifyLoginRequest2(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
	_ASSERT((user)&&(*user));
	int PortNo=LOWORD(udata);
	if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS))
		return;	
	WSLogEvent("USR%d: VSCNotifyLoginRequest(%d,%s)",PortNo,rid,user);
	VSCodec *vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
	if(!vsc)
		return;
	FeedUser *fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
	if(!fuser)
		return;
	fuser->waitAuth=false;
	FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
	if(!ustats)
		return;

	// Check users.ini
	char reason[256]={0};
	strcpy(reason,"Not authorized.");
	int rc=-1;
	int maxorders=0,maxbrowse=0;
	char tpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(tpath,"%s\\setup\\users.ini",pcfg->RunPath.c_str());
	#else
	sprintf(tpath,"%s/setup/users.ini",pcfg->RunPath.c_str());
	#endif
	FILE *fp=fopen(tpath,"rt");
	if(fp)
	{
		char ubuf[1024]={0};
		while(fgets(ubuf,sizeof(ubuf),fp))
		{
			char *uptr;
			for(uptr=ubuf;!isprint(*uptr);uptr++)
				;
			if((!*uptr)||(*uptr=='\r')||(*uptr=='\n')||(!strncmp(uptr,"//",2)))
				continue;
			const char *uname=0,*upass=0;
			const char *entitle=0;
			int col=0;
			for(char *tok=strtoke(uptr,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				col++;
				switch(col)
				{
				case 1: uname=tok; break;
				case 2: upass=tok; break;
				case 3: maxorders=atoi(tok); break;
				case 4: maxbrowse=atoi(tok); break;
				case 5: entitle=tok; break;
				};
			}
			if(!_stricmp(uname,user))
			{
				MD5_CTX hash;
				MD5Init(&hash);
				unsigned char pbuf[32],dbuf[16];
				memset(pbuf,0,sizeof(pbuf));
				strncpy((char*)pbuf,upass,sizeof(pbuf));
				MD5Update(&hash,pbuf,sizeof(pbuf));
				MD5Final(dbuf,&hash);
				if(!memcmp(dbuf,phash,16))
				{
					rc=0;
					fuser->authed=true;
					fuser->user=user;
					fuser->epassLen=phlen;
					fuser->loginDate=WSDate;
					fuser->loginTime=WSTime;
					memcpy(fuser->epass,phash,16);
					fuser->DetPtr1=(void*)(LONGLONG)maxorders;
					fuser->DetPtr2=(void*)(LONGLONG)maxbrowse;
					fuser->DetPtr3=(void*)MakeEntitlementList((char*)entitle);
					WSLogEvent("USR%d: Logged in as %s (maxorders=%d)",PortNo,user,maxorders);
					sprintf(UsrPort[PortNo].Status,"%s %08d-%06d",
						user,fuser->loginDate,fuser->loginTime);
					break;
				}
			}
		}
		fclose(fp);
	}
	if(rc<0)
		WSLogEvent("USR%d: Failed login(%s): %s ",PortNo,user,reason);		

	char rbuf[256],*rptr=rbuf;
	if(vsc->EncodeLoginReply2(rptr,sizeof(rbuf),rc,rid,rc<0?reason:"",REGION.c_str(),WSDATE?WSDATE:WSDate)>0)
	{
		#ifdef _DEBUG
		WSLogEvent("USR%d: EncodeLoginReply2(rc=%d,rid=%d,reason=%s) %d bytes,maxorders=%d",
			PortNo,rc,rid,rc<0?reason:"",rptr -rbuf,maxorders);
		#endif
		WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
		ustats->msgCnt++;
	}
	else
		WSLogError("USR%d: EncodeLoginReply2 failed.",PortNo);
}
// Server-to-server login reply
void ViewServer::VSCNotifyLoginReply2(void *udata, int rc, DWORD rid, const char *reason, const char *name, int wsdate)
{
	int PortNo=LOWORD(udata);
	if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS))
		return;	
	WSLogEvent("CON%d: Logged into %s (%08d) at %s:%d",
		PortNo,name,wsdate,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);

	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return;
	rtmap->rgname=name;
	rtmap->wsdate=wsdate;
	sprintf(ConPort[PortNo].OnLineStatusText,"%s,%08d",name,wsdate);
	APPSYSMAP *ramap=new APPSYSMAP;
	ConPort[PortNo].DetPtr7=ramap;

	// Browse peer's app systems
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(vsc)
	{
		int& rid=(int&)ConPort[PortNo].DetPtr4;
		// select * from APPSYSTEMS
		VSQuery *pquery=new VSQuery;
		pquery->FwdPortType=WS_CON;
		pquery->FwdPortNo=PortNo;
		pquery->select="*";
		pquery->from="APPSYSTEMS";
		pquery->where="";
		pquery->rid=rid++;
		pquery->hist=true;
		pquery->live=false;
		pquery->unique=true;
		pquery->tzbias=QueryTimeZoneBias;
	#ifdef _DEBUG
		WSLogEvent("EncodeSqlIndexRequest(rid=%d,sel=%s,from=%s,where=%s,maxorders=%d,hist=%d,live=%d,unique=%d,istart=%d,iter=%d)",
			pquery->rid,pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),0,pquery->hist,pquery->live,pquery->unique,0,0);
	#endif
		char rbuf[1024]={0},*rptr=rbuf;
		if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),pquery->rid,pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),0,pquery->unique,0,+1,0)<0)
		{
			delete pquery;
			WSLogError("CON%d: Failed EncodeSqlRequest",PortNo);
		}
		else
		{
			(*rtmap)[pquery->rid]=pquery;
			WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
		}
	}
}
// Server-to-server browse result
void ViewServer::VSCNotifySqlIndexReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	int PortNo=LOWORD(udata);
	if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS))
		return;	
	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(!vsc)
		return;
	APPSYSMAP *ramap=(APPSYSMAP *)ConPort[PortNo].DetPtr7;
	if(!ramap)
		return;

	VSQUERYMAP::iterator rit=rtmap->find(rid);
	if(rit==rtmap->end())
	{
		WSLogEvent("CON%d: Forward query not found for VSCNotifySqlIndexReply(rid=%d)",PortNo,rid);
		return;
	}
	VSQuery *pquery=rit->second;
	pquery->eiter=(int)iter;
	if(pquery->from=="APPSYSTEMS") // Browse app systems
	{
		if(rc<0)
		{
			WSLogError("CON%d: Query(%s) failed: %s",PortNo,pquery->from.c_str(),reason);
			return;
		}
		if(nstr>0)
		{
			int scnt=0;
			for(int i=0;i<nstr;i++)
			{
				AppSystem *rsys=CreateSystem(*ramap,pstr[i],scnt);
				if(rsys)
				{
					// select * from APPINSTANCE where AppSystem==<asname>
					int& rid=(int&)ConPort[PortNo].DetPtr4;
					VSQuery *iquery=new VSQuery;
					iquery->FwdPortType=WS_CON;
					iquery->FwdPortNo=PortNo;
					iquery->select="*";
					iquery->from="APPINSTANCE";
					char where[128]={0};
					sprintf(where,"AppSystem=='%s'",pstr[i]);
					iquery->where=where;
					iquery->rid=rid++;
					iquery->hist=true;
					iquery->live=false;
					iquery->unique=true;
					iquery->ikey1=rsys;
					iquery->tzbias=QueryTimeZoneBias;
				#ifdef _DEBUG
					WSLogEvent("CON%d: EncodeSqlIndexRequest(rid=%d,sel=%s,from=%s,where=%s,maxorders=%d,hist=%d,live=%d,unique=%d,istart=%d,iter=%d)",
						PortNo,iquery->rid,iquery->select.c_str(),iquery->from.c_str(),iquery->where.c_str(),0,iquery->hist,iquery->live,iquery->unique,0,0);
				#endif
					char rbuf[1024]={0},*rptr=rbuf;
					if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),iquery->rid,iquery->select.c_str(),iquery->from.c_str(),iquery->where.c_str(),0,iquery->unique,0,+1,0)<0)
					{
						delete iquery;
						WSLogError("CON%d: Failed EncodeSqlIndexRequest",PortNo);
					}
					else
					{
						(*rtmap)[iquery->rid]=iquery;
						WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
					}
				}
			}
		}
		// More to come
		if(more)
			return;
		// Query iteration
		if(iter)
		{
		#ifdef _DEBUG
			WSLogEvent("CON%d: EncodeSqlIndexRequest(rid=%d,sel=%s,from=%s,where=%s,maxorders=%d,hist=%d,live=%d,unique=%d,istart=%d,iter=%d)",
				PortNo,pquery->rid,pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),0,pquery->hist,pquery->live,pquery->unique,0,pquery->eiter);
		#endif
			char rbuf[1024]={0},*rptr=rbuf;
			if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),pquery->rid,pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),0,pquery->unique,0,+1,0)<0)
			{
				delete pquery;
				WSLogError("CON%d: Failed EncodeSqlIndexRequest",PortNo);
			}
			else
				WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
			return;
		}
		// All done
		if(LOG_BROWSING>0)
			WSLogEvent("CON%d: Browsed %d app systems",PortNo,ramap->size());
		rtmap->erase(rit);
		delete pquery;
	}
	else if(pquery->from=="APPINSTANCE") // Browse app instances
	{
		AppSystem *rsys=pquery->ikey1;
		if(rc<0)
		{
			WSLogError("CON%d: Query(%s,%s) failed: %s",PortNo,pquery->from.c_str(),pquery->where.c_str(),reason);
			return;
		}
		if(nstr>0)
		{
			int icnt=0;
			for(int i=0;i<nstr;i++)
				rsys->CreateInstance(pstr[i],icnt);
		}
		// More to come
		if(more)
			return;
		// Query iteration
		if(iter)
		{
		#ifdef _DEBUG
			WSLogEvent("CON%d: EncodeSqlIndexRequest(rid=%d,sel=%s,from=%s,where=%s,maxorders=%d,hist=%d,live=%d,unique=%d,istart=%d,iter=%d)",
				PortNo,pquery->rid,pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),0,pquery->hist,pquery->live,pquery->unique,0,pquery->eiter);
		#endif
			char rbuf[1024]={0},*rptr=rbuf;
			if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),pquery->rid,pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),0,pquery->unique,0,+1,0)<0)
			{
				delete pquery;
				WSLogError("CON%d: Failed EncodeSqlIndexRequest",PortNo);
			}
			else
				WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
			return;
		}
		// All done
		if(LOG_BROWSING>0)
			WSLogEvent("CON%d: Browsed %d instances for %s",PortNo,rsys->imap.size(),rsys->sname.c_str());
		rtmap->erase(rit);
		delete pquery;
	}
}
int VSQuery::ProxyFixResult(bool hist, int proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
{
	int acnt=0;
	for(int i=0;i<nfix;i++)
	{
		VSQueryResult *qresult=new VSQueryResult;
		qresult->rtime=GetTickCount();
		// Make a copy of the FIX string; don't just point to same memory that could go out of scope
		qresult->rfix.slen=pfix[i].llen;
		qresult->rfix.sbuf=new char[pfix[i].llen];
		memcpy(qresult->rfix.sbuf,pfix[i].fbuf,pfix[i].llen);
		if(hist)
			hresults.push_back(qresult); 
		else
			lresults.push_back(qresult); 
		rcnt++; acnt++;
	}
	return acnt;
}
void ViewServer::VSCNotifySqlFixReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, FIXINFO *pfix, int nfix, bool more, LONGLONG iter, const char *reason)
{
	// Find the forwarded query
	int PortNo=LOWORD(udata);
	if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS))
		return;	
	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(!vsc)
		return;
	APPSYSMAP *ramap=(APPSYSMAP *)ConPort[PortNo].DetPtr7;
	if(!ramap)
		return;
	VSQUERYMAP::iterator rit=rtmap->find(rid);
	if(rit==rtmap->end())
	{
		WSLogEvent("CON%d: Forward query not found for VSCNotifySqlFixReply(rid=%d)",PortNo,rid);
		return;
	}
	VSQuery *fquery=rit->second;
	fquery->eiter=(int)iter;

	// Find the USR query
	if((fquery->PortType!=WS_USR)||(fquery->PortNo<0)||(fquery->PortNo>=NO_OF_USR_PORTS)||(!UsrPort[fquery->PortNo].SockActive))
		return;
	int UsrPortNo=fquery->PortNo;
	FixStats *ustats=(FixStats *)UsrPort[UsrPortNo].DetPtr3;
	if(!ustats)
		return;
	VSQUERYMAP *qmap=(VSQUERYMAP*)UsrPort[UsrPortNo].DetPtr4;
	if(!qmap)
		return;
	VSQUERYMAP::iterator qit=qmap->find(fquery->uiter);
	if(qit==qmap->end())
	{
		WSLogEvent("USR%d: Query(iter=%d) not found for VSCNotifySqlFixReply(rid=%d)",UsrPortNo,fquery->uiter,rid);
		return;
	}
	VSQuery *pquery=qit->second;
	if((!more)&&(!iter))
	{
		rtmap->erase(rit);
		pquery->eiter=0;
		qmap->erase(qit);
	}
	if((rc<0)||(nfix<1))
	{
		int nenc=0;
		SendSqlResult(pquery,rc,endcnt,totcnt,hist,proto,nenc,more,reason);
	}
	else
	{
		if(pquery->ProxyFixResult(hist,proto,pfix,nfix,more,iter,reason)<0)
			return;
		int nenc=nfix;
		SendSqlResult(pquery,rc,endcnt,totcnt,hist,proto,nenc,more,reason);
		ustats->msgCnt+=nenc;
	}
	if((!more)&&(!iter))
	{
		delete fquery;
		delete pquery;
	}
}
int VSQuery::ProxyDsvResult(bool hist, int proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	int acnt=0;
	for(int i=0;i<nstr;i++)
	{
		VSQueryResult *qresult=new VSQueryResult;
		qresult->rtime=GetTickCount();
		// Make a copy of the FIX string; don't just point to same memory that could go out of scope
		int slen=(int)strlen(pstr[i])+1;
		qresult->obuf=new char[slen];
		strcpy(qresult->obuf,pstr[i]);
		if(hist)
			hresults.push_back(qresult); 
		else
			lresults.push_back(qresult); 
		rcnt++; acnt++;
	}
	return acnt;
}
void ViewServer::VSCNotifySqlDsvReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// Find the forwarded query
	int PortNo=LOWORD(udata);
	if((PortNo<0)||(PortNo>=NO_OF_CON_PORTS))
		return;	
	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(!vsc)
		return;
	APPSYSMAP *ramap=(APPSYSMAP *)ConPort[PortNo].DetPtr7;
	if(!ramap)
		return;
	VSQUERYMAP::iterator rit=rtmap->find(rid);
	if(rit==rtmap->end())
	{
		WSLogEvent("CON%d: Forward query not found for VSCNotifySqlDsvReply(rid=%d)",PortNo,rid);
		return;
	}
	VSQuery *fquery=rit->second;
	fquery->eiter=(int)iter;

	// Find the USR query
	if((fquery->PortType!=WS_USR)||(fquery->PortNo<0)||(fquery->PortNo>=NO_OF_USR_PORTS)||(!UsrPort[fquery->PortNo].SockActive))
		return;
	int UsrPortNo=fquery->PortNo;
	FixStats *ustats=(FixStats *)UsrPort[UsrPortNo].DetPtr3;
	if(!ustats)
		return;
	VSQUERYMAP *qmap=(VSQUERYMAP*)UsrPort[UsrPortNo].DetPtr4;
	if(!qmap)
		return;
	VSQUERYMAP::iterator qit=qmap->find(fquery->uiter);
	if(qit==qmap->end())
	{
		WSLogEvent("USR%d: Query(iter=%d) not found for VSCNotifySqlDsvReply(rid=%d)",UsrPortNo,fquery->uiter,rid);
		return;
	}
	VSQuery *pquery=qit->second;
	if((!more)&&(!iter))
	{
		rtmap->erase(rit);
		pquery->eiter=0;
		qmap->erase(qit);
	}
	if((rc<0)||(nstr<1))
	{
		int nenc=0;
		SendSqlResult(pquery,rc,endcnt,totcnt,hist,proto,nenc,more,reason);
	}
	else
	{
		if(pquery->ProxyDsvResult(hist,proto,pstr,nstr,more,iter,reason)<0)
			return;
		int nenc=nstr;
		SendSqlResult(pquery,rc,endcnt,totcnt,hist,proto,nenc,more,reason);
		ustats->msgCnt+=nenc;
	}
	if((!more)&&(!iter))
	{
		delete fquery;
		delete pquery;
	}
}

#ifdef TASK_THREAD_POOL
int _stdcall _PostTaskThread(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	int rc=pquery->pmod->PostTaskThread(fuser,skey,taskid,rid,pquery,hint);
	return rc;
}
int ViewServer::PostTaskThread(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	TASKOVL *tovl=new TASKOVL;
	tovl->fuser=fuser;
	tovl->skey=skey;
	tovl->taskid=taskid;
	tovl->rid=rid;
	tovl->pquery=pquery;
	memcpy(&tovl->fhint,hint,sizeof(FeedHint));
	PostQueuedCompletionStatus(taskIocpPort,sizeof(TASKOVL),(ULONG_PTR)this,tovl);
	return 1;
}
#endif

// For EncodeSqlRequest2
void ViewServer::VSCNotifySqlRequest2(void *udata, DWORD rid, const char *select, const char *_from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter, int ttl)
{
    char from[64] = {0};
    if(strcmp(_from, "IQOS") == 0)
        strcpy(from, "DETAILS");
    else
        strcpy(from, _from);
	_ASSERT((select)&&(*select)&&(from)&&(*from)&&(where));
	int PortType=HIWORD(udata);
	int PortNo=LOWORD(udata);
	VSCodec *vsc=0;
	FeedUser *fuser=0;
	FILE *fp=0;
	// Thin server query
	const char *PortTypeStr="???";
	FixStats *ustats=0;
	if(PortType==WS_USR)
	{
		PortTypeStr="USR";
		if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS))
			return;	
		vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
		if(!vsc)
			return;
		fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
		if((!fuser)||(!fuser->authed))
			return;
		ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(!ustats)
			return;
	}
	// Sysmon query
	else if(PortType==WS_UMR)
	{
		PortTypeStr="UMR";
		if((PortNo<0)||(PortNo>=NO_OF_UMR_PORTS))
			return;	
		vsc=(VSCodec *)UmrPort[PortNo].DetPtr6;
		if(!vsc)
			return;
		fuser=(FeedUser*)UmrPort[PortNo].DetPtr9;
		if(!fuser)
			return;
		fp=(FILE*)UmrPort[PortNo].DetPtr3;
		if(!fp)
			return;
	}
	else
	{
		_ASSERT(false);
		return;
	}
	if(LOG_QUERIES>0)
	WSLogEvent("%s%d: NotifySqlRequest(rid=%d,select=%s,from=%s,where=%s,maxorders=%d,hist=%d,live=%d,iter=%d)",
		PortTypeStr,PortNo,rid,select,from,where,maxorders,hist,live,(int)iter);

	// Iteration of previous query
	VSQUERYMAP *qmap=0;
	if(PortType==WS_USR)
	{
		qmap=(VSQUERYMAP*)UsrPort[PortNo].DetPtr4;
		if(!qmap)
		{
			qmap=new VSQUERYMAP;
			UsrPort[PortNo].DetPtr4=qmap;
		}
	}
	else if(PortType==WS_UMR)
	{
		qmap=(VSQUERYMAP*)UmrPort[PortNo].DetPtr4;
		if(!qmap)
		{
			qmap=new VSQUERYMAP;
			UmrPort[PortNo].DetPtr4=qmap;
		}
	}
	_ASSERT(qmap);
	VSQuery *pquery=0;
	// Override paging size with entitlement limit from users.ini
	int MAXORDERS_LIMIT=(int)(LONGLONG)fuser->DetPtr1;
	if((maxorders<1)||((MAXORDERS_LIMIT>0)&&(maxorders>MAXORDERS_LIMIT)))
		maxorders=MAXORDERS_LIMIT;
	// 'iter' must be validated; don't trust any outside values
	VSQUERYMAP::iterator qit=qmap->find((int)iter);
	// New query
	if(qit==qmap->end())
	{
		pquery=new VSQuery;
		pquery->fuser=fuser;
		pquery->rid=rid;
		pquery->PortType=PortType;
		pquery->PortNo=PortNo;
		pquery->vsc=vsc;
		pquery->fp=fp;
		pquery->select=select;
		pquery->from=from;
		pquery->where=where;
		pquery->maxorders=maxorders;
		pquery->maxbrowse=(int)(LONGLONG)fuser->DetPtr2; //DT10472
		pquery->hist=hist;
		pquery->live=live;
		pquery->ttl=ttl;
		pquery->unique=false;
		pquery->tstart=GetTickCount();
		pquery->tzbias=QueryTimeZoneBias;
		pquery->eiter=++qmap->eiterNext;
		//DT9398
		for(unsigned int i=0;i<odb.INDEX_AUXKEYS;i++)
		{
			pquery->qscope.AuxKeyNames.push_back(odb.AUXKEY_NAMES[i]);
		}
		(*qmap)[pquery->eiter]=pquery;
		qit=qmap->find(pquery->eiter);

#ifdef SPECTRUM
		if(ComplexQuery(pquery,qmap,qit,fuser)) //DT10472
			return;
#endif
		// Virtual DETAILS table
		if(!_stricmp(from,"DETAILS"))
		{
			if(StartQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual ORDERS table
		else if(!_stricmp(from,"ORDERS"))
		{
			if(StartQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual ACCOUNTS table
		else if(!_stricmp(from,"ACCOUNTS"))
		{
			if(StartAccountsQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
	#ifdef IQDNS
		// Virtual USERS table
		else if(!_stricmp(from,"USERS"))
		{
			if(StartUsersQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual VARSID table
		else if(!_stricmp(from,"VARSID"))
		{
			if(StartVarsidQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual DOMREL table
		else if(!_stricmp(from,"DOMREL"))
		{
			if(StartDomrelQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual DOMREL table
		else if(!_stricmp(from,"DOMTREE"))
		{
			if(StartDomtreeQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual SERVERS table
		else if(!_stricmp(from,"SERVERS"))
		{
			if(StartServersQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
	#endif
		// Invalid from
		else
		{
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,0);
			qmap->erase(qit);
			delete pquery;
			return;
		}

		// There is no local AppSystem when forwarded
		if(pquery->FwdPortType>=0)
			return;
		// Historic query
		if(hist)
		{
			// Virtual DETAILS table
			if(!_stricmp(from,"DETAILS"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueDetailsQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
			// Virtual ORDERS table
			else if(!_stricmp(from,"ORDERS"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueOrdersQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
			// Virtual ACCOUNTS table
			else if(!_stricmp(from,"ACCOUNTS"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueAccountsQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
		#ifdef IQDNS
			// Virtual USERS table
			else if(!_stricmp(from,"USERS"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueUsersQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
			// Virtual VARSID table
			else if(!_stricmp(from,"VARSID"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueVarsidQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
			// Virtual DOMREL table
			else if(!_stricmp(from,"DOMREL"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueDomrelQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
			// Virtual DOMTREE table
			else if(!_stricmp(from,"DOMTREE"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueDomtreeQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
			// Virtual SERVERS table
			else if(!_stricmp(from,"SERVERS"))
			{
				if(pquery->morehist)
				{
					pquery->tcontinue=pquery->tstart;
					ContinueServersQuery(pquery);
					// Auto-delete when no more history and not live
					if((!pquery->morehist)&&(!pquery->live))
					{
						qmap->erase(qit);
						delete pquery;
						return;
					}
				}
			}
		#endif
			else
				_ASSERT(false);
		}

		// Live query
		if(live)
		{
			AddLiveQuery(pquery,fuser);
		}
	}
	// Continued query
	else
	{
		pquery=qit->second;
		// Some attributes can change on continue
		pquery->select=select;
		pquery->maxorders=maxorders;
		pquery->rid=rid;

		// Throttle iteration
		// TODO: Handle GetTickCount value reset every 49 days
		DWORD tnow=GetTickCount();
		DWORD tdiff=tnow -pquery->tcontinue;
		if(tdiff<1000)
		{
			if((MAXORDERS_LIMIT>0)&&(pquery->scontinue>=MAXORDERS_LIMIT))
			{
				SleepEx(1000 -tdiff,true);
				pquery->tcontinue=tnow=GetTickCount();
				pquery->scontinue=0;
			}
		}
		else
		{
			pquery->tcontinue=tnow;
			pquery->scontinue=0;
		}

		#ifdef SPECTRUM
		//DT10472
		if(pquery->complexquery)
		{
			ComplexQueryContinueQuery(pquery,qmap,qit);
			return;
		}
		#endif

		// There is no local AppSystem when forwarded
		if(pquery->FwdPortType>=0)
		{
			ContinueForwardQuery(pquery);
			return;
		}
		// Virtual DETAILS table
		if(!_stricmp(pquery->from.c_str(),"DETAILS"))
		{
			if(ContinueDetailsQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual ORDERS table
		else if(!_stricmp(pquery->from.c_str(),"ORDERS"))
		{
			if(ContinueOrdersQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}

			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual ACCOUNTS table
		else if(!_stricmp(pquery->from.c_str(),"ACCOUNTS"))
		{
			if(ContinueAccountsQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
	#ifdef IQDNS
		// Virtual USERS table
		else if(!_stricmp(pquery->from.c_str(),"USERS"))
		{
			if(ContinueUsersQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual VARSID table
		else if(!_stricmp(pquery->from.c_str(),"VARSID"))
		{
			if(ContinueVarsidQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual DOMREL table
		else if(!_stricmp(pquery->from.c_str(),"DOMREL"))
		{
			if(ContinueDomrelQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual DOMTREE table
		else if(!_stricmp(pquery->from.c_str(),"DOMTREE"))
		{
			if(ContinueDomrelQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		// Virtual SERVERS table
		else if(!_stricmp(pquery->from.c_str(),"SERVERS"))
		{
			if(ContinueServersQuery(pquery)<0)
			{
				if(!pquery->live)
				{
					qmap->erase(qit);
					delete pquery;
				}
				return;
			}
			// Auto-delete when no more history and not live
			if((!pquery->morehist)&&(!pquery->live))
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
	#endif
		// Invalid from
		else
		{
			int nenc=0;
			SendSqlResult(pquery,-1,0,0,true,0,nenc,false,0);
			if(!pquery->live)
			{
				qmap->erase(qit);
				delete pquery;
			}
			return;
		}
	}

	return;
}
// Picks one index from the where clause, hopefully the one with smallest result set to walk
int ViewServer::StartQuery(VSQuery *pquery)
{
	const char *where=pquery->where.c_str();
	VSQueryScope& qscope=pquery->qscope;

	// Search all orders
	// Pre-process the where clause to narrow the search scope
	char reason[256]={0};
	if(pquery->NarrowScope(&pquery->qscope,where,this,reason)<0)
	{
		int nenc=0;
		SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
		return -1;
	}
	pquery->asys=0;
	pquery->ainst=0;
	OrderDB *pdb=0;
	// Region-specific query
	if(qscope.CondRegion.size()>0)
	{
		for(CONDLIST::iterator cit=qscope.CondRegion.begin();cit!=qscope.CondRegion.end();cit++)
		{
			ExprTok *etok=*cit;
			const char *Region=qscope.GetEqStrVal(etok);
			int ConPortNo=-1;
			APPSYSMAP *ramap=GetRemoteRegion(Region,ConPortNo);
			if(!ramap)
			{
				pquery->morehist=false;
				sprintf(reason,"Region(%s) not found.",Region);
				int nenc=0;
				SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
				return -1;
			}
			if(pquery->ttl<2)
			{
				pquery->morehist=false;
				sprintf(reason,"Ttl(%d) must be >1 for query on external region(%s).",pquery->ttl,Region);
				int nenc=0;
				SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
				return -1;
			}
			return StartForwardQuery(pquery,0,ConPortNo);
		}
		_ASSERT(false);
		return -1;
	}
	WaitForSingleObject(asmux,INFINITE);
	// Specific systems
	if(qscope.CondAppSystem.size()>0)
	{
		for(CONDLIST::iterator cit=qscope.CondAppSystem.begin();cit!=qscope.CondAppSystem.end();cit++)
		{
			ExprTok *etok=*cit;
			const char *SysName=qscope.GetEqStrVal(etok);
			if(SysName)
			{
				AppSystem *asys=GetAppSystem(SysName);
				if(asys)
				{
					if(IsEntitled(pquery->fuser,asys->sname.c_str(),0))
					{
						for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
							pquery->ailist.push_back(iit->second);
					}
					else
					{
						ReleaseMutex(asmux);
						pquery->morehist=false;
						sprintf(reason,"AppInstID(%s*) not entitled.",SysName);
						int nenc=0;
						SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
						return -1;
					}
				}
				else
				{
					ReleaseMutex(asmux);
					// The system might exist on a remote VSDB
					if(pquery->ttl>1)
						return StartForwardQuery(pquery,SysName,-1);
					else
					{
						pquery->morehist=false;
						sprintf(reason,"AppInstID(%s*) not found.",SysName);
						int nenc=0;
						SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
						return -1;
					}
				}
			}
		}
	}
	// Specific instances
	if(qscope.CondAppInstID.size()>0)
	{
		for(CONDLIST::iterator cit=qscope.CondAppInstID.begin();cit!=qscope.CondAppInstID.end();cit++)
		{
			ExprTok *etok=*cit;
			const char *InstName=qscope.GetEqStrVal(etok);
			if(InstName)
			{
				AppInstance *ainst=GetAppInstance(InstName);
				if(ainst)
				{
					if(IsEntitled(pquery->fuser,ainst->asys->sname.c_str(),ainst->iname.c_str()))
						pquery->ailist.push_back(ainst);
					else
					{
						ReleaseMutex(asmux);
						pquery->morehist=false;
						sprintf(reason,"AppInstID(%s) not entitled.",InstName);
						int nenc=0;
						SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
						return -1;
					}
				}
				else
				{
					ReleaseMutex(asmux);
					// The instance might exist on a remote VSDB
					if(pquery->ttl>1)
						return StartForwardQuery(pquery,InstName,-1);
					else
					{
						pquery->morehist=false;
						sprintf(reason,"AppInstID(%s) not found.",InstName);
						int nenc=0;
						SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
						return -1;
					}
				}
			}
		}
	}
	pquery->aiit=pquery->ailist.begin();
	if(pquery->aiit!=pquery->ailist.end())
	{
		pquery->ainst=*pquery->aiit;
		pquery->aoit=(*pquery->aiit)->oset.begin();
		pquery->asys=pquery->ainst->asys;
		pdb=&pquery->asys->odb;
	}

	// AppSystem map iterator when no AppInstID conditions present
	if(pdb)
		pquery->asit=asmap.end();
	else
	{
		pquery->asit=asmap.begin();
		while(pquery->asit!=asmap.end())
		{
			if(IsEntitled(pquery->fuser,pquery->asit->second->sname.c_str(),0))
			{
				pquery->asys=pquery->asit->second;
				pdb=&pquery->asys->odb;
				break;
			}
			else
				pquery->asit++;
		}
		if(pquery->asit==asmap.end())
		{
			ReleaseMutex(asmux);
			int nenc=0;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
	}
	ReleaseMutex(asmux);

	if(pquery->hist)
	{
		pquery->asys->odb.Lock();
		pquery->morehist=true;
		pquery->spos=0; // 'supos' not used for SQL query
		NextAppSystem(pquery,true);
		if(pquery && pquery->asys)
			pquery->asys->odb.Unlock();
	}
	else
		pquery->morehist=false;
	return 0;
}
int ViewServer::StartForwardQuery(VSQuery *pquery, const char *AppInstID, int PortNo)
{
	// Search by browsed instance maps when PortNo not supplied
	if(PortNo<0)
	{
		for(int i=0;i<NO_OF_CON_PORTS;i++)
		{
			if(!ConPort[i].SockConnected)
				continue;
			if(ConPort[i].DetPtr==(void*)PROTO_VSCLIENT)
			{
				APPSYSMAP *ramap=(APPSYSMAP *)ConPort[i].DetPtr7;
				if(ramap)
				{
					if(strrcmp(AppInstID,"*"))
					{
						APPSYSMAP::iterator ait=ramap->find(AppInstID);
						if(ait!=ramap->end())
						{
							PortNo=i;
							break;
						}
					}
					else
					{
						for(APPSYSMAP::iterator ait=ramap->begin();ait!=ramap->end();ait++)
						{
							AppSystem *rsys=ait->second;
							APPINSTMAP::iterator iit=rsys->imap.find(AppInstID);
							if(iit!=rsys->imap.end())
							{
								PortNo=i;
								break;
							}
						}
					}
				}
			}
		}
		if(PortNo<0)
			return -1;
	}

	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return -1;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(!vsc)
		return -1;
	// We need the CON port exclusively while we're modifying the 'rtmap'
	WSLockPort(WS_CON,PortNo,false);
	WSLockPort(WS_CON,PortNo,true);
	int& rid=(int&)ConPort[PortNo].DetPtr4;
	char rbuf[1024]={0},*rptr=rbuf;
	VSQuery *fquery=new VSQuery;
	fquery->PortType=pquery->PortType;
	fquery->PortNo=pquery->PortNo;
	fquery->FwdPortType=WS_CON;
	fquery->FwdPortNo=PortNo;
	fquery->select=pquery->select;
	fquery->from=pquery->from;
	fquery->where=pquery->where;
	fquery->rid=rid++;
	fquery->maxorders=pquery->maxorders;
	fquery->hist=pquery->hist;
	fquery->live=pquery->live;
	fquery->unique=pquery->unique;
	fquery->eiter=0;
	fquery->ttl=pquery->ttl -1;
	fquery->uiter=pquery->eiter;
	fquery->tzbias=QueryTimeZoneBias;
	if(fquery->from=="INDICES")
	{
		if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),fquery->rid,fquery->select.c_str(),fquery->from.c_str(),fquery->where.c_str(),fquery->maxorders,fquery->unique,0,+1,fquery->eiter)<0)
		{
			delete fquery;
			WSLogError("CON%d: Failed EncodeSqlIndexRequest",PortNo);
		}
		else
		{
			(*rtmap)[fquery->rid]=fquery;
			WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
		}
	}
	else
	{
		if(vsc->EncodeSqlRequest2(rptr,sizeof(rbuf),fquery->rid,fquery->select.c_str(),fquery->from.c_str(),fquery->where.c_str(),fquery->maxorders,fquery->hist,fquery->live,fquery->eiter,fquery->ttl)<0)
			WSLogError("CON%d: Failed EncodeSqlRequest",PortNo);
		else
		{
			(*rtmap)[fquery->rid]=fquery;
			WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
		}
	}
	pquery->FwdPortType=WS_CON;
	pquery->FwdPortNo=PortNo;
	pquery->crid=fquery->rid;
	if(LOG_QUERIES>0)
		WSLogEvent("USR%d: Forwarded query(iter=%d) to CON%d [%s] as query(rid=%d)",
			pquery->PortNo,pquery->eiter,PortNo,ConPort[PortNo].CfgNote,fquery->rid);
	WSUnlockPort(WS_CON,PortNo,true);
	WSUnlockPort(WS_CON,PortNo,false);
	return 0;
}
int ViewServer::ContinueForwardQuery(VSQuery *pquery)
{
	_ASSERT((pquery)&&(pquery->FwdPortType>=0));
	if((pquery->FwdPortType!=WS_CON)||(pquery->FwdPortNo<0)||(pquery->FwdPortNo>=NO_OF_CON_PORTS)||(!ConPort[pquery->FwdPortNo].SockConnected))
		return -1;
	int PortNo=pquery->FwdPortNo;
	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return -1;
	VSQUERYMAP::iterator rit=rtmap->find(pquery->crid);
	if(rit==rtmap->end())
	{
		WSLogEvent("CON%d: Forward query(rid=%d) not found for ContinueForwardQuery(iter=%d)",PortNo,pquery->crid,pquery->eiter);
		return -1;
	}
	VSQuery *fquery=rit->second;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(!vsc)
		return -1;
	// We need the CON port exclusively while we're modifying the 'rtmap'
	WSLockPort(WS_CON,PortNo,false);
	WSLockPort(WS_CON,PortNo,true);
	char rbuf[1024]={0},*rptr=rbuf;
	if(fquery->from=="INDICES")
	{
		if(vsc->EncodeSqlIndexRequest(rptr,sizeof(rbuf),fquery->rid,fquery->select.c_str(),fquery->from.c_str(),fquery->where.c_str(),fquery->maxorders,fquery->unique,0,+1,fquery->eiter)<0)
		{
			delete fquery;
			WSLogError("CON%d: Failed EncodeSqlIndexRequest",PortNo);
		}
		else
		{
			(*rtmap)[fquery->rid]=fquery;
			WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
		}
	}
	else
	{
		//DT9044
		if(pquery->binarydatareq)
		{
			if(vsc->EncodeDatRequest(rptr,sizeof(rbuf),fquery->rid,fquery->select.c_str(),fquery->from.c_str(),fquery->where.c_str(),fquery->maxorders,fquery->hist,fquery->live,fquery->eiter)<0)
				WSLogError("CON%d: EncodeDatRequest EncodeSqlRequest",PortNo);
			else
			{
				(*rtmap)[fquery->rid]=fquery;
				WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
			}
		}
		else
		{
			if(vsc->EncodeSqlRequest2(rptr,sizeof(rbuf),fquery->rid,fquery->select.c_str(),fquery->from.c_str(),fquery->where.c_str(),fquery->maxorders,fquery->hist,fquery->live,fquery->eiter,fquery->ttl)<0)
				WSLogError("CON%d: Failed EncodeSqlRequest",PortNo);
			else
			{
				(*rtmap)[fquery->rid]=fquery;
				WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
			}
		}
	}
	if(LOG_QUERIES>0)
		WSLogEvent("USR%d: Forwarded continue(%d,%d) to CON%d [%s] as continue(%d,%d)",
			pquery->PortNo,pquery->rid,pquery->eiter,PortNo,ConPort[PortNo].CfgNote,fquery->rid,fquery->eiter);
	WSUnlockPort(WS_CON,PortNo,true);
	WSUnlockPort(WS_CON,PortNo,false);
	return 0;
}
int ViewServer::CancelForwardQuery(VSQuery *pquery)
{
	_ASSERT((pquery)&&(pquery->FwdPortType>=0));
	if((pquery->FwdPortType!=WS_CON)||(pquery->FwdPortNo<0)||(pquery->FwdPortNo>=NO_OF_CON_PORTS)||(!ConPort[pquery->FwdPortNo].SockConnected))
		return -1;
	int PortNo=pquery->FwdPortNo;
	VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[PortNo].DetPtr5;
	if(!rtmap)
		return -1;
	VSQUERYMAP::iterator rit=rtmap->find(pquery->crid);
	if(rit==rtmap->end())
	{
		WSLogEvent("CON%d: Forward query(rid=%d) not found for CancelForwardQuery(iter=%d)",PortNo,pquery->crid,pquery->eiter);
		return -1;
	}
	VSQuery *fquery=rit->second;
	VSCodec *vsc=(VSCodec*)ConPort[PortNo].DetPtr6;
	if(!vsc)
		return -1;
	// We need the CON port exclusively while we're modifying the 'rtmap'
	WSLockPort(WS_CON,PortNo,false);
	WSLockPort(WS_CON,PortNo,true);
	char rbuf[1024]={0},*rptr=rbuf;
	if(vsc->EncodeCancelRequest(rptr,sizeof(rbuf),fquery->rid,fquery->eiter)<0)
		WSLogError("CON%d: Failed EncodeCancelRequest",PortNo);
	else
	{
		WSConSendMsg(1,(int)(rptr -rbuf),rbuf,PortNo);
		if(LOG_QUERIES>0)
			WSLogEvent("USR%d: Forwarded cancel(%d,%d) to CON%d [%s] as cancel(%d,%d)",
				pquery->PortNo,pquery->rid,pquery->eiter,PortNo,ConPort[PortNo].CfgNote,fquery->rid,fquery->eiter);
		rtmap->erase(rit);
		delete fquery;
	}
	WSUnlockPort(WS_CON,PortNo,true);
	WSUnlockPort(WS_CON,PortNo,false);
	return 0;
}
// For EncodeCancelRequest
void ViewServer::VSCNotifyCancelRequest(void *udata, DWORD rid, LONGLONG iter)
{
	_ASSERT(iter);
	int PortType=HIWORD(udata);
	int PortNo=LOWORD(udata);
	VSCodec *vsc=0;
	FeedUser *fuser=0;
	FILE *fp=0;
	// Thin server query
	if(PortType==WS_USR)
	{
		if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS))
			return;	
		vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
		if(!vsc)
			return;
		fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
		if((!fuser)||(!fuser->authed))
			return;
	}
	// Sysmon query
	else if(PortType==WS_UMR)
	{
		if((PortNo<0)||(PortNo>=NO_OF_UMR_PORTS))
			return;	
		vsc=(VSCodec *)UmrPort[PortNo].DetPtr6;
		if(!vsc)
			return;
		fuser=(FeedUser*)UmrPort[PortNo].DetPtr9;
		if(!fuser)
			return;
		fp=(FILE*)UmrPort[PortNo].DetPtr3;
		if(!fp)
			return;
	}
	if(LOG_QUERIES>0)
	WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d)",PortNo,rid,(int)iter);

	// Iteration of previous query
	VSQuery *pquery=0;
	VSQUERYMAP *qmap=0;
	if(PortType==WS_USR)
	{
		qmap=(VSQUERYMAP*)UsrPort[PortNo].DetPtr4;
		if(!qmap)
		{
			qmap=new VSQUERYMAP;
			UsrPort[PortNo].DetPtr4=qmap;
		}
		_ASSERT(qmap);
		VSQUERYMAP::iterator qit=qmap->find((int)iter);
		if(qit==qmap->end())
		{
			if(LOG_QUERIES>0)
			WSLogEvent("USR%d: Query(iter=%d) not found for cancel.",PortNo,(int)iter);
		}
		else
		{
			pquery=qit->second;
			if(LOG_QUERIES>0)
			WSLogEvent("USR%d: Cancelled query(iter=%d).",PortNo,(int)iter);
			qmap->erase(qit);
		}
	}
	else if(PortType==WS_UMR)
	{
		qmap=(VSQUERYMAP*)UmrPort[PortNo].DetPtr4;
		if(!qmap)
		{
			qmap=new VSQUERYMAP;
			UmrPort[PortNo].DetPtr4=qmap;
		}
		_ASSERT(qmap);
		VSQUERYMAP::iterator qit=qmap->find((int)iter);
		if(qit==qmap->end())
		{
			if(LOG_QUERIES>0)
			WSLogEvent("UMR%d: Query(iter=%d) not found for cancel.",PortNo,(int)iter);
		}
		else
		{
			pquery=qit->second;
			if(LOG_QUERIES>0)
			WSLogEvent("UMR%d: Cancelled query(iter=%d).",PortNo,(int)iter);
			qmap->erase(qit);
		}
	}
	if(pquery)
	{
		// Deregister live tasks
		if(pquery->live)
		{
			if(pquery->ptask)
			{
				WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d).DelUserTask1",PortNo,rid,(int)iter);
				taskmgr.DelUserTask(pquery->ptask); pquery->ptask=0;
				WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d).DelUserTask1.done",PortNo,rid,(int)iter);
			}

			for(LiveQueryMap::iterator itr=pquery->LiveQueries.begin(); itr!=pquery->LiveQueries.end();itr++)
			{
				WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d).DelUserTask map",PortNo,rid,(int)iter);
				taskmgr.DelUserTask((*itr).first);
				WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d).DelUserTask map done",PortNo,rid,(int)iter);
			}
		}
		if(pquery->FwdPortType>=0)
		{
			WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d).CancelForwardQuery",PortNo,rid,(int)iter);
			CancelForwardQuery(pquery);
		}
		delete pquery; pquery=0;
	}
	//WSLogEvent("USR%d: NotifyCancelRequest(rid=%d,iter=%d).out",PortNo,rid,(int)iter);
	return;
}

int ViewServer::SendSqlIndexResult(VSQuery *pquery, int rc, int endcnt, int totcnt, const char **pstr, int& ndets, bool more, const char *reason)
{
	DWORD rid=pquery->rid;
	const char *from=pquery->from.c_str();
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	VSCodec *vsc=pquery->vsc;
	FILE *fp=pquery->fp;
	FixStats *ustats=0;
	if(PortType==WS_USR)
	{
		ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(!ustats)
			return -1;
	}
	VSRESULTLIST& rlist=pquery->hresults;
	//bool more=pquery->morehist;
	LONGLONG iter=pquery->eiter;
	if(!reason) reason="";

	char *rbuf=pquery->rbuf,*rptr=rbuf;
	// DSV encoding
	int nstr=ndets;
	if(vsc->EncodeSqlIndexReply(rptr,sizeof(pquery->rbuf),rc,rid,endcnt,totcnt,PROTO_DSV,pstr,nstr,more,iter,reason)>0)
	{
		if(PortType==WS_USR)
		{
			if(LOG_QUERIES>0)
			WSLogEvent("USR%d: EncodeSqlIndexReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,hist=%d,proto=%d,nstr=%d,more=%d,iter=%d,reason=%s) %d bytes",
				PortNo,rc,rid,endcnt -(ndets -nstr),totcnt,true,PROTO_DSV,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
			WSUsrSendMsg(1,(WORD)(rptr -rbuf),(char*)rbuf,PortNo);
			ustats->msgCnt++;
		}
		else if(PortType==WS_UMR)
		{
			if(LOG_QUERIES>0)
			fprintf(fp,"UMR%d: EncodeSqlIndexReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,hist=%d,proto=%d,nstr=%d,more=%d,iter=%d,reason=%s) %d bytes\n",
				PortNo,rc,rid,endcnt,totcnt,true,PROTO_DSV,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
			char fstr[2048]={0};
			for(int f=0;f<nstr;f++)
			{
				int flen=(int)strlen(pstr[f]);
				if(flen>=sizeof(fstr))
					flen=sizeof(fstr) -1;
				memcpy(fstr,pstr[f],flen); fstr[flen]=0;
				fprintf(fp,"%s\n",fstr);
			}
		}
		ndets=nstr;
		return 1;
	}
	else
	{
		_ASSERT(false);
		if(PortType==WS_USR)
			WSLogError("USR%d: EncodeSqlIndexReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,hist=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes failed to encode!",
				PortNo,rc,rid,endcnt,totcnt,true,PROTO_DSV,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
		return -1;
	}
}
// Expects Region="<iqmatrix_instname>"
APPSYSMAP *ViewServer::GetRemoteRegion(const char *Region, int& ConPortNo)
{
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		if(ConPort[i].DetPtr==(void*)PROTO_VSCLIENT)
		{
			VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[i].DetPtr5;
			APPSYSMAP *ramap=(APPSYSMAP *)ConPort[i].DetPtr7;
			if((ramap)&&(rtmap)&&(!_stricmp(rtmap->rgname.c_str(),Region)))
			{
				ConPortNo=i;
				return ramap;
			}
		}
	}
	return 0;
}
// Expects AppInstID="<sysname>*", Region optional
AppSystem *ViewServer::GetRemoteSystem(const char *Region, const char *AppInstID, int& ConPortNo)
{
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		if(ConPort[i].DetPtr==(void*)PROTO_VSCLIENT)
		{
			VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[i].DetPtr5;
			APPSYSMAP *ramap=(APPSYSMAP *)ConPort[i].DetPtr7;
			if((ramap)&&(rtmap))
			{
				if((Region)&&(*Region)&&(_stricmp(rtmap->rgname.c_str(),Region)))
					continue;
				APPSYSMAP::iterator ait=ramap->find(AppInstID);
				if(ait!=ramap->end())
				{
					ConPortNo=i;
					return ait->second;
				}
			}
		}
	}
	return 0;
}
// Expects AppInstID="<instname>", Region optional
AppInstance *ViewServer::GetRemoteInstance(const char *Region, const char *AppInstID, int& ConPortNo)
{
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		if(ConPort[i].DetPtr==(void*)PROTO_VSCLIENT)
		{
			VSQUERYMAP *rtmap=(VSQUERYMAP *)ConPort[i].DetPtr5;
			APPSYSMAP *ramap=(APPSYSMAP *)ConPort[i].DetPtr7;
			if((ramap)&&(rtmap))
			{
				if((Region)&&(*Region)&&(_stricmp(rtmap->rgname.c_str(),Region)))
					continue;
				for(APPSYSMAP::iterator ait=ramap->begin();ait!=ramap->end();ait++)
				{
					AppSystem *rsys=ait->second;
					APPINSTMAP::iterator iit=rsys->imap.find(AppInstID);
					if(iit!=rsys->imap.end())
					{
						ConPortNo=i;
						return iit->second;
					}
				}
			}
		}
	}
	return 0;
}
void ViewServer::VSCNotifySqlIndexRequest2(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter, int ttl)
{
	_ASSERT((select)&&(*select)&&(from)&&(*from)&&(where));
	int PortType=HIWORD(udata);
	int PortNo=LOWORD(udata);
	VSCodec *vsc=0;
	FeedUser *fuser=0;
	FILE *fp=0;
	// Thin server query
	const char *PortTypeStr="???";
	FixStats *ustats=0;
	if(PortType==WS_USR)
	{
		PortTypeStr="USR";
		if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS))
			return;	
		vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
		if(!vsc)
			return;
		fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
		if((!fuser)||(!fuser->authed))
			return;
		ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(!ustats)
			return;
	}
	// Sysmon query
	else if(PortType==WS_UMR)
	{
		PortTypeStr="UMR";
		if((PortNo<0)||(PortNo>=NO_OF_UMR_PORTS))
			return;	
		vsc=(VSCodec *)UmrPort[PortNo].DetPtr6;
		if(!vsc)
			return;
		fuser=(FeedUser*)UmrPort[PortNo].DetPtr9;
		if(!fuser)
			return;
		fp=(FILE*)UmrPort[PortNo].DetPtr3;
		if(!fp)
			return;
	}
	else
	{
		_ASSERT(false);
		return;
	}
	if(LOG_BROWSING)
	WSLogEvent("%s%d: NotifySqlIndexRequest(rid=%d,select=%s,from=%s,where=%s,maxorders=%d,unique=%d,istart=%d,idir=%d,iter=%d)",
		PortTypeStr,PortNo,rid,select,from,where,maxorders,unique,istart,idir,(int)iter);

	// Iteration of previous query
	VSQUERYMAP *qmap=0;
	if(PortType==WS_USR)
	{
		qmap=(VSQUERYMAP*)UsrPort[PortNo].DetPtr4;
		if(!qmap)
		{
			qmap=new VSQUERYMAP;
			UsrPort[PortNo].DetPtr4=qmap;
		}
	}
	else if(PortType==WS_UMR)
	{
		qmap=(VSQUERYMAP*)UmrPort[PortNo].DetPtr4;
		if(!qmap)
		{
			qmap=new VSQUERYMAP;
			UmrPort[PortNo].DetPtr4=qmap;
		}
	}
	_ASSERT(qmap);

	char rbuf[16380],*rptr=rbuf;
	VSQuery *pquery=0;
	// Override paging size with entitlement limit from users.ini
	int MAXBROWSE_LIMIT=(int)(LONGLONG)fuser->DetPtr2;
	if((maxorders<1)||((MAXBROWSE_LIMIT>0)&&(maxorders>MAXBROWSE_LIMIT)))
		maxorders=MAXBROWSE_LIMIT;
	// 'iter' must be validated; don't trust any outside values
	VSQUERYMAP::iterator qit;
	if(!(int)iter)
	{
		pquery=new VSQuery;
		pquery->fuser=fuser;
		pquery->rid=rid;
		pquery->PortType=PortType;
		pquery->PortNo=PortNo;
		pquery->vsc=vsc;
		pquery->fp=fp;
		pquery->select=select;
		pquery->from=from;
		pquery->where=where;
		pquery->maxorders=maxorders;
		pquery->hist=true;
		pquery->live=false;
		pquery->unique=unique;
		pquery->tstart=GetTickCount();
		pquery->tzbias=QueryTimeZoneBias;
		pquery->eiter=++qmap->eiterNext;
		//DT9398
		for(unsigned int i=0;i<odb.INDEX_AUXKEYS;i++)
			pquery->qscope.AuxKeyNames.push_back(odb.AUXKEY_NAMES[i]);
		(*qmap)[pquery->eiter]=pquery;
		qit=qmap->find(pquery->eiter);
	}
	else
	{
		qit=qmap->find((int)iter);
		if(qit==qmap->end())
		{
			int nenc=0;
			char reason[256]={0};
			sprintf(reason,"Iter(%d) not found.",(int)iter);
			if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),-1,rid,0,0,0,nenc,false,0,reason)>0)
			{
				if(PortType==WS_USR)
				{
					if(LOG_BROWSING)
					WSLogEvent("USR%d: EncodeSqlReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
						PortNo,-1,rid,0,0,0,0,0,0,reason,rptr -rbuf);
					WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
					ustats->msgCnt++;
				}
			}
			else
				_ASSERT(false);
			return;
		}
		else
		{
			pquery=qit->second;
			pquery->rid=rid;
		}
	}

	// Virtual APPSYSTEMS table
	if(!_stricmp(from,"APPSYSTEMS"))
	{
		// External region
		if(!strincmp(where,"Region=='",9))
		{
			char rgn[32]={0};
			strncpy(rgn,where +9,sizeof(rgn)-1);
			rgn[sizeof(rgn)-1]=0;
			char *wptr=strchr(rgn,'\'');
			if(wptr) *wptr=0;
			int ConPortNo=-1;
			APPSYSMAP *ramap=GetRemoteRegion(rgn,ConPortNo);
			if(ramap)
			{
				int nstr=(int)ramap->size();
				const char **pstr=new const char *[nstr];
				int s=0;
				for(APPSYSMAP::iterator ait=ramap->begin();ait!=ramap->end();ait++)
					pstr[s++]=ait->second->sname.c_str();
				for(s=0;s<nstr;)
				{
					int nenc=nstr -s;
					SendSqlIndexResult(pquery,0,nstr,nstr,&pstr[s],nenc,false,"");
					s+=nenc;
				}
				delete pstr;
			}
			else
			{
				char reason[256]={0};
				sprintf(reason,"Region '%s' not found.",rgn);
				int nenc=0;
				SendSqlIndexResult(pquery,-1,0,0,0,nenc,false,reason);
			}
			return;
		}

		WaitForSingleObject(asmux,INFINITE);
		// Returns all systems (usually less than 20)
		int proto=0;
		int nstr=(int)asmap.size();
		const char **pstr=0;
		char *ibuf=0;
		if(nstr>0)
		{
		#ifndef IQSMP
			// This was hard-coded for iqview, but isn't needed for vsctest
			if(IQFLOW)
			{
				pstr=new const char *[1];
				pstr[0]="IQMLALGO";
				nstr=1;
			}
			else
		#endif
			{
				USERENTITLELIST *elist=(USERENTITLELIST *)fuser->DetPtr3;
				// Pre-generated system browse list 'asbrowse'
				if((elist)&&(elist->size()>0)&&(elist->front().first=="*"))
					pstr=asbrowse;
				// Entitled systems only
				else
				{
					pstr=new const char *[elist->size()];
					int ilen=(int)elist->size()*12+1;
					ibuf=new char[ilen];
					memset(ibuf,0,ilen);
					char *iptr=ibuf; nstr=0;
					for(USERENTITLELIST::iterator eit=elist->begin();eit!=elist->end();eit++)
					{
						USERENTITLEMENT& ent=*eit;
						strcpy(iptr,ent.first.c_str());
						pstr[nstr++]=iptr; iptr+=strlen(iptr)+1;
					}
				}
			}
			int i;
			for(i=0;i<nstr;)
			{
				int nenc=nstr -i;
				SendSqlIndexResult(pquery,0,i+nenc,nstr,&pstr[i],nenc,false,"");
				i+=nenc;
			}
		}
		// No results sent
		else
		{
			int nenc=0;
			SendSqlIndexResult(pquery,0,0,0,0,nenc,false,"");
		}
		if((ibuf)&&(pstr))
		{
			delete[] pstr; pstr=0;
			delete ibuf; ibuf=0;
		}
		ReleaseMutex(asmux);
		qmap->erase(qit);
		delete pquery; pquery=0;
	}
	// Virtual APPINSTANCE table
	else if(!_stricmp(from,"APPINSTANCE"))
	{
		// External system
		char rgn[32]={0};
		const char *rptr=stristr(where,"Region=='");
		if(rptr)
		{
			strncpy(rgn,rptr +9,sizeof(rgn)-1);
			rgn[sizeof(rgn)-1]=0;
			char *qptr=strchr(rgn,'\'');
			if(qptr) *qptr=0;
		}
		char sname[32]={0};
		const char *sptr=stristr(where,"AppSystem=='");
		if(sptr)
		{
			strncpy(sname,sptr +12,sizeof(sname)-1);
			sname[sizeof(sname)-1]=0;
			char *qptr=strchr(sname,'\'');
			if(qptr) *qptr=0;
		}
		if((*rgn)&&(*sname))
		{
			int ConPortNo=-1;
			AppSystem *rsys=GetRemoteSystem(rgn,sname,ConPortNo);
			if(rsys)
			{
				int nstr=(int)rsys->imap.size();
				const char **pstr=new const char *[nstr];
				int s=0;
				for(APPINSTMAP::iterator iit=rsys->imap.begin();iit!=rsys->imap.end();iit++)
					pstr[s++]=iit->second->iname.c_str();
				for(s=0;s<nstr;)
				{
					int nenc=nstr -s;
					SendSqlIndexResult(pquery,0,nstr,nstr,&pstr[s],nenc,false,"");
					s+=nenc;
				}
			}
			else
			{
				char reason[256]={0};
				sprintf(reason,"Region '%s', app system '%s' not found.",rgn,sname);
				int nenc=0;
				SendSqlIndexResult(pquery,-1,0,0,0,nenc,false,reason);
			}
			return;
		}

		WaitForSingleObject(asmux,INFINITE);
		// 'aibrowse' was pre-generated after AppSystem.iset modified
		// Requires where="AppSystem=="
		const char **pstr=0;
		char *ibuf=0;
		int nstr=0;
		AppSystem *asys=0;
		if(!strincmp(where,"AppSystem=='",12))
		{
			// Entitlement-driven instance browse list
			char wstr[32]={0};
			strncpy(wstr,where +12,sizeof(wstr)-1);
			wstr[sizeof(wstr)-1]=0;
			char *wptr=strchr(wstr,'\'');
			if(wptr) *wptr=0;
			strcat(wstr,"*");
			_strupr(wstr);
			asys=GetAppSystem(wstr);
			if(asys)
			{
				USERENTITLELIST *elist=(USERENTITLELIST *)fuser->DetPtr3;
				// Pre-generated instance browse list 'aibrowse'
				if((elist)&&(elist->size()>0)&&(elist->front().first=="*"))
				{
					nstr=(int)asys->imap.size() +1;
					pstr=asys->aibrowse;
				}
				// Entitled instances only
				else
				{
					pstr=new const char *[(int)asys->imap.size()];
					int ilen=(int)asys->imap.size()*12+1;
					ibuf=new char[ilen];
					memset(ibuf,0,ilen);
					char *iptr=ibuf; nstr=0;
					for(USERENTITLELIST::iterator eit=elist->begin();eit!=elist->end();eit++)
					{
						USERENTITLEMENT& ent=*eit;
						if(ent.first==asys->sname)
						{
							if(ent.second=="*")
							{
								for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
								{
									strcpy(iptr,iit->second->iname.c_str());
									pstr[nstr++]=iptr; iptr+=strlen(iptr)+1;
								}
							}
							else
							{
								strcpy(iptr,ent.second.c_str());
								pstr[nstr++]=iptr; iptr+=strlen(iptr)+1;
							}
						}
					}
				}
			}
		}
		if((nstr>0)&&(asys))
		{
			int proto=0;
			for(int i=0;i<nstr;)
			{
				int nenc=nstr -i;
				SendSqlIndexResult(pquery,0,i+nenc,nstr,&pstr[i],nenc,false,"");
				i+=nenc;
			}
		}
		// No results sent
		else
		{
			int nenc=0;
			SendSqlIndexResult(pquery,0,0,0,0,nenc,false,"");
		}
		if((ibuf)&&(pstr))
		{
			delete[] pstr; pstr=0;
			delete ibuf; ibuf=0;
		}
		ReleaseMutex(asmux);
		qmap->erase(qit);
		delete pquery; pquery=0;
	}
	// Virtual INDICES table (AppSystem,Key,Position,OrderLoc)
	else if(!_stricmp(from,"INDICES"))
	{
		// Requires single 'AppInstID==' and 'IndexName==' where conditions
		char ainame[64]={0};
		char *wptr=(char*)stristr(where,"AppInstID=='");
		if(wptr)
		{
			strncpy(ainame,wptr +12,sizeof(ainame)-1);
			wptr=strchr(ainame,'\'');
			if(wptr) *wptr=0;
		}
		char indname[64]={0};
		wptr=(char*)stristr(where,"IndexName=='");
		if(wptr)
		{
			strncpy(indname,wptr +12,sizeof(indname)-1);
			wptr=strchr(indname,'\'');
			if(wptr) *wptr=0;
		}
		// Optional 'Key==' where condition
		char kmatch[256]={0},kop[16]={0};
		LONGLONG kts=0;
		wptr=(char*)stristr(where,"Key=='");
		if(wptr)
		{
			strncpy(kmatch,wptr +6,sizeof(kmatch)-1);
			wptr=strchr(kmatch,'\'');
			if(wptr) *wptr=0;
			strcpy(kop,"==");
		}
	#ifdef IQSMP
		wptr=(char*)stristr(where,"Key>='");
		if(wptr)
		{
			strncpy(kmatch,wptr +6,sizeof(kmatch)-1);
			wptr=strchr(kmatch,'\'');
			if(wptr) *wptr=0;
			strcpy(kop,">=");
		}
	#endif
	#ifdef MULTI_DAY
		// Optional AND date (to only see index values for a single date)
		int kdate=0;
		wptr=(char*)stristr(where,"OrderDate==");
		//#ifdef MULTI_DAY_HIST
		//// DBOrderDate used when OrderDate key is the one we're looking up from historic instance
		//if(!wptr)
		//	wptr=(char*)stristr(where,"DBOrderDate==");
		//#endif
		if(wptr)
			kdate=myatoi(wptr +11);
	#endif

		int nstr=0,nsent=0;
		if((*indname)&&(*ainame))
		{
			WaitForSingleObject(asmux,INFINITE);
			// AppInstID condition can match '<app_system>*' or single '<app_instance>'
			AppSystem *asys=0;
			AppInstance *ainst=0;
			if(strrcmp(ainame,"*"))
			{
				AppSystem *ssys=GetAppSystem(ainame);
				if(ssys)
				{
					if(IsEntitled(fuser,ssys->sname.c_str(),0))
						asys=ssys;
				}
			}
			else
			{
				AppInstance *sinst=GetAppInstance(ainame);
				if(sinst)
				{
					if(IsEntitled(fuser,sinst->asys->sname.c_str(),sinst->iname.c_str()))
					{
						ainst=sinst; asys=ainst->asys;
					}
				}
			}
			if(asys)
			{
				if(true)
				{
					ReleaseMutex(asmux);

					OrderDB *idb=&asys->odb;
				#ifdef MULTI_DAY
					if((kdate>0)&&(idb->HISTORIC_DAYS>0))
					{
						OrderDB *pdb=asys->GetDB(kdate);
						if(pdb) idb=pdb;
					}
				#endif
					idb->Lock();
					idb->DoDelayInit();
					// Which index
					VSINDEX *pidx=0;
					TSINDEX *tidx=0;
				#ifdef MULTI_DAY
					DATEINDEX *didx=0;
				#endif
					bool single=false;
					// One-to-one hash_map indices
					if(!_stricmp(indname,"ClOrdID"))
					{
						pidx=&idb->omap;
						if(odb.INDEX_CLORDID_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"Filled Orders"))
						pidx=&idb->fomap;
					else if(!_stricmp(indname,"Open Orders"))
						pidx=&idb->oomap;
					if(pidx)
						single=true;
					// One-to-many multimap indices
					else if(!_stricmp(indname,"RootOrderID"))
					{
						pidx=&idb->pmap;
						if(odb.INDEX_ROOTORDERID_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"FirstClOrdID"))
					{
						pidx=&idb->fmap;
						if(odb.INDEX_FIRSTCLORDID_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"ClParentOrderID"))
					{
						pidx=&idb->cpmap;
						if(odb.INDEX_CLPARENTORDERID_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"Symbol"))
					{
						pidx=&idb->symmap;
						if(odb.INDEX_SYMBOL_CS)
							pquery->CaseSen=true;
					}
					//else if(!_stricmp(indname,"Price"))
					//else if(!_stricmp(indname,"Side"))
					else if(!_stricmp(indname,"Account"))
					{
						pidx=&idb->acctmap;
						if(odb.INDEX_ACCOUNT_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"EcnOrderID"))
					{
						pidx=&idb->omap2;
						if(odb.INDEX_ECNORDERID_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"ClientID"))
					{
						pidx=&idb->cimap;
						if(odb.INDEX_CLIENTID_CS)
							pquery->CaseSen=true;
					}
					else if(!_stricmp(indname,"TransactTime"))
					{
						tidx=&idb->tsmap;
						kts=_atoi64(kmatch);
					}
					else if(!_stricmp(indname,"Connection"))
					{
						pidx=&idb->cnmap;
						if(odb.INDEX_CONNECTIONS_CS)
							pquery->CaseSen=true;
					}
					//DT9398
					else
					{
						int idx=pquery->qscope.ValueInAuxKeys(indname);
						if(idx!=-1)
						{
							pidx=&idb->akmap[idx];
							if(odb.INDEX_AUXKEYS_CS)
								pquery->CaseSen=true;
						}
					}
				#ifdef MULTI_DAY
					#ifndef MULTI_DAY_ITEMLOC
					ITEMLOCSET odset;
					#endif
					if(!_stricmp(indname,"OrderDate"))
					{
						if(idb->HISTORIC_DAYS>0)
						{
							char **pstr=0;
							//if(strrcmp(ainame,"*"))
							{
								pquery->tpos=asys->odblist.size();
								pstr=new char *[pquery->tpos];
								for(list<OrderDB*>::iterator oit=asys->odblist.begin();oit!=asys->odblist.end();oit++)
								{
									#ifdef DNS_INSTANCES
									OrderDB *pdb=*oit;
									// This doesn't work because the detail file map hasn't been loaded 8(
									//bool skip=true;
									//DETAILFILEMAP& dfmap=pdb->GetDetailFiles();
									//for(DETAILFILEMAP::iterator dfit=dfmap.begin();dfit!=dfmap.end();dfit++)
									//{
									//	VSDetailFile *pdfile=dfit->second;
									//	if(pdfile->fsize.QuadPart>0)
									//	{
									//		skip=false; break;
									//	}
									//}
									//if(skip)
									//	continue;
									#endif
									pstr[nstr]=new char[64 +16 +1];
									sprintf(pstr[nstr],"%s\1%08d\1%d\1%X\1",
										#ifdef DNS_INSTANCES
										asys->sname.c_str(),pdb->wsdate,pquery->spos++,0);
										#else
										asys->sname.c_str(),(*oit)->wsdate,pquery->spos++,0);
										#endif
									nstr++;
								}
							}
							// TODO: We have no per-instance date index of orders, 
							// and I'm not sure if it's worth the memory overhead
							// Otherwise, only send dates for the specific app instance
							//else
							//{
							//	pstr=new char *[idb->odmap.size()];
							//	set<int> dset;
							//	for(DATEINDEX::iterator diit=idb->odmap.begin();diit!=idb->odmap.end();diit++)
							//		dset.insert(diit->first);
							//	for(set<int>::iterator dit=dset.begin();dit!=dset.end();dit++)
							//	{
							//		pstr[nstr]=new char[64 +16 +1];
							//		sprintf(pstr[nstr],"%s\1%08d\1%d\1%X\1",
							//			asys->sname.c_str(),*dit,pquery->spos++,0);
							//		nstr++;
							//	}
							//}
							// Send last buffers
							if(nstr>0)
							{
								//int proto=PROTO_DSV;
								for(int i=0;i<nstr;)
								{
									int nenc=nstr -i;
									SendSqlIndexResult(pquery,0,pquery->spos,pquery->tpos,(const char **)&pstr[i],nenc,false,"");
									i+=nenc; nsent+=nenc;
								}
							}
							// We've sent some and have no more leftovers but have not sent "no more" to client
							else
							{
								int nenc=0;
								SendSqlIndexResult(pquery,0,0,0,0,nenc,false,"");
							}
							delete pstr;
						}
						else if(idb->INDEX_ORDERDATE)
						{
							didx=&idb->odmap;
							kts=kdate;
						}
						// There's no index to walk; return just the current database date
						else
						{
							pquery->supos=pquery->spos;
							pquery->suval=pquery->miit.first;
							char *pstr=new char[64 +8 +1];
							sprintf(pstr,"%s\1%08d\1%d\1%X\1",
								asys->sname.c_str(),idb->wsdate,pquery->spos,0);
							nstr++;
							int nenc=1;
							SendSqlIndexResult(pquery,0,pquery->spos,pquery->tpos,(const char **)&pstr,nenc,false,"");
						}
					}
					#ifndef MULTI_DAY_ITEMLOC
					// This is kinda slow when we have hundreds of thousands of orders.
					// If the item location struct has the date in it, we don't need to join against a set of orders for the date. 
					else if(idb->INDEX_ORDERDATE)
					{
						// Add candidate orders to "join" set
						for(DATEINDEX::iterator diit=idb->odmap.find(kdate);
							(diit!=idb->odmap.end())&&(diit->first==kdate);
							diit++)
							odset.insert(diit->second);
					}
					#endif
					if((pidx)||(tidx)||(didx))
				#else
					if((pidx)||(tidx))
				#endif
					{
						if(!pquery->tpos)
						{
							if(tidx)
								pquery->tpos=(int)tidx->m_size();
						#ifdef MULTI_DAY
							else if(didx)
								pquery->tpos=(int)didx->size();
						#endif
							else if(single)
								pquery->tpos=(int)pidx->size();
							else
								pquery->tpos=(int)pidx->m_size();
						}
						LONGLONG lts=0;
						// TODO: 256 hasn't been verified as the optimum number of rows for 16K buffer
					#define BROWSE_RESULT_SIZE 256
						char **pstr=new char *[BROWSE_RESULT_SIZE];
						memset(pstr,0,BROWSE_RESULT_SIZE*sizeof(char *));
						// TODO: Use 'idir' for backward iteration
						// Query by start position (inefficient for large position values)
						bool restart=true;
						if(istart>1)
						{
							if(unique)
							{
								// The start position is set to the last unique search position +1
								if(istart==pquery->supos+1)
									restart=false;
								// TODO: make skipping around more efficient
								else if(istart>pquery->supos)
									;
								else//istart<pquery->supos
									;
							}
							else
							{
								// The start position is set to the last non-unique search position +1
								if(istart==pquery->spos+1)
									restart=false;
								// TODO: make skipping around more efficient
								else if(istart>pquery->spos)
									;
								else//istart<pquery->spos
									;
							}
						}
						// Next by 'iter' only
						else if((istart<1)&&(iter>0))
						{
							restart=false;
						}
						// Start at the beginning
						if(restart)
						{
							// Find start position by Key
							if(kmatch[0])
							{
								if(!strcmp(kop,"=="))
								{
									if(tidx)
										pquery->tiit=tidx->m_find(kts);
								#ifdef MULTI_DAY
									else if(didx)
										pquery->diit=didx->find((int)kts);
								#endif
									else if(single) 
										pquery->iit=pidx->find(kmatch);
									else 
										pquery->miit=pidx->m_find(kmatch);
									pquery->spos=pquery->supos=0;
								}
							#ifdef IQSMP
								else if(!strcmp(kop,">="))
								{
									pquery->spos=pquery->supos=0;
									if(tidx)
									{
										for(pquery->tiit=tidx->m_begin();
											(pquery->tiit!=tidx->m_end())&&(pquery->tiit.first<kts);
											pquery->tiit++)
											pquery->spos++;
										kts=0;
									}
								#ifdef MULTI_DAY
									else if(didx)
									{
										for(pquery->diit=didx->begin();
											(pquery->diit!=didx->end())&&(pquery->diit->first<kts);
											pquery->diit++)
											pquery->spos++;
										kts=0;
									}
								#endif
									else if(single) 
									{
										for(pquery->iit=pidx->begin();
											(pquery->iit!=pidx->end())&&(strcmp(pquery->iit.first.c_str(),kmatch)<0);
											pquery->iit++)
											pquery->spos++;
									}
									else 
									{
										for(pquery->miit=pidx->m_begin();
											(pquery->miit!=pidx->m_end())&&(strcmp(pquery->miit.first.c_str(),kmatch)<0);
											pquery->miit++)
											pquery->spos++;
									}
								}
							#endif
							}
							// Very beginning
							else
							{
								if(tidx) pquery->tiit=tidx->m_begin();
							#ifdef MULTI_DAY
								else if(didx) pquery->diit=didx->begin();
							#endif
								else if(single) pquery->iit=pidx->begin();
								else pquery->miit=pidx->m_begin();
								pquery->spos=pquery->supos=0;
							}
						}
						bool more=true;
						while(true)
						{
							// Return AppSystem,Key,Position,OrderLoc,
							// Time index
							if(tidx)
							{
								if(pquery->tiit==tidx->m_end())
								{
									more=false;
									break;
								}
								pquery->spos++;
								// Unique keys only
								if((unique)&&(pquery->tiit.first==lts))
								{
									pquery->tiit++;
									continue;
								}
								// Named key browse
								if((kts)&&(kts!=pquery->tiit.first))
								{
									more=false;
									break;
								}
								// Narrow by app instance
								//if((ainst)&&(ainst->oset.find((LONGLONG)pquery->tiit.second)==ainst->oset.end()))
								if((ainst)&&(ainst->oset.find(pquery->tiit.second)==ainst->oset.end()))
								{
									pquery->tiit++;
									continue;
								}
								// Skip to position requested
								if((istart>0)&&(pquery->spos<istart))
								{
									pquery->tiit++;
									continue;
								}
								// "join" on OrderDate
								#ifdef MULTI_DAY_ITEMLOC
								if((kdate>0)&&(odb.HISTORIC_DAYS<=0)&&(odb.INDEX_ORDERDATE)&&(pquery->tiit.second.wsdate!=kdate))
								#else
								if((kdate>0)&&(odb.HISTORIC_DAYS<=0)&&(odb.INDEX_ORDERDATE)&&(odset.find(pquery->tiit.second)==odset.end()))
								#endif
								{
									pquery->tiit++;
									continue;
								}
								pquery->supos=pquery->spos;
								lts=pquery->tiit.first;
								pstr[nstr]=new char[64 +16 +1];
								sprintf(pstr[nstr],"%s\1%I64d\1%d\1%X\1",
									asys->sname.c_str(),pquery->tiit.first,pquery->spos,pquery->tiit.second);
								nstr++;
							}
						#ifdef MULTI_DAY
							else if(didx)
							{
								if(pquery->diit==didx->end())
								{
									more=false;
									break;
								}
								pquery->spos++;
								// Unique keys only
								if((unique)&&(pquery->diit->first==(int)lts))
								{
									pquery->diit++;
									continue;
								}
								// Named key browse
								if((kts)&&(kts!=pquery->diit->first))
								{
									more=false;
									break;
								}
								// Narrow by app instance
								//if((ainst)&&(ainst->oset.find((LONGLONG)pquery->diit->second)==ainst->oset.end()))
								if((ainst)&&(ainst->oset.find(pquery->diit->second)==ainst->oset.end()))
								{
									pquery->diit++;
									continue;
								}
								// Skip to position requested
								if((istart>0)&&(pquery->spos<istart))
								{
									pquery->diit++;
									continue;
								}
								pquery->supos=pquery->spos;
								lts=pquery->diit->first;
								pstr[nstr]=new char[64 +16 +1];
								sprintf(pstr[nstr],"%s\1%d\1%d\1%X\1",
									asys->sname.c_str(),pquery->diit->first,pquery->spos,pquery->diit->second);
								nstr++;
							}
						#endif
							// One-to-one index
							else if(single)
							{
								if(pquery->iit==pidx->end())
								{
									more=false;
									break;
								}
								pquery->spos++;
								// Unique keys only
								if((unique)&&(pquery->iit.first==pquery->suval))
								{
									pquery->iit++;
									continue;
								}
								// Named key browse
								if((kmatch[0])&&
									(pquery->CaseSen ? strcmp(kmatch,pquery->iit.first.c_str()) : _stricmp(kmatch,pquery->iit.first.c_str()))!=0)
								{
									more=false;
									break;
								}
								// Narrow by app instance
								//if((ainst)&&(ainst->oset.find((LONGLONG)pquery->iit.second)==ainst->oset.end()))
								if((ainst)&&(ainst->oset.find(pquery->iit.second)==ainst->oset.end()))
								{
									pquery->iit++;
									continue;
								}
								// Skip to position requested
								if((istart>0)&&(pquery->spos<istart))
								{
									pquery->iit++;
									continue;
								}
								// "join" on OrderDate
								#ifdef MULTI_DAY_ITEMLOC
								if((kdate>0)&&(odb.HISTORIC_DAYS<=0)&&(odb.INDEX_ORDERDATE)&&(pquery->iit.second.wsdate!=kdate))
								#else
								if((kdate>0)&&(odb.HISTORIC_DAYS<=0)&&(odb.INDEX_ORDERDATE)&&(odset.find(pquery->iit.second)==odset.end()))
								#endif
								{
									pquery->iit++;
									continue;
								}
								pquery->supos=pquery->spos;
								pquery->suval=pquery->iit.first;
								pstr[nstr]=new char[64 +pquery->iit.first.length() +1];
							#ifdef MULTI_DAY
								// These indices have the date prefixed
								if((idb->INDEX_ORDERDATE)&&
								   ((!_stricmp(indname,"ClOrdID"))||(!_stricmp(indname,"Filled Orders"))||(!_stricmp(indname,"Open Orders"))))
									sprintf(pstr[nstr],"%s\1%s\1%d\1%X\1",
										asys->sname.c_str(),pquery->iit.first.c_str() +9,pquery->spos,pquery->iit.second);
								else
									sprintf(pstr[nstr],"%s\1%s\1%d\1%X\1",
										asys->sname.c_str(),pquery->iit.first.c_str(),pquery->spos,pquery->iit.second);
							#else
								sprintf(pstr[nstr],"%s\1%s\1%d\1%X\1",
									asys->sname.c_str(),pquery->iit.first.c_str(),pquery->spos,pquery->iit.second);
							#endif
								nstr++;
							}
							// Multimap index
							else
							{
								if(pquery->miit==pidx->m_end())
								{
									more=false;
									break;
								}
								pquery->spos++;
								// Unique keys only
								if((unique)&&(pquery->miit.first==pquery->suval))
								{
									pquery->miit++;
									continue;
								}
								// Named key browse
								if((kmatch[0])&&
									(pquery->CaseSen ? strcmp(kmatch,pquery->iit.first.c_str()) : _stricmp(kmatch,pquery->miit.first.c_str()))!=0)
								{
									more=false;
									break;
								}
								// Narrow by app instance
								//if((ainst)&&(ainst->oset.find((LONGLONG)pquery->miit.second)==ainst->oset.end()))
								if((ainst)&&(ainst->oset.find(pquery->miit.second)==ainst->oset.end()))
								{
									pquery->miit++;
									continue;
								}
								// Skip to position requested
								if((istart>0)&&(pquery->spos<istart))
								{
									pquery->miit++;
									continue;
								}
								// "join" on OrderDate
								#ifdef MULTI_DAY_ITEMLOC
								if((kdate>0)&&(odb.HISTORIC_DAYS<=0)&&(odb.INDEX_ORDERDATE)&&(pquery->miit.second.wsdate!=kdate))
								#else
								if((kdate>0)&&(odb.HISTORIC_DAYS<=0)&&(odb.INDEX_ORDERDATE)&&(odset.find(pquery->miit.second)==odset.end()))
								#endif
								{
									pquery->miit++;
									continue;
								}
								pquery->supos=pquery->spos;
								pquery->suval=pquery->miit.first;
								pstr[nstr]=new char[64 +pquery->miit.first.length() +1];
								sprintf(pstr[nstr],"%s\1%s\1%d\1%X\1",
									asys->sname.c_str(),pquery->miit.first.c_str(),pquery->spos,pquery->miit.second);
								nstr++;
							}
							// Maximum number of orders before next iteration
							if((maxorders>0)&&(nsent +nstr>=maxorders))
								break;
							// Send a block as soon as it fills up
							if(nstr>=BROWSE_RESULT_SIZE)
							{
								int proto=PROTO_DSV;
								for(int i=0;i<nstr;)
								{
									int nenc=nstr -i;
									SendSqlIndexResult(pquery,0,pquery->spos,pquery->tpos,(const char **)&pstr[i],nenc,true,"");
									i+=nenc; nsent+=nenc;
								}
								memset(pstr,0,BROWSE_RESULT_SIZE*sizeof(char *)); nstr=0;
							}
							if(tidx) pquery->tiit++;
						#ifdef MULTI_DAY
							else if(didx) pquery->diit++;
						#endif
							else if(single) pquery->iit++;
							else pquery->miit++;
						}
						// Save last unique value for unique value iteration
						// Send last buffers
						if(nstr>0)
						{
							//int proto=PROTO_DSV;
							for(int i=0;i<nstr;)
							{
								int nenc=nstr -i;
								SendSqlIndexResult(pquery,0,pquery->spos,pquery->tpos,(const char **)&pstr[i],nenc,false,"");
								i+=nenc; nsent+=nenc;
							}
						}
						// We've sent some and have no more leftovers but have not sent "no more" to client
						else if((nsent>0)&&(!more))
						{
							int nenc=0;
							SendSqlIndexResult(pquery,0,pquery->spos,pquery->tpos,0,nenc,false,"");
						}
						delete pstr;
					}
					idb->Unlock();
				}//IsEntitled system
				else
				{
					ReleaseMutex(asmux);
					char reason[256]={0};
					sprintf(reason,"User not entitled to app system '%s'.",asys->sname.c_str());
					if(LOG_QUERIES>0)
						WSLogEvent("USR%d: %s",pquery->PortNo,reason);
					int nenc=0;
					SendSqlIndexResult(pquery,-1,0,0,0,nenc,false,reason);
					return;
				}
			}
			else
				ReleaseMutex(asmux);
		}
		// No results sent
		if(nsent<1)
		{
			int nenc=0;
			// Send 'eiter' to allow browsing after reaching the end, 
			// but endpos=tpos to let the client know we've reached the end
			SendSqlIndexResult(pquery,0,pquery->tpos,pquery->tpos,0,nenc,false,"");
			// Don't delete the query to allow browsing after reaching the end
			//qmap->erase(qit);
			//delete pquery; pquery=0;
		}
	}
	// Invalid from
	else
	{
		int nenc=0;
		char reason[256]={0};
		sprintf(reason,"Invalid from(%s) table.",from);
		SendSqlIndexResult(pquery,-1,0,0,0,nenc,false,reason);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	return;
}

#ifdef TASK_THREAD_POOL
int ViewServer::TaskThread()
{
	while(!taskThreadAbort)
	{
		DWORD rbytes=0;
		ULONG_PTR ckey=0;
		LPOVERLAPPED povl=0;
		GetQueuedCompletionStatus(taskIocpPort,&rbytes,&ckey,&povl,1000);
		if(rbytes>0)
		{
			_ASSERT((ViewServer*)ckey==this);
			TASKOVL *tovl=(TASKOVL *)povl;
			switch(tovl->taskid)
			{
			case TASK_SQL_DETAILS:
				SendSqlDetailsLive(tovl->fuser,tovl->skey,tovl->taskid,tovl->rid,tovl->pquery,&tovl->fhint);
				if(tovl->fhint.pfix)
				{
					delete tovl->fhint.pfix; tovl->fhint.pfix=0;
				}
				break;
			case TASK_SQL_ORDERS:
				SendSqlOrdersLive(tovl->fuser,tovl->skey,tovl->taskid,tovl->rid,tovl->pquery,&tovl->fhint);
				if(tovl->fhint.porder)
				{
					if(tovl->fhint.unload)
					{
						VSINDEX::iterator ooit=tovl->fhint.asys->odb.oomap.find(tovl->fhint.porder->GetOrderKey());
						if(ooit!=tovl->fhint.asys->odb.oomap.end())
							tovl->fhint.asys->odb.oomap.erase(ooit);
						#ifdef UNLOAD_TERM_ORDERS
						UnloadOrder(tovl->fhint.unldUscPortNo,tovl->fhint.asys,tovl->fhint.porder,tovl->fhint.unldLoc); tovl->fhint.porder=0;
						#endif
					}
					else
					{
						tovl->fhint.asys->odb.FreeOrder(tovl->fhint.porder); tovl->fhint.porder=0;
					}
				}
				break;
			case TASK_SQL_ACCOUNTS:
				SendSqlAccountsLive(tovl->fuser,tovl->skey,tovl->taskid,tovl->rid,tovl->pquery,&tovl->fhint);
				break;
			default:
				_ASSERT(false);
			};
			delete tovl;
		}
	}
	return 0;
}
#endif

VSQuery::~VSQuery()
{
	for(LiveQueryMap::iterator itr=LiveQueries.begin(); itr!=LiveQueries.end();itr++)
	{
		delete ((*itr).second);
	}
	LiveQueries.clear();

	for(VSRESULTLIST::iterator rit=hresults.begin();rit!=hresults.end();rit++)
		delete *rit;
	hresults.clear();
	for(VSRESULTLIST::iterator rit=lresults.begin();rit!=lresults.end();rit++)
		delete *rit;
	lresults.clear();
}

uchar ViewServer::ConvertKeyType2TaskID(KEYTYPE keytype)
{
	if(keytype==KEY_APPSYSTEM)	return TASK_APPSYS;
	if(keytype==KEY_APPINSTID)	return TASK_APPINST;
	if(keytype==KEY_CLORDID)	return TASK_CLORDID;
	if(keytype==KEY_ROOTORDERID)	return TASK_ROOTORDERID;
	if(keytype==KEY_FIRSTCLORDID)	return TASK_FIRSTCLORDID;
	if(keytype==KEY_CLPARENTORDERID) return TASK_PARENTCLORDID;
	if(keytype==KEY_SYMBOL)		return TASK_SYMBOL;
	if(keytype==KEY_ACCOUNT)	return TASK_ACCOUNT;
	if(keytype==KEY_ECNORDERID)	return TASK_ECNORDERID;
	if(keytype==KEY_CLIENTID)	return TASK_CLIENTID;
	return 0;
}

bool ViewServer::AddLiveQuery(VSQuery* pquery, FeedUser *fuser)
{
	pquery->pmod=this;

	// Virtual DETAILS table
	if(!_stricmp(pquery->from.c_str(),"DETAILS"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"DETAILS",TASK_SQL_DETAILS,pquery,pquery->rid,_PostTaskThread);
		#else
		string key;
		KEYTYPE qt;
		if(pquery->LiveQueryKey(key,qt))
			 pquery->ptask=taskmgr.AddTask(fuser,key,ConvertKeyType2TaskID(qt),pquery,pquery->rid,_SendSqlDetailsLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual ORDERS table
	else if(!_stricmp(pquery->from.c_str(),"ORDERS"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"ORDERS",TASK_SQL_ORDERS,pquery,pquery->rid,_PostTaskThread);
		#else
		string key;
		KEYTYPE qt;
		if(pquery->LiveQueryKey(key,qt))
			pquery->ptask=taskmgr.AddTask(fuser,key,ConvertKeyType2TaskID(qt),pquery,pquery->rid,_SendSqlOrdersLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual ACCOUNTS table
	else if(!_stricmp(pquery->from.c_str(),"ACCOUNTS"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"ACCOUNTS",TASK_SQL_ACCOUNTS,pquery,pquery->rid,_PostTaskThread);
		#else
		pquery->ptask=taskmgr.AddTask(fuser,"ACCOUNTS",TASK_SQL_ACCOUNTS,pquery,pquery->rid,_SendSqlAccountsLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	#ifdef IQDNS
	// Virtual USERS table
	else if(!_stricmp(pquery->from.c_str(),"USERS"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"USERS",TASK_SQL_USERS,pquery,pquery->rid,_PostTaskThread);
		#else
		pquery->ptask=taskmgr.AddTask(fuser,"USERS",TASK_SQL_USERS,pquery,pquery->rid,_SendSqlUsersLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual VARSID table
	else if(!_stricmp(pquery->from.c_str(),"VARSID"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"VARSID",TASK_SQL_VARSID,pquery,pquery->rid,_PostTaskThread);
		#else
		pquery->ptask=taskmgr.AddTask(fuser,"VARSID",TASK_SQL_VARSID,pquery,pquery->rid,_SendSqlVarsidLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual DOMREL table
	else if(!_stricmp(pquery->from.c_str(),"DOMREL"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"DOMREL",TASK_SQL_DOMREL,pquery,pquery->rid,_PostTaskThread);
		#else
		pquery->ptask=taskmgr.AddTask(fuser,"DOMREL",TASK_SQL_DOMREL,pquery,pquery->rid,_SendSqlDomrelLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual DOMTREE table
	else if(!_stricmp(pquery->from.c_str(),"DOMTREE"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"DOMTREE",TASK_SQL_DOMTREE,pquery,pquery->rid,_PostTaskThread);
		#else
		pquery->ptask=taskmgr.AddTask(fuser,"DOMTREE",TASK_SQL_DOMTREE,pquery,pquery->rid,_SendSqlDomtreeLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual DOMTREE table
	else if(!_stricmp(pquery->from.c_str(),"SERVERS"))
	{
		#ifdef TASK_THREAD_POOL
		pquery->ptask=taskmgr.AddTask(fuser,"SERVERS",TASK_SQL_SERVERS,pquery,pquery->rid,_PostTaskThread);
		#else
		pquery->ptask=taskmgr.AddTask(fuser,"SERVERS",TASK_SQL_SERVERS,pquery,pquery->rid,_SendSqlServersLive);
		#endif
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	#endif
	// Invalid from
	else
	{
		_ASSERT(false);
		return false;
	}

	return true;
}

#ifdef SPECTRUM
//DT10472
KEYTYPE ViewServer::ConditionMatch(const char* str)
{
	if(!strcmp(str,"AppInstID") || !strncmp(str,"AppInstID ",10))
	{
		if(strchr(str,'*'))
			return KEY_APPSYSTEM;
		return KEY_APPINSTID;
	}
	if(!strcmp(str,"AppSystem") || !strncmp(str,"AppSystem ",10))		return KEY_APPSYSTEM;
	if(!strcmp(str,"ClOrdID") || !strncmp(str,"ClOrdID ",8))		return KEY_CLORDID;
	if(!strcmp(str,"RootOrderID") || !strncmp(str,"RootOrderID ",12))	return KEY_ROOTORDERID;
	if(!strcmp(str,"FirstClOrdID") || !strncmp(str,"FirstClOrdID ",13))	return KEY_FIRSTCLORDID;
	if(!strcmp(str,"ClParentOrderID") || !strncmp(str,"ClParentOrderID ",16))	return KEY_CLPARENTORDERID;
	if(!strcmp(str,"Symbol") || !strncmp(str,"Symbol ",7))			return KEY_SYMBOL;
	if(!strcmp(str,"Account") || !strncmp(str,"Account ",8))		return KEY_ACCOUNT;
	if(!strcmp(str,"EcnOrderID") || !strncmp(str,"EcnOrderID ",11))		return KEY_ECNORDERID;
	if(!strcmp(str,"ClientID") || !strncmp(str,"ClientID ",9))		return KEY_CLIENTID;
	if(!strcmp(str,"HighMsgType") || !strncmp(str,"HighMsgType ",12))	return KEY_HIGHMSGTYPE;
	if(!strcmp(str,"HighExecType") || !strncmp(str,"HighExecType ",13))	return KEY_HIGHEXECTYPE;
	if(!strcmp(str,"OrderQty") || !strncmp(str,"OrderQty ",9))		return KEY_ORDERQTY;
	if(!strcmp(str,"CumQty") || !strncmp(str,"CumQty ",7))			return KEY_CUMQTY;
	if(!strcmp(str,"FillQty") || !strncmp(str,"FillQty ",8))		return KEY_FILLQTY;
	if(!strcmp(str,"OpenOrder") || !strncmp(str,"OpenOrder ",10))		return KEY_OPENORDER;
	if(!strcmp(str,"Term") || !strncmp(str,"Term ",5))			return KEY_TERM;
	if(!strcmp(str,"TransactTime") || !strncmp(str,"TransactTime ",13))	return KEY_TRANSACTTIME;
	if(!strcmp(str,"Connection") || !strncmp(str,"Connection ",11))		return KEY_CONNECTION;
	if(!strcmp(str,"OrderLoc") || !strncmp(str,"OrderLoc ",9))		return KEY_ORDERLOC;
	for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
	{
		string tmp=odb.AUXKEY_NAMES[i];
		if(!strcmp(str,tmp.c_str()))
		{
			return (KEYTYPE)(KEY_AUXKEY1+(KEYTYPE)i);
		}		
		tmp+=string(" ");
		if(!strncmp(str,tmp.c_str(),tmp.size()))
		{
			return (KEYTYPE)(KEY_AUXKEY1+(KEYTYPE)i);
		}
	}
	return KEY_UNKNOWN;
}

bool GetMap(KEYTYPE type,OrderDB& odb,VSINDEX* &idx)
{
	idx=0;
	switch(type)
	{
		case KEY_CLORDID:		idx=&(odb.omap);     break;
		case KEY_ROOTORDERID:		idx=&(odb.pmap);     break;
		case KEY_FIRSTCLORDID:		idx=&(odb.fmap);     break;
		case KEY_CLPARENTORDERID:	idx=&(odb.cpmap);    break;
		case KEY_SYMBOL:		idx=&(odb.symmap);   break;
		case KEY_ACCOUNT:		idx=&(odb.acctmap);  break;
		case KEY_ECNORDERID:		idx=&(odb.omap2);    break;
		case KEY_CLIENTID:		idx=&(odb.cimap);    break;
		case KEY_TERM:			idx=&(odb.fomap);    break;
		case KEY_OPENORDER:		idx=&(odb.oomap);    break;
		case KEY_CONNECTION:		idx=&(odb.cnmap);    break;
		case KEY_AUXKEY1:		idx=&(odb.akmap[0]); break;
		case KEY_AUXKEY2:		idx=&(odb.akmap[1]); break;
		case KEY_AUXKEY3:		idx=&(odb.akmap[2]); break;
		case KEY_AUXKEY4:		idx=&(odb.akmap[3]); break;
		case KEY_AUXKEY5:		idx=&(odb.akmap[4]); break;
		case KEY_AUXKEY6:		idx=&(odb.akmap[5]); break;
		case KEY_HIGHMSGTYPE:		break;
		case KEY_HIGHEXECTYPE:		break;
		case KEY_ORDERQTY:		break;
		case KEY_CUMQTY:		break;
		case KEY_FILLQTY:		break;
		case KEY_TRANSACTTIME:		break;
		case KEY_ORDERLOC:		break;
		default:			break;
	}
	return (idx ? true : false);
}
	
// Function to parse a list in for following format: ('A','B','C')
bool InList(string where, vector<string>& Tokens)
{
	if(where.find('(')==string::npos || where.find(')')==string::npos)
		return false;

	size_t len=where.find_last_of(')') - where.find_first_of('(') -1;
	string inList=where.substr(where.find_first_of('(')+1,len);	
	int inListClauselen=inList.size();
	bool inSingleQuote=false;
	size_t begin=string::npos;
	for(int i=0; i<inListClauselen; i++)
	{
		if(begin==string::npos)
			begin=i;

		if(inSingleQuote)
		{
			if(inList[i]=='\'')
			inSingleQuote=false;
			continue;
		}
		if(inList[i]=='\'' && !inSingleQuote)
		{
			inSingleQuote=true;
		}
		else if(inList[i]==',')
		{
			Tokens.push_back(inList.substr(begin,i-begin));
			begin=string::npos;
		}
	}
	if(begin!=string::npos)
		Tokens.push_back(inList.substr(begin));

	for(int i=0;i<(int)Tokens.size();i++)
	{
		while(Tokens[i].find('\'')!=string::npos)
			Tokens[i].erase(Tokens[i].find('\''),1);
		while(Tokens[i].find(')')!=string::npos)
			Tokens[i].erase(Tokens[i].find(')'),1);
		while(Tokens[i].find('(')!=string::npos)
			Tokens[i].erase(Tokens[i].find('('),1);
	}
	return true;
}

WhereSegment::WhereSegment(string seg, WHERE_OPS op, APPSYSMAP* asmap)
{
	Segment=seg;
	oper=op;
	for(APPSYSMAP::iterator asItr=asmap->begin(); asItr!=asmap->end(); asItr++)
	{
		cqResultSet.insert(make_pair<string,ITEMLOCSET>((*asItr).first,ITEMLOCSET()));
	}
}

// Returns TRUE if the query is handled (successfully or otherwise) and FALSE if it cannot be handled.
bool ViewServer::ComplexQuery(VSQuery* pquery, VSQUERYMAP* qmap, VSQUERYMAP::iterator& qit,FeedUser *fuser)
{
	if(!pquery || !qmap || !fuser)
		return false;

	if(!pquery->hist)
		return false;

    if(_stricmp(pquery->from.c_str(),"DETAILS") && _stricmp(pquery->from.c_str(),"ORDERS"))
		return false;
	
	// Parse the where clause into segments
	list<WhereSegment> WhereList;
	int NumOfSegments=BuildWhereSegments(WhereList,pquery->where);

	// Handle only "IN"/"NOT IN" based queries and/or multi-queries
	if(pquery->where.find(" IN ")==string::npos && NumOfSegments<2)
		return false;

	pquery->complexquery=true;
	pquery->cqResultSetItr=pquery->cqResultSet.end();

	int proto=PROTO_UNKNOWN;
    if(pquery->binarydatareq)
        proto=PROTO_DAT;
    else
    {
        if(!_stricmp(pquery->from.c_str(),"DETAILS"))
            proto=PROTO_FIX;
        else if(!_stricmp(pquery->from.c_str(),"ORDERS"))
            proto=PROTO_DSV;
    }
	if(proto==PROTO_UNKNOWN)
	{
		int nenc=0;
		pquery->morehist=false;
		SendSqlResult(pquery,-1,0,0,pquery->hist,proto,nenc,false,"Complex query - table not supported");
		if(qmap)
			qmap->erase(qit);
		delete pquery;
		pquery=0;
		return true;
	}

	// Initializee the pquery's result set
	for(APPSYSMAP::iterator asItr=asmap.begin(); asItr!=asmap.end(); asItr++)
	{
		pquery->cqResultSet.insert(make_pair<string,ITEMLOCSET>((*asItr).first,ITEMLOCSET()));
	}

	// Iterate thru each where segment
	for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
	{
		// Handle the secondary keys later
		KEYTYPE keytype=ConditionMatch((*segItr).Segment.c_str());
		if(keytype==KEY_HIGHMSGTYPE || keytype==KEY_HIGHEXECTYPE || keytype==KEY_ORDERQTY || keytype==KEY_CUMQTY || keytype==KEY_FILLQTY ||
			keytype==KEY_OPENORDER || keytype==KEY_TERM)
		{
			continue;
		}

		vector<string> Tokens;
		if(!InList((*segItr).Segment,Tokens))
		{
			int nenc=0;
			pquery->morehist=false;
			SendSqlResult(pquery,-1,0,0,pquery->hist,proto,nenc,false,"Complex query - InList error");
			if(qmap)
				qmap->erase(qit);
			delete pquery;
			pquery=0;
			return true;
		}

		for(int i=0; i<(int)Tokens.size(); i++)
		{
			if(!ComplexQuerySearch(pquery,ConditionMatch((*segItr).Segment.c_str()),Tokens[i],&(*segItr)))
			{
				int nenc=0;
				pquery->morehist=false;
				SendSqlResult(pquery,-1,0,0,pquery->hist,proto,nenc,false,"Complex query - Search failed");
				if(qmap)
					qmap->erase(qit);
				delete pquery;
				pquery=0;
				return true;
			}
		}
	}

	//TEMP
	/*
	for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
	{
		WhereSegment& ws=(*segItr);
		for(CQRESULTLISTSET::iterator cqrsItr=ws.cqResultSet.begin(); cqrsItr!=ws.cqResultSet.end(); cqrsItr++)
		{
			WSLogEvent("The result set for %s in appsys %s is %d",ws.Segment.c_str(),(*cqrsItr).first.c_str(),(*cqrsItr).second.size());
		}
	}
	*/

	// Union or intersection the sets based on AND or OR
	for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
	{
		WhereSegment& ws=(*segItr);

		// Handle the secondary keys later
		KEYTYPE keytype=ConditionMatch(ws.Segment.c_str());
		if(keytype==KEY_HIGHMSGTYPE || keytype==KEY_HIGHEXECTYPE || keytype==KEY_ORDERQTY || keytype==KEY_CUMQTY || keytype==KEY_FILLQTY ||
			keytype==KEY_OPENORDER || keytype==KEY_TERM)
		{
			continue;
		}

		for(APPSYSMAP::iterator asItr=asmap.begin(); asItr!=asmap.end(); asItr++)
		{
			CQRESULTLISTSET::iterator cqrsItr=ws.cqResultSet.find((*asItr).first);
			CQRESULTLISTSET::iterator permItr=pquery->cqResultSet.find((*asItr).first);

			if(cqrsItr==ws.cqResultSet.end())
			{
				if(ws.oper==OPS_AND)
					(*permItr).second.erase((*permItr).second.begin(),(*permItr).second.end());
				continue;
			}

			vector<LONGLONG> tempSet((*permItr).second.size()+(*cqrsItr).second.size());
			vector<LONGLONG>::iterator tempSetItr;

			if(ws.oper==OPS_AND)
			{
				if(strstr((*segItr).Segment.c_str()," NOT IN "))
				{
					tempSetItr=set_difference((*permItr).second.begin(),(*permItr).second.end(),
						(*cqrsItr).second.begin(),(*cqrsItr).second.end(),tempSet.begin());
				}
				else
				{
					tempSetItr=set_intersection((*permItr).second.begin(),(*permItr).second.end(),
						(*cqrsItr).second.begin(),(*cqrsItr).second.end(),tempSet.begin());
				}
			}
			else // oper==OPS_OR
			{
				if(strstr((*segItr).Segment.c_str()," NOT IN "))
				{
					WSLogEvent(" OR NOT INs not allowed");
					int nenc=0;
					pquery->morehist=false;
					SendSqlResult(pquery,-1,0,0,pquery->hist,proto,nenc,false,"Invalid NOT IN query (standalone or OR'ed)");
					if(qmap)
						qmap->erase(qit);
					delete pquery;
					pquery=0;
					return true;
				}
				else
				{
					tempSetItr=set_union((*permItr).second.begin(),(*permItr).second.end(),
						(*cqrsItr).second.begin(),(*cqrsItr).second.end(),tempSet.begin());
				}
			}
			
			// Clear the previous set
			(*permItr).second.clear();
	
			// Populate the permanent set
			if(tempSetItr!=tempSet.begin())
			{
				(*permItr).second.insert(tempSet.begin(),tempSetItr);
			}
		}
	}

	//TEMP
	/*
	for(CQRESULTLISTSET::iterator cqrsItr=pquery->cqResultSet.begin(); cqrsItr!=pquery->cqResultSet.end(); cqrsItr++)
	{
		WSLogEvent("Union or intersection (second check) for appsys %s is %d",(*cqrsItr).first.c_str(),(*cqrsItr).second.size());
	}
	*/

	// Handle secondary keys
	for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
	{
		WhereSegment& ws=(*segItr);

		KEYTYPE keytype=ConditionMatch(ws.Segment.c_str());

		if(keytype!=KEY_HIGHMSGTYPE && keytype!=KEY_HIGHEXECTYPE && keytype!=KEY_ORDERQTY && keytype!=KEY_CUMQTY && keytype!=KEY_FILLQTY &&
			keytype!=KEY_OPENORDER && keytype!=KEY_TERM)
			continue;

		if(ws.oper!=OPS_AND)
		{
			int nenc=0;
			pquery->morehist=false;
			SendSqlResult(pquery,-1,0,0,pquery->hist,proto,nenc,false,"Complex query - Cannot OR a secondary search key");
			if(qmap)
				qmap->erase(qit);
			delete pquery;
			pquery=0;
			return true;
		}

		bool notIn=false;
		if(ws.Segment.find(" NOT IN ")!=string::npos)
			notIn=true;

		vector<string> Tokens;
		InList(ws.Segment,Tokens);
		for(CQRESULTLISTSET::iterator permItr=pquery->cqResultSet.begin(); permItr!=pquery->cqResultSet.end(); permItr++)
		{
			APPSYSMAP::iterator asit=asmap.find((*permItr).first);
			if(asit==asmap.end())
				continue;

			OrderDB *pdb=&((*asit).second->odb);
			for(set<LONGLONG>::iterator sItr=(*permItr).second.begin(); sItr!=(*permItr).second.end(); )
			{
				VSOrder* porder=pdb->ReadOrder((*sItr));
				bool found=false;
				if(!porder)
				{
					(*permItr).second.erase(sItr++);
					continue;
				}

				if(ws.Segment==string("OpenOrder"))
				{
					VSINDEX::iterator ooit=pdb->oomap.find(porder->GetClOrdID());
					if((notIn && ooit!=pdb->oomap.end()) || (!notIn && ooit==pdb->oomap.end()))
					{
						(*permItr).second.erase(sItr++);
					}
					else
					{
						sItr++;
					}
					continue;
				}

				if(ws.Segment==string("Term"))
				{
					VSINDEX::iterator ooit=pdb->oomap.find(porder->GetClOrdID());
					if((notIn && ooit!=pdb->oomap.end()) || (!notIn && ooit==pdb->oomap.end()))
					{
						sItr++;
					}
					else
					{
						(*permItr).second.erase(sItr++);
					}
					continue;
				}

				for(int i=0; i<(int)Tokens.size() && !found; i++)
				{
					switch(ConditionMatch(ws.Segment.c_str()))
					{
					case KEY_HIGHMSGTYPE:
						if(Tokens[i][0]==porder->GetHighMsgType())
							found=true;
						break;
					case KEY_HIGHEXECTYPE:
						if(Tokens[i][0]==porder->GetHighExecType())
							found=true;
						break;
					case KEY_ORDERQTY:
						if(atol(Tokens[i].c_str())==porder->GetOrderQty())
							found=true;
						break;
					case KEY_CUMQTY:
						if(atol(Tokens[i].c_str())==porder->GetCumQty())
							found=true;
						break;
					case KEY_FILLQTY:
						if(atol(Tokens[i].c_str())==porder->GetFillQty())
							found=true;
						break;
					default:
						break;
					}
				}

				if((notIn && found) || (!notIn && !found))
				{
					#ifdef WIN32
					sItr=(*permItr).second.erase(sItr);
					#else
					(*permItr).second.erase(sItr++);
					#endif
				}
				else
				{
					sItr++;
				}
			}
		}
	}

	// Get the total of the result set
	for(CQRESULTLISTSET::iterator itr=pquery->cqResultSet.begin(); itr!=pquery->cqResultSet.end(); itr++)
	{
		if(proto==PROTO_FIX)
		{
			APPSYSMAP::iterator asItr=asmap.find((*itr).first);
			if(asItr!=asmap.end())
			{
				AppSystem* as=(*asItr).second;
				for(ITEMLOCSET::iterator ilItr=(*itr).second.begin(); ilItr!=(*itr).second.end(); ilItr++)
				{
					VSOrder* porder=as->odb.ReadOrder(*ilItr);
					if(porder)
						pquery->tpos+=porder->GetNumDetails();
				}
			}
		}
		else
		{
			pquery->tpos+=(*itr).second.size();
		}
	}

	/*
	WSLogEvent("Union or intersection the sets. total:%d",pquery->tpos);
	*/

	if(pquery->maxbrowse<pquery->tpos)
	{
		int nenc=0;
		pquery->morehist=false;
		char errStr[128]={0};
		sprintf(errStr,"Too many results(%d), please narrow search %d results or less",pquery->tpos,pquery->maxbrowse);
		SendSqlResult(pquery,-1,0,0,pquery->hist,proto,nenc,false,errStr);
		if(qmap)
			qmap->erase(qit);
		delete pquery;
		pquery=0;
		return true;
	}

	// Initialize the iterators
	pquery->cqResultSetItr=pquery->cqResultSet.begin();
	pquery->aoit=(*pquery->cqResultSetItr).second.begin();

    map<string,ITEMLOCSET>& tmpsetmap = pquery->cqResultSet;
    int setmapsize = tmpsetmap.size();
    for(map<string,ITEMLOCSET>::iterator tmpiter = tmpsetmap.begin(); tmpiter != tmpsetmap.end(); tmpiter++)
    {
        ITEMLOCSET& tmpset = tmpiter->second;
        int setsize = tmpset.size();
        int jj=0;
    }

	if(!ComplexQuerySendResults(pquery,qmap,qit))
	{
		if(qmap)
			qmap->erase(qit);
		delete pquery;
		pquery=0;
        return true;
	}

	// Add Live Queries.  Add a task entry for each Token in the where clause
	if(pquery && pquery->live)
	{
		// If every segment is contains AND operation, register only the first segment list
		bool andsOnly=true;
		for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
		{
			if(((*segItr).oper==OPS_NONE) || ((*segItr).oper==OPS_AND))
				continue;
			andsOnly=false;
			break;
		}
		
		for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
		{
			vector<string> Tokens;
			if(!InList((*segItr).Segment,Tokens))
				continue;

			KEYTYPE keytype=pquery->pmod->ConditionMatch((*segItr).Segment.c_str());
			for(int i=0; i<(int)Tokens.size(); i++)
			{
				ComplexQueryAddLive(pquery,fuser,ConvertKeyType2TaskID(keytype),Tokens[i]);
			}
			if(andsOnly)
				break;
		}
	}
    // Keep pquery alive for continued queries and live queries
	return true;
}

bool ViewServer::ComplexQuerySearch(VSQuery* pquery,KEYTYPE type,string& key,WhereSegment* wSegment)
{
	if(!pquery)
	{
		WSLogError("ComplexQuerySearch(), invalid input parameter");
		return false;
	}

	if(type==KEY_UNKNOWN)
	{
		WSLogEvent("ComplexQuerySearch(), invalid key type");
		return false;
	}

	if(key.size()<1)
	{
		WSLogEvent("ComplexQuerySearch(), invalid search key");
		return false;
	}

	if(type==KEY_APPINSTID)
	{
		AppInstance *ainst=GetAppInstance(key.c_str());
		if(!ainst)
			return true;

		CQRESULTLISTSET::iterator itr=wSegment->cqResultSet.find(ainst->asys->skey);
		if(itr!=wSegment->cqResultSet.end())
			(*itr).second.insert(ainst->oset.begin(),ainst->oset.end());
		return true;
	}
	if(type==KEY_APPSYSTEM)
	{
		APPSYSMAP::iterator itr=asmap.find(key);
		if(itr==asmap.end())
			return true;

		AppSystem* as=(*itr).second;
		for(APPINSTMAP::iterator iit=as->imap.begin();iit!=as->imap.end();iit++)
		{
			AppInstance *ainst=iit->second;
			CQRESULTLISTSET::iterator itr=wSegment->cqResultSet.find(ainst->asys->skey);
			if(itr!=wSegment->cqResultSet.end())
				(*itr).second.insert(ainst->oset.begin(),ainst->oset.end());
		}
		return true;
	}
	for(APPSYSMAP::iterator itr=asmap.begin(); itr!=asmap.end(); itr++)
	{
		AppSystem* rsys=itr->second;
		if(rsys)
		{
			VSINDEX* map=0;
			if(!GetMap(type,rsys->odb,map))
				continue;
			switch(type)
			{
			case KEY_CLORDID:
				{
					VSINDEX::iterator oit=map->find(key);
					if(oit!=map->end())
					{
						CQRESULTLISTSET::iterator cqItr=wSegment->cqResultSet.find(rsys->skey);
						if(cqItr!=wSegment->cqResultSet.end())
							(*cqItr).second.insert(oit.second);
					}
				}
				break;

			case KEY_SYMBOL:
			case KEY_ROOTORDERID:
			case KEY_FIRSTCLORDID:
			case KEY_CLPARENTORDERID:
			case KEY_ACCOUNT:
			case KEY_ECNORDERID:
			case KEY_CLIENTID:
			case KEY_TERM:
			case KEY_OPENORDER:
			case KEY_CONNECTION:
			case KEY_AUXKEY1:
			case KEY_AUXKEY2:
			case KEY_AUXKEY3:
			case KEY_AUXKEY4:
			case KEY_AUXKEY5:
			case KEY_AUXKEY6:
				{
					if(!map)
					{
						WSLogError("ComplexQuerySearch(), bad map");
						return false;
					}
 					CQRESULTLISTSET::iterator itr=wSegment->cqResultSet.find(rsys->skey);
					if(itr==wSegment->cqResultSet.end())
					{
						WSLogEvent("Error, appsys not found");
						return false;
					}
					map->m_copy(key,(*itr).second);
				}
				break;

			// Secondary search keys only
			case KEY_HIGHMSGTYPE:
			case KEY_HIGHEXECTYPE:
			case KEY_ORDERQTY:
			case KEY_CUMQTY:
			case KEY_FILLQTY:
			case KEY_TRANSACTTIME:
			case KEY_ORDERLOC:
				if(wSegment->oper!=OPS_AND)
				{
					WSLogEvent("Secondary search key must be anded only");
					return false;
				}
				break;
			default:
				break;
			}
		}
	}
	return true;
}

bool ViewServer::ComplexQuerySendResults(VSQuery*& pquery,VSQUERYMAP *qmap,VSQUERYMAP::iterator& qit)
{
	if(!pquery || !qmap)
	{
		WSLogEvent("ComplexQuerySendResults(), invalid object");
		return false;
	}

	if(!pquery->complexquery)
	{
		WSLogEvent("ComplexQuerySendResults(), not a complex query");
		return false;
	}

	int proto=PROTO_UNKNOWN;
    if(pquery->binarydatareq)
        proto=PROTO_DAT;
    else
    {
        if(!_stricmp(pquery->from.c_str(),"DETAILS"))
            proto=PROTO_FIX;
        else if(!_stricmp(pquery->from.c_str(),"ORDERS"))
            proto=PROTO_DSV;
    }

	if(proto==PROTO_UNKNOWN)
	{
		WSLogEvent("ComplexQuerySendResults(), unknown protocol");
		return false;
	}

	int tsent=0;
	int ndets=0;
	int nfix=0;
	list<char *> dlist; // list of details that need unloading
	
	// Outer loop is for the appsystems
	// Inner loop is to iterater thru the set in each app system
	while(pquery->cqResultSetItr!=pquery->cqResultSet.end())
	{
		if(tsent>=pquery->maxorders)
			break;

		if(pquery->aoit==(*pquery->cqResultSetItr).second.end())
		{
			pquery->cqResultSetItr++;
            if(pquery->cqResultSetItr==pquery->cqResultSet.end())
                break;
			pquery->aoit=(*pquery->cqResultSetItr).second.begin();
			continue;
		}

		for(;pquery->aoit!=(*pquery->cqResultSetItr).second.end();pquery->aoit++)
		{
			if(tsent>=pquery->maxorders)
				break;

			APPSYSMAP::iterator a_itr=asmap.find((*pquery->cqResultSetItr).first);
			if(a_itr==asmap.end())
				continue;
			AppSystem* rsys=a_itr->second;

			VSOrder* porder=rsys->odb.ReadOrder(*pquery->aoit);
			if(!porder)
				continue;

			if(proto==PROTO_DSV)
			{
				if(!pquery->FilterOrderResult(porder,true,this))
					continue;

				ndets++;
				if(ndets>=256 || ndets>=pquery->maxorders)
				{
					int nenc=ndets;
					if(SendSqlResult(pquery,0,ndets+pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
					{
						tsent+=nenc;
						ndets-=nenc;
						pquery->spos+=nenc;
					}
				}
			}
            else if(proto==PROTO_DAT)
            {
				int ndets_porder=(int)porder->GetNumDetails();
 				for(int d=0;d<ndets_porder;d++)
				{
					unsigned short UscPortNo=(unsigned short)-1;
					unsigned short dlen=0;
					LONGLONG doff=porder->GetDetail(d,UscPortNo,dlen);
					if(((short)dlen<0)||(dlen>4096)) // sanity limit
						continue;
					if(((short)UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
						continue;
					VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
					if(!dfile)
						continue;
					char *dptr=new char[dlen+1];
					memset(dptr,0,dlen+1);
					if(rsys->odb.ReadDetail(dfile,doff,dptr,dlen)<=0)
					{
						WSLogEvent("Error reading detail file");
						delete dptr;
						continue;
					}
                    int filter_status = pquery->FilterClbackupResult(dptr, dlen, true, this);
                    if(filter_status <= 0)
                    {
                        if(filter_status < 0)
						    WSLogEvent("Error filtering detail file");
						delete dptr;
						continue;
                    }
                    ndets++;

                    if(ndets>=16 || ndets>=pquery->maxorders)
                    {
                        int nenc=ndets;
                        if(SendSqlResult(pquery,0,ndets+pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
                        {
                            tsent+=nenc;
                            ndets-=nenc;
                            pquery->spos+=nenc;
                        }
                    }
                }
           }
			else if(proto==PROTO_FIX)
			{
				int ndets=(int)porder->GetNumDetails();

				for(int d=0;d<ndets;d++)
				{
					unsigned short UscPortNo=(unsigned short)-1;
					unsigned short dlen=0;
					LONGLONG doff=porder->GetDetail(d,UscPortNo,dlen);
					if(((short)dlen<0)||(dlen>4096)) // sanity limit
						continue;
					if(((short)UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
						continue;
					VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
					if(!dfile)
						continue;
					char *dptr=new char[dlen+1];
					memset(dptr,0,dlen+1);
					if(rsys->odb.ReadDetail(dfile,doff,dptr,dlen)<=0)
					{
						WSLogEvent("Error reading detail file");
						delete dptr;
						continue;
					}
					FIXINFO ifix;
					ifix.Reset();
					if(dfile->proto==PROTO_MLFIXLOGS)
						ifix.FIXDELIM=';';
					else if(dfile->proto==PROTO_TJMLOG)
						ifix.FIXDELIM=' ';
					else if(dfile->proto==PROTO_TBACKUP)
						ifix.FIXDELIM='|';
					else if((dfile->proto==PROTO_MLPH_EMEA)||(dfile->proto==PROTO_MLPH_CC))
					{
						ifix.FIXDELIM=';';
						ifix.FIXDELIM2=' ';
					}
					else
						ifix.FIXDELIM=0x01;
					ifix.noSession=true;
					ifix.supressChkErr=true;
					ifix.FixMsgReady(dptr,(dfile->proto==PROTO_TJMLOG)?dlen -1:dlen);
					// Filter where FIX conditions
					if(pquery->FilterFixResult(&ifix,true,this,false)>0)
					{
						nfix++;
						dlist.push_back(dptr);
						// Send filled blocks as soon as possible
						if(nfix>=16 || nfix>=pquery->maxorders)
						{
							int nenc=nfix;
							if(SendSqlResult(pquery,0,nfix+pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
							{
								tsent+=nenc;
								nfix-=nenc;
								pquery->spos+=nenc;
							}
							int i=0;
							for(list<char*>::iterator dit=dlist.begin();
								(dit!=dlist.end())&&(i<nenc);
								#ifdef WIN32
								dit=dlist.erase(dit),i++)
								#else
								dlist.erase(dit++),i++)
								#endif
								delete *dit;
						}
					}
				}	
			}	
		}
	}

	/*
	WSLogEvent("ComplexQuerySendResults(), done with loop");//TEMP
	*/

	if(pquery->cqResultSetItr==pquery->cqResultSet.end())
	{
		/*
		WSLogEvent("ComplexQuerySendResults(), end of result set");
		*/
		pquery->morehist=false;
		if(proto==PROTO_FIX)
			ndets=nfix;
		int nenc=ndets;
		if(SendSqlResult(pquery,0,ndets+pquery->spos,pquery->tpos,pquery->hist,proto,nenc,false,0)>0)
		{
			if(nenc!=ndets)
				WSLogEvent("ComplexQuerySendResults(), SendSqlResult not all sent (%d,%d)",nenc,ndets);
			pquery->spos+=ndets;
		}
		else
		{
			WSLogError("ComplexQuerySendResults(), SendSqlResult failed");
		}
		if(!pquery->live)
		{
			/*
			WSLogEvent("ComplexQuerySendResults(), erasing query from qmap");
			*/
			if(qmap)
				qmap->erase(qit);
			delete pquery;
			pquery=0;
		}
	}
	else if(ndets)
	{
		WSLogEvent("ComplexQuerySendResults(), ndets left");
		if(proto==PROTO_FIX)
                        ndets=nfix;
                int nenc=ndets;
		if(SendSqlResult(pquery,0,ndets+pquery->spos,pquery->tpos,pquery->hist,proto,nenc,true,0)>0)
		{
			if(nenc!=ndets)
				WSLogEvent("ComplexQuerySendResults(), SendSqlResult not all sent (%d,%d)",nenc,ndets);
			pquery->spos+=ndets;
		}
		else
		{
			WSLogError("ComplexQuerySendResults(), SendSqlResult failed");
		}
	}

	// unload details
	if(dlist.size())
	{
		for(list<char*>::iterator dit=dlist.begin();dit!=dlist.end();dit++)
		{
			char *dptr=*dit;
			delete dptr;
		}
		dlist.clear();
	}
	return true;
}


bool ViewServer::ComplexQueryContinueQuery(VSQuery* pquery,VSQUERYMAP *qmap,VSQUERYMAP::iterator& qit)
{
	if(!ComplexQuerySendResults(pquery,qmap,qit))
	{
		if(qmap)
			qmap->erase(qit);
		delete pquery;
		pquery=0;
	}
	return true;
}


int ViewServer::BuildWhereSegments(list<WhereSegment>& WhereList, string _where)
{
	// Build a list of where seqments by separating the " AND " and " OR " clauses
	string tempWhere=_where;

    size_t temp;
	while((temp=tempWhere.find("||"))!=string::npos)
		tempWhere.replace(temp,2," OR ");
	while((temp=tempWhere.find("&&"))!=string::npos)
		tempWhere.replace(temp,2," AND ");

	WHERE_OPS prevOper=OPS_NONE;
	while(true)
	{
		size_t orLocation=tempWhere.find(" OR ");
		size_t andLocation=tempWhere.find(" AND ");

		if(orLocation==string::npos && andLocation==string::npos)
		{
			WhereList.push_back(WhereSegment(tempWhere,prevOper,&asmap));
			break;
		}

		size_t end=(andLocation < orLocation) ? andLocation : orLocation;
		if(end!=string::npos)
		{
			WhereList.push_back(WhereSegment(tempWhere.substr(0,end),prevOper,&asmap));
			if(end==andLocation)
			{
				tempWhere.erase(0,end+5);
				prevOper=OPS_AND;
			}
			else
			{
				tempWhere.erase(0,end+4);
				prevOper=OPS_OR;
			}
		}
	}

	// Convert all segments into the IN format
	for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
	{
        (*segItr).Segment.erase(0, (*segItr).Segment.find_first_not_of(" "));
        (*segItr).Segment.erase((*segItr).Segment.find_last_not_of(" ")+1);

		if(strcmp((*segItr).Segment.c_str()," IN ")==0)
			continue;

		size_t sep=(*segItr).Segment.find("==");
		if(sep!=string::npos)
		{
			(*segItr).Segment=((*segItr).Segment.substr(0,sep) + " IN (" + (*segItr).Segment.substr(sep+2) + ")");
		}

		sep=(*segItr).Segment.find("!=");
		if(sep!=string::npos)
		{
			(*segItr).Segment=((*segItr).Segment.substr(0,sep) + " NOT IN (" + (*segItr).Segment.substr(sep+2) + ")");
		}
	}

	return WhereList.size();
}

bool ViewServer::ComplexQueryAddLive(VSQuery* pquery,FeedUser* fuser,uchar qt,string& key)
{
	//WSLogEvent("!!!ComplexQueryAddLive(), %d-%s",pquery->rid,pquery->where.c_str());
	pquery->pmod=this;

	VSQueryLive* vsql=new VSQueryLive(pquery);
	if(!vsql)
	{
		WSLogError("Memory allocation error creating VSSQueryLive!!!");
		return false;
	}

	// Virtual DETAILS table
	if(!_stricmp(pquery->from.c_str(),"DETAILS"))
	{
		//WSLogEvent("ComplexQueryAddLive(), adding DETAILS task, key:%s",key.c_str());
		TASKENTRY* te=taskmgr.AddTask(fuser,key,qt,vsql,pquery->rid,_SendSqlComplexDetailsLive);
		pquery->LiveQueries.insert(pair<TASKENTRY*,VSQueryLive*>(te,vsql));
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	// Virtual ORDERS table
	else if(!_stricmp(pquery->from.c_str(),"ORDERS"))
	{
		//WSLogEvent("ComplexQueryAddLive(), adding ORDERS task, key:%s",key.c_str());
		TASKENTRY* te=taskmgr.AddTask(fuser,key,qt,vsql,pquery->rid,_SendSqlComplexOrdersLive);
		pquery->LiveQueries.insert(pair<TASKENTRY*,VSQueryLive*>(te,vsql));
		// Send query iterator for cancel
		int nenc=0;
		SendSqlResult(pquery,0,0,0,false,0,nenc,false,0);
	}
	else
	{
		return false;
	}
	return true;
}

int _stdcall _SendSqlComplexOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQueryLive* vsql=(VSQueryLive*)udata;
	if(!vsql)
		return 0;

	int rc=vsql->pquery->pmod->SendSqlComplexOrdersLive(fuser,skey,taskid,rid,vsql,hint);
	return rc;
}

int ViewServer::SendSqlComplexOrdersLive(FeedUser *fuser,const string& skey,uchar taskid,DWORD rid,void *udata,void *hint)
{
	ViewServer::FeedHint *fhint=(FeedHint *)hint;
	int PortNo=fuser->PortNo;
	if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS)||(!UsrPort[PortNo].SockActive))
		return 0;

	WSLockPort(WS_USR,PortNo,true); // for 'pquery'
	if(!UsrPort[PortNo].SockActive)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	VSCodec *vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
	if(!vsc)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
	if(!ustats)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	VSQueryLive* vsql=(VSQueryLive*)udata;
	if(vsql->pquery->where.empty())
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}

	if(vsql->FilterOrder(fhint->porder))
	{
		// Filter selected columns
		if(vsql->pquery->FilterOrderResult(fhint->porder,false,NULL)>0)
		{
			int nenc=1;
			SendSqlResult(vsql->pquery,0,++vsql->pquery->spos,0,false,ViewServer::PROTO_DSV,nenc,false,0);
		}
	}
	
	// Must return 1 for further updates
	WSUnlockPort(WS_USR,PortNo,true);
	return 1;
}

int _stdcall _SendSqlComplexDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQueryLive* vsql=(VSQueryLive*)udata;
	if(!vsql)
		return 0;

	int rc=vsql->pquery->pmod->SendSqlComplexDetailsLive(fuser,skey,taskid,rid,vsql,hint);
	return rc;
}


int ViewServer::SendSqlComplexDetailsLive(FeedUser *fuser,const string& skey,uchar taskid,DWORD rid,void *udata,void *hint)
{
	ViewServer::FeedHint *fhint=(FeedHint *)hint;
	int PortNo=fuser->PortNo;
	if((PortNo<0)||(PortNo>=NO_OF_USR_PORTS)||(!UsrPort[PortNo].SockActive))
		return 0;

	WSLockPort(WS_USR,PortNo,true); // for 'pquery'
	if(!UsrPort[PortNo].SockActive)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	VSCodec *vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
	if(!vsc)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
	if(!ustats)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	VSQueryLive* vsql=(VSQueryLive*)udata;
	if(vsql->pquery->where.empty())
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 0;
	}
	if(vsql->FilterOrder(fhint->porder))
	{
		// Load the detail once for all clients
		if(!fhint->pfix)
		{
		       if(LoadFixDetail(fhint)<0)
		       {
				WSUnlockPort(WS_USR,PortNo,true);
				return 1;
		       }
		}
		// Filter selected tags
		if(vsql->pquery->FilterFixResult(fhint->pfix,false,this,false)>0)
		{
			int nenc=1;
			SendSqlResult(vsql->pquery,0,++vsql->pquery->spos,0,false,ViewServer::PROTO_FIX,nenc,false,0);
			ustats->msgCnt+=nenc;
		}
		else
		{
			WSLogEvent("FilterFixResult() failed");
		}
	}
	// Must return 1 for further updates
	WSUnlockPort(WS_USR,PortNo,true);
	return 1;
}

#endif//SPECTRUM
