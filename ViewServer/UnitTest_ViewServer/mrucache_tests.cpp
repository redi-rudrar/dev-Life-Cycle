#include "stdafx.h"

using namespace ::testing;

namespace viewserver_unit_tests
{

class MRUCacheTester :public VSMruCache
{
public:
	int GetItemCount()
	{
		return nitems;
	}
	int GetItemPosition(const char *key)
	{
		for(int i=0;i<MRU_SIZE;i++)
		{
			if(!_stricmp(items[i].key,key))
				return i;
		}
		return -1;
	}
};

class MRUCacheTests :public Test
{
public:
	MRUCacheTests() 
		:Test()
	{
	}
	~MRUCacheTests()
	{
		FreeOrders();
		FreeTargets();
	}

	typedef MRUCacheTester *Target;
	Target CreateTarget()
	{
		Target target=new MRUCacheTester();
		targetList.push_back(target);
		return target;
	}

	VSOrder *CreateOrder()
	{
		VSOrder *vsorder=new VSOrder();
		orderList.push_back(vsorder);
		return vsorder;
	}

protected:
	list<Target> targetList;
	void FreeTargets()
	{
		for(list<Target>::iterator tit=targetList.begin();tit!=targetList.end();tit++)
			delete *tit;
		targetList.clear();
	}

	list<VSOrder*> orderList;
	void FreeOrders()
	{
		for(list<VSOrder*>::iterator oit=orderList.begin();oit!=orderList.end();oit++)
			delete *oit;
		orderList.clear();
	}
};

// int VSMruCache::AddItem(const char *key, VSOrder *val)
TEST_F(MRUCacheTests, AddItem_validKey_returnsSuccess)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey="20160125.YB24601";
	VSOrder *vsorder=CreateOrder();

	//Act
	int actual=target->AddItem(vskey,vsorder);

