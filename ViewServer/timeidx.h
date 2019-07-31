// Order index class
#ifndef _TIMEIDX_H
#define _TIMEIDX_H

#include <map>
using namespace std;
//#include <hash_map>
//using namespace __gnu_cxx;

// We may have 8 or more indices with 200 million items per index,
// so save the minimum in each index entry.
//#ifndef ITEMLOC
//typedef LONGLONG ITEMLOC;
//#endif

// This defines the interface for a disk-based index that can contain 200 million items.
// Implement as thunk to mem-only hash_map for now. To implement multiple database indices, 
// use a separate instance of this class for each index
class TimeIndex
{
public:
	TimeIndex();
	~TimeIndex();
	int Init(bool multi);
	void Cleanup();

	// Multi-thread safe
	HANDLE mutex;
	inline int Lock()
	{
		// Until deadlocks resolved
		//return (WaitForSingleObject(mutex,INFINITE)==WAIT_OBJECT_0)?0:-1;
		return 0;
	}
	inline int Unlock()
	{
		// Until deadlocks resolved
		//return ReleaseMutex(mutex)?0:-1;
		return 0;
	}

	// Insert-order iterator
	//class m_iterator;
	class iterator
	{
	public:
		iterator();
		bool operator!=(const iterator& it);
		bool operator==(const iterator& it);
		iterator& operator++(int);
		void clear();
		LONGLONG first;
		ITEMLOC second;

	protected:
		friend class TimeIndex;
		//friend class m_iterator;
		TimeIndex *omap;
		//hash_map<LONGLONG,ITEMLOC>::iterator hit;
		map<LONGLONG,ITEMLOC>::iterator hit;
	};
	iterator find(const LONGLONG& okey);
	iterator begin();
	iterator end();
	void erase(iterator& it);
	//ITEMLOC& operator[](const LONGLONG& okey); use insert and update instead
	iterator insert(const LONGLONG& okey, const ITEMLOC& nloc);
	iterator update(iterator& it, const ITEMLOC& nloc);
	size_t size();

	// Multi-map iterator
	class m_iterator
	{
	public:
		m_iterator();
		//m_iterator(const iterator& it);
		//m_iterator& operator=(const iterator& it);
		bool operator!=(const m_iterator& mit);
		bool operator==(const m_iterator& mit);
		m_iterator& operator++(int);
		void clear();
		LONGLONG first;
		ITEMLOC second;

	protected:
		friend class TimeIndex;
		TimeIndex *omap;
		multimap<LONGLONG,ITEMLOC>::iterator hit;
	};
	m_iterator m_find(const LONGLONG& okey);
	m_iterator m_begin();
	m_iterator m_end();
	void m_erase(m_iterator& mit);
	m_iterator m_insert(const LONGLONG& okey, const ITEMLOC& nloc);
	m_iterator m_insert(m_iterator& mit, const LONGLONG& okey, const ITEMLOC& nloc);
	m_iterator m_update(m_iterator& mit, const ITEMLOC& nloc);
	size_t m_size();

	void clear();

protected:
	//hash_map<LONGLONG,ITEMLOC> hmap;
	map<LONGLONG,ITEMLOC> hmap;
	multimap<LONGLONG,ITEMLOC> mmap;
	size_t usize;
};

#endif//_TIMEIDX_H
