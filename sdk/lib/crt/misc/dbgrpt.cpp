/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug CRT reporting functions
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

// This file should not be included in release builds,
// but since we do not have a good mechanism for this at the moment,
// just rely on the compiler to optimize it away instead of omitting the code.
//#ifdef _DEBUG

#include <crtdbg.h>
#include <stdio.h>
#include <signal.h>
#include <windows.h>

#undef OutputDebugString
#undef _CrtSetReportMode
#undef _CrtSetReportFile

#define DBGRPT_MAX_BUFFER_SIZE  4096
#define DBGRPT_ASSERT_PREFIX_MESSAGE    "Assertion failed: "
#define DBGRPT_ASSERT_PREFIX_NOMESSAGE  "Assertion failed!"
#define DBGRPT_STRING_TOO_LONG          "_CrtDbgReport: String too long"

// Keep track of active asserts
static long _CrtInAssert = -1;
// State per type
static int _CrtModeOutputFormat[_CRT_ERRCNT] =
{
    _CRTDBG_MODE_DEBUG,
    _CRTDBG_MODE_WNDW,
    _CRTDBG_MODE_WNDW,
};
// Caption per type
static const wchar_t* _CrtModeMessages[_CRT_ERRCNT] =
{
    L"Warning",
    L"Error",
    L"Assertion Failed"
};
// Report files
static _HFILE _CrtReportFiles[_CRT_ERRCNT] =
{
    _CRTDBG_INVALID_HFILE,
    _CRTDBG_INVALID_HFILE,
    _CRTDBG_INVALID_HFILE
};

// Manually delay-load as to not have a dependency on user32
typedef int (WINAPI *tMessageBoxW)(_In_opt_ HWND hWnd, _In_opt_ LPCWSTR lpText, _In_opt_ LPCWSTR lpCaption, _In_ UINT uType);
static HMODULE _CrtUser32Handle = NULL;
static tMessageBoxW _CrtMessageBoxW = NULL;

template <typename char_t>
struct dbgrpt_char_traits;

template<>
struct dbgrpt_char_traits<char>
{
    typedef char char_t;

    static const wchar_t* szAssertionMessage;
    static const char_t* szEmptyString;
    static const char_t* szUnknownFile;

    static void OutputDebugString(const char_t* message);
    static size_t StringLength(const char_t* str) { return strlen(str); }
};

template<>
struct dbgrpt_char_traits<wchar_t>
{
    typedef wchar_t char_t;

    static const wchar_t* szAssertionMessage;
    static const char_t* szEmptyString;
    static const char_t* szUnknownFile;

    static void OutputDebugString(const char_t* message);
    static size_t StringLength(const char_t* str) { return wcslen(str); };
};

// Shortcut
typedef dbgrpt_char_traits<char> achar_traits;
typedef dbgrpt_char_traits<wchar_t> wchar_traits;

const wchar_t* achar_traits::szAssertionMessage =
    L"Debug %s!\n"
    L"%s%hs" /* module */
    L"%s%hs" /* filename */
    L"%s%s" /* linenumber */
    L"%s%hs" /* message */
    L"\n\n(Press Retry to debug the application)";
const wchar_t* wchar_traits::szAssertionMessage =
    L"Debug %s!\n"
    L"%s%ws" /* module */
    L"%s%ws" /* filename */
    L"%s%s" /* linenumber */
    L"%s%ws" /* message */
    L"\n\n(Press Retry to debug the application)";

const achar_traits::char_t* achar_traits::szEmptyString = "";
const wchar_traits::char_t* wchar_traits::szEmptyString = L"";

const achar_traits::char_t* achar_traits::szUnknownFile = "<unknown file>";
const wchar_traits::char_t* wchar_traits::szUnknownFile = L"<unknown file>";

inline void achar_traits::OutputDebugString(const char* message)
{
    OutputDebugStringA(message);
}

inline void wchar_traits::OutputDebugString(const wchar_t* message)
{
    OutputDebugStringW(message);
}

