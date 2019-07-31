// IQMatrixView.h : interface of the CIQMatrixView class
//


#pragma once


class CIQMatrixView : public CView
{
protected: // create from serialization only
	CIQMatrixView();
	DECLARE_DYNCREATE(CIQMatrixView)

// Attributes
public:
	CIQMatrixDoc* GetDocument() const;

// Operations
public:
	// Legacy connection list and dialogs
	int WSConClick(int PortNo){return 0;}
	int WSUscClick(int PortNo){return 0;}
	int WSUsrClick(int PortNo){return 0;}
	int WSFileClick(int PortNo){return 0;}
	int WSUmcClick(int PortNo){return 0;}
	int WSUmrClick(int PortNo){return 0;}
	int WSCgdClick(int PortNo){return 0;}
	int WSUgcClick(int PortNo){return 0;}
	int WSUgrClick(int PortNo){return 0;}
	int WSCtoClick(int PortNo){return 0;}
	int WSCtiClick(int PortNo){return 0;}
	int WSOtherClick(int PortNo){return 0;}

	LRESULT CALLBACK cbReport(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbConPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbUscPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbUsrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbFilePortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbUmcPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbUmrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbCgdPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbUgcPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbUgrPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbCtoPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbCtiPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK cbOtherPortSetupBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	void ConPortSetupBox(int PortNo);
	void UscPortSetupBox(int PortNo);
	void UsrPortSetupBox(int PortNo);
	void FilePortSetupBox(int PortNo);
	void UmcPortSetupBox(int PortNo);
	void UmrPortSetupBox(int PortNo);
	void CgdPortSetupBox(int PortNo);
	void UgcPortSetupBox(int PortNo);
	void UgrPortSetupBox(int PortNo);
	void CtoPortSetupBox(int PortNo);
	void CtiPortSetupBox(int PortNo);
	void OtherPortSetupBox(int PortNo);

	void GetDispPort(WSPortType Type, int PortNo, char *PortName);
	LRESULT CALLBACK cbConList(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void ResetErrorCnt(void);
	int WSResize(int cx,int cy);
	void WSSetAppHead(char *Head);
	void WSSetAppStatus(char *Status);
	void CALLBACK WSDisplayStatus(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	int AddConListItem(WSPortType PortType, int PortNo);
	int DeleteConListItem(WSPortType PortType, int PortNo);
	//int WSVSetAppHead(char *Head);
	void UILogEntry(HWND dlgWnd, DWORD mask, const char *estr);

	int WSConGenPortLine(CONPORT *ConPort,int PortNo,char *pline,int len);
	int WSCgdGenPortLine(CGDPORT *CgdPort,int PortNo,char *pline,int len);
	int WSFileGenPortLine(FILEPORT *FilePort,int PortNo,char *pline,int len);
	int WSUscGenPortLine(USCPORT *UscPort,int PortNo,char *pline,int len);
	int WSUgcGenPortLine(UGCPORT *UgcPort,int PortNo,char *pline,int len);
	int WSUmcGenPortLine(UMCPORT *UmcPort,int PortNo,char *pline,int len);
	int WSCtoGenPortLine(CTOPORT *CtoPort,int PortNo,char *pline,int len);
	int WSCtiGenPortLine(CTIPORT *CtiPort,int PortNo,char *pline,int len);
	int WSVGenPortCfg();

	int WSVSaveToCsv();
	int WSVDumpHold(int PortType, int PortNo);

	LPARAM dlgParam;
	HMENU hmenu;

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CIQMatrixView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
};

#ifndef _DEBUG  // debug version in IQMatrixView.cpp
inline CIQMatrixDoc* CIQMatrixView::GetDocument() const
   { return reinterpret_cast<CIQMatrixDoc*>(m_pDocument); }
#endif

