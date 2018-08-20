/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS EventCreate Command
 * FILE:            base/applications/cmdutils/eventcreate/eventcreate.c
 * PURPOSE:         Allows reporting custom user events in event logs,
 *                  by using the old-school NT <= 2k3 logging API.
 * PROGRAMMER:      Hermes Belusca-Maito
 *
 * RATIONALE AND NOTE ABOUT THE IMPLEMENTATION:
 *
 *    Contrary to what can be expected, there is no simple way of logging inside
 *    a NT event log an arbitrary string, for example a description text, that
 *    can then be viewed in a human-readable form under an event viewer. Indeed,
 *    a NT log entry is not just a simple arbitrary string, but is instead made
 *    of an identifier (ID), an event "source" and an arbitrary data chunk.
 *    To make things somewhat simpler, the data chunk is divided in two parts:
 *    an array of data strings and a raw (binary) data chunk.
 *
 *    How then can a log entry be reconstructed? At each NT log is associated
 *    one or many event "sources", which are binary files (PE format) containing
 *    a table of predefined string templates (message table resource), indexed
 *    by identifiers. The ID and event source specified in a given log entry
 *    inside a given log allows to refer to one of the string template inside
 *    the specified event source of the log. A human-readable event description
 *    that is shown by an event viewer is obtained by associating the string
 *    template together with the array of data strings of the log entry.
 *    Each of the data strings is a parameter for the string template (formatted
 *    in a printf-like format).
 *
 *    Thus we see that the human-readable event description of a log entry is
 *    not completely arbitrary but is dictated by both the string templates and
 *    the data strings of the log entry. Only the data strings can be arbitrary.
 *
 *    Therefore, what can we do to be able to report arbitrary human-readable
 *    events, the description of which the user specifies at the command-line?
 *    There is actually only one possible way: store the description text as
 *    a string inside the array of data strings of the event. But we need the
 *    event to be displayed correctly. For that it needs to be associated with
 *    an event source, and its ID must point to a suitable string template, the
 *    association of which with the user-specified arbitrary data string should
 *    directly display this arbitrary string. The suitable string template is
 *    therefore the identity template: "%1" (in the format for message strings).
 *    The last problem, that may constitute a limitation of this technique, is
 *    that this string template is tied to a given event ID. What if the user
 *    wants to use a different event ID? The solution is the event source to
 *    contain as many same identity templates as different IDs the user can use.
 *    This is quite a redundant and limiting technique!
 *
 *    On MS Windows, the EventCreate.exe command contains the identity template
 *    for all IDs from 1 to 1001 included, yet it is only possible to specify
 *    an event ID from 1 to 1000 included.
 *    The Powershell command "Write-EventLog" allows using IDs from 0 to 65535
 *    included, thus covering all of the 2-byte unsigned integer space; its
 *    corresponding event source file "EventLogMessages.dll"
 *    (inside "%SystemRoot%\Microsoft.NET\Framework\vX.Y.ZZZZZ") therefore
 *    contains the identity template for all IDs from 0 to 65535, making it a
 *    large file.
 *
 *    For ReactOS I want to have a compromise between disk space and usage
 *    flexibility, therefore I choose to include as well the identity template
 *    for all IDs from 0 to 65535 included, as done by Powershell. If somebody
 *    wants to change these limits, one has to perform the following steps:
 *
 *    0- Update the "/ID EventID" description in the help string "IDS_HELP"
 *       inside the lang/xx-YY.rc resource files;
 *
 *    1- Change in this file the two #defines EVENT_ID_MIN and EVENT_ID_MAX
 *       to other values of one's choice (called 'ID_min' and 'ID_max');
 *
 *    2- Regenerate and replace the event message string templates file using
 *       the event message string templates file generator (evtmsggen tool):
 *         $ evtmsggen ID_min ID_max evtmsgstr.mc
 *
 *    3- Recompile the EventCreate command.
 *
 */

#include <stdio.h>
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE

#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#include <conutils.h>

#include <strsafe.h>

#include "resource.h"

/*
 * The minimal and maximal values of the allowed ID range.
 * See the "NOTE ABOUT THE IMPLEMENTATION" above.
 *
 * Here are some examples of values:
 * Windows' EventCreate.exe command   : ID_min = 1 and ID_max = 1000
 * Powershell "Write-EventLog" command: ID_min = 0 and ID_max = 65535
 *
 * ReactOS' EventCreate.exe command uses the same limits as Powershell.
 */
#define EVENT_ID_MIN    0
#define EVENT_ID_MAX    65535

/*
 * The EventCreate command internal name (used for both setting the default
 * event source name and specifying the default event source file path).
 */
#define APPLICATION_NAME    L"EventCreate"


VOID PrintError(DWORD dwError)
{
    if (dwError == ERROR_SUCCESS)
        return;

    ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
               NULL, dwError, LANG_USER_DEFAULT);
    ConPuts(StdErr, L"\n");
}


