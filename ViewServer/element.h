// Modified for VS2010 conversion

#ifndef __Element_H_defined
#define __Element_H_defined

#include <stdlib.h>
#include <crtdbg.h>

#include <string>
#include <set>
#include <hash_set>
#include <list>
#include <stack>
#include <algorithm>

#define derived_cast	static_cast

class Element
{
public:
	typedef enum
	{
		BaseType = 1,
		ListType,
		SetType,
		HashType,
		StringType,
		BinaryType
	} Type;

	Element() : parentData(0)			{ }

	Element(const std::string& t) : parentData(0), tagData(t) { }

	virtual ~Element()	{}

	virtual Element* clone() const = 0;

	struct lessByTag
	{
		bool operator()(const Element* left, const Element* right) const
		{
			return ::_stricmp(left->tagData.c_str(), right->tagData.c_str()) < 0;
		}
	};

	bool operator== (const Element& e) const { return ::_stricmp(tagData.c_str(), e.tagData.c_str()) == 0; }
	bool operator== (const Element* e) const { return ::_stricmp(tagData.c_str(), e->tagData.c_str()) == 0; }

	virtual Type type()				{ return BaseType; }
	virtual size_t size()			{ return 1; }
	virtual bool empty()			{ return false; }

	virtual bool insert(Element* r)	{ return false; }

	const std::string& tag()		{ return tagData; }
	void tag(const std::string& t)	{ tagData = t; }

	Element* parent()				{ return parentData; }
	void parent(Element* e)			{ parentData = e; }

private:
	std::string tagData;

	Element* parentData;
};


class StrElem : public Element
{
public:
	StrElem()	{}

	StrElem(const std::string& t, const std::string& s = "") : Element(t), valData(s) {}

	virtual ~StrElem()	{}

	virtual StrElem* clone() const	{ return new StrElem(*this); }

	virtual Type type()				{ return StringType; }

	const std::string& val()		{ return valData; }
	void val(const std::string& v)	{ valData = v; }

private:
	std::string valData;
};
// + DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
//Code removed
class BinaryElem : public Element
{
public:

	typedef enum
	{
		BinaryTypeUndef = 0
		,BinaryTypeBigTradeRec = 1
		,BinaryTypeGBT = 2	// DT10961 DLA 20120229
	} BinaryTypeEnum;

	BinaryElem() : valData(""), len(0), freeMe(false), binaryType(BinaryTypeUndef)  {}
// - DT8105a

	BinaryElem(std::string t, void* v = 0, size_t l = 0, bool f = false,
						int y = BinaryTypeUndef) : // DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
					Element(t), valData(v), len(l), freeMe(f), binaryType(y) {}

	BinaryElem(const BinaryElem& orig) : Element(orig), len(orig.len)
	{ if (len) valData = malloc(len);
	  deepCopy(orig);
	}

	virtual ~BinaryElem()				{ if (freeMe) free(valData); }

	virtual BinaryElem* clone() const	{ return new BinaryElem(*this);	}

	BinaryElem operator= (const BinaryElem& rhs)
	{ if (freeMe && valData) { free(valData); freeMe = false; }
	  if (rhs.len) valData = malloc(rhs.len);
	  deepCopy(rhs);
	  return *this;
	}

	virtual Type type()				{ return BinaryType; }

	void* val()						{ return valData; }
	size_t valLen()					{ return len; }
	int valType()					{ return binaryType; }

private:
	void* valData;
	size_t len;
	int binaryType;
	bool freeMe;

	inline void deepCopy(const BinaryElem& other)
	{ if (valData && other.len)
	  { memcpy(valData, other.valData, other.len); freeMe = true; len = other.len; binaryType = other.binaryType;} // DT7549 DLA 20100830
	  else
	  { valData = ""; len = 0; freeMe = false; binaryType = other.binaryType; }	// DT7549 DLA 20100830
	}
};

typedef std::list<Element*> ElementList;
typedef ElementList::iterator ElementListI;
typedef ElementList::const_iterator ElementListConstI;

typedef std::set<Element*, Element::lessByTag> ElementSet;
typedef ElementSet::iterator ElementSetI;
typedef ElementSet::const_iterator ElementSetConstI;

// + DT7549 dla 20101203
namespace stdext {
inline size_t hash_value(Element* p)
{
	return stdext:: hash_value(p->tag()); 
}
};
// - DT7549 dla 20101203
typedef stdext::hash_set<Element*, stdext::hash_compare<Element*, Element::lessByTag > > ElementHash;
typedef ElementHash::iterator ElementHashI;
typedef ElementHash::const_iterator ElementHashConstI;

