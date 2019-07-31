
#include "stdafx.h"
#ifdef REDIOLEDBCON
#include "vsoledb.h"
#include <atlstr.h>
#include <ATLComTime.h>

// From Session.cpp
CQuerySession::CQuerySession() : m_ps(DBPROPSET_DBINIT)
{
	m_bOpen = FALSE;
	#ifndef IQDEVTEST
	m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "");
	m_ps.AddProperty(DBPROP_INIT_LCID, (long)0);
	m_ps.AddProperty(DBPROP_INIT_LOCATION, "");
	m_ps.AddProperty(DBPROP_INIT_MODE, (long)31);
	m_ps.AddProperty(DBPROP_INIT_PROMPT, (short)2);
	m_ps.AddProperty(DBPROP_INIT_PROVIDERSTRING, ""); // need IP addresses
	m_ps.AddProperty(DBPROP_INIT_TIMEOUT, (long)20);
	#endif
}

CQuerySession::~CQuerySession()
{
	Close();
}

#define DBPROP_INIT_APPID 0xffff0006L

HRESULT CQuerySession::Open(const char *strUser, const char *strPassword, const char *appId, const char *strServerlist)
{
	Close();

	#ifdef IQDEVTEST
	// Dev Nexus Database
	//m_ps.AddProperty(DBPROP_AUTH_USERID, "SymbolsUser");
	//m_ps.AddProperty(DBPROP_AUTH_PASSWORD, "symbolspass");
	// Dev StockLoan Database
	//m_ps.AddProperty(DBPROP_AUTH_USERID, REDIBUS_USER.c_str());
	//m_ps.AddProperty(DBPROP_AUTH_PASSWORD, REDIBUS_PASS.c_str());
	m_ps.AddProperty(DBPROP_AUTH_USERID, "u24601");
	m_ps.AddProperty(DBPROP_AUTH_PASSWORD, "Abc123");
	// .132 SQL Server
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "171.130.66.132");
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "\\\\B1CC1DE5C25F0\\MSSQLSERVER");
	// .106 SQL Server
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "DAF-00-UUN\\T106");
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "DAF-00-UUN");
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "\\\\171.130.66.106\\T106");
	m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "171.130.66.106");
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "171.130.66.106:2363");
	//m_ps.AddProperty(DBPROP_INIT_CATALOG, "T106");
    //m_ps.AddProperty(DBPROP_INIT_LCID, (long)1033);
    //m_ps.AddProperty(DBPROP_INIT_PROMPT, (short)4);
	//m_ps.AddProperty(OLE DB DBPROP_AUTH_INTEGRATED, "SSPI"); for Windows NT auth
	//m_ps.AddProperty(DBPROP_INIT_PROVIDERSTRING, "Provider=sqloledb;Address=DAF-00-UUN;Server=T106;UID=SymbolsUser;PWD=symbolspass;Database=Symbols;");
	//m_ps.AddProperty(DBPROP_INIT_PROVIDERSTRING, "Provider=sqloledb;Data Source=\\\\DAF-00-UUN\\T106;Network Library=dbmssocn;Initial Catalog=Symbols;User ID=SymbolsUser;Password=symbolspass;");
	//m_ps.AddProperty(/*SSPROP_INIT_NETWORKLIBRARY*/DBPROP_INIT_PROVIDERSTRING, "Network Libarary=dbmssocn");
    //m_ps.AddProperty(DBPROP_INIT_CATALOG, "Symbols");
	//m_ps.AddProperty(DBPROP_INIT_CATALOG, "StockLoan");
	m_ps.AddProperty(DBPROP_INIT_CATALOG, "RediBus");
    HRESULT hr = m_db.Open(_T("SQLOLEDB.1"), &m_ps);
	// ODBC interface
	//m_ps.AddProperty(DBPROP_INIT_DATASOURCE, "DevSymbols");
	//HRESULT hr = m_db.OpenWithPromptFileName();
	#else
	m_ps.AddProperty(DBPROP_AUTH_USERID, strUser);
	m_ps.AddProperty(DBPROP_AUTH_PASSWORD, strPassword);
	m_ps.AddProperty(DBPROP_INIT_PROVIDERSTRING, strServerlist);
	m_ps.AddProperty(DBPROP_INIT_APPID, appId);
	HRESULT hr = m_db.OpenWithServiceComponents(_T("RediOleDB.Provider.1"), &m_ps);
	#endif
	if(FAILED(hr))
	{
		return hr;
	}
	hr = m_session.Open(m_db);
	if(FAILED(hr))
	{
		m_db.Close();
		return hr;
	}
	m_bOpen = TRUE;
	return hr;
}

void CQuerySession::Close()
{
	if (m_bOpen)
	{
		m_db.Close();
		m_session.Close();
		m_bOpen = FALSE;
	}
}

CSession &CQuerySession::GetSession()
{
	return m_session;
}

// From Query.cpp
CQuery::CQuery() 
	:m_cookie(NULL),m_Valid(false), m_eventHandler(NULL)
	,m_Done(false),m_PortType(-1),m_PortNo(-1),m_qid(0),m_queryResult(0),m_LastDataTime(0)
{
}

CQuery::~CQuery() 
{ 
	ReleaseCommand(); 
}; 

// The following buffer is used to store parameter values.
typedef struct tagSPROCPARAMS {
   long lReturnValue;
   char InsertKey[80+1];
   //long inParam;
} SPROCPARAMS;
HRESULT CQuery::Open(	CQueryChangeEventHandler *eventHandler,
	const CSession& session, LPCTSTR szCommand,
	DBPROPSET *pPropSet)
{
	ReleaseCommand();
	m_eventHandler = eventHandler;
	DBROWCOUNT pRowsAffected = 0;
#ifdef IQDEVTEST
	// Stored-procedure calls
	if(stristr(szCommand,"{rpc"))
	{
		// Create a Command object.
		HRESULT hr=-1;
		IDBCreateCommand* pIDBCreateCommand=0;
		if(FAILED(session.m_spOpenRowset->QueryInterface(IID_IDBCreateCommand,(void**)&pIDBCreateCommand)))
			return hr;
		ICommandText* pICommandText=0;
		if(FAILED(pIDBCreateCommand->CreateCommand(0,IID_ICommandText,(IUnknown**)&pICommandText))) 
			return hr;
		if(FAILED(pICommandText->SetCommandText(DBGUID_DBSQL,str2w(szCommand))))
			return hr;
		SPROCPARAMS sprocparams={{0}};
		DBPARAMS Params={0,0,0};
		ICommandWithParameters* pICommandWithParams=0;
		IAccessor* pIAccessor=0;
		HACCESSOR hAccessor=0;
		DB_UPARAMS ords[2]={1,2};
		DBPARAMBINDINFO pbis[2]={
			{L"DBTYPE_I4",L"@ReturnVal",sizeof(long),DBPARAMFLAGS_ISOUTPUT,11,0},
			{L"DBTYPE_VARCHAR",L"@InsertKey",sizeof(sprocparams.InsertKey),DBPARAMFLAGS_ISOUTPUT,0,0}
		};
		DBBINDING lvb[2]={
			{1,offsetof(SPROCPARAMS,lReturnValue),0,0,0,0,0,DBPART_VALUE,DBMEMOWNER_CLIENTOWNED,DBPARAMIO_OUTPUT,sizeof(long),0,DBTYPE_I4,11,0},
			{2,offsetof(SPROCPARAMS,InsertKey),0,0,0,0,0,DBPART_VALUE,DBMEMOWNER_CLIENTOWNED,DBPARAMIO_OUTPUT,sizeof(sprocparams.InsertKey),0,DBTYPE_STR,0,0}
		};
		DBBINDSTATUS lvbs[2]={0,0};
		// SQL parameters
		//SetParameterInfo(1,uparms,pbis);
		if(FAILED(pICommandText->QueryInterface(IID_ICommandWithParameters,(void**)&pICommandWithParams))) 
			return hr;
		if(FAILED(pICommandWithParams->SetParameterInfo(2,ords,pbis))) 
			return hr;
		// Local binding
		if(FAILED(pICommandWithParams->QueryInterface(IID_IAccessor,(void**)&pIAccessor)))
			return hr;
		if(FAILED(pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA,2,lvb,sizeof(SPROCPARAMS),&hAccessor,lvbs)))
			return hr;
		Params.pData=&sprocparams;
		Params.cParamSets=1;
		Params.hAccessor=hAccessor;
		// Execute
		DBROWCOUNT cNumRows=0;
		IRowset* pIRowset=0;
		hr=pICommandText->Execute(0,IID_IRowset,&Params,&cNumRows,(IUnknown **)&pIRowset);
		if(FAILED(hr))
			return hr;
		// This result set is not important, so release it without processing.
		if(pIRowset)
			pIRowset->Release();
		//IMultipleResults* pIMultipleResults=0;
		//hr=pICommandText->Execute(0,IID_IMultipleResults,&Params,&cNumRows,(IUnknown **)&pIMultipleResults);
		//if(FAILED(hr))
		//	return hr;
		//// For each rowset or count of rows affected...
		//do
		//{
		//	long cRowsAffected=0;
		//	hr=((IMultipleResults*)pIMultipleResults)->GetResult(0,0,IID_IRowset,&cRowsAffected,(IUnknown**)&pIRowset);
		//	switch (hr)
		//	{
		//	case S_OK:
		//	{
		//		if (pIRowset != NULL)
		//		{
		//			// Process data from the rowset and release.
		//			// TODO: Call pIRowset->GetData with accessor to get values
		//			pIRowset->Release();
		//		}
		//		else if (cRowsAffected != -1)
		//		{
		//			//printf("Command succeeded. %ld rows affected.\n\n",cRowsAffected);
		//		}
		//		else
		//		{
		//			//printf("Command succeeded.\n\n");
		//		}

		//		break;
		//	}
		//	case DB_S_NORESULT:
		//	case DB_S_STOPLIMITREACHED:
		//		break;
		//	//default:
		//	//{
		//	//	DumpError(pIResults, IID_IMultipleResults);
		//	//	break;
		//	//}
		//	};
		//}while(hr==S_OK);

		// The return value
		//sprocparams.lReturnValue;
		// The output parameter
		char wstr[256]={0};
		strcpy(wstr,sprocparams.InsertKey);
		char *wptr=strchr(wstr,'=');
		if(wptr)
		{
			wptr[1]='\''; strcat(wptr,"'");
		}
		m_InsertKey=wstr;

		// Release memory.
		if(pIAccessor)
		{
			pIAccessor->ReleaseAccessor(hAccessor,0);
			pIAccessor->Release();
		}
		if(pICommandWithParams) 
			pICommandWithParams->Release();
		pICommandText->Release();
		pIDBCreateCommand->Release();
		return hr;
	}
	// TRANSACT-SQL
	else
#endif
	{
		HRESULT hr = CCommand<CDynamicAccessorEx>::Open(session,szCommand,pPropSet,&pRowsAffected);
		if (FAILED(hr) || m_spRowset == NULL)
			return hr;
		m_Valid = TRUE;
		if (!ReadInitialSnapshot()) 
			return E_FAIL;
	#ifndef IQDEVTEST
		if (!EnableNotify())
			return E_FAIL;
	#endif
		return hr;
	}
}

BOOL CQuery::ReadInitialSnapshot()
{
	LONG size = 5000;
	DBCOUNTITEM retCnt;
	HROW prghRows[5000];
	HROW *pRows = prghRows; 

	HRESULT hr=m_spRowset->GetNextRows(NULL,0,size,&retCnt,&pRows);
#ifdef IQDEVTEST
	// SQL Server returns correct row count before GetNextRows completions
	if(!retCnt)
		m_eventHandler->OnRowChange(this,0,20,DBEVENTPHASE_DIDEVENT);
	else
	{
		while((retCnt>0)&&(IsOpen()))
		{
			for(ULONG i=0;i<retCnt;++i)
			{
				if (m_eventHandler)
					m_eventHandler->OnRowChange(this,prghRows[i],DBREASON_ROW_INSERT,DBEVENTPHASE_DIDEVENT);
			}
			if(hr==DB_S_ENDOFROWSET)
				m_eventHandler->OnRowChange(this,0,20,DBEVENTPHASE_DIDEVENT);
			m_spRowset->ReleaseRows(retCnt,prghRows,0,0,0); retCnt=0;
			if((hr==0)||(hr==DB_S_ROWLIMITEXCEEDED))
				hr = m_spRowset->GetNextRows(NULL,0,size,&retCnt,&pRows); 
		}
	}
#else
	// RediOleDB GetNextRows may return 0, but the snapshot isn't complete until DB_S_ENDOFROWSET
	while (hr == 0 && IsOpen())
	{
		if(retCnt>0)
		{
			for ( ULONG i=0; i<retCnt; ++i)
			{
				if (m_eventHandler)
					m_eventHandler->OnRowChange(this, prghRows[i], DBREASON_ROW_INSERT, DBEVENTPHASE_DIDEVENT);
			}
			m_spRowset->ReleaseRows(retCnt,prghRows,0,0,0);
		}
		else
			SleepEx(10,true);
		hr = m_spRowset->GetNextRows(NULL, 0, size, &retCnt, &pRows); 
		_ASSERT((hr!=DB_S_ENDOFROWSET)||(retCnt<1));
	}
#endif
	return TRUE;
}

void CQuery::ReleaseCommand()
{
	m_Valid=false;
	m_eventHandler = NULL;
	DisableNotify();
	if (m_spCommand != NULL) 
	{
		m_spCommand->Cancel();
	}
	CCommand<CDynamicAccessorEx>::ReleaseCommand();
	Close();
}

BOOL CQuery::EnableNotify()
{
	DisableNotify();
	if (!IsOpen() || !m_spRowset)
		return FALSE;
	m_cookie = -1;
	if (!SUCCEEDED(AtlAdvise(m_spRowset, this, IID_IRowsetNotify, &m_cookie)))
		return FALSE;
	return TRUE;
}

void CQuery::DisableNotify()
{
	DWORD tmp = m_cookie;
	CComPtr<IRowset> itmp;
	if (m_cookie && m_spRowset)
	{
		m_cookie = 0;
		itmp = m_spRowset;
	}
	if (tmp && itmp)
		AtlUnadvise(itmp, IID_IRowsetNotify, tmp);
}

STDMETHODIMP CQuery::OnRowChange(IRowset* pRowset, ULONG cRows, const HROW rghRows[], DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny)
{
	HRESULT lRes = S_OK;

	for ( ULONG i=0; i<cRows; ++i)
	{
		if (IsOpen() && m_eventHandler)
			m_eventHandler->OnRowChange(this, rghRows[i], eReason, ePhase);
		else
			lRes=DB_S_UNWANTEDPHASE;
	}
	m_LastDataTime=GetTickCount();
	pRowset->ReleaseRows(cRows,rghRows,0,0,0);
	return lRes;
}

HRESULT CQuery::ActivateRow(HROW hRow)
{
	m_hRow = hRow; 
	if (!hRow) 
		return S_OK;
	HRESULT res = GetData(0); 
	if (FAILED(res)) 
		m_hRow = NULL; 
	return res;
}

// The connection can take over a minute, so run it in a separate thread
enum RediConnectThreadStatus
{
	RCTS_UNKNOWN,
	RCTS_CONNECTING,
	RCTS_CONNECTED,
	RCTS_DISCONNECTING,
	RCTS_DISCONNECTED,
	RCTS_COUNT
};
class RediConnectThreadData
{
public:
	RediConnectThreadData()
		:pmod(0)
		,ConPortNo(-1)
		,cqs(0)
		,tstart(0)
		,status(RCTS_UNKNOWN)
		,nqid(0)
		,qlist()
	{
	}

