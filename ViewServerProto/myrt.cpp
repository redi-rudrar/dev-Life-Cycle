
#include "stdafx.h"
#include "myrt.h"

#ifdef USE_MYRT

#define mychartodigit(c) (((c>='0')&&(c<='9')?(c-'0'):-1))

extern "C"
{

long myatol(
        const char *nptr
        )
{
        int c;              /* current char */
        long total;      /* current total */
        int sign;           /* if '-', then negative, otherwise positive */
        while ( myisspace((int)(unsigned char)*nptr) )
            ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;           /* save sign indication */
        if (c == '-' || c == '+')
            c = (int)(unsigned char)*nptr++;    /* skip sign */

        total = 0;

        while ( (c = mychartodigit(c)) != -1 ) {
            total = 10 * total + c;     /* accumulate digit */
            c = (unsigned char)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}

__int64 myatoi64(
        const char *nptr
        )
{
        int c;              /* current char */
        __int64 total;      /* current total */
        int sign;           /* if '-', then negative, otherwise positive */
        while ( myisspace((int)(unsigned char)*nptr) )
            ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;           /* save sign indication */
        if (c == '-' || c == '+')
            c = (int)(unsigned char)*nptr++;    /* skip sign */

        total = 0;

        while ( (c = mychartodigit(c)) != -1 ) {
            total = 10 * total + c;     /* accumulate digit */
            c = (unsigned char)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}

};//extern "C"

#endif//USE_MYRT
