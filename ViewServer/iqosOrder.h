// + DT10235 YL 20111031
#ifndef __IQOS_ORDER_H__
#define __IQOS_ORDER_H__

/* Sample IqosOrder message

IqosOrder
(
	L2SessionID="1",
	L2MessageID="1",
	Response="0",
	Action="1",
	Status="2",
	RejectReason="Invalid Domain",
	IsParentOrder="true",
	Domain="DEMOKMA32",
	List="LIST_ID_1",
	OrderNo="20111027-1001",
	ParentOrderNo="20111027-1001",
	Source="FrontEnd",
	User="NDB5UY5",
	Group="IQDEV",
	Trader="YVES",
	ClientOrderID="CLIENT-0001",
	Symbol="BAC",
	Price="5.73",
	Slice="10",
	Qty="200",
	TradingAccount="TESTACCT",
	ClearingAccount="YT55",
	Broker="ICAP",
	PortfolioCodes="XYZ",
	Route="IQOS",
	ExeRoute="ARCA",
	OrderType="4",
	Side="1",
	OrderStatus="-2",
	IsAlgoOrder="false",
	IsFIX="false",
	UserDefined_1="",
	UserDefined_2="",
	UserDefined_3="",
	FilledQty="100",
	TotalLeavesQty="100",
	WorkingLeavesQty="60",
	NonWorkingLeavesQty="40",
	CanceledQty="0",
	AvgPrice="5.74",
	LastPrice="5.75",
	LastTime="15:21:38"
	PrefMMID="ARCA"
	Leg1
	(
		Symbol="IBM",
		Price="152.35",
		Ratio="2",
		Side="2"
	)
	Leg2
	(
		Symbol="YHOO",
		Price="13.75",
		Ratio="1",
		Side="1"
	}
	Leg3
	(
		Symbol="MSFT",
		Price="26.91",
		Ratio="2",
		Side="1"
	}
)
*/

#include "gbtOrder.h"

#include <string>
#include <sstream>
#include <map>

enum IqosOrderAction
{
	IqosOrderActionInsert	= 1,
	IqosOrderActionDelete	= 2,
	IqosOrderActionCancel	= 3,
	IqosOrderActionUpdate	= 4,
	IqosCancelAction		= 5,
	IqosRollupAction		= 6,
	IqosOrderActionConfirm	= 7
};

class IqosOrder : public GBTOrder
{
public:
	// Default constructor
	IqosOrder() : GBTOrder(OrderType::Iqos)
	{
		_setMainElems.tag(OrderType::GetTypeString(_type));
	}

	// Constructor that parses 2012 Msg
	IqosOrder(const char* msg, size_t len) : GBTOrder(OrderType::Iqos)
	{
		_setMainElems.tag(OrderType::GetTypeString(_type));
		parseMsg(msg, len);
	}

	// Destructor
	virtual ~IqosOrder()
	{
	}

	// Parse 2012 message
	bool parseMsg(const char* msg, size_t len)
	{
		if (NULL == msg)
			return false;

		if (len < 0)
			len = strlen(msg);	//Potentially dangerous if binary data is passed

		if(OrderType::Is(msg) == OrderType::Unknown) 
		{
			_errMsg = OrderType::GetTypeString(_type);
			_errMsg += " ERROR: Unknown message type."; 
			return false;
		}
		_msg.assign(msg, len);

		Parser parser;
		SetElem* pSetElem = derived_cast<SetElem*>(parser.parse(msg, len));
		if (NULL == pSetElem || pSetElem->type() != Element::SetType)
		{
			_errMsg = OrderType::GetTypeString(_type);
			_errMsg += " ERROR: Unable to parse message.";
			delete pSetElem; pSetElem = NULL;
			return false;
		}

		_setMainElems = *pSetElem;
		pSetElem->val().clear();
		delete pSetElem; pSetElem = NULL;

		// Populate set with elements, thereby ensuring uniqueness
		ElementSetI esi;
		for (esi = _setMainElems.val().begin() ; esi != _setMainElems.val().end() ; esi++)
		{
			Element* pElem = *esi;
			if (NULL == pElem || Element::SetType != pElem->type())
			{
				continue;
			}

			SetElem* pSetElem = static_cast<SetElem*>(pElem);
			if (0 == _strnicmp(pElem->tag().c_str(), "Leg", 3)) //Look for "Leg####" format
			{
				std::string strLegNumber;
				const char* pTempPtr = pElem->tag().c_str() + 3;
				while (*pTempPtr != '\0')
				{
					if (0 == isdigit(*pTempPtr))
						break;

					strLegNumber += *pTempPtr;
					pTempPtr++;
				}

				if (*pTempPtr == '\0')
				{
					int dLegNumber = atoi(strLegNumber.c_str());
					if (dLegNumber > 0)
					{
						_mapLegElems[dLegNumber] = pSetElem;
					}
				}
			}
		}

		return true;
	}