	ViewServer *pmod;
	int ConPortNo;
	CQuerySession *cqs;
	DWORD tstart;
	RediConnectThreadStatus status;
	DWORD nqid;
	list<CQuery*> qlist;				// FIFO queue of active queries on this connection
	list<CQueryResult *> rlist;			// FIFO queue of query results
};
static DWORD WINAPI _BootRediConnectThread(LPVOID arg)
{
	RediConnectThreadData *ptd=(RediConnectThreadData *)arg;
	int rc=ptd->pmod->RediConnectThread(ptd);
	delete ptd;
	return rc;
}
// Called from WSTimeChange
int ViewServer::RediTimeChange()
{
	if(!runNowTaskList.empty())
	{
		for(list<SCHEDENTRY*>::iterator rit=runNowTaskList.begin();rit!=runNowTaskList.end();rit++)
			taskmgr.RunScheduledTaskNow(*rit,WSDate,WSTime);
	}
	taskmgr.CheckSchedule(WSDate,WSTime);
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(ConPort[i].DetPtr!=(void*)PROTO_REDIOLEDB)
			continue;
		// Number of seconds waiting
		RediConnectThreadData *ptd=(RediConnectThreadData *)ConPort[i].ShellDetPtr;
		if(ptd)
		{
			DWORD msdiff=GetTickCount() -ptd->tstart;
			if(ptd->status==RCTS_CONNECTING)
				sprintf(ConPort[i].OnLineStatusText,"CONNECTING %d sec",(msdiff/1000));
			else if(ptd->status==RCTS_CONNECTED)
			{
				// Heartbeat select from 1-row table
				#ifndef IQDEVTEST
				if(REDIBUS_HEARTBEAT>0)
				{
					if(WSTime%REDIBUS_HEARTBEAT==0)
					{
						int qid=VSRediQuery(i,"select * from SYS_SERVERS",-1,-1,0,(void*)0x01);
						if(qid>0)
						{
							if(VSWaitRediQuery(i,1000,qid,false)<0)
							{
								if(REDIBUS_BOUNCE_MISSED_HBS)
								{
									WSLogError("CON%d: Failed heartbeat--bouncing port!",i);
									WSCloseConPort(i);
								}
								else
									WSLogError("CON%d: Failed heartbeat!",i);
							}
						}

						int tqid=VSRediQuery(i,"select * from TIME",-1,-1,0,(void*)0x02);
						if(tqid>0)
						{
							ConPort[i].BeatsOut++;
							if(VSWaitRediQuery(i,1000,tqid,false)<0)
							{
								if(REDIBUS_BOUNCE_MISSED_HBS)
								{
									WSLogError("CON%d: Failed heartbeat--bouncing port!",i);
									WSCloseConPort(i);
								}
								else
									WSLogError("CON%d: Failed heartbeat!",i);
							}
						}
					}
				}
				// Fake heartbeat
				else
				#endif
				{
					//sprintf(ConPort[i].OnLineStatusText,"CONNECTED %d sec",(msdiff/1000));
					ConPort[i].BeatsIn++; ConPort[i].BeatsOut++;
				}
			}
			else if(ptd->status==RCTS_DISCONNECTING)
				sprintf(ConPort[i].OnLineStatusText,"DISCONNECTING %d sec",(msdiff/1000));
		}
	}
	return 0;
}
int ViewServer::RediConnectThread(RediConnectThreadData *ptd)
{
	int ConPortNo=ptd->ConPortNo;
	if((ConPortNo<0)||(ConPortNo>=NO_OF_CON_PORTS))
		return -1;
	ConPort[ConPortNo].ShellDetPtr=ptd;
	::CoInitialize(0);
	CQuerySession *cqs=ptd->cqs=new CQuerySession;
	char url[1024]={0};
	// Lookup port in CON_URL_LIST first
	for(list<pair<int,string>>::iterator cit=CON_URL_LIST.begin();cit!=CON_URL_LIST.end();cit++)
	{
		if(cit->first==ConPortNo)
		{
			sprintf_s(url,"URL=%s",cit->second.c_str());
			break;
		}
	}
	// Alternative ip is already selected by wsocksapi code
	if(!url[0])
	{
		sprintf_s(url,sizeof(url)-2,"URL=secure:%s[%d]",ConPort[ConPortNo].RemoteIP,ConPort[ConPortNo].Port);
		for(int i=0;i<ConPort[ConPortNo].AltIPCount;i++)
		{
			if(i==ConPort[ConPortNo].CurrentAltIP)
				continue;
			if(!ConPort[ConPortNo].AltRemoteIPOn[i])
				continue;
			sprintf_s(url+strlen(url),sizeof(url)-2-strlen(url),",secure:%s[%d]",
				ConPort[ConPortNo].AltRemoteIP[i],ConPort[ConPortNo].AltRemotePort?ConPort[ConPortNo].AltRemotePort[i]:ConPort[ConPortNo].Port);
		}
		strcat(url,";");
	}
	strcpy(ConPort[ConPortNo].OnLineStatusText,"CONNECTING");
	WSLogEvent("CON%d connecting to %s...",ConPortNo,url);
	ptd->status=RCTS_CONNECTING;
	ptd->tstart=GetTickCount();
	// Make OleDB connection then call WSOpenConPort when successfull
	int tstart=WSTime;
	HRESULT res=cqs->Open(REDIBUS_USER.c_str(),REDIBUS_PASS.c_str(),"SDK",url);
	int tstop=WSTime;
	// Copy all RediOleDB connection info
	ImportRediOleDBLog(WSDate,tstart,tstop);
	if(FAILED(res))
	{
		WSLogError("CON%d: FAILED CONNECT at %06d",ConPortNo,WSTime);
		sprintf(ConPort[ConPortNo].OnLineStatusText,"FAILED CONNECT at %06d",WSTime);
		delete cqs; ptd->cqs=0; ConPort[ConPortNo].ShellDetPtr=0;
		ptd->status=RCTS_DISCONNECTED;		
		return -1;
	}
	// Aborted by user
	if(ptd->status==RCTS_DISCONNECTING)
	{
		WSLogError("CON%d: CONNECT ABORTED at %06d",ConPortNo,WSTime);
		sprintf(ConPort[ConPortNo].OnLineStatusText,"CONNECT ABORTED at %06d",WSTime);
		delete cqs; ptd->cqs=0; ConPort[ConPortNo].ShellDetPtr=0;
		ptd->status=RCTS_DISCONNECTED;		
		return -1;
	}
	strcpy(ConPort[ConPortNo].OnLineStatusText,"CONNECTED");
	ptd->status=RCTS_CONNECTED;
	ptd->tstart=GetTickCount();
	ConPort[ConPortNo].SockConnected=true;
	WSConOpened(ConPortNo);

	// Wait till port needs to be closed
	while(ptd->status==RCTS_CONNECTED)
	{
		EnterCriticalSection(&rediOleMux);
		// Look for unissued queries from the result list
		for(list<CQueryResult*>::iterator rit=ptd->rlist.begin();rit!=ptd->rlist.end();rit++)
		{
			CQueryResult *res=*rit;
			if(res->m_queryStatus!=RQS_UNISSUED)
				continue;
			const char *command=res->m_select.c_str();
			strncpy(ConPort[ConPortNo].OnLineStatusText,command,sizeof(ConPort[ConPortNo].Status)-1);
			ConPort[ConPortNo].Status[sizeof(ConPort[ConPortNo].Status)-1]=0;

			CComPtr<CQuery> qry;
			CComObject<CQuery> *pob=NULL;
			if(FAILED(CComObject<CQuery>::CreateInstance(&pob)))
			{
				WSLogEvent("Failed creating CQuery for (%s)",command);
				continue;
			}
			qry=pob;
			qry->m_PortType=WS_CON;
			qry->m_PortNo=ConPortNo;
			qry->m_qid=res->m_qid;
			qry->m_queryResult=res;
			qry->m_LastDataTime=GetTickCount();
			res->m_queryStatus=RQS_INPROGRESS;
			res->m_tstart=GetTickCount();		
			res->m_query=qry;
			#ifdef _DEBUG
			WSLogEvent("DEBUG %08X:%s",res->m_udata,command);
			#endif
			// If qry->Open fails, ~CComPtr<CQuery> dereference counting will destroy the object at end of this while loop
			ConPort[ConPortNo].PacketsOut++;
			if(qry->Open(this,cqs->GetSession(),command,0)<0)
			{
				char cbuf[sizeof(ConPort[ConPortNo].OnLineStatusText)]={0};
				strncpy(cbuf,command,sizeof(cbuf)-8); cbuf[sizeof(cbuf)-1]=0;
				sprintf(ConPort[ConPortNo].OnLineStatusText,"Failed %s",cbuf);
				#ifdef _DEBUG
				WSLogEvent("DEBUG %08X:Failed %s",res->m_udata,command);
				#endif
				res->m_queryStatus=RQS_FAILED;
				res->m_query=0; qry->m_queryResult=0; qry=0;
			}
			else
			{
				#ifdef IQDEVTEST
				if(!strncmp(command,"{rpc ",5))
				{
					//res->m_queryStatus=RQS_INPROGRESS;
					//qry->InternalAddRef();
					//ptd->qlist.push_back(qry);
					char dtable[256]={0};
					if(stristr(command,"spsltran_trans"))
						strcpy(dtable,"SLTRAN_DETAIL");
					else
					{
						strcpy(dtable,command +7);
						strupr(dtable);
						char *dptr=strstr(dtable,"_TRANS");
						if(dptr) *dptr=0;
					}
					if(dtable[0])
					{
						FeedHint fhint={0,0,0,-1,0,PROTO_REDIOLEDB};
						taskmgr.CallTasks(dtable,TASK_REDIBUS,(void*)qry->m_InsertKey.c_str());
					}
					res->m_queryStatus=RQS_COMPLETED;
					res->m_query=0; qry->m_queryResult=0; qry=0;
				}
				else
				{
					res->m_queryStatus=RQS_COMPLETED;
					res->m_query=0; qry->m_queryResult=0; qry=0;
				}
				#else
				ptd->qlist.push_back(qry);
				#endif
			}
		}

		// Check status on queries
		for(list<CQuery*>::iterator qit=ptd->qlist.begin();qit!=ptd->qlist.end();qit++)
		{
			CQuery *qry=*qit;
			CQueryResult *res=qry->m_queryResult;
			if((!res)||(res->m_queryStatus!=RQS_INPROGRESS))
				continue;
			if(stristr(res->m_select.c_str(),"INSERT INTO"))
			{
				ConPort[ConPortNo].PacketsIn+=res->m_rows;
				#ifdef _DEBUG
				WSLogEvent("DEBUG %08X:%s",res->m_udata,ConPort[ConPortNo].OnLineStatusText);
				#endif
				sprintf(ConPort[ConPortNo].OnLineStatusText,"%d results",res->m_rows);
				res->m_queryStatus=RQS_COMPLETED;
				continue;
			}
			bool qdone=false;
			if(res->m_idleTimeout>0)
				qdone=qry->IsIdle(res->m_idleTimeout);
			else
				qdone=qry->IsDone();
			if(qdone)
			{
				ConPort[ConPortNo].PacketsIn+=res->m_rows;
				//sprintf(ConPort[ConPortNo].OnLineStatusText,"%d results",res->m_rows);
				#ifdef _DEBUG
				WSLogEvent("DEBUG %08X:%s",res->m_udata,ConPort[ConPortNo].OnLineStatusText);
				#endif
				res->m_queryStatus=RQS_COMPLETED;
				continue;
			}
			if(ptd->status!=RCTS_CONNECTED)
			{
				ConPort[ConPortNo].PacketsIn+=res->m_rows;
				sprintf(ConPort[ConPortNo].OnLineStatusText,"Aborted after %d results",res->m_rows);
				#ifdef _DEBUG
				WSLogEvent("DEBUG %08X:%s",res->m_udata,ConPort[ConPortNo].OnLineStatusText);
				#endif
				res->m_queryStatus=RQS_ABORTED;
				continue;
			}
			// No idle timeout for queries that need live updates
			//// 10 seconds since last result for the query??
			//if(GetTickCount() -qry->m_tlast>=10000)
			//{
			//	ConPort[ConPortNo].PacketsIn+=qry->m_rows;
			//	sprintf(ConPort[ConPortNo].OnLineStatusText,"Timeout after idle %d ms, received %d results",10000,qry->m_rows);
			//	#ifdef _DEBUG
			//	WSLogEvent("DEBUG %08X:%s",qry->m_udata,ConPort[ConPortNo].OnLineStatusText);
			//	#endif
			//	qry->m_queryStatus=RQS_ABORTED;
			//	continue;
			//}

			// Release the critical section so the app can timeout
			LeaveCriticalSection(&rediOleMux);
			SleepEx(100,true);
			EnterCriticalSection(&rediOleMux);
		}
		LeaveCriticalSection(&rediOleMux);
		SleepEx(100,true);

		// Remove completed queries
		EnterCriticalSection(&rediOleMux);
		for(list<CQuery*>::iterator qit=ptd->qlist.begin();qit!=ptd->qlist.end();)
		{
			CQuery *qry=*qit;
			CQueryResult *res=qry->m_queryResult;
			if((res)&&(res->m_queryStatus<=RQS_INPROGRESS))
			{
				qit++; continue;
			}
			qit=ptd->qlist.erase(qit);
			// Unhook the query object from the result
			if(qry->m_queryResult)
			{
				qry->m_queryResult->m_query=0; qry->m_queryResult=0;
			}
			qry->Close();
		}
		LeaveCriticalSection(&rediOleMux);
	}
	if(ptd->status==RCTS_DISCONNECTING)
		ptd->status=RCTS_DISCONNECTED;
	cqs->Close(); delete cqs;

	// Cleanup for outstanding queries
	EnterCriticalSection(&rediOleMux);
	for(list<CQueryResult*>::iterator rit=ptd->rlist.begin();rit!=ptd->rlist.end();rit++)
	{
		CQueryResult *res=*rit;
		res->m_queryStatus=RQS_ABORTED;
	}
	// Give app threads a chance to receive the abort notification
	LeaveCriticalSection(&rediOleMux);
	SleepEx(15,true);
	EnterCriticalSection(&rediOleMux);
	for(list<CQuery*>::iterator qit=ptd->qlist.begin();qit!=ptd->qlist.end();qit++)
	{
		CQuery *qry=*qit;
		if(qry->m_queryResult)
		{
			qry->m_queryResult->m_query=0; qry->m_queryResult=0;
		}
		qry->Close();
	}
	ptd->qlist.clear();
	for(list<CQueryResult*>::iterator rit=ptd->rlist.begin();rit!=ptd->rlist.end();rit++)
	{
		CQueryResult *res=*rit;
		if(res->m_query)
		{
			res->m_query->m_queryResult=0; res->m_query=0;
		}
		delete res;
	}
	ptd->rlist.clear();
	LeaveCriticalSection(&rediOleMux);
	::CoUninitialize();
	return 0;
}
void ViewServer::WSConConnecting(int PortNo)
{
	if(!strncmp(ConPort[PortNo].CfgNote,"$REDIOLEDB$",11))
	{
		for(int i=0;i<WS_MAX_ALT_PORTS;i++)
		{
			if(++ConPort[PortNo].CurrentAltIP>=WS_MAX_ALT_PORTS)
				ConPort[PortNo].CurrentAltIP=0;
			if(ConPort[PortNo].AltRemoteIPOn[ConPort[PortNo].CurrentAltIP])
			{
				strcpy(ConPort[PortNo].RemoteIP,ConPort[PortNo].AltRemoteIP[ConPort[PortNo].CurrentAltIP]);
				if(ConPort[PortNo].AltRemotePort[ConPort[PortNo].CurrentAltIP])
					ConPort[PortNo].Port=ConPort[PortNo].AltRemotePort[ConPort[PortNo].CurrentAltIP];
				break;
			}
		}

		//WSLogError("CON%d connecting to %s:%d...",PortNo,ConPort[PortNo].RemoteIP,ConPort[PortNo].Port);
		strcpy(ConPort[PortNo].Note,ConPort[PortNo].CfgNote);
		ConPort[PortNo].DetPtr=(void*)PROTO_REDIOLEDB;
		RediConnectThreadData *ptd=new RediConnectThreadData;
		ptd->pmod=this;
		ptd->ConPortNo=PortNo;
		ptd->cqs=0;
		ptd->tstart=0;
		ptd->status=RCTS_UNKNOWN;
		DWORD tid=0;
		HANDLE thnd=CreateThread(0,0,_BootRediConnectThread,ptd,0,&tid);
		if(thnd!=INVALID_HANDLE_VALUE)
			CloseHandle(thnd);
	}
}
void ViewServer::WSConClosing(int PortNo)
{
	if(ConPort[PortNo].DetPtr==(void*)PROTO_REDIOLEDB)
	{
		// Wait for the OLEDB session to close
		strcpy(ConPort[PortNo].OnLineStatusText,"DISCONNECTING");
		RediConnectThreadData *ptd=(RediConnectThreadData *)ConPort[PortNo].ShellDetPtr;
		if(!ptd)
			return;
		int hold=ConPort[PortNo].ConnectHold; ConPort[PortNo].ConnectHold=false;
		ptd->tstart=GetTickCount();
		ptd->status=RCTS_DISCONNECTING;
		DWORD tclast=GetTickCount();
		while(ptd->status==RCTS_DISCONNECTING)
		{
			SleepEx(100,true);
			DWORD tnow=GetTickCount();
			if(tnow -ptd->tstart>=10000)
				break;
			else if(tnow -tclast>=1000)
			{
				WSBusyThreadReport();
				RediTimeChange(); tclast=tnow;
			}
		}
		ConPort[PortNo].ConnectHold=hold;
		strcpy(ConPort[PortNo].OnLineStatusText,"DISCONNECTED");
	}
}
// The query needs to be executed by the thread that opened the session, so post the query to that thread. 
// The caller will get VSOleDBMsgReady called back.
int ViewServer::VSRediQuery(int ConPortNo, const char *select, int PortType, int PortNo, FILE *fp, void *udata)
{
	if((ConPortNo<0)||(ConPortNo>=NO_OF_CON_PORTS))
		return -1;
	RediConnectThreadData *ptd=(RediConnectThreadData *)ConPort[ConPortNo].ShellDetPtr;
	// Not connected
	if((!ptd)||(ptd->status!=RCTS_CONNECTED))
		return -1;
	EnterCriticalSection(&rediOleMux);
	CQueryResult *res=new CQueryResult;
	if((++ptd->nqid)<0)
		ptd->nqid=1;
	res->m_qid=ptd->nqid;
	res->m_select=select;
	res->m_porttype=PortType;
	res->m_portno=PortNo;
	res->m_fp=fp;
	res->m_udata=udata;
	if((strstr(select," RTSYM"))||(strstr(select," RTPOSITION")))
		res->m_idleTimeout=30;
	ptd->rlist.push_back(res);
	LeaveCriticalSection(&rediOleMux);
	return res->m_qid;
}
// Wait on VSRediQuery with timeout in seconds
int ViewServer::VSWaitRediQuery(int ConPortNo, int timeout, int qid, bool callTimeChange)
{
	if((ConPortNo<0)||(ConPortNo>=NO_OF_CON_PORTS))
		return -1;
	// Not connected
	RediConnectThreadData *ptd=(RediConnectThreadData *)ConPort[ConPortNo].ShellDetPtr;
	if((!ptd)||(ptd->status!=RCTS_CONNECTED))
		return -1;
	CQueryResult *res=0;
	EnterCriticalSection(&rediOleMux);
	for(list<CQueryResult*>::iterator rit=ptd->rlist.begin();rit!=ptd->rlist.end();rit++)
	{
		CQueryResult* sres=*rit;
		if(sres->m_qid==qid)
		{
			res=sres;
			break;
		}
	}
	// Not found
	if(!res)
	{
		LeaveCriticalSection(&rediOleMux);
		return -1;
	}
	// Query found
	// Wait for query to start and finish
	DWORD tstart=GetTickCount(),tclast=tstart;
	while(res->m_queryStatus<=RQS_INPROGRESS)
	{
		// Timeout
		if(GetTickCount() -tstart>=(DWORD)(timeout*1000))
		{
			res->m_queryStatus=RQS_TIMEOUT;
			break;
		}

		LeaveCriticalSection(&rediOleMux);
		DWORD tnow=GetTickCount();
		if(tnow -tclast>=1000)
		{
			WSBusyThreadReport();
			if(callTimeChange)
				RediTimeChange(); 
			tclast=tnow;
		}
		SleepEx(100,true);
		EnterCriticalSection(&rediOleMux);
	}
	// Query done; remove completed query results
	// We have to look for the result again because we left the critical section while waiting
	for(list<CQueryResult*>::iterator rit=ptd->rlist.begin();rit!=ptd->rlist.end();rit++)
	{
		CQueryResult *sres=*rit;
		if(sres==res)
		{
			ptd->rlist.erase(rit);
			break;
		}
	}
	int rc=-1;
	if(res->m_queryStatus==RQS_TIMEOUT)
		rc=0;
	else if(res->m_queryStatus==RQS_COMPLETED)
		rc=1;
	// Unhook the result from the query object
	if(res->m_query)
	{
		res->m_query->m_queryResult=0; res->m_query=0;
	}
	delete res;
	LeaveCriticalSection(&rediOleMux);
	return rc;
}
int ViewServer::VSOleDBMsgReady(int ConPortNo, char *Msg, int MsgLen, int PortType, int PortNo, FILE *fp, void *udata)
{
	int UsrPortNo=(PortType==WS_USR) ?PortNo :-1;
	if(MsgLen>0)
	{
		if((ConPortNo>=0)&&(ConPortNo<NO_OF_CON_PORTS))
		{
			ConPort[ConPortNo].PacketsIn++;
			ConPort[ConPortNo].BytesIn+=MsgLen;
			// select * from SYS_SERVERS
			if(udata==(void*)0x01)
			{
				const char *row="",*server="",*status="";
				int col=0;
				for(const char *tok=strtoke(Msg,",");tok;tok=strtoke(0,","))
				{
					switch(++col)
					{
					case 1: row=tok; break;
					case 4: server=tok; break;
					case 5: status=tok; break;
					};
					if(col>=5) break;
				}
				if(stricmp(row,"Row"))
				{
					REDIHEARTBEAT *redihb=(REDIHEARTBEAT*)ConPort[ConPortNo].DetPtr3;
					if(!redihb)
					{
						redihb=new REDIHEARTBEAT;
						memset(redihb,0,sizeof(REDIHEARTBEAT));
						ConPort[ConPortNo].DetPtr3=redihb;
					}
					strcpy(redihb->server,server);
					char *rptr=strchr(redihb->server,'[');
					if(rptr) 
					{
						*rptr=0; redihb->serverPort=atoi(rptr +1);
					}
					strcpy(redihb->status,status);

					if(_stricmp(redihb->status,"Connected"))
					{
						redihb->disconCount++;
						if(redihb->disconCount>=3)
						{
							if(REDIBUS_BOUNCE_MISSED_HBS)
							{
								WSLogError("CON%d: %d disconnect statuses--bouncing port!",ConPortNo,redihb->disconCount);
								WSCloseConPort(ConPortNo);
							}
							else
								WSLogError("CON%d: %d disconnect statuses!",ConPortNo,redihb->disconCount);
							redihb->disconCount=0;
						}
					}
					else
						redihb->disconCount=0;

					strcpy(ConPort[ConPortNo].RemoteIP,redihb->server);
					ConPort[ConPortNo].Port=redihb->serverPort;
					strcpy(ConPort[ConPortNo].OnLineStatusText,redihb->status);
				}
			}
			// select * from TIME
			else if(udata==(void*)0x02)
			{
				const char *row="",*timekey="",*curtime="";
				int col=0;
				for(const char *tok=strtoke(Msg,",");tok;tok=strtoke(0,","))
				{
					switch(++col)
					{
					case 1: row=tok; break;
					case 4: timekey=tok; break;
					case 5: curtime=tok; break;
					};
					if(col>=5) break;
				}
				if(stricmp(row,"Row"))
				{
					ConPort[ConPortNo].BeatsIn++;
					REDIHEARTBEAT *redihb=(REDIHEARTBEAT*)ConPort[ConPortNo].DetPtr3;
					if(!redihb)
					{
						redihb=new REDIHEARTBEAT;
						memset(redihb,0,sizeof(REDIHEARTBEAT));
						ConPort[ConPortNo].DetPtr3=redihb;
					}
					if(!strcmp(redihb->serverTime,curtime))
					{
						redihb->staleTimeCount++;
						sprintf(ConPort[ConPortNo].OnLineStatusText,"%s (%s %s) STALE!!",redihb->status,redihb->serverTime,redihb->serverTimeZone);
						if(redihb->staleTimeCount>=3)
						{
							if(REDIBUS_BOUNCE_MISSED_HBS)
							{
								WSLogError("CON%d: %d stale heartbeats--bouncing port!",ConPortNo,redihb->staleTimeCount);
								WSCloseConPort(ConPortNo);
							}
							else
								WSLogError("CON%d: %d stale heartbeats!",ConPortNo,redihb->staleTimeCount);
							redihb->staleTimeCount=0;
						}
					}
					else
					{
						strcpy(redihb->serverTime,curtime);
						strcpy(redihb->serverTimeZone,timekey);
						sprintf(ConPort[ConPortNo].OnLineStatusText,"%s (%s %s)",redihb->status,redihb->serverTime,redihb->serverTimeZone);
						redihb->staleTimeCount=0;
					}
				}
			}
		}

		if(fp)
			fprintf(fp,"%s\n",Msg);
		else if((UsrPortNo>=0)&&(UsrPortNo<NO_OF_USR_PORTS))
		{
			char rbuf[2048]={0},*rptr=rbuf;
			int qid=(int)LOWORD(udata);
			rptr=rbuf; memcpy(rptr,&qid,4); rptr+=4;
			*rptr=0; rptr++;
			strncpy(rptr,Msg,MsgLen); rptr+=strlen(rptr);
			WSUsrSendMsg(102,(int)(rptr -rbuf),rbuf,UsrPortNo);
		}
		#ifdef _DEBUG
		else if(PortType>=0)
			WSLogEvent("DEBUG unsent %08X:%s",udata,Msg);
		#endif
	}
	// Done
	else
	{
		if((UsrPortNo>=0)&&(UsrPortNo<NO_OF_USR_PORTS))
		{
			char rbuf[8]={0},*rptr=rbuf;
			int qid=(int)LOWORD(udata);
			rptr=rbuf; memcpy(rptr,&qid,4); rptr+=4;
			*rptr=1; rptr++;
			WSUsrSendMsg(102,(int)(rptr -rbuf),rbuf,UsrPortNo);
		}
	}
	return 0;
}

