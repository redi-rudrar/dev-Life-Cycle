
#include "stdafx.h"
#include "vsdefs.h"
#include "sfix.h"

DropClient::DropClient()
	:pnotify(0)
	,udata(0)
	,scid()
	,ssid()
	,tcid()
	,tsid()
	,loginTime(0)
	,iseq(0)
	,oseq(0)
	,passive(false)
	,ihbinterval(30000)
	,ihblast(0)
	,ohbinterval(30000)
	,ohblast(0)
	,msgq()
	,dirty(false)
	,inlist()
	,waitgapfill(0)
	,mux()
	,replayMap()
{
}
DropClient::~DropClient()
{
}
int DropClient::Init(const char *sids, int iseq, int oseq, bool passive, DropClientNotify *pnotify, void *udata)
{
	if(!pnotify)
		return -1;
	if((!sids)||(!*sids)||(!pnotify))
		return -1;
	char scpy[80]={0};
	strncpy(scpy,sids,sizeof(scpy)-1); scpy[sizeof(scpy)-1]=0;
	const char *t49=strtoke(scpy,",");
	if(!t49) return -1;
	const char *t50=strtoke(0,",");
	if(!t50) return -1;
	const char *t56=strtoke(0,",");
	if(!t56) return -1;
	const char *t57=strtoke(0,",");
	if(!t57) t57="";
	scid=t49; ssid=t50; tcid=t56; tsid=t57;

	InitializeCriticalSection(&mux);
	this->pnotify=pnotify;
	this->udata=udata;
	this->iseq=iseq;
	this->oseq=oseq;
	this->passive=passive;
	return 0;
}
void DropClient::Shutdown()
{
	if(pnotify)
		DeleteCriticalSection(&mux);
	pnotify=0;
}
// Must be protected by critical section
int DropClient::EncodeFixMsg(char *&rptr, int rlen, FIXINFO& ifx)
{
	ifx.SetTag(8,"FIX.4.2");
	int mseq=ifx.TagInt(34);
	if(mseq>0)
	{
		if(mseq>oseq)
			oseq=mseq;
		else
			ifx.SetTag(43,'Y');
	}
	else
		ifx.SetTag(34,++oseq);
	if(!scid.empty())
		ifx.SetTag(49,scid.c_str());
	if(!ssid.empty())
		ifx.SetTag(50,ssid.c_str());
	if(!tcid.empty())
		ifx.SetTag(56,tcid.c_str());
	if(!tsid.empty())
		ifx.SetTag(57,tsid.c_str());
	if(!ifx.GetTag(52))
	{
		char tstr[32]={0};
	#ifdef WIN32
		SYSTEMTIME tsys;
		GetSystemTime(&tsys);
		sprintf(tstr,"%04d%02d%02d-%02d:%02d:%02d",
			tsys.wYear,tsys.wMonth,tsys.wDay,tsys.wHour,tsys.wMinute,tsys.wSecond);
	#else
		int wst=WSTime();
		sprintf(tstr,"%08d-%02d:%02d:%02d",WSDate(),wst/10000,(wst%10000)/100,wst%100);
	#endif
		ifx.SetTag(52,tstr);
	}
	if((!ifx.GetTag(98))&&(ifx.TagChar(35)=='A'))
		ifx.SetTag(98,0);
	ifx.Merge();

	if(ifx.llen>rlen)
		return -1;
	const char *rbeg=rptr;
	memcpy(rptr,ifx.fbuf,ifx.llen); rptr+=ifx.llen;
#ifdef _DEBUG
	char dbuf[1024]={0};
	strcpy(dbuf,"O:");
	memcpy(dbuf+2,ifx.fbuf,ifx.llen); strcpy(dbuf+2+ifx.llen,"\r\n");
	OutputDebugString(dbuf);
#endif
	return 0;
}
int DropClient::SendFixMsg(FIXINFO& ifx)
{
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	LONGLONG woff=pnotify->DCWriteFix(udata,'O',&ifx);
	if((ifx.TagChar(35)=='8')&&(mytoupper(ifx.TagChar(43))!='Y')&&(mytoupper(ifx.TagChar(97))!='Y'))
		replayMap[ifx.TagInt(34)]=woff;
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::Login(int hbsec)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	ifx.SetTag(35,'A');
	ifx.SetTag(108,hbsec);
	ihbinterval=hbsec*1000; 
	ohblast=GetTickCount();
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	pnotify->DCWriteFix(udata,'O',&ifx);
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::Logout(const char *text)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	ifx.SetTag(35,'5');
	ifx.SetTag(40,text);
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	pnotify->DCWriteFix(udata,'O',&ifx);
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::Heartbeat(const char *text)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	ifx.SetTag(35,'0');
	if((text)&&(*text))
		ifx.SetTag(112,text);
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	pnotify->DCWriteFix(udata,'O',&ifx);
	ohblast=GetTickCount();
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::ResendReq(int start, int stop)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	ifx.SetTag(35,'2');
	ifx.SetTag(7,start);
	ifx.SetTag(16,stop);
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	pnotify->DCWriteFix(udata,'O',&ifx);
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::ResendReply(int start, int stop)
{
	// Scan for last sequence numbers and provide "newinlog" functionality
	FIXINFO ifx;
	memset(&ifx,0,sizeof(FIXINFO));
	ifx.FIXDELIM=0x01;
	ifx.sbuf=new char[2048];
	memset(ifx.sbuf,0,2048);
	ifx.ssize=2048;
	char fbuf[1024]={0};
	int gstart=0,seq=0;
	if(stop==0)
		stop=oseq;
	for(seq=start;seq<=stop;seq++)
	{
		ifx.slen=0; ifx.Reset();
		char mdir=0;
		LONGLONG roff=-1;
		map<int,LONGLONG>::iterator rit=replayMap.find(seq);
		if(rit!=replayMap.end())
		{
			roff=rit->second;
			LONGLONG noff=pnotify->DCReadFix(udata,roff,mdir,&ifx);
			if(ifx.llen<1)
				mdir=0;
		}
		if(mdir=='O')
		{
			// Fill any open gaps
			if(gstart>0)
			{
				GapFill(gstart,seq); gstart=0;
			}
			ifx.SetTag(43,'Y'); //PossDupFlag
			ifx.SetTag(97,'Y'); //PossResend
			ifx.DelTag(52); // Will be replaced with current timestamp
			// FIXINFO::Merge requires a fresh 'sbuf' to write the new FIX string
			char *dbuf=ifx.sbuf;
			ifx.sbuf=new char[2048];
			memset(ifx.sbuf,0,2048);

			EnterCriticalSection(&mux);
			int fsize=sizeof(fbuf);
			char *fptr=fbuf;
			if(EncodeFixMsg(fptr,fsize,ifx)<0)
			{
				delete dbuf;
				LeaveCriticalSection(&mux);
				return -1;
			}
			delete dbuf;
			pnotify->DCWriteFix(udata,'O',&ifx);
			dirty=true;
			LeaveCriticalSection(&mux);
			pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
		}
		else
		{
			if(gstart<=0)
				gstart=seq;
		}
	}
	// Fill any remaining gaps
	if(gstart>0)
	{
		GapFill(gstart,seq); gstart=0;
	}
	return 0;
}
int DropClient::GapFill(int start, int stop)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	ifx.SetTag(35,'4');
	ifx.SetTag(34,start);
	if(stop>oseq)
		stop=oseq;
	else if(stop<1)
		stop=oseq;
	ifx.SetTag(36,stop+1);
	ifx.SetTag(123,'Y');
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	pnotify->DCWriteFix(udata,'O',&ifx);
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::SeqReset(int nstart)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	ifx.SetTag(35,'4');
	ifx.SetTag(123,'N');
	ifx.SetTag(36,nstart);
	EnterCriticalSection(&mux);
	char fbuf[1024]={0};
	int fsize=sizeof(fbuf);
	char *fptr=fbuf;
	if(EncodeFixMsg(fptr,fsize,ifx)<0)
	{
		LeaveCriticalSection(&mux);
		return -1;
	}
	pnotify->DCWriteFix(udata,'O',&ifx);
	dirty=true;
	LeaveCriticalSection(&mux);
	pnotify->DCSendMsg(udata,fbuf,(int)(fptr -fbuf));
	return 0;
}
int DropClient::DecodeFix(const char *sfix, int slen)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	int flen=ifx.FixMsgReady((char*)sfix,slen);
	if(flen<1)
		return flen;
