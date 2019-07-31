
#include "stdafx.h"
#ifdef AWAY_FIXSERVER
#include "dbcommon.h"
//#include "MESSAGE.H"

#pragma pack(push,8)
// TODO: BIGTRADEREC is probable not 64-bit compatible!!!

// 762 : New Order Status to FIXSERVER (64-bit compatible version)
typedef struct tdMsg762Rec
{
	int ReportLogNo;
	int RejectReasonNo;
	char RejectReasonText[128];
	BIGTRADEREC BigTradeRec;
} MSG762REC;

// 999 : TradeSvr IP Notification (64-bit compatible version)
typedef struct tdMsg999Rec
{
	char Domain[DOMAIN_LEN];
	char IP[20];	
	
	// +DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	// made this a union for clarity, its FE's port but didnt want to break other apps
	union 
	{
		int  fePort;
		int  Port;
	};
	// -DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	char OmsServerName[DOMAIN_LEN];
	char ParentDomain[16];
	char ServerName[DOMAIN_LEN];
	char Done;
	char EntDomain[DOMAIN_LEN];
	char SpareBytes[3];
	union {
		int DomainFlag;
		struct
		{
			bool EnhancedSecurity : 1;	// is used, sent in 1960 msg
		};
	};
	unsigned char PasswordLifetime;// is used, sent in 1960 msg
	char spare[3];	// DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	int  boPort;	// DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
	char Res2[16];	// DT11462 - kma - 20120511 - Update CL to send Port Info to DNS
} MSG999REC;
#pragma pack(pop)