// From QueryClient.cpp
template <class TYPE>
static void ConvertDBValue(TYPE& retval, LPVOID pVal)
{
	_ASSERT(pVal);
	retval = *(const TYPE*)pVal;
}

static CString GetDBValueHelper(DBTYPE dbType, LPVOID pVal)
{
	//strCol must be uppercase
	CString result;
	if (!pVal)
		return result;

	switch (dbType)
	{
	case DBTYPE_I1:
		{
			TCHAR ch;
			ConvertDBValue(ch, pVal);
			result = ch;
		}
		break;
	case DBTYPE_I2:
		{
			SHORT s;
			ConvertDBValue(s, pVal);
			long val = s;
			result.Format(_T("%d"),val);
		}
		break;
	case DBTYPE_I4:
		{
			LONG n;
			ConvertDBValue(n, pVal);
			result.Format(_T("%d"),n);
		}
		break;
	case DBTYPE_UI1:
		{
			BYTE b;
			ConvertDBValue(b, pVal);
			long val = b;
			result.Format(_T("%d"),val);
		}
		break;
	case DBTYPE_UI2:
		{
			USHORT us;
			ConvertDBValue(us, pVal);
			long val = us;
			result.Format(_T("%d"),val);
		}
		break;
	case DBTYPE_UI4:
		{
			ULONG ul;
			ConvertDBValue(ul, pVal);
			long val = ul;
			result.Format(_T("%d"),val);
		}
		break;
	case DBTYPE_R4:
		{
			float ch;
			ConvertDBValue(ch, pVal);
			double val = ch;
			result.Format(_T("%f"),val);
		}
		break;
	case DBTYPE_R8:
		{
			double psz;
			ConvertDBValue(psz, pVal);
			double val = psz;
			result.Format(_T("%f"),val);
		}
		break;
	case DBTYPE_BSTR:
	case DBTYPE_WSTR:
		{
			OLECHAR* psz = (OLECHAR*)pVal;
			CString str(psz);
			result = str;
		}
		break;
	case DBTYPE_BSTR|DBTYPE_BYREF: 
	case DBTYPE_WSTR|DBTYPE_BYREF: 
		{
			OLECHAR* psz = *((OLECHAR**)pVal);
			CString str(psz);
			result = str;
		}
		break;
	case DBTYPE_STR|DBTYPE_BYREF: 
		{  
			//USES_CONVERSION; needed for UNICODE compile
			LPCSTR psz = *((LPCSTR*)pVal);
			//result = A2T(psz);
			result = psz;
		}
		break;
	case DBTYPE_STR:
		{
			//USES_CONVERSION; needed for UNICODE compile
			LPCSTR psz = (LPCSTR)pVal;
			//result = A2T(psz);
			result = psz;
		}
		break;
	case DBTYPE_DATE:
		{
			COleDateTime dt(*((DATE*) pVal));
			if(dt.GetYear()>1899)
				result = dt.Format(_T("%c"));
		}
		break;
	default:
		return result;
	}
	return result;
}
#ifdef IQDEVTEST
static void PrintRow(char *rbuf, int rsize, LPCTSTR prefix, CQuery *pSender, HROW hRow)
{
	char *rptr=rbuf;
	if(hRow==-1)
	{
		DBORDINAL colCnt = pSender->GetColumnCount();
		sprintf(rptr,"Row,Action,"); rptr+=strlen(rptr);
		for (DBORDINAL i=1; i<=colCnt; ++i)
		{
			LPOLESTR ostr = pSender->GetColumnName(i);
			CString colName = ostr ?w2str((const WCHAR *)ostr) :"";
			//sprintf(rptr,"%S,",colName); rptr+=strlen(rptr);
			sprintf(rptr,"%s,",(const char*)colName); rptr+=strlen(rptr);
		}
		//sprintf(rptr,"\n"); rptr+=strlen(rptr);
		*rptr=0;
	}
	else
	{
		pSender->ActivateRow(hRow);
		DBORDINAL colCnt = pSender->GetColumnCount();
		//sprintf(rptr,"%d,%S,",++pSender->m_rows,prefix); rptr+=strlen(rptr);
		CQueryResult *res=pSender->m_queryResult;
		sprintf(rptr,"%d,%s,",++res->m_rows,prefix); rptr+=strlen(rptr);
		for (DBORDINAL i=1; i<=colCnt; ++i)
		{
			LPOLESTR ostr = pSender->GetColumnName(i);
			CString colName = ostr ?w2str((const WCHAR *)ostr) :"";
			DBTYPE dtype = DBTYPE_EMPTY;
			pSender->GetColumnType(i, &dtype);
			void*pData = pSender->GetValue(colName);
			CString colValue = GetDBValueHelper(dtype, pData);
			if(colValue.Find(_T(","))>=0)
				//sprintf(rptr,"\"%S\",",colValue);
				sprintf(rptr,"\"%s\",",(const char*)colValue);
			else
				//sprintf(rptr,"%S,",colValue);
				sprintf(rptr,"%s,",(const char*)colValue);
			rptr+=strlen(rptr);
		}
		//sprintf(rptr,"\n"); rptr+=strlen(rptr);
		*rptr=0;
	}
}
#else
static void PrintRow(char *rbuf, int rsize, LPCTSTR prefix, CQuery *pSender, HROW hRow)
{
	pSender->ActivateRow(hRow);
	char *rptr=rbuf;
	if(hRow==-1)
	{
		DBORDINAL colCnt = pSender->GetColumnCount();
		sprintf(rptr,"Row,Action,"); rptr+=strlen(rptr);
		for (DBORDINAL i=0; i< colCnt; ++i)
		{
			CString colName = pSender->GetColumnName(i);
			//sprintf(rptr,"%S,",colName); rptr+=strlen(rptr);
			sprintf(rptr,"%s,",(const char*)colName); rptr+=strlen(rptr);
		}
		//sprintf(rptr,"\n"); rptr+=strlen(rptr);
		*rptr=0;
	}
	else
	{
		DBORDINAL colCnt = pSender->GetColumnCount();
		//sprintf(rptr,"%d,%S,",++pSender->m_rows,prefix); rptr+=strlen(rptr);
		CQueryResult *res=pSender->m_queryResult;
		sprintf(rptr,"%d,%s,",++res->m_rows,prefix); rptr+=strlen(rptr);
		for (DBORDINAL i=0; i< colCnt; ++i)
		{
			CString colName = pSender->GetColumnName(i);
			DBTYPE dtype = DBTYPE_EMPTY;
			pSender->GetColumnType(i, &dtype);
			void*pData = pSender->GetValue(colName);
			CString colValue = GetDBValueHelper(dtype, pData);
			if(colValue.Find(_T(","))>=0)
				//sprintf(rptr,"\"%S\",",colValue);
				sprintf(rptr,"\"%s\",",(const char*)colValue);
			else
				//sprintf(rptr,"%S,",colValue);
				sprintf(rptr,"%s,",(const char*)colValue);
			rptr+=strlen(rptr);
		}
		//sprintf(rptr,"\n"); rptr+=strlen(rptr);
		*rptr=0;
	}
}
#endif
void ViewServer::OnRowChange(CQuery *pSender, HROW hRow, DBREASON eReason, DBEVENTPHASE ePhase)
{
#ifdef _DEBUG
	char drbuf[32]={0};
	const char *rstr="";
	switch(eReason)
	{
	case DBREASON_ROWSET_FETCHPOSITIONCHANGE: rstr="DBREASON_ROWSET_FETCHPOSITIONCHANGE"; break;
	case DBREASON_ROWSET_RELEASE: rstr="DBREASON_ROWSET_RELEASE"; break;
	case DBREASON_COLUMN_SET: rstr="DBREASON_COLUMN_SET"; break;
	case DBREASON_COLUMN_RECALCULATED: rstr="DBREASON_COLUMN_RECALCULATED"; break;
	case DBREASON_ROW_ACTIVATE: rstr="DBREASON_ROW_ACTIVATE"; break;
	case DBREASON_ROW_RELEASE: rstr="DBREASON_ROW_RELEASE"; break;
	case DBREASON_ROW_DELETE: rstr="DBREASON_ROW_DELETE"; break;
	case DBREASON_ROW_FIRSTCHANGE: rstr="DBREASON_ROW_FIRSTCHANGE"; break;
	case DBREASON_ROW_INSERT: rstr="DBREASON_ROW_INSERT"; break;
	case DBREASON_ROW_RESYNCH: rstr="DBREASON_ROW_RESYNCH"; break;
	case DBREASON_ROW_UNDOCHANGE: rstr="DBREASON_ROW_UNDOCHANGE"; break;
	case DBREASON_ROW_UNDOINSERT: rstr="DBREASON_ROW_UNDOINSERT"; break;
	case DBREASON_ROW_UNDODELETE: rstr="DBREASON_ROW_UNDODELETE"; break;
	case DBREASON_ROW_UPDATE: rstr="DBREASON_ROW_UPDATE"; break;
	case DBREASON_ROWSET_CHANGED: rstr="DBREASON_ROWSET_CHANGED"; break;
	case DBREASON_ROWPOSITION_CHANGED: rstr="DBREASON_ROWPOSITION_CHANGED"; break;
	case DBREASON_ROWPOSITION_CHAPTERCHANGED: rstr="DBREASON_ROWPOSITION_CHAPTERCHANGED"; break;
	case DBREASON_ROWPOSITION_CLEARED: rstr="DBREASON_ROWPOSITION_CLEARED"; break;
	case DBREASON_ROW_ASYNCHINSERT: rstr="DBREASON_ROW_ASYNCHINSERT"; break;
	default: sprintf(drbuf,"%d",eReason); rstr=drbuf; break;
	};
	char dpbuf[32]={0};
	const char *pstr="";
	switch(ePhase)
	{
	case DBEVENTPHASE_OKTODO: pstr="DBEVENTPHASE_OKTODO"; break;
	case DBEVENTPHASE_ABOUTTODO: pstr="DBEVENTPHASE_ABOUTTODO"; break;
	case DBEVENTPHASE_SYNCHAFTER: pstr="DBEVENTPHASE_SYNCHAFTER"; break;
	case DBEVENTPHASE_FAILEDTODO: pstr="DBEVENTPHASE_FAILEDTODO"; break;
	case DBEVENTPHASE_DIDEVENT: pstr="DBEVENTPHASE_DIDEVENT"; break;
	default: sprintf(dpbuf,"%d",ePhase); pstr=dpbuf; break;
	};
	char dbuf[256]={0};
	sprintf(dbuf,"DEBUG OnRowChange(reason=%s, phase=%s)\r\n",rstr,pstr);
	OutputDebugString(dbuf);
#endif

	CQueryResult *res=pSender->m_queryResult;
	if(!res)
		return;
	char rbuf[4096]={0};
	switch(eReason)
	{
	case DBREASON_ROW_UPDATE:
		switch(ePhase)
		{
		case DBEVENTPHASE_ABOUTTODO:
			// ignore
			return;
			break;
		case DBEVENTPHASE_DIDEVENT:
			//PrintRow(_T("Updated"), pSender, hRow);
			PrintRow(rbuf,sizeof(rbuf),"Updated",pSender,hRow);
			VSOleDBMsgReady(pSender->m_PortNo,rbuf,strlen(rbuf),res->m_porttype,res->m_portno,res->m_fp,res->m_udata);
			break;
		}
		break;
	case DBREASON_ROW_INSERT:
		//PrintRow(_T("Inserted"), pSender, hRow);
		if((pSender->m_PortType==WS_CON)&&(pSender->m_PortNo>=0)&&(pSender->m_PortNo<NO_OF_CON_PORTS))
		{
			if(res->m_rows==0)
			{
				PrintRow(rbuf,sizeof(rbuf),"Inserted",pSender,-1);
				VSOleDBMsgReady(pSender->m_PortNo,rbuf,strlen(rbuf),res->m_porttype,res->m_portno,res->m_fp,res->m_udata);
			}
			PrintRow(rbuf,sizeof(rbuf),"Inserted",pSender,hRow);
			VSOleDBMsgReady(pSender->m_PortNo,rbuf,strlen(rbuf),res->m_porttype,res->m_portno,res->m_fp,res->m_udata);
			res->m_tlast=GetTickCount();
		}
		break;
	case DBREASON_ROW_DELETE:
		//PrintRow(_T("Deleted"), pSender, hRow);
		PrintRow(rbuf,sizeof(rbuf),"Deleted",pSender,hRow);
		VSOleDBMsgReady(pSender->m_PortNo,rbuf,strlen(rbuf),res->m_porttype,res->m_portno,res->m_fp,res->m_udata);
		break;
	case 20:
		pSender->SetDone();
		if(res->m_rows==0)
		{
			PrintRow(rbuf,sizeof(rbuf),"Inserted",pSender,-1);
			VSOleDBMsgReady(pSender->m_PortNo,rbuf,strlen(rbuf),res->m_porttype,res->m_portno,res->m_fp,res->m_udata);
		}
		VSOleDBMsgReady(pSender->m_PortNo,0,0,res->m_porttype,res->m_portno,res->m_fp,res->m_udata);
		break;
	#ifdef _DEBUG
	default:
	{
		char dbuf[256]={0};
		sprintf(dbuf,"Reason %d not supported, ignored\r\n",eReason);
		OutputDebugString(dbuf);
	}
	#endif
	}
}

