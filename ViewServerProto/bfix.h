#ifndef _BFIX_H
#define _BFIX_H

#define BFIX_SCHEMA_1_2 0x0102
#define BFIX_SCHEMA_2_0 0x0200
#define BFIX_SCHEMA_2_3 0x0203
#define BFIX_SCHEMA_2_7 0x0207
#define BFIX_CUR_SCHEMA BFIX_SCHEMA_2_0
#define COMPID_LEN 16
typedef void *pcFixSession;
#include "wstring.h"
#include <stdio.h>

#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_1_2)

#define OFT_ACCOUNT			1
#define OFT_AVGPX			6
#define OFT_BEGINSEQNO		7
#define OFT_BEGINSTRING		8
#define OFT_BODYLENGTH		9
#define OFT_CHECKSUM		10
#define OFT_CLORDID			11
#define OFT_COMMISSION		12
#define OFT_COMMTYPE		13
#define OFT_CUMQTY			14
#define OFT_CURRENCY		15
#define OFT_ENDSEQNO		16
#define OFT_EXECID			17
#define OFT_EXECINST		18
#define OFT_EXECTRANSTYPE	20
#define OFT_HANDLINST		21
#define OFT_LASTCAPACITY	29
#define OFT_LASTMKT			30
#define OFT_LASTPX			31
#define OFT_LASTQTY			32
#define OFT_MSGSEQNUM		34
#define OFT_MSGTYPE			35
#define OFT_NEWSEQNUM		36
#define OFT_ORDERID			37
#define OFT_ORDERQTY		38
#define OFT_ORDSTATUS		39
#define OFT_ORDTYPE			40
#define OFT_ORIGCLORDID		41
#define OFT_POSSDUPFLAG		43
#define OFT_PRICE			44
#define OFT_RULE80A			47
#define OFT_SENDERCOMPID	49
#define OFT_SENDERSUBID		50
#define OFT_SENDINGTIME		52
#define OFT_SIDE			54
#define OFT_SYMBOL			55
#define OFT_TARGETCOMPID	56
#define OFT_TARGETSUBID		57
#define OFT_TEXT			58
#define OFT_TIMEINFORCE		59
#define OFT_TRANSACTTIME	60
#define OFT_SYMBOLSFX		65
#define OFT_LISTID			66
#define OFT_EXECBROKER		76
#define OFT_POSITIONEFFECT	77
#define OFT_ENCRYPTMETHOD	98
#define OFT_STOPPX			99
#define OFT_EXDESTINATION	100
#define OFT_CXLREJREASON	102
#define OFT_ORDREJREASON	103
#define OFT_HEARTBTINT		108
#define OFT_CLIENTID		109
#define OFT_MAXFLOOR		111
#define OFT_TESTREQID		112
#define OFT_LOCATEREQD		114
#define OFT_ONBEHALFOFCOMPID 115
#define OFT_ONBEHALFOFSUBID 116
#define OFT_GAPFILLFLAG		123
#define OFT_EXPIRETIME		126
#define OFT_DELIVERTOCOMPID	128
#define OFT_DELIVERTOSUBID	129
#define OFT_EXECTYPE		150
#define OFT_LEAVESQTY		151
#define OFT_SECURITYTYPE	167
#define OFT_EFFECTIVETIME	168
#define OFT_SECONDARYORDERID 198
#define OFT_MATURITYMONTHYEAR 200
#define OFT_STRIKEPRICE		202
#define OFT_CUSTOMERORFIRM	204
#define OFT_MATURITYDATE	205
#define OFT_SECURITYEXCHANGE 207
#define OFT_PEGOFFSETVALUE	211
#define OFT_SOLICITEDFLAG	377
#define OFT_DISCRETIONINST	388
#define OFT_DISCRETIONOFFSETVALUE 389
#define OFT_EXPIREDATE		432
#define OFT_CXLREJRESPONSETO 434
#define OFT_CLEARINGFIRM	439
#define OFT_CLEARINGACCOUNT 440
#define OFT_CFICODE			461
#define OFT_ORDERRESTRICTIONS 529
#define OFT_CASHMARGIN		544
#define OFT_ROUTENAME		2000
#define OFT_SPECORDERTYPE	2001
#define OFT_EXPIRATIONTYPE	2002
#define OFT_DISPLAYTYPE		2003
#define OFT_CASHORMARGIN	2004
#define OFT_CLPARENTID		2005
#define OFT_ATEPARENTID		2007
#define OFT_SHORTSELLCODE	2008
#define OFT_OMSORDERNO		2009
#define OFT_OMSORIGORDERNO	2010
#define OFT_OMSPARENTORDERNO 2011
#define OFT_OMSDOMAIN		2012
#define OFT_OMSACCOUNT		2013
#define OFT_OMSUSER			2014

