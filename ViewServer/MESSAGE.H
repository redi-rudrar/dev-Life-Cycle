//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING		This file is not to be modified except by dev managers
//  WARNING
//  WARNING
//  WARNING		Its a system wide file!!!
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING
//  WARNING

#include "dbcommon.h"

#ifndef __MESSAGE_H
#define __MESSAGE_H

#define MSG1831BUFSIZE (1024 * 2)

#define MSG822BUFSIZE 1024
#define MSG1822BUFSIZE	1024
#define MSG829BUFSIZE 1024
#define MSG831BUFSIZE 1024

#define MAX_NUM_MSGS837 10
#define MAX_NUM_MSGS1837 10

// A IQIpLink message consists of 4 zero's follows by a tag list.
// Tags are delimited using ascii character 01 abd the complete message is terminated with ascii character 03
//

// a Free format msg which is driven of a "tag=" list separeted the 0x01 and terminated by 0x03

// ALL DATA UNLESS OTHERWISE SPECIFIED MUST BE SENT IN UPPERCASE !!!!!!


//
// TagList:
// 1	= MsgLength	// Must always be present and must be the first field in the tag list.
					// 					in bytes excluding the 4 header 0's
					// but including all the separation and termination flieds. Failure to send the msg, or
					// if the terminator is not found at MsgLength characters from Msgstart ,will result in
					// the connection being terminated without any warning.

// 2	= MsgID		// Must always be present and the second field, This field will identify nature of the
					// data/instructions contained in the msg. Only the data/instructions of the MsgID that
					// is the second field will be processed. Failure to include this tag as a second msg
					// will result in the msg not being prosessed.The use of this field is broken into 3 groups
					// 1-99 both directions, 100-499 Client to Server and 500-999 Server to client.

// 3	= VerNo		// Used to indentify what version of messages will be sent. Used in 101 Login Msg

// 97	= Historic  // This Flag indicate if the data send is part for the Historic Data Send during the
					// Post-Login Data Transfer This is added to male programming easier, the msg=501 tag 99=99
					// mark the official end of the Post-Login Data Transfer The Flag will be set to 1 to indicate
					// historic data and ommited in all other cases

// 98	= BlockSeq	// This tag indicates the sequence in a block transfer.This is to show what to expect
					// During transfer of sequence of data 1&3 does not carry data 1&0 Can be used to reset
					// a local database and 3&0 to signal tranfer completed. To avoid unseccary messages
					// 0 can be used to send just a single item. Typical empty list would be 1&3 that would
					// indicated no Data found, but reset localdb and transfercomplete.
					// The main implenemtation of this tag is just after the login when the server sends all
					// the historic data to the client.
					// 0=Single Item (Data); 1=Block Start(No Data) ; 2=Block Item (Data)
					// 3=Block End(No Data)

// 99	= RetCode	// This tag indicates a success=0 ; 1>=Error code in some msg's ex. 500 login responce
					// Also the retcode the server needs from your heartbeat responce


// 100	= Domain	// Must always be present in Msg 100-499. This field will be checked against the loged-in
					// user on that port
					// if the field does not match the connection will be terminated without any warninng

// 101	= UserID	// Must always be present in Msg 100-499 . This field will be checked against the loged-in
					// user on that port
					// if the field does not match the connection will be terminated without any warning.

// 102	= Password	// Only needed in login Msg. It's is strongly recomended that this fields is not filled in
					// any in other messages to minimize the posible unlawfull detection of passwords. Remember
					// You are not just exposing your client , but the data integraty of every one on the system.

// 103	= Account	// Must always be present in all messages indicated . This field will be checked against the
					// loged in user at that port if the field does not match the connection will be terminated
					// without any warning.

// 104  = AccType	// 0 = Margin
					// 1 = Cash

// 105	= Symbol	// Symbol Name

// 106	= Exchange	// Exchange Code
					// 11 = Bulletin Board
					// 12 = Option
					// 13 = Nyse
					// 14 = Amex
					// 15 = Nasdaq

// 107	= Region	// Exchange Region Code
					// k = Bulletin Board

// 108	= Side		// 1 = Buy
					// 2 = Sell
					// 3 = Sell Short

// 109	= Type		// 1 = Market
					// 2 = Limit

// 110	= Shares	// Number of shares ordered

// 111	= Price		// Price for Order, must be set to 0 for market orders

// 112	= Route		// Executing ECN ex. ISLD,ARCA,ADP,SOES,SNET or Default of auto route

// 113	= StopType	// 1 = Stop out at Fixed StopAmount
					// 2 = Stop out at Traling StopAmount

// 114	= StopAmount// Fixed Stop Price for StopType=1 or Traling Stop Amount for StopType=2

// 115	= Gtc		// Set to 1 for Good Till Cancel Order

// 116	= Ext		// Set to 1 for Extended Hour Trading

// 117	= PrefMMID	// Preferenced MarketMaker ID

// 118	= AOL		// Set Order to Aon or None

// 119	= IOC		// Set Order to Immediate or Cancel / for Arca this is ARCA_NOW

// 120	= OrderNo	// InstaQuote OrderNo , This is unique per order&Domain

// 121	= LastStatus// Status Of the Order at this detail Line
					// -2 = Received from Client
					// -1 = Placed at ECN
					// 0  = Confirmed by ECN
					// 1  = Part Filled
					// 2  = Filled
					// 3  = Expired/Done for the day (Remaning shares Canceled)
					// 4  = Remaning shares Canceled
					// 8  = Rejected (Remaning shares Canceled)

// 122	= LastShares// Shares executed on this detail line

// 123	= LastPrice // Price of shares executed on this detail line

// 124	= ExeShares // Total Shares Executed for this order up to and including the detail line

// 125	= ECNId     // This is the ECN that the Route sent the order to. 99% of the time it will be the same as
					// Route, Demos go the the DEMOECN and if your company has auto routing

// 126	= OnTheOpen // Use in combination with Tag 109=1 Market to make a market on open order

// 127	= OnTheClose// Use in combination with Tag 109=1 Market to make a market on close order

// 128	= ReservedShow// This is the number of shares you want to show in the market, 0 = show all and
					// (-1) = invisiable,

// 129	= ISLD_BOOK	// Show this order only in the ISLD BOOK

// 130	= FOK		// Set Order to Fill or KIll

// 131	= NYSE_DIRECT// Send Nyse Order to NYSE-AUTOX

// 132	= LastTime   // The time this detail line was put into the system

// 131	= NYSE_DIRECT// Send Nyse Order to NYSE-AUTOX

// 140	= Position  // Indicates what type of position is held
					//	0 = Short
					//	1 = Long
// 141	= PrevShares// Shares in Current Position that comes from overnight positions

// 142  = DayShares // Shares in Current Position that comes from transactions done today

// 143	= PrevCost	// Cost of Shares in Current Position that comes from overnight positions

// 144  = DayCost	// Cost of Shares in Current Position that comes from transactions done today

// 145	= OptionRootSymbol// Root Symbol Name - if Symbol(105) is a option

// 146	= OptionRootExchange// Exchange Code - if Symbol(105) is a option
					// See Tag(106) for Values

// 147	= Region	// Exchange Region Code  - if Symbol(105) is a option
					// See Tag(106) for Values

// 150	= BPBuyStock	// Buypower Avaible for Buying Stock

// 151	= BPBuyOptions // Buypower Avaible for Buying Option

// 152	= BPShortStock // Buypower Avaible for Shorting Stocks

// xxx153	= BPSellCoverOpt // Buypower Avaible for Selling Covered Options NO LONGER !!!! USED NO MORE

// 154	= BPSellNakedOpt // Buypower Avaible for Selling Naked Options

// 998	= Memo		// This Field can be used to add a client memo to orders when its place. This Memo Field
					// will be send back to the client on the Received confirmation so the memo field can be
					// matched up with a Instaquote OrderNo

// 999	= Text		// All text comments , including reject reasons, and system messages. This field should not
					// parsased since if format is not fixed or documented at all. It main use is to provide
					// a human readible version of some msg's that can be passed on to the frontend user.

// MsgID List

// Required Tags list assumes that Tags 1=MsgLength and 2=MsgID is required for all messages and that
// Tags 100=Domain and 101=UserID is required for all messages 100-499


// 1	= HeartBeat // The message must be sent by the server +/- every second, the client must send the exact msg
					// back to the server within the time limit stated in the text msg on the server will
					// disconnect the client.
					// Required Tags :	99=RetCode (Server generated seqno)
					//					999=Text (States what the current server timeout is set to)
					// Optional Tags :	None

// 101	= Login		// Logon request msg, Must always be the first message sent. Failure to do so will result in
					// the connection being terminated without any warning. PLEASE read msgID=1 regarding dupplicated
					// connections
					// Required Tags :	3=VerNo
					//					102=Password
					//					103=Account
					// Optional Tags :	None

// 102	= PlaceOrder// Message will place new orders into the market
					// Required Tags :	103=Account
					//					104=Account Type
					//					105=Symbol
					//					106=Exchange
					//					108=Side
					//					109=Type
					//					110=Shares
					//					111=Price
					//					112=Route
					//					100=Domain
					//					101=User

					// Optional Tags :	107=Region
					//					113=Stop Type Must Have Tag=114 Set
					//					114=Stop Amount Must be present with Tag=113
					//					115=Gtc
					//					116=Ext
					//					117=PrefMMID
					//					118=AOL
					//					119=IOC
					//					126=OnTheOpen
					//					127=OnTheClose
					//					128=ReservedShow
					//					129=ISLD_BOOK
					//					130=FOK
					//					998=Memo
					//					999=Text

// 103	= CancelOrder// Message will send a Cancel Request to the market. Remember this is only a request
					// Until you are confirmed canceled consider this order live. Always it is very important
					// remember that executions can still be reported after a cancel request.

					// Required Tags :	103=Account
					//					105=Symbol
					//					106=Exchange
					//					120=OrderNo

					// Optional Tags :	107=Region

// 112	= Buy Hedge // Message will place Hedge order
					//Required Tags 	100=Domain
					//					101=User
					//					103=Account
					//					104=Account Type
					//					105=Symbol
					//					106=Exchange
					//					110=Shares
					//					112=Route // ESSX
					// Optional Tags :	134=AutoSell(=1)



// 500	= LoginResponce	//Login responce MSG, this msg will notify the client of a successfull/failed login
					// It is important not to consider a client logged on until all the post login data has been received
					// 0 means Login expected , but 99 will signal when the Historical Data has been transmitted and the
					// client can continue with sending trades and other messages.
					// Required Tags :	99=RetCode(0=success;1=failed;2=duplicate login;99=Post-Login Data Transfer Done)
					// Optional Tags :	999=Text

// 501	= OrderDetails//Carries all details about an order ex. revieved/placed/executed
					// After login all the historic Order details is also sent to the client
					// as a data block utilizing tag=98
					// Required Tags :	98=BlockSeq
					//					100=Domain
					//					101=UserID
					//					103=Account
					//					104=Account Type
					//					105=Symbol
					//					106=Exchange
					//					108=Side
					//					109=Type
					//					110=Shares
					//					111=Price
					//					112=Route
					//					120=OrderNo
					//					121=LastStatus
					//					132=LastTime

					// Optional Tags :	97=Historic
					//					107=Region
					//					113=Stop Type Must Have Tag=114 Set
					//					114=Stop Amount Must be present with Tag=113
					//					115=Gtc
					//					116=Ext
					//					117=PrefMMID
					//					118=AOL
					//					119=IOC
					//					126=OnTheOpen
					//					127=OnTheClose
					//					128=ReservedShow
					//					129=ISLD_BOOK
					//					130=FOK
					//					998=Memo
					//					999=Text

// 502	= PositionDetails//Carries all details about the current open positions in the account
					// After login all the current open positions is also sent to the client
					// as a data block utilizing tag=98 then updates are sent as trading day goes on
					// Required Tags :	98=BlockSeq
					//					100=Domain
					//					103=Account
					//					104=Account Type
					//					105=Symbol
					//					106=Exchange
					//					140=Position
					//					141=PrevShares
					//					142=DayShares
					//					143=PrevCost
					//					144=DayCost
					//					145=OptionRootSymbol
					//					146=OptionRootExchange
					//					147=OptionRootRegion

					// Optional Tags :	97=Historic
					//					107=Region

// 503	= AccStat	// Carries all accounts  Buying power Details. This Message will
					// be sent at login and again everytime one the the fields change during the session
					// Required Tags :	100=Domain
					//					103=Account
					//					150=BPBuyStock
					//					151=BPBuyOptions
					//					152=BPShortStock
					//					153=BPSellCoverOpt
					//					154=BPSellNakedOpt
					//					155=TotalEquity
					//					156=TotalExposure
					// Optional Tags :	97=Historic

// 3000 = MSG_IQWINVIEWPROXY (Used for inter-application communication between IQ and Optronics) RCN - DT939

typedef struct tdClientInfo
{
	char Uuid[16];			// uuid of port
	long L2SessionID;		// Supplied by client and returned with  requested data
	long L2MessageID;		// Supplied by client and returned with  requested data
	long SessionID;			// Supplied by client and returned with  requested data
	long MessageID;			// Supplied by client and returned with  requested data

	union {	// Flags are sent back to client
		char ListTypeFlags;
		struct
		{
			bool ListStart	: 1;	// Indictes first record
			bool DataItem	: 1;	// Indicates Data on this item is valid
			bool ListEnd	: 1;	// End of list - ItemEmpty may be set
			bool UpdateItem : 1;	// Indicates this item is an individual item, not part of a list
		};
	};


} CLIENTINFO;

// New Front-End messages

// 48 : Request small chart
typedef struct tdMsg048Rec
{
	HWND UserWindow;
	HWND UserSession;
	int Interval;
	int NumBars;
	long BeginTime;
	long EndTime;
	char Reserved[8];
	EXSYMBOL ExSymbol;
	char Market[20];
} MSG048REC;

// 122/123 : Notify chart disconnect/connect
typedef struct tdMsg122Rec
{
	int Server; // 1-Hisintra, 2-Hisdaily
} MSG122REC;

// DNS message
// -----------

// 240 : Add User Entitlement
//	IQMUSER IqmUser;

// 244 : Modify User Entitlement
//	IQMUSER IqmUser;

// Order Staging, Get Groups by User/Domain
// MSG301REC is FE version, MSG321REC is BO version of message
typedef struct tdMsg301Rec
{
    union 
    {
	char _stub[512];

        struct 
        {
            long L2SessionID;
            long L2MessageID;

            char Response;			// 0 - default, 1 - response, 2 - notify, -1 - terminator
            char User[USER_LEN];
            char SuperUser;
            char Group[USER_LEN];
            char Domain[DOMAIN_LEN];
        };
    };

    tdMsg301Rec( void ) { initialize(); }

    void initialize( void )
    {
        memset( this, '\0', sizeof( tdMsg301Rec ) );
    }

} MSG301REC, MSG321REC;

// Order Staging, Add/Delete/Publish List msg
typedef struct tdMsg303Rec
{
    union
    {
	char _stub[512];

	struct 
        {
            long L2SessionID;
            long L2MessageID;

            char Response;		// 0 - default, 1 - response, 2 - notify, -1 - terminator
            char Action;		// 1 - Add List, 2 - Ack List, 3 - Publish List, 4 - Remove List, 5 - Reject List
            char Status;	        // Response status 0 - success, Non-Zero - failure
            char ListStatus;		// 0 = new, 1 = pending confirmation, 2 = published
            char IsFIX;                 // 0 = not FIX 1 = FIX
            char List[IQOS_LIST_LEN];   // List Name
            char User[USER_LEN];	// List Creator
            char Group[USER_LEN];	// List Group
            char Domain[DOMAIN_LEN];	// List Domain
			unsigned long ParentCount;	// Number of parent orders in the list
			char ApprovingUser[USER_LEN];	// User who approved the list
			char RejectReason[REASON_LEN];	// User input elaborating on the list reject
        };
    };

    tdMsg303Rec( void ) { initialize(); }

    void initialize( void )
    {
        memset( this, '\0', sizeof( tdMsg303Rec ) );
    }

} MSG303REC;

// ----------------------------------------------------------------------------
/// @brief based on a new way the FrontEnd sends messages (they have the
///        header (MSGHEADER) in every message) Mimicing that here makes sure this 
///        and the FrontEnd are aligned exactly the same. b/c of the 
///        double embedded, having the header imbedded is really important
///        for alignment. There were compile issues with including wsocks msgheader
///        information into this file, so we had to copy a fake header here.
/// @note 2009may21 DT4419 troymguillory
// ----------------------------------------------------------------------------
typedef struct tdMsgHeaderDupe
{
	WORD MsgID;
	WORD MsgLen;
} MSGHEADER_DUPE;

// MSG321REC See above
// Order Staging, Add/Change/Delete User from Group or Add/Delete Group
typedef struct tdMsg322Rec
{
    union 
    {
        char _stub[512];

        struct 
        {
            long L2SessionID;
            long L2MessageID;

            char Response;			// 0 - default, 1 - response, 2 - notify, -1 - terminator
            char Action;			// 1 - Add/Change User, 2 - Add Group, 3 - Remove User from Group 4 - Remove Group
            char Status;			// Response status 0 - success, Non-Zero - failure
            char User[USER_LEN];
            char Group[USER_LEN];
            char Domain[DOMAIN_LEN];
            char SuperUser;
        };
    };

    tdMsg322Rec( void ) { initialize(); }

    void initialize( void )
    {
        memset( this, '\0', sizeof( tdMsg322Rec ) );
    }

} MSG322REC;


