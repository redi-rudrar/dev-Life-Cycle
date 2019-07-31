#ifndef __DBCOMMON_H_
#define __DBCOMMON_H_

#include "sizes.h"
#include "Build.h"
//`#include "accset.h"


#define CURRENCY_USD 1			//USA
#define CURRENCY_CAD 2			//CANADIAN
#define CURRENCY_EUR 3			//Euro
#define CURRENCY_JPY 4			// Japanese Yen
#define CURRENCY_GBP 5			// British Pound
#define CURRENCY_CHF 6			// Swiss Franc
#define CURRENCY_AUD 7			// Australian Dollar
#define CURRENCY_HKD 8			// Hong Kong Dollar

#define CURRENCY_UNKNOWN 255	//Unknown - waiting for fix :)


// SYSTEM WIDE CAM CODES!!!!!
// Dont mess with TEXAS - and MORE SO - DONT REDEFINE THESE!!!
#define CAM_CMTA 1
#define CAM_PORTFOLIO 2
#define CAM_SHORTAUTH	3
#define CAM_PROGRAM		4
#define CAM_SHORTLOCATE 5
#define CAM_SOLUNSOLICITED	6
#define CAM_ANONYMOUS		7
#define CAM_TRANSTYPE_EQT	8 // Trnsaction Type Code for EQUITY	- BLUESHEETS
#define CAM_TRANSTYPE_OPT	9 // Trnsaction Type Code for OPTIONS	- BLUESHEETS
#define CAM_CMTA2 10
#define CAM_TRADER_ALT_ACC 11	// DT#1307 - Futures Account CAM Code  aka - Send frontend selected account to trader
#define CAM_NOT_HELD		12	// DT#2663 - oats Feb 4 2008 - not held - needed for VFIN route?

#define CAM_CMTA_VALUE_CMTA 0
#define CAM_CMTA_FILTER_ALLOATION4 1 // if ALLOCATE_MPID -> then use this to detemine whether we need to add to aggregate list for allocation

#define CAM_PORTFOLIO_VALUE_SD	0
#define CAM_PROCESS_CODE 1			// DT4726 Bud 12/16/08
#define CAM_SHORTAUTH_VALUE		0
#define CAM_PROGRAM_VALUE	0
#define CAM_SHORTLOCATE_VALUE 0
#define CAM_SOLUNSOLICITES_VALUE 0
#define CAM_SHORTLOCATE_METHOD 9
#define CAM_ANONYMOUS_VALUE	0
#define CAM_TRANSTYPE_OPT_VALUE	0 // Trnsaction Type Code for OPTIONS	- BLUESHEETS
#define CAM_TRANSTYPE_OPT_ISBD 	1 // IsBD value for OPTIONS

#define CAM_TRANSTYPE_EQT_VALUE	0 // Trnsaction Type Code for EQUITY	- BLUESHEETS
#define CAM_TRADER_ALT_ACC_VALUE 0 	// DT#1307 - Futures Account CAM Code  aka - Send frontend selected account to trader
#define CAM_NOT_HELD_VALUE 0 	// DT#2663 - oats Feb 4 2008 - not held - needed for VFIN route?

//REGSHO: Reg Sho Rule 200 reasons (short tick rule)

#define REGSHO200_REASON_GROUP1		1
#define REGSHO200_REASON_GROUP2		2
#define REGSHO200_REASON_GROUP3		3

#define REGSHO200_REASON_EXTERNAL	7

#define REGSHO200_REASON_NONEQTY	8	// Not an equity



//AON flags:

#define AON_ALL_OR_NONE			1
#define AON_IOC					2	// immediate or cancel
#define AON_ON_THE_OPEN			4
#define AON_ON_THE_CLOSE		8
#define AON_ISLD_SHOW_BOOK_ONLY 16
#define AON_FOK					32	// Fill or Kill
#define AON_NYSE_DIRECT			64	//040
#define AON_ARCA_NOW			128 // 0x80 immediate or cancel (IOC probably on as well so check this one first)


// Server capabilities flags used by BO to enable stuff
#define FEATURE_OPTION_EXERCISE		0x00001
#define FEATURE_TRADE_HEDGES		0x00002
#define FEATURE_OPTIONS_MAPPING		0x00004
#define FEATURE_TRADE_ALLOCATION	0x00008
#define FEATURE_AGREEMENTS			0x00010
#define FEATURE_OVERRIDE_OPENCLOSE	0x00020
#define FEATURE_ALLOCATE_ORDERS		0x00040 // true if we allocate by orders and not positions
#define FEATURE_ALLOCATE_ACCTYPE	0x00080 // true if we allocate with account types included
#define FEATURE_ACCOUNT_MONITOR		0x00100 // true if we allow 525 message tomoitor accounts and log into mulitple accounts for trading
#define FEATURE_DROPCOPY_ACCOUNT	0x00200 // true if we allow DropCopy by Account
#define FEATURE_FE_ALLOCATIONS		0x00400 // true if we allow frontend to allocate
#define FEATURE_OMS					0x00800 // true if we are configured to use an OMS
#define FEATURE_UBS					0x01000 // true if we have UBS stuff on - export ttype 19 - ABNPRIME
#define FEATURE_USER_CANNOT_PREFMM	0x02000 // true if we do not allow user to use PrefMM
#define FEATURE_SIDECAR_ACCOUNTS	0x04000 // true if we use ClreaingAccount/Sidecar
#define FEATURE_BOFA				0x08000 // true if we have UBS stuff on - export ttype 19 - ABNPRIME
#define FEATURE_OMS2				0x10000 // true if we have OMSv2 on this domain
#define FEATURE_FE_REPORTS			0x20000 // true if we allow frontend to export on this domain  // td 2969

#define FEATURE_ANY_OMS (FEATURE_OMS|FEATURE_OMS2)

#define FEATURE_ACCOUNT_ROUTES_MERGE 0x40000 // true if we have customized routes on this account
#define FEATURE_ROUTES_REPLACE		 0x80000 // true if FE must replace routes.txt

#define FEATURE_STOCKLOAN			 0x100000 // true if domain is set up for StockLoan System
#define FEATURE_MANUALTICKETS		 0x200000 // true if USER can do manual tickets (OATS)
#define FEATURE_SPREADTRADES		 0x400000 // true if CLServer can do spread trades
#define FEATURE_BOFA_ALLOCATIONS	 0x800000 // true if we are doing PB/ETS style allocation tool
#define FEATURE_INSTRUMENT_LIMITS	 0x1000000 // true if we are configured for futures limits
#define FEATURE_PRODUCTTYPEALLOCATIONS	0x2000000 // true if we are configured for futures
#define FEATURE_OPTRONICS			0x4000000 // true if we can enable OPTRONICS button on frontend
#define FEATURE_SECUREPASSWORDS		0x8000000 // true if we are enabled for secure passwords
#define FEATURE_PORTFOLIOROLLUP		0x10000000 // true if we are enabled for allocations rollup
#define FEATURE_ORDERSTAGING		0x20000000 // true if we are enabled for orderstaging DT4419 bud 11/24/2008

#define FEATURE_ACCOUNT_ROUTES		(FEATURE_ROUTES_REPLACE | FEATURE_ACCOUNT_ROUTES_MERGE)

#define SHORT_ACCOUNT 2
#define CASH_ACCOUNT 1
#define MARGIN_ACCOUNT 0

#define ACTION_BUY 1
#define ACTION_SELL 2
#define ACTION_SHORT 3

// Account actions - after trade plonked into correct account - used by allocaitons
#define AACTION_BUY				11
#define AACTION_SELL			12
#define AACTION_SELL_SHORT		13
#define AACTION_BUY_TO_CLOSE	14

typedef struct tdExSymbol
{
	char Exchange;
	char Region;
	char Symbol[SYMBOL_LEN];
	unsigned char CurrencyCode;
	char Reserved;

	operator const char*()
	{	static char s[SYMBOL_LEN+12];
		sprintf(s, "%s,%d,%c,%d", Symbol,Exchange,Region,CurrencyCode);
		return s;
	}

} EXSYMBOL;

typedef struct tdSymbol32
{
	char Exchange;
	char Region;
	char Symbol[32];
	unsigned char CurrencyCode;
	char Reserved;

	operator const char*()
	{	static char s[32+12];
		sprintf(s, "%s,%d,%c,%d", Symbol,Exchange,Region,CurrencyCode);
		return s;
	}

} SYMBOL32;

typedef struct tdUserAccEnts
{
	union u1{
		char TraderBits;
		struct s1
		{
			bool Trader  : 1;
			bool IgnorePrefMM : 1;
			bool tSpare3 : 1;
			bool tSpare4 : 1;
			bool tSpare5 : 1;
			bool tSpare6 : 1;
			bool tSpare7 : 1;
			bool tSpare8 : 1;
		};
	};
	union u2{
		char ViewTradingBits;
		struct s2
		{
			bool ViewTrading : 1;
			bool vSpare2 : 1;
			bool vSpare3 : 1;
			bool vSpare4 : 1;
			bool vSpare5 : 1;
			bool vSpare6 : 1;
			bool vSpare7 : 1;
			bool vSpare8 : 1;
		};
	};
	union u3{
		char ViewAccountBits;
		struct s3
		{
			bool ViewAccount: 1;
			bool OmsAssignTrades : 1;	// user is allowed to assign trades to an OMS order (after the fact)
			bool OmsConfirmOrders : 1;	// User allowed to CONFIRM new OMS orders
			bool OmsViewChildren : 1;	// User may view OMS Child orders
			bool OmsViewOrders : 1;		// User can see OMS orders in this account
			bool aSpare6 : 1;
			bool aSpare7 : 1;
			bool aSpare8 : 1;
		};
	};
	union u4{
		char AllocBits;
		struct s4
		{
			bool ViewAllocations : 1;
			bool CanAllocate	 : 1;
			bool CanSetCommision : 1;
			bool CanAllocateToAnyAcc : 1;
			bool lSpare5 : 1;
			bool lSpare6 : 1;
			bool lSpare7 : 1;
			bool CanOnlyAllocDefaults : 1;
		};
	};
}USERACCENTS;