	// Generate 2012 msg
	const char* toMsg()
	{
		_msg = Parser::toMsg(&_setMainElems);
		return _msg.c_str();
	}

	// Clean up elements
	void reset()
	{
		_mapLegElems.clear();
		_setMainElems.reset();
	}

	// ---------------------------------------------------
	// ---------- Basic element/value accessors ----------
	// ---------------------------------------------------

	// Get element from any set
	static Element* getElement(const SetElem& rSetElem, const std::string& strTag)
	{
		return rSetElem.find(strTag);
	}

	// Get value from any set
	static const char* getValue(const SetElem& rSetElem, const std::string& strTag)
	{
		Element* pElem = getElement(rSetElem, strTag);
		if (NULL != pElem && Element::StringType == pElem->type())
		{
			StrElem* pStrElem = static_cast<StrElem*>(pElem);
			return pStrElem->val().c_str();
		}
		return NULL;
	}

	// Set tag/value to any set
	static bool setElement(SetElem& rSetElem, const std::string &strTag, const std::string& strValue)
	{
		if (true == strTag.empty())
			return false;

		Element* pOldElem = rSetElem.find(strTag);
		if (NULL == pOldElem)
		{
			StrElem* pNewElem = new StrElem(strTag, strValue);
			return rSetElem.insert(pNewElem);
		}
		else if (Element::StringType == pOldElem->type())
		{
			StrElem* pOldStrElem = static_cast<StrElem*>(pOldElem);
			pOldStrElem->val(strValue);
		}

		return true;
	}

	// Get element from the main set
	Element* getElement(const std::string& strTag) const
	{
		return IqosOrder::getElement(_setMainElems, strTag);
	}

	// Get value from the main set
	const char* getValue(const std::string& strTag) const
	{
		return IqosOrder::getValue(_setMainElems, strTag);
	}

	// Set tag/value to the main set
	bool setElement(const std::string& strTag, const std::string& strValue)
	{
		return IqosOrder::setElement(_setMainElems, strTag, strValue);
	}

	// Get element from one leg
	Element* getElement(int dLeg, const std::string& strTag) const
	{
		SetElemByLegsConstIter iter = _mapLegElems.find(dLeg);
		if (iter != _mapLegElems.end())
		{
			SetElem* pSetElem = iter->second;
			if (NULL != pSetElem)
				return getElement(*pSetElem, strTag);
		}
		return NULL;
	}

	// Get value from one leg
	const char* getValue(int dLeg, const std::string& strTag) const
	{
		SetElemByLegsConstIter iter = _mapLegElems.find(dLeg);
		if (iter != _mapLegElems.end())
		{
			SetElem* pSetElem = iter->second;
			if (NULL != pSetElem)
				return getValue(*pSetElem, strTag);
		}
		return NULL;
	}

	// Set tag/value to one leg
	bool setElement(int dLeg, const std::string& strTag, const std::string& strValue)
	{
		SetElemByLegsIter iter = _mapLegElems.find(dLeg);
		SetElem* pSetElem = NULL;
		if (iter != _mapLegElems.end())
			pSetElem = iter->second;
		if (NULL == pSetElem)
		{
			pSetElem = new SetElem;
			_mapLegElems[dLeg] = pSetElem;
		}
		if (NULL != pSetElem)
			return IqosOrder::setElement(*pSetElem, strTag, strValue);
		return false;
	}

