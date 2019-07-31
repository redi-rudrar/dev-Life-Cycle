
#include "stdafx.h"
#include "smtask.h"
#ifdef WIN32
#include <crtdbg.h>
#endif
#include <time.h>

#ifndef WS_USR
#define WS_USR 1
#endif
#ifndef WS_UMR
#define WS_UMR 11
#endif

SMTaskMgr::SMTaskMgr()
{
	MAX_USERS=0;
	feedUsers=0;
	nusers=0;

	WSDate=0;
	#ifdef SMTASK_CRIT_SECTION
	InitializeCriticalSection(&mutex);
	#else
	mutex=CreateMutex(0,false,0);
	#endif

	TASK_BLOCK_SIZE=0;
	taskCnt=delTaskCnt=0;
	tblocks=0; 
	newTasks=freedTasks=0;
}
SMTaskMgr::~SMTaskMgr()
{
	Shutdown();
	#ifdef SMTASK_CRIT_SECTION
	DeleteCriticalSection(&mutex);
	#endif
}
int SMTaskMgr::Startup(int maxUsers, int taskBlockSize)
{
	if((maxUsers<1)||(taskBlockSize<8))
		return -1;
	MAX_USERS=maxUsers;
	feedUsers=new FeedUser [MAX_USERS+1];
	TASK_BLOCK_SIZE=taskBlockSize;
	return 0;
}
void SMTaskMgr::Shutdown()
{
	#ifdef SMTASK_CRIT_SECTION
	EnterCriticalSection(&mutex);
	#else
	if(mutex)
		Lock();
	#endif
	for(SCHEDULE::iterator sit=schedule.begin();sit!=schedule.end();sit++)
	{
		SCHEDENTRY *stask=*sit;
		delete stask;
	}
	schedule.clear();

	for(TASKITEMMAP::iterator iit=itemMap.begin();iit!=itemMap.end();iit++)
	{
		TASKITEM *pitem=iit->second;
		pitem->ptasks=0;
		delete pitem;
	}
	itemMap.clear();

	if(feedUsers)
	{
		for(int i=0;i<nusers;i++)
		{
			FeedUser *fuser=&feedUsers[i];
			fuser->ptasks=0; fuser->taskCnt=0;
		}
		delete[] feedUsers; feedUsers=0;
	}
	nusers=0;

	newTasks=freedTasks=0;
	while(tblocks)
	{
		TASKBLOCK *dblock=tblocks;
		tblocks=tblocks->next;
		if(dblock->tasks)
			delete [] dblock->tasks;
		delete dblock;
	}
	taskCnt=delTaskCnt=0;

	#ifdef SMTASK_CRIT_SECTION
	LeaveCriticalSection(&mutex);
	#else
	if(mutex)
	{
		#ifdef WIN32
		CloseHandle(mutex); mutex=0;
		#else
		DeleteMutex(mutex); mutex=0;
		#endif
	}
	#endif
}

TASKENTRY *SMTaskMgr::AllocTask()
{
	if(!feedUsers)
		return 0;
	if(freedTasks)
	{
		TASKENTRY *ptask=freedTasks;
		freedTasks=freedTasks->inext;
		ptask->iprev=ptask->inext=0;
		ptask->uprev=ptask->unext=0;
		return ptask;
	}
	if(newTasks)
	{
		TASKENTRY *ptask=newTasks;
		newTasks=newTasks->inext;
		ptask->iprev=ptask->inext=0;
		ptask->uprev=ptask->unext=0;
		return ptask;
	}
	// Need a new block
	TASKBLOCK *tblock=new TASKBLOCK;
	if(!tblock)
		return 0;
	memset(tblock,0,sizeof(TASKBLOCK));
	tblock->tasks=new TASKENTRY[TASK_BLOCK_SIZE];
	memset(tblock->tasks,0,TASK_BLOCK_SIZE *sizeof(TASKENTRY));
	tblock->next=tblocks; tblocks=tblock;
	// Pre-build a list of new tasks
	for(int i=TASK_BLOCK_SIZE -1;i>=0;i--)
	{
		TASKENTRY *ptask=&tblock->tasks[i];
		ptask->inext=newTasks; newTasks=ptask;
	}
	TASKENTRY *ptask=newTasks;
	newTasks=newTasks->inext;
	ptask->iprev=ptask->inext=0;
	ptask->uprev=ptask->unext=0;
	return ptask;
}
int SMTaskMgr::FreeTask(TASKENTRY *ptask)
{
	_ASSERT((ptask->udel)&&(ptask->idel));
	// Append to freed list
	memset(ptask,0,sizeof(TASKENTRY));
	ptask->inext=freedTasks;	
	freedTasks=ptask;
	return 0;
}
FeedUser *SMTaskMgr::AddUser(int PortType, int PortNo)
{
	if(((PortType!=WS_USR)&&(PortType!=WS_UMR))||(PortNo<0)||(PortNo>MAX_USERS))
		return 0;
	if(!feedUsers)
		return 0;
	FeedUser *fuser=&feedUsers[PortNo];
	fuser->Reset();
	fuser->PortType=PortType;
	fuser->PortNo=PortNo;
	nusers++;
	return fuser;
}
int SMTaskMgr::RemUser(int PortType, int PortNo)
{
	if(((PortType!=WS_USR)&&(PortType!=WS_UMR))||(PortNo<0)||(PortNo>MAX_USERS))
		return -1;
	if(!feedUsers)
		return 0;
	FeedUser *fuser=&feedUsers[PortNo];
	fuser->deleted=GetTickCount();
	DelUserTasks(fuser);
	if(nusers>0) nusers--;
	return 0;
}

