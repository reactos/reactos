/***
*dbgrptt.c - Debug CRT Reporting Functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*******************************************************************************/

#ifndef _DEBUG
    #error This file is supported only in debug builds
    #define _DEBUG // For design-time support, when editing/viewing CRT sources
#endif

#include <corecrt_internal.h>
#include <errno.h>
#include <malloc.h>
#include <minmax.h>
#include <stdio.h>
#include <stdlib.h>



extern "C" int __cdecl __acrt_MessageWindowA(
    int         report_type,
    void*       return_address,
    char const* file_name,
    char const* line_number,
    char const* module_name,
    char const* user_message
    );

extern "C" int __cdecl __acrt_MessageWindowW(
    int            report_type,
    void*          return_address,
    wchar_t const* file_name,
    wchar_t const* line_number,
    wchar_t const* module_name,
    wchar_t const* user_message
    );

extern "C" {

_CRT_REPORT_HOOK _pfnReportHook;

__crt_report_hook_node<char> *_pReportHookList;
__crt_report_hook_node<wchar_t> *_pReportHookListW;

long _crtAssertBusy = -1;

// Enclaves only support MODE_DEBUG for error output
#ifdef _UCRT_ENCLAVE_BUILD

int const _CrtDbgMode[_CRT_ERRCNT]
{
    _CRTDBG_MODE_DEBUG,
    _CRTDBG_MODE_DEBUG,
    _CRTDBG_MODE_DEBUG
};

_HFILE const _CrtDbgFile[_CRT_ERRCNT]
{
    _CRTDBG_INVALID_HFILE,
    _CRTDBG_INVALID_HFILE,
    _CRTDBG_INVALID_HFILE
};

#else

int _CrtDbgMode[_CRT_ERRCNT]
{
    _CRTDBG_MODE_DEBUG,
    _CRTDBG_MODE_WNDW,
    _CRTDBG_MODE_WNDW
};

_HFILE _CrtDbgFile[_CRT_ERRCNT]
{
    _CRTDBG_INVALID_HFILE,
    _CRTDBG_INVALID_HFILE,
    _CRTDBG_INVALID_HFILE
};

/***
*int _CrtSetReportMode - set the reporting mode for a given report type
*
*Purpose:
*       set the reporting mode for a given report type
*
*Entry:
*       int nRptType    - the report type
*       int fMode       - new mode for given report type
*
*Exit:
*       previous mode for given report type
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/
int __cdecl _CrtSetReportMode(
    int nRptType,
    int fMode
    )
{
    int oldMode;

    /* validation section */
    _VALIDATE_RETURN(nRptType >= 0 && nRptType < _CRT_ERRCNT, EINVAL, -1);
    _VALIDATE_RETURN(
        fMode == _CRTDBG_REPORT_MODE ||
        (fMode & ~(_CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW)) == 0,
        EINVAL,
        -1);

    if (fMode == _CRTDBG_REPORT_MODE)
        return _CrtDbgMode[nRptType];

    oldMode = _CrtDbgMode[nRptType];

    _CrtDbgMode[nRptType] = fMode;

    return oldMode;
}

/***
*int _CrtSetReportFile - set the reporting file for a given report type
*
*Purpose:
*       set the reporting file for a given report type
*
*Entry:
*       int nRptType    - the report type
*       _HFILE hFile    - new file for given report type
*
*Exit:
*       previous file for given report type
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/
_HFILE __cdecl _CrtSetReportFile(
    int nRptType,
    _HFILE hFile
    )
{
    _HFILE oldFile;

    /* validation section */
    _VALIDATE_RETURN(nRptType >= 0 && nRptType < _CRT_ERRCNT, EINVAL, _CRTDBG_HFILE_ERROR);

    if (hFile == _CRTDBG_REPORT_FILE)
        return _CrtDbgFile[nRptType];

    oldFile = _CrtDbgFile[nRptType];

    if (_CRTDBG_FILE_STDOUT == hFile)
        _CrtDbgFile[nRptType] = GetStdHandle(STD_OUTPUT_HANDLE);
    else if (_CRTDBG_FILE_STDERR == hFile)
        _CrtDbgFile[nRptType] = GetStdHandle(STD_ERROR_HANDLE);
    else
        _CrtDbgFile[nRptType] = hFile;

    return oldFile;
}

