
#include "stdafx.h"

#include <stdio.h>
#ifdef _WIN32
#include <crtdbg.h>
#endif
#include "bfix.h"
#ifdef USE_MYRT
#include "myrt.h"
#else
#define myatoi atoi
#define myisdigit isdigit
#endif

static void WSLogError(const char *fmt,...)
{
	va_list alist;
	va_start(alist,fmt);
	char estr[256]={0};
	vsprintf_s(estr,sizeof(estr),fmt,alist);
	va_end(alist);
	OutputDebugString(estr);
	OutputDebugString("\n");
}

#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_1_2)
FIXINFO::FIXINFO()
{
	fci=0;
	fbuf=0;
	ntags=0;
	llen=0;
	chksum=0;
	supressChkErr=1;
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	maxtags=0;
	tags=0;
	sbuf=0;
	slen=ssize=0;
#else
	memset(tags,0,MAXTAGS*sizeof(FIXTAG));
#endif

	FIXDELIM=0x01;
	FIXDELIM2=0;
	memset(TagStrStr,0,sizeof(TagStrStr));
	TagStrIdx=-1;
}
FIXINFO::~FIXINFO()
{
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	if(tags)
	{
		//for(int i=0;i<ntags;i++)
		//{
		//	if(tags[i].name)
		//		delete tags[i].name;
		//}
		delete tags; tags=0;
		maxtags=0;
	}
	if(sbuf)
	{
		delete sbuf; sbuf=0;
	}
#endif
}

