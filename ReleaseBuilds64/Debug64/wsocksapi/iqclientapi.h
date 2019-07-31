
#ifndef _IQCLIENTAPI_H
#define _IQCLIENTAPI_H
// 
// iqclientapi.h : main header file for the iqclientapi DLL
//
// THE INFORMATION CONTAINED HEREIN IS PROVIDED “AS IS” AND NO PERSON OR ENTITY ASSOCIATED WITH THE 
// INSTAQUOTE MARKET DATA API MAKES ANY REPRESENTATION OR WARRANTY, EXPRESS OR IMPLIED, AS TO THE 
// INSTAQUOTE MARKET DATA API (OR THE RESULTS TO BE OBTAINED BY THE USE THEREOF) OR ANY OTHER MATTER 
// AND EACH SUCH PERSON AND ENTITY SPECIFICALLY DISCLAIMS ANY WARRANTY OF ORIGINALITY, ACCURACY, 
// COMPLETENESS, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  SUCH PERSONS AND ENTITIES 
// DO NOT WARRANT THAT THE INSTAQUOTE MARKET DATA API WILL CONFORM TO ANY DESCRIPTION THEREOF OR 
// BE FREE OF ERRORS.  THE ENTIRE RISK OF ANY USE OF THE INSTAQUOTE MARKET DATA API IS ASSUMED BY THE USER.
//
// NO PERSON OR ENTITY ASSOCIATED WITH THE INSTAQUOTE MARKET DATA API SHALL HAVE ANY LIABILITY FOR 
// DAMAGE OF ANY KIND ARISING IN ANY MANNER OUT OF OR IN CONNECTION WITH ANY USER’S USE OF (OR ANY 
// INABILITY TO USE) THE INSTAQUOTE MARKET DATA API, WHETHER DIRECT, INDIRECT, INCIDENTAL, SPECIAL 
// OR CONSEQUENTIAL (INCLUDING, WITHOUT LIMITATION, LOSS OF DATA, LOSS OF USE, CLAIMS OF THIRD PARTIES 
// OR LOST PROFITS OR REVENUES OR OTHER ECONOMIC LOSS), WHETHER IN TORT (INCLUDING NEGLIGENCE AND STRICT 
// LIABILITY), CONTRACT OR OTHERWISE, WHETHER OR NOT ANY SUCH PERSON OR ENTITY HAS BEEN ADVISED OF, 
// OR OTHERWISE MIGHT HAVE ANTICIPATED THE POSSIBILITY OF, SUCH DAMAGES.
//
// No proprietary or ownership interest of any kind is granted with respect to the INSTAQUOTE MARKET DATA API 
// (or any rights therein).
//
// Copyright © 2009 Banc of America Securities, LLC. All rights reserved.
//
#ifdef WIN32
#ifdef MAKE_IQCLIENTAPI
#define IQEXPORT __declspec(dllexport)
#else
#define IQEXPORT __declspec(dllimport)
#endif
#else
#define IQEXPORT
#endif

#ifndef __int64
#define __int64 long long
#endif

// API data types
#pragma pack(push,1)
struct IQPRICE
{
	char PriceCode;
	char Market;
	__int64 Integral;
};