//begin DT5793 - kma OSI Phase 2:  CL support for OCC Intraday Updates
// reqOPTIONINFO
typedef struct tdMsg338Rec
{
	EXSYMBOL szSymbol;
	void *hSession;
	void *hWnd;
} MSG338REC;
//end DT5793 - kma OSI Phase 2:  CL support for OCC Intraday Updates

typedef struct tdMsg490Rec
{
	int Command;
	char cParam[20];
	char sParam[20][20];
	int iParam[20];
	int lParam[20];
	float fParam[20];
	double dParam[20];
} MSG490REC;

// 498 : NYLQ Entry
typedef struct tdMsg498Rec
{
	char Exchange;
	char Region;
	char Symbol[20];	
	char MMId[10];
	double Bid;
	long BidSize;
	double Ask;
	long AskSize;
	char Time[7];

	char FirmQuoteCondition;
	char BidXPressIndicator;
	char AskXPressIndicator;
}	MSG498REC;

// 499 : Book Entry
typedef struct tdMsg499Rec
{
	char Exchange;
	char Region;
	char Symbol[20];	
	char MMId[10];
	double Bid;
	long BidSize;
	double Ask;
	long AskSize;
	char Time[7];
} MSG499REC;

// CLSERVER messages
// -----------------

// 501 : Request client login
typedef struct tdMsg501RecOld
{
	long VerNo;
	long L2MessageID;
	char User[USER_LEN];
	char Password[PASSWORD_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG501REC_OLD;

#define UTYPE_LVL1		1
#define UTYPE_LVL2		2
#define UTYPE_IPLINK	3
#define UTYPE_BO		4
#define UTYPE_EXPRESS   5
#define UTYPE_FIX		6
#define UTYPE_IQVIEW	7
#define UTYPE_TWIST		8

typedef struct tdMsg501Rec
{
	short VerNo;		// this is not the Version number - L2 sends this as 1  - if its anything else - you dont get positions
	short BuildType;	// this is the product code - so that the same cannot log into wrong domains  //0 = Normal, 1 = DirectTrade, 2 = ABN Trade; 3 = B of A
	char UserType;
	char Ip3;	// section 3 of the feed being used	xxx.xxx.123.xxx
	char Ip4;	// section 4 of the feed being used	xxx.xxx.xxx.123
	char LogonFlags; // fresh login, set to 0. subsequent logins to change acccount fia toolbar - send 1. If 0 is sent user is logged out by server. 
	char User[USER_LEN];
	char Password[PASSWORD_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG501REC;

// 502 : Request Quote for client
/*typedef struct tdMsg502Rec
{
	long L2SessionID;
	long L2MessageID;
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	int  Row;
	char Symbol[SYMBOL_LEN];	
} MSG502REC;*/

// 503 : Place Order from client
//		 TRADREC STRUCT

// 504 : Cancel Order from client
//     : TRADEREC STRUCT
// Needs :
//          Domain
//			User
//			Account
//			OrderNo
//			ExSymbol.Symbol	(display of messages only)
//


// 505 : Request next OrderNo from database
//		: EMPTY MESSAGE

// 506 : Request AccStat from Clserver


#define SENDFLAG_BITSPRESENT		0x0000000000000001	// must be set if any are set
#define SENDFLAG_DETAILSPACKED		0x0000000000000002
#define SENDFLAG_DETAILS			0x0000000000000004
#define SENDFLAG_POSILINES			0x0000000000000008
#define SENDFLAG_ORDERS				0x0000000000000010
#define SENDFLAG_ACCSTAT			0x0000000000000020
#define SENDFLAG_506TOEND			0x0000000000000040	// PLEASE SEND A 506 BACK WHEN LAST ITEM IS SENT

#define SENDFLAG_CLASSIC506		(SENDFLAG_BITSPRESENT|SENDFLAG_DETAILS|SENDFLAG_POSILINES|SENDFLAG_ORDERS|SENDFLAG_ACCSTAT)


typedef struct tdMsg506Rec
{
	long L2SessionID;
	long L2MessageID;
	char Account[ACCOUNT_LEN];
	long SendFlags;
	char Spare[12];

	char Domain[DOMAIN_LEN];
} MSG506REC;


// 507 : Signal end of Account Read from DataBase
// CLserver sends this when its got the hole account list and then waits
// for it to bounce back before opening user ports

// 508 : Domain Counter Request
typedef struct tdMsg508Rec
{
	char Domain[DOMAIN_LEN];
}MSG508REC;

// 509 : Equity Request
typedef struct tdMsg509Rec
{
	char Domain[DOMAIN_LEN];
}MSG509REC;

// 510 : Open Order Request
typedef struct tdMsg510Rec
{
	char Domain[DOMAIN_LEN];
}MSG510REC;


// 511 : Open Order Request
typedef struct tdMsg511Rec
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];

	char EcnId[ECN_LEN];

	long StartTime;
	long EndTime;

	char PendingOnly;
	char SendNextHistBlocks;
	char SpareBuf[510];

}MSG511REC;

// 512 :  Send Executions

// 513 :  Send Historic Data from Buffer
typedef struct tdMsg513Rec
{
	int MsgNo;
	int BlockSize;
	char SpareBuf[120];
}MSG513REC;

// 514 : Request Open Positions - all accounts, all domains,. etc. Sends Msg 559 to client

typedef struct tdMsg514Rec
{
	long L2SessionID;
	long L2MessageID;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char SubDomains;
} MSG514REC;

// 515 : BO message informing  CLServer what data to send to BO

#define BO_USR_VIEW_POSITIONS		0x02
#define BO_USR_VIEW_ACCSTAT			0x04
#define BO_USR_VIEW_EQUITY			0x08
#define BO_USR_VIEW_OPENORDERS      0x10	// discontinued
#define BO_USR_VIEW_OPENORDERS2     0x20
#define BO_USR_VIEW_HEDGES			0x40
#define BO_USR_VIEW_MESSAGES		0x80
#define BO_USR_VIEW_USERS		    0x100
#define BO_USR_VIEW_AGGREGATES	    0x200
#define BO_USR_VIEW_STOCKLOAN		0x400
#define BO_USR_VIEW_DOMAINSTATS		0x800


// Application specific messages - as a start, any message with this id is sent to the window managing that functionality in the BO

#define ID_USER_MONITOR_MESSAGE		-123


typedef struct tdMsg515Rec
{
	char	User[USER_LEN];
	ULONG	Flags;			// 32 bits of flags
	ULONG	SpareFlags;		// another 32 bits of flags
	int     Operation;		// Set, toggle etc   NOT USED

} MSG515REC;

// 516 : Request AccStat for all accounts accross domain
typedef struct tdMsg516Rec
{
	long L2SessionID;
	long L2MessageID;
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];

} MSG516REC;

// 517 : Product Message - sent from frontend  to clserver - SEND TO CL BUILDS 2008 and up
typedef struct tdMsg517Rec
{
	long L2SessionID;
	long L2MessageID;
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];

	long ProductType;
	long BuildNo;

	long Long1;
	long Long2;
	long Long3;
	long Long4;

	char Spare[256];

} MSG517REC;


// 519 : Convert Order
//     :
// Needs :
//		Type=2
//			TradeRec Domain, Account, OrderNumber to cancel, New details for replacing away order
//
typedef struct tdMsg519Rec
{
	long Type;			// 1 = convert limit order to market 2=Cancel Replace AWAY order
						// Type=2 and Shares=0 deletes the AWAY order, Shares>0 does a replace
	long Spare;

	TRADEREC TradeRec;

} MSG519REC;





//520 = HEDGE_TRADE STRUCTURE - PLACE HEDGE ORDER		incoming

//521 = HEDGE_TRADE STRUCTURE - HEDGE ORDER STATUS		outgoing
//
typedef struct tdMsg521Rec
{
	long SessionID;		// set on call and returned as is
	long MessageID;		// set on call and returned as is
	long Type;			// 0 for later
	long Counter;		// record number - if this is a request, will start at 1, 0 for individual update
	long Spare;

	HEDGE_TRADE Trd;

} MSG521REC;



//522 = HEDGE_TRADE STRUCTURE - CANCEL REQUEST			incoming


// 525 = Account Watch message - register and un-register monitoring of account

typedef struct tdMsg525Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];
	char Account[ACCOUNT_LEN];
	char UserType;
	char LogonFlags;

	// wasted space?

	long VerNo;		// this is not the Version number - L2 sends this as 1  - if its anything else - you dont get positions
	union
	{
		long WatchBits;
		struct
		{
			bool Enable : 1;
			bool Monitor_PosiRec : 1;	// Msg  560
			bool Monitor_AccStat : 1;   // Msg  556

			bool Monitor_PosiTrack: 1;	// Nsg  564

			bool Monitor_OrderDetails: 1; // Msg 553
			bool Monitor_OrderLines	: 1; // Msg 557 - on login
			bool Monitor_OpenOrderLines	: 1; // Only want OPEN orders
			bool Monitor_UserAccRec	: 1;	// DT5636 bud 09/01/09 Multi Account Selection entitlement
		};
	};

} MSG525REC;
// + DT10085 - bjl - Audit 5000 Message support for new login
typedef struct tdMsg1525Rec
{
	MSG525REC Msg525Rec;
	UINT EncryptionType; // 0 - Plaintext, 2 - SHA512
	UINT PassLen;
	char Password[64];
} MSG1525REC;
// - DT10085
// 526 = Account Watch message - Static historic data


typedef struct tdMsg526Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];
	char Account[ACCOUNT_LEN];
	char UserType;
	char LogonFlags;

	// wasted space?

	long VerNo;		// this is not the Version number - L2 sends this as 1  - if its anything else - you dont get positions
	union
	{
		long WatchBits;
		struct
		{
			bool Monitor_OrderDetails_Executions_Only: 1; // Msg 558
		};
	};

} MSG526REC;

// 533 : Place Order from client
//		 BIGTRADREC STRUCT (792 bytes)


// 535 : Trade note
typedef struct tdMsg535Rec
{
	long L2SessionID;
	long L2MessageID;

	// these fields must match the trade coming in
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char OrderNo[ORDERNO_LEN];
	EXSYMBOL ExSymbol;
	long Shares;

	long MessageFlags;

	char Text1[80];
	char Text2[80];
	char Text3[80];
	char Text4[80];

} MSG535REC;

// 536 : Extra trade fields // DT#356
typedef struct tdMsg536Rec
{
	long L2SessionID;
	long L2MessageID;

	// these fields must match the trade coming in
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];

	char OrderNo[ORDERNO_LEN];		// not needed - fe does not have order no yet
	EXSYMBOL ExSymbol;
	long Shares;

	long TextLength;

	char * Data;					// sent in as null, makes it easier to access data.

	// text data follows

} MSG536REC;

// 537 : Extra trade fields - sent from CL to the frontend for TAG'd routes (EASE. GETS2 etc)
typedef struct tdMsg537Rec
{
	long L2SessionID;
	long L2MessageID;

	// these fields must match the trade coming in
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];

	char OrderNo[ORDERNO_LEN];		// not needed - fe does not have order no yet
	EXSYMBOL ExSymbol;
	long Shares;

	long TextLength;

	char * Data;					// sent in as null, makes it easier to access data.

	// text data follows

} MSG537REC;

// 540 : Assign Order To OMS Order
typedef struct tdMsg540Rec
{
	long L2SessionID;
	long L2MessageID;

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];

	char OrderNo[ORDERNO_LEN];
	unsigned long WorkOrderRefNo;		// Set to 0 breaks the link!!

} MSG540REC,MSG_OmsAssignOrder_REC;

// 541 : Assign Order To OMS Order
typedef struct tdMsg541Rec
{
	long L2SessionID;
	long L2MessageID;

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char OmsAccount[ACCOUNT_LEN];		//

	char OrderNo[ORDERNO_LEN];
	unsigned long WorkOrderRefNo;		// Set to 0 breaks the link!!

} MSG541REC;

// 550 : Valid login User-Account Info
typedef struct tdMsg550Rec
{
	long L2SessionID;
	unsigned long L2MessageID; // DT7549 DLA 20110104 - Pairs Trading
	USERACCREC UserAccRec;
} MSG550REC;

// 551 : Invalid login Request
typedef struct tdMsg551Rec
{
	long L2SessionID;
	long L2MessageID;
	char Reason[80];
} MSG551REC;

// 552 : Quote Feed to Client
// symbol,LastShares,Change,Bid,Ask,OpenPrice,PrevClose,HighPrice,LowPrice,Volume,Tick,LastSize,Market,FormatVal,LastReg,ClosePrice,Uused-2,Unused-3
// char *QuoteFeedLine

// 553 : Order detail line to Client
typedef struct tdMsg553Rec
{
	long L2SessionID;
	long L2MessageID;
	TRADEREC TradeRec;
	long MinderUpdateOnly;
} MSG553REC;

// 555 : Next OrderNo from database
typedef struct tdMsg555Rec
{
	long OrderNo;
} MSG555REC;

