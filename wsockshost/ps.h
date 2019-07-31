
#ifndef _PS_H
#define _PS_H

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct FindProcData
{
    int pid;
    char path[MAX_PATH];
    time_t last;
    char domain[256];
    char account[256];
};

extern int ps(FindProcData *pdata, int maxprocs);

#ifdef __cplusplus
};//extern "C"
#endif

#endif//_PS_H