static
HMODULE _CrtGetUser32()
{
    if (_CrtUser32Handle == NULL)
    {
        HMODULE mod = LoadLibraryExW(L"user32.dll", NULL, 0 /* NT6+: LOAD_LIBRARY_SEARCH_SYSTEM32 */);
        if (mod == NULL)
            mod = (HMODULE)INVALID_HANDLE_VALUE;

        if (_InterlockedCompareExchangePointer((PVOID*)&_CrtUser32Handle, mod, NULL))
        {
            if (mod != INVALID_HANDLE_VALUE)
                FreeLibrary(mod);
        }
    }

    return (_CrtUser32Handle != INVALID_HANDLE_VALUE ? _CrtUser32Handle : NULL);
}

static tMessageBoxW _CrtGetMessageBox()
{
    HMODULE mod = _CrtGetUser32();

    if (_CrtMessageBoxW == NULL && mod != INVALID_HANDLE_VALUE)
    {
        tMessageBoxW proc = (tMessageBoxW)GetProcAddress(mod, "MessageBoxW");
        if (proc == NULL)
            proc = (tMessageBoxW)INVALID_HANDLE_VALUE;

        _InterlockedCompareExchangePointer((PVOID*)&_CrtMessageBoxW, (PVOID)proc, NULL);
    }

    return (_CrtMessageBoxW != INVALID_HANDLE_VALUE ? _CrtMessageBoxW : NULL);
}


