#include "stdafx.h"

#include "wsocksapi.h"
#include "sysmonc.h"
#include "wstring.h"

#define TVDELIM '|'
#define TAG_REQUESTID "rid"
#define TAG_CMD "cmd"

int SysCmdCodec::Init()
{
	return 0;
}
int SysCmdCodec::Shutdown()
{
	return 0;
}
// Tags are limited to 127 chars, but no limit on values
int SysCmdCodec::GetTagValues(TVMAP& tvmap, const char *parm, int plen)
{
	int ntags=0;
	const char *send=parm +plen;
	for(const char *sptr=parm;sptr<send;)
	{
		char tag[128]={0},*val=0;
		const char *tptr=sptr;
		for(;(sptr<send)&&(*sptr!='=')&&(*sptr!=TVDELIM);sptr++)
			;
		int tlen=(int)(sptr -tptr);
		if(tlen>127) tlen=127;
		memcpy(tag,tptr,tlen); tag[tlen]=0;
		_strlwr(tag);
		if(sptr>=send)
			break;
		const char *vptr=sptr;
		if((sptr<send)&&(*sptr=='='))
		{
			vptr++; sptr++;
			for(;(sptr<send)&&(*sptr!=TVDELIM);sptr++)
				;
			int vlen=(int)(sptr -vptr);
			val=(char*)vptr; 
			char save=val[vlen]; val[vlen]=0;
			tvmap[tag]=val;
			val[vlen]=save;
		}
		else
			tvmap[tag]="";
		ntags++;
		if(sptr>=send)
			break;
		sptr++;
	}
	return ntags;
}
int SysCmdCodec::SetTagValues(TVMAP& tvmap, char *parm, int plen)
{
	char *pptr=parm;
	const char *pend=parm +plen;
	for(TVMAP::iterator tit=tvmap.begin();tit!=tvmap.end();tit++)
	{
		int vlen=(int)tit->first.length() +1 +(int)tit->second.length() +2;
		if(pptr +vlen>=pend)
		{
			_ASSERT(false);
			return -1;
		}
		// Values may not have the delimiter in them
		if(strchr(tit->second.c_str(),TVDELIM))
			_ASSERT(false);
		else
		{
			sprintf(pptr,"%s=%s%c",tit->first.c_str(),tit->second.c_str(),TVDELIM);
			pptr+=strlen(pptr);
		}
	}
	_ASSERT(pptr -parm<=plen);
	return (int)(pptr -parm);
}
string SysCmdCodec::GetValue(TVMAP& tvmap, const string tag)
{
	string val;
	char ltag[128]={0};
	strncpy(ltag,tag.c_str(),127); ltag[127]=0;
	_strlwr(ltag);
	TVMAP::iterator tit=tvmap.find(ltag);
	if(tit==tvmap.end())
		return (string)"";
	return tit->second;
}
bool SysCmdCodec::HasValue(TVMAP& tvmap, const string tag)
{
	string val;
	char ltag[128]={0};
	strncpy(ltag,tag.c_str(),127); ltag[127]=0;
	_strlwr(ltag);
	TVMAP::iterator tit=tvmap.find(ltag);
	if(tit==tvmap.end())
		return false;
	return true;
}
int SysCmdCodec::SetValue(TVMAP& tvmap, const string tag, string value)
{
	// Values may not have the delimiter in them
	if(strchr(value.c_str(),TVDELIM))
	{
		_ASSERT(false);
		return -1;
	}
	char ltag[128]={0};
	strncpy(ltag,tag.c_str(),127); ltag[127]=0;
	_strlwr(ltag);
	tvmap[ltag]=value;
	return 0;
}
int SysCmdCodec::DelTag(TVMAP& tvmap, const string tag)
{
	char ltag[128]={0};
	strncpy(ltag,tag.c_str(),127); ltag[127]=0;
	_strlwr(ltag);
	TVMAP::iterator tit=tvmap.find(ltag);
	if(tit==tvmap.end())
		return -1;
	tvmap.erase(tit);
	return 0;
}

int SysCmdCodec::EscapeText(char *etext, const char *text)
{
	char *eptr=etext;
	for(const char *tptr=text;*tptr;tptr++)
	{
		char ch=*tptr;
		if(ch=='|')
		{
			strcpy(eptr,"\\/"); eptr+=2;
		}
		else
		{
			*eptr=ch; eptr++;
		}
	}
	return 0;
}
int SysCmdCodec::UnescapeText(char *etext, const char *text)
{
	char *eptr=etext;
	bool escape=false;
	for(const char *tptr=text;*tptr;tptr++)
	{
		char ch=*tptr;
		if((!escape)&&(ch=='\\'))
			escape=true;
		else if(escape)
		{
			if(ch=='/') 
			{
				*eptr='|'; eptr++;
			}
			else
			{
				*eptr='\\'; eptr++; *eptr=ch; eptr++;
			}
			escape=false;
		}
		else
		{
			*eptr=ch; eptr++;
		}
	}
	return 0;
}

int SysCmdCodec::Encode(char *&mptr, int mlen, bool request, int reqid, string cmd, const char *parm, int plen)
{
	const char *mend=mptr +mlen;
	sprintf(mptr,"%s=%d%c",TAG_REQUESTID,reqid,TVDELIM); mptr+=strlen(mptr);
	sprintf(mptr,"%s=%s%c",TAG_CMD,cmd.c_str(),TVDELIM); mptr+=strlen(mptr);
	if(parm)
	{
		if(!plen) plen=(int)strlen(parm);
		memcpy(mptr,parm,plen); mptr+=plen;
	}
	*mptr=0; mptr++;
	_ASSERT(mptr<=mend);
	return 0;
}
int SysCmdCodec::Decode(const char *&mptr, int mlen, bool request, SysCmdNotify *pnotify, void *udata)
{
	const char *mend=mptr +mlen;
	char reqid[128]={0},cmd[128]={0};
	const char *parm=0;
	int plen=0;

	// Many commands/responses may be embedded in one message separated by null
	while(mptr<mend)
	{
		parm=0; plen=0;
		// RequestID
		int tlen=(int)strlen(TAG_REQUESTID);
		if((mend -mptr<tlen+1)||(strncmp(mptr,TAG_REQUESTID,tlen))||(mptr[tlen]!='='))
			return -1;
		mptr+=tlen+1;
		const char *rptr;
		for(rptr=mptr;(mptr<mend)&&(*mptr!=TVDELIM);mptr++)
			;
		int rlen=(int)(mptr -rptr);
		if(rlen>127) rlen=127;
		memcpy(reqid,rptr,rlen); reqid[rlen]=0;
		mptr++;
		if(mptr>=mend)
			return -1;
		// Cmd
		tlen=(int)strlen(TAG_CMD);
		if((mend -mptr<tlen+1)||(strncmp(mptr,TAG_CMD,tlen))||(mptr[tlen]!='='))
			return -1;
		mptr+=tlen+1;
		const char *cptr;
		for(cptr=mptr;(mptr<mend)&&(*mptr!=TVDELIM);mptr++)
			;
		int clen=(int)(mptr -cptr);
		if(clen>127) clen=127;
		memcpy(cmd,cptr,clen); cmd[clen]=0;
		// Parms
		mptr++;
		if(mptr<mend)
		{
			parm=mptr;
			for(;(mptr<mend)&&(*mptr);mptr++)
				;
			plen=(int)(mptr -parm);
		}
		if(pnotify)
		{
			if(request)
				pnotify->NotifyRequest(atol(reqid),cmd,parm,plen,udata);
			else
				pnotify->NotifyResponse(atol(reqid),cmd,parm,plen,udata);
		}
		// Next command
		if(mptr<mend)
			mptr++;
	}
	return 0;
}

SysFeedCodec::SysFeedCodec()
	:ndata(0)
{
}
SysFeedCodec::~SysFeedCodec()
{
}
int SysFeedCodec::Init()
{
#ifdef _DEBUG
	// Self-validate encoding
	char buf[1024]={0},*bptr=buf,*bend=buf +sizeof(buf);
	#ifdef WIN32
	encode_int7(bptr,(int)(bend -bptr),-(__int64)2999888777);
	#endif
	encode_int7(bptr,(int)(bend -bptr),-1000000);
	encode_int7(bptr,(int)(bend -bptr),-8192);
	encode_int7(bptr,(int)(bend -bptr),-4096);
	encode_int7(bptr,(int)(bend -bptr),-1024);
	encode_int7(bptr,(int)(bend -bptr),-256);
	encode_int7(bptr,(int)(bend -bptr),-1);
	encode_int7(bptr,(int)(bend -bptr),0);
	encode_int7(bptr,(int)(bend -bptr),1);
	encode_int7(bptr,(int)(bend -bptr),256);
	encode_int7(bptr,(int)(bend -bptr),1024);
	encode_int7(bptr,(int)(bend -bptr),4096);
	encode_int7(bptr,(int)(bend -bptr),8192);
	encode_int7(bptr,(int)(bend -bptr),1000000);
	#ifdef WIN32
	encode_int7(bptr,(int)(bend -bptr),2999888777);
	#endif

	bptr=buf;
	__int64 ival=0;
	#ifdef WIN32
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-(__int64)2999888777);
	#endif
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-1000000);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-8192);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-4096);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-1024);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-256);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==-1);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==0);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==1);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==256);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==1024);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==4096);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==8192);
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==1000000);
	#ifdef WIN32
	decode_int7(ival,(const char *&)bptr,bend,true); _ASSERT(ival==2999888777);
	#endif
#endif
	return 0;
}
int SysFeedCodec::Shutdown()
{
	return 0;
}

// Stop-bit, sign-bit 7-bit integer encoder
int SysFeedCodec::encode_int7(char *&fptr, int flen, __int64 ival)
{
	bool neg=false;
	if(ival<0)
	{
		neg=true; ival=-ival;
	}
	// Write 7 bits at a time backwards into a temporary buffer
	unsigned char revbuf[32]={0},*rptr=revbuf;
	if(ival>0)
	{
		while((ival>0)&&(rptr<revbuf+32))
		{
			*rptr=(unsigned char)(ival&0x7F); rptr++;
			ival=ival>>7;
		}
		if(ival>0)//overflow
			return -1;
		rptr--;
	}
	else
		*rptr=0;
	// High byte can only hold 6 bits
	// Need an extra byte
	if(*rptr>=0x40)
	{
		if(rptr>=revbuf+32)// overflow
			return -1;
		unsigned char hbit=(*rptr)&0x80; // only move the high bit to the extra byte
		*rptr=(*rptr)&0x7F; rptr++; *rptr=(hbit>>7);
	}
	if(neg) *rptr+=0x40;
	// Low byte gets stop bit
	revbuf[0]+=0x80;
	// Invert to network order
	for(;rptr>=revbuf;rptr--)
	{
		*fptr=*rptr; fptr++;
	}
	return 0;
}
int SysFeedCodec::decode_int7(__int64& ival, const char *&eptr, const char *eend, bool sgn)
{
	ival=0;
	// If negative allowed (sign bit)
	bool neg=(sgn && eptr[0]&0x40) ?true :false;
	// Max 63 bits from 9 bytes
	for(int nb=0;(eptr<eend)&&(nb<9);eptr++,nb++)
	{
		if(!nb) // Take only 6 bits from high byte
			ival=(eptr[0]&0x3F);
		else // Shift 7 bits to make room for the next 7 bits
			ival=(ival<<7) |(eptr[0]&0x7F);
		if(eptr[0]&0x80)
		{
			eptr++;
			break;
		}
	}
	if(neg)
		ival=-ival;
	return 0;
}
int SysFeedCodec::encode_pchar(char *&fptr, int flen, const char *sval, int slen)
{
	const char *fstart=fptr;
	const char *fend=fptr +flen;
	if(!slen)
		slen=(int)strlen(sval) +1;
	const char *send=sval +slen;
	for(const char *sptr=sval;(fptr<fend)&&(sptr<send)&&(*sptr);sptr++)
	{
		*fptr=*sptr; fptr++;
	}
	if(fptr<fend)
	{
		*fptr=0; fptr++;
	}
	return (int)(fptr -fstart);
}
int SysFeedCodec::decode_pchar(char *sval, int slen, const char *&eptr, const char *eend)
{
	const char *send=sval+slen;
	char *sptr=sval;
	for(;(eptr<eend)&&(*eptr);eptr++)
	{
        if(sptr<(send -1))
        {
            *sptr=*eptr; sptr++;
        }
	}
	eptr++;
	slen=(int)(sptr -sval);
	if(sptr<send)
	{
		*sptr=0; sptr++;
	}
	return slen;
}

int SysFeedCodec::encode_binary(char *&fptr, int flen, const void *buf, int blen)
{
	if(flen<4+blen)
		return -1;
	if(encode_int7(fptr,flen,blen)<0)
		return -1;
	const char *fstart=fptr;
	memcpy(fptr,buf,blen); fptr+=blen;
	return (int)(fptr -fstart);
}
int SysFeedCodec::decode_binary(char *buf, int blen, const char *&eptr, const char *eend)
{
	__int64 elen=0;
	if(decode_int7(elen,eptr,eend,false)<0)
		return -1;
	if(blen<elen)
		return -1;
	memcpy(buf,eptr,(int)elen); eptr+=elen;
	return (int)elen;
}

