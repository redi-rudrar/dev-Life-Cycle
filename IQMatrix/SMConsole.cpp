// SMConsole.cpp : implementation file
//

#include "stdafx.h"
#include "IQMatrix.h"
#include "SMConsole.h"
#include ".\smconsole.h"
#include "wstring.h"
#include <shlwapi.h>

// SMConsole dialog

IMPLEMENT_DYNAMIC(SMConsoleDlg, CDialog)
SMConsoleDlg::SMConsoleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SMConsoleDlg::IDD, pParent)
	,phost(0)
	,smc(0)
	//,tbuf(0)
	//,tsize(0)
	,uitid(0)
	,ffont(0)
	,pfont(0)
	//,editRatio(0)
{
}

SMConsoleDlg::~SMConsoleDlg()
{
	//if(tbuf)
	//{
	//	delete tbuf; tbuf=0; tsize=0;
	//}	
	#ifdef DIALOG_DLLS
	dlgapi.ShutdownDlgServer();
	#endif
    delete(ffont);
    delete(pfont);
    ClearHelp();
}

void SMConsoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT, m_edit);
	DDX_Control(pDX, IDC_APATH, m_apath);
	DDX_Control(pDX, IDC_CMD, m_cmd);
	DDX_Control(pDX, IDC_CMDLABEL, m_cmdlabel);
	DDX_Control(pDX, IDC_SEND, m_send);
	DDX_Control(pDX, IDC_FONT, m_font);
	DDX_Control(pDX, IDC_PARMS, m_parms);
	DDX_Control(pDX, IDC_SAMPLES, m_samples);
	DDX_Control(pDX, IDC_CMDLIST, m_cmdlist);
	DDX_Control(pDX, IDC_PARMHELP, m_parmhelp);
}


BEGIN_MESSAGE_MAP(SMConsoleDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_SEND, OnBnClickedSend)
	ON_CBN_SELCHANGE(IDC_APATH, OnCbnSelchangeApath)
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_FONT, OnBnClickedFont)
	ON_NOTIFY(NM_DBLCLK, IDC_SAMPLES, OnNMDblclkSamples)
	ON_NOTIFY(NM_DBLCLK, IDC_PARMS, OnNMDblclkParms)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_PARMS, OnLvnEndlabeleditParms)
	ON_NOTIFY(NM_RCLICK, IDC_PARMS, OnNMRclickParms)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_PARMS, OnLvnBeginlabeleditParms)
	ON_CBN_SELCHANGE(IDC_CMDLIST, OnCbnSelchangeCmdlist)
	ON_NOTIFY(NM_RCLICK, IDC_SAMPLES, OnNMRclickSamples)
	ON_COMMAND(ID_EDITCOMMONSAMPLES, OnEditCommonSamples)
	ON_COMMAND(ID_RELOADCOMMONSAMPLES, OnReloadCommonSamples)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
END_MESSAGE_MAP()


// SMConsoleDlg message handlers
void SMConsoleDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	ShowWindow(SW_HIDE);
	SaveConsole("logs\\console.txt");
	// The applist should rebuild when it's closed and reopened
	m_apath.ResetContent();
	//CDialog::OnClose();
}
void SMConsoleDlg::OnOK()
{
}
void SMConsoleDlg::OnCancel()
{
}
void SMConsoleDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if(smc)
		smc->ConPort[0].ConnectHold=bShow ?false :true;
}

void SMConsoleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if(IsWindow(m_edit.m_hWnd))
	{
		// Move the label Y only
		RECT lrect;
		m_cmdlabel.GetWindowRect(&lrect);
		ScreenToClient(&lrect);
		int ly=lrect.bottom -lrect.top;
		lrect.bottom=cy -17; lrect.top=lrect.bottom -ly;
		m_cmdlabel.MoveWindow(&lrect);
		// Keep the button width and height but translate X and Y
		RECT srect;
		m_send.GetWindowRect(&srect);
		ScreenToClient(&srect);
		int sx=srect.right -srect.left,sy=srect.bottom -srect.top;
		srect.right=cx -11; srect.left=srect.right -sx;
		srect.bottom=cy -11; srect.top=srect.bottom -sy;
		m_send.MoveWindow(&srect);
		// Keep the cmd height, but stretch the width
		RECT mrect;
		m_cmd.GetWindowRect(&mrect);
		ScreenToClient(&mrect);
		int my=mrect.bottom -mrect.top;
		mrect.right=srect.left -6;
		mrect.bottom=cy -11; mrect.top=mrect.bottom -my;
		m_cmd.MoveWindow(&mrect);
		// Keep the samples width and height
		RECT smrect;
		m_samples.GetWindowRect(&smrect);
		ScreenToClient(&smrect);
		int smx=smrect.right -smrect.left,smy=smrect.bottom -smrect.top;
		smrect.right=cx -11; smrect.left=smrect.right -smx;
		smrect.bottom=mrect.top -11; smrect.top=smrect.bottom -smy;
		m_samples.MoveWindow(&smrect);
		// Keep the parms width but stretch the height
		RECT pmrect;
		m_parms.GetWindowRect(&pmrect);
		ScreenToClient(&pmrect);
		int pmx=pmrect.right -pmrect.left;
		pmrect.right=cx -11; pmrect.left=pmrect.right -pmx;
		pmrect.bottom=smrect.top -11;
		m_parms.MoveWindow(&pmrect);
		// Stretch width and height of console history
		RECT erect;
		m_edit.GetWindowRect(&erect);
		ScreenToClient(&erect);
		erect.right=pmrect.left -11;
		erect.bottom=mrect.top -11;
		m_edit.MoveWindow(&erect);
		// Keep the path height, but stretch the width
		RECT arect;
		m_apath.GetWindowRect(&arect);
		ScreenToClient(&arect);
		arect.right=mrect.right;
		m_apath.MoveWindow(&arect);
		// Keep the parm help width and height, but translate X
		RECT phrect;
		m_parmhelp.GetWindowRect(&phrect);
		ScreenToClient(&phrect);
		int phx=phrect.right -phrect.left;
		phrect.left=pmrect.left; phrect.right=phrect.left +phx;
		m_parmhelp.MoveWindow(&phrect);
		// Keep the cmdlist width and height, but translate X
		RECT clrect;
		m_cmdlist.GetWindowRect(&clrect);
		ScreenToClient(&clrect);
		int clx=clrect.right -clrect.left;
		clrect.right=cx -11; clrect.left=clrect.right -clx;
		m_cmdlist.MoveWindow(&clrect);
		// Keep the font width and height, but translate X
		RECT frect;
		m_font.GetWindowRect(&frect);
		ScreenToClient(&frect);
		int fx=frect.right -frect.left;
		frect.right=cx -11; frect.left=frect.right -fx;
		m_font.MoveWindow(&frect);
	}
}

