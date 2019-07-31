#pragma once
#include "afxcmn.h"


// CGroupOrderDlg dialog

class CGroupOrderDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGroupOrderDlg)

public:
	CGroupOrderDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGroupOrderDlg();

	list<WORD> psortlist;

// Dialog Data
	enum { IDD = IDD_GROUP_ORDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list;
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
};
