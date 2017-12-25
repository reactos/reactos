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

#include "precomp.h"

#include <winnls.h>
#include <objbase.h>
#include <initguid.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static HMODULE hadvapi32;
static NTSTATUS (WINAPI *pLsaClose)(LSA_HANDLE);
static NTSTATUS (WINAPI *pLsaEnumerateAccountRights)(LSA_HANDLE,PSID,PLSA_UNICODE_STRING*,PULONG);
static NTSTATUS (WINAPI *pLsaFreeMemory)(PVOID);
static NTSTATUS (WINAPI *pLsaOpenPolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
static NTSTATUS (WINAPI *pLsaQueryInformationPolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
static BOOL     (WINAPI *pConvertSidToStringSidA)(PSID,LPSTR*);
#ifndef __REACTOS__
static BOOL     (WINAPI *pConvertStringSidToSidA)(LPCSTR,PSID*);
#endif
static NTSTATUS (WINAPI *pLsaLookupNames2)(LSA_HANDLE,ULONG,ULONG,PLSA_UNICODE_STRING,PLSA_REFERENCED_DOMAIN_LIST*,PLSA_TRANSLATED_SID2*);
static NTSTATUS (WINAPI *pLsaLookupSids)(LSA_HANDLE,ULONG,PSID*,LSA_REFERENCED_DOMAIN_LIST**,LSA_TRANSLATED_NAME**);
static PVOID    (WINAPI *pFreeSid)(PSID);

static BOOL init(void)
{
    hadvapi32 = GetModuleHandleA("advapi32.dll");

    pLsaClose = (void*)GetProcAddress(hadvapi32, "LsaClose");
    pLsaEnumerateAccountRights = (void*)GetProcAddress(hadvapi32, "LsaEnumerateAccountRights");
    pLsaFreeMemory = (void*)GetProcAddress(hadvapi32, "LsaFreeMemory");
    pLsaOpenPolicy = (void*)GetProcAddress(hadvapi32, "LsaOpenPolicy");
    pLsaQueryInformationPolicy = (void*)GetProcAddress(hadvapi32, "LsaQueryInformationPolicy");
    pConvertSidToStringSidA = (void*)GetProcAddress(hadvapi32, "ConvertSidToStringSidA");
    pConvertStringSidToSidA = (void*)GetProcAddress(hadvapi32, "ConvertStringSidToSidA");
    pLsaLookupNames2 = (void*)GetProcAddress(hadvapi32, "LsaLookupNames2");
    pLsaLookupSids = (void*)GetProcAddress(hadvapi32, "LsaLookupSids");
    pFreeSid = (void*)GetProcAddress(hadvapi32, "FreeSid");

    if (pLsaClose && pLsaEnumerateAccountRights && pLsaFreeMemory && pLsaOpenPolicy && pLsaQueryInformationPolicy && pConvertSidToStringSidA && pConvertStringSidToSidA && pFreeSid)
        return TRUE;

    return FALSE;
}

static void test_lsa(void)
{
    static WCHAR machineW[] = {'W','i','n','e','N','o','M','a','c','h','i','n','e',0};
    LSA_UNICODE_STRING machine;
    NTSTATUS status;
    LSA_HANDLE handle;
    LSA_OBJECT_ATTRIBUTES object_attributes;

    ZeroMemory(&object_attributes, sizeof(object_attributes));
    object_attributes.Length = sizeof(object_attributes);

    machine.Buffer = machineW;
    machine.Length = sizeof(machineW) - 2;
    machine.MaximumLength = sizeof(machineW);

    status = pLsaOpenPolicy( &machine, &object_attributes, POLICY_LOOKUP_NAMES, &handle);
    ok(status == RPC_NT_SERVER_UNAVAILABLE,
       "LsaOpenPolicy(POLICY_LOOKUP_NAMES) for invalid machine returned 0x%08x\n", status);

    status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_ALL_ACCESS, &handle);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
       "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08x\n", status);

    /* try a more restricted access mask if necessary */
    if (status == STATUS_ACCESS_DENIED) {
        trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION|POLICY_LOOKUP_NAMES\n");
        status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_VIEW_LOCAL_INFORMATION|POLICY_LOOKUP_NAMES, &handle);
        ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION|POLICY_LOOKUP_NAMES) returned 0x%08x\n", status);
    }

    if (status == STATUS_SUCCESS) {
        PPOLICY_AUDIT_EVENTS_INFO audit_events_info;
        PPOLICY_PRIMARY_DOMAIN_INFO primary_domain_info;
        PPOLICY_ACCOUNT_DOMAIN_INFO account_domain_info;
        PPOLICY_DNS_DOMAIN_INFO dns_domain_info;
        HANDLE token;
        BOOL ret;

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

        /* We need a valid SID to pass to LsaEnumerateAccountRights */
        ret = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
        ok(ret, "Unable to obtain process token, error %u\n", GetLastError( ));
        if (ret) {
            char buffer[64];
            DWORD len;
            TOKEN_USER *token_user = (TOKEN_USER *) buffer;
            ret = GetTokenInformation( token, TokenUser, (LPVOID) token_user, sizeof(buffer), &len );
            ok(ret || GetLastError( ) == ERROR_INSUFFICIENT_BUFFER, "Unable to obtain token information, error %u\n", GetLastError( ));
            if (! ret && GetLastError( ) == ERROR_INSUFFICIENT_BUFFER) {
                trace("Resizing buffer to %u.\n", len);
                token_user = LocalAlloc( 0, len );
                if (token_user != NULL)
                    ret = GetTokenInformation( token, TokenUser, (LPVOID) token_user, len, &len );
            }

            if (ret) {
                PLSA_UNICODE_STRING rights;
                ULONG rights_count;
                rights = (PLSA_UNICODE_STRING) 0xdeadbeaf;
                rights_count = 0xcafecafe;
                status = pLsaEnumerateAccountRights(handle, token_user->User.Sid, &rights, &rights_count);
                ok(status == STATUS_SUCCESS || status == STATUS_OBJECT_NAME_NOT_FOUND, "Unexpected status 0x%x\n", status);
                if (status == STATUS_SUCCESS)
                    pLsaFreeMemory( rights );
                else
                    ok(rights == NULL && rights_count == 0, "Expected rights and rights_count to be set to 0 on failure\n");
            }
            if (token_user != NULL && token_user != (TOKEN_USER *) buffer)
                LocalFree( token_user );
            CloseHandle( token );
        }

        status = pLsaClose(handle);
        ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08x\n", status);
    }
}