template <typename char_t>
static int _CrtDbgReportWindow(int reportType, const char_t *filename, int linenumber, const char_t *moduleName, const char_t* message)
{
    typedef dbgrpt_char_traits<char_t> traits;

    wchar_t szCompleteMessage[DBGRPT_MAX_BUFFER_SIZE] = {0};
    wchar_t LineBuffer[20] = {0};

    if (filename && !filename[0])
        filename = NULL;
    if (moduleName && !moduleName[0])
        moduleName = NULL;
    if (message && !message[0])
        message = NULL;
    if (linenumber)
        _itow(linenumber, LineBuffer, 10);

    _snwprintf(szCompleteMessage,
               _countof(szCompleteMessage) - 1,
               traits::szAssertionMessage,
               _CrtModeMessages[reportType],
               moduleName ? L"\nModule: " : L"", moduleName ? moduleName : traits::szEmptyString,
               filename ? L"\nFile: " : L"", filename ? filename : traits::szEmptyString,
               LineBuffer[0] ? L"\nLine: " : L"", LineBuffer[0] ? LineBuffer : L"",
               message ? L"\n\n" : L"", message ? message : traits::szEmptyString);

    if (IsDebuggerPresent())
    {
        OutputDebugStringW(szCompleteMessage);
    }

    tMessageBoxW messageBox = _CrtGetMessageBox();
    if (!messageBox)
        return (IsDebuggerPresent() ? IDRETRY : IDABORT);

    // TODO: If we are not interacive, add MB_SERVICE_NOTIFICATION
    return messageBox(NULL, szCompleteMessage, L"ReactOS C++ Runtime Library",
                      MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
}

template <typename char_t>
static int _CrtEnterDbgReport(int reportType, const char_t *filename, int linenumber)
{
    typedef dbgrpt_char_traits<char_t> traits;

    if (reportType < 0 || reportType >= _CRT_ERRCNT)
        return FALSE;

    if (reportType == _CRT_ASSERT)
    {
        if (_InterlockedIncrement(&_CrtInAssert) > 0)
        {
            char LineBuffer[20] = {0};

            _itoa(linenumber, LineBuffer, 10);

            OutputDebugStringA("Nested Assert from File: ");
            traits::OutputDebugString(filename ? filename : traits::szUnknownFile);
            OutputDebugStringA(", Line: ");
            OutputDebugStringA(LineBuffer);
            OutputDebugStringA("\n");

            _CrtDbgBreak();

            _InterlockedDecrement(&_CrtInAssert);
            return FALSE;
        }
    }
    return TRUE;
}

static
void _CrtLeaveDbgReport(int reportType)
{
    if (reportType == _CRT_ASSERT)
        _InterlockedDecrement(&_CrtInAssert);
}

EXTERN_C
int __cdecl _CrtSetReportMode(int reportType, int reportMode)
{
    if (reportType >= _CRT_ERRCNT || reportType < 0)
        return 0;

    int oldReportMode = _CrtModeOutputFormat[reportType];
    if (reportMode != _CRTDBG_REPORT_MODE)
        _CrtModeOutputFormat[reportType] = reportMode;
    return oldReportMode;
}

EXTERN_C
_HFILE __cdecl _CrtSetReportFile(int reportType, _HFILE reportFile)
{
    if (reportType >= _CRT_ERRCNT || reportType < 0)
        return NULL;

    _HFILE oldReportFile = _CrtReportFiles[reportType];
    if (reportFile != _CRTDBG_REPORT_FILE)
        _CrtReportFiles[reportType] = reportFile;
    return oldReportFile;
}

template <typename char_t>
static inline BOOL _CrtDbgReportToFile(HANDLE hFile, const char_t* szMsg)
{
    typedef dbgrpt_char_traits<char_t> traits;

    if (hFile == _CRTDBG_INVALID_HFILE || hFile == NULL)
        return FALSE;

    if (hFile == _CRTDBG_FILE_STDOUT)
        hFile = ::GetStdHandle(STD_OUTPUT_HANDLE);
    else if (hFile == _CRTDBG_FILE_STDERR)
        hFile = ::GetStdHandle(STD_ERROR_HANDLE);

    DWORD cbMsg = (DWORD)(traits::StringLength(szMsg) * sizeof(char_t));
    return ::WriteFile(hFile, szMsg, cbMsg, &cbMsg, NULL);
}

template <typename char_t>
static int _CrtHandleDbgReport(int reportType, const char_t* szCompleteMessage, const char_t* szFormatted,
                               const char_t *filename, int linenumber, const char_t *moduleName)
{
    typedef dbgrpt_char_traits<char_t> traits;

    if (_CrtModeOutputFormat[reportType] & _CRTDBG_MODE_FILE)
    {
        _CrtDbgReportToFile<char_t>(_CrtReportFiles[reportType], szCompleteMessage);
    }

    if (_CrtModeOutputFormat[reportType] & _CRTDBG_MODE_DEBUG)
    {
        traits::OutputDebugString(szCompleteMessage);
    }

    if (_CrtModeOutputFormat[reportType] & _CRTDBG_MODE_WNDW)
    {
        int nResult = _CrtDbgReportWindow(reportType, filename, linenumber, moduleName, szFormatted);
        switch (nResult)
        {
        case IDRETRY:
            return TRUE;
        case IDIGNORE:
        default:
            return FALSE;
        case IDABORT:
            raise(SIGABRT);
            _exit(3);
            return FALSE;   // Unreachable
        }
    }

    return FALSE;
}


EXTERN_C
int __cdecl
_VCrtDbgReportA(
    int reportType,
    const char *filename,
    int linenumber,
    const char *moduleName,
    const char *format,
    va_list arglist)
{
    char szFormatted[DBGRPT_MAX_BUFFER_SIZE] = {0};       // The user provided message
    char szCompleteMessage[DBGRPT_MAX_BUFFER_SIZE] = {0}; // The output for debug / file

    // Check for recursive _CrtDbgReport calls, and validate reportType
    if (!_CrtEnterDbgReport(reportType, filename, linenumber))
        return -1;

    if (filename)
    {
        _snprintf(szCompleteMessage,
                  _countof(szCompleteMessage) - 1,
                  "%s(%d) : ",
                  filename,
                  linenumber);
    }

    if (format)
    {
        int len = _vsnprintf(szFormatted,
                             _countof(szFormatted) - 2 - _countof(DBGRPT_ASSERT_PREFIX_MESSAGE),
                             format,
                             arglist);
        if (len < 0)
        {
            strcpy(szFormatted, DBGRPT_STRING_TOO_LONG);
        }

        if (reportType == _CRT_ASSERT)
            strcat(szCompleteMessage, DBGRPT_ASSERT_PREFIX_MESSAGE);
        strcat(szCompleteMessage, szFormatted);
    }
    else if (reportType == _CRT_ASSERT)
    {
        strcat(szCompleteMessage, DBGRPT_ASSERT_PREFIX_NOMESSAGE);
    }

    if (reportType == _CRT_ASSERT)
    {
        if (_CrtModeOutputFormat[reportType] & _CRTDBG_MODE_FILE)
            strcat(szCompleteMessage, "\r");
        strcat(szCompleteMessage, "\n");
    }

    // FIXME: Handle user report hooks here

    int nResult = _CrtHandleDbgReport(reportType, szCompleteMessage, szFormatted, filename, linenumber, moduleName);

    _CrtLeaveDbgReport(reportType);

    return nResult;
}

EXTERN_C
int __cdecl
_VCrtDbgReportW(
    int reportType,
    const wchar_t *filename,
    int linenumber,
    const wchar_t *moduleName,
    const wchar_t *format,
    va_list arglist)
{
    wchar_t szFormatted[DBGRPT_MAX_BUFFER_SIZE] = {0};       // The user provided message
    wchar_t szCompleteMessage[DBGRPT_MAX_BUFFER_SIZE] = {0}; // The output for debug / file

    // Check for recursive _CrtDbgReportW calls, and validate reportType
    if (!_CrtEnterDbgReport(reportType, filename, linenumber))
        return -1;

    if (filename)
    {
        _snwprintf(szCompleteMessage,
                   _countof(szCompleteMessage) - 1,
                   L"%s(%d) : ",
                   filename,
                   linenumber);
    }

    if (format)
    {
        int len = _vsnwprintf(szFormatted,
                              _countof(szFormatted) - 2 - _countof(DBGRPT_ASSERT_PREFIX_MESSAGE),
                              format,
                              arglist);
        if (len < 0)
        {
            wcscpy(szFormatted, _CRT_WIDE(DBGRPT_STRING_TOO_LONG));
        }

        if (reportType == _CRT_ASSERT)
            wcscat(szCompleteMessage, _CRT_WIDE(DBGRPT_ASSERT_PREFIX_MESSAGE));
        wcscat(szCompleteMessage, szFormatted);
    }
    else if (reportType == _CRT_ASSERT)
    {
        wcscat(szCompleteMessage, _CRT_WIDE(DBGRPT_ASSERT_PREFIX_NOMESSAGE));
    }

    if (reportType == _CRT_ASSERT)
    {
        if (_CrtModeOutputFormat[reportType] & _CRTDBG_MODE_FILE)
            wcscat(szCompleteMessage, L"\r");
        wcscat(szCompleteMessage, L"\n");
    }

    // FIXME: Handle user report hooks here

    int nResult = _CrtHandleDbgReport(reportType, szCompleteMessage, szFormatted, filename, linenumber, moduleName);

    _CrtLeaveDbgReport(reportType);

    return nResult;
}

EXTERN_C
int __cdecl
_CrtDbgReportV(
    int reportType,
    const char *filename,
    int linenumber,
    const char *moduleName,
    const char *format,
    va_list arglist)
{
    return _VCrtDbgReportA(reportType, filename, linenumber, moduleName, format, arglist);
}

EXTERN_C
int __cdecl
_CrtDbgReportWV(
    int reportType,
    const wchar_t *filename,
    int linenumber,
    const wchar_t *moduleName,
    const wchar_t *format,
    va_list arglist)
{
    return _VCrtDbgReportW(reportType, filename, linenumber, moduleName, format, arglist);
}

EXTERN_C
int __cdecl
_CrtDbgReport(
    int reportType,
    const char *filename,
    int linenumber,
    const char *moduleName,
    const char *format,
    ...)
{
    va_list arglist;
    int result;

    va_start(arglist, format);
    result = _VCrtDbgReportA(reportType, filename, linenumber, moduleName, format, arglist);
    va_end(arglist);
    return result;
}

EXTERN_C
int __cdecl
_CrtDbgReportW(
    int reportType,
    const wchar_t *filename,
    int linenumber,
    const wchar_t *moduleName,
    const wchar_t *format,
    ...)
{
    va_list arglist;
    int result;

    va_start(arglist, format);
    result = _VCrtDbgReportW(reportType, filename, linenumber, moduleName, format, arglist);
    va_end(arglist);
    return result;
}

//#endif // _DEBUG