struct REDIBUSTASK
{
	ViewServer *pmod;
	string localFile;
	string writePath;
	string remoteFile;
	int qtmax;
	string sql;
	int taskid;
	int rc;
};
static DWORD CALLBACK _RedibusThread(LPVOID arg)
{
	REDIBUSTASK *ptd=(REDIBUSTASK*)arg;
	ptd->pmod->RedibusTask(ptd);
	//delete ptd; This is a repeating task
	return 0;
}
static int _stdcall _RedibusTask(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	DWORD tid=0;
	HANDLE thnd=CreateThread(0,0,_RedibusThread,udata,0,&tid);
	if(thnd)
		CloseHandle(thnd);
	return 0;
}
int ViewServer::LoadRedibusIni()
{
	// Scheduled tasks
	bool weekdays[]={0,1,1,1,1,1,0};
	bool closedays[]={0,0,1,1,1,1,1};
	bool everyday[]={1,1,1,1,1,1,1};
	taskUser.waitAuth=false; taskUser.authed=true;
	char spath[MAX_PATH]={0};
	sprintf(spath,"%s\\setup\\redibus.ini",pcfg->RunPath.c_str());
	FILE *fp=fopen(spath,"rt");
	if(!fp)
	{
		WSLogError("Failed opening %s!",spath);
		return -1;
	}

	taskmgr.DelScheduledTask(&taskUser,TASK_REDIBUS);
	int lno=0,ntasks=0;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!*rptr)||(!strncmp(rptr,"//",2)))
			continue;
		const char *stime="",*slfile="",*srfile="",*sqtmax="",*ssql="";
		int col=0;
		for(const char *tok=strtoke(rptr,",\r\n");tok;tok=strtoke(0,",\r\n"))
		{
			switch(++col)
			{
			case 1: stime=tok; break;
			case 2: slfile=tok; break;
			case 3: srfile=tok; break;
			case 4: sqtmax=tok; break;
			//case 5: ssql=tok; break;
			};
			if(col>=4) break; // To allow commas in the sql
		}
		ssql=strtoke(0,"\r\n");
		if((!*stime)||(!*slfile)||(!*sqtmax)||(!*ssql))
		{
			WSLogError("Invalid line %d in %s",lno,spath);
			continue;
		}
		int tstart=atoi(stime);
		REDIBUSTASK *ptd=new REDIBUSTASK;
		ptd->pmod=this;
		if(slfile[1]==':')
			ptd->localFile=slfile;
		else
			ptd->localFile=pcfg->RunPath +(string)"\\" +(string)slfile;
		ptd->remoteFile=srfile;
		ptd->qtmax=atoi(sqtmax);
		ptd->sql=ssql;
		ptd->taskid=++nextTaskId;
		ptd->rc=0;
		taskmgr.AddScheduledTask(&taskUser,TASK_REDIBUS,ptd,_RedibusTask,0,tstart,true,true,weekdays,WSDate,WSTime);
		ntasks++;
	}
	fclose(fp);
	WSLogEvent("Scheduled %d tasks from %s",ntasks,spath);
	return 0;
}
int ViewServer::RedibusTask(REDIBUSTASK *ptd)
{
	// Change any date variables in the select string
	char *sqlstr=new char[ptd->sql.length() +1];
	strcpy(sqlstr,ptd->sql.c_str());
	char *sptr=strstr(sqlstr,"mm/dd/yy");
	if(sptr)
	{
		SYSTEMTIME tsys;
		GetLocalTime(&tsys);
		char dstr[16]={0};
		sprintf(dstr,"%02d/%02d/%02d",tsys.wMonth,tsys.wDay,tsys.wYear -2000);
		memcpy(sptr,dstr,8);
	}

	// Open the output file
	char lpath[MAX_PATH]={0};
	strcpy(lpath,ptd->localFile.c_str());
	if((lpath[0]!='\\')&&(lpath[1]!=':'))
		sprintf(lpath,"%s\\%s",pcfg->RunPath.c_str(),ptd->localFile.c_str());
	char *dptr=strstr(lpath,"yyyymmdd");
	if(dptr)
	{
		SYSTEMTIME tsys;
		GetLocalTime(&tsys);
		char dstr[16]={0};
		sprintf(dstr,"%04d%02d%02d",tsys.wYear,tsys.wMonth,tsys.wDay);
		memcpy(dptr,dstr,8);
	}
	ptd->writePath=lpath;
	FILE *fp=fopen(lpath,"wt");
	if(!fp)
	{
		WSLogError("Failed writing local file (%s) for task (%s)!",lpath,sqlstr);
		delete sqlstr;
		ptd->rc=-1;
		return -1;
	}

	// Find a connection
	int ConPortNo=-1;
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		if(ConPort[i].DetPtr==(void*)PROTO_REDIOLEDB)
		{
			ConPortNo=i;
			break;
		}
	}
	if(ConPortNo<0)
	{
		WSLogError("No connected $REDIOLEDB$ ports found for task (%s)!",sqlstr);
		fclose(fp);
		delete sqlstr;
		ptd->rc=-1;
		return -1;
	}
	bool disableBounce=REDIBUS_BOUNCE_MISSED_HBS; REDIBUS_BOUNCE_MISSED_HBS=false;
	WSLogEvent("CON%d: RedibusTask(%s) => (%s)",ConPortNo,sqlstr,lpath);

	int qid=VSRediQuery(ConPortNo,sqlstr,-1,-1,fp,0);
	if((qid>0)&&(VSWaitRediQuery(ConPortNo,ptd->qtmax,qid,false)>0))
	{
		fclose(fp);
		WSLogEvent("CON%d: RedibusTask => (%s) completed.",ConPortNo,lpath);
		// Send positions to the hub
		if(strstr(sqlstr,"RTPOSITION"))
			SendRediPositions(lpath);
		else if(!stricmp(sqlstr,"select FirmId,Name from RTDOMAIN"))
		{
			QueryRediPositions(lpath);
		}
		else if(!stricmp(sqlstr,"select * from CAT"))
		{
			map<string,REDITABLE*> omap,nmap;
			if(RediSqlLoadCat(omap,"data\\CAT_prev.csv")<0)
				WSLogEvent("No data\\CAT_prev.csv to check for bus differences.");
			else if(RediSqlLoadCat(nmap,lpath)<0)
				WSLogError("Failed loading data\\CAT.csv to check for bus differences.");
			else
				RediSqlDiff(0,omap,nmap);
			char dpath[MAX_PATH]={0};
			sprintf(dpath,"%s\\data\\CAT_prev.csv",pcfg->RunPath.c_str());
			CopyFile(lpath,dpath,false);
		}
		else if(!stricmp(sqlstr,"select * from DESTINATIONINFO"))
		{
			char rdpath[MAX_PATH]={0};
			sprintf(rdpath,"%s\\data\\RediDestination.csv",pcfg->RunPath.c_str());
			GenRediDestinationCsv(rdpath,lpath);
			strcpy(lpath,rdpath);
		}
		// DEV-23788
		else if(strstr(ptd->remoteFile.c_str(),"RTSYM_EQ_ALL"))
		{
			char ext[64]={0};
			strncpy(ext,strstr(ptd->remoteFile.c_str(),"RTSYM_EQ_ALL")+12,sizeof(ext));
			char *eptr=strchr(ext,'.');
			if(eptr) *eptr=0;			
			char eupath[MAX_PATH]={0};
			sprintf(eupath,"P:\\dafsoft\\import\\EUList%s-%08d.txt",ext,WSDate);
			GenEUList(eupath,lpath);
		}
		// DEV-23769
		else if(strstr(ptd->remoteFile.c_str(),"RTSYM_EQ"))
		{
			char ext[64]={0};
			strncpy(ext,strstr(ptd->remoteFile.c_str(),"RTSYM_EQ")+8,sizeof(ext));
			char *eptr=strchr(ext,'.');
			if(eptr) *eptr=0;			
			char tsxpath[MAX_PATH]={0};
			sprintf(tsxpath,"P:\\dafsoft\\import\\TSXList%s-%08d.txt",ext,WSDate);
			GenTsxList(tsxpath,lpath);
		}
		ptd->rc=1;
	}
	else
	{
		fclose(fp);
		WSLogError("CON%d: RedibusTask => (%s) failed!",ConPortNo,lpath);
		ptd->rc=-2;
	}

	if(!ptd->remoteFile.empty())
	{
		FileOptions fopts=WSFileOptions;
		fopts.waitTimeout=60;
		WIN32_FIND_DATA fdata;
		memset(&fdata,0,sizeof(WIN32_FIND_DATA));
		HANDLE fhnd=FindFirstFile(lpath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
			FindClose(fhnd);
		if(WSTransmitFile(lpath,ptd->remoteFile.c_str(),0,0,0,&fopts))
			WSLogEvent("CON%d: RedibusTask transferred (%s) to (%s)",ConPortNo,lpath,ptd->remoteFile.c_str());
		else
			WSLogError("CON%d: RedibusTask failed to transfer (%s) to (%s)!",ConPortNo,lpath,ptd->remoteFile.c_str());
	}
	REDIBUS_BOUNCE_MISSED_HBS=disableBounce;
	delete sqlstr;
	return 0;
}
int ViewServer::RedibusTask(FILE *fp, const char *cmd, const char *taskid, const char *datetime)
{
	if(!stricmp(cmd,"list"))
	{
		fprintf(fp,"taskid,nextdate,nexttime,repeat,sto,days,status,localfile,writepath,remotefile,maxtime,sql,\n");
		for(FeedTaskMgr::stiterator tit=taskmgr.stbegin();tit!=taskmgr.stend();tit++)
		{
			SCHEDENTRY *pse=*tit;
			REDIBUSTASK *ptask=(REDIBUSTASK*)pse->udata;
			char days[16]={0};
			if(pse->days[0]) strcat(days,"U");
			if(pse->days[1]) strcat(days,"M");
			if(pse->days[2]) strcat(days,"T");
			if(pse->days[3]) strcat(days,"W");
			if(pse->days[4]) strcat(days,"R");
			if(pse->days[5]) strcat(days,"F");
			if(pse->days[6]) strcat(days,"S");
			char status[16]={0};
			if(!ptask->rc) strcpy(status,"scheduled");
			else if(ptask->rc<0) strcpy(status,"failed");
			else if(ptask->rc>0) strcpy(status,"success");
			fprintf(fp,"%d,%08d,%06d,%d,%d,%s,%s,%s,%s,%s,%d,%s,\n",
				ptask->taskid,pse->nextDate,pse->nextTime,pse->repeat,pse->sto,days,status,
				ptask->localFile.c_str(),ptask->writePath.c_str(),ptask->remoteFile.c_str(),ptask->qtmax,ptask->sql.c_str());
		}
	}
	else if(!stricmp(cmd,"run"))
	{
		int nqueued=0;
		// Failed tasks only
		if(!strcmp(taskid,"failed"))
		{
			for(FeedTaskMgr::stiterator tit=taskmgr.stbegin();tit!=taskmgr.stend();tit++)
			{
				SCHEDENTRY *pse=*tit;
				REDIBUSTASK *ptask=(REDIBUSTASK*)pse->udata;
				if(ptask->rc<0)
				{
					fprintf(fp,"Queueing task #%d: %s...\n",ptask->taskid,ptask->sql.c_str());
					runNowTaskList.push_back(pse); nqueued++;
				}
			}
		}
		// Failed or not run yet
		else if(!strcmp(taskid,"all"))
		{
			// Failed first
			for(FeedTaskMgr::stiterator tit=taskmgr.stbegin();tit!=taskmgr.stend();tit++)
			{
				SCHEDENTRY *pse=*tit;
				REDIBUSTASK *ptask=(REDIBUSTASK*)pse->udata;
				if(ptask->rc<0)
				{
					fprintf(fp,"Queueing task #%d: %s...\n",ptask->taskid,ptask->sql.c_str());
					runNowTaskList.push_back(pse); nqueued++;
				}
			}
			// Then tasks not yet run
			for(FeedTaskMgr::stiterator tit=taskmgr.stbegin();tit!=taskmgr.stend();tit++)
			{
				SCHEDENTRY *pse=*tit;
				REDIBUSTASK *ptask=(REDIBUSTASK*)pse->udata;
				if(ptask->rc==0) 
				{
					fprintf(fp,"Queueing task #%d: %s...\n",ptask->taskid,ptask->sql.c_str());
					runNowTaskList.push_back(pse); nqueued++;
				}
			}
		}
		// By ordered taskid list, regardless of status
		else
		{
			char stask[256]={0};
			strcpy(stask,taskid);
			for(const char *tok=strtok(stask,",");tok;tok=strtok(0,","))
			{
				int tid=atoi(tok);
				if(!tid) 
					continue;
				for(FeedTaskMgr::stiterator tit=taskmgr.stbegin();tit!=taskmgr.stend();tit++)
				{
					SCHEDENTRY *pse=*tit;
					REDIBUSTASK *ptask=(REDIBUSTASK*)pse->udata;
					if(ptask->taskid==tid)
					{
						fprintf(fp,"Queueing task #%d: %s...\n",ptask->taskid,ptask->sql.c_str());
						runNowTaskList.push_back(pse); nqueued++;
					}
				}
			}
		}
		fprintf(fp,"%d tasks queued.",nqueued);
	}
	else if(!stricmp(cmd,"modify"))
	{
	}
	else if(!stricmp(cmd,"reload"))
	{
	}
	else
	{
		fprintf(fp,"Unknown Cmd=%s",cmd);
	}
	return 0;
}
int ViewServer::ReadCsvThread(VSDetailFile *dfile)
{
	VSDistJournal *dj=(VSDistJournal *)UscPort[dfile->portno].DetPtr5;
	if(!dj)
		return -1;
	#ifndef NO_EFILLS_FILE
	if(!*ffpath)
		#ifdef WIN32
		sprintf(ffpath,"%s\\efills_%08d.fix",FILLS_FILE_DIR.c_str(),WSDate);
		#else
		sprintf(ffpath,"%s/efills_%08d.fix",FILLS_FILE_DIR.c_str(),WSDate);
		#endif
	#endif

	list<string> clist;
#define READBLOCKSIZE (512*1024) // 512K tried 256K,1M,2M,4M; this is the best value
	// In order to maximize CPU on this thread, always have an overlapping read while we're processing
	OVERLAPPED ovls[2];
	char *rbufs[2];
	char *rbuf=new char[READBLOCKSIZE*2],*rptr=rbuf,*rbend=rbuf +READBLOCKSIZE*2;
	if(!rbuf)
	{
		_ASSERT(false);
		return -1;
	}
	memset(rbuf,0,READBLOCKSIZE*2);
	for(int i=0;i<2;i++)
	{
		memset(&ovls[i],0,sizeof(OVERLAPPED));
		rbufs[i]=new char[READBLOCKSIZE];
		if(!rbufs[i])
		{
			_ASSERT(false);
			return -1;
		}
		memset(rbufs[i],0,READBLOCKSIZE);
	}
	int pendidx=-1;
	dfile->LockRead();
	// Since we're double-buffering, 'rnext' doesn't always equal 'dfile->rend'
	LARGE_INTEGER rnext;
	rnext.QuadPart=dfile->rend.QuadPart;
	FixStats *ustats=(FixStats *)UscPort[dfile->portno].DetPtr3;
	if(!ustats)
	{
		_ASSERT(false);
		dfile->UnlockRead();
		return -1;
	}
	while(dfile->rhnd!=INVALID_FILE_VALUE)
	{
		dfile->UnlockRead();
		// 'rnext' is local so we don't need to lock read.
		// Mutex write against VSOrder::WriteDetailBlock.
		dfile->LockWrite();
		if(dfile->rend.QuadPart>=dfile->fend.QuadPart)
		{
			dfile->UnlockWrite();
			SleepEx(100,true);
			dfile->LockRead();
			continue;
		}
		// Only reference the end of write value while we have it locked
		LONGLONG ravail=dfile->fend.QuadPart -rnext.QuadPart;
		dfile->UnlockWrite();

		// Read a big chunk of details 
		// Mutex read against VSOrder::ReadDetail
		dfile->LockRead();
		OVERLAPPED *povl=0;
		// Read is already pending
		//char dbuf[1024]={0};
		DWORD rbytes=0;
		if(pendidx<0)
		{
			// Issue first read: don't exceed half of 'rbuf'
			LONGLONG rllen=ravail;
			DWORD rleft=(DWORD)(rptr -rbuf);
			if(rllen>(READBLOCKSIZE -rleft))
				rllen=(READBLOCKSIZE -rleft);
			DWORD rlen=(DWORD)rllen;
			ravail-=rlen;
			//sprintf(dbuf,"DEBUG: Read1[%I64d,%I64d), pend=%d\r\n",rnext.QuadPart,rnext.QuadPart +rlen,0);
			//OutputDebugString(dbuf);
			DWORD rbytes=0;
			pendidx=0;
			povl=&ovls[pendidx];
			povl->Offset=rnext.LowPart;
			povl->OffsetHigh=rnext.HighPart;
			rnext.QuadPart+=rlen;
			int rc=ReadFile(dfile->rhnd,rbufs[pendidx],rlen,&rbytes,povl);
			int err=GetLastError();
			if((rc<0)&&(err!=ERROR_IO_PENDING))
			{
				#ifdef WIN32
				WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen);
				#else
				WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen);
				#endif
				dfile->UnlockRead();
				break;
			}
			pendidx=0;
		}
		// Wait for pending read
		povl=&ovls[pendidx];
		rbytes=0;
		GetOverlappedResult(dfile->rhnd,povl,&rbytes,true);
		if(rbytes>0)
		{
			_ASSERT(rptr +rbytes<=rbend);
			memcpy(rptr,rbufs[pendidx],rbytes); rptr+=rbytes; 
		}
		// Issue next overlapped read while we're processing
		bool lastAvail=false;
		if(ravail>0)
		{
			LONGLONG rllen2=ravail;
			if(rllen2>READBLOCKSIZE)
				rllen2=READBLOCKSIZE;
			DWORD rlen2=(DWORD)rllen2;
			ravail-=rlen2;
			DWORD rbytes2=0;
			pendidx=(pendidx +1)%2;
			//sprintf(dbuf,"DEBUG: Read2[%I64d,%I64d), pend=%d\r\n",rnext.QuadPart,rnext.QuadPart +rlen2,pendidx);
			//OutputDebugString(dbuf);
			povl=&ovls[pendidx];
			povl->Offset=rnext.LowPart;
			povl->OffsetHigh=rnext.HighPart;
			rnext.QuadPart+=rlen2;
			int rc=ReadFile(dfile->rhnd,rbufs[pendidx],rlen2,&rbytes2,povl);
			int err=GetLastError();
			if((rc<0)&&(err!=ERROR_IO_PENDING))
			{
				#ifdef WIN32
				WSLogError("Failed reading data file(%s) with error %d at %I64d for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen2);
				#else
				WSLogError("Failed reading data file(%s) with error %d at %lld for %d bytes!",dfile->prefix.c_str(),rnext.QuadPart,rlen2);
				#endif
				dfile->UnlockRead();
				break;
			}
		}
		else
		{
			pendidx=-1; lastAvail=true;
		}
		dfile->UnlockRead(); // No need to lock the file while we're parsing

		// There will be multiple messages within the block
		const char *fend=rptr;
		const char *fptr;
		for(fptr=rbuf;fptr<fend;)
		{
			const char *fendl=0,*fstart=fptr;
			int flen=0;
			char AppInstID[20]={0};			// AppInstID(11505)
			char ClOrdID[40]={0};			// ClOrdID(11)
			char RootOrderID[40]={0};		// tag 70129
			char FirstClOrdID[40]={0};		// tag 5055
			char ClParentOrderID[40]={0};	// tag 5035
			char Symbol[32]={0};			// Symbol(55,65)
			double Price=0.0;
			char Side=0;
			char SymSfx[12]={0};			// Symbol(55,65)
			char Account[20]={0};			// Account(1)
			char EcnOrderID[40]={0};		// EcnOrderID(37)
			char ClientID[24]={0};			// ClientID(109)
			char Connection[48]={0};		// Connection
			char AuxKey[AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN]={0}; // AuxKeys  DT9398
			char mtype=0,etype=0;
			int ordQty=0,cumQty=0,leavesQty=0,fillQty=0;
			LONGLONG TimeStamp=0;
			int OrderDate=0;
			const char *OrigOrdID="",*ExecID="";

			if(dfile->proto==PROTO_CSV)
			{
				char *nptr=(char*)strendl(fptr,fend);
				if(!nptr)
					break;
				*nptr=0;
				int flen=(int)(nptr -fptr);	fendl=fptr +flen;
				LONGLONG doffset=dfile->rend.QuadPart +(int)(fptr -rbuf);
				LONGLONG dend=doffset +flen;			

				char *rptr;
				for(rptr=(char*)fptr;(rptr<fendl)&&(isspace(*rptr));rptr++)
					;
				if((*rptr)&&(*rptr!='\r')&&(*rptr!='\n')&&(strncmp(rptr,"select ",7)))
				{
					int cno=0;
					if(strstr(dfile->fpath.c_str(),"\\RTPOSITION"))
					{
						// Parse the column header
						if(!strncmp(rptr,"Row,",4))
						{
							if(!clist.empty())
								clist.clear();
							for(const char *tok=strtoke(rptr,",");tok;tok=strtoke(0,","))
							//for(const char *tok=strtoke(rptr,"\1");tok;tok=strtoke(0,"\1"))
								clist.push_back(tok);
						}
						// Parse row data
						else
						{
							list<string>::iterator cit=clist.begin();
							const char *srow="",*sacct="",*sleaf="",*ssym="",*sqty="",*sprice="",*scur="",*stime="";
							for(const char *tok=strtoke(rptr,",");tok;tok=strtoke(0,","))
							//for(const char *tok=strtoke(rptr,"\1");tok;tok=strtoke(0,"\1"))
							{
								if(cit!=clist.end())
								{
									const char *cname=cit->c_str();
									if(!stricmp(cname,"Row"))
										srow=tok;
									else if(!stricmp(cname,"AccountSynonym"))
										sacct=tok;
									else if(!stricmp(cname,"ProductId"))
										ssym=tok;
									else if(!stricmp(cname,"RtLeafId"))
										sleaf=tok;
									else if(!stricmp(cname,"Quantity"))
										sqty=tok;
									else if(!stricmp(cname,"Price"))
										sprice=tok;
									else if(!stricmp(cname,"Currency"))
										scur=tok;
									else if(!stricmp(cname,"ModifiedTime"))
										stime=tok;
									cit++;
								}
							}
							TACCTMAP::iterator ait=tacmap.find(sacct);
							if(ait!=tacmap.end())
								//sprintf(AppInstID,"$%s",ait->second.first.c_str());
								sprintf(AppInstID,"%s%s",TACMAP_PREFIX.c_str(),ait->second.first.c_str());
							else
								strcpy(AppInstID,"$RTPOSITION");
							//_strupr(AppInstID);
							sprintf(ClOrdID,"RTPOS_%s",srow);
							strcpy(Symbol,ssym);
							Price=atof(sprice);
							//Side='1';
							strcpy(Account,sacct);
							int mon=0,dd=0,yy=0,hh=0,mm=0,ss=0;
							sscanf(stime,"%02d/%02d/%02d %02d:%02d:%02d",&mon,&dd,&yy,&hh,&mm,&ss); yy+=2000;
							int odate=(yy*10000) +(mon*100) +(dd);
							OrderDate=WSDATE ?WSDATE :WSDate;
							mtype=0x05; // iqpos
							etype='2';
							ordQty=atoi(sqty);
							cumQty=ordQty;
							TimeStamp=((LONGLONG)odate)*1000000 +((hh)*10000 +(mm)*100 +(ss));
							ExecID=srow;
							fillQty=0;

							AddJournal(dj,fptr,flen,doffset,dend,
										AppInstID,ClOrdID,RootOrderID,
										FirstClOrdID,ClParentOrderID,Symbol,
										Price,Side,
										Account,EcnOrderID,ClientID,
										Connection,OrderDate,AuxKey,
										mtype,etype,ordQty,cumQty,leavesQty,
										TimeStamp,OrigOrdID,ExecID,fillQty,ffpath);
							ustats->msgCnt++; ustats->totMsgCnt++;
						}
					}
					else if(strstr(dfile->fpath.c_str(),"\\RTAUDIT"))
					{
						// TODO: Parse audit .csv file line
					}
				}
				*nptr='\r';

				fptr=fendl;
				while((fptr<fend)&&((*fptr=='\r')||(*fptr=='\n'))) // CRLF
					fptr++;
				_ASSERT(fptr<=fend);
			}
		}

		// Preserve leftovers
		rptr=rbuf;
		if(fptr<=fend)
		{
			int bleft=(int)(fend -fptr);
			if(fptr>rbuf)
			{
				// This is the only function that modifies 'dfile->rend', so we don't need to lock read.
				_ASSERT((rbuf<=fptr)&&(fptr<=fend));
				dfile->rend.QuadPart+=(fptr -rbuf); 
				odb.SetZdirty();
				//sprintf(dbuf,"DEBUG: leftover %d [%I64d,%d), pend=%d\r\n",bleft,dfile->rend.QuadPart,dfile->rend.QuadPart +bleft,pendidx);
				//OutputDebugString(dbuf);
				memmove(rptr,fptr,bleft);
			}
			rptr+=bleft;
		}
		dfile->LockRead();
	}
	dfile->UnlockRead();
	for(int i=0;i<2;i++)
		delete rbufs[i];
	delete rbuf;
	return 0;
}
// Copies data\RTPOSITION.csv to data\RTPOSITION_yyyymmdd.csv but converts from CSV to DSV
int ViewServer::SendRediPositions(const char *fpath)
{
	VSDetailFile *dfile=0;
	for(int i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(!UscPort[i].InUse)
			continue;
		if(!strncmp(UscPort[i].CfgNote,"$RTPOSITION$",12))
		{
			dfile=(VSDetailFile *)UscPort[i].DetPtr1;
			break;
		}
	}
	if(!dfile)
	{
		WSLogError("Failed locating daily RTPOSITION file for (%s)!",fpath);
		return -1;
	}

	FILE *fp=fopen(fpath,"rb");
	if(!fp)
	{
		WSLogError("Failed opening RTPOSITION source (%s)!",fpath);
		return -1;
	}
	list<string> clist;
	char rbuf[1024]={0};
	int npos=0;
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		//// Convert all commas to 0x01
		//for(char *cptr=rbuf;*cptr;cptr++)
		//{
		//	if(*cptr==',') *cptr=0x01;
		//}
		// Copy to the detail file
		LONGLONG doff=-1;
		odb.WriteDetailBlock(dfile,rbuf,strlen(rbuf),doff);
		npos++;
	}
	fclose(fp);
	WSLogEvent("Copied %d RTPOSITIONs from (%s) to (%s).",npos>0?npos-1:npos,fpath,dfile->fpath.c_str());
	return 0;
}

