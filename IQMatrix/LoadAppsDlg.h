#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// LoadAppsDlg dialog

class LoadAppsDlg : public CDialog
{
	DECLARE_DYNAMIC(LoadAppsDlg)

public:
	LoadAppsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~LoadAppsDlg();
	int UpdateApp(WsocksApp *pApp);
	int UpdateItem(int lidx, AppConfig *pcfg);
	int sortcol;

// Dialog Data
	enum { IDD = IDD_LOADAPPS };
	APPCONFIGLIST alist;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_appList;
	afx_msg void OnBnClickedEdit();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnBnClickedLoad();
	afx_msg void OnBnClickedUnload();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHdnItemclickApplist(NMHDR *pNMHDR, LRESULT *pResult);
};
