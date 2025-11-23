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

static FN_HANDLE_CMD InterfaceShowInterface;

static
CMD_ENTRY
InterfaceShowCommands[] = 
{
    {L"interface", InterfaceShowInterface, IDS_HLP_INTERFACE_SHOW_INTERFACE, IDS_HLP_INTERFACE_SHOW_INTERFACE_EX, 0}
};

static
CMD_GROUP_ENTRY
InterfaceGroups[] = 
{
    {L"show", IDS_HLP_INTERFACE_SHOW, sizeof(InterfaceShowCommands) / sizeof(CMD_ENTRY), 0, InterfaceShowCommands, NULL},
};


static
DWORD
WINAPI
InterfaceShowInterface(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PrintMessage(L"InterfaceShowInterface(): Not implemented yet!\n");

    return ERROR_SUCCESS;
}


static
DWORD
WINAPI
InterfaceDumpFn(
    _In_ LPCWSTR pwszRouter,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwArgCount,
    _In_ LPCVOID pvData)
{
    DPRINT("InterfaceDumpFn(%S %p %lu %p)\n", pwszRouter, ppwcArguments, dwArgCount, pvData);

    PrintMessageFromModule(hDllInstance, IDS_DUMP_HEADERLINE);
    PrintMessage(L"# Interface Configuration\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_HEADERLINE);
    PrintMessage(L"pushd interface\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);
    PrintMessage(L"popd\n");
    PrintMessage(L"# End of Interface Configuration\n");
    PrintMessageFromModule(hDllInstance, IDS_DUMP_NEWLINE);

    return ERROR_SUCCESS;
}


DWORD
WINAPI
InterfaceStart(
    _In_ const GUID *pguidParent,
    _In_ DWORD dwVersion)
{
    NS_CONTEXT_ATTRIBUTES ContextAttributes;

    DPRINT("InterfaceStart()\n");

    ZeroMemory(&ContextAttributes, sizeof(ContextAttributes));
    ContextAttributes.dwVersion = 1;
    ContextAttributes.pwszContext = L"interface";
    ContextAttributes.guidHelper = GUID_IFMON_INTERFACE;

    ContextAttributes.ulNumTopCmds = 0;
    ContextAttributes.pTopCmds = NULL;

    ContextAttributes.ulNumGroups = sizeof(InterfaceGroups) / sizeof(CMD_GROUP_ENTRY);
    ContextAttributes.pCmdGroups = InterfaceGroups;

    ContextAttributes.pfnDumpFn = InterfaceDumpFn;

    RegisterContext(&ContextAttributes);

    return ERROR_SUCCESS;
}


DWORD
WINAPI
RegisterInterfaceHelper(VOID)
{
    NS_HELPER_ATTRIBUTES HelperAttributes;

    DPRINT("RegisterInterfaceHelper()\n");

    ZeroMemory(&HelperAttributes, sizeof(HelperAttributes));
    HelperAttributes.dwVersion = 1;
    HelperAttributes.guidHelper = GUID_IFMON_INTERFACE;
    HelperAttributes.pfnStart = InterfaceStart;
    HelperAttributes.pfnStop = NULL;
    RegisterHelper(NULL, &HelperAttributes);

    return ERROR_SUCCESS;
}
