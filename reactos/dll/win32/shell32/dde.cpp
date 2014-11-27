/*
 * Shell DDE Handling
 *
 * Copyright 2004 Robert Shearman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"
#include <ddeml.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(shelldde);

typedef DWORD(CALLBACK * pfnCommandHandler)(PWSTR strCommand, PWSTR strPath, LPITEMIDLIST pidl, INT unkS);

struct DDECommandHandler
{
    WCHAR Command[32];
    pfnCommandHandler Handler;
};

extern DDECommandHandler HandlerList [];
extern const int HandlerListLength;

/* DDE Instance ID */
static DWORD dwDDEInst;

/* String handles */
static HSZ hszProgmanTopic;
static HSZ hszProgmanService;
static HSZ hszShell;
static HSZ hszAppProperties;
static HSZ hszFolders;

static BOOL bInitialized;

static BOOL Dde_OnConnect(HSZ hszTopic, HSZ hszService)
{
    WCHAR szTopic[MAX_PATH];
    WCHAR szService[MAX_PATH];

    DdeQueryStringW(dwDDEInst, hszTopic, szTopic, _countof(szTopic), CP_WINUNICODE);
    DdeQueryStringW(dwDDEInst, hszService, szService, _countof(szService), CP_WINUNICODE);

    DbgPrint("Dde_OnConnect: topic=%S, service=%S\n", szTopic, szService);

    return TRUE;
}

static void Dde_OnConnectConfirm(HCONV hconv, HSZ hszTopic, HSZ hszService)
{
    WCHAR szTopic[MAX_PATH];
    WCHAR szService[MAX_PATH];

    DdeQueryStringW(dwDDEInst, hszTopic, szTopic, _countof(szTopic), CP_WINUNICODE);
    DdeQueryStringW(dwDDEInst, hszService, szService, _countof(szService), CP_WINUNICODE);

    DbgPrint("Dde_OnConnectConfirm: hconv=%p, topic=%S, service=%S\n", hconv, szTopic, szService);
}

static BOOL Dde_OnWildConnect(HSZ hszTopic, HSZ hszService)
{
    WCHAR szTopic[MAX_PATH];
    WCHAR szService[MAX_PATH];

    DdeQueryStringW(dwDDEInst, hszTopic, szTopic, _countof(szTopic), CP_WINUNICODE);
    DdeQueryStringW(dwDDEInst, hszService, szService, _countof(szService), CP_WINUNICODE);

    DbgPrint("Dde_OnWildConnect: topic=%S, service=%S\n", szTopic, szService);

    return FALSE;
}

static HDDEDATA Dde_OnRequest(UINT uFmt, HCONV hconv, HSZ hszTopic, HSZ hszItem)
{
    WCHAR szTopic[MAX_PATH];
    WCHAR szItem[MAX_PATH];

    DdeQueryStringW(dwDDEInst, hszTopic, szTopic, _countof(szTopic), CP_WINUNICODE);
    DdeQueryStringW(dwDDEInst, hszItem, szItem, _countof(szItem), CP_WINUNICODE);

    DbgPrint("Dde_OnRequest: uFmt=%d, hconv=%p, topic=%S, item=%S\n", hconv, szTopic, szItem);

    return NULL;
}

static LPITEMIDLIST _ILReadFromSharedMemory(PCWSTR strField)
{
    LPITEMIDLIST ret = NULL;

    // Ensure it really is an IDLIST-formatted parameter
    // Format for IDLIST params: ":pid:shared"
    if (*strField != L':')
        return NULL;

    HANDLE hData = (HANDLE) StrToIntW(strField + 1);
    PWSTR strSecond = StrChrW(strField + 1, L':');

    if (strSecond)
    {
        int pid = StrToIntW(strSecond + 1);
        void* pvShared = SHLockShared(hData, pid);
        if (pvShared)
        {
            ret = ILClone((LPCITEMIDLIST) pvShared);
            SHUnlockShared(pvShared);
            SHFreeShared(hData, pid);
        }
    }
    return ret;
}

