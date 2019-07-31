#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "vsquery.h"
#include "wstring.h"
#include "md5.h"
//DT9383
#include "vsquerylive.h"


extern int _stdcall _SendSqlDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
extern int _stdcall _SendSqlAccountsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);


//DT9044
void ViewServer::VSCNotifyDatRequest(void *udata, DWORD rid, const char *select, const char *_from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
    char from[64] = {0};
    strcpy(from, _from);
	_ASSERT((select)&&(*select)&&(from)&&(*from)&&(where));
	int PortType=HIWORD((PTRCAST)udata);
	int PortNo=LOWORD((PTRCAST)udata);
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
	else
	{
		_ASSERT(false);
		return;
	}

	if(LOG_QUERIES>0)
	{
        WSLogEvent("%s%d: NotifyDatRequest(rid=%d,select=%s,from=%s,where=%s,maxorders=%d,hist=%d,live=%d,iter=%d)",
			PortTypeStr,PortNo,rid,select,from,where,maxorders,hist,live,(int)iter);
	}

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
        pquery->pmod=this;
		pquery->hist=hist;
		pquery->live=live;
		pquery->unique=false;
		pquery->binarydatareq=true;  //DT9044
		pquery->tzbias=QueryTimeZoneBias;
        if(strcmp(from, "DETAILS") == 0)
            pquery->binarydatatype = QDT_BINARY_BTR; // TODO: This is only true for CLSERVER app system
        else if(strcmp(from, "IQOS") == 0)
        {
            pquery->binarydatatype = QDT_BINARY_IQOS;
            strcpy(from, "DETAILS");
            pquery->from = from;
        }
	#ifdef IQSMP
		else
			pquery->binarydatatype = QDT_BINARY_OTHER;
	#endif
		pquery->tstart=GetTickCount();
		pquery->eiter=++qmap->eiterNext;
		//DT9398
		for(unsigned int i=0;i<odb.INDEX_AUXKEYS;i++)
			pquery->qscope.AuxKeyNames.push_back(odb.AUXKEY_NAMES[i]);
		(*qmap)[pquery->eiter]=pquery;
		qit=qmap->find(pquery->eiter);

#ifdef SPECTRUM
		if(ComplexQuery(pquery,qmap,qit,fuser)) //DT10472
			return;
#endif

		if(!_stricmp(from,"ACCOUNTS"))
		{
			if(StartAccountsQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
		else if(!_stricmp(from,"DETAILS"))
		{
			if(StartQuery(pquery)<0)
			{
				qmap->erase(qit);
				delete pquery;
				return;
			}
		}
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
			// Virtual ACCOUNTS table
			if(!_stricmp(from,"ACCOUNTS"))
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
			else if(!_stricmp(from,"DETAILS"))
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
			else
			{
				_ASSERT(false);
			}
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
		// There is no local AppSystem when forwarded
		if(pquery->FwdPortType>=0)
		{
			ContinueForwardQuery(pquery);
			return;
		}
		
		// Virtual ACCOUNTS table
		if(!_stricmp(pquery->from.c_str(),"ACCOUNTS"))
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
		else if(!_stricmp(pquery->from.c_str(),"DETAILS"))
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

void ViewServer::VSCNotifyDatReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	// Find the forwarded query
	int PortNo=LOWORD((PTRCAST)udata);
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
		WSLogEvent("CON%d: Forward query not found for VSCNotifyDatReply(rid=%d)",PortNo,rid);
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
		WSLogEvent("USR%d: Query(iter=%d) not found for VSCNotifyDatReply(rid=%d)",UsrPortNo,fquery->uiter,rid);
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
		if(pquery->ProxyDatResult(hist,proto,pstrlen,pstr,nstr,more,iter,reason)<0)
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

//DT9044
int VSQuery::ProxyDatResult(bool hist, int proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	int acnt=0;
	for(int i=0;i<nstr;i++)
	{
		VSQueryResult *qresult=new VSQueryResult;
		qresult->rtime=GetTickCount();
		qresult->obuf=new char[pstrlen];
		memcpy(qresult->obuf,pstr[i],pstrlen);
		if(hist)
			hresults.push_back(qresult); 
		else
			lresults.push_back(qresult); 
		rcnt++; acnt++;
	}
	return acnt;
}

