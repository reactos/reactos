/* $Id: win32.c,v 1.11 2004/11/02 23:42:49 weiden Exp $
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

#include "precomp.h"

/* INTERNAL *******************************************************************/

typedef struct _ENUM_DEVICE_DRIVERS_CONTEXT
{
  LPVOID *lpImageBase;
  DWORD nCount;
} ENUM_DEVICE_DRIVERS_CONTEXT, *PENUM_DEVICE_DRIVERS_CONTEXT;

NTSTATUS STDCALL
EnumDeviceDriversCallback(IN PSYSTEM_MODULE_INFORMATION_ENTRY CurrentModule,
                          IN OUT PVOID CallbackContext)
{
  PENUM_DEVICE_DRIVERS_CONTEXT Context = (PENUM_DEVICE_DRIVERS_CONTEXT)CallbackContext;

  /* no more buffer space */
  if(Context->nCount == 0)
  {
    return STATUS_INFO_LENGTH_MISMATCH;
  }

  /* return current module */
  *Context->lpImageBase = CurrentModule->Base;

  /* go to next array slot */
  Context->lpImageBase++;
  Context->nCount--;

  return STATUS_SUCCESS;
}


typedef struct _ENUM_PROCESSES_CONTEXT
{
  DWORD *lpidProcess;
  DWORD nCount;
} ENUM_PROCESSES_CONTEXT, *PENUM_PROCESSES_CONTEXT;

NTSTATUS STDCALL
EnumProcessesCallback(IN PSYSTEM_PROCESSES CurrentProcess,
                      IN OUT PVOID CallbackContext)
{
  PENUM_PROCESSES_CONTEXT Context = (PENUM_PROCESSES_CONTEXT)CallbackContext;

  /* no more buffer space */
  if(Context->nCount == 0)
  {
    return STATUS_INFO_LENGTH_MISMATCH;
  }

  /* return current process */
  *Context->lpidProcess = CurrentProcess->ProcessId;

  /* go to next array slot */
  Context->lpidProcess++;
  Context->nCount--;

  return STATUS_SUCCESS;
}


typedef struct _ENUM_PROCESS_MODULES_CONTEXT
{
  HMODULE *lphModule;
  DWORD nCount;
} ENUM_PROCESS_MODULES_CONTEXT, *PENUM_PROCESS_MODULES_CONTEXT;

NTSTATUS STDCALL
EnumProcessModulesCallback(IN HANDLE ProcessHandle,
                           IN PLDR_MODULE CurrentModule,
                           IN OUT PVOID CallbackContext)
{
  PENUM_PROCESS_MODULES_CONTEXT Context = (PENUM_PROCESS_MODULES_CONTEXT)CallbackContext;

  /* no more buffer space */
  if(Context->nCount == 0)
  {
    return STATUS_INFO_LENGTH_MISMATCH;
  }

  /* return current process */
  *Context->lphModule = CurrentModule->BaseAddress;

  /* go to next array slot */
  Context->lphModule++;
  Context->nCount--;

  return STATUS_SUCCESS;
}


typedef struct _GET_DEVICE_DRIVER_NAME_CONTEXT
{
  LPVOID ImageBase;
  struct
  {
    ULONG bFullName : sizeof(ULONG) * 8 / 2;
    ULONG bUnicode : sizeof(ULONG) * 8 / 2;
  };
  DWORD nSize;
  union
  {
    LPVOID lpName;
    LPSTR lpAnsiName;
    LPWSTR lpUnicodeName;
  };
} GET_DEVICE_DRIVER_NAME_CONTEXT, *PGET_DEVICE_DRIVER_NAME_CONTEXT;

