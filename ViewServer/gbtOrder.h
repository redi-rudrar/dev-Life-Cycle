// Modified for VS2010 conversion

#ifndef __GBTOrder_H_defined
#define __GBTOrder_H_defined

// + DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
#include <sstream>	// DT10961 DLA 20120229
#include "element.h" 
#include "parser.h"
#include "orderType.h"

// + DT7549 DLA 20100910 - Pairs Trading
// ----------------------------------------------------------------------------------------
// Base class for all GBT Orders
// ----------------------------------------------------------------------------------------
class GBTOrder
{
public:
	//----
	// Constructors
	//----
	GBTOrder(OrderType::Type type) 
	{
		_type = type;
		_tagvals.tag(OrderType::GetTypeString(_type));
	}

	// Contructor that parses 2012 Msg
	GBTOrder(OrderType::Type type, const char* msg, size_t len)
	{ 
		_type = type;
		parseMsg(msg, len);
	}

	// Constructor that takes a Tag String, legTagStrMap, legBtrMap
	GBTOrder(OrderType::Type type, const std::string &TagStr, const void *btr)
		: _TagStr(TagStr)
	{
		_type = type;
		_tagvals.tag(OrderType::GetTypeString(_type));
		parseTagStr(_TagStr); 
		setBtr(btr);
	}
	//----
	// Virtual methods
	//----
	virtual ~GBTOrder() {};

	// Returns the complete tags for the message
	virtual const char* GetOrderTags()
	{
		return toTagStr();
	}
	// Parse 2012 message
	virtual bool parseMsg(const char* msg, size_t len)
	{
		if (!msg) return false;

		if (len < 0)
			len = strlen(msg); //Potentially dangerous if binary data is passed

		if(OrderType::Is(msg) == OrderType::Unknown) 
		{
			_errMsg = OrderType::GetTypeString(_type);
			_errMsg += " ERROR: Unknown message type."; 
			return false;
		}
		_msg.assign(msg, len);

		Parser parser;
		ListElem* listElem = derived_cast<ListElem*>(parser.parse(msg, len));
		if (!listElem || (listElem->type() != Element::ListType))
		{
			_errMsg = OrderType::GetTypeString(_type);
			_errMsg += " ERROR: Unable to parse message."; //Removed msg output, it doesn't really work when BTRs are present
			if(listElem)
				delete listElem;
			return false;
		}

		_tagvals = *listElem;
		listElem->clear();
		delete listElem;

		// Remove duplicates
		std::set<std::string> keyList;
		for (ElementListI i = _tagvals.first(); i != _tagvals.end();)
		{
//			StrElem* s = derived_cast<StrElem*>(*i); //No reason to cast this to a string
			if (keyList.find((*i)->tag()) != keyList.end())
			{
				// duplicate
				ElementListI dup = i;
				++i;
				_tagvals.remove(dup);
			}
			else
			{
				keyList.insert((*i)->tag());
				++i;
			}
		}
		return true;
	}

	//----
	// Utility methods
	//----
// Generate 2012 msg
	const char* toMsg()
	{
		_msg = Parser::toMsg(&_tagvals);
		return _msg.c_str();
	}

	// Get the length of the message
	size_t msgLen()	
	{
		if(_msg.empty()) //Suggest adding this to prevent a msgLen call against an unconstructed _msg
			toMsg();
		return _msg.size();
	}

	// Was there an error parsing
	bool hasError()					{ return _errMsg.empty() == false; }
// + DT8105 - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
	const char* toTagStr(Element *el)
	{
		if(el && (el->type() == Element::ListType))
			return toTagStr(derived_cast<ListElem *>(el),_TagStr);
		_TagStr.clear();
		return _TagStr.c_str();
	}
// - DT8105
	// Generate pipe delimited string of tags
	const char *toTagStr()
	{
		return toTagStr(&_tagvals, _TagStr);
	}

