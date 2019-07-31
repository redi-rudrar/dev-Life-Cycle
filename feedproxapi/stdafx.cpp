// stdafx.cpp : source file that includes just the standard includes
// iqclientapi.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#ifndef WIN32
#include <time.h>

DWORD GetTickCount()
{
	timespec tspec;
	clock_gettime(CLOCK_REALTIME,&tspec);
	DWORD ms=tspec.tv_sec*1000 +tspec.tv_nsec/1000000;
	return ms;
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
#endif//!WIN32
