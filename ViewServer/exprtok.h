// Lexicographical grammar expression parser on FIX messages
#ifndef _EXPRTOK_H
#define _EXPRTOK_H

#include "bfix.h"

class ExprTokNotify
{
public:
	virtual void ExprTokError(int rc, const char *emsg)=0;
	virtual bool GetValue(const char *var, class ExprTok *vtok, void *hint){return true;}
	virtual bool IsInSet(const char *set, const char *val, void *hint){return false;}
};

// Supports 'select <taglist> from <table> where <compound_tag_expression>'
class ExprTok
{
public:
	ExprTok()
		:exprBeg(0)
		,exprEnd(0)
		,vtype(0)
		,ival(0)
		,fval(0)
		,bval(false)
		,nparams(0)
		,pnotify(0)
		,vbtype(0)
	{
		sval[0]=0;
		oval[0]=0;
		memset(params,0,8*sizeof(ExprTok*));
		vname[0]=0;
	}
	~ExprTok()
	{
		Clean(this);
	}
	inline void Clean(){ return Clean(this); }
	static void Clean(ExprTok *ptok)
	{
		for(int i=0;i<ptok->nparams;i++)
		{
			if(ptok->params[i])
			{
				delete ptok->params[i]; ptok->params[i]=0;
			}
		}
	}
	int EvalExpr(const char *ebeg, const char *eend, FIXINFO *pfix);
	int EvalDeps(FIXINFO *pfix, const char *ebeg, const char *eend);
	int EvalExprTree(FIXINFO *pfix, void *hint, bool CaseSen=false);  //DT9491

	const char *exprBeg;	// Beginning of expression (inclusive)
	const char *exprEnd;	// End of expression (exclusive)
	char vtype;				// 0-unknown, I-integer, F-Decimal, S-string, B-bool, T-time
	LONGLONG ival;			// Integer value
	double fval;			// Decimal value
	bool bval;				// Boolean value
	char sval[1024];		// String value
	char oval[8];			// Operator
	int nparams;			// Number of parameters
	ExprTok *params[8];		// Function parameters
	ExprTokNotify *pnotify; // Notifcation interface
	char vname[256];		// Variable name
	char vbtype;			// Bound variable type

protected:
	int EvalDecimalExpr(const char *ebeg, const char *eend);
	int EvalIntegralExpr(const char *ebeg, const char *eend);
	const char *MatchParen(const char *ebeg, const char *eend);
	const char *MatchSingleQuote(const char *ebeg, const char *eend, char *str, int slen);
	static char PromoteType(ExprTok *t1, ExprTok *t2, bool forAssign=false);
	int EvalExprParms(const char *ebeg, const char *eend, int eparms, FIXINFO *pfix);
	void ParseError(int rc, const char *ebeg, const char *eend, const char *fmt, ...);
	int EvalDepParms(FIXINFO *pfix, const char *ebeg, const char *eend, int eparms);
};

#endif//_EXPRTOK_H
