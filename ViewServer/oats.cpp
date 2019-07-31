
#include "stdafx.h"
#include "oats.h"

// To provide upcall interface from OATS engine DLL
class OatsOrderImpl
	:public OatsOrder
{
public:
	OatsOrderImpl()
		:reported(false)
		,pdb(0)
		,porder(0)
		,details()
		,children()
	{
	}
	OatsOrderImpl(OrderDB *pdb, VSOrder *porder)
		:reported(false)
		,details()
		,children()
	{
		this->pdb=pdb;
		this->porder=porder;
	}
	~OatsOrderImpl()
	{
		for(list<OatsOrderImpl *>::iterator cit=children.begin();cit!=children.end();cit++)
			delete *cit;
		children.clear();
		for(list<FIXINFO *>::iterator dit=details.begin();dit!=details.end();dit++)
			delete *dit;
		details.clear();
		if(pdb)
		{
			if(porder)
			{
				pdb->FreeOrder(porder); porder=0;
			}
			pdb=0;
		}
	}

	// Upcalls
	inline const char *GetRootOrderID()
	{
		return porder->GetRootOrderID();
	}
	inline const char *GetClOrdID()
	{
		return porder->GetClOrdID();
	}
	inline const char *GetEcnOrderID()
	{
		return porder->GetEcnOrderID();
	}
	inline const char *GetSymbol()
	{
		return porder->GetSymbol();
	}
	inline int GetFillQty()
	{
		return porder->GetFillQty();
	}
	inline LONGLONG GetTransactTime()
	{
		return porder->GetTransactTime();
	}

	// Methods
	static char GetDetailTier(char MsgType, char ExecType);
	static LONGLONG GetTimeStamp(const char *tstr);
	int SortDetails();
	int SortChildren();

protected:
	friend class ViewServer;
	bool reported;
	list<FIXINFO *> details;
	list<OatsOrderImpl *> children;
	OrderDB *pdb;
	VSOrder *porder;
};
char OatsOrderImpl::GetDetailTier(char MsgType, char ExecType)
{
	char dtier=0;
	switch(MsgType)
	{
	case 'D': dtier=1;	
	case 'G': dtier=4;
	case 'F': dtier=6;
	case '8':
	{
		switch(ExecType)
		{
		case '0': dtier=2; break;
		case '1': dtier=3; break;
		case '3': 
		case '4': 
		case '5': 
		case '8': 
		case '9': dtier=5; break;
		case '2': dtier=7; break;
		};
		break;
	}
	};
	return dtier;
}
LONGLONG OatsOrderImpl::GetTimeStamp(const char *tstr)
{
	LONGLONG tsv=0;
	int wsdate=0,hh=0,mm=0,ss=0;
	sscanf(tstr,"%08d-%02d:%02d:%02d",&wsdate,&hh,&mm,&ss);
	tsv=wsdate; tsv*=1000000;
	tsv+=(hh*10000)+(mm*100)+(ss);
	return tsv;
}
static int __cdecl _SortDups(const void *e1, const void *e2)
{
	FIXINFO *f1=*(FIXINFO**)e1,*f2=*(FIXINFO**)e2;
	return f1->llen -f2->llen;
}
static int __cdecl _SortDetails(const void *e1, const void *e2)
{
	// Sort by detail tier first
	FIXINFO *f1=*(FIXINFO**)e1,*f2=*(FIXINFO**)e2;
	char etype1=f1->TagChar(150);
	if((!etype1)||(etype1=='F'))
		etype1=f1->TagChar(39);
	char dt1=OatsOrderImpl::GetDetailTier(f1->TagChar(35),etype1);
	char etype2=f2->TagChar(150);
	if((!etype2)||(etype2=='F'))
		etype2=f2->TagChar(39);
	char dt2=OatsOrderImpl::GetDetailTier(f2->TagChar(35),etype2);
	if(dt1!=dt2)
		return dt1 -dt2;
	// Then sort by timestamp
	LONGLONG cts1=OatsOrderImpl::GetTimeStamp(f1->TagStr(60));
	LONGLONG cts2=OatsOrderImpl::GetTimeStamp(f2->TagStr(60));
	if(cts1<cts2)
		return -1;
	else if(cts1>cts2)
		return +1;
	return 0;
}
int OatsOrderImpl::SortDetails()
{
	int ndets=(int)details.size();
	if(ndets>1)
	{
		FIXINFO **darray=new FIXINFO *[ndets];
		memset(darray,0,ndets*sizeof(FIXINFO *));
		FIXINFO **earray=new FIXINFO *[ndets];
		memset(earray,0,ndets*sizeof(FIXINFO *));
		int didx=0;
		for(list<FIXINFO *>::iterator dit=details.begin();dit!=details.end();dit++)
		{
			FIXINFO *pfix=*dit;
			darray[didx++]=pfix;
		}
		// Remove duplicates
		qsort(darray,didx,sizeof(FIXINFO*),_SortDups);
		int prev=-1,eidx=0;
		for(int i=0;i<didx;i++)
		{
			if((prev>=0)&&
			   (darray[i]->llen==darray[prev]->llen)&&
			   (!strncmp(darray[i]->fbuf,darray[prev]->fbuf,darray[i]->llen)))
			{
				darray[i]=0;
				continue;
			}
			earray[eidx++]=darray[i];
			prev=i;
		}
		// Sort by tier, then time
		qsort(earray,eidx,sizeof(FIXINFO*),_SortDetails);
		details.clear();
		for(int i=0;i<eidx;i++)
			details.push_back(earray[i]);
		delete earray;
		delete darray;
	}
	return 0;
}
static int __cdecl _SortChildren(const void *e1, const void *e2)
{
	// Sort by detail tier first
	OatsOrderImpl *c1=*(OatsOrderImpl**)e1,*c2=*(OatsOrderImpl**)e2;
	LONGLONG cts1=c1->GetTransactTime();
	LONGLONG cts2=c2->GetTransactTime();
	if(cts1<cts2)
		return -1;
	else if(cts1>cts2)
		return +1;
	return 0;
}
int OatsOrderImpl::SortChildren()
{
	// Put children in TransactTime order
	int nkids=(int)children.size();
	if(nkids>1)
	{
		OatsOrderImpl **carray=new OatsOrderImpl *[nkids];
		memset(carray,0,nkids*sizeof(OatsOrderImpl *));
		int cidx=0;
		for(list<OatsOrderImpl *>::iterator cit=children.begin();cit!=children.end();cit++)
		{
			OatsOrderImpl *child=*cit;
			carray[cidx++]=child;
		}
		qsort(carray,cidx,sizeof(OatsOrderImpl *),_SortChildren);
		children.clear();
		for(int i=0;i<cidx;i++)
			children.push_back(carray[i]);
		delete carray;
	}
	return 0;
}

