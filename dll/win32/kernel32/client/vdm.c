/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/vdm.c
 * PURPOSE:         Virtual Dos Machine (VDM) Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* TYPES **********************************************************************/

typedef struct _ENV_INFO
{
    ULONG NameType;
    ULONG NameLength;
    PWCHAR Name;
} ENV_INFO, *PENV_INFO;

/* GLOBALS ********************************************************************/

ENV_INFO BasepEnvNameType[] =
{
    {3, sizeof(L"PATH"), L"PATH"},
    {2, sizeof(L"WINDIR"), L"WINDIR"},
    {2, sizeof(L"SYSTEMROOT"), L"SYSTEMROOT"},
    {3, sizeof(L"TEMP"), L"TEMP"},
    {3, sizeof(L"TMP"), L"TMP"},
};

UNICODE_STRING BaseDotComSuffixName = RTL_CONSTANT_STRING(L".com");
UNICODE_STRING BaseDotPifSuffixName = RTL_CONSTANT_STRING(L".pif");
UNICODE_STRING BaseDotExeSuffixName = RTL_CONSTANT_STRING(L".exe");

/* FUNCTIONS ******************************************************************/

ULONG
WINAPI
BaseIsDosApplication(IN PUNICODE_STRING PathName,
                     IN NTSTATUS Status)
{
    UNICODE_STRING String;

    /* Is it a .com? */
    String.Length = BaseDotComSuffixName.Length;
    String.Buffer = &PathName->Buffer[(PathName->Length - String.Length) / sizeof(WCHAR)];
    if (RtlEqualUnicodeString(&String, &BaseDotComSuffixName, TRUE)) return 2;

    /* Is it a .pif? */
    String.Length = BaseDotPifSuffixName.Length;
    String.Buffer = &PathName->Buffer[(PathName->Length - String.Length) / sizeof(WCHAR)];
    if (RtlEqualUnicodeString(&String, &BaseDotPifSuffixName, TRUE)) return 3;

    /* Is it an exe? */
    String.Length = BaseDotExeSuffixName.Length;
    String.Buffer = &PathName->Buffer[(PathName->Length - String.Length) / sizeof(WCHAR)];
    if (RtlEqualUnicodeString(&String, &BaseDotExeSuffixName, TRUE)) return 1;
    return 0;
}