	// ----------------------------------------------
	// ---------- Specific types of fields ----------
	// ----------------------------------------------

	long getValueAsLong(const std::string& strTag) const
	{
		const char* pVal = getValue(strTag);
		if (NULL != pVal)
			return atol(pVal);
		return 0;
	}

	bool setValueAsLong(const std::string& strTag, long ldValue)
	{
		std::ostringstream oss;
		oss << ldValue;
		return setElement(strTag, oss.str());
	}

	long getValueAsLong(int dLeg, const std::string& strTag) const
	{
		const char* pVal = getValue(dLeg, strTag);
		if (NULL != pVal)
			return atol(pVal);
		return 0;
	}

	bool setValueAsLong(int dLeg, const std::string& strTag, long ldValue)
	{
		std::ostringstream oss;
		oss << ldValue;
		return setElement(dLeg, strTag, oss.str());
	}

	char getValueAsChar(const std::string& strTag) const
	{
		const char* pVal = getValue(strTag);
		if (NULL != pVal)
			return *pVal;
		return -1;
	}

	bool setValueAsChar(const std::string& strTag, char cValue)
	{
		return setElement(strTag, std::string(1, cValue));
	}

	char getValueAsChar(int dLeg, const std::string& strTag) const
	{
		const char* pVal = getValue(dLeg, strTag);
		if (NULL != pVal)
			return *pVal;
		return -1;
	}

	bool setValueAsChar(int dLeg, const std::string& strTag, char cValue)
	{
		return setElement(dLeg, strTag, std::string(1, cValue));
	}

	double getValueAsDouble(const std::string& strTag) const
	{
		const char* pVal = getValue(strTag);
		if (NULL != pVal)
			return atof(pVal);
		return 0.0;
	}

	bool setValueAsDouble(const std::string& strTag, double lfValue)
	{
		std::ostringstream oss;
		oss << lfValue;
		return setElement(strTag, oss.str());
	}

	double getValueAsDouble(int dLeg, const std::string& strTag) const
	{
		const char* pVal = getValue(dLeg, strTag);
		if (NULL != pVal)
			return atof(pVal);
		return 0.0;
	}

	bool setValueAsDouble(int dLeg, const std::string& strTag, double lfValue)
	{
		std::ostringstream oss;
		oss << lfValue;
		return setElement(dLeg, strTag, oss.str());
	}

	bool getValueAsBoolean(const std::string& strTag) const
	{
		const char* pVal = getValue(strTag);
		if (NULL == pVal)
			return false;
		if (0 == ::_strnicmp(pVal, "true", 4))		// VS2010 Bud 20130115
			return true;
		return false;
	}

	bool setValueAsBoolean(const std::string& strTag, bool bValue)
	{
		return setElement(strTag, bValue ? "true" : "false");
	}

	bool getValueAsBoolean(int dLeg, const std::string& strTag) const
	{
		const char* pVal = getValue(dLeg, strTag);
		if (NULL == pVal)
			return false;
		if (0 == ::_strnicmp(pVal, "true", 4))	// VS2010 Bud 20130115
			return true;
		return false;
	}

	bool setValueAsBoolean(int dLeg, const std::string& strTag, bool bValue)
	{
		return setElement(dLeg, strTag, bValue ? "true" : "false");
	}

	// -------------------------------------
	// ---------- Specific fields ----------
	// -------------------------------------

	inline long getL2SessionID() const { return getValueAsLong("L2SessionID"); }
	inline bool setL2SessionID(long ldSessionId) { return setValueAsLong("L2SessionID", ldSessionId); }

	inline long getL2MessageID() const { return getValueAsLong("L2MessageID"); }
	inline bool setL2MessageID(long ldMessageId) { return setValueAsLong("L2MessageID", ldMessageId); }

	inline long getResponse() const { return getValueAsLong("Response"); }
	inline bool setResponse(long ldResponse) { return setValueAsLong("Response", ldResponse); }

	inline long getAction() const { return getValueAsLong("Action"); }
	inline bool setAction(long ldAction) { return setValueAsLong("Action", ldAction); }

