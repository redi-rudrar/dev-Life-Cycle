// IQMatrixView.cpp : implementation of the CIQMatrixView class
//

#include "stdafx.h"
#include "IQMatrix.h"

#include "IQMatrixDoc.h"
#include "IQMatrixView.h"
#define APSTUDIO_INVOKED
#include "resource.h"
#include <shlwapi.h>
#include ".\iqmatrixview.h"
#include "wstring.h"
//#include "BroadcastPatchServer.h"
//#include "BroadcastReceiver.h"
//#include "BroadcastTransmitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define WS_NEW_DIALOGS

// CIQMatrixView

IMPLEMENT_DYNCREATE(CIQMatrixView, CView)

BEGIN_MESSAGE_MAP(CIQMatrixView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_SIZE()
//	ON_WM_CLOSE()
//	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CIQMatrixView construction/destruction

CIQMatrixView::CIQMatrixView()
{
	// TODO: add construction code here
	dlgParam=0;
}

CIQMatrixView::~CIQMatrixView()
{
}

BOOL CIQMatrixView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CIQMatrixView drawing

void CIQMatrixView::OnDraw(CDC* /*pDC*/)
{
	CIQMatrixDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CIQMatrixView printing

BOOL CIQMatrixView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CIQMatrixView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CIQMatrixView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CIQMatrixView diagnostics

#ifdef _DEBUG
void CIQMatrixView::AssertValid() const
{
	CView::AssertValid();
}

void CIQMatrixView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CIQMatrixDoc* CIQMatrixView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CIQMatrixDoc)));
	return (CIQMatrixDoc*)m_pDocument;
}
#endif //_DEBUG

#define WSOCKS_EXTERN static

#define NO_OF_CON_PORTS pdoc->pmod->NO_OF_CON_PORTS
#define NO_OF_USC_PORTS pdoc->pmod->NO_OF_USC_PORTS
#define NO_OF_USR_PORTS pdoc->pmod->NO_OF_USR_PORTS
#define NO_OF_FILE_PORTS pdoc->pmod->NO_OF_FILE_PORTS
#define NO_OF_UMC_PORTS pdoc->pmod->NO_OF_UMC_PORTS
#define NO_OF_UMR_PORTS pdoc->pmod->NO_OF_UMR_PORTS
#define NO_OF_CGD_PORTS pdoc->pmod->NO_OF_CGD_PORTS
#define NO_OF_UGC_PORTS pdoc->pmod->NO_OF_UGC_PORTS
#define NO_OF_UGR_PORTS pdoc->pmod->NO_OF_UGR_PORTS
#define NO_OF_CTO_PORTS pdoc->pmod->NO_OF_CTO_PORTS
#define NO_OF_CTI_PORTS pdoc->pmod->NO_OF_CTI_PORTS
#define NO_OF_OTHER_PORTS pdoc->pmod->NO_OF_OTHER_PORTS
#define NO_OF_BRT_PORTS pdoc->pmod->NO_OF_BRT_PORTS
#define NO_OF_BRR_PORTS pdoc->pmod->NO_OF_BRR_PORTS

#define NO_OF_FILPS NO_OF_FILE_PORTS
#define NO_OF_UMCS NO_OF_UMC_PORTS
#define NO_OF_UMRS NO_OF_UMR_PORTS
#define NO_OF_CGDS NO_OF_CGD_PORTS
#define NO_OF_UGRS NO_OF_UGR_PORTS
#define NO_OF_UGCS NO_OF_UGC_PORTS
#define NO_OF_CTOS NO_OF_CTO_PORTS
#define NO_OF_CTIS NO_OF_CTI_PORTS
#define NO_OF_OTHS NO_OF_OTHER_PORTS
#define NO_OF_BRTS NO_OF_BRT_PORTS
#define NO_OF_BRRS NO_OF_BRR_PORTS

// A dynamic dialog item
#define DLGITEM(idx,tlen) \
	DWORD DI ## idx ## style; \
	DWORD DI ## idx ## dwExtendedStyle; \
	short DI ## idx ## x; \
	short DI ## idx ## y; \
	short DI ## idx ## cx; \
	short DI ## idx ## cy; \
	WORD  DI ## idx ## id; \
	WORD  DI ## idx ## Class[2]; \
	WORD  DI ## idx ## Text[ ## tlen ## ]; \
	WORD  DI ## idx ## CreationData;

#define WSH_USER			_APS_NEXT_COMMAND_VALUE  /* Start of Wsocks defines */
#define WSH_LISTBOX			(WSH_USER+1)
#define WSH_LB_CONLIST		(WSH_USER+2)
#define WSH_IDC_PORTNO		(WSH_USER+3)
#define WSH_XFER_STATUS     (WSH_USER+4)
#define WSH_IDC_SESSIONID   (WSH_USER+5)
#define WSH_IDC_GDCFG		(WSH_USER+6)
#define WSH_IDC_EDIT		(WSH_USER+7)
#define WSH_IDC_IP			(WSH_USER+8)

#define WS_LOGERRORS
#define WS_LOG_MAX 500
#define WS_LOG_WIDTH 90

#define WSM_LOGERROR	(WM_USER+1)
#define WSM_LOGEVENT	(WM_USER+2)
#define WSM_LOGDEBUG	(WM_USER+3)
#define WSM_LOGRECOVER	(WM_USER+4)
#define WSM_LOGRETRY	(WM_USER+5)

#define WS_IDC_START_IP		(WSH_USER+100)
#define WS_IDC_END_IP		(WSH_USER+119)
#define WS_IDC_START_ROAM	(WSH_USER+120)
#define WS_IDC_END_ROAM		(WSH_USER+139)
#define WS_IDC_START_HOLD	(WSH_USER+140)
//#define WS_IDC_END_HOLD		(WSH_USER+159)
#define WS_IDC_COMPRESSED	(WSH_USER+141)
#define WS_IDC_CTYPE		(WSH_USER+142)
#define WS_IDC_START_NOTE	(WSH_USER+160)
//#define WS_IDC_END_NOTE		(WSH_USER+179)
#define WS_IDC_START_BOUNCE (WSH_USER+180)
//#define WS_IDC_END_BOUNCE   (WSH_USER+199)
#define WS_IDC_START_BSTART	(WSH_USER+200)
//#define WS_IDC_END_BSTART	(WSH_USER+219)
#define WS_IDC_START_BEND	(WSH_USER+220)
//#define WS_IDC_END_BEND	    (WSH_USER+239)
#define WS_IDC_START_DRIVES	(WSH_USER+240)
//#define WS_IDC_END_DRIVES	(WSH_USER+259)
#define WS_IDC_START_CUR	(WSH_USER+260)
#define WS_IDC_END_CUR		(WSH_USER+279)
#define WS_IDC_DOREC        (WSH_USER+280)
#define WS_IDC_DISCONNECT   (WSH_USER+281)
#define WS_IDC_PROPS        (WSH_USER+282)
#define WS_IDC_PROPLIST     (WSH_USER+283)
#define WS_IDC_RESET        (WSH_USER+284)
#define WS_IDC_COPY			(WSH_USER+285)
#define WSH_IDC_EDITPORTS	(WSH_USER+286)
#define WS_IDC_LINEID		(WSH_USER+287)
#define WS_IDC_LINENAME		(WSH_USER+288)
#define WS_IDC_CLIENTID		(WSH_USER+289)
#define WS_IDC_GENPORTCFG	(WSH_USER+290)
#define WS_IDC_ERRORLOG     (WSH_USER+294)
#define WS_IDC_EVENTLOG     (WSH_USER+295)
#define WS_IDC_SAVETOCSV	(WSH_USER+297)
#define WS_IDC_DUMPHOLD		(WSH_USER+298)
#define WS_IDC_BROWSE_SETUP (WSH_USER+299)
#define WS_IDC_BROWSE_LOGS	(WSH_USER+300)
#define WS_IDC_RELOAD_APP	(WSH_USER+301)
#define WS_IDC_CLEAN_RECORDINGS	(WSH_USER+302)
#define WS_IDC_START_PORT	(WSH_USER+303)
#define WS_IDC_END_PORT		(WSH_USER+322)

#define STATUS_UPDATE 1000

#define WSLogError pdoc->pmod->WSLogError

static char WSConListSizeStr[8][64];
static int WSConListSizeIdx=-1;
#define SS WSConListSize
static const char *WSConListSize(__int64 val)
{
	WSConListSizeIdx++; WSConListSizeIdx%=8;
	char *str=WSConListSizeStr[WSConListSizeIdx];
	*str=0;
	if(val>(1000*1000*1000))
		sprintf(str,"%.1fg",(double)(val/1000/1000)/1000);
	else if(val>(1000*1000))
		sprintf(str,"%.1fm",(double)val/1000/1000);
	else if(val>1000)
		sprintf(str,"%dk",(int)val/1000);
	else
		sprintf(str,"%db",(int)val);
	return str;
}

// Mesage handler for Error Report
static LRESULT CALLBACK cbReport(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbReport(hDlg,message,wParam,lParam);
}
LRESULT CALLBACK CIQMatrixView::cbReport(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT Code;

	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
		{
			Code=HIWORD(wParam);
			switch(Code)
			{
			case LBN_DBLCLK:
				{
					int Item;

					if((Item=(int)::SendMessage(::GetDlgItem(hDlg,WSH_LISTBOX),LB_GETCURSEL,0,0))==LB_ERR)
						break;
					::SendMessage(::GetDlgItem(hDlg,WSH_LISTBOX), LB_DELETESTRING, Item, 0);
					break;
				}
			}
			break;
		}
		// These messages are posted by other threads to write logs,
		// because only the main thread (thread that called ResetErrorCnt)
		// can update the log dialogs
		case WSM_LOGERROR:
			{
				char *pstr = (char *)lParam;
				theApp.WSHostLogError(pstr);
				free(pstr);
			}
			break;
		case WSM_LOGEVENT:
			{
				char *pstr = (char *)lParam;
				theApp.WSHostLogEvent(pstr);
				free(pstr);
			}
			break;
#ifdef WS_DEBUG_LOG
		case WSM_LOGDEBUG:
			{
				char *pstr = (char *)lParam;
				WSLogDebug(pstr);
				free(pstr);
			}
			break;
#endif
#ifdef WS_RECOVER_LOG
		case WSM_LOGRECOVER:
			{
				char *pstr = (char *)lParam;
				WSLogRecover(pstr);
				free(pstr);
			}
			break;
#endif
	}
    return FALSE;
}

static void WSbtrim(char *Value)
{
     char *s, *d;

     d = s = Value;								   /* point d to string base  */
     while (*s && (*s == ' ' || *s == '\"'))       /* skip over first spaces */
          ++s;

	 while ((*d++ = *s++) != '\0');                /* copy until null        */

     d = s = Value;

	 s += (strlen(Value) - 1);
     if (s <= d)
        return;

	 while (s >= d && (*s == '\0' || *s == ' ' || *s == '\"' || *s == '\t' || *s == '\r' || *s == '\n')) {    /* find end of string */
         *s = '\0';
         --s;
     }

}
#define HASPT(r,x,y) \
	((x>=r.left)&&(x<r.right)&&(y>=r.top)&&(y<r.bottom))
static bool RectsOverlap(RECT& r1, RECT& r2)
{
	if ( HASPT(r1, r2.left, r2.top) )
		return true;
	if ( HASPT(r1, r2.left, r2.bottom) )
		return true;
	if ( HASPT(r1, r2.right, r2.top) )
		return true;
	if ( HASPT(r1, r2.right, r2.bottom) )
		return true;
	return false;
}

// Mesage handler for Error & Event Report box.
#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif
LRESULT CIQMatrixView::cbConPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tempstr[1024];
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	CONPORT *ConPort=pdoc->pmod->ConPort;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			ConPort[PortNo].setupDlg=hDlg;
			SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"ConPort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",ConPort[PortNo].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_START_HOLD,ConPort[PortNo].ConnectHold);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,ConPort[PortNo].CfgNote);
			sprintf(Tempstr,"%ld",ConPort[PortNo].Bounce);
			::SetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr);
			sprintf(Tempstr,"%ld",ConPort[PortNo].BounceStart);
			::SetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr);
			sprintf(Tempstr,"%ld",ConPort[PortNo].BounceEnd);
			::SetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_COMPRESSED,ConPort[PortNo].Compressed||ConPort[PortNo].Encrypted);
			if(ConPort[PortNo].Encrypted)
				sprintf(Tempstr,"E%d",ConPort[PortNo].Encrypted);
			else if(ConPort[PortNo].CompressType>0)
				sprintf(Tempstr,"C%d",ConPort[PortNo].CompressType);
			else
				Tempstr[0]=0;
			::SetDlgItemText(hDlg,WS_IDC_CTYPE,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,ConPort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
			int AltIPCount =  ConPort[PortNo].AltIPCount;
			for(int j=0;j<AltIPCount;j++)
			{
				::SetDlgItemText(hDlg,WS_IDC_START_IP+j,ConPort[PortNo].AltRemoteIP[j]);
				sprintf(Tempstr,"%d",ConPort[PortNo].AltRemotePort[j]);
				::SetDlgItemText(hDlg,WS_IDC_START_PORT+j,Tempstr);
				::CheckDlgButton(hDlg,WS_IDC_START_ROAM+j,ConPort[PortNo].AltRemoteIPOn[j]);
				if(ConPort[PortNo].CurrentAltIP==j)
					::CheckDlgButton(hDlg,WS_IDC_START_CUR+j,TRUE);
			}

			// Don't overlap another ConPort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_CON_PORTS; i++ )
			{
				if ( i==PortNo || !ConPort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(ConPort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=::GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Compressed, CType=0, EType=0, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20];
				u_short AltRemotePort[WS_MAX_ALT_PORTS];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Compressed=::IsDlgButtonChecked(hDlg,WS_IDC_COMPRESSED)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_CTYPE,Tempstr,1023);
				if(toupper(Tempstr[0])=='C')
					CType=atol(Tempstr +1);
				else if(toupper(Tempstr[0])=='E')
				{
					EType=atol(Tempstr +1);
					if(EType==2)
						Compressed=false;
				}
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				memset(AltRemotePort,0,WS_MAX_ALT_PORTS*sizeof(u_short));
				int i,j;
				for(i=0,j=0;i<ConPort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					::GetDlgItemText(hDlg,WS_IDC_START_PORT+i,Tempstr,10);
					AltRemotePort[j]=atoi(Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				// See if we must reset
				if(!Reset)
				{
					BOOL needReset=FALSE;
					if((Port!=ConPort[PortNo].Port)||
					   (Hold!=ConPort[PortNo].ConnectHold)||
					   (strcmp(CfgNote,ConPort[PortNo].CfgNote)!=0)||
					   (Current!=ConPort[PortNo].CurrentAltIP)||
					   (Compressed!=ConPort[PortNo].Compressed)||
					   (CType!=ConPort[PortNo].CompressType)||
					   (EType!=ConPort[PortNo].Encrypted))
						needReset=TRUE;
					if((strcmp(AltRemoteIP[Current],ConPort[PortNo].AltRemoteIP[Current])!=0)||
					   (AltRemotePort[Current]!=ConPort[PortNo].AltRemotePort[Current])||
					   (AltRemoteIPOn[Current]!=ConPort[PortNo].AltRemoteIPOn[Current]))
						needReset=TRUE;
					if(needReset)
					{
						sprintf(Tempstr,"Changes require resetting ConPort %d. Continue?",PortNo);
						if(::MessageBox(hDlg,Tempstr,"Confirm Bounce",MB_ICONWARNING|MB_YESNO)!=IDYES)
							return TRUE;
						Reset=TRUE;
						::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
					}
				}

				// Change config
				ConPort[PortNo].Port=Port;
				ConPort[PortNo].ConnectHold=Hold;
				ConPort[PortNo].Bounce=Bounce;
				ConPort[PortNo].BounceStart=BounceStart;
				ConPort[PortNo].BounceEnd=BounceEnd;
				ConPort[PortNo].Compressed=Compressed;
				ConPort[PortNo].CompressType=CType;
				ConPort[PortNo].Encrypted=EType;
				strcpy(ConPort[PortNo].CfgNote,CfgNote);
				strcpy(ConPort[PortNo].Note,ConPort[PortNo].CfgNote);
				if(DoRec!=ConPort[PortNo].Recording.DoRec)
				{
					pdoc->pmod->WSCloseRecording(&ConPort[PortNo].Recording,WS_CON,PortNo);
					ConPort[PortNo].DoRecOnOpen=ConPort[PortNo].Recording.DoRec;
				}
				memcpy(ConPort[PortNo].AltRemoteIPOn,AltRemoteIPOn,WS_MAX_ALT_PORTS*sizeof(int));
				memcpy(ConPort[PortNo].AltRemotePort,AltRemotePort,WS_MAX_ALT_PORTS*sizeof(u_short));
				memcpy(ConPort[PortNo].AltRemoteIP,AltRemoteIP,WS_MAX_ALT_PORTS*20);
				ConPort[PortNo].AltIPCount=AltIPCount;
				if(Reset)
				{
					ConPort[PortNo].ReconnectDelay=ConPort[PortNo].MinReconnectDelay;
					pdoc->pmod->WSCloseConPort(PortNo);
					ConPort[PortNo].CurrentAltIP=Current-1;
				#ifdef WS_DECLINING_RECONNECT
					ConPort[PortNo].ReconnectDelay=0;
					ConPort[PortNo].ReconnectTime=0;
				#endif
					ConPort[PortNo].DoRecOnOpen=DoRec;
				}
				else
				{
					ConPort[PortNo].CurrentAltIP=Current;
					if(DoRec)
					{
						pdoc->pmod->WSOpenRecording(&ConPort[PortNo].Recording,hDlg,WS_CON,PortNo);
						ConPort[PortNo].DoRecOnOpen=ConPort[PortNo].Recording.DoRec;
					}
				}
				ConPort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_CON,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				ConPort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			case WS_IDC_COPY:
			{
				// Read values
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20];
				u_short AltRemotePort[WS_MAX_ALT_PORTS];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				memset(AltRemotePort,0,WS_MAX_ALT_PORTS*sizeof(u_short));
				int i,j;
				for(i=0,j=0;i<ConPort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					::GetDlgItemText(hDlg,WS_IDC_START_PORT+i,Tempstr,10);
					AltRemotePort[j]=atoi(Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				CONPORT tport;
				memcpy(&tport,&ConPort[PortNo],sizeof(CONPORT));
				tport.CurrentAltIP=Current;
				tport.ConnectHold=Hold;
				tport.Bounce=Bounce;
				tport.BounceStart=BounceStart;
				tport.BounceEnd=BounceEnd;
				strcpy(tport.CfgNote,CfgNote);
				WSConGenPortLine(&tport,PortNo,Tempstr,1024);

				// Save the text to the clipboard
				if ( !::OpenClipboard(hDlg) )
				{
					::MessageBox(hDlg,"Failed opening the Clipboard.","Error",MB_ICONERROR);
					return TRUE;
				}
				if ( !EmptyClipboard() )
				{
					::MessageBox(hDlg,"Failed emptying the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				HGLOBAL hData = GlobalAlloc(GHND, strlen(Tempstr)+3);
				if ( !hData )
				{
					::MessageBox(hDlg,"Failed allocating memory for the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				char *cbuf = (char *)GlobalLock(hData);
				sprintf(cbuf, "%s\r\n", Tempstr);
				GlobalUnlock(hData);

				if ( !::SetClipboardData(CF_TEXT, hData) )
				{
					int err = GetLastError();
					::MessageBox(hDlg,"Failed setting the Clipboard data.","Error",MB_ICONERROR);
					GlobalFree(hData);
					CloseClipboard();
					return TRUE;
				}
				CloseClipboard();
				return TRUE;
			}
			default:
				// Uncheck current if deactivated
				if((Code>=WS_IDC_START_ROAM)&&(Code<WS_IDC_END_ROAM))
				{
					if(!::IsDlgButtonChecked(hDlg,Code))
						::CheckDlgButton(hDlg,WS_IDC_START_CUR+(Code-WS_IDC_START_ROAM),FALSE);
				}
				// Only one IP may be current
				if((Code>=WS_IDC_START_CUR)&&(Code<WS_IDC_END_CUR))
				{
					for(UINT i=WS_IDC_START_CUR;i<WS_IDC_END_CUR;i++)
					{
						if(i!=Code)
							::CheckDlgButton(hDlg,i,FALSE);
					}
				}
				// Translate network aliases
				if((Code>=WS_IDC_START_IP)&&(Code<WS_IDC_END_IP))
				{
					int altidx=Code -WS_IDC_START_IP;
					::GetDlgItemText(hDlg,Code,Tempstr,20);
					if(strrcmp(Tempstr," "))
					{
						if(theApp.WSHTranslateAlias(Tempstr,sizeof(Tempstr)))
						{
							::SetDlgItemTextA(hDlg,Code,Tempstr);
							int tlen=(int)strlen(Tempstr);
							::SendMessage((HWND)lParam,EM_SETSEL,tlen,tlen);
						}
					}
				}
				return TRUE;
			}
			break;
		}
	}
    return FALSE;
}
static LRESULT CALLBACK cbConPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbConPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::ConPortSetupBox(int PortNo)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	if(ConPort[PortNo].setupDlg)
	{
		::BringWindowToTop(ConPort[PortNo].setupDlg);
		return;
	}

    typedef struct tdConPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,6)
		DLGITEM(3,5)
		DLGITEM(4,6)
		DLGITEM(5,8)
		DLGITEM(6,8)
		DLGITEM(7,6)
		DLGITEM(22,4)
		DLGITEM(23,6)
		DLGITEM(8,1)
		DLGITEM(9,1)
		DLGITEM(10,1)
		DLGITEM(11,1)
		DLGITEM(12,1)
		DLGITEM(13,1)
		DLGITEM(14,1)
		DLGITEM(24,1)
		DLGITEM(25,1)
		DLGITEM(15,3)
		DLGITEM(16,6)
		DLGITEM(17,6)
		DLGITEM(21,10)
		DLGITEM(18,4)
		DLGITEM(26,6)
		DLGITEM(19,8)
		DLGITEM(20,8)
	} CONPORTSETUPBOXHEADTEMPLATE;
	CONPORTSETUPBOXHEADTEMPLATE	ConPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,26,200,15, 340,43,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Hold",0
                                ,WS_VISIBLE,0,69,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,90,7,70,11,-1,{0xffff,0x0082},L"&Note",0
                                ,WS_VISIBLE,0,160,7,25,11,-1,{0xffff,0x0082},L"&Bounce",0
                                ,WS_VISIBLE,0,190,7,25,11,-1,{0xffff,0x0082},L"B&Start",0
                                ,WS_VISIBLE,0,215,7,25,11,-1,{0xffff,0x0082},L"B&End",0
                                ,WS_VISIBLE,0,240,7,20,11,-1,{0xffff,0x0082},L"Cmp",0
                                ,WS_VISIBLE,0,260,7,25,11,-1,{0xffff,0x0082},L"CType",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_START_HOLD,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,71,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,90,16,65,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,160,16,25,12,WS_IDC_START_BOUNCE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,190,16,20,12,WS_IDC_START_BSTART,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,215,16,20,12,WS_IDC_START_BEND,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,242,16,10,12,WS_IDC_COMPRESSED,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,260,16,20,12,WS_IDC_CTYPE,{0xffff,0x0081},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,285,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,285,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,285,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								,WS_VISIBLE|WS_TABSTOP,0,285,55,50,14,WS_IDC_COPY,{0xffff,0x0080},L"Clipboard",0
								,WS_VISIBLE,0,5,34,25,11,-1,{0xffff,0x0082},L"&IP",0
								,WS_VISIBLE,0,90,34,30,11,-1,{0xffff,0x0082},L"&Port",0
								,WS_VISIBLE,0,125,34,25,11,-1,{0xffff,0x0082},L"&Active",0
								,WS_VISIBLE | WS_TABSTOP,0,155,34,25,11,-1,{0xffff,0x0082},L"Current",0
								};
	typedef struct tdConPortSetupBoxIPTemplate
	{
		DLGITEM(1,1)
		DLGITEM(4,1)
		DLGITEM(2,1)
		DLGITEM(3,1)
	} CONPORTSETUPBOXIPTEMPLATE; 
	CONPORTSETUPBOXIPTEMPLATE ConPortSetupBoxIPTemplate 
							=	{WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,28,80,12,0,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,90,28,30,12,0,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,130,28,10,12,0,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,160,28,10,12,0,{0xffff,0x0080},L"",0};
	
	int AltIPCount =  ConPort[PortNo].AltIPCount;
	LPDLGTEMPLATE ConPortSetupBoxTemplate = (LPDLGTEMPLATE) calloc(sizeof(CONPORTSETUPBOXHEADTEMPLATE)
		+(sizeof(CONPORTSETUPBOXIPTEMPLATE)*(AltIPCount+1)),1);
	memcpy((char *)ConPortSetupBoxTemplate,(char*)&ConPortSetupBoxHeadTemplate,sizeof(CONPORTSETUPBOXHEADTEMPLATE));
	for(int j=0;j<AltIPCount+1;j++)
	{
		if(j<WS_MAX_ALT_PORTS)
		{
			ConPortSetupBoxTemplate->cy += 15;
			ConPortSetupBoxTemplate->cdit += 4;
			ConPortSetupBoxIPTemplate.DI1id= WS_IDC_START_IP + j;
			ConPortSetupBoxIPTemplate.DI1y += 15;
			ConPortSetupBoxIPTemplate.DI4id= WS_IDC_START_PORT + j;
			ConPortSetupBoxIPTemplate.DI4y += 15;
			ConPortSetupBoxIPTemplate.DI2id= WS_IDC_START_ROAM + j;
			ConPortSetupBoxIPTemplate.DI2y += 15;
			ConPortSetupBoxIPTemplate.DI3id= WS_IDC_START_CUR + j;
			ConPortSetupBoxIPTemplate.DI3y += 15;
			memcpy(((char *)ConPortSetupBoxTemplate)+sizeof(CONPORTSETUPBOXHEADTEMPLATE)+j*sizeof(CONPORTSETUPBOXIPTEMPLATE)
				,(char*)&ConPortSetupBoxIPTemplate,sizeof(CONPORTSETUPBOXIPTEMPLATE));
		}
	}
	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, ConPortSetupBoxTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbConPortSetupBox, (LPARAM)this);
	free(ConPortSetupBoxTemplate);
}
#ifdef WS_FILE_SERVER
LRESULT CIQMatrixView::cbFilePortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tempstr[1024];
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			FilePort[PortNo].setupDlg=hDlg;
			SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"FilePort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",FilePort[PortNo].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_START_HOLD,FilePort[PortNo].ConnectHold);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,FilePort[PortNo].CfgNote);
			sprintf(Tempstr,"%ld",FilePort[PortNo].Bounce);
			::SetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr);
			sprintf(Tempstr,"%ld",FilePort[PortNo].BounceStart);
			::SetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr);
			sprintf(Tempstr,"%ld",FilePort[PortNo].BounceEnd);
			::SetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr);
			::SetDlgItemText(hDlg,WS_IDC_START_DRIVES,FilePort[PortNo].DriveList);
			::CheckDlgButton(hDlg,WS_IDC_COMPRESSED,FilePort[PortNo].Compressed);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,FilePort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
			int AltIPCount =  FilePort[PortNo].AltIPCount;
			for(int j=0;j<AltIPCount;j++)
			{
				::SetDlgItemText(hDlg,WS_IDC_START_IP+j,FilePort[PortNo].AltRemoteIP[j]);
				::CheckDlgButton(hDlg,WS_IDC_START_ROAM+j,FilePort[PortNo].AltRemoteIPOn[j]);
				if(FilePort[PortNo].CurrentAltIP==j)
					::CheckDlgButton(hDlg,WS_IDC_START_CUR+j,TRUE);
			}

			// Don't overlap another FilePort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_FILE_PORTS; i++ )
			{
				if ( i==PortNo || !FilePort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(FilePort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Compressed, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20], DriveList[32];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_DRIVES,DriveList,31);
				Compressed=::IsDlgButtonChecked(hDlg,WS_IDC_COMPRESSED)==BST_CHECKED ?TRUE :FALSE;
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				int i,j;
				for(i=0,j=0;i<FilePort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				// See if we must reset
				if(!Reset)
				{
					BOOL needReset=FALSE;
					if((Port!=FilePort[PortNo].Port)||
					   (Hold!=FilePort[PortNo].ConnectHold)||
					   (strcmp(CfgNote,FilePort[PortNo].CfgNote)!=0)||
					   (Current!=FilePort[PortNo].CurrentAltIP)||
					   (Compressed!=FilePort[PortNo].Compressed))
						needReset=TRUE;
					if((strcmp(AltRemoteIP[Current],FilePort[PortNo].AltRemoteIP[Current])!=0)||
					   (AltRemoteIPOn[Current]!=FilePort[PortNo].AltRemoteIPOn[Current]))
						needReset=TRUE;
					if(needReset)
					{
						sprintf(Tempstr,"Changes require resetting FilePort %d. Continue?",PortNo);
						if(::MessageBox(hDlg,Tempstr,"Confirm Bounce",MB_ICONWARNING|MB_YESNO)!=IDYES)
							return TRUE;
						Reset=TRUE;
						::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
					}
				}

				// Change config
				FilePort[PortNo].Port=Port;
				FilePort[PortNo].ConnectHold=Hold;
				FilePort[PortNo].Bounce=Bounce;
				FilePort[PortNo].BounceStart=BounceStart;
				FilePort[PortNo].BounceEnd=BounceEnd;
				strcpy(FilePort[PortNo].DriveList,DriveList);
				FilePort[PortNo].Compressed=Compressed;
				strcpy(FilePort[PortNo].CfgNote,CfgNote);
				strcpy(FilePort[PortNo].Note,FilePort[PortNo].CfgNote);
				if(DoRec!=FilePort[PortNo].Recording.DoRec)
				{
					pdoc->pmod->WSCloseRecording(&FilePort[PortNo].Recording,WS_FIL,PortNo);
					FilePort[PortNo].DoRecOnOpen=FilePort[PortNo].Recording.DoRec;
				}
				memcpy(FilePort[PortNo].AltRemoteIPOn,AltRemoteIPOn,WS_MAX_ALT_PORTS*sizeof(int));
				memcpy(FilePort[PortNo].AltRemoteIP,AltRemoteIP,WS_MAX_ALT_PORTS*20);
				FilePort[PortNo].AltIPCount=AltIPCount;
				if(Reset)
				{
					FilePort[PortNo].ReconnectDelay=FilePort[PortNo].MinReconnectDelay;
					pdoc->pmod->WSCloseFilePort(PortNo);
					FilePort[PortNo].CurrentAltIP=Current-1;
				#ifdef WS_DECLINING_RECONNECT
					FilePort[PortNo].ReconnectDelay=0;
					FilePort[PortNo].ReconnectTime=0;
				#endif
					FilePort[PortNo].DoRecOnOpen=DoRec;
				}
				else
				{
					FilePort[PortNo].CurrentAltIP=Current;
					if(DoRec)
					{
						pdoc->pmod->WSOpenRecording(&FilePort[PortNo].Recording,hDlg,WS_FIL,PortNo);
						FilePort[PortNo].DoRecOnOpen=FilePort[PortNo].Recording.DoRec;
					}
				}
				FilePort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_FIL,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				FilePort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			case WS_IDC_COPY:
			{
				// Read values
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20], DriveList[32];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_DRIVES,DriveList,31);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				int i,j;
				for(i=0,j=0;i<FilePort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				FILEPORT tport;
				memcpy(&tport,&FilePort[PortNo],sizeof(FILEPORT));
				tport.CurrentAltIP=Current;
				tport.ConnectHold=Hold;
				tport.Bounce=Bounce;
				tport.BounceStart=BounceStart;
				tport.BounceEnd=BounceEnd;
				strcpy(tport.CfgNote,CfgNote);
				strcpy(tport.DriveList,DriveList);
				WSFileGenPortLine(&tport,PortNo,Tempstr,1024);

				// Save the text to the clipboard
				if ( !::OpenClipboard(hDlg) )
				{
					::MessageBox(hDlg,"Failed opening the Clipboard.","Error",MB_ICONERROR);
					return TRUE;
				}
				if ( !EmptyClipboard() )
				{
					::MessageBox(hDlg,"Failed emptying the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				HGLOBAL hData = GlobalAlloc(GHND, strlen(Tempstr)+3);
				if ( !hData )
				{
					::MessageBox(hDlg,"Failed allocating memory for the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				char *cbuf = (char *)GlobalLock(hData);
				sprintf(cbuf, "%s\r\n", Tempstr);
				GlobalUnlock(hData);

				if ( !::SetClipboardData(CF_TEXT, hData) )
				{
					int err = GetLastError();
					::MessageBox(hDlg,"Failed setting the Clipboard data.","Error",MB_ICONERROR);
					GlobalFree(hData);
					CloseClipboard();
					return TRUE;
				}
				CloseClipboard();
				return TRUE;
			}
			default:
				// Uncheck current if deactivated
				if((Code>=WS_IDC_START_ROAM)&&(Code<WS_IDC_END_ROAM))
				{
					if(!::IsDlgButtonChecked(hDlg,Code))
						::CheckDlgButton(hDlg,WS_IDC_START_CUR+(Code-WS_IDC_START_ROAM),FALSE);
				}
				// Only one IP may be current
				if((Code>=WS_IDC_START_CUR)&&(Code<WS_IDC_END_CUR))
				{
					for(UINT i=WS_IDC_START_CUR;i<WS_IDC_END_CUR;i++)
					{
						if(i!=Code)
							::CheckDlgButton(hDlg,i,FALSE);
					}
				}
				// Translate network aliases
				if((Code>=WS_IDC_START_IP)&&(Code<WS_IDC_END_IP))
				{
					int altidx=Code -WS_IDC_START_IP;
					::GetDlgItemText(hDlg,Code,Tempstr,20);
					if(strrcmp(Tempstr," "))
					{
						if(theApp.WSHTranslateAlias(Tempstr,sizeof(Tempstr)))
						{
							::SetDlgItemTextA(hDlg,Code,Tempstr);
							int tlen=(int)strlen(Tempstr);
							::SendMessage((HWND)lParam,EM_SETSEL,tlen,tlen);
						}
					}
				}
				return TRUE;
			}
			break;
		}
	}
    return FALSE;
}
LRESULT CALLBACK cbFilePortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbFilePortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::FilePortSetupBox(int PortNo)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	if(FilePort[PortNo].setupDlg)
	{
		::BringWindowToTop(FilePort[PortNo].setupDlg);
		return;
	}

    typedef struct tdFilePortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,6)
		DLGITEM(3,5)
		DLGITEM(4,6)
		DLGITEM(5,8)
		DLGITEM(6,8)
		DLGITEM(7,6)
		DLGITEM(22,8)
		DLGITEM(24,4)
		DLGITEM(8,1)
		DLGITEM(9,1)
		DLGITEM(10,1)
		DLGITEM(11,1)
		DLGITEM(12,1)
		DLGITEM(13,1)
		DLGITEM(14,1)
		DLGITEM(23,1)
		DLGITEM(25,1)
		DLGITEM(15,3)
		DLGITEM(16,6)
		DLGITEM(17,6)
		DLGITEM(21,10)
		DLGITEM(18,4)
		DLGITEM(19,8)
		DLGITEM(20,8)
	} FILEPORTSETUPBOXHEADTEMPLATE;
	FILEPORTSETUPBOXHEADTEMPLATE	FilePortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,25,200,15, 350,43,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Hold",0
                                ,WS_VISIBLE,0,69,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,90,7,70,11,-1,{0xffff,0x0082},L"&Note",0
                                ,WS_VISIBLE,0,160,7,25,11,-1,{0xffff,0x0082},L"&Bounce",0
                                ,WS_VISIBLE,0,190,7,25,11,-1,{0xffff,0x0082},L"B&Start",0
                                ,WS_VISIBLE,0,215,7,25,11,-1,{0xffff,0x0082},L"B&End",0
								,WS_VISIBLE,0,240,7,25,11,-1,{0xffff,0x0082},L"&Drives",0
								,WS_VISIBLE,0,270,7,20,11,-1,{0xffff,0x0082},L"Cmp",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_START_HOLD,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,71,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,90,16,65,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,160,16,25,12,WS_IDC_START_BOUNCE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,190,16,20,12,WS_IDC_START_BSTART,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,215,16,20,12,WS_IDC_START_BEND,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,240,16,25,12,WS_IDC_START_DRIVES,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,272,16,10,12,WS_IDC_COMPRESSED,{0xffff,0x0080},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,295,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,295,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,295,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								,WS_VISIBLE|WS_TABSTOP,0,295,55,50,14,WS_IDC_COPY,{0xffff,0x0080},L"Clipboard",0
								,WS_VISIBLE,0,5,34,25,11,-1,{0xffff,0x0082},L"&IP",0
								,WS_VISIBLE,0,85,34,25,11,-1,{0xffff,0x0082},L"&Active",0
								,WS_VISIBLE | WS_TABSTOP,0,110,34,25,11,-1,{0xffff,0x0082},L"Current",0
								};
	typedef struct tdFilePortSetupBoxIPTemplate
	{
		DLGITEM(1,1)
		DLGITEM(2,1)
		DLGITEM(3,1)
	} FILEPORTSETUPBOXIPTEMPLATE; 
	FILEPORTSETUPBOXIPTEMPLATE FilePortSetupBoxIPTemplate 
							=	{WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,28,80,12,0,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,90,28,10,12,0,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,115,28,10,12,0,{0xffff,0x0080},L"",0};
	
	int AltIPCount =  FilePort[PortNo].AltIPCount;
	LPDLGTEMPLATE FilePortSetupBoxTemplate = (LPDLGTEMPLATE) calloc(sizeof(FILEPORTSETUPBOXHEADTEMPLATE)
		+(sizeof(FILEPORTSETUPBOXIPTEMPLATE)*(AltIPCount+1)),1);
	memcpy((char *)FilePortSetupBoxTemplate,(char*)&FilePortSetupBoxHeadTemplate,sizeof(FILEPORTSETUPBOXHEADTEMPLATE));
	for(int j=0;j<AltIPCount+1;j++)
	{
		if(j<WS_MAX_ALT_PORTS)
		{
			FilePortSetupBoxTemplate->cy += 15;
			FilePortSetupBoxTemplate->cdit += 3;
			FilePortSetupBoxIPTemplate.DI1id= WS_IDC_START_IP + j;
			FilePortSetupBoxIPTemplate.DI1y += 15;
			FilePortSetupBoxIPTemplate.DI2id= WS_IDC_START_ROAM + j;
			FilePortSetupBoxIPTemplate.DI2y += 15;
			FilePortSetupBoxIPTemplate.DI3id= WS_IDC_START_CUR + j;
			FilePortSetupBoxIPTemplate.DI3y += 15;
			memcpy(((char *)FilePortSetupBoxTemplate)+sizeof(FILEPORTSETUPBOXHEADTEMPLATE)+j*sizeof(FILEPORTSETUPBOXIPTEMPLATE)
				,(char*)&FilePortSetupBoxIPTemplate,sizeof(FILEPORTSETUPBOXIPTEMPLATE));
		}
	}
	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, FilePortSetupBoxTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbFilePortSetupBox, (LPARAM)this);
	free(FilePortSetupBoxTemplate);
}
#endif//WS_FILE_SERVER
LRESULT CALLBACK CIQMatrixView::cbUscPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tempstr[1024];
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	USCPORT *UscPort=pdoc->pmod->UscPort;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			UscPort[PortNo].setupDlg=hDlg;
			::SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"UscPort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",UscPort[PortNo].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_START_HOLD,UscPort[PortNo].ConnectHold);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,UscPort[PortNo].CfgNote);
			::CheckDlgButton(hDlg,WS_IDC_COMPRESSED,UscPort[PortNo].Compressed||UscPort[PortNo].Encrypted);
			if(UscPort[PortNo].Encrypted)
				sprintf(Tempstr,"E%d",UscPort[PortNo].Encrypted);
			else if(UscPort[PortNo].CompressType>0)
				sprintf(Tempstr,"C%d",UscPort[PortNo].CompressType);
			else
				Tempstr[0]=0;
			::SetDlgItemText(hDlg,WS_IDC_CTYPE,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,UscPort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,FALSE);

			// Don't overlap another UscPort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_USC_PORTS; i++ )
			{
				if ( i==PortNo || !UscPort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(UscPort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=::GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Compressed, CType=0, EType=0, Current=-1, Reset;
				char Note[80];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,Note,80);
				if(strcmp(Note,UscPort[PortNo].CfgNote)!=0)
					UscPort[PortNo].CfgNoteDirty=true;
				Compressed=::IsDlgButtonChecked(hDlg,WS_IDC_COMPRESSED)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_CTYPE,Tempstr,1023);
				if(toupper(Tempstr[0])=='C')
					CType=atol(Tempstr +1);
				else if(toupper(Tempstr[0])=='E')
				{
					EType=atol(Tempstr +1);
					if(EType==2)
						Compressed=false;
				}
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				if((!Reset)&&(Port!=UscPort[PortNo].Port))
				{
					sprintf(Tempstr,"Changes require resetting UscPort %d. Continue?",PortNo);
					if(::MessageBox(hDlg,Tempstr,"Confirm Bounce",MB_ICONWARNING|MB_YESNO)!=IDYES)
						return TRUE;
					Reset=TRUE;
					::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
				}

				// See if we must reset
				if(!Reset)
				{
					BOOL needReset=FALSE;
					if((Port!=UscPort[PortNo].Port)||
					   (Hold!=UscPort[PortNo].ConnectHold)||
					   (Compressed!=UscPort[PortNo].Compressed)||
					   (CType!=UscPort[PortNo].CompressType)||
					   (EType!=UscPort[PortNo].Encrypted))
						needReset=TRUE;
					if(needReset)
					{
						sprintf(Tempstr,"Changes require resetting UscPort %d. Continue?",PortNo);
						if(::MessageBox(hDlg,Tempstr,"Confirm Bounce",MB_ICONWARNING|MB_YESNO)!=IDYES)
							return TRUE;
						Reset=TRUE;
						::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
					}
				}

				// Change config
				strcpy(UscPort[PortNo].CfgNote,Note);
				UscPort[PortNo].Compressed=Compressed;
				UscPort[PortNo].CompressType=CType;
				UscPort[PortNo].Encrypted=EType;
				if(DoRec!=UscPort[PortNo].Recording.DoRec)
				{
					UscPort[PortNo].Recording.DoRec=DoRec;
					if(DoRec)
						pdoc->pmod->WSLogEvent("Auto-Record USC%d ON",PortNo);
					else
						pdoc->pmod->WSLogEvent("Auto-Record USC%d OFF",PortNo);
				}
				if(Reset)
				{
					UscPort[PortNo].Port=Port;
					UscPort[PortNo].ConnectHold=Hold;
					UscPort[PortNo].Status[0]=0;
					pdoc->pmod->WSCloseUscPort(PortNo);
				}
				UscPort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_USC,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				UscPort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
		}
	}
    return FALSE;
}
static LRESULT CALLBACK cbUscPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbUscPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::UscPortSetupBox(int PortNo)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	if(UscPort[PortNo].setupDlg)
	{
		::BringWindowToTop(UscPort[PortNo].setupDlg);
		return;
	}

    typedef struct tdUscPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,6)
		DLGITEM(3,5)
		DLGITEM(4,9)
		DLGITEM(12,4)
		DLGITEM(13,6)
		DLGITEM(5,1)
		DLGITEM(6,1)
		DLGITEM(7,1)
		DLGITEM(8,1)
		DLGITEM(14,1)
		DLGITEM(15,1)
		DLGITEM(9,3)
		DLGITEM(10,6)
		DLGITEM(11,6)
	} USCPORTSETUPBOXHEADTEMPLATE;
	USCPORTSETUPBOXHEADTEMPLATE	UscPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,15,200,19, 331,62,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Hold",0
                                ,WS_VISIBLE,0,69,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,90,7,70,11,-1,{0xffff,0x0082},L"&CfgNote",0
								,WS_VISIBLE,0,5,32,20,11,-1,{0xffff,0x0082},L"Cmp",0
								,WS_VISIBLE,0,26,32,25,11,-1,{0xffff,0x0082},L"CType",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_START_HOLD,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,72,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,90,16,175,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,8,42,10,12,WS_IDC_COMPRESSED,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,26,42,20,12,WS_IDC_CTYPE,{0xffff,0x0081},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,276,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,276,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,276,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								};

	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPDLGTEMPLATE)&UscPortSetupBoxHeadTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbUscPortSetupBox, (LPARAM)this);
}
LRESULT CIQMatrixView::cbUsrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	char Tempstr[1024];

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			UsrPort[PortNo].setupDlg=hDlg;
			SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"UsrPort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",UscPort[UsrPort[PortNo].UscPort].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,UsrPort[PortNo].Note);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,UsrPort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,FALSE);

			// Don't overlap another UsrPort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_USR_PORTS; i++ )
			{
				if ( i==PortNo || !UsrPort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(UsrPort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, Reset;
				char Note[80];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,Note,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;

				// Change config
				strcpy(UsrPort[PortNo].Note,Note);
				if(DoRec!=UsrPort[PortNo].Recording.DoRec)
					pdoc->pmod->WSCloseRecording(&UsrPort[PortNo].Recording,WS_USR,PortNo);
				if(DoRec)
					pdoc->pmod->WSOpenRecording(&UsrPort[PortNo].Recording,hDlg,WS_USR,PortNo);
				if(Reset)
				{
					pdoc->pmod->WSCloseUsrPort(PortNo);
				}
				UsrPort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_USR,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				UsrPort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
		}
	}
    return FALSE;
}
LRESULT CALLBACK cbUsrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbUsrPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::UsrPortSetupBox(int PortNo)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	if(UsrPort[PortNo].setupDlg)
	{
		::BringWindowToTop(UsrPort[PortNo].setupDlg);
		return;
	}

    typedef struct tdUsrPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,5)
		DLGITEM(3,6)
		DLGITEM(4,1)
		DLGITEM(5,1)
		DLGITEM(6,1)
		DLGITEM(7,3)
		DLGITEM(8,6)
		DLGITEM(9,6)
	} USRPORTSETUPBOXHEADTEMPLATE;
	USRPORTSETUPBOXHEADTEMPLATE	UsrPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,9,200,15, 310,55,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,69,7,70,11,-1,{0xffff,0x0082},L"&Note",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL|ES_READONLY,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,71,16,175,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,255,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,255,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,255,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								};
	
	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPDLGTEMPLATE)&UsrPortSetupBoxHeadTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbUsrPortSetupBox, (LPARAM)this);
}

