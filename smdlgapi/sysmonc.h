
#ifndef SYSMON_H
#define SYSMON_H
//
// THE INFORMATION CONTAINED HEREIN IS PROVIDED <93>AS IS<94> AND NO PERSON OR ENTITY ASSOCIATED WITH THE
// INSTAQUOTE MARKET DATA API MAKES ANY REPRESENTATION OR WARRANTY, EXPRESS OR IMPLIED, AS TO THE
// INSTAQUOTE MARKET DATA API (OR THE RESULTS TO BE OBTAINED BY THE USE THEREOF) OR ANY OTHER MATTER
// AND EACH SUCH PERSON AND ENTITY SPECIFICALLY DISCLAIMS ANY WARRANTY OF ORIGINALITY, ACCURACY,
// COMPLETENESS, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  SUCH PERSONS AND ENTITIES
// DO NOT WARRANT THAT THE INSTAQUOTE MARKET DATA API WILL CONFORM TO ANY DESCRIPTION THEREOF OR
// BE FREE OF ERRORS.  THE ENTIRE RISK OF ANY USE OF THE INSTAQUOTE MARKET DATA API IS ASSUMED BY THE USER.
//
// NO PERSON OR ENTITY ASSOCIATED WITH THE INSTAQUOTE MARKET DATA API SHALL HAVE ANY LIABILITY FOR
// DAMAGE OF ANY KIND ARISING IN ANY MANNER OUT OF OR IN CONNECTION WITH ANY USER<92>S USE OF (OR ANY
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

#pragma pack(push,1)

#define SYSMONLINK

// Notification interface for SysCmdCodec users
class SysCmdNotify
{
public:
	virtual void NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata)=0;
	virtual void NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata)=0;
};
// Encoder for commands and answers to/from a SysmonServer
class SysCmdCodec
{
public:
	SYSMONLINK int Init();
	SYSMONLINK int Shutdown();
	SYSMONLINK int Encode(char *&mptr, int mlen, bool request, int reqid, string cmd, const char *parm, int plen);
	SYSMONLINK int Decode(const char *&mptr, int mlen, bool request, SysCmdNotify *pnotify, void *udata);
	// Convenience functions
	SYSMONLINK int GetTagValues(TVMAP& tvmap, const char *parm, int plen);
	SYSMONLINK string GetValue(TVMAP& tvmap, const string tag);
	SYSMONLINK bool HasValue(TVMAP& tvmap, const string tag);
	SYSMONLINK int SetTagValues(TVMAP& tvmap, char *parm, int plen);
	SYSMONLINK int SetValue(TVMAP& tvmap, const string tag, string value);
	SYSMONLINK int DelTag(TVMAP& tvmap, const string tag);
	SYSMONLINK int EscapeText(char *etext, const char *text);
	SYSMONLINK int UnescapeText(char *etext, const char *text);
};

// Notification interface for SysFeedCodec users
struct SMFEEDS
{
	union u1
	{
		struct wantBits
		{
		DWORD wantLvl1:1;	// i.e. connlist
		DWORD wantLvl2:1;	// i.e. mportclient
		DWORD wantError:1;	// i.e. the eye
		DWORD wantEvent:1;	// i.e. mportclient
		//DWORD wantRetry:1;	// i.e. mportclient
		DWORD wantTrademon:1; // i.e. trademon
        DWORD wantHeartbeat:1;
		DWORD reserved:25;
		DWORD wantCustom:1; // i.e. app-defined
		} bits;
		DWORD dwWant;
	} uwant;
	DWORD lvl1Interval;
	DWORD lvl1UpdateTime;
	DWORD lvl2Interval;
	DWORD lvl2UpdateTime;
};
// dwWant masks
#define SMFMASK_LVL1		(0x00000001)
#define SMFMASK_LVL2		(0x00000002)
#define SMFMASK_ERRORLOG	(0x00000004)
#define SMFMASK_EVENTLOG	(0x00000008)
#define SMFMASK_TRADEMON	(0x00000010)
#define SMFMASK_HEARTBEAT   (0x00000020)
#define SMFMASK_CUSTOM		(0xFFFFFFC0)
// Feed ids
#define SMFNO_LVL1		1
#define SMFNO_LVL2		2
#define SMFNO_ERRORLOG	3
#define SMFNO_EVENTLOG	4
#define SMFNO_TRADEMON	5
#define SMFNO_HEARTBEAT 6
#define SMFNO_CUSTOM	31

