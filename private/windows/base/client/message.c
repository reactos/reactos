/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    message.c

Abstract:

    This module contains the Win32 Message Management APIs

Author:

    Steve Wood (stevewo) 24-Jan-1991

Revision History:

                  02-May-94 BruceMa Fix FormatMessage to accept Win32 status
                                    codes wrapped as HRESULTS

--*/

#include "basedll.h"

DWORD
APIENTRY
BaseDllFormatMessage(
    BOOLEAN ArgumentsAreAnsi,
    DWORD dwFlags,
    LPVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    PWSTR lpBuffer,
    DWORD nSize,
    va_list *arglist
    );

DWORD
APIENTRY
FormatMessageA(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPSTR lpBuffer,
    DWORD nSize,
    va_list *lpArguments
    )
{
    NTSTATUS Status;
    DWORD Result;
    PWSTR UnicodeSource;
    PWSTR UnicodeBuffer;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        RtlInitAnsiString( &AnsiString, lpSource );
        Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            return 0;
            }

        UnicodeSource = UnicodeString.Buffer;
        }
    else {
        UnicodeSource = (PWSTR)lpSource;
        }

    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        UnicodeBuffer = (PWSTR)lpBuffer;
        }
    else {
        UnicodeBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                         MAKE_TAG( TMP_TAG ),
                                         nSize * sizeof( WCHAR )
                                       );
        }
    if (UnicodeBuffer != NULL) {
        Result = BaseDllFormatMessage( TRUE,
                                       dwFlags,
                                       (LPVOID)UnicodeSource,
                                       dwMessageId,
                                       dwLanguageId,
                                       UnicodeBuffer,
                                       nSize,
                                       lpArguments
                                     );
        }
    else {
        BaseSetLastNTError( STATUS_NO_MEMORY );
        Result = 0;
        }

    if (UnicodeSource != (PWSTR)lpSource) {
        RtlFreeUnicodeString( &UnicodeString );
        }

    if (Result != 0) {
        UnicodeString.Length = (USHORT)(Result * sizeof( WCHAR ));
        UnicodeString.MaximumLength = (USHORT)(UnicodeString.Length + sizeof( UNICODE_NULL ));
        if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
            UnicodeString.Buffer = *(PWSTR *)lpBuffer;
            UnicodeBuffer = UnicodeString.Buffer;
            Status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeString, TRUE );
            if (NT_SUCCESS( Status )) {
                *(LPSTR *)lpBuffer = AnsiString.Buffer;
                }
            else {
                *(LPSTR *)lpBuffer = NULL;
                }
            }
        else {
            UnicodeString.Buffer = UnicodeBuffer;
            AnsiString.Buffer = lpBuffer;
            AnsiString.Length = 0;
            AnsiString.MaximumLength = (USHORT)nSize;
            Status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeString, FALSE );
            }

        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            Result = 0;
            }
        else {
            //
            // Ajust return value, since Result contains Unicode char counts,
            // we have to adjust it to ANSI char counts
            //
            Result = AnsiString.Length;
            }
        }
    else {
        if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
            UnicodeBuffer = NULL;
            }
        }

    if (UnicodeBuffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );
        }

    return Result;
}


DWORD
APIENTRY
FormatMessageW(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    PWSTR lpBuffer,
    DWORD nSize,
    va_list *lpArguments
    )
{
    return BaseDllFormatMessage( FALSE,
                                 dwFlags,
                                 (LPVOID)lpSource,
                                 dwMessageId,
                                 dwLanguageId,
                                 lpBuffer,
                                 nSize,
                                 lpArguments
                               );
}


BOOLEAN
CreateVirtualBuffer(
    OUT PVIRTUAL_BUFFER Buffer,
    IN ULONG CommitSize OPTIONAL,
    IN ULONG ReserveSize OPTIONAL
    )
{
    MEMORY_BASIC_INFORMATION MemoryInformation;
    ULONG MemoryInformationLength;

    if (!ARGUMENT_PRESENT( LongToPtr(CommitSize) )) {
        CommitSize = 1;
        }

    if (!ARGUMENT_PRESENT( LongToPtr(ReserveSize) )) {
        ReserveSize = ROUND_UP( CommitSize, 0x10000 );
        }

    Buffer->Base = VirtualAlloc( NULL,
                                 ReserveSize,
                                 MEM_RESERVE,
                                 PAGE_READWRITE
                               );
    if (Buffer->Base == NULL) {
        return FALSE;
        }

    MemoryInformationLength = VirtualQuery( Buffer->Base,
                                            &MemoryInformation,
                                            sizeof( MemoryInformation )
                                          );
    if (MemoryInformationLength == sizeof( MemoryInformation )) {
        ReserveSize = (ULONG)MemoryInformation.RegionSize;
        if (VirtualAlloc( Buffer->Base,
                          CommitSize,
                          MEM_COMMIT,
                          PAGE_READWRITE
                        ) != NULL
           ) {
            MemoryInformationLength = VirtualQuery( Buffer->Base,
                                                    &MemoryInformation,
                                                    sizeof( MemoryInformation )
                                                  );
            if (MemoryInformationLength == sizeof( MemoryInformation )) {
                CommitSize = (ULONG)MemoryInformation.RegionSize;
                Buffer->CommitLimit = (PVOID)
                    ((char *)Buffer->Base + CommitSize);

                Buffer->ReserveLimit = (PVOID)
                    ((char *)Buffer->Base + ReserveSize);

                return TRUE;
                }
            }
        }

    VirtualFree( Buffer->Base, 0, MEM_RELEASE );
    return FALSE;
}


