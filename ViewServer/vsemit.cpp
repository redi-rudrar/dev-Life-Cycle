// DEV-18992
#include <stdafx.h>
#include "ViewServer.h"

static bool GetEmitParameter(const char *cmdbuf, const char *parm, char value[256])
{
	value[0]=0;
	char search[256]={0};
	sprintf(search,"%s=",parm);
	const char *sptr=strstr(cmdbuf,search);
	if(!sptr)
		return false;
	sptr+=strlen(search);
	const char *eptr=strchr(sptr,'|');
	if(!eptr) eptr=strchr(sptr,0x0D);
	if(!eptr) eptr=strchr(sptr,0x0A);
	if(!eptr) eptr=sptr +strlen(sptr);
	int vlen=(int)(eptr -sptr);
	strncpy(value,sptr,vlen);
	value[vlen]=0;
	return true;
}

DropClient *ViewServer::GetDropClient(const char *sender, const char *target)
{
	for(DCMAP::iterator dit=dcmap.begin();dit!=dcmap.end();dit++)
	{
		DropClient *pdc=dit->second;
		if((pdc->scid==sender)&&(pdc->tcid==target))
			return pdc;
	}
	return 0;
}

int ViewServer::LoadEmitFIXMapping()
{
	FILE *fp=fopen("setup\\emitfix.txt","rt");
	if(!fp)
		return -1;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		// Remove CRLF
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(*rptr!=0x0D)&&(*rptr!=0x0A);rptr++)
			;
		if(rptr) *rptr=0;
		// Skip leading spaces
		for(rptr=rbuf;(*rptr)&&(!isprint(*rptr));rptr++)
			;
		// Blank line
		if((!*rptr)||(!strncmp(rptr,"//",2)))
			continue;
		char *setting=strtok(rptr,"=");
		const char *val=setting ?strtok(0,"=") :0;
		if((!setting)||(!val))
			continue;
		_strupr(setting);
		int tag=atoi(val);
		if(tag>0)
		{
			emitTagMap[setting]=tag;
			emitTagReverseMap[tag]=setting;
		}
	}
	fclose(fp);
	return emitTagMap.size();
}

int ViewServer::TranslateEmitMessage(FIXINFO& ifx, char *cmdbuf)
{
	int ntags=0;
	if(emitTagMap.empty())
		LoadEmitFIXMapping();
	for(const char *tok=strtok(cmdbuf,"|");tok;tok=strtok(0,"|"))
	{
		char cmd[256]={0},val[256]={0};
		const char *tptr=strchr(tok,'=');
		if(tptr)
		{
			int tlen=(int)(tptr-tok);
			strncpy(cmd,tok,tlen); cmd[tlen]=0;
			strcpy(val,tptr+1);
		}
		else
			strcpy(cmd,tok);

		_strupr(cmd);
		map<string,int>::iterator eit=emitTagMap.find(cmd);
		if(eit!=emitTagMap.end())
		{
			ifx.SetTag(eit->second,val); ntags++;
		}
	}
	return ntags;
}

const string ViewServer::GenerateEmitClOrdID()
{
	char clordid[64]={0};
	sprintf(clordid,"%08dEm%06d",WSDate,++emitSeqno);
	return clordid;
}

int ViewServer::EmitMsgReady(int PortNo)
{
	int Size=UsrPort[PortNo].InBuffer.LocalSize;
	for(int i=0;i<Size;i++)
	{
		if(UsrPort[PortNo].InBuffer.Block[i]==0x0A)
		{
			for(i++;(i<Size)&&((UsrPort[PortNo].InBuffer.Block[i]==0x0D)||(UsrPort[PortNo].InBuffer.Block[i]==0x0A));i++)
				;
			return i;
		}
	}
	return 0;
}

void ViewServer::EmitPortClosed(int PortNo)
{
	map<string,int> tmpMap;
	for(map<string,int>::iterator cit=emitClientOrderMap.begin();cit!=emitClientOrderMap.end();cit++)
	{
		if(cit->second!=PortNo)
			tmpMap.insert(*cit);
	}
	emitClientOrderMap.swap(tmpMap);
}