NTSTATUS STDCALL
GetDeviceDriverNameCallback(IN PSYSTEM_MODULE_INFORMATION_ENTRY CurrentModule,
                            IN OUT PVOID CallbackContext)
{
  PGET_DEVICE_DRIVER_NAME_CONTEXT Context = (PGET_DEVICE_DRIVER_NAME_CONTEXT)CallbackContext;

  /* module found */
  if(Context->ImageBase == CurrentModule->Base)
  {
    PCHAR pcModuleName;
    ULONG l;

    /* get the full name or just the filename part */
    if(Context->bFullName)
      pcModuleName = &CurrentModule->ImageName[0];
    else
      pcModuleName = &CurrentModule->ImageName[CurrentModule->PathLength];

    /* get the length of the name */
    l = strlen(pcModuleName);

    if(Context->nSize <= l)
    {
      /* use the user buffer's length */
      l = Context->nSize;
    }
    else
    {
      /* enough space for the null terminator */
      Context->nSize = ++l;
    }

    /* copy the string */
    if(Context->bUnicode)
    {
      ANSI_STRING AnsiString;
      UNICODE_STRING UnicodeString;

      UnicodeString.Length = 0;
      UnicodeString.MaximumLength = l * sizeof(WCHAR);
      UnicodeString.Buffer = Context->lpUnicodeName;

      RtlInitAnsiString(&AnsiString, pcModuleName);
      /* driver names should always be in language-neutral ASCII, so we don't
         bother calling AreFileApisANSI() */
      RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);
    }
    else
    {
      memcpy(Context->lpAnsiName, pcModuleName, l);
    }

    /* terminate the enumeration */
    return STATUS_NO_MORE_FILES;
  }
  else
  {
    /* continue searching */
    return STATUS_SUCCESS;
  }
}


static DWORD
InternalGetDeviceDriverName(BOOLEAN bUnicode,
                            BOOLEAN bFullName,
                            LPVOID ImageBase,
                            LPVOID lpName,
                            DWORD nSize)
{
  GET_DEVICE_DRIVER_NAME_CONTEXT Context;
  NTSTATUS Status;

  if(lpName == NULL || nSize == 0)
  {
    return 0;
  }

  if(ImageBase == NULL)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return 0;
  }

  Context.ImageBase = ImageBase;
  Context.bFullName = bFullName;
  Context.bUnicode = bUnicode;
  Context.nSize = nSize;
  Context.lpName = lpName;

  /* start the enumeration */
  Status = PsaEnumerateSystemModules(GetDeviceDriverNameCallback, &Context);

  if(Status == STATUS_NO_MORE_FILES)
  {
    /* module was found, return string size */
    return Context.nSize;
  }
  else if(NT_SUCCESS(Status))
  {
    /* module was not found */
    SetLastError(ERROR_INVALID_HANDLE);
  }
  else
  {
    /* an error occurred */
    SetLastErrorByStatus(Status);
  }
  return 0;
}


static DWORD
InternalGetMappedFileName(BOOLEAN bUnicode,
                          HANDLE hProcess,
                          LPVOID lpv,
                          LPVOID lpName,
                          DWORD nSize)
{
  PMEMORY_SECTION_NAME pmsnName;
  ULONG nBufSize;
  NTSTATUS Status;

  if(nSize == 0 || lpName == NULL)
  {
    return 0;
  }

  if(nSize > (0xFFFF / sizeof(WCHAR)))
  {
    /* if the user buffer contains more characters than would fit in an
       UNICODE_STRING, limit the buffer size. RATIONALE: we don't limit buffer
       size elsewhere because here superfluous buffer size will mean a larger
       temporary buffer */
    nBufSize = 0xFFFF / sizeof(WCHAR);
  }
  else
  {
    nBufSize = nSize * sizeof(WCHAR);
  }

  /* allocate the memory */
  pmsnName = PsaiMalloc(nBufSize + offsetof(MEMORY_SECTION_NAME, NameBuffer));

  if(pmsnName == NULL)
  {
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
   Status = NtQueryVirtualMemory(hProcess,
                                 lpv,
                                 MemorySectionName,
                                 pmsnName,
                                 nBufSize,
                                 NULL);
   if(!NT_SUCCESS(Status))
   {
     PsaiFree(pmsnName);
     SetLastErrorByStatus(Status);
     return 0;
   }

   if(bUnicode)
   {
     /* destination is an Unicode string: direct copy */
     memcpy((LPWSTR)lpName, pmsnName->NameBuffer, pmsnName->SectionFileName.Length);

     PsaiFree(pmsnName);

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
     ANSI_STRING AnsiString;

     AnsiString.Length = 0;
     AnsiString.MaximumLength = nSize;
     AnsiString.Buffer = (LPSTR)lpName;

     if(AreFileApisANSI())
       RtlUnicodeStringToAnsiString(&AnsiString, &pmsnName->SectionFileName, FALSE);
     else
       RtlUnicodeStringToOemString(&AnsiString, &pmsnName->SectionFileName, FALSE);

     PsaiFree(pmsnName);

     if(AnsiString.Length < nSize)
     {
       /* null-terminate the string */
       ((LPSTR)lpName)[AnsiString.Length] = 0;
       return AnsiString.Length + 1;
     }

     return AnsiString.Length;
   }

#if 0
   }
   __finally
   {
     PsaiFree(pmsnName);
   }
#endif
}