static DWORD Dde_OnExecute(HCONV hconv, HSZ hszTopic, HDDEDATA hdata)
{
    WCHAR szTopic[MAX_PATH];
    WCHAR szCommand[MAX_PATH];
    WCHAR *pszCommand;

    DdeQueryStringW(dwDDEInst, hszTopic, szTopic, _countof(szTopic), CP_WINUNICODE);

    pszCommand = (WCHAR*) DdeAccessData(hdata, NULL);
    if (!pszCommand)
        return DDE_FNOTPROCESSED;

    StringCchCopyW(szCommand, _countof(szCommand), pszCommand);

    DdeUnaccessData(hdata);

    DbgPrint("Dde_OnExecute: hconv=%p, topic=%S, command=%S\n", hconv, szTopic, pszCommand);

    /*
    [ViewFolder("%l", %I, %S)]    -- Open a folder in standard mode
    [ExploreFolder("%l", %I, %S)] -- Open a folder in "explore" mode (file tree is shown to the left by default)
    [FindFolder("%l", %I)]        -- Open a folder in "find" mode (search panel is shown to the left by default)
    [ShellFile("%1","%1",%S)]     -- Execute the contents of the specified .SCF file

    Approximate grammar (Upper names define rules, <lower> names define terminals, single-quotes are literals):

        Rules
            Command = ('[' Function ']') | Function
            Function = <identifier> '(' Parameters ')'
            Parameters = (<quoted-string> (',' <idlist> (',' <number>)?)?)?

        Terminals
            <identifier> =  [a-zA-Z]+
            <quoted-string> = \"([^\"]|\\.)\"
            <idlist> = \:[0-9]+\:[0-9]+
            <number> = [0-9]+
    */

    WCHAR Command[MAX_PATH] = L"";
    WCHAR Path[MAX_PATH] = L"";
    LPITEMIDLIST IdList = NULL;
    INT UnknownParameter = 0;

    // Simplified parsing (assumes the command will not be TOO broken):

    PWSTR cmd = szCommand;
    //    1. if starts with [, skip first char
    if (*cmd == L'[')
        cmd++;

    if (*cmd == L']')
    {
        ERR("Empty command. Nothing to run.\n");
        return DDE_FNOTPROCESSED;
    }

    // Read until first (, and take text before ( as command name
    {
        PWSTR cmdEnd = StrChrW(cmd, L'(');

        if (!cmdEnd)
        {
            ERR("Could not find '('. Invalid command.\n");
            return DDE_FNOTPROCESSED;
        }

        *cmdEnd = 0;

        StringCchCopy(Command, _countof(Command), cmd);

        cmd = cmdEnd + 1;
    }

    // Read first param after (, expecting quoted string
    if (*cmd != L')')
    {
        // Copy unescaped string
        PWSTR dst = Path;
        BOOL isQuote = FALSE;

        PWSTR arg = cmd;

        while (*arg && (isQuote || *arg != L','))
        {
            if (*arg == L'"')
            {
                isQuote = !isQuote;
                if (isQuote && arg != cmd) // do not copy the " at the beginning of the string
                {
                    *(dst++) = L'"';
                }
            }
            else
            {
                *(dst++) = *arg;
            }

            arg++;
        }

        cmd = arg + 1;

        while (*cmd == L' ')
            cmd++;
    }

    // Read second param, expecting an idlist in shared memory
    if (*cmd != L')')
    {
        if (*cmd != ':')
        {
            ERR("Expected ':'. Invalid command.\n");
            return DDE_FNOTPROCESSED;
        }

        PWSTR idlistEnd = StrChrW(cmd, L',');

        if (!idlistEnd)
            idlistEnd = StrChrW(cmd, L')');

        if (!idlistEnd)
        {
            ERR("Expected ',' or ')'. Invalid command.\n");
            return DDE_FNOTPROCESSED;
        }

        IdList = _ILReadFromSharedMemory(cmd);

        cmd = idlistEnd + 1;
    }

    // Read third param, expecting an integer
    if (*cmd != L')')
    {
        UnknownParameter = StrToIntW(cmd);
    }

    DbgPrint("Parse end: cmd=%S, S=%d, pidl=%p, path=%S\n", Command, UnknownParameter, IdList, Path);

    // Find handler in list
    for (int i = 0; i < HandlerListLength; i++)
    {
        DDECommandHandler & handler = HandlerList[i];
        if (StrCmpW(handler.Command, Command) == 0)
        {
            return handler.Handler(Command, Path, IdList, UnknownParameter);
        }
    }

    // No handler found
    ERR("Unknown command %S\n", Command);
    return DDE_FNOTPROCESSED;
}

static void Dde_OnDisconnect(HCONV hconv)
{
    DbgPrint("Dde_OnDisconnect: hconv=%p\n", hconv);
}

static HDDEDATA CALLBACK DdeCallback(
    UINT uType,
    UINT uFmt,
    HCONV hconv,
    HSZ hsz1,
    HSZ hsz2,
    HDDEDATA hdata,
    ULONG_PTR dwData1,
    ULONG_PTR dwData2)
{
    switch (uType)
    {
    case XTYP_CONNECT:
        return (HDDEDATA) (DWORD_PTR) Dde_OnConnect(hsz1, hsz2);
    case XTYP_CONNECT_CONFIRM:
        Dde_OnConnectConfirm(hconv, hsz1, hsz2);
        return NULL;
    case XTYP_WILDCONNECT:
        return (HDDEDATA) (DWORD_PTR) Dde_OnWildConnect(hsz1, hsz2);
    case XTYP_REQUEST:
        return Dde_OnRequest(uFmt, hconv, hsz1, hsz2);
    case XTYP_EXECUTE:
        return (HDDEDATA) (DWORD_PTR) Dde_OnExecute(hconv, hsz1, hdata);
    case XTYP_DISCONNECT:
        Dde_OnDisconnect(hconv);
        return NULL;
    case XTYP_REGISTER:
        return NULL;
    default:
        DbgPrint("DdeCallback: unknown uType=%d\n", uType);
        return NULL;
    }
}
/*************************************************************************
 * ShellDDEInit (SHELL32.@)
 *
 * Registers the Shell DDE services with the system so that applications
 * can use them.
 *
 * PARAMS
 *  bInit [I] TRUE to initialize the services, FALSE to uninitialize.
 *
 * RETURNS
 *  Nothing.
 */
