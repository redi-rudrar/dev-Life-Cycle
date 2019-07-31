
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "iqosOrder.h"

//DT9044
#include <time.h>

#if !defined(WIN32)&&!defined(_CONSOLE)
#include <dirent.h>
#endif

//DT9044: Make playbacks easier
extern int WildCardCompare(const char *wildcardStr, const char *matchStr);

#pragma pack(push,8)
#include "MESSAGE.H"

REGIONGROUP getRegionGroupFromString(const char* str)
{
	if(!str) return RG_UNKNOWN;
	if(!strcmp(str,"24E")) return RG_24E;
	if(!strcmp(str,"24O")) return RG_24O;
	if(!strcmp(str,"AS"))  return RG_AS;
	if(!strcmp(str,"HKE")) return RG_HKE;
	if(!strcmp(str,"HKO")) return RG_HKO;
	if(!strcmp(str,"LN"))  return RG_LN;
	if(!strcmp(str,"RE"))  return RG_RE;
	if(!strcmp(str,"REE")) return RG_REE;
	if(!strcmp(str,"REO")) return RG_REO;
	if(!strcmp(str,"RT"))  return RG_RT;
	if(!strcmp(str,"RTE")) return RG_RTE;
	if(!strcmp(str,"RTO")) return RG_RTO;
	if(!strcmp(str,"US"))  return RG_US;
	return RG_UNKNOWN;
}

typedef struct GarDel
{
	long ReportLogNo;
	int MsgNumber;
	int Size;
	void * Data;

} GMSG;
#pragma pack(pop)

class CLBackupFile
{
public:
	CLBackupFile()
		:lfp(0)
		,flog(false)
	{
		memset(remotePath,0,MAX_PATH);
		memset(localPath,0,MAX_PATH);
		memset(&Msg822Rec,0,sizeof(MSG822REC));
		memset(&Msg950Rec,0,sizeof(MSG950REC));
	}
	~CLBackupFile()
	{
		if(lfp)
		{
			fclose(lfp); lfp=0;
		}
	}

	char remotePath[MAX_PATH];
	char localPath[MAX_PATH];
	FILE *lfp;
	bool flog;
	MSG822REC Msg822Rec;
	MSG950REC Msg950Rec;
};
typedef map<string,CLBackupFile *> CLBACKUPMAP;

class CLBackupClient
{
public:
	CLBackupClient()
		:wsdate(0)
		,LastReportLogNo(0)
	{
		memset(remoteBase,0,MAX_PATH);
		memset(localBase,0,MAX_PATH);
		memset(iniLocation,0,MAX_PATH);
	}
	~CLBackupClient()
	{
		for(CLBACKUPMAP::iterator fit=fmap.begin();fit!=fmap.end();fit++)
			delete fit->second;
		fmap.clear();
	}

	int wsdate;
	long LastReportLogNo;
	char remoteBase[MAX_PATH];
	char localBase[MAX_PATH];
	char iniLocation[MAX_PATH];
	CLBACKUPMAP fmap;
};
int ViewServer::CLBackupInit(int UscPortNo,char* prefix)
{
	CLBackupClient *cbc=new CLBackupClient;
	cbc->wsdate=WSDATE ?WSDATE :WSDate;
	cbc->LastReportLogNo=0;
	sprintf(cbc->iniLocation,"%s_SyncPoint.ini",prefix);
	while(char* semi=strchr(cbc->iniLocation,';'))
		*semi=':';
	FILE* fp=fopen(cbc->iniLocation,"r");
	if(!fp)
	{
		WSLogEvent("Unable to open %s (New connection?).  LastReportLogNo set to 0",cbc->iniLocation);
	}
	else
	{
		char buff[80]={0};
		while(fgets(buff,80,fp))
		{
			int date=0;
			int rln=0;
			sscanf(buff,"%d:%d",&date,&rln);
			if(cbc->wsdate==date)
				cbc->LastReportLogNo=rln;
		}
		fclose(fp);
	}
	UscPort[UscPortNo].DetPtr6=cbc;
	return 0;
}
int ViewServer::CLBackupShutdown(int UscPortNo)
{
	CLBackupTimeChange(UscPortNo);
	CLBackupClient *cbc=(CLBackupClient *)UscPort[UscPortNo].DetPtr6;
	if(cbc)
	{
		delete cbc; UscPort[UscPortNo].DetPtr6=0;
	}
	return 0;
}
int ViewServer::CLBackupOpened(int UsrPortNo)
{
	CLBackupClient *cbc=(CLBackupClient *)UscPort[UsrPort[UsrPortNo].UscPort].DetPtr6;
	if(!cbc)
		return -1;
	WSLogEvent("USR%d: SyncPoint(%08d,%ld)",UsrPortNo,cbc->wsdate,cbc->LastReportLogNo);

	MSG717REC Msg717Rec;
	memset(&Msg717Rec,0,sizeof(MSG717REC));
	Msg717Rec.LastReportLogNo=cbc->LastReportLogNo;
	Msg717Rec.SessionDate=cbc->wsdate;
	//DT9044: Check for success and remove the 707 message per the request of the ClServer team
	if(!WSUsrSendMsg(717,sizeof(MSG717REC),(char*)&Msg717Rec,UsrPortNo))
		WSLogEvent("USR%d: WSUsrSendMsg() failed, msg717");
	return 0;
}
int ViewServer::CLBackupClosed(int UsrPortNo)
{
	return 0;
}
void ViewServer::CLBackupDateChange(int UsrPortNo)
{
	_ASSERT(false);//untested
	CLBackupClient *cbc=(CLBackupClient *)UscPort[UsrPort[UsrPortNo].UscPort].DetPtr6;
	if(!cbc)
		return;
	if(cbc->wsdate!=WSDate)
	{
		WSLogEvent("USR%d: Cleanup(%08d,%ld)",UsrPortNo,cbc->wsdate,cbc->LastReportLogNo);
		MSG718REC Msg718Rec;
		memset(&Msg718Rec,0,sizeof(MSG718REC));
		Msg718Rec.CleanUpToReportLogNo=cbc->LastReportLogNo;
		Msg718Rec.SessionDate=cbc->wsdate;
		WSUsrSendMsg(718,sizeof(MSG718REC),(char*)&Msg718Rec,UsrPortNo);

		MSG713REC Msg713Rec;
		memset(&Msg713Rec,0,sizeof(MSG713REC));
		Msg713Rec.CleanUpToReportLogNo=cbc->LastReportLogNo;
		WSUsrSendMsg(713,sizeof(MSG713REC),(char*)&Msg713Rec,UsrPortNo);

		cbc->wsdate=WSDate;
		cbc->LastReportLogNo=0;
		WSCloseUsrPort(UsrPortNo);
	}
}
void ViewServer::CLBackupTimeChange(int UsrPortNo)
{
	CLBackupClient *cbc=(CLBackupClient *)UscPort[UsrPort[UsrPortNo].UscPort].DetPtr6;
	if(cbc)
	{
		sprintf(UsrPort[UsrPortNo].Status,"%08d,%ld",cbc->wsdate,cbc->LastReportLogNo);

		if((this->WSTime % 10)==0) // Update the ReportLogNo file every 10 seconds
		{
			MSG713REC Msg713Rec;
			memset(&Msg713Rec,0,sizeof(MSG713REC));
			Msg713Rec.CleanUpToReportLogNo=cbc->LastReportLogNo;
			WSUsrSendMsg(713,sizeof(MSG713REC),(char*)&Msg713Rec,UsrPortNo);

			FILE* fp=fopen(cbc->iniLocation,"w");
			if(!fp)
			{
				WSLogError("Error writing %s.  Unable to save LastReportLogNo's",cbc->iniLocation);
			}			
			else
			{
				fprintf(fp,"%08d:%ld\n",cbc->wsdate,cbc->LastReportLogNo);
				fclose(fp);
			}
		}
	}
}

