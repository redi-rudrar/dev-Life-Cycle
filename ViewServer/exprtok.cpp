
#include "stdafx.h"
#include "exprtok.h"
#include <time.h>

void ExprTok::ParseError(int rc, const char *ebeg, const char *eend, const char *fmt, ...)
{
	if(!pnotify)
		return;
	va_list alist;
	va_start(alist,fmt);
	char ebuf[1024]={0},*eptr=ebuf;
	vsprintf_s(eptr,sizeof(ebuf),fmt,alist);
	eptr+=strlen(eptr);
	strcpy(eptr," : "); eptr+=strlen(eptr);
	int elen=(int)(eend -ebeg);
	int eleft=(int)(ebuf +sizeof(ebuf) -1 -eptr);
	if(elen>eleft) elen=eleft;
	memcpy(eptr,ebeg,elen); eptr+=elen; *eptr=0;
	pnotify->ExprTokError(-1,ebuf);
	va_end(alist);
}
// Floating point or integer number
int ExprTok::EvalDecimalExpr(const char *ebeg, const char *eend)
{
	// Skip leading whitespace
	const char *lptr;
	for(lptr=ebeg;(lptr<eend)&&myisspace(*lptr);lptr++)
		;
	// Skip trailing whitespace
	const char *rptr;
	for(rptr=eend;(rptr>lptr)&&myisspace(rptr[-1]);rptr--)
		;
	// Null is not a decimal
	if(lptr>=rptr)
		return -1;
	bool neg=false,dec=false;
	LONGLONG lval=0,dval=0;
	int dcnt=0;
	for(const char *dptr=lptr;dptr<rptr;dptr++)
	{
		char ch=*dptr;
		if(ch=='-')
		{
			if(dptr==lptr)
				neg=true;
			else
				return -1; // NAN (not a number)
		}
		else if(ch=='.')
		{
			if(dec)
				return -1; // can't have more than one decimal
			dec=true;
			continue;
		}
		else if(!myisdigit(ch))
			return -1; // NAN
		if(dec) 
		{
			dval=(dval*10) +(ch -'0'); dcnt++;
		}
		else 
			lval=(lval*10) +(ch -'0');
	}
	vtype='F';
	exprBeg=lptr;
	exprEnd=rptr;
	double decval=(double)dval;
	for(int d=0;d<dcnt;d++)
		decval/=10;
	fval=(double)lval +decval;
	if(neg)
		fval=-fval;
	//ival=(LONGLONG)fval; // assign for promotion
	return 1;
}
// Non-floating point integer only with no alpha characters nor decimal points.
int ExprTok::EvalIntegralExpr(const char *ebeg, const char *eend)
{
	// Skip leading whitespace
	const char *lptr;
	for(lptr=ebeg;(lptr<eend)&&myisspace(*lptr);lptr++)
		;
	// Skip trailing whitespace
	const char *rptr;
	for(rptr=eend;(rptr>lptr)&&myisspace(rptr[-1]);rptr--)
		;
	// Null is not an integral
	if(lptr>=rptr)
		return -1;
	bool neg=false;
	LONGLONG lval=0;
	for(const char *dptr=lptr;dptr<rptr;dptr++)
	{
		char ch=*dptr;
		if(ch=='-')
		{
			if(dptr==lptr) // first position only
				neg=true;
			else
				return -1; // NAN
		}
		else if(!myisdigit(ch))
			return -1; // NAN
		lval=(lval*10) +(ch -'0');
	}
	vtype='I';
	exprBeg=lptr;
	exprEnd=rptr;
	if(neg)
		lval=-lval;
	ival=lval;
	//fval=(double)ival; // assign for promotion
	return 1;
}
// Finds the appropriate matching close parenthesis when 'ebeg' points to open parenthesis, allowing nested parenthesis.
const char *ExprTok::MatchParen(const char *ebeg, const char *eend)
{
	if((ebeg>=eend)||(*ebeg!='('))
	{
		ParseError(-1,ebeg,eend,"MatchParen parse failure");
		return 0;
	}
	int nopen=1;
	for(const char *eptr=ebeg +1;eptr<eend;eptr++)
	{
		if(*eptr=='(')
			nopen++;
		else if(*eptr==')')
		{
			nopen--;
			if(nopen<=0)
				return eptr;
		}
	}
	ParseError(-1,ebeg,eend,"Unmatched parenthesis");
	return 0;
}
// Finds the appropriate matching single quote when 'ebeg' points to open single quote, allowing escaped quotes.
// Optionally copies the string to the supplied buffer without any escape characters.
const char *ExprTok::MatchSingleQuote(const char *ebeg, const char *eend, char *str, int slen)
{
	if((ebeg>=eend)||(*ebeg!='\''))
	{
		ParseError(-1,ebeg,eend,"MatchSingleQuote parse failure");
		return 0;
	}
	char *sptr=str,*send=sptr +slen;;
	bool escape=false;
	for(const char *eptr=ebeg +1;(eptr<eend)&&((!str)||(sptr<send));eptr++)
	{
		if((!escape)&&(*eptr=='\\'))
		{
			escape=true;
			continue;
		}
		if(*eptr=='\'')
		{
			if(!escape)
			{
				if(str)
				{
					if(sptr<send) *sptr=0;
					else if(send>str) send[-1]=0;
				}
				return eptr;
			}
		}
		if(str)
		{
			*sptr=*eptr; sptr++;
		}
		escape=false;
	}
	ParseError(-1,ebeg,eend,"Unmatched quote");
	return 0;
}
// Type promotion
char ExprTok::PromoteType(ExprTok *t1, ExprTok *t2, bool forAssign)
{
	if(!t2)
		return 0;
	char ctype=t2->vtype; // prefer parm2 type
	if(t1->vtype!=t2->vtype)
	{
		ctype=0;
		switch(t1->vtype)
		{
		case 'I':
			switch(t2->vtype)
			{
			case 'I': _ASSERT(false); ctype='I'; break;
			case 'F': ctype='F'; t1->fval=(double)t1->ival; break;
			case 'B': ctype='I'; t2->ival=(t2->bval?1:0); break;
			case 'S': break;
			case 'T': ctype='T'; break;
			case 'V': ctype='V'; break;
			default: ctype='I'; t2->ival=0;
			};
			break;
		case 'F':
			switch(t2->vtype)
			{
			case 'I': ctype='F'; t2->fval=(double)t2->ival; break;
			case 'F': _ASSERT(false); ctype='F'; break;
			case 'B': ctype='F'; t2->fval=(t2->bval?1.0:0.0); break;
			case 'S': break;
			case 'T': break;
			case 'V': ctype='V'; break;
			default: ctype='F'; t2->fval=0.0; 
			};
			break;
		case 'B':
			switch(t2->vtype)
			{
			case 'I': ctype='I'; t1->ival=(t1->bval?1:0); break;
			case 'F': ctype='F'; t1->fval=(t1->bval?1.0:0.0); break;
			case 'B': _ASSERT(false); ctype='B'; break;
			case 'S': break;
			case 'T': break;
			case 'V': ctype='V'; break;
			default: ctype='B'; t2->bval=false;
			};
			break;
		case 'S':
			switch(t2->vtype)
			{
			case 'I': break;
			case 'F': break;
			case 'B': break;
			case 'S': _ASSERT(false); ctype='S'; break;
			case 'T': ctype='T'; t1->ival=myatoi64(t1->sval); break;
			case 'V': ctype='V'; break;
			default: ctype='S'; t2->sval[0]=0; 
			};
			break;
		case 'T':
			switch(t2->vtype)
			{
			case 'I': ctype='T'; break;
			case 'F': break;
			case 'B': break;
			case 'S': ctype='T'; t2->ival=myatoi64(t2->sval); break;
			case 'T': _ASSERT(false); ctype='T'; break;
			case 'V': ctype='V'; break;
			default: ctype='T'; t2->ival=0; 
			};
			break;
		case 'V':
			if(forAssign)
				ctype=t2->vtype;
			else
				ctype='V'; 
			break;
		default: // NULL or tag doesn't exist
			switch(t2->vtype)
			{
			case 'I': ctype='I'; t1->ival=0; break;
			case 'F': ctype='F'; t1->fval=0.0; break;
			case 'B': ctype='B'; t1->bval=false; break;
			case 'S': ctype='S'; t1->sval[0]=0; break;
			case 'T': ctype='T'; t1->ival=0; break;
			case 'V': ctype='V'; break;
			default: ctype='B'; t1->bval=t2->bval=false; 
			};
		};
	}
	return ctype;
}
// Comma-delimited parameter list of expressions
int ExprTok::EvalExprParms(const char *ebeg, const char *eend, int eparms, FIXINFO *pfix)
{
	// No parms expected
	if(eparms<1)
	{
		// Skip leading whitespace
		const char *lptr;
		for(lptr=ebeg;(lptr<eend)&&myisspace(*lptr);lptr++)
			;
		if(lptr<eend)
		{
			ParseError(-1,ebeg,eend,"Parameter passed to void function");
			return -1;
		}
		return 1;
	}
	// Up to 8 params supported now
	else if(eparms>sizeof(params)/sizeof(ExprTok*))
	{
		ParseError(-1,ebeg,eend,"Max 8 parameters per function supported");
		return -1;
	}
	const char *pbeg=ebeg;
	for(nparams=0;nparams<eparms;nparams++)
	{
		// Find end of param, escaping commas inside strings
		ExprTok *ctok=new ExprTok;
		ctok->pnotify=pnotify;
		const char *pend;
		for(pend=pbeg;(pend<eend)&&(*pend!=',');)
		{
			if(*pend=='\'')
			{
				ctok->vtype='S';
				ctok->exprBeg=pend;
				pend=MatchSingleQuote(pend,eend,ctok->sval,sizeof(ctok->sval));
				if(!pend)
				{
					delete ctok;
					return -1; // unmatched quote
				}
				pend++;
				ctok->exprEnd=pend;
				continue; // keep going till we actually find the comma
			}
			else
				pend++;
		}
		if(ctok->vtype!='S')
		{
			if(ctok->EvalExpr(pbeg,pend,pfix)<0)
			{
				delete ctok;
				return -1;
			}
		}
		params[nparams]=ctok; 
		pbeg=(*pend==',')?pend+1:pend;
	}
	return 1;
}
// Evaluates complex expressions containing comparison operators, arithmetic operators, functions,
// integrals, decimals, and strings.
int ExprTok::EvalExpr(const char *ebeg, const char *eend, FIXINFO *pfix)
{
	// Null value
	if(ebeg>=eend)
	{
		//vtype='S';
		vtype=0;
		exprBeg=ebeg;
		exprEnd=ebeg +4;
		memcpy(sval,"\0NULL\0",6);
		return 1;
	}

	// Find && || operators
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			tptr=tend +1;
			continue;
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		int olen=0;
		if(!strncmp(tptr,"AND",3))
		   olen=3;
		else if(!strncmp(tptr,"OR",2))
		   olen=2;
		else if(!strncmp(tptr,"&&",2)||
		   !strncmp(tptr,"||",2))
		   olen=2;
		// [tbeg,tptr) op [tptr+olen,eend)
		if(olen>0)
		{
			vtype='B';
			exprBeg=ebeg;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			// Evaluate sub-expressions
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalExpr(ebeg,tptr,pfix)<0)
				return -1;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[1]->EvalExpr(tptr +olen,eend,pfix)<0)
				return -1;
			if(params[0]->vtype!='B')
			{
				ParseError(-1,params[0]->exprBeg,params[0]->exprEnd,"Non-boolean expression");
				return -1;
			}
			else if(params[1]->vtype!='B')
			{
				ParseError(-1,params[1]->exprBeg,params[1]->exprEnd,"Non-boolean expression");
				return -1;
			}
			// Evaluate operators
			if((!strcmp(oval,"AND"))||(!strcmp(oval,"&&")))
			{
				bval=(params[0]->bval && params[1]->bval);
				return 1;
			}
			else if((!strcmp(oval,"OR"))||(!strcmp(oval,"||")))
			{
				bval=(params[0]->bval || params[1]->bval);
				return 1;
			}
			ParseError(-1,ebeg,eend,"Unknown operator %s",oval);
			return -1;
		}
		tptr++;
	}

	// Find comparison operators
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			tptr=tend +1;
			continue;
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		// Comparison operators
		int olen=0;
		if(!strncmp(tptr,"==",2)||
		   !strncmp(tptr,"!=",2)||
		   !strncmp(tptr,">=",2)||
		   !strncmp(tptr,"<=",2))
			olen=2;
		// Comparison operators
		else if(!strncmp(tptr,"<",1)||
				!strncmp(tptr,">",1))
			olen=1;
		// [tbeg,tptr) op [tptr+olen,eend)
		if(olen>0)
		{		
			vtype='B';
			exprBeg=ebeg;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			// Evaluate sub-expressions
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalExpr(ebeg,tptr,pfix)<0)
				return -1;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[1]->EvalExpr(tptr +olen,eend,pfix)<0)
				return -1;
			// Type promotion
			char ctype=PromoteType(params[0],params[1]);
			if(!ctype)
			{
				ParseError(-1,ebeg,eend,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
				return -1;
			}
			// Evaluate operators
			if(!strcmp(oval,"=="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival==params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval==params[1]->fval)?true:false; break;
				case 'S': 
				{
					if((!params[0]->sval[0])&&(!_stricmp(params[0]->sval +1,"NULL")))
						bval=(params[1]->sval[0]?false:true);
					else if((!params[1]->sval[0])&&(!_stricmp(params[1]->sval +1,"NULL")))
						bval=(params[0]->sval[0]?false:true);
					else
						bval=(_stricmp(params[0]->sval,params[1]->sval))?false:true; 
					break;
				}
				case 'B': bval=(params[0]->bval==params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival==params[1]->ival)?true:false; break;
				case 'V': 
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					switch(params[cidx]->vtype)
					{
					case 'I': bval=true; break;
					case 'F': bval=true; break;
					case 'S': bval=true; break;
					case 'B': bval=true; break;
					case 'T': bval=true; break;
					};
					break;
				}
				default: 
					ParseError(-1,ebeg,eend,"Unsupported type %c for == operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,"!="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival==params[1]->ival)?false:true; break;
				case 'F': bval=(params[0]->fval==params[1]->fval)?false:true; break;
				case 'S': 
				{
					if((!params[0]->sval[0])&&(!_stricmp(params[0]->sval +1,"NULL")))
						bval=(params[1]->sval[0]?true:false);
					else if((!params[1]->sval[0])&&(!_stricmp(params[1]->sval +1,"NULL")))
						bval=(params[0]->sval[0]?true:false);
					else
						bval=(_stricmp(params[0]->sval,params[1]->sval))?true:false; 
					break;
				}
				case 'B': bval=(params[0]->bval==params[1]->bval)?false:true; break;
				case 'T': bval=(params[0]->ival==params[1]->ival)?false:true; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					switch(params[cidx]->vtype)
					{
					case 'I': bval=true; break;
					case 'F': bval=true; break;
					case 'S': bval=true; break;
					case 'B': bval=true; break;
					case 'T': bval=true; break;
					};
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for != operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,">="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival>=params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval>=params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)>=0)?true:false; break;
				case 'B': bval=(params[0]->bval>=params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival>=params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					switch(params[cidx]->vtype)
					{
					case 'I': bval=true; break;
					case 'F': bval=true; break;
					case 'S': bval=true; break;
					case 'B': bval=true; break;
					case 'T': bval=true; break;
					};
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for >= operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,"<="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival<=params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval<=params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)<=0)?true:false; break;
				case 'B': bval=(params[0]->bval<=params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival<=params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					switch(params[cidx]->vtype)
					{
					case 'I': bval=true; break;
					case 'F': bval=true; break;
					case 'S': bval=true; break;
					case 'B': bval=true; break;
					case 'T': bval=true; break;
					};
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for <= operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,">"))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival>params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval>params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)>0)?true:false; break;
				case 'B': bval=(params[0]->bval>params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival>params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					switch(params[cidx]->vtype)
					{
					case 'I': bval=true; break;
					case 'F': bval=true; break;
					case 'S': bval=true; break;
					case 'B': bval=true; break;
					case 'T': bval=true; break;
					};
					break;
				}
				default: 
					ParseError(-1,ebeg,eend,"Unsupported type %c for > operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,"<"))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival<params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval<params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)<0)?true:false; break;
				case 'B': bval=(params[0]->bval<params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival<params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					switch(params[cidx]->vtype)
					{
					case 'I': bval=true; break;
					case 'F': bval=true; break;
					case 'S': bval=true; break;
					case 'B': bval=true; break;
					case 'T': bval=true; break;
					};
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for < operator",ctype);
					return -1;
				};
			}
			else
			{
				ParseError(-1,ebeg,eend,"Unknown comparison operator %s",oval);
				return -1;
			}
			return 1;
		}
		tptr++;
	}

	// Find arithmetic operators
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			tptr=tend +1;
			continue;
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		// Binary arithmetic operators
		int olen=0;
		if(!strncmp(tptr,"+",1)||
		   !strncmp(tptr,"-",1)||
		   !strncmp(tptr,"*",1)||
		   !strncmp(tptr,"/",1))
		{
			olen=1; 
		}
		if(olen>0)
		{
			vtype='I';
			exprBeg=ebeg;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			// Evaluate sub-expressions
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalExpr(ebeg,tptr,pfix)<0)
				return -1;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[1]->EvalExpr(tptr +olen,eend,pfix)<0)
				return -1;
			vtype=PromoteType(params[0],params[1]);
			if(!vtype)
			{
				ParseError(-1,ebeg,eend,"No promotion supported on type %c %s type %c",params[0]->vtype,oval,params[1]->vtype);
				return -1;
			}
			if(!strcmp(oval,"+"))
			{				
				// Allow string concatenation
				if((params[0]->vtype=='S')&&(params[1]->vtype=='S'))
				{
					vtype='S';
					int slen1=(int)strlen(params[0]->sval);
					if(slen1>=sizeof(sval)) 
						slen1=sizeof(sval) -1;
					int slen2=(int)strlen(params[1]->sval);
					if(slen1 +slen2>=sizeof(sval)) 
						slen2=sizeof(sval) -1 -slen1;
					int tlen=slen1 +slen2;
					strncpy(sval,params[0]->sval,slen1);
					if(slen2>0)
						strncpy(sval +slen1,params[1]->sval,slen2);
					sval[tlen]=0;
					return 1;
				}
				// Compute t2=t1 +diff(sec)
				else if((params[0]->vtype=='T')&&(params[1]->vtype=='I'))
				{
					vtype='T';
					ival=0; fval=0; sval[0]=0;
					DWORD dd=(DWORD)(params[0]->ival/1000000);
					DWORD tt=(DWORD)(params[0]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm;
					memset(&gtm,0,sizeof(struct tm));
					gtm.tm_year=(dd/10000) -1900;
					gtm.tm_mon=(dd%10000)/100 -1;
					gtm.tm_mday=(dd%100);
					gtm.tm_hour=tt/10000;
					gtm.tm_min=(tt%10000)/100;
					gtm.tm_sec=(tt%100);
					gtm.tm_isdst=-1;
					time_t gts=mktime(&gtm);
					if(!gts)
						return 1; // failed time conversion
					gts+=(int)params[1]->ival;
					// Convert back to components
					struct tm *ntm=localtime(&gts);
					if(!ntm)
						return 1; // failed new time conversion
					gtm=*ntm;
					dd=(gtm.tm_year +1900)*10000 +(gtm.tm_mon +1)*100 +(gtm.tm_mday);
					tt=(gtm.tm_hour *10000) +(gtm.tm_min *100) +(gtm.tm_sec);
					ival=((LONGLONG)dd)*1000000 +tt;
					fval=(double)ival;
					#ifdef WIN32
					sprintf(sval,"%I64d",ival);
					#else
					sprintf(sval,"%lld",ival);
					#endif
					return 1;
				}
				else if(vtype=='I')
				{
					ival=params[0]->ival +params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval +params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for + operator",vtype);
				return -1;
			}
			else if(!strcmp(oval,"-"))
			{
				// Compute diff(sec)=t1 -t2
				if((params[0]->vtype=='T')&&(params[1]->vtype=='T'))
				{
					vtype='I';
					ival=0; fval=0;
					DWORD dd=(DWORD)(params[0]->ival/1000000);
					DWORD tt=(DWORD)(params[0]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm1;
					memset(&gtm1,0,sizeof(struct tm));
					gtm1.tm_year=(dd/10000) -1900;
					gtm1.tm_mon=(dd%10000)/100 -1;
					gtm1.tm_mday=(dd%100);
					gtm1.tm_hour=tt/10000;
					gtm1.tm_min=(tt%10000)/100;
					gtm1.tm_sec=(tt%100);
					gtm1.tm_isdst=-1;
					time_t gts1=mktime(&gtm1);
					if(!gts1)
						return 1; // failed time conversion

					dd=(DWORD)(params[1]->ival/1000000);
					tt=(DWORD)(params[1]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm2;
					memset(&gtm2,0,sizeof(struct tm));
					gtm2.tm_year=(dd/10000) -1900;
					gtm2.tm_mon=(dd%10000)/100 -1;
					gtm2.tm_mday=(dd%100);
					gtm2.tm_hour=tt/10000;
					gtm2.tm_min=(tt%10000)/100;
					gtm2.tm_sec=(tt%100);
					gtm2.tm_isdst=-1;
					time_t gts2=mktime(&gtm2);
					if(!gts2)
						return 1; // failed time conversion
					ival=gts1 -gts2;
					fval=(float)ival;
					return 1;
				}
				// Compute t2=t1 -diff(sec)
				else if((params[0]->vtype=='T')&&(params[1]->vtype=='I'))
				{
					vtype='T';
					ival=0; fval=0; sval[0]=0;
					DWORD dd=(DWORD)(params[0]->ival/1000000);
					DWORD tt=(DWORD)(params[0]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm;
					memset(&gtm,0,sizeof(struct tm));
					gtm.tm_year=(dd/10000) -1900;
					gtm.tm_mon=(dd%10000)/100 -1;
					gtm.tm_mday=(dd%100);
					gtm.tm_hour=tt/10000;
					gtm.tm_min=(tt%10000)/100;
					gtm.tm_sec=(tt%100);
					gtm.tm_isdst=-1;
					time_t gts=mktime(&gtm);
					if(!gts)
						return 1; // failed time conversion
					gts-=(int)params[1]->ival;
					// Convert back to components
					struct tm *ntm=localtime(&gts);
					if(!ntm)
						return 1; // failed new time conversion
					gtm=*ntm;
					dd=(gtm.tm_year +1900)*10000 +(gtm.tm_mon +1)*100 +(gtm.tm_mday);
					tt=(gtm.tm_hour *10000) +(gtm.tm_min *100) +(gtm.tm_sec);
					ival=((LONGLONG)dd)*1000000 +tt;
					fval=(double)ival;
					#ifdef WIN32
					sprintf(sval,"%I64d",ival);
					#else
					sprintf(sval,"%lld",ival);
					#endif
					return 1;
				}
				else if(vtype=='I')
				{
					ival=params[0]->ival -params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval -params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for - operator",vtype);
				return -1;
			}
			else if(!strcmp(oval,"*"))
			{
				if(vtype=='I')
				{
					ival=params[0]->ival *params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval *params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for * operator",vtype);
				return -1;
			}
			else if(!strcmp(oval,"/"))
			{
				if(vtype=='I')
				{
					ival=params[0]->ival /params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval /params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for / operator",vtype);
				return -1;
			}
			return -1;
		}
		// Unary arithmetic operators
		else if(!strncmp(tptr,"NOT",3))
			olen=3;
		else if(!strncmp(tptr,"!",1))
			olen=1;
		if(olen>0)
		{
			vtype='B';
			exprBeg=tptr;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalExpr(tptr +olen,eend,pfix)<0)
				return -1;
			if(params[0]->vtype=='B')
			{
				bval=!params[0]->bval;
				return 1;
			}
			else if(params[0]->vtype=='I')
			{
				bval=(params[0]->bval?true:false);
				return 1;
			}
			ParseError(-1,ebeg,eend,"Unsupported type %c for %s operator",params[0]->vtype,oval);
			return -1;
		}
		tptr++;
	}

	// Is this a function?
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			// Function name
			char tname[256]={0};
			const char *tbeg;
			for(tbeg=ebeg;(tbeg<eend)&&(myisspace(*tbeg));tbeg++)
				;
			int tlen=(int)(tptr -tbeg);
			if(tlen>=256) tlen=255;
			strncpy(tname,tbeg,tlen); tname[tlen]=0;
			// Evaluate the built-in functions
			// tag(n)
			if(!strcmp(tname,"tag"))
			{
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tname=params[0]->sval;
					FIXTAG *ptag=pfix->GetTag(tname);
					if(ptag)
					{
						int vlen=ptag->vlen;
						if(vlen>sizeof(sval)-1)
							vlen=sizeof(sval)-1;
						strncpy(sval,ptag->val,vlen); sval[vlen]=0;
					}
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				ExprTok ttok;
				ttok.pnotify=pnotify;
				// If the tag is missing or null, then no type
				if(!*sval)
					vtype=0;
				// If the tag's value an integer, then use that type
				//ival=pfix->TagInt(tno);
				else if(ttok.EvalIntegralExpr(sval,sval +strlen(sval))>0)
				{
					vtype='I';
					ival=ttok.ival;
					//fval=(double)ival; // for promotion
					return 1;
				}
				// If the tag's value an decimal, then use that type
				//fval=pfix->TagFloat(tno);
				else if(ttok.EvalDecimalExpr(sval,sval +strlen(sval))>0)
				{
					vtype='F';
					fval=ttok.fval;
					//ival=(LONGLONG)fval; // for promotion
					return 1;
				}
				// Otherwise, assume it's a string
				else
					vtype='S';
				return 1;
			}
			// tagstr(n)
			else if(!strcmp(tname,"tagstr"))
			{
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tname=params[0]->sval;
					FIXTAG *ptag=pfix->GetTag(tname);
					if(ptag)
					{
						int vlen=ptag->vlen;
						if(vlen>sizeof(sval)-1)
							vlen=sizeof(sval)-1;
						strncpy(sval,ptag->val,vlen); sval[vlen]=0;
					}
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				vtype='S';
				return 1;
			}
			// tagint(n)
			else if(!strcmp(tname,"tagint"))
			{
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tname=params[0]->sval;
					FIXTAG *ptag=pfix->GetTag(tname);
					if(ptag)
					{
						int vlen=ptag->vlen;
						if(vlen>sizeof(sval)-1)
							vlen=sizeof(sval)-1;
						strncpy(sval,ptag->val,vlen); sval[vlen]=0;
					}
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				ival=myatoi64(sval);
				vtype='I';
				return 1;
			}
			// tagdec(n)
			else if(!strcmp(tname,"tagdec"))
			{
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tname=params[0]->sval;
					FIXTAG *ptag=pfix->GetTag(tname);
					if(ptag)
					{
						int vlen=ptag->vlen;
						if(vlen>sizeof(sval)-1)
							vlen=sizeof(sval)-1;
						strncpy(sval,ptag->val,vlen); sval[vlen]=0;
					}
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				fval=atof(sval);
				vtype='F';
				return 1;
			}
			// hastag(n)
			else if(!strcmp(tname,"hastag"))
			{
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					bval=pfix->GetTag(tno)?true:false;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tname=params[0]->sval;
					bval=pfix->GetTag(tname)?true:false;
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				vtype='B';
				return 1;
			}
			// substr(str,start,len)
			else if(!strcmp(tname,"substr"))
			{
				vtype='S';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr+1,tend,3,pfix)<0)
					return -1;
				if(params[0]->vtype!='S')
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1;
				}
				else if(params[1]->vtype!='I')
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 2");
					return -1;
				}
				else if(params[2]->vtype!='I')
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 3");
					return -1;
				}
				sval[0]=0;
				int tlen=(int)strlen(params[0]->sval);
				int start=(int)params[1]->ival;				
				if(start>=tlen) // bad start, return empty
					return 1;
				int slen=(int)params[2]->ival;
				if(slen>tlen -start)
					slen=tlen -start;
				if(slen>=sizeof(sval))
					slen=sizeof(sval) -1;
				if(slen<1) // bad length, return empty
					return 1;
				strncpy(sval,params[0]->sval +start,slen); sval[slen]=0;
				return 1;
			}
			// strlen(str)
			else if(!strcmp(tname,"strlen"))
			{
				vtype='I';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				if(params[0]->vtype!='S')
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1;
				}
				ival=strlen(params[0]->sval);
				return 1;
			}
			// strstr(str,sub)
			else if(!strcmp(tname,"strstr"))
			{
				vtype='S';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,2,pfix)<0)
					return -1;
				if(params[0]->vtype!='S')
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1;
				}
				if(params[1]->vtype!='S')
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 2");
					return -1;
				}
				const char *sptr=stristr(params[0]->sval,params[1]->sval);
				if(sptr)
				{
					int slen=(int)strlen(sptr);
					if(slen>=sizeof(sval))
						slen=sizeof(sval) -1;
					strncpy(sval,sptr,slen); sval[slen]=0;
				}
				else
					sval[0]=0;
				return 1;
			}
			// time(str)
			else if(!strcmp(tname,"time"))
			{				
				vtype='T';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalExprParms(tptr +1,tend,1,pfix)<0)
					return -1;
				// time(0) returns current time GMT
				if((params[0]->vtype=='I')&&(params[0]->ival==0))
				{
				#ifdef WIN32
					SYSTEMTIME tsys;
					GetSystemTime(&tsys);
					ival=(tsys.wYear *10000000000) +(tsys.wMonth*100000000) +(tsys.wDay*1000000) +
						 (tsys.wHour *10000) +(tsys.wMinute*100) +(tsys.wSecond);
				#else
					ival=((LONGLONG)WSDate())*1000000 +WSTime();
				#endif
					fval=(double)ival;
					#ifdef WIN32
					sprintf(sval,"%I64d",ival);
					#else
					sprintf(sval,"%lld",ival);
					#endif
					return 1;
				}
				else if(params[0]->vtype!='S')
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1; // yyyymmdd-HH:MM:SS only
				}
				strncpy(sval,params[0]->sval,17); sval[17]=0;
				if((sval[8]!='-')||(sval[11]!=':')||(sval[14]!=':'))
				{
					ParseError(-1,ebeg,eend,"Time format error: expected yyyymmdd-HH:MM:SS");
					return -1; // yyyymmdd-HH:MM:SS only
				}
				int dd=myatoi(sval),hh=myatoi(sval +9),mm=myatoi(sval +12),ss=myatoi(sval +15);
				ival=((LONGLONG)dd *1000000) +(hh*10000) +(mm*100) +ss;
				fval=(double)ival;
				return 1;
			}
			// else add other functions here
			// Parenthesis for order of operations only
			else if(!*tname)
			{
				return EvalExpr(tptr +1,tend,pfix);
			}
			ParseError(-1,tptr,eend,"Unknown function %s",tname);
			return -1; // Unknown function
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		tptr++;
	}

	// Skip leading whitespace
	const char *lptr;
	for(lptr=ebeg;(lptr<eend)&&myisspace(*lptr);lptr++)
		;
	// Skip trailing whitespace
	const char *rptr;
	for(rptr=eend;(rptr>lptr)&&myisspace(rptr[-1]);rptr--)
		;
	// Is this a string literal?
	for(const char *tptr=lptr;tptr<rptr;)
	{
		if(*tptr=='\'')
		{
			const char *send=MatchSingleQuote(tptr,rptr,sval,sizeof(sval));
			if(!send)
				return -1; // unmatched quote
			vtype='S';
			exprBeg=tptr;
			exprEnd=send +1;
			return 1;
		}
		tptr++;
	}

	// Is this an integer?
	if(EvalIntegralExpr(lptr,rptr)>0)
		return 1;

	// Is this a decimal?
	if(EvalDecimalExpr(lptr,rptr)>0)
		return 1;

	// Intrinsic mnemonic
	// NULL
	if((rptr -lptr==4)&&(!strincmp(lptr,"NULL",4)))
	{
		//vtype='S';
		vtype=0;
		exprBeg=ebeg;
		exprEnd=ebeg +4;
		memcpy(sval,"\0NULL\0",6);
		return 1;
	}
	// TRUE
	else if((rptr -lptr==4)&&(!strincmp(lptr,"TRUE",4)))
	{
		vtype='B';
		exprBeg=ebeg;
		exprEnd=ebeg +4;
		bval=true;
		return 1;
	}
	// FALSE
	else if((rptr -lptr==5)&&(!strincmp(lptr,"FALSE",5)))
	{
		vtype='B';
		exprBeg=ebeg;
		exprEnd=ebeg +5;
		bval=false;
		return 1;
	}
	// Variables must start with character
	else if(isalpha(*lptr))
	{
		vtype='V';
		exprBeg=lptr;
		exprEnd=rptr;
		int slen=(int)(rptr -lptr);
		strncpy(sval,lptr,slen); sval[slen]=0;
		return 1;
	}

	_ASSERT(false);
	ParseError(-1,lptr,rptr,"Syntax error");
	return -1;
}