struct RedibusQueryTaskData
{
	ViewServer *pmod;
	int UsrPortNo;
	int ConPortNo;
	string select;
};
int _stdcall _NotifyRediBusInsert(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	RedibusQueryTaskData *ptd=(RedibusQueryTaskData *)udata;
	if(!ptd)
		return -1;
	int rc=ptd->pmod->NotifyRediBusInsert(fuser,skey,taskid,rid,udata,hint);
	return rc;
}
int ViewServer::NotifyRediBusInsert(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	RedibusQueryTaskData *ptd=(RedibusQueryTaskData *)udata;
	if(!ptd)
		return -1;
	const char *InsertKey=(const char *)hint;
	char sql[1024]={0};
	if(stristr(ptd->select.c_str(),"WHERE"))
		sprintf(sql,"%s AND %s",ptd->select.c_str(),InsertKey);
	else
		sprintf(sql,"%s WHERE %s",ptd->select.c_str(),InsertKey);
	//#ifdef _DEBUG
	WSLogEvent("USR%d: DEBUG NotifyRediBusInsert(%s)",ptd->UsrPortNo,sql);
	//#endif
	// Issue client-specific query for newly-inserted item
	int pqid=VSRediQuery(ptd->ConPortNo,sql,WS_USR,fuser->PortNo,0,udata);
	// Don't wait on query, because this callback is nested within another query
	//if(pqid>0)
	//{
	//	if(VSWaitRediQuery(ptd->ConPortNo,10,pqid,false)<0)
	//		pqid=-1;
	//}
	return 1;
}
int ViewServer::HandleUsrRpxFakeAPIMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	VSCodec *vsc=(VSCodec *)UsrPort[PortNo].DetPtr6;
	if(vsc)
	{
		// SQL query
		if(MsgHeader->MsgID==101)
		{
			DWORD qid=0;
			memcpy(&qid,Msg,4);
			// Pass through SQLServer
			int ConPortNo=-1;
			for(int i=0;i<NO_OF_CON_PORTS;i++)
			{
				if(!ConPort[i].SockConnected)
					continue;
				if(ConPort[i].DetPtr==(void*)PROTO_REDIOLEDB)
				{
					ConPortNo=i;
					break;
				}
			}
			if(ConPortNo<0)
			{
				WSLogError("USR%d: No connected $REDIOLEDB$ ports found.",PortNo);
				return -1;
			}
			//rpxFakeApiPort=PortNo;
			char *sstr=new char[MsgHeader->MsgLen -4 +1024];
			strncpy(sstr,Msg +4,MsgHeader->MsgLen -4);
			sstr[MsgHeader->MsgLen -4]=0;
			WSLogEvent("USR%d: HandleUsrRpxFakeAPIMsg(%d,%s)",PortNo,qid,sstr);

			char tname[256]={0};
			string execstr;
			if(stristr(sstr,"INSERT INTO "))
			{
				const char *tptr=stristr(sstr,"INTO ") +5;
				const char *tend;
				for(tend=tptr;(*tend)&&(*tend!=' ')&&(*tend!='(');tend++)
					;
				if(!tend) tend=tptr +strlen(tptr);
				strncpy(tname,tptr,(int)(tend -tptr));
				tname[(int)(tend -tptr)]=0;
				if(!stristr(tname,"_TRANS"))
				{
					WSLogError("USR%d: INSERT INTO non-TRANS table(%s) not permitted.",PortNo,tname);
					return -1;
				}
				// Auto-generate a tranId for SLTRAN_TRANS
				if(!stricmp(tname,"SLTRAN_TRANS"))
				{
					char *cptr=strchr(sstr,'(');
					if(cptr)
					{
						memmove(cptr +8,cptr+1,strlen(cptr+1)+1);
						memcpy(cptr+1,"tranId,",7);
						cptr=(char*)stristr(cptr,"VALUES");
						if(cptr)
						{
							cptr=strchr(cptr,'(');
							if(cptr)
							{
								UUID uuid;
								UuidCreateSequential(&uuid);
								unsigned char *suuid;
								UuidToString(&uuid,&suuid);
								memmove(cptr+40,cptr+1,strlen(cptr+1)+1);
								cptr[1]='\''; memcpy(cptr+2,suuid,36); cptr[38]='\''; cptr[39]=',';
								RpcStringFree(&suuid);
							}
						}
					}
					execstr="{rpc spsltran_trans};";
				}
				// All others just call the corresponding stored procedure
				else if(strrcmp(tname,"_TRANS"))
				{
					char spcmd[256]={0};
					sprintf(spcmd,"{rpc sp%s}",tname);
					_strlwr(spcmd +10);
					execstr=spcmd;
				}
			}
			// Apply deletes straight to table
			else if(stristr(sstr,"DELETE FROM "))
			{
				char *tptr=(char*)stristr(sstr,"FROM ");
				if(tptr)
				{
					tptr+=5;
					const char *tend=strchr(tptr,' ');
					if(!tend) tend=tptr +strlen(tptr);
					strncpy(tname,tptr,(int)(tend -tptr));
					tname[(int)(tend -tptr)]=0;
				}
				if(!stristr(tname,"_TRANS"))
				{
					WSLogError("USR%d: DELETE FROM non-TRANS table(%s) not permitted.",PortNo,tname);
					return -1;
				}
				// Change table name
				char dname[256]={0};
				if(!stricmp(tname,"SLTRAN_TRANS"))
					strcpy(dname,"SLTRAN_DETAIL");
				else
				{
					strcpy(dname,tname);
					char *dptr=strstr(dname,"_TRANS");
					if(dptr) *dptr=0;
				}
				memmove(tptr +strlen(dname),tptr +strlen(tname),strlen(tptr +strlen(tname))+1);
				memcpy(tptr,dname,strlen(dname));
			}
			// Apply updates straight to table
			else if(stristr(sstr,"UPDATE "))
			{
				char *tptr=(char*)stristr(sstr,"UPDATE ");
				if(tptr)
				{
					tptr+=7;
					const char *tend=strchr(tptr,' ');
					if(!tend) tend=tptr +strlen(tptr);
					strncpy(tname,tptr,(int)(tend -tptr));
					tname[(int)(tend -tptr)]=0;
				}
				if(!stristr(tname,"_TRANS"))
				{
					WSLogError("USR%d: UPDATE non-TRANS table(%s) not permitted.",PortNo,tname);
					return -1;
				}
				// Change table name
				char dname[256]={0};
				if(!stricmp(tname,"SLTRAN_TRANS"))
					strcpy(dname,"SLTRAN_DETAIL");
				else
				{
					strcpy(dname,tname);
					char *dptr=strstr(dname,"_TRANS");
					if(dptr) *dptr=0;
				}
				memmove(tptr +strlen(dname),tptr +strlen(tname),strlen(tptr +strlen(tname))+1);
				memcpy(tptr,dname,strlen(dname));
			}
			// Future updates to the table will be sent as "live" updates
			else if(stristr(sstr,"SELECT"))
			{
				const char *tptr=stristr(sstr,"FROM ");
				if(tptr)
				{
					tptr+=5;
					const char *tend=strchr(tptr,' ');
					if(!tend) tend=tptr +strlen(tptr);
					strncpy(tname,tptr,(int)(tend -tptr));
					tname[(int)(tend -tptr)]=0;
					FeedUser *fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
					if(fuser)
					{
						fuser->authed=true; fuser->waitAuth=false;
						// There may be more than one query per user per table
						//TASKENTRY *ptask=taskmgr.FindTask(fuser,tname,TASK_REDIBUS);
						//if(!ptask)
						//{
							RedibusQueryTaskData *ptd=new RedibusQueryTaskData;
							ptd->pmod=this;
							ptd->UsrPortNo=PortNo;
							ptd->ConPortNo=ConPortNo;
							ptd->select=sstr;
							TASKENTRY *ptask=taskmgr.AddTask(fuser,tname,TASK_REDIBUS,ptd,qid,_NotifyRediBusInsert);
						//}
					}
				}
			}

			// TRANSACT-SQL query
			int pqid=VSRediQuery(ConPortNo,sstr,WS_USR,PortNo,0,(void*)qid);
			if(pqid>0)
			{
				int qtmax=30;
				if((strstr(sstr," RTSYM"))||(strstr(sstr," RTPOSITION")))
					qtmax=600;
				if(VSWaitRediQuery(ConPortNo,qtmax,pqid,true)<0)
					pqid=-1;
				// Execute stored procedure on successful insert
				else if(!execstr.empty())
				{
					int pqid=VSRediQuery(ConPortNo,execstr.c_str(),WS_USR,PortNo,0,(void*)qid);
					if(pqid>0)
					{
						if(VSWaitRediQuery(ConPortNo,10,pqid,true)<0)
							pqid=-1;
					}
				}
			}
			//rpxFakeApiPort=-1;
			delete sstr;
		}
		// SQL deregister
		else if(MsgHeader->MsgID==103)
		{
			DWORD qid=0;
			memcpy(&qid,Msg,4);
			FeedUser *fuser=(FeedUser*)UsrPort[PortNo].DetPtr9;
			if(fuser)
			{
				for(TASKENTRY *ptask=fuser->ptasks;ptask;ptask=ptask->unext)
				{
					if(ptask->rid==qid)
					{
						taskmgr.DelUserTask(ptask);
						break;
					}
				}
			}
		}
	}
	FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
	if(ustats)
	{
		ustats->msgCnt++; ustats->totMsgCnt++;
	}
	return 0;
}

