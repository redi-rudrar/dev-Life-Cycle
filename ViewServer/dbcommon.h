// Modified for VS2010 conversion 

#ifndef __DBCOMMON_H_
#define __DBCOMMON_H_

#include "sizes.h"
#include "Build.h"
//`#include "accset.h"


// 2010 C++ is 1600
#if _MSC_VER < 1600
#undef sprintf_s
#define sprintf_s _snprintf
#endif

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
#define CAM_OVERRIDE_RESTRICTED_LIST 14	
#define CAM_OVERRIDE_NEWRESTRICTED_LIST 15 // DT7668 - bjl - New Restricted List
#define CAM_OVERRIDE_RULE201 16		// DT7703 - bjl - Rule 201 - Order Validation Logic
#define CAM_LISTID 17 // DT10235 - bjl - IQOS
#define CAM_GROUPID 18 // DT10235 - bjl - IQOS

#define CAM_CMTA_VALUE_CMTA 0
#define CAM_CMTA_FILTER_ALLOATION4 1 // if ALLOCATE_MPID -> then use this to detemine whether we need to add to aggregate list for allocation

#define CAM_PORTFOLIO_VALUE_SD	0
#define CAM_SHORTAUTH_VALUE		0
#define CAM_PROGRAM_VALUE	0
#define CAM_SHORTLOCATE_VALUE 0
#define CAM_SOLUNSOLICITES_VALUE 0
#define CAM_SHORTLOCATE_METHOD 9
#define CAM_ANONYMOUS_VALUE	0
#define CAM_TRANSTYPE_OPT_VALUE	0 // Trnsaction Type Code for OPTIONS	- BLUESHEETS
#define CAM_TRANSTYPE_OPT_ISBD 	1 // IsBD value for OPTIONS

#define CAM_TRANSTYPE_EQT_VALUE	0 // Trnsaction Type Code for EQUITY	- BLUESHEETS
#define CAM_TRADER_ALT_ACC_VALUE 0 		// DT#1307 - Futures Account CAM Code  aka - Send frontend selected account to trader
#define CAM_FUTURES_CAPACITY_VALUE 	1		// DT9181 Bud 05/09/11 Capacity for Futures
#define CAM_FUTURES_COUNTRYSTATE_VALUE 	2	// DT9181 Bud 05/09/11 Country/State for Futures
#define CAM_NOT_HELD_VALUE 0 	// DT#2663 - oats Feb 4 2008 - not held - needed for VFIN route?

#define CAM_OVERRIDE_RESTRICTED_LIST_VALUE 0	

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
#define FEATURE_ANALYTICS			0x00002	// DT8296 Bud 02/08/11	EPX/GDA User Entitlement
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
//#define FEATURE_UBS					0x01000 // true if we have UBS stuff on - export ttype 19 - ABNPRIME
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
#define FEATURE_MULTIACC			0x10000000 // DT5636 kma 7/14/09 Multi Account Selection entitlement 
#define FEATURE_ORDERSTAGING		0x20000000 // true if we are enabled for orderstaging DT4419 bud 11/24/2008
#define FEATURE_FACILITATED_ORDERS	0x40000000 // true if we are enabled for facilitated DT5453 troymguillory 02jun2009
#define FEATURE_PAIRS				0x80000000 // DT7549 - bjl - Pairs Trading

#define FEATURE_ACCOUNT_ROUTES		(FEATURE_ROUTES_REPLACE | FEATURE_ACCOUNT_ROUTES_MERGE)

#define SHORT_ACCOUNT 2
#define CASH_ACCOUNT 1
#define MARGIN_ACCOUNT 0

#define ACTION_BUY 1
#define ACTION_SELL 2
#define ACTION_SHORT 3

// Account actions - after trade plonked into correct account - used by allocations
#define AACTION_BUY				11
#define AACTION_SELL			12
#define AACTION_SELL_SHORT		13
#define AACTION_BUY_TO_CLOSE	14

