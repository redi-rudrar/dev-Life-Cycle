
#ifndef _IQCLSERVER_H
#define _IQCLSERVER_H

#include "sizes.h"

typedef struct tdPosiStats
{
	float Balance[4];
	float Position[4];
	float MaintRequired[4];
	float MoneyMarket;
	float SMA;
	float OutstandingRegT;
	float MarginEquity;
	float TotalEquity;
	float MarginMarketValue;
	float TotalMarketValue;
	float MaintExcess;
	float DayTradeBp;
	float RegTBp;
	float OverNightBp;
	float PersEquity;
	float CreditIntBal;
} POSISTATS;


typedef struct tdAccountRec
{
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char Entity[20];
	char LastName[NAME_LEN];
	char MiddleName[NAME_LEN];
	char FirstName[NAME_LEN];
	char Address1[NAME_LEN];
	char Address2[NAME_LEN];
	char Address3[NAME_LEN];
	char Address4[NAME_LEN];
	char City[20];
	char State[4];
	char Zip[12];
	char TelHome[TEL_LEN];
	char TelOffice[TEL_LEN];
	char Fax[TEL_LEN];
	char Cell[TEL_LEN];
	char BirthDate[DATE_LEN];
	char SSNId[20];
	char IRA;
	char BprBuyStocksYn;

    union {
        char  CanTradeBits;        /* 1111/1111 */
        struct {
            bool	   CanTradeSpare1			: 1;	//1
            bool       CanTradeOtcBB			: 1;	//2 True if account allowd to trade 11k OTCBB
            bool	   IgnoreShortlistOtcBB		: 1;	//3
            bool	   CanShortOtcBB			: 1;	//4
            bool	   CanTradeFutures			: 1;	//5 FUTURES - Can BUY
            bool	   CanShortFutures			: 1;	//6	FUTURES - Can SHORT
            bool	   IgnoreShortlistFutures	: 1;	//7	FUTURES - Ignore Shortlist
            bool	   IgnoreExchangeThresholdList: 1;	//8	Set - it will bypass the Exchange Threshold files for this account
        };

    };


	char BprBuyOptionsYn;
//	char BprBuyOptionsCash_Deleted;
    union {
        char  BpBits;        /* 1111/1111 */
        struct {
            bool       IgnoreSellPosiCheck	: 1;	//1 True to Sell Stock Not Owned
            bool	   AllowSecondaryCover	: 1;	//2 True to Alloc Trying for Secondary Cover
            bool	   CanUseOMS			: 1;	//3 True if account is used for OMS
            bool	   UseRouteList			: 1;	//4 True if we want the account restricted route list
            bool	   DoNotLoadPositions	: 1;	//5 td 3395
            bool	   ForceStockLoans		: 1;	//spare5
            bool	   IsBd	: 1;	//spare6
            bool	   TradeTestStocksOnly		: 1;	//1 If true, user can only trade TEST stocks
        };

    };
	char BprShortStocksYn;
	char AccountDropPortId;
	char BprAlwaysAllowToClosePositionYn;
	char SagePortDeleted;
	char BprSellCoYn;
    union {
        char  Bits;        /* 1111/1111 */
        struct {
            bool       IgnoreBpLimit	: 1;	//1 True if we dont limit account to BP
            bool	   CanDoAwayOrders	: 1;	//2	True if account can do AWAY orders
            bool	   IgnoreTradeRulesAfterHours	: 1;	//
            bool	   CanShortUnknownOptions	: 1;	// true to override domain switch to prevent short of un-found opitons
            bool	   UseOverNightReclaimedBP	: 1;	//	true if account can use reclainmed ov Bp
            bool	   IgnoreShortlist		: 1;	//
            bool	   DVP					: 1;	// TRUE IF DVP ACOUNT
            bool	   IgnoreShortlistAfterHours: 1;	//
        };

    };

	char BprSellNoYn;
	char AdpPortDeleted;
	char DoNotReport2Clearing;
	char IgnoreTradeRules;		// is now IGNORE TRADING RULES in BO	- KMA changed name from IgnoreBP - 'cause its confusing
	char Group[5][USER_LEN];
	char CommissionStruct[20];
	double BprBuyStocksAmt;
	double BprBuyOptionsAmt;
	double BprShortStocksAmt;
	double EqtMultiplier;
	double EquityMinimumTrigger;
	double BprSellNoAmt;

    union {
        short Bitfields;        /* 1111/1111 /1111/1111 */
        struct {
            bool       Monitored			: 1;	//1 True if account is monitored
            bool	   UseTradeStartTime	: 1;	//2	True if the TradeStartTime is set
            bool	   UseTradeEndTime		: 1;	//3	True if the TradeEndTime is set
            bool	   DumpByTime			: 1;	//4	True if DumpAllTime is set
            bool	   DumpByEquity			: 1;	//5 True if EquityMinimumTrigger is set
            bool	   TradingHalted		: 1;	//6 True if user is not allowed to trade - halted
            bool	   Dumping				: 1;	//7  We are busy dumping th account
            bool	   UseShareLimitDollars	: 1;	//8
            bool       UseShareLimitShares	: 1;	//9
            bool	   CapacityPrincipal	: 1;	//10 Account is Agent ot Principal
            bool	   CanTradeHedges		: 1;	//11 True if account can buy hedges
            bool	   DoesNotDayend		: 1; 	//12 True if account does not carry positions to next day
            bool	   DisableAutoDivider	: 1;
            bool	   CanDoProgramedOrders	: 1;
            bool	   CanOverrideOpenClose	: 1;	// user decides whether he is opening/closing
            bool	   UsePerTradeLimits	: 1;	// True if we should check cost and shares per trade

        };

    };
	short SagePort;
	long  DumpAllTime;

	double Eai;
	double Enw;
	double Elnw;
	double IntDep;
	double YtoDPl;
	double MtoDPl;
	char Notes[NOTES_LEN-5];
	char RRNumber[5];
//	ACCHIST AccHist[24];
	char ClearingAccount[ACCOUNT_LEN];
	float PerTrade_EqLimitDollar;
	long  PerTrade_EqLimitShares;
	float PerTrade_FutLimitDollar;
	long  PerTrade_FutLimitShares;
	float PerTrade_OptLimitDollar;
	long  PerTrade_OptLimitShares;
	float PerTrade_OtcBbLimitDollar;
	long  PerTrade_OtcBbLimitShares;

	double LimitSharesDollarValueFutures;
	long LimitSharesValueFutures;
	double LimitSharesDollarValueOptions;
	long LimitSharesValueOptions;
	double LimitSharesDollarValueOTC;
	long LimitSharesValueOTC;
	char Fund[80];						// Field may consist of mulitple fund name/#'s - Separated by ;    /// NOT IN DB
	char TranTypeEquity;				/// default transaction type for equity trades
	char TranTypeOptions;				/// default transaction type for option trades
    union {
        char  MoreBits;        /* 1111/1111 */
        struct {
            bool	AllowTranTypeOverride	: 1;	// Allow Transaction Type override (from FIX or FE)
            bool	AllowSolicitedOverride	: 1;	// Allow Solicited flag override (from FIX or FE)
			bool	OMSTradingAccount		: 1;
			bool	UseIntraDayExposureOnly : 1;	// DT#41 intraday total exposure calculation
			bool	DoNotForceReloadOnBoEdtit : 1;	// DT#SWIMFAST
			bool	SpecialDemoX			: 1;	// If true, we allow small executions doing DEMOX - off means we dont do small fills
			bool	FacilitatingContraAccount	: 1;	// 12may2009 troymguillory DT5453
			bool	SpreadsEquityLeg			: 1;	// DT6254 - kma - R4 2009 Optronics entitlement
        };

	};
	char SolicitedFlag;				// S=Solicited, U=Unsolicited, blank=unknown

	// + DT6616 - Restricted List (CL)
	union {
		unsigned long MoreBits2;	// DT7031 DLA 20100326
		struct {
			bool	IgnoreRestrictedList : 1;	// Do not take the domain restricted list into account
			bool	OptionsProfessional : 1;  // DT 8462 02/17/2011 JonHull
		};
	};
	// - DT6616 - Restricted List (CL)

	// DT6566 YL 20091216 - start
	union {
		char MarginSettings;
		struct {
			bool	UseNewMargin	: 1;	// Use new margin engine instead of buying power
			bool	BOReleaseIntradayBP : 1;	// DT7001 DLA 20100401, used only for sending ACCINDKEY.ReleaseIntradayBP to backoffice
		};
	};
	// DT6566 YL 20091216 - end

	char ReleaseIntradayBPSwitch;		// see enum eReleaseIntradayBP for values // DT7001 DLA 20100401

	// + DT7809 - bjl - Additional Client Identifiers
	char Spare[2178];					/// NOT IN DB	// DT6566 YL 20091216	// DT6616 - Restricted List (CL) // DT7031 DLA 20100326  // DT7001 DLA 20100401	// DT9033 DLA 20110425

	// + DT9033 DLA 20110425
	char oats_ReceivingDepartmentID[13];
	char oats_OriginatingDepartmentID[13];
	char oats_DeskTypeCode[13];
	char oats_AccountTypeCode2;
	char oats_MemberTypeCode2;
	char oats_OrderOriginationCode;
	char oats_RoutingFirmMPID[5];
	char oats_InformationBarrierID[13];
	// - DT9033 DLA 20110425

	char LegalName[46];
	char Copper[16];
	char IPS[16];
	char CCIValue[46];
	char SOMSID[16];
	char ClientType[16];
	// - DT7809

	char oats_AccountTypeCode;
	char oats_MemberTypeCode;
	char oats_FirmMPID[5];

    union {
        char  Canadians;        /* 1111/1111 */
        struct {
			bool	BprBuyCanadianStockYn	:1;
			bool	BprShortCanadianStockYn : 1 ;
			// DT8552 DLA 20110120 - deleted pink sheet code
		};
	};
	double MaxBpUsed;
	double TotalExposureSelloutValueUNUSED;
	double TotalExposureLimitValue;

#define MAX_CMTA 8
	char CMTARoute[MAX_CMTA][ECN_LEN];	/// NOT IN DB
	short CMTACode[MAX_CMTA];			/// NOT IN DB
	//	ACCBPSETUP AccBpSetup;			 // when useing - start uusing from the bottom because the ACcBpSetup grows from the top
	char PosiStartDate[DATE_LEN];
	POSISTATS PenStat;					/// NOT IN DB
	double BprOverRide;
	long TradeStartTime;	// Trading hours
	long TradeEndTime;
	double BprDivider;
	double ComsPar[3];
	char OfficeCode[5];
	char  Field_deleted;					/// NOT IN DB
	short AdpPort;

	long LimitSharesDollarValue;		// used for equity trades, see above for futures, options, and otcbb.
	long LimitSharesValue;				// used for equity trades, see above for futures, options, and otcbb.
} ACCOUNTREC;