// 556 : Account Status Record from BuyPowerServer
typedef struct tdMsg556Rec
{
	long L2SessionID;
	long L2MessageID;
	ACCSTATREC AccStatRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG556REC;

// 557 : Trade List Record from ClientServer
typedef struct tdMsg557Rec
{
	long L2SessionID;
	long L2MessageID;
	TRADEREC TradeRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG557REC;

// 558 : TradeDetails List Record from ClientServer
/*typedef struct tdMsg558Rec
{
	long L2SessionID;
	long L2MessageID;
	char NoOfBlocks;
	ORDERDETPKT	OrderDetPkt[10];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG558REC;

// 558 : Packed TradeDetails List Record from ClientServer
typedef struct tdMsg558Rec_1
{
	long L2SessionID;
	long L2MessageID;
	char NoOfBlocks;
	ORDERDETPKT_1 OrderDetPkt[10];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG558REC_1;

// 558 : Packed TradeDetails List Record from ClientServer
typedef struct tdMsg558Rec_2
{
	long L2SessionID;
	long L2MessageID;
	char NoOfBlocks;
	ORDERDETPKT_2 OrderDetPkt[10];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG558REC_2;


// 558 : Packed TradeDetails List Record from ClientServer
typedef struct tdMsg558Rec_3
{
	long L2SessionID;
	long L2MessageID;
	char NoOfBlocks;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	ORDERDETPKT_3 OrderDetPkt[10];
} MSG558REC_3;
*/
// 559 : OpenPosition list from ClientServer
typedef struct tdMsg559Rec
{
	long L2SessionID;
	long L2MessageID;
	POSIREC PosiRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG559REC;

// 560 : OpenPosition update from ClientServer
typedef struct tdMsg560Rec
{
	long L2SessionID;
	long L2MessageID;
	POSIREC PosiRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG560REC;

// 561 : Alert List from ClientServer
typedef struct tdMsg561Rec
{
	long L2SessionID;
	long L2MessageID;
	ALERTSREC AlertsRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG561REC;


// 562 : Rejection Explanation Message
// DO NOT USE 570 is replacing this message
//		 TRADREC STRUCT
/*typedef struct tdMsg562Rec
{
	long L2SessionID;
	long L2MessageID;
	int  ReasonNo;	// 0 = normal Reject // 1 = short rule try 1/16 up // 99 = cancel rejected
	char Reason[80];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG562REC;
*/

// 563 : Duplicated login detected
//
typedef struct tdMsg563Rec
{
	long L2SessionID;
	long L2MessageID;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG563REC;

// 564 : PosiTrac list from ClientServer
typedef struct tdMsg564Rec
{
	long L2SessionID;
	long L2MessageID;
	POSIDETREC PosiDetRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG564REC;


/******************************************************** This message commented out - No-one seems to use it. **************


	  If you find a product using it, feel free to put it back.




// 565 : TradeDetails List Packed from ClientServer
typedef struct tdMsg565Rec
{
	long L2SessionID;
	long L2MessageID;
	EXSYMBOL ExSymbol;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char AccType;		// 0: Margin 1:Cash
	char Action;			// 1:buy, 2:Sell, 3:Short
	char Type;			// 1:Market, 2:Limit
	char Gtc;			// True / False
	char Ext;			// True / False
	char LastStatus;
	char MMId[ECN_LEN];
	char OrderNo[ORDERNO_LEN];
	double Price;
	double LastPrice;
	long Format;
	long LastTime;
	long Pending;
	long LastShares;
	long Canceled;
	long Shares;
} MSG565REC;

******************************************************************************************************************************/

// 566 : Reserved for new accstat message
// 567 : Reserved for new accstat message
// 568 : Reserved for new accstat message

// 569 : Account Status Record from BuyPowerServer
typedef struct tdMsg569Rec
{
	long SessionID;
	long MessageID;
	ACCHIST TodaysAccHist;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG569REC;

// 570 : New Text Message with Reson No
typedef struct tdMsg570Rec
{
	long L2SessionID;
	long L2MessageID;
	WORD ReasonNo;		// 3 destroys blue place order box, 99=resets the account and calls LOGIN again
						// 4=fill warning, 1=reject
	WORD DisplayYN;
	char Text[1024];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG570REC;

// Reson Number Explained
// 0 : SysOps Msg to Client
// 1 : Trade Rejected
// 2 : Cancel Rejected
// 3 : Trade Rejected with Blue Bubble delete
// 99 : Reload Account
// 100-200 : Confirm Message

// 571 : Account Status Record from BuyPowerServer
/*typedef struct tdMsg571Rec
{
	long L2SessionID;
	long L2MessageID;
	ACCSTATSREC AccStatRec;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG571REC;
*/
// 572 : Variable Length Packed Trade Detail
// Format :
// unsigned int TotalLength;
// unsinged char MsgType :
#define  MSG572_DETAIL_ONLY 1
#define  MSG572_ORDER_ONLY 2
#define  MSG572_ORDERDETAIL_COMBO 3
// follow any number of blocks in this format;
// unsigned char ValueCode; 0=end of Data
// unsigned char DataLentgh : 1-255;
// Comment : Receiving TradeRec is asummed set to 0 , so no 0 values are sent
#define MSG572_MAX_LEN 1024

// Obsolete replaced with 583
/*//580 : Domain Counter Message
typedef struct tdMsg580Rec
{
	char Domain[DOMAIN_LEN];
	DOMAINCOUNTER DomainCounter;
} MSG580REC;
*/

//581 : Equity mesage from CLServer to BOClient
typedef struct tdMsg581Rec
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	float PersEquity;
	float TotalEquity;
	float TotalExposure;
	float EqErrDeleted;
	float MVErrDeleted;
} MSG581REC;

//582 : Open Orders mesage from CLServer to BOClient
typedef struct tdMsg582Rec
{
	int Open;
	TRADEREC TradeRec;
} MSG582REC;


//583 : Domain Counter Message
typedef struct tdMsg583Rec
{
	char Domain[DOMAIN_LEN];
	DOMAINCOUNT DomainCount;
} MSG583REC;

//584 : Open Orders mesage from CLServer to BOClient	- does insert update and delete
typedef struct tdMsg584Rec
{
	int Action;			// 0 = wrong! 1=insert 2=update 3=delete
	long TradeDetRef;
	TRADEREC TradeRec;
} MSG584REC;

//585 : New Domain Counter Message one info peace per line
/*typedef struct tdMsg585Rec
{
	char  Symbol[SYMBOL_LEN];	
	float LastPrice;
	long  Shares;
} MSG585REC;
*/
//586 : Account Information - shares, trades, PnL
/*typedef struct tdMsg586Rec
{
	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char Symbol [SYMBOL_LEN];	

	long   LongShares;
	long   ShortShares;
	long   EquityTrades;
	long   OptionTrades;
	long   EquityShares;
	long   OptionShares;

	double OpenEquity;
	double TotalEquity;
	double OpenPositionsPLMM;
	double ClosedPositionsPLMM;
	double TotalExposureLimitValue;

	double BprOverRide;			// td 3250

	long   NumberOfOrderLines; // DT#5205 INTERNAL - Add info to login so that FE can know how much data to expect
	long   NumberOfPosiLines;// DT#5205 INTERNAL - Add info to login so that FE can know how much data to expect
 
	long   FuturesTrades;		// DT5835 / DT5865 - Back Office - Add new Futures columns to Comp Stats window
	long   FuturesContracts;	// DT5835 / DT5865 - Back Office - Add new Futures columns to Comp Stats window

	char   spare[168];			// DT5835 / DT5865 - Back Office - Add new Futures columns to Comp Stats window

} MSG586REC;
*/
// sent to L2 FE to inform that BD info changed
// for now, domain and account will be filled in and symbol will be blank
typedef struct tdMsg587Rec
{
	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	EXSYMBOL ExSymbol;

}  MSG587REC;

//588 : BD PrevClose updated - need to request new - sent to FE

typedef struct tdMsg588Rec
{
	long L2SessionID;
	long L2MessageID;

	union
	{
		long flags;
		struct
		{
			unsigned short NoDataFound : 1; // Set if  BdPrevCLose is valid
			unsigned short BdPrevCloseSet : 1; //
			unsigned short LastPriceSet  : 1; //3
			unsigned short CheckShortlist  : 1; // DT6361 - 11/17/09 kma - XTrade Gap (CL) -ETB in Level II

			unsigned short OnShortlist  : 1; //// DT6361 - 11/17/09 kma - XTrade Gap (CL) -ETB in Level II
			unsigned short Bit6  : 1; //6
			unsigned short Bit7  : 1; //7
			unsigned short Bit8  : 1; //8
		};
	};
	long Shares;// DT6361 - 11/17/09 kma - XTrade Gap (CL) -ETB in Level II

	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	EXSYMBOL ExSymbol;

	// !!!!!!!!!!! NEVER ADD FIELDS ABOVE THIS MARKER

	double BdPrevClose;
	double LastPrice;

} MSG588REC;

//590 : Cancel All Pending or Batch (Old Dump All Msg)
typedef struct tdMsg590Rec
{
	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User	[USER_LEN];

	long  Type;		// 1 - Cancel All Pending; 2 - SKIPPED DUE TO OLD DUMP ALL CODE; 3 - Cancel All Batch		//=2 to dump  // 4 - Cancel All this Account // DT7549 dla 20101130
	long  BatchNo;	//Batch to Cancel

} MSG590REC;

//591: Hedge Entry
/*typedef struct tdMsg591Rec
{
	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char Symbol [SYMBOL_LEN];	

	long  Type;	// = 0 for normal 1=faked
	long  Maximum;
	long  Available;

	long  Number;
	long  ThisNumber;

} MSG591REC;*/

//592: Request Messages
typedef struct tdMsg592Rec
{
	long SessionID;
	long MessageID;

	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];

	char User[USER_LEN];		//Not used - blank
	char OrderNo[ORDERNO_LEN];	//Not used - blank

	long  Type;	//


} MSG592REC;


// Type field of Msg 593 is  bit set:
#define M_FOR_USER			0x00000001L
#define M_FOR_ACCOUNT		0x00000002L
#define M_FOR_BO			0x00000004L
#define M_FOR_CLEARING		0x00000008L

#define M_FOR_ALL  (M_FOR_USER|M_FOR_ACCOUNT|M_FOR_BO)

#define M_BY_USER			0x00000008L
#define M_BY_TRADER			0x00000010L
#define M_BY_SERVER			0x00000020L


#define M_IS_MESSAGE		0x00000040L
#define M_IS_ERROR			0x00000080L
#define M_IS_WARNING		0x00000100L
#define M_IS_EVENT			0x00000200L

#define M_IS_TRADESTATUS	0x00000400L


//593: Request Messages
typedef struct tM593
{
	long SessionID;
	long MessageID;

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char OrderNo[ORDERNO_LEN];
	char Origin[NAME_LEN];
	long Type;
	long Date;
	long Time;
// + DT7910 - bjl - Variable Length Reject Reason Message
	long DiskOffsetOrMagicNumber; //Either the on-disk offset of the message (old style messages) or INVALID_SET_FILE_POINTER for variable length reject text
								  //When reading off disk, this is used to determine whether we're reading a 593 or 593v message
// - DT7910
	char Text[255];

} MSG593REC, M593;


//594: Request Account Data
typedef struct tdMsg594Rec
{
	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User   [USER_LEN];
	int  Send;

  union
	{
		long DataType;
		struct
		{
			unsigned short Hedges   : 1; //Hedge Data
			unsigned short Messages : 1; //Message Log Data
			unsigned short StockLoans  : 1; //3
			unsigned short Bit4  : 1; //4

			unsigned short Bit5  : 1; //5
			unsigned short Bit6  : 1; //6
			unsigned short Bit7  : 1; //7
			unsigned short Bit8  : 1; //8
		};
	};
	long Reserved;
} MSG594REC;

typedef struct tdMsg600Rec
{
	char Version;				// 0x01 for now
	char Code;					// non-zero = problems, send email anyway
	char szKey[128];			// ini section
	char szEntry[128];			// ini entry
	char szUserMessage[1024];	// message to display
} MSG600REC;

typedef struct tdMsg601Rec
{
	char Version;				// 0x01 for now
	char Code;
} MSG601REC;

// DATABASE messages
// -----------------

// 604 : Update Quote Line
//		 MSG502REC struct



// 611 : Request New User
typedef struct tdMsg611Rec
{
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG611REC;

// 612 : Request New Account
typedef struct tdMsg612Rec
{
	long SessionID;
	long MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG612REC;

// 614: Request New WORKORDER

typedef struct tdMsg614Rec
{
	long SessionID;
	long MessageID;

	int Action;			// 0 = wrong! 1=insert 2=update 3=delete
	unsigned long WorkOrderRefNo;

	WORKORDERREC		WorkOrderRec;
} MSG614REC;



// OBSOLETE CODE
// 615 : Request New Trade
/*typedef struct tdMsg615Rec
{
	long SessionID;
	long MessageID;
	TRADEREC TradeRec;
	long TempTradeDetRef;
} MSG615REC;
*/

// 616: Request Move WORKORDER

typedef struct tdMsg616Rec
{
	long SessionID;
	long MessageID;

	unsigned long WorkOrderRefNo;

    union {
        char	Flags;        // 1111/1111
        struct {
            bool       MoveToNewUser		: 1;	//1 true if NewUser needs to be used
            bool	   MoveToNewGroup		: 1;	//2	true if NewGroup needs to be used
            bool	   Spare3				: 1;	//3
            bool	   Spare4				: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8

        };

    };
	char NewUser[USER_LEN];
	char NewGroup[USER_LEN];
	char Spare[64];

	WORKORDERREC		WorkOrderRec;

} MSG616REC;

// 621 : Request User Info
typedef struct tdMsg621Rec
{
	long L2SessionID;
	long L2MessageID;
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char AccountList;
	char FindUser;
} MSG621REC;

// 622 : Request Account Info
typedef struct tdMsg622Rec
{
	long SessionID;
	long MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char FindAccount;
	char AllDomains;

    union {// kma added this - used 2 spare bytes - to handle more selective queries (for OMS)
        char	QFlags[2];        // 1111/1111  1111/1111
        struct {
            bool       OmsAccountsOnly		: 1;	//1 true if we only want OMS accounts
            bool	   Spare2				: 1;	//2
            bool	   Spare3				: 1;	//3
            bool	   Spare4				: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8

        };

    };

} MSG622REC;

// 623 : Request User-Account Info
typedef struct tdMsg623Rec
{
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG623REC;

// 624: Request WORKORDER info
typedef struct tdMsg624Rec
{
	long SessionID;
	long MessageID;

	int Action;			// 0 = wrong! 1=insert 2=update 3=delete
	unsigned long WorkOrderRefNo;

	WORKORDERREC		WorkOrderRec;
} MSG624REC;



// 625 : Request Trade Info
typedef struct tdMsg625Rec
{
	long SessionID;
	long MessageID;
	char OrderNo[ORDERNO_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char FindByAccount;
	char FromFileDate[9];
	char ToFileDate[9];
	char OpenTradeList;
	long TradeDetRef;
} MSG625REC;

// 626 : Request Trade record - Seaches for a Trade Record if TradeDetRef is 0,
//       otherwise return trade detail  NOT IMPLEMENTED - TradeDetRef MUST BE 0
//		 returns 557 with record MSG557REC
typedef struct tdMsg626Rec
{
	long SessionID;
	long MessageID;
	char OrderNo[ORDERNO_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	long TradeDetRef;
} MSG626REC;

// 627 : Request HEDGE Trade records
// To use: Zero out, and fill in account, domain, and symbol
// if no symbol, all will be sent as
//
typedef struct tdMsg627Rec
{
	long SessionID;		// set on call and returned as is
	long MessageID;		// set on call and returned as is
	long Type;			// 0 for details  1=trades
	long Counter;		// record number - if this is a request, will start at 1, 0 for individual update
	long Spare;

	HEDGE_TRADE Trd;

} MSG627REC;

// 628 : Request Account list
// Send this to a clserver, and it will send accounts which match.
// Depending on DataFormat variable, diff message formats can be returned.
//
typedef struct tdMsg628Rec
{
	long SessionID;
	long MessageID;

	long Version;

	char Account[ACCOUNT_LEN];	// populate with first chars of account to search for (or empty)
	char Domain[DOMAIN_LEN];	// populate with domain being looked for
	char FindAccount;			// true if you want a list search
	char AllDomains;			// dunno

	int DataFormat;		// This deterines what message number will be used to return the list
						// only 672 supported now.



	union {// kma added this - used 2 spare bytes - to handle more selective queries (for OMS)
        char	QFlags[2];        // 1111/1111  1111/1111
        struct {
            bool       OmsAccountsOnly			: 1;	//1 true if we only want OMS accounts
            bool	   AccountsWithDayOrders	: 1;	//2
            bool	   AccountsWithOVOrders		: 1;	//3
            bool	   NeedsAllocations			: 1;	//4	acc's with un-allocated positions only
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8

        };

    };

	char Spare[128];

} MSG628REC;

// Route Level limits CLServer/BackOffice config messages - all comments are for Route Level Limits ONLY
typedef struct tdMsg629Rec
{
	CLIENTINFO  clientInfo;

	// Type of Request/Response message (Back Office send requests, CLServer sends responses)
	enum {
		Query,
		QueryResponse,
		Update,					// Domain,Account,Route fields required,
		UpdateResponse,
		ErrorResponse			// Response error text will be contained in value.val.string member
	} messageType;

	// Level of setting to query for or what level the setting was set at on responses
	enum Level
	{
		Unknown = 'U',			// An error
		System = 'S',			// No domain, account, route or exchange required
		Domain = 'D',			// domain required on query/update
		Account = 'A',			// domain and account required on query/update
		Exchange = 'X',			// exchange required on query/update
		Route = 'R'				// route required on query/update
	} level;

	char name[48];	// Setting name - Leave blank to get all settings for key/level

	char domain[DOMAIN_LEN];	// Domain name - may be blank for System queries
	char account[ACCOUNT_LEN];	// Account name - may be blank for System or Domain queries

	char route[ECNNAME_LEN];	// Route name - may be blank for System, Domain, Account, Exchange queries
	char exchange[ECNNAME_LEN];	// Exchange name - may be blank for System, Domain, Account, Route queries

	// Type of value contained in the union
	enum Type
	{
		unknown,
		boolean,
		string,
		integer,
		floatingPoint,
	} type;

	// The actual value by type name
	union {
		bool boolean;
		char string[128];
		long integer;
		double floatingPoint;
	} value;

	struct
	{
		bool isLocked	: 1;	// True if the setting is locked, level above indicates which level is locked
		bool isValid	: 1;	// True if the value of the setting is valid
		bool isDeleted	: 1;	// True if the setting's value should be deleted
		bool hasInheritance : 1;	// True if this value inherits it's value from a higher level
	};

	struct {
		char spareBuffer[2048];
	} alignMe;

} MSG629REC;

// Instrument Level limits CLServer/BackOffice config messages
typedef struct tdMsg1629Rec
{
	CLIENTINFO  clientInfo;

	long length;
	char xmlText[1];
} MSG1629REC;

const unsigned long Msg1629HdrLen = sizeof(CLIENTINFO) + sizeof(long);

// Example: MSG1629REC* Msg1629Rec = malloc(Msg1629HdrLen + xmlTextLen + 1);
//		or	MSG1629REC* Msg1629Rec = malloc(sizeof(MSG1629REC) + xmlTextLen);
//
//			Msg1629Rec->length = xmlTextLen;
//			strncpy(Msg1629Rec->xmlText, xmlText, xmlTextLen-1;);
//			Msg1629Rec->xmlText[xmlTextLen-1] = '\000';

// 630 : Change User's password
typedef struct tdMsg630Rec
{
	long SessionID;
	long MessageID;

	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char OldPassword[PASSWORD_LEN];
	char NewPassword[PASSWORD_LEN];

	char Status;	// send to CLS as a 0, and CLS will return the same struct with this field =1 is successfull, or 2 if not.
					// On error, this will be followed by Msg570 with the actual error message
					// Value of 3 is sent from CLS to DNS for Routing to the correct CLS
					
					// When FRONETED receives >2 result, its an error, but it only displays a line to say the attempt failed.
					// Frontend gets a 2, it displays the BOFA explanation as well as failed message
					

	char EntitlementDomain[DOMAIN_LEN];

} MSG630REC;

// 1630 : Change a different User's password
typedef struct tdMsg1630Rec
{
	long SessionID;
	long MessageID;

	char AdminUser[USER_LEN];	// Id of user making the change, if AdminUser == User, no BORights reqd
	char Domain[DOMAIN_LEN];	// Domain of User

	char User[USER_LEN];
	char OldPassword[PASSWORD_LEN];
	char NewPassword[PASSWORD_LEN];

	char EntitlementDomain[DOMAIN_LEN];	// Populated by DNS, may be same as Domain

	char RequestorUuid[16];				// Populated by DNS
	int RequestorPortNo;				// Populated by DNS

	char Status;						// FE apps set to 0, CLS will set to 1 = success or 2 = error

	char Reason[128];					// Empty string or failure reason if status = 2

	char spare[128];

} MSG1630REC;

// 631 : Update User Record
typedef struct tdMsg631Rec
{
	long SessionID;
	long MessageID;
	USERREC UserRec;
} MSG631REC;

// 632 : Update Account Record
typedef struct tdMsg632Rec
{
	long SessionID;
	long MessageID;
	ACCOUNTREC AccountRec;
} MSG632REC;

// 633 : Update User-Account Record
typedef struct tdMsg633rec
{
	long SessionID;
	long MessageID;
	USERACCREC UserAccRec;
} MSG633REC;

// 634: Update WORKORDER


// 635 : Update Trade Record
typedef struct tdMsg635Rec
{
	long SessionID;
	long MessageID;
	TRADEREC TradeRec;
	long TradeDetRef;	//	0=new -1=move, -2=delete trade, -3=Del all overnight, -5=Delete Line in TempTradeDetRef, -6=Assign Order to WorkOrder specified by TempTradeDetRef
	long TempTradeDetRef;
} MSG635REC;

// 636 : Update Account Record at End Of Day
typedef struct tdMsg636Rec
{
	long SessionID;
	long MessageID;
	ACCOUNTREC AccountRec;
} MSG636REC;

// 637 : Split Trade accross accounts
// uses MSG822REC structure to pack the information:
// FileDesc = OrderNumber
// Domain = Domain
// Block: first line contains a 1
// Second line is the original TradeDetRef of the order being split
// Rest is a line by line breakdown:
// containing the Account Number followed by a comma, and the amount.


// Monitor Messages: - Action field of 638

#define MONITOR_DUMPALL				2
#define MONITOR_SUSPEND_ACCOUNT		3
#define MONITOR_RESUME_ACCOUNT		4

// Account monitor message structure
typedef struct tdMsg638Rec
{
	long SessionID;
	long MessageID;

	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	int  Action;

} MSG638REC;

typedef struct tdMsg639Rec
{
	CLIENTINFO  ClientInfo;
	char		Action;		// MAKE IT 1 for now
	USERDOCREC  UserDocRec;
	char Spare[40];

} MSG639REC;



// 641 : Delete User Record
typedef struct tdMsg641Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];

	char Spare[512];

} MSG641REC;

// 642 : Delete User-Account Record
typedef struct tdMsg642Rec
{
	long SessionID;
	long MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG642REC;

// 643 : Delete User-Account Record
typedef struct tdMsg643Rec
{
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG643REC;

// 644: Delete WORKORDER // NOT USED = covered by 634

// 651 : User Info
typedef struct tdMsg651Rec
{
	long L2SessionID;
	long L2MessageID;
	long SessionID;
	unsigned long MessageID; // DT7549 DLA 20110104 - Pairs Trading
	USERREC UserRec;
} MSG651REC;

// 652 : Account Info
typedef struct tdMsg652Rec
{
	long SessionID;
	long MessageID;
	ACCOUNTREC AccountRec;
} MSG652REC;

// 653 : User-Account Info
typedef struct tdMsg653Rec
{
	long L2SessionID;
	long L2MessageID;
	long SessionID;
	long MessageID;
	char Password1[PASSWORD_LEN];
	USERACCREC UserAccRec;
} MSG653REC;

// 654 : WORKORDER List

//xxxx : Work Order Message

typedef struct tdMsg654Rec
{
	long SessionID;
	long MessageID;

	int Action;			// 0 = wrong! 1=insert 2=update 3=delete
	unsigned long WorkOrderRefNo;

	WORKORDERREC		WorkOrderRec;
} MSG654REC;


// 655 : Trade Info
typedef struct tdMsg655Rec
{
	long SessionID;
	long MessageID;
	TRADEREC TradeRec;
	long TradeDetRef;
} MSG655REC;

// 657 : OpenPostion Record
typedef struct tdMsg657Rec
{
	long L2SessionID;
	long L2MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char Remove;
	POSIREC PosiRec;
} MSG657REC;

// 658 : Request Trade Info - BigTradeRec Edition - Bud 06/09/06 TD#5149
typedef MSG625REC MSG658REC;

// 659 : Trade Info	- BigTradeRec Edition - Bud 06/09/06 TD#5149
typedef struct tdMsg659Rec
{
	long SessionID;
	long MessageID;
	BIGTRADEREC BigTradeRec;
	long TradeDetRef;
} MSG659REC;

// 660 : Update Trade Record - BigTradeRec Edition - Bud 06/09/06 TD#5149
typedef struct tdMsg660Rec
{
	long SessionID;
	long MessageID;
	BIGTRADEREC BigTradeRec;
	long TradeDetRef;	//	0=new -1=move, -2=delete trade, -3=Del all overnight, -5=Delete Line in TempTradeDetRef, -6=Assign Order to WorkOrder specified by TempTradeDetRef, -7=Update
	long TempTradeDetRef;
} MSG660REC;


// 661 : User Responce
typedef struct tdMsg661Rec
{
	long L2SessionID;
	long L2MessageID;
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file
} MSG661REC;

// 662 : Account Responce
typedef struct tdMsg662Rec
{
	long SessionID;
	long MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file
} MSG662REC;

// 663 : User-Account Responce
typedef struct tdMsg663Rec
{
	long L2SessionID;
	long L2MessageID;
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file (21 = end)
} MSG663REC;

// 664 : WORKORDER Response  = use 654

// 665 : Trade Responce
typedef struct tdMsg665Rec
{
	long SessionID;
	long MessageID;
	char OrderNo[ORDERNO_LEN];
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file
	long TempTradeDetRef;
	long TradeDetRef;
} MSG665REC;

// 667 : OpenPostion Response
typedef struct tdMsg667Rec
{
	long L2SessionID;
	long L2MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file
} MSG667REC;

// 669 : User-DOC Responce
typedef struct tdMsg669Rec
{
	CLIENTINFO ClientInfo;

	char User[USER_LEN];
	char DocId[DOCID_LEN];
	char Domain[DOMAIN_LEN];
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file (21 = end)
} MSG669REC;
// + DT11185 - bjl - BO Feed server monitoring
typedef struct
{
	unsigned long IQClientIP;
	unsigned long IQClientATSIP;
	unsigned short IQClientPort;
	unsigned short IQClientATSPort;
} IQCLIENTIPS;
// - DT11185
// 671 : UserList
typedef struct tdMsg671Rec
{
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char LastName[NAME_LEN];
	char FirstName[NAME_LEN];
	IQCLIENTIPS IQClientIPs; // DT11185 - bjl - BO Feed server monitoring
	long CLServerPort;
	long AccountMonitorPort;
	char NewFirstName[NAME_LEN]; // Used when BO_FIRSTNAME_IS_ACCOUNT is set // DT11185 - bjl - BO Feed server monitoring
} MSG671REC;

// 672 : AccountList
typedef struct tdMsg672Rec
{
	long SessionID;
	long MessageID;
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
	char LastName[NAME_LEN];
	char FirstName[NAME_LEN];
} MSG672REC;

// 675 : TradeList
typedef struct tdMsg675Rec
{
	long SessionID;
	long MessageID;
	TRADEREC TradeRec;
	long TradeDetRef;
	char DetailOnly;
} MSG675REC;


// 677 : OpenPostion List
typedef struct tdMsg677Rec
{
	long SessionID;
	long MessageID;
	POSIREC PosiRec;
} MSG677REC;

// 678 : Position  Price Update
typedef struct tdMsg678Rec
{
	EXSYMBOL ExSymbol;
	double P_L_Long_Price;
	double P_L_Short_Price;
} MSG678REC;

// 679 : Account has been reloaded message
typedef struct tdMsg679Rec
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[NAME_LEN];
	long Flags;

} MSG679REC;

// 680 : boclient ---> clserver - mass data edit
typedef struct tdMsg680Rec
{
	unsigned char ver;
	bool bCommit;
	char Domain[DOMAIN_LEN];
	unsigned int iRecordSize;
	unsigned int iSearchOffset;
	unsigned int iReplaceOffset;
	unsigned char iSearchDataType;
	unsigned char iReplaceDataType;
	char szSearchText[256];
	char szReplaceText[256];
} MSG680REC;

// 681 : clserver ---> boclient - mass data edit verification msg
typedef struct tdMsg681Rec
{
	unsigned char ver;
	char Domain[DOMAIN_LEN];
	int BufLen;
} MSG681REC;

// Criteria used for Back Office cancel pending orders MSG690
typedef struct tdCancelCriteria
{
	char Account[ACCOUNT_LEN];			// Required

	char Ecn[ECN_LEN];					// Optional, only cancels pending orders on ecn specified
    union {
        unsigned long	Flags;
        struct {
            bool CancelGTC		: 1;	// True if GTC orders should be canceled
            bool Domainwide		: 1;	// True if cancel applies to ALL accounts on domain
            bool Spare3			: 1;
            bool Spare4			: 1;
            bool Spare5			: 1;
            bool Spare6			: 1;
            bool Spare7			: 1;
            bool Spare8			: 1;
			bool Spare9			: 1;
        };
    };

	char SpareBytes[256];
} CANCELCRITERIA;

//690 : Back Office Cancel All Pending for Account
typedef struct tdMsg690Rec
{
	char Domain[DOMAIN_LEN];			// Required
	char BOPasswd[PASSWORD_LEN];		// Required

	CANCELCRITERIA Criteria;

	char SpareBytes[256];
} MSG690REC;


// 699 : Account Changed reload all data
/*typedef struct tdMsg699Rec
{
	char Account[ACCOUNT_LEN];
	char Domain[DOMAIN_LEN];
} MSG699REC;
*/
// TRADESVR messages
// -----------------
// 700 : Incomming Heart Beat
//     : NO DATA

// 701 : Order Request from client server
//     : TRADEREC STRUCT

// THIS MSG  IS REDUNDENT WAS REPLACES WITH 710
// 702 : Order Status from Trade Parser
//     : TRADEREC STRUCT

// 703 : Cancel Request from client server
//     : TRADEREC STRUCT

// THIS MSG  IS REDUNDENT WAS REPLACES WITH 710
// 704 : Rejection Explanation Message from TradeParser
//
typedef struct tdMsg704Rec
{
	EXSYMBOL ExSymbol;
	int  ReasonNo;	// 1 = normal Reject // 2 = Cancel Rejected
	char Reason[80];
	char OrderNo[ORDERNO_LEN];
} MSG704REC;

// 705 : Cancel All Pending Order For Domain
typedef struct tdMsg705Rec
{
	char Domain[DOMAIN_LEN];
} MSG705REC;

// 706 : Advertise Domains to TradeParser
typedef struct tdMsg706Rec
{
	char Domain[DOMAIN_LEN];
} MSG706REC;

// 707 : Request Total Resend to TradeParser
typedef struct tdMsg707Rec
{
	long LastReportLogNo;
} MSG707REC;

// 708 : Cancel Pending Record
typedef struct tdMsg708Rec
{
	char DomainToDelete[DOMAIN_LEN];

	TRADEREC TradeRec;

} MSG708REC;


// 709 : Orginal Trade Placed
//     : TRADEREC STRUCT

// 710 : New Order Status from Trade Parser
typedef struct tdMsg710Rec
{
	long ReportLogNo;
	long RejectReasonNo;
// + DT8953 - bjl
	union
	{
		char RejectReasonText[128];
		struct
		{
			char ECNParserOrderNo[24];
			char TraderGiveUp[8];
			char Spare1[91];
			char Liquidity[2]; //Not NULL terminated
			char Spare2[3];
		};
	};
// - DT8953
	TRADEREC TradeRec;
} MSG710REC;

// 711 : Parser Status from Trade Parser
typedef struct tdMsg711Rec
{
	int Status; //0 : OffLine 1 : OnLine

} MSG711REC;

// 712 : Cancel All Pending Order For Domain and ECN
typedef struct tdMsg712Rec
{
	char Domain[DOMAIN_LEN];
	char Ecn   [ECN_LEN];

} MSG712REC;


// 713 : PHASE OUT with 718 Garaunteed Delivery cleanup
typedef struct tdMsg713Rec
{
	long CleanUpToReportLogNo;
} MSG713REC;


// 714 : Software Version from Trade Parser
typedef struct tdMsg714Rec
{
	int Version; //0 : OffLine 1 : OnLine
} MSG714REC;

// 715 : Cancel All Pending Order For Domain - with selections
typedef struct tdMsg715Rec
{
	char Domain[DOMAIN_LEN];
	long KeepGtc;// expand to flags later
	char Spare[80];

} MSG715REC;

// 717 : Request Total Resend to TradeParser
// BAO - If you get a 717 on a PORT - ignore any 707's on the same port.
// Not all the CLservers will be on the new version

typedef struct tdMsg717Rec
{
	long LastReportLogNo;
	long SessionDate;				// WSDATE

    union {
        char	Flags;        // 1111/1111
        struct {
            bool       WarnIfSessionNameDiff	: 1;	//1 true if traders should WSLOGERROR
            bool	   ErrorIfSessionNameDiff	: 1;	//2	Dont allow connection/playback
            bool	   ReportLog762				: 1;	//3	true if traders should report with MSG762REC instead of MSG710REC
            bool	   Spare4				: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7 FUTURRE
            bool	   Spare8				: 1;	//8 FUTURE

        };

    };
	char SessionName[256];
	char SpareBlock[256];

} MSG717REC;

// 718 : Garaunteed Delivery cleanup
typedef struct tdMsg718Rec
{
	long CleanUpToReportLogNo;
	long SessionDate;
    union {
        char	Flags;        // 1111/1111
        struct {
            bool       Spare1				: 1;	//1
            bool	   Spare2				: 1;	//2
            bool	   Spare3				: 1;	//3
            bool	   Spare4				: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8
        };
	char cSpare[128];
    };
} MSG718REC;

// 719 : Garaunteed Delivery Admin Send Direct do not place in GDLog or GDQue
typedef struct tdMsg719Rec
{
	long ReportLogNo;
	long SessionDate;
    union {
        char	Flags;        // 1111/1111
        struct {
            bool       FixSessionDate		: 1;	//1
            bool	   Spare2				: 1;	//2
            bool	   Spare3				: 1;	//3
            bool	   Spare4				: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8
        };
	char cSpare[128];
    };
} MSG719REC;

// 721 : Parser Status from Trade Parser
typedef struct tdMsg721Rec
{
	int Suspended; //0 : Available, <> 0 is Suspended
	int Spare;	// could be non-zero

	// Begin DT7278 bud 08/09/10
	union {
		int TraderFlags;
		struct {
			bool	SupportsBTG			: 1;	// 1 - Supports Binary Tags & Grouping
			bool	Spare2				: 1;    // 2
			bool	Spare3				: 1;    // 4
            bool	Spare4 				: 1;	// 8
            bool	Spare5				: 1;	// 16
            bool	Spare6				: 1;	// 32
            bool	Spare7				: 1;	// 64
            bool	Spare8				: 1;	// 128
			bool	Spare9				: 1;    // 256
			bool	Spare10				: 1;	// 512
			bool	Spare11				: 1;    // 1024
			bool	Spare12				: 1;	// 2048
			bool	Spare13				: 1;	// 4096
			bool	Spare14				: 1;	// 8192
			bool	Spare15				: 1;	// 16384
			bool	Spare16				: 1;	// 32768
		};
	};
	// End DT7278 bud 08/09/10

	int version;
	char SenderCompID[16];
	char TargetCompID[16];
	union {
		unsigned short SupportedSecurityTypes;
		struct {
			bool	   Equities				: 1;    //1
			bool	   Options				: 1;    //2
			bool	   Futures				: 1;    //3
            bool	   Algo 				: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8
			bool       Spare9				: 1;    //9
			bool       Spare10				: 1;	//10
			bool       Spare11				: 1;    //11
			bool       Spare12				: 1;	//12
			bool       Spare13				: 1;	//13
			bool       Spare14				: 1;	//14
			bool       Spare15				: 1;	//15
			bool       Spare16				: 1;	//16
		};
	};
	union {
		unsigned short  SupportedTIFs;
		struct {
			bool	   Day					: 1;    //1
			bool	   GTC					: 1;    //2
			bool	   OnOpen				: 1;    //3
			bool	   IOC					: 1;    //4
			bool	   FOK					: 1;    //5
            bool	   GTX					: 1;	//6
            bool	   GTD					: 1;	//7
            bool	   OnClose				: 1;	//8
			bool       Ext					: 1;    //9
			bool       Spare10				: 1;	//10
			bool       Spare11				: 1;    //11
			bool       Spare12				: 1;	//12
			bool       Spare13				: 1;	//13
			bool       Spare14				: 1;	//14
			bool       Spare15				: 1;	//15
			bool       Spare16				: 1;	//16
		};
	};
	union {
		unsigned short SupportedOrderTypes;
		struct {
			bool	   Facilitated			: 1;    //1
			bool	   Spread				: 1;    //2
			bool	   eStops				: 1;    //3
			bool	   PegMkt				: 1;    //4
			bool	   PegMid				: 1;    //5
            bool	   PegBest				: 1;	//6
            bool	   PegPrime				: 1;	//7
            bool	   PegLast				: 1;	//8
			bool       BookOnly				: 1;    //9
			bool       Discretion			: 1;	//10
			bool       ReservedOrders		: 1;    //11
			bool       HiddenOrders			: 1;	//12
			bool       AON   				: 1;	//13
			bool       NotHeld				: 1;	//14
			bool       MidPointMatch		: 1;	//15 // DT6958 bud 08/25/10
			bool       Spare16				: 1;	//16
		};
	};
	union {
		unsigned short SupportedCurrencies;
		struct {
			bool	   USD      			: 1;    //1
			bool	   Canadian				: 1;    //2
			bool	   Spare3				: 1;    //3
			bool	   Spare4				: 1;    //4
			bool	   Spare5				: 1;    //5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8
			bool       Spare9				: 1;    //9
			bool       Spare10				: 1;	//10
			bool       Spare11				: 1;    //11
			bool       Spare12				: 1;	//12
			bool       Spare13				: 1;	//13
			bool       Spare14				: 1;	//14
			bool       Spare15				: 1;	//15
			bool       Spare16				: 1;	//16
		};
	};
	char sparebuf[156];
} MSG721REC;


// 736 : Extra trader fields // DT4721 Bud 12/17/08
typedef struct tdMsg736Rec
{
	long MessageVersion;		// always 1 for now
	long MessageType;			// 1 - Additional BigTradeRec Fields
								// 2 - MLAlgo Route Tags

	char Domain[DOMAIN_LEN];	// Domain and OrderNo uniquely identify the (possible) msg that follows
	char OrderNo[ORDERNO_LEN];

	char Account[ACCOUNT_LEN];	// Future use
	char User[USER_LEN];		// Future use

	long TextLength;			// Length of the data which follows

	char *Data;					// sent in as null, makes it easier to access data.

	// text data follows

} MSG736REC;
// end DT4721 Bud 12/17/08

// 737 : Send Last Stock Loan OrderNo by Domain received from CLServer following reconnect
typedef struct tdMsg737Rec
{
  char Domain[DOMAIN_LEN];
  char OrderNo[ORDERNO_LEN];
} MSG737REC;

// 749 : Place Order at Trade Parser
//     : BIGTRADEREC STRUCT Complex Order

// 751 : Place Order at Trade Parser
//     : TRADEREC STRUCT

// 752 : Updated Order Status to Client
//     : TRADEREC STRUCT


// 753 : Place Cancel at Trade Parser
//     : TRADEREC STRUCT
// 1753 : Place cancel at Trade Parser
//     : BIGTRADEREC STRUCT
typedef struct tdMsg1753Rec
{
	TRADEREC TradeRec;
	char OptionalData[256];

} MSG1753REC;


// 754 : Rejection Explanation Message to Client
typedef struct tdMsg754Rec
{
	int  ReasonNo;	// 0 = normal Reject // 1 = short rule try 1/16 up // 99 = cancel rejected
	char Reason[80];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
} MSG754REC;

// 755 : Trade Rejection/Info Message to Client	// DT#344
typedef struct tdMsg755Rec
{
	char		Domain[DOMAIN_LEN];
	char		Account[ACCOUNT_LEN];
	char		User[USER_LEN];
	char		OrderNo[USER_LEN];
	EXSYMBOL	ExSymbol;
	int			Action;
	long		Shares;
	double		Price;

	char		ReasonNo;	// same as 570 ReasonNo
							// 3 destroys blue place order box, 99=resets the account and calls LOGIN again
							// 4=fill warning, 1=reject
	WORD DisplayYN;

	long L2SessionID;
	long L2MessageID;

    union
	{
        long	Flags;        // 1111/1111  - These flags used to denote what type of message is being sent.
        struct
		{
            bool       Error				: 1;	//1
            bool	   Warning				: 1;	//2
            bool	   Information			: 1;	//3
            bool	   ShortSellReject		: 1;	//4
            bool	   Spare5				: 1;	//5
            bool	   Spare6				: 1;	//6
            bool	   Spare7				: 1;	//7
            bool	   Spare8				: 1;	//8
        };
	};

	char Lines[6][81];

	char Spare[256];
} MSG755REC;

// 756 : DropCopy / Synced Traderec
typedef struct tdMsg756Rec
{
	long ReportLogNo;

	// just to make sure we have space for options
	long TimeConfirmed;// time_t version assigned to a long (TIME_T is actually a long)
	char OfficeCode[4];
	char Spare3[4];

	TRADEREC TradeRec;

} MSG756REC;

// 757 : Place Order at Trade Parser
//     : BIGTRADEREC STRUCT

// 1757 : Place Order at Trade Parser
//     : BIGTRADEREC STRUCT
typedef struct tdMsg1757Rec
{
	BIGTRADEREC BigTradeRec;
	char OptionalData[256];

} MSG1757REC;



// 758 : DropCopy / Synced BigTraderec
// 759 : DropCopy / Synced BigTraderec - MASTER ORDERS
typedef struct tdMsg758Rec
{
	long ReportLogNo;

	// just to make sure we have space for options
	long TimeReceived;	// time_t version assigned to a long (TIME_T is actually a long)
	long TimeConfirmed;	// time_t version assigned to a long (TIME_T is actually a long)

	char OfficeCode[4];
	char RRNumber[5];
	char ClearingAccount[ACCOUNT_LEN];

	char ReportingParty;	// DT6089 bud 10/27/09
	union {					// + DT9576 DLA 20120201
		char Notes[40];			// DT6089 bud 10/27/09
		char Entity[20];
	};						// - DT9576 DLA 20120201
	char ComCode;			// DT6376 YL 20091124
	char Spare[5];			// DT6376 YL 20091124

	long SettlementDate;	// DT6089 YL 20091012

	BIGTRADEREC BigTradeRec;

} MSG758REC;



// 761 : Place Order from FIXSERVER
//     : BIGTRADEREC STRUCT

// 762 : New Order Status to FIXSERVER
typedef struct tdMsg762Rec
{
	long ReportLogNo;
	long RejectReasonNo;
	char RejectReasonText[128];
	BIGTRADEREC BigTradeRec;
} MSG762REC;

// 1762 : New Order Status to FIXSERVER with Optional Data
typedef struct tdMsg1762Rec
{
	MSG762REC Msg762Rec;
	char OptionalData[256];
} MSG1762REC;

// 763 : Place Cancel from FIXSERVER
//     : TRADEREC STRUCT

// 770 : Domain list from TRADERS to
typedef struct tdMsg770Rec
{
	char Domain[DOMAIN_LEN];
	char GiveUp[GIVEUP_LEN];
	char EcnName[ECN_LEN];
	char Description[80];
} MSG770REC;

//NS++
//This command string is used by CLServer and Eye. This will ensure that if this string is changed both exe
//will get updated since this H file is shared by both EXE
#define MSG799_UPGRADE_EXE_COMMAND "Upgrade Executable"
//NS--

typedef struct tdMsg799Rec
{
	long SessionID;
	long MessageID;

	int Action;	// 0=part of list 1=query (returns list) 2=Response after Execute SUCCESS   3=CMD FAILED 4=UNKNOWN CMD   17=Execute
	char Command[30];
	char Descr[80];

	char DomainName[DOMAIN_LEN];
	char ExeName[50];  // executable name to use for upgrade command.
	int  Confirm;  // 1=yes 0=no
	char Spare[246];

} MSG799REC;

//800: Send to clserver - to request domain list
//		CLS replies with MSG999REC - one for each domain

// BACK OFFICE mesages
// -------------------
// 801 : Request bo user login

typedef struct tdMsg801Rec
{
	long SessionID;
	long MessageID;
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	char Password[PASSWORD_LEN];
} MSG801REC;

// 805 : Request Domain's CamcodeSet
typedef struct tdMsg805Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
} MSG805REC;

// 806 : Request Acc CamcodeSet from Frontend
//typedef struct tdMsg805Rec


// 807 : Update Domain's Camcode
typedef struct tdMsg807Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
//	CAMCODESET CamCodeSet;
	int Delete;
} MSG807REC;

// 808 : Domain Camcode Set
typedef struct tdMsg808Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	long SetSize;
	bool LastBlock;
} MSG808REC;
//CAMCODESET CamCodeSet Array to Follow in VarLen Block;

// 809 : Domain Camcode  Value Set/Get
typedef struct tdMsg809Rec
{
	long SessionID;
	long MessageID;

	char Action;				//1=request send to client, 2=set value , 3=edit, 4=delete
	char Domain[DOMAIN_LEN];

	CAMCODE CamCode;

} MSG809REC;


// 810 : Domain Camcode Set
typedef struct tdMsg810Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	long SetSize;
	bool LastBlock;
} MSG810REC;
//CAMCODESET CamCodeSet Array to Follow in VarLen Block;

// 811 : Detail Query - returns message 859

typedef struct tdMsg811Rec
{
	long SessionID;
	long MessageID;

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];

	long OrderStartTime;	// not implemented
	long OrderEndTime;		// not implemented
	long FillStartTime;
	long FillEndTime;
    long NumRecords;

	union
	{
		char flags;
		struct
		{
			bool PendingOnly : 1;
			bool UseQueryFields: 1;	// kma ex[periment
			bool ExecutionsOnly: 1;	// kma ex[periment
			bool FindLinkedOrders : 1;		// DT5454 Bud 05/14/09
			bool ReturnOrders : 1;		// DT5593 YL 20090604	// DT5593A YL 20090625
		};
	};

	char SendNextHistBlocks;  // NOT USED

	// Query Fields
	char OrderNo[ORDERNO_LEN];
	char ECNOrderNo[ORDERNO_LEN];
	char Symbol[24];	
	char ECNId[ECN_LEN];

	double OrderPriceFrom;	// search order price
	double OrderPriceTo;	// Search Order Price

	double FillPriceFrom;	// search Execution price
	double FillPriceTo;		// Search Execution Price

	// Query Fields

	char SpareBuf[430];

}MSG811REC;

// 815 : AddTrade
typedef struct tdMsg815rec
{
	long Type ; // 1 : intraday // 2 : overnight
	TRADEREC TradeRec;
} MSG815REC;

// 816 : ImportTrade
typedef struct tdMsg816Rec
{
	TRADEREC TradeRec;
} MSG816REC;


// 820 : Reset Account to Penson
typedef struct tdMsg820RecOld
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
} MSG820REC_OLD;

// 820 : Reset Account to Penson
typedef struct tdMsg820Rec
{
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
#if BUILD_NUMBER > 1032
	long ReloadType;	// 1=IQ 2=IQ Morning, 3=reset to penson, 5=Delete single ACCOUNTs OV positions
	char Spare[32];
#endif
} MSG820REC;

// 821 : Text File Request
typedef struct tdMsg821Rec
{
	int Type;			//	1=Short List 2=Comms File List, 3=Specific Comms file
	char Domain[DOMAIN_LEN];
	char FileName[80];
} MSG821REC;

// 822 : Text Data Send
typedef struct tdMsg822Rec
{
	int  Type;
	char Domain[DOMAIN_LEN];
	int  Size; //if size not = 1024 assume eof
	int  BlockNo;
	char FileDesc[80];
	char Block[MSG822BUFSIZE];
} MSG822REC;

// 1822 : Text Data Send (handles longer file/paths)
typedef struct tdMsg1822Rec
{
	int  Type;
	char Domain[DOMAIN_LEN];
	int  Size; //if size not = 1024 assume eof
	int  BlockNo;
	char FileDesc[MAX_PATH+1];
	char Block[MSG1822BUFSIZE];
} MSG1822REC;

// TEXT FILE TYPE
// 1 : Shortlist

// 823 : NEVER USED - Update Short List Data Send - single line - Insert/update/delete
typedef struct tdMsg823Rec
{
	int  Operation;	// 0=Save&done 1=Update/insert  2=delete

	char Domain[DOMAIN_LEN];

	char Symbol[20];	
	long Value;

} MSG823REC;

// 824 : Test Coms Request
typedef struct tdMsg824Rec
{
	char Domain[DOMAIN_LEN];
	char CommissionStruct[20];
	double ComsPar[3];
	char ECNId[ECN_LEN];
	char MMId[ECN_LEN];
	long Shares;
} MSG824REC;

// 825 : Get / Set OATS parameters
typedef struct tdMsg825Rec
{
	char GetSet;   // get==1, set=2
	char Domain[DOMAIN_LEN];
	char FilePrefix[80];
	char UserId[80];
	char Password[80];
	char OsoCode[80];
	char GiveUp[80];
	bool UseSimpleFileName;
	long Flags;
} MSG825REC;

// 826 : Text File Request (FTP FILE TRANSFER XFER)
typedef struct tdMsg826Rec
{
	long SessionID;
	long MessageID;
	int Type;			//	 2=Comms File / List, 5=Applies To file / list;  7=Domain Ini file / list; 9=Account Route.txt; 10=Domain Route.txt
	char Domain[DOMAIN_LEN];
	char FileName[80];
	int List;
} MSG826REC;

// 827 : File List
typedef struct tdMsg827Rec
{
	long SessionID;
	long MessageID;
	int  Type;
	char Domain[DOMAIN_LEN];
	long TotalFileSize;
	char Bof;
	char Eof;
	char FileName[80];
	char FileDesc[80];
} MSG827REC;

// 828 : File Header
typedef struct tdMsg828Rec
{
	long SessionID;
	long MessageID;
	int  Type;
	char Domain[DOMAIN_LEN];
	long TotalFileSize;
	char FileName[80];
	char FileDesc[80]; //Place DOC_ID Here is reponse to 669:
} MSG828REC;

// 829 : File Data Block
typedef struct tdMsg829Rec
{
	long SessionID;
	long MessageID;
	long BlockSize;
	char Eof;
	char Block[MSG829BUFSIZE];
} MSG829REC;

// 830 : Log request sent to traders and response goes to the eye
typedef struct tdMsg830Rec
{
	char Domain[DOMAIN_LEN];
	long StartTime;
	long EndTime;
	long ReqType;	// 1=Place Log, 2=ReportLog, 3=Cancel Log, 4=In Log, 5=Out Log, 6=New In Log, -1=Cancel Send
} MSG830REC;

// 831 : Response to Log request sent to eye from traders
typedef struct tdMsg831Rec
{
	char Domain[DOMAIN_LEN];
	char TraderId[80];
	int  NoUpdate;
	int  ECN;
	int  ECN_TYPE;
	int  SeqNo;
	long ReqType;	// 1=Place Log, 2=ReportLog, 3=Cancel Log, 4=In Log, 5=Out Log, 6=New In Log, 7=Stuck Order, 8=User Log, 9=Open Orders
  char Block[MSG831BUFSIZE];
} MSG831REC;

// 1831 : *NEW* Response to Log request sent to eye from traders
typedef struct tdMsg1831Rec
{
	char Domain[DOMAIN_LEN];
	char TraderId[80];
	int  NoUpdate;
	int  ECN;
	int  ECN_TYPE;
	int  SeqNo;
	long ReqType;	// 1=Place Log, 2=ReportLog, 3=Cancel Log, 4=In Log, 5=Out Log, 6=New In Log, 7=Stuck Order, 8=User Log, 9=Open Orders
  char Block[MSG1831BUFSIZE];
} MSG1831REC;

// 832 : request log entries for specific orderno
typedef struct tdMsg832Rec
{
	char Domain[DOMAIN_LEN];
  char OrderNo[ORDERNO_LEN];
  long RequestNo;
} MSG832REC;

// 833 : Free Format Text Responce to Msg832
typedef struct tdMsg833Rec
{
  long RequestNo;
  int DataLen;
  long Start;
  long End;
	int  ECN_TYPE;
} MSG833REC;

// 834 : Login Request (Eye -> Traders)
typedef struct tdMsg834Rec
{
	char User[40];
	char Pwd[40];
	long EyeVersion;
} MSG834REC;

// 835 : Login Response (Traders -> Eye)
typedef struct tdMsg835Rec
{
	char User[40];
	int  User_Rights;
} MSG835REC;

// 836 : Admin Order Overrride
typedef struct tdMsg836Rec
{
	int    OrderType; //0=Confirm, 1=Reject, 2=Fill, 3=Cancel, 4=Expire, 5=Confirm Cancel
	char   User[40];
	char   ParserOrderNo[ORDERNO_LEN];
	char   Contra_RejectReason[128];
	int    Shares;
	double Price;
} MSG836REC;

// 837 : Bulk Message Response for Lof Request Traders -> Eye (MAX NUM MSGS = xx)
typedef struct tdMsg837Rec
{
	int NumMsgs;
  MSG831REC MsgBlock[MAX_NUM_MSGS837];
} MSG837REC;

// 1837 : *NEW* Bulk Message Response for Lof Request Traders -> Eye (MAX NUM MSGS = xx)
typedef struct tdMsg1837Rec
{
	int NumMsgs;
  MSG1831REC MsgBlock[MAX_NUM_MSGS1837];
} MSG1837REC;

// 838 : Cancel Order Request from Trade Mon (EYE)
typedef struct tdMsg838Rec
{
  TRADEREC TradeRec;
	char   User[40];
} MSG838REC;

// 839 : Bulk Message Response complete for Traders -> TradeMon
typedef struct tdMsg839Rec
{
	int NumMsgs;
} MSG839REC;

typedef struct tdMsg840Rec
{
	long SessionID;
	long MessageID;

	int	 ReqAction;		// what this message must do: list all, one, etc
	int  RecType;		// 1 = Aggregate record 2 = Allocation record

	int  RecNum;		// 1 = Aggregate record 2 = Allocation record

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];

	EXSYMBOL ExSymbol;
	int		 SideAction;

	long	Shares;
	char Spare1[4]; // DT11309 - bjl - R4 2012- Enable Cost+ Commissions from Allocations Window
	double	Price;

	long	SharesAllocated;
	char Spare2[4]; // DT11309 - bjl - R4 2012- Enable Cost+ Commissions from Allocations Window
	double  CostAllocated;

    union {
        char	Flags;        // 1111/1111
        struct {
            bool       OnlyOneAllocationDefault				: 1;	//1 true id only one allocation account set
            bool	   AccountDefaultAllocationsBalance		: 1;	//2	true if defaults balance to 100%
            bool	   ResolveContractInformation			: 1;	//3 true if requesting additional msg 844 for option contract information
// + DT11309 - bjl - R4 2012- Enable Cost+ Commissions from Allocations Window
            bool	   ShowComms							: 1;	//4 true if BO should show commissions on allocations
            bool	   SpareFlag5							: 1;	//5
            bool	   SpareFlag6							: 1;	//6
            bool	   SpareFlag7							: 1;	//7 
            bool	   SpareFlag8							: 1;	//8 
// - DT11309

        };

    };
	char	 OrderNo[ORDERNO_LEN];
	char	AccType;

	char Portfolio;	// H=Hard$, S=Soft$
// + DT11309 - bjl - R4 2012- Enable Cost+ Commissions from Allocations Window
	char Spare3[5];
	double Comms;
	double ECNFee;
	double CommsAllocated;
	char Spare4[24];
// - DT11309


} MSG840REC;


#define NORMAL_COMM			  0
#define CLEARING_ASSIGNS_COMM 1
#define FORCE_ZERO_COMM		  2
#define COMM_IS_PER_SHARE     3
#define COMM_IS_FIXED_AMOUNT  4
#define COMM_IS_PERCENT_TOTAL 5	// DT11309 - bjl - R4 2012- Enable Cost+ Commissions from Allocations Window



// RecType is set to this flag when its a dummy 841
#define ALLOCATION_GENERATED_DEFAULT    0x08 // DT11309 - bjl - R4 2012- Enable Cost+ Commissions from Allocations Window

typedef struct tdMsg841Rec
{
	long SessionID;
	long MessageID;


	int	 ReqAction;		// what this message must do: list all, one, etc
	int  RecType;		// 1 = Aggregate record 2 = Allocation record

	int  RecNum;		// 1 = Aggregate record 2 = Allocation record


	char     Domain[DOMAIN_LEN];
	char     Account[ACCOUNT_LEN];

	EXSYMBOL ExSymbol;
	int		 SideAction;

	long	 Shares;
	char     MoreSpare[4];
	double	 Price;
	double	 Cost;
	char     AllocAccount[ACCOUNT_LEN];

	long     Key;
	char	Spare4[4];

	double	 Commission;

    char	 CommsType;        // 1111/1111  commission type

    char	 OrderNo[ORDERNO_LEN];		  // Order No if we allocate by tradae
	char	 AccType;
	char Spare2[2];

	ULONG	ExchangeCode;	// exhchanges to which this default applies

	char	 AllocCMTA[6];	// DT6598 - kma - 1/5/10 - CMTA in allocationm

	char	 Spare[30];		// DT6598 - kma - 1/5/10 - CMTA in allocationm

} MSG841REC;

typedef struct tdMsg842Rec
{
	long SessionID;
	long MessageID;


	int	 ReqAction;		//
	int  RecType;		//

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];


} MSG842REC;

typedef struct tdMsg843Rec
{
	long SessionID;
	long MessageID;


	int	 ReqAction;		//
	int  RecType;		//

	int  RecNum;		//

	DEFASSIGNMENT Assignment;

} MSG843REC;

// LL 20080108 DT2742 - contract information for option aggregates
typedef struct tdMsg844Rec
{
	long SessionID;
	long MessageID;
	char OptionSymbol[SYMBOL_LEN_INTERNAL];

	// derived from CONTRACT structure of option.h
	double StrikePrice;
	double Conversion;
	long ExpireDate;
	char Type;
	char Class[4]; // NOT OSI COMPLIANT, BUT FIELD IS NEVER USED IN ANY! APP IN VSS- kma - 12/30/2009
	char RootSymbol[SYMBOL_LEN_INTERNAL];
	char OptRootType;
} MSG844REC;

typedef struct tdMsg845Rec // Only used to send list back to FE/bo
{
	long SessionID;
	long MessageID;


	int	 RecNum;		//	1 for reset, otherwise record number, 0 to end

	char Domain[DOMAIN_LEN];	//comeback: blank - can be used on any domain
	char Account[ACCOUNT_LEN];  // comeback: blank - can be used on any account in this domain.

	long AllocEntitleBits;		//

	char AllocAccount[ACCOUNT_LEN];	// blank - can allocate to anything  on above selection
	char AllocAccountName[40];		// not used if blank above

	char Spare[40];		// not used


} MSG845REC;

// Export Account Allocations (Save and SUBMIT button on Bofa's allocation tool
typedef struct tdMsg846Rec
{
	long SessionID;
	long MessageID;

	char Domain[DOMAIN_LEN];	//comeback: blank - can be used on any domain
	char Account[ACCOUNT_LEN];  // comeback: blank - can be used on any account in this domain.

} MSG846REC;

// 850 : Valid login User-Account Info
//		 MSG654REC STRUCT

// 851 : Invalid login Request
typedef struct tdMsg851Rec
{
	long SessionID;
	long MessageID;
	char Reason[80];
} MSG851REC;

// 853 : Order detail line to Client (BigTradeRec) {Temporarily Reserved}
typedef struct tdMsg853Rec
{
	long L2SessionID;
	long L2MessageID;
	long MinderUpdateOnly;
	BIGTRADEREC BigTradeRec;
} MSG853REC;

// 857 : Trade List Record from ClientServer (BigTradeRec) {Temporarily Reserved}
typedef struct tdMsg857Rec
{
	long L2SessionID;
	long L2MessageID;
	char Account[ACCOUNT_LEN];
	char User[USER_LEN];
	char Domain[DOMAIN_LEN];
	BIGTRADEREC BigTradeRec;
} MSG857REC;


// 859 : Trade List Record From 811 QUERY
typedef struct tdMsg859Rec
{
	long SessionID;
	long MessageID;
	BIGTRADEREC BigTradeRec;
} MSG859REC;

// 858 : TradeDetails List Record from ClientServer (BigTradeRec) {Temporarily Reserved}
//typedef struct tdMsg858Rec
//{
//	long L2SessionID;
//	long L2MessageID;
//	char Account[ACCOUNT_LEN];
//	char User[USER_LEN];
//	char Domain[DOMAIN_LEN];
//	char NoOfBlocks;
//	ORDERDETPKT	OrderDetPkt[10]; //Need to Change Order Det Pkt to accept BigTradeRecs - KLEDER
//} MSG858REC;

// 860 : Request Load Data
typedef struct tdMsg860Rec
{
	char Domain[DOMAIN_LEN];
	int Type;  // 1=Load ShortList,0=Load All Penson Files,50=Load Price file, 100=Export Penson Files,99=Load Clearing
} MSG860REC;

// 1860 : Request Load Data - replaces 820 and 860 and adds userID and others
// DEV-6463 - SOD Mass Load - Positions and BP
typedef struct tdMsg1860Rec
{
	char Domain[64];
	char Account[64];
	char UserId[64];
	char Command[64];   // make it text iso convoluted type 

	int Type;		// as is expected by the code
	int Type2;

	char Spare[128];
	
} MSG1860REC;

// 861 : Responce to LoadRequests
typedef struct tdMsg861Rec
{
	char Message[256];
} MSG861REC;

// 862 : Clearing Message sent from Export server to Clserver
typedef struct tdMsg862Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	int  MessageType;		// 0=msg, 1=error, 2=export completely completed

	char Message[256];
	char Spare  [256];

} MSG862REC;




// 865 : Request report/file
typedef struct tdMsg865Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	char Account[DOMAIN_LEN];

	int ReportType;  // set to 20 for UTM report

	char FileName[40];
	char Spare [216];

} MSG865REC;

// 880	: Account Route Query - Sends back 881 messages
typedef struct tdMsg880Rec		// Query Domain
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];		// domain we are asking for
	char Account[ACCOUNT_LEN];		// Account we are asking for

	union {
		long Flags;
		struct
		{
			bool Spare	: 1;	//
			bool spare2 : 1;	//
			bool SPARE3 : 1;	//
		};
	};

} MSG880REC;

