
#ifndef _PRICE_H
#define _PRICE_H
//
// THE INFORMATION CONTAINED HEREIN IS PROVIDED <93>AS IS<94> AND NO PERSON OR ENTITY ASSOCIATED WITH THE
// INSTAQUOTE MARKET DATA API MAKES ANY REPRESENTATION OR WARRANTY, EXPRESS OR IMPLIED, AS TO THE
// INSTAQUOTE MARKET DATA API (OR THE RESULTS TO BE OBTAINED BY THE USE THEREOF) OR ANY OTHER MATTER
// AND EACH SUCH PERSON AND ENTITY SPECIFICALLY DISCLAIMS ANY WARRANTY OF ORIGINALITY, ACCURACY,
// COMPLETENESS, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  SUCH PERSONS AND ENTITIES
// DO NOT WARRANT THAT THE INSTAQUOTE MARKET DATA API WILL CONFORM TO ANY DESCRIPTION THEREOF OR
// BE FREE OF ERRORS.  THE ENTIRE RISK OF ANY USE OF THE INSTAQUOTE MARKET DATA API IS ASSUMED BY THE USER.
//
// NO PERSON OR ENTITY ASSOCIATED WITH THE INSTAQUOTE MARKET DATA API SHALL HAVE ANY LIABILITY FOR
// DAMAGE OF ANY KIND ARISING IN ANY MANNER OUT OF OR IN CONNECTION WITH ANY USER<92>S USE OF (OR ANY
// INABILITY TO USE) THE INSTAQUOTE MARKET DATA API, WHETHER DIRECT, INDIRECT, INCIDENTAL, SPECIAL
// OR CONSEQUENTIAL (INCLUDING, WITHOUT LIMITATION, LOSS OF DATA, LOSS OF USE, CLAIMS OF THIRD PARTIES
// OR LOST PROFITS OR REVENUES OR OTHER ECONOMIC LOSS), WHETHER IN TORT (INCLUDING NEGLIGENCE AND STRICT
// LIABILITY), CONTRACT OR OTHERWISE, WHETHER OR NOT ANY SUCH PERSON OR ENTITY HAS BEEN ADVISED OF,
// OR OTHERWISE MIGHT HAVE ANTICIPATED THE POSSIBILITY OF, SUCH DAMAGES.
//
// No proprietary or ownership interest of any kind is granted with respect to the INSTAQUOTE MARKET DATA API
// (or any rights therein).
//
// Copyright © 2009 Banc of America Securities, LLC. All rights reserved.
//

#pragma pack(push,1)

class PRICE
{
public:
	PRICE()
		:PriceCode(0)
		,Region(0)
		,Integral(0)
	{
	}
	PRICE(char c, DWORD d)
	{
		PriceCode=c;
		Integral=d;
	}

	char PriceCode;
	char Region;
	__int64 Integral;