LRESULT CIQMatrixView::cbCgdPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef WS_GUARANTEED
	char Tempstr[1024];
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			CgdPort[PortNo].setupDlg=hDlg;
			SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"CgdPort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",CgdPort[PortNo].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_START_HOLD,CgdPort[PortNo].ConnectHold);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,CgdPort[PortNo].CfgNote);
			sprintf(Tempstr,"%ld",CgdPort[PortNo].Bounce);
			::SetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr);
			sprintf(Tempstr,"%ld",CgdPort[PortNo].BounceStart);
			::SetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr);
			sprintf(Tempstr,"%ld",CgdPort[PortNo].BounceEnd);
			::CheckDlgButton(hDlg,WS_IDC_COMPRESSED,CgdPort[PortNo].Compressed);
			sprintf(Tempstr,"%ld",CgdPort[PortNo].GDId.LineId);
			::SetDlgItemText(hDlg,WS_IDC_LINEID,Tempstr);
			::SetDlgItemText(hDlg,WS_IDC_LINENAME,CgdPort[PortNo].GDId.LineName);
			::SetDlgItemText(hDlg,WS_IDC_CLIENTID,CgdPort[PortNo].GDId.ClientId);
			::SetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,CgdPort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
			int AltIPCount =  CgdPort[PortNo].AltIPCount;
			for(int j=0;j<AltIPCount;j++)
			{
				::SetDlgItemText(hDlg,WS_IDC_START_IP+j,CgdPort[PortNo].AltRemoteIP[j]);
				::CheckDlgButton(hDlg,WS_IDC_START_ROAM+j,CgdPort[PortNo].AltRemoteIPOn[j]);
				if(CgdPort[PortNo].CurrentAltIP==j)
					::CheckDlgButton(hDlg,WS_IDC_START_CUR+j,TRUE);
			}

			// Don't overlap another CgdPort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_CGD_PORTS; i++ )
			{
				if ( i==PortNo || !CgdPort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(CgdPort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Compressed, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset, LineId;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20], LineName[20], ClientId[40];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Compressed=::IsDlgButtonChecked(hDlg,WS_IDC_COMPRESSED)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_LINEID,Tempstr,1023);
                LineId=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_LINENAME,LineName,19);
				::GetDlgItemText(hDlg,WS_IDC_CLIENTID,ClientId,39);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				int i,j;
				for(i=0,j=0;i<CgdPort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				// See if we must reset
				if(!Reset)
				{
					BOOL needReset=FALSE;
					if((Port!=CgdPort[PortNo].Port)||
					   (Hold!=CgdPort[PortNo].ConnectHold)||
					   (strcmp(CfgNote,CgdPort[PortNo].CfgNote)!=0)||
					   (Current!=CgdPort[PortNo].CurrentAltIP)||
					   (Compressed!=CgdPort[PortNo].Compressed))
						needReset=TRUE;
					if((strcmp(AltRemoteIP[Current],CgdPort[PortNo].AltRemoteIP[Current])!=0)||
					   (AltRemoteIPOn[Current]!=CgdPort[PortNo].AltRemoteIPOn[Current]))
						needReset=TRUE;
					if(needReset)
					{
						sprintf(Tempstr,"Changes require resetting CgdPort %d. Continue?",PortNo);
						if(::MessageBox(hDlg,Tempstr,"Confirm Bounce",MB_ICONWARNING|MB_YESNO)!=IDYES)
							return TRUE;
						Reset=TRUE;
						::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
					}
				}

				// Change config
				CgdPort[PortNo].Port=Port;
				CgdPort[PortNo].ConnectHold=Hold;
				CgdPort[PortNo].Bounce=Bounce;
				CgdPort[PortNo].BounceStart=BounceStart;
				CgdPort[PortNo].BounceEnd=BounceEnd;
				CgdPort[PortNo].Compressed=Compressed;
				CgdPort[PortNo].GDId.LineId=LineId;
				strcpy(CgdPort[PortNo].GDId.LineName,LineName);
				strcpy(CgdPort[PortNo].GDId.ClientId,ClientId);
				strcpy(CgdPort[PortNo].CfgNote,CfgNote);
				strcpy(CgdPort[PortNo].Note,CgdPort[PortNo].CfgNote);
				if(DoRec!=CgdPort[PortNo].Recording.DoRec)
				{
					pdoc->pmod->WSCloseRecording(&CgdPort[PortNo].Recording,WS_CGD,PortNo);
					CgdPort[PortNo].DoRecOnOpen=CgdPort[PortNo].Recording.DoRec;
				}
				memcpy(CgdPort[PortNo].AltRemoteIPOn,AltRemoteIPOn,WS_MAX_ALT_PORTS*sizeof(int));
				memcpy(CgdPort[PortNo].AltRemoteIP,AltRemoteIP,WS_MAX_ALT_PORTS*20);
				CgdPort[PortNo].AltIPCount=AltIPCount;
				if(Reset)
				{
					CgdPort[PortNo].ReconnectDelay=CgdPort[PortNo].MinReconnectDelay;
					pdoc->pmod->WSCloseCgdPort(PortNo);
					CgdPort[PortNo].CurrentAltIP=Current-1;
				#ifdef WS_DECLINING_RECONNECT
					CgdPort[PortNo].ReconnectDelay=0;
					CgdPort[PortNo].ReconnectTime=0;
				#endif
					CgdPort[PortNo].DoRecOnOpen=DoRec;
				}
				else
				{
					CgdPort[PortNo].CurrentAltIP=Current;
					if(DoRec)
					{
						pdoc->pmod->WSOpenRecording(&CgdPort[PortNo].Recording,hDlg,WS_CGD,PortNo);
						CgdPort[PortNo].DoRecOnOpen=CgdPort[PortNo].Recording.DoRec;
					}
				}
				CgdPort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_CGD,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				CgdPort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			case WS_IDC_COPY:
			{
				// Read values
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset, LineId;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20], LineName[20], ClientId[40];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_LINEID,Tempstr,1023);
                LineId=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_LINENAME,LineName,19);
				::GetDlgItemText(hDlg,WS_IDC_CLIENTID,ClientId,39);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				int i,j;
				for(i=0,j=0;i<CgdPort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				CGDPORT tport;
				memcpy(&tport,&CgdPort[PortNo],sizeof(CGDPORT));
				tport.CurrentAltIP=Current;
				tport.ConnectHold=Hold;
				tport.Bounce=Bounce;
				tport.BounceStart=BounceStart;
				tport.BounceEnd=BounceEnd;
				strcpy(tport.CfgNote,CfgNote);
				WSCgdGenPortLine(&tport,PortNo,Tempstr,1024);

				// Save the text to the clipboard
				if ( !::OpenClipboard(hDlg) )
				{
					::MessageBox(hDlg,"Failed opening the Clipboard.","Error",MB_ICONERROR);
					return TRUE;
				}
				if ( !EmptyClipboard() )
				{
					::MessageBox(hDlg,"Failed emptying the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				HGLOBAL hData = GlobalAlloc(GHND, strlen(Tempstr)+3);
				if ( !hData )
				{
					::MessageBox(hDlg,"Failed allocating memory for the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				char *cbuf = (char *)GlobalLock(hData);
				sprintf(cbuf, "%s\r\n", Tempstr);
				GlobalUnlock(hData);

				if ( !::SetClipboardData(CF_TEXT, hData) )
				{
					int err = GetLastError();
					::MessageBox(hDlg,"Failed setting the Clipboard data.","Error",MB_ICONERROR);
					GlobalFree(hData);
					CloseClipboard();
					return TRUE;
				}
				CloseClipboard();
				return TRUE;
			}
			default:
				// Uncheck current if deactivated
				if((Code>=WS_IDC_START_ROAM)&&(Code<WS_IDC_END_ROAM))
				{
					if(!::IsDlgButtonChecked(hDlg,Code))
						::CheckDlgButton(hDlg,WS_IDC_START_CUR+(Code-WS_IDC_START_ROAM),FALSE);
				}
				// Only one IP may be current
				if((Code>=WS_IDC_START_CUR)&&(Code<WS_IDC_END_CUR))
				{
					for(UINT i=WS_IDC_START_CUR;i<WS_IDC_END_CUR;i++)
					{
						if(i!=Code)
							::CheckDlgButton(hDlg,i,FALSE);
					}
				}
				// Translate network aliases
				if((Code>=WS_IDC_START_IP)&&(Code<WS_IDC_END_IP))
				{
					int altidx=Code -WS_IDC_START_IP;
					::GetDlgItemText(hDlg,Code,Tempstr,20);
					if(strrcmp(Tempstr," "))
					{
						if(theApp.WSHTranslateAlias(Tempstr,sizeof(Tempstr)))
						{
							::SetDlgItemTextA(hDlg,Code,Tempstr);
							int tlen=(int)strlen(Tempstr);
							::SendMessage((HWND)lParam,EM_SETSEL,tlen,tlen);
						}
					}
				}
				return TRUE;
			}
			break;
		}
	}
#endif//WS_GUARANTEED
    return FALSE;
}
LRESULT CALLBACK cbCgdPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbCgdPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::CgdPortSetupBox(int PortNo)
{
#ifdef WS_GUARANTEED
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	if(CgdPort[PortNo].setupDlg)
	{
		::BringWindowToTop(CgdPort[PortNo].setupDlg);
		return;
	}

    typedef struct tdCgdPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,6)
		DLGITEM(3,5)
		DLGITEM(4,6)
		DLGITEM(5,8)
		DLGITEM(6,8)
		DLGITEM(7,6)
		DLGITEM(22,8)
		DLGITEM(23,10)
		DLGITEM(24,10)
		DLGITEM(28,4)
		DLGITEM(8,1)
		DLGITEM(9,1)
		DLGITEM(10,1)
		DLGITEM(11,1)
		DLGITEM(12,1)
		DLGITEM(13,1)
		DLGITEM(14,1)
		DLGITEM(25,1)
		DLGITEM(26,1)
		DLGITEM(27,1)
		DLGITEM(29,1)
		DLGITEM(15,3)
		DLGITEM(16,6)
		DLGITEM(17,6)
		DLGITEM(21,10)
		DLGITEM(18,4)
		DLGITEM(19,8)
		DLGITEM(20,8)
	} CGDPORTSETUPBOXHEADTEMPLATE;
	CGDPORTSETUPBOXHEADTEMPLATE	CgdPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,29,200,15, 320,75,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Hold",0
                                ,WS_VISIBLE,0,69,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,90,7,70,11,-1,{0xffff,0x0082},L"&Note",0
                                ,WS_VISIBLE,0,160,7,25,11,-1,{0xffff,0x0082},L"&Bounce",0
                                ,WS_VISIBLE,0,190,7,25,11,-1,{0xffff,0x0082},L"B&Start",0
                                ,WS_VISIBLE,0,215,7,25,11,-1,{0xffff,0x0082},L"B&End",0
								,WS_VISIBLE,0,5,32,25,11,-1,{0xffff,0x0082},L"Line ID",0
                                ,WS_VISIBLE,0,35,32,40,11,-1,{0xffff,0x0082},L"Line Name",0
                                ,WS_VISIBLE,0,80,32,40,11,-1,{0xffff,0x0082},L"Client ID",0
								,WS_VISIBLE,0,240,7,20,11,-1,{0xffff,0x0082},L"Cmp",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_START_HOLD,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,71,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,90,16,65,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,160,16,25,12,WS_IDC_START_BOUNCE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,190,16,20,12,WS_IDC_START_BSTART,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,215,16,20,12,WS_IDC_START_BEND,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,45,25,12,WS_IDC_LINEID,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,35,45,40,12,WS_IDC_LINENAME,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,80,45,40,12,WS_IDC_CLIENTID,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,242,16,10,12,WS_IDC_COMPRESSED,{0xffff,0x0080},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,265,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,265,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,265,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								,WS_VISIBLE|WS_TABSTOP,0,265,55,50,14,WS_IDC_COPY,{0xffff,0x0080},L"Clipboard",0
								,WS_VISIBLE,0,5,61,25,11,-1,{0xffff,0x0082},L"&IP",0
								,WS_VISIBLE,0,85,61,25,11,-1,{0xffff,0x0082},L"&Active",0
								,WS_VISIBLE | WS_TABSTOP,0,110,61,25,11,-1,{0xffff,0x0082},L"Current",0
								};
	typedef struct tdCgdPortSetupBoxIPTemplate
	{
		DLGITEM(1,1)
		DLGITEM(2,1)
		DLGITEM(3,1)
	} CGDPORTSETUPBOXIPTEMPLATE; 
	CGDPORTSETUPBOXIPTEMPLATE CgdPortSetupBoxIPTemplate 
							=	{WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,59,80,12,0,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,90,59,10,12,0,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,115,59,10,12,0,{0xffff,0x0080},L"",0};
	
	int AltIPCount =  CgdPort[PortNo].AltIPCount;
	LPDLGTEMPLATE CgdPortSetupBoxTemplate = (LPDLGTEMPLATE) calloc(sizeof(CGDPORTSETUPBOXHEADTEMPLATE)
		+(sizeof(CGDPORTSETUPBOXIPTEMPLATE)*(AltIPCount+1)),1);
	memcpy((char *)CgdPortSetupBoxTemplate,(char*)&CgdPortSetupBoxHeadTemplate,sizeof(CGDPORTSETUPBOXHEADTEMPLATE));
	for(int j=0;j<AltIPCount+1;j++)
	{
		if(j<WS_MAX_ALT_PORTS)
		{
			CgdPortSetupBoxTemplate->cy += 15;
			CgdPortSetupBoxTemplate->cdit += 3;
			CgdPortSetupBoxIPTemplate.DI1id= WS_IDC_START_IP + j;
			CgdPortSetupBoxIPTemplate.DI1y += 15;
			CgdPortSetupBoxIPTemplate.DI2id= WS_IDC_START_ROAM + j;
			CgdPortSetupBoxIPTemplate.DI2y += 15;
			CgdPortSetupBoxIPTemplate.DI3id= WS_IDC_START_CUR + j;
			CgdPortSetupBoxIPTemplate.DI3y += 15;
			memcpy(((char *)CgdPortSetupBoxTemplate)+sizeof(CGDPORTSETUPBOXHEADTEMPLATE)+j*sizeof(CGDPORTSETUPBOXIPTEMPLATE)
				,(char*)&CgdPortSetupBoxIPTemplate,sizeof(CGDPORTSETUPBOXIPTEMPLATE));
		}
	}
	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, CgdPortSetupBoxTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbCgdPortSetupBox, (LPARAM)this);
	free(CgdPortSetupBoxTemplate);
#endif//WS_GUARANTEED
}

LRESULT CIQMatrixView::cbUgcPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef WS_GUARANTEED
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;

    switch ( message )
    {
    case WM_INITDIALOG:
        {
        int PortNo = (int)lParam;
        char tstr[128];
        sprintf(tstr, "UgcPort: %d", lParam);
        ::SetWindowText(hDlg, tstr);
        sprintf(tstr, "%d", UgcPort[PortNo].Port);
        ::SetDlgItemText(hDlg, WSH_IDC_PORTNO, tstr);
        ::SetDlgItemText(hDlg, WSH_IDC_GDCFG, UgcPort[PortNo].GDCfg);
        ::SetDlgItemText(hDlg, WSH_IDC_SESSIONID, UgcPort[PortNo].SessionId);
		::CheckDlgButton(hDlg,WS_IDC_DOREC,UgcPort[PortNo].Recording.DoRec);
        ::SetWindowLong(hDlg, GWL_USERDATA, PortNo);
        }
        return FALSE;
    case WM_COMMAND:
        switch ( wParam )
        {
        case IDOK:
            {
            int PortNo = GetWindowLong(hDlg, GWL_USERDATA), Compressed, DoRec;
            char cstr[80], sstr[20];
            ::GetDlgItemText(hDlg, WSH_IDC_GDCFG, cstr, sizeof(cstr)); cstr[79]=0;
            ::GetDlgItemText(hDlg, WSH_IDC_SESSIONID, sstr, sizeof(sstr)); sstr[19]=0;
			Compressed=::IsDlgButtonChecked(hDlg,WS_IDC_COMPRESSED)==BST_CHECKED ?TRUE :FALSE;
			DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
            bool doreset = false;
            char lastCfg[128];
            strcpy(lastCfg, UgcPort[PortNo].GDCfg);
            strcpy(UgcPort[PortNo].GDCfg, cstr);
            if(pdoc->pmod->WSCfgUgcPort(PortNo))
            {
                doreset = true;
                WSLogError("UGC Port %d GDCfg changed to (%s)", PortNo, cstr);
            }
            else
            {
                strcpy(UgcPort[PortNo].GDCfg, lastCfg);
                char emsg[1024];
                sprintf(emsg, "Invalid Ugc Port %d configuraion (%s)!", PortNo, UgcPort[PortNo].GDCfg);
				::MessageBox(hDlg, emsg, "Ugc Config", MB_ICONERROR);
                ::SetDlgItemText(hDlg, WSH_IDC_GDCFG, lastCfg);
                return TRUE;
            }
            if ( strcmp(sstr, UgcPort[PortNo].SessionId) )
            {
                strcpy(UgcPort[PortNo].SessionId, sstr);
                doreset = true;
            }
			if(DoRec!=UgcPort[PortNo].Recording.DoRec)
			{
				UgcPort[PortNo].Recording.DoRec=DoRec;
				if(DoRec)
					pdoc->pmod->WSLogEvent("Auto-Record UGC%d ON",PortNo);
				else
					pdoc->pmod->WSLogEvent("Auto-Record UGC%d OFF",PortNo);
			}
			UgcPort[PortNo].Compressed=Compressed;
            if ( doreset )
			{
//                pdoc->pmod->WSResetUgcId(PortNo);
                WSLogError("UGC Port %d SessionId reset to (%s)", PortNo, UgcPort[PortNo].SessionId);
			}
			pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_UGC,PortNo);
			SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
            EndDialog(hDlg, IDOK);
            }
            return TRUE;
        case IDCANCEL:
			SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        case WSH_IDC_EDIT:
            {
            char cfgnote[80], apath[MAX_PATH];
            ::GetDlgItemText(hDlg, WSH_IDC_GDCFG, cfgnote, sizeof(cfgnote));
			memset(apath, 0, sizeof(apath));
            char *aptr = 0;
            GetFullPathName(cfgnote, sizeof(apath), apath, &aptr);
            if ( PathFileExists(apath) )
                ShellExecute(hDlg, "open", apath, 0, 0, SW_SHOW);
            else
            {
                char emsg[1024];
                sprintf(emsg, "\"%s\" doesn't exist. Create it?", apath);
				if ( ::MessageBox(hDlg, emsg, "Confirm Create", MB_ICONQUESTION|MB_YESNO) == IDYES )
                {
                    HANDLE fhnd = CreateFile(apath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, 0, 0);
                    if ( fhnd == INVALID_HANDLE_VALUE )
                    {
                        sprintf(emsg, "Failed creating \"%s\".", apath);
						::MessageBox(hDlg, emsg, "CreateFile Failed", MB_ICONERROR);
                    }
                    else
                    {
                        CloseHandle(fhnd);
                        ShellExecute(hDlg, "edit", apath, 0, 0, SW_SHOW);
                    }
                }
            }
            }
            return TRUE;
        }
        return FALSE;
    }
#endif//WS_GUARANTEED
    return FALSE;
}
LRESULT CALLBACK cbUgcPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbUgcPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::UgcPortSetupBox(int PortNo)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;

    HWND hDlg;
    char Tempstr[1024];
	typedef struct tdUgcPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,8)
		DLGITEM(2,1)
		DLGITEM(3,11)
		DLGITEM(4,1)
		DLGITEM(5,7)
		DLGITEM(6,1)
		DLGITEM(7,6)
		DLGITEM(8,1)
		DLGITEM(12,5)
		DLGITEM(13,1)
		DLGITEM(9,3)
		DLGITEM(10,7)
		DLGITEM(11,9)
	} UGCPORTSETUPBOXHEADTEMPLATE;
	UGCPORTSETUPBOXHEADTEMPLATE	UgcPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME | WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | DS_SETFONT,0,13,5,20,220,92,0,0,0,8,L"MS Sans Serif"
								,WS_VISIBLE,0,16,9,25,8,-1,{0xffff,0x0082},L"PortNo:",0
								,WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY,0,45,9,40,14,WSH_IDC_PORTNO,{0xffff,0x0082},L"",0
								,WS_VISIBLE,0,7,27,34,8,-1,{0xffff,0x0082},L"SessionId:",0
								,WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER,0,45,25,50,14,WSH_IDC_SESSIONID,{0xffff,0x0081},L"",0
								,WS_VISIBLE,0,17,45,24,8,-1,{0xffff,0x0082},L"GDCfg:",0
								,WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER,0,45,43,120,14,WSH_IDC_GDCFG,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE,0,17,63,24,8,-1,{0xffff,0x0082},L"&Rec:",0
								,WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,0,45,63,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
								,WS_VISIBLE,0,17,75,20,11,-1,{0xffff,0x0082},L"Cmp:",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,45,75,10,12,WS_IDC_COMPRESSED,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE | WS_TABSTOP,0,173,7,40,14,IDOK,{0xffff,0x0080},L"OK",0
                                ,WS_VISIBLE | WS_TABSTOP,0,173,24,40,14,IDCANCEL,{0xffff,0x0080},L"Cancel",0
                                ,WS_VISIBLE | WS_TABSTOP,0,173,43,40,14,WSH_IDC_EDIT,{0xffff,0x0080},L"&Edit...",0
                                };

	dlgParam=PortNo;
	hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPDLGTEMPLATE)&UgcPortSetupBoxHeadTemplate, pdoc->pmod->WShConList, (DLGPROC)::cbUgcPortSetupBox, (LPARAM)this);
	sprintf(Tempstr,"UgcPort : %ld",PortNo);
	::SetWindowText(hDlg,Tempstr);
	sprintf(Tempstr,"%ld",UgcPort[PortNo].Port);
	::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
}
LRESULT CIQMatrixView::cbUgrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef WS_GUARANTEED
	char Tempstr[1024];
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			UgrPort[PortNo].setupDlg=hDlg;
			SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"UgrPort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",UgcPort[UgrPort[PortNo].UgcPort].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,UgrPort[PortNo].Note);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,UgrPort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,FALSE);

			// Don't overlap another UgrPort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_UGR_PORTS; i++ )
			{
				if ( i==PortNo || !UgrPort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(UgrPort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=::GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, Reset;
				char Note[80];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,Note,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;

				// Change config
				strcpy(UgrPort[PortNo].Note,Note);
				if(DoRec!=UgrPort[PortNo].Recording.DoRec)
					pdoc->pmod->WSCloseRecording(&UgrPort[PortNo].Recording,WS_UGR,PortNo);
				if(DoRec)
					pdoc->pmod->WSOpenRecording(&UgrPort[PortNo].Recording,hDlg,WS_UGR,PortNo);
				if(Reset)
				{
					pdoc->pmod->WSCloseUgrPort(PortNo);
				}
				UgrPort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_UGR,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				UgrPort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
		}
	}
#endif//WS_GUARANTEED
    return FALSE;
}
LRESULT CALLBACK cbUgrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbUgrPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::UgrPortSetupBox(int PortNo)
{
#ifdef WS_GUARANTEED
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	if(UgrPort[PortNo].setupDlg)
	{
		::BringWindowToTop(UgrPort[PortNo].setupDlg);
		return;
	}

    typedef struct tdUgrPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,5)
		DLGITEM(3,6)
		DLGITEM(4,1)
		DLGITEM(5,1)
		DLGITEM(6,1)
		DLGITEM(7,3)
		DLGITEM(8,6)
		DLGITEM(9,6)
	} UGRPORTSETUPBOXHEADTEMPLATE;
	UGRPORTSETUPBOXHEADTEMPLATE	UgrPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,9,200,15, 310,55,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,69,7,70,11,-1,{0xffff,0x0082},L"&Note",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL|ES_READONLY,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL|ES_READONLY,0,71,16,175,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,255,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,255,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,255,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								};

	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPDLGTEMPLATE)&UgrPortSetupBoxHeadTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbUgrPortSetupBox, (LPARAM)this);
#endif//WS_GUARANTEED
}