FIXTAG *FIXINFO::AddTag(int tag, const char *val, int tstart, int vlen)
{
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	if(ntags>=maxtags)
	{
		int nmax=maxtags;
		if(nmax>0)
			nmax*=2;
		else
			nmax=32;
		FIXTAG *ntags=new FIXTAG[nmax];
		memset(ntags,0,nmax*sizeof(FIXTAG));
		memcpy(ntags,tags,maxtags*sizeof(FIXTAG));
		delete tags;
		tags=ntags;
		maxtags=nmax;
	}
#else
	if(ntags>=MAXTAGS)
	{
		_ASSERT(false);
		WSLogError("AddTag overflow at %d tags!",ntags);
		return 0;
	}
#endif
	FIXTAG *ftag=&tags[ntags++];
	ftag->no=tag;
	ftag->name=0;
	ftag->tstart=tstart;
	ftag->vlen=vlen;
	ftag->val=val;
	return ftag;
}
void FIXINFO::RebaseFixInfo(char *Msg)
{
	LONGLONG diff=(Msg -fbuf);
	fbuf+=diff;
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	for(int i=0;i<maxtags;i++)
	{
		if((!tags[i].stag)&&(tags[i].val))
		{
			tags[i].val+=diff;
		}
	}
#else
	for(int i=0;i<MAXTAGS;i++)
	{
		if(tags[i].val)
		{
			tags[i].val+=diff;
		}
	}
#endif
}
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
FIXTAG *FIXINFO::AddTag(int tag, const char *tname, int tnlen, const char *val, int tstart, int vlen)
{
	if(ntags>=maxtags)
	{
		int nmax=maxtags;
		if(nmax>0)
			nmax*=2;
		else
			nmax=32;
		FIXTAG *ntags=new FIXTAG[nmax];
		memset(ntags,0,nmax*sizeof(FIXTAG));
		memcpy(ntags,tags,maxtags*sizeof(FIXTAG));
		delete tags;
		tags=ntags;
		maxtags=nmax;
	}
	FIXTAG *ftag=&tags[ntags++];
	ftag->no=tag;
	if(tname && *tname)
	{
		//int tnlen=strlen(tname)+1;
		//ftag->name=new char[tnlen];
		ftag->name=tname;
		ftag->tnlen=tnlen;
	}
	else
	{
		ftag->name=0;
		ftag->tnlen=0;
	}
	ftag->tstart=tstart;
	ftag->vlen=vlen;
	ftag->val=val;
	return ftag;
}
void FIXINFO::RebaseSetInfo(char *Msg)
{
	LONGLONG diff=(Msg -sbuf);
	sbuf+=diff;
	for(int i=0;i<maxtags;i++)
	{
		if((tags[i].stag)&&(tags[i].val))
		{
			tags[i].val+=diff;
		}
	}
}
void FIXINFO::Copy(const FIXINFO *ifix, bool deepCopy)
{
	fci=ifix->fci;
	//fbuf=ifix->fbuf;
	if(deepCopy)
	{
		fbuf=0;
		for(int i=0;i<ifix->ntags;i++)
		{
		#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_7)
			if(ifix->tags[i].name)
			{
				/* The tag name inside 'sbuf' might get reallocated, so best to use the dynamic tag name set
				FIXTAG *ftag=&ifix->tags[i];
				char tname[128];
				int tnlen=ftag->tnlen;
				if(tnlen>127)
					tnlen=127;
				memcpy(tname,ftag->name,tnlen);
				tname[tnlen]=0;
				ftag=SetTag(tname,&ifix->tags[i]);
				ftag->name=ifix->tags[i].name;
				ftag->dname=true;
				*/
				FIXTAG *ftag=&ifix->tags[i];
				char tname[128];
				int tnlen=ftag->tnlen;
				if(tnlen>127)
					tnlen=127;
				memcpy(tname,ftag->name,tnlen);
				tname[tnlen]=0;
				SetDynTag(tname,ftag);
			}
			else
		#endif
				SetTag(ifix->tags[i].no,&ifix->tags[i]);
		}
	}
	else
	{
		fbuf=ifix->fbuf;
		//maxtags=ifix->maxtags;
		maxtags=ifix->ntags;
		tags=new FIXTAG[maxtags];
		memset(tags,0,maxtags*sizeof(FIXTAG));
		ntags=ifix->ntags;
		memcpy(tags,ifix->tags,ntags*sizeof(FIXTAG));
		llen=ifix->llen;
		chksum=ifix->chksum;
		// Don't copy the sbuf for mem ownership reasons
		//sbuf=ifix->sbuf;
		//slen=ifix->slen;
		//ssize=ifix->ssize;
	}
}
#endif//BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0
int FIXINFO::FixMsgReady(char *Msg, int MsgLen)
{
	if(MsgLen>0)
	{
		// UsrPort[PortNo].InBuffer.Block can change, so reset the buffer pointer
		FIXINFO *ifix=this;
		if(!ifix->fbuf)
			ifix->fbuf=Msg;
		else if(ifix->fbuf!=Msg)
			ifix->RebaseFixInfo(Msg);
		const char *MsgEnd=Msg+MsgLen;
		for(const char *mptr=Msg+ifix->llen;mptr<MsgEnd;)
		{
			const char *tend=(char*)strechr(mptr,FIXDELIM,MsgEnd);
			// Allow last tag to not have delim	when not a live session
			if(noSession)
			{
				if(!tend) 
					tend=MsgEnd;
			}
			if(tend<=mptr)
				return 0;
			const char *vptr=strechr(mptr,'=',tend);
			if(vptr) 
				vptr++; 
			else
				vptr=tend;
			//char val[256];
			int vlen=(int)(tend-vptr);
			//memcpy(val,vptr,vlen); val[vlen]=0;

			int tag=myatoi(mptr);
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
			const char *tname=0;
			int tnlen=0;
			if(!myisdigit(*mptr))
			{
				tname=mptr;
				tnlen=(int)(vptr -mptr -1);
			}
			if(!ifix->AddTag(tag,tname,tnlen,vptr,(int)(mptr-Msg),vlen))
				return -1;
#else
			if(!ifix->AddTag(tag,vptr,mptr-Msg,vlen))
				return -1;
#endif
			// Allow free-form FIX
			if(!noSession)
			{
				// Thow away everything before SOM
				if((ifix->ntags==1)
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
					&&(tag!=8)&&(!tname||strincmp(tname,OFTN_BEGINSTRING,tnlen)!=0)
#endif
					)
				{
					mptr=tend+1;
					ifix->llen=(int)(mptr-Msg);
					if(!supressChkErr)
						WSLogError("Tag %d received before SOM! Discarding %d bytes",tag,ifix->llen);
					return ifix->llen;
				}
			}
			if(tag==10)
			{
				int tchk=myatoi(vptr);
				if((ifix->chksum!=tchk)&&(!supressChkErr))
					WSLogError("Bad checksum in msg %d of %d: computed %d!",ifix->TagInt(34),tchk,ifix->chksum);
			}
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
			else if(tname&&strincmp(tname,OFTN_CHECKSUM,tnlen)==0)
			{
				int tchk=myatoi(vptr);
				if((ifix->chksum!=tchk)&&(!supressChkErr))
					WSLogError("Bad checksum in msg %d of %d: computed %d!",ifix->TagInt(OFTN_MSGSEQNUM),tchk,ifix->chksum);
			}
#endif
			else// if(!ifix->GetTag(10))
			{
				for(const char *cptr=mptr;cptr<tend;cptr++)
					ifix->chksum+=*cptr;
				ifix->chksum+=0x01;
			}
			//*tend='|'; 
			mptr=(tend<MsgEnd) ?tend+1 :tend;
			// Handle multiple back to back delimiters and
			// optional 2nd delimiter (in addition to primary delimiter)
			while((mptr<MsgEnd)&&
				  ((*mptr==FIXDELIM)||((FIXDELIM2)&&(*mptr==FIXDELIM2))))
				mptr++;
			ifix->llen=(int)(mptr-Msg);
			// Allow free-form FIX
			if(!noSession)
			{
				if(tag==10
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
					||((tname)&&(tnlen>0)&&(strincmp(tname,OFTN_CHECKSUM,tnlen)==0))
#endif
					)
					break;
			}
		}
		if(noSession)
			return ifix->llen;
		else
		{
			if(ifix->GetTag(10)
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
			||ifix->GetTag(OFTN_CHECKSUM)
#endif
				)
				return ifix->llen;
		}
	}
	return 0;
}