static BOOL
GetUserToken(
    OUT PTOKEN_USER* ppUserToken)
{
    BOOL Success = FALSE;
    DWORD dwError;
    HANDLE hToken;
    DWORD cbTokenBuffer = 0;
    PTOKEN_USER pUserToken = NULL;

    *ppUserToken = NULL;

    /* Get the process token */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    /* Retrieve token's information */
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &cbTokenBuffer))
    {
        dwError = GetLastError();
        if (dwError != ERROR_INSUFFICIENT_BUFFER)
            goto Quit;
    }

    pUserToken = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbTokenBuffer);
    if (!pUserToken)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Quit;
    }

    if (!GetTokenInformation(hToken, TokenUser, pUserToken, cbTokenBuffer, &cbTokenBuffer))
    {
        dwError = GetLastError();
        goto Quit;
    }

    Success = TRUE;
    dwError = ERROR_SUCCESS;
    *ppUserToken = pUserToken;

Quit:
    if (Success == FALSE)
    {
        if (pUserToken)
            HeapFree(GetProcessHeap(), 0, pUserToken);
    }

    CloseHandle(hToken);

    SetLastError(dwError);

    return Success;
}

static LONG
InstallEventSource(
    IN HKEY    hEventLogKey,
    IN LPCWSTR EventLogSource)
{
    LONG lRet;
    HKEY hSourceKey = NULL;
    DWORD dwDisposition = 0;
    DWORD dwData;

    LPCWSTR EventMessageFile;
    DWORD PathSize;
    WCHAR ExePath[MAX_PATH];

    lRet = RegCreateKeyExW(hEventLogKey,
                           EventLogSource,
                           0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_SET_VALUE, NULL,
                           &hSourceKey, &dwDisposition);
    if (lRet != ERROR_SUCCESS)
        goto Quit;
    if (dwDisposition != REG_CREATED_NEW_KEY)
    {
        /* The source key already exists, just quit */
        goto Quit;
    }

    /* We just have created the new source. Add the values. */

    /*
     * Retrieve the full path of the current running executable.
     * We need it to install our custom event source.
     * - In case of success, try to replace the ReactOS installation path
     *   (if present in the executable path) by %SystemRoot%.
     * - In case of failure, use a default path.
     */
    PathSize = GetModuleFileNameW(NULL, ExePath, ARRAYSIZE(ExePath));
    if ((PathSize == 0) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        /* We failed, copy the default value */
        StringCchCopyW(ExePath, ARRAYSIZE(ExePath),
                       L"%SystemRoot%\\System32\\" APPLICATION_NAME L".exe");
    }
    else
    {
        /* Alternatively one can use SharedUserData->NtSystemRoot */
        WCHAR TmpDir[ARRAYSIZE(ExePath)];
        PathSize = GetSystemWindowsDirectoryW(TmpDir, ARRAYSIZE(TmpDir));
        if ((PathSize > 0) && (_wcsnicmp(ExePath, TmpDir, PathSize) == 0))
        {
            StringCchCopyW(TmpDir, ARRAYSIZE(TmpDir), L"%SystemRoot%");
            StringCchCatW(TmpDir, ARRAYSIZE(TmpDir), ExePath + PathSize);
            StringCchCopyW(ExePath, ARRAYSIZE(ExePath), TmpDir);
        }
    }
    EventMessageFile = ExePath;

    lRet = ERROR_SUCCESS;

    dwData = 1;
    RegSetValueExW(hSourceKey, L"CustomSource", 0, REG_DWORD,
                   (LPBYTE)&dwData, sizeof(dwData));

    // FIXME: Set those flags according to caller's rights?
    // Or, if we are using the Security log?
    dwData = EVENTLOG_SUCCESS | EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE
            /* | EVENTLOG_AUDIT_SUCCESS | EVENTLOG_AUDIT_FAILURE */ ;
    RegSetValueExW(hSourceKey, L"TypesSupported", 0, REG_DWORD,
                   (LPBYTE)&dwData, sizeof(dwData));

    RegSetValueExW(hSourceKey, L"EventMessageFile", 0, REG_EXPAND_SZ,
                   (LPBYTE)EventMessageFile, (DWORD)(wcslen(EventMessageFile) + 1) * sizeof(WCHAR));

    RegFlushKey(hSourceKey);

Quit:
    if (hSourceKey)
        RegCloseKey(hSourceKey);

    return lRet;
}

