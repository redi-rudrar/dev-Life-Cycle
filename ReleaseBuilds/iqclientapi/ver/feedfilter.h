
#ifndef _FEEDFILTER_H
#define _FEEDFILTER_H

#include <map>
using namespace std;

#ifdef MAKE_STATIC
	#define FEEDEXPORT
#else
	#ifdef MAKE_MODULE
	#define FEEDEXPORT __declspec(dllexport)
	#else
	#define FEEDEXPORT __declspec(dllimport)
	#endif
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

// Fast feed filter
class FeedFilter
{
public:
	FeedFilter()
	{
		memset(FilterExchList,false,32*27);
		memset(FilterLtrs,true,27);
	}

	inline int EnableRegZero(uchar exch) {return EnableRegZero(exch,true);}
	inline int DisableRegZero(uchar exch) {return EnableRegZero(exch,false);}
	inline int Enable(uchar exch, uchar region) {return Enable(exch,region,true);}
	inline int Disable(uchar exch, uchar region) {return Enable(exch,region,false);}
	inline bool IsDisabled(uchar exch, uchar region) {return !IsEnabled(exch,region);}
	inline void EnableAll() {memset(FilterExchList,true,32*27);}
	inline void DisableAll() {memset(FilterExchList,false,32*27);}
	inline bool IsEnabled(uchar exch, uchar region)
	{
		if((exch<1)||(exch>31)||((region)&&((region<'a')||(region>'z'))))
			return false;
		if(region)
			return FilterExchList[(exch *27) +(region -'a' +1)];
		else
			return FilterExchList[exch *27];
	}

	inline int EnableLtr(char ltr) {return EnableLtr(ltr,true);}
	inline int DisableLtr(char ltr) {return EnableLtr(ltr,false);}
	inline void EnableAllLtrs() {memset(FilterLtrs,true,27);}
	inline void DisableAllLtrs() {memset(FilterLtrs,false,27);}
	inline bool IsLtrEnabled(char ltr)
	{
		int lidx=26;
		if((ltr>='A')&&(ltr<='Z'))
			lidx=ltr -'A';
		else if((ltr>='a')&&(ltr<='z'))
			lidx=ltr -'a';
		return FilterLtrs[lidx];
	}

protected:
	bool FilterExchList[32*27];
	bool FilterLtrs[27];

	int Enable(uchar exch, uchar region, bool state)
	{
		if((exch<1)||(exch>31)||((region)&&((region<'a')||(region>'z'))))
			return -1;
		if(region)
			FilterExchList[(exch *27) +(region -'a' +1)]=state;
		else
			memset(&FilterExchList[exch *27],state,27);
		return 0;
	}
	int EnableRegZero(uchar exch, bool state)
	{
		if((exch<1)||(exch>31))
			return -1;
		FilterExchList[exch *27 +0]=state;
		return 0;
	}
	int EnableLtr(char ltr, bool state)
	{
		int lidx=26;
		if((ltr>='A')&&(ltr<='Z'))
			lidx=ltr -'A';
		else if((ltr>='a')&&(ltr<='z'))
			lidx=ltr -'a';
		FilterLtrs[lidx]=state;
		return 0;
	}
};

