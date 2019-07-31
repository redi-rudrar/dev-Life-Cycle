// feedproxapi.h : main header file for the feedproxapi DLL
//
#ifndef _IQCSPAPI_H
#define _IQCSPAPI_H
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

#include "iqclientapi.h"
#include "feedfilter.h"

#ifdef WIN32
#ifdef MAKE_STATIC
#define CSPEXPORT
#elif defined(MAKE_IQCSPAPI)
#define CSPEXPORT __declspec(dllexport)
#else
#define CSPEXPORT __declspec(dllimport)
#endif
#else
#define CSPEXPORT
#endif

// Callback interface
class IQCspNotify
{
public:
	// Notifies an error condition
	virtual int NotifyErrorFP(int PortType, int PortNo, int ecode, const char *emsg)=0;
	virtual int NotifyEventFP(int PortType, int PortNo, int ecode, const char *emsg)=0;
	// Notifies an heartbeat
	virtual int NotifyHeartbeatFP(int PortType, int PortNo)=0;
	// Notifies feed
	virtual int NotifyFilter(int PortType, int PortNo, SymHeader *shdr, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Time(int PortType, int PortNo, const char *symbol, Lvl1Time *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Trade(int PortType, int PortNo, const char *symbol, Lvl1Trade *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1NBBO(int PortType, int PortNo, const char *symbol, Lvl1NBBO *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Summary(int PortType, int PortNo, const char *symbol, Lvl1Summary *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Imbal(int PortType, int PortNo, const char *symbol, Lvl1Imbalance *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Regional(int PortType, int PortNo, const char *symbol, Lvl2Regional *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Book(int PortType, int PortNo, const char *symbol, Lvl2Book *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Mmid(int PortType, int PortNo, const char *symbol, Lvl2Mmid *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl2Broker(int PortType, int PortNo, const char *symbol, Lvl2Broker *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1ListingEx(int PortType, int PortNo, const char *symbol, Lvl1ListingEx *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Composite(int PortType, int PortNo, const char *symbol, Lvl1Composite *pquote, void *udata, const char *rptr, int rlen)=0;
	virtual int NotifyLvl1Analytics(int PortType, int PortNo, const char *symbol, Lvl1Analytics *pquote, void *udata, const char *rptr, int rlen)=0;
};
// InstaQuote feed protocol
// Use this interface if you want to manage your own TCP connections.
// Use FeedproxConn if you want the API to manage the TCP connections for you.
class IQCspApi
{
public:
	CSPEXPORT IQCspApi();
	CSPEXPORT int Init(IQCspNotify *pnotify, int PortType, int PortNo);
	CSPEXPORT int Shutdown();
	CSPEXPORT int Parse(const char *buf, int blen, bool whole, void *udata);

	CSPEXPORT static const char *GetAttrName(IQATTR attr);
	CSPEXPORT static const char *GetMarketName(IQMarketCenter mc);
	CSPEXPORT static IQAssetType MarketCode(char exch, char reg, IQMarketCenter& mc);
	CSPEXPORT static IQAssetType MarketCode(IQMarketCenter mc, char& exch, char& reg);
	CSPEXPORT static IQAssetType ListingMarket(IQMarketCenter mc, IQMarketCenter& lexch);

	CSPEXPORT static char GetComstockChar(IQATTRENTRY *pattr);
	CSPEXPORT static int GetComstockStr(IQATTRENTRY *pattr, char *sbuf, int slen);
	CSPEXPORT static int GetComstockMMID(IQATTRENTRY *pattr, char *sbuf, int slen);
	CSPEXPORT static __int64 GetComstockLong(IQATTRENTRY *pattr);
	CSPEXPORT static __int64 GetComstockShares(IQATTRENTRY *pattr, int lotsize);
	CSPEXPORT static double GetComstockPrice(IQATTRENTRY *pattr);
	CSPEXPORT static int GetComstockMultiCode(IQATTRENTRY *pattr, char *sbuf, int slen);

	FeedStats feedStats;

protected:
	IQCspNotify *pnotify;
	int PortType;
	int PortNo;

	enum UquoteType
	{
		UQT_UNKNOWN,
		UQT_LVL1TIME,
		UQT_LVL1TRADE,
		UQT_LVL1NBBO,
		UQT_LVL1SUMMARY,
		UQT_LVL1IMBAL,
		UQT_LVL2REGIONAL,
		UQT_LVL2BOOK,
		UQT_LVL2MMID,
		UQT_LVL2BROKER,
		UQT_LVL1LISTINGEX,
		UQT_LVL1COMPOSITE,
		UQT_LVL1ANALYTICS,
		UQT_COUNT
	};
	struct Uquote
	{
		// Temporary
		UquoteType qtype;
		int nbats;
		SymHeader shdr;
		char lastFormat;
		IQATTRENTRY fmt;
		bool l2haveBid;
		IQATTRENTRY l2bidPx;
		IQATTRENTRY l2bidSize;
		bool l2haveAsk;
		IQATTRENTRY l2askPx;
		IQATTRENTRY l2askSize;
		IQATTRENTRY l2qcond;
		int side;
		IQATTRENTRY wsdate;
		IQATTRENTRY wstime;
		// Notify results
		Lvl1Time l1time;
		Lvl1Trade l1trade;
		Lvl1NBBO l1nbbo;
		Lvl1Summary l1sum;
		Lvl1Imbalance l1imbal;
		Lvl2Regional l2reg;
		Lvl2Book l2book;
		Lvl2Mmid l2mmid;
		Lvl2Broker l2broker;
		Lvl1ListingEx l1listingex;
		Lvl1Composite l1composite;
		Lvl1Analytics l1analytics;
	};
	Uquote uquote;

	int ParseBate(const char *& bptr, const char *& nptr, Uquote& upquote, IQCspNotify *pnotify);
	static int SkipBate(const char *& bptr, const char *& nptr);
	static inline IQATTRENTRY *SetAttr(IQATTRENTRY *pattr, IQATTR attr, IQATTRTYPE atype, void *val, unsigned short vlen)
	{
		pattr->attr=attr;
		pattr->atype=atype;
		pattr->value=val;
		pattr->vlen=vlen;
		return pattr;
	}
};

#endif//_IQCSPAPI_H
