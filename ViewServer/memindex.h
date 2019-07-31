// Order index class
#ifndef _MEMINDEX_H
#define _MEMINDEX_H

#include <string>
#include <map>
#include <set>
using namespace std;
#ifdef WIN32
#include <hash_map>
using namespace __gnu_cxx;
#else
#define hash_map map
#endif

#ifndef USE_DISK_INDEX
// We may have 8 or more indices with 200 million items per index,
// so save the minimum in each index entry.

#ifdef MULTI_DAY_ITEMLOC
// When loading indices for more than one date into memory, the order offset is not unique enough.
// This class encapsulates date+offset for globally unique order location.
class ITEMLOC
{
public:
	int wsdate;
	LONGLONG offset;

	ITEMLOC(){}
	ITEMLOC(const ITEMLOC& iloc){ wsdate=iloc.wsdate; offset=iloc.offset; }
	ITEMLOC(const LONGLONG& off){ wsdate=0; offset=off; }
	ITEMLOC(const int& off){ wsdate=0; offset=off; }
	ITEMLOC& operator=(const ITEMLOC& iloc)
	{
		wsdate=iloc.wsdate;
		offset=iloc.offset;
		return *this;
	}
	ITEMLOC& operator=(const LONGLONG& off)
	{
		offset=off;
		return *this;
	}
	bool operator<(const ITEMLOC& iloc)	const
	{
		if(wsdate<iloc.wsdate)
			return true;
		else if(wsdate>iloc.wsdate)
			return false;
		else
			return offset<iloc.offset;
	};
	bool operator==(const ITEMLOC& iloc) const
	{
		return (wsdate==iloc.wsdate)&&(offset==iloc.offset);
	}
	operator LONGLONG() const
	{ 
		return offset; 
	}
};
#else
typedef LONGLONG ITEMLOC;
#endif

struct extract_second
{
	template <typename T, typename U>
	const U operator() (const std::pair<T,U> &p) const
	{
		return p.second;
	}
}; 

// This defines the interface for a disk-based index that can contain 200 million items.
// Implement as thunk to mem-only hash_map for now. To implement multiple database indices, 
// use a separate instance of this class for each index
class MemIndex
{
public:
	MemIndex();
	~MemIndex();
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
		string first;
		ITEMLOC second;

	protected:
		friend class MemIndex;
		//friend class m_iterator;
		MemIndex *omap;
	#ifdef MULTI_HASH
		int aidx;
	#endif
		hash_map<string,ITEMLOC>::iterator hit;
	};
	iterator find(const string& okey);
	iterator begin();
	iterator end();
	void erase(iterator& it);
	//ITEMLOC& operator[](const string& okey); use insert and update instead
	iterator insert(const string& okey, const ITEMLOC& nloc);
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
		string first;
		ITEMLOC second;

	protected:
		friend class MemIndex;
		MemIndex *omap;
	#ifdef MULTI_HASH
		int aidx;
	#endif
		multimap<string,ITEMLOC>::iterator hit;
	};
	m_iterator m_find(const string& okey);
	m_iterator m_begin();
	m_iterator m_end();
	void m_erase(m_iterator& mit);
	m_iterator m_insert(const string& okey, const ITEMLOC& nloc);
	m_iterator m_insert(m_iterator& mit, const string& okey, const ITEMLOC& nloc);
	m_iterator m_update(m_iterator& mit, const ITEMLOC& nloc);
	size_t m_size();

	void clear();
#ifdef MULTI_HASH
	int ShowMultiHash(FILE *fp, const char *mname);
#endif

	size_t m_copy(const string& key, set<LONGLONG>& s);


protected:
#ifdef MULTI_HASH
	hash_map<string,ITEMLOC> *ahmap;
	multimap<string,ITEMLOC> *ammap;
#else
	hash_map<string,ITEMLOC> hmap;
	multimap<string,ITEMLOC> mmap;
#endif
	size_t usize;
};

#endif//!USE_DISK_INDEX
#endif//_MEMINDEX_H