/****
	List - allows faster insert/append/remove of Elements, retains order of Elements received

	Methods-
	ListElem()						Default Constructor
	ListElem(const std::string& t)	Constructor with tag initialization

	ElementList& val()					Access list directly

	ElementListI first()				Iterator pointing to beginning of list
	ElementListI end()					Iterator pointing past end of list

	size_t size size()					Number of elements in the list
	bool empty()						Is the list empty?

	bool insert(Element& e)				Insert element at front of list
	bool insert(Element* e)				Insert element at front of list
	bool insert(Element* e, ElementListI where)	Insert element at this point in list,
												first() and end	() are fast

	bool append(Element& e)				Append element to end of list
	bool append(Element* e)				Append element to end of list

	bool remove(std::string& t)			Remove all elements where tag() == t
	bool remove(Element& e)				Remove all elements matching e.tag()
	bool remove(Element* e)				Remove all elements matching e->tag()
	bool remove(ElementListI where)		Remove element at location, first() and end	() are fast

	bool update(Element& e)				Update all values in list matching e.tag()
	bool update(Element* e)				Update all values in list matching e->tag()
	bool update(ElementListI i, Element* e)	Update element at this point in list with e values
											(including tag)

	Element* find(std::string& t)		Find 1st element in list where tag() == t
	Element* find(std::string& t, ElementListI from)
											Find next element in list where tag() == t
	Element* find(Element& e)			Find 1st element in list matching e.tag()
	Element* find(Element& e, ElementListI from)	Find next element in list matching e.tag()
	Element* find(Element* e)			Find 1st element in list matching e->tag()
	Element* find(Element* e, ElementListI from)	Find next element in list matching e->tag()

	ElementListI* findI(std::string& t)	Find 1st iterator in list where tag() == t
	ElementListI* findI(std::string& t, ElementListI from)
											Find next iterator in list where tag() == t
	ElementListI* findI(Element& e)		Find 1st iterator in list matching e.tag()
	ElementListI findI(Element& e, ElementListI from)
											Find next iterator in list matchin e.tag()
	ElementListI findI(Element* e)		Find 1st iterator in list matching e->tag()
	ElementListI findI(Element* e, ElementListI from)
											Find next iterator in list matchin e->tag()
****/
class ListElem : public Element
{
public:
	ListElem() {}

	ListElem(const std::string& t) : Element(t) {}

	virtual ~ListElem()
	{ 
		ElementListI i = first();
		while (i != end())
		{ 
			ElementListI r = i++;
			delete *r; 
		}	
	}

	ElementList& val()				{ return valData; }

	virtual ListElem* clone() const	{ return new ListElem(*this); }

	ElementListI first() 			{ return valData.begin(); }
	ElementListI end() 				{ return valData.end(); }

	virtual Type type() 			{ return ListType; }

	size_t size()					{ return valData.size(); }
	bool empty()					{ return valData.empty(); }
	void clear()					{ valData.clear(); }

	bool insert(const Element& r)	{ return insert(r, end()); }
	bool insert(const Element& r, const ElementListI where)
	{
		Element* e = r.clone();
		return e ? 
			insert(e, where) : 
			false;
	}
	bool insert(Element* e)	{ return insert(e, end()); }
	bool insert(Element* e, const ElementListI where)
	{ e->parent(this);
	  try {
	    if (where == first())  valData.push_front(e);
	    else if (where == end()) valData.push_back(e);
	    else valData.insert(where, e);
	  }
	  catch (...) { return false; }
	  return true;
	}

	bool append(const Element& e)	{ return insert(e, end()); }
	bool append(Element* e)			{ return insert(e, end()); }

	bool remove(const std::string& t) 
	{ ElementListI i = first();
	  while (i != end())
	  { if (::_stricmp((*i)->tag().c_str(), t.c_str()) == 0) 
	    { ElementListI r = i++; delete *r; valData.erase(r); }
	    else ++i; }
	  return true;
	}
	bool remove(const Element& e)	{ return remove(&e); }
	bool remove(const Element* e)
	{ ElementListI i = first();
	  while (i != end())
	  { if (**i == *e) { ElementListI r = i++; delete *r; valData.erase(r); } else ++i; }
	  return true;
	}
	bool remove(ElementListI where)
	{ delete *where;
	  if (where == first()) valData.pop_front();
	  else if (where == end()) valData.pop_back();
	  else valData.erase(where);
	  return true;
	}

	bool update(const Element& e)	{ return update(&e); }
	bool update(const Element* e)
	{ for (ElementListI i = first(); i != end(); ++i) { if (**i == *e) **i = *e; } return true; }
	bool update(ElementListI i, Element* e)	{ **i = *e; return true; }

	Element* find(const std::string& t)	{ return find(t, first()); }
	Element* find(const std::string& t, ElementListI from)
	{ for (ElementListI i = from; i != end(); ++i)
	  { if (::_stricmp((**i).tag().c_str(), t.c_str()) == 0) return *i; } return 0;
	}
	Element* find(const Element& e)	{ return find(&e, first()); }
	Element* find(const Element& e, ElementListI from)
	{ return find(&e, from); }
	Element* find(const Element* e)	{ return find(e, first()); }
	Element* find(const Element* e, ElementListI from)
	{ for (ElementListI i = from; i != end(); ++i) { if (**i == *e) return *i; } return 0; }

