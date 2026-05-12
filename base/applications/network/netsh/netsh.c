/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell main file
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HMODULE g_hModule = NULL;

/* FUNCTIONS ******************************************************************/

DWORD
RunScript(
    _In_ LPCWSTR filename)
{
    WCHAR tmp_string[MAX_STRING_SIZE];
    FILE *script;
    DWORD dwError = ERROR_SUCCESS;

    /* Open the file for processing */
    script = _wfopen(filename, L"r");
    if (script == NULL)
    {
        ConResPrintf(StdErr, IDS_OPEN_FAILED, filename);
        return ERROR_FILE_NOT_FOUND;
    }

    /* Read and process the script */
    while (fgetws(tmp_string, MAX_STRING_SIZE, script) != NULL)
    {
        dwError = InterpretLine(tmp_string);
        if (dwError != ERROR_SUCCESS)
            break;
    }

    /* Close the file */
    fclose(script);

    return dwError;
}


LPWSTR
MergeStrings(
    _In_ LPWSTR pszStringArray[],
    _In_ UINT nCount)
{
    LPWSTR pszOutString = NULL;
    size_t nLength;
    UINT i;

    if ((pszStringArray == NULL) || (nCount == 0))
        return NULL;

    nLength = 0;
    for (i = 0; i < nCount; i++)
        nLength += wcslen(pszStringArray[i]);

    if (nLength > 0)
        nLength += nCount; /* Space characters and terminating zero */

    pszOutString = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength * sizeof(WCHAR));
    if (pszOutString == NULL)
        return NULL;

    for (i = 0; i < nCount; i++)
    {
        if (i != 0)
            wcscat(pszOutString, L" ");
        wcscat(pszOutString, pszStringArray[i]);
    }

    return pszOutString;
}


/*
 * wmain():
 * Main entry point of the application.
 */