#define OFT_SECONDARYORDERID 198
#define OFT_ECNORDERID		2100
#define OFT_ECNCHAINID		2101
#define OFT_ECNID			2102
#define OFT_ECNCONTRA		2103
#define OFT_TIMEPLACEDWITHECN 2104
#define OFT_ECNEXECID		2105

#define OFT_ECNREFID		2107

#define OFT_REFORDID		2200
#define OFT_REFORIGID		2201
#define OFT_REFCHAINID		2202
#define OFT_REFPARENTID		2203
#define OFT_REFACCOUNT		2204
#define OFT_REFDOMAIN		2205
#define OFT_REFUSER			2206

#define OFT_ORD_CUMPX		2601
#define OFT_ORD_AVGPX		2602
#define OFT_ORD_CUMQTY		2603
#define OFT_ORD_ORDSTATUS	2604
#define OFT_ORD_CRSTATUS	2605
#define OFT_ORD_LEAVESQTY	2606
#define OFT_ORD_CURRENTORDERQTY	2607
#define OFT_ORD_REMAININGQTY 2608
#define OFT_ORD_OPENQTY		2609
#define OFT_ORD_PENDINGQTY	2621
#define OFT_REP_CUMPX		2610
#define OFT_REP_AVGPX		2611
#define OFT_REP_CUMQTY		2612
#define OFT_REP_ORDSTATUS	2613
#define OFT_REP_CRSTATUS	2614
#define OFT_REP_LEAVESQTY	2615
#define OFT_REP_CURRENTORDERQTY	2616
#define OFT_REP_REMAININGQTY	2617
#define OFT_REP_OPENQTY		2618
#define OFT_REP_PENDINGQTY	2622

#define OFT_NUM_FILLS		2619
#define OFT_STRATEGY_CODEPAGE 2620

#define OFT_CLDOMAIN		2801
#define OFT_ATESUFFIX		2801
#define OFT_TARGETSTRATEGY	5900
#define OFT_DURATION		5904
#define OFT_PARTICIPATIONRATE 5905
#define OFT_EXECUTIONMODE	5906
#define OFT_RANDOMRESERVEDISPLAYRANGE 8020
#define OFT_ADDREMOVELIQUIDITY 9960

#define OFTN_OBJECTTYPE				"ObjectType"
#define OFTN_ATEMATCHDOMAIN			"AteMatchDomain"
#define OFTN_ATEMATCHACCOUNT		"AteMatchAccount"
#define OFTN_ATEMATCHPARENT			"AteMatchParent"
#define OFTN_ATEMATCHCHILD			"AteMatchChild"
#define OFTN_ATEEXPORTDIR			"AteExportDir"
#define OFTN_ATEEXPORTDOMAIN		"AteExportDomain"
#define OFTN_ATEEXPORTACCOUNT		"AteExportAccount"

#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_7)
#define OFTN_REFORDID				"RefOrdID"
#define OFTN_REFORIGID				"RefOrigID"
#define OFTN_REFCHAINID				"RefChainID"
#define OFTN_REFPARENTID			"RefParentID"
#define OFTN_REFACCOUNT				"RefAccount"
#define OFTN_REFDOMAIN				"RefDomain"
#define OFTN_REFUSER				"RefUser"
// Replaced by OFTN_SECONDARYCLORDID
//#define OFTN_ECNORDERID				"EcnOrderID"
#define OFTN_ECNLINEID				"EcnLineID"
#define OFTN_ECNCHAINID				"EcnChainID"
#define OFTN_ECNID					"EcnID"
#define OFTN_ECNCONTRA				"EcnContra"
#define OFTN_TIMEPLACEDWITHECN		"TimePlacedWithEcn"
#define OFTN_ECNEXECID				"EcnExecID"
#define OFTN_ECNREFID				"EcnRefID"
// Replaced by OFTN_HYDRAROUTE
//#define OFTN_ROUTENAME				"RouteName"
#define OFTN_SPECORDERTYPE			"SpecOrderType"
#define OFTN_EXPIRATIONTYPE			"ExpirationType"
#define OFTN_DISPLAYTYPE			"DisplayType"
#define OFTN_DURATION				"Duration"
#define OFTN_EXECUTIONMODE			"ExecutionMode"
#define OFTN_RANDOMRESERVEDISPLAYRANGE	"RandomReserveDisplayRange"
#define OFTN_ADDREMOVELIQUIDITY		"AddRemoveLiquidity"
#define OFTN_DROPREFID				"DropRefID"
#define OFTN_IDSOURCE				"IDSource"
#define OFTN_CONTRACTMULITPLIER		"ContractMultiplier"
#define OFTN_PEGDIFFERENCE			"PegDifference"
#define OFTN_DISCRETIONOFFSET 		"DiscretionOffset"
#define OFTN_OPENCLOSE				"OpenClose"
#define OFTN_LIQUIDITYCODE          "LiquidityCode"
#define OFTN_REPORTTOEXCH			"ReportToExch"
#define OFTN_SHORTSELLCODE			"ShortSellCode"