int ViewServer::ReportOats(FILE *fp, const char *dllpath, const char *AppInstID)
{
	// Dynamic OATS engine loaded from DLL
	HMODULE dhnd=LoadLibrary(dllpath);
	if(!dhnd)
		return -1;
	GETOATSENGINE pGetOatsEngine=(GETOATSENGINE)GetProcAddress(dhnd,GET_OATS_EXPORT_FUNC);
	FREEOATSENGINE pFreeOatsEngine=(FREEOATSENGINE)GetProcAddress(dhnd,FREE_OATS_EXPORT_FUNC);
	if((!pGetOatsEngine)||(!pFreeOatsEngine))
		goto error;
	// Create an engine instance
	OatsEngine *poats=pGetOatsEngine();
	if(!poats)
		goto error;
	// Initialize
	if(poats->StartEngine(fp,WSDate,AppInstID)<0)
		goto error;

	{//scope
	APPSYSMAP::iterator ait=asmap.find(AppInstID);
	if(ait==asmap.end())
		goto error;
	AppSystem *asys=ait->second;
	OrderDB *pdb=&asys->odb;
	// Walk all parent keys
	string lroid;
	for(VSINDEX::m_iterator pit=pdb->pmap.m_begin();pit!=pdb->pmap.m_end();pit++)
	{
		if(pit.first==lroid)
			continue;
		lroid=pit.first;
		VSOrder *parent=0;
		// Try to find the parent order
		VSINDEX::iterator poit=pdb->omap.find(lroid);
		if(poit!=pdb->omap.end())
			parent=pdb->ReadOrder(poit.second);
		OatsOrderImpl *oparent=0;
		if(parent)
		{
			parent=pdb->ReadOrder(pit.second);
			if(!parent)
				continue;
			// One specific app instance
			if(!strrcmp(AppInstID,"*"))
			{
				if(stricmp(AppInstID,parent->GetAppInstID()))
				{
					pdb->FreeOrder(parent);
					continue;
				}
			}
			//fprintf(fp,"DEBUG parent %s,%s,%X\n",parent->GetAppInstID(),parent->GetClOrdID(),parent->GetOffset());
			// Create OATS parent order
			oparent=new OatsOrderImpl(pdb,parent);
			// Parent details
			for(int didx=0;didx<parent->GetNumDetails();didx++)
			{
				unsigned short UscPortNo=-1;
				unsigned short dlen=0;
				LONGLONG doff=parent->GetDetail(didx,UscPortNo,dlen);
				if((dlen<0)||(dlen>4096)) // sanity limit
					continue;
				if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
					continue;
				VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
				if(!dfile)
				{
					// TODO: This has potential for filling up the log file
					//WSLogError("USC%d: Missing detail file!",UscPortNo);
					continue;
				}
				int proto=dfile->proto;
				char *dptr=new char[dlen+1];
				memset(dptr,0,dlen+1);
				OrderDB *pdb=&asys->odb;
				if(pdb->ReadDetail(dfile,doff,dptr,dlen)<0)
				{
					delete dptr;
					continue;
				}
				FIXINFO *pfix=new FIXINFO;
				pfix->Reset();
				pfix->sbuf=dptr;
				pfix->slen=dlen;
				pfix->FIXDELIM=(dfile->proto==PROTO_MLFIXLOGS)?';':0x01;
				pfix->noSession=true;
				pfix->supressChkErr=true;
				if(pfix->FixMsgReady(dptr,dlen)<1)
				{
					delete dptr;
					continue;
				}
				oparent->details.push_back(pfix);
			}
			oparent->SortDetails();
			// Report parent details
			FIXINFO *ufix=0;
			for(list<FIXINFO*>::iterator pdit=oparent->details.begin();pdit!=oparent->details.end();pdit++)
			{
				FIXINFO *pfix=*pdit;
				if(!ufix) ufix=pfix;
				char mtype=pfix->TagChar(35);
				switch(mtype)
				{
				case '8': // execution report
				{
					char etype=pfix->TagChar(150);
					if((!etype)||(etype=='F'))
						etype=pfix->TagChar(39);
					switch(etype)
					{
					case '0': // confirmed
					{
						const char *RootOrderID=pfix->TagStr(70129);
						if(stricmp(RootOrderID,pfix->TagStr(11))!=0)
						{
							_ASSERT(false);
							fprintf(fp,"DEBUG 70129=%s,11=%s\n",RootOrderID,pfix->TagStr(11));
						}
						poats->ParentOrderCreated(oparent,pfix);
						oparent->reported=true;
						break;
					}
					case '5': // replaced
					{
						poats->ParentOrderReplaced(oparent,pfix);
						oparent->reported=true;
						break;
					}
					case '4': // cancelled
					{
						// TODO: Must correct tag 41=tag 11 on cancelled report!
						poats->ParentOrderCancelled(oparent,pfix);
						oparent->reported=true;
						break;
					}
					case '2': // filled
					case '3': // done for day
					{
						poats->ParentOrderTerminated(oparent,pfix);
						oparent->reported=true;
						break;
					}
					};
					break;
				}
				};
			}
			// For when we're missing details
			if(!oparent->reported)
			{
				//fprintf(fp,"#OE#,NW,%s,%s,%.2f,%d,%I64d (missing parent, %d details)\n",
				//	parent->GetClOrdID(),parent->GetSymbol(),0.0,parent->GetOrderQty(),parent->GetTransactTime(),oparent->details.size());
				poats->ParentOrderUnreported(oparent,ufix);
				oparent->reported=true;
			}
		}
		else
		{
			oparent=new OatsOrderImpl;
			//fprintf(fp,"#OE#,NW,%s,%s,%.2f,%d,%d (missing parent, %d details)\n",
			//	lroid.c_str(),"",0.0,0,0,0,0);
			poats->ParentOrderUnreported(WSDate,lroid.c_str());
			oparent->reported=true;
		}
		_ASSERT(oparent);

		// Add all children of this parent
		for(VSINDEX::m_iterator cit=pdb->pmap.m_find(lroid);
			(cit!=pdb->pmap.m_end())&&(cit.first==lroid);
			cit++)
		{
			if((parent)&&(cit.second==parent->GetOffset()))
				continue;
			VSOrder *child=pdb->ReadOrder(cit.second);
			if(!child)
				continue;
			// One specific app instance
			if(!strrcmp(AppInstID,"*"))
			{
				if(stricmp(AppInstID,child->GetAppInstID()))
				{
					pdb->FreeOrder(child);
					continue;
				}
			}
			//fprintf(fp,"DEBUG  child %s,%s,%X\n",child->GetAppInstID(),child->GetClOrdID(),child->GetOffset());
			OatsOrderImpl *ochild=new OatsOrderImpl(pdb,child);
			// Child details
			for(int didx=0;didx<child->GetNumDetails();didx++)
			{
				unsigned short UscPortNo=-1;
				unsigned short dlen=0;
				LONGLONG doff=child->GetDetail(didx,UscPortNo,dlen);
				if((dlen<0)||(dlen>4096)) // sanity limit
					continue;
				if((UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
					continue;
				VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
				if(!dfile)
				{
					// TODO: This has potential for filling up the log file
					//WSLogError("USC%d: Missing detail file!",UscPortNo);
					continue;
				}
				int proto=dfile->proto;
				char *dptr=new char[dlen+1];
				memset(dptr,0,dlen+1);
				OrderDB *pdb=&asys->odb;
				if(pdb->ReadDetail(dfile,doff,dptr,dlen)<0)
				{
					delete dptr;
					continue;
				}
				FIXINFO *pfix=new FIXINFO;
				pfix->Reset();
				pfix->sbuf=dptr;
				pfix->slen=dlen;
				pfix->FIXDELIM=(dfile->proto==PROTO_MLFIXLOGS)?';':0x01;
				pfix->noSession=true;
				pfix->supressChkErr=true;
				if(pfix->FixMsgReady(dptr,dlen)<1)
				{
					delete dptr;
					continue;
				}
				ochild->details.push_back(pfix);
			}
			ochild->SortDetails();
			oparent->children.push_back(ochild);
		}
		oparent->SortChildren();
		// Report all children's details
		for(list<OatsOrderImpl*>::iterator cit=oparent->children.begin();cit!=oparent->children.end();cit++)
		{
			OatsOrderImpl *ochild=*cit;
			VSOrder *child=ochild->porder;
			// Walk child details
			FIXINFO *ufix=0;
			for(list<FIXINFO*>::iterator cdit=ochild->details.begin();cdit!=ochild->details.end();cdit++)
			{
				FIXINFO *cfix=*cdit;
				if(!ufix) ufix=cfix;
				char mtype=cfix->TagChar(35);
				switch(mtype)
				{
				case '8': // execution report
				{
					char etype=cfix->TagChar(150);
					if((!etype)||(etype=='F'))
						etype=cfix->TagChar(39);
					switch(etype)
					{
					case '0': // confirmed
					{
						poats->ChildOrderCreated(ochild,oparent,cfix);
						ochild->reported=true;
						break;
					}
					case '5': // replaced
					{
						poats->ChildOrderReplaced(ochild,oparent,cfix);
						ochild->reported=true;
						break;
					}
					case '4': // cancelled
					{
						poats->ChildOrderCancelled(ochild,oparent,cfix);
						ochild->reported=true;
						break;
					}
					case '2': // filled
					case '3': // done for day
					case '8': // rejected
					{
						poats->ChildOrderTerminated(ochild,oparent,cfix);
						ochild->reported=true;
						break;
					}
					};
					break;
				}
				};
			}
			if(!ochild->reported)
			{
				//fprintf(fp,"#OE#,RT,%s,%s,%s,%.2f,%d,%I64d (missing child, %d details)\n",
				//	child->GetRootOrderID(),child->GetClOrdID(),child->GetSymbol(),0.0,child->GetOrderQty(),child->GetTransactTime(),ochild->details.size());
				poats->ChildOrderUnreported(ochild,oparent,ufix);
				ochild->reported=true;
			}
		}
		delete oparent; oparent=0;
	}
	}//scope

	// Shutdown
	poats->StopEngine();
	pFreeOatsEngine(poats);
	FreeLibrary(dhnd);
	return 0;
error:
	FreeLibrary(dhnd);
	return -1;
}
