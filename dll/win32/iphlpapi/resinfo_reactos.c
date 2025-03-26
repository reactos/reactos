/*
 * iphlpapi dll implementation -- Setting and storing route information
 *
 * These are stubs for functions that set routing information on the target
 * operating system.  They are grouped here because their implementation will
 * vary widely by operating system.
 *
 * Copyright (C) 2004 Art Yerkes
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include "iphlpapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

typedef struct _NAME_SERVER_LIST_PRIVATE {
    UINT NumServers;
    IP_ADDR_STRING * pCurrent;
} NAME_SERVER_LIST_PRIVATE, *PNAME_SERVER_LIST_PRIVATE;

typedef VOID (*ENUM_INTERFACE_CALLBACK)(
    HKEY ChildKeyHandle,
    LPWSTR ChildKeyName,
    PVOID CallbackContext);

LSTATUS
QueryNameServer(
    IN HKEY hInterface,
    IN LPCWSTR NameServerKey,
    OUT LPWSTR * OutNameServer)
{
    DWORD dwLength, dwType;
    LPWSTR NameServer;
    LSTATUS Status;

    /* query ns */
    dwLength = 0;
    Status = RegQueryValueExW(hInterface, NameServerKey, NULL, &dwType, NULL, &dwLength);

    if (Status != ERROR_SUCCESS)
    {
        /* failed to retrieve size */
        TRACE("Status %x\n", Status);
        return Status;
    }

    /* add terminating null */
    dwLength += sizeof(WCHAR);

    /* allocate name server */
    NameServer = HeapAlloc(GetProcessHeap(), 0, dwLength);

    if (!NameServer)
    {
        /* no memory */
        return ERROR_OUTOFMEMORY;
    }

    /* query ns */
    Status = RegQueryValueExW(hInterface, NameServerKey, NULL, &dwType, (LPBYTE)NameServer, &dwLength);

    if (Status != ERROR_SUCCESS || dwType != REG_SZ)
    {
        /* failed to retrieve ns */
        HeapFree(GetProcessHeap(), 0, NameServer);
        return Status;
    }

    /* null terminate it */
    NameServer[dwLength / sizeof(WCHAR)] = L'\0';

    /* store result */
    *OutNameServer = NameServer;

    return STATUS_SUCCESS;
}


LSTATUS
EnumNameServers(
    IN HKEY hInterface,
    IN LPWSTR InterfaceName,
    PVOID ServerCallbackContext,
    EnumNameServersFunc CallbackRoutine)
{
    LSTATUS Status;
    LPWSTR NameServer;
    WCHAR Buffer[50];
    DWORD Length;
    LPWSTR Start, Comma;

    /* query static assigned name server */
    Status = QueryNameServer(hInterface, L"NameServer", &NameServer);
    if (Status != ERROR_SUCCESS)
    {
        /* query dynamic assigned name server */
        Status = QueryNameServer(hInterface, L"DhcpNameServer", &NameServer);

        if (Status != ERROR_SUCCESS)
        {
            /* failed to retrieve name servers */
            return Status;
        }
    }

    /* enumerate all name servers, terminated by comma */
    Start = NameServer;

    do
    {
        /* find next terminator */
        Comma = wcschr(Start, L',');

        if (Comma)
        {
            /* calculate length */
            Length = Comma - Start;

            /* copy name server */
            RtlMoveMemory(Buffer, Start, Length * sizeof(WCHAR));

            /* null terminate it */
            Buffer[Length] = L'\0';

            /* perform callback */
            CallbackRoutine(InterfaceName, Buffer, ServerCallbackContext);

        }
        else
        {
            /* perform callback */
            CallbackRoutine(InterfaceName, Start, ServerCallbackContext);

            /* last entry */
            break;
        }

        /* increment offset */
        Start = Comma + 1;

    }while(TRUE);

    /* free name server string */
    HeapFree(GetProcessHeap(), 0, NameServer);

    /* done */
    return ERROR_SUCCESS;
}

LSTATUS
EnumInterfaces(
    ENUM_INTERFACE_CALLBACK CallbackRoutine,
    PVOID InterfaceCallbackContext)
{
    HKEY hKey, hInterface;
    LSTATUS Status;
    DWORD NumInterfaces, InterfaceNameLen, Index, Length;
    LPWSTR InterfaceName;

    /* first open interface key */
    Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces", 0, KEY_READ, &hKey);

    /* check for success */
    if (Status != ERROR_SUCCESS)
    {
        /* failed to open interface key */
        return Status;
    }

    /* now get maximum interface name length and number of interfaces */
    Status = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &NumInterfaces, &InterfaceNameLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (Status != ERROR_SUCCESS)
    {
        /* failed to get key info */
        RegCloseKey(hKey);
        return Status;
    }

    /* RegQueryInfoKey does not include terminating null */
    InterfaceNameLen++;

    /* allocate interface name */
    InterfaceName = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, InterfaceNameLen * sizeof(WCHAR));

    if (!InterfaceName)
    {
        /* no memory */
        RegCloseKey(hKey);
        return ERROR_OUTOFMEMORY;
    }

    /* no enumerate all interfaces */
    for(Index = 0; Index < NumInterfaces; Index++)
    {
        /* query interface name */
        Length = InterfaceNameLen;
        Status = RegEnumKeyExW(hKey, Index, InterfaceName, &Length, NULL, NULL, NULL, NULL);

        if (Status == ERROR_SUCCESS)
        {
            /* make sure it is null terminated */
            InterfaceName[Length] = L'\0';

            /* now open child key */
            Status = RegOpenKeyExW(hKey, InterfaceName, 0, KEY_READ, &hInterface);

            if (Status == ERROR_SUCCESS)
            {
                /* perform enumeration callback */
                CallbackRoutine(hInterface, InterfaceName, InterfaceCallbackContext);

                /* close interface key */
                RegCloseKey(hInterface);
            }
        }
    }

    /* free interface name */
    HeapFree(GetProcessHeap(), 0, InterfaceName);

    /* close root interface key */
    RegCloseKey(hKey);

    /* done */
    return Status;
}

