#include "stdafx.h"
#include "smtask.h"
using namespace sysmon;
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

using namespace ::testing;

namespace viewserver_unit_tests
{

class WsocksHostTester :public WsocksHostImpl
{
public:
	WsocksHostTester()
		:config()
	{
		char cdir[MAX_PATH]={0},ldir[MAX_PATH]={0};
		GetCurrentDirectory(sizeof(cdir),cdir);
		sprintf(ldir,"%s\\logs",cdir);
		WSHInitHost(ldir);
	}
	~WsocksHostTester()
	{
		WSHCleanupHost();
	}

	int Load(WsocksApp *wsapp)
	{
		char cdir[MAX_PATH]={0},ldir[MAX_PATH]={0},sdir[MAX_PATH]={0};
		GetCurrentDirectory(sizeof(cdir),cdir);
		sprintf(ldir,"%s\\logs",cdir);
		sprintf(sdir,"%s\\setup\\setup.ini",cdir);
		config.RunPath=cdir;
		config.LogPath=ldir;
		config.ConfigPath=sdir;

		wsapp->phost=this;
		wsapp->pcfg=&config;
		if(wsapp->WSHInitModule(wsapp->pcfg,wsapp->phost)<0)
			return -1;
		return 0;
	}
	int Unload(WsocksApp *wsapp)
	{
		return wsapp->WSHCleanupModule();
	}

protected:
	// WsocksHostImpl notifications
	// App loading
	virtual int WSHAppLoading(WsocksApp *pmod){ return 0; }
	virtual int WSHAppLoadFailed(WsocksApp *pmod){ return 0; }
	virtual int WSHAppLoaded(WsocksApp *pmod){ return 0; }
	virtual int WSHAppUnloaded(WsocksApp *pmod){ return 0; }

	// Port creation
	virtual int WSHPortCreated(WsocksApp *pmod, WSPort *pport){ return 0; }
	virtual int WSHPortModified(WsocksApp *pmod, WSPort *pport){ return 0; }
	virtual int WSHPortDeleted(WsocksApp *pmod, WSPort *pport){ return 0; }

	// Logging
	virtual int WSHLoggedEvent(WsocksApp *pmod, const char *estr){ return 0; }
	virtual int WSHLoggedError(WsocksApp *pmod, const char *estr){ return 0; }
	virtual int WSHLoggedDebug(WsocksApp *pmod, const char *estr){ return 0; }
	virtual int WSHLoggedRetry(WsocksApp *pmod, const char *estr){ return 0; }
	virtual int WSHLoggedRecover(WsocksApp *pmod, const char *estr){ return 0; }
	virtual int WSHHostLoggedEvent(const char *estr){ return 0; }
	virtual int WSHHostLoggedError(const char *estr){ return 0; }

	// Show/hide UI window
	virtual int WSHShowWindow(int nCmdShow){ return 0; }

	// Status
	virtual void WSHSetAppHead(WsocksApp *pmod, const char *heading){}
	virtual void WSHSetAppStatus(WsocksApp *pmod, const char *status){}
	virtual int WSHGetAppHead(WsocksApp *pmod, char *heading, int hlen){ return 0; }
	virtual int WSHGetAppStatus(WsocksApp *pmod, char *status, int slen){ return 0; }

	virtual int WSHExitApp(WsocksApp *pmod, int ecode){ return 0; }
	virtual int WSHExitHost(int ecode){ return 0; }

	AppConfig config;
};

class ViewServerTester :public ViewServer
{
public:
	ViewServerTester()
		:ViewServer()
		,wshost()
	{
		wshost.Load(this);
		while(WSSuspendMode)
			SleepEx(100,true);
	}
	void Unload()
	{
		wshost.Unload(this);
	}

	virtual int WSPortsCfg()
	{
		int uscPortNo=GetTestUscPort();
		strcpy(UscPort[uscPortNo].CfgNote,"$VSDIST$REDIRPT");
		return 1;
	}
	inline AppSystem *GetTestSystem()
	{
		return GetAppSystem("TWIST*");
	}
	inline int GetTestUscPort()
	{
		return 1;
	}
	inline VSOrder *CreateOrder(int UscPortNo, AppSystem *asys, const char *okey, bool& isnew, ITEMLOC& oloc)
	{
		return __super::CreateOrder(UscPortNo,asys,okey,isnew,oloc);
	}
	inline int CommitOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc)
	{
		return __super::CommitOrder(UscPortNo,asys,porder,oloc);
	}
	inline int UnloadOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc)
	{
		return __super::UnloadOrder(UscPortNo,asys,porder,oloc);
	}
	inline int ModifyOrder(const char *appinstid, const char *okey, int wsdate,
		const char *account, const char *clientid, const char *symbol,
		const char *nappinstid, const char *newaccount, const char *newrootorderid, const char *newfirstclordid,
		const char *newsymbol)
	{
		return __super::ModifyOrder(appinstid,okey,wsdate,account,clientid,symbol,
			nappinstid,newaccount,newrootorderid,newfirstclordid,newsymbol);
	}

