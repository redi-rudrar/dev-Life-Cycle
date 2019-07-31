#pragma once
#include "afxcmn.h"
#include "afxdtctl.h"


// CTraceOrder dialog

class CTraceOrder : public CDialogEx
{
	DECLARE_DYNAMIC(CTraceOrder)

public:
	CTraceOrder(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTraceOrder();
	list<string> slist;
	string idtype;
	string side;

// Dialog Data
	enum { IDD = IDD_TRACE_ORDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_system;
	CString m_orderid;
	CComboBox m_idtype;
	CString m_symbol;
	CComboBox m_side;
	double m_price;
	int m_shares;
	int m_date;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	CDateTimeCtrl m_dateCtrl;
};