#define OFTN_OMSORDERNO				"OmsOrderNo"
#define OFTN_OMSORIGORDERNO			"OmsOrigOrderNo"
#define OFTN_OMSPARENTORDERNO		"OmsParentOrderNo"
#define OFTN_OMSDOMAIN				"OmsDomain"
#define OFTN_OMSACCOUNT				"OmsAccount"
#define OFTN_OMSUSER				"OmsUser"
#define OFTN_OMSLOCK				"OmsLock"
#define OFTN_OMSLOCKKEY				"OmsLockKey"
#define OFTN_OMSDROPCOPY			"OmsDropCopy"

#define OFTN_CLPARENTID				"ClParentID"
#define OFTN_ATEPARENTID			"ATEParentID"
#define OFTN_ATESUFFIX				"ATESuffix"
#define OFTN_CLDOMAIN				"ClDomain"

#define OFTN_ORD_CUMPX				"OrdCumPx"
#define OFTN_ORD_AVGPX				"OrdAvgPx"
#define OFTN_ORD_CUMQTY				"OrdCumQty"
#define OFTN_ORD_ORDSTATUS			"OrdOrdStatus"
#define OFTN_ORD_CRSTATUS			"OrdCrStatus"
#define OFTN_ORD_LEAVESQTY			"OrdLeavesQty"
#define OFTN_ORD_CURRENTORDERQTY	"OrdCurrentOrderQty"
#define OFTN_ORD_REMAININGQTY		"OrdRemainingQty"
#define OFTN_ORD_OPENQTY			"OrdOpenQty"
#define OFTN_REP_CUMPX				"RepCumPx"
#define OFTN_REP_AVGPX				"RepAvgPx"
#define OFTN_REP_CUMQTY				"RepCumQty"
#define OFTN_REP_ORDSTATUS			"RepOrdStatus"
#define OFTN_REP_CRSTATUS			"RepCrStatus"
#define OFTN_REP_LEAVESQTY			"RepLeavesQty"
#define OFTN_REP_CURRENTORDERQTY	"RepCurrentOrderQty"
#define OFTN_REP_REMAININGQTY		"RepRemainingQty"
#define OFTN_REP_OPENQTY			"RepOpenQty"

#define OFTN_NUM_FILLS				"NumFills"
#define OFTN_STRATEGY_CODEPAGE		"StrategyCodePage"

#define OFTN_DROPCOPY				"DropCopy"
#define OFTN_HOSTPTR				"HostPtr"
#define OFTN_HALTED					"Halted"
#define OFTN_SUSPENDED				"Suspended"

#elif(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
#define OFTN_BEGINSTRING			"BeginString"
#define OFTN_BODYLENGTH				"BodyLength"
#define OFTN_CHECKSUM				"CheckSum"
#define OFTN_MSGSEQNUM				"MsgSeqNum"
#define OFTN_MSGTYPE				"MsgType"
#define OFTN_SENDERCOMPID			"SenderCompID"
#define OFTN_SENDERSUBID			"SenderSubID"
#define OFTN_SENDINGTIME			"SendingTime"
#define OFTN_TARGETCOMPID			"TargetCompID"
#define OFTN_TARGETSUBID			"TargetSubID"
#define OFTN_CHKSUM					"ChkSum"
#define OFTN_DROPCOPY				"DropCopy"
#define OFTN_HOSTPTR				"HostPtr"
#endif

