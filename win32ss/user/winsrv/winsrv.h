#pragma once

/* PSDK/NDK Headers */
#include <stdio.h>
#include <windows.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <csr/server.h>
#include <win/base.h>
#include <win/windows.h>

VOID
WINAPI
Win32CsrHardError(IN PCSR_THREAD ThreadData,
                  IN PHARDERROR_MSG Message);
    
CSR_API(SrvRegisterServicesProcess);



/*****************************

/\*
typedef VOID (WINAPI *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;
*\/

/\* exitros.c *\/
CSR_API(CsrExitReactos);
CSR_API(CsrSetLogonNotifyWindow);
CSR_API(CsrRegisterLogonProcess);
// CSR_API(CsrRegisterSystemClasses);

*****************************/

/* EOF */