int ViewServer::TranslateAwayMsg(MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	int Size=MsgHeader->MsgLen;
	FixStats *ustats=(FixStats *)UgrPort[PortNo].DetPtr3;
	if(ustats)
	{
		ustats->msgCnt+=Size; ustats->totMsgCnt+=Size;
	}

	switch(MsgHeader->MsgID)
	{
	case 16:
		UgrPort[PortNo].BeatsIn++;
		return 1;
	// Write drop messages from TIAD or regular FIXServer to the USC file
	case 761:
	case 763:
	{
		int UscPortNo=(int)(LONGLONG)UgrPort[PortNo].DetPtr4;
		if((UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS))
		{
			VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
			if(dfile)
			{
			#ifndef NO_FIX_WRITES
				LONGLONG doff=-1;
				odb.WriteDetailBlock(dfile,(char*)MsgHeader,sizeof(MSGHEADER),doff);
				doff=-1;
				odb.WriteDetailBlock(dfile,Msg,Size,doff);
			#endif
			}
		}
		//UgrPort[PortNo].DetPtr1=(void*)(LONGLONG)(sizeof(MSGHEADER) +Size);
		//UgrPort[PortNo].PacketsIn++;

		// The ack is sent regardless of whether the drop actually made it to Redi
		if(MsgHeader->MsgLen>=sizeof(BIGTRADEREC))
		{
			MSG762REC Msg762Rec;
			memset(&Msg762Rec,0,sizeof(MSG762REC));
			int& ReportLogNo=(int&)UgrPort[PortNo].DetPtr5;
			Msg762Rec.ReportLogNo=++ReportLogNo;
			memcpy(&Msg762Rec.BigTradeRec,Msg,sizeof(BIGTRADEREC));
			if(Msg762Rec.BigTradeRec.TradeRec.LastStatus==TRADE_PLACED)
			{
				// Pending New
				Msg762Rec.BigTradeRec.TradeRec.LastStatus=TRADE_RECEIVED;
				WSUgcSendMsg(762,sizeof(MSG762REC),(char*)&Msg762Rec,UgrPort[PortNo].UgcPort,UgrPort[PortNo].GDId.LineId);
				// Confirmed
				Msg762Rec.BigTradeRec.TradeRec.LastStatus=TRADE_CONFIRMED;
				WSUgcSendMsg(762,sizeof(MSG762REC),(char*)&Msg762Rec,UgrPort[PortNo].UgcPort,UgrPort[PortNo].GDId.LineId);
				// Filled
				Msg762Rec.BigTradeRec.TradeRec.LastStatus=TRADE_FILLED;
				Msg762Rec.BigTradeRec.TradeRec.LastShares=Msg762Rec.BigTradeRec.TradeRec.Shares;
				Msg762Rec.BigTradeRec.TradeRec.LastPrice=Msg762Rec.BigTradeRec.TradeRec.Price;
				Msg762Rec.BigTradeRec.TradeRec.Pending=0;
				WSUgcSendMsg(762,sizeof(MSG762REC),(char*)&Msg762Rec,UgrPort[PortNo].UgcPort,UgrPort[PortNo].GDId.LineId);
			}
		}
		return 1;
	}
	// Domain list request
	case 800:
	{
		// Get domain list from CfgNote
		const char *dptr=strchr(UgcPort[UgrPort[PortNo].UgcPort].CfgNote,'$');
		if(dptr) dptr=strchr(dptr+1,'$');
		if(dptr) dptr=strchr(dptr+1,'$');
		if(dptr) dptr++;
		else break;
		char dlist[80];
		strcpy(dlist,dptr);
		MSG999REC Msg999Rec;
		for(const char *domain=strtok(dlist,",");domain;domain=strtok(0,","))
		{
			memset(&Msg999Rec,0,sizeof(MSG999REC));
			strcpy(Msg999Rec.Domain,domain);
			strcpy(Msg999Rec.ServerName,UgcPort[UgrPort[PortNo].UgcPort].bindIP);
			strcpy(Msg999Rec.EntDomain,domain);
			Msg999Rec.Done=false;
			WSUgrSendNGMsg(999,sizeof(MSG999REC),(char*)&Msg999Rec,PortNo);
		}
		memset(&Msg999Rec,0,sizeof(MSG999REC));
		Msg999Rec.Done=true;
		WSUgrSendNGMsg(999,sizeof(MSG999REC),(char*)&Msg999Rec,PortNo);
		return 1;
	}
	};
	return 0;
}
int ViewServer::ReadAwayThread(VSDetailFile *dfile)
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
			if(dfile->proto==PROTO_CLS_BTR)
			{
				MSGHEADER MsgHeader;
				memcpy(&MsgHeader,fptr,sizeof(MSGHEADER));
				if((int)(fend -fptr<(int)sizeof(MSGHEADER) +MsgHeader.MsgLen))
				{
					SleepEx(100,true); // prevent max CPU when there's no more to read
					break; // Read more from file
				}
				fendl=fptr +sizeof(MSGHEADER) +MsgHeader.MsgLen;

				DropClient *pdc=0;
				int ConPortNo=(int)(LONGLONG)UscPort[dfile->portno].DetPtr4;
				if((ConPortNo>=0)&&(ConPortNo<NO_OF_CON_PORTS)&&(ConPort[ConPortNo].DetPtr==(void*)PROTO_FIXDROP))
					pdc=(DropClient *)ConPort[ConPortNo].DetPtr5;
				// Create drop client so we can queue up outgoing messages
				if(!pdc)
				{
					DCMAP::iterator dit=dcmap.find(ConPortNo);
					if(dit==dcmap.end())
					{
						pdc=new DropClient;
						int UscPortNo=-1;
						char sess[256]={0};
						strcpy(sess,ConPort[ConPortNo].CfgNote +9);
						bool passive=false;
						char *sptr=strchr(sess,'$');
						if(sptr)
						{
							*sptr=0; UscPortNo=atoi(sptr+1);
						}
						if(strrcmp(sess,",PASSIVE"))
						{
							sess[strlen(sess) -8]=0;
							passive=true;
						}
						if(pdc->Init(sess,0,0,passive,this,(void*)(PTRCAST)MAKELONG(ConPortNo,WS_CON))<0)
						{
							delete pdc; pdc=0;
						}
						else
						{
							dcmap[ConPortNo]=pdc;
							ConPort[ConPortNo].DetPtr4=(void*)(PTRCAST)UscPortNo;
							ConPort[ConPortNo].DetPtr5=pdc;
						}
					}
					else
						pdc=dit->second;
				}

				// Use the doneaway translations for this client
				DoneAwayLookup *pdalu=0;
				DAMAP::iterator dit=damap.find(dfile->prefix);
				if(dit!=damap.end())
					pdalu=&dit->second;

				switch(MsgHeader.MsgID)
				{
				case 761:
				{
					BIGTRADEREC BigTradeRec;
					memcpy(&BigTradeRec,fptr +sizeof(MSGHEADER),sizeof(BIGTRADEREC));
					// Convert to FIX and send to Apollo
					FIXINFO dfx;
					memset(&dfx,0,sizeof(FIXINFO));
					dfx.FIXDELIM=0x01;
					// AWAY drops only get 35=D
					dfx.SetTag(8,"FIX.4.2");
					const char *account=BigTradeRec.TradeRec.Account;
					if((pdalu)&&(TRANSLATE_DONE_AWAY_ACCOUNT))
					{
						map<string,string>::iterator dait=pdalu->doneAwayAccountMap.find(account);
						if(dait!=pdalu->doneAwayAccountMap.end())
							account=dait->second.c_str();
					}
					dfx.SetTag(1,account);
					dfx.SetTag(6,BigTradeRec.TradeRec.Price);
					char cloid[32]={0};
					sprintf(cloid,"%s_%d",BigTradeRec.TradeRecA.Memo,pdc->oseq+1);
					dfx.SetTag(11,cloid);
					dfx.SetTag(14,BigTradeRec.TradeRec.Shares);
					char execid[32]={0};
					sprintf(execid,"%s-%d",BigTradeRec.TradeRecA.Memo,pdc->oseq+1);
					dfx.SetTag(17,execid);
					dfx.SetTag(20,'0');
					//dfx.SetTag(29,""); // TODO LastCapacity?
					dfx.SetTag(31,BigTradeRec.TradeRec.Price);
					dfx.SetTag(32,BigTradeRec.TradeRec.Shares);
					dfx.SetTag(35,'8');
					char ordid[32]={0};
					sprintf(ordid,"%s.%d",BigTradeRec.TradeRecA.Memo,pdc->oseq+1);
					dfx.SetTag(37,ordid);
					dfx.SetTag(38,BigTradeRec.TradeRec.Shares);
					dfx.SetTag(39,'2');
					char type=0;
					//switch(BigTradeRec.TradeRec.Type)
					//{
					//case 1: type='1'; break;
					//case 2: type='2'; break;
					//};
					//DEV-4336: The price shows up in Message Monitor when we send 
					// as limit order executions with last price in price tag
					type='2';
					if(type) dfx.SetTag(40,type);
					if(type=='2')
						dfx.SetTag(44,BigTradeRec.TradeRec.Price);

					const char *user=BigTradeRec.TradeRec.Domain;
					if(pdalu)
					{
						if(pdalu->DONE_AWAY_USER_TAG>0)
						{
							switch(pdalu->DONE_AWAY_USER_TAG)
							{
							case 1: user=BigTradeRec.TradeRec.Account; break;
							case 50: user=BigTradeRec.TradeRec.User; break;
							case 57: user=BigTradeRec.TradeRec.Domain; break;
							};
						}
						else if(TRANSLATE_DONE_AWAY_USER)
						{							
							map<string,string>::iterator duit=pdalu->doneAwayUserMap.find(user);
							if(duit==pdalu->doneAwayUserMap.end())
							{
								duit=pdalu->doneAwayUserMap.find("default");
								if(duit==pdalu->doneAwayUserMap.end())
									user=duit->second.c_str();
							}
							else
								user=duit->second.c_str();
						}
					}
					dfx.SetTag(50,user);
					dfx.SetTag(10500,user);

					char side=0;
					switch(BigTradeRec.TradeRec.Action)
					{
					case 1: side='1'; break;
					case 2: side='2'; break;
					case 3: side='5'; break;
					};
					if(side) dfx.SetTag(54,side);
					dfx.SetTag(55,BigTradeRec.TradeRec.ExSymbol.Symbol);
					dfx.SetTag(59,'0');
					char ts[32]={0};
					sprintf(ts,"%08d-%02d:%02d:%02d",WSDate,WSTime/10000,(WSTime%10000)/100,WSTime%100);
					dfx.SetTag(60,ts);
					if(BigTradeRec.TradeRec.PrefMMID[0])
						dfx.SetTag(76,BigTradeRec.TradeRec.PrefMMID);
					if(BigTradeRec.TradeRec.User[0])
						dfx.SetTag(109,BigTradeRec.TradeRec.User);
					dfx.SetTag(150,'2');
					dfx.SetTag(151,'0');
					//dfx.SetTag(439,""); TODO ClearingFirm?
					if((BigTradeRec.TradeRec.PrefMMYN)&&(BigTradeRec.TradeRec.PrefMMID[0])&&(stricmp(BigTradeRec.TradeRec.PrefMMID,"AWAY")!=0))
						dfx.SetTag(113,'Y'); // doneaway type Floor (to be cleared)
					else
						dfx.SetTag(113,'N'); // doneaway type Quick Insert
					if(!pdc)
					{
						// Create drop client so we can queue up outgoing messages
						char sstr[2]={dfx.TagChar(54),0};
						WSLogError("CON%d FIX session not found to drop (%s,%s,%s,%s,%s,%s,%.4f,%d)",ConPortNo,
							dfx.TagStr(50),dfx.TagStr(dfx.GetTag(10600)?10600:1),dfx.TagStr(11),dfx.TagStr(17),
							sstr,dfx.TagStr(55),dfx.TagFloat(31),dfx.TagInt(32));
						break;
					}
					// SendFixMsg will encode and add to the FIX log, even when the session isn't connected
					pdc->SendFixMsg(dfx);
					break;
				}
				};
				fptr=fendl;
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
#endif