#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
#else
#define MAXTAGS 1024
#endif
typedef struct tdFixTag
{
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
    int no;
    WORD tstart;
	bool stag;
	unsigned char tnlen;
    unsigned char vlen;
	const char *name;
    const char *val;
#else
    int no;
    WORD tstart;
    WORD vlen;
    const char *val;
#endif
} FIXTAG;
typedef struct tdFixConInfo
{
	int PortType;
	int PortNo;
	char LogonSenderCompID[COMPID_LEN];
	char LogonSenderSubID[COMPID_LEN];
	char LogonTargetCompID[COMPID_LEN];
	char LogonTargetSubID[COMPID_LEN];
	char OnBehalfOfCompID[COMPID_LEN];
	char OnBehalfOfSubID[COMPID_LEN];
	char Account[32];
	int Proto;
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_7)
	pcTmlSession mTmlSess;
#endif
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	pcFixSession mFixSess;
#endif
} FIXCONINFO;

class FIXINFO
{
public:
	FIXINFO();
	~FIXINFO();

	inline FIXTAG *GetTag(int tno)
	{
		for(int i=0;i<ntags;i++)
		{
			if(tags[i].no==tno)
				return &tags[i];
		}
		return 0;
	}
	char TagStrStr[16][256];
	int TagStrIdx;
	inline const char *TagStr(int tno) 
	{ 
		//static char TagStrStr[16][256];
		//static int TagStrIdx=-1;
		TagStrIdx=(TagStrIdx+1)%16;
		char *TagStr=TagStrStr[TagStrIdx];
		TagStr[0]=0;

		FIXTAG *ftag=GetTag(tno);
		if(ftag && ftag->val)
		{
			memcpy(TagStr,ftag->val,ftag->vlen);
			TagStr[ftag->vlen]=0;
		}
		return TagStr;
	}
	inline int TagInt(int tno) 
	{ 
		FIXTAG *ftag=GetTag(tno);
		// atol does not call strlen and does not require a null-terminated string
		return ftag && ftag->val && ftag->vlen>0 ?atoi(ftag->val) :0; 
	}
	inline double TagFloat(int tno) 
	{ 
		FIXTAG *ftag=GetTag(tno);
		// atof calls strlen and requires a null-terminated string
		//return ftag && ftag->val && ftag->vlen>0 ?atof(ftag->val) :0.0; 
		if(ftag && ftag->val && ftag->vlen>0 && ftag->vlen<128)
		{
			char vstr[128];
			memcpy(vstr,ftag->val,ftag->vlen);
			vstr[ftag->vlen]=0;
			return atof(vstr);
		}
		else
			return 0.0;
	}
	inline char TagChar(int tno) 
	{ 
		FIXTAG *ftag=GetTag(tno);
		return ftag && ftag->val && ftag->vlen>0 ?ftag->val[0] :0; 
	}
	FIXTAG *AddTag(int tag, const char *val, int tstart, int vlen);
	int FixMsgReady(char *Msg, int MsgLen);
	inline void Reset()
	{
		fbuf=0; llen=0; ntags=0;
	}
	void Copy(const FIXINFO *ifix, bool deepCopy=false);
	void RebaseFixInfo(char *Msg);
	void RebaseSetInfo(char *Msg);
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_3)
	inline FIXTAG *SetDynTag(const char *tname, FIXTAG *ftag)
	{ 
#error No static variables please
		static set<string> dynTagNames;
		if(ftag)
		{
			const char *dtname=0;
			set<string>::iterator it=dynTagNames.find(tname);
			if(it==dynTagNames.end())
			{
				dynTagNames.insert(tname);
				it=dynTagNames.find(tname);
			}
			dtname=it->c_str();
			return _SetTag(0,dtname,ftag->val,ftag->vlen); 
		}
		else
			DelTag(tname);
		return 0;
	}
