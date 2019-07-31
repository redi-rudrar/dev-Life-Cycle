
#ifndef _MCFILTER_H
#define _MCFILTER_H

#include "iqcspapi.h"

class MCFilter
{
public:
	MCFilter()
	{
		EnableAllAssets();
		EnableAll();
		EnableAllLtrs();
		matchUnknown=false;
	}

	inline int Enable(IQMarketCenter mc) {return Enable(mc,true);}
	inline int Disable(IQMarketCenter mc) {return Enable(mc,false);}
	inline bool IsDisabled(IQMarketCenter mc) {return !IsEnabled(mc);}
	inline void EnableAll() {memset(FilterExchList,true,IQMC_COUNT*sizeof(bool));}
	inline void DisableAll() {memset(FilterExchList,false,IQMC_COUNT*sizeof(bool));}
	inline bool IsEnabled(IQMarketCenter mc)
	{
		if((mc==IQMC_UNKNOWN)&&(matchUnknown))
			return true;
		else if((mc<=IQMC_UNKNOWN)||(mc>=IQMC_COUNT))
			return false;
		return FilterExchList[mc];
	}
	int SetExchRegFilter(const char *fspec);

	inline int EnableLtr(char ltr) {return EnableLtr(ltr,true);}
	inline int DisableLtr(char ltr) {return EnableLtr(ltr,false);}
	inline void EnableAllLtrs() {memset(FilterLtrs,true,27);}
	inline void DisableAllLtrs() {memset(FilterLtrs,false,27);}
	inline bool IsLtrEnabled(char ltr)
	{
		int lidx=26;
		if((ltr>='A')&&(ltr<='Z'))
			lidx=ltr -'A';
		else if((ltr>='a')&&(ltr<='z'))
			lidx=ltr -'a';
		return FilterLtrs[lidx];
	}

	inline int EnableAsset(IQAssetType atype) {return EnableAsset(atype,true);}
	inline int DisableAsset(IQAssetType atype) {return EnableAsset(atype,false);}
	inline void EnableAllAssets() {memset(FilterAssets,true,IQAST_COUNT);};
	inline void DisableAllAssets() {memset(FilterAssets,false,IQAST_COUNT);};
	inline bool IsAssetEnabled(IQAssetType atype)
	{
		if((atype==IQAST_UNKNOWN)&&(matchUnknown))
			return true;
		else if((atype<=IQAST_UNKNOWN)||(atype>=IQAST_COUNT))
			return false;
		return FilterAssets[atype];
	}

	inline bool IsEnabled(IQAssetType atype, IQMarketCenter mc, char ltr)
	{
		return IsAssetEnabled(atype) &&IsEnabled(mc) &&IsLtrEnabled(ltr);
	}

	MCFilter& operator=(const MCFilter& mcf)
	{
		memcpy(FilterAssets,mcf.FilterAssets,sizeof(FilterAssets));
		memcpy(FilterExchList,mcf.FilterExchList,sizeof(FilterExchList));
		memcpy(FilterLtrs,mcf.FilterLtrs,sizeof(FilterLtrs));
		matchUnknown=mcf.matchUnknown;
		return *this;
	}

protected:
	bool FilterAssets[IQAST_COUNT];
	bool FilterExchList[IQMC_COUNT];
	bool FilterLtrs[27];
	bool matchUnknown;

	inline int Enable(IQMarketCenter mc, bool state)
	{
		if((mc<=IQMC_UNKNOWN)||(mc>=IQMC_COUNT))
			return -1;
		FilterExchList[mc]=state;
		return 0;
	}
	inline int EnableLtr(char ltr, bool state)
	{
		int lidx=26;
		if((ltr>='A')&&(ltr<='Z'))
			lidx=ltr -'A';
		else if((ltr>='a')&&(ltr<='z'))
			lidx=ltr -'a';
		FilterLtrs[lidx]=state;
		return 0;
	}
	inline int EnableAsset(IQAssetType atype, bool state)
	{
		if((atype<=IQAST_UNKNOWN)||(atype>=IQAST_COUNT))
			return -1;
		FilterAssets[atype]=state;
		return 0;
	}
};

#endif//_MCFILTER_H