enum ParmListCols
{
	PCOL_VALUE,
	PCOL_PARM,
	PCOL_OPT,
	PCOL_SEL,
	PCOL_DESC,
	PCOL_COUNT
};
enum SampListCols
{
	SCOL_SAMP,
	SCOL_DESC,
	SCOL_TYPE,
	SCOL_COUNT
};
BOOL SMConsoleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if((!phost)||(!phost->lbmon))
		return FALSE;
	uitid=GetCurrentThreadId();
	m_cmd.LimitText(2048);

	//// Remember the edit control's ratio for resizing
	//RECT crect;
	//GetClientRect(&crect);
	//int cx=crect.right -crect.left;
	//RECT erect;
	//m_edit.GetWindowRect(&erect);
	//ScreenToClient(&erect);
	//editRatio=(erect.right -erect.left)*100 /(cx -22);

	// Parameter columns
	LVCOLUMN pcols[]={
		{LVCF_WIDTH|LVCF_TEXT,0,60,"Value",0,PCOL_VALUE,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,60,"Parm",0,PCOL_PARM,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,30,"Opt",0,PCOL_OPT,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,30,"Sel",0,PCOL_SEL,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,200,"Desc",0,PCOL_DESC,0,0},
		{0,0,0,0,0},
	};
    m_parms.SetExtendedStyle(m_parms.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_SHOWSELALWAYS);
	int i;
	for(i=0;pcols[i].mask;i++)
	{
		m_parms.InsertColumn(i,&pcols[i]);
	}

	// Sample columns
	LVCOLUMN scols[]={
		{LVCF_WIDTH|LVCF_TEXT,0,200,"Sample",0,SCOL_SAMP,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,200,"Desc",0,SCOL_DESC,0,0},
		{LVCF_WIDTH|LVCF_TEXT,0,60,"Type",0,SCOL_TYPE,0,0},
		{0,0,0,0,0},
	};
    m_samples.SetExtendedStyle(m_samples.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_SHOWSELALWAYS);
	for(i=0;scols[i].mask;i++)
	{
		m_samples.InsertColumn(i,&scols[i]);
	}

	// Create a fixed font
	ffont=new CFont;
	ffont->CreateFont(
		9,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		"Fixedsys");                 // lpszFacename
	// Copy the default font
	CFont *tfont=m_edit.GetFont();
	LOGFONT lf;
	tfont->GetLogFont(&lf);
	pfont=new CFont;
	pfont->CreateFontIndirect(&lf);

	AppConfig *pcfg=new AppConfig;
	pcfg->aname="SMCon";
	pcfg->ver=WS_APPVERS_1_0;
	pcfg->Enabled=true;
	pcfg->aclass="SMConsole";
	char mpath[MAX_PATH]={0};
	GetModuleFileName(0,mpath,MAX_PATH);
	pcfg->DllPath=mpath;
	GetCurrentDirectory(MAX_PATH,mpath);
	pcfg->RunPath=mpath;
	sprintf(mpath,"%s\\logs",pcfg->RunPath.c_str());
	pcfg->LogPath=mpath;
	sprintf(mpath,"%s\\setup\\app.ini",pcfg->RunPath.c_str());
	pcfg->ConfigPath=mpath;
	pcfg->RunCmd="SMConsole";
	pcfg->asyncMode=0;
	pcfg->TargetThreads="LBMon"; // share the lbmon thread
	pcfg->wshLoopback=true;
	pcfg->loopback=true;
    pcfg->shareHostLog=true;
	pcfg->lbDllHnd=GetModuleHandle(0);
	pcfg->plbGETAPPINTERFACE=SMConsole_GetAppInterface;
	smc=(SMConsole*)phost->WSHLoadApp(pcfg);
	if(!smc)
		return FALSE;
	smc->pdlg=this;
	smc->ConPort[0].Port=phost->lbmon->LBMGetLBPort();
	smc->ConPort[0].ConnectHold=false;

	OnReloadCommonSamples();
	#ifdef DIALOG_DLLS
	char ppath[MAX_PATH]={0};
	sprintf(ppath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
	dlgapi.InitDlgServer("SMConsole",this,smc->ccodec,ppath);
	#endif
	return TRUE;
}
void SMConsoleDlg::OnBnClickedFont()
{
	if(m_font.GetCheck()==BST_CHECKED)
		m_edit.SetFont(ffont);
	else
		m_edit.SetFont(pfont);
	UpdateData(true);
}
int SMConsoleDlg::AppendString(const char *astr)
{
	int tlen=m_edit.GetWindowTextLength();
	m_edit.SetSel(tlen,tlen);
	char tstr[1024]={0};
	sprintf(tstr,"%s\r\n",astr);
	m_edit.ReplaceSel(tstr);
	return 0;
}
int SMConsoleDlg::WSHLoggedError(const char *estr)
{
	AppendString(estr +9);
	return 0;
}
int SMConsoleDlg::WSHLoggedEvent(const char *estr)
{
	AppendString(estr +9);
	return 0;
}
int SMConsoleDlg::NotifyApp(const char *apath)
{
	if(apath)
	{
		if(GetCurrentThreadId()==uitid)
		{
			int sidx=m_apath.FindStringExact(-1,apath);
			if(sidx<0)
			{
				m_apath.InsertString(-1,apath);
				if(m_apath.GetCurSel()<0)
				{
					m_apath.SelectString(-1,apath);
					OnCbnSelchangeApath();
				}
			}
		}
		else
		{
			char *acpy=_strdup(apath);
			PostMessage(WM_USER +1,0,(LPARAM)acpy);
		}
	}
	else
		m_apath.Clear();
	return 0;
}
int SMConsoleDlg::SaveConsole(const char *spath)
{
	// The text already has \r\n embedded, so write binary
	FILE *sfp=fopen(spath,"wb");
	if(!sfp)
	{
		smc->WSLogEvent("Resp> Failed opening (%s)!",spath);
		return -1;
	}
	int tlen=m_edit.GetWindowTextLength();
	char *tbuf=new char[tlen];
	if(tbuf)
	{
		m_edit.GetWindowText(tbuf,tlen);
		fwrite(tbuf,1,tlen,sfp);
		delete tbuf;
	}
	fclose(sfp);
	return 0;
}
void SMConsoleDlg::OnBnClickedSend()
{
	int sidx=m_apath.GetCurSel();
	if(sidx<0)
	{
		smc->WSLogEvent("AppPath required for Cmd.");
		return;
	}
	CString apath;
	m_apath.GetLBText(sidx,apath);

	char cbuf[2048]={0},*cmd=cbuf,parms[2048]={0},*pptr=parms;
	m_cmd.GetWindowText(cbuf,2048);
	if(!*cbuf)
		return;
	CString lcmd;
	if(m_cmd.GetCount()>0)
	{
		m_cmd.GetLBText(0,lcmd);
		if(lcmd!=cbuf)
			m_cmd.InsertString(0,cbuf);
	}
	else
		m_cmd.InsertString(0,cbuf);
	// Help for common commands
	if(!strcmp(cbuf,"?"))
		strcpy(cbuf,"helpcommon");
	// Clear the console
	else if(!strcmp(cbuf,"cls"))
	{
		m_edit.SetWindowText("");
		m_cmd.SetWindowText("");
		return;
	}
	// Save the console
	else if(!strncmp(cbuf,"save ",5))
	{
		char spath[MAX_PATH]={0},*sname=0;
		GetFullPathName(cbuf +5,MAX_PATH,spath,&sname);
		if(PathFileExists(spath))
		{
			smc->WSLogEvent("Resp> File (%s) already exits!",spath);
			return;
		}
		if(!SaveConsole(spath))
		{
			smc->WSLogEvent("Resp> Saved console to (%s).",spath);
			ShellExecute(m_hWnd,"open","notepad.exe",spath,0,SW_SHOW);
			m_cmd.SetWindowText("");
		}
		return;
	}

	sprintf(pptr,"DeliverTo=%s|OnBehalfOf=SMConsole|",apath); pptr+=strlen(pptr);
	char *cptr=strchr(cmd,' ');
	if(cptr)
	{
		*cptr=0;
		////Was trying to be nice, but sometimes we want spaces in parameters
		//for(cptr++;*cptr;cptr++)
		//{
		//	if(*cptr==' ') *pptr='|';
		//	else *pptr=*cptr;
		//	pptr++;
		//}
		strcpy(pptr,cptr+1); pptr+=strlen(pptr);
	}

	char mbuf[2048]={0},*mptr=mbuf;
	smc->ccodec->Encode(mptr,sizeof(mbuf),true,++smc->rid,cmd,parms,0);
	//// don't display passwords
	//char dbuf[2048]={0};
	//strcpy(dbuf,mbuf); dbuf[mptr -mbuf]=0;
	//char *dptr=(char*)stristr(dbuf,"Pass=");
	//if(dptr)
	//{
	//	const char *eptr=strchr(dptr,'|');
	//	if(!eptr) eptr=dptr +strlen(dptr);
	//	memset(dptr +5,'*',eptr -dptr -5);
	//}
	smc->WSLogEvent("Cmd> %s",mbuf);
	m_cmd.SetWindowText("");
	if(!smc->WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,0))
	{
		AppendString("Failed sending command!");
		smc->WSLogError("Failed sending command!");
	}
}

