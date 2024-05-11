/***
*assert.c - Display a message and abort
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <corecrt_internal_stdio.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#undef NDEBUG
#define _ASSERT_OK
#include <assert.h>

// Assertion string components:
#define MAXLINELEN  64 /* max length for line in message box */
#define ASSERTBUFSZ (MAXLINELEN * 9) /* 9 lines in message box */

// Format of stderr for assertions:
//
//      Assertion failed: <expression>, file c:\test\mytest\bar.c, line 69
//



_GENERATE_TCHAR_STRING_FUNCTIONS(assert_format, "Assertion failed: %Ts, file %Ts, line %d\n")

// Enclaves only support assertions sent to the debugger.
// This mode could also be enabled for normal apps as well.

#ifdef _UCRT_ENCLAVE_BUILD

template <typename Character>
__declspec(noreturn) static void __cdecl common_assert_to_debug(
    Character const* const expression,
    Character const* const file_name,
    unsigned         const line_number
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    Character assert_buffer[ASSERTBUFSZ];
    if (traits::sntprintf_s(assert_buffer, _countof(assert_buffer), _countof(assert_buffer), get_assert_format(Character()), expression, file_name, line_number) < 0)
    {
        abort();
    }
    traits::output_debug_string(assert_buffer);
    abort();
}

template <typename Character>
static void __cdecl common_assert(
    Character const* const expression,
    Character const* const file_name,
    unsigned         const line_number,
    void*            const
    ) throw()
{
    common_assert_to_debug(expression, file_name, line_number);
}

#else /* ^^^ _UCRT_ENCLAVE_BUILD ^^^ // vvv !_UCRT_ENCLAVE_BUILD vvv */

// Format of MessageBox for assertions:
//
//      ================= Microsft Visual C++ Debug Library ================
//
//      Assertion Failed!
//
//      Program: c:\test\mytest\foo.exe
//      File: c:\test\mytest\bar.c
//      Line: 69
//
//      Expression: <expression>
//
//      For information on how your program can cause an assertion
//      failure, see the Visual C++ documentation on asserts
//
//      (Press Retry to debug the application - JIT must be enabled)
//
//      ===================================================================



_GENERATE_TCHAR_STRING_FUNCTIONS(banner_text, "Microsoft Visual C++ Runtime Library")

_GENERATE_TCHAR_STRING_FUNCTIONS(box_intro,        "Assertion failed!")
_GENERATE_TCHAR_STRING_FUNCTIONS(program_intro,    "Program: ")
_GENERATE_TCHAR_STRING_FUNCTIONS(file_intro,       "File: ")
_GENERATE_TCHAR_STRING_FUNCTIONS(line_intro,       "Line: ")
_GENERATE_TCHAR_STRING_FUNCTIONS(expression_intro, "Expression: ")
_GENERATE_TCHAR_STRING_FUNCTIONS(info_intro,       "For information on how your program can cause an assertion\nfailure, see the Visual C++ documentation on asserts")
_GENERATE_TCHAR_STRING_FUNCTIONS(help_intro,       "(Press Retry to debug the application - JIT must be enabled)")

_GENERATE_TCHAR_STRING_FUNCTIONS(dot_dot_dot,      "...")
_GENERATE_TCHAR_STRING_FUNCTIONS(newline,          "\n")
_GENERATE_TCHAR_STRING_FUNCTIONS(double_newline,   "\n\n")

_GENERATE_TCHAR_STRING_FUNCTIONS(program_name_unknown_text, "<program name unknown>")

