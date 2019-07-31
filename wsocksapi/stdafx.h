// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#ifndef _STDAFX_H
#define _STDAFX_H

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef VS2010
#define _CRT_SECURE_NO_WARNINGS
#else
// The following needed for VS2003, but not VS2010
// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400	// Change this to the appropriate value to target Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <crtdbg.h>
#include <time.h>

#include <string>
#include <list>
#include <map>
#include <set>
using namespace std;
#include "hash_map"
using namespace __gnu_cxx;
#define TLSDEF __declspec(thread)
#define USE_MYRT
#define INVALID_FILE_VALUE INVALID_HANDLE_VALUE

#ifndef VS2010
extern int __cdecl sprintf_s(char *, size_t, const char *, ...);
#define _TRUNCATE 0
extern int __cdecl vsnprintf_s(char *, size_t, int, const char *, va_list);
extern char *strcpy_s(char *, size_t, const char *);
extern char *strncpy_s(char *, size_t, const char *, size_t);
#endif

#define WSTRING_STRTOKE
#define WS_WRITEBUFF_FULL

#endif//_STDAFX_H

