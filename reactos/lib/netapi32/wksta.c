/* Copyright 2002 Andriy Palamarchuk
 * Copyright (c) 2003 Juan Lang
 *
 * netapi32 user functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "nb30.h"
#include "lmcons.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "lmwksta.h"
#include "iphlpapi.h"
#include "winerror.h"
#include "ntstatus.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "netbios.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                NETAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
BOOL NETAPI_IsLocalComputer(LPCWSTR ServerName)
{
    if (!ServerName)
    {
        return TRUE;
    }
    else
    {
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
        BOOL Result;
        LPWSTR buf;

        NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) &buf);
        Result = GetComputerNameW(buf,  &dwSize);
        if (Result && (ServerName[0] == '\\') && (ServerName[1] == '\\'))
            ServerName += 2;
        Result = Result && !lstrcmpW(ServerName, buf);
        NetApiBufferFree(buf);

        return Result;
    }
}

static void wprint_mac(WCHAR* buffer, int len, PMIB_IFROW ifRow)
{
    int i;
    unsigned char val;

    if (!buffer)
        return;
    if (len < 1)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    for (i = 0; i < ifRow->dwPhysAddrLen && 2 * i < len; i++)
    {
        val = ifRow->bPhysAddr[i];
        if ((val >>4) >9)
            buffer[2*i] = (WCHAR)((val >>4) + 'A' - 10);
        else
            buffer[2*i] = (WCHAR)((val >>4) + '0');
        if ((val & 0xf ) >9)
            buffer[2*i+1] = (WCHAR)((val & 0xf) + 'A' - 10);
        else
            buffer[2*i+1] = (WCHAR)((val & 0xf) + '0');
    }
    buffer[2*i]=(WCHAR)0;
}

/* Theoretically this could be too short, except that MS defines
 * MAX_ADAPTER_NAME as 128, and MAX_INTERFACE_NAME_LEN as 256, and both
 * represent a count of WCHARs, so even with an extroardinarily long header
 * this will be plenty
 */
#define MAX_TRANSPORT_NAME MAX_INTERFACE_NAME_LEN
#define MAX_TRANSPORT_ADDR 13

#define NBT_TRANSPORT_NAME_HEADER "\\Device\\NetBT_Tcpip_"
#define UNKNOWN_TRANSPORT_NAME_HEADER "\\Device\\UnknownTransport_"

static void wprint_name(WCHAR *buffer, int len, ULONG transport,
 PMIB_IFROW ifRow)
{
    WCHAR *ptr1, *ptr2;
    const char *name;

    if (!buffer)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
        name = NBT_TRANSPORT_NAME_HEADER;
    else
        name = UNKNOWN_TRANSPORT_NAME_HEADER;

    for (ptr1 = buffer; *name && ptr1 < buffer + len; ptr1++, name++)
        *ptr1 = *name;
    for (ptr2 = ifRow->wszName; *ptr2 && ptr1 < buffer + len; ptr1++, ptr2++)
        *ptr1 = *ptr2;
    *ptr1 = '\0';
}
 
struct WkstaTransportEnumData
{
    UCHAR          n_adapt;
    UCHAR          n_read;
    DWORD          prefmaxlen;
    LPBYTE        *pbuf;
    NET_API_STATUS ret;
};

