
#include "stdafx.h"
#include "IQMatrix.h"
#include "SMConsole.h"
#include "wstring.h"

#ifndef DIALOG_DLLS
// Copied from legacy iqserver
#pragma pack(push,8)
#define CUR_UNKNOWN		0
#define CUR_USD			1
#define CUR_CAD			2
#define CUR_UNRESOLVED	255

struct _CSPMDR
{								// CSP Token (Bate)
    int pxyPortType;
    int pxyPortNo;
	BOOL reqlearn;
	BOOL reqlvl1;
	BOOL reqlvl2;
	BOOL recvdlearn;
	BOOL recvdlvl1;
	BOOL recvdlvl2;
	BOOL dereglvl1;
	BOOL dereglvl2;
	BOOL reflvl1;
	BOOL reflvl2;
	// Old IQMDR below
	//XTCMSG	XtcMsg;
	long	lSymbolId;							// $$$$ Local Symbol Id
	char	cExchange;							// *0114 (*) Exchange Id	
	char	cRegion;							// *0116 (*) Regional Id
	char	cSymbol[20];						// *0000 (*) Ticker Symbol
	char	cMDXQuoteCon;							
	char	cAlert;								//  ???? (g)
	char	cUpdate;							//  ???? (f72)
	char	cNull;
	char	cQuoteCondition;					//  ???? (u)
	char	cFormat;							//  ???? (f0-f9 and f71)	
	//--------------------------------------------------------- Mirror IQBAR (32 Bytes)
	long	lDate;								//  0137 (d)
	long	lTime;								//  0130 (z)
	float	fOpen;								//  0109 (o)
	float	fHigh;								//  0104 (h)
	float	fLow;								//  0105 (l)
	float	fLast;								//  ???? (t)	
	long	lVol;								//  0106 (v)
	long	lOi;								//  0110 (e)
	//--------------------------------------------------------- Mirror IQLVL2
	float	fBid;								//  0102 (b,q)
	float	fAsk;								//  0103 (a,r)
	long	lBidSize;							//  0118 (j,p)
	long	lAskSize;							//  0119 (k,x)
	long	lLastSize;							//  0112 (i) Incremental Volume
	char	cSalesCondition;					//  ???? (f79)
	char	cLastRegion;						//  0117 (*)
	char	cBidRegion;							//  0120 (*)
	char	cAskRegion;							//  0121 (*)
	long	lPrevVol;							//  0126 (w)
	float	fPrevClose;							//  0111 (y)
	//---------------------------------------------------------
	float	fClose;								//  0101 (c)
//	char	cCurrencyCode[4];					//  0122 (m)
	long	TickLastVol;
	float	fNetChange;							//  0108 (n)
	float	fSettlePrice;						//  0107 (s)
	long	lRegionBidSize;						//  ???? (f59)
	long	lRegionAskSize;						//  ???? (f60)
	float	vWap1;								//  ???? (q)
	float	vWap2;								//  ???? (r)
	//=========================================================
	long	BarRec;								// Prev 1 Min Bar
	long	nBarRec;							// Number Entries
	char    Expressbid;
	char    Expressask;
	char    OrderInbalance;
	char    IIndication;
	int		nbboTime;							// Last NBBO time
	long	Lvl2Rec;							// Level 2 Record
	long	nLvl2Rec;							// Number Entries
	float	fSimClose;
	float	fSimPrevClose;
	//---------------------------------------------------------
	// Extended Bate Codes
	//long	lExpDate;							// 0127 (f54, YYMMDD )
	//float	fStrike;							// 0128 (f55, ?????? )
	//long	lRes1;						// Task List1
	//BYTE	cType; 
	//BYTE	cUPC;
	//BYTE	cFlag3;
	//BYTE	cFlag4;
	//---------------------------------------------------------
	//char	cRes16[16];
	//long	lBarDate;
	//long	lBarTime;
	//float	fBarOpen;
	//float	fBarHigh;
	//float	fBarLow;
	//float	fBarLast;
	//long	lBarVol;
	//long	lBarOi;
	//float   fFormTVolume;
	//float   fLastRegular;
	//float	TickLastPrice;
	//---------------------------------------------------------
	// Extended Bate Codes
	int	lExpDate;							// 0127 (f54, YYMMDD )
	float	fStrike;							// 0128 (f55, ?????? )
	//WORD    PHSTick;
	//WORD    ISETick;
	//char cRes4[4];
	int		llotSize;
//	int	lRes3;						// Task List1
	BYTE	cType; 
	BYTE	cUPC;
	BYTE	cTestSymbol;
	BYTE	cFlag4;
	//---------------------------------------------------------
	//WORD    ASEVol;
	//WORD    CBOVol;
	//WORD    PSEVol;
	//WORD    PHSVol;
	//WORD    ISEVol;
	//WORD    ASETick;
	//WORD    CBOTick;
	//WORD    PSETick;
	__int64 llVol;
	__int64 llPrevVol;
	//char	cRes16[6];
	//---------------------------------------------------------
	int	lBar1Date;
	int	lBar1Time;
	float	fBar1Open;
	float	fBar1High;
	float	fBar1Low;
	float	fBar1Last;
	int	lBar1Vol;
	int	lBar1Oi;
	float   fFormTVolume;
	float   fLastRegular;
	float	TickLastPrice;
	//---------------------------------------------------------
	char    cReset;
	char	Tick;
	char	LastTickDay;
	char	Trin;
	char	cRes[16];
};

typedef struct
{	long	lId;
	long	lPrev;	
	long	lNext;
	WORD	uRec;
	WORD	cbRec;
} _VDBREC;

#define SO_CSPSYMBOL	20

struct _CSPADDREC
{	
	//union{
	//	_VDBREC	VdbRec;
	//	XTCMSG XtcMsg;
	//};
	long	lMdr;
	char	cExchange;
	char	cRegion;
	char	cSymbol[SO_CSPSYMBOL];
	char	cPad1[2];		// struct align 8
	float	fSimClose;
	float	fSimPrevClose;
	float	fSettlePrice;
	long	PrevCloseDate;
	long	PrevCloseTime;
	long	LastTradeDate;
	long	LastTradeTime;
	char	Currency;
	char	cPad2[3];		// struct align 8
	long	BSEVol;								
	long    ASEVol;
	long    CBOVol;
	long    PSEVol;
	long    PHSVol;
	long    ISEVol;
	long	BSETick;
	long    ASETick;
	long    CBOTick;
	long    PSETick;
	long    PHSTick;
	long    ISETick;
	long	NDQVol;
	long	NDQTick;
	char	ListingEx;
	char	cPad3[3];		// struct align 8
	double  fVwapSum;
	long	lVwapVol;
	float	fImbalancePrice;
	int		lImbalanceSize;
	int		lPairedSize;
	int		iImbalanceTime;
	char	cImbalanceSide;
	char	cImbalanceStatus;
	char	cPad4[2];		// struct align 8
	long	BATVol;
	long	BATTick;
	long	C2Vol;
	long	C2Tick;
	float	fPinkBid;
	float	fPinkAsk;
	long	lPinkBidSize;
	long	lPinkAskSize;
	long	lCompBidSize;
	long	lCompAskSize;
	long	NBXVol;
	long	NBXTick;
	long	MIAVol;
	long	MIATick;
	char	reserved[24];	// Pad to 256 bytes total
	char	cDoNotUse[8];	// legacy bug only sends the first 248 bytes
	char	reserved2[10];	// Replaces _VDBREC
	// We don't really have to match iqserver's definition exactly, 
	// because this definition is solely used for the dialogs.
	char	cFullName[40];
};

struct _CSPLVLXREC
{	//XTCMSG	XtcHdr; 
	char	cSymbol[20];
	long	lDate;
	long	lTime;
	char	cExchange;
	char	cRegion;
	char	cMMID[4];
	char	cRes1;								// Reserved for NULL terminator
	char	cSalesCondition;
	float	fBid;
	float	fAsk;
	long	lBidSize;
	long	lAskSize;
};

struct _CSPLVLXREC2 :public _CSPLVLXREC
{
	char	csBid[96];
	char	csAsk[96];
};
#pragma pack(pop)

static BOOL _ApplyFastLvl1=false;

