
#ifndef _WSTRING_H
#define _WSTRING_H

#ifdef WIN32
#include <windows.h>
#endif
#include <string.h>

#ifdef MAKE_DLL
	#ifdef MAKE_WSOCKSAPI
	#define WSEXPORT __declspec(dllexport)
	#else
	#define WSEXPORT __declspec(dllimport)
	#endif
#else
	#ifdef MAKE_WSOCKSAPI
	#define WSEXPORT
	#else
	#define WSEXPORT extern
	#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WIN32
WSEXPORT const char *w2str(
    const WCHAR *wstr);

WSEXPORT const WCHAR *str2w(
    const char *str);
#endif

WSEXPORT const char *strechr(
    const char *buf, 
    char ch, 
    const char *bend);

WSEXPORT const char *strendl(
    const char *buf, 
    const char *bend);

WSEXPORT const char *strendln(
    const char *buf, 
    const char *bend);

WSEXPORT const char *strestr(
    const char *src, 
    const char *sub, 
    const char *bend);

WSEXPORT const char *stristr(
    const char *src, 
    const char *sub);

WSEXPORT int strrcmp(
    const char *src, 
    const char *sub);

WSEXPORT int strincmp(
	const char *s1, 
	const char *s2, 
	int cnt);

#ifdef WSTRING_STRTOKE
WSEXPORT char *strtoke(
	char *str, 
	const char *toks);
#endif

#ifdef __cplusplus
}
#endif

#endif//_WSTRING_H

