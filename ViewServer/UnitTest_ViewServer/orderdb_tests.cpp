#include "stdafx.h"

using namespace ::testing;

namespace viewserver_unit_tests
{

class OrderDBTester :public OrderDB
{
public:
};

class OrderDBTests :public Test, protected OrderDBNotify
{
public:
	OrderDBTests() 
		:Test()
		,cntUnloadNotify(0)
	{
	}
	~OrderDBTests()
	{
		FreeTargets();
	}

	typedef OrderDBTester *Target;
	Target CreateTarget()
	{
		Target target=new OrderDBTester();
		target->HAVE_FUSION_IO=true;
		//target->Init(this);
		char cdir[MAX_PATH]={0},ddir[MAX_PATH]={0};
		GetCurrentDirectory(sizeof(cdir),cdir);
		sprintf(ddir,"%s\\data",cdir);
		bool cxlInit=false;
		SYSTEMTIME tloc;
		GetLocalTime(&tloc);
		int wsdate=(tloc.wYear*10000) +(tloc.wMonth*100) +(tloc.wDay);
		char dbname[64];
		sprintf(dbname,"TEST_%08d.dat",wsdate);
		AppSystem asys;
		target->Init(this,ddir,dbname,wsdate,false,cxlInit,&asys);
		targetList.push_back(target);
		return target;
	}
	int GetUnloadNotifyCount()
	{
		return cntUnloadNotify;
	}

protected:
	list<Target> targetList;
	void FreeTargets()
	{
		for(list<Target>::iterator tit=targetList.begin();tit!=targetList.end();tit++)
		{
			Target target=*tit;
			target->Shutdown();
			delete target;
		}
		targetList.clear();
	}