static void get_sid_info(PSID psid, LPSTR *user, LPSTR *dom)
{
    static char account[257], domain[257];
    DWORD user_size, dom_size;
    SID_NAME_USE use;
    BOOL ret;

    *user = account;
    *dom = domain;

    user_size = dom_size = 257;
    account[0] = domain[0] = 0;
    ret = LookupAccountSidA(NULL, psid, account, &user_size, domain, &dom_size, &use);
    ok(ret, "LookupAccountSidA failed %u\n", GetLastError());
}

static void test_LsaLookupNames2(void)
{
    static const WCHAR n1[] = {'L','O','C','A','L',' ','S','E','R','V','I','C','E'};
    static const WCHAR n2[] = {'N','T',' ','A','U','T','H','O','R','I','T','Y','\\','L','o','c','a','l','S','e','r','v','i','c','e'};

    NTSTATUS status;
    LSA_HANDLE handle;
    LSA_OBJECT_ATTRIBUTES attrs;
    PLSA_REFERENCED_DOMAIN_LIST domains;
    PLSA_TRANSLATED_SID2 sids;
    LSA_UNICODE_STRING name[3];
    LPSTR account, sid_dom;

    if (!pLsaLookupNames2)
    {
        win_skip("LsaLookupNames2 not available\n");
        return;
    }

    if ((PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_ENGLISH) ||
        (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH))
    {
        skip("Non-English locale (skipping LsaLookupNames2 tests)\n");
        return;
    }

    memset(&attrs, 0, sizeof(attrs));
    attrs.Length = sizeof(attrs);

    status = pLsaOpenPolicy(NULL, &attrs, POLICY_ALL_ACCESS, &handle);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
       "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08x\n", status);

    /* try a more restricted access mask if necessary */
    if (status == STATUS_ACCESS_DENIED)
    {
        trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION\n");
        status = pLsaOpenPolicy(NULL, &attrs, POLICY_LOOKUP_NAMES, &handle);
        ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION) returned 0x%08x\n", status);
    }
    if (status != STATUS_SUCCESS)
    {
        skip("Cannot acquire policy handle\n");
        return;
    }

    name[0].Buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(n1));
    name[0].Length = name[0].MaximumLength = sizeof(n1);
    memcpy(name[0].Buffer, n1, sizeof(n1));

    name[1].Buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(n1));
    name[1].Length = name[1].MaximumLength = sizeof(n1) - sizeof(WCHAR);
    memcpy(name[1].Buffer, n1, sizeof(n1) - sizeof(WCHAR));

    name[2].Buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(n2));
    name[2].Length = name[2].MaximumLength = sizeof(n2);
    memcpy(name[2].Buffer, n2, sizeof(n2));

    /* account name only */
    sids = NULL;
    domains = NULL;
    status = pLsaLookupNames2(handle, 0, 1, &name[0], &domains, &sids);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %x)\n", status);
    ok(sids[0].Use == SidTypeWellKnownGroup, "expected SidTypeWellKnownGroup, got %u\n", sids[0].Use);
    ok(sids[0].Flags == 0, "expected 0, got 0x%08x\n", sids[0].Flags);
    ok(domains->Entries == 1, "expected 1, got %u\n", domains->Entries);
    get_sid_info(sids[0].Sid, &account, &sid_dom);
    ok(!strcmp(account, "LOCAL SERVICE"), "expected \"LOCAL SERVICE\", got \"%s\"\n", account);
    ok(!strcmp(sid_dom, "NT AUTHORITY"), "expected \"NT AUTHORITY\", got \"%s\"\n", sid_dom);
    pLsaFreeMemory(sids);
    pLsaFreeMemory(domains);

    /* unknown account name */
    sids = NULL;
    domains = NULL;
    status = pLsaLookupNames2(handle, 0, 1, &name[1], &domains, &sids);
    ok(status == STATUS_NONE_MAPPED, "expected STATUS_NONE_MAPPED, got %x)\n", status);
    ok(sids[0].Use == SidTypeUnknown, "expected SidTypeUnknown, got %u\n", sids[0].Use);
    ok(sids[0].Flags == 0, "expected 0, got 0x%08x\n", sids[0].Flags);
    ok(domains->Entries == 0, "expected 0, got %u\n", domains->Entries);
    pLsaFreeMemory(sids);
    pLsaFreeMemory(domains);

    /* account + domain */
    sids = NULL;
    domains = NULL;
    status = pLsaLookupNames2(handle, 0, 1, &name[2], &domains, &sids);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %x)\n", status);
    ok(sids[0].Use == SidTypeWellKnownGroup, "expected SidTypeWellKnownGroup, got %u\n", sids[0].Use);
    ok(sids[0].Flags == 0, "expected 0, got 0x%08x\n", sids[0].Flags);
    ok(domains->Entries == 1, "expected 1, got %u\n", domains->Entries);
    get_sid_info(sids[0].Sid, &account, &sid_dom);
    ok(!strcmp(account, "LOCAL SERVICE"), "expected \"LOCAL SERVICE\", got \"%s\"\n", account);
    ok(!strcmp(sid_dom, "NT AUTHORITY"), "expected \"NT AUTHORITY\", got \"%s\"\n", sid_dom);
    pLsaFreeMemory(sids);
    pLsaFreeMemory(domains);

    /* all three */
    sids = NULL;
    domains = NULL;
    status = pLsaLookupNames2(handle, 0, 3, name, &domains, &sids);
    ok(status == STATUS_SOME_NOT_MAPPED, "expected STATUS_SOME_NOT_MAPPED, got %x)\n", status);
    ok(sids[0].Use == SidTypeWellKnownGroup, "expected SidTypeWellKnownGroup, got %u\n", sids[0].Use);
    ok(sids[1].Use == SidTypeUnknown, "expected SidTypeUnknown, got %u\n", sids[1].Use);
    ok(sids[2].Use == SidTypeWellKnownGroup, "expected SidTypeWellKnownGroup, got %u\n", sids[2].Use);
    ok(sids[0].DomainIndex == 0, "expected 0, got %u\n", sids[0].DomainIndex);
    ok(domains->Entries == 1, "expected 1, got %u\n", domains->Entries);
    pLsaFreeMemory(sids);
    pLsaFreeMemory(domains);

    HeapFree(GetProcessHeap(), 0, name[0].Buffer);
    HeapFree(GetProcessHeap(), 0, name[1].Buffer);
    HeapFree(GetProcessHeap(), 0, name[2].Buffer);

    status = pLsaClose(handle);
    ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08x\n", status);
}

