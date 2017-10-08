////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifdef NT_NATIVE_MODE

#include "regtools.h"
#include <stdarg.h>

/*typedef BOOLEAN (*PPsGetVersion) (
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );

//PPsGetVersion _PsGetVersion = PsGetVersion;

/*NTSTATUS
KernelGetProcAddress(
    PWCHAR DllName,
    PUCHAR ProcName,
    PVOID* ProcAddr
    )
{
  NTSTATUS RC;
  HANDLE h;
  UNICODE_STRING uname;
  ANSI_STRING aname;

  RtlInitUnicodeString(&uname, DllName);
  *ProcAddr = NULL;

 // RC = LdrGetDllHandle(NULL, NULL, &uname, &h);
  if(!NT_SUCCESS(RC))
    return RC;

  RtlInitAnsiString(&aname, ProcName);
  
//  RC = LdrGetProcedureAddress(h, &aname, 0, ProcAddr);
  return RC;
} */


BOOLEAN
GetOsVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    )
{
  WCHAR Str[32];
  ULONG mn=0, mj=0, bld=0;

//  if(_PsGetVersion)
//    return _PsGetVersion(MajorVersion, MinorVersion, BuildNumber, CSDVersion);

  RtlZeroMemory(Str, sizeof(Str));
  if(RegTGetStringValue(NULL, L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                   L"CurrentVersion",
                                   &Str[0], sizeof(Str)-sizeof(WCHAR))) {
    ULONG i=0;
    WCHAR a;
    while(a = Str[i]) {
      if(a == '.')
        break;
      if(a < '0' || a > '9')
        break;
      mj = mj*16 + (a-'0');
      i++;
    }
    i++;
    while(a = Str[i]) {
      if(a == '.')
        break;
      if(a < '0' || a > '9')
        break;
      mn = mn*16 + (a-'0');
      i++;
    }
  }

  if(RegTGetStringValue(NULL, L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                   L"CurrentBuildNumber",
                                   &Str[0], sizeof(Str)-sizeof(WCHAR))) {
    ULONG i=0;
    WCHAR a;
    while(a = Str[i]) {
      if(a < '0' || a > '9')
        break;
      bld = bld*10 + (a-'0');
      i++;
    }
  }
  if(MajorVersion)
    *MajorVersion = mj;
  if(MinorVersion)
    *MinorVersion = mn;
  if(BuildNumber)
    *BuildNumber = bld;
  return TRUE;
}

BOOLEAN
MyDeviceIoControl(
    HANDLE h,
    DWORD  dwIoControlCode,
    PVOID  lpInBuffer,
    DWORD  nInBufferSize,
    PVOID  lpOutBuffer,
    DWORD  nOutBufferSize,
    DWORD* lpBytesReturned,
    PVOID  lpOverlapped
    )
{

    NTSTATUS RC;
    BOOLEAN DevIoCtl = TRUE;
    IO_STATUS_BLOCK Iosb;

    if ( dwIoControlCode >> 16 == FILE_DEVICE_FILE_SYSTEM ) {
        DevIoCtl = FALSE;
    } else {
        DevIoCtl = TRUE;
    }

    if ( DevIoCtl ) {
        RC = NtDeviceIoControlFile(
                    h,
                    NULL,
                    NULL,             // APC routine
                    NULL,             // APC Context
                    &Iosb,
                    dwIoControlCode,  // IoControlCode
                    lpInBuffer,       // Buffer for data to the FS
                    nInBufferSize,
                    lpOutBuffer,      // OutputBuffer for data from the FS
                    nOutBufferSize    // OutputBuffer Length
                    );
    } else {
        RC = NtFsControlFile(
                    h,
                    NULL,
                    NULL,             // APC routine
                    NULL,             // APC Context
                    &Iosb,
                    dwIoControlCode,  // IoControlCode
                    lpInBuffer,       // Buffer for data to the FS
                    nInBufferSize,
                    lpOutBuffer,      // OutputBuffer for data from the FS
                    nOutBufferSize    // OutputBuffer Length
                    );
    }

    if ( RC == STATUS_PENDING) {
        // Operation must complete before return & Iosb destroyed
        RC = NtWaitForSingleObject( h, FALSE, NULL );
        if ( NT_SUCCESS(RC)) {
            RC = Iosb.Status;
        }
    }

    if ( NT_SUCCESS(RC) ) {
        *lpBytesReturned = Iosb.Information;
        return TRUE;
    } else {
        // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(RC) ) {
            *lpBytesReturned = Iosb.Information;
        }
        return FALSE;
    }
}

VOID
Sleep(
    ULONG t
    )
{
    LARGE_INTEGER delay = {0,0};
    delay.QuadPart = -10I64*1000*t;
    NtDelayExecution(FALSE, &delay);
}