int ViewServer::HandleUscClserverMsg(const MSGHEADER *MsgHeader, char *Msg, int UscPortNo, FixStats *pstats, LONGLONG doffset, int proto)
{
	_ASSERT((MsgHeader)&&(Msg)&&(UscPortNo>=0)&&(UscPortNo<NO_OF_USC_PORTS)&&(pstats));
	// Stats
	pstats->msgCnt++; pstats->totMsgCnt++;

	int dlen=sizeof(MSGHEADER) +MsgHeader->MsgLen;
	char mtype=0,etype=0;
	MSG758REC Msg758Rec;
	//DT9044
	ACCOUNTREC AccountRec;

	// TagStr supports no more than 16 strings on stack so copy to stack variables
	char AppInstID[20]={0};			// AppInstID(11505)
	char ClOrdID[40]={0};			// ClOrdID(11)
	char RootOrderID[40]={0};		// tag 70129
	char FirstClOrdID[40]={0};		// tag 5055
	char ClParentOrderID[40]={0};	// tag 5035
	char Symbol[32]={0};			// Symbol(55,65)
	double Price=0.0;
	char Side=0;
	char Account[20]={0};			// Account(1)
	char EcnOrderID[40]={0};		// EcnOrderID(37)
	char ClientID[24]={0};			// ClientID(109)
	char Connection[48]={0};		// Connection
	char AuxKey[AUX_KEYS_MAX_NUM*AUX_KEYS_MAX_LEN]={0}; // AuxKeys  DT9398
	LONGLONG Timestamp=0;
	int OrderDate=0;
	int OrderQty=0;
	int CumQty=0;
	int LeavesQty=0;
	char OrigClOrdID[40]={0};
	int LastQty=0;
	char ExecID[40]={0};

	switch(MsgHeader->MsgID)
	{
	case 16:
		return 0;
	// $CLDROP$ messages
	// Old messsages retired
	case 752: // execution report (TRADEREC only) from CLServer
	{
		TRADEREC Msg752Rec;
		if(MsgHeader->MsgLen<sizeof(TRADEREC))
			break;
		memcpy(&Msg752Rec,Msg,sizeof(TRADEREC));
		_ASSERT(false);
		return -1;
	}
	case 756: // execution report w/ TRADEREC from CLServer
	{
		MSG756REC Msg756Rec;
		if(MsgHeader->MsgLen<sizeof(MSG756REC))
			break;
		memcpy(&Msg756Rec,Msg,sizeof(MSG756REC));
		_ASSERT(false);
		return -1;
	}
	case 758: // execution report w/ BIGTRADEREC from CLServer
	case 759: // sync'ed MASTER execution report w/ BIGTRADEREC from CLServer
	{
		if(MsgHeader->MsgLen<sizeof(MSG758REC))
			break;
		memcpy(&Msg758Rec,Msg,sizeof(MSG758REC));
		mtype='8',etype='1';
		break;
	}
#ifdef SINGLE_CLBACKUP_FILE
	// $CLBACKUP$ messages
	case 707:
	case 719:
	case 826:
	case 951:
		return 0;
	// TODO: send indexes
	case 822:
	{
		MSG822REC Msg822Rec;
		if(MsgHeader->MsgLen<sizeof(MSG822REC))
			return -1;
		memcpy((char*)&Msg822Rec,Msg,sizeof(Msg822Rec));
		return 0;
	}
	case 950:
	{
		MSG950REC Msg950Rec;
		if(MsgHeader->MsgLen<sizeof(MSG950REC))
			return -1;
		memcpy((char*)&Msg950Rec,Msg,sizeof(Msg950Rec));
		char *Data=0;
		if(Msg950Rec.DataLen>0)
			Data=Msg +sizeof(MSG950REC);
		switch(Msg950Rec.FileType)
		{
		case BA_FT_USER: // 1
		case BA_FT_USR_ACC: // 3
			return 0;
		case BA_FT_ACC: // 2
		{
			//DT9044: Support BA_FT_ACC (IQ AccountRec data)
			if(Msg950Rec.DataLen<sizeof(ACCOUNTREC))
				return 0;

			memcpy(&AccountRec,Data,sizeof(ACCOUNTREC));

			if(!AccountRec.Domain[0])
			{
			#ifdef _DEBUG
				// Don't fill up the event log with this
				WSLogEvent("USC%d MSG950REC received. FileType (BA_FT_ACC) with no domain",UscPortNo);
			#endif
				return -1;
			}
			if(!AccountRec.Account[0])
			{
				WSLogEvent("USC%d MSG950REC received. FileType (BA_FT_ACC) with no account",UscPortNo);
				return -1;
			}

			strncpy(AppInstID,AccountRec.Domain,sizeof(AppInstID) -1);
			strncpy(Account,AccountRec.Account,sizeof(Account) -1);

			// Place a special value in the AuxKey so the hub knows this an IQ AccountRec
			strcpy(AuxKey,VSDB_ACCOUNTREC_IND);

			time_t rawtime;
			time(&rawtime);
			tm* ptm=gmtime(&rawtime);
			Timestamp=(LONGLONG)(atoi(WScDate))*1000000 +(ptm->tm_hour*10000) +(ptm->tm_min*100) +ptm->tm_sec;
		#ifdef MULTI_DAY
			OrderDate=(int)(Timestamp/1000000);
		#endif
		#ifdef MULTI_RECTYPES
			mtype=0x04; // iqaccount
			sprintf(ClOrdID,"%s.%s",AccountRec.Domain,AccountRec.Account);
		#endif
			break;
		}
		case BA_FT_TRADE: // 4
		{
			if(Msg950Rec.DataLen!=sizeof(BIGTRADEREC))
				return 0;
			memcpy(&Msg758Rec.BigTradeRec,Data,sizeof(BIGTRADEREC));
		#ifdef _DEBUG
			WSLogEvent("Received BA_FT_TRADE message %s at %I64d",Msg758Rec.BigTradeRec.TradeRec.OrderNo,doffset);
		#endif
			switch(Msg758Rec.BigTradeRec.TradeRec.LastStatus)
			{
			case CANCEL_SENT:
			case CANCEL_RECEIVED:
			case CANCEL_PLACED:
				mtype='F'; etype=0;
				break;
			case TRADE_RECEIVED:
			case TRADE_PLACED:
				mtype='D'; etype=0;
				break;
			case TRADE_CONFIRMED:
				mtype='8'; etype='0';
				break;
			case TRADE_PARTFILL:
				mtype='8'; etype='1';
				break;
			case TRADE_FILLED:
				mtype='8'; etype='2';
			#ifdef MULTI_RECTYPES
				if(strncmp(Msg758Rec.BigTradeRec.TradeRec.OrderNo,Msg758Rec.BigTradeRec.TradeRec.FileDate,8)!=0)
					mtype=0x05; // iqpos
			#endif
				break;
			case TRADE_EXPIRED:
				mtype='8'; etype='3';
				break;
			case TRADE_CANCELED:
				mtype='8'; etype='4';
				break;
			case TRADE_CANCELPENDING:
				mtype='8'; etype='6';
				break;
			case TRADE_REJECTED:
				mtype='8'; etype='8';
				break;
			// TODO: Translate these statuses
			//#define TRADE_STOPPED		(-8) 	// Order stopped waiting on fill
			// 9:Suspended, A:Pending New
			// B:Calculated, C:Expired
			// 100 : Cancel Rejected by ECN
			// 101 : Cancel Rejected by PARSER - OVER CANCEL
			// 102 : Cancel Rejected by PARSER - ORDER NOT CONFIRMED
			// 103 : Cancel Rejected by PARSER - ORDER NOT FOUND
			// 104 : Cancel Rejected by ECN - Cancel/Replace not allowed DT3347
			default:
				return 0;
			};
			// Don't get here unless the BigTradeRec has been filled out
			if(!Msg758Rec.BigTradeRec.TradeRec.Domain[0])
			{
				WSLogEvent("USC%d MSG950REC received. FileType (BA_FT_TRADE) with no domain",UscPortNo);
				return 0;
			}
			if(!Msg758Rec.BigTradeRec.TradeRec.OrderNo[0])
			{
				WSLogEvent("USC%d MSG950REC received. FileType (BA_FT_TRADE) with no OrderNo",UscPortNo);
				return 0;
			}

			strncpy(AppInstID,Msg758Rec.BigTradeRec.TradeRec.Domain,sizeof(AppInstID) -1);
			//DT9044: All orders are in the same AppSys, the ClOrdID needs to be <domain>_OrderNo
			sprintf_s(ClOrdID,sizeof(ClOrdID)-1,"%s_%s",Msg758Rec.BigTradeRec.TradeRec.Domain,Msg758Rec.BigTradeRec.TradeRec.OrderNo);
			//if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
			strncpy(RootOrderID,Msg758Rec.BigTradeRec.TradeRecB.ClientOriginalOrderId,sizeof(RootOrderID) -1);
			strncpy(ClParentOrderID,Msg758Rec.BigTradeRec.TradeRecA.LinkOrderNo,sizeof(ClParentOrderID) -1);
			strncpy(Symbol,Msg758Rec.BigTradeRec.TradeRec.ExSymbol.Symbol,sizeof(Symbol) -1);
			Price=(double)Msg758Rec.BigTradeRec.TradeRec.Price;
			switch(Msg758Rec.BigTradeRec.TradeRec.Action)
			{
			case 1: Side='1'; break;// 1:buy
			case 2: Side='2'; break;// 2:Sell
			case 3: Side='5'; break;// 3:Short 
			// 4:Exercise Option
			default: Side=0;
			};
			strncpy(Account,Msg758Rec.BigTradeRec.TradeRec.Account,sizeof(Account) -1);
			// e.g. ECNOrderNo=10:53:54 on placed, but _TIAD0119 on confirmed
			if(Msg758Rec.BigTradeRec.TradeRec.LastStatus==TRADE_CONFIRMED)
				strncpy(EcnOrderID,Msg758Rec.BigTradeRec.TradeRec.ECNOrderNo,sizeof(EcnOrderID) -1);
			if(Msg758Rec.BigTradeRec.TradeRecB.Portfolio[0])
				strncpy(ClientID,Msg758Rec.BigTradeRec.TradeRecB.Portfolio,sizeof(ClientID) -1);
			else if(Msg758Rec.BigTradeRec.TradeRec.User[0])
				//strncpy(ClientID,Msg758Rec.BigTradeRec.TradeRec.User,sizeof(ClientID) -1);
				_snprintf(ClientID,sizeof(ClientID) -1,"%s.%s",Msg758Rec.BigTradeRec.TradeRec.Domain,Msg758Rec.BigTradeRec.TradeRec.User);
			strncpy(Connection,Msg758Rec.BigTradeRec.TradeRec.PrefECN,sizeof(Connection) -1);
			sprintf_s(AuxKey,sizeof(AuxKey)-1,"%ld",Msg758Rec.BigTradeRec.TradeRec.TradeDetRef);

			if(Msg758Rec.BigTradeRec.TradeRec.CancelRequested)
				mtype='F';
			else if(!etype)
			{
				if(Msg758Rec.BigTradeRec.TradeRec.Canceled)
					etype='4';
				else if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
					etype='5';
			}
			OrderQty=Msg758Rec.BigTradeRec.TradeRec.Shares;
			CumQty=Msg758Rec.BigTradeRec.TradeRec.ExeShares;
			LeavesQty=Msg758Rec.BigTradeRec.TradeRec.Pending;;
			int hh=0,mm=0,ss=0;
			if(Msg758Rec.BigTradeRec.TradeRec.LastTime[0])
			{
				sscanf(Msg758Rec.BigTradeRec.TradeRec.LastTime,"%02d:%02d:%02d",&hh,&mm,&ss);
				Timestamp=(LONGLONG)(atoi(Msg758Rec.BigTradeRec.TradeRec.LastDate))*1000000 +(hh*10000) +(mm*100) +ss;
			}
			else if(Msg758Rec.BigTradeRec.TradeRec.OrderNo[8]=='-')
				Timestamp=(LONGLONG)(atoi(Msg758Rec.BigTradeRec.TradeRec.OrderNo))*1000000;
		#ifdef MULTI_DAY
			// Sometimes the transaction date is the current date, but orderno is previous date for a position (more accurate)
			//if((myisdigit(Msg758Rec.BigTradeRec.TradeRec.OrderNo[0]))&&(Msg758Rec.BigTradeRec.TradeRec.OrderNo[8]=='-'))
			//	OrderDate=myatoi(Msg758Rec.BigTradeRec.TradeRec.OrderNo);
			//else
			//	OrderDate=(int)(Timestamp/1000000);
			OrderDate=myatoi(Msg758Rec.BigTradeRec.TradeRec.FileDate);
		#endif
			LastQty=Msg758Rec.BigTradeRec.TradeRec.LastShares;
			strncpy(ExecID,Msg758Rec.BigTradeRec.TradeRec.ECNExecID,sizeof(ExecID) -1);

			// Link C/R chains
			strncpy(FirstClOrdID,ClOrdID,sizeof(FirstClOrdID)-1);
			map<string,string>::iterator fit=fm->find(ClOrdID);
			if(fit!=fm->end())
				strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
			else
			{
				char OrigClOrdID[40]={0};
				//strncpy(OrigClOrdID,Msg758Rec.BigTradeRec.TradeRecB.ClientOriginalOrderId,sizeof(RootOrderID) -1);
				if((Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)&&(mtype!='8'))
				{
					mtype='G';
					sprintf_s(OrigClOrdID,sizeof(OrigClOrdID)-1,"%s_%s-%d",Msg758Rec.BigTradeRec.TradeRec.Domain,Msg758Rec.BigTradeRec.TradeRec.FileDate,Msg758Rec.BigTradeRec.TradeRecB.GroupOrderNo);
				}
				if(*OrigClOrdID)
				{
					fit=fm->find(OrigClOrdID);
					if(fit!=fm->end())
						strncpy(FirstClOrdID,fit->second.c_str(),sizeof(FirstClOrdID)-1);
				}
				fm->insert(pair<string,string>(ClOrdID,FirstClOrdID));
			}
			break;
		}
		case BA_FT_IQOS: // 33
		{
			IqosOrder iqosOrder(Data, Msg950Rec.DataLen);
#ifdef _DEBUG
			WSLogEvent("Received IQOS Msg: %s", iqosOrder.toMsg());
#endif

//			WSLogEvent("Received %s at %I64d",Msg758Rec.BigTradeRec.TradeRec.OrderNo,doffset);
			switch(iqosOrder.getOrderStatus())
			{
			case CANCEL_SENT:
			case CANCEL_RECEIVED:
			case CANCEL_PLACED:
				mtype='F'; etype=0;
				break;
			case TRADE_RECEIVED:
			case TRADE_PLACED:
				mtype='D'; etype=0;
				break;
			case TRADE_CONFIRMED:
				mtype='8'; etype='0';
				break;
			case TRADE_PARTFILL:
				mtype='8'; etype='1';
				break;
			case TRADE_FILLED:
				mtype='8'; etype='2';
				break;
			case TRADE_EXPIRED:
				mtype='8'; etype='3';
				break;
			case TRADE_CANCELED:
				mtype='8'; etype='4';
				break;
			case TRADE_CANCELPENDING:
				mtype='8'; etype='6';
				break;
			case TRADE_REJECTED:
				mtype='8'; etype='8';
				break;
			// TODO: Translate these statuses
			//#define TRADE_STOPPED		(-8) 	// Order stopped waiting on fill
			// 9:Suspended, A:Pending New
			// B:Calculated, C:Expired
			// 100 : Cancel Rejected by ECN
			// 101 : Cancel Rejected by PARSER - OVER CANCEL
			// 102 : Cancel Rejected by PARSER - ORDER NOT CONFIRMED
			// 103 : Cancel Rejected by PARSER - ORDER NOT FOUND
			// 104 : Cancel Rejected by ECN - Cancel/Replace not allowed DT3347
			default:
				return 0;
			};
//			WSLogEvent("IQOS: mtype=%c", mtype);
			// Don't get here unless the BigTradeRec has been filled out
			if(!iqosOrder.getDomain() || strlen(iqosOrder.getDomain()) == 0)
			{
				WSLogEvent("USC%d MSG950REC received. FileType (BA_FT_IQOS) with no domain",UscPortNo);
				return 0;
			}
			if(!iqosOrder.getOrderNo() || strlen(iqosOrder.getOrderNo()) == 0)
			{
				WSLogEvent("USC%d MSG950REC received. FileType (BA_FT_IQOS) with no OrderNo",UscPortNo);
				return 0;
			}

			strncpy(AppInstID, iqosOrder.getDomain(),sizeof(AppInstID) -1);
//			WSLogEvent("IQOS: AppInstID=%s", AppInstID);
			//DT9044: All orders are in the same AppSys, the ClOrdID needs to be <domain>_OrderNo
			sprintf_s(ClOrdID,sizeof(ClOrdID)-1,"%s_%s", iqosOrder.getDomain(), iqosOrder.getOrderNo());
//			WSLogEvent("IQOS: ClOrdID=%s", ClOrdID);
			//if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
			//strncpy(FirstClOrdID,Msg758Rec.BigTradeRec.TradeRec.OrderNo,sizeof(FirstClOrdID) -1);
//TODO			strncpy(RootOrderID, Msg758Rec.BigTradeRec.TradeRecB.ClientOriginalOrderId,sizeof(RootOrderID) -1);
            if(iqosOrder.getParentOrderNo())
                strncpy(ClParentOrderID, iqosOrder.getParentOrderNo(),sizeof(ClParentOrderID) -1);
			if(iqosOrder.getSymbol())
				strncpy(Symbol, iqosOrder.getSymbol(),sizeof(Symbol) -1);
//			WSLogEvent("IQOS: Symbol=%s", Symbol);
			Price=iqosOrder.getPrice();
//			WSLogEvent("IQOS: Price=%f", Price);
			switch(iqosOrder.getSide())
			{
			case 1: Side='1'; break;// 1:buy
			case 2: Side='2'; break;// 2:Sell
			case 3: Side='5'; break;// 3:Short 
			// 4:Exercise Option
			default: Side=0;
			};
//			WSLogEvent("IQOS: Side=%c", Side);
			if(iqosOrder.getClearingAccount())
				strncpy(Account, iqosOrder.getClearingAccount(), sizeof(Account) -1);
			else if(iqosOrder.getTradingAccount())
                strncpy(Account, iqosOrder.getTradingAccount(), sizeof(Account) -1);
//			WSLogEvent("IQOS: Account=%s", Account);
            if(iqosOrder.getUser())
				strncpy(ClientID, iqosOrder.getUser(), sizeof(ClientID) -1);
//			WSLogEvent("IQOS: ClientID=%s", ClientID);
			if(iqosOrder.getRoute())
				strncpy(Connection, iqosOrder.getRoute(), sizeof(Connection) -1);
//			WSLogEvent("IQOS: Connection=%s", Connection);
			OrderQty=iqosOrder.getQty();
//			WSLogEvent("IQOS: OrderQty=%d", OrderQty);
			CumQty=iqosOrder.getQty()-iqosOrder.getTotalLeavesQty();
			LeavesQty=iqosOrder.getTotalLeavesQty();
            if(iqosOrder.getLastTime())
            {
			    int hh=0,mm=0,ss=0;
			    sscanf(iqosOrder.getLastTime(), "%02d:%02d:%02d", &hh,&mm,&ss);
			    Timestamp=(LONGLONG)(WSDate*1000000 +(hh*10000) +(mm*100) +ss);
			#ifdef MULTI_DAY
				OrderDate=(int)(Timestamp/1000000);
			#endif
            }
//			WSLogEvent("IQOS: Timestamp=%d", Timestamp);
			LastQty=iqosOrder.getFilledQty();
//			WSLogEvent("IQOS: LastQty=%d", LastQty);
            sprintf(AuxKey,"%c%s%c%s%c%d%c",AUX_KEYS_DELIM, 
                iqosOrder.getList()     ?iqosOrder.getList():"",    AUX_KEYS_DELIM,
                iqosOrder.getGroup()    ?iqosOrder.getGroup():"",   AUX_KEYS_DELIM,
                iqosOrder.getVersion(), AUX_KEYS_DELIM
                );
//			WSLogEvent("IQOS: AuxKey=%s", AuxKey);
			break;
		}
		case BA_FT_MSG: // 5
		case BA_FT_HEDGE_TRADE: // 6
		case BA_FT_ALLOC: // 7
		case BA_FT_DEF_ALLOC: // 8
		case BA_FT_REPORTLOG: // 9
		case BA_FT_INDEXDB: //  10
		case BA_FT_INLOG: // 11
		case BA_FT_OUTLOG: // 12
		case BA_FT_NEWINLOG: // 13
		case BA_FT_FULLLOG: // 14
		case BA_FT_PLACELOG: // 15
		case BA_FT_CANCELLOG: // 16
		case BA_FT_TRADELOG: // 17
		case BA_FT_USERLOG: // 18
		case BA_FT_GDLOG: // 19
		case BA_FT_OMSORDERLOG: // 20
		case BA_FT_USR_DOC: // 21
		case BA_FT_ACC_ROUTE: // 22
		case BA_FT_SHORTLOCATE_LOG: // 23
		case BA_FT_PROXY_IN_LOG: // 24
		case BA_FT_PROXY_OUT_LOG: // 25
		case BA_FT_PROXY_FULL_LOG: // 26
		case BA_FT_PROXY_RAW_LOG: // 27
		case BA_FT_SPECINLOG: // 28
		case BA_FT_SPECOUTLOG: //  29
		case BA_FT_SPECFULLLOG: // 30
		case BA_FT_STOCKLOAN: // 31
		default:
			return 0;
		};
		break;
	}
#endif//SINGLE_CLBACKUP_FILE
	default:
		WSLogEvent("Invalid message id:%d, UscPortNo:%d ",MsgHeader->MsgID,UscPortNo);
		return -1;
	};

	VSDistJournal *dj=(VSDistJournal *)UscPort[UscPortNo].DetPtr5;
	if(!dj)
		return -1;
	// Don't exceed per-journal memory limit.
	// Can do about 1.6M msgs/100MB journal memory
	if(dj->GetMemUsage()>=MAX_DISTJ_SIZE*1024*1024)
	{
		DWORD wstart=GetTickCount(),wnext=wstart +1000;
		while(dj->GetMemUsage()>=MAX_DISTJ_SIZE*1024*1024)
		{
			WSUnlockPort(WS_USC,UscPortNo,false);
			DWORD tnow=GetTickCount();
			if(tnow>=wnext)
			{
				WSLogError("USC%d: MAX_DISTJ_SIZE(%d MB) exceeded-Pausing send.",UscPortNo,MAX_DISTJ_SIZE);
				while(wnext<=tnow)
					wnext+=1000;
			}
			SleepEx(100,true);
			WSLockPort(WS_USC,UscPortNo,false);
		}
	}

	#ifndef NO_EFILLS_FILE
	if(!*ffpath)
		#ifdef WIN32
		sprintf(ffpath,"%s\\efills_%08d.msg",FILLS_FILE_DIR.c_str(),WSDate);
		#else
		sprintf(ffpath,"%s/efills_%08d.msg",FILLS_FILE_DIR.c_str(),WSDate);
		#endif
	#endif

	LONGLONG dend=doffset +dlen;

	AddJournal(dj,Msg -sizeof(MSGHEADER),dlen,doffset,dend,
			AppInstID,ClOrdID,RootOrderID,
			FirstClOrdID,ClParentOrderID,Symbol,
			Price,Side,
			Account,EcnOrderID,ClientID,
			Connection,OrderDate,AuxKey,
			mtype,etype,OrderQty,CumQty,LeavesQty, 
			Timestamp,OrigClOrdID,ExecID,LastQty,ffpath);
	return 0;
}

// For testing from log files
int ViewServer::PlayCldropLogs(int PortNo, int proto, const char *fmatch, bool& cxl, int rstart, float playRate)
{
	_ASSERT((PortNo>=0)&&(fmatch)&&(*fmatch));
	map<string,string> fmap;
	char fdir[MAX_PATH],*fname=0;
	GetFullPathNameEx(fmatch,MAX_PATH,fdir,&fname);
	if(fname) *fname=0;
#if defined(WIN32)||defined(_CONSOLE)
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(fmatch,&fdata);
	if(fhnd==INVALID_HANDLE_VALUE)
		return -1;
	do{
		if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
			;
		else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			;
		else
		{
			char fpath[MAX_PATH]={0},fkey[MAX_PATH]={0};
			sprintf(fpath,"%s%s",fdir,fdata.cFileName);

			//DT9044
			if(!WildCardCompare(fmatch, fpath))
				continue;

			strcpy(fkey,fdata.cFileName);
			char *nptr=strrchr(fkey,'.');
			if(nptr)
				sprintf(nptr+1,"%02d",atoi(nptr +1)); // to sort the extension number correctly
			fmap[fkey]=fpath;
		}
	}while(FindNextFile(fhnd,&fdata));
	FindClose(fhnd);
#else
	DIR *pdir=opendir(fmatch);
	if(!pdir)
		return -1;
	dirent *fdata=0;
	do{
		fdata=readdir(pdir);
		if(fdata)
		{
			if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
				continue;
			char fpath[MAX_PATH]={0},fkey[MAX_PATH]={0};
			sprintf(fpath,"%s%s",fdir,fdata->d_name);
			strcpy(fkey,fdata->d_name);
			char *nptr=strrchr(fkey,'.');
			if(nptr)
				sprintf(nptr+1,"%02d",atoi(nptr +1)); // to sort the extension number correctly
			fmap[fkey]=fpath;
		}
	}while(fdata);
	closedir(pdir);
#endif

	if(rstart>0)
		WSLogEvent("CON%d: Realtime playback relative to %06d...",PortNo,rstart);
	DWORD pstart=GetPlayTime(); // real playback start time
	DWORD vstart=GetPlayTime(rstart*1000); // virtual playback start time
	DWORD vtime=vstart; // current virtual time
	DWORD lnext=vstart; // next eventlog virtual time
	int fcnt=0;
	for(map<string,string>::iterator fit=fmap.begin();(fit!=fmap.end())&&(!cxl);fit++)
	{
		const char *fpath=fit->second.c_str();
		FILE *fp=fopen(fpath,"rb");
		if(fp)
		{
			fcnt++;
			fseeko(fp,0,SEEK_END);
			int fsize=ftell(fp);
			fseeko(fp,0,SEEK_SET);
			WSLogEvent("CON%d: Playing %s (%s bytes)...",PortNo,fpath,SizeStr(fsize));
			// Maximum timestamp to wait for during realtime playback
			DWORD fmax=0;
			if(rstart>0)
			{
				char sdate[5];
				strncpy(sdate,WScDate,4); sdate[4]=0;
				const char *dptr=strstr(fpath,sdate);
				if(dptr)
				{
					DWORD fts=myatoi(dptr +8)*1000;
					fmax=GetPlayTime(fts) +3600000; // 1 hour after file time
				}
			}

			char rbuf[4096]={0};
			//DT9044
			while(fread(rbuf,1,sizeof(rbuf),fp))
			{
				if((!ConPort[PortNo].SockConnected)||(ConPort[PortNo].appClosed))
				{
					WSLogEvent("CON%d: Playback aborted",PortNo);
					return -1;
				}
				WSConSendBuff(sizeof(rbuf),rbuf,PortNo,1);
				FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
				if(cstats)
				{
					cstats->msgCnt++; cstats->totMsgCnt++;
				}
			}
			fclose(fp);
		}
	}
	// Threads that initiate overlapped sends should not exit before send completion.
	// Otherwise, if no subsequent sends are issued (to get WSAECONNRESET), 
	// then those overlapped operations will never complete!
	if((ConPort[PortNo].SockConnected)&&(ConPort[PortNo].NumOvlSends>0))
	{
		WSLogEvent("CON%d: Waiting for send completion...",PortNo);
		while((ConPort[PortNo].SockConnected)&&(ConPort[PortNo].IOCPSendBytes>0))
			SleepEx(100,true);
	}
	WSLogEvent("CON%d: Playback complete",PortNo);
	return 0;
}