// Response to 880 message
typedef struct tdMsg881Rec
{
	CLIENTINFO ClientInfo;
// + DT11533 - bjl - R4 2012 - CL needs to use Sub-Routes when enabling routes for an Account
	SMALLACCROUTEREC AccRouteRec;
	//Spare Removed
// - DT11533
}MSG881REC;

// 883 : Ecn Statistics Counter - CLServer to BoClient, better than 583
typedef struct tdMsg883Rec
{
	char Domain[DOMAIN_LEN];
	char ECN[ECN_LEN];
	DOMAINCOUNT DomainCount;
} MSG883REC;

// 884 - Save / edit Account Route record - sends back 885 with OK /reason
typedef struct tdMSG884REC
{
	CLIENTINFO		ClientInfo;
	char			Action;		// MAKE IT 1 for now
	ACCROUTEREC		AccRouteRec;
	char			Spare[40];

} MSG884REC;

// 885 : Account-Route Responce
typedef struct tdMsg885Rec
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char Route[ECN_LEN];
	char Agreement[AGREEMENT_LEN];	// AgreementShortName in DB    Client / ROute / Destination Agreement
	char Reason[80];
	int  ReasonNo; // See Responce List at the bottom of this file (21 = end)
	char SubRouteName[20]; // DT11533 - bjl - R4 2012 - CL needs to use Sub-Routes when enabling routes for an Account
} MSG885REC;