#ifdef SMTASK_STRINGKEY
TASKITEM *SMTaskMgr::FindUserItem(const string& skey)
{
	TASKITEMMAP::iterator iit=itemMap.find(skey);
	if(iit==itemMap.end())
		return 0;
	return iit->second;
}
#else
TASKITEM *SMTaskMgr::FindUserItem(void *uitem)
{
	TASKITEMMAP::iterator iit=itemMap.find((DWORD)uitem);
	if(iit==itemMap.end())
		return 0;
	return iit->second;
}
#endif
#ifdef SMTASK_STRINGKEY
TASKITEM *SMTaskMgr::AddUserItem(const string& skey)
{
	TASKITEM *pitem=FindUserItem(skey);
	if(!pitem)
	{
		pitem=new TASKITEM;
		pitem->skey=skey;
		pitem->ptasks=0;
		pitem->taskCnt=0;
		itemMap[skey]=pitem;
	}
	return pitem;
}
#else
TASKITEM *SMTaskMgr::AddUserItem(void *uitem, HANDLE umutex)
{
	TASKITEM *pitem=FindUserItem(uitem);
	if(!pitem)
	{
		pitem=new TASKITEM;
		pitem->uitem=uitem;
		pitem->ptasks=0;
		pitem->taskCnt=0;
		itemMap[(DWORD)uitem]=pitem;
	}
	return pitem;
}
#endif
#ifdef SMTASK_STRINGKEY
int SMTaskMgr::DelUserItem(const string& skey)
{
	TASKITEMMAP::iterator iit=itemMap.find(skey);
	if(iit==itemMap.end())
		return -1;
	TASKITEM *titem=iit->second;
	itemMap.erase(iit);
	for(TASKENTRY *ptask=titem->ptasks;ptask;ptask=ptask->inext)
	{
		if(ptask->titem)
			DelTask(ptask->fuser,ptask->titem->skey,ptask->taskid);
	}
	DelItemTasks(titem);
	delete titem;
	return 0;
}
#else
int SMTaskMgr::DelUserItem(void *uitem)
{
	TASKITEMMAP::iterator iit=itemMap.find((DWORD)uitem);
	if(iit==itemMap.end())
		return -1;
	TASKITEM *titem=iit->second;
	itemMap.erase(iit);
	for(TASKENTRY *ptask=titem->ptasks;ptask;ptask=ptask->inext)
	{
		if(ptask->titem)
			DelTask(ptask->fuser,ptask->titem->uitem,ptask->taskid);
	}
	DelItemTasks(titem);
	delete titem;
	return 0;
}
#endif
#ifdef SMTASK_STRINGKEY
int SMTaskMgr::CountTasks(const string& skey)
{
	TASKITEMMAP::iterator iit=itemMap.find(skey);
	if(iit==itemMap.end())
		return -1;
	TASKITEM *titem=iit->second;
	return titem->taskCnt;
}
#else
int SMTaskMgr::CountTasks(void *uitem)
{
	TASKITEMMAP::iterator iit=itemMap.find((DWORD)uitem);
	if(iit==itemMap.end())
		return -1;
	TASKITEM *titem=iit->second;
	return titem->taskCnt;
}
#endif
int SMTaskMgr::CountTasks()
{
	int tsize=taskCnt -delTaskCnt;
	if(tsize<0) tsize=0;
	return tsize;
}

