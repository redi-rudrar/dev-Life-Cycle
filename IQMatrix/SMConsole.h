#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#ifdef DIALOG_DLLS
#include "smdlgapi.h"
#endif

//typedef list<string> APPHELP;
// Fully-described application help
struct PARMHELP
{
	string parm;
	string defval;
	bool opt;
	bool sel;
	string desc;
};
typedef list<PARMHELP*> PARMLIST;
struct SAMPLE
{
	int no;
	int type;
	string ex;
	string desc;
};
typedef list<SAMPLE*> SAMPLIST;
struct CMDHELP
{
	struct APPHELP *ahelp;
	string cmd;
	string desc;
	PARMLIST parmlist;
	SAMPLIST samplist;
	bool common;
};
typedef map<string,CMDHELP*> CMDMAP;
struct APPHELP
{
	CMDMAP cmdmap;
};
typedef map<string,APPHELP*> HELPMAP;

// SMConsole dialog
class SMConsoleDlg : public CDialog
	#ifdef DIALOG_DLLS
	,public SMDlgServerNotify
	#endif
{
	DECLARE_DYNAMIC(SMConsoleDlg)

public:
	SMConsoleDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~SMConsoleDlg();
	int WSHLoggedError(const char *estr);
	int WSHLoggedEvent(const char *estr);
	int NotifyApp(const char *apath);
	int NotifyHelp(TVMAP& tvmap, string cmd, char *parms);

	#ifdef DIALOG_DLLS
	// SMDlgServerNotify
	int SMDSSendCmd(const char *mptr, int mlen);
	void SMDSLogEvent(const char *fmt, ...);
	void SMDSLogError(const char *fmt, ...);
	#endif

// Dialog Data
	enum { IDD = IDD_SMCONSOLE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//char *tbuf;
	//int tsize;
	int AppendString(const char *astr);
	int SaveConsole(const char *spath);

	HELPMAP helpMap;
	int LoadHelp(const char *apath, APPHELP *phelp);
    void ClearHelp();
	int BuildCmd();

	HELPMAP csamples;
	int FindCommonSamples(char *fpath);

	DECLARE_MESSAGE_MAP()
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	CEdit m_edit;
	CComboBox m_apath;
	class CIQMatrixApp *phost;
	DWORD uitid;
	CFont *pfont,*ffont;
	class SMConsole *smc;
	//int editRatio;
	#ifdef DIALOG_DLLS
	SMDlgApi dlgapi;
	#endif

public:
	CComboBox m_cmd;
	afx_msg void OnBnClickedSend();
	CStatic m_cmdlabel;
	CButton m_send;
	afx_msg void OnCbnSelchangeApath();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClose();
	afx_msg void OnBnClickedFont();
	CButton m_font;
	CListCtrl m_parms;
	CListCtrl m_samples;
	afx_msg void OnNMDblclkSamples(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkParms(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditParms(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickParms(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBeginlabeleditParms(NMHDR *pNMHDR, LRESULT *pResult);
	CComboBox m_cmdlist;
	afx_msg void OnCbnSelchangeCmdlist();
	afx_msg void OnNMRclickSamples(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEditCommonSamples();
	afx_msg void OnReloadCommonSamples();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	CStatic m_parmhelp;
};

// SMConsole App
class SMConsole 
	:public WsocksApp
	,public SysCmdNotify
	,public SysFeedNotify
{
public:
	SMConsole();
    ~SMConsole();

	// WsocksApp
	int WSHInitModule(AppConfig *config, class WsocksHost *apphost);
	int WSHCleanupModule();

	int WSPortsCfg();
	void WSTimeChange();

    void WSConOpened(int PortNo);
    int WSConMsgReady(int PortNo);
    int WSConStripMsg(int PortNo);
    int WSBeforeConSend(int PortNo);
    void WSConClosed(int PortNo);

	// SysCmdNotify
	void NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata);
	void NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata);

	// SysFeedNotify
	 void NotifyLog(const char *aclass, const char *aname, 
		const char *lpath, bool hist, __int64 off, int rectype, const char *buf, int blen, void *udata);
	 void NotifyLvl1(const char *aclass, const char *aname, 
		WSPortType PortType, int PortNo, bool hist, int rectype, const char *buf, int blen, void *udata);
	 void NotifyLvl2(const char *aclass, const char *aname, 
		WSPortType PortType, int PortNo, bool hist, int rectype, const char *buf, int blen, void *udata);
	 //void NotifyOrder(const char *aclass, const char *aname, 
	 //	const char *ordid, bool hist, int rectype, const char *buf, int blen, void *udata);
	void NotifyTrademon(const char *aclass, const char *aname, 
		bool hist, int rectype, const char *Domain, const char *TraderId, int NoUpdate, int ECN, int ECN_TYPE, int SeqNo, int ReqType,
		const __int64& foff, const char *buf, int blen, void *udata, const char *rptr, int rlen);
    void NotifyHeartbeat(const char *aclass, const char *aname,
        bool hist, int heartbeat, const char *message, int messageLength, void *udata);

protected:
	friend class SMConsoleDlg;
	int UmcPort;
	class SysFeedCodec *fcodec;
	class SysCmdCodec *ccodec;
	int rid;
	SMConsoleDlg *pdlg;

	int TranslateConMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo);

#ifndef DIALOG_DLLS
	// Built-in iqserver dialogs
	string lvlxAppPath;
	struct _CSPMDR *lvl1Result;
	struct _CSPADDREC *lvl1AddResult;
	list<struct _CSPLVLXREC2 *> lvl2Results;
	HANDLE lvl1Event,lvl2Event;
	int Lvl1Dlg(const char *apath);
	int Lvl2Dlg(const char *apath);
	int MontageDlg(const char *apath);
	int Lvl1Answer(int rc, const char *fpath);
	int Lvl2Answer(int rc, const char *fpath);
	int MontageAnswer(int rc, const char *fpath);

public:
	long _FindCspMdrRec(char *pSymbol, BYTE cExchange, BYTE cRegion );
	long FindFirstCspLvlXRec2(struct _CSPMDR*,struct _CSPLVLXREC2**,BOOL,BOOL);
	long FindNextCspLvlXRec2(struct _CSPLVLXREC2**,long);
	void FindCloseCspLvlXRec();
	void ReadCspMdrRec(struct _CSPMDR *pMdr, long lRec );
	long GetCspAddRec(long lRec,_CSPADDREC *pAddRec);
#endif
};

extern "C" WsocksApp* __stdcall SMConsole_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname);
extern "C" void __stdcall SMConsole_FreeAppInterface(WsocksApp *pmod);
