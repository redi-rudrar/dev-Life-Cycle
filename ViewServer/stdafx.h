// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef VS2010
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
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
#include <windows.h>
#include <crtdbg.h>
#define USE_MYRT
#include "myrt.h"

#ifndef VS2010
extern int __cdecl sprintf_s(char *, size_t, const char *, ...);
extern int __cdecl vsprintf_s(char *, size_t, const char *, va_list);
extern char *strcpy_s(char *, size_t, const char *);
extern char *strncpy_s(char *, size_t, const char *, size_t);
#endif
#if defined(WIN32)||defined(_CONSOLE)
#define fseeko fseek
#define ftello ftell
#endif
#define GetFullPathNameEx GetFullPathName

#include "vsdefs.h"
#include "ViewServer.h"
