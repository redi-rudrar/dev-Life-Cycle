
#include "stdafx.h"
#include "vsdefs.h"
#ifndef WIN32
#include "memindex.h"
#endif
#include "timeidx.h"

// Implement as thunk to hash_map for now
TimeIndex::TimeIndex()
	:hmap()
	,mmap()
	,usize(0)
{
	mutex=CreateMutex(0,false,0);
}
TimeIndex::~TimeIndex()
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
int TimeIndex::Init(bool multi)
{
	return 0;
}
void TimeIndex::Cleanup()
{
	hmap.clear();
	mmap.clear();
	usize=0;
}
// Insert-order iterator
TimeIndex::iterator::iterator()
	:first()
	,second(-1)
	,hit()
	,omap()
{
}
bool TimeIndex::iterator::operator!=(const iterator& it)
{
	return hit!=it.hit;
}
bool TimeIndex::iterator::operator==(const iterator& it)
{
	return hit==it.hit;
}
TimeIndex::iterator& TimeIndex::iterator::operator++(int)
{
	omap->Lock();
	hit++;
	if(hit==omap->hmap.end())
	{
		first=0;
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
void TimeIndex::iterator::clear()
{
	if(omap)
		omap->Lock();
	first=0;
	second=-1;
	if(omap)
		hit=omap->hmap.end();
	if(omap)
		omap->Unlock();
}
TimeIndex::iterator TimeIndex::find(const LONGLONG& okey)
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
TimeIndex::iterator TimeIndex::begin()
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
TimeIndex::iterator TimeIndex::end()
{
	iterator it;
	it.omap=this;
	Lock();
	it.hit=hmap.end();
	Unlock();
	return it;
}
//TimeIndex::iterator TimeIndex::erase(iterator& it)
//{
//	if(it.omap==this)
//	{
//		Lock();
//		string dkey=it.first;
//		hash_map<LONGLONG,ITEMLOC>::iterator nit=it.hit; nit++;
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
void TimeIndex::erase(iterator& it)
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
size_t TimeIndex::size()
{
	return usize;
}
void TimeIndex::clear()
{
	Lock();
	hmap.clear();
	Unlock();
}
//ITEMLOC& TimeIndex::operator[](const LONGLONG& okey)
//{
//	static ITEMLOC oloc;
//	return oloc;
//}
TimeIndex::iterator TimeIndex::insert(const LONGLONG& okey, const ITEMLOC& nloc)
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
		//	it.hit=hmap.insert(pair<LONGLONG,ITEMLOC>(okey,nloc));
		hmap[okey]=nloc;
		it.hit=hmap.find(okey);
		//else
		//	it.hit=hmap.insert(it.hit,pair<LONGLONG,ITEMLOC>(okey,nloc));
	}
	if(it.hit!=hmap.end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
TimeIndex::iterator TimeIndex::update(iterator& it, const ITEMLOC& nloc)
{
	it.hit->second=nloc;
	return it;
}

// Multi-map iterator
TimeIndex::m_iterator::m_iterator()
	:first()
	,second(-1)
	,hit()
	,omap()
{
}
//TimeIndex::m_iterator::m_iterator(const iterator& it)
//{
//	omap=it.omap;
//	hit=it.hit;
//	first=it.first;
//	second=it.second;
//}
//TimeIndex::m_iterator& TimeIndex::m_iterator::operator=(const iterator& it)
//{
//	omap=it.omap;
//	hit=it.hit;
//	first=it.first;
//	second=it.second;
//	return *this;
//}
bool TimeIndex::m_iterator::operator!=(const m_iterator& it)
{
	return hit!=it.hit;
}
bool TimeIndex::m_iterator::operator==(const m_iterator& it)
{
	return hit==it.hit;
}
TimeIndex::m_iterator& TimeIndex::m_iterator::operator++(int)
{
	omap->Lock();
	hit++;
	if(hit==omap->mmap.end())
	{
		first=0;
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
void TimeIndex::m_iterator::clear()
{
	if(omap)
		omap->Lock();
	first=0;
	second=-1;
	if(omap)
		hit=omap->mmap.end();
	if(omap)
		omap->Unlock();
}
TimeIndex::m_iterator TimeIndex::m_find(const LONGLONG& okey)
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
TimeIndex::m_iterator TimeIndex::m_begin()
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
TimeIndex::m_iterator TimeIndex::m_end()
{
	m_iterator it;
	it.omap=this;
	Lock();
	it.hit=mmap.end();
	Unlock();
	return it;
}
void TimeIndex::m_erase(m_iterator& it)
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
TimeIndex::m_iterator TimeIndex::m_insert(const LONGLONG& okey, const ITEMLOC& nloc)
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
		it.hit=mmap.insert(pair<LONGLONG,ITEMLOC>(okey,nloc));
	else
		it.hit=mmap.insert(it.hit,pair<LONGLONG,ITEMLOC>(okey,nloc));
	if(it.hit!=mmap.end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
TimeIndex::m_iterator TimeIndex::m_insert(m_iterator& mit, const LONGLONG& okey, const ITEMLOC& nloc)
{
	Lock();
	m_iterator it;
	it.omap=this;
	it.hit=mmap.insert(mit.hit,pair<LONGLONG,ITEMLOC>(okey,nloc));
	if(it.hit!=mmap.end())
	{
		it.first=it.hit->first;
		it.second=it.hit->second;
	}
	Unlock();
	return it;
}
TimeIndex::m_iterator TimeIndex::m_update(m_iterator& mit, const ITEMLOC& nloc)
{
	mit.hit->second=nloc;
	return mit;
}
size_t TimeIndex::m_size()
{
	Lock();
	return mmap.size();
	Unlock();
}
