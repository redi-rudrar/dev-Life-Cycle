// IQMatrixDoc.cpp : implementation of the CIQMatrixDoc class
//

#include "stdafx.h"
#include "IQMatrix.h"

#include "IQMatrixDoc.h"
#include ".\iqmatrixdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIQMatrixDoc

IMPLEMENT_DYNCREATE(CIQMatrixDoc, CDocument)

BEGIN_MESSAGE_MAP(CIQMatrixDoc, CDocument)
END_MESSAGE_MAP()


// CIQMatrixDoc construction/destruction
WsocksApp *CIQMatrixDoc::nextDocApp=0;
CIQMatrixDoc::CIQMatrixDoc()
{
	// TODO: add one-time construction code here
	pmod=nextDocApp;
	nextDocApp=0;
}

CIQMatrixDoc::~CIQMatrixDoc()
{
}

BOOL CIQMatrixDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CIQMatrixDoc serialization

void CIQMatrixDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CIQMatrixDoc diagnostics

#ifdef _DEBUG
void CIQMatrixDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CIQMatrixDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CIQMatrixDoc commands
BOOL CIQMatrixDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	// TODO:  Add your specialized creation code here

	return TRUE;
}
void CIQMatrixDoc::OnCloseDocument()
{
	if(pmod)
	{
		bool lastShutdown=theApp.uiShutdown;
		theApp.uiShutdown=true;
		theApp.WSHCloseApp(pmod);
		theApp.uiShutdown=lastShutdown;
		pmod=0;
	}

	CDocument::OnCloseDocument();
}