#endif /* _UCRT_ENCLAVE_BUILD */

/***
*_CRT_REPORT_HOOK _CrtSetReportHook() - set client report hook
*
*Purpose:
*       set client report hook. This function is provided only in ANSI
*       for backward compatibility. No Unicode Version of this function exists
*
*Entry:
*       _CRT_REPORT_HOOK pfnNewHook - new report hook
*
*Exit:
*       return previous hook
*
*Exceptions:
*
*******************************************************************************/

_CRT_REPORT_HOOK __cdecl _CrtSetReportHook(_CRT_REPORT_HOOK pfnNewHook)
{
    _CRT_REPORT_HOOK pfnOldHook = _pfnReportHook;
    _pfnReportHook = pfnNewHook;
    return pfnOldHook;
}

/***
*_CRT_REPORT_HOOK _CrtGetReportHook() - get client report hook
*
*Purpose:
*       get client report hook.
*
*Entry:
*
*Exit:
*       return current hook
*
*Exceptions:
*
*******************************************************************************/

_CRT_REPORT_HOOK __cdecl _CrtGetReportHook(void)
{
    return _pfnReportHook;
}

#define ASSERTINTRO1 "Assertion failed: "
#define ASSERTINTRO2 "Assertion failed!"

/***
*int _VCrtDbgReportA() - _CrtDbgReport calls into this function
*
*Purpose:
*       See remarks for _CrtDbgReport.
*
*Entry:
*       int             nRptType    - report type
*       char const*    szFile      - file name
*       int             nLine       - line number
*       char const*    szModule    - module name
*       char const*    szFormat    - format string
*       va_list         arglist      - var args arglist
*
*Exit:
*       See remarks for _CrtDbgReport
*
*Exceptions:
*
*******************************************************************************/