class SysFeedNotify
{
public:
	virtual void NotifyLog(const char *aclass, const char *aname, 
		const char *lpath, bool hist, __int64 off, int rectype, const char *buf, int blen, void *udata)=0;
	virtual void NotifyLvl1(const char *aclass, const char *aname, 
		WSPortType PortType, int PortNo, bool hist, int rectype, const char *buf, int blen, void *udata)=0;
	virtual void NotifyLvl2(const char *aclass, const char *aname, 
		WSPortType PortType, int PortNo, bool hist, int rectype, const char *buf, int blen, void *udata)=0;
	//virtual void NotifyOrder(const char *aclass, const char *aname, 
	//	const char *ordid, bool hist, int rectype, const char *buf, int blen, void *udata)=0;
	virtual void NotifyTrademon(const char *aclass, const char *aname, 
		bool hist, int rectype, const char *Domain, const char *TraderId, int NoUpdate, int ECN, int ECN_TYPE, int SeqNo, int ReqType,
		const __int64& foff, const char *buf, int blen, void *udata, const char *rptr, int rlen)=0;
    virtual void NotifyHeartbeat(const char *aclass, const char *aname, bool hist, int heartbeat,
        const char *message, int messageLength, void *udata)=0;
};
// Sysmon feed encoder/decoder for WsocksHost, SysmonProxy, and SysmonClient
class SysFeedCodec
{
public:
	SYSMONLINK SysFeedCodec();
	SYSMONLINK ~SysFeedCodec();
	SYSMONLINK int Init();
	SYSMONLINK int Shutdown();
	SYSMONLINK int EncodeLog(char *&mptr, int mlen, const char *aclass, const char *aname, 
		const char *lpath, bool hist, __int64 off, const char *buf, int blen);
	SYSMONLINK int EncodeLvl1(char *&mptr, int mlen, const char *aclass, const char *aname, 
		const char *heading, const char *status, WsocksApp *pmod, bool hist, HANDLE& ihnd, bool force);
	SYSMONLINK int EncodeLvl2(char *&mptr, int mlen, const char *aclass, const char *aname, 
		const char *heading, const char *status, WsocksApp *pmod, bool hist, HANDLE& ihnd, bool force);
	//SYSMONLINK int EncodeOrder(char *&mptr, int mlen, const char *aclass, const char *aname, 
	//	const char *ordid, bool hist, const char *buf, int blen);
	SYSMONLINK int EncodeTrademon(char *&mptr, int mlen, const char *aclass, const char *aname, 
		bool hist, const char *Domain, const char *TraderId, int NoUpdate, int ECN, int ECN_TYPE, int SeqNo, int ReqType,
		const __int64& foff, const char *buf, int blen);
    SYSMONLINK int EncodeHeartbeat(char *&mptr, int mlen, const char *aclass, const char *aname,
        bool hist, int heartbeat, const char *message, int messageLength);

	SYSMONLINK int Decode(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
	SYSMONLINK int DecodeLog(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
	SYSMONLINK int DecodeLvl1(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
	SYSMONLINK int DecodeLvl2(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
	//SYSMONLINK int DecodeOrder(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
	SYSMONLINK int DecodeTrademon(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
    SYSMONLINK int DecodeHeartbeat(const char *&mptr, int mlen, SysFeedNotify *pnotify, void *udata);
	SYSMONLINK int DecodeProxy(const char *&mptr, int mlen, int& fid, bool& hist, char *aclass, int clen, char *aname, int nlen);

protected:
	friend class SMProxy;
	void *ndata;

	int encode_int7(char *&fptr, int flen, __int64 ival);
	int decode_int7(__int64& ival, const char *&eptr, const char *eend, bool sgn=true);
	#if defined(BIT64)&&defined(WIN32)
	inline int decode_int7(SOCKET& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		return decode_int7((__int64&)ival,eptr,eend,sgn);
	}
	#endif
	inline int decode_int7(unsigned long& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(unsigned long)ival64;
		return 0;
	}
	inline int decode_int7(unsigned int& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(unsigned int)ival64;
		return 0;
	}
	inline int decode_int7(unsigned short& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(unsigned short)ival64;
		return 0;
	}
	inline int decode_int7(unsigned char& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(unsigned char)ival64;
		return 0;
	}
	inline int decode_int7(long& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(long)ival64;
		return 0;
	}
	inline int decode_int7(int& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(int)ival64;
		return 0;
	}
	inline int decode_int7(short& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(short)ival64;
		return 0;
	}
	inline int decode_int7(char& ival, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		ival=(char)ival64;
		return 0;
	}
	inline int decode_int7(bool& bval, const char *&eptr, const char *eend, bool sgn=true)
	{
		__int64 ival64=0;
		decode_int7(ival64,eptr,eend,sgn);
		bval=ival64?true:false;
		return 0;
	}
	int encode_pchar(char *&fptr, int flen, const char *sval, int slen);
	int decode_pchar(char *sval, int slen, const char *&eptr, const char *eend);
	int encode_binary(char *&fptr, int flen, const void *buf, int blen);
	int decode_binary(char *buf, int blen, const char *&eptr, const char *eend);

	SYSMONLINK int EncodeLvl1(char *&mptr, int mlen, const char *aclass, const char *aname, 
		WsocksApp *pmod, WSPortType PortType, int PortNo, bool hist, const char *buf, int blen, bool force);
	SYSMONLINK int EncodeLvl2(char *&mptr, int mlen, const char *aclass, const char *aname, 
		WsocksApp *pmod, WSPortType PortType, int PortNo, bool hist, const char *buf, int blen, bool force);
};

typedef struct tdMsg1112Rec
{
	char bindIP[20];
	u_short bindPort;
	unsigned char uuid[16];		// Added 20111028
} MSG1112REC; // 38 bytes

#pragma pack(pop)

#endif//SYSMON_H
