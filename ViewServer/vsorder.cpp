
#include "stdafx.h"
#include "vsdefs.h"
#include "vsorder.h"
#include "bfix.h"
#include "wstring.h"
#include "IQClServer.h" //DT9044
#include <time.h>

#define DBGROWSIZE (8*1024*VSORDER_RECSIZE)
#define DETGROWSIZE (4*1024*1024)

#ifdef MULTI_DAY_HIST
struct DELAYINITARGS
{
	OrderDBNotify *pnotify;
	string dbdir;
	string dbname;
	int wsdate;
	bool readonly;
	AppSystem *asys;
};
#endif

//#pragma pack(push,1)
//#pragma pack(pop)
LONGLONG OrderDB::somem=0;

OrderDB::OrderDB()
	// public items
	:fpath()
	,wsdate(0)
	,readonly(false)
	,ORDER_CACHE_SIZE(0)
	,INDEX_ROOTORDERID(true)
	,INDEX_FIRSTCLORDID(true)
	,INDEX_CLPARENTORDERID(true)
	,INDEX_SYMBOL(true)
	,INDEX_ACCOUNT(true)
	,INDEX_ECNORDERID(true)
	,INDEX_CLIENTID(true)
	,INDEX_TRANSACTTIME(true)
	,INDEX_FILLED_ORDERS(true)
	,INDEX_OPEN_ORDERS(true)
	,INDEX_CONNECTIONS(false)
#ifdef MULTI_DAY
	,INDEX_ORDERDATE(false)