//POST / HTTP/1.1
//Content-Type: application/x-www-form-urlencoded; charset=utf-8
//Host: amcvw-db0035.rdti.com
//Action=GenerateOats
void ViewServer::HandleEmitRequest(int PortNo, int Size)
{
	char cmdbuf[4096]={0};
	int msize=Size;
	if(msize>sizeof(cmdbuf)-1)
		msize=sizeof(cmdbuf)-1;
	memcpy(cmdbuf,UsrPort[PortNo].InBuffer.Block,msize);
	cmdbuf[msize]=0;

	char action[256]={0};
	GetEmitParameter(cmdbuf,"Action",action);
	char *mptr;
	for(mptr=cmdbuf;(mptr<cmdbuf+msize)&&(*mptr!=0x0D)&&(*mptr!=0x0A);mptr++)
		;
	int mlen=(int)(mptr -cmdbuf); 
	for(;(mptr<cmdbuf+msize)&&((*mptr==0x0D)||(*mptr==0x0A));mptr++)
		*mptr=0;

	char reply[256]={0};
	if(!strcmp(action,"EmitFixMsg"))
	{
		char sender[256]={0},target[256]={0};
		GetEmitParameter(cmdbuf,"SenderCompId",sender);
		GetEmitParameter(cmdbuf,"TargetCompId",target);
		DropClient *pdc=GetDropClient(sender,target);
	    if(pdc)
		{
			// TODO: Proxy FIX message
			FIXINFO ifx;
			ifx.Reset();
			ifx.noSession=false;
			ifx.supressChkErr=true;
			ifx.FIXDELIM=0x01;
			TranslateEmitMessage(ifx,cmdbuf);
			string clordid=ifx.TagStr(11);
			if((!strcmp(ifx.TagStr(35),"D"))||(!strcmp(ifx.TagStr(35),"F"))||(!strcmp(ifx.TagStr(35),"G")))
			{
				if(clordid.empty())
				{
					clordid=GenerateEmitClOrdID();
					ifx.SetTag(11,clordid.c_str());
				}
				emitClientOrderMap[clordid]=PortNo;
			}

			sprintf(reply,"<EmitFixMsgResponse>\r\n"
				"<HTTPCode>200</HTTPCode>\r\n"
				"<Message>OK</Message>\r\n"
				"<ClOrdID>%s</ClOrdID>\r\n"
				"<TimeCompleted>%08d-%02d:%02d:%02d</TimeCompleted>\r\n"
				"</EmitFixMsgResponse>\r\n",
				clordid.c_str(),WSDate,WSTime/10000,(WSTime%10000)/100,WSTime%100);
			WSUsrSendBuff(strlen(reply),reply,PortNo,1);
			pdc->SendFixMsg(ifx);
		}
		else
		{
			sprintf(reply,"<EmitFixMsgResponse>\r\n"
				"<HTTPCode>503</HTTPCode>\r\n"
				"<Message>Service Unavailable</Message>\r\n"
				"<TimeCompleted>%08d-%02d:%02d:%02d</TimeCompleted>\r\n"
				"</EmitFixMsgResponse>\r\n",
				WSDate,WSTime/10000,(WSTime%10000)/100,WSTime%100);
			WSUsrSendBuff(strlen(reply),reply,PortNo,1);
		}
	}
	else
	{
		sprintf(reply,"<EmitFixMsgResponse>\r\n"
			"<HTTPCode>400</HTTPCode>\r\n"
			"<Message>Bad Request</Message>\r\n"
			"<TimeCompleted>%08d-%02d:%02d:%02d</TimeCompleted>\r\n"
			"</EmitFixMsgResponse>\r\n",
			WSDate,WSTime/10000,(WSTime%10000)/100,WSTime%100);
		WSUsrSendBuff(strlen(reply),reply,PortNo,1);
	}
}

int ViewServer::TranslateEmitResponse(FIXINFO& ifx, char *response)
{
	char *rptr=response;
	int ntags=0;
	if(emitTagReverseMap.empty())
		LoadEmitFIXMapping();
	sprintf(rptr,"<EmitFixExecutionReport>\r\n"); rptr+=strlen(rptr);
	for(int i=0;i<ifx.ntags;i++)
	{
		int tag=ifx.tags[i].no;
		map<int,string>::iterator eit=emitTagReverseMap.find(tag);
		if(eit!=emitTagReverseMap.end())
		{
			char vstr[128]={0};
			strncpy(vstr,ifx.tags[i].val,ifx.tags[i].vlen);
			vstr[ifx.tags[i].vlen]=0;
			sprintf(rptr,"<%s>%s</%s>\r\n",eit->second.c_str(),vstr,eit->second.c_str()); rptr+=strlen(rptr); ntags++;
		}
	}
	sprintf(rptr,"</EmitFixExecutionReport>\r\n"); rptr+=strlen(rptr);
	return ntags;
}

int ViewServer::ReturnEmitReponse(FIXINFO& ifx)
{
	string clordid=ifx.TagStr(11);
	map<string,int>::iterator cit=emitClientOrderMap.find(clordid);
	if(cit!=emitClientOrderMap.end())
	{
		int UsrPortNo=cit->second;
		if((UsrPortNo>=0)&&(UsrPortNo<NO_OF_USR_PORTS)&&(UsrPort[UsrPortNo].SockActive))
		{
			char response[4096]={0};
			TranslateEmitResponse(ifx,response);
			WSUsrSendBuff(strlen(response),response,UsrPortNo,1);
		}
	}
	return 0;
}
