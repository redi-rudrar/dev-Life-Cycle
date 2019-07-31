// BinaryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vsctest3.h"
#include "BinaryDlg.h"
#include "afxdialogex.h"


// CBinaryDlg dialog

IMPLEMENT_DYNAMIC(CBinaryDlg, CDialogEx)

CBinaryDlg::CBinaryDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBinaryDlg::IDD, pParent)
	,heading("Binary")
	,qbuf(0)
	,qlen(0)
	,relativeOffsets(true)
{
}

CBinaryDlg::~CBinaryDlg()
{
}

void CBinaryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT, m_editCtrl);
}


BEGIN_MESSAGE_MAP(CBinaryDlg, CDialogEx)
	ON_WM_SIZE()
	ON_MESSAGE(WM_USER +5, &CBinaryDlg::OnWmUser5)
END_MESSAGE_MAP()


// CBinaryDlg message handlers


BOOL CBinaryDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	static CFont *courier = 0;
	if ( !courier )
	{
		courier = new CFont;
		courier->CreatePointFont(100, "Courier New");
	}
	m_editCtrl.SetFont(courier);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
LRESULT CBinaryDlg::OnWmUser5(WPARAM wParam, LPARAM lParam)
{
	char wbuf[8096];
	memset(wbuf, 0, sizeof(wbuf));
	const char *wend = wbuf +sizeof(wbuf);
	char *wptr = wbuf;

	DWORD base = (DWORD)qbuf;
	//sprintf(wptr, "%s\r\n", heading); wptr += strlen(wptr);
	SetWindowText(heading);
	if(relativeOffsets)
		sprintf(wptr, "[%08X, %08X)\r\n", 0, qlen);
	else
		sprintf(wptr, "[%08X, %08X)\r\n", base, base +qlen); 
	wptr += strlen(wptr);
	int width = 16;
    char qsbuf[67];
    for ( DWORD i=0; i<qlen; i+=width )
    {
        memset(qsbuf, 0, sizeof(qsbuf));
        char *qhptr = qsbuf;
        char *qtptr = qsbuf +50;
        if ( width < 9 )
            qtptr -= 25;
        for ( int j=0; j<width; j++ )
        {
            if ( i +j >= qlen )
            {
                strcpy(qhptr, "   "); qhptr += 3;
                if ( j == 7 )
                {
                    *qhptr = ' ';
                    qhptr ++;
                }
                *qtptr = ' '; qtptr ++;
                continue;
            }
            unsigned char ch = qbuf[i +j];
            sprintf(qhptr, "%02X ", ch); qhptr += 3;
            if ( j == 7 )
            {
                *qhptr = ' ';
                qhptr ++;
            }
            if ( isprint(ch) )
                *qtptr = ch;
            else
                *qtptr = '.';
            qtptr ++;
        }
        for ( int j=0; j<66; j++ )
        {
            if ( !qsbuf[j] )
                qsbuf[j] = ' ';
        }
		if ( wptr +67 >= wend )
			break;
		if(relativeOffsets)
			sprintf(wptr, "%08X   %s\r\n", i, qsbuf); 
		else
			sprintf(wptr, "%08X   %s\r\n", base +i, qsbuf); 
		wptr += strlen(wptr);
	}

	//m_edit = wbuf;
	m_editCtrl.SetWindowText(wbuf);
	UpdateData(false);
	return TRUE;
}

void CBinaryDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if ( IsWindow(m_editCtrl.m_hWnd) )
	{
		RECT erect;
		m_editCtrl.GetWindowRect(&erect);
		ScreenToClient(&erect);
		m_editCtrl.MoveWindow(erect.left, erect.top, cx -erect.left *2, cy -erect.top *2);	
	}
}