	inline long getStatus() const { return getValueAsLong("Status"); }
	inline bool setStatus(long ldStatus) { return setValueAsLong("Status", ldStatus); }

	inline const char* getRejectReason() const { return getValue("RejectReason"); }
	inline bool setRejectReason(const char* pRejectReason) { return setElement("RejectReason", pRejectReason); }

	inline bool getIsParentOrder() const { return getValueAsBoolean("IsParentOrder"); }
	inline bool setIsParentOrder(bool bIsParentOrder) { return setValueAsBoolean("IsParentOrder", bIsParentOrder); }

	inline const char* getDomain() const { return getValue("Domain"); }
	inline bool setDomain(const char* pDomain) { return setElement("Domain", pDomain); }

	inline const char* getList() const { return getValue("List"); }
	inline bool setList(const char* pList) { return setElement("List", pList); }

	inline const char* getOrderNo() const { return getValue("OrderNo"); }
	inline bool setOrderNo(const char* pOrderNo) { return setElement("OrderNo", pOrderNo); }

	inline const char* getParentOrderNo() const { return getValue("ParentOrderNo"); }
	inline bool setParentOrderNo(const char* pParentOrderNo) { return setElement("ParentOrderNo", pParentOrderNo); }

	inline const char* getFIXOrderID() const { return getValue("FIXOrderID"); }
	inline bool setFIXOrderID(const char* pFIXOrderID) { return setElement("FIXOrderID", pFIXOrderID); }

	inline const char* getReplacedOrderNo() const { return getValue("ReplacedOrderNo"); }
	inline bool setReplacedOrderNo(const char* pReplacedOrderNo) { return setElement("ReplacedOrderNo", pReplacedOrderNo); }

	inline const char* getOriginalOrderNo() const { return getValue("OriginalOrderNo"); }
	inline bool setOriginalOrderNo(const char* pOriginalOrderNo) { return setElement("OriginalOrderNo", pOriginalOrderNo); }

	inline const char* getCurrentOrderNo() const { return getValue("CurrentOrderNo"); }
	inline bool setCurrentOrderNo(const char* pCurrentOrderNo) { return setElement("CurrentOrderNo", pCurrentOrderNo); }

	inline long getVersion() const { return getValueAsLong("Version"); }
	inline bool setVersion(long ldVersion) { return setValueAsLong("Version", ldVersion); }

	inline const char* getSource() const { return getValue("Source"); }
	inline bool setSource(const char* pSource) { return setElement("Source", pSource); }

	inline const char* getUser() const { return getValue("User"); }
	inline bool setUser(const char* pUser) { return setElement("User", pUser); }

	inline const char* getGroup() const { return getValue("Group"); }
	inline bool setGroup(const char* pGroup) { return setElement("Group", pGroup); }

	inline const char* getTrader() const { return getValue("Trader"); }
	inline bool setTrader(const char* pTrader) { return setElement("Trader", pTrader); }

	inline const char* getClientID() const { return getValue("ClientID"); }
	inline bool setClientID(const char* pClientID) { return setElement("ClientID", pClientID); }

	inline long getFIXLineID() const { return getValueAsLong("FIXLineID"); }
	inline bool setFIXLineID(long ldFIXLineID) { return setValueAsLong("FIXLineID", ldFIXLineID); }

	inline const char* getClientOrderID() const { return getValue("ClientOrderID"); }
	inline bool setClientOrderID(const char* pClientOrderID) { return setElement("ClientOrderID", pClientOrderID); }

	inline const char* getSymbol() const { return getValue("Symbol"); }
	inline bool setSymbol(const char* pSymbol) { return setElement("Symbol", pSymbol); }

	inline const char* getOptionRoot() const { return getValue("OptionRoot"); }
	inline bool setOptionRoot(const char* pOptionRoot) { return setElement("OptionRoot", pOptionRoot); }

	inline long getExpirationDate() const { return getValueAsLong("ExpirationDate"); }
	inline bool setExpirationDate(long ldExpirationDate) { return setValueAsLong("ExpirationDate", ldExpirationDate); }

	inline double getStrikePrice() const { return getValueAsDouble("StrikePrice"); }
	inline bool setStrikePrice(long lfStrikePrice) { return setValueAsDouble("StrikePrice", lfStrikePrice); }