LRESULT CIQMatrixView::cbCtiPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	char Tempstr[1024];

	switch (message)
	{
		case WM_INITDIALOG:
		{
			int PortNo = (int)lParam;
			CtiPort[PortNo].setupDlg=hDlg;
			SetWindowLong(hDlg,GWL_USERDATA,PortNo);
			sprintf(Tempstr,"CtiPort: %ld",PortNo);
			::SetWindowText(hDlg,Tempstr);
			sprintf(Tempstr,"%ld",CtiPort[PortNo].Port);
			::SetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_START_HOLD,CtiPort[PortNo].ConnectHold);
			::SetDlgItemText(hDlg,WS_IDC_START_NOTE,CtiPort[PortNo].CfgNote);
			sprintf(Tempstr,"%ld",CtiPort[PortNo].Bounce);
			::SetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr);
			sprintf(Tempstr,"%ld",CtiPort[PortNo].BounceStart);
			::SetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr);
			sprintf(Tempstr,"%ld",CtiPort[PortNo].BounceEnd);
			::SetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr);
			::CheckDlgButton(hDlg,WS_IDC_DOREC,CtiPort[PortNo].Recording.DoRec);
			::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
			int AltIPCount =  CtiPort[PortNo].AltIPCount;
			for(int j=0;j<AltIPCount;j++)
			{
				::SetDlgItemText(hDlg,WS_IDC_START_IP+j,CtiPort[PortNo].AltRemoteIP[j]);
				::CheckDlgButton(hDlg,WS_IDC_START_ROAM+j,CtiPort[PortNo].AltRemoteIPOn[j]);
				if(CtiPort[PortNo].CurrentAltIP==j)
					::CheckDlgButton(hDlg,WS_IDC_START_CUR+j,TRUE);
			}

			// Don't overlap another CtiPort setup dialog
			RECT crect, prect;
			::GetWindowRect(hDlg, &crect);
			::GetWindowRect(::GetDesktopWindow(), &prect);
			int cx = crect.right -crect.left;
			int cy = crect.bottom -crect.top;
			for ( int i=0; i<NO_OF_CTI_PORTS; i++ )
			{
				if ( i==PortNo || !CtiPort[i].setupDlg )
					continue;
				RECT trect;
				::GetWindowRect(CtiPort[i].setupDlg, &trect);
				while ( RectsOverlap(trect, crect) )
				{
					crect.left += 0; crect.top = trect.bottom;
					crect.right += 0; crect.bottom = crect.top +cy;
					// Don't exceed the height of the parent
					if ( crect.bottom >= prect.bottom )
					{
						crect.left += cx; crect.top = prect.top;
						crect.right += cx; crect.bottom = crect.top +cy;
					}
				}
			}
			::MoveWindow(hDlg, crect.left, crect.top, cx, cy, true);
			return TRUE;
		}
		case WM_COMMAND:
		{
			UINT Code=LOWORD(wParam);
			int PortNo=GetWindowLong(hDlg,GWL_USERDATA);
			switch(Code)
			{
			case IDOK:
			{
				// Validate changes first
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				int i,j;
				for(i=0,j=0;i<CtiPort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				// See if we must reset
				if(!Reset)
				{
					BOOL needReset=FALSE;
					if((Port!=CtiPort[PortNo].Port)||
					   (Hold!=CtiPort[PortNo].ConnectHold)||
					   (strcmp(CfgNote,CtiPort[PortNo].CfgNote)!=0)||
					   (Current!=CtiPort[PortNo].CurrentAltIP))
						needReset=TRUE;
					if((strcmp(AltRemoteIP[Current],CtiPort[PortNo].AltRemoteIP[Current])!=0)||
					   (AltRemoteIPOn[Current]!=CtiPort[PortNo].AltRemoteIPOn[Current]))
						needReset=TRUE;
					if(needReset)
					{
						sprintf(Tempstr,"Changes require resetting CtiPort %d. Continue?",PortNo);
						if(::MessageBox(hDlg,Tempstr,"Confirm Bounce",MB_ICONWARNING|MB_YESNO)!=IDYES)
							return TRUE;
						Reset=TRUE;
						::CheckDlgButton(hDlg,WS_IDC_RESET,TRUE);
					}
				}

				// Change config
				CtiPort[PortNo].Port=Port;
				CtiPort[PortNo].ConnectHold=Hold;
				CtiPort[PortNo].Bounce=Bounce;
				CtiPort[PortNo].BounceStart=BounceStart;
				CtiPort[PortNo].BounceEnd=BounceEnd;
				strcpy(CtiPort[PortNo].CfgNote,CfgNote);
				strcpy(CtiPort[PortNo].Note,CtiPort[PortNo].CfgNote);
				if(DoRec!=CtiPort[PortNo].Recording.DoRec)
					pdoc->pmod->WSCloseRecording(&CtiPort[PortNo].Recording,WS_CTI,PortNo);
				if(DoRec)
					pdoc->pmod->WSOpenRecording(&CtiPort[PortNo].Recording,hDlg,WS_CTI,PortNo);
				memcpy(CtiPort[PortNo].AltRemoteIPOn,AltRemoteIPOn,WS_MAX_ALT_PORTS*sizeof(int));
				memcpy(CtiPort[PortNo].AltRemoteIP,AltRemoteIP,WS_MAX_ALT_PORTS*20);
				CtiPort[PortNo].AltIPCount=AltIPCount;
				if(Reset)
				{
					pdoc->pmod->WSCloseCtiPort(PortNo);
					CtiPort[PortNo].CurrentAltIP=Current-1;
				}
				else
					CtiPort[PortNo].CurrentAltIP=Current;
				CtiPort[PortNo].setupDlg=0;
				pdoc->pmod->phost->WSHUpdatePort(pdoc->pmod,WS_CTI,PortNo);
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			case IDCANCEL:
				CtiPort[PortNo].setupDlg=0;
				SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			case WS_IDC_COPY:
			{
				// Read values
				int Port, Hold, DoRec, Bounce, BounceStart, BounceEnd, Current=-1, AltRemoteIPOn[WS_MAX_ALT_PORTS], AltIPCount, Reset;
				char CfgNote[80], AltRemoteIP[WS_MAX_ALT_PORTS][20];
				::GetDlgItemText(hDlg,WSH_IDC_PORTNO,Tempstr,1023);
				Port=atoi(Tempstr);
				Hold=::IsDlgButtonChecked(hDlg,WS_IDC_START_HOLD)==BST_CHECKED ?TRUE :FALSE;
				DoRec=::IsDlgButtonChecked(hDlg,WS_IDC_DOREC)==BST_CHECKED ?TRUE :FALSE;
				::GetDlgItemText(hDlg,WS_IDC_START_NOTE,CfgNote,80);
				::GetDlgItemText(hDlg,WS_IDC_START_BOUNCE,Tempstr,1023);
                Bounce=atol(Tempstr);
				if((Bounce!=0)&&(Bounce<500))
					Bounce=500;
				::GetDlgItemText(hDlg,WS_IDC_START_BSTART,Tempstr,1023);
                BounceStart=atol(Tempstr);
				::GetDlgItemText(hDlg,WS_IDC_START_BEND,Tempstr,1023);
                BounceEnd=atol(Tempstr);
				Reset=::IsDlgButtonChecked(hDlg,WS_IDC_RESET)==BST_CHECKED ?TRUE :FALSE;
				memset(AltRemoteIPOn,0,WS_MAX_ALT_PORTS*sizeof(int));
				memset(AltRemoteIP,0,WS_MAX_ALT_PORTS*20);
				int i,j;
				for(i=0,j=0;i<CtiPort[PortNo].AltIPCount+1;i++)
				{
					::GetDlgItemText(hDlg,WS_IDC_START_IP+i,Tempstr,20);
					WSbtrim(Tempstr);
					if(!Tempstr[0])
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
						{
							::MessageBox(hDlg,"The current connection must not be blank.","Error",MB_ICONERROR);
							return TRUE;
						}
						continue;
					}
					strcpy(AltRemoteIP[j],Tempstr);
					AltRemoteIPOn[j]=::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)==BST_CHECKED ?TRUE :FALSE;
					if(::IsDlgButtonChecked(hDlg,WS_IDC_START_CUR+i)==BST_CHECKED)
					{
						if(::IsDlgButtonChecked(hDlg,WS_IDC_START_ROAM+i)!=BST_CHECKED)
						{
							::MessageBox(hDlg,"The \"Current\" connection must be active.","Error",MB_ICONERROR);
							return TRUE;
						}
						Current=j;
					}
					j++;
				}
				AltIPCount=j;

				CTIPORT tport;
				memcpy(&tport,&CtiPort[PortNo],sizeof(CTIPORT));
				tport.CurrentAltIP=Current;
				tport.ConnectHold=Hold;
				tport.Bounce=Bounce;
				tport.BounceStart=BounceStart;
				tport.BounceEnd=BounceEnd;
				strcpy(tport.CfgNote,CfgNote);
				WSCtiGenPortLine(&tport,PortNo,Tempstr,1024);

				// Save the text to the clipboard
				if ( !::OpenClipboard(hDlg) )
				{
					::MessageBox(hDlg,"Failed opening the Clipboard.","Error",MB_ICONERROR);
					return TRUE;
				}
				if ( !EmptyClipboard() )
				{
					::MessageBox(hDlg,"Failed emptying the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				HGLOBAL hData = GlobalAlloc(GHND, strlen(Tempstr)+3);
				if ( !hData )
				{
					::MessageBox(hDlg,"Failed allocating memory for the Clipboard.","Error",MB_ICONERROR);
					CloseClipboard();
					return TRUE;
				}
				char *cbuf = (char *)GlobalLock(hData);
				sprintf(cbuf, "%s\r\n", Tempstr);
				GlobalUnlock(hData);

				if ( !::SetClipboardData(CF_TEXT, hData) )
				{
					int err = GetLastError();
					::MessageBox(hDlg,"Failed setting the Clipboard data.","Error",MB_ICONERROR);
					GlobalFree(hData);
					CloseClipboard();
					return TRUE;
				}
				CloseClipboard();
				return TRUE;
			}
			default:
				// Uncheck current if deactivated
				if((Code>=WS_IDC_START_ROAM)&&(Code<WS_IDC_END_ROAM))
				{
					if(!::IsDlgButtonChecked(hDlg,Code))
						::CheckDlgButton(hDlg,WS_IDC_START_CUR+(Code-WS_IDC_START_ROAM),FALSE);
				}
				// Only one IP may be current
				if((Code>=WS_IDC_START_CUR)&&(Code<WS_IDC_END_CUR))
				{
					for(UINT i=WS_IDC_START_CUR;i<WS_IDC_END_CUR;i++)
					{
						if(i!=Code)
							::CheckDlgButton(hDlg,i,FALSE);
					}
				}
				// Translate network aliases
				if((Code>=WS_IDC_START_IP)&&(Code<WS_IDC_END_IP))
				{
					int altidx=Code -WS_IDC_START_IP;
					::GetDlgItemText(hDlg,Code,Tempstr,20);
					if(strrcmp(Tempstr," "))
					{
						if(theApp.WSHTranslateAlias(Tempstr,sizeof(Tempstr)))
						{
							::SetDlgItemTextA(hDlg,Code,Tempstr);
							int tlen=(int)strlen(Tempstr);
							::SendMessage((HWND)lParam,EM_SETSEL,tlen,tlen);
						}
					}
				}
				return TRUE;
			}
			break;
		}
	}
    return FALSE;
}
LRESULT CALLBACK cbCtiPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbCtiPortSetupBox(hDlg,message,wParam,lParam);
}
void CIQMatrixView::CtiPortSetupBox(int PortNo)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	if(CtiPort[PortNo].setupDlg)
	{
		::BringWindowToTop(CtiPort[PortNo].setupDlg);
		return;
	}

    typedef struct tdCtiPortSetupBoxHeadTemplate
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
        WORD PointSize;
        WORD Font[14];
		DLGITEM(1,10)
		DLGITEM(2,6)
		DLGITEM(3,5)
		DLGITEM(4,6)
		DLGITEM(5,8)
		DLGITEM(6,8)
		DLGITEM(7,6)
		DLGITEM(8,1)
		DLGITEM(9,1)
		DLGITEM(10,1)
		DLGITEM(11,1)
		DLGITEM(12,1)
		DLGITEM(13,1)
		DLGITEM(14,1)
		DLGITEM(15,3)
		DLGITEM(16,6)
		DLGITEM(17,6)
		DLGITEM(21,10)
		DLGITEM(18,4)
		DLGITEM(19,8)
		DLGITEM(20,8)
	} CTIPORTSETUPBOXHEADTEMPLATE;
	CTIPORTSETUPBOXHEADTEMPLATE	CtiPortSetupBoxHeadTemplate
							=	{DS_MODALFRAME|WS_SYSMENU|WS_VISIBLE|WS_CAPTION|DS_SETFONT,0,21,200,15, 310,43,0,0,0,9,L"MS Sans Serif"
								,WS_VISIBLE,0,5,7,39,11,-1,{0xffff,0x0082},L"TCP &Port",0
                                ,WS_VISIBLE,0,48,7,20,11,-1,{0xffff,0x0082},L"&Hold",0
                                ,WS_VISIBLE,0,69,7,20,11,-1,{0xffff,0x0082},L"&Rec",0
                                ,WS_VISIBLE,0,90,7,70,11,-1,{0xffff,0x0082},L"&Note",0
                                ,WS_VISIBLE,0,160,7,25,11,-1,{0xffff,0x0082},L"&Bounce",0
                                ,WS_VISIBLE,0,190,7,25,11,-1,{0xffff,0x0082},L"B&Start",0
                                ,WS_VISIBLE,0,215,7,25,11,-1,{0xffff,0x0082},L"B&End",0
								,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,16,40,12,WSH_IDC_PORTNO,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,51,16,10,12,WS_IDC_START_HOLD,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,71,16,10,12,WS_IDC_DOREC,{0xffff,0x0080},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,90,16,65,12,WS_IDC_START_NOTE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,160,16,25,12,WS_IDC_START_BOUNCE,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,190,16,20,12,WS_IDC_START_BSTART,{0xffff,0x0081},L"",0
                                ,WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,215,16,20,12,WS_IDC_START_BEND,{0xffff,0x0081},L"",0
								,WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0,255,5,50,14,IDOK,{0xffff,0x0080},L"OK",0
								,WS_VISIBLE|WS_TABSTOP,0,255,23,50,14,IDCANCEL,{0xffff,0x0080},L"Close",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,255,39,50,12,WS_IDC_RESET,{0xffff,0x0080},L"Reset",0
								,WS_VISIBLE|WS_TABSTOP,0,255,55,50,14,WS_IDC_COPY,{0xffff,0x0080},L"Clipboard",0
								,WS_VISIBLE,0,5,34,25,11,-1,{0xffff,0x0082},L"&IP",0
								,WS_VISIBLE,0,85,34,25,11,-1,{0xffff,0x0082},L"&Active",0
								,WS_VISIBLE | WS_TABSTOP,0,110,34,25,11,-1,{0xffff,0x0082},L"Current",0
								};
	typedef struct tdCtiPortSetupBoxIPTemplate
	{
		DLGITEM(1,1)
		DLGITEM(2,1)
		DLGITEM(3,1)
	} CTIPORTSETUPBOXIPTEMPLATE; 
	CTIPORTSETUPBOXIPTEMPLATE CtiPortSetupBoxIPTemplate 
							=	{WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,0,5,28,80,12,0,{0xffff,0x0081},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,90,28,10,12,0,{0xffff,0x0080},L"",0
								,WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX ,0,115,28,10,12,0,{0xffff,0x0080},L"",0};
	
	int AltIPCount =  CtiPort[PortNo].AltIPCount;
	LPDLGTEMPLATE CtiPortSetupBoxTemplate = (LPDLGTEMPLATE) calloc(sizeof(CTIPORTSETUPBOXHEADTEMPLATE)
		+(sizeof(CTIPORTSETUPBOXIPTEMPLATE)*(AltIPCount+1)),1);
	memcpy((char *)CtiPortSetupBoxTemplate,(char*)&CtiPortSetupBoxHeadTemplate,sizeof(CTIPORTSETUPBOXHEADTEMPLATE));
	for(int j=0;j<AltIPCount+1;j++)
	{
		if(j<WS_MAX_ALT_PORTS)
		{
			CtiPortSetupBoxTemplate->cy += 15;
			CtiPortSetupBoxTemplate->cdit += 3;
			CtiPortSetupBoxIPTemplate.DI1id= WS_IDC_START_IP + j;
			CtiPortSetupBoxIPTemplate.DI1y += 15;
			CtiPortSetupBoxIPTemplate.DI2id= WS_IDC_START_ROAM + j;
			CtiPortSetupBoxIPTemplate.DI2y += 15;
			CtiPortSetupBoxIPTemplate.DI3id= WS_IDC_START_CUR + j;
			CtiPortSetupBoxIPTemplate.DI3y += 15;
			memcpy(((char *)CtiPortSetupBoxTemplate)+sizeof(CTIPORTSETUPBOXHEADTEMPLATE)+j*sizeof(CTIPORTSETUPBOXIPTEMPLATE)
				,(char*)&CtiPortSetupBoxIPTemplate,sizeof(CTIPORTSETUPBOXIPTEMPLATE));
		}
	}
	dlgParam=PortNo;
	HWND hDlg=CreateDialogIndirectParam(pdoc->pmod->WShInst, CtiPortSetupBoxTemplate, pdoc->pmod->WShWnd, (DLGPROC)::cbCtiPortSetupBox, (LPARAM)this);
	free(CtiPortSetupBoxTemplate);
}

// Now only returns a connection list name for the port type and number
void CIQMatrixView::GetDispPort(WSPortType Type, int PortNo, char *PortName)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	switch(Type)
	{
		case WS_CON: sprintf(PortName,"C%03d",PortNo);break;
		case WS_CGD: sprintf(PortName,"D%03d",PortNo);break;
		case WS_USC: sprintf(PortName,"U%03d",PortNo);break;
		case WS_UGC: sprintf(PortName,"G%03d",PortNo);break;
		case WS_USR: sprintf(PortName,"U%03d-%03d",UsrPort[PortNo].UscPort,PortNo);break;
#ifdef WS_GUARANTEED
		case WS_UGR: sprintf(PortName,"G%03d-%03d",UgrPort[PortNo].UgcPort,PortNo);break;
#endif
#ifdef WS_MONITOR
		case WS_UMC: sprintf(PortName,"M%03d",PortNo);break;
		case WS_UMR: sprintf(PortName,"M%03d-%03d",UmrPort[PortNo].UmcPort,PortNo);break;
#endif //#ifdef WS_MONITOR
		case WS_FIL: sprintf(PortName,"F%03d",PortNo);break;
		case WS_OTH: sprintf(PortName,"Z%03d",PortNo);break;
		case WS_CON_TOT: sprintf(PortName,"C__T",PortNo);break;
        case WS_CGD_TOT: sprintf(PortName,"D__T",PortNo);break;
		case WS_USR_TOT: sprintf(PortName,"U__T",PortNo);break;
		case WS_UGR_TOT: sprintf(PortName,"G__T",PortNo);break;
        case WS_FIL_TOT: sprintf(PortName,"F__T",PortNo);break;
		case WS_CTO: sprintf(PortName,"O%03d",PortNo);break;
		case WS_CTO_TOT: sprintf(PortName,"O__T",PortNo);break;
		case WS_CTI: sprintf(PortName,"I%03d",PortNo);break;
		case WS_CTI_TOT: sprintf(PortName,"I__T",PortNo);break;
		case WS_BRT: sprintf(PortName,"T%03d",PortNo);break;
		case WS_BRT_TOT: sprintf(PortName,"T__T",PortNo);break;
		case WS_BRR: sprintf(PortName,"R%03d",PortNo);break;
		case WS_BRR_TOT: sprintf(PortName,"R__T",PortNo);break;
		default: sprintf(PortName,"?%03d",PortNo);break;
	}
}