typedef struct tdExSymbol
{
	char Exchange;
	char Region;
	char Symbol[SYMBOL_LEN];
	unsigned char CurrencyCode;
	char Reserved;

	operator const char*()
	{	static char s[SYMBOL_LEN+12];
		::sprintf_s(s,sizeof(s),"%s,%d,%c,%d",Symbol,Exchange,Region,CurrencyCode);
		return s;
	}

} EXSYMBOL;

typedef struct tdTradeRec //Master Record for all Trades
{
	// Start of Fields to Place Order
	char User[USER_LEN];		// Name of User that places the order
	char Account[ACCOUNT_LEN];	// Account order was placed in
	char Domain[DOMAIN_LEN];	// Domain that Account belongs too
	EXSYMBOL ExSymbol;			// Symbol name including Exchange and Region
	char PrefECN[ECN_LEN];		// Route selected by User in Front End
	char PrefMMID[ECN_LEN];		// Prefrenced Market Marker / Option Exchange
	char OrderNo[ORDERNO_LEN-4];	// Internal OrderNo assigned to Order YYYYMMDD-XXXXXXXX
	long WorkOrderRefNo;		// stole 4 bytes from orderno!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	long Shares;				// Number of Shares for Order
	long ReservedShares;		// Number of Shares To Show OR Set to (-1) for Invisible Order on ISLD
	WORD Format;				// Format Code: (0x0001-0x000F, 1-15): Fraction=1/2^Format) AND/OR (0x0100-0x0F00, 256-3840): Decimal with 0x0100=1 Decimal, 0x0200=2 Decimals, etc
	char Tick;					// U=Uptick,D=DownTick.....
	char ClosePosition;			// Set to 1 to indicate that transaction will close a position.
	float Price;				// Price for Order
	float StopAmount;			// Stop Trigger Price for Market/LimitStops OR Trailing Amount for TrailStop
	float OptDayRootPrice;		// Current Qoute of Option Root Symbol
	float DiscretionaryAmount;	// Amount for Discretionary Order OR Used for Pegged if ISLD 1=PegMkt 2=PegBest
	float OptLockRootPrice;		// Option Root Price Locked in at the last Option Fill
	float OptStrikePrice;		// Strike price for Option
	float OptConversion;		// Conversion Rate for Option as per OCC
	char AccType;				// 0: Margin 1:Cash
	char Action;				// 1:buy, 2:Sell, 3:Short, 4:Exercise Option
	char Type;					// 1:Market, 2:Limit, 3: Exercise Day, 4: Exercise Overnight
	char Gtc;					// True / False
	char Ext;					// True / False
	char LastStatus;			// See defines following at the end of this structure
//	char Aon;					// bit0=AON bit1=IOC/ArcaNow bit2=OnOpen bit3=OnClose bit4=BookOnly bit5=FOK bit6=NyseDirect
	union
	{
		char Aon;					// bit0=AON bit1=IOC/ArcaNow bit2=OnOpen bit3=OnClose bit4=IsldBookOnly bit5=FOK bit6=NyseDirect
		struct
		{
			unsigned char AON				: 1;	//0x01
			unsigned char IOC_ArcaNow		: 1;	//0x02
			unsigned char OnOpen			: 1;	//0x04
			unsigned char OnClose			: 1;	//0x08
			unsigned char BookOnly			: 1;	//0x010
			unsigned char FOK				: 1;	//0x020
			unsigned char NyseDirect		: 1;	//0x040
			unsigned char AON_ArcaNow		: 1;	//0x080
		};
	};
	char PrefMMYN;				// Set to 1 if Market Maker was preferenced
	char StopType;				// 1:Stop // 2: Traling Stop
	char OrigAction;			// Original Action Selected by user, this could differ from Action when system change Short to Sell
	char CancelCount;			// Number of times the User requested a Cancel
	char LastDate[9];			// Date of LastStatus Change
	char LastTime[9];			// Time of LastStatus Change
	char ECNOrderNo[ORDERNO_LEN];  // OrderNo assigned for ECN OR!!!! Original OrderNo for Replace
	char ECNExecID[20];			// Execution ID assigned by ECN for a specific fill
	char ECNId[ECN_LEN];		// Actual Route Used will be equal to PrefECN unless PrefECN="Default" and the system assigned a route
	char MMId[ECN_LEN];			// Actual Marker Maker / Contra Account
//	bool CancelRequested;		// Set if User has selected atleast one Cancel / Used to detect unsolisited Cancels
    union {
        char	MoreBits1;    // 1111/1111
        struct {
			bool	CancelRequested: 1; // Set if User has selected atleast one Cancel / Used to detect unsolisited Cancels
			bool Hunter		: 1; //
			bool VWAP			: 1; //
			bool TWAP			: 1; //
			bool InLine_Lmt	: 1; //
			bool CrossingOrder	: 1; //6
			bool PreOpen		: 1; // If Crossing_Order: 0-EOD Cross, 1-PreOpen
			bool ECNStop		: 1; // This is the original e-stop incdicator. Never set incoming, but is set on the way back to the frontend
		};
    };

	char PosibleDupp;			// Set if ECN Playback indicate the this OrderStatus is a posible Dupplicate
	char LastCapacity;			//  1= Agent 2 = Cross as agent 3 = Cross as principal 4 = Principal
	long Pending;				// No of Shares still outstanding
	long ExeShares;				// Total No of Shares Executed
	long LastShares;			// No of Shares fill on the this PartFill
	long Canceled;				// No of Shares that was Canceled/Rejected/Expired by ECN
	long OpenShares;			// CALCULATED:No of Shares that this Order Contribute to Day Open Position
	long PrevOpenShares;		// No of Shares that this Order Contribute to OverNight Open Position
	long ClosedShares;			// CALCULATED:No of Shares from this Order that is allreasdy closed out again
	double AvgPrice;			// Average Price of the total Execution
	double LastPrice;			// Price of this Partfill
	double Coms;				// Commisions payable on this order
	double OpenCost;			// CALCULATED:Total Cost of the OpenShares
	union {						// DT7706 bud 08/12/10
		char OptClass[4];			// 3 Character Option Class if this a option // DT7706 bud 08/12/10
		float BestBidWhenPlaced;	// DT7706 bud 08/12/10
	};							// DT7706 bud 08/12/10
	long ClosedSharesNotAgainstTheBox;	// Number od shares for calc pnl in FE
	double AvgOpenPrice;		// CALCULATED:Average Price for OpenShares
// begin DT#5442 - KMA - INTERNAL SPEEDUP - Capacity improvements require this
#ifdef _CLSERVER_
	unsigned long OriginalOrderNumber;	// replaced unsigned short versions 
	unsigned long PrevOrderNumber;		// replaced unsigned short versions 
#else
	double M2MPrice;			// Price Of this stock at yesterdays close if this is a overnight position KMAKMA NEVER USED IN CLS
#endif
// end DT#5442 - KMA - INTERNAL SPEEDUP - Capacity improvements require this

	double CloseCost;			// CALCULATED:Total Income from Closing the order
	long   OptExpireDate;		// Option Expiration Date
	float DisplayValue;			//Peg Diff
	double AvgClosePrice;		// CALCULATED:Average Closing Price for this Order
	float CurrentStopPrice;		// Current Trigger Price for Trailing Stops
	char OptRootExchange;	 	// Exchange Code for Option Root Symbol
	char OptRootRegion;			// Region Code for Option Root Symbol
	char OptType;				// Put='P' Call='C'
    union {
        char	Bitfields;        /* 1111/1111  */
        struct {
            bool       DoNotClear		: 1;	//1
            bool	   OverrideComms	: 1;	//2	True if user has overwritten coms field
            bool	   OverrideECNfee	: 1;	//3	true if  user has overwritten EcnFee field
            bool	   CommsGenerated	: 1;	//4	True if this line had had comms generated
            bool	   CreatedByBO		: 1;	//5 True if this record was created by a BO action
            bool	   EditedByBo		: 1;	//6 True if edited by a BO user
            bool	   LiquidityReported: 1;
            bool	   AddLiquidity		: 1;	// if LiquidityReported then this is add or remove

        };

    };
	long  GtcDate;				// Last Date for Good Till Cancel Orders
	long  TradeDetRef;			// Database unique reference number for this record
	char  OptRootSymbol[8];		// Option Root Symbol
	double CloseProfit;			// CALCULATED:Profit made from closing this order
	float  ECNFee;				// Fee Charged by ECN
	union {
        char PegOrder;        /* 1111/1111 */
        struct {
            unsigned char    PegMkt : 1; //1 Peg-Mkt
            unsigned char    PegBest : 1; //2 Peg-Best
            unsigned char    PegLast : 1; //3 Peg-Last
            unsigned char    PegMid : 1; //4 Peg-Mid
            unsigned char    PegPrime : 1; //5 Peg-Prime
            unsigned char    ClosingOffset : 1; // DT6981 bud 04/16/09
            unsigned char    MidPointMatch : 1; // DT6958 bud 08/25/10
            unsigned char    OT_8 : 1; //8 OrderType PEGS ONLY
		};
	};
	union {
        char	OrderTypeBits[3];        /*1111/1111 1111/1111 1111/1111 */
        struct {
            unsigned char      PegWithNoLimit	: 1;	//9 For Mkt Pegs
            unsigned char	   Peg_Display_Type	: 5;	//10 For Type of Display Pegging
            unsigned char	   Disc_Sign		: 1;	//13 0 Pos, 1 Neg
            unsigned char	   EcnRandomReserveRangeSet: 1;	// true means EcnRandomReserveRange value is to be used (FE sends it in using ExeShares)

			bool			   BofaExecutionServicesOrder : 1; // true if this order is on either side of a BOFA execusion services route
			bool			   BofaExecutionServicesDomain : 1; // true if this order came into a BOFA Execution Services domain
			bool			   HideMMID	 : 1;
			bool			   Liquidity2BytesReported : 1;	// DT#LIQUIDITY - True means We added a 2 byte liquidity field, encoded into the PartFill and Filled 710 message, in RejectReasonText[123] and RejectReasonText[124]

            //unsigned char EASE_TargetStrat_deleted		: 4;	//16EASE Target Strategy


//            unsigned char EASE_ExecMode_deleted		: 4;	//20EASE Execution Mode
			bool			   CoreSession : 1;			// DT#5040 - kma - 3/4/9 - add capability to handle core only trading
			bool			   CancelledByExchange : 1; // passed in from trader - means EXPIRED is actually cancelled by exchange  DT5204 - IQ Bridge: SEB request for CancelledByExchange
			bool			   OptronicsTradeFlag		  : 1;	// DT6254 bud 10/19/09
			bool			   EcnGtd : 1;		// DT6383 - XTrade Gap (CL) – Native GTCs - TRUE means its Exchange based.

            unsigned char      OvernightShortAgainstTheBox: 1;	//25OrderType
            unsigned char	   FileDateContainsCAMS		  : 1;	//26 Cam codes in FileDate and FileTime
            unsigned char	   CommissionOverrideType	  : 2;	//27 00=fixed, 1=Per Share, 2=Percentage, 3=unused
            unsigned char	   RoutedAway: 1;	//LiquidityReported=0 because routed away
            unsigned char	   ShortLocateAffirmation: 1;	//30 Affirmation for Shorting off the Short List : BOFA only, after they say "OK" to the "Do you affirm?" msg
            unsigned char	   RegSho200Exempt: 1;			//31 True if stock was found in the Short Rule Exempt list
            unsigned char	   OddLot: 1;	//32 Populate before sending to trader TD:2586
		};
    };
	char   FileDate[9];		// Date of the Record

    union {
        char	OrderOrigin;        /* 1111/1111  */
        struct {
            bool       OriginFromTradeSplit		: 1;	//1 Order created by a trade split
            bool	   OriginOptionFromExercise	: 1;	//2	Option part of exercise
            bool	   OriginEqtyFromExercise		: 1;//3	Equity paft of exrcise
            bool	   OriginClearing			: 1;	//4	Created from Import Positions
            bool	   OriginOvernightExercise	: 1;	//5 True if Origin was from overnight exercise
            bool	   OrderWasCancelReplaced	: 1;	//6
            bool	   CancelReqFromBO			: 1;	//7	True if this came from a BO - So force the send to ECN
            bool	   HasShortSellCode			: 1;	//8 true if User supplied a SHORT SELL CODE to Authorise short

        };

    };
    union {
        char	MorebitFields;        /* 1111/1111  */
        struct {
            bool       OrderConfirmed			: 1;	//1 true if a confirm was received
            bool	   ReplaceRequest			: 1;	// true if this order is a CancelReplace of another
            bool	   OverrideOpenClose		: 1;	// Comes in and this says we're using FE override for OPEN and CLOSING Transactiopns
            bool	   ClosingTrade				: 1;    // OverrideOpenClose==true -> This field indicates whether its a Buy to CLOSE or a SELL to CLOSE (using Action)
            unsigned char 	   MasterSlave		: 2;
            bool	   Express			: 1;
            bool	   ArcaEx			: 1;

        };

    };
	char	UserType;
	long    ClientRef;
} TRADEREC;



