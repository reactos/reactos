/*
 * PROJECT:     ReactOS Whoami
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/cmdutils/whoami/whoami.c
 * PURPOSE:     Displays information about the current local user, groups and privileges.
 * PROGRAMMERS: Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 */

#define SECURITY_WIN32
#include <security.h>
#include <sddl.h>
#include <strsafe.h>

#include <conutils.h>

#include "resource.h"

#define wprintf(...) ConPrintf(StdOut, ##__VA_ARGS__)

BOOL NoHeader = FALSE;
UINT NoHeaderArgCount = 0;
UINT PrintFormatArgCount = 0;

enum
{
    undefined,
    table,
    list,
    csv
} PrintFormat = undefined;


BOOL GetArgument(WCHAR* arg, int argc, WCHAR* argv[])
{
    int i;

    if (!arg)
        return FALSE;

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], arg) == 0)
            return TRUE;
    }

    return FALSE;
}

/* blanking out the accepted modifiers will make filtering easier later on */
void BlankArgument(int argc, WCHAR* argv[])
{
    argv[argc] = L"";
}

/* helper functions; let's keep it tidy to avoid redundancies */

LPWSTR WhoamiGetUser(EXTENDED_NAME_FORMAT NameFormat)
{
    LPWSTR UsrBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
    ULONG UsrSiz = MAX_PATH;

    if (UsrBuf == NULL)
        return NULL;

    if (GetUserNameExW(NameFormat, UsrBuf, &UsrSiz))
    {
        CharLowerW(UsrBuf);
        return UsrBuf;
    }

    HeapFree(GetProcessHeap(), 0, UsrBuf);
    return NULL;
}

BOOL WhoamiFree(VOID* Buffer)
{
    return HeapFree(GetProcessHeap(), 0, Buffer);
}


VOID* WhoamiGetTokenInfo(TOKEN_INFORMATION_CLASS TokenType)
{
    HANDLE hToken = 0;
    DWORD dwLength = 0;
    VOID* pTokenInfo = 0;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
    {
        GetTokenInformation(hToken,
                            TokenType,
                            NULL,
                            dwLength,
                            &dwLength);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pTokenInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
            if (pTokenInfo == NULL)
            {
                wprintf(L"ERROR: not enough memory to allocate the token structure.\n");
                exit(1);
            }
        }

        if (!GetTokenInformation(hToken, TokenType,
                                 (LPVOID)pTokenInfo,
                                 dwLength,
                                 &dwLength))
        {
            wprintf(L"ERROR 0x%x: could not get token information.\n", GetLastError());
            WhoamiFree(pTokenInfo);
            exit(1);
        }

        CloseHandle(hToken);
    }

    return pTokenInfo;
}

LPWSTR WhoamiLoadRcString(INT ResId)
{
    #define RC_STRING_MAX_SIZE 850
    static WCHAR TmpBuffer[RC_STRING_MAX_SIZE];

    LoadStringW(GetModuleHandleW(NULL), ResId, TmpBuffer, RC_STRING_MAX_SIZE);

    return TmpBuffer;
}

void WhoamiPrintHeader(int HeaderId)
{
    PWSTR Header = WhoamiLoadRcString(HeaderId);
    DWORD Length = wcslen(Header);

    if (NoHeader || PrintFormat == csv)
        return;

    wprintf(L"\n%s\n", Header);

    while (Length--)
        wprintf(L"-");

    wprintf(L"\n\n");
}

typedef struct
{
    UINT Rows;
    UINT Cols;
    LPWSTR Content[1];
} WhoamiTable;

/* create and prepare a new table for printing */
WhoamiTable *WhoamiAllocTable(UINT Rows, UINT Cols)
{
    WhoamiTable *pTable = HeapAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    sizeof(WhoamiTable) + sizeof(LPWSTR) * Rows * Cols);

    // wprintf(L"DEBUG: Allocating %dx%d elem table for printing.\n\n", Rows, Cols);

    if (!pTable)
    {
        wprintf(L"ERROR: Not enough memory for displaying the table.\n");
        exit(1);
    }

    pTable->Rows = Rows;
    pTable->Cols = Cols;

    return pTable;
}

