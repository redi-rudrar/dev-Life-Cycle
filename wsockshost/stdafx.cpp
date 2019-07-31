// stdafx.cpp : source file that includes just the standard includes
// wsockshost.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef WIN32
#ifndef VS2010
int __cdecl sprintf_s(char dst[], const char *fmt, ...)
{
	va_list alist;
	va_start(alist,fmt);
	int rc=vsprintf(dst,fmt,alist);
	va_end(alist);
	return rc;
}
char *strcpy_s(char dst[], const char *src)
{
	return strcpy(dst,src);
}
#endif
#endif