	int cntUnloadNotify;
	// OrderDBNotify
	virtual void ODBError(int ecode, const char *emsg){};
	virtual void ODBEvent(int ecode, const char *emsg){};
	virtual void ODBIndexOrder(class AppSystem *asys, OrderDB *pdb, VSOrder *porder, LONGLONG offset){};
	virtual void ODBBusyLoad(LONGLONG offset, int lperc){};
	virtual void ODBOrderUnloaded(VSOrder *porder, LONGLONG offset){ cntUnloadNotify++; };
};

// VSOrder *OrderDB::AllocOrder();
TEST_F(OrderDBTests, AllocOrder_any_returnsOrder)
{
	//Arrange
	Target target(CreateTarget());

	//Act
	VSOrder *actual=target->AllocOrder();

	//Assert
	ASSERT_NE(0,(int)actual);
}
#ifdef _DEBUG
TEST_F(OrderDBTests, AllocOrder_perfOneSecond_processAtLeast500K)
#else
TEST_F(OrderDBTests, AllocOrder_perfOneSecond_processAtLeast1M)
#endif
{
	//Arrange
	Target target(CreateTarget());
	#ifdef _DEBUG
	int expected=500000;
	#else
	int expected=1000000;
	#endif

	//Act
	DWORD tstart=GetTickCount(),tnow=tstart;
	int actual=0;
	while((tnow=GetTickCount()) -tstart<1000)
	{
		target->AllocOrder(); actual++;
	}
	printf("             Allocated %d orders in %d ms\n",actual,tnow -tstart);

	//Assert
	ASSERT_LT(expected,actual);
}
TEST_F(OrderDBTests, AllocOrder_exceedCacheLimit_returnsOrder)
{
	//Arrange
	Target target(CreateTarget());
	target->HAVE_FUSION_IO=false;
	target->ORDER_CACHE_SIZE=1;
	char okey[40]={0};
	ITEMLOC iloc;
	iloc.offset=0;
	for(int i=0;i<8192;i++)
	{
		VSOrder *vsorder=target->AllocOrder();
		sprintf(okey,"%d",i+1);
		vsorder->SetOrderKey(okey);
		target->omap.insert(okey,iloc);
		iloc.offset++;
	}

	//Act
	VSOrder *actual=target->AllocOrder();

	//Assert
	ASSERT_NE(0,(int)actual);
}
// void OrderDB::FreeOrder(VSOrder *porder);
TEST_F(OrderDBTests, FreeOrder_any_unloadNotificationCalled)
{
	//Arrange
	Target target(CreateTarget());
	VSOrder *vsorder=target->AllocOrder();

	//Act
	target->FreeOrder(vsorder);
	int actual=GetUnloadNotifyCount();

	//Assert
	ASSERT_EQ(1,actual);
}
#ifdef _DEBUG
TEST_F(OrderDBTests, FreeOrder_perfOneSecond_processAtLeast200K)
#else
TEST_F(OrderDBTests, FreeOrder_perfOneSecond_processAtLeast1M)
#endif
{
	//Arrange
	Target target(CreateTarget());
	#ifdef _DEBUG
	int expected=200000;
	#else
	int expected=1000000;
	#endif
	list<VSOrder*> orderList;
	for(int i=0;i<expected;i++)
	{
		VSOrder *vsorder=target->AllocOrder(); 
		orderList.push_back(vsorder);
	}

	//Act
	DWORD tstart=GetTickCount(),tnow=tstart;
	int actual=0;
	list<VSOrder*>::iterator oit=orderList.begin();
	while((tnow=GetTickCount()) -tstart<1000)
	{
		if(oit==orderList.end())
			break;
		VSOrder *vsorder=*oit; oit++;
		target->FreeOrder(vsorder); actual++;
	}
	printf("             Freed %d orders in %d ms\n",actual,tnow -tstart);

	while(oit!=orderList.end())
	{
		VSOrder *vsorder=*oit; oit++;
		target->FreeOrder(vsorder); 
	}
	orderList.clear();

	//Assert
	ASSERT_LE(expected,actual);
}
// int OrderDB::WriteOrder(VSOrder *porder, ITEMLOC& offset);
TEST_F(OrderDBTests, WriteOrder_simpleOrder_returnsSuccess)
{
	//Arrange
	Target target(CreateTarget());
	VSOrder *vsorder=target->AllocOrder();
	const char *appinstid="BNPSTG";
	vsorder->SetAppInstID(appinstid);
	vsorder->SetClOrdID("YB24601");
	vsorder->SetClientID("gtest");
	vsorder->SetSymbol("BAC");
	vsorder->SetPrice(18.01);
	vsorder->SetSide('1');
	vsorder->SetOrderQty(100);
	vsorder->SetOrderKey(vsorder->GetClOrdID());

	//Act
	ITEMLOC offset;
	offset.wsdate=0;
	offset.offset=-1;
	int actual=target->WriteOrder(vsorder,offset);

	//Assert
	ASSERT_EQ(0,actual);
}
#ifdef _DEBUG
TEST_F(OrderDBTests, WriteOrder_perfOneSecond_processAtLeast40K)
#else
TEST_F(OrderDBTests, WriteOrder_perfOneSecond_processAtLeast150K)
#endif
{
	//Arrange
	Target target(CreateTarget());
	#ifdef _DEBUG
	int expected=40000;
	#else
	int expected=150000;
	#endif
	list<VSOrder*> orderList;
	for(int i=0;i<expected;i++)
	{
		VSOrder *vsorder=target->AllocOrder(); 
		const char *appinstid="BNPSTG";
		vsorder->SetAppInstID(appinstid);
		char clordid[64];
		sprintf(clordid,"%08d.YB%06d",i+1);
		vsorder->SetClOrdID(clordid);
		vsorder->SetClientID("gtest");
		vsorder->SetSymbol("BAC");
		vsorder->SetPrice(18.01);
		vsorder->SetSide('1');
		vsorder->SetOrderQty(100);
		vsorder->SetOrderKey(vsorder->GetClOrdID());
		orderList.push_back(vsorder);
	}

	//Act
	DWORD tstart=GetTickCount(),tnow=tstart;
	int actual=0;
	list<VSOrder*>::iterator oit=orderList.begin();
	while((tnow=GetTickCount()) -tstart<1000)
	{
		if(oit==orderList.end())
			break;
		VSOrder *vsorder=*oit; oit++;
		ITEMLOC offset;
		offset.wsdate=0;
		offset.offset=-1;
		target->WriteOrder(vsorder,offset); actual++;
		target->FreeOrder(vsorder);
	}
	printf("             Wrote %d orders in %d ms\n",actual,tnow -tstart);

	while(oit!=orderList.end())
	{
		VSOrder *vsorder=*oit; oit++;
		target->FreeOrder(vsorder); 
	}
	orderList.clear();

	//Assert
	ASSERT_LE(expected,actual);
}
// VSOrder *OrderDB::ReadOrder(const ITEMLOC& offset);
TEST_F(OrderDBTests, ReadOrder_orderExists_returnsOrder)
{
	//Arrange
	Target target(CreateTarget());
	VSOrder *vsorder=target->AllocOrder();
	const char *appinstid="BNPSTG";
	vsorder->SetAppInstID(appinstid);
	vsorder->SetClOrdID("YB24601");
	vsorder->SetClientID("gtest");
	vsorder->SetSymbol("BAC");
	vsorder->SetPrice(18.01);
	vsorder->SetSide('1');
	vsorder->SetOrderQty(100);
	vsorder->SetOrderKey(vsorder->GetClOrdID());
	ITEMLOC offset;
	offset.wsdate=0;
	offset.offset=-1;
	target->WriteOrder(vsorder,offset);

	//Act
	VSOrder *actual=target->ReadOrder(offset);

	//Assert
	ASSERT_NE(0,(int)actual);
}
TEST_F(OrderDBTests, ReadOrder_orderNotFound_returnsNull)
{
	//Arrange
	Target target(CreateTarget());
	SYSTEMTIME tloc;
	GetLocalTime(&tloc);
	ITEMLOC offset;
	offset.wsdate=(tloc.wYear*10000) +(tloc.wMonth*100) +(tloc.wDay);
	offset.offset=100000000;

	//Act
	VSOrder *actual=target->ReadOrder(offset);

	//Assert
	ASSERT_EQ(0,(int)actual);
}
TEST_F(OrderDBTests, ReadOrder_orderCached_returnsOrder)
{
	//Arrange
	Target target(CreateTarget());
	target->HAVE_FUSION_IO=false;
	VSOrder *vsorder=target->AllocOrder();
	const char *appinstid="BNPSTG";
	vsorder->SetAppInstID(appinstid);
	vsorder->SetClOrdID("YB24601");
	vsorder->SetClientID("gtest");
	vsorder->SetSymbol("BAC");
	vsorder->SetPrice(18.01);
	vsorder->SetSide('1');
	vsorder->SetOrderQty(100);
	vsorder->SetOrderKey(vsorder->GetClOrdID());
	ITEMLOC offset;
	offset.wsdate=0;
	offset.offset=-1;
	target->WriteOrder(vsorder,offset);

	//Act
	VSOrder *actual=target->ReadOrder(offset);

	//Assert
	ASSERT_NE(0,(int)actual);
}
#ifdef _DEBUG
TEST_F(OrderDBTests, ReadOrder_perfOneSecond_processAtLeast50K)
#else
TEST_F(OrderDBTests, ReadOrder_perfOneSecond_processAtLeast200K)
#endif
{
	//Arrange
	Target target(CreateTarget());
	#ifdef _DEBUG
	int expected=50000;
	#else
	int expected=200000;
	#endif

	//Act
	DWORD tstart=GetTickCount(),tnow=tstart;
	int actual=0;
	ITEMLOC offset;
	offset.wsdate=0;
	offset.offset=0;
	while((tnow=GetTickCount()) -tstart<1000)
	{
		VSOrder *vsorder=target->ReadOrder(offset);
		if(!vsorder) 
			break;
		actual++; 
		offset.offset+=VSORDER_RECSIZE;
		if(offset.offset>=150000*VSORDER_RECSIZE)
			offset.offset=0;
		target->FreeOrder(vsorder);
	}
	printf("             Read %d orders in %d ms\n",actual,tnow -tstart);

	//Assert
	ASSERT_LE(expected,actual);
}

}