	ElementListI findI(const std::string& t)	{ return findI(t, first()); }
	ElementListI findI(const std::string& t, ElementListI from)
	{ for (ElementListI i = from; i != end(); ++i)
	  { if (::_stricmp((**i).tag().c_str(), t.c_str()) == 0) return i; } return end();
	}
	ElementListI findI(Element& e)	{ return findI(&e, first()); }
	ElementListI findI(Element& e, ElementListI from)
	{ return findI(&e, from); }
	ElementListI findI(Element* e)	{ return findI(e, first()); }
	ElementListI findI(Element* e, ElementListI from)
	{ for (ElementListI i = from; i != end(); ++i) { if (**i == *e) return i; } return end(); }

private:
	ElementList valData;
};

/****
	Set - allows faster lookup of Elements, guaranteed unique tags and ASCII collated sequencing

	Methods-
	SetElem()						Default Constructor
	SetElem(const std::string& t)	Constructor with tag initialization

	ElementSet& val()					Access set directly

	ElementSetI first()					Iterator pointing to beginning of set
	ElementSetI end()					Iterator pointing past end of set

	size_t size size()					Number of elements in the set
	bool empty()						Is the set empty?

	bool insert(Element& e)				Insert element at front of set
	bool insert(Element* e)				Insert element into set
	bool insert(Element* e, ElementSetI where)	Insert element at this point in set (where is ignored)

	bool append(Element& e)				Append element to end of list
	bool append(Element* e)				Append element into set (same as insert)

	bool remove(std::string& t)			Remove all elements where tag() == t
	bool remove(Element& e)				Remove all elements matching e.tag()
	bool remove(Element* e)				Remove element matching e->tag()
	bool remove(ElementSetI where)		Remove this element

	bool update(Element* e)				Update all value of element in set matching e->tag()
	bool update(ElementSetI i, Element* e)	Update this element in set with e values
											(not including tag)

	Element* find(std::string& t)		Find 1st element in set where tag() == t
	Element* find(std::string& t, ElementListI from)
											Find next element in list where tag() == t
											(from is ignored)
	Element* find(Element* e)			Find element in set matching e->tag()
	Element* find(Element* e, ElementSetI from)	Find element in set matching e->tag()
													(from is ignored)

	Element* find(std::string& t)		Find iterator in set where tag() == t
	ElementSetI findIterator(Element* e)	Find iterator in set matching e->tag()
	ElementSetI findIterator(Element* e, ElementSetI from)
											Find iterator in set matching e->tag()
											(from is ignored)
****/
class SetElem : public Element
{
public:
	SetElem() {}

	SetElem(const std::string& t) : Element(t) {}

	virtual ~SetElem()
	{ for (ElementSetI i = first(); i != end(); i++) delete *i;	return; }

	virtual SetElem* clone() const	{ return new SetElem(*this); }

	ElementSet& val()		{ return valData; }

	ElementSetI first() 	{ return valData.begin(); }
	ElementSetI end() 		{ return valData.end(); }

	virtual Type type() 	{ return SetType; }

	size_t size()			{ return valData.size(); }
	bool empty()			{ return valData.empty(); }

	bool insert(const Element& r)	{ Element* e = r.clone(); return e ? insert(e) : false; }
	bool insert(Element* e)		// DT10235 YL 20111031
	{ e->parent(this);
	  try { if (valData.insert(e).second) return true; }
	  catch (...) {}
	  return false;
	}

	bool append(const Element& e) { return insert(e); }
	bool append(Element* e) { return insert(e); }

	bool remove(std::string& t)
	{ ElementSetI i = findI(t); if (i != end()) return remove(i); else return false; }
	bool remove(Element& e)	{ remove(&e); }
	bool remove(Element* e)
	{ ElementSetI i = findI(e); if (i != end()) return remove(i); else return false; }
	bool remove(ElementSetI where)
	{ delete *where; valData.erase(where); return true;	}

	bool update(Element& e)	{ update(&e); }
	bool update(Element* e)
	{ ElementSetI i = findI(e);
	  if (i != end()) { **i = *e; return true; } else return false;
	}
	bool update(const ElementSetI i, const Element* e)	{ **i = *e; return true; }

	Element* find(const std::string& t, const void* unused = 0) const									// DT10235 YL 20111031
	{ BinaryElem e(t); ElementSetConstI i = valData.find(&e); return i == valData.end() ? 0 : *i; }		// DT10235 YL 20111031
	Element* find(Element& e, const void* unused = 0)
	{ find(&e); }
	Element* find(Element* e, const void* unused = 0)
	{ ElementSetI i = valData.find(e); return i == end() ? 0 : *i; }