enum IQATTRTYPE
{
	IQAT_UNKNOWN,
	IQAT_GROUP,
	IQAT_NULL,
	IQAT_UCHAR,		// Host-order intrinsic types
	IQAT_WORD16,	
	IQAT_INT32,
	IQAT_UINT32,
	IQAT_INT64,
	IQAT_UINT64,
	IQAT_FLOAT,
	IQAT_DOUBLE,
	IQAT_PCHAR,
	IQAT_PRICE,		// IQPRICE
	IQAT_SHARES,	// UINT32
	IQAT_BINARY,	// WORD16, UCHAR[]
	IQAT_INT7,		// Stop-bit signed int encoding (2's complement)
	IQAT_UINT7,		// Stop-bit uint encoding
	IQAT_FLOAT7,	// Stop-bit signed int mantissa, uint precision
	IQAT_COMSTOCK,	// Comstock
	IQAT_COUNT
};
// Symbol attributes
enum IQATTR
{
	IQA_UNKNOWN			=0,
	// Reference
	IQA_SYMBOL			=1,
	IQA_REFERENCE		=2,
	IQA_IDSOURCE		=3,
	IQA_LISTING_EXCHANGE=4,
	IQA_ASSET_TYPE		=5,
	IQA_FULL_NAME		=6,
	IQA_LOT_SIZE		=7,
	IQA_DISPLAY_FORMAT	=8,
	IQA_UNDERLYER		=9,
	IQA_MININUM_FLUCTUATION=10,
	IQA_FLUCTUATION_VALUE=11,
	IQA_CURRENCY		=12,
	IQA_EXPIRATION		=13,
	IQA_STRIKE_PX		=14,
	IQA_CUSIP			=15,
	IQA_ISIN			=16,
	// Quoting
	IQA_HISTORIC		=20,
	IQA_LINEID			=21,
	IQA_SEQNO			=22,
	IQA_GROUP_COUNT		=23,
	// Level 1
	IQA_REALTIME		=31,
	IQA_DELAYED			=32,
	IQA_QUOTE_EXCHANGE	=33,
	IQA_DATE			=34,
	IQA_TIME			=35,
	IQA_OPEN_PX			=36,
	IQA_HIGH_PX			=37,
	IQA_LOW_PX			=38,
	IQA_LAST_PX			=39,
	IQA_LAST_SIZE		=40,
	IQA_LAST_REGULAR_PX	=41,
	IQA_NETCHANGE_PX	=42,
	IQA_VOLUME			=43,
	IQA_FORMT_VOLUME	=44,
	IQA_PREV_VOLUME		=45,
	IQA_OPEN_INTEREST	=46,
	IQA_BID_PX			=47,
	IQA_BID_SIZE		=48,
	IQA_BID_REGION		=49,
	IQA_BID_CONDITION	=50,
	IQA_ASK_PX			=51,
	IQA_ASK_SIZE		=52,
	IQA_ASK_REGION		=53,
	IQA_ASK_CONDITION	=54,
	IQA_QUOTE_CONDITION	=55,
	IQA_TRADE_PX		=56,
	IQA_TRADE_SIZE		=57,
	IQA_TRADE_REGION	=58,
	IQA_SALE_CONDITION	=59,
	IQA_LASTTRADE_DATE	=60,
	IQA_LASTTRADE_TIME	=61,
	IQA_CLOSE_PX		=62,
	IQA_PREVCLOSE_PX	=63,
	IQA_PREVCLOSE_DATE	=64,
	IQA_PREVCLOSE_TIME	=65,
	IQA_SETTLEMENT_PX	=66,
	IQA_VWAP1_PX		=67,
	IQA_VWAP2_VOL		=68,
	IQA_VWAP_SUM		=69,
	IQA_VWAP_VOLUME		=70,
	// Level 2
	IQA_LVL2_BEGIN		=80,
	IQA_LVL2_END		=81,
	IQA_LVL2_GROUP		=82,
	IQA_LVL2_REGION		=83,
	IQA_MMID			=84,
	IQA_BROKERNO		=85,
	IQA_BOOK_LEVEL		=86,
	// Level 1
	IQA_LVL1_BBO		=87,
	IQA_LVL1_TRADE		=88,
	// Level 3
	IQA_LVL3_GROUP		=90,
	IQA_CLORDID			=91,
	IQA_ORDERID			=92,
	IQA_MSGSEQNUM		=93,
	IQA_SIDE			=94,
	// Chart
	IQA_BAR_GROUP		=100,
	IQA_CHART_BEGIN		=101,
	IQA_CHART_END		=102,
	IQA_CHART_INTERVAL	=103,
	// Option market center volumes
	IQA_ASE_VOLUME		=104,
	IQA_BOX_VOLUME		=105,
	IQA_CBO_VOLUME		=106,
	IQA_ISE_VOLUME		=107,
	IQA_PSE_VOLUME		=108,
	IQA_PHS_VOLUME		=109,
	IQA_NDQ_VOLUME		=110,
	IQA_ASE_TICK		=111,
	IQA_BOX_TICK		=112,
	IQA_CBO_TICK		=113,
	IQA_ISE_TICK		=114,
	IQA_PSE_TICK		=115,
	IQA_PHS_TICK		=116,
	IQA_NDQ_TICK		=117,
	// Extensions
	IQA_TIMESTAMP		=118,
	IQA_SUMMARY			=119,
	IQA_CORRECTION_NO	=120,
	//IQA_ORDTYPE			=121,
	//IQA_EXECINST		=122,
	IQA_IMBALANCE_PX	=123,
	IQA_IMBALANCE_SIZE	=124,
	IQA_IMBALANCE_SIDE	=125,
	IQA_IMBALANCE_PAIRED_SIZE=126,
	IQA_IMBALANCE_STATUS=127,
	IQA_IMBALANCE_TIME	=128,
	IQA_CLOSE_SYMBOL    =129,
	IQA_BAT_VOLUME		=130,
	IQA_BAT_TICK		=131,
	IQA_C2_VOLUME		=132,
	IQA_C2_TICK			=133,
	IQA_COMPOSITE_BID   =134,
	IQA_COMPOSITE_ASK   =135,
	IQA_MULTI_SALE_COND =136,
	IQA_BS_THEO_PX		=137,
	IQA_BS_GAMMA		=138,
	IQA_BS_DELTA		=139,
	IQA_BS_RHO			=140,
	IQA_BS_VEGA			=141,
	IQA_BS_THETA		=142,
	IQA_BS_VOL			=143,
	IQA_BS_IMPLIED_VOL	=144,
	IQA_BS_IVBID		=145,
	IQA_BS_IVASK		=146,
	IQA_BINOMIAL_THEO_PX=147,
	IQA_BINOMIAL_GAMMA	=148,
	IQA_BINOMIAL_DELTA	=149,
	IQA_BINOMIAL_RHO	=150,
	IQA_BINOMIAL_VEGA	=151,
	IQA_BINOMIAL_THETA	=152,
	IQA_BINOMIAL_VOL	=153,
	IQA_BINOMIAL_IMPLIED_VOL=154,
	IQA_BINOMIAL_IVBID	=155,
	IQA_BINOMIAL_IVASK	=156,
	IQA_LVL1_ANALYTICS	=157,
	IQA_COUNT
};
//// List of attributes
//struct IQATTRLIST
//{
//	IQATTR attr;
//	IQATTRTYPE atype;
//	bool dynamic;
//	char reserved;
//	unsigned short vlen;
//	union{
//		void *value;
//		IQATTRLIST *group;
//	};
//	IQATTRLIST *prev,*next;
//};
// Single attribute
struct IQATTRENTRY
{
	IQATTR attr;
	IQATTRTYPE atype;
	bool dynamic;
	char reserved;
	unsigned short vlen;
	union{
		void *value;
		struct IQATTRLIST *group;
	};
};
// List of attributes
struct IQATTRLIST :public IQATTRENTRY
{
	IQATTRLIST *prev,*next;
};

