/* $ $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/doscalls.h
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. robertk@mok.lvcm.com
 * REVISION HISTORY:
 *    13-03-2002  Created
 */



#include "os2def.h"


// FIXME: use ib headers
#define EXIT_THREAD					0
#define EXIT_PROCESS				1
#define NO_ERROR 0
#define ERROR_INVALID_HANDLE 5
#define ERROR_FILE_NOT_FOUND		6
// for this 

// Give the user nicer names that the internal ones
#define DosSleep				Dos32Sleep
#define DosCreateThread			Dos32CreateThread
#define DosOpen					Dos32Open
#define DosClose				Dos32Close
#define DosRead					Dos32Read
#define DosWrite				Dos32Write
#define DosBeep					Dos32Beep
#define DosExit					Dos32Exit
   

APIRET STDCALL Dos32Sleep(ULONG msec);

APIRET STDCALL Dos32CreateThread(PTID ptid,
                               PFNTHREAD pfn,
                               ULONG param,
                               ULONG flag,
                               ULONG cbStack);

APIRET STDCALL  Dos32Open(PSZ    pszFileName,
                        PHFILE pHf,
                        PULONG pulAction,
                        ULONG  cbFile,
                        ULONG  ulAttribute,
                        ULONG  fsOpenFlags,
                        ULONG  fsOpenMode,
                        PVOID reserved );  //ULONGPEAOP2 peaop2)

APIRET STDCALL  Dos32Close(HFILE hFile);

APIRET STDCALL  Dos32Read(HFILE hFile,
                        PVOID pBuffer,
                        ULONG cbRead,
                        PULONG pcbActual);

APIRET STDCALL  Dos32Write(HFILE hFile,
                         PVOID pBuffer,
                         ULONG cbWrite,
                         PULONG pcbActual);

APIRET STDCALL Dos32DevIOCtl(HFILE hDevice, ULONG category, ULONG function,
							PVOID pParams,ULONG cbParmLenMax,PULONG pcbParmLen,
							PVOID pData,ULONG cbDataLenMax,PULONG pcbDataLen);


APIRET STDCALL Dos32Beep(ULONG freq,
                       ULONG dur);

VOID STDCALL Dos32Exit(ULONG action,
                     ULONG result);










