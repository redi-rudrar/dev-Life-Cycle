
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"

#ifdef IQSMP
extern "C"
{
#pragma pack(push,8)

//Copied from "..\..\TraderLook\PlaceLog.h"
#include "sizes.h"
#define ORDERNO_LEN64 64
#define EXTRADATA_LEN 256
#include "dbcommon.h"
// Small placelog: shoud be 560 bytes
typedef struct tdPlaceLogItem1
{
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponceTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , recieved from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	TRADEREC TradeRec;
	unsigned long Offset;
	tdPlaceLogItem1 *NextPlaceLogItem;
} PLACELOGITEM1;

// Big placelog: shoud be 960 bytes
typedef struct tdPlaceLogItem2
{
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponceTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , recieved from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
    // New one from trader-dev
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem2 *NextPlaceLogItem;
} PLACELOGITEM2;

// Big placelog: shoud be 992 bytes
typedef struct tdPlaceLogItem3
{
    unsigned char VerTag;               // Must be 0xFF
    unsigned char Version;              // Version number=3
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponceTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , recieved from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
    // New one from trader-dev
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem3 *NextPlaceLogItem;
    // Added 08/18
    char DeliverToCompId[16];
    char DeliverToSubId[16];
//	unsigned long OrderTime;
} PLACELOGITEM3;

// Big placelog: shoud be 992 bytes
typedef struct tdPlaceLogItem3HV
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number=3
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	long ResponseTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem3 *NextPlaceLogItem;
	char SenderSubID[16];
	char TargetSubID[16];
	char ClientRoute[ECN_LEN];
	char PreCanRepStatus;		// Used to restore LastStatus if cancel or replace is rejected
	char CancelOrderNo[ORDERNO_LEN];
	long OrderQty;			// chain of order quantity
	long CumQty;			// chain of cumulative quantity
	double TotalCost;		// chain of cost (for avg price)
	char ECNOrderNo[ORDERNO_LEN];
} PLACELOGITEM3HV;

// 1256 Bytes
typedef struct tdPlaceLogItem4
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number=4
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponseTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	// New one from trader-dev
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem4 *NextPlaceLogItem;
	// Added 08/18
	char DeliverToCompId[16];
	char DeliverToSubId[16];
	unsigned long OrderTime;
	char NameValuePairs[256];   
} PLACELOGITEM4;


// HV FIX Server 
typedef struct tdPlaceLogItem5
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	long ResponseTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem5 *NextPlaceLogItem;
	char SenderSubID[16];
	char TargetSubID[16];
	unsigned long OrderTime;
	char ClientRoute[ECN_LEN];
	char PreCanRepStatus;		// Used to restore LastStatus if cancel or replace is rejected
	char CancelOrderNo[ORDERNO_LEN];
	long OrderQty;			// chain of order quantity
	long CumQty;			// chain of cumulative quantity
	double TotalCost;		// chain of cost (for avg price)
	char ECNOrderNo[ORDERNO_LEN];
	bool IsACancelPending;  // DT1079: Keep track of pending cancels in order to report the correct OrdStatus on Execution Reports.
	bool IsAReplacePending; // DT1079: Keep track of pending replaces in order to report the correct OrdStatus on Execution Reports.
} PLACELOGITEM5;

// ATE FIXServer
typedef struct tdPlaceLogItem6
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number=3
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponseTimer;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem3 *NextPlaceLogItem;
	char DeliverToCompId[16];
	char DeliverToSubId[16];
	unsigned long OrderTime;
	void* HoldList;
	void* SkipList;
	char PendingCancelNo[ORDERNO_LEN]; // Current cancel orderno
	char PendingStatus; // 0,'E','6','A','H','S'
	char PreviousStatus; // BTR.LastStatus before PendingStatus
	char CxlRejected; // 0,1
	char CxlConfirmed; // 0,1
	int TargetConPort;
	char ClientDomain[DOMAIN_LEN];
	char NameValuePairs[256];   
} PLACELOGITEM6;

typedef struct tdPlaceLogItem7
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponceTimer;
	char ParserRefNo[ORDERNO_LEN64]; //ORDERNO_LEN]; // Used to send Cancels , recieved from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	unsigned long Offset;
	tdPlaceLogItem7 *NextPlaceLogItem;
} PLACELOGITEM7;

// From FIXServerIQOS
// We'll use and send this placelogitem to trademon
struct MLCLData		// DT5143 RCN 20090327
{
    int         ival_111;
    int         ival_6211;
    int         ival_6389;
    int         ival_6401;
    int         strategy;
    int         ival_6403;
    int         ival_6404;
    int         ival_6407;
    int         ival_6411;
    int         ival_6439;
    int         ival_6440;
    int         ival_6469;
    int         ival_6470_1;
    int         ival_6410;
    char    	sval_6434[SYMBOL_LEN_INTERNAL];
    char        sval_126[32];
    char        sval_6168[32];
    char        sval_6436[16];
    char        sval_6441[16];
    char        sval_6448[8];
    char        sval_6468[8];
    char        sval_6442[8];
    char        sval_6466[8];
    char        sval_81[2];
    char        iceberg[2];
    char        sval_6336_1[2];
    char        sval_6336_2[2];
    char        sval_6336_3[2];
    char        sval_6408[2];
    char        sval_6408_1[2];
    char        sval_6412[2];
    char        sval_6412_1[2];
    char        sval_6412_2[2];
    char        sval_6424[2];
    char        sval_6470[2];
    char        sval_6483[2];
    char        sval_6493_1[16];
    char        sval_6493_2[2];
    char        crossingparameters[2];
    int         mlStrategy;
	char		sval_IQEntryTime[22];

    MLCLData()
    {
        reset();
    }

    void reset()
    {
        memset(sval_81, '\0', sizeof(sval_81));
        memset(iceberg, '\0', sizeof(iceberg));
        memset(sval_126, '\0', sizeof(sval_126));
        memset(sval_6168, '\0', sizeof(sval_6168));
        memset(sval_6336_1, '\0', sizeof(sval_6336_1));
        memset(sval_6336_2, '\0', sizeof(sval_6336_2));
        memset(sval_6336_3, '\0', sizeof(sval_6336_3));
        memset(sval_6408, '\0', sizeof(sval_6408));
        memset(sval_6408_1, '\0', sizeof(sval_6408_1));
        memset(sval_6412, '\0', sizeof(sval_6412));
        memset(sval_6412_1, '\0', sizeof(sval_6412_1));
        memset(sval_6412_2, '\0', sizeof(sval_6412_2));
        memset(sval_6424, '\0', sizeof(sval_6424));
        memset(sval_6434, '\0', sizeof(sval_6434));
        memset(sval_6436, '\0', sizeof(sval_6436));
        memset(crossingparameters, '\0', sizeof(crossingparameters));
        memset(sval_6441, '\0', sizeof(sval_6441));
        memset(sval_6442, '\0', sizeof(sval_6442));
        memset(sval_6448, '\0', sizeof(sval_6448));
        memset(sval_6466, '\0', sizeof(sval_6466));
        memset(sval_6468, '\0', sizeof(sval_6468));
        memset(sval_6470, '\0', sizeof(sval_6470));
        memset(sval_6483, '\0', sizeof(sval_6483));
        memset(sval_6493_1, '\0', sizeof(sval_6493_1));
        memset(sval_6493_2, '\0', sizeof(sval_6493_2));
        ival_111=ival_6211=ival_6389=ival_6401=strategy=ival_6403=ival_6404=ival_6410=ival_6411=ival_6439=ival_6440=ival_6469=mlStrategy=ival_6470_1=ival_6407=-1;
        memset(sval_IQEntryTime, '\0', sizeof(sval_IQEntryTime));
    }
};
#define PLACELOG_NAMEVALUE_BUFFER_SIZE 256
// FIXSERVER placelog: shoud be 992 bytes
typedef struct tdPlaceLogItem3IQOS
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long TradePlaceTime;
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	// New one from trader-dev
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
    MLCLData    mlCLData;
	unsigned long Offset;
	tdPlaceLogItem3 *NextPlaceLogItem;
	// Added 08/18
	char DeliverToCompId[16];
	char DeliverToSubId[16];
//	char OverallOrdStatus;
//#ifdef ORDER_TIMINGS
	unsigned long OrderTime;

    //=================================================
    // Parameters for OrderHolding
	//char holdMsg[1024];
	struct hMsgs* HoldList;
	struct hMsgs* SkipList;
	char PendingCancelNo[ORDERNO_LEN]; // Current cancel orderno
	char PendingStatus; // 0,'E','6','A','H','S'
	char PreviousStatus; // BTR.LastStatus before PendingStatus
	char CxlRejected; // 0,1
	char CxlConfirmed; // 0,1
	int TargetConPort;
    //=================================================

	//#endif
	char ClientDomain[DOMAIN_LEN];
	bool IsACancelPending;  // DT1763: Keep track of a pending (in the market) cancel for this item
	bool IsAReplacePending; // DT1763: Keep track of a pending (in the market) replace for this item
	char NameValuePairs[PLACELOG_NAMEVALUE_BUFFER_SIZE];  

    bool ComplexOrder;                                  // Mark if an order is complex
    char ComplexParentParserOrderNo[ORDERNO_LEN];       // Populated in a child order, pointing to its parent; empty for parent
    char ComplexChildLegRef[ORDERNO_LEN];               // Empty for parent order
    char ComplexNextChildNextLegRef[ORDERNO_LEN];       // Link all children orders under a parent

    char IQOSListId[IQOS_LIST_LEN];                     // IQOS ListID
    char IQOSGroupId[DOMAIN_LEN];
    int  IQOSNumOfParentOrder;                          // Total number of parent orders in a list
    char IQOSPrevParentParserOrderNo[ORDERNO_LEN];      // Link all IQOS parent orders from a list
    char IQOSNextParentParserOrderNo[ORDERNO_LEN];      // Link all IQOS parent orders from a list
    int  IQOSLastFilled;                                // Number of shares filled by the order that is replaced by the current order
} PLACELOGITEM3IQOS;

typedef struct tdPlaceLogItem8	// DT5143 RCN 20090327
{
	unsigned char    VerTag;							// Must be 0xFF
	unsigned char    Version;							// Version number
	char             ParserOrderNo[ORDERNO_LEN];		// Used when Order was placed
	char             ParserOrderNoGiveup[GIVEUP_LEN];	// GiveUp used with above OrderNo
	char             Giveup[GIVEUP_LEN];				// Domains NASDAQ giveup
	char             ISICode[GIVEUP_LEN];				// Domains ISI Code or ADP TERMINAL ID chars 1&2
	char             ClearingAccount[ACCOUNT_LEN];		// Clearing Account for REDI, open for the rest
	long             GiveupOrderNo;						// Auto Numbering for ECN's that does not use IqouteOrderNo
	long             ResponseTimer;
	char             ParserRefNo[ORDERNO_LEN];			// Used to send Cancels , received from ECN at confirm
	char             ReplaceOrderNo[ORDERNO_LEN];		// Used to reference the original Order No for Cancel Replace
	long             PlaceShares;
	char             ADPCode[GIVEUP_LEN];
	double           CMEPriceRatio;
	BIGTRADEREC      BigTradeRec;
    MLCLData         mlCLData;
	unsigned long    Offset;
	tdPlaceLogItem8 *NextPlaceLogItem;
	char             DeliverToCompId[16];
	char             DeliverToSubId[16];
	unsigned long    OrderTime;
	void*            HoldList;
	void*            SkipList;
	char             PendingCancelNo[ORDERNO_LEN];		// Current cancel orderno
	char             PendingStatus;						// 0,'E','6','A','H','S'
	char             PreviousStatus;					// BTR.LastStatus before PendingStatus
	char             CxlRejected;						// 0,1
	char             CxlConfirmed;						// 0,1
	int              TargetConPort;
	char             ClientDomain[DOMAIN_LEN];
	bool             IsACancelPending;					// DT1763: Keep track of a pending (in the market) cancel for this item
	bool             IsAReplacePending;					// DT1763: Keep track of a pending (in the market) replace for this item
	char             NameValuePairs[256];   
} PLACELOGITEM8;