BOOLEAN
ExtendVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer,
    IN PVOID Address
    )
{
    SIZE_T NewCommitSize;
    PVOID NewCommitLimit;

    if (Address >= Buffer->Base) {
        if (Address < Buffer->CommitLimit) {
            return TRUE;
            }

        if (Address >= Buffer->ReserveLimit) {
            return FALSE;
            }

        NewCommitSize =
            ((SIZE_T)ROUND_UP_TO_PAGES( (ULONG_PTR)Address + 1 ) - (ULONG_PTR)Buffer->CommitLimit);

        NewCommitLimit = VirtualAlloc( Buffer->CommitLimit,
                                       NewCommitSize,
                                       MEM_COMMIT,
                                       PAGE_READWRITE
                                     );
        if (NewCommitLimit != NULL) {
            Buffer->CommitLimit = (PVOID)
                ((ULONG_PTR)NewCommitLimit + NewCommitSize);

            return TRUE;
            }
        }

    return FALSE;
}


BOOLEAN
TrimVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    )
{
    Buffer->CommitLimit = Buffer->Base;
    return (BOOLEAN)VirtualFree( Buffer->Base, 0, MEM_DECOMMIT );
}

BOOLEAN
FreeVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    )
{
    return (BOOLEAN)VirtualFree( Buffer->Base, 0, MEM_RELEASE );
}

int
VirtualBufferExceptionHandler(
    IN DWORD ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PVIRTUAL_BUFFER Buffer
    )
{
    PVOID BadAddress;

    //
    // If this is an access violation touching memory within
    // our reserved buffer, but outside of the committed portion
    // of the buffer, then we are going to take this exception.
    //

    if (ExceptionCode == STATUS_ACCESS_VIOLATION) {
        BadAddress = (PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ];
        if (BadAddress >= Buffer->CommitLimit &&
            BadAddress < Buffer->ReserveLimit
           ) {
            //
            // This is our exception.  If there is room to commit
            // more memory, try to do so.  If no room or unable
            // to commit, then execute the exception handler.
            // Otherwise we were able to commit the additional
            // buffer space, so update the commit limit on the
            // caller's stack and retry the faulting instruction.
            //

            if (ExtendVirtualBuffer( Buffer, BadAddress )) {
        return EXCEPTION_CONTINUE_EXECUTION;
                }
            else {
                return EXCEPTION_EXECUTE_HANDLER;
                }
            }
        }

    //
    // Not an exception we care about, so pass it up the chain.
    //

    return EXCEPTION_CONTINUE_SEARCH;
}

HMODULE BasepNetMsg;

DWORD
APIENTRY
BaseDllFormatMessage(
    BOOLEAN ArgumentsAreAnsi,
    DWORD dwFlags,
    LPVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    PWSTR lpBuffer,
    DWORD nSize,
    va_list *arglist
    )
{
    VIRTUAL_BUFFER Buffer;
    NTSTATUS Status;
    PVOID DllHandle;
    ULONG MaximumWidth;
    ULONG LengthNeeded, Result;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PWSTR MessageFormat;
    PWSTR lpAllocedBuffer;
    PWSTR lpDst;
    BOOLEAN IgnoreInserts;
    BOOLEAN ArgumentsAreAnArray;

    /* If this is a Win32 error wrapped as an OLE HRESULT then unwrap it */
    if (((dwMessageId & 0xffff0000) == 0x80070000)  &&
        (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)      &&
        !(dwFlags & FORMAT_MESSAGE_FROM_HMODULE)    &&
        !(dwFlags & FORMAT_MESSAGE_FROM_STRING))
    {
        dwMessageId &= 0x0000ffff;
    }

    if (lpBuffer == NULL) {
        BaseSetLastNTError( STATUS_INVALID_PARAMETER );
        return 0;
        }

    lpAllocedBuffer = NULL;
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        *(PVOID *)lpBuffer = NULL;
        }

    if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) {
        IgnoreInserts = TRUE;
        }
    else {
        IgnoreInserts = FALSE;
        }

    if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY) {
        ArgumentsAreAnArray = TRUE;
        }
    else {
        ArgumentsAreAnArray = FALSE;
        }

    Result = 0;
    if (CreateVirtualBuffer( &Buffer, nSize + 1, 0 )) try {
        UnicodeString.Buffer = NULL;
        MaximumWidth = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
        if (MaximumWidth == FORMAT_MESSAGE_MAX_WIDTH_MASK) {
            MaximumWidth = 0xFFFFFFFF;
            }

        UnicodeString.Buffer = NULL;
        if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
            MessageFormat = lpSource;
            }
        else {
            if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
                DllHandle = BasepMapModuleHandle( (HMODULE)lpSource, TRUE );
                }
            else
            if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
