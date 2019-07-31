#ifndef _VSQUERYLIVE_H
#define _VSQUERYLIVE_H

#include "vsquery.h"


//DT10472
class VSQueryLive
{
public:
	VSQueryLive(VSQuery* _pquery);
	bool FilterOrder(VSOrder* porder);

	VSQuery* pquery;

private:
	bool CheckOrder(WhereSegment& ws, VSQuery* pquery, VSOrder* porder);

};

#endif//_VSQUERYLIVE_H