// Delivery options
typedef unsigned long IQDELIVERYOPTION;
#define IQDO_NONE			0x0000
#define IQDO_LAST_QUOTE		0x0001		// Want the last quote before the feed
#define IQDO_REALTIME		0x0002		// Want realtime feed
#define IQDO_DELAYED		0x0004		// Want delayed feed
#define IQDO_ONESEC			0x0008		// Want one-second feed
#define IQDO_TRADES_ONLY	0x0010		// Want only trades for lvl1
#define IQDO_PRICE_CHANGE	0x0020		// BBO and lvl2 only when price changes

enum IQAssetType
{
	IQAST_UNKNOWN,
	IQAST_EQUITY,
	IQAST_EQOPTION,
	IQAST_FUTURE,
	IQAST_EQINDEX,
	IQAST_TIME,
	IQAST_TOPTEN_LIST,
	IQAST_TEST,
	IQAST_COUNT
};

enum IQMarketCenter
{
	IQMC_UNKNOWN,
	// Listed
	IQMC_NYSE,					// New York Stock Exchange
	IQMC_NYSE_ARCA,				// Archipelago
	IQMC_BATS,					// BATS BZX Equity Exchange
	IQMC_CBSX,					// CBOE Stock Exchange
	IQMC_CINN,					// Cincinnati Stock Exchange
	IQMC_CHX,					// Chicago Stock Exchange
	IQMC_ISEQ,					// International Stock Exchange Equities
	IQMC_NASD,					// National Association of Securities Dealers
	IQMC_BSE,					// Boston Stock Exchange
	IQMC_PHLX,					// Philadelphia Stock Exchange
	IQMC_CBOEIDX,				// CBOE Indices
	IQMC_NYSE_CQS,				// NYSE CQS/CTS quote
	IQMC_AMEX_CQS,				// AMEX CQS/CTS quote
	IQMC_NAS_CQS,				// NASDAQ CQS/CTS quote
	IQMC_NYSE_AMEX,				// American Stock Exchange
	// Nasdaq
	IQMC_NASDAQ,				// NASDAQ Stock Exchange
	IQMC_NASL2,					// NASDAQ Level 2
	IQMC_NASDAQ_MFDS,			// NASDAQ Mutual Funds
	IQMC_NASDAQ_MONEY,			// NASDAQ Money Market
	// OTCs
	IQMC_OTCPINK,				// OTC Pink Sheets
	IQMC_OTCBB,					// OTC Bulletin Boards
	// Dept of book
	IQMC_ARCABOOK,				// ARCA depth of book
	IQMC_NASDAQ_SINGLEBOOK,		// NSDQ depth of book
	IQMC_NYSE_OPENBOOK,			// NYSE limit-order book
	IQMC_BLOOMBERG_TRADEBOOK,	// Bloomberg depth of book
	// International
	IQMC_TSXL1,					// Toronto Stock Exchange
	IQMC_TSXL2,					// TSX level 2
	IQMC_VENTUREL1,				// TSX Venture Exchange
	IQMC_VENTUREL2,				// Venture level 2
	// Indices
	IQMC_NASDAQ_NIDS,			// NASDAQ indices
	IQMC_OPRAIDX,				// OPRA indices
	IQMC_DJIDX,					// Dow Jones indices
	IQMC_PBOTIDX,				// Philadelphia Board of Trade indices
	IQMC_CBOTIDX,				// Chicago Board of Trade indices
	// Options
	IQMC_OPRA,					// Option Price Reporting Authority
	IQMC_ASE,					// Nyse Amex Options
	IQMC_BOX,					// Boston Options Exchange
	IQMC_CBO,					// Chicago Board Options Exchange
	IQMC_ISE,					// International Securities Exchange Options 
	IQMC_NDQ,					// Nasdaq Options Market
	IQMC_PSE,					// Pacific Stock Exchange Options (NyseArca Options)
	IQMC_PHS,					// Philadelphia Options Exchange (Nasdaq)
	// Futures
	IQMC_CME,					// Chicago Mercantile Exchange
	IQMC_CMEMINI,				// CME E-minis
	IQMC_CMEBOOK,				// CME best limits
	IQMC_CME_CBOT,				// Chicago Board of Trade
	IQMC_CME_CBOTMINI,			// CBOT mini-sized
	IQMC_CBOTBOOK,				// CBOT best limits
	IQMC_CBOT_OPTIONS,			// CBOT future options and options on spreads
	IQMC_CBOT_SPREADS,			// CBOT spreads
	IQMC_CME_NYMEX,				// New York Mercantile Exchange
	IQMC_CME_COMEX,				// Commodity Exchange, Inc.
	IQMC_CME_OPTIONS,			// CME future options and options on spreads
	IQMC_CME_SPREADS,			// CME spreads
	// InstaQuote
	IQMC_IQTIME,				// InstaQuote Time
	IQMC_COMSTOCK_TIME,			// Comstock Time
	IQMC_FEEDBITS,				// Feedbits
	IQMC_MLXN,					// Merrill Lynch ECN
	IQMC_CME_KCBT,				// CME Kansas City
	IQMC_CME_MINNEAPOLIS,		// CME Minneapolis
	IQMC_BAT,					// BATS Options Exchange
	IQMC_C2,					// CBOE's C2 Options Exchange
	IQMC_EDGA,					// EDGA Equity Exchange
	IQMC_EDGX,					// EDGX Equity Exchange
	IQMC_BYX,					// BATS Y Equity Exchange
	IQMC_OTCDUAL,
	IQMC_BOFA,
	IQMC_NASL2_SHARES,			// NASDAQ Level 2 in shares
	IQMC_NBX,					// Nasdaq options market
	IQMC_MIA,					// Miami options market
	IQMC_GEM,					// ISE Gemini options market
	// Others
	IQMC_COUNT, 
};

