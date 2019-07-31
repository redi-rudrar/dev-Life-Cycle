#pragma once
#include "afxwin.h"


// CBinaryDlg dialog

class CBinaryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBinaryDlg)

public:
	CBinaryDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBinaryDlg();

	CString heading;
	const char *qbuf;
	DWORD qlen;
	bool relativeOffsets;

// Dialog Data
	enum { IDD = IDD_BINARY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	LRESULT OnWmUser5(WPARAM wParam, LPARAM lParam);
	CEdit m_editCtrl;
};