static BOOL
CheckLogOrSourceExistence(
    IN LPCWSTR UNCServerName OPTIONAL,
    IN LPCWSTR EventLogName,
    IN LPCWSTR EventLogSource,
    IN BOOL    AllowAppSources OPTIONAL)
{
    /*
     * The 'AllowAppSources' parameter allows the usage of
     * application (non-custom) sources, when set to TRUE.
     * Its default value is FALSE.
     */

#define MAX_KEY_LENGTH 255 // or 256 ??

    BOOL Success = FALSE;
    LONG lRet;
    HKEY hEventLogKey = NULL, hLogKey = NULL;
    DWORD NameLen;
    DWORD dwIndex;

    BOOL LogNameValid, LogSourceValid;
    BOOL FoundLog = FALSE, FoundSource = FALSE;
    BOOL SourceAlreadyExists = FALSE, SourceCreated = FALSE, IsCustomSource = FALSE;

    WCHAR LogName[MAX_KEY_LENGTH];    // Current event log being tested for.
    WCHAR LogNameErr[MAX_KEY_LENGTH]; // Event log in which the source already exists.

    UNREFERENCED_PARAMETER(UNCServerName); // FIXME: Use remote server if needed!

    LogNameValid   = (EventLogName && *EventLogName);
    LogSourceValid = (EventLogSource && *EventLogSource);

    /*
     * If neither the log name nor the log source are specified,
     * there is no need to continue. Just fail.
     */
    if (!LogNameValid && !LogSourceValid)
        return FALSE;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, // FIXME: Use remote server if needed!
                         L"SYSTEM\\CurrentControlSet\\Services\\EventLog",
                         0, KEY_ENUMERATE_SUB_KEYS,
                         &hEventLogKey);
    if (lRet != ERROR_SUCCESS)
        goto Quit;

    /*
     * If we just have a valid log name but no specified source, check whether
     * the log key exist by atttempting to open it. If we fail: no log.
     * In all cases we do not perform other tests nor create any source.
     */
    if (LogNameValid && !LogSourceValid)
    {
        lRet = RegOpenKeyExW(hEventLogKey,
                             EventLogName,
                             0, KEY_QUERY_VALUE,
                             &hLogKey);
        RegCloseKey(hLogKey);
        FoundLog = (lRet == ERROR_SUCCESS);

        if (FoundLog)
        {
            /* Set the flags to consistent values */
            SourceCreated  = TRUE;
            IsCustomSource = TRUE;
        }
        goto Finalize;
    }

    /* Here, LogSourceValid is always TRUE */

    /*
     * We now have a valid source and either an event log name or none.
     * Search for the source existence over all the existing logs:
     * we loop through the event logs and we will:
     * - localize whether the specified source exists and in which log it does;
     * - and at the same time, check whether the specified log does exist.
     */
    dwIndex = 0;
    while (TRUE)
    {
        NameLen = ARRAYSIZE(LogName);
        LogName[0] = L'\0';

        lRet = RegEnumKeyExW(hEventLogKey, dwIndex, LogName, &NameLen,
                             NULL, NULL, NULL, NULL);
        if (dwIndex > 0)
        {
            if (lRet == ERROR_NO_MORE_ITEMS)
            {
                /*
                 * We may/may not have found our log and may/may not have found
                 * our source. Quit the loop, we will check those details after.
                 */
                break; // goto Finalize;
            }
        }
        if (lRet != ERROR_SUCCESS)
        {
            /* A registry error happened, just fail */
            goto Quit;
        }

        /* We will then continue with the next log */
        ++dwIndex;

        /* If we have specified a log, check whether we have found it */
        if (LogNameValid && _wcsicmp(LogName, EventLogName) == 0)
        {
            /*
             * We have found the specified log, but do not break yet: if we have
             * a specified source, we want to be sure it does not exist elsewhere.
             */
            FoundLog = TRUE;
        }

        /*
         * The following case: if (LogNameValid && !LogSourceValid) {...}
         * was already dealt with before. Here, LogSourceValid is always TRUE.
         */

        /* Now determine whether we need to continue */
        if (/* LogNameValid && */ FoundLog)
        {
#if 0
            if (!LogSourceValid)
            {
                /*
                 * We have found our log and we do not use any source,
                 * we can stop scanning now.
                 */
                /* Set the flags to consistent values */
                SourceCreated  = TRUE;
                IsCustomSource = TRUE;
                break; // goto Finalize;
            }
#endif
            if (SourceAlreadyExists)
            {
                /*
                 * We have finally found our log but the source existed elsewhere,
                 * stop scanning and we will error that the source is not in the
                 * expected log. On the contrary, if our log was not found yet,
                 * continue scanning to attempt to find it and, if the log is not
                 * found at the end we will error that the log does not exist.
                 */
                break; // goto Finalize;
            }
        }

        /*
         * If we have specified a source and have not found it so far,
         * check for its presence in this log.
         * NOTE: Here, LogSourceValid is always TRUE.
         */
        if (LogSourceValid && !FoundSource)
        {
            HKEY hKeySource = NULL;

            /* Check the sources inside this log */
            lRet = RegOpenKeyExW(hEventLogKey, LogName, 0, KEY_READ, &hLogKey);
            if (lRet != ERROR_SUCCESS)
            {
                /* A registry error happened, just fail */
                goto Quit;
            }

            /*
             * Attempt to open the source key.
             *
             * NOTE: Alternatively we could have scanned each source key
             * in this log by using RegEnumKeyExW.
             */
            lRet = RegOpenKeyExW(hLogKey, EventLogSource,
                                 0, KEY_QUERY_VALUE,
                                 &hKeySource);

            /* Get rid of the log key handle */
            RegCloseKey(hLogKey);
            hLogKey = NULL;

            if (lRet == ERROR_SUCCESS) // || lRet == ERROR_ACCESS_DENIED
            {
                /*
                 * We have found our source in this log (it can be
                 * in a different log than the one specified).
                 */
                FoundSource = TRUE;
                // lRet = ERROR_SUCCESS;
            }
            else if (lRet == ERROR_FILE_NOT_FOUND)
            {
                /* Our source was not found there */
                lRet = ERROR_SUCCESS;
                hKeySource = NULL;
            }
            else // if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND)
            {
                /* A registry error happened, but we continue scanning the other logs... */
                hKeySource = NULL;
            }

            /* If we have not found our source, continue scanning the other logs */
            if (!FoundSource)
                continue;

            /*
             * We have found our source, but is it in the correct log?
             *
             * NOTE: We check only in the case we have specified a log,
             * otherwise we just care about the existence of the source
             * and we do not check for its presence in the other logs.
             */
            if (LogNameValid && !(FoundLog && _wcsicmp(LogName, EventLogName) == 0))
            {
                /* Now get rid of the source key handle */
                RegCloseKey(hKeySource);
                hKeySource = NULL;

                /* The source is in another log than the specified one */
                SourceAlreadyExists = TRUE;

                /* Save the log name in which the source already exists */
                RtlCopyMemory(LogNameErr, LogName, sizeof(LogName));

                /*
                 * We continue because we want to also know whether we can
                 * still find our specified log (and we will error that the
                 * source exists elsewhere), or whether the log does not exist
                 * (and we will error accordingly).
                 */
                continue;
            }

            /*
             * We have found our source, and if we have specified a log,
             * the source is in the correct log.
             */
            SourceCreated = TRUE;

            /*
             * Check whether this is one of our custom sources
             * (application sources do not have this value present).
             */
            IsCustomSource = FALSE;

            lRet = RegQueryValueExW(hKeySource, L"CustomSource", NULL, NULL, NULL, NULL);

            /* Now get rid of the source key handle */
            RegCloseKey(hKeySource);
            hKeySource = NULL;

            if (lRet == ERROR_SUCCESS)
            {
                IsCustomSource = TRUE;
            }
            else if (lRet == ERROR_FILE_NOT_FOUND)
            {
                // IsCustomSource = FALSE;
            }
            else // if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND)
            {
                /* A registry error happened, just fail */
                goto Quit;
            }

            /*
             * We have found our source and it may be (or not) a custom source,
             * and it is in the correct event log (if we have specified one).
             * Break the search loop.
             */
            break; // goto Finalize;
        }
    }

    /*
     * No errors happened so far.
     * Perform last validity checks (the flags are all valid and 'LogName'
     * contains the name of the last log having been tested for).
     */
