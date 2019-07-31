
#ifndef _SMDLGAPI_H
#define _SMDLGAPI_H

#include <string>
#include <map>
using namespace std;

#define SMDLGAPI_DATE	20120725

// Provider DLLs must export this function
#define SMDLGAPI_INTERFACE_VERSION 0x0100
#if defined(WIN32)||defined(_CONSOLE)
#ifdef BIT64
#define SMDLGAPI_EXPORT_FUNC "?GetDlgInterface@@YAPEAVSMDlgProvider@@PEAUHINSTANCE__@@GPEAVSMDlgApi@@@Z" // 64-bit signature
#else
#define SMDLGAPI_EXPORT_FUNC "?GetDlgInterface@@YGPAVSMDlgProvider@@PAUHINSTANCE__@@GPAVSMDlgApi@@@Z" // 32-bit signature
#endif
typedef class SMDlgProvider *(__stdcall *GETDLGINTERFACE)(HMODULE dllhnd, WORD version, class SMDlgApi *dlgapi);
typedef void (__stdcall *FREEDLGINTERFACE)(SMDlgProvider *provider);
#else//!WIN32
#define WSHOST_EXPORT_FUNC "GetDlgInterface"
typedef void *(__stdcall *GETDLGINTERFACE)(HMODULE dllhnd, WORD version, class SMDlgApi *dlgapi);
typedef void (__stdcall *FREEDLGINTERFACE)(void *pmod);
#endif//!WIN32

// Dialog providers must implement this interface
class SMDlgServerNotify;
class SMDlgProvider
{
public:
	SMDlgProvider();
	virtual int SMDPValidateModule(const char *challenge);
	// Sysmon console protocol
	virtual bool SMDPIsProvider(string aclass, string aname, string cmd)=0;
	virtual int SMDPOpenDialog(SMDlgServerNotify *dsnotify, string oboi, string apath, string cmd)=0;
	virtual int SMDPDisconnected(SMDlgServerNotify *dsnotify)=0;
	virtual int SMDPNotifyResponse(SMDlgServerNotify *dsnotify, string apath, string cmd, const char *resp, int rlen)=0;
	virtual int SMDPNotifyFile(SMDlgServerNotify *dsnotify, string apath, int rc, string fkey, const char *fpath)=0;
	// IQ messaging protocol
	virtual bool SMDPIsProvider(string aclass, string aname, int msgid)=0;
	virtual int SMDPOpenDialog(SMDlgServerNotify *dsnotify, string aclass, string aname, int msgid)=0;
	virtual int SMDPNotifyMsg(SMDlgServerNotify *dsnotify, string aclass, string aname, struct tdMsgHeader *MsgHeader, char *Msg)=0;

public:
	GETDLGINTERFACE pGetDlgInterface;
	FREEDLGINTERFACE pFreeDlgInterface;
	string name;
	class SMDlgApi *dlgapi;
	string fpath;
	HMODULE dllhnd;
};


// Dialog subscribers must implement this interface
class SMDlgServerNotify
{
public:
	// Sysmon console protocol
	virtual int SMDSSendCmd(const char *mbuf, int mlen)=0;
	virtual void SMDSLogEvent(const char *fmt, ...)=0;
	virtual void SMDSLogError(const char *fmt, ...)=0;
	// IQ messaging protocol
	virtual int SMDSConnect(int ConPortNo, const char *ip, int port, const char *compresstype, const char *gateway, const char *socks5)=0;
	virtual int SMDSDisconnect(int ConPortNo)=0;
	virtual int SMDSSendMsg(int msgid, int mlen, const char *mbuf)=0;
};

// Dialog subscribers need an instance of this class
class SMDlgApi
{
public:
	SMDlgApi();

	// For servers wanting dialog services
	int InitDlgServer(string oboi, SMDlgServerNotify *dsnotify, class SysCmdCodec *ccodec, const char *pdir);
	int ShutdownDlgServer();
	int FindDlgProvider(string aclass, string aname, string apath, string cmd);
	int NotifyResponse(string apath, string cmd, const char *resp, int rlen);
	int NotifyFile(string apath, int rc, string fkey, const char *fpath);
	int NotifyDisconnected(string apath, string cmd);

	int FindDlgProvider(string aclass, string aname, int msgid);
	int NotifyMsg(string apath, struct tdMsgHeader *MsgHeader, char *Msg);
	int NotifyDisconnected(string aclass, string aname, int msgid);

public:
	SMDlgServerNotify *dsnotify;
	class SysCmdCodec *ccodec;
	string oboi;

protected:
	typedef map<string,SMDlgProvider*> PROVDERMAP;
	PROVDERMAP pmap;
	int LoadDlgProviders(const char *pdir);
	int FreeDlgProvider(SMDlgProvider*);
	int ValidateApp(SMDlgProvider *provider, char reason[256]);
};

#endif//_SMDLGAPI_H