//PlaceLogItem14 matches version 14 in trader
typedef struct tdPlaceLogItem14
{
	unsigned char VerTag;               // Must be 0xFF
	unsigned char Version;              // Version number
	char ParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char ParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN]; // Domains NASDAQ giveup
	char ISICode[GIVEUP_LEN]; // Domains ISI Code or ADP TERMINAL ID chars 1&2
	char ClearingAccount[ACCOUNT_LEN]; // Clearing Account for REDI, open for the rest
	long GiveupOrderNo; // Auto Numbering for ECN's that does not use IqouteOrderNo
	long ResponceTimer;
	char ParserRefNo[ORDERNO_LEN64]; //ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	char ReplaceOrderNo[ORDERNO_LEN]; // Used to reference the original Order No for Cancel Replace
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	double CMEPriceRatio;
	BIGTRADEREC BigTradeRec;
	char extraData[EXTRADATA_LEN];
    long OrigGiveupOrderNo;
// Current EASE trader may not have random reserve yet
//	long RandomReserve;
	unsigned long Offset;
	tdPlaceLogItem14 *NextPlaceLogItem;
	LONGLONG eor;	//DT5902 End of record marker
} PLACELOGITEM14;

typedef struct tdCancelLogItem
{
	TRADEREC TradeRec;
	char OriginalParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char OriginalParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN];
	char ISICode[GIVEUP_LEN];
	char ClearingAccount[ACCOUNT_LEN];
	long OriginalGiveupOrderNo; 
	char CancelParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char CancelParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	long CancelGiveupOrderNo; 
	char ParserRefNo[ORDERNO_LEN64]; //ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	long ResponceTimer_Deleted;
	long CancelTimer_Deleted;
	long CancelCount;
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	unsigned long Offset;
	tdCancelLogItem *NextCancelLogItem;
} CANCELLOGITEM;

// We'll use this cancellogitem, but only send MSG833REC to trademon
// Big cancellog: shoud be 976 bytes
typedef struct tdCancelLogItem2
{
	BIGTRADEREC BigTradeRec;
	char OriginalParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char OriginalParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN];
	char ISICode[GIVEUP_LEN];
	char ClearingAccount[ACCOUNT_LEN];
	long OriginalGiveupOrderNo; 
	char CancelParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char CancelParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	long CancelGiveupOrderNo; 
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	long ResponseTimer_Deleted;
	long CancelTimer_Deleted;
	long CancelCount;
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	unsigned long Offset;
	tdCancelLogItem2 *NextCancelLogItem;
} CANCELLOGITEM2;

// We'll use this cancellogitem, but only send MSG833REC to trademon
// Big cancellog: shoud be 976 bytes
typedef struct tdCancelLogItem2ATE
{
	BIGTRADEREC BigTradeRec;
	char OriginalParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char OriginalParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN];
	char ISICode[GIVEUP_LEN];
	char ClearingAccount[ACCOUNT_LEN];
	long OriginalGiveupOrderNo; 
	char CancelParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char CancelParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	long CancelGiveupOrderNo; 
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	long ResponseTimer_Deleted;
	long CancelTimer_Deleted;
	long CancelCount;
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	unsigned long Offset;
	tdCancelLogItem2 *NextCancelLogItem;
	//char holdMsg[1024];
	char PendingStatus; // 0,'E','6','A','H','S'
	char CxlRejected; // 0,1
	int TargetConPort;
	char NameValuePairs[PLACELOG_NAMEVALUE_BUFFER_SIZE];   
} CANCELLOGITEM2ATE;

// We'll use this cancellogitem, but only send MSG833REC to trademon
// Big cancellog: shoud be 976 bytes
typedef struct tdCancelLogItem2IQOS
{
	BIGTRADEREC BigTradeRec;
	char OriginalParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char OriginalParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	char Giveup[GIVEUP_LEN];
	char ISICode[GIVEUP_LEN];
	char ClearingAccount[ACCOUNT_LEN];
	long OriginalGiveupOrderNo; 
	char CancelParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
	char CancelParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
	long CancelGiveupOrderNo; 
	char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
	long ResponseTimer_Deleted;
	long CancelTimer_Deleted;
	long CancelCount;
	long PlaceShares;
	char ADPCode[GIVEUP_LEN];
	unsigned long Offset;
	tdCancelLogItem2IQOS *NextCancelLogItem;
    //=================================================
    // Prameters for OrderHolding
	//char holdMsg[1024];
	char PendingStatus; // 0,'E','6','A','H','S'
	char CxlRejected; // 0,1
	int TargetConPort;
    //=================================================
	char NameValuePairs[PLACELOG_NAMEVALUE_BUFFER_SIZE];   

    char IQOSListId[IQOS_LIST_LEN];                     // IQOS ListID
    char IQOSGroupId[DOMAIN_LEN];
    int  IQOSNumOfParentOrder;                              // Total number of parent orders in a list
} CANCELLOGITEM2IQOS;

//Copied from "..\..\TraderLook\ReportLog.h"
// Trader
typedef struct tdReportLogItem1
{
	long ReportLogNo;
	long RejectReasonNo;
	char RejectReasonText[128]; 
	char ParserSeqNo[EXECID_LEN];
	int ReportBatch_deleted;
	TRADEREC TradeRec;
	TRADEREC OriginalTradeRec;
	tdReportLogItem1 *NextReportLogItem;
	tdReportLogItem1 *PrevReportLogItem;
} REPORTLOGITEM1;

// HV Trader
typedef struct tdReportLogItem2
{
	long ReportLogNo;
	long RejectReasonNo;
	char RejectReasonText[128]; // On confirm status, this is set to ParserOrderNo for CLSERVER
	char ParserSeqNo[EXECID_LEN];
	int ReportBatch_deleted;
	BIGTRADEREC BigTradeRec;
	BIGTRADEREC OriginalBigTradeRec;
	tdReportLogItem2 *NextReportLogItem;
	tdReportLogItem2 *PrevReportLogItem;
} REPORTLOGITEM2;

typedef struct tdReportLogItem2IQOS
{
	long ReportLogNo;
    long RejectReasonNo;
    char RejectReasonText[128];
    union
    {
        char ReportData[1800];
        struct
        {
            char ParserSeqNo[EXECID_LEN];
            int ReportBatch_deleted;
            BIGTRADEREC BigTradeRec;
            TRADEREC OriginalTradeRec;
        };
    };
    int  ReportLogType;

	tdReportLogItem2 *NextReportLogItem;
	tdReportLogItem2 *PrevReportLogItem;
} REPORTLOGITEM2IQOS;

// FIX Server
typedef struct tdReportLogItem3
{
	long ReportLogNo;
	long RejectReasonNo;
	char RejectReasonText[128];
	char ParserSeqNo[EXECID_LEN];
	int ReportBatch_deleted;
	BIGTRADEREC BigTradeRec;
	TRADEREC OriginalTradeRec;
	tdReportLogItem3 *NextReportLogItem;
	tdReportLogItem3 *PrevReportLogItem;
} REPORTLOGITEM3;

// HV Fix Server
typedef struct tdReportLogItem4
{
	long ReportLogNo;
	long RejectReasonNo;
	char RejectReasonText[128];
	BIGTRADEREC BigTradeRec;
} REPORTLOGITEM4;

typedef struct tdReportLogItem5
{
	long ReportLogNo;
	long RejectReasonNo;
	char RejectReasonText[128]; // On confirm status, this is set to ParserOrderNo for CLSERVER
	char ParserSeqNo[EXECID_LEN];
	int ReportBatch_deleted;
	BIGTRADEREC BigTradeRec;
	BIGTRADEREC OriginalBigTradeRec;
	char OptionalData[256];
	tdReportLogItem5 *NextReportLogItem;
	tdReportLogItem5 *PrevReportLogItem;
} REPORTLOGITEM5;

#pragma pack(pop)
};

