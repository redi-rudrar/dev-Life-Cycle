// These defines are used for performance enhancement over Windows DLL implementation of string functions
#ifndef _MYRT_H
#define _MYRT_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef USE_MYRT
#define myislower(c) (int)(((c>='a')&&(c<='z'))?2:0)
#define myisupper(c) (int)(((c>='A')&&(c<='Z'))?1:0)
#define mytolower(c) (int)((c>='A')&&(c<='Z')?(c-'A'+'a'):(c))
#define mytoupper(c) (int)((c>='a')&&(c<='z')?(c-'a'+'A'):(c))
#define myatoi (int)myatol
extern long myatol(const char *nptr);
extern __int64 myatoi64(const char *nptr);
#define myisdigit(c) (int)(((c>='0')&&(c<='9'))?4:0)
#define myisspace isspace
#define myisalpha isalpha

#else//!USE_MYRT
// MSVCRT version is much slower due to TLS!
#define myislower islower
#define myisupper isupper
#define mytolower tolower
#define mytoupper toupper
#define myatoi atoi
#define myatol atol
#define myatoi64 _atoi64
#define myisdigit isdigit
#define myisspace isspace
#define myisalpha isalpha

#endif//!USE_MYRT

#ifdef __cplusplus
};//extern "C"
#endif

#endif//_MYRT_H
