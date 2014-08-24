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

/* DDE Instance ID */
static DWORD dwDDEInst;

/* String handles */
static HSZ hszProgmanTopic;
static HSZ hszProgmanService;
static HSZ hszShell;
static HSZ hszAppProperties;
static HSZ hszFolders;

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

static DWORD Dde_OnExecute(HCONV hconv, HSZ hszTopic, HDDEDATA hdata)
{
    WCHAR szTopic[MAX_PATH];
    WCHAR szCommand[MAX_PATH];
    WCHAR *pszCommand;

    DdeQueryStringW(dwDDEInst, hszTopic, szTopic, _countof(szTopic), CP_WINUNICODE);

    pszCommand = (WCHAR*)DdeAccessData(hdata, NULL);
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
    
    */

    return DDE_FACK;
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
        return (HDDEDATA)(DWORD_PTR)Dde_OnConnect(hsz1, hsz2);
    case XTYP_CONNECT_CONFIRM:
        Dde_OnConnectConfirm(hconv, hsz1, hsz2);
        return NULL;
    case XTYP_WILDCONNECT:
        return (HDDEDATA)(DWORD_PTR)Dde_OnWildConnect(hsz1, hsz2);
    case XTYP_REQUEST:
        return Dde_OnRequest(uFmt, hconv, hsz1, hsz2);
    case XTYP_EXECUTE:
        return (HDDEDATA)(DWORD_PTR)Dde_OnExecute(hconv, hsz1, hdata);
    case XTYP_DISCONNECT:
        Dde_OnDisconnect(hconv);
        return NULL;
    default:
        DbgPrint("DdeCallback: unknown uType=%d\n", uType);
    case XTYP_REGISTER:
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

    if (bInit)
    {
        DdeInitializeW(&dwDDEInst, DdeCallback, CBF_FAIL_ADVISES | CBF_FAIL_POKES, 0);

        hszProgmanTopic = DdeCreateStringHandleW(dwDDEInst, L"Progman", CP_WINUNICODE);
        hszProgmanService = DdeCreateStringHandleW(dwDDEInst, L"Progman", CP_WINUNICODE);
        hszShell = DdeCreateStringHandleW(dwDDEInst, L"Shell", CP_WINUNICODE);
        hszAppProperties = DdeCreateStringHandleW(dwDDEInst, L"AppProperties", CP_WINUNICODE);
        hszFolders = DdeCreateStringHandleW(dwDDEInst, L"Folders", CP_WINUNICODE);

        DdeNameService(dwDDEInst, hszFolders, 0, DNS_FILTEROFF);
    }
    else
    {
        /* unregister all services */
        DdeNameService(dwDDEInst, 0, 0, DNS_UNREGISTER);

        DdeFreeStringHandle(dwDDEInst, hszFolders);
        DdeFreeStringHandle(dwDDEInst, hszAppProperties);
        DdeFreeStringHandle(dwDDEInst, hszShell);
        DdeFreeStringHandle(dwDDEInst, hszProgmanService);
        DdeFreeStringHandle(dwDDEInst, hszProgmanTopic);

        DdeUninitialize(dwDDEInst);
    }
}