typedef struct tdUserAccRec
{
	// IF WE EVER CHANGE THIS STRUCT YOU HAVE TO FIX ReadUsrAcc() for Remote User Creation
	// AND ALSO AddUAcIndkey() that keeps a copy of some of these fields
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	USERACCENTS UserAccEnts;
	EXSYMBOL Quotes[5];
} USERACCREC;

// Backoffice Entitlements (old style) bytes
#define  SEC_IDC_BOVIEW			0
//#define  SEC_ALLOCATEONLY		1			// DT 645 20061031 RM - pv removed 12/06/2006
#define  SEC_BOADDEDITUSER		2
#define  SEC_BOADDEDITON		3
#define  SEC_BOADDEDITINTRA		4
#define  SEC_BOADDEDITBOUSER	5
#define  SEC_BOMOVEORDER		6
#define  SEC_BODELETEORDER		7
#define  SEC_BOEDITORDERDETAIL	8
#define  SEC_BOADDEDITACC		9
#define  SEC_BOEDITCOMISSION	10
#define  SEC_BOSETLIMITS		11
#define  SEC_BOVIEWPOSITIONS 	12
#define  SEC_BO_OATS			13
#define  SEC_BOSETEQTYLIMITS	14
#define  SEC_BODELUSER			15
#define  SEC_BORESETDOMAINCLEARING 16		//td 3745
#define  SEC_STOCKLOAN_VIEW		17
#define  SEC_STOCKLOAN_EDIT		18
#define  SEC_STOCKLOAN_ADMIN	19

typedef struct tdUserRec
{
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char Password[PASSWORD_LEN];
	char LastName[NAME_LEN];
	char FirstName[NAME_LEN];
	char MiddleName[NAME_LEN];
	char Occupation[NAME_LEN];
	char Company[NAME_LEN];
	char SSNId[20];
	char Address1[NAME_LEN];
	char Address2[NAME_LEN];
	char Address3[NAME_LEN];
	char Address4[NAME_LEN];	// ctually encryption key - 1st char is '1' - '3' & next 24 is the key
	char City[20];
	char State[4];
	char Zip[11];
    union u1{
        char	Bitfields;        /* 1111/1111  */
        struct s1{
            bool       NonBillFlag			: 1;	//1 True if user shoudlt be included in user billing
            bool	   UserAgreementOK		: 1;	//2	True if the user agreements are all ok
            bool	   UserAgreementNeeded	: 1;	//3	True if we are waiting for an agreement
            bool	   ReplicatedUser		: 1;	//4	SHOULD NEVER APPEAR ON FILE - ONLY HERE TO MATCH FIELDS IN USRINDKEY
            bool	   UserDeletePending	: 1;	//5
            bool	   EmailSent			: 1;	//6
            bool	   UserSuspended		: 1;	//7 FUTURRE
            bool	   UserDeleted			: 1;	//8 FUTURE

        };

    };
	char Country[NAME_LEN];
	char EmailOld[NAME_LEN];
	char TelHome[TEL_LEN];
	char TelOffice[TEL_LEN];
	char Fax[TEL_LEN];
	char Cell[TEL_LEN];
	char Professional;
	char BORights[20];
	char Entitle[128];
	char Documents[128];

#if BUILD_NUMBER < 21170
	long StartDate[128][12];

#else

	long StartDate;

	char SuggestedVarsId[VARSID_LEN];
	long SuggestedVarsIdChangeDate;
	long SuggestedVarsIdChangeTime;
	char VarsId[VARSID_LEN];
	long VarsIdChangeDate;
	long VarsIdChangeTime;
	unsigned long CheckSum;

    union u2{
        long BORightsEx;        /* 1111/1111/1111/1111 */
        struct s2{
			bool	VarsIdAdmin:	 1;	// true if user can do VARSID Administration
			bool	VarsIdSuper:	 1;	// true if user can do VARSID Supervisor - can edit old vars
            bool	IQNetRoutesViewOnly	: 1;	// true if user has view-only for the routes dialogue : TD#5208
	        bool	BOAllocationsOnly	: 1;	// to replace the SEC_ALLOCATEONLY bit - pv 12/06/2006
			bool	BOViewOnly			: 1;	//
			bool	BOCanCancelOrders	: 1;	// True if this user can cancel orders from the BO
			bool	TechSupport			: 1;	// DT 1374: Tech Support will not be allowed to view personal information
			bool	IQDevUser			: 1;	// LL 20080124 DT2855 - add DEV user entitlement
			bool       spare9			: 1;
			bool	   spare10			: 1;
			bool	   spare11			: 1;	//
			bool	   spare12			: 1;	//
			bool	   spare13			: 1;	//
			bool	   spare14			: 1;	//
			bool	   spare15			: 1;	//
			bool	   spare16			: 1;	//
			bool	   spare17			: 1;	//
			bool	   spare18			: 1;	//
			bool	   spare19			: 1;	//
			bool	   spare20			: 1;	//
			bool	   spare21			: 1;	//
			bool	   spare22			: 1;	//
			bool	   spare23			: 1;	//
			bool	   spare24			: 1;	//
			bool	   spare25			: 1;	//
			bool	   spare26			: 1;	//
			bool	   spare27			: 1;	//
			bool	   spare28			: 1;	//
			bool	   spare29			: 1;	//
			bool	   spare30			: 1;	//
			bool	   spare31			: 1;	//
			bool	CanOnlyAllocDefaults: 1;	// true if user can only allocate to default accounts

        };
    };

	char Email[128];

	char PreviousVarsId[NAME_LEN];

    union u3{
        long	VarsIdBitFields;        /* 1111/1111  */
        struct s3{
			bool		VarsIdHasOwnField	: 1;	// True if vars id is no longer in company field
            bool		VarsIdEdit			: 1;	// True if vars id data doesnt match user record data
			bool		VarsIdActivated		: 1;	// Vars Id has been activated for this user
			bool		VarsIdForced		: 1;	// Vars Id activation was forced for this user
			bool		VarsIdChanged		: 1;	// Vars Id activation has been changed, but not activated
		};
    };
	char Citizenship1[2];		// ISO country code NOT null terminated
	char Citizenship2[2];

    union u4{
        long UserFlagsEx;        /* 1111/1111/1111/1111 */
        struct s4{
			bool	   IsInternal			: 1;	// LL 20080303 DT3159 - required for "Bank Info"
			bool	   HaveSavedRecord		: 1;	// LL 20080319 DT3248 - flag to indicate if user record has been previously saved
            bool	   MovedToNBK			: 1;	// LL 20080331 DT3248 - flag to indicate if user was already moved to NBK domain
	        bool	   UserChangedPasswd	: 1;	//	If the user has changed their passwd manually (as opposed to Automated system)
			bool	   EncryptedPasswd		: 1;	//	If the users passwd is encrypted DT4649 Bud 12/04/08
			bool	   spare6			: 1;	//
			bool	   spare7			: 1;	//
			bool	   spare8			: 1;	//
			bool       spare9			: 1;	//
			bool	   spare10			: 1;	//
			bool	   spare11			: 1;	//
			bool	   spare12			: 1;	//
			bool	   spare13			: 1;	//
			bool	   spare14			: 1;	//
			bool	   spare15			: 1;	//
			bool	   spare16			: 1;	//
			bool	   spare17			: 1;	//
			bool	   spare18			: 1;	//
			bool	   spare19			: 1;	//
			bool	   spare20			: 1;	//
			bool	   spare21			: 1;	//
			bool	   spare22			: 1;	//
			bool	   spare23			: 1;	//
			bool	   spare24			: 1;	//
			bool	   spare25			: 1;	//
			bool	   spare26			: 1;	//
			bool	   spare27			: 1;	//
			bool	   spare28			: 1;	//
			bool	   spare29			: 1;	//
			bool	   spare30			: 1;	//
			bool	   spare31			: 1;	//
			bool	   spare32			: 1;	//
        };
    };
//DT#2447 NS++  11/30/07
	long DisableTime;
	long DisableDate;
//DT#2447 NS--  11/30/07

	char NBKAlias[16];							// LL 20080303 DT3159 - "NBK alias" for "Bank Info"
	char SecretQuestion[80];					// LL 20080303 DT3159 - secret question for "Bank Info"
	char SecretAnswer[50];						// LL 20080303 DT3159 - answer to secret question for "Bank Info"

	// LL 20080318 DT3248 - add sponsor name, relationship, e-mail, and phone fields
	char SponsorName[NAME_LEN * 2 + 1];
	char SponsorRelationship[32];
	char SponsorEmail[128];
	char SponsorPhone[TEL_LEN];

	long expiredPasswd;			// Date at which this expires (future test)
	long failedLoginAttempts;	// number of failed login attempts)
	long lockedOutPasswd;		// NOT USED 

	long changedPasswd;
	long lastLoginDate;
	long lastLoginTime;

	long lastBuildNo;						// These may be removed in the future as DNS data
	unsigned long lastIP;
	unsigned long lastFeedIP;
	long lastProdType;
	char lastDomain[DOMAIN_LEN];
	char lastAccount[ACCOUNT_LEN];

	char BigSpace[5410]; //	StartDate[128][12];
#endif

	long EndDate[128][12];
} USERREC;

// Used to store user edit activities
typedef struct tUsrActivityRec
{
	long adate;
	long atime;

	char BOUser[USER_LEN];
	char Domain[DOMAIN_LEN];
	char User  [USER_LEN];

	char Action;
	char Item;
	char NewVal;
} USERACTIVITYREC;

typedef struct tdAccRouteRec
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char Route[ECN_LEN];
	char Agreement[AGREEMENT_LEN];	// AgreementShortName in DB    Client / ROute / Destination Agreement

    union u1{
        short BitFields;        /* 1111/1111/1111/1111 */
        struct s1{
				bool		Deleted			: 1;	// true if deleted
				bool		Spare2			: 1;
	            bool	   Spare3			: 1;
		        bool	   Spare4			: 1;
				bool	   spare5			: 1;	//
				bool	   spare6			: 1;	//
				bool	   spare7			: 1;	//
				bool	   spare8			: 1;	//
				bool       spare9			: 1;
				bool	   spare10			: 1;
				bool	   spare11			: 1;	//
				bool	   spare12			: 1;	//
				bool	   spare13			: 1;	//
				bool	   spare14			: 1;	//
				bool	   spare15			: 1;	//
				bool	   spare16			: 1;	//

        };
    };
	char sSpare[80];			 // Extra space
	char RouteSpare[4*1024]; // ROUTE structure will come in here

} ACCROUTEREC,  *pACCROUTEREC;