#ifndef WIN32
#define NO_OVL_READ_DETAILS
#endif
#ifdef NO_OVL_READ_DETAILS
int ViewServer::ReadMsgThread(VSDetailFile *dfile)
{
	return -1;
}
#else//!NO_OVL_READ_DETAILS
// Reads MSG log files
int ViewServer::ReadMsgThread(VSDetailFile *dfile)
{
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
	while(dfile->rhnd!=INVALID_HANDLE_VALUE)
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
			if((!rc)&&(err!=ERROR_IO_PENDING))
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
			if((!rc)&&(err!=ERROR_IO_PENDING))
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

		// Process each detail
		// MSG protocol
		const char *fend=rptr;
		const char *fptr;
		MSGHEADER MsgHeader;
		for(fptr=rbuf;fptr<fend;)
		{
			if(fend -fptr<sizeof(MSGHEADER))
				break;
			memcpy(&MsgHeader,fptr,sizeof(MSGHEADER));
			if(fend -fptr<(int)sizeof(MSGHEADER) +MsgHeader.MsgLen)
				break;
			// TODO: If we want 100% accurate counts, then we'll have to mutex 'ustats' against WSTimeChange
			WSLockPort(WS_USC,dfile->portno,false);
			if((dfile->proto==PROTO_CLDROP)||(dfile->proto==PROTO_CLBACKUP))
			{
				//WSLogEvent("ReadMsgThread(),Calling HandleUscClserverMsg(),ID:%d,Len:%d,PortNo:%d,Loc:%I64d,%d",MsgHeader.MsgID,MsgHeader.MsgLen,dfile->portno,dfile->rend,(int)(fptr -rbuf));
				HandleUscClserverMsg(&MsgHeader,(char*)fptr+sizeof(MSGHEADER),dfile->portno,ustats,dfile->rend.QuadPart +(int)(fptr -rbuf),dfile->proto);
			}
			WSUnlockPort(WS_USC,dfile->portno,false);
			fptr+=sizeof(MSGHEADER) +MsgHeader.MsgLen;
		}

		// Preserve leftovers
		rptr=rbuf;
		if(fptr>rbuf)
		{
			// This is the only function that modifies 'dfile->rend', so we don't need to lock read.
			_ASSERT((rbuf<=fptr)&&(fptr<=fend));
			dfile->rend.QuadPart+=(fptr -rbuf); 
			odb.SetZdirty();
			int bleft=(int)(fend -fptr);
			if(bleft>0)
			{
				memmove(rptr,fptr,bleft); rptr+=bleft;
			}
		}
		dfile->LockRead();
	}
	dfile->UnlockRead();
	for(int i=0;i<2;i++)
		delete rbufs[i];
	delete rbuf;
	return 0;
}
#endif//!NO_OVL_READ_DETAILS

int VSQuery::FilterCldropResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify)
{
	// Convert to FIX for results
	MSG758REC Msg758Rec;
	memcpy(&Msg758Rec,dptr +sizeof(MSGHEADER),sizeof(MSG758REC));
	FIXINFO ifix,*pfix=&ifix;
	ifix.Reset();
	ifix.FIXDELIM=0x01;
	ifix.noSession=true;
	ifix.supressChkErr=true;
	ifix.SetTag(35,'8');
	ifix.SetTag(150,'1');
	if(Msg758Rec.BigTradeRec.TradeRec.Canceled)
		ifix.SetTag(150,'4');
	else if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
		ifix.SetTag(150,'5');
	ifix.SetTag(11505,Msg758Rec.BigTradeRec.TradeRec.Domain);
	ifix.SetTag(11,Msg758Rec.BigTradeRec.TradeRec.OrderNo);
	ifix.SetTag(1,Msg758Rec.BigTradeRec.TradeRec.Account);
	ifix.SetTag(109,Msg758Rec.BigTradeRec.TradeRec.User);
	ifix.SetTag(40,Msg758Rec.BigTradeRec.TradeRec.Price==0.0?"1":"2");
	switch(Msg758Rec.BigTradeRec.TradeRec.Action)
	{
	case 1: ifix.SetTag(54,"1"); break;
	case 2: ifix.SetTag(54,"2"); break;
	case 3: ifix.SetTag(54,"5"); break;
	};
	ifix.SetTag(55,Msg758Rec.BigTradeRec.TradeRec.ExSymbol.Symbol);
	ifix.SetTag(44,Msg758Rec.BigTradeRec.TradeRec.Price);
	ifix.SetTag(38,(int)Msg758Rec.BigTradeRec.TradeRec.Shares);
	ifix.SetTag(14,(int)Msg758Rec.BigTradeRec.TradeRec.ExeShares);
	ifix.SetTag(151,(int)Msg758Rec.BigTradeRec.TradeRec.Pending);
	char Timestamp[32]={0};
	sprintf(Timestamp,"%s-%s",Msg758Rec.BigTradeRec.TradeRec.LastDate,Msg758Rec.BigTradeRec.TradeRec.LastTime);
	ifix.SetTag(52,Timestamp);
	//if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
	//	ifix.SetTag(41,Msg758Rec.BigTradeRec.TradeRec.OrderNo);
	if(Msg758Rec.BigTradeRec.TradeRec.LastShares>0)
	ifix.SetTag(32,(int)Msg758Rec.BigTradeRec.TradeRec.LastShares);
	ifix.SetTag(17,Msg758Rec.BigTradeRec.TradeRec.ECNExecID);
	ifix.Merge();

	// Evaluate where clause FIX conditions
	if(!where.empty())
	{
		const char *wstr=where.c_str();
		qscope.etok.pnotify=pnotify;
		//DT9398
		EvalHint ehint={0,pfix,qscope.AuxKeyNames,0};
		qscope.etok.EvalExprTree(pfix,&ehint,CaseSen);
		switch(qscope.etok.vtype)
		{
		case 'B':
			if(!qscope.etok.bval)
				return 0;
			break;
		case 'I':
			if(!qscope.etok.ival) 
				return 0;
			break;
		case 'F':
			if(qscope.etok.fval==0.0) 
				return 0;
			break;
		case 'S':
			if(!qscope.etok.sval[0]) 
				return 0;
			break;
		case 'V':
			break;
		default:
			return 0;
		};
	}
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();
	// Make a copy of the FIX string; don't just point to same memory that could go out of scope
	qresult->rfix.slen=pfix->llen;
	qresult->rfix.sbuf=new char[pfix->llen];
	memset(qresult->rfix.sbuf,0,pfix->llen);
	char *rptr=qresult->rfix.sbuf;
	const char *rend=rptr +pfix->llen,*fend=pfix->fbuf +pfix->llen;
	// Send entire FIX string
	if(select=="*")
	{
		memcpy(rptr,pfix->fbuf,pfix->llen); rptr+=pfix->llen;
	}
	// Filter only desired items
	else
	{
		// Copy only the tags in the select clause
		char sstr[1024]={0};
		strcpy(sstr,select.c_str());
		for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
		{
			int tno=myatoi(tok);
			if(!tno)
				continue;
			FIXTAG *ptag=pfix->GetTag(tno);
			if(ptag)
			{
				const char *btag=pfix->fbuf +ptag->tstart;
				const char *etag=ptag->val +ptag->vlen;
				if(rend -rptr<(int)(etag -btag) +1)
				{
					_ASSERT(false);
					break;
				}
				memcpy(rptr,btag,etag -btag); rptr+=(etag -btag);
				*rptr=pfix->FIXDELIM; rptr++;
			}
		}
		_ASSERT(rptr<rend);
		*rptr=0;
	}	
	qresult->rfix.Reset();
	qresult->rfix.FIXDELIM=pfix->FIXDELIM;
	qresult->rfix.noSession=pfix->noSession;
	qresult->rfix.supressChkErr=true;
	// We're not really using the FIX msg for query so no need to do anhy parsing
	//qresult->rfix.FixMsgReady(qresult->rfix.sbuf,(int)(rptr -qresult->rfix.sbuf));
	qresult->rfix.fbuf=qresult->rfix.sbuf;
	qresult->rfix.llen=(int)(rptr -qresult->rfix.sbuf);
	if(hist) 
		hresults.push_back(qresult);
	else 
		lresults.push_back(qresult);
	rcnt++;
	return 1;
}

int VSQuery::FilterClbackupResult(const char *dptr, int dlen, bool hist, ExprTokNotify *pnotify)
{
    MSGHEADER MsgHeader;
    memcpy(&MsgHeader, dptr, sizeof(MSGHEADER));
    if(MsgHeader.MsgID != 950)
        return -1;

    MSG950REC Msg950Rec;
    memcpy(&Msg950Rec, dptr+sizeof(MSGHEADER), sizeof(MSG950REC));
    if(binarydatatype == QDT_BINARY_IQOS && Msg950Rec.FileType == BA_FT_IQOS)
    {
        int DatLength = sizeof(MSGHEADER) + sizeof(MSG950REC) + Msg950Rec.DataLen;
        VSQueryResult *qresult=new VSQueryResult;
        //qresult->hist=hist;
        qresult->rtime=GetTickCount();

        // For BinaryDataRequests, ignore the SQL select list and send the message 
        qresult->obuf=new char[DatLength];
        memset(qresult->obuf, 0, DatLength);
        char *rptr=qresult->obuf;

        // Send the entire 950 Msg
        memcpy(rptr, dptr, DatLength);
        rptr+=DatLength;
        qresult->DataLength = DatLength;
        if(binarydatalen < DatLength)
            binarydatalen=DatLength;

        if(hist) 
            hresults.push_back(qresult);
        else 
            lresults.push_back(qresult);
        rcnt++;
        return 1;
    }
    else if(binarydatatype == QDT_BINARY_BTR && Msg950Rec.FileType != BA_FT_IQOS)
    {
	// Convert to FIX for results
	MSG758REC Msg758Rec;
	memcpy(&Msg758Rec.BigTradeRec,dptr +sizeof(MSGHEADER) +sizeof(MSG950REC),sizeof(BIGTRADEREC));
	FIXINFO ifix,*pfix=&ifix;
	ifix.Reset();
	ifix.FIXDELIM=0x01;
	ifix.noSession=true;
	ifix.supressChkErr=true;
	ifix.SetTag(35,'8');
	ifix.SetTag(150,'1');
	if(Msg758Rec.BigTradeRec.TradeRec.Canceled)
		ifix.SetTag(150,'4');
	else if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
		ifix.SetTag(150,'5');
	ifix.SetTag(11505,Msg758Rec.BigTradeRec.TradeRec.Domain);
	ifix.SetTag(11,Msg758Rec.BigTradeRec.TradeRec.OrderNo);
	ifix.SetTag(1,Msg758Rec.BigTradeRec.TradeRec.Account);
	ifix.SetTag(109,Msg758Rec.BigTradeRec.TradeRec.User);
	ifix.SetTag(40,Msg758Rec.BigTradeRec.TradeRec.Price==0.0?"1":"2");
	switch(Msg758Rec.BigTradeRec.TradeRec.Action)
	{
	case 1: ifix.SetTag(54,"1"); break;
	case 2: ifix.SetTag(54,"2"); break;
	case 3: ifix.SetTag(54,"5"); break;
	};
	
	ifix.SetTag(55,Msg758Rec.BigTradeRec.TradeRec.ExSymbol.Symbol);
	ifix.SetTag(44,Msg758Rec.BigTradeRec.TradeRec.Price);
	ifix.SetTag(38,(int)Msg758Rec.BigTradeRec.TradeRec.Shares);
	ifix.SetTag(14,(int)Msg758Rec.BigTradeRec.TradeRec.ExeShares);
	ifix.SetTag(151,(int)Msg758Rec.BigTradeRec.TradeRec.Pending);
	char Timestamp[32]={0};
	//DT10008
	if(strlen(Msg758Rec.BigTradeRec.TradeRec.LastDate)+strlen(Msg758Rec.BigTradeRec.TradeRec.LastTime)<30)
		sprintf(Timestamp,"%s-%s",Msg758Rec.BigTradeRec.TradeRec.LastDate,Msg758Rec.BigTradeRec.TradeRec.LastTime);
	ifix.SetTag(52,Timestamp);
	//if(Msg758Rec.BigTradeRec.TradeRec.ReplaceRequest)
	//	ifix.SetTag(41,Msg758Rec.BigTradeRec.TradeRec.OrderNo);
	if(Msg758Rec.BigTradeRec.TradeRec.LastShares>0)
	ifix.SetTag(32,(int)Msg758Rec.BigTradeRec.TradeRec.LastShares);
	ifix.SetTag(17,Msg758Rec.BigTradeRec.TradeRec.ECNExecID);

	//DT9044
	ifix.SetTag(2100,Msg758Rec.BigTradeRec.TradeRecB.ClientOriginalOrderId);
	ifix.SetTag(37,Msg758Rec.BigTradeRec.TradeRec.ECNOrderNo);
	ifix.SetTag(5035,Msg758Rec.BigTradeRec.TradeRecA.LinkOrderNo);
	ifix.SetTag(1999,(int)Msg758Rec.BigTradeRec.TradeRec.TradeDetRef); 
	ifix.Merge();

	// Evaluate where clause FIX conditions
	if(!where.empty())
	{
		const char *wstr=where.c_str();
		qscope.etok.pnotify=pnotify;
		EvalHint ehint={0,pfix,qscope.AuxKeyNames,0};
		qscope.etok.EvalExprTree(pfix,&ehint,CaseSen);
		switch(qscope.etok.vtype)
		{
		case 'B':
			if(!qscope.etok.bval)
				return 0;
			break;
		case 'I':
			if(!qscope.etok.ival) 
				return 0;
			break;
		case 'F':
			if(qscope.etok.fval==0.0) 
				return 0;
			break;
		case 'S':
			if(!qscope.etok.sval[0]) 
				return 0;
			break;
		case 'V':
			break;
		default:
			return 0;
		};
	}
	VSQueryResult *qresult=new VSQueryResult;
	//qresult->hist=hist;
	qresult->rtime=GetTickCount();

	//DT9044: Handle binary requests for the IQ BigTradeRec
	if(this->binarydatareq)
	{
		// For BinaryDataRequests, ignore the SQL select list and send the BTR 
		qresult->obuf=new char[sizeof(BIGTRADEREC)];
		memset(qresult->obuf,0,sizeof(BIGTRADEREC));
		char *rptr=qresult->obuf;
		const char *rend=rptr +pfix->llen,*fend=pfix->fbuf +pfix->llen;

		// Send the entire BTR
		memcpy(rptr,&Msg758Rec.BigTradeRec,sizeof(BIGTRADEREC));
		rptr+=sizeof(BIGTRADEREC);
		binarydatalen=sizeof(BIGTRADEREC);
	}
	else
	{
		// Make a copy of the FIX string; don't just point to same memory that could go out of scope
		qresult->rfix.slen=pfix->llen;
		qresult->rfix.sbuf=new char[pfix->llen];
		memset(qresult->rfix.sbuf,0,pfix->llen);
		char *rptr=qresult->rfix.sbuf;
		const char *rend=rptr +pfix->llen,*fend=pfix->fbuf +pfix->llen;

		if(select=="*")
		{
			memcpy(rptr,pfix->fbuf,pfix->llen); rptr+=pfix->llen;
		}
		// Filter only desired items
		else
		{
			// Copy only the tags in the select clause
			char sstr[1024]={0};
			strcpy(sstr,select.c_str());
			for(char *tok=strtok(sstr,",\r\n");tok;tok=strtok(0,",\r\n"))
			{
				int tno=myatoi(tok);
				if(!tno)
					continue;
				FIXTAG *ptag=pfix->GetTag(tno);
				if(ptag)
				{
					const char *btag=pfix->fbuf +ptag->tstart;
					const char *etag=ptag->val +ptag->vlen;
					if(rend -rptr<(int)(etag -btag) +1)
					{
						_ASSERT(false);
						break;
					}
					memcpy(rptr,btag,etag -btag); rptr+=(etag -btag);
					*rptr=pfix->FIXDELIM; rptr++;
				}
			}
			_ASSERT(rptr<rend);
			*rptr=0;
		}	
		qresult->rfix.Reset();
		qresult->rfix.FIXDELIM=pfix->FIXDELIM;
		qresult->rfix.noSession=pfix->noSession;
		qresult->rfix.supressChkErr=true;
		// We're not really using the FIX msg for query so no need to do anhy parsing
		//qresult->rfix.FixMsgReady(qresult->rfix.sbuf,(int)(rptr -qresult->rfix.sbuf));
		qresult->rfix.fbuf=qresult->rfix.sbuf;
		qresult->rfix.llen=(int)(rptr -qresult->rfix.sbuf);
	}

	if(hist) 
		hresults.push_back(qresult);
	else 
		lresults.push_back(qresult);
	rcnt++;
	return 1;
    }
    return 0;
}

int ViewServer::HandleUsrCLbackupMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	CLBackupClient *cbc=(CLBackupClient *)UscPort[UsrPort[PortNo].UscPort].DetPtr6;
	if(!cbc)
		return -1;
	FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
	if(!ustats)
		return -1;
#ifdef SINGLE_CLBACKUP_FILE

	// Record the message straight to the detail file
	int UscPortNo=(int)(PTRCAST)UsrPort[PortNo].UscPort;
	VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
	if(dfile)
	{
		int wsize=sizeof(MSGHEADER) +MsgHeader->MsgLen;
		LONGLONG doff=-1;
		odb.WriteDetailBlock(dfile,(char*)MsgHeader,wsize,doff);
		//WSLogEvent("HandleUsrCLbackupMsg(),writing detail block:%d,%d,%I64d,%I64d",MsgHeader->MsgID,MsgHeader->MsgLen,wsize,doff); //JON
		FixStats *ustats=(FixStats *)UsrPort[PortNo].DetPtr3;
		if(ustats)
		{
			ustats->msgCnt+=wsize; ustats->totMsgCnt+=wsize;
		}
	}
	// Update syncpoint
	if((MsgHeader->MsgID==950)&&(MsgHeader->MsgLen>=sizeof(MSG950REC)))
	{
		MSG950REC Msg950Rec;
		memcpy((char*)&Msg950Rec,Msg,sizeof(Msg950Rec));
		cbc->LastReportLogNo=Msg950Rec.ReportLogNo;
	}
