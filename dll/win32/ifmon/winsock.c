/*
 * PROJECT:    ReactOS IF Monitor DLL
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    NetSh Helper interface context functions
 * COPYRIGHT:  Copyright 2025 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#include <guiddef.h>

#define NDEBUG
#include <debug.h>

#include "guid.h"
#include "resource.h"

static FN_HANDLE_CMD ResetCommand;
static FN_HANDLE_CMD ShowCatalogCommand;

static
CMD_ENTRY g_TestCmdTable[] = 
{
    {L"reset", ResetCommand, IDS_HLP_WINSOCK_RESET, IDS_HLP_WINSOCK_RESET_EX, 0}
};

static
CMD_ENTRY g_WinsockShowCmdTable[] = 
{
    {L"catalog", ShowCatalogCommand, IDS_HLP_WINSOCK_SHOW_CATALOG, IDS_HLP_WINSOCK_SHOW_CATALOG_EX, 0}
};

static
CMD_GROUP_ENTRY g_WinsockGroupCmds[] = 
{
    {L"show", IDS_HLP_WINSOCK_SHOW, sizeof(g_WinsockShowCmdTable)/sizeof(CMD_ENTRY), 0, g_WinsockShowCmdTable, NULL},
};


static
DWORD
WINAPI
ResetCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PrintMessage(L"ResetCommand(): Not implemented yet!\n");
    return ERROR_SUCCESS;
}


static
DWORD
WINAPI
ShowCatalogCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PrintMessage(L"ShowCatalogCommand(): Not implemented yet!\n");
    return ERROR_SUCCESS;
}


static
DWORD
WINAPI
WinsockStart(
    _In_ const GUID *pguidParent,
    _In_ DWORD dwVersion)
{
    NS_CONTEXT_ATTRIBUTES ContextAttributes;

    DPRINT1("WinsockStart()\n");

    ZeroMemory(&ContextAttributes, sizeof(ContextAttributes));
    ContextAttributes.dwVersion = 1;
    ContextAttributes.pwszContext = L"winsock";
    ContextAttributes.guidHelper = GUID_IFMON_WINSOCK;

    ContextAttributes.ulNumTopCmds = 1;
    ContextAttributes.pTopCmds = g_TestCmdTable;

    ContextAttributes.ulNumGroups = 1;
    ContextAttributes.pCmdGroups = g_WinsockGroupCmds;

    RegisterContext(&ContextAttributes);

    return ERROR_SUCCESS;
}


DWORD
WINAPI
RegisterWinsockHelper(VOID)
{
    NS_HELPER_ATTRIBUTES HelperAttributes;

    ZeroMemory(&HelperAttributes, sizeof(HelperAttributes));
    HelperAttributes.dwVersion  = 1;
    HelperAttributes.guidHelper = GUID_IFMON_WINSOCK;
    HelperAttributes.pfnStart = WinsockStart;
    HelperAttributes.pfnStop = NULL;
    RegisterHelper(NULL, &HelperAttributes);

    return ERROR_SUCCESS;
}