#pragma warning(push)
#pragma warning(disable:6262)
// prefast (6262): This func uses lots of stack because we want to tolerate very large reports, and we can't use malloc here.
int __cdecl _VCrtDbgReportA(
    int nRptType,
    void * returnAddress,
    char const* szFile,
    int nLine,
    char const* szModule,
    char const* szFormat,
    va_list arglist
    )
{
    int retval=0;
    int handled=FALSE;

    char    szLineMessage[DBGRPT_MAX_MSG]{0};
    char    szOutMessage [DBGRPT_MAX_MSG]{0};
    wchar_t szOutMessage2[DBGRPT_MAX_MSG]{0};
    char    szUserMessage[DBGRPT_MAX_MSG]{0};

    if (nRptType < 0 || nRptType >= _CRT_ERRCNT)
        return -1;

    /*
     * handle the (hopefully rare) case of
     *
     * 1) ASSERT while already dealing with an ASSERT
     *      or
     * 2) two threads asserting at the same time
     */

    __try
    {
        if (_CRT_ASSERT == nRptType && _InterlockedIncrement(&_crtAssertBusy) > 0)
        {
            /* use only 'safe' functions -- must not assert in here! */

            _ERRCHECK(_itoa_s(nLine, szLineMessage, DBGRPT_MAX_MSG, 10));

            __acrt_OutputDebugStringA("Second Chance Assertion Failed: File ");
            __acrt_OutputDebugStringA(szFile ? szFile : "<file unknown>");
            __acrt_OutputDebugStringA(", Line ");
            __acrt_OutputDebugStringA(szLineMessage);
            __acrt_OutputDebugStringA("\n");

            _CrtDbgBreak();
            retval=-1;
            __leave;
        }

        // Leave space for ASSERTINTRO1 and "\r\n"
        if (szFormat)
        {
            int szlen = 0;
            _ERRCHECK_SPRINTF(szlen = _vsnprintf_s(szUserMessage, DBGRPT_MAX_MSG,
                                                    DBGRPT_MAX_MSG - 2- max(sizeof(ASSERTINTRO1),sizeof(ASSERTINTRO2)),
                                                    szFormat, arglist));
            if (szlen < 0)
            {
                _ERRCHECK(strcpy_s(szUserMessage, DBGRPT_MAX_MSG, DBGRPT_TOOLONGMSG));
            }
        }

        if (_CRT_ASSERT == nRptType)
        {
            _ERRCHECK(strcpy_s(szLineMessage, DBGRPT_MAX_MSG, szFormat ? ASSERTINTRO1 : ASSERTINTRO2));
        }

        _ERRCHECK(strcat_s(szLineMessage, DBGRPT_MAX_MSG, szUserMessage));

        if (_CRT_ASSERT == nRptType)
        {
            if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_FILE)
            {
                _ERRCHECK(strcat_s(szLineMessage, DBGRPT_MAX_MSG, "\r"));
            }

            _ERRCHECK(strcat_s(szLineMessage, DBGRPT_MAX_MSG, "\n"));
        }

        if (szFile)
        {
            int szlen = 0;
            _ERRCHECK_SPRINTF(szlen = _snprintf_s(szOutMessage, DBGRPT_MAX_MSG, DBGRPT_MAX_MSG - 1, "%s(%d) : %s",
                                                    szFile, nLine, szLineMessage));
            if (szlen < 0)
            {
                _ERRCHECK(strcpy_s(szOutMessage, DBGRPT_MAX_MSG, DBGRPT_TOOLONGMSG));
            }
        }
        else
        {
            _ERRCHECK(strcpy_s(szOutMessage, DBGRPT_MAX_MSG, szLineMessage));
        }

        {
            size_t ret = 0;
            errno_t e = 0;
            _ERRCHECK_EINVAL_ERANGE(e = mbstowcs_s(&ret, szOutMessage2, DBGRPT_MAX_MSG, szOutMessage, _TRUNCATE));
            if(e != 0)
            {
                _ERRCHECK(wcscpy_s(szOutMessage2, DBGRPT_MAX_MSG, _CRT_WIDE(DBGRPT_INVALIDMSG)));
            }
        }

        /* User hook may handle report.
            We have to check the ANSI Hook2 List & then the UNICODE Hook2 List.
            Then we have check any ANSI individual Hook set through
            SetReportHook */

        if (_pReportHookList || _pReportHookListW)
        {
            __crt_report_hook_node<char> *pnode=nullptr;
            __crt_report_hook_node<wchar_t> *pnodeW=nullptr;

            __acrt_lock(__acrt_debug_lock);
            __try
            {
                for (pnode = _pReportHookList; pnode; pnode = pnode->next)
                {
                    int hook_retval=0;
                    if (pnode->hook(nRptType, szOutMessage, &hook_retval))
                    {
                        handled=TRUE;
                        retval=hook_retval;
                        __leave;
                    }
                }

                for (pnodeW = _pReportHookListW; pnodeW; pnodeW = pnodeW->next)
                {
                    int hook_retval=0;
                    if (pnodeW->hook(nRptType, szOutMessage2, &hook_retval))
                    {
                        handled=TRUE;
                        retval=hook_retval;
                        __leave;
                    }
                }
            }
            __finally
            {
                __acrt_unlock(__acrt_debug_lock);
            }
            __endtry
        }

        if (handled)
            __leave;

        if (_pfnReportHook)
        {
            int hook_retval=0;
            if (_pfnReportHook(nRptType, szOutMessage, &hook_retval))
            {
                retval = hook_retval;
                __leave;
            }
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_FILE)
        {
            if (_CrtDbgFile[nRptType] != _CRTDBG_INVALID_HFILE)
            {
                DWORD bytes_written = 0;
                WriteFile(_CrtDbgFile[nRptType], szOutMessage, static_cast<DWORD>(strlen(szOutMessage)), &bytes_written, nullptr);
            }
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_DEBUG)
        {
            __acrt_OutputDebugStringA(szOutMessage);
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_WNDW)
        {
            szLineMessage[0] = 0;
            if (nLine)
            {
                _ERRCHECK(_itoa_s(nLine, szLineMessage, DBGRPT_MAX_MSG, 10));
            }

            retval = __acrt_MessageWindowA(nRptType, returnAddress, szFile, (nLine ? szLineMessage : nullptr), szModule, szUserMessage);
        }
    }
    __finally
    {
        if (_CRT_ASSERT == nRptType)
        {
            _InterlockedDecrement(&_crtAssertBusy);
        }
    }
    __endtry

    return retval;
}
#pragma warning(pop)

/***
*int _VCrtDbgReportW() - _CrtDbgReportW calls into this function
*
*Purpose:
*       See remarks for _CrtDbgReport.
*
*Entry:
*       int             nRptType    - report type
*       wchar_t const* szFile      - file name
*       int             nLine       - line number
*       wchar_t const* szModule    - module name
*       wchar_t const* szFormat    - format string
*       va_list         arglist      - var args arglist
*
*Exit:
*       See remarks for _CrtDbgReport
*
*Exceptions:
*
*******************************************************************************/

