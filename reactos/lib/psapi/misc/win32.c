/* $Id: win32.c,v 1.4 2002/08/31 17:11:24 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/win32.c
 * PURPOSE:     Win32 interfaces for PSAPI
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <windows.h>
#include <psapi.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ddk/ntddk.h>
#include <internal/psapi.h>

/* EmptyWorkingSet */
BOOL STDCALL EmptyWorkingSet(HANDLE hProcess)
{
 NTSTATUS nErrCode;
 QUOTA_LIMITS qlProcessQuota;

 /* query the working set */
 nErrCode = NtQueryInformationProcess
 (
  hProcess,
  ProcessQuotaLimits,
  &qlProcessQuota,
  sizeof(qlProcessQuota),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
  goto fail;

 /* empty the working set */
 qlProcessQuota.MinimumWorkingSetSize = -1;
 qlProcessQuota.MaximumWorkingSetSize = -1;

 /* set the working set */
 nErrCode = NtSetInformationProcess
 (
  hProcess,
  ProcessQuotaLimits,
  &qlProcessQuota,
  sizeof(qlProcessQuota)
 );

 /* success */
 if(NT_SUCCESS(nErrCode))
  return (TRUE);

fail:
 /* failure */
 SetLastError(RtlNtStatusToDosError(nErrCode));
 return (FALSE);
}

/* EnumDeviceDrivers */
/* callback context */
typedef struct _ENUM_DEVICE_DRIVERS_CONTEXT
{
 LPVOID *lpImageBase;
 DWORD nCount;
} ENUM_DEVICE_DRIVERS_CONTEXT, *PENUM_DEVICE_DRIVERS_CONTEXT;

/* callback routine */
NTSTATUS STDCALL EnumDeviceDriversCallback
(
 IN ULONG ModuleCount,
 IN PSYSTEM_MODULE_ENTRY CurrentModule,
 IN OUT PVOID CallbackContext
)
{
 register PENUM_DEVICE_DRIVERS_CONTEXT peddcContext =
  (PENUM_DEVICE_DRIVERS_CONTEXT)CallbackContext;

 /* no more buffer space */
 if(peddcContext->nCount == 0)
  return STATUS_INFO_LENGTH_MISMATCH;

 /* return current module */
 *(peddcContext->lpImageBase) = CurrentModule->BaseAddress;

 /* go to next array slot */
 (peddcContext->lpImageBase) ++;
 (peddcContext->nCount) --;

 return STATUS_SUCCESS;
}

/* exported interface */
BOOL STDCALL EnumDeviceDrivers
(
 LPVOID *lpImageBase,
 DWORD cb,
 LPDWORD lpcbNeeded
)
{
 register NTSTATUS nErrCode;
 ENUM_DEVICE_DRIVERS_CONTEXT eddcContext = {lpImageBase, cb / sizeof(PVOID)};

 cb /= sizeof(PVOID);

 /* do nothing if the buffer is empty */
 if(cb == 0 || lpImageBase == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }

 /* enumerate the system modules */
 nErrCode = PsaEnumerateSystemModules(&EnumDeviceDriversCallback, &eddcContext);

 /* return the count of bytes returned */
 *lpcbNeeded = (cb - eddcContext.nCount) * sizeof(PVOID);

 /* success */
 if(NT_SUCCESS(nErrCode) || nErrCode == STATUS_INFO_LENGTH_MISMATCH)
  return (TRUE);
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* EnumProcesses */
/* callback context */
typedef struct _ENUM_PROCESSES_CONTEXT
{
 DWORD *lpidProcess;
 DWORD nCount;
} ENUM_PROCESSES_CONTEXT, *PENUM_PROCESSES_CONTEXT;

/* callback routine */
NTSTATUS STDCALL EnumProcessesCallback
(
 IN PSYSTEM_PROCESS_INFORMATION CurrentProcess,
 IN OUT PVOID CallbackContext
)
{
 register PENUM_PROCESSES_CONTEXT pepcContext =
  (PENUM_PROCESSES_CONTEXT)CallbackContext;

 /* no more buffer space */
 if(pepcContext->nCount == 0)
  return STATUS_INFO_LENGTH_MISMATCH;

 /* return current process */
 *(pepcContext->lpidProcess) = CurrentProcess->ProcessId;

 /* go to next array slot */
 (pepcContext->lpidProcess) ++;
 (pepcContext->nCount) --;

 return STATUS_SUCCESS;
}

/* exported interface */
BOOL STDCALL EnumProcesses
(
 DWORD *lpidProcess,
 DWORD cb,
 LPDWORD lpcbNeeded
)
{
 register NTSTATUS nErrCode;
 ENUM_PROCESSES_CONTEXT epcContext = {lpidProcess, cb / sizeof(DWORD)};

 cb /= sizeof(DWORD);

 /* do nothing if the buffer is empty */
 if(cb == 0 || lpidProcess == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }

 /* enumerate the process ids */
 nErrCode = PsaEnumerateProcesses(&EnumProcessesCallback, &epcContext);

 *lpcbNeeded = (cb - epcContext.nCount) * sizeof(DWORD);

 /* success */
 if(NT_SUCCESS(nErrCode) || nErrCode == STATUS_INFO_LENGTH_MISMATCH)
  return (TRUE);
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* EnumProcessModules */
/* callback context */
typedef struct _ENUM_PROCESS_MODULES_CONTEXT
{
 HMODULE *lphModule;
 DWORD nCount;
} ENUM_PROCESS_MODULES_CONTEXT, *PENUM_PROCESS_MODULES_CONTEXT;

/* callback routine */
NTSTATUS STDCALL EnumProcessModulesCallback
(
 IN HANDLE ProcessHandle,
 IN PLDR_MODULE CurrentModule,
 IN OUT PVOID CallbackContext
)
{
 register PENUM_PROCESS_MODULES_CONTEXT pepmcContext =
  (PENUM_PROCESS_MODULES_CONTEXT)CallbackContext;

 /* no more buffer space */
 if(pepmcContext->nCount == 0)
  return STATUS_INFO_LENGTH_MISMATCH;

 /* return current process */
 *(pepmcContext->lphModule) = CurrentModule->BaseAddress;

 /* go to next array slot */
 (pepmcContext->lphModule) ++;
 (pepmcContext->nCount) --;

 return STATUS_SUCCESS;
}

/* exported interface */
BOOL STDCALL EnumProcessModules(
  HANDLE hProcess,
  HMODULE *lphModule,
  DWORD cb,
  LPDWORD lpcbNeeded
)
{
 register NTSTATUS nErrCode;
 ENUM_PROCESS_MODULES_CONTEXT epmcContext = {lphModule, cb / sizeof(HMODULE)};

 cb /= sizeof(DWORD);

 /* do nothing if the buffer is empty */
 if(cb == 0 || lphModule == NULL)
 {
  *lpcbNeeded = 0;
  return (TRUE);
 }

 /* enumerate the process modules */
 nErrCode = PsaEnumerateProcessModules
 (
  hProcess,
  &EnumProcessModulesCallback,
  &epmcContext
 );

 *lpcbNeeded = (cb - epmcContext.nCount) * sizeof(DWORD);

 /* success */
 if(NT_SUCCESS(nErrCode) || nErrCode == STATUS_INFO_LENGTH_MISMATCH)
  return (TRUE);
 else
 {
  /* failure */
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return (FALSE);
 }
}

/* GetDeviceDriverBase/FileName */
/* common callback context */
typedef struct _GET_DEVICE_DRIVER_NAME_CONTEXT
{
 LPVOID ImageBase;
 struct
 {
  ULONG bFullName:sizeof(ULONG) * 8 / 2;
  ULONG bUnicode:sizeof(ULONG) * 8 / 2;
 };
 DWORD nSize;
 union
 {
  LPVOID lpName;
  LPSTR lpAnsiName;
  LPWSTR lpUnicodeName;
 };
} GET_DEVICE_DRIVER_NAME_CONTEXT, *PGET_DEVICE_DRIVER_NAME_CONTEXT;

/* common callback routine */
NTSTATUS STDCALL GetDeviceDriverNameCallback
(
 IN ULONG ModuleCount,
 IN PSYSTEM_MODULE_ENTRY CurrentModule,
 IN OUT PVOID CallbackContext
)
{
 register PGET_DEVICE_DRIVER_NAME_CONTEXT pgddncContext =
  (PGET_DEVICE_DRIVER_NAME_CONTEXT) CallbackContext;

 /* module found */
 if(pgddncContext->ImageBase == CurrentModule->BaseAddress)
 {
  register PCHAR pcModuleName;
  register ULONG l;

  /* get the full name or just the filename part */
  if(pgddncContext->bFullName)
   pcModuleName = &CurrentModule->Name[0];
  else
   pcModuleName = &CurrentModule->Name[CurrentModule->PathLength];

  /* get the length of the name */
  l = strlen(pcModuleName);

  /* if user buffer smaller than the name */
  if(pgddncContext->nSize <= l)
   /* use the user buffer's length */
   l = pgddncContext->nSize;
  /* if user buffer larger than the name */
  else
  {
   /* enough space for the null terminator */
   l ++;
   pgddncContext->nSize = l;
  }

  /* copy the string */
  if(pgddncContext->bUnicode)
  {
   /* Unicode: convert and copy */
   ANSI_STRING strAnsi = {l, l, pcModuleName};
   UNICODE_STRING wstrUnicode =
   {
     0,
     l * sizeof(WCHAR),
     pgddncContext->lpUnicodeName
   };
   /* driver names should always be in language-neutral ASCII, so we don't
      bother calling AreFileApisANSI() */
   RtlAnsiStringToUnicodeString(&wstrUnicode, &strAnsi, FALSE);
  }
  else
   /* ANSI/OEM: direct copy */
   memcpy(pgddncContext->lpAnsiName, pcModuleName, l);

  /* terminate the enumeration */
  return STATUS_NO_MORE_FILES;
 }
 /* continue searching */
 else
  return STATUS_SUCCESS;
}

/* common internal implementation */
DWORD FASTCALL internalGetDeviceDriverName(
  BOOLEAN bUnicode,
  BOOLEAN bFullName,
  LPVOID ImageBase,
  LPVOID lpName,
  DWORD nSize
)
{
 register NTSTATUS nErrCode;
 GET_DEVICE_DRIVER_NAME_CONTEXT gddncContext =
 {
  ImageBase,
  { bFullName, bUnicode },
  nSize,
  { lpName }
 };

 /* empty buffer */
 if(lpName == NULL || nSize == 0)
  return 0;

 /* invalid image base */
 if(ImageBase == NULL)
 {
  SetLastError(ERROR_INVALID_HANDLE);
  return 0;
 }

 /* start the enumeration */
 nErrCode = PsaEnumerateSystemModules
 (
  &GetDeviceDriverNameCallback,
  &gddncContext
 );

 if(nErrCode == STATUS_NO_MORE_FILES)
  /* module was found, return string size */
  return gddncContext.nSize;
 else
 {
  if(NT_SUCCESS(nErrCode))
   /* module was not found */
   SetLastError(ERROR_INVALID_HANDLE);
  else
   /* an error occurred */
   SetLastError(RtlNtStatusToDosError(nErrCode));

  /* failure */
  return 0;
 }
}

/* exported interfaces */
/*
 NOTES:
  - nSize is not, as stated by Microsoft's documentation, the byte size, but the
    count of characters in the buffer
  - the return value is the count of characters copied into the buffer
  - the functions don't attempt to null-terminate the string
 */
DWORD STDCALL GetDeviceDriverBaseNameA(
  LPVOID ImageBase,
  LPSTR lpBaseName,
  DWORD nSize
)
{
 return internalGetDeviceDriverName(FALSE, FALSE, ImageBase, lpBaseName, nSize);
}

DWORD STDCALL GetDeviceDriverFileNameA(
  LPVOID ImageBase,
  LPSTR lpFilename,
  DWORD nSize
)
{
 return internalGetDeviceDriverName(FALSE, TRUE, ImageBase, lpFilename, nSize);
}

DWORD STDCALL GetDeviceDriverBaseNameW(
  LPVOID ImageBase,
  LPWSTR lpBaseName,
  DWORD nSize
)
{
 return internalGetDeviceDriverName(TRUE, FALSE, ImageBase, lpBaseName, nSize);
}

DWORD STDCALL GetDeviceDriverFileNameW(
  LPVOID ImageBase,
  LPWSTR lpFilename,
  DWORD nSize
)
{
 return internalGetDeviceDriverName(TRUE, TRUE, ImageBase, lpFilename, nSize);
}

/* GetMappedFileName */
/* common internal implementation */
DWORD FASTCALL internalGetMappedFileName(
  BOOLEAN bUnicode,
  HANDLE hProcess,    
  LPVOID lpv,         
  LPVOID lpName,   
  DWORD nSize         
)
{
 register NTSTATUS nErrCode;
 register ULONG nBufSize;
 PMEMORY_SECTION_NAME pmsnName;

 /* empty buffer */
 if(nSize == 0 || (LPSTR)lpName == NULL)
  return 0;

 if(nSize > (0xFFFF / sizeof(WCHAR)))
  /* if the user buffer contains more characters than would fit in an
     UNICODE_STRING, limit the buffer size. RATIONALE: we don't limit buffer
     size elsewhere because here superfluous buffer size will mean a larger
     temporary buffer */
  nBufSize = 0xFFFF / sizeof(WCHAR);
 else
  nBufSize = nSize * sizeof(WCHAR);
 
 /* allocate the memory */
 pmsnName = malloc(nBufSize + offsetof(MEMORY_SECTION_NAME, NameBuffer));
 
 if(pmsnName == NULL)
 {
  /* failure */
  SetLastError(ERROR_OUTOFMEMORY);
  return 0;
 }

 /* initialize the destination buffer */
 pmsnName->SectionFileName.Length = 0;
 pmsnName->SectionFileName.Length = nBufSize;

#if 0
 __try
 {
#endif
  /* query the name */
  nErrCode = NtQueryVirtualMemory
  (
   hProcess,
   lpv,
   MemorySectionName,
   pmsnName,
   nBufSize,
   NULL
  );
  
  if(!NT_SUCCESS(nErrCode))
  {
   /* failure */
   SetLastError(RtlNtStatusToDosError(nErrCode));
#if 0
#else
   /* free the buffer */
   free(pmsnName);
#endif
   return 0;
  }
  
  /* copy the name */
  if(bUnicode)
  {
   /* destination is an Unicode string: direct copy */
   memcpy
   (
    (LPWSTR)lpName,
    pmsnName->NameBuffer,
    pmsnName->SectionFileName.Length
   );
   
#if 0
#else
   /* free the buffer */
   free(pmsnName);
#endif
   
   if(pmsnName->SectionFileName.Length < nSize)
   {
    /* null-terminate the string */
    ((LPWSTR)lpName)[pmsnName->SectionFileName.Length] = 0;
    return pmsnName->SectionFileName.Length + 1;
   }
   
   return pmsnName->SectionFileName.Length;
  }
  else
  {
   ANSI_STRING strAnsi = {0, nSize, (LPSTR)lpName};

   if(AreFileApisANSI())
    /* destination is an ANSI string: convert and copy */
    RtlUnicodeStringToAnsiString(&strAnsi, &pmsnName->SectionFileName, FALSE);
   else
    /* destination is an OEM string: convert and copy */
    RtlUnicodeStringToOemString(&strAnsi, &pmsnName->SectionFileName, FALSE);

#if 0
#else
   /* free the buffer */
   free(pmsnName);
#endif

   if(strAnsi.Length < nSize)
   {
    /* null-terminate the string */
    ((LPSTR)lpName)[strAnsi.Length] = 0;
    return strAnsi.Length + 1;
   }

   return strAnsi.Length;
  }

#if 0
 }
 __finally
 {
  free(pmsnName);
 }
#endif
}

/* exported interfaces */
DWORD STDCALL GetMappedFileNameA(
  HANDLE hProcess,    
  LPVOID lpv,         
  LPSTR lpFilename,   
  DWORD nSize         
)
{
 return internalGetMappedFileName(FALSE, hProcess, lpv, lpFilename, nSize);
}

DWORD STDCALL GetMappedFileNameW(
  HANDLE hProcess,    
  LPVOID lpv,         
  LPWSTR lpFilename,  
  DWORD nSize         
)
{
 return internalGetMappedFileName(TRUE, hProcess, lpv, lpFilename, nSize);
}

/* GetModuleInformation */
/* common callback context */
typedef struct _GET_MODULE_INFORMATION_FLAGS
{
 ULONG bWantName:sizeof(ULONG) * 8 / 4;
 ULONG bUnicode:sizeof(ULONG) * 8 / 4;
 ULONG bFullName:sizeof(ULONG) * 8 / 4;
} GET_MODULE_INFORMATION_FLAGS, *PGET_MODULE_INFORMATION_FLAGS;

typedef struct _GET_MODULE_INFORMATION_CONTEXT
{
 HMODULE hModule;
 GET_MODULE_INFORMATION_FLAGS Flags;
 DWORD nBufSize;
 union
 {
  LPWSTR lpUnicodeName;
  LPSTR lpAnsiName;
  LPMODULEINFO lpmodinfo;
  LPVOID lpBuffer;
 };
} GET_MODULE_INFORMATION_CONTEXT, *PGET_MODULE_INFORMATION_CONTEXT;

/* common callback */
NTSTATUS STDCALL GetModuleInformationCallback
(
 IN HANDLE ProcessHandle,
 IN PLDR_MODULE CurrentModule,
 IN OUT PVOID CallbackContext
)
{
 register PGET_MODULE_INFORMATION_CONTEXT pgmicContext =
  (PGET_MODULE_INFORMATION_CONTEXT)CallbackContext;

 /* found the module we were looking for */
 if(CurrentModule->BaseAddress == pgmicContext->hModule)
 {
  /* we want the module name */
  if(pgmicContext->Flags.bWantName)
  {
   register NTSTATUS nErrCode;
   register PUNICODE_STRING pwstrSource;
   register ULONG l;
   
   if(pgmicContext->Flags.bFullName)
    /* full name */
    pwstrSource = &(CurrentModule->FullDllName);
   else
    /* base name only */
    pwstrSource = &(CurrentModule->BaseDllName);
   
   /* paranoia */
   pwstrSource->Length -= pwstrSource->Length % sizeof(WCHAR);
   
   /* l is the byte size of the user buffer */
   l = pgmicContext->nBufSize * sizeof(WCHAR);
   
   /* if the user buffer has room for the string and a null terminator */
   if(l >= (pwstrSource->Length + sizeof(WCHAR)))
   {
    /* limit the buffer size */
    l = pwstrSource->Length;
    
    /* null-terminate the string */
    if(pgmicContext->Flags.bUnicode)
     pgmicContext->lpUnicodeName[l / sizeof(WCHAR)] = 0;
    else
     pgmicContext->lpAnsiName[l / sizeof(WCHAR)] = 0;
   }

   if(pgmicContext->Flags.bUnicode)
   {
    /* Unicode: direct copy */
    /* NOTE: I've chosen not to check for ProcessHandle == NtCurrentProcess(),
       this function is complicated enough as it is */
    nErrCode = NtReadVirtualMemory
    (
     ProcessHandle,
     pwstrSource->Buffer,
     pgmicContext->lpUnicodeName,
     l,
     NULL
    );

    if(NT_SUCCESS(nErrCode))
     pgmicContext->nBufSize = l / sizeof(WCHAR);
    else
    {
     pgmicContext->nBufSize = 0;
     return nErrCode;
    }
   }
   else
   {
    /* ANSI/OEM: convert and copy */
    register LPWSTR pwcUnicodeBuf;
    ANSI_STRING strAnsi = {0, pgmicContext->nBufSize, pgmicContext->lpAnsiName};
    UNICODE_STRING wstrUnicodeBuf;
    
    /* allocate the local buffer */
    pwcUnicodeBuf = malloc(pwstrSource->Length);

#if 0
    __try
    {
#endif
     if(pwcUnicodeBuf == NULL)
      /* failure */
#if 0
      return STATUS_NO_MEMORY;
#else
     {
      nErrCode = STATUS_NO_MEMORY;
      goto exitWithStatus;
     }
#endif
 
     /* copy the string in the local buffer */
     nErrCode = NtReadVirtualMemory
     (
      ProcessHandle,
      pwstrSource->Buffer,
      pwcUnicodeBuf,
      l,
      NULL
     );
 
     if(!NT_SUCCESS(nErrCode))
      /* failure */
#if 0
      return nErrCode;
#else
      goto exitWithStatus;
#endif
     
     /* initialize Unicode string buffer */
     wstrUnicodeBuf.Length = wstrUnicodeBuf.MaximumLength = l;
     wstrUnicodeBuf.Buffer = pwcUnicodeBuf;
     
     /* convert and copy */
     if(AreFileApisANSI())
      RtlUnicodeStringToAnsiString(&strAnsi, &wstrUnicodeBuf, FALSE);
     else
      RtlUnicodeStringToOemString(&strAnsi, &wstrUnicodeBuf, FALSE);
     
     /* return the string size */
     pgmicContext->nBufSize = strAnsi.Length;
#if 0
    }
    __finally
    {
     /* free the buffer */
     free(pwcUnicodeBuf);
    }
#else
     /* success */
     nErrCode = STATUS_NO_MORE_FILES;

exitWithStatus:
     /* free the buffer */
     free(pwcUnicodeBuf);
     return nErrCode;
#endif
   }
   
  }
  /* we want other module information */
  else
  {
   register ULONG nSize = pgmicContext->nBufSize;
   
   /* base address */
   if(nSize >= sizeof(CurrentModule->BaseAddress))
   {
    pgmicContext->lpmodinfo->lpBaseOfDll = CurrentModule->BaseAddress;
    nSize -= sizeof(CurrentModule->BaseAddress);
   }
   
   /* image size */
   if(nSize >= sizeof(CurrentModule->SizeOfImage))
   {
    pgmicContext->lpmodinfo->SizeOfImage = CurrentModule->SizeOfImage;
    nSize -= sizeof(CurrentModule->SizeOfImage);
   }
   
   /* entry point */
   if(nSize >= sizeof(CurrentModule->EntryPoint))
    /* ??? FIXME? is "EntryPoint" just the offset, or the real address? */
    pgmicContext->lpmodinfo->EntryPoint = (PVOID)CurrentModule->EntryPoint;
   
   pgmicContext->nBufSize = TRUE;
  }
  
  return STATUS_NO_MORE_FILES;
 }

 return STATUS_SUCCESS;
}

/* common internal implementation */
DWORD FASTCALL internalGetModuleInformation(
  HANDLE hProcess,
  HMODULE hModule,
  GET_MODULE_INFORMATION_FLAGS Flags,
  LPVOID lpBuffer,
  DWORD nBufSize
)
{
 register NTSTATUS nErrCode;
 GET_MODULE_INFORMATION_CONTEXT gmicContext =
 {
  hModule,
  Flags,
  nBufSize,
  {lpBuffer}
 };


 nErrCode = PsaEnumerateProcessModules
 (
  hProcess,
  &GetModuleInformationCallback,
  &gmicContext
 );

 if(nErrCode == STATUS_NO_MORE_FILES)
  return gmicContext.nBufSize;
 else
 {
  if(NT_SUCCESS(nErrCode))
   SetLastError(ERROR_INVALID_HANDLE);
  else
   SetLastError(RtlNtStatusToDosError(nErrCode));

  return 0;
 }
}

/* exported interfaces */
DWORD STDCALL GetModuleBaseNameA(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPSTR lpBaseName,   // base name buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 register GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, FALSE, FALSE};
 return internalGetModuleInformation
 (
  hProcess,
  hModule,
  Flags,
  lpBaseName,
  nSize
 );
}

DWORD STDCALL GetModuleBaseNameW(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPWSTR lpBaseName,  // base name buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 register GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, TRUE, FALSE};
 return internalGetModuleInformation
 (
  hProcess,
  hModule,
  Flags,
  lpBaseName,
  nSize
 );
}

DWORD STDCALL GetModuleFileNameExA(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPSTR lpFilename,   // path buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 register GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, FALSE, TRUE};
 return internalGetModuleInformation
 (
  hProcess,
  hModule,
  Flags,
  lpFilename,
  nSize
 );
}

DWORD STDCALL GetModuleFileNameExW(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPWSTR lpFilename,  // path buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 register GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, TRUE, TRUE};
 return internalGetModuleInformation
 (
  hProcess,
  hModule,
  Flags,
  lpFilename,
  nSize
 );
}

BOOL STDCALL GetModuleInformation(
  HANDLE hProcess,         // handle to process
  HMODULE hModule,         // handle to module
  LPMODULEINFO lpmodinfo,  // information buffer
  DWORD cb                 // size of buffer
)
{
 register GET_MODULE_INFORMATION_FLAGS Flags = {FALSE, FALSE, FALSE};
 return (BOOL)internalGetModuleInformation
 (
  hProcess,
  hModule,
  Flags,
  lpmodinfo,
  cb
 );
}
/* EOF */