// 886 : Domain-Route List Req - sends back 887 records
typedef struct tdMsg886Rec
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];
	long Flags;
	char Spare[200];

} MSG886REC;

// 887 : Domain-Route List Req - sends back 887 records
typedef struct tdMsg887Rec
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];
	char Route[ECN_LEN];
	char Agreement[AGREEMENT_LEN];	// AgreementShortName in DB    Client / ROute / Destination Agreement
// + DT11533 - bjl - R4 2012 - CL needs to use Sub-Routes when enabling routes for an Account
	char SubRouteName[20];
	// Spare Removed
// - DT11533
} MSG887REC;

// 890 : Query for all User-Account Info from a specific domain/account
typedef struct tdMsg890Rec
{
	long SessionID;
	long MessageID;
	char Domain[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
} MSG890REC;

// 891 : User-Account Info in response for msg 890
typedef struct tdMsg891Rec
{
	long SessionID;
	long MessageID;
	USERACCREC UserAccRec;
} MSG891REC;

//Msg900Rec - Trade Data to T&S
typedef struct tdMsg900Data
{
	long Time;
	long SeqNo;
	long Shares;
	float Price;
	WORD Type;
	WORD Condition;
	char MMID[MMID_LEN];
	char TradeRegion;
	long OrigSeqNo;
} MSG900DATA;

//Msg901Rec - Quote Data to T&S
typedef struct tdMsg901Data
{
	long Time;
	long SeqNo;
	float Bid;
	long BidSize;
	float Ask;
	long AskSize;
	WORD Type;
	WORD Condition;
	char MMID[MMID_LEN];
	char QuoteRegion;
} MSG901DATA;

typedef struct tdMsg900Rec // Trade Msg
{
	EXSYMBOL ExSymbol;
	MSG900DATA Data;
} MSG900REC;

typedef struct tdMsg901Rec // Qoute Msg
{
	EXSYMBOL ExSymbol;
	MSG901DATA Data;
} MSG901REC;

typedef struct tdMsg902Rec // Cancel/Correct Msg
{
	EXSYMBOL ExSymbol;
	MSG900DATA Data;
	long OrigSeqNo;
} MSG902REC;

typedef struct tdMsg908Rec
{
	EXSYMBOL ExSymbol; 	//0	0
	char TradingEx[7];	//12	10
	char ListingEx[7];	//16	16
	char Tick;		//10	12
	char Spare1;
	float Last;		//1	1
	float Open;		//2	5
	float High;		//3	7
	float Low;		//4	8
	float PrevClose;	//5	6
	float Bid;		//6	3
	float Ask;		//7	4
	float Change;		//9	2
	float LastRegular;	//14	14
	float Close;		//15	15
	long LastSize;		//11	11
	long Volume;		//8	9
	WORD Format;		//13	13
	// Begin DT8301 Bud 12/01/10
	union
	{
		char Spare2[2];
		struct
		{
			bool	isPink		: 1;
		};
	};
	// end DT8301 Bud 12/01/10
} MSG908REC;

// Begin DT8301 Bud 12/01/10
typedef struct tdMsg908RecExt
{
	EXSYMBOL ExSymbol; 	//0	0
	char TradingEx[7];	//12	10
	char ListingEx[7];	//16	16
	char Tick;		//10	12
	char Spare1;
	float Last;		//1	1
	float Open;		//2	5
	float High;		//3	7
	float Low;		//4	8
	float PrevClose;	//5	6
	float Bid;		//6	3
	float Ask;		//7	4
	float Change;		//9	2
	float LastRegular;	//14	14
	float Close;		//15	15
	long LastSize;		//11	11
	long Volume;		//8	9
	WORD Format;		//13	13
	union
	{
		char Spare2[2];
		struct
		{
			bool	isPink		: 1;
		};
	};
	float PinkBid;
	float PinkAsk;
} MSG908RECEXT;
// End DT8301 Bud 12/01/10

// NEW 909 MESSAGE - Same as 908, but its not a real quote - just meant to update info. (ie Currency info)
typedef struct tdMsg909Rec
{
	EXSYMBOL ExSymbol; 	//0	0
	char TradingEx[7];	//12	10
	char ListingEx[7];	//16	16
	char Tick;		//10	12
	char Spare1;
	float Last;		//1	1
	float Open;		//2	5
	float High;		//3	7
	float Low;		//4	8
	float PrevClose;	//5	6
	float Bid;		//6	3
	float Ask;		//7	4
	float Change;		//9	2
	float LastRegular;	//14	14
	float Close;		//15	15
	long LastSize;		//11	11
	long Volume;		//8	9
	WORD Format;		//13	13
	char Spare2[20];
} MSG909REC;

typedef struct tdMsg910Rec // Re-request message - sent from Quote server, caused by Admin person
{
	union
	{
		long UserFlags;
		struct
		{
			bool	RequestErrors		: 1; //1 True if CLS needs to request all ERROR'd symbols (caused by system not having feed earlier)
			bool	SingleSymbolOnly	: 1; //1 If true, ExSymbol must contain the single symbol to be requested
			bool	ExpirationsChanged	: 1; // only for EquityUpdate
			bool	StrikesChanged		: 1; // only for EquityUpdate
			bool	RefreshOptions		: 1; // only for EquityUpdate
			bool	FutureUpdate		: 1;
			bool	EquityUpdate		: 1;
			bool	OptionUpdate		: 1;
			bool	StrikeAdded			: 1; // only for OptionUpdate
			bool	StrikeModified		: 1; // only for OptionUpdate
			bool	StrikeDeleted		: 1; // only for OptionUpdate
	// + DT7668 - bjl - New Restricted List
			bool  RestrictedChanged : 1; // only for Restricted List Changes
			bool  PinkBuyerAware	: 1; // only for PinkSheets
	// - DT7668
		};
	};
	EXSYMBOL UnderlyingSymbol;
	long Expiration; // WSDate when NewStrikes=1, only for EquityUpdate
	char Spare[84];
	void *hSession;
	void *hWnd;

} MSG910REC; // sizeof=124


typedef struct tdMsg915Rec // notification of new 1min bar to hisintra
{
	EXSYMBOL ExSymbol;
	long Date;
	long Time;
	float Open;
	float High;
	float Low;
	float Last;
	long Volume;
}MSG915REC;

// 929 : Request Vars Id list by domain
typedef struct tdMsg929Rec
{
	long SessionID;
	long MessageID;

	char Domain[DOMAIN_LEN];

	char FileName[80];

	char Spare[64];
} MSG929REC;

// 930 : CLServer Dns Notification
typedef struct tdMsg930RecA
{
	char Domain[DOMAIN_LEN];
	char ParentDomain[DOMAIN_LEN];
	int  SPort;
	char ExportDomainName[DOMAIN_LEN];
	char DomainEnvironment; // D=DEV, P=Production, S=Staging, E=error I dont know, // DEV-2632 - Staging + Prod environment
	char Spare[63];// DEV-2632 - Staging + Prod environment
} MSG930RECA, MSG930REC;

// 931: Query in DafDB Web app - code is disabled.
//      Looks like it accepted a 930 to query and sent back the full details

typedef struct tdMsg932Rec // Vars ID wrapper for WEB <-> CLS
{
	int Action;			// 0 = wrong! 1=insert 2=update 3=delete
	CLIENTINFO ClientInfo;
	VARSIDREC VarsId;

} MSG932REC;


#define VARSID_ACTION_FORCE 1
#define VARSID_ACTION_CHECK 2
#define VARSID_ACTION_ACTIVATE 3

typedef struct tdMsg933Rec // Vars ID FORCE from BO	 - see also 934 and 938
{
	long SessionID;
	long MessageID;

	char Action;			// VARSID_ACTION_FORCE or VARSID_ACTION_CHECK
	char Status;			// < 0 = error   0 = nothing!   3=internal use   4=succeessfull 5=NO MATCH 10=Empty fields

	char Domain[DOMAIN_LEN];
	char User[USER_LEN];

	char BoUser[USER_LEN];
	char BoPassword[PASSWORD_LEN];

	char SuggestedVarsId[VARSID_LEN];
	char CurrentVarsId[VARSID_LEN];
	char ForceVarsId[VARSID_LEN];

	char Reason[200];


} MSG933REC;

#define VARSID_STATUS_NODATA 3
#define VARSID_STATUS_MATCH 4
#define VARSID_STATUS_MISMATCH 5

typedef struct tdMsg934Rec // DNS CHECK VARSID
{
	CLIENTINFO ClientInfo;
	CLIENTINFO ServerInfo;

	char		Status;			// VARSID_STATUS_MATCH or VARSID_STATUS_MISMATCH

	char		VarsId[VARSID_LEN];	// check this varsid
	USERREC		UserRec;			// for this user data
	VARSIDREC	VarsIdRec;			// answer back with this filled in

} MSG934REC;

// 935 : CLServer User Activation Notification
typedef struct tdMsg935Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];
	char Action;
	char Spare[80];

} MSG935REC;

