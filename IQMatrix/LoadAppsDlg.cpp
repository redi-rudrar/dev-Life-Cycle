// LoadAppsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "IQMatrix.h"
#include "LoadAppsDlg.h"
#include ".\loadappsdlg.h"


// LoadAppsDlg dialog

IMPLEMENT_DYNAMIC(LoadAppsDlg, CDialog)
LoadAppsDlg::LoadAppsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LoadAppsDlg::IDD, pParent)
{
}

LoadAppsDlg::~LoadAppsDlg()
{
}

void LoadAppsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_APPLIST, m_appList);
}


BEGIN_MESSAGE_MAP(LoadAppsDlg, CDialog)
	ON_BN_CLICKED(IDC_EDIT, OnBnClickedEdit)
	ON_BN_CLICKED(IDC_REFRESH, OnBnClickedRefresh)
	ON_BN_CLICKED(IDC_LOAD, OnBnClickedLoad)
	ON_BN_CLICKED(IDC_UNLOAD, OnBnClickedUnload)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickApplist)
END_MESSAGE_MAP()

// LoadAppsDlg message handlers
void LoadAppsDlg::OnBnClickedEdit()
{
	ShellExecute(m_hWnd,"open","notepad.exe","setup\\app.ini",0,SW_SHOW);
}

enum AppCols
{
	ACOL_LINE,
	ACOL_VER,
	ACOL_CLASS,
	ACOL_NAME,
	ACOL_ENABLED,
	ACOL_LOADED,
	ACOL_THREADS,
	ACOL_DLLPATH,
	ACOL_CONFIGPATH,
	ACOL_RUNPATH,
	ACOL_RUNCMD,
	ACOL_LOGPATH,
	ACOL_LOOPBACK,
	ACOL_ASYNC,
	ACOL_COUNT
};
int LoadAppsDlg::UpdateItem(int lidx, AppConfig *pcfg)
{
	CString vstr;
	vstr.Format("%d",pcfg->line);
	m_appList.SetItemText(lidx,ACOL_LINE,vstr);
	vstr.Format("%d.%d",HIBYTE(pcfg->ver),LOBYTE(pcfg->ver));
	m_appList.SetItemText(lidx,ACOL_VER,vstr);
	m_appList.SetItemText(lidx,ACOL_CLASS,pcfg->aclass.c_str());
	m_appList.SetItemText(lidx,ACOL_NAME,pcfg->aname.c_str());
	m_appList.SetItemText(lidx,ACOL_ASYNC,pcfg->asyncMode?"Y":"N");
	m_appList.SetItemText(lidx,ACOL_DLLPATH,pcfg->DllPath.c_str());
	m_appList.SetItemText(lidx,ACOL_CONFIGPATH,pcfg->ConfigPath.c_str());
	m_appList.SetItemText(lidx,ACOL_RUNPATH,pcfg->RunPath.c_str());
	m_appList.SetItemText(lidx,ACOL_RUNCMD,pcfg->RunCmd.c_str());
	m_appList.SetItemText(lidx,ACOL_LOGPATH,pcfg->LogPath.c_str());
	m_appList.SetItemText(lidx,ACOL_ENABLED,pcfg->Enabled?"Y":"N");
	m_appList.SetItemText(lidx,ACOL_THREADS,pcfg->TargetThreads.c_str());
	m_appList.SetItemText(lidx,ACOL_LOOPBACK,pcfg->wshLoopback?"Y":"N");
	WsocksApp *pmod=theApp.WSHGetApp(pcfg->aname);
	m_appList.SetItemText(lidx,ACOL_LOADED,pmod?"Y":"N");
	m_appList.SetItemData(lidx,(DWORD)(PTRCAST)pcfg);
	return 0;
}
void LoadAppsDlg::OnBnClickedRefresh()
{
	m_appList.DeleteAllItems();
	theApp.WSHLoadAppIni(0,&alist);
	for(APPCONFIGLIST::iterator ait=alist.begin();ait!=alist.end();ait++)
	{
		AppConfig *ncfg=*ait;
		LVITEM lvi;
		memset(&lvi,0,sizeof(LVITEM));
		lvi.iItem=m_appList.GetItemCount();
		int nidx=m_appList.InsertItem(&lvi);
		if(nidx>=0)
			UpdateItem(nidx,ncfg);
	}
}
int LoadAppsDlg::UpdateApp(WsocksApp *pApp)
{
	if(!IsWindow(m_appList))
		return -1;
	for(int i=0;i<m_appList.GetItemCount();i++)
	{
		AppConfig *ncfg=(AppConfig *)m_appList.GetItemData(i);
		if((ncfg)&&(ncfg->aname==pApp->pcfg->aname))
		{
			UpdateItem(i,pApp->pcfg);
			break;
		}
	}
	return 0;
}