int
wmain(
    _In_ int argc,
    _In_ const LPWSTR argv[])
{
    LPCWSTR pszScriptFileName = NULL;
    LPCWSTR pszAliasFileName = NULL;
    LPCWSTR pszContext = NULL;
    LPWSTR pszCommand = NULL;
    int index;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("wmain(%S)\n", GetCommandLineW());

    g_hModule = GetModuleHandle(NULL);

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* FIXME: Init code goes here */
    InitAliases();
    CreateRootHelper();
    CreateRootContext();
    LoadHelpers();

    /* Process the command arguments */
    for (index = 1; index < argc; index++)
    {
        if ((_wcsicmp(argv[index], L"-?") == 0) ||
            (_wcsicmp(argv[index], L"/?") == 0) ||
            (_wcsicmp(argv[index], L"?") == 0))
        {
            /* Help option */
            ConResPuts(StdOut, IDS_APP_USAGE);
            dwError = ERROR_SUCCESS;
            goto done;
        }
        else if ((_wcsicmp(argv[index], L"-a") == 0) ||
                 (_wcsicmp(argv[index], L"/a") == 0))
        {
            /* Aliasfile option */
            if ((index + 1) < argc)
            {
                index++;
                ConPuts(StdOut, L"\nThe -a option is not implemented yet\n");
                pszAliasFileName = argv[index];
            }
            else
            {
                ConResPuts(StdOut, IDS_APP_USAGE);
                dwError = ERROR_INVALID_SYNTAX;
                goto done;
            }
        }
        else if ((_wcsicmp(argv[index], L"-c") == 0) ||
                 (_wcsicmp(argv[index], L"/c") == 0))
        {
            /* Context option */
            if ((index + 1) < argc)
            {
                index++;
                pszContext = argv[index];
            }
            else
            {
                ConResPuts(StdOut, IDS_APP_USAGE);
                dwError = ERROR_INVALID_SYNTAX;
                goto done;
            }
        }
        else if ((_wcsicmp(argv[index], L"-f") == 0) ||
                 (_wcsicmp(argv[index], L"/f") == 0))
        {
            /* File option */
            if ((index + 1) < argc)
            {
                index++;
                pszScriptFileName = argv[index];
            }
            else
            {
                ConResPuts(StdOut, IDS_APP_USAGE);
                dwError = ERROR_INVALID_SYNTAX;
                goto done;
            }
        }
        else if ((_wcsicmp(argv[index], L"-r") == 0) ||
                 (_wcsicmp(argv[index], L"/r") == 0))
        {
            /* Remote option */
            if ((index + 1) < argc)
            {
                index++;
                pszMachine = HeapAlloc(GetProcessHeap(), 0, (wcslen(argv[index]) + 1) * sizeof(WCHAR));
                if (pszMachine == NULL)
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    PrintError(g_hModule, dwError);
                    goto done;
                }

                wcscpy(pszMachine, argv[index]);
            }
            else
            {
                ConResPuts(StdOut, IDS_APP_USAGE);
                dwError = ERROR_INVALID_SYNTAX;
                goto done;
            }
        }
        else
        {
            if (pszScriptFileName != NULL)
            {
                ConResPuts(StdOut, IDS_APP_USAGE);
                dwError = ERROR_INVALID_SYNTAX;
                goto done;
            }
            else if (pszCommand == NULL)
            {
                pszCommand = MergeStrings((LPWSTR*)&argv[index], argc - index);
                if (pszCommand == NULL)
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    PrintError(g_hModule, dwError);
                    goto done;
                }

                break;
            }
        }
    }

    /* Run the alias file */
    if (pszAliasFileName != NULL)
    {
        dwError = RunScript(pszAliasFileName);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Set a context */
    if (pszContext)
    {
        dwError = InterpretLine((LPWSTR)pszContext);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Run a script, the command line instruction or the interactive interpeter */
    if (pszScriptFileName != NULL)
    {
        dwError = RunScript(pszScriptFileName);
    }
    else if (pszCommand != NULL)
    {
        dwError = InterpretLine(pszCommand);
    }
    else
    {
        InterpretInteractive();
    }

done:
    /* FIXME: Cleanup code goes here */
    if (pszMachine != NULL)
        HeapFree(GetProcessHeap(), 0, pszMachine);

    if (pszCommand != NULL)
        HeapFree(GetProcessHeap(), 0, pszCommand);

    CleanupContext();
    UnloadHelpers();
    DestroyAliases();

    return (dwError == ERROR_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
}

VOID
WINAPI
FreeQuotedString(
    _In_ LPWSTR pszQuotedString)
{
    DPRINT("FreeQuotedString(%S)\n", pszQuotedString);
    HeapFree(GetProcessHeap(), 0, pszQuotedString);
}

VOID
WINAPI
FreeString(
    _In_ LPWSTR pszString)
{
    DPRINT("FreeString(%S)\n", pszString);
    LocalFree(pszString);
}

LPWSTR
WINAPI
MakeQuotedString(
    _In_ LPWSTR pszString)
{
    LPWSTR pszQuotedString;

    DPRINT("MakeQuotedString(%S)\n", pszString);

    pszQuotedString = HeapAlloc(GetProcessHeap(), 0, (wcslen(pszString) + 3) * sizeof(WCHAR));
    if (pszQuotedString == NULL)
        return NULL;

    _swprintf(pszQuotedString, L"\"%s\"", pszString);

    return pszQuotedString;
}

LPWSTR
CDECL
MakeString(
    _In_ HANDLE hModule,
    _In_ DWORD dwMsgId,
    ...)
{
    LPCWSTR pszStr;
    LPWSTR pszInBuffer, pszOutBuffer = NULL;
    DWORD dwLength;
    va_list ap;

    DPRINT("MakeString(%p %lu ...)\n", hModule, dwMsgId);

    dwLength = LoadStringW(hModule, dwMsgId, (LPWSTR)&pszStr, 0);
    if (dwLength == 0)
        return NULL;

    /* Allocate and copy the resource string, NUL-terminated */
    pszInBuffer = HeapAlloc(GetProcessHeap(), 0, (dwLength + 1) * sizeof(WCHAR));
    if (pszInBuffer == NULL)
        return NULL;
    CopyMemory(pszInBuffer, pszStr, dwLength * sizeof(WCHAR));
    pszInBuffer[dwLength] = UNICODE_NULL;

    va_start(ap, dwMsgId);
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszInBuffer,
                   0,
                   0,
                   (LPWSTR)&pszOutBuffer,
                   0,
                   &ap);
    va_end(ap);

    HeapFree(GetProcessHeap(), 0, pszInBuffer);

    return pszOutBuffer;
}

DWORD
WINAPI
MatchEnumTag(
    _In_ HANDLE hModule,
    _In_ LPCWSTR pwcArg,
    _In_ DWORD dwNumArg,
    _In_ const TOKEN_VALUE *pEnumTable,
    _Out_ PDWORD pdwValue)
{
    DWORD i;

    DPRINT("MatchEnumTag(%p %p %lu %p %p)\n", hModule, pwcArg, dwNumArg, pEnumTable, pdwValue);

    if ((pEnumTable == NULL) || (pdwValue == NULL))
        return ERROR_INVALID_PARAMETER;

    for (i = 0; i < dwNumArg; i++)
    {
        if (MatchToken(pwcArg, pEnumTable[i].pwszToken))
        {
            *pdwValue = pEnumTable[i].dwValue;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_FOUND;
}

DWORD
WINAPI
MatchTagsInCmdLine(
    _In_ HANDLE hModule,
    _Inout_ LPWSTR *ppwcArguments,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ TAG_TYPE *pttTags,
    _In_ DWORD dwTagCount,
    _Out_ DWORD *pdwTagType)
{
    DWORD i;

    DPRINT1("MatchTagsInCmdLine(%p %p %lu %lu %p %lu %p) stub!\n",
            hModule, ppwcArguments, dwCurrentIndex, dwArgCount,
            pttTags, dwTagCount, pdwTagType);

    for (i = dwCurrentIndex; i < dwArgCount; i++)
    {
        DPRINT1("Argument %lu: %S\n", i, ppwcArguments[i]);
    }

    for (i = 0; i < dwTagCount; i++)
    {
        DPRINT1("Tag %lu: %S\n", i, pttTags[i].pwszTag);
    }

    return 0;
}

BOOL
WINAPI
MatchToken(
    _In_ LPCWSTR pwszUserToken,
    _In_ LPCWSTR pwszCmdToken)
{
    DPRINT("MatchToken(%S %S)\n", pwszUserToken, pwszCmdToken);

    if ((pwszUserToken == NULL) || (pwszCmdToken == NULL))
        return FALSE;

    return (_wcsnicmp(pwszUserToken, pwszCmdToken, wcslen(pwszUserToken)) == 0) ? TRUE : FALSE;
}

DWORD
WINAPI 
NsGetFriendlyNameFromIfName(
    _In_ DWORD dwUnknown1,
    _In_ PWSTR pszIfName, 
    _Inout_ PWSTR pszFriendlyName,
    _Inout_ PDWORD pdwFriendlyName)
{
    UNICODE_STRING UnicodeIfName;
    GUID InterfaceGuid;
    NTSTATUS Status;
    DWORD ret;

    DPRINT("NsGetFriendlyNameFromIfName(%lx %S %p %p)\n",
           dwUnknown1, pszIfName, pszFriendlyName, pdwFriendlyName);

    RtlInitUnicodeString(&UnicodeIfName, pszIfName);
    Status = RtlGUIDFromString(&UnicodeIfName,
                               &InterfaceGuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlGUIDFromString failed 0x%08lx\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    ret = NhGetInterfaceNameFromDeviceGuid(&InterfaceGuid,
                                           pszFriendlyName,
                                           pdwFriendlyName,
                                           0, 0);
    if (ret != ERROR_SUCCESS)
    {
        DPRINT1("NhGetInterfaceNameFromDeviceGuid() failed %lu\n", ret);
    }

    return ret;
}

DWORD
WINAPI
PreprocessCommand(
    _In_ HANDLE hModule,
    _Inout_ LPWSTR *ppwcArguments,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ TAG_TYPE *pttTags,
    _In_ DWORD dwTagCount,
    _In_ DWORD dwMinArgs,
    _In_ DWORD dwMaxArgs,
    _Out_ DWORD *pdwTagType)
{
    DPRINT1("PreprocessCommand()\n");
    return 0;
}

DWORD
CDECL
PrintError(
    _In_opt_ HANDLE hModule,
    _In_ DWORD dwErrId,
    ...)
{
    INT Length;
    va_list ap;

    va_start(ap, dwErrId);

    if (hModule)
    {
        Length = ConResMsgPrintfExV(StdErr, hModule, 0, dwErrId,
                                    LANG_USER_DEFAULT, &ap);
    }
    else
    {
        if ((dwErrId > NETSH_ERROR_BASE) && (dwErrId < NETSH_ERROR_END))
        {
            Length = ConResMsgPrintfExV(StdErr, g_hModule, 0, dwErrId,
                                        LANG_USER_DEFAULT, &ap);
        }
        else
        {
            Length = ConMsgPrintfV(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
                                   NULL, dwErrId,
                                   LANG_USER_DEFAULT, &ap);
        }
    }

    va_end(ap);

    return (DWORD)Length;
}

DWORD
CDECL
PrintMessageFromModule(
    _In_ HANDLE hModule,
    _In_ DWORD  dwMsgId,
    ...)
{
    INT Length;
    va_list ap;

    va_start(ap, dwMsgId);
    Length = ConResPrintfExV(StdOut, hModule, dwMsgId,
                             LANG_USER_DEFAULT, ap);
    va_end(ap);

    return (DWORD)Length;
}

DWORD
CDECL
PrintMessage(
    _In_ LPCWSTR pwszFormat,
    ...)
{
    INT Length;
    va_list ap;

    va_start(ap, pwszFormat);
    Length = ConPrintfV(StdOut, pwszFormat, ap);
    va_end(ap);

    return (DWORD)Length;
}
