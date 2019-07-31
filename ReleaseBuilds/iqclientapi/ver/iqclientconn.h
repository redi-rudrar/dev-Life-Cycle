
#ifndef _IQCLIENTCONN_H
#define _IQCLIENTCONN_H
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

#ifndef SOCKET
#define SOCKET int
#endif

class IqclientConn :public IqclientNotify
{
public:
	// Two-phase API initialization
	IQEXPORT IqclientConn();
	IQEXPORT ~IqclientConn();
	IQEXPORT int Init(IqclientNotify *pnotify, void *idata);
	IQEXPORT int Shutdown();

	// Session
	IQEXPORT int Login(const char **dnslist, int ndns, const char *domain, const char *user, const char *passwd, int lineid);
	IQEXPORT int Logout();
	IQEXPORT int Heartbeat();
	IQEXPORT int Disconnected();

	// Instrument reference data
	IQEXPORT int GetSymbolAttr(const char *symbol, IQATTRLIST& alist, void *udata);

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

	// Receive a message from IQ
	IQEXPORT int RecvIQ(const char *buf, int blen);

	// Attributes
	IQEXPORT static const char *GetAttrName(IQATTR attr);
	IQEXPORT static const char *GetMarketName(IQMarketCenter mc);

	// Called to make a connection to IQ
	IQEXPORT int ConnectIQ(struct sockaddr_in *addr, int alen);
	// Called to drop a connection from IQ
	IQEXPORT int DisconnectIQ(struct sockaddr_in *addr, int alen);
	// Called to send a message to IQ
	IQEXPORT int SendIQ(const char *buf, int blen);
	// Called to wait for message response
	IQEXPORT int WaitIQ();

private:
	class IqcapiImpl *aimpl;
	SOCKET sd;
	IqclientNotify *pnotify;

	// For IqclientNotify
	// Called to update dns ip list
	int DnsListIQ(void *idata, const char **dnslist, int ndns);
	// Called to make a connection to IQ
	int ConnectIQ(void *idata, struct sockaddr_in *addr, int alen);
	// Called to drop a connection from IQ
	int DisconnectIQ(void *idata, struct sockaddr_in *addr, int alen);
	// Called to send a message to IQ
	int SendIQ(void *idata, const char *buf, int blen);
	// Called to wait for message response
	int WaitIQ(void *idata);

	// Notifies an error condition
	int NotifyErrorIQ(void *idata, int ecode, const char *emsg);
	// Notifies an heartbeat
	int NotifyHeartbeatIQ(void *idata);
	// Notifies symbol reference data
	int NotifySymbolAttr(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata);
	// Notifies historic quote
	int NotifyLvl1Hist(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata);
	int NotifyLvl2Hist(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata);
	// Notifies feed
	int NotifyFilter(void *idata, SymHeader *shdr, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Time(void *idata, const char *symbol, Lvl1Time *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Trade(void *idata, const char *symbol, Lvl1Trade *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1NBBO(void *idata, const char *symbol, Lvl1NBBO *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Summary(void *idata, const char *symbol, Lvl1Summary *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Imbal(void *idata, const char *symbol, Lvl1Imbalance *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Regional(void *idata, const char *symbol, Lvl2Regional *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Book(void *idata, const char *symbol, Lvl2Book *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Mmid(void *idata, const char *symbol, Lvl2Mmid *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Broker(void *idata, const char *symbol, Lvl2Broker *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1ListingEx(void *idata, const char *symbol, Lvl1ListingEx *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Composite(void *idata, const char *symbol, Lvl1Composite *pquote, void *udata, const char *rptr, int rlen);
	// Notifies chart data
	int NotifyChart(void *idata, const char *symbol, IQATTRLIST& alist, int asize, void *udata);
	// Notifies news data
	int NotifyHeadline(void *idata, long date, const char *artid, const char *headline, const char *keywords, void *udata);
	int NotifyNewsArticle(void *idata, long date, const char *artid, const char *keywords, const char *body, int bodylen, void *udata);
};

#endif//_IQCLIENTCONN_H