static BOOL WkstaEnumAdaptersCallback(UCHAR totalLANAs, UCHAR lanaIndex,
 ULONG transport, const NetBIOSAdapterImpl *data, void *closure)
{
    BOOL ret;
    struct WkstaTransportEnumData *enumData = (struct WkstaTransportEnumData *)
     closure;

    if (enumData && enumData->pbuf)
    {
        if (lanaIndex == 0)
        {
            DWORD toAllocate;

            enumData->n_adapt = totalLANAs;
            enumData->n_read = 0;

            toAllocate = totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0)
             + MAX_TRANSPORT_NAME * sizeof(WCHAR) +
             MAX_TRANSPORT_ADDR * sizeof(WCHAR));
            if (enumData->prefmaxlen != MAX_PREFERRED_LENGTH)
                toAllocate = enumData->prefmaxlen;
            NetApiBufferAllocate(toAllocate, (LPVOID *)enumData->pbuf);
        }
        if (*(enumData->pbuf))
        {
            UCHAR spaceFor;

            if (enumData->prefmaxlen == MAX_PREFERRED_LENGTH)
                spaceFor = totalLANAs;
            else
                spaceFor = enumData->prefmaxlen /
                 (sizeof(WKSTA_TRANSPORT_INFO_0) + (MAX_TRANSPORT_NAME +
                 MAX_TRANSPORT_ADDR) * sizeof(WCHAR));
            if (enumData->n_read < spaceFor)
            {
                PWKSTA_TRANSPORT_INFO_0 ti;
                LPWSTR transport_name, transport_addr;
                MIB_IFROW ifRow;

                ti = (PWKSTA_TRANSPORT_INFO_0)(*(enumData->pbuf) +
                 enumData->n_read * sizeof(WKSTA_TRANSPORT_INFO_0));
                transport_name = (LPWSTR)(*(enumData->pbuf) +
                 totalLANAs * sizeof(WKSTA_TRANSPORT_INFO_0) +
                 enumData->n_read * MAX_TRANSPORT_NAME * sizeof(WCHAR));
                transport_addr = (LPWSTR)(*(enumData->pbuf) +
                 totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0) +
                 MAX_TRANSPORT_NAME * sizeof(WCHAR)) +
                 enumData->n_read * MAX_TRANSPORT_ADDR * sizeof(WCHAR));

                ifRow.dwIndex = data->ifIndex;
                GetIfEntry(&ifRow);
                ti->wkti0_quality_of_service = 0;
                ti->wkti0_number_of_vcs = 0;
                ti->wkti0_transport_name = transport_name;
                wprint_name(ti->wkti0_transport_name, MAX_TRANSPORT_NAME,
                 transport, &ifRow);
                ti->wkti0_transport_address = transport_addr;
                wprint_mac(ti->wkti0_transport_address, MAX_TRANSPORT_ADDR,
                 &ifRow);
                if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
                    ti->wkti0_wan_ish = TRUE;
                else
                    ti->wkti0_wan_ish = FALSE;
                TRACE("%d of %d:ti at %p\n", lanaIndex, totalLANAs, ti);
                TRACE("transport_name at %p %s\n",
                 ti->wkti0_transport_name,
                 debugstr_w(ti->wkti0_transport_name));
                TRACE("transport_address at %p %s\n",
                 ti->wkti0_transport_address,
                 debugstr_w(ti->wkti0_transport_address));
                enumData->n_read++;
                enumData->ret = NERR_Success;
                ret = TRUE;
            }
            else
            {
                enumData->ret = ERROR_MORE_DATA;
                ret = FALSE;
            }
        }
        else
        {
            enumData->ret = ERROR_OUTOFMEMORY;
            ret = FALSE;
        }
    }
    else
        ret = FALSE;
    return ret;
}

NET_API_STATUS WINAPI 
NetWkstaTransportEnum(LPWSTR ServerName, DWORD level, PBYTE* pbuf,
      DWORD prefmaxlen, LPDWORD read_entries,
      PDWORD total_entries, PDWORD hresume)
{
    NET_API_STATUS ret;

    TRACE(":%s, 0x%08lx, %p, 0x%08lx, %p, %p, %p\n", debugstr_w(ServerName), 
     level, pbuf, prefmaxlen, read_entries, total_entries,hresume);
    if (!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME(":not implemented for non-local computers\n");
        ret = ERROR_INVALID_LEVEL;
    }
    else
    {
        if (hresume && *hresume)
        {
          FIXME(":resume handle not implemented\n");
          return ERROR_INVALID_LEVEL;
        }

        switch (level)
        {
            case 0: /* transport info */
            {
                ULONG allTransports;
                struct WkstaTransportEnumData enumData;

                if (NetBIOSNumAdapters() == 0)
                  return ERROR_NETWORK_UNREACHABLE;
                if (!read_entries)
                  return STATUS_ACCESS_VIOLATION;
                if (!total_entries || !pbuf)
                  return RPC_X_NULL_REF_POINTER;

                enumData.prefmaxlen = prefmaxlen;
                enumData.pbuf = pbuf;
                memcpy(&allTransports, ALL_TRANSPORTS, sizeof(ULONG));
                NetBIOSEnumAdapters(allTransports, WkstaEnumAdaptersCallback,
                 &enumData);
                *read_entries = enumData.n_read;
                *total_entries = enumData.n_adapt;
                if (hresume) *hresume= 0;
                ret = enumData.ret;
                break;
            }
            default:
                ERR("Invalid level %ld is specified\n", level);
                ret = ERROR_INVALID_LEVEL;
        }
    }
    return ret;
}