	bool parseTagStr(const std::string &tags)
	{
//		if(!tags.length()) //This may be valid, actually. double check it
//			return false;
		return parseTagStr(&_tagvals, tags);
	}
	bool parseTagStr(Element *el, const std::string &tags)
	{
		if(el && (el->type() == Element::ListType))
			return parseTagStr(derived_cast<ListElem*>(el),tags);
		return false;
	}
	// Parse pipe delimited string of tags
	bool parseTagStr(ListElem *listElem, const std::string &tags)
	{
		std::string errStr;
		std::string::iterator loc;
		std::string::iterator KeyStart;
		std::string::iterator KeyEnd;
		std::string::iterator ValueStart;
		std::string::iterator ValueEnd;
		std::string::size_type erroroffset=0;
		std::string IncomingTagString(tags);

		if(!IncomingTagString.length())
			return false;
		loc=IncomingTagString.begin();
		ValueEnd=ValueStart=KeyEnd=KeyStart=loc;
		bool InQuotes=false;
		while (loc<IncomingTagString.end())
		{
			if((*loc)=='\"' && (InQuotes || loc==ValueStart)) //loc==ValueStart makes it so we only accept quotes starting at the beginning of a value. Is it useful?
				InQuotes=!InQuotes;							  //the resulting errors are more similar to the original parsetags, but lets some things slide that the others don't
			if(loc<IncomingTagString.end())
			{
				if(!InQuotes && (*loc)=='=')
				{
					if(KeyStart==KeyEnd)
						ValueStart=(KeyEnd=loc)+1;
					else
					{
						errStr = "Value cannot contain '=' sign for key/val pair at offset ";
						break;
					}
				}
				else if((*loc)=='\\')
						loc++;
			}
			if(!InQuotes && ((loc+1>=IncomingTagString.end()) || ((*loc)=='|')))
			{
				ValueEnd=loc;
				if(loc<IncomingTagString.end() && (*loc)!='|')
					ValueEnd++;
					
				if (KeyStart==KeyEnd && ValueStart==ValueEnd)
				{
					errStr = "No data found for key/val pair at offset ";
					break;
				}
				else if (KeyStart==ValueStart)
				{
					errStr = "No equal sign (=) found for key/val pair at offset ";
					break;
				}
				else if (KeyStart==KeyEnd)
				{
					errStr = "No key found for key/val pair at offset ";
					break;
				}
				else if (ValueStart==ValueEnd)
				{
					errStr = "No value found for key/val pair at offset ";
					break;
				}
				else
				{
					std::string Key=IncomingTagString.substr(KeyStart-IncomingTagString.begin(),KeyEnd-KeyStart);
					std::string Value=UnescapeString(IncomingTagString.substr(ValueStart-IncomingTagString.begin(),ValueEnd-ValueStart));
					if (setValue(listElem, Key, Value) == false)
					{
						errStr = "Cannot setValue for key(" + Key + ") and value(" + Value + ") at offset ";
						break;
					}
				}
				ValueEnd=ValueStart=KeyEnd=KeyStart=loc+1;
			}
			loc++;
		}
		if(!errStr.length())
		{
			if(InQuotes) 
				errStr = "No ending '\"' found for key/val pair at offset ";
		}

		if (errStr.length())
		{
			char errAtStr[64];	errAtStr[0] = '\000';
			::_itoa(static_cast<unsigned long>(KeyStart-IncomingTagString.begin()), errAtStr, 10);

			_errMsg = errStr + errAtStr;
			return false;
		}
		else
			_errMsg = "";

		return true;
	}
	
	//----
	// Set methods
	//----
	bool setBinaryElement(const char *ElementName, const void *data,size_t datalen, int type)
	{
		return setBinaryElement(&_tagvals,ElementName,data,datalen,false,type);
	}
	//Set the BTR
	bool setBtr(const void *btr)
	{
		return setBtr(&_tagvals, btr);
	}
	//  Set a string value based on tag
	bool setValue(const std::string &tag, const std::string &value)
	{
		return setValue(&_tagvals, tag, value);
	}
	bool setValue(Element *el, const std::string &tag, const std::string &value)
	{
		if(el && (el->type() == Element::ListType))
			return setValue(derived_cast<ListElem*>(el), tag, value);
		return false;
	}
	// + DT10961 DLA 20120229
	bool setValueAsLong(const std::string& tag, long value)
	{
		std::ostringstream oss;
		oss << value;
		return setValue(tag, oss.str());
	}
	// - DT10961 DLA 20120229

