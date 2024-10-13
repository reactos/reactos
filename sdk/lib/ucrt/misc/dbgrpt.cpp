/***
*dbgrpt.c - Debug CRT Reporting Functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*******************************************************************************/
#ifndef _DEBUG
    #error This file is supported only in debug builds
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <intrin.h>
#include <malloc.h>
#include <minmax.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/*---------------------------------------------------------------------------
 *
 * Debug Reporting
 *
 --------------------------------------------------------------------------*/

extern "C" _CRT_REPORT_HOOK _pfnReportHook;
extern "C" __crt_report_hook_node<char>* _pReportHookList;
extern "C" __crt_report_hook_node<wchar_t>* _pReportHookListW;

static __crt_report_hook_node<char>*&    __cdecl get_report_hook_list(char)    throw() { return _pReportHookList;  }
static __crt_report_hook_node<wchar_t>*& __cdecl get_report_hook_list(wchar_t) throw() { return _pReportHookListW; }

static wchar_t const* const report_type_messages[_CRT_ERRCNT] =
{
    L"Warning",
    L"Error",
    L"Assertion Failed"
};

// Enclaves only support MODE_DEBUG for error output
#ifndef _UCRT_ENCLAVE_BUILD

static wchar_t const* __cdecl get_output_message_format(char) throw()
{
    return L"Debug %ls!\n\nProgram: %hs%ls%ls%hs%ls%hs%ls%hs%ls%ls%hs%ls\n\n(Press Retry to debug the application)\n";
}

static wchar_t const* __cdecl get_output_message_format(wchar_t) throw()
{
    return L"Debug %ls!\n\nProgram: %ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls\n\n(Press Retry to debug the application)\n";
}

static wchar_t const* const more_info_string =
    L"\n\nFor information on how your program can cause an assertion"
    L"\nfailure, see the Visual C++ documentation on asserts.";

_GENERATE_TCHAR_STRING_FUNCTIONS(program_name_unknown_text, "<program name unknown>")

#endif /* _UCRT_ENCLAVE_BUILD */