/************************************************************
 *                NetWkstaUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetWkstaUserGetInfo(LPWSTR reserved, DWORD level,
                                          PBYTE* bufptr)
{
    TRACE("(%s, %ld, %p)\n", debugstr_w(reserved), level, bufptr);
    switch (level)
    {
    case 0:
    {
        PWKSTA_USER_INFO_0 ui;
        DWORD dwSize = UNLEN + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_0) + dwSize * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PWKSTA_USER_INFO_0) *bufptr;
        ui->wkui0_username = (LPWSTR) (*bufptr + sizeof(WKSTA_USER_INFO_0));

        /* get data */
        if (!GetUserNameW(ui->wkui0_username, &dwSize))
        {
            NetApiBufferFree(ui);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        else
            NetApiBufferReallocate(
                *bufptr, sizeof(WKSTA_USER_INFO_0) +
                (lstrlenW(ui->wkui0_username) + 1) * sizeof(WCHAR),
                (LPVOID *) bufptr);
        break;
    }

    case 1:
    {
        PWKSTA_USER_INFO_1 ui;
        PWKSTA_USER_INFO_0 ui0;
        DWORD dwSize;
        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        LSA_HANDLE PolicyHandle;
        PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
        NTSTATUS NtStatus;

        /* sizes of the field buffers in WCHARS */
        int username_sz, logon_domain_sz, oth_domains_sz, logon_server_sz;

        FIXME("Level 1 processing is partially implemented\n");
        oth_domains_sz = 1;
        logon_server_sz = 1;

        /* get some information first to estimate size of the buffer */
        ui0 = NULL;
        NetWkstaUserGetInfo(NULL, 0, (PBYTE *) &ui0);
        username_sz = lstrlenW(ui0->wkui0_username) + 1;

        ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
                                 POLICY_VIEW_LOCAL_INFORMATION,
                                 &PolicyHandle);
        if (NtStatus != STATUS_SUCCESS)
        {
            ERR("LsaOpenPolicyFailed with NT status %lx\n",
                LsaNtStatusToWinError(NtStatus));
            NetApiBufferFree(ui0);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation,
                                  (PVOID*) &DomainInfo);
        logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
        LsaClose(PolicyHandle);

        /* set up buffer */
        NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1) +
                             (username_sz + logon_domain_sz +
                              oth_domains_sz + logon_server_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        ui = (WKSTA_USER_INFO_1 *) *bufptr;
        ui->wkui1_username = (LPWSTR) (*bufptr + sizeof(WKSTA_USER_INFO_1));
        ui->wkui1_logon_domain = (LPWSTR) (
            ((PBYTE) ui->wkui1_username) + username_sz * sizeof(WCHAR));
        ui->wkui1_oth_domains = (LPWSTR) (
            ((PBYTE) ui->wkui1_logon_domain) +
            logon_domain_sz * sizeof(WCHAR));
        ui->wkui1_logon_server = (LPWSTR) (
            ((PBYTE) ui->wkui1_oth_domains) +
            oth_domains_sz * sizeof(WCHAR));

        /* get data */
        dwSize = username_sz;
        lstrcpyW(ui->wkui1_username, ui0->wkui0_username);
        NetApiBufferFree(ui0);

        lstrcpynW(ui->wkui1_logon_domain, DomainInfo->DomainName.Buffer,
                logon_domain_sz);
        LsaFreeMemory(DomainInfo);

        /* FIXME. Not implemented. Populated with empty strings */
        ui->wkui1_oth_domains[0] = 0;
        ui->wkui1_logon_server[0] = 0;
        break;
    }
    case 1101:
    {
        PWKSTA_USER_INFO_1101 ui;
        DWORD dwSize = 1;

        FIXME("Stub. Level 1101 processing is not implemented\n");
        /* FIXME see also wkui1_oth_domains for level 1 */

        /* set up buffer */
        NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1101) + dwSize * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PWKSTA_USER_INFO_1101) *bufptr;
        ui->wkui1101_oth_domains = (LPWSTR)(ui + 1);

        /* get data */
        ui->wkui1101_oth_domains[0] = 0;
        break;
    }
    default:
        ERR("Invalid level %ld is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetpGetComputerName  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetpGetComputerName(LPWSTR *Buffer)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    TRACE("(%p)\n", Buffer);
    NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) Buffer);
    if (GetComputerNameW(*Buffer,  &dwSize))
    {
        NetApiBufferReallocate(
            *Buffer, dwSize * sizeof(WCHAR),
            (LPVOID *) Buffer);
        return NERR_Success;
    }
    else
    {
        NetApiBufferFree(*Buffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

NET_API_STATUS WINAPI NetWkstaGetInfo( LPWSTR servername, DWORD level,
                                       LPBYTE* bufptr)
{
    NET_API_STATUS ret;

    TRACE("%p %ld %p\n", debugstr_w( servername ), level, bufptr );
    if (servername)
    {
        FIXME("remote computers not supported\n");
        return ERROR_INVALID_LEVEL;
    }
    if (!bufptr) return ERROR_INVALID_PARAMETER;

    switch (level)
    {
        case 100:
        {
            DWORD computerNameLen, domainNameLen, size;
            WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            NTSTATUS NtStatus;
           
            computerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerNameW(computerName, &computerNameLen);
            computerNameLen++; /* include NULL terminator */

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
                ret = LsaNtStatusToWinError(NtStatus);
            else
            {
                PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;

                LsaQueryInformationPolicy(PolicyHandle,
                 PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
                domainNameLen = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
                size = sizeof(WKSTA_INFO_100) + computerNameLen * sizeof(WCHAR)
                 + domainNameLen * sizeof(WCHAR);
                ret = NetApiBufferAllocate(size, (LPVOID *)bufptr);
                if (ret == NERR_Success)
                {
                    PWKSTA_INFO_100 info = (PWKSTA_INFO_100)*bufptr;
                    OSVERSIONINFOW verInfo;

                    info->wki100_platform_id = PLATFORM_ID_NT;
                    info->wki100_computername = (LPWSTR)(*bufptr +
                     sizeof(WKSTA_INFO_100));
                    memcpy(info->wki100_computername, computerName,
                     computerNameLen * sizeof(WCHAR));
                    info->wki100_langroup = (LPWSTR)(*bufptr +
                     sizeof(WKSTA_INFO_100) + computerNameLen * sizeof(WCHAR));
                    memcpy(info->wki100_langroup, DomainInfo->DomainName.Buffer,
                     domainNameLen * sizeof(WCHAR));
                    memset(&verInfo, 0, sizeof(verInfo));
                    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
                    GetVersionExW(&verInfo);
                    info->wki100_ver_major = verInfo.dwMajorVersion;
                    info->wki100_ver_minor = verInfo.dwMinorVersion;
                }
                LsaFreeMemory(DomainInfo);
                LsaClose(PolicyHandle);
            }
            break;
        }

        default:
            FIXME("level %ld unimplemented\n", level);
            ret = ERROR_INVALID_LEVEL;
    }
    return ret;
}