/* allocate and fill a new entry in the table */
void WhoamiSetTable(WhoamiTable *pTable, WCHAR *Entry, UINT Row, UINT Col)
{
    LPWSTR Target = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              1 + wcslen(Entry) * sizeof(Entry[0]));

    // wprintf(L"DEBUG: Setting table value '%lp' '%ls' for %lu %lu.\n", entry, entry, row, col);

    if (!Target)
        exit(1);

    wcscpy(Target, Entry);

    pTable->Content[Row * pTable->Cols + Col] = Target;
}

/* fill a new entry in the table */
void WhoamiSetTableDyn(WhoamiTable *pTable, WCHAR *Entry, UINT Row, UINT Col)
{
    pTable->Content[Row * pTable->Cols + Col] = Entry;
}

/* print and deallocate the table */
void WhoamiPrintTable(WhoamiTable *pTable)
{
    UINT i, j;
    UINT CurRow, CurCol;
    UINT *ColLength;


    if (!pTable)
    {
        wprintf(L"ERROR: The table passed for display is empty.\n");
        exit(1);
    }

    /* if we are going to print a *list* or *table*; take note of the total
       column size, as we will need it later on when printing them in a tabular
       fashion, according to their windows counterparts */

    if (PrintFormat != csv)
    {
        ColLength = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UINT) * pTable->Cols);

        if (PrintFormat == list)
        {
            for (j = 0; j < pTable->Cols; j++)
                if (pTable->Content[j])
                {
                    UINT ThisLength = wcslen(pTable->Content[j]);

                    /* now that we're here, seize the opportunity and add those pesky ":" */
                    pTable->Content[j][ThisLength++] = L':';
                    pTable->Content[j][ThisLength] = UNICODE_NULL;

                    ColLength[0] = max(ThisLength, ColLength[0]);
                }
        }
        else
        {
            for (j = 0; j < pTable->Cols; j++)
                for (i = 0; i < pTable->Rows; i++)
                    if (pTable->Content[i * pTable->Cols + j])
                    {
                        UINT ThisLength = wcslen(pTable->Content[i * pTable->Cols + j]);
                        ColLength[j] = max(ThisLength, ColLength[j]);
                    }
        }
    }

    switch (PrintFormat)
    {
        case csv:
        {
            for (i = 0; i < pTable->Rows; i++)
            {
                if (!pTable->Content[i * pTable->Cols])
                    continue;

                /* if the user especified /nh then skip the column labels */
                if (NoHeader && i == 0)
                    continue;

                for (j = 0; j < pTable->Cols; j++)
                {
                    if (pTable->Content[i * pTable->Cols + j])
                    {
                        wprintf(L"\"%s\"%s",
                                pTable->Content[i * pTable->Cols + j],
                                (j+1 < pTable->Cols ? L"," : L""));
                    }
                }
                wprintf(L"\n");
            }

            break;

        }

        case list:
        {
            UINT FinalRow = 0;

            /* fixme: we need to do two passes to find out which entry is the last one shown, or not null this is not exactly optimal */
            for (CurRow = 1; CurRow < pTable->Rows; CurRow++)
            {
                /* if the first member of this row isn't available, then forget it */
                if (!pTable->Content[CurRow * pTable->Cols])
                    continue;

                FinalRow = CurRow;
            }

            for (CurRow = 1; CurRow < pTable->Rows; CurRow++)
            {
                /* if the first member of this row isn't available, then forget it */
                if (!pTable->Content[CurRow * pTable->Cols])
                    continue;

                /* if the user especified /nh then skip the column labels */
                if (NoHeader && i == 0)
                    continue;

                for (CurCol = 0; CurCol < pTable->Cols; CurCol++)
                {
                    wprintf(L"%-*s %s\n",
                            ColLength[0],
                            pTable->Content[CurCol],
                            pTable->Content[CurRow * pTable->Cols + CurCol]);
                }

                /* don't add two carriage returns at the very end */
                if (CurRow != FinalRow)
                    wprintf(L"\n");
            }

            break;
        }


        case table:
        default:
        {
            for (i = 0; i < pTable->Rows; i++)
            {
                /* if the first member of this row isn't available, then forget it */
                if (!pTable->Content[i * pTable->Cols])
                    continue;

                /* if the user especified /nh then skip the column labels too */
                if (NoHeader && i == 0)
                    continue;

                for (j = 0; j < pTable->Cols; j++)
                {
                    if (pTable->Content[i * pTable->Cols + j])
                    {
                        wprintf(L"%-*s ", ColLength[j], pTable->Content[i * pTable->Cols + j]);
                    }
                }
                wprintf(L"\n");

                /* add the cute underline thingie for the table header */
                if (i == 0)
                {
                    for (j = 0; j < pTable->Cols; j++)
                    {
                        DWORD Length = ColLength[j];

                        while (Length--)
                            wprintf(L"=");

                        /* a spacing between all the columns except for the last one */
                        if (pTable->Cols != (i + 1))
                            wprintf(L" ");
                    }

                    wprintf(L"\n");
                }
            }

        }
    }

    /* fixme: when many tables are displayed in a single run we
              have to sandwich carriage returns in between. */
    // if (!final_entry)
        wprintf(L"\n");

    for (i = 0; i < pTable->Rows; i++)
        for (j = 0; j < pTable->Cols; j++)
            WhoamiFree(pTable->Content[i * pTable->Cols + j]);

    WhoamiFree(pTable);

    if (PrintFormat != csv)
        HeapFree(GetProcessHeap(), 0, ColLength);
}

