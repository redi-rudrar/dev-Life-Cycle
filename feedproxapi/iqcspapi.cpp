
#include "stdafx.h"
#define MAKE_IQCSPAPI
#include "iqcspapi.h"
#include "wstring.h"
#include "price.h"

typedef const char *CPCHAR;

IQCspApi::IQCspApi()
{
	memset(&uquote,0,sizeof(uquote));
	memset(&feedStats,0,sizeof(FeedStats));
}
int IQCspApi::Init(IQCspNotify *pnotify, int PortType, int PortNo)
{
	this->pnotify=pnotify;
	this->PortType=PortType;
	this->PortNo=PortNo;
	return 0;
}
int IQCspApi::Shutdown()
{
	this->pnotify=0;
	this->PortType=-1;
	this->PortNo=-1;
	return 0;
}
int IQCspApi::SkipBate(const char *& bptr, const char *& nptr)
{
	for(bptr++;(bptr<nptr)&&(!myislower(*bptr));bptr++)
		;
	return 0;
}
int IQCspApi::Parse(const char *Msg, int MsgLen, bool whole, LPVOID udata)
{
	if(MsgLen<2)
		return 0;
	const char *bend=Msg +MsgLen;
	// Skip leftovers
	CPCHAR bptr;
	for(bptr=Msg;bptr<bend;bptr++)
	{
		if(*bptr<0x20)
			break;
	}
	//memset(&uquote,0,sizeof(uquote));
	while(bptr<bend)
	{
		// Find beginning of next message
		const char *begin=bptr;
		CPCHAR nptr;
		char *sptr;
		for(nptr=bptr +1;nptr<bend;nptr++)
		{
			if(*nptr<0x20)
				break;
		}
		// Must have at least two exchange tokens
		if((!whole)&&(nptr>=bend))
			return (int)(bptr -Msg);
		feedStats.msgCnt++; feedStats.totMsgCnt++;

		//ResetAttrs();
		uquote.qtype=UQT_UNKNOWN;
		uquote.shdr.mc=IQMC_UNKNOWN;
		// <exch>[<reg>]<SYMBOL>[<bat><val>*]
		uquote.nbats=0;
		memset(&uquote.fmt,0,sizeof(uquote.fmt));
		memset(&uquote.wstime,0,sizeof(uquote.wstime));
		// <exch>
		if(bptr>=nptr)
		{
			feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
		}
		{//scope
		uquote.shdr.offset=(int)(bptr -Msg);
		uquote.shdr.exch=*bptr; bptr++;

		// <2>Get_SYS{yyyymmdd,hhmm}
		uquote.shdr.sym[0]=0;
		if(uquote.shdr.exch==0x02)
		{
			int slen=(int)(nptr -bptr);
			if(slen>31) slen=31;
			else if(slen<10)
			{
				feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
			}
			uquote.shdr.mc=IQMC_COMSTOCK_TIME;
			uquote.qtype=UQT_LVL1TIME;
			uquote.l1time.mc=uquote.shdr.mc;
			// Watch out for "Get_SYS{,hhmm}" from the option feed!
			bptr=strechr(bptr,'{',nptr);
			if(bptr)
			{
				uquote.l1time.wsdate=myatoi(bptr +1);
				bptr=strechr(bptr,',',nptr);
				if(bptr)
					uquote.l1time.wstime=(atoi(bptr +1))*100;
			}
			goto pass_timefeed;
		}
		// <31>hh:mm[:ss]
		else if(uquote.shdr.exch==0x1f)
		{
			int slen=(int)(nptr -bptr);
			if(slen>31) slen=31;
			else if(slen<5)
			{
				feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
			}
			uquote.shdr.mc=IQMC_IQTIME;
			uquote.qtype=UQT_LVL1TIME;
			int hh=(bptr[0] -'0')*10 +(bptr[1] -'0');bptr+=3;
			int mm=(bptr[0] -'0')*10 +(bptr[1] -'0');bptr+=3;
			int ss=0;
			if(bptr<nptr)
			{
				ss=(bptr[0] -'0')*10 +(bptr[1] -'0'); bptr+=3;
			}
			uquote.l1time.mc=uquote.shdr.mc;
			uquote.l1time.wsdate=0;
			uquote.l1time.wstime=(hh*10000)+(mm*100)+(ss);
			goto pass_timefeed;
		}

		// [<reg>]
		uquote.shdr.reg=0;
		if(bptr>=nptr)
		{
			feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
		}
		if(myislower(*bptr))
		{
			uquote.shdr.reg=*bptr; bptr++;
		}
		uquote.shdr.atype=MarketCode(uquote.shdr.exch,(uquote.shdr.exch==10) ?0 :uquote.shdr.reg,uquote.shdr.mc);

		// There should only be one region, but we need to be able to handle a buggy feed
		while(myislower(*bptr))
			bptr++;

		if((uquote.shdr.exch!=10)&&(uquote.shdr.mc==IQMC_UNKNOWN))
			goto skipmsg;

		// <SYMBOL>
		if(bptr>=nptr)
		{
			feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
		}
		sptr=uquote.shdr.sym;
		// Pre-filter the market center and first letter of the symbol
		if(!myislower(*bptr))
		{
			*sptr=*bptr; sptr++; *sptr=0; bptr++;
		}
		if(!pnotify->NotifyFilter(PortType,PortNo,&uquote.shdr,udata,begin,(int)(nptr -begin)))
		{
			feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
		}
		// Rest of the symbol
		for(;(bptr<nptr)&&(!myislower(*bptr));bptr++)
		{
			*sptr=*bptr; sptr++; *sptr=0;
		}
		if(uquote.shdr.exch==12)
		{
			*sptr='#'; sptr++; *sptr=0;
		}

		// [<bat><val>*]
		uquote.lastFormat=0;
		while((bptr<nptr)&&(*bptr>=32))
		{
			int rc=ParseBate(bptr,nptr,uquote,pnotify);
			if(rc<0)
			{
				char ecsp[1024]={0};
				strcpy(ecsp,"nToken Msg: ");
				strncpy(ecsp +strlen(ecsp),begin,nptr -begin); ecsp[1023]=0;
				pnotify->NotifyErrorFP(PortType,PortNo,1,ecsp);
				feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
			}
			else
				uquote.nbats++;
		}
		// Fix up market center for book quotes
		if(uquote.shdr.exch==10)
		{
			uquote.shdr.atype=MarketCode(uquote.shdr.exch,uquote.shdr.reg,uquote.shdr.mc);
		}
		// ComStock time feeds (exch 2 and 31) won't match any bates
		if(uquote.nbats<=0)
		{
			feedStats.skipCnt++; feedStats.totSkipCnt++; goto skipmsg;
		}
		}//scope

pass_timefeed:
		if(pnotify)
		{
			switch(uquote.qtype)
			{
			case UQT_LVL1TIME: 
			{
				pnotify->NotifyLvl1Time(PortType,PortNo,uquote.shdr.sym,&uquote.l1time,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1TRADE: 
			{
				uquote.l1trade.shdr=uquote.shdr;
				uquote.l1trade.fmt=uquote.fmt;
				uquote.l1trade.wstime=uquote.wstime;
				pnotify->NotifyLvl1Trade(PortType,PortNo,uquote.shdr.sym,&uquote.l1trade,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1NBBO: 
			{
				uquote.l1nbbo.shdr=uquote.shdr;
				uquote.l1nbbo.fmt=uquote.fmt;
				uquote.l1nbbo.wstime=uquote.wstime;
				uquote.l1nbbo.listingEx=uquote.l1listingex.listingEx;
				pnotify->NotifyLvl1NBBO(PortType,PortNo,uquote.shdr.sym,&uquote.l1nbbo,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1SUMMARY: 
			{
				uquote.l1sum.shdr=uquote.shdr;
				uquote.l1sum.fmt=uquote.fmt;
				uquote.l1sum.wstime=uquote.wstime;
				pnotify->NotifyLvl1Summary(PortType,PortNo,uquote.shdr.sym,&uquote.l1sum,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1IMBAL:
			{
				uquote.l1imbal.shdr=uquote.shdr;
				uquote.l1imbal.fmt=uquote.fmt;
				uquote.l1imbal.wstime=uquote.wstime;
				pnotify->NotifyLvl1Imbal(PortType,PortNo,uquote.shdr.sym,&uquote.l1imbal,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL2REGIONAL: 
			{
				SetAttr(&uquote.l2reg.region,IQA_LVL2_REGION,IQAT_COMSTOCK,&uquote.shdr.reg,1);
				uquote.l2reg.shdr=uquote.shdr;
				uquote.l2reg.fmt=uquote.fmt;
				uquote.l2reg.haveBid=uquote.l2haveBid;
				uquote.l2reg.bidPx=uquote.l2bidPx;
				uquote.l2reg.bidSize=uquote.l2bidSize;
				uquote.l2reg.haveAsk=uquote.l2haveAsk;
				uquote.l2reg.askPx=uquote.l2askPx;
				uquote.l2reg.askSize=uquote.l2askSize;
				uquote.l2reg.quoteCond=uquote.l2qcond;
				uquote.l2reg.wstime=uquote.wstime;
				pnotify->NotifyLvl2Regional(PortType,PortNo,uquote.shdr.sym,&uquote.l2reg,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL2BOOK: 
			{
				uquote.l2book.shdr=uquote.shdr;
				uquote.l2book.fmt=uquote.fmt;
				uquote.l2book.haveBid=uquote.l2haveBid;
				uquote.l2book.bidPx=uquote.l2bidPx;
				uquote.l2book.bidSize=uquote.l2bidSize;
				uquote.l2book.haveAsk=uquote.l2haveAsk;
				uquote.l2book.askPx=uquote.l2askPx;
				uquote.l2book.askSize=uquote.l2askSize;
				uquote.l2book.quoteCond=uquote.l2qcond;
				uquote.l2book.wstime=uquote.wstime;
				pnotify->NotifyLvl2Book(PortType,PortNo,uquote.shdr.sym,&uquote.l2book,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL2MMID: 
			{
				uquote.l2mmid.shdr=uquote.shdr;
				uquote.l2mmid.fmt=uquote.fmt;
				uquote.l2mmid.haveBid=uquote.l2haveBid;
				uquote.l2mmid.bidPx=uquote.l2bidPx;
				uquote.l2mmid.bidSize=uquote.l2bidSize;
				uquote.l2mmid.haveAsk=uquote.l2haveAsk;
				uquote.l2mmid.askPx=uquote.l2askPx;
				uquote.l2mmid.askSize=uquote.l2askSize;
				uquote.l2mmid.quoteCond=uquote.l2qcond;
				uquote.l2mmid.wstime=uquote.wstime;
				pnotify->NotifyLvl2Mmid(PortType,PortNo,uquote.shdr.sym,&uquote.l2mmid,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL2BROKER: 
			{
				uquote.l2broker.shdr=uquote.shdr;
				uquote.l2broker.fmt=uquote.fmt;
				uquote.l2broker.haveBid=uquote.l2haveBid;
				uquote.l2broker.bidPx=uquote.l2bidPx;
				uquote.l2broker.bidSize=uquote.l2bidSize;
				uquote.l2broker.haveAsk=uquote.l2haveAsk;
				uquote.l2broker.askPx=uquote.l2askPx;
				uquote.l2broker.askSize=uquote.l2askSize;
				uquote.l2broker.quoteCond=uquote.l2qcond;
				uquote.l2broker.wstime=uquote.wstime;
				pnotify->NotifyLvl2Broker(PortType,PortNo,uquote.shdr.sym,&uquote.l2broker,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1LISTINGEX:
			{
				uquote.l1listingex.shdr=uquote.shdr;
				pnotify->NotifyLvl1ListingEx(PortType,PortNo,uquote.shdr.sym,&uquote.l1listingex,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1COMPOSITE:
			{
				uquote.l1composite.shdr=uquote.shdr;
				pnotify->NotifyLvl1Composite(PortType,PortNo,uquote.shdr.sym,&uquote.l1composite,udata,begin,(int)(nptr -begin)); 
				break;
			}
			case UQT_LVL1ANALYTICS:
			{
				uquote.l1analytics.shdr=uquote.shdr;
				uquote.l1analytics.fmt=uquote.fmt;
				pnotify->NotifyLvl1Analytics(PortType,PortNo,uquote.shdr.sym,&uquote.l1analytics,udata,begin,(int)(nptr -begin)); 
				break;
			}
			default: _ASSERT(false);
			};
		}

skipmsg:
		bptr=nptr;
	}
	return (int)(bptr -Msg);
}
//static int ParseBateA(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	if(uquote.qtype==UQT_UNKNOWN)
//	{
//		uquote.qtype=UQT_LVL1NBBO;
//		memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
//	}
//	uquote.l1nbbo.haveAsk=true;
//
//	// Optional price region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		pattr=IQCspApi::SetAttr(&uquote.l1nbbo.askReg,IQA_ASK_REGION,IQAT_COMSTOCK,(void*)bptr,1);
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.l1nbbo.askPx,IQA_ASK_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	pattr->reserved=uquote.lastFormat;
//	return 0;
//}
//static int ParseBateB(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	if(uquote.qtype==UQT_UNKNOWN)
//	{
//		uquote.qtype=UQT_LVL1NBBO;
//		memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
//	}
//	uquote.l1nbbo.haveBid=true;
//
//	// Optional price region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		pattr=IQCspApi::SetAttr(&uquote.l1nbbo.bidReg,IQA_BID_REGION,IQAT_COMSTOCK,(void*)bptr,1);
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.l1nbbo.bidPx,IQA_BID_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	pattr->reserved=uquote.lastFormat;
//	return 0;
//}
//static int ParseBateJ(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	if(uquote.qtype==UQT_UNKNOWN)
//	{
//		uquote.qtype=UQT_LVL1NBBO;
//		memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
//	}
//	uquote.l1nbbo.haveBid=true;
//
//	// Optional share region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.l1nbbo.bidSize,IQA_BID_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	return 0;
//}
//static int ParseBateK(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	if(uquote.qtype==UQT_UNKNOWN)
//	{
//		uquote.qtype=UQT_LVL1NBBO;
//		memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
//	}
//	uquote.l1nbbo.haveAsk=true;
//
//	// Optional share region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.l1nbbo.askSize,IQA_ASK_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	return 0;
//}
//static int ParseBateP(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	// Optional share region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.l2bidSize,IQA_BID_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	return 0;
//}
//static int ParseBateQ(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	if(uquote.qtype==UQT_UNKNOWN)
//	{
//		uquote.qtype=UQT_LVL2REGIONAL;
//		uquote.l2haveBid=false;
//		memset(&uquote.l2bidReg,0,sizeof(uquote.l2bidReg));
//		memset(&uquote.l2bidPx,0,sizeof(uquote.l2bidPx));
//		memset(&uquote.l2bidSize,0,sizeof(uquote.l2bidSize));
//		uquote.l2haveAsk=false;
//		memset(&uquote.l2askReg,0,sizeof(uquote.l2askReg));
//		memset(&uquote.l2askPx,0,sizeof(uquote.l2askPx));
//		memset(&uquote.l2askSize,0,sizeof(uquote.l2askSize));
//		memset(&uquote.l2qcond,0,sizeof(uquote.l2qcond));
//	}
//	uquote.l2haveBid=true;
//
//	// Optional price region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		pattr=IQCspApi::SetAttr(&uquote.l2bidReg,IQA_BID_REGION,IQAT_COMSTOCK,(void*)bptr,1);
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	// We don't know what type of lvl2 quote until we get to f66
//	pattr=IQCspApi::SetAttr(&uquote.l2bidPx,IQA_BID_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	pattr->reserved=uquote.lastFormat;
//	return 0;
//}
//static int ParseBateR(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	if(uquote.qtype==UQT_UNKNOWN)
//	{
//		uquote.qtype=UQT_LVL2REGIONAL;
//		uquote.l2haveBid=false;
//		memset(&uquote.l2bidReg,0,sizeof(uquote.l2bidReg));
//		memset(&uquote.l2bidPx,0,sizeof(uquote.l2bidPx));
//		memset(&uquote.l2bidSize,0,sizeof(uquote.l2bidSize));
//		uquote.l2haveAsk=false;
//		memset(&uquote.l2askReg,0,sizeof(uquote.l2askReg));
//		memset(&uquote.l2askPx,0,sizeof(uquote.l2askPx));
//		memset(&uquote.l2askSize,0,sizeof(uquote.l2askSize));
//		memset(&uquote.l2qcond,0,sizeof(uquote.l2qcond));
//	}
//	uquote.l2haveAsk=true;
//
//	// Optional price region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		pattr=IQCspApi::SetAttr(&uquote.l2askReg,IQA_ASK_REGION,IQAT_COMSTOCK,(void*)bptr,1);
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	// We don't know what type of lvl2 quote until we get to f66
//	pattr=IQCspApi::SetAttr(&uquote.l2askPx,IQA_ASK_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	pattr->reserved=uquote.lastFormat;
//	return 0;
//}
//static int ParseBateX(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	if(bptr>=nptr)
//		return -1;
//	// Optional share region
//	if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
//	{
//		bptr++;
//	}
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.l2askSize,IQA_ASK_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	return 0;
//}
//static int ParseBateZ(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify)
//{
//	const char *begin=bptr;
//	IQATTRENTRY *pattr=0;
//	//if(ParseDateTime(bate.tval,bptr,nptr)<0)
//	//	return -1;
//	char tstr[8];
//	memcpy(tstr,begin,6); tstr[6]=0;
////		feedStats.lastQuoteTime=myatoi(tstr);
//	if(IQCspApi::SkipBate(bptr,nptr)<0)
//		return -1;
//	pattr=IQCspApi::SetAttr(&uquote.wstime,IQA_TIME,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
//	return 0;
//}
//typedef int (*ParseOneBate)(const char *& bptr, const char *& nptr, Uquote& uquote, IQCspNotify *pnotify);
//static ParseOneBate bateFuncs[26]={
//	ParseBateA,
//	ParseBateB,
//	0,0,0,0,0,0,0,
//	ParseBateJ,
//	ParseBateK,
//	0,0,0,0,
//	ParseBateP,
//	ParseBateQ,
//	ParseBateR,
//	0,0,0,0,0,
//	ParseBateX,
//	0,
//	ParseBateZ
//};
// This is a very fast pre-parse that only parses the bate codes and keeps the values in Comstock encoding.
int IQCspApi::ParseBate(CPCHAR& bptr, CPCHAR& nptr, Uquote& uquote, IQCspNotify *pnotify)
{
	if(bptr>=nptr)
		return -1;
	char bat=*bptr; bptr++;
	IQATTRENTRY *pattr=0;
	const char *begin=bptr;

	// The theory is a big switch statement can be slower than function call,
	// but I didn't see any significant improvement over this switch.
	// Using 4 threads, I achieved 4.3M mps!
	//ParseOneBate pfunc=bateFuncs[bat -'a'];
	//if(pfunc)
	//	return pfunc(bptr,nptr,uquote,pnotify);

	switch(bat)
	{
	case 'a': // best ask price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1NBBO;
			memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
		}
		uquote.l1nbbo.haveAsk=true;

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			pattr=SetAttr(&uquote.l1nbbo.askReg,IQA_ASK_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1nbbo.askPx,IQA_ASK_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'b': // best bid price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1NBBO;
			memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
		}
		uquote.l1nbbo.haveBid=true;

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			pattr=SetAttr(&uquote.l1nbbo.bidReg,IQA_BID_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1nbbo.bidPx,IQA_BID_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'c': // last price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			pattr=SetAttr(&uquote.l1sum.lastReg,IQA_TRADE_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.lastPx,IQA_LAST_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'h': // high price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			//pattr=SetAttr(&uquote.l1sum.highReg,IQA_HIGH_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.highPx,IQA_HIGH_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'l': // low price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			//pattr=SetAttr(&uquote.l1sum.lowReg,IQA_LOW_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.lowPx,IQA_LOW_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'n': // net change price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.netChangePx,IQA_NETCHANGE_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'o': // open price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			//pattr=SetAttr(&uquote.l1sum.openReg,IQA_OPEN_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.openPx,IQA_OPEN_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'q': // lvl2 bid price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL2REGIONAL;
			uquote.l2haveBid=false;
			memset(&uquote.l2bidPx,0,sizeof(uquote.l2bidPx));
			memset(&uquote.l2bidSize,0,sizeof(uquote.l2bidSize));
			uquote.l2haveAsk=false;
			memset(&uquote.l2askPx,0,sizeof(uquote.l2askPx));
			memset(&uquote.l2askSize,0,sizeof(uquote.l2askSize));
			memset(&uquote.l2qcond,0,sizeof(uquote.l2qcond));
		}
		uquote.l2haveBid=true;

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			//pattr=SetAttr(&uquote.l2bidReg,IQA_BID_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		// We don't know what type of lvl2 quote until we get to f66
		pattr=SetAttr(&uquote.l2bidPx,IQA_BID_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'r': // lvl2 ask price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL2REGIONAL;
			uquote.l2haveBid=false;
			memset(&uquote.l2bidPx,0,sizeof(uquote.l2bidPx));
			memset(&uquote.l2bidSize,0,sizeof(uquote.l2bidSize));
			uquote.l2haveAsk=false;
			memset(&uquote.l2askPx,0,sizeof(uquote.l2askPx));
			memset(&uquote.l2askSize,0,sizeof(uquote.l2askSize));
			memset(&uquote.l2qcond,0,sizeof(uquote.l2qcond));
		}
		uquote.l2haveAsk=true;

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			//pattr=SetAttr(&uquote.l2askReg,IQA_ASK_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		// We don't know what type of lvl2 quote until we get to f66
		pattr=SetAttr(&uquote.l2askPx,IQA_ASK_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 's': // settlement price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.settlementPx,IQA_SETTLEMENT_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 't': // trade price
	{
		if(bptr>=nptr)
			return -1;
		uquote.qtype=UQT_LVL1TRADE;
		memset(&uquote.l1trade,0,sizeof(uquote.l1trade));

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			pattr=SetAttr(&uquote.l1trade.tradeReg,IQA_TRADE_REGION,IQAT_COMSTOCK,(void*)bptr,1);
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1trade.tradePx,IQA_TRADE_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'y': // prev close price
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.prevClosePx,IQA_PREVCLOSE_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'd': // date
	{
		if(bptr>=nptr)
			return -1;
		//if(ParseDateTime(bate.tval,bptr,nptr)<0)
		//	return -1;
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.wsdate,IQA_DATE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'z': // time
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		//if(ParseDateTime(bate.tval,bptr,nptr)<0)
		//	return -1;
		char tstr[8];
		memcpy(tstr,begin,6); tstr[6]=0;
		feedStats.lastQuoteTime=myatoi(tstr);
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.wstime,IQA_TIME,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'e': // open interest
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.openInterest,IQA_OPEN_INTEREST,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'i': // last size
	{
		if(bptr>=nptr)
			return -1;
		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1trade.tradeSize,IQA_TRADE_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'j': // best bid size
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1NBBO;
			memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
		}
		uquote.l1nbbo.haveBid=true;

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1nbbo.bidSize,IQA_BID_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'k': // best ask size
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1NBBO;
			memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
		}
		uquote.l1nbbo.haveAsk=true;

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1nbbo.askSize,IQA_ASK_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'p': // lvl2 bid size
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL2REGIONAL;
			uquote.l2haveBid=false;
			memset(&uquote.l2bidPx,0,sizeof(uquote.l2bidPx));
			memset(&uquote.l2bidSize,0,sizeof(uquote.l2bidSize));
			uquote.l2haveAsk=false;
			memset(&uquote.l2askPx,0,sizeof(uquote.l2askPx));
			memset(&uquote.l2askSize,0,sizeof(uquote.l2askSize));
			memset(&uquote.l2qcond,0,sizeof(uquote.l2qcond));
		}
		uquote.l2haveBid=true;

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l2bidSize,IQA_BID_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'v': // volume
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr((uquote.qtype==UQT_LVL1TRADE) ?&uquote.l1trade.volume :&uquote.l1sum.volume,
			IQA_VOLUME,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'w': // prev volume
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.prevVolume,IQA_PREV_VOLUME,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'x': // lvl2 ask size
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL2REGIONAL;
			uquote.l2haveBid=false;
			memset(&uquote.l2bidPx,0,sizeof(uquote.l2bidPx));
			memset(&uquote.l2bidSize,0,sizeof(uquote.l2bidSize));
			uquote.l2haveAsk=false;
			memset(&uquote.l2askPx,0,sizeof(uquote.l2askPx));
			memset(&uquote.l2askSize,0,sizeof(uquote.l2askSize));
			memset(&uquote.l2qcond,0,sizeof(uquote.l2qcond));
		}
		uquote.l2haveAsk=true;

		// Optional share region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l2askSize,IQA_ASK_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'm': // last regular trade
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1SUMMARY;
			memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
		}

		// Optional price region
		if(((*bptr>='A')&&(*bptr<='Z'))||((*bptr>='a')&&(*bptr<='z')))
		{
			bptr++;
		}
		//if(ParseInt(bate.ival,bptr,nptr)<0)
		//	return -1;
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l1sum.lastRegularPx,IQA_LAST_REGULAR_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		pattr->reserved=uquote.lastFormat;
		break;
	}
	case 'u': // quote condition
	{
		if(bptr>=nptr)
			return -1;
		if(uquote.qtype==UQT_UNKNOWN)
		{
			uquote.qtype=UQT_LVL1NBBO;
			memset(&uquote.l1nbbo,0,sizeof(uquote.l1nbbo));
		}

		//if(ParseInt(bate.ival,bptr,nptr)<0)
		//	return -1;
		if(SkipBate(bptr,nptr)<0)
			return -1;
		pattr=SetAttr(&uquote.l2qcond,IQA_QUOTE_CONDITION,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
		break;
	}
	case 'f': // function codes
	case 'g': // function codes
	{
		int fcode=0;
		//if(ParseInt(fcode,bptr,nptr)<0)
		//	return -1;
		for(;(bptr<nptr)&&(myisdigit(*bptr));bptr++)
		{
			fcode*=10; fcode+=(*bptr -'0');
		}
		//bate.fcode=fcode;
		if(fcode>9)
		{
			if(bptr>=nptr)
				return -1;
			if(*bptr!=',')
				return -1;
			bptr++;
		}
		if(bat=='f')
		{
			switch(fcode)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9: // Last format
			{
				uquote.lastFormat=fcode;
				pattr=SetAttr(&uquote.fmt,IQA_DISPLAY_FORMAT,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			case 17: // Trade sequence number for corrections
			{
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1trade.seqNo,IQA_SEQNO,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			//case 18: // ExecInst
			//{
			//	attr=IQA_EXECINST;
			//	if(SkipBate(bptr,nptr)<0)
			//		return -1;
			//	begin+=3;
			//pattr=SetAttr(&uquote.l1trade.execInst,IQA_EXECINST,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
			//	break;
			//}
			case 19: // Latency timestamp
			{
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				//pattr=SetAttr(&uquote.qts.seqNo,IQA_TIMESTAMP,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));

				// Latency test code (overstamp)
				//char lstr[32]={0};
				//timeBeginPeriod(1);
				//DWORD lts=timeGetTime();
				//sprintf(lstr,"%d",lts);
				//memcpy((char*)begin,lstr,strlen(lstr));
				break;
			}
			case 21: // fImbalancePrice
			{
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1IMBAL;
					memset(&uquote.l1imbal,0,sizeof(uquote.l1imbal));
				}
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1imbal.refPx,IQA_IMBALANCE_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			}
			case 22: // bid lImbalanceSize
			{
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1IMBAL;
					memset(&uquote.l1imbal,0,sizeof(uquote.l1imbal));
				}
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1imbal.size,IQA_IMBALANCE_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				uquote.side=1;
				SetAttr(&uquote.l1imbal.side,IQA_IMBALANCE_SIDE,IQAT_UCHAR,(void*)&uquote.side,1);
				break;
			}
			case 23: // ask lImbalanceSize
			{
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1IMBAL;
					memset(&uquote.l1imbal,0,sizeof(uquote.l1imbal));
				}
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1imbal.size,IQA_IMBALANCE_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				uquote.side=2;
				SetAttr(&uquote.l1imbal.side,IQA_IMBALANCE_SIDE,IQAT_UCHAR,(void*)&uquote.side,1);
				break;
			}
			case 24: // lPairedSize
			{
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1IMBAL;
					memset(&uquote.l1imbal,0,sizeof(uquote.l1imbal));
				}
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1imbal.pairedSize,IQA_IMBALANCE_PAIRED_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			case 25: // cImbalanceStatus
			{
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1IMBAL;
					memset(&uquote.l1imbal,0,sizeof(uquote.l1imbal));
				}
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1imbal.status,IQA_IMBALANCE_STATUS,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			case 26: // Black Scholes theoretical price
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsTheoPx,IQA_BS_THEO_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 27: // Black Scholes gamma
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsGamma,IQA_BS_GAMMA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 28: // Black Scholes delta
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsDelta,IQA_BS_DELTA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 29: // Black Scholes rho
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsRho,IQA_BS_RHO,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 30: // Black Scholes vega
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsVega,IQA_BS_VEGA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 31: // Black Scholes theta
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsTheta,IQA_BS_THETA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 32: // Black Scholes vol
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsVol,IQA_BS_VOL,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 33: // Black Scholes implied vol
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsImpliedVol,IQA_BS_IMPLIED_VOL,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 34: // Black Scholes IV bid
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsIVBid,IQA_BS_IVBID,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 35: // Black Scholes IV ask
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.bsIVAsk,IQA_BS_IVASK,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 36: // Binonmial theoretical price
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binTheoPx,IQA_BINOMIAL_THEO_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 37: // Binonmial gamma
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1COMPOSITE;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binGamma,IQA_BINOMIAL_GAMMA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 38: // Binonmial delta
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binDelta,IQA_BINOMIAL_DELTA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 39: // Binonmial rho
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binRho,IQA_BINOMIAL_RHO,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			//case 40: // OrdType
			//{
			//	if(SkipBate(bptr,nptr)<0)
			//		return -1;
			//	begin+=3;
			//	pattr=SetAttr(&uquote.l2ordtype,IQA_ORDTYPE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
			//	break;
			//}
			case 40: // Binonmial vega
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binVega,IQA_BINOMIAL_VEGA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 41: // Corrected trade sequence number
			{
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1trade.correctNo,IQA_CORRECTION_NO,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			case 42: // Binonmial theta
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binTheta,IQA_BINOMIAL_THETA,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 43: // Binonmial vol
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binVol,IQA_BINOMIAL_VOL,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 44: // Binonmial implied vol
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binImpliedVol,IQA_BINOMIAL_IMPLIED_VOL,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 45: // Binonmial IV bid
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binIVBid,IQA_BINOMIAL_IVBID,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 46: // Binonmial IV ask
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1ANALYTICS;
					memset(&uquote.l1analytics,0,sizeof(uquote.l1analytics));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1analytics.binIVAsk,IQA_BINOMIAL_IVASK,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			case 54: // ExpDate
			{
				_ASSERT(false);//untested
				//if(ParseDateTime(bate.tval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 55: // Strike
			{
				if(bptr>=nptr)
					return -1;
				// Optional price region
				if(myislower(*bptr))
				{
					bptr++;
				}
				//if(ParsePrice(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 58: // PrevClose
			{
				if(bptr>=nptr)
					return -1;
				// Optional price region
				if(myislower(*bptr))
				{
					bptr++;
				}
				//if(ParsePrice(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 59: // Opra best bid size
			{
				if(bptr>=nptr)
					return -1;
				// Optional size region
				if(myislower(*bptr))
				{
					bptr++;
				}
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 60: // Opra best ask size
			{
				if(bptr>=nptr)
					return -1;
				// Optional size region
				if(myislower(*bptr))
				{
					bptr++;
				}
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 78: // Special price codes ^,!,$
			case 80: // Special price code @
			case 81: // Special price code #
			{
				if(bptr>=nptr)
					return -1;
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1TRADE;
					memset(&uquote.l1trade,0,sizeof(uquote.l1trade));
				}
				// Optional price region
				if(myislower(*bptr))
				{
					pattr=SetAttr(&uquote.l1trade.tradeReg,IQA_TRADE_REGION,IQAT_COMSTOCK,(void*)bptr,1);
					bptr++;
				}
				//if(ParsePrice(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1trade.tradePx,IQA_TRADE_PX,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=uquote.lastFormat;
				break;
			}
			case 66:
			case 95:
			case 96: // MMID
			{
				//if(ParseStr(bate.sval,bptr,nptr)<0)
				//	return -1;
				// Book
				if(!memcmp(bptr+2,"7373",4))
				{
					uquote.qtype=UQT_LVL2BOOK;
					memset(&uquote.l2book,0,sizeof(uquote.l2book));
					uquote.shdr.reg='a' +(bptr[0]-'0')*8 +(bptr[1]-'0') -1;
					if(SkipBate(bptr,nptr)<0)
						return -1;
					begin+=3;
					pattr=SetAttr(&uquote.l2book.level,IQA_BOOK_LEVEL,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				}
				// MMID or brokerno
				else
				{
					if(SkipBate(bptr,nptr)<0)
						return -1;
					begin+=3;
					if(fcode==66)
					{
						uquote.qtype=UQT_LVL2MMID;
						memset(&uquote.l2mmid,0,sizeof(uquote.l2mmid));
						pattr=SetAttr(&uquote.l2mmid.mmid,IQA_MMID,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
					}
					else
					{
						uquote.qtype=UQT_LVL2BROKER;
						memset(&uquote.l2broker,0,sizeof(uquote.l2broker));
						pattr=SetAttr(&uquote.l2broker.broker,IQA_BROKERNO,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
					}
				}
				break;
			}
			//case 36: // Short cap gains
			//case 37: // Other cap gains
			//case 38: // Long cap gains
			//case 39: // Unalloc dist
			case 83:
			case 84:
			case 85: // Dividend
			case 87: // Foot codes
			{
				if(bptr>=nptr)
					return -1;
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 72: // Update (OTCBB)
			{
				if(bptr>=nptr)
					return -1;
				// e.g. .kAACQWx0f72,0f2h063f2l059f2cU060f2mU060v-19500
				//if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1SUMMARY;
					memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
				}
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 67: // Quote cond
			{
				if(bptr>=nptr)
					return -1;
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l2qcond,IQA_QUOTE_CONDITION,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=fcode;
				break;
			}
			case 71: // Format
			{
				if(bptr>=nptr)
					return -1;
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.fmt,IQA_DISPLAY_FORMAT,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=fcode;
				break;
			}
			case 73: // Multi-sales cond
			{
				if(bptr>=nptr)
					return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				memset(&uquote.l1trade.msaleCond,0,sizeof(uquote.l1trade.msaleCond));
				pattr=SetAttr(&uquote.l1trade.msaleCond,IQA_MULTI_SALE_COND,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=fcode;
				break;
			}
			case 70: // Sales cond
			case 79: // Sale cond
			{
				if(bptr>=nptr)
					return -1;
				//if(ParseInt(bate.pval,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1trade.saleCond,IQA_SALE_CONDITION,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=fcode;
				break;
			}
			case 77: // Shares (absolute value)
			case 82: // Special price codes @,# (absolute value)
			{
				//if(ParseInt(bate.ival,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				begin+=3;
				pattr=SetAttr(&uquote.l1trade.tradeSize,IQA_TRADE_SIZE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				pattr->reserved=true;
				break;
			}
			case 97: // Express
			{
				//if(ParseInt(bate.ival,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			case 99: // New day
			{
				//if(ParseChar(bate.cval,bptr,nptr)<0)
				//	return -1;
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1SUMMARY;
					memset(&uquote.l1sum,0,sizeof(uquote.l1sum));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1sum.f99,IQA_CLOSE_SYMBOL,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			case 89: // Listing exchange
			{
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1LISTINGEX;
					memset(&uquote.l1listingex,0,sizeof(uquote.l1listingex));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1listingex.listingEx,IQA_LISTING_EXCHANGE,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			}
			case 61: // Composite Bid
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1COMPOSITE;
					memset(&uquote.l1composite,0,sizeof(uquote.l1composite));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1composite.compositeBidSize,IQA_COMPOSITE_BID,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			case 62: // Composite Ask
				if(uquote.qtype==UQT_UNKNOWN)
				{
					uquote.qtype=UQT_LVL1COMPOSITE;
					memset(&uquote.l1composite,0,sizeof(uquote.l1composite));
				}
				begin=bptr;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				pattr=SetAttr(&uquote.l1composite.compositeAskSize,IQA_COMPOSITE_ASK,IQAT_COMSTOCK,(void*)begin,(int)(bptr -begin));
				break;
			default:
				_ASSERT(false);
				char ecsp[1024]={0};
				sprintf(ecsp,"nToken f,(%d)",fcode);
				strncpy(ecsp +strlen(ecsp),begin,(int)(nptr -begin)); ecsp[1023]=0;
				pnotify->NotifyErrorFP(PortType,PortNo,1,ecsp);
				return -1;
			};
		}
		else if(bat=='g')
		{
			_ASSERT(false);//untested
			switch(fcode)
			{
			case 1:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 50:
			case 60:
			case 70:
			case 80:
			case 90: // Alert
			{
				//if(ParseInt(bate.ival,bptr,nptr)<0)
				//	return -1;
				if(SkipBate(bptr,nptr)<0)
					return -1;
				break;
			}
			default:
				_ASSERT(false);
				char ecsp[1024]={0};
				sprintf(ecsp,"nToken g,(%d)",fcode);
				strncpy(ecsp +strlen(ecsp),begin,(int)(nptr -begin)); ecsp[1023]=0;
				pnotify->NotifyErrorFP(PortType,PortNo,1,ecsp);
				return -1;
			};
		}
		break;
	}
	default:
		//_ASSERT(false);
		char ecsp[1024]={0};
		sprintf(ecsp,"nToken Bate(%02d=%c)",bat,bat);
		strncpy(ecsp +strlen(ecsp),begin,nptr -begin); ecsp[1023]=0;
		pnotify->NotifyErrorFP(PortType,PortNo,1,ecsp);

		char tstr[256]={0};
		sprintf(tstr,"nToken Bate(%02d=%c)\n",bat,bat);
		OutputDebugString(tstr);
		return -1;
	};
	return 0;
}
const char *IQCspApi::GetAttrName(IQATTR attr)
{
	switch(attr)
	{
	case IQA_UNKNOWN:						return "IQA_UNKNOWN";
	// Reference							// Reference
	case IQA_SYMBOL:						return "IQA_SYMBOL";
	case IQA_REFERENCE:						return "IQA_REFERENCE";
	case IQA_IDSOURCE:						return "IQA_IDSOURCE";
	case IQA_LISTING_EXCHANGE:				return "IQA_LISTING_EXCHANGE";
	case IQA_ASSET_TYPE:					return "IQA_ASSET_TYPE";
	case IQA_FULL_NAME:						return "IQA_FULL_NAME";
	case IQA_LOT_SIZE:						return "IQA_LOT_SIZE";
	case IQA_DISPLAY_FORMAT:				return "IQA_DISPLAY_FORMAT";
	case IQA_UNDERLYER:						return "IQA_UNDERLYER";
	case IQA_MININUM_FLUCTUATION:			return "IQA_MININUM_FLUCTUATION";
	case IQA_FLUCTUATION_VALUE:				return "IQA_FLUCTUATION_VALUE";
	case IQA_CURRENCY:						return "IQA_CURRENCY";
	case IQA_EXPIRATION:					return "IQA_EXPIRATION";
	case IQA_STRIKE_PX:						return "IQA_STRIKE_PX";
	case IQA_CUSIP:							return "IQA_CUSIP";
	case IQA_ISIN:							return "IQA_ISIN";
	// Quoting								// Quoting
	case IQA_HISTORIC:						return "IQA_HISTORIC";
	case IQA_LINEID:						return "IQA_LINEID";
	case IQA_SEQNO:							return "IQA_SEQNO";
	case IQA_GROUP_COUNT:					return "IQA_GROUP_COUNT";
	// Level 1								// Level 1
	case IQA_REALTIME:						return "IQA_REALTIME";
	case IQA_DELAYED:						return "IQA_DELAYED";
	case IQA_QUOTE_EXCHANGE:				return "IQA_QUOTE_EXCHANGE";
	case IQA_DATE:							return "IQA_DATE";
	case IQA_TIME:							return "IQA_TIME";
	case IQA_OPEN_PX:						return "IQA_OPEN_PX";
	case IQA_HIGH_PX:						return "IQA_HIGH_PX";
	case IQA_LOW_PX:						return "IQA_LOW_PX";
	case IQA_LAST_PX:						return "IQA_LAST_PX";
	case IQA_LAST_SIZE:						return "IQA_LAST_SIZE";
	case IQA_LAST_REGULAR_PX:				return "IQA_LAST_REGULAR_PX";
	case IQA_NETCHANGE_PX:					return "IQA_NETCHANGE_PX";
	case IQA_VOLUME:						return "IQA_VOLUME";
	case IQA_FORMT_VOLUME:					return "IQA_FORMT_VOLUME";
	case IQA_PREV_VOLUME:					return "IQA_PREV_VOLUME";
	case IQA_OPEN_INTEREST:					return "IQA_OPEN_INTEREST";
	case IQA_BID_PX:						return "IQA_BID_PX";
	case IQA_BID_SIZE:						return "IQA_BID_SIZE";
	case IQA_BID_REGION:					return "IQA_BID_REGION";
	case IQA_BID_CONDITION:					return "IQA_BID_CONDITION";
	case IQA_ASK_PX:						return "IQA_ASK_PX";
	case IQA_ASK_SIZE:						return "IQA_ASK_SIZE";
	case IQA_ASK_REGION:					return "IQA_ASK_REGION";
	case IQA_ASK_CONDITION:					return "IQA_ASK_CONDITION";
	case IQA_QUOTE_CONDITION:				return "IQA_QUOTE_CONDITION";
	case IQA_TRADE_PX:						return "IQA_TRADE_PX";
	case IQA_TRADE_SIZE:					return "IQA_TRADE_SIZE";
	case IQA_TRADE_REGION:					return "IQA_TRADE_REGION";
	case IQA_SALE_CONDITION:				return "IQA_SALE_CONDITION";
	case IQA_LASTTRADE_DATE:				return "IQA_LASTTRADE_DATE";
	case IQA_LASTTRADE_TIME:				return "IQA_LASTTRADE_TIME";
	case IQA_CLOSE_PX:						return "IQA_CLOSE_PX";
	case IQA_PREVCLOSE_PX:					return "IQA_PREVCLOSE_PX";
	case IQA_PREVCLOSE_DATE:				return "IQA_PREVCLOSE_DATE";
	case IQA_PREVCLOSE_TIME:				return "IQA_PREVCLOSE_TIME";
	case IQA_SETTLEMENT_PX:					return "IQA_SETTLEMENT_PX";
	//case IQA_VWAP1_PX:					//return "IQA_VWAP1_PX";
	//case IQA_VWAP2_PX:					//return "IQA_VWAP2_PX";
	case IQA_VWAP_SUM:						return "IQA_VWAP_SUM";
	case IQA_VWAP_VOLUME:					return "IQA_VWAP_VOLUME";
	// Level 2								// Level 2
	case IQA_LVL2_BEGIN:					return "IQA_LVL2_BEGIN";
	case IQA_LVL2_END:						return "IQA_LVL2_END";
	case IQA_LVL2_GROUP:					return "IQA_LVL2_GROUP";
	case IQA_LVL2_REGION:					return "IQA_LVL2_REGION";
	case IQA_MMID:							return "IQA_MMID";
	case IQA_BROKERNO:						return "IQA_BROKERNO";
	// Level 3								// Level 3
	case IQA_LVL3_GROUP:					return "IQA_LVL3_GROUP";
	case IQA_CLORDID:						return "IQA_CLORDID";
	case IQA_ORDERID:						return "IQA_ORDERID";
	case IQA_MSGSEQNUM:						return "IQA_MSGSEQNUM";
	case IQA_SIDE:							return "IQA_SIDE";
	// Chart								// Chart
	case IQA_BAR_GROUP:						return "IQA_BAR_GROUP";
	case IQA_CHART_BEGIN:					return "IQA_CHART_BEGIN";
	case IQA_CHART_END:						return "IQA_CHART_END";
	case IQA_CHART_INTERVAL:				return "IQA_CHART_INTERVAL";
	// Option market center volumes
	case IQA_ASE_VOLUME:					return "IQA_ASE_VOLUME";
	case IQA_BOX_VOLUME:					return "IQA_BOX_VOLUME";
	case IQA_CBO_VOLUME:					return "IQA_CBO_VOLUME";
	case IQA_ISE_VOLUME:					return "IQA_ISE_VOLUME";
	case IQA_PSE_VOLUME:					return "IQA_PSE_VOLUME";
	case IQA_PHS_VOLUME:					return "IQA_PHS_VOLUME";
	case IQA_NDQ_VOLUME:					return "IQA_NDQ_VOLUME";
	case IQA_ASE_TICK:						return "IQA_ASE_TICK";
	case IQA_BOX_TICK:						return "IQA_BOX_TICK";
	case IQA_CBO_TICK:						return "IQA_CBO_TICK";
	case IQA_ISE_TICK:						return "IQA_ISE_TICK";
	case IQA_PSE_TICK:						return "IQA_PSE_TICK";
	case IQA_PHS_TICK:						return "IQA_PHS_TICK";
	case IQA_NDQ_TICK:						return "IQA_NDQ_TICK";
	// Extensions
	case IQA_TIMESTAMP:						return "IQA_TIMESTAMP";
	case IQA_SUMMARY:						return "IQA_SUMMARY";
	case IQA_CORRECTION_NO:					return "IQA_CORRECTION_NO";
	//case IQA_ORDTYPE:						return "IQA_ORDTYPE";
	//case IQA_EXECINST:					return "IQA_EXECINST";
	case IQA_IMBALANCE_PX:					return "IQA_IMBALANCE_PX";
	case IQA_IMBALANCE_SIZE:				return "IQA_IMBALANCE_SIZE";
	case IQA_IMBALANCE_SIDE:				return "IQA_IMBALANCE_SIDE";
	case IQA_IMBALANCE_PAIRED_SIZE:			return "IQA_IMBALANCE_PAIRED_SIZE";
	case IQA_IMBALANCE_STATUS:				return "IQA_IMBALANCE_STATUS";
	case IQA_IMBALANCE_TIME:				return "IQA_IMBALANCE_TIME";
	case IQA_CLOSE_SYMBOL:					return "IQA_CLOSE_SYMBOL";
	case IQA_BAT_VOLUME:					return "IQA_BAT_VOLUME";
	case IQA_BAT_TICK:						return "IQA_BAT_TICK";
	case IQA_C2_VOLUME:						return "IQA_C2_VOLUME";
	case IQA_C2_TICK:						return "IQA_C2_TICK";
	case IQA_COMPOSITE_BID:					return "IQA_COMPOSITE_BID";
	case IQA_COMPOSITE_ASK:					return "IQA_COMPOSITE_ASK";
	//case IQA_COUNT:							return "IQA_COUNT";
	};
	return "???";
}
// Market center names
const char *IQCspApi::GetMarketName(IQMarketCenter mc)
{
	switch(mc)
	{
	case IQMC_UNKNOWN: return "Unknown";
	case IQMC_OTCPINK: return "OTCPINK";
	case IQMC_OTCBB: return "OTCBB";
	case IQMC_OTCDUAL: return "OTCDUAL";
	case IQMC_OPRA: return "OPRA";
	case IQMC_ASE: return "ASE";
	case IQMC_BOX: return "BOX";
	case IQMC_CBO: return "CBO";
	case IQMC_ISE: return "ISE";
	case IQMC_NDQ: return "NDQ";
	case IQMC_PSE: return "PSE";
	case IQMC_PHS: return "PHS";
	case IQMC_BAT: return "BAT";
	case IQMC_C2: return "C2";
	case IQMC_NBX: return "MBX";
	case IQMC_MIA: return "MIA";
	case IQMC_GEM: return "GEM";
	case IQMC_OPRAIDX: return "OPRAIDX";
	case IQMC_NYSE: return "NYSE";
	case IQMC_NYSE_AMEX: return "NYSE_AMEX";
	case IQMC_NYSE_ARCA: return "NYSE_ARCA";
	case IQMC_NYSE_OPENBOOK: return "NYSE_OPENBOOK";
	case IQMC_ARCABOOK: return "ARCABOOK";
	case IQMC_MLXN: return "MLXN";
	case IQMC_BYX: return "BYX";
	case IQMC_BATS: return "BATS";
	case IQMC_CBSX: return "CBSX";
	case IQMC_CINN: return "CINN";
	case IQMC_CHX: return "CHX";
	//case IQMC_CTS: return "CTS";
	case IQMC_ISEQ: return "ISEQ";
	case IQMC_EDGA: return "EDGA";
	case IQMC_EDGX: return "EDGX";
	case IQMC_NASD: return "NASD";
	case IQMC_BSE: return "BSE";
	case IQMC_PHLX: return "PHLX";
	//case IQMC_CBOE: return "CBOE";
	case IQMC_AMEX_CQS: return "AMEX_CQS";
	case IQMC_NYSE_CQS: return "NYSE_CQS";
	case IQMC_NAS_CQS: return "NAS_CQS";
	case IQMC_NASDAQ: return "NASDAQ";
	case IQMC_NASL2: return "NASL2";
	case IQMC_NASL2_SHARES: return "NASL2S";
	case IQMC_NASDAQ_NIDS: return "NASDAQ_NIDS";
	case IQMC_NASDAQ_MFDS: return "NASDAQ_MFDS";
	case IQMC_NASDAQ_MONEY: return "IQMC_NASDAQ_MONEY";
	case IQMC_NASDAQ_SINGLEBOOK: return "NASDAQ_SINGLEBOOK";
	case IQMC_BLOOMBERG_TRADEBOOK: return "BLOOMBERG_TRADEBOOK";
	case IQMC_CME: return "CME";
	case IQMC_CMEMINI: return "CMEMINI";
	case IQMC_CMEBOOK: return "CMEBOOK";
	case IQMC_CME_CBOT: return "CME_CBOT";
	case IQMC_CME_CBOTMINI: return "CME_CBOTMINI";
	case IQMC_CBOTBOOK: return "CBOTBOOK";
	case IQMC_CBOT_OPTIONS: return "IQMC_CBOT_OPTIONS";
	case IQMC_CBOT_SPREADS: return "IQMC_CBOT_SPREADS";
	case IQMC_CME_NYMEX: return "CME_NYMEX";
	case IQMC_CME_COMEX: return "CME_COMEX";
	case IQMC_CME_OPTIONS: return "IQMC_CME_OPTIONS";
	case IQMC_CME_SPREADS: return "IQMC_CME_SPREADS";
	case IQMC_CME_KCBT: return "IQMC_CME_KCBT";
	case IQMC_CME_MINNEAPOLIS: return "IQMC_CME_MINNEAPOLIS";
	case IQMC_TSXL1: return "TSXL1";
	case IQMC_TSXL2: return "TSXL2";
	case IQMC_VENTUREL1: return "VENTUREL1";
	case IQMC_VENTUREL2: return "VENTUREL2";
	case IQMC_DJIDX: return "DJIDX";
	case IQMC_PBOTIDX: return "PBOTIDX";
	case IQMC_CBOTIDX: return "CBOTIDX";
	case IQMC_CBOEIDX: return "CBOEIDX";
	case IQMC_FEEDBITS: return "IQMC_FEEDBITS";
	case IQMC_COMSTOCK_TIME: return "COMSTOCK_TIME";
	case IQMC_IQTIME: return "IQTIME";
	};
	_ASSERT(false);
	return "Unknown";
}
// Map Comstock exchange/region to API market codes
IQAssetType IQCspApi::MarketCode(char exch, char reg, IQMarketCenter& mc)
{
	switch(exch)
	{
	case 2:
		mc=IQMC_COMSTOCK_TIME;
		//mc=IQMC_OPRA_TIME;
		return IQAST_TIME;
	case 8:
		mc=IQMC_NASL2_SHARES;
		return IQAST_EQUITY;
	case 9:
		mc=IQMC_NASL2;
		return IQAST_EQUITY;
	case 11:
		if(reg=='k')
			mc=IQMC_OTCBB;
		else if(reg=='p')
			mc=IQMC_OTCPINK;
		else
			mc=IQMC_OTCDUAL;
		return IQAST_EQUITY;
	case 12:
		if(reg=='a')
			mc=IQMC_ASE;
		else if(reg=='b')
			mc=IQMC_BOX;
		else if(reg=='c')
			mc=IQMC_CBO;
		else if(reg=='i')
			mc=IQMC_ISE;
		else if(reg=='p')
			mc=IQMC_PSE;
		else if(reg=='x')
			mc=IQMC_PHS;
		else if(reg=='q')
			mc=IQMC_NDQ;
		else if(reg=='z')
			mc=IQMC_BAT;
		else if(reg=='w')
			mc=IQMC_C2;
		else if(reg=='t')
			mc=IQMC_NBX;
		else if(reg=='m')
			mc=IQMC_MIA;
		else if(reg=='h')
			mc=IQMC_GEM;
		else
			mc=IQMC_OPRA;
		return IQAST_EQOPTION;
	case 13:
		if(reg=='a')
			mc=IQMC_AMEX_CQS;
		else if(reg=='b')
			mc=IQMC_BSE;
		else if(reg=='c')
			mc=IQMC_CINN;
		else if(reg=='d')
			mc=IQMC_NASD;
		else if(reg=='i')
			mc=IQMC_ISEQ;
		else if(reg=='j')
			mc=IQMC_EDGA;
		else if(reg=='k')
			mc=IQMC_EDGX;
		else if(reg=='m')
			mc=IQMC_CHX;
		else if(reg=='n')
			mc=IQMC_NYSE_CQS;
		else if(reg=='p')
			mc=IQMC_NYSE_ARCA;
		//else if(reg=='s')
		//	mc=IQMC_CTS; Consolidated Tape System
		else if(reg=='t')
			mc=IQMC_NAS_CQS;
		else if(reg=='w')
			mc=IQMC_CBSX;
		else if(reg=='x')
			mc=IQMC_PHLX;
		else if(reg=='z')
			mc=IQMC_BATS;
		else if(reg=='y')
			mc=IQMC_BYX;
		else if(exch==14)
			mc=IQMC_NYSE_AMEX;
		else// if(exch==13)
			mc=IQMC_NYSE;
		return IQAST_EQUITY;
	case 14:
		if(reg=='a')
			mc=IQMC_AMEX_CQS;
		else if(reg=='b')
			mc=IQMC_BSE;
		else if(reg=='c')
			mc=IQMC_CINN;
		else if(reg=='d')
			mc=IQMC_NASD;
		else if(reg=='i')
			mc=IQMC_ISEQ;
		else if(reg=='j')
			mc=IQMC_EDGA;
		else if(reg=='k')
			mc=IQMC_EDGX;
		else if(reg=='m')
			mc=IQMC_CHX;
		else if(reg=='n')
			mc=IQMC_NYSE_CQS;
		else if(reg=='p')
			mc=IQMC_NYSE_ARCA;
		//else if(reg=='s')
		//	mc=IQMC_CTS; Consolidated Tape System
		else if(reg=='t')
			mc=IQMC_NAS_CQS;
		else if(reg=='w')
			mc=IQMC_CBSX;
		else if(reg=='x')
			mc=IQMC_PHLX;
		else if(reg=='y')
			mc=IQMC_BYX;
		else if(reg=='z')
			mc=IQMC_BATS;
		else if(exch==14)
			mc=IQMC_NYSE_AMEX;
		else// if(exch==14)
			mc=IQMC_NYSE_AMEX;
		return IQAST_EQUITY;
	case 10:
		if((reg=='o')||(reg=='p'))
			mc=IQMC_NYSE_OPENBOOK;
		else if(reg=='a')
			mc=IQMC_ARCABOOK;
		else if(reg=='i')
			mc=IQMC_NASDAQ_SINGLEBOOK;
		else if(reg=='t')
			mc=IQMC_BLOOMBERG_TRADEBOOK;
		else if(reg=='m')
			mc=IQMC_CMEBOOK;
		else if(reg=='q')
			mc=IQMC_CBOTBOOK;
		else if((reg=='x')||(reg=='b'))
			mc=IQMC_MLXN;
		else
			mc=IQMC_UNKNOWN;
		return ((reg=='m')||(reg=='q')) ?IQAST_FUTURE :IQAST_EQUITY;
	case 15:
		mc=IQMC_NASDAQ;
		return IQAST_EQUITY;
	case 16:
		if(reg=='n')
			mc=IQMC_CME_NYMEX;
		else if(reg=='c')
			mc=IQMC_CME_COMEX;
		else if(reg=='o')
			mc=IQMC_CME_OPTIONS;
		else if(reg=='s')
			mc=IQMC_CME_SPREADS;
		else
			mc=IQMC_CME;
		return IQAST_FUTURE;
	case 17:
		if(reg=='t')
			mc=IQMC_TSXL1;
		else if(reg=='k')
			mc=IQMC_VENTUREL1;
		else
			mc=IQMC_UNKNOWN;
		return IQAST_EQUITY;
	case 18:
		if(reg=='t')
			mc=IQMC_TSXL2;
		else if(reg=='k')
			mc=IQMC_VENTUREL2;
		else
			mc=IQMC_UNKNOWN;
		return IQAST_EQUITY;
	case 19:
		if(reg=='e')
			mc=IQMC_CME_CBOTMINI;
		else if(reg=='o')
			mc=IQMC_CBOT_OPTIONS;
		else if(reg=='s')
			mc=IQMC_CBOT_SPREADS;
		else
			mc=IQMC_CME_CBOT;
		return IQAST_FUTURE;
	case 20:
		mc=IQMC_CME_KCBT;
		return IQAST_FUTURE;
	case 27:
		if(reg=='m')
			mc=IQMC_NASDAQ_MFDS;
		else if(reg=='n')
			mc=IQMC_NASDAQ_MONEY;
		else if(reg=='a')
			mc=IQMC_PBOTIDX;
		else if(reg=='b')
			mc=IQMC_CBOTIDX;
		else if(reg=='c')
			mc=IQMC_CBOEIDX;			
			//mc=IQMC_OPRAIDX;
			//mc=IQMC_DJIDX;
		else if(reg=='f')
		{
			mc=IQMC_FEEDBITS;
			return IQAST_EQINDEX;
		}
		else
			mc=IQMC_NASDAQ_NIDS;
		return (reg=='b') ?IQAST_FUTURE :IQAST_EQINDEX;
	case 28:
		mc=IQMC_CMEMINI;
		return IQAST_FUTURE;
	case 29:
		mc=IQMC_CME_MINNEAPOLIS;
		return IQAST_FUTURE;
	case 31: 
		mc=IQMC_IQTIME;
		return IQAST_TIME;
	};
	mc=IQMC_UNKNOWN;
	return IQAST_UNKNOWN;
}
IQAssetType IQCspApi::MarketCode(IQMarketCenter mc, char& exch, char& reg)
{
	switch(mc)
	{
	case IQMC_OTCPINK: exch=11; reg='p'; return IQAST_EQUITY;
	case IQMC_OTCBB: exch=11; reg='k'; return IQAST_EQUITY;
	case IQMC_OTCDUAL: exch=11; reg=0; return IQAST_EQUITY;
	case IQMC_OPRA: exch=12; reg=0; return IQAST_EQOPTION;
	case IQMC_ASE: exch=12; reg='a'; return IQAST_EQOPTION;
	case IQMC_BOX: exch=12; reg='b'; return IQAST_EQOPTION;
	case IQMC_CBO: exch=12; reg='c'; return IQAST_EQOPTION;
	case IQMC_ISE: exch=12; reg='i'; return IQAST_EQOPTION;
	case IQMC_NDQ: exch=12; reg='q'; return IQAST_EQOPTION;
	case IQMC_PSE: exch=12; reg='p'; return IQAST_EQOPTION;
	case IQMC_PHS: exch=12; reg='x'; return IQAST_EQOPTION;
	case IQMC_BAT: exch=12; reg='z'; return IQAST_EQOPTION;
	case IQMC_C2: exch=12; reg='w'; return IQAST_EQOPTION;
	case IQMC_NBX: exch=12; reg='t'; return IQAST_EQOPTION;
	case IQMC_MIA: exch=12; reg='m'; return IQAST_EQOPTION;
	case IQMC_GEM: exch=12; reg='h'; return IQAST_EQOPTION;
	case IQMC_OPRAIDX: exch=27; reg=0; return IQAST_EQINDEX;
	case IQMC_NYSE: exch=13; reg=0; return IQAST_EQUITY;
	case IQMC_NYSE_AMEX: exch=14; reg=0; return IQAST_EQUITY;
	case IQMC_NYSE_ARCA: exch=13; reg='p'; return IQAST_EQUITY;
	case IQMC_NYSE_OPENBOOK: exch=10; reg='p'; return IQAST_EQUITY;
	case IQMC_ARCABOOK: exch=10; reg='a'; return IQAST_EQUITY;
	case IQMC_MLXN: exch=10; reg=/*'x'*/'b'; return IQAST_EQUITY;
	case IQMC_BYX: exch=13; reg='y'; return IQAST_EQUITY;
	case IQMC_BATS: exch=13; reg='z'; return IQAST_EQUITY;
	case IQMC_CBSX: exch=13; reg='w'; return IQAST_EQUITY;
	case IQMC_CINN: exch=13; reg='c'; return IQAST_EQUITY;
	case IQMC_CHX: exch=13; reg='m'; return IQAST_EQUITY;
	//case IQMC_CTS: exch=13; reg=0; return IQAST_EQUITY;
	case IQMC_ISEQ: exch=13; reg='i'; return IQAST_EQUITY;
	case IQMC_EDGA: exch=13; reg='j'; return IQAST_EQUITY;
	case IQMC_EDGX: exch=13; reg='k'; return IQAST_EQUITY;
	case IQMC_NASD: exch=13; reg='d'; return IQAST_EQUITY;
	case IQMC_BSE: exch=13; reg='b'; return IQAST_EQUITY;
	case IQMC_PHLX: exch=13; reg='x'; return IQAST_EQUITY;
	case IQMC_AMEX_CQS: exch=13; reg='a'; return IQAST_EQUITY;
	case IQMC_NYSE_CQS: exch=13; reg='n'; return IQAST_EQUITY;
	case IQMC_NAS_CQS: exch=13; reg='t'; return IQAST_EQUITY;
	case IQMC_NASDAQ: exch=15; reg=0; return IQAST_EQUITY;
	case IQMC_NASL2: exch=9; reg=0; return IQAST_EQUITY;
	case IQMC_NASL2_SHARES: exch=8; reg=0; return IQAST_EQUITY;
	case IQMC_NASDAQ_NIDS: exch=27; reg=0; return IQAST_EQINDEX;
	case IQMC_NASDAQ_MFDS: exch=27; reg='m'; return IQAST_EQINDEX;
	case IQMC_NASDAQ_MONEY: exch=27; reg='n'; return IQAST_EQINDEX;
	case IQMC_NASDAQ_SINGLEBOOK: exch=10; reg='i'; return IQAST_EQUITY;
	case IQMC_BLOOMBERG_TRADEBOOK: exch=10; reg='t'; return IQAST_EQUITY;
	case IQMC_CME: exch=16; reg=0; return IQAST_FUTURE;
	case IQMC_CMEMINI: exch=28; reg=0; return IQAST_FUTURE;
	case IQMC_CMEBOOK: exch=10; reg='m'; return IQAST_FUTURE;
	case IQMC_CME_CBOT: exch=19; reg=0; return IQAST_FUTURE;
	case IQMC_CME_CBOTMINI: exch=19; reg='e'; return IQAST_FUTURE;
	case IQMC_CBOTBOOK: exch=10; reg='q'; return IQAST_FUTURE;
	case IQMC_CBOT_OPTIONS: exch=19; reg='o'; return IQAST_FUTURE;
	case IQMC_CBOT_SPREADS: exch=19; reg='s'; return IQAST_FUTURE;
	case IQMC_CME_NYMEX: exch=16; reg='n'; return IQAST_FUTURE;
	case IQMC_CME_COMEX: exch=16; reg='c'; return IQAST_FUTURE;
	case IQMC_CME_OPTIONS: exch=16; reg='o'; return IQAST_FUTURE;
	case IQMC_CME_SPREADS: exch=16; reg='s'; return IQAST_FUTURE;
	case IQMC_TSXL1: exch=17; reg='t'; return IQAST_EQUITY;
	case IQMC_TSXL2: exch=18; reg='t'; return IQAST_EQUITY;
	case IQMC_VENTUREL1: exch=17; reg='k'; return IQAST_EQUITY;
	case IQMC_VENTUREL2: exch=18; reg='k'; return IQAST_EQUITY;
	case IQMC_CME_KCBT: exch=20; reg=0; return IQAST_FUTURE;
	case IQMC_DJIDX: exch=27; reg=0; return IQAST_EQUITY;
	case IQMC_PBOTIDX: exch=27; reg='a'; return IQAST_EQINDEX;
	case IQMC_CBOTIDX: exch=27; reg='b'; return IQAST_FUTURE;
	case IQMC_CBOEIDX: exch=27; reg='c'; return IQAST_EQINDEX;
	case IQMC_FEEDBITS: exch=27; reg='f'; return IQAST_EQINDEX;
	case IQMC_CME_MINNEAPOLIS: exch=29; reg=0; return IQAST_FUTURE;
	case IQMC_COMSTOCK_TIME: exch=2; reg=0; return IQAST_TIME;
	case IQMC_IQTIME: exch=31; reg=0; return IQAST_TIME;
	case IQMC_BOFA: exch=15; reg=0; return IQAST_EQUITY;
	};
	exch=0; reg=0;
	return IQAST_UNKNOWN;
}
IQAssetType IQCspApi::ListingMarket(IQMarketCenter mc, IQMarketCenter& lexch)
{
	switch(mc)
	{
	case IQMC_OTCPINK: lexch=IQMC_OTCPINK; return IQAST_EQUITY;
	case IQMC_OTCBB: lexch=IQMC_OTCBB; return IQAST_EQUITY;
	case IQMC_OTCDUAL: lexch=IQMC_OTCDUAL; return IQAST_EQUITY;
	case IQMC_OPRA: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_ASE: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_BOX: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_CBO: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_ISE: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_NDQ: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_PSE: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_PHS: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_BAT: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_C2: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_NBX: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_MIA: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_GEM: lexch=IQMC_OPRA; return IQAST_EQOPTION;
	case IQMC_OPRAIDX: lexch=IQMC_OPRAIDX; return IQAST_EQINDEX;
	case IQMC_NYSE: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_NYSE_AMEX: lexch=IQMC_NYSE_AMEX; return IQAST_EQUITY;
	case IQMC_NYSE_ARCA: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_NYSE_OPENBOOK: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_ARCABOOK: lexch=IQMC_UNKNOWN; return IQAST_EQUITY;
	case IQMC_MLXN: lexch=IQMC_UNKNOWN; return IQAST_EQUITY;
	case IQMC_BYX: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_BATS: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_CBSX: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_CINN: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_CHX: lexch=IQMC_NYSE; return IQAST_EQUITY;
	//case IQMC_CTS: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_ISEQ: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_EDGA: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_EDGX: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_NASD: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_BSE: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_PHLX: lexch=IQMC_NYSE; return IQAST_EQUITY;
	//case IQMC_CBOE: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_AMEX_CQS: lexch=IQMC_NYSE_AMEX; return IQAST_EQUITY;
	case IQMC_NYSE_CQS: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_NAS_CQS: lexch=IQMC_NYSE; return IQAST_EQUITY;
	case IQMC_NASDAQ: lexch=IQMC_NASDAQ; return IQAST_EQUITY;
	case IQMC_NASL2: lexch=IQMC_UNKNOWN; return IQAST_EQUITY;
	case IQMC_NASL2_SHARES: lexch=IQMC_UNKNOWN; return IQAST_EQUITY;
	case IQMC_NASDAQ_NIDS: lexch=IQMC_NASDAQ_NIDS; return IQAST_EQINDEX;
	case IQMC_NASDAQ_MFDS: lexch=IQMC_NASDAQ_MFDS; return IQAST_EQINDEX;
	case IQMC_NASDAQ_MONEY: lexch=IQMC_NASDAQ_MONEY; return IQAST_EQINDEX;
	case IQMC_NASDAQ_SINGLEBOOK: lexch=IQMC_UNKNOWN; return IQAST_EQUITY;
	case IQMC_BLOOMBERG_TRADEBOOK: lexch=IQMC_UNKNOWN; return IQAST_EQUITY;
	case IQMC_CME: lexch=IQMC_CME; return IQAST_FUTURE;
	case IQMC_CMEMINI: lexch=IQMC_CMEMINI; return IQAST_FUTURE;
	case IQMC_CMEBOOK: lexch=IQMC_UNKNOWN; return IQAST_FUTURE;
	case IQMC_CME_CBOT: lexch=IQMC_CME_CBOT; return IQAST_FUTURE;
	case IQMC_CME_CBOTMINI: lexch=IQMC_CME_CBOTMINI; return IQAST_FUTURE;
	case IQMC_CBOTBOOK: lexch=IQMC_CME_CBOT; return IQAST_FUTURE;
	case IQMC_CBOT_OPTIONS: lexch=IQMC_CBOT_OPTIONS; return IQAST_FUTURE;
	case IQMC_CBOT_SPREADS: lexch=IQMC_CBOT_SPREADS; return IQAST_FUTURE;
	case IQMC_CME_NYMEX: lexch=IQMC_CME_NYMEX; return IQAST_FUTURE;
	case IQMC_CME_COMEX: lexch=IQMC_CME_COMEX; return IQAST_FUTURE;
	case IQMC_CME_OPTIONS: lexch=IQMC_CME_OPTIONS; return IQAST_FUTURE;
	case IQMC_CME_SPREADS: lexch=IQMC_CME_SPREADS; return IQAST_FUTURE;
	case IQMC_TSXL1: lexch=IQMC_TSXL1; return IQAST_EQUITY;
	case IQMC_TSXL2: lexch=IQMC_TSXL1; return IQAST_EQUITY;
	case IQMC_VENTUREL1: lexch=IQMC_VENTUREL1; return IQAST_EQUITY;
	case IQMC_VENTUREL2: lexch=IQMC_VENTUREL1; return IQAST_EQUITY;
	case IQMC_CME_KCBT: lexch=IQMC_CME_KCBT; return IQAST_FUTURE;
	case IQMC_DJIDX: lexch=IQMC_DJIDX; return IQAST_EQUITY;
	case IQMC_PBOTIDX: lexch=IQMC_PBOTIDX; return IQAST_EQINDEX;
	case IQMC_CBOTIDX: lexch=IQMC_CBOTIDX; return IQAST_FUTURE;
	case IQMC_CBOEIDX: lexch=IQMC_CBOEIDX; return IQAST_EQINDEX;
	case IQMC_CME_MINNEAPOLIS: lexch=IQMC_CME_MINNEAPOLIS; return IQAST_FUTURE;
	case IQMC_FEEDBITS: lexch=IQMC_FEEDBITS; return IQAST_EQINDEX;
	case IQMC_COMSTOCK_TIME: lexch=IQMC_COMSTOCK_TIME; return IQAST_TIME;
	case IQMC_IQTIME: lexch=IQMC_IQTIME; return IQAST_TIME;
	case IQMC_BOFA: lexch=IQMC_BOFA; return IQAST_EQUITY;
	};
	lexch=IQMC_UNKNOWN;
	return IQAST_UNKNOWN;
}

char IQCspApi::GetComstockChar(IQATTRENTRY *pattr)
{
	if(pattr->vlen<1)
		return 0;
	return *(char *)pattr->value;
}
int IQCspApi::GetComstockStr(IQATTRENTRY *pattr, char *sbuf, int slen)
{
	switch(pattr->attr)
	{
	case IQA_BOOK_LEVEL:
	case IQA_MMID: // f66
	case IQA_BROKERNO: // f95,f96
		return GetComstockMMID(pattr,sbuf,slen);
	case IQA_MULTI_SALE_COND: // f73
		return GetComstockMultiCode(pattr,sbuf,slen);
	default:
	{
		int tlen=pattr->vlen;
		if(slen<tlen)
			return -1;
		memcpy(sbuf,(const char *)pattr->value,pattr->vlen);
		return pattr->vlen -1;
	}
	};
	return -1;
}
int IQCspApi::GetComstockMMID(IQATTRENTRY *pattr, char *sbuf, int slen)
{
	switch(pattr->attr)
	{
	case IQA_BOOK_LEVEL:
	case IQA_MMID: // f66
	case IQA_BROKERNO: // f95,f96
	{
		if(pattr->vlen<8)
			return -1;
		if(slen<5)
			return -1;
		char *sptr=sbuf;
		// Two octal digits used for one letter
		char *digits=(char *)pattr->value;
		for(int i=0;(i<pattr->vlen)&&(digits[i])&&(digits[i+1]);i+=2)
		{
			unsigned char octval=(digits[i] -'0')*8 +(digits[i+1] -'0');
			if(octval==59)
				*sptr='_';
			else
				*sptr='A' +(octval -1); 
			sptr++; *sptr=0;
		}
		// Validate book MMID
		if(sbuf[1]=='_')
		{
			if((sbuf[2]!='_')||
			   (sbuf[0]<'A')||(sbuf[0]>'Z')||
			   (sbuf[3]<'a')||(sbuf[3]>'z'))
				return -1;
		}
		return (int)(sptr -sbuf);
	}
	};
	_ASSERT(false);
	return -1;
}
int IQCspApi::GetComstockMultiCode(IQATTRENTRY *pattr, char *sbuf, int slen)
{
	switch(pattr->attr)
	{
	case IQA_MULTI_SALE_COND: // f73
	{
		if(pattr->vlen<8)
			return -1;
		if(slen<5)
			return -1;
		char *sptr=sbuf;
		// Two octal digits used for one letter
		char *digits=(char *)pattr->value;
		for(int i=0;(i<pattr->vlen)&&(digits[i])&&(digits[i+1]);i+=2)
		{
			// We're using decimal, not octal digits
			unsigned char decval=(digits[i] -'0')*10 +(digits[i+1] -'0');
			if(!decval)
				*sptr=' ';
			else if((decval>0)&&(decval<27))
				*sptr='A' +(decval -1); 
			else if((decval>=27)&&(decval<37))
				*sptr='0' +(decval -27);
			sptr++; *sptr=0;
		}
		return (int)(sptr -sbuf);
	}
	};
	_ASSERT(false);
	return -1;
}
__int64 IQCspApi::GetComstockLong(IQATTRENTRY *pattr)
{
	__int64 ival=0;
	bool abs=false;
	char *digits=(char*)pattr->value;
	for(int i=0;(i<pattr->vlen)&&(digits[i]);i++)
	{
		if(i<2)
		{
			// optional region
			if((digits[i]>='a')&&(digits[i]<='z'))
				continue;
			if(digits[i]=='-') 
			{
				abs=true; continue;
			}
		}
		if(!myisdigit(digits[i]))
			break;
		ival*=10; ival+=(digits[i] -'0');
	}
	return ival;
}
__int64 IQCspApi::GetComstockShares(IQATTRENTRY *pattr, int lotsize)
{
	__int64 ival=0;
	bool abs=false;
	char *digits=(char*)pattr->value;
	for(int i=0;(i<pattr->vlen)&&(digits[i]);i++)
	{
		if(i<2)
		{
			// optional region
			if((digits[i]>='a')&&(digits[i]<='z'))
				continue;
			if(digits[i]=='-') 
			{
				abs=true; continue;
			}
		}
		if(!myisdigit(digits[i]))
			break;
		ival*=10; ival+=(digits[i] -'0');
	}
	if((!abs)&&(!pattr->reserved))
		ival*=lotsize;
	return ival;
}
double IQCspApi::GetComstockPrice(IQATTRENTRY *pattr)
{
	PRICE px;
	px.PriceCode=pattr->reserved;
	char *digits=(char*)pattr->value;
	for(int i=0;(i<pattr->vlen)&&(digits[i]);i++)
	{
		if(!i)
		{
			// optional region
			if((digits[i]>='a')&&(digits[i]<='z'))
			{
				px.Region=*digits; continue;
			}
			if(digits[i]=='-') 
			{
				px.PriceCode=-px.PriceCode; continue;
			}
		}
		// Handle bad legacy Comstock:
		// [11]kFMCBf72,0f2h41500f2l41500f2mU41500v-103
		if(myisdigit(digits[i]))
		{
			px.Integral*=10; px.Integral+=(digits[i] -'0');
		}
		// Fraction codes
		else if((digits[i]=='!')||//0x21 1/8
				(digits[i]=='"')||//0x22 1/4
				(digits[i]=='#')||//0x23 3/8
				(digits[i]=='$')||//0x24 1/2
				(digits[i]=='%')||//0x25 5/8
				(digits[i]=='&')||//0x26 3/4
				(digits[i]=='\''))//0x27 7/8
			px.PriceCode=digits[i];
	}
	return (double)px;
}
