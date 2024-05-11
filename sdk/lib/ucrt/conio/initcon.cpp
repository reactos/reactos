//
// initcon.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the global console output handle and the __dcrt_lowio_initialize_console_output() and
// __dcrt_terminate_console_output() functions, which initialize and close that handle.
// Also defines functions that can be used instead of the Console API to enable retry behavior
// in case the cached console handle is freed.

#include <corecrt_internal_lowio.h>

_CRT_LINKER_FORCE_INCLUDE(__dcrt_console_output_terminator);

// The global console output handle.
static HANDLE __dcrt_lowio_console_output_handle = _console_uninitialized_handle;

// Initializes the global console output handle
static void __dcrt_lowio_initialize_console_output()
{
    __dcrt_lowio_console_output_handle = CreateFileW(
        L"CONOUT$",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
}

extern "C" BOOL __cdecl __dcrt_lowio_ensure_console_output_initialized()
{
    if (__dcrt_lowio_console_output_handle == _console_uninitialized_handle)
    {
        __dcrt_lowio_initialize_console_output();
    }

    if (__dcrt_lowio_console_output_handle == _console_invalid_handle)
    {
        return FALSE;
    }
    return TRUE;
}

// Closes the global console output handle
extern "C" void __cdecl __dcrt_terminate_console_output()
{
    if (   __dcrt_lowio_console_output_handle != _console_invalid_handle
        && __dcrt_lowio_console_output_handle != _console_uninitialized_handle)
    {
        CloseHandle(__dcrt_lowio_console_output_handle);
    }
}

template <typename Func>
static BOOL console_output_reopen_and_retry(Func const& fp) throw()
{
    BOOL result = fp();
    if (!result && GetLastError() == ERROR_INVALID_HANDLE) {
        __dcrt_terminate_console_output();
        __dcrt_lowio_initialize_console_output();
        result = fp();
    }
    return result;
}

extern "C" BOOL __cdecl __dcrt_write_console(
    _In_  void const * lpBuffer,
    _In_  DWORD        nNumberOfCharsToWrite,
    _Out_ LPDWORD      lpNumberOfCharsWritten
    )
{
    return console_output_reopen_and_retry(
        [lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten]()
        {
            return ::WriteConsoleW(
                __dcrt_lowio_console_output_handle,
                lpBuffer,
                nNumberOfCharsToWrite,
                lpNumberOfCharsWritten,
                nullptr);
        });
}
