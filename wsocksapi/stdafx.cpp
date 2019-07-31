// stdafx.cpp : source file that includes just the standard includes
// iqclientapi.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#ifdef WIN32
BOOL DeleteMutex(HANDLE hMutex)
{
	return CloseHandle(hMutex);
}

#ifndef VS2010
int __cdecl sprintf_s(char *dst, size_t, const char *fmt, ...)
{
	va_list alist;
	va_start(alist,fmt);
	int rc=vsprintf(dst,fmt,alist);
	va_end(alist);
	return rc;
}
int __cdecl vsnprintf_s(char *dst, size_t, int, const char *fmt, va_list alist)
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

#else//!WIN32
#include <time.h>

DWORD GetTickCount()
{
	timespec tspec;
	clock_gettime(CLOCK_REALTIME,&tspec);
	return tspec.tv_nsec *1000;
}

__int64 _atoi64(const char *str)
{
	return atol(str);
}

long WSDate()
{
	time_t tnow=time(0);
	tm *ltm=localtime(&tnow);
	long wsdt=(ltm->tm_year*10000)+(ltm->tm_mon*100)+(ltm->tm_mday);
	return wsdt;
}

long WSTime()
{
	time_t tnow=time(0);
	tm *ltm=localtime(&tnow);
	long wstm=(ltm->tm_hour*10000)+(ltm->tm_min*100)+(ltm->tm_sec);
	return wstm;
}

int _kbhit()   
{   
    struct timeval tv;   
    fd_set fds;   
    tv.tv_sec = 0;   
    tv.tv_usec = 0;   
    FD_ZERO(&fds);   
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0   
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);   
    return FD_ISSET(STDIN_FILENO, &fds);   
}

int WSAGetLastError()
{
	return errno;
}

int GetLastError()
{
        return errno;
}

int PathFileExists(const char *fpath)
{
	struct stat sbuf;
	if(!stat(fpath,&sbuf))
		return 1;
	return 0;
}

char *strlwr(char *str)
{
	static int cdiff='A' -'a';
	for(char *sptr=str;*sptr;sptr++)
	{
		if((*sptr>='A')&&(*sptr<='Z'))
			*sptr-=cdiff;	
	}
	return str;
}

char *strupr(char *str)
{
        static int cdiff='a' -'A';
        for(char *sptr=str;*sptr;sptr++)
        {
                if((*sptr>='a')&&(*sptr<='z'))
                        *sptr+=cdiff;
        }
        return str;
}

void Assert(const char *fpath, int line)
{
	printf("ASSERTION failed at %s,%d\n",fpath,line);
}

#endif