#ifdef SMTASK_STRINGKEY
TASKENTRY *SMTaskMgr::FindTask(FeedUser *fuser, const string& skey, uchar taskid)
{
	for(TASKENTRY *stask=fuser->ptasks;stask;stask=stask->unext)
	{
		if((stask->fuser==fuser)&&(stask->titem)&&(stask->titem->skey==skey)&&(stask->taskid==taskid))
			return stask;
	}
	return 0;
}
// This is effectively the same as FindTask, but is more efficient when the user has hundreds of tasks.
TASKENTRY *SMTaskMgr::FindUserTask(FeedUser *fuser, const string& skey, uchar taskid)
{	
	TASKITEM *titem=FindUserItem(skey);
	if(!titem)
		return 0;	
	for(TASKENTRY *stask=titem->ptasks;stask;stask=stask->inext)
	{
		if((stask->fuser==fuser)&&(stask->taskid==taskid))
			return stask;
	}
	return 0;
}
#else
TASKENTRY *SMTaskMgr::FindTask(FeedUser *fuser, void *uitem, uchar taskid)
{
	for(TASKENTRY *stask=fuser->ptasks;stask;stask=stask->unext)
	{
		if((stask->fuser==fuser)&&(stask->titem)&&(stask->titem->uitem==uitem)&&(stask->taskid==taskid))
			return stask;
	}
	return 0;
}
// This is effectively the same as FindTask, but is more efficient when the user has hundreds of tasks.
TASKENTRY *SMTaskMgr::FindUserTask(FeedUser *fuser, void *uitem, uchar taskid)
{	
	TASKITEM *titem=FindUserItem(uitem);
	if(!titem)
		return 0;	
	for(TASKENTRY *stask=titem->ptasks;stask;stask=stask->inext)
	{
		if((stask->fuser==fuser)&&(stask->taskid==taskid))
			return stask;
	}
	return 0;
}
#endif

#ifdef SMTASK_STRINGKEY
TASKENTRY *SMTaskMgr::AddTask(FeedUser *fuser, const string& skey, uchar taskid, void *udata, DWORD rid, TASKCALLBACK cbfunc)
{
	if(!fuser->authed)
		return 0;
	Lock();
	TASKITEM *titem=AddUserItem(skey);
	if(!titem)
	{
		Unlock();
		return 0;
	}
#ifdef SMTASK_MULTITASK
	// Allow multiple tasks on the same item
	TASKENTRY *ptask=0;
#else
	TASKENTRY *ptask=FindTask(fuser,skey,taskid);
	if(ptask)
	{
	//	ptask->fuser=fuser;
	//	ptask->cbfunc=cbfunc;
	//	ptask->udata=udata;
	//	ptask->rid=rid;
	//	ptask->dest=-1;
	//	if(ptask->udel)
	//	{
	//		ptask->udel=0;
	//		if(delTaskCnt>0) delTaskCnt--;
	//	}
	//	ptask->idel=0;
		DelUserTask(ptask);
	}
	else
#endif
	{
		ptask=AllocTask();
		if(!ptask)
		{
			Unlock();
			return 0;
		}
		ptask->fuser=fuser;
		ptask->titem=titem;
		ptask->taskid=taskid;
		ptask->cbfunc=cbfunc;
		ptask->udata=udata;
		ptask->rid=rid;
		ptask->dest=-1;
		ptask->udel=0;
		ptask->idel=0;
		taskCnt++;
		if(titem->ptasks)
		{
			ptask->inext=titem->ptasks; titem->ptasks->iprev=ptask;
		}
		titem->ptasks=ptask; titem->taskCnt++;
		if(fuser->ptasks)
		{
			ptask->unext=fuser->ptasks; fuser->ptasks->uprev=ptask;
		}
		fuser->ptasks=ptask; fuser->taskCnt++;
	}
	_ASSERT((ptask->fuser)&&(ptask->titem)&&(ptask->taskid)&&(ptask->cbfunc));
	Unlock();
	return ptask;
}
#else
TASKENTRY *SMTaskMgr::AddTask(FeedUser *fuser, void *uitem, HANDLE umutex, uchar taskid, void *udata, DWORD rid, TASKCALLBACK cbfunc)
{
	if(!fuser->authed)
		return 0;
	Lock();
	TASKITEM *titem=AddUserItem(uitem,umutex);
	if(!titem)
	{
		Unlock();
		return 0;
	}
	TASKENTRY *ptask=FindTask(fuser,uitem,taskid);
	if(ptask)
	{
		ptask->cbfunc=cbfunc;
		ptask->udata=udata;
		ptask->rid=rid;
		ptask->dest=-1;
		ptask->fuser=fuser;
		ptask->udel=0;
		ptask->idel=0;
	}
	else
	{
		ptask=AllocTask();
		if(!ptask)
		{
			Unlock();
			return 0;
		}
		ptask->fuser=fuser;
		ptask->titem=titem;
		ptask->taskid=taskid;
		ptask->cbfunc=cbfunc;
		ptask->udata=udata;
		ptask->rid=rid;
		ptask->dest=-1;
		ptask->udel=0;
		ptask->idel=0;
		taskCnt++;
		if(titem->ptasks)
		{
			ptask->inext=titem->ptasks; titem->ptasks->iprev=ptask;
		}
		titem->ptasks=ptask; titem->taskCnt++;
		if(fuser->ptasks)
		{
			ptask->unext=fuser->ptasks; fuser->ptasks->uprev=ptask;
		}
		fuser->ptasks=ptask; fuser->taskCnt++;
	}	
	_ASSERT((ptask->fuser)&&(ptask->titem)&&(ptask->taskid)&&(ptask->cbfunc));
	Unlock();
	return ptask;
}
#endif

