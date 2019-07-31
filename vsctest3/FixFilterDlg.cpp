// FixFilterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vsctest3.h"
#include "FixFilterDlg.h"
#include "afxdialogex.h"


// CFixFilterDlg dialog

IMPLEMENT_DYNAMIC(CFixFilterDlg, CDialogEx)

CFixFilterDlg::CFixFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFixFilterDlg::IDD, pParent)
	, m_text(_T(""))
	, m_alltags(0)
{

}

CFixFilterDlg::~CFixFilterDlg()
{
}

void CFixFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TEXT, m_text);
	DDX_Check(pDX, IDC_ALLTAGS, m_alltags);
}


BEGIN_MESSAGE_MAP(CFixFilterDlg, CDialogEx)
	ON_BN_CLICKED(IDC_ALLTAGS, &CFixFilterDlg::OnBnClickedAlltags)
END_MESSAGE_MAP()


// CFixFilterDlg message handlers


BOOL CFixFilterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	UpdateData(false);
	OnBnClickedAlltags();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CFixFilterDlg::OnBnClickedAlltags()
{
	// TODO: Add your control notification handler code here
	UpdateData(true);
	GetDlgItem(IDC_TEXT)->EnableWindow(m_alltags!=BST_CHECKED);
}
