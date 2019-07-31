
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"

#pragma pack(push,8)
#include "MESSAGE.H"
#include "TraderBackupMsg.h"
#pragma pack(pop)

class TBackupFile
{
public:
	TBackupFile()
		:lfp(0)
		,flog(false)
	{
		memset(remotePath,0,MAX_PATH);
		memset(localPath,0,MAX_PATH);
		memset(&Msg500Rec,0,sizeof(MSG500REC));
		memset(&Msg950Rec,0,sizeof(MSG950REC));
	}
	~TBackupFile()
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
	MSG500REC Msg500Rec;
	MSG950REC Msg950Rec;
};
typedef map<string,TBackupFile *> TBACKUPMAP;

class TBackupClient
{
public:
	TBackupClient()
	{
		memset(remoteBase,0,MAX_PATH);
		memset(localBase,0,MAX_PATH);
	}
	~TBackupClient()
	{
		for(TBACKUPMAP::iterator fit=fmap.begin();fit!=fmap.end();fit++)
			delete fit->second;
		fmap.clear();
	}

	char remoteBase[MAX_PATH];
	char localBase[MAX_PATH];
	TBACKUPMAP fmap;
};

int ViewServer::TBackupOpened(int ConPortNo)
{
	ConPort[ConPortNo].DetPtr5=new TBackupClient;
	return 0;
}
int ViewServer::TBackupClosed(int ConPortNo)
{
	TBackupClient *tbc=(TBackupClient *)ConPort[ConPortNo].DetPtr5;
	if(tbc)
	{
		delete tbc; ConPort[ConPortNo].DetPtr5=0;
	}
	return 0;
}
int ViewServer::HandleConTbackupMsg(const MSGHEADER *MsgHeader, char *Msg, int PortNo)
{
	TBackupClient *tbc=(TBackupClient *)ConPort[PortNo].DetPtr5;
	if(!tbc)
		return -1;
	FixStats *cstats=(FixStats *)ConPort[PortNo].DetPtr3;
	if(!cstats)
		return -1;
	switch(MsgHeader->MsgID)
	{
	case 472: // VersionRequest
	case 474: // FileTransferRequest
	case 475: // NewExeReady
	case 479: // GenerateErrorEventLogsRequest
		break;
	case 495: // InitBackup
	{
		if(MsgHeader->MsgLen<sizeof(MSG495REC))
			return -1;
		MSG495REC Msg495Rec;
		memcpy(&Msg495Rec,Msg,sizeof(MSG495REC));
		WSLogEvent("CON%d: InitBackup(%s)",PortNo,Msg495Rec.baseDirectory);
		strncpy(tbc->remoteBase,Msg495Rec.baseDirectory,sizeof(tbc->remoteBase)-1);
		#ifdef WIN32
		sprintf(tbc->localBase,"%s\\data",pcfg->RunPath.c_str());
		#else
		sprintf(tbc->localBase,"%s/data",pcfg->RunPath.c_str());
		#endif
		tbc->remoteBase[sizeof(tbc->remoteBase) -1]=0;
		break;
	}
	case 500: // FileNotification
	{
		if(MsgHeader->MsgLen<sizeof(MSG500REC))
			return -1;
		MSG500REC Msg500Rec;
		memcpy(&Msg500Rec,Msg,sizeof(MSG500REC));
		if(Msg500Rec.action==MSG500_ACTION_TR_INFORM_HASH)
		{
			// TODO: We get duplicate notifications (probably bug in Trader)
			char rpath[MAX_PATH]={0};
			strncpy(rpath,Msg500Rec.relativeFilePath,sizeof(rpath)-1);
			_strupr(rpath);
			if((strstr(rpath,"FULLLOG"))&&(strstr(rpath,WScDate)))
			{
				TBACKUPMAP::iterator fit=tbc->fmap.find(rpath);
				if(fit==tbc->fmap.end())
				{
					WSLogEvent("CON%d: FileNotification(%s)",PortNo,Msg500Rec.relativeFilePath);
					TBackupFile *tfile=new TBackupFile;
					tfile->flog=true;
					strncpy(tfile->remotePath,Msg500Rec.relativeFilePath,sizeof(tfile->remotePath)-1);
					tfile->remotePath[sizeof(tfile->remotePath)-1]=0;
					tbc->fmap.insert(pair<string,TBackupFile*>(rpath,tfile));
				}
			}
			// If we want any other logs
			//else
			//{
			//	TBackupFile *tfile=new TBackupFile;
			//	tfile->flog=false;
			//#ifdef WIN32
			//	sprintf(tfile->localPath,"%s\\%s",tbc->localBase,rpath);
			//	char *rptr=strchr(tfile->localPath +strlen(tbc->localBase) +1,'\\');
			//#else
			//	sprintf(tfile->localPath,"%s/%s",tbc->localBase,rpath);
			//	char *rptr=strchr(tfile->localPath +strlen(tbc->localBase) +1,'/');
			//#endif
			//	if(rptr) *rptr='_';
			//	_strupr(tfile->localPath);
			//	tfile->lfp=fopen(tfile->localPath,"wb");
			//	if(tfile->lfp)
			//	{
			//		 TODO: compute local file's digest
			//	}
			//	else
			//		WSLogError("CON%d: Failed opening %s for write!",PortNo,tfile->localPath);
			//}
		}
		break;
	}
	case 950: // LiveBackupMessage
	{
		if(MsgHeader->MsgLen<sizeof(MSG950REC))
			return -1;
		MSG950REC* Msg950Rec=(MSG950REC*)Msg;
		if(Msg950Rec->Command==BA_COM_OPEN)
		{
			char rpath[MAX_PATH]={0};
			strncpy(rpath,Msg +sizeof(MSG950REC),Msg950Rec->DataLen); rpath[Msg950Rec->DataLen]=0;
			_strupr(rpath);
			TBACKUPMAP::iterator fit=tbc->fmap.find(rpath);
			if(fit!=tbc->fmap.end())
			{
				TBackupFile *tfile=fit->second;
				tfile->Msg950Rec=*Msg950Rec;
				if(!tfile->Msg500Rec.action)
				{
					// Request the file contents
					tfile->Msg500Rec.action=MSG500_ACTION_TB_REQUEST_FILE;
					strcpy(tfile->Msg500Rec.relativeFilePath,tfile->remotePath);
					WSLogEvent("CON%d: Requesting file transfer for (%s)...",PortNo,tfile->remotePath);
					WSConSendMsg(500,sizeof(MSG500REC),(char*)&tfile->Msg500Rec,PortNo);
				}
			}
		}
		else if(Msg950Rec->Command==BA_COM_WRITE)
		{
			for(TBACKUPMAP::iterator fit=tbc->fmap.begin();fit!=tbc->fmap.end();fit++)
			{
				TBackupFile *tfile=fit->second;
				if(tfile->Msg950Rec.FileHandle==Msg950Rec->FileHandle)
				{
					// Write FIX to the detail file
					if(tfile->flog)
					{
						int UscPortNo=(int)(PTRCAST)ConPort[PortNo].DetPtr4;
						VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
						if(dfile)
						{
							LONGLONG doff=Msg950Rec->DataOffset>=0?(LONGLONG)Msg950Rec->DataOffset:-1;
							odb.WriteDetailBlock(dfile,Msg +sizeof(MSG950REC),Msg950Rec->DataLen,doff);
							cstats->msgCnt+=Msg950Rec->DataLen; cstats->totMsgCnt+=Msg950Rec->DataLen;
						}
					}
					else
					{
						_ASSERT(false);//untested
						fseeko(tfile->lfp,Msg950Rec->DataOffset,SEEK_SET);
						fwrite(Msg +sizeof(MSG950REC),1,Msg950Rec->DataLen,tfile->lfp);
					}
					break;
				}
			}
		}
		else if(Msg950Rec->Command==BA_COM_CLOSE)
		{
			for(TBACKUPMAP::iterator fit=tbc->fmap.begin();fit!=tbc->fmap.end();fit++)
			{
				TBackupFile *tfile=fit->second;
				if(tfile->Msg950Rec.FileHandle==Msg950Rec->FileHandle)
				{
					break;
				}
			}
		}
		break;
	}
	case 822: // ReceiveTextFile
	case 1822: // ReceiveTextFile
	{
		if(MsgHeader->MsgLen<sizeof(MSG822REC))
			return -1;
		char rpath[MAX_PATH]={0};
		int rsize=0;
		char *rptr=0;
		if(MsgHeader->MsgID==822)
		{
			MSG822REC* Msg822Rec=(MSG822REC*)Msg;
			strncpy(rpath,Msg822Rec->FileDesc,sizeof(rpath)-1);
			rsize=Msg822Rec->Size;
			rptr=Msg822Rec->Block;
		}
		else if(MsgHeader->MsgID==1822)
		{
			MSG1822REC* Msg1822Rec=(MSG1822REC*)Msg;
			strncpy(rpath,Msg1822Rec->FileDesc,sizeof(rpath)-1);
			rsize=Msg1822Rec->Size;
			rptr=Msg1822Rec->Block;
		}
		rpath[sizeof(rpath)-1]=0; _strupr(rpath);
		TBACKUPMAP::iterator fit=tbc->fmap.find(rpath);
		if(fit!=tbc->fmap.end())
		{
			TBackupFile *tfile=fit->second;
			if(rsize>0)
			{
				// Write FIX to the detail file
				if(tfile->flog)
				{
					int UscPortNo=(int)(PTRCAST)ConPort[PortNo].DetPtr4;
					VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
					if(dfile)
					{
						LONGLONG doff=-1;
						odb.WriteDetailBlock(dfile,rptr,rsize,doff);
						cstats->msgCnt+=rsize; cstats->totMsgCnt+=rsize;
					}
				}
				else
				{
					_ASSERT(false);//untested
					fwrite(rptr,1,rsize,tfile->lfp);
				}
			}
		}
		break;
	}
	case 477: // FileTransferDone
	{
		if(MsgHeader->MsgLen<sizeof(MSG477REC))
			return -1;
		MSG477REC* Msg477Rec=(MSG477REC*)Msg;
		break;
	}
	case 478: // FileTransferDoneAck
	{
		if(MsgHeader->MsgLen<sizeof(MSG478REC))
			return -1;
		MSG478REC* Msg478Rec=(MSG478REC*)Msg;
		break;
	}
	default:
		_ASSERT(false);
		return -1;
	};
	return 0;
}
