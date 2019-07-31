
#ifndef _OATS_H
#define _OATS_H

#include "bfix.h"

class OatsOrder
{
public:	
	virtual const char *GetRootOrderID()=0;
	virtual const char *GetClOrdID()=0;
	virtual const char *GetEcnOrderID()=0;
	virtual const char *GetSymbol()=0;
	virtual int GetFillQty()=0;
	virtual LONGLONG GetTransactTime()=0;
};

// OATS report engine interface
class OatsEngine
{
public:
	// HD
	virtual int StartEngine(FILE *fp, int wsdate, const char *firmid)=0;
	// TR
	virtual int StopEngine()=0;
	// NW
	virtual int ParentOrderCreated(OatsOrder *porder, FIXINFO *pfix)=0;
	// CR
	virtual int ParentOrderReplaced(OatsOrder *porder, FIXINFO *pfix)=0;
	// CL
	virtual int ParentOrderCancelled(OatsOrder *porder, FIXINFO *pfix)=0;
	// ???
	virtual int ParentOrderTerminated(OatsOrder *porder, FIXINFO *pfix)=0;
	virtual int ParentOrderUnreported(OatsOrder *porder, FIXINFO *pfix)=0;
	virtual int ParentOrderUnreported(int wsdate, const char *ClOrdID)=0;
	// RT
	virtual int ChildOrderCreated(OatsOrder *porder, OatsOrder *parent, FIXINFO *pfix)=0;
	virtual int ChildOrderReplaced(OatsOrder *porder, OatsOrder *parent, FIXINFO *pfix)=0;
	virtual int ChildOrderCancelled(OatsOrder *porder, OatsOrder *parent, FIXINFO *pfix)=0;
	virtual int ChildOrderTerminated(OatsOrder *porder, OatsOrder *parent, FIXINFO *pfix)=0;
	virtual int ChildOrderUnreported(OatsOrder *porder, OatsOrder *parent, FIXINFO *pfix)=0;
};

#ifdef WIN32
#ifdef BIT64
#define GET_OATS_EXPORT_FUNC "?GetOatsEngine" // 64-bit signature
#define FREE_OATS_EXPORT_FUNC "?FreeOatsEngine" // 64-bit signature
#else
#define GET_OATS_EXPORT_FUNC "?GetOatsEngine@@YGPAVOatsEngine@@XZ" // 32-bit signature
#define FREE_OATS_EXPORT_FUNC "?FreeOatsEngine@@YGXPAVOatsEngine@@@Z" // 32-bit signature
#endif
#endif
typedef OatsEngine * (__stdcall *GETOATSENGINE)();
typedef void (__stdcall *FREEOATSENGINE)(OatsEngine *poats);

#endif//_OATS_H