/***
*_assert() - Display a message and abort
*
*Purpose:
*       The assert macro calls this routine if the assert expression is
*       true.  By placing the assert code in a subroutine instead of within
*       the body of the macro, programs that call assert multiple times will
*       save space.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
static void __cdecl common_assert_to_stderr_direct(char const*, char const*, unsigned) throw()
{
    // No action for narrow strings
}

static void __cdecl common_assert_to_stderr_direct(
    wchar_t const* const expression,
    wchar_t const* const file_name,
    unsigned       const line_number
    ) throw()
{
    HANDLE const stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
#pragma warning(suppress:__WARNING_REDUNDANT_POINTER_TEST) // 28922
    if (stderr_handle == INVALID_HANDLE_VALUE || stderr_handle == nullptr)
    {
        return;
    }

    if (GetFileType(stderr_handle) != FILE_TYPE_CHAR)
    {
        return;
    }

    wchar_t assert_buffer[ASSERTBUFSZ];
#pragma warning(suppress:__WARNING_BANNED_API_USAGE) // 28719
    if (swprintf(assert_buffer, _countof(assert_buffer), get_assert_format(wchar_t()), expression, file_name, line_number) < 0)
    {
        return;
    }

    DWORD const assert_buffer_length = static_cast<DWORD>(wcslen(assert_buffer));
    DWORD characters_written = 0;
    if (WriteConsoleW(stderr_handle, assert_buffer, assert_buffer_length, &characters_written, nullptr) == 0)
    {
        return;
    }

    abort();
}

template <typename Character>
__declspec(noreturn) static void __cdecl common_assert_to_stderr(
    Character const* const expression,
    Character const* const file_name,
    unsigned         const line_number
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    // Try to write directly to the console.  This is only supported for wide
    // character strings.  If we have a narrow character string or the write
    // fails, we fall back to call through stdio.
    common_assert_to_stderr_direct(expression, file_name, line_number);

    // If stderr does not yet have a buffer, set it to use single character
    // buffering to avoid dynamic allocation of a stream buffer:
    if (!__crt_stdio_stream(stderr).has_any_buffer())
    {
        setvbuf(stderr, nullptr, _IONBF, 0);
    }

    traits::ftprintf(stderr, get_assert_format(Character()), expression, file_name, line_number);
    fflush(stderr);
    abort();
}

template <typename Character>
static void __cdecl common_assert_to_message_box_build_string(
    Character*       const assert_buffer,
    size_t           const assert_buffer_count,
    Character const* const expression,
    Character const* const file_name,
    unsigned         const line_number,
    void*            const return_address
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    // Line 1: Box introduction line:
    _ERRCHECK(traits::tcscpy_s(assert_buffer, assert_buffer_count, get_box_intro(Character())));
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_double_newline(Character())));

    // Line 2: Program line:
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_program_intro(Character())));

    Character program_name[_MAX_PATH + 1]{};

    HMODULE asserting_module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            static_cast<wchar_t const*>(return_address),
            &asserting_module))
    {
        asserting_module = nullptr;
    }

    #ifdef CRTDLL
    // If the assert came from within the CRT DLL, report it as having come
    // from the EXE instead:
    if (asserting_module == reinterpret_cast<HMODULE>(&__ImageBase))
    {
        asserting_module = nullptr;
    }
    #endif

    if (!traits::get_module_file_name(asserting_module, program_name, static_cast<DWORD>(_countof(program_name))))
    {
        _ERRCHECK(traits::tcscpy_s(program_name, _countof(program_name), get_program_name_unknown_text(Character())));
    }

    Character* pchProg = program_name;
    if (program_intro_count + traits::tcslen(program_name) + newline_length > MAXLINELEN)
    {
        pchProg += (program_intro_count + traits::tcslen(program_name) + newline_length) - MAXLINELEN;
        // Only replace first (sizeof(Character) * dot_dot_dot_length) bytes to ellipsis:
        _ERRCHECK(memcpy_s(
            pchProg,
            sizeof(Character) * ((MAX_PATH + 1) - (pchProg - program_name)),
            get_dot_dot_dot(Character()),
            sizeof(Character) * dot_dot_dot_length
            ));
    }

    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, pchProg));
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_newline(Character())));

    // Line 3:  File line
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_file_intro(Character())));

    if (file_intro_count + traits::tcslen(file_name) + newline_length > MAXLINELEN)
    {
        size_t const ffn = MAXLINELEN - file_intro_count - newline_length;

        size_t p   = 0;
        size_t len = 0;
        Character const* pch = file_name;
        for (len = traits::tcslen(file_name), p = 1;
             pch[len - p] != '\\' && pch[len - p] != '/' && p < len;
             p++)
        {
        }

        // Trim the path and file name so that they fit, using up to 2/3 of
        // the maximum number of characters for the path and the remaining
        // 1/3 for the file name:
        if ((ffn - ffn / 3) < (len - p) && ffn / 3 > p)
        {
            // The path is too long.  Use the first part of the path and the
            // full file name:
            _ERRCHECK(traits::tcsncat_s(assert_buffer, assert_buffer_count, pch, ffn - dot_dot_dot_length - p));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, get_dot_dot_dot(Character())));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, pch + len - p));
        }
        else if (ffn - ffn / 3 > len - p)
        {
            // The file name is too long.  Use the full path and the first
            // and last part of the file name, with a ... in between:
            p = p / 2;
            _ERRCHECK(traits::tcsncat_s(assert_buffer, assert_buffer_count, pch, ffn - dot_dot_dot_length - p));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, get_dot_dot_dot(Character())));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, pch + len - p));
        }
        else
        {
            // Both are too long.  Use the first part of the path and the
            // first and last part of the file name, with ...s in between:
            _ERRCHECK(traits::tcsncat_s(assert_buffer, assert_buffer_count, pch, ffn - ffn / 3 - dot_dot_dot_length));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, get_dot_dot_dot(Character())));
            _ERRCHECK(traits::tcsncat_s(assert_buffer, assert_buffer_count, pch + len - p, ffn / 6 - 1));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, get_dot_dot_dot(Character())));
            _ERRCHECK(traits::tcscat_s (assert_buffer, assert_buffer_count, pch + len - (ffn / 3 - ffn / 6 - 2)));
        }
    }
    else
    {
        // Plenty of room on the line; just append the full path and file name:
        _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, file_name));
    }

    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_newline(Character())));

    // Line 4: Line Number line:
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_line_intro(Character())));
    _ERRCHECK(traits::itot_s(
        line_number,
        assert_buffer           + traits::tcslen(assert_buffer),
        assert_buffer_count - traits::tcslen(assert_buffer),
        10));
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_double_newline(Character())));

    // Line 5: Message line:
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_expression_intro(Character())));

    size_t const characters_used =
        traits::tcslen(assert_buffer) +
        2 * double_newline_length +
        info_intro_length +
        help_intro_count;

    if (characters_used + traits::tcslen(expression) > assert_buffer_count)
    {
        size_t const characters_to_write = assert_buffer_count - (characters_used + dot_dot_dot_length);
        _ERRCHECK(traits::tcsncat_s(
            assert_buffer,
            assert_buffer_count,
            expression,
            characters_to_write));
        _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_dot_dot_dot(Character())));
    }
    else
    {
        _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, expression));
    }

    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_double_newline(Character())));

    // Info line:
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_info_intro(Character())));
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_double_newline(Character())));

    // Help line:
    _ERRCHECK(traits::tcscat_s(assert_buffer, assert_buffer_count, get_help_intro(Character())));
}



template <typename Character>
static void __cdecl common_assert_to_message_box(
    Character const* const expression,
    Character const* const file_name,
    unsigned         const line_number,
    void*            const return_address
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    Character assert_buffer[ASSERTBUFSZ]{};
    common_assert_to_message_box_build_string(
        assert_buffer,
        _countof(assert_buffer),
        expression,
        file_name,
        line_number,
        return_address);

    int const action = traits::show_message_box(
        assert_buffer,
        get_banner_text(Character()),
        MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

    switch (action)
    {
    case IDABORT: // Abort the program:
    {
        raise(SIGABRT);

        // We won't usually get here, but it's possible that a user-registered
        // abort handler returns, so exit the program immediately.  Note that
        // even though we are "aborting," we do not call abort() because we do
        // not want to invoke Watson (the user has already had an opportunity
        // to debug the error and chose not to).
        _exit(3);
    }
    case IDRETRY: // Break into the debugger then return control to caller
    {
        __debugbreak();
        return;
    }
    case IDIGNORE: // Return control to caller
    {
        return;
    }
    default: // This should not happen; treat as fatal error:
    {
        abort();
    }
    }
}

template <typename Character>
static void __cdecl common_assert(
    Character const* const expression,
    Character const* const file_name,
    unsigned         const line_number,
    void*            const return_address
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    int const current_error_mode = _set_error_mode(_REPORT_ERRMODE);
    if (current_error_mode == _OUT_TO_STDERR)
    {
        return common_assert_to_stderr(expression, file_name, line_number);
    }

    if (current_error_mode == _OUT_TO_DEFAULT && _query_app_type() == _crt_console_app)
    {
        return common_assert_to_stderr(expression, file_name, line_number);
    }

    return common_assert_to_message_box(expression, file_name, line_number, return_address);
}

#endif /* _UCRT_ENCLAVE_BUILD */

extern "C" void __cdecl _assert(
    char const* const expression,
    char const* const file_name,
    unsigned    const line_number
    ) throw()
{
    return common_assert(expression, file_name, line_number, _ReturnAddress());
}

extern "C" void __cdecl _wassert(
    wchar_t const* const expression,
    wchar_t const* const file_name,
    unsigned       const line_number
    ) 
{
    return common_assert(expression, file_name, line_number, _ReturnAddress());
}
