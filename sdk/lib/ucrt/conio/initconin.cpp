//
// initconin.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the global console input handle and the __dcrt_lowio_initialize_console_input() and
// __dcrt_terminate_console_input() functions, which initialize and close that handle.
// Also defines functions that can be used instead of the Console API to enable retry behavior
// in case the cached console handle is freed.

#include <corecrt_internal_lowio.h>

_CRT_LINKER_FORCE_INCLUDE(__dcrt_console_input_terminator);

// The global console input handle.
static HANDLE __dcrt_lowio_console_input_handle = _console_uninitialized_handle;

// Initializes the global console input handle
static void __dcrt_lowio_initialize_console_input()
{
    __dcrt_lowio_console_input_handle = CreateFileW(
        L"CONIN$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
}

extern "C" BOOL __cdecl __dcrt_lowio_ensure_console_input_initialized()
{
    if (__dcrt_lowio_console_input_handle == _console_uninitialized_handle)
    {
        __dcrt_lowio_initialize_console_input();
    }

    if (__dcrt_lowio_console_input_handle == _console_invalid_handle)
    {
        return FALSE;
    }
    return TRUE;
}

// Closes the global console input handle
extern "C" void __cdecl __dcrt_terminate_console_input()
{
    if (   __dcrt_lowio_console_input_handle != _console_invalid_handle
        && __dcrt_lowio_console_input_handle != _console_uninitialized_handle)
    {
        CloseHandle(__dcrt_lowio_console_input_handle);
    }
}

template <typename Func>
static BOOL console_input_reopen_and_retry(Func const& fp) throw()
{
    BOOL result = fp();
    if (!result && GetLastError() == ERROR_INVALID_HANDLE) {
        __dcrt_terminate_console_input();
        __dcrt_lowio_initialize_console_input();
        result = fp();
    }
    return result;
}

extern "C" BOOL __cdecl __dcrt_read_console_input(
    _Out_ PINPUT_RECORD lpBuffer,
    _In_  DWORD         nLength,
    _Out_ LPDWORD       lpNumberOfEventsRead
    )
{
    return console_input_reopen_and_retry(
        [lpBuffer, nLength, lpNumberOfEventsRead]()
        {
            return ::ReadConsoleInputW(
                __dcrt_lowio_console_input_handle,
                lpBuffer,
                nLength,
                lpNumberOfEventsRead
                );
        });
}

extern "C" BOOL __cdecl __dcrt_read_console(
    _Out_ LPVOID  lpBuffer,
    _In_  DWORD   nNumberOfCharsToRead,
    _Out_ LPDWORD lpNumberOfCharsRead
    )
{
    return console_input_reopen_and_retry(
        [lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead]()
        {
            return ::ReadConsoleW(
                __dcrt_lowio_console_input_handle,
                lpBuffer,
                nNumberOfCharsToRead,
                lpNumberOfCharsRead,
                nullptr
                );
        });
}

extern "C" BOOL __cdecl __dcrt_get_number_of_console_input_events(
    _Out_ LPDWORD lpcNumberOfEvents
    )
{
    return console_input_reopen_and_retry(
        [lpcNumberOfEvents]()
        {
            return ::GetNumberOfConsoleInputEvents(
                __dcrt_lowio_console_input_handle,
                lpcNumberOfEvents
                );
        });
}

extern "C" BOOL __cdecl __dcrt_peek_console_input_a(
    _Out_ PINPUT_RECORD lpBuffer,
    _In_  DWORD         nLength,
    _Out_ LPDWORD       lpNumberOfEventsRead
    )
{
    return console_input_reopen_and_retry(
        [lpBuffer, nLength, lpNumberOfEventsRead]()
        {
            return ::PeekConsoleInputA(
                __dcrt_lowio_console_input_handle,
                lpBuffer,
                nLength,
                lpNumberOfEventsRead
                );
        });
}

extern "C" BOOL __cdecl __dcrt_get_input_console_mode(
    _Out_ LPDWORD lpMode
    )
{
    return console_input_reopen_and_retry(
        [lpMode]()
        {
            return ::GetConsoleMode(
                __dcrt_lowio_console_input_handle,
                lpMode
                );
        });
}

extern "C" BOOL __cdecl __dcrt_set_input_console_mode(
    _In_ DWORD dwMode
    )
{
    return console_input_reopen_and_retry(
        [dwMode]()
        {
            return ::SetConsoleMode(
                __dcrt_lowio_console_input_handle,
                dwMode
                );
        });
}