#endif
	,INDEX_AUXKEYS(0) //DT9398
	//DT9491
	,INDEX_CLORDID_CS(false)
	,INDEX_ROOTORDERID_CS(false)
	,INDEX_FIRSTCLORDID_CS(false)
	,INDEX_CLPARENTORDERID_CS(false)
	,INDEX_SYMBOL_CS(false)
	,INDEX_ACCOUNT_CS(false)
	,INDEX_ECNORDERID_CS(false)
	,INDEX_CLIENTID_CS(false)
	,INDEX_CONNECTIONS_CS(false)
	,INDEX_AUXKEYS_CS(false)
	,HAVE_FUSION_IO(false)
	// protected items
	,pnotify(0)
	#ifdef WIN32
	,fhnd(INVALID_FILE_VALUE)	
	#else
	,fhnd(0)	
	#endif
	,oblocks(0)
	,freeOrders(0)
	,noblocks(0)
	,nomem(0)
	,nopend(0)
	,omap()
	,pmap()
	,fmap()
	,symmap()
	,acctmap()
	,omap2()
	,fomap()
	,oomap()
	,cimap()
	,ndblocks(0)
	,ndmem(0)
	,ndpend(0)
	,dfmap()
	,zfp(0)
	,zdirty(false)
	,efp(0)
	,edirty(false)
	,comap()
	,cnmap()
	,cpmap()
	,akmap()
	#ifdef MULTI_DAY_HIST
	,HISTORIC_DAYS(0)
	,delayInitArgs(0)
	#endif
{
	mutex=CreateMutex(0,false,0);
	#ifdef WIN32
	fsize.QuadPart=0;
	fend.QuadPart=0;
	#else
	fsize=0;
	fend=0;
	#endif
	memset(zpath,0,sizeof(zpath));
	memset(epath,0,sizeof(epath));

	//DT9398
	for(int i=0;i<AUX_KEYS_MAX_NUM;i++)
	{
		AUXKEY_NAMES[i].erase();
		AUXKEY_TAGS[i]=0;
	#ifdef AUXKEY_EXPR
		AUXKEY_ORD_EXPR[i].erase();
		AUXKEY_EXEC_EXPR[i].erase();
	#endif
	}
}
OrderDB::~OrderDB()
{
	if(mutex)
	#ifdef _DEBUG
		WaitForSingleObject(mutex,INFINITE);
	#else
		WaitForSingleObject(mutex,1000);
	#endif

	Shutdown();

	if(mutex)
	{
		#ifdef WIN32
		CloseHandle(mutex); mutex=0;
		#else
		DeleteMutex(mutex); mutex=0;
		#endif
	}
	#ifdef MULTI_DAY_HIST
	if(delayInitArgs)
	{
		delete (DELAYINITARGS*)delayInitArgs; delayInitArgs=0;
	}
	#endif
}
char OrderDB::SizeStr_zstrs[8][64]={"","","","","","",""};
int OrderDB::SizeStr_zidx=0;
const char *OrderDB::SizeStr(__int64 bigsize, bool byteUnits)
{
	__int64 kil = byteUnits ?1024 :1000;
    __int64 meg = kil *kil;
    __int64 gig = meg *kil;
    //static __thread int zidx = -1;
    char *zstrs[8]={SizeStr_zstrs[0],SizeStr_zstrs[1],SizeStr_zstrs[2],SizeStr_zstrs[3],SizeStr_zstrs[4],SizeStr_zstrs[5],SizeStr_zstrs[6],SizeStr_zstrs[7]};
    int& zidx=SizeStr_zidx;
    zidx = (zidx +1) %8;
    char *zstr = zstrs[zidx];
    memset(zstr, 0, sizeof(zstr));
    if ( bigsize > gig )
    {
        double frem = (double)(bigsize %gig);
        frem /= (double)gig;
        int irem = (int)(frem *10);
        sprintf(zstr, "%d.%dG", (int)(bigsize /gig), irem);
    }
    else if ( bigsize >= meg )
    {
        double frem = (double)(bigsize %meg);
        frem /= (double)meg;
        int irem = (int)(frem *10);
        sprintf(zstr, "%d.%dM", (int)(bigsize /meg), irem);
    }
    else if ( bigsize >= kil )
    {
        double frem = (double)(bigsize %kil);
        frem /= (double)kil;
        int irem = (int)(frem *10);
        sprintf(zstr, "%d.%dK", (int)(bigsize /kil), irem);
    }
    else
        sprintf(zstr, "%d", bigsize);
    return zstr;
}
int OrderDB::Init(OrderDBNotify *pnotify, const char *dbdir, const char *dbname, int wsdate, bool readonly, bool& cxlInit, AppSystem *asys)
{
	if((!pnotify)||(!dbdir)||(!*dbdir)||(!dbname)||(!*dbname)||(cxlInit))
		return -1;
	char dbpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(dbpath,"%s\\%s",dbdir,dbname);
	#else
	sprintf(dbpath,"%s/%s",dbdir,dbname);
	#endif
	this->pnotify=pnotify;
	this->fpath=dbpath;
	this->wsdate=wsdate;
	this->readonly=readonly;
	char emsg[512]={0};

	// Initialize the order index
#ifdef USE_DISK_INDEX
	char ibpath[MAX_PATH]={0};
	#ifdef WIN32
	sprintf(ibpath,"%s\\%s",dbdir,dbname);
	#else
	sprintf(ibpath,"%s/%s",dbdir,dbname);
	#endif
	char *eptr=strrchr(ibpath,'.');
	if(eptr)
		strcpy(eptr,".obk");
	//char idpath[MAX_PATH]={0};
	//sprintf(idpath,"%s\\%s",dbdir,dbname);
	//eptr=strrchr(idpath,'.');
	//if(eptr)
	//	strcpy(eptr,".oix");
	if(omap.Init(true,32768,2048,1,ibpath,0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(%s)!",ibpath);
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	// Initialize the parent/child index
	if(pmap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(pmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(fmap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(fmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(symmap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(symmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(acctmap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(acctmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(omap2.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(omap2)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(fomap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(fomap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(oomap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(oomap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(cimap.Init(true,32768,2048,1,"",0)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(cimap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(tsmap.Init(true)<0)
	{
		sprintf(emsg,"Failed DiskIndex::Init(tsmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
#else//!USE_DISK_INDEX
	if(omap.Init(false)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init()!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(pmap.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(pmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(fmap.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(fmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(symmap.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(symmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(acctmap.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(acctmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(omap2.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(omap2)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(fomap.Init(false)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(fomap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(oomap.Init(false)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(oomap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(cimap.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(cimap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(tsmap.Init(true)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(tsmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(cnmap.Init(false)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(cnmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	if(cpmap.Init(false)<0)
	{
		sprintf(emsg,"Failed MemIndex::Init(cpmap)!");
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	for(unsigned int i=0;i<INDEX_AUXKEYS; i++) //DT9398
	{
		if(akmap[i].Init(false)<0)
		{
			sprintf(emsg,"Failed MemIndex::Init(akmap)!");
			pnotify->ODBError(-1,emsg);
			return -1;
		}
	}
#endif//!USE_DISK_INDEX

#ifdef USE_TREE_INDEX
	// Pre-load tree data
	FILE *fp=fopen("orderids.txt","rt");
	if(fp)
	{
		char rbuf[1024]={0};
		int lno=0;
		while(fgets(rbuf,sizeof(rbuf),fp))
		{
			lno++;
			if(cxlInit)
				return -1;
			for(char *rptr=rbuf;isspace(*rptr);rptr++)
				;
			if((!*rptr)||(*rptr=='\r')||(*rptr=='\n')||(!strncmp(rptr,"//",2)))
				continue;
			const char *RootOrderID=strtoke(rptr,",\r\n");
			if(!RootOrderID)
				continue;
			const char *ClOrdID=strtoke(0,",\r\n");
			if(!ClOrdID)
				continue;
			omap.pre_insert(ClOrdID);
			if(INDEX_ROOTORDERID)
				pmap.pre_insert(RootOrderID);
		}
		fclose(fp);
	}
	omap.pre_close();
	if(INDEX_ROOTORDERID)
		pmap.pre_close();
#endif

	// Open the orders data file
#ifdef WIN32
	if(readonly)
		fhnd=CreateFile(dbpath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
	else
		fhnd=CreateFile(dbpath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_ALWAYS,0,0);
	if(fhnd==INVALID_FILE_VALUE)
	{
		sprintf(emsg,"Failed opening %s!",dbpath);
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	fsize.LowPart=GetFileSize(fhnd,(LPDWORD)&fsize.HighPart);
	sprintf(emsg,"Loading %s (%s bytes)...",dbpath,SizeStr(fsize.QuadPart,true));
	pnotify->ODBEvent(0,emsg);
//#ifndef USE_DISK_INDEX
	// Scan existing records when we don't have a disk-based index
	DWORD bcnt=0;
	OVERLAPPED ovl;
	memset(&ovl,0,sizeof(OVERLAPPED));
	for(fend.QuadPart=0;fend.QuadPart<fsize.QuadPart;)
	{
		if(cxlInit)
		{
			fend.QuadPart=fsize.QuadPart;
			return -1;
		}
		VSRECHEAD header;
		memset(&header,0,sizeof(VSRECHEAD));
		SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
		DWORD rbytes=0;
		if(!ReadFile(fhnd,&header,sizeof(VSRECHEAD),&rbytes,0)||(rbytes!=sizeof(VSRECHEAD)))
		{
			#ifdef WIN32
			sprintf(emsg,"Failed reading record header at offset %I64d!",fend.QuadPart);
			#else
			sprintf(emsg,"Failed reading record header at offset %lld!",fend.QuadPart);
			#endif
			pnotify->ODBError(-1,emsg);
			break;
		}
		// Blank record
		if(header.marker==0)
			break;
		// Corrupted record
		else if(header.marker!=0xFDFDFDFD)
		{
			#ifdef WIN32
			sprintf(emsg,"Invalid order marker(%X) at offset %I64d!",header.marker,fend.QuadPart);
			#else
			sprintf(emsg,"Invalid order marker(%X) at offset %lld!",header.marker,fend.QuadPart);
			#endif
			pnotify->ODBError(-1,emsg);
			break;
		}
		// Real order
	#ifdef MULTI_RECTYPES
		if((header.rtype==0x01)||(header.rtype==0x04)||(header.rtype==0x05))
	#else
		if(header.rtype==0x01)
	#endif
		{
			VSOrder *porder=ReadOrder(fend.QuadPart);
			if(!porder)
			{
				_ASSERT(false);
				#ifdef WIN32
				sprintf(emsg,"Failed reading VSOrder at offset %I64d!",fend.QuadPart);
				#else
				sprintf(emsg,"Failed reading VSOrder at offset %lld!",fend.QuadPart);
				#endif
				pnotify->ODBError(-1,emsg);
				return -1;
			}
			int rlen=porder->rlen;

			//DT9044: Reload the AccountRecs upon restart
			if(!strncmp(porder->GetClOrdID(),VSDB_ACCOUNTREC_IND,strlen(VSDB_ACCOUNTREC_IND)))
			{
				ViewServer* vs=(ViewServer*)pnotify;
				MSG632REC Msg632Rec;
				memset(&Msg632Rec,0,sizeof(MSG632REC));
				
				int Conn=atol(porder->GetConnection());
				LONGLONG doff=_atoi64(porder->GetAuxKey());  //DT9398
				VSDetailFile *dfile=(VSDetailFile*)vs->UscPort[Conn].DetPtr1;
			#ifdef MULTI_DAY_HIST
				if(HISTORIC_DAYS>0)
					;
				else
			#endif
				if(!dfile)
				{
					sprintf(emsg,"Error reloading IQ AccountRec(%s), please set IQCLFLOW!",porder->GetClOrdID());
					pnotify->ODBError(-1,emsg);
				}
				else if(ReadDetail(dfile,doff+28,(char*)&Msg632Rec.AccountRec,sizeof(ACCOUNTREC))>0)
				{
					if(Msg632Rec.AccountRec.Account[0])
					{
						_strupr(Msg632Rec.AccountRec.Domain);
						_strupr(Msg632Rec.AccountRec.Account);
						Account* acc=vs->CreateDnsAccount(-1,Msg632Rec,asys);
						if(!acc)
						{
							#ifdef WIN32
							sprintf(emsg,"Error re-creating account at offset %I64d!",fend.QuadPart);
							#else
							sprintf(emsg,"Error re-creating account at offset %lld!",fend.QuadPart);
							#endif
							pnotify->ODBError(-1,emsg);
						}
					}
				}
			}
			else
			{
			#ifdef MULTI_DAY_HIST
				pnotify->ODBIndexOrder(asys,this,porder,fend.QuadPart);
			#else
				pnotify->ODBIndexOrder(asys,porder,fend.QuadPart);
			#endif
			}
			fend.QuadPart+=rlen;
		}
		// Detail extension
		else if(header.rtype==0x03)
		{
			// Skip because it's read with the header already needed when order is accessed.
			fend.QuadPart+=header.rlen;
		}
		// We don't know about it
		else
		{
			_ASSERT(false);
			#ifdef WIN32
			sprintf(emsg,"Invalid record type(%d) at offset %I64d!",header.rtype,fend.QuadPart);
			#else
			sprintf(emsg,"Invalid record type(%d) at offset %lld!",header.rtype,fend.QuadPart);
			#endif
			pnotify->ODBError(-1,emsg);
			fend.QuadPart+=header.rlen;
		}
		if((++bcnt)%1000==0)
		{
			int lperc=(int)(fend.QuadPart*100/(fsize.QuadPart?fsize.QuadPart:1));
			pnotify->ODBBusyLoad(fend.QuadPart,lperc);
		}
	}
	pnotify->ODBBusyLoad(fend.QuadPart,100);
//#endif//!USE_DISK_INDEX
	if(!readonly)
	{
		// Pre-grow in 8MB chunks
		for(;fsize.QuadPart<=fend.QuadPart;fsize.QuadPart+=DBGROWSIZE)
			;
		GrowFile(fsize.QuadPart);
	}
#else//!WIN32
	if(readonly)
		fhnd=fopen(dbpath,"rb");
	else
	{
		fhnd=fopen(dbpath,"r+b");
		if(!fhnd)
			fhnd=fopen(dbpath,"w+b");
	}
	if(!fhnd)
	{
		sprintf(emsg,"Failed opening %s!",dbpath);
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	fseeko(fhnd,0,SEEK_END);
	fsize=ftello(fhnd);
	fseeko(fhnd,0,SEEK_SET);
	sprintf(emsg,"Loading %s (%s bytes)...",dbpath,SizeStr(fsize,true));
	pnotify->ODBEvent(0,emsg);
	// TODO: Use Linux LFS API (large file support)
	// Scan existing records when we don't have a disk-based index
	DWORD bcnt=0;
	for(fend=0;fend<fsize;)
	{
		if(cxlInit)
		{
			fend=fsize;
			return -1;
		}
		VSRECHEAD header;
		memset(&header,0,sizeof(VSRECHEAD));
		fseeko(fhnd,fend,SEEK_SET);
		if(fread(&header,1,sizeof(VSRECHEAD),fhnd)!=sizeof(VSRECHEAD))
		{
			#ifdef WIN32
			sprintf(emsg,"Failed reading record header at offset %I64d!",fend);
			#else
			sprintf(emsg,"Failed reading record header at offset %lld!",fend);
			#endif
			pnotify->ODBError(-1,emsg);
			break;
		}
		// Blank record
		if(header.marker==0)
			break;
		// Corrupted record
		else if(header.marker!=0xFDFDFDFD)
		{
			#ifdef WIN32
			sprintf(emsg,"Invalid order marker(%X) at offset %I64d!",header.marker,fend);
			#else
			sprintf(emsg,"Invalid order marker(%X) at offset %lld!",header.marker,fend);
			#endif
			pnotify->ODBError(-1,emsg);
			break;
		}
		// Real order
		if(header.rtype==0x01)
		{
			VSOrder *porder=ReadOrder(fend);
			if(!porder)
			{
				_ASSERT(false);
				#ifdef WIN32
				sprintf(emsg,"Failed reading VSOrder at offset %I64d!",fend);
				#else
				sprintf(emsg,"Failed reading VSOrder at offset %lld!",fend);
				#endif
				pnotify->ODBError(-1,emsg);
				return -1;
			}
			int rlen=porder->rlen;
			pnotify->ODBIndexOrder(asys,porder,fend);
			fend+=rlen;
		}
		// Detail extension
		else if(header.rtype==0x03)
		{
			// Skip because it's read with the header already needed when order is accessed.
			fend+=header.rlen;
		}
		// We don't know about it
		else
		{
			_ASSERT(false);
			#ifdef WIN32
			sprintf(emsg,"Invalid record type(%d) at offset %I64d!",header.rtype,fend);
			#else
			sprintf(emsg,"Invalid record type(%d) at offset %lld!",header.rtype,fend);
			#endif
			pnotify->ODBError(-1,emsg);
			fend+=header.rlen;
		}
		if((++bcnt)%1000==0)
		{
			int lperc=(int)(fend*100/(fsize?fsize:1));
			pnotify->ODBBusyLoad(fend,lperc);
		}
	}
	pnotify->ODBBusyLoad(fend,100);
	if(!readonly)
	{
		// Pre-grow in 8MB chunks
		for(;fsize<=fend;fsize+=DBGROWSIZE)
			;
		GrowFile(fsize);
	}
#endif//!WIN32
	return 0;
}
int OrderDB::DelayInit(OrderDBNotify *pnotify, const char *dbdir, const char *dbname, int wsdate, bool readonly, bool& cxlInit, AppSystem *asys)
{
	DELAYINITARGS *dia=new DELAYINITARGS;
	dia->pnotify=pnotify;
	dia->dbdir=dbdir;
	dia->dbname=dbname;
	dia->wsdate=wsdate;
	dia->readonly=readonly;
	dia->asys=asys;
	delayInitArgs=dia;
	this->wsdate=wsdate;
	return 0;
}
int OrderDB::DoDelayInit()
{
	DELAYINITARGS *dia=(DELAYINITARGS *)delayInitArgs; delayInitArgs=0;
	if(!dia)
		return -1;
	bool cxlInit=false;
	int rc=Init(dia->pnotify,dia->dbdir.c_str(),dia->dbname.c_str(),dia->wsdate,dia->readonly,cxlInit,dia->asys);
	delete dia;
	return rc;
}
void OrderDB::Shutdown()
{
	Lock();
	char emsg[256]={0};
	#ifdef WIN32
	sprintf(emsg,"Database shutdown (%s bytes)",SizeStr(fend.QuadPart,true));
	#else
	sprintf(emsg,"Database shutdown (%s bytes)",SizeStr(fend,true));
	#endif
	if(pnotify)
		pnotify->ODBEvent(0,emsg);
	for(unsigned int i=0;i<INDEX_AUXKEYS;i++) //DT9398
		akmap[i].Cleanup();
	cpmap.Cleanup();
	cnmap.Cleanup();
	oomap.Cleanup();
	fomap.Cleanup();
	tsmap.Cleanup();
	cimap.Cleanup();
	omap2.Cleanup();
	acctmap.Cleanup();
	symmap.Cleanup();
	fmap.Cleanup();
	pmap.Cleanup();
	omap.Cleanup();
	#ifdef _DEBUG
	// This can take a long time, and there's really no reason to do it in release build if the app is exiting
	// Clean up allocated orders
	for(VSORDERBLOCK *fblock=oblocks;fblock;fblock=fblock->next)
	{
		for(int i=0;i<sizeof(fblock->orders)/sizeof(VSOrder);i++)
		{
			VSOrder *porder=&fblock->orders[i];
			if(porder->offset>=0)
				FreeOrder(porder);
		}
	}
	#endif
	// Clean up order blocks
	freeOrders=0;
	while(oblocks)
	{
		VSORDERBLOCK *dblock=oblocks;
		oblocks=oblocks->next;
		delete dblock;
	}
	noblocks=0;
	nopend=0;
	ndblocks=0;
	ndpend=0;
	#ifdef WIN32
	if(fhnd!=INVALID_FILE_VALUE)
	{
		// Close the data file and truncate to used data only
		SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
		SetEndOfFile(fhnd);
		CloseHandle(fhnd); fhnd=INVALID_FILE_VALUE;
	}
	fend.QuadPart=fsize.QuadPart=0;
	#else
	if(fhnd)
	{
		//ftruncate(fhnd->_file,fend);
		fclose(fhnd); fhnd=0;
	}
	fend=0;
	#endif
	fpath=""; wsdate=0; readonly=false;
	pnotify=0;
	comap.clear();
	Unlock();
}

VSOrder *OrderDB::AllocOrder()
{
	Lock();
	if(!freeOrders)
	{
		// If cache limit reached, then free an old order we haven't used in a while
		// With fusion I/O, orders are only cached when really in use.
		bool newblock=true;
		if((!HAVE_FUSION_IO)&&(ORDER_CACHE_SIZE>0)&&(somem>=((LONGLONG)ORDER_CACHE_SIZE*1024*1024)))
		{
			// Since the database allocates orders in blocks, don't unload if
			// mem limit reached but free orders available.
			ITEMLOC dloc=-1;
			VSOrder *dorder=GetLRUOrder(dloc);
			if(dorder)
			{
			#ifdef NOMAP_TERM_ORDERS
				VSINDEX::iterator doit=omap.find(dorder->GetOrderKey());
				if(doit!=omap.end())
					omap.erase(doit);
			#endif
				FreeOrder(dorder); dorder=0;
				newblock=false;
			}
			else
			{
				// TODO: We need to find an old order to unload from all app systems, not just this one.
				// Allow exceeding the order limit for now.
				newblock=true;
			}
		}
		if(newblock)
		{
			// Allocate orders in blocks
			VSORDERBLOCK *nblock=new VSORDERBLOCK; noblocks++;
			if(!nblock)
			{
				pnotify->ODBError(-1,"Failed allocating VSORDERBLOCK!");
				return 0;
			}
			nblock->next=oblocks; oblocks=nblock;
			somem+=sizeof(VSORDERBLOCK);
			// Put all new orders in a free list for fast reuse
			for(int i=sizeof(nblock->orders)/sizeof(VSOrder) -1;i>=0;i--)
			{
				VSOrder *porder=&nblock->orders[i];
				porder->freeNext=freeOrders; freeOrders=porder;
			}
		}
	}
	// Take the first free one
	VSOrder *porder=freeOrders;
	freeOrders=freeOrders->freeNext;
	porder->freeNext=0;
	nomem++;
	Unlock();
	return porder;
}
void OrderDB::FreeOrder(VSOrder *porder)
{
	if(!porder)
		return;
	if(pnotify)
		pnotify->ODBOrderUnloaded(porder,porder->offset);
	Lock();
	// Check that orders have been committed or never used
	_ASSERT((porder->offset>=0)||(!porder->dcnt));
#ifndef TASK_THREAD_POOL
	if(!HAVE_FUSION_IO)
	{
		CACHEDORDERMAP::iterator cit=comap.find(porder->offset);
		if(cit!=comap.end())
			comap.erase(cit);
	}
#endif
	if(nomem>0) 
		nomem--;
	while(porder->deFirst)
	{
		VSDetExt *pde=porder->deFirst;
		porder->deFirst=porder->deFirst->nextDetExt;
		FreeDetExt(pde);
	}
	porder->deLast=0;
	porder->Reset();
	porder->freeNext=freeOrders; freeOrders=porder;
	Unlock();
}
//void OrderDB::FreeOrders(VSOrder*& pendWritesFirst, VSOrder*& pendWritesLast)
//{
//	while(pendWritesFirst)
//	{
//		VSOrder *porder=pendWritesFirst;
//		pendWritesFirst=pendWritesFirst->pendNext;
//		FreeOrder(porder);
//		if(nopend>0) nopend--;
//	}
//	pendWritesFirst=pendWritesLast=0;
//}
VSOrder *OrderDB::GetLRUOrder(ITEMLOC& oloc)
{
	if(freeOrders)
		return 0;
	if(!oblocks)
		return 0;
	// Last block is oldest
	VSOrder *porder=0;
	VSORDERBLOCK *pblock;
	for(pblock=oblocks;(pblock)&&(pblock->next);pblock=pblock->next)
		;
	// First used order from the top
	for(int i=0;i<sizeof(pblock->orders)/sizeof(VSOrder);i++)
	{
		if(!pblock->orders[i].freeNext)
		{
			porder=&pblock->orders[i];
			VSINDEX::iterator oit=omap.find(porder->GetOrderKey());
			if(oit!=omap.end())
			{
				oloc=oit.second;
				break;
			}
		}
	}
	return porder;
}
int OrderDB::GrowFile(LONGLONG minSize)
{
	if(minSize<0)
		return -1;
	Lock();
	if(fhnd==INVALID_FILE_VALUE)
	{
		Unlock();
		return -1;
	}
#ifdef WIN32
	LARGE_INTEGER nsize;
	for(nsize.QuadPart=0;nsize.QuadPart<minSize;nsize.QuadPart+=DBGROWSIZE)
		;
	SetFilePointer(fhnd,nsize.LowPart,(PLONG)&nsize.HighPart,FILE_BEGIN);
	SetEndOfFile(fhnd);
	fsize.QuadPart=nsize.QuadPart;
#else
	LONGLONG nsize;
	for(nsize=0;nsize<minSize;nsize+=DBGROWSIZE)
		;
	//ftruncate(fhnd->_file,nsize);
	fsize=nsize;
#endif
	Unlock();
	return 0;
}
// There's no need to do overlapped I/O on the order if we've already committed the details
int OrderDB::WriteOrder(VSOrder *porder, ITEMLOC& offset)
{
	if(!porder)
		return -1;
	Lock();
	if(fhnd==INVALID_FILE_VALUE)
	{
		Unlock();
		return -1;
	}
#ifdef WIN32
	#ifdef MULTI_DAY_ITEMLOC
	offset.wsdate=wsdate;
	#endif
	if((LONGLONG)offset<0)
	{
	#ifdef NO_DB_WRITES
		porder->offset=offset=0;
		Unlock();
		return 0;
	#endif
		offset=fend.QuadPart; fend.QuadPart+=VSORDER_RECSIZE;
		if(fend.QuadPart>fsize.QuadPart)
		{
			// Growing the data file by large blocks increases write throughput
			if(GrowFile(fend.QuadPart)<0)
			{
				char emsg[256]={0};
				sprintf(emsg,"Failed growing data file to %s bytes!",SizeStr(fend.QuadPart,true));
				pnotify->ODBError(-1,emsg);
				Unlock();
				return 0;
			}
		}
	}
	// Also write all detail extension index blocks
	for(VSDetExt *pde=porder->deFirst;pde;pde=pde->nextDetExt)
	{
		if(pde->dirty)
		{
			_ASSERT(porder->offset>=0);
			LONGLONG eoff=pde->offset;
			WriteDetExt(pde,eoff);
		}
	}

	LARGE_INTEGER loff;
	loff.QuadPart=offset;
#ifdef VALIDATE_DBWRITES
	// To prevent corruption, read the location before we write 
	// to make sure we're not overwriting another record
	{
		char emsg[256]={0};
		SetFilePointer(fhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
		DWORD rbytes=0;
		VSOrder rorder;
		int rc=ReadFile(fhnd,&rorder.marker,VSORDER_RECSIZE,&rbytes,0);
		int lerr=GetLastError();
		if((rc<0)||(rbytes!=VSORDER_RECSIZE))
		{
			#ifdef WIN32
			sprintf(emsg,"Failed read to validate order(%s) at offset %I64d!",porder->OrderKey,loff.QuadPart);
			#else
			sprintf(emsg,"Failed read to validate order(%s) at offset %lld!",porder->OrderKey,loff.QuadPart);
			#endif
			pnotify->ODBError(-1,emsg);
			Unlock();
			return -1;
		}
		// New order validation
		if(porder->offset<0)
		{
			if((rorder.AppInstID[0])||(rorder.ClOrdID[0]))
			{
				#ifdef WIN32
				sprintf(emsg,"Failed write on order(%s) at offset %I64d--order(%s/%s) at that location!",
				#else
				sprintf(emsg,"Failed write on order(%s) at offset %lld--order(%s/%s) at that location!",
				#endif
					porder->OrderKey,loff.QuadPart,rorder.AppInstID,rorder.ClOrdID);
				pnotify->ODBError(-1,emsg);
				Unlock();
				return -1;
			}
		}
		// Update order validation
		else
		{
			if((_stricmp(rorder.AppInstID,porder->AppInstID))||_stricmp(rorder.ClOrdID,porder->ClOrdID))
			{
				#ifdef WIN32
				sprintf(emsg,"Failed update on order(%s) at offset %I64d--order(%s/%s) at that location!",
				#else
				sprintf(emsg,"Failed update on order(%s) at offset %lld--order(%s/%s) at that location!",
				#endif
					porder->OrderKey,loff.QuadPart,rorder.AppInstID,rorder.ClOrdID);
				pnotify->ODBError(-1,emsg);
				Unlock();
				return -1;
			}
		}
	}
#endif
	porder->ovl.Offset=loff.LowPart;
	porder->ovl.OffsetHigh=loff.HighPart;
	porder->marker=0xFDFDFDFD;
#ifdef MULTI_RECTYPES
	if(!porder->rtype)
		porder->rtype=0x01;
#else
	porder->rtype=0x01;
#endif
	porder->rlen=VSORDER_RECSIZE;
	if(porder->deFirst)
		porder->noff=porder->deFirst->offset;
	_ASSERT((char*)&porder->noff +sizeof(porder->noff) -(char*)&porder->marker==VSORDER_RECSIZE);
	SetFilePointer(fhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
	//DT9044
	if(strncmp(porder->ClOrdID,VSDB_ACCOUNTREC_IND,strlen(VSDB_ACCOUNTREC_IND)))
	{
		_ASSERT(porder->AppInstID[0]);
		_ASSERT(porder->ClOrdID[0]);
	}
	DWORD wbytes=0;
	int rc=WriteFile(fhnd,&porder->marker,VSORDER_RECSIZE,&wbytes,0);
	int lerr=GetLastError();
	if((rc<0)||(wbytes!=VSORDER_RECSIZE))
	{
		char emsg[256]={0};
		#ifdef WIN32
		sprintf(emsg,"Failed write on order(%s) at offset %I64d!",porder->OrderKey,loff.QuadPart);
		#else
		sprintf(emsg,"Failed write on order(%s) at offset %lld!",porder->OrderKey,loff.QuadPart);
		#endif
		pnotify->ODBError(-1,emsg);
		Unlock();
		return -1;
	}
#else//!WIN32
	if(offset<(ITEMLOC)0)
	{
	#ifdef NO_DB_WRITES
		porder->offset=offset=0;
		Unlock();
		return 0;
	#endif
		offset=fend; fend+=VSORDER_RECSIZE;
		if(fend>fsize)
		{
			// Growing the data file by large blocks increases write throughput
			if(GrowFile(fend)<0)
			{
				char emsg[256]={0};
				sprintf(emsg,"Failed growing data file to %s bytes!",SizeStr(fend,true));
				pnotify->ODBError(-1,emsg);
				Unlock();
				return 0;
			}
		}
	}
	// Also write all detail extension index blocks
	for(VSDetExt *pde=porder->deFirst;pde;pde=pde->nextDetExt)
	{
		if(pde->dirty)
		{
			_ASSERT(porder->offset>=0);
			LONGLONG eoff=pde->offset;
			WriteDetExt(pde,eoff);
		}
	}
	LONGLONG loff=offset;
	porder->marker=0xFDFDFDFD;
	porder->rtype=0x01;
	porder->rlen=VSORDER_RECSIZE;
	if(porder->deFirst)
		porder->noff=porder->deFirst->offset;
	_ASSERT((char*)&porder->noff +sizeof(porder->noff) -(char*)&porder->marker==VSORDER_RECSIZE);
	fseeko(fhnd,loff,SEEK_SET);
	_ASSERT(porder->AppInstID[0]);
	_ASSERT(porder->ClOrdID[0]);
	DWORD wbytes=0;
	wbytes=(DWORD)fwrite(&porder->marker,1,VSORDER_RECSIZE,fhnd);
	if(wbytes!=VSORDER_RECSIZE)
	{
		_ASSERT(false);//untested
		char emsg[256]={0};
		#ifdef WIN32
		sprintf(emsg,"Failed write on order(%s) at offset %I64d!",porder->OrderKey,loff);
		#else
		sprintf(emsg,"Failed write on order(%s) at offset %lld!",porder->OrderKey,loff);
		#endif
		pnotify->ODBError(-1,emsg);
		Unlock();
		return -1;
	}
	fflush(fhnd);
#endif//!WIN32
	porder->offset=offset;
	porder->dirty=false;
	//pnotify->ODBOrderCommitted(0,porder);
#ifndef TASK_THREAD_POOL
	// Prod will have fusion I/O. This is mainly to speed up dev and staging
	if(!HAVE_FUSION_IO)
	{
		CACHEDORDERMAP::iterator cit=comap.find(offset);
		if(cit==comap.end())
		{
			while(comap.size()>1000)
				comap.erase(comap.begin());
			comap.insert(pair<ITEMLOC,VSOrder*>(offset,porder));
		}
	}
#endif
	Unlock();
	return 0;
}
VSOrder *OrderDB::ReadOrder(const ITEMLOC& roffset)
{
	ITEMLOC offset=roffset;
	if(offset<(ITEMLOC)0)
		return 0;
#ifndef TASK_THREAD_POOL
	// Check internal cache
	if(!HAVE_FUSION_IO)
	{
		CACHEDORDERMAP::iterator cit=comap.find(offset);
		if(cit!=comap.end())
		{
			VSOrder *corder=cit->second;
			_ASSERT(corder->OrderKey[0]);
			return corder;
		}
	}
#endif
	Lock();
	if(fhnd==INVALID_FILE_VALUE)
	{
		Unlock();
		return 0;
	}
	VSOrder *porder=AllocOrder();
	if(!porder)
	{
		_ASSERT(false);
		Unlock();
		return 0;
	}
	#ifdef MULTI_DAY_ITEMLOC
	offset.wsdate=wsdate;
	#endif
	porder->offset=offset;
#ifdef WIN32
	LARGE_INTEGER loff;
	loff.QuadPart=(LONGLONG)offset;
	SetFilePointer(fhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
	DWORD rbytes=0;
	if(!ReadFile(fhnd,&porder->marker,VSORDER_RECSIZE,&rbytes,0)||(rbytes!=VSORDER_RECSIZE)||(porder->marker!=0xFDFDFDFD))
	{
		_ASSERT(false);
		char emsg[256]={0};
		sprintf(emsg,"Failed ReadOrder(%s) at offset %I64d!",fpath.c_str(),loff.QuadPart);
		pnotify->ODBError(-1,emsg);
		porder->offset=-1;
		FreeOrder(porder);
		Unlock();
		return 0;
	}
	porder->offset=offset;
#ifdef MULTI_DAY
	if(INDEX_ORDERDATE)
		sprintf(porder->OrderKey,"%08d.%s",porder->OrderDate,porder->ClOrdID);
	else
		strcpy(porder->OrderKey,porder->ClOrdID);
#else
	strcpy(porder->OrderKey,porder->ClOrdID);
#endif
	// Also read all detail extension index blocks
	if(porder->noff>=porder->offset)
	{
		LONGLONG noff=porder->noff;
		while(noff>loff.QuadPart)
		{
			VSDetExt *pde=ReadDetExt(noff);
			if(!pde)
			{
				_ASSERT(false);
				FreeOrder(porder);
				Unlock();
				return 0;
			}
			porder->AddDetExt(pde);
			loff.QuadPart=noff; noff=pde->noff;
		}
	}
	else
		porder->noff=-1;
#else
	LONGLONG loff=(ITEMLOC)offset;
	fseeko(fhnd,loff,SEEK_SET);
	int rbytes=(int)fread(&porder->marker,1,VSORDER_RECSIZE,fhnd);
	if((rbytes!=VSORDER_RECSIZE)||(porder->marker!=0xFDFDFDFD))
	{
		_ASSERT(false);
		char emsg[256]={0};
		sprintf(emsg,"Failed ReadOrder(%s) at offset %lld!",fpath.c_str(),loff);
		pnotify->ODBError(-1,emsg);
		FreeOrder(porder);
		Unlock();
		return 0;
	}
	porder->offset=loff;
	strcpy(porder->OrderKey,porder->ClOrdID);
	// Also read all detail extension index blocks
	if(porder->noff>=porder->offset)
	{
		LONGLONG noff=porder->noff;
		while(noff>loff)
		{
			VSDetExt *pde=ReadDetExt(noff);
			if(!pde)
			{
				_ASSERT(false);
				FreeOrder(porder);
				Unlock();
				return 0;
			}
			porder->AddDetExt(pde);
			loff=noff; noff=pde->noff;
		}
	}
	else
		porder->noff=-1;
#endif
	porder->dirty=false;
#ifndef TASK_THREAD_POOL
	// Prod will have fusion I/O. This is mainly to speed up dev and staging
	if(!HAVE_FUSION_IO)
	{
		while(comap.size()>1000)
			comap.erase(comap.begin());
		comap.insert(pair<ITEMLOC,VSOrder*>(offset,porder));
	}
#endif
	Unlock();
	return porder;
}
VSDetExt *OrderDB::AllocDetExt()
{
	somem+=sizeof(VSDetExt);
	return new VSDetExt;
}
void OrderDB::FreeDetExt(VSDetExt *pde)
{
	delete pde;
	somem-=sizeof(VSDetExt);
}
int OrderDB::WriteDetExt(VSDetExt *pde, LONGLONG& offset)
{
	if(!pde)
		return -1;
	Lock();
	if(fhnd==INVALID_FILE_VALUE)
	{
		Unlock();
		return -1;
	}
#ifdef WIN32
	if(offset<0)
	{
	#ifdef NO_DB_WRITES
		pde->offset=offset=0;
		Unlock();
		return 0;
	#endif
		offset=fend.QuadPart; fend.QuadPart+=VSDETEXT_RECSIZE;
		if(fend.QuadPart>fsize.QuadPart)
		{
			// Growing the data file by large blocks increases write throughput
			if(GrowFile(fend.QuadPart)<0)
			{
				char emsg[256]={0};
				sprintf(emsg,"Failed growing data file to %s bytes!",SizeStr(fend.QuadPart,true));
				pnotify->ODBError(-1,emsg);
				Unlock();
				return 0;
			}
		}
	}
	LARGE_INTEGER loff;
	loff.QuadPart=offset;
#ifdef VALIDATE_DBWRITES
	// To prevent corruption, read the location before we write 
	// to make sure we're not overwriting another record
	{
		char emsg[256]={0};
		SetFilePointer(fhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
		DWORD rbytes=0;
		VSDetExt rde;
		int rc=ReadFile(fhnd,&rde.marker,VSDETEXT_RECSIZE,&rbytes,0);
		int lerr=GetLastError();
		if((rc<0)||(rbytes!=VSDETEXT_RECSIZE))
		{
			#ifdef WIN32
			sprintf(emsg,"Failed read to validate detext(%s) at offset %I64d!",pde->OrderKey,loff.QuadPart);
			#else
			sprintf(emsg,"Failed read to validate detext(%s) at offset %lld!",pde->OrderKey,loff.QuadPart);
			#endif
			pnotify->ODBError(-1,emsg);
			Unlock();
			return -1;
		}
		// New order validation
		if(pde->offset<0)
		{
			if(rde.OrderKey[0])
			{
				#ifdef WIN32
				sprintf(emsg,"Failed write on detext(%s) at offset %I64d--detext(%s) at that location!",
				#else
				sprintf(emsg,"Failed write on detext(%s) at offset %lld--detext(%s) at that location!",
				#endif
					pde->OrderKey,loff.QuadPart,rde.OrderKey);
				pnotify->ODBError(-1,emsg);
				Unlock();
				return -1;
			}
		}
		// Update order validation
		else
		{
			if(_stricmp(rde.OrderKey,pde->OrderKey))
			{
				#ifdef WIN32
				sprintf(emsg,"Failed update on detext(%s) at offset %I64d--detext(%s) at that location!",
				#else
				sprintf(emsg,"Failed update on detext(%s) at offset %lld--detext(%s) at that location!",
				#endif
					pde->OrderKey,loff.QuadPart,rde.OrderKey);
				pnotify->ODBError(-1,emsg);
				Unlock();
				return -1;
			}
		}
	}
#endif
	pde->marker=0xFDFDFDFD;
	pde->rtype=0x03;
	pde->rlen=VSDETEXT_RECSIZE;
	_ASSERT((char*)&pde->noff +sizeof(pde->noff) -(char*)&pde->marker==VSDETEXT_RECSIZE);
	SetFilePointer(fhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
	DWORD wbytes=0;
	if(!WriteFile(fhnd,&pde->marker,VSDETEXT_RECSIZE,&wbytes,0)||(wbytes!=VSDETEXT_RECSIZE))
	{
		int lerr=GetLastError();
		_ASSERT(false);//untested
		char emsg[256]={0};
		sprintf(emsg,"Failed write on detext(%s) at offset %I64d!",pde->OrderKey,loff.QuadPart);
		pnotify->ODBError(-1,emsg);
		Unlock();
		return -1;
	}
#else//!WIN32
	if(offset<0)
	{
	#ifdef NO_DB_WRITES
		pde->offset=offset=0;
		Unlock();
		return 0;
	#endif
		offset=fend; fend+=VSDETEXT_RECSIZE;
		if(fend>fsize)
		{
			// Growing the data file by large blocks increases write throughput
			if(GrowFile(fend)<0)
			{
				char emsg[256]={0};
				sprintf(emsg,"Failed growing data file to %s bytes!",SizeStr(fend,true));
				pnotify->ODBError(-1,emsg);
				Unlock();
				return 0;
			}
		}
	}
	LONGLONG loff=offset;
	pde->marker=0xFDFDFDFD;
	pde->rtype=0x03;
	pde->rlen=VSDETEXT_RECSIZE;
	_ASSERT((char*)&pde->noff +sizeof(pde->noff) -(char*)&pde->marker==VSDETEXT_RECSIZE);
	fseeko(fhnd,loff,SEEK_SET);
	DWORD wbytes=(DWORD)fwrite(&pde->marker,1,VSDETEXT_RECSIZE,fhnd);
	if(wbytes!=VSDETEXT_RECSIZE)
	{
		int lerr=GetLastError();
		_ASSERT(false);//untested
		char emsg[256]={0};
		sprintf(emsg,"Failed write on detext(%s) at offset %lld!",pde->OrderKey,loff);
		pnotify->ODBError(-1,emsg);
		Unlock();
		return -1;
	}
	fflush(fhnd);
#endif//!WIN32
	pde->offset=offset;
	pde->dirty=false;
	Unlock();
	return 0;
}
VSDetExt *OrderDB::ReadDetExt(LONGLONG offset)
{
	if(offset<0)
		return 0;
	Lock();
	if(fhnd==INVALID_FILE_VALUE)
	{
		Unlock();
		return 0;
	}
	VSDetExt *pde=AllocDetExt();
	if(!pde)
	{
		_ASSERT(false);
		Unlock();
		return 0;
	}
	pde->offset=offset;
#ifdef WIN32
	LARGE_INTEGER loff;
	loff.QuadPart=offset;
	SetFilePointer(fhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
	DWORD rbytes=0;
	if(!ReadFile(fhnd,&pde->marker,VSDETEXT_RECSIZE,&rbytes,0)||(rbytes!=VSDETEXT_RECSIZE)||(pde->marker!=0xFDFDFDFD))
	{
		_ASSERT(false);
		char emsg[256]={0};
		sprintf(emsg,"Failed ReadDetExt at offset %I64d!",loff.QuadPart);
		pnotify->ODBError(-1,emsg);
		FreeDetExt(pde);
		Unlock();
		return 0;
	}
	pde->offset=loff.QuadPart;
#else
	LONGLONG loff=offset;
	fseeko(fhnd,loff,SEEK_SET);
	DWORD rbytes=(DWORD)fread(&pde->marker,1,VSDETEXT_RECSIZE,fhnd);
	if((rbytes!=VSDETEXT_RECSIZE)||(pde->marker!=0xFDFDFDFD))
	{
		_ASSERT(false);
		char emsg[256]={0};
		sprintf(emsg,"Failed ReadDetExt at offset %lld!",loff);
		pnotify->ODBError(-1,emsg);
		FreeDetExt(pde);
		Unlock();
		return 0;
	}
	pde->offset=loff;
#endif
	pde->dirty=false;
	Unlock();
	return pde;
}
int OrderDB::AssociateOrder(VSOrder *porder, const ITEMLOC& oloc, DWORD& totOrderCnt, DWORD& totRootCnt)
{
	// Multimap behavior test code
	//#ifdef _DEBUG
	//ITEMLOC tloc(0,0);
	//VSINDEX tmap;
	//tloc.off++; tmap.m_insert("A",tloc);
	//tloc.off++; tmap.m_insert("A",tloc);
	//tloc.off++; tmap.m_insert("B",tloc);
	//VSINDEX::m_iterator tit=tmap.m_find("A");
	//if(tit!=tmap.end())
	//{
	//	tloc.off++; tmap.m_insert(tit,"A",tloc);
	//}
	//char tbuf[256]={0};
	//for(tit=tmap.m_begin();tit!=tmap.m_end();tit++)
	//{
	//	sprintf(tbuf,"%s,%d\r\n",tit.first.c_str(),tit.second.off);
	//	OutputDebugString(tbuf);
	//}
	//#endif

	if(!porder)
		return -1;
	totOrderCnt++;
	// Multimap by RootOrderID
	Lock();
	if(INDEX_ROOTORDERID)
	{
		string poid=porder->GetRootOrderID();
		if(!poid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator pit=pmap.m_find(poid);
			//for(;(pit!=pmap.m_end())&&(pit.first==poid);pit++)
			//{
			//	if(pit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				pit=pmap.insert(poid,oloc);
			#else
				if(pit==pmap.m_end())
				{
					pit=pmap.m_insert(poid,oloc);
					totRootCnt++;
				}
				else
					pit=pmap.m_insert(pit,poid,oloc);
			#endif
			}
		}
	}
	if(INDEX_FIRSTCLORDID)
	{
		string foid=porder->GetFirstClOrdID();
		if(!foid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator fit=fmap.m_find(foid);
			//for(;(fit!=fmap.m_end())&&(fit.first==foid);fit++)
			//{
			//	if(fit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				fit=fmap.insert(foid,oloc);
			#else
				if(fit==fmap.m_end())
					fit=fmap.m_insert(foid,oloc);
				else
					fit=fmap.m_insert(fit,foid,oloc);
			#endif
			}
		}
	}
	if(INDEX_CLPARENTORDERID)
	{
		string cpoid=porder->GetClParentOrderID();
		if(!cpoid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator cpit=cpmap.m_find(cpoid);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				cpit=cpmap.insert(cpoid,oloc);
			#else
				if(cpit==cpmap.m_end())
					cpit=cpmap.m_insert(cpoid,oloc);
				else
					cpit=cpmap.m_insert(cpit,cpoid,oloc);
			#endif
			}
		}
	}

	if(INDEX_SYMBOL)
	{
		// Multimap by symbol
		string symbol=porder->GetSymbol();
		if(!symbol.empty())
		{
			bool found=false;
			VSINDEX::m_iterator sit=symmap.m_find(symbol);
			// Very slow for thousands of orders!!
			//for(;(sit!=symmap.m_end())&&(sit.first==symbol);sit++)
			//{
			//	if(sit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				sit=symmap.insert(symbol,oloc);
			#else
				if(sit==symmap.m_end())
					sit=symmap.m_insert(symbol,oloc);
				else
					sit=symmap.m_insert(sit,symbol,oloc);
			#endif
			}
		}
	}

	if(INDEX_ACCOUNT)
	{
		// Multimap by account
		string acct=porder->GetAccount();
		if(!acct.empty())
		{
			bool found=false;
			VSINDEX::m_iterator ait=acctmap.m_find(acct);
			//for(;(ait!=acctmap.m_end())&&(ait.first==acct);ait++)
			//{
			//	if(ait.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				ait=acctmap.insert(acct,oloc);
			#else
				if(ait==acctmap.m_end())
					ait=acctmap.m_insert(acct,oloc);
				else
					ait=acctmap.m_insert(ait,acct,oloc);
			#endif
			}
		}
	}
	if(INDEX_ECNORDERID)
	{
		// Multimap by ecn orderid
		string ecnoid=porder->GetEcnOrderID();
		if(!ecnoid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator eit=omap2.m_find(ecnoid);
			//for(;(eit!=omap2.m_end())&&(eit.first==ecnoid);eit++)
			//{
			//	if(eit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				eit=omap2.insert(ecnoid,oloc);
			#else
				if(eit==omap2.m_end())
					eit=omap2.m_insert(ecnoid,oloc);
				else
					eit=omap2.m_insert(eit,ecnoid,oloc);
			#endif
			}
		}
	}
	if(INDEX_CLIENTID)
	{
		// Multimap by clientid
		string clientid=porder->GetClientID();
		if(!clientid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator ciit=cimap.m_find(clientid);
			//for(;(ciit!=cimap.m_end())&&(ciit.first==cimap);ciit++)
			//{
			//	if(ciit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				ciit=cimap.insert(clientid,oloc);
			#else
				if(ciit==cimap.m_end())
					ciit=cimap.m_insert(clientid,oloc);
				else
					ciit=cimap.m_insert(ciit,clientid,oloc);
			#endif
			}
		}
	}
	if(INDEX_TRANSACTTIME)
	{
		// Multimap by transacttime
		LONGLONG cts=porder->GetTransactTime();
		if(cts)
		{
			//char tstr[64]={0};
			//DWORD wsdate=(int)(cts/1000000);
			//DWORD wstime=(int)(cts%1000000);
			//sprintf(tstr,"%08d-%02d:%02d:%02d",wsdate,wstime/10000,(wstime%10000)/100,wstime%100);
			//sprintf(tstr,"%I64d",cts);
			bool found=false;
			TSINDEX::m_iterator tsit=tsmap.m_find(cts);
			//for(;(tsit!=tsmap.end())&&(tsit.first==okey);tsit++)
			//{
			//	if(tsit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			//#ifdef USE_DISK_INDEX
			//	tsit=tsmap.insert(cts,oloc);
			//#else
				if(tsit==tsmap.m_end())
					tsit=tsmap.m_insert(cts,oloc);
				else
					tsit=tsmap.m_insert(tsit,cts,oloc);
			//#endif
			}
		}
	}
	if(INDEX_OPEN_ORDERS)
	{
		// Hash map by open orders
		if(!porder->IsTerminated())
		{
			string okey=porder->GetOrderKey();
			if(!okey.empty())
			{
				bool found=false;
				VSINDEX::iterator aiit=oomap.find(okey);
				//for(;(aiit!=oomap.end())&&(aiit.first==okey);aiit++)
				//{
				//	if(aiit.second.off==oloc.off)
				//	{
				//		found=true;
				//		break;
				//	}
				//}
				if(!found)
				{
				#ifdef USE_DISK_INDEX
					aiit=oomap.insert(okey,oloc);
				#else
					if(aiit==oomap.end())
						aiit=oomap.insert(okey,oloc);
					// Only add the order once (not multimap behavior)
					//else
					//	aiit=oomap.insert(aiit,okey,oloc);
				#endif
				}
			}
		}
	}	
	if(INDEX_CONNECTIONS)
	{
		// Multimap by SCID(49),SSID(50),TCID(56),TSID(57)
		string ckey=porder->GetConnection();
		if(!ckey.empty())
		{
			bool found=false;
			VSINDEX::m_iterator coit=cnmap.m_find(ckey);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				coit=cnmap.m_insert(ckey,oloc);
			#else
				if(coit==cnmap.m_end())
					coit=cnmap.m_insert(ckey,oloc);
				else
					coit=cnmap.m_insert(coit,ckey,oloc);
			#endif
			}
		}
	}
#ifdef MULTI_DAY
	if(INDEX_ORDERDATE)
	{
		int odkey=porder->GetOrderDate();
		if(odkey)
		{
			bool found=false;
			DATEINDEX::iterator odit=odmap.find(odkey);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				odit=odmap.m_insert(pair<int,ITEMLOC>(odkey,oloc));
			#else
				if(odit==odmap.end())
					odit=odmap.insert(pair<int,ITEMLOC>(odkey,oloc));
				else
					odit=odmap.insert(odit,pair<int,ITEMLOC>(odkey,oloc));
			#endif
			}
		}
	}
#endif
	for(unsigned int i=0; i<INDEX_AUXKEYS; i++) //DT9398
	{
		string akey=porder->GetAuxKey(i);
		if(!akey.empty())
		{
			bool found=false;
			VSINDEX::m_iterator akit=akmap[i].m_find(akey);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				akit=akmap.insert(akey,oloc);
			#else
				if(akit==akmap[i].m_end())
					akit=akmap[i].m_insert(akey,oloc);
				else
					akit=akmap[i].m_insert(akit,akey,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
// Copy this code to disassociate order keys
//int OrderDB::DisAssociateOrder(VSOrder *porder)
//{
//	if(!porder)
//		return -1;
//	//if(totOrderCnt>0) totOrderCnt--;
//	// Multimap by RootOrderID
//	Lock();
//	ITEMLOC oloc=porder->GetOffset();
//	if(INDEX_ROOTORDERID)
//	{
//		string poid=porder->GetRootOrderID();
//		if(!poid.empty())
//		{
//			for(VSINDEX::m_iterator pit=pmap.m_find(poid);
//				(pit!=pmap.m_end())&&(pit.first==poid);
//				pit++)
//			{
//				if(pit.second==oloc)
//				{
//					pmap.m_erase(pit);
//					break;
//				}
//			}
//		}
//	}
//	if(INDEX_FIRSTCLORDID)
//	{
//		string foid=porder->GetFirstClOrdID();
//		if(!foid.empty())
//		{
//			for(VSINDEX::m_iterator fit=fmap.m_find(foid);
//				(fit!=fmap.m_end())&&(fit.first==foid);
//				fit++)
//			{
//				if(fit.second==oloc)
//				{
//					fmap.m_erase(fit);
//					break;
//				}
//			}
//		}
//	}
//	if(INDEX_CLPARENTORDERID)
//	{
//		string cpoid=porder->GetClParentOrderID();
//		if(!cpoid.empty())
//		{
//			for(VSINDEX::m_iterator cpit=cpmap.m_find(cpoid);
//				(cpit!=cpmap.m_end())&&(cpit.first==cpoid);
//				cpit++)
//			{
//				if(cpit.second==oloc)
//				{
//					cpmap.m_erase(cpit);
//					break;
//				}
//			}
//		}
//	}
//
//	if(INDEX_SYMBOL)
//	{
//		// Multimap by symbol
//		string symbol=porder->GetSymbol();
//		if(!symbol.empty())
//		{
//			for(VSINDEX::m_iterator sit=symmap.m_find(symbol);
//				(sit!=symmap.m_end())&&(sit.first==symbol);
//				sit++)
//			{
//				if(sit.second==oloc)
//				{
//					symmap.m_erase(sit);
//					break;
//				}
//			}
//		}
//	}
//
//	if(INDEX_ACCOUNT)
//	{
//		// Multimap by account
//		string acct=porder->GetAccount();
//		if(!acct.empty())
//		{
//			for(VSINDEX::m_iterator ait=acctmap.m_find(acct);
//				(ait!=acctmap.m_end())&&(ait.first==acct);
//				ait++)
//			{
//				if(ait.second==oloc)
//				{
//					acctmap.m_erase(ait);
//					break;
//				}
//			}
//		}
//	}
//	if(INDEX_ECNORDERID)
//	{
//		// Multimap by ecn orderid
//		string ecnoid=porder->GetEcnOrderID();
//		if(!ecnoid.empty())
//		{
//			for(VSINDEX::m_iterator eit=omap2.m_find(ecnoid);
//				(eit!=omap2.m_end())&&(eit.first==ecnoid);
//				eit++)
//			{
//				if(eit.second==oloc)
//				{
//					omap2.m_erase(eit);
//					break;
//				}
//			}
//		}
//	}
//	if(INDEX_CLIENTID)
//	{
//		// Multimap by clientid
//		string clientid=porder->GetClientID();
//		if(!clientid.empty())
//		{
//			for(VSINDEX::m_iterator ciit=cimap.m_find(clientid);
//				(ciit!=cimap.m_end())&&(ciit.first==clientid);
//				ciit++)
//			{
//				if(ciit.second==oloc)
//				{
//					cimap.m_erase(ciit);
//					break;
//				}
//			}
//		}
//	}
//	if(INDEX_TRANSACTTIME)
//	{
//		// Multimap by transacttime
//		LONGLONG cts=porder->GetTransactTime();
//		if(cts)
//		{
//			for(TSINDEX::m_iterator tsit=tsmap.m_find(cts);
//				(tsit!=tsmap.m_end())&&(tsit.first==cts);
//				tsit++)
//			{
//				if(tsit.second==oloc)
//				{
//					tsmap.m_erase(tsit);
//					break;
//				}
//			}
//		}
//	}
//	if(INDEX_OPEN_ORDERS)
//	{
//		// Hash map by open orders
//		if(!porder->IsTerminated())
//		{
//			string okey=porder->GetOrderKey();
//			if(!okey.empty())
//			{
//				for(VSINDEX::m_iterator aiit=oomap.m_find(okey);
//					(aiit!=oomap.m_end())&&(aiit.first==okey);
//					aiit++)
//				{
//					if(aiit.second==oloc)
//					{
//						oomap.m_erase(aiit);
//						break;
//					}
//				}
//			}
//		}
//	}	
//	if(INDEX_CONNECTIONS)
//	{
//		// Multimap by SCID(49),SSID(50),TCID(56),TSID(57)
//		string ckey=porder->GetConnection();
//		if(!ckey.empty())
//		{
//			for(VSINDEX::m_iterator coit=cnmap.m_find(ckey);
//				(coit!=cnmap.m_end())&&(coit.first==ckey);
//				coit++)
//			{
//				if(coit.second==oloc)
//				{
//					cnmap.m_erase(coit);
//					break;
//				}
//			}
//		}
//	}
//	#ifdef MULTI_DAY
//	if(INDEX_ORDERDATE)
//	{
//		int odkey=porder->GetOrderDate();
//		if(odkey)
//		{
//			for(DATEINDEX::iterator odit=odmap.find(odkey);
//				(odit!=odmap.end())&&(odit->first==odkey);
//				odit++)
//			{
//				if(odit->second==oloc)
//				{
//					odmap.erase(odit);
//					break;
//				}
//			}
//		}
//	}
//	#endif
//	for(unsigned int i=0; i<INDEX_AUXKEYS; i++) //DT9398
//	{
//		string akey=porder->GetAuxKey(i);
//		if(!akey.empty())
//		{
//			for(VSINDEX::m_iterator akit=akmap[i].m_find(akey);
//				(akit!=akmap[i].m_end())&&(akit.first==akey);
//				akit++)
//			{
//				if(akit.second==oloc)
//				{
//					akmap[i].m_erase(akit);
//					break;
//				}
//			}
//		}
//	}
//	Unlock();
//	return 1;
//}
int OrderDB::AssociateAccount(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_ACCOUNT)
	{
		// Multimap by account
		string acct=porder->GetAccount();
		if(!acct.empty())
		{
			bool found=false;
			VSINDEX::m_iterator ait=acctmap.m_find(acct);
			//for(;(ait!=acctmap.m_end())&&(ait.first==acct);ait++)
			//{
			//	if(ait.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				ait=acctmap.insert(acct,oloc);
			#else
				if(ait==acctmap.m_end())
					ait=acctmap.m_insert(acct,oloc);
				else
					ait=acctmap.m_insert(ait,acct,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::DisAssociateAccount(VSOrder *porder)
{
	Lock();
	if(INDEX_ACCOUNT)
	{
		// Multimap by account
		string acct=porder->GetAccount();
		if(!acct.empty())
		{
			ITEMLOC oloc=porder->GetOffset();
			for(VSINDEX::m_iterator ait=acctmap.m_find(acct);
				(ait!=acctmap.m_end())&&(ait.first==acct);
				ait++)
			{
				if(ait.second==oloc)
				{
					acctmap.m_erase(ait);
					break;
				}
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateFirstClOrdID(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_FIRSTCLORDID)
	{
		string foid=porder->GetFirstClOrdID();
		if(!foid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator fit=fmap.m_find(foid);
			//for(;(fit!=fmap.m_end())&&(fit.first==foid);fit++)
			//{
			//	if(fit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				fit=fmap.insert(foid,oloc);
			#else
				if(fit==fmap.m_end())
					fit=fmap.m_insert(foid,oloc);
				else
					fit=fmap.m_insert(fit,foid,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::DisAssociateFirstClOrdID(VSOrder *porder)
{
	Lock();
	if(INDEX_FIRSTCLORDID)
	{
		string foid=porder->GetFirstClOrdID();
		if(!foid.empty())
		{
			ITEMLOC oloc=porder->GetOffset();
			for(VSINDEX::m_iterator fit=fmap.m_find(foid);
				(fit!=fmap.m_end())&&(fit.first==foid);
				fit++)
			{
				if(fit.second==oloc)
				{
					fmap.m_erase(fit);
					break;
				}
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateEcnOrderID(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_ECNORDERID)
	{
		// Multimap by ecn orderid
		string ecnoid=porder->GetEcnOrderID();
		if(!ecnoid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator eit=omap2.m_find(ecnoid);
			//for(;(eit!=omap2.m_end())&&(eit.first==ecnoid);eit++)
			//{
			//	if(eit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				eit=omap2.insert(ecnoid,oloc);
			#else
				if(eit==omap2.m_end())
					eit=omap2.m_insert(ecnoid,oloc);
				else
					eit=omap2.m_insert(eit,ecnoid,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateClientID(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_CLIENTID)
	{
		// Multimap by ecn orderid
		string clientid=porder->GetClientID();
		if(!clientid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator ciit=cimap.m_find(clientid);
			//for(;(ciit!=cimap.m_end())&&(ciit.first==clientid);ciit++)
			//{
			//	if(ciit.second.off==oloc.off)
			//	{
			//		found=true;
			//		break;
			//	}
			//}
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				ciit=cimap.insert(clientid,oloc);
			#else
				if(ciit==cimap.m_end())
					ciit=cimap.m_insert(clientid,oloc);
				else
					ciit=cimap.m_insert(ciit,clientid,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateClParentOrderID(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_CLPARENTORDERID)
	{
		string cpoid=porder->GetClParentOrderID();
		if(!cpoid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator cpit=cpmap.m_find(cpoid);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				cpit=cpmap.insert(cpoid,oloc);
			#else
				if(cpit==cpmap.m_end())
					cpit=cpmap.m_insert(cpoid,oloc);
				else
					cpit=cpmap.m_insert(cpit,cpoid,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateTransactTime(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_TRANSACTTIME)
	{
		LONGLONG cts=porder->GetTransactTime();
		if(cts>0)
		{
			bool found=false;
			TSINDEX::m_iterator tsit=tsmap.m_find(cts);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				tsit=tsmap.insert(cts,oloc);
			#else
				if(tsit==tsmap.m_end())
					tsit=tsmap.m_insert(cts,oloc);
				else
					tsit=tsmap.m_insert(tsit,cts,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
#ifdef MULTI_DAY
int OrderDB::AssociateOrderDate(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_ORDERDATE)
	{
		int odate=porder->GetOrderDate();
		if(odate>0)
		{
			bool found=false;
			DATEINDEX::iterator odit=odmap.find(odate);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				odit=odmap.insert(pair<int,ITEMLOC>(odate,oloc));
			#else
				if(odit==odmap.end())
					odit=odmap.insert(pair<int,ITEMLOC>(odate,oloc));
				else
					odit=odmap.insert(odit,pair<int,ITEMLOC>(odate,oloc));
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
#endif
int OrderDB::AssociateAuxKey(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	for(unsigned int i=0; i<INDEX_AUXKEYS;i++) //DT9398
	{
		string akoid=porder->GetAuxKey(i);
		if(!akoid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator akit=akmap[i].m_find(akoid);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				akit=akmap.insert(akoid,oloc);
			#else
				if(akit==akmap[i].m_end())
					akit=akmap[i].m_insert(akoid,oloc);
				else
					akit=akmap[i].m_insert(akit,akoid,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateRootOrderID(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_ROOTORDERID)
	{
		string poid=porder->GetRootOrderID();
		if(!poid.empty())
		{
			bool found=false;
			VSINDEX::m_iterator pit=pmap.m_find(poid);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				pit=pmap.insert(poid,oloc);
			#else
				if(pit==pmap.m_end())
				{
					pit=pmap.m_insert(poid,oloc);
					//totRootCnt++;
				}
				else
					pit=pmap.m_insert(pit,poid,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::DisAssociateRootOrderID(VSOrder *porder)
{
	Lock();
	if(INDEX_ROOTORDERID)
	{
		string poid=porder->GetRootOrderID();
		if(!poid.empty())
		{
			ITEMLOC oloc=porder->GetOffset();
			for(VSINDEX::m_iterator pit=pmap.m_find(poid);
				(pit!=pmap.m_end())&&(pit.first==poid);
				pit++)
			{
				if(pit.second==oloc)
				{
					pmap.m_erase(pit);
					break;
				}
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::AssociateSymbol(VSOrder *porder, const ITEMLOC& oloc)
{
	if(!porder)
		return -1;
	Lock();
	if(INDEX_SYMBOL)
	{
		// Multimap by symbol
		string symbol=porder->GetSymbol();
		if(!symbol.empty())
		{
			bool found=false;
			VSINDEX::m_iterator sit=symmap.m_find(symbol);
			if(!found)
			{
			#ifdef USE_DISK_INDEX
				sit=symmap.insert(symbol,oloc);
			#else
				if(sit==symmap.m_end())
					sit=symmap.m_insert(symbol,oloc);
				else
					sit=symmap.m_insert(sit,symbol,oloc);
			#endif
			}
		}
	}
	Unlock();
	return 1;
}
int OrderDB::DisAssociateSymbol(VSOrder *porder)
{
	Lock();
	if(INDEX_SYMBOL)
	{
		// Multimap by symbol
		string symbol=porder->GetSymbol();
		if(!symbol.empty())
		{
			ITEMLOC oloc=porder->GetOffset();
			for(VSINDEX::m_iterator sit=symmap.m_find(symbol);
				(sit!=symmap.m_end())&&(sit.first==symbol);
				sit++)
			{
				if(sit.second==oloc)
				{
					symmap.m_erase(sit);
					break;
				}
			}
		}
	}
	Unlock();
	return 1;
}

VSDetailFile::VSDetailFile()
	:fpath()
	,prefix()
	,wsdate(0)
	,readonly(false)
	,portno(-1)
	,proto(0)
	#ifdef WIN32
	,whnd(INVALID_FILE_VALUE)
	,rhnd(INVALID_FILE_VALUE)
	#else
	,whnd(0)
	,rhnd(0)
	#endif
	,rthnd(0)
	,pnotify(0)
{
	#ifdef WIN32
	fend.QuadPart=fsize.QuadPart=rend.QuadPart=aend.QuadPart=0;
	#else
	fend=fsize=rend=aend=0;
	#endif
	wmutex=CreateMutex(0,false,0);
	rmutex=CreateMutex(0,false,0);
}
VSDetailFile::~VSDetailFile()
{
	if(wmutex)
	#ifdef _DEBUG
		WaitForSingleObject(wmutex,INFINITE);
	#else
		WaitForSingleObject(wmutex,1000);
	#endif
	if(rmutex)
	#ifdef _DEBUG
		WaitForSingleObject(rmutex,INFINITE);
	#else
		WaitForSingleObject(rmutex,1000);
	#endif

	Shutdown();

#ifdef WIN32
	if(rmutex)
	{
		CloseHandle(rmutex); rmutex=0;
	}
	if(wmutex)
	{
		CloseHandle(wmutex); wmutex=0;
	}
#else
	if(rmutex)
	{
		DeleteMutex(rmutex); rmutex=0;
	}
	if(wmutex)
	{
		DeleteMutex(wmutex); wmutex=0;
	}
#endif
}
int VSDetailFile::Init(VSDetailFileNotify *pnotify, const char *dbpath, const char *prefix, int wsdate, bool readonly, short portno, char proto)
{
	if((!pnotify)||(!dbpath)||(!*dbpath))
		return -1;
	LockWrite();
	this->pnotify=pnotify;
	this->fpath=dbpath;
	this->prefix=prefix;
	this->wsdate=wsdate;
	this->readonly=readonly;
	this->portno=portno;
	this->proto=proto;
	// Use separate handle for read and write
#ifdef WIN32
	if(!readonly)
	{
		whnd=CreateFile(dbpath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
		if(whnd==INVALID_FILE_VALUE)
		{
			UnlockWrite();
			return -1;
		}
	}
	#ifdef NO_OVL_READ_DETAILS
	rhnd=CreateFile(dbpath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
	#else
	rhnd=CreateFile(dbpath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
	#endif
	if(rhnd==INVALID_FILE_VALUE)
	{
		CloseHandle(whnd); whnd=INVALID_FILE_VALUE;
		UnlockWrite();
		return -1;
	}
	fsize.LowPart=GetFileSize(rhnd,(LPDWORD)&fsize.HighPart);
	char emsg[256]={0};
	sprintf(emsg,"Opening %s (%s bytes)...",dbpath,OrderDB::SizeStr(fsize.QuadPart,true));
	pnotify->VSDFEvent(0,emsg);
	// Scan existing records
	//OVERLAPPED ovl;
	//memset(&ovl,0,sizeof(OVERLAPPED));
	//for(fend.QuadPart=0;fend.QuadPart<fsize.QuadPart;)
	//{
	//}
	// Sometimes if we abort early (like under debugger), the unused data at the
	// end of the file doesn't get removed, so fend=fsize isn't correct. Scan backwards
	// from the end to find the real end of data
	fend.QuadPart=fsize.QuadPart;

	// Blank data may be valid for ClServer msgs, sanity check
	if(proto==19) //PROTO_CLBACKUP)
	{
		if(whnd!=INVALID_FILE_VALUE)
		{
			LARGE_INTEGER dend;
			dend.QuadPart=0;
			SetFilePointer(whnd,dend.LowPart,(PLONG)&dend.HighPart,FILE_BEGIN);
			MSGHEADER MsgHeader;
			DWORD rbytes=0;
			while(ReadFile(whnd,&MsgHeader,sizeof(MSGHEADER),&rbytes,0) && rbytes)
			{
				if(MsgHeader.MsgID==0)
					break;
				dend.QuadPart+=(sizeof(MSGHEADER)+MsgHeader.MsgLen);
				SetFilePointer(whnd,dend.LowPart,(PLONG)&dend.HighPart,FILE_BEGIN);
			}
			if(fend.QuadPart!=dend.QuadPart)
			{
				char emsg[256]={0};
				sprintf(emsg,"!!!Possible data corruption opening %s. Reload from ClServer (%I64d,%I64d).",dbpath,fend.QuadPart,dend.QuadPart);
				pnotify->VSDFError(0,emsg);
			}
		}
	}
	else
	{
		char rbuf[4096]={0};
		LARGE_INTEGER dend;
		if(rend.QuadPart%sizeof(rbuf))
			dend.QuadPart=(fend.QuadPart/sizeof(rbuf))*sizeof(rbuf);
		else
			dend.QuadPart=fend.QuadPart -sizeof(rbuf); 
		for(;dend.QuadPart>=0;dend.QuadPart-=sizeof(rbuf))
		{
			SetFilePointer(whnd,dend.LowPart,(PLONG)&dend.HighPart,FILE_BEGIN);
			DWORD rbytes=0;
			if(!ReadFile(whnd,rbuf,sizeof(rbuf),&rbytes,0)||(rbytes<1))
				break;
			const char *rptr;
			for(rptr=rbuf +rbytes;(rptr>rbuf)&&(rptr[-1]==0);rptr--)
				;
			// All empty
			if(rptr<=rbuf)
				fend.QuadPart-=rbytes;
			// Some empty
			else if(rptr<rbuf +rbytes)
			{
				fend.QuadPart-=(rbuf +rbytes -rptr);
				break;
			}
			// No empty padding
			else
				break;
		}
	}
	rend.QuadPart=fend.QuadPart;
	aend.QuadPart=0;
	if(!readonly)
	{
		// Pre-grow in 8MB chunks
		for(;fsize.QuadPart<=fend.QuadPart;fsize.QuadPart+=DETGROWSIZE)
			;
		GrowFile(fsize.QuadPart);
	}
#else//!WIN32
	if(!readonly)
	{
		whnd=fopen(dbpath,"a+b");
		if(whnd==INVALID_FILE_VALUE)
		{
			UnlockWrite();
			return -1;
		}
	}
	rhnd=fopen(dbpath,"rb");
	if(!rhnd)
	{
		if(whnd)
		{
			fclose(whnd); whnd=0;
		}
		UnlockWrite();
		return -1;
	}
	fseeko(rhnd,0,SEEK_END);
	fsize=ftello(rhnd);
	fseeko(rhnd,0,SEEK_SET);
	char emsg[256]={0};
	sprintf(emsg,"Opening %s (%s bytes)...",dbpath,OrderDB::SizeStr(fsize,true));
	pnotify->VSDFEvent(0,emsg);
	// Scan existing records
	//OVERLAPPED ovl;
	//memset(&ovl,0,sizeof(OVERLAPPED));
	//for(fend=0;fend<fsize;)
	//{
	//}
	// Sometimes if we abort early (like under debugger), the unused data at the
	// end of the file doesn't get removed, so fend=fsize isn't correct. Scan backwards
	// from the end to find the real end of data
	fend=fsize;
	char rbuf[4096]={0};
	LONGLONG dend;
	if(rend%sizeof(rbuf))
		dend=(fend/sizeof(rbuf))*sizeof(rbuf);
	else
		dend=fend -sizeof(rbuf);
	if(whnd!=INVALID_FILE_VALUE)
	{
		for(;dend>=0;dend-=sizeof(rbuf))
		{
			fseeko(whnd,dend,SEEK_SET);
			int rbytes=(int)fread(rbuf,1,sizeof(rbuf),whnd);
			if(rbytes<1)
				break;
			const char *rptr;
			for(rptr=rbuf +rbytes;(rptr>rbuf)&&(rptr[-1]==0);rptr--)
				;
			// All empty
			if(rptr<=rbuf)
				fend-=rbytes;
			// Some empty
			else if(rptr<rbuf +rbytes)
			{
				fend-=(rbuf +rbytes -rptr);
				break;
			}
			// No empty padding
			else
				break;
		}
	}
	rend=fend;
	aend=0;
	if(!readonly)
	{
		// Pre-grow in 8MB chunks
		for(;fsize<=fend;fsize+=DETGROWSIZE)
			;
		GrowFile(fsize);
	}
#endif//!WIN32
	UnlockWrite();
	return 0;
}
void VSDetailFile::Shutdown()
{
	LockWrite();
	LockRead();
#ifdef WIN32
	if(rhnd!=INVALID_FILE_VALUE)
	{
		CloseHandle(rhnd); rhnd=INVALID_FILE_VALUE;
	}
	if(whnd!=INVALID_FILE_VALUE)
	{
		char emsg[256]={0};
		sprintf(emsg,"Closing %s (%s bytes)...",fpath.c_str(),OrderDB::SizeStr(fend.QuadPart,true));
		pnotify->VSDFEvent(0,emsg);
		// Close the data file and truncate to used data only
		DWORD dwPtr=SetFilePointer(whnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
		SetEndOfFile(whnd);
		CloseHandle(whnd); whnd=INVALID_FILE_VALUE;
	}
	fend.QuadPart=fsize.QuadPart=rend.QuadPart=aend.QuadPart=0;
#else
	if(rhnd!=INVALID_FILE_VALUE)
	{
		fclose(rhnd); rhnd=0;
	}
	if(whnd!=INVALID_FILE_VALUE)
	{
		char emsg[256]={0};
		sprintf(emsg,"Closing %s (%s bytes)...",fpath.c_str(),OrderDB::SizeStr(fend,true));
		pnotify->VSDFEvent(0,emsg);
		// Close the data file and truncate to used data only
		//ftruncate(whnd->_file,fend);
		fclose(whnd); whnd=0;
	}
	fend=fsize=rend=aend=0;
#endif
	fpath=""; prefix=""; wsdate=0; readonly=false;
	pnotify=0;
	UnlockRead();
	UnlockWrite();
}
int VSDetailFile::GrowFile(LONGLONG minSize)
{
	if(minSize<0)
		return -1;
	LockWrite();
	if(whnd==INVALID_FILE_VALUE)
	{
		UnlockWrite();
		return -1;
	}
#ifdef WIN32
	LARGE_INTEGER nsize;
	for(nsize.QuadPart=0;nsize.QuadPart<minSize;nsize.QuadPart+=DBGROWSIZE)
		;
	SetFilePointer(whnd,nsize.LowPart,(PLONG)&nsize.HighPart,FILE_BEGIN);
	SetEndOfFile(whnd);
	fsize.QuadPart=nsize.QuadPart;
#else
	LONGLONG nsize;
	for(nsize=0;nsize<minSize;nsize+=DBGROWSIZE)
		;
	//ftruncate(whnd->_file,nsize);
	fsize=nsize;
#endif
	UnlockWrite();
	return 0;
}

void OrderDB::VSDFError(int rc, const char *emsg)
{
	if(pnotify) pnotify->ODBError(rc,emsg);
}
void OrderDB::VSDFEvent(int rc, const char *emsg)
{
	if(pnotify) pnotify->ODBEvent(rc,emsg);
}
#ifdef MULTI_DAY_HIST
VSDetailFile *OrderDB::OpenDetailFile(const char *prefix, int portno, int proto, const char *dpath, bool readOnly, int wsdate)
#else
VSDetailFile *OrderDB::OpenDetailFile(const char *prefix, int portno, int proto, const char *dpath, bool readOnly)
#endif
{
	Lock();
#ifdef MULTI_DAY_HIST
	char dkey[64]={0};
	sprintf(dkey,"%s,%08d",prefix,wsdate);
	DETAILFILEMAP::iterator dit=dfmap.find(dkey);
#else
	DETAILFILEMAP::iterator dit=dfmap.find(prefix);
#endif
	if(dit!=dfmap.end())
	{
		VSDetailFile *dfile=dit->second;
		Unlock();
		return dfile;
	}
	VSDetailFile *dfile=new VSDetailFile;
	if(!dfile)
	{
		Unlock();
		return 0;
	}
	char tpath[MAX_PATH]={0};
	#ifdef WIN32
	if((proto==0x0C)||(proto==0x11)||(proto==0x13))
		sprintf(tpath,"%s\\%s_%08d.msg",dpath,prefix,wsdate);
#ifdef IQSMP
	else if((proto==0x16)||(proto==0x17)||(proto==0x18)||
			(proto==0x1A)||(proto==0x1B)||(proto==0x1C))
		sprintf(tpath,"%s\\%s_%08d.dat",dpath,prefix,wsdate);
	else if((proto==0x19)||(proto==0x1D))
		sprintf(tpath,"%s\\%s_%08d.txt",dpath,prefix,wsdate);
#endif
#ifdef AWAY_FIXSERVER
	else if(proto==0x24)
		sprintf(tpath,"%s\\%s_%08d.msg",dpath,prefix,wsdate);
#endif
	else if(proto==0x26)
		sprintf(tpath,"%s\\%s_%08d.csv",dpath,prefix,wsdate);
	else
		sprintf(tpath,"%s\\%s_%08d.fix",dpath,prefix,wsdate);
	#else
	if(proto==0x0C)
		sprintf(tpath,"%s/%s_%08d.msg",dpath,prefix,wsdate);
	else
		sprintf(tpath,"%s/%s_%08d.fix",dpath,prefix,wsdate);
	#endif
	if(dfile->Init(this,tpath,prefix,wsdate,readonly||readOnly,portno,proto)<0)
	{
		char emsg[256]={0};
		sprintf(emsg,"Error Initializing detail file %s,%s,%d,%d",tpath,prefix,wsdate,readonly);
		pnotify->ODBError(0,emsg);
		delete dfile;
		Unlock();
		return 0;
	}
#ifdef MULTI_DAY_HIST
	dfmap[dkey]=dfile;
#else
	dfmap[prefix]=dfile;
#endif
	Unlock();
	return dfile;
}
#ifdef MULTI_DAY_HIST
VSDetailFile *OrderDB::GetDetailFile(const char *prefix, int wsdate)
{
	char dkey[64]={0};
	sprintf(dkey,"%s,%08d",prefix,wsdate);
	DETAILFILEMAP::iterator dit=dfmap.find(dkey);
	if(dit==dfmap.end())
		return 0;
	return dit->second;
}
VSDetailFile *OrderDB::GetDetailFile(int UscPortNo, int wsdate)
{
	for(DETAILFILEMAP::iterator dit=dfmap.begin();dit!=dfmap.end();dit++)
	{
		VSDetailFile *dfile=dit->second;
		if((dfile->portno==UscPortNo)&&(dfile->wsdate==wsdate))
			return dfile;
	}
	return 0;
}
//int OrderDB::DetailFileCount(int wsdate)
//{
//	int cnt=0;
//	for(DETAILFILEMAP::iterator dit=dfmap.begin();dit!=dfmap.end();dit++)
//	{
//		VSDetailFile *dfile=dit->second;
//		if(dfile->wsdate==wsdate)
//			cnt++;
//	}
//	return cnt;
//}
#else
VSDetailFile *OrderDB::GetDetailFile(const char *prefix)
{
	DETAILFILEMAP::iterator dit=dfmap.find(prefix);
	if(dit==dfmap.end())
		return 0;
	return dit->second;
}
#endif
int OrderDB::CloseDetailFile(VSDetailFile *dfile)
{
	if(!dfile)
		return -1;
	Lock();
	DETAILFILEMAP::iterator dit=dfmap.find(dfile->prefix);
	if(dit!=dfmap.end())
		dfmap.erase(dit);
	Unlock();

	dfile->Shutdown();
	if(dfile->rthnd)
	{
	#ifdef WIN32
		#ifdef _DEBUG
		WaitForSingleObject(dfile->rthnd,INFINITE);
		#else
		WaitForSingleObject(dfile->rthnd,3000);
		#endif
		CloseHandle(dfile->rthnd); dfile->rthnd=0;
	#else
		#ifdef _DEBUG
		WaitForSingleObject((HANDLE)(LONGLONG)dfile->rthnd,INFINITE);
		#else
		WaitForSingleObject((HANDLE)(LONGLONG)dfile->rthnd,3000);
		#endif
		dfile->rthnd=0;
	#endif
	}
	delete dfile;
	return 0;
}
int OrderDB::WriteDetailBlock(VSDetailFile *dfile, const char *bptr, int blen, LONGLONG& offset)
{
	if((!dfile)||(!bptr)||(blen<1))
		return -1;
	dfile->LockWrite();
#ifdef WIN32
	offset=dfile->fend.QuadPart; 
	dfile->fend.QuadPart+=blen;
	LARGE_INTEGER loff;
	loff.QuadPart=offset;
	if(dfile->fend.QuadPart>dfile->fsize.QuadPart)
	{
		// Growing the data file by large blocks increases write throughput
		if(dfile->GrowFile(dfile->fend.QuadPart)<0)
		{
			char emsg[256]={0};
			sprintf(emsg,"Failed growing data file to %s bytes!",SizeStr(dfile->fend.QuadPart,true));
			dfile->UnlockWrite();
			pnotify->ODBError(-1,emsg);
			return 0;
		}
	}
	DWORD wbytes=0;
	SetFilePointer(dfile->whnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
	int rc=WriteFile(dfile->whnd,bptr,blen,&wbytes,0);
	if((rc<0)||(wbytes!=blen))
	{
		_ASSERT(false);//untested
		char emsg[256]={0};
		sprintf(emsg,"Failed write on detail at offset %I64d!",offset);
		dfile->UnlockWrite();
		pnotify->ODBError(-1,emsg);
		return -1;
	}

#else
	offset=dfile->fend; dfile->fend+=blen;
	LONGLONG loff=offset;
	if(dfile->fend>dfile->fsize)
	{
		// Growing the data file by large blocks increases write throughput
		if(dfile->GrowFile(dfile->fend)<0)
		{
			char emsg[256]={0};
			sprintf(emsg,"Failed growing data file to %s bytes!",SizeStr(dfile->fend,true));
			dfile->UnlockWrite();
			pnotify->ODBError(-1,emsg);
			return 0;
		}
	}
	fseeko(dfile->whnd,loff,SEEK_SET);
	int wbytes=(int)fwrite(bptr,1,blen,dfile->whnd);
	if(wbytes!=blen)
	{
		_ASSERT(false);//untested
		char emsg[256]={0};
		sprintf(emsg,"Failed write on detail at offset %lld!",offset);
		dfile->UnlockWrite();
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	fflush(dfile->whnd);
#endif
	dfile->UnlockWrite();
	SetZdirty();
	return 0;
}
int OrderDB::ReadDetail(VSDetailFile *dfile, LONGLONG offset, char *fptr, int flen)
{
	if(offset<0)
		return -1;
	dfile->LockRead();
	// Delayed-open for historic instances
	if((dfile->rhnd==INVALID_FILE_VALUE)&&(dfile->readonly))
	{
		#ifdef NO_OVL_READ_DETAILS
		dfile->rhnd=CreateFile(dfile->fpath.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
		#else
		dfile->rhnd=CreateFile(dfile->fpath.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
		#endif
	}
	if(dfile->rhnd==INVALID_FILE_VALUE)
	{
		dfile->UnlockRead();
		return -1;
	}
#ifdef WIN32
	#ifdef NO_OVL_READ_DETAILS
	LARGE_INTEGER loff;
	loff.QuadPart=offset;
	SetFilePointer(dfile->rhnd,loff.LowPart,(PLONG)&loff.HighPart,FILE_BEGIN);
	DWORD rbytes=0;
	int rc=ReadFile(dfile->rhnd,fptr,flen,&rbytes,0);
	if(rc<0)
	{
		char emsg[256]={0};
		sprintf(emsg,"Failed ReadDetail(%s) at offset %I64d!",dfile->prefix.c_str(),loff.QuadPart);
		dfile->UnlockRead();
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	#else//!NO_OVL_READ_DETAILS
	LARGE_INTEGER loff;
	loff.QuadPart=offset;
	OVERLAPPED ovl;
	memset(&ovl,0,sizeof(OVERLAPPED));
	ovl.Offset=loff.LowPart;
	ovl.OffsetHigh=loff.HighPart;
	DWORD rbytes=0;
	int rc=ReadFile(dfile->rhnd,fptr,flen,&rbytes,&ovl);
	int err=GetLastError();
	if((!rc)&&(err!=ERROR_IO_PENDING))  // ReadFile return TRUE on success and FALSE on failure
	{
		char emsg[256]={0};
		sprintf(emsg,"Failed(%d) ReadDetail(%s) at offset %I64d!",err,dfile->prefix.c_str(),loff.QuadPart);
		dfile->UnlockRead();
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	//// Don't wait forever for the read
	//DWORD wstart=GetTickCount(),tnow=wstart;
	//while(tnow -wstart<=1000)
	//{
	//	GetOverlappedResult(dfile->rhnd,&ovl,&rbytes,false);
	//	if(rbytes>0)
	//		break;
	//	SleepEx(1,true);
	//	tnow=GetTickCount();
	//	if(tnow -wstart>=1000)
	//	{
	//		char emsg[256]={0};
	//		sprintf(emsg,"Timeout ReadDetail(%s) at offset %I64d!",dfile->prefix.c_str(),loff.QuadPart);
	//		dfile->UnlockRead();
	//		pnotify->ODBError(-1,emsg);
	//		return -1;
	//	}
	//}
	GetOverlappedResult(dfile->rhnd,&ovl,&rbytes,true);
	#endif//!NO_OVL_READ_DETAILS
#else//!WIN32
	LONGLONG loff=offset;
	fseeko(dfile->rhnd,loff,SEEK_SET);
	if(fread(fptr,1,flen,dfile->rhnd)!=flen)
	{
		char emsg[256]={0};
		sprintf(emsg,"Failed ReadDetail(%s) at offset %lld!",dfile->prefix.c_str(),loff);
		dfile->UnlockRead();
		pnotify->ODBError(-1,emsg);
		return -1;
	}
#endif//!WIN32
	dfile->UnlockRead();
	return rbytes;
}
int OrderDB::OpenZmapFile()
{
	Lock();
	if(zfp)
	{
		Unlock();
		return -1;
	}
	char dbdir[MAX_PATH]={0},*dptr=0;
	GetFullPathNameEx(fpath.c_str(),MAX_PATH,dbdir,&dptr);
	if(dptr) *dptr=0;
	sprintf(zpath,"%sVSDB_%08d.zmap",dbdir,wsdate);
	zfp=fopen(zpath,readonly?"rt":"r+t");
	if(!zfp)
		zfp=fopen(zpath,"wt");
	if(!zfp)
	{
		Unlock();
		char emsg[256]={0};
		sprintf(emsg,"Failed opening %s!",zpath);
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	char rbuf[1024]={0};
	int line=0;
	while(fgets(rbuf,sizeof(rbuf),zfp))
	{
		line++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!strncmp(rptr,"//",2))||(*rptr=='\r')||(*rptr=='\n'))
			continue;
		const char *prefix=0;
		int fdate=0;
		LONGLONG wpos=0,rpos=0,apos=0;
		int col=0;
		for(const char *tok=strtoke(rptr,",\r\n");tok;tok=strtoke(0,",\r\n"))
		{
			switch(++col)
			{
		#ifdef MULTI_DAY_HIST
			case 1: prefix=tok; break;
			case 2: fdate=atoi(tok); break;
			case 3: rpos=_atoi64(tok); break;
			case 4: wpos=_atoi64(tok); break;
			case 5: apos=_atoi64(tok); break;
		#else
			case 1: prefix=tok; break;
			case 2: rpos=_atoi64(tok); break;
			case 3: wpos=_atoi64(tok); break;
			case 4: apos=_atoi64(tok); break;
		#endif
			};
		}
		VSDetailFile *dfile=0;
		#ifdef MULTI_DAY_HIST
		char dkey[64]={0};
		sprintf(dkey,"%s,%08d",prefix,fdate);
		DETAILFILEMAP::iterator dit=dfmap.find(dkey);
		#else
		DETAILFILEMAP::iterator dit=dfmap.find(prefix);
		#endif
		if(dit==dfmap.end())
		{
			char emsg[256]={0};
			sprintf(emsg,"Zmap detail file(%s) not found",prefix);
			pnotify->ODBError(0,emsg);
			continue;
		}
		else
			dfile=dit->second;
		dfile->LockWrite();
		dfile->LockRead();
	#ifdef WIN32
		if(rpos>dfile->fend.QuadPart)
		{
			char emsg[256]={0};
			sprintf(emsg,"Zmap reported file(%s) has been read for %I64d bytes, but is only %I64d bytes!",
				prefix,rpos,dfile->fend.QuadPart);
			pnotify->ODBError(0,emsg);
			dfile->UnlockRead();
			dfile->UnlockWrite();
			Unlock();
			return -1;
		}
		// Start from last acked offset instead of last read offset
		dfile->rend.QuadPart=apos;//rpos;
		dfile->aend.QuadPart=apos;
		if(dfile->fend.QuadPart<wpos)
		{
			char emsg[256]={0};
			sprintf(emsg,"Zmap reported file(%s) was written for %I64d bytes, but is only %I64d bytes!",
				prefix,wpos,dfile->fend.QuadPart);
			pnotify->ODBError(0,emsg);
			dfile->UnlockRead();
			dfile->UnlockWrite();
			Unlock();
			return -1;
		}
	#else//!WIN32
		if(rpos>dfile->fend.QuadPart)
		{
			char emsg[256]={0};
			sprintf(emsg,"Zmap reported file(%s) has been read for %lld bytes, but is only %lld bytes!",
				prefix,rpos,dfile->fend);
			pnotify->ODBError(0,emsg);
			dfile->UnlockRead();
			dfile->UnlockWrite();
			Unlock();
			return -1;
		}
		// Start from last acked offset instead of last read offset
		dfile->rend=apos;//rpos;
		dfile->aend=apos;
		if(dfile->fend<wpos)
		{
			char emsg[256]={0};
			sprintf(emsg,"Zmap reported file(%s) was written for %lld bytes, but is only %lld bytes!",
				prefix,wpos,dfile->fend);
			pnotify->ODBError(0,emsg);
			dfile->UnlockRead();
			dfile->UnlockWrite();
			Unlock();
			return -1;
		}
	#endif//!WIN32
		dfile->UnlockRead();
		dfile->UnlockWrite();
	}
	Unlock();
	return 0;
}
int OrderDB::UpdateZmapFile(bool zmapReset)
{
	Lock();
	if(!zmapReset)
	{
		if((!zdirty)||(!zpath[0]))
		{
			Unlock();
			return 0;
		}
	}
	zdirty=false;
	freopen(zpath,"wt",zfp);
	fprintf(zfp,"//prefix,date,read,write,ack,\n");
	for(DETAILFILEMAP::iterator dit=dfmap.begin();dit!=dfmap.end();dit++)
	{
		VSDetailFile *dfile=dit->second;
	#ifdef WIN32
		// 'rend' and 'wend' are strictly increasing, so we don't really need to
		// lock out the read threads to save the most-current value.
		//dfile->LockWrite();
		LONGLONG wend=dfile->fend.QuadPart;
		//dfile->UnlockWrite();
		//dfile->LockRead();
		LONGLONG rend=dfile->rend.QuadPart;
		LONGLONG aend=dfile->aend.QuadPart;
		//dfile->UnlockRead();
		if(zmapReset)
		{
			char emsg[256]={0};
			sprintf(emsg,"Resetting zmap entry (for HUB) for %s to 0",dit->first.c_str());
			pnotify->ODBEvent(0,emsg);
		}
		fprintf(zfp,"%s,%I64d,%I64d,%I64d,\n",dit->first.c_str(),rend,wend,zmapReset ? 0 : aend);
	#else
		LONGLONG wend=dfile->fend;
		LONGLONG rend=dfile->rend;
		LONGLONG aend=dfile->aend;
		fprintf(zfp,"%s,%lld,%lld,%lld,\n",dit->first.c_str(),rend,wend,aend);
	#endif	
	}
	fflush(zfp);
	Unlock();
	return 0;
}
int OrderDB::CloseZmapFile()
{
	Lock();
	if(zfp)
	{
		UpdateZmapFile();
		fclose(zfp); zfp=0;
	}
	Unlock();
	return 0;
}

#ifdef MULTI_DAY_HIST
int OrderDB::OpenExtFile(int wsdate, bool delayInit)
#else
int OrderDB::OpenExtFile()
#endif
{
	Lock();
	if(efp)
	{
		Unlock();
		return -1;
	}
	char dbdir[MAX_PATH]={0},*dptr=0;
	GetFullPathNameEx(fpath.c_str(),MAX_PATH,dbdir,&dptr);
	if(dptr) *dptr=0;
	sprintf(epath,"%sVSDB_%08d.ext",dbdir,wsdate);
	efp=fopen(epath,readonly?"rt":"r+t");
	if((!efp)&&(!readonly))
		efp=fopen(epath,"wt");
	if(!efp)
	{
		Unlock();
		char emsg[256]={0};
		sprintf(emsg,"Failed opening %s!",epath);
		pnotify->ODBError(-1,emsg);
		return -1;
	}
	char rbuf[1024]={0};
	int line=0;
	while(fgets(rbuf,sizeof(rbuf),efp))
	{
		line++;
		char *rptr;
		for(rptr=rbuf;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!strncmp(rptr,"//",2))||(*rptr=='\r')||(*rptr=='\n'))
			continue;
		const char *prefix=0;
		int UscPortNo=-1,proto=0,clientdate=wsdate;
		const char *edir=0;
		int col=0;
		for(const char *tok=strtoke(rptr,",\r\n");tok;tok=strtoke(0,",\r\n"))
		{
			switch(++col)
			{
			case 1: prefix=tok; break;
			case 2: UscPortNo=atoi(tok); break;
			case 3: edir=tok; break;
			case 4: proto=atoi(tok); break;
			case 5: clientdate=atoi(tok); break;
			};
		}
		VSDetailFile *dfile=0;
		DETAILFILEMAP::iterator dit=dfmap.find(prefix);
		if(dit==dfmap.end())
		{
		#ifdef MULTI_DAY_HIST
			dfile=OpenDetailFile(prefix,UscPortNo,proto,edir,true,clientdate);
			if(!dfile)
			{
				char emsg[256]={0};
				sprintf(emsg,"Ext detail file(%s,%08d) not found",prefix,wsdate);
				pnotify->ODBError(0,emsg);
				continue;
			}
			// Close the read file handle to save file handles
			if(delayInit)
			{
				CloseHandle(dfile->rhnd); dfile->rhnd=INVALID_HANDLE_VALUE;
			}
		#else
			dfile=OpenDetailFile(prefix,UscPortNo,proto,edir,true);
			if(!dfile)
			{
				char emsg[256]={0};
				sprintf(emsg,"Ext detail file(%s) not found",prefix);
				pnotify->ODBError(0,emsg);
				continue;
			}
		#endif
		}
		else
			dfile=dit->second;
	}
	Unlock();
	return 0;
}
int OrderDB::UpdateExtFile()
{
	Lock();
	if((!edirty)||(!epath[0]))
	{
		Unlock();
		return 0;
	}
	edirty=false;
	freopen(epath,"wt",efp);
	fprintf(efp,"//prefix,uscportno,edir,proto,wsdate,\n");
	for(DETAILFILEMAP::iterator dit=dfmap.begin();dit!=dfmap.end();dit++)
	{
		VSDetailFile *dfile=dit->second;
		char fdir[MAX_PATH]={0},*fptr=0;
		GetFullPathNameEx(dfile->fpath.c_str(),MAX_PATH,fdir,&fptr);
		if(fptr) fptr[-1]=0;
	#ifdef MULTI_DAY_HIST
		char prefix[64]={0};
		strcpy(prefix,dit->first.c_str());
		char *dptr=strchr(prefix,',');
		if(dptr) *dptr=0;
		fprintf(efp,"%s,%d,%s,%d,%d,\n",prefix,dfile->portno,fdir,dfile->proto,dfile->wsdate);
	#else
		fprintf(efp,"%s,%d,%s,%d,\n",dit->first.c_str(),dfile->portno,fdir,dfile->proto);
	#endif
	}
	fflush(efp);
	Unlock();
	return 0;
}
int OrderDB::CloseExtFile()
{
	Lock();
	if(efp)
	{
		UpdateExtFile();
		fclose(efp); efp=0;
	}
	Unlock();
	return 0;
}

void OrderDB::CopySettings(const OrderDB& odb,const char* sname)
{
	ORDER_CACHE_SIZE=odb.ORDER_CACHE_SIZE;
	INDEX_ROOTORDERID=odb.INDEX_ROOTORDERID;
	INDEX_FIRSTCLORDID=odb.INDEX_FIRSTCLORDID;
	INDEX_CLPARENTORDERID=odb.INDEX_CLPARENTORDERID;
	INDEX_SYMBOL=odb.INDEX_SYMBOL;
	INDEX_ACCOUNT=odb.INDEX_ACCOUNT;
	INDEX_ECNORDERID=odb.INDEX_ECNORDERID;
	INDEX_CLIENTID=odb.INDEX_CLIENTID;
	INDEX_TRANSACTTIME=odb.INDEX_TRANSACTTIME;
	INDEX_FILLED_ORDERS=odb.INDEX_FILLED_ORDERS;
	INDEX_OPEN_ORDERS=odb.INDEX_OPEN_ORDERS;
	INDEX_CONNECTIONS=odb.INDEX_CONNECTIONS;
#ifdef MULTI_DAY
	INDEX_ORDERDATE=odb.INDEX_ORDERDATE;
#endif
	//DT9398
	INDEX_AUXKEYS=odb.INDEX_AUXKEYS;
	for(unsigned int i=0;i<INDEX_AUXKEYS;i++)
	{
		AUXKEY_NAMES[i]=odb.AUXKEY_NAMES[i];
		AUXKEY_TAGS[i]=odb.AUXKEY_TAGS[i];
	#ifdef AUXKEY_EXPR
		AUXKEY_ORD_EXPR[i]=odb.AUXKEY_ORD_EXPR[i];
		AUXKEY_EXEC_EXPR[i]=odb.AUXKEY_TAGS[i];
	#endif
	}
	//DT9491
	INDEX_CLORDID_CS=odb.INDEX_CLORDID_CS;
	INDEX_ROOTORDERID_CS=odb.INDEX_ROOTORDERID_CS;
	INDEX_FIRSTCLORDID_CS=odb.INDEX_FIRSTCLORDID_CS;
	INDEX_CLPARENTORDERID_CS=odb.INDEX_CLPARENTORDERID_CS;
	INDEX_SYMBOL_CS=odb.INDEX_SYMBOL_CS;
	INDEX_ACCOUNT_CS=odb.INDEX_ACCOUNT_CS;
	INDEX_ECNORDERID_CS=odb.INDEX_ECNORDERID_CS;
	INDEX_CLIENTID_CS=odb.INDEX_CLIENTID_CS;
	INDEX_CONNECTIONS_CS=odb.INDEX_CONNECTIONS_CS;
	INDEX_AUXKEYS_CS=odb.INDEX_AUXKEYS_CS;
	HAVE_FUSION_IO=odb.HAVE_FUSION_IO;
#ifdef MULTI_DAY_HIST
	HISTORIC_DAYS=odb.HISTORIC_DAYS;
#endif
}

int OrderDB::TruncateDatFile(const char *datPath)
{
	HANDLE fhnd=CreateFile(datPath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	if(fhnd==INVALID_HANDLE_VALUE)
		return -1;
	LARGE_INTEGER fsize,fend;
 	fsize.LowPart=GetFileSize(fhnd,(LPDWORD)&fsize.HighPart);
	bool isPosition=strstr(datPath,"RTPOSITION") ?true :false;
	for(fend.QuadPart=0;fend.QuadPart<fsize.QuadPart;)
	{
		VSRECHEAD header;
		memset(&header,0,sizeof(VSRECHEAD));
		SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
		DWORD rbytes=0;
		if(!ReadFile(fhnd,&header,sizeof(VSRECHEAD),&rbytes,0)||(rbytes!=sizeof(VSRECHEAD)))
			break;
		// Blank record
		if(header.marker==0)
		{
			SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
			break;
		}
		// Corrupted record
		else if(header.marker!=0xFDFDFDFD)
			return -1;
		// Real record
		// Prefix '$' to RTPOSITION app instances to resolve instance name collission
		if(isPosition)
		{
			if((header.rtype==0x01)||(header.rtype==0x04)||(header.rtype==0x05))
			{
				VSOrder tord;
				int osize=header.rlen -sizeof(VSRECHEAD);
				if(osize>VSORDER_RECSIZE)
					osize=VSORDER_RECSIZE;
				memcpy(&tord,&header,sizeof(VSRECHEAD));
				DWORD obytes=0;
				if(ReadFile(fhnd,(char*)&tord +sizeof(VSRECHEAD),osize -sizeof(VSRECHEAD),&obytes,0))
				{
					if((tord.AppInstID[0])&&(tord.AppInstID[0]!='$'))
					{
						memmove(tord.AppInstID+1,tord.AppInstID,strlen(tord.AppInstID)+1);
						tord.AppInstID[0]='$';
						LARGE_INTEGER woff;
						woff.QuadPart=fend.QuadPart +sizeof(VSRECHEAD);
						SetFilePointer(fhnd,woff.LowPart,(PLONG)&woff.HighPart,FILE_BEGIN);
						DWORD wbytes=0;
						WriteFile(fhnd,(char*)&tord +sizeof(VSRECHEAD),obytes,&wbytes,0);
					}
				}
			}
		}
		if(fend.QuadPart+header.rlen<=fsize.QuadPart)
			fend.QuadPart+=header.rlen;
		else
		{
			SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
			break;
		}
	}
	SetEndOfFile(fhnd);
	CloseHandle(fhnd);
	return 0;
}
int OrderDB::TruncateCsvFile(const char *datPath)
{
	HANDLE fhnd=CreateFile(datPath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	if(fhnd==INVALID_HANDLE_VALUE)
		return -1;
	LARGE_INTEGER fsize,fend;
 	fsize.LowPart=GetFileSize(fhnd,(LPDWORD)&fsize.HighPart);
	char rbuf[2048];
	memset(rbuf,0,sizeof(rbuf));
	for(fend.QuadPart=0;fend.QuadPart<fsize.QuadPart;)
	{
		SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
		DWORD rbytes=0;
		if(!ReadFile(fhnd,rbuf,sizeof(rbuf)-1,&rbytes,0)||(rbytes<1))
			break;
		rbuf[rbytes]=0;
		// Blank record
		if(rbuf[0]==0)
		{
			SetFilePointer(fhnd,fend.LowPart,(PLONG)&fend.HighPart,FILE_BEGIN);
			break;
		}
		// Real record
		fend.QuadPart+=strlen(rbuf);
	}
	SetEndOfFile(fhnd);
	CloseHandle(fhnd);
	return 0;
}

//#ifdef MULTI_DAY
//int VSOrder::GetLocaleOrderDate(int bias)
//{
//	if(!bias)
//		return OrderDate;
//
//	struct tm transactTime;
//	memset(&transactTime,0,sizeof(transactTime));
//	int tmpDate=(int)(TransactTime/1000000);
//	int tmpTime=(int)(TransactTime%1000000);
//	transactTime.tm_year=(tmpDate/10000) -1900;
//	transactTime.tm_mon=((tmpDate%10000)/100) -1;
//	transactTime.tm_mday=tmpDate%100;
//	transactTime.tm_hour=tmpTime/10000;
//	transactTime.tm_min=(tmpTime%10000)/100;
//	transactTime.tm_sec=tmpTime%100;
//	transactTime.tm_isdst=-1;
//	int numMinutes=(transactTime.tm_hour*60) +(transactTime.tm_min);
//	// When TZ behind GMT
//	if((bias>0)&&(numMinutes>=bias))
//		return OrderDate;
//	// When TZ ahead of GMT
//	if((bias<0)&&(numMinutes>=1440 +bias))
//		return OrderDate;
//
//	time_t tmpTimeT=mktime(&transactTime);
//	if(!tmpTimeT)
//		return OrderDate;
//	tmpTimeT-=bias*60;
//	transactTime=*localtime(&tmpTimeT);
//	int localeOrderDate=(transactTime.tm_year+1900)*10000 +(transactTime.tm_mon+1)*100 + (transactTime.tm_mday);
//	return localeOrderDate;
//}
//#endif