HANDLE hGlobalHeap = NULL;

extern "C"
PVOID
MyGlobalAlloc(
    ULONG Size
    )
{
    if(!hGlobalHeap) {
        // Initialize some heap
        hGlobalHeap = RtlCreateHeap( HEAP_GROWABLE,    // Flags
                                         NULL,              // HeapBase
                                         0,                 // ReserveSize
                                         0,                 // CommitSize
                                         NULL,              // Lock
                                         NULL );            // Parameters
        if(!hGlobalHeap || hGlobalHeap == (HANDLE)(-1)) {
            hGlobalHeap = NULL;
            return NULL;
        }
    }
    return RtlAllocateHeap( hGlobalHeap, 0, Size );
}

extern "C"
VOID
MyGlobalFree(
    PVOID Addr
    )
{
    if(!hGlobalHeap) {
//        BrutePoint();
        return;
    }
    RtlFreeHeap( hGlobalHeap, 0, Addr );
    return;
}

CHAR dbg_print_tmp_buff[2048];
WCHAR dbg_stringBuffer[2048];

BOOLEAN was_enter = TRUE;

extern "C"
VOID
PrintNtConsole(
    PCHAR DebugMessage,
    ...
    )
{
    int len;
    UNICODE_STRING msgBuff;
    va_list ap;
    va_start(ap, DebugMessage);

    if(was_enter) {
        strcpy(&dbg_print_tmp_buff[0], NT_DBG_PREFIX);
        len = _vsnprintf(&dbg_print_tmp_buff[sizeof(NT_DBG_PREFIX)-1], 2047-sizeof(NT_DBG_PREFIX), DebugMessage, ap);
    } else {
        len = _vsnprintf(&dbg_print_tmp_buff[0], 2047, DebugMessage, ap);
    }
    dbg_print_tmp_buff[2047] = 0;
    if(len > 0 &&
       (dbg_print_tmp_buff[len-1] == '\n' ||
        dbg_print_tmp_buff[len-1] == '\r') ) {
        was_enter = TRUE;
    } else {
        was_enter = FALSE;
    }

    len = swprintf( dbg_stringBuffer, L"%S", dbg_print_tmp_buff );
    msgBuff.Buffer = dbg_stringBuffer;
    msgBuff.Length = len * sizeof(WCHAR);
    msgBuff.MaximumLength = msgBuff.Length + sizeof(WCHAR);
    NtDisplayString( &msgBuff );

    va_end(ap);

} // end PrintNtConsole()