int WhoamiLogonId(void)
{
    PTOKEN_GROUPS pGroupInfo = (PTOKEN_GROUPS) WhoamiGetTokenInfo(TokenGroups);
    DWORD dwIndex = 0;
    LPWSTR pSidStr = 0;
    PSID pSid = 0;

    if (pGroupInfo == NULL)
        return 0;

    /* lets see if we can find the logon SID in that list, should be there */
    for (dwIndex = 0; dwIndex < pGroupInfo->GroupCount; dwIndex++)
    {
        if ((pGroupInfo->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
        {
            pSid = pGroupInfo->Groups[dwIndex].Sid;
        }
    }

    if (pSid == 0)
    {
        WhoamiFree(pGroupInfo);
        wprintf(L"ERROR: Couldn't find the logon SID.\n");
        return 1;
    }
    if (!ConvertSidToStringSidW(pSid, &pSidStr))
    {
        WhoamiFree(pGroupInfo);
        wprintf(L"ERROR: Couldn't convert the logon SID to a string.\n");
        return 1;
    }
    else
    {
        /* let's show our converted logon SID */
        wprintf(L"%s\n", pSidStr);
    }

    /* cleanup our allocations */
    LocalFree(pSidStr);
    WhoamiFree(pGroupInfo);

    return 0;
}

int WhoamiUser(void)
{
    PTOKEN_USER pUserInfo = (PTOKEN_USER) WhoamiGetTokenInfo(TokenUser);
    LPWSTR pUserStr = NULL;
    LPWSTR pSidStr = NULL;
    WhoamiTable *UserTable = NULL;

    if (pUserInfo == NULL)
    {
        return 1;
    }

    pUserStr = WhoamiGetUser(NameSamCompatible);
    if (pUserStr == NULL)
    {
        WhoamiFree(pUserInfo);
        return 1;
    }

    UserTable = WhoamiAllocTable(2, 2);

    WhoamiPrintHeader(IDS_USER_HEADER);

    /* set the column labels */
    WhoamiSetTable(UserTable, WhoamiLoadRcString(IDS_COL_USER_NAME), 0, 0);
    WhoamiSetTable(UserTable, WhoamiLoadRcString(IDS_COL_SID), 0, 1);

    ConvertSidToStringSidW(pUserInfo->User.Sid, &pSidStr);

    /* set the values for our single row of data */
    WhoamiSetTable(UserTable, pUserStr, 1, 0);
    WhoamiSetTable(UserTable, pSidStr, 1, 1);

    WhoamiPrintTable(UserTable);

    /* cleanup our allocations */
    LocalFree(pSidStr);
    WhoamiFree(pUserInfo);
    WhoamiFree(pUserStr);

    return 0;
}

int WhoamiGroups(void)
{
    DWORD dwIndex = 0;
    LPWSTR pSidStr = 0;

    static WCHAR szGroupName[255] = {0};
    static WCHAR szDomainName[255] = {0};

    DWORD cchGroupName  = _countof(szGroupName);
    DWORD cchDomainName = _countof(szGroupName);

    SID_NAME_USE Use = 0;
    BYTE SidNameUseStr[12] =
    {
        /* SidTypeUser           */ -1,
        /* SidTypeGroup          */ -1,
        /* SidTypeDomain         */ -1,
        /* SidTypeUser           */ -1,
        /* SidTypeAlias          */ IDS_TP_ALIAS,
        /* SidTypeWellKnownGroup */ IDS_TP_WELL_KNOWN_GROUP,
        /* SidTypeDeletedAccount */ -1,
        /* SidTypeInvalid        */ -1,
        /* SidTypeUnknown        */ -1,
        /* SidTypeComputer       */ -1,
        /* SidTypeLabel          */ IDS_TP_LABEL
    };

    PTOKEN_GROUPS pGroupInfo = (PTOKEN_GROUPS)WhoamiGetTokenInfo(TokenGroups);
    UINT PrintingRow;
    WhoamiTable *GroupTable = NULL;

    if (pGroupInfo == NULL)
    {
        return 1;
    }

    /* the header is the first (0) row, so we start in the second one (1) */
    PrintingRow = 1;

    GroupTable = WhoamiAllocTable(pGroupInfo->GroupCount + 1, 4);

    WhoamiPrintHeader(IDS_GROU_HEADER);

    WhoamiSetTable(GroupTable, WhoamiLoadRcString(IDS_COL_GROUP_NAME), 0, 0);
    WhoamiSetTable(GroupTable, WhoamiLoadRcString(IDS_COL_TYPE), 0, 1);
    WhoamiSetTable(GroupTable, WhoamiLoadRcString(IDS_COL_SID), 0, 2);
    WhoamiSetTable(GroupTable, WhoamiLoadRcString(IDS_COL_ATTRIB), 0, 3);

    for (dwIndex = 0; dwIndex < pGroupInfo->GroupCount; dwIndex++)
    {
        LookupAccountSidW(NULL,
                          pGroupInfo->Groups[dwIndex].Sid,
                          (LPWSTR)&szGroupName,
                          &cchGroupName,
                          (LPWSTR)&szDomainName,
                          &cchDomainName,
                          &Use);

        /* the original tool seems to limit the list to these kind of SID items */
        if ((Use == SidTypeWellKnownGroup || Use == SidTypeAlias ||
            Use == SidTypeLabel) && !(pGroupInfo->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID))
        {
                wchar_t tmpBuffer[666];

            /* looks like windows treats 0x60 as 0x7 for some reason, let's just nod and call it a day:
               0x60 is SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED
               0x07 is SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED */

            if (pGroupInfo->Groups[dwIndex].Attributes == 0x60)
                pGroupInfo->Groups[dwIndex].Attributes = 0x07;

            /* 1- format it as DOMAIN\GROUP if the domain exists, or just GROUP if not */
            _snwprintf((LPWSTR)&tmpBuffer,
                       _countof(tmpBuffer),
                       L"%s%s%s",
                       szDomainName,
                       cchDomainName ? L"\\" : L"",
                       szGroupName);

            WhoamiSetTable(GroupTable, tmpBuffer, PrintingRow, 0);

            /* 2- let's find out the group type by using a simple lookup table for lack of a better method */
            WhoamiSetTable(GroupTable, WhoamiLoadRcString(SidNameUseStr[Use]), PrintingRow, 1);

            /* 3- turn that SID into text-form */
            ConvertSidToStringSidW(pGroupInfo->Groups[dwIndex].Sid, &pSidStr);

            WhoamiSetTable(GroupTable, pSidStr, PrintingRow, 2);

            LocalFree(pSidStr);

            /* 4- reuse that buffer for appending the attributes in text-form at the very end */
            ZeroMemory(tmpBuffer, sizeof(tmpBuffer));

            if (pGroupInfo->Groups[dwIndex].Attributes & SE_GROUP_MANDATORY)
                StringCchCat(tmpBuffer, _countof(tmpBuffer), WhoamiLoadRcString(IDS_ATTR_GROUP_MANDATORY));
            if (pGroupInfo->Groups[dwIndex].Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
                StringCchCat(tmpBuffer, _countof(tmpBuffer), WhoamiLoadRcString(IDS_ATTR_GROUP_ENABLED_BY_DEFAULT));
            if (pGroupInfo->Groups[dwIndex].Attributes & SE_GROUP_ENABLED)
                StringCchCat(tmpBuffer, _countof(tmpBuffer), WhoamiLoadRcString(IDS_ATTR_GROUP_ENABLED));
            if (pGroupInfo->Groups[dwIndex].Attributes & SE_GROUP_OWNER)
                StringCchCat(tmpBuffer, _countof(tmpBuffer), WhoamiLoadRcString(IDS_ATTR_GROUP_OWNER));

            /* remove the last comma (', ' which is 2 wchars) of the buffer, let's keep it simple */
            tmpBuffer[max(wcslen(tmpBuffer) - 2, 0)] = UNICODE_NULL;

            WhoamiSetTable(GroupTable, tmpBuffer, PrintingRow, 3);

            PrintingRow++;
        }

        /* reset the buffers so that we can reuse them */
        ZeroMemory(szGroupName, sizeof(szGroupName));
        ZeroMemory(szDomainName, sizeof(szDomainName));

        cchGroupName = 255;
        cchDomainName = 255;
    }

    WhoamiPrintTable(GroupTable);

    /* cleanup our allocations */
    WhoamiFree((LPVOID)pGroupInfo);

    return 0;
}

int WhoamiPriv(void)
{
    PTOKEN_PRIVILEGES pPrivInfo = (PTOKEN_PRIVILEGES) WhoamiGetTokenInfo(TokenPrivileges);
    DWORD dwResult = 0, dwIndex = 0;
    WhoamiTable *PrivTable = NULL;

    if (pPrivInfo == NULL)
    {
        return 1;
    }

    PrivTable = WhoamiAllocTable(pPrivInfo->PrivilegeCount + 1, 3);

    WhoamiPrintHeader(IDS_PRIV_HEADER);

    WhoamiSetTable(PrivTable, WhoamiLoadRcString(IDS_COL_PRIV_NAME), 0, 0);
    WhoamiSetTable(PrivTable, WhoamiLoadRcString(IDS_COL_DESCRIPTION), 0, 1);
    WhoamiSetTable(PrivTable, WhoamiLoadRcString(IDS_COL_STATE), 0, 2);

    for (dwIndex = 0; dwIndex < pPrivInfo->PrivilegeCount; dwIndex++)
    {
        PWSTR PrivName = NULL, DispName = NULL;
        DWORD PrivNameSize = 0, DispNameSize = 0;
        BOOL ret = FALSE;

        ret = LookupPrivilegeNameW(NULL,
                                   &pPrivInfo->Privileges[dwIndex].Luid,
                                   NULL,
                                   &PrivNameSize);

        PrivName = HeapAlloc(GetProcessHeap(), 0, ++PrivNameSize*sizeof(WCHAR));

        LookupPrivilegeNameW(NULL,
                             &pPrivInfo->Privileges[dwIndex].Luid,
                             PrivName,
                             &PrivNameSize);

        WhoamiSetTableDyn(PrivTable, PrivName, dwIndex + 1, 0);


        /* try to grab the size of the string, also, beware, as this call is
           unimplemented in ReactOS/Wine at the moment */

        LookupPrivilegeDisplayNameW(NULL, PrivName, NULL, &DispNameSize, &dwResult);

        DispName = HeapAlloc(GetProcessHeap(), 0, ++DispNameSize * sizeof(WCHAR));

        ret = LookupPrivilegeDisplayNameW(NULL, PrivName, DispName, &DispNameSize, &dwResult);

        if (ret && DispName)
        {
            // wprintf(L"DispName: %d %x '%s'\n", DispNameSize, GetLastError(), DispName);
            WhoamiSetTableDyn(PrivTable, DispName, dwIndex + 1, 1);
        }
        else
        {
            WhoamiSetTable(PrivTable, WhoamiLoadRcString(IDS_UNKNOWN_DESCRIPTION), dwIndex + 1, 1);

            if (DispName != NULL)
                WhoamiFree(DispName);
        }

        if (pPrivInfo->Privileges[dwIndex].Attributes & SE_PRIVILEGE_ENABLED)
            WhoamiSetTable(PrivTable, WhoamiLoadRcString(IDS_STATE_ENABLED),  dwIndex + 1, 2);
        else
            WhoamiSetTable(PrivTable, WhoamiLoadRcString(IDS_STATE_DISABLED), dwIndex + 1, 2);
    }

    WhoamiPrintTable(PrivTable);

    /* cleanup our allocations */
    WhoamiFree(pPrivInfo);

    return 0;
}

int wmain(int argc, WCHAR* argv[])
{
    #define WAM_USER   1<<0
    #define WAM_GROUPS 1<<1
    #define WAM_PRIV   1<<2

    INT i;
    BYTE WamBit = 0;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();


    /* * * * * * * * * * * * * * * *
     * A: no parameters whatsoever */

    if (argc == 1)
    {
        /* if there's no arguments just choose the simple path and display the user's identity in lowercase */
        LPWSTR UserBuffer = WhoamiGetUser(NameSamCompatible);

        if (UserBuffer)
        {
            wprintf(L"%s\n", UserBuffer);
            WhoamiFree(UserBuffer);
            return 0;
        }
        else
        {
            return 1;
        }
    }

    /* first things first-- let's detect and manage both printing modifiers (/fo and /nh) */
    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/nh") == 0)
        {
            NoHeaderArgCount++;

            if (NoHeader == FALSE)
            {
                NoHeader = TRUE;
                // wprintf(L"Headers disabled!\n");
                BlankArgument(i, argv);
            }
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/fo") == 0)
        {
            if ((i + 1) < argc)
            {
                // wprintf(L"exists another param after /fo\n");

                PrintFormatArgCount++;

                if (_wcsicmp(argv[i + 1], L"table") == 0 && PrintFormat != table)
                {
                    PrintFormat = table;
                    // wprintf(L"Changed to table format\n");
                    BlankArgument(i, argv);
                    BlankArgument(i + 1, argv);
                }
                else if (_wcsicmp(argv[i + 1], L"list") == 0 && PrintFormat != list)
                {
                    PrintFormat = list;
                    // wprintf(L"Changed to list format\n");
                    BlankArgument(i, argv);
                    BlankArgument(i + 1, argv);

                    /* looks like you can't use the "/fo list /nh" options together
                       for some stupid reason */
                    if (PrintFormat == list && NoHeader != FALSE)
                    {
                        wprintf(WhoamiLoadRcString(IDS_ERROR_NH_LIST));
                        return 1;
                    }
                }
                else if (_wcsicmp(argv[i + 1], L"csv") == 0 && PrintFormat != csv)
                {
                    PrintFormat = csv;
                    // wprintf(L"Changed to csv format\n");
                    BlankArgument(i, argv);
                    BlankArgument(i + 1, argv);
                }
                /* /nh or /fo after /fo isn't parsed as a value */
                else if (_wcsicmp(argv[i + 1], L"/nh") == 0 || _wcsicmp(argv[i + 1], L"/fo") == 0

                /* same goes for the other named options, not ideal, but works */
                         || _wcsicmp(argv[i + 1], L"/priv") == 0
                         || _wcsicmp(argv[i + 1], L"/groups") == 0
                         || _wcsicmp(argv[i + 1], L"/user") == 0
                         || _wcsicmp(argv[i + 1], L"/all") == 0
                         || _wcsicmp(argv[i + 1], L"") == 0)
                {
                    goto FoValueExpected;
                }
                else
                {
                    wprintf(WhoamiLoadRcString(IDS_ERROR_VALUENOTALLOWED), argv[i + 1]);
                    return 1;
                }
            }
            else
            {
                FoValueExpected:

                wprintf(WhoamiLoadRcString(IDS_ERROR_VALUEXPECTED));
                return 1;
            }
        }
    }

    if (NoHeaderArgCount >= 2)
    {
        wprintf(WhoamiLoadRcString(IDS_ERROR_1TIMES), L"/nh");
        return 1;
    }
    /* special case when there's just a /nh as argument; it outputs nothing */
    else if (NoHeaderArgCount == 1 && argc == 2)
    {
        return 0;
    }

    if (PrintFormatArgCount >= 2)
    {
        wprintf(WhoamiLoadRcString(IDS_ERROR_1TIMES), L"/fo");
        return 1;
    }
    /* if there's just /fo <format>... call it invalid */
    else if (PrintFormatArgCount == 1 && argc == 3)
    {
        goto InvalidSyntax;
    }

    /* * * * * * * * * * * * * *
     * B: one single parameter */

    if (argc == 2)
    {
        /* now let's try to parse the triumvirate of simpler, single (1) arguments... plus help */
        if (_wcsicmp(argv[1], L"/?") == 0)
        {
            wprintf(WhoamiLoadRcString(IDS_HELP));
            return 0;
        }

        else if (_wcsicmp(argv[1], L"/upn") == 0)
        {
            LPWSTR UserBuffer = WhoamiGetUser(NameUserPrincipal);

            if (UserBuffer)
            {
                wprintf(L"%s\n", UserBuffer);
                WhoamiFree(UserBuffer);
                return 0;
            }
            else
            {
                wprintf(WhoamiLoadRcString(IDS_ERROR_UPN));
                return 1;
            }
        }

        else if (_wcsicmp(argv[1], L"/fqdn") == 0)
        {
            LPWSTR UserBuffer = WhoamiGetUser(NameFullyQualifiedDN);

            if (UserBuffer)
            {
                wprintf(L"%s\n", UserBuffer);
                WhoamiFree(UserBuffer);
                return 0;
            }
            else
            {
                wprintf(WhoamiLoadRcString(IDS_ERROR_FQDN));
                return 1;
            }
        }

        else if (_wcsicmp(argv[1], L"/logonid") == 0)
        {
            return WhoamiLogonId();
        }
    }

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     * C: One main parameter with extra tasty modifiers to play with */

    /* sometimes is just easier to whitelist for lack of a better method */
    for (i=1; i<argc; i++)
    {
        if ((_wcsicmp(argv[i], L"/user") != 0) &&
            (_wcsicmp(argv[i], L"/groups") != 0) &&
            (_wcsicmp(argv[i], L"/priv") != 0) &&
            (_wcsicmp(argv[i], L"/all") != 0) &&
            (_wcsicmp(argv[i], L"") != 0))
        {
            wprintf(WhoamiLoadRcString(IDS_ERROR_INVALIDARG), argv[i]);
            return 1;
        }
    }

    if (GetArgument(L"/user", argc, argv))
    {
        WamBit |= WAM_USER;
    }

    if (GetArgument(L"/groups", argc, argv))
    {
        WamBit |= WAM_GROUPS;
    }

    if (GetArgument(L"/priv", argc, argv))
    {
        WamBit |= WAM_PRIV;
    }

    if (GetArgument(L"/all", argc, argv))
    {
        /* one can't have it /all and any of the other options at the same time */
        if ((WamBit & (WAM_USER | WAM_GROUPS | WAM_PRIV)) == 0)
        {
            WamBit |= (WAM_USER | WAM_GROUPS | WAM_PRIV);
        }
        else
        {
            goto InvalidSyntax;
        }
    }

    if (WamBit & WAM_USER)
    {
        WhoamiUser();
    }
    if (WamBit & WAM_GROUPS)
    {
        WhoamiGroups();
    }
    if (WamBit & WAM_PRIV)
    {
        WhoamiPriv();
    }

    return 0;

InvalidSyntax:
    wprintf(WhoamiLoadRcString(IDS_ERROR_INVALIDSYNTAX));
    return 1;
}
