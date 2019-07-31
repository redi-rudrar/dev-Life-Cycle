#ifndef __Parser_H_defined
#define __Parser_H_defined

#include "element.h"

/*
Callbacks:
  Begin
  Element Start
  Element Complete
  End

[] = List
() = Set
{} = Hash
<type,len,data> = Binary 
*/

class Parser
{
public:
	typedef enum
	{
		NoError = 0,
		SomeError
	} ParseError;

	typedef enum
	{
		Undefined = 0,
		Begin = 1,
		End = 2,
		Escape = '\\',
		String = '"',
		PairElement = '=',
		BinaryElement = '<',
		ListElement = '[',
		SetElement = '(',
		HashElement = '{',
		EndElement = ',',
		EndBinaryElement = '>',
		EndListBlock = ']',
		EndSetBlock = ')',
		EndHashBlock = '}'
	} ParserState;

	Parser() : errLoc(0)		{ state.push(Begin); }

	virtual ~Parser()			{}

	Element* parse(const char* ptr, size_t len = -1)
	{
		const char* start = ptr;
		char buf[8192];
		int bufIdx = 0;
		Element* elem;
		bool quit = false;

		while (*ptr && state.size() && ((size_t) (ptr-start) < len)) // DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
		{
			ParserState s = static_cast<ParserState>(*ptr);
			if (state.top() == String)
			{
				switch (s)
				{
				case String:
					state.pop();
					break;
				case Escape:
					buf[bufIdx++] = *(++ptr);
					break;
				default:
					buf[bufIdx++] = *ptr;
					break;
				}

				ptr++;
				continue;
			}

			switch (s)
			{
			case Escape:
				buf[bufIdx++] = *(++ptr);
				break;
			case '\n':
			case '\r':
				break;
			case String:
				state.push(String);
				break;
			case PairElement:
			case BinaryElement:
			case ListElement:
			case SetElement:
			case HashElement:
				if (state.top() != String)
				{
					buf[bufIdx] = '\000'; bufIdx = 0;

					elem = factory(s, buf);
					if (elem == NULL)
					{
						//Y Error
						break;
					}

					container.push(elem);
					state.push(s);

					if (s == BinaryElement)
					{
						ptr++;
						bool bIsRootElem = false;
						elem = processBinaryElement(buf, bufIdx, ptr, elem, bIsRootElem);
						if (bIsRootElem)
							return elem;
					}

					elem = NULL;
				}
				break;
			case EndElement:
			{
				bool bIsRootElem = false;
				Element* pElem = endElement(buf, bufIdx, bIsRootElem);
				if (bIsRootElem)
					return pElem;
			}
			break;
			case EndBinaryElement:
				break;
			case EndListBlock:
			{
				bool bIsRootElem = false;
				Element* pElem = endContainer(buf, bufIdx, bIsRootElem, Element::ListType);
				if (bIsRootElem)
					return pElem;
			}
			break;
			case EndSetBlock:
			{
				bool bIsRootElem = false;
				Element* pElem = endContainer(buf, bufIdx, bIsRootElem, Element::SetType);
				if (bIsRootElem)
					return pElem;
			}
			break;
			case EndHashBlock:
			{
				bool bIsRootElem = false;
				Element* pElem = endContainer(buf, bufIdx, bIsRootElem, Element::HashType);
				if (bIsRootElem)
					return pElem;
			}
			break;
			default:
				buf[bufIdx++] = *ptr;
				break;
			}

			ptr++;
		}

		return 0;
	}

	std::string errStr;

	static std::string toMsg(Element* e)
	{
		std::string str;

		switch (e->type())
		{
		case Element::ListType:
		{
			ListElem* l = derived_cast<ListElem*>(e);
			str += l->tag() + '[';										// Tag[
			for (ElementListI i = l->first(); i != l->end(); i++)
			{
				if(i!=l->first())
					str+=',';
				str += toMsg(*i);
			}
			str += static_cast<char>(EndListBlock);						// ]
			break;
		}
		case Element::SetType:
		{
			SetElem* s = derived_cast<SetElem*>(e);
			str += s->tag() + '(';										// Tag(		// DT8956 - bjl - Delta Adjusted Orders: MLDELHG orders are showing ?? in LegInfo column when Equity leg is a Sell Side
			for (ElementSetI i = s->first(); i != s->end(); i++)
			{
				if(i!=s->first())
					str+=',';
				str += toMsg(*i);
			}
			str += static_cast<char>(EndSetBlock);						// )		// DT8956 - bjl - Delta Adjusted Orders: MLDELHG orders are showing ?? in LegInfo column when Equity leg is a Sell Side
			break;
		}
		case Element::HashType:
		{
			HashElem* h = derived_cast<HashElem*>(e);
			str += h->tag() + '{';										// Tag{		// DT8956 - bjl - Delta Adjusted Orders: MLDELHG orders are showing ?? in LegInfo column when Equity leg is a Sell Side
			for (ElementHashI i = h->first(); i != h->end(); i++)
			{
				if(i!=h->first())
					str+=',';
				str += toMsg(*i);
			}
			str += static_cast<char>(EndHashBlock);						// }		// DT8956 - bjl - Delta Adjusted Orders: MLDELHG orders are showing ?? in LegInfo column when Equity leg is a Sell Side
			break;
		}
		case Element::StringType:
		{
			StrElem* s = derived_cast<StrElem*>(e);
			str = s->tag() + '=' + toQuotedString(s->val().c_str());			// Tag=Val,
			break;
		}
		case Element::BinaryType:
		{
			BinaryElem* b = derived_cast<BinaryElem*>(e);
			char numStr[48];
			_itoa(b->valType(), numStr, 10);
			str = b->tag() + '<'; str += numStr; str += ',';			// Tag<type,
			_ltoa(static_cast<long>(b->valLen()), numStr, 10);
			str += numStr; str += ',';									// len,
			std::string bData;
			bData.assign(static_cast<char*>(b->val()), b->valLen());
			str += bData + '>';											// data>
			break;
		}
		default:
			break;
		}

		return str;
	}

