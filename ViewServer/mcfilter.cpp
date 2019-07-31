
#include "stdafx.h"
#include "mcfilter.h"

int MCFilter::SetExchRegFilter(const char *fspec)
{
	char fcpy[1024]={0};
	strncpy(fcpy,fspec,1023); fcpy[1023]=0;
	DisableAll();
	bool disableAssets=true;
	for(char *tok=strtok(fcpy,",\r\n");tok;tok=strtok(0,",\r\n"))
	{
		//if(*tok=='?')
		//	curport->TryUnknown=true;
		//else
		if((!strcmp(tok,"ALL"))||(!strcmp(tok,"0")))
		{
			EnableAll();
		}
		else if(!strcmp(tok,"-ALL"))
		{
			DisableAll();
			DisableAllAssets();
		}
		else if((!strcmp(tok,"EQ"))||(!strcmp(tok,"-EQ")))
		{
			if(disableAssets)
			{
				DisableAllAssets(); disableAssets=false;
			}
			bool deny=(*tok=='-')?true:false;
			if(deny)
			{
				DisableAsset(IQAST_EQUITY);
				DisableAsset(IQAST_EQINDEX);
			}
			else
			{
				EnableAsset(IQAST_EQUITY);
				EnableAsset(IQAST_EQINDEX);
			}
		#ifdef NASLV2_EXCH8
			static char eqexchlist[]={8,9,10,11,13,14,15,17,18,27,0};
		#else
			static char eqexchlist[]={9,10,11,13,14,15,17,18,27,0};
		#endif
			IQMarketCenter mc=IQMC_UNKNOWN;
			for(int i=0;eqexchlist[i];i++)
			{
				int Exch=eqexchlist[i];
				IQCspApi::MarketCode(Exch,0,mc);
				if(mc!=IQMC_UNKNOWN)
				{
					if(deny)
						Disable(mc);
					else
						Enable(mc);
				}
				for(int reg=0;reg<26;reg++)
				{
					if((Exch==10)&&(('a'+reg=='m')||('a'+reg=='q'))) // Not 10mq
						continue;
					if((Exch==27)&&('a'+reg=='b')) // Not 27b
						continue;
					IQCspApi::MarketCode(Exch,'a' +reg,mc);
					if(mc!=IQMC_UNKNOWN)
					{
						if(deny)
							Disable(mc);
						else
							Enable(mc);
					}
				}
			}
		}
		else if((!strcmp(tok,"OPT"))||(!strcmp(tok,"-OPT")))
		{
			if(disableAssets)
			{
				DisableAllAssets(); disableAssets=false;
			}
			bool deny=(*tok=='-')?true:false;
			if(deny)
				DisableAsset(IQAST_EQOPTION);
			else
				EnableAsset(IQAST_EQOPTION);
			IQMarketCenter mc=IQMC_UNKNOWN;
			IQCspApi::MarketCode(12,0,mc);
			if(mc!=IQMC_UNKNOWN)
			{
				if(deny)
					Disable(mc);
				else
					Enable(mc);
			}
			for(int reg=0;reg<26;reg++)
			{
				IQCspApi::MarketCode(12,'a' +reg,mc);
				if(mc!=IQMC_UNKNOWN)
				{
					if(deny)
						Disable(mc);
					else
						Enable(mc);
				}
			}
		}
		else if((!strcmp(tok,"FUT"))||(!strcmp(tok,"-FUT")))
		{
			if(disableAssets)
			{
				DisableAllAssets(); disableAssets=false;
			}
			bool deny=(*tok=='-')?true:false;
			if(deny)
				DisableAsset(IQAST_FUTURE);
			else
				EnableAsset(IQAST_FUTURE);
			static char eqexchlist[]={16,19,28,0};
			IQMarketCenter mc=IQMC_UNKNOWN;
			for(int i=0;eqexchlist[i];i++)
			{
				int Exch=eqexchlist[i];
				IQCspApi::MarketCode(Exch,0,mc);
				if(mc!=IQMC_UNKNOWN)
				{
					if(deny)
						Disable(mc);
					else
						Enable(mc);
				}
				for(int reg=0;reg<26;reg++)
				{
					IQCspApi::MarketCode(Exch,'a' +reg,mc);
					if(mc!=IQMC_UNKNOWN)
					{
						if(deny)
							Disable(mc);
						else
							Enable(mc);
					}
				}
			}
			IQCspApi::MarketCode(10,'m',mc);
			if(deny)
				Disable(mc);
			else
				Enable(mc);
			IQCspApi::MarketCode(10,'q',mc);
			if(deny)
				Disable(mc);
			else
				Enable(mc);
			IQCspApi::MarketCode(27,'b',mc);
			if(deny)
				Disable(mc);
			else
				Enable(mc);
		}
		// Matches IQMC_UNKNOWN
		else if(!strcmp(tok,"?"))
		{
			matchUnknown=true;
		}
		else
		{
			// Exchange[reg]
			if((myisdigit(tok[0]))||((tok[0]=='-')&&(myisdigit(tok[1]))))
			{
				bool deny=false;
				int Exch=myatol(tok);
				if(Exch<0)
				{
					deny=true;
					Exch=-Exch;
				}
				char Region=0;
				const char *tptr;
				for(tptr=tok;(*tptr)&&((myisdigit(*tptr))||(*tptr=='-'));tptr++)
					;
				IQMarketCenter mc=IQMC_UNKNOWN;
				// Set all regions
				if((!*tptr)||(*tptr=='*'))
				{
					IQCspApi::MarketCode(Exch,0,mc);
					Enable(mc);
					for(int reg=0;reg<26;reg++)
					{
						IQCspApi::MarketCode(Exch,'a' +reg,mc);
						if(mc!=IQMC_UNKNOWN)
						{
							if(deny)
								Disable(mc);
							else
								Enable(mc);
						}
					}
				}
				// Set one or more specific regions
				else
				{
					for(;*tptr;tptr++)
					{
						if(*tptr=='_')
						{
							IQCspApi::MarketCode(Exch,0,mc);
							if(mc!=IQMC_UNKNOWN)
							{
								if(deny)
									Disable(mc);
								else
									Enable(mc);
							}
						}
						else
						{
							Region=*tptr;
							IQCspApi::MarketCode(Exch,Region,mc);
							if(mc!=IQMC_UNKNOWN)
							{
								if(deny)
									Disable(mc);
								else
									Enable(mc);
							}
						}
					}
				}
			}
			// Letter filter
			else
			{
				DisableAllLtrs();
				for(char *lptr=tok;*lptr;)
				{
					if(lptr[1]=='-')
					{
						for(char ltr=mytoupper(lptr[0]);ltr<=mytoupper(lptr[2]);ltr++)
							EnableLtr(ltr);
						lptr+=3;
					}
					else
					{
						EnableLtr(*lptr); lptr++;
					}
				}
			}
		}
	}
	return 0;
}