protected:
	WsocksHostTester wshost;
};

class ViewServerTests :public Test
{
public:
	ViewServerTests() 
		:Test()
	{
	}
	~ViewServerTests()
	{
		FreeTargets();
	}

	typedef ViewServerTester *Target;
	Target CreateTarget()
	{
		Target target=new ViewServerTester();
		targetList.push_back(target);
		return target;
	}

protected:
	list<Target> targetList;
	void FreeTargets()
	{
		for(list<Target>::iterator tit=targetList.begin();tit!=targetList.end();tit++)
			(*tit)->Unload();
		targetList.clear();
	}
};

// VSOrder *ViewServer::CreateOrder(int UscPortNo, AppSystem *asys, const char *okey, bool& isnew, ITEMLOC& oloc);
TEST_F(ViewServerTests, CreateOrder_any_returnsOrder)
{
	//Arrange
	Target target(CreateTarget());
	char okey[64];
	sprintf(okey,"%08d.YB24601",target->WSDate);
	bool isnew=false;
	ITEMLOC oloc;
	oloc.offset=0;

	//Act
	VSOrder *actual=target->CreateOrder(target->GetTestUscPort(),target->GetTestSystem(),okey,isnew,oloc);

	//Assert
	ASSERT_NE(0,(int)actual);
}
// int ViewServer::CommitOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc);
TEST_F(ViewServerTests, CommitOrder_simpleOrder_returnsSuccess)
{
	//Arrange
	Target target(CreateTarget());
	char okey[64];
	sprintf(okey,"%08d.YB24601",target->WSDate);
	bool isnew=false;
	ITEMLOC oloc;
	oloc.offset=0;
	VSOrder *vsorder=target->CreateOrder(target->GetTestUscPort(),target->GetTestSystem(),okey,isnew,oloc);
	vsorder->SetAppInstID("BNPSTG");
	vsorder->SetClOrdID("YB24601");
	vsorder->SetClientID("gtest");
	vsorder->SetSymbol("BAC");
	vsorder->SetPrice(18.01);
	vsorder->SetSide('1');
	vsorder->SetOrderQty(100);

	//Act
	int actual=target->CommitOrder(target->GetTestUscPort(),target->GetTestSystem(),vsorder,oloc);

	//Assert
	ASSERT_EQ(0,actual);
}
// int ViewServer::UnloadOrder(int UscPortNo, AppSystem *asys, VSOrder *porder, ITEMLOC& oloc);
TEST_F(ViewServerTests, UnloadOrder_emptyOrder_returnsSuccess)
{
	//Arrange
	Target target(CreateTarget());
	char okey[64];
	sprintf(okey,"%08d.YB24601",target->WSDate);
	bool isnew=false;
	ITEMLOC oloc;
	oloc.offset=0;
	VSOrder *vsorder=target->CreateOrder(target->GetTestUscPort(),target->GetTestSystem(),okey,isnew,oloc);

	//Act
	int actual=target->UnloadOrder(target->GetTestUscPort(),target->GetTestSystem(),vsorder,oloc);

	//Assert
	ASSERT_EQ(0,actual);
}
// int ViewServer::ModifyOrder(const char *appinstid, const char *okey, int wsdate,
//		const char *account, const char *clientid, const char *symbol,
//		const char *nappinstid, const char *newaccount, const char *newrootorderid, const char *newfirstclordid,
//		const char *newsymbol);
TEST_F(ViewServerTests, ModifyOrder_changeSymbol_returnsSuccess)
{
	//Arrange
	Target target(CreateTarget());
	char okey[64];
	sprintf(okey,"%08d.YB24601",target->WSDate);
	bool isnew=false;
	ITEMLOC oloc;
	oloc.offset=0;
	VSOrder *vsorder=target->CreateOrder(target->GetTestUscPort(),target->GetTestSystem(),okey,isnew,oloc);
	const char *appinstid="BNPSTG";
	vsorder->SetAppInstID(appinstid);
	vsorder->SetClOrdID("YB24601");
	vsorder->SetClientID("gtest");
	vsorder->SetSymbol("BAC");
	vsorder->SetPrice(18.01);
	vsorder->SetSide('1');
	vsorder->SetOrderQty(100);
	target->CommitOrder(target->GetTestUscPort(),target->GetTestSystem(),vsorder,oloc);
	target->UnloadOrder(target->GetTestUscPort(),target->GetTestSystem(),vsorder,oloc);

	//Act
	int actual=target->ModifyOrder(appinstid,okey,target->WSDate,vsorder->GetAccount(),vsorder->GetClientID(),vsorder->GetSymbol(),
		appinstid,vsorder->GetAccount(),vsorder->GetRootOrderID(),vsorder->GetFirstClOrdID(),"ABC");

	//Assert
	ASSERT_EQ(1,actual);
}

}
