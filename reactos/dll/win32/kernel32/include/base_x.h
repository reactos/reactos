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
// Note that Basep8BitStringToStaticUnicodeString looks deceptively similar.
// However, that function was designed for File APIs, which can be switched into
// a special "OEM" mode, that uses different NLS files/encoding, and thus calls
// RtlOemStringToAnsiString (see SetFileApisToOEM). Thererefore, this macro and
// that function are not interchangeable. As a separate note, that function uses
// the *Ex version of the Rtl conversion APIs, which does stricter checking that
// is not done when this macro is used.
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

//
// This macro (split it up in 3 pieces to allow for intermediary code in between)
// wraps the usual code path required to create an NT object based on a Unicode
// (Wide) Win32 object creation API.
//
// It makes use of BasepConvertObjectAttributes and allows for a custom access
// mode to be used, and also sets the correct error codes in case of a collision
//
#define CreateNtObjectFromWin32ApiPrologue(sec, name)                           \
{                                                                               \
    NTSTATUS Status;                                                            \
    OBJECT_ATTRIBUTES LocalAttributes;                                          \
    POBJECT_ATTRIBUTES ObjectAttributes;                                        \
    HANDLE Handle;                                                              \
    UNICODE_STRING ObjectName;                                                  \
    if (name) RtlInitUnicodeString(&ObjectName, name);                          \
    ObjectAttributes = BasepConvertObjectAttributes(&LocalAttributes,           \
                                                    sec,                        \
                                                    name ? &ObjectName : NULL);
#define CreateNtObjectFromWin32ApiBody(ntobj, access, args...)                  \
    Status = NtCreate##ntobj(&Handle, access, ObjectAttributes, args);
#define CreateNtObjectFromWin32ApiEpilogue                                      \
    if (NT_SUCCESS(Status))                                                     \
    {                                                                           \
        if (Status == STATUS_OBJECT_NAME_EXISTS)                                \
            SetLastError(ERROR_ALREADY_EXISTS);                                 \
        else                                                                    \
            SetLastError(ERROR_SUCCESS);                                        \
        return Handle;                                                          \
    }                                                                           \
    SetLastErrorByStatus(Status);                                               \
    return NULL;                                                                \
}

//
// This macro uses the CreateNtObjectFromWin32Api macros from above to create an
// NT object based on the Win32 API settings.
//
// Note that it is hardcoded to always use XXX_ALL_ACCESS permissions, which is
// the behavior up until Vista. When/if the target moves to Vista, the macro can
// be improved to support caller-specified access masks, as the underlying macro
// above does support this.
//
#define CreateNtObjectFromWin32Api(obj, ntobj, capsobj, sec, name, args...)     \
    CreateNtObjectFromWin32ApiPrologue(sec, name);                              \
    CreateNtObjectFromWin32ApiBody(ntobj, capsobj##_ALL_ACCESS, args);          \
    CreateNtObjectFromWin32ApiEpilogue