#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
FIXTAG *FIXINFO::_SetTag(int tno, const char *tname, const char *val, int vlen)
{
	if(tno)
	{
		FIXTAG *ftag=GetTag(tno);
		if(ftag)
		{
			if((ftag->vlen==vlen)&&(!strncmp(ftag->val,val,vlen)))
				return ftag;
			DelTag(tno);
		}
	}
	if(tname && *tname)
	{
		FIXTAG *ftag=GetTag(tname);
		if(ftag)
		{
			if((ftag->vlen==vlen)&&(!strncmp(ftag->val,val,vlen)))
				return ftag;
			DelTag(tname);
		}
	}
	char tstr[128]={0};
	if(tname && *tname)
		sprintf_s(tstr,sizeof(tstr),"%s=",tname);
	else
		sprintf_s(tstr,sizeof(tstr),"%d=",tno);
	int nlen=(int)strlen(tstr) +vlen +1;
	int nsize=ssize;
	char *vcpy=0;
	while(nsize -slen<nlen)
	{
		if(nsize) nsize*=2;
		else nsize=128;
		char *nbuf=new char[nsize];
		memset(nbuf,0,nsize);
		memcpy(nbuf,sbuf,slen);
		// 'val' may be set to one of the fix tag values in 'sbuf'
		if(vlen>0)
		{
			vcpy=new char[vlen];
			memcpy(vcpy,val,vlen);
			val=vcpy;
		}
		if(sbuf) delete sbuf;
		RebaseSetInfo(nbuf);
		sbuf=nbuf;
		ssize=nsize;
	}
	char *tstart=sbuf +slen;
	strcpy_s(tstart,sbuf +ssize -tstart,tstr);
	char *vptr=tstart +strlen(tstart);
	memcpy(vptr,val,vlen);
	vptr[vlen]=FIXDELIM;
	if(vcpy)
	{
		delete vcpy; vcpy=0;
	}
	slen+=nlen;

	if(ntags>=maxtags)
	{
		int nmax=maxtags;
		if(nmax>0)
			nmax*=2;
		else
			nmax=32;
		FIXTAG *ntags=new FIXTAG[nmax];
		memcpy(ntags,tags,maxtags*sizeof(FIXTAG));
		memset(&ntags[maxtags],0,(nmax -maxtags)*sizeof(FIXTAG));
		delete tags;
		tags=ntags;
		maxtags=nmax;
	}
	FIXTAG *ftag=&tags[ntags++];
	ftag->no=tno;
	if(tname && *tname)
	{
		//int tnlen=strlen(tname)+1;
		//ftag->name=new char[tnlen];
		ftag->name=tname;
		ftag->tnlen=(int)strlen(tname);
	}
	else
	{
		ftag->name=0;
		ftag->tnlen=0;
	}
	ftag->tstart=(int)(tstart -sbuf);
	ftag->vlen=vlen;
	ftag->val=vptr;
	ftag->stag=true;
	return ftag;
}
void FIXINFO::DelTag(int tno)
{
	for(int i=0;i<ntags;i++)
	{
		if(tags[i].no==tno)
		{
			ntags --;
			memcpy(&tags[i],&tags[i+1],(ntags -i)*sizeof(FIXTAG));
			memset(&tags[ntags],0,sizeof(FIXTAG));
			break;
		}
	}
}
static int SortTags(const void *e1, const void *e2)
{
	FIXTAG *ftag1=(FIXTAG *)e1;
	FIXTAG *ftag2=(FIXTAG *)e2;
	if(!ftag1->no && !ftag2->no && ftag1->name && ftag2->name)
		return !_stricmp(ftag1->name,ftag2->name);
	return ftag1->no -ftag2->no;
}
int FIXINFO::Merge()
{
	if(!sbuf||fbuf==sbuf)
		return 0;
	// Default required tags
	if(!GetTag(8)&&!GetTag(OFTN_BEGINSTRING))
		SetTag(8,"FIX.4.2");

	qsort(tags,ntags,sizeof(FIXTAG),SortTags);

	int blen=0;
	for(int i=0;i<ntags;i++)
	{
		FIXTAG *ftag=&tags[i];
		bool excused=false;
		switch(ftag->no)
		{
		case 8:
		case 9:
		case 10:
			excused=true;
			break;
		case 0:
			if(ftag->name)
			{
				if(!strincmp(ftag->name,OFTN_BEGINSTRING,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_BODYLENGTH,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_CHECKSUM,ftag->tnlen))
					excused=true;
			}
		}
		if(!excused)
		{
			char tstr[128]={0},*tptr=tstr;
			if(ftag->name)
				memcpy(tptr,ftag->name,ftag->tnlen);
			else
				sprintf_s(tptr,sizeof(tstr) -(tptr -tstr),"%d",ftag->no);
			tptr+=strlen(tptr);
			strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"="); tptr+=strlen(tptr);
			for(tptr=tstr;*tptr;tptr++)
				blen++;
			for(const char *vptr=ftag->val;vptr<ftag->val +ftag->vlen;vptr++)
				blen++;
			blen++;
		}
	}
	if(GetTag(OFTN_BODYLENGTH))
		SetTag(OFTN_BODYLENGTH,blen);
	else
		SetTag(9,blen);

	chksum=0;
	for(int i=0;i<ntags;i++)
	{
		FIXTAG *ftag=&tags[i];
		bool excused=false;
		switch(ftag->no)
		{
		case 10:
			excused=true;
			break;
		case 0:
			if(ftag->name)
			{
				if(!strincmp(ftag->name,OFTN_CHECKSUM,ftag->tnlen))
					excused=true;
			}
		}
		if(!excused)
		{
			char tstr[128]={0},*tptr=tstr;
			if(ftag->name)
				memcpy(tptr,ftag->name,ftag->tnlen);
			else
				sprintf_s(tptr,sizeof(tstr) -(tptr -tstr),"%d",ftag->no);
			tptr+=strlen(tptr);
			strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"="); tptr+=strlen(tptr);
			for(tptr=tstr;*tptr;tptr++)
				chksum+=*tptr;
			for(const char *vptr=ftag->val;vptr<ftag->val +ftag->vlen;vptr++)
				chksum+=*vptr;
			chksum+=0x01;
		}
	}
	char cstr[8]={0};
	sprintf_s(cstr,sizeof(cstr),"%03d",chksum);
	if(GetTag(OFTN_CHECKSUM))
		SetTag(OFTN_CHECKSUM,cstr);
	else
		SetTag(10,cstr);

	int nlen=llen +slen;
	char *nbuf=new char[nlen];
	memset(nbuf,0,nlen);
	char *nptr=nbuf;
	slen=0;
	// Header tags
	FIXTAG *ftag=GetTag(8);
	if(!ftag)
		ftag=GetTag(OFTN_BEGINSTRING);
	char tstr[128]={0},*tptr=tstr;
	if(ftag->name)
	{
		memcpy(tptr,ftag->name,ftag->tnlen); tptr+=ftag->tnlen;
		strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"=");
	}
	else
		sprintf_s(tstr,sizeof(tstr) -(tptr -tstr),"%d=",ftag->no);
	tptr+=strlen(tptr);
	strcpy_s(nptr,nlen -(nptr -nbuf),tstr); nptr+=strlen(nptr);
	memcpy(nptr,ftag->val,ftag->vlen); nptr+=ftag->vlen;
	*nptr=FIXDELIM; nptr++;

	ftag=GetTag(9);
	if(!ftag)
		ftag=GetTag(OFTN_BODYLENGTH);
	tptr=tstr;
	if(ftag->name)
	{
		memcpy(tptr,ftag->name,ftag->tnlen); tptr+=ftag->tnlen;
		strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"=");
	}
	else
		sprintf_s(tstr,sizeof(tstr),"%d=",ftag->no);
	tptr+=strlen(tptr);
	strcpy_s(nptr,nlen -(nptr -nbuf),tstr); nptr+=strlen(nptr);
	memcpy(nptr,ftag->val,ftag->vlen); nptr+=ftag->vlen;
	*nptr=FIXDELIM; nptr++;

	ftag=GetTag(35);
	if(!ftag)
		ftag=GetTag(OFTN_MSGTYPE);
	if(ftag)
	{
		tptr=tstr;
		if(ftag->name)
		{
			memcpy(tptr,ftag->name,ftag->tnlen); tptr+=ftag->tnlen;
			strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"=");
		}
		else
			sprintf_s(tstr,sizeof(tstr),"%d=",ftag->no);
		tptr+=strlen(tptr);
		strcpy_s(nptr,nlen -(nptr -nbuf),tstr); nptr+=strlen(nptr);
		memcpy(nptr,ftag->val,ftag->vlen); nptr+=ftag->vlen;
		*nptr=FIXDELIM; nptr++;
	}

	for(int i=0;i<ntags;i++)
	{
		FIXTAG *ftag=&tags[i];
		bool hdrtag=false;
		switch(ftag->no)
		{
		case 34:
		case 49:
		case 50:
		case 52:
		case 56:
		case 57:
			hdrtag=true;
			break;
		case 0:
			if(ftag->name)
			{
				if(!strincmp(ftag->name,OFTN_MSGSEQNUM,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_SENDERCOMPID,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_SENDERSUBID,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_SENDINGTIME,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_TARGETCOMPID,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_TARGETSUBID,ftag->tnlen))
					hdrtag=true;
			}
		}
		if(hdrtag)
		{
			char tstr[128]={0},*tptr=tstr;
			if(ftag->name)
			{
				memcpy(tptr,ftag->name,ftag->tnlen); tptr+=ftag->tnlen;
				strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"=");
			}
			else
				sprintf_s(tstr,sizeof(tstr),"%d=",ftag->no);
			tptr+=strlen(tptr);
			strcpy_s(nptr,nlen -(nptr -nbuf),tstr); nptr+=strlen(nptr);
			memcpy(nptr,ftag->val,ftag->vlen); nptr+=ftag->vlen;
			*nptr=FIXDELIM; nptr++;
		}
	}
	// Body tags
	for(int i=0;i<ntags;i++)
	{
		FIXTAG *ftag=&tags[i];
		bool excused=false;
		switch(ftag->no)
		{
		case 8:
		case 9:
		case 10:
		case 34:
		case 35:
		case 49:
		case 50:
		case 52:
		case 56:
		case 57:
			excused=true;
			break;
		case 0:
			if(ftag->name)
			{
				if(!strincmp(ftag->name,OFTN_BEGINSTRING,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_BODYLENGTH,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_CHECKSUM,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_MSGSEQNUM,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_MSGTYPE,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_SENDERCOMPID,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_SENDERSUBID,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_SENDINGTIME,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_TARGETCOMPID,ftag->tnlen)||
				   !strincmp(ftag->name,OFTN_TARGETSUBID,ftag->tnlen))
					excused=true;
			}
		}
		if(!excused)
		{
			char tstr[128]={0},*tptr=tstr;
			if(ftag->name)
				memcpy(tptr,ftag->name,ftag->tnlen);
			else
				sprintf_s(tptr,sizeof(tstr) -(tptr -tstr),"%d",ftag->no);
			tptr+=strlen(tptr);
			strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"="); tptr+=strlen(tptr);
			strcpy_s(nptr,nlen -(nptr -nbuf),tstr); nptr+=strlen(nptr);
			memcpy(nptr,ftag->val,ftag->vlen); nptr+=ftag->vlen;
			*nptr=FIXDELIM; nptr++;
		}
	}
	// End tag
	ftag=GetTag(10);
	if(!ftag)
		ftag=GetTag(OFTN_CHECKSUM);
	tptr=tstr;
	if(ftag->name)
	{
		memcpy(tptr,ftag->name,ftag->tnlen); tptr+=ftag->tnlen;
		strcpy_s(tptr,sizeof(tstr) -(tptr -tstr),"=");
	}
	else
		sprintf_s(tstr,sizeof(tstr),"%d=",ftag->no);
	tptr+=strlen(tptr);
	strcpy_s(nptr,nlen -(nptr -nbuf),tstr); nptr+=strlen(nptr);
	memcpy(nptr,ftag->val,ftag->vlen); nptr+=ftag->vlen;
	*nptr=FIXDELIM; nptr++;
	_ASSERT(!strestr(nbuf,"==",nptr));

	_ASSERT(nptr -nbuf<=nlen);
	if(sbuf) delete sbuf;
	sbuf=0;
	ssize=slen=0;

	memset(tags,0,maxtags*sizeof(FIXTAG)); ntags=0;
	fbuf=0;	llen=0; chksum=0;
	FixMsgReady(nbuf,(int)(nptr -nbuf));
	sbuf=nbuf;
	ssize=nlen;
	slen=(int)(nptr -nbuf);
	return 0;
}

void FIXINFO::DelTag(const char *tname)
{
	int tnlen=(int)strlen(tname);
	for(int i=0;i<ntags;i++)
	{
		if((tags[i].name)&&(tags[i].tnlen==tnlen)&&(!strincmp(tags[i].name,tname,tags[i].tnlen)))
		{
			ntags --;
			memcpy(&tags[i],&tags[i+1],(ntags -i)*sizeof(FIXTAG));
			memset(&tags[ntags],0,sizeof(FIXTAG));
			break;
		}
	}
}
#endif//BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0

#endif//BFIX_CUR_SCHEMA>=BFIX_SCHEMA_1_2