	ElementSetI findI(const std::string& t, const void* unused = 0)
	{ BinaryElem e(t); return valData.find(&e); }
	ElementSetI findI(Element& e, const void* unused = 0)
	{ return valData.find(&e); }
	ElementSetI findI(Element* e, const void* unused = 0)
	{ return valData.find(e); }

	// + DT10235 YL 20111101
	void reset()
	{
		for (ElementSetI i = first(); i != end(); i++) delete *i;
		valData.clear();
	}
	// - DT10235 YL 20111101

private:
	ElementSet valData;
};

/****
	Hash Set - allows fastest lookup of Elements, guaranteed unique tags and no sequence

	Methods-
	HashElem()						Default Constructor
	HashElem(const std::string& t)	Constructor with tag initialization

	HashElem& val()					Access set directly

	ElementHashI first()				Iterator pointing to beginning of set
	ElementHashI end()					Iterator pointing past end of set

	size_t size size()					Number of elements in the set
	bool empty()						Is the set empty?

	bool insert(Element& e)				Insert element at front of set
	bool insert(Element* e)				Insert element into set
	bool insert(Element* e, ElementSetI where)	Insert element at this point in set (where is ignored)

	bool append(Element& e)				Append element to end of list
	bool append(Element* e)				Append element into set (same as insert)

	bool remove(std::string& t)			Remove all elements where tag() == t
	bool remove(Element& e)				Remove all elements matching e.tag()
	bool remove(Element* e)				Remove element matching e->tag()
	bool remove(ElementHashI where)		Remove this element

	bool update(Element* e)				Update all value of element in set matching e->tag()
	bool update(ElementHashI i, Element* e)	Update this element in set with e values
											(not including tag)

	Element* find(std::string& t)		Find 1st element in set where tag() == t
	Element* find(std::string& t, ElementListI from)
											Find next element in list where tag() == t
											(from is ignored)
	Element* find(Element* e)			Find element in set matching e->tag()
	Element* find(Element* e, ElementSetI from)	Find element in set matching e->tag()
													(from is ignored)

	Element* find(std::string& t)		Find iterator in set where tag() == t
	ElementHashI findIterator(Element* e)	Find iterator in set matching e->tag()
	ElementHashI findIterator(Element* e, ElementSetI from)
											Find iterator in set matching e->tag()
											(from is ignored)
****/
class HashElem : public Element
{
public:
	HashElem() {}

	HashElem(const std::string& t) : Element(t) {}

	virtual ~HashElem()
	{ for (ElementHashI i = first(); i != end(); i++) delete *i;	return; }

	virtual HashElem* clone() const	{ return new HashElem(*this); }

	ElementHash& val()		{ return valData; }

	ElementHashI first()	{ return valData.begin(); }
	ElementHashI end()		{ return valData.end(); }

	virtual Type type()		{ return HashType; }

	size_t size()			{ return valData.size(); }
	bool empty()			{ return valData.empty(); }

	bool insert(const Element& r)	{ Element* e = r.clone(); return e ? insert(e) : false; }
	bool insert(Element* e, const void* unused = 0)
	{ e->parent(this);
	  try { if (valData.insert(e).second) return true; }
	  catch (...) {}
	  return false;
	}

	bool append(const Element& e) { return insert(e); }
	bool append(Element* e) { return insert(e); }

	bool remove(std::string& t)
	{ ElementHashI i = findI(t); if (i != end()) return remove(i); else return false; }
	bool remove(Element& e)	{ remove(&e); }
	bool remove(Element* e)
	{ ElementHashI i = findI(e); if (i != end()) return remove(i); else return false; }
	bool remove(ElementHashI where)
	{ delete *where; valData.erase(where); return true;	}

	bool update(Element& e)	{ update(&e); }
	bool update(Element* e)
	{ ElementHashI i = findI(e);
	  if (i != end()) { **i = *e; return true; } else return false;
	}
	bool update(ElementSetI i, Element* e)	{ **i = *e; return true; }

	Element* find(std::string& t, const void* unused = 0)
	{ BinaryElem e(t); ElementHashI i = valData.find(&e); return i == end() ? 0 : *i; }
	Element* find(Element& e)	{ find(&e); }
	Element* find(Element* e, const void* unused = 0)
	{ ElementHashI i = valData.find(e); return i == end() ? 0 : *i; }

	ElementHashI findI(std::string& t, const void* unused = 0)
	{ BinaryElem e(t); return valData.find(&e); }
	ElementHashI findI(Element& e)	{  return valData.find(&e); }
	ElementHashI findI(Element* e, const void* unused = 0)
	{ return valData.find(e); }

private:
	ElementHash valData;
};

#endif