#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif
LRESULT CALLBACK cbLevel1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT lrect;
	HWND lwnd=GetDlgItem(hDlg,IDC_LIST);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SMConsoleDlg *pdlg=(SMConsoleDlg *)lParam;
		SetWindowLong(hDlg,GWL_USERDATA,(DWORD)(PTRCAST)pdlg);
		LVCOLUMN lcols[2]={
			{LVCF_WIDTH|LVCF_TEXT,0,100,"Field",0,0,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,120,"Value",0,1,0,0},
		};
		int i;
		POINT p1,p2;

		ListView_SetExtendedListViewStyle(lwnd, LVS_EX_FULLROWSELECT);
		for (i=0;i<2;i++)
			ListView_InsertColumn(lwnd,i,&lcols[i]);

		GetWindowRect(lwnd,&lrect);
		p1.x=lrect.left; p1.y=lrect.top; p2.x=lrect.right; p2.y=lrect.bottom;
		ScreenToClient(hDlg,&p1); ScreenToClient(hDlg,&p2);
		lrect.left=p1.x; lrect.top=p1.y; lrect.right=p2.x; lrect.bottom=p2.y;
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
        case IDC_SYMBOL:
        {
            if (HIWORD(wParam)==EN_CHANGE)
            {
				SMConsoleDlg *pdlg=(SMConsoleDlg *)(PTRCAST)GetWindowLong(hDlg,GWL_USERDATA);
				_CSPMDR Mdr;
				long lMdr;
				char cSymbol[SO_CSPSYMBOL+1]={0},cExchange=0,cRegion=0,*sptr;
				LVITEM litem;
				char *fstr,vstr[128];
				int idx,UsrPort=-1;
				_CSPADDREC CspAddRec;
				BOOL isSpread=FALSE;
				char pxfmt[16]={"%.4f"};

				memset(&Mdr,0,sizeof(Mdr));
				GetDlgItemText(hDlg,IDC_SYMBOL,cSymbol,SO_CSPSYMBOL+1);
#if defined(SPREAD_TRADING)&&defined(PROX_ALL_IQSERVER)
				if(strchr(cSymbol,'~'))
				{
					extern struct SpreadDef *GetSpread(int PortType, int PortNo, const char *ComplexID);
					char ssym[SYMBOL_LEN]={0};
					int UsrPort=-1;

					sscanf(cSymbol,"%d,%s",&UsrPort,ssym);
					if(!MakeSpreadQuoteEx(WS_USR,UsrPort,ssym,&Mdr))
						isSpread=TRUE;
				}
#endif
				sptr=strchr(cSymbol,',');
				if(sptr)
				{
					*sptr=0;
					cExchange=atoi(++sptr);
					sptr=strchr(sptr,',');
					if(sptr)
						cRegion=*(++sptr);
				}
				ListView_DeleteAllItems(lwnd);

				if(!isSpread)
				{
					lMdr=pdlg->smc->_FindCspMdrRec(cSymbol,cExchange,cRegion);
					if(lMdr<4)
						break;
					_ApplyFastLvl1=TRUE;
					pdlg->smc->ReadCspMdrRec(&Mdr,lMdr);
					_ApplyFastLvl1=FALSE;
					if(Mdr.cFormat>4)
						sprintf(pxfmt,"%%.%df",Mdr.cFormat);
				}
				memset(&CspAddRec,0,sizeof(_CSPADDREC));
				//if(Mdr.MdrAddRec)
				//	GetCspAddRec(Mdr.MdrAddRec,&CspAddRec);
				pdlg->smc->GetCspAddRec(0,&CspAddRec);

				for(idx=-12;idx<=123;idx++)
				{
					int hr,min,sec,yr,mon,day;

					switch(idx)
					{
					// _CSPMDR
					case -12: fstr="pxyPortType"; sprintf(vstr,"%d",Mdr.pxyPortType); break;
					case -11: fstr="pxyPortNo"; sprintf(vstr,"%d",Mdr.pxyPortNo); break;
					case -10: fstr="reqlearn"; sprintf(vstr,"%d",Mdr.reqlearn); break;
					case -9: fstr="reqlvl1"; sprintf(vstr,"%d",Mdr.reqlvl1); break;
					case -8: fstr="reqlvl2"; sprintf(vstr,"%d",Mdr.reqlvl2); break;
					case -7: fstr="recvdlearn"; sprintf(vstr,"%d",Mdr.recvdlearn); break;
					case -6: fstr="recvdlvl1"; sprintf(vstr,"%d",Mdr.recvdlvl1); break;
					case -5: fstr="recvdlvl2"; sprintf(vstr,"%d",Mdr.recvdlvl2); break;
					case -4: fstr="dereglvl1"; sprintf(vstr,"%d",Mdr.dereglvl1); break;
					case -3: fstr="dereglvl2"; sprintf(vstr,"%d",Mdr.dereglvl2); break;
					case -2: fstr="reflvl1"; sprintf(vstr,"%d",Mdr.reflvl1); break;
					case -1: fstr="reflvl2"; sprintf(vstr,"%d",Mdr.reflvl2); break;
					case 0: fstr="cFullName"; strcpy(vstr,CspAddRec.cFullName); break;
					//case 0: fstr="VdbRec.lId"; sprintf(vstr,"%d",Mdr.VdbRec.lId); break;
					//case 1: fstr="VdbRec.lPrev"; sprintf(vstr,"%d",Mdr.VdbRec.lPrev); break;
					//case 2: fstr="VdbRec.lNext"; sprintf(vstr,"%d",Mdr.VdbRec.lNext); break;
					//case 3: fstr="VdbRec.uRec"; sprintf(vstr,"%d",Mdr.VdbRec.uRec); break;
					//case 4: fstr="VdbRec.cbRec"; sprintf(vstr,"%d",Mdr.VdbRec.cbRec); break;
					case 5: fstr="lSymbolId"; sprintf(vstr,"%d",Mdr.lSymbolId); break;
					case 6: fstr="cExchange"; sprintf(vstr,"%d",Mdr.cExchange); break;
					case 7: fstr="cRegion"; sprintf(vstr,"%c",Mdr.cRegion); break;
					case 8: fstr="cSymbol"; sprintf(vstr,"%s",Mdr.cSymbol); break;
					case 9: fstr="cMDXQuoteCon"; sprintf(vstr,"%c",Mdr.cMDXQuoteCon); break;
					case 10: fstr="cAlert"; sprintf(vstr,"%c",Mdr.cAlert); break;
					case 11: fstr="cUpdate"; sprintf(vstr,"%d",Mdr.cUpdate); break;
					case 12: fstr="cNull"; sprintf(vstr,"%c",Mdr.cNull); break;
					//case 13: fstr="cQuoteCon"; sprintf(vstr,"%d",Mdr.cQuoteCon); break;
					case 14: fstr="cFormat"; sprintf(vstr,"%d",Mdr.cFormat); break;

					case 15: fstr="lDate"; 
						//yr=((Mdr.lDate&0xFF000000)>>24)*100+((Mdr.lDate&0x00FF0000)>>16);
						//mon=(Mdr.lDate&0x0000FF00)>>8;
						//day=(Mdr.lDate&0x000000FF);
						yr=Mdr.lDate/10000;
						mon=(Mdr.lDate%10000)/100;
						day=Mdr.lDate%100;
						sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
						break;
					case 16: fstr="lTime"; 
						//hr=(Mdr.lTime&0xFF000000)>>24;
						//min=(Mdr.lTime&0x00FF0000)>>16;
						//sec=(Mdr.lTime&0x0000FF00)>>8;
						hr=Mdr.lTime/10000;
						min=(Mdr.lTime%10000)/100;
						sec=Mdr.lTime%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						break;
					case 17: fstr="fOpen"; sprintf(vstr,pxfmt,Mdr.fOpen); break;
					case 18: fstr="fHigh"; sprintf(vstr,pxfmt,Mdr.fHigh); break;
					case 19: fstr="fLow"; sprintf(vstr,pxfmt,Mdr.fLow); break;
					case 20: fstr="fLast"; sprintf(vstr,pxfmt,Mdr.fLast); break;
					case 21: fstr="lVol"; sprintf(vstr,"%d",Mdr.lVol); break;
					case 22: fstr="fOi"; sprintf(vstr,"%d",Mdr.lOi); break;

					case 23: fstr="fBid"; sprintf(vstr,pxfmt,Mdr.fBid); break;
					case 24: fstr="fAsk"; sprintf(vstr,pxfmt,Mdr.fAsk); break;
					case 25: fstr="lBidSize"; sprintf(vstr,"%d",Mdr.lBidSize); break;
					case 26: fstr="lAskSize"; sprintf(vstr,"%d",Mdr.lAskSize); break;
					case 27: fstr="lLastSize"; sprintf(vstr,"%d",Mdr.lLastSize); break;
					case 28: fstr="cSalesCondition"; sprintf(vstr,"%c",Mdr.cSalesCondition); break;
					case 29: fstr="cLastRegion"; sprintf(vstr,"%c",Mdr.cLastRegion); break;
					case 30: fstr="cBidRegion"; sprintf(vstr,"%c",Mdr.cBidRegion); break;
					case 31: fstr="cAskRegion"; sprintf(vstr,"%c",Mdr.cAskRegion); break;
					case 32: fstr="lPrevVol"; sprintf(vstr,"%d",Mdr.lPrevVol); break;
					case 33: fstr="fPrevClose"; sprintf(vstr,pxfmt,Mdr.fPrevClose); break;

					case 34: fstr="fClose"; sprintf(vstr,pxfmt,Mdr.fClose); break;
					case 35: fstr="TickLastVol"; sprintf(vstr,"%d",Mdr.TickLastVol); break;
					case 36: fstr="fNetChange"; sprintf(vstr,pxfmt,Mdr.fNetChange); break;
					case 37: fstr="prevSettlePrice"; sprintf(vstr,pxfmt,Mdr.fSettlePrice); break;
					case 38: fstr="lRegionBidSize"; sprintf(vstr,"%d",Mdr.lRegionBidSize); break;
					case 39: fstr="lRegionAskSize"; sprintf(vstr,"%d",Mdr.lRegionAskSize); break;
					case 40: fstr="vWap1"; sprintf(vstr,pxfmt,Mdr.vWap1); break;
					case 41: fstr="vWap2"; sprintf(vstr,pxfmt,Mdr.vWap2); break;

					case 42: fstr="BarRec"; sprintf(vstr,"%d",Mdr.BarRec); break;
					case 43: fstr="nBarRec"; sprintf(vstr,"%d",Mdr.nBarRec); break;
					case 44: fstr="Expressbid"; sprintf(vstr,"%c",Mdr.Expressbid); break;
					case 45: fstr="Expressask"; sprintf(vstr,"%c",Mdr.Expressask); break;
					case 46: fstr="OrderInbalance"; sprintf(vstr,"%c",Mdr.OrderInbalance); break;
					case 47: fstr="IIndication"; sprintf(vstr,"%c",Mdr.IIndication); break;
					case 48: fstr="nbboTime"; 
						sprintf(vstr,"%d",Mdr.nbboTime); 
						sprintf(vstr,"%02d:%02d:%02d",Mdr.nbboTime/10000,(Mdr.nbboTime%10000)/100,Mdr.nbboTime%100); 
						break;
					//case 47: fstr="DlyRec"; sprintf(vstr,"%d",Mdr.DlyRec); break;
					//case 48: fstr="Lvl2Rec"; sprintf(vstr,"%d",Mdr.Lvl2Rec); break;
					//case 49: fstr="Lvl1Rec"; sprintf(vstr,"%d",Mdr.Lvl1Rec); break;
					//case 50: fstr="MdrWork"; sprintf(vstr,"%d",Mdr.MdrWork); break;
					//case 51: fstr="MdrAddRec"; sprintf(vstr,"%d",Mdr.MdrAddRec); break;
					case 51: fstr="llotSize"; sprintf(vstr,"%d",Mdr.llotSize); break;
					case 52: fstr="lExpDate"; sprintf(vstr,"%d",Mdr.lExpDate); break;
					case 53: fstr="fStrike"; sprintf(vstr,pxfmt,Mdr.fStrike); break;
					//case 54: fstr="PHSTick"; sprintf(vstr,"%d",Mdr.PHSTick); break;
					//case 55: fstr="ISETick"; sprintf(vstr,"%d",Mdr.ISETick); break;
					case 56: fstr="cType"; sprintf(vstr,"%d",Mdr.cType); break;
					case 57: fstr="cUPC"; sprintf(vstr,"%d",Mdr.cUPC); break;
					case 58: fstr="cTestSymbol"; sprintf(vstr,"%d",Mdr.cTestSymbol); break;
					case 59: fstr="cFlag4"; sprintf(vstr,"%d",Mdr.cFlag4); break;

					case 60: fstr="ASEVol"; sprintf(vstr,"%ld",CspAddRec.ASEVol); break;
					case 61: fstr="BSEVol"; sprintf(vstr,"%ld",CspAddRec.BSEVol); break;
					case 62: fstr="CBOVol"; sprintf(vstr,"%ld",CspAddRec.CBOVol); break;
					case 63: fstr="PSEVol"; sprintf(vstr,"%ld",CspAddRec.PSEVol); break;
					case 64: fstr="PHSVol"; sprintf(vstr,"%ld",CspAddRec.PHSVol); break;
					case 65: fstr="ISEVol"; sprintf(vstr,"%ld",CspAddRec.ISEVol); break;
					case 66: fstr="NDQVol"; sprintf(vstr,"%ld",CspAddRec.NDQVol); break;
					case 67: fstr="BATVol"; sprintf(vstr,"%ld",CspAddRec.BATVol); break;
					case 68: fstr="C2Vol"; sprintf(vstr,"%ld",CspAddRec.C2Vol); break;
					case 69: fstr="NBXVol"; sprintf(vstr,"%ld",CspAddRec.NBXVol); break;
					case 70: fstr="MIAVol"; sprintf(vstr,"%ld",CspAddRec.MIAVol); break;
					case 71: fstr="ASETick"; sprintf(vstr,"%ld",CspAddRec.ASETick); break;
					case 72: fstr="BSETick"; sprintf(vstr,"%ld",CspAddRec.BSETick); break;
					case 73: fstr="CBOTick"; sprintf(vstr,"%ld",CspAddRec.CBOTick); break;
					case 74: fstr="PSETick"; sprintf(vstr,"%ld",CspAddRec.PSETick); break;
					case 75: fstr="ISETick"; sprintf(vstr,"%ld",CspAddRec.ISETick); break;
					case 76: fstr="NDQTick"; sprintf(vstr,"%ld",CspAddRec.NDQTick); break;
					case 77: fstr="BATTick"; sprintf(vstr,"%ld",CspAddRec.BATTick); break;
					case 78: fstr="C2Tick"; sprintf(vstr,"%ld",CspAddRec.C2Tick); break;
					case 79: fstr="NBXTick"; sprintf(vstr,"%ld",CspAddRec.NBXTick); break;
					case 80: fstr="MIATick"; sprintf(vstr,"%ld",CspAddRec.MIATick); break;

					case 81: fstr="lBar1Date"; 
						//yr=((Mdr.lBar1Date&0xFF000000)>>24)*100+((Mdr.lBar1Date&0x00FF0000)>>16);
						//mon=(Mdr.lBar1Date&0x0000FF00)>>8;
						//day=(Mdr.lBar1Date&0x000000FF);
						yr=Mdr.lBar1Date/10000;
						mon=(Mdr.lBar1Date%10000)/100;
						day=Mdr.lBar1Date%100;
						sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
						break;
					case 82: fstr="lBar1Time"; 
						//hr=(Mdr.lBar1Time&0xFF000000)>>24;
						//min=(Mdr.lBar1Time&0x00FF0000)>>16;
						//sec=(Mdr.lBar1Time&0x0000FF00)>>8;
						hr=Mdr.lBar1Time/10000;
						min=(Mdr.lBar1Time%10000)/100;
						sec=Mdr.lBar1Time%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						break;
					case 83: fstr="fBar1Open"; sprintf(vstr,pxfmt,Mdr.fBar1Open); break;
					case 84: fstr="fBar1High"; sprintf(vstr,pxfmt,Mdr.fBar1High); break;
					case 85: fstr="fBar1Low"; sprintf(vstr,pxfmt,Mdr.fBar1Low); break;
					case 86: fstr="fBar1Last"; sprintf(vstr,pxfmt,Mdr.fBar1Last); break;
					case 87: fstr="lBar1Vol"; sprintf(vstr,"%d",Mdr.lBar1Vol); break;
					case 88: fstr="lBar1Oi"; sprintf(vstr,"%d",Mdr.lBar1Oi); break;
					case 89: fstr="fFormTVolume"; sprintf(vstr,pxfmt,Mdr.fFormTVolume); break;
					case 90: fstr="fLastRegular"; sprintf(vstr,pxfmt,Mdr.fLastRegular); break;
					case 91: fstr="TickLastPrice"; sprintf(vstr,pxfmt,Mdr.TickLastPrice); break;

					case 92: fstr="cReset"; sprintf(vstr,"%c",Mdr.cReset); break;
					case 93: fstr="Tick"; sprintf(vstr,"%d",Mdr.Tick); break;
					case 94: fstr="LastTickDay"; sprintf(vstr,"%d",Mdr.LastTickDay); break;
					case 95: fstr="Trin"; sprintf(vstr,"%d",Mdr.Trin); break;
					// _CSPADDREC
					//case 88: fstr="VdbRec.lId"; sprintf(vstr,"%d",CspAddRec.VdbRec.lId); break;
					//case 89: fstr="VdbRec.lPrev"; sprintf(vstr,"%d",CspAddRec.VdbRec.lPrev); break;
					//case 90: fstr="VdbRec.lNext"; sprintf(vstr,"%d",CspAddRec.VdbRec.lNext); break;
					//case 91: fstr="VdbRec.uRec"; sprintf(vstr,"%d",CspAddRec.VdbRec.uRec); break;
					//case 92: fstr="VdbRec.cbRec"; sprintf(vstr,"%d",CspAddRec.VdbRec.cbRec); break;
					case 96: fstr="lMdr"; sprintf(vstr,"%d",CspAddRec.lMdr); break;
					case 97: fstr="cExchange"; sprintf(vstr,"%d",CspAddRec.cExchange); break;
					case 98: fstr="cRegion"; sprintf(vstr,"%c",CspAddRec.cRegion); break;
					case 99: fstr="cSymbol"; sprintf(vstr,"%s",CspAddRec.cSymbol); break;
					case 100: fstr="fSimClose"; sprintf(vstr,pxfmt,CspAddRec.fSimClose); break;
					case 101: fstr="fSimPrevClose"; sprintf(vstr,pxfmt,CspAddRec.fSimPrevClose); break;
					case 102: fstr="fSettlePrice"; sprintf(vstr,pxfmt,CspAddRec.fSettlePrice); break;
					case 103: fstr="PrevCloseDate"; 
						//yr=((CspAddRec.PrevCloseDate&0xFF000000)>>24)*100+((CspAddRec.PrevCloseDate&0x00FF0000)>>16);
						//mon=(CspAddRec.PrevCloseDate&0x0000FF00)>>8;
						//day=(CspAddRec.PrevCloseDate&0x000000FF);
						yr=CspAddRec.PrevCloseDate/10000;
						mon=(CspAddRec.PrevCloseDate%10000)/100;
						day=CspAddRec.PrevCloseDate%100;
						sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
						break;
					case 104: fstr="PrevCloseTime"; 
						//hr=(CspAddRec.PrevCloseTime&0xFF000000)>>24;
						//min=(CspAddRec.PrevCloseTime&0x00FF0000)>>16;
						//sec=(CspAddRec.PrevCloseTime&0x0000FF00)>>8;
						hr=CspAddRec.PrevCloseTime/10000;
						min=(CspAddRec.PrevCloseTime%10000)/100;
						sec=CspAddRec.PrevCloseTime%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						break;
					case 105: fstr="LastTradeDate"; 
						//yr=((CspAddRec.LastTradeDate&0xFF000000)>>24)*100+((CspAddRec.LastTradeDate&0x00FF0000)>>16);
						//mon=(CspAddRec.LastTradeDate&0x0000FF00)>>8;
						//day=(CspAddRec.LastTradeDate&0x000000FF);
						yr=CspAddRec.LastTradeDate/10000;
						mon=(CspAddRec.LastTradeDate%10000)/100;
						day=CspAddRec.LastTradeDate%100;
						sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
						break;
					case 106: fstr="LastTradeTime"; 
						//hr=(CspAddRec.LastTradeTime&0xFF000000)>>24;
						//min=(CspAddRec.LastTradeTime&0x00FF0000)>>16;
						//sec=(CspAddRec.LastTradeTime&0x0000FF00)>>8;
						hr=CspAddRec.LastTradeTime/10000;
						min=(CspAddRec.LastTradeTime%10000)/100;
						sec=CspAddRec.LastTradeTime%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						break;
					case 107: fstr="Currency"; 
						switch(CspAddRec.Currency)
						{
						case CUR_USD: strcpy(vstr,"USD"); break;
						case CUR_CAD: strcpy(vstr,"CAD"); break;
						case CUR_UNKNOWN: 
						default: strcpy(vstr,"Unknown"); break;
						}
						break;
					case 108: fstr="ListingEx"; sprintf(vstr, "%c", CspAddRec.ListingEx); break;
					case 109: fstr="lVwapVol"; sprintf(vstr, "%d", CspAddRec.lVwapVol); break;
					case 110: fstr="fVwapSum"; sprintf(vstr, "%.6f", CspAddRec.fVwapSum); break;
					case 111: fstr="fVwapAvg"; sprintf(vstr, "%.4f", CspAddRec.fVwapSum/(CspAddRec.lVwapVol?CspAddRec.lVwapVol:1)); break;
					case 112: fstr="fImbalancePrice"; sprintf(vstr,pxfmt,CspAddRec.fImbalancePrice); break;
					case 113: fstr="lImbalanceSize"; sprintf(vstr,"%d",CspAddRec.lImbalanceSize); break;
					case 114: fstr="lPairedSize"; sprintf(vstr,"%d",CspAddRec.lPairedSize); break;
					case 115: fstr="iImbalanceTime"; //sprintf(vstr,"%06d",CspAddRec.iImbalanceTime); break;
						hr=CspAddRec.iImbalanceTime/10000;
						min=(CspAddRec.iImbalanceTime%10000)/100;
						sec=CspAddRec.iImbalanceTime%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						break;
					case 116: fstr="cImbalanceSide"; sprintf(vstr,"%d",CspAddRec.cImbalanceSide); break;
					case 117: fstr="cImbalanceStatus"; sprintf(vstr,"%d",CspAddRec.cImbalanceStatus); break;
					case 118: fstr="fPinkBid"; sprintf(vstr,pxfmt,CspAddRec.fPinkBid); break;
					case 119: fstr="fPinkAsk"; sprintf(vstr,pxfmt,CspAddRec.fPinkAsk); break;
					case 120: fstr="lPinkBidSize"; sprintf(vstr,"%d",CspAddRec.lPinkBidSize); break;
					case 121: fstr="lPinkAskSize"; sprintf(vstr,"%d",CspAddRec.lPinkAskSize); break;
					case 122: fstr="lCompBidSize"; sprintf(vstr,"%d",CspAddRec.lCompBidSize); break;
					case 123: fstr="lCompAskSize"; sprintf(vstr,"%d",CspAddRec.lCompAskSize); break;
					default: fstr=0;
					}
					if(fstr)
					{
						memset(&litem,0,sizeof(LVITEM));
						litem.iItem=ListView_GetItemCount(lwnd);
						int iidx=ListView_InsertItem(lwnd,&litem);
						if(iidx>=0)
						{
							ListView_SetItemText(lwnd,iidx,0,fstr);
							ListView_SetItemText(lwnd,iidx,1,vstr);
						}
					}
					}
                return TRUE;
            }
        }
		}
		break;
	}
	case WM_SIZE:
	{
		if(lwnd)
		{
			int lx=LOWORD(lParam)-22;
			int ly=HIWORD(lParam)-lrect.top-11;
			MoveWindow(lwnd,lrect.left,lrect.top,lx,ly,TRUE);
		}
		break;
	}
	}
    return FALSE;
}
static int CALLBACK SortLevel2(LPARAM e1, LPARAM e2, LPARAM hint)
{
	_CSPLVLXREC *lpRec1 = (_CSPLVLXREC *)e1;
	_CSPLVLXREC *lpRec2 = (_CSPLVLXREC *)e2;
	if ( hint < 0 )
		return SortLevel2(e2, e1, -hint);
	switch ( hint -1 )
	{
	case 0: 
	case 1: 
		if(lpRec1->lDate!=lpRec2->lDate)
			return lpRec1->lDate -lpRec2->lDate;
		return (lpRec1->lTime&0xFFFFFF00) -(lpRec2->lTime&0xFFFFFF00);
	case 2: return lpRec1->cExchange -lpRec2->cExchange;
	case 3: return lpRec1->cRegion -lpRec2->cRegion;
	case 4: 
		if(lpRec1->cExchange==18)
		{
			int cmp=memcmp(lpRec1->cMMID,lpRec2->cMMID,4);
			float fval1=lpRec1->fBid!=0.0 ?lpRec1->fBid :lpRec1->fAsk;
			float fval2=lpRec2->fBid!=0.0 ?lpRec2->fBid :lpRec2->fAsk;
			if(cmp)
				return cmp;
			if(fval1<fval2)
				return -1;
			else if(fval1>fval2)
				return +1;
			else
				return 0;
		}
		else
			return memcmp(lpRec1->cMMID,lpRec2->cMMID,4);
	case 5: return lpRec1->cSalesCondition -lpRec2->cSalesCondition;
	case 6: 
		if(lpRec1->cExchange==18)
		{
			float fval1=lpRec1->fBid!=0.0 ?lpRec1->fBid :lpRec1->fAsk;
			float fval2=lpRec2->fBid!=0.0 ?lpRec2->fBid :lpRec2->fAsk;
			long lval1=lpRec1->fBid!=0.0 ?lpRec1->lBidSize :lpRec1->lAskSize;
			long lval2=lpRec2->fBid!=0.0 ?lpRec2->lBidSize :lpRec2->lAskSize;
			if(fval1<fval2)
				return -1;
			else if(fval1>fval2)
				return +1;
			else
				return lval1 -lval2;
		}
		else
		{
			if(lpRec1->fBid<lpRec2->fBid)
				return -1;
			else if(lpRec1->fBid>lpRec2->fBid)
				return +1;
			else if(lpRec1->lBidSize==lpRec2->lBidSize)
			{
				char mmid1[8],mmid2[8];
				if(lpRec1->cMMID[0]) strcpy(mmid1,lpRec1->cMMID);
				else sprintf(mmid1,"%c",lpRec1->cRegion);
				if(lpRec2->cMMID[0]) strcpy(mmid2,lpRec2->cMMID);
				else sprintf(mmid2,"%c",lpRec2->cRegion);
				return -_stricmp(mmid1,mmid2);
			}
			else
				return lpRec1->lBidSize -lpRec2->lBidSize;
		}
		return 0;
	case 7:
		if(lpRec1->cExchange==18)
		{
			float fval1=lpRec1->fAsk!=0.0 ?lpRec1->fAsk :lpRec1->fBid;
			float fval2=lpRec2->fAsk!=0.0 ?lpRec2->fAsk :lpRec2->fBid;
			long lval1=lpRec1->fAsk!=0.0 ?lpRec1->lAskSize :lpRec1->lBidSize;
			long lval2=lpRec2->fAsk!=0.0 ?lpRec2->lAskSize :lpRec2->lBidSize;
			if(fval1<fval2)
				return -1;
			else if(fval1>fval2)
				return +1;
			else
				return lval1 -lval2;
		}
		else
		{
			if(lpRec1->fAsk<lpRec2->fAsk)
				return -1;
			else if(lpRec1->fAsk>lpRec2->fAsk)
				return +1;
			else if(lpRec2->lAskSize==lpRec1->lAskSize)
			{
				char mmid1[8],mmid2[8];
				if(lpRec1->cMMID[0]) strcpy(mmid1,lpRec1->cMMID);
				else sprintf(mmid1,"%c",lpRec1->cRegion);
				if(lpRec2->cMMID[0]) strcpy(mmid2,lpRec2->cMMID);
				else sprintf(mmid2,"%c",lpRec2->cRegion);
				return _stricmp(mmid1,mmid2);
			}
			else
				return lpRec2->lAskSize -lpRec1->lAskSize;
		}
		return 0;
	case 8: return lpRec1->lBidSize -lpRec2->lBidSize;
	case 9: return lpRec1->lAskSize -lpRec2->lAskSize;
	}
	return 0;
}
LRESULT CALLBACK cbLevel2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT lrect;
	static int sortcol=-1;
	static BOOL showdel = FALSE;
	static BOOL applyFast = FALSE;
	HWND lwnd=GetDlgItem(hDlg,IDC_LIST);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SMConsoleDlg *pdlg=(SMConsoleDlg *)lParam;
		SetWindowLong(hDlg,GWL_USERDATA,(DWORD)(PTRCAST)pdlg);
		LVCOLUMN lcols[12]={
			{LVCF_WIDTH|LVCF_TEXT,0,80,"lDate",0,0,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,80,"lTime",0,1,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cExchange",0,2,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cRegion",0,3,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,60,"cMMID",0,4,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cSalesCondition",0,5,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,80,"fBid",0,6,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,80,"fAsk",0,7,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,60,"lBidSize",0,8,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,60,"lAskSize",0,9,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,200,"csBid",0,10,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,200,"csAsk",0,11,0,0},
		};
		int i;
		POINT p1,p2;

		ListView_SetExtendedListViewStyle(lwnd, LVS_EX_FULLROWSELECT);
		for (i=0;i<12;i++)
			ListView_InsertColumn(lwnd,i,&lcols[i]);

		GetWindowRect(lwnd,&lrect);
		p1.x=lrect.left; p1.y=lrect.top; p2.x=lrect.right; p2.y=lrect.bottom;
		ScreenToClient(hDlg,&p1); ScreenToClient(hDlg,&p2);
		lrect.left=p1.x; lrect.top=p1.y; lrect.right=p2.x; lrect.bottom=p2.y;
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
		{
			SMConsoleDlg *pdlg=(SMConsoleDlg *)(PTRCAST)GetWindowLong(hDlg,GWL_USERDATA);
			pdlg->smc->FindCloseCspLvlXRec();
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
        case IDC_SYMBOL:
        {
            if (HIWORD(wParam)==EN_CHANGE)
            {
				SMConsoleDlg *pdlg=(SMConsoleDlg *)(PTRCAST)GetWindowLong(hDlg,GWL_USERDATA);
				_CSPMDR Mdr;
				long lMdr;
				char cSymbol[SO_CSPSYMBOL+1]={0},cExchange=0,cRegion=0,*sptr;
				LVITEM litem;
				char vstr[128];
				int idx;
				_CSPLVLXREC2 *lpRec=0;
				char pxfmt[16]={"%.4f"};

				memset(&Mdr,0,sizeof(Mdr));
				GetDlgItemText(hDlg,IDC_SYMBOL,cSymbol,SO_CSPSYMBOL+1);
				sptr=strchr(cSymbol,',');
				if(sptr)
				{
					*sptr=0;
					cExchange=atoi(++sptr);
					sptr=strchr(sptr,',');
					if(sptr)
						cRegion=*(++sptr);
				}
				pdlg->smc->FindCloseCspLvlXRec();
				ListView_DeleteAllItems(lwnd);

				lMdr=pdlg->smc->_FindCspMdrRec(cSymbol,cExchange,cRegion);
				if(lMdr<4)
					break;
				_ApplyFastLvl1=applyFast;
				pdlg->smc->ReadCspMdrRec(&Mdr,lMdr);
				_ApplyFastLvl1=FALSE;
				if(Mdr.cFormat>4)
					sprintf(pxfmt,"%%.%df",Mdr.cFormat);
				for(idx=pdlg->smc->FindFirstCspLvlXRec2(&Mdr,&lpRec,applyFast,showdel);idx!=0;idx=pdlg->smc->FindNextCspLvlXRec2(&lpRec,idx))
				{
					int hr,min,sec,yr,mon,day,nidx;
					char mmid[5];
					if(!showdel)
					{
						if((!lpRec->lBidSize)&&(!lpRec->lAskSize))
						{
							continue;
						}
					}

					memset(&litem,0,sizeof(LVITEM));
					litem.iItem=idx;
					litem.mask=LVIF_PARAM;
					litem.lParam=(LPARAM)lpRec;
					nidx=ListView_InsertItem(lwnd,&litem);

					//yr=((lpRec->lDate&0xFF000000)>>24)*100+((lpRec->lDate&0x00FF0000)>>16);
					//mon=(lpRec->lDate&0x0000FF00)>>8;
					//day=(lpRec->lDate&0x000000FF);
					yr=lpRec->lDate/10000;
					mon=(lpRec->lDate%10000)/100;
					day=lpRec->lDate%100;
					sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
					ListView_SetItemText(lwnd,nidx,0,vstr);

					//hr=(lpRec->lTime&0xFF000000)>>24;
					//min=(lpRec->lTime&0x00FF0000)>>16;
					//sec=(lpRec->lTime&0x0000FF00)>>8;
					hr=lpRec->lTime/10000;
					min=(lpRec->lTime%10000)/100;
					sec=lpRec->lTime%100;
					sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
					ListView_SetItemText(lwnd,nidx,1,vstr);

					sprintf(vstr,"%d",lpRec->cExchange);
					ListView_SetItemText(lwnd,nidx,2,vstr);
					sprintf(vstr,"%c",lpRec->cRegion);
					ListView_SetItemText(lwnd,nidx,3,vstr);
					memcpy(mmid,lpRec->cMMID,4); mmid[4]=0;
					//if(lpRec->cExchange==0x12)
					//{
					//	char *mptr;
					//	int mval;

					//	for(mptr=mmid;*mptr;mptr++)
					//		*mptr-=49;//17;
					//	mval=atoi(mmid);
					//	sprintf(mmid,"%d",mval);
					//}

					sprintf(vstr,"%s",mmid);
					ListView_SetItemText(lwnd,nidx,4,vstr);
					sprintf(vstr,"%d",lpRec->cSalesCondition);
					ListView_SetItemText(lwnd,nidx,5,vstr);
					sprintf(vstr,pxfmt,lpRec->fBid);
					ListView_SetItemText(lwnd,nidx,6,vstr);
					sprintf(vstr,pxfmt,lpRec->fAsk);
					ListView_SetItemText(lwnd,nidx,7,vstr);
					sprintf(vstr,"%d",lpRec->lBidSize);
					ListView_SetItemText(lwnd,nidx,8,vstr);
					sprintf(vstr,"%d",lpRec->lAskSize);
					ListView_SetItemText(lwnd,nidx,9,vstr);
					if(lpRec->csBid[0])
						sprintf(vstr,"[%d]%s",lpRec->csBid[0],lpRec->csBid+1);
					else
						vstr[0]=0;
					ListView_SetItemText(lwnd,nidx,10,vstr);
					if(lpRec->csAsk[0])
						sprintf(vstr,"[%d]%s",lpRec->csAsk[0],lpRec->csAsk+1);
					else
						vstr[0]=0;
					ListView_SetItemText(lwnd,nidx,11,vstr);
				}
				//pdlg->smc->FindCloseCspLvlXRec(idx);
				ListView_SortItems(lwnd, SortLevel2, sortcol);
                return TRUE;
            }
        }
		case IDC_SHOW_DEL:
		{
			BOOL nshowdel=(IsDlgButtonChecked(hDlg,IDC_SHOW_DEL)==BST_CHECKED)?TRUE :FALSE;
			if(nshowdel!=showdel)
			{
				showdel=nshowdel;
				cbLevel2(hDlg,WM_COMMAND,MAKELONG(IDC_SYMBOL,EN_CHANGE),0);
			}
			return TRUE;
		}
		case IDC_APPLY_FASTLVL2:
		{
			BOOL napplyFast=(IsDlgButtonChecked(hDlg,IDC_APPLY_FASTLVL2)==BST_CHECKED)?TRUE :FALSE;
			if(napplyFast!=applyFast)
			{
				applyFast=napplyFast;
				cbLevel2(hDlg,WM_COMMAND,MAKELONG(IDC_SYMBOL,EN_CHANGE),0);
			}
			return TRUE;
		}
		}
		break;
	}
	case WM_NOTIFY:
		if ( wParam == IDC_LIST )
		{
			NMHDR *lpNmHdr = (NMHDR *)lParam;
			if ( lpNmHdr->code == LVN_COLUMNCLICK )
			{
				LPNMLISTVIEW pNmListView = (LPNMLISTVIEW)lParam;
				int ncol = pNmListView->iSubItem +1;
				if ( ncol == sortcol )
					sortcol = -sortcol;
				else
					sortcol = ncol;
				ListView_SortItems(lwnd, SortLevel2, sortcol);
			}
			return TRUE;
		}
		return FALSE;
	case WM_SIZE:
	{
		if(lwnd)
		{
			int lx=LOWORD(lParam)-22;
			int ly=HIWORD(lParam)-lrect.top-11;
			MoveWindow(lwnd,lrect.left,lrect.top,lx,ly,TRUE);
		}
		break;
	}
	}
    return FALSE;
}
LRESULT CALLBACK cbMontage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT blrect,alrect;
	static int bsortcol=-7,asortcol=8;
	static BOOL showdel = FALSE;
	HWND blwnd=GetDlgItem(hDlg,IDC_BIDLIST);
	HWND alwnd=GetDlgItem(hDlg,IDC_ASKLIST);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SMConsoleDlg *pdlg=(SMConsoleDlg *)lParam;
		SetWindowLong(hDlg,GWL_USERDATA,(DWORD)(PTRCAST)pdlg);
		LVCOLUMN blcols[8]={
			{LVCF_WIDTH|LVCF_TEXT,0,80,"lDate",0,0,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,80,"lTime",0,1,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cExchange",0,2,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cRegion",0,3,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,60,"cMMID",0,4,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cSalesCondition",0,5,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,80,"fBid",0,6,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,60,"lBidSize",0,8,0,0},
		};
		LVCOLUMN alcols[8]={
			{LVCF_WIDTH|LVCF_TEXT,0,80,"lDate",0,0,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,80,"lTime",0,1,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cExchange",0,2,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cRegion",0,3,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,60,"cMMID",0,4,0,0},
			{LVCF_WIDTH|LVCF_TEXT,0,40,"cSalesCondition",0,5,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,80,"fAsk",0,7,0,0},
			{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,60,"lAskSize",0,9,0,0},
		};
		int i;
		POINT p1,p2;

		ListView_SetExtendedListViewStyle(blwnd, LVS_EX_FULLROWSELECT);
		for (i=0;i<8;i++)
			ListView_InsertColumn(blwnd,i,&blcols[i]);
		ListView_SetExtendedListViewStyle(alwnd, LVS_EX_FULLROWSELECT);
		for (i=0;i<8;i++)
			ListView_InsertColumn(alwnd,i,&alcols[i]);

		GetWindowRect(blwnd,&blrect);
		p1.x=blrect.left; p1.y=blrect.top; p2.x=blrect.right; p2.y=blrect.bottom;
		ScreenToClient(hDlg,&p1); ScreenToClient(hDlg,&p2);
		blrect.left=p1.x; blrect.top=p1.y; blrect.right=p2.x; blrect.bottom=p2.y;

		GetWindowRect(alwnd,&alrect);
		p1.x=alrect.left; p1.y=alrect.top; p2.x=alrect.right; p2.y=alrect.bottom;
		ScreenToClient(hDlg,&p1); ScreenToClient(hDlg,&p2);
		alrect.left=p1.x; alrect.top=p1.y; alrect.right=p2.x; alrect.bottom=p2.y;
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
		{
			SMConsoleDlg *pdlg=(SMConsoleDlg *)(PTRCAST)GetWindowLong(hDlg,GWL_USERDATA);
			pdlg->smc->FindCloseCspLvlXRec();
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
        case IDC_SYMBOL:
        {
            if (HIWORD(wParam)==EN_CHANGE)
            {
				SMConsoleDlg *pdlg=(SMConsoleDlg *)(PTRCAST)GetWindowLong(hDlg,GWL_USERDATA);
				_CSPMDR Mdr;
				long lMdr;
				char cSymbol[SO_CSPSYMBOL]={0},cExchange=0,cRegion=0,*sptr;
				LVITEM litem;
				char vstr[128];
				int idx;
				_CSPLVLXREC2 *lpRec=0;
				char pxfmt[16]={"%.4f"},bbostr[64]={0};

				memset(&Mdr,0,sizeof(Mdr));
				GetDlgItemText(hDlg,IDC_SYMBOL,cSymbol,SO_CSPSYMBOL);
				sptr=strchr(cSymbol,',');
				if(sptr)
				{
					*sptr=0;
					cExchange=atoi(++sptr);
					sptr=strchr(sptr,',');
					if(sptr)
						cRegion=*(++sptr);
				}
				pdlg->smc->FindCloseCspLvlXRec();
				ListView_DeleteAllItems(blwnd);
				ListView_DeleteAllItems(alwnd);

				lMdr=pdlg->smc->_FindCspMdrRec(cSymbol,cExchange,cRegion);
				if(lMdr<4)
					break;
				_ApplyFastLvl1=TRUE;
				pdlg->smc->ReadCspMdrRec(&Mdr,lMdr);
				_ApplyFastLvl1=FALSE;
				if(Mdr.cFormat>4)
					sprintf(pxfmt,"%%.%df",Mdr.cFormat);
				sprintf(bbostr,"BBO: %.2f x%d | %.2f x%d",Mdr.fBid,Mdr.lBidSize,Mdr.fAsk,Mdr.lAskSize);
				SetDlgItemText(hDlg,IDC_BBO,bbostr);
				for(idx=pdlg->smc->FindFirstCspLvlXRec2(&Mdr,&lpRec,TRUE,showdel);idx!=0;idx=pdlg->smc->FindNextCspLvlXRec2(&lpRec,idx))
				{
					int hr,min,sec,yr,mon,day,nidx;
					char mmid[5];
					if((showdel)||(lpRec->lBidSize))
					{
						memset(&litem,0,sizeof(LVITEM));
						litem.iItem=idx;
						litem.mask=LVIF_PARAM;
						litem.lParam=(LPARAM)lpRec;
						nidx=ListView_InsertItem(blwnd,&litem);

						//yr=((lpRec->lDate&0xFF000000)>>24)*100+((lpRec->lDate&0x00FF0000)>>16);
						//mon=(lpRec->lDate&0x0000FF00)>>8;
						//day=(lpRec->lDate&0x000000FF);
						yr=lpRec->lDate/10000;
						mon=(lpRec->lDate%10000)/100;
						day=lpRec->lDate%100;
						sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
						ListView_SetItemText(blwnd,nidx,0,vstr);

						//hr=(lpRec->lTime&0xFF000000)>>24;
						//min=(lpRec->lTime&0x00FF0000)>>16;
						//sec=(lpRec->lTime&0x0000FF00)>>8;
						hr=lpRec->lTime/10000;
						min=(lpRec->lTime%10000)/100;
						sec=lpRec->lTime%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						ListView_SetItemText(blwnd,nidx,1,vstr);

						sprintf(vstr,"%d",lpRec->cExchange);
						ListView_SetItemText(blwnd,nidx,2,vstr);
						sprintf(vstr,"%c",lpRec->cRegion);
						ListView_SetItemText(blwnd,nidx,3,vstr);
						memcpy(mmid,lpRec->cMMID,4); mmid[4]=0;
						//if(lpRec->cExchange==0x12)
						//{
						//	char *mptr;
						//	int mval;

						//	for(mptr=mmid;*mptr;mptr++)
						//		*mptr-=49;//17;
						//	mval=atoi(mmid);
						//	sprintf(mmid,"%d",mval);
						//}

						sprintf(vstr,"%s",mmid);
						ListView_SetItemText(blwnd,nidx,4,vstr);
						sprintf(vstr,"%c",lpRec->cSalesCondition?'A' +lpRec->cSalesCondition -1:0);
						ListView_SetItemText(blwnd,nidx,5,vstr);
						sprintf(vstr,pxfmt,lpRec->fBid);
						ListView_SetItemText(blwnd,nidx,6,vstr);
						sprintf(vstr,"%d",lpRec->lBidSize);
						ListView_SetItemText(blwnd,nidx,7,vstr);
					}

					if((showdel)||(lpRec->lAskSize))
					{
						memset(&litem,0,sizeof(LVITEM));
						litem.iItem=idx;
						litem.mask=LVIF_PARAM;
						litem.lParam=(LPARAM)lpRec;
						nidx=ListView_InsertItem(alwnd,&litem);

						//yr=((lpRec->lDate&0xFF000000)>>24)*100+((lpRec->lDate&0x00FF0000)>>16);
						//mon=(lpRec->lDate&0x0000FF00)>>8;
						//day=(lpRec->lDate&0x000000FF);
						yr=lpRec->lDate/10000;
						mon=(lpRec->lDate%10000)/100;
						day=lpRec->lDate%100;
						sprintf(vstr,"%04d/%02d/%02d",yr,mon,day); 
						ListView_SetItemText(alwnd,nidx,0,vstr);

						//hr=(lpRec->lTime&0xFF000000)>>24;
						//min=(lpRec->lTime&0x00FF0000)>>16;
						//sec=(lpRec->lTime&0x0000FF00)>>8;
						hr=lpRec->lTime/10000;
						min=(lpRec->lTime%10000)/100;
						sec=lpRec->lTime%100;
						sprintf(vstr,"%02d:%02d:%02d",hr,min,sec); 
						ListView_SetItemText(alwnd,nidx,1,vstr);

						sprintf(vstr,"%d",lpRec->cExchange);
						ListView_SetItemText(alwnd,nidx,2,vstr);
						sprintf(vstr,"%c",lpRec->cRegion);
						ListView_SetItemText(alwnd,nidx,3,vstr);
						memcpy(mmid,lpRec->cMMID,4); mmid[4]=0;
						//if(lpRec->cExchange==0x12)
						//{
						//	char *mptr;
						//	int mval;

						//	for(mptr=mmid;*mptr;mptr++)
						//		*mptr-=49;//17;
						//	mval=atoi(mmid);
						//	sprintf(mmid,"%d",mval);
						//}

						sprintf(vstr,"%s",mmid);
						ListView_SetItemText(alwnd,nidx,4,vstr);
						sprintf(vstr,"%c",lpRec->cSalesCondition?'A'+lpRec->cSalesCondition -1:' ');
						ListView_SetItemText(alwnd,nidx,5,vstr);
						sprintf(vstr,pxfmt,lpRec->fAsk);
						ListView_SetItemText(alwnd,nidx,6,vstr);
						sprintf(vstr,"%d",lpRec->lAskSize);
						ListView_SetItemText(alwnd,nidx,7,vstr);
					}
				}
				//pdlg->smc->FindCloseCspLvlXRec(idx);
				ListView_SortItems(blwnd, SortLevel2, bsortcol);
				ListView_SortItems(alwnd, SortLevel2, asortcol);
                return TRUE;
            }
        }
		case IDC_SHOW_DEL:
		{
			BOOL nshowdel=(IsDlgButtonChecked(hDlg,IDC_SHOW_DEL)==BST_CHECKED)?TRUE :FALSE;
			if(nshowdel!=showdel)
			{
				showdel=nshowdel;
				cbMontage(hDlg,WM_COMMAND,MAKELONG(IDC_SYMBOL,EN_CHANGE),0);
			}
			return TRUE;
		}
		}
		break;
	}
	case WM_NOTIFY:
		if ( wParam == IDC_BIDLIST )
		{
			NMHDR *lpNmHdr = (NMHDR *)lParam;
			if ( lpNmHdr->code == LVN_COLUMNCLICK )
			{
				LPNMLISTVIEW pNmListView = (LPNMLISTVIEW)lParam;
				int ncol = pNmListView->iSubItem +1;
				if(ncol==7) ncol=7;
				else if(ncol==8) ncol=9;
				if ( ncol == bsortcol )
					bsortcol = -bsortcol;
				else
					bsortcol = ncol;
				ListView_SortItems(blwnd, SortLevel2, bsortcol);
			}
			return TRUE;
		}
		else if ( wParam == IDC_ASKLIST )
		{
			NMHDR *lpNmHdr = (NMHDR *)lParam;
			if ( lpNmHdr->code == LVN_COLUMNCLICK )
			{
				LPNMLISTVIEW pNmListView = (LPNMLISTVIEW)lParam;
				int ncol = pNmListView->iSubItem +1;
				if(ncol==7) ncol=8;
				else if(ncol==8) ncol=10;
				if ( ncol == asortcol )
					asortcol = -asortcol;
				else
					asortcol = ncol;
				ListView_SortItems(alwnd, SortLevel2, asortcol);
			}
			return TRUE;
		}
		return FALSE;
	case WM_SIZE:
	{
		if(blwnd)
		{
			int lx=(LOWORD(lParam)-22-4)/2;
			int ly=HIWORD(lParam)-blrect.top-11;
			int ax=blrect.left+lx+4;
			MoveWindow(blwnd,blrect.left,blrect.top,lx,ly,TRUE);
			MoveWindow(alwnd,ax,alrect.top,lx,ly,TRUE);
		}
		break;
	}
	}
    return FALSE;
}
int SMConsole::Lvl1Dlg(const char *apath)
{
	lvlxAppPath=apath;
	return (int)DialogBoxParam(theApp.m_hInstance,(LPCTSTR)IDD_LEVEL1,0,(DLGPROC)cbLevel1,(LPARAM)pdlg);
}
int SMConsole::Lvl1Answer(int rc, const char *fpath)
{
	// build an internal lvl1 record from the file
	if(rc)
		lvl1Result=0;
	else
	{
		FILE *fp=fopen(fpath,"rt");
		if(fp)
		{
			lvl1Result=new _CSPMDR;
			memset(lvl1Result,0,sizeof(_CSPMDR));
			lvl1AddResult=new _CSPADDREC;
			memset(lvl1AddResult,0,sizeof(_CSPADDREC));
			char rbuf[1024]={0};
			while(fgets(rbuf,sizeof(rbuf),fp))
			{
				char *delim=strchr(rbuf +16,' ');
				if(!delim) continue;
				*delim=0;
				const char *attr;
				for(attr=rbuf;(attr<delim)&&(*attr==' ');attr++)
					;
				if(!*attr) continue;
				const char *val=delim +1;
				delim=(char*)strendl(val,val +strlen(val));
				if(delim) *delim=0;
				if(!strcmp(attr,"pxyPortType")) lvl1Result->pxyPortType=atoi(val);
				if(!strcmp(attr,"pxyPortNo")) lvl1Result->pxyPortNo=atoi(val);
				if(!strcmp(attr,"reqlearn")) lvl1Result->reqlearn=atoi(val);
				if(!strcmp(attr,"reqlvl1")) lvl1Result->reqlvl1=atoi(val);
				if(!strcmp(attr,"reqlvl2")) lvl1Result->reqlvl2=atoi(val);
				if(!strcmp(attr,"recvdlearn")) lvl1Result->recvdlearn=atoi(val);
				if(!strcmp(attr,"recvdlvl1")) lvl1Result->recvdlvl1=atoi(val);
				if(!strcmp(attr,"recvdlvl2")) lvl1Result->recvdlvl2=atoi(val);
				if(!strcmp(attr,"dereglvl1")) lvl1Result->dereglvl1=atoi(val);
				if(!strcmp(attr,"dereglvl2")) lvl1Result->dereglvl2=atoi(val);
				if(!strcmp(attr,"reflvl1")) lvl1Result->reflvl1=atoi(val);
				if(!strcmp(attr,"reflvl2")) lvl1Result->reflvl2=atoi(val);
				if(!strcmp(attr,"lSymbolId")) lvl1Result->lSymbolId=atol(val);
				if(!strcmp(attr,"cExchange")) lvl1Result->cExchange=atoi(val);
				if(!strcmp(attr,"cRegion")) lvl1Result->cRegion=*val;
				if(!strcmp(attr,"cSymbol")) strncpy(lvl1Result->cSymbol,val,20);
				if(!strcmp(attr,"cMDXQuoteCon")) lvl1Result->cMDXQuoteCon=*val;
				if(!strcmp(attr,"cAlert")) lvl1Result->cAlert=*val;
				if(!strcmp(attr,"cUpdate")) lvl1Result->cUpdate=atoi(val);
				if(!strcmp(attr,"cNull")) lvl1Result->cNull=*val;
				if(!strcmp(attr,"cFullName")) strncpy(lvl1AddResult->cFullName,val,40);
				if(!strcmp(attr,"cFormat")) lvl1Result->cFormat=atoi(val);
				if(!strcmp(attr,"lDate")) lvl1Result->lDate=(atoi(val)*10000 +atoi(val +5)*100 +atoi(val +8));//2010/02/01
				if(!strcmp(attr,"lTime")) lvl1Result->lTime=(atoi(val)*10000 +atoi(val +3)*100 +atoi(val +6));//16:30:49
				if(!strcmp(attr,"fOpen")) lvl1Result->fOpen=(float)atof(val);
				if(!strcmp(attr,"fHigh")) lvl1Result->fHigh=(float)atof(val);
				if(!strcmp(attr,"fLow")) lvl1Result->fLow=(float)atof(val);
				if(!strcmp(attr,"fLast")) lvl1Result->fLast=(float)atof(val);
				if(!strcmp(attr,"lVol")) lvl1Result->lVol=atol(val);
				if(!strcmp(attr,"fOi")) lvl1Result->lOi=atol(val);
				if(!strcmp(attr,"fBid")) lvl1Result->fBid=(float)atof(val);
				if(!strcmp(attr,"fAsk")) lvl1Result->fAsk=(float)atof(val);
				if(!strcmp(attr,"lBidSize")) lvl1Result->lBidSize=atol(val);
				if(!strcmp(attr,"lAskSize")) lvl1Result->lAskSize=atol(val);
				if(!strcmp(attr,"lLastSize")) lvl1Result->lLastSize=atol(val);
				if(!strcmp(attr,"cSalesCondition")) lvl1Result->cSalesCondition=*val;
				if(!strcmp(attr,"cLastRegion")) lvl1Result->cLastRegion=*val;
				if(!strcmp(attr,"cBidRegion")) lvl1Result->cBidRegion=*val;
				if(!strcmp(attr,"cAskRegion")) lvl1Result->cAskRegion=*val;
				if(!strcmp(attr,"lPrevVol")) lvl1Result->lPrevVol=atol(val);
				if(!strcmp(attr,"fPrevClose")) lvl1Result->fPrevClose=(float)atof(val);
				if(!strcmp(attr,"fClose")) lvl1Result->fClose=(float)atof(val);
				if(!strcmp(attr,"lPrevVol")) lvl1Result->lPrevVol=atol(val);
				if(!strcmp(attr,"TickLastVol")) lvl1Result->TickLastVol=atol(val);
				if(!strcmp(attr,"prevSettlePrice")) lvl1Result->fSettlePrice=(float)atof(val);
				if(!strcmp(attr,"lRegionBidSize")) lvl1Result->lRegionBidSize=atol(val);
				if(!strcmp(attr,"lRegionAskSize")) lvl1Result->lRegionAskSize=atol(val);
				if(!strcmp(attr,"vWap1")) lvl1Result->vWap1=(float)atof(val);
				if(!strcmp(attr,"vWap2")) lvl1Result->vWap2=(float)atof(val);
				if(!strcmp(attr,"BarRec")) lvl1Result->BarRec=atol(val);
				if(!strcmp(attr,"nBarRec")) lvl1Result->nBarRec=atoi(val);
				if(!strcmp(attr,"Expressbid")) lvl1Result->Expressbid=*val;
				if(!strcmp(attr,"Expressask")) lvl1Result->Expressask=*val;
				if(!strcmp(attr,"OrderInbalance")) lvl1Result->OrderInbalance=*val;
				if(!strcmp(attr,"IIndication")) lvl1Result->IIndication=*val;
				if(!strcmp(attr,"nbboTime")) lvl1Result->nbboTime=(atoi(val)*10000 +atoi(val +3)*100 +atoi(val +6));// 16:23:51
				if(!strcmp(attr,"lExpDate")) lvl1Result->lExpDate=atoi(val);
				if(!strcmp(attr,"fStrike")) lvl1Result->fStrike=(float)atof(val);
				if(!strcmp(attr,"llotSize")) lvl1Result->llotSize=atoi(val);
				if(!strcmp(attr,"llVol")) lvl1Result->llVol=_atoi64(val);
				if(!strcmp(attr,"llPrevVol")) lvl1Result->llPrevVol=_atoi64(val);
				if(!strcmp(attr,"cType")) lvl1Result->cType=atoi(val);
				if(!strcmp(attr,"cUPC")) lvl1Result->cUPC=atoi(val);
				if(!strcmp(attr,"cTestSymbol")) lvl1Result->cTestSymbol=atoi(val);
				if(!strcmp(attr,"cFlag4")) lvl1Result->cFlag4=atoi(val);
				if(!strcmp(attr,"ASEVol")) lvl1AddResult->ASEVol=atoi(val);
				if(!strcmp(attr,"BSEVol")) lvl1AddResult->BSEVol=atoi(val);
				if(!strcmp(attr,"CBOVol")) lvl1AddResult->CBOVol=atoi(val);
				if(!strcmp(attr,"PSEVol")) lvl1AddResult->PSEVol=atoi(val);
				if(!strcmp(attr,"PHSVol")) lvl1AddResult->PHSVol=atoi(val);
				if(!strcmp(attr,"ISEVol")) lvl1AddResult->ISEVol=atoi(val);
				if(!strcmp(attr,"NDQVol")) lvl1AddResult->NDQVol=atoi(val);
				if(!strcmp(attr,"BATVol")) lvl1AddResult->BATVol=atoi(val);
				if(!strcmp(attr,"C2Vol")) lvl1AddResult->C2Vol=atoi(val);
				if(!strcmp(attr,"NBXVol")) lvl1AddResult->NBXVol=atoi(val);
				if(!strcmp(attr,"MIAVol")) lvl1AddResult->MIAVol=atoi(val);
				if(!strcmp(attr,"ASETick")) lvl1AddResult->ASETick=atoi(val);
				if(!strcmp(attr,"BSETick")) lvl1AddResult->BSETick=atoi(val);
				if(!strcmp(attr,"CBOTick")) lvl1AddResult->CBOTick=atoi(val);
				if(!strcmp(attr,"PSETick")) lvl1AddResult->PSETick=atoi(val);
				if(!strcmp(attr,"ISETick")) lvl1AddResult->ISETick=atoi(val);
				if(!strcmp(attr,"NDQTick")) lvl1AddResult->NDQTick=atoi(val);
				if(!strcmp(attr,"BATTick")) lvl1AddResult->BATTick=atoi(val);
				if(!strcmp(attr,"C2Tick")) lvl1AddResult->C2Tick=atoi(val);
				if(!strcmp(attr,"NBXTick")) lvl1AddResult->NBXTick=atoi(val);
				if(!strcmp(attr,"MIATick")) lvl1AddResult->MIATick=atoi(val);
				if(!strcmp(attr,"lBar1Date")) lvl1Result->lBar1Date=(atoi(val)*10000 +atoi(val +5)*100 +atoi(val +8));// 0000/00/00
				if(!strcmp(attr,"lBar1Time")) lvl1Result->lBar1Time=(atoi(val)*10000 +atoi(val +3)*100 +atoi(val +6));// 00:00:00
				if(!strcmp(attr,"fBar1Open")) lvl1Result->fBar1Open=(float)atof(val);
				if(!strcmp(attr,"fBar1High")) lvl1Result->fBar1High=(float)atof(val);
				if(!strcmp(attr,"fBar1Low")) lvl1Result->fBar1Low=(float)atof(val);
				if(!strcmp(attr,"fBar1Last")) lvl1Result->fBar1Last=(float)atof(val);
				if(!strcmp(attr,"lBar1Vol")) lvl1Result->lBar1Vol=atoi(val);
				if(!strcmp(attr,"lBar1Oi")) lvl1Result->lBar1Oi=atoi(val);
				if(!strcmp(attr,"fFormTVolume")) lvl1Result->fFormTVolume=(float)atol(val);
				if(!strcmp(attr,"fLastRegular")) lvl1Result->fLastRegular=(float)atof(val);
				if(!strcmp(attr,"TickLastPrice")) lvl1Result->TickLastPrice=(float)atof(val);
				if(!strcmp(attr,"cReset")) lvl1Result->cReset=atoi(val);
				if(!strcmp(attr,"Tick")) lvl1Result->Tick=atoi(val);
				if(!strcmp(attr,"LastTickDay")) lvl1Result->LastTickDay=atoi(val);
				if(!strcmp(attr,"Trin")) lvl1Result->Trin=atoi(val); 
				if(!strcmp(attr,"lMdr")) lvl1AddResult->lMdr=atoi(val);
				if(!strcmp(attr,"cExchange")) lvl1AddResult->cExchange=atoi(val);
				if(!strcmp(attr,"cRegion")) lvl1AddResult->cRegion=*val;
				if(!strcmp(attr,"cSymbol")) strncpy(lvl1AddResult->cSymbol,val,20);
				if(!strcmp(attr,"fSimClose")) lvl1AddResult->fSimClose=(float)atof(val);
				if(!strcmp(attr,"fSimPrevClose")) lvl1AddResult->fSimPrevClose=(float)atof(val);
				if(!strcmp(attr,"fSettlePrice")) lvl1AddResult->fSettlePrice=(float)atof(val);
				if(!strcmp(attr,"PrevCloseDate")) lvl1AddResult->PrevCloseDate=(atoi(val)*10000 +atoi(val +5)*100 +atoi(val +8));// 2010/02/01
				if(!strcmp(attr,"PrevCloseTime")) lvl1AddResult->PrevCloseTime=(atoi(val)*10000 +atoi(val +3)*100 +atoi(val +6));// 16:31:44
				if(!strcmp(attr,"LastTradeDate")) lvl1AddResult->LastTradeDate=(atoi(val)*10000 +atoi(val +5)*100 +atoi(val +8));// 2010/02/01
				if(!strcmp(attr,"LastTradeTime")) lvl1AddResult->LastTradeTime=(atoi(val)*10000 +atoi(val +3)*100 +atoi(val +6));// 16:23:51
				if(!strcmp(attr,"Currency")) 
				{
					if(!strcmp(val,"USD")) lvl1AddResult->Currency=CUR_USD;
					else if(!strcmp(val,"CAD")) lvl1AddResult->Currency=CUR_CAD;
					else if(!strcmp(val,"255")) lvl1AddResult->Currency=(char)CUR_UNRESOLVED;
					else lvl1AddResult->Currency=CUR_UNKNOWN;
				}
				if(!strcmp(attr,"ListingEx")) lvl1AddResult->ListingEx=*val;
				if(!strcmp(attr,"lVwapVol")) lvl1AddResult->lVwapVol=atol(val);
				if(!strcmp(attr,"fVwapSum")) lvl1AddResult->fVwapSum=atof(val);
				//fVwapAvg
				if(!strcmp(attr,"fImbalancePrice")) lvl1AddResult->fImbalancePrice=(float)atof(val);
				if(!strcmp(attr,"lImbalanceSize")) lvl1AddResult->lImbalanceSize=atol(val);
				if(!strcmp(attr,"lPairedSize")) lvl1AddResult->lPairedSize=atol(val);
				if(!strcmp(attr,"iImbalanceTime")) lvl1AddResult->iImbalanceTime=(atoi(val)*10000 +atoi(val +3)*100 +atoi(val +6)); // 00:00:00
				if(!strcmp(attr,"cImbalanceSide")) lvl1AddResult->cImbalanceSide=atoi(val);
				if(!strcmp(attr,"cImbalanceStatus")) lvl1AddResult->cImbalanceStatus=atoi(val);
				if(!strcmp(attr,"fPinkBid")) lvl1AddResult->fPinkBid=(float)atof(val);
				if(!strcmp(attr,"fPinkAsk")) lvl1AddResult->fPinkAsk=(float)atof(val);
				if(!strcmp(attr,"lPinkBidSize")) lvl1AddResult->lPinkBidSize=atoi(val);
				if(!strcmp(attr,"lPinkAskSize")) lvl1AddResult->lPinkAskSize=atoi(val);
				if(!strcmp(attr,"lCompBidSize")) lvl1AddResult->lCompBidSize=atoi(val);
				if(!strcmp(attr,"lCompAskSize")) lvl1AddResult->lCompAskSize=atoi(val);
			}
			fclose(fp);
		}
	}
	// return from _FindCspMdrRec
	SetEvent(lvl1Event);
	return 0;
}
int SMConsole::Lvl2Dlg(const char *apath)
{
	lvlxAppPath=apath;
	return (int)DialogBoxParam(theApp.m_hInstance,(LPCTSTR)IDD_LEVEL2,0,(DLGPROC)cbLevel2,(LPARAM)pdlg);
}
int SMConsole::Lvl2Answer(int rc, const char *fpath)
{
	// build internal lvl2 records from the file
	if(rc)
	{
		lvl1Result=0;
		lvl1AddResult=0;
	}
	else
	{
		FILE *fp=fopen(fpath,"rt");
		if(fp)
		{
			//Sym:A, Last:29.5000, Vol:4209393, PrevClose:29.1300
			//		BestBid:   29.2900,    200,                          BestAsk:   29.7100,    200
			//Bid:13,a,    ,C,   29.4800,3080000,164056,20100202,  Ask:13,a,    ,C,   29.5000, 420000,164056,20100202,
			//Bid:13, ,P__j, ,   29.4800,  31000,155950,20100202,  Ask:13, ,P__p, ,   29.5000,   4300,160010,20100202,
			//Bid:13,a,    , ,   24.5000,  10000,170000,20100202,  Ask:  , ,    , ,          ,       ,      ,        ,
			//Bid:13, ,A__f, ,   23.7300,   1000,160249,20100202,  Ask:  , ,    , ,          ,       ,      ,        ,
			char sym[32]={0};
			char rbuf[1024]={0};
			map<string,_CSPLVLXREC2 *> l2map;
			while(fgets(rbuf,sizeof(rbuf),fp))
			{
				if(!strncmp(rbuf,"Sym:",4))
					continue;
				else if(strstr(rbuf,"BestBid:"))
					continue;
				else if(!strncmp(rbuf,"Bid:",4))
				{
					char key[16]={0},sbid[512]={0},sask[512]={0};
					const char *aptr=strstr(rbuf,"Ask:");
					if((!aptr)||(aptr -rbuf>511))
						continue;
					strncpy(sbid,rbuf,aptr -rbuf);
					const char *cptr=strchr(aptr,'[');
					if(!cptr) cptr=aptr +strlen(aptr);
					if(cptr -aptr>511)
						continue;
					strncpy(sask,aptr,cptr -aptr);
					// Parse the bid side
					_CSPLVLXREC2 lvl2;
					memset(&lvl2,0,sizeof(_CSPLVLXREC2));
					if(lvl1Result)
						strncpy(lvl2.cSymbol,lvl1Result->cSymbol,20);
					int col=0;
					for(const char *tok=strtoke(sbid,",");tok;tok=strtoke(0,","))
					{
						col++;
						switch(col)
						{
						case 1: lvl2.cExchange=atoi(tok +4); break;
						case 2: lvl2.cRegion=(*tok==' ')?0:*tok; break;
                        case 3: 
							strncpy_s(lvl2.cMMID,sizeof(lvl2.cMMID)+1,tok,4); 
							if(lvl2.cMMID[0]==' ')
							{
								if(lvl2.cMMID[3]!=' ') // TSX
									sprintf(lvl2.cMMID,"%d",atoi(tok));
								else
									lvl2.cMMID[0]=0; // Blank
							}
							break; // cMMID is designed not to include null terminater
						case 4: lvl2.cSalesCondition=(*tok==' ')?0:*tok -'A'+1; break;
						case 5: lvl2.fBid=(float)atof(tok); break;
						case 6: lvl2.lBidSize=atol(tok); break;
						case 7: lvl2.lTime=atol(tok); break;
						case 8: lvl2.lDate=atol(tok); break;
						};
					}
					if(lvl2.cExchange)
					{
						if(lvl2.cMMID[0])
						{
							if(isdigit(lvl2.cMMID[0]))
								sprintf_s(key,16,"%s.%.4f",lvl2.cMMID,lvl2.fBid);
							else
							{
								strncpy(key,lvl2.cMMID,4);
								if(lvl2.cExchange == 11 && lvl2.cRegion == 'p')
								{
									key[4] = 'p';
									key[5] = '\0';
								}
								else if(lvl2.cExchange == 11 && lvl2.cRegion == 'k')
								{
									key[4] = 'k';
									key[5] = '\0';
								}
								else
									key[4] = '\0';
							}
						}
						else sprintf(key,"%c",lvl2.cRegion);
						map<string,_CSPLVLXREC2 *>::iterator l2it=l2map.find(key);
						_CSPLVLXREC2 *plvl2=0;
						if(l2it==l2map.end())
						{
							plvl2=new _CSPLVLXREC2;
							memcpy(plvl2,&lvl2,sizeof(_CSPLVLXREC2));
							l2map[key]=plvl2;
							lvl2Results.push_back(plvl2);
						}
						else
						{
							plvl2=l2it->second;
							plvl2->fBid=lvl2.fBid;
							plvl2->lBidSize=lvl2.lBidSize;
							plvl2->lTime=lvl2.lTime;
							plvl2->lDate=lvl2.lDate;
						}
						if(*cptr=='[')
						{
							plvl2->csBid[0]=atoi(cptr+1);
							const char *nptr=strchr(cptr+4,'[');
							if(nptr) nptr--;
							else nptr=cptr +strlen(cptr);
							if(plvl2->csBid[0]>0)
							{
								cptr+=4;
								if(nptr -cptr>94)
									memcpy(plvl2->csBid +1,cptr,94);
								else
									memcpy(plvl2->csBid +1,cptr,nptr -cptr);
								plvl2->csBid[95]=0;
								for(char *sptr=plvl2->csBid +strlen(plvl2->csBid) -1;(sptr>plvl2->csBid)&&(*sptr==' ');sptr--)
									*sptr=0;
							}
							cptr=nptr;
							if(*cptr==',') cptr++;
						}
					}

					// Parse the ask side
					memset(&lvl2,0,sizeof(_CSPLVLXREC2));
					if(lvl1Result)
						strncpy(lvl2.cSymbol,lvl1Result->cSymbol,20);
					col=0;
					for(const char *tok=strtoke(sask,",");tok;tok=strtoke(0,","))
					{
						col++;
						switch(col)
						{
						case 1: lvl2.cExchange=atoi(tok +4); break;
						case 2: lvl2.cRegion=(*tok==' ')?0:*tok; break;
                        case 3: 
							strncpy_s(lvl2.cMMID,sizeof(lvl2.cMMID)+1,tok,4); 
							if(lvl2.cMMID[0]==' ') // blank or TSX
							{
								if(lvl2.cMMID[3]!=' ') // TSX
									sprintf(lvl2.cMMID,"%d",atoi(tok));
								else
									lvl2.cMMID[0]=0; // Blank
							}
							break; // cMMID is designed not to include null terminater
						case 4: lvl2.cSalesCondition=(*tok==' ')?0:*tok -'A' +1; break;
						case 5: lvl2.fAsk=(float)atof(tok); break;
						case 6: lvl2.lAskSize=atol(tok); break;
						case 7: lvl2.lTime=atol(tok); break;
						case 8: lvl2.lDate=atol(tok); break;
						};
					}
					if(lvl2.cExchange)
					{
						if(lvl2.cMMID[0])
						{
							if(isdigit(lvl2.cMMID[0]))
								sprintf_s(key,16,"%s.%.4f",lvl2.cMMID,lvl2.fAsk);
							else
							{
								strncpy(key,lvl2.cMMID,4);
								if(lvl2.cExchange == 11 && lvl2.cRegion == 'p')
								{
									key[4] = 'p';
									key[5] = '\0';
								}
								else if(lvl2.cExchange == 11 && lvl2.cRegion == 'k')
								{
									key[4] = 'k';
									key[5] = '\0';
								}
								else
									key[4] = '\0';
							}
						}
						else sprintf(key,"%c",lvl2.cRegion);
						map<string,_CSPLVLXREC2 *>::iterator l2it=l2map.find(key);
						_CSPLVLXREC2 *plvl2=0;
						if(l2it==l2map.end())
						{
							plvl2=new _CSPLVLXREC2;
							memcpy(plvl2,&lvl2,sizeof(_CSPLVLXREC2));
							l2map[key]=plvl2;
							lvl2Results.push_back(plvl2);
						}
						else
						{
							plvl2=l2it->second;
							plvl2->fAsk=lvl2.fAsk;
							plvl2->lAskSize=lvl2.lAskSize;
							plvl2->lTime=lvl2.lTime;
							plvl2->lDate=lvl2.lDate;
						}
						if(*cptr=='[')
						{
							plvl2->csAsk[0]=atoi(cptr+1);
							const char *nptr=cptr +strlen(cptr);
							if(nptr[-2]==',') nptr-=2;
							if(plvl2->csAsk[0]>0)
							{
								cptr+=4;
								if(nptr -cptr>94)
									memcpy(plvl2->csAsk +1,cptr,94);
								else
									memcpy(plvl2->csAsk +1,cptr,nptr -cptr);
								plvl2->csAsk[95]=0;
								for(char *sptr=plvl2->csAsk +strlen(plvl2->csAsk) -1;(sptr>plvl2->csAsk)&&(*sptr==' ');sptr--)
									*sptr=0;
							}
							cptr=nptr;
						}
					}
				}
			}
			fclose(fp);
		}
	}
	// return from FindFirstCspLvlXRec
	SetEvent(lvl2Event);
	return 0;
}
int SMConsole::MontageDlg(const char *apath)
{
	lvlxAppPath=apath;
	return (int)DialogBoxParam(theApp.m_hInstance,(LPCTSTR)IDD_MONTAGE,0,(DLGPROC)cbMontage,(LPARAM)pdlg);
}
int SMConsole::MontageAnswer(int rc, const char *fpath)
{
	return Lvl2Answer(rc,fpath);
}
long SMConsole::_FindCspMdrRec(char *pSymbol, BYTE cExchange, BYTE cRegion)
{
	if(!lvl1Event)
		lvl1Event=CreateEvent(0,false,false,0);
	// Send lvl1 command
	char mbuf[2048]={0},*mptr=mbuf,parms[256]={0};
	sprintf(parms,"DeliverTo=%s|OnBehalfOf=SMConsole|Symbol=%s|FileKey=lvl1.txt|",lvlxAppPath.c_str(),pSymbol);
	ccodec->Encode(mptr,sizeof(mbuf),true,++rid,"lvl1",parms,0);
	WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,0);
	#ifdef _DEBUG
	WaitForSingleObject(lvl1Event,INFINITE);
	#else
	WaitForSingleObject(lvl1Event,3000);
	#endif
	if(lvl1Result)
		return 4;
	return 0;
}
void SMConsole::ReadCspMdrRec( _CSPMDR *pMdr, long lRec )
{
	if(lvl1Result)
	{
		memcpy(pMdr,lvl1Result,sizeof(_CSPMDR));
	}
}
long SMConsole::GetCspAddRec(long lRec,_CSPADDREC *pAddRec)
{
	if(lvl1Result)
	{
		delete lvl1Result; lvl1Result=0;
	}
	if(lvl1AddResult)
	{
		memcpy(pAddRec,lvl1AddResult,sizeof(_CSPADDREC));
		delete lvl1AddResult; lvl1AddResult=0;
		return 0;
	}
	return -1;
}
long SMConsole::FindFirstCspLvlXRec2(_CSPMDR *pmdr, _CSPLVLXREC2 **pplvl2, BOOL applyFast, BOOL showdel)
{
	if(!lvl2Event)
		lvl2Event=CreateEvent(0,false,false,0);
	// Send lvl2 command
	char mbuf[2048]={0},*mptr=mbuf,parms[256]={0};
	sprintf(parms,"DeliverTo=%s|OnBehalfOf=SMConsole|Symbol=%s|ApplyFast=%s|ShowDel=%s|FileKey=lvl2.txt|",
		lvlxAppPath.c_str(),pmdr->cSymbol,applyFast?"Y":"N",showdel?"Y":"N");
	ccodec->Encode(mptr,sizeof(mbuf),true,++rid,"lvl2",parms,0);
	WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,0);
	#ifdef _DEBUG
	WaitForSingleObject(lvl2Event,INFINITE);
	#else
	WaitForSingleObject(lvl2Event,3000);
	#endif
	if(lvl2Results.empty())
		return 0;
	_CSPLVLXREC2 *plvl2=lvl2Results.front();
	*pplvl2=plvl2;
	return 1;
}
long SMConsole::FindNextCspLvlXRec2(_CSPLVLXREC2** pplvl2, long idx)
{
	if(idx>=(int)lvl2Results.size())
		return 0;
	int i=0;
	list<struct _CSPLVLXREC2 *>::iterator l2it;
	for(l2it=lvl2Results.begin();(l2it!=lvl2Results.end())&&(i<idx);l2it++)
		i++;
	_CSPLVLXREC2 *plvl2=*l2it;
	*pplvl2=plvl2;
	return idx +1;
}
void SMConsole::FindCloseCspLvlXRec()
{
	for(list<struct _CSPLVLXREC2 *>::iterator l2it=lvl2Results.begin();l2it!=lvl2Results.end();l2it++)
		delete *l2it;
	lvl2Results.clear();
}
#endif//!DIALOG_DLLS