/***
*_CRT_REPORT_HOOK _CrtSetReportHook2() - configure client report hook in list
*
*Purpose:
*       Install or remove a client report hook from the report list.  Exists
*       separately from _CrtSetReportHook because the older function doesn't
*       work well in an environment where DLLs that are loaded and unloaded
*       dynamically out of LIFO order want to install report hooks.
*       This function exists in 2 forms - ANSI & UNICODE
*
*Entry:
*       int mode - _CRT_RPTHOOK_INSTALL or _CRT_RPTHOOK_REMOVE
*       _CRT_REPORT_HOOK pfnNewHook - report hook to install/remove/query
*
*Exit:
*       Returns -1 if an error was encountered, with EINVAL or ENOMEM set,
*       else returns the reference count of pfnNewHook after the call.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/
template <typename Character, typename Hook>
static int __cdecl common_set_report_hook(
    int  const mode,
    Hook const new_hook
    ) throw()
{
    using node_type = __crt_report_hook_node<Character>;

    _VALIDATE_RETURN(mode == _CRT_RPTHOOK_INSTALL || mode == _CRT_RPTHOOK_REMOVE, EINVAL, -1);
    _VALIDATE_RETURN(new_hook != nullptr,                                         EINVAL, -1);

    return __acrt_lock_and_call(__acrt_debug_lock, [&]
    {
        node_type*& hook_list = get_report_hook_list(Character());

        node_type* p;
        int ret = 0;

        // Search for new hook function to see if it's already installed
        for (p = hook_list; p != nullptr; p = p->next)
            if (p->hook == new_hook)
                break;

        if (mode == _CRT_RPTHOOK_REMOVE)
        {
            // Remove request - free list node if refcount goes to zero
            if (p != nullptr)
            {
                if ((ret = --p->refcount) == 0)
                {
                    if (p->next)
                    {
                        p->next->prev = p->prev;
                    }
                    if (p->prev)
                    {
                        p->prev->next = p->next;
                    }
                    else
                    {
                        hook_list = p->next;
                    }

                    _free_crt(p);
                }
            }
            else
            {
                _ASSERTE(("The hook function is not in the list!", 0));
                errno = EINVAL;
                return -1;
            }
        }
        else
        {
            // Insert request
            if (p != nullptr)
            {
                // Hook function already registered, move to head of list
                ret = ++p->refcount;
                if (p != hook_list)
                {
                    if (p->next)
                        p->next->prev = p->prev;
                    p->prev->next = p->next;
                    p->prev = nullptr;
                    p->next = hook_list;
                    hook_list->prev = p;
                    hook_list = p;
                }
            }
            else
            {
                // Hook function not already registered, insert new node
                __crt_unique_heap_ptr<node_type> new_node(_calloc_crt_t(node_type, 1));
                if (!new_node)
                {
                    ret = -1;
                    errno = ENOMEM;
                }
                else
                {
                    new_node.get()->prev = nullptr;
                    new_node.get()->next = hook_list;

                    if (hook_list)
                    {
                        hook_list->prev = new_node.get();
                    }

                    ret = new_node.get()->refcount = 1;
                    new_node.get()->hook = new_hook;
                    hook_list = new_node.detach();
                }
            }
        }

        return ret;
    });
}

extern "C" int __cdecl _CrtSetReportHook2(
    int              const mode,
    _CRT_REPORT_HOOK const new_hook
    )
{
    return common_set_report_hook<char>(mode, new_hook);
}

extern "C" int __cdecl _CrtSetReportHookW2(
    int               const mode,
    _CRT_REPORT_HOOKW const new_hook
    )
{
    return common_set_report_hook<wchar_t>(mode, new_hook);
}

#define MAXLINELEN 64

/***
*int _CrtDbgReport() - primary reporting function
*
*Purpose:
*       Display a message window with the following format.
*
*       ================= Microsft Visual C++ Debug Library ================
*
*       {Warning! | Error! | Assertion Failed!}
*
*       Program: c:\test\mytest\foo.exe
*       [Module: c:\test\mytest\bar.dll]
*       [File: c:\test\mytest\bar.c]
*       [Line: 69]
*
*       {<warning or error message> | Expression: <expression>}
*
*       [For information on how your program can cause an assertion
*        failure, see the Visual C++ documentation on asserts]
*
*       (Press Retry to debug the application)
*
*       ===================================================================
*
*Entry:
*       int             nRptType    - report type
*       void *          returnAddress - return address of caller
*       const TCHAR *    szFile      - file name
*       int             nLine       - line number
*       const TCHAR *    szModule    - module name
*       const TCHAR *    szFormat    - format string
*       ...                         - var args
*
*Exit:
*       if (MessageBox)
*       {
*           Abort -> aborts
*           Retry -> return TRUE
*           Ignore-> return FALSE
*       }
*       else
*           return FALSE
*
*Exceptions:
*       If something goes wrong, we do not assert, but we return -1.
*
*******************************************************************************/
extern "C" int __cdecl _CrtDbgReport(
    int         const report_type,
    char const* const file_name,
    int         const line_number,
    char const* const module_name,
    char const* const format,
    ...)
{
    va_list arglist;
    va_start(arglist, format);
    int const result = _VCrtDbgReportA(report_type, _ReturnAddress(), file_name, line_number, module_name, format, arglist);
    va_end(arglist);
    return result;
}

extern "C" int __cdecl _CrtDbgReportW(
    int            const report_type,
    wchar_t const* const file_name,
    int            const line_number,
    wchar_t const* const module_name,
    wchar_t const* const format,
    ...)
{
    va_list arglist;
    va_start(arglist, format);
    int const result = _VCrtDbgReportW(report_type, _ReturnAddress(), file_name, line_number, module_name, format, arglist);
    va_end(arglist);
    return result;
}

// Enclaves only support MODE_DEBUG for error output
#if !defined _UCRT_ENCLAVE_BUILD

