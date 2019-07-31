
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "vsquery.h"
#include "wstring.h"
#include "md5.h"
#include "IQClServer.h"  //DT9044
#include "vsquerylive.h"

#ifdef SPECTRUM
#include <vector>
using namespace std;
#endif

const char *VSQueryScope::GetEqStrVal(ExprTok *cond)
{
	if((!strcmp(cond->oval,"=="))&&(cond->nparams==2))
	{
		if(cond->params[0]->vtype=='S')
			return cond->params[0]->sval;
		else if(cond->params[1]->vtype=='S')
			return cond->params[1]->sval;
	}
	return 0;
}
#ifdef MULTI_DAY
LONGLONG VSQueryScope::GetEqIntVal(ExprTok *cond)
{
	if((!strcmp(cond->oval,"=="))&&(cond->nparams==2))
	{
		if(cond->params[0]->vtype=='I')
			return cond->params[0]->ival;
		else if(cond->params[1]->vtype=='I')
			return cond->params[1]->ival;
	}
	return 0;
}
#endif

// VSCNotify
void ViewServer::VSCNotifyError(int rc, const char *emsg)
{
	_ASSERT(emsg);
	if(LOG_QUERIES>0)
	WSLogError(emsg);
}
void ViewServer::VSCNotifyEvent(int rc, const char *emsg)
{
	_ASSERT(emsg);
	if(LOG_QUERIES>0)
	WSLogEvent(emsg);
}
USERENTITLELIST *ViewServer::MakeEntitlementList(char *estr)
{
	USERENTITLELIST *elist=new USERENTITLELIST;
	if(!strcmp(estr,"*"))
		elist->push_back(USERENTITLEMENT("*","*"));
	else
	{
		for(char *tok=strtok(estr,"|");tok;tok=strtok(0,"|"))
		{
			char *sname=tok;
			char *iname=strchr(tok,'/');
			if(iname)
			{
				*iname=0; iname++;
			}
			else
				iname="*";
			USERENTITLEMENT ent;
			ent.first=sname;
			ent.second=iname;
			elist->push_back(ent);
		}
	}
	return elist;
}
bool ViewServer::IsEntitled(FeedUser *fuser, const char *sname, const char *iname)
{
	_ASSERT((fuser)&&(sname)&&(*sname));
	if(!fuser->authed)
		return false;
	USERENTITLELIST *elist=(USERENTITLELIST *)fuser->DetPtr3;
	if(!elist)
		return false;
	for(USERENTITLELIST::iterator eit=elist->begin();eit!=elist->end();eit++)
	{
		USERENTITLEMENT& ent=*eit;
		// entitled all systems
		if(ent.first=="*")
			return true;
		if(!_stricmp(ent.first.c_str(),sname))
		{
			// optional check
			if((!iname)||(!*iname))
				return true;
			// entitled all instances
			else if(ent.second=="*")
				return true;
			else if(!_stricmp(ent.second.c_str(),iname))
				return true;
		}
	}
	return false;
}
// For EncodeLoginRequest
void ViewServer::VSCNotifyLoginRequest(void *udata, DWORD rid, const char *user, const unsigned char *phash, int phlen)
{
	_ASSERT((user)&&(*user));
	int PortNo=LOWORD((PTRCAST)udata);
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
			if(!uname || !upass || !maxorders || !maxbrowse || !entitle)
			{
				WSLogError("Invalid entry in users.ini. Format: \"uname,pw,maxorders,maxbrowse,entitlemts\"");
				continue;
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
					fuser->DetPtr1=(void*)(PTRCAST)maxorders;
					fuser->DetPtr2=(void*)(PTRCAST)maxbrowse;
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
	if(vsc->EncodeLoginReply(rptr,sizeof(rbuf),rc,rid,rc<0?reason:"")>0)
	{
		#ifdef _DEBUG
		WSLogEvent("USR%d: EncodeLoginReply(rc=%d,rid=%d,reason=%s) %d bytes,maxorders=%d",
			PortNo,rc,rid,rc<0?reason:"",rptr -rbuf,maxorders);
		#endif
		WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
		ustats->msgCnt++;
	}
	else
		WSLogError("USR%d: EncodeLoginReply failed.",PortNo);
}
void ViewServer::VSCNotifyLoginReply(void *udata, int rc, DWORD rid, const char *reason)
{
	_ASSERT(false);
}
int ViewServer::LoadFixDetail(FeedHint *fhint)
{
	// Load the detail once for all clients
	if(fhint->pfix)
		return 0;
	unsigned short UscPortNo=(unsigned short)-1;
	unsigned short dlen=0;
	LONGLONG doff=fhint->porder->GetDetail(fhint->didx,UscPortNo,dlen);
	if(((short)dlen<0)||(dlen>4096)) // sanity limit
		return -1;
	if(((short)UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
		return -1;
	VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
	if(!dfile)
	{
		// This could fill up the log file otherwise
		WSLogErrorOnce("USC%d: Missing detail file!",UscPortNo);
		return -1;
	}
	fhint->proto=dfile->proto;
	char *dptr=new char[dlen+1];
	memset(dptr,0,dlen+1);
	OrderDB *pdb=&fhint->asys->odb;
	if(pdb->ReadDetail(dfile,doff,dptr,dlen)<=0)
		return -1;
	fhint->pfix=new FIXINFO;
	fhint->pfix->Reset();
	fhint->pfix->sbuf=dptr;
	fhint->pfix->slen=dlen;
	if(dfile->proto==PROTO_MLFIXLOGS)
		fhint->pfix->FIXDELIM=';';
	else if(dfile->proto==PROTO_TJMLOG)
		fhint->pfix->FIXDELIM=' ';
	else if((dfile->proto==PROTO_TBACKUP)||(dfile->proto==PROTO_FULLLOG))
		fhint->pfix->FIXDELIM='|';
	else if((dfile->proto==PROTO_MLPH_EMEA)||(dfile->proto==PROTO_MLPH_CC))
	{
		fhint->pfix->FIXDELIM=';';
		fhint->pfix->FIXDELIM2=' ';
	}
	else
		fhint->pfix->FIXDELIM=0x01;
	fhint->pfix->noSession=true;
	fhint->pfix->supressChkErr=true;
	if(fhint->pfix->FixMsgReady(dptr,(dfile->proto==PROTO_TJMLOG)?dlen -1:dlen)<1)
		return -1;
	return 0;
}

// Evaluate where clause order conditions against a real order
int VSQuery::FilterOrder(VSOrder *porder, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={porder,0,qscope.AuxKeyNames,0}; //DT9398
	qscope.etok.EvalExprTree(0,&ehint,CaseSen);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause FIX conditions against the given FIX msg,
// and adds result FIX string to 'hresult' or 'lresult'.
int VSQuery::FilterFixResult(FIXINFO *pfix, bool hist, ExprTokNotify *pnotify, bool checkWhere)
{
	if(!pfix->llen)
	{
		_ASSERT(false);
		return 0;
	}
	// Evaluate where clause FIX conditions
	if(!where.empty() && checkWhere)
	{
		const char *wstr=where.c_str();
		qscope.etok.pnotify=pnotify;
		EvalHint ehint={0,pfix,qscope.AuxKeyNames,0}; //DT9398
		qscope.etok.EvalExprTree(pfix,&ehint,CaseSen);
		switch(qscope.etok.vtype)
		{
		case 'B':
			if(!qscope.etok.bval)
				return 0;
			break;
		case 'I':
			if(!qscope.etok.ival) 
				return 0;
			break;
		case 'F':
			if(qscope.etok.fval==0.0) 
				return 0;
			break;
		case 'S':
			if(!qscope.etok.sval[0]) 
				return 0;
			break;
		case 'V':
			break;
		default:
			return 0;
		};
	}
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->rfix.slen=pfix->llen;
	qresult->rfix.sbuf=new char[pfix->llen];
	memset(qresult->rfix.sbuf,0,pfix->llen);
	char *rptr=qresult->rfix.sbuf;
	const char *rend=rptr +pfix->llen,*fend=pfix->fbuf +pfix->llen;
	// Send entire FIX string
	if(select=="*")
	{
		memcpy(rptr,pfix->fbuf,pfix->llen); rptr+=pfix->llen;
	}
	// Filter only desired items
	else
	{
		// Copy only the tags in the select clause
		char sstr[1024]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			int tno=myatoi(tok);
			if(!tno)
				continue;
			FIXTAG *ptag=pfix->GetTag(tno);
			if(ptag)
			{
				const char *btag=pfix->fbuf +ptag->tstart;
				const char *etag=ptag->val +ptag->vlen;
				if(rend -rptr<(int)(etag -btag) +1)
				{
					_ASSERT(false);
					break;
				}
				memcpy(rptr,btag,etag -btag); rptr+=(etag -btag);
				*rptr=pfix->FIXDELIM; rptr++;
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}	
	qresult->rfix.Reset();
	qresult->rfix.FIXDELIM=pfix->FIXDELIM;
	qresult->rfix.noSession=pfix->noSession;
	qresult->rfix.supressChkErr=true;
	// We're not really using the FIX msg for query so no need to do anhy parsing
	//qresult->rfix.FixMsgReady(qresult->rfix.sbuf,(int)(rptr -qresult->rfix.sbuf));
	qresult->rfix.fbuf=qresult->rfix.sbuf;
	qresult->rfix.llen=(int)(rptr -qresult->rfix.sbuf);
	if(pfix->fci)
	{
		qresult->rfix.fci=new FIXCONINFO;
		memcpy(qresult->rfix.fci,pfix->fci,sizeof(FIXCONINFO));
	}
	if(hist) 
		hresults.push_back(qresult);
	else 
		lresults.push_back(qresult);
	rcnt++;
	return 1;
}
// Checks where clause order conditions against the given order,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterOrderResult(VSOrder *porder, bool hist, ExprTokNotify *pnotify)
{
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[VSORDER_RECSIZE +15];
	memset(qresult->obuf,0,VSORDER_RECSIZE +15);
	char *rptr=qresult->obuf;
	const char *rend=rptr +VSORDER_RECSIZE +15;
	// Send all order attributes
	if(select=="*")
	{
		sprintf(rptr,"%s\1%s\1%s\1%s\1%s\1%s\1%s\1%s\1",
			porder->GetAppInstID(),
			porder->GetClOrdID(),
			porder->GetRootOrderID(),
			porder->GetFirstClOrdID(),
			porder->GetSymbol(),
			porder->GetAccount(),
			porder->GetEcnOrderID(),
			porder->GetClientID());
		rptr+=strlen(rptr);
		char hmt[2]={porder->GetHighMsgType(),0};
		char het[2]={porder->GetHighExecType(),0};
		char ss[2]={porder->GetSide(),0};
		#ifdef WIN32
		sprintf(rptr,"%f\1%s\1%d\1%d\1%d\1%s\1%s\1%s\1%I64d\1%I64d\1%s\1%s\1%08d\1%s\1",
		#else
		sprintf(rptr,"%f\1%s\1%d\1%d\1%d\1%s\1%s\1%s\1%lld\1%lld\1%s\1%s\1%08d\1%s\1",
		#endif
			porder->GetPrice(),
			ss,
			porder->GetOrderQty(),
			porder->GetCumQty(),
			porder->GetFillQty(),
			porder->IsTerminated()?"TRUE":"FALSE",
			hmt,het,porder->GetTransactTime(),(LONGLONG)porder->GetOffset(),
			porder->GetConnection(),
			porder->GetClParentOrderID(),
			//porder->GetLocaleOrderDate(tzbias),
			porder->GetOrderDate(),
			porder->GetAuxKey());
		rptr+=strlen(rptr);
	}
	// Filter only desired attributes
	else
	{
		// Copy only the attributes in the select clause
		char sstr[1024]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtoke(sstr,",\r\n");tok;tok=strtoke(0,",\r\n"))
		{
			if(!_stricmp(tok,"AppInstID"))
			{
				sprintf(rptr,"%s\1",porder->GetAppInstID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"ClOrdID"))
			{
				sprintf(rptr,"%s\1",porder->GetClOrdID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"RootOrderID"))
			{
				sprintf(rptr,"%s\1",porder->GetRootOrderID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"FirstClOrdID"))
			{
				sprintf(rptr,"%s\1",porder->GetFirstClOrdID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"Symbol"))
			{
				sprintf(rptr,"%s\1",porder->GetSymbol()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"Price"))
			{
				char PriceStr[16]={0};
				sprintf(PriceStr,"%f",porder->GetPrice());
				char *decptr=strchr(PriceStr,'.');
				if(decptr)
				{
					for(char *pptr=decptr +strlen(decptr) -1;(pptr>decptr+2)&&(*pptr=='0');pptr--)
						*pptr=0;
				}
				sprintf(rptr,"%s\1",PriceStr); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"Side"))
			{
				char ss[2]={porder->GetSide(),0};
				sprintf(rptr,"%s\1",ss); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"Account"))
			{
				sprintf(rptr,"%s\1",porder->GetAccount()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"EcnOrderID"))
			{
				sprintf(rptr,"%s\1",porder->GetEcnOrderID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"ClientID"))
			{
				sprintf(rptr,"%s\1",porder->GetClientID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"OrderQty"))
			{
				sprintf(rptr,"%d\1",porder->GetOrderQty()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"CumQty"))
			{
				sprintf(rptr,"%d\1",porder->GetCumQty()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"FillQty"))
			{
				sprintf(rptr,"%d\1",porder->GetFillQty()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"Term"))
			{
				sprintf(rptr,"%s\1",porder->IsTerminated()?"TRUE":"FALSE"); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"HighMsgType"))
			{
				char hmt[2]={porder->GetHighMsgType(),0};
				sprintf(rptr,"%s\1",hmt); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"HighExecType"))
			{
				char het[2]={porder->GetHighExecType(),0};
				sprintf(rptr,"%s\1",het); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"TransactTime"))
			{
				#ifdef WIN32
				sprintf(rptr,"%I64d\1",porder->GetTransactTime()); rptr+=strlen(rptr);
				#else
				sprintf(rptr,"%lld\1",porder->GetTransactTime()); rptr+=strlen(rptr);
				#endif
			}
			else if(!_stricmp(tok,"Connection"))
			{
				sprintf(rptr,"%s\1",porder->GetConnection()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"ClParentOrderID"))
			{
				sprintf(rptr,"%s\1",porder->GetClParentOrderID()); rptr+=strlen(rptr);
			}
			else if(!_stricmp(tok,"OrderLoc"))
			{
				sprintf(rptr,"%X\1",(LONGLONG)porder->GetOffset()); rptr+=strlen(rptr);
			}
		#ifdef MULTI_DAY
			else if(!_stricmp(tok,"OrderDate"))
			{
				//sprintf(rptr,"%08d\1",porder->GetLocaleOrderDate(tzbias)); rptr+=strlen(rptr);
				sprintf(rptr,"%08d\1",porder->GetOrderDate()); rptr+=strlen(rptr);
			}
		#endif
			else
			{
				//DT9398
				unsigned int idx=0;
				for(list<string>::iterator itr=qscope.AuxKeyNames.begin();itr!=qscope.AuxKeyNames.end();itr++)
				{
					if(!_stricmp(tok,itr->c_str()))
					{
						sprintf(rptr,"%s\1",porder->GetAuxKey(idx)); rptr+=strlen(rptr);
					}
					idx++;
				}
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}
// Evaluate where clause order conditions against a real account
int VSQuery::FilterAccount(Account *pacc, ExprTokNotify *pnotify)
{
	if(where.empty())
		return 1;
	qscope.etok.pnotify=pnotify;
	EvalHint ehint={0,0,qscope.AuxKeyNames,pacc}; //DT9398
	qscope.etok.EvalExprTree(0,&ehint,CaseSen);
	switch(qscope.etok.vtype)
	{
	case 'B':
		if(!qscope.etok.bval)
			goto nomatch;
		break;
	case 'I':
		if(!qscope.etok.ival) 
			goto nomatch;
		break;
	case 'F':
		if(qscope.etok.fval==0.0) 
			goto nomatch;
		break;
	case 'S':
		if(!qscope.etok.sval[0]) 
			goto nomatch;
		break;
	case 'V':
		break;
	default:
		goto nomatch;
	};
	return 1;
nomatch:
	return 0;
}
// Checks where clause account conditions against the given account,
// and adds result csv string to 'hresult' or 'lresult'.
int VSQuery::FilterAccountResult(Account *acc, bool hist, ExprTokNotify *pnotify)
{
	//DT9044: sanity check
	if(!acc)
		return 0;

	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->obuf=new char[sizeof(ACCOUNTREC)];  //DT9044
	memset(qresult->obuf,0,sizeof(ACCOUNTREC));  //DT9044
	char *rptr=qresult->obuf;
	const char *rend=rptr +sizeof(ACCOUNTREC);   //DT9044

	// DT9044: binary data request - Ignoring SELECT criteria
	if(binarydatareq)  
	{
		if(acc->iqptr)
		{
			memcpy(rptr,acc->iqptr,sizeof(ACCOUNTREC));
			rptr+=sizeof(ACCOUNTREC);
			binarydatalen=sizeof(ACCOUNTREC);
			qresult->DataLength=binarydatalen;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		// Send all account attributes
		if(select=="*")
		{
			#ifdef WIN32
			sprintf(rptr,"%s\1%s\1%I64d\1%I64d\1%I64d\1%06d\1%d\1",
			#else
			sprintf(rptr,"%s\1%s\1%lld\1%lld\1%lld\1%06d\1%d\1",
			#endif
				acc->AcctName.c_str(),
				acc->AcctType.c_str(),
				acc->AcctOrderQty,
				acc->AcctCumQty,
				acc->AcctFillQty,
				acc->AcctUpdateTime,
				acc->AcctMsgCnt);
			rptr+=strlen(rptr);
		}
		// Filter only desired attributes
		else
		{
			// Copy only the attributes in the select clause
			char sstr[1024]={0};
			strcpy(sstr,select.c_str());
			for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
			{
				if(!_stricmp(tok,"AcctName"))
				{
					sprintf(rptr,"%s\1",acc->AcctName.c_str()); rptr+=strlen(rptr);
				}
				else if(!_stricmp(tok,"AcctType"))
				{
					sprintf(rptr,"%s\1",acc->AcctType.c_str()); rptr+=strlen(rptr);
				}
				else if(!_stricmp(tok,"AcctOrderQty"))
				{
					#ifdef WIN32
					sprintf(rptr,"%I64d\1",acc->AcctOrderQty); rptr+=strlen(rptr);
					#else
					sprintf(rptr,"%lld\1",acc->AcctOrderQty); rptr+=strlen(rptr);
					#endif
				}
				else if(!_stricmp(tok,"AcctCumQty"))
				{
					#ifdef WIN32
					sprintf(rptr,"%I64d\1",acc->AcctCumQty); rptr+=strlen(rptr);
					#else
					sprintf(rptr,"%lld\1",acc->AcctCumQty); rptr+=strlen(rptr);
					#endif
				}
				else if(!_stricmp(tok,"AcctFillQty"))
				{
					#ifdef WIN32
					sprintf(rptr,"%I64d\1",acc->AcctFillQty); rptr+=strlen(rptr);
					#else
					sprintf(rptr,"%lld\1",acc->AcctFillQty); rptr+=strlen(rptr);
					#endif
				}
				else if(!_stricmp(tok,"AcctUpdateTime"))
				{
					sprintf(rptr,"%06d\1",acc->AcctUpdateTime); rptr+=strlen(rptr);
				}
				else if(!_stricmp(tok,"AcctMsgCnt"))
				{
					sprintf(rptr,"%d\1",acc->AcctMsgCnt); rptr+=strlen(rptr);
				}
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}
	if(hist)
		hresults.push_back(qresult); 
	else
		lresults.push_back(qresult); 
	rcnt++;
	return 1;
}
// Pick out key tags in the where clause that will allow us to reduce the number of orders/details
// that we need to search, as opposed to full order or detail population
void ViewServer::ExprTokError(int rc, const char *emsg)
{
#ifdef _DEBUG
	if(LOG_QUERIES>0)
	WSLogEvent(emsg);
#endif
}
bool ViewServer::GetValue(const char *var, class ExprTok *vtok, void *hint)
{
	EvalHint *ehint=(EvalHint *)hint;
	if(ehint->porder)
	{
		VSOrder *porder=ehint->porder;
		if(!strcmp(var,"AppInstID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetAppInstID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"ClOrdID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetClOrdID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"RootOrderID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetRootOrderID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"FirstClOrdID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetFirstClOrdID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"ClParentOrderID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetClParentOrderID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"Symbol"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetSymbol(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"Price"))
		{
			vtok->vtype='F';
			vtok->fval=porder->GetPrice();
			return true;
		}
		else if(!strcmp(var,"Side"))
		{
			vtok->vtype='S';
			vtok->sval[0]=porder->GetSide(); vtok->sval[1]=0;
			return true;
		}
		else if(!strcmp(var,"Account"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetAccount(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"EcnOrderID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetEcnOrderID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"ClientID"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetClientID(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"HighMsgType"))
		{
			vtok->vtype='S';
			vtok->sval[0]=porder->GetHighMsgType(); vtok->sval[1]=0;
			return true;
		}
		else if(!strcmp(var,"HighExecType"))
		{
			vtok->vtype='S';
			vtok->sval[0]=porder->GetHighExecType(); vtok->sval[1]=0;
			return true;
		}
		else if(!strcmp(var,"OrderQty"))
		{
			vtok->vtype='I';
			vtok->ival=porder->GetOrderQty();
			return true;
		}
		else if(!strcmp(var,"CumQty"))
		{
			vtok->vtype='I';
			vtok->ival=porder->GetCumQty();
			return true;
		}
		else if(!strcmp(var,"FillQty"))
		{
			vtok->vtype='I';
			vtok->ival=porder->GetFillQty();
			return true;
		}
		else if(!strcmp(var,"OpenOrder"))
		{
			vtok->vtype='B';
			vtok->bval=porder->IsTerminated()?false:true;
			return true;
		}
		else if(!strcmp(var,"Term"))
		{
			vtok->vtype='B';
			vtok->bval=porder->IsTerminated()?true:false;
			return true;
		}
		else if(!strcmp(var,"OrderLoc"))
		{
			vtok->vtype='S';
			sprintf(vtok->sval,"%X",(LONGLONG)porder->GetOffset());
			return true;
		}
		else if(!strcmp(var,"TransactTime"))
		{
			vtok->vtype='T';
			vtok->ival=porder->GetTransactTime();
			return true;
		}
		else if(!strcmp(var,"Connection"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,porder->GetConnection(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
	#ifdef MULTI_DAY
		else if(!strcmp(var,"OrderDate"))
		{
			vtok->vtype='I';
			//vtok->ival=porder->GetLocaleOrderDate(QueryTimeZoneBias);
			vtok->ival=porder->GetOrderDate();
			return true;
		}
	#endif
		//DT9398
		else
		{
			int AuxKeyIndex=0;
			for(list<string>::iterator itr=ehint->AuxKeyNames.begin(); itr!=ehint->AuxKeyNames.end();itr++,AuxKeyIndex++)
			{
				if(!strcmp(var,itr->c_str()))
				{
					vtok->vtype='S';
					strncpy(vtok->sval,porder->GetAuxKey(AuxKeyIndex),sizeof(vtok->sval) -1); //TODO
					vtok->sval[sizeof(vtok->sval) -1]=0;
					return true;
				}
			}
		}
	}
	else if(ehint->pfix)
	{
		FIXINFO *pfix=ehint->pfix;
		// FIX values
		FIXTAG *ptag=0;
		int tno=myatoi(var);
		// Numeric tag
		if(tno>0)
			ptag=pfix->GetTag(tno);
		// String tag
		else
			ptag=pfix->GetTag(var);
		if(!ptag)
			return false;
		// TODO: int values?
		vtok->vtype='S';
		int vlen=ptag->vlen;
		if(vlen>sizeof(vtok->sval) -1)
			vlen=sizeof(vtok->sval) -1;
		strncpy(vtok->sval,ptag->val,vlen);
		vtok->sval[vlen]=0;
		return true;
	}
	else if(ehint->pacc)
	{
		Account *pacc=ehint->pacc;
		if(!strcmp(var,"AcctName"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,pacc->AcctName.c_str(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
		else if(!strcmp(var,"AcctType"))
		{
			vtok->vtype='S';
			strncpy(vtok->sval,pacc->AcctType.c_str(),sizeof(vtok->sval) -1);
			vtok->sval[sizeof(vtok->sval) -1]=0;
			return true;
		}
	}
#ifdef IQDNS
	else if(ehint->puser)
		return GetUserValue(var,vtok,hint);
	else if(ehint->vrec)
		return GetVarsidValue(var,vtok,hint);
	else if(ehint->pdom)
		return GetDomrelValue(var,vtok,hint);
	else if(ehint->pdomt)
		return GetDomtreeValue(var,vtok,hint);
	else if(ehint->psrv)
		return GetServerValue(var,vtok,hint);
#endif
	return false;
}
// ExprTokNotify callback to determine if an AppInstID is in an AppSystem set.
bool ViewServer::IsInSet(const char *set, const char *val, void *hint)
{
	EvalHint *ehint=(EvalHint *)hint;
	if(ehint->porder)
	{
		AppSystem *asys=GetAppSystem(set);
		if(asys)
		{
			APPINSTMAP::iterator iit=asys->imap.find(val);
			if(iit!=asys->imap.end())
				return true;
		}
	}
	return false;
}
int VSQuery::NarrowScope(VSQueryScope *qscope, const char *where, ExprTokNotify *pnotify, char reason[256])
{
	FIXINFO qfix; // TODO: Remove after we get this working
	memset(&qfix,0,sizeof(FIXINFO));
	// EvalDeps pre-parses and builds the expression tree under 'qscope->etok'
	qscope->etok.pnotify=pnotify;
	qscope->etok.EvalDeps(&qfix,where,where +strlen(where));
	// RecurseScope walks the expression tree to determine which index to walk and the fastest way to walk it
	RecurseScope(qscope,&qscope->etok,pnotify,reason);
	return 0;
}
int VSQuery::RecurseScope(VSQueryScope *qscope, ExprTok *etok, ExprTokNotify *pnotify, char reason[256])
{
	// Depth-first search for boolean operators on variables
	for(int i=0;i<etok->nparams;i++)
		RecurseScope(qscope,etok->params[i],pnotify,reason);
	switch(etok->vtype)
	{
	case 'I':
	case 'F':
	case 'S':
	case 'T':
	case 'V':
		break;
	case 'B':
	{
		// Look for boolean operators
		if((etok->oval[0])&&(etok->nparams==2))
		{
			const char *vname=0;
			ExprTok *vtok=0;
			if(etok->params[0]->vtype=='V')
			{
				vname=etok->params[0]->vname; vtok=etok->params[1];
			}
			else if(etok->params[1]->vtype=='V')
			{
				vname=etok->params[1]->vname; vtok=etok->params[0];
			}
			if(!vname)
				break;
			if(!_stricmp(vname,"Region"))
				qscope->CondRegion.push_back(etok);
			else if(!_stricmp(vname,"AppInstID"))
			{
				if((vtok->vtype=='S')&&(strrcmp(vtok->sval,"*")))
					qscope->CondAppSystem.push_back(etok);
				else
					qscope->CondAppInstID.push_back(etok);
			}
			else if(!_stricmp(vname,"ClOrdID"))
				qscope->CondClOrdID.push_back(etok);
			else if(!_stricmp(vname,"RootOrderID"))
				qscope->CondRootOrderID.push_back(etok);
			else if(!_stricmp(vname,"FirstClOrdID"))
				qscope->CondFirstClOrdID.push_back(etok);
			else if(!_stricmp(vname,"ClParentOrderID"))
				qscope->CondClParentOrderID.push_back(etok);
			else if(!_stricmp(vname,"Symbol"))
				qscope->CondSymbol.push_back(etok);
			//else if(!_stricmp(vname,"Price"))
			//	qscope->CondPrice.push_back(etok);
			//else if(!_stricmp(vname,"Side"))
			//	qscope->CondSide.push_back(etok);
			else if(!_stricmp(vname,"Account"))
				qscope->CondAccount.push_back(etok);
			else if(!_stricmp(vname,"EcnOrderID"))
				qscope->CondEcnOrderID.push_back(etok);
			else if(!_stricmp(vname,"ClientID"))
				qscope->CondClientID.push_back(etok);
			else if(!_stricmp(vname,"HighMsgType"))
				qscope->CondHighMsgType.push_back(etok);
			else if(!_stricmp(vname,"HighExecType"))
				qscope->CondHighExecType.push_back(etok);
			else if(!_stricmp(vname,"OrderQty"))
				qscope->CondOrderQty.push_back(etok);
			else if(!_stricmp(vname,"CumQty"))
				qscope->CondCumQty.push_back(etok);
			else if(!_stricmp(vname,"FillQty"))
				qscope->CondFillQty.push_back(etok);
			else if(!_stricmp(vname,"OpenOrder"))
				qscope->CondOpenOrder.push_back(etok);
			else if(!_stricmp(vname,"Term"))
			{
				if((etok->nparams==2)&&(etok->params[1]->bval))
					qscope->CondTermOrder.push_back(etok);
				else
					qscope->CondOpenOrder.push_back(etok);
			}
			else if(!_stricmp(vname,"TransactTime"))
				qscope->CondTransactTime.push_back(etok);
			else if(!_stricmp(vname,"Connection"))
				qscope->CondConnection.push_back(etok);
		#ifdef MULTI_DAY
			else if(!_stricmp(vname,"OrderDate"))
				qscope->CondOrderDate.push_back(etok);
		#endif
			//DT9398
			else if(qscope->ValueInAuxKeys(vname)!=-1)
				qscope->CondAuxKeys[qscope->ValueInAuxKeys(vname)].push_back(etok);
			else if(!_stricmp(vname,"AcctName"))
				qscope->CondAcctName.push_back(etok);
			else if(!_stricmp(vname,"AcctType"))
				qscope->CondAcctType.push_back(etok);
		#ifdef IQDNS
			else if(!_stricmp(vname,"UserDomain"))
				qscope->CondUserDomain.push_back(etok);
			else if(!_stricmp(vname,"UserName"))
				qscope->CondUserName.push_back(etok);
			else if(!_stricmp(vname,"VarsidKey"))
				qscope->CondVarsidKey.push_back(etok);			
			else if(!_stricmp(vname,"DomainRoot"))
				qscope->CondDomainRoot.push_back(etok);
			//else if(!_stricmp(vname,"Host"))
			//	qscope->CondHost.push_back(etok);
		#endif
			else if(!_stricmp(vname,"OrderLoc"))
			{
				if(!strcmp(etok->oval,"=="))
				{
					//sscanf(vstr+2,"%X",&qscope->EqOrderLoc); // hex offset
					qscope->EqOrderLoc=0;
					char *vptr=vtok->sval;
					for(;(*vptr)&&(isspace(*vptr));vptr++)
						;
					for(;*vptr;vptr++)
					{
						char ch=toupper(*vptr);
						if(isspace(ch))
							break;
						else if(isdigit(ch)||((ch>='A')&&(ch<='F')))
						{
							char dval=0;
							switch(ch)
							{
							case 'F': dval=15; break;
							case 'E': dval=14; break;
							case 'D': dval=13; break;
							case 'C': dval=12; break;
							case 'B': dval=11; break;
							case 'A': dval=10; break;
							default: dval=(ch -'0');
							};
							qscope->EqOrderLoc=qscope->EqOrderLoc*16 +dval;
						}
						else
							break;
					}
					if(vptr==vtok->sval)
					{
						qscope->EqOrderLoc=-1;
						sprintf(reason,"Invalid hex offset(%s) for 'OrderLoc'",vtok->sval);
						pnotify->ExprTokError(-1,reason);
						return -1;
					}
				}
				else
				{
					sprintf(reason,"Unsupported operator(%s) on 'OrderLoc'",etok->oval);
					pnotify->ExprTokError(-1,reason);
					return -1;
				}
			}
			else
			{
				sprintf(reason,"Invalid column(%s) on 'ORDERS'",vname);
				pnotify->ExprTokError(-1,reason);
				return -1;
			}
		}
		break;
	}
	};
	return 0;
}

// 'skey'=ClOrdID, 'taskid'=TASK_SQL
int _stdcall _SendSqlDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	if(!pquery)
		return -1;
	int rc=pquery->pmod->SendSqlDetailsLive(fuser,skey,taskid,rid,pquery,hint);
	return rc;
}
int ViewServer::SendSqlDetailsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	// Return 0 to remove the task on conditions where the user has disconnected
	VSQuery *pquery=(VSQuery *)udata;
	if(!pquery)
		return 0;
	FeedHint *fhint=(FeedHint *)hint;
	if(!fuser)
		return 0;
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
	// DEV-6516: Past this point, return 1 on error, because the user is still connected and we don't want
	// the task to be mysteriously yanked.

	// This is a status update on an order, but no new detail was added to the order
	// (i.e. a previous order status updated by a cancel execution with different ClOrdID)
	if(fhint->didx<0)
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 1;
	}
	// Disallow live queries with no conditions
	if(pquery->where.empty() && !pquery->binarydatareq) //DT9044
	{
		WSUnlockPort(WS_USR,PortNo,true);
		WSLogEvent("ViewServer::SendSqlDetailsLive(), no conditions");
		return 1;
	}
	// Filter against where order conditions
	else if(pquery->FilterOrder(fhint->porder,this)>0)
	{
	#ifdef IQSMP
		unsigned short UscPortNo=(unsigned short)-1;
		unsigned short dlen=0;
		LONGLONG doff=fhint->porder->GetDetail(fhint->didx,UscPortNo,dlen);

		if(((short)dlen<0)||(dlen>4096)) // sanity limit
		{
			_ASSERT(false);
			WSUnlockPort(WS_USR,PortNo,true);
			return 1;
		}
		if(((short)UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
		{
			_ASSERT(false);
			WSUnlockPort(WS_USR,PortNo,true);
			return 1;
		}
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(!dfile)
		{
			// This could fill up the log file otherwise
			WSLogErrorOnce("USC%d: Missing detail file!",UscPortNo);
			WSUnlockPort(WS_USR,PortNo,true);
			return 0;
		}
		fhint->proto=dfile->proto;

		// Load the detail once for all clients
		if(!fhint->pfix)
		{
			if(LoadFixDetail(fhint)<0)
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 1;
			}
		}
		char *dptr=fhint->pfix->fbuf;
		// MSG protocol
		if(dfile->proto==PROTO_CLDROP)
		{
			if(pquery->FilterCldropResult(dptr,dlen,false,this)<1)
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 1;
			}
		}
		else if(dfile->proto==PROTO_CLBACKUP)
		{
			if(pquery->FilterClbackupResult(dptr,dlen,false,this)<1)
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 1;
			}
		}
		// No filtering, send entire detail
		else if((dfile->proto==PROTO_FIXS_PLACELOG)||
				(dfile->proto==PROTO_FIXS_CXLLOG)||
				(dfile->proto==PROTO_FIXS_RPTLOG)||
				(dfile->proto==PROTO_TRADER_PLACELOG)||
				(dfile->proto==PROTO_TRADER_CXLLOG)||
				(dfile->proto==PROTO_TRADER_RPTLOG))
		{
			if((pquery->binarydatareq)&&(pquery->FilterSmpResult(dptr,dlen,false,this)<1))
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 1;
			}
		}
		// CSV embedded into FIX result protocol
		else if((dfile->proto==PROTO_RTOATS)||
				(dfile->proto==PROTO_FIXS_EVELOG)||
				(dfile->proto==PROTO_CLS_EVENTLOG)||
				(dfile->proto==PROTO_TRADER_EVELOG)||
				(dfile->proto==PROTO_CSV))
		{
			FIXINFO ifix;
			ifix.Reset();
			ifix.noSession=true;
			ifix.supressChkErr=true;
			ifix.fbuf=dptr;
			ifix.llen=dlen;
			if(pquery->FilterFixResult(&ifix,false,this)<1)
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 1;
			}
		}
		// FIX protocol
		else
		{
			FIXCONINFO fci;
			memset(&fci,0,sizeof(FIXCONINFO));
			FIXINFO ifix;
			ifix.Reset();
			// Needed for Redi to be able to lookup order info to copy to cancel/replace and fills
			// and for historic instance to set 'BTR.Domain'
			ifix.fci=&fci;
			if(fhint->asys)
				strncpy(fci.LogonTargetCompID,fhint->asys->sname.c_str(),sizeof(fci.LogonTargetCompID));
			if(fhint->ainst)
				strncpy(fci.LogonTargetSubID,fhint->ainst->iname.c_str(),sizeof(fci.LogonTargetSubID));
			fci.mFixSess=(pcFixSession)fhint->asys;
			fci.Proto=fhint->porder->GetOrderDate(); // DEV-22005
			switch(dfile->proto)
			{
			case PROTO_MLFIXLOGS: ifix.FIXDELIM=';'; break;
			case PROTO_TJMLOG: ifix.FIXDELIM=' '; break;
			case PROTO_TBACKUP:
			case PROTO_FULLLOG: ifix.FIXDELIM='|'; break;
			case PROTO_MLPH_EMEA: 
			case PROTO_MLPH_CC: ifix.FIXDELIM=';'; ifix.FIXDELIM2=' '; break;
			case PROTO_FIX:
			case PROTO_MLPUBHUB:
			case PROTO_IQOMS:
			case PROTO_IQFIXS:
			case PROTO_FIXDROP:
			case PROTO_OMS_CBOE:
			case PROTO_TJM: 
			case PROTO_TWIST: 
			case PROTO_RTECHOATS: ifix.FIXDELIM=0x01; break;
			case PROTO_TWISTLOG: ifix.FIXDELIM='_'; break;
			};
			#ifdef REDIOLEDBCON
			if((dfile->proto==PROTO_TWIST)||(dfile->proto==PROTO_RTECHOATS)||((!pquery->binarydatareq)&&(ifix.FIXDELIM)))
			#else
			if((!pquery->binarydatareq)&&(ifix.FIXDELIM))
			#endif
			{
				ifix.noSession=true;
				ifix.supressChkErr=true;
				ifix.FixMsgReady(dptr,(dfile->proto==PROTO_TJMLOG)?dlen -1:dlen);
			#ifdef _DEBUG
				const char *cloid=ifix.TagStr(11);
				if(!*cloid)
				{
					WSLogEvent("USR%d: WARNING: Order(%s) detail #%d is missing tag 11. Possible index corruption!",
						PortNo,fhint->porder->GetOrderKey(),fhint->didx);
					_ASSERT(false);
					WSUnlockPort(WS_USR,PortNo,true);
					return 1;
				}
			#endif
				// Filter where FIX conditions
				if(pquery->FilterFixResult(&ifix,false,this)<1)
				{
					WSUnlockPort(WS_USR,PortNo,true);
					return 1;
				}
			}
		}
		// We always send live data immediately even if the user hasn't finished iterating the historic data
		int nenc=1;
		SendSqlResult(pquery,0,++pquery->spos,0,false,fhint->proto,nenc,false,0);
		ustats->msgCnt+=nenc;
	#else//!IQSMP
		//DT9044
		if(pquery->binarydatareq)
		{
			unsigned short UscPortNo=(unsigned short)-1;
			unsigned short dlen=0;
			LONGLONG doff=fhint->porder->GetDetail(fhint->didx,UscPortNo,dlen);

			if(((short)dlen<0)||(dlen>4096)) // sanity limit
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 0;
			}
			if(((short)UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 0;
			}
			VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
			if(!dfile)
			{
				// This could fill up the log file otherwise
				WSLogErrorOnce("USC%d: Missing detail file!",UscPortNo);
				WSUnlockPort(WS_USR,PortNo,true);
				return 0;
			}

			fhint->proto=dfile->proto;
			char *dptr=new char[dlen+1];
			memset(dptr,0,dlen+1);
			OrderDB *pdb=&fhint->asys->odb;
			if(pdb->ReadDetail(dfile,doff,dptr,dlen)<=0)
			{
				WSUnlockPort(WS_USR,PortNo,true);
				return 0;
			}

			if(pquery->FilterClbackupResult(dptr,dlen,false,this)>0)
			{
				// We always send live data immediately even if the user hasn't finished iterating the historic data
				int nenc=1;
				SendSqlResult(pquery,0,++pquery->spos,0,false,fhint->proto,nenc,false,0);
				ustats->msgCnt+=nenc;
			}
		}
		else
		{
			// FIX protocol

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
			if(pquery->FilterFixResult(fhint->pfix,false,this)>0)
			{
				fhint->proto=PROTO_FIX;
				// We always send live data immediately even if the user hasn't finished iterating the historic data
				int nenc=1;
				SendSqlResult(pquery,0,++pquery->spos,0,false,fhint->proto,nenc,false,0);
				ustats->msgCnt+=nenc;
			}
			else
			{
				WSLogEvent("ViewServer::SendSqlDetailsLive(), pquery->FilterFixResult failed");
			}
		}
	#endif//IQSMP
	}
	// Must return 1 for further updates
	WSUnlockPort(WS_USR,PortNo,true);
	return 1;
}
int _stdcall _SendSqlOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	if(!pquery)
		return -1;
	int rc=pquery->pmod->SendSqlOrdersLive(fuser,skey,taskid,rid,pquery,hint);
	return rc;
}
int ViewServer::SendSqlOrdersLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	if(!pquery)
		return 0;
	FeedHint *fhint=(FeedHint *)hint;
	if(!fuser)
		return 0;
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

	// Disallow live queries with no conditions
	if(pquery->where.empty())
	{
		WSUnlockPort(WS_USR,PortNo,true);
		return 1;
	}
	// TODO: Reduce order updates to once per second
	// Filter against where order conditions
	else if(pquery->FilterOrder(fhint->porder,this)>0)
	{
		// Filter selected columns
		if(pquery->FilterOrderResult(fhint->porder,false,this)>0)
		{
			fhint->proto=PROTO_DSV;
			// We always send live data immediately even if the user hasn't finished iterating the historic data
			int nenc=1;
			SendSqlResult(pquery,0,++pquery->spos,0,false,fhint->proto,nenc,false,0);
			ustats->msgCnt+=nenc;
		}
	}

	// Must return 1 for further updates
	WSUnlockPort(WS_USR,PortNo,true);
	return 1;
}
int _stdcall _SendSqlAccountsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	if(!pquery)
		return -1;
	int rc=pquery->pmod->SendSqlAccountsLive(fuser,skey,taskid,rid,pquery,hint);
	return rc;
}
int ViewServer::SendSqlAccountsLive(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	VSQuery *pquery=(VSQuery *)udata;
	if(!pquery)
		return 0;
	FeedHint *fhint=(FeedHint *)hint;
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

	int nenc=0;
	// TODO: Reduce account updates to once per second
	// AppInstance account
	// Filter against where account conditions
	if(pquery->FilterAccount(fhint->ainst,this)>0)
	{
		// Filter selected columns
		if(pquery->FilterAccountResult(fhint->ainst,false,this)>0)
		{
			fhint->proto=PROTO_DSV; nenc++;
		}
	}
	// AppSystem account
	// Filter against where account conditions
	if(pquery->FilterAccount(fhint->asys,this)>0)
	{
		// Filter selected columns
		if(pquery->FilterAccountResult(fhint->asys,false,this)>0)
		{
			fhint->proto=PROTO_DSV; nenc++;
		}
	}
	// TODO: Evaluate 'match' accounts
	if(nenc>0)
	{
		// We always send live data immediately even if the user hasn't finished iterating the historic data
		SendSqlResult(pquery,0,++pquery->spos,0,false,fhint->proto,nenc,false,0);
		ustats->msgCnt+=nenc;
	}
	// Must return 1 for further updates
	WSUnlockPort(WS_USR,PortNo,true);
	return 1;
}
// The virtual DETAILS table contains all FIX details for all orders.
// The virtual ORDERS table contains all VSOrder details for all orders and parent orders.
// Ex:
// select * from details where 11=<ClOrdID> AND tag(32)>0 (all fill details from <ClOrdID>)
// select 11 from orders where RootOrderID=<RootOrderID> AND term=0 (all open orders under <RootOrderID>)
void ViewServer::VSCNotifySqlRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool hist, bool live, LONGLONG iter)
{
	return VSCNotifySqlRequest2(udata,rid,select,from,where,maxorders,hist,live,iter,1);
}
// Encapsulated historic and live send for SQL query supporting multiple protocols.
// 'ndets' returns the number of details that could be encoded into the 'VSQuery::rbuf' 16K buffer.
int ViewServer::SendSqlResult(VSQuery *pquery, int rc, int endpos, int totpos, bool hist, char proto, int& ndets, bool more, const char *reason)
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
	VSRESULTLIST& rlist=hist ?pquery->hresults :pquery->lresults;
	// more=1 when there is at least one more packet following
	// iter>0 when there is more history to iterate or to provide cancel handle for live query
	LONGLONG iter=0;
	if((pquery->morehist)||(!hist))
		iter=pquery->eiter;
	if(!reason) reason="";

#ifdef IQSMP
	bool datProto=false,fixProto=false;
	if(pquery->from=="ORDERS")
		proto=PROTO_DSV;
	else if(pquery->from=="ACCOUNTS")
	{
		if(proto==PROTO_DAT)
			datProto=true;
		else
			proto=PROTO_DSV;
	}
	#ifdef IQDNS
	else if(pquery->from=="USERS")
		proto=PROTO_DSV;
	else if(pquery->from=="VARSID")
		proto=PROTO_DSV;
	else if(pquery->from=="DOMREL")
		proto=PROTO_DSV;
	else if(pquery->from=="DOMTREE")
		proto=PROTO_DSV;
	else if(pquery->from=="SERVERS")
		proto=PROTO_DSV;
	#endif
	else if(pquery->from=="IQOS")
	{
		proto=PROTO_DAT; datProto=true;
	}
	else
	{
		switch(proto)
		{
		case PROTO_CLDROP:
		case PROTO_CLBACKUP:
		case PROTO_DAT:
			proto=PROTO_DAT; datProto=true;
			break;
		case PROTO_FIXS_PLACELOG:
		case PROTO_FIXS_CXLLOG:
		case PROTO_FIXS_RPTLOG:
		case PROTO_TRADER_PLACELOG:
		case PROTO_TRADER_CXLLOG:
		case PROTO_TRADER_RPTLOG:
			datProto=true;
			break;
		case PROTO_FIX:
		case PROTO_MLFIXLOGS:
		case PROTO_MLPUBHUB:
		case PROTO_IQOMS:
		case PROTO_IQFIXS:
		case PROTO_TJMLOG:
		case PROTO_FIXDROP:
		case PROTO_TBACKUP:
		case PROTO_FULLLOG:
		case PROTO_TJM:
		case PROTO_MLPH_EMEA:
		case PROTO_MLPH_CC:
		case PROTO_TWISTLOG:
		case PROTO_OMS_CBOE:
		case PROTO_RTOATS:
		case PROTO_FIXS_EVELOG:
		case PROTO_CLS_EVENTLOG:
		case PROTO_TRADER_EVELOG:
		case PROTO_CSV:
			fixProto=true;
			break;
		case PROTO_TWIST:
		case PROTO_RTECHOATS:
		case PROTO_UNKNOWN:
			if(pquery->binarydatareq)
			{
				proto=PROTO_DAT; datProto=true;
			}
			else
			{
				proto=PROTO_FIX; fixProto=true;
			}
			break;
		};
	}
#else
	//DT9044
	if(pquery->binarydatareq)
		proto=PROTO_DAT;

	if(!proto)
	{
		if(pquery->from=="ORDERS")
			proto=PROTO_DSV;
		else if(pquery->from=="ACCOUNTS")
			proto=PROTO_DSV;
	#ifdef IQDNS
		else if(pquery->from=="USERS")
			proto=PROTO_DSV;
		else if(pquery->from=="VARSID")
			proto=PROTO_DSV;
		else if(pquery->from=="DOMREL")
			proto=PROTO_DSV;
		else if(pquery->from=="DOMTREE")
			proto=PROTO_DSV;
		else if(pquery->from=="SERVERS")
			proto=PROTO_DSV;
	#endif
		else
			proto=PROTO_FIX;
	}
#endif

	char *rbuf=pquery->rbuf,*rptr=rbuf;
	// DSV encoding
	if(proto==PROTO_DSV)
	{
		int nstr=ndets;
		const char **pstr=0;
		if(nstr>0)
		{
			pstr=new const char *[nstr];
			memset(pstr,0,nstr*sizeof(const char *));
			int i=0;
			for(VSRESULTLIST::iterator rit=rlist.begin();(rit!=rlist.end())&&(i<nstr);rit++,i++)
				pstr[i]=(*rit)->obuf;
			if(i<nstr)
			{
				_ASSERT(false);
				nstr=i;
			}
		}
		if(vsc->EncodeSqlDsvReply(rptr,sizeof(pquery->rbuf),rc,rid,endpos,totpos,hist,proto,pstr,nstr,more,iter,reason)>0)
		{
			if(PortType==WS_USR)
			{
				// Don't want to log live updates, but do want to see live iterator
				if((LOG_QUERIES)&&((hist)||(!nstr)))
				WSLogEvent("USR%d: EncodeSqlDsvReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nstr=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,rc,rid,endpos,totpos,hist,proto,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),(char*)rbuf,PortNo);
				ustats->msgCnt+=nstr;
			}
			else if(PortType==WS_UMR)
			{
				if((LOG_QUERIES)&&((hist)||(!nstr)))
				fprintf(fp,"UMR%d: EncodeSqlDsvReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nstr=%d,more=%d,iter=%d,reason=%s) %d bytes\n",
					PortNo,rc,rid,endpos,totpos,hist,proto,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
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
			if(pstr)
			{
				delete pstr;
				int i=0;
				for(VSRESULTLIST::iterator rit=rlist.begin();(rit!=rlist.end())&&(i<nstr);i++)
				{
					delete *rit;
					#ifdef WIN32
					rit=rlist.erase(rit);
					#else
					rlist.erase(rit++);
					#endif
				}
				ndets=nstr;
			}
			return 1;
		}
		else
		{
			_ASSERT(false);
			if(PortType==WS_USR)
			{
				if((hist)||(!nstr))
				WSLogError("USR%d: EncodeSqlDsvReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes failed to encode!",
					PortNo,rc,rid,endpos,totpos,hist,proto,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
			}
			if(pstr)
				delete pstr;
			return -1;
		}
	}
#ifdef IQSMP
	// Binary encoding
	else if(datProto)
#else
	// DSV encoding
	else if(proto==PROTO_DAT)
#endif
	{
		int nstr=ndets,nskip=0;
		const void **pstr=0;
        short * pstrlen=0;
		if(nstr>0)
		{
			pstr=new const void *[nstr];
			memset(pstr,0,nstr*sizeof(const char *));
            pstrlen=new short[nstr];
			int i=0;
			for(VSRESULTLIST::iterator rit=rlist.begin();(rit!=rlist.end())&&(i<nstr);rit++)
			{
				VSQueryResult* vsres=*rit;
				if(vsres->obuf)
                {
					pstr[i]=vsres->obuf;
                    if(vsres->DataLength > 0)
                        pstrlen[i] = vsres->DataLength;
                    else
                        pstrlen[i] = pquery->binarydatalen;
					i++;
                }
			#ifdef REDIOLEDBCON
				// For TWIST, we want to save space and keep the drop stored as FIX,
				// but convert to BTR for CL and OATS generator.
				else
				{
					if((UsrPort[PortNo].DetPtr8 == (void*)1 ? ConvertFixToBTR(vsres) : ConvertFixToBTR_Old(vsres))<0)
						nskip++;
					else
					{
						pstr[i]=vsres->obuf;
						pstrlen[i]=vsres->DataLength;
						i++;
					}
				}
			#else
				else
					_ASSERT(false);
			#endif
			}
			if(i<nstr)
			{
				//_ASSERT(false); allowed by nskip
				nstr=i;
			}
		}
		if(vsc->EncodeDatReply(rptr,sizeof(pquery->rbuf),rc,rid,endpos,totpos,hist,proto,pstrlen,pstr,nstr,more,iter,reason)>0)
		{
			if(PortType==WS_USR)
			{
				// Don't want to log live updates, but do want to see live iterator
				if((LOG_QUERIES)&&((hist)||(!nstr)))
				WSLogEvent("USR%d: EncodeDatReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nstr=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,rc,rid,endpos,totpos,hist,proto,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),(char*)rbuf,PortNo);
				ustats->msgCnt+=nstr;
			}
			if(pstr)
			{
				delete pstr;
				int i=0;
				for(VSRESULTLIST::iterator rit=rlist.begin();(rit!=rlist.end())&&(i<nstr);i++)
				{
					delete *rit;
					#ifdef WIN32
					rit=rlist.erase(rit);
					#else
					rlist.erase(rit++);
					#endif
				}
				ndets=nstr +nskip;
			}
			return 1;
		}
		else
		{
			_ASSERT(false);
			if(PortType==WS_USR)
			{
				if((hist)||(!nstr))
				WSLogError("USR%d: EncodeDatReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes failed to encode!",
					PortNo,rc,rid,endpos,totpos,hist,proto,nstr,more,(int)iter,reason,(int)(rptr -rbuf));
			}
			if(pstr)
				delete pstr;
			return -1;
		}
	}
	// FIX encoding
#ifdef IQSMP
	else if(fixProto)
#else
	else
#endif
	{
		int nfix=ndets;
		FIXINFO *pfix=0;
		if(nfix>0)
		{
			if(nfix>sizeof(pquery->efix)/sizeof(FIXINFO))
				nfix=sizeof(pquery->efix)/sizeof(FIXINFO);
			int i=0;
			for(VSRESULTLIST::iterator rit=rlist.begin();(rit!=rlist.end())&&(i<nfix);rit++,i++)
				memcpy(&pquery->efix[i],&(*rit)->rfix,sizeof(FIXINFO));
		}
		if(vsc->EncodeSqlFixReply(rptr,sizeof(pquery->rbuf),rc,rid,endpos,totpos,hist,proto,pquery->efix,nfix,more,iter,reason)>0)
		{
			if(PortType==WS_USR)
			{
				// Don't want to log live updates, but do want to see live iterator
				if((LOG_QUERIES)&&((hist)||(!nfix)))
				WSLogEvent("USR%d: EncodeSqlFixReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,rc,rid,endpos,totpos,hist,proto,nfix,more,(int)iter,reason,(int)(rptr -rbuf));
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),(char*)rbuf,PortNo);
				ustats->msgCnt+=nfix;
			}
			else if(PortType==WS_UMR)
			{
				if((LOG_QUERIES)&&((hist)||(!nfix)))
				fprintf(fp,"UMR%d: EncodeSqlFixReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes\n",
					PortNo,rc,rid,endpos,totpos,hist,proto,nfix,more,(int)iter,reason,(int)(rptr -rbuf));
				char fstr[2048]={0};
				for(int f=0;f<nfix;f++)
				{
					int flen=pfix[f].llen;
					if(flen>=sizeof(fstr))
						flen=sizeof(fstr) -1;
					memcpy(fstr,pfix[f].fbuf,flen); fstr[flen]=0;
					fprintf(fp,"%s\n",fstr);
				}
			}
			if(nfix>0)
			{
				int i=0;
				for(VSRESULTLIST::iterator rit=rlist.begin();(rit!=rlist.end())&&(i<nfix);i++)
				{
					delete *rit;
					#ifdef WIN32
					rit=rlist.erase(rit);
					#else
					rlist.erase(rit++);
					#endif
				}
				ndets=nfix;
			}
			memset(pquery->efix,0,sizeof(pquery->efix));
			return 1;
		}
		else
		{
			_ASSERT(false);
			if(PortType==WS_USR)
			{
				if((hist)||(!nfix))
				WSLogError("USR%d: EncodeSqlFixReply(rc=%d,rid=%d,endpos=%d,totpos=%d,hist=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes failed to encode!",
					PortNo,rc,rid,endpos,totpos,hist,proto,nfix,more,(int)iter,reason,(int)(rptr -rbuf));
			}
			memset(pquery->efix,0,sizeof(pquery->efix));
			return -1;
		}
	}
	return -1;
}
int ViewServer::NextAppSystem(VSQuery *pquery, bool first)
{
	_ASSERT(pquery->hist);
	if(!first)
	{
		// End of system iteration
		WaitForSingleObject(asmux,INFINITE);
		if(pquery->asit==asmap.end())
		{
			ReleaseMutex(asmux);
			return -1;
		}
		pquery->asit++;
		while(pquery->asit!=asmap.end())
		{
			if(IsEntitled(pquery->fuser,pquery->asit->second->sname.c_str(),0))
				break;
			else
				pquery->asit++;
		}
		if(pquery->asit==asmap.end())
		{
			ReleaseMutex(asmux);
			return -1;
		}
		ReleaseMutex(asmux);
		pquery->asys->odb.Unlock();
		pquery->asys=pquery->asit->second;
		pquery->ainst=0;
		pquery->ailist.clear();
		pquery->asys->odb.Lock();
	}
try_next:
	OrderDB *pdb=&pquery->asys->odb;
	VSQueryScope& qscope=pquery->qscope;
	int AuxKeyIndex=-1; //DT9398
	// Set up the index iteration pointers
	// Test for indices that have highest probability for smallest search set first
	// Go straight to the child order

	//DT9873
	if(!DetermineBestSearchCond(pquery,qscope,pdb))
	{
		//Old way
		if(qscope.CondClOrdID.size()>0)
		{
			pquery->pidx=&pdb->omap;
			pquery->tpos=(int)pquery->pidx->size();
			// Single value, map search the key
			const char *EqClOrdID=qscope.GetEqStrVal(qscope.CondClOrdID.front());
			if((EqClOrdID)&&(qscope.CondClOrdID.size()<2))
			{
				pquery->kmatch=EqClOrdID;
				if(odb.INDEX_CLORDID_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->iit=pdb->omap.find(EqClOrdID);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->iit=pdb->omap.begin();
			if(pquery->iit==pdb->omap.end())
				pquery->morehist=false;
		}
		// Hex string order location
		else if(qscope.EqOrderLoc>=0)
		{
			pquery->koffset=qscope.EqOrderLoc;
		}
		// EcnOrderID index
		else if(qscope.CondEcnOrderID.size()>0)
		{
			pquery->midx=&pdb->omap2;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqEcnOrderID=qscope.GetEqStrVal(qscope.CondEcnOrderID.front());
			if((EqEcnOrderID)&&(qscope.CondEcnOrderID.size()<2))
			{
				pquery->kmatch=EqEcnOrderID;
				if(odb.INDEX_ECNORDERID_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->omap2.m_find(EqEcnOrderID);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->omap2.m_begin();
			if(pquery->miit==pdb->omap2.m_end())
				pquery->morehist=false;
		}
		// FirstClOrdID index
		else if(qscope.CondFirstClOrdID.size()>0)
		{
			pquery->midx=&pdb->fmap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqFirstClOrdID=qscope.GetEqStrVal(qscope.CondFirstClOrdID.front());
			if((EqFirstClOrdID)&&(qscope.CondFirstClOrdID.size()<2))
			{
				pquery->kmatch=EqFirstClOrdID;
				if(odb.INDEX_FIRSTCLORDID_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->fmap.m_find(EqFirstClOrdID);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->fmap.m_begin();
			if(pquery->miit==pdb->fmap.m_end())
				pquery->morehist=false;
		}
		// ClParentOrderID index
		else if(qscope.CondClParentOrderID.size()>0)
		{
			pquery->midx=&pdb->cpmap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqClParentOrderID=qscope.GetEqStrVal(qscope.CondClParentOrderID.front());
			if((EqClParentOrderID)&&(qscope.CondClParentOrderID.size()<2))
			{
				pquery->kmatch=EqClParentOrderID;
				if(odb.INDEX_CLPARENTORDERID_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->cpmap.m_find(EqClParentOrderID);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->cpmap.m_begin();
			if(pquery->miit==pdb->cpmap.m_end())
				pquery->morehist=false;
		}
		// RootOrderID index
		else if(qscope.CondRootOrderID.size()>0)
		{
			pquery->midx=&pdb->pmap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqRootOrderID=qscope.GetEqStrVal(qscope.CondRootOrderID.front());
			if((EqRootOrderID)&&(qscope.CondRootOrderID.size()<2))
			{
				pquery->kmatch=EqRootOrderID;
				if(odb.INDEX_ROOTORDERID_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->pmap.m_find(EqRootOrderID);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->pmap.m_begin();
			if(pquery->miit==pdb->pmap.m_end())
				pquery->morehist=false;
		}
		// ClientID index
		else if(qscope.CondClientID.size()>0)
		{
			pquery->midx=&pdb->cimap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqClientID=qscope.GetEqStrVal(qscope.CondClientID.front());
			if((EqClientID)&&(qscope.CondClientID.size()<2))
			{
				pquery->kmatch=EqClientID;
				if(odb.INDEX_CLIENTID_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->cimap.m_find(EqClientID);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->cimap.m_begin();
			if(pquery->miit==pdb->cimap.m_end())
				pquery->morehist=false;
		}
		// Symbol index
		else if(qscope.CondSymbol.size()>0)
		{
			pquery->midx=&pdb->symmap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqSymbol=qscope.GetEqStrVal(qscope.CondSymbol.front());
			if((EqSymbol)&&(qscope.CondSymbol.size()<2))
			{
				pquery->kmatch=EqSymbol;
				if(odb.INDEX_SYMBOL_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->symmap.m_find(EqSymbol);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->symmap.m_begin();
			if(pquery->miit==pdb->symmap.m_end())
				pquery->morehist=false;
		}
		// Connection index
		else if(qscope.CondConnection.size()>0)
		{
			pquery->midx=&pdb->cnmap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqConnection=qscope.GetEqStrVal(qscope.CondConnection.front());
			if((EqConnection)&&(qscope.CondConnection.size()<2))
			{
				pquery->kmatch=EqConnection;
				if(odb.INDEX_CONNECTIONS_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->cnmap.m_find(EqConnection);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->cnmap.m_begin();
			if(pquery->miit==pdb->cnmap.m_end())
				pquery->morehist=false;
		}
		// AuxKey index
		//DT9398
		else if(CONDLIST* c=qscope.CondAuxKeySearch(AuxKeyIndex))
		{
			pquery->midx=&pdb->akmap[AuxKeyIndex];
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqAuxKey=qscope.GetEqStrVal(c->front());
			if((EqAuxKey)&&(c->size()<2))
			{
				pquery->kmatch=EqAuxKey;
				if(odb.INDEX_AUXKEYS_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->akmap[AuxKeyIndex].m_find(EqAuxKey);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->akmap[AuxKeyIndex].m_begin();
			if(pquery->miit==pdb->akmap[AuxKeyIndex].m_end())
				pquery->morehist=false;
		}
		// Open orders index
		else if(qscope.CondOpenOrder.size()>0)
		{
			pquery->pidx=&pdb->oomap;
			pquery->tpos=(int)pquery->pidx->size();
			pquery->iit=pdb->oomap.begin();
			if(pquery->iit==pdb->oomap.end())
				pquery->morehist=false;
		}
		// Filled orders index
		else if((qscope.CondFillQty.size()>0)||(qscope.CondCumQty.size()>0))
		{
			pquery->pidx=&pdb->fomap;
			pquery->tpos=(int)pquery->pidx->size();
			pquery->iit=pdb->fomap.begin();
			if(pquery->iit==pdb->fomap.end())
				pquery->morehist=false;
		}
		// Account index 
		else if(qscope.CondAccount.size()>0)
		{
			pquery->midx=&pdb->acctmap;
			pquery->tpos=(int)pquery->midx->m_size();
			// Single value, map search the key
			const char *EqAccount=qscope.GetEqStrVal(qscope.CondAccount.front());
			if((EqAccount)&&(qscope.CondAccount.size()<2))
			{
				pquery->kmatch=EqAccount;
				if(odb.INDEX_ACCOUNT_CS)  //DT9491
					pquery->CaseSen=true;
				pquery->miit=pdb->acctmap.m_find(EqAccount);
			}
			// Multiple values, we'll have to walk all keys
			else
				pquery->miit=pdb->acctmap.m_begin();
			if(pquery->miit==pdb->acctmap.m_end())
				pquery->morehist=false;
		}
		// TransactTime index
		else if(qscope.CondTransactTime.size()>0)
		{
			pquery->tidx=&pdb->tsmap;
			pquery->tpos=(int)pquery->tidx->m_size();
			// Single value, map search the key
			const char *EqTransactTime=qscope.GetEqStrVal(qscope.CondTransactTime.front());
			if((EqTransactTime)&&(qscope.CondTransactTime.size()<2))
			{
				pquery->kts=myatoi64(EqTransactTime);
				pquery->tiit=pdb->tsmap.m_find(pquery->kts);
			}
			// Multiple values, we'll have to walk all keys
			else
			{
				pquery->kts=0;
				pquery->tiit=pdb->tsmap.m_begin();
			}
			if(pquery->tiit==pdb->tsmap.m_end())
				pquery->morehist=false;
		}
	#ifdef MULTI_DAY
		// OrderDate index
		else if(qscope.CondOrderDate.size()>0)
		{
			pquery->didx=&pdb->odmap;
			pquery->tpos=(int)pquery->didx->size();
			// Single value, map search the key
			int EqOrderDate=(int)qscope.GetEqIntVal(qscope.CondOrderDate.front());
			if((EqOrderDate)&&(qscope.CondOrderDate.size()<2))
			{
				pquery->kdate=EqOrderDate;
				pquery->diit=pdb->odmap.find(pquery->kdate);
			}
			// Multiple values, we'll have to walk all keys
			else
			{
				pquery->kdate=0;
				pquery->diit=pdb->odmap.begin();
			}
			if(pquery->diit==pdb->odmap.end())
				pquery->morehist=false;
		}
	#endif
		// AppInst index (default)
		// Walk all orders in the system
		else//if((!qscope.EqAppInstID.empty())||(qscope.CondTermOrder.size()>0))
		{
			// All instances in the system
			if((qscope.CondAppSystem.empty())&&(qscope.CondAppInstID.empty()))
			{
				pquery->ailist.clear();
 				for(APPINSTMAP::iterator iit=pquery->asys->imap.begin();iit!=pquery->asys->imap.end();iit++)
					pquery->ailist.push_back(iit->second);
			}
			pquery->aiit=pquery->ailist.begin();
			if(pquery->aiit==pquery->ailist.end())
				pquery->morehist=false;
			else
			{
				pquery->ainst=*pquery->aiit;
				pquery->aoit=(*pquery->aiit)->oset.begin();
			}
			pquery->tpos=ocnt; // worst case (TODO: track counts of orders in each AppSystem)
		}
	}
	// Empty index result
	if(!pquery->morehist)
	{
		// Check the Next app system
		WaitForSingleObject(asmux,INFINITE);
		if(pquery->asit!=asmap.end())
			pquery->asit++;
		while(pquery->asit!=asmap.end())
		{
			pquery->asys->odb.Unlock();
			if(IsEntitled(pquery->fuser,pquery->asit->second->sname.c_str(),0))
			{
				pquery->asys=pquery->asit->second;
				ReleaseMutex(asmux);
				pquery->morehist=true;
				pquery->asys->odb.Lock();
				goto try_next;
			}
			else
				pquery->asit++;
		}
		ReleaseMutex(asmux);
		if(first)
		{
			// Either key not found or nothing in the selected index
			int nenc=0;
			SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,0,nenc,false,0);
			return -1;
		}
		// No more systems
		return -1;
	}
	return 0;
}
#ifdef IQSMP
// Slave function for ContinueDetailsQuery
int ViewServer::SendResults(int& nfix, VSQuery *pquery, int proto, int& tsent, bool needMoreEnd, list<char*>& dlist)
{
	int nenc=nfix;
	#ifdef MULTI_PROTO_DETAILS
	if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,/*proto*/0,nenc,true,0)>0)
	#else
	if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
	#endif
	{
		tsent+=nenc; nfix-=nenc; needMoreEnd=true;
	}
	else
		nfix=0;
	// unload details that were sent (may not be all of them)
	int i=0;
	#ifdef MULTI_PROTO_DETAILS
	for(list<pair<int,char*>>::iterator dit=dlist.begin();
		(dit!=dlist.end())&&(i<nenc);
		#ifdef WIN32
		dit=dlist.erase(dit),i++)
		#else
		dlist.erase(dit++),i++)
		#endif
		delete dit->second;
	#else
	for(list<char*>::iterator dit=dlist.begin();
		(dit!=dlist.end())&&(i<nenc);
		#ifdef WIN32
		dit=dlist.erase(dit),i++)
		#else
		dlist.erase(dit++),i++)
		#endif
		delete *dit;
	#endif
	return 0;
}
#endif
// Virtual DETAILS table (colums are union of every FIX tag present in result set)
int ViewServer::ContinueDetailsQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;
	AppSystem *asys=pquery->asys;
	OrderDB *pdb=&asys->odb;
#ifdef MULTI_DAY_HIST
	if(pdb->HISTORIC_DAYS>0)
	{
		OrderDB *sdb=asys->GetDB(pquery->kdate);
		if(sdb) pdb=sdb;
	}
#endif
	pdb->Lock();

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,nfix=0,proto=0;
		#ifdef IQSMP
		int lproto=0;
		#endif
		list<char *> dlist; // list of details that need unloading
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			_ASSERT(pquery->asys);
			// We are limited to 'pquery->maxorders' orders to search per second,
			// which may yield an 'olist' smaller than that, depending on where conditions
			list<VSOrder*> olist;
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				ITEMLOC oloc;
				// Order offset
				if(pquery->koffset>=0)
				{
					if(pquery->morehist)
					{
						oloc=pquery->koffset;
						pquery->spos++; pquery->scontinue++;
						pquery->morehist=false;
					}
					else
						break;
				}
				// Single index
				else if(pquery->pidx)
				{
					if((pquery->iit==pquery->pidx->end())||
						((!pquery->kmatch.empty())&&
						(pquery->CaseSen ? strcmp(pquery->kmatch.c_str(),pquery->iit.first.c_str()) : _stricmp(pquery->kmatch.c_str(),pquery->iit.first.c_str())))) //DT9491
					{
						// If no candidates yet, then we can change pdb.
						// Otherwise, we need the keep the same pdb to read the order.
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->iit.second;
					pquery->iit++; pquery->spos++; pquery->scontinue++;
				}
				// Multi index
				else if(pquery->midx)
				{
					if((pquery->miit==pquery->midx->m_end())||
					   ((!pquery->kmatch.empty())&&
					   (pquery->CaseSen ? strcmp(pquery->kmatch.c_str(),pquery->miit.first.c_str()) : _stricmp(pquery->kmatch.c_str(),pquery->miit.first.c_str())))) //DT9491
					{
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->miit.second;
					pquery->miit++; pquery->spos++; pquery->scontinue++;
				}
				// Time index
				else if(pquery->tidx)
				{
					if((pquery->tiit==pquery->tidx->m_end())||
					   ((pquery->kts)&&(pquery->kts!=pquery->tiit.first)))
					{
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->tiit.second;
					pquery->tiit++; pquery->spos++; pquery->scontinue++;
				}
				// AppInstance index
				else if(!pquery->ailist.empty())
				{
					while(pquery->aoit==pquery->ainst->oset.end())
					{
						pquery->aiit++;
						if(pquery->aiit==pquery->ailist.end())
						{
							if(NextAppSystem(pquery,false)<0)
							{
								pquery->morehist=false;
								break;
							}
							else if(olist.empty())
							{
								pdb=&pquery->asys->odb;
							#ifdef MULTI_DAY_HIST
								if(pdb->HISTORIC_DAYS>0)
								{
									OrderDB *sdb=asys->GetDB(pquery->kdate);
									if(sdb) pdb=sdb;
								}
							#endif
							}
						}
						pquery->ainst=*pquery->aiit;
						pquery->aoit=pquery->ainst->oset.begin();
					}
				#ifdef MULTI_DAY_HIST
					OrderDB *asdb=&pquery->asys->odb;
					if(pdb->HISTORIC_DAYS>0)
					{
						OrderDB *sdb=asys->GetDB(pquery->kdate);
						if(sdb) asdb=sdb;
					}
					if((!pquery->morehist)||(pdb!=asdb))
						break;
				#else
					if((!pquery->morehist)||(pdb!=&pquery->asys->odb))
						break;
				#endif
					oloc=*pquery->aoit;
					pquery->aoit++; pquery->spos++; pquery->scontinue++;
				}
				// If the system has no instances
				else
				{
					pquery->morehist=false;
					break;
				}
				// Check order against where order conditions before adding to list
				if((pquery->kdate)&&(oloc.wsdate!=pquery->kdate))
					continue;
				VSOrder *porder=pdb->ReadOrder(oloc);
				if(!porder)
					continue;
			#ifdef MULTI_RECTYPES
				if((porder->GetType()!=0x01)&&(porder->GetType()!=0x05))
					continue;
			#endif
			#ifdef MULTI_DAY
				// Filter by date
				//if((pquery->kdate>0)&&(porder->GetLocaleOrderDate(QueryTimeZoneBias)!=pquery->kdate))
				if((pquery->kdate>0)&&(porder->GetOrderDate()!=pquery->kdate))
					continue;
			#endif
				if(pquery->FilterOrder(porder,this)>0)
					olist.push_back(porder);
				else
					pdb->FreeOrder(porder);
			}//for(int ncheck

			// NextAppSystem may have changed 'pquery->asys', but we need to read the 
			// orders from the system with the 'pdb' database. Avoid holding mutex for
			// more than one system at a time.
		#ifdef MULTI_DAY_HIST
			OrderDB *asdb=&pquery->asys->odb;
			if(pdb->HISTORIC_DAYS>0)
			{
				OrderDB *sdb=asys->GetDB(pquery->kdate);
				if(sdb) asdb=sdb;
			}
			if(pdb!=asdb)
			{
				asdb->Unlock();
				pdb->Lock();
			}
		#else
			if(pdb!=&pquery->asys->odb)
			{
				pquery->asys->odb.Unlock();
				pdb->Lock();
			}
		#endif
			// Read all orders' details
			FIXINFO ifix;
			FIXCONINFO fci;
			memset(&fci,0,sizeof(FIXCONINFO));
			ifix.noSession=true;
			ifix.supressChkErr=true;
			// Walk details for all matched orders. If a user is entitled to details for an order, 
			// then he should get them all in the same iteration. 
			for(list<VSOrder*>::iterator lit=olist.begin();lit!=olist.end();lit++)
			{
				VSOrder *porder=*lit;
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
				#ifdef MULTI_DAY_HIST
					if(pdb->HISTORIC_DAYS>0)
						UscPort[UscPortNo].DetPtr1=odb.GetDetailFile(UscPortNo,pquery->kdate);
				#endif
					VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
					if(!dfile)
					{
						// This could fill up the log file otherwise
						WSLogErrorOnce("USC%d: Missing detail file!",UscPortNo);
						continue;
					}
					proto=dfile->proto;
					char *dptr=new char[dlen+1];
					memset(dptr,0,dlen+1);

					if(pdb->ReadDetail(dfile,doff,dptr,dlen)>0)
					{
					#ifdef IQSMP
						// MSG protocol
						if(dfile->proto==PROTO_CLDROP)
						{
							if(pquery->FilterCldropResult(dptr,dlen,true,this)>0)
							{
								proto=dfile->proto;
								#ifdef IQSMP
								if((lproto)&&(proto!=lproto))
									SendResults(nfix,pquery,lproto,tsent,needMoreEnd,dlist);
								lproto=proto;
								#endif
								nfix++;
								#ifdef MULTI_PROTO_DETAILS
								dlist.push_back(pair<int,char*>(dfile->proto,dptr));
								#else
								dlist.push_back(dptr);
								#endif
								// Send filled blocks as soon as possible
								if(nfix>=16)
									SendResults(nfix,pquery,proto,tsent,needMoreEnd,dlist);
							}
							else
								delete dptr;
						}
						else if(dfile->proto==PROTO_CLBACKUP)
						{
							if(pquery->FilterClbackupResult(dptr,dlen,true,this)>0)
							{
								proto=dfile->proto;
								if((lproto)&&(proto!=lproto))
									SendResults(nfix,pquery,lproto,tsent,needMoreEnd,dlist);
								lproto=proto;

								nfix++;
								#ifdef MULTI_PROTO_DETAILS
								dlist.push_back(pair<int,char*>(dfile->proto,dptr));
								#else
								dlist.push_back(dptr);
								#endif
								// Send filled blocks as soon as possible
								if(nfix>=16)
									SendResults(nfix,pquery,proto,tsent,needMoreEnd,dlist);
							}
							else
							{
								delete dptr;
							}
						}
						// No filtering, send entire detail
						else if((dfile->proto==PROTO_FIXS_PLACELOG)||
								(dfile->proto==PROTO_FIXS_CXLLOG)||
								(dfile->proto==PROTO_FIXS_RPTLOG)||
								(dfile->proto==PROTO_TRADER_PLACELOG)||
								(dfile->proto==PROTO_TRADER_CXLLOG)||
								(dfile->proto==PROTO_TRADER_RPTLOG))
						{
							#ifdef MULTI_PROTO_DETAILS
							if(pquery->FilterSmpResult(dfile->proto,dptr,dlen,true,this)>0)
							#else
							if((pquery->binarydatareq)&&(pquery->FilterSmpResult(dptr,dlen,true,this)>0))
							#endif
							{
								proto=dfile->proto;
								if((lproto)&&(proto!=lproto))
									SendResults(nfix,pquery,lproto,tsent,needMoreEnd,dlist);
								lproto=proto;

								nfix++;
								#ifdef MULTI_PROTO_DETAILS
								dlist.push_back(pair<int,char*>(dfile->proto,dptr));
								#else
								dlist.push_back(dptr);
								#endif
								// Send filled blocks as soon as possible
								if(nfix>=16)
									SendResults(nfix,pquery,proto,tsent,needMoreEnd,dlist);
							}
							else
							{
								delete dptr;
							}
						}
						// CSV embedded into FIX result protocol
						else if((dfile->proto==PROTO_RTOATS)||
								(dfile->proto==PROTO_FIXS_EVELOG)||
								(dfile->proto==PROTO_CLS_EVENTLOG)||
								(dfile->proto==PROTO_TRADER_EVELOG)||
								(dfile->proto==PROTO_CSV))
						{
							ifix.Reset();
							ifix.noSession=true;
							ifix.supressChkErr=true;
							ifix.fbuf=dptr;
							ifix.llen=dlen;
							if(pquery->FilterFixResult(&ifix,true,this)>0)
							{
								proto=dfile->proto;
								if((lproto)&&(proto!=lproto))
									SendResults(nfix,pquery,lproto,tsent,needMoreEnd,dlist);
								lproto=proto;

								nfix++;
								#ifdef MULTI_PROTO_DETAILS
								dlist.push_back(pair<int,char*>(dfile->proto,dptr));
								#else
								dlist.push_back(dptr);
								#endif
								if(nfix>=16)
									SendResults(nfix,pquery,proto,tsent,needMoreEnd,dlist);
							}
						}
						// FIX protocol
						else
						{
							ifix.Reset();
							// Needed for Redi to be able to lookup order info to copy to cancel/replace and fills
							ifix.fci=&fci;
							strncpy(fci.LogonTargetCompID,asys->sname.c_str(),sizeof(fci.LogonTargetCompID));
							strncpy(fci.LogonTargetSubID,porder->GetAppInstID(),sizeof(fci.LogonTargetSubID));
							fci.mFixSess=(pcFixSession)asys;
							fci.Proto=porder->GetOrderDate(); // DEV-22005
							switch(dfile->proto)
							{
							case PROTO_MLFIXLOGS: ifix.FIXDELIM=';'; break;
							case PROTO_TJMLOG: ifix.FIXDELIM=' '; break;
							case PROTO_TBACKUP:
							case PROTO_FULLLOG: ifix.FIXDELIM='|'; break;
							case PROTO_MLPH_EMEA: 
							case PROTO_MLPH_CC: ifix.FIXDELIM=';'; ifix.FIXDELIM2=' '; break;
							case PROTO_FIX:
							case PROTO_MLPUBHUB:
							case PROTO_IQOMS:
							case PROTO_IQFIXS:
							case PROTO_FIXDROP:
							case PROTO_OMS_CBOE:
							case PROTO_TJM: 
							case PROTO_TWIST: 
							case PROTO_RTECHOATS: ifix.FIXDELIM=0x01; break;
							case PROTO_TWISTLOG: ifix.FIXDELIM='_'; break;
							};
							#ifdef MULTI_PROTO_DETAILS
							if(ifix.FIXDELIM)
							#elif defined(REDIOLEDBCON)
							if((dfile->proto==PROTO_TWIST)||(dfile->proto==PROTO_RTECHOATS)||((!pquery->binarydatareq)&&(ifix.FIXDELIM)))
							#else
							if((!pquery->binarydatareq)&&(ifix.FIXDELIM))
							#endif
							{
								ifix.noSession=true;
								ifix.supressChkErr=true;
								ifix.FixMsgReady(dptr,(dfile->proto==PROTO_TJMLOG)?dlen -1:dlen);
							// Orders missing tag 11 allowed now
							//#ifdef _DEBUG
							//	const char *cloid=ifix.TagStr(11);
							//	if(!*cloid)
							//	{
							//		WSLogEvent("USR%d: WARNING: Order(%s) detail #%d is missing tag 11. Possible index corruption!",
							//			PortNo,porder->GetOrderKey(),d);
							//		_ASSERT(false);
							//		delete dptr;
							//		//if(strcmp(from,"*")!=0)
							//			continue;
							//	}
							//#endif
								// Filter where FIX conditions
								#ifdef MULTI_PROTO_DETAILS
								if(pquery->FilterFixResult(dfile->proto,&ifix,true,this)>0)
								#else
								if(pquery->FilterFixResult(&ifix,true,this)>0)
								#endif
								{
									proto=dfile->proto;
									if((lproto)&&(proto!=lproto))
										SendResults(nfix,pquery,lproto,tsent,needMoreEnd,dlist);
									lproto=proto;

									nfix++;
									#ifdef MULTI_PROTO_DETAILS
									dlist.push_back(pair<int,char*>(dfile->proto,dptr));
									#else
									dlist.push_back(dptr);
									#endif
									// Send filled blocks as soon as possible
									if(nfix>=16)
										SendResults(nfix,pquery,proto,tsent,needMoreEnd,dlist);
								}
								else
									delete dptr;
							}
							else
								delete dptr;
						}
					#else//!IQSMP
						// MSG protocol
						if(dfile->proto==PROTO_CLDROP)
						{
							if(pquery->FilterCldropResult(dptr,dlen,true,this)>0)
							{
								nfix++;
								dlist.push_back(dptr);
								// Send filled blocks as soon as possible
								if(nfix>=16)
								{
									int nenc=nfix;
									if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
									{
										tsent+=nenc; nfix-=nenc; needMoreEnd=true;
									}
									else
										nfix=0;
									// unload details that were sent (may not be all of them)
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
							else
								delete dptr;
						}
						else if(dfile->proto==PROTO_CLBACKUP)
						{
							if(pquery->FilterClbackupResult(dptr,dlen,true,this)>0)
							{
								nfix++;
								dlist.push_back(dptr);
								// Send filled blocks as soon as possible
								if(nfix>=16)
								{
									int nenc=nfix;
									if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
									{
										tsent+=nenc; nfix-=nenc; needMoreEnd=true;
									}
									else
										nfix=0;

									// unload details that were sent (may not be all of them)
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
							else
							{
								delete dptr;
							}
						}
						// FIX protocol
						else
						{
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
						#ifdef _DEBUG
							const char *cloid=ifix.TagStr(11);
							if(!*cloid)
							{
								WSLogEvent("USR%d: WARNING: Order(%s) detail #%d is missing tag 11. Possible index corruption!",
									PortNo,porder->GetOrderKey(),d);
								_ASSERT(false);
								delete dptr;
								//if(strcmp(from,"*")!=0)
									continue;
							}
						#endif
							// Filter where FIX conditions
							if(pquery->FilterFixResult(&ifix,true,this)>0)
							{
								nfix++;
								dlist.push_back(dptr);
								// Send filled blocks as soon as possible
								if(nfix>=16)
								{
									int nenc=nfix;
									if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,proto,nenc,true,0)>0)
									{
										tsent+=nenc; nfix-=nenc; needMoreEnd=true;
									}
									else
										nfix=0;
									// unload details that were sent (may not be all of them)
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
							else
								delete dptr;
						}
					#endif//!IQSMP
					}
					else
					{
						delete dptr;
					}
				}//for(int d=0;d<ndets;d++)
				pdb->FreeOrder(porder); porder=0;
			}//for(VSINDEX::iterator
			olist.clear();
		}

		// Send last packets
		while((nfix>0)||(needMoreEnd))
		{
			int nenc=nfix;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,proto,nenc,false,0)>0)
			{
				tsent+=nenc; nfix-=nenc;
			}
			else
				nfix=0;
			if(nfix<1)
				needMoreEnd=false;
		}
		_ASSERT(nfix<1);
		// unload details
		for(list<char*>::iterator dit=dlist.begin();dit!=dlist.end();dit++)
		{
			char *dptr=*dit;
			delete dptr;
		}
		dlist.clear();
	}

	// No live data buffering supported
	//// Send buffered live data when all historic done
	//if((pquery->live)&&(!pquery->morehist)&&(!pquery->lresults.empty()))
	//{
	//	int nenc=(int)pquery->lresults.size();
	//	pquery->spos+=nenc;
	//	pquery->tpos+=nenc;
	//	SendSqlResult(pquery,0,pquery->spos,pquery->tpos,false,PROTO_DSV,nenc);
	//}
	pdb->Unlock();
	return 0;
}
// Virtual ORDERS table (colums are all attributes in VSOrder object)
int ViewServer::ContinueOrdersQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;
	AppSystem *asys=pquery->asys;
	OrderDB *pdb=&asys->odb;
#ifdef MULTI_DAY_HIST
	if(pdb->HISTORIC_DAYS>0)
	{
		OrderDB *sdb=asys->GetDB(pquery->kdate);
		if(sdb) pdb=sdb;
	}
#endif
	pdb->Lock();

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			// We are limited to 'pquery->maxorders' orders to search per second,
			// which may yield an 'olist' smaller than that, depending on where conditions
			list<VSOrder *> olist;
		#ifdef MULTI_DAY
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);)
		#else
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
		#endif
			{
				ITEMLOC oloc;
			#ifdef MULTI_DAY_ITEMLOC
				oloc.wsdate=pdb->wsdate;
			#endif
				// Order offset
				if(pquery->koffset>=0)
				{
					if(pquery->morehist)
					{
						oloc=pquery->koffset;
						pquery->spos++; pquery->scontinue++;
						pquery->morehist=false;
					}
					else
						break;
				}
				// Single index
				else if(pquery->pidx)
				{
					if((pquery->iit==pquery->pidx->end())||
						((!pquery->kmatch.empty())&&
						(pquery->CaseSen ? strcmp(pquery->kmatch.c_str(),pquery->iit.first.c_str()) : _stricmp(pquery->kmatch.c_str(),pquery->iit.first.c_str())))) //DT9491
					{
						// If no candidates yet, then we can change pdb.
						// Otherwise, we need the keep the same pdb to read the order.
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->iit.second;
					pquery->iit++; pquery->spos++; pquery->scontinue++;
				}
				// Multi index
				else if(pquery->midx)
				{
					if((pquery->miit==pquery->midx->m_end())||
					   ((!pquery->kmatch.empty())&&
					   (pquery->CaseSen ? strcmp(pquery->kmatch.c_str(),pquery->miit.first.c_str()) : _stricmp(pquery->kmatch.c_str(),pquery->miit.first.c_str())))) //DT9491
					{
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->miit.second;
					pquery->miit++; pquery->spos++; pquery->scontinue++;
				}
				// Time index
				else if(pquery->tidx)
				{
					if((pquery->tiit==pquery->tidx->m_end())||
					   ((pquery->kts)&&(pquery->kts!=pquery->tiit.first)))
					{
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->tiit.second;
					pquery->tiit++; pquery->spos++; pquery->scontinue++;
				}
			#ifdef MULTI_DAY
				// Date index
				else if(pquery->didx)
				{
					if((pquery->diit==pquery->didx->end())||
					   ((pquery->kdate)&&((int)pquery->kdate!=pquery->diit->first)))
					{
						if(NextAppSystem(pquery,false)<0)
						{
							pquery->morehist=false;
							break;
						}
						else if(olist.empty())
						{
							pdb=&pquery->asys->odb;
						#ifdef MULTI_DAY_HIST
							if(pdb->HISTORIC_DAYS>0)
							{
								OrderDB *sdb=asys->GetDB(pquery->kdate);
								if(sdb) pdb=sdb;
							}
						#endif
						}
						else
							break;
					}
					oloc=pquery->diit->second;
					pquery->diit++; pquery->spos++; pquery->scontinue++;
				}
			#endif
				// AppInstance index
				else if(!pquery->ailist.empty())
				{
					while(pquery->aoit==pquery->ainst->oset.end())
					{
						pquery->aiit++;
						if(pquery->aiit==pquery->ailist.end())
						{
							if(NextAppSystem(pquery,false)<0)
							{
								pquery->morehist=false;
								break;
							}
							else if(olist.empty())
							{
								pdb=&pquery->asys->odb;
							#ifdef MULTI_DAY_HIST
								if(pdb->HISTORIC_DAYS>0)
								{
									OrderDB *sdb=asys->GetDB(pquery->kdate);
									if(sdb) pdb=sdb;
								}
							#endif
							}
						}
						pquery->ainst=*pquery->aiit;
						pquery->aoit=pquery->ainst->oset.begin();
					}
				#ifdef MULTI_DAY_HIST
					OrderDB *asdb=&pquery->asys->odb;
					if(pdb->HISTORIC_DAYS>0)
					{
						OrderDB *sdb=asys->GetDB(pquery->kdate);
						if(sdb) asdb=sdb;
					}
					if((!pquery->morehist)||(pdb!=asdb))
						break;
				#else
					if((!pquery->morehist)||(pdb!=&pquery->asys->odb))
						break;
				#endif
					oloc=*pquery->aoit;
					pquery->aoit++; pquery->spos++; pquery->scontinue++;
				}
				else
				{
					_ASSERT(false);
					break;
				}
			#ifdef MULTI_DAY
				// "join" on OrderDate
				#ifdef MULTI_DAY_ITEMLOC
				if((pquery->kdate>0)&&(pdb->HISTORIC_DAYS<=0)&&(pdb->INDEX_ORDERDATE)&&(oloc.wsdate!=pquery->kdate))
				#else
				if((pquery->kdate>0)&&(pdb->HISTORIC_DAYS<=0)&&(pdb->INDEX_ORDERDATE)&&(pquery->odset.find(oloc)==pquery->odset.end()))
				#endif
					continue;
				// Multi-day join eliminations don't count against throttle limit
				ncheck++;
			#endif
				// Check order against where order conditions before adding to list
				VSOrder *porder=pdb->ReadOrder(oloc);
				if(!porder)
					continue;
			#ifdef MULTI_RECTYPES
				if((porder->GetType()!=0x01)&&(porder->GetType()!=0x05))
					continue;
			#endif
			//#ifdef MULTI_DAY
			//	// Filter by date
			//	//if((pquery->kdate>0)&&(porder->GetLocaleOrderDate(tzbias)!=pquery->kdate))
			//	if((pquery->kdate>0)&&(porder->GetOrderDate()!=pquery->kdate))
			//		continue;
			//#endif

				if(pquery->FilterOrder(porder,this)>0)
					olist.push_back(porder);
				else
					pdb->FreeOrder(porder);
			}//for(int ncheck

			// NextAppSystem may have changed 'pquery->asys', but we need to read the 
			// orders from the system with the 'pdb' database. Avoid holding mutex for
			// more than one system at a time.
		#ifdef MULTI_DAY_HIST
			OrderDB *asdb=&pquery->asys->odb;
			if(pdb->HISTORIC_DAYS>0)
			{
				OrderDB *sdb=asys->GetDB(pquery->kdate);
				if(sdb) asdb=sdb;
			}
			if(pdb!=&pquery->asys->odb)
			{
				asdb->Unlock();
				pdb->Lock();
			}
		#else
			if(pdb!=&pquery->asys->odb)
			{
				pquery->asys->odb.Unlock();
				pdb->Lock();
			}
		#endif
			// Read orders
			char proto=0;
			for(list<VSOrder*>::iterator lit=olist.begin();lit!=olist.end();lit++)
			{
				VSOrder *porder=*lit;
				// Filter selected order columns
				if(pquery->FilterOrderResult(porder,true,this)>0)
				{
					ndets++;
					// Send filled blocks as soon as possible
					if(ndets>=256)
					{
						int nenc=ndets;
						if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,true,0)>0)
						{
							tsent+=nenc; ndets-=nenc; needMoreEnd=true;
						}
						else
							ndets=0;
					}
				}
				pdb->FreeOrder(porder); porder=0;
			}//for(olist
			olist.clear();
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}

	// No live data buffering supported
	//// Send buffered live data when all historic done
	//if((pquery->live)&&(!pquery->morehist)&&(!pquery->lresults.empty()))
	//{
	//	int nenc=(int)pquery->lresults.size();
	//	pquery->spos+=nenc;
	//	pquery->tpos+=nenc;
	//	SendSqlResult(pquery,0,pquery->spos,pquery->tpos,false,PROTO_DSV,nenc);
	//}
	pdb->Unlock();
	return 0;
}
int ViewServer::StartAccountsQuery(VSQuery *pquery)
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
	pquery->acc=0;
	WaitForSingleObject(asmux,INFINITE);
	for(CONDLIST::iterator cit=qscope.CondAppInstID.begin();cit!=qscope.CondAppInstID.end();cit++)
	{
		ExprTok *etok=*cit;
		if(!strcmp(etok->oval,"=="))
		{
			const char *AcctName=etok->sval;
			ACCTMAP::iterator ait=acmap.find(AcctName);
			if(ait!=acmap.end())
			{
				// TODO: Account entitlements
				if(/*IsEntitled(pquery->fuser,qscope.EqAcctName.c_str())*/true)
					pquery->aclist.push_back(ait->second);
				else
				{
					ReleaseMutex(asmux);
					char reason[256]={0};
					sprintf(reason,"User not entitled to account '%s'.",AcctName);
					if(LOG_QUERIES>0)
						WSLogEvent("USR%d: %s",pquery->PortNo,reason);
					int nenc=0;
					SendSqlResult(pquery,-1,0,0,true,0,nenc,false,reason);
					return -1;
				}
			}
		}
	}
	pquery->aclit=pquery->aclist.begin();
	if(pquery->aclit!=pquery->aclist.end())
		pquery->acc=*pquery->aclit;
	if(pquery->acc)
		pquery->acit=acmap.end();
	else
	{
		pquery->acit=acmap.begin();
		if(pquery->acit==acmap.end())
		{
			ReleaseMutex(asmux);
			int nenc=0;
			//DT9044 Minor bug fix
			pquery->morehist=false;
			SendSqlResult(pquery,0,0,0,true,0,nenc,false,0);
			return 0;
		}
		else
			pquery->acc=pquery->acit->second;
	}
	ReleaseMutex(asmux);

	if(pquery->hist)
	{
		pquery->morehist=true;
		pquery->spos=0; // 'supos' not used for SQL query
	}
	else
		pquery->morehist=false;
	return 0;
}
// Virtual ACCOUNTS table (colums are all attributes in VSOrder object)
int ViewServer::ContinueAccountsQuery(VSQuery *pquery)
{
	int rid=pquery->rid;
	int PortType=pquery->PortType;
	int PortNo=pquery->PortNo;
	FILE *fp=pquery->fp;

	// Iterate historic data
	if(pquery->hist)
	{
		int tsent=0,ndets=0;
		bool needMoreEnd=true;
		if(pquery->morehist)
		{
			for(int ncheck=0;(ncheck<pquery->maxorders)&&(pquery->morehist);ncheck++)
			{
				if(pquery->FilterAccount(pquery->acc,this)>0)
				{
					if(pquery->FilterAccountResult(pquery->acc,pquery->hist,this)>0)
					{
						ndets++;
						// Send filled blocks as soon as possible
						if(ndets>=256)
						{
							int nenc=ndets;
							if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,pquery->binarydatareq ?PROTO_DAT :PROTO_DSV,nenc,true,0)>0)
							{
								tsent+=nenc; ndets-=nenc; needMoreEnd=true;
							}
							else
								ndets=0;
						}
					}
				}
				// Account index
				pquery->acit++; pquery->spos++; pquery->scontinue++;
				if(pquery->acit==acmap.end())
				{
					pquery->morehist=false;
					break;
				}
				pquery->acc=pquery->acit->second;
			}
		}
		// Send last packet
		while((ndets>0)||(needMoreEnd))
		{
			int nenc=ndets;
			if(SendSqlResult(pquery,0,pquery->spos,pquery->tpos,true,pquery->binarydatareq ?PROTO_DAT :PROTO_DSV,nenc,false,0)>0)
			{
				tsent+=nenc; ndets-=nenc;
			}
			else
				ndets=0;
			if(ndets<1)
				needMoreEnd=false;
		}
		_ASSERT(ndets<1);
	}
	return 0;
}
void ViewServer::VSCNotifySqlIndexRequest(void *udata, DWORD rid, const char *select, const char *from, const char *where, int maxorders, bool unique, int istart, char idir, LONGLONG iter)
{
	return VSCNotifySqlIndexRequest2(udata,rid,select,from,where,maxorders,unique,istart,idir,iter,1);
}
// For EncodeDescribeRequest
void ViewServer::VSCNotifyDescribeRequest(void *udata, DWORD rid, const char *from, int maxrows, LONGLONG iter)
{
	_ASSERT((from)&&(*from));
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
	if(LOG_BROWSING)
	WSLogEvent("%s%d: NotifyDescribeRequest(rid=%d,from=%s,maxrows=%d,iter=%d)",
		PortTypeStr,PortNo,rid,from,maxrows,(int)iter);

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

	char rbuf[16380],*rptr=rbuf;
	VSQuery *pquery=0;
	// Override paging size with entitlement limit from users.ini
	int MAXBROWSE_LIMIT=(int)(PTRCAST)fuser->DetPtr2;
	if((maxrows<1)||((MAXBROWSE_LIMIT>0)&&(maxrows>MAXBROWSE_LIMIT)))
		maxrows=MAXBROWSE_LIMIT;
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
		pquery->select="describe";
		pquery->from=from;
		pquery->where="";
		pquery->maxorders=maxrows;
		pquery->hist=true;
		pquery->live=false;
		pquery->unique=true;
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

	// Virtual ORDERS table
	if(!_stricmp(from,"ORDERS"))
	{
		const char *desc[22 +AUX_KEYS_MAX_NUM]={
			"Column,Type,Size,FixTag,",
			"AppInstID,String,Var,11505,",
			"ClOrdID,String,Var,11,",
			"RootOrderID,Int,Var,70129,",
			"FirstClOrdID,String,Var,5055,",
			"Symbol,String,Var,55.65,",
			"Price,Double,8,44,",
			"Side,Char,1,54,",
			"Account,String,Var,1,",
			"EcnOrderID,String,Var,37,",
			"ClientID,String,Var,11,",
			"OrderQty,Int,Var,38,",
			"CumQty,Int,Var,14,",
			"FillQty,Int,Var,32,",
			"Term,Boolean,1,150,",
			"HighMsgType,Char,1,35,",
			"HighExecType,Char,1,150,",
			"TransactTime,TimeStamp,14,60,",
			"OrderLoc,String,Var,,",
			"Connection,String,Var,49.50.56.57,",
			"ClParentOrderID,String,Var,5035,",
		#ifdef MULTI_DAY
			"OrderDate,Int,Var,60,",
		#endif
			"AuxKey,String,Var,,"
			};
		int nenc=22;
		char adesc[AUX_KEYS_MAX_NUM][64];
		for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
		{
			desc[nenc +i]=0;
			memset(adesc[i],0,64); desc[nenc +i]=adesc[i];
		}
		if(odb.INDEX_AUXKEYS)
		{
			for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
			{
				if(!odb.AUXKEY_NAMES[i].empty())
				{
					if(odb.AUXKEY_TAGS[i])
					{
						sprintf(adesc[i],"%s,String,Var,%d,",odb.AUXKEY_NAMES[i].c_str(),odb.AUXKEY_TAGS[i]); nenc++;
					}
				#ifdef AUXKEY_EXPR
					else if(!odb.AUXKEY_ORD_EXPR[i].empty())
					{
						sprintf(adesc[i],"%s,String,Var,%s,",odb.AUXKEY_NAMES[i].c_str(),odb.AUXKEY_ORD_EXPR[i].c_str()); nenc++;
					}
					else if(!odb.AUXKEY_EXEC_EXPR[i].empty())
					{
						sprintf(adesc[i],"%s,String,Var,%s,",odb.AUXKEY_NAMES[i].c_str(),odb.AUXKEY_EXEC_EXPR[i].c_str()); nenc++;
					}
				#endif
				}
			}
		}
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,23,23,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,17,17,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual DETAILS table
	else if(!_stricmp(from,"DETAILS"))
	{
		// TODO: Union of all fix tags encountered
		_ASSERT(false); // unfinished describe DETAILS
		int nenc=0;
		char reason[256]={0};
		sprintf(reason,"Describe DETAILS not supported yet.");
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),-1,rid,0,0,0,nenc,false,0,reason)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,0,0,0,0,0,0,reason,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual DETAILS table
	else if(!_stricmp(from,"BTRDETAILS"))
	{
		// To read PDB for BTR: http://msdn.microsoft.com/en-us/library/eee38t3h.aspx
		_ASSERT(false); // unfinished describe BTRDETAILS
		int nenc=0;
		char reason[256]={0};
		sprintf(reason,"Describe BTRDETAILS not supported yet.");
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),-1,rid,0,0,0,nenc,false,0,reason)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,0,0,0,0,0,0,reason,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual INDICES table
	else if(!_stricmp(from,"INDICES"))
	{
		const char *desc[5]={
			"Column,Type,Size,Description,",
			"AppSystem,String,Var,Application system,",
			"Key,String,Var,Index key value,",
			"Position,Int,Var,Dynamic entry position within index,",
			"OrderLoc,String,Var,Hex location of order,"
			};
		int nenc=5;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,5,5,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual ACCOUNTS table
	else if(!_stricmp(from,"ACCOUNTS"))
	{
		const char *desc[8]={
			"Column,Type,Size,Description,",
			"AcctName,String,Var,System/instance/user account,",
			"AcctType,String,Var,AppSystem/AppInstance/custom type,",
			"AcctOrderQty,Int,Var,Placed order quantity,",
			"AcctCumQty,Int,Var,Reported fill quantity,",
			"AcctFillQty,Int,Var,Counted fill quantity,"
			"AcctUpdateTime,Int,Var,Last update time,"
			"AcctMsgCnt,Int,Var,Number of messages,"
			};
		int nenc=6;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,6,6,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
#ifdef IQDNS
	// Virtual USERS table
	else if(!_stricmp(from,"USERS"))
	{
		const char *desc[17]={
			"Column,Type,Size,Description,",
			// MSG1960REC attributes
			"UserDomain,String,Var,IQ domain,",
			"UserName,String,Var,IQ user,",
			"Pro,Boolean,1,,",
			"VarsIdSet,Boolean,1,,",
			"EnableNonProVARSID,Boolean,1,,",
			"PasswdLockOut,Boolean,1,,",
			"EnhancedSecurity,Boolean,1,,",
			"UserDocAckNeeded,Boolean,1,,",
			"UserAgreementNeeded,Boolean,1,,",
			"UserDeleted,Boolean,1,,",
			"FeedSuspended,Boolean,1,,",
			"VarsId,String,Var,,",
			"Citizenship1,String,Var,,",
			"Citizenship2,String,Var,,",
			"PasswordLifetime,String,Var,,",
			"PwdExpireDate,Int,Var,,"
			};
		int nenc=17;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,17,17,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual VARSID table
	else if(!_stricmp(from,"VARSID"))
	{
		const char *desc[24]={
			"Column,Type,Size,Description,",
			// VARSIDREC attributes
			"Company,String,Var,,",
			"Company2,String,Var,,",
			"Firm,String,Var,,",
			"TaxId,String,Var,,",
			"BrokerDealer,String,Var,,",
			"ShortName,String,Var,,",
			"FirstName,String,Var,,",
			"MiddleName,String,Var,,",
			"LastName,String,Var,,",
			"Address1,String,Var,,",
			"Address2,String,Var,,",
			"Address3,String,Var,,",
			"Address4,String,Var,,",
			"City,String,Var,,",
			"State,String,Var,,",
			"Zip,String,Var,,",
			"Country,String,Var,,",
			"Phone,String,Var,,",
			"FAX,String,Var,,",
			"Email,String,Var,,",
			"Created,String,Int,,",
			"Modified,String,Int,,",
			"ModifiedBy,String,Var,,"
			};
		int nenc=23;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,23,23,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual DOMREL tables
	else if(!_stricmp(from,"DOMREL"))
	{
		const char *desc[4]={
			"Column,Type,Size,Description,",
			"EntParent,String,Var,,",
			"Domain,String,Var,,",
			"RealParent,String,Var,,"
			};
		int nenc=4;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,4,4,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual DOMTREE table
	else if(!_stricmp(from,"DOMTREE"))
	{
		const char *desc[4]={
			"Column,Type,Size,Description,",
			"RealParent,String,Var,,",
			"Domain,String,Var,,",
			"EntParent,String,Var,,"
			};
		int nenc=4;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,4,4,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	// Virtual SERVERS table
	else if(!_stricmp(from,"SERVERS"))
	{
		const char *desc[12]={
			"Column,Type,Size,Description,",
			"Host,String,Var,,",
			"Port,String,Var,,",
			"Cpu,Int,4,,",
			"Memory,Int,4,,",
			"TypeServer,Int,4,,",
			"Nrusers,Int,4,,",
			"OS,Int,4,,",
			"Iqversion,Int,4,,",
			"Nropenquotes,Int,4,,",
			"Domain,String,Var,,",
			"Iqversion,Int,4,,"
			};
		int nenc=12;
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),0,rid,12,12,desc,nenc,false,0,0)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,5,5,nenc,false,0,0,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
#endif
	// Invalid from
	else
	{
		int nenc=0;
		char reason[256]={0};
		sprintf(reason,"Invalid from(%s) table.",from);
		if(vsc->EncodeDescribeReply(rptr,sizeof(rbuf),-1,rid,0,0,0,nenc,false,0,reason)>0)
		{
			if(PortType==WS_USR)
			{
				if(LOG_BROWSING)
				WSLogEvent("USR%d: EncodeDescribeReply(rc=%d,rid=%d,endcnt=%d,totcnt=%d,proto=%d,nfix=%d,more=%d,iter=%d,reason=%s) %d bytes",
					PortNo,-1,rid,0,0,0,0,0,0,reason,rptr -rbuf);
				WSUsrSendMsg(1,(WORD)(rptr -rbuf),rbuf,PortNo);
				ustats->msgCnt++;
			}
		}
		else
			_ASSERT(false);
		qmap->erase(qit);
		delete pquery; pquery=0;
		return;
	}
	return;
}
void ViewServer::VSCNotifyDescribeReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, const char **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	_ASSERT(false);
}

bool VSQuery::SetLiveQueryKey(CONDLIST& cl, string& keyStr, KEYTYPE& qt, KEYTYPE Type)
{
	if((Type==KEY_APPINSTID || Type==KEY_APPSYSTEM) && cl.size()==1)
	{
		ExprTok* et=cl.front();
		if(et && (!strcmp(et->oval,"=="))&&(et->nparams==2))
		{
			if(et->params[1]->vtype=='S')
			{
				keyStr=et->params[1]->sval;
				qt=Type;
				return true;
			}
		}
	}
	if(cl.size()==1 && qscope.GetEqStrVal(cl.front()))
	{
		keyStr=qscope.GetEqStrVal(cl.front());
		qt=Type;
		return true;
	}
	return false;
}

bool VSQuery::LiveQueryKey(string& keyStr, KEYTYPE& qt)
{
	if(SetLiveQueryKey(qscope.CondClOrdID,keyStr,qt,KEY_CLORDID))
		return true;
	else if(SetLiveQueryKey(qscope.CondClParentOrderID,keyStr,qt,KEY_CLPARENTORDERID))
		return true;
	else if(SetLiveQueryKey(qscope.CondAppInstID,keyStr,qt,KEY_APPINSTID))
		return true;
	else if(SetLiveQueryKey(qscope.CondAppSystem,keyStr,qt,KEY_APPSYSTEM))
		return true;
	else if(SetLiveQueryKey(qscope.CondSymbol,keyStr,qt,KEY_SYMBOL))
		return true;
	else if(SetLiveQueryKey(qscope.CondRootOrderID,keyStr,qt,KEY_ROOTORDERID))
		return true;
	else if(SetLiveQueryKey(qscope.CondFirstClOrdID,keyStr,qt,KEY_FIRSTCLORDID))
		return true;
	else if(SetLiveQueryKey(qscope.CondAccount,keyStr,qt,KEY_ACCOUNT))
		return true;
	else if(SetLiveQueryKey(qscope.CondEcnOrderID,keyStr,qt,KEY_ECNORDERID))
		return true;
	else if(SetLiveQueryKey(qscope.CondClientID,keyStr,qt,KEY_CLIENTID))
		return true;

	return false;
}

//DT9873
int ViewServer::SingleSearchCondtion(VSQueryScope& qscope,const char*& key, int& AuxKeyIndex)
{
	if(qscope.CondClOrdID.size()==1 && 
		!memcmp(qscope.CondClOrdID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondClOrdID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondClOrdID.front());
	#ifdef MULTI_DAY
		if(qscope.CondOrderDate.size()==1 && 
			(odb.INDEX_ORDERDATE) &&
			!memcmp(qscope.CondOrderDate.front()->oval,"==",2) &&
			qscope.GetEqIntVal(qscope.CondOrderDate.front())!=0)
		{
			sprintf(qscope.multikey,"%08d.%s",(int)qscope.GetEqIntVal(qscope.CondOrderDate.front()),key); key=qscope.multikey;
		}
	#endif
		return key?1:0; //ClOrdID
	}
	if(qscope.EqOrderLoc>=0)
		return 2;
	if(qscope.CondEcnOrderID.size()==1 && 
		!memcmp(qscope.CondEcnOrderID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondEcnOrderID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondEcnOrderID.front());
		return key?3:0; //EcnOrderID
	}
	if(qscope.CondFirstClOrdID.size()==1 && 
		!memcmp(qscope.CondFirstClOrdID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondFirstClOrdID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondFirstClOrdID.front());
		return key?4:0; //FirstClOrdID
	}
	if(qscope.CondClParentOrderID.size()==1 && 
		!memcmp(qscope.CondClParentOrderID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondClParentOrderID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondClParentOrderID.front());
		return key?5:0; //ClParentOrderID
	}
	if(qscope.CondRootOrderID.size()==1 && 
		!memcmp(qscope.CondRootOrderID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondRootOrderID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondRootOrderID.front());
		return key?6:0; //RootOrderID
	}
	if(qscope.CondClientID.size()==1 && 
		!memcmp(qscope.CondClientID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondClientID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondClientID.front());
		return key?7:0; //ClientID
	}
	if(qscope.CondSymbol.size()==1 && 
		!memcmp(qscope.CondSymbol.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondSymbol.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondSymbol.front());
		return key?8:0; //Symbol
	}
	if(qscope.CondConnection.size()==1 && 
		!memcmp(qscope.CondConnection.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondConnection.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondConnection.front());
		return key?9:0; //CondConnection
	}
	for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
	{
		if(qscope.CondAuxKeys[i].size()==1 && 
			!memcmp(qscope.CondAuxKeys[i].front()->oval,"==",2) &&
			qscope.GetEqStrVal(qscope.CondAuxKeys[i].front())!=0)
		{
			AuxKeyIndex=i;
			key=qscope.GetEqStrVal(qscope.CondAuxKeys[i].front());
			return key?10:0;
		}
	}
	if(qscope.CondOpenOrder.size()==1)
		return 11; // Open orders index
	if(qscope.CondFillQty.size()==1)
		return 12; // Filled orders index
	if(qscope.CondCumQty.size()==1)
		return 12; // Filled orders index
	if(qscope.CondAccount.size()==1 && 
		!memcmp(qscope.CondAccount.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondAccount.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondAccount.front());
		return key?13:0; //Account
	}
	if(qscope.CondTransactTime.size()==1 && 
		!memcmp(qscope.CondTransactTime.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondTransactTime.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondTransactTime.front());
		return key?14:0; //Transaction time
	}
#ifdef MULTI_DAY
	if(qscope.CondOrderDate.size()==1 &&
		(odb.INDEX_ORDERDATE) &&
		!memcmp(qscope.CondOrderDate.front()->oval,"==",2) &&
		qscope.GetEqIntVal(qscope.CondOrderDate.front())!=0)
	{
		//int kdate=(int)qscope.GetEqIntVal(qscope.CondOrderDate.front());
		key=qscope.CondOrderDate.front()->params[1]->exprBeg;
		return key?16:0; //OrderDate
	}
#endif
	if(qscope.CondAppInstID.size()==1 && 
		!memcmp(qscope.CondAppInstID.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondAppInstID.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondAppInstID.front());
		return key?15:0; //AppInstID
	}
	if(qscope.CondAppSystem.size()==1 && 
		!memcmp(qscope.CondAppSystem.front()->oval,"==",2) &&
		qscope.GetEqStrVal(qscope.CondAppSystem.front())!=0)
	{
		key=qscope.GetEqStrVal(qscope.CondAppSystem.front());
		return key?15:0; //AppSystem
	}
	return 0;
}

//DT9873: The change is a work-around for queries like where=ClOrdID!='A' AND Symbol=='B'
// Without this, the query keys off the ClOrdID (searching everything) instead of Symbol
// b/c ClOrdID is the higher priority.  
bool ViewServer::DetermineBestSearchCond(VSQuery* pquery, VSQueryScope& qscope, OrderDB *pdb)
{
	if(!pquery || !pdb)
		return false;

#ifdef MULTI_DAY
	// OrderDate index is always AND-ed with other conditions
	if(qscope.CondOrderDate.size()>0)
	{
		//pquery->tpos=(int)pquery->didx->size();
		// Single value, map search the key
		int EqOrderDate=(int)qscope.GetEqIntVal(qscope.CondOrderDate.front());
	#ifdef MULTI_DAY_HIST
		if(EqOrderDate>0)
		{
			OrderDB *idb=pquery->asys->GetDB(EqOrderDate);
			if(idb) 
			{
				idb->Lock();
				idb->DoDelayInit();
				pdb=idb;
				idb->Unlock();
			}
		}
	#endif
		// Narrow scope by subset of orders for the date
		if(pdb->INDEX_ORDERDATE)
		{
			if((EqOrderDate)&&(qscope.CondOrderDate.size()<2))
			{
				pquery->kdate=EqOrderDate;
				pquery->didx=&pdb->odmap;
				pquery->diit=pdb->odmap.find((int)pquery->kdate);
			}
			// Multiple values, we'll have to walk all keys
			else
			{
				pquery->kdate=0;
				pquery->didx=&pdb->odmap;
				pquery->diit=pdb->odmap.begin();
			}
			if(pquery->diit==pdb->odmap.end())
				pquery->morehist=false;
			#ifndef MULTI_DAY_ITEMLOC
			// This is kinda slow when we have hundreds of thousands of orders.
			// If the item location struct has the date in it, we don't need to join against a set of orders for the date. 
			// Add candidate orders to "join" set
			else if(pdb->HISTORIC_DAYS<=0)
			{
				for(DATEINDEX::iterator diit=pdb->odmap.find(pquery->kdate);
					(diit!=pdb->odmap.end())&&(diit->first==pquery->kdate);
					diit++)
					pquery->odset.insert(diit->second);
			}
			#endif
		}
		// There's no date index to join
		else
		{
			pquery->kdate=EqOrderDate;
		}
	}
#endif

	const char* key=0;
	int AuxKeyIndex=0;
	switch(SingleSearchCondtion(qscope,key,AuxKeyIndex))
	{
	case 1: //ClOrdID
		pquery->pidx=&pdb->omap;
		pquery->tpos=(int)pquery->pidx->size();
		pquery->kmatch=key;
		if(pdb->INDEX_CLORDID_CS)
			pquery->CaseSen=true;
		pquery->iit=pdb->omap.find(key);
		#ifdef MULTI_DAY
		// The lookup may fail due to locale date being different from GMT date,
		// so try the next or prev date
		if((pquery->iit==pdb->omap.end())&&(QueryTimeZoneBias!=0))
		{
			int condDate=CalcTPlusDate(atoi(key),(QueryTimeZoneBias>0)?+1:-1,false);
			char sCondDate[16]={0};
			sprintf(sCondDate,"%08d",condDate);
			char multikey[256];
			strcpy(multikey,qscope.multikey);
			memcpy(multikey,sCondDate,8);
			pquery->iit=pdb->omap.find(multikey);
			if(pquery->iit!=pdb->omap.end())
			{
				strcpy(qscope.multikey,multikey);
				pquery->kmatch=key=qscope.multikey;
			}
		}
		#endif
		if(pquery->iit==pdb->omap.end())
			pquery->morehist=false;
		return true;
	case 2: // Hex string order location
		pquery->koffset=qscope.EqOrderLoc;
		return true;
	case 3:  //ECNOrderID
		pquery->midx=&pdb->omap2;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_ECNORDERID_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->omap2.m_find(key);
		if(pquery->miit==pdb->omap2.m_end())
			pquery->morehist=false;
		return true;
	case 4: // FirstClOrdID index
		pquery->midx=&pdb->fmap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_FIRSTCLORDID_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->fmap.m_find(key);
		if(pquery->miit==pdb->fmap.m_end())
			pquery->morehist=false;
		return true;
	case 5:// ClParentOrderID index
		pquery->midx=&pdb->cpmap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_CLPARENTORDERID_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->cpmap.m_find(key);
		if(pquery->miit==pdb->cpmap.m_end())
			pquery->morehist=false;
		return true;
	case 6: // RootOrderID index
		pquery->midx=&pdb->pmap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_ROOTORDERID_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->pmap.m_find(key);
			if(pquery->miit==pdb->pmap.m_end())
				pquery->morehist=false;
		return true;
	case 7: // ClientID index
		pquery->midx=&pdb->cimap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_CLIENTID_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->cimap.m_find(key);
		if(pquery->miit==pdb->cimap.m_end())
			pquery->morehist=false;
		return true;
	case 8: // Symbol index
		pquery->midx=&pdb->symmap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_SYMBOL_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->symmap.m_find(key);
		if(pquery->miit==pdb->symmap.m_end())
			pquery->morehist=false;
		return true;
	case 9: //Connection
		pquery->midx=&pdb->cnmap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_CONNECTIONS_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->cnmap.m_find(key);
		if(pquery->miit==pdb->cnmap.m_end())
			pquery->morehist=false;
		return true;
	case 10: // Auxkey
		pquery->midx=&pdb->akmap[AuxKeyIndex];
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_AUXKEYS_CS)
			pquery->CaseSen=true;
		pquery->miit=pdb->akmap[AuxKeyIndex].m_find(key);
		if(pquery->miit==pdb->akmap[AuxKeyIndex].m_end())
			pquery->morehist=false;
		return true;
	case 11: // OpenOrder
		pquery->pidx=&pdb->oomap;
		pquery->tpos=(int)pquery->pidx->size();
		pquery->iit=pdb->oomap.begin();
		if(pquery->iit==pdb->oomap.end())
			pquery->morehist=false;
		return true;
	case 12: // Filled orders index
		pquery->pidx=&pdb->fomap;
		pquery->tpos=(int)pquery->pidx->size();
		pquery->iit=pdb->fomap.begin();
		if(pquery->iit==pdb->fomap.end())
			pquery->morehist=false;
		return true;
	case 13: // Account index 
		pquery->midx=&pdb->acctmap;
		pquery->tpos=(int)pquery->midx->m_size();
		pquery->kmatch=key;
		if(pdb->INDEX_ACCOUNT_CS)  //DT9491
			pquery->CaseSen=true;
		pquery->miit=pdb->acctmap.m_find(key);
		if(pquery->miit==pdb->acctmap.m_end())
			pquery->morehist=false;
		return true;
	case 14: // TransactTime index
		pquery->tidx=&pdb->tsmap;
		pquery->tpos=(int)pquery->tidx->m_size();
		pquery->kts=myatoi64(key);
		pquery->tiit=pdb->tsmap.m_find(pquery->kts);
		if(pquery->tiit==pdb->tsmap.m_end())
			pquery->morehist=false;
		return true;
	case 15: // AppInstance/AppSystem
		// All instances in the system
		if((qscope.CondAppSystem.empty())&&(qscope.CondAppInstID.empty()))
		{
			pquery->ailist.clear();
 			for(APPINSTMAP::iterator iit=pquery->asys->imap.begin();iit!=pquery->asys->imap.end();iit++)
				pquery->ailist.push_back(iit->second);
		}
		pquery->aiit=pquery->ailist.begin();
		if(pquery->aiit==pquery->ailist.end())
			pquery->morehist=false;
		else
		{
			pquery->ainst=*pquery->aiit;
			pquery->aoit=(*pquery->aiit)->oset.begin();
		}
		pquery->tpos=ocnt; // worst case (TODO: track counts of orders in each AppSystem)
		return true;
#ifdef MULTI_DAY
	case 16: // OrderDate index
		pquery->didx=&pdb->odmap;
		pquery->tpos=(int)pquery->didx->size();
		pquery->kts=myatoi64(key);
		pquery->diit=pdb->odmap.find((int)pquery->kts);
		if(pquery->diit==pdb->odmap.end())
			pquery->morehist=false;
		return true;
#endif
	case 0:
	default:
		break;
	}
	return false;
}

