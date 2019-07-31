
#include "stdafx.h"
#include "vsdefs.h"
#include "memindex.h"

#ifdef MULTI_HASH
// TODO: Test if 64 is better than 4
#define NUM_HASH_ARRAYS 64

static int HashStr(const char *str)
{
	// Copied from GNU's hash_fun.h
	unsigned long __h = 0;
	for ( ; *str; ++str)
	__h = 5*__h + *str;
	return (int)(__h%NUM_HASH_ARRAYS);
}
MemIndex::MemIndex()
	:ahmap(0)
	,ammap(0)
	,usize(0)
{
	mutex=CreateMutex(0,false,0);
}
MemIndex::~MemIndex()
{
	if(mutex)
	{
	#ifdef _DEBUG
		WaitForSingleObject(mutex,INFINITE);
	#else
		WaitForSingleObject(mutex,1000);
	#endif
		CloseHandle(mutex); mutex=0;
	}
}
int MemIndex::Init(bool multi)
{
	if(multi)
	{
		ammap=new multimap<string,ITEMLOC>[NUM_HASH_ARRAYS];
	}
	else
	{
		ahmap=new hash_map<string,ITEMLOC>[NUM_HASH_ARRAYS];
	}
	return 0;
}
void MemIndex::Cleanup()
{
	if(ahmap)
	{
		for(int a=0;a<NUM_HASH_ARRAYS;a++)
			ahmap[a].clear();
		delete[] ahmap; ahmap=0;
	}
	if(ammap)
	{
		for(int a=0;a<NUM_HASH_ARRAYS;a++)
			ammap[a].clear();
		delete[] ammap; ammap=0;
	}
	usize=0;
}
// Insert-order iterator
MemIndex::iterator::iterator()
	:first()
	,second(-1)
	,hit()
	,omap()
	,aidx(0)
{
}
bool MemIndex::iterator::operator!=(const iterator& it)
{
	return hit!=it.hit;
}
bool MemIndex::iterator::operator==(const iterator& it)
{
	return hit==it.hit;
}
MemIndex::iterator& MemIndex::iterator::operator++(int)
{
	omap->Lock();
	hit++;
	if(hit==omap->ahmap[aidx].end())
	{
		for(aidx++;aidx<NUM_HASH_ARRAYS;aidx++)
		{
			hit=omap->ahmap[aidx].begin();
			if(hit!=omap->ahmap[aidx].end())
				break;
		}
		if(aidx<NUM_HASH_ARRAYS)
		{
			first=hit->first;
			second=hit->second;
		}
		else
		{
			first.clear();
			second=-1;
			aidx=NUM_HASH_ARRAYS -1;
		}
	}
	else
	{
		first=hit->first;
		second=hit->second;
	}
	omap->Unlock();
	return *this;
}
void MemIndex::iterator::clear()
{
	if(omap)
		omap->Lock();
	first.clear();
	second=-1;
	if(omap)
		hit=omap->ahmap[NUM_HASH_ARRAYS-1].end();
	if(omap)
		omap->Unlock();
}
MemIndex::iterator MemIndex::find(const string& okey)
{
	Lock();
	iterator it;
	it.omap=this;
	it.aidx=HashStr(okey.c_str());
	it.hit=ahmap[it.aidx].find(okey);
	if(it.hit==ahmap[it.aidx].end())
		it=end();
	else
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::iterator MemIndex::begin()
{
	iterator it;
	it.omap=this;
	Lock();
	for(it.aidx=0;it.aidx<NUM_HASH_ARRAYS;it.aidx++)
	{
		it.hit=ahmap[it.aidx].begin();
		if(it.hit!=ahmap[it.aidx].end())
		{
			it.first=it.hit->first;
			it.second=it.hit->second;
			break;
		}
	}
	if(it.aidx>=NUM_HASH_ARRAYS)
		it=end();
	Unlock();
	return it;
}
MemIndex::iterator MemIndex::end()
{
	iterator it;
	it.omap=this;
	Lock();
	it.aidx=NUM_HASH_ARRAYS-1;
	it.hit=ahmap[it.aidx].end();
	Unlock();
	return it;
}
void MemIndex::erase(iterator& it)
{
	if(it.omap==this)
	{
		Lock();
		ahmap[it.aidx].erase(it.hit);
		if(usize>0) usize--;
		Unlock();
	}
	else
		_ASSERT(false);
}
size_t MemIndex::size()
{
	return usize;
}
void MemIndex::clear()
{
	Lock();
	for(int a=0;a<NUM_HASH_ARRAYS;a++)
		ahmap[a].clear();
	Unlock();
}
//ITEMLOC& MemIndex::operator[](const string& okey)
//{
//	static ITEMLOC oloc;
//	return oloc;
//}
MemIndex::iterator MemIndex::insert(const string& okey, const ITEMLOC& nloc)
{
	Lock();
	iterator it;
	it.omap=this;
	it.aidx=HashStr(okey.c_str());
	//it.hit=ahmap[it.aidx].find(okey);
	//if(it.hit==ahmap[it.aidx].end()) // unique key count
	//{
	//	usize++;
	//	// It's much faster to not do all this multimap iteration,
	//	// since this insert assumes one unique item per key
	//	//for(;(it.hit!=ahmap[it.aidx].end())&&(it.hit->first==okey);it.hit++)
	//	//	;
	//	//if(it.hit==ahmap[it.aidx].end())
	//	//	it.hit=hmap.insert(pair<string,ITEMLOC>(okey,nloc));
	//	(ahmap[it.aidx])[okey]=nloc;
	//	it.hit=ahmap[it.aidx].find(okey);
	//	//else
	//	//	it.hit=hmap.insert(it.hit,pair<string,ITEMLOC>(okey,nloc));
	//}
	//if(it.hit!=ahmap[it.aidx].end())
	//{
	//	it.first=it.hit->first;
	//	it.second=it.hit->second;
	//}
	// Doesn't check for duplicate for speed
	(ahmap[it.aidx])[okey]=nloc; usize++;
	it.hit=ahmap[it.aidx].find(okey);
	if(it.hit!=ahmap[it.aidx].end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::iterator MemIndex::update(iterator& it, const ITEMLOC& nloc)
{
	it.hit->second=nloc;
	return it;
}

// Multi-map iterator
MemIndex::m_iterator::m_iterator()
	:first()
	,second(-1)
	,hit()
	,omap()
	,aidx(0)
{
}
//MemIndex::m_iterator::m_iterator(const iterator& it)
//{
//	omap=it.omap;
//	hit=it.hit;
//	first=it.first;
//	second=it.second;
//}
//MemIndex::m_iterator& MemIndex::m_iterator::operator=(const iterator& it)
//{
//	omap=it.omap;
//	hit=it.hit;
//	first=it.first;
//	second=it.second;
//	return *this;
//}
bool MemIndex::m_iterator::operator!=(const m_iterator& it)
{
	return hit!=it.hit;
}
bool MemIndex::m_iterator::operator==(const m_iterator& it)
{
	return hit==it.hit;
}
MemIndex::m_iterator& MemIndex::m_iterator::operator++(int)
{
	omap->Lock();
	hit++;
	if(hit==omap->ammap[aidx].end())
	{
		for(aidx++;aidx<NUM_HASH_ARRAYS;aidx++)
		{
			hit=omap->ammap[aidx].begin();
			if(hit!=omap->ammap[aidx].end())
				break;
		}
		if(aidx<NUM_HASH_ARRAYS)
		{
			first=hit->first;
			second=hit->second;
		}
		else
		{
			first.clear();
			second=-1;
			aidx=NUM_HASH_ARRAYS -1;
		}
	}
	else
	{
		first=hit->first;
		second=hit->second;
	}
	omap->Unlock();
	return *this;
}
void MemIndex::m_iterator::clear()
{
	if(omap)
		omap->Lock();
	first.clear();
	second=-1;
	if(omap)
		hit=omap->ammap[NUM_HASH_ARRAYS-1].end();
	if(omap)
		omap->Unlock();
}
MemIndex::m_iterator MemIndex::m_find(const string& okey)
{
	m_iterator it;
	it.omap=this;
	Lock();
	it.aidx=HashStr(okey.c_str());
	it.hit=ammap[it.aidx].find(okey);
	if(it.hit==ammap[it.aidx].end())
		it=m_end();
	else
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::m_iterator MemIndex::m_begin()
{
	m_iterator it;
	it.omap=this;
	Lock();
	for(it.aidx=0;it.aidx<NUM_HASH_ARRAYS;it.aidx++)
	{
		it.hit=ammap[it.aidx].begin();
		if(it.hit!=ammap[it.aidx].end())
		{
			it.first=it.hit->first;
			it.second=it.hit->second;
			break;
		}
	}
	if(it.aidx>=NUM_HASH_ARRAYS)
		it=m_end();
	Unlock();
	return it;
}
MemIndex::m_iterator MemIndex::m_end()
{
	m_iterator it;
	it.omap=this;
	Lock();
	it.aidx=NUM_HASH_ARRAYS-1;
	it.hit=ammap[it.aidx].end();
	Unlock();
	return it;
}
void MemIndex::m_erase(m_iterator& it)
{
	if(it.omap==this)
	{
		Lock();
		ammap[it.aidx].erase(it.hit);
		if(usize>0) usize--;
		Unlock();
	}
	else
		_ASSERT(false);
}
MemIndex::m_iterator MemIndex::m_insert(const string& okey, const ITEMLOC& nloc)
{
	Lock();
	m_iterator it;
	it.omap=this;
	it.aidx=HashStr(okey.c_str());
	// Doesn't check for duplicate for speed
	//it.hit=ammap[it.aidx].find(okey);
	//if(it.hit==ammap[it.aidx].end())
		usize++; // unique key count
	//for(;(it.hit!=ammap[it.aidx].end())&&(it.hit->first==okey);it.hit++)
	//	;
	//if(it.hit==ammap[it.aidx].end())
		it.hit=ammap[it.aidx].insert(pair<string,ITEMLOC>(okey,nloc));
	//else
	//	it.hit=ammap[it.aidx].insert(it.hit,pair<string,ITEMLOC>(okey,nloc));
	if(it.hit!=ammap[it.aidx].end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::m_iterator MemIndex::m_insert(m_iterator& mit, const string& okey, const ITEMLOC& nloc)
{
	Lock();
	m_iterator it;
	it.omap=this;
	it.aidx=HashStr(okey.c_str());
	it.hit=ammap[it.aidx].insert(mit.hit,pair<string,ITEMLOC>(okey,nloc));
	if(it.hit!=ammap[it.aidx].end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::m_iterator MemIndex::m_update(m_iterator& mit, const ITEMLOC& nloc)
{
	mit.hit->second=nloc;
	return mit;
}
size_t MemIndex::m_size()
{
	Lock();
	size_t tsize=0;
	for(int a=0;a<NUM_HASH_ARRAYS;a++)
		tsize+=ammap[a].size();
	return tsize;
	Unlock();
}

int MemIndex::ShowMultiHash(FILE *fp, const char *mname)
{
	fprintf(fp,"\n%s MultiHash Stats\n",mname);
	if(ahmap)
	{
		int htot=0,etot=0,hmin=-1,hmax=0;
		int a;
		for(a=0;a<NUM_HASH_ARRAYS;a++)
		{
			int msize=(int)ahmap[a].size();
			htot+=msize;
			if(msize>hmax)
				hmax=msize;
			if((hmin<0)||(msize<hmin))
				hmin=msize;
			if(msize<1)
				etot++;
		}
		int havgsize=htot/(a?a:1);
		fprintf(fp,"Total %d entires, %d unique entries, avg %d over %d maps, min %d, max %d, %d emtpy maps\n",
			htot,htot,havgsize,a,hmin,hmax,etot);
	}
	if(ammap)
	{
		int htot=0,etot=0,hmin=-1,hmax=0;
		int a;
		for(a=0;a<NUM_HASH_ARRAYS;a++)
		{
			int msize=(int)ammap[a].size();
			htot+=msize;
			if(msize>hmax)
				hmax=msize;
			if((hmin<0)||(msize<hmin))
				hmin=msize;
			if(msize<1)
				etot++;
		}
		int havgsize=htot/(a?a:1);
		fprintf(fp,"Total %d entires, %d unique entries, avg %d over %d maps, min %d, max %d, %d emtpy maps\n",
			htot,usize,havgsize,a,hmin,hmax,etot);
	}
	return 0;
}

#endif//MULTI_HASH