	inline char getPutCall() const { return getValueAsChar("PutCall"); }
	inline bool setPutCall(char cPutCall) { return setValueAsChar("PutCall", cPutCall); }

	inline double getPrice() const { return getValueAsDouble("Price"); }
	inline bool setPrice(double lfPrice) { return setValueAsDouble("Price", lfPrice); }

	inline long getSlice() const { return getValueAsLong("Slice"); }
	inline bool setSlice(long ldSlice) { return setValueAsLong("Slice", ldSlice); }

	inline long getQty() const { return getValueAsLong("Qty"); }
	inline bool setQty(long ldQty) { return setValueAsLong("Qty", ldQty); }

	inline const char* getTradingAccount() const { return getValue("TradingAccount"); }
	inline bool setTradingAccount(const char* pTradingAccount) { return setElement("TradingAccount", pTradingAccount); }

	inline const char* getClearingAccount() const { return getValue("ClearingAccount"); }
	inline bool setClearingAccount(const char* pClearingAccount) { return setElement("ClearingAccount", pClearingAccount); }

	inline const char* getAccType() const { return getValue("AccType"); }
	inline bool setAccType(const char* pAccType) { return setElement("AccType", pAccType); }

	inline const char* getBroker() const { return getValue("Broker"); }
	inline bool setBroker(const char* pBroker) { return setElement("Broker", pBroker); }

	inline const char* getRoute() const { return getValue("Route"); }
	inline bool setRoute(const char* pRoute) { return setElement("Route", pRoute); }

	inline const char* getSubRoute() const { return getValue("SubRoute"); }
	inline bool setSubRoute(const char* pSubRoute) { return setElement("SubRoute", pSubRoute); }

	inline long getOrderType() const { return getValueAsLong("OrderType"); }
	inline bool setOrderType(long ldOrderType) { return setValueAsLong("OrderType", ldOrderType); }

	inline long getSide() const { return getValueAsLong("Side"); }
	inline bool setSide(long ldSide) { return setValueAsLong("Side", ldSide); }

	inline long getOrderStatus() const { return getValueAsLong("OrderStatus"); }
	inline bool setOrderStatus(long ldOrderStatus) { return setValueAsLong("OrderStatus", ldOrderStatus); }

	inline long getOrderWarnings() const { return getValueAsLong("OrderWarnings"); }
	inline bool setOrderWarnings(long ldOrderWarnings) { return setValueAsLong("OrderWarnings", ldOrderWarnings); }

	inline bool getIsFIX() const { return getValueAsBoolean("IsFIX"); }
	inline bool setIsFIX(bool bIsFIX) { return setValueAsBoolean("IsFIX", bIsFIX); }

	inline long getNumberParentsInList() const { return getValueAsLong("NumberParentsInList"); }
	inline bool setNumberParentsInList(long ldNumberParentsInList) { return setValueAsLong("NumberParentsInList", ldNumberParentsInList); }

	inline const char* getUserComment() const { return getValue("UserComment"); }
	inline bool setUserComment(const char* pUserComment) { return setElement("UserComment", pUserComment); }

	inline const char* getUserDefined_1() const { return getValue("UserDefined_1"); }
	inline bool setUserDefined_1(const char* pUserDefined_1) { return setElement("UserDefined_1", pUserDefined_1); }

	inline const char* getUserDefined_2() const { return getValue("UserDefined_2"); }
	inline bool setUserDefined_2(const char* pUserDefined_2) { return setElement("UserDefined_2", pUserDefined_2); }

	inline const char* getUserDefined_3() const { return getValue("UserDefined_3"); }
	inline bool setUserDefined_3(const char* pUserDefined_3) { return setElement("UserDefined_3", pUserDefined_3); }

	inline long getFilledQty() const { return getValueAsLong("FilledQty"); }
	inline bool setFilledQty(long ldFilledQty) { return setValueAsLong("FilledQty", ldFilledQty); }

	inline long getTotalLeavesQty() const { return getValueAsLong("TotalLeavesQty"); }
	inline bool setTotalLeavesQty(long ldTotalLeavesQty) { return setValueAsLong("TotalLeavesQty", ldTotalLeavesQty); }