// Common header for all quotes
struct SymHeader
{
	char exch;
	char reg;
	char sym[32];
	IQMarketCenter mc;
	IQAssetType atype;
	int offset;
};
// IQ time (2) and Comstock time (31)
struct Lvl1Time
{
	IQMarketCenter mc;
	int wsdate;
	int wstime;
};
// Equity, options, and futures trades
struct Lvl1Trade
{
	SymHeader shdr;
	IQATTRENTRY fmt;
	IQATTRENTRY tradeReg;
	IQATTRENTRY tradePx;
	IQATTRENTRY tradeSize;
	IQATTRENTRY saleCond;
	IQATTRENTRY msaleCond;
	IQATTRENTRY seqNo;
	IQATTRENTRY correctNo;
	IQATTRENTRY volume;
	IQATTRENTRY wstime;
};
// Equity, options, and futures NBBO quotes
struct Lvl1NBBO
{
	SymHeader shdr;
	IQATTRENTRY fmt;
	bool haveBid;
	IQATTRENTRY bidReg;
	IQATTRENTRY bidPx;
	IQATTRENTRY bidSize;
	bool haveAsk;
	IQATTRENTRY askReg;
	IQATTRENTRY askPx;
	IQATTRENTRY askSize;
	IQATTRENTRY wstime;
	IQATTRENTRY listingEx;
};
// Nasdaq end of day summary, equity and index intra-day corrections, and futures corrections
struct Lvl1Summary
{
	SymHeader shdr;
	IQATTRENTRY fmt;
	IQATTRENTRY openReg;
	IQATTRENTRY openPx;
	IQATTRENTRY highReg;
	IQATTRENTRY highPx;
	IQATTRENTRY lowReg;
	IQATTRENTRY lowPx;
	IQATTRENTRY lastReg;
	IQATTRENTRY lastPx;
	IQATTRENTRY volume;
	IQATTRENTRY openInterest;
	IQATTRENTRY closePx;
	IQATTRENTRY prevClosePx;
	IQATTRENTRY prevVolume;
	IQATTRENTRY lastRegularPx;
	IQATTRENTRY netChangePx;
	IQATTRENTRY settlementPx;
	IQATTRENTRY f99;
	IQATTRENTRY wstime;
};
// Nyse imbalance feed
struct Lvl1Imbalance
{
	SymHeader shdr;
	IQATTRENTRY fmt;
	IQATTRENTRY side;
	IQATTRENTRY refPx;
	IQATTRENTRY size;
	IQATTRENTRY pairedSize;
	IQATTRENTRY status;
	IQATTRENTRY wstime;
};
// SIAC equity regionals and option market centers
struct Lvl2Regional
{
	SymHeader shdr;
	IQATTRENTRY region;
	IQATTRENTRY fmt;
	bool haveBid;
	IQATTRENTRY bidPx;
	IQATTRENTRY bidSize;
	bool haveAsk;
	IQATTRENTRY askPx;
	IQATTRENTRY askSize;
	IQATTRENTRY quoteCond;
	IQATTRENTRY wstime;
};
// Arcabook, Singlebook, Tradebook, and CmeBook
struct Lvl2Book
{
	SymHeader shdr;
	IQATTRENTRY level;
	IQATTRENTRY fmt;
	bool haveBid;
	IQATTRENTRY bidPx;
	IQATTRENTRY bidSize;
	bool haveAsk;
	IQATTRENTRY askPx;
	IQATTRENTRY askSize;
	IQATTRENTRY quoteCond;
	IQATTRENTRY wstime;
};
// SIAC mmid and Nasdaq level 2
struct Lvl2Mmid
{
	SymHeader shdr;
	IQATTRENTRY mmid;
	IQATTRENTRY fmt;
	bool haveBid;
	IQATTRENTRY bidPx;
	IQATTRENTRY bidSize;
	bool haveAsk;
	IQATTRENTRY askPx;
	IQATTRENTRY askSize;
	IQATTRENTRY quoteCond;
	IQATTRENTRY wstime;
};
// TSX and VEN level 2
struct Lvl2Broker
{
	SymHeader shdr;
	IQATTRENTRY broker;
	IQATTRENTRY fmt;
	bool haveBid;
	IQATTRENTRY bidPx;
	IQATTRENTRY bidSize;
	bool haveAsk;
	IQATTRENTRY askPx;
	IQATTRENTRY askSize;
	IQATTRENTRY quoteCond;
	IQATTRENTRY wstime;
};