// Comma-delimited parameter list of expressions
int ExprTok::EvalDepParms(FIXINFO *pfix, const char *ebeg, const char *eend, int eparms)
{
	// No parms expected
	if(eparms<1)
	{
		// Skip leading whitespace
		const char *lptr;
		for(lptr=ebeg;(lptr<eend)&&myisspace(*lptr);lptr++)
			;
		if(lptr<eend)
		{
			ParseError(-1,ebeg,eend,"Parameter passed to void function");
			return -1;
		}
		return 1;
	}
	// Up to 8 params supported now
	else if(eparms>sizeof(params)/sizeof(ExprTok*))
	{
		ParseError(-1,ebeg,eend,"Max 8 parameters per function supported");
		return -1;
	}
	const char *pbeg=ebeg;
	for(nparams=0;nparams<eparms;nparams++)
	{
		// Find end of param, escaping commas inside strings
		ExprTok *ctok=new ExprTok;
		ctok->pnotify=pnotify;
		const char *pend;
		for(pend=pbeg;(pend<eend)&&(*pend!=',');)
		{
			if(*pend=='\'')
			{
				ctok->vtype='S';
				ctok->exprBeg=pend;
				pend=MatchSingleQuote(pend,eend,ctok->sval,sizeof(ctok->sval));
				if(!pend)
				{
					delete ctok;
					return -1; // unmatched quote
				}
				pend++;
				ctok->exprEnd=pend;
				continue; // keep going till we actually find the comma
			}
			else
				pend++;
		}
		if(ctok->vtype!='S')
		{
			if(ctok->EvalDeps(pfix,pbeg,pend)<0)
			{
				delete ctok;
				return -1;
			}
		}
		params[nparams]=ctok; 
		pbeg=(*pend==',')?pend+1:pend;
	}
	return 1;
}
// Sets the tags required to be evaluated for the complex expression
int ExprTok::EvalDeps(FIXINFO *pfix, const char *ebeg, const char *eend)
{
	// Null value
	if(ebeg>=eend)
	{
		//vtype='S';
		vtype=0;
		exprBeg=ebeg;
		exprEnd=ebeg +4;
		memcpy(sval,"\0NULL\0",6);
		return 1;
	}

	// Find && || operators
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			tptr=tend +1;
			continue;
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		int olen=0;
		if(!strncmp(tptr,"AND",3))
		   olen=3;
		else if(!strncmp(tptr,"OR",2))
		   olen=2;
		else if(!strncmp(tptr,"&&",2)||
		   !strncmp(tptr,"||",2))
		   olen=2;
		// [tbeg,tptr) op [tptr+olen,eend)
		if(olen>0)
		{
			vtype='B';
			exprBeg=ebeg;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			// Evaluate sub-expressions
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalDeps(pfix,ebeg,tptr)<0)
				return -1;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[1]->EvalDeps(pfix,tptr +olen,eend)<0)
				return -1;
			if(params[0]->vtype!='B')
			{
				ParseError(-1,params[0]->exprBeg,params[0]->exprEnd,"Non-boolean expression");
				return -1;
			}
			else if(params[1]->vtype!='B')
			{
				ParseError(-1,params[1]->exprBeg,params[1]->exprEnd,"Non-boolean expression");
				return -1;
			}
			// Evaluate operators
			if((!strcmp(oval,"AND"))||(!strcmp(oval,"&&")))
			{
				bval=(params[0]->bval && params[1]->bval);
				return 1;
			}
			else if((!strcmp(oval,"OR"))||(!strcmp(oval,"||")))
			{
				bval=(params[0]->bval || params[1]->bval);
				return 1;
			}
			ParseError(-1,ebeg,eend,"Unknown operator %s",oval);
			return -1;
		}
		tptr++;
	}

	// Find comparison operators
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			tptr=tend +1;
			continue;
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		// Comparison operators
		int olen=0;
		if(!strncmp(tptr,"==",2)||
		   !strncmp(tptr,"!=",2)||
		   !strncmp(tptr,">=",2)||
		   !strncmp(tptr,"<=",2))
			olen=2;
		// Comparison operators
		else if(!strncmp(tptr,"<",1)||
				!strncmp(tptr,">",1))
			olen=1;
		// [tbeg,tptr) op [tptr+olen,eend)
		if(olen>0)
		{		
			vtype='B';
			exprBeg=ebeg;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			// Evaluate sub-expressions
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalDeps(pfix,ebeg,tptr)<0)
				return -1;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[1]->EvalDeps(pfix,tptr +olen,eend)<0)
				return -1;
			// Type promotion
			char ctype=PromoteType(params[0],params[1]);
			if(!ctype)
			{
				ParseError(-1,ebeg,eend,"No promotion supported on type %c %s type %c",params[0]->vtype,oval,params[1]->vtype);
				return -1;
			}
			// Evaluate operators
			if(!strcmp(oval,"=="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival==params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval==params[1]->fval)?true:false; break;
				case 'S': 
				{
					if((!params[0]->sval[0])&&(!_stricmp(params[0]->sval +1,"NULL")))
						bval=(params[1]->sval[0]?false:true);
					else if((!params[1]->sval[0])&&(!_stricmp(params[1]->sval +1,"NULL")))
						bval=(params[0]->sval[0]?false:true);
					else
						bval=(_stricmp(params[0]->sval,params[1]->sval))?false:true; 
					break;
				}
				case 'B': bval=(params[0]->bval==params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival==params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					int slen=0;
					if(params[vidx]->ival>0)
						pfix->AddTag((int)params[vidx]->ival,0,0,params[cidx]->sval,0,slen+2);
					else
						pfix->AddTag(0,params[vidx]->sval,(int)strlen(params[vidx]->sval),params[cidx]->sval,0,slen+2);
					bval=true;
					break;
				}
				default: 
					ParseError(-1,ebeg,eend,"Unsupported type %c for == operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,"!="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival==params[1]->ival)?false:true; break;
				case 'F': bval=(params[0]->fval==params[1]->fval)?false:true; break;
				case 'S': 
				{
					if((!params[0]->sval[0])&&(!_stricmp(params[0]->sval +1,"NULL")))
						bval=(params[1]->sval[0]?true:false);
					else if((!params[1]->sval[0])&&(!_stricmp(params[1]->sval +1,"NULL")))
						bval=(params[0]->sval[0]?true:false);
					else
						bval=(_stricmp(params[0]->sval,params[1]->sval))?true:false; 
					break;
				}
				case 'B': bval=(params[0]->bval==params[1]->bval)?false:true; break;
				case 'T': bval=(params[0]->ival==params[1]->ival)?false:true; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					int slen=0;
					if(params[vidx]->ival>0)
						pfix->AddTag((int)params[vidx]->ival,0,0,params[cidx]->sval,0,slen+2);
					else
						pfix->AddTag(0,params[vidx]->sval,(int)strlen(params[vidx]->sval),params[cidx]->sval,0,slen+2);
					bval=true;
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for != operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,">="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival>=params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval>=params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)>=0)?true:false; break;
				case 'B': bval=(params[0]->bval>=params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival>=params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					int slen=0;
					if(params[vidx]->ival>0)
						pfix->AddTag((int)params[vidx]->ival,0,0,params[cidx]->sval,0,slen+2);
					else
						pfix->AddTag(0,params[vidx]->sval,(int)strlen(params[vidx]->sval),params[cidx]->sval,0,slen+2);
					bval=true;
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for >= operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,"<="))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival<=params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval<=params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)<=0)?true:false; break;
				case 'B': bval=(params[0]->bval<=params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival<=params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					int slen=0;
					if(params[vidx]->ival>0)
						pfix->AddTag((int)params[vidx]->ival,0,0,params[cidx]->sval,0,slen+2);
					else
						pfix->AddTag(0,params[vidx]->sval,(int)strlen(params[vidx]->sval),params[cidx]->sval,0,slen+2);
					bval=true;
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for <= operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,">"))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival>params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval>params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)>0)?true:false; break;
				case 'B': bval=(params[0]->bval>params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival>params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					int slen=0;
					if(params[vidx]->ival>0)
						pfix->AddTag((int)params[vidx]->ival,0,0,params[cidx]->sval,0,slen+2);
					else
						pfix->AddTag(0,params[vidx]->sval,(int)strlen(params[vidx]->sval),params[cidx]->sval,0,slen+2);
					bval=true;
					break;
				}
				default: 
					ParseError(-1,ebeg,eend,"Unsupported type %c for > operator",ctype);
					return -1;
				};
			}
			else if(!strcmp(oval,"<"))
			{
				switch(ctype)
				{
				case 'I': bval=(params[0]->ival<params[1]->ival)?true:false; break;
				case 'F': bval=(params[0]->fval<params[1]->fval)?true:false; break;
				case 'S': bval=(_stricmp(params[0]->sval,params[1]->sval)<0)?true:false; break;
				case 'B': bval=(params[0]->bval<params[1]->bval)?true:false; break;
				case 'T': bval=(params[0]->ival<params[1]->ival)?true:false; break;
				case 'V':
				{
					int vidx=(params[0]->vtype=='V')?0:1; // the variable
					int cidx=(vidx>0)?0:1; // the constant
					int slen=0;
					if(params[vidx]->ival>0)
						pfix->AddTag((int)params[vidx]->ival,0,0,params[cidx]->sval,0,slen+2);
					else
						pfix->AddTag(0,params[vidx]->sval,(int)strlen(params[vidx]->sval),params[cidx]->sval,0,slen+2);
					bval=true;
					break;
				}
				default:
					ParseError(-1,ebeg,eend,"Unsupported type %c for < operator",ctype);
					return -1;
				};
			}
			else
			{
				ParseError(-1,ebeg,eend,"Unknown comparison operator %s",oval);
				return -1;
			}
			return 1;
		}
		tptr++;
	}

	// Find arithmetic operators
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			tptr=tend +1;
			continue;
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		// Binary arithmetic operators
		int olen=0;
		if(!strncmp(tptr,"+",1)||
		   !strncmp(tptr,"-",1)||
		   !strncmp(tptr,"*",1)||
		   !strncmp(tptr,"/",1))
		{
			olen=1; 
		}
		if(olen>0)
		{
			vtype='I';
			exprBeg=ebeg;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			// Evaluate sub-expressions
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalDeps(pfix,ebeg,tptr)<0)
				return -1;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[1]->EvalDeps(pfix,tptr +olen,eend)<0)
				return -1;
			vtype=PromoteType(params[0],params[1]);
			if(!vtype)
			{
				ParseError(-1,ebeg,eend,"No promotion supported on type %c %s type %c",params[0]->vtype,oval,params[1]->vtype);
				return -1;
			}
			if(!strcmp(oval,"+"))
			{				
				// Allow string concatenation
				if((params[0]->vtype=='S')&&(params[1]->vtype=='S'))
				{
					vtype='S';
					int slen1=(int)strlen(params[0]->sval);
					int slen2=(int)strlen(params[1]->sval);
					int tlen=slen1 +slen2;
					if(tlen>=sizeof(sval))
						tlen=sizeof(sval) -1;
					strncpy(sval,params[0]->sval,tlen);
					tlen-=slen1;
					if(tlen>0)
					{
						strncpy(sval +slen1,params[1]->sval,tlen);
						sval[slen1 +tlen]=0;
					}
					return 1;
				}
				// Compute t2=t1 +diff(sec)
				else if((params[0]->vtype=='T')&&(params[1]->vtype=='I'))
				{
					vtype='T';
					ival=0; fval=0; sval[0]=0;
					DWORD dd=(DWORD)(params[0]->ival/1000000);
					DWORD tt=(DWORD)(params[0]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm;
					memset(&gtm,0,sizeof(struct tm));
					gtm.tm_year=(dd/10000) -1900;
					gtm.tm_mon=(dd%10000)/100 -1;
					gtm.tm_mday=(dd%100);
					gtm.tm_hour=tt/10000;
					gtm.tm_min=(tt%10000)/100;
					gtm.tm_sec=(tt%100);
					gtm.tm_isdst=-1;
					time_t gts=mktime(&gtm);
					if(!gts)
						return 1; // failed time conversion
					gts+=(int)params[1]->ival;
					// Convert back to components
					struct tm *ntm=localtime(&gts);
					if(!ntm)
						return 1; // failed new time conversion
					gtm=*ntm;
					dd=(gtm.tm_year +1900)*10000 +(gtm.tm_mon +1)*100 +(gtm.tm_mday);
					tt=(gtm.tm_hour *10000) +(gtm.tm_min *100) +(gtm.tm_sec);
					ival=((LONGLONG)dd)*1000000 +tt;
					fval=(double)ival;
					#ifdef WIN32
					sprintf(sval,"%I64d",ival);
					#else
					sprintf(sval,"%lld",ival);
					#endif
					return 1;
				}
				else if(vtype=='I')
				{
					ival=params[0]->ival +params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval +params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for + operator",vtype);
				return -1;
			}
			else if(!strcmp(oval,"-"))
			{
				// Compute diff(sec)=t1 -t2
				if((params[0]->vtype=='T')&&(params[1]->vtype=='T'))
				{
					vtype='I';
					ival=0; fval=0;
					DWORD dd=(DWORD)(params[0]->ival/1000000);
					DWORD tt=(DWORD)(params[0]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm1;
					memset(&gtm1,0,sizeof(struct tm));
					gtm1.tm_year=(dd/10000) -1900;
					gtm1.tm_mon=(dd%10000)/100 -1;
					gtm1.tm_mday=(dd%100);
					gtm1.tm_hour=tt/10000;
					gtm1.tm_min=(tt%10000)/100;
					gtm1.tm_sec=(tt%100);
					gtm1.tm_isdst=-1;
					time_t gts1=mktime(&gtm1);
					if(!gts1)
						return 1; // failed time conversion

					dd=(DWORD)(params[1]->ival/1000000);
					tt=(DWORD)(params[1]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm2;
					memset(&gtm2,0,sizeof(struct tm));
					gtm2.tm_year=(dd/10000) -1900;
					gtm2.tm_mon=(dd%10000)/100 -1;
					gtm2.tm_mday=(dd%100);
					gtm2.tm_hour=tt/10000;
					gtm2.tm_min=(tt%10000)/100;
					gtm2.tm_sec=(tt%100);
					gtm2.tm_isdst=-1;
					time_t gts2=mktime(&gtm2);
					if(!gts2)
						return 1; // failed time conversion
					ival=gts1 -gts2;
					fval=(float)ival;
					return 1;
				}
				// Compute t2=t1 -diff(sec)
				else if((params[0]->vtype=='T')&&(params[1]->vtype=='I'))
				{
					vtype='T';
					ival=0; fval=0; sval[0]=0;
					DWORD dd=(DWORD)(params[0]->ival/1000000);
					DWORD tt=(DWORD)(params[0]->ival%1000000);
					// Convert to time_t from components
					struct tm gtm;
					memset(&gtm,0,sizeof(struct tm));
					gtm.tm_year=(dd/10000) -1900;
					gtm.tm_mon=(dd%10000)/100 -1;
					gtm.tm_mday=(dd%100);
					gtm.tm_hour=tt/10000;
					gtm.tm_min=(tt%10000)/100;
					gtm.tm_sec=(tt%100);
					gtm.tm_isdst=-1;
					time_t gts=mktime(&gtm);
					if(!gts)
						return 1; // failed time conversion
					gts-=(int)params[1]->ival;
					// Convert back to components
					struct tm *ntm=localtime(&gts);
					if(!ntm)
						return 1; // failed new time conversion
					gtm=*ntm;
					dd=(gtm.tm_year +1900)*10000 +(gtm.tm_mon +1)*100 +(gtm.tm_mday);
					tt=(gtm.tm_hour *10000) +(gtm.tm_min *100) +(gtm.tm_sec);
					ival=((LONGLONG)dd)*1000000 +tt;
					fval=(double)ival;
					#ifdef WIN32
					sprintf(sval,"%I64d",ival);
					#else
					sprintf(sval,"%lld",ival);
					#endif
					return 1;
				}
				else if(vtype=='I')
				{
					ival=params[0]->ival -params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval -params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for - operator",vtype);
				return -1;
			}
			else if(!strcmp(oval,"*"))
			{
				if(vtype=='I')
				{
					ival=params[0]->ival *params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval *params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for * operator",vtype);
				return -1;
			}
			else if(!strcmp(oval,"/"))
			{
				if(vtype=='I')
				{
					ival=params[0]->ival /params[1]->ival;
					return 1;
				}
				else if(vtype=='F')
				{
					fval=params[0]->fval /params[1]->fval;
					return 1;
				}
				ParseError(-1,ebeg,eend,"Unsupported type %c for / operator",vtype);
				return -1;
			}
			return -1;
		}
		// Unary arithmetic operators
		else if(!strncmp(tptr,"NOT",3))
			olen=3;
		else if(!strncmp(tptr,"!",1))
			olen=1;
		if(olen>0)
		{
			vtype='B';
			exprBeg=tptr;
			exprEnd=eend;
			strncpy(oval,tptr,olen); oval[olen]=0;
			nparams=0;
			params[nparams]=new ExprTok;
			params[nparams++]->pnotify=pnotify;
			if(params[0]->EvalDeps(pfix,tptr +olen,eend)<0)
				return -1;
			if(params[0]->vtype=='B')
			{
				bval=!params[0]->bval;
				return 1;
			}
			else if(params[0]->vtype=='I')
			{
				bval=(params[0]->bval?true:false);
				return 1;
			}
			ParseError(-1,ebeg,eend,"Unsupported type %c for %s operator",params[0]->vtype,oval);
			return -1;
		}
		tptr++;
	}

	// Is this a function?
	for(const char *tptr=ebeg;tptr<eend;)
	{
		if(*tptr=='\'')
		{
			const char *tend=MatchSingleQuote(tptr,eend,0,0);
			if(!tend)
				return -1; // unmatched quote
			tptr=tend +1;
			continue;
		}
		if(*tptr=='(')
		{
			const char *tend=MatchParen(tptr,eend);
			if(!tend)
				return -1; // unmatched parenthesis
			// Function name
			char tname[256]={0};
			const char *tbeg;
			for(tbeg=ebeg;(tbeg<eend)&&(myisspace(*tbeg));tbeg++)
				;
			int tlen=(int)(tptr -tbeg);
			if(tlen>=256) tlen=255;
			strncpy(tname,tbeg,tlen); tname[tlen]=0;
			// Evaluate the built-in functions
			// tag(n)
			if(!strcmp(tname,"tag"))
			{
				strcpy(vname,tname);
				vbtype='V';
				vtype='V';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					ival=tno;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tagname=params[0]->sval;
					strcpy(sval,tagname);
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				return 1;
			}
			// tagstr(n)
			else if(!strcmp(tname,"tagstr"))
			{
				strcpy(vname,tname);
				vbtype='S';
				vtype='S';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					ival=tno;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tagname=params[0]->sval;
					strcpy(sval,tagname);
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				return 1;
			}
			// tagint(n)
			else if(!strcmp(tname,"tagint"))
			{
				strcpy(vname,tname);
				vbtype='I';
				vtype='I';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					ival=tno;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tagname=params[0]->sval;
					strcpy(sval,tagname);
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				return 1;
			}
			// tagdec(n)
			else if(!strcmp(tname,"tagdec"))
			{
				strcpy(vname,tname);
				vbtype='F';
				vtype='F';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					ival=tno;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tagname=params[0]->sval;
					strcpy(sval,tagname);
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				return 1;
			}
			// hastag(n)
			else if(!strcmp(tname,"hastag"))
			{
				strcpy(vname,tname);
				vbtype='B';
				vtype='B';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				if(params[0]->vtype=='I')
				{
					int tno=(int)params[0]->ival;
					ival=tno;
				}
				else if(params[0]->vtype=='S')
				{
					const char *tagname=params[0]->sval;
					strcpy(sval,tagname);
				}
				else
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 1");
					return -1; // parameter must be integer
				}
				return 1;
			}
			// substr(str,start,len)
			else if(!strcmp(tname,"substr"))
			{
				strcpy(vname,tname);
				vbtype='S';
				vtype='S';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr+1,tend,3)<0)
					return -1;
				if((params[0]->vtype!='S')&&(params[0]->vtype!='V'))
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1;
				}
				else if(params[1]->vtype!='I')
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 2");
					return -1;
				}
				else if(params[2]->vtype!='I')
				{
					ParseError(-1,ebeg,eend,"Integer type expected for parameter 3");
					return -1;
				}
				sval[0]=0;
				// Dependency check only--don't evaluate
				//int tlen=(int)strlen(params[0]->sval);
				//int start=(int)params[1]->ival;				
				//if(start>=tlen) // bad start, return empty
				//	return 1;
				//int slen=(int)params[2]->ival;
				//if(slen>tlen -start)
				//	slen=tlen -start;
				//if(slen>=sizeof(sval))
				//	slen=sizeof(sval) -1;
				//if(slen<1) // bad length, return empty
				//	return 1;
				//strncpy(sval,params[0]->sval +start,slen); sval[slen]=0;
				return 1;
			}
			// strlen(str)
			else if(!strcmp(tname,"strlen"))
			{
				strcpy(vname,tname);
				vbtype='I';
				vtype='I';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				if((params[0]->vtype!='S')&&(params[0]->vtype!='V'))
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1;
				}
				ival=strlen(params[0]->sval);
				return 1;
			}
			// strstr(str,sub)
			else if(!strcmp(tname,"strstr"))
			{
				strcpy(vname,tname);
				vbtype='S';
				vtype='S';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,2)<0)
					return -1;
				if((params[0]->vtype!='S')&&(params[0]->vtype!='V'))
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1;
				}
				if((params[1]->vtype!='S')&&(params[1]->vtype!='V'))
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 2");
					return -1;
				}
				// Dependency check only--don't evaluate
				//const char *sptr=stristr(params[0]->sval,params[1]->sval);
				//if(sptr)
				//{
				//	int slen=(int)strlen(sptr);
				//	if(slen>=sizeof(sval))
				//		slen=sizeof(sval) -1;
				//	strncpy(sval,sptr,slen); sval[slen]=0;
				//}
				//else
					sval[0]=0;
				return 1;
			}
			// time(str)
			else if(!strcmp(tname,"time"))
			{				
				strcpy(vname,tname);
				vbtype='T';
				vtype='T';
				exprBeg=tbeg;
				exprEnd=eend;
				if(EvalDepParms(pfix,tptr +1,tend,1)<0)
					return -1;
				// time(0) returns current time GMT
				if((params[0]->vtype=='I')&&(params[0]->ival==0))
				{
				#ifdef WIN32
					SYSTEMTIME tsys;
					GetSystemTime(&tsys);
					ival=(tsys.wYear *10000000000) +(tsys.wMonth*100000000) +(tsys.wDay*1000000) +
						 (tsys.wHour *10000) +(tsys.wMinute*100) +(tsys.wSecond);
				#else
					ival=((LONGLONG)WSDate()*1000000) +WSTime();
				#endif
					fval=(double)ival;
					#ifdef WIN32
					sprintf(sval,"%I64d",ival);
					#else
					sprintf(sval,"%lld",ival);
					#endif
					return 1;
				}
				else if((params[0]->vtype!='S')&&(params[0]->vtype!='V'))
				{
					ParseError(-1,ebeg,eend,"String type expected for parameter 1");
					return -1; // yyyymmdd-HH:MM:SS only
				}
				// Dependency check only--don't evaluate
				//strncpy(sval,params[0]->sval,17); sval[17]=0;
				//if((sval[8]!='-')||(sval[11]!=':')||(sval[14]!=':'))
				//{
				//	ParseError(-1,ebeg,eend,"Time format error: expected yyyymmdd-HH:MM:SS");
				//	return -1; // yyyymmdd-HH:MM:SS only
				//}
				//int dd=myatoi(sval),hh=myatoi(sval +9),mm=myatoi(sval +12),ss=myatoi(sval +15);
				//ival=((LONGLONG)dd *1000000) +(hh*10000) +(mm*100) +ss;
				//fval=(double)ival;
				return 1;
			}
			// else add other functions here
			// Parenthesis for order of operations only
			else if(!*tname)
			{
				return EvalDeps(pfix,tptr +1,tend);
			}
			ParseError(-1,tptr,eend,"Unknown function %s",tname);
			return -1; // Unknown function
		}
		else if(*tptr==')')
		{
			ParseError(-1,ebeg,eend,"Unmatched ) parenthesis");
			return -1; // unmatched parenthesis
		}
		tptr++;
	}

	// Skip leading whitespace
	const char *lptr;
	for(lptr=ebeg;(lptr<eend)&&myisspace(*lptr);lptr++)
		;
	// Skip trailing whitespace
	const char *rptr;
	for(rptr=eend;(rptr>lptr)&&myisspace(rptr[-1]);rptr--)
		;
	// Is this a string literal?
	for(const char *tptr=lptr;tptr<rptr;)
	{
		if(*tptr=='\'')
		{
			const char *send=MatchSingleQuote(tptr,rptr,sval,sizeof(sval));
			if(!send)
				return -1; // unmatched quote
			vtype='S';
			exprBeg=tptr;
			exprEnd=send +1;
			// An exception for 'AppInstID*': treat like a function
			if(strrcmp(sval,"*"))
			{
				strcpy(vname,sval);
				vbtype=vtype='S';
			}
			return 1;
		}
		tptr++;
	}

	// Is this an integer?
	if(EvalIntegralExpr(lptr,rptr)>0)
		return 1;

	// Is this a decimal?
	if(EvalDecimalExpr(lptr,rptr)>0)
		return 1;

	// Intrinsic mnemonic
	// NULL
	if((rptr -lptr==4)&&(!strincmp(lptr,"NULL",4)))
	{
		vtype='S';
		exprBeg=ebeg;
		exprEnd=ebeg +4;
		memcpy(sval,"\0NULL\0",6);
		return 1;
	}
	// TRUE
	else if((rptr -lptr==4)&&(!strincmp(lptr,"TRUE",4)))
	{
		vtype='B';
		exprBeg=ebeg;
		exprEnd=ebeg +4;
		bval=true;
		return 1;
	}
	// FALSE
	else if((rptr -lptr==5)&&(!strincmp(lptr,"FALSE",5)))
	{
		vtype='B';
		exprBeg=ebeg;
		exprEnd=ebeg +5;
		bval=false;
		return 1;
	}
	// Variables must start with character
	else if(isalpha(*lptr))
	{
		vtype='V';
		exprBeg=lptr;
		exprEnd=rptr;
		int slen=(int)(rptr -lptr);
		strncpy(vname,lptr,slen); vname[slen]=0;
		vbtype='V';
		return 1;
	}

	_ASSERT(false);
	ParseError(-1,lptr,rptr,"Syntax error");
	return -1;
}
// Evaluates expression tree build by EvalDeps with latest variable values supplied by GetValue callback
int ExprTok::EvalExprTree(FIXINFO *pfix, void *hint, bool CaseSen)
{
	// Depth-first evaluation
	for(int i=0;i<nparams;i++)
		params[i]->EvalExprTree(pfix,hint,CaseSen);

	// Function or Variable
	if(vname[0])
	{
		vtype=vbtype;
		// Built-in functions
		if(!strcmp(vname,"tag"))
		{
			// Ignore FIX expressions when no FIX message passed in
			if(!pfix)
			{
				vtype='V';
				return 0;
			}
			if(params[0]->vtype=='I')
			{
				int tno=(int)params[0]->ival;
				strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
			}
			else if(params[0]->vtype=='S')
			{
				const char *tname=params[0]->sval;
				FIXTAG *ptag=pfix->GetTag(tname);
				if(ptag)
				{
					int vlen=ptag->vlen;
					if(vlen>sizeof(sval)-1)
						vlen=sizeof(sval)-1;
					strncpy(sval,ptag->val,vlen); sval[vlen]=0;
				}
			}
			else
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 1");
				return -1; // parameter must be integer
			}
			ExprTok ttok;
			ttok.pnotify=pnotify;
			// If the tag is missing or null, then no type
			if(!*sval)
				vtype=0;
			// If the tag's value an integer, then use that type
			//ival=pfix->TagInt(tno);
			else if(ttok.EvalIntegralExpr(sval,sval +strlen(sval))>0)
			{
				vtype='I';
				ival=ttok.ival;
				return 0;
			}
			// If the tag's value an decimal, then use that type
			//fval=pfix->TagFloat(tno);
			else if(ttok.EvalDecimalExpr(sval,sval +strlen(sval))>0)
			{
				vtype='F';
				fval=ttok.fval;
				return 0;
			}
			// Otherwise, assume it's a string
			else
				vtype='S';
			return 0;
		}
		else if(!strcmp(vname,"tagstr"))
		{
			if(!pfix)
			{
				vtype='V';
				return 0;
			}
			vtype='S';
			if(params[0]->vtype=='I')
			{
				int tno=(int)params[0]->ival;
				strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
			}
			else if(params[0]->vtype=='S')
			{
				const char *tname=params[0]->sval;
				FIXTAG *ptag=pfix->GetTag(tname);
				if(ptag)
				{
					int vlen=ptag->vlen;
					if(vlen>sizeof(sval)-1)
						vlen=sizeof(sval)-1;
					strncpy(sval,ptag->val,vlen); sval[vlen]=0;
				}
			}
			else
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 1");
				return -1; // parameter must be integer
			}
			return 0;
		}
		else if(!strcmp(vname,"tagint"))
		{
			if(!pfix)
			{
				vtype='V';
				return 0;
			}
			vtype='I';
			if(params[0]->vtype=='I')
			{
				int tno=(int)params[0]->ival;
				strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
			}
			else if(params[0]->vtype=='S')
			{
				const char *tname=params[0]->sval;
				FIXTAG *ptag=pfix->GetTag(tname);
				if(ptag)
				{
					int vlen=ptag->vlen;
					if(vlen>sizeof(sval)-1)
						vlen=sizeof(sval)-1;
					strncpy(sval,ptag->val,vlen); sval[vlen]=0;
				}
			}
			else
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 1");
				return -1; // parameter must be integer
			}
			ival=myatoi64(sval);
			return 1;
		}
		else if(!strcmp(vname,"tagdec"))
		{
			if(!pfix)
			{
				vtype='V';
				return 0;
			}
			vtype='F';
			if(params[0]->vtype=='I')
			{
				int tno=(int)params[0]->ival;
				strncpy(sval,pfix->TagStr(tno),(int)sizeof(sval)); sval[sizeof(sval)-1]=0;
			}
			else if(params[0]->vtype=='S')
			{
				const char *tname=params[0]->sval;
				FIXTAG *ptag=pfix->GetTag(tname);
				if(ptag)
				{
					int vlen=ptag->vlen;
					if(vlen>sizeof(sval)-1)
						vlen=sizeof(sval)-1;
					strncpy(sval,ptag->val,vlen); sval[vlen]=0;
				}
			}
			else
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 1");
				return -1; // parameter must be integer
			}
			fval=atof(sval);
			return 0;
		}
		else if(!strcmp(vname,"hastag"))
		{
			if(!pfix)
			{
				vtype='V';
				return 0;
			}
			vtype='B';
			if(params[0]->vtype=='I')
			{
				int tno=(int)params[0]->ival;
				bval=pfix->GetTag(tno)?true:false;
			}
			else if(params[0]->vtype=='S')
			{
				const char *tname=params[0]->sval;
				bval=pfix->GetTag(tname)?true:false;
			}
			else
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 1");
				return -1; // parameter must be integer
			}
			return 0;
		}
		else if(!strcmp(vname,"substr"))
		{
			vtype='S';
			if(params[0]->vtype!='S')
			{
				ParseError(-1,exprBeg,exprEnd,"String type expected for parameter 1");
				return -1;
			}
			else if(params[1]->vtype!='I')
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 2");
				return -1;
			}
			else if(params[2]->vtype!='I')
			{
				ParseError(-1,exprBeg,exprEnd,"Integer type expected for parameter 3");
				return -1;
			}
			sval[0]=0;
			int tlen=(int)strlen(params[0]->sval);
			int start=(int)params[1]->ival;				
			if(start>=tlen) // bad start, return empty
				return 0;
			int slen=(int)params[2]->ival;
			if(slen>tlen -start)
				slen=tlen -start;
			if(slen>=sizeof(sval))
				slen=sizeof(sval) -1;
			if(slen<1) // bad length, return empty
				return 0;
			strncpy(sval,params[0]->sval +start,slen); sval[slen]=0;
			return 0;
		}
		else if(!strcmp(vname,"strlen"))
		{
			vtype='I';
			if(params[0]->vtype!='S')
			{
				ParseError(-1,exprBeg,exprEnd,"String type expected for parameter 1");
				return -1;
			}
			ival=strlen(params[0]->sval);
			return 0;
		}
		else if(!strcmp(vname,"strstr"))
		{
			vtype='S';
			if(params[0]->vtype!='S')
			{
				ParseError(-1,exprBeg,exprEnd,"String type expected for parameter 1");
				return -1;
			}
			if(params[1]->vtype!='S')
			{
				ParseError(-1,exprBeg,exprEnd,"String type expected for parameter 2");
				return -1;
			}
			const char *sptr=stristr(params[0]->sval,params[1]->sval);
			if(sptr)
			{
				int slen=(int)strlen(sptr);
				if(slen>=sizeof(sval))
					slen=sizeof(sval) -1;
				strncpy(sval,sptr,slen); sval[slen]=0;
			}
			else
				sval[0]=0;
			return 0;
		}
		else if(!strcmp(vname,"time"))
		{
			vtype='T';
			// time(0) returns current time GMT
			if((params[0]->vtype=='I')&&(params[0]->ival==0))
			{
				#ifdef WIN32
				SYSTEMTIME tsys;
				GetSystemTime(&tsys);
				ival=(tsys.wYear *10000000000) +(tsys.wMonth*100000000) +(tsys.wDay*1000000) +
						(tsys.wHour *10000) +(tsys.wMinute*100) +(tsys.wSecond);
				fval=(double)ival;
				sprintf(sval,"%I64d",ival);
				#else
				//TODO: Verify
				ival=((LONGLONG)WSDate()*1000000) +WSTime();
				#endif
				return 1;
			}
			else if(params[0]->vtype!='S')
			{
				ParseError(-1,exprBeg,exprEnd,"String type expected for parameter 1");
				return -1; // yyyymmdd-HH:MM:SS only
			}
			strncpy(sval,params[0]->sval,17); sval[17]=0;
			if((sval[8]!='-')||(sval[11]!=':')||(sval[14]!=':'))
			{
				ParseError(-1,exprBeg,exprEnd,"Time format error: expected yyyymmdd-HH:MM:SS");
				return -1; // yyyymmdd-HH:MM:SS only
			}
			int dd=myatoi(sval),hh=myatoi(sval +9),mm=myatoi(sval +12),ss=myatoi(sval +15);
			ival=((LONGLONG)dd *1000000) +(hh*10000) +(mm*100) +ss;
			fval=(double)ival;
			return 0;
		}
		// TODO: Can we support app-specific functions?
		// Set variable
		else if(strrcmp(vname,"*"))
		{
			// Should we call GetValue to get set type?
			vtype='S';
			return 0;
		}
		// Ignore variable conditions when filtering FIX
		else if(pfix)
		{
			vtype='V';
			return 0;
		}
		// Variable
		else
		{
			ExprTok vtok=*this;
			if(!pnotify->GetValue(vname,&vtok,hint))
				return -1;
			char ctype=PromoteType(this,&vtok,true);
			if(!ctype)
			{
				ParseError(-1,exprBeg,exprEnd,"No promotion supported to assign type %c to type %c",vtok.vtype,this->vtype);
				return -1;
			}
			switch(ctype)
			{
			case 'B': vtype='B'; bval=vtok.bval; break;
			case 'I': vtype='I'; ival=vtok.ival; break;
			case 'F': vtype='F'; fval=vtok.fval; break;
			case 'S': vtype='S'; memcpy(sval,vtok.sval,sizeof(sval)); break;
			case 'T': vtype='T'; ival=vtok.ival; break;
			case 'V': break;
			};
			// Prevent double-delete on copied pointers
			memset(&vtok,0,sizeof(ExprTok));
		}
		return 0;
	}
	// Constant
	else if(!oval[0])
		return 0;

	// Conjunction operators
	if((!strcmp(oval,"AND"))||(!strcmp(oval,"&&")))
	{
		bval=true;
		for(int i=0;i<nparams;i++)
		{
			bool pval=true;
			switch(params[i]->vtype)
			{
			case 'B': pval=params[i]->bval; break;
			case 'I': pval=params[i]->ival?true:false; break;
			case 'F': pval=params[i]->fval==0.0?false:true; break;
			case 'S': pval=params[i]->sval[0]?true:false; break;
			case 'T': pval=params[i]->ival?true:false; break;
			case 'V': pval=true; break;
			default: return -1;
			};
			if(!pval)
			{
				bval=false; // stop on first false expression
				break;
			}
		}
		return 0;
	}
	else if((!strcmp(oval,"OR"))||(!strcmp(oval,"||")))
	{
		bval=false;
		for(int i=0;i<nparams;i++)
		{
			bool pval=false;
			switch(params[i]->vtype)
			{
			case 'B': pval=params[i]->bval; break;
			case 'I': pval=params[i]->ival?true:false; break;
			case 'F': pval=params[i]->fval==0.0?false:true; break;
			case 'S': pval=params[i]->sval[0]?true:false; break;
			case 'T': pval=params[i]->ival?true:false; break;
			case 'V': pval=false; break;
			default: return -1;
			};
			if(pval)
			{
				bval=true; // stop on first true expression
				break;
			}
		}
		return 0;
	}
	// Comparison operators
	else if(!strcmp(oval,"=="))
	{
		bval=false;
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=(params[0]->bval==params[1]->bval); break;
		case 'I': bval=(params[0]->ival==params[1]->ival); break;
		case 'F': bval=(params[0]->fval==params[1]->fval); break;
		case 'S': 
		{
			// Set variable
			if(strrcmp(params[1]->sval,"*"))
				bval=pnotify->IsInSet(params[1]->sval,params[0]->sval,hint);
			else
				bval=(CaseSen ? strcmp(params[0]->sval,params[1]->sval) : _stricmp(params[0]->sval,params[1]->sval))?false:true; 
			break;
		}
		case 'T': bval=(params[0]->ival==params[1]->ival); break;
		case 'V': bval=true; break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,"!="))
	{
		bval=true;
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=(params[0]->bval!=params[1]->bval); break;
		case 'I': bval=(params[0]->ival!=params[1]->ival); break;
		case 'F': bval=(params[0]->fval!=params[1]->fval); break;
		case 'S': bval=(CaseSen ? strcmp(params[0]->sval,params[1]->sval) : _stricmp(params[0]->sval,params[1]->sval))?true:false; break;
		case 'T': bval=(params[0]->ival!=params[1]->ival); break;
		case 'V': bval=true; break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,">="))
	{
		bval=false;
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=(params[0]->bval>=params[1]->bval); break;
		case 'I': bval=(params[0]->ival>=params[1]->ival); break;
		case 'F': bval=(params[0]->fval>=params[1]->fval); break;
		case 'S': bval=(CaseSen ? strcmp(params[0]->sval,params[1]->sval) : _stricmp(params[0]->sval,params[1]->sval))>=0?true:false; break;
		case 'T': bval=(params[0]->ival>=params[1]->ival); break;
		case 'V': bval=true; break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,"<="))
	{
		bval=false;
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=(params[0]->bval<=params[1]->bval); break;
		case 'I': bval=(params[0]->ival<=params[1]->ival); break;
		case 'F': bval=(params[0]->fval<=params[1]->fval); break;
		case 'S': bval=(CaseSen ? strcmp(params[0]->sval,params[1]->sval) : _stricmp(params[0]->sval,params[1]->sval))<=0?true:false; break;
		case 'T': bval=(params[0]->ival<=params[1]->ival); break;
		case 'V': bval=true; break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,">"))
	{
		bval=false;
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=(params[0]->bval>params[1]->bval); break;
		case 'I': bval=(params[0]->ival>params[1]->ival); break;
		case 'F': bval=(params[0]->fval>params[1]->fval); break;
		case 'S': bval=(CaseSen ? strcmp(params[0]->sval,params[1]->sval) : _stricmp(params[0]->sval,params[1]->sval))>0?true:false; break;
		case 'T': bval=(params[0]->ival>params[1]->ival); break;
		case 'V': bval=true; break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,"<"))
	{
		bval=false;
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=(params[0]->bval<params[1]->bval); break;
		case 'I': bval=(params[0]->ival<params[1]->ival); break;
		case 'F': bval=(params[0]->fval<params[1]->fval); break;
		case 'S': bval=(CaseSen ? strcmp(params[0]->sval,params[1]->sval) : _stricmp(params[0]->sval,params[1]->sval))<0?true:false; break;
		case 'T': bval=(params[0]->ival<params[1]->ival); break;
		case 'V': bval=true; break;
		default: return -1;
		};
		return 0;
	}
	// Arithmetic operators
	else if(!strcmp(oval,"+"))
	{
		if(nparams!=2)
			return -1;
		// Compute t2=t1 +diff(sec)
		if((params[0]->vtype=='T')&&(params[1]->vtype=='I'))
		{
			vtype='T';
			ival=0; fval=0; sval[0]=0;
			DWORD dd=(DWORD)(params[0]->ival/1000000);
			DWORD tt=(DWORD)(params[0]->ival%1000000);
			// Convert to time_t from components
			struct tm gtm;
			memset(&gtm,0,sizeof(struct tm));
			gtm.tm_year=(dd/10000) -1900;
			gtm.tm_mon=(dd%10000)/100 -1;
			gtm.tm_mday=(dd%100);
			gtm.tm_hour=tt/10000;
			gtm.tm_min=(tt%10000)/100;
			gtm.tm_sec=(tt%100);
			gtm.tm_isdst=-1;
			time_t gts=mktime(&gtm);
			if(!gts)
				return -1; // failed time conversion
			gts+=(int)params[1]->ival;
			// Convert back to components
			struct tm *ntm=localtime(&gts);
			if(!ntm)
				return -1; // failed new time conversion
			gtm=*ntm;
			dd=(gtm.tm_year +1900)*10000 +(gtm.tm_mon +1)*100 +(gtm.tm_mday);
			tt=(gtm.tm_hour *10000) +(gtm.tm_min *100) +(gtm.tm_sec);
			ival=((LONGLONG)dd)*1000000 +tt;
			return 0;
		}
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'I': ival=params[0]->ival +params[1]->ival; break;
		case 'F': fval=params[0]->fval +params[1]->fval; break;
		case 'S':
		{
			int slen1=(int)strlen(params[0]->sval);
			int slen2=(int)strlen(params[1]->sval);
			int tlen=slen1 +slen2;
			if(tlen>=sizeof(sval))
				tlen=sizeof(sval) -1;
			strncpy(sval,params[0]->sval,tlen);
			tlen-=slen1;
			if(tlen>0)
			{
				strncpy(sval +slen1,params[1]->sval,tlen);
				sval[slen1 +tlen]=0;
			}
			break;
		}
		case 'T': ival=params[0]->ival; break;
		case 'V': break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,"-"))
	{
		if(nparams!=2)
			return -1;
		// Compute diff(sec)=t1 -t2
		if((params[0]->vtype=='T')&&(params[1]->vtype=='T'))
		{
			vtype='I';
			ival=0; fval=0;
			DWORD dd=(DWORD)(params[0]->ival/1000000);
			DWORD tt=(DWORD)(params[0]->ival%1000000);
			// Convert to time_t from components
			struct tm gtm1;
			memset(&gtm1,0,sizeof(struct tm));
			gtm1.tm_year=(dd/10000) -1900;
			gtm1.tm_mon=(dd%10000)/100 -1;
			gtm1.tm_mday=(dd%100);
			gtm1.tm_hour=tt/10000;
			gtm1.tm_min=(tt%10000)/100;
			gtm1.tm_sec=(tt%100);
			gtm1.tm_isdst=-1;
			time_t gts1=mktime(&gtm1);
			if(!gts1)
				return -1; // failed time conversion

			dd=(DWORD)(params[1]->ival/1000000);
			tt=(DWORD)(params[1]->ival%1000000);
			// Convert to time_t from components
			struct tm gtm2;
			memset(&gtm2,0,sizeof(struct tm));
			gtm2.tm_year=(dd/10000) -1900;
			gtm2.tm_mon=(dd%10000)/100 -1;
			gtm2.tm_mday=(dd%100);
			gtm2.tm_hour=tt/10000;
			gtm2.tm_min=(tt%10000)/100;
			gtm2.tm_sec=(tt%100);
			gtm2.tm_isdst=-1;
			time_t gts2=mktime(&gtm2);
			if(!gts2)
				return -1; // failed time conversion
			ival=gts1 -gts2;
			return 0;
		}
		// Compute t2=t1 -diff(sec)
		else if((params[0]->vtype=='T')&&(params[1]->vtype=='I'))
		{
			vtype='T';
			ival=0; fval=0; sval[0]=0;
			DWORD dd=(DWORD)(params[0]->ival/1000000);
			DWORD tt=(DWORD)(params[0]->ival%1000000);
			// Convert to time_t from components
			struct tm gtm;
			memset(&gtm,0,sizeof(struct tm));
			gtm.tm_year=(dd/10000) -1900;
			gtm.tm_mon=(dd%10000)/100 -1;
			gtm.tm_mday=(dd%100);
			gtm.tm_hour=tt/10000;
			gtm.tm_min=(tt%10000)/100;
			gtm.tm_sec=(tt%100);
			gtm.tm_isdst=-1;
			time_t gts=mktime(&gtm);
			if(!gts)
				return -1; // failed time conversion
			gts-=(int)params[1]->ival;
			// Convert back to components
			struct tm *ntm=localtime(&gts);
			if(!ntm)
				return -1; // failed new time conversion
			gtm=*ntm;
			dd=(gtm.tm_year +1900)*10000 +(gtm.tm_mon +1)*100 +(gtm.tm_mday);
			tt=(gtm.tm_hour *10000) +(gtm.tm_min *100) +(gtm.tm_sec);
			ival=((LONGLONG)dd)*1000000 +tt;
			return 0;
		}
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'I': ival=params[0]->ival -params[1]->ival; break;
		case 'F': fval=params[0]->fval -params[1]->fval; break;
		case 'T': ival=params[0]->ival; break;
		case 'V': break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,"*"))
	{
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'I': ival=params[0]->ival *params[1]->ival; break;
		case 'F': fval=params[0]->fval *params[1]->fval; break;
		case 'V': break;
		default: return -1;
		};
		return 0;
	}
	else if(!strcmp(oval,"/"))
	{
		if(nparams!=2)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'I': ival=params[0]->ival /params[1]->ival; break;
		case 'F': fval=params[0]->fval /params[1]->fval; break;
		case 'V': break;
		default: return -1;
		};
		return 0;
	}
	// Unary arithmetic operators
	else if((!strcmp(oval,"NOT"))||(!strcmp(oval,"!")))
	{
		if(nparams!=1)
			return -1;
		char ctype=PromoteType(params[0],params[1]);
		if(!ctype)
		{
			if(!params[1])
				ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to NULL",params[0]->vtype);
			else
				ParseError(-1,exprBeg,exprEnd,"No promotion supported to compare type %c to type %c",params[0]->vtype,params[1]->vtype);
			return -1;
		}
		switch(ctype)
		{
		case 'B': bval=!params[0]->bval; break;
		case 'I': bval=(params[0]->ival?false:true); break;
		case 'V': break;
		default: return -1;
		};
		return 0;
	}
	_ASSERT(false);
	return -1;
}