BOOL
WINAPI
BaseCheckVDM(IN ULONG BinaryType,
             IN PCWCH ApplicationName,
             IN PCWCH CommandLine,
             IN PCWCH CurrentDirectory,
             IN PANSI_STRING AnsiEnvironment,
             IN PCSR_API_MESSAGE Msg,
             IN OUT PULONG iTask,
             IN DWORD CreationFlags,
             IN LPSTARTUPINFOW StartupInfo)
{
    /* This is not supported */
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
BaseUpdateVDMEntry(IN ULONG UpdateIndex,
                   IN OUT PHANDLE WaitHandle,
                   IN ULONG IndexInfo,
                   IN ULONG BinaryType)
{
    NTSTATUS Status;
    CSR_API_MESSAGE Msg;
    ULONG CsrRequest = MAKE_CSR_API(UPDATE_VDM_ENTRY, CSR_CONSOLE);

    /* Check what update is being sent */
    switch (UpdateIndex)
    {
        /* VDM is being undone */
        case VdmEntryUndo:

            /* Tell the server how far we had gotten along */
            Msg.Data.UpdateVdmEntry.iTask = (ULONG)*WaitHandle;
            Msg.Data.UpdateVdmEntry.VDMCreationState = IndexInfo;
            break;

        /* VDM is ready with a new process handle */
        case VdmEntryUpdateProcess:

            /* Send it the process handle */
            Msg.Data.UpdateVdmEntry.VDMProcessHandle = *WaitHandle;
            Msg.Data.UpdateVdmEntry.iTask = IndexInfo;
            break;
    }

    /* Also check what kind of binary this is for the console handle */
    if (BinaryType == BINARY_TYPE_WOW)
    {
        /* Magic value for 16-bit apps */
        Msg.Data.UpdateVdmEntry.ConsoleHandle = (HANDLE)-1;
    }
    else if (Msg.Data.UpdateVdmEntry.iTask)
    {
        /* No handle for true VDM */
        Msg.Data.UpdateVdmEntry.ConsoleHandle = 0;
    }
    else
    {
        /* Otherwise, send the regular consoel handle */
        Msg.Data.UpdateVdmEntry.ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    }

    /* Finally write the index and binary type */
    Msg.Data.UpdateVdmEntry.EntryIndex = UpdateIndex;
    Msg.Data.UpdateVdmEntry.BinaryType = BinaryType;

    /* Send the message to CSRSS */
    Status = CsrClientCallServer(&Msg, NULL, CsrRequest, sizeof(Msg));
    if (!(NT_SUCCESS(Status)) || !(NT_SUCCESS(Msg.Status)))
    {
        /* Handle failure */
        BaseSetLastNTError(Msg.Status);
        return FALSE;
    }

    /* If this was an update, CSRSS returns a new wait handle */
    if (UpdateIndex == VdmEntryUpdateProcess)
    {
        /* Return it to the caller */
        *WaitHandle = Msg.Data.UpdateVdmEntry.WaitObjectForParent;
    }

    /* We made it */
    return TRUE;
}

BOOL
WINAPI
BaseCheckForVDM(IN HANDLE ProcessHandle,
                OUT LPDWORD ExitCode)
{
    NTSTATUS Status;
    EVENT_BASIC_INFORMATION EventBasicInfo;
    CSR_API_MESSAGE Msg;
    ULONG CsrRequest = MAKE_CSR_API(GET_VDM_EXIT_CODE, CSR_CONSOLE);

    /* It's VDM if the process is actually a wait handle (an event) */
    Status = NtQueryEvent(ProcessHandle,
                          EventBasicInformation,
                          &EventBasicInfo,
                          sizeof(EventBasicInfo),
                          NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Setup the input parameters */
    Msg.Data.GetVdmExitCode.ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    Msg.Data.GetVdmExitCode.hParent = ProcessHandle;

    /* Call CSRSS */
    Status = CsrClientCallServer(&Msg, NULL, CsrRequest, sizeof(Msg));
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get the exit code from the reply */
    *ExitCode = Msg.Data.GetVdmExitCode.ExitCode;
    return TRUE;
}

BOOL
WINAPI
BaseGetVdmConfigInfo(IN LPCWSTR Reserved,
                     IN ULONG DosSeqId,
                     IN ULONG BinaryType,
                     IN PUNICODE_STRING CmdLineString,
                     OUT PULONG VdmSize)
{
    WCHAR Buffer[MAX_PATH];
    WCHAR CommandLine[MAX_PATH * 2];
    ULONG Length;

    /* Clear the buffer in case we fail */
    CmdLineString->Buffer = 0;

    /* Always return the same size */
    *VdmSize = 0x1000000;

    /* Get the system directory */
    Length = GetSystemDirectoryW(Buffer, MAX_PATH);
    if (!(Length) || (Length >= MAX_PATH))
    {
        /* Eliminate no path or path too big */
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* Check if this is VDM with a DOS Sequence ID */
    if (DosSeqId)
    {
        /* Build the VDM string for it */
        _snwprintf(CommandLine,
                   sizeof(CommandLine),
                   L"\"%s\\ntvdm.exe\" -i%lx %s%c",
                   Buffer,
                   DosSeqId,
                   (BinaryType == 0x10) ? L" " : L"-w",
                   (BinaryType == 0x40) ? 's' : ' ');
    }
    else
    {
        /* Non-DOS, build the stirng for it without the task ID */
        _snwprintf(CommandLine,
                   sizeof(CommandLine),
                   L"\"%s\\ntvdm.exe\"  %s%c",
                   Buffer,
                   (BinaryType == 0x10) ? L" " : L"-w",
                   (BinaryType == 0x40) ? 's' : ' ');
    }

    /* Create the actual string */
    return RtlCreateUnicodeString(CmdLineString, CommandLine);
}

UINT
WINAPI
BaseGetEnvNameType_U(IN PWCHAR Name,
                     IN ULONG NameLength)
{
    PENV_INFO EnvInfo;
    ULONG NameType, i;

    /* Start by assuming unknown type */
    NameType = 1;

    /* Loop all the environment names */
    for (i = 0; i < (sizeof(BasepEnvNameType) / sizeof(ENV_INFO)); i++)
    {
        /* Get this entry */
        EnvInfo = &BasepEnvNameType[i];

        /* Check if it matches the name */
        if ((EnvInfo->NameLength == NameLength) &&
            !(_wcsnicmp(EnvInfo->Name, Name, NameLength)))
        {
            /* It does, return the type */
            NameType = EnvInfo->NameType;
            break;
        }
    }

    /* Return what we found, or unknown if nothing */
    return NameType;
}

BOOL
NTAPI
BaseDestroyVDMEnvironment(IN PANSI_STRING AnsiEnv,
                          IN PUNICODE_STRING UnicodeEnv)
{
    ULONG Dummy = 0;

    /* Clear the ASCII buffer since Rtl creates this for us */
    if (AnsiEnv->Buffer) RtlFreeAnsiString(AnsiEnv);

    /* The Unicode buffer is build by hand, though */
    if (UnicodeEnv->Buffer)
    {
        /* So clear it through the API */
        NtFreeVirtualMemory(NtCurrentProcess(),
                            (PVOID*)&UnicodeEnv->Buffer,
                            &Dummy,
                            MEM_RELEASE);
    }

    /* All done */
    return TRUE;
}

BOOL
NTAPI
BaseCreateVDMEnvironment(IN PWCHAR lpEnvironment,
                         IN PANSI_STRING AnsiEnv,
                         IN PUNICODE_STRING UnicodeEnv)
{
    BOOL Result;
    ULONG RegionSize, EnvironmentSize = 0;
    PWCHAR p, Environment, NewEnvironment;
    NTSTATUS Status;

    /* Make sure we have both strings */
    if (!(AnsiEnv) || !(UnicodeEnv))
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if an environment was passed in */
    if (!lpEnvironment)
    {
        /* Nope, create one */
        Status = RtlCreateEnvironment(TRUE, (PWCHAR*)&Environment);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }
    else
    {
        /* Use the one we got */
        Environment = lpEnvironment;
    }

    /* Do we have something now ? */
    if (!Environment)
    {
        /* Still not, fail out */
        SetLastError(ERROR_BAD_ENVIRONMENT);
        goto Quickie;
    }

    /* Count how much space the whole environment takes */
    p = Environment;
    while ((*p++ != UNICODE_NULL) && (*p != UNICODE_NULL)) EnvironmentSize++;
    EnvironmentSize += sizeof(UNICODE_NULL);

    /* Allocate a new copy */
    RegionSize = (EnvironmentSize + MAX_PATH) * sizeof(WCHAR);
    if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(),
                                            (PVOID*)&NewEnvironment,
                                            0,
                                            &RegionSize,
                                            MEM_COMMIT,
                                            PAGE_READWRITE)))
    {
        /* We failed, bail out */
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        NewEnvironment = NULL;
        goto Quickie;
    }

    /* Begin parsing the new environment */
    p = NewEnvironment;

    /* FIXME: Code here */

    /* Terminate it */
    *p++ = UNICODE_NULL;

    /* Initialize the unicode string to hold it */
    EnvironmentSize = (p - NewEnvironment) * sizeof(WCHAR);
    RtlInitEmptyUnicodeString(UnicodeEnv, NewEnvironment, EnvironmentSize);
    UnicodeEnv->Length = EnvironmentSize;

    /* Create the ASCII version of it */
    Status = RtlUnicodeStringToAnsiString(AnsiEnv, UnicodeEnv, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Set last error if conversion failure */
        BaseSetLastNTError(Status);
    }
    else
    {
        /* Everything went okay, so return success */
        Result = TRUE;
        NewEnvironment = NULL;
    }

Quickie:
    /* Cleanup path starts here, start by destroying the envrionment copy */
    if (!(lpEnvironment) && (Environment)) RtlDestroyEnvironment(Environment);

    /* See if we are here due to failure */
    if (NewEnvironment)
    {
        /* Initialize the paths to be empty */
        RtlInitEmptyUnicodeString(UnicodeEnv, NULL, 0);
        RtlInitEmptyAnsiString(AnsiEnv, NULL, 0);

        /* Free the environment copy */
        RegionSize = 0;
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&NewEnvironment,
                                     &RegionSize,
                                     MEM_RELEASE);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Return the result */
    return Result;
}


