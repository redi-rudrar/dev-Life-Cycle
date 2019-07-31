
#ifndef _IQFLOW_H
#define _IQFLOW_H

class MatchEntry
{
public:
	MatchEntry()
		:key()
		,t2005()
		,t2007()
		,accountType()
		,programTrading()
		,arbitrage()
		,memberType()
	{
	}

	string key;
	string t2005;
	string t2007;
	string accountType;
	string programTrading;
	string arbitrage;
	string memberType;
};

#endif//_IQFLOW_H
