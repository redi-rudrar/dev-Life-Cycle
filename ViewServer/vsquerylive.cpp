//DT9383


#include "stdafx.h"
#ifdef SPECTRUM
#include "vsquerylive.h"
#include "ViewServer.h"


bool InList(string where, vector<string>& Tokens);


VSQueryLive::VSQueryLive(VSQuery* _pquery)
{
	pquery=_pquery;
}

bool VSQueryLive::FilterOrder(VSOrder* porder)
{
	if(!pquery || !pquery->pmod || !porder)
		return false;

	list<WhereSegment> WhereList;
	if(pquery->pmod->BuildWhereSegments(WhereList,pquery->where)==0)
		return false; // No segments

	// Iterate thru each where segment
	bool result=true;
	bool first=true;
	for(list<WhereSegment>::iterator segItr=WhereList.begin(); segItr!=WhereList.end(); segItr++)
	{
		WhereSegment& ws=(*segItr);

		if(ws.oper==OPS_OR)
		{
			if(!first && result)
				return true;

			if(CheckOrder(ws,pquery,porder))
				return true;
		}
		else if(result)
		{
			result=CheckOrder(ws,pquery,porder);
		}
		first=false;
	}

	return result;
}

bool VSQueryLive::CheckOrder(WhereSegment& ws, VSQuery* pquery, VSOrder* porder)
{
        bool inSegment=false;
        bool notInSegment=false;
        if(strstr(ws.Segment.c_str()," NOT IN "))
                notInSegment=true;
        else if(strstr(ws.Segment.c_str()," IN "))
                inSegment=true;

        if(!inSegment && !notInSegment)
                return false;

        // Get the tokens for this segment
        vector<string> Tokens;
        if(!InList(ws.Segment,Tokens))
                return false;

        // Get the keytype of this segment
        KEYTYPE keytype=pquery->pmod->ConditionMatch(ws.Segment.c_str());
        bool match=false;
        for(int i=0; i<(int)Tokens.size(); i++)
        {
                switch(keytype)
                {
                        case KEY_APPSYSTEM:
                                {
                                AppSystem* asys=pquery->pmod->GetAppSystem(porder->GetAppInstID());
                                if(asys && Tokens[i]==asys->skey)
                                        match=true;
                                }
                                break;
                        case KEY_APPINSTID:
                                if(Tokens[i]==string(porder->GetAppInstID()))
                                        match=true;
                                break;
                        case KEY_CLORDID:
                                if(Tokens[i]==string(porder->GetClOrdID()))
                                        match=true;
                                break;
                        case KEY_CLPARENTORDERID:
                                if(Tokens[i]==string(porder->GetClParentOrderID()))
                                        match=true;
                                break;
                        case KEY_FIRSTCLORDID:
                                if(Tokens[i]==string(porder->GetFirstClOrdID()))
                                        match=true;
                                break;
                        case KEY_ROOTORDERID:
                                if(Tokens[i]==string(porder->GetRootOrderID()))
                                        match=true;
                                break;
                        case KEY_SYMBOL:
                                if(Tokens[i]==string(porder->GetSymbol()))
                                        match=true;
                                break;
                        case KEY_ACCOUNT:
                                if(Tokens[i]==string(porder->GetAccount()))
                                        match=true;
                                break;
                        case KEY_ECNORDERID:
                                if(Tokens[i]==string(porder->GetEcnOrderID()))
                                        match=true;
                                break;
                        case KEY_CLIENTID:
                                if(Tokens[i]==string(porder->GetClientID()))
                                        match=true;
                                break;
                        case KEY_AUXKEY1:
                                if(Tokens[i]==string(porder->GetAuxKey(0)))
                                        match=true;
                                break;
                        case KEY_AUXKEY2:
                                if(Tokens[i]==string(porder->GetAuxKey(1)))
                                        match=true;
                                break;
                        case KEY_AUXKEY3:
                                if(Tokens[i]==string(porder->GetAuxKey(2)))
                                        match=true;
                                break;
                        case KEY_AUXKEY4:
                                if(Tokens[i]==string(porder->GetAuxKey(3)))
                                        match=true;
                                break;
                        case KEY_AUXKEY5:
                                if(Tokens[i]==string(porder->GetAuxKey(4)))
                                        match=true;
                                break;
                        case KEY_AUXKEY6:
                                if(Tokens[i]==string(porder->GetAuxKey(5)))
                                        match=true;
                                break;
                        case KEY_HIGHMSGTYPE:
                                if(Tokens[i][0]==porder->GetHighMsgType())
                                        match=true;
                                break;
                        case KEY_HIGHEXECTYPE:
                                if(Tokens[i][0]==porder->GetHighExecType())
                                        match=true;
                                break;
                        case KEY_ORDERQTY:
                                if(atol(Tokens[i].c_str())==porder->GetOrderQty())
                                        match=true;
                                break;
                        case KEY_CUMQTY:
                                if(atol(Tokens[i].c_str())==porder->GetCumQty())
                                        match=true;
                                break;
                        case KEY_FILLQTY:
                                if(atol(Tokens[i].c_str())==porder->GetFillQty())
                                        match=true;
                                break;
        
                        case KEY_UNKNOWN:
                        default:
                                return false;
                }
        
                if(inSegment && match)
                        break;
        }
        
        if(inSegment && !match)
                return false;
        if(notInSegment && match)
                return false;

	return true;
}
#endif//SPECTRUM