// Special handling bits
#define OT_ADD 0x00000001
#define OT_AON 0x00000002
#define OT_CNH 0x00000004
#define OT_DIR 0x00000008
#define OT_DLO 0x02000000
#define OT_EPT 0x00000010
#define OT_FOK 0x00000020
#define OT_GO  0x08000000
#define OT_IO  0x00000040
#define OT_IOC 0x00000080
#define OT_LOC 0x00000200
#define OT_LOO 0x00000100
#define OT_MAC 0x00000800
#define OT_MAO 0x00000400
#define OT_MOC 0x00001000
#define OT_MOO 0x00002000
#define OT_MQT 0x00004000
#define OT_MTL 0x00000800
#define OT_NH  0x00008000
#define OT_OPT 0x01000000
#define OT_Go  0x08000000
#define OT_OVD 0x00010000
#define OT_PEG 0x00020000
#define OT_RSV 0x00040000
#define OT_SCL 0x00100000
#define OT_SST 0x00080000
#define OT_TMO 0x00200000
#define OT_TS  0x00400000
#define OT_WRK 0x00800000
#define OT_F0  0x04000000
#define OT_F3  0x00080000
#define OT_F6  0x08000000
#define OT_F7  0x00000001
#define OT_F8  0x00000002
#define OT_F9  0x10000000
#define OT_FA  0x00000004
#define OT_FB  0x00000008
#define OT_FC  0x00000010
#define OT_FD  0x00000020
#define OT_FH  0x00000040
#define OT_FI  0x00000080
#define OT_FJ  0x00000100
#define OT_FK  0x00000200
#define OT_FL  0x00000400
#define OT_FM  0x20000000
#define OT_FN  0x00001000
#define OT_FO  0x00002000
#define OT_FP  0x00004000
#define OT_FQ  0x00008000
#define OT_FR  0x00010000
#define OT_FS  0x00020000
#define OT_FT  0x00040000
#define OT_FW  0x40000000
#define OT_FX  0x00100000
#define OT_FY  0x00200000
#define OT_FZ  0x00400000
#define OT_Fb  0x00800000
#define OT_Fc  0x01000000
#define OT_Fd  0x02000000
#define OT_Fe  0x04000000
#define OT_AOB 0x10000000
#define OT_MOB 0x40000000


typedef struct tdExSymbol
{
	char Exchange;
	char Region;
	char Symbol[SYMBOL_LEN];// SYMBOL_LEN=22 DEV-1778 - Options need to be 21 chars
	//unsigned char CurrencyCode;
	
	operator const char*()
	{	static char s[SYMBOL_LEN+12];
		::sprintf_s(s,sizeof(s),"%s,%d,%c,%d",Symbol,Exchange,Region,-1);
		return s;
	}

} EXSYMBOL;