	inline long getWorkingLeavesQty() const { return getValueAsLong("WorkingLeavesQty"); }
	inline bool setWorkingLeavesQty(long ldWorkingLeavesQty) { return setValueAsLong("WorkingLeavesQty", ldWorkingLeavesQty); }

	inline long getNonWorkingLeavesQty() const { return getValueAsLong("NonWorkingLeavesQty"); }
	inline bool setNonWorkingLeavesQty(long ldNonWorkingLeavesQty) { return setValueAsLong("NonWorkingLeavesQty", ldNonWorkingLeavesQty); }

	inline long getCanceledQty() const { return getValueAsLong("CanceledQty"); }
	inline bool setCanceledQty(long ldCanceledQty) { return setValueAsLong("CanceledQty", ldCanceledQty); }

	inline double getAvgPrice() const { return getValueAsDouble("AvgPrice"); }
	inline bool setAvgPrice(double lfAvgPrice) { return setValueAsDouble("AvgPrice", lfAvgPrice); }

	inline double getLastPrice() const { return getValueAsDouble("LastPrice"); }
	inline bool setLastPrice(double lfLastPrice) { return setValueAsDouble("LastPrice", lfLastPrice); }

	inline const char* getLastTime() const { return getValue("LastTime"); }
	inline bool setLastTime(const char* pLastTime) { return setElement("LastTime", pLastTime); }

	inline const char* getLastDate() const { return getValue("LastDate"); }
	inline bool setLastDate(const char* pLastDate) { return setElement("LastDate", pLastDate); }

	inline const char* getPrefMMID() const { return getValue("PrefMMID"); }
	inline bool setPrefMMID(const char* pPrefMMID) { return setElement("PrefMMID", pPrefMMID); }

	inline const char* getAlgoData() const { return getValue("AlgoData"); }
	inline bool setAlgoData(const char* pAlgoData) { return setElement("AlgoData", pAlgoData); }

// + DT11916 DT11928 - bjl - IQOS - Do Not Report OATS
	inline bool getNotOATSReportable() const { return getValueAsBoolean("NotOATSReportable"); }
	inline bool setNotOATSReportable(bool bNotOATSReportable) { return setValueAsBoolean("NotOATSReportable", bNotOATSReportable); }
// - DT11916 DT11928
	// -------------------------------------------------------
	// ---------- Fields substituting for cam codes ----------
	// -------------------------------------------------------

	inline const char* getCMTA() const { return getValue("CMTA"); }
	inline bool setCMTA(const char* pCMTA) { return setElement("CMTA", pCMTA); }

	inline const char* getPortfolio() const { return getValue("Portfolio"); }
	inline bool setPortfolio(const char* pPortfolio) { return setElement("Portfolio", pPortfolio); }

	inline const char* getShortAuth() const { return getValue("ShortAuth"); }
	inline bool setShortAuth(const char* pShortAuth) { return setElement("ShortAuth", pShortAuth); }

	inline const char* getProgram() const { return getValue("Program"); }
	inline bool setProgram(const char* pProgram) { return setElement("Program", pProgram); }

	inline const char* getShortLocate() const { return getValue("ShortLocate"); }
	inline bool setShortLocate(const char* pShortLocate) { return setElement("ShortLocate", pShortLocate); }

	inline const char* getSolunSolicited() const { return getValue("SolunSolicited"); }
	inline bool setSolunSolicited(const char* pSolunSolicited) { return setElement("SolunSolicited", pSolunSolicited); }

	inline const char* getAnonymous() const { return getValue("Anonymous"); }
	inline bool setAnonymous(const char* pAnonymous) { return setElement("Anonymous", pAnonymous); }

	inline const char* getTransTypeEqt() const { return getValue("TransTypeEqt"); }
	inline bool setTransTypeEqt(const char* pTransTypeEqt) { return setElement("TransTypeEqt", pTransTypeEqt); }

	inline const char* getTransTypeOpt() const { return getValue("TransTypeOpt"); }
	inline bool setTransTypeOpt(const char* pTransTypeOpt) { return setElement("TransTypeOpt", pTransTypeOpt); }