	//----
	// Get methods
	//----

	OrderType::Type GetType() { return _type; }

	// Get 2012 msg
	const char* msg()
	{ 
		return _msg.empty() ? toMsg() : _msg.c_str(); 
	}
	// Current error message
	const char* errMsg()			{ return _errMsg.c_str(); }

	// Returns begin and end iterators
	ElementListConstI GetBegin()
	{ 
		return _tagvals.first();
	}
	ElementListConstI GetEnd() 
	{ 
		return _tagvals.end();
	}

	// Get the BTR
	const void *getBtr()
	{	
		return getBtr(&_tagvals);
	}
// + DT9053 - bjl - Add ECNParserOrderNo, TraderGiveup, and Liquidity to Trader 2012 message
	// Get the BTR from passed list
	const void *getBtr(ListElem* listElem)
	{
		if (listElem)
		{
			BinaryElem* binElem = derived_cast<BinaryElem*>(listElem->find("BigTradeRec"));
// + DT9243 - bjl - CLServer rejects BNPALG Orders from older Front Ends
			if(binElem && (binElem->type() == Element::BinaryType) && (binElem->valLen() == 800) /* && (binElem->valType() == BinaryElem::BinaryTypeBigTradeRec)*/ )
				return binElem->val();
// - DT9243
		}
		return NULL;
	}
// - DT9053
	Element * GetElement(const std::string &tag)
	{
		return GetElement(&_tagvals,tag);
	}
	Element * GetElement(Element *el, const std::string &tag)
	{
		if(el && (el->type() == Element::ListType))
			return GetElement(derived_cast<ListElem*>(el),tag);
		return NULL;
	}
// + DT9053 - bjl - Add ECNParserOrderNo, TraderGiveup, and Liquidity to Trader 2012 message
	Element * GetElement(ListElem *listElem, const std::string &tag)
	{
		if(!listElem)
			return NULL;
		return listElem->find(tag);
	}

	ListElem* getListElement()
	{
		return &_tagvals;
	}
// - DT9053
	ListElem* getListElement(const std::string &tag, bool create=false)
	{
		return getListElement(&_tagvals,tag,create);
	}
	// Get a string value based on tag
	const char *getValue(const std::string &tag)
	{ 
		return getValue(&_tagvals, tag);
	}
	const char *getValue(Element *el, const std::string &tag)
	{
		if(el && (el->type() == Element::ListType))
			return getValue(derived_cast<ListElem*>(el),tag);
		return "";
	}
// + DT9053 - bjl - Add ECNParserOrderNo, TraderGiveup, and Liquidity to Trader 2012 message
	// Get a string value from passed list
	const char* getValue(ListElem *listElem, const std::string &tag)
	{ 
		if (listElem)
		{
			StrElem* tv = derived_cast<StrElem*>(listElem->find(tag));
			if(tv && (tv->type() == Element::StringType))
				return tv->val().c_str();
		}
		return "";
	}
// - DT9053

	// + DT10961 DLA 20120229
	const void *getBinaryElement(const std::string &tag, const size_t size)
	{
		BinaryElem* binElem = derived_cast<BinaryElem*>(_tagvals.find(tag));
		if(binElem && (binElem->type() == Element::BinaryType) && (binElem->valLen() == size))
			return binElem->val();
		else
			return NULL;
	}
	long getValueAsLong(const std::string& tag)
	{
		const char* pVal = getValue(tag);
		if (NULL != pVal)
			return atol(pVal);
		return 0;
	}
	// - DT10961 DLA 20120229

	// + DT11594 DLA 20120626
	bool getValueAsBool(const std::string& tag)
	{
		const char* pVal = getValue(tag);
		if (NULL != pVal)
		{
			if (pVal[0] == '1')
				return true;
		}
		return false;
	}
	// - DT11594 DLA 20120626

protected:
	OrderType::Type _type;
	std::string _msg;
	ListElem _tagvals;
	std::string _TagStr;
	std::string _errMsg;

	//----
	// Utility methods
	//----