static void test_LsaLookupSids(void)
{
    LSA_REFERENCED_DOMAIN_LIST *list;
    LSA_OBJECT_ATTRIBUTES attrs;
    LSA_TRANSLATED_NAME *names;
    LSA_HANDLE policy;
    TOKEN_USER *user;
    NTSTATUS status;
    HANDLE token;
    DWORD size;
    BOOL ret;

    memset(&attrs, 0, sizeof(attrs));
    attrs.Length = sizeof(attrs);

    status = pLsaOpenPolicy(NULL, &attrs, POLICY_LOOKUP_NAMES, &policy);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);

    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &token);
    ok(ret, "got %d\n", ret);

    ret = GetTokenInformation(token, TokenUser, NULL, 0, &size);
    ok(!ret, "got %d\n", ret);

    user = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetTokenInformation(token, TokenUser, user, size, &size);
    ok(ret, "got %d\n", ret);

    status = pLsaLookupSids(policy, 1, &user->User.Sid, &list, &names);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);

    ok(list->Entries > 0, "got %d\n", list->Entries);
    if (list->Entries)
    {
       ok((char*)list->Domains - (char*)list > 0, "%p, %p\n", list, list->Domains);
       ok((char*)list->Domains[0].Sid - (char*)list->Domains > 0, "%p, %p\n", list->Domains, list->Domains[0].Sid);
       ok(list->Domains[0].Name.MaximumLength > list->Domains[0].Name.Length, "got %d, %d\n", list->Domains[0].Name.MaximumLength,
           list->Domains[0].Name.Length);
    }

    pLsaFreeMemory(names);
    pLsaFreeMemory(list);

    HeapFree(GetProcessHeap(), 0, user);

    CloseHandle(token);

    status = pLsaClose(policy);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);
}