	static std::string toQuotedString(const char* ptr)
	{
		if (!ptr)
			return "";

		const char* curPtr = ptr;
		std::string quotedStr("\"");

		while (*curPtr)
		{
			switch (*curPtr)
			{
			case Escape:
			case String:
			case PairElement:
			case BinaryElement:
			case ListElement:
			case SetElement:
			case HashElement:
			case EndElement:
			case EndBinaryElement:
			case EndListBlock:
			case EndSetBlock:
			case EndHashBlock:
				quotedStr += '\\';
			}
			quotedStr += *curPtr;
			++curPtr;
		}
		quotedStr += '"';
		return quotedStr;
	}

private:
	inline Element* factory(ParserState s, const char* buf)
	{
		switch (s)
		{
		case PairElement:		return new StrElem(buf);
		case BinaryElement:		return new BinaryElem(buf);
		case ListElement:		return new ListElem(buf);	
		case SetElement:		return new SetElem(buf);
		case HashElement:		return new HashElem(buf);
		default:				return 0;
		}
	}

	Element* processBinaryElement(char* buf, int& bufIdx, const char*& ptr, Element* pElem, bool& bIsRootElem)
	{
		int binaryType = BinaryElem::BinaryTypeUndef; // DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling 	// DT7549 DLA 20100830
		bool gotBinaryType = false;				// DT7549 DLA 20100830
		while (*ptr)
		{
			ParserState ps = static_cast<ParserState>(*ptr);
			switch (ps)
			{
				case EndElement:
				{
					buf[bufIdx] = '\000'; bufIdx = 0;

					if (gotBinaryType == false)	// DT7549 DLA 20100830
					{
						binaryType = atol(buf);
						gotBinaryType = true;	// DT7549 DLA 20100830
						break;
					}

					size_t len = atol(buf);
					if (len)
					{
						BinaryElem* pBinElem = static_cast<BinaryElem*>(pElem);
						if (pBinElem != NULL)
							*pBinElem = BinaryElem(pElem->tag(), const_cast<char*>(++ptr), len, false, binaryType); // DT7549 DLA 20100830
						ptr += len;
					}

					container.pop();
					state.pop();

					if (container.empty())
						return pElem;

					container.top()->insert(pElem);
					return pElem;
				}
				break;
				default:
					buf[bufIdx++] = *ptr;
			}
			ptr++;
		}

		return pElem;
	}

	Element* Parser::endElement(char* buf, int& bufIdx, bool& bIsRootElem)
	{
		buf[bufIdx] = '\000'; bufIdx = 0;

		bIsRootElem = false;
		Element* pElem = NULL;
		if (state.top() == PairElement)
		{
			if (!container.empty())
			{
				pElem = container.top();
				if (pElem != NULL)
				{
					StrElem* pStringElem = static_cast<StrElem*>(pElem);
					pStringElem->val(buf);
				}

				container.pop();
				if (container.empty())
					bIsRootElem = true;
				else if (pElem != NULL)
					container.top()->insert(pElem);
			}
	//Y			else
	//Y				error

			state.pop();
		}
		else if (strlen(buf) != 0)
		{
			pElem = factory(PairElement, buf);

			if (container.empty())
				bIsRootElem = true;
			else if (pElem != NULL)
				container.top()->insert(pElem);
		}
	//Y		else
	//Y			error

		return pElem;
	}

	Element* endContainer(char* buf, int& bufIdx, bool& bIsRootElem, Element::Type eType)
	{
		Element* pElem = endElement(buf, bufIdx, bIsRootElem);

		if (bIsRootElem || container.empty())
		{
			//Y Log error
			return pElem;
		}

		pElem = container.top();
		if (pElem == NULL || pElem->type() != eType)
		{
			//Y Log error
			return pElem;
		}

		container.pop();
		if (container.empty())
			bIsRootElem = true;
		else if (pElem != NULL)
			container.top()->insert(pElem);

		return pElem;
	}

	std::stack<ParserState> state;
	std::stack<Element*> container;
	ParseError error;
	const char *errLoc;
};


#endif