// Mesage handler for Conlist box.
static LRESULT CALLBACK cbConList(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CIQMatrixView *pview=0;
	if(message==WM_INITDIALOG)
	{
		pview=(CIQMatrixView*)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG64)(PTRCAST)pview);
		lParam=pview->dlgParam;
	}
	else
		pview=(CIQMatrixView*)(PTRCAST)GetWindowLongPtr(hDlg,DWLP_USER);
	if(!pview)
		return FALSE;
	return pview->cbConList(hDlg,message,wParam,lParam);
}
LRESULT CALLBACK CIQMatrixView::cbConList(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tempstr[1024];
	UINT Code;
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	if((!pdoc->pmod)||(!pdoc->pmod->WSInitialized()))
		return FALSE;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	switch (message)
	{
		case WM_INITDIALOG:
		{
#if defined(WS_DEBUG_LOG)||defined(WS_RECOVER_LOG)
			SetTimer(hDlg,0x01,1000,0);
#endif
			return TRUE;
		}
		case WM_NOTIFY:
		{
			Code=((LPNMHDR) lParam)->code;
			switch (Code)
			{
			case LVN_GETDISPINFO:
				{
					WSPortType Type=(WSPortType)HIWORD(((NMLVDISPINFO*)lParam)->item.lParam);
					int PortNo=LOWORD(((NMLVDISPINFO*)lParam)->item.lParam);
					char PortName[32]={0};
					GetDispPort(Type,PortNo,PortName);

					switch (((NMLVDISPINFO*)lParam)->item.iSubItem)
					{
					case 0:
						((NMLVDISPINFO*)lParam)->item.pszText=PortName;
						break;
					case 1:
						switch (Type)
						{
							case WS_CON:
								sprintf(Tempstr,"%s:%d",ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USC:
								if(UscPort[PortNo].Port)
									sprintf(Tempstr,"%s:%d",UscPort[PortNo].LocalIP,UscPort[PortNo].Port);
								else
									sprintf(Tempstr,"%s:[%d]",UscPort[PortNo].LocalIP,UscPort[PortNo].bindPort);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USR:
								sprintf(Tempstr,"%s:%d",UsrPort[PortNo].RemoteIP,UscPort[UsrPort[PortNo].UscPort].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								sprintf(Tempstr,"%s:%d",CgdPort[PortNo].RemoteIP,CgdPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGC:
								if(UgcPort[PortNo].Port)
									sprintf(Tempstr,"%s:%d",UgcPort[PortNo].LocalIP,UgcPort[PortNo].Port);
								else
									sprintf(Tempstr,"%s:[%d]",UgcPort[PortNo].LocalIP,UgcPort[PortNo].bindPort);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGR:
								sprintf(Tempstr,"%s:%d",UgrPort[PortNo].RemoteIP,UgcPort[UgrPort[PortNo].UgcPort].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								if(UmcPort[PortNo].Port)
									sprintf(Tempstr,"%s:%d",UmcPort[PortNo].LocalIP,UmcPort[PortNo].Port);
								else
									sprintf(Tempstr,"%s:[%d]",UmcPort[PortNo].LocalIP,UmcPort[PortNo].bindPort);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UMR:
								sprintf(Tempstr,"%s:%d",UmrPort[PortNo].RemoteIP,UmcPort[UmrPort[PortNo].UmcPort].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								if(!strcmp(CtoPort[PortNo].RemoteIP,"255.255.255.255"))
									sprintf(Tempstr,"[%s]*:%d",CtoPort[PortNo].LocalIP,CtoPort[PortNo].Port);
								else
									sprintf(Tempstr,"%s:%d",CtoPort[PortNo].RemoteIP,CtoPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_CTI:
								if(!strcmp(CtiPort[PortNo].RemoteIP,"0.0.0.0"))
									sprintf(Tempstr,"[%s]*:%d",CtiPort[PortNo].LocalIP,CtiPort[PortNo].Port);
								else
									sprintf(Tempstr,"%s:%d",CtiPort[PortNo].RemoteIP,CtiPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
                            case WS_FIL:
								sprintf(Tempstr,"%s:%d",FilePort[PortNo].RemoteIP,FilePort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								sprintf(Tempstr,"%s:%d",OtherPort[PortNo].RemoteIP,OtherPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif
#ifdef WS_BROADCAST_TRANSMITTER
                            case WS_BRT:
								sprintf(Tempstr,"%s:%d",BrtPort[PortNo].RemoteIP,BrtPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
                                break;
#endif
#ifdef WS_BROADCAST_RECEIVER
                            case WS_BRR:
                                sprintf(Tempstr,"%s:%d",BrrPort[PortNo].RemoteIP,BrrPort[PortNo].Port);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
                                break;
#endif
						}
						break;
					case 2:
						switch (Type)
						{
							case WS_USR:
								if(UsrPort[PortNo].ClientIP[0])
								{
									sprintf(Tempstr,"%s:%d",UsrPort[PortNo].ClientIP,UsrPort[PortNo].ClientPort);
									((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								}
								else
									((NMLVDISPINFO*)lParam)->item.pszText="";
								break;
						}
						break;
					case 3:
						switch (Type)
						{
							case WS_CON:
								((NMLVDISPINFO*)lParam)->item.pszText=ConPort[PortNo].Note;
								break;
							case WS_USC:
								((NMLVDISPINFO*)lParam)->item.pszText=UscPort[PortNo].CfgNote;
								break;
							case WS_USR:
								((NMLVDISPINFO*)lParam)->item.pszText=UsrPort[PortNo].Note;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								((NMLVDISPINFO*)lParam)->item.pszText=CgdPort[PortNo].Note;
								break;
							case WS_UGC:
								((NMLVDISPINFO*)lParam)->item.pszText=UgcPort[PortNo].GDCfg;
								break;
							case WS_UGR:
								((NMLVDISPINFO*)lParam)->item.pszText=UgrPort[PortNo].Note;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								((NMLVDISPINFO*)lParam)->item.pszText=UmcPort[PortNo].CfgNote;
								break;
							case WS_UMR:
								((NMLVDISPINFO*)lParam)->item.pszText=UmrPort[PortNo].Note;
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								((NMLVDISPINFO*)lParam)->item.pszText=CtoPort[PortNo].Note;
								break;
							case WS_CTI:
								((NMLVDISPINFO*)lParam)->item.pszText=CtiPort[PortNo].Note;
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
                            case WS_FIL:
								((NMLVDISPINFO*)lParam)->item.pszText=FilePort[PortNo].Note;
								break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								((NMLVDISPINFO*)lParam)->item.pszText=OtherPort[PortNo].Note;
								break;
#endif
#ifdef WS_BROADCAST_TRANSMITTER
                            case WS_BRT:
								((NMLVDISPINFO*)lParam)->item.pszText=BrtPort[PortNo].CfgNote;
                                break;
#endif
#ifdef WS_BROADCAST_RECEIVER
                            case WS_BRR:
								((NMLVDISPINFO*)lParam)->item.pszText=BrrPort[PortNo].CfgNote;
                                break;
#endif
						}
						break;
					case 4:
						switch (Type)
						{
							case WS_CON:
								((NMLVDISPINFO*)lParam)->item.pszText=ConPort[PortNo].Status;
								break;
							case WS_CON_TOT:
								((NMLVDISPINFO*)lParam)->item.pszText=ConPort[NO_OF_CON_PORTS].Status;
								break;
							case WS_USC:
								((NMLVDISPINFO*)lParam)->item.pszText=UscPort[PortNo].Status;
								break;
							case WS_USR:
								((NMLVDISPINFO*)lParam)->item.pszText=UsrPort[PortNo].Status;
								break;
							case WS_USR_TOT:
								((NMLVDISPINFO*)lParam)->item.pszText=UscPort[NO_OF_USC_PORTS].Status;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								((NMLVDISPINFO*)lParam)->item.pszText=CgdPort[PortNo].Status;
								break;
							case WS_UGC:
//								((NMLVDISPINFO*)lParam)->item.pszText=UgcPort[PortNo].GDId.SessionId;
								break;
							case WS_UGR:
								((NMLVDISPINFO*)lParam)->item.pszText=UgrPort[PortNo].Status;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								((NMLVDISPINFO*)lParam)->item.pszText=UmcPort[PortNo].Status;
								break;
							case WS_UMR:
								((NMLVDISPINFO*)lParam)->item.pszText=UmrPort[PortNo].Status;
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								((NMLVDISPINFO*)lParam)->item.pszText=CtoPort[PortNo].Status;
								break;
							case WS_CTI:
								((NMLVDISPINFO*)lParam)->item.pszText=CtiPort[PortNo].Status;
								break;
							case WS_CTI_TOT:
								((NMLVDISPINFO*)lParam)->item.pszText=CtiPort[NO_OF_CTI_PORTS].Status;
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
							case WS_FIL:
								((NMLVDISPINFO*)lParam)->item.pszText=FilePort[PortNo].Status;
								break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								((NMLVDISPINFO*)lParam)->item.pszText=OtherPort[PortNo].Status;
								break;
#endif
#ifdef WS_BROADCAST_TRANSMITTER
                            case WS_BRT_TOT:
								((NMLVDISPINFO*)lParam)->item.pszText=BrtPort[NO_OF_BRT_PORTS].Status;
                                break;
                            case WS_BRT:
								((NMLVDISPINFO*)lParam)->item.pszText=BrtPort[PortNo].Status;
                                break;
#endif
#ifdef WS_BROADCAST_RECEIVER
                            case WS_BRR_TOT:
								((NMLVDISPINFO*)lParam)->item.pszText=BrrPort[NO_OF_BRR_PORTS].Status;
                                break;
                            case WS_BRR:
								((NMLVDISPINFO*)lParam)->item.pszText=BrrPort[PortNo].Status;
                                break;
#endif
						}
						break;
					case 5:
						switch (Type)
						{
							case WS_CON:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,ConPort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(ConPort[PortNo].InBuffer.Size +ConPort[PortNo].TBufferSize)
									#else
									,SS(ConPort[PortNo].InBuffer.Size)
									#endif
									,SS(ConPort[PortNo].BytesPerSecIn)
									,SS(ConPort[PortNo].BytesPerSecAvgIn)
									,SS(ConPort[PortNo].BytesPerSecMaxIn)
									,ConPort[PortNo].BlocksIn
									,ConPort[PortNo].PacketsIn
									,SS(ConPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USC:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UscPort[PortNo].BeatsIn
									,SS(UscPort[PortNo].InBuffer_Size)
									,SS(UscPort[PortNo].BytesPerSecIn)
									,SS(UscPort[PortNo].BytesPerSecAvgIn)
									,SS(UscPort[PortNo].BytesPerSecMaxIn)
									,UscPort[PortNo].BlocksIn
									,UscPort[PortNo].PacketsIn
									,SS(UscPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UsrPort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(UsrPort[PortNo].InBuffer.Size +UsrPort[PortNo].TBufferSize)
									#else
									,SS(UsrPort[PortNo].InBuffer.Size)
									#endif
									,SS(UsrPort[PortNo].BytesPerSecIn)
									,SS(UsrPort[PortNo].BytesPerSecAvgIn)
									,SS(UsrPort[PortNo].BytesPerSecMaxIn)
									,UsrPort[PortNo].BlocksIn
									,UsrPort[PortNo].PacketsIn
									,SS(UsrPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CgdPort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(CgdPort[PortNo].InBuffer.Size +CgdPort[PortNo].TBufferSize)
									#else
									,SS(CgdPort[PortNo].InBuffer.Size)
									#endif
									,SS(CgdPort[PortNo].BytesPerSecIn)
									,SS(CgdPort[PortNo].BytesPerSecAvgIn)
									,SS(CgdPort[PortNo].BytesPerSecMaxIn)
									,CgdPort[PortNo].BlocksIn
									,CgdPort[PortNo].PacketsIn
									,SS(CgdPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGC:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UgcPort[PortNo].BeatsIn
									,SS(UgcPort[PortNo].InBuffer_Size)
									,SS(UgcPort[PortNo].BytesPerSecIn)
									,SS(UgcPort[PortNo].BytesPerSecAvgIn)
									,SS(UgcPort[PortNo].BytesPerSecMaxIn)
									,UgcPort[PortNo].BlocksIn
									,UgcPort[PortNo].PacketsIn
									,SS(UgcPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UgrPort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(UgrPort[PortNo].InBuffer.Size +UgrPort[PortNo].TBufferSize)
									#else
									,SS(UgrPort[PortNo].InBuffer.Size)
									#endif
									,SS(UgrPort[PortNo].BytesPerSecIn)
									,SS(UgrPort[PortNo].BytesPerSecAvgIn)
									,SS(UgrPort[PortNo].BytesPerSecMaxIn)
									,UgrPort[PortNo].BlocksIn
									,UgrPort[PortNo].PacketsIn
									,SS(UgrPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UmcPort[PortNo].BeatsIn
									,SS(UmcPort[PortNo].InBuffer_Size)
									,SS(UmcPort[PortNo].BytesPerSecIn)
									,SS(UmcPort[PortNo].BytesPerSecAvgIn)
									,SS(UmcPort[PortNo].BytesPerSecMaxIn)
									,UmcPort[PortNo].BlocksIn
									,UmcPort[PortNo].PacketsIn
									,SS(UmcPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UMR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UmrPort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(UmrPort[PortNo].InBuffer.Size +UmrPort[PortNo].TBufferSize)
									#else
									,SS(UmrPort[PortNo].InBuffer.Size)
									#endif
									,SS(UmrPort[PortNo].BytesPerSecIn)
									,SS(UmrPort[PortNo].BytesPerSecAvgIn)
									,SS(UmrPort[PortNo].BytesPerSecMaxIn)
									,UmrPort[PortNo].BlocksIn
									,UmrPort[PortNo].PacketsIn
									,SS(UmrPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTI:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CtiPort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(CtiPort[PortNo].InBuffer.Size +CtiPort[PortNo].TBufferSize)
									#else
									,SS(CtiPort[PortNo].InBuffer.Size)
									#endif
									,SS(CtiPort[PortNo].BytesPerSecIn)
									,SS(CtiPort[PortNo].BytesPerSecAvgIn)
									,SS(CtiPort[PortNo].BytesPerSecMaxIn)
									,CtiPort[PortNo].BlocksIn
									,CtiPort[PortNo].PacketsIn
									,SS(CtiPort[PortNo].BytesIn)
									);
								CtiPort[PortNo].PacketsIn=0;
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_CTI_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CtiPort[NO_OF_CTI_PORTS].BeatsIn
									,SS(CtiPort[NO_OF_CTI_PORTS].InBuffer.Size)
									,SS(CtiPort[NO_OF_CTI_PORTS].BytesPerSecIn)
									,SS(CtiPort[NO_OF_CTI_PORTS].BytesPerSecAvgIn)
									,SS(CtiPort[NO_OF_CTI_PORTS].BytesPerSecMaxIn)
									,CtiPort[NO_OF_CTI_PORTS].BlocksIn
									,CtiPort[NO_OF_CTI_PORTS].PacketsIn
									,SS(CtiPort[NO_OF_CTI_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
							case WS_FIL:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,FilePort[PortNo].BeatsIn
									#ifdef WS_OTPP
									,SS(FilePort[PortNo].InBuffer.Size +FilePort[PortNo].TBufferSize)
									#else
									,SS(FilePort[PortNo].InBuffer.Size)
									#endif
									,SS(FilePort[PortNo].BytesPerSecIn)
									,SS(FilePort[PortNo].BytesPerSecAvgIn)
									,SS(FilePort[PortNo].BytesPerSecMaxIn)
									,FilePort[PortNo].BlocksIn
									,FilePort[PortNo].PacketsIn
									,SS(FilePort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								((NMLVDISPINFO*)lParam)->item.pszText=OtherPort[PortNo].Recieved;
								break;
#endif
							case WS_CON_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,ConPort[NO_OF_CON_PORTS].BeatsIn
									,SS(ConPort[NO_OF_CON_PORTS].InBuffer.Size)
									,SS(ConPort[NO_OF_CON_PORTS].BytesPerSecIn)
									,SS(ConPort[NO_OF_CON_PORTS].BytesPerSecAvgIn)
									,SS(ConPort[NO_OF_CON_PORTS].BytesPerSecMaxIn)
									,ConPort[NO_OF_CON_PORTS].BlocksIn
									,ConPort[NO_OF_CON_PORTS].PacketsIn
									,SS(ConPort[NO_OF_CON_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USR_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UsrPort[NO_OF_USR_PORTS].BeatsIn
									,SS(UsrPort[NO_OF_USR_PORTS].InBuffer.Size)
									,SS(UsrPort[NO_OF_USR_PORTS].BytesPerSecIn)
									,SS(UsrPort[NO_OF_USR_PORTS].BytesPerSecAvgIn)
									,SS(UsrPort[NO_OF_USR_PORTS].BytesPerSecMaxIn)
									,UsrPort[NO_OF_USR_PORTS].BlocksIn
									,UsrPort[NO_OF_USR_PORTS].PacketsIn
									,SS(UsrPort[NO_OF_USR_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CgdPort[NO_OF_CGD_PORTS].BeatsIn
									,SS(CgdPort[NO_OF_CGD_PORTS].InBuffer.Size)
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesPerSecIn)
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesPerSecAvgIn)
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesPerSecMaxIn)
									,CgdPort[NO_OF_CGD_PORTS].BlocksIn
									,CgdPort[NO_OF_CGD_PORTS].PacketsIn
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGR_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UgrPort[NO_OF_UGR_PORTS].BeatsIn
									,SS(UgrPort[NO_OF_UGR_PORTS].InBuffer.Size)
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesPerSecIn)
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesPerSecAvgIn)
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesPerSecMaxIn)
									,UgrPort[NO_OF_UGR_PORTS].BlocksIn
									,UgrPort[NO_OF_UGR_PORTS].PacketsIn
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_FILE_SERVER
							case WS_FIL_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,FilePort[NO_OF_FILE_PORTS].BeatsIn
									,SS(FilePort[NO_OF_FILE_PORTS].InBuffer.Size)
									,SS(FilePort[NO_OF_FILE_PORTS].BytesPerSecIn)
									,SS(FilePort[NO_OF_FILE_PORTS].BytesPerSecAvgIn)
									,SS(FilePort[NO_OF_FILE_PORTS].BytesPerSecMaxIn)
									,FilePort[NO_OF_FILE_PORTS].BlocksIn
									,FilePort[NO_OF_FILE_PORTS].PacketsIn
									,SS(FilePort[NO_OF_FILE_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif
#ifdef WS_BROADCAST_RECEIVER
                            case WS_BRR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,BrrPort[PortNo].BeatsIn
									,SS(BrrPort[PortNo].Depth)
									,SS(BrrPort[PortNo].BytesPerSecIn)
									,SS(BrrPort[PortNo].BytesPerSecAvgIn)
									,SS(BrrPort[PortNo].BytesPerSecMaxIn)
									,BrrPort[PortNo].BlocksIn
									,BrrPort[PortNo].PacketsIn
									,SS(BrrPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
                            case WS_BRR_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,BrrPort[NO_OF_BRR_PORTS].BeatsIn
									,SS(BrrPort[NO_OF_BRR_PORTS].Depth)
									,SS(BrrPort[NO_OF_BRR_PORTS].BytesPerSecIn)
									,SS(BrrPort[NO_OF_BRR_PORTS].BytesPerSecAvgIn)
									,SS(BrrPort[NO_OF_BRR_PORTS].BytesPerSecMaxIn)
									,BrrPort[NO_OF_BRR_PORTS].BlocksIn
									,BrrPort[NO_OF_BRR_PORTS].PacketsIn
									,SS(BrrPort[NO_OF_BRR_PORTS].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //WS_BROADCAST_RECEIVER
#ifdef WS_BROADCAST_TRANSMITTER
                            case WS_BRT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,BrtPort[PortNo].BeatsIn
									,SS(0)
									,SS(BrtPort[PortNo].BytesPerSecIn)
									,SS(BrtPort[PortNo].BytesPerSecAvgIn)
									,SS(BrtPort[PortNo].BytesPerSecMaxIn)
									,BrtPort[PortNo].BlocksIn
									,BrtPort[PortNo].PacketsIn
									,SS(BrtPort[PortNo].BytesIn)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //WS_BROADCAST_TRANSMITTER
						}
						break;
					case 6:
						switch (Type)
						{
							case WS_CON:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,ConPort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(ConPort[PortNo].OutBuffer.Size +ConPort[PortNo].IOCPSendBytes)
									#else
									,SS(ConPort[PortNo].OutBuffer.Size)
									#endif
									,SS(ConPort[PortNo].BytesPerSecOut)
									,SS(ConPort[PortNo].BytesPerSecAvgOut)
									,SS(ConPort[PortNo].BytesPerSecMaxOut)
									,ConPort[PortNo].BlocksOut
									,ConPort[PortNo].PacketsOut
									,SS(ConPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USC:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UscPort[PortNo].BeatsOut
									,SS(UscPort[PortNo].OutBuffer_Size)
									,SS(UscPort[PortNo].BytesPerSecOut)
									,SS(UscPort[PortNo].BytesPerSecAvgOut)
									,SS(UscPort[PortNo].BytesPerSecMaxOut)
									,UscPort[PortNo].BlocksOut
									,UscPort[PortNo].PacketsOut
									,SS(UscPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UsrPort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(UsrPort[PortNo].OutBuffer.Size +UsrPort[PortNo].IOCPSendBytes)
									#elif defined(WS_COMPLETION_PORT_SENDS)
									,SS(UsrPort[PortNo].IOCPSendBytes)
									#else
									,SS(UsrPort[PortNo].OutBuffer.Size)
									#endif
									,SS(UsrPort[PortNo].BytesPerSecOut)
									,SS(UsrPort[PortNo].BytesPerSecAvgOut)
									,SS(UsrPort[PortNo].BytesPerSecMaxOut)
									,UsrPort[PortNo].BlocksOut
									,UsrPort[PortNo].PacketsOut
									,SS(UsrPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CgdPort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(CgdPort[PortNo].OutBuffer.Size +CgdPort[PortNo].IOCPSendBytes)
									#else
									,SS(CgdPort[PortNo].OutBuffer.Size)
									#endif
									,SS(CgdPort[PortNo].BytesPerSecOut)
									,SS(CgdPort[PortNo].BytesPerSecAvgOut)
									,SS(CgdPort[PortNo].BytesPerSecMaxOut)
									,CgdPort[PortNo].BlocksOut
									,CgdPort[PortNo].PacketsOut
									,SS(CgdPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGC:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UgcPort[PortNo].BeatsOut
									,SS(UgcPort[PortNo].OutBuffer_Size)
									,SS(UgcPort[PortNo].BytesPerSecOut)
									,SS(UgcPort[PortNo].BytesPerSecAvgOut)
									,SS(UgcPort[PortNo].BytesPerSecMaxOut)
									,UgcPort[PortNo].BlocksOut
									,UgcPort[PortNo].PacketsOut
									,SS(UgcPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UgrPort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(UgrPort[PortNo].OutBuffer.Size +UgrPort[PortNo].IOCPSendBytes)
									#else
									,SS(UgrPort[PortNo].OutBuffer.Size)
									#endif
									,SS(UgrPort[PortNo].BytesPerSecOut)
									,SS(UgrPort[PortNo].BytesPerSecAvgOut)
									,SS(UgrPort[PortNo].BytesPerSecMaxOut)
									,UgrPort[PortNo].BlocksOut
									,UgrPort[PortNo].PacketsOut
									,SS(UgrPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UmcPort[PortNo].BeatsOut
									,SS(UmcPort[PortNo].OutBuffer_Size)
									,SS(UmcPort[PortNo].BytesPerSecOut)
									,SS(UmcPort[PortNo].BytesPerSecAvgOut)
									,SS(UmcPort[PortNo].BytesPerSecMaxOut)
									,UmcPort[PortNo].BlocksOut
									,UmcPort[PortNo].PacketsOut
									,SS(UmcPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UMR:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UmrPort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(UmrPort[PortNo].OutBuffer.Size +UmrPort[PortNo].IOCPSendBytes)
									#else
									,SS(UmrPort[PortNo].OutBuffer.Size)
									#endif
									,SS(UmrPort[PortNo].BytesPerSecOut)
									,SS(UmrPort[PortNo].BytesPerSecAvgOut)
									,SS(UmrPort[PortNo].BytesPerSecMaxOut)
									,UmrPort[PortNo].BlocksOut
									,UmrPort[PortNo].PacketsOut
									,SS(UmrPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CtoPort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(CtoPort[PortNo].OutBuffer.Size +CtoPort[PortNo].IOCPSendBytes)
									#else
									,SS(CtoPort[PortNo].OutBuffer.Size)
									#endif
									,SS(CtoPort[PortNo].BytesPerSecOut)
									,SS(CtoPort[PortNo].BytesPerSecAvgOut)
									,SS(CtoPort[PortNo].BytesPerSecMaxOut)
									,CtoPort[PortNo].BlocksOut
									,CtoPort[PortNo].PacketsOut
									,SS(CtoPort[PortNo].BytesOut)
									);
								CtoPort[PortNo].PacketsOut=0;
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_CTO_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CtoPort[NO_OF_CTO_PORTS].BeatsOut
									#ifdef WS_OTPP
									,SS(CtoPort[NO_OF_CTO_PORTS].OutBuffer.Size +CtoPort[NO_OF_CTO_PORTS].IOCPSendBytes)
									#else
									,SS(CtoPort[NO_OF_CTO_PORTS].OutBuffer.Size)
									#endif
									,SS(CtoPort[NO_OF_CTO_PORTS].BytesPerSecOut)
									,SS(CtoPort[NO_OF_CTO_PORTS].BytesPerSecAvgOut)
									,SS(CtoPort[NO_OF_CTO_PORTS].BytesPerSecMaxOut)
									,CtoPort[NO_OF_CTO_PORTS].BlocksOut
									,CtoPort[NO_OF_CTO_PORTS].PacketsOut
									,SS(CtoPort[NO_OF_CTO_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
							case WS_FIL:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,FilePort[PortNo].BeatsOut
									#ifdef WS_OTPP
									,SS(FilePort[PortNo].OutBuffer.Size +FilePort[PortNo].IOCPSendBytes)
									#else
									,SS(FilePort[PortNo].OutBuffer.Size)
									#endif
									,SS(FilePort[PortNo].BytesPerSecOut)
									,SS(FilePort[PortNo].BytesPerSecAvgOut)
									,SS(FilePort[PortNo].BytesPerSecMaxOut)
									,FilePort[PortNo].BlocksOut
									,FilePort[PortNo].PacketsOut
									,SS(FilePort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								((NMLVDISPINFO*)lParam)->item.pszText=OtherPort[PortNo].Transmit;
								break;
#endif
							case WS_CON_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,ConPort[NO_OF_CON_PORTS].BeatsOut
									#ifdef WS_OTPP
									,SS(ConPort[NO_OF_CON_PORTS].OutBuffer.Size +ConPort[NO_OF_CON_PORTS].IOCPSendBytes)
									#else
									,SS(ConPort[NO_OF_CON_PORTS].OutBuffer.Size)
									#endif
									,SS(ConPort[NO_OF_CON_PORTS].BytesPerSecOut)
									,SS(ConPort[NO_OF_CON_PORTS].BytesPerSecAvgOut)
									,SS(ConPort[NO_OF_CON_PORTS].BytesPerSecMaxOut)
									,ConPort[NO_OF_CON_PORTS].BlocksOut
									,ConPort[NO_OF_CON_PORTS].PacketsOut
									,SS(ConPort[NO_OF_CON_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_USR_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UsrPort[NO_OF_USR_PORTS].BeatsOut
									#ifdef WS_OTPP
									,SS(UsrPort[NO_OF_USR_PORTS].OutBuffer.Size +UsrPort[NO_OF_USR_PORTS].IOCPSendBytes)
									#elif defined(WS_COMPLETION_PORT_SENDS)
									,SS(UsrPort[NO_OF_USR_PORTS].IOCPSendBytes)
									#else
									,SS(UsrPort[NO_OF_USR_PORTS].OutBuffer.Size)
									#endif
									,SS(UsrPort[NO_OF_USR_PORTS].BytesPerSecOut)
									,SS(UsrPort[NO_OF_USR_PORTS].BytesPerSecAvgOut)
									,SS(UsrPort[NO_OF_USR_PORTS].BytesPerSecMaxOut)
									,UsrPort[NO_OF_USR_PORTS].BlocksOut
									,UsrPort[NO_OF_USR_PORTS].PacketsOut
									,SS(UsrPort[NO_OF_USR_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#ifdef WS_GUARANTEED
							case WS_CGD_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,CgdPort[NO_OF_CGD_PORTS].BeatsOut
									#ifdef WS_OTPP
									,SS(CgdPort[NO_OF_CGD_PORTS].OutBuffer.Size +CgdPort[NO_OF_CGD_PORTS].IOCPSendBytes)
									#else
									,SS(CgdPort[NO_OF_CGD_PORTS].OutBuffer.Size)
									#endif
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesPerSecOut)
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesPerSecAvgOut)
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesPerSecMaxOut)
									,CgdPort[NO_OF_CGD_PORTS].BlocksOut
									,CgdPort[NO_OF_CGD_PORTS].PacketsOut
									,SS(CgdPort[NO_OF_CGD_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_UGR_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,UgrPort[NO_OF_UGR_PORTS].BeatsOut
									#ifdef WS_OTPP
									,SS(UgrPort[NO_OF_UGR_PORTS].OutBuffer.Size +UgrPort[NO_OF_UGR_PORTS].IOCPSendBytes)
									#else
									,SS(UgrPort[NO_OF_UGR_PORTS].OutBuffer.Size)
									#endif
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesPerSecOut)
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesPerSecAvgOut)
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesPerSecMaxOut)
									,UgrPort[NO_OF_UGR_PORTS].BlocksOut
									,UgrPort[NO_OF_UGR_PORTS].PacketsOut
									,SS(UgrPort[NO_OF_UGR_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_FILE_SERVER
							case WS_FIL_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,FilePort[NO_OF_FILE_PORTS].BeatsOut
									#ifdef WS_OTPP
									,SS(FilePort[NO_OF_FILE_PORTS].OutBuffer.Size +FilePort[NO_OF_FILE_PORTS].IOCPSendBytes)
									#else
									,SS(FilePort[NO_OF_FILE_PORTS].OutBuffer.Size)
									#endif
									,SS(FilePort[NO_OF_FILE_PORTS].BytesPerSecOut)
									,SS(FilePort[NO_OF_FILE_PORTS].BytesPerSecAvgOut)
									,SS(FilePort[NO_OF_FILE_PORTS].BytesPerSecMaxOut)
									,FilePort[NO_OF_FILE_PORTS].BlocksOut
									,FilePort[NO_OF_FILE_PORTS].PacketsOut
									,SS(FilePort[NO_OF_FILE_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif //#ifdef WS_FILE_SERVER
#ifdef WS_BROADCAST_TRANSMITTER
							case WS_BRT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,BrtPort[PortNo].BeatsOut
									,"0" // hold buffer
									,SS(BrtPort[PortNo].BytesPerSecOut)
									,SS(BrtPort[PortNo].BytesPerSecAvgOut)
									,SS(BrtPort[PortNo].BytesPerSecMaxOut)
									,BrtPort[PortNo].BlocksOut
									,BrtPort[PortNo].PacketsOut
									,SS(BrtPort[PortNo].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
							case WS_BRT_TOT:
								sprintf(Tempstr,"HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s"
									,BrtPort[NO_OF_BRT_PORTS].BeatsOut
									,"0" // hold buffer
									,SS(BrtPort[NO_OF_BRT_PORTS].BytesPerSecOut)
									,SS(BrtPort[NO_OF_BRT_PORTS].BytesPerSecAvgOut)
									,SS(BrtPort[NO_OF_BRT_PORTS].BytesPerSecMaxOut)
									,BrtPort[NO_OF_BRT_PORTS].BlocksOut
									,BrtPort[NO_OF_BRT_PORTS].PacketsOut
									,SS(BrtPort[NO_OF_BRT_PORTS].BytesOut)
									);
								((NMLVDISPINFO*)lParam)->item.pszText=Tempstr;
								break;
#endif // WS_BROADCAST_TRANSMITTER
						}
						break;
					}
					return FALSE;
				}
			case NM_DBLCLK:
			case NM_CLICK:
				{
					LVITEM LvItem={LVIF_PARAM,((NMITEMACTIVATE*)lParam)->iItem,0,0,0,NULL,0,0,0,0};
					if(!ListView_GetItem(pdoc->pmod->WShConList,&LvItem))
						break;
					WSPortType Type=(WSPortType)HIWORD(LvItem.lParam);
					int PortNo=LOWORD(LvItem.lParam);
					char PortName[32]={0};
					GetDispPort(Type,PortNo,PortName);

					if(Code==NM_DBLCLK)
					{
						switch (Type)
						{
							case WS_CON:
								ConPortSetupBox(PortNo);
								break;
							case WS_USC:
								UscPortSetupBox(PortNo);
								break;
							case WS_USR:
								pdoc->pmod->WSCloseUsrPort(PortNo);
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								CgdPortSetupBox(PortNo);
								break;
							case WS_UGC:
								UgcPortSetupBox(PortNo);
								break;
							case WS_UGR:
								pdoc->pmod->WSCloseUgrPort(PortNo);
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								pdoc->pmod->WSCloseUmcPort(PortNo);
								break;
							case WS_UMR:
								pdoc->pmod->WSCloseUmrPort(PortNo);
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								pdoc->pmod->WSCloseCtoPort(PortNo);
								break;
							case WS_CTI:
                                CtiPortSetupBox(PortNo);
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
                            case WS_FIL:
                                FilePortSetupBox(PortNo);
                                break;
#endif //WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								pdoc->pmod->WSCloseOtherPort(PortNo);
								break;
#endif
#ifdef WS_BROADCAST_TRANSMITTER
                            case WS_BRT:
                                break;
#endif
#ifdef WS_BROADCAST_RECEIVER
                            case WS_BRR:
                                break;
#endif
						}
					}
					if(Code==NM_CLICK)
					{
						switch (Type)
						{
							case WS_CON:
								WSConClick(PortNo);
								break;
							case WS_USC:
								//WSUscClick(PortNo);
								break;
							case WS_USR:
								WSUsrClick(PortNo);
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								WSCgdClick(PortNo);
								break;
							case WS_UGC:
								//WSUgcClick(PortNo);
								break;
							case WS_UGR:
								WSUgrClick(PortNo);
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								//WSUmcClick(PortNo);
								break;
							case WS_UMR:
								WSUmrClick(PortNo);
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								WSCtoClick(PortNo);
								break;
							case WS_CTI:
								WSCtiClick(PortNo);
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
							case WS_FIL:
								//WSFileClick(PortNo);
								break;
#endif //WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
							WSOtherClick(PortNo);
								break;
#endif
						}
					}
					break;
				}
#ifdef WS_NEW_DIALOGS
				case NM_RCLICK:
				{
					DWORD dflags=0, pflags=0, hflags=0, dhflags=0;
					for (int i=0;i<ListView_GetItemCount(pdoc->pmod->WShConList);i++)
					{
						LVITEM sli;
						memset(&sli,0,sizeof(LVITEM));
						sli.iItem=i;
						sli.mask=LVIF_STATE|LVIF_PARAM;
						sli.stateMask=LVIS_SELECTED;
						ListView_GetItem(pdoc->pmod->WShConList,&sli);
						// Disable menus
						if (sli.state &LVIS_SELECTED)
						{
							WSPortType Type=(WSPortType)HIWORD(sli.lParam);
							int PortNo=LOWORD(sli.lParam);
							char PortName[32]={0};
							GetDispPort(Type,PortNo,PortName);

							switch (Type)
							{
								case WS_CON:
									if(ConPort[PortNo].ConnectHold) hflags=MF_CHECKED;
									break;
								case WS_CON_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
								case WS_USC:
									dflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
								case WS_USR:
									 hflags=MF_GRAYED;
									break;
								case WS_USR_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
#ifdef WS_GUARANTEED
								case WS_CGD:
									if(CgdPort[PortNo].ConnectHold) hflags=MF_CHECKED;
									break;
								case WS_CGD_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
								case WS_UGC:
									dflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
								case WS_UGR:
									hflags=MF_GRAYED;
									break;
								case WS_UGR_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
								case WS_UMC:
									dflags=MF_GRAYED; pflags=MF_GRAYED; /*hflags=MF_GRAYED;*/ dhflags=MF_GRAYED;
									break;
								case WS_UMR:
									pflags=MF_GRAYED; hflags=MF_GRAYED;
									break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
								case WS_CTO:
									pflags=MF_GRAYED;
									if(CtoPort[PortNo].ConnectHold) hflags=MF_CHECKED;
									break;
								case WS_CTO_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
								case WS_CTI:
									if(CtiPort[PortNo].ConnectHold) hflags=MF_CHECKED;
									break;
								case WS_CTI_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
								case WS_FIL:
									if(FilePort[PortNo].ConnectHold) hflags=MF_CHECKED;
									break;
								case WS_FIL_TOT:
									dflags=MF_GRAYED; pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
#endif //WS_FILE_SERVER
#ifdef WS_OTHER
								case WS_OTH:
									pflags=MF_GRAYED; hflags=MF_GRAYED; dhflags=MF_GRAYED;
									break;
#endif
							}
							break;
						}
					}

					HMENU hmenu = CreatePopupMenu();
					AppendMenu(hmenu, MF_STRING |pflags, WS_IDC_PROPS, "P&roperties");
					AppendMenu(hmenu, MF_STRING, WSH_IDC_EDITPORTS, "&Edit Ports.txt");
					AppendMenu(hmenu, MF_STRING, WS_IDC_GENPORTCFG, "&Generate Ports.txt");
					AppendMenu(hmenu, MF_STRING, WS_IDC_SAVETOCSV, "&Save to csv");
					AppendMenu(hmenu, MF_STRING |dhflags, WS_IDC_DUMPHOLD, "Dump Holding &Buffer");
					AppendMenu(hmenu, MF_STRING, WS_IDC_CLEAN_RECORDINGS, "Clean Recordings 7 Days");
					AppendMenu(hmenu, MF_STRING, WS_IDC_BROWSE_SETUP, "&Browse setup...");
					AppendMenu(hmenu, MF_SEPARATOR, 0, 0);

					//char errLog[MAX_PATH];
					//sprintf(errLog,"logs\\%serr.txt",pdoc->pmod->WScDate);
					//AppendMenu(hmenu, MF_STRING, WS_IDC_ERRORLOG, errLog);
					//char eveLog[MAX_PATH];
					//sprintf(eveLog,"logs\\%seve.txt",pdoc->pmod->WScDate);
					//AppendMenu(hmenu, MF_STRING, WS_IDC_EVENTLOG, eveLog);
#ifdef WS_DEBUG_LOG
					//char dbgLog[MAX_PATH];
					//sprintf(dbgLog,"logs\\%sdbg.txt",pdoc->pmod->WScDate);
					//AppendMenu(hmenu, MF_STRING, WS_IDC_DEBUGLOG, dbgLog);
#endif
#ifdef WS_RECOVER_LOG
					//char recLog[MAX_PATH];
					//sprintf(recLog,"logs\\%srec.txt",pdoc->pmod->WScDate);
					//AppendMenu(hmenu, MF_STRING, WS_IDC_RECOVERLOG, recLog);
#endif
#ifdef WS_RETRY_LOG
					//char retryLog[MAX_PATH];
					//sprintf(retryLog,"logs\\%sretry.csv",pdoc->pmod->WScDate);
					//AppendMenu(hmenu, MF_STRING, WS_IDC_RETRYLOG, retryLog);
#endif
					if(!pdoc->pmod->WSErrorLogPath.empty())
						AppendMenu(hmenu, MF_STRING, WS_IDC_ERRORLOG, pdoc->pmod->WSErrorLogPath.c_str());
					if(!pdoc->pmod->WSEventLogPath.empty())
						AppendMenu(hmenu, MF_STRING, WS_IDC_EVENTLOG, pdoc->pmod->WSEventLogPath.c_str());
				#ifdef WS_DEBUG_LOG
					if(!pdoc->pmod->WSDebugLogPath.empty())
						AppendMenu(hmenu, MF_STRING, WS_IDC_DEBUGLOG, pdoc->pmod->WSDebugLogPath.c_str());
				#endif
				#ifdef WS_RECOVER_LOG
					if(!pdoc->pmod->WSRecoverLogPath.empty())
						AppendMenu(hmenu, MF_STRING, WS_IDC_RECOVERLOG, pdoc->pmod->WSRecoverLogPath.c_str());
				#endif
				#ifdef WS_RETRY_LOG
					if(!pdoc->pmod->WSRetryLogPath.empty())
						AppendMenu(hmenu, MF_STRING, WS_IDC_RETRYLOG, pdoc->pmod->WSRetryLogPath.c_str());
				#endif
					AppendMenu(hmenu, MF_STRING, WS_IDC_BROWSE_LOGS, "&Browse logs...");

					AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
					AppendMenu(hmenu, MF_STRING |dflags, WS_IDC_DISCONNECT, "Dis&connect");
					AppendMenu(hmenu, MF_STRING |hflags, WS_IDC_START_HOLD, "&Hold");

					AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
					AppendMenu(hmenu, MF_STRING, WS_IDC_RELOAD_APP, "&Reload...");

					POINT pt;
					GetCursorPos(&pt);
					TrackPopupMenu(hmenu,
						TPM_LEFTALIGN |TPM_TOPALIGN |TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, 0);
					break;
				}
#endif//WS_NEW_DIALOGS
			}
			break;
		}
#ifdef WS_NEW_DIALOGS
		case WM_COMMAND:
		{
			switch ( LOWORD(wParam) )
			{
			case WS_IDC_DISCONNECT:
			{
				for (int i=0;i<ListView_GetItemCount(pdoc->pmod->WShConList);i++)
				{
					LVITEM sli;
					memset(&sli,0,sizeof(LVITEM));
					sli.iItem=i;
					sli.mask=LVIF_STATE|LVIF_PARAM;
					sli.stateMask=LVIS_SELECTED;
					ListView_GetItem(pdoc->pmod->WShConList,&sli);
					if (sli.state &LVIS_SELECTED)
					{
						WSPortType Type=(WSPortType)HIWORD(sli.lParam);
						int PortNo=LOWORD(sli.lParam);
						char PortName[32]={0};
						GetDispPort(Type,PortNo,PortName);

						switch (Type)
						{
							case WS_CON:
								#ifdef WS_DECLINING_RECONNECT
								ConPort[PortNo].ReconnectDelay=ConPort[PortNo].MinReconnectDelay;
								#endif
								pdoc->pmod->WSCloseConPort(PortNo);
								break;
							case WS_USC:
								//WSCloseUscPort(PortNo);
								break;
							case WS_USR:
								pdoc->pmod->WSCloseUsrPort(PortNo);
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								#ifdef WS_DECLINING_RECONNECT
								CgdPort[PortNo].ReconnectDelay=CgdPort[PortNo].MinReconnectDelay;
								#endif
								pdoc->pmod->WSCloseCgdPort(PortNo);
								break;
							case WS_UGC:
								//WSCloseUgcPort(PortNo);
								break;
							case WS_UGR:
								pdoc->pmod->WSCloseUgrPort(PortNo);
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								pdoc->pmod->WSCloseUmcPort(PortNo);
								break;
							case WS_UMR:
								pdoc->pmod->WSCloseUmrPort(PortNo);
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								pdoc->pmod->WSCloseCtoPort(PortNo);
								break;
							case WS_CTI:
								pdoc->pmod->WSCloseCtiPort(PortNo);
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
							case WS_FIL:
								#ifdef WS_DECLINING_RECONNECT
								FilePort[PortNo].ReconnectDelay=FilePort[PortNo].MinReconnectDelay;
								#endif
								pdoc->pmod->WSCloseFilePort(PortNo);
								break;
#endif //WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								//WSOtherClosed(PortNo);
								pdoc->pmod->WSCloseOtherPort(PortNo);
								break;
#endif
#ifdef WS_BROADCAST_TRANSMITTER
							case WS_BRT:
								pdoc->pmod->WSCloseBrtPort(PortNo);
								pdoc->pmod->BrtPort[PortNo].ReconCount=0;
								break;
#endif
#ifdef WS_BROADCAST_RECEIVER
							case WS_BRR:
								pdoc->pmod->WSCloseBrrPort(PortNo);
								pdoc->pmod->BrrPort[PortNo].ReconCount=0;
								break;
#endif
						}
						break;
					}
				}
				break;
			}
			case WS_IDC_PROPS:
			{
				int idx = ListView_GetNextItem(pdoc->pmod->WShConList, -1, LVNI_SELECTED);
				if ( idx>=0 )
				{
					LVITEM sli;
					memset(&sli,0,sizeof(LVITEM));
					sli.iItem=idx;
					sli.mask=LVIF_PARAM;
					ListView_GetItem(pdoc->pmod->WShConList,&sli);

					WSPortType Type=(WSPortType)HIWORD(sli.lParam);
					int PortNo=LOWORD(sli.lParam);
					char PortName[32]={0};
					GetDispPort(Type,PortNo,PortName);

					if(Type==WS_USR)
						UsrPortSetupBox(PortNo);
					else if(Type==WS_USC)
						UscPortSetupBox(PortNo);
#ifdef WS_GUARANTEED
					else if(Type==WS_UGR)
						UgrPortSetupBox(PortNo);
					else if(Type==WS_UGC)
						UgcPortSetupBox(PortNo);
#endif
					else
					{
						// Translate to double click on selected item
						for (int i=0;i<ListView_GetItemCount(pdoc->pmod->WShConList);i++)
						{
							LVITEM sli;
							memset(&sli,0,sizeof(LVITEM));
							sli.iItem=i;
							sli.mask=LVIF_STATE;
							sli.stateMask=LVIS_SELECTED;
							ListView_GetItem(pdoc->pmod->WShConList,&sli);
							if (sli.state &LVIS_SELECTED)
							{
								NMITEMACTIVATE nmia;
								memset(&nmia,0,sizeof(NMITEMACTIVATE));
								nmia.hdr.code=NM_DBLCLK;
								nmia.iItem=i;
								return cbConList(hDlg,WM_NOTIFY,0,(LPARAM)&nmia);
							}
						}
					}
				}
				break;
			}
			case WSH_IDC_EDITPORTS:
				//ShellExecute(hDlg,"open","setup\\ports.txt",0,0,SW_SHOW);
				ShellExecute(hDlg,"open",pdoc->pmod->pcfg->ConfigPath.c_str(),0,0,SW_SHOW);
				break;
			case WS_IDC_GENPORTCFG:
				WSVGenPortCfg();
				break;
			case WS_IDC_SAVETOCSV:
				WSVSaveToCsv();
				break;
			case WS_IDC_BROWSE_SETUP:
			{
				char spath[MAX_PATH]={0},*sptr=0;
				GetFullPathName(pdoc->pmod->pcfg->ConfigPath.c_str(),MAX_PATH,spath,&sptr);
				if(sptr) *sptr=0;
				ShellExecute(hDlg,"open",spath,0,0,SW_SHOW);
				break;
			}
			case WS_IDC_DUMPHOLD:
			{
				int idx = ListView_GetNextItem(pdoc->pmod->WShConList, -1, LVNI_SELECTED);
				if ( idx>=0 )
				{
					LVITEM sli;
					memset(&sli,0,sizeof(LVITEM));
					sli.iItem=idx;
					sli.mask=LVIF_PARAM;
					ListView_GetItem(pdoc->pmod->WShConList,&sli);

					WSPortType Type=(WSPortType)HIWORD(sli.lParam);
					int PortNo=LOWORD(sli.lParam);
					char PortName[32]={0};
					GetDispPort(Type,PortNo,PortName);
					WSVDumpHold(Type,PortNo);
				}
				break;
			}
			case WS_IDC_CLEAN_RECORDINGS:
			{
				pdoc->pmod->phost->WSHCleanRecordings(pdoc->pmod,7,0,TRUE);
				break;
			}
			case WS_IDC_EVENTLOG:
			{
				char FileName[MAX_PATH];
//#ifdef CLSERVER_EXPORT
//				sprintf(FileName,"logs\\export%seve.txt",pdoc->pmod->WScDate);
//#else
//				sprintf(FileName,"logs\\%seve.txt",pdoc->pmod->WScDate);
//#endif
				strcpy(FileName,pdoc->pmod->WSEventLogPath.c_str());
				if(FileName[1]!=':')
					sprintf(FileName,"%s\\%s",pdoc->pmod->pcfg->RunPath.c_str(),pdoc->pmod->WSEventLogPath.c_str());
				// Use notepad instead of user-defined association,
				// because 3rd party programs may lock the logs.
				ShellExecute(pdoc->pmod->WShWnd, "open", "notepad.exe", FileName, 0, SW_SHOW);
				break;
			}
			case WS_IDC_ERRORLOG:
			{
				char FileName[MAX_PATH];
//#ifdef CLSERVER_EXPORT
//				sprintf(FileName,"logs\\export%serr.txt",pdoc->pmod->WScDate);
//#else
//				sprintf(FileName,"logs\\%serr.txt",pdoc->pmod->WScDate);
//#endif
				strcpy(FileName,pdoc->pmod->WSErrorLogPath.c_str());
				if(FileName[1]!=':')
					sprintf(FileName,"%s\\%s",pdoc->pmod->pcfg->RunPath.c_str(),pdoc->pmod->WSErrorLogPath.c_str());
				ShellExecute(pdoc->pmod->WShWnd, "open", "notepad.exe", FileName, 0, SW_SHOW);
				break;
			}
#ifdef WS_DEBUG_LOG
			case WS_IDC_DEBUGLOG:
			{
				char FileName[MAX_PATH];
				//sprintf(FileName,"logs\\%sdbg.txt",pdoc->pmod->WScDate);
				strcpy(FileName,pdoc->pmod->WSDebugLogPath.c_str());
				if(FileName[1]!=':')
					sprintf(FileName,"%s\\%s",pdoc->pmod->pcfg->RunPath.c_str(),pdoc->pmod->WSDebugLogPath.c_str());
				ShellExecute(pdoc->pmod->WShWnd, "open", "notepad.exe", FileName, 0, SW_SHOW);
				break;
			}
#endif
#ifdef WS_RECOVER_LOG
			case WS_IDC_RECOVERLOG:
			{
				char FileName[MAX_PATH];
				//sprintf(FileName,"logs\\%srec.txt",pdoc->pmod->WScDate);
				strcpy(FileName,pdoc->pmod->WSRecoverLogPath.c_str());
				if(FileName[1]!=':')
					sprintf(FileName,"%s\\%s",pdoc->pmod->pcfg->RunPath.c_str(),pdoc->pmod->WSRecoverLogPath.c_str());
				ShellExecute(pdoc->pmod->WShWnd, "open", "notepad.exe", FileName, 0, SW_SHOW);
				break;
			}
#endif
#ifdef WS_RETRY_LOG
			case WS_IDC_RETRYLOG:
			{
				char FileName[MAX_PATH];
				//sprintf(FileName,"logs\\%sretry.csv",pdoc->pmod->WScDate);
				strcpy(FileName,pdoc->pmod->WSRetryLogPath.c_str());
				if(FileName[1]!=':')
					sprintf(FileName,"%s\\%s",pdoc->pmod->pcfg->RunPath.c_str(),pdoc->pmod->WSRetryLogPath.c_str());
				ShellExecute(pdoc->pmod->WShWnd, "open", "notepad.exe", FileName, 0, SW_SHOW);
				break;
			}
#endif
			case WS_IDC_BROWSE_LOGS:
			{
				ShellExecute(hDlg,"open",pdoc->pmod->pcfg->LogPath.c_str(),0,0,SW_SHOW);
				break;
			}
			case WS_IDC_START_HOLD:
			{
				for (int i=0;i<ListView_GetItemCount(pdoc->pmod->WShConList);i++)
				{
					LVITEM sli;
					memset(&sli,0,sizeof(LVITEM));
					sli.iItem=i;
					sli.mask=LVIF_STATE|LVIF_PARAM;
					sli.stateMask=LVIS_SELECTED;
					ListView_GetItem(pdoc->pmod->WShConList,&sli);
					if (sli.state &LVIS_SELECTED)
					{
						WSPortType Type=(WSPortType)HIWORD(sli.lParam);
						int PortNo=LOWORD(sli.lParam);
						char PortName[32]={0};
						GetDispPort(Type,PortNo,PortName);

						switch (Type)
						{
							case WS_CON:
								ConPort[PortNo].ConnectHold=!ConPort[PortNo].ConnectHold;
							#ifdef WS_DECLINING_RECONNECT
								if(ConPort[PortNo].ConnectHold)
								{
									ConPort[PortNo].ReconnectDelay=0;
									ConPort[PortNo].ReconnectTime=0;
								}
							#endif
								break;
							case WS_USC:
								UscPort[PortNo].ConnectHold=!UscPort[PortNo].ConnectHold;
								UscPort[PortNo].Status[0]=0;
								break;
							case WS_USR:
								break;
#ifdef WS_GUARANTEED
							case WS_CGD:
								CgdPort[PortNo].ConnectHold=!CgdPort[PortNo].ConnectHold;
							#ifdef WS_DECLINING_RECONNECT
								if(CgdPort[PortNo].ConnectHold)
								{
									CgdPort[PortNo].ReconnectDelay=0;
									CgdPort[PortNo].ReconnectTime=0;
								}
							#endif
								break;
							case WS_UGC:
							case WS_UGR:
								break;
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
							case WS_UMC:
								UmcPort[PortNo].ConnectHold=!UmcPort[PortNo].ConnectHold;
								UmcPort[PortNo].Status[0]=0;
								break;
							case WS_UMR:
								break;
#endif //#ifdef WS_MONITOR
#ifdef WS_CAST
							case WS_CTO:
								CtoPort[PortNo].ConnectHold=!CtoPort[PortNo].ConnectHold;
								break;
							case WS_CTI:
								CtiPort[PortNo].ConnectHold=!CtiPort[PortNo].ConnectHold;
								break;
#endif//#ifdef WS_CAST
#ifdef WS_FILE_SERVER
							case WS_FIL:
								FilePort[PortNo].ConnectHold=!FilePort[PortNo].ConnectHold;
							#ifdef WS_DECLINING_RECONNECT
								if(FilePort[PortNo].ConnectHold)
								{
									FilePort[PortNo].ReconnectDelay=0;
									FilePort[PortNo].ReconnectTime=0;
								}
							#endif
								break;
#endif //WS_FILE_SERVER
#ifdef WS_OTHER
							case WS_OTH:
								break;
#endif
#ifdef WS_BROADCAST_TRANSMITTER
							case WS_BRT:
								BrtPort[PortNo].ConnectHold=!BrtPort[PortNo].ConnectHold;
								if(!BrtPort[PortNo].ConnectHold)
									BrtPort[PortNo].ReconCount=0;
								break;
#endif
#ifdef WS_BROADCAST_RECEIVER
							case WS_BRR:
								BrrPort[PortNo].ConnectHold=!BrrPort[PortNo].ConnectHold;
								if(!BrrPort[PortNo].ConnectHold)
									BrrPort[PortNo].ReconCount=0;
								break;
#endif
						}
						break;
					}
				}
				break;
			}
			case WS_IDC_RELOAD_APP:
			{
				char wmsg[1024]={0};
				sprintf(wmsg,"Confirm stop and reload of \"%s\". Continue?",pdoc->pmod->pcfg->aname.c_str());
				if(MessageBox(wmsg,"Reload App",MB_ICONQUESTION|MB_YESNO)==IDYES)
					theApp.WSHLoadAppIni(pdoc->pmod->pcfg->aname.c_str());
				break;
			}
			}//LOWORD(wParam)
			break;
		}//WM_COMMAND
#endif//WS_NEW_DIALOGS
#if defined(WS_DEBUG_LOG)||defined(WS_RECOVER_LOG)
		case WM_TIMER:
			if(wParam==0x01)
			{
				RECT lrect=WShDlgConList_LastRect;
				::GetWindowRect(WShDlgConList,&WShDlgConList_LastRect);
				if((!lrect.left)&&(!lrect.top)&&(!lrect.right)&&(!lrect.bottom))
					lrect=WShDlgConList_LastRect;
				RECT mrect=WShDlgConList_LastRect;
#ifdef WS_DEBUG_LOG
				if(IsWindow(WShDlgDebug))
				{
					RECT nrect;
					::GetWindowRect(WShDlgDebug, &nrect);
					int cx = nrect.right -nrect.left;
					int cy = nrect.bottom -nrect.top;
					int dx = nrect.left -lrect.left;
					int dy = nrect.top -lrect.top;
					nrect.left = mrect.left +dx; nrect.right = nrect.left +cx;
					nrect.top = mrect.top +dy; nrect.bottom = nrect.top +cy;
					::MoveWindow(WShDlgDebug, nrect.left, nrect.top, nrect.right -nrect.left, nrect.bottom -nrect.top, true);
				}
#endif
#ifdef WS_RECOVER_LOG
				if(IsWindow(WShDlgRecover))
				{
					RECT nrect;
					::GetWindowRect(WShDlgRecover, &nrect);
					int cx = nrect.right -nrect.left;
					int cy = nrect.bottom -nrect.top;
					int dx = nrect.left -lrect.left;
					int dy = nrect.top -lrect.top;
					nrect.left = mrect.left +dx; nrect.right = nrect.left +cx;
					nrect.top = mrect.top +dy; nrect.bottom = nrect.top +cy;
					::MoveWindow(WShDlgRecover, nrect.left, nrect.top, nrect.right -nrect.left, nrect.bottom -nrect.top, true);
				}
#endif
				break;
			}
#endif
 	}
    return FALSE;
}

// I hate the name of this function, but is kept for legacy. It mainly creates the error and connection lists,
// but also resets the error count (which is not useful)
void CIQMatrixView::ResetErrorCnt(void)
{
	CIQMatrixDoc *pdoc=GetDocument();
#ifndef UNIX
#ifdef WS_OVERLAP_IO
    WSMainThreadId=GetCurrentThreadId();
#endif
#ifndef WS_DISPOFF
	//setting up Error window
	struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
		DLGITEM(1,10)
	} WSErrorTemplate =	{DS_MODALFRAME | WS_CHILD | WS_VISIBLE | WS_CAPTION,0,1,0,0,393,43,0,0,0
							,WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY ,WS_EX_DLGMODALFRAME,0,0,393,43,WSH_LISTBOX
								,{0xffff,0x0083},L"Error Log",0};
#ifdef WS_SCREEN_WIDTH
	WSErrorTemplate.cx    = WS_SCREEN_WIDTH;
	WSErrorTemplate.DI1cx = WS_SCREEN_WIDTH;
#endif

	//setting up Event window
	struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
		DLGITEM(1,10)
	} WSEventTemplate =	{DS_MODALFRAME | WS_CHILD | WS_VISIBLE | WS_CAPTION,0,1,0,55,393,43,0,0,0
							,WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY ,WS_EX_DLGMODALFRAME,0,0,393,43,WSH_LISTBOX
								,{0xffff,0x0083},L"Event Log",0};

#ifdef WS_DEBUG_LOG
	//setting up Debug window
	struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
		DLGITEM(1,10)
	} WSDebugTemplate =	{DS_MODALFRAME | WS_POPUP | WS_CAPTION,0,1,100,30,393,43,0,0,0
							,WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY ,WS_EX_DLGMODALFRAME,0,0,393,43,WSH_LISTBOX
								,{0xffff,0x0083},L"Debug Log",0};
#endif

#ifdef WS_RECOVER_LOG
	//setting up Recover window
	struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
		DLGITEM(1,12)
	} WSRecoverTemplate =	{DS_MODALFRAME | WS_POPUP | WS_CAPTION,0,1,100,30,393,43,0,0,0
							,WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY ,WS_EX_DLGMODALFRAME,0,0,393,43,WSH_LISTBOX
								,{0xffff,0x0083},L"Recover Log",0};
#endif

#ifdef WS_SCREEN_WIDTH
	WSErrorTemplate.cx    = WS_SCREEN_WIDTH;
	WSErrorTemplate.DI1cx = WS_SCREEN_WIDTH;
#endif

	dlgParam=0;
	pdoc->pmod->WShDlgError=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPCDLGTEMPLATE)&WSErrorTemplate,pdoc->pmod->WShWnd,(DLGPROC)::cbReport,(LPARAM)this);
	::SetWindowText(pdoc->pmod->WShDlgError,"ERROR LOG");

#ifdef WS_SCREEN_WIDTH
	WSEventTemplate.cx    = WS_SCREEN_WIDTH;
	WSEventTemplate.DI1cx = WS_SCREEN_WIDTH;
#endif

	dlgParam=0;
	pdoc->pmod->WShDlgEvent=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPCDLGTEMPLATE)&WSEventTemplate,pdoc->pmod->WShWnd,(DLGPROC)::cbReport,(LPARAM)this);
	::SetWindowText(pdoc->pmod->WShDlgEvent,"EVENT LOG");

#ifdef WS_DEBUG_LOG
#ifdef WS_SCREEN_WIDTH
	WSEventTemplate.cx    = WS_SCREEN_WIDTH;
	WSEventTemplate.DI1cx = WS_SCREEN_WIDTH;
#endif

	dlgParam=0;
	pdoc->pmod->WShDlgDebug =CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPCDLGTEMPLATE)&WSDebugTemplate,pdoc->pmod->WShWnd,(DLGPROC)::cbReport,(LPARAM)this);
	::SetWindowText(WShDlgDebug,"DEBUG LOG");
	ShowWindow(pdoc->pmod->WShDlgDebug,WSShowDlgDebug);
#endif

#ifdef WS_RECOVER_LOG
#ifdef WS_SCREEN_WIDTH
	WSEventTemplate.cx    = WS_SCREEN_WIDTH;
	WSEventTemplate.DI1cx = WS_SCREEN_WIDTH;
#endif

	dlgParam=0;
	pdoc->pmod->WShDlgRecover =CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPCDLGTEMPLATE)&WSRecoverTemplate,pdoc->pmod->WShWnd,(DLGPROC)::cbReport,(LPARAM)this);
	::SetWindowText(WShDlgRecover,"RECOVER LOG");
	ShowWindow(pdoc->pmod->WShDlgDebug,WSShowDlgRecover);
#endif

//setup Connect List Window
	struct
	{
		DWORD style;
		DWORD dwExtendedStyle;
		WORD cdit;
		short x;
		short y;
		short cx;
		short cy;
		WORD Menu;
		WORD Class;
		WORD Title;
	} WSConListTemplate =	{DS_MODALFRAME | WS_CHILD | WS_VISIBLE | WS_CAPTION,0,0,0,110,393,144,0,0,0};
	int i;
	char ColomHead[7][20]={"PORT","IP-ADDRESS","CLIENT-IP","NOTE","STATUS","RECEIVE","TRANSMIT"};
	LVCOLUMN MyColoms[7]={
						{LVCF_WIDTH | LVCF_TEXT | LVCF_FMT , LVCFMT_LEFT,60,ColomHead[0],0,0,0,0}
						,{LVCF_WIDTH | LVCF_TEXT, 0,130,ColomHead[1],0,1,0,0}
						,{LVCF_WIDTH | LVCF_TEXT, 0,10,ColomHead[2],0,2,0,0}
						,{LVCF_WIDTH | LVCF_TEXT, 0,130,ColomHead[3],0,3,0,0}
						,{LVCF_WIDTH | LVCF_TEXT, 0,100,ColomHead[4],0,4,0,0}
						,{LVCF_WIDTH | LVCF_TEXT, 0,172,ColomHead[5],0,5,0,0}
						,{LVCF_WIDTH | LVCF_TEXT, 0,172,ColomHead[6],0,6,0,0}
						};


	INITCOMMONCONTROLSEX icex;

#ifdef WS_SCREEN_WIDTH
	WSConListTemplate.cx    = WS_SCREEN_WIDTH;
#endif

#ifdef WS_CONLIST_HEIGHT
	WSConListTemplate.cy    = WS_CONLIST_HEIGHT;
#endif

	// Ensure that the common control DLL is loaded.
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	dlgParam=0;
	pdoc->pmod->WShDlgConList=CreateDialogIndirectParam(pdoc->pmod->WShInst, (LPCDLGTEMPLATE)&WSConListTemplate,pdoc->pmod->WShWnd,(DLGPROC)::cbConList,(LPARAM)this);

	RECT r;
	::GetWindowRect(pdoc->pmod->WShDlgConList, &r);
	pdoc->pmod->WShConList=CreateWindow(WC_LISTVIEW,"CONLIST",WS_CHILD | LVS_REPORT  | WS_VISIBLE | WS_VSCROLL | LVS_SORTASCENDING | LVS_SHOWSELALWAYS
		,0,0,r.right -r.left,r.bottom -r.top,pdoc->pmod->WShDlgConList,0,pdoc->pmod->WShInst,0);
	for (i=0;i<7;i++)
	{
		ListView_InsertColumn(pdoc->pmod->WShConList,i,&MyColoms[i]);
	}
	DWORD dwStyle = (DWORD)::SendMessage(pdoc->pmod->WShConList, LVM_GETEXTENDEDLISTVIEWSTYLE,0,0);
	dwStyle |= LVS_EX_FULLROWSELECT;
	::SendMessage(pdoc->pmod->WShConList, LVM_SETEXTENDEDLISTVIEWSTYLE,0,dwStyle);
#endif
#endif //ifndef UNIX
	//LastErrorCnt=0;
	//CheckTime();
}

// CIQMatrixView message handlers
int CIQMatrixView::WSResize(int cx,int cy)
{
	CIQMatrixDoc *pdoc=GetDocument();
	RECT drect;
	POINT p1,p2;
	if((pdoc->pmod)&&(pdoc->pmod->WShWnd)&&IsWindow(pdoc->pmod->WShWnd)&&
	   (pdoc->pmod->WShDlgError)&&IsWindow(pdoc->pmod->WShDlgError)&&
	   (pdoc->pmod->WShDlgEvent)&&IsWindow(pdoc->pmod->WShDlgEvent)&&
	   (pdoc->pmod->WShDlgConList)&&IsWindow(pdoc->pmod->WShDlgConList)&&
	   (pdoc->pmod->WShConList)&&IsWindow(pdoc->pmod->WShConList))
	{
		::GetWindowRect(pdoc->pmod->WShDlgError,&drect);
		p1.x=drect.left; p1.y=drect.top; p2.x=drect.right; p2.y=drect.bottom;
		::ScreenToClient(pdoc->pmod->WShWnd,&p1); ::ScreenToClient(pdoc->pmod->WShWnd,&p2);
		p2.x=p1.x+cx;
		::MoveWindow(pdoc->pmod->WShDlgError,p1.x,p1.y,p2.x-p1.x,p2.y-p1.y,true);
		::MoveWindow(::GetDlgItem(pdoc->pmod->WShDlgError,WSH_LISTBOX),0,0,p2.x-p1.x-5,p2.y-p1.y-10,true);

		::GetWindowRect(pdoc->pmod->WShDlgEvent,&drect);
		p1.x=drect.left; p1.y=drect.top; p2.x=drect.right; p2.y=drect.bottom;
		::ScreenToClient(pdoc->pmod->WShWnd,&p1); ::ScreenToClient(pdoc->pmod->WShWnd,&p2);
		p2.x=p1.x+cx;
		::MoveWindow(pdoc->pmod->WShDlgEvent,p1.x,p1.y,p2.x-p1.x,p2.y-p1.y,true);
		::MoveWindow(::GetDlgItem(pdoc->pmod->WShDlgEvent,WSH_LISTBOX),0,0,p2.x-p1.x-5,p2.y-p1.y-10,true);

		//MoveWindow(pdoc->pmod->WShDlgDebug,drect.left,drect.top,drect.left+cx,drect.bottom,true);

		::GetWindowRect(pdoc->pmod->WShDlgConList,&drect);
		p1.x=drect.left; p1.y=drect.top; p2.x=drect.right; p2.y=drect.bottom;
		::ScreenToClient(pdoc->pmod->WShWnd,&p1); ::ScreenToClient(pdoc->pmod->WShWnd,&p2);
		p2.x=p1.x+cx; p2.y=cy;
		::MoveWindow(pdoc->pmod->WShDlgConList,p1.x,p1.y,p2.x-p1.x,p2.y-p1.y,true);
		::MoveWindow(pdoc->pmod->WShConList,0,0,p2.x-p1.x-5,p2.y-p1.y-25,true);
	}
	return 0;
}

void CIQMatrixView::WSSetAppHead(char *Head)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
#ifndef WS_DISPOFF
	::SetWindowText(pdoc->pmod->WShWnd,Head);
#endif// NOT WS_DISPOFF
}
void CIQMatrixView::WSSetAppStatus(char *Status)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
#ifndef WS_DISPOFF
	char szTemp[1024];
	struct tm ltm;
	memset(&ltm,0,sizeof(tm));
	ltm.tm_year=pdoc->pmod->buildDate/10000 -1900;
	ltm.tm_mon=(pdoc->pmod->buildDate%10000)/100 -1;
	ltm.tm_mday=pdoc->pmod->buildDate%100;
	ltm.tm_hour=pdoc->pmod->buildTime/10000;
	ltm.tm_min=(pdoc->pmod->buildTime%10000)/100;
	ltm.tm_sec=pdoc->pmod->buildTime%100;
	ltm.tm_isdst=-1;
	char bstr[128]={0};
	strftime(bstr,128,"%b %d %Y-%H:%M:%S",&ltm);
	sprintf(szTemp,"%s[%s]",theApp.AppStatusStr,bstr);
	strcat(szTemp,Status);
#ifdef UNIX
	printf("%s\n",szTemp);
#else
	::SetWindowText(pdoc->pmod->WShDlgConList,szTemp);
	pdoc->status=szTemp;
#endif
#endif// NOT WS_DISPOFF
}

// Updates the view connection list once per second
void CIQMatrixView::WSDisplayStatus(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	CIQMatrixDoc *pdoc=GetDocument();
	if((!pdoc->pmod)||(!pdoc->pmod->WSInitialized()))
		return;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	int i;
#ifndef WS_DISPOFF
	char szTemp1[255];
#endif// NOT WS_DISPOFF
	int none=TRUE;
	int NoOfUscs=0;
	int NoOfUgcs=0;
	int NoOfUsers=0;
	int NoOfUmcs=0;
	int NoOfUmers=0;
	int NoOfCons=0;
    int NoOfCgds=0;
	int NoOfCtos=0;
	int NoOfCtis=0;
    int NoOfFiles=0;
	char szTemp[1024];
//#endif// NOT WS_DISPOFF
	//CheckTime();
	for(i=0;i<NO_OF_USR_PORTS;i++)
	{
		if(UsrPort[i].SockActive)
		{
			UsrPort[i].SinceOpenTimer++;
		}
	}
#ifdef WS_GUARANTEED
	for(i=0;i<NO_OF_UGR_PORTS;i++)
	{
		if(UgrPort[i].SockActive)
		{
			UgrPort[i].SinceOpenTimer++;
		}
	}
#endif //#ifdef WS_GUARANTEED
#ifdef WS_MONITOR
	for(i=0;i<NO_OF_UMR_PORTS;i++)
	{
		if(UmrPort[i].SockActive)
		{
			UmrPort[i].SinceOpenTimer++;
		}
	}
#endif //#ifdef WS_MONITOR
// The front-end wants statistics even though the display is off
//#ifndef WS_DISPOFF
	szTemp[0]=0;
	ConPort[NO_OF_CON_PORTS].BeatsIn=0;
	ConPort[NO_OF_CON_PORTS].InBuffer.Size=0;
	ConPort[NO_OF_CON_PORTS].BytesPerSecIn=0;
	ConPort[NO_OF_CON_PORTS].BytesPerSecAvgIn=0;
	ConPort[NO_OF_CON_PORTS].BlocksIn=0;
	ConPort[NO_OF_CON_PORTS].PacketsIn=0;

	ConPort[NO_OF_CON_PORTS].BeatsOut=0;
	ConPort[NO_OF_CON_PORTS].OutBuffer.Size=0;
	ConPort[NO_OF_CON_PORTS].BytesPerSecOut=0;
	ConPort[NO_OF_CON_PORTS].BytesPerSecAvgOut=0;
	ConPort[NO_OF_CON_PORTS].BlocksOut=0;
	ConPort[NO_OF_CON_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	ConPort[NO_OF_CON_PORTS].IOCPSendBytes=0;
	#elif defined(WS_COMPLETION_PORT_SENDS)
	ConPort[NO_OF_CON_PORTS].IOCPSendBytes=0;
	#endif
	#ifdef WS_BROADCAST_RECEIVER
	ConPort[NO_OF_CON_PORTS].broadcastRepairCnt=0;
	#endif

	for(i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(ConPort[i].InUse)
		{
			NoOfCons++;
			if(!pdoc->pmod->initialized)
				strcpy(ConPort[i].Status,"LOADING ");
			else if (ConPort[i].ConnectHold)
				strcpy(ConPort[i].Status,"HOLDING ");
			else if (ConPort[i].ReconCount)
			{
				if((ConPort[i].S5Connect)&&(ConPort[i].S5Status==0))
					_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"CONNECTING TO PROXY %s:%d",ConPort[i].S5RemoteIP,ConPort[i].S5Port);
				else
					strcpy(ConPort[i].Status,"CONNECTING ");
			}
			else if((ConPort[i].S5Connect)&&(ConPort[i].S5Status==10))
				_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"LOGGING INTO PROXY %s:%d",ConPort[i].S5RemoteIP,ConPort[i].S5Port);
			else if((ConPort[i].S5Connect)&&(ConPort[i].S5Status<100))
				_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"CONNECTING TO REMOTE VIA %s:%d",ConPort[i].S5RemoteIP,ConPort[i].S5Port);
			else if(!ConPort[i].SockOpen)
			{
			#ifdef WS_DECLINING_RECONNECT
				if(ConPort[i].ReconnectTime)
					_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"CONNECT DELAYED %u ms",ConPort[i].ReconnectTime -GetTickCount());
				else
			#endif
					_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"INACTIVE");
			}
		#ifdef WS_BROADCAST_RECEIVER
			else if(ConPort[i].broadcastRepairCnt>0)
				_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"REPAIR %d",ConPort[i].broadcastRepairCnt);
		#endif
			else if((ConPort[i].RecvBuffLimit>0)&&(ConPort[i].TBufferSize +ConPort[i].InBuffer.Size>=(DWORD)ConPort[i].RecvBuffLimit*1024))
				strcpy(ConPort[i].Status,"BUFFER FULL");
			else if(*ConPort[i].OnLineStatusText)
				strcpy(ConPort[i].Status, ConPort[i].OnLineStatusText);
			else
				strcpy(ConPort[i].Status,/*"CONNECTED"*/"");
			if(ConPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -ConPort[i].SendTimeOut>=1000)
					_snprintf(ConPort[i].Status,sizeof(ConPort[i].Status),"%sRetry:%d",ConPort[i].Status,(GetTickCount() -ConPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_CON,i);
			}
			ConPort[i].Seconds++;
			ConPort[i].BytesPerSecIn=(DWORD)(ConPort[i].BytesIn-ConPort[i].BytesLastIn);
			#ifdef WS_OTPP
			ConPort[i].BytesPerSecIn+=ConPort[i].TBufferSize;
			#endif
			if(ConPort[i].BytesPerSecIn>ConPort[i].BytesPerSecMaxIn)
				ConPort[i].BytesPerSecMaxIn=ConPort[i].BytesPerSecIn;
			ConPort[i].BytesPerSecAvgIn=(DWORD)(ConPort[i].BytesIn/ConPort[i].Seconds);
			ConPort[i].BytesLastIn=ConPort[i].BytesIn;
			ConPort[i].BytesPerSecOut=(DWORD)(ConPort[i].BytesOut-ConPort[i].BytesLastOut);
			if(ConPort[i].BytesPerSecOut>ConPort[i].BytesPerSecMaxOut)
				ConPort[i].BytesPerSecMaxOut=ConPort[i].BytesPerSecOut;
			ConPort[i].BytesPerSecAvgOut=(DWORD)(ConPort[i].BytesOut/ConPort[i].Seconds);
			ConPort[i].BytesLastOut=ConPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,ConPort[i].RemoteIP,ConPort[i].Port
				,ConPort[i].Note
				,ConPort[i].Status);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,ConPort[i].BeatsIn
				,SS(ConPort[i].InBuffer.Size)
				,SS(ConPort[i].BytesPerSecIn)
				,SS(ConPort[i].BytesPerSecAvgIn)
				,SS(ConPort[i].BytesPerSecMaxIn)
				,ConPort[i].BlocksIn
				,ConPort[i].PacketsIn
				,SS(ConPort[i].BytesIn)
				);
		printf("\n");
#endif
			ConPort[NO_OF_CON_PORTS].BeatsIn+=ConPort[i].BeatsIn;
			ConPort[NO_OF_CON_PORTS].InBuffer.Size+=ConPort[i].InBuffer.Size;
			ConPort[NO_OF_CON_PORTS].BlocksIn+=ConPort[i].BlocksIn;
			ConPort[NO_OF_CON_PORTS].PacketsIn+=ConPort[i].PacketsIn;
			ConPort[NO_OF_CON_PORTS].BytesIn+=ConPort[i].BytesPerSecIn;

			ConPort[NO_OF_CON_PORTS].BeatsOut+=ConPort[i].BeatsOut;
			ConPort[NO_OF_CON_PORTS].OutBuffer.Size+=ConPort[i].OutBuffer.Size;
			ConPort[NO_OF_CON_PORTS].BlocksOut+=ConPort[i].BlocksOut;
			ConPort[NO_OF_CON_PORTS].PacketsOut+=ConPort[i].PacketsOut;
			ConPort[NO_OF_CON_PORTS].BytesOut+=ConPort[i].BytesPerSecOut;
			#ifdef WS_OTPP
			ConPort[NO_OF_CON_PORTS].IOCPSendBytes+=ConPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			ConPort[NO_OF_CON_PORTS].IOCPSendBytes+=ConPort[i].IOCPSendBytes;
			#endif
#ifdef WS_BROADCAST_RECEIVER
			ConPort[NO_OF_CON_PORTS].broadcastRepairCnt+=ConPort[i].broadcastRepairCnt;
			// Let the app decrement the number of outstanding repairs
			//ConPort[i].broadcastRepairCnt=0;
#endif

		}
		else if(ConPort[i].RemoteIP[0])
		{
			strcpy(ConPort[i].Status,"DISABLED ");
			sprintf(ConPort[i].Note,"[%s]",ConPort[i].CfgNote);
		}
	}
	strcpy(ConPort[NO_OF_CON_PORTS].Status, ConPort[NO_OF_CON_PORTS].OnLineStatusText);
	ConPort[NO_OF_CON_PORTS].Seconds++;
	ConPort[NO_OF_CON_PORTS].BytesPerSecIn=(DWORD)(ConPort[NO_OF_CON_PORTS].BytesIn-ConPort[NO_OF_CON_PORTS].BytesLastIn);
	ConPort[NO_OF_CON_PORTS].BytesLastIn=ConPort[NO_OF_CON_PORTS].BytesIn;
	ConPort[NO_OF_CON_PORTS].BytesPerSecAvgIn=(DWORD)(ConPort[NO_OF_CON_PORTS].BytesIn/ConPort[NO_OF_CON_PORTS].Seconds);
	if(ConPort[NO_OF_CON_PORTS].BytesPerSecIn>ConPort[NO_OF_CON_PORTS].BytesPerSecMaxIn)
		ConPort[NO_OF_CON_PORTS].BytesPerSecMaxIn=ConPort[NO_OF_CON_PORTS].BytesPerSecIn;

	ConPort[NO_OF_CON_PORTS].BytesPerSecOut=(DWORD)(ConPort[NO_OF_CON_PORTS].BytesOut-ConPort[NO_OF_CON_PORTS].BytesLastOut);
	ConPort[NO_OF_CON_PORTS].BytesLastOut=ConPort[NO_OF_CON_PORTS].BytesOut;
	ConPort[NO_OF_CON_PORTS].BytesPerSecAvgOut=(DWORD)(ConPort[NO_OF_CON_PORTS].BytesOut/ConPort[NO_OF_CON_PORTS].Seconds);
	if(ConPort[NO_OF_CON_PORTS].BytesPerSecOut>ConPort[NO_OF_CON_PORTS].BytesPerSecMaxOut)
		ConPort[NO_OF_CON_PORTS].BytesPerSecMaxOut=ConPort[NO_OF_CON_PORTS].BytesPerSecOut;

#ifdef WS_GUARANTEED
	szTemp[0]=0;
	CgdPort[NO_OF_CGD_PORTS].BeatsIn=0;
	CgdPort[NO_OF_CGD_PORTS].InBuffer.Size=0;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecIn=0;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecAvgIn=0;
	CgdPort[NO_OF_CGD_PORTS].BlocksIn=0;
	CgdPort[NO_OF_CGD_PORTS].PacketsIn=0;

	CgdPort[NO_OF_CGD_PORTS].BeatsOut=0;
	CgdPort[NO_OF_CGD_PORTS].OutBuffer.Size=0;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecOut=0;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecAvgOut=0;
	CgdPort[NO_OF_CGD_PORTS].BlocksOut=0;
	CgdPort[NO_OF_CGD_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	CgdPort[NO_OF_CGD_PORTS].IOCPSendBytes=0;
	#elif defined(WS_COMPLETION_PORT_SENDS)
	CgdPort[NO_OF_CGD_PORTS].IOCPSendBytes=0;
	#endif

	for(i=0;i<NO_OF_CGD_PORTS;i++)
	{
		if(CgdPort[i].InUse)
		{
			NoOfCgds++;
			if(!pdoc->pmod->initialized)
				strcpy(CgdPort[i].Status,"LOADING ");
			else if (CgdPort[i].ConnectHold)
				strcpy(CgdPort[i].Status,"HOLDING ");
			else if (CgdPort[i].ReconCount)
			{
				if((CgdPort[i].S5Connect)&&(CgdPort[i].S5Status==0))
					_snprintf(CgdPort[i].Status,sizeof(CgdPort[i].Status),"CONNECTING TO PROXY %s:%d",CgdPort[i].S5RemoteIP,CgdPort[i].S5Port);
				else
					strcpy(CgdPort[i].Status,"CONNECTING ");
			}
			else if((CgdPort[i].S5Connect)&&(CgdPort[i].S5Status==10))
				_snprintf(CgdPort[i].Status,sizeof(CgdPort[i].Status),"LOGGING INTO PROXY %s:%d",CgdPort[i].S5RemoteIP,CgdPort[i].S5Port);
			else if((CgdPort[i].S5Connect)&&(CgdPort[i].S5Status<100))
				_snprintf(CgdPort[i].Status,sizeof(CgdPort[i].Status),"CONNECTING TO REMOTE VIA %s:%d",CgdPort[i].S5RemoteIP,CgdPort[i].S5Port);
			else if(!CgdPort[i].SockOpen)
			{
			#ifdef WS_DECLINING_RECONNECT
				if(CgdPort[i].ReconnectTime)
					_snprintf(CgdPort[i].Status,sizeof(CgdPort[i].Status),"CONNECT DELAYED %u ms",CgdPort[i].ReconnectTime -GetTickCount());
				else
			#endif
					_snprintf(CgdPort[i].Status,sizeof(CgdPort[i].Status),"INACTIVE");
			}
			else if((CgdPort[i].RecvBuffLimit>0)&&(CgdPort[i].TBufferSize +CgdPort[i].InBuffer.Size>=(DWORD)CgdPort[i].RecvBuffLimit*1024))
				strcpy(CgdPort[i].Status,"BUFFER FULL");
			else if(*CgdPort[i].OnLineStatusText)
				sprintf(CgdPort[i].Status, "%s (o=%d,l=%d,i=%d)", CgdPort[i].OnLineStatusText, CgdPort[i].NextGDOutLogId, CgdPort[i].LastGDSendLogId, CgdPort[i].NextGDInLogId);
			else
				sprintf(CgdPort[i].Status, "CONNECTED (o=%d,l=%d,i=%d)", CgdPort[i].NextGDOutLogId, CgdPort[i].LastGDSendLogId, CgdPort[i].NextGDInLogId);
			if(CgdPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -CgdPort[i].SendTimeOut>=1000)
					_snprintf(CgdPort[i].Status,sizeof(CgdPort[i].Status),"%sRetry:%d",CgdPort[i].Status,(GetTickCount() -CgdPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_CGD,i);
			}
			CgdPort[i].Seconds++;
			CgdPort[i].BytesPerSecIn=(DWORD)(CgdPort[i].BytesIn-CgdPort[i].BytesLastIn);
			#ifdef WS_OTPP
			CgdPort[i].BytesPerSecIn+=CgdPort[i].TBufferSize;
			#endif
			if(CgdPort[i].BytesPerSecIn>CgdPort[i].BytesPerSecMaxIn)
				CgdPort[i].BytesPerSecMaxIn=CgdPort[i].BytesPerSecIn;
			CgdPort[i].BytesPerSecAvgIn=(DWORD)(CgdPort[i].BytesIn/CgdPort[i].Seconds);
			CgdPort[i].BytesLastIn=CgdPort[i].BytesIn;
			CgdPort[i].BytesPerSecOut=(DWORD)(CgdPort[i].BytesOut-CgdPort[i].BytesLastOut);
			if(CgdPort[i].BytesPerSecOut>CgdPort[i].BytesPerSecMaxOut)
				CgdPort[i].BytesPerSecMaxOut=CgdPort[i].BytesPerSecOut;
			CgdPort[i].BytesPerSecAvgOut=(DWORD)(CgdPort[i].BytesOut/CgdPort[i].Seconds);
			CgdPort[i].BytesLastOut=CgdPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,CgdPort[i].RemoteIP,CgdPort[i].Port
				,CgdPort[i].Note
					,CgdPort[i].Status);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,CgdPort[i].BeatsIn
				,SS(CgdPort[i].InBuffer.Size)
				,SS(CgdPort[i].BytesPerSecIn)
				,SS(CgdPort[i].BytesPerSecAvgIn)
				,SS(CgdPort[i].BytesPerSecMaxIn)
				,CgdPort[i].BlocksIn
				,CgdPort[i].PacketsIn
				,SS(CgdPort[i].BytesIn)
				);
		printf("\n");
#endif
			CgdPort[NO_OF_CGD_PORTS].BeatsIn+=CgdPort[i].BeatsIn;
			CgdPort[NO_OF_CGD_PORTS].InBuffer.Size+=CgdPort[i].InBuffer.Size;
			CgdPort[NO_OF_CGD_PORTS].BlocksIn+=CgdPort[i].BlocksIn;
			CgdPort[NO_OF_CGD_PORTS].PacketsIn+=CgdPort[i].PacketsIn;
			CgdPort[NO_OF_CGD_PORTS].BytesIn+=CgdPort[i].BytesPerSecIn;

			CgdPort[NO_OF_CGD_PORTS].BeatsOut+=CgdPort[i].BeatsOut;
			CgdPort[NO_OF_CGD_PORTS].OutBuffer.Size+=CgdPort[i].OutBuffer.Size;
			CgdPort[NO_OF_CGD_PORTS].BlocksOut+=CgdPort[i].BlocksOut;
			CgdPort[NO_OF_CGD_PORTS].PacketsOut+=CgdPort[i].PacketsOut;
			CgdPort[NO_OF_CGD_PORTS].BytesOut+=CgdPort[i].BytesPerSecOut;
			#ifdef WS_OTPP
			CgdPort[NO_OF_CGD_PORTS].IOCPSendBytes+=CgdPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			CgdPort[NO_OF_CGD_PORTS].IOCPSendBytes+=CgdPort[i].IOCPSendBytes;
			#endif
		}
		else if(CgdPort[i].RemoteIP[0])
		{
			strcpy(CgdPort[i].Status,"DISABLED ");
			sprintf(CgdPort[i].Note,"[%s]",CgdPort[i].CfgNote);
		}
	}
	CgdPort[NO_OF_CGD_PORTS].Seconds++;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecIn=(DWORD)(CgdPort[NO_OF_CGD_PORTS].BytesIn-CgdPort[NO_OF_CGD_PORTS].BytesLastIn);
	CgdPort[NO_OF_CGD_PORTS].BytesLastIn=CgdPort[NO_OF_CGD_PORTS].BytesIn;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecAvgIn=(DWORD)(CgdPort[NO_OF_CGD_PORTS].BytesIn/CgdPort[NO_OF_CGD_PORTS].Seconds);
	if(CgdPort[NO_OF_CGD_PORTS].BytesPerSecIn>CgdPort[NO_OF_CGD_PORTS].BytesPerSecMaxIn)
		CgdPort[NO_OF_CGD_PORTS].BytesPerSecMaxIn=CgdPort[NO_OF_CGD_PORTS].BytesPerSecIn;

	CgdPort[NO_OF_CGD_PORTS].BytesPerSecOut=(DWORD)(CgdPort[NO_OF_CGD_PORTS].BytesOut-CgdPort[NO_OF_CGD_PORTS].BytesLastOut);
	CgdPort[NO_OF_CGD_PORTS].BytesLastOut=CgdPort[NO_OF_CGD_PORTS].BytesOut;
	CgdPort[NO_OF_CGD_PORTS].BytesPerSecAvgOut=(DWORD)(CgdPort[NO_OF_CGD_PORTS].BytesOut/CgdPort[NO_OF_CGD_PORTS].Seconds);
	if(CgdPort[NO_OF_CGD_PORTS].BytesPerSecOut>CgdPort[NO_OF_CGD_PORTS].BytesPerSecMaxOut)
		CgdPort[NO_OF_CGD_PORTS].BytesPerSecMaxOut=CgdPort[NO_OF_CGD_PORTS].BytesPerSecOut;
#endif //#ifdef WS_GUARANTEED

#ifdef WS_FILE_SERVER
	szTemp[0]=0;
	FilePort[NO_OF_FILE_PORTS].BeatsIn=0;
	FilePort[NO_OF_FILE_PORTS].InBuffer.Size=0;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecIn=0;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecAvgIn=0;
	FilePort[NO_OF_FILE_PORTS].BlocksIn=0;
	FilePort[NO_OF_FILE_PORTS].PacketsIn=0;

	FilePort[NO_OF_FILE_PORTS].BeatsOut=0;
	FilePort[NO_OF_FILE_PORTS].OutBuffer.Size=0;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecOut=0;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecAvgOut=0;
	FilePort[NO_OF_FILE_PORTS].BlocksOut=0;
	FilePort[NO_OF_FILE_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	FilePort[NO_OF_FILE_PORTS].IOCPSendBytes=0;
	#elif defined(WS_COMPLETION_PORT_SENDS)
	FilePort[NO_OF_FILE_PORTS].IOCPSendBytes=0;
	#endif

	for(i=0;i<NO_OF_FILE_PORTS;i++)
	{
		if(FilePort[i].InUse)
		{
			NoOfFiles++;
			if(!pdoc->pmod->initialized)
				strcpy(FilePort[i].Status,"LOADING ");
			else if (FilePort[i].ConnectHold)
				strcpy(FilePort[i].Status,"HOLDING ");
			else if (FilePort[i].ReconCount)
			{
				if((FilePort[i].S5Connect)&&(FilePort[i].S5Status==0))
					_snprintf(FilePort[i].Status,sizeof(FilePort[i].Status),"CONNECTING TO PROXY %s:%d",FilePort[i].S5RemoteIP,FilePort[i].S5Port);
				else
					strcpy(FilePort[i].Status,"CONNECTING ");
			}
			else if((FilePort[i].S5Connect)&&(FilePort[i].S5Status==10))
				_snprintf(FilePort[i].Status,sizeof(FilePort[i].Status),"LOGGING INTO PROXY %s:%d",FilePort[i].S5RemoteIP,FilePort[i].S5Port);
			else if((FilePort[i].S5Connect)&&(FilePort[i].S5Status<100))
				_snprintf(FilePort[i].Status,sizeof(FilePort[i].Status),"CONNECTING TO REMOTE VIA %s:%d",FilePort[i].S5RemoteIP,FilePort[i].S5Port);
			else if(!FilePort[i].SockOpen)
			{
			#ifdef WS_DECLINING_RECONNECT
				if(FilePort[i].ReconnectTime)
					_snprintf(FilePort[i].Status,sizeof(FilePort[i].Status),"CONNECT DELAYED %u ms",FilePort[i].ReconnectTime -GetTickCount());
				else
			#endif
					_snprintf(FilePort[i].Status,sizeof(FilePort[i].Status),"INACTIVE");
			}
			else if((FilePort[i].RecvBuffLimit>0)&&(FilePort[i].TBufferSize +FilePort[i].InBuffer.Size>=(DWORD)FilePort[i].RecvBuffLimit*1024))
				strcpy(FilePort[i].Status,"BUFFER FULL");
			else if(*FilePort[i].OnLineStatusText)
				strcpy(FilePort[i].Status, FilePort[i].OnLineStatusText);
			else
				strcpy(FilePort[i].Status,/*"CONNECTED"*/"");
			if(FilePort[i].SendTimeOut>0)
			{
				if(GetTickCount() -FilePort[i].SendTimeOut>=1000)
					_snprintf(FilePort[i].Status,sizeof(FilePort[i].Status),"%sRetry:%d",FilePort[i].Status,(GetTickCount() -FilePort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_FIL,i);
			}
			FilePort[i].Seconds++;
			FilePort[i].BytesPerSecIn=(DWORD)(FilePort[i].BytesIn-FilePort[i].BytesLastIn);
			#ifdef WS_OTPP
			FilePort[i].BytesPerSecIn+=FilePort[i].TBufferSize;
			#endif
			if(FilePort[i].BytesPerSecIn>FilePort[i].BytesPerSecMaxIn)
				FilePort[i].BytesPerSecMaxIn=FilePort[i].BytesPerSecIn;
			FilePort[i].BytesPerSecAvgIn=(DWORD)(FilePort[i].BytesIn/FilePort[i].Seconds);
			FilePort[i].BytesLastIn=FilePort[i].BytesIn;
			FilePort[i].BytesPerSecOut=(DWORD)(FilePort[i].BytesOut-FilePort[i].BytesLastOut);
			if(FilePort[i].BytesPerSecOut>FilePort[i].BytesPerSecMaxOut)
				FilePort[i].BytesPerSecMaxOut=FilePort[i].BytesPerSecOut;
			FilePort[i].BytesPerSecAvgOut=(DWORD)(FilePort[i].BytesOut/FilePort[i].Seconds);
			FilePort[i].BytesLastOut=FilePort[i].BytesOut;
#ifdef UNIX
			printf("F%03d|%s:%d|%s|%s|",i,FilePort[i].RemoteIP,FilePort[i].Port
				,FilePort[i].Note
					,FilePort[i].Status);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,FilePort[i].BeatsIn
				,SS(FilePort[i].InBuffer.Size)
				,SS(FilePort[i].BytesPerSecIn)
				,SS(FilePort[i].BytesPerSecAvgIn)
				,SS(FilePort[i].BytesPerSecMaxIn)
				,FilePort[i].BlocksIn
				,FilePort[i].PacketsIn
				,SS(FilePort[i].BytesIn)
				);
		printf("\n");
#endif
			FilePort[NO_OF_FILE_PORTS].BeatsIn+=FilePort[i].BeatsIn;
			FilePort[NO_OF_FILE_PORTS].InBuffer.Size+=FilePort[i].InBuffer.Size;
			FilePort[NO_OF_FILE_PORTS].BlocksIn+=FilePort[i].BlocksIn;
			FilePort[NO_OF_FILE_PORTS].PacketsIn+=FilePort[i].PacketsIn;
			FilePort[NO_OF_FILE_PORTS].BytesIn+=FilePort[i].BytesPerSecIn;

			FilePort[NO_OF_FILE_PORTS].BeatsOut+=FilePort[i].BeatsOut;
			FilePort[NO_OF_FILE_PORTS].OutBuffer.Size+=FilePort[i].OutBuffer.Size;
			FilePort[NO_OF_FILE_PORTS].BlocksOut+=FilePort[i].BlocksOut;
			FilePort[NO_OF_FILE_PORTS].PacketsOut+=FilePort[i].PacketsOut;
			FilePort[NO_OF_FILE_PORTS].BytesOut+=FilePort[i].BytesPerSecOut;
			#ifdef WS_OTPP
			FilePort[NO_OF_FILE_PORTS].IOCPSendBytes+=FilePort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			FilePort[NO_OF_FILE_PORTS].IOCPSendBytes+=FilePort[i].IOCPSendBytes;
			#endif
		}
		else if(FilePort[i].RemoteIP[0])
		{
			strcpy(FilePort[i].Status,"DISABLED ");
			sprintf(FilePort[i].Note,"[%s]",FilePort[i].CfgNote);
		}
	}
	FilePort[NO_OF_FILE_PORTS].Seconds++;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecIn=(DWORD)(FilePort[NO_OF_FILE_PORTS].BytesIn-FilePort[NO_OF_FILE_PORTS].BytesLastIn);
	FilePort[NO_OF_FILE_PORTS].BytesLastIn=FilePort[NO_OF_FILE_PORTS].BytesIn;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecAvgIn=(DWORD)(FilePort[NO_OF_FILE_PORTS].BytesIn/FilePort[NO_OF_FILE_PORTS].Seconds);
	if(FilePort[NO_OF_FILE_PORTS].BytesPerSecIn>FilePort[NO_OF_FILE_PORTS].BytesPerSecMaxIn)
		FilePort[NO_OF_FILE_PORTS].BytesPerSecMaxIn=FilePort[NO_OF_FILE_PORTS].BytesPerSecIn;

	FilePort[NO_OF_FILE_PORTS].BytesPerSecOut=(DWORD)(FilePort[NO_OF_FILE_PORTS].BytesOut-FilePort[NO_OF_FILE_PORTS].BytesLastOut);
	FilePort[NO_OF_FILE_PORTS].BytesLastOut=FilePort[NO_OF_FILE_PORTS].BytesOut;
	FilePort[NO_OF_FILE_PORTS].BytesPerSecAvgOut=(DWORD)(FilePort[NO_OF_FILE_PORTS].BytesOut/FilePort[NO_OF_FILE_PORTS].Seconds);
	if(FilePort[NO_OF_FILE_PORTS].BytesPerSecOut>FilePort[NO_OF_FILE_PORTS].BytesPerSecMaxOut)
		FilePort[NO_OF_FILE_PORTS].BytesPerSecMaxOut=FilePort[NO_OF_FILE_PORTS].BytesPerSecOut;
#endif //#ifdef WS_FILE_SERVER

#ifdef WS_CAST
	CtoPort[NO_OF_CTO_PORTS].BeatsIn=0;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecIn=0;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecAvgIn=0;
	CtoPort[NO_OF_CTO_PORTS].BlocksIn=0;
	CtoPort[NO_OF_CTO_PORTS].PacketsIn=0;

	CtoPort[NO_OF_CTO_PORTS].BeatsOut=0;
	CtoPort[NO_OF_CTO_PORTS].OutBuffer.Size=0;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecOut=0;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecAvgOut=0;
	CtoPort[NO_OF_CTO_PORTS].BlocksOut=0;
	CtoPort[NO_OF_CTO_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	CtoPort[NO_OF_CTO_PORTS].IOCPSendBytes=0;
	#endif

	for(i=0;i<NO_OF_CTO_PORTS;i++)
	{
		if(CtoPort[i].InUse)
		{
			NoOfCtos++;
			if(!pdoc->pmod->initialized)
				strcpy(CtoPort[i].Status,"LOADING ");
			else if (CtoPort[i].ConnectHold)
				strcpy(CtoPort[i].Status,"HOLDING ");
			else
				strcpy(CtoPort[i].Status, CtoPort[i].OnLineStatusText);
			if(CtoPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -CtoPort[i].SendTimeOut>=1000)
					_snprintf(CtoPort[i].Status,sizeof(CtoPort[i].Status),"%sRetry:%d",CtoPort[i].Status,(GetTickCount() -CtoPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_CTO,i);
			}
			CtoPort[i].Seconds++;
			CtoPort[i].BytesPerSecIn=(DWORD)(CtoPort[i].BytesIn-CtoPort[i].BytesLastIn);
			if(CtoPort[i].BytesPerSecIn>CtoPort[i].BytesPerSecMaxIn)
				CtoPort[i].BytesPerSecMaxIn=CtoPort[i].BytesPerSecIn;
			CtoPort[i].BytesPerSecAvgIn=(DWORD)(CtoPort[i].BytesIn/CtoPort[i].Seconds);
			CtoPort[i].BytesLastIn=CtoPort[i].BytesIn;
			CtoPort[i].BytesPerSecOut=(DWORD)(CtoPort[i].BytesOut-CtoPort[i].BytesLastOut);
			if(CtoPort[i].BytesPerSecOut>CtoPort[i].BytesPerSecMaxOut)
				CtoPort[i].BytesPerSecMaxOut=CtoPort[i].BytesPerSecOut;
			CtoPort[i].BytesPerSecAvgOut=(DWORD)(CtoPort[i].BytesOut/CtoPort[i].Seconds);
			CtoPort[i].BytesLastOut=CtoPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,CtoPort[i].RemoteIP,CtoPort[i].Port
				,CtoPort[i].Note
					,CtoPort[i].Status);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,CtoPort[i].BeatsIn
				,SS(CtoPort[i].InBuffer.Size)
				,SS(CtoPort[i].BytesPerSecIn)
				,SS(CtoPort[i].BytesPerSecAvgIn)
				,SS(CtoPort[i].BytesPerSecMaxIn)
				,CtoPort[i].BlocksIn
				,CtoPort[i].PacketsIn
				,SS(CtoPort[i].BytesIn)
				);
		printf("\n");
#endif
			CtoPort[NO_OF_CTO_PORTS].BeatsIn+=CtoPort[i].BeatsIn;
			CtoPort[NO_OF_CTO_PORTS].BlocksIn+=CtoPort[i].BlocksIn;
			CtoPort[NO_OF_CTO_PORTS].PacketsIn+=CtoPort[i].PacketsIn;
			CtoPort[NO_OF_CTO_PORTS].BytesIn+=CtoPort[i].BytesPerSecIn;

			CtoPort[NO_OF_CTO_PORTS].BeatsOut+=CtoPort[i].BeatsOut;
			CtoPort[NO_OF_CTO_PORTS].OutBuffer.Size+=CtoPort[i].OutBuffer.Size;
			CtoPort[NO_OF_CTO_PORTS].BlocksOut+=CtoPort[i].BlocksOut;
			CtoPort[NO_OF_CTO_PORTS].PacketsOut+=CtoPort[i].PacketsOut;
			CtoPort[NO_OF_CTO_PORTS].BytesOut+=CtoPort[i].BytesPerSecOut;
			#ifdef WS_OTPP
			CtoPort[NO_OF_CTO_PORTS].IOCPSendBytes+=CtoPort[i].IOCPSendBytes;
			#endif
		}
	}
	CtoPort[NO_OF_CTO_PORTS].Seconds++;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecIn=(DWORD)(CtoPort[NO_OF_CTO_PORTS].BytesIn-CtoPort[NO_OF_CTO_PORTS].BytesLastIn);
	CtoPort[NO_OF_CTO_PORTS].BytesLastIn=CtoPort[NO_OF_CTO_PORTS].BytesIn;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecAvgIn=(DWORD)(CtoPort[NO_OF_CTO_PORTS].BytesIn/CtoPort[NO_OF_CTO_PORTS].Seconds);
	if(CtoPort[NO_OF_CTO_PORTS].BytesPerSecIn>CtoPort[NO_OF_CTO_PORTS].BytesPerSecMaxIn)
		CtoPort[NO_OF_CTO_PORTS].BytesPerSecMaxIn=CtoPort[NO_OF_CTO_PORTS].BytesPerSecIn;

	CtoPort[NO_OF_CTO_PORTS].BytesPerSecOut=(DWORD)(CtoPort[NO_OF_CTO_PORTS].BytesOut-CtoPort[NO_OF_CTO_PORTS].BytesLastOut);
	CtoPort[NO_OF_CTO_PORTS].BytesLastOut=CtoPort[NO_OF_CTO_PORTS].BytesOut;
	CtoPort[NO_OF_CTO_PORTS].BytesPerSecAvgOut=(DWORD)(CtoPort[NO_OF_CTO_PORTS].BytesOut/CtoPort[NO_OF_CTO_PORTS].Seconds);
	if(CtoPort[NO_OF_CTO_PORTS].BytesPerSecOut>CtoPort[NO_OF_CTO_PORTS].BytesPerSecMaxOut)
		CtoPort[NO_OF_CTO_PORTS].BytesPerSecMaxOut=CtoPort[NO_OF_CTO_PORTS].BytesPerSecOut;
////////
	CtiPort[NO_OF_CTI_PORTS].BeatsIn=0;
	CtiPort[NO_OF_CTI_PORTS].InBuffer.Size=0;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecIn=0;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecAvgIn=0;
	CtiPort[NO_OF_CTI_PORTS].BlocksIn=0;
	CtiPort[NO_OF_CTI_PORTS].PacketsIn=0;

	CtiPort[NO_OF_CTI_PORTS].BeatsOut=0;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecOut=0;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecAvgOut=0;
	CtiPort[NO_OF_CTI_PORTS].BlocksOut=0;
	CtiPort[NO_OF_CTI_PORTS].PacketsOut=0;

	for(i=0;i<NO_OF_CTI_PORTS;i++)
	{
		if(CtiPort[i].InUse)
		{
			NoOfCtis++;
			if(!pdoc->pmod->initialized)
				strcpy(CtiPort[i].Status,"LOADING ");
			else if (CtiPort[i].ConnectHold)
				strcpy(CtiPort[i].Status,"HOLDING ");
			else if (CtiPort[i].ReconCount)
				strcpy(CtiPort[i].Status,"CONNECTING ");
#ifdef WS_CTI_ONLINESTATUSTEXT_FIRST
			if(CtiPort[i].OnLineStatusText[0])
				strcpy(CtiPort[i].Status, CtiPort[i].OnLineStatusText);
#else
			else
				strcpy(CtiPort[i].Status, CtiPort[i].OnLineStatusText);
#endif
			if(CtiPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -CtiPort[i].SendTimeOut>=1000)
					_snprintf(CtiPort[i].Status,sizeof(CtiPort[i].Status),"%sRetry:%d",CtiPort[i].Status,(GetTickCount() -CtiPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_CTI,i);
			}
			CtiPort[i].Seconds++;
			CtiPort[i].BytesPerSecIn=(DWORD)(CtiPort[i].BytesIn-CtiPort[i].BytesLastIn);
			#ifdef WS_OTPP
			CtiPort[i].BytesPerSecIn+=CtiPort[i].TBufferSize;
			#endif
			if(CtiPort[i].BytesPerSecIn>CtiPort[i].BytesPerSecMaxIn)
				CtiPort[i].BytesPerSecMaxIn=CtiPort[i].BytesPerSecIn;
			CtiPort[i].BytesPerSecAvgIn=(DWORD)(CtiPort[i].BytesIn/CtiPort[i].Seconds);
			CtiPort[i].BytesLastIn=CtiPort[i].BytesIn;
			CtiPort[i].BytesPerSecOut=(DWORD)(CtiPort[i].BytesOut-CtiPort[i].BytesLastOut);
			if(CtiPort[i].BytesPerSecOut>CtiPort[i].BytesPerSecMaxOut)
				CtiPort[i].BytesPerSecMaxOut=CtiPort[i].BytesPerSecOut;
			CtiPort[i].BytesPerSecAvgOut=(DWORD)(CtiPort[i].BytesOut/CtiPort[i].Seconds);
			CtiPort[i].BytesLastOut=CtiPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,CtiPort[i].RemoteIP,CtiPort[i].Port
				,CtiPort[i].Note
				,CtiPort[i].OnLineStatusText);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,CtiPort[i].BeatsIn
				,SS(CtiPort[i].InBuffer.Size)
				,SS(CtiPort[i].BytesPerSecIn)
				,SS(CtiPort[i].BytesPerSecAvgIn)
				,SS(CtiPort[i].BytesPerSecMaxIn)
				,CtiPort[i].BlocksIn
				,CtiPort[i].PacketsIn
				,SS(CtiPort[i].BytesIn)
				);
		printf("\n");
#endif
			CtiPort[NO_OF_CTI_PORTS].BeatsIn+=CtiPort[i].BeatsIn;
			CtiPort[NO_OF_CTI_PORTS].InBuffer.Size+=CtiPort[i].InBuffer.Size;
			CtiPort[NO_OF_CTI_PORTS].BlocksIn+=CtiPort[i].BlocksIn;
			CtiPort[NO_OF_CTI_PORTS].PacketsIn+=CtiPort[i].PacketsIn;
			CtiPort[NO_OF_CTI_PORTS].BytesIn+=CtiPort[i].BytesPerSecIn;

			CtiPort[NO_OF_CTI_PORTS].BeatsOut+=CtiPort[i].BeatsOut;
			CtiPort[NO_OF_CTI_PORTS].BlocksOut+=CtiPort[i].BlocksOut;
			CtiPort[NO_OF_CTI_PORTS].PacketsOut+=CtiPort[i].PacketsOut;
			CtiPort[NO_OF_CTI_PORTS].BytesOut+=CtiPort[i].BytesPerSecOut;
		}
	}
	strcpy(CtiPort[NO_OF_CTI_PORTS].Status,CtiPort[NO_OF_CTI_PORTS].OnLineStatusText);
	CtiPort[NO_OF_CTI_PORTS].Seconds++;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecIn=(DWORD)(CtiPort[NO_OF_CTI_PORTS].BytesIn-CtiPort[NO_OF_CTI_PORTS].BytesLastIn);
	CtiPort[NO_OF_CTI_PORTS].BytesLastIn=CtiPort[NO_OF_CTI_PORTS].BytesIn;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecAvgIn=(DWORD)(CtiPort[NO_OF_CTI_PORTS].BytesIn/CtiPort[NO_OF_CTI_PORTS].Seconds);
	if(CtiPort[NO_OF_CTI_PORTS].BytesPerSecIn>CtiPort[NO_OF_CTI_PORTS].BytesPerSecMaxIn)
		CtiPort[NO_OF_CTI_PORTS].BytesPerSecMaxIn=CtiPort[NO_OF_CTI_PORTS].BytesPerSecIn;

	CtiPort[NO_OF_CTI_PORTS].BytesPerSecOut=(DWORD)(CtiPort[NO_OF_CTI_PORTS].BytesOut-CtiPort[NO_OF_CTI_PORTS].BytesLastOut);
	CtiPort[NO_OF_CTI_PORTS].BytesLastOut=CtiPort[NO_OF_CTI_PORTS].BytesOut;
	CtiPort[NO_OF_CTI_PORTS].BytesPerSecAvgOut=(DWORD)(CtiPort[NO_OF_CTI_PORTS].BytesOut/CtiPort[NO_OF_CTI_PORTS].Seconds);
	if(CtiPort[NO_OF_CTI_PORTS].BytesPerSecOut>CtiPort[NO_OF_CTI_PORTS].BytesPerSecMaxOut)
		CtiPort[NO_OF_CTI_PORTS].BytesPerSecMaxOut=CtiPort[NO_OF_CTI_PORTS].BytesPerSecOut;

#endif//#ifdef WS_CAST


	for(i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(UscPort[i].InUse)
		{
			UscPort[i].BeatsIn=0;
			UscPort[i].InBuffer_Size=0;
			UscPort[i].BytesPerSecIn=0;
			UscPort[i].BytesPerSecAvgIn=0;
			UscPort[i].BlocksIn=0;
			UscPort[i].PacketsIn=0;

			UscPort[i].BeatsOut=0;
			UscPort[i].OutBuffer_Size=0;
			UscPort[i].BytesPerSecOut=0;
			UscPort[i].BytesPerSecAvgOut=0;
			UscPort[i].BlocksOut=0;
			UscPort[i].PacketsOut=0;

			NoOfUscs++;
		}
	}
	UsrPort[NO_OF_USR_PORTS].BeatsIn=0;
	UsrPort[NO_OF_USR_PORTS].InBuffer.Size=0;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecIn=0;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecAvgIn=0;
	UsrPort[NO_OF_USR_PORTS].BlocksIn=0;
	UsrPort[NO_OF_USR_PORTS].PacketsIn=0;

	UsrPort[NO_OF_USR_PORTS].BeatsOut=0;
	UsrPort[NO_OF_USR_PORTS].OutBuffer.Size=0;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecOut=0;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecAvgOut=0;
	UsrPort[NO_OF_USR_PORTS].BlocksOut=0;
	UsrPort[NO_OF_USR_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	UsrPort[NO_OF_USR_PORTS].IOCPSendBytes=0;
	#elif defined(WS_COMPLETION_PORT_SENDS)
	UsrPort[NO_OF_USR_PORTS].IOCPSendBytes=0;
	#endif
#ifdef WS_BROADCAST_TRANSMITTER
	UsrPort[NO_OF_USR_PORTS].broadcastRepairCnt=0;
#endif

	for(i=0;i<NO_OF_USR_PORTS;i++)
	{
		if(UsrPort[i].SockActive)
		{
			NoOfUsers++;
			if(UsrPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -UsrPort[i].SendTimeOut>=1000)
					_snprintf(UsrPort[i].Status,sizeof(UsrPort[i].Status),"Retry:%d",(GetTickCount() -UsrPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_USR,i);
			}
		#ifdef WS_BROADCAST_TRANSMITTER
			else if(UsrPort[i].broadcastRepairCnt>0)
				_snprintf(UsrPort[i].Status,sizeof(UsrPort[i].Status),"Repair:%d",UsrPort[i].broadcastRepairCnt);
		#endif
			else if(!UsrPort[i].Status[0])
				strcpy(UsrPort[i].Status," ");
			UsrPort[i].Seconds++;
			UsrPort[i].BytesPerSecIn=(DWORD)(UsrPort[i].BytesIn-UsrPort[i].BytesLastIn);
			#ifdef WS_OTPP
			UsrPort[i].BytesPerSecIn+=UsrPort[i].TBufferSize;
			#endif
			if(UsrPort[i].BytesPerSecIn>UsrPort[i].BytesPerSecMaxIn)
				UsrPort[i].BytesPerSecMaxIn=UsrPort[i].BytesPerSecIn;
			UsrPort[i].BytesPerSecAvgIn=(DWORD)(UsrPort[i].BytesIn/UsrPort[i].Seconds);
			UsrPort[i].BytesLastIn=UsrPort[i].BytesIn;
			UsrPort[i].BytesPerSecOut=(DWORD)(UsrPort[i].BytesOut-UsrPort[i].BytesLastOut);
			if(UsrPort[i].BytesPerSecOut>UsrPort[i].BytesPerSecMaxOut)
				UsrPort[i].BytesPerSecMaxOut=UsrPort[i].BytesPerSecOut;
			UsrPort[i].BytesPerSecAvgOut=(DWORD)(UsrPort[i].BytesOut/UsrPort[i].Seconds);
			UsrPort[i].BytesLastOut=UsrPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,UsrPort[i].RemoteIP,UscPort[UsrPort[i].UscPort].Port
				,UsrPort[i].Note
				,"");
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,UsrPort[i].BeatsIn
				,SS(UsrPort[i].InBuffer.Size)
				,SS(UsrPort[i].BytesPerSecIn)
				,SS(UsrPort[i].BytesPerSecAvgIn)
				,SS(UsrPort[i].BytesPerSecMaxIn)
				,UsrPort[i].BlocksIn
				,UsrPort[i].PacketsIn
				,SS(UsrPort[i].BytesIn)
				);
		printf("\n");
#endif
			UsrPort[NO_OF_USR_PORTS].BeatsIn+=UsrPort[i].BeatsIn;
			UsrPort[NO_OF_USR_PORTS].InBuffer.Size+=UsrPort[i].InBuffer.Size;
			UsrPort[NO_OF_USR_PORTS].BlocksIn+=UsrPort[i].BlocksIn;
			UsrPort[NO_OF_USR_PORTS].PacketsIn+=UsrPort[i].PacketsIn;
			UsrPort[NO_OF_USR_PORTS].BytesIn+=UsrPort[i].BytesPerSecIn;

			UsrPort[NO_OF_USR_PORTS].BeatsOut+=UsrPort[i].BeatsOut;
			#ifdef WS_OTPP
			UsrPort[NO_OF_USR_PORTS].IOCPSendBytes+=UsrPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			UsrPort[NO_OF_USR_PORTS].IOCPSendBytes+=UsrPort[i].IOCPSendBytes;
			#endif
			UsrPort[NO_OF_USR_PORTS].OutBuffer.Size+=UsrPort[i].OutBuffer.Size;
			UsrPort[NO_OF_USR_PORTS].BlocksOut+=UsrPort[i].BlocksOut;
			UsrPort[NO_OF_USR_PORTS].PacketsOut+=UsrPort[i].PacketsOut;
			UsrPort[NO_OF_USR_PORTS].BytesOut+=UsrPort[i].BytesPerSecOut;

			UscPort[UsrPort[i].UscPort].BeatsIn+=UsrPort[i].BeatsIn;
			UscPort[UsrPort[i].UscPort].InBuffer_Size+=UsrPort[i].InBuffer.Size;
			UscPort[UsrPort[i].UscPort].BlocksIn+=UsrPort[i].BlocksIn;
			UscPort[UsrPort[i].UscPort].PacketsIn+=UsrPort[i].PacketsIn;
			UscPort[UsrPort[i].UscPort].BytesIn+=UsrPort[i].BytesPerSecIn;

			UscPort[UsrPort[i].UscPort].BeatsOut+=UsrPort[i].BeatsOut;
			#ifdef WS_OTPP
			UscPort[UsrPort[i].UscPort].OutBuffer_Size+=UsrPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			UscPort[UsrPort[i].UscPort].OutBuffer_Size+=UsrPort[i].IOCPSendBytes;
			#endif
			UscPort[UsrPort[i].UscPort].OutBuffer_Size+=UsrPort[i].OutBuffer.Size;
			UscPort[UsrPort[i].UscPort].BlocksOut+=UsrPort[i].BlocksOut;
			UscPort[UsrPort[i].UscPort].PacketsOut+=UsrPort[i].PacketsOut;
			UscPort[UsrPort[i].UscPort].BytesOut+=UsrPort[i].BytesPerSecOut;
		}
		else if(UsrPort[i].TimeTillClose>0)
			_snprintf(UsrPort[i].Status,sizeof(UsrPort[i].Status),"TimeTillClose:%d",UsrPort[i].TimeTillClose);
	}
	UsrPort[NO_OF_USR_PORTS].Seconds++;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecIn=(DWORD)(UsrPort[NO_OF_USR_PORTS].BytesIn-UsrPort[NO_OF_USR_PORTS].BytesLastIn);
	UsrPort[NO_OF_USR_PORTS].BytesLastIn=UsrPort[NO_OF_USR_PORTS].BytesIn;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecAvgIn=(DWORD)(UsrPort[NO_OF_USR_PORTS].BytesIn/UsrPort[NO_OF_USR_PORTS].Seconds);
	if(UsrPort[NO_OF_USR_PORTS].BytesPerSecIn>UsrPort[NO_OF_USR_PORTS].BytesPerSecMaxIn)
		UsrPort[NO_OF_USR_PORTS].BytesPerSecMaxIn=UsrPort[NO_OF_USR_PORTS].BytesPerSecIn;

	UsrPort[NO_OF_USR_PORTS].BytesPerSecOut=(DWORD)(UsrPort[NO_OF_USR_PORTS].BytesOut-UsrPort[NO_OF_USR_PORTS].BytesLastOut);
	UsrPort[NO_OF_USR_PORTS].BytesLastOut=UsrPort[NO_OF_USR_PORTS].BytesOut;
	UsrPort[NO_OF_USR_PORTS].BytesPerSecAvgOut=(DWORD)(UsrPort[NO_OF_USR_PORTS].BytesOut/UsrPort[NO_OF_USR_PORTS].Seconds);
	if(UsrPort[NO_OF_USR_PORTS].BytesPerSecOut>UsrPort[NO_OF_USR_PORTS].BytesPerSecMaxOut)
		UsrPort[NO_OF_USR_PORTS].BytesPerSecMaxOut=UsrPort[NO_OF_USR_PORTS].BytesPerSecOut;

	UscPort[NO_OF_USC_PORTS].BeatsIn=0;
	UscPort[NO_OF_USC_PORTS].InBuffer_Size=0;
	UscPort[NO_OF_USC_PORTS].BytesPerSecIn=0;
	UscPort[NO_OF_USC_PORTS].BytesPerSecAvgIn=0;
	UscPort[NO_OF_USC_PORTS].BlocksIn=0;
	UscPort[NO_OF_USC_PORTS].PacketsIn=0;

	UscPort[NO_OF_USC_PORTS].BeatsOut=0;
	UscPort[NO_OF_USC_PORTS].OutBuffer_Size=0;
	UscPort[NO_OF_USC_PORTS].BytesPerSecOut=0;
	UscPort[NO_OF_USC_PORTS].BytesPerSecAvgOut=0;
	UscPort[NO_OF_USC_PORTS].BlocksOut=0;
	UscPort[NO_OF_USC_PORTS].PacketsOut=0;

	for(i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(UscPort[i].InUse)
		{
			if(!pdoc->pmod->initialized)
				strcpy(UscPort[i].Status,"LOADING ");
			else if (UscPort[i].ConnectHold)
				strcpy(UscPort[i].Status,"HOLDING ");
			else if((!strcmp(UscPort[i].Status,"LOADING "))&&(pdoc->pmod->initialized))
				UscPort[i].Status[0]=0;
			UscPort[i].Seconds++;
			UscPort[i].BytesPerSecIn=(DWORD)(UscPort[i].BytesIn-UscPort[i].BytesLastIn);
			if(UscPort[i].BytesPerSecIn>UscPort[i].BytesPerSecMaxIn)
				UscPort[i].BytesPerSecMaxIn=UscPort[i].BytesPerSecIn;
			UscPort[i].BytesPerSecAvgIn=(DWORD)(UscPort[i].BytesIn/UscPort[i].Seconds);
			UscPort[i].BytesLastIn=UscPort[i].BytesIn;

			UscPort[i].BytesPerSecOut=(DWORD)(UscPort[i].BytesOut-UscPort[i].BytesLastOut);
			if(UscPort[i].BytesPerSecOut>UscPort[i].BytesPerSecMaxOut)
				UscPort[i].BytesPerSecMaxOut=UscPort[i].BytesPerSecOut;
			UscPort[i].BytesPerSecAvgOut=(DWORD)(UscPort[i].BytesOut/UscPort[i].Seconds);
			UscPort[i].BytesLastOut=UscPort[i].BytesOut;

			UscPort[NO_OF_USC_PORTS].BeatsIn+=UscPort[i].BeatsIn;
			UscPort[NO_OF_USC_PORTS].InBuffer_Size+=UscPort[i].InBuffer_Size;
			UscPort[NO_OF_USC_PORTS].BlocksIn+=UscPort[i].BlocksIn;
			UscPort[NO_OF_USC_PORTS].PacketsIn+=UscPort[i].PacketsIn;
			UscPort[NO_OF_USC_PORTS].BytesIn+=UscPort[i].BytesPerSecIn;

			UscPort[NO_OF_USC_PORTS].BeatsOut+=UscPort[i].BeatsOut;
			UscPort[NO_OF_USC_PORTS].OutBuffer_Size+=UscPort[i].OutBuffer_Size;
			UscPort[NO_OF_USC_PORTS].BlocksOut+=UscPort[i].BlocksOut;
			UscPort[NO_OF_USC_PORTS].PacketsOut+=UscPort[i].PacketsOut;
			UscPort[NO_OF_USC_PORTS].BytesOut+=UscPort[i].BytesPerSecOut;
		}
	}

	UscPort[NO_OF_USC_PORTS].Seconds++;
	UscPort[NO_OF_USC_PORTS].BytesPerSecIn=(DWORD)(UscPort[NO_OF_USC_PORTS].BytesIn-UscPort[NO_OF_USC_PORTS].BytesLastIn);
	UscPort[NO_OF_USC_PORTS].BytesLastIn=UscPort[NO_OF_USC_PORTS].BytesIn;
	UscPort[NO_OF_USC_PORTS].BytesPerSecAvgIn=(DWORD)(UscPort[NO_OF_USC_PORTS].BytesIn/UscPort[NO_OF_USC_PORTS].Seconds);
	if(UscPort[NO_OF_USC_PORTS].BytesPerSecIn>UscPort[NO_OF_USC_PORTS].BytesPerSecMaxIn)
		UscPort[NO_OF_USC_PORTS].BytesPerSecMaxIn=UscPort[NO_OF_USC_PORTS].BytesPerSecIn;

	UscPort[NO_OF_USC_PORTS].BytesPerSecOut=(DWORD)(UscPort[NO_OF_USC_PORTS].BytesOut-UscPort[NO_OF_USC_PORTS].BytesLastOut);
	UscPort[NO_OF_USC_PORTS].BytesLastOut=UscPort[NO_OF_USC_PORTS].BytesOut;
	UscPort[NO_OF_USC_PORTS].BytesPerSecAvgOut=(DWORD)(UscPort[NO_OF_USC_PORTS].BytesOut/UscPort[NO_OF_USC_PORTS].Seconds);
	if(UscPort[NO_OF_USC_PORTS].BytesPerSecOut>UscPort[NO_OF_USC_PORTS].BytesPerSecMaxOut)
		UscPort[NO_OF_USC_PORTS].BytesPerSecMaxOut=UscPort[NO_OF_USC_PORTS].BytesPerSecOut;

#ifdef WS_GUARANTEED
	for(i=0;i<NO_OF_UGC_PORTS;i++)
	{
		if(UgcPort[i].InUse)
		{
			UgcPort[i].BeatsIn=0;
			UgcPort[i].InBuffer_Size=0;
			UgcPort[i].BytesPerSecIn=0;
			UgcPort[i].BytesPerSecAvgIn=0;
			UgcPort[i].BlocksIn=0;
			UgcPort[i].PacketsIn=0;

			UgcPort[i].BeatsOut=0;
			UgcPort[i].OutBuffer_Size=0;
			UgcPort[i].BytesPerSecOut=0;
			UgcPort[i].BytesPerSecAvgOut=0;
			UgcPort[i].BlocksOut=0;
			UgcPort[i].PacketsOut=0;

			NoOfUgcs++;
		}
	}
	UgrPort[NO_OF_UGR_PORTS].BeatsIn=0;
	UgrPort[NO_OF_UGR_PORTS].InBuffer.Size=0;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecIn=0;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecAvgIn=0;
	UgrPort[NO_OF_UGR_PORTS].BlocksIn=0;
	UgrPort[NO_OF_UGR_PORTS].PacketsIn=0;

	UgrPort[NO_OF_UGR_PORTS].BeatsOut=0;
	UgrPort[NO_OF_UGR_PORTS].OutBuffer.Size=0;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecOut=0;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecAvgOut=0;
	UgrPort[NO_OF_UGR_PORTS].BlocksOut=0;
	UgrPort[NO_OF_UGR_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	UgrPort[NO_OF_UGR_PORTS].IOCPSendBytes=0;
	#elif defined(WS_COMPLETION_PORT_SENDS)
	UgrPort[NO_OF_UGR_PORTS].IOCPSendBytes=0;
	#endif

	for(i=0;i<NO_OF_UGR_PORTS;i++)
	{
		if(UgrPort[i].SockActive)
		{
			NoOfUsers++;
			if(UgrPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -UgrPort[i].SendTimeOut>=1000)
					_snprintf(UgrPort[i].Status,sizeof(UgrPort[i].Status),"Retry:%d",(GetTickCount() -UgrPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_UGR,i);
			}
			else
				sprintf(UgrPort[i].Status, "%s (o=%d,i=%d)", UgrPort[i].GDId.SessionId, UgrPort[i].NextGDOutLogId, UgrPort[i].NextGDInLogId);
			UgrPort[i].Seconds++;
			UgrPort[i].BytesPerSecIn=(DWORD)(UgrPort[i].BytesIn-UgrPort[i].BytesLastIn);
			#ifdef WS_OTPP
			UgrPort[i].BytesPerSecIn+=UgrPort[i].TBufferSize;
			#endif
			if(UgrPort[i].BytesPerSecIn>UgrPort[i].BytesPerSecMaxIn)
				UgrPort[i].BytesPerSecMaxIn=UgrPort[i].BytesPerSecIn;
			UgrPort[i].BytesPerSecAvgIn=(DWORD)(UgrPort[i].BytesIn/UgrPort[i].Seconds);
			UgrPort[i].BytesLastIn=UgrPort[i].BytesIn;
			UgrPort[i].BytesPerSecOut=(DWORD)(UgrPort[i].BytesOut-UgrPort[i].BytesLastOut);
			if(UgrPort[i].BytesPerSecOut>UgrPort[i].BytesPerSecMaxOut)
				UgrPort[i].BytesPerSecMaxOut=UgrPort[i].BytesPerSecOut;
			UgrPort[i].BytesPerSecAvgOut=(DWORD)(UgrPort[i].BytesOut/UgrPort[i].Seconds);
			UgrPort[i].BytesLastOut=UgrPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,UgrPort[i].RemoteIP,UgcPort[UgrPort[i].UgcPort].Port
				,UgrPort[i].Note
				,"");
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,UgrPort[i].BeatsIn
				,SS(UgrPort[i].InBuffer.Size)
				,SS(UgrPort[i].BytesPerSecIn)
				,SS(UgrPort[i].BytesPerSecAvgIn)
				,SS(UgrPort[i].BytesPerSecMaxIn)
				,UgrPort[i].BlocksIn
				,UgrPort[i].PacketsIn
				,SS(UgrPort[i].BytesIn)
				);
		printf("\n");
#endif
			UgrPort[NO_OF_UGR_PORTS].BeatsIn+=UgrPort[i].BeatsIn;
			UgrPort[NO_OF_UGR_PORTS].InBuffer.Size+=UgrPort[i].InBuffer.Size;
			UgrPort[NO_OF_UGR_PORTS].BlocksIn+=UgrPort[i].BlocksIn;
			UgrPort[NO_OF_UGR_PORTS].PacketsIn+=UgrPort[i].PacketsIn;
			UgrPort[NO_OF_UGR_PORTS].BytesIn+=UgrPort[i].BytesPerSecIn;

			UgrPort[NO_OF_UGR_PORTS].BeatsOut+=UgrPort[i].BeatsOut;
			UgrPort[NO_OF_UGR_PORTS].OutBuffer.Size+=UgrPort[i].OutBuffer.Size;
			UgrPort[NO_OF_UGR_PORTS].BlocksOut+=UgrPort[i].BlocksOut;
			UgrPort[NO_OF_UGR_PORTS].PacketsOut+=UgrPort[i].PacketsOut;
			UgrPort[NO_OF_UGR_PORTS].BytesOut+=UgrPort[i].BytesPerSecOut;
			#ifdef WS_OTPP
			UgrPort[NO_OF_UGR_PORTS].IOCPSendBytes+=UgrPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			UgrPort[NO_OF_UGR_PORTS].IOCPSendBytes+=UgrPort[i].IOCPSendBytes;
			#endif

			UgcPort[UgrPort[i].UgcPort].BeatsIn+=UgrPort[i].BeatsIn;
			UgcPort[UgrPort[i].UgcPort].InBuffer_Size+=UgrPort[i].InBuffer.Size;
			UgcPort[UgrPort[i].UgcPort].BlocksIn+=UgrPort[i].BlocksIn;
			UgcPort[UgrPort[i].UgcPort].PacketsIn+=UgrPort[i].PacketsIn;
			UgcPort[UgrPort[i].UgcPort].BytesIn+=UgrPort[i].BytesPerSecIn;

			UgcPort[UgrPort[i].UgcPort].BeatsOut+=UgrPort[i].BeatsOut;
			#ifdef WS_OTPP
			UgcPort[UgrPort[i].UgcPort].OutBuffer_Size+=UgrPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			UgcPort[UgrPort[i].UgcPort].OutBuffer_Size+=UgrPort[i].IOCPSendBytes;
			#endif
			UgcPort[UgrPort[i].UgcPort].OutBuffer_Size+=UgrPort[i].OutBuffer.Size;
			UgcPort[UgrPort[i].UgcPort].BlocksOut+=UgrPort[i].BlocksOut;
			UgcPort[UgrPort[i].UgcPort].PacketsOut+=UgrPort[i].PacketsOut;
			UgcPort[UgrPort[i].UgcPort].BytesOut+=UgrPort[i].BytesPerSecOut;
		}
		else if(UgrPort[i].TimeTillClose>0)
			_snprintf(UgrPort[i].Status,sizeof(UgrPort[i].Status),"TimeTillClose:%d",UgrPort[i].TimeTillClose);
	}
	UgrPort[NO_OF_UGR_PORTS].Seconds++;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecIn=(DWORD)(UgrPort[NO_OF_UGR_PORTS].BytesIn-UgrPort[NO_OF_UGR_PORTS].BytesLastIn);
	UgrPort[NO_OF_UGR_PORTS].BytesLastIn=UgrPort[NO_OF_UGR_PORTS].BytesIn;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecAvgIn=(DWORD)(UgrPort[NO_OF_UGR_PORTS].BytesIn/UgrPort[NO_OF_UGR_PORTS].Seconds);
	if(UgrPort[NO_OF_UGR_PORTS].BytesPerSecIn>UgrPort[NO_OF_UGR_PORTS].BytesPerSecMaxIn)
		UgrPort[NO_OF_UGR_PORTS].BytesPerSecMaxIn=UgrPort[NO_OF_UGR_PORTS].BytesPerSecIn;

	UgrPort[NO_OF_UGR_PORTS].BytesPerSecOut=(DWORD)(UgrPort[NO_OF_UGR_PORTS].BytesOut-UgrPort[NO_OF_UGR_PORTS].BytesLastOut);
	UgrPort[NO_OF_UGR_PORTS].BytesLastOut=UgrPort[NO_OF_UGR_PORTS].BytesOut;
	UgrPort[NO_OF_UGR_PORTS].BytesPerSecAvgOut=(DWORD)(UgrPort[NO_OF_UGR_PORTS].BytesOut/UgrPort[NO_OF_UGR_PORTS].Seconds);
	if(UgrPort[NO_OF_UGR_PORTS].BytesPerSecOut>UgrPort[NO_OF_UGR_PORTS].BytesPerSecMaxOut)
		UgrPort[NO_OF_UGR_PORTS].BytesPerSecMaxOut=UgrPort[NO_OF_UGR_PORTS].BytesPerSecOut;

	UgcPort[NO_OF_UGC_PORTS].BeatsIn=0;
	UgcPort[NO_OF_UGC_PORTS].InBuffer_Size=0;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecIn=0;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecAvgIn=0;
	UgcPort[NO_OF_UGC_PORTS].BlocksIn=0;
	UgcPort[NO_OF_UGC_PORTS].PacketsIn=0;

	UgcPort[NO_OF_UGC_PORTS].BeatsOut=0;
	UgcPort[NO_OF_UGC_PORTS].OutBuffer_Size=0;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecOut=0;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecAvgOut=0;
	UgcPort[NO_OF_UGC_PORTS].BlocksOut=0;
	UgcPort[NO_OF_UGC_PORTS].PacketsOut=0;

	for(i=0;i<NO_OF_UGC_PORTS;i++)
	{
		if(UgcPort[i].InUse)
		{
			if(!pdoc->pmod->initialized)
				strcpy(UgcPort[i].Status,"LOADING ");
			else if (UgcPort[i].ConnectHold)
				strcpy(UgcPort[i].Status,"HOLDING ");
			else if((!strcmp(UgcPort[i].Status,"LOADING "))&&(pdoc->pmod->initialized))
				UgcPort[i].Status[0]=0;
			UgcPort[i].Seconds++;
			UgcPort[i].BytesPerSecIn=(DWORD)(UgcPort[i].BytesIn-UgcPort[i].BytesLastIn);
			if(UgcPort[i].BytesPerSecIn>UgcPort[i].BytesPerSecMaxIn)
				UgcPort[i].BytesPerSecMaxIn=UgcPort[i].BytesPerSecIn;
			UgcPort[i].BytesPerSecAvgIn=(DWORD)(UgcPort[i].BytesIn/UgcPort[i].Seconds);
			UgcPort[i].BytesLastIn=UgcPort[i].BytesIn;

			UgcPort[i].BytesPerSecOut=(DWORD)(UgcPort[i].BytesOut-UgcPort[i].BytesLastOut);
			if(UgcPort[i].BytesPerSecOut>UgcPort[i].BytesPerSecMaxOut)
				UgcPort[i].BytesPerSecMaxOut=UgcPort[i].BytesPerSecOut;
			UgcPort[i].BytesPerSecAvgOut=(DWORD)(UgcPort[i].BytesOut/UgcPort[i].Seconds);
			UgcPort[i].BytesLastOut=UgcPort[i].BytesOut;

			UgcPort[NO_OF_UGC_PORTS].BeatsIn+=UgcPort[i].BeatsIn;
			UgcPort[NO_OF_UGC_PORTS].InBuffer_Size+=UgcPort[i].InBuffer_Size;
			UgcPort[NO_OF_UGC_PORTS].BlocksIn+=UgcPort[i].BlocksIn;
			UgcPort[NO_OF_UGC_PORTS].PacketsIn+=UgcPort[i].PacketsIn;
			UgcPort[NO_OF_UGC_PORTS].BytesIn+=UgcPort[i].BytesPerSecIn;

			UgcPort[NO_OF_UGC_PORTS].BeatsOut+=UgcPort[i].BeatsOut;
			UgcPort[NO_OF_UGC_PORTS].OutBuffer_Size+=UgcPort[i].OutBuffer_Size;
			UgcPort[NO_OF_UGC_PORTS].BlocksOut+=UgcPort[i].BlocksOut;
			UgcPort[NO_OF_UGC_PORTS].PacketsOut+=UgcPort[i].PacketsOut;
			UgcPort[NO_OF_UGC_PORTS].BytesOut+=UgcPort[i].BytesPerSecOut;
		}
	}

	UgcPort[NO_OF_UGC_PORTS].Seconds++;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecIn=(DWORD)(UgcPort[NO_OF_UGC_PORTS].BytesIn-UgcPort[NO_OF_UGC_PORTS].BytesLastIn);
	UgcPort[NO_OF_UGC_PORTS].BytesLastIn=UgcPort[NO_OF_UGC_PORTS].BytesIn;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecAvgIn=(DWORD)(UgcPort[NO_OF_UGC_PORTS].BytesIn/UgcPort[NO_OF_UGC_PORTS].Seconds);
	if(UgcPort[NO_OF_UGC_PORTS].BytesPerSecIn>UgcPort[NO_OF_UGC_PORTS].BytesPerSecMaxIn)
		UgcPort[NO_OF_UGC_PORTS].BytesPerSecMaxIn=UgcPort[NO_OF_UGC_PORTS].BytesPerSecIn;

	UgcPort[NO_OF_UGC_PORTS].BytesPerSecOut=(DWORD)(UgcPort[NO_OF_UGC_PORTS].BytesOut-UgcPort[NO_OF_UGC_PORTS].BytesLastOut);
	UgcPort[NO_OF_UGC_PORTS].BytesLastOut=UgcPort[NO_OF_UGC_PORTS].BytesOut;
	UgcPort[NO_OF_UGC_PORTS].BytesPerSecAvgOut=(DWORD)(UgcPort[NO_OF_UGC_PORTS].BytesOut/UgcPort[NO_OF_UGC_PORTS].Seconds);
	if(UgcPort[NO_OF_UGC_PORTS].BytesPerSecOut>UgcPort[NO_OF_UGC_PORTS].BytesPerSecMaxOut)
		UgcPort[NO_OF_UGC_PORTS].BytesPerSecMaxOut=UgcPort[NO_OF_UGC_PORTS].BytesPerSecOut;
#endif //#ifdef WS_GUARANTEED

#ifdef WS_MONITOR
	for(i=0;i<NO_OF_UMC_PORTS;i++)
	{
		if(UmcPort[i].InUse)
		{
			UmcPort[i].BeatsIn=0;
			UmcPort[i].InBuffer_Size=0;
			UmcPort[i].BytesPerSecIn=0;
			UmcPort[i].BytesPerSecAvgIn=0;
			UmcPort[i].BlocksIn=0;
			UmcPort[i].PacketsIn=0;

			UmcPort[i].BeatsOut=0;
			UmcPort[i].OutBuffer_Size=0;
			UmcPort[i].BytesPerSecOut=0;
			UmcPort[i].BytesPerSecAvgOut=0;
			UmcPort[i].BlocksOut=0;
			UmcPort[i].PacketsOut=0;

			NoOfUmcs++;
		}
	}
	UmrPort[NO_OF_UMR_PORTS].BeatsIn=0;
	UmrPort[NO_OF_UMR_PORTS].InBuffer.Size=0;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecIn=0;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecAvgIn=0;
	UmrPort[NO_OF_UMR_PORTS].BlocksIn=0;
	UmrPort[NO_OF_UMR_PORTS].PacketsIn=0;

	UmrPort[NO_OF_UMR_PORTS].BeatsOut=0;
	UmrPort[NO_OF_UMR_PORTS].OutBuffer.Size=0;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecOut=0;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecAvgOut=0;
	UmrPort[NO_OF_UMR_PORTS].BlocksOut=0;
	UmrPort[NO_OF_UMR_PORTS].PacketsOut=0;
	#ifdef WS_OTPP
	UmrPort[NO_OF_UMR_PORTS].IOCPSendBytes=0;
	#elif defined(WS_COMPLETION_PORT_SENDS)
	UmrPort[NO_OF_UMR_PORTS].IOCPSendBytes=0;
	#endif

	for(i=0;i<NO_OF_UMR_PORTS;i++)
	{
		if(UmrPort[i].SockActive)
		{
			NoOfUsers++;
			if(UmrPort[i].SendTimeOut>0)
			{
				if(GetTickCount() -UmrPort[i].SendTimeOut>=1000)
					_snprintf(UmrPort[i].Status,sizeof(UmrPort[i].Status),"Retry:%d",(GetTickCount() -UmrPort[i].SendTimeOut)/1000);
				//theApp.WSHLogSendTimeout(WS_UMR,i);
			}
			//else if(UmrPort[i].DetPtr4)
			//	strcpy(UmrPort[i].Status,(char*)UmrPort[i].DetPtr4);
			else if(!UmrPort[i].Status[0])
				strcpy(UmrPort[i].Status," ");
			UmrPort[i].Seconds++;
			UmrPort[i].BytesPerSecIn=(DWORD)(UmrPort[i].BytesIn-UmrPort[i].BytesLastIn);
			#ifdef WS_OTPP
			UmrPort[i].BytesPerSecIn+=UmrPort[i].TBufferSize;
			#endif
			if(UmrPort[i].BytesPerSecIn>UmrPort[i].BytesPerSecMaxIn)
				UmrPort[i].BytesPerSecMaxIn=UmrPort[i].BytesPerSecIn;
			UmrPort[i].BytesPerSecAvgIn=(DWORD)(UmrPort[i].BytesIn/UmrPort[i].Seconds);
			UmrPort[i].BytesLastIn=UmrPort[i].BytesIn;
			UmrPort[i].BytesPerSecOut=(DWORD)(UmrPort[i].BytesOut-UmrPort[i].BytesLastOut);
			if(UmrPort[i].BytesPerSecOut>UmrPort[i].BytesPerSecMaxOut)
				UmrPort[i].BytesPerSecMaxOut=UmrPort[i].BytesPerSecOut;
			UmrPort[i].BytesPerSecAvgOut=(DWORD)(UmrPort[i].BytesOut/UmrPort[i].Seconds);
			UmrPort[i].BytesLastOut=UmrPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,UmrPort[i].RemoteIP,UmcPort[UmrPort[i].UmcPort].Port
				,UmrPort[i].Note
				,"");
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,UmrPort[i].BeatsIn
				,SS(UmrPort[i].InBuffer.Size)
				,SS(UmrPort[i].BytesPerSecIn)
				,SS(UmrPort[i].BytesPerSecAvgIn)
				,SS(UmrPort[i].BytesPerSecMaxIn)
				,UmrPort[i].BlocksIn
				,UmrPort[i].PacketsIn
				,SS(UmrPort[i].BytesIn)
				);
		printf("\n");
#endif
			UmrPort[NO_OF_UMR_PORTS].BeatsIn+=UmrPort[i].BeatsIn;
			UmrPort[NO_OF_UMR_PORTS].InBuffer.Size+=UmrPort[i].InBuffer.Size;
			UmrPort[NO_OF_UMR_PORTS].BlocksIn+=UmrPort[i].BlocksIn;
			UmrPort[NO_OF_UMR_PORTS].PacketsIn+=UmrPort[i].PacketsIn;
			UmrPort[NO_OF_UMR_PORTS].BytesIn+=UmrPort[i].BytesPerSecIn;

			UmrPort[NO_OF_UMR_PORTS].BeatsOut+=UmrPort[i].BeatsOut;
			UmrPort[NO_OF_UMR_PORTS].OutBuffer.Size+=UmrPort[i].OutBuffer.Size;
			UmrPort[NO_OF_UMR_PORTS].BlocksOut+=UmrPort[i].BlocksOut;
			UmrPort[NO_OF_UMR_PORTS].PacketsOut+=UmrPort[i].PacketsOut;
			UmrPort[NO_OF_UMR_PORTS].BytesOut+=UmrPort[i].BytesPerSecOut;
			#ifdef WS_OTPP
			UmrPort[NO_OF_UMR_PORTS].IOCPSendBytes+=UmrPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			UmrPort[NO_OF_UMR_PORTS].IOCPSendBytes+=UmrPort[i].IOCPSendBytes;
			#endif

			UmcPort[UmrPort[i].UmcPort].BeatsIn+=UmrPort[i].BeatsIn;
			UmcPort[UmrPort[i].UmcPort].InBuffer_Size+=UmrPort[i].InBuffer.Size;
			UmcPort[UmrPort[i].UmcPort].BlocksIn+=UmrPort[i].BlocksIn;
			UmcPort[UmrPort[i].UmcPort].PacketsIn+=UmrPort[i].PacketsIn;
			UmcPort[UmrPort[i].UmcPort].BytesIn+=UmrPort[i].BytesPerSecIn;

			UmcPort[UmrPort[i].UmcPort].BeatsOut+=UmrPort[i].BeatsOut;
			#ifdef WS_OTPP
			UmcPort[UmrPort[i].UmcPort].OutBuffer_Size+=UmrPort[i].IOCPSendBytes;
			#elif defined(WS_COMPLETION_PORT_SENDS)
			UmcPort[UmrPort[i].UmcPort].OutBuffer_Size+=UmrPort[i].IOCPSendBytes;
			#endif
			UmcPort[UmrPort[i].UmcPort].OutBuffer_Size+=UmrPort[i].OutBuffer.Size;
			UmcPort[UmrPort[i].UmcPort].BlocksOut+=UmrPort[i].BlocksOut;
			UmcPort[UmrPort[i].UmcPort].PacketsOut+=UmrPort[i].PacketsOut;
			UmcPort[UmrPort[i].UmcPort].BytesOut+=UmrPort[i].BytesPerSecOut;
		}
		else if(UmrPort[i].TimeTillClose>0)
			_snprintf(UmrPort[i].Status,sizeof(UmrPort[i].Status),"TimeTillClose:%d",UmrPort[i].TimeTillClose);
	}
	UmrPort[NO_OF_UMR_PORTS].Seconds++;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecIn=(DWORD)(UmrPort[NO_OF_UMR_PORTS].BytesIn-UmrPort[NO_OF_UMR_PORTS].BytesLastIn);
	UmrPort[NO_OF_UMR_PORTS].BytesLastIn=UmrPort[NO_OF_UMR_PORTS].BytesIn;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecAvgIn=(DWORD)(UmrPort[NO_OF_UMR_PORTS].BytesIn/UmrPort[NO_OF_UMR_PORTS].Seconds);
	if(UmrPort[NO_OF_UMR_PORTS].BytesPerSecIn>UmrPort[NO_OF_UMR_PORTS].BytesPerSecMaxIn)
		UmrPort[NO_OF_UMR_PORTS].BytesPerSecMaxIn=UmrPort[NO_OF_UMR_PORTS].BytesPerSecIn;

	UmrPort[NO_OF_UMR_PORTS].BytesPerSecOut=(DWORD)(UmrPort[NO_OF_UMR_PORTS].BytesOut-UmrPort[NO_OF_UMR_PORTS].BytesLastOut);
	UmrPort[NO_OF_UMR_PORTS].BytesLastOut=UmrPort[NO_OF_UMR_PORTS].BytesOut;
	UmrPort[NO_OF_UMR_PORTS].BytesPerSecAvgOut=(DWORD)(UmrPort[NO_OF_UMR_PORTS].BytesOut/UmrPort[NO_OF_UMR_PORTS].Seconds);
	if(UmrPort[NO_OF_UMR_PORTS].BytesPerSecOut>UmrPort[NO_OF_UMR_PORTS].BytesPerSecMaxOut)
		UmrPort[NO_OF_UMR_PORTS].BytesPerSecMaxOut=UmrPort[NO_OF_UMR_PORTS].BytesPerSecOut;

	UmcPort[NO_OF_UMC_PORTS].BeatsIn=0;
	UmcPort[NO_OF_UMC_PORTS].InBuffer_Size=0;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecIn=0;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecAvgIn=0;
	UmcPort[NO_OF_UMC_PORTS].BlocksIn=0;
	UmcPort[NO_OF_UMC_PORTS].PacketsIn=0;

	UmcPort[NO_OF_UMC_PORTS].BeatsOut=0;
	UmcPort[NO_OF_UMC_PORTS].OutBuffer_Size=0;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecOut=0;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecAvgOut=0;
	UmcPort[NO_OF_UMC_PORTS].BlocksOut=0;
	UmcPort[NO_OF_UMC_PORTS].PacketsOut=0;

	for(i=0;i<NO_OF_UMC_PORTS;i++)
	{
		if(UmcPort[i].InUse)
		{
			if(!pdoc->pmod->initialized)
				strcpy(UmcPort[i].Status,"LOADING ");
			else if (UmcPort[i].ConnectHold)
				strcpy(UmcPort[i].Status,"HOLDING ");
			else if((!strcmp(UmcPort[i].Status,"LOADING "))&&(pdoc->pmod->initialized))
				UmcPort[i].Status[0]=0;
			UmcPort[i].Seconds++;
			UmcPort[i].BytesPerSecIn=(DWORD)(UmcPort[i].BytesIn-UmcPort[i].BytesLastIn);
			if(UmcPort[i].BytesPerSecIn>UmcPort[i].BytesPerSecMaxIn)
				UmcPort[i].BytesPerSecMaxIn=UmcPort[i].BytesPerSecIn;
			UmcPort[i].BytesPerSecAvgIn=(DWORD)(UmcPort[i].BytesIn/UmcPort[i].Seconds);
			UmcPort[i].BytesLastIn=UmcPort[i].BytesIn;

			UmcPort[i].BytesPerSecOut=(DWORD)(UmcPort[i].BytesOut-UmcPort[i].BytesLastOut);
			if(UmcPort[i].BytesPerSecOut>UmcPort[i].BytesPerSecMaxOut)
				UmcPort[i].BytesPerSecMaxOut=UmcPort[i].BytesPerSecOut;
			UmcPort[i].BytesPerSecAvgOut=(DWORD)(UmcPort[i].BytesOut/UmcPort[i].Seconds);
			UmcPort[i].BytesLastOut=UmcPort[i].BytesOut;

			UmcPort[NO_OF_UMC_PORTS].BeatsIn+=UmcPort[i].BeatsIn;
			UmcPort[NO_OF_UMC_PORTS].InBuffer_Size+=UmcPort[i].InBuffer_Size;
			UmcPort[NO_OF_UMC_PORTS].BlocksIn+=UmcPort[i].BlocksIn;
			UmcPort[NO_OF_UMC_PORTS].PacketsIn+=UmcPort[i].PacketsIn;
			UmcPort[NO_OF_UMC_PORTS].BytesIn+=UmcPort[i].BytesPerSecIn;

			UmcPort[NO_OF_UMC_PORTS].BeatsOut+=UmcPort[i].BeatsOut;
			UmcPort[NO_OF_UMC_PORTS].OutBuffer_Size+=UmcPort[i].OutBuffer_Size;
			UmcPort[NO_OF_UMC_PORTS].BlocksOut+=UmcPort[i].BlocksOut;
			UmcPort[NO_OF_UMC_PORTS].PacketsOut+=UmcPort[i].PacketsOut;
			UmcPort[NO_OF_UMC_PORTS].BytesOut+=UmcPort[i].BytesPerSecOut;
		}
	}

	UmcPort[NO_OF_UMC_PORTS].Seconds++;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecIn=(DWORD)(UmcPort[NO_OF_UMC_PORTS].BytesIn-UmcPort[NO_OF_UMC_PORTS].BytesLastIn);
	UmcPort[NO_OF_UMC_PORTS].BytesLastIn=UmcPort[NO_OF_UMC_PORTS].BytesIn;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecAvgIn=(DWORD)(UmcPort[NO_OF_UMC_PORTS].BytesIn/UmcPort[NO_OF_UMC_PORTS].Seconds);
	if(UmcPort[NO_OF_UMC_PORTS].BytesPerSecIn>UmcPort[NO_OF_UMC_PORTS].BytesPerSecMaxIn)
		UmcPort[NO_OF_UMC_PORTS].BytesPerSecMaxIn=UmcPort[NO_OF_UMC_PORTS].BytesPerSecIn;

	UmcPort[NO_OF_UMC_PORTS].BytesPerSecOut=(DWORD)(UmcPort[NO_OF_UMC_PORTS].BytesOut-UmcPort[NO_OF_UMC_PORTS].BytesLastOut);
	UmcPort[NO_OF_UMC_PORTS].BytesLastOut=UmcPort[NO_OF_UMC_PORTS].BytesOut;
	UmcPort[NO_OF_UMC_PORTS].BytesPerSecAvgOut=(DWORD)(UmcPort[NO_OF_UMC_PORTS].BytesOut/UmcPort[NO_OF_UMC_PORTS].Seconds);
	if(UmcPort[NO_OF_UMC_PORTS].BytesPerSecOut>UmcPort[NO_OF_UMC_PORTS].BytesPerSecMaxOut)
		UmcPort[NO_OF_UMC_PORTS].BytesPerSecMaxOut=UmcPort[NO_OF_UMC_PORTS].BytesPerSecOut;
#endif //#ifdef WS_MONITOR

#ifdef WS_BROADCAST_RECEIVER
	BrrPort[NO_OF_BRR_PORTS].BeatsIn=0;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecIn=0;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecAvgIn=0;
	BrrPort[NO_OF_BRR_PORTS].BlocksIn=0;
	BrrPort[NO_OF_BRR_PORTS].PacketsIn=0;

	BrrPort[NO_OF_BRR_PORTS].BeatsOut=0;
	BrrPort[NO_OF_BRR_PORTS].Depth=0;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecOut=0;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecAvgOut=0;
	BrrPort[NO_OF_BRR_PORTS].BlocksOut=0;
	BrrPort[NO_OF_BRR_PORTS].PacketsOut=0;
	BrrPort[NO_OF_BRR_PORTS].Mps=0;

	for(i=0;i<NO_OF_BRR_PORTS;i++)
	{
		if(BrrPort[i].InUse)
		{
			//NoOfBrrs++;
			if(!pdoc->pmod->initialized)
				strcpy(BrrPort[i].Status,"LOADING ");
			else if(BrrPort[i].ConnectHold)
				strcpy(BrrPort[i].Status,"HOLDING ");
			else if ((BrrPort[i].receiver)&&(!BrrPort[i].receiver->IsEnabled()))
				strcpy(BrrPort[i].Status,"WAIT JOIN");
			else
				sprintf(BrrPort[i].Status, "JOINED %s mps",WSConListSize(BrrPort[i].Mps));
			BrrPort[i].Seconds++;
			if(BrrPort[i].receiver)
			{
				Statistics &statistics = BrrPort[i].receiver->GetStatistics();
				BrrPort[i].BytesIn = (unsigned long)statistics.GetStatisticsReceiveByte();
				BrrPort[i].BlocksIn = (unsigned long)statistics.GetStatisticsReceiveCount();
				BrrPort[i].BytesOut = (unsigned long)statistics.GetStatisticsSendByte();
				BrrPort[i].BlocksOut = (unsigned long)statistics.GetStatisticsSendCount();
				BrrPort[i].Depth = BrrPort[i].receiver->GetDepth();
			}
			else
			{
				BrrPort[i].BytesIn = 0;
				BrrPort[i].BlocksIn = 0;
				BrrPort[i].BytesOut = 0;
				BrrPort[i].BlocksOut = 0;
				BrrPort[i].Depth = 0;
			}

			BrrPort[i].BytesPerSecIn = BrrPort[i].BytesIn - BrrPort[i].BytesLastIn;
			if(BrrPort[i].BytesPerSecIn>BrrPort[i].BytesPerSecMaxIn)
				BrrPort[i].BytesPerSecMaxIn=BrrPort[i].BytesPerSecIn;
            BrrPort[i].BytesPerSecAvgIn=BrrPort[i].BytesIn/BrrPort[i].Seconds;
			BrrPort[i].BytesLastIn=BrrPort[i].BytesIn;
			BrrPort[i].BytesPerSecOut=BrrPort[i].BytesOut-BrrPort[i].BytesLastOut;
            if(BrrPort[i].BytesPerSecOut>BrrPort[i].BytesPerSecMaxOut)
				BrrPort[i].BytesPerSecMaxOut=BrrPort[i].BytesPerSecOut;
			BrrPort[i].BytesPerSecAvgOut=BrrPort[i].BytesOut/BrrPort[i].Seconds;
			BrrPort[i].BytesLastOut=BrrPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,BrrPort[i].RemoteIP,BrrPort[i].Port
				,BrrPort[i].Note
				,BrrPort[i].OnLineStatusText);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,BrrPort[i].BeatsIn
				,SS(BrrPort[i].InBuffer.Size)
				,SS(BrrPort[i].BytesPerSecIn)
				,SS(BrrPort[i].BytesPerSecAvgIn)
				,SS(BrrPort[i].BytesPerSecMaxIn)
				,BrrPort[i].BlocksIn
				,BrrPort[i].PacketsIn
				,SS(BrrPort[i].BytesIn)
				);
		printf("\n");
#endif
			BrrPort[NO_OF_BRR_PORTS].BeatsIn+=BrrPort[i].BeatsIn;
			BrrPort[NO_OF_BRR_PORTS].BlocksIn+=BrrPort[i].BlocksIn;
			BrrPort[NO_OF_BRR_PORTS].PacketsIn+=BrrPort[i].PacketsIn;
			BrrPort[NO_OF_BRR_PORTS].BytesIn+=BrrPort[i].BytesPerSecIn;

			BrrPort[NO_OF_BRR_PORTS].BeatsOut+=BrrPort[i].BeatsOut;
			BrrPort[NO_OF_BRR_PORTS].Depth += BrrPort[i].Depth;
			BrrPort[NO_OF_BRR_PORTS].BlocksOut+=BrrPort[i].BlocksOut;
			BrrPort[NO_OF_BRR_PORTS].PacketsOut+=BrrPort[i].PacketsOut;
			BrrPort[NO_OF_BRR_PORTS].BytesOut+=BrrPort[i].BytesPerSecOut;
			BrrPort[NO_OF_BRR_PORTS].Mps+=BrrPort[i].Mps; BrrPort[i].Mps=0;
		}
	}
	sprintf(BrrPort[NO_OF_BRR_PORTS].Status,"TOT %s mps",WSConListSize(BrrPort[NO_OF_BRR_PORTS].Mps));
	BrrPort[NO_OF_BRT_PORTS].Mps=0;
	BrrPort[NO_OF_BRR_PORTS].Seconds++;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecIn=BrrPort[NO_OF_BRR_PORTS].BytesIn-BrrPort[NO_OF_BRR_PORTS].BytesLastIn;
	BrrPort[NO_OF_BRR_PORTS].BytesLastIn=BrrPort[NO_OF_BRR_PORTS].BytesIn;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecAvgIn=BrrPort[NO_OF_BRR_PORTS].BytesIn/BrrPort[NO_OF_BRR_PORTS].Seconds;
	if(BrrPort[NO_OF_BRR_PORTS].BytesPerSecIn>BrrPort[NO_OF_BRR_PORTS].BytesPerSecMaxIn)
		BrrPort[NO_OF_BRR_PORTS].BytesPerSecMaxIn=BrrPort[NO_OF_BRR_PORTS].BytesPerSecIn;

	BrrPort[NO_OF_BRR_PORTS].BytesPerSecOut=BrrPort[NO_OF_BRR_PORTS].BytesOut-BrrPort[NO_OF_BRR_PORTS].BytesLastOut;
	BrrPort[NO_OF_BRR_PORTS].BytesLastOut=BrrPort[NO_OF_BRR_PORTS].BytesOut;
	BrrPort[NO_OF_BRR_PORTS].BytesPerSecAvgOut=BrrPort[NO_OF_BRR_PORTS].BytesOut/BrrPort[NO_OF_BRR_PORTS].Seconds;
	if(BrrPort[NO_OF_BRR_PORTS].BytesPerSecOut>BrrPort[NO_OF_BRR_PORTS].BytesPerSecMaxOut)
		BrrPort[NO_OF_BRR_PORTS].BytesPerSecMaxOut=BrrPort[NO_OF_BRR_PORTS].BytesPerSecOut;

#endif // BROADCAST_RECEIVER
#ifdef WS_BROADCAST_TRANSMITTER
    BrtPort[NO_OF_BRT_PORTS].BeatsIn=0;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecIn=0;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecAvgIn=0;
	BrtPort[NO_OF_BRT_PORTS].BlocksIn=0;
	BrtPort[NO_OF_BRT_PORTS].PacketsIn=0;

	BrtPort[NO_OF_BRT_PORTS].BeatsOut=0;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecOut=0;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecAvgOut=0;
	BrtPort[NO_OF_BRT_PORTS].BlocksOut=0;
	BrtPort[NO_OF_BRT_PORTS].PacketsOut=0;
	BrtPort[NO_OF_BRT_PORTS].Mps=0;

	for(i=0;i<NO_OF_BRT_PORTS;i++)
	{
		if(BrtPort[i].transmitter)
		{
			//NoOfBrts++;
			if(!pdoc->pmod->initialized)
				strcpy(BrtPort[i].Status,"LOADING ");
			else if(BrtPort[i].ConnectHold)
				strcpy(BrtPort[i].Status,"HOLDING ");
			else if (!BrtPort[i].transmitter->IsEnabled())
				strcpy(BrtPort[i].Status,"WAIT HOST");
			else
				sprintf(BrtPort[i].Status, "HOSTED %s mps",WSConListSize(BrtPort[i].Mps));
			BrtPort[i].Seconds++;
            Statistics &statistics = BrtPort[i].transmitter->GetStatistics();
            BrtPort[i].BytesIn = (DWORD)statistics.GetStatisticsReceiveByte();
            BrtPort[i].BlocksIn = (DWORD)statistics.GetStatisticsReceiveCount();
            BrtPort[i].BytesOut = (DWORD)statistics.GetStatisticsSendByte();
            BrtPort[i].BlocksOut = (DWORD)statistics.GetStatisticsSendCount();

			BrtPort[i].BytesPerSecIn = BrtPort[i].BytesIn - BrtPort[i].BytesLastIn;
			if(BrtPort[i].BytesPerSecIn>BrtPort[i].BytesPerSecMaxIn)
				BrtPort[i].BytesPerSecMaxIn=BrtPort[i].BytesPerSecIn;
            BrtPort[i].BytesPerSecAvgIn=BrtPort[i].BytesIn/BrtPort[i].Seconds;
			BrtPort[i].BytesLastIn=BrtPort[i].BytesIn;
			BrtPort[i].BytesPerSecOut=BrtPort[i].BytesOut-BrtPort[i].BytesLastOut;
            if(BrtPort[i].BytesPerSecOut>BrtPort[i].BytesPerSecMaxOut)
				BrtPort[i].BytesPerSecMaxOut=BrtPort[i].BytesPerSecOut;
			BrtPort[i].BytesPerSecAvgOut=BrtPort[i].BytesOut/BrtPort[i].Seconds;
			BrtPort[i].BytesLastOut=BrtPort[i].BytesOut;
#ifdef UNIX
			printf("C%03d|%s:%d|%s|%s|",i,BrtPort[i].RemoteIP,BrtPort[i].Port
				,BrtPort[i].Note
				,BrtPort[i].OnLineStatusText);
			printf("HB:%ld,H%s,R%s,RA%s,RM%s,#%ld,P%ld,B%s|"
				,BrtPort[i].BeatsIn
				,SS(BrtPort[i].InBuffer.Size)
				,SS(BrtPort[i].BytesPerSecIn)
				,SS(BrtPort[i].BytesPerSecAvgIn)
				,SS(BrtPort[i].BytesPerSecMaxIn)
				,BrtPort[i].BlocksIn
				,BrtPort[i].PacketsIn
				,SS(BrtPort[i].BytesIn)
				);
		printf("\n");
#endif
			BrtPort[NO_OF_BRT_PORTS].BeatsIn+=BrtPort[i].BeatsIn;
			BrtPort[NO_OF_BRT_PORTS].BlocksIn+=BrtPort[i].BlocksIn;
			BrtPort[NO_OF_BRT_PORTS].PacketsIn+=BrtPort[i].PacketsIn;
			BrtPort[NO_OF_BRT_PORTS].BytesIn+=BrtPort[i].BytesPerSecIn;

			BrtPort[NO_OF_BRT_PORTS].BeatsOut+=BrtPort[i].BeatsOut;
			BrtPort[NO_OF_BRT_PORTS].BlocksOut+=BrtPort[i].BlocksOut;
			BrtPort[NO_OF_BRT_PORTS].PacketsOut+=BrtPort[i].PacketsOut;
			BrtPort[NO_OF_BRT_PORTS].BytesOut+=BrtPort[i].BytesPerSecOut;
			BrtPort[NO_OF_BRT_PORTS].Mps+=BrtPort[i].Mps; BrtPort[i].Mps=0;
		}
	}
	sprintf(BrtPort[NO_OF_BRT_PORTS].Status,"TOT %s mps",WSConListSize(BrtPort[NO_OF_BRT_PORTS].Mps));
	BrtPort[NO_OF_BRT_PORTS].Mps=0;
	BrtPort[NO_OF_BRT_PORTS].Seconds++;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecIn=BrtPort[NO_OF_BRT_PORTS].BytesIn-BrtPort[NO_OF_BRT_PORTS].BytesLastIn;
	BrtPort[NO_OF_BRT_PORTS].BytesLastIn=BrtPort[NO_OF_BRT_PORTS].BytesIn;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecAvgIn=BrtPort[NO_OF_BRT_PORTS].BytesIn/BrtPort[NO_OF_BRT_PORTS].Seconds;
	if(BrtPort[NO_OF_BRT_PORTS].BytesPerSecIn>BrtPort[NO_OF_BRT_PORTS].BytesPerSecMaxIn)
		BrtPort[NO_OF_BRT_PORTS].BytesPerSecMaxIn=BrtPort[NO_OF_BRT_PORTS].BytesPerSecIn;

	BrtPort[NO_OF_BRT_PORTS].BytesPerSecOut=BrtPort[NO_OF_BRT_PORTS].BytesOut-BrtPort[NO_OF_BRT_PORTS].BytesLastOut;
	BrtPort[NO_OF_BRT_PORTS].BytesLastOut=BrtPort[NO_OF_BRT_PORTS].BytesOut;
	BrtPort[NO_OF_BRT_PORTS].BytesPerSecAvgOut=BrtPort[NO_OF_BRT_PORTS].BytesOut/BrtPort[NO_OF_BRT_PORTS].Seconds;
	if(BrtPort[NO_OF_BRT_PORTS].BytesPerSecOut>BrtPort[NO_OF_BRT_PORTS].BytesPerSecMaxOut)
		BrtPort[NO_OF_BRT_PORTS].BytesPerSecMaxOut=BrtPort[NO_OF_BRT_PORTS].BytesPerSecOut;
#endif // BROADCAST_TRANSMITTER

#ifndef WS_DISPOFF
#ifndef UNIX
	ListView_Update(pdoc->pmod->WShConList, 0);
#endif
	sprintf(szTemp1," (Ti:%ld)(Mi:%I64d)",pdoc->pmod->lastTickDiff,theApp.LastIdle);
	strcat(szTemp,szTemp1);
	if(NoOfUscs>0)
	{
		sprintf(szTemp1," (U:%d)",NoOfUscs);
		strcat(szTemp,szTemp1);
	}
	if(NoOfUgcs>0)
	{
		sprintf(szTemp1," (G:%d)",NoOfUgcs);
		strcat(szTemp,szTemp1);
	}
	if(NoOfUsers>0)
	{
		sprintf(szTemp1," (u:%d)",NoOfUsers);
		theApp.WSNoOfUserPortsUsed = NoOfUsers;
		strcat(szTemp,szTemp1);
	}
	if(NoOfUmcs>0)
	{
		sprintf(szTemp1," (M:%d)",NoOfUmcs);
		strcat(szTemp,szTemp1);
	}
	if(NoOfUmers>0)
	{
		sprintf(szTemp1," (m:%d)",NoOfUmers);
		strcat(szTemp,szTemp1);
	}
	if(NoOfCons>0)
	{
		sprintf(szTemp1," (C:%d)",NoOfCons);
		strcat(szTemp,szTemp1);
	}
	if(NoOfCgds>0)
	{
		sprintf(szTemp1," (D:%d)",NoOfCgds);
		strcat(szTemp,szTemp1);
	}
	if(NoOfFiles>0)
	{
		sprintf(szTemp1," (F:%d)",NoOfFiles);
		strcat(szTemp,szTemp1);
	}
	if(theApp.LastErrorCnt!=0)
	{
		sprintf(szTemp1," (E:%ld)",theApp.LastErrorCnt);
		strcat(szTemp,szTemp1);
	}
#ifdef WS_OIO
#elif defined(WS_OTPP)
#elif defined(WS_COMPLETION_PORT_SENDS)
	sprintf(szTemp1, " (IOCP:%s)",SizeStr(iocpWriteBytes));
	strcat(szTemp,szTemp1);
#endif
    WSSetAppStatus(szTemp);
//#ifdef WS_MONITOR
//	theApp.WSHSendMonitorData(pdoc->pmod,-1);
//#endif //#ifdef WS_MONITOR
#ifdef WS_UPDATE_CALLBACK
	WSUpdateCallBack();
#endif
#endif// NOT WS_DISPOFF
}

// Add Item to ConList
int CIQMatrixView::AddConListItem(WSPortType PortType, int PortNo)
{
	LVITEM LvItem;
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;

	char PortName[32]={0};
	GetDispPort(PortType,PortNo,PortName);

	// Find any entries that didn't get cleaned up on last disconnect
	while(pdoc->pmod->WShConList)
	{
		LV_FINDINFO lvfi;
		memset(&lvfi,0,sizeof(LV_FINDINFO));
		lvfi.flags=LVFI_PARAM;
		lvfi.lParam=MAKELONG(PortNo,PortType);
		int iItem=ListView_FindItem(pdoc->pmod->WShConList,-1,&lvfi);
		if(iItem>=0)
		{
			theApp.WSHostLogError("!CRASH: Connection list item (%s) not cleaned up before new connection. Previous WSXxxClosed probably crashed!",PortName);
			ListView_DeleteItem(pdoc->pmod->WShConList,iItem);
		}
		else
			break;
	}

	LvItem.mask=LVIF_TEXT | LVIF_PARAM;
	LvItem.iItem=0;
	LvItem.iSubItem=0;
	LvItem.pszText=PortName;
	LvItem.lParam=MAKELONG(PortNo,PortType);
	LvItem.iItem=ListView_InsertItem(pdoc->pmod->WShConList, &LvItem);

	if(ListView_GetItemCount(pdoc->pmod->WShConList)==1)
		pdoc->UpdateAllViews(0,0x03,0);
	return(TRUE);
}

// Delete Item from ConList
int CIQMatrixView::DeleteConListItem(WSPortType PortType, int PortNo)
{
	int iItem;
	LVFINDINFO plvfi;
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;

	plvfi.flags=LVFI_PARAM;
	plvfi.lParam=MAKELONG(PortNo,PortType);
	iItem=ListView_FindItem(pdoc->pmod->WShConList,-1,&plvfi);
	if(iItem<0)
	{
		//theApp.WSHostLogError("!WSOCKS: ERROR : ConList Was not Found for Delete");
		//_ASSERT(false);
		return(-1);
	}
	iItem=ListView_DeleteItem(pdoc->pmod->WShConList, iItem);

	if(iItem<0)
	{
		theApp.WSHostLogError("!WSOCKS: ERROR : ConList Was Found but Not Delete");
		_ASSERT(false);
		return(-1);
	}
	return(iItem);
}

void CIQMatrixView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	CIQMatrixDoc *pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	pdoc->SetTitle(pdoc->pmod->pcfg->aname.c_str());

	CreateDirectory(pdoc->pmod->pcfg->LogPath.c_str(),0);

	pdoc->pmod->WShInst=theApp.m_hInstance;
	pdoc->pmod->WShWnd=m_hWnd;
	pdoc->pmod->WS_USER=WSH_USER;
	if(!pdoc->pmod->pcfg->Menu.empty())
		hmenu=LoadMenu((HINSTANCE)pdoc->pmod->dllhnd,pdoc->pmod->pcfg->Menu.c_str());
	ResetErrorCnt();
	CMDIChildWnd *pframe=(CMDIChildWnd*)this->GetParentFrame();
	pframe->MDIMaximize();
}

void CIQMatrixView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	WSResize(cx,cy);
}

void CIQMatrixView::UILogEntry(HWND dlgWnd, DWORD mask, const char *estr)
{
	CIQMatrixDoc* pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;

#ifndef WS_DISPOFF
	char Tempstr[2048]={0},Tempstr1[2048]={0};
	strncpy(Tempstr,estr,2047); Tempstr[2047]=0;
	char *tptr=Tempstr +strlen(Tempstr) -2;
	if((*tptr=='\r')||(*tptr=='\n')) *tptr=0;
	int DelItem;
	while((DelItem=(int)::SendMessage(::GetDlgItem(dlgWnd,WSH_LISTBOX), LB_GETCOUNT, 0, (LPARAM) Tempstr))>WS_LOG_MAX)
		::SendMessage(::GetDlgItem(dlgWnd,WSH_LISTBOX), LB_DELETESTRING, DelItem-1, 0);
		for(int j=(((int)strlen(&Tempstr[11]))/WS_LOG_WIDTH);j>=0;j--)
		{
			if(j==0)
			{
				strncpy(Tempstr1,Tempstr,WS_LOG_WIDTH+11);
				Tempstr1[WS_LOG_WIDTH+11]=0;
			}
			else
			{
				memset(Tempstr1,' ',20);
				strncpy(&Tempstr1[17],&Tempstr[j*WS_LOG_WIDTH+11],WS_LOG_WIDTH);
				Tempstr1[WS_LOG_WIDTH+17]=0;
			}
			::SendMessage(::GetDlgItem(dlgWnd,WSH_LISTBOX), LB_INSERTSTRING, 0, (LPARAM) Tempstr1);
		}
#endif// NOT WS_DISPOFF
}
void CIQMatrixView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CIQMatrixDoc* pdoc=GetDocument();
	if(!pdoc->pmod)
		return;
	// Port created
	if(lHint==0x01)
	{
		WSPort *pport=(WSPort *)pHint;
		switch(pport->PortType)
		{
		case WS_CON:
		case WS_USR:
		case WS_FIL:
		case WS_CGD:
		case WS_UGR:
		case WS_CTO:
		case WS_CTI:
		case WS_BRT:
		case WS_BRR:
			;
		};
		AddConListItem(pport->PortType,pport->PortNo);
	}
	// Port updated
	else if(lHint==0x02)
	{
		WSPort *pport=(WSPort *)pHint;
		if(((pport->PortType==WS_USR)||(pport->PortType==WS_UMR)||(pport->PortType==WS_UGR))&&
			(pport->sd==INVALID_SOCKET))
		{
			//ListView_DeleteItem(pdoc->pmod->WShConList,lidx);
			DeleteConListItem(pport->PortType,pport->PortNo);
		}
		else if((pport->PortType==WS_OTH)&&(pport->PortNo>=0)&&(pport->PortNo<NO_OF_OTHER_PORTS)&&
				(!pdoc->pmod->OtherPort[pport->PortNo].InUse))
		{
			DeleteConListItem(pport->PortType,pport->PortNo);
		}
	}
	// Resize for view
	else if(lHint==0x03)
	{
		RECT rect;
		GetClientRect(&rect);
		WSResize(rect.right -rect.left,rect.bottom -rect.top+1);
	}
	// WSLogEvent
	else if(lHint==0x04)
	{
		const char *estr=(const char *)pHint;
		UILogEntry(pdoc->pmod->WShDlgEvent,0x0010,estr);
	}
	// WSLogError
	else if(lHint==0x05)
	{
		const char *estr=(const char *)pHint;
		UILogEntry(pdoc->pmod->WShDlgError,0x0010,estr);
	}
	// WSLogDebug
	else if(lHint==0x06)
	{
		const char *estr=(const char *)pHint;
		UILogEntry(pdoc->pmod->WShDlgEvent,0,estr);
	}
	// WSLogRetry
	else if(lHint==0x07)
	{
		const char *estr=(const char *)pHint;
		UILogEntry(pdoc->pmod->WShDlgEvent,0,estr);
	}
	// WSLogRecover
	else if(lHint==0x08)
	{
		const char *estr=(const char *)pHint;
		UILogEntry(pdoc->pmod->WShDlgEvent,0,estr);
	}
	else if(lHint==0x09)
	{
		const char *head=(const char *)pHint;
		pdoc->SetTitle(head);
	}
	else if(lHint==0x0A)
	{
		char *status=(char *)pHint;
		WSSetAppStatus(status);
	}
	// Port deleted
	else if(lHint==0x0B)
	{
		int PortType=HIWORD((int)(PTRCAST)pHint);
		int PortNo=LOWORD((int)(PTRCAST)pHint);
		if(PortType==WS_CON)
			pdoc->pmod->WSLogEvent("CON%d peer app shutdown.",PortNo);
		DeleteConListItem((WSPortType)PortType,PortNo);
	}
}

int CIQMatrixView::WSConGenPortLine(CONPORT *ConPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	int Current=ConPort->CurrentAltIP;
	if(Current<0) Current=0;
	if((!ConPort->AltRemotePort[Current])||(ConPort->AltRemotePort[Current]==ConPort->Port))
		sprintf(tptr,"C%d:AUTO:%d:%s",PortNo,ConPort->Port,ConPort->AltRemoteIP[Current]);
	else
		sprintf(tptr,"C%d:AUTO:%d:%s/%d",PortNo,ConPort->Port,ConPort->AltRemoteIP[Current],ConPort->AltRemotePort[Current]);
	tptr+=strlen(tptr);
	bool addRoam=false;
	for (int j=0;j<ConPort->AltIPCount;j++)
	{
		if(j!=Current)
		{
			if((!ConPort->AltRemotePort[j])||(ConPort->AltRemotePort[j]==ConPort->Port))
				sprintf(tptr,":A%s",ConPort->AltRemoteIP[j]);
			else
				sprintf(tptr,":A%s/%d",ConPort->AltRemoteIP[j],ConPort->AltRemotePort[j]); 
			tptr+=strlen(tptr);
			if(ConPort->AltRemoteIPOn[j])
				addRoam=true;
		}
	}
	if(addRoam)
	{
		sprintf(tptr,":R"); tptr+=strlen(tptr);
	}
#ifdef WS_COMPRESS
	if((ConPort->Compressed)&&(!ConPort->Encrypted))
	{
		if(ConPort->CompressType>0)
			sprintf(tptr,":C%d",ConPort->CompressType);
		else
			sprintf(tptr,":C");
		tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(ConPort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(ConPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(ConPort->Bounce)
	{
		sprintf(tptr,":B%d",ConPort->Bounce); tptr+=strlen(tptr);
		if(ConPort->BounceStart)
		{
			sprintf(tptr,":<%d",ConPort->BounceStart); tptr+=strlen(tptr);
		}
		if(ConPort->BounceEnd)
		{
			sprintf(tptr,":>%d",ConPort->BounceEnd); tptr+=strlen(tptr);
		}
	}
	if(ConPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",ConPort->BlockSize); tptr+=strlen(tptr);
	}
	if(ConPort->ChokeSize!=0)
	{
		sprintf(tptr,":L%d",ConPort->ChokeSize); tptr+=strlen(tptr);
	}
	if(ConPort->S5Connect)
	{
		sprintf(tptr,":5"); tptr+=strlen(tptr);
		if(ConPort->S5Port!=1080)
		{
			sprintf(tptr,":6%d",ConPort->S5Port); tptr+=strlen(tptr);
		}
		if(ConPort->S5Methode!=00)
		{
			sprintf(tptr,":7%d",ConPort->S5Methode); tptr+=strlen(tptr);
		}
		if(ConPort->S5Version!=5)
		{
			sprintf(tptr,":8%d",ConPort->S5Version); tptr+=strlen(tptr);
		}
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(ConPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",ConPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",ConPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(ConPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",ConPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSCgdGenPortLine(CGDPORT *CgdPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	int Current=CgdPort->CurrentAltIP;
	if(Current<0) Current=0;
	sprintf(tptr,"D%d:AUTO:%d:%s",PortNo,CgdPort->Port,CgdPort->AltRemoteIP[Current]); tptr+=strlen(tptr);
	bool addRoam=false;
	for (int j=0;j<CgdPort->AltIPCount;j++)
	{
		if(j!=Current)
		{
			sprintf(tptr,":A%s",CgdPort->AltRemoteIP[j]); tptr+=strlen(tptr);
			if(CgdPort->AltRemoteIPOn[j])
				addRoam=true;
		}
	}
	if(addRoam)
	{
		sprintf(tptr,":R"); tptr+=strlen(tptr);
	}
#ifdef WS_COMPRESS
	if((CgdPort->Compressed)&&(!CgdPort->Encrypted))
	{
		sprintf(tptr,":C"); tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(CgdPort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(CgdPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(CgdPort->Bounce)
	{
		sprintf(tptr,":B%d",CgdPort->Bounce); tptr+=strlen(tptr);
		if(CgdPort->BounceStart)
		{
			sprintf(tptr,":<%d",CgdPort->BounceStart); tptr+=strlen(tptr);
		}
		if(CgdPort->BounceEnd)
		{
			sprintf(tptr,":>%d",CgdPort->BounceEnd); tptr+=strlen(tptr);
		}
	}
	if(CgdPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",CgdPort->BlockSize); tptr+=strlen(tptr);
	}
	if(CgdPort->ChokeSize!=0)
	{
		sprintf(tptr,":L%d",CgdPort->ChokeSize); tptr+=strlen(tptr);
	}
	if(CgdPort->S5Connect)
	{
		sprintf(tptr,":5"); tptr+=strlen(tptr);
		if(CgdPort->S5Port!=1080)
		{
			sprintf(tptr,":6%d",CgdPort->S5Port); tptr+=strlen(tptr);
		}
		if(CgdPort->S5Methode!=00)
		{
			sprintf(tptr,":7%d",CgdPort->S5Methode); tptr+=strlen(tptr);
		}
		if(CgdPort->S5Version!=5)
		{
			sprintf(tptr,":8%d",CgdPort->S5Version); tptr+=strlen(tptr);
		}
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(CgdPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",CgdPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",CgdPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	sprintf(tptr,":G%d,%s,%s",CgdPort->GDId.LineId,CgdPort->GDId.LineName,CgdPort->GDId.ClientId); tptr+=strlen(tptr);
	if(CgdPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",CgdPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSFileGenPortLine(FILEPORT *FilePort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	int Current=FilePort->CurrentAltIP;
	if(Current<0) Current=0;
	sprintf(tptr,"F%d:AUTO:%d:%s",PortNo,FilePort->Port,FilePort->AltRemoteIP[Current]); tptr+=strlen(tptr);
	bool addRoam=false;
	for (int j=0;j<FilePort->AltIPCount;j++)
	{
		if(j!=Current)
		{
			sprintf(tptr,":A%s",FilePort->AltRemoteIP[j]); tptr+=strlen(tptr);
			if(FilePort->AltRemoteIPOn[j])
				addRoam=true;
		}
	}
	if(addRoam)
	{
		sprintf(tptr,":R"); tptr+=strlen(tptr);
	}
#ifdef WS_COMPRESS
	if((FilePort->Compressed)&&(!FilePort->Encrypted))
	{
		sprintf(tptr,":C"); tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(FilePort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(FilePort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(FilePort->Bounce)
	{
		sprintf(tptr,":B%d",FilePort->Bounce); tptr+=strlen(tptr);
		if(FilePort->BounceStart)
		{
			sprintf(tptr,":<%d",FilePort->BounceStart); tptr+=strlen(tptr);
		}
		if(FilePort->BounceEnd)
		{
			sprintf(tptr,":>%d",FilePort->BounceEnd); tptr+=strlen(tptr);
		}
	}
	if(FilePort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",FilePort->BlockSize); tptr+=strlen(tptr);
	}
	if(FilePort->ChokeSize!=0)
	{
		sprintf(tptr,":L%d",FilePort->ChokeSize); tptr+=strlen(tptr);
	}
	if(FilePort->S5Connect)
	{
		sprintf(tptr,":5"); tptr+=strlen(tptr);
		if(FilePort->S5Port!=1080)
		{
			sprintf(tptr,":6%d",FilePort->S5Port); tptr+=strlen(tptr);
		}
		if(FilePort->S5Methode!=00)
		{
			sprintf(tptr,":7%d",FilePort->S5Methode); tptr+=strlen(tptr);
		}
		if(FilePort->S5Version!=5)
		{
			sprintf(tptr,":8%d",FilePort->S5Version); tptr+=strlen(tptr);
		}
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(FilePort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",FilePort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",FilePort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(FilePort->CfgNote[0])
	{
		sprintf(tptr,":N%s",FilePort->CfgNote); tptr+=strlen(tptr);
	}
	sprintf(tptr,":D%s",FilePort->DriveList); tptr+=strlen(tptr);
	return 0;
}
int CIQMatrixView::WSUscGenPortLine(USCPORT *UscPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	sprintf(tptr,"U%d:%s:%d",PortNo,UscPort->LocalIP,UscPort->Port); tptr+=strlen(tptr);
#ifdef WS_COMPRESS
	if((UscPort->Compressed)&&(!UscPort->Encrypted))
	{
		if(UscPort->CompressType>0)
			sprintf(tptr,":C%d",UscPort->CompressType);
		else
			sprintf(tptr,":C");
		tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(UscPort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(UscPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(UscPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",UscPort->BlockSize); tptr+=strlen(tptr);
	}
	if((UscPort->TimeOut)&&(UscPort->TimeOut!=1000000))
	{
		sprintf(tptr,":T%d",UscPort->TimeOut); tptr+=strlen(tptr);
	}
	if((UscPort->TimeOutSize)&&(UscPort->TimeOutSize!=60))
	{
		sprintf(tptr,":S%d",UscPort->TimeOutSize); tptr+=strlen(tptr);
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(UscPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",UscPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",UscPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(UscPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",UscPort->CfgNote); tptr+=strlen(tptr);
	}
	if(UscPort->IPAclCfg[0])
	{
		sprintf(tptr,":I%s",UscPort->IPAclCfg); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSUgcGenPortLine(UGCPORT *UgcPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	sprintf(tptr,"G%d:%s:%d",PortNo,UgcPort->LocalIP,UgcPort->Port); tptr+=strlen(tptr);
#ifdef WS_COMPRESS
	if((UgcPort->Compressed)&&(!UgcPort->Encrypted))
	{
		sprintf(tptr,":C"); tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(UgcPort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(UgcPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(UgcPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",UgcPort->BlockSize); tptr+=strlen(tptr);
	}
	if((UgcPort->TimeOut)&&(UgcPort->TimeOut!=1000000))
	{
		sprintf(tptr,":T%d",UgcPort->TimeOut); tptr+=strlen(tptr);
	}
	if((UgcPort->TimeOutSize)&&(UgcPort->TimeOutSize!=60))
	{
		sprintf(tptr,":S%d",UgcPort->TimeOutSize); tptr+=strlen(tptr);
	}
	if(UgcPort->GDCfg[0])
	{
		sprintf(tptr,":G%s",UgcPort->GDCfg); tptr+=strlen(tptr);
	}
	if(UgcPort->GDGap!=100)
	{
		sprintf(tptr,":J%d",UgcPort->GDGap); tptr+=strlen(tptr);
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(UgcPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",UgcPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",UgcPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(UgcPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",UgcPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSUmcGenPortLine(UMCPORT *UmcPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	sprintf(tptr,"M%d:%s:%d",PortNo,UmcPort->LocalIP,UmcPort->Port); tptr+=strlen(tptr);
#ifdef WS_COMPRESS
	if((UmcPort->Compressed)&&(!UmcPort->Encrypted))
	{
		sprintf(tptr,":C");	tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(UmcPort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(UmcPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(UmcPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",UmcPort->BlockSize); tptr+=strlen(tptr);
	}
	if((UmcPort->TimeOut)&&(UmcPort->TimeOut!=1000000))
	{
		sprintf(tptr,":T%d",UmcPort->TimeOut); tptr+=strlen(tptr);
	}
	if((UmcPort->TimeOutSize)&&(UmcPort->TimeOutSize!=60))
	{
		sprintf(tptr,":S%d",UmcPort->TimeOutSize); tptr+=strlen(tptr);
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(UmcPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",UmcPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",UmcPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(UmcPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",UmcPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSCtoGenPortLine(CTOPORT *CtoPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	sprintf(tptr,"O%d:%s:%d:%s",PortNo,CtoPort->LocalIP,CtoPort->Port,CtoPort->RemoteIP); tptr+=strlen(tptr);
	if(CtoPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(CtoPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",CtoPort->BlockSize); tptr+=strlen(tptr);
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(CtoPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",CtoPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",CtoPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(CtoPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",CtoPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSCtiGenPortLine(CTIPORT *CtiPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	int Current=CtiPort->CurrentAltIP;
	if(Current<0) Current=0;
	sprintf(tptr,"I%d:%s:%d:%s",PortNo,CtiPort->LocalIP,CtiPort->Port,CtiPort->AltRemoteIP[Current]); tptr+=strlen(tptr);
	bool addRoam=false;
	for (int j=0;j<CtiPort->AltIPCount;j++)
	{
		if(j!=Current)
		{
			sprintf(tptr,":A%s",CtiPort->AltRemoteIP[j]); tptr+=strlen(tptr);
			if(CtiPort->AltRemoteIPOn[j])
				addRoam=true;
		}
	}
	if(addRoam)
	{
		sprintf(tptr,":R"); tptr+=strlen(tptr);
	}
	if(CtiPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(CtiPort->Bounce)
	{
		sprintf(tptr,":B%d",CtiPort->Bounce); tptr+=strlen(tptr);
		if(CtiPort->BounceStart)
		{
			sprintf(tptr,":<%d",CtiPort->BounceStart); tptr+=strlen(tptr);
		}
		if(CtiPort->BounceEnd)
		{
			sprintf(tptr,":>%d",CtiPort->BounceEnd); tptr+=strlen(tptr);
		}
	}
	if(CtiPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",CtiPort->BlockSize); tptr+=strlen(tptr);
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(CtiPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",CtiPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",CtiPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(CtiPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",CtiPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}
int CIQMatrixView::WSVGenPortCfg()
{
	CIQMatrixDoc* pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	char fpath[MAX_PATH];
	sprintf(fpath,"setup\\ports_%d-%d.txt",pdoc->pmod->WSDate,pdoc->pmod->WSTime);
	FILE *fp=fopen(fpath,"wt");
	if(!fp)
	{
		WSLogError("Failed opening (%s) for writing!",fpath);
		return -1;
	}
	fprintf(fp,"// Generated by WSOCKS on %d %d\r\n// Con Ports\r\n",pdoc->pmod->WSDate,pdoc->pmod->WSTime);
	int i;
	for(i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(ConPort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSConGenPortLine(&ConPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#ifdef WS_GUARANTEED
	fprintf(fp,"// Cgd Ports\n");
	for(i=0;i<NO_OF_CGD_PORTS;i++)
	{
		if(CgdPort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSCgdGenPortLine(&CgdPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#endif
#ifdef WS_FILE_SERVER
	fprintf(fp,"// File Ports\n");
	for(i=0;i<NO_OF_FILE_PORTS;i++)
	{
		if(FilePort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSFileGenPortLine(&FilePort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#endif

	fprintf(fp,"// Usc Ports\n");
	for(i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(UscPort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSUscGenPortLine(&UscPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#ifdef WS_GUARANTEED
	fprintf(fp,"// Ugc Ports\n");
	for(i=0;i<NO_OF_UGC_PORTS;i++)
	{
		if(UgcPort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSUgcGenPortLine(&UgcPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#endif
#ifdef WS_MONITOR
	fprintf(fp,"// Umr Ports\n");
	for(i=0;i<NO_OF_UMC_PORTS;i++)
	{
		if(UmcPort[i].SockActive)
		{
			char pline[256];
			memset(pline,0,256);
			WSUmcGenPortLine(&UmcPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#endif

#ifdef WS_CAST
	fprintf(fp,"// Cto Ports\n");
	for(i=0;i<NO_OF_CTO_PORTS;i++)
	{
		if(CtoPort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSCtoGenPortLine(&CtoPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
	fprintf(fp,"// Cti Ports\n");
	for(i=0;i<NO_OF_CTI_PORTS;i++)
	{
		if(CtiPort[i].InUse)
		{
			char pline[256];
			memset(pline,0,256);
			WSCtiGenPortLine(&CtiPort[i],i,pline,256);
			fprintf(fp,"%s\n",pline);
		}
	}
#endif
	fclose(fp);
	ShellExecute(pdoc->pmod->WShWnd, "open", fpath, 0, 0, SW_SHOW);

	char cpath[MAX_PATH],*cptr=0;
	GetFullPathName("setup\\ports.txt",sizeof(cpath),cpath,&cptr);
	char wmsg[256];
	sprintf(wmsg,"Replace %s?",cpath);
	if(::MessageBox(pdoc->pmod->WShWnd,wmsg,"Confirm Overwrite",MB_ICONQUESTION|MB_YESNO)==IDYES)
	{
		char bpath[MAX_PATH];
		WIN32_FIND_DATAA fdata;
		HANDLE fhnd=FindFirstFile(cpath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			SYSTEMTIME tsys;
			FileTimeToSystemTime(&fdata.ftLastWriteTime,&tsys);
			FindClose(fhnd);
			sprintf(bpath,"setup\\ports_%04d%02d%02d-%02d%02d%02d.txt",
				tsys.wYear,tsys.wMonth,tsys.wDay,tsys.wHour,tsys.wMinute,tsys.wSecond);
			if(PathFileExists(bpath))
			{
				sprintf(wmsg,"Overwrite %s?",bpath);
				if(::MessageBox(pdoc->pmod->WShWnd,wmsg,"Confirm Overwrite",MB_ICONQUESTION|MB_YESNO)!=IDYES)
					return 0;
			}
		}
		else
			WSLogError("Warning: (%s) not found!",cpath);
		if(!MoveFile(cpath,bpath))
		{
			WSLogError("Failed moving (%s) to (%s)!",cpath,bpath);
			return -1;
		}
		if(!MoveFile(fpath,cpath))
		{
			WSLogError("Failed moving (%s) to (%s)!",fpath,cpath);
			return -1;
		}
		ShellExecute(pdoc->pmod->WShWnd, "open", cpath, 0, 0, SW_SHOW);
	}
	return 0;
}

int CIQMatrixView::WSVSaveToCsv()
{
	CIQMatrixDoc* pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	HWND clwnd=ListView_GetHeader(pdoc->pmod->WShConList);

	char spath[MAX_PATH], *sptr;
	memset(spath, 0, sizeof(spath));
	GetFullPathName("logs\\conlist.csv", sizeof(spath), spath, &sptr);
	//if ( PathFileExists(spath) )
	//{
	//	char wmsg[256]={0};
	//	sprintf(wmsg,"Overwrite \"%s\"?",spath);
	//	if ( MessageBox(pdoc->pmod->WShConList, wmsg, "Confirm Overwrite", MB_ICONWARNING|MB_YESNO) != IDYES )
	//		return -1;
	//}
	HANDLE fhnd = CreateFile(spath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if ( fhnd == INVALID_HANDLE_VALUE )
	{
		char wmsg[256]={0};
		sprintf(wmsg,"Failed opening \"%s\"!", spath);
		::MessageBox(pdoc->pmod->WShConList, wmsg, "Error", MB_ICONERROR);
		return -1;
	}
	DWORD wbytes = 0;
	
	int NumberColums = Header_GetItemCount(clwnd);
	int Column;
	for (Column = 0;Column < NumberColums;Column++)
	{
		char label[256];
		HDITEM hdi;
		memset(&hdi, 0, sizeof(hdi));
		hdi.mask = HDI_TEXT;
		hdi.pszText = label;
		hdi.cchTextMax = sizeof(label);
		Header_GetItem(clwnd, Column, &hdi);
		if (Column != 0)
			WriteFile(fhnd,",",1,&wbytes,0);
		WriteFile(fhnd,"\"",1,&wbytes,0);
		WriteFile(fhnd,label,(int)strlen(label),&wbytes,0);
		WriteFile(fhnd,"\"",1,&wbytes,0);
	}
	WriteFile(fhnd,"\r\n",2,&wbytes,0);

	int crows = ListView_GetItemCount(pdoc->pmod->WShConList);
	for(int i=0;i<crows;i++)
	{
		//LVITEM lvi;
		//lvi.iItem=i;
		//lvi.mask=LVIF_STATE;
		//lvi.stateMask=LVIS_SELECTED;
		//ListView_GetItem(pdoc->pmod->WShConList,&lvi);
		//if(!(lvi.state&LVIS_SELECTED))
		//	continue;
		
		for (Column = 0;Column < NumberColums;Column++)
		{
			char str[1024]={0};
			ListView_GetItemText(pdoc->pmod->WShConList,i,Column,str,1024);
			if (Column != 0)
				WriteFile(fhnd,",",1,&wbytes,0);
			WriteFile(fhnd,"\"",1,&wbytes,0);
			WriteFile(fhnd,str,(int)strlen(str),&wbytes,0);
			WriteFile(fhnd,"\"",1,&wbytes,0);
		}
		WriteFile(fhnd,"\r\n",2,&wbytes,0);
	}
	CloseHandle(fhnd);
	
	ShellExecute(pdoc->pmod->WShConList, "open", "notepad.exe", spath, 0, SW_SHOW);
	return 0;
}

int CIQMatrixView::WSVDumpHold(int PortType, int PortNo)
{
	CIQMatrixDoc* pdoc=GetDocument();
	if(!pdoc->pmod)
		return FALSE;
	CONPORT *ConPort=pdoc->pmod->ConPort;
	USCPORT *UscPort=pdoc->pmod->UscPort;
	USRPORT *UsrPort=pdoc->pmod->UsrPort;
	FILEPORT *FilePort=pdoc->pmod->FilePort;
	UMCPORT *UmcPort=pdoc->pmod->UmcPort;
	UMRPORT *UmrPort=pdoc->pmod->UmrPort;
	CGDPORT *CgdPort=pdoc->pmod->CgdPort;
	UGCPORT *UgcPort=pdoc->pmod->UgcPort;
	UGRPORT *UgrPort=pdoc->pmod->UgrPort;
	CTOPORT *CtoPort=pdoc->pmod->CtoPort;
	CTIPORT *CtiPort=pdoc->pmod->CtiPort;
	OTHERPORT *OtherPort=pdoc->pmod->OtherPort;

	const char *PortTypeStr="???";
	BUFFER *InBuffer=0;
	switch(PortType)
	{
	case WS_CON: PortTypeStr="CON"; InBuffer=&ConPort[PortNo].InBuffer; break;
	case WS_USR: PortTypeStr="USR"; InBuffer=&UsrPort[PortNo].InBuffer; break;
#ifdef WS_FILE_SERVER
	case WS_FIL: PortTypeStr="FIL"; InBuffer=&FilePort[PortNo].InBuffer; break;
#endif
#ifdef WS_GUARANTEED
	case WS_CGD: PortTypeStr="CGD"; InBuffer=&CgdPort[PortNo].InBuffer; break;
	case WS_UGR: PortTypeStr="UGR"; InBuffer=&UgrPort[PortNo].InBuffer; break;
#endif
#ifdef WS_MONITOR
	case WS_UMR: PortTypeStr="UMR"; InBuffer=&UmrPort[PortNo].InBuffer; break;
#endif
	}
	HWND clwnd=ListView_GetHeader(pdoc->pmod->WShConList);

	char sname[MAX_PATH], spath[MAX_PATH], *sptr;
	sprintf(sname,"logs\\hold_%s%d.csv",PortTypeStr,PortNo);
	memset(spath, 0, sizeof(spath));
	GetFullPathName(sname, sizeof(spath), spath, &sptr);
	if ( PathFileExists(spath) )
	{
		char wmsg[256]={0};
		sprintf(wmsg,"Overwrite \"%s\"?",spath);
		if ( ::MessageBox(pdoc->pmod->WShConList, wmsg, "Confirm Overwrite", MB_ICONWARNING|MB_YESNO) != IDYES )
			return -1;
	}
	HANDLE fhnd = CreateFile(spath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if ( fhnd == INVALID_HANDLE_VALUE )
	{
		char wmsg[256]={0};
		sprintf(wmsg,"Failed opening \"%s\"!", spath);
		::MessageBox(pdoc->pmod->WShConList, wmsg, "Error", MB_ICONERROR);
		return -1;
	}
	for(BUFFBLOCK *BufBlock=InBuffer->TopBlock;BufBlock!=0;BufBlock=(BUFFBLOCK*)BufBlock->NextBuffBlock)
	{
		DWORD wbytes = 0;
		WriteFile(fhnd,BufBlock->Block,BufBlock->Size,&wbytes,0);
	}
	CloseHandle(fhnd);
	
	ShellExecute(pdoc->pmod->WShConList, "open", "notepad.exe", spath, 0, SW_SHOW);
	return 0;
}
