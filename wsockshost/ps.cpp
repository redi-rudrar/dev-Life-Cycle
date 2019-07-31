
#include "stdafx.h"
#if defined(WIN32)||defined(_CONSOLE)
#include <windows.h>
#include "psapi.h"
#include <tlhelp32.h>
#include "ps.h"

static int GetProcUser(HANDLE hProcess, char *domain, DWORD dlen, char *account, DWORD alen)
{
    HANDLE ptoken = 0;
    if ( !OpenProcessToken(hProcess, TOKEN_QUERY, &ptoken) )
        return -1;
    DWORD slen = 0;
    if ( !GetTokenInformation(ptoken, TokenUser, 0, 0, &slen) &&
         GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        int err = GetLastError();
        CloseHandle(ptoken);
        return -1;
    }
    TOKEN_USER *ptu = (TOKEN_USER *)new char[slen];
    if ( !ptu )
    {
        CloseHandle(ptoken);
        return -1;
    }
    memset(ptu, 0, slen);
    if ( !GetTokenInformation(ptoken, TokenUser, ptu, slen, &slen) )
    {
        delete ptu;
        CloseHandle(ptoken);
        return -1;
    }
    SID_NAME_USE snu;
    memset(&snu, 0, sizeof(snu));
    LookupAccountSid(0, ptu->User.Sid, account, &alen, domain, &dlen, &snu);
    delete ptu;
    CloseHandle(ptoken);
    return 0;
}
static int FindProcNT(FindProcData *search, int MAX_PIDS)
{
	int nsearch=0;

    // From Platform SDK documentation
    HINSTANCE pmod = LoadLibraryA("psapi.dll");
    if ( !pmod )
        return -1;
    BOOL (WINAPI *lpfEnumProcesses)(DWORD *, DWORD cb, DWORD *) =
        (BOOL (WINAPI *)(DWORD *, DWORD, DWORD *))
        GetProcAddress(pmod, "EnumProcesses");
    BOOL (WINAPI *lpfEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD) =
        (BOOL (WINAPI *)(HANDLE, HMODULE *, DWORD, LPDWORD))
        GetProcAddress(pmod, "EnumProcessModules");
    DWORD (WINAPI *lpfGetModuleFileNameEx)(HANDLE, HMODULE, LPCSTR, DWORD) =
        (DWORD (WINAPI *)(HANDLE, HMODULE, LPCSTR, DWORD))
        GetProcAddress(pmod, "GetModuleFileNameExA");
    if ( !lpfEnumProcesses ||
         !lpfEnumProcessModules ||
         !lpfGetModuleFileNameEx )
        return -1;

     // Call the PSAPI function EnumProcesses to get all of the
     // ProcID's currently in the system.
     // NOTE: In the documentation, the third parameter of
     // EnumProcesses is named cbNeeded, which implies that you
     // can call the function once to find out how much space to
     // allocate for a buffer and again to fill the buffer.
     // This is not the case. The cbNeeded parameter returns
     // the number of PIDs returned, so if your buffer size is
     // zero cbNeeded returns zero.
     // NOTE: The "HeapAlloc" loop here ensures that we
     // actually allocate a buffer large enough for all the
     // PIDs in the system.
     DWORD dwSize = 0;
     DWORD dwSize2 = 256 *sizeof(DWORD);
     LPDWORD lpdwPIDs = 0;
     do
     {
        if( lpdwPIDs )
        {
           HeapFree(GetProcessHeap(), 0, lpdwPIDs) ;
           dwSize2 *= 2 ;
        }
        lpdwPIDs = (LPDWORD)HeapAlloc(GetProcessHeap(), 0, dwSize2);
        if( lpdwPIDs == NULL )
        {
           FreeLibrary(pmod);
           return -1;
        }
        if( !lpfEnumProcesses(lpdwPIDs, dwSize2, &dwSize) )
        {
           HeapFree(GetProcessHeap(), 0, lpdwPIDs) ;
           FreeLibrary(pmod);
           return -1;
        }
     } while( dwSize == dwSize2 );

     // How many ProcID's did we get?
     dwSize /= sizeof(DWORD) ;

     // Loop through each ProcID.
     time_t tnow = time(0);
     for ( DWORD dwIndex=0; dwIndex<dwSize; dwIndex++ )
     {
        char szFileName[MAX_PATH];
        szFileName[0] = 0 ;
        // Open the process (if we can... security does not
        // permit every process in the system).
        HANDLE hProcess = OpenProcess(
           PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,
           false, lpdwPIDs[dwIndex]);
        if( hProcess )
        {
           // Here we call EnumProcessModules to get only the
           // first module in the process this is important,
           // because this will be the .EXE module for which we
           // will retrieve the full path name in a second.
           HMODULE hMod = 0;
           if( lpfEnumProcessModules(hProcess, &hMod, 
                sizeof(hMod), &dwSize2) )
           {
              // Get Full pathname:
              if( lpfGetModuleFileNameEx(hProcess, hMod,
                    szFileName, sizeof(szFileName)) )
              {
                    if ( nsearch < MAX_PIDS )
                    {
                        search[nsearch].pid = lpdwPIDs[dwIndex];
                        GetLongPathName(szFileName, search[nsearch].path, sizeof(search[nsearch].path));
                        search[nsearch].last = tnow;
                        GetProcUser(hProcess, search[nsearch].domain,255, search[nsearch].account,255);
                        nsearch ++;
                    }
              }
           }
           CloseHandle( hProcess ) ;
        }
     }
     HeapFree(GetProcessHeap(), 0, lpdwPIDs) ;
     FreeLibrary(pmod);
    return nsearch;
}
// Sort by path
static int _cdecl SortByCommand(const void *elem1, const void *elem2)
{
    FindProcData *fpd1 = (FindProcData *)elem1;
    FindProcData *fpd2 = (FindProcData *)elem2;
    return _stricmp(fpd1->path, fpd2->path);
}
int ps(FindProcData *pdata, int maxprocs)
{
    int nprocs=FindProcNT(pdata,maxprocs);
    qsort(pdata,nprocs,sizeof(FindProcData),SortByCommand);
    return nprocs;
}

#endif//WIN32