void LoadAppsDlg::OnBnClickedLoad()
{
	POSITION pos=m_appList.GetFirstSelectedItemPosition();
	while(pos)
	{
		int sidx=m_appList.GetNextSelectedItem(pos);
		if(sidx>=0)
		{
			AppConfig *ncfg=(AppConfig *)m_appList.GetItemData(sidx);
			if(ncfg)
			{
				WsocksApp *pmod=theApp.WSHGetApp(ncfg->aname);
				if(pmod)
				{
					CString wmsg;
					wmsg.Format("App(%s) is already running. Reload?",ncfg->aname.c_str());
					if(MessageBox(wmsg,"Confirm Reload",MB_ICONWARNING|MB_YESNO)==IDYES)
						theApp.WSHReloadApp(pmod);
				}
				else
				{
					theApp.WSHQueueApp(ncfg);
				}
			}
		}
	}
}

void LoadAppsDlg::OnBnClickedUnload()
{
	POSITION pos=m_appList.GetFirstSelectedItemPosition();
	while(pos)
	{
		int sidx=m_appList.GetNextSelectedItem(pos);
		if(sidx>=0)
		{
			AppConfig *ncfg=(AppConfig *)m_appList.GetItemData(sidx);
			if(ncfg)
			{
				WsocksApp *pmod=theApp.WSHGetApp(ncfg->aname);
				if(pmod)
				{
					CString wmsg;
					wmsg.Format("Shutdown App(%s)?",ncfg->aname.c_str());
					if(MessageBox(wmsg,"Confirm Shutdown",MB_ICONWARNING|MB_YESNO)==IDYES)
						theApp.WSHDequeueApp(pmod);
				}
				else
				{
					CString emsg;
					emsg.Format("App(%s) is not running.",ncfg->aname.c_str());
					MessageBox(emsg,"Failed Unload",MB_ICONERROR);
				}
			}
		}
	}
}

BOOL LoadAppsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	LVCOLUMN lcols[]={
		{LVCF_WIDTH|LVCF_TEXT|LVCF_FMT,LVCFMT_RIGHT,40,"Line",0,ACOL_LINE,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,40,"Ver",0,ACOL_VER,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,80,"Class",0,ACOL_CLASS,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,80,"Name",0,ACOL_NAME,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,50,"Enabled",0,ACOL_ENABLED,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,50,"Loaded",0,ACOL_LOADED,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,100,"Threads",0,ACOL_THREADS,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,256,"DllPath",0,ACOL_DLLPATH,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,256,"ConfigPath",0,ACOL_CONFIGPATH,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,256,"RunPath",0,ACOL_RUNPATH,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,128,"RunCmd",0,ACOL_RUNCMD,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,256,"LogPath",0,ACOL_LOGPATH,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,60,"Loopback",0,ACOL_LOOPBACK,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,60,"AsyncCB",0,ACOL_ASYNC,0,0},
		{0,0,0,0,0},
	};

    m_appList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	for(int i=0;lcols[i].mask;i++)
	{
		m_appList.InsertColumn(i,&lcols[i]);
	}
	sortcol=ACOL_LINE;
	OnBnClickedRefresh();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void LoadAppsDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}

void LoadAppsDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CWnd *ebutton=GetDlgItem(IDC_EDIT);
	if(ebutton)
	{
		RECT erect;
		ebutton->GetWindowRect(&erect);
		ScreenToClient(&erect);
		int ex=erect.right -erect.left;
		erect.right=cx -11; erect.left=erect.right -ex;
		ebutton->MoveWindow(&erect);

		RECT lrect;
		m_appList.GetWindowRect(&lrect);
		ScreenToClient(&lrect);
		lrect.right=erect.left -11;
		lrect.bottom=cy -11;
		m_appList.MoveWindow(&lrect);	
	}

	CWnd *rbutton=GetDlgItem(IDC_REFRESH);
	if(rbutton)
	{
		RECT rrect;
		rbutton->GetWindowRect(&rrect);
		ScreenToClient(&rrect);
		int rx=rrect.right -rrect.left;
		rrect.right=cx -11; rrect.left=rrect.right -rx;
		rbutton->MoveWindow(&rrect);
	}

	CWnd *lbutton=GetDlgItem(IDC_LOAD);
	if(lbutton)
	{
		RECT lrect;
		lbutton->GetWindowRect(&lrect);
		ScreenToClient(&lrect);
		int lx=lrect.right -lrect.left;
		lrect.right=cx -11; lrect.left=lrect.right -lx;
		lbutton->MoveWindow(&lrect);
	}

	CWnd *ubutton=GetDlgItem(IDC_UNLOAD);
	if(ubutton)
	{
		RECT urect;
		ubutton->GetWindowRect(&urect);
		ScreenToClient(&urect);
		int ux=urect.right -urect.left;
		urect.right=cx -11; urect.left=urect.right -ux;
		ubutton->MoveWindow(&urect);
	}
}

static int CALLBACK SortApps(LPARAM e1, LPARAM e2, LPARAM hint)
{
	int sortcol=(int)hint;
	if(sortcol<0)
		return SortApps(e2,e1,-hint);

	AppConfig *pcfg1=(AppConfig *)e1;
	AppConfig *pcfg2=(AppConfig *)e2;
	switch(sortcol-1)
	{
	case ACOL_LINE:
		return pcfg1->line -pcfg2->line;
	case ACOL_VER:
		return pcfg1->ver -pcfg2->ver;
	case ACOL_CLASS:
		return _stricmp(pcfg1->aclass.c_str(),pcfg2->aclass.c_str());
	case ACOL_NAME:
		return _stricmp(pcfg1->aname.c_str(),pcfg2->aname.c_str());
	case ACOL_ENABLED:
		return pcfg1->Enabled -pcfg2->Enabled;
	case ACOL_LOADED:
	{
		int load1=0,load2=0;
		WsocksApp *pmod1=theApp.WSHGetApp(pcfg1->aname);
		if(pmod1) load1=1;
		WsocksApp *pmod2=theApp.WSHGetApp(pcfg2->aname);
		if(pmod2) load2=1;
		return load1 -load2;
	}
	case ACOL_THREADS:
		return _stricmp(pcfg1->TargetThreads.c_str(),pcfg2->TargetThreads.c_str());
	case ACOL_DLLPATH:
		return _stricmp(pcfg1->DllPath.c_str(),pcfg2->DllPath.c_str());
	case ACOL_CONFIGPATH:
		return _stricmp(pcfg1->ConfigPath.c_str(),pcfg2->ConfigPath.c_str());
	case ACOL_RUNPATH:
		return _stricmp(pcfg1->RunPath.c_str(),pcfg2->RunPath.c_str());
	case ACOL_RUNCMD:
		return _stricmp(pcfg1->RunCmd.c_str(),pcfg2->RunCmd.c_str());
	case ACOL_LOGPATH:
		return _stricmp(pcfg1->LogPath.c_str(),pcfg2->LogPath.c_str());
	case ACOL_LOOPBACK:
		return pcfg1->wshLoopback -pcfg2->wshLoopback;
	case ACOL_ASYNC:
		return pcfg1->asyncMode -pcfg2->asyncMode;
	};
	return 0;
}
void LoadAppsDlg::OnHdnItemclickApplist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nsort=pNMListView->iSubItem +1;
	if(nsort==sortcol)
		sortcol=-sortcol;
	else
		sortcol=nsort;
	m_appList.SortItems(SortApps,(DWORD_PTR)sortcol);
	*pResult = 0;
}