// The Eye feed
int SysFeedCodec::EncodeLog(char *&mptr, int mlen, const char *aclass, const char *aname, 
							const char *lpath, bool hist, __int64 off, const char *buf, int blen)
{
	// TODO: Make this codec efficient
	const char *mstart=mptr;
	const char *mend=mptr +mlen;
	if(strrcmp(lpath,"err.txt"))
		*mptr=SMFNO_ERRORLOG; 
	else//if(strrcmp(lpath,"eve.txt"))
		*mptr=SMFNO_EVENTLOG; 
	mptr++;
	encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
	encode_pchar(mptr,(int)(mend -mptr),aclass,0);
	encode_pchar(mptr,(int)(mend -mptr),aname,0);
	encode_pchar(mptr,(int)(mend -mptr),lpath,0);
	encode_int7(mptr,(int)(mend -mptr),off);
	encode_binary(mptr,(int)(mend -mptr),buf,blen);
	_ASSERT(mptr<mend);
	return (int)(mptr -mstart);
}
int SysFeedCodec::DecodeLog(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata)
{
	const char *mend=mptr +mlen;
	bool hist=false;
	decode_int7(hist,mptr,mend);
	char aclass[256]={0},aname[256]={0};
	decode_pchar(aclass,sizeof(aclass),mptr,mend);
	decode_pchar(aname,sizeof(aname),mptr,mend);
	char lpath[MAX_PATH]={0};
	decode_pchar(lpath,sizeof(lpath),mptr,mend);
	__int64 off=0;
	decode_int7(off,mptr,mend);
	char buf[4096]={0}; // This buffer size must match SendEyeHist functions
	int blen=decode_binary(buf,sizeof(buf),mptr,mend);
	_ASSERT(mptr<=mend);
	pnotify->NotifyLog(aclass,aname,lpath,hist,off,0x00,buf,blen,udata);
	return 0;
}