struct REDICOL
{
	string name;
	int index;
	int vartype;
	int maxdatalen;
	int precision;
	int keytype;
};
struct REDITABLE
{
	string name;
	map<int,REDICOL> cols;
};
static const char *RediVarType(int vartype)
{
	switch(vartype)
	{
	case 2: return "varchar(40)";
	case 3: return "int";
	case 4:
	case 7: return "decimal(12,4)";
	case 5:
	case 6:
	case 17: return "datetime";
	};
	return "";
}
// Loads schema for all tables in CAT.csv
int ViewServer::RediSqlLoadCat(map<string,REDITABLE*>& tmap, const char *fpath)
{
	FILE *rfp=fopen(fpath,"rt");
	if(!rfp)
		return -1;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),rfp))
	{
		if((!strncmp(rbuf,"select ",7))||(!strncmp(rbuf,"Row,",4)))
			continue;
		int col=0;
		REDICOL rcol;
		const char *stable="";
		for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
		{
			switch(++col)
			{
			case 5: stable=tok; break;
			case 6: rcol.name=tok; break;
			case 7: rcol.index=atoi(tok); break;
			case 8: rcol.vartype=atoi(tok); break;
			case 9: rcol.maxdatalen=atoi(tok); break;
			case 13: rcol.precision=atoi(tok); break;
			case 17: rcol.keytype=atoi(tok); break;
			};
			if(col>=17) break;
		}
		REDITABLE *ptable=0;
		map<string,REDITABLE*>::iterator tit=tmap.find(stable);
		if(tit==tmap.end())
		{
			ptable=new REDITABLE;
			ptable->name=stable;
			tmap[stable]=ptable;
		}
		else
			ptable=tit->second;
		ptable->cols[rcol.index]=rcol;
	}
	fclose(rfp);
	return 0;
}
#if defined(_DEBUG)&&defined(IQDEVTEST)
int ViewServer::RediSqlScript(FILE *fp, const char *tables)
{
	map<string,REDITABLE*> tmap;
	//RediSqlLoadCat(tmap,"C:\\src\\oledbtest\\release\\CAT.csv");
	RediSqlLoadCat(tmap,"SysmonTempFiles\\CAT.csv");

	char *tcpy=strdup(tables);
	for(const char *tok=strtok(tcpy,",");tok;tok=strtok(0,","))
	{
		map<string,REDITABLE*>::iterator tit=tmap.end();
		if(!strcmp(tok,"*"))
			tit=tmap.begin();
		else
			tit=tmap.find(tok);
		while(tit!=tmap.end())
		{
			REDITABLE *ptable=tit->second;
			fprintf(fp,"DROP TABLE [dbo].[%s];\n",ptable->name.c_str());
			fprintf(fp,"CREATE TABLE [dbo].[%s](\n",ptable->name.c_str());
			for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
			{
				REDICOL& rcol=cit->second;
				const char *tstr="",*nstr="";
				char sstr[32]={0};
				switch(rcol.vartype)
				{
				case 2: 
					tstr="nvarchar"; 
					sprintf(sstr,"(%d)",rcol.maxdatalen?rcol.maxdatalen:40);
					break;
				case 3: 
					tstr="int";
					//if(rcol.maxdatalen)
					//	sprintf(sstr,"(%d)",rcol.maxdatalen);
					break;
				case 4: 
				case 7:
					tstr="decimal";
					if(rcol.precision) sprintf(sstr,"(%d,%d)",rcol.precision);
					else sprintf(sstr,"(12,4)");
					break;
				case 5: // date only
				case 6: // time only
				case 17: 
					tstr="datetime"; 
					break;
				//case 5: // date only
				//case 6: // time only
				//	tstr="int";
				//	break;
				default:
					_ASSERT(false);
				};
				if(rcol.keytype==7) nstr="NOT NULL";
				else nstr="NULL";
				fprintf(fp,"\t[%s] [%s]%s %s,\n",rcol.name.c_str(),tstr,sstr,nstr);
			}
			fprintf(fp,");\n\n");

			// Stored procedure
			if(strrcmp(ptable->name.c_str(),"_TRANS"))
			{
				char stable[256]={0},dtable[256]={0},spname[256]={0},kname[256]={0};
				const char *ktype="";
				strcpy(stable,ptable->name.c_str());
				strcpy(dtable,stable); dtable[strlen(dtable)-6]=0;
				sprintf(spname,"sp%s",stable); strlwr(spname);
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if(rcol.keytype==7)
					{
						strcpy(kname,rcol.name.c_str());
						ktype=RediVarType(rcol.vartype);
						break;
					}
				}
				bool rowkey=false,modtime=false;
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if(!stricmp(rcol.name.c_str(),"RowKey"))
						rowkey=true;
					else if(!stricmp(rcol.name.c_str(),"ModifiedTime"))
						modtime=true;
				}
				fprintf(fp,"USE [RediBus]\n");
				fprintf(fp,"GO\n");
				fprintf(fp,"\n");
				fprintf(fp,"-- Stored procedure to be called after insert into %s\n",stable);
				fprintf(fp,"--DROP PROCEDURE %s;\n",spname);
				fprintf(fp,"--CREATE PROCEDURE %s\n",spname);
				fprintf(fp,"ALTER PROCEDURE %s\n",spname);
				fprintf(fp,"	@InsertKey [varchar](80) OUTPUT\n");
				fprintf(fp,"AS\n");
				fprintf(fp,"BEGIN\n");
				fprintf(fp,"	BEGIN TRANSACTION;\n");
				if(rowkey)
					fprintf(fp,"	IF EXISTS (SELECT TOP(1) RowKey FROM %s)\n",stable);
				else
					fprintf(fp,"	IF EXISTS (SELECT TOP(1) %s FROM %s)\n",kname,stable);
				fprintf(fp,"	BEGIN\n");
				fprintf(fp,"		-- Grab top row of %s table\n",stable);
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					fprintf(fp,"		DECLARE @%s %s;\n",rcol.name.c_str(),RediVarType(rcol.vartype));
				}
				fprintf(fp,"		SELECT TOP(1) \n");
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					fprintf(fp,"			@%s=%s",rcol.name.c_str(),rcol.name.c_str());
					if(rcol.index+1<(int)ptable->cols.size()) fprintf(fp,",\n");
					else fprintf(fp,"\n");
				}
				fprintf(fp,"			FROM %s;\n",stable);
				string dkey;
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if(rcol.keytype!=7)
						continue;
					if(dkey.empty())
						dkey+=(string)"\n			" +rcol.name +(string)"=@" +rcol.name;
					else
						dkey+=(string)"\n			AND " +rcol.name +(string)"=@" +rcol.name;
				}
				fprintf(fp,"		DELETE FROM %s WHERE %s;\n",stable,dkey.c_str());
				if(rowkey)
				{
					string rkey;
					for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
					{
						REDICOL& rcol=cit->second;
						if(rcol.keytype==7)
						{
							if(!rkey.empty())
								rkey+=(string)"\n			+'.' +";
							else
								rkey+=(string)"\n			";
							if(!strincmp(RediVarType(rcol.vartype),"varchar",7))
								rkey+=(string)"@" +rcol.name;
							else
								rkey+=(string)"CONVERT(varchar(40),@" +rcol.name +(string)")";
						}
					}
					fprintf(fp,"		SET @RowKey = %s;\n",rkey.c_str());
				}
				if(modtime)
					fprintf(fp,"		SET @ModifiedTime = CURRENT_TIMESTAMP;\n");
				fprintf(fp,"\n");
				fprintf(fp,"		-- INSERT\n");
				fprintf(fp,"		IF (@RequestType = 1) BEGIN\n");
				fprintf(fp,"			-- Add to %s\n",dtable);
				fprintf(fp,"			INSERT INTO [RediBus].[dbo].[%s](\n",dtable);
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if((!stricmp(rcol.name.c_str(),"RequestType"))||(!stricmp(rcol.name.c_str(),"RequestStatus")))
						continue;
					fprintf(fp,"				%s",rcol.name.c_str());
					if(rcol.index+1<(int)ptable->cols.size()) fprintf(fp,",\n");
					else fprintf(fp,")\n");
				}
				fprintf(fp,"			VALUES(\n");
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if((!stricmp(rcol.name.c_str(),"RequestType"))||(!stricmp(rcol.name.c_str(),"RequestStatus")))
						continue;
					fprintf(fp,"				@%s",rcol.name.c_str());
					if(rcol.index+1<(int)ptable->cols.size()) fprintf(fp,",\n");
					else fprintf(fp,")\n");
				}
				fprintf(fp,"		-- UPDATE\n");
				fprintf(fp,"		END ELSE BEGIN IF (@RequestType = 2) BEGIN\n");
				fprintf(fp,"			UPDATE [RediBus].[dbo].[%s] SET\n",dtable);
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if((!stricmp(rcol.name.c_str(),"RequestType"))||(!stricmp(rcol.name.c_str(),"RequestStatus"))||(!stricmp(rcol.name.c_str(),"RowKey")))
						continue;
					if(rcol.keytype==7)
						continue;
					fprintf(fp,"				%s=@%s",rcol.name.c_str(),rcol.name.c_str());
					if(rcol.index+1<(int)ptable->cols.size()) fprintf(fp,",\n");
					else fprintf(fp,"\n");
				}
				if(rowkey)
					fprintf(fp,"			WHERE %RowKey=@RowKey\n");
				else
					fprintf(fp,"			WHERE %s=@%s\n",kname,kname);
				fprintf(fp,"		-- DELETE\n");
				fprintf(fp,"		END ELSE BEGIN IF (@RequestType = 3) BEGIN\n");
				fprintf(fp,"			DELETE FROM [RediBus].[dbo].[%s]\n",dtable);
				if(rowkey)
					fprintf(fp,"			WHERE %RowKey=@RowKey\n");
				else
					fprintf(fp,"			WHERE %s=@%s\n",kname,kname);
				fprintf(fp,"		END ELSE BEGIN\n");
				fprintf(fp,"			PRINT 'Unsupported RequestType';\n");
				fprintf(fp,"		END END END\n");
				fprintf(fp,"	END\n");
				fprintf(fp,"	COMMIT TRANSACTION;\n");
				if(rowkey)
					fprintf(fp,"	SET @InsertKey = 'RowKey = ' +@RowKey;\n");
				else
					fprintf(fp,"	SET @InsertKey = '%s = ' +@%s;\n",kname,kname);
				fprintf(fp,"	RETURN 1;\n");
				fprintf(fp,"END\n");
				fprintf(fp,"\n");
				fprintf(fp,"-- Test calls\n");
				fprintf(fp,"-- INSERT\n");
				string colstr;
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if(rcol.keytype!=7)
						continue;
					if(!colstr.empty()) 
						colstr+=",";
					colstr+=rcol.name;
				}
				string valstr;
				for(map<int,REDICOL>::iterator cit=ptable->cols.begin();cit!=ptable->cols.end();cit++)
				{
					REDICOL& rcol=cit->second;
					if(rcol.keytype!=7)
						continue;
					if(!valstr.empty())
						valstr+=",";
					const char *dval="'xxx'";
					switch(rcol.vartype)
					{
					case 3: dval="0"; break;
					case 4:
					case 7: dval="0.0"; break;
					case 5: dval="'mm/dd/yyyy'"; break;
					case 6: dval="'hh:mm:ss'"; break;
					case 17: dval="'mm/dd/yyyy hh:mm:ss'"; break;
					};
					valstr+=dval;
				}
				fprintf(fp,"INSERT INTO %s(RequestType,%s)\n",stable,colstr.c_str());
				fprintf(fp,"	VALUES (1,%s)\n",valstr.c_str());
				fprintf(fp,"DECLARE @InsertKey [varchar](80);\n");
				fprintf(fp,"EXECUTE %s @InsertKey OUTPUT;\n",spname);
				fprintf(fp,"PRINT @InsertKey;\n");
				fprintf(fp,"SELECT * FROM %s;\n",stable);
				fprintf(fp,"SELECT * FROM %s WHERE %s='xxx';\n",dtable,/*rowkey?"RowKey":*/kname);
				fprintf(fp,"-- UPDATE\n");
				fprintf(fp,"INSERT INTO %s(RequestType,%s)\n",stable,colstr.c_str());
				fprintf(fp,"	VALUES (2,%s)\n",valstr.c_str());
				fprintf(fp,"-- DELETE\n");
				fprintf(fp,"INSERT INTO %s(RequestType,%s)\n",stable,colstr.c_str());
				fprintf(fp,"	VALUES (3,%s)\n",valstr.c_str());
				fprintf(fp,"-- Reset test\n");
				fprintf(fp,"DELETE FROM %s;\n",stable);
				fprintf(fp,"DELETE FROM %s WHERE %s='xxx';\n",dtable,/*rowkey?"RowKey":*/kname);
				fprintf(fp,"\n");
				fprintf(fp,"SET ANSI_PADDING OFF\n");
				fprintf(fp,"GO\n");
			}
			if(!strcmp(tok,"*"))
				tit++;
			else
				tit=tmap.end();
			if(!stricmp(tok,"CAT"))
			{
				map<string,REDITABLE*> omap;
				RediSqlLoadCat(omap,"C:\\src\\oledbtest\\release\\CAT.csv");
				RediSqlDiff(fp,omap,tmap);
			}
		}
	}
	free(tcpy);

	for(map<string,REDITABLE*>::iterator tit=tmap.begin();tit!=tmap.end();tit++)
		delete tit->second;
	tmap.clear();
	return 0;
}
int ViewServer::RediSqlLoad(FILE *fp, const char *tables)
{
	char *tcpy=strdup(tables);
	string header;
	char rbuf[2048]={0},vbuf[2048]={0},sstr[4096]={0};
	for(const char *tok=strtok(tcpy,",");tok;tok=strtok(0,","))
	{
		char dpath[MAX_PATH]={0};
		sprintf(dpath,"SysmonTempFiles\\%s.csv",tok);
		FILE *rfp=fopen(dpath,"rt");
		if(rfp)
		{
			while(fgets(rbuf,sizeof(rbuf),rfp))
			{
				char *rend=(char*)strendl(rbuf,rbuf +strlen(rbuf));
				if(rend) *rend=0;
				else rend=rbuf +strlen(rbuf);
				if(!strncmp(rbuf,"select ",7))
					continue;
				else if(!strncmp(rbuf,"Row,Action,Bookmark,",20))
				{
					char *cptr;
					for(cptr=rbuf +strlen(rbuf) -1;(cptr>rbuf)&&(isspace(*cptr));cptr--)
						;
					if(*cptr==',') *cptr=0;
					header=rbuf +20;
					continue;
				}
				char *vptr=vbuf;
				int col=0;
				//for(char *rtok=strtoke(rbuf,",");rtok;rtok=strtoke(0,","))
				char *rtok=rbuf;
				while(*rtok)
				{
					for(;(*rtok)&&(isspace(*rtok));rtok++)
						;
					char *rnext=rtok;
					// Look for closing escape quote
					if(*rnext=='\"')
					{
						rtok++; rnext++;
						for(;(*rnext)&&(*rnext!='\"');rnext++)
							;
					}
					// Look for comma
					else
					{
						for(;(*rnext)&&(*rnext!=',');rnext++)
							;
					}
					char nch=*rnext; *rnext=0;
					if(++col>3)
					{
						while(strchr(rtok,'\''))
							*strchr(rtok,'\'')=' ';
						sprintf(vptr,"'%s',",rtok); vptr+=strlen(vptr);
					}
					*rnext=nch; 
					if(nch=='\"')
						rtok=rnext+2;
					else
						rtok=rnext+1;
				}
				if(vptr>vbuf) vptr[-1]=0;
				sprintf(sstr,"INSERT INTO [dbo].[%s](%s) VALUES(%s)",tok,header.c_str(),vbuf);
				fprintf(fp,"%s\n",sstr);
			}
			fclose(rfp);

			// Inserts from IQ DNS data
			if(!stricmp(tok,"RTDOMAIN"))
			{
				RediSqlInsertDnsDomains(fp);
			}
			else if(!stricmp(tok,"RTACCOUNT"))
			{
				RediSqlInsertDnsAccounts(fp);
			}
			else if(!stricmp(tok,"RTUSER"))
			{
				RediSqlInsertDnsUsers(fp);
			}
		}
		else
			fprintf(fp,"%s not found.\n",dpath);
	}
	free(tcpy);
	return 0;
}
#endif//_DEBUG
int ViewServer::RediSqlDiff(FILE *fp, map<string,REDITABLE*>& tmap, map<string,REDITABLE*>& nmap)
{
	if(fp) 
		fprintf(fp,"Comparing CAT tables...\n");
	else 
		WSLogEvent("Comparing CAT tables...");
	int ndiff=0;
	for(map<string,REDITABLE*>::iterator nit=nmap.begin();nit!=nmap.end();nit++)
	{
		REDITABLE *ntable=nit->second;
		map<string,REDITABLE*>::iterator tit=tmap.find(ntable->name);
		if(tit==tmap.end())
		{
			if(fp) 
				fprintf(fp,"Table(%s) is new and didn't previously exist.\n",ntable->name.c_str());
			else if((!strncmp(ntable->name.c_str(),"RT",2))||(!strncmp(ntable->name.c_str(),"SL",2)))
				WSLogError("Table(%s) is new and didn't previously exist.",ntable->name.c_str());
			else
				WSLogEvent("Table(%s) is new and didn't previously exist.",ntable->name.c_str());
			ndiff++; continue;
		}
		REDITABLE *otable=tit->second;
		for(map<int,REDICOL>::iterator cit=ntable->cols.begin();cit!=ntable->cols.end();cit++)
		{
			bool found=false;
			REDICOL& ncol=cit->second;
			for(map<int,REDICOL>::iterator ocit=otable->cols.begin();ocit!=otable->cols.end();ocit++)
			{
				REDICOL& ocol=ocit->second;
				if(!stricmp(ocol.name.c_str(),ncol.name.c_str()))
				{
					found=true; break;
				}
			}
			if(!found)
			{
				if(fp) 
					fprintf(fp,"Column(%s.%s) is new and didn't previously exist.\n",ntable->name.c_str(),ncol.name.c_str());
				else if((!strncmp(ntable->name.c_str(),"RT",2))||(!strncmp(ntable->name.c_str(),"SL",2)))
					WSLogError("Column(%s.%s) is new and didn't previously exist.",ntable->name.c_str(),ncol.name.c_str());
				else
					WSLogEvent("Column(%s.%s) is new and didn't previously exist.",ntable->name.c_str(),ncol.name.c_str());
				ndiff++; continue;
			}
		}
		for(map<int,REDICOL>::iterator ocit=otable->cols.begin();ocit!=otable->cols.end();ocit++)
		{
			REDICOL& ocol=ocit->second;
			bool found=false;
			for(map<int,REDICOL>::iterator cit=ntable->cols.begin();cit!=ntable->cols.end();cit++)
			{
				REDICOL& ncol=cit->second;
				if(!stricmp(ocol.name.c_str(),ncol.name.c_str()))
				{
					found=true; break;
				}
			}
			if(!found)
			{
				if(fp) 
					fprintf(fp,"Column(%s.%s) has been deleted and no longer exists.\n",otable->name.c_str(),ocol.name.c_str());
				else if((!strncmp(ntable->name.c_str(),"RT",2))||(!strncmp(ntable->name.c_str(),"SL",2)))
					WSLogError("Column(%s.%s) has been deleted and no longer exists.",otable->name.c_str(),ocol.name.c_str());
				else
					WSLogEvent("Column(%s.%s) has been deleted and no longer exists.",otable->name.c_str(),ocol.name.c_str());
				ndiff++; continue;
			}
		}
	}
	if(fp) 
		fprintf(fp,"Total %d differences\n",ndiff);
	else 
		WSLogEvent("Total %d differences",ndiff);
	return 0;
}
// yyyymmndd to yyyy-mm-dd 00:00:00
static string SQLDATE(string col)
{
	if(col.empty())
		return (string)"";
	string dvstr=col.substr(0,8);
	int dval=atoi(dvstr.c_str());
	char dstr[256]={0};
	sprintf(dstr,"%04d-%02d-%02d 00:00:00",dval/10000,(dval%10000)/100,dval%100);
	return (string)dstr;
}
// hhmmss to yyyy-mm-dd hh:mm:ss
static string SQLTIME(int dval, string col)
{
	if(col.empty())
		return (string)"";
	string tvstr;
	if(col.length()==6)
		tvstr=col;
	else if(col.length()==14)
		tvstr=col.substr(8,6);
	else if(col.length()==17)
		tvstr=col.substr(8,6);
	int tval=atoi(tvstr.c_str());
	char tstr[256]={0};
	sprintf(tstr,"%04d-%02d-%02d %02d:%02d:%02d",
		dval/10000,(dval%10000)/100,dval%100,
		tval/10000,(tval%10000)/100,tval%100);
	return (string)tstr;
}
// yyyymmddhhmmss to yyyy-mm-dd hh:mm:ss
static string SQLTS(string col)
{
	if(col.empty())
		return (string)"";
	string dvstr=col.substr(0,8);
	string tvstr=col.substr(8,6);
	int dval=atoi(dvstr.c_str());
	int tval=atoi(tvstr.c_str());
	char tstr[256]={0};
	sprintf(tstr,"%04d-%02d-%02d %02d:%02d:%02d",
		dval/10000,(dval%10000)/100,dval%100,
		tval/10000,(tval%10000)/100,tval%100);
	return (string)tstr;
}
static string SQLDECIMAL(string col)
{
	if(col.empty())
		return (string)"0.0";
	return col;
}
#ifdef REDISQLOATS
int ViewServer::RediSqlImportOats(FILE *fp, const char *fpath)
{
	fprintf(fp,"Importing %s...\n",fpath);
	FILE *rfp=fopen(fpath,"rt");
	if(!rfp)
	{
		fprintf(fp,"Failed opening %s!\n",fpath);
		return -1;
	}

	int ConPortNo=-1;
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].SockConnected)
			continue;
		if(ConPort[i].DetPtr==(void*)PROTO_REDIOLEDB)
		{
			ConPortNo=i;
			break;
		}
	}
	if(ConPortNo<0)
	{
		fprintf(fp,"No connected $REDIOLEDB$ ports found!");
		return -1;
	}

	char rbuf[1024]={0},sql[2048]={0};
	string cols[128];
	int nsql=0,nfailed=0;
	string foreid;
	list<int> qlist;
	while(fgets(rbuf,sizeof(rbuf),rfp))
	{
		for(int c=0;c<128;c++)
			cols[c].clear();
		int col=0;
		for(char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
		{
			while((*tok)&&(isspace(*tok)))
				tok++;
			char *tptr;
			for(tptr=tok;(*tptr);tptr++)
			{
				if(*tptr==',') *tptr='.';
			}
			for(tptr--;(tptr>=tok)&&(isspace(*tptr));tptr--)
				*tptr=0;
			cols[col++]=tok;
		}
		sql[0]=0;
		if(cols[0]=="#HD#")
		{
			foreid=cols[3];
			sprintf(sql,"INSERT INTO OATSHD("
				"RecordType,VersionDescription,GenerationDate,ForeID,OSOID,UserID,Password,OrderRecvFirmMPID,CorrectionTime,IsDeleted) "
				"VALUES('%s','%s','%s','%s','%s','%s','%s','%s','%s',0)",
				cols[0].c_str(),cols[1].c_str(),SQLDATE(cols[2]).c_str(),cols[3].c_str(),cols[4].c_str(),cols[5].c_str(),cols[6].c_str(),cols[7].c_str(),SQLTS(cols[8]).c_str());
		}
		else if(cols[1]=="NW")
		{
			sprintf(sql,"INSERT INTO OATSHD("
				"RecordType,EventType,ActionType,ROEID,CorrectionTime,ResubmitFlag,OrderRecvFirmMPID,OrderRecvDate,OrderRecvOrderID,RoutingMPID,"
				"RoutedOrderID,OrderRecvTimestamp,RecvMethodCode,SymbolID,BuySell,SharesQty,LimitPrice,CustomerInstructionFlag,StopPrice,TimeInForce,"
				"ExpirationDate,ExpirationTime,DoNotReduceIncrease,FirstSpecialHandling,SecondSpecialHandling,ThirdSpecialHandling,FourthSpecialHandling,FifthSpecialHandling,RecvTerminalID,RecvDepartmentID,"
				"OrigDepartmentID,AccountType,ProgramTrading,Arbitrage,MemberType,ECNFlag,OrderCancelTimestamp,InfoBarrierID,DeskRecvTimestamp,"
				"DeskType,DeskSpecialHandling1,DeskSpecialHandling2,DeskSpecialHandling3,DeskSpecialHandling4,DeskSpecialHandling5,TMOTriggerTimestamp,NegotiatedTrade,OrderOigination,RejectedROEID,"
				"EndOfRecordMarker,IsDeleted)"
				"VALUES('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s',0)",
				cols[0].c_str(),cols[1].c_str(),cols[2].c_str(),cols[3].c_str(),cols[4].c_str(),cols[5].c_str(),cols[6].c_str(),cols[7].c_str(),cols[8].c_str(),cols[9].c_str(),
				cols[10].c_str(),cols[11].c_str(),cols[12].c_str(),cols[13].c_str(),cols[14].c_str(),cols[15].c_str(),cols[16].c_str(),cols[17].c_str(),cols[18].c_str(),cols[19].c_str(),
				cols[20].c_str(),cols[21].c_str(),cols[22].c_str(),cols[23].c_str(),cols[24].c_str(),cols[25].c_str(),cols[26].c_str(),cols[27].c_str(),cols[28].c_str(),cols[29].c_str(),
				cols[30].c_str(),cols[31].c_str(),cols[32].c_str(),cols[33].c_str(),cols[34].c_str(),cols[35].c_str(),cols[36].c_str(),cols[37].c_str(),cols[38].c_str(),cols[39].c_str(),
				cols[40].c_str(),cols[41].c_str(),cols[42].c_str(),cols[43].c_str(),cols[44].c_str(),cols[45].c_str(),cols[46].c_str(),cols[47].c_str(),cols[48].c_str(),cols[49].c_str(),
				cols[50].c_str());
		}
		else if(cols[1]=="CR")
		{
			sprintf(sql,"INSERT INTO OATSCR("
				"RecordType,EventType,ActionType,ROEID,CorrectionTime,ResubmitFlag,OrderRecvFirmMPID,ReplacedRecvDate,ReplacedOrderID,OrderRecvDate,"
				"OrderRecvOrderID,RoutingMPID,RoutedOrderID,OrderRecvTimestamp,RecvMethodCode,SymbolID,BuySell,SharesQty,LimitPrice,CustomerInstructionFlag,"
				"StopPrice,TimeInForce,ExpirationDate,ExpirationTime,DoNotReduceIncrease,FirstSpecialHandling,SecondSpecialHandling,ThirdSpecialHandling,FourthSpecialHandling,FifthSpecialHandling,"
				"RecvTerminalID,RecvDepartmentID,OrigDepartmentID,AccountType,ProgramTrading,Arbitrage,MemberType,ECNFlag,TMOTriggerTimestamp,CancelledByFlag,"
				"OrderOigination,InfoBarrierID,RejectedROEID,EndOfRecordMarker,IsDeleted)"
				"VALUES("
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s',0)",
				cols[0].c_str(),cols[1].c_str(),cols[2].c_str(),cols[3].c_str(),SQLTS(cols[4]).c_str(),cols[5].c_str(),cols[6].c_str(),SQLDATE(cols[7]).c_str(),cols[8].c_str(),SQLDATE(cols[9]).c_str(),
				cols[10].c_str(),cols[11].c_str(),cols[12].c_str(),SQLTS(cols[13]).c_str(),cols[14].c_str(),cols[15].c_str(),cols[16].c_str(),cols[17].c_str(),SQLDECIMAL(cols[18]).c_str(),cols[19].c_str(),
				SQLDECIMAL(cols[20]).c_str(),cols[21].c_str(),SQLDATE(cols[22]).c_str(),SQLTIME(atoi(cols[22].c_str()),cols[23]).c_str(),cols[24].c_str(),cols[25].c_str(),cols[26].c_str(),cols[27].c_str(),cols[28].c_str(),cols[29].c_str(),
				cols[30].c_str(),cols[31].c_str(),cols[32].c_str(),cols[33].c_str(),cols[34].c_str(),cols[35].c_str(),cols[36].c_str(),cols[37].c_str(),SQLTS(cols[38]).c_str(),cols[39].c_str(),
				cols[40].c_str(),cols[41].c_str(),SQLTS(cols[42]).c_str(),cols[43].c_str());
		}
		else if(cols[1]=="CL")
		{
			sprintf(sql,"INSERT INTO OATSCL("
				"RecordType,EventType,ActionType,ROEID,CorrectionTime,ResubmitFlag,OrderRecvFirmMPID,OrderRecvDate,OrderRecvOrderID,SymbolID,"
				"CancelTimestamp,CancelTypeFlag,CancelQty,CancelLeavesQty,CancelledByFlag,OrigBrokerDealerNumber,RejectedROEID,EndOfRecordMarker,IsDeleted)"
				"VALUES("
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s',0)",
				cols[0].c_str(),cols[1].c_str(),cols[2].c_str(),cols[3].c_str(),SQLTS(cols[4]).c_str(),cols[5].c_str(),cols[6].c_str(),SQLDATE(cols[7]).c_str(),cols[8].c_str(),cols[9].c_str(),
				SQLTS(cols[10]).c_str(),cols[11].c_str(),cols[12].c_str(),cols[13].c_str(),cols[14].c_str(),cols[15].c_str(),cols[16].c_str(),cols[17].c_str(),cols[18].c_str());
		}
		else if(cols[1]=="OR")
		{
			sprintf(sql,"INSERT INTO OATSOR("
				"RecordType,EventType,ActionType,ROEID,CorrectionTime,ResubmitFlag,OrderRecvFirmMPID,OrderRecvDate,OrderRecvOrderID,RoutingMPID,"
				"RoutedOrderID,OrderRecvTimestamp,RecvMethodCode,SymbolID,BuySell,LimitPrice,CustomerInstructionFlag,StopPrice,TimeInForce,ExpirationDate,"
				"ExpirationTime,DoNotReduceIncrease,FirstSpecialHandling,SecondSpecialHandling,ThirdSpecialHandling,FourthSpecialHandling,FifthSpecialHandling,RecvTerminalID,RecvDepartmentID,OrigDepartmentID,"
				"AccountType,ProgramTrading,Arbitrage,SentToRoutedOrderID,SendToMPID,OrderSentTimestamp,RoutedSharesQty,RoutingMethod,SpecialRoutingCondition,MemberType,"
				"Destination,ECNFlag,OrderCancelTimestamp,CancelledByFlag,SharesQty,InfoBarrierID,DeskRecvTimestamp,DeskType,DeskSpecialHandling1,DeskSpecialHandling2,"
				"DeskSpecialHandling3,DeskSpecialHandling4,DeskSpecialHandling5,TMOTimestamp,RoutedOrderTypeInd,RoutePrice,ISOInd,ShortSaleExemptInd,OrderOigination,ExchangeParticipantID,"
				"ConnectionID,RejectedROEID,EndOfRecordMarker,IsDeleted)"
				"VALUES("
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s',0)",
				cols[0].c_str(),cols[1].c_str(),cols[2].c_str(),cols[3].c_str(),SQLTS(cols[4]).c_str(),cols[5].c_str(),cols[6].c_str(),SQLDATE(cols[7]).c_str(),cols[8].c_str(),cols[9].c_str(),
				cols[10].c_str(),SQLDATE(cols[11]).c_str(),cols[12].c_str(),cols[13].c_str(),cols[14].c_str(),cols[15].c_str(),cols[16].c_str(),SQLDECIMAL(cols[17]).c_str(),cols[18].c_str(),SQLDATE(cols[19]).c_str(),
				SQLTIME(WSDate,cols[20]).c_str(),cols[21].c_str(),cols[22].c_str(),cols[23].c_str(),cols[24].c_str(),cols[25].c_str(),cols[26].c_str(),cols[27].c_str(),cols[28].c_str(),cols[29].c_str(),
				cols[30].c_str(),cols[31].c_str(),cols[32].c_str(),cols[33].c_str(),cols[34].c_str(),SQLTS(cols[35]).c_str(),cols[36].c_str(),cols[37].c_str(),cols[38].c_str(),cols[39].c_str(),
				cols[40].c_str(),cols[41].c_str(),SQLTS(cols[42]).c_str(),cols[43].c_str(),cols[44].c_str(),cols[45].c_str(),SQLTS(cols[46]).c_str(),cols[47].c_str(),cols[48].c_str(),cols[49].c_str(),
				cols[50].c_str(),cols[51].c_str(),cols[52].c_str(),SQLTS(cols[53]).c_str(),cols[54].c_str(),cols[55].c_str(),cols[56].c_str(),cols[57].c_str(),cols[58].c_str(),cols[59].c_str(),
				cols[60].c_str(),cols[61].c_str(),cols[62].c_str());
		}
		else if(cols[1]=="RT")
		{
			sprintf(sql,"INSERT INTO OATSOR("
				"RecordType,EventType,ActionType,ROEID,CorrectionTime,ResubmitFlag,OrderRecvFirmMPID,OrderRecvDate,OrderRecvOrderID,SendToMPID,"
				"RoutedOrderID,SymbolID,OrderSentTimestamp,RoutedSharesQty,RoutingMethod,SpecialRoutingCondition,OrigBrokerDealerNumber,Destination,RoutedOrderTypeInd,RoutePrice,"
				"ISOInd,ShortSaleExemptInd,ExchangeParticipantID,ConnectionID,RejectedROEID,EndOfRecordMarker,IsDeleted)"
				"VALUES("
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
				"'%s','%s','%s','%s','%s','%s',0)",
				cols[0].c_str(),cols[1].c_str(),cols[2].c_str(),cols[3].c_str(),SQLTS(cols[4]).c_str(),cols[5].c_str(),cols[6].c_str(),SQLDATE(cols[7]).c_str(),cols[8].c_str(),cols[9].c_str(),
				cols[10].c_str(),cols[11].c_str(),SQLTS(cols[12]).c_str(),cols[13].c_str(),cols[14].c_str(),cols[15].c_str(),cols[16].c_str(),cols[17].c_str(),cols[18].c_str(),SQLDECIMAL(cols[19]).c_str(),
				cols[20].c_str(),cols[21].c_str(),cols[22].c_str(),cols[23].c_str(),cols[24].c_str(),cols[25].c_str());
		}
		else if(cols[0]=="#TR#")
		{
			sprintf(sql,"INSERT INTO OATSTR("
				"ForeID,RecordType,RecordCount,CorrectionTime,IsDeleted) "
				"VALUES('%s','%s','%s','%s',0)",
				foreid.c_str(),cols[0].c_str(),cols[1].c_str(),SQLTS(cols[2]).c_str());
		}

		if(sql[0])
		{
			nsql++;
			int qid=VSRediQuery(ConPortNo,sql,-1,-1,0,(void*)nsql);
			if(qid>0)
				qlist.push_back(qid);
		}
	}
	fclose(rfp);

	for(list<int>::iterator qit=qlist.begin();qit!=qlist.end();qit++)
	{
		int qid=*qit;
		if(qid>0)
		{
			if(VSWaitRediQuery(ConPortNo,10,qid,true)<0)
			{
				fprintf(fp,"Failed SQL: %s\n",sql);	nfailed++;
			}
		}
	}
	qlist.clear();
	fprintf(fp,"Total %d inserts, %d failed\n",nsql,nfailed);
	return 0;
}
#endif
int ViewServer::GenRediDestinationCsv(const char *opath, const char *dpath)
{
	char tpath[MAX_PATH]={0};
	char rbuf[1024]={0};
	map<int,string> negmap;
	sprintf(tpath,"%s\\data\\ENUM_NEG.csv",pcfg->RunPath.c_str());
	FILE *fp=fopen(tpath,"rt");
	if(fp)
	{
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0;
			const char *tag="";
			int value=0;
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)
				{
				case 6: tag=tok; break;
				case 7: value=atoi(tok); break;
				};
			}
			if(!stricmp(tag,"Tag"))
				continue;
			negmap[value]=tag;
		}
		fclose(fp);
	}

	sprintf(tpath,"%s\\data\\ENUM_EXCHANGE.csv",pcfg->RunPath.c_str());
	map<int,string> exchmap;
	fp=fopen(tpath,"rt");
	if(fp)
	{
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0;
			const char *tag="";
			int value=0;
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)
				{
				case 6: tag=tok; break;
				case 7: value=atoi(tok); break;
				};
			}
			if(!stricmp(tag,"Tag"))
				continue;
			exchmap[value]=tag;
		}
		fclose(fp);
	}

	sprintf(tpath,"%s\\data\\BROKERDMA.csv",pcfg->RunPath.c_str());
	map<string,int> dmamap;
	fp=fopen(tpath,"rt");
	if(fp)
	{
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0;
			const char *destid="";
			int altdestid=0;
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)
				{
				case 4: destid=tok; break;
				case 7: altdestid=atoi(tok); break;
				};
			}
			if(!stricmp(destid,"DestinationId"))
				continue;
			dmamap[destid]=altdestid;
		}
		fclose(fp);
	}

	fp=fopen(dpath,"rt");
	if(fp)
	{
		FILE *wfp=fopen(opath,"wt");
		if(wfp)
		{
			fprintf(wfp,"DestinationEnumId,ShortAlias,LongAlias,Broker,Exchange,ExecBrokerMPID,\n");
			while(fgets(rbuf,sizeof(rbuf),fp))
			{
				int col=0;
				const char *enumid="",*broker="",*route="",*shortalias="",*longalias="",*execbrokermpid="";
				int brokerid=0,deskenum=0;
				for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
				{
					switch(++col)
					{
					case 4: enumid=tok; break;
					case 5: brokerid=atoi(tok); break;
					case 6: deskenum=atoi(tok); break;
					case 7: broker=tok; break;
					case 8: route=tok; break;
					case 10: shortalias=tok; break;
					case 11: longalias=tok; break;
					case 12: execbrokermpid = tok; break;
					};
				}
				if(!stricmp(enumid,"DestinationEnumId"))
					continue;
				if(brokerid==-1)
				{
					const char *dmadest=route;
					map<int,string>::iterator eit=exchmap.find(atoi(enumid));
					if(eit!=exchmap.end())
						dmadest=eit->second.c_str();
					fprintf(wfp,"%s,%s,%s,%s,%s,%s,\n",enumid,shortalias,longalias,broker,dmadest,execbrokermpid);
				}
				else if(brokerid>0)
				{
					if(!stricmp(route,"ALGO"))
					{
						fprintf(wfp,"%s,%s,%s,%s,%s ALGO,%s,\n",enumid,shortalias,longalias,broker,broker,execbrokermpid);
					}
					else if(!stricmp(route,"DESK"))
					{
						const char *desktype="DESK";
						map<int,string>::iterator nit=negmap.find(deskenum);
						if(nit!=negmap.end())
							desktype=nit->second.c_str();
						fprintf(wfp,"%s,%s,%s,%s,%s %s,%s,\n",enumid,shortalias,longalias,broker,broker,desktype,execbrokermpid);
					}
					else if(!stricmp(route,"DMA"))
					{
						const char *dmadest="DMA";
						map<string,int>::iterator dit=dmamap.find(enumid);
						if(dit!=dmamap.end())
						{
							map<int,string>::iterator eit=exchmap.find(dit->second);
							if(eit!=exchmap.end())
								dmadest=eit->second.c_str();
						}
						fprintf(wfp,"%s,%s,%s,%s,%s %s,%s,\n",enumid,shortalias,longalias,broker,broker,dmadest,execbrokermpid);
					}
				}
			}
			fclose(wfp);
		}
		fclose(fp);
	}
	return 0;
}
typedef vector<char *> RTSYMROW;
static const char *GetColValue(map<string,int>& cols, RTSYMROW& row, const char *field)
{
	map<string,int>::iterator cit=cols.find(field);
	if(cit==cols.end())
		return "";
	if(cit->second>=(int)row.size())
		return "";
	return row[cit->second];
}
int ViewServer::GenTsxList(const char *opath, const char *dpath)
{
	char tpath[MAX_PATH]={0};
	char rbuf[1024]={0};
	FILE *fp=fopen(dpath,"rt");
	if(!fp)
	{
		WSLogError("GenTsxList failed reading %s",dpath);
		return -1;
	}

	FILE *wfp=fopen(opath,"wt");
	if(!wfp)
	{
		WSLogError("GenTsxList failed writing %s",opath);
		return -1;
	}

	map<string,int> cols;
	map<string,RTSYMROW*> isinmap;
	map<string,string> ca_us_map;
	int lno=0;
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(*rptr!='\r')&&(*rptr!='\n');rptr++)
			;
		if(rptr) *rptr=0;
		if((!*rbuf)||(!strncmp(rbuf,"//",2)))
			continue;

		RTSYMROW row;
		for(char *tok=strtoke(rbuf,",");tok;tok=strtoke(0,","))
			row.push_back(tok);

		// Column map from the header row
		if(cols.empty())
		{
			for(int c=0;c<(int)row.size();c++)
				cols.insert(pair<string,int>(row[c],c));
			continue;
		}

		string cc=GetColValue(cols,row,"CountryCode");
		if((cc!="US")&&(cc!="CA"))
			continue;

		string prodtype=GetColValue(cols,row,"ProductType");
		if(prodtype!="STOCK")
			continue;

		string isin=GetColValue(cols,row,"Isin");
		if(isin.empty())
			continue;

		string symbol=GetColValue(cols,row,"Symbol");
		if(symbol.substr(0,2)=="R.")
			symbol=symbol.substr(2);

		map<string,RTSYMROW*>::iterator iit=isinmap.find(isin);
		if(iit==isinmap.end())
		{
			RTSYMROW *rowcopy=new RTSYMROW;
			for(RTSYMROW::iterator rit=row.begin();rit!=row.end();rit++)
				rowcopy->push_back(strdup(*rit));
			isinmap.insert(pair<string,RTSYMROW*>(isin,rowcopy));
		}
		else
		{
			string ca_sym,us_sym;
			if(cc=="CA") ca_sym=symbol;
			else if(cc=="US") us_sym=symbol;

			RTSYMROW& match=*iit->second;
			string msymbol=GetColValue(cols,match,"Symbol");
			if(msymbol.substr(1,2)=="R.")
				msymbol=msymbol.substr(3);
			string mcc=GetColValue(cols,match,"CountryCode");
			if(mcc=="CA") ca_sym=msymbol;
			else if(mcc=="US") us_sym=msymbol;
					
			if((!ca_sym.empty())&&(!us_sym.empty()))
				ca_us_map.insert(pair<string,string>(ca_sym,us_sym));
		}
	}

	fprintf(wfp,"//CA_sym,US_sym\n");
	for(map<string,string>::iterator sit=ca_us_map.begin();sit!=ca_us_map.end();sit++)
		fprintf(wfp,"%s,%s\n",sit->first.c_str(),sit->second.c_str());			
	fprintf(wfp,"//%d symbols\n",ca_us_map.size());
	fclose(wfp);
	fclose(fp);
	WSLogEvent("GenTsxList wrote %d symbols to %s",ca_us_map.size(),opath);

	ca_us_map.clear();
	for(map<string,RTSYMROW*>::iterator iit=isinmap.begin();iit!=isinmap.end();iit++)
	{
		RTSYMROW *rowcopy=iit->second;
		for(RTSYMROW::iterator rit=rowcopy->begin();rit!=rowcopy->end();rit++)
			free(*rit);
		delete rowcopy;
	}
	isinmap.clear();
	cols.clear();
	return 0;
}
int ViewServer::GenEUList(const char *opath, const char *dpath)
{
	char tpath[MAX_PATH]={0};
	char rbuf[1024]={0};
	FILE *fp=fopen(dpath,"rt");
	if(!fp)
	{
		WSLogError("GenEUList failed reading %s",dpath);
		return -1;
	}

	FILE *wfp=fopen(opath,"wt");
	if(!wfp)
	{
		WSLogError("GenEUList failed writing %s",opath);
		return -1;
	}

	map<string,int> cols;
	multimap<string,RTSYMROW*> isinmap;
	map<string,string> eu_us_map;
	int lno=0;
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		lno++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(*rptr!='\r')&&(*rptr!='\n');rptr++)
			;
		if(rptr) *rptr=0;
		if((!*rbuf)||(!strncmp(rbuf,"//",2)))
			continue;

		RTSYMROW row;
		for(char *tok=strtoke(rbuf,",");tok;tok=strtoke(0,","))
			row.push_back(tok);

		// Column map from the header row
		if(cols.empty())
		{
			for(int c=0;c<(int)row.size();c++)
				cols.insert(pair<string,int>(row[c],c));
			continue;
		}

		string cc=GetColValue(cols,row,"CountryCode");

		string prodtype=GetColValue(cols,row,"ProductType");
		if(prodtype!="STOCK")
			continue;

		string isin=GetColValue(cols,row,"Isin");
		if(isin.empty())
			continue;

		string symbol=GetColValue(cols,row,"Symbol");
		if(symbol.substr(0,2)=="R.")
			symbol=symbol.substr(2);

		RTSYMROW *rowcopy=new RTSYMROW;
		for(RTSYMROW::iterator rit=row.begin();rit!=row.end();rit++)
			rowcopy->push_back(strdup(*rit));
		isinmap.insert(pair<string,RTSYMROW*>(isin,rowcopy));
	}

	for(multimap<string,RTSYMROW*>::iterator iit=isinmap.begin();iit!=isinmap.end();)
	{		
		string eu_sym,us_sym;
		RTSYMROW *usrow=0;
		multimap<string,RTSYMROW*>::iterator mit;
		for(mit=iit;(mit!=isinmap.end())&&(mit->first==iit->first);mit++)
		{
			string mcc=GetColValue(cols,*mit->second,"CountryCode");
			if(mcc=="US")
			{
				usrow=mit->second;
				string symbol=GetColValue(cols,*mit->second,"Symbol");
				if(symbol.substr(0,2)=="R.")
					symbol=symbol.substr(2);
				us_sym=symbol;
			}
		}
		for(mit=iit;(mit!=isinmap.end())&&(mit->first==iit->first);mit++)
		{
			string mcc=GetColValue(cols,*mit->second,"CountryCode");
			if(mcc!="US")
			{
				string symbol=GetColValue(cols,*mit->second,"Symbol");
				if(symbol.substr(0,2)=="R.")
					symbol=symbol.substr(2);
				eu_sym=symbol;
				if((!eu_sym.empty())&&(!us_sym.empty()))
					eu_us_map.insert(pair<string,string>(eu_sym,us_sym));
			}
		}
		iit=mit;
	}

	fprintf(wfp,"//EU_sym,US_sym\n");
	for(map<string,string>::iterator sit=eu_us_map.begin();sit!=eu_us_map.end();sit++)
		fprintf(wfp,"%s,%s\n",sit->first.c_str(),sit->second.c_str());			
	fprintf(wfp,"//%d symbols\n",eu_us_map.size());
	fclose(wfp);
	fclose(fp);
	WSLogEvent("GenEUList wrote %d symbols to %s",eu_us_map.size(),opath);

	eu_us_map.clear();
	for(multimap<string,RTSYMROW*>::iterator iit=isinmap.begin();iit!=isinmap.end();iit++)
	{
		RTSYMROW *rowcopy=iit->second;
		for(RTSYMROW::iterator rit=rowcopy->begin();rit!=rowcopy->end();rit++)
			free(*rit);
		delete rowcopy;
	}
	isinmap.clear();
	cols.clear();
	return 0;
}
// Query from the RTPOSITION table for all unique firm ids in the file
int ViewServer::QueryRediPositions(const char *dpath)
{
	char tpath[MAX_PATH]={0};
	char rbuf[1024]={0};
	map<int,int> parentidmap;
	sprintf(tpath,"%s\\data\\RTFIRM_PARENTID.csv",pcfg->RunPath.c_str());
	FILE *fp=fopen(tpath,"rt");
	if(fp)
	{
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0;
			int id=0,parentid=0;
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)
				{
				case 4: id=atoi(tok); break;
				case 5: parentid=atoi(tok); break;
				};
			}
			if(!id)
				continue;
			parentidmap[id]=parentid;
		}
		fclose(fp);
	}

	set<int> firmset;
	fp=fopen(dpath,"rt");
	if(fp)
	{
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0;
			int firmid=0;
			const char *name="";
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				switch(++col)
				{
				case 4: firmid=atoi(tok); break;
				case 5: name=tok; break;
				};
			}
			if(!firmid)
				continue;
			// Only query topmost parent firms
			while(firmid)
			{
				map<int,int>::iterator pit=parentidmap.find(firmid);
				if(pit!=parentidmap.end())
				{
					if(pit->second==firmid)
						break;
					firmid=pit->second;
				}
				else
					break;
			}
			// Only query parent firms once
			if(firmset.find(firmid)==firmset.end())
			{
				firmset.insert(firmid);
				char ssql[256]={0},lfile[MAX_PATH]={0};
				sprintf(ssql,"select * from RTPOSITION where RtFirmId=%d",firmid);
				sprintf(lfile,"%s\\data\\RTPOSITION_%d.csv",pcfg->RunPath.c_str(),firmid);
				REDIBUSTASK *ptd=new REDIBUSTASK;
				ptd->pmod=this;
				ptd->localFile=lfile;
				ptd->qtmax=60;
				ptd->sql=ssql;
				ptd->taskid=++nextTaskId;
				ptd->rc=0;
				bool weekdays[]={0,1,1,1,1,1,0};
				// Non-repeating immmediate task
				SCHEDENTRY *ptask=(SCHEDENTRY *)taskmgr.AddScheduledTask(&taskUser,TASK_REDIBUS,ptd,_RedibusTask,WSDate,WSTime,false,true,weekdays,WSDate,WSTime);
				if(ptask)
					runNowTaskList.push_back(ptask);
			}
		}
		fclose(fp);
	}
	return 0;
}
// Identify and copy useful lines from RediOleDB log
int ViewServer::ImportRediOleDBLog(int wsdate, int tstart, int tstop)
{
	// Adding dependency to advapi32.dll causes the entire viewserver.dll to fail to load?
	//const char *keypath="HKEY_CURRENT_USER\\Software\\Spear Leeds & Kellogg\\RediOleDbProvider\\Settings";
	//const char *keyname="Log Directory";
	//HKEY hkey=0;
	//if(RegOpenKey(HKEY_CURRENT_USER,keypath,&hkey)==ERROR_SUCCESS)
	//{
	//	char lpath[MAX_PATH]={0};
	//	DWORD lsize=MAX_PATH;
	//	RegGetValue(hkey,0,keyname,RRF_RT_REG_SZ,0,lpath,&lsize);
	//	REDIOLEDB_PATH=lpath;
	//	RegCloseKey(hkey);
	//}

	char fmatch[MAX_PATH]={0};
	sprintf(fmatch,"%s\\RediOledb????.idx",REDIOLEDB_PATH.c_str());
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(fmatch,&fdata);
	if(fhnd==INVALID_HANDLE_VALUE)
		return -1;
	do{
		if((!strcmp(fdata.cFileName,"."))||(!strcmp(fdata.cFileName,"..")))
			;
		else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			;
		else
		{
			FILETIME lft;
			FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&lft);
			SYSTEMTIME tsys;
			FileTimeToSystemTime(&lft,&tsys);
			int fdate=(tsys.wYear*10000) +(tsys.wMonth*100) +(tsys.wDay);
			//int ftime=(tsys.wHour*10000) +(tsys.wMinute*100) +(tsys.wSecond);
			//if((fdate==wsdate)&&(ftime>=tstart))
			if(fdate==wsdate)
			{
				char fpath[MAX_PATH]={0};
				sprintf(fpath,"%s\\%s",REDIOLEDB_PATH.c_str(),fdata.cFileName);
				FILE *fp=fopen(fpath,"rt");
				if(fp)
				{
					char rbuf[1024]={0};
					fgets(rbuf,sizeof(rbuf),fp);
					if(stristr(rbuf,"ViewServer_iqm.exe")
					#ifdef _DEBUG
					   ||strstr(rbuf,"IQMatrixD.exe")
					#endif
					  )
					{
						bool found=false;
						while(fgets(rbuf,sizeof(rbuf),fp))
						{
							if((strstr(rbuf,"CheckConnectionEvtThrd"))||
							   (strstr(rbuf," Connecting("))||
							   (strstr(rbuf," Connected("))||
							   (strstr(rbuf," Client Info:"))||
							   (strstr(rbuf," CreateSession:")))							   
							{
								int ldate=(wsdate/10000)*10000 +(atoi(rbuf)*100) +(atoi(rbuf+3));
								int ltime=(atoi(rbuf+6)*10000) +(atoi(rbuf+9)*100) +(atoi(rbuf+12));
								if((ldate==wsdate)&&(ltime>=tstart)&&(ltime<=tstop))
								{
									if(!found)
									{
										WSLogEvent("Found RediOleDB log at %s",fpath); found=true;
									}
									char *rptr=strchr(rbuf,'\r');
									if(!rptr) rptr=strchr(rbuf,'\n');
									if(rptr) *rptr=0;
									WSLogEvent("RediOleDB: %s",rbuf);
								}
							}
						}
						if(found)
						{
							fclose(fp);
							FindClose(fhnd);
							return 0;
						}
					}
					fclose(fp);
				}
			}
		}
	}while(FindNextFile(fhnd,&fdata));
	FindClose(fhnd);
	return -1;
}
int ViewServer::LoadDoneAwayUsersIni()
{
	for(DAMAP::iterator dit=damap.begin();dit!=damap.end();dit++)
	{
		DoneAwayLookup& dalu=dit->second;
		WSLogEvent("Loading done away users for %s...",dit->first.c_str());
		map<string,string> namap;
		FILE *fp=fopen(dalu.DONE_AWAY_USERS_PATH.c_str(),"rt");
		if(!fp)
		{
			WSLogError("Failed opening %s!",dalu.DONE_AWAY_USERS_PATH.c_str());
			continue;
		}
		char rbuf[2048]={0};
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0,ascol=0,aicol=0;
			if(!strncmp(rbuf,"//",2))
				continue;
			const char *domain=strtoke(rbuf,":\r\n");
			const char *userid=domain ?strtoke(0,"\r\n") :"";
			namap[domain]=userid;
		}
		fclose(fp);

		dalu.doneAwayUserMap.clear();
		dalu.doneAwayUserMap.swap(namap);
		WSLogEvent("Loaded %d users from %s",dalu.doneAwayUserMap.size(),dalu.DONE_AWAY_USERS_PATH.c_str());
	}
	return 0;
}
int ViewServer::LoadDoneAwayAccountsCsv()
{	
	for(DAMAP::iterator dit=damap.begin();dit!=damap.end();dit++)
	{
		DoneAwayLookup& dalu=dit->second;
		map<string,string> namap;
		WSLogEvent("Loading done away accounts for %s...",dit->first.c_str());
		FILE *fp=fopen(dalu.DONE_AWAY_ACCOUNTS_PATH.c_str(),"rt");
		if(!fp)
		{
			WSLogError("Failed opening %s!",dalu.DONE_AWAY_ACCOUNTS_PATH.c_str());
			continue;
		}
		char rbuf[2048]={0};
		int ascol=0,aicol=0,delcol=0;
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			int col=0;
			if(!strncmp(rbuf,"select ",7))
				continue;
			else if(!strncmp(rbuf,"Row,",4))
			{
				for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
				{
					col++;
					if(!stricmp(tok,"AccountSynonym"))
						ascol=col;
					else if(!stricmp(tok,"AccountId"))
						aicol=col;
					else if(!stricmp(tok,"IsDeleted"))
						delcol=col;
				}			
			}
			else
			{
				const char *asyn="",*aid="",*del="";
				for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
				{
					col++;
					if(col==ascol)
						asyn=tok;
					else if(col==aicol)
						aid=tok;
					else if(col==delcol)
						del=tok;
				}
				if(strcmp(del,"1")!=0)
					namap[aid]=asyn;
			}
		}
		fclose(fp);

		dalu.doneAwayAccountMap.clear();
		dalu.doneAwayAccountMap.swap(namap);
		WSLogEvent("Loaded %d accounts from %s",dalu.doneAwayAccountMap.size(),dalu.DONE_AWAY_ACCOUNTS_PATH.c_str());
	}
	return 0;
}
int ViewServer::LoadTUserMapCsv()
{	
	TUSERMAP tdomainmap;
	if(!TUSERDOMAIN_INI.empty())
	{
		char dpath[MAX_PATH]={0};
		strcpy(dpath,TUSERDOMAIN_INI.c_str());
		if((dpath[0]!='\\')&&(dpath[1]!=':'))
			sprintf(dpath,"%s\\%s",pcfg->RunPath.c_str(),TUSERDOMAIN_INI.c_str());
		WSLogEvent("Loading domain mapping from %s...",dpath);
		FILE *fp=fopen(dpath,"rt");
		if(!fp)
			WSLogError("Failed opening %s!",dpath);
		else
		{
			TUSERMAP ndomainmap;
			char rbuf[2048]={0};
			while(fgets(rbuf,sizeof(rbuf),fp))
			{
				if(!strncmp(rbuf,"//",2))
					continue;
				const char *firmid="",*domain="";
				int col=0;
				for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
				{
					col++;
					switch(col)
					{
					case 1: firmid=tok; break;
					case 2: domain=tok; break;
					};
				}
				tdomainmap[firmid]=domain;
			}
			fclose(fp);
			WSLogEvent("Loaded %d domains from %s",tdomainmap.size(),dpath);
		}
	}

	if(TUSERMAP_PATH.empty())
		return 0;
	char upath[MAX_PATH]={0};
	strcpy(upath,TUSERMAP_PATH.c_str());
	if((upath[0]!='\\')&&(upath[1]!=':'))
		sprintf(upath,"%s\\%s",pcfg->RunPath.c_str(),TUSERMAP_PATH.c_str());
	WSLogEvent("Loading user mapping from %s...",upath);
	FILE *fp=fopen(upath,"rt");
	if(!fp)
	{
		WSLogError("Failed opening %s!",upath);
		return -1;
	}
	TUSERMAP nusermap;
	char rbuf[2048]={0};
	int fcol=0,ucol=0;
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		int col=0;
		if((!strncmp(rbuf,"select ",7))||(!strncmp(rbuf,"//",2))||(*rbuf=='\r')||(*rbuf=='\n'))
			continue;
		else if(!strncmp(rbuf,"Row,",4))
		{
			for(const char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				col++;
				if(!stricmp(tok,"FirmId"))
					fcol=col;
				else if(!stricmp(tok,"UserId"))
					ucol=col;
			}			
		}
		else
		{
			const char *firmid="";
			char *userid="";
			for(char *tok=strtoke(rbuf,",\r\n");tok;tok=strtoke(0,",\r\n"))
			{
				col++;
				if(col==fcol)
					firmid=tok;
				else if(col==ucol)
					userid=tok;
			}
			string domain=firmid;
			TUSERMAP::iterator dit=tdomainmap.find(firmid);
			if(dit!=tdomainmap.end())
				domain=dit->second.c_str();
			_strupr(userid);
			nusermap[userid]=domain;
		}
	}
	fclose(fp);
	tusermap.clear();
	tusermap.swap(nusermap);
	WSLogEvent("Loaded %d users from %s",tusermap.size(),upath);
	return 0;
}
int _stdcall _RTOatsTask(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint)
{
	ViewServer *pmod=(ViewServer*)udata;
	return pmod ?pmod->RTOatsTask() :-1;
}
int ViewServer::RTOatsTask()
{
	WSLogEvent("RTOATS_IMPORT_TIME expired");
	for(int i=0;i<NO_OF_CON_PORTS;i++)
	{
		if(!ConPort[i].InUse)
			continue;
		if(strstr(ConPort[i].CfgNote,"$RTOATS$"))
		{
			if(ConPort[i].ConnectHold)
			{
				char *dstr=strstr(ConPort[i].CfgNote,"yyyymmdd");
				if(dstr)
					memcpy(dstr,WScDate,8);
				WSLogEvent("Connecting CON%d: %s...",i,ConPort[i].CfgNote);
				ConPort[i].ConnectHold=false;
			}
		}
	}
	return 0;
}
#endif//REDIOLEDBCON