#ifdef _DEBUG
	char dbuf[1024]={0};
	strcpy(dbuf,"I:");
	memcpy(dbuf+2,sfix,flen); strcpy(dbuf+2+flen,"\r\n");
	OutputDebugString(dbuf);
#endif

	pnotify->DCWriteFix(udata,'I',&ifx);
	char mtype=ifx.TagChar(35);
	if(passive)
	{
		iseq=ifx.TagInt(34);
		dirty=true;
	}
	else
	{
		int mseq=ifx.TagInt(34);
		bool holdmsg=false;
		TrackInCode rc=TrackIn(mseq);
		if(rc==TRKI_NEW_GAP)
		{
			// Ask for earliest gap fill
			INBLOCK gap;
			if(GetGap(gap,0)>0)
			{
				if(ResendReq(gap.begin,gap.end -1)<0)
					return -1;
				waitgapfill=GetTickCount();
			}
			holdmsg=true;
		}
		// Ask for next gap fill
		else if(rc==TRKI_GAP_CLOSED)
		{
			waitgapfill=0;
			INBLOCK gap;
			if(GetGap(gap,0)>0)
			{
				if(ResendReq(gap.begin,gap.end -1)<0)
					return -1;
				waitgapfill=GetTickCount();
			}
		}
		// Restart the gap fill timer
		else if(rc==TRKI_REPLAYED)
		{
			if(waitgapfill)
				waitgapfill=GetTickCount();
		}
		else if(rc==TRKI_IN_SEQ)
		{
			iseq=mseq;
			// Check gap fill request timeout
			if(waitgapfill>0)
			{
				DWORD tnow=GetTickCount();
				if(tnow -waitgapfill>=10000)
				{
					waitgapfill=0;
					INBLOCK gap;
					if(GetGap(gap,0)>0)
					{
						if(ResendReq(gap.begin,gap.end -1)<0)
							return -1;
						waitgapfill=GetTickCount();
					}
				}
				else
					holdmsg=true;
			}
		}

		if(holdmsg)
		{
			switch(mtype)
			{
			case '0':
			case '1':
			case '2':
			case '4':
			case '5':
			case 'A':
				// Don't block session messages
				break;
			default:
			{
				// Copy the message and wait till we filled the gap before reporting this one
				char *scpy=new char[slen+1];
				memcpy(scpy,sfix,slen); scpy[slen]=0;
				msgq.push_back(pair<int,char*>(mseq,scpy));
				return flen;
			}
			};
		}
		dirty=true;
	}

	// Process the message
	switch(mtype)
	{
	// Heartbeat
	case '0':
	{
		// tag(112) may have test request string
		//pnotify->DCWriteFix(udata,'I',&ifx);
		ihblast=GetTickCount();
		break;
	}
	// Test
	case '1': 
	{
		//pnotify->DCWriteFix(udata,'I',&ifx);
		Heartbeat(ifx.TagStr(112));
		break;
	}
	// Resend request
	case '2': 
	{
		//pnotify->DCWriteFix(udata,'I',&ifx);
		//GapFill(ifx.TagInt(7),ifx.TagInt(16));
		ResendReply(ifx.TagInt(7),ifx.TagInt(16));
		break;
	}
	// Gap fill or seq reset
	case '4': 
	{
		// Gap fill
		int bgap=ifx.TagInt(34);
		int egap=ifx.TagInt(36);
		char gapfill=ifx.TagChar(123);
		if(egap<1)
			egap=iseq +1;
		if(iseq<egap -1)
			iseq=egap -1;
		if(!passive)
		{
			// Sequence reset
			if(toupper(gapfill)!='Y')
				bgap=1;
			// Act like we got every message in the range
			int gclosed=0;
			for(int fseq=bgap;fseq<egap;fseq++)
			{
				TrackInCode rc=TrackIn(fseq);
				if(rc==TRKI_GAP_CLOSED)
				{
					waitgapfill=0; gclosed++;
				}
			}
			if(gclosed>0)
			{
				INBLOCK gap;
				if(GetGap(gap,0)>0)
				{
					if(ResendReq(gap.begin,gap.end -1)<0)
						return -1;
					waitgapfill=GetTickCount();
				}
			}
		}
		//pnotify->DCWriteFix(udata,'I',&ifx);
		break;
	}
	case '5': 
	{
		//pnotify->DCWriteFix(udata,'I',&ifx);
		pnotify->DCLoggedOut(udata,ifx.TagStr(58)); 
		loginTime=0;
		break;
	}
	case 'A': 
	{
	#ifdef WIN32
		SYSTEMTIME tsys;
		GetLocalTime(&tsys);
		loginTime=(tsys.wHour*10000) +(tsys.wMinute*100) +(tsys.wSecond);
	#else
		loginTime=::WSTime();
	#endif
		ohbinterval=ifx.TagInt(108)*1000;
		pnotify->DCLoggedIn(udata,ohbinterval); 
		//pnotify->DCWriteFix(udata,'I',&ifx);
		// Request first gap fill after login success 
		// iif we haven't already requested one from login response seqno 
		if(!waitgapfill)
		{
			DropClient::INBLOCK gap;
			if(GetGap(gap,0)>0)
			{
				if(ResendReq(gap.begin,gap.end -1)<0)
					return -1;
				waitgapfill=GetTickCount();
			}
		}
		break;
	}
	//case 'D': 
	//case 'F': 
	//case 'G':
	//case '3': 
	//case '8':
	//case '9': 
	//case 'Q':
	//default: 
	//	pnotify->DCWriteFix(udata,'I',&ifx);
	};
	if(!passive)
	{
		// Resolved any gaps?
		while((!msgq.empty())&&(msgq.front().first<=iseq+1))
		{
			QMSG& qmsg=msgq.front();
			char *scpy=qmsg.second;
			msgq.pop_front();
			DecodeFix(scpy,(int)strlen(scpy));
			delete scpy;
		}
	}
	return flen;
}
string DropClient::GetConnID()
{
	char cstr[80]={0};
	sprintf(cstr,"%s.%s.%s.%s",scid.c_str(),ssid.c_str(),tcid.c_str(),tsid.c_str());
	return (string)cstr;
}
int DropClient::GetLastSeqNos(const char *fpath)
{
	if(!pnotify)
		return -1;
	LONGLONG roff=0,noff=0;
	FIXINFO ifx;
	memset(&ifx,0,sizeof(FIXINFO));
	ifx.FIXDELIM=0x01;
	ifx.sbuf=new char[2048];
	memset(ifx.sbuf,0,2048);
	ifx.ssize=2048;
	char mdir=0;
	while((noff=pnotify->DCReadFix(udata,roff,mdir,&ifx))>0)
	{
		// Map replayable messages and increment roff
		int t34=ifx.TagInt(34);
		string t49=ifx.TagStr(49),t56=ifx.TagStr(56);
		if(t34)
		{
			if(mdir=='O')
			{
				if(t34>oseq)
					oseq=t34;
				// Map orignal execution report locations for resend requests
				char t35=ifx.TagChar(35);
				if((t35=='8')&&(mytoupper(ifx.TagChar(43))!='Y')&&(mytoupper(ifx.TagChar(97))!='Y'))
					replayMap[t34]=roff;
			}
			else if(mdir=='I')
			{
				TrackIn(t34);
				if(t34>iseq)
					iseq=t34;
				char t35=ifx.TagChar(35);
				if(t35=='4')
				{
					int bgap=ifx.TagInt(34);
					int egap=ifx.TagInt(36);
					if(egap<1)
						egap=iseq +1;
					// Act like we got every message in the range
					for(int fseq=bgap;fseq<egap;fseq++)
						TrackIn(fseq);
				}
			}
		}
		if(noff>roff)
			roff=noff;
		else
			break;
	}
	return 0;
}
int DropClient::ScanLastSeqNos(const char *fpath)
{
	// Scan for last sequence numbers and provide "newinlog" functionality
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
		return -1;
	FIXINFO ifix;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		const char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		const char *rend=rbuf +strlen(rbuf);
        char *eptr=(char*)strendl(rptr,rend);
		if(eptr) *eptr=0;
		if(!*rptr)
			continue;
		char mdir=rptr[0];
		for(int d=0;d<3;d++)
		{
			const char *nptr=strechr(rptr,':',rptr +32);
			if(!nptr)
				break;
			rptr=nptr+1;
		}
		ifix.Reset();
		ifix.FIXDELIM=0x01;
		ifix.noSession=true;
		ifix.supressChkErr=true;
		if(ifix.FixMsgReady((char*)rptr,(int)(rend -rptr))>0)
		{
			int t34=ifix.TagInt(34);
			string t49=ifix.TagStr(49),t56=ifix.TagStr(56);
			if(t34)
			{
				//if((t49==pdc->scid)&&(t56==pdc->tcid))
				//{
				//	if(t34>pdc->oseq)
				//		pdc->oseq=t34;
				//}
				//else if((t49==pdc->tcid)&&(t56==pdc->scid))
				//{
				//	if(t34>pdc->iseq)
				//		pdc->iseq=t34;
				//}
				if(mdir=='O')
				{
					if(t34>oseq)
						oseq=t34;
				}
				else if(mdir=='I')
				{
					TrackIn(t34);
					if(t34>iseq)
						iseq=t34;
					char t35=ifix.TagChar(35);
					if(t35=='4')
					{
						int bgap=ifix.TagInt(34);
						int egap=ifix.TagInt(36);
						if(egap<1)
							egap=iseq +1;
						// Act like we got every message in the range
						for(int fseq=bgap;fseq<egap;fseq++)
							TrackIn(fseq);
					}
				}
			}
		}
	}
	fclose(fp);
	return 0;
}// Traks incoming message blocks
DropClient::TrackInCode DropClient::TrackIn(int mseq)
{
	TrackInCode rc=TRKI_UNKNOWN;
	INBLOCK *prev=0;
	for(INLIST::iterator iit=inlist.begin();iit!=inlist.end();iit++)
	{
		INBLOCK& ib=*iit;
		// Extended block at end (normal)
		if(mseq==ib.end)
		{
			// Did that close a gap?
			iit++;
			if(iit!=inlist.end())
			{
				INBLOCK& nb=*iit;
				// Don't allow two blocks to overlap
				if(ib.end<nb.begin)
				{
					ib.end++;
					if(ib.end>=nb.begin)
						return TRKI_GAP_CLOSED;
				}
				return TRKI_REPLAYED;
			}
			ib.end++;
			return TRKI_IN_SEQ;
		}
		// Replayed in the middle of a block
		else if((mseq>=ib.begin)&&(mseq<ib.end))
			return TRKI_REPLAYED;
		// Extended block at beginning (unusual)
		else if(mseq==ib.begin -1)
		{
			ib.begin--;
			// Did that close a gap?
			if((ib.begin==1)||((prev)&&(prev->end>=ib.begin)))
				return TRKI_GAP_CLOSED;
			return TRKI_REPLAYED;
		}
		// New block in the middle
		else if(mseq<ib.begin)
		{
			INBLOCK nb;
			nb.begin=mseq;
			nb.end=mseq +1;
			iit=inlist.insert(iit,nb);
			return TRKI_REPLAYED;//TRKI_NEW_GAP;??
		}
		prev=&ib;
	}
	// New block at end
	INBLOCK nb;
	nb.begin=mseq;
	nb.end=mseq +1;
	inlist.push_back(nb);
	return mseq==1 ?TRKI_IN_SEQ :TRKI_NEW_GAP;
}
// Search for message gaps between blocks
int DropClient::GetGap(INBLOCK& gap, int skip)
{
	gap.begin=gap.end=0;
	int lseq=0;
	for(INLIST::iterator iit=inlist.begin();iit!=inlist.end();iit++)
	{
		INBLOCK& ib=*iit;
		if(ib.begin>lseq+1)
		{
			if(skip>0)
				skip--;
			else
			{
				gap.begin=lseq +1;
				gap.end=ib.begin;
				return 1;
			}
		}
		lseq=ib.end -1;
	}
	return 0;
}

