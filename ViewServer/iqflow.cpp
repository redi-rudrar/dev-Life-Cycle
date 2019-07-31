
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "iqflow.h"

int ViewServer::LoadMatchFile()
{
	const char *fpath="match.txt";
	FILE *rfp=fopen(fpath,"rt");
	if(!rfp)
	{
		WSLogError("Failed opening %s!",fpath);
		return -1;
	}
	list<MatchEntry*> nmatchList;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),rfp))
	{
		char *rptr;
		for(rptr=rbuf;isspace(*rptr);rptr++)
			;
		char *eptr;
		for(eptr=rptr +(int)strlen(rptr) -1;
			(eptr>rbuf)&&((*eptr=='\r')||(*eptr=='\n')||isspace(*eptr));
			eptr--)
			*eptr=0;
		if((!*rptr)||(!strncmp(rptr,"//",2)))
			continue;
		char *key=strtoke(rptr,":");
		if(!*key)
			continue;
		char *val=key +strlen(key) +1;
		MatchEntry *pmatch=new MatchEntry;
		pmatch->key=key;
		int col=0;
		for(const char *tok=strtoke(val,"/");tok;tok=strtoke(0,"/"))
		{
			col++;
			switch(col)
			{
			case 1: pmatch->t2005=tok; break;
			case 2: pmatch->t2007=tok; break;
			case 3: pmatch->accountType=tok; break;
			case 4: pmatch->programTrading=tok; break;
			case 5: pmatch->arbitrage=tok; break;
			case 6: pmatch->memberType=tok; break;
			};
		}
		nmatchList.push_back(pmatch);
	}
	fclose(rfp);
	matchList.clear();
	matchList.swap(nmatchList);
	for(list<MatchEntry*>::iterator oit=nmatchList.begin();oit!=nmatchList.end();oit++)
		delete *oit;
	nmatchList.clear();
	WSLogEvent("Loaded %d match rules from %s",matchList.size(),fpath);
	return 0;
}
MatchEntry *ViewServer::GetMatch(const char *tparent)
{
	if((!tparent)||(!*tparent))
		return 0;
	for(list<MatchEntry*>::iterator mit=matchList.begin();mit!=matchList.end();mit++)
	{
		MatchEntry *pmatch=*mit;
		if(!strncmp(tparent,pmatch->t2007.c_str(),pmatch->t2007.length()))
			return pmatch;
		if(!strncmp(tparent,pmatch->t2005.c_str(),pmatch->t2005.length()))
			return pmatch;
	}
	return 0;
}
