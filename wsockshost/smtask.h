
#ifndef _FEEDTASK_H
#define _FEEDTASK_H

#include <string>
#include <list>
#include <map>
#include <set>
using namespace std;

#define MAKE_STATIC
#define SMTASK_STRINGKEY
#define SMTASK_MULTITASK
#define SMTASK_CRIT_SECTION

#ifdef MAKE_STATIC
	#define TASKEXPORT
#else
	#ifdef MAKE_MODULE
	#define TASKEXPORT __declspec(dllexport)
	#else
	#define TASKEXPORT __declspec(dllimport)
	#endif
#endif

#ifndef uchar
#define uchar unsigned char
#endif

// Task callback signature
class FeedUser;
#ifdef SMTASK_STRINGKEY
typedef int (_stdcall *TASKCALLBACK)(FeedUser *fuser, const string& skey, uchar taskid, DWORD rid, void *udata, void *hint);
#else
typedef int (_stdcall *TASKCALLBACK)(FeedUser *fuser, void *uitem, uchar taskid, DWORD rid, void *udata, void *hint);
#endif

// One registered callback instance per item
struct TASKITEM;
struct TASKENTRY
{
	FeedUser *fuser;	// who
	TASKITEM *titem;	// what
	uchar taskid;		// how
	void *udata;		// parms
	DWORD rid;			// proxy request id
	int dest;			// proxy destination
	TASKCALLBACK cbfunc;
	TASKENTRY *uprev;	// user task list
	TASKENTRY *unext;	
	TASKENTRY *iprev;	// item task list
	TASKENTRY *inext;	
	DWORD udel;			// user delete time
	DWORD idel;			// item delete time
};
struct TASKBLOCK
{
	TASKENTRY *tasks;
	int ntasks;
	TASKBLOCK *next;
};

// A taskable object like a symbol
#ifdef SMTASK_STRINGKEY
struct TASKITEM
{
	string skey;
	TASKENTRY *ptasks;
	int taskCnt;
};
typedef map<string,TASKITEM *> TASKITEMMAP;
#else
struct TASKITEM
{
	void *uitem;
	TASKENTRY *ptasks;
	int taskCnt;
};
typedef map<DWORD,TASKITEM *> TASKITEMMAP;
#endif

struct SCHEDENTRY :public TASKENTRY
{
	long nextDate;
	long nextTime;
	bool repeat;
	bool sto;			// Scheduled-time-only
	bool days[7];		// UMTWRFS
};
typedef list<SCHEDENTRY *> SCHEDULE;

class FeedUser
{
public:
	FeedUser()
	{
		Reset();
	}
	inline void Reset()
	{
		PortType=-1;
		PortNo=-1;
		domain="";
		user="";
		epassLen=0;
		loginDate=0;
		loginTime=0;
		buildVersion=0;
		programVersion=0;
		ptasks=0;
		taskCnt=0;
		authed=false;
		waitAuth=true;
		DetPtr=0;
		DetPtr1=0;
		DetPtr2=0;
		DetPtr3=0;
		deleted=0;
		memset(epass,0,128);
	}
	static const string Key(const string domain, const string user)
	{
		string ukey=domain +"/" +user;
		return ukey;
	}

public:
	int PortType;
	int PortNo;
	string domain;
	string user;
	WORD epassLen;
	uchar epass[128];
	long loginDate;
	long loginTime;
	DWORD buildVersion;
	int programVersion;
	TASKENTRY *ptasks;
	int taskCnt;
	bool authed;
	bool waitAuth;
	void *DetPtr;
	void *DetPtr1;
	void *DetPtr2;
	void *DetPtr3;
	DWORD deleted;
};

class SMTaskMgr
{
public:
	TASKEXPORT SMTaskMgr();
	TASKEXPORT ~SMTaskMgr();
	TASKEXPORT int Startup(int maxUsers, int taskBlockSize);
	TASKEXPORT void Shutdown();

	// User management
	TASKEXPORT FeedUser *AddUser(int PortType, int PortNo);
	TASKEXPORT int RemUser(int PortType, int PortNo);
	TASKEXPORT int CountUsers(){ return nusers; }