// Per feed statistics
class FeedStats
{
public:
	FeedStats()
		:Exchange(0)
		,Region(0)
		,Letter1(0)
		,Letter2(0)
		,msgCnt(0)
		,lvl1Cnt(0)
		,lvl2Cnt(0)
		,tradeCnt(0)
		,shareCnt(0)
		,skipCnt(0)
		,totMsgCnt(0)
		,totLvl1Cnt(0)
		,totLvl2Cnt(0)
		,totTradeCnt(0)
		,totShareCnt(0)
		,totSkipCnt(0)
		,maxMsgCnt(0)
		,maxLvl1Cnt(0)
		,maxLvl2Cnt(0)
		,maxTradeCnt(0)
		,maxShareCnt(0)
		,maxSkipCnt(0)
		,avgMsgCnt(0)
		,avgLvl1Cnt(0)
		,avgLvl2Cnt(0)
		,avgTradeCnt(0)
		,avgShareCnt(0)
		,avgSkipCnt(0)
		,avgTick(0)
		,lastQuoteTime(0)
	#ifdef _DEBUG
		,mapFindCnt(0)
		,mapAddCnt(0)
	#endif
	{
	}
	inline void ResetCounts()
	{
		msgCnt=0;
		lvl1Cnt=0;
		lvl2Cnt=0;
		tradeCnt=0;
		shareCnt=0;
		skipCnt=0;
		#ifdef _DEBUG
		mapFindCnt=0;
		mapAddCnt=0;
		#endif
	}
	inline void ResetTotals()
	{
		totMsgCnt=0;
		totLvl1Cnt=0;
		totLvl2Cnt=0;
		totTradeCnt=0;
		totShareCnt=0;
		totSkipCnt=0;
	}
	inline void ResetMax()
	{
		maxMsgCnt=0;
		maxLvl1Cnt=0;
		maxLvl2Cnt=0;
		maxTradeCnt=0;
		maxShareCnt=0;
		maxSkipCnt=0;
	}
	inline void ResetAverages()
	{
		avgMsgCnt=0;
		avgLvl1Cnt=0;
		avgLvl2Cnt=0;
		avgTradeCnt=0;
		avgShareCnt=0;
		avgSkipCnt=0;
		avgTick=0;
	}
	inline void ResetAll()
	{
		ResetCounts();
		ResetTotals();
		ResetMax();
		ResetAverages();
	}
	inline void CheckMax()
	{
		if(msgCnt>maxMsgCnt) maxMsgCnt=msgCnt;
		if(lvl1Cnt>maxLvl1Cnt) maxLvl1Cnt=lvl1Cnt;
		if(lvl2Cnt>maxLvl2Cnt) maxLvl2Cnt=lvl2Cnt;
		if(tradeCnt>maxTradeCnt) maxTradeCnt=tradeCnt;
		if(shareCnt>maxShareCnt) maxShareCnt=shareCnt;
		if(skipCnt>maxSkipCnt) maxSkipCnt=skipCnt;
	}
	inline void AddAverage()
	{
		avgMsgCnt+=msgCnt;
		avgLvl1Cnt+=lvl1Cnt;
		avgLvl2Cnt+=lvl2Cnt;
		avgTradeCnt+=tradeCnt;
		avgShareCnt+=shareCnt;
		avgSkipCnt+=skipCnt;
		avgTick++;
	}
	inline FeedStats& operator+=(FeedStats& fst)
	{
		msgCnt+=fst.msgCnt;
		lvl1Cnt+=fst.lvl1Cnt;
		lvl2Cnt+=fst.lvl2Cnt;
		tradeCnt+=fst.tradeCnt;
		shareCnt+=fst.shareCnt;
		skipCnt+=fst.skipCnt;

		totMsgCnt+=fst.msgCnt;
		totLvl1Cnt+=fst.lvl1Cnt;
		totLvl2Cnt+=fst.lvl2Cnt;
		totShareCnt+=fst.shareCnt;
		totSkipCnt+=fst.skipCnt;

		CheckMax();
		AddAverage();
		if(fst.lastQuoteTime>lastQuoteTime) lastQuoteTime=fst.lastQuoteTime;
		return *this;
	}

public:
	uchar Exchange;
	uchar Region;
	uchar Letter1;
	uchar Letter2;
	// Last tick
	DWORD msgCnt;
	DWORD lvl1Cnt;
	DWORD lvl2Cnt;
	DWORD tradeCnt;
	DWORD shareCnt;
	DWORD skipCnt;
	// Total per day
	__int64 totMsgCnt;
	__int64 totLvl1Cnt;
	__int64 totLvl2Cnt;
	__int64 totTradeCnt;
	__int64 totShareCnt;
	__int64 totSkipCnt;
	// Max per day
	DWORD maxMsgCnt;
	DWORD maxLvl1Cnt;
	DWORD maxLvl2Cnt;
	DWORD maxTradeCnt;
	DWORD maxShareCnt;
	DWORD maxSkipCnt;
	// Hourly avg
	__int64 avgMsgCnt;
	__int64 avgLvl1Cnt;
	__int64 avgLvl2Cnt;
	__int64 avgTradeCnt;
	__int64 avgShareCnt;
	__int64 avgSkipCnt;
	DWORD avgTick;
	long lastQuoteTime;
	#ifdef _DEBUG
	DWORD mapFindCnt;
	DWORD mapAddCnt;
	#endif
};

typedef list<FeedStats*> FSLIST;

// Feed.ini definition
class Feed
{
public:
	Feed()
		:name()
		,feedFilter()
		,AlertNoFeed(false)
		,AlertBegin(0)
		,AlertEnd(0)
		,NoFeedInterval(0)
		,TestLocalLatency(false)
		,TestClockLatency(false)
		,LatencyThreshold(0)
		,fslist()
		,NoFeedStartTime(0)
		,LatestQuoteTime(0)
	{
	}

public:
	// Config
	string name;
	FeedFilter feedFilter;
	bool AlertNoFeed;
	long AlertBegin;
	long AlertEnd;
	long NoFeedInterval;
	bool TestLocalLatency;
	bool TestClockLatency;
	long LatencyThreshold;

	// Real-time
	FSLIST fslist;
	FeedStats tfst;
	long NoFeedStartTime;
	long LatestQuoteTime;
};
typedef FeedStats EXCHSTATS[32*27];

class FEEDMAP :public map<string,Feed*>
{
public:
	FEEDEXPORT int LoadFeedIni(EXCHSTATS& exchStats);
	FEEDEXPORT void FreeFeedIni();

	inline virtual void WSLogError(const char *format,...){}
	inline virtual void WSLogEvent(const char *format,...){}
};

#endif//_FEEDFILTER_H