	operator double()
	{
		if((PriceCode>=0x21)&&(PriceCode<=0x27))
		{
			double dval=(double)Integral;
			if(PriceCode=='!')//0x21 1/8
				dval=dval*1/8;
			else if(PriceCode=='"')//0x22 1/4
				dval=dval*1/4;
			else if(PriceCode=='#')//0x23 3/8
				dval=dval*3/8;
			else if(PriceCode=='$')//0x24 1/2
				dval=dval*1/2;
			else if(PriceCode=='%')//0x25 5/8
				dval=dval*5/8;
			else if(PriceCode=='&')//0x26 3/4
				dval=dval*3/4;
			else if(PriceCode=='\'')//0x27 7/8
				dval=dval*7/8;
			return dval;
		}
		else if(PriceCode>0)
		{
			double dval=(double)Integral;
			for(int d=0;d<PriceCode;d++)
				dval/=10;
			return dval;
		}
		else if(PriceCode<0)
		{
			double dval=-(double)Integral;
			for(int d=0;d<-PriceCode;d++)
				dval/=10;
			return dval;
		}
		else
			return (double)Integral;
	}
	// Fast comparison with no floating point operations
	bool operator>(const PRICE& px)
	{
		if(PriceCode==px.PriceCode)
			return Integral>px.Integral;
		// Have to do floating point for fractionals
		else if(((PriceCode>=0x21)&&(PriceCode<=0x27))||
			    ((px.PriceCode>=0x21)&&(px.PriceCode<=0x27)))
		{
			double tval=(double)(*this);
			double dval=(double)(PRICE)px;
			return tval>dval;
		}
		else if(PriceCode<px.PriceCode)
		{
			__int64 ival=Integral;
			for(int d=PriceCode;d<px.PriceCode;d++)
				ival*=10;
			return ival>px.Integral;
		}
		else// if(PriceCode>px.PriceCode)
		{
			__int64 ival=px.Integral;
			for(int d=px.PriceCode;d<PriceCode;d++)
				ival*=10;
			return Integral>ival;
		}
	}
	// Fast comparison with no floating point operations
	bool operator<(const PRICE& px)
	{
		if(PriceCode==px.PriceCode)
			return Integral<px.Integral;
		// Have to do floating point for fractionals
		else if(((PriceCode>=0x21)&&(PriceCode<=0x27))||
			    ((px.PriceCode>=0x21)&&(px.PriceCode<=0x27)))
		{
			double tval=(double)*this;
			double dval=(double)(PRICE)px;
			return tval<dval;
		}
		else if(PriceCode<px.PriceCode)
		{
			__int64 ival=Integral;
			for(int d=PriceCode;d<px.PriceCode;d++)
				ival*=10;
			return ival<px.Integral;
		}
		else// if(PriceCode>px.PriceCode)
		{
			__int64 ival=px.Integral;
			for(int d=px.PriceCode;d<PriceCode;d++)
				ival*=10;
			return Integral<ival;
		}
	}
	// Fast price to string converter without floating point operations
	operator const string()
	{
		char istr[64]={0},vstr[64]={0};
		// Use floating-point print for fractionals
		if((PriceCode=='!')||
		   (PriceCode=='"')||
		   (PriceCode=='#')||
		   (PriceCode=='$')||
		   (PriceCode=='%')||
		   (PriceCode=='&')||
		   (PriceCode=='\''))
		{
			sprintf(vstr,"%f",(double)*this);
			return (string)vstr;
		}

		sprintf(istr,"%I64d",Integral);
		int ilen=(int)strlen(istr);
		int ndec=PriceCode>0 ?(PriceCode<=16 ?PriceCode:0) :-PriceCode;
		int nwhole=ilen -ndec;
		int nzero=0;
		if(ndec>ilen)
			nzero=ndec -ilen;
		char *vptr=vstr,*iptr=istr;
		if(PriceCode<0)
		{
			*vptr='-'; vptr++;
		}
		if(nwhole>0)
		{
			for(int i=0;i<nwhole;i++)
			{
				*vptr=*iptr; vptr++; iptr++;
			}
		}
		else
		{
			*vptr='0'; vptr++;
		}
		*vptr='.'; vptr++;
		int i;
		for(i=0;i<nzero;i++)
		{
			*vptr='0'; vptr++;
		}
		for(i=0;i<ndec;i++)
		{
			*vptr=*iptr; vptr++; iptr++;
		}
		*vptr=0;
		return (string)vstr;
	}
	// Fast string to price converter without floating point operations
	PRICE& operator=(const string& sval)
	{
		const char *cstr=sval.c_str();
		const char *dptr=strchr(cstr,'.');
		// Have decimals
		if(dptr)
		{
			PriceCode=0;
			char istr[32]={0},*iptr=istr;
			bool neg=false;
			for(const char *cptr=cstr;*cptr;cptr++)
			{
				char ch=*cptr;
				if(ch=='-')
				{
					neg=true; continue;
				}
				else if(ch=='.')
					continue;
				else
				{
					*iptr=*cptr; iptr++;
					if(cptr>dptr)
						PriceCode++;
				}
			}
			*iptr=0;
			if(neg)
				PriceCode=-PriceCode;
			Integral=_atoi64(istr);
		}
		// No decimals
		else
		{
			if(*cstr=='-')
			{
				PriceCode=-PriceCode;
				Integral=_atoi64(cstr +1);
			}
			else
			{
				PriceCode=0;
				Integral=_atoi64(cstr);
			}
		}
		return *this;
	}
	// There's probably a better way to do this
	PRICE& operator=(const double dval)
	{
		char vstr[64]={0};
		if(PriceCode)
		{
			if((PriceCode=='!')||
				(PriceCode=='"')||
				(PriceCode=='#')||
				(PriceCode=='$')||
				(PriceCode=='%')||
				(PriceCode=='&')||
				(PriceCode=='\''))
			{
				double nval=dval;
				if(PriceCode=='!')//0x21 1/8
					nval=nval*8;
				else if(PriceCode=='"')//0x22 1/4
					nval=nval*4;
				else if(PriceCode=='#')//0x23 3/8
					nval=nval*8/3;
				else if(PriceCode=='$')//0x24 1/2
					nval=nval*2;
				else if(PriceCode=='%')//0x25 5/8
					nval=nval*8/5;
				else if(PriceCode=='&')//0x26 3/4
					nval=nval*4/3;
				else if(PriceCode=='\'')//0x27 7/8
					nval=nval*8/7;
				sprintf(vstr,"%f",nval);
			}
			else if(PriceCode>9)
			{
				sprintf(vstr,"%.09f",dval);
				char *vptr=vstr +strlen(vstr) -1;
				// Try to strip down to 6 decimals
				for(int d=0;d<3;d++)
				{
					if(*vptr=='0')
						*vptr=0;
					else
						break;
					vptr--;
				}
			}
			else
			{
				char vfmt[8]={0};
				sprintf(vfmt,"%%.0%df",PriceCode<0?-PriceCode:PriceCode);
				sprintf(vstr,vfmt,dval);
			}
		}
		else
		{
			sprintf(vstr,"%f",dval);
		}
		return *this=vstr;
	}
	PRICE& operator=(const DWORD dval)
	{
		PriceCode=0;
		Integral=dval;
		return *this;
	}
	PRICE& operator=(const long lval)
	{
		if(lval<0)
		{
			PriceCode=-1;
			Integral=lval*10;
		}
		else
		{
			PriceCode=0;
			Integral=lval;
		}
		return *this;
	}
};

#pragma pack(pop)

#endif//_PRICE_H
