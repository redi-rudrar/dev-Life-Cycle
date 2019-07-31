#ifndef _VSOLEDB_H
#define _VSOLEDB_H

#include <oledb.h>
#include <atldbcli.h>
#include <atlcom.h>

// From Session.h
class CQuerySession  
{
public:
	CQuerySession();
	virtual ~CQuerySession();

	HRESULT Open(const char *strUser, const char *strPassword, const char *appId, const char *strServerlist);
	void Close();
	CSession &GetSession() ;

private:
	CDBPropSet m_ps;
	CDataSource m_db;
	CSession m_session;
	BOOL m_bOpen;
};

class CDynamicAccessorEx : public CDynamicAccessor
{
public:
	CDynamicAccessorEx()
	{
#if _MSC_VER > 1200
		m_nBlobSize = 1024 * 64;
#endif
	}
};

enum RediQueryStatus
{
	RQS_UNISSUED,		// Not issued yet
	RQS_INPROGRESS,		// Issued successfully
	RQS_FAILED,			// Failed immediately
	RQS_COMPLETED,		// Finished
	RQS_TIMEOUT,		// Timed out
	RQS_ABORTED,		// Cancelled
	RQS_COUNT
};
// A separate object from CQuery to be accessible from app threads.
class CQueryResult
{
public:
	CQueryResult()
		:m_qid(0)
		,m_porttype(-1)
		,m_portno(-1)
		,m_fp(0)
		,m_udata(0)
		,m_select()
		,m_queryStatus(RQS_UNISSUED)
		,m_rows(0)
		,m_tstart(0)
		,m_tlast(0)
		,m_idleTimeout(0)
		,m_query(0)
	{
	}
	int m_qid;
	int m_porttype;
	int m_portno;
	FILE *m_fp;
	void *m_udata;
	string m_select;
	RediQueryStatus m_queryStatus;
	int m_rows;
	DWORD m_tstart;
	DWORD m_tlast;
	int m_idleTimeout;
	class CQuery *m_query;
};
// From Query.h
class CQueryChangeEventHandler
{
public:
	virtual void OnRowChange (class CQuery *pSender, HROW hRow, DBREASON eReason, DBEVENTPHASE ePhase) = NULL;
};

// CQuery object will only be accessed from RediConnectThread (one that called CoInitialize)
class CQuery: public CCommand<CDynamicAccessorEx>,
			public CComObjectRootEx<CComMultiThreadModel>,
			public IRowsetNotifyImpl
{
public:
	CQuery();
	~CQuery();

	HRESULT _AtlInitialConstruct()
	{
		return S_OK;
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	BEGIN_COM_MAP(CQuery)
		COM_INTERFACE_ENTRY(IRowsetNotify)
	END_COM_MAP()

	HRESULT Open( CQueryChangeEventHandler *eventHandler,
		const CSession& session, LPCTSTR szCommand = NULL,
		DBPROPSET *pPropSet = NULL);

	void ReleaseCommand();
	bool IsOpen() { return m_Valid; }
	bool IsDone() { return m_Done; }
	bool IsIdle(int sec) { return (GetTickCount() -m_LastDataTime>=(DWORD)(sec*1000)) ?true :false; }

	HRESULT ActivateRow(HROW hRow);

	void SetDone() { m_Done=true; }
	int m_PortType;
	int m_PortNo;
	int m_qid;
	DWORD m_LastDataTime;
	CQueryResult *m_queryResult;
	#ifdef IQDEVTEST
	string	m_InsertKey;
	#endif

protected:
	STDMETHOD(OnRowChange)(IRowset* pRowset, ULONG cRows, const HROW rghRows[], DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);
	BOOL ReadInitialSnapshot();
	BOOL EnableNotify();
	void DisableNotify();
protected:
	CQueryChangeEventHandler *m_eventHandler;
	DWORD	m_cookie;
	bool	m_Valid;
	bool	m_Done;
};

// From QueryClient.h
//class CQuerySampleClient: public CQueryChangeEventHandler
//{
//public:
//	CQuerySampleClient() :updateCnt(0) {}
//
//	void OnRowChange (CQuery *pSender, HROW hRow, DBREASON eReason, DBEVENTPHASE ePhase);
//private:
//	void PrintRow(LPCTSTR prefix, CQuery *pSender, HROW hRow);
//
//	int updateCnt;
//};

#endif _VSOLEDB_H