static void test_LsaLookupSids_NullBuffers(void)
{
    LSA_REFERENCED_DOMAIN_LIST *list;
    LSA_OBJECT_ATTRIBUTES attrs;
    LSA_TRANSLATED_NAME *names;
    LSA_HANDLE policy;
    NTSTATUS status;
    BOOL ret;
    PSID sid;

    memset(&attrs, 0, sizeof(attrs));
    attrs.Length = sizeof(attrs);

    status = pLsaOpenPolicy(NULL, &attrs, POLICY_LOOKUP_NAMES, &policy);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);

    ret = pConvertStringSidToSidA("S-1-1-0", &sid);
    ok(ret == TRUE, "pConvertStringSidToSidA returned false\n");

    status = pLsaLookupSids(policy, 1, &sid, &list, &names);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);

    ok(list->Entries > 0, "got %d\n", list->Entries);

    if (list->Entries)
    {
       ok((char*)list->Domains - (char*)list > 0, "%p, %p\n", list, list->Domains);
       ok((char*)list->Domains[0].Sid - (char*)list->Domains > 0, "%p, %p\n", list->Domains, list->Domains[0].Sid);
       ok(list->Domains[0].Name.MaximumLength > list->Domains[0].Name.Length, "got %d, %d\n", list->Domains[0].Name.MaximumLength,
           list->Domains[0].Name.Length);
       ok(list->Domains[0].Name.Buffer != NULL, "domain[0] name buffer is null\n");
    }

    pLsaFreeMemory(names);
    pLsaFreeMemory(list);

    pFreeSid(sid);

    status = pLsaClose(policy);
    ok(status == STATUS_SUCCESS, "got 0x%08x\n", status);
}

START_TEST(lsa)
{
    if (!init()) {
        win_skip("Needed functions are not available\n");
        return;
    }

    test_lsa();
    test_LsaLookupNames2();
    test_LsaLookupSids();
    test_LsaLookupSids_NullBuffers();
}