Finalize:
    lRet = ERROR_SUCCESS; // but do not set Success to TRUE yet.

    // FIXME: Shut up a GCC warning/error about 'SourceCreated' being unused.
    // We will use it later on.
    UNREFERENCED_PARAMETER(SourceCreated);

    /*
     * The source does not exist (SourceCreated == FALSE), create it.
     * Note that we then must have a specified log that exists on the system.
     */
    // NOTE: IsCustomSource always FALSE here.

    if (LogNameValid && !FoundLog)
    {
        /* We have specified a log but it does not exist! */
        ConResPrintf(StdErr, IDS_LOG_NOT_FOUND, EventLogName);
        goto Quit;
    }

    //
    // Here, LogNameValid == TRUE  && FoundLog == TRUE, or
    //       LogNameValid == FALSE && FoundLog == FALSE.
    //

    if (LogNameValid /* && FoundLog */ && !LogSourceValid /* && !FoundSource && !SourceAlreadyExists */)
    {
        /* No source, just use the log */
        // NOTE: For this case, SourceCreated and IsCustomSource were both set to TRUE.
        Success = TRUE;
        goto Quit;
    }

    if (/* LogSourceValid && */ FoundSource && SourceAlreadyExists)
    {
        /* The source is in another log than the specified one */
        ConResPrintf(StdErr, IDS_SOURCE_EXISTS, LogNameErr);
        goto Quit;
    }

    if (/* LogSourceValid && */ FoundSource && !SourceAlreadyExists)
    {
        /* We can directly use the source */

        // if (SourceCreated)
        {
            /* The source already exists, check whether this is a custom one */
            if (IsCustomSource || AllowAppSources)
            {
                /* This is a custom source, fine! */
                Success = TRUE;
                goto Quit;
            }
            else
            {
                /* This is NOT a custom source, we must return an error! */
                ConResPuts(StdErr, IDS_SOURCE_NOT_CUSTOM);
                goto Quit;
            }
        }
    }

    if (LogSourceValid && !FoundSource)
    {
        if (!LogNameValid /* && !FoundLog */)
        {
            /* The log name is not specified, we cannot create the source */
            ConResPuts(StdErr, IDS_SOURCE_NOCREATE);
            goto Quit;
        }
        else // LogNameValid && FoundLog
        {
            /* Create a new source in the specified log */

            lRet = RegOpenKeyExW(hEventLogKey,
                                 EventLogName,
                                 0, KEY_CREATE_SUB_KEY, // KEY_WRITE
                                 &hLogKey);
            if (lRet != ERROR_SUCCESS)
                goto Quit;

            /* Register the new event source */
            lRet = InstallEventSource(hLogKey, EventLogSource);

            RegCloseKey(hLogKey);

            if (lRet != ERROR_SUCCESS)
            {
                PrintError(lRet);
                ConPrintf(StdErr, L"Impossible to create the source `%s' for log `%s'!\n",
                          EventLogSource, EventLogName);
                goto Quit;
            }

            SourceCreated = TRUE;
            Success = TRUE;
        }
    }

