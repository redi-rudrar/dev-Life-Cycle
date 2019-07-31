#pragma once
#include "afxwin.h"
#include <string>
#include <map>
using namespace std;

// CLoginDlg dialog
struct CRED
{
	string dest;
	string user;
	string pass;
	int lastDate;
	int lastTime;
	string tunnel;
	string gateway;
};
class CREDMAP :public map<string,CRED>
{
public:
	static int GetDestAlias(const char *cdest, string& dest, string& alias);
	int Save();
	int Load();
};

class CLoginDlg : public CDialog
{
	DECLARE_DYNAMIC(CLoginDlg)

public:
	CLoginDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLoginDlg();

	static CREDMAP credmap;
	BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_LOGIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_user;
	CString m_pass;
	CString m_server;
	CString m_selalias;
	CComboBox m_ipaddr;
	CString m_tunnel;
	CString m_gateway;
	afx_msg void OnBnClickedOk();
//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnCbnSelchangeIpaddr();
	afx_msg void OnCbnEditchangeIpaddr();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnEnChangeTunnel2();
};
