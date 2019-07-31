#pragma once


// CFixFilterDlg dialog

class CFixFilterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFixFilterDlg)

public:
	CFixFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFixFilterDlg();

// Dialog Data
	enum { IDD = IDD_FIXFILTER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_text;
	virtual BOOL OnInitDialog();
	int m_alltags;
	afx_msg void OnBnClickedAlltags();
};