/***
*int __crtMessageWindow() - report to a message window
*
*Purpose:
*       put report into message window, allow user to choose action to take
*
*Entry:
*       int             nRptType      - report type
*       const TCHAR *    szFile        - file name
*       const TCHAR *    szLine        - line number
*       const TCHAR *    szModule      - module name
*       const TCHAR *    szUserMessage - user message
*
*Exit:
*       if (MessageBox)
*       {
*           Abort -> aborts
*           Retry -> return TRUE
*           Ignore-> return FALSE
*       }
*       else
*           return FALSE
*
*Exceptions:
*       If something goes wrong, we do not assert, but we simply return -1,
*       which will trigger the debugger automatically (the same as the user
*       pressing the Retry button).
*
*******************************************************************************/
template <typename Character>
static int __cdecl common_message_window(
    int              const report_type,
    void*            const return_address,
    Character const* const file_name,
    Character const* const line_number,
    Character const* const module_name,
    Character const* const user_message
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    if (user_message == nullptr)
    {
        return 1;
    }

    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            static_cast<LPCWSTR>(return_address),
            &module))
    {
        module = nullptr;
    }
    #ifdef CRTDLL
    else if (module == reinterpret_cast<HMODULE>(&__ImageBase))
    {
        // If the message was reported from within the CRT DLL, report it as
        // having come from the EXE instead:
        module = nullptr;
    }
    #endif

    Character program_name[MAX_PATH + 1]{};
    if (!traits::get_module_file_name(module, program_name, static_cast<DWORD>(_countof(program_name))))
    {
        _ERRCHECK(traits::tcscpy_s(program_name, _countof(program_name), get_program_name_unknown_text(Character())));
    }

    // Shorten the program name:
    size_t const program_name_length = traits::tcslen(program_name);
    Character*   short_program_name  = program_name;
    if (program_name_length > MAXLINELEN)
    {
        short_program_name += program_name_length - MAXLINELEN;
        static_assert(MAXLINELEN > 3, "");
#pragma warning(push)
#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) // 26015
        short_program_name[0] = '.';
        short_program_name[1] = '.';
        short_program_name[2] = '.';
#pragma warning(pop)
    }

    // Shorten the module name:
    size_t    const  module_name_length = module_name ? traits::tcslen(module_name) : 0;
    Character const* short_module_name  = nullptr;
    if (module_name && module_name_length > MAXLINELEN)
    {
        short_module_name = module_name + module_name_length - MAXLINELEN + 3;
    }

    static Character const empty_string[] = { '\0' };

    wchar_t message_buffer[DBGRPT_MAX_MSG];
    int const sprintf_result = _snwprintf_s(
        message_buffer,
        _countof(message_buffer),
        _countof(message_buffer) - 1,

        get_output_message_format(Character()),

        report_type_messages[report_type],
        short_program_name,
        module_name ? L"\nModule: " : L"",
        short_module_name ? L"..." : L"",
        short_module_name ? short_module_name : (module_name ? module_name : empty_string),
        file_name ? L"\nFile: " : L"",
        file_name ? file_name : empty_string,
        line_number ? L"\nLine: " : L"",
        line_number ? line_number : empty_string,
        user_message[0] ? L"\n\n" : L"",
        user_message[0] && _CRT_ASSERT == report_type ? L"Expression: " : L"",
        user_message[0] ? user_message : empty_string,
        _CRT_ASSERT == report_type ? more_info_string : L"");

    _ERRCHECK_SPRINTF(sprintf_result);

    if (sprintf_result < 0)
    {
        _ERRCHECK(wcscpy_s(message_buffer, DBGRPT_MAX_MSG, _CRT_WIDE(DBGRPT_TOOLONGMSG)));
    }

    int const message_box_result = __acrt_show_wide_message_box(
        message_buffer,
        L"Microsoft Visual C++ Runtime Library",
        MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

    switch (message_box_result)
    {
    case IDABORT: // Abort:  Terminate execution
    {
        // Note that even though we are "aborting," we do not call abort()
        // because we do not want to invoke Watson (the user has already had an
        // opportunity to debug the error and chose not to).
        __crt_signal_handler_t const sigabrt_action = __acrt_get_sigabrt_handler();
        if (sigabrt_action != SIG_DFL)
        {
            raise(SIGABRT);
        }

        TerminateProcess(GetCurrentProcess(), 3);
    }

    case IDRETRY:
    {
        return 1; // Retry:  Debug break
    }

    case IDIGNORE:
    default:
    {
        return 0; // Ignore:  Continue execution
    }
    }
}

extern "C" int __cdecl __acrt_MessageWindowA(
    int         const report_type,
    void*       const return_address,
    char const* const file_name,
    char const* const line_number,
    char const* const module_name,
    char const* const user_message
    )
{
    return common_message_window(report_type, return_address, file_name, line_number, module_name, user_message);
}

extern "C" int __cdecl __acrt_MessageWindowW(
    int            const report_type,
    void*          const return_address,
    wchar_t const* const file_name,
    wchar_t const* const line_number,
    wchar_t const* const module_name,
    wchar_t const* const user_message
    )
{
    return common_message_window(report_type, return_address, file_name, line_number, module_name, user_message);
}

#endif