#define ORDER_NORMAL 0
#define ORDER_MASTER 1
#define ORDER_SLAVE 2
#define ORDER_ROLLUP 3
#define ORDER_OUTGOING -1

typedef struct tdTradeRecA
{
	char Memo[23];
	char OrderType; //ORDER_TYPE_DUMPALL, STD, etc
//	int  SpareInt;

	short OriginalOrderDate;		// stored as Number of days since 01/01/2000
	unsigned short OriginalOrderNumber_old;		// atol of order DT#5442 - KMA - INTERNAL SPEEDUP - Capacity improvements require this

	short FIXLineId;
	unsigned short EcnRandomReserveRange; // 0-50000 random reserve range

	char CancelMemo[23];
//	char SpareChar;
    union {
        char	BitFieldsA;        /* 1111/1111  */
        struct {
            bool       DoNotKillIfGtc			: 1;	//1 True means keep it if its a gtc which is cancelled
            bool	   DoNotPlaceOrder			: 1;	//2	True if this order should stay RECEIVED - used to keep expired GTC's
            bool	   ConvertToMarketWhenCancelled: 1;	//3	if set, this order will be replaced as a market order when the cancel comes back from the trader
            bool	   GtcWasReplaced			: 1;	//4	True if order was replaced after it expired
            bool	   ReplaceOrderWhenCancelled: 1;	//5 if set Order Will Be Replaced NewShares=OldPending
            bool	   ExchangeThresholdChecked		: 1;	//6 true if this is a short and the exchange's Hard2Borrow was checked
            bool	   HoldUntilEcnAvailable	: 1;	//7	true on RECEIVED orders (GTC) which could not be placed because the ECN was not up
            bool	   Deleted					: 1;	//8 TRUE if this line is deleted

        };

    };

	short PlacePort;
	char ECNParserOrderNo[ORDERNO_LEN];
//	char ReplaceOrderNo[ORDERNO_LEN];
	char TrailTriggerCount;
	char StopTriggerCount;

	short StepRefNo;		// Step on execution path - can be a short	// DT7706 bud 08/12/10
	char ShortExemptCode;												// DT7706 bud 08/12/10
	char OverrideRestrictedListCode; //0 = CC14 override, 1 = Emergency Override, 2+ custom	// DT7668 - bjl - New Restricted List	// DT7706 bud 08/12/10

	char Notes[8];			// used for Short Sale Request Id
	char ExecutingBrokerCode[7];

//	long WorkOrderRefNoX;
//	char Spare[3];

	char LinkOrderNo[ORDERNO_LEN - sizeof(long)];
#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
#endif
	long ComplexSharesRatio;							// Shares/complex order for this leg
#ifdef WIN32
#pragma pack(pop)
#endif
    union {
        char	BitFieldsB;        /* 1111/1111  */
        struct {
            bool       T13Checked	: 1;	//1 True means T13 list was checked
            bool	   OatsNonDirectedOrder	: 1;	//2	true if order is deliberately set to NON-DIRECTED by user
            bool	   OatsManualTicket	: 1;	//3	True if Order has manual received date /time
            bool	   NotForAllocation	: 1;	//4	if true - this trade must not make it into allocation/aggregate maps
            bool	   AutoAllocate	: 1;	//5 if true - Allocate this order to account in BigtradeRec...AltAccount2
            bool	   FlipOnExport	: 1;	//6 use altaccount for export
            bool	   OverrideRestrictedList : 1; // true if user overrides restricted list
            bool	   OverrideTradeDateTime  : 1;	// DT6089 YL 20091029 - true if user overrides trade date and time on ticket

        };

    };

}TRADERECA;

