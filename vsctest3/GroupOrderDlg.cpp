// GroupOrderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vsctest3.h"
#include "GroupOrderDlg.h"
#include "afxdialogex.h"
#include "vsctest3Doc.h"
#include "vsctest3View.h"

// CGroupOrderDlg dialog
enum GroupCols
{
	GCOL_GROUP,
	GCOL_ENABLED,
	GCOL_COUNT
};

IMPLEMENT_DYNAMIC(CGroupOrderDlg, CDialogEx)

CGroupOrderDlg::CGroupOrderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGroupOrderDlg::IDD, pParent)
	,psortlist()
{

}

CGroupOrderDlg::~CGroupOrderDlg()
{
}

void CGroupOrderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_list);
}


BEGIN_MESSAGE_MAP(CGroupOrderDlg, CDialogEx)
	ON_BN_CLICKED(IDC_UP, &CGroupOrderDlg::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_DOWN, &CGroupOrderDlg::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDOK, &CGroupOrderDlg::OnBnClickedOk)
	ON_NOTIFY(NM_RCLICK, IDC_LIST, &CGroupOrderDlg::OnNMRClick)
END_MESSAGE_MAP()


// CGroupOrderDlg message handlers
BOOL CGroupOrderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add extra initialization here
	m_list.ModifyStyle(0,LVS_REPORT);
	m_list.ModifyStyle(0,LVS_SHOWSELALWAYS);
	DWORD estyle=m_list.GetExtendedStyle();
	estyle&=~LVS_EX_TRACKSELECT;
	estyle|=LVS_EX_FULLROWSELECT;
	m_list.SetExtendedStyle(estyle);

	LVCOLUMN gcols[]={
		{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,150,"Group",0,GCOL_GROUP,0,0},
		{LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM,0,40,"On",0,GCOL_ENABLED,0,0},
		{0,0,0,0,0},
	};
	for(int i=0;gcols[i].mask;i++)
		m_list.InsertColumn(i,&gcols[i]);

	for(list<WORD>::iterator sit=psortlist.begin();sit!=psortlist.end();sit++)
	{
		WORD psortcol=*sit;
		const char *kstr="";
		switch(HIBYTE(psortcol))
		{
		case OCOL_APPINSTID: kstr="AppInstID"; break;
		case OCOL_CLORDID: kstr="ClOrdID"; break;
		case OCOL_ROOTORDERID: kstr="RootOrderID"; break;
		case OCOL_FIRSTCLORDID: kstr="FirstClOrdID"; break;
		case OCOL_SYMBOL: kstr="Symbol"; break;
		case OCOL_ACCOUNT: kstr="Account"; break;
		case OCOL_ECNORDERID: kstr="EcnOrderID"; break;
		case OCOL_CLIENTID: kstr="ClientID"; break;
		case OCOL_PRICE: kstr="Price"; break;
		case OCOL_SIDE: kstr="Side"; break;
		case OCOL_ORDERQTY: kstr="OrderQty"; break;
		case OCOL_CUMQTY: kstr="CumQty"; break;
		case OCOL_FILLQTY: kstr="FillQty"; break;
		case OCOL_TERM: kstr="Term"; break;
		case OCOL_HIGHMSGTYPE: kstr="HighMsgType"; break;
		case OCOL_HIGHEXECTYPE: kstr="HighExecType"; break;
		case OCOL_TRANSACTTIME: kstr="TransactTime (Minute)"; break;
		case OCOL_ORDERLOC: kstr="OrderLoc"; break;
		case OCOL_CONNECTION: kstr="Connection"; break;
		case OCOL_CLPARENTORDERID: kstr="ClParentOrderID"; break;
		case OCOL_ORDERDATE: kstr="OrderDate"; break;
		//case OCOL_ROUTEDORDERID: kstr="RoutedOrderID"; break;
		case OCOL_ROUTINGBROKER: kstr="RoutingBroker"; break;
		case OCOL_SECURITYTYPE: kstr="SecurityType"; break;
		};
		LV_ITEM lvi;
		memset(&lvi,0,sizeof(LV_ITEM));
		lvi.iItem=m_list.GetItemCount();
		int nidx=m_list.InsertItem(&lvi);
		if(nidx>=0)
		{
			m_list.SetItemText(nidx,GCOL_GROUP,kstr);
			m_list.SetItemText(nidx,GCOL_ENABLED,LOBYTE(psortcol)?"Y":"N");
			m_list.SetItemData(nidx,psortcol);
		}
	}
	UpdateData(false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGroupOrderDlg::OnBnClickedButtonUp()
{
	// Add your control notification handler code here
	list<int> dlist;
	list<pair<string,WORD>> slist;
	int fpos=-1;
	POSITION pos=m_list.GetFirstSelectedItemPosition();
	while(pos)
	{
		int sidx=m_list.GetNextSelectedItem(pos);
		if(sidx>=0)
			dlist.push_back(sidx);
	}
	for(list<int>::reverse_iterator dit=dlist.rbegin();dit!=dlist.rend();dit++)
	{
		int sidx=*dit;
		fpos=sidx -1;
		if(fpos<0) fpos=0;
		pair<string,WORD> sitem;
		sitem.first=m_list.GetItemText(sidx,GCOL_GROUP);
		sitem.second=(WORD)m_list.GetItemData(sidx);
		slist.push_front(sitem);
		m_list.DeleteItem(sidx);
	}
	for(list<pair<string,WORD>>::iterator sit=slist.begin();sit!=slist.end();sit++)
	{
		pair<string,WORD>& sitem=*sit;
		LV_ITEM lvi;
		memset(&lvi,0,sizeof(LV_ITEM));
		lvi.iItem=fpos++;
		int nidx=m_list.InsertItem(&lvi);
		if(nidx>=0)
		{
			m_list.SetItemText(nidx,GCOL_GROUP,sitem.first.c_str());
			m_list.SetItemText(nidx,GCOL_ENABLED,LOBYTE(sitem.second)?"Y":"N");
			m_list.SetItemData(nidx,sitem.second);
			m_list.SetItemState(nidx,LVIS_SELECTED,LVIS_SELECTED);
		}
	}
}
void CGroupOrderDlg::OnBnClickedButtonDown()
{
	// Add your control notification handler code here
	list<int> dlist;
	list<pair<string,WORD>> slist;
	int fpos=-1;
	POSITION pos=m_list.GetFirstSelectedItemPosition();
	while(pos)
	{
		int sidx=m_list.GetNextSelectedItem(pos);
		if(sidx>=0)
			dlist.push_back(sidx);
	}
	for(list<int>::reverse_iterator dit=dlist.rbegin();dit!=dlist.rend();dit++)
	{
		int sidx=*dit;
		fpos=sidx +1;
		if(fpos>m_list.GetItemCount()) 
			fpos=m_list.GetItemCount();
		pair<string,WORD> sitem;
		sitem.first=m_list.GetItemText(sidx,GCOL_GROUP);
		sitem.second=(WORD)m_list.GetItemData(sidx);
		slist.push_front(sitem);
		m_list.DeleteItem(sidx);
	}
	for(list<pair<string,WORD>>::iterator sit=slist.begin();sit!=slist.end();sit++)
	{
		pair<string,WORD>& sitem=*sit;
		LV_ITEM lvi;
		memset(&lvi,0,sizeof(LV_ITEM));
		lvi.iItem=fpos++;
		int nidx=m_list.InsertItem(&lvi);
		if(nidx>=0)
		{
			m_list.SetItemText(nidx,GCOL_GROUP,sitem.first.c_str());
			m_list.SetItemText(nidx,GCOL_ENABLED,LOBYTE(sitem.second)?"Y":"N");
			m_list.SetItemData(nidx,sitem.second);
			m_list.SetItemState(nidx,LVIS_SELECTED,LVIS_SELECTED);
		}
	}
}
void CGroupOrderDlg::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	POSITION pos=m_list.GetFirstSelectedItemPosition();
	while(pos)
	{
		int sidx=m_list.GetNextSelectedItem(pos);
		if(sidx>=0)
		{
			WORD psortcol=(WORD)m_list.GetItemData(sidx);
			psortcol=MAKEWORD(LOBYTE(psortcol)?0:1,HIBYTE(psortcol));
			m_list.SetItemText(sidx,GCOL_ENABLED,LOBYTE(psortcol)?"Y":"N");
			m_list.SetItemData(sidx,psortcol);
		}
	}
}

void CGroupOrderDlg::OnBnClickedOk()
{
	// Add your control notification handler code here
	psortlist.clear();
	for(int i=0;i<m_list.GetItemCount();i++)
		psortlist.push_back((WORD)m_list.GetItemData(i));
	CDialogEx::OnOK();
}
