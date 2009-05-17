/*
 * Unit tests for lsa functions
 *
 * Copyright (c) 2006 Robert Reif
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

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "sddl.h"
#include "winnls.h"
#include "objbase.h"
#include "initguid.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static HMODULE hadvapi32;
static NTSTATUS (WINAPI *pLsaClose)(LSA_HANDLE);
static NTSTATUS (WINAPI *pLsaFreeMemory)(PVOID);
static NTSTATUS (WINAPI *pLsaOpenPolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
static NTSTATUS (WINAPI *pLsaQueryInformationPolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
static BOOL     (WINAPI *pConvertSidToStringSidA)(PSID pSid, LPSTR *str);

static BOOL init(void)
{
    hadvapi32 = GetModuleHandle("advapi32.dll");

    pLsaClose = (void*)GetProcAddress(hadvapi32, "LsaClose");
    pLsaFreeMemory = (void*)GetProcAddress(hadvapi32, "LsaFreeMemory");
    pLsaOpenPolicy = (void*)GetProcAddress(hadvapi32, "LsaOpenPolicy");
    pLsaQueryInformationPolicy = (void*)GetProcAddress(hadvapi32, "LsaQueryInformationPolicy");
    pConvertSidToStringSidA = (void*)GetProcAddress(hadvapi32, "ConvertSidToStringSidA");

    if (pLsaClose && pLsaFreeMemory && pLsaOpenPolicy && pLsaQueryInformationPolicy && pConvertSidToStringSidA)
        return TRUE;

    return FALSE;
}

static void test_lsa(void)
{
    NTSTATUS status;
    LSA_HANDLE handle;
    LSA_OBJECT_ATTRIBUTES object_attributes;

    ZeroMemory(&object_attributes, sizeof(object_attributes));
    object_attributes.Length = sizeof(object_attributes);

    status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_ALL_ACCESS, &handle);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
       "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08x\n", status);

    /* try a more restricted access mask if necessary */
    if (status == STATUS_ACCESS_DENIED) {
        trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION\n");
        status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_VIEW_LOCAL_INFORMATION, &handle);
        ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION) returned 0x%08x\n", status);
    }

    if (status == STATUS_SUCCESS) {
        PPOLICY_AUDIT_EVENTS_INFO audit_events_info;
        PPOLICY_PRIMARY_DOMAIN_INFO primary_domain_info;
        PPOLICY_ACCOUNT_DOMAIN_INFO account_domain_info;
        PPOLICY_DNS_DOMAIN_INFO dns_domain_info;

        status = pLsaQueryInformationPolicy(handle, PolicyAuditEventsInformation, (PVOID*)&audit_events_info);
        if (status == STATUS_ACCESS_DENIED)
            skip("Not enough rights to retrieve PolicyAuditEventsInformation\n");
        else
            ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyAuditEventsInformation) failed, returned 0x%08x\n", status);
        if (status == STATUS_SUCCESS) {
            pLsaFreeMemory((LPVOID)audit_events_info);
        }

        status = pLsaQueryInformationPolicy(handle, PolicyPrimaryDomainInformation, (PVOID*)&primary_domain_info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyPrimaryDomainInformation) failed, returned 0x%08x\n", status);
        if (status == STATUS_SUCCESS) {
            if (primary_domain_info->Sid) {
                LPSTR strsid;
                if (pConvertSidToStringSidA(primary_domain_info->Sid, &strsid))
                {
                    if (primary_domain_info->Name.Buffer) {
                        LPSTR name = NULL;
                        UINT len;
                        len = WideCharToMultiByte( CP_ACP, 0, primary_domain_info->Name.Buffer, -1, NULL, 0, NULL, NULL );
                        name = LocalAlloc( 0, len );
                        WideCharToMultiByte( CP_ACP, 0, primary_domain_info->Name.Buffer, -1, name, len, NULL, NULL );
                        trace("  name: %s sid: %s\n", name, strsid);
                        LocalFree( name );
                    } else
                        trace("  name: NULL sid: %s\n", strsid);
                    LocalFree( strsid );
                }
                else
                    trace("invalid sid\n");
            }
            else
                trace("Running on a standalone system.\n");
            pLsaFreeMemory((LPVOID)primary_domain_info);
        }

        status = pLsaQueryInformationPolicy(handle, PolicyAccountDomainInformation, (PVOID*)&account_domain_info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyAccountDomainInformation) failed, returned 0x%08x\n", status);
        if (status == STATUS_SUCCESS) {
            pLsaFreeMemory((LPVOID)account_domain_info);
        }

        /* This isn't supported in NT4 */
        status = pLsaQueryInformationPolicy(handle, PolicyDnsDomainInformation, (PVOID*)&dns_domain_info);
        ok(status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER,
           "LsaQueryInformationPolicy(PolicyDnsDomainInformation) failed, returned 0x%08x\n", status);
        if (status == STATUS_SUCCESS) {
            if (dns_domain_info->Sid || !IsEqualGUID(&dns_domain_info->DomainGuid, &GUID_NULL)) {
                LPSTR strsid = NULL;
                LPSTR name = NULL;
                LPSTR domain = NULL;
                LPSTR forest = NULL;
                LPSTR guidstr = NULL;
                WCHAR guidstrW[64];
                UINT len;
                guidstrW[0] = '\0';
                pConvertSidToStringSidA(dns_domain_info->Sid, &strsid);
                StringFromGUID2(&dns_domain_info->DomainGuid, guidstrW, sizeof(guidstrW)/sizeof(WCHAR));
                len = WideCharToMultiByte( CP_ACP, 0, guidstrW, -1, NULL, 0, NULL, NULL );
                guidstr = LocalAlloc( 0, len );
                WideCharToMultiByte( CP_ACP, 0, guidstrW, -1, guidstr, len, NULL, NULL );
                if (dns_domain_info->Name.Buffer) {
                    len = WideCharToMultiByte( CP_ACP, 0, dns_domain_info->Name.Buffer, -1, NULL, 0, NULL, NULL );
                    name = LocalAlloc( 0, len );
                    WideCharToMultiByte( CP_ACP, 0, dns_domain_info->Name.Buffer, -1, name, len, NULL, NULL );
                }
                if (dns_domain_info->DnsDomainName.Buffer) {
                    len = WideCharToMultiByte( CP_ACP, 0, dns_domain_info->DnsDomainName.Buffer, -1, NULL, 0, NULL, NULL );
                    domain = LocalAlloc( 0, len );
                    WideCharToMultiByte( CP_ACP, 0, dns_domain_info->DnsDomainName.Buffer, -1, domain, len, NULL, NULL );
                }
                if (dns_domain_info->DnsForestName.Buffer) {
                    len = WideCharToMultiByte( CP_ACP, 0, dns_domain_info->DnsForestName.Buffer, -1, NULL, 0, NULL, NULL );
                    forest = LocalAlloc( 0, len );
                    WideCharToMultiByte( CP_ACP, 0, dns_domain_info->DnsForestName.Buffer, -1, forest, len, NULL, NULL );
                }
                trace("  name: %s domain: %s forest: %s guid: %s sid: %s\n",
                      name ? name : "NULL", domain ? domain : "NULL",
                      forest ? forest : "NULL", guidstr, strsid ? strsid : "NULL");
                LocalFree( name );
                LocalFree( forest );
                LocalFree( domain );
                LocalFree( guidstr );
                LocalFree( strsid );
            }
            else
                trace("Running on a standalone system.\n");
            pLsaFreeMemory((LPVOID)dns_domain_info);
        }

        status = pLsaClose(handle);
        ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08x\n", status);
    }
}

START_TEST(lsa)
{
    if (!init()) {
        win_skip("Needed functions are not available\n");
        return;
    }

    test_lsa();
}