	std::string EscapeString(std::string x)
	{
		std::string::size_type loc=0;

		if((loc=x.find_first_of("({[<>]})|=\\\'\"",loc))==std::string::npos) //if there's no characters that need escaping, we won't escape it, this avoids escaping starttime/endtime and such
			return x;

		do
		{
			x.insert(loc,"\\");
			loc+=2;
		}while(((loc=x.find_first_of("({[<>]})|=\\\'\"",loc))!=std::string::npos));
		x.insert(x.begin(),'\"');
		x.append("\"");
		return x;
	}

	std::string UnescapeString(std::string x)
	{
		std::string::size_type loc=0;

		if(x.length()<2) //Should have a minimum of ""
			return x;

		if(x.at(loc)=='"' && x.at(x.length()-1)=='"')
		{
			x.erase(loc,1);
			x.erase(x.length()-1,1);
		}
		else
			return x;

		while((loc=x.find_first_of("\\",loc))!=std::string::npos)
		{
			x.erase(loc,1); //Note that this doesn't properly restore things like tabs or newlines \t \r \n, etc
			loc++;
		}
		return x;
	}

	// Generate pipe delimited string of tags
	const char* toTagStr(ListElem *listElem, std::string &tagStr)
	{
		tagStr.clear();
		for (ElementListI i = listElem->first(); i != listElem->end(); i++)
		{
			if((*i)->type()!=Element::StringType)
				continue;
			StrElem* s = derived_cast<StrElem*>(*i);
			tagStr.append(s->tag() + "=" + s->val() + "|");
		}
		if(tagStr.length())
			tagStr.erase(tagStr.end()-1); //remove trailing |
		return tagStr.c_str(); 
	}

	//----
	// Set methods
	//----

	bool setBinaryElement(ListElem* listElem, const char *ElementName, const void *data, size_t datalen, bool freeMe, int datatype)
	{
		if (listElem && ElementName && data && datalen && strlen(ElementName))
		{
			BinaryElem new_binElem(ElementName, (void *)data, datalen, freeMe, datatype);
			BinaryElem* binElem = derived_cast<BinaryElem*>(listElem->find(ElementName));
			if (binElem)
			{
				if(binElem->type() != Element::BinaryType)
					return false;
				*binElem = new_binElem; 
				return true;
			}
			else
				return listElem->insert(new_binElem);
		}
		return false;
	}
	// Set the BTR on passed list
	bool setBtr(ListElem* listElem, const void *btr)
	{
		return setBinaryElement(listElem,"BigTradeRec",btr,800,false,BinaryElem::BinaryTypeBigTradeRec);
	}

	// Set a string value from passed list
	bool setValue(ListElem *listElem, const std::string &tag, const std::string &value)
	{ 
		if (listElem && tag.length())
		{
			StrElem* tv = derived_cast<StrElem*>(listElem->find(tag));
			
			if (tv)
			{
				if(tv->type() != Element::StringType)
					return false;
				tv->val(value);
				return true;
			}
			else
			{
				StrElem new_tv(tag, value); 
				return listElem->append(new_tv);
			}
		}
		return false;
	}

	//----
	// Get methods
	//----

	// Code moved // DT9053 - bjl - Add ECNParserOrderNo, TraderGiveup, and Liquidity to Trader 2012 message

	// Function that gets the ListElem object, if it's not found and flag is set, it will create it
	ListElem* getListElement(ListElem *listElem, const std::string &tag, bool create)
	{
		if(!listElem)
			return NULL;

		Element *el = GetElement(listElem,tag);
		if(el)
		{
			if(el->type() == Element::ListType)
				return derived_cast<ListElem*>(el);
			return NULL;
		}

		if (create)
		{
			ListElem* new_listElem = new ListElem(tag);
			if(new_listElem)
			{
				if (listElem->insert(new_listElem))
					return new_listElem;
			}
		}
		return NULL;
	}
	// Code moved // DT9053 - bjl - Add ECNParserOrderNo, TraderGiveup, and Liquidity to Trader 2012 message
};
// - DT8105a
#endif
// - DT7549 DLA 20100910 - Pairs Trading