struct Lvl1ListingEx
{
	SymHeader shdr;
	IQATTRENTRY listingEx;
};

struct Lvl1Composite
{
	SymHeader shdr;
	IQATTRENTRY compositeBidSize;
	IQATTRENTRY compositeAskSize;
};

struct Lvl1Analytics
{
	SymHeader shdr;
	IQATTRENTRY fmt;
	IQATTRENTRY bsTheoPx;
	IQATTRENTRY bsGamma;
	IQATTRENTRY bsDelta;
	IQATTRENTRY bsRho;
	IQATTRENTRY bsVega;
	IQATTRENTRY bsTheta;
	IQATTRENTRY bsVol;
	IQATTRENTRY bsImpliedVol;
	IQATTRENTRY bsIVBid;
	IQATTRENTRY bsIVAsk;
	IQATTRENTRY binTheoPx;
	IQATTRENTRY binGamma;
	IQATTRENTRY binDelta;
	IQATTRENTRY binRho;
	IQATTRENTRY binVega;
	IQATTRENTRY binTheta;
	IQATTRENTRY binVol;
	IQATTRENTRY binImpliedVol;
	IQATTRENTRY binIVBid;
	IQATTRENTRY binIVAsk;
};

// Chart intervals
#define DAILY_CHART_INTERVAL 86400
#define MINUTE_CHART_INTERVAL 60