static char IQExecType(char status)
{
	char etype=0;
	switch(status)
	{
	//#define TRADE_STOPPED		(-8) 	// Order stopped waiting on fill
	//#define WILL_LOCK			(-7) 	// Order will Lock Market
	//#define CANCEL_SENT			(-6) 	// Cancel Sent to server
	//#define CANCEL_RECEIVED		(-5) 	// Cancel Received to server
	//#define CANCEL_PLACED		(-4) 	// Cancel Placed at ECN
	//#define TRADE_SENT			(-3) 	// Sent to server
	//#define TRADE_RECEIVED		(-2)	// Received by server
	//#define TRADE_PLACED		(-1)	// Placed at Ecn
	case TRADE_CONFIRMED: etype='0'; break; // Confirmed by ECN
	case TRADE_PARTFILL: etype='1'; break; // Part filled
	case TRADE_FILLED: etype='2'; break; // Full Filled
	case TRADE_EXPIRED: etype='3'; break; // Expired - Done for the day
	case TRADE_CANCELED: etype='4'; break;// Full Canceled
	case 5: etype='5'; break;				// 5:Replaced,
	case TRADE_CANCELPENDING: etype='6'; break;// 6:Pending Cancel/Replace

	case 7: etype='7'; break; // 7:Stopped
	case TRADE_REJECTED: etype='8'; break; //8:Rejected
	//						// 9:Suspended, A:Pending New
	//						// B:Calculated, C:Expired
	case 100: // 100 : Cancel Rejected by ECN
	case 101: // 101 : Cancel Rejected by PARSER - OVER CANCEL
	case 102: // 102 : Cancel Rejected by PARSER - ORDER NOT CONFIRMED
	case 103: // 103 : Cancel Rejected by PARSER - ORDER NOT FOUND
	case 104: // 104 : Cancel Rejected by ECN - Cancel/Replace not allowed DT3347
		etype='9'; break;
	};
	return etype;
}
int ViewServer::ReadSmpThread(VSDetailFile *dfile)
{
	VSDistJournal *dj=(VSDistJournal *)UscPort[dfile->portno].DetPtr5;
	if(!dj)
		return -1;
	#ifndef NO_EFILLS_FILE
	if(!*ffpath)
		#ifdef WIN32
		sprintf(ffpath,"%s\\efills_%08d.fix",FILLS_FILE_DIR.c_str(),WSDate);
		#else
		sprintf(ffpath,"%s/efills_%08d.fix",FILLS_FILE_DIR.c_str(),WSDate);
		#endif
	#endif
	char OatsAppInstID[20]={0},OatsClientID[80]={0};
	int OatsFileDate=0;
	map<string,string> cltcmap;

#define READBLOCKSIZE (512*1024) // 512K tried 256K,1M,2M,4M; this is the best value
	// In order to maximize CPU on this thread, always have an overlapping read while we're processing
	OVERLAPPED ovls[2];
	char *rbufs[2];
	char *rbuf=new char[READBLOCKSIZE*2],*rptr=rbuf,*rbend=rbuf +READBLOCKSIZE*2;
	if(!rbuf)
	{
		_ASSERT(false);
		return -1;
	}
	memset(rbuf,0,READBLOCKSIZE*2);
	for(int i=0;i<2;i++)
	{
		memset(&ovls[i],0,sizeof(OVERLAPPED));
		rbufs[i]=new char[READBLOCKSIZE];
		if(!rbufs[i])
		{
			_ASSERT(false);
			return -1;
		}
		memset(rbufs[i],0,READBLOCKSIZE);
	}
	int pendidx=-1;
	dfile->LockRead();
	// Since we're double-buffering, 'rnext' doesn't always equal 'dfile->rend'
	LARGE_INTEGER rnext;
	rnext.QuadPart=dfile->rend.QuadPart;
	FixStats *ustats=(FixStats *)UscPort[dfile->portno].DetPtr3;
	if(!ustats)
	{
		_ASSERT(false);
		dfile->UnlockRead();
		return -1;
	}
	while(dfile->rhnd!=INVALID_FILE_VALUE)
	{
		dfile->UnlockRead();
		// 'rnext' is local so we don't need to lock read.
		// Mutex write against VSOrder::WriteDetailBlock.
		dfile->LockWrite();
		if(dfile->rend.QuadPart>=dfile->fend.QuadPart)
		{
			dfile->UnlockWrite();
			SleepEx(100,true);
			dfile->LockRead();
			continue;
		}
		// Only reference the end of write value while we have it locked
		LONGLONG ravail=dfile->fend.QuadPart -rnext.QuadPart;
		dfile->UnlockWrite();

		// Read a big chunk of details 
		// Mutex read against VSOrder::ReadDetail
		dfile->LockRead();
		OVERLAPPED *povl=0;
		// Read is already pending
		//char dbuf[1024]={0};
		DWORD rbytes=0;
		if(pendidx<0)
		{
			// Issue first read: don't exceed half of 'rbuf'
			LONGLONG rllen=ravail;
			DWORD rleft=(DWORD)(rptr -rbuf);
			if(rllen>(READBLOCKSIZE -rleft))
				rllen=(READBLOCKSIZE -rleft);
			DWORD rlen=(DWORD)rllen;
			ravail-=rlen;
			//sprintf(dbuf,"DEBUG: Read1[%I64d,%I64d), pend=%d\r\n",rnext.QuadPart,rnext.QuadPart +rlen,0);
			//OutputDebugString(dbuf);
			DWORD rbytes=0;
			pendidx=0;
			povl=&ovls[pendidx];
			povl->Offset=rnext.LowPart;
			povl->OffsetHigh=rnext.HighPart;
			rnext.QuadPart+=rlen;
			int rc=ReadFile(dfile->rhnd,rbufs[pendidx],rlen,&rbytes,povl);
			int err=GetLastError();
			if((rc<0)&&(err!=ERROR_IO_PENDING))
			{
				#ifdef WIN32
				WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen);
				#else
				WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen);
				#endif
				dfile->UnlockRead();
				break;
			}
			pendidx=0;
		}
		// Wait for pending read
		povl=&ovls[pendidx];
		rbytes=0;
		GetOverlappedResult(dfile->rhnd,povl,&rbytes,true);
		if(rbytes>0)
		{
			_ASSERT(rptr +rbytes<=rbend);
			memcpy(rptr,rbufs[pendidx],rbytes); rptr+=rbytes; 
		}
		// Issue next overlapped read while we're processing
		bool lastAvail=false;
		if(ravail>0)
		{
			LONGLONG rllen2=ravail;
			if(rllen2>READBLOCKSIZE)
				rllen2=READBLOCKSIZE;
			DWORD rlen2=(DWORD)rllen2;
			ravail-=rlen2;
			DWORD rbytes2=0;
			pendidx=(pendidx +1)%2;
			//sprintf(dbuf,"DEBUG: Read2[%I64d,%I64d), pend=%d\r\n",rnext.QuadPart,rnext.QuadPart +rlen2,pendidx);
			//OutputDebugString(dbuf);
			povl=&ovls[pendidx];
			povl->Offset=rnext.LowPart;
			povl->OffsetHigh=rnext.HighPart;
			rnext.QuadPart+=rlen2;
			int rc=ReadFile(dfile->rhnd,rbufs[pendidx],rlen2,&rbytes2,povl);
			int err=GetLastError();
			if((rc<0)&&(err!=ERROR_IO_PENDING))
			{
				#ifdef WIN32
				WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen2);
				#else
				WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen2);
				#endif
				dfile->UnlockRead();
				break;
			}
		}
		else
		{
			pendidx=-1; lastAvail=true;
		}
		dfile->UnlockRead(); // No need to lock the file while we're parsing

		// There will be multiple messages within the block
		const char *fend=rptr;
		const char *fptr;
		for(fptr=rbuf;fptr<fend;)
		{
			const char *fendl=0,*fstart=fptr;
			int flen=0;
			char AppInstID[20]={0};			// AppInstID(11505)
			char ClOrdID[40]={0};			// ClOrdID(11)
			char RootOrderID[40]={0};		// tag 70129
			char FirstClOrdID[40]={0};		// tag 5055
			char ClParentOrderID[40]={0};	// tag 5035
			char Symbol[32]={0};			// Symbol(55,65)
			double Price=0.0;
			char Side=0;
			char SymSfx[12]={0};			// Symbol(55,65)
			char Account[20]={0};			// Account(1)
			char EcnOrderID[40]={0};		// EcnOrderID(37)
			char ClientID[24]={0};			// ClientID(109)
			char Connection[48]={0};		// Connection
			char AuxKey[AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN]={0}; // AuxKeys  DT9398
			char mtype=0,etype=0;
			int ordQty=0,cumQty=0,leavesQty=0,fillQty=0;
			LONGLONG TimeStamp=0;
			int OrderDate=0;
			const char *OrigOrdID="",*ExecID="";
			APPINSTMARKER *uaim=(APPINSTMARKER*)UscPort[dfile->portno].DetPtr6;

			if((dfile->proto==PROTO_FIXS_PLACELOG)||(dfile->proto==PROTO_TRADER_PLACELOG))
			{
				//if((int)(fend -fptr)<sizeof(PLACELOGITEM1)) // smallest item
				//{
				//	if(lastAvail)
				//		SleepEx(100,true); // prevent max CPU when there's no more to read
				//	break; // Read more from file
				//}
				APPINSTMARKER *aim=(APPINSTMARKER*)fptr;
				//PLACELOGITEM1	*pli1 = (PLACELOGITEM1 *)fptr;
				//PLACELOGITEM3	*pli3 = (PLACELOGITEM3 *)fptr;
				//PLACELOGITEM4	*pli4 = (PLACELOGITEM4 *)fptr;
				//PLACELOGITEM5	*pli5 = (PLACELOGITEM5 *)fptr;
				//PLACELOGITEM6	*pli6 = (PLACELOGITEM6 *)fptr;
				//PLACELOGITEM7	*pli7 = (PLACELOGITEM7 *)fptr;
				//PLACELOGITEM8	*pli8 = (PLACELOGITEM8 *)fptr;				// DT5143 RCN 20090324
				//PLACELOGITEM14	*pli14 = (PLACELOGITEM14 *)fptr;
				//PLACELOGITEM3IQOS *pliqos = (PLACELOGITEM3IQOS *)fptr;

				int avail=(int)(fend -fptr);
				flen=0;
				if(avail<sizeof(APPINSTMARKER))
					flen=sizeof(APPINSTMARKER);
				else if(aim->marker==0xFEFEFEFE)
				{
					flen=sizeof(APPINSTMARKER);
					if(!uaim)
					{
						uaim=new APPINSTMARKER;
						UscPort[dfile->portno].DetPtr6=uaim;
					}
					memcpy(uaim,aim,sizeof(APPINSTMARKER));
				}
				// The Traderlook method doesn't work for stream processing
				//if(aim->marker==0xFEFEFEFE)
				//{
				//	flen=sizeof(APPINSTMARKER);
				//	if(!uaim)
				//	{
				//		uaim=new APPINSTMARKER;
				//		memcpy(uaim,aim,sizeof(APPINSTMARKER));
				//		UscPort[dfile->portno].DetPtr6=uaim;
				//	}
				//}
				else if(uaim)
				{
					// Parent and child FIX server
					if(uaim->buildDate==2041)
						flen=sizeof(PLACELOGITEM8);
					// NyseCCG Trader
					else if(uaim->buildDate==3089)
						flen=sizeof(PLACELOGITEM14);
					// MLALGO Trader
					else if(uaim->buildDate==2030)
						flen=sizeof(PLACELOGITEM7);
					// IQOS FIX server and trader
					else if((uaim->buildDate==1005)||(uaim->buildDate==3119))
						flen=sizeof(PLACELOGITEM3IQOS);
				}
				// Not enough available or we don't recognize the build version
				if((flen<1)||(avail<flen))
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}

				_ASSERT(flen>0);
				//if ((pli14->VerTag == 0xFF) && (pli14->Version == 14 || pli14->Version == 15))
				// NYSE CCG Traders
				if(flen==sizeof(PLACELOGITEM14))
				{
					PLACELOGITEM14	*pli14 = (PLACELOGITEM14 *)fptr;
					//flen=sizeof(PLACELOGITEM14);
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,pli14->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",pli14->BigTradeRec.TradeRec.Domain,pli14->BigTradeRec.TradeRec.OrderNo);
					strncpy(Symbol,pli14->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=pli14->BigTradeRec.TradeRec.Price;
					switch(pli14->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,pli14->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					if(pli14->BigTradeRec.TradeRecB.AltAccount1[0])
						strncpy(ClientID,pli14->BigTradeRec.TradeRecB.AltAccount1,sizeof(ClientID) -1);
					else
						strncpy(ClientID,pli14->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					strncpy(EcnOrderID,pli14->BigTradeRec.TradeRec.ECNOrderNo,sizeof(EcnOrderID) -1);
					etype=IQExecType(pli14->BigTradeRec.TradeRec.LastStatus);
					//if((etype=='0')||(etype=='5'))
					//	mtype='8';
					//else
						mtype=pli14->ReplaceOrderNo[0] ?'G' :'D';
					ordQty=pli14->PlaceShares;
					cumQty=pli14->BigTradeRec.TradeRec.ExeShares;
					leavesQty=pli14->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(pli14->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(pli14->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pli14->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pli14->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=pli14->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",pli14->BigTradeRec.TradeRec.User,pli14->BigTradeRec.TradeRec.Domain);
					//if(pli14->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",pli14->ParserOrderNo,pli14->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",pli14->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						char OrigClOrdID[40]={0};
						sprintf(OrigClOrdID,"%s-%s",pli14->BigTradeRec.TradeRec.Domain,pli14->ReplaceOrderNo);
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				//else if ((pli8->VerTag == 0xFF) && (pli8->Version == 101) && (isprint(pliqos->IQOSGroupId[0])))
				else if(flen==sizeof(PLACELOGITEM3IQOS))
				{
					_ASSERT(false); // TODO: Untested
					PLACELOGITEM3IQOS *pliqos = (PLACELOGITEM3IQOS *)fptr;
					//flen=sizeof(PLACELOGITEM3IQOS);
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,pliqos->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",pliqos->BigTradeRec.TradeRec.Domain,pliqos->BigTradeRec.TradeRec.OrderNo);
					//strncpy(ClParentOrderID,pliqos->IQOSListId,sizeof(ClParentOrderID) -1);
					strncpy(ClParentOrderID,pliqos->ParserOrderNo,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,pliqos->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=pliqos->BigTradeRec.TradeRec.Price;
					switch(pliqos->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,pliqos->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					_snprintf(EcnOrderID,sizeof(EcnOrderID) -1,"%s-%s",pliqos->BigTradeRec.TradeRec.Domain,pliqos->BigTradeRec.TradeRec.OrderNo);
					if(pliqos->IQOSGroupId[0])
						strncpy(ClientID,pliqos->IQOSGroupId,sizeof(ClientID) -1);
					else if(pliqos->BigTradeRec.TradeRecB.AltAccount1[0])
						strncpy(ClientID,pliqos->BigTradeRec.TradeRecB.AltAccount1,sizeof(ClientID) -1);
					else
						strncpy(ClientID,pliqos->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					etype=IQExecType(pliqos->BigTradeRec.TradeRec.LastStatus);
					if((etype=='0')||(etype=='5'))
						mtype='8';
					else
						mtype=pliqos->ReplaceOrderNo[0] ?'G' :'D';
					ordQty=pliqos->PlaceShares;
					cumQty=pliqos->BigTradeRec.TradeRec.ExeShares;
					leavesQty=pliqos->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(pliqos->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(pliqos->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pliqos->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pliqos->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=pliqos->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",pliqos->BigTradeRec.TradeRec.User,pliqos->BigTradeRec.TradeRec.Domain);
					//if(pliqos->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",pliqos->ParserOrderNo,pliqos->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",pliqos->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,pliqos->ParserOrderNo,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(pliqos->ParserOrderNo);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						const char *OrigClOrdID=pliqos->ReplaceOrderNo;
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(pliqos->ParserOrderNo,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				//else if ((pli8->VerTag == 0xFF) && (pli8->Version == 101))		// DT5143 RCN 20090324
				// MLALGO FIX Servers
				else if(flen==sizeof(PLACELOGITEM8))
				{
					PLACELOGITEM8	*pli8 = (PLACELOGITEM8 *)fptr;				// DT5143 RCN 20090324
					//flen=sizeof(PLACELOGITEM8);
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,pli8->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",pli8->BigTradeRec.TradeRec.Domain,pli8->BigTradeRec.TradeRec.OrderNo);
					strncpy(Symbol,pli8->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=pli8->BigTradeRec.TradeRec.Price;
					switch(pli8->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,pli8->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					if(pli8->BigTradeRec.TradeRecB.AltAccount1[0])
						strncpy(ClientID,pli8->BigTradeRec.TradeRecB.AltAccount1,sizeof(ClientID) -1);
					else
						strncpy(ClientID,pli8->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					_snprintf(EcnOrderID,sizeof(EcnOrderID) -1,"%s-%s",pli8->BigTradeRec.TradeRec.Domain,pli8->BigTradeRec.TradeRec.OrderNo);
					etype=IQExecType(pli8->BigTradeRec.TradeRec.LastStatus);
					//if((etype=='0')||(etype=='5'))
					//	mtype='8';
					//else
						mtype=pli8->ReplaceOrderNo[0] ?'G' :'D';
					ordQty=pli8->PlaceShares;
					cumQty=pli8->BigTradeRec.TradeRec.ExeShares;
					leavesQty=pli8->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(pli8->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(pli8->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pli8->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pli8->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=pli8->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",pli8->BigTradeRec.TradeRec.User,pli8->BigTradeRec.TradeRec.Domain);
					//if(pli8->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",pli8->ParserOrderNo,pli8->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",pli8->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,pli8->ParserOrderNo,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(pli8->ParserOrderNo);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						const char *OrigClOrdID=pli8->ReplaceOrderNo;
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(pli8->ParserOrderNo,FirstClOrdID));
					}

					// Child FIX server
					const char *t2005=strstr(pli8->NameValuePairs,"2005=");
					if(t2005) t2005=strchr(t2005+5,'%');
					if(t2005) t2005=strchr(t2005+1,'%');
					if(t2005) t2005=strchr(t2005+1,'%');
					if(t2005)
					{
						t2005++;
						const char *eptr=strchr(t2005,'|');
						if(!eptr) eptr=t2005 +strlen(t2005);
						// The child FIX server's 2005 tag is only suitable for RootOrderID key.
						// It does not contain the proper value for FirstClOrdID
						strncpy(RootOrderID,t2005,(int)(eptr -t2005));
						strncpy(ClParentOrderID,pli8->ParserOrderNo,sizeof(ClParentOrderID) -1);
					}
					// Parent FIX server
					else
					{
						strncpy(RootOrderID,FirstClOrdID,sizeof(RootOrderID) -1);
						//strncpy(ClParentOrderID,pli8->BigTradeRec.TradeRecB.ClientOriginalOrderId,sizeof(ClParentOrderID) -1);
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype/*etype=='0'?etype:0*/,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					// Only do this if we don't have reportlog
					//// Separate journal entry to update the status
					//AddJournal(dj,fptr,flen,doffset,dend,
					//			AppInstID,ClOrdID,RootOrderID,
					//			FirstClOrdID,ClParentOrderID,Symbol,
					//			Price,Side,
					//			Account,EcnOrderID,ClientID,
					//			Connection,AuxKey,
					//			'8',etype,ordQty,cumQty,leavesQty,
					//			TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// MLALGO Traders
				//else if ((pli7->VerTag == 0xFF) && (pli7->Version == 12))
				else if(flen==sizeof(PLACELOGITEM7))
				{
					PLACELOGITEM7	*pli7 = (PLACELOGITEM7 *)fptr;
					//flen=sizeof(PLACELOGITEM7);
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,pli7->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",pli7->BigTradeRec.TradeRec.Domain,pli7->BigTradeRec.TradeRec.OrderNo);
					strncpy(Symbol,pli7->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=pli7->BigTradeRec.TradeRec.Price;
					switch(pli7->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,pli7->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					if(pli7->BigTradeRec.TradeRecB.AltAccount1[0])
						strncpy(ClientID,pli7->BigTradeRec.TradeRecB.AltAccount1,sizeof(ClientID) -1);
					else
						strncpy(ClientID,pli7->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					strncpy(EcnOrderID,pli7->BigTradeRec.TradeRec.ECNOrderNo,sizeof(EcnOrderID) -1);
					etype=IQExecType(pli7->BigTradeRec.TradeRec.LastStatus);
					//if((etype=='0')||(etype=='5'))
					//	mtype='8';
					//else
						mtype=pli7->ReplaceOrderNo[0] ?'G' :'D';
					ordQty=pli7->PlaceShares;
					cumQty=pli7->BigTradeRec.TradeRec.ExeShares;
					leavesQty=pli7->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(pli7->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(pli7->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pli7->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pli7->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=pli7->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",pli7->BigTradeRec.TradeRec.User,pli7->BigTradeRec.TradeRec.Domain);
					//if(pli7->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",pli7->ParserOrderNo,pli7->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",pli7->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						char OrigClOrdID[40]={0};
						sprintf(OrigClOrdID,"%s-%s",pli7->BigTradeRec.TradeRec.Domain,pli7->ReplaceOrderNo);
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				//else if ((pli6->VerTag == 0xFF) && (pli6->Version == 100))
				else if(flen==sizeof(PLACELOGITEM6))
				{
					PLACELOGITEM6	*pli6 = (PLACELOGITEM6 *)fptr;
					//flen=sizeof(PLACELOGITEM6);
					_ASSERT(false); // TODO: Untested
				}
				//else if ((pli5->VerTag == 0xFF) && (pli5->Version == 5))
				else if(flen==sizeof(PLACELOGITEM5))
				{
					PLACELOGITEM5	*pli5 = (PLACELOGITEM5 *)fptr;
					//flen=sizeof(PLACELOGITEM5);
					_ASSERT(false); // TODO: Untested
				}
				//else if ((pli4->VerTag == 0xFF) && (pli4->Version == 4))
				else if(flen==sizeof(PLACELOGITEM4))
				{
					PLACELOGITEM4	*pli4 = (PLACELOGITEM4 *)fptr;
					//flen=sizeof(PLACELOGITEM4);
					_ASSERT(false); // TODO: Untested
				}
				//else if ((pli3->VerTag == 0xFF) && (pli3->Version == 3))
				else if(flen==sizeof(PLACELOGITEM3))
				{
					PLACELOGITEM3	*pli3 = (PLACELOGITEM3 *)fptr;
					//// Some PLACELOGITEM3 files have order timings in the struct also
					//if((f->GetLength() % (sizeof(PLACELOGITEM3)+8) == 0) &&   // Order timings - sizeof(unsigned long) + packing (4 bytes)
					//	(f->GetLength() % sizeof(PLACELOGITEM3) == 0))
					//{
					//	// Evenly divisible by both sizes, cannot tell from the file size, read the next item
					//	f->Read(&buffer, sizeof(buffer));
					//	pli3 = (PLACELOGITEM3 *)buffer;
					//	if(isprint(pli3->ParserOrderNo[0]))
					//	{
					//		flen=sizeof(PLACELOGITEM3);
					//	}
					//	else
					//	{
					//		flen=sizeof(PLACELOGITEM3)+8;
					//	}
					//}
					//else if(f->GetLength() % (sizeof(PLACELOGITEM3)+8) == 0)
					//{
					//	flen=sizeof(PLACELOGITEM3)+8;
					//}
					//else
					//{
						//flen=sizeof(PLACELOGITEM3);
						_ASSERT(false); // TODO: Untested
					//}
				}
				//else if (isprint(pli1->TradeRec.User[0]))
				else if(flen==sizeof(PLACELOGITEM1))
				{
					PLACELOGITEM1	*pli1 = (PLACELOGITEM1 *)fptr;
					//flen=sizeof(PLACELOGITEM1);
					_ASSERT(false); // TODO: Untested
				}
				else if(flen==sizeof(PLACELOGITEM2))
				{
					PLACELOGITEM2	*pli2 = (PLACELOGITEM2 *)fptr;
					//flen=sizeof(PLACELOGITEM2);
					_ASSERT(false); // TODO: Untested
				}
				fendl=fptr +flen;

				fptr=fendl;
				_ASSERT(fptr<=fend);
			}
			else if((dfile->proto==PROTO_FIXS_CXLLOG)||(dfile->proto==PROTO_TRADER_CXLLOG))
			{
				APPINSTMARKER *aim=(APPINSTMARKER*)fptr;
				//CANCELLOGITEM *cli1=(CANCELLOGITEM *)fptr;
				//CANCELLOGITEM2 *cli2=(CANCELLOGITEM2 *)fptr;
				//CANCELLOGITEM2ATE *cli2ate=(CANCELLOGITEM2ATE *)fptr;
				//CANCELLOGITEM2IQOS *cli2iqos=(CANCELLOGITEM2IQOS *)fptr;

				int avail=(int)(fend -fptr);
				flen=0;
				if(avail<sizeof(APPINSTMARKER))
					flen=sizeof(APPINSTMARKER);
				else if(aim->marker==0xFEFEFEFE)
				{
					flen=sizeof(APPINSTMARKER);
					if(!uaim)
					{
						uaim=new APPINSTMARKER;
						UscPort[dfile->portno].DetPtr6=uaim;
					}
					memcpy(uaim,aim,sizeof(APPINSTMARKER));
				}
				// The Traderlook method doesn't work for stream processing
				//flen=(int)(fend -fptr);
				//if(aim->marker==0xFEFEFEFE)
				//	flen=sizeof(APPINSTMARKER);
				//else if((flen>=sizeof(CANCELLOGITEM2IQOS))&&(isprint(cli2iqos->IQOSGroupId[0])))
				//	flen=sizeof(CANCELLOGITEM2IQOS);
				//else if((flen>=sizeof(CANCELLOGITEM2ATE)&&((cli2ate->PendingStatus!=0)||(cli2ate->NameValuePairs[0]))))
				//	flen=sizeof(CANCELLOGITEM2ATE);
				//else if((flen>=sizeof(CANCELLOGITEM2)&&(isprint(cli2->CancelParserOrderNo[0]))))
				//	flen=sizeof(CANCELLOGITEM2);
				//else if(flen>=sizeof(CANCELLOGITEM))
				//	flen=sizeof(CANCELLOGITEM);
				else if(uaim)
				{
					// Parent and child FIX server
					if(uaim->buildDate==2041)
						flen=sizeof(CANCELLOGITEM2ATE);
					// NyseCCG Trader
					else if(uaim->buildDate==3089)
						flen=sizeof(CANCELLOGITEM);
					// MLALGO Trader
					else if(uaim->buildDate==2030)
						flen=sizeof(CANCELLOGITEM);
					// IQOS FIX server and trader
					else if((uaim->buildDate==1005)||(uaim->buildDate==3119))
						flen=sizeof(CANCELLOGITEM2IQOS);
				}
				// Not enough available or we don't recognize the build version
				if((flen<1)||(avail<flen))
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}

				_ASSERT(flen>0);
				// MLGALGOTraders
				if(flen==sizeof(CANCELLOGITEM))
				{
					CANCELLOGITEM *cli1=(CANCELLOGITEM *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;
					// Additional CANCELLOGITEM members
					//char OriginalParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
					//char OriginalParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
					//char Giveup[GIVEUP_LEN];
					//char ISICode[GIVEUP_LEN];
					//char ClearingAccount[ACCOUNT_LEN];
					//long OriginalGiveupOrderNo; 
					//char CancelParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
					//char CancelParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
					//long CancelGiveupOrderNo; 
					//char ParserRefNo[ORDERNO_LEN64]; //ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
					//long ResponceTimer_Deleted;
					//long CancelTimer_Deleted;
					//long CancelCount;
					//long PlaceShares;
					//char ADPCode[GIVEUP_LEN];
					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,cli1->TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",cli1->TradeRec.Domain,cli1->TradeRec.OrderNo);
					//strncpy(ClParentOrderID,cli1->CancelParserOrderNo,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,cli1->TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=cli1->TradeRec.Price;
					switch(cli1->TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,cli1->TradeRec.Account,sizeof(Account) -1);
					strncpy(EcnOrderID,cli1->TradeRec.ECNOrderNo,sizeof(EcnOrderID) -1);
					etype=IQExecType(cli1->TradeRec.LastStatus);
					//if(etype=='4')
					//	mtype='8';
					//else
						mtype='F';
					ordQty=cli1->PlaceShares;
					cumQty=cli1->TradeRec.ExeShares;
					leavesQty=cli1->TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(cli1->TradeRec.LastDate)*1000000 
						+(atoi(cli1->TradeRec.LastTime)*10000 +atoi(cli1->TradeRec.LastTime +3)*100 +atoi(cli1->TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					OrigOrdID=cli1->OriginalParserOrderNo;
					ExecID=cli1->TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",cli1->TradeRec.User,cli1->TradeRec.Domain);
					//if(cli1->TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",cli1->CancelParserOrderNo,cli1->TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",cli1->TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						//const char *OrigClOrdID=cli1->OriginalParserOrderNo;
						//if(*OrigClOrdID)
						//{
						//	fit=fm->find(OrigClOrdID);
						//	if(fit!=fm->end())
						//		strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						//}
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				else if(flen==sizeof(CANCELLOGITEM2))
				{
					_ASSERT(false); // TODO: Untested
					CANCELLOGITEM2 *cli2=(CANCELLOGITEM2 *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;
					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,cli2->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",cli2->BigTradeRec.TradeRec.Domain,cli2->BigTradeRec.TradeRec.OrderNo);
					strncpy(ClParentOrderID,cli2->CancelParserOrderNo,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,cli2->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=cli2->BigTradeRec.TradeRec.Price;
					switch(cli2->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,cli2->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					_snprintf(EcnOrderID,sizeof(EcnOrderID) -1,"%s-%s",cli2->BigTradeRec.TradeRec.Domain,cli2->BigTradeRec.TradeRec.OrderNo);
					etype=IQExecType(cli2->BigTradeRec.TradeRec.LastStatus);
					if(etype=='4')
						mtype='8';
					else
						mtype='F';
					ordQty=cli2->PlaceShares;
					cumQty=cli2->BigTradeRec.TradeRec.ExeShares;
					leavesQty=cli2->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(cli2->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(cli2->BigTradeRec.TradeRec.LastTime)*10000 +atoi(cli2->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(cli2->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					OrigOrdID=cli2->OriginalParserOrderNo;
					ExecID=cli2->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",cli2->BigTradeRec.TradeRec.User,cli2->BigTradeRec.TradeRec.Domain);
					//if(cli2->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",cli2->CancelParserOrderNo,cli2->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",cli2->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						const char *OrigClOrdID=cli2->OriginalParserOrderNo;
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// MLALGO FIX Servers
				else if(flen==sizeof(CANCELLOGITEM2ATE))
				{
					CANCELLOGITEM2ATE *cli2ate=(CANCELLOGITEM2ATE *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;
					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,cli2ate->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",cli2ate->BigTradeRec.TradeRec.Domain,cli2ate->BigTradeRec.TradeRec.OrderNo);
					strncpy(Symbol,cli2ate->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=cli2ate->BigTradeRec.TradeRec.Price;
					switch(cli2ate->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,cli2ate->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					if(cli2ate->BigTradeRec.TradeRecB.AltAccount1[0])
						strncpy(ClientID,cli2ate->BigTradeRec.TradeRecB.AltAccount1,sizeof(ClientID) -1);
					else
						strncpy(ClientID,cli2ate->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					_snprintf(EcnOrderID,sizeof(EcnOrderID) -1,"%s-%s",cli2ate->BigTradeRec.TradeRec.Domain,cli2ate->BigTradeRec.TradeRec.OrderNo);
					etype=IQExecType(cli2ate->BigTradeRec.TradeRec.LastStatus);
					if(etype=='4')
						mtype='8';
					else
						mtype='F';
					ordQty=cli2ate->PlaceShares;
					cumQty=cli2ate->BigTradeRec.TradeRec.ExeShares;
					leavesQty=cli2ate->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(cli2ate->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(cli2ate->BigTradeRec.TradeRec.LastTime)*10000 +atoi(cli2ate->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(cli2ate->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					OrigOrdID=cli2ate->OriginalParserOrderNo;
					ExecID=cli2ate->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",cli2ate->BigTradeRec.TradeRec.User,cli2ate->BigTradeRec.TradeRec.Domain);
					//if(cli2ate->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",cli2ate->CancelParserOrderNo,cli2ate->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",cli2ate->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,cli2ate->CancelParserOrderNo,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(cli2ate->CancelParserOrderNo);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						const char *OrigClOrdID=cli2ate->OriginalParserOrderNo;
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
							else
								strncpy(FirstClOrdID,OrigClOrdID,sizeof(FirstClOrdID)-1);
							fm->insert(pair<string,string>(OrigClOrdID,FirstClOrdID));
						}
						fm->insert(pair<string,string>(cli2ate->CancelParserOrderNo,FirstClOrdID));
					}

					// Child FIX server
					const char *t2005=strstr(cli2ate->NameValuePairs,"2005=");
					if(t2005) t2005=strchr(t2005+5,'%');
					if(t2005) t2005=strchr(t2005+1,'%');
					if(t2005) t2005=strchr(t2005+1,'%');
					if(t2005)
					{
						t2005++;
						const char *eptr=strchr(t2005,'|');
						if(!eptr) eptr=t2005 +strlen(t2005);
						// The child FIX server's 2005 tag is only suitable for RootOrderID key.
						// It does not contain the proper value for FirstClOrdID
						strncpy(RootOrderID,t2005,(int)(eptr -t2005));
						strncpy(ClParentOrderID,cli2ate->CancelParserOrderNo,sizeof(ClParentOrderID) -1);
					}
					// Parent FIX server
					else
					{
						strncpy(RootOrderID,FirstClOrdID,sizeof(RootOrderID) -1);
						strncpy(ClParentOrderID,cli2ate->BigTradeRec.TradeRecB.ClientOriginalOrderId,sizeof(ClParentOrderID) -1);
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				else if(flen==sizeof(CANCELLOGITEM2IQOS))
				{
					_ASSERT(false); // TODO: Untested
					CANCELLOGITEM2IQOS *cli2iqos=(CANCELLOGITEM2IQOS *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;
					// Additional CANCELLOGITEM2IQOS members
					//char OriginalParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
					//char OriginalParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
					//char Giveup[GIVEUP_LEN];
					//char ISICode[GIVEUP_LEN];
					//char ClearingAccount[ACCOUNT_LEN];
					//long OriginalGiveupOrderNo; 
					//char CancelParserOrderNo[ORDERNO_LEN]; // Used when Order was placed
					//char CancelParserOrderNoGiveup[GIVEUP_LEN]; // GiveUp used with above OrderNo
					//long CancelGiveupOrderNo; 
					//char ParserRefNo[ORDERNO_LEN]; // Used to send Cancels , received from ECN at confirm
					//long ResponseTimer_Deleted;
					//long CancelTimer_Deleted;
					//long CancelCount;
					//long PlaceShares;
					//char ADPCode[GIVEUP_LEN];
					//unsigned long Offset;
					//tdCancelLogItem2IQOS *NextCancelLogItem;
					////=================================================
					//// Prameters for OrderHolding
					////char holdMsg[1024];
					//char PendingStatus; // 0,'E','6','A','H','S'
					//char CxlRejected; // 0,1
					//int TargetConPort;
					////=================================================
					//char NameValuePairs[PLACELOG_NAMEVALUE_BUFFER_SIZE];   

					//char IQOSListId[IQOS_LIST_LEN];                     // IQOS ListID
					//char IQOSGroupId[DOMAIN_LEN];
					//int  IQOSNumOfParentOrder;                              // Total number of parent orders in a list
					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,cli2iqos->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",cli2iqos->BigTradeRec.TradeRec.Domain,cli2iqos->BigTradeRec.TradeRec.OrderNo);
					//strncpy(ClParentOrderID,cli2iqos->IQOSListId,sizeof(ClParentOrderID) -1);
					strncpy(ClParentOrderID,cli2iqos->CancelParserOrderNo,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,cli2iqos->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=cli2iqos->BigTradeRec.TradeRec.Price;
					switch(cli2iqos->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,cli2iqos->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					_snprintf(EcnOrderID,sizeof(EcnOrderID) -1,"%s-%s",cli2iqos->BigTradeRec.TradeRec.Domain,cli2iqos->BigTradeRec.TradeRec.OrderNo);
					strncpy(ClientID,cli2iqos->IQOSGroupId,sizeof(ClientID) -1);
					etype=IQExecType(cli2iqos->BigTradeRec.TradeRec.LastStatus);
					if(etype=='4')
						mtype='8';
					else
						mtype='F';
					ordQty=cli2iqos->PlaceShares;
					cumQty=cli2iqos->BigTradeRec.TradeRec.ExeShares;
					leavesQty=cli2iqos->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(cli2iqos->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(cli2iqos->BigTradeRec.TradeRec.LastTime)*10000 +atoi(cli2iqos->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(cli2iqos->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					OrigOrdID=cli2iqos->OriginalParserOrderNo;
					ExecID=cli2iqos->BigTradeRec.TradeRec.ECNExecID;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",cli2iqos->BigTradeRec.TradeRec.User,cli2iqos->BigTradeRec.TradeRec.Domain);
					//if(cli2iqos->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",cli2iqos->CancelParserOrderNo,cli2iqos->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",cli2iqos->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						const char *OrigClOrdID=cli2iqos->OriginalParserOrderNo;
						if(*OrigClOrdID)
						{
							fit=fm->find(OrigClOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				fendl=fptr +flen;

				fptr=fendl;
				_ASSERT(fptr<=fend);
			}
			else if((dfile->proto==PROTO_FIXS_RPTLOG)||(dfile->proto==PROTO_TRADER_RPTLOG))
			{
				APPINSTMARKER *aim=(APPINSTMARKER*)fptr;
				//REPORTLOGITEM1 *rli1=(REPORTLOGITEM1 *)fptr;
				//REPORTLOGITEM2 *rli2=(REPORTLOGITEM2 *)fptr;
				//REPORTLOGITEM2IQOS *rli2iqos=(REPORTLOGITEM2IQOS *)fptr;
				//REPORTLOGITEM3 *rli3=(REPORTLOGITEM3 *)fptr;
				//REPORTLOGITEM4 *rli4=(REPORTLOGITEM4 *)fptr;
				//REPORTLOGITEM5 *rli5=(REPORTLOGITEM5 *)fptr;

				int avail=(int)(fend -fptr);
				flen=0;
				if(avail<sizeof(APPINSTMARKER))
					flen=sizeof(APPINSTMARKER);
				else if(aim->marker==0xFEFEFEFE)
				{
					flen=sizeof(APPINSTMARKER);
					if(!uaim)
					{
						uaim=new APPINSTMARKER;
						UscPort[dfile->portno].DetPtr6=uaim;
					}
					memcpy(uaim,aim,sizeof(APPINSTMARKER));
				}
				// The Traderlook method doesn't work for stream processing
				//else if(flen>sizeof(REPORTLOGITEM4)) // Smallest reportlog
				//{
				//	REPORTLOGITEM1 *nli1=(REPORTLOGITEM1 *)(fptr +sizeof(REPORTLOGITEM1));
				//	REPORTLOGITEM2 *nli2=(REPORTLOGITEM2 *)(fptr +sizeof(REPORTLOGITEM2));
				//	REPORTLOGITEM2IQOS *nli2iqos=(REPORTLOGITEM2IQOS *)(fptr +sizeof(REPORTLOGITEM2IQOS));
				//	REPORTLOGITEM3 *nli3=(REPORTLOGITEM3 *)(fptr +sizeof(REPORTLOGITEM3));
				//	REPORTLOGITEM4 *nli4=(REPORTLOGITEM4 *)(fptr +sizeof(REPORTLOGITEM4));
				//	REPORTLOGITEM5 *nli5=(REPORTLOGITEM5 *)(fptr +sizeof(REPORTLOGITEM5));
				//	if((flen==sizeof(REPORTLOGITEM1))||(nli1->ReportLogNo==rli1->ReportLogNo +1))
				//		flen=sizeof(REPORTLOGITEM1);
				//	else if((flen==sizeof(REPORTLOGITEM2))||(nli2->ReportLogNo==rli2->ReportLogNo +1))
				//		flen=sizeof(REPORTLOGITEM2);
				//	else if((flen==sizeof(REPORTLOGITEM2IQOS))||(nli2iqos->ReportLogNo==rli2iqos->ReportLogNo +1))
				//		flen=sizeof(REPORTLOGITEM2IQOS);
				//	else if((flen==sizeof(REPORTLOGITEM3))||(nli3->ReportLogNo==rli3->ReportLogNo +1))
				//		flen=sizeof(REPORTLOGITEM3);
				//	else if((flen==sizeof(REPORTLOGITEM4))||(nli4->ReportLogNo==rli4->ReportLogNo +1))
				//		flen=sizeof(REPORTLOGITEM4);
				//	else if((flen==sizeof(REPORTLOGITEM5))||(nli5->ReportLogNo==rli5->ReportLogNo +1))
				//		flen=sizeof(REPORTLOGITEM5);
				//}
				else if(uaim)
				{
					// Parent and child FIX server
					if(uaim->buildDate==2041)
						flen=sizeof(REPORTLOGITEM3);
					// NyseCCG Trader
					else if(uaim->buildDate==3089)
						flen=sizeof(REPORTLOGITEM1);
					// MLALGO Trader
					else if(uaim->buildDate==2030)
						flen=sizeof(REPORTLOGITEM5);
					// IQOS FIX server and trader
					else if((uaim->buildDate==1005)||(uaim->buildDate==3119))
						flen=sizeof(REPORTLOGITEM2IQOS);
				}
				// Not enough available or we don't recognize the build version
				if((flen<1)||(avail<flen))
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}

				_ASSERT(flen>0);
				// NYSE CCG Traders
				if(flen==sizeof(REPORTLOGITEM1))
				{
					REPORTLOGITEM1 *rli1=(REPORTLOGITEM1 *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,rli1->TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",rli1->TradeRec.Domain,rli1->TradeRec.OrderNo);
					//strncpy(ClParentOrderID,rli1->IQOSListId,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,rli1->TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=rli1->TradeRec.LastPrice;
					switch(rli1->TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,rli1->TradeRec.Account,sizeof(Account) -1);
					strncpy(ClientID,rli1->TradeRec.User,sizeof(ClientID) -1);
					strncpy(ClientID,rli1->TradeRec.User,sizeof(ClientID) -1);
					strncpy(EcnOrderID,rli1->TradeRec.ECNOrderNo,sizeof(EcnOrderID) -1);
					mtype='8';
					etype=IQExecType(rli1->TradeRec.LastStatus);
					ordQty=rli1->TradeRec.Shares;
					cumQty=rli1->TradeRec.ExeShares;
					leavesQty=rli1->TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(rli1->TradeRec.LastDate)*1000000 
						+(atoi(rli1->TradeRec.LastTime)*10000 +atoi(rli1->TradeRec.LastTime +3)*100 +atoi(rli1->TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=rli1->ParserSeqNo;
					fillQty=rli1->TradeRec.LastShares;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",rli1->TradeRec.User,rli1->TradeRec.Domain);
					//if(rli1->TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",rli1->TradeRec.ECNOrderNo,rli1->TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",rli1->TradeRec.ECNOrderNo,AUX_KEYS_DELIM);
					//strncpy(ClientID,rli1->IQOSGroupId,sizeof(ClientID) -1);

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				else if(flen==sizeof(REPORTLOGITEM2))
				{
					_ASSERT(false); // TODO: Untested
					REPORTLOGITEM2 *rli2=(REPORTLOGITEM2 *)fptr;
				}
				else if(flen==sizeof(REPORTLOGITEM2IQOS))
				{
					_ASSERT(false); // TODO: Untested
					REPORTLOGITEM2IQOS *rli2iqos=(REPORTLOGITEM2IQOS *)fptr;
					// TODO: Unfinished.. looks like tag/value pair, not binary record
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;
					mtype='8';
					etype=IQExecType(rli2iqos->BigTradeRec.TradeRec.LastStatus);

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// MLALGO FIX Servers
				else if(flen==sizeof(REPORTLOGITEM3))
				{
					REPORTLOGITEM3 *rli3=(REPORTLOGITEM3 *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,rli3->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					//if(rli3->BigTradeRec.TradeRecA.Memo[0])
					//	strncpy(ClOrdID,rli3->BigTradeRec.TradeRecA.Memo,sizeof(ClOrdID) -1);
					//else
						_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",rli3->BigTradeRec.TradeRec.Domain,rli3->BigTradeRec.TradeRec.OrderNo);
					//strncpy(ClParentOrderID,rli3->IQOSListId,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,rli3->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=rli3->BigTradeRec.TradeRec.LastPrice;
					switch(rli3->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,rli3->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					if(rli3->BigTradeRec.TradeRecB.AltAccount1[0])
						strncpy(ClientID,rli3->BigTradeRec.TradeRecB.AltAccount1,sizeof(ClientID) -1);
					else
						strncpy(ClientID,rli3->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					_snprintf(EcnOrderID,sizeof(EcnOrderID) -1,"%s-%s",rli3->BigTradeRec.TradeRec.Domain,rli3->BigTradeRec.TradeRec.OrderNo);
					//strncpy(ClientID,rli3->IQOSGroupId,sizeof(ClientID) -1);
					mtype='8';
					etype=IQExecType(rli3->BigTradeRec.TradeRec.LastStatus);
					ordQty=rli3->BigTradeRec.TradeRec.Shares;
					cumQty=rli3->BigTradeRec.TradeRec.ExeShares;
					leavesQty=rli3->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(rli3->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(rli3->BigTradeRec.TradeRec.LastTime)*10000 +atoi(rli3->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(rli3->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=rli3->ParserSeqNo;
					fillQty=rli3->BigTradeRec.TradeRec.LastShares;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",rli3->BigTradeRec.TradeRec.User,rli3->BigTradeRec.TradeRec.Domain);
					//if(rli3->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",rli3->BigTradeRec.TradeRec.ECNOrderNo,rli3->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",rli3->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);
					//strncpy(ClParentOrderID,rli3->BigTradeRec.TradeRecA.Memo,sizeof(ClParentOrderID) -1);

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				else if(flen==sizeof(REPORTLOGITEM4))
				{
					_ASSERT(false); // TODO: Untested
					REPORTLOGITEM4 *rli4=(REPORTLOGITEM4 *)fptr;
				}
				// MLALGO Traders
				else if(flen==sizeof(REPORTLOGITEM5))
				{
					REPORTLOGITEM5 *rli5=(REPORTLOGITEM5 *)fptr;
					LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
					LONGLONG dend=doffset +flen;

					if(uaim)
						strncpy(AppInstID,uaim->AppInstID,sizeof(AppInstID) -1);
					else
						strncpy(AppInstID,rli5->BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
					_snprintf(ClOrdID,sizeof(ClOrdID) -1,"%s-%s",rli5->BigTradeRec.TradeRec.Domain,rli5->BigTradeRec.TradeRec.OrderNo);
					//strncpy(ClParentOrderID,rli5->IQOSListId,sizeof(ClParentOrderID) -1);
					strncpy(Symbol,rli5->BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
					Price=rli5->BigTradeRec.TradeRec.LastPrice;
					switch(rli5->BigTradeRec.TradeRec.Action)
					{
					case 1: Side='1'; break;
					case 2: Side='2'; break;
					case 3: Side='5'; break;
					};
					strncpy(Account,rli5->BigTradeRec.TradeRec.Account,sizeof(Account) -1);
					strncpy(ClientID,rli5->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					strncpy(ClientID,rli5->BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
					strncpy(EcnOrderID,rli5->BigTradeRec.TradeRec.ECNOrderNo,sizeof(EcnOrderID) -1);
					mtype='8';
					etype=IQExecType(rli5->BigTradeRec.TradeRec.LastStatus);
					ordQty=rli5->BigTradeRec.TradeRec.Shares;
					cumQty=rli5->BigTradeRec.TradeRec.ExeShares;
					leavesQty=rli5->BigTradeRec.TradeRec.Pending;
					TimeStamp=(LONGLONG)atoi(rli5->BigTradeRec.TradeRec.LastDate)*1000000 
						+(atoi(rli5->BigTradeRec.TradeRec.LastTime)*10000 +atoi(rli5->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(rli5->BigTradeRec.TradeRec.LastTime +6));
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
					ExecID=rli5->ParserSeqNo;
					fillQty=rli5->BigTradeRec.TradeRec.LastShares;
					_snprintf(Connection,sizeof(Connection) -1,"%s.%s",rli5->BigTradeRec.TradeRec.User,rli5->BigTradeRec.TradeRec.Domain);
					//if(rli5->BigTradeRec.TradeRec.MMId[0])
					//	_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s.%s%c",rli5->BigTradeRec.TradeRec.ECNOrderNo,rli5->BigTradeRec.TradeRec.MMId,AUX_KEYS_DELIM);
					_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",rli5->BigTradeRec.TradeRec.ECNOrderNo,AUX_KEYS_DELIM);
					//strncpy(ClientID,rli5->IQOSGroupId,sizeof(ClientID) -1);

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				fendl=fptr +flen;

				fptr=fendl;
				_ASSERT(fptr<=fend);
			}
			else if(dfile->proto==PROTO_CLS_EVENTLOG)
			{
				fendl=strendl(fptr,fend);
				if(!fendl)
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}
				if((fendl>fptr)&&(fendl[-1]=='\r')) // CRLF
					fendl--;
				flen=(int)(fendl -fptr);
				char ech=*fendl; *((char*)fendl)=0;

				LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
				LONGLONG dend=doffset +flen;
				const char *tptr=strchr(fptr,'@');
				if(tptr)
				{
					TimeStamp=WSDate -WSDate%100 +atoi(fptr); TimeStamp*=1000000;
					TimeStamp+=atoi(tptr +1)*10000 +atoi(tptr +4)*100 +atoi(tptr +7);
				#ifdef MULTI_DAY
					OrderDate=(int)(TimeStamp/1000000000);
				#endif
				}
				char *eptr=0;
				// AppInstID max 19 chars!!!
				if(!strncmp(fptr,"APPINST:",8))
				{
					char *smpaid=new char[20];
					strncpy(smpaid,fptr +8,19);
					if(UscPort[dfile->portno].DetPtr2)
						delete (char*)UscPort[dfile->portno].DetPtr2;
					UscPort[dfile->portno].DetPtr2=smpaid;
				}
				// TODO: Unfinished--parse the samples below and set appropriate index values
				// NOTICE : Order Placed  : Account[DEMOIQOS:9000],User[EDDIE],Symbol[GOOG],#[100],$[2.000000],Order[20120906-1004],CRef[0],[DEMOX pr ],TC[0],PP[30999],ff[0.000000]
				else if((eptr=(char*)strstr(fptr,"NOTICE : Order Placed  :")))
				{
					char domain[256]={0};
					for(const char *tok=strtoke(eptr +25,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"Account[",8))
						{
							strncpy(domain,tok +8,strlen(tok +8) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"Symbol[",7))
							strncpy(Symbol,tok +7,strlen(tok +7) -1);
						else if(!strncmp(tok,"#[",2))
							ordQty=atoi(tok +2);
						else if(!strncmp(tok,"$[",2))
							Price=atof(tok +2);
						else if(!strncmp(tok,"Order[",6))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +6,strlen(tok +6) -1);
						}
					}
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype='D';

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// NOTICE : Replace Requested Account[DEMOIQOS:SUP1000],User[MIKEL],Symbol[DELL],Order[20120906-1041]
				//else if((eptr=(char*)strstr(fptr,"NOTICE : Replace Requested")))
				//{
				//	for(const char *tok=strtoke(eptr +27,",");tok;tok=strtoke(0,","))
				//	{
				//		if(!strncmp(tok,"Account[",8))
				//		{
				//			char domain[256]={0};
				//			strncpy(domain,tok +8,strlen(tok +8) -1);
				//			char *dptr=strchr(domain,':');
				//			if(dptr)
				//			{
				//				*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
				//			}
				//			else
				//				strncpy(Account,domain,sizeof(Account) -1);
				//			strncpy(AppInstID,domain,sizeof(AppInstID) -1);
				//		}
				//		else if(!strncmp(tok,"Symbol[",7))
				//			strncpy(Symbol,tok +7,strlen(tok +7) -1);
				//		else if(!strncmp(tok,"Order[",6))
				//			strncpy(ClOrdID,tok +6,strlen(tok +6) -1);
				//	}
				//	// AppInstID default
				//	if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
				//		strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
				//	mtype='G';
				//	AddJournal(dj,fptr,flen,doffset,dend,
				//				AppInstID,ClOrdID,RootOrderID,
				//				FirstClOrdID,ClParentOrderID,Symbol,
				//				Price,Side,
				//				Account,EcnOrderID,ClientID,
				//				Connection,AuxKey,
				//				mtype,etype,ordQty,cumQty,leavesQty,
				//				TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				//}
				// From ECN[MLALGO],ExecId[REPLACE_REQ],Acc[MAPOPCO:ALGOACC],Ord[20120924-1019],a[SELL],s[DSPG],Stat[Confirmed],Shares[0],Price[6.100000],AON[0] Qty[20000] TC[469200296] [REPLACE REQUESTED]
				else if(((eptr=(char*)strstr(fptr,"From ECN[")))&&(strstr(fptr,"[REPLACE REQUESTED]")))
				{
					char domain[256]={0};
					for(const char *tok=strtoke(eptr,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"Acc[",4))
						{
							strncpy(domain,tok +4,strlen(tok +4) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"Ord[",4))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +4,strlen(tok +4) -1);
						}
						// Local map to link replace requests
						else if(strstr(tok,"TC["))
						{
							char TC[32]={0};
							tok=strstr(tok,"TC[");
							tok+=3;
							const char *eptr=strchr(tok,']');
							if(!eptr) eptr=tok +strlen(tok);
							strncpy(TC,tok,(int)(eptr -tok));
							cltcmap.insert(pair<string,string>(TC,ClOrdID));
						}
					}
				}
				// NOTICE : Order Replaced in Msg757: Account[DEMOIQOS:SUP1000],User[MIKEL],Symbol[DELL],#[2000],$[10.000000],Order[20120906-1042],CRef[0],[DEMOX pr ],TC[2050134251],PP[30999],ff[0.000000] Gtc[00000000]
				else if((eptr=(char*)strstr(fptr,"NOTICE : Order Replaced in Msg757:")))
				{
					char domain[256]={0};
					char TC[32]={0};
					for(const char *tok=strtoke(eptr +35,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"Account[",8))
						{
							strncpy(domain,tok +8,strlen(tok +8) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"Symbol[",7))
							strncpy(Symbol,tok +7,strlen(tok +7) -1);
						else if(!strncmp(tok,"#[",2))
							ordQty=atoi(tok +2);
						else if(!strncmp(tok,"$[",2))
							Price=atof(tok +2);
						else if(!strncmp(tok,"Order[",6))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +6,strlen(tok +6) -1);
						}
						else if(!strncmp(tok,"TC[",3))
							strncpy(TC,tok +3,strlen(tok +3) -1);
					}
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype='0';
					etype='5';

					// Link C/R chains
					strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
					map<string,string>::iterator fit=fm->find(ClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
					else
					{
						map<string,string>::iterator cit=cltcmap.find(TC);
						if(cit!=cltcmap.end())
						{
							OrigOrdID=cit->second.c_str();
							fit=fm->find(OrigOrdID);
							if(fit!=fm->end())
								strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						}
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
					}

					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								'G',0,ordQty,0,0,
								TimeStamp,OrigOrdID,ExecID,0,ffpath);
					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// NOTICE : Order REJECTED : Account[DEMOIQOS:QA1000],User[CHANM],Symbol[AAPL],Order[20120906-1027],[No Long Position in AAPL found to Sell]
				else if(strstr(fptr,"NOTICE : Order REJECTED :"))
				{
					_ASSERT(false); //Untested
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype='8';
					etype='8';
					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// NOTICE : Cancel Requested Account[DEMOIQOS:SUP1000],User[MIKEL],Symbol[DELL],Order[20120906-1043]
				// NOTICE : Cancel Requested on Completed order Account[MACOPCO1:ALGOACC],User[MLCHFIX1],Symbol[VRNT],Order[20120924-1169]
				else if((strstr(fptr,"NOTICE : Cancel Requested"))&&(eptr=(char*)strstr(fptr,"Account[")))
				{
					char domain[256]={0};
					for(const char *tok=strtoke(eptr,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"Account[",8))
						{
							strncpy(domain,tok +8,strlen(tok +8) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"Symbol[",7))
							strncpy(Symbol,tok +7,strlen(tok +7) -1);
						else if(!strncmp(tok,"Order[",6))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +6,strlen(tok +6) -1);
						}
					}
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype='F';
					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// MSG710: ECN[DEMOX],EcnOrderNo[20120906-1004],ExecId[DEMO-658],Acc[DEMOIQOS:9000],Ord[20120906-1004],a[BUY],s[GOOG],Stat[Confirmed],Shares[0],Price[0.000000],AON[0],Liq[] TC[0]
				// MSG710: ECN[],EcnOrderNo[],ExecId[DEMO-660],Acc[DEMOIQOS:],Ord[20120906-1026],a[none],s[],Stat[Part Filled],Shares[240],Price[672.500000],AON[0],Liq[B2] TC[100558]
				// MSG710: ECN[],EcnOrderNo[],ExecId[DEMO-696],Acc[DEMOIQOS:],Ord[20120906-1079],a[none],s[],Stat[Part Filled],Shares[218],Price[24.330000],AON[0],Liq[B2] TC[5658624]
				else if((eptr=(char*)strstr(fptr,"MSG710:")))
				{
					char domain[256]={0};
					for(const char *tok=strtoke(eptr +8,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"EcnOrderNo[",11))
						{
							strncpy(EcnOrderID,tok +11,strlen(tok +11) -1);
							_snprintf(AuxKey,AUX_KEYS_MAX_LEN -1,"%s%c",EcnOrderID,AUX_KEYS_DELIM);
						}
						else if(!strncmp(tok,"Acc[",4))
						{
							strncpy(domain,tok +4,strlen(tok +4) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"a[",2))
						{
							if(!strncmp(tok +2,"BUY",3)) Side='1';
							else if(!strncmp(tok +2,"SELL",4)) Side='2';
							else if(!strncmp(tok +2,"SHORT",5)) Side='5';
						}
						else if(!strncmp(tok,"s[",2))
							strncpy(Symbol,tok +2,strlen(tok +2) -1);
						else if(!strncmp(tok,"Stat[",5))
						{
							if(!strncmp(tok +5,"Confirmed",9)) etype='0';
							else if(!strncmp(tok +5,"Part Filled",11)) etype='1';
							else if(!strncmp(tok +5,"Filled",6)) etype='2';
							else if(!strncmp(tok +5,"Expired",7)) etype='3';
							else if(!strncmp(tok +5,"Canceled",8)) etype='4';
							else if(!strncmp(tok +5,"Rejected",8)) etype='8';
							else if(!strncmp(tok +5,"ERROR",5)) etype='8';
							else _ASSERT(false);
						}
						else if(!strncmp(tok,"Shares[",7))
							fillQty=atoi(tok +7);
						else if(!strncmp(tok,"Price[",6))
							Price=atof(tok +6);
						else if(!strncmp(tok,"Ord[",4))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +4,strlen(tok +4) -1);
						}
						else if(!strncmp(tok,"ExecId[",7))
						{
							ExecID=tok +7;
							char *nptr=(char*)ExecID;
							nptr[strlen(nptr) -1]=0;
						}
					}
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype='8';
					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// HELD710 : HELD : Account[DEMOIQOS:SUP1000],User[MIKEL],Symbol[DELL],Order[20120906-1043]
				else if((eptr=(char*)strstr(fptr,"HELD710 : HELD :")))
				{
					char domain[256]={0};
					for(const char *tok=strtoke(eptr +17,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"Account[",8))
						{
							strncpy(domain,tok +8,strlen(tok +8) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"Symbol[",7))
							strncpy(Symbol,tok +7,strlen(tok +7) -1);
						else if(!strncmp(tok,"Order[",6))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +6,strlen(tok +6) -1);
						}
					}
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype=0;
					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				// HELD710 : RELEASED [0]: Account[DEMOIQOS:SUP1000],User[MIKEL],Symbol[DELL],Order[20120906-1043],RecordNo[000000001]
				else if((eptr=(char*)strstr(fptr,"HELD710 : RELEASED")))
				{
					char domain[256]={0};
					char *nptr=strchr(eptr +18,':');
					if(nptr) eptr=nptr +2;
					else eptr+=19;
					for(const char *tok=strtoke(eptr,",");tok;tok=strtoke(0,","))
					{
						if(!strncmp(tok,"Account[",8))
						{
							strncpy(domain,tok +8,strlen(tok +8) -1);
							char *dptr=strchr(domain,':');
							if(dptr)
							{
								*dptr=0; strncpy(Account,dptr +1,sizeof(Account) -1);
							}
							else
								strncpy(Account,domain,sizeof(Account) -1);
							strncpy(AppInstID,domain,sizeof(AppInstID) -1);
						}
						else if(!strncmp(tok,"Symbol[",7))
							strncpy(Symbol,tok +7,strlen(tok +7) -1);
						else if(!strncmp(tok,"Order[",6))
						{
							sprintf(ClOrdID,"%s-",domain);
							strncpy(ClOrdID +strlen(ClOrdID),tok +6,strlen(tok +6) -1);
						}
					}
					// AppInstID default
					if((!AppInstID[0])&&(UscPort[dfile->portno].DetPtr2))
						strncpy(AppInstID,(char*)UscPort[dfile->portno].DetPtr2,sizeof(AppInstID)-1);
					mtype=0;
					AddJournal(dj,fptr,flen,doffset,dend,
								AppInstID,ClOrdID,RootOrderID,
								FirstClOrdID,ClParentOrderID,Symbol,
								Price,Side,
								Account,EcnOrderID,ClientID,
								Connection,OrderDate,AuxKey,
								mtype,etype,ordQty,cumQty,leavesQty,
								TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
				}
				*((char*)fendl)=ech;

				fptr=fendl;
				while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
					fptr++;
				_ASSERT(fptr<=fend);
			}
			else if(dfile->proto==PROTO_FIXS_EVELOG)
			{
				fendl=strendl(fptr,fend);
				if(!fendl)
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}
				if((fendl>fptr)&&(fendl[-1]=='\r')) // CRLF
					fendl--;
				flen=(int)(fendl -fptr);
				char ech=*fendl; *((char*)fendl)=0;

				LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
				LONGLONG dend=doffset +flen;
				char *eptr=0;
				// AppInstID max 19 chars!!!
				if(!strncmp(fptr,"APPINST:",8))
				{
					char *smpaid=new char[20];
					strncpy(smpaid,fptr +8,19);
					if(UscPort[dfile->portno].DetPtr2)
						delete (char*)UscPort[dfile->portno].DetPtr2;
					UscPort[dfile->portno].DetPtr2=smpaid;
				}
				// TODO: Unfinished
				*((char*)fendl)=ech;

				fptr=fendl;
				while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
					fptr++;
				_ASSERT(fptr<=fend);
			}
			else if(dfile->proto==PROTO_TRADER_EVELOG)
			{
				fendl=strendl(fptr,fend);
				if(!fendl)
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}
				if((fendl>fptr)&&(fendl[-1]=='\r')) // CRLF
					fendl--;
				flen=(int)(fendl -fptr);
				char ech=*fendl; *((char*)fendl)=0;

				LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
				LONGLONG dend=doffset +flen;
				char *eptr=0;
				// AppInstID max 19 chars!!!
				if(!strncmp(fptr,"APPINST:",8))
				{
					char *smpaid=new char[20];
					strncpy(smpaid,fptr +8,19);
					if(UscPort[dfile->portno].DetPtr2)
						delete (char*)UscPort[dfile->portno].DetPtr2;
					UscPort[dfile->portno].DetPtr2=smpaid;
				}
				// TODO: Unfinished
				*((char*)fendl)=ech;

				fptr=fendl;
				while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
					fptr++;
				_ASSERT(fptr<=fend);
			}
			else if(dfile->proto==PROTO_RTOATS)
			{
				fendl=strendl(fptr,fend);
				if(!fendl)
				{
					if(lastAvail)
						SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}
				if((fendl>fptr)&&(fendl[-1]=='\r')) // CRLF
					fendl--;
				flen=(int)(fendl -fptr);
				LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
				LONGLONG dend=doffset +flen;

				vector<const char *> cols;
				char ech=*fendl; *((char*)fendl)=0;
				for(const char *tok=strtoke((char*)fptr,",");tok;tok=strtoke(0,","))
					cols.push_back(tok);
				if(cols.size()>1)
				{
					if(!strcmp(cols[0],"#HD#"))
					{
						strncpy(OatsAppInstID,cols[7],sizeof(OatsAppInstID) -1);
						strncpy(OatsClientID,cols[3],sizeof(OatsClientID) -1);
						OatsFileDate=atoi(cols[2]);

						strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						strncpy(ClOrdID,ClientID,sizeof(ClOrdID) -1);
						OrderDate=OatsFileDate;
						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					}
					else if(!strcmp(cols[0],"#TR#"))
					{
						strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						strncpy(ClOrdID,ClientID,sizeof(ClOrdID) -1);
						OrderDate=OatsFileDate;
						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);

						memset(OatsAppInstID,0,sizeof(OatsAppInstID));
						memset(OatsClientID,0,sizeof(OatsClientID));
					}
					else if(!strcmp(cols[1],"NW"))
					{
						strncpy(AppInstID,cols[6],sizeof(AppInstID) -1);
						if(!AppInstID[0])
							strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						strncpy(ClOrdID,cols[8],sizeof(ClOrdID) -1);
						strncpy(FirstClOrdID,cols[8],sizeof(FirstClOrdID) -1);
						strncpy(Symbol,cols[13],sizeof(Symbol) -1);
						Price=atof(cols[16]);
						if(!strcmp(cols[14],"B")) Side='1';
						else if(!strcmp(cols[14],"S")) Side='2';
						else if(!strcmp(cols[14],"SL")) Side='2';
						else if(!strcmp(cols[14],"SX")) Side='5';
						else if(!strcmp(cols[14],"SS")) Side='5';
						else if(!strcmp(cols[14],"SE")) Side='6';
						mtype='D';
						ordQty=atoi(cols[15]);
						if(strncmp(cols[7]+8,"000000",6)!=0)
							TimeStamp=_atoi64(cols[7]);
						else
							TimeStamp=_atoi64(cols[11]);
					#ifdef MULTI_DAY
						OrderDate=(int)(TimeStamp/1000000000);
					#endif

						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					}
					else if(!strcmp(cols[1],"CR"))
					{
						strncpy(AppInstID,cols[6],sizeof(AppInstID) -1);
						if(!AppInstID[0])
							strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						strncpy(ClOrdID,cols[10],sizeof(ClOrdID) -1);
						strncpy(FirstClOrdID,cols[8],sizeof(FirstClOrdID) -1);
						strncpy(Symbol,cols[15],sizeof(Symbol) -1);
						Price=atof(cols[18]);
						if(!strcmp(cols[16],"B")) Side='1';
						else if(!strcmp(cols[14],"S")) Side='2';
						else if(!strcmp(cols[16],"SL")) Side='2';
						else if(!strcmp(cols[16],"SX")) Side='5';
						else if(!strcmp(cols[14],"SS")) Side='5';
						else if(!strcmp(cols[16],"SE")) Side='6';
						mtype='G';
						ordQty=atoi(cols[17]);
						//if(strncmp(cols[7]+8,"000000",6)!=0)
						//	TimeStamp=_atoi64(cols[7]);
						//else
							TimeStamp=_atoi64(cols[13]);
					#ifdef MULTI_DAY
						OrderDate=(int)(TimeStamp/1000000000);
					#endif

						// Link C/R chains
						map<string,string>::iterator fit=fm->find(FirstClOrdID);
						if(fit!=fm->end())
							strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));

						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					}
					else if(!strcmp(cols[1],"CL"))
					{
						strncpy(AppInstID,cols[6],sizeof(AppInstID) -1);
						if(!AppInstID[0])
							strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						strncpy(ClOrdID,cols[8],sizeof(ClOrdID) -1);
						strncpy(FirstClOrdID,cols[8],sizeof(FirstClOrdID) -1);
						strncpy(Symbol,cols[9],sizeof(Symbol) -1);
						mtype='F';
						ordQty=atoi(cols[12]);
						leavesQty=atoi(cols[13]);
						if(strncmp(cols[7]+8,"000000",6)!=0)
							TimeStamp=_atoi64(cols[7]);
						else
							TimeStamp=_atoi64(cols[10]);
					#ifdef MULTI_DAY
						OrderDate=(int)(TimeStamp/1000000000);
					#endif

						// Link C/R chains
						map<string,string>::iterator fit=fm->find(ClOrdID);
						if(fit!=fm->end())
							strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
						fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));

						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					}
					else if(!strcmp(cols[1],"OR"))
					{
						strncpy(AppInstID,cols[6],sizeof(AppInstID) -1);
						if(!AppInstID[0])
							strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						strncpy(ClOrdID,cols[8],sizeof(ClOrdID) -1);
						strncpy(FirstClOrdID,cols[8],sizeof(FirstClOrdID) -1);
						strncpy(Symbol,cols[13],sizeof(Symbol) -1);
						Price=atof(cols[15]);
						if(!strcmp(cols[14],"B")) Side='1';
						else if(!strcmp(cols[14],"S")) Side='2';
						else if(!strcmp(cols[14],"SL")) Side='2';
						else if(!strcmp(cols[14],"SX")) Side='5';
						else if(!strcmp(cols[14],"SS")) Side='5';
						else if(!strcmp(cols[14],"SE")) Side='6';
						strncpy(EcnOrderID,cols[33],sizeof(EcnOrderID) -1);
						strncpy(Connection,cols[34],sizeof(Connection) -1);
						mtype='D';
						ordQty=atoi(cols[44]);
						if(strncmp(cols[7]+8,"000000",6)!=0)
							TimeStamp=_atoi64(cols[7]);
						else
							TimeStamp=_atoi64(cols[35]);
					#ifdef MULTI_DAY
						OrderDate=(int)(TimeStamp/1000000000);
					#endif

						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					}
					else if(!strcmp(cols[1],"RT"))
					{
						strncpy(AppInstID,cols[6],sizeof(AppInstID) -1);
						if(!AppInstID[0])
							strcpy(AppInstID,OatsAppInstID);
						strncpy(ClientID,OatsClientID,sizeof(ClientID) -1);
						if((cols[17][0])&&(cols[23][0]))
							sprintf(ClOrdID,"%s.%s.%s",cols[10],cols[17],cols[23]);
						else if(cols[17][0])
							sprintf(ClOrdID,"%s.%s",cols[10],cols[17]);
						else
							strncpy(ClOrdID,cols[10],sizeof(ClOrdID) -1);
						strncpy(FirstClOrdID,cols[8],sizeof(FirstClOrdID) -1);
						strncpy(RootOrderID,cols[8],sizeof(RootOrderID) -1);
						strncpy(Symbol,cols[11],sizeof(Symbol) -1);
						Price=atof(cols[19]);
						if(!strcmp(cols[14],"B")) Side='1';
						else if(!strcmp(cols[14],"S")) Side='2';
						else if(!strcmp(cols[14],"SL")) Side='2';
						else if(!strcmp(cols[14],"SX")) Side='5';
						else if(!strcmp(cols[14],"SE")) Side='6';
						strncpy(EcnOrderID,cols[10],sizeof(EcnOrderID) -1);
						strncpy(Connection,cols[17],sizeof(Connection) -1);
						if(*cols[23])
						{
							strcat(Connection,".");
							strcat(Connection,cols[23]);
						}
						if(*cols[22])
						{
							strcat(Connection,"/");
							strcat(Connection,cols[22]);
						}
						mtype='D';
						ordQty=atoi(cols[13]);
						TimeStamp=_atoi64(cols[12]);
					#ifdef MULTI_DAY
						OrderDate=(int)(TimeStamp/1000000000);
					#endif

						AddJournal(dj,fptr,flen,doffset,dend,
									AppInstID,ClOrdID,RootOrderID,
									FirstClOrdID,ClParentOrderID,Symbol,
									Price,Side,
									Account,EcnOrderID,ClientID,
									Connection,OrderDate,AuxKey,
									mtype,etype,ordQty,cumQty,leavesQty,
									TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
					}
				}
				*((char*)fendl)=ech;

				fptr=fendl;
				while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
					fptr++;
				_ASSERT(fptr<=fend);
			}
		}

		// Preserve leftovers
		rptr=rbuf;
		if(fptr<=fend)
		{
			int bleft=(int)(fend -fptr);
			if(fptr>rbuf)
			{
				// This is the only function that modifies 'dfile->rend', so we don't need to lock read.
				_ASSERT((rbuf<=fptr)&&(fptr<=fend));
				dfile->rend.QuadPart+=(fptr -rbuf); 
				odb.SetZdirty();
				//sprintf(dbuf,"DEBUG: leftover %d [%I64d,%d), pend=%d\r\n",bleft,dfile->rend.QuadPart,dfile->rend.QuadPart +bleft,pendidx);
				//OutputDebugString(dbuf);
				memmove(rptr,fptr,bleft);
			}
			rptr+=bleft;
		}
		dfile->LockRead();
	}
	dfile->UnlockRead();
	for(int i=0;i<2;i++)
		delete rbufs[i];
	delete rbuf;
	return 0;
}
int ViewServer::SmpShutdown(int UscPortNo)
{
	APPINSTMARKER *uaim=(APPINSTMARKER*)UscPort[UscPortNo].DetPtr6;
	if(uaim)
	{
		UscPort[UscPortNo].DetPtr6; delete uaim;
	}
	return 0;
}

#ifdef MULTI_PROTO_DETAILS
int VSQuery::FilterSmpResult(int proto, const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify)
#else
int VSQuery::FilterSmpResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify)
#endif
{
    int DatLength=dlen;
    VSQueryResult *qresult=new VSQueryResult;
    //qresult->hist=hist;
	#ifdef MULTI_PROTO_DETAILS
	qresult->proto=proto;
	#endif
    qresult->rtime=GetTickCount();

    // For BinaryDataRequests, ignore the SQL select list and send the message 
    qresult->obuf=new char[DatLength];
    memset(qresult->obuf, 0, DatLength);
    char *rptr=qresult->obuf;

    // Send the entire 950 Msg
    memcpy(rptr, dptr, DatLength);
    rptr+=DatLength;
    qresult->DataLength = DatLength;
    if(binarydatalen < DatLength)
        binarydatalen=DatLength;

    if(hist) 
        hresults.push_back(qresult);
    else 
        lresults.push_back(qresult);
    rcnt++;
    return 1;
}
#endif//IQSMP