// 936 : Ack from FE to DNS that user has signed a document from the frontend
typedef struct tdMsg936Rec
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];
	char User[USER_LEN];

	char Initials[USER_LEN];
	char Password[PASSWORD_LEN];

	char DocId[DOCID_LEN];			// system wide unique DOC Id (string)
	long DocNum;					// document number - UNIQUE DOC NUMBER - system wide - (maps to the DocId)

	char Signed;				// signed / Needs to Sign
	char SignMethod;			// Whatever method he used to ACK the doc
	long SignDate;				// Date done, or 0 if not yet
	long SignTime;				// Time done, or 0 if not yet
	long SignVersion;			// Version of DOC signed
	long BuildVersion;			// Version of FE used to sign this

	char DocDomain[DOMAIN_LEN];		// domain where DocId exists
	char spare[4];

} MSG936REC;

// 937 : Msg from DNS to FE that DOC needs to be signed
typedef struct tdMsg937Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];

	char DocName[DOCNAME_LEN];			// File Name to request
	char DocId[DOCID_LEN];				// system wide unique DOC Id (string)
	long DocNum;						// document number - UNIQUE DOC NUMBER - system wide - (maps to the DocId)

	int SignMethod;						// See DbCommon DOMAINDOCREC
	int CancelAction;					// See DbCommon DOMAINDOCREC

	char spare[200];

} MSG937REC;