#ifdef SMTASK_STRINGKEY
int SMTaskMgr::DelTask(FeedUser *fuser, const string& skey, uchar taskid)
{
	Lock();
	TASKENTRY *ptask;
	for(ptask=fuser->ptasks;ptask;ptask=ptask->unext)
	{
		if((ptask->fuser==fuser)&&(ptask->titem)&&(ptask->titem->skey==skey)&&(ptask->taskid==taskid))
		{
			DelUserTask(ptask);
			break;
		}
	}
	Unlock();
	return ptask?0:-1;
}
#else
int SMTaskMgr::DelTask(FeedUser *fuser, void *uitem, uchar taskid)
{
	Lock();
	TASKENTRY *ptask;
	for(ptask=fuser->ptasks;ptask;ptask=ptask->unext)
	{
		if((ptask->fuser==fuser)&&(ptask->titem)&&(ptask->titem->uitem==uitem)&&(ptask->taskid==taskid))
		{
			DelUserTask(ptask);
			break;
		}
	}
	Unlock();
	return ptask?0:-1;
}
#endif
int SMTaskMgr::DelUserTask(TASKENTRY *dtask)
{
	Lock();
	//char dbuf[256]={0};
	//sprintf(dbuf,"%d DelUserTask(%x)\r\n",GetCurrentThreadId(),dtask);
	//OutputDebugString(dbuf);
	if(!dtask->udel)
	{
		if(dtask->fuser)
		{
			if(dtask->unext) dtask->unext->uprev=dtask->uprev;
			if(dtask->uprev) dtask->uprev->unext=dtask->unext;
			else dtask->fuser->ptasks=dtask->unext;
			dtask->uprev=dtask->unext=0;
			dtask->udata=0;
			if(dtask->fuser->taskCnt>0) dtask->fuser->taskCnt--;
			dtask->udel=GetTickCount(); delTaskCnt++;
		}
		// dtask->fuser "shouldn't" be null, but we'll protect against crash just in case
		else
		{
			if(dtask->unext) dtask->unext->uprev=dtask->uprev;
			if(dtask->uprev) dtask->uprev->unext=dtask->unext;
			dtask->uprev=dtask->unext=0;
			dtask->udata=0;
			dtask->udel=GetTickCount(); delTaskCnt++;
		}
	}
	if(dtask->idel)
		FreeTask(dtask);
	Unlock();
	return 0;
}
int SMTaskMgr::DelUserTasks(FeedUser *fuser)
{
	Lock();
	while(fuser->ptasks)
		DelUserTask(fuser->ptasks);
	Unlock();
	return 0;
}

