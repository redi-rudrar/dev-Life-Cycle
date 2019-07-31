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

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <crtdbg.h>

#include <string>
#include <list>
#include <map>
#include <set>
using namespace std;
#include "hash_map"
using namespace __gnu_cxx;
#define USE_MYRT
#include "myrt.h"

#else//!WIN32
#include <string>
#include <list>
#include <map>
#include <set>
using namespace std;
#define hash_map map
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef HANDLE
#define HANDLE unsigned long
#endif

#ifndef __int64
#define __int64 long long 
#endif

#ifndef _stdcall
#define _stdcall 
#endif

#ifndef _ASSERT
#define _ASSERT(x)
#endif

#ifndef MAKELONG
#define MAKELONG(l,h) ((h)<<16 +(l))
#endif

#ifndef MAKEWORD
#define MAKEWORD(l,h) ((h)<<8 +(l))
#endif

#ifndef HIWORD
#define HIWORD(d) ((d&0xFFFF0000)>>16)
#endif
#ifndef LOWORD
#define LOWORD(d) (d&0xFFFF)
#endif

#ifndef LPVOID
typedef void *LPVOID;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef UCHAR
typedef unsigned char UCHAR;
#endif
#ifndef uchar
typedef unsigned char uchar;
#endif

#ifndef HWND
#define HWND void *
#endif

#ifndef SIZE_T
#define SIZE_T size_t
#endif

#ifndef SOCKET
typedef int SOCKET;
#endif

#ifndef SOCKADDR_IN
typedef sockaddr_in SOCKADDR_IN;
#endif

#ifndef SOCKADDR
typedef sockaddr SOCKADDR;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

extern DWORD GetTickCount();
extern __int64 _atoi64(const char *str);
extern long WSDate();
extern long WSTime();
extern int _kbhit();

#define stricmp strcasecmp
#define closesocket close
#define SleepEx(d,b) sleep(d/1000)
#define _getch getchar

#pragma pack(push,8)

#endif//!_WIN32


#define WS_MAX_BLOCK_SIZE 128
#define WSAPILINK extern

#define WSTRING_STRTOKE
#define FEEDTASK_STRINGKEY
#define FEEDTASK_MULTITASK
#define WS_WRITEBUFF_FULL

#endif//_STDAFX_H