typedef struct tdUserAccEnts
{
	union {
		char TraderBits;
		struct
		{
			bool Trader  : 1;
			bool IgnorePrefMM : 1;
			bool AllowFacilitatedOrders : 1;	// 12may2009 troymguillory DT5453
			bool tSpare4 : 1;
			bool tSpare5 : 1;
			bool tSpare6 : 1;
			bool tSpare7 : 1;
			bool tSpare8 : 1;
		};
	};
	union {
		char ViewTradingBits;
		struct
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
	union {
		char ViewAccountBits;
		struct
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
	union {
		char AllocBits;
		struct
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
    union {
        char	Bitfields;        /* 1111/1111  */
        struct {
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

#if BUILD_NUMBER < 21170 && !defined OATSGEN
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

    union {
        long BORightsEx;        /* 1111/1111/1111/1111 */
        struct {
			bool	VarsIdAdmin:	 1;	// true if user can do VARSID Administration
			bool	VarsIdSuper:	 1;	// true if user can do VARSID Supervisor - can edit old vars
            bool	IQNetRoutesViewOnly	: 1;	// true if user has view-only for the routes dialogue : TD#5208
	        bool	BOAllocationsOnly	: 1;	// to replace the SEC_ALLOCATEONLY bit - pv 12/06/2006
			bool	BOViewOnly			: 1;	//
			bool	BOCanCancelOrders	: 1;	// True if this user can cancel orders from the BO
			bool	TechSupport			: 1;	// DT 1374: Tech Support will not be allowed to view personal information
			bool	IQDevUser			: 1;	// LL 20080124 DT2855 - add DEV user entitlement
			bool	IQOSOnly			: 1;	// DT4419 troymguillory 2009jul09
			bool	IqTradeMultiAcc		: 1;	// DT5636 - Multi Account trading
			bool	OptronicsWithEquity	: 1;	// DT6254 - kma - R4 2009 Optronics entitlement
			bool	OptronicsOptionsOnly: 1;	// DT6254 - kma - R4 2009 Optronics entitlement
			bool	AllowPairs			: 1;	// DT7549 - bjl - Pairs Trading
			bool	AllowGDA			: 1;	// DT8292 - jonhull 2011/02/22
			bool	BOEditRoutes		: 1;	// DT10959 - bjl - R3 2012: Allow B/Ds to add/remove routes to their accounts  
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

    union {
        long	VarsIdBitFields;        /* 1111/1111  */
        struct {
			bool		VarsIdHasOwnField	: 1;	// True if vars id is no longer in company field
            bool		VarsIdEdit			: 1;	// True if vars id data doesnt match user record data
			bool		VarsIdActivated		: 1;	// Vars Id has been activated for this user
			bool		VarsIdForced		: 1;	// Vars Id activation was forced for this user
			bool		VarsIdChanged		: 1;	// Vars Id activation has been changed, but not activated
		};
    };
	char Citizenship1[2];		// ISO country code NOT null terminated
	char Citizenship2[2];

    union {
        long UserFlagsEx;        /* 1111/1111/1111/1111 */
        struct {
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
	char EncryptionType;	// dt10091 -  // PASSWORD Encryption type 0-Plain text, 1-MD5 2-IQ-SGA-512 
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

	long ScheduledDisableDate;		// YL 20090413 DT5146 - date that the user should be disabled
	
	char CostCenter[60];			// DT6280 - kam - Add "Cost Center" field to User Account Setup in BO
	char ActivationCode[PASSWORD_LEN];	// DT10087 DLA 20110826
	unsigned char PasswordHashLen;		// dt10091 - SAVE PWD hash
	unsigned char PasswordHash[255];	// dt10091 - SAME PWD hash
// + DEV-1286 - bjl - Add Name, Address, and Stockloan ID to user File (ADMIN directory)
	char RepId[32];
	char Active;
	char BigSpace[5041];  // dt10091 - SAME PWD hash
// - DEV-1286
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

    union {
        short BitFields;        /* 1111/1111/1111/1111 */
        struct {
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
// + DT11533 - bjl - R4 2012 - CL needs to use Sub-Routes when enabling routes for an Account
	char SubRouteName[20];
	char sSpare[60];			 // Extra space
// - DT11533
	char RouteSpare[4*1024]; // ROUTE structure will come in here

} ACCROUTEREC,  *pACCROUTEREC;
// + DT11533 - bjl - R4 2012 - CL needs to use Sub-Routes when enabling routes for an Account
// New smaller Account Route Record, now with 98.16% less wasted space
typedef struct tdSmallAccRouteRec
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char Route[ECN_LEN];
	char Agreement[AGREEMENT_LEN];	// AgreementShortName in DB    Client / ROute / Destination Agreement

    union {
        short BitFields;        /* 1111/1111/1111/1111 */
        struct {
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
	char SubRouteName[20];
} SMALLACCROUTEREC,  *pSMALLACCROUTEREC;
// - DT11533
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
            bool	   CapacityPrincipal	: 1;	//10 Account is Agent (Customer) or Principal
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
			bool	EnableTimeAndTick : 1; // DT10233 - bjl - Account Settings for New Time and Tick File
			bool	AllowFirmToFirm : 1; // DT10954 - bjl - GELP: Facilitated Orders - block firm to firm
		};
	};
	// - DT6616 - Restricted List (CL)

	// DT6566 YL 20091216 - start
	union {
		char MarginSettings;
		struct {
			bool	UseNewMargin	: 1;	// Use new margin engine instead of buying power
			bool	BOReleaseIntradayBP : 1;	// DT7001 DLA 20100401, used only for sending ACCINDKEY.ReleaseIntradayBP to backoffice
			bool	DoSellPosiCheckWhenIgnoringTradeRules : 1; // DT10414 - bjl - Request to not sell more than the current position setting in back office, seperate from "Ignore Trading Rules"
		};
	};
	// DT6566 YL 20091216 - end

	char ReleaseIntradayBPSwitch;		// see enum eReleaseIntradayBP for values // DT7001 DLA 20100401

	// + DT7809 - bjl - Additional Client Identifiers
// + DT10964 - bjl - R3 2012: Add Large Trader Reporting Field in Backoffice 
	char Spare[2137];					/// NOT IN DB	// DT6566 YL 20091216	// DT6616 - Restricted List (CL) // DT7031 DLA 20100326  // DT7001 DLA 20100401	// DT9033 DLA 20110425
	char IQAccountName[ACCOUNT_LEN];
	char LargeTraderReportingID[25];
// - DT10964
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
	char TimeInForce;			// 0=Day, 1=Good Till Cancel(GTC), 2=At the opening(OPG), 3=Immediate Or Cancel(IOC), 4=Fill Or Kill(FOK), 5=Good Till Crossing(GTX), 6=Good Till Date
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
		char SettlementCurrency[4]; // DEV-21994: Add Futures to Lifecycle drop  (ISO 4217 Standard Currency)
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

	double PriceDbl;			// CALCULATED:Total Income from Closing the order
	long   OptExpireDate;		// Option Expiration Date
	float DisplayValue;			//Peg Diff
	double something_deleted;		// same as price as a dbl
	float CurrentStopPrice_deleted;		// Current Trigger Price for Trailing Stops
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
			bool			   EcnGtd : 1;		// DT6383 - XTrade Gap (CL) - Native GTCs - TRUE means its Exchange based.

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
// WAS        bool	   Express			: 1;	// DEV-7257 - Update EOD files from bust and correction messages
//            bool	   ArcaEx			: 1;	// DEV-7257 - Update EOD files from bust and correction messages
            bool	   isBust 			: 1;	// DEV-7257 - Update EOD files from bust and correction messages
            bool	   isCorrection		: 1;	// DEV-7257 - Update EOD files from bust and correction messages

        };

    };
	char	UserType;
	unsigned long    ExpireTime;			// DEV-17640
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
	char Settlement;				// DEV-7231 - bjl - Clserver - Allow Settlement instructions to populate EOD clearing file
	union {
		char spare2_deleted;		// DEV-7231 - bjl - Clserver - Allow Settlement instructions to populate EOD clearing file
		char ProductType;			// DEV-21994: Add Futures to Lifecycle drop 
	};
	short FIXLineId;
	unsigned short EcnRandomReserveRange; // 0-50000 random reserve range

	// DT9035 Bud 20110708
	union {
		char CancelMemo[23];
		struct
		{
			char nullByte;
			char OATSConnectionID[13];
			char SpareForOATSGiveup[9];
		};
	};
	// end DT9035 Bud 20110708

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
	union {
		char TrailTriggerCount;
		char RegionGroup;			// DEV-21994: Add Futures to Lifecycle drop
	};
	char StopTriggerCount;

	short StepRefNo;		// Step on execution path - can be a short	// DT7706 bud 08/12/10
	char ShortExemptCode;												// DT7706 bud 08/12/10
	char OverrideRestrictedListCode; //0 = CC14 override, 1 = Emergency Override, 2+ custom	// DT7668 - bjl - New Restricted List	// DT7706 bud 08/12/10

	char Notes[8];			// used for Short Sale Request Id
	char ExecutingBrokerCode[7];

//	long WorkOrderRefNoX;
//	char Spare[3];

	char LinkOrderNo[ORDERNO_LEN - sizeof(long)];

#pragma pack(push)
#pragma pack(1)
	long ComplexSharesRatio;							// Shares/complex order for this leg
#pragma pack(pop)
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

/*typedef struct tdBatchOrderInfo
{
	char BatchName[9];
	char Spare1;

	WORD	Row;
	long	BatchNumber;

} BATCHORDERINFO;
*/

#define CAPACITY_UNKNOWN			0
#define CAPACITY_AGENCY				1
#define CAPACITY_CROSS_AS_AGENCY	2
#define CAPACITY_CROSS_AS_PRINCIPAL	3
#define CAPACITY_PRINCIPAL			4
#define CAPACITY_RISKLESS_PRINCIPAL	5 // BOFA says so - not in FIX spec


#define STRAT_ECN_GETS				1			//CIBC's Version of EASE - kleder - 05.26.2005 - TD#2125

typedef struct tdTradeRecB
{
	char StrategyName[12];// DEV-8466 CL Needs to support the Algo Strategy used so that it can be passed in the Lifecycle Drop	


//	unsigned char EASE_TargetStrat_deleted;	// REDI DEV-9723	Need BigTradeRec fields allocated for Redi OATS
	char oats_DeskTypeCode;	// // REDI DEV-9723	Need BigTradeRec fields allocated for Redi OATS

    //unsigned char EASE_ExecMode_deleted;	// REDI DEV-9723	Need BigTradeRec fields allocated for Redi OATS
	char oats_AccountTypeCode;				// REDI DEV-9723	Need BigTradeRec fields allocated for Redi OATS
	
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

#pragma pack(push)
#pragma pack(1)
       union
       {
             struct
             {
                    unsigned short ShortAuthMethod;
                    unsigned char ShortAuthResult;
                    char CamList[24];// camcodes sent from fe
             };
             char ECNExecID[27];
       };
#pragma pack(pop)


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
			unsigned char NotOATSReportable	: 1;	// DT10158 Bud 20110908
			unsigned char RediSpreadTrade	: 1;	// Indicates this is a spread trade (currently only used to determine which CMTA for allocations)
			unsigned char isOrderStatus		: 1;	// DEV-11284
			unsigned char SpareOrderBits	: 1;	// DT9355 Bud 05/23/11
			char LocateType;						// DT12196 Bud 20130403
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
	char  RediOrderType;			// as reported from REDI/VS - indicates ticket, normal, etc aka TWIST Ticket
	float CommissionOverride;
	long  GroupOrderNo;			// assigned for CancelReplace and Grouped orders

	char ISIN[13];// redi
	char spare33_deleted[3];


	// FIX SERVER NEEDS TO POPULATE
	char ClientOriginalOrderId[20];		// used as RoutedOrderId in OATS

	long WriteTimeStamp;
	union {
		long unused_CRDelta_deleted; 	// dff for RVR
		char Currency[4];				// DEV-21994: Add Futures to Lifecycle drop (ISO 4217 Standard Currency)
	};
	char RegSho200ExemptReason_deleted;	// REGSHO Rule 200 reason 1=EXEMPT 2=G2 3=G3 and add others  here
	char TraderGiveUp[MMID_LEN];

	char TranTypeCode;				// Transaction type code - defaults to what is set on the account record
	unsigned short Strategy_ECN_deleted;	//Added to enumerate the renamed instances of Strategy Route (ie EASE -> GETS)
	char ProgramTradeStrategy_deleted[2];

	char Solicited;				// S=Solicited, U=Unsolicited, blank=unknown
	unsigned char PriceCurrencyCode;// Currency code in which the Price was give (as per FIX spec)

	char RediDestination[25]; 
//	long RouteAgreementId;		// SPARE Client Agreement ID used to OK this
//	char RouteFundId[21];		// SPARE Fund Id attached to the route agreement above

	char eStopType;				// DT:1014.844 E-Stops

	char CustomCapacity;		// DT4792 - Market Maker Capacity - will be an index into a table that the trader uses to send out
	unsigned char Version;		// DT6090 bud 10/21/09

	float currentBid;			// DT10988 Current Bid on Detail
	float currentAsk;			// DT10988 Current Ask on Detail
	//unsigned long  IQOSRollupID; // REDI DEV-9723 Need BigTradeRec fields allocated for Redi OATS
    long OATS_special2;// REDI DEV-9723      Need BigTradeRec fields allocated for Redi OATS

	
	char oats_OriginatingDepartmentID[4]; // REDI DEV-9723	Need BigTradeRec fields allocated for Redi OATS

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


	
	long OATS_special;					// REDI DEV-9723	Need BigTradeRec fields allocated for Redi OATS


	char OptClass8[8];			// DT5676 - 20090626 - kma - OSI - OptClass[4] and OptClass8[8]  fields populated
	char ECNExecID_aux[ACCOUNT_LEN];	// Used in conjunction with ECNExecID to support 40 character ExecIDs for allocations
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
} TRADERECB;





typedef struct tdOriginalBigTradeRec
{
	TRADEREC TradeRec;
	TRADERECA TradeRecA;

} ORIGINALBIGTRADEREC;

//NOTE" FIX server uses this struct in a message - it will need to be changed if this struct changes size

#pragma pack(push)
#pragma pack(1)

typedef struct tdMiFID
{
	char SendingTime[21];  // Includes milliseconds
	char TransactTime[21]; // Includes milliseconds
	char BrokerLEI[21];
	char ApaMicCode[6];
	char TransReportVenue[6];
	int TradeReportIndicator;
	int RoleIndicator;
	int MatchingType;
	int TradeSubType;
	int SecondaryTrdType;
	char SecondaryExecID[42];
	int NoTradePriceConditions;
	char TradePriceCondition[6];
	int NoTrdRegPublications;
	char TrdRegPublicationType[6];
	char TrdRegPublicationReason[6];
	char SystematicInternalizer;
	char RiskReduction;
	char TradingInst;
	char AlgoOrder;
	char Undisclosed;
} MiFID;

#pragma pack(pop)

typedef struct tdBigTradeRec
{
	TRADEREC TradeRec;
	TRADERECA TradeRecA;
	TRADERECB TradeRecB;
	MiFID mifid;
	union
	{
	char Unused[195];
		struct
		{
			char LastPriceAsChar[22];
			char RestateReason;
		};
	};
	std::string LastPriceStr();

} BIGTRADEREC,ORDERREC,* pORDERREC,*pBIGTRADEREC;
//NOTE" FIX server uses this struct in a message - it will need to be changed if this struct changes size

typedef struct tdBigTradeRec_Old
{
	TRADEREC TradeRec;
	TRADERECA TradeRecA;
	TRADERECB TradeRecB;
} BIGTRADEREC_OLD;


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
	union
	{
		char ExpirationFlags[2];
		struct
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
	union
	{
		char SomeBits;
		struct
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
#define P_BitFieldsA			75
#define P_OrderBits				76	// DT7549 DLA 20100910 - Pairs Trading


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
#define EL_OPTRONICS_O_CHANGED			22 // DT6254- kma - R4 2009 Optronics entitlement
#define EL_OPTRONICS_E_CHANGED			23 // DT6254- kma - R4 2009 Optronics entitlement


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

// begin DT7001 DLA 20100406
enum eReleaseIntradayBP {
	eReleaseIntradayBP_ON	= 0,
	eReleaseIntradayBP_OFF	= 1,
	eReleaseIntradayBP_AUTO	= 2
};
// end DT7001 DLA 20100406

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

  union
	{
		long HedgeBits;
		struct
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

	union
	{
			char AdminFlags;
			struct
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

	union
	{
		long StockLoanBits;
		struct
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

	union
	{
		char AdminFlags;
		struct
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

	long SpareLong;
//	unsigned long SeqNo;	// Sent sequence number (for logging only!)
	char Rate[11];			// Twist Bus indicativeRate	// DT10744 DLA 20120203
	char Spare[76];			// at this rate we need spare space	 // DT10744 DLA 20120203

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

	char    AllocCMTA[6]; // DT6598 - kma - 1/5/10 - CMTA in allocationm
	char Spare[370];		// DT6598 - kma - 1/5/10 - CMTA in allocationm

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

  union
	{
		unsigned long Flags;        /* 1111/1111/1111/1111 */
		struct {
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
	union
	{
		char Bits;
		struct {
			bool NonPro : 1;    //True if this is a Non-Pro VarsId record
		};
	};
	char			SpareByte;

	union
	{
		unsigned char DeletedFlags;
		struct {
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

	typedef union
	{
		unsigned char StatusBits;        /* 1111/1111 */
		struct {
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

// NOTE: Shared with the clserver, if changing notify clserver group
typedef enum RediRegionGroup {
	RG_UNKNOWN=0,
	RG_24E,
	RG_24O,
	RG_AS,
	RG_HKE,
	RG_HKO,
	RG_LN,
	RG_RE,
	RG_REE,
	RG_REO,
	RG_RT,
	RG_RTE,
	RG_RTO,
	RG_US
} REGIONGROUP;

#endif