EXTERN_C void WINAPI ShellDDEInit(BOOL bInit)
{
    DbgPrint("ShellDDEInit bInit = %s\n", bInit ? "TRUE" : "FALSE");

    if (bInit && !bInitialized)
    {
        DdeInitializeW(&dwDDEInst, DdeCallback, CBF_FAIL_ADVISES | CBF_FAIL_POKES, 0);

        hszProgmanTopic = DdeCreateStringHandleW(dwDDEInst, L"Progman", CP_WINUNICODE);
        hszProgmanService = DdeCreateStringHandleW(dwDDEInst, L"Progman", CP_WINUNICODE);
        hszShell = DdeCreateStringHandleW(dwDDEInst, L"Shell", CP_WINUNICODE);
        hszAppProperties = DdeCreateStringHandleW(dwDDEInst, L"AppProperties", CP_WINUNICODE);
        hszFolders = DdeCreateStringHandleW(dwDDEInst, L"Folders", CP_WINUNICODE);

        if (hszProgmanTopic && hszProgmanService &&
            hszShell && hszAppProperties && hszFolders &&
            DdeNameService(dwDDEInst, hszFolders, 0, DNS_REGISTER) &&
            DdeNameService(dwDDEInst, hszProgmanService, 0, DNS_REGISTER) &&
            DdeNameService(dwDDEInst, hszShell, 0, DNS_REGISTER))
        {
            bInitialized = TRUE;
        }
    }
    else if (!bInit && bInitialized)
    {
        /* unregister all services */
        DdeNameService(dwDDEInst, 0, 0, DNS_UNREGISTER);

        if (hszFolders)
            DdeFreeStringHandle(dwDDEInst, hszFolders);
        if (hszAppProperties)
            DdeFreeStringHandle(dwDDEInst, hszAppProperties);
        if (hszShell)
            DdeFreeStringHandle(dwDDEInst, hszShell);
        if (hszProgmanService)
            DdeFreeStringHandle(dwDDEInst, hszProgmanService);
        if (hszProgmanTopic)
            DdeFreeStringHandle(dwDDEInst, hszProgmanTopic);

        DdeUninitialize(dwDDEInst);

        bInitialized = FALSE;
    }
}

static DWORD CALLBACK DDE_OnViewFolder(PWSTR strCommand, PWSTR strPath, LPITEMIDLIST pidl, INT unkS)
{
    if (!pidl)
        pidl = ILCreateFromPathW(strPath);

    if (!pidl)
        return DDE_FNOTPROCESSED;

    if (FAILED(SHOpenNewFrame(pidl, NULL, 0, 0)))
        return DDE_FNOTPROCESSED;

    return DDE_FACK;
}

static DWORD CALLBACK DDW_OnExploreFolder(PWSTR strCommand, PWSTR strPath, LPITEMIDLIST pidl, INT unkS)
{
    if (!pidl)
        pidl = ILCreateFromPathW(strPath);

    if (!pidl)
        return DDE_FNOTPROCESSED;

    if (FAILED(SHOpenNewFrame(pidl, NULL, 0, SH_EXPLORER_CMDLINE_FLAG_E)))
        return DDE_FNOTPROCESSED;

    return DDE_FACK;
}

static DWORD CALLBACK DDE_OnFindFolder(PWSTR strCommand, PWSTR strPath, LPITEMIDLIST pidl, INT unkS)
{
    UNIMPLEMENTED;
    return DDE_FNOTPROCESSED;
}

static DWORD CALLBACK DDE_OnShellFile(PWSTR strCommand, PWSTR strPath, LPITEMIDLIST pidl, INT unkS)
{
    UNIMPLEMENTED;
    return DDE_FNOTPROCESSED;
}

DDECommandHandler HandlerList [] = {

        { L"ViewFolder", DDE_OnViewFolder },
        { L"ExploreFolder", DDW_OnExploreFolder },
        { L"FindFolder", DDE_OnFindFolder },
        { L"ShellFile", DDE_OnShellFile }
};

const int HandlerListLength = _countof(HandlerList);