Quit:
    if (hEventLogKey)
        RegCloseKey(hEventLogKey);

    SetLastError(lRet);

    return Success;
}


/**************************   P A R S E R    A P I   **************************/

enum TYPE
{
    TYPE_None = 0,
    TYPE_Str,
//  TYPE_U8,
//  TYPE_U16,
    TYPE_U32,
};

#define OPTION_ALLOWED_LIST 0x01
#define OPTION_NOT_EMPTY    0x02
#define OPTION_TRIM_SPACE   0x04
#define OPTION_EXCLUSIVE    0x08
#define OPTION_MANDATORY    0x10

typedef struct _OPTION
{
    /* Constant data */
    PWSTR OptionName;       // Option switch name
    ULONG Type;             // Type of data stored in the 'Value' member (UNUSED) (bool, string, int, ..., or function to call)
    ULONG Flags;            // Flags (preprocess the string or not, cache the string, stop processing...)
    ULONG MaxOfInstances;   // Maximum number of times this option can be seen in the command line (or 0: do not care)
    // PWSTR OptionHelp;       // Help string, or resource ID of the (localized) string (use the MAKEINTRESOURCE macro to create this value).
    // PVOID Callback() ??
    PWSTR AllowedValues;    // Optional list of allowed values, given as a string of values separated by a pipe symbol '|'.

    /* Parsing data */
    PWSTR OptionStr;        // Pointer to the original option string
    ULONG Instances;        // Number of times this option is seen in the command line
    ULONG ValueSize;        // Size of the buffer pointed by 'Value' ??
    PVOID Value;            // A pointer to part of the command line, or an allocated buffer
} OPTION, *POPTION;

#define NEW_OPT(Name, Type, Flags, MaxOfInstances, ValueSize, ValueBuffer) \
    {(Name), (Type), (Flags), (MaxOfInstances), NULL, NULL, 0, (ValueSize), (ValueBuffer)}

#define NEW_OPT_EX(Name, Type, Flags, AllowedValues, MaxOfInstances, ValueSize, ValueBuffer) \
    {(Name), (Type), (Flags), (MaxOfInstances), (AllowedValues), NULL, 0, (ValueSize), (ValueBuffer)}

static PWSTR
TrimLeftRightWhitespace(
    IN PWSTR String)
{
    PWSTR pStr;

    /* Trim whitespace on left (just advance the pointer) */
    while (*String && iswspace(*String))
        ++String;

    /* Trim whitespace on right (NULL-terminate) */
    pStr = String + wcslen(String) - 1;
    while (pStr >= String && iswspace(*pStr))
        --pStr;
    *++pStr = L'\0';

    /* Return the modified pointer */
    return String;
}

typedef enum _PARSER_ERROR
{
    Success = 0,
    InvalidSyntax,
    InvalidOption,
    ValueRequired,
    ValueIsEmpty,
    InvalidValue,
    ValueNotAllowed,
    TooManySameOption,
    MandatoryOptionAbsent,
} PARSER_ERROR;

typedef VOID (__cdecl *PRINT_ERROR_FUNC)(IN PARSER_ERROR, ...);