#else//!SINGLE_CLBACKUP_FILE
	switch(MsgHeader->MsgID)
	{
	// $CLBACKUP$ messages
	case 822:
	{
		MSG822REC Msg822Rec;
		if(MsgHeader->MsgLen<sizeof(MSG822REC))
			break;
		memcpy((char*)&Msg822Rec,Msg,sizeof(Msg822Rec));
		
		// Record the 822 message to the detail file
		int UscPortNo=(int)(PTRCAST)UsrPort[PortNo].UscPort;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
			int wsize=sizeof(MSGHEADER) +sizeof(MSG822REC);
			LONGLONG doff=-1;
			odb.WriteDetailBlock(dfile,Msg -sizeof(MSGHEADER),wsize,doff);
			ustats->msgCnt+=wsize; ustats->totMsgCnt+=wsize;
		}

		char rpath[MAX_PATH]={0};
		strcpy(rpath,Msg822Rec.FileDesc);
		_strupr(rpath);
		CLBACKUPMAP::iterator fit=cbc->fmap.find(rpath);
		if(fit==cbc->fmap.end())
		{
			WSLogEvent("USR%d: FileNotification(%s)",PortNo,rpath);
			CLBackupFile *tfile=new CLBackupFile;
			tfile->flog=true;
			strncpy(tfile->remotePath,rpath,sizeof(tfile->remotePath)-1);
			tfile->remotePath[sizeof(tfile->remotePath)-1]=0;
			cbc->fmap.insert(pair<string,CLBackupFile*>(rpath,tfile));
		}
		break;
	}
	case 950:
	{
		MSG950REC Msg950Rec;
		if(MsgHeader->MsgLen<sizeof(MSG950REC))
			break;
		memcpy((char*)&Msg950Rec,Msg,sizeof(Msg950Rec));
		cbc->LastReportLogNo=Msg950Rec.ReportLogNo;
		char *Data=0;
		if(Msg950Rec.DataLen)
			Data=Msg+sizeof(Msg950Rec);

		// Record the 950 message to the detail file
		int UscPortNo=(int)(PTRCAST)UsrPort[PortNo].UscPort;
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(dfile)
		{
			int wsize=sizeof(MSGHEADER) +sizeof(MSG950REC) +Msg950Rec.DataLen;
			LONGLONG doff=-1;
			odb.WriteDetailBlock(dfile,Msg -sizeof(MSGHEADER),wsize,doff);
			ustats->msgCnt+=wsize; ustats->totMsgCnt+=wsize;
		}

		switch(Msg950Rec.Command)
		{
		case BA_COM_OPEN:
		{
			//BA_OpenFile(Msg950Rec,Data,-1,UscPortNo);
			if(!Data)
			{
				_ASSERT(false);
				break;
			}
			char rpath[MAX_PATH]={0};
			strncpy(rpath,Msg +sizeof(MSG950REC),Msg950Rec.DataLen); rpath[Msg950Rec.DataLen]=0;
			_strupr(rpath);
			CLBackupFile *tfile=0;
			CLBACKUPMAP::iterator fit=cbc->fmap.find(rpath);
			if(fit==cbc->fmap.end())
			{
				_ASSERT(Data);
				WSLogEvent("USR%d: FileNotification(%s)",PortNo,Data);
				tfile=new CLBackupFile;
				tfile->flog=true;
				strncpy(tfile->remotePath,Data,sizeof(tfile->remotePath)-1);
				tfile->remotePath[sizeof(tfile->remotePath)-1]=0;
				tfile->Msg950Rec=Msg950Rec;
				cbc->fmap.insert(pair<string,CLBackupFile*>(rpath,tfile));
			}
			else
				tfile=fit->second;
			tfile->Msg950Rec=Msg950Rec;
			//if(tfile->lfp)
			//{
			//	SetFilePointer(tfile->lfp,0,0,FILE_BEGIN);
			//	SetEndOfFile(tfile->lfp);
			//}
			break;
		}
		case BA_COM_WRITE:
		{
			//BA_WriteFile(Msg950Rec,Data,false);
			for(CLBACKUPMAP::iterator fit=cbc->fmap.begin();fit!=cbc->fmap.end();fit++)
			{
				CLBackupFile *tfile=fit->second;
				if(tfile->Msg950Rec.FileHandle==Msg950Rec.FileHandle)
				{
					//// Write FIX to the detail file
					//if(tfile->flog)
					//{
					//	int UscPortNo=(int)(PTRCAST)UsrPort[PortNo].UscPort;
					//	VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
					//	if(dfile)
					//	{
					//		LONGLONG doff=Msg950Rec.DataOffset>=0?(LONGLONG)Msg950Rec.DataOffset:-1;
					//		odb.WriteDetailBlock(dfile,Msg +sizeof(MSG950REC),Msg950Rec.DataLen,doff);
					//		ustats->msgCnt+=Msg950Rec.DataLen; ustats->totMsgCnt+=Msg950Rec.DataLen;
					//	}
					//}
					//else
					//{
					//	_ASSERT(false);//untested
					//	fseeko(tfile->lfp,Msg950Rec.DataOffset,SEEK_SET);
					//	fwrite(Msg +sizeof(MSG950REC),1,Msg950Rec.DataLen,tfile->lfp);
					//}
					break;
				}
			}
			break;
		}
		case BA_COM_READ:
			//BA_ReadFile(Msg950Rec,Data,false);
			break;
		case BA_COM_SYNC:
			//BA_ReadFile(Msg950Rec,Data,true);
			break;
		case BA_COM_CLOSE:
		{
			//BA_CloseFile(Msg950Rec);
			for(CLBACKUPMAP::iterator fit=cbc->fmap.begin();fit!=cbc->fmap.end();fit++)
			{
				CLBackupFile *tfile=fit->second;
				if(tfile->Msg950Rec.FileHandle==Msg950Rec.FileHandle)
				{
					if(tfile->lfp)
					{
						fclose(tfile->lfp); tfile->lfp=0;
					}
					break;
				}
			}
			break;
		}

		case BA_COM_RESET:
			//BA_Reset(PortNo, Msg950Rec.DataOffset ? true : false);
			break;

		case BA_COM_DAYEND:
			//char Date[100];
			//sprintf (Date, "%08ld", Msg950Rec.DataOffset);
			//WSLogEvent("Dayend to [%s] message received from C[%d] on %s", Date, PortNo, UsrPort[PortNo].RemoteIP);
			//BA_Dayend(Date);
			break;
		default:
			//WSLogError("Invalid command %d on Message 950",Msg950Rec.Command);
			_ASSERT(false);
			break;
		}
		break;
	}
	//case 707:
	//case 719:
	//case 826:
	//case 951:
	//	break;
	default:
		_ASSERT(false);
		return -1;
	};
#endif//!SINGLE_CLBACKUP_FILE
	return 0;
}
//DT9044
bool ViewServer::ClBackupAccountRec(int PortNo, int aidx, char *args[32],int lidx)
{
	int UscPortNo=UsrPort[PortNo].UscPort;

	//0=AppInstID,1=ClOrdID,2=RootOrderID,3=FirstClOrdID,4=ClParentOrderID,5=Symbol,6=PriceStr,7=SideStr,8=Account,
	//9=EcnOrderID,10=ClientID,11=Timestamp,12=dj->locAlias,13=doffset,14=dlen,15=Connection,16=AuxKey,17=OrderQty

	// 9 common order arguments
	if(aidx<9)
	{
		WSLogError("USR%d: Incorrect number of fields(%d) on account journal entry(%d).",PortNo,aidx,lidx);
		return false;
	}
	char *AppInstID=args[0];
	if((!AppInstID)||(!*AppInstID))
	{
		WSLogError("USR%d: Missing 'AppInstID' on account journal entry(%d)!",PortNo,lidx);
		return false;
	}
	_strupr(AppInstID);
	char *Acct=args[8];
	if(!Acct)
	{
		WSLogError("USR%d: Missing 'Account' on account journal entry(%d)!",PortNo,lidx);
		return false;
	}
	if(!odb.INDEX_ACCOUNT_CS) //DT9491
		_strupr(Acct);
	char *TransactTimeStr=args[11];
	if(!TransactTimeStr)
	{
		WSLogError("USR%d: Missing 'TransactTime' on account journal entry(%d)!",PortNo,lidx);
		return false;
	}
	LONGLONG TransactTime=myatoi64(TransactTimeStr);
	if((!args[13])||(!*args[13]))
	{
		WSLogError("USR%d: Missing 'doff' on account journal entry(%d)!",PortNo,lidx);
		return false;
	}
	LONGLONG doff=myatoi64(args[13]);
	if(doff<0)
	{
		#ifdef WIN32
		WSLogError("USR%d: Invalid doff(%I64d) on account journal entry(%d)!",PortNo,doff,lidx);
		#else
		WSLogError("USR%d: Invalid doff(%lld) on account journal entry(%d)!",PortNo,doff,lidx);
		#endif
		return false;
	}
	int dlen=myatoi(args[14]);
	if(dlen<0)
	{
		WSLogError("USR%d: Invalid dlen(%d) on account journal entry(%d)!",PortNo,dlen,lidx);
		return false;
	}

    VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
    if(!dfile)
	{
		WSLogError("USR%d: Detail file pointer is null upon receiving an IQ AccountRec",PortNo);
		return false;
	}
    if(dfile->proto!=PROTO_CLBACKUP)
	{
		WSLogError("USR%d: Received an IQ AccountRec on a %d proto! Check the configuration (ports.txt)",PortNo,dfile->proto);
		return false;
	}

	AppSystem *asys=GetAppSystem(AppInstID);
	if(!asys)
	{
		WaitForSingleObject(asmux,INFINITE);
		WSLogError("AppSystem for AppInstance(%s) not found.",args[0]);
		asys=GetAppSystem("UNKNOWN*");
		if(!asys)
		{
			WSLogError("USR%d: AppSystem(%s) for (%s) not found!",PortNo,"UNKNOWN",args[0]);
			ReleaseMutex(asmux);
			return false;
		}
		int icnt=0;
		AppInstance *ainst=asys->CreateInstance(args[0],icnt);
		if(ainst)
		{
			aimap[ainst->iname]=ainst;
			asys->PregenBrowse();
			WSLogEvent("Created AppInstance(%s) under AppSystem(UNKNOWN)",ainst->iname.c_str());
		}
		ReleaseMutex(asmux);
	}

	OrderDB *pdb=&asys->odb;
	pdb->Lock();
	char *dptr=new char[dlen+1];
	memset(dptr,0,dlen+1);
	Account* acc=0;

	// Read the detail from the other side
	if(pdb->ReadDetail(dfile,doff,dptr,dlen)>0)
	{
		MSG632REC Msg632Rec;
		memset(&Msg632Rec,0,sizeof(MSG632REC));
		memcpy(&Msg632Rec.AccountRec,dptr+28,sizeof(ACCOUNTREC));
		_strupr(Msg632Rec.AccountRec.Domain);
		_strupr(Msg632Rec.AccountRec.Account);
		acc=CreateDnsAccount(-1,Msg632Rec);
		if(!acc)
		{
			pdb->Unlock();
			if(dptr) delete dptr;
			WSLogError("USR%d: Error creating account for (%s,%s)",PortNo,Msg632Rec.AccountRec.Domain,Msg632Rec.AccountRec.Account);
			return false;
		}
	}

	// Use the order database for the account records for persistence only,
	// queries will be done using the account objects.
	char okey[256]={0};
	// This add the letter 'd' to the end??
	//sprintf(okey,"%s_%d_%i64d",VSDB_ACCOUNTREC_IND,dfile->portno,doff);  //Unique key is $$ACCOUNT_<portno>_<offset>
	sprintf(okey,"%s_%d_%I64d",VSDB_ACCOUNTREC_IND,dfile->portno,doff);  //Unique key is $$ACCOUNT_<portno>_<offset>
	bool isnew=true;
	ITEMLOC idoff=doff;
	VSOrder *porder=CreateOrder(UscPortNo,asys,okey,isnew,idoff);
	doff=idoff;
	if(!porder)
	{
		pdb->Unlock();
		if(dptr) delete dptr;
		WSLogError("USR%d: Failed CreateOrder(%s)",PortNo,okey);
		return false;
	}

	// Only set the ClOrdID, parse out for queries that might include this "non-order"
	// These are used to aide in the reload of IQ AccountRecs upon restart
	porder->SetClOrdID(okey);
	char Conn[10];
	sprintf(Conn,"%d",UscPortNo);
	porder->SetConnection(Conn);
	char doffs[20];
	sprintf(doffs,"%I64d",doff);
	porder->SetAuxKey(0,doffs);

	// Commit and free the order as needed
	if(porder)
	{
		if(porder->IsDirty())
		{
			idoff=doff;
			CommitOrder(UscPortNo,asys,porder,idoff);
			doff=idoff;
		}

		// Call "live" tasks
		FeedHint achint={(AppSystem*)acc,0,0,-1,0,PROTO_UNKNOWN};
		taskmgr.CallTasks("ACCOUNTS",TASK_SQL_ACCOUNTS,&achint);

	#ifndef TASK_THREAD_POOL
		// When we have fusion I/O, the disk copy is as good as memory
		if(pdb->HAVE_FUSION_IO)
		{
			idoff=doff;
			UnloadOrder(UscPortNo,asys,porder,idoff);
			doff=idoff;
		}
		#ifdef UNLOAD_TERM_ORDERS
		// Evict orders when they terminate
		else if(porder->IsTerminated())
		{
			idoff=doff;
			UnloadOrder(UscPortNo,asys,porder,idoff);
			doff=idoff;
		}
		#endif
		//pdb->SetZdirty(); is this needed?
	#endif
		// Remove the dummy order from the ClOrdID index so it doesn't show up in vsctest3
		OrderDB *pdb=asys ?&asys->odb :&this->odb;
		VSINDEX::iterator oit=pdb->omap.find(okey);
		if(oit!=pdb->omap.end())
			pdb->omap.erase(oit);
	}

	pdb->Unlock();
	if(dptr) delete dptr;
	return true;
}