LRESULT SMConsoleDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	// Posted by NotifyApp
	case WM_USER +1:
	{
		char *apath=(char*)lParam;
		NotifyApp(apath);
		free(apath);
		return FALSE;
	}
	// Posted by OnCbnSelchangeCmd
	case WM_USER +2:
	{
		BuildCmd();
		return FALSE;
	}
	};

	return CDialog::DefWindowProc(message, wParam, lParam);
}

void SMConsoleDlg::OnCbnSelchangeApath()
{
	int sidx=m_apath.GetCurSel();
	if(sidx<0)
		return;
	CString apath;
	m_apath.GetLBText(sidx,apath);
	HELPMAP::iterator hit=helpMap.find((const char *)apath);
	APPHELP *ahelp=0;
	if(hit==helpMap.end())
	{
		char mbuf[1024]={0},*mptr=mbuf,parms[1024]={0};
		sprintf(parms,"DeliverTo=%s|OnBehalfOf=SMConsole|",apath);
		smc->ccodec->Encode(mptr,sizeof(mbuf),true,++smc->rid,"help",parms,0);
		smc->WSLogEvent("Cmd> %s",mbuf);
		smc->WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,0);
	}
	else
		LoadHelp(apath,hit->second);
}
int SMConsoleDlg::LoadHelp(const char *apath, APPHELP *ahelp)
{
	m_cmdlist.ResetContent();
	m_parms.DeleteAllItems();
	m_samples.DeleteAllItems();
	for(CMDMAP::iterator cit=ahelp->cmdmap.begin();cit!=ahelp->cmdmap.end();cit++)
	{
		CMDHELP *chelp=cit->second;
		char cstr[1024]={0};
		sprintf(cstr,"%s, %s%s",chelp->cmd.c_str(),chelp->desc.c_str(),chelp->common?" (common)":"");
		int idx=m_cmdlist.InsertString(-1,cstr);
		if(idx>=0)
			m_cmdlist.SetItemData(idx,(DWORD_PTR)chelp);
	}
	m_cmdlist.InsertString(-1,"Command List:");
	m_cmdlist.SelectString(-1,"Command List:");
	return 0;
}
void SMConsoleDlg::ClearHelp()
{
    for(HELPMAP::iterator hit = helpMap.begin(); hit != helpMap.end(); hit++)
    {
        APPHELP *ahelp = hit->second;
        for(CMDMAP::iterator cit = ahelp->cmdmap.begin(); cit != ahelp->cmdmap.end(); cit++)
        {
            CMDHELP *chelp = cit->second;
            for(PARMLIST::iterator pit = chelp->parmlist.begin(); pit != chelp->parmlist.end(); pit++)
            {
                PARMHELP *phelp = *pit;
                delete(phelp);
            }
            for(SAMPLIST::iterator sit = chelp->samplist.begin(); sit != chelp->samplist.end(); sit++)
            {
                SAMPLE *shelp = *sit;
                delete(shelp);
            }
            chelp->parmlist.clear();
            chelp->samplist.clear();
            delete(chelp);
        }
        ahelp->cmdmap.clear();
        delete(ahelp);
    }
    helpMap.clear();
}
int SMConsoleDlg::NotifyHelp(TVMAP& tvmap, string cmd, char *parms)
{
	// Find the app's help
	APPHELP *ahelp=0;
	string apath=smc->ccodec->GetValue(tvmap,"OnBehalfOf");
	HELPMAP::iterator hit=helpMap.find(apath);
	if(hit==helpMap.end())
	{
		ahelp=new APPHELP;
		helpMap[apath]=ahelp;
	}
	else
		ahelp=hit->second;

	string begin=smc->ccodec->GetValue(tvmap,"Begin");
	if((begin=="Y")||(begin=="y"))
	{
		for(CMDMAP::iterator cit=ahelp->cmdmap.begin();cit!=ahelp->cmdmap.end();)
		{
			CMDHELP *chelp=cit->second;
			if(((cmd=="helpcommon")&&(chelp->common))||
			   ((cmd=="help")&&(!chelp->common)))
			{
				for(PARMLIST::iterator pit=chelp->parmlist.begin();pit!=chelp->parmlist.end();pit++)
					delete *pit;
				chelp->parmlist.clear();
				for(SAMPLIST::iterator sit=chelp->samplist.begin();sit!=chelp->samplist.end();sit++)
					delete *sit;
				chelp->samplist.clear();
				delete chelp;
				cit=ahelp->cmdmap.erase(cit);
			}
			else
				cit++;
		}
	}

	// Fully-described help
	string hcmd=smc->ccodec->GetValue(tvmap,"HelpCmd");
	if(!hcmd.empty())
	{
		// Command
		CMDHELP *chelp=0;
		CMDMAP::iterator cit=ahelp->cmdmap.find(hcmd);
		if(cit==ahelp->cmdmap.end())
		{
			chelp=new CMDHELP;
			chelp->ahelp=ahelp;
			chelp->cmd=hcmd;
			chelp->common=(cmd=="helpcommon")?true:false;
			ahelp->cmdmap[hcmd]=chelp;
		}
		else
		{
			chelp=cit->second;
			for(PARMLIST::iterator pit=chelp->parmlist.begin();pit!=chelp->parmlist.end();pit++)
				delete *pit;
			chelp->parmlist.clear();
			for(SAMPLIST::iterator sit=chelp->samplist.begin();sit!=chelp->samplist.end();sit++)
				delete *sit;
			chelp->samplist.clear();
		}
		string desc=smc->ccodec->GetValue(tvmap,"Desc");
		if(!desc.empty())
			chelp->desc=desc;
		// Parameters
		int nparms=atol(smc->ccodec->GetValue(tvmap,"NumParms").c_str());
		for(int i=1;i<=nparms;i++)
		{
			char tstr[128]={0};
			sprintf(tstr,"Parm%d",i);
			string parm=smc->ccodec->GetValue(tvmap,tstr);
			sprintf(tstr,"ParmDef%d",i);
			string def=smc->ccodec->GetValue(tvmap,tstr);
			sprintf(tstr,"ParmOpt%d",i);
			string opt=smc->ccodec->GetValue(tvmap,tstr);
			sprintf(tstr,"ParmDesc%d",i);
			string pdesc=smc->ccodec->GetValue(tvmap,tstr);
			PARMHELP *phelp=0;
			for(PARMLIST::iterator pit=chelp->parmlist.begin();pit!=chelp->parmlist.end();pit++)
			{
				if((*pit)->parm==parm)
				{
					phelp=*pit;
					break;
				}
			}
			if(!phelp)
			{
				phelp=new PARMHELP;
				phelp->parm=parm;
				chelp->parmlist.push_back(phelp);
			}
			phelp->defval=def;
			phelp->opt=(opt=="Y"||opt=="y")?true:false;
			phelp->sel=!phelp->opt;
			phelp->desc=pdesc;
		}
		// Developer samples
		int nsamples=atol(smc->ccodec->GetValue(tvmap,"NumSamples").c_str());
		for(int i=1;i<=nsamples;i++)
		{
			char tstr[128]={0};
			sprintf(tstr,"Sample%d",i);
			string samp=smc->ccodec->GetValue(tvmap,tstr);
			sprintf(tstr,"SampleDesc%d",i);
			string sdesc=smc->ccodec->GetValue(tvmap,tstr);
			// Convert / to | except when preceeded by escape
			char sampstr[1024]={0},*sptr=sampstr;
			bool esc=false;
			for(const char *cptr=samp.c_str();*cptr;cptr++)
			{
				if(esc)
				{
					*sptr=*cptr; esc=false; sptr++;
					continue;
				}
				else if(*cptr=='\\')
				{
					esc=true;
					continue;
				}
				else if(*cptr=='/')
					*sptr='|';
				else
					*sptr=*cptr;
				esc=false; sptr++;
			}
			*sptr=0;

			SAMPLE *shelp=0;
			for(SAMPLIST::iterator sit=chelp->samplist.begin();sit!=chelp->samplist.end();sit++)
			{
				if((*sit)->no==i)
				{
					shelp=*sit;
					break;
				}
			}
			if(!shelp)
			{
				shelp=new SAMPLE;
				shelp->no=i;
				shelp->type=1;
				chelp->samplist.push_back(shelp);
			}
			shelp->ex=sampstr;
			shelp->desc=sdesc;
		}
	}

	// Load the current help
	string end=smc->ccodec->GetValue(tvmap,"End");
	if((end=="Y")||(end=="y"))
	{
		int sidx=m_apath.GetCurSel();
		if(sidx>=0)
		{
			CString spath;
			m_apath.GetLBText(sidx,spath);
			if(!_stricmp(spath,apath.c_str()))
				LoadHelp(spath,ahelp);
		}
	}
	return 0;
}
static int WINAPI SortParms(LPARAM e1, LPARAM e2, LPARAM hint)
{
	PARMHELP *phelp1=(PARMHELP *)e1;
	PARMHELP *phelp2=(PARMHELP *)e2;
	return _stricmp(phelp1->parm.c_str(),phelp2->parm.c_str());
}
void SMConsoleDlg::OnCbnSelchangeCmdlist()
{
	m_parms.DeleteAllItems();
	m_samples.DeleteAllItems();

	int sidx=m_cmdlist.GetCurSel();
	if(sidx<0)
		return;
	//CString cmd;
	//m_cmdlist.GetLBText(sidx,cmd);
	CMDHELP *chelp=(CMDHELP*)m_cmdlist.GetItemData(sidx);
	if(!chelp)
		return;

	// Populate common samples (i.e. created by operations)
	int aidx=m_apath.GetCurSel();
	if(aidx>=0)
	{
		CString apath;
		m_apath.GetLBText(aidx,apath);
		// Search for app instance examples
		char ainst[256]={0};
		char *aptr=(char*)strrchr(apath,'/');
		if(aptr) strcpy(ainst,aptr +1);
		else strcpy(ainst,apath);
		HELPMAP::iterator ait=csamples.find(ainst);
		if(ait!=csamples.end())
		{
			APPHELP *sahelp=ait->second;
			CMDMAP::iterator cit=sahelp->cmdmap.find(chelp->cmd);
			if(cit!=sahelp->cmdmap.end())
			{
				CMDHELP *schelp=cit->second;
				for(SAMPLIST::iterator sit=schelp->samplist.begin();sit!=schelp->samplist.end();sit++)
				{
					SAMPLE *shelp=*sit;
					LVITEM litem;
					memset(&litem,0,sizeof(LVITEM));
					litem.iItem=m_samples.GetItemCount();
					litem.mask=LVIF_PARAM;
					litem.lParam=(DWORD)(PTRCAST)shelp;
					int idx=m_samples.InsertItem(&litem);
					if(idx>=0)
					{
						m_samples.SetItemText(idx,SCOL_SAMP,shelp->ex.c_str());
						m_samples.SetItemText(idx,SCOL_DESC,shelp->desc.c_str());
						string stype;
						switch(shelp->type)
						{
						case 1: stype="app"; break;
						case 2: stype="class"; break;
						case 3: stype="inst"; break;
						};
						m_samples.SetItemText(idx,SCOL_TYPE,stype.c_str());
					}
				}
			}
		}
		// Search for app class examples
		char aclass[256]={0},aname[256]={0};
		strcpy(aclass,ainst);
		aptr=strchr(aclass,'.');
		if(aptr) 
		{
			strcpy(aname,aptr +1);
			*(aptr+1)=0;
		}
		ait=csamples.find(aclass);
		if(ait!=csamples.end())
		{
			APPHELP *sahelp=ait->second;
			CMDMAP::iterator cit=sahelp->cmdmap.find(chelp->cmd);
			if(cit!=sahelp->cmdmap.end())
			{
				CMDHELP *schelp=cit->second;
				for(SAMPLIST::iterator sit=schelp->samplist.begin();sit!=schelp->samplist.end();sit++)
				{
					SAMPLE *shelp=*sit;
					LVITEM litem;
					memset(&litem,0,sizeof(LVITEM));
					litem.iItem=m_samples.GetItemCount();
					litem.mask=LVIF_PARAM;
					litem.lParam=(DWORD)(PTRCAST)shelp;
					int idx=m_samples.InsertItem(&litem);
					if(idx>=0)
					{
						m_samples.SetItemText(idx,SCOL_SAMP,shelp->ex.c_str());
						m_samples.SetItemText(idx,SCOL_DESC,shelp->desc.c_str());
						string stype;
						switch(shelp->type)
						{
						case 1: stype="app"; break;
						case 2: stype="class"; break;
						case 3: stype="inst"; break;
						};
						m_samples.SetItemText(idx,SCOL_TYPE,stype.c_str());
					}
				}
			}
		}

		#ifdef DIALOG_DLLS
		aclass[strlen(aclass) -1]=0;
		dlgapi.FindDlgProvider(aclass,aname,(const char*)apath,chelp->cmd);
		#else
		// Built-in iqserver rendering apps
		if((!_stricmp(aclass,"iqclient."))||(!_stricmp(aclass,"iqserver.")))
		{
			if(chelp->cmd=="lvl1")
				smc->Lvl1Dlg(apath);
			else if(chelp->cmd=="lvl2")
				smc->Lvl2Dlg(apath);
			else if(chelp->cmd=="montage")
				smc->MontageDlg(apath);
		}
		#endif
	}

	// Populate parameters
	for(PARMLIST::iterator pit=chelp->parmlist.begin();pit!=chelp->parmlist.end();pit++)
	{
		PARMHELP *phelp=*pit;
		LVITEM litem;
		memset(&litem,0,sizeof(LVITEM));
		litem.iItem=m_parms.GetItemCount();
		litem.mask=LVIF_PARAM;
		litem.lParam=(DWORD)(PTRCAST)phelp;
		int idx=m_parms.InsertItem(&litem);
		if(idx>=0)
		{
			m_parms.SetItemText(idx,PCOL_PARM,phelp->parm.c_str());
			m_parms.SetItemText(idx,PCOL_VALUE,phelp->defval.c_str());
			m_parms.SetItemText(idx,PCOL_OPT,phelp->opt?"Y":"N");
			m_parms.SetItemText(idx,PCOL_SEL,phelp->sel?"Y":"N");
			m_parms.SetItemText(idx,PCOL_DESC,phelp->desc.c_str());
		}
	}
	// Since editlabels take the first column, we have to sort on "Parm" column ourself
	m_parms.SortItems(SortParms,0);

	// Populate application-provided samples
	for(SAMPLIST::iterator sit=chelp->samplist.begin();sit!=chelp->samplist.end();sit++)
	{
		SAMPLE *shelp=*sit;
		LVITEM litem;
		memset(&litem,0,sizeof(LVITEM));
		litem.iItem=m_samples.GetItemCount();
		litem.mask=LVIF_PARAM;
		litem.lParam=(DWORD)(PTRCAST)shelp;
		int idx=m_samples.InsertItem(&litem);
		if(idx>=0)
		{
			m_samples.SetItemText(idx,SCOL_SAMP,shelp->ex.c_str());
			m_samples.SetItemText(idx,SCOL_DESC,shelp->desc.c_str());
			string stype;
			switch(shelp->type)
			{
			case 1: stype="app"; break;
			case 2: stype="class"; break;
			case 3: stype="inst"; break;
			};
			m_samples.SetItemText(idx,SCOL_TYPE,stype.c_str());
		}
	}

	// The new text hasn't been applied yet, so post a message to build the command
	PostMessage(WM_USER +2,0,0);
}
void SMConsoleDlg::OnNMDblclkSamples(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	for(POSITION pos=m_samples.GetFirstSelectedItemPosition();pos;)
	{
		int sidx=m_samples.GetNextSelectedItem(pos);
		if(sidx>=0)
		{
			SAMPLE *shelp=(SAMPLE*)m_samples.GetItemData(sidx);
			if(shelp)
				m_cmd.SetWindowText(shelp->ex.c_str());
		}
	}
}
void SMConsoleDlg::OnNMDblclkParms(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	for(POSITION pos=m_parms.GetFirstSelectedItemPosition();pos;)
	{
		int sidx=m_parms.GetNextSelectedItem(pos);
		if(sidx>=0)
		{
			m_parms.EditLabel(sidx);
			break;
		}
	}
}