BOOL
DoParse(
    IN INT argc,
    IN WCHAR* argv[],
    IN OUT POPTION Options,
    IN ULONG NumOptions,
    IN PRINT_ERROR_FUNC PrintErrorFunc OPTIONAL)
{
    BOOL ExclusiveOptionPresent = FALSE;
    PWSTR OptionStr = NULL;
    UINT i;

    /*
     * The 'Option' index is reset to 'NumOptions' (total number of elements in
     * the 'Options' list) before retrieving a new option. This is done so that
     * we know it cannot index a valid option at that moment.
     */
    UINT Option = NumOptions;

    /* Parse command line for options */
    for (i = 1; i < argc; ++i)
    {
        /* Check for new options */

        if (argv[i][0] == L'-' || argv[i][0] == L'/')
        {
            /// FIXME: This test is problematic if this concerns the last option in the command-line!
            /// A hack-fix is to repeat this check after the 'for'-loop.
            if (Option != NumOptions)
            {
                if (PrintErrorFunc)
                    PrintErrorFunc(ValueRequired, OptionStr);
                return FALSE;
            }

            /*
             * If we have already encountered an (unique) exclusive option,
             * just break now.
             */
            if (ExclusiveOptionPresent)
                break;

            OptionStr = argv[i];

            /* Lookup for the option in the list of options */
            for (Option = 0; Option < NumOptions; ++Option)
            {
                if (_wcsicmp(OptionStr + 1, Options[Option].OptionName) == 0)
                    break;
            }

            if (Option >= NumOptions)
            {
                if (PrintErrorFunc)
                    PrintErrorFunc(InvalidOption, OptionStr);
                return FALSE;
            }


            /* An option is being set */

            if (Options[Option].MaxOfInstances != 0 &&
                Options[Option].Instances >= Options[Option].MaxOfInstances)
            {
                if (PrintErrorFunc)
                    PrintErrorFunc(TooManySameOption, OptionStr, Options[Option].MaxOfInstances);
                return FALSE;
            }
            ++Options[Option].Instances;

            Options[Option].OptionStr = OptionStr;

            /*
             * If this option is exclusive, remember it for later.
             * We will then short-circuit the regular validity checks
             * and instead check whether this is the only option specified
             * on the command-line.
             */
            if (Options[Option].Flags & OPTION_EXCLUSIVE)
                ExclusiveOptionPresent = TRUE;

            /* Preprocess the option before setting its value */
            switch (Options[Option].Type)
            {
                case TYPE_None: // ~= TYPE_Bool
                {
                    /* Set the associated boolean */
                    BOOL* pBool = (BOOL*)Options[Option].Value;
                    *pBool = TRUE;

                    /* No associated value, so reset the index */
                    Option = NumOptions;
                }

                /* Fall-back */

                case TYPE_Str:

                // case TYPE_U8:
                // case TYPE_U16:
                case TYPE_U32:
                    break;

                default:
                {
                    wprintf(L"PARSER: Unsupported option type %lu\n", Options[Option].Type);
                    break;
                }
            }
        }
        else
        {
            /* A value for an option is being set */
            switch (Options[Option].Type)
            {
                case TYPE_None:
                {
                    /* There must be no associated value */
                    if (PrintErrorFunc)
                        PrintErrorFunc(ValueNotAllowed, OptionStr);
                    return FALSE;
                }

                case TYPE_Str:
                {
                    /* Retrieve the string */
                    PWSTR* pStr = (PWSTR*)Options[Option].Value;
                    *pStr = argv[i];

                    /* Trim whitespace if needed */
                    if (Options[Option].Flags & OPTION_TRIM_SPACE)
                        *pStr = TrimLeftRightWhitespace(*pStr);

                    /* Check whether or not the value can be empty */
                    if ((Options[Option].Flags & OPTION_NOT_EMPTY) && !**pStr)
                    {
                        /* Value cannot be empty */
                        if (PrintErrorFunc)
                            PrintErrorFunc(ValueIsEmpty, OptionStr);
                        return FALSE;
                    }

                    /* Check whether the value is part of the allowed list of values */
                    if (Options[Option].Flags & OPTION_ALLOWED_LIST)
                    {
                        PWSTR AllowedValues, Scan;
                        SIZE_T Length;

                        AllowedValues = Options[Option].AllowedValues;
                        if (!AllowedValues)
                        {
                            /* The array is empty, no allowed values */
                            if (PrintErrorFunc)
                                PrintErrorFunc(InvalidValue, *pStr, OptionStr);
                            return FALSE;
                        }

                        Scan = AllowedValues;
                        while (*Scan)
                        {
                            /* Find the values separator */
                            Length = wcscspn(Scan, L"|");

                            /* Check whether this is an allowed value */
                            if ((wcslen(*pStr) == Length) &&
                                (_wcsnicmp(*pStr, Scan, Length) == 0))
                            {
                                /* Found it! */
                                break;
                            }

                            /* Go to the next test value */
                            Scan += Length;
                            if (*Scan) ++Scan; // Skip the separator
                        }

                        if (!*Scan)
                        {
                            /* The value is not allowed */
                            if (PrintErrorFunc)
                                PrintErrorFunc(InvalidValue, *pStr, OptionStr);
                            return FALSE;
                        }
                    }

                    break;
                }

                // case TYPE_U8:
                // case TYPE_U16:
                case TYPE_U32:
                {
                    PWCHAR pszNext = NULL;

                    /* The number is specified in base 10 */
                    // NOTE: We might use '0' so that the base is automatically determined.
                    *(ULONG*)Options[Option].Value = wcstoul(argv[i], &pszNext, 10);
                    if (*pszNext)
                    {
                        /* The value is not a valid numeric value and is not allowed */
                        if (PrintErrorFunc)
                            PrintErrorFunc(InvalidValue, argv[i], OptionStr);
                        return FALSE;
                    }
                    break;
                }

                default:
                {
                    wprintf(L"PARSER: Unsupported option type %lu\n", Options[Option].Type);
                    break;
                }
            }

            /* Reset the index */
            Option = NumOptions;
        }
    }

    /// HACK-fix for the check done inside the 'for'-loop.
    if (Option != NumOptions)
    {
        if (PrintErrorFunc)
            PrintErrorFunc(ValueRequired, OptionStr);
        return FALSE;
    }

    /* Finalize options validity checks */

    if (ExclusiveOptionPresent)
    {
        /*
         * An exclusive option present on the command-line:
         * check whether this is the only option specified.
         */
        for (i = 0; i < NumOptions; ++i)
        {
            if (!(Options[i].Flags & OPTION_EXCLUSIVE) && (Options[i].Instances != 0))
            {
                /* A non-exclusive option is present on the command-line, fail */
                if (PrintErrorFunc)
                    PrintErrorFunc(InvalidSyntax);
                return FALSE;
            }
        }

        /* No other checks needed, we are done */
        return TRUE;
    }

    /* Check whether the required options were specified */
    for (i = 0; i < NumOptions; ++i)
    {
        /* Regular validity checks */
        if ((Options[i].Flags & OPTION_MANDATORY) && (Options[i].Instances == 0))
        {
            if (PrintErrorFunc)
                PrintErrorFunc(MandatoryOptionAbsent, Options[i].OptionName);
            return FALSE;
        }
    }

    /* All checks are done */
    return TRUE;
}