typedef struct tdBatchOrderInfo
{
	char BatchName[9];
	char Spare1;

	WORD	Row;
	long	BatchNumber;

} BATCHORDERINFO;

#define CAPACITY_UNKNOWN			0
#define CAPACITY_AGENCY				1
#define CAPACITY_CROSS_AS_AGENCY	2
#define CAPACITY_CROSS_AS_PRINCIPAL	3
#define CAPACITY_PRINCIPAL			4
#define CAPACITY_RISKLESS_PRINCIPAL	5 // BOFA says so - not in FIX spec


#define STRAT_ECN_GETS				1			//CIBC's Version of EASE - kleder - 05.26.2005 - TD#2125

typedef struct tdTradeRecB
{
	long EASE_EffTime;
	long EASE_ExpTime;
	long EASE_PartRate;

    unsigned char EASE_TargetStrat;	//EASE Target Strategy
    unsigned char EASE_ExecMode;	//EASE Execution Mode
	char OptRootType; // 1=index - refer to tdOptRoot for more info

	union// starts on boundary
	{
		unsigned char Bits2;
		struct
		{
			unsigned char aoAccountHasIgnoreThresholdListSet	: 1; // True is the account setting is on to ignore Threshold Files
			unsigned char aoDomainIgnoresThresholdOnShortLocates : 1; //10
			bool DeskStageChild		: 1;			// DT6084 Bud 10/10/09	
			bool FacilitatedOrder	: 1;			// If this order is a facilited order DT5453 Bud 05/14/09
			bool ComplexOrder		: 1;			// If this order is part of a complex order
			bool ComplexOrderParent	: 1;			// If this order is the parent of leg orders
			bool OrderStagingOrder	: 1;			// DT4419 If this order is associated with Order Staging bud 10/27/08
			bool OrderStagingParent	: 1;			// DT4419 If this order is an Order Staging Parent bud 10/27/08
		};
	};


	union// starts on boundary
	{
		char TRBBits[2];
		struct
		{
			unsigned char EASE_Duration     : 1; //EASE_ExpTime = 0: ExpTime - 1: Duration
			unsigned char EASE_ExpTimeClose : 1; //If ExpTime needs to be set to Close
			unsigned char EASE_EffTimeNow   : 1; //If the Efftime is set to NOW			unsigned char Bit4  : 1; //4
			unsigned char EASE_Anonymous    : 1; //Anon Order - NOT JUST FOR EASE

			unsigned char SHWB_Order		: 1; //True if Order is for SCHWAB Not EASE
			unsigned char SHWB_AutoEx		: 1; //Auto Ex for SHWB - SSLN Strat
			unsigned char UBS_Order			: 1; //True if Order is for A4/UBS Not EASE
			unsigned char QLAB_Order		: 1; //True if Order is for QLAB Not EASE

			unsigned char IsBd  : 1; //9	TD#3684 - OSOR Route - Sets TAG6122='b'
			unsigned char NeedStockLoanApproval : 1; //10
			unsigned char StockLoanApproved : 1; //11
			unsigned char IsBdWasSetByCamCode : 1; //12	true if the IsBd field was set using a CamCode

			unsigned char IgnoreSellPosiCheckWasTrue : 1; //13
			unsigned char RouteTagOrder	: 1; //14 true if we used Route Tags to create the order (tag=valuestring from frontend for EASE)
			unsigned char SendMsg736	: 1; //15 true if we need to send a 736 message to the trader DT4721 Bud 12/22/08
			unsigned char NotHeldOrder : 1; //16
		};
	};
	short CMTA;
	char Portfolio[20];

	unsigned short ShortAuthMethod;
	unsigned char ShortAuthResult;
	char CamList[24];// camcodes sent from fe

	union
	{
		unsigned char CalcAccStatBits;
		struct
		{
			bool Exempt_BP				: 1; //1 if true, we dont check BP and order does not affect BP
			bool Exempt_Exposure		: 1; //2 if true, we dont check Exposure and order does not affect Exposure
			bool Exempt_ShortLocate		: 1; //3 if true, Shorts are allowed - no need for stockloan
			bool RLLShareLimitOk		: 1; //4 if true, ShareLimit was validated by route level limits

			bool RLLValueLimitOk		: 1; //5 if true, ValueLimit was validated by route level limits
			unsigned char Busted		: 1; //6 true if this line is a bust  // DT#4941 - KMA - INTERNAL SPEEDUP - Capacity improvements require this
			unsigned char AutoReplaceOrder : 1; //15 - NYSE auto replace flag - so we dont have to test all the coditions all the time  // DT#4941 - KMA - INTERNAL SPEEDUP - Capacity improvements require this
			bool OutsideNBBO			: 1; // DT6088 Bud 10/21/09

		};
	};

	// + DT7549 DLA 20100910 - Pairs Trading
	union
	{
		unsigned short OrderBits;
		struct
		{
			unsigned char GBTOrder			: 1;	// Group/Binary/Tag Order flag
			unsigned char OptionPro			: 1;	// DT8462 Bud 02/23/11
			unsigned char WRKSpclHndling	: 1;	// DT9355 Bud 05/23/11
			unsigned char OVDSpclHndling	: 1;	// DT9355 Bud 05/23/11
			unsigned char SpareOrderBits	: 4;	// DT9355 Bud 05/23/11
			unsigned char SpareChar;
		};
	};
	// - DT7549 DLA 20100910 - Pairs Trading

	union// starts on boundary
	{
		unsigned char ShortLocateCodes;
		struct
		{
			unsigned char FeShortLocateSelected : 1; // True if the FRONTEND sent new ShortLocate CamCodes
			unsigned char ShortLocateMethod		: 7;    // Actual Short Locate method selected
								// 0=Nothing selected
								// 1=As normal (same as nothing)
								// 2=Local Account Locate (ie - must use Manual Locate screen)
								// 3=Auto Locate - if none available at the time of the SHORT, go find a locate
		};
	};
	char  RediOrderType;		// T-ticket
	float CommissionOverride;
	long  GroupOrderNo;			// assigned for CancelReplace and Grouped orders

	//BATCHORDERINFO BatchInfo;
	char ISIN[13];// redi
	char spare33[3];

	// FIX SERVER NEEDS TO POPULATE
	char ClientOriginalOrderId[20];		// used as RoutedOrderId in OATS

	long WriteTimeStamp;
	long CRDelta;			// dff for RVR
	char RegSho200ExemptReason;	// REGSHO Rule 200 reason 1=EXEMPT 2=G2 3=G3 and add others  here
	char TraderGiveUp[MMID_LEN];

	char TranTypeCode;				// Transaction type code - defaults to what is set on the account record
	unsigned short Strategy_ECN;	//Added to enumerate the renamed instances of Strategy Route (ie EASE -> GETS)
	char ProgramTradeStrategy[2];

	char Solicited;				// S=Solicited, U=Unsolicited, blank=unknown
	unsigned char PriceCurrencyCode;// Currency code in which the Price was give (as per FIX spec)
	long RouteAgreementId;		// Client Agreement ID used to OK this
	char RouteFundId[21];		// Fund Id attached to the route agreement above
	char eStopType;				// DT:1014.844 E-Stops

	char CustomCapacity;		// DT4792 - Market Maker Capacity - will be an index into a table that the trader uses to send out
	unsigned char Version;		// DT6090 bud 10/21/09

	long   CR_ChainShares;		// From Fix - current Shares ordered on chain
	long   CR_ChainExeShares;	// From Fix - current EXE Shares on chain
	double CR_ChainCost;		// From Fix - current Cost of chain

	long OrderReceivedDate;		// yyyymmdd
	long OrderReceivedTime;		// hhmmss

	union							// DT9181 bud 05/09/2011
	{
		char MemoLast3[4];

		struct
		{
			char FuturesCountry[2]; //Possible for country to be populated but no state, putting it first allows it to show up in BO.
			char FuturesState[2];
		};
	};								// end DT9181 bud 05/09/2011


	
	long MinQty;				// DT#5023 - MLALGO - Add Tag100 - MinQty support to CLServer

	char OptClass8[8];			// DT5676 - 20090626 - kma - OSI - OptClass[4] and OptClass8[8]  fields populated
	char ClearingAccount[ACCOUNT_LEN];	// Incoming field from the frontend, so that a trader can post trades to an account at export time.
	char AltAccount1[ACCOUNT_LEN];		// This is the Back Office "Clearing Account" (also known as sidecar) when it gets to Trader. See TD 4115

	union
	{
		char AltAccount2[ACCOUNT_LEN];		// Used by HV
		// expansion for EASE 3.6
		struct
		{
			long EASE_OnOpen;
			long EASE_OnClose;
			union
			{
				char EaseFlags;
				struct
				{
					bool EASE_OnClosePercentType_ORD	: 1; //1
					bool EASE_OnClosePercentType_ADV	: 1; //2
					bool EASE_OnClosePercentType_CLOSE	: 1; //3
					bool EASE_OnOpenSet					: 1; //4
					bool EASE_OnCloseSet				: 1; //5
					bool EASE_CompleteToday				: 1; //6
					bool EASE_OnOpenShares				: 1; //7 If true, EASE_OnOpen value is number of shares. If false, EASE_OnOpen is percentage (ex: 15=15%).
					bool EASE_OnCloseShares				: 1; //8 If true, EASE_OnClose value is number of shares. If false, EASE_OnClose is percentage (ex: 15=15%).
				};
			};
			char Liquidity2Bytes[2];// these are 2 chars, NO NULL TERMINATOR - SO BE CAREFULL
			char ProcessCode;			// DT4726 Bud 12/16/08
			long TotalOrderQuantity;	//DT6507 - kma - Issue on multiple cancel replaces on multiple days for GTC orders
										// this may include shares executed previous day - used to keep order shares for exhange gtc's

		};
	};

	long LmtPriceLeft;		// PRICE field, as a LONG - Left side of .
	long LmtPriceRight;		// PRICE field, as a LONG - Right side of .
		// Kevin's code to re-create the text version of price using left and right parts
		//		char CharPrice[100];
		//		int Negative=(BigTradeRec.TradeRecB.LmtPriceRight<0||BigTradeRec.TradeRecB.LmtPriceLeft<0);
		//		sprintf(CharPrice, "%s%d.%07d", Negative?"-":"", abs(BigTradeRec.TradeRecB.LmtPriceLeft), abs(BigTradeRec.TradeRecB.LmtPriceRight));
		// end Kevin's code to re-create the text version of price using left and right parts

} TRADERECB;





typedef struct tdOriginalBigTradeRec
{
	TRADEREC TradeRec;
	TRADERECA TradeRecA;

} ORIGINALBIGTRADEREC;

//NOTE" FIX server uses this struct in a message - it will need to be changed if this struct changes size

typedef struct tdBigTradeRec
{
	TRADEREC TradeRec;
	TRADERECA TradeRecA;
	TRADERECB TradeRecB;
} BIGTRADEREC,ORDERREC,* pORDERREC,*pBIGTRADEREC;


// 632 : Update Account Record
typedef struct tdMsg632Rec
{
	long SessionID;
	long MessageID;
	ACCOUNTREC AccountRec;
} MSG632REC;


// 950 : Forward Message to BACKUP clserver
typedef struct tdMsg950Rec
{
	long ReportLogNo;
	int Command;
	long FileType;
	long FileHandle;
	unsigned long DataOffset;
	int DataLen;
} MSG950REC;






#endif // _IQCLSERVER_H

