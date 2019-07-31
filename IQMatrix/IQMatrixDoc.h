// IQMatrixDoc.h : interface of the CIQMatrixDoc class
//


#pragma once

class CIQMatrixDoc : public CDocument
{
protected: // create from serialization only
	CIQMatrixDoc();
	DECLARE_DYNCREATE(CIQMatrixDoc)

// Attributes
public:
	static WsocksApp *nextDocApp;
	WsocksApp *pmod;
	string heading;
	string status;

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CIQMatrixDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnCloseDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
};