// 938 - contains a 934 record
#define MSG938REC MSG934REC

// 951 : BACKUP Server <> CLSERVER Domain Notification  - For domains as well as Dayend msg
typedef struct tdMsg951Rec
{
	short Cnt;	// number or domains being sent
	short Item;	// Item number in seq

	char Action;// 0=list 1=  2=Disable this domain on Backup; 3=Enable Domain on backup
						//	 2 and 3 control the DO_NOT_LOAD.TXT file on the domain on the BACKUP program
	char Domain[DOMAIN_LEN];
	char Spare[128];

} MSG951REC;



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

// garaunteed delivery wrapper message - preceeds thbe actual message being sent
typedef struct tdMsg955Rec
{
	long ReportLogNo;
	int MsgID;
	int MsgLen;
	void * Data;// must be ignored

} MSG955REC;

// MSG1957 CLS->DNS message to expire passwd or set passwd expiration date
// MSG1958 DNS->CLS message to expire passwd or set passwd expiration date
// MSG1958 BO->CLS message to expire passwd or set passwd expiration date
typedef struct tdMsg1957Rec 
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];

	union
	{
		long PasswdFlags;
		struct
		{
			bool	PasswdLockOut		: 1; 
			bool	SetPwdExpireDate	: 1; 
			bool	SetTempPwd			: 1;
			bool	Spare4				: 1;
			bool	Spare5				: 1;
			bool	Spare6				: 1;
		};
	};

	long ExpireDate;

	char TempPwd[PASSWORD_LEN];

	char Spare[64];
} MSG1957REC, MSG1958REC, MSG1959REC;

#define BA_COM_OPEN 1
#define BA_COM_WRITE 2
#define BA_COM_READ 3
#define BA_COM_SYNC 4
#define BA_COM_CLOSE 5
#define BA_COM_RESET 6
#define BA_COM_RESYNC_DONE 7		/// DOES NOT GET SENT, internal use only
#define BA_COM_DAYEND 99


#define BA_FT_USER 1
#define BA_FT_ACC 2
#define BA_FT_USR_ACC 3
#define BA_FT_TRADE 4
#define BA_FT_MSG 5
#define BA_FT_HEDGE_TRADE 6
#define BA_FT_ALLOC 7
#define BA_FT_DEF_ALLOC 8
#define BA_FT_REPORTLOG 9
#define BA_FT_INDEXDB  10
#define BA_FT_INLOG 11
#define BA_FT_OUTLOG 12
#define BA_FT_NEWINLOG 13
#define BA_FT_FULLLOG 14
#define BA_FT_PLACELOG 15
#define BA_FT_CANCELLOG 16
#define BA_FT_TRADELOG 17
#define BA_FT_USERLOG 18
#define BA_FT_GDLOG 19
#define BA_FT_OMSORDERLOG 20
#define BA_FT_USR_DOC 21
#define BA_FT_ACC_ROUTE 22
#define BA_FT_SHORTLOCATE_LOG 23
#define BA_FT_PROXY_IN_LOG 24
#define BA_FT_PROXY_OUT_LOG 25
#define BA_FT_PROXY_FULL_LOG 26
#define BA_FT_PROXY_RAW_LOG 27
#define BA_FT_SPECINLOG 28
#define BA_FT_SPECOUTLOG 29
#define BA_FT_SPECFULLLOG 30
#define BA_FT_STOCKLOAN 31
#define BA_FT_HEADER 32 // 8 byte header at the beginning of users and account db. Can be any lenght and used for anything else
#define BA_FT_IQOS 33	// DT10235 YL 20111101

// 960 new DNS user entitle
typedef struct tdMsg960Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];

	char Entitlements[16];
	long Books;
	union
	{
		long UserFlags;
		struct
		{
			unsigned short Pro		: 1; //Pro/Non-Pro
			unsigned short VarsIdSet: 1; //True if VARSID field has been accepted and needs to be tested
			unsigned short EnableNonProVARSID: 1; // True is NON-PRO vars-ID is in effect on this user's domain
			unsigned short PasswdLockOut		: 1; //4 True if User Is Locked out and needs passwd reset

			unsigned short EnhancedSecurity		: 1; //5	True if passwd mgmt is enabled on Domain
			unsigned short UserDocAckNeeded		: 1; //6	true if there are Document Acknowlegements needed
			unsigned short UserAgreementNeeded  : 1; //7	True if user needs to sign an agreement (NYSE, AMEX, etc)
			unsigned short UserDeleted			: 1; //8

			unsigned short FeedSuspended		: 1; //9	True if user should not get any feed
		};
	};

	char BORights[3];
	char cSpare;

	long BoRightsEx;

} MSG960REC;

// 1960 new DNS user entitle
typedef struct tdMsg1960Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];

	char Entitlements[16];
	long Books;
	union
	{
		long UserFlags;
		struct
		{
			unsigned short Pro		: 1; //Pro/Non-Pro
			unsigned short VarsIdSet: 1; //True if VARSID field has been accepted and needs to be tested
			unsigned short EnableNonProVARSID: 1; // True is NON-PRO vars-ID is in effect on this user's domain
			unsigned short PasswdLockOut		: 1; //4 True if User Is Locked out and needs passwd reset

			unsigned short EnhancedSecurity		: 1; //5	True if passwd mgmt is enabled on Domain
			unsigned short UserDocAckNeeded		: 1; //6	true if there are Document Acknowlegements needed
			unsigned short UserAgreementNeeded  : 1; //7	True if user needs to sign an agreement (NYSE, AMEX, etc)
			unsigned short UserDeleted			: 1; //8

			unsigned short FeedSuspended		: 1; //9	True if user should not get any feed
		};
	};

	char BORights[3];
	unsigned char PasswordLifetime;

	long BORightsEx;

	char VarsId[VARSID_LEN];
	char Citizenship1[2];
	char Citizenship2[2];
	long PwdExpireDate;// date the password expires
	char EncryptionType;	// 0-Plain text, 1-MD5
	unsigned char PasswordHashLen;		// dt10091 - SAVE PWD hash
	char Spare[22];
} MSG1960REC;

// 961 DNS entitle login ok
// NoData

// 962 DNS global login request- send it and it is returned
typedef struct tdMsg962Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Password[PASSWORD_LEN];
	long ProductType;	// 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver,0X80=tWIST
	long BuildNo;

	char LoginOk;
	char RejectReason[80];
	long LineID;
	char Oms[DOMAIN_LEN];


	short DocSignVersion;
#define DOC_SCHEMA_1_0 (USHORT)0x0100

	char passwdStatus;		// 0 - OK, 1 - Expiring, 2 - Expired
	char EncryptionType;	// 0-Plain text, 1-MD5
	char AuditCapable;		// 0-do not return 1963, 1-ok to receive 1963
	char Spare[55];

}  MSG962REC;

// 963 Service IP-Adresses
typedef struct tdMsg963Rec
{
	char ServiceName[64];
	char Domain[DOMAIN_LEN];
	char IP[20];
	int Port;
} MSG963REC;

// 964 User-Acc Data
// USERACCREC

// 965 User Query from CLSERVER/OMS warp with 990 to get an interpreted list of Spesific Domain Acc's
typedef struct tdMsg965Rec
{
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];

	long RequestId;

	///////////////////////////////////

	char UserPortType;
	short UserPortNo;
	short LoginMsgId;

	char UserType;
	char Spares[49];		// always used at some stage

	union
	{
		char SpareBuf[200];			// added so we can add larger login structs
		MSG501REC Msg501Rec;		// size 72
		MSG801REC Msg801Rec;		// size 56
		MSG525REC Msg525Rec;		// size 76
	} Msg;

} MSG965REC;


// 966 REPLAY to User Query from CLSERVER
typedef struct tdMsg966Rec
{
	MSG965REC Msg965Rec;
	MSG960REC Msg960Rec;
	char LoginOk;
// DT10196 - bjl - Audit: Hashed Password Support (KMA)
	char EncryptionType;	// dt10091 -  // PASSWORD Encryption type 0-Plain text, 1-MD5 2-IQ-SGA-512 
	unsigned char PasswordHashLen;		// dt10091 - SAVE PWD hash
	char aSopare1;
} MSG966REC;

// 5966 Audit REPLAY to User Query from OMS
typedef struct tdMsg5966Rec
{
	MSG965REC Msg965Rec;
	MSG960REC Msg960Rec;
	char LoginOk;
	int EncryptionType;
	int PassLen;
} MSG5966REC;

// 967 REPLAY to User Query from CLSERVER USERACC PART
typedef struct tdMsg967Rec
{
	MSG965REC Msg965Rec;
	USERACCREC Msg964Rec;
	char EndOfList;
} MSG967REC;

// 968 RePlicate Domain Relation //REPLACED WITH 969
typedef struct tdMsg968Rec
{
	char Domain[DOMAIN_LEN];
	char Parent[DOMAIN_LEN];
} MSG968REC;

// 969 RePlicate Domain Relation with Logic Parent //REPLACED WITH 972
typedef struct tdMsg969Rec
{
	char Domain[DOMAIN_LEN];
	char Parent[DOMAIN_LEN];
	char LogicParent[DOMAIN_LEN];
} MSG969REC;

// 970 UserDocRec sent from CLS to DNS - all the documents attahced to a user (signed or not)
typedef struct tdMsg970Rec 		// Query User Docs
{
	CLIENTINFO ClientInfo;
	USERDOCREC UserDocRec;

} MSG970REC;

// 971 DOMAINDOCREC sent from CLS to DNS = lists all the docs attached to a domain

typedef struct tdMsg971Rec		//
{
	CLIENTINFO ClientInfo;
	DOMAINDOCREC DomainDocRec;

} MSG971REC;

// 972 Replicate Domain Relation with Logic Parent and OmsServerName;
typedef struct tdMsg972Rec
{
	char Domain[DOMAIN_LEN];
	char Parent[DOMAIN_LEN];
	char LogicParent[DOMAIN_LEN];
	char OmsServerName[DOMAIN_LEN];
} MSG972REC;

// 980	: User Document Query - Causes 970 messages to be sent back
typedef struct tdMsg980Rec		// Query Domain
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];		// domain we are asking for
	char User[USER_LEN];			// User we are asking for

	union {
		long Flags;
		struct
		{
			bool Spare	: 1;	//
			bool spare2 : 1;	//
			bool SPARE3 : 1;	//
		};
	};


} MSG980REC;


// 981	: Domain Query - Causes 971 messages to be sent back
typedef struct tdMsg981Rec		// Query Domain
{
	CLIENTINFO ClientInfo;

	char Domain[DOMAIN_LEN];		// domain we are asking for

	union {
		long Flags;
		struct
		{
			bool WantConfig		: 1;	// true - This will attach the Domain INI to the return message
			bool WantDocs		: 1;	// true - You want the Domain Doc List returned
			bool WantChildren	: 1;	// True - Return all childen as well
		};
	};


} MSG981REC;

// 990 User Query from CLSERVER/OMS with an interpreted list of Spesific Domain Acc USE 965 for Plain (Short) List
typedef struct tdMsg990Rec
{
	MSG965REC Msg965Rec;
} MSG990REC;

// 997 : OmsServer IP Notification
typedef struct tdMsg997Rec
{
	char OmsName[DOMAIN_LEN];
	char IP[10][40];		// 10 addresses possible - supports IPv6 maxlen=40
	char NetworkId[10];		// For each IP, the network Code is listed here
	char Current;			// Index in the IP table of the current setting  (0-9)
	long PortNo[10];		// Port No's
} MSG997REC;


// 998 : TradeSvr IP Notification
typedef struct tdMsg998Rec
{
	char Domain[DOMAIN_LEN];
	char IP[10][40];		// 10 addresses possible - supports IPv6 maxlen=40
	char NetworkId[10];		// For each IP, the network Code is listed here
	char Current;			// Index in the IP table of the current setting  (0-9)

} MSG998REC;

// 999 : TradeSvr IP Notification
typedef struct tdMsg999Rec
{
	char Domain[DOMAIN_LEN];
	char IP[20];	
	
	// +DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	// made this a union for clarity, its FE's port but didnt want to break other apps
	union 
	{
		int  fePort;
		int  Port;
	};
	// -DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	char OmsServerName[DOMAIN_LEN];
	char ParentDomain[16];
	char ServerName[DOMAIN_LEN];
	char Done;
	char EntDomain[DOMAIN_LEN];
	char SpareBytes[3];
	union {
		long DomainFlag;
		struct
		{
			bool EnhancedSecurity : 1;	// is used, sent in 1960 msg
		};
	};
	unsigned char PasswordLifetime;// is used, sent in 1960 msg
	char spare[3];	// DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	int  boPort;	// DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	char Res2[16];	// DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
} MSG999REC;

// Request Responce List
//		0 : New Item was Created
//		1 : New Item Failed : Duplicate Allready Exist
//		2 : New Item Failed : Error during creation
//		10 : Item was Updated
//		11 : Item Update Failed : Item Does not Exist
//		12 : Item Update Failed : Error During Update
//		20 : Item could not be found : Item was not found
//		21 : Item lookup: End of Item List
//		22 : Item lookup: End of Item List,List Exceeded MaxItems
//      23 : Item lookup: Start of List for Level 2
//		30 : Item was Deleted
//		31 : Item Delete Failed : Item Does not Exist
//		32 : Item Delete Failed : Error During Delete
//		99 : Unknown Domain

typedef struct tdMsg20Rec
{
	char szHost[20];
	char szPort[8];
	WORD wCpu;
	WORD wMemory;
	WORD wTypeServer;
	WORD wNrusers;
	WORD wOS;
	WORD wIqversion;
	WORD wNropenquotes;
	char szDomain[9];
	char ui[9];

} MSG20REC;

#define IQPROD_CLSERVER		2
#define IQPROD_IQSERVER		3
#define IQPROD_IQCLIENT		4
#define IQPROD_TRADER		5
#define IQPROD_DNS			6
#define IQPROD_TIMESALES	7
#define IQPROD_WEBAUTH		8
#define IQPROD_BACKUP		9
#define IQPROD_OMS			10
#define IQPROD_NQPROX		11
#define IQPROD_CHARTS		12
#define IQPROD_HISINTRA		13
#define IQPROD_HISDAILY		14
#define IQPROD_SYMBLIST		15
#define IQPROD_FILESERVER	16
#define IQPROD_NEWS			17
#define IQPROD_FEEDPROX		18
#define IQPROD_IQFIXSERVER	19
#define IQPROD_FIXDROPCOPY	20
#define IQPROD_REPORTSERVER	21


typedef struct tdMsg16HBRec
{
	WORD wThisByteShouldBeZero;
	WORD wTypeServer;			// See IQPROD defines

	char ServerName[19];
	char GroupId;
	char ClsName[8];

	DWORD wMemoryTotal;
	DWORD wMemoryUsed;

	char CpuPerc;

	union {
		long Flags;
		struct
		{
			bool IsDemo : 1;	// true - this is a demo machine
			bool HasDb	: 1;	// true - This has access db turned on/active for Iplink
			bool IsTestMachine : 1;	// Should only be true on Development machines
			bool TradingSuspended : 1;	// true means no-one can trade - caused by some errors
			bool TradingDay : 1;		// true means its not a weekend or holiday
		};
	};

	WORD nUsers;
	WORD Ti;
	WORD BuildNo;
	char BuildName[10];

	WORD WSErrorCount;

	long BuiltDateTime;		//	(time_t)
	long StartDateTime;		//	(time_t)
	long CurrentDateTime;	//	(time_t)

	long LastTradeTime;		//	(time_t)

	long Lc1;		//	(generic counter 1)
	long Lc2;		//	(generic counter 2)

	long TotalTrades;		//	Total number of trades in system (overnight as well)
	long TotalVolume;		// Total Volumne

} MSG16HBREC;

// 1000 = Error message to the EYE
// 1001 = Old EVENT message - should no be sent anymore  unless WS_THE_EYE_EVENTS is defined
// 1002 = CLS Startuup messgae - sent to EYE - contains all startup errors
// 1004 = A text message which is logged as an error message, which makes it show up on the eye
// 1005 = EYE to CL - To delete a minute message
// 1010 - Serfver VAR editor

// Prototype Monitoring message, user must allocate the Data Len plus sizeof(MSG1020REC) and cast to MSG1020REC
// ie. MSG1020REC* = msg1020Rec = (MSG1020REC*) malloc(sizeof(MSG1020REC) + strlen(myData));
typedef struct tdMsg1020Rec
{
	typedef enum
	{
		none,
		Response,
		Get,
		Set,
		IOI,		// Indication of Interest (by a Viewer)
		NLI,		// No Longer Interested (by a Viewer)
		RegObject	// Used to register a monitored object (application or service)
	} MsgType;

	MsgType Type;

	char Data[2048];
} MSG1020REC;

// 1100 to 1199 Reserved for system monitor messagwes
// 527 - sends domain stats to monitor

