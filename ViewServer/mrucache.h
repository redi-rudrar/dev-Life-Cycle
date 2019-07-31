// Most-recently used cache for fast ORDERMAP::iterator lookups.
#ifndef _MRUCACHE_H
#define _MRUCACHE_H

#include "vsorder.h"

class VSMruCache
{
protected:
#define MRU_SIZE 4 // Tried 4, 8, and 16 and only +/-2% hit
	struct CACHEITEM
	{
		char key[64];
		VSOrder *val;
	};
	int nitems;
	CACHEITEM items[MRU_SIZE];
	
public:
	VSMruCache()
		:nitems(0)
		,nsearch(0),nhit(0),nmiss(0)
	{
		for(int i=0;i<MRU_SIZE;i++)
		{
			memset(items[i].key,0,sizeof(items[i].key));
			items[i].val=0;
		}
	}
	int AddItem(const char *key, VSOrder *val)
	{
		int klen=(int)strlen(key);
		if(klen>=64)
		{
			_ASSERT(false);
			return -1;
		}
		// If it's already in the list, move it to the bottom
		for(int i=nitems -1;i>=0;i--) // LIFO search
		{
			if(!_stricmp(items[i].key,key))
			{
				if(i<nitems -1)
				{
					CACHEITEM tci=items[i];
					for(;i<nitems -1;i++)
						items[i]=items[i+1];
					items[i]=tci;
				}
				return 0;
			}
		}
		// Newest at the bottom
		if(nitems<MRU_SIZE)
		{
			strcpy(items[nitems].key,key);
			items[nitems].val=val;
			nitems++;
			return 0;
		}
		else
		{
			// memmove isn't nice with STL
			//memmove(&items[0],&items[1],(nitems -1)*sizeof(CACHEITEM));
			for(int i=0;i<MRU_SIZE -1;i++)
				items[i]=items[i+1];
			strcpy(items[MRU_SIZE -1].key,key);
			items[MRU_SIZE -1].val=val;
			return 0;
		}
	}
	LONGLONG nsearch,nhit,nmiss;
	VSOrder *GetItem(const char *key)
	{
		nsearch++;
		for(int i=nitems -1;i>=0;i--) // LIFO search
		{
			if(!_stricmp(items[i].key,key))
			{
				nhit++;
				return items[i].val;
			}
		}
		nmiss++;
		return 0;
	}
	int RemItemByKey(const char *key)
	{
		for(int i=nitems -1;i>=0;i--) // LIFO search
		{
			if(!_stricmp(items[i].key,key))
			{
				//if(i<nitems -1)
				//	memmove(&items[i],&items[i+1],(nitems -i)*sizeof(CACHEITEM));
				for(;i<nitems -1;i++)
					items[i]=items[i+1];
				nitems--;
				memset(items[nitems].key,0,sizeof(items[nitems].key));
				items[nitems].val=0;
				return 0;
			}
		}
		return -1;
	}
	int RemItemByVal(VSOrder *val)
	{
		for(int i=nitems -1;i>=0;i--) // LIFO search
		{
			if(items[i].val==val)
			{
				//if(i<nitems -1)
				//	memmove(&items[i],&items[i+1],(nitems -i)*sizeof(CACHEITEM));
				for(;i<nitems -1;i++)
					items[i]=items[i+1];
				nitems--;
				memset(items[nitems].key,0,sizeof(items[nitems].key));
				items[nitems].val=0;
				return 0;
			}
		}
		return -1;
	}
};

#endif//_MRUCACHE_H
