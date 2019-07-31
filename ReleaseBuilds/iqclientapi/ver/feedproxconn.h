
#ifndef _FEEDPROXCONN_H
#define _FEEDPROXCONN_H
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

#include "iqcspapi.h"

#ifdef MAKE_FEEDPROXAPI
#define FPEXPORT __declspec(dllexport)
#else
#define FPEXPORT __declspec(dllimport)
#endif+

// Callback interface
class FeedproxNotify
{
public:
	// Notifies an error condition
	virtual int NotifyErrorIQ(void *idata, int ecode, const char *emsg)=0;
	// Notifies an heartbeat
	virtual int NotifyHeartbeatIQ(void *idata)=0;
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
};

class FeedproxConn :public IQCspNotify
{
public:
	// Two-phase API initialization
	FPEXPORT FeedproxConn();
	FPEXPORT ~FeedproxConn();
	FPEXPORT int Init(FeedproxNotify *pnotify, void *idata);
	FPEXPORT int Shutdown();

	// Session
	// Called to make a connection to IQ
	FPEXPORT int ConnectIQ(const char *ip, int port);
	// Called to drop a connection from IQ
	FPEXPORT int DisconnectIQ();
	// Called to send a message to IQ
	FPEXPORT int SendIQ(const char *buf, int blen);
	// Called to wait for message response
	FPEXPORT int WaitIQ();
	// Receive a message from IQ
	FPEXPORT int RecvIQ(const char *buf, int blen);
	// Send a heartbeat
	FPEXPORT int Heartbeat();

private:
	FeedproxNotify *pnotify;
	void *idata;
	SOCKET sd;
	IQCspApi *aimpl;
	struct tdBuffer *wsb;
	char DecompBuff[128*1024*2];

	// For IQCspNotify
	// Notifies an error condition
	int NotifyErrorFP(int PortType, int PortNo, int ecode, const char *emsg);
	// Notifies an heartbeat
	int NotifyHeartbeatFP(int PortType, int PortNo);
	// Notifies feed
	int NotifyFilter(int PortType, int PortNo, SymHeader *shdr, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Time(int PortType, int PortNo, const char *symbol, Lvl1Time *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Trade(int PortType, int PortNo, const char *symbol, Lvl1Trade *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1NBBO(int PortType, int PortNo, const char *symbol, Lvl1NBBO *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Summary(int PortType, int PortNo, const char *symbol, Lvl1Summary *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Imbal(int PortType, int PortNo, const char *symbol, Lvl1Imbalance *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Regional(int PortType, int PortNo, const char *symbol, Lvl2Regional *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Book(int PortType, int PortNo, const char *symbol, Lvl2Book *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Mmid(int PortType, int PortNo, const char *symbol, Lvl2Mmid *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl2Broker(int PortType, int PortNo, const char *symbol, Lvl2Broker *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1ListingEx(int PortType, int PortNo, const char *symbol, Lvl1ListingEx *pquote, void *udata, const char *rptr, int rlen);
	int NotifyLvl1Composite(int PortType, int PortNo, const char *symbol, Lvl1Composite *pquote, void *udata, const char *rptr, int rlen);
};

#endif//_FEEDPROXCONN_H