retrySystem:
                DllHandle = (PVOID)BaseDllHandle;
                }
            else {
                BaseSetLastNTError( STATUS_INVALID_PARAMETER );
                goto failureExit;
                }

retrySystem2:
            Status = RtlFindMessage( DllHandle,
                                     PtrToUlong(RT_MESSAGETABLE),
                                     (ULONG)dwLanguageId,
                                     dwMessageId,
                                     &MessageEntry
                                   );

            if (!NT_SUCCESS( Status )) {
                if (Status == STATUS_MESSAGE_NOT_FOUND) {
                    if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE &&
                        dwFlags & FORMAT_MESSAGE_FROM_SYSTEM
                       ) {
                        dwFlags &= ~FORMAT_MESSAGE_FROM_HMODULE;
                        goto retrySystem;
                        }
                    if ( dwFlags & FORMAT_MESSAGE_FROM_SYSTEM &&
                         DllHandle == (PVOID)BaseDllHandle
                       ) {
                        //
                        // The message isn't in kernel32.dll, conditionally
                        // load netmsg.dll to see if the message is there.
                        // Leave the dll mapped for subsequent message lookups
                        //
                        if (!BasepNetMsg) {
                            BasepNetMsg = LoadLibraryExW(L"netmsg.dll",NULL,LOAD_LIBRARY_AS_DATAFILE);
                            }
                        if (BasepNetMsg) {
                            DllHandle = BasepNetMsg;
                            goto retrySystem2;
                            }
                        }
                    SetLastError( ERROR_MR_MID_NOT_FOUND );
                    }
                else {
                    BaseSetLastNTError( Status );
                    }
                goto failureExit;
                }

            if (!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {
                RtlInitAnsiString( &AnsiString, MessageEntry->Text );
                Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );
                if (!NT_SUCCESS( Status )) {
                    BaseSetLastNTError( Status );
                    goto failureExit;
                    }

                MessageFormat = UnicodeString.Buffer;
                }
            else {
                MessageFormat = (PWSTR)MessageEntry->Text;
                }
            }

        Status = RtlFormatMessage( MessageFormat,
                                   MaximumWidth,
                                   IgnoreInserts,
                                   ArgumentsAreAnsi,
                                   ArgumentsAreAnArray,
                                   arglist,
                                   Buffer.Base,
                                   (ULONG)((PCHAR)Buffer.ReserveLimit - (PCHAR)Buffer.Base),
                                   &LengthNeeded
                                 );

        RtlFreeUnicodeString( &UnicodeString );

        if (NT_SUCCESS( Status )) {
            if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
                lpAllocedBuffer = (PWSTR)LocalAlloc( LMEM_FIXED, LengthNeeded );
                if (lpAllocedBuffer == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    goto failureExit;
                    }

                lpDst = lpAllocedBuffer;
                }
            else
            if ((LengthNeeded / sizeof( WCHAR )) > nSize) {
                BaseSetLastNTError( STATUS_BUFFER_TOO_SMALL );
                goto failureExit;
                }
            else {
                lpDst = lpBuffer;
                }

            RtlMoveMemory( lpDst, Buffer.Base, LengthNeeded );
            Result = (LengthNeeded - sizeof( WCHAR )) / sizeof( WCHAR );
            }
        else {
            BaseSetLastNTError( Status );
            }

failureExit:    ;
        }
    except( VirtualBufferExceptionHandler( GetExceptionCode(),
                                           GetExceptionInformation(),
                                           &Buffer
                                         )
          ) {
        if (GetExceptionCode() == STATUS_ACCESS_VIOLATION) {
            BaseSetLastNTError( STATUS_NO_MEMORY );
            }
        else {
            BaseSetLastNTError( GetExceptionCode() );
            }

        Result = 0;
        }

    if (lpAllocedBuffer != NULL) {
        if (Result) {
            *(PVOID *)lpBuffer = lpAllocedBuffer;
            }
        else {
            LocalFree( lpAllocedBuffer );
            }
        }

    FreeVirtualBuffer( &Buffer );

    return( Result );
}