void SMConsoleDlg::OnLvnBeginlabeleditParms(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	//// Tried to move the window to the second column, but didn't work
	//CEdit *pedit=m_parms.GetEditControl();
	//if(pedit)
	//{
	//	RECT erect;
	//	pedit->GetWindowRect(&erect);
	//	ScreenToClient(&erect);
	//	int dx=10;
	//	erect.left+=dx; erect.right+=dx;
	//	pedit->MoveWindow(&erect);
	//}
	*pResult = 0;
}
void SMConsoleDlg::OnLvnEndlabeleditParms(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	CEdit *pedit=m_parms.GetEditControl();
	if(pedit)
	{
		CString nval;
		pedit->GetWindowText(nval);
		for(POSITION pos=m_parms.GetFirstSelectedItemPosition();pos;)
		{
			int sidx=m_parms.GetNextSelectedItem(pos);
			if(sidx>=0)
			{
				PARMHELP *phelp=(PARMHELP*)m_parms.GetItemData(sidx);
				if(phelp)
				{
					if(!_stricmp(phelp->parm.c_str(),"symbol"))
						nval.MakeUpper();
					phelp->defval=nval;
					m_parms.SetItemText(sidx,PCOL_VALUE,nval);
					phelp->sel=true;
					m_parms.SetItemText(sidx,PCOL_SEL,phelp->sel?"Y":"N");
					BuildCmd();
				}
				break;
			}
		}
	}
	*pResult = 0;
}
void SMConsoleDlg::OnNMRclickParms(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
	for(POSITION pos=m_parms.GetFirstSelectedItemPosition();pos;)
	{
		int sidx=m_parms.GetNextSelectedItem(pos);
		if(sidx>=0)
		{
			PARMHELP *phelp=(PARMHELP*)m_parms.GetItemData(sidx);
			if(phelp)
			{
				phelp->sel=!phelp->sel;
				m_parms.SetItemText(sidx,PCOL_SEL,phelp->sel?"Y":"N");
				BuildCmd();
			}
			break;
		}
	}
}
int SMConsoleDlg::BuildCmd()
{
	int sidx=m_cmdlist.GetCurSel();
	if(sidx<0)
		return -1;
	CMDHELP *chelp=(CMDHELP*)m_cmdlist.GetItemData(sidx);
	if(chelp)
	{
		char cstr[1024]={0},*cptr=cstr;
		sprintf(cptr,"%s ",chelp->cmd.c_str()); cptr+=strlen(cptr);
		for(PARMLIST::iterator pit=chelp->parmlist.begin();pit!=chelp->parmlist.end();pit++)
		{
			PARMHELP *phelp=*pit;
			if(phelp->sel)
			{
				sprintf(cptr,"%s=%s|",phelp->parm.c_str(),phelp->defval.c_str()); cptr+=strlen(cptr);
			}
		}
		*cptr=0;
		m_cmd.SetWindowText(cstr);
	}
	return 0;
}
void SMConsoleDlg::OnNMRclickSamples(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	HMENU hmenu = CreatePopupMenu();
	AppendMenu(hmenu,MF_STRING,ID_EDITCOMMONSAMPLES,"&Edit common samples...");
	AppendMenu(hmenu,MF_STRING,ID_RELOADCOMMONSAMPLES,"&Reload common samples");
	POINT pt;
	GetCursorPos(&pt);
	TrackPopupMenu(hmenu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,m_hWnd,0);
}
int SMConsoleDlg::FindCommonSamples(char *fpath)
{
	strcpy(fpath,"P:\\dafsoft\\setup\\sysmon_samples.txt");
	if(PathFileExists(fpath))
		return 0;
	strcpy(fpath,"X:\\software\\sw_beta\\sysmon_samples.txt");
	if(PathFileExists(fpath))
		return 0;
	strcpy(fpath,"X:\\sw_beta\\sysmon_samples.txt");
	if(PathFileExists(fpath))
		return 0;
	MessageBox("No sysmon_samples.txt file could be found in any of the default locations.","File not found",MB_ICONERROR);
	return -1;
}
void SMConsoleDlg::OnEditCommonSamples()
{
	char fpath[MAX_PATH]={0};
	if(FindCommonSamples(fpath)<0)
		return;
	ShellExecute(m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
}
void SMConsoleDlg::OnReloadCommonSamples()
{
	char fpath[MAX_PATH]={0};
	if(FindCommonSamples(fpath)<0)
		return;
	char estr[1024]={0};
	HELPMAP nhelp;
	FILE *fp=fopen(fpath,"rt");
	if(!fp)
	{
		sprintf(estr,"Failed opening \"%s\"!",fpath);
		MessageBox(estr,"Failed Open",MB_ICONERROR);
		return;
	}
	APPHELP *ahelp=0;
	int lno=0,nsamp=0,stype=0;
	char rbuf[1024]={0};
	while(fgets(rbuf,1024,fp))
	{
		lno++;
		// Skip comments and blank lines
		if(!strncmp(rbuf,"//",2)||(!rbuf[0])||(rbuf[0]=='\r')||(rbuf[0]=='\n'))
			continue;
		// [appclass[.appinst]]
		if(rbuf[0]=='[')
		{
			char *rptr=strchr(rbuf,']');
			if(!rptr) rptr=rbuf +strlen(rbuf);
			char appcxt[128]={0};
			strncpy(appcxt,rbuf+1,rptr -rbuf -1);
			if(!strchr(appcxt,'.'))
			{
				stype=2; // class
				strcat(appcxt,".");
			}
			else
				stype=3; // instance
			HELPMAP::iterator ait=nhelp.find(appcxt);
			if(ait==nhelp.end())
			{
				ahelp=new APPHELP;
				nhelp[appcxt]=ahelp;
			}
			else
				ahelp=ait->second;
			continue;
		}
		// cmd|sample[|desc]
		else if(!ahelp)
		{
			sprintf(estr,"No application context for sample at line %d!",lno);
			goto error;
		}
		char *cmd=rbuf;
		char *ex=strchr(cmd,'|');
		if(!ex)
		{
			sprintf(estr,"Sample field missing at line %d!",lno);
			goto error;
		}
		*ex=0; ex++;
		char *desc=strchr(ex,'|');
		//if(!desc)
		//{
		//	sprintf(estr,"Description field missing at line %d!",lno);
		//	MessageBox(estr,"Failed Load",MB_ICONERROR);
		//	return;
		//}
		if(desc)
		{
			*desc=0; desc++;
			char *next=strchr(desc,'\r');
			if(!next) next=strchr(desc,'\n');
			if(next) *next=0;
		}

		// Convert / to | except when preceeded by escape
		char sampstr[1024]={0},*sptr=sampstr;
		bool esc=false;
		for(const char *cptr=ex;*cptr;cptr++)
		{
			if(esc)
			{
				*sptr=*cptr; esc=false; sptr++;
				continue;
			}
			else if(*cptr=='\\')
			{
				esc=true;
				continue;
			}
			else if(*cptr=='/')
				*sptr='|';
			else
				*sptr=*cptr;
			esc=false; sptr++;
		}
		*sptr=0;

		CMDHELP *chelp=0;
		CMDMAP::iterator cit=ahelp->cmdmap.find(cmd);
		if(cit==ahelp->cmdmap.end())
		{
			chelp=new CMDHELP;
			chelp->cmd=cmd;
			chelp->ahelp=ahelp;
			ahelp->cmdmap[cmd]=chelp;
		}
		else
			chelp=cit->second;

		SAMPLE *shelp=new SAMPLE;
		shelp->no=++nsamp;
		shelp->type=stype;
		shelp->ex=sampstr;
		if(desc)
			shelp->desc=desc;
		chelp->samplist.push_back(shelp);
	}
	fclose(fp);
	smc->WSLogEvent("%d samples loaded from \"%s\".",nsamp,fpath);
	for(HELPMAP::iterator ait=csamples.begin();ait!=csamples.end();ait++)
	{
		APPHELP *ahelp=ait->second;
		for(CMDMAP::iterator cit=ahelp->cmdmap.begin();cit!=ahelp->cmdmap.end();cit++)
		{
			CMDHELP *chelp=cit->second;
			for(SAMPLIST::iterator sit=chelp->samplist.begin();sit!=chelp->samplist.end();sit++)
				delete *sit;
			chelp->samplist.clear();
			delete chelp;
		}
		ahelp->cmdmap.clear();
		delete ahelp;
	}
	csamples.clear();
	csamples.swap(nhelp);
	// Refresh the samples
	OnCbnSelchangeCmdlist();
	return;

error:
    fclose(fp);
	MessageBox(estr,"Failed Load",MB_ICONERROR);
	for(HELPMAP::iterator ait=nhelp.begin();ait!=nhelp.end();ait++)
	{
		APPHELP *ahelp=ait->second;
		for(CMDMAP::iterator cit=ahelp->cmdmap.begin();cit!=ahelp->cmdmap.end();cit++)
		{
			CMDHELP *chelp=cit->second;
			for(SAMPLIST::iterator sit=chelp->samplist.begin();sit!=chelp->samplist.end();sit++)
				delete *sit;
			chelp->samplist.clear();
			delete chelp;
		}
		ahelp->cmdmap.clear();
		delete ahelp;
	}
	nhelp.clear();
}


SMConsole::SMConsole()
	:fcodec(0)
	,ccodec(0)
	,rid(0)
	,pdlg(0)
	#ifndef DIALOG_DLLS
	,lvlxAppPath()
	,lvl1Event(0)
	,lvl2Event(0)
	#endif
{
}
// VS2010 error C2600: 'SMConsole::~SMConsole' : cannot define a compiler-generated special member function (must be declared in the class first)
SMConsole::~SMConsole()
{
    delete pcfg; pcfg = 0;
}
int SMConsole::WSHInitModule(AppConfig *config, class WsocksHost *apphost)
{
	NO_OF_CON_PORTS=1;
	if(WSInitSocks()<0)
		return -1;
	if(!ccodec)
	{
		ccodec=new SysCmdCodec;
		if(ccodec->Init()<0)
			return -1;
	}
	if(!fcodec)
	{
		fcodec=new SysFeedCodec;
		if(fcodec->Init()<0)
			return -1;
	}
	return 0;
}
int SMConsole::WSHCleanupModule()
{
	if(ccodec)
	{
		delete ccodec; ccodec=0;
	}
	if(fcodec)
	{
		delete fcodec; fcodec=0;
	}
	WSCloseSocks();
	return 0;
}

int SMConsole::WSPortsCfg()
{
	// One CON port
	WSCreatePort(WS_CON,0);
	strcpy(ConPort[0].AltRemoteIP[0],"127.0.0.1");
	ConPort[0].AltRemoteIPOn[0]=1;
	ConPort[0].AltIPCount=1;
	strcpy(ConPort[0].LocalIP,"127.0.0.1");
	ConPort[0].Port=0;// To be determined, hence ConnectHold=true
	strcpy(ConPort[0].CfgNote,"SMConsole");
	ConPort[0].BlockSize=WS_MAX_BLOCK_SIZE;
	ConPort[0].Compressed=false;
	ConPort[0].ConnectHold=true;
	return 1;
}
void SMConsole::WSConOpened(int PortNo)
{
	WSLogEvent("CON%d connected to %s:%d)...",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
	// Encode messages just like the proxy and client
	char mbuf[1024]={0},*mptr=mbuf;
	char parms[256]={0};
	sprintf(parms,"User=%s|Pass=|","SMConsole");
	ccodec->Encode(mptr,sizeof(mbuf),true,++rid,"login",parms,0);
	WSLogEvent("Cmd> %s",mbuf);
	WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,PortNo);
}
int SMConsole::WSConMsgReady(int PortNo)
{
	if(ConPort[PortNo].InBuffer.Size<1)
		return 0;
	if(ConPort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
		return 0;
	// Apps that support loopback should strip the msg before handling
	// because any new reply will be handled inline
	MSGHEADER MsgHeader;
	memcpy(&MsgHeader,ConPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(ConPort[PortNo].InBuffer.Size<sizeof(MSGHEADER)+MsgHeader.MsgLen)
		return 0;
	char *Msg=new char[MsgHeader.MsgLen];
	memcpy(Msg,ConPort[PortNo].InBuffer.Block +sizeof(MSGHEADER),MsgHeader.MsgLen);
	if(!WSStripBuff(&ConPort[PortNo].InBuffer,sizeof(MSGHEADER) +MsgHeader.MsgLen))
	{
		WSCloseConPort(PortNo);
		return 0;
	}
	TranslateConMsg(&MsgHeader,Msg,PortNo);
	delete Msg;
	//ConPort[PortNo].DetPtr2=(void*)(sizeof(MSGHEADER)+MsgHeader.MsgLen);
	return 1;
}
int SMConsole::WSConStripMsg(int PortNo)
{
	int size=(int)(PTRCAST)ConPort[PortNo].DetPtr2;
	while(size>0)
	{
		int bsize=size;
		if(bsize>ConPort[PortNo].BlockSize*1024)
			bsize=ConPort[PortNo].BlockSize*1024;
		if(!WSStripBuff(&ConPort[PortNo].InBuffer,bsize))
		{
			WSCloseConPort(PortNo);
			return -1;
		}
		size-=bsize;
	}
	return 0;
}
int SMConsole::TranslateConMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	switch(MsgHeader->MsgID)
	{
	case 16:
		ConPort[PortNo].BeatsIn++;
		break;
	// New sysmon response
	case 1109:
	{
		const char *mend=Msg +MsgHeader->MsgLen;
		while(Msg<mend)
		{
			if(ccodec->Decode((const char *&)Msg,(int)(mend -Msg),false,this,(void*)(PTRCAST)MAKELONG(PortNo,WS_CON))<0)
				return -1;
		}
		break;
	}
	// New sysmon feed
	case 1110:
	{
		const char *mend=Msg +MsgHeader->MsgLen;
		while(Msg<mend)
		{
			if(fcodec->Decode((const char *&)Msg,(int)(mend -Msg),this,(void*)(PTRCAST)MAKELONG(PortNo,WS_CON))<0)
				return -1;
		}
		break;
	}
	// File transfer
	case 1111:
	{
		int rid=0;
		char fkey[256]={0};
		char fpath[MAX_PATH]={0};
		char dtid[256]={0},oboi[256]={0};
		int rc=phost->WSHRecvFile(this,Msg,MsgHeader->MsgLen,WS_CON,PortNo,
			&rid,fkey,sizeof(fkey),fpath,MAX_PATH,dtid,sizeof(dtid),oboi,sizeof(oboi));
		#ifdef DIALOG_DLLS
		if(!rc)
		{
			if(pdlg->dlgapi.NotifyFile(oboi,rc,fkey,fpath)<0)
				ShellExecute(pdlg->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
		}
		else if(rc<0)
			pdlg->dlgapi.NotifyFile(oboi,rc,fkey,fpath);
		#else
		if(!rc)
		{
			// Built-in iqserver rendering apps
			if((!strincmp(oboi,"iqclient.",9))||(!strincmp(oboi,"iqserver.",9)))
			{
				if(!_stricmp(fkey,"lvl1.txt"))
					Lvl1Answer(0,fpath);
				else if(!_stricmp(fkey,"lvl2.txt"))
					Lvl2Answer(0,fpath);
				else if(!_stricmp(fkey,"montage.txt"))
					MontageAnswer(0,fpath);
				else
					ShellExecute(pdlg->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
			}
			else
				ShellExecute(pdlg->m_hWnd,"open","notepad.exe",fpath,0,SW_SHOW);
		}
		#endif
		break;
	}
	};
	return 0;
}
int SMConsole::WSBeforeConSend(int PortNo)
{
	return 0;
}
void SMConsole::WSConClosed(int PortNo)
{
	WSLogEvent("CON%d disconnected from %s:%d.",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
}

void SMConsole::NotifyRequest(int reqid, string cmd, const char *parm, int plen, void *udata)
{
	_ASSERT(false);
}
void SMConsole::NotifyResponse(int reqid, string cmd, const char *resp, int rlen, void *udata)
{
	int PortType=HIWORD((int)(PTRCAST)udata);
	int PortNo=LOWORD((int)(PTRCAST)udata);
	if(PortType!=WS_CON)
		return;
	TVMAP tvmap;
	if(ccodec->GetTagValues(tvmap,resp,rlen)<0)
		return;
	// TODO: filter out resp tags we don't really need to see
	if((cmd!="help")&&(cmd!="helpcommon"))
	{
		char lstr[2048]={0},*lptr=lstr;
		sprintf(lptr,"Resp> rid=%d|cmd=%s|",reqid,cmd.c_str()); lptr+=strlen(lptr);
		memcpy(lptr,resp,rlen); lptr+=rlen; // 'resp' isn't null-terminated
		*lptr=0;
		WSLogEvent(lstr);
	}

	char mbuf[1024]={0},*mptr=mbuf;
	char parms[256]={0};
	if(cmd=="login")
	{
		ccodec->Encode(mptr,sizeof(mbuf),true,++rid,"applist","Hist=Y|Live=Y|",0);
		WSLogEvent("Cmd> %s",mbuf);
		pdlg->NotifyApp(0);
		WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,PortNo);
	}
	else if(cmd=="applist")
	{
		string apath=ccodec->GetValue(tvmap,"AppPath");
		if(!apath.empty())
			pdlg->NotifyApp(apath.c_str());
	}
	else if((cmd=="help")||(cmd=="helpcommon"))
	{
		pdlg->NotifyHelp(tvmap,cmd,(char*)resp);
	}
	else if(cmd=="subscribe")
	{
		string feed=ccodec->GetValue(tvmap,"Feed");
		if((feed=="eventlog")||(feed=="errorlog"))
		{
			string rc=ccodec->GetValue(tvmap,"rc");
			// More historic
			if(rc=="+1")
			{
				string begin=ccodec->GetValue(tvmap,"Begin");
				string end=ccodec->GetValue(tvmap,"End");
				char parms[256]={0};
				sprintf(parms,"Feed=%s|Hist=Y|Begin=%s|End=0|",feed.c_str(),end.c_str());
				char mbuf[2048]={0},*mptr=mbuf;
				ccodec->Encode(mptr,sizeof(mbuf),true,++rid,cmd,parms,0);
				WSConSendMsg(1108,(int)(mptr -mbuf),mbuf,PortNo);
			}
			// Got all history
			else if(rc=="0")
			{
				string aclass=ccodec->GetValue(tvmap,"AppClass");
				string aname=ccodec->GetValue(tvmap,"AppName");
				string fkey=ccodec->GetValue(tvmap,"FileKey");
				char fpath[MAX_PATH]={0};
				sprintf(fpath,"%s\\SysmonTempFiles\\%s.%s.%s",pcfg->RunPath.c_str(),aclass.c_str(),aname.c_str(),fkey.c_str());
				ShellExecute(WShWnd,"open","notepad.exe",fpath,0,SW_SHOW);
			}
		}
	}
	#ifdef DIALOG_DLLS
	string apath=ccodec->GetValue(tvmap,"AppPath");
	if(apath.empty())
		apath=ccodec->GetValue(tvmap,"OnBehalfOf");
	pdlg->dlgapi.NotifyResponse(apath,cmd,resp,rlen);
	#else
	// Built-in iqserver rendering apps
	else if(cmd=="lvl1")
	{
		string rc=ccodec->GetValue(tvmap,"rc");
		string reason=ccodec->GetValue(tvmap,"Reason");
		if(rc=="-1")
			Lvl1Answer(-1,reason.c_str());
	}
	else if(cmd=="lvl2")
	{
		string rc=ccodec->GetValue(tvmap,"rc");
		string reason=ccodec->GetValue(tvmap,"Reason");
		if(rc=="-1")
			Lvl2Answer(-1,reason.c_str());
	}
	else if(cmd=="montage")
	{
		string rc=ccodec->GetValue(tvmap,"rc");
		string reason=ccodec->GetValue(tvmap,"Reason");
		if(rc=="-1")
			MontageAnswer(-1,reason.c_str());
	}
	#endif
}

void SMConsole::NotifyLog(const char *aclass, const char *aname, 
	const char *lpath, bool hist, __int64 off, int rectype, const char *buf, int blen, void *udata)
{
	//const char *lname=strrchr(lpath,'\\');
	//if(!lname) lname=strrchr(lpath,'/');
	//if(lname) lname++;
	//else lname=lpath;
	// This is uglier, but more efficient
	const char *lname=0;
	if(*lpath)
	{
		for(lname=lpath +strlen(lpath) -1;(lname>lpath);lname--)
		{
			if((*lname=='/')||(*lname=='\\'))
			{
				lname++;
				break;
			}
		}
	}
	else
		lname=lpath;

	char fpath[MAX_PATH]={0};
	sprintf(fpath,"%s\\SysmonTempFiles\\%s.%s.%s",pcfg->RunPath.c_str(),aclass,aname,lname);
	HANDLE fhnd=CreateFile(fpath,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
	if(fhnd!=INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER loff;
		loff.QuadPart=off;
		SetFilePointer(fhnd,loff.LowPart,&loff.HighPart,FILE_BEGIN);
		DWORD wbytes=0;
		WriteFile(fhnd,buf,blen,&wbytes,0);
		CloseHandle(fhnd);
	}
}
void SMConsole::NotifyLvl1(const char *aclass, const char *aname, 
	WSPortType PortType, int PortNo, bool hist, int rectype, const char *buf, int blen, void *udata)
{
}
void SMConsole::NotifyLvl2(const char *aclass, const char *aname, 
	WSPortType PortType, int PortNo, bool hist, int rectype, const char *buf, int blen, void *udata)
{
}
//void SMConsole::NotifyOrder(const char *aclass, const char *aname, 
//	const char *ordid, bool hist, int rectype, const char *buf, int blen, void *udata)
//{
//}
void SMConsole::NotifyTrademon(const char *aclass, const char *aname, 
	bool hist, int rectype, const char *Domain, const char *TraderId, int NoUpdate, int ECN, int ECN_TYPE, int SeqNo, int ReqType,
	const __int64& foff, const char *buf, int blen, void *udata, const char *rptr, int rlen)
{
}
void SMConsole::NotifyHeartbeat(const char *aclass, const char *aname,
    bool hist, int heartbeat, const char *message, int messageLength, void *udata)
{
}

void SMConsole::WSTimeChange()
{
}

#ifdef DIALOG_DLLS
// SMDlgServerNotify
int SMConsoleDlg::SMDSSendCmd(const char *mbuf, int mlen)
{
	if(!smc)
		return -1;
	smc->WSConSendMsg(1108,mlen,(char*)mbuf,0);
	return 0;
}
void SMConsoleDlg::SMDSLogEvent(const char *fmt, ...)
{
}
void SMConsoleDlg::SMDSLogError(const char *fmt, ...)
{
}
#endif

#ifdef WIN32
WsocksApp * __stdcall SMConsole_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#else
void * __stdcall SMConsole_GetAppInterface(HMODULE dllhnd, WORD version, const char *aclass, const char *aname)
#endif
{
        if(version!=WSHOST_INTERFACE_VERSION)
                return 0;
        if(!_stricmp(aclass,"SMConsole"))
        {
				SMConsole *pmod=new SMConsole;
                pmod->pFreeAppInterface=SMConsole_FreeAppInterface;
                return pmod;
        }
        return 0;
}
#ifdef WIN32
void __stdcall SMConsole_FreeAppInterface(WsocksApp *pmod)
{
#else
void __stdcall SMConsole_FreeAppInterface(void *amod)
{
        WsocksApp *pmod=(WsocksApp *)amod;
#endif
		if(!_stricmp(pmod->pcfg->aclass.c_str(),"SMConsole"))
        {
                delete (SMConsole*)pmod;
        }
}
