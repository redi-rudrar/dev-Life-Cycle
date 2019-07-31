// stdafx.cpp : source file that includes just the standard includes
// ViewServerProto.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifndef VS2010
int __cdecl sprintf_s(char *dst, size_t, const char *fmt, ...)
{
	va_list alist;
	va_start(alist,fmt);
	int rc=vsprintf(dst,fmt,alist);
	va_end(alist);
	return rc;
}
int __cdecl vsprintf_s(char *dst, size_t, const char *fmt, va_list alist)
{
	return vsprintf(dst,fmt,alist);
}
char *strcpy_s(char *dst, size_t, const char *src)
{
	return strcpy(dst,src);
}
char *strncpy_s(char *dst, size_t, const char *src, size_t len)
{
	return strncpy(dst,src,len);
}
#endif
