/***
*report_runtime_error.cpp - startup error messages
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Prints out banner for runtime error messages.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <stdlib.h>



// This is used during the expansion of the runtime error text.
#define EOL L"\r\n"

static bool __cdecl issue_debug_notification(wchar_t const* const message) throw()
{
    // This is referenced only in the Debug CRT build
    UNREFERENCED_PARAMETER(message);

#ifdef _DEBUG
    switch (_CrtDbgReportW(_CRT_ERROR, nullptr, 0, nullptr, L"%ls", message))
    {
    case 1:
        _CrtDbgBreak();
        return true;

    case 0:
        return true;
    }
#endif // _DEBUG

    return false;
}




// Enclaves do not support error messages outside of OutputDebugString.
#ifdef _UCRT_ENCLAVE_BUILD

extern "C" void __cdecl __acrt_report_runtime_error(wchar_t const* const message)
{
    // Report the error using the debug
    issue_debug_notification(message);
}

#else /* ^^^ _UCRT_ENCLAVE_BUILD ^^^ // vvv !_UCRT_ENCLAVE_BUILD vvv */

/*
 * __acrt_app_type, together with __error_mode, determine how error messages
 * are written out.
 */
static _crt_app_type __acrt_app_type = _crt_unknown_app;

/***
*void _set_app_type(int apptype) - interface to change __acrt_app_type
*
*Purpose:
*       Set, or change, the value of __acrt_app_type.
*
*       Set the default debug lib report destination for console apps.
*
*       This function is for INTERNAL USE ONLY.
*
*Entry:
*       int modeval =   _crt_unknown_app,   unknown
*                       _crt_console_app,   console, or command line, application
*                       _crt_gui_app,       GUI, or Windows, application
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

extern "C" void __cdecl _set_app_type(_crt_app_type const new_app_type)
{
    __acrt_app_type = new_app_type;
}

extern "C" _crt_app_type __cdecl _query_app_type()
{
    return __acrt_app_type;
}



static bool __cdecl should_write_error_to_console() throw()
{
    int const error_mode = _set_error_mode(_REPORT_ERRMODE);

    if (error_mode == _OUT_TO_STDERR)
    {
        return true;
    }

    if (error_mode == _OUT_TO_DEFAULT && __acrt_app_type == _crt_console_app)
    {
        return true;
    }

    return false;
}



static void write_string_to_console(wchar_t const* const wide_string) throw()
{
    HANDLE const handle = GetStdHandle(STD_ERROR_HANDLE);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // We convert the wide string to a narrow string by truncating each character.
    // Currently, the text for each runtime error consists only of ASCII, so this
    // is acceptable.  If the error text is ever localized, this would need to
    // change.
    size_t const narrow_buffer_count = 500;
    char narrow_buffer[narrow_buffer_count];

    char* const narrow_first = narrow_buffer;
    char* const narrow_last  = narrow_first + narrow_buffer_count;

    // Note that this loop copies the null terminator if the loop terminates
    // befoe running out of buffer space:
    char*          narrow_it = narrow_first;
    wchar_t const* wide_it   = wide_string;
    do
    {
        *narrow_it = static_cast<char>(*wide_it);
    }
    while (++narrow_it != narrow_last && *wide_it++ != '\0');

    // If we did run out of buffer space, this will null-terminate the text that
    // we were able to copy:
    *(narrow_last - 1) = '\0';

    DWORD const bytes_to_write = static_cast<DWORD>(narrow_it - narrow_first - 1); // Account for null terminator
    DWORD       bytes_written  = 0;
    WriteFile(handle, narrow_buffer, bytes_to_write, &bytes_written, nullptr);
}



extern "C" void __cdecl __acrt_report_runtime_error(wchar_t const* const message)
{
    // Before we report the error via the normal path, report the error using
    // the debug
    if (issue_debug_notification(message))
    {
        return;
    }

    if (should_write_error_to_console())
    {
        write_string_to_console(message);
    }
    else
    {
        #define MSGTEXTPREFIX L"Runtime Error!\n\nProgram: "
        static wchar_t outmsg[sizeof(MSGTEXTPREFIX) / sizeof(wchar_t) + _MAX_PATH + 2 + 500];
        // runtime error msg + progname + 2 newline + runtime error text.
        wchar_t* progname = &outmsg[sizeof(MSGTEXTPREFIX) / sizeof(wchar_t) - 1];
        size_t progname_size = _countof(outmsg) - (progname - outmsg);
        wchar_t* pch = progname;

        _ERRCHECK(wcscpy_s(outmsg, _countof(outmsg), MSGTEXTPREFIX));

        progname[MAX_PATH] = L'\0';
        if (!GetModuleFileNameW(nullptr, progname, MAX_PATH))
        {
            _ERRCHECK(wcscpy_s(progname, progname_size, L"<program name unknown>"));
        }

        #define MAXLINELEN 60
        if (wcslen(pch) + 1 > MAXLINELEN)
        {
            pch += wcslen(progname) + 1 - MAXLINELEN;
            _ERRCHECK(wcsncpy_s(pch, progname_size - (pch - progname), L"...", 3));
        }

        _ERRCHECK(wcscat_s(outmsg, _countof(outmsg), L"\n\n"));
        _ERRCHECK(wcscat_s(outmsg, _countof(outmsg), message));

        // Okay to ignore return value here, this is just to display the message box.
        // Only caller is abort() (so we shouldn't/can't handle IDABORT), so the process
        // will end shortly.
        __acrt_show_wide_message_box(
            outmsg,
            L"Microsoft Visual C++ Runtime Library",
            MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
    }
}

#endif /* _UCRT_ENCLAVE_BUILD */