#ifdef SMTASK_STRINGKEY
int SMTaskMgr::CallTasks(const string& skey, uchar taskid, void *hint)
{
	TASKITEM *titem=FindUserItem(skey);
	if(!titem)
		return -1;
	Lock();
	int rc=CallItemTasks(titem,taskid,hint);
	Unlock();
	return rc;
}
int SMTaskMgr::CallItemTasks(TASKITEM *titem, uchar taskid, void *hint)
{
	if(!titem)
	{
		_ASSERT(false);
		return -1;
	}
	for(TASKENTRY *ptask=titem->ptasks;ptask;)
	{
		if((ptask->udel)||(!ptask->fuser))
		{
			TASKENTRY *dtask=ptask;
			ptask=ptask->inext;
			DelItemTask(dtask);
			continue;
		}
		if((!ptask->cbfunc)||(ptask->taskid!=taskid))
		{
			ptask=ptask->inext;
			continue;
		}
		if(ptask->cbfunc(ptask->fuser,titem->skey,ptask->taskid,ptask->rid,ptask->udata,hint)<=0)
		{
			TASKENTRY *dtask=ptask;
			ptask=ptask->inext;
			if(!dtask->udel)
				DelTask(dtask->fuser,titem->skey,dtask->taskid);
			DelItemTask(dtask);
			continue;
		}
		ptask=ptask->inext;
	}
	return 0;
}
#else
// If the caller caches the TASKITEM from AddTask, then there's no need to look up the item again.
// At 500K mps, every lookup counts.
//int SMTaskMgr::CallTasks(void *uitem, uchar taskid, void *hint)
//{
//	TASKITEM *titem=FindUserItem(skey);
//	if(!titem)
//		return -1;
//	Lock();
//	int rc=CallItemTasks(titem,taskid,hint);
//	Unlock();
//	return rc;
//}
// CallItemTasks should not be called from more than one thread on the same object at once
int SMTaskMgr::CallItemTasks(TASKITEM *titem, uchar taskid, void *hint)
{
	if(!titem)
	{
		_ASSERT(false);
		return -1;
	}
	for(TASKENTRY *ptask=titem->ptasks;ptask;)
	{
		if((ptask->udel)||(!ptask->fuser))
		{
			TASKENTRY *dtask=ptask;
			ptask=ptask->inext;
			DelItemTask(dtask);
			continue;
		}
		if((!ptask->cbfunc)||(ptask->taskid!=taskid))
		{
			ptask=ptask->inext;
			continue;
		}
		if(ptask->cbfunc(ptask->fuser,titem->uitem,ptask->taskid,ptask->rid,ptask->udata,hint)<=0)
		{
			TASKENTRY *dtask=ptask;
			ptask=ptask->inext;
			if(!dtask->udel)
				DelTask(dtask->fuser,titem->uitem,dtask->taskid);
			DelItemTask(dtask);
			continue;
		}
		ptask=ptask->inext;
	}
	return 0;
}
#endif
int SMTaskMgr::DelItemTask(TASKENTRY *dtask)
{
	Lock();
	//char dbuf[256]={0};
	//sprintf(dbuf,"%d DelItemTask(%x)\r\n",GetCurrentThreadId(),dtask);
	//OutputDebugString(dbuf);
	if(!dtask->idel)
	{
		if(dtask->inext) dtask->inext->iprev=dtask->iprev;
		if(dtask->iprev) dtask->iprev->inext=dtask->inext;
		else if(dtask->titem) dtask->titem->ptasks=dtask->inext;
		dtask->iprev=dtask->inext=0;
		// dtask->titem "shouldn't" be null, but we'll protect against crash just in case
		if((dtask->titem)&&(dtask->titem->taskCnt>0)) dtask->titem->taskCnt--;
		dtask->idel=GetTickCount();
	}
	if(dtask->udel)
		FreeTask(dtask);
	Unlock();
	return 0;
}
int SMTaskMgr::DelItemTasks(TASKITEM *titem)
{
	Lock();
	while(titem->ptasks)
		DelItemTask(titem->ptasks);
	Unlock();
	return 0;
}

