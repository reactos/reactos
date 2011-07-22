/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/include/base_x.h
 * PURPOSE:         Base API Client Macros
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#pragma once

/* INCLUDES *******************************************************************/

//
// This macro (split it up in 3 pieces to allow for intermediary code in between)
// converts a NULL-terminated ASCII string, usually associated with an object
// name, into its NT-native UNICODE_STRING structure, by using the TEB's Static
// Unicode String.
//
// It should only be used when the name is supposed to be less than MAX_PATH
// (260 characters).
//
// It returns the correct ERROR_FILENAME_EXCED_RANGE Win32 error when the path
// is too long.
//
#define ConvertAnsiToUnicodePrologue                                            \
{                                                                               \
    NTSTATUS Status;                                                            \
    PUNICODE_STRING UnicodeCache;                                               \
    ANSI_STRING AnsiName;
#define ConvertAnsiToUnicodeBody(name)                                          \
    UnicodeCache = &NtCurrentTeb()->StaticUnicodeString;                        \
    RtlInitAnsiString(&AnsiName, name);                                         \
    Status = RtlAnsiStringToUnicodeString(UnicodeCache, &AnsiName, FALSE);
#define ConvertAnsiToUnicodeEpilogue                                            \
    if (Status == STATUS_BUFFER_OVERFLOW)                                       \
        SetLastError(ERROR_FILENAME_EXCED_RANGE);                               \
    else                                                                        \
        SetLastErrorByStatus(Status);                                           \
    return FALSE;                                                               \
}

//
// This macro uses the ConvertAnsiToUnicode macros above to convert a CreateXxxA
// Win32 API into its equivalent CreateXxxW API.
//
#define ConvertWin32AnsiObjectApiToUnicodeApi(obj, name, args...)               \
    ConvertAnsiToUnicodePrologue                                                \
    if (!name) return Create##obj##W(args, NULL);                               \
    ConvertAnsiToUnicodeBody(name)                                              \
    if (NT_SUCCESS(Status)) return Create##obj##W(args, UnicodeCache->Buffer);  \
    ConvertAnsiToUnicodeEpilogue

//
// This macro uses the ConvertAnsiToUnicode macros above to convert a FindFirst*A
// Win32 API into its equivalent FindFirst*W API.
//
#define ConvertWin32AnsiChangeApiToUnicodeApi(obj, name, args...)               \
    ConvertAnsiToUnicodePrologue                                                \
    ConvertAnsiToUnicodeBody(name)                                              \
    if (NT_SUCCESS(Status)) return obj##W(UnicodeCache->Buffer, args);          \
    ConvertAnsiToUnicodeEpilogue