typedef struct tdAccHist
{
	double BuyTrades;
	double BuyQuantity;
	double BuyValue;
	double SellTrades;
	double SellQuantity;
	double SellValue;
	double ShortTrades;
	double ShortQuantity;
	double ShortValue;
	double InterestDebit;
	double InterestCredit;
	double CommissionsGross;
	double CommissionsStandard;
	double CommissionsNet;
}ACCHIST;

typedef struct tdPosiStat // to be deletted soon use POSISTATS
{
	double Cash;
	double CashPosi;
	double DayTradeBp;
	double OverNightBp;
	double LongPosi;
	double ShortPosi;
	double MaxBpUsed;
	double MarginEquity;
	double NonMarginEquity;
	double PersEquity;
}POSISTAT;

typedef struct tdAccStatRec
{
	char StartDate[DATE_LEN];
	POSISTAT StartPosiStat;
	POSISTAT CurrentPosiStat;
	double BpBuyStocks;
	double BpBuyOptions;
	double BpShortStocks;
	double BpSellNakedOptions;
	double TotalEquity;
    double LiquidationValue;
	double PrevClosedPl;
	double IntraDayClosedPl;
	double PrevStillOpenPl;
	double NewOpenedPl;
	double TotalExposure;
	float  PrevClosedPlMM;	  //
	float  PrevStillOpenPlMM; //Mark to the market

} ACCSTATREC;

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

typedef struct tdBuyingPower
{
	float BuyCash;
	float BuyStocks;
	float BuyOptions;
	float ShortStocks;
	float SellOptions;
	float SellCoverOpt;
	float SellNakedOpt;
} BUYINGPOWER;

typedef struct tdProfitLoss
{
	float PrevClosed;
	float IntraDayClosed;
	float PrevStillOpen;
	float NewOpened;
	float MarkToMarket;
	float StartYtoD;
	float StartMtoD;
} PROFITLOSS;