// IQSERVER to DNS mesages
// -------------------
typedef struct tdMsg1102Rec //IQSERVER to DNS load balance data
{
    long Time;
    int TotUsers;
    int TotQuotes;
    int PortNo;             // Used for DNS replicator
	int TotMsgsPerSec;
	char szHost[20];
	char szPort[8];
	char szDomain[9];
	bool Unused[3];
	int smpError;			// smproxy detected error
	int smpErrTime;
    char Reserved[62];
} MSG1102REC;


// MSG1200REC to MSG1299REC reserved for new TML complex order system

// + DT9045 YL 20110608
// Report config message
typedef struct tdMsg1300Rec
{
	unsigned long	ReportID;
} MSG1300REC;
// - DT9045 YL 20110608

// 1400 to 1499 Reserved for CLSERVER
//
//
typedef struct tdMsg1400Rec
{
	CLIENTINFO  ClientInfo;

	unsigned long ReportLogNo;   // record number - starting from 1 on the "trader" side
						// when the connection is opened, a 707 message will be sent from CLS with the last number
						// the clserver received. The SSA app must then play back all records to get cls current.
	STOCKLOAN Trd;

} MSG1400REC;

#define MSG1401REC MSG1400REC

///////////////////////////////////////

// Domain Export data request - sent from exporting cls to domain containing data
typedef struct tdMsg1410Rec
{
	CLIENTINFO		ClientInfo;	// filled in by originator. and passed back in the 1412 message

	char Domain[DOMAIN_LEN];

	char Spare[128];

} MSG1410REC, *pMSG1410REC;

typedef struct tdMsg1412Rec
{
	CLIENTINFO		ClientInfo;
	BIGTRADEREC		BigTradeRec;

}  MSG1412REC;

/*
typedef struct tdMsg1411Rec
{
	CLIENTINFO		ClientInfo;
	MINIACCOUNTREC  TradeListItem;

} MSG14110REC;
*/

// Domain Export Server, Notification of domains and levels supported
typedef struct tdMsg1415Rec
{
	char ExportDomainName[DOMAIN_LEN];
	long DomainExportLevel;

	char Spare[128];

} MSG1415REC, *pMSG1415REC;

// Domain Export data request - sent from domain that needs to send export data to export server
typedef struct tdMsg1416Rec
{
	char ExportDomainName[DOMAIN_LEN];

	char Spare[128];

}  MSG1416REC, *pMSG1416REC;


//////////////////////////////////////

// KMA - Message range 2000-2999 reserved for new OMS



//521 = StockLoan STRUCTURE - StockLoan STATUS		outgoing
//
typedef struct tdMsg1421Rec
{
	long SessionID;		// set on call and returned as is
	long MessageID;		// set on call and returned as is
	long Type;			// 0 for later
	long Counter;		// record number - if this is a request, will start at 1, 0 for individual update
	long Spare;

	STOCKLOAN Trd;

} MSG1421REC;

//1423: StockLoan Entry

#define RATE_TYPE_NUMBER		0
#define RATE_TYPE_TEXT			1
/*
typedef struct tdMsg1423Rec
{
	char Domain	[DOMAIN_LEN];
	char Account[ACCOUNT_LEN];
	char Symbol [SYMBOL_LEN];	

	long  Type;	// = 0 for normal 1=faked
	long  Ordered;
	long  Approved;
	long  NoGood;
	long  Used;
	long  Available;
	long  Needed;

	unsigned char RateType;		// designates the data type of "Rate"
					// if RATE_TYPE_NUMBER, Rate must be cast into double
	char	Rate[11]; // if type is number then it is packed in as double 

	char	Spare[12]; // kma was here

	long  Number;
	long  ThisNumber;

} MSG1423REC;

// 1425 : Request StockLoan Detail/Order records
// To use: Zero out, and fill in account, domain, and symbol
// if no symbol, all will be sent as
//
typedef struct tdMsg1425Rec
{
	long SessionID;		// set on call and returned as is
	long MessageID;		// set on call and returned as is
	long Type;			// 0 for details  1=trades
	long Counter;		// record number - if this is a request, will start at 1, 0 for individual update
	long Spare;

	STOCKLOAN Trd;

} MSG1425REC;*/

// 1427 : Request/Response for StockLoan Route (Proxy) status
typedef struct tdMsg1427Rec
{
	char Domain [DOMAIN_LEN];			// For Set Status an empty string means all
	char Route [ECN_LEN];				// For Set Status an empty string means all
	long Type;							// 0 - Query Status, 1 - Set Status, 2 - Status Response

	union {
		long Status;
		struct
		{
			bool ManualApproval : 1;	// true - Manual Approval status (true is on)
		};
	};

	char Spare[28];

} MSG1427REC;

//DT#2526 NS++ 12/07/07
// 1428 : Send Domain/Account/Manager code to ssa_proxy
typedef struct tdMsg1428Rec
{
	char Domain [DOMAIN_LEN];			
	char Account[ACCOUNT_LEN];
	char ManagerCode[MANAGER_CODE_LEN];

	char Spare[24];

} MSG1428REC;
//DT#2526 NS-- 12/07/07

/////////////////////////////////////////////////////

// DT5453 Bud 05/12/09
// 1478 : Biggest TradeRec

#define Msg1478 ExtTrade 		// DT6090 bud 10/21/09
namespace ExtTrade				// DT6090 bud 10/21/09
{
	enum Product
	{
		UnknownProduct	= 0,
        LevelI			= 1,
        LevelII			= 2,
        IQExpress		= 3,
        IPLink			= 4,
        BackOffice		= 5,
        IQView			= 6,
        Fix				= 7,
		DNS				= 8,
		CLServer		= 9,
		Trader			= 10,
		OMS				= 11,
		Backup			= 12,
		ReportServer	= 13
	};

	enum Type
	{
		Unknown = 0,
		TradeRecOnly = 1,
		TagsOnly = 2,
		TradeRecAndTags = 3
	};

	const unsigned long HdrLen = sizeof(Msg1478::Product) * 2 + sizeof(Msg1478::Type) +
										sizeof(unsigned short) * 2;

	const unsigned long BtrLen = sizeof(BIGTRADEREC);

	const unsigned long HdrBtrLen = HdrLen + BtrLen;
};

typedef struct tdMsg1478Rec
{
	Msg1478::Product origId;
	Msg1478::Product destId;

	Msg1478::Type msgType;

	unsigned short tagOffset;
	unsigned short tagLen;

        // as of 2009may21: we must ensure that the first double in the btr btr.TradeRec.AvgPrice is on the correct 
        // boundary so that the compiler won't insert padding. If any changes are made to the fields above btr 
        // (i.e. Msg1478::HdrLen) please ensure the boundary condition is met. Right now the first double is on an
        // 8 byte boundary (272) from btr, so we need to keep the HdrLen divisible by 8

	union {					// Just labels, either a BIGTRADEREC will be at this offset or tags
		BIGTRADEREC btr;
		char tags[1];
	};

private:
	tdMsg1478Rec()	{}   // MSG1478REC Must always be malloc'd so the tag memory is contiguous with the message

} MSG1478REC;

// DT6090 bud 10/21/09
#define ExtTradeRec tdMsg1478Rec
#define EXTTRADEREC MSG1478REC

typedef ExtTradeRec tdMsg1483Rec;
typedef EXTTRADEREC MSG1483REC;
// end DT6090 bud 10/21/09


// Example:
//
//  msgType is Msg1478::TradeRecAndTags, origId and destId are any valid Msg1478::Product type:
//
//	Useful variables for readability:
//			char* myTagStr = "HelloWorld=true|";
//			BIGTRADEREC myBigTradeRec;
//
//			unsigned long tagLen = strlen(myTagStr) + 1;
//			unsigned long msgLen = Msg1478::HdrBtrLen + tagLen;
//
//	Message initialization:
//			MSG1478REC* Msg1478Rec = (MSG1478REC*) malloc(msgLen);
//	
//			Msg1478Rec->tagOffset = Msg1478::BtrLen;
//			Msg1478Rec->tagLen = tagLen;
//
//			Msg1478Rec->msgType = Msg1478::TradeRecAndTags;
//			Msg1478Rec->origId = Msg1478::LevelII;
//			Msg1478Rec->destId = Msg1478::CLServer;
//
// To access BIGTRADEREC directly-
//			strncpy(Msg1478Rec->btr.TradeRec.Account, "Account1", ACCOUNT_LEN);
//
// To memcpy an existing BIGTRADEREC-
//			memcpy(&(Msg1478Rec->btr), &myBigTradeRec, sizeof(BIGTRADEREC));
//
// To strcpy in the tags-
//			char* msgTags = &(Msg1478Rec->tags[Msg1478Rec->tagOffset]);
//			strncpy(msgTags, myTagStr, Msg1478Rec->tagLen);

// end DT5453 Bud 05/12/09 nobody expects the spanish inquisition

///////////////
// 1553 : OMS Order Message (Static block update)
typedef struct tdMsg1553Rec
{
	long L2SessionID;
	long L2MessageID;
	long MinderUpdateOnly;
	ORDERREC OrderRec;
} MSG1553REC;

// 1557 : Order Rec Client
typedef struct tdMsg1557Rec
{
	long SessionID;
	long MessageID;
	long Action;

	WORKORDERREFNO	WorkOrderRefNo;
	ORDERREC	 	OrderRec;

} MSG1557REC;


// actions on 1584 message
#define OOL_ORDER_INSERT	1
#define OOL_ORDER_UPDATE	2
#define OOL_ORDER_DELETE	3

#define OOL_DETAIL_INSERT   7
#define OOL_DETAIL_UPDATE   8
#define OOL_DETAIL_DELETE	9
#define OOL_UPDATE_DONE		99


//1584 : Open Orders mesage from CLServer to BOClient	- does insert update and delete
typedef struct tdMsg1584Rec
{
	int Action;			// 0 = wrong! 1=insert 2=update 3=delete
	long TradeDetRef;
	BIGTRADEREC BigTradeRec;
} MSG1584REC;

// new message for FIX server requests from TradeMon
typedef struct tdMsg1585Rec
{
	BIGTRADEREC BigTradeRec;
	int request_no;
	char filler[64];
} MSG1585REC;

// Extended 830 for FIX
typedef struct tdMsg1586Rec
{
	MSG830REC	m830;
	char User[USER_LEN];
	char filler[256];
} MSG1586REC;

// MSG1622REC : // SERVER Request Account Record
// SAME AS MSG622REC
//

// MSG4210REC  reserved for BFG		INI file
// MSG4211REC  reserved for BFG		Rules

// Message for inter-TradeMon comunications
typedef struct tdMsg4201Rec
{
	short version;
	short spare1;

	typedef enum
	{
		null,
		handled,
		ignored,
		reported,
		noted
	} DISPOSITION;

	// we will only use one of these
	union
	{
		GUID guid;
		char text[256];
	};

	DISPOSITION disposition;
	char spare[100];
} MSG4201REC;

// reserve 4300 - 4399 for FIX Link		WRK 2006.11.13
typedef struct tdMsg4301Rec
{
	int MessageID;		// set on call and returned as is
	char szType[4];		// @@T Trade ML message, @@M FIX message, @@C FIXLink command MSG, @@R response to a command

} MSG43015REC;

// Service IP-Adresses
typedef struct tdMsg1963Rec
{
	char ServiceName[64];
	char Domain[DOMAIN_LEN];
	char IP[20];
	int Port;
	char Transport;			// 0-plain, 1-zlib, 2-DES3/zlib, 3-ssl/plain, 4-ssl/zlib
	char Protocol;			// 0-legacy, 1-msg 5000
	char reserved[2];
	char GatewayIP[20];     // IQATS location
	int GatewayPort;
	char ClientIP[20];      // Copied from msg 5000
	int ClientPort;         // Copied from msg 5000
	char AppInstance[20];
	int ClientMask;
	int LineID;
	char Spare[72];
} MSG1963REC; // 256 bytes

// 5000 Global login request
typedef struct tdMsg5000Rec
{
	char ServiceName[64];   // in case of bad routing we can see this in the logs
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	char Oms[DOMAIN_LEN];
	short BuildType; // this is the product code - so that the same cannot log into wrong domains  //0 = Normal, 1 = DirectTrade, 2 = ABN Trade; 3 = B of A
	char Spare2[2];  // alignment causes spare
	int ProductType; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver,0x80=TWIST
	int BuildNo;
	int LineID;
	int VerNo;       // the fe version number – I think current is 9
	short DocSignVersion; // should be in 5001
	char LogonFlags;  // first time or not – used as a bool for multiple account login 
	char EncryptionType;    //  PASSWORD Encryption type 0-Plain text, 1-MD5 2-IQ-SGA-512 3=TWIST Token
	char ClientIP[20];      // IP of client for which this message is on behalf
	int ClientPort;         // The client connecting port isn’t very useful, so maybe the WSOCKS USR portno
	int PassLen;
// + DT11185 - bjl - BO Feed server monitoring
	unsigned long IQClientIP;
	unsigned long IQClientATSIP;
	unsigned short IQClientPort;
	unsigned short IQClientATSPort;
// - DT11185
	char Spare[64];
}  MSG5000REC; //256 +ePassword

// 5001 Global login response
typedef struct tdMsg5001Rec
{
	char ServiceName[64];
	char Domain[DOMAIN_LEN];
	char User[USER_LEN];
	char Account[ACCOUNT_LEN];
	int ProductType; // 0x01=Level1 0x02=Level2 0x04=IqExpress, 0x08=IPLink, 0x10=Backoffice,0x20=IqView,0x40=Fixserver
	char LoginOk;
	char RejectCode; // 0-OK, 1-Password failure, 2-Unable to connect
	char RejectReason[80];
	BYTE  cEnable[32][32];  // Entitlements (union with RejectReason?)
	int LineID;
	short DocSignVersion;
	char passwdStatus;            // 0 - OK, 1 - Expiring, 2 - Expired
	int PwdExpireDate;// date the password expires
	char ClientIP[20];      // Copied from msg 5000
	int ClientPort;         // Copied from msg 5000
	char Spare[20];
}  MSG5001REC; // 1280 bytes



// Sync/Connection Message for Backup Source & Repository
typedef struct tdMsg5432Rec
{
	typedef enum
	{
		unknown,
		fileSource,
		fileRepository
	} BackupType;

	char name[32];
	BackupType type;				// Is sender a Source or Repository

	// NTFS time of the most currently modified file in the file list, always Source time
	unsigned _int64 latestFileListTimeStamp;

	union {
		unsigned long Flags;		// Up to 32 bools in a convenient container, perfect for travel...
		struct
		{
			bool resetFileList : 1;
			bool Spare1	: 1;
			bool Spare2 : 1;
			bool Spare3 : 1;
			bool Spare4 : 1;
			bool Spare5 : 1;
			bool Spare6 : 1;
			bool Spare7 : 1;
			bool Spare8 : 1;
			bool Spare9 : 1;
			bool Spare10 : 1;
		};
	};

	char spare[128];
} MSG5432REC;

// File List entry from Backup Source to Repository
typedef struct tdMsg5433Rec
{
	unsigned long index;

	// NTFS mod time of the file
	unsigned _int64 fileTimeStamp;

	char fileName[260];

	char spare[64];
} MSG5433REC;

// Market Participant Position - Binary forward of message to CustomStreamServer
/*typedef struct tdMsg5437
{
	char data[1];
} MSG5436REC;
*/
// 1036 : Admin Order Overrride
typedef struct tdMsg1036Rec
{
	long SessionID;
	long MessageID;

	int    Action; //0=nothing. 1=UR out, 2=Price correction, 3=Shares etc........................

	char   Domain[DOMAIN_LEN];
	char   Account[ACCOUNT_LEN];

	char   OrderNo[ORDERNO_LEN];
	long   TradeDetRef;
	char   Reason[128];
	
	long	Shares;
	double	Price;

} MSG1036REC;

// FIXServer connection information for THEEYE
typedef struct tdMsg5438Rec
{
	bool bConnected;
	char szInfo      [256];
} MSG5438REC;

// FIXServer connection management information for THEEYE
typedef struct tdMsg5439Rec
{
	bool Reset;
	bool Done;
} MSG5439REC;

// + DT7704 - bjl - Reading 201 Flag from Feed
// RegSHO Status Message
typedef struct tdMsg6503Rec
{
	char szSymbol[21];
	char cStatus;
	union
	{
		char cFlags;
		struct
		{
			bool bStart : 1;
			bool bEnd : 1;
			bool cRestricted : 1;
		};
	};
	char szReserved[9];
	// 32 bytes total
} MSG6503REC;

// RegSHO Status Request Message
typedef struct tdMsg6504Rec
{
      char szReserved[32];
      // 32 bytes total
} MSG6504REC;

// - DT7704

#define MSG1037REC MSG1036REC

typedef struct tdMsg6310Rec
{
	long SessionID;
	long MessageID;
	char UserId[64];
	char Token[64];
	char UTC [64];

	int LoginMsgId;
	
	int UserPortNo;
	int UserPortType;

} MSG6310REC;

typedef struct tdMsg6311Rec
{
	tdMsg6310Rec msg6310rec;
	long retcode;
	char Reason[256];
} MSG6311REC;

typedef struct tdMsg6312Rec		// bus query request message
{
	char content;				// variable length bus query request date 
} MSG6312REC;

typedef struct tdMsg6313Rec		// bus query response message
{
	char content;				// variable length bus response data
} MSG6313REC;

typedef struct tdMsg6901Rec
{
	long SessionID;
	long MessageID;

} MSG6901REC;

typedef struct tdMsg6902Rec
{
	long SessionID;
	long MessageID;

} MSG6902REC;

#endif // __MESSAGE_H
