// TraceOrder.cpp : implementation file
//

#include "stdafx.h"
#include "vsctest3.h"
#include "TraceOrder.h"
#include "afxdialogex.h"


// CTraceOrder dialog

IMPLEMENT_DYNAMIC(CTraceOrder, CDialogEx)

CTraceOrder::CTraceOrder(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTraceOrder::IDD, pParent)
	, m_orderid(_T(""))
	, m_symbol(_T(""))
	, m_price(0)
	, m_shares(0)
	, m_date(0)
{

}

CTraceOrder::~CTraceOrder()
{
}

void CTraceOrder::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSTEM, m_system);
	DDX_Text(pDX, IDC_CLORDID, m_orderid);
	DDX_Control(pDX, IDC_IDTYPE, m_idtype);
	DDX_Text(pDX, IDC_SYMBOL, m_symbol);
	DDX_Control(pDX, IDC_SIDE, m_side);
	DDX_Text(pDX, IDC_PRICE, m_price);
	DDX_Text(pDX, IDC_SHARES, m_shares);
	DDX_Control(pDX, IDC_DATETIMEPICKER1, m_dateCtrl);
}


BEGIN_MESSAGE_MAP(CTraceOrder, CDialogEx)
	ON_BN_CLICKED(IDOK, &CTraceOrder::OnBnClickedOk)
END_MESSAGE_MAP()


// CTraceOrder message handlers


BOOL CTraceOrder::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add extra initialization here
	UpdateData(true);
	m_system.ResetContent();
	if(!slist.empty())
	{
		for(list<string>::iterator sit=slist.begin();sit!=slist.end();sit++)
			m_system.AddString(sit->c_str());
		CString first;
		m_system.GetLBText(0,first);
		m_system.SelectString(-1,first);
	}

	m_idtype.ResetContent();
	m_idtype.AddString("ClOrdID");
	m_idtype.AddString("RootOrderID");
	m_idtype.AddString("FirstClOrdID");
	m_idtype.AddString("ClParentOrderID");
	//m_idtype.AddString("Symbol");
	m_idtype.AddString("Account");
	m_idtype.AddString("EcnOrderID");
	m_idtype.AddString("ClientID");
	m_idtype.AddString("TransactTime");
	//m_idtype.AddString("Filled Orders");
	//m_idtype.AddString("Open Orders");
	m_idtype.AddString("Connection");
	//m_idtype.AddString("RoutedOrderID");
	m_idtype.AddString("RoutingBroker");
	m_idtype.AddString("OrderDate");
	m_idtype.SelectString(-1,idtype.c_str());

	m_side.ResetContent();
	m_side.AddString("ANY");
	m_side.AddString("B");
	m_side.AddString("SL");
	m_side.AddString("SS");
	m_side.AddString("SX");
	m_side.SelectString(-1,side.c_str());

	if(m_date)
	{
		SYSTEMTIME tsys;
		GetSystemTime(&tsys);
		tsys.wYear=m_date/10000;
		tsys.wMonth=(m_date%10000)/100;
		tsys.wDay=m_date%100;
		m_dateCtrl.SetTime(&tsys);
	}

	UpdateData(false);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CTraceOrder::OnBnClickedOk()
{
	// Add your control notification handler code here
	UpdateData(true);
	if((m_orderid.IsEmpty())&&(m_symbol.IsEmpty()))
	{
		MessageBox("Either Key or Symbol must be supplied.","Trace",MB_ICONWARNING);
		return;
	}

	slist.clear();
	CString selname;
	m_system.GetLBText(m_system.GetCurSel(),selname);
	if(selname=="ALL")
	{
		for(int i=0;i<m_system.GetCount();i++)
		{
			m_system.GetLBText(i,selname);
			if(selname!="ALL")
				slist.push_back((const char *)selname);
		}
	}
	else
		slist.push_back((const char *)selname);

	m_idtype.GetLBText(m_idtype.GetCurSel(),selname);
	idtype=selname;

	m_side.GetLBText(m_side.GetCurSel(),selname);
	side=selname;

	SYSTEMTIME tsys;
	m_dateCtrl.GetTime(&tsys);
	m_date=(tsys.wYear*10000) +(tsys.wMonth*100) +(tsys.wDay);

	CDialogEx::OnOK();
}