	//Assert
	ASSERT_EQ(0,actual);
}
TEST_F(MRUCacheTests, AddItem_invalidKey_returnsFailure)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey="1234567890123456789012345678901234567890123456789012345678901234567890";
	VSOrder *vsorder=CreateOrder();

	//Act
	int actual=target->AddItem(vskey,vsorder);

	//Assert
	ASSERT_EQ(-1,actual);
}
TEST_F(MRUCacheTests, AddItem_twoItems_itemCountIsTwo)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";
	VSOrder *vsorder2=CreateOrder();
	target->AddItem(vskey1,vsorder1);

	//Act
	target->AddItem(vskey2,vsorder2);
	int actual=target->GetItemCount();

	//Assert
	ASSERT_EQ(2,actual);
}
TEST_F(MRUCacheTests, AddItem_duplicateItem_doesNotIncreaseCount)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";
	VSOrder *vsorder2=CreateOrder();
	target->AddItem(vskey1,vsorder1);
	target->AddItem(vskey2,vsorder2);
	int expected=target->GetItemCount();

	//Act
	target->AddItem(vskey1,vsorder1);
	int actual=target->GetItemCount();

	//Assert
	ASSERT_EQ(expected,actual);
}
TEST_F(MRUCacheTests, AddItem_maxItemsPlusOne_firstItemRemoved)
{
	//Arrange
	Target target(CreateTarget());
	char vskey[64]={0},vskey1[64]={0};
	for(int i=1;i<=MRU_SIZE;i++)
	{
		sprintf(vskey,"20160125.YB2460%d",i);
		if(!vskey1[0])
			strcpy(vskey1,vskey);
		VSOrder *vsorder=CreateOrder();
		target->AddItem(vskey,vsorder);
	}
	sprintf(vskey,"20160125.YB2460%d",MRU_SIZE+1);
	VSOrder *vsorder=CreateOrder();

	//Act
	target->AddItem(vskey,vsorder);
	VSOrder *actual=target->GetItem(vskey1);

	//Assert
	ASSERT_EQ(0,actual);
}
#ifdef _DEBUG
TEST_F(MRUCacheTests, AddItem_perfOneSecond_processAtLeast400K)
#else
TEST_F(MRUCacheTests, AddItem_perfOneSecond_processAtLeast1500K)
#endif
{
	//Arrange
	#ifdef _DEBUG
	int expected=400000;
	#else
	int expected=1500000;
	#endif
	Target target(CreateTarget());
	char vskey[64]={0};
	VSOrder *vsorder=CreateOrder();

	//Act
	DWORD tstart=GetTickCount(),tnow=tstart;
	int actual=0;
	while((tnow=GetTickCount()) -tstart<1000)
	{
		sprintf(vskey,"20160125.YB%07d",++actual);
		target->AddItem(vskey,vsorder);
	}
	printf("             Added %d items in %d ms\n",actual,tnow -tstart);

	//Assert
	ASSERT_LT(expected,actual);
}
// VSOrder *VSMruCache::GetItem(const char *key)
TEST_F(MRUCacheTests, GetItem_itemFound_returnsItem)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";
	VSOrder *vsorder2=CreateOrder();
	target->AddItem(vskey1,vsorder1);

	//Act
	VSOrder *actual=target->GetItem(vskey1);

	//Assert
	ASSERT_EQ(vsorder1,actual);
}
TEST_F(MRUCacheTests, GetItem_itemNotFound_returnsNull)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";

	//Act
	VSOrder *actual=target->GetItem(vskey2);

	//Assert
	ASSERT_EQ(0,actual);
}
#ifdef _DEBUG
TEST_F(MRUCacheTests, GetItem_perfOneSecond_processAtLeast400K)
#else
TEST_F(MRUCacheTests, GetItem_perfOneSecond_processAtLeast2M)
#endif
{
	//Arrange
	#ifdef _DEBUG
	int expected=400000;
	#else
	int expected=2000000;
	#endif
	Target target(CreateTarget());
	char vskey[64]={0};
	VSOrder *vsorder=CreateOrder();
	sprintf(vskey,"20160125.YB24601");
	target->AddItem(vskey,vsorder);
	sprintf(vskey,"20160125.YB24602");
	target->AddItem(vskey,vsorder);
	sprintf(vskey,"20160125.YB24603");
	target->AddItem(vskey,vsorder);
	sprintf(vskey,"20160125.YB24604");
	target->AddItem(vskey,vsorder);

	//Act
	DWORD tstart=GetTickCount(),tnow=tstart;
	int actual=0;
	while((tnow=GetTickCount()) -tstart<1000)
	{
		sprintf(vskey,"20160125.YB2460%d",1+(++actual)%4);
		target->GetItem(vskey);
	}
	printf("             Got %d items in %d ms\n",actual,tnow -tstart);

	//Assert
	ASSERT_LT(expected,actual);
}
// int VSMruCache::RemItemByKey(const char *key)
TEST_F(MRUCacheTests, RemItemByKey_itemFound_itemRemoved)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";
	VSOrder *vsorder2=CreateOrder();
	target->AddItem(vskey1,vsorder1);

	//Act
	target->RemItemByKey(vskey1);
	VSOrder *actual=target->GetItem(vskey1);

	//Assert
	ASSERT_EQ(0,actual);
}
TEST_F(MRUCacheTests, RemItemByKey_itemNotFound_returnsFailure)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";

	//Act
	int actual=target->RemItemByKey(vskey2);

	//Assert
	ASSERT_EQ(-1,actual);
}
// int VSMruCache::RemItemByVal(VSOrder *val)
TEST_F(MRUCacheTests, RemItemByVal_itemFound_itemRemoved)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";
	VSOrder *vsorder2=CreateOrder();
	target->AddItem(vskey1,vsorder1);

	//Act
	target->RemItemByVal(vsorder1);
	VSOrder *actual=target->GetItem(vskey1);

	//Assert
	ASSERT_EQ(0,actual);
}
TEST_F(MRUCacheTests, RemItemByVal_itemNotFound_returnsFailure)
{
	//Arrange
	Target target(CreateTarget());
	const char *vskey1="20160125.YB24601";
	VSOrder *vsorder1=CreateOrder();
	const char *vskey2="20160125.YB24602";
	VSOrder *vsorder2=CreateOrder();

	//Act
	int actual=target->RemItemByVal(vsorder2);

	//Assert
	ASSERT_EQ(-1,actual);
}

}
