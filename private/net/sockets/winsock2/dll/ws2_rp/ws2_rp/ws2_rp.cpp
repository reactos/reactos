//------------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Rights Reserved
//
// ws2_rp.cpp
//
//
//
//------------------------------------------------------------------------

#define UNICODE
#define INITGUID

#include <windows.h>
#include <compobj.h>
#include <ws2_rp.h>

//------------------------------------------------------------------------
//  These functions are in NT (advapi32.dll), but not in Windows95/98,
//  so will be explicitly loaded:
//------------------------------------------------------------------------

typedef BOOL (*OPEN_PROCESS_TOKEN)( HANDLE  hProcess,
                                    DWORD   dwDesiredAccess,
                                    HANDLE *phToken );

typedef BOOL (*IS_TOKEN_RESTRICTED)( HANDLE hToken );

//------------------------------------------------------------------------
// Globals:
//
// RP_Init() sets g_fIsRestricted to TRUE iff we're in a restricted process.
//
//------------------------------------------------------------------------

static BOOL  g_fIsInitialized = FALSE;
static BOOL  g_fIsRestricted = FALSE;

//------------------------------------------------------------------------
// RP_Init()
//
//------------------------------------------------------------------------
DWORD RP_Init()
{
   DWORD  dwStatus;
   DWORD  dwPid;
   HANDLE hProcess;
   HANDLE hToken;
   HINSTANCE  hInstance;
   OPEN_PROCESS_TOKEN   pOpenProcessToken;  // advapi32.dll
   IS_TOKEN_RESTRICTED  pIsTokenRestricted; // advapi32.dll

   g_fIsInitialized = TRUE;

   // We will need a couple of calls from advapi32.dll to look 
   // at the process's token (for NT5.0). We'll do a LoadLibrary()
   // so that if advapi32.dll isn't present (Windows98) then 
   // everything will keep on running...
   hInstance = LoadLibrary(TEXT("advapi32.dll"));
   if (!hInstance)
       {
       g_fIsRestricted = FALSE;
       return NO_ERROR;
       }

   pOpenProcessToken = (OPEN_PROCESS_TOKEN)GetProcAddress( hInstance, "OpenProcessToken" );
   pIsTokenRestricted = (IS_TOKEN_RESTRICTED)GetProcAddress( hInstance, "IsTokenRestricted" );
   if ( (!pOpenProcessToken) || (!pIsTokenRestricted) )
       {
       g_fIsRestricted = FALSE;
       return NO_ERROR;
       }

   // Ok, let's get the process token and check it to see if 
   // its restricted...

   dwPid = GetCurrentProcessId();   // Looks like this call can't fail.

   hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, TRUE, dwPid );
   if (!hProcess)
      {
      g_fIsRestricted = FALSE;
      #ifdef DBG
      dwStatus = GetLastError();
      #endif
      return NO_ERROR;           // Want RP_Init() to always succeed.
      }

   if (!pOpenProcessToken(hProcess,TOKEN_QUERY,&hToken))
      {
      g_fIsRestricted = FALSE;
      #ifdef DBG
      dwStatus = GetLastError();
      #endif
      CloseHandle(hProcess);
      return NO_ERROR;           // Want RP_Init() to always succeed.
      }

   CloseHandle(hProcess);

   g_fIsRestricted = (pIsTokenRestricted(hToken))? TRUE : FALSE;

   CloseHandle(hToken);

   return NO_ERROR;
}


//------------------------------------------------------------------------
// RP_IsRestrictedProcess()
//
//------------------------------------------------------------------------
inline BOOL RP_IsRestrictedProcess()
{
   if (!g_fIsInitialized)
       {
       DWORD dwStatus = RP_Init();
       }

   return g_fIsRestricted;
}