#endif
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	// Dynamic message generation
	FIXTAG *AddTag(int tag, const char *tname, int tnlen, const char *val, int tstart, int vlen);
	FIXTAG *_SetTag(int tno, const char *tname, const char *val, int vlen);
	inline FIXTAG *SetTag(int tno, const char *val, int vlen=0)
	{
		return _SetTag(tno,0,val,vlen?vlen:(int)strlen(val));
	}
	inline FIXTAG *SetTag(int tno, FIXTAG *ftag)
	{ 
		if(ftag)
			return _SetTag(tno,0,ftag->val,ftag->vlen); 
		else
			DelTag(tno);
		return 0;
	}
	inline FIXTAG *SetTag(int tno, char val)
	{ 
		return _SetTag(tno,0,&val,val?1:0); 
	}
	inline FIXTAG *SetTag(int tno, int val)
	{
		char vstr[32]={0};
		sprintf_s(vstr,sizeof(vstr),"%d",val);
		return _SetTag(tno,0,vstr,(int)strlen(vstr));
	}
	inline FIXTAG *SetTag(int tno, double val)
	{
		char vstr[64]={0};
		if(val==0.0)
			sprintf_s(vstr,sizeof(vstr),"0.0",val);
		else
			sprintf_s(vstr,sizeof(vstr),"%.4f",val);
		return _SetTag(tno,0,vstr,(int)strlen(vstr));
	}
	void DelTag(int tno);
	int Merge();

	// Name-based tags
	inline FIXTAG *GetTag(const char *tname)
	{
		int tnlen=(int)strlen(tname);
		for(int i=0;i<ntags;i++)
		{
			if((tags[i].name)&&(tags[i].tnlen==tnlen)&&(!strincmp(tags[i].name,tname,tags[i].tnlen)))
				return &tags[i];
		}
		return 0;
	}
	inline const char *TagStr(const char *tname) 
	{ 
		//static char TagStrStr[16][256];
		//static int TagStrIdx=-1;
		TagStrIdx=(TagStrIdx+1)%16;
		char *TagStr=TagStrStr[TagStrIdx];
		TagStr[0]=0;

		FIXTAG *ftag=GetTag(tname);
		if(ftag && ftag->val)
		{
			memcpy(TagStr,ftag->val,ftag->vlen);
			TagStr[ftag->vlen]=0;
		}
		return TagStr;
	}
	inline int TagInt(const char *tname) 
	{ 
		FIXTAG *ftag=GetTag(tname);
		return ftag && ftag->val && ftag->vlen>0 ?atoi(ftag->val) :0; 
	}
	inline double TagFloat(const char *tname) 
	{ 
		FIXTAG *ftag=GetTag(tname);
		return ftag && ftag->val && ftag->vlen>0 ?atof(ftag->val) :0; 
	}
	inline char TagChar(const char *tname) 
	{ 
		FIXTAG *ftag=GetTag(tname);
		return ftag && ftag->val && ftag->vlen>0 ?ftag->val[0] :0; 
	}
	//FIXTAG *AddTag(const char *tname, const char *val, int tstart, int vlen);
	//FIXTAG *SetTag(const char *tname, const char *val, int vlen);
	inline FIXTAG *SetTag(const char *tname, const char *val, int vlen=0)
	{
		return _SetTag(0,tname,val,vlen?vlen:(int)strlen(val));
	}
	inline FIXTAG *SetTag(const char *tname, FIXTAG *ftag)
	{ 
		if(ftag)
			return _SetTag(0,tname,ftag->val,ftag->vlen); 
		else
			DelTag(tname);
		return 0;
	}
	inline FIXTAG *SetTag(const char *tname, char val)
	{ 
		return _SetTag(0,tname,&val,val?1:0); 
	}
	inline FIXTAG *SetTag(const char *tname, int val)
	{
		char vstr[128]={0};
		sprintf_s(vstr,sizeof(vstr),"%d",val);
		return _SetTag(0,tname,vstr,(int)strlen(vstr));
	}
	inline FIXTAG *SetTag(const char *tname, double val)
	{
		char vstr[128]={0};
		if(val==0.0)
			sprintf_s(vstr,sizeof(vstr),"0.0",val);
		else
			sprintf_s(vstr,sizeof(vstr),"%.4f",val);
		return _SetTag(0,tname,vstr,(int)strlen(vstr));
	}
	void DelTag(const char *tname);
#endif

public:
	FIXCONINFO *fci;
	// Recv FIX
	char *fbuf;
	int ntags;
#if(BFIX_CUR_SCHEMA>=BFIX_SCHEMA_2_0)
	FIXTAG *tags;
	int maxtags;
	char *sbuf;
	int slen;
	int ssize;
#else
	FIXTAG tags[MAXTAGS];
#endif
	int llen;
	unsigned char chksum;
	bool supressChkErr;
	bool noSession;
	unsigned char FIXDELIM;
	unsigned char FIXDELIM2;
};

#define IPORTSPEC(ifix) ifix->fci?PortSpec(ifix->fci->PortType,ifix->fci->PortNo):PortSpec(-1,-1)

#endif//BFIX_CUR_SCHEMA>=BFIX_SCHEMA_1_2

#endif//_BFIX_H