long SMTaskMgr::NextSchedDate(bool days[7], bool allowToday)
{
	time_t tnow=time(0);
	tm ltm=*localtime(&tnow);
	for(int i=0;i<7;i++)
	{
		if((days[ltm.tm_wday])&&((allowToday)||(ltm.tm_mday!=WSDate%100)))
		{
			/*time_t tnext=*/mktime(&ltm);
			return ((1900 +ltm.tm_year) *10000) +(ltm.tm_mon +1)*100 +ltm.tm_mday;
		}
		ltm.tm_wday++;
		ltm.tm_wday%=7;
		ltm.tm_mday++;
	}
	return 0;
}
TASKENTRY *SMTaskMgr::AddScheduledTask(FeedUser *fuser, uchar taskid, void *udata, TASKCALLBACK cbfunc,
										 long nextDate, long nextTime, bool repeat, bool sto, bool days[7],
										 long wdate, long wtime)
{
	if(!fuser->authed)
		return 0;
	WSDate=wdate;
	if(!nextDate)
		nextDate=NextSchedDate(days,true);
	// Don't schedule tasks for today if time has already expired
	if((repeat)&&(sto)&&
	   ((nextDate<wdate)||((nextDate==wdate)&&(nextTime<=wtime))))
		nextDate=NextSchedDate(days,false);
	SCHEDENTRY *ptask=new SCHEDENTRY;
	memset(ptask,0,sizeof(SCHEDENTRY));
	ptask->fuser=fuser;
	ptask->taskid=taskid;
	ptask->udata=udata;
	ptask->cbfunc=cbfunc;
	ptask->nextDate=nextDate;
	ptask->nextTime=nextTime;
	ptask->repeat=repeat;
	ptask->sto=sto;
	memcpy(ptask->days,days,7);

	Lock();
	for(SCHEDULE::iterator sit=schedule.begin();sit!=schedule.end();sit++)
	{
		SCHEDENTRY *stask=*sit;
		if(stask->nextDate<nextDate)
			continue;
		else if((stask->nextDate==nextDate)&&(stask->nextTime<nextTime))
			continue;
		// Middle of list
		else if((stask->nextDate>nextDate)||(stask->nextTime>nextTime))
		{
			schedule.insert(sit,ptask);
			Unlock();
			return ptask;
		}
	}
	// Append to end of the list
	schedule.push_back(ptask);
	Unlock();
	return ptask;
}
int SMTaskMgr::DelScheduledTask(FeedUser *fuser, uchar taskid)
{
	Lock();
	for(SCHEDULE::iterator sit=schedule.begin();sit!=schedule.end();sit++)
	{
		SCHEDENTRY *ptask=*sit;
		if((ptask->fuser==fuser)&&(ptask->taskid==taskid))
		{
			schedule.erase(sit);
			delete ptask;
			Unlock();
			return 0;
		}
	}
	Unlock();
	return -1;
}
int SMTaskMgr::CheckSchedule(long wdate, long wtime)
{
	WSDate=wdate;
	while(!schedule.empty())
	{
		Lock();
		SCHEDENTRY *ptask=schedule.front();
		// Task not expired yet: the list is sorted by date+time so no need to check further
		if((ptask->nextDate>wdate)||(ptask->nextTime>wtime))
		{
			Unlock();
			return 0;
		}
		schedule.pop_front();
		// Scheduled time only (within 5 seconds after the scheduled time)
		struct tm wtm,ntm;
		memset(&wtm,0,sizeof(struct tm));
		wtm.tm_year=wdate/10000 -1900;
		wtm.tm_mon=(wdate%10000)/100 -1;
		wtm.tm_mday=wdate%100;
		wtm.tm_hour=wtime/10000;
		wtm.tm_min=(wtime%10000)/100;
		wtm.tm_sec=wtime%100;
		wtm.tm_isdst=-1;
		time_t wts=mktime(&wtm);
		memset(&ntm,0,sizeof(struct tm));
		ntm.tm_year=ptask->nextDate/10000 -1900;
		ntm.tm_mon=(ptask->nextDate%10000)/100 -1;
		ntm.tm_mday=ptask->nextDate%100;
		ntm.tm_hour=ptask->nextTime/10000;
		ntm.tm_min=(ptask->nextTime%10000)/100;
		ntm.tm_sec=ptask->nextTime%100;
		ntm.tm_isdst=-1;
		time_t nts=mktime(&ntm);
		time_t tdiff=wts -nts;
		if((!ptask->sto)||((tdiff>=0)&&(tdiff<=5)))
		{
			if(ptask->cbfunc)
			#ifdef SMTASK_STRINGKEY
				ptask->cbfunc(ptask->fuser,"",ptask->taskid,ptask->rid,ptask->udata,0);
			#else
				ptask->cbfunc(ptask->fuser,0,ptask->taskid,ptask->rid,ptask->udata,0);
			#endif
		}
		// Reschedule repeating tasks
		if(ptask->repeat)
		{
			int nextDate=NextSchedDate(ptask->days,false);
			AddScheduledTask(ptask->fuser,ptask->taskid,ptask->udata,ptask->cbfunc,
				nextDate,ptask->nextTime,ptask->repeat,ptask->sto,ptask->days,wdate,wtime);
		}
		delete ptask;
		Unlock();
	}
	return 1;
}
