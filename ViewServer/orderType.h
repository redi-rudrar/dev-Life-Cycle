// Modified for VS2010 conversion

#ifndef __OrderType_H_defined
#define __OrderType_H_defined

#include <string.h>

struct OrderType {
public:
	typedef enum
	{
// + DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
		Unknown = 0
		,Algo = 1
		,Pair = 2	// DT7549 DLA 20100830
		,DeltaAdjusted = 3 
		,Iqos = 4	// DT10235 YL 20111031
		,GBT = 5 // DT10961 DLA 20120229
	} Type;
	static const char *GetTypeString(Type x) // DT8105 - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
	{
		switch(x)
		{
		case Algo:
			return "AlgoOrder";
		case Pair:
			return "PairOrder";
		case DeltaAdjusted:
			return "DeltaAdjustedOrder";
		// + DT10235 YL 20111031
		case Iqos:
			return "IqosOrder";
		// - DT10235 YL 20111031
		case GBT:	// DT10961 DLA 20120229
			return "GBT";
		}
		return "UnknownOrder";
	}
// + DT8105 - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
	static const char * GetTypeString(const char *msg)
	{
		char *strptr;
		static char buffer[32];		//This buffer may have to be expanded if we use longer order names. I'm wary about
		if(!msg)					//using strchr against the msg directly, since a malformed message may be missing the '[' 
			return "UnknownOrder";  //entirely, ending up with who knows what copied into the returned string
		strncpy(buffer,msg,32);
		buffer[31]='\0';
		strptr=strchr(buffer,'[');
		if(!strptr)
			return "UnknownOrder";
		strptr[0]='\0';
		return buffer;
	}
// - DT8105
// - DT8105a
	static Type Is(const char* str)
	{ if (::_strnicmp(str, "AlgoOrder[", 9) == 0)				return Algo;	// DT7549 DLA 20100910 - Pairs Trading
	  if (::_strnicmp(str, "PairOrder[", 9) == 0)				return Pair;	// DT7549 DLA 20100910 - Pairs Trading
	  if (::_strnicmp(str, "DeltaAdjustedOrder[", 18) == 0)	return DeltaAdjusted; // DT8105a - bjl - Delta Adjusted Orders: Auto Equity Hedging Order Handling
	  if (::_strnicmp(str, "IqosOrder(", 9) == 0)			return Iqos;		// DT10235 YL 20111031
	  if (::_strnicmp(str, "GBT[", 4) == 0)					return GBT;			// DT10961 DLA 20120229
	  else													return Unknown;
	}
private:
	OrderType()	{};
};

#endif