	#ifdef SMTASK_STRINGKEY
	TASKEXPORT TASKITEM *FindUserItem(const string& skey);
	// Taskentry and taskitem management
	TASKEXPORT TASKENTRY *FindTask(FeedUser *fuser, const string& skey, uchar taskid);
	TASKEXPORT TASKENTRY *FindUserTask(FeedUser *fuser, const string& skey, uchar taskid);
	TASKEXPORT TASKENTRY *AddTask(FeedUser *fuser, const string& skey, uchar taskid, void *udata, DWORD rid, TASKCALLBACK cbfunc);
	TASKEXPORT int DelTask(FeedUser *fuser, const string& skey, uchar taskid);
	TASKEXPORT int CallTasks(const string& skey, uchar taskid, void *hint);
	TASKEXPORT int CallItemTasks(TASKITEM *titem, uchar taskid, void *hint);
	TASKEXPORT int DelUserTasks(FeedUser *fuser);
	TASKEXPORT int DelUserItem(const string& skey);
	TASKEXPORT int CountTasks(const string& skey);
	#else
	TASKEXPORT TASKITEM *FindUserItem(void *uitem);
	// Taskentry and taskitem management
	TASKEXPORT TASKENTRY *FindTask(FeedUser *fuser, void *uitem, uchar taskid);
	TASKEXPORT TASKENTRY *FindUserTask(FeedUser *fuser, void *uitem, uchar taskid);
	TASKEXPORT TASKENTRY *AddTask(FeedUser *fuser, void *uitem, HANDLE umutex, uchar taskid, void *udata, DWORD rid, TASKCALLBACK cbfunc);
	TASKEXPORT int DelTask(FeedUser *fuser, void *uitem, uchar taskid);
	//TASKEXPORT int CallTasks(void *uitem, uchar taskid, void *hint);
	TASKEXPORT int CallItemTasks(TASKITEM *titem, uchar taskid, void *hint);
	TASKEXPORT int DelUserTasks(FeedUser *fuser);
	TASKEXPORT int DelUserItem(void *uitem);
	TASKEXPORT int CountTasks(void *uitem);
	#endif
	TASKEXPORT int DelUserTask(TASKENTRY *dtask);
	TASKEXPORT int DelItemTask(TASKENTRY *dtask);
	TASKEXPORT int DelItemTasks(TASKITEM *titem);

	// Item iteration
	TASKEXPORT inline TASKITEMMAP::iterator ibegin(){return itemMap.begin();}
	TASKEXPORT inline TASKITEMMAP::iterator iend(){return itemMap.end();}
	TASKEXPORT int CountTasks();

	// Multi-thread support
	#ifdef SMTASK_CRIT_SECTION
	virtual inline int Lock(){EnterCriticalSection(&mutex); return 0;}
	virtual inline int Unlock(){LeaveCriticalSection(&mutex); return 0;}
	#else
	virtual inline int Lock(){ return WaitForSingleObject(mutex,INFINITE)==WAIT_OBJECT_0?0:-1; }
	virtual inline int Unlock(){return ReleaseMutex(mutex)?0:-1;}
	#endif

	// Scheduled tasks
	TASKEXPORT long NextSchedDate(bool days[7], bool allowToday);
	TASKEXPORT TASKENTRY *AddScheduledTask(FeedUser *fuser, uchar taskid, void *udata, TASKCALLBACK cbfunc,
									long nextDate, long nextTime, bool repeat, bool sto, bool days[7],
									long wdate, long wtime);
	TASKEXPORT int DelScheduledTask(FeedUser *fuser, uchar taskid);
	TASKEXPORT int CheckSchedule(long wdate, long wtime);

protected:
	// User database
	int MAX_USERS;
	FeedUser *feedUsers;
	int nusers;

	// Task database
	int taskCnt;
	int delTaskCnt;
	int TASK_BLOCK_SIZE;
	TASKBLOCK *tblocks;
	TASKENTRY *newTasks,*freedTasks;
	TASKENTRY *AllocTask();
	int FreeTask(TASKENTRY *ptask);

	// Item database
	TASKITEMMAP itemMap;
	#ifdef SMTASK_STRINGKEY
	TASKITEM *AddUserItem(const string& skey);
	#else
	TASKITEM *AddUserItem(void *uitem, HANDLE umutex);
	#endif

	// Scheduled tasks
	SCHEDULE schedule;
	long WSDate;

	// Multi-thread support
	#ifdef SMTASK_CRIT_SECTION
	CRITICAL_SECTION mutex;
	#else
	HANDLE mutex;
	#endif
};

#endif//_FEEDTASK_H
