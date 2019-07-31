
#include "stdafx.h"
#include "vsctest3.h"
#include "vsctest3Doc.h"
#include "vsctest3View.h"

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
void Cvsctest3View::FIXServerReply(void *udata, int rc, DWORD rid, int endcnt, int totcnt, bool hist, char proto, short pstrlen, const void **pstr, int nstr, bool more, LONGLONG iter, const char *reason)
{
	CListCtrl& clc=GetListCtrl();
	switch(proto)
	{
	case /*PROTO_FIXS_PLACELOG*/0x16:
	case /*PROTO_TRADER_PLACELOG*/0x1B:
	{
		// MLALGO FIX Servers
		if(pstrlen==sizeof(PLACELOGITEM8))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				PLACELOGITEM8 *pli8=(PLACELOGITEM8 *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,37,55,65,54,44,38,60,76,14,151,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=%c|",pli8->BigTradeRec.TradeRec.ReplaceRequest?'G':'D'); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(pli8->BigTradeRec.TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s|",pli8->ParserOrderNo); tptr+=strlen(tptr); break;
					case 41: 
					{
						if(pli8->ReplaceOrderNo[0])
						{
							sprintf(tptr,"41=%s|",pli8->ReplaceOrderNo); tptr+=strlen(tptr); 
						}
						break;
					}
					case 17: break;
					case 37: sprintf(tptr,"37=%s-%s|",pli8->BigTradeRec.TradeRec.Domain,pli8->BigTradeRec.TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",pli8->BigTradeRec.TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(pli8->BigTradeRec.TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",pli8->BigTradeRec.TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",pli8->BigTradeRec.TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(pli8->BigTradeRec.TradeRec.LastDate);
						int wstime=atoi(pli8->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pli8->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pli8->BigTradeRec.TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",pli8->BigTradeRec.TradeRec.PrefMMID[0]?pli8->BigTradeRec.TradeRec.PrefMMID:pli8->BigTradeRec.TradeRec.PrefECN); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",pli8->BigTradeRec.TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",pli8->BigTradeRec.TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31:
					case 32: break;
					case 58: strcpy(tptr,"58=placelog|"); tptr+=strlen(tptr); break;
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".placelog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
			#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		// NYSE CCG Traders
		else if(pstrlen==sizeof(PLACELOGITEM14))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				PLACELOGITEM14 *pli14=(PLACELOGITEM14 *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,37,55,65,54,44,38,60,76,14,151,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=%c|",pli14->BigTradeRec.TradeRec.ReplaceRequest?'G':'D'); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(pli14->BigTradeRec.TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s-%s|",pli14->BigTradeRec.TradeRec.Domain,pli14->BigTradeRec.TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 41: 
					{
						if(pli14->ReplaceOrderNo[0])
						{
							sprintf(tptr,"41=%s-%s|",pli14->BigTradeRec.TradeRec.Domain,pli14->ReplaceOrderNo); tptr+=strlen(tptr); 
						}
						break;
					}
					case 17: break;
					case 37: sprintf(tptr,"37=%s|",pli14->BigTradeRec.TradeRec.ECNOrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",pli14->BigTradeRec.TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(pli14->BigTradeRec.TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",pli14->BigTradeRec.TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",pli14->BigTradeRec.TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(pli14->BigTradeRec.TradeRec.LastDate);
						int wstime=atoi(pli14->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pli14->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pli14->BigTradeRec.TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",pli14->BigTradeRec.TradeRec.PrefMMID[0]?pli14->BigTradeRec.TradeRec.PrefMMID:pli14->BigTradeRec.TradeRec.PrefECN); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",pli14->BigTradeRec.TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",pli14->BigTradeRec.TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31:
					case 32: break;
					case 58: strcpy(tptr,"58=placelog|"); tptr+=strlen(tptr); break;
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".placelog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
			#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		// MLALGO Traders
		else if(pstrlen==sizeof(PLACELOGITEM7))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				PLACELOGITEM7 *pli7=(PLACELOGITEM7 *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,37,55,65,54,44,38,60,76,14,151,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=%c|",pli7->BigTradeRec.TradeRec.ReplaceRequest?'G':'D'); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(pli7->BigTradeRec.TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s-%s|",pli7->BigTradeRec.TradeRec.Domain,pli7->BigTradeRec.TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 41: 
					{
						if(pli7->ReplaceOrderNo[0])
						{
							sprintf(tptr,"41=%s-%s|",pli7->BigTradeRec.TradeRec.Domain,pli7->ReplaceOrderNo); tptr+=strlen(tptr); 
						}
						break;
					}
					case 17: break;
					case 37: sprintf(tptr,"37=%s|",pli7->BigTradeRec.TradeRec.ECNOrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",pli7->BigTradeRec.TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(pli7->BigTradeRec.TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",pli7->BigTradeRec.TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",pli7->BigTradeRec.TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(pli7->BigTradeRec.TradeRec.LastDate);
						int wstime=atoi(pli7->BigTradeRec.TradeRec.LastTime)*10000 +atoi(pli7->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(pli7->BigTradeRec.TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",pli7->BigTradeRec.TradeRec.PrefMMID[0]?pli7->BigTradeRec.TradeRec.PrefMMID:pli7->BigTradeRec.TradeRec.PrefECN); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",pli7->BigTradeRec.TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",pli7->BigTradeRec.TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31:
					case 32: break;
					case 58: strcpy(tptr,"58=placelog|"); tptr+=strlen(tptr); break;
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".placelog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
			#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		else
			_ASSERT(false);
		break;
	}
	case /*PROTO_FIXS_CXLLOG*/0x17:
	case /*PROTO_TRADER_CXLLOG*/0x1C:
	{
		// MLALGO FIX Servers
		if(pstrlen==sizeof(CANCELLOGITEM2ATE))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				CANCELLOGITEM2ATE *cli2ate=(CANCELLOGITEM2ATE *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,17,37,55,65,54,44,38,60,76,14,151,31,32,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=%c|",cli2ate->BigTradeRec.TradeRec.LastStatus==TRADE_CANCELED?'8':'F'); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(cli2ate->BigTradeRec.TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s|",cli2ate->CancelParserOrderNo); tptr+=strlen(tptr); break;
					case 41: sprintf(tptr,"41=%s|",cli2ate->OriginalParserOrderNo); tptr+=strlen(tptr); break;
					case 17: break;
					case 37: sprintf(tptr,"37=%s-%s|",cli2ate->BigTradeRec.TradeRec.Domain,cli2ate->BigTradeRec.TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",cli2ate->BigTradeRec.TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(cli2ate->BigTradeRec.TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",cli2ate->BigTradeRec.TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",cli2ate->BigTradeRec.TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(cli2ate->BigTradeRec.TradeRec.LastDate);
						int wstime=atoi(cli2ate->BigTradeRec.TradeRec.LastTime)*10000 +atoi(cli2ate->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(cli2ate->BigTradeRec.TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",cli2ate->BigTradeRec.TradeRec.PrefMMID[0]?cli2ate->BigTradeRec.TradeRec.PrefMMID:cli2ate->BigTradeRec.TradeRec.PrefECN); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",cli2ate->BigTradeRec.TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",cli2ate->BigTradeRec.TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31:
					case 32: break;
					case 58: strcpy(tptr,"58=cancellog|"); tptr+=strlen(tptr); break;
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".cancellog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
				#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		// MLALGO Traders
		else if(pstrlen==sizeof(CANCELLOGITEM))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				CANCELLOGITEM *cli1=(CANCELLOGITEM *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,17,37,55,65,54,44,38,60,76,14,151,31,32,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=%c|",cli1->TradeRec.LastStatus==TRADE_CANCELED?'8':'F'); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(cli1->TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s-%s|",cli1->TradeRec.Domain,cli1->TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 41: break;
					case 17: break;
					case 37: sprintf(tptr,"37=%s|",cli1->CancelParserOrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",cli1->TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(cli1->TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",cli1->TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",cli1->TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(cli1->TradeRec.LastDate);
						int wstime=atoi(cli1->TradeRec.LastTime)*10000 +atoi(cli1->TradeRec.LastTime +3)*100 +atoi(cli1->TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",cli1->TradeRec.PrefMMID[0]?cli1->TradeRec.PrefMMID:cli1->TradeRec.PrefECN); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",cli1->TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",cli1->TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31:
					case 32: break;
					case 58: strcpy(tptr,"58=cancellog|"); tptr+=strlen(tptr); break;
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".cancellog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
				#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		else
			_ASSERT(false);
		break;
	}
	case /*PROTO_FIXS_RPTLOG*/0x18:
	case /*PROTO_TRADER_RPTLOG*/0x1D:
	{
		// MLALGO FIX Servers
		if(pstrlen==sizeof(REPORTLOGITEM3))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				REPORTLOGITEM3 *rli3=(REPORTLOGITEM3 *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,17,37,55,65,54,44,38,60,76,14,151,31,32,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=8|"); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(rli3->BigTradeRec.TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s|",rli3->BigTradeRec.TradeRecA.Memo); tptr+=strlen(tptr); break;
					case 41: break;
					case 17: sprintf(tptr,"17=%s|",rli3->BigTradeRec.TradeRec.ECNExecID); tptr+=strlen(tptr); break;
					case 37: sprintf(tptr,"37=%s-%s|",rli3->BigTradeRec.TradeRec.Domain,rli3->BigTradeRec.TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",rli3->BigTradeRec.TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(rli3->BigTradeRec.TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",rli3->BigTradeRec.TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",rli3->BigTradeRec.TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(rli3->BigTradeRec.TradeRec.LastDate);
						int wstime=atoi(rli3->BigTradeRec.TradeRec.LastTime)*10000 +atoi(rli3->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(rli3->BigTradeRec.TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",rli3->BigTradeRec.TradeRec.PrefECN[0]?rli3->BigTradeRec.TradeRec.PrefECN:rli3->BigTradeRec.TradeRec.PrefMMID); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",rli3->BigTradeRec.TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",rli3->BigTradeRec.TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31: sprintf(tptr,"31=%.4f|",rli3->BigTradeRec.TradeRec.LastPrice); tptr+=strlen(tptr); break;
					case 32: sprintf(tptr,"32=%d|",rli3->BigTradeRec.TradeRec.LastShares); tptr+=strlen(tptr); break;
					case 58: 
					{
						if(rli3->RejectReasonText[0])
							sprintf(tptr,"58=reportlog: %s",rli3->RejectReasonText); 
						else
							strcpy(tptr,"58=reportlog|"); 
						tptr+=strlen(tptr); 
						break;
					}
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".reportlog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
			#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		// NYSE CCG Traders
		else if(pstrlen==sizeof(REPORTLOGITEM1))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				REPORTLOGITEM1 *rli1=(REPORTLOGITEM1 *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,17,37,55,65,54,44,38,60,76,14,151,31,32,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=8|"); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(rli1->TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s-%s|",rli1->TradeRec.Domain,rli1->TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 41: break;
					case 17: sprintf(tptr,"17=%s|",rli1->TradeRec.ECNExecID); tptr+=strlen(tptr); break;
					case 37: sprintf(tptr,"37=%s|",rli1->TradeRec.ECNOrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",rli1->TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(rli1->TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",rli1->TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",rli1->TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(rli1->TradeRec.LastDate);
						int wstime=atoi(rli1->TradeRec.LastTime)*10000 +atoi(rli1->TradeRec.LastTime +3)*100 +atoi(rli1->TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",rli1->TradeRec.PrefECN[0]?rli1->TradeRec.PrefECN:rli1->TradeRec.PrefMMID); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",rli1->TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",rli1->TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31: sprintf(tptr,"31=%.4f|",rli1->TradeRec.LastPrice); tptr+=strlen(tptr); break;
					case 32: sprintf(tptr,"32=%d|",rli1->TradeRec.LastShares); tptr+=strlen(tptr); break;
					case 58: 
					{
						if(rli1->RejectReasonText[0])
							sprintf(tptr,"58=reportlog: %s",rli1->RejectReasonText); 
						else
							strcpy(tptr,"58=reportlog|"); 
						tptr+=strlen(tptr); 
						break;
					}
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".reportlog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
			#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		// MLALGO Traders
		else if(pstrlen==sizeof(REPORTLOGITEM5))
		{
			FIXINFO *pfix=new FIXINFO[nstr];
			memset(pfix,0,nstr*sizeof(FIXINFO));
			for(int i=0;i<nstr;i++)
			{
				REPORTLOGITEM5 *rli5=(REPORTLOGITEM5 *)pstr[i];
				pfix[i].Reset();
				pfix[i].FIXDELIM='|';
				pfix[i].noSession=true;
				char *tbuf=new char[2048];
				memset(tbuf,0,2048);
				char *tptr=tbuf;
				string tagfilter=(theApp.allfixtags) ?"35,150,11,41,17,37,55,65,54,44,38,60,76,14,151,31,32,58" :theApp.fixfilter;
				for(const char *fptr=tagfilter.c_str();fptr;fptr=strchr(fptr,','))
				{
					if(*fptr==',') fptr++;
					int tno=atoi(fptr);
					if(tno<=0)
						continue;
					switch(tno)
					{
					case 35: sprintf(tptr,"35=8|"); tptr+=strlen(tptr); break;
					case 150:
					{
						char etype[2]={IQExecType(rli5->BigTradeRec.TradeRec.LastStatus),0};
						sprintf(tptr,"150=%s|",etype); tptr+=strlen(tptr);
						break;
					}
					case 11: sprintf(tptr,"11=%s-%s|",rli5->BigTradeRec.TradeRec.Domain,rli5->BigTradeRec.TradeRec.OrderNo); tptr+=strlen(tptr); break;
					case 41: break;
					case 17: sprintf(tptr,"17=%s|",rli5->BigTradeRec.TradeRec.ECNExecID); tptr+=strlen(tptr); break;
					case 37: sprintf(tptr,"37=%s|",rli5->BigTradeRec.TradeRec.ECNOrderNo); tptr+=strlen(tptr); break;
					case 55: sprintf(tptr,"55=%s|",rli5->BigTradeRec.TradeRec.ExSymbol.Symbol); tptr+=strlen(tptr); break;
					case 65: break;
					case 54: 
					{
						char side[8]={0};
						switch(rli5->BigTradeRec.TradeRec.Action)
						{
						case 1: strcpy(side,"B"); break;
						case 2: strcpy(side,"SL"); break;
						case 3: strcpy(side,"SS"); break;
						};
						sprintf(tptr,"54=%s|",side); tptr+=strlen(tptr); 
						break;
					}
					case 44: sprintf(tptr,"44=%.4f|",rli5->BigTradeRec.TradeRec.Price); tptr+=strlen(tptr); break;
					case 38: sprintf(tptr,"38=%d|",rli5->BigTradeRec.TradeRec.Shares); tptr+=strlen(tptr); break;
					case 60: 
					{
						SYSTEMTIME lst;
						CString stz;
						int wsdate=atoi(rli5->BigTradeRec.TradeRec.LastDate);
						int wstime=atoi(rli5->BigTradeRec.TradeRec.LastTime)*10000 +atoi(rli5->BigTradeRec.TradeRec.LastTime +3)*100 +atoi(rli5->BigTradeRec.TradeRec.LastTime +6);
						LONGLONG kval=wsdate; kval*=1000000;
						kval+=wstime; kval*=1000;
						ESTToGMT(kval,lst,stz);
						sprintf(tptr,"60=%04d%02d%02d-%02d:%02d:%02d|",
									lst.wYear,lst.wMonth,lst.wDay,
									lst.wHour,lst.wMinute,lst.wSecond,
									lst.wMilliseconds); tptr+=strlen(tptr); 
						break;
					}
					case 76: sprintf(tptr,"76=%s|",rli5->BigTradeRec.TradeRec.PrefECN[0]?rli5->BigTradeRec.TradeRec.PrefECN:rli5->BigTradeRec.TradeRec.PrefMMID); tptr+=strlen(tptr); break;
					case 14: sprintf(tptr,"14=%d|",rli5->BigTradeRec.TradeRec.ExeShares); tptr+=strlen(tptr); break;
					case 151: sprintf(tptr,"151=%d|",rli5->BigTradeRec.TradeRec.Pending); tptr+=strlen(tptr); break;
					case 31: sprintf(tptr,"31=%.4f|",rli5->BigTradeRec.TradeRec.LastPrice); tptr+=strlen(tptr); break;
					case 32: sprintf(tptr,"32=%d|",rli5->BigTradeRec.TradeRec.LastShares); tptr+=strlen(tptr); break;
					case 58: 
					{
						if(rli5->RejectReasonText[0])
							sprintf(tptr,"58=reportlog: %s",rli5->RejectReasonText); 
						else
							strcpy(tptr,"58=reportlog|"); 
						tptr+=strlen(tptr); 
						break;
					}
					};
				}
				pfix[i].FixMsgReady(tbuf,(int)(tptr -tbuf));
			}
			CString lasys=curq.asys; curq.asys=lasys +".reportlog";
			#ifdef TIME_CONVERT
			bool lgmt=curq.isgmt; curq.isgmt=false;
			#endif
			VSCNotifySqlFixReply(udata,rc,rid,endcnt,totcnt,hist,proto,pfix,nstr,more,iter,reason);
			curq.asys=lasys; 
			#ifdef TIME_CONVERT
			curq.isgmt=lgmt;
			#endif
			for(int i=0;i<nstr;i++)
				delete pfix[i].fbuf;
			delete [] pfix;
		}
		else
			_ASSERT(false);
		break;
	}
	};
}