// Sysmon feed
int SysFeedCodec::EncodeLvl1(char *&mptr, int mlen, const char *aclass, const char *aname, 
	WsocksApp *pmod, WSPortType PortType, int PortNo, bool hist, const char *buf, int blen, bool force)
{
	const char *mstart=mptr;
	const char *mend=mptr +mlen;
	switch(PortType)
	{
	case WS_CON:
	{
		if((pmod->ConPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_CON_PORTS)||(force))
		{
			*mptr=SMFNO_LVL1; mptr++;
			encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
			encode_pchar(mptr,(int)(mend -mptr),aclass,0);
			encode_pchar(mptr,(int)(mend -mptr),aname,0);
			encode_int7(mptr,(int)(mend -mptr),0x16);//record schema
			encode_int7(mptr,(int)(mend -mptr),PortType);
			encode_int7(mptr,(int)(mend -mptr),PortNo);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InUse);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].SockConnected);
			encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].RemoteIP,20);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Port);
			encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].CfgNote,80);    
			encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OnLineStatusText,80);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BeatsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InBuffer.Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecAvgIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecMaxIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BlocksIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].PacketsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BeatsOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutBuffer.Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecAvgOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecMaxOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BlocksOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].PacketsOut);	
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesOut);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltRemoteIP[i],20);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltRemoteIPOn[i]);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltRemotePort[i]);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltIPCount);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].CurrentAltIP);
			encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ConnectHold);
		}
		return 0;
	}
	case WS_USC:
	{
		if((pmod->UscPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_USC_PORTS)||(force))
		{
			*mptr=SMFNO_LVL1; mptr++;
			encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
			encode_pchar(mptr,(int)(mend -mptr),aclass,0);
			encode_pchar(mptr,(int)(mend -mptr),aname,0);
			encode_int7(mptr,(int)(mend -mptr),0x03);//record schema
			encode_int7(mptr,(int)(mend -mptr),PortType);
			encode_int7(mptr,(int)(mend -mptr),PortNo);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].InUse);
			encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].LocalIP,20);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Port);
			encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].CfgNote,80);    
			encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Status,80);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BeatsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].InBuffer_Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecAvgIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecMaxIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BlocksIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].PacketsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BeatsOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].OutBuffer_Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecAvgOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecMaxOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BlocksOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].PacketsOut);	
			encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesOut);
		}
		return 0;
	}
	case WS_CTI:
	{
		if((pmod->CtiPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_CTI_PORTS)||(force))
		{
			*mptr=SMFNO_LVL1; mptr++;
			encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
			encode_pchar(mptr,(int)(mend -mptr),aclass,0);
			encode_pchar(mptr,(int)(mend -mptr),aname,0);
			encode_int7(mptr,(int)(mend -mptr),0x04);//record schema
			encode_int7(mptr,(int)(mend -mptr),PortType);
			encode_int7(mptr,(int)(mend -mptr),PortNo);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].InUse);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].SockConnected);
			encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].RemoteIP,20);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Port);
			encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].CfgNote,80);    
			encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].OnLineStatusText,80);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BeatsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].InBuffer.Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecAvgIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecMaxIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BlocksIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].PacketsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BeatsOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecAvgOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecMaxOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BlocksOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].PacketsOut);	
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesOut);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].AltRemoteIP[i],20);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].AltRemoteIPOn[i]);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].AltIPCount);
			encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].CurrentAltIP);
		}
		return 0;
	}
	case WS_FIL:
	{
		if((pmod->FilePort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_FILE_PORTS)||(force))
		{
			*mptr=SMFNO_LVL1; mptr++;
			encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
			encode_pchar(mptr,(int)(mend -mptr),aclass,0);
			encode_pchar(mptr,(int)(mend -mptr),aname,0);
			encode_int7(mptr,(int)(mend -mptr),0x14);//record schema
			encode_int7(mptr,(int)(mend -mptr),PortType);
			encode_int7(mptr,(int)(mend -mptr),PortNo);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InUse);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].SockConnected);
			encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].RemoteIP,20);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Port);
			encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].CfgNote,80);    
			encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OnLineStatusText,80);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BeatsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InBuffer.Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecAvgIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecMaxIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BlocksIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].PacketsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BeatsOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutBuffer.Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecAvgOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecMaxOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BlocksOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].PacketsOut);	
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesOut);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].AltRemoteIP[i],20);
			for(int i=0;i<WS_MAX_ALT_PORTS;i++)
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].AltRemoteIPOn[i]);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].AltIPCount);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].CurrentAltIP);
			encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ConnectHold);
		}
		return 0;
	}
    case WS_UMC:
    {
        if((pmod->UmcPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_UMC_PORTS)||(force))
        {
            *mptr=SMFNO_LVL1; mptr++;
			encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
			encode_pchar(mptr,(int)(mend -mptr),aclass,0);
			encode_pchar(mptr,(int)(mend -mptr),aname,0);
			encode_int7(mptr,(int)(mend -mptr),0x15);//record schema
			encode_int7(mptr,(int)(mend -mptr),PortType);
			encode_int7(mptr,(int)(mend -mptr),PortNo);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].InUse);
			encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].LocalIP,20);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Port);
			encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].CfgNote,80);    
			encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Status,80);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BeatsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].InBuffer_Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecAvgIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecMaxIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BlocksIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].PacketsIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesIn);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BeatsOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].OutBuffer_Size);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecAvgOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecMaxOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BlocksOut);
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].PacketsOut);	
			encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesOut);
        }
        return(0);
    }
	};
	return -1;
}
int SysFeedCodec::EncodeLvl1(char *&mptr, int mlen, const char *aclass, const char *aname, 
	const char *heading, const char *status, WsocksApp *pmod, bool hist, HANDLE& ihnd, bool force)
{
	const char *mstart=mptr;
	const char *mend=mptr +mlen;
	// Symon really only needs CON ports and USC totals
	switch(HIWORD(ihnd))
	{
	// Connection frame first
	case 0:
	{
		*mptr=SMFNO_LVL1; mptr++;
		encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
		encode_pchar(mptr,(int)(mend -mptr),aclass,0);
		encode_pchar(mptr,(int)(mend -mptr),aname,0);
		encode_int7(mptr,(int)(mend -mptr),0x01);//record schema
		encode_int7(mptr,(int)(mend -mptr),-1);
		encode_int7(mptr,(int)(mend -mptr),-1);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CON_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_USC_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_USR_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UMC_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UMR_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CGD_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UGC_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UGR_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_FILE_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CTI_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CTO_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_OTHER_PORTS);
		encode_pchar(mptr,(int)(mend -mptr),heading,0);
		encode_pchar(mptr,(int)(mend -mptr),status,0);
		ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_CON+1);
		return 1;
	}
	// CON ports lvl1
	case WS_CON+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl1(mptr,mlen,aclass,aname,pmod,WS_CON,PortNo,hist,(char*)&pmod->ConPort[PortNo],sizeof(CONPORT),force);
		if(PortNo<pmod->NO_OF_CON_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_CON+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_USC+1);
		return 1;
	}
	// USC ports lvl1
	case WS_USC+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl1(mptr,mlen,aclass,aname,pmod,WS_USC,PortNo,hist,(char*)&pmod->UscPort[PortNo],sizeof(USCPORT),force);
		if(PortNo<pmod->NO_OF_USC_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_USC+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_CTI+1);
		return 1;
	}
	// CTI ports lvl1
	case WS_CTI+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl1(mptr,mlen,aclass,aname,pmod,WS_CTI,PortNo,hist,(char*)&pmod->CtiPort[PortNo],sizeof(CTIPORT),force);
		if(PortNo<pmod->NO_OF_CTI_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_CTI+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_FIL+1);
		return 1;
	}
	// FIL ports lvl1
	case WS_FIL+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl1(mptr,mlen,aclass,aname,pmod,WS_FIL,PortNo,hist,(char*)&pmod->FilePort[PortNo],sizeof(FILEPORT),force);
		if(PortNo<pmod->NO_OF_FILE_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_FIL+1);
		else if(!_stricmp(aclass,"smproxy"))
            ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_UMC+1);
        else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(-1,-1);
		return 1;
	}
    // UMC ports lvl1
    case WS_UMC+1:
    {
        int PortNo=LOWORD(ihnd);
        EncodeLvl1(mptr,mlen,aclass,aname,pmod,WS_UMC,PortNo,hist,(char*)&pmod->UmcPort[PortNo],sizeof(UMCPORT),force);
        if(PortNo<pmod->NO_OF_UMC_PORTS)
            ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_UMC+1);
        else
            ihnd=(HANDLE)(LONGLONG)MAKELONG(-1,-1);
        return 1;
    }
	// End of update
	case 0xFFFF:
		return 0;
	};
	return -1;
}
int SysFeedCodec::DecodeLvl1(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata)
{
	const char *mend=mptr +mlen;
	bool hist=false;
	decode_int7(hist,mptr,mend);
	char aclass[256]={0},aname[256]={0};
	decode_pchar(aclass,sizeof(aclass),mptr,mend);
	decode_pchar(aname,sizeof(aname),mptr,mend);
	unsigned char rectype=0;
	decode_int7(rectype,mptr,mend);
	WSPortType PortType=WS_UNKNOWN;
	int PortNo=-1;
	switch(rectype)
	{
	case 0x01: // Lvl1 frame
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		__int64 nports[14];
		decode_int7(nports[0],mptr,mend);
		decode_int7(nports[1],mptr,mend);
		decode_int7(nports[2],mptr,mend);
		decode_int7(nports[3],mptr,mend);
		decode_int7(nports[4],mptr,mend);
		decode_int7(nports[5],mptr,mend);
		decode_int7(nports[6],mptr,mend);
		decode_int7(nports[7],mptr,mend);
		decode_int7(nports[8],mptr,mend);
		decode_int7(nports[9],mptr,mend);
		decode_int7(nports[10],mptr,mend);
		decode_int7(nports[11],mptr,mend);
		char heading[256]={0};
		decode_pchar(heading,sizeof(heading),mptr,mend);
		nports[12]=(PTRCAST)heading;
		char status[256]={0};
		decode_pchar(status,sizeof(status),mptr,mend);
		nports[13]=(PTRCAST)status;
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)nports,sizeof(nports),udata);
		break;
	}
	case 0x02: // WS_CON lvl1 (replaced by 0x11)
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CONPORT cport;
		memset(&cport,0,sizeof(CONPORT));
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.OutBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);	
		decode_int7(cport.BytesOut,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CONPORT),udata);
		break;
	}
	case 0x03: // WS_USC lvl1
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		USCPORT uport;
		memset(&uport,0,sizeof(USCPORT));
		decode_int7(uport.InUse,mptr,mend);
		decode_pchar(uport.LocalIP,20,mptr,mend);
		decode_int7(uport.Port,mptr,mend);
		decode_pchar(uport.CfgNote,80,mptr,mend);    
		decode_pchar(uport.Status,80,mptr,mend);
		decode_int7(uport.BeatsIn,mptr,mend);
		decode_int7(uport.InBuffer_Size,mptr,mend);
		decode_int7(uport.BytesPerSecIn,mptr,mend);
		decode_int7(uport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(uport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(uport.BlocksIn,mptr,mend);
		decode_int7(uport.PacketsIn,mptr,mend);
		decode_int7(uport.BytesIn,mptr,mend);
		decode_int7(uport.BeatsOut,mptr,mend);
		decode_int7(uport.OutBuffer_Size,mptr,mend);
		decode_int7(uport.BytesPerSecOut,mptr,mend);
		decode_int7(uport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(uport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(uport.BlocksOut,mptr,mend);
		decode_int7(uport.PacketsOut,mptr,mend);	
		decode_int7(uport.BytesOut,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&uport,sizeof(USCPORT),udata);
		break;
	}
	case 0x11: // WS_CON lvl1 (replaces 0x02)
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CONPORT cport;
		memset(&cport,0,sizeof(CONPORT));
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.OutBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);	
		decode_int7(cport.BytesOut,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(cport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemoteIPOn[i],mptr,mend);
		decode_int7(cport.AltIPCount,mptr,mend);
		decode_int7(cport.CurrentAltIP,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CONPORT),udata);
		break;
	}
	case 0x12: // WS_CON lvl1 (replaces 0x11)
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CONPORT cport;
		memset(&cport,0,sizeof(CONPORT));
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.OutBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);	
		decode_int7(cport.BytesOut,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(cport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemoteIPOn[i],mptr,mend);
		decode_int7(cport.AltIPCount,mptr,mend);
		decode_int7(cport.CurrentAltIP,mptr,mend);
		decode_int7(cport.ConnectHold,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CONPORT),udata);
		break;
	}
	case 0x16: // WS_CON lvl1 (replaces 0x12)
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CONPORT cport;
		memset(&cport,0,sizeof(CONPORT));
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.OutBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);	
		decode_int7(cport.BytesOut,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(cport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemoteIPOn[i],mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemotePort[i],mptr,mend);
		decode_int7(cport.AltIPCount,mptr,mend);
		decode_int7(cport.CurrentAltIP,mptr,mend);
		decode_int7(cport.ConnectHold,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CONPORT),udata);
		break;
	}
	case 0x04: // WS_CTI lvl1
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CTIPORT cport;
		memset(&cport,0,sizeof(CTIPORT));
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);	
		decode_int7(cport.BytesOut,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(cport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemoteIPOn[i],mptr,mend);
		decode_int7(cport.AltIPCount,mptr,mend);
		decode_int7(cport.CurrentAltIP,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CTIPORT),udata);
		break;
	}
    case 0x14: // WS_FIL lvl1
	case 0x05: // DEPRECATED: changed to 0x14 to avoid confliction with level2 feed code
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		FILEPORT fport;
		memset(&fport,0,sizeof(FILEPORT));
		decode_int7(fport.InUse,mptr,mend);
		decode_int7(fport.SockConnected,mptr,mend);
		decode_pchar(fport.RemoteIP,20,mptr,mend);
		decode_int7(fport.Port,mptr,mend);
		decode_pchar(fport.CfgNote,80,mptr,mend);
		decode_pchar(fport.OnLineStatusText,80,mptr,mend);
		decode_int7(fport.BeatsIn,mptr,mend);
		decode_int7(fport.InBuffer.Size,mptr,mend);
		decode_int7(fport.BytesPerSecIn,mptr,mend);
		decode_int7(fport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(fport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(fport.BlocksIn,mptr,mend);
		decode_int7(fport.PacketsIn,mptr,mend);
		decode_int7(fport.BytesIn,mptr,mend);
		decode_int7(fport.BeatsOut,mptr,mend);
		decode_int7(fport.OutBuffer.Size,mptr,mend);
		decode_int7(fport.BytesPerSecOut,mptr,mend);
		decode_int7(fport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(fport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(fport.BlocksOut,mptr,mend);
		decode_int7(fport.PacketsOut,mptr,mend);	
		decode_int7(fport.BytesOut,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(fport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(fport.AltRemoteIPOn[i],mptr,mend);
		decode_int7(fport.AltIPCount,mptr,mend);
		decode_int7(fport.CurrentAltIP,mptr,mend);
		decode_int7(fport.ConnectHold,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&fport,sizeof(FILEPORT),udata);
		break;
	}
    case 0x15: // WS_UMC lvl1
    {
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		UMCPORT mport;
		memset(&mport,0,sizeof(UMCPORT));
		decode_int7(mport.InUse,mptr,mend);
		decode_pchar(mport.LocalIP,20,mptr,mend);
		decode_int7(mport.Port,mptr,mend);
		decode_pchar(mport.CfgNote,80,mptr,mend);    
		decode_pchar(mport.Status,80,mptr,mend);
		decode_int7(mport.BeatsIn,mptr,mend);
		decode_int7(mport.InBuffer_Size,mptr,mend);
		decode_int7(mport.BytesPerSecIn,mptr,mend);
		decode_int7(mport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(mport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(mport.BlocksIn,mptr,mend);
		decode_int7(mport.PacketsIn,mptr,mend);
		decode_int7(mport.BytesIn,mptr,mend);
		decode_int7(mport.BeatsOut,mptr,mend);
		decode_int7(mport.OutBuffer_Size,mptr,mend);
		decode_int7(mport.BytesPerSecOut,mptr,mend);
		decode_int7(mport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(mport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(mport.BlocksOut,mptr,mend);
		decode_int7(mport.PacketsOut,mptr,mend);	
		decode_int7(mport.BytesOut,mptr,mend);
		pnotify->NotifyLvl1(aclass,aname,PortType,PortNo,hist,rectype,(char*)&mport,sizeof(UMCPORT),udata);
        break;
    }
	default:
		return -1;
	};
	_ASSERT(mptr==mend);
	mptr=mend;
	return 0;
}

// Mportclient feed
int SysFeedCodec::EncodeLvl2(char *&mptr, int mlen, const char *aclass, const char *aname, 
	WsocksApp *pmod, WSPortType PortType, int PortNo, bool hist, const char *buf, int blen, bool force)
{
	const char *mstart=mptr;
	const char *mend=mptr +mlen;
	switch(PortType)
	{
	case WS_CON:
	{
		if(pmod->CON_PORT_VERSION==0x0101)
		{
			if((pmod->ConPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_CON_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x17);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].RemoteIP,20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltRemoteIP[i],20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltRemoteIPOn[i]);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltRemotePort[i]);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].S5RemoteIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].S5Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].AltIPCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].CurrentAltIP);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Compressed);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].CompressType);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Encrypted);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].S5Connect);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].S5Methode);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].S5Version);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BlockSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ChokeSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].CfgNote,80);    
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Bounce);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BounceStart);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BounceEnd);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].PortWasActiveBeforeSuspend);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].setupDlg);
				//MonitorGroups[8][16);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].MinReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].MaxReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].MinReconnectReset);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ReconnectTime);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ConnectTime);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].rmutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].smutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].recvThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].sendThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ImmediateSendLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].RecvBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].SendBuffLimit); // New for this version
				// evert thing ,including SockOpen and below,gets reset during WSConClosePort();
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].SockOpen);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].SockConnected);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].S5Status);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OnLineStatusText,80);

				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].EncryptionOn);
				encode_binary(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].pw,24);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Iiv,8);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Oiv,8);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].ReconCount);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].SinceLastBeatCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].LastDataTime);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->ConPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].LastChokeSize);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].Uuid,16);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].MonPortDataSize);
				//struct tdMonPortData *MonPortData;
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InBuffer.BlockSize);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutBuffer.BlockSize);
				//BUFFER InCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].InCompBuffer.BlockSize);
				//BUFFER OutCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].OutCompBuffer.BlockSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].lastReadyStart); // New for this version
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].IOCPSendBytes); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].peerClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].appClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->ConPort[PortNo].pendingClose); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].pendOvlRecvList); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->ConPort[PortNo].pendOvlSendList); // New for this version
				#endif
				// About 200-300 bytes
			}
			return 0;
		}
		break;
	}
	case WS_USC:
	{
		if(pmod->USC_PORT_VERSION==0x0101)
		{
			if((pmod->UscPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_USC_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x06);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].LocalIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].bindPort); //New for this version
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->UscPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BlockSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].CfgNote,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].TimeOutSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].TimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].TimeOutPeriod);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Compressed);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].CompressType);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Encrypted);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].PortWasActiveBeforeSuspend);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].IPAclCfg,128);
				//IPACL *IPAclList;
				//encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].IPAclList.Ip,20);
				//encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].IPAclList.Mask,20);
				//encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].IPAclList.lIp);
				//encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].IPAclList.lMask);
				//encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].IPAclList.action);
				//struct tdIPAcl *NextIPAcl;
				//char MonitorGroups[8][16];
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].tmutex); //New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].activeThread); //New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].ImmediateSendLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].RecvBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].SendBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].appClosed); // New for this version
			#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].pendingClose); // New for this version
			#endif
				// evert thing ,including SockActive and below,gets reset during WSUscClosePort());
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].SockActive);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].InBuffer_Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].OutBuffer_Size);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Uuid,16);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UscPort[PortNo].setupDlg);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UscPort[PortNo].CfgNoteDirty);
			}
			return 0;
		}
		break;
	}
	case WS_USR:
	{
		if((pmod->USR_PORT_VERSION==0x0101)||
		   (pmod->USR_PORT_VERSION==0x0200))
		{
			if((pmod->UsrPort[PortNo].SockActive)||(PortNo==pmod->NO_OF_USR_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x07);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].SockActive);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].RemoteIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].SinceLastBeatCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].UscPort);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->UsrPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].EncryptionOn);
				encode_binary(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].pw,24);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].IK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].IK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].IK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Iiv,8);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].OK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].OK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].OK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Oiv,8);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].SinceOpenTimer);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Uuid,16);
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].InBuffer.BlockSize);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].OutBuffer.BlockSize);
				//BUFFER InCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].InCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].InCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].InCompBuffer.BlockSize);
				//BUFFER OutCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].OutCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].OutCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].OutCompBuffer.BlockSize);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UsrPort[PortNo].setupDlg);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].TimeTillClose);
				encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].TimeOutStart);
                encode_pchar(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].ClientIP,sizeof(pmod->UsrPort[PortNo].ClientIP));
                encode_int7(mptr,(int)(mend -mptr),pmod->UsrPort[PortNo].ClientPort);
			}
			return 0;
		}
		break;
	}
	case WS_UMC:
	{
		if(pmod->UMC_PORT_VERSION==0x0101)
		{
			if((pmod->UmcPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_UMC_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x08);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].LocalIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].bindPort); //New for this version
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->UmcPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BlockSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].CfgNote,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].TimeOutSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].TimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].TimeOutPeriod);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Compressed);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Encrypted);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].PortWasActiveBeforeSuspend);
				//IPACL *IPAclList;
				//encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].IPAclList.Ip,20);
				//encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].IPAclList.Mask,20);
				//encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].IPAclList.lIp);
				//encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].IPAclList.lMask);
				//encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].IPAclList.action);
				//struct tdIPAcl *NextIPAcl;
				//char MonitorGroups[8][16];
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].tmutex); //New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].activeThread); //New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].ImmediateSendLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].RecvBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].SendBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].appClosed); // New for this version
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].pendingClose); // New for this version
				#endif
				// evert thing ,including SockActive and below,gets reset during WSUmcClosePort());
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].SockActive);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].InBuffer_Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].OutBuffer_Size);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Uuid,16);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmcPort[PortNo].Recording.RecInHnd);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmcPort[PortNo].Status,80);
			}
			return 0;
		}
		break;
	}
	case WS_UMR:
	{
		if(pmod->UMR_PORT_VERSION==0x0101)
		{
			if((pmod->UmrPort[PortNo].SockActive)||(PortNo==pmod->NO_OF_UMR_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x09);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].SockActive);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].RemoteIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].SinceLastBeatCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].UmcPort);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->UmrPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].EncryptionOn);
				encode_binary(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].pw,24);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].IK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].IK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].IK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Iiv,8);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].OK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].OK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].OK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Oiv,8);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].SinceOpenTimer);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Uuid,16);
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].InBuffer.BlockSize);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].OutBuffer.BlockSize);
				//BUFFER InCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].InCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].InCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].InCompBuffer.BlockSize);
				//BUFFER OutCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].OutCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].OutCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].OutCompBuffer.BlockSize);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UmrPort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].TimeTillClose);
				encode_int7(mptr,(int)(mend -mptr),pmod->UmrPort[PortNo].TimeOutStart);
			}
			return 0;
		}
		break;
	}
	case WS_FIL:
	{
		if(pmod->FIL_PORT_VERSION==0x0101)
		{
			if((pmod->FilePort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_FILE_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x0A);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].RemoteIP,20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].AltRemoteIP[i],20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].AltRemoteIPOn[i]);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].S5RemoteIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].S5Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].AltIPCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].CurrentAltIP);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Compressed);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Encrypted);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].S5Connect);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].S5Methode);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].S5Version);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BlockSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ChokeSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].CfgNote,80);    
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Bounce);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BounceStart);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BounceEnd);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].PortWasActiveBeforeSuspend);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].DriveList,32);    
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].setupDlg);
				//MonitorGroups[8][16);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].MinReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].MaxReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].MinReconnectReset);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ReconnectTime);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ConnectTime);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].rmutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].smutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].recvThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].sendThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ImmediateSendLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].RecvBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].SendBuffLimit); // New for this version
				// evert thing ,including SockOpen and below,gets reset during WSFileClosePort();
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].SockOpen);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].SockConnected);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].S5Status);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OnLineStatusText,80);

				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].EncryptionOn);
				encode_binary(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].pw,24);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].IK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].IK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].IK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Iiv,8);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].OK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].OK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].OK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Oiv,8);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].ReconCount);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].SinceLastBeatCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].LastDataTime);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->FilePort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].LastChokeSize);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].Uuid,16);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].MonPortDataSize);
				//struct tdMonPortData *MonPortData;
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InBuffer.BlockSize);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutBuffer.BlockSize);
				//BUFFER InCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].InCompBuffer.BlockSize);
				//BUFFER OutCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].OutCompBuffer.BlockSize);
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].IOCPSendBytes); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].peerClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].appClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->FilePort[PortNo].pendingClose); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].pendOvlRecvList); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->FilePort[PortNo].pendOvlSendList); // New for this version
				#endif
			}
			return 0;
		}
		break;
	}
	case WS_CGD:
	{
		if(pmod->CGD_PORT_VERSION==0x0101)
		{
			if((pmod->CgdPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_CGD_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x0B);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].RemoteIP,20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].AltRemoteIP[i],20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].AltRemoteIPOn[i]);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].S5RemoteIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].S5Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].AltIPCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].CurrentAltIP);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Compressed);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Encrypted);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].S5Connect);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].S5Methode);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].S5Version);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BlockSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ChokeSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].CfgNote,80);    
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Bounce);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BounceStart);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BounceEnd);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].PortWasActiveBeforeSuspend);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].setupDlg);
				//MonitorGroups[8][16);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].MinReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].MaxReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].MinReconnectReset);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ReconnectDelay);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ReconnectTime);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ConnectTime);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].rmutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].smutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].recvThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].sendThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ImmediateSendLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].RecvBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].SendBuffLimit); // New for this version
				// evert thing ,including SockOpen and below,gets reset during WSCgdClosePort();
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].SockOpen);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].SockConnected);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].S5Status);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OnLineStatusText,80);

				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].EncryptionOn);
				encode_binary(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].pw,24);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].IK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].IK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].IK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Iiv,8);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].OK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].OK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].OK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Oiv,8);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].ReconCount);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].SinceLastBeatCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].LastDataTime);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->CgdPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].LastChokeSize);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].Uuid,16);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].MonPortDataSize);
				//struct tdMonPortData *MonPortData;
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InBuffer.BlockSize);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OutBuffer.BlockSize);
				//BUFFER InCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].InCompBuffer.BlockSize);
				//BUFFER OutCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OutCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OutCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].OutCompBuffer.BlockSize);
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].IOCPSendBytes); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].peerClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].appClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CgdPort[PortNo].pendingClose); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].pendOvlRecvList); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CgdPort[PortNo].pendOvlSendList); // New for this version
				#endif
			}
			return 0;
		}
		break;
	}
	case WS_UGC:
	{
		if(pmod->UGC_PORT_VERSION==0x0101)
		{
			if((pmod->UgcPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_UGC_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x0C);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].LocalIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].bindPort); //New for this version
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->UgcPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BlockSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].CfgNote,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].TimeOutSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].TimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].TimeOutPeriod);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Compressed);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Encrypted);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].PortWasActiveBeforeSuspend);
				//char MonitorGroups[8][16];
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].tmutex); //New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].activeThread); //New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].ImmediateSendLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].RecvBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].SendBuffLimit); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].appClosed); // New for this version
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].pendingClose); // New for this version
				#endif
				// evert thing ,including SockActive and below,gets reset during WSUgcClosePort());
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].SockActive);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].InBuffer_Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].OutBuffer_Size);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Uuid,16);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgcPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgcPort[PortNo].Recording.RecInHnd);
			}
			return 0;
		}
		break;
	}
	case WS_UGR:
	{
		if(pmod->UGR_PORT_VERSION==0x0101)
		{
			if((pmod->UgrPort[PortNo].SockActive)||(PortNo==pmod->NO_OF_UGR_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x0D);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].SockActive);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].RemoteIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].SinceLastBeatCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].UgcPort);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->UgrPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].EncryptionOn);
				encode_binary(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].pw,24);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].IK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].IK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].IK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Iiv,8);
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].OK1,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].OK2,sizeof(DES_KS));
				//encode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].OK3,sizeof(DES_KS));
				encode_binary(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Oiv,8);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].SinceOpenTimer);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Uuid,16);
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].InBuffer.BlockSize);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].OutBuffer.BlockSize);
				//BUFFER InCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].InCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].InCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].InCompBuffer.BlockSize);
				//BUFFER OutCompBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].OutCompBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].OutCompBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].OutCompBuffer.BlockSize);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].Recording.RecInHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->UgrPort[PortNo].setupDlg);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].TimeTillClose);
				encode_int7(mptr,(int)(mend -mptr),pmod->UgrPort[PortNo].TimeOutStart);
			}
			return 0;
		}
		break;
	}
	case WS_CTO:
	{
		if(pmod->CTO_PORT_VERSION==0x0101)
		{
			if((pmod->CtoPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_CTO_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x0E);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].RemoteIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].cast_addr.sin_addr.s_addr);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BlockSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].CfgNote,80);    
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].PortWasActiveBeforeSuspend);
				//MonitorGroups[8][16);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].smutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].sendThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].SendBuffLimit); // New for this version
				// evert thing ,including SockActive and below,gets reset during WSCtoClosePort();
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].SockActive);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].OnLineStatusText,80);

				encode_pchar(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].LastDataTime);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->CtoPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].Uuid,16);
				//BUFFER OutBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].OutBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].OutBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].OutBuffer.BlockSize);
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].IOCPSendBytes); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].appClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtoPort[PortNo].pendingClose); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].pendOvlRecvList); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtoPort[PortNo].pendOvlSendList); // New for this version
				#endif
			}
			return 0;
		}
		break;
	}
	case WS_CTI:
	{
		if(pmod->CTI_PORT_VERSION==0x0101)
		{
			if((pmod->CtiPort[PortNo].LocalIP[0])||(PortNo==pmod->NO_OF_CTI_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x0F);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				// remember to change to size of the memset in WSCtiClosePort() if u add or change to this non reset block
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].LocalIP,20);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].RemoteIP,20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].AltRemoteIP[i],20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].AltRemoteIPOn[i]);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].AltIPCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].CurrentAltIP);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Port);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].ConnectHold);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].InUse);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BlockSize);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].CfgNote,80);    
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Bounce);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BounceStart);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BounceEnd);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].PortWasActiveBeforeSuspend);
				//WSRECORDING Recording;
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Recording.RecordPath,MAX_PATH);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Recording.DoRec);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].Recording.RecOutHnd);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].Recording.RecInHnd);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].IPAclCfg,128);
				//IPACL *IPAclList;
				//IPACLMAP IPAclMap;
				//char MonitorGroups[8][16];
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].PrefixPacketLen);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].rmutex); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].recvThread); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].RecvBuffLimit); // New for this version
				// evert thing ,including SockActive and below,gets reset during WSCtiClosePort();
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].SockActive);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].SockConnected);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].OnLineStatusText,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].ReconCount);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Status,80);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].SendTimeOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].LastDataTime);
				encode_int7(mptr,(int)(mend -mptr),(PTRCAST)pmod->CtiPort[PortNo].Sock);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Seconds);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesLastIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecMaxIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecAvgIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesLastOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecMaxOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BytesPerSecAvgOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].PacketsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].PacketsOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BlocksIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BlocksOut);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BeatsIn);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].BeatsOut);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].Uuid,16);
				//BUFFER InBuffer;
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].InBuffer.Size);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].InBuffer.LocalSize);
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].InBuffer.BlockSize);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].setupDlg);
				#ifdef WS_OIO
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].appClosed); // New for this version
				encode_int7(mptr,(int)(mend -mptr),pmod->CtiPort[PortNo].pendingClose); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].pendOvlRecvList); // New for this version
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->CtiPort[PortNo].pendOvlSendList); // New for this version
				#endif
			}
			return 0;
		}
		break;
	}
	case WS_OTH:
	{
		if(pmod->OTHER_PORT_VERSION==0x0100)
		{
			if((pmod->OtherPort[PortNo].InUse)||(PortNo==pmod->NO_OF_OTHER_PORTS)||(force))
			{
				*mptr=SMFNO_LVL2; mptr++;
				encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
				encode_pchar(mptr,(int)(mend -mptr),aclass,0);
				encode_pchar(mptr,(int)(mend -mptr),aname,0);
				encode_int7(mptr,(int)(mend -mptr),0x10);//record schema
				encode_int7(mptr,(int)(mend -mptr),PortType);
				encode_int7(mptr,(int)(mend -mptr),PortNo);
				encode_pchar(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].RemoteIP,20);
				encode_int7(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].Port);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_pchar(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].AltRemoteIP[i],20);
				for(int i=0;i<WS_MAX_ALT_PORTS;i++)
					encode_int7(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].AltRemoteIPOn[i]);
				encode_int7(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].AltIPCount);
				encode_int7(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].InUse);
				encode_pchar(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].Note,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].Status,80);
				encode_pchar(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].Recieved,50);
				encode_pchar(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].Transmit,50);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr1);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr2);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr3);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr4);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr5);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr6);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr7);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr8);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr9);
				encode_int7(mptr,(int)(mend -mptr),(DWORD)(PTRCAST)pmod->OtherPort[PortNo].DetPtr10);
				encode_binary(mptr,(int)(mend -mptr),pmod->OtherPort[PortNo].Uuid,16);
				//char MonitorGroups[8][16];
			}
			return 0;
		}
		break;
	}
	};
	return -1;
}
int SysFeedCodec::EncodeLvl2(char *&mptr, int mlen, const char *aclass, const char *aname, 
	const char *heading, const char *status, WsocksApp *pmod, bool hist, HANDLE& ihnd, bool force)
{
	const char *mstart=mptr;
	const char *mend=mptr +mlen;
	switch(HIWORD(ihnd))
	{
	// Connection frame first
	case 0:
	{
		*mptr=SMFNO_LVL2; mptr++;
		encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
		encode_pchar(mptr,(int)(mend -mptr),aclass,0);
		encode_pchar(mptr,(int)(mend -mptr),aname,0);
		encode_int7(mptr,(int)(mend -mptr),0x13);//record schema
		encode_int7(mptr,(int)(mend -mptr),-1);
		encode_int7(mptr,(int)(mend -mptr),-1);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CON_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_USC_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_USR_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UMC_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UMR_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CGD_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UGC_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_UGR_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_FILE_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CTI_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_CTO_PORTS);
		encode_int7(mptr,(int)(mend -mptr),pmod->NO_OF_OTHER_PORTS);
		encode_pchar(mptr,(int)(mend -mptr),heading,0);
		encode_pchar(mptr,(int)(mend -mptr),status,0);
		ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_CON+1);
		return 1;
	}
	// CON ports lvl2
	case WS_CON+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_CON,PortNo,hist,(char*)&pmod->ConPort[PortNo],sizeof(CONPORT),force);
		if(PortNo<pmod->NO_OF_CON_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_CON+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_USC+1);
		return 1;
	}
	// USC ports lvl2
	case WS_USC+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_USC,PortNo,hist,(char*)&pmod->UscPort[PortNo],sizeof(USCPORT),force);
		if(PortNo<pmod->NO_OF_USC_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_USC+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_USR+1);
		return 1;
	}
	// USR ports lvl2
	case WS_USR+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_USR,PortNo,hist,(char*)&pmod->UsrPort[PortNo],sizeof(USRPORT),force);
		if(PortNo<pmod->NO_OF_USR_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_USR+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_UMC+1);
		return 1;
	}
	// UMC ports lvl2
	case WS_UMC+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_UMC,PortNo,hist,(char*)&pmod->UmcPort[PortNo],sizeof(UMCPORT),force);
		if(PortNo<pmod->NO_OF_UMC_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_UMC+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_UMR+1);
		return 1;
	}
	// UMR ports lvl2
	case WS_UMR+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_UMR,PortNo,hist,(char*)&pmod->UmrPort[PortNo],sizeof(UMRPORT),force);
		if(PortNo<pmod->NO_OF_UMR_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_UMR+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_FIL+1);
		return 1;
	}
	// FIL ports lvl2
	case WS_FIL+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_FIL,PortNo,hist,(char*)&pmod->FilePort[PortNo],sizeof(FILEPORT),force);
		if(PortNo<pmod->NO_OF_FILE_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_FIL+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_CGD+1);
		return 1;
	}
	// CGD ports lvl2
	case WS_CGD+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_CGD,PortNo,hist,(char*)&pmod->CgdPort[PortNo],sizeof(CGDPORT),force);
		if(PortNo<pmod->NO_OF_CGD_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_CGD+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_UGC+1);
		return 1;
	}
	// UGC ports lvl2
	case WS_UGC+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_UGC,PortNo,hist,(char*)&pmod->UgcPort[PortNo],sizeof(UGCPORT),force);
		if(PortNo<pmod->NO_OF_UGC_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_UGC+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_UGR+1);
		return 1;
	}
	// UGR ports lvl2
	case WS_UGR+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_UGR,PortNo,hist,(char*)&pmod->UgrPort[PortNo],sizeof(UGRPORT),force);
		if(PortNo<pmod->NO_OF_UGR_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_UGR+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_CTO+1);
		return 1;
	}
	// CTO ports lvl2
	case WS_CTO+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_CTO,PortNo,hist,(char*)&pmod->CtoPort[PortNo],sizeof(CTOPORT),force);
		if(PortNo<pmod->NO_OF_CTO_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_CTO+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_CTI+1);
		return 1;
	}
	// CTI ports lvl2
	case WS_CTI+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_CTI,PortNo,hist,(char*)&pmod->CtiPort[PortNo],sizeof(CTIPORT),force);
		if(PortNo<pmod->NO_OF_CTI_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_CTI+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(0,WS_OTH+1);
		return 1;
	}
	// OTH ports lvl2
	case WS_OTH+1:
	{
		int PortNo=LOWORD(ihnd);
		EncodeLvl2(mptr,mlen,aclass,aname,pmod,WS_OTH,PortNo,hist,(char*)&pmod->OtherPort[PortNo],sizeof(OTHERPORT),force);
		if(PortNo<pmod->NO_OF_OTHER_PORTS)
			ihnd=(HANDLE)(LONGLONG)MAKELONG(PortNo+1,WS_OTH+1);
		else
			ihnd=(HANDLE)(LONGLONG)MAKELONG(-1,-1);
		return 1;
	}
	// End of update
	case 0xFFFF:
		return 0;
	};
	return -1;
}
int SysFeedCodec::DecodeLvl2(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata)
{
	const char *mend=mptr +mlen;
	bool hist=false;
	decode_int7(hist,mptr,mend);
	char aclass[256]={0},aname[256]={0};
	decode_pchar(aclass,sizeof(aclass),mptr,mend);
	decode_pchar(aname,sizeof(aname),mptr,mend);
	unsigned char rectype=0;
	decode_int7(rectype,mptr,mend);
	WSPortType PortType=WS_UNKNOWN;
	int PortNo=-1;
	switch(rectype)
	{
	case 0x13: //Lvl2 frame
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		__int64 nports[14];
		decode_int7(nports[0],mptr,mend);
		decode_int7(nports[1],mptr,mend);
		decode_int7(nports[2],mptr,mend);
		decode_int7(nports[3],mptr,mend);
		decode_int7(nports[4],mptr,mend);
		decode_int7(nports[5],mptr,mend);
		decode_int7(nports[6],mptr,mend);
		decode_int7(nports[7],mptr,mend);
		decode_int7(nports[8],mptr,mend);
		decode_int7(nports[9],mptr,mend);
		decode_int7(nports[10],mptr,mend);
		decode_int7(nports[11],mptr,mend);
		char heading[256]={0};
		decode_pchar(heading,sizeof(heading),mptr,mend);
		nports[12]=(PTRCAST)heading;
		char status[256]={0};
		decode_pchar(status,sizeof(status),mptr,mend);
		nports[13]=(PTRCAST)status;
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)nports,sizeof(nports),udata);
		break;
	}
	case 0x05: //WS_CON lvl2 (replaced by 0x17)
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CONPORT cport;
		memset(&cport,0,sizeof(CONPORT));
		decode_pchar(cport.LocalIP,20,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(cport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemoteIPOn[i],mptr,mend);
		decode_pchar(cport.S5RemoteIP,20,mptr,mend);
		decode_int7(cport.S5Port,mptr,mend);
		decode_int7(cport.AltIPCount,mptr,mend);
		decode_int7(cport.CurrentAltIP,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_int7(cport.ConnectHold,mptr,mend);
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.Compressed,mptr,mend);
		decode_int7(cport.CompressType,mptr,mend);
		decode_int7(cport.Encrypted,mptr,mend);
		decode_int7(cport.S5Connect,mptr,mend);
		decode_int7(cport.S5Methode,mptr,mend);
		decode_int7(cport.S5Version,mptr,mend);
		decode_int7(cport.BlockSize,mptr,mend);
		decode_int7(cport.ChokeSize,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);    
		decode_int7(cport.Bounce,mptr,mend);
		decode_int7(cport.BounceStart,mptr,mend);
		decode_int7(cport.BounceEnd,mptr,mend);
		decode_int7(cport.PortWasActiveBeforeSuspend,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(cport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(cport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)cport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)cport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)cport.setupDlg,mptr,mend);
		//MonitorGroups[8][16,mptr,mend);
		decode_int7(cport.MinReconnectDelay,mptr,mend);
		decode_int7(cport.MaxReconnectDelay,mptr,mend);
		decode_int7(cport.MinReconnectReset,mptr,mend);
		decode_int7(cport.ReconnectDelay,mptr,mend);
		decode_int7(cport.ReconnectTime,mptr,mend);
		decode_int7(cport.ConnectTime,mptr,mend);
		decode_int7((DWORD&)cport.rmutex,mptr,mend); // New for this version
		decode_int7((DWORD&)cport.smutex,mptr,mend); // New for this version
		decode_int7(cport.recvThread,mptr,mend); // New for this version
		decode_int7(cport.sendThread,mptr,mend); // New for this version
		decode_int7(cport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(cport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(cport.SendBuffLimit,mptr,mend); // New for this version
		// evert thing ,including SockOpen and below,gets reset during WSConClosePort(,mptr,mend);
		decode_int7(cport.SockOpen,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_int7(cport.S5Status,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);

		decode_int7(cport.EncryptionOn,mptr,mend);
		decode_binary((char*)cport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)cport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)cport.Oiv,8,mptr,mend);
		decode_int7(cport.ReconCount,mptr,mend);
		decode_pchar(cport.Note,80,mptr,mend);
		decode_pchar(cport.Status,80,mptr,mend);
		decode_int7(cport.SendTimeOut,mptr,mend);
		decode_int7(cport.SinceLastBeatCount,mptr,mend);
		decode_int7(cport.LastDataTime,mptr,mend);
		decode_int7((LONGLONG&)cport.Sock,mptr,mend);
		decode_int7(cport.Seconds,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BytesLastIn,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesOut,mptr,mend);
		decode_int7(cport.BytesLastOut,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.LastChokeSize,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr10,mptr,mend);
		decode_binary(cport.Uuid,16,mptr,mend);
		decode_int7(cport.MonPortDataSize,mptr,mend);
		//struct tdMonPortData *MonPortData;
		//BUFFER InBuffer;
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.InBuffer.LocalSize,mptr,mend);
		decode_int7(cport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(cport.OutBuffer.Size,mptr,mend);
		decode_int7(cport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(cport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(cport.InCompBuffer.Size,mptr,mend);
		decode_int7(cport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(cport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(cport.OutCompBuffer.Size,mptr,mend);
		decode_int7(cport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(cport.OutCompBuffer.BlockSize,mptr,mend);
		decode_int7(cport.lastReadyStart,mptr,mend); // New for this version
		#ifdef WS_OIO
		decode_int7(cport.IOCPSendBytes,mptr,mend); // New for this version
		decode_int7(cport.peerClosed,mptr,mend); // New for this version
		decode_int7(cport.appClosed,mptr,mend); // New for this version
		decode_int7(cport.pendingClose,mptr,mend); // New for this version
		decode_int7((DWORD&)cport.pendOvlRecvList,mptr,mend); // New for this version
		decode_int7((DWORD&)cport.pendOvlSendList,mptr,mend); // New for this version
		#endif
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CONPORT),udata);
		break;
	}
	case 0x17: //WS_CON lvl2 (replaces 0x05)
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CONPORT cport;
		memset(&cport,0,sizeof(CONPORT));
		decode_pchar(cport.LocalIP,20,mptr,mend);
		decode_pchar(cport.RemoteIP,20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(cport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemoteIPOn[i],mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(cport.AltRemotePort[i],mptr,mend);
		decode_pchar(cport.S5RemoteIP,20,mptr,mend);
		decode_int7(cport.S5Port,mptr,mend);
		decode_int7(cport.AltIPCount,mptr,mend);
		decode_int7(cport.CurrentAltIP,mptr,mend);
		decode_int7(cport.Port,mptr,mend);
		decode_int7(cport.ConnectHold,mptr,mend);
		decode_int7(cport.InUse,mptr,mend);
		decode_int7(cport.Compressed,mptr,mend);
		decode_int7(cport.CompressType,mptr,mend);
		decode_int7(cport.Encrypted,mptr,mend);
		decode_int7(cport.S5Connect,mptr,mend);
		decode_int7(cport.S5Methode,mptr,mend);
		decode_int7(cport.S5Version,mptr,mend);
		decode_int7(cport.BlockSize,mptr,mend);
		decode_int7(cport.ChokeSize,mptr,mend);
		decode_pchar(cport.CfgNote,80,mptr,mend);    
		decode_int7(cport.Bounce,mptr,mend);
		decode_int7(cport.BounceStart,mptr,mend);
		decode_int7(cport.BounceEnd,mptr,mend);
		decode_int7(cport.PortWasActiveBeforeSuspend,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(cport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(cport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)cport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)cport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)cport.setupDlg,mptr,mend);
		//MonitorGroups[8][16,mptr,mend);
		decode_int7(cport.MinReconnectDelay,mptr,mend);
		decode_int7(cport.MaxReconnectDelay,mptr,mend);
		decode_int7(cport.MinReconnectReset,mptr,mend);
		decode_int7(cport.ReconnectDelay,mptr,mend);
		decode_int7(cport.ReconnectTime,mptr,mend);
		decode_int7(cport.ConnectTime,mptr,mend);
		decode_int7((DWORD&)cport.rmutex,mptr,mend); // New for this version
		decode_int7((DWORD&)cport.smutex,mptr,mend); // New for this version
		decode_int7(cport.recvThread,mptr,mend); // New for this version
		decode_int7(cport.sendThread,mptr,mend); // New for this version
		decode_int7(cport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(cport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(cport.SendBuffLimit,mptr,mend); // New for this version
		// evert thing ,including SockOpen and below,gets reset during WSConClosePort(,mptr,mend);
		decode_int7(cport.SockOpen,mptr,mend);
		decode_int7(cport.SockConnected,mptr,mend);
		decode_int7(cport.S5Status,mptr,mend);
		decode_pchar(cport.OnLineStatusText,80,mptr,mend);

		decode_int7(cport.EncryptionOn,mptr,mend);
		decode_binary((char*)cport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)cport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->ConPort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)cport.Oiv,8,mptr,mend);
		decode_int7(cport.ReconCount,mptr,mend);
		decode_pchar(cport.Note,80,mptr,mend);
		decode_pchar(cport.Status,80,mptr,mend);
		decode_int7(cport.SendTimeOut,mptr,mend);
		decode_int7(cport.SinceLastBeatCount,mptr,mend);
		decode_int7(cport.LastDataTime,mptr,mend);
		decode_int7((LONGLONG&)cport.Sock,mptr,mend);
		decode_int7(cport.Seconds,mptr,mend);
		decode_int7(cport.BytesIn,mptr,mend);
		decode_int7(cport.BytesLastIn,mptr,mend);
		decode_int7(cport.BytesPerSecIn,mptr,mend);
		decode_int7(cport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(cport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(cport.BytesOut,mptr,mend);
		decode_int7(cport.BytesLastOut,mptr,mend);
		decode_int7(cport.BytesPerSecOut,mptr,mend);
		decode_int7(cport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(cport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(cport.PacketsIn,mptr,mend);
		decode_int7(cport.PacketsOut,mptr,mend);
		decode_int7(cport.BlocksIn,mptr,mend);
		decode_int7(cport.BlocksOut,mptr,mend);
		decode_int7(cport.BeatsIn,mptr,mend);
		decode_int7(cport.BeatsOut,mptr,mend);
		decode_int7(cport.LastChokeSize,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)cport.DetPtr10,mptr,mend);
		decode_binary(cport.Uuid,16,mptr,mend);
		decode_int7(cport.MonPortDataSize,mptr,mend);
		//struct tdMonPortData *MonPortData;
		//BUFFER InBuffer;
		decode_int7(cport.InBuffer.Size,mptr,mend);
		decode_int7(cport.InBuffer.LocalSize,mptr,mend);
		decode_int7(cport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(cport.OutBuffer.Size,mptr,mend);
		decode_int7(cport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(cport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(cport.InCompBuffer.Size,mptr,mend);
		decode_int7(cport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(cport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(cport.OutCompBuffer.Size,mptr,mend);
		decode_int7(cport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(cport.OutCompBuffer.BlockSize,mptr,mend);
		decode_int7(cport.lastReadyStart,mptr,mend); // New for this version
		#ifdef WS_OIO
		decode_int7(cport.IOCPSendBytes,mptr,mend); // New for this version
		decode_int7(cport.peerClosed,mptr,mend); // New for this version
		decode_int7(cport.appClosed,mptr,mend); // New for this version
		decode_int7(cport.pendingClose,mptr,mend); // New for this version
		decode_int7((DWORD&)cport.pendOvlRecvList,mptr,mend); // New for this version
		decode_int7((DWORD&)cport.pendOvlSendList,mptr,mend); // New for this version
		#endif
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&cport,sizeof(CONPORT),udata);
		break;
	}
	case 0x06: //WS_USC lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		USCPORT uport;
		memset(&uport,0,sizeof(USCPORT));
		decode_pchar(uport.LocalIP,20,mptr,mend);
		decode_int7(uport.Port,mptr,mend);
		decode_int7(uport.bindPort,mptr,mend); //New for this version
		decode_int7((LONGLONG&)uport.Sock,mptr,mend);
		decode_int7(uport.ConnectHold,mptr,mend);
		decode_int7(uport.InUse,mptr,mend);
		decode_int7(uport.BlockSize,mptr,mend);
		decode_pchar(uport.CfgNote,80,mptr,mend);
		decode_int7(uport.TimeOutSize,mptr,mend);
		decode_int7(uport.TimeOut,mptr,mend);
		decode_int7(uport.TimeOutPeriod,mptr,mend);
		decode_int7(uport.Compressed,mptr,mend);
		decode_int7(uport.CompressType,mptr,mend);
		decode_int7(uport.Encrypted,mptr,mend);
		decode_int7(uport.PortWasActiveBeforeSuspend,mptr,mend);
		decode_pchar(uport.IPAclCfg,128,mptr,mend);
		//IPACL *IPAclList;
		//decode_pchar(uport.IPAclList.Ip,20,mptr,mend);
		//decode_pchar(uport.IPAclList.Mask,20,mptr,mend);
		//decode_int7(uport.IPAclList.lIp,mptr,mend);
		//decode_int7(uport.IPAclList.lMask,mptr,mend);
		//decode_int7(uport.IPAclList.action,mptr,mend);
		//struct tdIPAcl *NextIPAcl;
		//char MonitorGroups[8][16];
		decode_int7((DWORD&)uport.tmutex,mptr,mend); //New for this version
		decode_int7(uport.activeThread,mptr,mend); //New for this version
		decode_int7(uport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(uport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(uport.SendBuffLimit,mptr,mend); // New for this version
		decode_int7(uport.appClosed,mptr,mend); // New for this version
		#ifdef WS_OIO
		decode_int7(uport.pendingClose,mptr,mend); // New for this version
		#endif
		// evert thing ,including SockActive and below,gets reset during WSUscClosePort(),mptr,mend);
		decode_int7(uport.SockActive,mptr,mend);
		decode_int7(uport.Seconds,mptr,mend);
		decode_int7(uport.BytesIn,mptr,mend);
		decode_int7(uport.BytesLastIn,mptr,mend);
		decode_int7(uport.BytesPerSecIn,mptr,mend);
		decode_int7(uport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(uport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(uport.BytesOut,mptr,mend);
		decode_int7(uport.BytesLastOut,mptr,mend);
		decode_int7(uport.BytesPerSecOut,mptr,mend);
		decode_int7(uport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(uport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(uport.PacketsIn,mptr,mend);
		decode_int7(uport.PacketsOut,mptr,mend);
		decode_int7(uport.BlocksIn,mptr,mend);
		decode_int7(uport.BlocksOut,mptr,mend);
		decode_int7(uport.BeatsIn,mptr,mend);
		decode_int7(uport.BeatsOut,mptr,mend);
		decode_int7(uport.InBuffer_Size,mptr,mend);
		decode_int7(uport.OutBuffer_Size,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr10,mptr,mend);
		decode_binary(uport.Uuid,16,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(uport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(uport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)uport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)uport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)uport.setupDlg,mptr,mend);
		decode_pchar(uport.Status,80,mptr,mend);
		decode_int7(uport.CfgNoteDirty,mptr,mend);
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&uport,sizeof(USCPORT),udata);
		break;
	}
	case 0x07: //WS_USR lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		USRPORT uport;
		memset(&uport,0,sizeof(USRPORT));
		decode_int7(uport.SockActive,mptr,mend);
		decode_pchar(uport.RemoteIP,20,mptr,mend);
		decode_pchar(uport.LocalIP,20,mptr,mend);
		decode_pchar(uport.Note,80,mptr,mend);
		decode_pchar(uport.Status,80,mptr,mend);
		decode_int7(uport.SendTimeOut,mptr,mend);
		decode_int7(uport.SinceLastBeatCount,mptr,mend);
		decode_int7(uport.UscPort,mptr,mend);
		decode_int7((LONGLONG&)uport.Sock,mptr,mend);
		decode_int7(uport.Seconds,mptr,mend);
		decode_int7(uport.EncryptionOn,mptr,mend);
		decode_binary((char*)uport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)uport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UsrPort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)uport.Oiv,8,mptr,mend);
		decode_int7(uport.Seconds,mptr,mend);
		decode_int7(uport.BytesIn,mptr,mend);
		decode_int7(uport.BytesLastIn,mptr,mend);
		decode_int7(uport.BytesPerSecIn,mptr,mend);
		decode_int7(uport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(uport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(uport.BytesOut,mptr,mend);
		decode_int7(uport.BytesLastOut,mptr,mend);
		decode_int7(uport.BytesPerSecOut,mptr,mend);
		decode_int7(uport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(uport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(uport.PacketsIn,mptr,mend);
		decode_int7(uport.PacketsOut,mptr,mend);
		decode_int7(uport.BlocksIn,mptr,mend);
		decode_int7(uport.BlocksOut,mptr,mend);
		decode_int7(uport.BeatsIn,mptr,mend);
		decode_int7(uport.BeatsOut,mptr,mend);
		decode_int7(uport.SinceOpenTimer,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)uport.DetPtr10,mptr,mend);
		decode_binary(uport.Uuid,16,mptr,mend);
		//BUFFER InBuffer;
		decode_int7(uport.InBuffer.Size,mptr,mend);
		decode_int7(uport.InBuffer.LocalSize,mptr,mend);
		decode_int7(uport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(uport.OutBuffer.Size,mptr,mend);
		decode_int7(uport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(uport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(uport.InCompBuffer.Size,mptr,mend);
		decode_int7(uport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(uport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(uport.OutCompBuffer.Size,mptr,mend);
		decode_int7(uport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(uport.OutCompBuffer.BlockSize,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(uport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(uport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)uport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)uport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)uport.setupDlg,mptr,mend);
		decode_int7(uport.TimeTillClose,mptr,mend);
		decode_int7(uport.TimeOutStart,mptr,mend);
        if(mptr < mend)
        {
            decode_pchar(uport.ClientIP,sizeof(uport.ClientIP),mptr,mend);
            decode_int7(uport.ClientPort,mptr,mend);
        }
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&uport,sizeof(USRPORT),udata);
		break;
	}
	case 0x08: //WS_UMC lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		UMCPORT mport;
		memset(&mport,0,sizeof(UMCPORT));
		decode_pchar(mport.LocalIP,20,mptr,mend);
		decode_int7(mport.Port,mptr,mend);
		decode_int7(mport.bindPort,mptr,mend); //New for this version
		decode_int7((LONGLONG&)mport.Sock,mptr,mend);
		decode_int7(mport.ConnectHold,mptr,mend);
		decode_int7(mport.InUse,mptr,mend);
		decode_int7(mport.BlockSize,mptr,mend);
		decode_pchar(mport.CfgNote,80,mptr,mend);
		decode_int7(mport.TimeOutSize,mptr,mend);
		decode_int7(mport.TimeOut,mptr,mend);
		decode_int7(mport.TimeOutPeriod,mptr,mend);
		decode_int7(mport.Compressed,mptr,mend);
		decode_int7(mport.Encrypted,mptr,mend);
		decode_int7(mport.PortWasActiveBeforeSuspend,mptr,mend);
		//IPACL *IPAclList;
		//decode_pchar(mport.IPAclList.Ip,20,mptr,mend);
		//decode_pchar(mport.IPAclList.Mask,20,mptr,mend);
		//decode_int7(mport.IPAclList.lIp,mptr,mend);
		//decode_int7(mport.IPAclList.lMask,mptr,mend);
		//decode_int7(mport.IPAclList.action,mptr,mend);
		//struct tdIPAcl *NextIPAcl;
		//char MonitorGroups[8][16];
		decode_int7((DWORD&)mport.tmutex,mptr,mend); //New for this version
		decode_int7(mport.activeThread,mptr,mend); //New for this version
		decode_int7(mport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(mport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(mport.SendBuffLimit,mptr,mend); // New for this version
		decode_int7(mport.appClosed,mptr,mend); // New for this version
		#ifdef WS_OIO
		decode_int7(mport.pendingClose,mptr,mend); // New for this version
		#endif
		// evert thing ,including SockActive and below,gets reset during WSUmcClosePort(),mptr,mend);
		decode_int7(mport.SockActive,mptr,mend);
		decode_int7(mport.Seconds,mptr,mend);
		decode_int7(mport.BytesIn,mptr,mend);
		decode_int7(mport.BytesLastIn,mptr,mend);
		decode_int7(mport.BytesPerSecIn,mptr,mend);
		decode_int7(mport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(mport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(mport.BytesOut,mptr,mend);
		decode_int7(mport.BytesLastOut,mptr,mend);
		decode_int7(mport.BytesPerSecOut,mptr,mend);
		decode_int7(mport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(mport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(mport.PacketsIn,mptr,mend);
		decode_int7(mport.PacketsOut,mptr,mend);
		decode_int7(mport.BlocksIn,mptr,mend);
		decode_int7(mport.BlocksOut,mptr,mend);
		decode_int7(mport.BeatsIn,mptr,mend);
		decode_int7(mport.BeatsOut,mptr,mend);
		decode_int7(mport.InBuffer_Size,mptr,mend);
		decode_int7(mport.OutBuffer_Size,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr10,mptr,mend);
		decode_binary(mport.Uuid,16,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(mport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(mport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)mport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)mport.Recording.RecInHnd,mptr,mend);
		decode_pchar(mport.Status,80,mptr,mend);
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&mport,sizeof(UMCPORT),udata);
		break;
	}
	case 0x09: //WS_UMR lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		UMRPORT mport;
		memset(&mport,0,sizeof(UMRPORT));
		decode_int7(mport.SockActive,mptr,mend);
		decode_pchar(mport.RemoteIP,20,mptr,mend);
		decode_pchar(mport.LocalIP,20,mptr,mend);
		decode_pchar(mport.Note,80,mptr,mend);
		decode_pchar(mport.Status,80,mptr,mend);
		decode_int7(mport.SendTimeOut,mptr,mend);
		decode_int7(mport.SinceLastBeatCount,mptr,mend);
		decode_int7(mport.UmcPort,mptr,mend);
		decode_int7((LONGLONG&)mport.Sock,mptr,mend);
		decode_int7(mport.Seconds,mptr,mend);
		decode_int7(mport.EncryptionOn,mptr,mend);
		decode_binary((char*)mport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)mport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UmrPort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)mport.Oiv,8,mptr,mend);
		decode_int7(mport.Seconds,mptr,mend);
		decode_int7(mport.BytesIn,mptr,mend);
		decode_int7(mport.BytesLastIn,mptr,mend);
		decode_int7(mport.BytesPerSecIn,mptr,mend);
		decode_int7(mport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(mport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(mport.BytesOut,mptr,mend);
		decode_int7(mport.BytesLastOut,mptr,mend);
		decode_int7(mport.BytesPerSecOut,mptr,mend);
		decode_int7(mport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(mport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(mport.PacketsIn,mptr,mend);
		decode_int7(mport.PacketsOut,mptr,mend);
		decode_int7(mport.BlocksIn,mptr,mend);
		decode_int7(mport.BlocksOut,mptr,mend);
		decode_int7(mport.BeatsIn,mptr,mend);
		decode_int7(mport.BeatsOut,mptr,mend);
		decode_int7(mport.SinceOpenTimer,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)mport.DetPtr10,mptr,mend);
		decode_binary(mport.Uuid,16,mptr,mend);
		//BUFFER InBuffer;
		decode_int7(mport.InBuffer.Size,mptr,mend);
		decode_int7(mport.InBuffer.LocalSize,mptr,mend);
		decode_int7(mport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(mport.OutBuffer.Size,mptr,mend);
		decode_int7(mport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(mport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(mport.InCompBuffer.Size,mptr,mend);
		decode_int7(mport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(mport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(mport.OutCompBuffer.Size,mptr,mend);
		decode_int7(mport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(mport.OutCompBuffer.BlockSize,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(mport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(mport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)mport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)mport.Recording.RecInHnd,mptr,mend);
		decode_int7(mport.TimeTillClose,mptr,mend);
		decode_int7(mport.TimeOutStart,mptr,mend);
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&mport,sizeof(UMRPORT),udata);
		break;
	}
	case 0x0A: //WS_FIL lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		FILEPORT fport;
		memset(&fport,0,sizeof(FILEPORT));
		decode_pchar(fport.LocalIP,20,mptr,mend);
		decode_pchar(fport.RemoteIP,20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(fport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(fport.AltRemoteIPOn[i],mptr,mend);
		decode_pchar(fport.S5RemoteIP,20,mptr,mend);
		decode_int7(fport.S5Port,mptr,mend);
		decode_int7(fport.AltIPCount,mptr,mend);
		decode_int7(fport.CurrentAltIP,mptr,mend);
		decode_int7(fport.Port,mptr,mend);
		decode_int7(fport.ConnectHold,mptr,mend);
		decode_int7(fport.InUse,mptr,mend);
		decode_int7(fport.Compressed,mptr,mend);
		decode_int7(fport.Encrypted,mptr,mend);
		decode_int7(fport.S5Connect,mptr,mend);
		decode_int7(fport.S5Methode,mptr,mend);
		decode_int7(fport.S5Version,mptr,mend);
		decode_int7(fport.BlockSize,mptr,mend);
		decode_int7(fport.ChokeSize,mptr,mend);
		decode_pchar(fport.CfgNote,80,mptr,mend);    
		decode_int7(fport.Bounce,mptr,mend);
		decode_int7(fport.BounceStart,mptr,mend);
		decode_int7(fport.BounceEnd,mptr,mend);
		decode_int7(fport.PortWasActiveBeforeSuspend,mptr,mend);
		decode_pchar(fport.DriveList,32,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(fport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(fport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)fport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)fport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)fport.setupDlg,mptr,mend);
		//MonitorGroups[8][16,mptr,mend);
		decode_int7(fport.MinReconnectDelay,mptr,mend);
		decode_int7(fport.MaxReconnectDelay,mptr,mend);
		decode_int7(fport.MinReconnectReset,mptr,mend);
		decode_int7(fport.ReconnectDelay,mptr,mend);
		decode_int7(fport.ReconnectTime,mptr,mend);
		decode_int7(fport.ConnectTime,mptr,mend);
		decode_int7((DWORD&)fport.rmutex,mptr,mend); // New for this version
		decode_int7((DWORD&)fport.smutex,mptr,mend); // New for this version
		decode_int7(fport.recvThread,mptr,mend); // New for this version
		decode_int7(fport.sendThread,mptr,mend); // New for this version
		decode_int7(fport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(fport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(fport.SendBuffLimit,mptr,mend); // New for this version
		// evert thing ,including SockOpen and below,gets reset during WSFileClosePort(,mptr,mend);
		decode_int7(fport.SockOpen,mptr,mend);
		decode_int7(fport.SockConnected,mptr,mend);
		decode_int7(fport.S5Status,mptr,mend);
		decode_pchar(fport.OnLineStatusText,80,mptr,mend);

		decode_int7(fport.EncryptionOn,mptr,mend);
		decode_binary((char*)fport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)fport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->FilePort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)fport.Oiv,8,mptr,mend);
		decode_int7(fport.ReconCount,mptr,mend);
		decode_pchar(fport.Note,80,mptr,mend);
		decode_pchar(fport.Status,80,mptr,mend);
		decode_int7(fport.SendTimeOut,mptr,mend);
		decode_int7(fport.SinceLastBeatCount,mptr,mend);
		decode_int7(fport.LastDataTime,mptr,mend);
		decode_int7((LONGLONG&)fport.Sock,mptr,mend);
		decode_int7(fport.Seconds,mptr,mend);
		decode_int7(fport.BytesIn,mptr,mend);
		decode_int7(fport.BytesLastIn,mptr,mend);
		decode_int7(fport.BytesPerSecIn,mptr,mend);
		decode_int7(fport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(fport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(fport.BytesOut,mptr,mend);
		decode_int7(fport.BytesLastOut,mptr,mend);
		decode_int7(fport.BytesPerSecOut,mptr,mend);
		decode_int7(fport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(fport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(fport.PacketsIn,mptr,mend);
		decode_int7(fport.PacketsOut,mptr,mend);
		decode_int7(fport.BlocksIn,mptr,mend);
		decode_int7(fport.BlocksOut,mptr,mend);
		decode_int7(fport.BeatsIn,mptr,mend);
		decode_int7(fport.BeatsOut,mptr,mend);
		decode_int7(fport.LastChokeSize,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)fport.DetPtr10,mptr,mend);
		decode_binary(fport.Uuid,16,mptr,mend);
		decode_int7(fport.MonPortDataSize,mptr,mend);
		//struct tdMonPortData *MonPortData;
		//BUFFER InBuffer;
		decode_int7(fport.InBuffer.Size,mptr,mend);
		decode_int7(fport.InBuffer.LocalSize,mptr,mend);
		decode_int7(fport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(fport.OutBuffer.Size,mptr,mend);
		decode_int7(fport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(fport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(fport.InCompBuffer.Size,mptr,mend);
		decode_int7(fport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(fport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(fport.OutCompBuffer.Size,mptr,mend);
		decode_int7(fport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(fport.OutCompBuffer.BlockSize,mptr,mend);
		#ifdef WS_OIO
		decode_int7(fport.IOCPSendBytes,mptr,mend); // New for this version
		decode_int7(fport.peerClosed,mptr,mend); // New for this version
		decode_int7(fport.appClosed,mptr,mend); // New for this version
		decode_int7(fport.pendingClose,mptr,mend); // New for this version
		decode_int7((DWORD&)fport.pendOvlRecvList,mptr,mend); // New for this version
		decode_int7((DWORD&)fport.pendOvlSendList,mptr,mend); // New for this version
		#endif
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&fport,sizeof(FILEPORT),udata);
		break;
	}
	case 0x0B: //WS_CGD lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CGDPORT dport;
		memset(&dport,0,sizeof(CGDPORT));
		decode_pchar(dport.LocalIP,20,mptr,mend);
		decode_pchar(dport.RemoteIP,20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(dport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(dport.AltRemoteIPOn[i],mptr,mend);
		decode_pchar(dport.S5RemoteIP,20,mptr,mend);
		decode_int7(dport.S5Port,mptr,mend);
		decode_int7(dport.AltIPCount,mptr,mend);
		decode_int7(dport.CurrentAltIP,mptr,mend);
		decode_int7(dport.Port,mptr,mend);
		decode_int7(dport.ConnectHold,mptr,mend);
		decode_int7(dport.InUse,mptr,mend);
		decode_int7(dport.Compressed,mptr,mend);
		decode_int7(dport.Encrypted,mptr,mend);
		decode_int7(dport.S5Connect,mptr,mend);
		decode_int7(dport.S5Methode,mptr,mend);
		decode_int7(dport.S5Version,mptr,mend);
		decode_int7(dport.BlockSize,mptr,mend);
		decode_int7(dport.ChokeSize,mptr,mend);
		decode_pchar(dport.CfgNote,80,mptr,mend);    
		decode_int7(dport.Bounce,mptr,mend);
		decode_int7(dport.BounceStart,mptr,mend);
		decode_int7(dport.BounceEnd,mptr,mend);
		decode_int7(dport.PortWasActiveBeforeSuspend,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(dport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(dport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)dport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)dport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)dport.setupDlg,mptr,mend);
		//MonitorGroups[8][16,mptr,mend);
		decode_int7(dport.MinReconnectDelay,mptr,mend);
		decode_int7(dport.MaxReconnectDelay,mptr,mend);
		decode_int7(dport.MinReconnectReset,mptr,mend);
		decode_int7(dport.ReconnectDelay,mptr,mend);
		decode_int7(dport.ReconnectTime,mptr,mend);
		decode_int7(dport.ConnectTime,mptr,mend);
		decode_int7((DWORD&)dport.rmutex,mptr,mend); // New for this version
		decode_int7((DWORD&)dport.smutex,mptr,mend); // New for this version
		decode_int7(dport.recvThread,mptr,mend); // New for this version
		decode_int7(dport.sendThread,mptr,mend); // New for this version
		decode_int7(dport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(dport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(dport.SendBuffLimit,mptr,mend); // New for this version
		// evert thing ,including SockOpen and below,gets reset during WSCgdClosePort(,mptr,mend);
		decode_int7(dport.SockOpen,mptr,mend);
		decode_int7(dport.SockConnected,mptr,mend);
		decode_int7(dport.S5Status,mptr,mend);
		decode_pchar(dport.OnLineStatusText,80,mptr,mend);

		decode_int7(dport.EncryptionOn,mptr,mend);
		decode_binary((char*)dport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)dport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->CgdPort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)dport.Oiv,8,mptr,mend);
		decode_int7(dport.ReconCount,mptr,mend);
		decode_pchar(dport.Note,80,mptr,mend);
		decode_pchar(dport.Status,80,mptr,mend);
		decode_int7(dport.SendTimeOut,mptr,mend);
		decode_int7(dport.SinceLastBeatCount,mptr,mend);
		decode_int7(dport.LastDataTime,mptr,mend);
		decode_int7((LONGLONG&)dport.Sock,mptr,mend);
		decode_int7(dport.Seconds,mptr,mend);
		decode_int7(dport.BytesIn,mptr,mend);
		decode_int7(dport.BytesLastIn,mptr,mend);
		decode_int7(dport.BytesPerSecIn,mptr,mend);
		decode_int7(dport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(dport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(dport.BytesOut,mptr,mend);
		decode_int7(dport.BytesLastOut,mptr,mend);
		decode_int7(dport.BytesPerSecOut,mptr,mend);
		decode_int7(dport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(dport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(dport.PacketsIn,mptr,mend);
		decode_int7(dport.PacketsOut,mptr,mend);
		decode_int7(dport.BlocksIn,mptr,mend);
		decode_int7(dport.BlocksOut,mptr,mend);
		decode_int7(dport.BeatsIn,mptr,mend);
		decode_int7(dport.BeatsOut,mptr,mend);
		decode_int7(dport.LastChokeSize,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)dport.DetPtr10,mptr,mend);
		decode_binary(dport.Uuid,16,mptr,mend);
		decode_int7(dport.MonPortDataSize,mptr,mend);
		//struct tdMonPortData *MonPortData;
		//BUFFER InBuffer;
		decode_int7(dport.InBuffer.Size,mptr,mend);
		decode_int7(dport.InBuffer.LocalSize,mptr,mend);
		decode_int7(dport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(dport.OutBuffer.Size,mptr,mend);
		decode_int7(dport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(dport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(dport.InCompBuffer.Size,mptr,mend);
		decode_int7(dport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(dport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(dport.OutCompBuffer.Size,mptr,mend);
		decode_int7(dport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(dport.OutCompBuffer.BlockSize,mptr,mend);
		#ifdef WS_OIO
		decode_int7(dport.IOCPSendBytes,mptr,mend); // New for this version
		decode_int7(dport.peerClosed,mptr,mend); // New for this version
		decode_int7(dport.appClosed,mptr,mend); // New for this version
		decode_int7(dport.pendingClose,mptr,mend); // New for this version
		decode_int7((DWORD&)dport.pendOvlRecvList,mptr,mend); // New for this version
		decode_int7((DWORD&)dport.pendOvlSendList,mptr,mend); // New for this version
		#endif
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&dport,sizeof(CGDPORT),udata);
		break;
	}
	case 0x0C: //WS_UGC lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		UGCPORT gport;
		memset(&gport,0,sizeof(UGCPORT));
		decode_pchar(gport.LocalIP,20,mptr,mend);
		decode_int7(gport.Port,mptr,mend);
		decode_int7(gport.bindPort,mptr,mend); //New for this version
		decode_int7((LONGLONG&)gport.Sock,mptr,mend);
		decode_int7(gport.ConnectHold,mptr,mend);
		decode_int7(gport.InUse,mptr,mend);
		decode_int7(gport.BlockSize,mptr,mend);
		decode_pchar(gport.CfgNote,80,mptr,mend);
		decode_int7(gport.TimeOutSize,mptr,mend);
		decode_int7(gport.TimeOut,mptr,mend);
		decode_int7(gport.TimeOutPeriod,mptr,mend);
		decode_int7(gport.Compressed,mptr,mend);
		decode_int7(gport.Encrypted,mptr,mend);
		decode_int7(gport.PortWasActiveBeforeSuspend,mptr,mend);
		//char MonitorGroups[8][16];
		decode_int7((DWORD&)gport.tmutex,mptr,mend); //New for this version
		decode_int7(gport.activeThread,mptr,mend); //New for this version
		decode_int7(gport.ImmediateSendLimit,mptr,mend); // New for this version
		decode_int7(gport.RecvBuffLimit,mptr,mend); // New for this version
		decode_int7(gport.SendBuffLimit,mptr,mend); // New for this version
		decode_int7(gport.appClosed,mptr,mend); // New for this version
		#ifdef WS_OIO
		decode_int7(gport.pendingClose,mptr,mend); // New for this version
		#endif
		// evert thing ,including SockActive and below,gets reset during WSUgcClosePort(),mptr,mend);
		decode_int7(gport.SockActive,mptr,mend);
		decode_int7(gport.Seconds,mptr,mend);
		decode_int7(gport.BytesIn,mptr,mend);
		decode_int7(gport.BytesLastIn,mptr,mend);
		decode_int7(gport.BytesPerSecIn,mptr,mend);
		decode_int7(gport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(gport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(gport.BytesOut,mptr,mend);
		decode_int7(gport.BytesLastOut,mptr,mend);
		decode_int7(gport.BytesPerSecOut,mptr,mend);
		decode_int7(gport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(gport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(gport.PacketsIn,mptr,mend);
		decode_int7(gport.PacketsOut,mptr,mend);
		decode_int7(gport.BlocksIn,mptr,mend);
		decode_int7(gport.BlocksOut,mptr,mend);
		decode_int7(gport.BeatsIn,mptr,mend);
		decode_int7(gport.BeatsOut,mptr,mend);
		decode_int7(gport.InBuffer_Size,mptr,mend);
		decode_int7(gport.OutBuffer_Size,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr10,mptr,mend);
		decode_binary(gport.Uuid,16,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(gport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(gport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)gport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)gport.Recording.RecInHnd,mptr,mend);
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&gport,sizeof(UGCPORT),udata);
		break;
	}
	case 0x0D: //WS_UGR lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		UGRPORT gport;
		memset(&gport,0,sizeof(UGRPORT));
		decode_int7(gport.SockActive,mptr,mend);
		decode_pchar(gport.RemoteIP,20,mptr,mend);
		decode_pchar(gport.LocalIP,20,mptr,mend);
		decode_pchar(gport.Note,80,mptr,mend);
		decode_pchar(gport.Status,80,mptr,mend);
		decode_int7(gport.SendTimeOut,mptr,mend);
		decode_int7(gport.SinceLastBeatCount,mptr,mend);
		decode_int7(gport.UgcPort,mptr,mend);
		decode_int7((LONGLONG&)gport.Sock,mptr,mend);
		decode_int7(gport.Seconds,mptr,mend);
		decode_int7(gport.EncryptionOn,mptr,mend);
		decode_binary((char*)gport.pw,24,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].IK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].IK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].IK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)gport.Iiv,8,mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].OK1,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].OK2,sizeof(DES_KS),mptr,mend);
		//decode_binary(mptr,(int)(mend -mptr),&pmod->UgrPort[PortNo].OK3,sizeof(DES_KS),mptr,mend);
		decode_binary((char*)gport.Oiv,8,mptr,mend);
		decode_int7(gport.Seconds,mptr,mend);
		decode_int7(gport.BytesIn,mptr,mend);
		decode_int7(gport.BytesLastIn,mptr,mend);
		decode_int7(gport.BytesPerSecIn,mptr,mend);
		decode_int7(gport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(gport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(gport.BytesOut,mptr,mend);
		decode_int7(gport.BytesLastOut,mptr,mend);
		decode_int7(gport.BytesPerSecOut,mptr,mend);
		decode_int7(gport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(gport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(gport.PacketsIn,mptr,mend);
		decode_int7(gport.PacketsOut,mptr,mend);
		decode_int7(gport.BlocksIn,mptr,mend);
		decode_int7(gport.BlocksOut,mptr,mend);
		decode_int7(gport.BeatsIn,mptr,mend);
		decode_int7(gport.BeatsOut,mptr,mend);
		decode_int7(gport.SinceOpenTimer,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)gport.DetPtr10,mptr,mend);
		decode_binary(gport.Uuid,16,mptr,mend);
		//BUFFER InBuffer;
		decode_int7(gport.InBuffer.Size,mptr,mend);
		decode_int7(gport.InBuffer.LocalSize,mptr,mend);
		decode_int7(gport.InBuffer.BlockSize,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(gport.OutBuffer.Size,mptr,mend);
		decode_int7(gport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(gport.OutBuffer.BlockSize,mptr,mend);
		//BUFFER InCompBuffer;
		decode_int7(gport.InCompBuffer.Size,mptr,mend);
		decode_int7(gport.InCompBuffer.LocalSize,mptr,mend);
		decode_int7(gport.InCompBuffer.BlockSize,mptr,mend);
		//BUFFER OutCompBuffer;
		decode_int7(gport.OutCompBuffer.Size,mptr,mend);
		decode_int7(gport.OutCompBuffer.LocalSize,mptr,mend);
		decode_int7(gport.OutCompBuffer.BlockSize,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(gport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(gport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)gport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)gport.Recording.RecInHnd,mptr,mend);
		decode_int7((DWORD&)gport.setupDlg,mptr,mend);
		decode_int7(gport.TimeTillClose,mptr,mend);
		decode_int7(gport.TimeOutStart,mptr,mend);		
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&gport,sizeof(UGRPORT),udata);
		break;
	}
	case 0x0E: //WS_CTO lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CTOPORT oport;
		memset(&oport,0,sizeof(CTOPORT));
		decode_pchar(oport.LocalIP,20,mptr,mend);
		decode_pchar(oport.RemoteIP,20,mptr,mend);
		decode_int7(oport.cast_addr.sin_addr.s_addr,mptr,mend);
		decode_int7(oport.Port,mptr,mend);
		decode_int7(oport.ConnectHold,mptr,mend);
		decode_int7(oport.InUse,mptr,mend);
		decode_int7(oport.BlockSize,mptr,mend);
		decode_pchar(oport.CfgNote,80,mptr,mend);    
		decode_int7(oport.PortWasActiveBeforeSuspend,mptr,mend);
		//MonitorGroups[8][16,mptr,mend);
		decode_int7((DWORD&)oport.smutex,mptr,mend); // New for this version
		decode_int7(oport.sendThread,mptr,mend); // New for this version
		decode_int7(oport.SendBuffLimit,mptr,mend); // New for this version
		// evert thing ,including SockActive and below,gets reset during WSCtoClosePort(,mptr,mend);
		decode_int7(oport.SockActive,mptr,mend);
		decode_pchar(oport.OnLineStatusText,80,mptr,mend);

		decode_pchar(oport.Note,80,mptr,mend);
		decode_pchar(oport.Status,80,mptr,mend);
		decode_int7(oport.SendTimeOut,mptr,mend);
		decode_int7(oport.LastDataTime,mptr,mend);
		decode_int7((LONGLONG&)oport.Sock,mptr,mend);
		decode_int7(oport.Seconds,mptr,mend);
		decode_int7(oport.BytesIn,mptr,mend);
		decode_int7(oport.BytesLastIn,mptr,mend);
		decode_int7(oport.BytesPerSecIn,mptr,mend);
		decode_int7(oport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(oport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(oport.BytesOut,mptr,mend);
		decode_int7(oport.BytesLastOut,mptr,mend);
		decode_int7(oport.BytesPerSecOut,mptr,mend);
		decode_int7(oport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(oport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(oport.PacketsIn,mptr,mend);
		decode_int7(oport.PacketsOut,mptr,mend);
		decode_int7(oport.BlocksIn,mptr,mend);
		decode_int7(oport.BlocksOut,mptr,mend);
		decode_int7(oport.BeatsIn,mptr,mend);
		decode_int7(oport.BeatsOut,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)oport.DetPtr10,mptr,mend);
		decode_binary(oport.Uuid,16,mptr,mend);
		//BUFFER OutBuffer;
		decode_int7(oport.OutBuffer.Size,mptr,mend);
		decode_int7(oport.OutBuffer.LocalSize,mptr,mend);
		decode_int7(oport.OutBuffer.BlockSize,mptr,mend);
		#ifdef WS_OIO
		decode_int7(oport.IOCPSendBytes,mptr,mend); // New for this version
		decode_int7(oport.appClosed,mptr,mend); // New for this version
		decode_int7(oport.pendingClose,mptr,mend); // New for this version
		decode_int7((DWORD&)oport.pendOvlRecvList,mptr,mend); // New for this version
		decode_int7((DWORD&)oport.pendOvlSendList,mptr,mend); // New for this version
		#endif
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&oport,sizeof(CTOPORT),udata);
		break;
	}
	case 0x0F: //WS_CTI lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		CTIPORT iport;
		memset(&iport,0,sizeof(CTIPORT));
		decode_pchar(iport.LocalIP,20,mptr,mend);
		decode_pchar(iport.RemoteIP,20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(iport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(iport.AltRemoteIPOn[i],mptr,mend);
		decode_int7(iport.AltIPCount,mptr,mend);
		decode_int7(iport.CurrentAltIP,mptr,mend);
		decode_int7(iport.Port,mptr,mend);
		decode_int7(iport.ConnectHold,mptr,mend);
		decode_int7(iport.InUse,mptr,mend);
		decode_int7(iport.BlockSize,mptr,mend);
		decode_pchar(iport.CfgNote,80,mptr,mend);    
		decode_int7(iport.Bounce,mptr,mend);
		decode_int7(iport.BounceStart,mptr,mend);
		decode_int7(iport.BounceEnd,mptr,mend);
		decode_int7(iport.PortWasActiveBeforeSuspend,mptr,mend);
		//WSRECORDING Recording;
		decode_pchar(iport.Recording.RecordPath,MAX_PATH,mptr,mend);
		decode_int7(iport.Recording.DoRec,mptr,mend);
		decode_int7((DWORD&)iport.Recording.RecOutHnd,mptr,mend);
		decode_int7((DWORD&)iport.Recording.RecInHnd,mptr,mend);
		decode_pchar(iport.IPAclCfg,128,mptr,mend);
		//IPACL *IPAclList;
		//IPACLMAP IPAclMap;
		//char MonitorGroups[8][16];
		decode_int7(iport.PrefixPacketLen,mptr,mend);
		decode_int7((DWORD&)iport.rmutex,mptr,mend); // New for this version
		decode_int7(iport.recvThread,mptr,mend); // New for this version
		decode_int7(iport.RecvBuffLimit,mptr,mend); // New for this version
		// evert thing ,including SockActive and below,gets reset during WSCtiClosePort(,mptr,mend);
		decode_int7(iport.SockActive,mptr,mend);
		decode_int7(iport.SockConnected,mptr,mend);
		decode_pchar(iport.OnLineStatusText,80,mptr,mend);
		decode_int7(iport.ReconCount,mptr,mend);
		decode_pchar(iport.Note,80,mptr,mend);
		decode_pchar(iport.Status,80,mptr,mend);
		decode_int7(iport.SendTimeOut,mptr,mend);
		decode_int7(iport.LastDataTime,mptr,mend);
		decode_int7((LONGLONG&)iport.Sock,mptr,mend);
		decode_int7(iport.Seconds,mptr,mend);
		decode_int7(iport.BytesIn,mptr,mend);
		decode_int7(iport.BytesLastIn,mptr,mend);
		decode_int7(iport.BytesPerSecIn,mptr,mend);
		decode_int7(iport.BytesPerSecMaxIn,mptr,mend);
		decode_int7(iport.BytesPerSecAvgIn,mptr,mend);
		decode_int7(iport.BytesOut,mptr,mend);
		decode_int7(iport.BytesLastOut,mptr,mend);
		decode_int7(iport.BytesPerSecOut,mptr,mend);
		decode_int7(iport.BytesPerSecMaxOut,mptr,mend);
		decode_int7(iport.BytesPerSecAvgOut,mptr,mend);
		decode_int7(iport.PacketsIn,mptr,mend);
		decode_int7(iport.PacketsOut,mptr,mend);
		decode_int7(iport.BlocksIn,mptr,mend);
		decode_int7(iport.BlocksOut,mptr,mend);
		decode_int7(iport.BeatsIn,mptr,mend);
		decode_int7(iport.BeatsOut,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)iport.DetPtr10,mptr,mend);
		decode_binary(iport.Uuid,16,mptr,mend);
		//BUFFER InBuffer;
		decode_int7(iport.InBuffer.Size,mptr,mend);
		decode_int7(iport.InBuffer.LocalSize,mptr,mend);
		decode_int7(iport.InBuffer.BlockSize,mptr,mend);
		decode_int7((DWORD&)iport.setupDlg,mptr,mend);
		#ifdef WS_OIO
		decode_int7(iport.appClosed,mptr,mend); // New for this version
		decode_int7(iport.pendingClose,mptr,mend); // New for this version
		decode_int7((DWORD&)iport.pendOvlRecvList,mptr,mend); // New for this version
		decode_int7((DWORD&)iport.pendOvlSendList,mptr,mend); // New for this version
		#endif
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&iport,sizeof(CTIPORT),udata);
		break;
	}
	case 0x10: //WS_OTH lvl2
	{
		decode_int7((int&)PortType,mptr,mend);
		decode_int7(PortNo,mptr,mend);
		OTHERPORT zport;
		memset(&zport,0,sizeof(OTHERPORT));
		decode_pchar(zport.RemoteIP,20,mptr,mend);
		decode_int7(zport.Port,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_pchar(zport.AltRemoteIP[i],20,mptr,mend);
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
			decode_int7(zport.AltRemoteIPOn[i],mptr,mend);
		decode_int7(zport.AltIPCount,mptr,mend);
		decode_int7(zport.InUse,mptr,mend);
		decode_pchar(zport.Note,80,mptr,mend);
		decode_pchar(zport.Status,80,mptr,mend);
		decode_pchar(zport.Recieved,50,mptr,mend);
		decode_pchar(zport.Transmit,50,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr1,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr2,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr3,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr4,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr5,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr6,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr7,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr8,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr9,mptr,mend);
		decode_int7((DWORD&)zport.DetPtr10,mptr,mend);
		decode_binary(zport.Uuid,16,mptr,mend);
		//char MonitorGroups[8][16];
		pnotify->NotifyLvl2(aclass,aname,PortType,PortNo,hist,rectype,(char*)&zport,sizeof(OTHERPORT),udata);
		break;
	}
	default:
		return -1;
	};
	_ASSERT(mptr<=mend);
	mptr=mend;
	return 0;
}
// Heartbeat feed
int SysFeedCodec::EncodeHeartbeat(char *&mptr, int mlen, const char *aclass, const char *aname,
    bool hist, int heartbeat, const char *message, int messageLength)
{
    const char *mstart = mptr;
    const char *mend = mptr + mlen;
    *mptr = SMFNO_HEARTBEAT;
    mptr++;
    encode_int7(mptr, (int)(mend - mptr), hist ? 0x01 : 0x00);
    encode_pchar(mptr, (int)(mend - mptr), aclass, 0);
    encode_pchar(mptr, (int)(mend - mptr), aname, 0);
    encode_int7(mptr, (int)(mend - mptr), heartbeat);
    encode_int7(mptr, (int)(mend - mptr), messageLength);
    encode_binary(mptr, (int)(mend - mptr), message, messageLength);
    return(0);
}
// Trademon feed
int SysFeedCodec::EncodeTrademon(char *&mptr, int mlen, const char *aclass, const char *aname, 
	bool hist, const char *Domain, const char *TraderId, int NoUpdate, int ECN, int ECN_TYPE, int SeqNo, int ReqType,
	const __int64& foff, const char *buf, int blen)
{
	const char *mstart=mptr;
	const char *mend=mptr +mlen;
	*mptr=SMFNO_TRADEMON; mptr++;
	encode_int7(mptr,(int)(mend -mptr),hist?0x01:0x00);
	encode_pchar(mptr,(int)(mend -mptr),aclass,0);
	encode_pchar(mptr,(int)(mend -mptr),aname,0);
	// Binary and history always encoded as record type 1
	// Live single text string updates encoded as record type 2
	switch(ReqType)
	{
	case 1://Placelog
	case 2://Reportlog
	case 3://Cancellog
	case 12://LogMerge Reportlog
	case 13://LogMerge Cancellog
		encode_int7(mptr,(int)(mend -mptr),0x01);//record schema
		encode_pchar(mptr,(int)(mend -mptr),Domain,0);
		encode_pchar(mptr,(int)(mend -mptr),TraderId,0);
		encode_int7(mptr,(int)(mend -mptr),NoUpdate);
		encode_int7(mptr,(int)(mend -mptr),ECN);
		encode_int7(mptr,(int)(mend -mptr),ECN_TYPE);
		encode_int7(mptr,(int)(mend -mptr),SeqNo);
		encode_int7(mptr,(int)(mend -mptr),ReqType);
		encode_int7(mptr,(int)(mend -mptr),foff);
		encode_binary(mptr,(int)(mend -mptr),buf,blen);
		break;
	case 4://In Log
	case 5://Out Log
	case 6://New In Log
	case 7://Stuck Order
	case 8://User Log
	case 9://Open Orders
	case 10://Error Log
	case 11://FullLog
	case 14://ECNLog
	case 15://Heartbeat status (i.e. WSConListTitle)
		if(hist)//record schema
			encode_int7(mptr,(int)(mend -mptr),0x01);
		else
			encode_int7(mptr,(int)(mend -mptr),0x02);
		encode_pchar(mptr,(int)(mend -mptr),Domain,0);
		encode_pchar(mptr,(int)(mend -mptr),TraderId,0);
		encode_int7(mptr,(int)(mend -mptr),NoUpdate);
		encode_int7(mptr,(int)(mend -mptr),ECN);
		encode_int7(mptr,(int)(mend -mptr),ECN_TYPE);
		encode_int7(mptr,(int)(mend -mptr),SeqNo);
		encode_int7(mptr,(int)(mend -mptr),ReqType);
		encode_int7(mptr,(int)(mend -mptr),foff);
		if(hist)
			encode_binary(mptr,(int)(mend -mptr),buf,blen);
		else
			encode_pchar(mptr,(int)(mend -mptr),buf,0);
		break;
	};
	_ASSERT(mptr<mend);
	return (int)(mptr -mstart);
}
int SysFeedCodec::DecodeTrademon(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata)
{
	const char *mstart=mptr -1;
	const char *mend=mptr +mlen;
	bool hist=false;
	decode_int7(hist,mptr,mend);
	char aclass[256]={0},aname[256]={0};
	decode_pchar(aclass,sizeof(aclass),mptr,mend);
	decode_pchar(aname,sizeof(aname),mptr,mend);
	int rtype=0;
	decode_int7(rtype,mptr,mend);
	char Domain[64]={0};
	decode_pchar(Domain,sizeof(Domain),mptr,mend);
	char TraderId[64]={0};
	decode_pchar(TraderId,sizeof(TraderId),mptr,mend);
	int NoUpdate=0;
	decode_int7(NoUpdate,mptr,mend);
	int ECN=0;
	decode_int7(ECN,mptr,mend);
	int ECN_TYPE=0;
	decode_int7(ECN_TYPE,mptr,mend);
	int SeqNo=0;
	decode_int7(SeqNo,mptr,mend);
	int ReqType=0;
	decode_int7(ReqType,mptr,mend);
	__int64 foff=0;
	decode_int7(foff,mptr,mend);
	char buf[2048]={0};
	int blen=0;
	switch(rtype)
	{
	case 0x01:
		blen=decode_binary(buf,sizeof(buf),mptr,mend);
		break;
	case 0x02:
		blen=decode_pchar(buf,sizeof(buf),mptr,mend);
		break;
	};
	_ASSERT(mptr<=mend);
	//pnotify->NotifyOrder(aclass,aname,ordid,hist,0x00,buf,blen,udata);
	pnotify->NotifyTrademon(aclass,aname,hist,rtype,Domain,TraderId,NoUpdate,ECN,ECN_TYPE,SeqNo,ReqType,foff,buf,blen,udata,mstart,(int)(mptr -mstart));
	return 0;
}
int SysFeedCodec::DecodeHeartbeat(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata)
{
    const char *mstart = mptr - 1;
    const char *mend = mptr + mlen;
    bool hist = false;
    decode_int7(hist, mptr, mend);
    char aclass[256];
    char aname[256];
    memset(aclass, 0, sizeof(aclass));
    memset(aname, 0, sizeof(aname));
    decode_pchar(aclass, sizeof(aclass), mptr, mend);
    decode_pchar(aname, sizeof(aname), mptr, mend);
    int heartbeat = 0;
    decode_int7(heartbeat, mptr, mend);
    int messageLength;
    decode_int7(messageLength, mptr, mend);
    char message[256];
    memset(message, 0, sizeof(message));
    decode_binary(message, messageLength, mptr, mend);
    _ASSERT(mptr <= mend);
    pnotify->NotifyHeartbeat(aclass, aname, hist, heartbeat, message, messageLength, udata);
    return(0);
}

int SysFeedCodec::Decode(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata)
{
	const char *mend=mptr +mlen;
	if(!pnotify)
		return -1;
	ndata=udata;
	switch(*mptr)
	{
	case SMFNO_ERRORLOG:
	case SMFNO_EVENTLOG: 
		return DecodeLog(++mptr,mlen-1,pnotify,udata);
	case SMFNO_LVL1: 
		return DecodeLvl1(++mptr,mlen-1,pnotify,udata);
	case SMFNO_LVL2: 
		return DecodeLvl2(++mptr,mlen-1,pnotify,udata);
	case SMFNO_TRADEMON: 
		//return DecodeOrder(++mptr,mlen-1,pnotify,udata);
		return DecodeTrademon(++mptr,mlen-1,pnotify,udata);
    case SMFNO_HEARTBEAT:
        return DecodeHeartbeat(++mptr, mlen - 1, pnotify, udata);
	};
	return -1;
}
int SysFeedCodec::DecodeProxy(const char *&mptr, int mlen, int& fid, bool& hist, char *aclass, int clen, char *aname, int nlen)
{
	const char *mend=mptr +mlen;
	fid=*mptr; mptr++;
	decode_int7(hist,mptr,mend);
	decode_pchar(aclass,clen,mptr,mend);
	decode_pchar(aname,nlen,mptr,mend);
	return 0;
}