	inline const char* getCMTA2() const { return getValue("CMTA2"); }
	inline bool setCMTA2(const char* pCMTA2) { return setElement("CMTA2", pCMTA2); }

	inline const char* getTraderAltAcc() const { return getValue("TraderAltAcc"); }
	inline bool setTraderAltAcc(const char* pTraderAltAcc) { return setElement("TraderAltAcc", pTraderAltAcc); }

	inline const char* getNotHeld() const { return getValue("NotHeld"); }
	inline bool setNotHeld(const char* pNotHeld) { return setElement("NotHeld", pNotHeld); }

	inline const char* getOverrideRestrictedList() const { return getValue("OverrideRestrictedList"); }
	inline bool setOverrideRestrictedList(const char* pOverrideRestrictedList) { return setElement("OverrideRestrictedList", pOverrideRestrictedList); }

	inline const char* getOverrideNewRestrictedList() const { return getValue("OverrideNewRestrictedList"); }
	inline bool setOverrideNewRestrictedList(const char* pOverrideNewRestrictedList) { return setElement("OverrideNewRestrictedList", pOverrideNewRestrictedList); }

	inline const char* getOverrideRule201() const { return getValue("OverrideRule201"); }
	inline bool setOverrideRule201(const char* pOverrideRule201) { return setElement("OverrideRule201", pOverrideRule201); }

	// --------------------------------------------
	// ---------- Specific fields by leg ----------
	// --------------------------------------------

	inline const char* getSymbol(int dLeg) const { return getValue(dLeg, "Symbol"); }
	inline bool setSymbol(int dLeg, const char* pSymbol) { return setElement(dLeg, "Symbol", pSymbol); }

	inline const char* getOptionRoot(int dLeg) const { return getValue(dLeg, "OptionRoot"); }
	inline bool setOptionRoot(int dLeg, const char* pOptionRoot) { return setElement(dLeg, "OptionRoot", pOptionRoot); }

	inline long getExpirationDate(int dLeg) const { return getValueAsLong(dLeg, "ExpirationDate"); }
	inline bool setExpirationDate(int dLeg, long ldExpirationDate) { return setValueAsLong(dLeg, "ExpirationDate", ldExpirationDate); }

	inline double getStrikePrice(int dLeg) const { return getValueAsDouble(dLeg, "StrikePrice"); }
	inline bool setStrikePrice(int dLeg,  double lfStrikePrice) { return setValueAsDouble(dLeg, "StrikePrice", lfStrikePrice); }

	inline char getPutCall(int dLeg) const { return getValueAsChar(dLeg, "PutCall"); }
	inline bool setPutCall(int dLeg, char cPutCall) { return setValueAsChar(dLeg, "PutCall", cPutCall); }

	inline double getPrice(int dLeg) const { return getValueAsDouble(dLeg, "Price"); }
	inline bool setPrice(int dLeg, double lfPrice) { return setValueAsDouble(dLeg, "Price", lfPrice); }

	inline long getRatio(int dLeg) const { return getValueAsLong(dLeg, "Ratio"); }
	inline bool setRatio(int dLeg, long ldRatio) { return setValueAsLong(dLeg, "Ratio", ldRatio); }

	inline long getSide(int dLeg) const { return getValueAsLong(dLeg, "Side"); }
	inline bool setSide(int dLeg, long ldSide) { return setValueAsLong(dLeg, "Side", ldSide); }

	// -------------------------------------
	// ---------- Other accessors ----------
	// -------------------------------------

	inline unsigned long getLegCount() const { return static_cast<unsigned long>(_mapLegElems.size()); }

private:
	IqosOrder(const IqosOrder&);
	const IqosOrder operator=(const IqosOrder&);

	typedef std::map<int, SetElem*>			SetElemByLegs;
	typedef SetElemByLegs::iterator			SetElemByLegsIter;
	typedef SetElemByLegs::const_iterator	SetElemByLegsConstIter;

	SetElem			_setMainElems;
	SetElemByLegs	_mapLegElems;
};

#endif	// __IQOS_ORDER_H__
// - DT10235 YL 20111031