#pragma warning(push)
#pragma warning(disable:6262)
// prefast(6262): This func uses lots of stack because we want to tolerate very large reports, and we can't use malloc here.
int __cdecl _VCrtDbgReportW
(
    int nRptType,
    void * returnAddress,
    wchar_t const* szFile,
    int nLine,
    wchar_t const* szModule,
    wchar_t const* szFormat,
    va_list arglist
    )
{
    int retval=0;
    int handled=FALSE;
    wchar_t szLineMessage[DBGRPT_MAX_MSG] = {0};
    wchar_t szOutMessage[DBGRPT_MAX_MSG] = {0};
    char szOutMessage2[DBGRPT_MAX_MSG] = {0};
    wchar_t szUserMessage[DBGRPT_MAX_MSG] = {0};

    if (nRptType < 0 || nRptType >= _CRT_ERRCNT)
        return -1;

    /*
     * handle the (hopefully rare) case of
     *
     * 1) ASSERT while already dealing with an ASSERT
     *      or
     * 2) two threads asserting at the same time
     */

    __try
    {
        if (_CRT_ASSERT == nRptType && _InterlockedIncrement(&_crtAssertBusy) > 0)
        {
            /* use only 'safe' functions -- must not assert in here! */

            _ERRCHECK(_itow_s(nLine, szLineMessage, DBGRPT_MAX_MSG, 10));

            OutputDebugStringW(L"Second Chance Assertion Failed: File ");
            OutputDebugStringW(szFile ? szFile : L"<file unknown>");
            OutputDebugStringW(L", Line ");
            OutputDebugStringW(szLineMessage);
            OutputDebugStringW(L"\n");

            _CrtDbgBreak();
            retval = -1;
            __leave;
        }

        if (szFormat)
        {
            // Leave space for ASSERTINTRO{1,2} and "\r\n"
            size_t const max_assert_intro_count = __max(_countof(ASSERTINTRO1), _countof(ASSERTINTRO2));
            size_t const max_user_message_count = _countof(szUserMessage) - 2 - max_assert_intro_count;

            // Force use of the legacy stdio wide character format specifiers
            // mode for source compatibility.  If we ever revisit support for
            // the standard format specifiers, we'll need to revisit this as
            // well.
            int szlen = 0;
            _ERRCHECK_SPRINTF(szlen = __stdio_common_vsnwprintf_s(
                _CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS,
                szUserMessage,
                _countof(szUserMessage),
                max_user_message_count,
                szFormat,
                nullptr,
                arglist));
            if (szlen < 0)
            {
                _ERRCHECK(wcscpy_s(szUserMessage, DBGRPT_MAX_MSG, _CRT_WIDE(DBGRPT_TOOLONGMSG)));
            }
        }

        if (_CRT_ASSERT == nRptType)
            _ERRCHECK(wcscpy_s(szLineMessage, DBGRPT_MAX_MSG, szFormat ? _CRT_WIDE(ASSERTINTRO1) : _CRT_WIDE(ASSERTINTRO2)));

        _ERRCHECK(wcscat_s(szLineMessage, DBGRPT_MAX_MSG, szUserMessage));

        if (_CRT_ASSERT == nRptType)
        {
            if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_FILE)
                _ERRCHECK(wcscat_s(szLineMessage, DBGRPT_MAX_MSG, L"\r"));
            {
                _ERRCHECK(wcscat_s(szLineMessage, DBGRPT_MAX_MSG, L"\n"));
            }
        }

        if (szFile)
        {
            int szlen = 0;
            _ERRCHECK_SPRINTF(szlen = _snwprintf_s(szOutMessage, DBGRPT_MAX_MSG, DBGRPT_MAX_MSG, L"%ls(%d) : %ls",
                                                    szFile, nLine, szLineMessage));
            if (szlen < 0)
                _ERRCHECK(wcscpy_s(szOutMessage, DBGRPT_MAX_MSG, _CRT_WIDE(DBGRPT_TOOLONGMSG)));
        }
        else
        {
            _ERRCHECK(wcscpy_s(szOutMessage, DBGRPT_MAX_MSG, szLineMessage));
        }

        /* scope */
        {
            errno_t e = _ERRCHECK_EINVAL_ERANGE(wcstombs_s(nullptr, szOutMessage2, DBGRPT_MAX_MSG, szOutMessage, _TRUNCATE));
            if(e != 0)
                _ERRCHECK(strcpy_s(szOutMessage2, DBGRPT_MAX_MSG, DBGRPT_INVALIDMSG));
        }

        /* User hook may handle report.
            We have to check the ANSI Hook2 List & then the UNICODE Hook2 List.
            Then we have check any ANSI individual Hook set through
            SetReportHook */

        if (_pReportHookList || _pReportHookListW)
        {
            __crt_report_hook_node<char> *pnode=nullptr;
            __crt_report_hook_node<wchar_t> *pnodeW=nullptr;

            __acrt_lock(__acrt_debug_lock);
            __try
            {
                for (pnode = _pReportHookList; pnode; pnode = pnode->next)
                {
                    int hook_retval=0;
                    if (pnode->hook(nRptType, szOutMessage2, &hook_retval))
                    {
                        retval=hook_retval;
                        handled=TRUE;
                        __leave;
                    }
                }

                for (pnodeW = _pReportHookListW; pnodeW; pnodeW = pnodeW->next)
                {
                    int hook_retval=0;
                    if (pnodeW->hook(nRptType, szOutMessage, &hook_retval))
                    {
                        retval=hook_retval;
                        handled=TRUE;
                        __leave;
                    }
                }
            }
            __finally
            {
                __acrt_unlock(__acrt_debug_lock);
            }
            __endtry
        }

        if (handled)
            __leave;

        if(_pfnReportHook)
        {
            int hook_retval=0;
            if (_pfnReportHook(nRptType, szOutMessage2, &hook_retval))
            {
                retval = hook_retval;
                __leave;
            }
        }

        if ((_CrtDbgMode[nRptType] & _CRTDBG_MODE_FILE) && _CrtDbgFile[nRptType] != _CRTDBG_INVALID_HFILE)
        {
            /* Use WriteConsole for Consoles, WriteFile otherwise */
            switch (GetFileType(_CrtDbgFile[nRptType]))
            {
            case FILE_TYPE_CHAR:
            {
                DWORD characters_written = 0;
                if (WriteConsoleW(_CrtDbgFile[nRptType], szOutMessage, static_cast<DWORD>(wcslen(szOutMessage)), &characters_written, nullptr))
                    break;

                /* If WriteConsole fails & LastError is ERROR_INVALID_VALUE, then the console is redirected */
                if (GetLastError() != ERROR_INVALID_HANDLE)
                    break;
            }
            default:
            {
                char szaOutMessage[DBGRPT_MAX_MSG];
                size_t ret = 0;
                errno_t e = _ERRCHECK_EINVAL_ERANGE(wcstombs_s(&ret, szaOutMessage, DBGRPT_MAX_MSG, szOutMessage, _TRUNCATE));

                if (e != 0 && e != STRUNCATE)
                {
                    DWORD bytes_written = 0;
                    WriteFile(_CrtDbgFile[nRptType], szOutMessage, static_cast<DWORD>(wcslen(szOutMessage)) * 2, &bytes_written, nullptr);
                }
                else
                {
                    /* ret counts for the null terminator as well */
                    if (ret > 0)
                    {
                        --ret;
                    }

                    DWORD bytes_written = 0;
                    WriteFile(_CrtDbgFile[nRptType], szaOutMessage, static_cast<DWORD>(ret), &bytes_written, nullptr);
                }
            }
            }
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_DEBUG)
        {
            ::OutputDebugStringW(szOutMessage);
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_WNDW)
        {
            szLineMessage[0] = 0;
            if (nLine)
            {
                _ERRCHECK(_itow_s(nLine, szLineMessage, DBGRPT_MAX_MSG, 10));
            }
            retval = __acrt_MessageWindowW(nRptType, returnAddress, szFile, (nLine ? szLineMessage : nullptr), szModule, szUserMessage);
        }
    }
    __finally
    {
        if (_CRT_ASSERT == nRptType)
        {
            _InterlockedDecrement(&_crtAssertBusy);
        }
    }
    __endtry

    return retval;
}
#pragma warning(pop)

} // extern "C"