/******************************************************************************/


static VOID
__cdecl
PrintParserError(PARSER_ERROR Error, ...)
{
    /* WARNING: Please keep this lookup table in sync with the resources! */
    static UINT ErrorIDs[] =
    {
        0,                  /* Success */
        IDS_BADSYNTAX_0,    /* InvalidSyntax */
        IDS_INVALIDSWITCH,  /* InvalidOption */
        IDS_BADSYNTAX_1,    /* ValueRequired */
        IDS_BADSYNTAX_2,    /* ValueIsEmpty */
        IDS_BADSYNTAX_3,    /* InvalidValue */
        IDS_BADSYNTAX_4,    /* ValueNotAllowed */
        IDS_BADSYNTAX_5,    /* TooManySameOption */
        IDS_BADSYNTAX_6,    /* MandatoryOptionAbsent */
    };

    va_list args;

    if (Error < ARRAYSIZE(ErrorIDs))
    {
        va_start(args, Error);
        ConResPrintfV(StdErr, ErrorIDs[Error], args);
        va_end(args);

        if (Error != Success)
            ConResPuts(StdErr, IDS_USAGE);
    }
    else
    {
        ConPrintf(StdErr, L"PARSER: Unknown error %d\n", Error);
    }
}

int wmain(int argc, WCHAR* argv[])
{
    BOOL Success = FALSE;
    HANDLE hEventLog;
    PTOKEN_USER pUserToken;

    /* Default option values */
    BOOL  bDisplayHelp  = FALSE;
    PWSTR szSystem      = NULL;
    PWSTR szDomainUser  = NULL;
    PWSTR szPassword    = NULL;
    PWSTR szLogName     = NULL;
    PWSTR szEventSource = NULL;
    PWSTR szEventType   = NULL;
    PWSTR szDescription = NULL;
    ULONG ulEventType   = EVENTLOG_INFORMATION_TYPE;
    ULONG ulEventCategory   = 0;
    ULONG ulEventIdentifier = 0;

    OPTION Options[] =
    {
        /* Help */
        NEW_OPT(L"?", TYPE_None, // ~= TYPE_Bool,
                OPTION_EXCLUSIVE,
                1,
                sizeof(bDisplayHelp), &bDisplayHelp),

        /* System */
        NEW_OPT(L"S", TYPE_Str,
                OPTION_NOT_EMPTY | OPTION_TRIM_SPACE,
                1,
                sizeof(szSystem), &szSystem),

        /* Domain & User */
        NEW_OPT(L"U", TYPE_Str,
                OPTION_NOT_EMPTY | OPTION_TRIM_SPACE,
                1,
                sizeof(szDomainUser), &szDomainUser),

        /* Password */
        NEW_OPT(L"P", TYPE_Str,
                0,
                1,
                sizeof(szPassword), &szPassword),

        /* Log name */
        NEW_OPT(L"L", TYPE_Str,
                OPTION_NOT_EMPTY | OPTION_TRIM_SPACE,
                1,
                sizeof(szLogName), &szLogName),

        /* Event source */
        NEW_OPT(L"SO", TYPE_Str,
                OPTION_NOT_EMPTY | OPTION_TRIM_SPACE,
                1,
                sizeof(szEventSource), &szEventSource),

        /* Event type */
        NEW_OPT_EX(L"T", TYPE_Str,
                   OPTION_MANDATORY | OPTION_NOT_EMPTY | OPTION_TRIM_SPACE | OPTION_ALLOWED_LIST,
                   L"SUCCESS|ERROR|WARNING|INFORMATION",
                   1,
                   sizeof(szEventType), &szEventType),

        /* Event category (ReactOS additional option) */
        NEW_OPT(L"C", TYPE_U32,
                0,
                1,
                sizeof(ulEventCategory), &ulEventCategory),

        /* Event ID */
        NEW_OPT(L"ID", TYPE_U32,
                OPTION_MANDATORY,
                1,
                sizeof(ulEventIdentifier), &ulEventIdentifier),

        /* Event description */
        NEW_OPT(L"D", TYPE_Str,
                OPTION_MANDATORY,
                1,
                sizeof(szDescription), &szDescription),
    };
#define OPT_SYSTEM  (Options[1])
#define OPT_USER    (Options[2])
#define OPT_PASSWD  (Options[3])
#define OPT_EVTID   (Options[8])

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Parse command line for options */
    if (!DoParse(argc, argv, Options, ARRAYSIZE(Options), PrintParserError))
        return EXIT_FAILURE;

    /* Finalize options validity checks */

    if (bDisplayHelp)
    {
        if (argc > 2)
        {
            /* Invalid syntax */
            PrintParserError(InvalidSyntax);
            return EXIT_FAILURE;
        }

        ConResPuts(StdOut, IDS_HELP);
        return EXIT_SUCCESS;
    }

    if (szSystem || szDomainUser || szPassword)
    {
        // TODO: Implement!
        if (szSystem)
            ConResPrintf(StdOut, IDS_SWITCH_UNIMPLEMENTED, OPT_SYSTEM.OptionStr);
        if (szDomainUser)
            ConResPrintf(StdOut, IDS_SWITCH_UNIMPLEMENTED, OPT_USER.OptionStr);
        if (szPassword)
            ConResPrintf(StdOut, IDS_SWITCH_UNIMPLEMENTED, OPT_PASSWD.OptionStr);
        return EXIT_FAILURE;
    }

    if (ulEventIdentifier < EVENT_ID_MIN || ulEventIdentifier > EVENT_ID_MAX)
    {
        /* Invalid event identifier */
        ConResPrintf(StdErr, IDS_BADSYNTAX_7, OPT_EVTID.OptionStr, EVENT_ID_MIN, EVENT_ID_MAX);
        ConResPuts(StdErr, IDS_USAGE);
        return EXIT_FAILURE;
    }

    /*
     * Set the event type. Note that we forbid the user
     * to use security auditing types.
     */
    if (_wcsicmp(szEventType, L"SUCCESS") == 0)
        ulEventType = EVENTLOG_SUCCESS;
    else
    if (_wcsicmp(szEventType, L"ERROR") == 0)
        ulEventType = EVENTLOG_ERROR_TYPE;
    else
    if (_wcsicmp(szEventType, L"WARNING") == 0)
        ulEventType = EVENTLOG_WARNING_TYPE;
    else
    if (_wcsicmp(szEventType, L"INFORMATION") == 0)
        ulEventType = EVENTLOG_INFORMATION_TYPE;
    else
    {
        /* Use a default event type */
        ulEventType = EVENTLOG_SUCCESS;
    }

    /*
     * If we have a source, do not care about the log (as long as we will be
     * able to find the source later).
     * But if we do not have a source, then two cases:
     * - either we have a log name so that we will use OpenEventLog (and use
     *   default log's source), unless this is the Application log in which case
     *   we use the default source;
     * - or we do not have a log name so that we use default log and source names.
     */
    if (!szEventSource)
    {
        if (!szLogName)
            szLogName = L"Application";

        if (_wcsicmp(szLogName, L"Application") == 0)
            szEventSource = APPLICATION_NAME;
    }

    // FIXME: Check whether szLogName == L"Security" !!

    /*
     * The event APIs OpenEventLog and RegisterEventSource fall back to using
     * the 'Application' log when the specified log name or event source do not
     * exist on the system.
     * To prevent that and be able to error the user that the specified log name
     * or event source do not exist, we need to manually perform the existence
     * checks by ourselves.
     *
     * Check whether either the specified event log OR event source exist on
     * the system. If the event log does not exist, return an error. Otherwise
     * check whether a specified source already exists (everywhere). If found
     * in a different log, return an error. If not found, create the source
     * in the specified event log.
     *
     * NOTE: By default we forbid the usage of application (non-custom) sources.
     * An optional switch can be added to EventCreate to allow such sources
     * to be used.
     */
    if (!CheckLogOrSourceExistence(szSystem, szLogName, szEventSource, FALSE))
    {
        PrintError(GetLastError());
        return EXIT_FAILURE;
    }

    /* Open the event log, by source or by log name */
    if (szEventSource) // && *szEventSource
        hEventLog = RegisterEventSourceW(szSystem, szEventSource);
    else
        hEventLog = OpenEventLogW(szSystem, szLogName);

    if (!hEventLog)
    {
        PrintError(GetLastError());
        return EXIT_FAILURE;
    }

    /* Retrieve the current user token and report the event */
    if (GetUserToken(&pUserToken))
    {
        Success = ReportEventW(hEventLog,
                               ulEventType,
                               ulEventCategory,
                               ulEventIdentifier,
                               pUserToken->User.Sid,
                               1,   // One string
                               0,   // No raw data
                               (LPCWSTR*)&szDescription,
                               NULL // No raw data
                               );
        if (!Success)
        {
            PrintError(GetLastError());
            ConPuts(StdErr, L"Failed to report event!\n");
        }
        else
        {
            /* Show success */
            ConPuts(StdOut, L"\n");
            if (!szEventSource)
                ConResPrintf(StdOut, IDS_SUCCESS_1, szEventType, szLogName);
            else if (!szLogName)
                ConResPrintf(StdOut, IDS_SUCCESS_2, szEventType, szEventSource);
            else
                ConResPrintf(StdOut, IDS_SUCCESS_3, szEventType, szLogName, szEventSource);
        }

        HeapFree(GetProcessHeap(), 0, pUserToken);
    }
    else
    {
        PrintError(GetLastError());
        ConPuts(StdErr, L"GetUserToken() failed!\n");
    }

    /* Close the event log */
    if (szEventSource && *szEventSource)
        DeregisterEventSource(hEventLog);
    else
        CloseEventLog(hEventLog);

    return (Success ? EXIT_SUCCESS : EXIT_FAILURE);
}