/*typedef struct tdAccStatsRec
{
	POSISTATS PensonOpenDeleted;
	POSISTATS CalcOpenDeleted;
	float CalcOpenErrDeleted[9];
	POSISTATS CalcCurrentDeleted;
	BUYINGPOWER CalcBuyingPowerDeteled;
	PROFITLOSS CalcProfitLossDeleted;
	float OtherMarketValueDeleted;
	float OtherMaintRequiredDeleted;
	float OtherMarketExposureDeleted;
	char CalcProfitsDeleted; //1=Bid/Ask 2=Last;
} ACCSTATSREC;
*/
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

    union u1{
        char  CanTradeBits;        /* 1111/1111 */
        struct s1{
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
    union u2{
        char  BpBits;        /* 1111/1111 */
        struct s2{
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
    union u3{
        char  Bits;        /* 1111/1111 */
        struct s3{
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

    union u4{
        short Bitfields;        /* 1111/1111 /1111/1111 */
        struct s4{
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
    union u5{
        char  MoreBits;        /* 1111/1111 */
        struct s5{
            bool	AllowTranTypeOverride	: 1;	// Allow Transaction Type override (from FIX or FE)
            bool	AllowSolicitedOverride	: 1;	// Allow Solicited flag override (from FIX or FE)
			bool	OMSTradingAccount		: 1;
			bool	UseIntraDayExposureOnly : 1;	// DT#41 intraday total exposure calculation
			bool	DoNotForceReloadOnBoEdtit : 1;	// DT#SWIMFAST
        };

	};
	char SolicitedFlag;				// S=Solicited, U=Unsolicited, blank=unknown
	char Spare[2400];					/// NOT IN DB

	char oats_AccountTypeCode;
	char oats_MemberTypeCode;
	char oats_FirmMPID[5];

    union u6{
        char  Canadians;        /* 1111/1111 */
        struct s6{
			bool	BprBuyCanadianStockYn	:1;
			bool	BprShortCanadianStockYn : 1 ;
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
	union u1
	{
		char Aon;					// bit0=AON bit1=IOC/ArcaNow bit2=OnOpen bit3=OnClose bit4=IsldBookOnly bit5=FOK bit6=NyseDirect
		struct s1
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
    union u2{
        char	MoreBits1;    // 1111/1111
        struct s2{
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
	char OptClass[4];			// 3 Character Option Class if this a option
	long ClosedSharesNotAgainstTheBox;	// Number od shares for calc pnl in FE
	double AvgOpenPrice;		// CALCULATED:Average Price for OpenShares
	double M2MPrice;			// Price Of this stock at yesterdays close if this is a overnight position KMAKMA NEVER USED IN CLS
	double CloseCost;			// CALCULATED:Total Income from Closing the order
	long   OptExpireDate;		// Option Expiration Date
	float DisplayValue;			//Peg Diff
	double AvgClosePrice;		// CALCULATED:Average Closing Price for this Order
	float CurrentStopPrice;		// Current Trigger Price for Trailing Stops
	char OptRootExchange;	 	// Exchange Code for Option Root Symbol
	char OptRootRegion;			// Region Code for Option Root Symbol
	char OptType;				// Put='P' Call='C'
    union u3{
        char	Bitfields;        /* 1111/1111  */
        struct s3{
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
	union u4{
        char PegOrder;        /* 1111/1111 */
        struct s4{
            unsigned char    PegMkt : 1; //1 Peg-Mkt
            unsigned char    PegBest : 1; //2 Peg-Best
            unsigned char    PegLast : 1; //3 Peg-Last
            unsigned char    PegMid : 1; //4 Peg-Mid
            unsigned char    PegPrime : 1; //5 Peg-Prime
            unsigned char    OT_6 : 1; //6 Peg-Win REMOVED
            unsigned char    OT_7 : 1; //7 OrderType PEGS ONLY
            unsigned char    OT_8 : 1; //8 OrderType PEGS ONLY
		};
	};
	union u5{
        char	OrderTypeBits[3];        /*1111/1111 1111/1111 1111/1111 */
        struct s5{
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
			bool			   zSpareBit1 : 1;
			bool			   zSpareBit2 : 1;
			bool			   zSpareBit3 : 1;
			bool			   zSpareBit4 : 1;

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

    union u6{
        char	OrderOrigin;        /* 1111/1111  */
        struct s6{
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
    union u7{
        char	MorebitFields;        /* 1111/1111  */
        struct s7{
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


typedef struct tdBTR //Master Record for all Trades with Sym32
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
	union u1
	{
		char Aon;					// bit0=AON bit1=IOC/ArcaNow bit2=OnOpen bit3=OnClose bit4=IsldBookOnly bit5=FOK bit6=NyseDirect
		struct s1
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
    union u2{
        char	MoreBits1;    // 1111/1111
        struct s2{
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
	char OptClass[4];			// 3 Character Option Class if this a option
	long ClosedSharesNotAgainstTheBox;	// Number od shares for calc pnl in FE
	double AvgOpenPrice;		// CALCULATED:Average Price for OpenShares
	double M2MPrice;			// Price Of this stock at yesterdays close if this is a overnight position KMAKMA NEVER USED IN CLS
	double CloseCost;			// CALCULATED:Total Income from Closing the order
	long   OptExpireDate;		// Option Expiration Date
	float DisplayValue;			//Peg Diff
	double AvgClosePrice;		// CALCULATED:Average Closing Price for this Order
	float CurrentStopPrice;		// Current Trigger Price for Trailing Stops
	char OptRootExchange;	 	// Exchange Code for Option Root Symbol
	char OptRootRegion;			// Region Code for Option Root Symbol
	char OptType;				// Put='P' Call='C'
    union u3{
        char	Bitfields;        /* 1111/1111  */
        struct s3{
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
	union u4{
        char PegOrder;        /* 1111/1111 */
        struct s4{
            unsigned char    PegMkt : 1; //1 Peg-Mkt
            unsigned char    PegBest : 1; //2 Peg-Best
            unsigned char    PegLast : 1; //3 Peg-Last
            unsigned char    PegMid : 1; //4 Peg-Mid
            unsigned char    PegPrime : 1; //5 Peg-Prime
            unsigned char    OT_6 : 1; //6 Peg-Win REMOVED
            unsigned char    OT_7 : 1; //7 OrderType PEGS ONLY
            unsigned char    OT_8 : 1; //8 OrderType PEGS ONLY
		};
	};
	union u5{
        char	OrderTypeBits[3];        /*1111/1111 1111/1111 1111/1111 */
        struct s5{
            unsigned char      PegWithNoLimit	: 1;	//9 For Mkt Pegs
            unsigned char	   Peg_Display_Type	: 5;	//10 For Type of Display Pegging
            unsigned char	   Disc_Sign		: 1;	//13 0 Pos, 1 Neg
            unsigned char	   EcnRandomReserveRangeSet: 1;	// true means EcnRandomReserveRange value is to be used (FE sends it in using ExeShares)

			bool			   BofaExecutionServicesOrder : 1; // true if this order is on either side of a BOFA execusion services route
			bool			   BofaExecutionServicesDomain : 1; // true if this order came into a BOFA Execution Services domain
			bool			   HideMMID	 : 1;
			bool			   SpareBit4 : 1;
            //unsigned char EASE_TargetStrat_deleted		: 4;	//16EASE Target Strategy


//            unsigned char EASE_ExecMode_deleted		: 4;	//20EASE Execution Mode
			bool			   zSpareBit1 : 1;
			bool			   zSpareBit2 : 1;
			bool			   zSpareBit3 : 1;
			bool			   zSpareBit4 : 1;

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

    union u6{
        char	OrderOrigin;        /* 1111/1111  */
        struct s6{
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
    union u7{
        char	MorebitFields;        /* 1111/1111  */
        struct s7{
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
	SYMBOL32 Sym32;
} BTR;


#define ORDER_NORMAL 0
#define ORDER_MASTER 1
#define ORDER_SLAVE 2

typedef struct tdTradeRecA
{
	char Memo[23];
	char OrderType; //ORDER_TYPE_DUMPALL, STD, etc
//	int  SpareInt;

	short OriginalOrderDate;		// stored as Number of days since 01/01/2000
	unsigned short OriginalOrderNumber;		// atol of order

	short FIXLineId;
	unsigned short EcnRandomReserveRange; // 0-50000 random reserve range

	char CancelMemo[23];
//	char SpareChar;
    union u1{
        char	BitFieldsA;        /* 1111/1111  */
        struct s1{
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

	long StepRefNo;		// Step on execution path

	char Notes[8];			// used for Short Sale Request Id
	char ExecutingBrokerCode[7];

//	long WorkOrderRefNoX;
//	char Spare[3];

	char LinkOrderNo[ORDERNO_LEN - sizeof(long)];

#pragma pack(push,1)
	long ComplexSharesRatio;							// Shares/complex order for this leg
#pragma pack(pop)
    union u2{
        char	BitFieldsB;        /* 1111/1111  */
        struct s2{
            bool       T13Checked	: 1;	//1 True means T13 list was checked
            bool	   OatsNonDirectedOrder	: 1;	//2	true if order is deliberately set to NON-DIRECTED by user
            bool	   OatsManualTicket	: 1;	//3	True if Order has manual received date /time
            bool	   NotForAllocation	: 1;	//4	if true - this trade must not make it into allocation/aggregate maps
            bool	   AutoAllocate	: 1;	//5 if true - Allocate this order to account in BigtradeRec...AltAccount2
            bool	   FlipOnExport	: 1;	//6 use altaccount for export
            bool	   b7	: 1;	//7
            bool	   b8	: 1;	//8

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

	union u1// starts on boundary
	{
		unsigned char Bits2;
		struct s1
		{
			unsigned char aoAccountHasIgnoreThresholdListSet	: 1; // True is the account setting is on to ignore Threshold Files
			unsigned char aoDomainIgnoresThresholdOnShortLocates : 1; //10
			unsigned char abit3 : 1; //11
			unsigned char abit4 : 1; //12
			bool ComplexOrder		: 1;			// If this order is part of a complex order
			bool ComplexOrderParent	: 1;			// If this order is the parent of leg orders
			bool OrderStagingOrder	: 1;			//15 If this order is associated with Order Staging
			bool OrderStagingParent	: 1;			//16 If this order is an Order Staging Parent DTYYYY bud 10/27/08
		};
	};


	union u2// starts on boundary
	{
		char TRBBits[2];
		struct s2
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

	union u3
	{
		unsigned char CalcAccStatBits;
		struct s3
		{
			bool Exempt_BP				: 1; //1 if true, we dont check BP and order does not affect BP
			bool Exempt_Exposure		: 1; //2 if true, we dont check Exposure and order does not affect Exposure
			bool Exempt_ShortLocate		: 1; //3 if true, Shorts are allowed - no need for stockloan
			bool RLLShareLimitOk		: 1; //4 if true, ShareLimit was validated by route level limits

			bool RLLValueLimitOk		: 1; //5 if true, ValueLimit was validated by route level limits
			unsigned char Bit6 : 1; //6
			unsigned char Bit7 : 1; //7
			unsigned char Bit8 : 1; //8

		};
	};

	unsigned short PrevOrderNumber;		// atol of order
	union u4// starts on boundary
	{
		unsigned char ShortLocateCodes;
		struct s4
		{
			unsigned char FeShortLocateSelected : 1; // True if the FRONTEND sent new ShortLocate CamCodes
			unsigned char ShortLocateMethod		: 7;    // Actual Short Locate method selected
								// 0=Nothing selected
								// 1=As normal (same as nothing)
								// 2=Local Account Locate (ie - must use Manual Locate screen)
								// 3=Auto Locate - if none available at the time of the SHORT, go find a locate
		};
	};
	char  LiquidityDELETED;			// as reported from ecn
	float CommissionOverride;
	long  GroupOrderNo;			// assigned for CancelReplace and Grouped orders

	BATCHORDERINFO BatchInfo;

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
	char BLANK[2];				// adjusted from 144 due to adding above

	long   CR_ChainShares;		// From Fix - current Shares ordered on chain
	long   CR_ChainExeShares;	// From Fix - current EXE Shares on chain
	double CR_ChainCost;		// From Fix - current Cost of chain

	long OrderReceivedDate;		// yyyymmdd
	long OrderReceivedTime;		// hhmmss

	char ClearingDomain[DOMAIN_LEN];				// adjusted from 144 due to adding above
	char ClearingAccount[ACCOUNT_LEN];	// Incoming field from the frontend, so that a trader can post trades to an account at export time.
	char AltAccount1[ACCOUNT_LEN];		// This is the Back Office "Clearing Account" (also known as sidecar) when it gets to Trader. See TD 4115

	union u5
	{
		char AltAccount2[ACCOUNT_LEN];		// Used by HV
		// expansion for EASE 3.6
		struct s5
		{
			long EASE_OnOpen;
			long EASE_OnClose;
			union u6
			{
				char EaseFlags;
				struct s6
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
			long SpareWord;

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
//NOTE" FIX server uses this struct in a message - it will need to be changed if this struct changes size

/***
typedef struct tdMiniTradeRec
{
	// Start of Fields to Place Order
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char AccType;
	EXSYMBOL ExSymbol;
	char LastStatus;		// See defines following
	char PrefECN[ECN_LEN];
	char PrefMMID[ECN_LEN];
	char OrderNo[ORDERNO_LEN];
	char ECNOrderNo[ORDERNO_LEN];
	char PrefMMYN;       // Prefrence a Mkt Maker
	char ECNExecID[20];
	char ECNId[ECN_LEN];
	char MMId[ECN_LEN];
	long ExeShares;
	long TradeDetRef;
	long LastDate;
	char LastTime[9];
	char FileDate[9];
	char CancelRequested;		// Set if User has selected atleast one Cancel / Used to detect unsolisited Cancels
	long ItemDate;
	float Comms;
	float EcnFee;

} *pMINITRADEREC, MINITRADEREC;
**/

#define WO_EXP_DAY		0
#define WO_EXP_DAYPLUS  1
#define WO_EXP_GTC		2

typedef unsigned long WORKORDERREFNO;

typedef struct tdWorkOrderRec
{
	WORKORDERREFNO WorkOrderRefNo;
	WORKORDERREFNO ParentWorkOrderRefNo;
	char WorkUser[USER_LEN];
	char WorkDomain[DOMAIN_LEN];
	char EnterUser[USER_LEN];
	char EnterDomain[DOMAIN_LEN];
	char TradeAccount[ACCOUNT_LEN];
	char TradeDomain[DOMAIN_LEN];
	EXSYMBOL ExSymbol;
	long Shares;				// Number of Shares for This Order
	long ChildShares;			// Number of Shares for Child Orders
	long TotShares;				// Total of Shares for This Order plus Child Orders
	long Pending;				// Number of Shares Pending for This Order
	long ChildPending;			// Number of Shares Pending for Child Orders
	long TotPending;			// Number of Shares Pending for Order plus Child Orders
	long OnOrder;				// Number of Shares OnOrder for This Order
	long ChildOnOrder;			// Number of Shares OnOrder for Child Order
	long TotOnOrder;			// Number of Shares OnOrder for This Order plus Child Orders
	long TotLeaves;				// Number of Shares left to Order
	long Executed;				// Number of Shares Executed for This Order
	long ChildExecuted;			// Number of Shares Executed for Child Orders
	long TotExecuted;			// Number of Shares Executed for This Order plus Child Orders
	unsigned long MemoRefno;	// Offset of Memo in memofile
	float Price;				// Price for Order
	float PriceLimit;			// Price Limit for Order
	float AvgPrice;				// Price for Order Child Orders
	float ChildAvgPrice;		// Price for Order This Order plus Child Orders
	float TotAvgPrice;				// Price for Order This Order
	char Action;				// 1:buy, 2:Sell, 3:Short, 4:Exercise Option
	char OrderName[11];			// Name if List
	char ShortSellCode[8];
	union u1
	{
		char ExpirationFlags[2];
		struct s1
		{
			unsigned char	Exp	: 2;	// 0=day,1=dayplus, 2=gtc
			bool    sp2			: 1;	// Extended  trading
			bool    bOverrideCommission : 1;		//
			unsigned char    CommissionType : 2;		// 00=fixed, 1=Per Share, 2=Percentage, 3=unused
			bool    sp7 : 1;
			bool    sp8 : 1;

			bool    sp9  : 1;
			bool    sp10 : 1;
			bool    sp11 : 1;
			bool    sp12 : 1;
			bool    sp13 : 1;
			bool    sp14 : 1;
			bool    sp15 : 1;
			bool    sp16 : 1;
		};
	};
	union u2
	{
		char SomeBits;
		struct s2
		{
			bool	PrefMMYN	: 1;	// Set to 1 if Market Maker was preferenced
			bool    bsp2 : 1;
			bool    bsp3 : 1;
			bool    bsp4 : 1;
			bool    bsp5 : 1;
			bool    bsp6 : 1;
			bool    bsp7 : 1;
			bool    bsp8 : 1;
		};
	};
	char SpareFlags[2];


	/// date and time stamp for audit trail
	long DateStamp;
	long TimeStamp;
	float PriceStamp;
	short AuditId;
	char OhSo2Spare[2];
	/// end of audit trail fields

	char PrefECN[ECN_LEN];		// Route selected by User in Front End
	char PrefMMID[ECN_LEN];		// Prefrenced Market Marker / Option Exchange

	char Instructions[20];
	char Notes[20];
	char OrderTypeString[20];

	char UserGroup[USER_LEN];
	float Commission;
	char OhSoMuchSpare[360];


	float Cost;					// Cost for Order Child Orders
	float ChildCost;			// Cost for Order This Order plus Child Orders
	float TotCost;				// Cost for Order This Order

	char Deleted;				// Disk Deleted Marker
	char OhSo3Spare[3];
		long CommitMarker; //DISK COMMIT END DO NOT MOVE
	long TreeItem;				// Pointer to a temporary key indetifying tree relation
	unsigned char Generation;	// TreeGeneration level.
	unsigned char GenLines[8];  // Bit map used to show what acentres have children below this item
	unsigned long LastChild;	// Used to Test if the childbrach currently inspected has to paint lines for other children
	unsigned long FamilySize;	// Say how many OffSpring
}WORKORDERREC,*pWORKORDERREC;


/*
typedef struct tdWorkOrderRec
{
	WORKORDERREFNO WorkOrderRefNo;
	WORKORDERREFNO ParentWorkOrderRefNo;
	char WorkUser[USER_LEN];
	char WorkDomain[DOMAIN_LEN];
	char EnterUser[USER_LEN];
	char EnterDomain[DOMAIN_LEN];
	char TradeAccount[ACCOUNT_LEN];
	char TradeDomain[DOMAIN_LEN];
	EXSYMBOL ExSymbol;
	long Shares;				// Number of Shares for This Order
	long ChildShares;			// Number of Shares for Child Orders
	long TotShares;				// Total of Shares for This Order plus Child Orders
	long Pending;				// Number of Shares Pending for This Order
	long ChildPending;			// Number of Shares Pending for Child Orders
	long TotPending;			// Number of Shares Pending for Order plus Child Orders
	long OnOrder;				// Number of Shares OnOrder for This Order
	long ChildOnOrder;			// Number of Shares OnOrder for Child Order
	long TotOnOrder;			// Number of Shares OnOrder for This Order plus Child Orders
	long TotLeaves;				// Number of Shares left to Order
	long Executed;				// Number of Shares Executed for This Order
	long ChildExecuted;			// Number of Shares Executed for Child Orders
	long TotExecuted;			// Number of Shares Executed for This Order plus Child Orders
	unsigned long MemoRefno;	// Offset of Memo in memofile
	float Price;				// Price for Order
	float PriceLimit;			// Price Limit for Order
	float AvgPrice;				// Price for Order Child Orders
	float ChildAvgPrice;		// Price for Order This Order plus Child Orders
	float TotAvgPrice;				// Price for Order This Order
	char Action;				// 1:buy, 2:Sell, 3:Short, 4:Exercise Option
	char OrderName[11];			// Name if List
	char ShortSellCode[8];
	union
	{
		char ExpirationFlags;
		struct
		{
			char	Exp			: 2;	// 0=day,1=dayplus, 2=gtc
			bool    sp2			: 1;	// Extended  trading
			bool    sp3 : 1;		//
			bool    sp4 : 1;
			bool    sp5 : 1;
			bool    sp6 : 1;
			bool    sp7 : 1;
			bool    sp8 : 1;

		};
	};
	char SpareFlags[3];


	/// date and time stamp for audit trail
	long DateStamp;
	long TimeStamp;
	float PriceStamp;
	short AuditId;
	char OhSo2Spare[2];
	/// end of audit trail fields

	char OhSoMuchSpare[456];

	float Cost;					// Cost for Order Child Orders
	float ChildCost;			// Cost for Order This Order plus Child Orders
	float TotCost;				// Cost for Order This Order

	char Deleted;				// Disk Deleted Marker
	char OhSo3Spare[3];
		long CommitMarker; //DISK COMMIT END DO NOT MOVE
	long TreeItem;				// Pointer to a temporary key indetifying tree relation
	unsigned char Generation;	// TreeGeneration level.
	unsigned char GenLines[8];  // Bit map used to show what acentres have children below this item
	unsigned long LastChild;	// Used to Test if the childbrach currently inspected has to paint lines for other children
	unsigned long FamilySize;	// Say how many OffSpring
}WORKORDERREC,*pWORKORDERREC;
*/

/*typedef struct tdTradeRec //Master Record for all Trades
{
	// Start of Fields to Place Order
	char User[USER_LEN];		// Name of User that places the order
	char Account[ACCOUNT_LEN];	// Account order was placed in
	char Domain[DOMAIN_LEN];	// Domain that Account belongs too
	EXSYMBOL ExSymbol;			// Symbol name including Exchange and Region
	char PrefECN[ECN_LEN];		// Route selected by User in Front End
	char PrefMMID[ECN_LEN];		// Prefrenced Market Marker / Option Exchange
	char OrderNo[ORDERNO_LEN];	// Internal OrderNo assigned to Order YYYYMMDD-XXXX
	long Shares;				// Number of Shares for Order
	long ReservedShares;		// Number of Shares To Show OR Set to (-1) for Invisible Order on ISLD
	WORD Format;				// Format Code (1-256=Fraction=1/Format) OR (>512= Desimal with 512=0Decimals 1024=1Decimal...)
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
	char Action;				// 1:buy, 2:Sell, 3:Short
	char Type;					// 1:Market, 2:Limit
	char Gtc;					// True / False
	char Ext;					// True / False
	char LastStatus;			// See defines following at the end of this structure
	char Aon;					// bit0=AON bit1=IOC/ArcaNow bit2=OnOpen bit3=OnClose bit4=IsldBookOnly bit5=FOK bit6=NyseDirect
	char PrefMMYN;				// Set to 1 if Market Maker was preferenced
	char StopType;				// 1:Stop // 2: Traling Stop
	char OrigAction;			// Original Action Selected by user, this could differ from Action when system change Short to Sell
	char CancelCount;			// Number of times the User requested a Cancel
	char LastDate[9];			// Date of LastStatus Change
	char LastTime[9];			// Time of LastStatus Change
	char ECNOrderNo[ORDERNO_LEN];  // OrderNo assigned for ECN
	char ECNExecID[20];			// Execution ID assigned by ECN for a specific fill
	char ECNId[ECN_LEN];		// Actual Route Used will be equal to PrefECN unless PrefECN="Default" and the system assigned a route
	char MMId[ECN_LEN];			// Actual Marker Maker / Contra Account
	char CancelRequested;		// Set if User has selected atleast one Cancel / Used to detect unsolisited Cancels
	char PosibleDupp;			// Set if ECN Playback indicate the this OrderStatus is a posible Dupplicate
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
#if BUILD_NUMBER <= 1004
	double OpenComs;			// re-used
#else
	char OptClass[4];			// 3 Character Option Class if this a option
	float LiquidityFee;			// Spare
#endif
	double AvgOpenPrice;		// CALCULATED:Average Price for OpenShares
	double M2MPrice;			// Price Of this stock at yesterdays close if this is a overnight position
	double CloseCost;			// CALCULATED:Total Income from Closing the order
#if BUILD_NUMBER <= 1004
	long   HedgeRequired;		// re-used
#else
	long   OptExpireDate;		// Option Expiration Date
#endif
	long   dbSeqNo;				// Record No if the backup ACCESS Database
	double AvgClosePrice;		// CALCULATED:Average Closing Price for this Order
	float CurrentStopPrice;		// Current Trigger Price for Trailing Stops
	char OptRootExchange;		// Exchange Code for Option Root Symbol
	char OptRootRegion;			// Region Code for Option Root Symbol
	char OptType;				// Put='P' Call='C'
//har DoNotClear;
    union {
        char	Bitfields;        // 1111/1111
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
	long GtcDate;				// Last Date for Good Till Cancel Orders
	long TradeDetRef;			// Database unique reference number for this record
	char OptRootSymbol[8];		// Option Root Symbol
	double CloseProfit;			// CALCULATED:Profit made from closing this order
#if BUILD_NUMBER <= 1001
	double DAFCharge;			// re-used
#else
	float ECNFee;				// Fee Charged by ECN
	float AgentComms;			// Commisions payable to Agent
#endif
	char	FileDate[9];		// Date of the Record
} TRADEREC;
*/

/*typedef struct tdBigTradeRec //Master Record for all Trades
{
	TRADEREC TradeRec;
	long NewData;
} BIGTRADEREC;
*/
// Last StatusCode;
#define TRADE_STOPPED		(-8) 	// Order stopped waiting on fill
#define WILL_LOCK			(-7) 	// Order will Lock Market
#define CANCEL_SENT			(-6) 	// Cancel Sent to server
#define CANCEL_RECEIVED		(-5) 	// Cancel Received to server
#define CANCEL_PLACED		(-4) 	// Cancel Placed at ECN
#define TRADE_SENT			(-3) 	// Sent to server
#define TRADE_RECEIVED		(-2)	// Received by server
#define TRADE_PLACED		(-1)	// Placed at Ecn
#define TRADE_CONFIRMED		0		// Confirmed by ECN
#define TRADE_PARTFILL		1		// Part filled
#define TRADE_FILLED		2		// Full Filled
#define TRADE_EXPIRED		3		// Expired - Done for the day
#define TRADE_CANCELED		4		// Full Canceled
									// 5:Replaced,
#define  TRADE_CANCELPENDING 6		// 6:Pending Cancel/Replace

						// 7:Stopped
#define TRADE_REJECTED		8	//8:Rejected
						// 9:Suspended, A:Pending New
						// B:Calculated, C:Expired
						// 100 : Cancel Rejected by ECN
						// 101 : Cancel Rejected by PARSER - OVER CANCEL
						// 102 : Cancel Rejected by PARSER - ORDER NOT CONFIRMED
						// 103 : Cancel Rejected by PARSER - ORDER NOT FOUND
						// 104 : Cancel Rejected by ECN - Cancel/Replace not allowed DT3347

#define P_User					1
#define P_Account				2
#define P_Domain				3
#define P_ExSymbol				4
#define P_PrefECN				5
#define P_PrefMMID				6
#define P_OrderNo				7
#define P_Shares				8
#define P_ReservedShares		9
#define P_Format				10
#define P_Tick					11
#define P_ClosePosition			12
#define P_Price					13
#define P_StopAmount			14
#define P_OptDayRootPrice		15
#define P_DiscretionaryAmount	16
#define P_OptLockRootPrice		17
#define P_OptStrikePrice		18
#define P_OptConversion			19
#define P_AccType_CancelCount	20
#define P_LastDate				21
#define P_LastTime				22
#define P_ECNOrderNo			23
#define P_ECNExecID				24
#define P_ECNId					25
#define P_MMId					26
#define P_CancelRequested		27
#define P_PosibleDupp			28
#define P_Pending				29
#define P_ExeShares				30
#define P_LastShares			31
#define P_Canceled				32
#define P_OpenShares			33
#define P_PrevOpenShares		34
#define P_ClosedShares			35
#define P_AvgPrice				36
#define P_LastPrice				37
#define P_Coms					38
#define P_OpenCost				39
#define P_OpenComs				40
#define P_AvgOpenPrice			41
#define P_M2MPrice				42
#define P_CloseCost				43
#define P_HedgeRequired			44
#define P_AvgClosePrice			45
#define P_CurrentStopPrice		46
#define P_OptRootExchange		47
#define P_OptRootRegion			48
#define P_OptType				49
#define P_DoNotClear			50
#define P_GtcDate				51
#define P_TradeDetRef			52
#define P_OptRootSymbol			53
#define P_CloseProfit			54
#define P_DAFCharge				55
#define P_FileDate				56
#define P_DbSeqNo				57
#define P_EcnFee				58
#define P_OptExpireDate			59
#define P_OptClass				60
//#define P_AON					61
#define P_Origin				62
#define P_MorebitFields			63
#define P_OrderTypeBits			64
#define P_PegOrder				65
#define P_DisplayValue			66
#define P_WorkOrderRefNo		67
#define P_Memo					68
#define P_CancelMemo			69
#define P_ClientOriginalOrderId 70
#define P_RegShoExempt200		71
#define P_CurrencyCode			72
#define P_TransTypeCode			73
#define P_EStopType				74


typedef struct tdOrderDetPkt
{
	int  LastStatus;
	long Pending;
	long LastShares;
	long Canceled;
	long Shares;
	long Format;
	double LastPrice;
	float Price;
	char MMId[ECN_LEN];
	char Symbol[SYMBOL_LEN];
	char OrderNo[ORDERNO_LEN];
	char LastTime[9];
	char Action;
	char Type;
} ORDERDETPKT;

typedef struct tdOrderDetPkt_1
{
	int  LastStatus;
	long Pending;
	long LastShares;
	long Canceled;
	long Shares;
	long Format;
	double LastPrice;
	float Price;
	char MMId[ECN_LEN];
	char Symbol[SYMBOL_LEN];
	char OrderNo[ORDERNO_LEN];
	char LastTime[9];
	char Action;
	char Type;
	char CancelRequested;
	char CancelCount;
} ORDERDETPKT_1;

typedef struct tdOrderDetPkt_2
{
	int LastStatus;
	long Pending;
	long LastShares;
	long Canceled;
	long Shares;
	long Format;
	double LastPrice;
	float Price;
	char MMId[ECN_LEN];
	char Symbol[SYMBOL_LEN];
	char OrderNo[ORDERNO_LEN];
	char LastTime[9];
	char tmp;
	char sp[4];
	char Action;
	char Type;
	char CancelRequested;
	char CancelCount;
	char AccType;
	char unused[3];
	long ClientRef;
	long SpareLong;
	char User[16];

} ORDERDETPKT_2;

typedef struct tdOrderDetPkt_3
{
	int LastStatus;
	long Pending;
	long LastShares;
	long Canceled;
	long Shares;
	long Format;
	double LastPrice;
	float Price;
	char MMId[ECN_LEN];
	char Contra[ECN_LEN];
	char Symbol[SYMBOL_LEN];
	char OrderNo[ORDERNO_LEN];
	char LastTime[9];
	char Tmp1;
    union u1{
        short BitFields;        /* 1111/1111/1111/1111 */
        struct s1{
            unsigned char  MasterSlave		: 2;
	            bool	   OverrideOpenClose: 1;
		        bool	   ClosingTrade		: 1;
				bool	   spare5			: 1;	//
				bool	   spare6			: 1;	//
				bool	   spare7			: 1;	//
				bool	   spare8			: 1;	//
				bool       spare9			: 1;
				bool	   spare10			: 1;
				bool	   spare11			: 1;	//
				bool	   spare12			: 1;	//
				bool	   spare13			: 1;	//
				bool	   spare14			: 1;	//
				bool	   spare15			: 1;	//
				bool	   spare16			: 1;	//

        };
    };
	char Exchange;
	char Region;
	char Action;
	char Type;
	char CancelRequested;
	char CancelCount;
	char AccType;
	char CurrencyCode;
	char StopType;			// KMA added to be able to show stops in details tab
	char eStopType;			// KMA added to be able to show stops in details tab
	long ClientRef;
	long SpareLong;
	char User[USER_LEN];
	char PrefMMId[ECN_LEN];
	char SpareSet[26];

} ORDERDETPKT_3;

typedef struct tdTradeRec1
{
	// Start of Fields to Place Order
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	EXSYMBOL ExSymbol;
	char PrefECN[ECN_LEN];
	long Shares;
	long ReservedShares;
	long lRes1;
	long lRes2;
	long lRes3;
	double Price;
	double StopLimitAmount;
	double TrailingStopAmout;
	double DiscretionaryAmout;
	double dRes1;
	double dRes2;
	double dRes3;
	int AccType;		// 0:Margin 1:Cash,
	int	Action;			// 1:buy, 2:Sell, 3:Short
	int	Type;			// 1:Market, 2:Limit
	int	Gtc;			// True / False
	int	Ext;			// True / False
	int LastStatus;		// See defines following
	int Format;
	int iRes1;
	int iRes2;
	int iRes3;
	// End of Fields to Place Order
	char LastDate[9];
	char LastTime[9];
	char PlacedDate[9];
	char PlacedTime[9];
	char ECNOrderNo[ORDERNO_LEN];
	char ECNExecID[20];
	char ECNLastDate[9];
	char ECNLastTime[9];
	char ECNId[ECN_LEN];
	char MMId[ECN_LEN];
	char OrderNo[ORDERNO_LEN];
	long Pending;
	long ExeShares;
	long LastShares;
	long Canceled;
	long OpenShares;
	long StartClosedShares;
	long ClosedShares;
	long lRes4;
	long lRes5;
	long lRes6;
	int SentforClearing;
	int ConfirmedbyClearing;
	int iRes4;
	int iRes5;
	int iRes6;
	double AvgPrice;
	double LastPrice;
	double ComsSplit[10];
	double OpenCost;
	double OpenComs;
	double AvgOpenPrice;
	double StartCloseCost;
	double CloseCost;
	double AvgStartClosePrice;
	double AvgClosePrice;
	double StartCloseComs;
	double CloseComs;
	double StartCloseProfit;
	double CloseProfit;
	double DAFCharge;
	double dRes4;
	double dRes5;
	double dRes6;
} TRADEREC1;

typedef struct tdPosiRec
{
	EXSYMBOL ExSymbol;
	double Last;
	long  PrevShares;
	long  DayShares;
	double PrevChange;
	double DayChange;
	double PrevPrice;
	double DayPrice;
	double PrevValue;
	double DayValue;	// value - shares*last price
	double PrevProfit;
	double DayProfit;
	double PrevCost;
	double DayCost;	  // cost, shares*price paid
	int   Position;
	int   AccType;
	int Format;
	char CoverSymbol[4]; //This Symbol is not NULL Terminated when 4 Chars Long
}  POSIREC;

typedef struct tdPosiDetRec
{
	char OpenOrderNo[ORDERNO_LEN];
	char CloseOrderNo[ORDERNO_LEN];
	EXSYMBOL ExSymbol;
	char Type;			//0:Previos Position, 1:NewPostion
	int	  Position;		// 0:None, 1:buy, 2:Sell, 3:Short
	int   Closed;
	long  Shares;
	double OpenPrice;
	double OpenCost;
	double ClosePrice;
	double CloseCost;
	double Change;
	double Profit;
	int AccType;
	int Format;
} POSIDETREC;


typedef struct tdAlertsRec
{
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	EXSYMBOL ExSymbol;
	char Last[PRICE_LEN];
	char PriceSmaller[PRICE_LEN];
	char PriceGreater[PRICE_LEN];
	char Price_Delete[PRICE_LEN];
	long Volume;
	long VolumeGreater;
	long Shares_Delete;
	int	 Action_Delete;		// 0:None, 1:buy, 2:Sell, 3:Short
	int	 Type_Delete;			// 1:Limit, 2:Market
	int	 Gtc_Delete;			// True / False
	int	 Ext_Delete;			// True / False
	int  EMail_Delete;			// True / Falee
	int  Active;			// True / Falee
	int	 Format_Delete;
} ALERTSREC;


// Obsolete replace by DOMAINCOUNT
/*
typedef struct tdDomainCounter
{
	long NoOfUsers;
	long TradesReceived;
	long TradesRejected;
	long TradesPlaced;
	long TradesECNRejected;
	long TradesConfirmed;
	long TradesCurrentActive;
	long TradesECNActive[4];
	long TradesCanceled;
	long TradesPartFilled;
	long TradesPartCancel;
	long TradesFullFilled;
	long TradesTotal;
}DOMAINCOUNTER;
*/

typedef struct tdDomainCount
{
	long NoOfUsers;
	long TradesReceived;
	long TradesRejected;
	long TradesPlaced;
	long TradesECNRejected;
	long TradesConfirmed;
	long TradesCurrentActive;
	long TradesECNActive[20];
	long TradesCanceled;
	long TradesPartFilled;
	long TradesPartCancel;
	long TradesFullFilled;
	long TradesTotal;
}DOMAINCOUNT;

/* moved to cls.h
typedef struct tdDomainStats
{
	DOMAINCOUNT DomainCount;
	ACCSTATREC AccStatRec;
	long TotalVolume;
}DOMAINSTATS;
*/
typedef struct {
	WORD	uMsg;
	WORD	cbMsg;
	} XTCMSG;

typedef	struct tdIqmUser
{
//---------------------------------------------	// 60 bytes
//	Login Data
	long	lStatus;
	long	lClass;
	long	lDomain;
	char	cDomain[16];
	char	cName[16];
	char	cPass[16];
//--------------------------------------------- // 112 bytes
//	Name Data
	char	cCompany[48];
	char	cTitle  [16];
	char	cFirst  [16];
	char	cMiddle [16];
	char	cLast   [16];
//--------------------------------------------- // 192 bytes
//	Address Data
	char	cAddress1[48];
	char	cAddress2[48];
	char	cAddress3[48];
	char	cAddress4[48];
//--------------------------------------------- // 64 bytes
//  Demographic Data
	char	cCity   [16];
	char	cState  [16];
	char	cZip    [16];
	char	cCountry[16];
//--------------------------------------------- // 80 bytes
//	Phone Numbers
	char	cPhoneCell[16];
	char	cPhoneFax [16];
	char	cPhoneHome[16];
	char	cPhonePage[16];
	char	cPhoneWork[16];
//--------------------------------------------- // 96 bytes
//	Email Info
	char	cEmailHome[48];
	char	cEmailWork[48];
//--------------------------------------------- // 1024 bytes
//	Enablement Data
	BYTE	cEnable[32][32];
//--------------------------------------------- // 256 bytes
//	Comment Data
	char	cComment[256];
//--------------------------------------------- // 20 bytes
//	BVZ Vars
	long	nonpro;
	long	pro;
	long	freetrial;
	long	daystoexpire;
	long	trialenddate;
//--------------------------------------------- // 140 bytes
	long	lDate;
	long	lTime;
	long	lReqDate;
	long	lReqTime;
//---------------------------------------------
//	Excess to 2K
	char	cRes[124];
//---------------------------------------------
}	IQMUSER;


#define EL_USER_CREATED					10
#define EL_USER_DISABLED				11
#define EL_USER_ENABLED					12
#define EL_ENTITLE_CHANGED				13
#define EL_AGREEMENT_SIGNED				14
#define EL_AGREEMENT_OK_SWITCHED		15
#define EL_NONBILL_SWITCHED				16
#define EL_AGREEMENT_NEEDED_SWITCHED	17
#define EL_DELETED						18
#define EL_SUSPENDED					19
#define EL_VARSID_ACTIVATE_SWITCHED		20
#define EL_VARSID_CHANGED				21


// User view in BO reacts to these messages
#define  BO_USER_LOGIN		0x001
#define  BO_USER_LOGOUT		0x002
#define  BO_FIRSTNAME_IS_ACCOUNT 0x004
#define  BO_FLAGS_SET		0xF0000000


// Standard Actions:


typedef enum eAction {

	eACTION_BUY				= 1,
	eACTION_SELL			= 2,
	eACTION_SHORT			= 3,
	eACTION_EXERCISE		= 4,
	eACTION_CANCEL			= 50,
	eACTION_CANCEL_ALL		= 60

} STANDARD_ACTION;



typedef struct tHEDGE_TRADE
{
	char Domain[DOMAIN_LEN];	// Domain that Account belongs too
	char Account[ACCOUNT_LEN];	// Account order was placed in
	char User[USER_LEN];		// Name of User that places the order

	EXSYMBOL ExSymbol;			// Symbol name including Exchange and Region

	char UserECN[ECN_LEN];		// ECN route selected by user

	char Expiration;		      // 0=Day

	long OrderedShares;			// Number of Shares for Order

	STANDARD_ACTION Action;		// Standard Actions

  union u1
	{
		long HedgeBits;
		struct s1
		{
			unsigned short AutoSell   : 1; //AutoSell Flag for Hedge
			unsigned short Cancel     : 1; //Cancel Flag for Hedge
			unsigned short CancelLast : 1; //Cancel Last Flag for Hedge
			unsigned short CancelAll  : 1; //Cancel All Flag for Hedge

			unsigned short Replaced  : 1; //5 True if this detail item has been replaced by something else - deleeted will be set as well
			unsigned short FromECN  : 1; //6	// true if this line came from TRADERS
			unsigned short Deleted  : 1; //7	// true if this is deleted
			unsigned short BO_Added  : 1; //8
		};
	};

	long Key;					// each detail has a unique key (acually the posiiton on disk) 0 if its an order rec

	char OrderNo[ORDERNO_LEN];	// Internal OrderNo assigned to Order YYYYMMDD-XXXX

	long ExecutedShares;		// Number of Shares executed on this (if filled or part filled)
								// FOR order line this will reflect total filled

	char PlaceECN[ECN_LEN];		// ECN where we are transacting this detail

	char ECNOrderNo[ORDERNO_LEN];  // OrderNo assigned for ECN
	char ECNExecID[40];			// Execution ID assigned by ECN for a specific fill

	char Status;				// Place Status of this detail rec
								// for ORDER will reflect Last known status

	char RecordType;			// 1=this is a DETAIL LINE
								// 2=this is an ORDER LINE - ie a summary of a trade

	long Date;				// Date this detail was created - as a long :YYYYMMDD
	long Time;				// Time this detail was created - as a long	:hhmmss

	int  ECNPort;			// Port at which this order was placed

	char CancelCount;		// number of cancels requested

	long ErrorCode;			// standard error codes


	// private internal only:
	long ReplacedKey;		// If set, this detail replaced the one ref'd by ReplacedKey

	union u2
	{
			char AdminFlags;
			struct s2
			{
				bool	IsDisabled: 1;	// if set, this line has been withdrawn from system
				bool    Invisible : 1;	// if set, we wont make this order part of rest - ie withhold its position
				bool    Temp : 1;		// if set, this is a temp line and is used for calcs and will be deleted
				bool    sp4 : 1;
				bool    sp5 : 1;
				bool    sp6 : 1;
				bool    sp7 : 1;
				bool    sp8 : 1;

			};
	};

	char Spare[95];			// at this rate we need spare space

} HEDGE_TRADE;

typedef struct tSTOCKLOAN
{
// To place order: Fill in from here:

	char Domain[DOMAIN_LEN];	// Domain that Account belongs too
	char Account[ACCOUNT_LEN];	// Account order was placed in
	char Account2[ACCOUNT_LEN];
	char Account3[ACCOUNT_LEN];

	char User[USER_LEN];		// Name of User that places the order
	char LastName[NAME_LEN];
	char FirstName[NAME_LEN];
	char MiddleName[NAME_LEN];

	char Comment[128];

	EXSYMBOL ExSymbol;			// Symbol name including Exchange and Region

	char UserECN[ECN_LEN];		// ECN route selected by user

	char Expiration;		    // 0=Day

	long OrderedShares;			// Number of Shares for Order (request or allocation)

	long ApprovedShares;		// Number of Shares approved on this (if filled or part filled)
								// FOR order line this will reflect total filled

	long NoGoodShares;			// Number of shares denied

	long UsedShares;			// Number of Shares executed on this (if filled or part filled)
								// FOR order line this will reflect total filled

	long PoolSizeTotal;			// Total number of shares available in pool for this symbol from approval system

	STANDARD_ACTION Action;		// Standard Actions

	char OrderType;				// 1=this is a Request for shares
								// 2=this is an Allocation of shares

	char RecordType;			// 1=this is a DETAIL LINE
								// 2=this is an ORDER LINE - ie a summary of a trade

	long Key;					// each detail has a unique key (acually the posiiton on disk) 0 if its an order rec
	char GlobalKey[16];			// Unique internal id

	char OrderNo[ORDERNO_LEN];	// Internal OrderNo assigned to Order YYYYMMDD-XXXX
	char PrevOrderNo[ORDERNO_LEN];
	char OriginalOrderNo[ORDERNO_LEN];

	char PlaceECN[ECN_LEN];		// ECN where we are transacting this detail

	char ECNOrderNo[ORDERNO_LEN];  // OrderNo assigned for ECN
	char ECNExecID[40];			// Execution ID assigned by ECN for a specific fill

	char Status;				// Place Status of this detail rec
								// for ORDER will reflect Last known status

  	char ApprovalId[32];
	char ApprovalSystem[32];
	char OriginalApprovalId[32];		// Spare1
	char OriginalApprovalSystem[32];	// Spare2

	long Date;				// Date this detail was created - as a long :YYYYMMDD
	long Time;				// Time this detail was created - as a long	:hhmmss

	int  ECNPort;			// Port at which this order was placed

	char CancelCount;		// number of cancels requested

	long ErrorCode;			// standard error codes

	union u3
	{
		long StockLoanBits;
		struct s3
		{
			unsigned short UpdatedFromFile   : 1; //The Stock loan was updated from file reload
			unsigned short Cancel     : 1; //Cancel Flag for StockLoan
			unsigned short CancelLast : 1; //Cancel Last Flag for StockLoan
			unsigned short CancelAll  : 1; //Cancel All Flag for StockLoan

			unsigned short Replaced  : 1; //5 True if this detail item has been replaced by something else - deleeted will be set as well
			unsigned short FromECN  : 1; //6	// true if this line came from TRADERS
			unsigned short Deleted  : 1; //7	// true if this is deleted
			unsigned short BO_Added  : 1; //8
			unsigned short FromUserApprovalReq: 1; //9
			unsigned short FromUserShortReq: 1; //10
			unsigned short FromStockLoanFile: 1; //11
			unsigned short CLServerGenerated: 1; //12
			//DT#2526 NS++ 12/12/07 
			unsigned short FromPhonePbShort:  1; // 13 This order was originated from Phone or Pb.com
			//DT#2526 NS-- 12/12/07 
		};
	};

	// private internal only:
	long ReplacedKey;		// If set, this detail replaced the one ref'd by ReplacedKey

	union u4
	{
		char AdminFlags;
		struct s4
		{
			bool	IsDisabled: 1;	// if set, this line has been withdrawn from system
			bool    Invisible : 1;	// if set, we wont make this order part of rest - ie withhold its position
			bool    Temp : 1;		// if set, this is a temp line and is used for calcs and will be deleted
			bool    sp4 : 1;
			bool    sp5 : 1;
			bool    sp6 : 1;
			bool    sp7 : 1;
			bool    sp8 : 1;
		};
	};

	unsigned long SeqNo;	// Sent sequence number (for logging only!)
	char Spare[87];			// at this rate we need spare space

} STOCKLOAN;

#define RT_DETAIL 1
#define RT_ORDER  2

#define STOCKLOAN_REQUEST		1
#define STOCKLOAN_ALLOCATION	2
#define STOCKLOAN_PHONE_PB		3 //DT#2526 NS++-- 12/12/07 


typedef struct tdDefAssignment
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];

	char AllocAccount[ACCOUNT_LEN];

	double Perc;	// percentage

	long  Flags;

	long  Key;

	char Comms[40];

	char Name[40];
	char Comment[80];

	// default commission - NEW fields for R1 20070328 KMA
	double	 Commission;
    char	 CommsType;        // 1111/1111  commission type
	// end NEW fields for R1   20070328 KMA

	char     Spare7[3];		    // back onto boundary

	ULONG	ExchangeCode;	// exhchanges to which this default applies

	char Spare[376];

} DEFASSIGNMENT;

typedef    union tdCamFlags{
        unsigned long Flags;        /* 1111/1111/1111/1111 */
        struct {
				unsigned char	Debug		: 1;	// used for debugging -
	            unsigned char	Locked		: 1;	// true if users cannot edit this list
		        unsigned char	Hidden		: 1;	// True if we dont show this to anyone
				unsigned char	Deleted		: 1;	//
				unsigned char	Default		: 1;	//true if this item is the default
				unsigned char	Global		: 1;	//Applies to all accounts - no need to assign
				unsigned char	spare8			: 1;	//
				unsigned char	spare9			: 1;
				unsigned char	spare10			: 1;
				unsigned char	spare11			: 1;	//
				unsigned char	spare12			: 1;	//
				unsigned char	spare13			: 1;	//
				unsigned char	spare14			: 1;	//
				unsigned char	spare15			: 1;	//
				unsigned char	spare16			: 1;	//
        };
} CAMFLAGS;

#define CAM_DESC_LEN 80
#define CAM_NAME_LEN 20

typedef struct tdCamCodeSet
{
	unsigned char Type;	// Cam Code Type - 1=CMTA's 2=Portfolio Codes
	unsigned short Code;	// item's identifier / Code

	CAMFLAGS CamFlags;

	char ShortDesc[CAM_NAME_LEN];
	char LongDesc[CAM_DESC_LEN];

}CAMCODESET,*pCAMCODESET;

typedef struct tdCamCode
{
	CAMCODESET CamCodeSet;
	char	   Values[10][20];

}CAMCODE, *pCAMCODE;

typedef struct tdCamCodeAcc
{
	char Domain[DOMAIN_LEN];
	char Account[DOMAIN_LEN];
	unsigned char Type;	// Cam Code Type - 1=CMTA's 2=Portfolio Codes
	unsigned short Code;	// item's identifier / Code
	CAMFLAGS CamFlags;
}CAMCODEACC,*pCAMCODEACC;

typedef struct tdDOMAINDOCREC
{
	char Domain[DOMAIN_LEN]; // DOmain name
	char Name[DOCNAME_LEN]; // so we dont have to look it up all the time - DOC name
	char DocId[DOCID_LEN];		// system wide unique DOC Id (string)
	long DocNum;				// document number - UNIQUE DOC NUMBER - system wide -
	int  Version;
	bool Required;	// or optional
	bool AppliesToChildren;		// True means it is needed on all children of this domain

	int SignMethod;
//	{
//		SIGN_NONE = 1,
//		SIGN_OK	= 2,
//		SIGN_I_AGREE = 3,
//		SIGN_PASSWORD = 4,
//	};
	int CancelAction;
//	{
//		CANCEL_LOGOUT = 1,
//		CANCEL_CONTINUE = 2,
//		CANCEL_WARNING = 3,
//	};
	long SignBeforeDate;					// must sign doc before this date - once proper s/w loaded
	char MandatoryBuildGraceMessage[256];	// Message to display until user upgrades the software
	long MandatoryBuildUpgradeDate;			// Last day user has where we display the message, and he can still get in with wrong s/w

	short MandatoryDocSignVersion;			// Frontend Version needed to sign the document
	short SpareShort;

	long ApplicableBuildNumberStart;		// Start FE Build number from which we require signing of THIS document
	long ApplicableBuildNumberEnd;			// Last / end FE Build number until which we require signing of THIS document

	char Signed;		// not part of domain data - temp use in DNS only
	char Ignored;		// not part of domain data - temp use in DNS only

}DOMAINDOCREC,*pDOMAINDOCREC;

typedef struct tdMASTERDOCREC
{
	char DocId[DOCID_LEN];		// system wide unique DOC Id (string)
	long DocNum;				// document number - UNIQUE DOC NUMBER - system wide -
	int  Version;
	char DocType[DOCTYPE_LEN];
	char Path[DOCNAME_LEN]; // so we dont have to look it up all the time - DOC name
}MASTERDOCREC,*pMASTERDOCREC;

typedef struct tdUserDocRec
{
	// these need to be on the MESSAGE
	long SessionID;
	long MessageID;
	// IF WE EVER CHANGE THIS STRUCT YOU HAVE TO FIX ReadUsrAcc() for Remote User Creation
	// AND ALSO AddUAcIndkey() that keeps a copy of some of these fields
	char Domain[DOMAIN_LEN];		//Domain where he signed the doc
	char FromDomain[DOMAIN_LEN];	//Domain which said it was required
	char User[USER_LEN];			//Name of user

	char DocId[DOCID_LEN];			// system wide unique DOC Id (string)
	long DocNum;					// document number - UNIQUE DOC NUMBER - system wide - (maps to the DocId)

	char RequiredOrOptional;	// whether this is required - R or O

	char Signed;				// signed / Needs to Sign
	char SignMethod;			// Whatever method he used to ACK the doc
	long SignDate;				// Date done, or 0 if not yet
	long SignTime;				// Time done, or 0 if not yet
	long SignVersion;			// Version of DOC signed
	long BuildVersion;			// Version of FE used to sign this

	long Deleted;				// When set (to a date) this is ignored. We still keep entry for reporting

	char Initials[USER_LEN];
	char Password[PASSWORD_LEN];

	char DataSpare[992];

} USERDOCREC,*pUSERDOCREC;

typedef struct tdRouteAgreementRec
{
	long RouteAgreementKey;						// internal - read from file, contolled by billing system
	char Route[ECN_LEN];						// Name of the route
	char BillingClientCode[CLIENTCODE_LEN];		// DB Short name / code for client
	long BillingClientKey;						// DB client key - married to BillingClientCode
	char AgreementId[AGREEMENT_LEN];			// Unique Name of agreement
	char Description[DESCRIPTION_LEN];			// Description of agreement - Ie "GS Desk to John Smith"
	char Account[ACCOUNT_LEN];					// Name of fund to which this applies (if filled in - match with Fund in Account)
	char Fund[FUND_LEN];						// Name of fund to which this applies (if filled in - match with Fund in Account)
	char Domain[DOMAIN_LEN];					// Name of domain to which this applies - if filled in

	char ClientCode[CLIENTCODE_LEN];			// Client Code to which this applies - if filled in
	long ClientKey;								// Client Code Key

  union u1
	{
		unsigned long Flags;        /* 1111/1111/1111/1111 */
		struct s1{
				unsigned char	spare1		: 1;	//
				unsigned char	spare2		: 1;	//
				unsigned char	Hidden		: 1;	// True if we dont show this to anyone
				unsigned char	Deleted		: 1;	//
				unsigned char	spare3		: 1;	//true if this item is the default
				unsigned char	spare4		: 1;	//Applies to all accounts - no need to assign
				unsigned char	spare8		: 1;	//
				unsigned char	spare9		: 1;
				unsigned char	spare10		: 1;
				unsigned char	spare11		: 1;	//
				unsigned char	spare12		: 1;	//
				unsigned char	spare13		: 1;	//
				unsigned char	spare14		: 1;	//
				unsigned char	spare15		: 1;	//
				unsigned char	spare16		: 1;	//
		};
	};

} ROUTEAGREEMENTREC;


typedef struct tdVarsIdRec
{
	unsigned char	Version;
	union u1
	{
		char Bits;
		struct s1{
			bool NonPro : 1;    //True if this is a Non-Pro VarsId record
		};
	};
	char			SpareByte;

	union u2
	{
		unsigned char DeletedFlags;
		struct s2{
			bool	Deleted			: 1;
			bool	DomainDeleted	: 1;
		};
	};

	long Key;
	char Id[VARSID_LEN];

	char Domain		[DOMAIN_LEN];
	char Company	[NAME_LEN];
	char Company2	[NAME_LEN];
	char Firm		[NAME_LEN];
	char TaxId		[20];
	char BrokerDealer[NAME_LEN];

	char ShortName	[NAME_LEN];
	char FirstName	[NAME_LEN];
	char MiddleName	[NAME_LEN];
	char LastName	[NAME_LEN];

	char Address1	[NAME_LEN];
	char Address2	[NAME_LEN];
	char Address3	[NAME_LEN];
	char Address4	[NAME_LEN];	// ctually encryption key - 1st char is '1' - '3' & next 24 is the key
	char City		[20];
	char State		[4];
	char Zip		[11];
	char Country	[32];

	char Phone		[16];
	char FAX		[16];
	char Email		[50];

	char SpareBytesKMA[3];

	long Created;
	long Modified;
	char ModifiedBy	[NAME_LEN];

	typedef union u3
	{
		unsigned char StatusBits;        /* 1111/1111 */
		struct s3{
			bool	Received	: 1;	//
			bool	SentToExch	: 1;	//
			bool	Approved	: 1;	//
			bool	Scanned		: 1;	//
			bool	Suspended	: 1;	//
		};
	} FeedStatus;


	FeedStatus Status		[154];		// Status bits for each feed - valid if corresponding Entity has a number in it
	unsigned short Entity	[154];		// Entity Key for each feed  -
										// a NON-ZERO value in this array means that the STATUS field is valid and must be tested

} VARSIDREC, *pVARSIDREC;

#endif