#ifdef FIX_SERVER
DropServer::DropServer()
{
}
int DropServer::EncodeFixMsg(char *&rptr, int rlen, FIXINFO& ifx)
{
	ifx.SetTag(8,"FIX.4.2");
	int mseq=ifx.TagInt(34);
	if(mseq>0)
	{
		if(mseq>oseq)
			oseq=mseq;
		else
			ifx.SetTag(43,'Y');
	}
	else
		ifx.SetTag(34,++oseq);
	// Reversed from DropClient
	if(!scid.empty())
		ifx.SetTag(56,scid.c_str());
	if(!ssid.empty())
		ifx.SetTag(57,ssid.c_str());
	if(!tcid.empty())
		ifx.SetTag(49,tcid.c_str());
	if(!tsid.empty())
		ifx.SetTag(50,tsid.c_str());
	if(!ifx.GetTag(52))
	{
		char tstr[32]={0};
	#ifdef WIN32
		SYSTEMTIME tsys;
		GetSystemTime(&tsys);
		sprintf(tstr,"%04d%02d%02d-%02d:%02d:%02d",
			tsys.wYear,tsys.wMonth,tsys.wDay,tsys.wHour,tsys.wMinute,tsys.wSecond);
	#else
		int wst=WSTime();
		sprintf(tstr,"%08d-%02d:%02d:%02d",WSDate(),wst/10000,(wst%10000)/100,wst%100);
	#endif
		ifx.SetTag(52,tstr);
	}
	ifx.SetTag(98,0);
	ifx.Merge();

	if(ifx.llen>rlen)
		return -1;
	const char *rbeg=rptr;
	memcpy(rptr,ifx.fbuf,ifx.llen); rptr+=ifx.llen;
#ifdef _DEBUG
	char dbuf[1024]={0};
	strcpy(dbuf,"O:");
	memcpy(dbuf+2,ifx.fbuf,ifx.llen); strcpy(dbuf+2+ifx.llen,"\r\n");
	OutputDebugString(dbuf);
#endif
	return 0;
}
int DropServer::DecodeFix(const char *sfix, int slen)
{
	if(!pnotify)
		return -1;
	FIXINFO ifx;
	ifx.Reset();
	ifx.noSession=false;
	ifx.supressChkErr=true;
	ifx.FIXDELIM=0x01;
	int flen=ifx.FixMsgReady((char*)sfix,slen);
	if(flen<1)
		return flen;
#ifdef _DEBUG
	char dbuf[1024]={0};
	strcpy(dbuf,"I:");
	memcpy(dbuf+2,sfix,flen); strcpy(dbuf+2+flen,"\r\n");
	OutputDebugString(dbuf);
#endif

	pnotify->DCWriteFix(udata,'I',&ifx);
	char mtype=ifx.TagChar(35);
	if(passive)
	{
		iseq=ifx.TagInt(34);
		dirty=true;
	}
	else
	{
		int mseq=ifx.TagInt(34);
		bool holdmsg=false;
		TrackInCode rc=TrackIn(mseq);
		if(rc==TRKI_NEW_GAP)
		{
			// Ask for earliest gap fill
			INBLOCK gap;
			if(GetGap(gap,0)>0)
			{
				if(ResendReq(gap.begin,gap.end -1)<0)
					return -1;
				waitgapfill=GetTickCount();
			}
			holdmsg=true;
		}
		// Ask for next gap fill
		else if(rc==TRKI_GAP_CLOSED)
		{
			waitgapfill=0;
			INBLOCK gap;
			if(GetGap(gap,0)>0)
			{
				if(ResendReq(gap.begin,gap.end -1)<0)
					return -1;
				waitgapfill=GetTickCount();
			}
		}
		// Restart the gap fill timer
		else if(rc==TRKI_REPLAYED)
		{
			if(waitgapfill)
				waitgapfill=GetTickCount();
		}
		else if(rc==TRKI_IN_SEQ)
		{
			iseq=mseq;
			// Check gap fill request timeout
			if(waitgapfill>0)
			{
				DWORD tnow=GetTickCount();
				if(tnow -waitgapfill>=10000)
				{
					waitgapfill=0;
					INBLOCK gap;
					if(GetGap(gap,0)>0)
					{
						if(ResendReq(gap.begin,gap.end -1)<0)
							return -1;
						waitgapfill=GetTickCount();
					}
				}
				else
					holdmsg=true;
			}
		}

		if(holdmsg)
		{
			switch(mtype)
			{
			case '0':
			case '1':
			case '2':
			case '4':
			case '5':
			case 'A':
				// Don't block session messages
				break;
			default:
			{
				// Copy the message and wait till we filled the gap before reporting this one
				char *scpy=new char[slen+1];
				memcpy(scpy,sfix,slen); scpy[slen]=0;
				msgq.push_back(pair<int,char*>(mseq,scpy));
				return flen;
			}
			};
		}
		dirty=true;
	}

	// Process the message
	switch(mtype)
	{
	// Heartbeat
	case '0':
	{
		// tag(112) may have test request string
		//pnotify->DCWriteFix(udata,'I',&ifx);
		ihblast=GetTickCount();
		break;
	}
	// Test
	case '1': 
	{
		//pnotify->DCWriteFix(udata,'I',&ifx);
		Heartbeat(ifx.TagStr(112));
		break;
	}
	// Resend request
	case '2': 
	{
		//pnotify->DCWriteFix(udata,'I',&ifx);
		//GapFill(ifx.TagInt(7),ifx.TagInt(16));
		ResendReply(ifx.TagInt(7),ifx.TagInt(16));
		break;
	}
	// Gap fill or seq reset
	case '4': 
	{
		// Gap fill
		int bgap=ifx.TagInt(34);
		int egap=ifx.TagInt(36);
		char gapfill=ifx.TagChar(123);
		if(egap<1)
			egap=iseq +1;
		if(iseq<egap -1)
			iseq=egap -1;
		if(!passive)
		{
			// Sequence reset
			if(toupper(gapfill)!='Y')
				bgap=1;
			// Act like we got every message in the range
			int gclosed=0;
			for(int fseq=bgap;fseq<egap;fseq++)
			{
				TrackInCode rc=TrackIn(fseq);
				if(rc==TRKI_GAP_CLOSED)
				{
					waitgapfill=0; gclosed++;
				}
			}
			if(gclosed>0)
			{
				INBLOCK gap;
				if(GetGap(gap,0)>0)
				{
					if(ResendReq(gap.begin,gap.end -1)<0)
						return -1;
					waitgapfill=GetTickCount();
				}
			}
		}
		//pnotify->DCWriteFix(udata,'I',&ifx);
		break;
	}
	case '5': 
	{
		//pnotify->DCWriteFix(udata,'I',&ifx);
		pnotify->DCLoggedOut(udata,ifx.TagStr(58)); 
		loginTime=0;
		break;
	}
	case 'A': 
	{
	#ifdef WIN32
		SYSTEMTIME tsys;
		GetLocalTime(&tsys);
		loginTime=(tsys.wHour*10000) +(tsys.wMinute*100) +(tsys.wSecond);
	#else
		loginTime=::WSTime();
	#endif
		ohbinterval=ifx.TagInt(108)*1000;
		const char *t49=ifx.TagStr(49),*t56=ifx.TagStr(56);
		//pnotify->DCWriteFix(udata,'I',&ifx);
		// Login successful
		if((!stricmp(scid.c_str(),t49))&&(!stricmp(tcid.c_str(),t56)))
		{
			Login(ifx.TagInt(108));
			pnotify->DCLoggedIn(udata,ohbinterval); 
			// Request first gap fill after login success 
			// iif we haven't already requested one from login response seqno 
			if(!waitgapfill)
			{
				DropServer::INBLOCK gap;
				if(GetGap(gap,0)>0)
				{
					if(ResendReq(gap.begin,gap.end -1)<0)
						return -1;
					waitgapfill=GetTickCount();
				}
			}
		}
		// Failed login
		else
		{
			Logout("Not authorized");
		}
		break;
	}
	//case 'D': 
	//case 'F': 
	//case 'G':
	//case '3': 
	//case '8':
	//case '9': 
	//case 'Q':
	//default: 
	//	pnotify->DCWriteFix(udata,'I',&ifx);
	};
	if(!passive)
	{
		// Resolved any gaps?
		while((!msgq.empty())&&(msgq.front().first<=iseq+1))
		{
			QMSG& qmsg=msgq.front();
			char *scpy=qmsg.second;
			msgq.pop_front();
			DecodeFix(scpy,(int)strlen(scpy));
			delete scpy;
		}
	}
	return flen;
}
#endif