extern "C"
NTSTATUS
EnvFileOpenW(
    PWCHAR Name,
    HANDLE* ph
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS Status;
    UNICODE_STRING fName;

    RtlInitUnicodeString(&fName, Name);

    InitializeObjectAttributes(&ObjectAttributes, &fName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateFile(ph,
                             GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatus,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_COMPLETE_IF_OPLOCKED /*| FILE_WRITE_THROUGH*/,
                             NULL,
                             0);

    return Status;
} // end EnvFileOpenW()

extern "C"
NTSTATUS
EnvFileOpenA(
    PCHAR Name,
    HANDLE* ph
    )
{
    ULONG len;
    PWCHAR NameW;
    NTSTATUS Status;

    len = strlen(Name);

    NameW = (PWCHAR)MyAllocatePool__(NonPagedPool, (len+1)*sizeof(WCHAR));
    if(!NameW)
        return STATUS_INSUFFICIENT_RESOURCES;

    swprintf(NameW, L"%S", Name);

    Status = EnvFileOpenW(NameW, ph);

    MyFreePool__(NameW);

    return Status;
} // end EnvFileOpenA()

extern "C"
NTSTATUS
EnvFileClose(
    HANDLE hFile
    )
{
    return NtClose(hFile);
} // end EnvFileClose()

extern "C"
NTSTATUS
EnvFileGetSizeByHandle(
    HANDLE hFile,
    PLONGLONG lpFileSize
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    Status = NtQueryInformationFile(
                hFile,
                &IoStatusBlock,
                &StandardInfo,
                sizeof(StandardInfo),
                FileStandardInformation
                );
    if (NT_SUCCESS(Status)) {
        *lpFileSize = StandardInfo.EndOfFile.QuadPart;
    }
    return Status;
} // end EnvFileGetSizeByHandle()

extern "C"
NTSTATUS
EnvFileGetSizeA(
    PCHAR Name,
    PLONGLONG lpFileSize
    )
{
    NTSTATUS Status;
    HANDLE hFile;

    (*lpFileSize) = -1I64;

    Status = EnvFileOpenA(Name, &hFile);

    if(!NT_SUCCESS(Status))
        return Status;

    Status = EnvFileGetSizeByHandle(hFile, lpFileSize);

    NtClose(hFile);

    return Status;
} // end EnvFileGetSizeA()

extern "C"
NTSTATUS
EnvFileGetSizeW(
    PWCHAR Name,
    PLONGLONG lpFileSize
    )
{
    NTSTATUS Status;
    HANDLE hFile;

    (*lpFileSize) = -1I64;

    Status = EnvFileOpenW(Name, &hFile);

    if(!NT_SUCCESS(Status))
        return Status;

    Status = EnvFileGetSizeByHandle(hFile, lpFileSize);

    NtClose(hFile);

    return Status;
} // end EnvFileGetSizeW()

extern "C"
BOOLEAN
EnvFileExistsA(PCHAR Name) {
    LONGLONG Size;
    EnvFileGetSizeA(Name, &Size);
    return Size != -1;
}

extern "C"
BOOLEAN
EnvFileExistsW(PWCHAR Name) {
    LONGLONG Size;
    EnvFileGetSizeW(Name, &Size);
    return Size != -1;
}

extern "C"
NTSTATUS
EnvFileWrite(
    HANDLE h,
    PVOID ioBuffer,
    ULONG Length,
    PULONG bytesWritten
    )
{
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS Status;

    Status = NtWriteFile(
                      h,
                      NULL,               // Event
                      NULL,               // ApcRoutine
                      NULL,               // ApcContext
                      &IoStatus,
                      ioBuffer,
                      Length,
                      NULL,               // ByteOffset
                      NULL                // Key
                      );
    (*bytesWritten) = IoStatus.Information;

    return Status;
} // end EnvFileWrite()

extern "C"
NTSTATUS
EnvFileRead(
    HANDLE h,
    PVOID ioBuffer,
    ULONG Length,
    PULONG bytesRead
    )
{
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS Status;

    Status = NtReadFile(
                      h,
                      NULL,               // Event
                      NULL,               // ApcRoutine
                      NULL,               // ApcContext
                      &IoStatus,
                      ioBuffer,
                      Length,
                      NULL,               // ByteOffset
                      NULL                // Key
                      );
    (*bytesRead) = IoStatus.Information;

    return Status;
} // end EnvFileRead()

extern "C"
NTSTATUS
EnvFileSetPointer(
    HANDLE hFile,
    LONGLONG lDistanceToMove,
    LONGLONG* lResultPointer,
    DWORD dwMoveMethod
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_STANDARD_INFORMATION FileInfo;

    switch (dwMoveMethod) {
        case ENV_FILE_BEGIN :
            CurrentPosition.CurrentByteOffset.QuadPart = lDistanceToMove;
                break;

        case ENV_FILE_CURRENT :

            // Get the current position of the file pointer
            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatus,
                        &CurrentPosition,
                        sizeof(CurrentPosition),
                        FilePositionInformation
                        );
            if(!NT_SUCCESS(Status)) {
                return Status;
            }
            CurrentPosition.CurrentByteOffset.QuadPart += lDistanceToMove;
            break;

        case ENV_FILE_END :
            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatus,
                        &FileInfo,
                        sizeof(FileInfo),
                        FileStandardInformation
                        );
            if (!NT_SUCCESS(Status)) {
                return Status;
            }
            CurrentPosition.CurrentByteOffset.QuadPart =
                                FileInfo.EndOfFile.QuadPart + lDistanceToMove;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
        }

    if ( CurrentPosition.CurrentByteOffset.QuadPart < 0 ) {
        return Status;
    }

    Status = NtSetInformationFile(
                hFile,
                &IoStatus,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );

    if(!NT_SUCCESS(Status)) {
        return Status;
    }
    if(lResultPointer) {
        *lResultPointer = CurrentPosition.CurrentByteOffset.QuadPart;
    }
    return STATUS_SUCCESS;
} // end EnvFileSetPointer()

NTSTATUS EnvFileDeleteW(PWCHAR Name) {

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS Status;
    UNICODE_STRING fName;
    HANDLE Handle;
    FILE_DISPOSITION_INFORMATION Disposition;

    RtlInitUnicodeString(&fName, Name);

    InitializeObjectAttributes(&ObjectAttributes, &fName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenFile(
                 &Handle,
                 (ACCESS_MASK)DELETE,
                 &ObjectAttributes,
                 &IoStatus,
                 FILE_SHARE_DELETE |
                 FILE_SHARE_READ |
                 FILE_SHARE_WRITE,
                 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
             );
    
    
    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
                 Handle,
                 &IoStatus,
                 &Disposition,
                 sizeof(Disposition),
                 FileDispositionInformation
             );

    NtClose(Handle);

    return Status;
}
#endif //NT_NATIVE_MODE