// Callback interface
class IqclientNotify
{
public:
	// Called to update dns ip list
	virtual int DnsListIQ(void *idata, const char **dnslist, int ndns)=0;
	// Called to make a connection to IQ
	virtual int ConnectIQ(void *idata, struct sockaddr_in *addr, int alen)=0;
	// Called to drop a connection from IQ
	virtual int DisconnectIQ(void *idata, struct sockaddr_in *addr, int alen)=0;
	// Called to send a message to IQ
	virtual int SendIQ(void *idata, const char *buf, int blen)=0;
	// Called to wait for message response
	virtual int WaitIQ(void *idata)=0;

	// Notifies an error condition
	virtual int NotifyErrorIQ(void *idata, int ecode, const char *emsg)=0;
	// Notifies an heartbeat
	virtual int NotifyHeartbeatIQ(void *idata)=0;
	// Notifies symbol reference data
	virtual int NotifySymbolAttr(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata)=0;
	// Notifies historic quote
	virtual int NotifyLvl1Hist(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata)=0;
	virtual int NotifyLvl2Hist(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata)=0;
	// Notifies feed
	virtual int NotifyFilter(void *idata, SymHeader *shdr, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Time(void *idata, const char *symbol, Lvl1Time *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Trade(void *idata, const char *symbol, Lvl1Trade *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1NBBO(void *idata, const char *symbol, Lvl1NBBO *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Summary(void *idata, const char *symbol, Lvl1Summary *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Imbal(void *idata, const char *symbol, Lvl1Imbalance *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Regional(void *idata, const char *symbol, Lvl2Regional *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Book(void *idata, const char *symbol, Lvl2Book *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Mmid(void *idata, const char *symbol, Lvl2Mmid *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Broker(void *idata, const char *symbol, Lvl2Broker *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1ListingEx(void *idata, const char *symbol, Lvl1ListingEx *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Composite(void *idata, const char *symbol, Lvl1Composite *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Analytics(void *idata, const char *symbol, Lvl1Analytics *pquote, void *udata, const char *rptr, int rlen)=0;
	// Notifies chart data
	virtual int NotifyChart(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata)=0;
	// Notifies news data
	virtual int NotifyHeadline(void *idata, long date, const char *artid, const char *headline, const char *keywords, void *udata)=0;
	virtual int NotifyNewsArticle(void *idata, long date, const char *artid, const char *keywords, const char *body, int bodylen, void *udata)=0;
};
// InstaQuote feed protocol
// Use this interface if you want to manage your own TCP connections.
// Use IqclientConn if you want the API to manage the TCP connections for you.
class IqclientApi
{
public:
	// Two-phase API initialization
	IQEXPORT IqclientApi();
	IQEXPORT ~IqclientApi();
	IQEXPORT int Init(IqclientNotify *pnotify, void *idata);
	IQEXPORT int Shutdown();

	// Session
	IQEXPORT int Login(const char **dnslist, int ndns, const char *domain, const char *user, const char *passwd, int lineid);
	IQEXPORT int Logout();
	IQEXPORT int Heartbeat();
	IQEXPORT int Disconnected();

	// Instrument reference data
	//IQEXPORT int GetSymbolAttr(const char *symbol, IQATTRLIST& alist, void *udata);

	// Level 1 feed
	IQEXPORT int RegisterLvl1(const char *symbol, IQDELIVERYOPTION opts, void *udata);
	IQEXPORT int DeregisterLvl1(const char *symbol);

	// Level 2 feed
	IQEXPORT int RegisterLvl2(const char *symbol, IQDELIVERYOPTION opts, void *udata);
	IQEXPORT int DeregisterLvl2(const char *symbol);

	// Level 3 feed
	IQEXPORT int RegisterLvl3(const char *symbol, IQDELIVERYOPTION opts, void *udata);
	IQEXPORT int DeregisterLvl3(const char *symbol);

	// Chart
	IQEXPORT int RegisterChart(const char *symbol, int interval, int maxbars, IQDELIVERYOPTION opts, void *udata);
	IQEXPORT int DeregisterChart(const char *symbol);

	// News
	IQEXPORT int RegisterHeadlines(void *udata);
	IQEXPORT int DeregisterHeadlines(void *udata);
	IQEXPORT int GetHeadlines(const char *keywords, int max, void *udata);
	IQEXPORT int GetNewsArticle(long date, const char *artid, void *udata);

	// Receive a message from IQ
	IQEXPORT int RecvIQ(const char *buf, int blen, bool whole, void *udata);

	// Attributes
	IQEXPORT static const char *GetAttrName(IQATTR attr);
	IQEXPORT static const char *GetMarketName(IQMarketCenter mc);
	IQEXPORT static IQAssetType MarketCode(char exch, char reg, IQMarketCenter& mc);
	IQEXPORT static IQAssetType MarketCode(IQMarketCenter mc, char& exch, char& reg);
	IQEXPORT static IQAssetType ListingMarket(IQMarketCenter mc, IQMarketCenter& lexch);

	IQEXPORT static char GetComstockChar(IQATTRENTRY *pattr);
	IQEXPORT static int GetComstockStr(IQATTRENTRY *pattr, char *sbuf, int slen);
	IQEXPORT static int GetComstockMMID(IQATTRENTRY *pattr, char *sbuf, int slen);
	IQEXPORT static __int64 GetComstockLong(IQATTRENTRY *pattr);
	IQEXPORT static __int64 GetComstockShares(IQATTRENTRY *pattr, int lotsize);
	IQEXPORT static double GetComstockPrice(IQATTRENTRY *pattr);

private:
	class IqcapiImpl *aimpl;
};

#pragma pack(pop)

#endif//_IQCLIENTAPI_H
