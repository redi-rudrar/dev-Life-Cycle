// FIX parsing statistics
#ifndef _FIXSTATS_H
#define _FIXSTATS_H

// Metrics
class FixStats
{
public:
	FixStats()
		:msgCnt(0),totMsgCnt(0),maxMsgCnt(0),avgMsgCnt(0),cntMsgCnt(0)
		,totFix35_D(0),totFix35_F(0),totFix35_G(0),totFix35_8(0),totFix150_4(0)
		,totKeyLen(0),maxKeyLen(0),avgKeyLen(0),cntKeyLen(0)
		,totFixTag(0),maxFixTag(0),avgFixTag(0),cntFixTag(0)
		,totFixLen(0),maxFixLen(0),avgFixLen(0),cntFixLen(0)
		//,totOrders(0)
		,lastSendTime(0)
	{
	}

	DWORD msgCnt;
	DWORD maxMsgCnt,avgMsgCnt,cntMsgCnt;
	LONGLONG totMsgCnt;
	DWORD maxKeyLen,avgKeyLen,cntKeyLen;
	LONGLONG totKeyLen;
	DWORD maxFixTag,avgFixTag,cntFixTag;
	LONGLONG totFixTag;
	DWORD maxFixLen,avgFixLen,cntFixLen;
	LONGLONG totFixLen;
	DWORD totFix35_D,totFix35_F,totFix35_G,totFix35_8,totFix150_4;
	//DWORD totOrders;
	DWORD lastSendTime;

	// All averages
	void SetAverages()
	{
		avgMsgCnt=(DWORD)(totMsgCnt/(cntMsgCnt ?cntMsgCnt :1));
		avgKeyLen=(DWORD)(totKeyLen/(cntKeyLen ?cntKeyLen :1));
		avgFixTag=(DWORD)(totFixTag/(cntFixTag ?cntFixTag :1));
		avgFixLen=(DWORD)(totFixLen/(cntFixLen ?cntFixLen :1));
	}
	// All stats
	void ResetAll()
	{		
		totMsgCnt=msgCnt=maxMsgCnt=avgMsgCnt=cntMsgCnt=0;
		totFix35_D=totFix35_F=totFix35_G=totFix35_8=totFix150_4=0;
		totKeyLen=maxKeyLen=avgKeyLen=cntKeyLen=0;
		totFixTag=maxFixTag=avgFixTag=cntFixTag=0;
		totFixLen=maxFixLen=avgFixLen=cntFixLen=0;
		//totOrders=0;
		lastSendTime=0;
	}
	FixStats& operator+=(FixStats& fs)
	{
		msgCnt+=fs.msgCnt;
		//totMsgCnt+=fs.totMsgCnt;
		totMsgCnt+=fs.msgCnt;
		//if(msgCnt>maxMsgCnt)
		//	maxMsgCnt=msgCnt;
		totFix35_D+=fs.totFix35_D;
		totFix35_D+=fs.totFix35_F;
		totFix35_D+=fs.totFix35_G;
		totFix35_D+=fs.totFix35_8;
		totFix35_D+=fs.totFix150_4;
		totKeyLen+=fs.totKeyLen;
		if(fs.maxKeyLen>maxKeyLen)
			maxKeyLen=fs.maxKeyLen;
		totFixTag+=fs.totFixTag;
		if(fs.maxFixTag>maxFixTag)
			maxFixTag=fs.maxFixTag;
		totFixLen+=fs.totFixLen;
		if(fs.maxFixLen>maxFixLen)
			maxFixLen=fs.maxFixLen;
		//totOrders+=fs.totOrders;
		return *this;
	}
};

#endif//_FIXSTATS_H