//DT9044
bool ViewServer::ClBackupCheckForDetailUpdate(OrderDB* pdb,VSOrder *porder,int UscPortNo,VSDetailFile* dfile,char* AppInstID,char* ClOrdID,
	char* AuxKey,AppSystem* asys, AppInstance* ainst,bool acct,ITEMLOC oloc,LONGLONG newdoff)
{
	for(int d=0;d<porder->GetNumDetails();d++)
	{
		unsigned short dlen=0;
		unsigned short uscPortNo=0;
		LONGLONG doff=porder->GetDetail(d,uscPortNo,dlen);
		char *dptr=new char[dlen+1];
		memset(dptr,0,dlen+1);
		if(pdb->ReadDetail(dfile,doff,dptr,dlen)>0)
		{
			MSG950REC* Msg950Rec=(MSG950REC*)dptr;
			char* data=0;
			if(Msg950Rec->DataLen>0)
			{
				data=(char*)Msg950Rec + sizeof(MSG950REC) +sizeof(MSGHEADER);
			}

			BIGTRADEREC* btr=(BIGTRADEREC*)data;
			if(!btr)
			{
				if(dptr) delete dptr;
				continue;
			}

			// Match on Domain, OrderNo, and TradeDetRef
			string DomainAndOrderNo=string(btr->TradeRec.Domain)+string("_")+string(btr->TradeRec.OrderNo);
			if(!strcmp(btr->TradeRec.Domain,AppInstID) &&
				!strcmp(DomainAndOrderNo.c_str(),ClOrdID) &&
				btr->TradeRec.TradeDetRef==atol(AuxKey))
			{
				// Update this detail
				porder->UpdateDetailOffset(d,newdoff);
				CommitOrder(UscPortNo,asys,porder,oloc);

				// Not updating the order information for now...  Not needed for ClServer usage.

				// Not updating the ofills file for now... Not needed for ClServer usage.

				// Call tasks
			#ifdef TASK_THREAD_POOL
				FeedHint fhint={asys,ainst,porder,d,0,PROTO_UNKNOWN,false,UscPortNo,oloc};
				// Unload must be done by TaskThread
				// When we have fusion I/O, the disk copy is as good as memory
				if(pdb->HAVE_FUSION_IO)
					fhint.unload=true;
				#ifdef UNLOAD_TERM_ORDERS
				// Evict orders when they terminate
				else if(porder->IsTerminated())
					fhint.unload=true;
				#endif
			#else
				FeedHint fhint={asys,ainst,porder,d,0,PROTO_UNKNOWN};
			#endif

				taskmgr.CallTasks(porder->GetClOrdID(),TASK_CLORDID,&fhint);
				taskmgr.CallTasks(porder->GetClParentOrderID(),TASK_PARENTCLORDID,&fhint);
				taskmgr.CallTasks(porder->GetAppInstID(),TASK_APPINST,&fhint);
				taskmgr.CallTasks(asys->skey,TASK_APPSYS,&fhint);
				taskmgr.CallTasks(porder->GetSymbol(),TASK_SYMBOL,&fhint);
				taskmgr.CallTasks(porder->GetRootOrderID(),TASK_ROOTORDERID,&fhint);
				taskmgr.CallTasks(porder->GetFirstClOrdID(),TASK_FIRSTCLORDID,&fhint);
				taskmgr.CallTasks(porder->GetAccount(),TASK_ACCOUNT,&fhint);
				taskmgr.CallTasks(porder->GetEcnOrderID(),TASK_ECNORDERID,&fhint);
				taskmgr.CallTasks(porder->GetClientID(),TASK_CLIENTID,&fhint);

				if(acct)
				{
					FeedHint achint={asys,ainst,porder,-1,0,PROTO_UNKNOWN};
					taskmgr.CallTasks("ACCOUNTS",TASK_SQL_ACCOUNTS,&achint);
				}
			#ifndef TASK_THREAD_POOL
				if(fhint.pfix)
				{
					delete fhint.pfix; fhint.pfix=0;
				}
			#endif

			#ifndef TASK_THREAD_POOL
				// When we have fusion I/O, the disk copy is as good as memory
				if(pdb->HAVE_FUSION_IO)
					UnloadOrder(UscPortNo,asys,porder,oloc);
				#ifdef UNLOAD_TERM_ORDERS
				// Evict orders when they terminate
				else if(porder->IsTerminated())
					UnloadOrder(UscPortNo,asys,porder,oloc);
				#endif
			#endif
				if(dptr) delete dptr;
				return true;
			}
		}
		if(dptr) delete dptr;
	}
	return false;
}
#ifdef REDIOLEDBCON
LONGLONG ViewServer::GMTToEST(LONGLONG gts, SYSTEMTIME& lst, string& stz)
{
	memset(&lst,0,sizeof(SYSTEMTIME));
	stz="";
	// This only works when client machine is on EST
	//SYSTEMTIME gst;
	//gst.wMilliseconds=gts%1000; gts/=1000;
	//gst.wSecond=gts%100; gts/=100;
	//gst.wMinute=gts%100; gts/=100;
	//gst.wHour=gts%100; gts/=100;
	//gst.wDay=gts%100; gts/=100;
	//gst.wMonth=gts%100; gts/=100;
	//gst.wYear=gts%10000;
	//gst.wDayOfWeek=0;
	//FILETIME gft,lft;
	//SystemTimeToFileTime(&gst,&gft);
	//FileTimeToLocalFileTime(&gft,&lft);
	//FileTimeToSystemTime(&lft,&lst);

	struct tm gst;
	int wMilliseconds=0;
	if(gts>=(LONGLONG)99991231000000)
	{
		wMilliseconds=gts%1000; gts/=1000;
	}
	gst.tm_sec=gts%100; gts/=100;
	gst.tm_min=gts%100; gts/=100;
	gst.tm_hour=gts%100; gts/=100;
	gst.tm_mday=gts%100; gts/=100;
	gst.tm_mon=gts%100 -1; gts/=100;
	gst.tm_year=gts%10000 -1900;
	gst.tm_isdst=-1;
	time_t tt=mktime(&gst);
	tt-=4*3600;
	if(!gst.tm_isdst)
		tt-=3600;
	if(tt<=0)
		return 0;
	gst=*localtime(&tt);
	#ifdef TIME_CONVERT
	stz=(gst.tm_isdst)?"EDT":"EST";
	#endif

	lst.wYear=gst.tm_year +1900;
	lst.wMonth=gst.tm_mon +1;
	lst.wDay=gst.tm_mday;
	lst.wHour=gst.tm_hour;
	lst.wMinute=gst.tm_min;
	lst.wSecond=gst.tm_sec;
	lst.wMilliseconds=wMilliseconds;

	LONGLONG lts=(lst.wYear*10000) +(lst.wMonth*100) +(lst.wDay); lts*=1000000;
	lts+=(lst.wHour*10000) +(lst.wMinute*100) +(lst.wSecond); lts*=1000;
	lts+=lst.wMilliseconds;
	return lts;
}
LONGLONG ViewServer::ESTToGMT(LONGLONG gts, SYSTEMTIME& lst, string& stz)
{
	memset(&lst,0,sizeof(SYSTEMTIME));
	stz="";

	struct tm gst;
	int wMilliseconds=0;
	if(gts>=(LONGLONG)99991231000000)
	{
		wMilliseconds=gts%1000; gts/=1000;
	}
	gst.tm_sec=gts%100; gts/=100;
	gst.tm_min=gts%100; gts/=100;
	gst.tm_hour=gts%100; gts/=100;
	gst.tm_mday=gts%100; gts/=100;
	gst.tm_mon=gts%100 -1; gts/=100;
	gst.tm_year=gts%10000 -1900;
	gst.tm_isdst=-1;
	time_t tt=mktime(&gst);
	if(tt<=0)
		return 0;
	tt+=4*3600;
	if(!gst.tm_isdst)
		tt+=3600;

	gst=*localtime(&tt);
	stz="GMT";

	lst.wYear=gst.tm_year +1900;
	lst.wMonth=gst.tm_mon +1;
	lst.wDay=gst.tm_mday;
	lst.wHour=gst.tm_hour;
	lst.wMinute=gst.tm_min;
	lst.wSecond=gst.tm_sec;
	lst.wMilliseconds=wMilliseconds;

	LONGLONG lts=(lst.wYear*10000) +(lst.wMonth*100) +(lst.wDay); lts*=1000000;
	lts+=(lst.wHour*10000) +(lst.wMinute*100) +(lst.wSecond); lts*=1000;
	lts+=lst.wMilliseconds;
	return lts;
}

void
ViewServer::decodeTranTypeCode(BIGTRADEREC *btr, const char *capacity)
{
	if(isdigit(capacity[0]))
	{
		//btr->TradeRecB.CustomCapacity=atoi(capacity);
		int cval=atoi(capacity);
		switch(cval)
		{
		case 1: 
		case 2:
			btr->TradeRecB.TranTypeCode='A'; btr->TradeRec.LastCapacity=CAPACITY_AGENCY; break;
		case 4: btr->TradeRecB.TranTypeCode='P'; btr->TradeRec.LastCapacity=CAPACITY_PRINCIPAL; break;
		case 18: btr->TradeRecB.TranTypeCode='C'; btr->TradeRec.LastCapacity=CAPACITY_AGENCY; break;
		case 21: btr->TradeRecB.TranTypeCode='F'; btr->TradeRec.LastCapacity=CAPACITY_AGENCY; break;
		case 19: btr->TradeRecB.TranTypeCode='B'; btr->TradeRec.LastCapacity=CAPACITY_PRINCIPAL; break;
		case 20: btr->TradeRecB.TranTypeCode='M'; btr->TradeRec.LastCapacity=CAPACITY_PRINCIPAL; break;
		};
	}
	else
	{
		switch(capacity[0])
		{
		case 'A': btr->TradeRecB.TranTypeCode='A'; btr->TradeRec.LastCapacity=CAPACITY_AGENCY; break;
		case 'P': btr->TradeRecB.TranTypeCode='P'; btr->TradeRec.LastCapacity=CAPACITY_PRINCIPAL; break;
		case 'C': btr->TradeRecB.TranTypeCode='C'; btr->TradeRec.LastCapacity=CAPACITY_AGENCY; break;
		case 'F': btr->TradeRecB.TranTypeCode='F'; btr->TradeRec.LastCapacity=CAPACITY_AGENCY; break;
		case 'B': btr->TradeRecB.TranTypeCode='B'; btr->TradeRec.LastCapacity=CAPACITY_PRINCIPAL; break;
		case 'M': btr->TradeRecB.TranTypeCode='M'; btr->TradeRec.LastCapacity=CAPACITY_PRINCIPAL; break;
		};
	}
}

void
ViewServer::decodeRediDestination(BIGTRADEREC *btr, const char *rediDestination)
{
	const char *execbrkmpid="";
	map<string,pair<string,string>>::iterator mit=rediDestMap.find(rediDestination);
	if(mit!=rediDestMap.end()) {
		rediDestination=mit->second.first.c_str();
		execbrkmpid=mit->second.second.c_str();
	}
	strncpy(btr->TradeRecB.RediDestination,rediDestination,sizeof(btr->TradeRecB.RediDestination)); 
	strncpy(btr->TradeRecA.ExecutingBrokerCode,execbrkmpid,sizeof(btr->TradeRecA.ExecutingBrokerCode));
}

void
ViewServer::decodeAlgoStrategy(BIGTRADEREC *btr, const char *algorithm, const char *rediDestination)
{
	char akey[128]={0};
	strncpy(akey,rediDestination,sizeof(akey));
	char *aptr=strchr(akey,' ');
	if(!aptr) aptr=akey +strlen(akey);
	sprintf(aptr,".%s",algorithm);
	if(akey[0])
	{
		map<string,string>::iterator ait=algoStrategyMap.find(akey);
		if(ait!=algoStrategyMap.end())
			strncpy(btr->TradeRecB.StrategyName,ait->second.c_str(),sizeof(btr->TradeRecB.StrategyName));
		else
			strncpy(btr->TradeRecB.StrategyName,algorithm,sizeof(btr->TradeRecB.StrategyName));
	}
}

int ViewServer::decodeProductType(const char *productType)
{
	if(!stricmp(productType,"FLEXOPT"))
		return 1;
	else if(!stricmp(productType,"FUTOPT"))
		return 2;
	else if(!stricmp(productType,"FLEXIDXOPT"))
		return 3;
	else if(!stricmp(productType,"IDXLEAPOPT"))
		return 4;
	else if(!stricmp(productType,"IDXOPT"))
		return 5;
	else if(!stricmp(productType,"QIDXOPT"))
		return 6;
	else if(!stricmp(productType,"INTOPT"))
		return 7;
	else if(!stricmp(productType,"LEAPOPT"))
		return 8;
	else if(!stricmp(productType,"LISTOPT"))
		return 9;
	else if(!stricmp(productType,"OTCOPT"))
		return 10;
	else if(!stricmp(productType,"PAPEROPT"))
		return 11;
	else if(!stricmp(productType,"CURROPT"))
		return 12;
	else if(!stricmp(productType,"TREASOPT"))
		return 13;
	else if(!stricmp(productType,"ETFPRODUCT"))
		return 14;
	else if(!stricmp(productType,"CFD"))
		return 15;
	else if(!stricmp(productType,"EQUITY"))
		return 16;
	else if(!stricmp(productType,"FUTURE"))
		return 17;
	else if(!stricmp(productType,"EMINI"))
		return 18;
	else if(!stricmp(productType,"BOND"))
		return 19;
	else if(!stricmp(productType,"SSF"))
		return 20;
	else if(!stricmp(productType,"FX"))
		return 21;
	else if(!stricmp(productType,"SWAP"))
		return 22;
	else
		return 0;
}

void ViewServer::decodeFutureExchange(const char *productType, int spreadCode, char& exchange, char& region)
{
	if(!stricmp(productType,"FUTOPT"))
	{
		exchange=16; region='o';
	}
	else if(!stricmp(productType,"FUTURE"))
	{
		exchange=16; region=0;
		if((spreadCode==2)||(spreadCode==3))
			region='s';
	}
	else if(!stricmp(productType,"EMINI"))
	{
		exchange=28; region='e';
		if((spreadCode==2)||(spreadCode==3))
			region='s';
	}
}

// The CL needs a unique order id for every leg of the spread
void ViewServer::getEcnParserOrderNo(BIGTRADEREC *btr, const char *t11, const char *t37)
{
	char ecnParserOrderNo[1024]={0};
	if((strlen(t11)>8)&&(!strncmp(t11,"20",2)))
		sprintf_s(ecnParserOrderNo,"%s_%s",t11+8,t37);
	else
		sprintf_s(ecnParserOrderNo,"%s_%s",t11,t37);
	strncpy(btr->TradeRecA.ECNParserOrderNo,ecnParserOrderNo,sizeof(btr->TradeRecA.ECNParserOrderNo));
	btr->TradeRecA.ECNParserOrderNo[sizeof(btr->TradeRecA.ECNParserOrderNo)-1]=0;
}

void ViewServer::getCancelMemo(BIGTRADEREC *btr, const char *t11, const char *t37)
{
	char cancelMemo[1024]={0};
	if((strlen(t11)>8)&&(!strncmp(t11,"20",2)))
		sprintf_s(cancelMemo,"%s_%s",t11+8,t37);
	else
		sprintf_s(cancelMemo,"%s_%s",t11,t37);
	strncpy(btr->TradeRecA.CancelMemo,cancelMemo,sizeof(btr->TradeRecA.CancelMemo));
	btr->TradeRecA.CancelMemo[sizeof(btr->TradeRecA.CancelMemo)-1]=0;
}

int ViewServer::ConvertFixToBTR_Old(VSQueryResult *vsres)
{
	if (vsres->obuf)
		return 0;
	//DWORD tstart=GetTickCount();
	BIGTRADEREC_OLD *btr = new BIGTRADEREC_OLD;
	memset(btr, 0, sizeof(BIGTRADEREC_OLD));
	vsres->obuf = (char*)btr;
	vsres->DataLength = sizeof(BIGTRADEREC_OLD);
	int flen = vsres->rfix.llen; vsres->rfix.llen = 0;
	vsres->rfix.FixMsgReady(vsres->rfix.fbuf, flen);
	return FixTags2BTR(vsres, (tdBigTradeRec*)btr, 0);
}

int ViewServer::ConvertFixToBTR(VSQueryResult *vsres)
{
	if (vsres->obuf)
		return 0;
	//DWORD tstart=GetTickCount();
	BIGTRADEREC *btr = new BIGTRADEREC;
	memset(btr, 0, sizeof(BIGTRADEREC));
	vsres->obuf = (char*)btr;
	vsres->DataLength = sizeof(BIGTRADEREC);
	int flen = vsres->rfix.llen; vsres->rfix.llen = 0;
	vsres->rfix.FixMsgReady(vsres->rfix.fbuf, flen);
	return FixTags2BTR(vsres, (tdBigTradeRec*)btr,&btr->mifid);
}