/* Check whether a file is an OS/2 or a very old Windows executable
 * by testing on import of KERNEL.
 *
 * FIXME: is reading the module imports the only way of discerning
 *        old Windows binaries from OS/2 ones ? At least it seems so...
 */
static DWORD WINAPI
InternalIsOS2OrOldWin(HANDLE hFile, IMAGE_DOS_HEADER *mz, IMAGE_OS2_HEADER *ne)
{
  DWORD CurPos;
  LPWORD modtab = NULL;
  LPSTR nametab = NULL;
  DWORD Read, Ret;
  int i;

  Ret = BINARY_OS216;
  CurPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

  /* read modref table */
  if((SetFilePointer(hFile, mz->e_lfanew + ne->ne_modtab, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
     (!(modtab = HeapAlloc(GetProcessHeap(), 0, ne->ne_cmod * sizeof(WORD)))) ||
     (!(ReadFile(hFile, modtab, ne->ne_cmod * sizeof(WORD), &Read, NULL))) ||
     (Read != (DWORD)ne->ne_cmod * sizeof(WORD)))
  {
    goto broken;
  }

  /* read imported names table */
  if((SetFilePointer(hFile, mz->e_lfanew + ne->ne_imptab, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
     (!(nametab = HeapAlloc(GetProcessHeap(), 0, ne->ne_enttab - ne->ne_imptab))) ||
     (!(ReadFile(hFile, nametab, ne->ne_enttab - ne->ne_imptab, &Read, NULL))) ||
     (Read != (DWORD)ne->ne_enttab - ne->ne_imptab))
  {
    goto broken;
  }

  for(i = 0; i < ne->ne_cmod; i++)
  {
    LPSTR module;
    module = &nametab[modtab[i]];
    if(!strncmp(&module[1], "KERNEL", module[0]))
    {
      /* very old windows file */
      Ret = BINARY_WIN16;
      goto done;
    }
  }

  broken:
  DPRINT1("InternalIsOS2OrOldWin(): Binary file seems to be broken\n");

  done:
  HeapFree(GetProcessHeap(), 0, modtab);
  HeapFree(GetProcessHeap(), 0, nametab);
  SetFilePointer(hFile, CurPos, NULL, FILE_BEGIN);
  return Ret;
}

static DWORD WINAPI
InternalGetBinaryType(HANDLE hFile)
{
  union
  {
    struct
    {
      unsigned char magic[4];
      unsigned char ignored[12];
      unsigned short type;
    } elf;
    struct
    {
      unsigned long magic;
      unsigned long cputype;
      unsigned long cpusubtype;
      unsigned long filetype;
    } macho;
    IMAGE_DOS_HEADER mz;
  } Header;
  char magic[4];
  DWORD Read;

  if((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
     (!ReadFile(hFile, &Header, sizeof(Header), &Read, NULL) ||
      (Read != sizeof(Header))))
  {
    return BINARY_UNKNOWN;
  }

  if(!memcmp(Header.elf.magic, "\177ELF", sizeof(Header.elf.magic)))
  {
    /* FIXME: we don't bother to check byte order, architecture, etc. */
    switch(Header.elf.type)
    {
      case 2:
        return BINARY_UNIX_EXE;
      case 3:
        return BINARY_UNIX_LIB;
    }
    return BINARY_UNKNOWN;
  }

  /* Mach-o File with Endian set to Big Endian  or Little Endian*/
  if(Header.macho.magic == 0xFEEDFACE ||
     Header.macho.magic == 0xCEFAEDFE)
  {
    switch(Header.macho.filetype)
    {
      case 0x8:
        /* MH_BUNDLE */
        return BINARY_UNIX_LIB;
    }
    return BINARY_UNKNOWN;
  }

  /* Not ELF, try DOS */
  if(Header.mz.e_magic == IMAGE_DOS_SIGNATURE)
  {
    /* We do have a DOS image so we will now try to seek into
     * the file by the amount indicated by the field
     * "Offset to extended header" and read in the
     * "magic" field information at that location.
     * This will tell us if there is more header information
     * to read or not.
     */
    if((SetFilePointer(hFile, Header.mz.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
       (!ReadFile(hFile, magic, sizeof(magic), &Read, NULL) ||
        (Read != sizeof(magic))))
    {
      return BINARY_DOS;
    }

    /* Reading the magic field succeeded so
     * we will try to determine what type it is.
     */
    if(!memcmp(magic, "PE\0\0", sizeof(magic)))
    {
      IMAGE_FILE_HEADER FileHeader;
      if(!ReadFile(hFile, &FileHeader, sizeof(IMAGE_FILE_HEADER), &Read, NULL) ||
         (Read != sizeof(IMAGE_FILE_HEADER)))
      {
        return BINARY_DOS;
      }

      /* FIXME - detect 32/64 bit */

      if(FileHeader.Characteristics & IMAGE_FILE_DLL)
        return BINARY_PE_DLL32;
      return BINARY_PE_EXE32;
    }

    if(!memcmp(magic, "NE", 1))
    {
      /* This is a Windows executable (NE) header.  This can
       * mean either a 16-bit OS/2 or a 16-bit Windows or even a
       * DOS program (running under a DOS extender).  To decide
       * which, we'll have to read the NE header.
       */
      IMAGE_OS2_HEADER ne;
      if((SetFilePointer(hFile, Header.mz.e_lfanew, NULL, FILE_BEGIN) == 1) ||
         !ReadFile(hFile, &ne, sizeof(IMAGE_OS2_HEADER), &Read, NULL) ||
         (Read != sizeof(IMAGE_OS2_HEADER)))
      {
        /* Couldn't read header, so abort. */
        return BINARY_DOS;
      }

      switch(ne.ne_exetyp)
      {
        case 2:
          return BINARY_WIN16;
        case 5:
          return BINARY_DOS;
        default:
          return InternalIsOS2OrOldWin(hFile, &Header.mz, &ne);
      }
    }
    return BINARY_DOS;
  }
  return BINARY_UNKNOWN;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetBinaryTypeW (
    LPCWSTR lpApplicationName,
    LPDWORD lpBinaryType
    )
{
  HANDLE hFile;
  DWORD BinType;

  if(!lpApplicationName || !lpBinaryType)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  hFile = CreateFileW(lpApplicationName, GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, 0, 0);
  if(hFile == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  BinType = InternalGetBinaryType(hFile);
  CloseHandle(hFile);

  switch(BinType)
  {
    case BINARY_UNKNOWN:
    {
      WCHAR *dot;

      /*
       * guess from filename
       */
      if(!(dot = wcsrchr(lpApplicationName, L'.')))
      {
        return FALSE;
      }
      if(!lstrcmpiW(dot, L".COM"))
      {
        *lpBinaryType = SCS_DOS_BINARY;
        return TRUE;
      }
      if(!lstrcmpiW(dot, L".PIF"))
      {
        *lpBinaryType = SCS_PIF_BINARY;
        return TRUE;
      }
      return FALSE;
    }
    case BINARY_PE_EXE32:
    case BINARY_PE_DLL32:
    {
      *lpBinaryType = SCS_32BIT_BINARY;
      return TRUE;
    }
    case BINARY_PE_EXE64:
    case BINARY_PE_DLL64:
    {
      *lpBinaryType = SCS_64BIT_BINARY;
      return TRUE;
    }
    case BINARY_WIN16:
    {
      *lpBinaryType = SCS_WOW_BINARY;
      return TRUE;
    }
    case BINARY_OS216:
    {
      *lpBinaryType = SCS_OS216_BINARY;
      return TRUE;
    }
    case BINARY_DOS:
    {
      *lpBinaryType = SCS_DOS_BINARY;
      return TRUE;
    }
    case BINARY_UNIX_EXE:
    case BINARY_UNIX_LIB:
    {
      return FALSE;
    }
  }

  DPRINT1("Invalid binary type returned!\n", BinType);
  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetBinaryTypeA(IN LPCSTR lpApplicationName,
               OUT LPDWORD lpBinaryType)
{
    ANSI_STRING ApplicationNameString;
    UNICODE_STRING ApplicationNameW;
    BOOL StringAllocated = FALSE, Result;
    NTSTATUS Status;

    RtlInitAnsiString(&ApplicationNameString, lpApplicationName);

    if (ApplicationNameString.Length * sizeof(WCHAR) >= NtCurrentTeb()->StaticUnicodeString.MaximumLength)
    {
        StringAllocated = TRUE;
        Status = RtlAnsiStringToUnicodeString(&ApplicationNameW, &ApplicationNameString, TRUE);
    }
    else
    {
        Status = RtlAnsiStringToUnicodeString(&(NtCurrentTeb()->StaticUnicodeString), &ApplicationNameString, FALSE);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (StringAllocated)
    {
        Result = GetBinaryTypeW(ApplicationNameW.Buffer, lpBinaryType);
        RtlFreeUnicodeString(&ApplicationNameW);
    }
    else
    {
        Result = GetBinaryTypeW(NtCurrentTeb()->StaticUnicodeString.Buffer, lpBinaryType);
    }

    return Result;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CmdBatNotification (
    DWORD   Unknown
    )
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
ExitVDM (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetNextVDMCommand (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterConsoleVDM (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8,
    DWORD   Unknown9,
    DWORD   Unknown10
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterWowBaseHandlers (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterWowExec (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
VDMOperationStarted (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}
