
#include "stdafx.h"
#include "vsdefs.h"
#include "memindex.h"

#if !defined(USE_DISK_INDEX)&&!defined(MULTI_HASH)
// Implement as thunk to hash_map for now
MemIndex::MemIndex()
	:hmap()
	,mmap()
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
	#ifdef WIN32
		CloseHandle(mutex); mutex=0;
	#else
		DeleteMutex(mutex); mutex=0;
	#endif
	}
}
int MemIndex::Init(bool multi)
{
	return 0;
}
void MemIndex::Cleanup()
{
	hmap.clear();
	mmap.clear();
	usize=0;
}
// Insert-order iterator
MemIndex::iterator::iterator()
	:first()
	,second(-1)
	,hit()
	,omap()
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
	if(hit==omap->hmap.end())
	{
		first.clear();
		second=-1;
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
		hit=omap->hmap.end();
	if(omap)
		omap->Unlock();
}
MemIndex::iterator MemIndex::find(const string& okey)
{
	Lock();
	iterator it;
	it.omap=this;
	it.hit=hmap.find(okey);
	if(it.hit!=hmap.end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}

size_t MemIndex::m_copy(const string& key,set<LONGLONG>& s)
{
        iterator it;
        it.omap=this;
        Lock();
	std::transform(
		mmap.lower_bound(key),
		mmap.upper_bound(key),
		std::inserter(s,s.begin()),
		extract_second()
	);
        Unlock();
        return 0;
}


MemIndex::iterator MemIndex::begin()
{
	iterator it;
	it.omap=this;
	Lock();
	it.hit=hmap.begin();
	if(it.hit!=hmap.end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::iterator MemIndex::end()
{
	iterator it;
	it.omap=this;
	Lock();
	it.hit=hmap.end();
	Unlock();
	return it;
}
//MemIndex::iterator MemIndex::erase(iterator& it)
//{
//	if(it.omap==this)
//	{
//		Lock();
//		string dkey=it.first;
//		hash_map<string,ITEMLOC>::iterator nit=it.hit; nit++;
//		hmap.erase(it.hit);
//		it.hit=nit;
//		if(it.hit==hmap.end())
//		{
//			if(usize>0) usize--;
//		}
//		else
//		{
//			it.first=it.hit->first;
//			it.second=it.hit->second;
//			if(it.first!=dkey)
//			{
//				if(usize>0) usize--;
//			}
//		}
//		Unlock();
//	}
//	else
//		_ASSERT(false);
//	return it;
//}
void MemIndex::erase(iterator& it)
{
	if(it.omap==this)
	{
		Lock();
		hmap.erase(it.hit);
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
	hmap.clear();
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
	it.hit=hmap.find(okey);
	if(it.hit==hmap.end()) // unique key count
	{
		usize++;
		// It's much faster to not do all this multimap iteration,
		// since this insert assumes one unique item per key
		//for(;(it.hit!=hmap.end())&&(it.hit->first==okey);it.hit++)
		//	;
		//if(it.hit==hmap.end())
		//	it.hit=hmap.insert(pair<string,ITEMLOC>(okey,nloc));
		hmap[okey]=nloc;
		it.hit=hmap.find(okey);
		//else
		//	it.hit=hmap.insert(it.hit,pair<string,ITEMLOC>(okey,nloc));
	}
	if(it.hit!=hmap.end())
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
	if(hit==omap->mmap.end())
	{
		first.clear();
		second=-1;
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
		hit=omap->mmap.end();
	if(omap)
		omap->Unlock();
}
MemIndex::m_iterator MemIndex::m_find(const string& okey)
{
	m_iterator it;
	it.omap=this;
	Lock();
	it.hit=mmap.find(okey);
	if(it.hit!=mmap.end())
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
	it.hit=mmap.begin();
	if(it.hit!=mmap.end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
MemIndex::m_iterator MemIndex::m_end()
{
	m_iterator it;
	it.omap=this;
	Lock();
	it.hit=mmap.end();
	Unlock();
	return it;
}
void MemIndex::m_erase(m_iterator& it)
{
	if(it.omap==this)
	{
		Lock();
		mmap.erase(it.hit);
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
	it.hit=mmap.find(okey);
	if(it.hit==mmap.end())
		usize++; // unique key count
	for(;(it.hit!=mmap.end())&&(it.hit->first==okey);it.hit++)
		;
	if(it.hit==mmap.end())
		it.hit=mmap.insert(pair<string,ITEMLOC>(okey,nloc));
	else
		it.hit=mmap.insert(it.hit,pair<string,ITEMLOC>(okey,nloc));
	if(it.hit!=mmap.end())
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
	it.hit=mmap.insert(mit.hit,pair<string,ITEMLOC>(okey,nloc));
	if(it.hit!=mmap.end())
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
	return mmap.size();
	Unlock();
}

#endif//!USE_DISK_INDEX && !MULTI_HASH