int ViewServer::FixTags2BTR(VSQueryResult *vsres, struct tdBigTradeRec *btr, MiFID* miFID)
{
	btr->TradeRec.OverrideOpenClose=1;
	bool delayedDecodeAlgoStrategy = false;
	for(int i=0;i<vsres->rfix.ntags;i++)
	{
		FIXTAG& tag=vsres->rfix.tags[i];
		switch(tag.no)
		{
		case 34: btr->TradeRec.TradeDetRef=vsres->rfix.TagInt(34); break;
		case 1: 
		case 10600:
		{
			if((tag.no == 1) && (btr->TradeRec.Account[0] !=0)) {
				break; // If tag tag 10600 already set account, do not update Account with tag 1
			}

			strncpy(btr->TradeRec.Account,vsres->rfix.TagStr(tag.no),sizeof(btr->TradeRec.Account)); 
			btr->TradeRec.Account[sizeof(btr->TradeRec.Account)-1]=0;
			// TODO: Now that we associate the orders with the correct domain app instance,
			// we don't need to look up the account again.
			// However this is convenient to be able to fix account to domain associations intraday.
			// Look up IQ domain by account and fill in btr->TradeRec.Domain
			TACCTMAP::iterator ait=tacmap.find(btr->TradeRec.Account);
			if(ait!=tacmap.end())
			{
				string domain=ait->second.first;
				Account *pacc=ait->second.second;
				strncpy(btr->TradeRec.Domain,domain.c_str(),sizeof(btr->TradeRec.Domain));
				btr->TradeRec.Domain[sizeof(btr->TradeRec.Domain)-1]=0;
			}
			break;
		}
		//case 49: strncpy(btr->TradeRec.Domain,vsres->rfix.TagStr(49),sizeof(btr->TradeRec.Domain)); break;
		case 50: 
			strncpy(btr->TradeRec.User,vsres->rfix.TagStr(50),sizeof(btr->TradeRec.User)); 
			btr->TradeRec.User[sizeof(btr->TradeRec.User)-1]=0;
			break;
		//case 56: 
		//	strncpy(btr->TradeRec.Domain,vsres->rfix.TagStr(56),sizeof(btr->TradeRec.Domain)); 
		//	btr->TradeRec.Domain[sizeof(btr->TradeRec.Domain)-1]=0;
		//	break;
		case 109: 
			strncpy(btr->TradeRecB.AltAccount1,vsres->rfix.TagStr(109),sizeof(btr->TradeRecB.AltAccount1)); 
			btr->TradeRecB.AltAccount1[sizeof(btr->TradeRecB.AltAccount1)-1]=0;
			break;
		case 39:
		case 150: 
			switch(vsres->rfix.TagChar(150))
			{
			case '0': btr->TradeRec.LastStatus=TRADE_CONFIRMED; btr->TradeRec.OrderConfirmed=true; break;
			case '1': btr->TradeRec.LastStatus=TRADE_PARTFILL; break;
			case '2': btr->TradeRec.LastStatus=TRADE_FILLED; break;
			case '3': btr->TradeRec.LastStatus=TRADE_EXPIRED; break;
			// Unlike FIX, CLServer rolls up cancel orders into the previous order,
			// so we make it look like a cancel reported on previous order
			case '4': btr->TradeRec.LastStatus=TRADE_CANCELED; 
				btr->TradeRec.Canceled=vsres->rfix.TagInt(38) -vsres->rfix.TagInt(14); 
				if(vsres->rfix.GetTag(41))
				{
					strncpy(btr->TradeRec.OrderNo,vsres->rfix.TagStr(41),sizeof(btr->TradeRec.OrderNo)); 
					btr->TradeRec.OrderNo[sizeof(btr->TradeRec.OrderNo)-1]=0;
					// DEV-18436: Distinguish client cancels from exchange-cancels
					if(strcmp(btr->TradeRec.OrderNo,vsres->rfix.TagStr(11)))
						btr->TradeRec.EditedByBo=true;
					if((!stricmp(vsres->rfix.TagStr(167),"MLEG"))||
					   (!stricmp(vsres->rfix.TagStr(442),"2"))||(!stricmp(vsres->rfix.TagStr(442),"3")))
						getEcnParserOrderNo(btr,vsres->rfix.TagStr(41),vsres->rfix.TagStr(37));
					else
						strncpy(btr->TradeRecA.ECNParserOrderNo,vsres->rfix.TagStr(41),sizeof(btr->TradeRecA.ECNParserOrderNo)); 
					btr->TradeRecA.ECNParserOrderNo[sizeof(btr->TradeRecA.ECNParserOrderNo)-1]=0;
				}
				memset(btr->TradeRecA.CancelMemo,0,sizeof(btr->TradeRecA.CancelMemo)); 
				break;
			// Unlike standard FIX, TWIST sends status 5 on replacing order, not replaced order
			case '5': 
				//btr->TradeRec.LastStatus=TRADE_CANCELED; 
				//btr->TradeRec.Canceled=vsres->rfix.TagInt(38) -vsres->rfix.TagInt(14); 
				//btr->TradeRec.OrderWasCancelReplaced=true; break;
				btr->TradeRec.LastStatus=TRADE_CONFIRMED; btr->TradeRec.OrderConfirmed=true; 
				btr->TradeRec.ReplaceRequest=true;
				break;
			case '6': btr->TradeRec.LastStatus=TRADE_CANCELPENDING; break;
			case '7': btr->TradeRec.LastStatus=TRADE_STOPPED; break;
			case '8': 
				btr->TradeRec.LastStatus=TRADE_REJECTED;
				// DEV-29121: We can't tell the difference between reject on new order and cancel/reject on replace,
				// so attempt to handle the new order reject at the OATS Generator
				// Non-standard Redi behavior getting confirm before unsolicited reject with different tag 11 and 41
				//if(vsres->rfix.GetTag(41))
				//{
				//	strncpy(btr->TradeRec.OrderNo,vsres->rfix.TagStr(41),sizeof(btr->TradeRec.OrderNo)); 
				//	btr->TradeRec.OrderNo[sizeof(btr->TradeRec.OrderNo)-1]=0;
				//	if((!stricmp(vsres->rfix.TagStr(167),"MLEG"))||
				//	   (!stricmp(vsres->rfix.TagStr(442),"2"))||(!stricmp(vsres->rfix.TagStr(442),"3")))
				//		getEcnParserOrderNo(btr,vsres->rfix.TagStr(41),vsres->rfix.TagStr(37));
				//	else
				//		strncpy(btr->TradeRecA.ECNParserOrderNo,vsres->rfix.TagStr(41),sizeof(btr->TradeRecA.ECNParserOrderNo)); 
				//	btr->TradeRecA.ECNParserOrderNo[sizeof(btr->TradeRecA.ECNParserOrderNo)-1]=0;
				//	memset(btr->TradeRecA.CancelMemo,0,sizeof(btr->TradeRecA.CancelMemo)); 
				//}
				break;
			case '9': btr->TradeRec.LastStatus=TRADE_REJECTED; break;
			case 'E': btr->TradeRec.LastStatus=TRADE_CANCELPENDING; btr->TradeRec.OrderWasCancelReplaced=true; break;
			};
			btr->TradeRec.Pending=vsres->rfix.TagInt(151);
			if(btr->TradeRec.Pending==0)
				btr->TradeRec.Pending=vsres->rfix.TagInt(38) -vsres->rfix.TagInt(14);
			break;
		// Unlike FIX, CLServer rolls up cancel orders into the previous order,
		// so we make it look like a cancel reject reported on previous order
		case 35: 
			switch(vsres->rfix.TagChar(35))
			{
			case '9':
				switch(vsres->rfix.TagChar(434))
				{
				case '1': btr->TradeRec.LastStatus=100; break; // Cancel Rejected by ECN
				case '2': btr->TradeRec.LastStatus=104; break; // Cancel Rejected by ECN - Cancel/Replace not allowed DT3347
				};
				if(vsres->rfix.GetTag(41))
				{
					strncpy(btr->TradeRec.OrderNo,vsres->rfix.TagStr(41),sizeof(btr->TradeRec.OrderNo)); 
					btr->TradeRec.OrderNo[sizeof(btr->TradeRec.OrderNo)-1]=0;
				}
				memset(btr->TradeRecA.CancelMemo,0,sizeof(btr->TradeRecA.CancelMemo)); 
				break;
			};
			break;
		case 11: 
			if(!btr->TradeRec.OrderNo[0])
			{
				strncpy(btr->TradeRec.OrderNo,vsres->rfix.TagStr(11),sizeof(btr->TradeRec.OrderNo));
				btr->TradeRec.OrderNo[sizeof(btr->TradeRec.OrderNo)-1]=0;
			}
			if(!btr->TradeRecA.ECNParserOrderNo[0])
			{
				if((!stricmp(vsres->rfix.TagStr(167),"MLEG"))||
				   (!stricmp(vsres->rfix.TagStr(442),"2"))||(!stricmp(vsres->rfix.TagStr(442),"3")))
					getEcnParserOrderNo(btr,vsres->rfix.TagStr(11),vsres->rfix.TagStr(37));
				else
					strncpy(btr->TradeRecA.ECNParserOrderNo,vsres->rfix.TagStr(11),sizeof(btr->TradeRecA.ECNParserOrderNo)); 
				btr->TradeRecA.ECNParserOrderNo[sizeof(btr->TradeRecA.ECNParserOrderNo)-1]=0;
				break;
			}
		case 41: 
			if (btr->TradeRec.LastStatus<100)
			{
				strncpy(btr->TradeRecA.CancelMemo, vsres->rfix.TagStr(41), sizeof(btr->TradeRecA.CancelMemo));
				if ((!stricmp(vsres->rfix.TagStr(167), "MLEG")) ||
					(!stricmp(vsres->rfix.TagStr(442), "2")) || (!stricmp(vsres->rfix.TagStr(442), "3")))
					getCancelMemo(btr, vsres->rfix.TagStr(41), vsres->rfix.TagStr(37));
				btr->TradeRecA.CancelMemo[sizeof(btr->TradeRecA.CancelMemo) - 1] = 0;
			}
			break;
		case 17:
		case 19: 
			// Gcore doesn't always drop tag 19, so always use it if it's available.
			// Populate the larger ECNEXECID also (to support allocation of spreads)
			if(vsres->rfix.GetTag(19))
			{
				strncpy(btr->TradeRec.ECNExecID,vsres->rfix.TagStr(19),sizeof(btr->TradeRec.ECNExecID)); 
				if(strlen(vsres->rfix.TagStr(19)) >= sizeof(btr->TradeRecB.ECNExecID)) // Use the ECNExecID_aux field to support ExecID's > 40 chars
				{
					strncpy(btr->TradeRecB.ECNExecID,vsres->rfix.TagStr(19),sizeof(btr->TradeRecB.ECNExecID)-1); 
					strncpy(btr->TradeRecB.ECNExecID_aux,&vsres->rfix.TagStr(19)[sizeof(btr->TradeRecB.ECNExecID)-1],sizeof(btr->TradeRecB.ECNExecID_aux)); 
				}
				else
				{
					strncpy(btr->TradeRecB.ECNExecID,vsres->rfix.TagStr(19),sizeof(btr->TradeRecB.ECNExecID)); 
				}
			}
			else
			{
				strncpy(btr->TradeRec.ECNExecID,vsres->rfix.TagStr(17),sizeof(btr->TradeRec.ECNExecID)); 
				if(strlen(vsres->rfix.TagStr(17)) >= sizeof(btr->TradeRecB.ECNExecID)) // Use the ECNExecID_aux field to support ExecID's > 40 chars
				{
					strncpy(btr->TradeRecB.ECNExecID,vsres->rfix.TagStr(17),sizeof(btr->TradeRecB.ECNExecID)-1); 
					strncpy(btr->TradeRecB.ECNExecID_aux,&vsres->rfix.TagStr(17)[sizeof(btr->TradeRecB.ECNExecID)-1],sizeof(btr->TradeRecB.ECNExecID_aux)); 
				}
				else
				{
					strncpy(btr->TradeRecB.ECNExecID,vsres->rfix.TagStr(17),sizeof(btr->TradeRecB.ECNExecID)); 
				}
			}
			btr->TradeRec.ECNExecID[sizeof(btr->TradeRec.ECNExecID)-1]=0;
			btr->TradeRecB.ECNExecID[sizeof(btr->TradeRecB.ECNExecID)-1]=0;
			btr->TradeRecB.ECNExecID_aux[sizeof(btr->TradeRecB.ECNExecID_aux)-1]=0;
			break;
		case 37: 
			strncpy(btr->TradeRec.ECNOrderNo,vsres->rfix.TagStr(37),sizeof(btr->TradeRec.ECNOrderNo)); 
			btr->TradeRec.ECNOrderNo[sizeof(btr->TradeRec.ECNOrderNo)-1]=0;
			break;
		case 198:
			#ifdef GS_RTECHOATS
			if((btr->TradeRecB.RediOrderType=='T')||(!strcmp(vsres->rfix.TagStr(100),"TICKET")))
			#endif
			{
				// This is a parent ticket
				if(!stricmp(vsres->rfix.TagStr(198),vsres->rfix.TagStr(37)))
				{
					btr->TradeRecB.OrderStagingOrder=1;
					btr->TradeRecB.OrderStagingParent=1;
					btr->TradeRec.WorkOrderRefNo=atoi(vsres->rfix.TagStr(198));
				}
				// Otherwise, it's a child order routed from a parent ticket
				else
				{
					btr->TradeRecB.OrderStagingOrder=1;
					btr->TradeRecB.OrderStagingParent=0;
					btr->TradeRec.WorkOrderRefNo=atoi(vsres->rfix.TagStr(198));
				}
			}
			break;
		case 55: 
			strncpy(btr->TradeRec.ExSymbol.Symbol,vsres->rfix.TagStr(55),sizeof(btr->TradeRec.ExSymbol.Symbol)); 
			btr->TradeRec.ExSymbol.Symbol[sizeof(btr->TradeRec.ExSymbol.Symbol)-1]=0;
			break;
		case 65: 
			strcat(btr->TradeRec.ExSymbol.Symbol,"."); strcat(btr->TradeRec.ExSymbol.Symbol,vsres->rfix.TagStr(65)); 
			btr->TradeRec.ExSymbol.Symbol[sizeof(btr->TradeRec.ExSymbol.Symbol)-1]=0;
			break;
		case 48: 
			if((!strcmp(vsres->rfix.TagStr(167),"OPT"))||(!strcmp(vsres->rfix.TagStr(167),"MLEG")))
			{
				strncpy(btr->TradeRec.OptRootSymbol,vsres->rfix.TagStr(55),sizeof(btr->TradeRec.OptRootSymbol));
				btr->TradeRec.OptRootSymbol[sizeof(btr->TradeRec.OptRootSymbol)-1]=0;
				// Allow options to overrite the CurrencyCode character for 21-character options
				strncpy(btr->TradeRec.ExSymbol.Symbol,vsres->rfix.TagStr(48),sizeof(btr->TradeRec.ExSymbol.Symbol)+1);
				btr->TradeRec.ExSymbol.Symbol[sizeof(btr->TradeRec.ExSymbol.Symbol)-1]=0;
				btr->TradeRec.ExSymbol.Exchange=12;
				btr->TradeRec.ExSymbol.Region=0;
			}
			break;
		case 54: 
			switch(vsres->rfix.TagChar(54))
			{
			case 'Z':
			case '1': btr->TradeRec.Action=1; break;
			case '2': btr->TradeRec.Action=2; break;
			case '3': btr->TradeRec.Action=1; break;  // Mark Buy minus (54=3) as a buy for Oats Gen purposes
			case '5': btr->TradeRec.Action=3; break;
			case '6': btr->TradeRec.Action=3; break;
			case 'H': 
				btr->TradeRec.Action=2; 
				btr->mifid.Undisclosed = 'Y';
				break;
			};
			break;
		case 44: 
			btr->TradeRec.Price=(float)vsres->rfix.TagFloat(44); 
			btr->TradeRec.PriceDbl=vsres->rfix.TagFloat(44);
			if(btr->TradeRec.Type==0) btr->TradeRec.Type=2;
			break;
		case 99: btr->TradeRec.StopAmount=(float)vsres->rfix.TagFloat(99); break;
		case 38:
			if(!vsres->rfix.GetTag(151))
				btr->TradeRec.Shares=vsres->rfix.TagInt(38); 
			break;
		case 40: 
			switch(vsres->rfix.TagChar(40))
			{
			case '1': btr->TradeRec.Type=1; break;
			case '2': btr->TradeRec.Type=2; break;
			case '3': btr->TradeRec.Type=1; break;
			case '4': btr->TradeRec.Type=2; break;
			case '5': btr->TradeRec.Type=1; btr->TradeRec.OnClose=1; break;
			case '7': btr->TradeRec.Type=2; break;
			case '8': btr->TradeRec.Type=2; break;
			case 'A': btr->TradeRec.Type=1; btr->TradeRec.OnClose=1; break;
			case 'B': btr->TradeRec.Type=2; btr->TradeRec.OnClose=1; break;
			case 'P': 
				btr->TradeRec.Type=(vsres->rfix.TagFloat(44)!=0.0) ?2 :1; 
				btr->TradeRec.PegOrder=1; 
				break;
			case 'S': btr->TradeRec.Type=2; break; // Stop market to limit?
			};
			break;
		case 111: btr->TradeRec.ReservedShares=vsres->rfix.TagInt(111); break;
		case 388: btr->TradeRec.Disc_Sign=vsres->rfix.TagChar(388); break;
		case 389: btr->TradeRec.DiscretionaryAmount=(float)vsres->rfix.TagFloat(389); break;
		case 18: 
			switch(vsres->rfix.TagChar(18))
			{
			case 'G': btr->TradeRec.AON=true; break;
			case 'M': btr->TradeRec.PegOrder=0; btr->TradeRec.PegMid=true; break;
			case 'P': btr->TradeRec.PegOrder=0; btr->TradeRec.PegMkt=true; break;
			case 'R': btr->TradeRec.PegOrder=0; btr->TradeRec.PegPrime=true; break;
			// Execution Instructions Cam Code
			case '1': btr->TradeRecB.NotHeldOrder=1; break;
			};
			break;
		case 47: 
		case 8047:
		case 19001:
		{
			const char *capacity=vsres->rfix.TagStr(tag.no);
			decodeTranTypeCode(btr, capacity);
			break;
		}
		case 29: 
		{
			btr->TradeRec.LastCapacity=vsres->rfix.TagInt(29);
			break;
		}
		case 59:
		{
			btr->TradeRec.TimeInForce = vsres->rfix.TagChar(59);
			// Also support some old/backward compatible settings
			switch(vsres->rfix.TagChar(59))
			{
			case '2': 
				btr->TradeRec.OnOpen=true; 
				break;
			case '3': 
				btr->TradeRec.IOC_ArcaNow=true; 
				break;
			case '4': 
				btr->TradeRec.FOK=true; 
				break;
			case '5': 
				btr->TradeRec.Ext=true; 
				break;
			};
			break;
		}
		case 60:
		{
			const char *tstr=vsres->rfix.TagStr(60);
			int wsdate=atoi(tstr);
			int wstime=(tstr[9]-'0')*100000 +(tstr[10]-'0')*10000 
				+(tstr[12]-'0')*1000 +(tstr[13]-'0')*100 
				+(tstr[15]-'0')*10 +(tstr[16]-'0');
			LONGLONG gts=wsdate; gts*=1000000; gts+=wstime;
			SYSTEMTIME lst;
			string stz;
			GMTToEST(gts,lst,stz);
			wsdate=(lst.wYear*10000) +(lst.wMonth*100) +(lst.wDay);
			wstime=(lst.wHour*10000) +(lst.wMinute*100) +(lst.wSecond);
			// DEV-22005
			//sprintf(btr->TradeRec.FileDate,"%08d",wsdate);
			sprintf(btr->TradeRec.FileDate,"%08d",vsres->rfix.fci->Proto);
			sprintf(btr->TradeRec.LastDate,"%08d",wsdate);
			sprintf(btr->TradeRec.LastTime,"%02d:%02d:%02d",wstime/10000,(wstime%10000)/100,wstime%100);
			break;
		}
		case 30: 
		{
			const char *lastMarket=vsres->rfix.TagStr(30);
			//// Moved to LastMarketDestination.txt
			//// Options
			//if(!strcmp(Dest,"Q")) Dest="ASE";
			//else if(!strcmp(Dest,"b")) Dest="BSE";
			//else if(!strcmp(Dest,"W")) Dest="CBO";
			//else if(!strcmp(Dest,"CBX")) Dest="CBO";
			//else if(!strcmp(Dest,"PX")) Dest="PSE";
			//else if(!strcmp(Dest,"5")) Dest="ISE";
			//else if(!strcmp(Dest,"64")) Dest="NDQ";
			//else if(!strcmp(Dest,"8")) Dest="PHS";
			//// Equities
			//else if(!strcmp(Dest,"N")) Dest="NYSE";
			//else if(!strcmp(Dest,"A")) Dest="AMEX";
			//else if(!strcmp(Dest,"I")) Dest="NSDQ";
			//else if(!strcmp(Dest,"C")) Dest="ARCA";
			//else if(!strcmp(Dest,"54")) Dest="BATS";
			//else if(!strcmp(Dest,"57")) Dest="EDGX";
			//else if(!strcmp(Dest,"56")) Dest="EDGA";
			//else if(!strcmp(Dest,"9")) Dest="BRUT";
			//strncpy(btr->TradeRec.PrefECN,Dest,sizeof(btr->TradeRec.PrefECN)); 
			map<string,string>::iterator mit=lastMarketMap.find(lastMarket);
			if(mit!=lastMarketMap.end())
				lastMarket=mit->second.c_str();
			strncpy(btr->TradeRec.PrefECN,lastMarket,sizeof(btr->TradeRec.PrefECN)); 
			btr->TradeRec.PrefECN[sizeof(btr->TradeRec.PrefECN)-1]=0;
			break;		
		}
		case 76: 
			strncpy(btr->TradeRec.MMId,vsres->rfix.TagStr(76),sizeof(btr->TradeRec.MMId)); 
			btr->TradeRec.MMId[sizeof(btr->TradeRec.MMId)-1]=0;
			break;		
		case 100: 
			strncpy(btr->TradeRec.ECNId,vsres->rfix.TagStr(100),sizeof(btr->TradeRec.ECNId)); 
			btr->TradeRec.ECNId[sizeof(btr->TradeRec.ECNId)-1]=0;
			if(!stricmp(btr->TradeRec.ECNId,"TICKET"))
				btr->TradeRecB.RediOrderType='T';
			else if(!stricmp(btr->TradeRec.ECNId,"AWAY"))
			{
				btr->TradeRec.DoNotClear=1;
				if(!stricmp(vsres->rfix.TagStr(113),"Y")) // Other or Floor
					btr->TradeRec.DoNotClear=0;
			}
			break;
		case 217:
		case 143: 
		{
			const char *rediDest=vsres->rfix.TagStr(tag.no);
			decodeRediDestination(btr, rediDest);
			break;
		}
		case 14: btr->TradeRec.ExeShares=vsres->rfix.TagInt(14); break;
		case 151:
		{
			int leavesQty=vsres->rfix.TagInt(151); 
			if(leavesQty!=0)
			{
				btr->TradeRec.Pending=leavesQty;
				btr->TradeRec.Shares=leavesQty;	
			}
			break;
		}
		case 31: btr->TradeRec.LastPrice=vsres->rfix.TagFloat(31);
				 strcpy(btr->LastPriceAsChar, vsres->rfix.TagStr(31));
				 break;

		case 32: btr->TradeRec.LastShares=vsres->rfix.TagInt(32); break;
		case 6: btr->TradeRec.AvgPrice=vsres->rfix.TagFloat(6); break;
		case 17303: // DEV-4834
			{
				string Memo = vsres->rfix.TagStr(17303);
				if(Memo.length() > 22)
				{
					strncpy(btr->TradeRecB.MemoLast3,Memo.substr(22,3).c_str(),sizeof(btr->TradeRecB.MemoLast3));
					btr->TradeRecB.MemoLast3[sizeof(btr->TradeRecB.MemoLast3)-1]=0;
				}
				strncpy(btr->TradeRecA.Memo,Memo.c_str(),sizeof(btr->TradeRecA.Memo));
				btr->TradeRecA.Memo[sizeof(btr->TradeRecA.Memo)-1]=0;
			}
			break;
		case 645: btr->TradeRecB.currentBid=(float)vsres->rfix.TagFloat(645); break;
		case 646: btr->TradeRecB.currentAsk=(float)vsres->rfix.TagFloat(646); break; 
		case 77: 
			if((toupper(vsres->rfix.TagChar(77))=='C')||(vsres->rfix.TagChar(77)=='1'))
			{
				btr->TradeRec.ClosePosition=1; btr->TradeRec.ClosingTrade=1;
			}
			break;
		case 113: /*btr->TradeRecB.NotOATSReportable=1; */break;
		case 126: 
		{
			const char *tstr=vsres->rfix.TagStr(126);
			int wsdate=atoi(tstr);
			int wstime=(tstr[9]-'0')*100000 +(tstr[10]-'0')*10000
				+(tstr[12]-'0')*1000 +(tstr[13]-'0')*100 
				+(tstr[15]-'0')*10 +(tstr[16]-'0');
			LONGLONG gts=wsdate; gts*=1000000; gts+=wstime;
			SYSTEMTIME lst;
			string stz;
			GMTToEST(gts,lst,stz);
			wsdate=(lst.wYear*10000) +(lst.wMonth*100) +(lst.wDay);
			wstime=(lst.wHour*10000) +(lst.wMinute*100) +(lst.wSecond);
			btr->TradeRec.ExpireTime=wstime;
			btr->TradeRec.GtcDate = wsdate;
			break;
		}
		case 9730:
		case 9882:
		case 9960:
		case 8023:
		{
			const char *liq=vsres->rfix.TagStr(tag.no);
			// 1 byte normalized code
			if(liq[1]==0) 
			{
				switch(liq[0])
				{
				case 'A': 
					btr->TradeRec.LiquidityReported=true;
					btr->TradeRec.AddLiquidity=true; btr->TradeRec.RoutedAway=false; 
					btr->TradeRecB.Liquidity2Bytes[0]='A'; btr->TradeRecB.Liquidity2Bytes[1]=0;
					break;
				case 'R': 
					btr->TradeRec.LiquidityReported=true;
					btr->TradeRec.AddLiquidity=false; btr->TradeRec.RoutedAway=false; 
					btr->TradeRecB.Liquidity2Bytes[0]='R'; btr->TradeRecB.Liquidity2Bytes[1]=0;
					break;
				case 'X': 
					btr->TradeRec.LiquidityReported=false;
					btr->TradeRec.AddLiquidity=false; btr->TradeRec.RoutedAway=true; 
					btr->TradeRecB.Liquidity2Bytes[0]='X'; btr->TradeRecB.Liquidity2Bytes[1]=0;
					break;
				default:
					btr->TradeRecB.Liquidity2Bytes[0]=liq[0]; btr->TradeRecB.Liquidity2Bytes[1]=0;
				};
			}
			// 2-byte codes only
			else if(liq[2]==0) 
			{
				btr->TradeRecB.Liquidity2Bytes[0]=liq[0]; btr->TradeRecB.Liquidity2Bytes[1]=liq[1];
			}
			break;
		}
		case 9961:
		{
			const char *liq=vsres->rfix.TagStr(9961);
			btr->TradeRecB.Liquidity2Bytes[0]=liq[0]; btr->TradeRecB.Liquidity2Bytes[1]=liq[1];
			btr->TradeRec.LiquidityReported=1;
			break;
		}
		// Portfolio Cam Code
		case 81: 
		case 8440:
		case 10026:
			strncpy(btr->TradeRecB.Portfolio,vsres->rfix.TagStr(tag.no),sizeof(btr->TradeRecB.Portfolio)); break;
			btr->TradeRecB.Portfolio[sizeof(btr->TradeRecB.Portfolio)-1]=0;
		// Locate Cam Code
		case 2008:
		case 523:
		case 5700:
		case 10027:
			btr->TradeRecB.FeShortLocateSelected=1;
			btr->TradeRecB.ShortLocateMethod=2;
			strncpy(btr->TradeRecA.Notes,vsres->rfix.TagStr(tag.no),sizeof(btr->TradeRecA.Notes));
			btr->TradeRecA.Notes[sizeof(btr->TradeRecA.Notes)-1]=0;
			break;
		// CMTA and CMTA Spreads Cam Code
		case 439: 
		case 440: 
		case 8439: 
		case 10028: btr->TradeRecB.CMTA=vsres->rfix.TagInt(tag.no); break;
		case 200: btr->TradeRec.OptExpireDate=vsres->rfix.TagInt(200)*100; break;
		case 205: btr->TradeRec.OptExpireDate+=vsres->rfix.TagInt(205); break;
		case 201: btr->TradeRec.OptType=vsres->rfix.TagChar(201); break;
		case 202: btr->TradeRec.OptStrikePrice=(float)vsres->rfix.TagFloat(202); break;
		case 167: 
			if((!stricmp(vsres->rfix.TagStr(167),"MLEG"))||
			   (!stricmp(vsres->rfix.TagStr(442),"2"))||(!stricmp(vsres->rfix.TagStr(442),"3")))
				btr->TradeRecB.RediSpreadTrade=1;
			break;
		case 20:
		{
			// DEV-7117
			char ExecTransType=vsres->rfix.TagChar(20);
			if(ExecTransType=='1')
				btr->TradeRec.isBust=1;
			else if(ExecTransType=='2')
				btr->TradeRec.isCorrection=1;
			// DEV-11284
			else if(ExecTransType=='3')
				btr->TradeRecB.isOrderStatus=1;
			break;
		}
		// DEV-6684: Settlement
		case 63: btr->TradeRecA.Settlement=vsres->rfix.TagInt(63); break;
		// DEV-8465: Algo strategy
		case 847:
		{
			const char *rediDestination = vsres->rfix.TagStr(143);
			if (!rediDestination[0]) {
				delayedDecodeAlgoStrategy = true;
			} else {
				decodeAlgoStrategy(btr, vsres->rfix.TagStr(847), rediDestination);
			}
			break;
		}
		// DEV-8465: Commissions
		case 12: 
			btr->TradeRec.OverrideComms=true;
			btr->TradeRecB.CommissionOverride=(float)vsres->rfix.TagFloat(12); 
			break;
		case 13: 
			switch(vsres->rfix.TagInt(13))
			{
			case 1: btr->TradeRec.CommissionOverrideType=1; break;
			case 2: btr->TradeRec.CommissionOverrideType=2; break;
			case 3: btr->TradeRec.CommissionOverrideType=0; break;
			};
			break;

		// DEV-3808
		case 2650: 
			strncpy(btr->TradeRecB.oats_OriginatingDepartmentID,vsres->rfix.TagStr(tag.no),sizeof(btr->TradeRecB.oats_OriginatingDepartmentID));
			btr->TradeRecB.oats_OriginatingDepartmentID[sizeof(btr->TradeRecB.oats_OriginatingDepartmentID)-1]=0;
			break;
		case 2651: 
			btr->TradeRecB.oats_AccountTypeCode=vsres->rfix.TagChar(2651); 
			break;
		case 2653:
			btr->TradeRecB.Solicited=vsres->rfix.TagChar(2653);
			break;
		case 2654: 
			btr->TradeRecB.oats_DeskTypeCode=vsres->rfix.TagChar(2654);
			break;
		case 2655: 
		case 2660:
			{
				const char *tstr=vsres->rfix.TagStr(tag.no);
				btr->TradeRecB.OrderReceivedDate=atoi(tstr);
				btr->TradeRecB.OrderReceivedTime=(tstr[9]-'0')*100000 +(tstr[10]-'0')*10000 
					+(tstr[12]-'0')*1000 +(tstr[13]-'0')*100 
					+(tstr[15]-'0')*10 +(tstr[16]-'0');
				break;
			}
		case 2657: // ManualOrderIndicator
			{
				char manualOrderIndicator=vsres->rfix.TagChar(2657);
				if(manualOrderIndicator=='2')
					btr->TradeRecA.OatsManualTicket = true;
				else 
					btr->TradeRecA.OatsManualTicket = false;
				break;
			}
		// SpecialHandling	
		case 2656:  	
		case 2658:
			{
				const char *tstr= vsres->rfix.TagStr(tag.no);

				char* copy = strdup(tstr); 
				char *tok=strtok(copy," "); 
				int x=0; 
				char* codes[sizeof(tok)]; 

				while(tok && x < sizeof(tok)) 
				{ 
				  codes[x]=tok; 
				  tok=strtok(0," "); 
				  x++;
				} 


				for (int i=0; i < x; i++)
				{
					if(strcmp (codes[i],"ADD") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_ADD;
					else if (strcmp (codes[i],"AON") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_AON;
					/*else if (strcmp (codes[i], "CND") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "CNH") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_CNH;
					/*else if (strcmp (codes[i], "CSH") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "DIR") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_DIR;
					else if (strcmp (codes[i], "DLO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_DLO;
					else if (strcmp (codes[i], "E.W") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_EPT;
					/*else if (strcmp (codes[i], "FBA") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "FOK") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_FOK;
					else if (strcmp (codes[i], "G") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_Go;
					/*else if (strcmp (codes[i], "IDX") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "IO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IO;
					else if (strcmp (codes[i], "IOC") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;
					/*else if (strcmp (codes[i], "ISO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "LOC") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_LOC;
					else if (strcmp (codes[i], "LOO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_LOO;
					else if (strcmp (codes[i], "MAC") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_MAC;
					else if (strcmp (codes[i], "MAO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_MAO;
					else if (strcmp (codes[i], "MOC") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_MOC;
					else if (strcmp (codes[i], "MOO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_MOO;
					else if (strcmp (codes[i], "MQT") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_MQT;
					/*else if (strcmp (codes[i], "MPT") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "MTL") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_MTL;
					/*else if (strcmp (codes[i], "ND") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "NH") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_NH;
					else if (strcmp (codes[i], "OPT") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_OPT;
					else if (strcmp (codes[i], "OVD") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_OVD;
					else if (strcmp (codes[i], "PEG") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_PEG;
					else if (strcmp (codes[i], "RSV") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_RSV;
					/*else if (strcmp (codes[i], "S.W") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "SCL") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_SCL;
					/*else if (strcmp (codes[i], "SLR") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;
					else if (strcmp (codes[i], "SLQ") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;
					else if (strcmp (codes[i], "SOQ") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_IOC;*/
					else if (strcmp (codes[i], "TMO") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_TMO;
					else if (strcmp (codes[i], "TS") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_TS;
					else if (strcmp (codes[i], "WRK") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_WRK;
					else if (strcmp (codes[i], "F0") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_F0;
					else if (strcmp (codes[i], "F3") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_F3;
					else if (strcmp (codes[i], "F6") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_F6;
					else if (strcmp (codes[i], "F7") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_F7;
					else if (strcmp (codes[i], "F8") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_F8;
					else if (strcmp (codes[i], "F9") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_F9;
					else if (strcmp (codes[i], "FA") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FA;
					else if (strcmp (codes[i], "FB") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FB;
					else if (strcmp (codes[i], "FC") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FC;
					else if (strcmp (codes[i], "FD") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FD;
					else if (strcmp (codes[i], "FH") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FH;
					else if (strcmp (codes[i], "FI") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FI;
					else if (strcmp (codes[i], "FJ") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FJ;
					else if (strcmp (codes[i], "FK") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FK;
					else if (strcmp (codes[i], "FL") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_FL;
					else if (strcmp (codes[i], "FM") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_FM;
					else if (strcmp (codes[i], "FN") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FN;
					else if (strcmp (codes[i], "FO") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FO;
					else if (strcmp (codes[i], "FP") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FP;
					else if (strcmp (codes[i], "FR") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FR;
					else if (strcmp (codes[i], "FS") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FS;
					else if (strcmp (codes[i], "FT") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FT;
					else if (strcmp (codes[i], "FW") == 0)
						btr->TradeRecB.OATS_special = btr->TradeRecB.OATS_special | OT_FW;
					else if (strcmp (codes[i], "FX") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FX;
					else if (strcmp (codes[i], "FY") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FY;
					else if (strcmp (codes[i], "FZ") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_FZ;
					else if (strcmp (codes[i], "Fb") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_Fb;
					else if (strcmp (codes[i], "Fc") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_Fc;
					else if (strcmp (codes[i], "Fd") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_Fd;
					else if (strcmp (codes[i], "Fe") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_Fe;
					else if (strcmp (codes[i], "AOB") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_AOB;
					else if (strcmp (codes[i], "MOB") == 0)
						btr->TradeRecB.OATS_special2 = btr->TradeRecB.OATS_special2 | OT_MOB;
				}

				break;
			}		
		case 10596: // DEV-11617
			{
				const char *MsgSource=vsres->rfix.TagStr(10596);
				if((!strncmp(MsgSource,"GC",2))||(!strncmp(MsgSource,"CC",2)))
					btr->TradeRecA.FIXLineId=MsgSource[0]-'A'+1;
				break;
			}
		
			// DEV-21994
		case 15:  // Currency
			_snprintf(btr->TradeRecB.Currency,3,"%s",vsres->rfix.TagStr(15));
			break;
		case 11016: // Settlement Currency
			_snprintf(btr->TradeRec.SettlementCurrency,3,"%s",vsres->rfix.TagStr(11016));
			break;
		case 1595: // Product Type Code
			btr->TradeRecA.ProductType = decodeProductType(vsres->rfix.TagStr(1595));
			decodeFutureExchange(vsres->rfix.TagStr(1595),vsres->rfix.TagInt(442),btr->TradeRec.ExSymbol.Exchange,btr->TradeRec.ExSymbol.Region);
			break;
		case 10583: // REDI RegionGroup
			btr->TradeRecA.RegionGroup = static_cast<char>(getRegionGroupFromString(vsres->rfix.TagStr(10583)));
			break;
		// MIFID settings
		case 52:
			if (miFID)
				memcpy(miFID->SendingTime, vsres->rfix.TagStr(52), 21);
			break;
		case 20001:
			if (miFID)
				_snprintf(miFID->BrokerLEI, 20, "%s", vsres->rfix.TagStr(20001));
			break;
		case 20072:
			if (miFID)
				_snprintf(miFID->ApaMicCode, 5, "%s", vsres->rfix.TagStr(20072));
			break;
		case 20073:
			if (miFID)
				_snprintf(miFID->TransReportVenue, 5, "%s", vsres->rfix.TagStr(20073));
			break;
		case 2524:
			if (miFID)
				miFID->TradeReportIndicator = vsres->rfix.TagInt(2524);
			break;
		case 20059:
			if (miFID)
				miFID->RoleIndicator = vsres->rfix.TagInt(20059);
			break;
		case 574:
			if (miFID)
				miFID->MatchingType = vsres->rfix.TagInt(574);
			break;
		case 829:
			if (miFID)
				miFID->TradeSubType = vsres->rfix.TagInt(829);
			break;
		case 855:
			if (miFID)
				miFID->SecondaryTrdType = vsres->rfix.TagInt(855);
			break;
		case 527:
			if (miFID)
				_snprintf(miFID->SecondaryExecID, 41, "%s", vsres->rfix.TagStr(527));
			break;
		case 8014:
			if (miFID)
			{
				std::stringstream ss( vsres->rfix.TagStr(8014) );
				int curr = 0;
				while (ss.good())
				{
					string substr;
					getline(ss, substr, ' ');
					if (substr == "13")
						miFID->TradePriceCondition[curr++] = 'D';
					else if (substr == "14")
						miFID->TradePriceCondition[curr++] = 'E';
					else if (substr == "16")
						miFID->TradePriceCondition[curr++] = 'G';
				}
				miFID->NoTradePriceConditions = curr;
			}
			break;
		case 8028:
			if (miFID)
			{
				std::stringstream ss(vsres->rfix.TagStr(8028));
				int curr = 0;
				while (ss.good())
				{
					string substr;
					getline(ss, substr, ' ');
					if (substr == "0")
						miFID->TrdRegPublicationType[curr++] = '0';
					else if (substr == "1")
						miFID->TrdRegPublicationType[curr++] = '1';
				}
				miFID->NoTrdRegPublications = curr;
			}
			break;
		case 8013:
			if (miFID)
				_snprintf(miFID->TrdRegPublicationReason, 6, "%s", vsres->rfix.TagStr(8013));
			break;
		case 8015:
			if (miFID)
			{
				miFID->RiskReduction = 'N';
				miFID->SystematicInternalizer = 'N';
				std::stringstream ss(vsres->rfix.TagStr(8015));
				int curr = 0;
				while (ss.good())
				{
					string substr;
					getline(ss, substr, ' ');
					if (substr == "3")
						miFID->RiskReduction = 'Y';
					else if (substr == "5")
						miFID->SystematicInternalizer = 'Y';
				}
			}
			break;
		case 2704:
			if (miFID)
			{
				switch (vsres->rfix.TagInt(2704))
				{
				case 0: miFID->TradingInst = '0'; break;
				case 1: miFID->TradingInst = '1'; break;
				case 2: miFID->TradingInst = '2'; break;
				case 3: miFID->TradingInst = '3'; break;
				}
			}
			break;
		case 2667:
			if (miFID)
			{
				switch (vsres->rfix.TagInt(2667))
				{
				case 0: miFID->AlgoOrder = 'N'; break;
				case 1: miFID->AlgoOrder = 'Y'; break;
				}
			}
			break;
		case 2672:
			if (miFID)
				memcpy(miFID->TransactTime, vsres->rfix.TagStr(2672), 21);
			break;
		case 378:
			btr->RestateReason = vsres->rfix.TagChar(378);
			break;

		};
	}

	// Fill in order info that may be missing on a detail
	if(vsres->rfix.fci)
	{
		// DEV-6538: Use the AppInstance associated with the order, so the historic instance won't need the GS_user.csv files
		if((!btr->TradeRec.Domain[0])&&(vsres->rfix.fci->LogonTargetSubID[0]))
		{
			strncpy(btr->TradeRec.Domain,vsres->rfix.fci->LogonTargetSubID,sizeof(btr->TradeRec.Domain));
			btr->TradeRec.Domain[sizeof(btr->TradeRec.Domain)-1]=0;
		}
		AppSystem *asys=(AppSystem*)vsres->rfix.fci->mFixSess;
		const char *clordid=vsres->rfix.TagStr(11);//btr->TradeRecA.ECNParserOrderNo;
	#ifdef GS_RTECHOATS
		if((btr->TradeRec.Domain[0]=='_')&&(btr->TradeRec.LastStatus==TRADE_CONFIRMED))
		{
			//clordid=vsres->rfix.TagStr(109);
			strncpy(btr->TradeRec.OrderNo,clordid,sizeof(btr->TradeRec.OrderNo));
			btr->TradeRec.OrderNo[sizeof(btr->TradeRec.OrderNo)-1]=0;
			strncpy(btr->TradeRecA.ECNParserOrderNo,clordid,sizeof(btr->TradeRecA.ECNParserOrderNo));
			btr->TradeRecA.ECNParserOrderNo[sizeof(btr->TradeRecA.ECNParserOrderNo)-1]=0;
		}
	#endif
		int odate=atoi(btr->TradeRec.FileDate);
		OrderDB *pdb=&asys->odb;
	#ifdef MULTI_DAY_HIST
		if(pdb->HISTORIC_DAYS>0)
		{
			OrderDB *sdb=asys->GetDB(odate);
			if(sdb) pdb=sdb;
		}
		char okey[256]={0};
		if(odb.INDEX_ORDERDATE)
			sprintf(okey,"%08d.%s",odate,clordid);
		else
			strcpy(okey,clordid);
		strupr(okey);
		VSINDEX::iterator oit=pdb->omap.find(okey);
	#else
		VSINDEX::iterator oit=pdb->omap.find(clordid);
	#endif
		//#ifdef MULTI_DAY
		//if((oit==pdb->omap.end())&&(odb.INDEX_ORDERDATE)&&(QueryTimeZoneBias!=0))
		//{
		//	odate=CalcTPlusDate(odate,(QueryTimeZoneBias>0)?+1:-1,false);
		//	sprintf(okey,"%08d.%s",odate,clordid);
		//	strupr(okey);
		//	oit=pdb->omap.find(okey);
		//}
		//#endif
		if(oit!=pdb->omap.end())
		{
			ITEMLOC& oloc=oit.second;
			VSOrder *porder=pdb->ReadOrder(oloc);
			if(porder)		
			{
				// ViewServer models cancel/replace correctly, so replaced orders get HighExecType=5
				char HighExecType=porder->GetHighExecType();
				if((porder->IsTerminated())&&((HighExecType=='0')||(HighExecType=='5')||(HighExecType=='1')))
					btr->TradeRec.OrderWasCancelReplaced=true;
				// Copy ECNID, TranTypeCode and RediDestination from first order in cancel/replace chain
				if(!btr->TradeRec.ECNId[0] || !btr->TradeRecB.TranTypeCode || !btr->TradeRecB.RediDestination[0] || delayedDecodeAlgoStrategy ||
				   !btr->TradeRecB.Currency[0] || !btr->TradeRec.SettlementCurrency[0] || !btr->TradeRecA.ProductType || !btr->TradeRecA.RegionGroup) // DEV-22191
				{
					const char *FirstClOrdID=porder->GetFirstClOrdID();
					if(*FirstClOrdID)
					{
						// Read first order in cancel/replace chain
						if(odb.INDEX_ORDERDATE)
							sprintf(okey,"%08d.%s",odate,FirstClOrdID);
						else
							strcpy(okey,FirstClOrdID);
						strupr(okey);
						VSINDEX::iterator foit=pdb->omap.find(okey);
						//#ifdef MULTI_DAY
						//if((foit==pdb->omap.end())&&(odb.INDEX_ORDERDATE)&&(QueryTimeZoneBias!=0))
						//{
						//	odate=CalcTPlusDate(odate,(QueryTimeZoneBias>0)?-1:+1,false);
						//	sprintf(okey,"%08d.%s",odate,FirstClOrdID);
						//	foit=pdb->omap.find(okey);
						//}
						//#endif
						if(foit!=pdb->omap.end())
						{
							ITEMLOC& foloc=foit.second;
							VSOrder *firstOrder=pdb->ReadOrder(foloc);
							if(firstOrder)
							{
								if (!btr->TradeRec.ECNId[0]) 
								{
									const char *EcnID=firstOrder->GetAuxKey();
									if(*EcnID)
									{
										strncpy(btr->TradeRec.ECNId,EcnID,sizeof(btr->TradeRec.ECNId));
										btr->TradeRec.ECNId[sizeof(btr->TradeRec.ECNId)-1]=0;
									}
								}

								if (!btr->TradeRecB.TranTypeCode || !btr->TradeRecB.RediDestination[0] || delayedDecodeAlgoStrategy ||
									!btr->TradeRecB.Currency[0] || !btr->TradeRec.SettlementCurrency[0] || !btr->TradeRecA.ProductType || !btr->TradeRecA.RegionGroup) 
								{
									unsigned short dlen=0;
									unsigned short uscPortNo=0;
									LONGLONG doff=firstOrder->GetDetail(0, uscPortNo, dlen);
									if((uscPortNo>=0)&&(uscPortNo<NO_OF_USC_PORTS)&&(UscPort[uscPortNo].DetPtr1))
									{
										VSDetailFile *dfile=(VSDetailFile *)UscPort[uscPortNo].DetPtr1;
										char *dptr=new char[dlen+1];
										if(pdb->ReadDetail(dfile, doff, dptr, dlen) > 0)
										{
											FIXINFO ifix;
											ifix.Reset();
											ifix.FIXDELIM=0x01;
											ifix.noSession=true;
											ifix.supressChkErr=true;
											ifix.FixMsgReady(dptr,dlen);

											if (!btr->TradeRecB.TranTypeCode) {
												const char *firstCapacity;
												firstCapacity = ifix.TagStr(47);
												if (!firstCapacity[0]) {
													firstCapacity = ifix.TagStr(8047);
													if (!firstCapacity[0]) {
														firstCapacity = ifix.TagStr(19001);
													}
												}
												if (firstCapacity[0]) {
													decodeTranTypeCode(btr, firstCapacity);
												}
											}

											if (!btr->TradeRecB.RediDestination[0] || delayedDecodeAlgoStrategy) {
												const char *firstRediDestination;
												firstRediDestination = ifix.TagStr(143);
												if (!firstRediDestination[0]) {
													firstRediDestination = ifix.TagStr(217);
												}
												if (!btr->TradeRecB.RediDestination[0] && firstRediDestination[0]) {
													decodeRediDestination(btr, firstRediDestination);
												}
												if (delayedDecodeAlgoStrategy) {
													const char *algorithm = ifix.TagStr(847);
													if (algorithm[0]) {
														decodeAlgoStrategy(btr, algorithm, firstRediDestination);
													}
												}
											}

											// DEV-22191
											if(!btr->TradeRecB.Currency[0])
												_snprintf(btr->TradeRecB.Currency,3,"%s",ifix.TagStr(15));
											if(!btr->TradeRec.SettlementCurrency[0])
												_snprintf(btr->TradeRec.SettlementCurrency,3,"%s",ifix.TagStr(11016));
											if(!btr->TradeRecA.ProductType)
												btr->TradeRecA.ProductType = decodeProductType(ifix.TagStr(1595));
											if(!btr->TradeRecA.RegionGroup)
												btr->TradeRecA.RegionGroup = static_cast<char>(getRegionGroupFromString(ifix.TagStr(10583)));
										}
										if(dptr) delete dptr;
									}
								}
	
								// Worried about freeing order another client is looking at the same time
								// However, when we HAVE_FUSION_IO, no order cache is used so no other client will be sharing this VSOrder structure
								if((firstOrder!=porder)&&(pdb->HAVE_FUSION_IO))
									pdb->FreeOrder(firstOrder);
							}
						}
					}
				}
				// Mark equity spread legs by MLEG securitytype
				char SecurityType[256]={0};
				strncpy(SecurityType,porder->GetAuxKey(1),256); SecurityType[255]=0;
				SecurityType[sizeof(SecurityType)-1]=0;
				if((!stricmp(SecurityType,"MLEG"))||
				   (!stricmp(vsres->rfix.TagStr(442),"2"))||(!stricmp(vsres->rfix.TagStr(442),"3")))
					btr->TradeRecB.RediSpreadTrade=1;
				if((btr->TradeRecB.RediSpreadTrade)&&(!vsres->rfix.GetTag(201)))
				{
					btr->TradeRec.Type=1;
					btr->TradeRec.Price=0.0;
					btr->TradeRec.PriceDbl=0.0;
				}
				// Make sure tickets have domains though tag 1 may be missing
				if(btr->TradeRecB.OrderStagingParent)
				{
					if(!btr->TradeRec.Domain[0])
					{
						strncpy(btr->TradeRec.Domain,porder->GetAppInstID(),sizeof(btr->TradeRec.Domain));
						btr->TradeRec.Domain[sizeof(btr->TradeRec.Domain)-1]=0;
					}
				}
			#ifdef GS_RTECHOATS
				// It's okay if these fixups are inefficient, because they're only used for RTECHOATS POC
				if(btr->TradeRec.Domain[0]=='_')
				{
					// When tag 1 (account) is missing from the drop
					if(!btr->TradeRec.Account[0])
					{
						strncpy(btr->TradeRec.Account,porder->GetAccount(),sizeof(btr->TradeRec.Account));
						btr->TradeRec.Account[sizeof(btr->TradeRec.Account)-1]=0;
					}
					// DEV-7240: Fill in missing RTECHOATS tag 41 for replace
					if((!btr->TradeRecA.CancelMemo[0])&&(vsres->rfix.TagChar(150)=='5'))
					{
						ITEMLOC oloc=porder->GetOffset();
						string FirstClOrdID=porder->GetFirstClOrdID();
						strncpy(btr->TradeRecA.CancelMemo,FirstClOrdID.c_str(),sizeof(btr->TradeRecA.CancelMemo));
						btr->TradeRecA.CancelMemo[sizeof(btr->TradeRecA.CancelMemo)-1]=0;
						for(VSINDEX::m_iterator fit=pdb->fmap.m_find(FirstClOrdID);
							(fit!=pdb->fmap.m_end())&&(fit.first==FirstClOrdID);
							fit++)
						{
							ITEMLOC sloc=fit.second;
							if(sloc<oloc)
							{
								VSOrder *sorder=pdb->ReadOrder(sloc);
								if(sorder)
								{
									strncpy(btr->TradeRecA.CancelMemo,sorder->GetClOrdID(),sizeof(btr->TradeRecA.CancelMemo));
									btr->TradeRecA.CancelMemo[sizeof(btr->TradeRecA.CancelMemo)-1]=0;
									// The limit price is often missing on replace
									if(btr->TradeRec.Price==0.0)
										btr->TradeRec.Price=(float)sorder->GetPrice();
									// Worried about freeing order another client is looking at the same time
									if((sorder!=porder)&&(pdb->HAVE_FUSION_IO))
										pdb->FreeOrder(sorder);
								}
								break;
							}
						}
					}
					// Mark order staging though tag 198 may be missing on replace.
					// Note it's also missing on cancel, but that shouldn't affect OG.
					const char *RootOrderID=porder->GetRootOrderID();
					if(RootOrderID[0])
					{
						// This is a parent ticket
						if(!stricmp(btr->TradeRec.ECNId,"TICKET"))
						{
							btr->TradeRecB.OrderStagingOrder=1;
							btr->TradeRecB.OrderStagingParent=1;
							btr->TradeRec.WorkOrderRefNo=atoi(porder->GetEcnOrderID());
						}
						// Otherwise, it's a child order routed from a parent ticket
						else
						{
							btr->TradeRecB.OrderStagingOrder=1;
							btr->TradeRecB.OrderStagingParent=0;
							btr->TradeRec.WorkOrderRefNo=atoi(RootOrderID);
						}
					}
				}
			#endif
				// Worried about freeing order another client is looking at the same time
				if(pdb->HAVE_FUSION_IO)
					pdb->FreeOrder(porder);
			}
		}
	#ifdef GS_RTECHOATS
		// It's okay if this fixup is inefficient, because it's only used for RTECHOATS POC
		else if((btr->TradeRec.Domain[0]=='_')&&(btr->TradeRec.LastStatus!=TRADE_CONFIRMED))
		{
			const char *eoid=vsres->rfix.TagStr(37);
			if((!*eoid)&&(btr->TradeRec.LastStatus==TRADE_CANCELED)&&(btr->TradeRec.WorkOrderRefNo>0))
			{
				sprintf(btr->TradeRec.ECNOrderNo,"%d",btr->TradeRec.WorkOrderRefNo);
				eoid=btr->TradeRec.ECNOrderNo;
			}
			const char *t60=vsres->rfix.TagStr(60);
			LONGLONG dts=atoi(t60); dts*=1000000;
			dts+=atoi(&t60[9])*10000 +atoi(&t60[12])*100 +atoi(&t60[15]);
			for(VSINDEX::m_iterator oit2=pdb->omap2.m_find(eoid);(oit2!=pdb->omap2.m_end())&&(oit2.first==eoid);oit2++)
			{
				ITEMLOC& oloc2=oit2.second;
				VSOrder *sorder=pdb->ReadOrder(oloc2);
				if(sorder)
				{
					// Match fills to the right replace order
					if(sorder->GetTransactTime()<=dts)
					{
						strncpy(btr->TradeRec.OrderNo,sorder->GetClOrdID(),sizeof(btr->TradeRec.OrderNo));
						btr->TradeRec.OrderNo[sizeof(btr->TradeRec.OrderNo)-1]=0;
						strncpy(btr->TradeRecA.ECNParserOrderNo,sorder->GetClOrdID(),sizeof(btr->TradeRecA.ECNParserOrderNo));
						btr->TradeRecA.ECNParserOrderNo[sizeof(btr->TradeRecA.ECNParserOrderNo)-1]=0;
						// When tag 1 (account) is missing from the drop
						if(!btr->TradeRec.Account[0])
						{
							strncpy(btr->TradeRec.Account,sorder->GetAccount(),sizeof(btr->TradeRec.Account));
							btr->TradeRec.Account[sizeof(btr->TradeRec.Account)-1]=0;
						}
						if(!btr->TradeRec.ECNId[0])
						{
							const char *EcnID=sorder->GetAuxKey();
							if(*EcnID)
							{
								strncpy(btr->TradeRec.ECNId,EcnID,sizeof(btr->TradeRec.ECNId));
								btr->TradeRec.ECNId[sizeof(btr->TradeRec.ECNId)-1]=0;
							}
						}
						// Cancel rejects improperly reported as rejects
						if((btr->TradeRec.LastStatus==TRADE_REJECTED)&&(sorder->GetNumDetails()>1))
							btr->TradeRec.LastStatus=100;
						// Worried about freeing order another client is looking at the same time
						//pdb->FreeOrder(sorder);
						break;
					}
					// Worried about freeing order another client is looking at the same time
					if(pdb->HAVE_FUSION_IO)
						pdb->FreeOrder(sorder);
				}
			}
		}
	#endif
	}

	// Lookup symbol exchange and region
	map<string,WORD>::iterator sit=symExchMap.find(btr->TradeRec.ExSymbol.Symbol);
	if(sit!=symExchMap.end())
	{
		btr->TradeRec.ExSymbol.Exchange=HIBYTE(sit->second);
		btr->TradeRec.ExSymbol.Region=LOBYTE(sit->second);
	}
	map<string,string>::iterator iit=symIsinMap.find(btr->TradeRec.ExSymbol.Symbol);
	if(iit!=symIsinMap.end())
	{
		strncpy(btr->TradeRecB.ISIN,iit->second.c_str(),12);
		btr->TradeRecB.ISIN[sizeof(btr->TradeRecB.ISIN)-1]=0;
	}
	//DWORD tstop=GetTickCount();
	//WSLogEvent("DEBUG ConvertFixToBTR %d ms",tstop -tstart);
	return 0;
}
#endif