typedef struct _GET_MODULE_INFORMATION_FLAGS
{
  ULONG bWantName : sizeof(ULONG) * 8 / 4;
  ULONG bUnicode : sizeof(ULONG) * 8 / 4;
  ULONG bFullName : sizeof(ULONG) * 8 / 4;
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

NTSTATUS STDCALL
GetModuleInformationCallback(IN HANDLE ProcessHandle,
                             IN PLDR_MODULE CurrentModule,
                             IN OUT PVOID CallbackContext)
{
  PGET_MODULE_INFORMATION_CONTEXT Context = (PGET_MODULE_INFORMATION_CONTEXT)CallbackContext;

  /* found the module we were looking for */
  if(CurrentModule->BaseAddress == Context->hModule)
  {
    /* we want the module name */
    if(Context->Flags.bWantName)
    {
      PUNICODE_STRING SourceString;
      ULONG l;
      NTSTATUS Status;

      if(Context->Flags.bFullName)
        SourceString = &(CurrentModule->FullDllName);
      else
        SourceString = &(CurrentModule->BaseDllName);

      SourceString->Length -= SourceString->Length % sizeof(WCHAR);

      /* l is the byte size of the user buffer */
      l = Context->nBufSize * sizeof(WCHAR);

      /* if the user buffer has room for the string and a null terminator */
      if(l >= (SourceString->Length + sizeof(WCHAR)))
      {
        /* limit the buffer size */
        l = SourceString->Length;

        /* null-terminate the string */
        if(Context->Flags.bUnicode)
          Context->lpUnicodeName[l / sizeof(WCHAR)] = 0;
        else
          Context->lpAnsiName[l / sizeof(WCHAR)] = 0;
      }

      if(Context->Flags.bUnicode)
      {
        /* Unicode: direct copy */
        /* NOTE: I've chosen not to check for ProcessHandle == NtCurrentProcess(),
                 this function is complicated enough as it is */
        Status = NtReadVirtualMemory(ProcessHandle,
                                     SourceString->Buffer,
                                     Context->lpUnicodeName,
                                     l,
                                     NULL);

        if(!NT_SUCCESS(Status))
        {
          Context->nBufSize = 0;
          return Status;
        }

        Context->nBufSize = l / sizeof(WCHAR);
      }
      else
      {
        /* ANSI/OEM: convert and copy */
        LPWSTR pwcUnicodeBuf;
        ANSI_STRING AnsiString;
        UNICODE_STRING UnicodeString;

        AnsiString.Length = 0;
        AnsiString.MaximumLength = Context->nBufSize;
        AnsiString.Buffer = Context->lpAnsiName;

        /* allocate the local buffer */
        pwcUnicodeBuf = PsaiMalloc(SourceString->Length);

#if 0
        __try
        {
#endif
        if(pwcUnicodeBuf == NULL)
        {
          Status = STATUS_NO_MEMORY;
          goto exitWithStatus;
        }

        /* copy the string in the local buffer */
        Status = NtReadVirtualMemory(ProcessHandle,
                                     SourceString->Buffer,
                                     pwcUnicodeBuf,
                                     l,
                                     NULL);

        if(!NT_SUCCESS(Status))
        {
          goto exitWithStatus;
        }

        /* initialize Unicode string buffer */
        UnicodeString.Length = UnicodeString.MaximumLength = l;
        UnicodeString.Buffer = pwcUnicodeBuf;

        /* convert and copy */
        if(AreFileApisANSI())
          RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
        else
          RtlUnicodeStringToOemString(&AnsiString, &UnicodeString, FALSE);

        /* return the string size */
        Context->nBufSize = AnsiString.Length;
#if 0
        }
        __finally
        {
          /* free the buffer */
          PsaiFree(pwcUnicodeBuf);
        }
#else
        Status = STATUS_NO_MORE_FILES;

exitWithStatus:
        /* free the buffer */
        PsaiFree(pwcUnicodeBuf);
        return Status;
#endif
      }
    }
    else
    {
      /* we want other module information */
      ULONG nSize = Context->nBufSize;

      /* base address */
      if(nSize >= sizeof(CurrentModule->BaseAddress))
      {
        Context->lpmodinfo->lpBaseOfDll = CurrentModule->BaseAddress;
        nSize -= sizeof(CurrentModule->BaseAddress);
      }

      /* image size */
      if(nSize >= sizeof(CurrentModule->SizeOfImage))
      {
        Context->lpmodinfo->SizeOfImage = CurrentModule->SizeOfImage;
        nSize -= sizeof(CurrentModule->SizeOfImage);
      }

      /* entry point */
      if(nSize >= sizeof(CurrentModule->EntryPoint))
      {
        /* ??? FIXME? is "EntryPoint" just the offset, or the real address? */
        Context->lpmodinfo->EntryPoint = (PVOID)CurrentModule->EntryPoint;
      }

      Context->nBufSize = TRUE;
    }

    return STATUS_NO_MORE_FILES;
  }

  return STATUS_SUCCESS;
}


static DWORD
InternalGetModuleInformation(HANDLE hProcess,
                             HMODULE hModule,
                             GET_MODULE_INFORMATION_FLAGS Flags,
                             LPVOID lpBuffer,
                             DWORD nBufSize)
{
  GET_MODULE_INFORMATION_CONTEXT Context;
  NTSTATUS Status;

  Context.hModule = hModule;
  Context.Flags = Flags;
  Context.nBufSize = nBufSize;
  Context.lpBuffer = lpBuffer;

  Status = PsaEnumerateProcessModules(hProcess, GetModuleInformationCallback, &Context);

  if(Status == STATUS_NO_MORE_FILES)
  {
    /* module was found, return string size */
    return Context.nBufSize;
  }
  else if(NT_SUCCESS(Status))
  {
    /* module was not found */
    SetLastError(ERROR_INVALID_HANDLE);
  }
  else
  {
    /* an error occurred */
    SetLastErrorByStatus(Status);
  }
  return 0;
}

/* PUBLIC *********************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
EmptyWorkingSet(HANDLE hProcess)
{
  QUOTA_LIMITS QuotaLimits;
  NTSTATUS Status;

  /* query the working set */
  Status = NtQueryInformationProcess(hProcess,
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QuotaLimits),
                                     NULL);

  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  /* empty the working set */
  QuotaLimits.MinimumWorkingSetSize = -1;
  QuotaLimits.MaximumWorkingSetSize = -1;

  /* set the working set */
  Status = NtSetInformationProcess(hProcess,
                                   ProcessQuotaLimits,
                                   &QuotaLimits,
                                   sizeof(QuotaLimits));
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDeviceDrivers(LPVOID *lpImageBase,
                  DWORD cb,
                  LPDWORD lpcbNeeded)
{
  ENUM_DEVICE_DRIVERS_CONTEXT Context;
  NTSTATUS Status;
 
  if(cb == 0 || lpImageBase == NULL)
  {
    *lpcbNeeded = 0;
    return TRUE;
  }
 
  cb /= sizeof(PVOID);

  Context.lpImageBase = lpImageBase;
  Context.nCount = cb;

  Status = PsaEnumerateSystemModules(EnumDeviceDriversCallback, &Context);

  /* return the count of bytes returned */
  *lpcbNeeded = (cb - Context.nCount) * sizeof(PVOID);

  if(!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumProcesses(DWORD *lpidProcess,
              DWORD cb,
              LPDWORD lpcbNeeded)
{
  ENUM_PROCESSES_CONTEXT Context;
  NTSTATUS Status;
  
  cb /= sizeof(DWORD);
  
  if(cb == 0 || lpidProcess == NULL)
  {
    *lpcbNeeded = 0;
    return TRUE;
  }
  
  Context.lpidProcess = lpidProcess;
  Context.nCount = cb;

  /* enumerate the process ids */
  Status = PsaEnumerateProcesses(EnumProcessesCallback, &Context);

  *lpcbNeeded = (cb - Context.nCount) * sizeof(DWORD);

  if(!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumProcessModules(HANDLE hProcess,
                   HMODULE *lphModule,
                   DWORD cb,
                   LPDWORD lpcbNeeded)
{
  ENUM_PROCESS_MODULES_CONTEXT Context;
  NTSTATUS Status;
  
  cb /= sizeof(HMODULE);
  
  if(cb == 0 || lphModule == NULL)
  {
    *lpcbNeeded = 0;
    return TRUE;
  }

  Context.lphModule = lphModule;
  Context.nCount = cb;

  /* enumerate the process modules */
  Status = PsaEnumerateProcessModules(hProcess, EnumProcessModulesCallback, &Context);

  *lpcbNeeded = (cb - Context.nCount) * sizeof(DWORD);

  if(!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  return TRUE;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetDeviceDriverBaseNameA(LPVOID ImageBase,
                         LPSTR lpBaseName,
                         DWORD nSize)
{
  return InternalGetDeviceDriverName(FALSE, FALSE, ImageBase, lpBaseName, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetDeviceDriverFileNameA(LPVOID ImageBase,
                         LPSTR lpFilename,
                         DWORD nSize)
{
  return InternalGetDeviceDriverName(FALSE, TRUE, ImageBase, lpFilename, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetDeviceDriverBaseNameW(LPVOID ImageBase,
                         LPWSTR lpBaseName,
                         DWORD nSize)
{
  return InternalGetDeviceDriverName(TRUE, FALSE, ImageBase, lpBaseName, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetDeviceDriverFileNameW(LPVOID ImageBase,
                         LPWSTR lpFilename,
                         DWORD nSize)
{
  return InternalGetDeviceDriverName(TRUE, TRUE, ImageBase, lpFilename, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetMappedFileNameA(HANDLE hProcess,
                   LPVOID lpv,
                   LPSTR lpFilename,
                   DWORD nSize)
{
  return InternalGetMappedFileName(FALSE, hProcess, lpv, lpFilename, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetMappedFileNameW(HANDLE hProcess,
                   LPVOID lpv,
                   LPWSTR lpFilename,
                   DWORD nSize)
{
  return InternalGetMappedFileName(TRUE, hProcess, lpv, lpFilename, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetModuleBaseNameA(HANDLE hProcess,
                   HMODULE hModule,
                   LPSTR lpBaseName,
                   DWORD nSize)
{
  GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, FALSE, FALSE};
  return InternalGetModuleInformation(hProcess, hModule, Flags, lpBaseName, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetModuleBaseNameW(HANDLE hProcess,
                   HMODULE hModule,
                   LPWSTR lpBaseName,
                   DWORD nSize)
{
  GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, TRUE, FALSE};
  return InternalGetModuleInformation(hProcess, hModule, Flags, lpBaseName, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetModuleFileNameExA(HANDLE hProcess,
                     HMODULE hModule,
                     LPSTR lpFilename,
                     DWORD nSize)
{
  GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, FALSE, TRUE};
  return InternalGetModuleInformation(hProcess, hModule, Flags, lpFilename, nSize);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetModuleFileNameExW(HANDLE hProcess,
                     HMODULE hModule,
                     LPWSTR lpFilename,
                     DWORD nSize)
{
  GET_MODULE_INFORMATION_FLAGS Flags = {TRUE, TRUE, TRUE};
  return InternalGetModuleInformation(hProcess, hModule, Flags, lpFilename, nSize);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetModuleInformation(HANDLE hProcess,
                     HMODULE hModule,
                     LPMODULEINFO lpmodinfo,
                     DWORD cb)
{
  GET_MODULE_INFORMATION_FLAGS Flags = {FALSE, FALSE, FALSE};
  return (BOOL)InternalGetModuleInformation(hProcess, hModule, Flags, lpmodinfo, cb);
}


/*
 * @implemented
 */
BOOL
STDCALL
InitializeProcessForWsWatch(HANDLE hProcess)
{
  NTSTATUS Status;

  Status = NtSetInformationProcess(hProcess,
                                   ProcessWorkingSetWatch,
                                   NULL,
                                   0);
  if(!NT_SUCCESS(Status))
  {
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetWsChanges(HANDLE hProcess,
             PPSAPI_WS_WATCH_INFORMATION lpWatchInfo,
             DWORD cb)
{
  NTSTATUS Status;

  Status = NtQueryInformationProcess(hProcess,
                                     ProcessWorkingSetWatch,
                                     (PVOID)lpWatchInfo,
                                     cb,
                                     NULL);
  if(!NT_SUCCESS(Status))
  {
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
  }

  return TRUE;
}

/* EOF */