VOID
CountNameServerCallback(
    IN LPWSTR InterfaceName,
    IN LPWSTR Server,
    IN PVOID CallbackContext)
{
    /* get context */
    PNAME_SERVER_LIST_PRIVATE Data = (PNAME_SERVER_LIST_PRIVATE)CallbackContext;

    /* increment server count */
    Data->NumServers++;
}

VOID
CountServerCallbackTrampoline(
    HKEY ChildKeyHandle,
    LPWSTR ChildKeyName,
    PVOID CallbackContext)
{
    EnumNameServers(ChildKeyHandle, ChildKeyName, CallbackContext, CountNameServerCallback);
}

LSTATUS
CountNameServers(
    IN PNAME_SERVER_LIST_PRIVATE PrivateData )
{
    return EnumInterfaces(CountServerCallbackTrampoline, (PVOID)PrivateData);
}

VOID
CreateNameServerListCallback(
    IN LPWSTR InterfaceName,
    IN LPWSTR Server,
    IN PVOID CallbackContext)
{
    /* get context */
    PNAME_SERVER_LIST_PRIVATE Data = (PNAME_SERVER_LIST_PRIVATE)CallbackContext;

    /* convert to ansi ns string */
    if (WideCharToMultiByte(CP_ACP, 0, Server, -1, Data->pCurrent->IpAddress.String, 16, NULL, NULL))
    {
        /* store offset to next name server struct */
        Data->pCurrent->Next = (struct _IP_ADDR_STRING*)(Data->pCurrent + 1);

        /* move to next entry */
        Data->pCurrent = Data->pCurrent->Next;

        /* increment server count */
        Data->NumServers++;
    }
    else
    {
        /* failed to convert dns server */
        Data->pCurrent->IpAddress.String[0] = '\0';
    }
}

VOID
CreateNameServerListCallbackTrampoline(
    HKEY ChildKeyHandle,
    LPWSTR ChildKeyName,
    PVOID CallbackContext)
{
    EnumNameServers(ChildKeyHandle, ChildKeyName, CallbackContext, CreateNameServerListCallback);
}

LSTATUS
MakeNameServerList(
    PNAME_SERVER_LIST_PRIVATE PrivateData )
{
    return EnumInterfaces(CreateNameServerListCallbackTrampoline, (PVOID)PrivateData);
}

PIPHLP_RES_INFO
getResInfo()
{
    NAME_SERVER_LIST_PRIVATE PrivateNSEnum;
    PIPHLP_RES_INFO ResInfo;
    IP_ADDR_STRING * DnsList = NULL;
    LSTATUS Status;

    PrivateNSEnum.NumServers = 0;

    /* count name servers */
    Status = CountNameServers(&PrivateNSEnum);

    if (Status != ERROR_SUCCESS)
    {
        /* failed to enumerate name servers */
        return NULL;
    }

    /* are there any servers */
    if (PrivateNSEnum.NumServers)
    {
        /* allocate dns servers */
        DnsList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PrivateNSEnum.NumServers * sizeof(IP_ADDR_STRING));

        if (!DnsList)
        {
            /* no memory */
            return NULL;
        }
    }

    /* allocate private struct */
    ResInfo = (PIPHLP_RES_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IPHLP_RES_INFO));

    if(!ResInfo)
    {
        /* no memory */
        if (DnsList)
        {
            /* free dns list */
            HeapFree( GetProcessHeap(), 0, DnsList);
        }
        return NULL;
    }

    /* are there any servers */
    if (PrivateNSEnum.NumServers)
    {
        /* initialize enumeration context */
        PrivateNSEnum.NumServers = 0;
        PrivateNSEnum.pCurrent = DnsList;

        /* enumerate servers */
        MakeNameServerList( &PrivateNSEnum );

        /* store result */
        ResInfo->DnsList = DnsList;
        ResInfo->riCount = PrivateNSEnum.NumServers;
    }

    /* done */
    return ResInfo;
}

VOID disposeResInfo( PIPHLP_RES_INFO InfoPtr )
{
    HeapFree(GetProcessHeap(), 0, InfoPtr->DnsList);
    RtlFreeHeap( GetProcessHeap(), 0, InfoPtr );
}
