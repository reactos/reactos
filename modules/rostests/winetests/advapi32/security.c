/*
 * Unit tests for security functions
 *
 * Copyright (c) 2004 Mike McCormack
 * Copyright (c) 2011 Dmitry Timoshkov
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
#include "winerror.h"
#include "winternl.h"
#include "aclapi.h"
#include "winnt.h"
#include "sddl.h"
#include "ntsecapi.h"
#include "lmcons.h"

#include "wine/test.h"

#ifdef __REACTOS__
/* FIXME: Removing these hacks requires fixing our incompatible wine/test.h and wine/debug.h. */
#ifndef wine_dbg_sprintf
static inline const char* wine_dbg_sprintf(const char* format, ...)
{
    static char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}
#endif

struct debug_info
{
    unsigned int str_pos;       /* current position in strings buffer */
    unsigned int out_pos;       /* current position in output buffer */
    char         strings[1020]; /* buffer for temporary strings */
    char         output[1020];  /* current output line */
};

C_ASSERT( sizeof(struct debug_info) == 0x800 );

static inline struct debug_info *get_info(void)
{
#ifdef _WIN64
    return (struct debug_info *)((TEB32 *)((char *)NtCurrentTeb() + 0x2000) + 1);
#else
    return (struct debug_info *)(NtCurrentTeb() + 1);
#endif
}

const char * __cdecl __wine_dbg_strdup( const char *str )
{
    struct debug_info *info = get_info();
    unsigned int pos = info->str_pos;
    size_t n = strlen( str ) + 1;

    if (pos + n > sizeof(info->strings)) pos = 0;
    info->str_pos = pos + n;
    return memcpy( info->strings + pos, str, n );
}

BOOL WINAPI GetWindowsAccountDomainSid(_In_ PSID  pSid, _Out_opt_ PSID  pDomainSid, _Inout_ DWORD *cbDomainSid);
BOOL WINAPI AddAuditAccessAceEx(_Inout_ PACL pAcl, _In_ DWORD dwAceRevision, _In_ DWORD AceFlags, _In_ DWORD dwAccessMask, _In_ PSID pSid, _In_ BOOL bAuditSuccess, _In_ BOOL bAuditFailure);
BOOL WINAPI EqualDomainSid(_In_ PSID pSid1, _In_ PSID pSid2, _Out_ BOOL* pfEqual);
#endif

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#endif

/* PROCESS_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define PROCESS_ALL_ACCESS_NT4 (PROCESS_ALL_ACCESS & ~0xf000)
#define PROCESS_ALL_ACCESS_VISTA (PROCESS_ALL_ACCESS | 0xf000)

#ifndef EVENT_QUERY_STATE
#define EVENT_QUERY_STATE 0x0001
#endif

#ifndef SEMAPHORE_QUERY_STATE
#define SEMAPHORE_QUERY_STATE 0x0001
#endif

#ifndef THREAD_SET_LIMITED_INFORMATION
#define THREAD_SET_LIMITED_INFORMATION 0x0400
#define THREAD_QUERY_LIMITED_INFORMATION 0x0800
#endif

#define THREAD_ALL_ACCESS_NT4 (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff)
#define THREAD_ALL_ACCESS_VISTA (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xffff)

#define expect_eq(expr, value, type, format) { type ret_ = expr; ok((value) == ret_, #expr " expected " format "  got " format "\n", (value), (ret_)); }

static BOOL (WINAPI *pAddMandatoryAce)(PACL,DWORD,DWORD,DWORD,PSID);
static VOID (WINAPI *pBuildTrusteeWithSidA)( PTRUSTEEA pTrustee, PSID pSid );
static VOID (WINAPI *pBuildTrusteeWithNameA)( PTRUSTEEA pTrustee, LPSTR pName );
static VOID (WINAPI *pBuildTrusteeWithObjectsAndNameA)( PTRUSTEEA pTrustee,
                                                          POBJECTS_AND_NAME_A pObjName,
                                                          SE_OBJECT_TYPE ObjectType,
                                                          LPSTR ObjectTypeName,
                                                          LPSTR InheritedObjectTypeName,
                                                          LPSTR Name );
static VOID (WINAPI *pBuildTrusteeWithObjectsAndSidA)( PTRUSTEEA pTrustee,
                                                         POBJECTS_AND_SID pObjSid,
                                                         GUID* pObjectGuid,
                                                         GUID* pInheritedObjectGuid,
                                                         PSID pSid );
static LPSTR (WINAPI *pGetTrusteeNameA)( PTRUSTEEA pTrustee );
static DWORD (WINAPI *pRtlAdjustPrivilege)(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);
static NTSTATUS (WINAPI *pNtAccessCheck)(PSECURITY_DESCRIPTOR, HANDLE, ACCESS_MASK, PGENERIC_MAPPING,
                                         PPRIVILEGE_SET, PULONG, PULONG, NTSTATUS*);
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)(LPCWSTR,PUNICODE_STRING,PWSTR*,CURDIR*);

static HMODULE hmod;
static int     myARGC;
static char**  myARGV;

static const char* debugstr_sid(PSID sid)
{
    LPSTR sidstr;
    DWORD le = GetLastError();
    const char *res;

    if (!ConvertSidToStringSidA(sid, &sidstr))
        res = wine_dbg_sprintf("ConvertSidToStringSidA failed le=%lu", GetLastError());
    else
    {
        res = __wine_dbg_strdup(sidstr);
        LocalFree(sidstr);
    }
    /* Restore the last error in case ConvertSidToStringSidA() modified it */
    SetLastError(le);
    return res;
}

struct sidRef
{
    SID_IDENTIFIER_AUTHORITY auth;
    const char *refStr;
};

static void init(void)
{
    HMODULE hntdll;

    hntdll = GetModuleHandleA("ntdll.dll");
    pNtAccessCheck = (void *)GetProcAddress( hntdll, "NtAccessCheck" );
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");

    hmod = GetModuleHandleA("advapi32.dll");
    pAddMandatoryAce = (void *)GetProcAddress(hmod, "AddMandatoryAce");

    myARGC = winetest_get_mainargs( &myARGV );
}

static SECURITY_DESCRIPTOR* test_get_security_descriptor(HANDLE handle, int line)
{
    /* use free(sd); when done */
    DWORD ret, length, needed;
    SECURITY_DESCRIPTOR *sd;

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(handle, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  NULL, 0, &needed);
    ok_(__FILE__, line)(!ret, "GetKernelObjectSecurity should fail\n");
    ok_(__FILE__, line)(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok_(__FILE__, line)(needed != 0xdeadbeef, "GetKernelObjectSecurity should return required buffer length\n");

    length = needed;
    sd = malloc(length);

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(handle, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  sd, length, &needed);
    ok_(__FILE__, line)(ret, "GetKernelObjectSecurity error %ld\n", GetLastError());
    ok_(__FILE__, line)(needed == length || needed == 0 /* file, pipe */, "GetKernelObjectSecurity should return %lu instead of %lu\n", length, needed);
    return sd;
}

static void test_owner_equal(HANDLE Handle, PSID expected, int line)
{
    BOOL res;
    SECURITY_DESCRIPTOR *queriedSD = NULL;
    PSID owner;
    BOOL owner_defaulted;

    queriedSD = test_get_security_descriptor( Handle, line );

    res = GetSecurityDescriptorOwner(queriedSD, &owner, &owner_defaulted);
    ok_(__FILE__, line)(res, "GetSecurityDescriptorOwner failed with error %ld\n", GetLastError());

#ifdef __REACTOS__
    /* The call to EqualSid below crashes on WS03. */
    if (GetNTVersion() > _WIN32_WINNT_WS03)
#endif
    ok_(__FILE__, line)(EqualSid(owner, expected), "Owner SIDs are not equal %s != %s\n",
                        debugstr_sid(owner), debugstr_sid(expected));
    ok_(__FILE__, line)(!owner_defaulted, "Defaulted is true\n");

    free(queriedSD);
}

static void test_group_equal(HANDLE Handle, PSID expected, int line)
{
    BOOL res;
    SECURITY_DESCRIPTOR *queriedSD = NULL;
    PSID group;
    BOOL group_defaulted;

    queriedSD = test_get_security_descriptor( Handle, line );

    res = GetSecurityDescriptorGroup(queriedSD, &group, &group_defaulted);
    ok_(__FILE__, line)(res, "GetSecurityDescriptorGroup failed with error %ld\n", GetLastError());

#ifdef __REACTOS__
    /* The call to EqualSid below crashes on WS03. */
    if (GetNTVersion() > _WIN32_WINNT_WS03)
#endif
    ok_(__FILE__, line)(EqualSid(group, expected), "Group SIDs are not equal %s != %s\n",
                        debugstr_sid(group), debugstr_sid(expected));
    ok_(__FILE__, line)(!group_defaulted, "Defaulted is true\n");

    free(queriedSD);
}

static void test_ConvertStringSidToSid(void)
{
    struct sidRef refs[] = {
     { { {0x00,0x00,0x33,0x44,0x55,0x66} }, "S-1-860116326-1" },
     { { {0x00,0x00,0x01,0x02,0x03,0x04} }, "S-1-16909060-1"  },
     { { {0x00,0x00,0x00,0x01,0x02,0x03} }, "S-1-66051-1"     },
     { { {0x00,0x00,0x00,0x00,0x01,0x02} }, "S-1-258-1"       },
     { { {0x00,0x00,0x00,0x00,0x00,0x02} }, "S-1-2-1"         },
     { { {0x00,0x00,0x00,0x00,0x00,0x0c} }, "S-1-12-1"        },
    };
    static const struct
    {
        const char *name;
        const char *sid;
        unsigned int optional;
    }
    str_to_sid_tests[] =
    {
        { "WD", "S-1-1-0" },
        { "wD", "S-1-1-0" },
        { "CO", "S-1-3-0" },
        { "CG", "S-1-3-1" },
        { "OW", "S-1-3-4", 1 }, /* Vista+ */
        { "NU", "S-1-5-2" },
        { "IU", "S-1-5-4" },
        { "SU", "S-1-5-6" },
        { "AN", "S-1-5-7" },
        { "ED", "S-1-5-9" },
        { "PS", "S-1-5-10" },
        { "AU", "S-1-5-11" },
        { "RC", "S-1-5-12" },
        { "SY", "S-1-5-18" },
        { "LS", "S-1-5-19" },
        { "NS", "S-1-5-20" },
        { "LA", "S-1-5-21-*-*-*-500" },
        { "LG", "S-1-5-21-*-*-*-501" },
        { "BO", "S-1-5-32-551" },
        { "BA", "S-1-5-32-544" },
        { "BU", "S-1-5-32-545" },
        { "BG", "S-1-5-32-546" },
        { "PU", "S-1-5-32-547" },
        { "AO", "S-1-5-32-548" },
        { "SO", "S-1-5-32-549" },
        { "PO", "S-1-5-32-550" },
        { "RE", "S-1-5-32-552" },
        { "RU", "S-1-5-32-554" },
        { "RD", "S-1-5-32-555" },
        { "NO", "S-1-5-32-556" },
        { "AC", "S-1-15-2-1", 1 }, /* Win8+ */
        { "CA", "", 1 },
        { "DA", "", 1 },
        { "DC", "", 1 },
        { "DD", "", 1 },
        { "DG", "", 1 },
        { "DU", "", 1 },
        { "EA", "", 1 },
        { "PA", "", 1 },
        { "RS", "", 1 },
        { "SA", "", 1 },
#ifdef __REACTOS__
        { "s-1-12-1", "S-1-12-1", 1 },         /* Crashes on ReactOS if not optional. */
        { "S-0x1-0XC-0x1a", "S-1-12-26", 1 },  /* Crashes on ReactOS if not optional. */
#else
        { "s-1-12-1", "S-1-12-1" },
        { "S-0x1-0XC-0x1a", "S-1-12-26" },
#endif
    };

    const char noSubAuthStr[] = "S-1-5";
    unsigned int i;
    PSID psid = NULL;
    SID *pisid;
    BOOL r, ret;
    LPSTR str = NULL;

    r = ConvertStringSidToSidA( NULL, NULL );
    ok( !r, "expected failure with NULL parameters\n" );
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED )
        return;
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = ConvertStringSidToSidA( refs[0].refStr, NULL );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = ConvertStringSidToSidA( NULL, &psid );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = ConvertStringSidToSidA( noSubAuthStr, &psid );
    ok( !r,
     "expected failure with no sub authorities\n" );
    ok( GetLastError() == ERROR_INVALID_SID,
     "expected GetLastError() is ERROR_INVALID_SID, got %ld\n",
     GetLastError() );

    r = ConvertStringSidToSidA( "WDandmorecharacters", &psid );
    ok( !r,
     "expected failure with too many characters\n" );
    ok( GetLastError() == ERROR_INVALID_SID,
     "expected GetLastError() is ERROR_INVALID_SID, got %ld\n",
     GetLastError() );

    r = ConvertStringSidToSidA( "WD)", &psid );
    ok( !r,
     "expected failure with too many characters\n" );
    ok( GetLastError() == ERROR_INVALID_SID,
     "expected GetLastError() is ERROR_INVALID_SID, got %ld\n",
     GetLastError() );

    ok(ConvertStringSidToSidA("S-1-5-21-93476-23408-4576", &psid), "ConvertStringSidToSidA failed\n");
    pisid = psid;
    ok(pisid->SubAuthorityCount == 4, "Invalid sub authority count - expected 4, got %d\n", pisid->SubAuthorityCount);
    ok(pisid->SubAuthority[0] == 21, "Invalid subauthority 0 - expected 21, got %ld\n", pisid->SubAuthority[0]);
    ok(pisid->SubAuthority[3] == 4576, "Invalid subauthority 0 - expected 4576, got %ld\n", pisid->SubAuthority[3]);
    LocalFree(str);
    LocalFree(psid);

    for( i = 0; i < ARRAY_SIZE(refs); i++ )
    {
        r = AllocateAndInitializeSid( &refs[i].auth, 1,1,0,0,0,0,0,0,0,
         &psid );
        ok( r, "failed to allocate sid\n" );
        r = ConvertSidToStringSidA( psid, &str );
        ok( r, "failed to convert sid\n" );
        if (r)
        {
            ok( !strcmp( str, refs[i].refStr ),
                "incorrect sid, expected %s, got %s\n", refs[i].refStr, str );
            LocalFree( str );
        }
        if( psid )
            FreeSid( psid );

        r = ConvertStringSidToSidA( refs[i].refStr, &psid );
        ok( r, "failed to parse sid string\n" );
        pisid = psid;
        ok( pisid &&
         !memcmp( pisid->IdentifierAuthority.Value, refs[i].auth.Value,
         sizeof(refs[i].auth) ),
         "string sid %s didn't parse to expected value\n"
         "(got 0x%04x%08lx, expected 0x%04x%08lx)\n",
         refs[i].refStr,
         MAKEWORD( pisid->IdentifierAuthority.Value[1],
         pisid->IdentifierAuthority.Value[0] ),
         MAKELONG( MAKEWORD( pisid->IdentifierAuthority.Value[5],
         pisid->IdentifierAuthority.Value[4] ),
         MAKEWORD( pisid->IdentifierAuthority.Value[3],
         pisid->IdentifierAuthority.Value[2] ) ),
         MAKEWORD( refs[i].auth.Value[1], refs[i].auth.Value[0] ),
         MAKELONG( MAKEWORD( refs[i].auth.Value[5], refs[i].auth.Value[4] ),
         MAKEWORD( refs[i].auth.Value[3], refs[i].auth.Value[2] ) ) );
        if( psid )
            LocalFree( psid );
    }

    for (i = 0; i < ARRAY_SIZE(str_to_sid_tests); i++)
    {
        char *str;

        ret = ConvertStringSidToSidA(str_to_sid_tests[i].name, &psid);
        if (!ret && str_to_sid_tests[i].optional)
        {
            skip("%u: failed to convert %s.\n", i, str_to_sid_tests[i].name);
            continue;
        }
        ok(ret, "%u: failed to convert string to sid.\n", i);

        if (str_to_sid_tests[i].optional || !strcmp(str_to_sid_tests[i].name, "LA") ||
            !strcmp(str_to_sid_tests[i].name, "LG"))
        {
            LocalFree(psid);
            continue;
        }

        ret = ConvertSidToStringSidA(psid, &str);
        ok(ret, "%u: failed to convert SID to string.\n", i);
        ok(!strcmp(str, str_to_sid_tests[i].sid), "%u: unexpected sid %s.\n", i, str);
        LocalFree(psid);
        LocalFree(str);
    }
}

static void test_trustee(void)
{
    GUID ObjectType = {0x12345678, 0x1234, 0x5678, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    GUID InheritedObjectType = {0x23456789, 0x2345, 0x6786, {0x2, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
    GUID ZeroGuid;
    OBJECTS_AND_NAME_A oan;
    OBJECTS_AND_SID oas;
    TRUSTEEA trustee;
    PSID psid;
    char szObjectTypeName[] = "ObjectTypeName";
    char szInheritedObjectTypeName[] = "InheritedObjectTypeName";
    char szTrusteeName[] = "szTrusteeName";
    SID_IDENTIFIER_AUTHORITY auth = { {0x11,0x22,0,0,0, 0} };

    memset( &ZeroGuid, 0x00, sizeof (ZeroGuid) );

    pBuildTrusteeWithSidA = (void *)GetProcAddress( hmod, "BuildTrusteeWithSidA" );
    pBuildTrusteeWithNameA = (void *)GetProcAddress( hmod, "BuildTrusteeWithNameA" );
    pBuildTrusteeWithObjectsAndNameA = (void *)GetProcAddress (hmod, "BuildTrusteeWithObjectsAndNameA" );
    pBuildTrusteeWithObjectsAndSidA = (void *)GetProcAddress (hmod, "BuildTrusteeWithObjectsAndSidA" );
    pGetTrusteeNameA = (void *)GetProcAddress (hmod, "GetTrusteeNameA" );
    if( !pBuildTrusteeWithSidA || !pBuildTrusteeWithNameA ||
        !pBuildTrusteeWithObjectsAndNameA || !pBuildTrusteeWithObjectsAndSidA ||
        !pGetTrusteeNameA )
        return;

    if ( ! AllocateAndInitializeSid( &auth, 1, 42, 0,0,0,0,0,0,0,&psid ) )
    {
        trace( "failed to init SID\n" );
       return;
    }

    /* test BuildTrusteeWithSidA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithSidA( &trustee, psid );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_SID, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == psid, "ptstrName wrong\n" );

    /* test BuildTrusteeWithObjectsAndSidA (test 1) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oas, 0xff, sizeof(oas) );
    pBuildTrusteeWithObjectsAndSidA(&trustee, &oas, &ObjectType,
                                    &InheritedObjectType, psid);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oas, "ptstrName wrong\n");
 
    ok(oas.ObjectsPresent == (ACE_OBJECT_TYPE_PRESENT | ACE_INHERITED_OBJECT_TYPE_PRESENT), "ObjectsPresent wrong\n");
    ok(!memcmp(&oas.ObjectTypeGuid, &ObjectType, sizeof(GUID)), "ObjectTypeGuid wrong\n");
    ok(!memcmp(&oas.InheritedObjectTypeGuid, &InheritedObjectType, sizeof(GUID)), "InheritedObjectTypeGuid wrong\n");
    ok(oas.pSid == psid, "pSid wrong\n");

    /* test GetTrusteeNameA */
    ok(pGetTrusteeNameA(&trustee) == (LPSTR)&oas, "GetTrusteeName returned wrong value\n");

    /* test BuildTrusteeWithObjectsAndSidA (test 2) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oas, 0xff, sizeof(oas) );
    pBuildTrusteeWithObjectsAndSidA(&trustee, &oas, NULL,
                                    &InheritedObjectType, psid);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oas, "ptstrName wrong\n");
 
    ok(oas.ObjectsPresent == ACE_INHERITED_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(!memcmp(&oas.ObjectTypeGuid, &ZeroGuid, sizeof(GUID)), "ObjectTypeGuid wrong\n");
    ok(!memcmp(&oas.InheritedObjectTypeGuid, &InheritedObjectType, sizeof(GUID)), "InheritedObjectTypeGuid wrong\n");
    ok(oas.pSid == psid, "pSid wrong\n");

    FreeSid( psid );

    /* test BuildTrusteeWithNameA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithNameA( &trustee, szTrusteeName );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_NAME, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == szTrusteeName, "ptstrName wrong\n" );

    /* test BuildTrusteeWithObjectsAndNameA (test 1) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, szObjectTypeName,
                                     szInheritedObjectTypeName, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == (ACE_OBJECT_TYPE_PRESENT | ACE_INHERITED_OBJECT_TYPE_PRESENT), "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == szInheritedObjectTypeName, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");

    /* test GetTrusteeNameA */
    ok(pGetTrusteeNameA(&trustee) == (LPSTR)&oan, "GetTrusteeName returned wrong value\n");

    /* test BuildTrusteeWithObjectsAndNameA (test 2) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, NULL,
                                     szInheritedObjectTypeName, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == ACE_INHERITED_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == szInheritedObjectTypeName, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");

    /* test BuildTrusteeWithObjectsAndNameA (test 3) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, szObjectTypeName,
                                     NULL, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == ACE_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == NULL, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");
}
 
/* If the first isn't defined, assume none is */
#ifndef SE_MIN_WELL_KNOWN_PRIVILEGE
#define SE_MIN_WELL_KNOWN_PRIVILEGE       2L
#define SE_CREATE_TOKEN_PRIVILEGE         2L
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   3L
#define SE_LOCK_MEMORY_PRIVILEGE          4L
#define SE_INCREASE_QUOTA_PRIVILEGE       5L
#define SE_MACHINE_ACCOUNT_PRIVILEGE      6L
#define SE_TCB_PRIVILEGE                  7L
#define SE_SECURITY_PRIVILEGE             8L
#define SE_TAKE_OWNERSHIP_PRIVILEGE       9L
#define SE_LOAD_DRIVER_PRIVILEGE         10L
#define SE_SYSTEM_PROFILE_PRIVILEGE      11L
#define SE_SYSTEMTIME_PRIVILEGE          12L
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 13L
#define SE_INC_BASE_PRIORITY_PRIVILEGE   14L
#define SE_CREATE_PAGEFILE_PRIVILEGE     15L
#define SE_CREATE_PERMANENT_PRIVILEGE    16L
#define SE_BACKUP_PRIVILEGE              17L
#define SE_RESTORE_PRIVILEGE             18L
#define SE_SHUTDOWN_PRIVILEGE            19L
#define SE_DEBUG_PRIVILEGE               20L
#define SE_AUDIT_PRIVILEGE               21L
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE  22L
#define SE_CHANGE_NOTIFY_PRIVILEGE       23L
#define SE_REMOTE_SHUTDOWN_PRIVILEGE     24L
#define SE_UNDOCK_PRIVILEGE              25L
#define SE_SYNC_AGENT_PRIVILEGE          26L
#define SE_ENABLE_DELEGATION_PRIVILEGE   27L
#define SE_MANAGE_VOLUME_PRIVILEGE       28L
#define SE_IMPERSONATE_PRIVILEGE         29L
#define SE_CREATE_GLOBAL_PRIVILEGE       30L
#define SE_MAX_WELL_KNOWN_PRIVILEGE      SE_CREATE_GLOBAL_PRIVILEGE
#endif /* ndef SE_MIN_WELL_KNOWN_PRIVILEGE */

static void test_allocateLuid(void)
{
    BOOL (WINAPI *pAllocateLocallyUniqueId)(PLUID);
    LUID luid1, luid2;
    BOOL ret;

    pAllocateLocallyUniqueId = (void*)GetProcAddress(hmod, "AllocateLocallyUniqueId");
    if (!pAllocateLocallyUniqueId) return;

    ret = pAllocateLocallyUniqueId(&luid1);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    ok(ret,
     "AllocateLocallyUniqueId failed: %ld\n", GetLastError());
    ret = pAllocateLocallyUniqueId(&luid2);
    ok( ret,
     "AllocateLocallyUniqueId failed: %ld\n", GetLastError());
    ok(luid1.LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE || luid1.HighPart != 0,
     "AllocateLocallyUniqueId returned a well-known LUID\n");
    ok(luid1.LowPart != luid2.LowPart || luid1.HighPart != luid2.HighPart,
     "AllocateLocallyUniqueId returned non-unique LUIDs\n");
    ret = pAllocateLocallyUniqueId(NULL);
    ok( !ret && GetLastError() == ERROR_NOACCESS,
     "AllocateLocallyUniqueId(NULL) didn't return ERROR_NOACCESS: %ld\n",
     GetLastError());
}

static void test_lookupPrivilegeName(void)
{
    BOOL (WINAPI *pLookupPrivilegeNameA)(LPCSTR, PLUID, LPSTR, LPDWORD);
    char buf[MAX_PATH]; /* arbitrary, seems long enough */
    DWORD cchName = sizeof(buf);
    LUID luid = { 0, 0 };
    LONG i;
    BOOL ret;

    /* check whether it's available first */
    pLookupPrivilegeNameA = (void*)GetProcAddress(hmod, "LookupPrivilegeNameA");
    if (!pLookupPrivilegeNameA) return;
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    /* check with a short buffer */
    cchName = 0;
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    ret = pLookupPrivilegeNameA(NULL, &luid, NULL, &cchName);
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "LookupPrivilegeNameA didn't fail with ERROR_INSUFFICIENT_BUFFER: %ld\n",
     GetLastError());
    ok(cchName == strlen("SeCreateTokenPrivilege") + 1,
     "LookupPrivilegeNameA returned an incorrect required length for\n"
     "SeCreateTokenPrivilege (got %ld, expected %d)\n", cchName,
     lstrlenA("SeCreateTokenPrivilege") + 1);
    /* check a known value and its returned length on success */
    cchName = sizeof(buf);
    ok(pLookupPrivilegeNameA(NULL, &luid, buf, &cchName) &&
     cchName == strlen("SeCreateTokenPrivilege"),
     "LookupPrivilegeNameA returned an incorrect output length for\n"
     "SeCreateTokenPrivilege (got %ld, expected %d)\n", cchName,
     (int)strlen("SeCreateTokenPrivilege"));
    /* check known values */
    for (i = SE_MIN_WELL_KNOWN_PRIVILEGE; i <= SE_MAX_WELL_KNOWN_PRIVILEGE; i++)
    {
        luid.LowPart = i;
        cchName = sizeof(buf);
        ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
        ok( ret || GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
         "LookupPrivilegeNameA(0.%ld) failed: %ld\n", i, GetLastError());
    }
    /* check a bogus LUID */
    luid.LowPart = 0xdeadbeef;
    cchName = sizeof(buf);
    ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeNameA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %ld\n",
     GetLastError());
    /* check on a bogus system */
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    cchName = sizeof(buf);
    ret = pLookupPrivilegeNameA("b0gu5.Nam3", &luid, buf, &cchName);
    ok( !ret && (GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
                 GetLastError() == RPC_S_INVALID_NET_ADDR) /* w2k8 */,
     "LookupPrivilegeNameA didn't fail with RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR: %ld\n",
     GetLastError());
}

struct NameToLUID
{
    const char *name;
    DWORD lowPart;
};

static void test_lookupPrivilegeValue(void)
{
    static const struct NameToLUID privs[] = {
     { "SeCreateTokenPrivilege", SE_CREATE_TOKEN_PRIVILEGE },
     { "SeAssignPrimaryTokenPrivilege", SE_ASSIGNPRIMARYTOKEN_PRIVILEGE },
     { "SeLockMemoryPrivilege", SE_LOCK_MEMORY_PRIVILEGE },
     { "SeIncreaseQuotaPrivilege", SE_INCREASE_QUOTA_PRIVILEGE },
     { "SeMachineAccountPrivilege", SE_MACHINE_ACCOUNT_PRIVILEGE },
     { "SeTcbPrivilege", SE_TCB_PRIVILEGE },
     { "SeSecurityPrivilege", SE_SECURITY_PRIVILEGE },
     { "SeTakeOwnershipPrivilege", SE_TAKE_OWNERSHIP_PRIVILEGE },
     { "SeLoadDriverPrivilege", SE_LOAD_DRIVER_PRIVILEGE },
     { "SeSystemProfilePrivilege", SE_SYSTEM_PROFILE_PRIVILEGE },
     { "SeSystemtimePrivilege", SE_SYSTEMTIME_PRIVILEGE },
     { "SeProfileSingleProcessPrivilege", SE_PROF_SINGLE_PROCESS_PRIVILEGE },
     { "SeIncreaseBasePriorityPrivilege", SE_INC_BASE_PRIORITY_PRIVILEGE },
     { "SeCreatePagefilePrivilege", SE_CREATE_PAGEFILE_PRIVILEGE },
     { "SeCreatePermanentPrivilege", SE_CREATE_PERMANENT_PRIVILEGE },
     { "SeBackupPrivilege", SE_BACKUP_PRIVILEGE },
     { "SeRestorePrivilege", SE_RESTORE_PRIVILEGE },
     { "SeShutdownPrivilege", SE_SHUTDOWN_PRIVILEGE },
     { "SeDebugPrivilege", SE_DEBUG_PRIVILEGE },
     { "SeAuditPrivilege", SE_AUDIT_PRIVILEGE },
     { "SeSystemEnvironmentPrivilege", SE_SYSTEM_ENVIRONMENT_PRIVILEGE },
     { "SeChangeNotifyPrivilege", SE_CHANGE_NOTIFY_PRIVILEGE },
     { "SeRemoteShutdownPrivilege", SE_REMOTE_SHUTDOWN_PRIVILEGE },
     { "SeUndockPrivilege", SE_UNDOCK_PRIVILEGE },
     { "SeSyncAgentPrivilege", SE_SYNC_AGENT_PRIVILEGE },
     { "SeEnableDelegationPrivilege", SE_ENABLE_DELEGATION_PRIVILEGE },
     { "SeManageVolumePrivilege", SE_MANAGE_VOLUME_PRIVILEGE },
     { "SeImpersonatePrivilege", SE_IMPERSONATE_PRIVILEGE },
     { "SeCreateGlobalPrivilege", SE_CREATE_GLOBAL_PRIVILEGE },
    };
    BOOL (WINAPI *pLookupPrivilegeValueA)(LPCSTR, LPCSTR, PLUID);
    unsigned int i;
    LUID luid;
    BOOL ret;

    /* check whether it's available first */
    pLookupPrivilegeValueA = (void*)GetProcAddress(hmod, "LookupPrivilegeValueA");
    if (!pLookupPrivilegeValueA) return;
    ret = pLookupPrivilegeValueA(NULL, "SeCreateTokenPrivilege", &luid);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    /* check a bogus system name */
    ret = pLookupPrivilegeValueA("b0gu5.Nam3", "SeCreateTokenPrivilege", &luid);
    ok( !ret && (GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
                GetLastError() == RPC_S_INVALID_NET_ADDR) /* w2k8 */,
     "LookupPrivilegeValueA didn't fail with RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR: %ld\n",
     GetLastError());
    /* check a NULL string */
    ret = pLookupPrivilegeValueA(NULL, 0, &luid);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeValueA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %ld\n",
     GetLastError());
    /* check a bogus privilege name */
    ret = pLookupPrivilegeValueA(NULL, "SeBogusPrivilege", &luid);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeValueA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %ld\n",
     GetLastError());
    /* check case insensitive */
    ret = pLookupPrivilegeValueA(NULL, "sEcREATEtOKENpRIVILEGE", &luid);
    ok( ret,
     "LookupPrivilegeValueA(NULL, sEcREATEtOKENpRIVILEGE, &luid) failed: %ld\n",
     GetLastError());
    for (i = 0; i < ARRAY_SIZE(privs); i++)
    {
        /* Not all privileges are implemented on all Windows versions, so
         * don't worry if the call fails
         */
        if (pLookupPrivilegeValueA(NULL, privs[i].name, &luid))
        {
            ok(luid.LowPart == privs[i].lowPart,
             "LookupPrivilegeValueA returned an invalid LUID for %s\n",
             privs[i].name);
        }
    }
}

static void test_FileSecurity(void)
{
    char wintmpdir [MAX_PATH];
    char path [MAX_PATH];
    char file [MAX_PATH];
    HANDLE fh, token;
    DWORD sdSize, retSize, rc, granted, priv_set_len;
    PRIVILEGE_SET priv_set;
    BOOL status;
    BYTE *sd;
    GENERIC_MAPPING mapping = { FILE_READ_DATA, FILE_WRITE_DATA, FILE_EXECUTE, FILE_ALL_ACCESS };
    const SECURITY_INFORMATION request = OWNER_SECURITY_INFORMATION
                                       | GROUP_SECURITY_INFORMATION
                                       | DACL_SECURITY_INFORMATION;

    if (!GetTempPathA (sizeof (wintmpdir), wintmpdir)) {
        win_skip ("GetTempPathA failed\n");
        return;
    }

    /* Create a temporary directory and in it a temporary file */
    strcat (strcpy (path, wintmpdir), "rary");
    SetLastError(0xdeadbeef);
    rc = CreateDirectoryA (path, NULL);
    ok (rc || GetLastError() == ERROR_ALREADY_EXISTS, "CreateDirectoryA "
        "failed for '%s' with %ld\n", path, GetLastError());

    strcat (strcpy (file, path), "\\ess");
    SetLastError(0xdeadbeef);
    fh = CreateFileA (file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok (fh != INVALID_HANDLE_VALUE, "CreateFileA "
        "failed for '%s' with %ld\n", file, GetLastError());
    CloseHandle (fh);

    /* For the temporary file ... */

    /* Get size needed */
    retSize = 0;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA (file, request, NULL, 0, &retSize);
    if (!rc && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
        win_skip("GetFileSecurityA is not implemented\n");
        goto cleanup;
    }
    ok (!rc, "GetFileSecurityA "
        "was expected to fail for '%s'\n", file);
    ok (GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetFileSecurityA "
        "returned %ld; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
    ok (retSize > sizeof (SECURITY_DESCRIPTOR), "GetFileSecurityA returned size %ld\n", retSize);

    sdSize = retSize;
    sd = malloc(sdSize);

    /* Get security descriptor for real */
    retSize = -1;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA (file, request, sd, sdSize, &retSize);
    ok (rc, "GetFileSecurityA "
        "was not expected to fail '%s': %ld\n", file, GetLastError());
    ok (retSize == sdSize,
        "GetFileSecurityA returned size %ld; expected %ld\n", retSize, sdSize);

    /* Use it to set security descriptor */
    SetLastError(0xdeadbeef);
    rc = SetFileSecurityA (file, request, sd);
    ok (rc, "SetFileSecurityA "
        "was not expected to fail '%s': %ld\n", file, GetLastError());

    free(sd);

    /* Repeat for the temporary directory ... */

    /* Get size needed */
    retSize = 0;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA (path, request, NULL, 0, &retSize);
    ok (!rc, "GetFileSecurityA "
        "was expected to fail for '%s'\n", path);
    ok (GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetFileSecurityA "
        "returned %ld; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
    ok (retSize > sizeof (SECURITY_DESCRIPTOR), "GetFileSecurityA returned size %ld\n", retSize);

    sdSize = retSize;
    sd = malloc(sdSize);

    /* Get security descriptor for real */
    retSize = -1;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA (path, request, sd, sdSize, &retSize);
    ok (rc, "GetFileSecurityA "
        "was not expected to fail '%s': %ld\n", path, GetLastError());
    ok (retSize == sdSize,
        "GetFileSecurityA returned size %ld; expected %ld\n", retSize, sdSize);

    /* Use it to set security descriptor */
    SetLastError(0xdeadbeef);
    rc = SetFileSecurityA (path, request, sd);
    ok (rc, "SetFileSecurityA "
        "was not expected to fail '%s': %ld\n", path, GetLastError());
    free(sd);

    /* Old test */
    strcpy (wintmpdir, "\\Should not exist");
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA (wintmpdir, OWNER_SECURITY_INFORMATION, NULL, 0, &sdSize);
    ok (!rc, "GetFileSecurityA should fail for not existing directories/files\n");
    ok (GetLastError() == ERROR_FILE_NOT_FOUND,
        "last error ERROR_FILE_NOT_FOUND expected, got %ld\n", GetLastError());

cleanup:
    /* Remove temporary file and directory */
    DeleteFileA(file);
    RemoveDirectoryA(path);

    /* Test file access permissions for a file with FILE_ATTRIBUTE_ARCHIVE */
    SetLastError(0xdeadbeef);
    rc = GetTempPathA(sizeof(wintmpdir), wintmpdir);
    ok(rc, "GetTempPath error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = GetTempFileNameA(wintmpdir, "tmp", 0, file);
    ok(rc, "GetTempFileName error %ld\n", GetLastError());

    rc = GetFileAttributesA(file);
    rc &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_COMPRESSED);
    ok(rc == FILE_ATTRIBUTE_ARCHIVE, "expected FILE_ATTRIBUTE_ARCHIVE got %#lx\n", rc);

    rc = GetFileSecurityA(file, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
                          NULL, 0, &sdSize);
    ok(!rc, "GetFileSecurity should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER got %ld\n", GetLastError());
    ok(sdSize > sizeof(SECURITY_DESCRIPTOR), "got sd size %ld\n", sdSize);

    sd = malloc(sdSize);
    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA(file, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
                          sd, sdSize, &retSize);
    ok(rc, "GetFileSecurity error %ld\n", GetLastError());
    ok(retSize == sdSize, "expected %ld, got %ld\n", sdSize, retSize);

    SetLastError(0xdeadbeef);
    rc = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token);
    ok(!rc, "OpenThreadToken should fail\n");
    ok(GetLastError() == ERROR_NO_TOKEN, "expected ERROR_NO_TOKEN, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = ImpersonateSelf(SecurityIdentification);
    ok(rc, "ImpersonateSelf error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token);
    ok(rc, "OpenThreadToken error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = RevertToSelf();
    ok(rc, "RevertToSelf error %ld\n", GetLastError());

    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_READ_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_READ_DATA, "expected FILE_READ_DATA, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_WRITE_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_WRITE_DATA, "expected FILE_WRITE_DATA, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_EXECUTE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_EXECUTE, "expected FILE_EXECUTE, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, DELETE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == DELETE, "expected DELETE, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_DELETE_CHILD, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_DELETE_CHILD, "expected FILE_DELETE_CHILD, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, 0x1ff, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == 0x1ff, "expected 0x1ff, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_ALL_ACCESS, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#lx\n", granted);

    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, 0xffffffff, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(!rc, "AccessCheck should fail\n");
    ok(GetLastError() == ERROR_GENERIC_NOT_MAPPED, "expected ERROR_GENERIC_NOT_MAPPED, got %ld\n", GetLastError());

    /* Test file access permissions for a file with FILE_ATTRIBUTE_READONLY */
    SetLastError(0xdeadbeef);
    fh = CreateFileA(file, FILE_READ_DATA, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok(fh != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = WriteFile(fh, "1", 1, &retSize, NULL);
    ok(!rc, "WriteFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    ok(retSize == 0, "expected 0, got %ld\n", retSize);
    CloseHandle(fh);

    rc = GetFileAttributesA(file);
    rc &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_COMPRESSED);
    todo_wine
    ok(rc == (FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY),
       "expected FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY got %#lx\n", rc);

    SetLastError(0xdeadbeef);
    rc = SetFileAttributesA(file, FILE_ATTRIBUTE_ARCHIVE);
    ok(rc, "SetFileAttributes error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    rc = DeleteFileA(file);
    ok(rc, "DeleteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    fh = CreateFileA(file, FILE_READ_DATA, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok(fh != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = WriteFile(fh, "1", 1, &retSize, NULL);
    ok(!rc, "WriteFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    ok(retSize == 0, "expected 0, got %ld\n", retSize);
    CloseHandle(fh);

    rc = GetFileAttributesA(file);
    rc &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_COMPRESSED);
    ok(rc == (FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY),
       "expected FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY got %#lx\n", rc);

    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA(file, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
                          sd, sdSize, &retSize);
    ok(rc, "GetFileSecurity error %ld\n", GetLastError());
    ok(retSize == sdSize, "expected %ld, got %ld\n", sdSize, retSize);

    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_READ_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_READ_DATA, "expected FILE_READ_DATA, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_WRITE_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_WRITE_DATA, "expected FILE_WRITE_DATA, got %#lx\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_EXECUTE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_EXECUTE, "expected FILE_EXECUTE, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, DELETE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == DELETE, "expected DELETE, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, WRITE_OWNER, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == WRITE_OWNER, "expected WRITE_OWNER, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, SYNCHRONIZE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == SYNCHRONIZE, "expected SYNCHRONIZE, got %#lx\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_DELETE_CHILD, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_DELETE_CHILD, "expected FILE_DELETE_CHILD, got %#lx\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, 0x1ff, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == 0x1ff, "expected 0x1ff, got %#lx\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_ALL_ACCESS, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %ld\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#lx\n", granted);
}
    SetLastError(0xdeadbeef);
    rc = DeleteFileA(file);
    ok(!rc, "DeleteFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    rc = SetFileAttributesA(file, FILE_ATTRIBUTE_ARCHIVE);
    ok(rc, "SetFileAttributes error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    rc = DeleteFileA(file);
    ok(rc, "DeleteFile error %ld\n", GetLastError());

    CloseHandle(token);
    free(sd);
}

static void test_AccessCheck(void)
{
    PSID EveryoneSid = NULL, AdminSid = NULL, UsersSid = NULL;
    PACL Acl = NULL;
    SECURITY_DESCRIPTOR *SecurityDescriptor = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    GENERIC_MAPPING Mapping = { KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS };
    ACCESS_MASK Access;
    BOOL AccessStatus;
    HANDLE Token;
    HANDLE ProcessToken;
    BOOL ret;
    DWORD PrivSetLen;
    PRIVILEGE_SET *PrivSet;
    BOOL res;
    HMODULE NtDllModule;
    BOOLEAN Enabled;
    DWORD err;
    NTSTATUS ntret, ntAccessStatus;

    NtDllModule = GetModuleHandleA("ntdll.dll");
    if (!NtDllModule)
    {
        skip("not running on NT, skipping test\n");
        return;
    }
    pRtlAdjustPrivilege = (void *)GetProcAddress(NtDllModule, "RtlAdjustPrivilege");
    if (!pRtlAdjustPrivilege)
    {
        win_skip("missing RtlAdjustPrivilege, skipping test\n");
        return;
    }

    Acl = malloc(256);
    res = InitializeAcl(Acl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("ACLs not implemented - skipping tests\n");
        free(Acl);
        return;
    }
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdminSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &UsersSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    SecurityDescriptor = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    res = InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ok(res, "InitializeSecurityDescriptor failed with error %ld\n", GetLastError());

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());

    PrivSetLen = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
    PrivSet = calloc(1, PrivSetLen);
    PrivSet->PrivilegeCount = 16;

    res = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &ProcessToken);
    ok(res, "OpenProcessToken failed with error %ld\n", GetLastError());

    pRtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, FALSE, TRUE, &Enabled);

    res = DuplicateToken(ProcessToken, SecurityImpersonation, &Token);
    ok(res, "DuplicateToken failed with error %ld\n", GetLastError());

    /* SD without owner/group */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_QUERY_VALUE, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_SECURITY_DESCR, "AccessCheck should have "
       "failed with ERROR_INVALID_SECURITY_DESCR, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Set owner and group */
    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %ld\n", GetLastError());
    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, TRUE);
    ok(res, "SetSecurityDescriptorGroup failed with error %ld\n", GetLastError());

    /* Generic access mask */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_GENERIC_NOT_MAPPED, "AccessCheck should have failed "
       "with ERROR_GENERIC_NOT_MAPPED, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Generic access mask - no privilegeset buffer */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      NULL, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Generic access mask - no returnlength */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      PrivSet, NULL, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Generic access mask - no privilegeset buffer, no returnlength */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      NULL, NULL, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* sd with no dacl present */
    Access = AccessStatus = 0x1abe11ed;
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, FALSE, NULL, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %ld\n",
        GetLastError());

    /* sd with no dacl present - no privilegeset buffer */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      NULL, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    if(pNtAccessCheck)
    {
       DWORD ntPrivSetLen = sizeof(PRIVILEGE_SET);

       /* Generic access mask - no privilegeset buffer */
       SetLastError(0xdeadbeef);
       Access = ntAccessStatus = 0x1abe11ed;
       ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                              NULL, &ntPrivSetLen, &Access, &ntAccessStatus);
       err = GetLastError();
       ok(ntret == STATUS_ACCESS_VIOLATION,
          "NtAccessCheck should have failed with STATUS_ACCESS_VIOLATION, got %lx\n", ntret);
       ok(err == 0xdeadbeef,
          "NtAccessCheck shouldn't set last error, got %ld\n", err);
       ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
          "Access and/or AccessStatus were changed!\n");
       ok(ntPrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", ntPrivSetLen);

      /* Generic access mask - no returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                             PrivSet, NULL, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_ACCESS_VIOLATION,
         "NtAccessCheck should have failed with STATUS_ACCESS_VIOLATION, got %lx\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %ld\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");

      /* Generic access mask - no privilegeset buffer, no returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                             NULL, NULL, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_ACCESS_VIOLATION,
         "NtAccessCheck should have failed with STATUS_ACCESS_VIOLATION, got %lx\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %ld\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");

      /* Generic access mask - zero returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntPrivSetLen = 0;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                             PrivSet, &ntPrivSetLen, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_GENERIC_NOT_MAPPED,
         "NtAccessCheck should have failed with STATUS_GENERIC_NOT_MAPPED, got %lx\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %ld\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");
      ok(ntPrivSetLen == 0, "PrivSetLen returns %ld\n", ntPrivSetLen);

      /* Generic access mask - insufficient returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntPrivSetLen = sizeof(PRIVILEGE_SET)-1;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                             PrivSet, &ntPrivSetLen, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_GENERIC_NOT_MAPPED,
         "NtAccessCheck should have failed with STATUS_GENERIC_NOT_MAPPED, got %lx\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %ld\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");
      ok(ntPrivSetLen == sizeof(PRIVILEGE_SET)-1, "PrivSetLen returns %ld\n", ntPrivSetLen);

      /* Key access mask - zero returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntPrivSetLen = 0;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                             PrivSet, &ntPrivSetLen, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_BUFFER_TOO_SMALL,
         "NtAccessCheck should have failed with STATUS_BUFFER_TOO_SMALL, got %lx\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %ld\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");
      ok(ntPrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", ntPrivSetLen);

      /* Key access mask - insufficient returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntPrivSetLen = sizeof(PRIVILEGE_SET)-1;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                             PrivSet, &ntPrivSetLen, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_BUFFER_TOO_SMALL,
         "NtAccessCheck should have failed with STATUS_BUFFER_TOO_SMALL, got %lx\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %ld\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");
      ok(ntPrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", ntPrivSetLen);
    }
    else
       win_skip("NtAccessCheck unavailable. Skipping.\n");

    /* sd with NULL dacl */
    Access = AccessStatus = 0x1abe11ed;
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, NULL, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %ld\n",
        GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, MAXIMUM_ALLOWED, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus && (Access == KEY_ALL_ACCESS),
        "AccessCheck failed to grant access with error %ld\n",
        GetLastError());

    /* sd with blank dacl */
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    err = GetLastError();
    ok(!AccessStatus && err == ERROR_ACCESS_DENIED, "AccessCheck should have failed "
       "with ERROR_ACCESS_DENIED, instead of %ld\n", err);
    ok(!Access, "Should have failed to grant any access, got 0x%08lx\n", Access);

    res = AddAccessAllowedAce(Acl, ACL_REVISION, KEY_READ, EveryoneSid);
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());

    res = AddAccessDeniedAce(Acl, ACL_REVISION, KEY_SET_VALUE, AdminSid);
    ok(res, "AddAccessDeniedAce failed with error %ld\n", GetLastError());

    /* sd with dacl */
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %ld\n",
        GetLastError());

    ret = AccessCheck(SecurityDescriptor, Token, MAXIMUM_ALLOWED, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(AccessStatus,
        "AccessCheck failed to grant any access with error %ld\n",
        GetLastError());
    trace("AccessCheck with MAXIMUM_ALLOWED got Access 0x%08lx\n", Access);

    /* Null PrivSet with null PrivSetLen pointer */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      NULL, NULL, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have "
       "failed with ERROR_NOACCESS, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Null PrivSet with zero PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = 0;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      0, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    todo_wine
    ok(!ret && err == ERROR_INSUFFICIENT_BUFFER, "AccessCheck should have "
       "failed with ERROR_INSUFFICIENT_BUFFER, instead of %ld\n", err);
    todo_wine
    ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Null PrivSet with insufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = 1;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      0, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have "
       "failed with ERROR_NOACCESS, instead of %ld\n", err);
    ok(PrivSetLen == 1, "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Null PrivSet with insufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = sizeof(PRIVILEGE_SET) - 1;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      0, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have "
       "failed with ERROR_NOACCESS, instead of %ld\n", err);
    ok(PrivSetLen == sizeof(PRIVILEGE_SET) - 1, "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Null PrivSet with minimal sufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = sizeof(PRIVILEGE_SET);
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      0, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have "
       "failed with ERROR_NOACCESS, instead of %ld\n", err);
    ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Valid PrivSet with zero PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = 0;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_INSUFFICIENT_BUFFER, "AccessCheck should have "
       "failed with ERROR_INSUFFICIENT_BUFFER, instead of %ld\n", err);
    ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Valid PrivSet with insufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = 1;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_INSUFFICIENT_BUFFER, "AccessCheck should have "
       "failed with ERROR_INSUFFICIENT_BUFFER, instead of %ld\n", err);
    ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Valid PrivSet with insufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = sizeof(PRIVILEGE_SET) - 1;
    PrivSet->PrivilegeCount = 0xdeadbeef;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_INSUFFICIENT_BUFFER, "AccessCheck should have "
       "failed with ERROR_INSUFFICIENT_BUFFER, instead of %ld\n", err);
    ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");
    ok(PrivSet->PrivilegeCount == 0xdeadbeef, "buffer contents should not be changed\n");

    /* Valid PrivSet with minimal sufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = sizeof(PRIVILEGE_SET);
    memset(PrivSet, 0xcc, PrivSetLen);
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %ld\n", GetLastError());
    ok(PrivSet->PrivilegeCount == 0, "PrivilegeCount returns %ld, expects 0\n",
        PrivSet->PrivilegeCount);

    /* Valid PrivSet with sufficient PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    PrivSetLen = sizeof(PRIVILEGE_SET) + 1;
    memset(PrivSet, 0xcc, PrivSetLen);
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    todo_wine
    ok(PrivSetLen == sizeof(PRIVILEGE_SET) + 1, "PrivSetLen returns %ld\n", PrivSetLen);
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %ld\n", GetLastError());
    ok(PrivSet->PrivilegeCount == 0, "PrivilegeCount returns %ld, expects 0\n",
        PrivSet->PrivilegeCount);

    PrivSetLen = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);

    /* Null PrivSet with valid PrivSetLen */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      0, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have "
       "failed with ERROR_NOACCESS, instead of %ld\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Access denied by SD */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_WRITE, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    err = GetLastError();
    ok(!AccessStatus && err == ERROR_ACCESS_DENIED, "AccessCheck should have failed "
       "with ERROR_ACCESS_DENIED, instead of %ld\n", err);
    ok(!Access, "Should have failed to grant any access, got 0x%08lx\n", Access);

    SetLastError(0xdeadbeef);
    PrivSet->PrivilegeCount = 16;
    ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret && !AccessStatus && GetLastError() == ERROR_PRIVILEGE_NOT_HELD,
        "AccessCheck should have failed with ERROR_PRIVILEGE_NOT_HELD, instead of %ld\n",
        GetLastError());

    ret = ImpersonateLoggedOnUser(Token);
    ok(ret, "ImpersonateLoggedOnUser failed with error %ld\n", GetLastError());
    ret = pRtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, TRUE, TRUE, &Enabled);
    if (!ret)
    {
        /* Valid PrivSet with zero PrivSetLen */
        SetLastError(0xdeadbeef);
        Access = AccessStatus = 0x1abe11ed;
        PrivSetLen = 0;
        ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                          PrivSet, &PrivSetLen, &Access, &AccessStatus);
        err = GetLastError();
        ok(!ret && err == ERROR_INSUFFICIENT_BUFFER, "AccessCheck should have "
           "failed with ERROR_INSUFFICIENT_BUFFER, instead of %ld\n", err);
        ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
        ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
           "Access and/or AccessStatus were changed!\n");

        /* Valid PrivSet with insufficient PrivSetLen */
        SetLastError(0xdeadbeef);
        Access = AccessStatus = 0x1abe11ed;
        PrivSetLen = sizeof(PRIVILEGE_SET) - 1;
        ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                          PrivSet, &PrivSetLen, &Access, &AccessStatus);
        err = GetLastError();
        ok(!ret && err == ERROR_INSUFFICIENT_BUFFER, "AccessCheck should have "
           "failed with ERROR_INSUFFICIENT_BUFFER, instead of %ld\n", err);
        ok(PrivSetLen == sizeof(PRIVILEGE_SET), "PrivSetLen returns %ld\n", PrivSetLen);
        ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
           "Access and/or AccessStatus were changed!\n");

        /* Valid PrivSet with minimal sufficient PrivSetLen */
        SetLastError(0xdeadbeef);
        Access = AccessStatus = 0x1abe11ed;
        PrivSetLen = sizeof(PRIVILEGE_SET);
        memset(PrivSet, 0xcc, PrivSetLen);
        ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                          PrivSet, &PrivSetLen, &Access, &AccessStatus);
        ok(ret && AccessStatus && GetLastError() == 0xdeadbeef,
            "AccessCheck should have succeeded, error %ld\n",
            GetLastError());
        ok(Access == ACCESS_SYSTEM_SECURITY,
            "Access should be equal to ACCESS_SYSTEM_SECURITY instead of 0x%08lx\n",
            Access);
        ok(PrivSet->PrivilegeCount == 1, "PrivilegeCount returns %ld, expects 1\n",
            PrivSet->PrivilegeCount);

        /* Valid PrivSet with large PrivSetLen */
        SetLastError(0xdeadbeef);
        Access = AccessStatus = 0x1abe11ed;
        PrivSetLen = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
        memset(PrivSet, 0xcc, PrivSetLen);
        ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                          PrivSet, &PrivSetLen, &Access, &AccessStatus);
        ok(ret && AccessStatus && GetLastError() == 0xdeadbeef,
            "AccessCheck should have succeeded, error %ld\n",
            GetLastError());
        ok(Access == ACCESS_SYSTEM_SECURITY,
            "Access should be equal to ACCESS_SYSTEM_SECURITY instead of 0x%08lx\n",
            Access);
        ok(PrivSet->PrivilegeCount == 1, "PrivilegeCount returns %ld, expects 1\n",
            PrivSet->PrivilegeCount);
    }
    else
        trace("Couldn't get SE_SECURITY_PRIVILEGE (0x%08x), skipping ACCESS_SYSTEM_SECURITY test\n",
            ret);
    ret = RevertToSelf();
    ok(ret, "RevertToSelf failed with error %ld\n", GetLastError());

    /* test INHERIT_ONLY_ACE */
    ret = InitializeAcl(Acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with error %ld\n", GetLastError());

    ret = AddAccessAllowedAceEx(Acl, ACL_REVISION, INHERIT_ONLY_ACE, KEY_READ, EveryoneSid);
    ok(ret, "AddAccessAllowedAceEx failed with error %ld\n", GetLastError());

    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %ld\n", GetLastError());
    err = GetLastError();
    ok(!AccessStatus && err == ERROR_ACCESS_DENIED, "AccessCheck should have failed "
       "with ERROR_ACCESS_DENIED, instead of %ld\n", err);
    ok(!Access, "Should have failed to grant any access, got 0x%08lx\n", Access);

    CloseHandle(Token);

    res = DuplicateToken(ProcessToken, SecurityAnonymous, &Token);
    ok(res, "DuplicateToken failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = AccessCheck(SecurityDescriptor, Token, MAXIMUM_ALLOWED, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_BAD_IMPERSONATION_LEVEL, "AccessCheck should have failed "
       "with ERROR_BAD_IMPERSONATION_LEVEL, instead of %ld\n", err);

    CloseHandle(Token);

    SetLastError(0xdeadbeef);
    ret = AccessCheck(SecurityDescriptor, ProcessToken, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NO_IMPERSONATION_TOKEN, "AccessCheck should have failed "
       "with ERROR_NO_IMPERSONATION_TOKEN, instead of %ld\n", err);

    CloseHandle(ProcessToken);

    if (EveryoneSid)
        FreeSid(EveryoneSid);
    if (AdminSid)
        FreeSid(AdminSid);
    if (UsersSid)
        FreeSid(UsersSid);
    free(Acl);
    free(SecurityDescriptor);
    free(PrivSet);
}

static TOKEN_USER *get_alloc_token_user( HANDLE token )
{
    TOKEN_USER *token_user;
    DWORD size;
    BOOL ret;

    ret = GetTokenInformation( token, TokenUser, NULL, 0, &size );
    ok(!ret, "Expected failure, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    token_user = malloc( size );
    ret = GetTokenInformation( token, TokenUser, token_user, size, &size );
    ok(ret, "GetTokenInformation failed with error %ld\n", GetLastError());

    return token_user;
}

static TOKEN_OWNER *get_alloc_token_owner( HANDLE token )
{
    TOKEN_OWNER *token_owner;
    DWORD size;
    BOOL ret;

    ret = GetTokenInformation( token, TokenOwner, NULL, 0, &size );
    ok(!ret, "Expected failure, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    token_owner = malloc( size );
    ret = GetTokenInformation( token, TokenOwner, token_owner, size, &size );
    ok(ret, "GetTokenInformation failed with error %ld\n", GetLastError());

    return token_owner;
}

static TOKEN_PRIMARY_GROUP *get_alloc_token_primary_group( HANDLE token )
{
    TOKEN_PRIMARY_GROUP *token_primary_group;
    DWORD size;
    BOOL ret;

    ret = GetTokenInformation( token, TokenPrimaryGroup, NULL, 0, &size );
    ok(!ret, "Expected failure, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    token_primary_group = malloc( size );
    ret = GetTokenInformation( token, TokenPrimaryGroup, token_primary_group, size, &size );
    ok(ret, "GetTokenInformation failed with error %ld\n", GetLastError());

    return token_primary_group;
}

/* test GetTokenInformation for the various attributes */
static void test_token_attr(void)
{
    HANDLE Token, ImpersonationToken;
    DWORD Size, Size2;
    TOKEN_PRIVILEGES *Privileges;
    TOKEN_GROUPS *Groups;
    TOKEN_USER *User;
    TOKEN_OWNER *Owner;
    TOKEN_DEFAULT_DACL *Dacl;
    BOOL ret;
    DWORD i, GLE;
    LPSTR SidString;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    ACL *acl;

    /* cygwin-like use case */
    SetLastError(0xdeadbeef);
    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("OpenProcessToken is not implemented\n");
        return;
    }
    ok(ret, "OpenProcessToken failed with error %ld\n", GetLastError());
    if (ret)
    {
        DWORD buf[256]; /* GetTokenInformation wants a dword-aligned buffer */
        Size = sizeof(buf);
        ret = GetTokenInformation(Token, TokenUser,(void*)buf, Size, &Size);
        ok(ret, "GetTokenInformation failed with error %ld\n", GetLastError());
        Size = sizeof(ImpersonationLevel);
        ret = GetTokenInformation(Token, TokenImpersonationLevel, &ImpersonationLevel, Size, &Size);
        GLE = GetLastError();
        ok(!ret && (GLE == ERROR_INVALID_PARAMETER), "GetTokenInformation(TokenImpersonationLevel) on primary token should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", GLE);
        CloseHandle(Token);
    }

    SetLastError(0xdeadbeef);
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &Token);
    ok(ret, "OpenProcessToken failed with error %ld\n", GetLastError());

    /* groups */
    /* insufficient buffer length */
    SetLastError(0xdeadbeef);
    Size2 = 0;
    ret = GetTokenInformation(Token, TokenGroups, NULL, 0, &Size2);
    ok(Size2 > 1, "got %ld\n", Size2);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "%d with error %ld\n", ret, GetLastError());
    Size2 -= 1;
    Groups = malloc(Size2);
    memset(Groups, 0xcc, Size2);
    Size = 0;
    ret = GetTokenInformation(Token, TokenGroups, Groups, Size2, &Size);
    ok(Size > 1, "got %ld\n", Size);
    ok((!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER) || broken(ret) /* wow64 */,
        "%d with error %ld\n", ret, GetLastError());
    if(!ret)
        ok(*((BYTE*)Groups) == 0xcc, "buffer altered\n");

    free(Groups);

    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(Token, TokenGroups, NULL, 0, &Size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "GetTokenInformation(TokenGroups) %s with error %ld\n",
        ret ? "succeeded" : "failed", GetLastError());
    Groups = malloc(Size);
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(Token, TokenGroups, Groups, Size, &Size);
    ok(ret, "GetTokenInformation(TokenGroups) failed with error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef,
       "GetTokenInformation shouldn't have set last error to %ld\n",
       GetLastError());
    trace("TokenGroups:\n");
    for (i = 0; i < Groups->GroupCount; i++)
    {
        DWORD NameLength = 255;
        CHAR Name[255];
        DWORD DomainLength = 255;
        CHAR Domain[255];
        SID_NAME_USE SidNameUse;
        Name[0] = '\0';
        Domain[0] = '\0';
        ret = LookupAccountSidA(NULL, Groups->Groups[i].Sid, Name, &NameLength, Domain, &DomainLength, &SidNameUse);
        if (ret)
        {
            ConvertSidToStringSidA(Groups->Groups[i].Sid, &SidString);
            trace("%s, %s\\%s use: %d attr: 0x%08lx\n", SidString, Domain, Name, SidNameUse, Groups->Groups[i].Attributes);
            LocalFree(SidString);
        }
        else trace("attr: 0x%08lx LookupAccountSid failed with error %ld\n", Groups->Groups[i].Attributes, GetLastError());
    }
    free(Groups);

    /* user */
    ret = GetTokenInformation(Token, TokenUser, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    User = malloc(Size);
    ret = GetTokenInformation(Token, TokenUser, User, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());

    ConvertSidToStringSidA(User->User.Sid, &SidString);
    trace("TokenUser: %s attr: 0x%08lx\n", SidString, User->User.Attributes);
    LocalFree(SidString);
    free(User);

    /* owner */
    ret = GetTokenInformation(Token, TokenOwner, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenOwner) failed with error %ld\n", GetLastError());
    Owner = malloc(Size);
    ret = GetTokenInformation(Token, TokenOwner, Owner, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenOwner) failed with error %ld\n", GetLastError());

    ConvertSidToStringSidA(Owner->Owner, &SidString);
    trace("TokenOwner: %s\n", SidString);
    LocalFree(SidString);
    free(Owner);

    /* logon */
    ret = GetTokenInformation(Token, TokenLogonSid, NULL, 0, &Size);
    if (!ret && (GetLastError() == ERROR_INVALID_PARAMETER))
        todo_wine win_skip("TokenLogonSid not supported. Skipping tests\n");
    else
    {
        ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "GetTokenInformation(TokenLogonSid) failed with error %ld\n", GetLastError());
        Groups = malloc(Size);
        ret = GetTokenInformation(Token, TokenLogonSid, Groups, Size, &Size);
        ok(ret,
            "GetTokenInformation(TokenLogonSid) failed with error %ld\n", GetLastError());
        if (ret)
        {
            ok(Groups->GroupCount == 1, "got %ld\n", Groups->GroupCount);
            if(Groups->GroupCount == 1)
            {
                ConvertSidToStringSidA(Groups->Groups[0].Sid, &SidString);
                trace("TokenLogon: %s\n", SidString);
                LocalFree(SidString);

                /* S-1-5-5-0-XXXXXX */
                ret = IsWellKnownSid(Groups->Groups[0].Sid, WinLogonIdsSid);
                ok(ret, "Unknown SID\n");

                ok(Groups->Groups[0].Attributes == (SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED | SE_GROUP_LOGON_ID),
                    "got %lx\n", Groups->Groups[0].Attributes);
            }
        }

        free(Groups);
    }

    /* privileges */
    ret = GetTokenInformation(Token, TokenPrivileges, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenPrivileges) failed with error %ld\n", GetLastError());
    Privileges = malloc(Size);
    ret = GetTokenInformation(Token, TokenPrivileges, Privileges, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenPrivileges) failed with error %ld\n", GetLastError());
    trace("TokenPrivileges:\n");
    for (i = 0; i < Privileges->PrivilegeCount; i++)
    {
        CHAR Name[256];
        DWORD NameLen = ARRAY_SIZE(Name);
        LookupPrivilegeNameA(NULL, &Privileges->Privileges[i].Luid, Name, &NameLen);
        trace("\t%s, 0x%lx\n", Name, Privileges->Privileges[i].Attributes);
    }
    free(Privileges);

    ret = DuplicateToken(Token, SecurityAnonymous, &ImpersonationToken);
    ok(ret, "DuplicateToken failed with error %ld\n", GetLastError());

    Size = sizeof(ImpersonationLevel);
    ret = GetTokenInformation(ImpersonationToken, TokenImpersonationLevel, &ImpersonationLevel, Size, &Size);
    ok(ret, "GetTokenInformation(TokenImpersonationLevel) failed with error %ld\n", GetLastError());
    ok(ImpersonationLevel == SecurityAnonymous, "ImpersonationLevel should have been SecurityAnonymous instead of %d\n", ImpersonationLevel);

    CloseHandle(ImpersonationToken);

    /* default dacl */
    ret = GetTokenInformation(Token, TokenDefaultDacl, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenDefaultDacl) failed with error %lu\n", GetLastError());

    Dacl = malloc(Size);
    ret = GetTokenInformation(Token, TokenDefaultDacl, Dacl, Size, &Size);
    ok(ret, "GetTokenInformation(TokenDefaultDacl) failed with error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetTokenInformation(Token, TokenDefaultDacl, NULL, 0);
    GLE = GetLastError();
    ok(!ret, "SetTokenInformation(TokenDefaultDacl) succeeded\n");
    ok(GLE == ERROR_BAD_LENGTH, "expected ERROR_BAD_LENGTH got %lu\n", GLE);

    SetLastError(0xdeadbeef);
    ret = SetTokenInformation(Token, TokenDefaultDacl, NULL, Size);
    GLE = GetLastError();
    ok(!ret, "SetTokenInformation(TokenDefaultDacl) succeeded\n");
    ok(GLE == ERROR_NOACCESS, "expected ERROR_NOACCESS got %lu\n", GLE);

    acl = Dacl->DefaultDacl;
    Dacl->DefaultDacl = NULL;

    ret = SetTokenInformation(Token, TokenDefaultDacl, Dacl, Size);
    ok(ret, "SetTokenInformation(TokenDefaultDacl) succeeded\n");

    Size2 = 0;
    Dacl->DefaultDacl = (ACL *)0xdeadbeef;
    ret = GetTokenInformation(Token, TokenDefaultDacl, Dacl, Size, &Size2);
    ok(ret, "GetTokenInformation(TokenDefaultDacl) failed with error %lu\n", GetLastError());
    ok(Dacl->DefaultDacl == NULL, "expected NULL, got %p\n", Dacl->DefaultDacl);
    ok(Size2 == sizeof(TOKEN_DEFAULT_DACL) || broken(Size2 == 2*sizeof(TOKEN_DEFAULT_DACL)), /* WoW64 */
       "got %lu expected sizeof(TOKEN_DEFAULT_DACL)\n", Size2);

    Dacl->DefaultDacl = acl;
    ret = SetTokenInformation(Token, TokenDefaultDacl, Dacl, Size);
    ok(ret, "SetTokenInformation(TokenDefaultDacl) failed with error %lu\n", GetLastError());

    if (Size2 == sizeof(TOKEN_DEFAULT_DACL)) {
        ret = GetTokenInformation(Token, TokenDefaultDacl, Dacl, Size, &Size2);
        ok(ret, "GetTokenInformation(TokenDefaultDacl) failed with error %lu\n", GetLastError());
    } else
        win_skip("TOKEN_DEFAULT_DACL size too small on WoW64\n");

    free(Dacl);
    CloseHandle(Token);
}

static void test_GetTokenInformation(void)
{
    DWORD is_app_container, size;
    HANDLE token;
    BOOL ret;

    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &token);
    ok(ret, "OpenProcessToken failed: %lu\n", GetLastError());

    size = 0;
    is_app_container = 0xdeadbeef;
    ret = GetTokenInformation(token, TokenIsAppContainer, &is_app_container,
                              sizeof(is_app_container), &size);
    ok(ret || broken(GetLastError() == ERROR_INVALID_PARAMETER ||
                     GetLastError() == ERROR_INVALID_FUNCTION), /* pre-win8 */
       "GetTokenInformation failed: %lu\n", GetLastError());
    if(ret) {
        ok(size == sizeof(is_app_container), "size = %lu\n", size);
        ok(!is_app_container, "is_app_container = %lx\n", is_app_container);
    }

    CloseHandle(token);
}

typedef union _MAX_SID
{
    SID sid;
    char max[SECURITY_MAX_SID_SIZE];
} MAX_SID;

static void test_sid_str(PSID * sid)
{
    char *str_sid;
    BOOL ret = ConvertSidToStringSidA(sid, &str_sid);
    ok(ret, "ConvertSidToStringSidA() failed: %ld\n", GetLastError());
    if (ret)
    {
        char account[MAX_PATH], domain[MAX_PATH];
        SID_NAME_USE use;
        DWORD acc_size = MAX_PATH;
        DWORD dom_size = MAX_PATH;
        ret = LookupAccountSidA (NULL, sid, account, &acc_size, domain, &dom_size, &use);
        ok(ret || GetLastError() == ERROR_NONE_MAPPED,
           "LookupAccountSid(%s) failed: %ld\n", str_sid, GetLastError());
        if (ret)
            trace(" %s %s\\%s %d\n", str_sid, domain, account, use);
        else if (GetLastError() == ERROR_NONE_MAPPED)
            trace(" %s couldn't be mapped\n", str_sid);
        LocalFree(str_sid);
    }
}

static const struct well_known_sid_value
{
    BOOL without_domain;
    const char *sid_string;
} well_known_sid_values[] = {
/*  0 */ {TRUE, "S-1-0-0"},  {TRUE, "S-1-1-0"},  {TRUE, "S-1-2-0"},  {TRUE, "S-1-3-0"},
/*  4 */ {TRUE, "S-1-3-1"},  {TRUE, "S-1-3-2"},  {TRUE, "S-1-3-3"},  {TRUE, "S-1-5"},
/*  8 */ {FALSE, "S-1-5-1"}, {TRUE, "S-1-5-2"},  {TRUE, "S-1-5-3"},  {TRUE, "S-1-5-4"},
/* 12 */ {TRUE, "S-1-5-6"},  {TRUE, "S-1-5-7"},  {TRUE, "S-1-5-8"},  {TRUE, "S-1-5-9"},
/* 16 */ {TRUE, "S-1-5-10"}, {TRUE, "S-1-5-11"}, {TRUE, "S-1-5-12"}, {TRUE, "S-1-5-13"},
/* 20 */ {TRUE, "S-1-5-14"}, {FALSE, NULL},      {TRUE, "S-1-5-18"}, {TRUE, "S-1-5-19"},
/* 24 */ {TRUE, "S-1-5-20"}, {TRUE, "S-1-5-32"},
/* 26 */ {FALSE, "S-1-5-32-544"}, {TRUE, "S-1-5-32-545"}, {TRUE, "S-1-5-32-546"},
/* 29 */ {TRUE, "S-1-5-32-547"},  {TRUE, "S-1-5-32-548"}, {TRUE, "S-1-5-32-549"},
/* 32 */ {TRUE, "S-1-5-32-550"},  {TRUE, "S-1-5-32-551"}, {TRUE, "S-1-5-32-552"},
/* 35 */ {TRUE, "S-1-5-32-554"},  {TRUE, "S-1-5-32-555"}, {TRUE, "S-1-5-32-556"},
/* 38 */ {FALSE, "S-1-5-21-12-23-34-45-56-500"}, {FALSE, "S-1-5-21-12-23-34-45-56-501"},
/* 40 */ {FALSE, "S-1-5-21-12-23-34-45-56-502"}, {FALSE, "S-1-5-21-12-23-34-45-56-512"},
/* 42 */ {FALSE, "S-1-5-21-12-23-34-45-56-513"}, {FALSE, "S-1-5-21-12-23-34-45-56-514"},
/* 44 */ {FALSE, "S-1-5-21-12-23-34-45-56-515"}, {FALSE, "S-1-5-21-12-23-34-45-56-516"},
/* 46 */ {FALSE, "S-1-5-21-12-23-34-45-56-517"}, {FALSE, "S-1-5-21-12-23-34-45-56-518"},
/* 48 */ {FALSE, "S-1-5-21-12-23-34-45-56-519"}, {FALSE, "S-1-5-21-12-23-34-45-56-520"},
/* 50 */ {FALSE, "S-1-5-21-12-23-34-45-56-553"},
/* Added in Windows Server 2003 */
/* 51 */ {TRUE, "S-1-5-64-10"},   {TRUE, "S-1-5-64-21"},   {TRUE, "S-1-5-64-14"},
/* 54 */ {TRUE, "S-1-5-15"},      {TRUE, "S-1-5-1000"},    {FALSE, "S-1-5-32-557"},
/* 57 */ {TRUE, "S-1-5-32-558"},  {TRUE, "S-1-5-32-559"},  {TRUE, "S-1-5-32-560"},
/* 60 */ {TRUE, "S-1-5-32-561"}, {TRUE, "S-1-5-32-562"},
/* Added in Windows Vista: */
/* 62 */ {TRUE, "S-1-5-32-568"},
/* 63 */ {TRUE, "S-1-5-17"},      {FALSE, "S-1-5-32-569"}, {TRUE, "S-1-16-0"},
/* 66 */ {TRUE, "S-1-16-4096"},   {TRUE, "S-1-16-8192"},   {TRUE, "S-1-16-12288"},
/* 69 */ {TRUE, "S-1-16-16384"},  {TRUE, "S-1-5-33"},      {TRUE, "S-1-3-4"},
/* 72 */ {FALSE, "S-1-5-21-12-23-34-45-56-571"},  {FALSE, "S-1-5-21-12-23-34-45-56-572"},
/* 74 */ {TRUE, "S-1-5-22"}, {FALSE, "S-1-5-21-12-23-34-45-56-521"}, {TRUE, "S-1-5-32-573"},
/* 77 */ {FALSE, "S-1-5-21-12-23-34-45-56-498"}, {TRUE, "S-1-5-32-574"}, {TRUE, "S-1-16-8448"},
/* 80 */ {FALSE, NULL}, {TRUE, "S-1-2-1"}, {TRUE, "S-1-5-65-1"}, {FALSE, NULL},
/* 84 */ {TRUE, "S-1-15-2-1"},
};

static void test_CreateWellKnownSid(void)
{
    SID_IDENTIFIER_AUTHORITY ident = { SECURITY_NT_AUTHORITY };
    PSID domainsid, sid;
    DWORD size, error;
    BOOL ret;
    unsigned int i;

    size = 0;
    SetLastError(0xdeadbeef);
    ret = CreateWellKnownSid(WinInteractiveSid, NULL, NULL, &size);
    error = GetLastError();
    ok(!ret, "CreateWellKnownSid succeeded\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %lu\n", error);
    ok(size, "expected size > 0\n");

    SetLastError(0xdeadbeef);
    ret = CreateWellKnownSid(WinInteractiveSid, NULL, NULL, &size);
    error = GetLastError();
    ok(!ret, "CreateWellKnownSid succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error);

    sid = malloc(size);
    ret = CreateWellKnownSid(WinInteractiveSid, NULL, sid, &size);
    ok(ret, "CreateWellKnownSid failed %lu\n", GetLastError());
    free(sid);

    /* a domain sid usually have three subauthorities but we test that CreateWellKnownSid doesn't check it */
    AllocateAndInitializeSid(&ident, 6, SECURITY_NT_NON_UNIQUE, 12, 23, 34, 45, 56, 0, 0, &domainsid);

    for (i = 0; i < ARRAY_SIZE(well_known_sid_values); i++)
    {
        const struct well_known_sid_value *value = &well_known_sid_values[i];
        char sid_buffer[SECURITY_MAX_SID_SIZE];
        LPSTR str;
        DWORD cb;

        if (value->sid_string == NULL)
            continue;

        /* some SIDs aren't implemented by all Windows versions - detect it */
        cb = sizeof(sid_buffer);
        if (!CreateWellKnownSid(i, NULL, sid_buffer, &cb))
        {
            skip("Well known SID %u not implemented\n", i);
            continue;
        }

        cb = sizeof(sid_buffer);
        ok(CreateWellKnownSid(i, value->without_domain ? NULL : domainsid, sid_buffer, &cb), "Couldn't create well known sid %u\n", i);
        expect_eq(GetSidLengthRequired(*GetSidSubAuthorityCount(sid_buffer)), cb, DWORD, "%ld");
        ok(IsValidSid(sid_buffer), "The sid is not valid\n");
        ok(ConvertSidToStringSidA(sid_buffer, &str), "Couldn't convert SID to string\n");
        ok(strcmp(str, value->sid_string) == 0, "%d: SID mismatch - expected %s, got %s\n", i,
            value->sid_string, str);
        LocalFree(str);

        if (value->without_domain)
        {
            char buf2[SECURITY_MAX_SID_SIZE];
            cb = sizeof(buf2);
            ok(CreateWellKnownSid(i, domainsid, buf2, &cb), "Couldn't create well known sid %u with optional domain\n", i);
            expect_eq(GetSidLengthRequired(*GetSidSubAuthorityCount(sid_buffer)), cb, DWORD, "%ld");
            ok(memcmp(buf2, sid_buffer, cb) == 0, "SID create with domain is different than without (%u)\n", i);
        }
    }

    FreeSid(domainsid);
}

static void test_LookupAccountSid(void)
{
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    CHAR accountA[MAX_PATH], domainA[MAX_PATH], usernameA[MAX_PATH];
    DWORD acc_sizeA, dom_sizeA, user_sizeA;
    DWORD real_acc_sizeA, real_dom_sizeA;
    WCHAR accountW[MAX_PATH], domainW[MAX_PATH];
    LSA_OBJECT_ATTRIBUTES object_attributes;
    DWORD acc_sizeW, dom_sizeW;
    DWORD real_acc_sizeW, real_dom_sizeW;
    PSID pUsersSid = NULL;
    SID_NAME_USE use;
    BOOL ret;
    DWORD error, size, cbti = 0;
    MAX_SID  max_sid;
    CHAR *str_sidA;
    int i;
    HANDLE hToken;
    PTOKEN_USER ptiUser = NULL;
    LSA_HANDLE handle;
    NTSTATUS status;

    /* native windows crashes if account size, domain size, or name use is NULL */

    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pUsersSid);
    ok(ret || (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED),
       "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    /* not running on NT so give up */
    if (!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
        return;

    real_acc_sizeA = MAX_PATH;
    real_dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &real_acc_sizeA, domainA, &real_dom_sizeA, &use);
    ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");

    /* try NULL account */
    acc_sizeA = MAX_PATH;
    dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, NULL, &acc_sizeA, domainA, &dom_sizeA, &use);
    ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");

    /* try NULL domain */
    acc_sizeA = MAX_PATH;
    dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, NULL, &dom_sizeA, &use);
    ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");

    /* try a small account buffer */
    acc_sizeA = 1;
    dom_sizeA = MAX_PATH;
    accountA[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    ok(!ret, "LookupAccountSidA() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidA() Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());

    /* try a 0 sized account buffer */
    acc_sizeA = 0;
    dom_sizeA = MAX_PATH;
    accountA[0] = 0;
    LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeA == real_acc_sizeA + 1,
       "LookupAccountSidA() Expected acc_size = %lu, got %lu\n",
       real_acc_sizeA + 1, acc_sizeA);

    /* try a 0 sized account buffer */
    acc_sizeA = 0;
    dom_sizeA = MAX_PATH;
    LookupAccountSidA(NULL, pUsersSid, NULL, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeA == real_acc_sizeA + 1,
       "LookupAccountSid() Expected acc_size = %lu, got %lu\n",
       real_acc_sizeA + 1, acc_sizeA);

    /* try a small domain buffer */
    dom_sizeA = 1;
    acc_sizeA = MAX_PATH;
    accountA[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    ok(!ret, "LookupAccountSidA() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidA() Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());

    /* try a 0 sized domain buffer */
    dom_sizeA = 0;
    acc_sizeA = MAX_PATH;
    accountA[0] = 0;
    LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeA == real_dom_sizeA + 1,
       "LookupAccountSidA() Expected dom_size = %lu, got %lu\n",
       real_dom_sizeA + 1, dom_sizeA);

    /* try a 0 sized domain buffer */
    dom_sizeA = 0;
    acc_sizeA = MAX_PATH;
    LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, NULL, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeA == real_dom_sizeA + 1,
       "LookupAccountSidA() Expected dom_size = %lu, got %lu\n",
       real_dom_sizeA + 1, dom_sizeA);

    real_acc_sizeW = MAX_PATH;
    real_dom_sizeW = MAX_PATH;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &real_acc_sizeW, domainW, &real_dom_sizeW, &use);
    ok(ret, "LookupAccountSidW() Expected TRUE, got FALSE\n");

    /* try an invalid system name */
    real_acc_sizeA = MAX_PATH;
    real_dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA("deepthought", pUsersSid, accountA, &real_acc_sizeA, domainA, &real_dom_sizeA, &use);
    ok(!ret, "LookupAccountSidA() Expected FALSE got TRUE\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE || GetLastError() == RPC_S_INVALID_NET_ADDR /* Vista */,
       "LookupAccountSidA() Expected RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR, got %lu\n", GetLastError());

    /* native windows crashes if domainW or accountW is NULL */

    /* try a small account buffer */
    acc_sizeW = 1;
    dom_sizeW = MAX_PATH;
    accountW[0] = 0;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    ok(!ret, "LookupAccountSidW() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidW() Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());

    /* try a 0 sized account buffer */
    acc_sizeW = 0;
    dom_sizeW = MAX_PATH;
    accountW[0] = 0;
    LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeW == real_acc_sizeW + 1,
       "LookupAccountSidW() Expected acc_size = %lu, got %lu\n",
       real_acc_sizeW + 1, acc_sizeW);

    /* try a 0 sized account buffer */
    acc_sizeW = 0;
    dom_sizeW = MAX_PATH;
    LookupAccountSidW(NULL, pUsersSid, NULL, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeW == real_acc_sizeW + 1,
       "LookupAccountSidW() Expected acc_size = %lu, got %lu\n",
       real_acc_sizeW + 1, acc_sizeW);

    /* try a small domain buffer */
    dom_sizeW = 1;
    acc_sizeW = MAX_PATH;
    accountW[0] = 0;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    ok(!ret, "LookupAccountSidW() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidW() Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());

    /* try a 0 sized domain buffer */
    dom_sizeW = 0;
    acc_sizeW = MAX_PATH;
    accountW[0] = 0;
    LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeW == real_dom_sizeW + 1,
       "LookupAccountSidW() Expected dom_size = %lu, got %lu\n",
       real_dom_sizeW + 1, dom_sizeW);

    /* try a 0 sized domain buffer */
    dom_sizeW = 0;
    acc_sizeW = MAX_PATH;
    LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, NULL, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeW == real_dom_sizeW + 1,
       "LookupAccountSidW() Expected dom_size = %lu, got %lu\n",
       real_dom_sizeW + 1, dom_sizeW);

    acc_sizeW = dom_sizeW = use = 0;
    SetLastError(0xdeadbeef);
    ret = LookupAccountSidW(NULL, pUsersSid, NULL, &acc_sizeW, NULL, &dom_sizeW, &use);
    error = GetLastError();
    ok(!ret, "LookupAccountSidW failed %lu\n", GetLastError());
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %lu\n", error);
    ok(acc_sizeW, "expected non-zero account size\n");
    ok(dom_sizeW, "expected non-zero domain size\n");
    ok(!use, "expected zero use %u\n", use);

    FreeSid(pUsersSid);

    /* Test LookupAccountSid with Sid retrieved from token information.
     This assumes this process is running under the account of the current user.*/
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE, &hToken);
    ok(ret, "OpenProcessToken failed with error %ld\n", GetLastError());
    ret = GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti);
    ok(!ret, "GetTokenInformation failed with error %ld\n", GetLastError());
    ptiUser = malloc(cbti);
    if (GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti))
    {
        acc_sizeA = dom_sizeA = MAX_PATH;
        ret = LookupAccountSidA(NULL, ptiUser->User.Sid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
        ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");
        user_sizeA = MAX_PATH;
        ret = GetUserNameA(usernameA , &user_sizeA);
        ok(ret, "GetUserNameA() Expected TRUE, got FALSE\n");
        ok(lstrcmpA(usernameA, accountA) == 0, "LookupAccountSidA() Expected account name: %s got: %s\n", usernameA, accountA );
    }
    free(ptiUser);

    trace("Well Known SIDs:\n");
    for (i = 0; i <= 60; i++)
    {
        size = SECURITY_MAX_SID_SIZE;
        if (CreateWellKnownSid(i, NULL, &max_sid.sid, &size))
        {
            if (ConvertSidToStringSidA(&max_sid.sid, &str_sidA))
            {
                acc_sizeA = MAX_PATH;
                dom_sizeA = MAX_PATH;
                if (LookupAccountSidA(NULL, &max_sid.sid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use))
                    trace(" %d: %s %s\\%s %d\n", i, str_sidA, domainA, accountA, use);
                LocalFree(str_sidA);
            }
        }
        else
        {
            if (GetLastError() != ERROR_INVALID_PARAMETER)
                trace(" CreateWellKnownSid(%d) failed: %ld\n", i, GetLastError());
            else
                trace(" %d: not supported\n", i);
        }
    }

    ZeroMemory(&object_attributes, sizeof(object_attributes));
    object_attributes.Length = sizeof(object_attributes);

    status = LsaOpenPolicy( NULL, &object_attributes, POLICY_ALL_ACCESS, &handle);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
       "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08lx\n", status);

    /* try a more restricted access mask if necessary */
    if (status == STATUS_ACCESS_DENIED) {
        trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION\n");
        status = LsaOpenPolicy( NULL, &object_attributes, POLICY_VIEW_LOCAL_INFORMATION, &handle);
        ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION) returned 0x%08lx\n", status);
    }

    if (status == STATUS_SUCCESS)
    {
        PPOLICY_ACCOUNT_DOMAIN_INFO info;
        status = LsaQueryInformationPolicy(handle, PolicyAccountDomainInformation, (PVOID*)&info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy() failed, returned 0x%08lx\n", status);
        if (status == STATUS_SUCCESS)
        {
            ok(info->DomainSid!=0, "LsaQueryInformationPolicy(PolicyAccountDomainInformation) missing SID\n");
            if (info->DomainSid)
            {
                int count = *GetSidSubAuthorityCount(info->DomainSid);
                CopySid(GetSidLengthRequired(count), &max_sid, info->DomainSid);
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_USER_RID_ADMIN;
                max_sid.sid.SubAuthorityCount = count + 1;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_USER_RID_GUEST;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_ADMINS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_USERS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_GUESTS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_COMPUTERS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_CONTROLLERS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_CERT_ADMINS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_SCHEMA_ADMINS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_ENTERPRISE_ADMINS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_POLICY_ADMINS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = DOMAIN_ALIAS_RID_RAS_SERVERS;
                test_sid_str((PSID)&max_sid.sid);
                max_sid.sid.SubAuthority[count] = 1000;	/* first user account */
                test_sid_str((PSID)&max_sid.sid);
            }

            LsaFreeMemory(info);
        }

        status = LsaClose(handle);
        ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08lx\n", status);
    }
}

static BOOL get_sid_info(PSID psid, LPSTR *user, LPSTR *dom)
{
    static CHAR account[UNLEN + 1];
    static CHAR domain[UNLEN + 1];
    DWORD size, dom_size;
    SID_NAME_USE use;

    *user = account;
    *dom = domain;

    size = dom_size = UNLEN + 1;
    account[0] = '\0';
    domain[0] = '\0';
    SetLastError(0xdeadbeef);
    return LookupAccountSidA(NULL, psid, account, &size, domain, &dom_size, &use);
}

static void check_wellknown_name(const char* name, WELL_KNOWN_SID_TYPE result)
{
    SID_IDENTIFIER_AUTHORITY ident = { SECURITY_NT_AUTHORITY };
    PSID domainsid = NULL;
    char wk_sid[SECURITY_MAX_SID_SIZE];
    DWORD cb;

    DWORD sid_size, domain_size;
    SID_NAME_USE sid_use;
    LPSTR domain, account, sid_domain, wk_domain, wk_account;
    PSID psid;
    BOOL ret ,ret2;

    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, " %s Should have failed to lookup account name\n", name);
    psid = malloc(sid_size);
    domain = malloc(domain_size);
    ret = LookupAccountNameA(NULL, name, psid, &sid_size, domain, &domain_size, &sid_use);

    if (!result)
    {
        ok(!ret, " %s Should have failed to lookup account name\n",name);
        goto cleanup;
    }

    AllocateAndInitializeSid(&ident, 6, SECURITY_NT_NON_UNIQUE, 12, 23, 34, 45, 56, 0, 0, &domainsid);
    cb = sizeof(wk_sid);
    if (!CreateWellKnownSid(result, domainsid, wk_sid, &cb))
    {
        win_skip("SID %i is not available on the system\n",result);
        goto cleanup;
    }

    ret2 = get_sid_info(wk_sid, &wk_account, &wk_domain);
    if (!ret2 && GetLastError() == ERROR_NONE_MAPPED)
    {
        win_skip("CreateWellKnownSid() succeeded but the account '%s' is not present (W2K)\n", name);
        goto cleanup;
    }

    get_sid_info(psid, &account, &sid_domain);

    ok(ret, "Failed to lookup account name %s\n",name);
    ok(sid_size != 0, "sid_size was zero\n");

#ifndef __REACTOS__ // This crashes on WS03, Vista, Win7, Win8.1, and Win10 1607.
    ok(EqualSid(psid,wk_sid),"%s Sid %s fails to match well known sid %s!\n",
       name, debugstr_sid(psid), debugstr_sid(wk_sid));
#endif

    ok(!lstrcmpA(account, wk_account), "Expected %s , got %s\n", account, wk_account);
    ok(!lstrcmpA(domain, wk_domain), "Expected %s, got %s\n", wk_domain, domain);
    ok(sid_use == SidTypeWellKnownGroup , "Expected Use (5), got %d\n", sid_use);

cleanup:
    FreeSid(domainsid);
    free(psid);
    free(domain);
}

static void test_LookupAccountName(void)
{
    DWORD sid_size, domain_size, user_size;
    DWORD sid_save, domain_save;
    CHAR user_name[UNLEN + 1];
    CHAR computer_name[UNLEN + 1];
    SID_NAME_USE sid_use;
    LPSTR domain, account, sid_dom;
    PSID psid;
    BOOL ret;

    /* native crashes if (assuming all other parameters correct):
     *  - peUse is NULL
     *  - Sid is NULL and cbSid is > 0
     *  - cbSid or cchReferencedDomainName are NULL
     *  - ReferencedDomainName is NULL and cchReferencedDomainName is the correct size
     */

    user_size = UNLEN + 1;
    SetLastError(0xdeadbeef);
    ret = GetUserNameA(user_name, &user_size);
    ok(ret, "Failed to get user name : %ld\n", GetLastError());

    /* get sizes */
    sid_size = 0;
    domain_size = 0;
    sid_use = 0xcafebabe;
    SetLastError(0xdeadbeef);
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("LookupAccountNameA is not implemented\n");
        return;
    }
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(sid_size != 0, "Expected non-zero sid size\n");
    ok(domain_size != 0, "Expected non-zero domain size\n");
    ok(sid_use == (SID_NAME_USE)0xcafebabe, "Expected 0xcafebabe, got %d\n", sid_use);

    sid_save = sid_size;
    domain_save = domain_size;

    psid = malloc(sid_size);
    domain = malloc(domain_size);

    /* try valid account name */
    ret = LookupAccountNameA(NULL, user_name, psid, &sid_size, domain, &domain_size, &sid_use);
    get_sid_info(psid, &account, &sid_dom);
    ok(ret, "Failed to lookup account name\n");
    ok(sid_size == GetLengthSid(psid), "Expected %ld, got %ld\n", GetLengthSid(psid), sid_size);
    ok(!lstrcmpA(account, user_name), "Expected %s, got %s\n", user_name, account);
    ok(!lstrcmpiA(domain, sid_dom), "Expected %s, got %s\n", sid_dom, domain);
    ok(domain_size == domain_save - 1, "Expected %ld, got %ld\n", domain_save - 1, domain_size);
    ok(strlen(domain) == domain_size, "Expected %d, got %ld\n", lstrlenA(domain), domain_size);
    ok(sid_use == SidTypeUser, "Expected SidTypeUser (%d), got %d\n", SidTypeUser, sid_use);
    domain_size = domain_save;
    sid_size = sid_save;

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ret = LookupAccountNameA(NULL, "Everyone", psid, &sid_size, domain, &domain_size, &sid_use);
        get_sid_info(psid, &account, &sid_dom);
        ok(ret, "Failed to lookup account name\n");
        ok(sid_size != 0, "sid_size was zero\n");
        ok(!lstrcmpA(account, "Everyone"), "Expected Everyone, got %s\n", account);
        ok(!lstrcmpiA(domain, sid_dom), "Expected %s, got %s\n", sid_dom, domain);
        ok(domain_size == 0, "Expected 0, got %ld\n", domain_size);
        ok(strlen(domain) == domain_size, "Expected %d, got %ld\n", lstrlenA(domain), domain_size);
        ok(sid_use == SidTypeWellKnownGroup, "Expected SidTypeWellKnownGroup (%d), got %d\n", SidTypeWellKnownGroup, sid_use);
        domain_size = domain_save;
    }

    /* NULL Sid with zero sid size */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, domain, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(sid_size == sid_save, "Expected %ld, got %ld\n", sid_save, sid_size);
    ok(domain_size == domain_save, "Expected %ld, got %ld\n", domain_save, domain_size);

    /* try cchReferencedDomainName - 1 */
    SetLastError(0xdeadbeef);
    domain_size--;
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, domain, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(sid_size == sid_save, "Expected %ld, got %ld\n", sid_save, sid_size);
    ok(domain_size == domain_save, "Expected %ld, got %ld\n", domain_save, domain_size);

    /* NULL ReferencedDomainName with zero domain name size */
    SetLastError(0xdeadbeef);
    domain_size = 0;
    ret = LookupAccountNameA(NULL, user_name, psid, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(sid_size == sid_save, "Expected %ld, got %ld\n", sid_save, sid_size);
    ok(domain_size == domain_save, "Expected %ld, got %ld\n", domain_save, domain_size);

    free(psid);
    free(domain);

    /* get sizes for NULL account name */
    sid_size = 0;
    domain_size = 0;
    sid_use = 0xcafebabe;
    SetLastError(0xdeadbeef);
    ret = LookupAccountNameA(NULL, NULL, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(sid_size != 0, "Expected non-zero sid size\n");
    ok(domain_size != 0, "Expected non-zero domain size\n");
    ok(sid_use == (SID_NAME_USE)0xcafebabe, "Expected 0xcafebabe, got %d\n", sid_use);

    psid = malloc(sid_size);
    domain = malloc(domain_size);

    /* try NULL account name */
    ret = LookupAccountNameA(NULL, NULL, psid, &sid_size, domain, &domain_size, &sid_use);
    get_sid_info(psid, &account, &sid_dom);
    ok(ret, "Failed to lookup account name\n");
    /* Using a fixed string will not work on different locales */
    ok(!lstrcmpiA(account, domain),
       "Got %s for account and %s for domain, these should be the same\n", account, domain);
    ok(sid_use == SidTypeDomain, "Expected SidTypeDomain (%d), got %d\n", SidTypeDomain, sid_use);

    free(psid);
    free(domain);

    /* try an invalid account name */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, "oogabooga", NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NONE_MAPPED ||
       broken(GetLastError() == ERROR_TRUSTED_RELATIONSHIP_FAILURE),
       "Expected ERROR_NONE_MAPPED, got %ld\n", GetLastError());
    ok(sid_size == 0, "Expected 0, got %ld\n", sid_size);
    ok(domain_size == 0, "Expected 0, got %ld\n", domain_size);

    /* try an invalid system name */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA("deepthought", NULL, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE || GetLastError() == RPC_S_INVALID_NET_ADDR /* Vista */,
       "Expected RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR, got %ld\n", GetLastError());
    ok(sid_size == 0, "Expected 0, got %ld\n", sid_size);
    ok(domain_size == 0, "Expected 0, got %ld\n", domain_size);

    /* try with the computer name as the account name */
    domain_size = sizeof(computer_name);
    GetComputerNameA(computer_name, &domain_size);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, computer_name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
       GetLastError() == ERROR_NONE_MAPPED /* in a domain */ ||
       broken(GetLastError() == ERROR_TRUSTED_DOMAIN_FAILURE) ||
       broken(GetLastError() == ERROR_TRUSTED_RELATIONSHIP_FAILURE)),
       "LookupAccountNameA failed: %ld\n", GetLastError());
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        psid = malloc(sid_size);
        domain = malloc(domain_size);
        ret = LookupAccountNameA(NULL, computer_name, psid, &sid_size, domain, &domain_size, &sid_use);
        ok(ret, "LookupAccountNameA failed: %ld\n", GetLastError());
        ok(sid_use == SidTypeDomain ||
           (sid_use == SidTypeUser && ! strcmp(computer_name, user_name)), "expected SidTypeDomain for %s, got %d\n", computer_name, sid_use);
        free(domain);
        free(psid);
    }

    /* Well Known names */
    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (skipping well known name creation tests)\n");
        return;
    }

    check_wellknown_name("LocalService", WinLocalServiceSid);
    check_wellknown_name("Local Service", WinLocalServiceSid);
    /* 2 spaces */
    check_wellknown_name("Local  Service", 0);
    check_wellknown_name("NetworkService", WinNetworkServiceSid);
    check_wellknown_name("Network Service", WinNetworkServiceSid);

    /* example of some names where the spaces are not optional */
    check_wellknown_name("Terminal Server User", WinTerminalServerSid);
    check_wellknown_name("TerminalServer User", 0);
    check_wellknown_name("TerminalServerUser", 0);
    check_wellknown_name("Terminal ServerUser", 0);

    check_wellknown_name("enterprise domain controllers",WinEnterpriseControllersSid);
    check_wellknown_name("enterprisedomain controllers", 0);
    check_wellknown_name("enterprise domaincontrollers", 0);
    check_wellknown_name("enterprisedomaincontrollers", 0);

    /* case insensitivity */
    check_wellknown_name("lOCAlServICE", WinLocalServiceSid);

    /* fully qualified account names */
    check_wellknown_name("NT AUTHORITY\\LocalService", WinLocalServiceSid);
    check_wellknown_name("nt authority\\Network Service", WinNetworkServiceSid);
    check_wellknown_name("nt authority test\\Network Service", 0);
    check_wellknown_name("Dummy\\Network Service", 0);
    check_wellknown_name("ntauthority\\Network Service", 0);
}

static void test_security_descriptor(void)
{
    SECURITY_DESCRIPTOR sd, *sd_rel, *sd_rel2, *sd_abs;
    char buf[8192];
    DWORD size, size_dacl, size_sacl, size_owner, size_group;
    BOOL isDefault, isPresent, ret;
    PACL pacl, dacl, sacl;
    PSID psid, owner, group;

    SetLastError(0xdeadbeef);
    ret = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("InitializeSecurityDescriptor is not implemented\n");
        return;
    }

    ok(GetSecurityDescriptorOwner(&sd, &psid, &isDefault), "GetSecurityDescriptorOwner failed\n");
    expect_eq(psid, NULL, PSID, "%p");
    expect_eq(isDefault, FALSE, BOOL, "%d");
    sd.Control |= SE_DACL_PRESENT | SE_SACL_PRESENT;

    SetLastError(0xdeadbeef);
    size = 5;
    expect_eq(MakeSelfRelativeSD(&sd, buf, &size), FALSE, BOOL, "%d");
    expect_eq(GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER, DWORD, "%lu");
    ok(size > 5, "Size not increased\n");
    if (size <= 8192)
    {
        expect_eq(MakeSelfRelativeSD(&sd, buf, &size), TRUE, BOOL, "%d");
        ok(GetSecurityDescriptorOwner(&sd, &psid, &isDefault), "GetSecurityDescriptorOwner failed\n");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
        ok(GetSecurityDescriptorGroup(&sd, &psid, &isDefault), "GetSecurityDescriptorGroup failed\n");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
        ok(GetSecurityDescriptorDacl(&sd, &isPresent, &pacl, &isDefault), "GetSecurityDescriptorDacl failed\n");
        expect_eq(isPresent, TRUE, BOOL, "%d");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
        ok(GetSecurityDescriptorSacl(&sd, &isPresent, &pacl, &isDefault), "GetSecurityDescriptorSacl failed\n");
        expect_eq(isPresent, TRUE, BOOL, "%d");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
    }

    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)"
        "(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)"
        "(AU;NPSA;0x12019f;;;SU)", SDDL_REVISION_1, (void **)&sd_rel, NULL);
    ok(ret, "got %lu\n", GetLastError());

    size = 0;
    ret = MakeSelfRelativeSD(sd_rel, NULL, &size);
    todo_wine ok(!ret && GetLastError() == ERROR_BAD_DESCRIPTOR_FORMAT, "got %lu\n", GetLastError());

    /* convert to absolute form */
    size = size_dacl = size_sacl = size_owner = size_group = 0;
    ret = MakeAbsoluteSD(sd_rel, NULL, &size, NULL, &size_dacl, NULL, &size_sacl, NULL, &size_owner, NULL,
                         &size_group);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", GetLastError());

    sd_abs = malloc(size + size_dacl + size_sacl + size_owner + size_group);
    dacl = (PACL)(sd_abs + 1);
    sacl = (PACL)((char *)dacl + size_dacl);
    owner = (PSID)((char *)sacl + size_sacl);
    group = (PSID)((char *)owner + size_owner);
    ret = MakeAbsoluteSD(sd_rel, sd_abs, &size, dacl, &size_dacl, sacl, &size_sacl, owner, &size_owner,
                         group, &size_group);
    ok(ret, "got %lu\n", GetLastError());

    size = 0;
    ret = MakeSelfRelativeSD(sd_abs, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", GetLastError());
    ok(size == 184, "got %lu\n", size);

    size += 4;
    sd_rel2 = malloc(size);
    ret = MakeSelfRelativeSD(sd_abs, sd_rel2, &size);
    ok(ret, "got %lu\n", GetLastError());
    ok(size == 188, "got %lu\n", size);

    free(sd_abs);
    free(sd_rel2);
    LocalFree(sd_rel);
}

#define TEST_GRANTED_ACCESS(a,b) test_granted_access(a,b,0,__LINE__)
#define TEST_GRANTED_ACCESS2(a,b,c) test_granted_access(a,b,c,__LINE__)
static void test_granted_access(HANDLE handle, ACCESS_MASK access,
                                ACCESS_MASK alt, int line)
{
    OBJECT_BASIC_INFORMATION obj_info;
    NTSTATUS status;

    status = NtQueryObject( handle, ObjectBasicInformation, &obj_info,
                            sizeof(obj_info), NULL );
    ok_(__FILE__, line)(!status, "NtQueryObject with err: %08lx\n", status);
    if (alt)
        ok_(__FILE__, line)(obj_info.GrantedAccess == access ||
            obj_info.GrantedAccess == alt, "Granted access should be 0x%08lx "
            "or 0x%08lx, instead of 0x%08lx\n", access, alt, obj_info.GrantedAccess);
    else
        ok_(__FILE__, line)(obj_info.GrantedAccess == access, "Granted access should "
            "be 0x%08lx, instead of 0x%08lx\n", access, obj_info.GrantedAccess);
}

#define CHECK_SET_SECURITY(o,i,e) \
    do{ \
        BOOL res_; \
        DWORD err; \
        SetLastError( 0xdeadbeef ); \
        res_ = SetKernelObjectSecurity( o, i, SecurityDescriptor ); \
        err = GetLastError(); \
        if (e == ERROR_SUCCESS) \
            ok(res_, "SetKernelObjectSecurity failed with %ld\n", err); \
        else \
            ok(!res_ && err == e, "SetKernelObjectSecurity should have failed " \
               "with %s, instead of %ld\n", #e, err); \
    }while(0)

static void test_process_security(void)
{
    BOOL res;
    PTOKEN_USER user;
    PTOKEN_OWNER owner;
    PTOKEN_PRIMARY_GROUP group;
    PSID AdminSid = NULL, UsersSid = NULL, UserSid = NULL;
    PACL Acl = NULL, ThreadAcl = NULL;
    SECURITY_DESCRIPTOR *SecurityDescriptor = NULL, *ThreadSecurityDescriptor = NULL;
    char buffer[MAX_PATH], account[MAX_PATH], domain[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    SECURITY_ATTRIBUTES psa, tsa;
    HANDLE token, event;
    DWORD size, acc_size, dom_size, ret;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    PSID EveryoneSid = NULL;
    SID_NAME_USE use;

    Acl = malloc(256);
    res = InitializeAcl(Acl, 256, ACL_REVISION);
    if (!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ACLs not implemented - skipping tests\n");
        free(Acl);
        return;
    }
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    /* get owner from the token we might be running as a user not admin */
    res = OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &token );
    ok(res, "OpenProcessToken failed with error %ld\n", GetLastError());
    if (!res)
    {
        free(Acl);
        return;
    }

    res = GetTokenInformation( token, TokenOwner, NULL, 0, &size );
    ok(!res, "Expected failure, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    owner = malloc(size);
    res = GetTokenInformation( token, TokenOwner, owner, size, &size );
    ok(res, "GetTokenInformation failed with error %ld\n", GetLastError());
    AdminSid = owner->Owner;
    test_sid_str(AdminSid);

    res = GetTokenInformation( token, TokenPrimaryGroup, NULL, 0, &size );
    ok(!res, "Expected failure, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    group = malloc(size);
    res = GetTokenInformation( token, TokenPrimaryGroup, group, size, &size );
    ok(res, "GetTokenInformation failed with error %ld\n", GetLastError());
    UsersSid = group->PrimaryGroup;
    test_sid_str(UsersSid);

    acc_size = sizeof(account);
    dom_size = sizeof(domain);
    ret = LookupAccountSidA( NULL, UsersSid, account, &acc_size, domain, &dom_size, &use );
    ok(ret, "LookupAccountSid failed with %ld\n", ret);
    ok(use == SidTypeGroup, "expect SidTypeGroup, got %d\n", use);
    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
        skip("Non-English locale (test with hardcoded 'None')\n");
    else
        ok(!strcmp(account, "None"), "expect None, got %s\n", account);

    res = GetTokenInformation( token, TokenUser, NULL, 0, &size );
    ok(!res, "Expected failure, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    user = malloc(size);
    res = GetTokenInformation( token, TokenUser, user, size, &size );
    ok(res, "GetTokenInformation failed with error %ld\n", GetLastError());
    UserSid = user->User.Sid;
    test_sid_str(UserSid);
    ok(EqualPrefixSid(UsersSid, UserSid), "TokenPrimaryGroup Sid and TokenUser Sid don't match.\n");

    CloseHandle( token );
    if (!res)
    {
        free(group);
        free(owner);
        free(user);
        free(Acl);
        return;
    }

    res = AddAccessDeniedAce(Acl, ACL_REVISION, PROCESS_VM_READ, AdminSid);
    ok(res, "AddAccessDeniedAce failed with error %ld\n", GetLastError());
    res = AddAccessAllowedAce(Acl, ACL_REVISION, PROCESS_ALL_ACCESS, AdminSid);
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());

    SecurityDescriptor = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    res = InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ok(res, "InitializeSecurityDescriptor failed with error %ld\n", GetLastError());

    event = CreateEventA( NULL, TRUE, TRUE, "test_event" );
    ok(event != NULL, "CreateEvent %ld\n", GetLastError());

    SecurityDescriptor->Revision = 0;
    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_UNKNOWN_REVISION );
    SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;

    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_INVALID_SECURITY_DESCR );
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_INVALID_SECURITY_DESCR );
    CHECK_SET_SECURITY( event, SACL_SECURITY_INFORMATION, ERROR_ACCESS_DENIED );
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    /* NULL DACL is valid and means that everyone has access */
    SecurityDescriptor->Control |= SE_DACL_PRESENT;
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );

#ifdef __REACTOS__
    /* This crashes on Vista, Win7, and Win8.1. */
    if (GetNTVersion() < _WIN32_WINNT_VISTA || GetNTVersion() >= _WIN32_WINNT_WIN10) {
#endif
    /* Set owner and group and dacl */
    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %ld\n", GetLastError());
    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_owner_equal( event, AdminSid, __LINE__ );

    res = SetSecurityDescriptorGroup(SecurityDescriptor, EveryoneSid, FALSE);
    ok(res, "SetSecurityDescriptorGroup failed with error %ld\n", GetLastError());
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_group_equal( event, EveryoneSid, __LINE__ );

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    /* setting a dacl should not change the owner or group */
    test_owner_equal( event, AdminSid, __LINE__ );
    test_group_equal( event, EveryoneSid, __LINE__ );

    /* Test again with a different SID in case the previous SID also happens to
     * be the one that is incorrectly replacing the group. */
    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, FALSE);
    ok(res, "SetSecurityDescriptorGroup failed with error %ld\n", GetLastError());
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_group_equal( event, UsersSid, __LINE__ );

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_group_equal( event, UsersSid, __LINE__ );
#ifdef __REACTOS__
    }
#endif

    sprintf(buffer, "%s security test", myARGV[0]);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    psa.nLength = sizeof(psa);
    psa.lpSecurityDescriptor = SecurityDescriptor;
    psa.bInheritHandle = TRUE;

    ThreadSecurityDescriptor = malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );
    res = InitializeSecurityDescriptor( ThreadSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
    ok(res, "InitializeSecurityDescriptor failed with error %ld\n", GetLastError());

    ThreadAcl = malloc( 256 );
    res = InitializeAcl( ThreadAcl, 256, ACL_REVISION );
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());
    res = AddAccessDeniedAce( ThreadAcl, ACL_REVISION, THREAD_SET_THREAD_TOKEN, AdminSid );
    ok(res, "AddAccessDeniedAce failed with error %ld\n", GetLastError() );
    res = AddAccessAllowedAce( ThreadAcl, ACL_REVISION, THREAD_ALL_ACCESS, AdminSid );
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());

    res = SetSecurityDescriptorOwner( ThreadSecurityDescriptor, AdminSid, FALSE );
    ok(res, "SetSecurityDescriptorOwner failed with error %ld\n", GetLastError());
    res = SetSecurityDescriptorGroup( ThreadSecurityDescriptor, UsersSid, FALSE );
    ok(res, "SetSecurityDescriptorGroup failed with error %ld\n", GetLastError());
    res = SetSecurityDescriptorDacl( ThreadSecurityDescriptor, TRUE, ThreadAcl, FALSE );
    ok(res, "SetSecurityDescriptorDacl failed with error %ld\n", GetLastError());

    tsa.nLength = sizeof(tsa);
    tsa.lpSecurityDescriptor = ThreadSecurityDescriptor;
    tsa.bInheritHandle = TRUE;

    /* Doesn't matter what ACL say we should get full access for ourselves */
    res = CreateProcessA( NULL, buffer, &psa, &tsa, FALSE, 0, NULL, NULL, &startup, &info );
    ok(res, "CreateProcess with err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS2( info.hProcess, PROCESS_ALL_ACCESS_NT4,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );
    TEST_GRANTED_ACCESS2( info.hThread, THREAD_ALL_ACCESS_NT4,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );
    wait_child_process( info.hProcess );

    FreeSid(EveryoneSid);
    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( event );
    free(group);
    free(owner);
    free(user);
    free(Acl);
    free(SecurityDescriptor);
    free(ThreadAcl);
    free(ThreadSecurityDescriptor);
}

static void test_process_security_child(void)
{
    HANDLE handle, handle1;
    BOOL ret;
    DWORD err;

    handle = OpenProcess( PROCESS_TERMINATE, FALSE, GetCurrentProcessId() );
    ok(handle != NULL, "OpenProcess(PROCESS_TERMINATE) with err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS( handle, PROCESS_TERMINATE );

    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, 0, TRUE, DUPLICATE_SAME_ACCESS );
    ok(ret, "duplicating handle err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS( handle1, PROCESS_TERMINATE );

    CloseHandle( handle1 );

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, PROCESS_ALL_ACCESS, TRUE, 0 );
    err = GetLastError();
#ifdef __REACTOS__
    ok((!ret && err == ERROR_ACCESS_DENIED) || broken(ret && err == 0xdeadbeef) /* Vista-Win10 1607 */, "duplicating handle should have failed "
#else
    ok(!ret && err == ERROR_ACCESS_DENIED, "duplicating handle should have failed "
#endif
       "with STATUS_ACCESS_DENIED, instead of err:%ld\n", err);

    CloseHandle( handle );

#ifndef __REACTOS__ // Incorrect for WS03-Win10 1607
    /* These two should fail - they are denied by ACL */
    handle = OpenProcess( PROCESS_VM_READ, FALSE, GetCurrentProcessId() );
    ok(handle == NULL, "OpenProcess(PROCESS_VM_READ) should have failed\n");
    handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    ok(handle == NULL, "OpenProcess(PROCESS_ALL_ACCESS) should have failed\n");
#endif

    /* Documented privilege elevation */
    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                           &handle, 0, TRUE, DUPLICATE_SAME_ACCESS );
    ok(ret, "duplicating handle err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS2( handle, PROCESS_ALL_ACCESS_NT4,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );

    CloseHandle( handle );

    /* Same only explicitly asking for all access rights */
    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                           &handle, PROCESS_ALL_ACCESS, TRUE, 0 );
    ok(ret, "duplicating handle err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS2( handle, PROCESS_ALL_ACCESS_NT4,
                          PROCESS_ALL_ACCESS | PROCESS_QUERY_LIMITED_INFORMATION );
    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, PROCESS_VM_READ, TRUE, 0 );
    ok(ret, "duplicating handle err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS( handle1, PROCESS_VM_READ );
    CloseHandle( handle1 );
    CloseHandle( handle );

    /* Test thread security */
    handle = OpenThread( THREAD_TERMINATE, FALSE, GetCurrentThreadId() );
    ok(handle != NULL, "OpenThread(THREAD_TERMINATE) with err:%ld\n", GetLastError());
    TEST_GRANTED_ACCESS( handle, THREAD_TERMINATE );
    CloseHandle( handle );

#ifndef __REACTOS__ // Incorrect for WS03-Win10 1607
    handle = OpenThread( THREAD_SET_THREAD_TOKEN, FALSE, GetCurrentThreadId() );
    ok(handle == NULL, "OpenThread(THREAD_SET_THREAD_TOKEN) should have failed\n");
#endif
}

static void test_impersonation_level(void)
{
    HANDLE Token, ProcessToken;
    HANDLE Token2;
    DWORD Size;
    TOKEN_PRIVILEGES *Privileges;
    TOKEN_USER *User;
    PRIVILEGE_SET *PrivilegeSet;
    BOOL AccessGranted;
    BOOL ret;
    HKEY hkey;
    DWORD error;

    SetLastError(0xdeadbeef);
    ret = ImpersonateSelf(SecurityAnonymous);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("ImpersonateSelf is not implemented\n");
        return;
    }
    ok(ret, "ImpersonateSelf(SecurityAnonymous) failed with error %ld\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(!ret, "OpenThreadToken should have failed\n");
    error = GetLastError();
    ok(error == ERROR_CANT_OPEN_ANONYMOUS, "OpenThreadToken on anonymous token should have returned ERROR_CANT_OPEN_ANONYMOUS instead of %ld\n", error);
    /* can't perform access check when opening object against an anonymous impersonation token */
    todo_wine {
    error = RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_CANT_OPEN_ANONYMOUS || error == ERROR_BAD_IMPERSONATION_LEVEL,
       "RegOpenKeyEx failed with %ld\n", error);
    }
    RevertToSelf();

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &ProcessToken);
    ok(ret, "OpenProcessToken failed with error %ld\n", GetLastError());

    ret = DuplicateTokenEx(ProcessToken,
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, NULL,
        SecurityAnonymous, TokenImpersonation, &Token);
    ok(ret, "DuplicateTokenEx failed with error %ld\n", GetLastError());
    /* can't increase the impersonation level */
    ret = DuplicateToken(Token, SecurityIdentification, &Token2);
    error = GetLastError();
    ok(!ret && error == ERROR_BAD_IMPERSONATION_LEVEL,
        "Duplicating a token and increasing the impersonation level should have failed with ERROR_BAD_IMPERSONATION_LEVEL instead of %ld\n", error);
    /* we can query anything from an anonymous token, including the user */
    ret = GetTokenInformation(Token, TokenUser, NULL, 0, &Size);
    error = GetLastError();
    ok(!ret && error == ERROR_INSUFFICIENT_BUFFER, "GetTokenInformation(TokenUser) should have failed with ERROR_INSUFFICIENT_BUFFER instead of %ld\n", error);
    User = malloc(Size);
    ret = GetTokenInformation(Token, TokenUser, User, Size, &Size);
    ok(ret, "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    free(User);

    /* PrivilegeCheck fails with SecurityAnonymous level */
    ret = GetTokenInformation(Token, TokenPrivileges, NULL, 0, &Size);
    error = GetLastError();
    ok(!ret && error == ERROR_INSUFFICIENT_BUFFER, "GetTokenInformation(TokenPrivileges) should have failed with ERROR_INSUFFICIENT_BUFFER instead of %ld\n", error);
    Privileges = malloc(Size);
    ret = GetTokenInformation(Token, TokenPrivileges, Privileges, Size, &Size);
    ok(ret, "GetTokenInformation(TokenPrivileges) failed with error %ld\n", GetLastError());

    PrivilegeSet = malloc(FIELD_OFFSET(PRIVILEGE_SET, Privilege[Privileges->PrivilegeCount]));
    PrivilegeSet->PrivilegeCount = Privileges->PrivilegeCount;
    memcpy(PrivilegeSet->Privilege, Privileges->Privileges, PrivilegeSet->PrivilegeCount * sizeof(PrivilegeSet->Privilege[0]));
    PrivilegeSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    free(Privileges);

    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    error = GetLastError();
    ok(!ret && error == ERROR_BAD_IMPERSONATION_LEVEL, "PrivilegeCheck for SecurityAnonymous token should have failed with ERROR_BAD_IMPERSONATION_LEVEL instead of %ld\n", error);

    CloseHandle(Token);

    ret = ImpersonateSelf(SecurityIdentification);
    ok(ret, "ImpersonateSelf(SecurityIdentification) failed with error %ld\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(ret, "OpenThreadToken failed with error %ld\n", GetLastError());

    /* can't perform access check when opening object against an identification impersonation token */
    error = RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    todo_wine {
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_BAD_IMPERSONATION_LEVEL || error == ERROR_ACCESS_DENIED,
       "RegOpenKeyEx should have failed with ERROR_INVALID_HANDLE, ERROR_BAD_IMPERSONATION_LEVEL or ERROR_ACCESS_DENIED instead of %ld\n", error);
    }
    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    ok(ret, "PrivilegeCheck for SecurityIdentification failed with error %ld\n", GetLastError());
    CloseHandle(Token);
    RevertToSelf();

    ret = ImpersonateSelf(SecurityImpersonation);
    ok(ret, "ImpersonateSelf(SecurityImpersonation) failed with error %ld\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(ret, "OpenThreadToken failed with error %ld\n", GetLastError());
    error = RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    ok(error == ERROR_SUCCESS, "RegOpenKeyEx should have succeeded instead of failing with %ld\n", error);
    RegCloseKey(hkey);
    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    ok(ret, "PrivilegeCheck for SecurityImpersonation failed with error %ld\n", GetLastError());
    RevertToSelf();

    CloseHandle(Token);
    CloseHandle(ProcessToken);

    free(PrivilegeSet);
}

static void test_SetEntriesInAclW(void)
{
    DWORD res;
    PSID EveryoneSid = NULL, UsersSid = NULL;
    PACL OldAcl = NULL, NewAcl;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    EXPLICIT_ACCESSW ExplicitAccess;
    static const WCHAR wszEveryone[] = {'E','v','e','r','y','o','n','e',0};
    static const WCHAR wszCurrentUser[] = { 'C','U','R','R','E','N','T','_','U','S','E','R','\0'};

    NewAcl = (PACL)0xdeadbeef;
    res = SetEntriesInAclW(0, NULL, NULL, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(NewAcl == NULL, "NewAcl=%p, expected NULL\n", NewAcl);
    LocalFree(NewAcl);

    OldAcl = malloc(256);
    res = InitializeAcl(OldAcl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ACLs not implemented - skipping tests\n");
        free(OldAcl);
        return;
    }
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &UsersSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AddAccessAllowedAce(OldAcl, ACL_REVISION, KEY_READ, UsersSid);
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());

    ExplicitAccess.grfAccessPermissions = KEY_WRITE;
    ExplicitAccess.grfAccessMode = GRANT_ACCESS;
    ExplicitAccess.grfInheritance = NO_INHERITANCE;
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = EveryoneSid;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = 0xDEADBEEF;
    ExplicitAccess.Trustee.pMultipleTrustee = (PVOID)0xDEADBEEF;
    res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.ptstrName = (LPWSTR)wszEveryone;
        res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_BAD_FORM;
        res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER,
            "SetEntriesInAclW failed: %lu\n", res);
        ok(NewAcl == NULL,
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
        res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER,
            "SetEntriesInAclW failed: %lu\n", res);
        ok(NewAcl == NULL,
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ExplicitAccess.grfAccessMode = SET_ACCESS;
        res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);
    }

    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ExplicitAccess.Trustee.ptstrName = (LPWSTR)wszCurrentUser;
    res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.grfAccessMode = REVOKE_ACCESS;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = UsersSid;
    res = SetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    FreeSid(UsersSid);
    FreeSid(EveryoneSid);
    free(OldAcl);
}

static void test_SetEntriesInAclA(void)
{
    DWORD res;
    PSID EveryoneSid = NULL, UsersSid = NULL;
    PACL OldAcl = NULL, NewAcl;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    EXPLICIT_ACCESSA ExplicitAccess;
    static const CHAR szEveryone[] = {'E','v','e','r','y','o','n','e',0};
    static const CHAR szCurrentUser[] = { 'C','U','R','R','E','N','T','_','U','S','E','R','\0'};

    NewAcl = (PACL)0xdeadbeef;
    res = SetEntriesInAclA(0, NULL, NULL, &NewAcl);
    if(res == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetEntriesInAclA is not implemented\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
    ok(NewAcl == NULL,
        "NewAcl=%p, expected NULL\n", NewAcl);
    LocalFree(NewAcl);

    OldAcl = malloc(256);
    res = InitializeAcl(OldAcl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ACLs not implemented - skipping tests\n");
        free(OldAcl);
        return;
    }
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &UsersSid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AddAccessAllowedAce(OldAcl, ACL_REVISION, KEY_READ, UsersSid);
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());

    ExplicitAccess.grfAccessPermissions = KEY_WRITE;
    ExplicitAccess.grfAccessMode = GRANT_ACCESS;
    ExplicitAccess.grfInheritance = NO_INHERITANCE;
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = EveryoneSid;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.ptstrName = (LPSTR)szEveryone;
        res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_BAD_FORM;
        res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER,
            "SetEntriesInAclA failed: %lu\n", res);
        ok(NewAcl == NULL,
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
        res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER,
            "SetEntriesInAclA failed: %lu\n", res);
        ok(NewAcl == NULL,
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ExplicitAccess.grfAccessMode = SET_ACCESS;
        res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);
    }

    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ExplicitAccess.Trustee.ptstrName = (LPSTR)szCurrentUser;
    res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.grfAccessMode = REVOKE_ACCESS;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = UsersSid;
    res = SetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %lu\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    FreeSid(UsersSid);
    FreeSid(EveryoneSid);
    free(OldAcl);
}

/* helper function for test_CreateDirectoryA */
static void get_nt_pathW(const char *name, UNICODE_STRING *nameW)
{
    UNICODE_STRING strW;
    ANSI_STRING str;
    NTSTATUS status;
    BOOLEAN ret;

    RtlInitAnsiString(&str, name);

    status = RtlAnsiStringToUnicodeString(&strW, &str, TRUE);
    ok(!status, "RtlAnsiStringToUnicodeString failed with %08lx\n", status);

    ret = pRtlDosPathNameToNtPathName_U(strW.Buffer, nameW, NULL, NULL);
    ok(ret, "RtlDosPathNameToNtPathName_U failed\n");

    RtlFreeUnicodeString(&strW);
}

static void test_inherited_dacl(PACL dacl, PSID admin_sid, PSID user_sid, DWORD flags, DWORD mask,
                                BOOL todo_count, BOOL todo_sid, BOOL todo_flags, int line)
{
    ACL_SIZE_INFORMATION acl_size;
    ACCESS_ALLOWED_ACE *ace;
    BOOL bret;

    bret = GetAclInformation(dacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok_(__FILE__, line)(bret, "GetAclInformation failed\n");

    todo_wine_if (todo_count)
        ok_(__FILE__, line)(acl_size.AceCount == 2,
            "GetAclInformation returned unexpected entry count (%ld != 2)\n",
            acl_size.AceCount);

    if (acl_size.AceCount > 0)
    {
        bret = GetAce(dacl, 0, (VOID **)&ace);
        ok_(__FILE__, line)(bret, "Failed to get Current User ACE\n");

        bret = EqualSid(&ace->SidStart, user_sid);
        todo_wine_if (todo_sid)
            ok_(__FILE__, line)(bret, "Current User ACE (%s) != Current User SID (%s)\n", debugstr_sid(&ace->SidStart), debugstr_sid(user_sid));

        todo_wine_if (todo_flags)
            ok_(__FILE__, line)(((ACE_HEADER *)ace)->AceFlags == flags,
                "Current User ACE has unexpected flags (0x%x != 0x%lx)\n",
                ((ACE_HEADER *)ace)->AceFlags, flags);

        ok_(__FILE__, line)(ace->Mask == mask,
            "Current User ACE has unexpected mask (0x%lx != 0x%lx)\n",
            ace->Mask, mask);
    }
    if (acl_size.AceCount > 1)
    {
        bret = GetAce(dacl, 1, (VOID **)&ace);
        ok_(__FILE__, line)(bret, "Failed to get Administators Group ACE\n");

        bret = EqualSid(&ace->SidStart, admin_sid);
        todo_wine_if (todo_sid)
            ok_(__FILE__, line)(bret, "Administators Group ACE (%s) != Administators Group SID (%s)\n", debugstr_sid(&ace->SidStart), debugstr_sid(admin_sid));

        todo_wine_if (todo_flags)
            ok_(__FILE__, line)(((ACE_HEADER *)ace)->AceFlags == flags,
                "Administators Group ACE has unexpected flags (0x%x != 0x%lx)\n",
                ((ACE_HEADER *)ace)->AceFlags, flags);

        ok_(__FILE__, line)(ace->Mask == mask,
            "Administators Group ACE has unexpected mask (0x%lx != 0x%lx)\n",
            ace->Mask, mask);
    }
}

static void test_CreateDirectoryA(void)
{
    char admin_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES], *user;
    DWORD sid_size = sizeof(admin_ptr), user_size;
    PSID admin_sid = (PSID) admin_ptr, user_sid;
    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = &sd;
    ACL_SIZE_INFORMATION acl_size;
    UNICODE_STRING tmpfileW;
    SECURITY_ATTRIBUTES sa;
    OBJECT_ATTRIBUTES attr;
    char tmpfile[MAX_PATH];
    char tmpdir[MAX_PATH];
    HANDLE token, hTemp;
    IO_STATUS_BLOCK io;
    struct _SID *owner;
    BOOL bret = TRUE;
    NTSTATUS status;
    DWORD error;
    PACL pDacl;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }
    bret = GetTokenInformation(token, TokenUser, NULL, 0, &user_size);
    ok(!bret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    user = malloc(user_size);
    bret = GetTokenInformation(token, TokenUser, user, user_size, &user_size);
    ok(bret, "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    CloseHandle( token );
    user_sid = ((TOKEN_USER *)user)->User.Sid;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    pDacl = calloc(1, 100);
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE,
                                 GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE,
                                 GENERIC_ALL, admin_sid);
    ok(bret, "Failed to add Administrator Group to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor.\n");

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    bret = CreateDirectoryA(tmpdir, &sa);
    ok(bret == TRUE, "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());
    free(pDacl);

#ifdef __REACTOS__
    /* The rest of this test crashes on WS03, Vista, Win7, and Win8.1. */
    if (GetNTVersion() < _WIN32_WINNT_WIN10)
        goto done;
#endif
    SetLastError(0xdeadbeef);
    error = GetNamedSecurityInfoA(tmpdir, SE_FILE_OBJECT,
                                  OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSID*)&owner,
                                  NULL, &pDacl, NULL, &pSD);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetNamedSecurityInfoA is not implemented\n");
        goto done;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);
    test_inherited_dacl(pDacl, admin_sid, user_sid, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE,
                        0x1f01ff, FALSE, TRUE, FALSE, __LINE__);
    LocalFree(pSD);

    /* Test inheritance of ACLs in CreateFile without security descriptor */
    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");

    hTemp = CreateFileA(tmpfile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    ok(hTemp != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError());

    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                  OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "Failed to get permissions on file\n");
    test_inherited_dacl(pDacl, admin_sid, user_sid, INHERITED_ACE,
                        0x1f01ff, TRUE, TRUE, TRUE, __LINE__);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in CreateFile with security descriptor -
     * When a security descriptor is set, then inheritance doesn't take effect */
    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = malloc(sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor\n");

    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    hTemp = CreateFileA(tmpfile, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                        CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    ok(hTemp != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError());
    free(pDacl);

    error = GetSecurityInfo(hTemp, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %ld\n", error);
    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    todo_wine
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%ld != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);

    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                  OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    todo_wine
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %ld\n", error);
    if (error == ERROR_SUCCESS)
    {
        bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
        ok(bret, "GetAclInformation failed\n");
        todo_wine
        ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%ld != 0).\n",
                                   acl_size.AceCount);
        LocalFree(pSD);
    }
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in NtCreateFile without security descriptor */
    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");
    get_nt_pathW(tmpfile, &tmpfileW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &tmpfileW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile(&hTemp, GENERIC_WRITE | DELETE, &attr, &io, NULL, 0,
                          FILE_SHARE_READ, FILE_CREATE, FILE_DELETE_ON_CLOSE, NULL, 0);
    ok(!status, "NtCreateFile failed with %08lx\n", status);
    RtlFreeUnicodeString(&tmpfileW);

    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                  OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "Failed to get permissions on file\n");
    test_inherited_dacl(pDacl, admin_sid, user_sid, INHERITED_ACE,
                        0x1f01ff, TRUE, TRUE, TRUE, __LINE__);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in NtCreateFile with security descriptor -
     * When a security descriptor is set, then inheritance doesn't take effect */
    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = malloc(sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor\n");

    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");
    get_nt_pathW(tmpfile, &tmpfileW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &tmpfileW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = pSD;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile(&hTemp, GENERIC_WRITE | DELETE, &attr, &io, NULL, 0,
                          FILE_SHARE_READ, FILE_CREATE, FILE_DELETE_ON_CLOSE, NULL, 0);
    ok(!status, "NtCreateFile failed with %08lx\n", status);
    RtlFreeUnicodeString(&tmpfileW);
    free(pDacl);

    error = GetSecurityInfo(hTemp, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %ld\n", error);
    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    todo_wine
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%ld != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);

    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                  OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    todo_wine
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %ld\n", error);
    if (error == ERROR_SUCCESS)
    {
        bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
        ok(bret, "GetAclInformation failed\n");
        todo_wine
        ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%ld != 0).\n",
                                   acl_size.AceCount);
        LocalFree(pSD);
    }
    CloseHandle(hTemp);

done:
    free(user);
    bret = RemoveDirectoryA(tmpdir);
    ok(bret == TRUE, "RemoveDirectoryA should always succeed\n");
}

static void test_GetNamedSecurityInfoA(void)
{
    char admin_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES], *user;
    char system_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES];
    char users_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES];
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    PSID admin_sid = (PSID) admin_ptr, users_sid = (PSID) users_ptr;
    PSID system_sid = (PSID) system_ptr, user_sid, localsys_sid;
    DWORD sid_size = sizeof(admin_ptr), user_size;
    char invalid_path[] = "/an invalid file path";
    int users_ace_id = -1, admins_ace_id = -1, i;
    char software_key[] = "MACHINE\\Software";
    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH+sizeof(void*)];
    SECURITY_DESCRIPTOR_CONTROL control;
    ACL_SIZE_INFORMATION acl_size;
    CHAR windows_dir[MAX_PATH];
    PSECURITY_DESCRIPTOR pSD;
    ACCESS_ALLOWED_ACE *ace;
    BOOL bret = TRUE;
    char tmpfile[MAX_PATH];
    DWORD error, revision;
    BOOL owner_defaulted;
    BOOL group_defaulted;
    BOOL dacl_defaulted;
    HANDLE token, hTemp, h;
    PSID owner, group;
    BOOL dacl_present;
    PACL pDacl;
    BYTE flags;
    NTSTATUS status;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }
    bret = GetTokenInformation(token, TokenUser, NULL, 0, &user_size);
    ok(!bret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    user = malloc(user_size);
    bret = GetTokenInformation(token, TokenUser, user, user_size, &user_size);
    ok(bret, "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    CloseHandle( token );
    user_sid = ((TOKEN_USER *)user)->User.Sid;

    bret = GetWindowsDirectoryA(windows_dir, MAX_PATH);
    ok(bret, "GetWindowsDirectory failed with error %ld\n", GetLastError());

#ifdef __REACTOS__
    /* The rest of this test crashes on WS03, Vista, Win7, and Win8.1 */
    if (GetNTVersion() < _WIN32_WINNT_WIN10) {
        free(user);
        return;
    }
#endif
    SetLastError(0xdeadbeef);
    error = GetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
        NULL, NULL, NULL, NULL, &pSD);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetNamedSecurityInfoA is not implemented\n");
        free(user);
        return;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);

    bret = GetSecurityDescriptorControl(pSD, &control, &revision);
    ok(bret, "GetSecurityDescriptorControl failed with error %ld\n", GetLastError());
    ok((control & (SE_SELF_RELATIVE|SE_DACL_PRESENT)) == (SE_SELF_RELATIVE|SE_DACL_PRESENT),
        "control (0x%x) doesn't have (SE_SELF_RELATIVE|SE_DACL_PRESENT) flags set\n", control);
    ok(revision == SECURITY_DESCRIPTOR_REVISION1, "revision was %ld instead of 1\n", revision);

    bret = GetSecurityDescriptorOwner(pSD, &owner, &owner_defaulted);
    ok(bret, "GetSecurityDescriptorOwner failed with error %ld\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");

    bret = GetSecurityDescriptorGroup(pSD, &group, &group_defaulted);
    ok(bret, "GetSecurityDescriptorGroup failed with error %ld\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
    LocalFree(pSD);


    /* NULL descriptor tests */

    error = GetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,
        NULL, NULL, NULL, NULL, NULL);
    ok(error==ERROR_INVALID_PARAMETER, "GetNamedSecurityInfo failed with error %ld\n", error);

    pDacl = NULL;
    error = GetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,
        NULL, NULL, &pDacl, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);
    ok(pDacl != NULL, "DACL should not be NULL\n");
    LocalFree(pSD);

    error = GetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,
        NULL, NULL, &pDacl, NULL, NULL);
    ok(error==ERROR_INVALID_PARAMETER, "GetNamedSecurityInfo failed with error %ld\n", error);

    /* Test behavior of SetNamedSecurityInfo with an invalid path */
    SetLastError(0xdeadbeef);
    error = SetNamedSecurityInfoA(invalid_path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                                  NULL, NULL, NULL);
    ok(error == ERROR_FILE_NOT_FOUND, "Unexpected error returned: 0x%lx\n", error);
    ok(GetLastError() == 0xdeadbeef, "Expected last error to remain unchanged.\n");

    /* Create security descriptor information and test that it comes back the same */
    pSD = &sd;
    pDacl = malloc(100);
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, admin_sid);
    ok(bret, "Failed to add Administrator Group to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor.\n");
    GetTempFileNameA(".", "foo", 0, tmpfile);
    hTemp = CreateFileA(tmpfile, WRITE_DAC|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    SetLastError(0xdeadbeef);
    error = SetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                                  NULL, pDacl, NULL);
    free(pDacl);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("SetNamedSecurityInfoA is not implemented\n");
        free(user);
        CloseHandle(hTemp);
        return;
    }
    ok(!error, "SetNamedSecurityInfoA failed with error %ld\n", error);
    SetLastError(0xdeadbeef);
    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                   NULL, NULL, &pDacl, NULL, &pSD);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetNamedSecurityInfoA is not implemented\n");
        free(user);
        CloseHandle(hTemp);
        return;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);

    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    if (acl_size.AceCount > 0)
    {
        bret = GetAce(pDacl, 0, (VOID **)&ace);
        ok(bret, "Failed to get Current User ACE.\n");
        bret = EqualSid(&ace->SidStart, user_sid);
        todo_wine ok(bret, "Current User ACE (%s) != Current User SID (%s).\n",
                     debugstr_sid(&ace->SidStart), debugstr_sid(user_sid));
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Current User ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff, "Current User ACE has unexpected mask (0x%lx != 0x1f01ff)\n",
                                  ace->Mask);
    }
    if (acl_size.AceCount > 1)
    {
        bret = GetAce(pDacl, 1, (VOID **)&ace);
        ok(bret, "Failed to get Administators Group ACE.\n");
        bret = EqualSid(&ace->SidStart, admin_sid);
        todo_wine ok(bret || broken(!bret) /* win2k */,
                     "Administators Group ACE (%s) != Administators Group SID (%s).\n",
                     debugstr_sid(&ace->SidStart), debugstr_sid(admin_sid));
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Administators Group ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff || broken(ace->Mask == GENERIC_ALL) /* win2k */,
           "Administators Group ACE has unexpected mask (0x%lx != 0x1f01ff)\n", ace->Mask);
    }
    LocalFree(pSD);

    /* show that setting empty DACL is not removing all file permissions */
    pDacl = malloc(sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    error = SetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
            NULL, NULL, pDacl, NULL);
    ok(!error, "SetNamedSecurityInfoA failed with error %ld\n", error);
    free(pDacl);

    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
            NULL, NULL, &pDacl, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);

    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    if (acl_size.AceCount > 0)
    {
        bret = GetAce(pDacl, 0, (VOID **)&ace);
        ok(bret, "Failed to get ACE.\n");
        todo_wine ok(((ACE_HEADER *)ace)->AceFlags & INHERITED_ACE,
                "ACE has unexpected flags: 0x%x\n", ((ACE_HEADER *)ace)->AceFlags);
    }
    LocalFree(pSD);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    CloseHandle(h);

    /* test setting NULL DACL */
    error = SetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL);
    ok(!error, "SetNamedSecurityInfoA failed with error %ld\n", error);

    error = GetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                NULL, NULL, &pDacl, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);
    todo_wine ok(!pDacl, "pDacl != NULL\n");
    LocalFree(pSD);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    CloseHandle(h);

    /* NtSetSecurityObject doesn't inherit DACL entries */
    pSD = sd+sizeof(void*)-((ULONG_PTR)sd)%sizeof(void*);
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = malloc(100);
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor.\n");
    status = NtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %lx\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    CloseHandle(h);

    SetSecurityDescriptorControl(pSD, SE_DACL_AUTO_INHERIT_REQ, SE_DACL_AUTO_INHERIT_REQ);
    status = NtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %lx\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    CloseHandle(h);

    SetSecurityDescriptorControl(pSD, SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED,
            SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED);
    status = NtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %lx\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    CloseHandle(h);

    /* test if DACL is properly mapped to permission */
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = AddAccessDeniedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor.\n");
    status = NtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %lx\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    CloseHandle(h);

    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = AddAccessDeniedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor.\n");
    status = NtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %lx\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    free(pDacl);
    free(user);
    CloseHandle(hTemp);

    /* Test querying the ownership of a built-in registry key */
    sid_size = sizeof(system_ptr);
    CreateWellKnownSid(WinLocalSystemSid, NULL, system_sid, &sid_size);
    error = GetNamedSecurityInfoA(software_key, SE_REGISTRY_KEY,
                                   OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,
                                   NULL, NULL, NULL, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);

    bret = AllocateAndInitializeSid(&SIDAuthNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &localsys_sid);
    ok(bret, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    bret = GetSecurityDescriptorOwner(pSD, &owner, &owner_defaulted);
    ok(bret, "GetSecurityDescriptorOwner failed with error %ld\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");
    ok(EqualSid(owner, admin_sid) || EqualSid(owner, localsys_sid),
                "MACHINE\\Software owner SID (%s) != Administrators SID (%s) or Local System Sid (%s).\n",
                debugstr_sid(owner), debugstr_sid(admin_sid), debugstr_sid(localsys_sid));

    bret = GetSecurityDescriptorGroup(pSD, &group, &group_defaulted);
    ok(bret, "GetSecurityDescriptorGroup failed with error %ld\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
    ok(EqualSid(group, admin_sid) || broken(EqualSid(group, system_sid)) /* before Win7 */
       || broken(((SID*)group)->SubAuthority[0] == SECURITY_NT_NON_UNIQUE) /* Vista */,
       "MACHINE\\Software group SID (%s) != Local System SID (%s or %s)\n",
       debugstr_sid(group), debugstr_sid(admin_sid), debugstr_sid(system_sid));
    LocalFree(pSD);

    /* Test querying the DACL of a built-in registry key */
    sid_size = sizeof(users_ptr);
    CreateWellKnownSid(WinBuiltinUsersSid, NULL, users_sid, &sid_size);
    error = GetNamedSecurityInfoA(software_key, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
                                   NULL, NULL, NULL, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %ld\n", error);

    bret = GetSecurityDescriptorDacl(pSD, &dacl_present, &pDacl, &dacl_defaulted);
    ok(bret, "GetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    ok(dacl_present, "DACL should be present\n");
    ok(pDacl && IsValidAcl(pDacl), "GetSecurityDescriptorDacl returned invalid DACL.\n");
    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount != 0, "GetAclInformation returned no ACLs\n");
    for (i=0; i<acl_size.AceCount; i++)
    {
        bret = GetAce(pDacl, i, (VOID **)&ace);
        ok(bret, "Failed to get ACE %d.\n", i);
        bret = EqualSid(&ace->SidStart, users_sid);
        if (bret) users_ace_id = i;
        bret = EqualSid(&ace->SidStart, admin_sid);
        if (bret) admins_ace_id = i;
    }
    ok(users_ace_id != -1 || broken(users_ace_id == -1) /* win2k */,
       "Builtin Users ACE not found.\n");
    if (users_ace_id != -1)
    {
        bret = GetAce(pDacl, users_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Builtin Users ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE)
           || broken(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE|INHERITED_ACE)) /* w2k8 */
           || broken(flags == (CONTAINER_INHERIT_ACE|INHERITED_ACE)) /* win 10 wow64 */
           || broken(flags == CONTAINER_INHERIT_ACE), /* win 10 */
           "Builtin Users ACE has unexpected flags (0x%x != 0x%x)\n", flags,
           INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE);
        ok(ace->Mask == GENERIC_READ
           || broken(ace->Mask == KEY_READ), /* win 10 */
           "Builtin Users ACE has unexpected mask (0x%lx != 0x%x)\n",
                                      ace->Mask, GENERIC_READ);
    }
    ok(admins_ace_id != -1, "Builtin Admins ACE not found.\n");
    if (admins_ace_id != -1)
    {
        bret = GetAce(pDacl, admins_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Builtin Admins ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == 0x0
           || broken(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE|INHERITED_ACE)) /* w2k8 */
           || broken(flags == (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE)) /* win7 */
           || broken(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE)) /* win8+ */
           || broken(flags == (CONTAINER_INHERIT_ACE|INHERITED_ACE)) /* win 10 wow64 */
           || broken(flags == CONTAINER_INHERIT_ACE), /* win 10 */
           "Builtin Admins ACE has unexpected flags (0x%x != 0x0)\n", flags);
        ok(ace->Mask == KEY_ALL_ACCESS || broken(ace->Mask == GENERIC_ALL) /* w2k8 */,
           "Builtin Admins ACE has unexpected mask (0x%lx != 0x%x)\n", ace->Mask, KEY_ALL_ACCESS);
    }

    FreeSid(localsys_sid);
    LocalFree(pSD);
}

static void test_ConvertStringSecurityDescriptor(void)
{
    BOOL ret;
    PSECURITY_DESCRIPTOR pSD;
    static const WCHAR Blank[] = { 0 };
    unsigned int i;
    ULONG size;
    ACL *acl;
    static const struct
    {
        const char *sidstring;
        DWORD      revision;
        BOOL       ret;
        DWORD      GLE;
        DWORD      altGLE;
        DWORD      ace_Mask;
    } cssd[] =
    {
        { "D:(A;;GA;;;WD)",                  0xdeadbeef,      FALSE, ERROR_UNKNOWN_REVISION },
        /* test ACE string type */
        { "D:(A;;GA;;;WD)",                  SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;;WD)",                  SDDL_REVISION_1, TRUE },
        { "ERROR:(D;;GA;;;WD)",              SDDL_REVISION_1, FALSE, ERROR_INVALID_PARAMETER },
        /* test ACE string with spaces */
        { " D:(D;;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D: (D;;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:( D;;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D ;;GA;;;WD)",                SDDL_REVISION_1, FALSE, RPC_S_INVALID_STRING_UUID, ERROR_INVALID_ACL }, /* Vista+ */
        { "D:(D; ;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;; GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA ;;;WD)",                SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL },
        { "D:(D;;GA; ;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;; ;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;; WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;;WD )",                SDDL_REVISION_1, TRUE },
        /* test ACE string access rights */
        { "D:(A;;GA;;;WD)",                  SDDL_REVISION_1, TRUE, 0, 0, GENERIC_ALL },
        { "D:(A;;1;;;WD)",                   SDDL_REVISION_1, TRUE, 0, 0, 1 },
        { "D:(A;;020000000000;;;WD)",        SDDL_REVISION_1, TRUE, 0, 0, GENERIC_READ },
        { "D:(A;;0X40000000;;;WD)",          SDDL_REVISION_1, TRUE, 0, 0, GENERIC_WRITE },
        { "D:(A;;GRGWGX;;;WD)",              SDDL_REVISION_1, TRUE, 0, 0, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE },
        { "D:(A;;RCSDWDWO;;;WD)",            SDDL_REVISION_1, TRUE, 0, 0, READ_CONTROL | DELETE | WRITE_DAC | WRITE_OWNER },
        { "D:(A;;RPWPCCDCLCSWLODTCR;;;WD)",  SDDL_REVISION_1, TRUE },
        { "D:(A;;FAFRFWFX;;;WD)",            SDDL_REVISION_1, TRUE },
        { "D:(A;;KAKRKWKX;;;WD)",            SDDL_REVISION_1, TRUE },
        { "D:(A;;0xFFFFFFFF;;;WD)",          SDDL_REVISION_1, TRUE },
        { "S:(AU;;0xFFFFFFFF;;;WD)",         SDDL_REVISION_1, TRUE },
        { "S:(AU;;0xDeAdBeEf;;;WD)",         SDDL_REVISION_1, TRUE },
        { "S:(AU;;GR0xFFFFFFFF;;;WD)",       SDDL_REVISION_1, TRUE },
        { "S:(AU;;0xFFFFFFFFGR;;;WD)",       SDDL_REVISION_1, TRUE },
        { "S:(AU;;0xFFFFFGR;;;WD)",          SDDL_REVISION_1, TRUE },
        /* test ACE string access right error case */
        { "D:(A;;ROB;;;WD)",                 SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL },
        /* test behaviour with empty strings */
        { "",                                SDDL_REVISION_1, TRUE },
        /* test ACE string SID */
        { "D:(D;;GA;;;S-1-0-0)",             SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;;WDANDSUCH)",           SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL },
        { "D:(D;;GA;;;Nonexistent account)", SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL, ERROR_INVALID_SID }, /* W2K */
    };

    for (i = 0; i < ARRAY_SIZE(cssd); i++)
    {
        DWORD GLE;

        SetLastError(0xdeadbeef);
        ret = ConvertStringSecurityDescriptorToSecurityDescriptorA(
            cssd[i].sidstring, cssd[i].revision, &pSD, NULL);
        GLE = GetLastError();
        ok(ret == cssd[i].ret, "(%02u) Expected %s (%ld)\n", i, cssd[i].ret ? "success" : "failure", GLE);
        if (!cssd[i].ret)
            ok(GLE == cssd[i].GLE ||
               (cssd[i].altGLE && GLE == cssd[i].altGLE),
               "(%02u) Unexpected last error %ld\n", i, GLE);
        if (ret)
        {
            if (cssd[i].ace_Mask)
            {
                ACCESS_ALLOWED_ACE *ace;

                acl = (ACL *)((char *)pSD + sizeof(SECURITY_DESCRIPTOR_RELATIVE));
                ok(acl->AclRevision == ACL_REVISION, "(%02u) Got %u\n", i, acl->AclRevision);

                ace = (ACCESS_ALLOWED_ACE *)(acl + 1);
                ok(ace->Mask == cssd[i].ace_Mask, "(%02u) Expected %08lx, got %08lx\n",
                   i, cssd[i].ace_Mask, ace->Mask);
            }
            LocalFree(pSD);
        }
    }

    /* test behaviour with NULL parameters */
    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA(
        NULL, 0xdeadbeef, &pSD, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        NULL, 0xdeadbeef, &pSD, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;ROB;;;WD)", 0xdeadbeef, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;ROB;;;WD)", SDDL_REVISION_1, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %ld\n",
        GetLastError());

    /* test behaviour with empty strings */
    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        Blank, SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %ld\n", GetLastError());
    LocalFree(pSD);

    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:P(A;;GRGW;;;BA)(A;;GRGW;;;S-1-5-21-0-0-0-1000)S:(ML;;NWNR;;;S-1-16-12288)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret || broken(!ret && GetLastError() == ERROR_INVALID_DATATYPE) /* win2k */,
       "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %lu\n", GetLastError());
    if (ret) LocalFree(pSD);

    /* empty DACL */
    size = 0;
    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA("D:", SDDL_REVISION_1, &pSD, &size);
    ok(ret, "unexpected error %lu\n", GetLastError());
    ok(size == sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sizeof(ACL), "got %lu\n", size);
    acl = (ACL *)((char *)pSD + sizeof(SECURITY_DESCRIPTOR_RELATIVE));
    ok(acl->AclRevision == ACL_REVISION, "got %u\n", acl->AclRevision);
    ok(!acl->Sbz1, "got %u\n", acl->Sbz1);
    ok(acl->AclSize == sizeof(*acl), "got %u\n", acl->AclSize);
    ok(!acl->AceCount, "got %u\n", acl->AceCount);
    ok(!acl->Sbz2, "got %u\n", acl->Sbz2);
    LocalFree(pSD);

    /* empty SACL */
    size = 0;
    SetLastError(0xdeadbeef);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorA("S:", SDDL_REVISION_1, &pSD, &size);
    ok(ret, "unexpected error %lu\n", GetLastError());
    ok(size == sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sizeof(ACL), "got %lu\n", size);
    acl = (ACL *)((char *)pSD + sizeof(SECURITY_DESCRIPTOR_RELATIVE));
    ok(!acl->Sbz1, "got %u\n", acl->Sbz1);
    ok(acl->AclSize == sizeof(*acl), "got %u\n", acl->AclSize);
    ok(!acl->AceCount, "got %u\n", acl->AceCount);
    ok(!acl->Sbz2, "got %u\n", acl->Sbz2);
    LocalFree(pSD);
}

static void test_ConvertSecurityDescriptorToString(void)
{
    SECURITY_DESCRIPTOR desc;
    SECURITY_INFORMATION sec_info = OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION;
    LPSTR string;
    DWORD size;
    PSID psid, psid2;
    PACL pacl;
    char sid_buf[256];
    char acl_buf[8192];
    ULONG len;

/* It seems Windows XP adds an extra character to the length of the string for each ACE in an ACL. We
 * don't replicate this feature so we only test len >= strlen+1. */
#define CHECK_RESULT_AND_FREE(exp_str) \
    ok(strcmp(string, (exp_str)) == 0, "String mismatch (expected \"%s\", got \"%s\")\n", (exp_str), string); \
    ok(len >= (strlen(exp_str) + 1), "Length mismatch (expected %d, got %ld)\n", lstrlenA(exp_str) + 1, len); \
    LocalFree(string);

#define CHECK_ONE_OF_AND_FREE(exp_str1, exp_str2) \
    ok(strcmp(string, (exp_str1)) == 0 || strcmp(string, (exp_str2)) == 0, "String mismatch (expected\n\"%s\" or\n\"%s\", got\n\"%s\")\n", (exp_str1), (exp_str2), string); \
    ok(len >= (strlen(exp_str1) + 1) || len >= (strlen(exp_str2) + 1), "Length mismatch (expected %d or %d, got %ld)\n", lstrlenA(exp_str1) + 1, lstrlenA(exp_str2) + 1, len); \
    LocalFree(string);

    InitializeSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("");

    size = 4096;
    CreateWellKnownSid(WinLocalSid, NULL, sid_buf, &size);
    SetSecurityDescriptorOwner(&desc, sid_buf, FALSE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:S-1-2-0");

    SetSecurityDescriptorOwner(&desc, sid_buf, TRUE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:S-1-2-0");

    size = sizeof(sid_buf);
    CreateWellKnownSid(WinLocalSystemSid, NULL, sid_buf, &size);
    SetSecurityDescriptorOwner(&desc, sid_buf, TRUE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SY");

    ConvertStringSidToSidA("S-1-5-21-93476-23408-4576", &psid);
    SetSecurityDescriptorGroup(&desc, psid, TRUE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576");

    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, GROUP_SECURITY_INFORMATION, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("G:S-1-5-21-93476-23408-4576");

    pacl = (PACL)acl_buf;
    InitializeAcl(pacl, sizeof(acl_buf), ACL_REVISION);
    SetSecurityDescriptorDacl(&desc, TRUE, pacl, TRUE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:");

    SetSecurityDescriptorDacl(&desc, TRUE, pacl, FALSE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:");

    ConvertStringSidToSidA("S-1-5-6", &psid2);
    AddAccessAllowedAceEx(pacl, ACL_REVISION, NO_PROPAGATE_INHERIT_ACE, 0xf0000000, psid2);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)");

    AddAccessAllowedAceEx(pacl, ACL_REVISION, INHERIT_ONLY_ACE|INHERITED_ACE, 0x00000003, psid2);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)");

    AddAccessDeniedAceEx(pacl, ACL_REVISION, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE, 0xffffffff, psid);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)");


    pacl = (PACL)acl_buf;
    InitializeAcl(pacl, sizeof(acl_buf), ACL_REVISION);
    SetSecurityDescriptorSacl(&desc, TRUE, pacl, FALSE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:S:");

    /* fails in win2k */
    SetSecurityDescriptorDacl(&desc, TRUE, NULL, FALSE);
    AddAuditAccessAceEx(pacl, ACL_REVISION, VALID_INHERIT_FLAGS, KEY_READ|KEY_WRITE, psid2, TRUE, TRUE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_ONE_OF_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)", /* XP */
        "O:SYG:S-1-5-21-93476-23408-4576D:NO_ACCESS_CONTROLS:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)" /* Vista */);

    /* fails in win2k */
    AddAuditAccessAceEx(pacl, ACL_REVISION, NO_PROPAGATE_INHERIT_ACE, FILE_GENERIC_READ|FILE_GENERIC_WRITE, psid2, TRUE, FALSE);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_ONE_OF_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)", /* XP */
        "O:SYG:S-1-5-21-93476-23408-4576D:NO_ACCESS_CONTROLS:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)" /* Vista */);

    LocalFree(psid2);
    LocalFree(psid);
}

static void test_SetSecurityDescriptorControl (PSECURITY_DESCRIPTOR sec)
{
    SECURITY_DESCRIPTOR_CONTROL ref;
    SECURITY_DESCRIPTOR_CONTROL test;

    SECURITY_DESCRIPTOR_CONTROL const mutable
        = SE_DACL_AUTO_INHERIT_REQ | SE_SACL_AUTO_INHERIT_REQ
        | SE_DACL_AUTO_INHERITED   | SE_SACL_AUTO_INHERITED
        | SE_DACL_PROTECTED        | SE_SACL_PROTECTED
        | 0x00000040               | 0x00000080        /* not defined in winnt.h */
        ;
    SECURITY_DESCRIPTOR_CONTROL const immutable
        = SE_OWNER_DEFAULTED       | SE_GROUP_DEFAULTED
        | SE_DACL_PRESENT          | SE_DACL_DEFAULTED
        | SE_SACL_PRESENT          | SE_SACL_DEFAULTED
        | SE_RM_CONTROL_VALID      | SE_SELF_RELATIVE
        ;

    int     bit;
    DWORD   dwRevision;
    LPCSTR  fmt = "Expected error %s, got %u\n";

    GetSecurityDescriptorControl (sec, &ref, &dwRevision);

    /* The mutable bits are mutable regardless of the truth of
       SE_DACL_PRESENT and/or SE_SACL_PRESENT */

    /* Check call barfs if any bit-of-interest is immutable */
    for (bit = 0; bit < 16; ++bit)
    {
        SECURITY_DESCRIPTOR_CONTROL const bitOfInterest = 1 << bit;
        SECURITY_DESCRIPTOR_CONTROL setOrClear = ref & bitOfInterest;

        SECURITY_DESCRIPTOR_CONTROL ctrl;

        DWORD   dwExpect  = (bitOfInterest & immutable)
                          ?  ERROR_INVALID_PARAMETER  :  0xbebecaca;
        LPCSTR  strExpect = (bitOfInterest & immutable)
                          ? "ERROR_INVALID_PARAMETER" : "0xbebecaca";

        ctrl = (bitOfInterest & mutable) ? ref + bitOfInterest : ref;
        setOrClear ^= bitOfInterest;
        SetLastError (0xbebecaca);
        SetSecurityDescriptorControl (sec, bitOfInterest, setOrClear);
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl(sec, &test, &dwRevision);
        expect_eq(test, ctrl, int, "%x");

        setOrClear ^= bitOfInterest;
        SetLastError (0xbebecaca);
        SetSecurityDescriptorControl (sec, bitOfInterest, setOrClear);
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl (sec, &test, &dwRevision);
        expect_eq(test, ref, int, "%x");
    }

    /* Check call barfs if any bit-to-set is immutable
       even when not a bit-of-interest */
    for (bit = 0; bit < 16; ++bit)
    {
        SECURITY_DESCRIPTOR_CONTROL const bitsOfInterest = mutable;
        SECURITY_DESCRIPTOR_CONTROL setOrClear = ref & bitsOfInterest;

        SECURITY_DESCRIPTOR_CONTROL ctrl;

        DWORD   dwExpect  = ((1 << bit) & immutable)
                          ?  ERROR_INVALID_PARAMETER  :  0xbebecaca;
        LPCSTR  strExpect = ((1 << bit) & immutable)
                          ? "ERROR_INVALID_PARAMETER" : "0xbebecaca";

        ctrl = ((1 << bit) & immutable) ? test : ref | mutable;
        setOrClear ^= bitsOfInterest;
        SetLastError (0xbebecaca);
        SetSecurityDescriptorControl (sec, bitsOfInterest, setOrClear | (1 << bit));
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl(sec, &test, &dwRevision);
        expect_eq(test, ctrl, int, "%x");

        ctrl = ((1 << bit) & immutable) ? test : ref | (1 << bit);
        setOrClear ^= bitsOfInterest;
        SetLastError (0xbebecaca);
        SetSecurityDescriptorControl (sec, bitsOfInterest, setOrClear | (1 << bit));
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl(sec, &test, &dwRevision);
        expect_eq(test, ctrl, int, "%x");
    }
}

static void test_PrivateObjectSecurity(void)
{
    SECURITY_INFORMATION sec_info = OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION;
    SECURITY_DESCRIPTOR_CONTROL ctrl;
    PSECURITY_DESCRIPTOR sec;
    DWORD dwDescSize;
    DWORD dwRevision;
    DWORD retSize;
    LPSTR string;
    ULONG len;
    PSECURITY_DESCRIPTOR buf;
    BOOL ret;

    ok(ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)"
          "(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)",
        SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");

    test_SetSecurityDescriptorControl(sec);

    LocalFree(sec);

    ok(ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576",
        SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");

    test_SetSecurityDescriptorControl(sec);

    LocalFree(sec);

    ok(ConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)", SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");
    buf = malloc(dwDescSize);
    SetSecurityDescriptorControl(sec, SE_DACL_PROTECTED, SE_DACL_PROTECTED);
    GetSecurityDescriptorControl(sec, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x9014, int, "%x");

    ret = GetPrivateObjectSecurity(sec, GROUP_SECURITY_INFORMATION, buf, dwDescSize, &retSize);
    ok(ret, "GetPrivateObjectSecurity failed (err=%lu)\n", GetLastError());
    ok(retSize <= dwDescSize, "Buffer too small (%ld vs %ld)\n", retSize, dwDescSize);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("G:S-1-5-21-93476-23408-4576");
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x8000, int, "%x");

    ret = GetPrivateObjectSecurity(sec, GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, buf, dwDescSize, &retSize);
    ok(ret, "GetPrivateObjectSecurity failed (err=%lu)\n", GetLastError());
    ok(retSize <= dwDescSize, "Buffer too small (%ld vs %ld)\n", retSize, dwDescSize);
    ret = ConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len);
    ok(ret, "Conversion failed err=%lu\n", GetLastError());
    CHECK_ONE_OF_AND_FREE("G:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)",
        "G:S-1-5-21-93476-23408-4576D:P(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"); /* Win7 */
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl & (~ SE_DACL_PROTECTED), 0x8004, int, "%x");

    ret = GetPrivateObjectSecurity(sec, sec_info, buf, dwDescSize, &retSize);
    ok(ret, "GetPrivateObjectSecurity failed (err=%lu)\n", GetLastError());
    ok(retSize == dwDescSize, "Buffer too small (%ld vs %ld)\n", retSize, dwDescSize);
    ok(ConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_ONE_OF_AND_FREE("O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)",
      "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:P(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)"); /* Win7 */
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl & (~ SE_DACL_PROTECTED), 0x8014, int, "%x");

    SetLastError(0xdeadbeef);
    ok(GetPrivateObjectSecurity(sec, sec_info, buf, 5, &retSize) == FALSE, "GetPrivateObjectSecurity should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected error ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    LocalFree(sec);
    free(buf);
}
#undef CHECK_RESULT_AND_FREE
#undef CHECK_ONE_OF_AND_FREE

static void test_InitializeAcl(void)
{
    char buffer[256];
    PACL pAcl = (PACL)buffer;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(ACL) - 1, ACL_REVISION);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("InitializeAcl is not implemented\n");
        return;
    }

    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "InitializeAcl with too small a buffer should have failed with ERROR_INSUFFICIENT_BUFFER instead of %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, 0xffffffff, ACL_REVISION);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl with too large a buffer should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION1);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl(ACL_REVISION1) should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", GetLastError());

    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION2);
    ok(ret, "InitializeAcl(ACL_REVISION2) failed with error %ld\n", GetLastError());

    ret = IsValidAcl(pAcl);
    ok(ret, "IsValidAcl failed with error %ld\n", GetLastError());

    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION3);
    ok(ret, "InitializeAcl(ACL_REVISION3) failed with error %ld\n", GetLastError());

    ret = IsValidAcl(pAcl);
    ok(ret, "IsValidAcl failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION4);
    ok(ret, "InitializeAcl(ACL_REVISION4) failed with error %ld\n", GetLastError());

    ret = IsValidAcl(pAcl);
    ok(ret, "IsValidAcl failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), -1);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl(-1) failed with error %ld\n", GetLastError());
}

static void test_GetSecurityInfo(void)
{
    char domain_users_ptr[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
    char b[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
    char admin_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES], dacl[100];
    PSID domain_users_sid = (PSID) domain_users_ptr, domain_sid;
    SID_IDENTIFIER_AUTHORITY sia = { SECURITY_NT_AUTHORITY };
    int domain_users_ace_id = -1, admins_ace_id = -1, i;
    DWORD sid_size = sizeof(admin_ptr), l = sizeof(b);
    SECURITY_ATTRIBUTES sa = {.nLength = sizeof(sa)};
    PSID admin_sid = (PSID) admin_ptr, user_sid;
    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    BOOL owner_defaulted, group_defaulted;
    BOOL dacl_defaulted, dacl_present;
    ACL_SIZE_INFORMATION acl_size;
    PSECURITY_DESCRIPTOR pSD;
    ACCESS_ALLOWED_ACE *ace;
    HANDLE token, obj;
    PSID owner, group;
    BOOL bret = TRUE;
    PACL pDacl;
    BYTE flags;
    DWORD ret;

    static const SE_OBJECT_TYPE kernel_types[] =
    {
        SE_FILE_OBJECT,
        SE_KERNEL_OBJECT,
        SE_WMIGUID_OBJECT,
    };

    static const SE_OBJECT_TYPE invalid_types[] =
    {
        SE_UNKNOWN_OBJECT_TYPE,
        SE_DS_OBJECT,
        SE_DS_OBJECT_ALL,
        SE_PROVIDER_DEFINED_OBJECT,
        SE_REGISTRY_WOW64_32KEY,
        SE_REGISTRY_WOW64_64KEY,
        0xdeadbeef,
    };

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }
    bret = GetTokenInformation(token, TokenUser, b, l, &l);
    ok(bret, "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    CloseHandle( token );
    user_sid = ((TOKEN_USER *)b)->User.Sid;

    /* Create something.  Files have lots of associated security info.  */
    obj = CreateFileA(myARGV[0], GENERIC_READ|WRITE_DAC, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (obj == INVALID_HANDLE_VALUE)
    {
        skip("Couldn't create an object for GetSecurityInfo test\n");
        return;
    }

    ret = GetSecurityInfo(obj, SE_FILE_OBJECT,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, &group, &pDacl, NULL, &pSD);
    if (ret == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetSecurityInfo is not implemented\n");
        CloseHandle(obj);
        return;
    }
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %ld\n", ret);
    ok(pSD != NULL, "GetSecurityInfo\n");
    ok(owner != NULL, "GetSecurityInfo\n");
    ok(group != NULL, "GetSecurityInfo\n");
    if (pDacl != NULL)
        ok(IsValidAcl(pDacl), "GetSecurityInfo\n");
    else
        win_skip("No ACL information returned\n");

    LocalFree(pSD);

    /* If we don't ask for the security descriptor, Windows will still give us
       the other stuff, leaving us no way to free it.  */
    ret = GetSecurityInfo(obj, SE_FILE_OBJECT,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, &group, &pDacl, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %ld\n", ret);
    ok(owner != NULL, "GetSecurityInfo\n");
    ok(group != NULL, "GetSecurityInfo\n");
    if (pDacl != NULL)
        ok(IsValidAcl(pDacl), "GetSecurityInfo\n");
    else
        win_skip("No ACL information returned\n");

    /* Create security descriptor information and test that it comes back the same */
    pSD = &sd;
    pDacl = (PACL)&dacl;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    bret = InitializeAcl(pDacl, sizeof(dacl), ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = AddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, admin_sid);
    ok(bret, "Failed to add Administrator Group to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security descriptor.\n");
    ret = SetSecurityInfo(obj, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                         NULL, NULL, pDacl, NULL);
    ok(ret == ERROR_SUCCESS, "SetSecurityInfo returned %ld\n", ret);
    ret = GetSecurityInfo(obj, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                          NULL, NULL, &pDacl, NULL, &pSD);
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %ld\n", ret);
    ok(pDacl && IsValidAcl(pDacl), "GetSecurityInfo returned invalid DACL.\n");
    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    if (acl_size.AceCount > 0)
    {
        bret = GetAce(pDacl, 0, (VOID **)&ace);
        ok(bret, "Failed to get Current User ACE.\n");
#ifndef __REACTOS__ // This crashes on WS03, Vista, Win7, and Win8.1.
        bret = EqualSid(&ace->SidStart, user_sid);
        todo_wine ok(bret, "Current User ACE (%s) != Current User SID (%s).\n",
                     debugstr_sid(&ace->SidStart), debugstr_sid(user_sid));
#endif
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Current User ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff, "Current User ACE has unexpected mask (0x%lx != 0x1f01ff)\n",
                                    ace->Mask);
    }
    if (acl_size.AceCount > 1)
    {
        bret = GetAce(pDacl, 1, (VOID **)&ace);
        ok(bret, "Failed to get Administators Group ACE.\n");
#ifndef __REACTOS__ // This crashes on WS03, Vista, Win7, and Win8.1.
        bret = EqualSid(&ace->SidStart, admin_sid);
        todo_wine ok(bret, "Administators Group ACE (%s) != Administators Group SID (%s).\n", debugstr_sid(&ace->SidStart), debugstr_sid(admin_sid));
#endif
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Administators Group ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff, "Administators Group ACE has unexpected mask (0x%lx != 0x1f01ff)\n",
                                  ace->Mask);
    }
    LocalFree(pSD);
    CloseHandle(obj);

    /* Obtain the "domain users" SID from the user SID */
    if (!AllocateAndInitializeSid(&sia, 4, *GetSidSubAuthority(user_sid, 0),
                                  *GetSidSubAuthority(user_sid, 1),
                                  *GetSidSubAuthority(user_sid, 2),
                                  *GetSidSubAuthority(user_sid, 3), 0, 0, 0, 0, &domain_sid))
    {
        win_skip("Failed to get current domain SID\n");
        return;
    }
    sid_size = sizeof(domain_users_ptr);
    CreateWellKnownSid(WinAccountDomainUsersSid, domain_sid, domain_users_sid, &sid_size);
    FreeSid(domain_sid);

    /* Test querying the ownership of a process */
    ret = GetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT,
                           OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,
                           NULL, NULL, NULL, NULL, &pSD);
    ok(!ret, "GetNamedSecurityInfo failed with error %ld\n", ret);

    bret = GetSecurityDescriptorOwner(pSD, &owner, &owner_defaulted);
    ok(bret, "GetSecurityDescriptorOwner failed with error %ld\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");
    ok(EqualSid(owner, admin_sid) || EqualSid(owner, user_sid),
       "Process owner SID != Administrators SID.\n");

    bret = GetSecurityDescriptorGroup(pSD, &group, &group_defaulted);
    ok(bret, "GetSecurityDescriptorGroup failed with error %ld\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
    ok(EqualSid(group, domain_users_sid), "Process group SID != Domain Users SID.\n");
    LocalFree(pSD);

    /* Test querying the DACL of a process */
    ret = GetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                                   NULL, NULL, NULL, NULL, &pSD);
    ok(!ret, "GetSecurityInfo failed with error %ld\n", ret);

    bret = GetSecurityDescriptorDacl(pSD, &dacl_present, &pDacl, &dacl_defaulted);
    ok(bret, "GetSecurityDescriptorDacl failed with error %ld\n", GetLastError());
    ok(dacl_present, "DACL should be present\n");
    ok(pDacl && IsValidAcl(pDacl), "GetSecurityDescriptorDacl returned invalid DACL.\n");
    bret = GetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount != 0, "GetAclInformation returned no ACLs\n");
    for (i=0; i<acl_size.AceCount; i++)
    {
        bret = GetAce(pDacl, i, (VOID **)&ace);
        ok(bret, "Failed to get ACE %d.\n", i);
        bret = EqualSid(&ace->SidStart, domain_users_sid);
        if (bret) domain_users_ace_id = i;
        bret = EqualSid(&ace->SidStart, admin_sid);
        if (bret) admins_ace_id = i;
    }
    ok(domain_users_ace_id != -1 || broken(domain_users_ace_id == -1) /* win2k */,
       "Domain Users ACE not found.\n");
    if (domain_users_ace_id != -1)
    {
        bret = GetAce(pDacl, domain_users_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Domain Users ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE),
           "Domain Users ACE has unexpected flags (0x%x != 0x%x)\n", flags,
           INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE);
        ok(ace->Mask == GENERIC_READ, "Domain Users ACE has unexpected mask (0x%lx != 0x%x)\n",
                                      ace->Mask, GENERIC_READ);
    }
    ok(admins_ace_id != -1 || broken(admins_ace_id == -1) /* xp */,
       "Builtin Admins ACE not found.\n");
    if (admins_ace_id != -1)
    {
        bret = GetAce(pDacl, admins_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Builtin Admins ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == 0x0, "Builtin Admins ACE has unexpected flags (0x%x != 0x0)\n", flags);
        ok(ace->Mask == PROCESS_ALL_ACCESS || broken(ace->Mask == 0x1f0fff) /* win2k */,
           "Builtin Admins ACE has unexpected mask (0x%lx != 0x%x)\n", ace->Mask, PROCESS_ALL_ACCESS);
    }
    LocalFree(pSD);

    ret = GetSecurityInfo(NULL, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
    ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

    ret = GetSecurityInfo(GetCurrentProcess(), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
    ok(!ret, "got error %lu\n", ret);
    LocalFree(pSD);

    sa.lpSecurityDescriptor = sd;
    obj = CreateEventA(&sa, TRUE, TRUE, NULL);
    pDacl = (PACL)&dacl;

    for (size_t i = 0; i < ARRAY_SIZE(kernel_types); ++i)
    {
        winetest_push_context("Type %#x", kernel_types[i]);

        ret = GetSecurityInfo(NULL, kernel_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
        ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

        ret = GetSecurityInfo(GetCurrentProcess(), kernel_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
        ok(!ret, "got error %lu\n", ret);
        LocalFree(pSD);

        ret = GetSecurityInfo(obj, kernel_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
        ok(!ret, "got error %lu\n", ret);
        LocalFree(pSD);

        ret = SetSecurityInfo(NULL, kernel_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, pDacl, NULL);
        ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

        ret = SetSecurityInfo(obj, kernel_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, pDacl, NULL);
        ok(!ret || ret == ERROR_NO_SECURITY_ON_OBJECT /* win 7 */, "got error %lu\n", ret);

        winetest_pop_context();
    }

    ret = GetSecurityInfo(GetCurrentProcess(), SE_REGISTRY_KEY,
            DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
    todo_wine ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

    ret = GetSecurityInfo(obj, SE_REGISTRY_KEY,
            DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
    todo_wine ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

    CloseHandle(obj);

    for (size_t i = 0; i < ARRAY_SIZE(invalid_types); ++i)
    {
        winetest_push_context("Type %#x", invalid_types[i]);

        ret = GetSecurityInfo(NULL, invalid_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
        ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

        ret = GetSecurityInfo((HANDLE)0xdeadbeef, invalid_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD);
        todo_wine ok(ret == ERROR_INVALID_PARAMETER, "got error %lu\n", ret);

        ret = SetSecurityInfo(NULL, invalid_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, pDacl, NULL);
        ok(ret == ERROR_INVALID_HANDLE, "got error %lu\n", ret);

        ret = SetSecurityInfo((HANDLE)0xdeadbeef, invalid_types[i],
                DACL_SECURITY_INFORMATION, NULL, NULL, pDacl, NULL);
        todo_wine ok(ret == ERROR_INVALID_PARAMETER, "got error %lu\n", ret);

        winetest_pop_context();
    }
}

static void test_GetSidSubAuthority(void)
{
    PSID psid = NULL;

    /* Note: on windows passing in an invalid index like -1, lets GetSidSubAuthority return 0x05000000 but
             still GetLastError returns ERROR_SUCCESS then. We don't test these unlikely cornercases here for now */
    ok(ConvertStringSidToSidA("S-1-5-21-93476-23408-4576",&psid),"ConvertStringSidToSidA failed\n");
    ok(IsValidSid(psid),"Sid is not valid\n");
    SetLastError(0xbebecaca);
    ok(*GetSidSubAuthorityCount(psid) == 4,"GetSidSubAuthorityCount gave %d expected 4\n", *GetSidSubAuthorityCount(psid));
    ok(GetLastError() == 0,"GetLastError returned %ld instead of 0\n",GetLastError());
    SetLastError(0xbebecaca);
    ok(*GetSidSubAuthority(psid,0) == 21,"GetSidSubAuthority gave %ld expected 21\n", *GetSidSubAuthority(psid,0));
    ok(GetLastError() == 0,"GetLastError returned %ld instead of 0\n",GetLastError());
    SetLastError(0xbebecaca);
    ok(*GetSidSubAuthority(psid,1) == 93476,"GetSidSubAuthority gave %ld expected 93476\n", *GetSidSubAuthority(psid,1));
    ok(GetLastError() == 0,"GetLastError returned %ld instead of 0\n",GetLastError());
    SetLastError(0xbebecaca);
    ok(GetSidSubAuthority(psid,4) != NULL,"Expected out of bounds GetSidSubAuthority to return a non-NULL pointer\n");
    ok(GetLastError() == 0,"GetLastError returned %ld instead of 0\n",GetLastError());
    LocalFree(psid);
}

static void test_CheckTokenMembership(void)
{
    PTOKEN_GROUPS token_groups;
    DWORD size;
    HANDLE process_token, token;
    BOOL is_member;
    BOOL ret;
    DWORD i;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &process_token);
    ok(ret, "OpenProcessToken failed with error %ld\n", GetLastError());

    ret = DuplicateToken(process_token, SecurityImpersonation, &token);
    ok(ret, "DuplicateToken failed with error %ld\n", GetLastError());

    /* groups */
    ret = GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "GetTokenInformation(TokenGroups) %s with error %ld\n",
        ret ? "succeeded" : "failed", GetLastError());
    token_groups = malloc(size);
    ret = GetTokenInformation(token, TokenGroups, token_groups, size, &size);
    ok(ret, "GetTokenInformation(TokenGroups) failed with error %ld\n", GetLastError());

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        if (token_groups->Groups[i].Attributes & SE_GROUP_ENABLED)
            break;
    }

    if (i == token_groups->GroupCount)
    {
        free(token_groups);
        CloseHandle(token);
        skip("user not a member of any group\n");
        return;
    }

    is_member = FALSE;
    ret = CheckTokenMembership(token, token_groups->Groups[i].Sid, &is_member);
    ok(ret, "CheckTokenMembership failed with error %ld\n", GetLastError());
    ok(is_member, "CheckTokenMembership should have detected sid as member\n");

    is_member = FALSE;
    ret = CheckTokenMembership(NULL, token_groups->Groups[i].Sid, &is_member);
    ok(ret, "CheckTokenMembership failed with error %ld\n", GetLastError());
    ok(is_member, "CheckTokenMembership should have detected sid as member\n");

    is_member = TRUE;
    SetLastError(0xdeadbeef);
    ret = CheckTokenMembership(process_token, token_groups->Groups[i].Sid, &is_member);
    ok(!ret && GetLastError() == ERROR_NO_IMPERSONATION_TOKEN,
        "CheckTokenMembership with process token %s with error %ld\n",
        ret ? "succeeded" : "failed", GetLastError());
    ok(!is_member, "CheckTokenMembership should have cleared is_member\n");

    free(token_groups);
    CloseHandle(token);
    CloseHandle(process_token);
}

static void test_EqualSid(void)
{
    PSID sid1, sid2;
    BOOL ret;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };

    SetLastError(0xdeadbeef);
    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid1);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("AllocateAndInitializeSid is not implemented\n");
        return;
    }
    ok(ret, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef,
       "AllocateAndInitializeSid shouldn't have set last error to %ld\n",
       GetLastError());

    ret = AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0, &sid2);
    ok(ret, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EqualSid(sid1, sid2);
    ok(!ret, "World and domain admins sids shouldn't have been equal\n");
    ok(GetLastError() == ERROR_SUCCESS,
       "EqualSid should have set last error to ERROR_SUCCESS instead of %ld\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    sid2 = FreeSid(sid2);
    ok(!sid2, "FreeSid should have returned NULL instead of %p\n", sid2);
    ok(GetLastError() == 0xdeadbeef,
       "FreeSid shouldn't have set last error to %ld\n",
       GetLastError());

    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid2);
    ok(ret, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

#ifndef __REACTOS__ // This crashes on WS03, Vista, Win7, and Win8.1.
    SetLastError(0xdeadbeef);
    ret = EqualSid(sid1, sid2);
    ok(ret, "Same sids should have been equal %s != %s\n",
       debugstr_sid(sid1), debugstr_sid(sid2));
    ok(GetLastError() == ERROR_SUCCESS,
       "EqualSid should have set last error to ERROR_SUCCESS instead of %ld\n",
       GetLastError());
#endif

    ((SID *)sid2)->Revision = 2;
    SetLastError(0xdeadbeef);
    ret = EqualSid(sid1, sid2);
    ok(!ret, "EqualSid with invalid sid should have returned FALSE\n");
    ok(GetLastError() == ERROR_SUCCESS,
       "EqualSid should have set last error to ERROR_SUCCESS instead of %ld\n",
       GetLastError());
    ((SID *)sid2)->Revision = SID_REVISION;

    FreeSid(sid1);
    FreeSid(sid2);
}

static void test_GetUserNameA(void)
{
    char buffer[UNLEN + 1], filler[UNLEN + 1];
    DWORD required_len, buffer_len;
    BOOL ret;

    /* Test crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        GetUserNameA(NULL, NULL);
    }

    SetLastError(0xdeadbeef);
    required_len = 0;
    ret = GetUserNameA(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(required_len != 0, "Outputted buffer length was %lu\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    required_len = 1;
    ret = GetUserNameA(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(required_len != 0 && required_len != 1, "Outputted buffer length was %lu\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());

    /* Tests crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        required_len = UNLEN + 1;
        GetUserNameA(NULL, &required_len);

        SetLastError(0xdeadbeef);
        GetUserNameA(buffer, NULL);
    }

    memset(filler, 'x', sizeof(filler));

    /* Note that GetUserNameA on XP and newer outputs the number of bytes
     * required for a Unicode string, which affects a test in the next block. */
    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    required_len = 0;
    ret = GetUserNameA(buffer, &required_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)), "Output buffer was altered\n");
    ok(required_len != 0, "Outputted buffer length was %lu\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len = required_len;
    ret = GetUserNameA(buffer, &buffer_len);
    ok(ret == TRUE, "GetUserNameA returned %d, last error %lu\n", ret, GetLastError());
    ok(memcmp(buffer, filler, sizeof(filler)) != 0, "Output buffer was untouched\n");
    ok(buffer_len == required_len ||
       broken(buffer_len == required_len / sizeof(WCHAR)), /* XP+ */
       "Outputted buffer length was %lu\n", buffer_len);
    ok(GetLastError() == 0xdeadbeef, "Last error was %lu\n", GetLastError());

    /* Use the reported buffer size from the last GetUserNameA call and pass
     * a length that is one less than the required value. */
    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len--;
    ret = GetUserNameA(buffer, &buffer_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)), "Output buffer was untouched\n");
    ok(buffer_len == required_len, "Outputted buffer length was %lu\n", buffer_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());
}

static void test_GetUserNameW(void)
{
    WCHAR buffer[UNLEN + 1], filler[UNLEN + 1];
    DWORD required_len, buffer_len;
    BOOL ret;

    /* Test crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        GetUserNameW(NULL, NULL);
    }

    SetLastError(0xdeadbeef);
    required_len = 0;
    ret = GetUserNameW(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(required_len != 0, "Outputted buffer length was %lu\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    required_len = 1;
    ret = GetUserNameW(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(required_len != 0 && required_len != 1, "Outputted buffer length was %lu\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());

    /* Tests crash on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        required_len = UNLEN + 1;
        GetUserNameW(NULL, &required_len);

        SetLastError(0xdeadbeef);
        GetUserNameW(buffer, NULL);
    }

    memset(filler, 'x', sizeof(filler));

    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    required_len = 0;
    ret = GetUserNameW(buffer, &required_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)), "Output buffer was altered\n");
    ok(required_len != 0, "Outputted buffer length was %lu\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len = required_len;
    ret = GetUserNameW(buffer, &buffer_len);
    ok(ret == TRUE, "GetUserNameW returned %d, last error %lu\n", ret, GetLastError());
    ok(memcmp(buffer, filler, sizeof(filler)) != 0, "Output buffer was untouched\n");
    ok(buffer_len == required_len, "Outputted buffer length was %lu\n", buffer_len);
    ok(GetLastError() == 0xdeadbeef, "Last error was %lu\n", GetLastError());

    /* GetUserNameW on XP and newer writes a truncated portion of the username string to the buffer. */
    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len--;
    ret = GetUserNameW(buffer, &buffer_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)) ||
       broken(memcmp(buffer, filler, sizeof(filler)) != 0), /* XP+ */
       "Output buffer was altered\n");
    ok(buffer_len == required_len, "Outputted buffer length was %lu\n", buffer_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %lu\n", GetLastError());
}

static void test_CreateRestrictedToken(void)
{
    HANDLE process_token, token, r_token;
    PTOKEN_GROUPS token_groups, groups2;
    LUID_AND_ATTRIBUTES lattr;
    SID_AND_ATTRIBUTES sattr;
    SECURITY_IMPERSONATION_LEVEL level;
    SID *removed_sid = NULL;
    char privs_buffer[1000];
    TOKEN_PRIVILEGES *privs = (TOKEN_PRIVILEGES *)privs_buffer;
    PRIVILEGE_SET priv_set;
    TOKEN_TYPE type;
    BOOL is_member;
    DWORD size;
    LUID luid = { 0, 0 };
    BOOL ret;
    DWORD i;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &process_token);
    ok(ret, "got error %ld\n", GetLastError());

    ret = DuplicateTokenEx(process_token, TOKEN_DUPLICATE|TOKEN_ADJUST_GROUPS|TOKEN_QUERY,
        NULL, SecurityImpersonation, TokenImpersonation, &token);
    ok(ret, "got error %ld\n", GetLastError());

    ret = GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "got %d with error %ld\n", ret, GetLastError());
    token_groups = malloc(size);
    ret = GetTokenInformation(token, TokenGroups, token_groups, size, &size);
    ok(ret, "got error %ld\n", GetLastError());

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        if (token_groups->Groups[i].Attributes & SE_GROUP_ENABLED)
        {
            removed_sid = token_groups->Groups[i].Sid;
            break;
        }
    }
    ok(!!removed_sid, "user is not a member of any group\n");

    is_member = FALSE;
    ret = CheckTokenMembership(token, removed_sid, &is_member);
    ok(ret, "got error %ld\n", GetLastError());
    ok(is_member, "not a member\n");

    sattr.Sid = removed_sid;
    sattr.Attributes = 0;
    r_token = NULL;
    ret = CreateRestrictedToken(token, 0, 1, &sattr, 0, NULL, 0, NULL, &r_token);
    ok(ret, "got error %ld\n", GetLastError());

    is_member = TRUE;
    ret = CheckTokenMembership(r_token, removed_sid, &is_member);
    ok(ret, "got error %ld\n", GetLastError());
    ok(!is_member, "not a member\n");

    ret = GetTokenInformation(r_token, TokenGroups, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d with error %ld\n",
        ret, GetLastError());
    groups2 = malloc(size);
    ret = GetTokenInformation(r_token, TokenGroups, groups2, size, &size);
    ok(ret, "got error %ld\n", GetLastError());

    for (i = 0; i < groups2->GroupCount; i++)
    {
        if (EqualSid(groups2->Groups[i].Sid, removed_sid))
        {
            DWORD attr = groups2->Groups[i].Attributes;
            ok(attr & SE_GROUP_USE_FOR_DENY_ONLY, "got wrong attributes %#lx\n", attr);
            ok(!(attr & SE_GROUP_ENABLED), "got wrong attributes %#lx\n", attr);
            break;
        }
    }

    free(groups2);

    size = sizeof(type);
    ret = GetTokenInformation(r_token, TokenType, &type, size, &size);
    ok(ret, "got error %ld\n", GetLastError());
    ok(type == TokenImpersonation, "got type %u\n", type);

    size = sizeof(level);
    ret = GetTokenInformation(r_token, TokenImpersonationLevel, &level, size, &size);
    ok(ret, "got error %ld\n", GetLastError());
    ok(level == SecurityImpersonation, "got level %u\n", type);

    CloseHandle(r_token);

    r_token = NULL;
    ret = CreateRestrictedToken(process_token, 0, 1, &sattr, 0, NULL, 0, NULL, &r_token);
    ok(ret, "got error %lu\n", GetLastError());

    size = sizeof(type);
    ret = GetTokenInformation(r_token, TokenType, &type, size, &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(type == TokenPrimary, "got type %u\n", type);

    CloseHandle(r_token);

    ret = GetTokenInformation(token, TokenPrivileges, privs, sizeof(privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());

    for (i = 0; i < privs->PrivilegeCount; i++)
    {
        if (privs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
        {
            luid = privs->Privileges[i].Luid;
            break;
        }
    }
    ok(i < privs->PrivilegeCount, "user has no privileges\n");

    lattr.Luid = luid;
    lattr.Attributes = 0;
    r_token = NULL;
    ret = CreateRestrictedToken(token, 0, 0, NULL, 1, &lattr, 0, NULL, &r_token);
    ok(ret, "got error %lu\n", GetLastError());

    priv_set.PrivilegeCount = 1;
    priv_set.Control = 0;
    priv_set.Privilege[0].Luid = luid;
    priv_set.Privilege[0].Attributes = 0;
    ret = PrivilegeCheck(r_token, &priv_set, &is_member);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!is_member, "privilege should not be enabled\n");

    ret = GetTokenInformation(r_token, TokenPrivileges, privs, sizeof(privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());

    is_member = FALSE;
    for (i = 0; i < privs->PrivilegeCount; i++)
    {
        if (!memcmp(&privs->Privileges[i].Luid, &luid, sizeof(luid)))
            is_member = TRUE;
    }
    ok(!is_member, "disabled privilege should not be present\n");

    CloseHandle(r_token);

    removed_sid->SubAuthority[0] = 0xdeadbeef;
    lattr.Luid.LowPart = 0xdeadbeef;
    r_token = NULL;
    ret = CreateRestrictedToken(token, 0, 1, &sattr, 1, &lattr, 0, NULL, &r_token);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(r_token);

    free(token_groups);
    CloseHandle(token);
    CloseHandle(process_token);
}

static void validate_default_security_descriptor(SECURITY_DESCRIPTOR *sd)
{
    BOOL ret, present, defaulted;
    ACL *acl;
    void *sid;

    ret = IsValidSecurityDescriptor(sd);
    ok(ret, "security descriptor is not valid\n");

    present = -1;
    defaulted = -1;
    acl = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetSecurityDescriptorDacl(sd, &present, &acl, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl error %ld\n", GetLastError());
    todo_wine
    ok(present == 1, "acl is not present\n");
    todo_wine
    ok(acl != (void *)0xdeadbeef && acl != NULL, "acl pointer is not set\n");
    ok(defaulted == 0, "defaulted is set to TRUE\n");

    defaulted = -1;
    sid = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetSecurityDescriptorOwner(sd, &sid, &defaulted);
    ok(ret, "GetSecurityDescriptorOwner error %ld\n", GetLastError());
    todo_wine
    ok(sid != (void *)0xdeadbeef && sid != NULL, "sid pointer is not set\n");
    ok(defaulted == 0, "defaulted is set to TRUE\n");

    defaulted = -1;
    sid = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetSecurityDescriptorGroup(sd, &sid, &defaulted);
    ok(ret, "GetSecurityDescriptorGroup error %ld\n", GetLastError());
    todo_wine
    ok(sid != (void *)0xdeadbeef && sid != NULL, "sid pointer is not set\n");
    ok(defaulted == 0, "defaulted is set to TRUE\n");
}

static void test_default_handle_security(HANDLE token, HANDLE handle, GENERIC_MAPPING *mapping)
{
    DWORD ret, granted, priv_set_len;
    BOOL status;
    PRIVILEGE_SET priv_set;
    SECURITY_DESCRIPTOR *sd;

    sd = test_get_security_descriptor(handle, __LINE__);
    validate_default_security_descriptor(sd);

    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, MAXIMUM_ALLOWED, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericAll, "expected all access %#lx, got %#lx\n", mapping->GenericAll, granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, 0, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %ld\n", GetLastError());
    ok(status == 0, "expected 0, got %d\n", status);
    ok(granted == 0, "expected 0, got %#lx\n", granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, ACCESS_SYSTEM_SECURITY, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %ld\n", GetLastError());
    ok(status == 0, "expected 0, got %d\n", status);
    ok(granted == 0, "expected 0, got %#lx\n", granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, mapping->GenericRead, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericRead, "expected read access %#lx, got %#lx\n", mapping->GenericRead, granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, mapping->GenericWrite, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericWrite, "expected write access %#lx, got %#lx\n", mapping->GenericWrite, granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, mapping->GenericExecute, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %ld\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericExecute, "expected execute access %#lx, got %#lx\n", mapping->GenericExecute, granted);
}
    free(sd);
}

static ACCESS_MASK get_obj_access(HANDLE obj)
{
    OBJECT_BASIC_INFORMATION info;
    NTSTATUS status;

    status = NtQueryObject(obj, ObjectBasicInformation, &info, sizeof(info), NULL);
    ok(!status, "NtQueryObject error %#lx\n", status);

    return info.GrantedAccess;
}

static void test_mutex_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE mutex, dup;
    GENERIC_MAPPING mapping = { STANDARD_RIGHTS_READ | MUTANT_QUERY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_WRITE | MUTEX_MODIFY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                STANDARD_RIGHTS_ALL | MUTEX_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | MUTANT_QUERY_STATE },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | MUTANT_QUERY_STATE }
    };

    SetLastError(0xdeadbeef);
    mutex = OpenMutexA(0, FALSE, "WineTestMutex");
    ok(!mutex, "mutex should not exist\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    mutex = CreateMutexA(NULL, FALSE, "WineTestMutex");
    ok(mutex != 0, "CreateMutex error %ld\n", GetLastError());

    access = get_obj_access(mutex);
    ok(access == MUTANT_ALL_ACCESS, "expected MUTANT_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), mutex, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);

        SetLastError(0xdeadbeef);
        dup = OpenMutexA(0, FALSE, "WineTestMutex");
        todo_wine
        ok(!dup, "OpenMutex should fail\n");
        todo_wine
        ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError());
    }

    test_default_handle_security(token, mutex, &mapping);

    CloseHandle (mutex);
}

static void test_event_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE event, dup;
    GENERIC_MAPPING mapping = { STANDARD_RIGHTS_READ | EVENT_QUERY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                STANDARD_RIGHTS_ALL | EVENT_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | EVENT_QUERY_STATE },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | EVENT_QUERY_STATE | EVENT_MODIFY_STATE }
    };

    SetLastError(0xdeadbeef);
    event = OpenEventA(0, FALSE, "WineTestEvent");
    ok(!event, "event should not exist\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    event = CreateEventA(NULL, FALSE, FALSE, "WineTestEvent");
    ok(event != 0, "CreateEvent error %ld\n", GetLastError());

    access = get_obj_access(event);
    ok(access == EVENT_ALL_ACCESS, "expected EVENT_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), event, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);

        SetLastError(0xdeadbeef);
        dup = OpenEventA(0, FALSE, "WineTestEvent");
        todo_wine
        ok(!dup, "OpenEvent should fail\n");
        todo_wine
        ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError());
    }

    test_default_handle_security(token, event, &mapping);

    CloseHandle(event);
}

static void test_semaphore_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE sem, dup;
    GENERIC_MAPPING mapping = { STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE,
                                STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE,
                                STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                STANDARD_RIGHTS_ALL | SEMAPHORE_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE }
    };

    SetLastError(0xdeadbeef);
    sem = OpenSemaphoreA(0, FALSE, "WineTestSemaphore");
    ok(!sem, "semaphore should not exist\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    sem = CreateSemaphoreA(NULL, 0, 10, "WineTestSemaphore");
    ok(sem != 0, "CreateSemaphore error %ld\n", GetLastError());

    access = get_obj_access(sem);
    ok(access == SEMAPHORE_ALL_ACCESS, "expected SEMAPHORE_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), sem, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    test_default_handle_security(token, sem, &mapping);

    CloseHandle(sem);
}

#define WINE_TEST_PIPE "\\\\.\\pipe\\WineTestPipe"
static void test_named_pipe_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE pipe, file, dup;
    GENERIC_MAPPING mapping = { FILE_GENERIC_READ,
                                FILE_GENERIC_WRITE,
                                FILE_GENERIC_EXECUTE,
                                STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, FILE_GENERIC_READ },
        { GENERIC_WRITE, FILE_GENERIC_WRITE },
        { GENERIC_EXECUTE, FILE_GENERIC_EXECUTE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS }
    };
    static const struct
    {
        DWORD open_mode;
        DWORD access;
    } creation_access[] =
    {
        { PIPE_ACCESS_INBOUND, FILE_GENERIC_READ },
        { PIPE_ACCESS_OUTBOUND, FILE_GENERIC_WRITE },
        { PIPE_ACCESS_DUPLEX, FILE_GENERIC_READ|FILE_GENERIC_WRITE },
        { PIPE_ACCESS_INBOUND|WRITE_DAC, FILE_GENERIC_READ|WRITE_DAC },
        { PIPE_ACCESS_INBOUND|WRITE_OWNER, FILE_GENERIC_READ|WRITE_OWNER }
        /* ACCESS_SYSTEM_SECURITY is also valid, but will fail with ERROR_PRIVILEGE_NOT_HELD */
    };

    /* Test the different security access options for pipes */
    for (i = 0; i < ARRAY_SIZE(creation_access); i++)
    {
        SetLastError(0xdeadbeef);
        pipe = CreateNamedPipeA(WINE_TEST_PIPE, creation_access[i].open_mode,
                                PIPE_TYPE_BYTE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES, 0, 0,
                                NMPWAIT_USE_DEFAULT_WAIT, NULL);
        ok(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe(0x%lx) error %ld\n",
                                         creation_access[i].open_mode, GetLastError());
        access = get_obj_access(pipe);
        ok(access == creation_access[i].access,
           "CreateNamedPipeA(0x%lx) pipe expected access 0x%lx (got 0x%lx)\n",
           creation_access[i].open_mode, creation_access[i].access, access);
        CloseHandle(pipe);
    }

    SetLastError(0xdeadbeef);
    pipe = CreateNamedPipeA(WINE_TEST_PIPE, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                            PIPE_TYPE_BYTE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES,
                            0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe error %ld\n", GetLastError());

    test_default_handle_security(token, pipe, &mapping);

    SetLastError(0xdeadbeef);
    file = CreateFileA(WINE_TEST_PIPE, FILE_ALL_ACCESS, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(file);
    CloseHandle(pipe);

    SetLastError(0xdeadbeef);
    file = CreateFileA("\\\\.\\pipe\\", FILE_ALL_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE || broken(file == INVALID_HANDLE_VALUE) /* before Vista */, "CreateFile error %ld\n", GetLastError());

    if (file != INVALID_HANDLE_VALUE)
    {
        access = get_obj_access(file);
        ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#lx\n", access);

        for (i = 0; i < ARRAY_SIZE(map); i++)
        {
            SetLastError( 0xdeadbeef );
            ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                                  map[i].generic, FALSE, 0);
            ok(ret, "DuplicateHandle error %ld\n", GetLastError());

            access = get_obj_access(dup);
            ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            CloseHandle(dup);
        }
    }

    CloseHandle(file);
}

static void test_file_security(HANDLE token)
{
    DWORD ret, i, access, bytes;
    HANDLE file, dup;
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, FILE_GENERIC_READ },
        { GENERIC_WRITE, FILE_GENERIC_WRITE },
        { GENERIC_EXECUTE, FILE_GENERIC_EXECUTE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS }
    };
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    char buf[16];

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "tmp", 0, file_name);

    /* file */
    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == (FILE_READ_ATTRIBUTES | SYNCHRONIZE), "expected FILE_READ_ATTRIBUTES | SYNCHRONIZE, got %#lx\n", access);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(file, buf, sizeof(buf), &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    ok(bytes == 0, "expected 0, got %lu\n", bytes);

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == (FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES), "expected FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES, got %#lx\n", access);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(file, buf, sizeof(buf), &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    ok(bytes == 0, "expected 0, got %lu\n", bytes);

    CloseHandle(file);
    DeleteFileA(file_name);

    /* directory */
    SetLastError(0xdeadbeef);
    file = CreateFileA(temp_path, GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(temp_path, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == (FILE_READ_ATTRIBUTES | SYNCHRONIZE), "expected FILE_READ_ATTRIBUTES | SYNCHRONIZE, got %#lx\n", access);

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(temp_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    access = get_obj_access(file);
    ok(access == (FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES), "expected FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES, got %#lx\n", access);

    CloseHandle(file);
}

static void test_filemap_security(void)
{
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    DWORD ret, i, access;
    HANDLE file, mapping, dup, created_mapping;
    static const struct
    {
        int generic, mapped;
        BOOL open_only;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | SECTION_QUERY | SECTION_MAP_READ },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE },
        { GENERIC_ALL, STANDARD_RIGHTS_REQUIRED | SECTION_ALL_ACCESS },
        { SECTION_MAP_READ | SECTION_MAP_WRITE, SECTION_MAP_READ | SECTION_MAP_WRITE },
        { SECTION_MAP_WRITE, SECTION_MAP_WRITE },
        { SECTION_MAP_READ | SECTION_QUERY, SECTION_MAP_READ | SECTION_QUERY },
        { SECTION_QUERY, SECTION_MAP_READ, TRUE },
        { SECTION_QUERY | SECTION_MAP_READ, SECTION_QUERY | SECTION_MAP_READ }
    };
    static const struct
    {
        int prot, mapped;
    } prot_map[] =
    {
        { 0, 0 },
        { PAGE_NOACCESS, 0 },
        { PAGE_READONLY, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ },
        { PAGE_READWRITE, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE },
        { PAGE_WRITECOPY, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ },
        { PAGE_EXECUTE, 0 },
        { PAGE_EXECUTE_READ, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE },
        { PAGE_EXECUTE_READWRITE, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE },
        { PAGE_EXECUTE_WRITECOPY, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE }
    };

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "tmp", 0, file_name);

    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
    SetFilePointer(file, 4096, NULL, FILE_BEGIN);
    SetEndOfFile(file);

    for (i = 0; i < ARRAY_SIZE(prot_map); i++)
    {
        if (map[i].open_only) continue;

        SetLastError(0xdeadbeef);
        mapping = CreateFileMappingW(file, NULL, prot_map[i].prot, 0, 4096, NULL);
        if (prot_map[i].mapped)
        {
            ok(mapping != 0, "CreateFileMapping(%04x) error %ld\n", prot_map[i].prot, GetLastError());
        }
        else
        {
            ok(!mapping, "CreateFileMapping(%04x) should fail\n", prot_map[i].prot);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
            continue;
        }

        access = get_obj_access(mapping);
        ok(access == prot_map[i].mapped, "%ld: expected %#x, got %#lx\n", i, prot_map[i].mapped, access);

        CloseHandle(mapping);
    }

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingW(file, NULL, PAGE_EXECUTE_READWRITE, 0, 4096, NULL);
    ok(mapping != 0, "CreateFileMapping error %ld\n", GetLastError());

    access = get_obj_access(mapping);
    ok(access == (STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE),
       "expected STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        if (map[i].open_only) continue;

        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), mapping, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(mapping);
    CloseHandle(file);
    DeleteFileA(file_name);

    created_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 0x1000,
                                         "Wine Test Open Mapping");
    ok(created_mapping != NULL, "CreateFileMapping failed with error %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        if (!map[i].generic) continue;

        mapping = OpenFileMappingA(map[i].generic, FALSE, "Wine Test Open Mapping");
        ok(mapping != NULL, "OpenFileMapping failed with error %ld\n", GetLastError());
        access = get_obj_access(mapping);
        ok(access == map[i].mapped, "%ld: unexpected access flags %#lx, expected %#x\n",
           i, access, map[i].mapped);
        CloseHandle(mapping);
    }

    CloseHandle(created_mapping);
}

static void test_thread_security(void)
{
    DWORD ret, i, access;
    HANDLE thread, dup;
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | THREAD_SET_INFORMATION | THREAD_SET_CONTEXT | THREAD_TERMINATE | THREAD_SUSPEND_RESUME | 0x4 },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, THREAD_ALL_ACCESS_NT4 }
    };

    SetLastError(0xdeadbeef);
    thread = CreateThread(NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(thread != 0, "CreateThread error %ld\n", GetLastError());

    access = get_obj_access(thread);
    ok(access == THREAD_ALL_ACCESS_NT4 || access == THREAD_ALL_ACCESS_VISTA, "expected THREAD_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), thread, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        switch (map[i].generic)
        {
        case GENERIC_READ:
        case GENERIC_EXECUTE:
            ok(access == map[i].mapped ||
               access == (map[i].mapped | THREAD_QUERY_LIMITED_INFORMATION) /* Vista+ */ ||
               access == (map[i].mapped | THREAD_QUERY_LIMITED_INFORMATION | THREAD_RESUME) /* win8 */,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        case GENERIC_WRITE:
            ok(access == map[i].mapped ||
               access == (map[i].mapped | THREAD_SET_LIMITED_INFORMATION) /* Vista+ */ ||
               access == (map[i].mapped | THREAD_SET_LIMITED_INFORMATION | THREAD_RESUME) /* win8 */,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        case GENERIC_ALL:
            ok(access == map[i].mapped || access == THREAD_ALL_ACCESS_VISTA,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        default:
            ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        }

        CloseHandle(dup);
    }

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle(GetCurrentProcess(), thread, GetCurrentProcess(), &dup,
                          THREAD_QUERY_INFORMATION, FALSE, 0);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());
    access = get_obj_access(dup);
    ok(access == (THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION) /* Vista+ */ ||
       access == THREAD_QUERY_INFORMATION /* before Vista */,
       "expected THREAD_QUERY_INFORMATION|THREAD_QUERY_LIMITED_INFORMATION, got %#lx\n", access);
    CloseHandle(dup);

    TerminateThread(thread, 0);
    CloseHandle(thread);
}

static void test_process_access(void)
{
    DWORD ret, i, access;
    HANDLE process, dup;
    STARTUPINFOA sti;
    PROCESS_INFORMATION pi;
    char cmdline[] = "winver.exe";
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME |
                         PROCESS_VM_WRITE | PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, PROCESS_ALL_ACCESS_NT4 }
    };

    memset(&sti, 0, sizeof(sti));
    sti.cb = sizeof(sti);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sti, &pi);
    ok(ret, "CreateProcess() error %ld\n", GetLastError());

    CloseHandle(pi.hThread);
    process = pi.hProcess;

    access = get_obj_access(process);
    ok(access == PROCESS_ALL_ACCESS_NT4 || access == PROCESS_ALL_ACCESS_VISTA, "expected PROCESS_ALL_ACCESS, got %#lx\n", access);

    for (i = 0; i < ARRAY_SIZE(map); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %ld\n", GetLastError());

        access = get_obj_access(dup);
        switch (map[i].generic)
        {
        case GENERIC_READ:
            ok(access == map[i].mapped || access == (map[i].mapped | PROCESS_QUERY_LIMITED_INFORMATION) /* Vista+ */,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        case GENERIC_WRITE:
            ok(access == map[i].mapped ||
               access == (map[i].mapped | PROCESS_TERMINATE) /* before Vista */ ||
               access == (map[i].mapped | PROCESS_SET_LIMITED_INFORMATION) /* win8 */ ||
               access == (map[i].mapped | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_LIMITED_INFORMATION) /* Win10 Anniversary Update */,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        case GENERIC_EXECUTE:
            ok(access == map[i].mapped || access == (map[i].mapped | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE) /* Vista+ */,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        case GENERIC_ALL:
            ok(access == map[i].mapped || access == PROCESS_ALL_ACCESS_VISTA,
               "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        default:
            ok(access == map[i].mapped, "%ld: expected %#x, got %#lx\n", i, map[i].mapped, access);
            break;
        }

        CloseHandle(dup);
    }

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                          PROCESS_QUERY_INFORMATION, FALSE, 0);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());
    access = get_obj_access(dup);
    ok(access == (PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION) /* Vista+ */ ||
       access == PROCESS_QUERY_INFORMATION /* before Vista */,
       "expected PROCESS_QUERY_INFORMATION|PROCESS_QUERY_LIMITED_INFORMATION, got %#lx\n", access);
    CloseHandle(dup);

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                          PROCESS_VM_OPERATION, FALSE, 0);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());
    access = get_obj_access(dup);
    ok(access == PROCESS_VM_OPERATION, "unexpected access right %lx\n", access);
    CloseHandle(dup);

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                          PROCESS_VM_WRITE, FALSE, 0);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());
    access = get_obj_access(dup);
    ok(access == PROCESS_VM_WRITE, "unexpected access right %lx\n", access);
    CloseHandle(dup);

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                          PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, 0);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());
    access = get_obj_access(dup);
    ok(access == (PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION) ||
       broken(access == (PROCESS_VM_OPERATION | PROCESS_VM_WRITE)) /* Win8 and before */,
       "expected PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_QUERY_LIMITED_INFORMATION, got %#lx\n", access);
    CloseHandle(dup);

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                          PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, 0);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());
    access = get_obj_access(dup);
    ok(access == (PROCESS_VM_OPERATION | PROCESS_VM_READ), "unexpected access right %lx\n", access);
    CloseHandle(dup);

    TerminateProcess(process, 0);
    CloseHandle(process);
}

static BOOL validate_impersonation_token(HANDLE token, DWORD *token_type)
{
    DWORD ret, needed;
    TOKEN_TYPE type;
    SECURITY_IMPERSONATION_LEVEL sil;

    type = 0xdeadbeef;
    needed = 0;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenType, &type, sizeof(type), &needed);
    ok(ret, "GetTokenInformation error %ld\n", GetLastError());
    ok(needed == sizeof(type), "GetTokenInformation should return required buffer length\n");
    ok(type == TokenPrimary || type == TokenImpersonation, "expected TokenPrimary or TokenImpersonation, got %d\n", type);

    *token_type = type;
    if (type != TokenImpersonation) return FALSE;

    needed = 0;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenImpersonationLevel, &sil, sizeof(sil), &needed);
    ok(ret, "GetTokenInformation error %ld\n", GetLastError());
    ok(needed == sizeof(sil), "GetTokenInformation should return required buffer length\n");
    ok(sil == SecurityImpersonation, "expected SecurityImpersonation, got %d\n", sil);

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenDefaultDacl, NULL, 0, &needed);
    ok(!ret, "GetTokenInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(needed != 0xdeadbeef, "GetTokenInformation should return required buffer length\n");
    ok(needed > sizeof(TOKEN_DEFAULT_DACL), "GetTokenInformation returned empty default DACL\n");

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenOwner, NULL, 0, &needed);
    ok(!ret, "GetTokenInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(needed != 0xdeadbeef, "GetTokenInformation should return required buffer length\n");
    ok(needed > sizeof(TOKEN_OWNER), "GetTokenInformation returned empty token owner\n");

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenPrimaryGroup, NULL, 0, &needed);
    ok(!ret, "GetTokenInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(needed != 0xdeadbeef, "GetTokenInformation should return required buffer length\n");
    ok(needed > sizeof(TOKEN_PRIMARY_GROUP), "GetTokenInformation returned empty token primary group\n");

    return TRUE;
}

static void test_kernel_objects_security(void)
{
    HANDLE token, process_token;
    DWORD ret, token_type;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &process_token);
    ok(ret, "OpenProcessToken error %ld\n", GetLastError());

    ret = validate_impersonation_token(process_token, &token_type);
    ok(token_type == TokenPrimary, "expected TokenPrimary, got %ld\n", token_type);
    ok(!ret, "access token should not be an impersonation token\n");

    ret = DuplicateToken(process_token, SecurityImpersonation, &token);
    ok(ret, "DuplicateToken error %ld\n", GetLastError());

    ret = validate_impersonation_token(token, &token_type);
    ok(ret, "access token should be a valid impersonation token\n");
    ok(token_type == TokenImpersonation, "expected TokenImpersonation, got %ld\n", token_type);

    test_mutex_security(token);
    test_event_security(token);
    test_named_pipe_security(token);
    test_semaphore_security(token);
    test_file_security(token);
    test_filemap_security();
    test_thread_security();
    test_process_access();
    /* FIXME: test other kernel object types */

    CloseHandle(process_token);
    CloseHandle(token);
}

static void test_TokenIntegrityLevel(void)
{
    TOKEN_MANDATORY_LABEL *tml;
    BYTE buffer[64];        /* using max. 28 byte in win7 x64 */
    HANDLE token;
    DWORD size;
    DWORD res;
    static SID medium_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                                                    {SECURITY_MANDATORY_HIGH_RID}};
    static SID high_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                                                    {SECURITY_MANDATORY_MEDIUM_RID}};

    SetLastError(0xdeadbeef);
    res = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    ok(res, "got %ld with %ld (expected TRUE)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = GetTokenInformation(token, TokenIntegrityLevel, buffer, sizeof(buffer), &size);

    /* not supported before Vista */
    if (!res && ((GetLastError() == ERROR_INVALID_PARAMETER) || GetLastError() == ERROR_INVALID_FUNCTION))
    {
        win_skip("TokenIntegrityLevel not supported\n");
        CloseHandle(token);
        return;
    }

    ok(res, "got %lu with %lu (expected TRUE)\n", res, GetLastError());
    if (!res)
    {
        CloseHandle(token);
        return;
    }

    tml = (TOKEN_MANDATORY_LABEL*) buffer;
    ok(tml->Label.Attributes == (SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED),
        "got 0x%lx (expected 0x%x)\n", tml->Label.Attributes, (SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED));

#ifdef __REACTOS__ // This crashes on Vista, Win7, and Win8.1.
    if (GetNTVersion() < _WIN32_WINNT_VISTA || GetNTVersion() >= _WIN32_WINNT_WIN10)
#endif
    ok(EqualSid(tml->Label.Sid, &medium_level) || EqualSid(tml->Label.Sid, &high_level),
       "got %s (expected %s or %s)\n", debugstr_sid(tml->Label.Sid),
       debugstr_sid(&medium_level), debugstr_sid(&high_level));

    CloseHandle(token);
}

static void test_default_dacl_owner_group_sid(void)
{
    TOKEN_USER *token_user;
    TOKEN_OWNER *token_owner;
    TOKEN_PRIMARY_GROUP *token_primary_group;
    HANDLE handle, token;
    BOOL ret, defaulted, present, found;
    DWORD size, index;
    SECURITY_DESCRIPTOR *sd;
    SECURITY_ATTRIBUTES sa;
    PSID owner, group;
    ACL *dacl;
    ACCESS_ALLOWED_ACE *ace;

    ret = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
    ok(ret, "OpenProcessToken failed with error %ld\n", GetLastError());

    token_user = get_alloc_token_user( token );
    token_owner = get_alloc_token_owner( token );
    token_primary_group = get_alloc_token_primary_group( token );

    CloseHandle( token );

    sd = malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );
    ret = InitializeSecurityDescriptor( sd, SECURITY_DESCRIPTOR_REVISION );
    ok( ret, "error %lu\n", GetLastError() );

    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle       = FALSE;
    handle = CreateEventA( &sa, TRUE, TRUE, "test_event" );
    ok( handle != NULL, "error %lu\n", GetLastError() );

    size = 0;
    ret = GetKernelObjectSecurity( handle, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, NULL, 0, &size );
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "error %lu\n", GetLastError() );

    sd = malloc( size );
    ret = GetKernelObjectSecurity( handle, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, sd, size, &size );
    ok( ret, "error %lu\n", GetLastError() );

    owner = (void *)0xdeadbeef;
    defaulted = TRUE;
    ret = GetSecurityDescriptorOwner( sd, &owner, &defaulted );
    ok( ret, "error %lu\n", GetLastError() );
    ok( owner != (void *)0xdeadbeef, "owner not set\n" );
    ok( !defaulted, "owner defaulted\n" );
    ok( EqualSid( owner, token_owner->Owner ), "owner shall equal token owner\n" );

    group = (void *)0xdeadbeef;
    defaulted = TRUE;
    ret = GetSecurityDescriptorGroup( sd, &group, &defaulted );
    ok( ret, "error %lu\n", GetLastError() );
    ok( group != (void *)0xdeadbeef, "group not set\n" );
    ok( !defaulted, "group defaulted\n" );
    ok( EqualSid( group, token_primary_group->PrimaryGroup ), "group shall equal token primary group\n" );

    dacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorDacl( sd, &present, &dacl, &defaulted );
    ok( ret, "error %lu\n", GetLastError() );
    ok( present, "dacl not present\n" );
    ok( dacl != (void *)0xdeadbeef, "dacl not set\n" );
    ok( !defaulted, "dacl defaulted\n" );

    index = 0;
    found = FALSE;
    while (GetAce( dacl, index++, (void **)&ace ))
    {
        ok( ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE,
            "expected ACCESS_ALLOWED_ACE_TYPE, got %d\n", ace->Header.AceType );
        if (EqualSid( &ace->SidStart, owner )) found = TRUE;
    }
    ok( found, "owner sid not found in dacl\n" );

    if (!EqualSid( token_user->User.Sid, token_owner->Owner ))
    {
        index = 0;
        found = FALSE;
        while (GetAce( dacl, index++, (void **)&ace ))
        {
            ok( ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE,
                "expected ACCESS_ALLOWED_ACE_TYPE, got %d\n", ace->Header.AceType );
            if (EqualSid( &ace->SidStart, token_user->User.Sid )) found = TRUE;
        }
        ok( !found, "DACL shall not reference token user if it is different from token owner\n" );
    }

    free( sa.lpSecurityDescriptor );
    free( sd );
    CloseHandle( handle );

    free( token_primary_group );
    free( token_owner );
    free( token_user );
}

static void test_AdjustTokenPrivileges(void)
{
    TOKEN_PRIVILEGES tp;
    HANDLE token;
    DWORD len;
    LUID luid;
    BOOL ret;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
        return;

    if (!LookupPrivilegeValueA(NULL, SE_BACKUP_NAME, &luid))
    {
        CloseHandle(token);
        return;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    len = 0xdeadbeef;
    ret = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, &len);
    ok(ret, "got %d\n", ret);
    ok(len == 0xdeadbeef, "got length %ld\n", len);

    /* revert */
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = 0;
    ret = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    ok(ret, "got %d\n", ret);

    CloseHandle(token);
}

static void test_AddAce(void)
{
    static SID const sidWorld = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY} , { SECURITY_WORLD_RID } };

    char acl_buf[1024], ace_buf[256];
    ACCESS_ALLOWED_ACE *ace = (ACCESS_ALLOWED_ACE*)ace_buf;
    PACL acl = (PACL)acl_buf;
    BOOL ret;

    memset(ace, 0, sizeof(ace_buf));
    ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ace->Header.AceSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+sizeof(SID);
    memcpy(&ace->SidStart, &sidWorld, sizeof(sidWorld));

    ret = InitializeAcl(acl, sizeof(acl_buf), ACL_REVISION2);
    ok(ret, "InitializeAcl failed: %ld\n", GetLastError());

    ret = AddAce(acl, ACL_REVISION1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    ret = AddAce(acl, ACL_REVISION2, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    ret = AddAce(acl, ACL_REVISION3, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    ok(acl->AclRevision == ACL_REVISION3, "acl->AclRevision = %d\n", acl->AclRevision);
    ret = AddAce(acl, ACL_REVISION4, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    ok(acl->AclRevision == ACL_REVISION4, "acl->AclRevision = %d\n", acl->AclRevision);
    ret = AddAce(acl, ACL_REVISION1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    ok(acl->AclRevision == ACL_REVISION4, "acl->AclRevision = %d\n", acl->AclRevision);
    ret = AddAce(acl, ACL_REVISION2, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());

    ret = AddAce(acl, MIN_ACL_REVISION-1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    /* next test succeededs but corrupts ACL */
    ret = AddAce(acl, MAX_ACL_REVISION+1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %ld\n", GetLastError());
    ok(acl->AclRevision == MAX_ACL_REVISION+1, "acl->AclRevision = %d\n", acl->AclRevision);
    SetLastError(0xdeadbeef);
    ret = AddAce(acl, ACL_REVISION1, MAXDWORD, ace, ace->Header.AceSize);
    ok(!ret, "AddAce succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %ld\n", GetLastError());
}

static void test_AddMandatoryAce(void)
{
    static SID low_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                            {SECURITY_MANDATORY_LOW_RID}};
    static SID medium_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                               {SECURITY_MANDATORY_MEDIUM_RID}};
    static SID_IDENTIFIER_AUTHORITY sia_world = {SECURITY_WORLD_SID_AUTHORITY};
    char buffer_sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_DESCRIPTOR *sd2, *sd = (SECURITY_DESCRIPTOR *)&buffer_sd;
    BOOL defaulted, present, ret;
    ACL_SIZE_INFORMATION acl_size_info;
    SYSTEM_MANDATORY_LABEL_ACE *ace;
    char buffer_acl[256];
    ACL *acl = (ACL *)&buffer_acl;
    SECURITY_ATTRIBUTES sa;
    DWORD size;
    HANDLE handle;
    SID *everyone;
    ACL *sacl;

    if (!pAddMandatoryAce)
    {
        win_skip("AddMandatoryAce not supported, skipping test\n");
        return;
    }

    ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(ret, "InitializeSecurityDescriptor failed with error %lu\n", GetLastError());

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle = FALSE;

    handle = CreateEventA(&sa, TRUE, TRUE, "test_event");
    ok(handle != NULL, "CreateEventA failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %u, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    sacl = (void *)0xdeadbeef;
    present = TRUE;
    ret = GetSecurityDescriptorSacl(sd2, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(!present, "SACL is present\n");
    ok(sacl == (void *)0xdeadbeef, "SACL is set\n");

    free(sd2);
    CloseHandle(handle);

    memset(buffer_acl, 0, sizeof(buffer_acl));
    ret = InitializeAcl(acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddMandatoryAce(acl, ACL_REVISION, 0, 0x1234, &low_level);
    ok(!ret, "AddMandatoryAce succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER got %lu\n", GetLastError());

    ret = pAddMandatoryAce(acl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, &low_level);
    ok(ret, "AddMandatoryAce failed with %lu\n", GetLastError());

    ret = GetAce(acl, 0, (void **)&ace);
    ok(ret, "got error %lu\n", GetLastError());
    ok(ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE, "got type %#x\n", ace->Header.AceType);
    ok(!ace->Header.AceFlags, "got flags %#x\n", ace->Header.AceFlags);
    ok(ace->Mask == SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, "got mask %#lx\n", ace->Mask);
    ok(EqualSid(&ace->SidStart, &low_level), "wrong sid\n");

    SetLastError(0xdeadbeef);
    ret = GetAce(acl, 1, (void **)&ace);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    ret = SetSecurityDescriptorSacl(sd, TRUE, acl, FALSE);
    ok(ret, "SetSecurityDescriptorSacl failed with error %lu\n", GetLastError());

    handle = CreateEventA(&sa, TRUE, TRUE, "test_event");
    ok(handle != NULL, "CreateEventA failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %u, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    sacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorSacl(sd2, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "SACL not present\n");
    ok(sacl != (void *)0xdeadbeef, "SACL not set\n");
    ok(!defaulted, "SACL defaulted\n");
    ret = GetAclInformation(sacl, &acl_size_info, sizeof(acl_size_info), AclSizeInformation);
    ok(ret, "GetAclInformation failed with error %lu\n", GetLastError());
    ok(acl_size_info.AceCount == 1, "SACL contains an unexpected ACE count %lu\n", acl_size_info.AceCount);

    ret = GetAce(sacl, 0, (void **)&ace);
    ok(ret, "GetAce failed with error %lu\n", GetLastError());
    ok (ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE, "Unexpected ACE type %#x\n", ace->Header.AceType);
    ok(!ace->Header.AceFlags, "Unexpected ACE flags %#x\n", ace->Header.AceFlags);
    ok(ace->Mask == SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, "Unexpected ACE mask %#lx\n", ace->Mask);
    ok(EqualSid(&ace->SidStart, &low_level), "Expected low integrity level\n");

    free(sd2);

    ret = pAddMandatoryAce(acl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP, &medium_level);
    ok(ret, "AddMandatoryAce failed with error %lu\n", GetLastError());

    ret = SetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd);
    ok(ret, "SetKernelObjectSecurity failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %u, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    sacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorSacl(sd2, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "SACL not present\n");
    ok(sacl != (void *)0xdeadbeef, "SACL not set\n");
    ok(sacl->AceCount == 2, "Expected 2 ACEs, got %d\n", sacl->AceCount);
    ok(!defaulted, "SACL defaulted\n");

    ret = GetAce(acl, 0, (void **)&ace);
    ok(ret, "got error %lu\n", GetLastError());
    ok(ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE, "got type %#x\n", ace->Header.AceType);
    ok(!ace->Header.AceFlags, "got flags %#x\n", ace->Header.AceFlags);
    ok(ace->Mask == SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, "got mask %#lx\n", ace->Mask);
    ok(EqualSid(&ace->SidStart, &low_level), "wrong sid\n");

    ret = GetAce(acl, 1, (void **)&ace);
    ok(ret, "got error %lu\n", GetLastError());
    ok(ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE, "got type %#x\n", ace->Header.AceType);
    ok(!ace->Header.AceFlags, "got flags %#x\n", ace->Header.AceFlags);
    ok(ace->Mask == SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP, "got mask %#lx\n", ace->Mask);
    ok(EqualSid(&ace->SidStart, &medium_level), "wrong sid\n");

    SetLastError(0xdeadbeef);
    ret = GetAce(acl, 2, (void **)&ace);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    free(sd2);

    ret = SetSecurityDescriptorSacl(sd, FALSE, NULL, FALSE);
    ok(ret, "SetSecurityDescriptorSacl failed with error %lu\n", GetLastError());

    ret = SetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd);
    ok(ret, "SetKernelObjectSecurity failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    sacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorSacl(sd2, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "SACL not present\n");
    ok(sacl && sacl != (void *)0xdeadbeef, "SACL not set\n");
    ok(!defaulted, "SACL defaulted\n");
    ok(!sacl->AceCount, "SACL contains an unexpected ACE count %u\n", sacl->AceCount);

    free(sd2);

    ret = InitializeAcl(acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with error %lu\n", GetLastError());

    ret = pAddMandatoryAce(acl, ACL_REVISION3, 0, SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP, &medium_level);
    ok(ret, "AddMandatoryAce failed with error %lu\n", GetLastError());

    ret = SetSecurityDescriptorSacl(sd, TRUE, acl, FALSE);
    ok(ret, "SetSecurityDescriptorSacl failed with error %lu\n", GetLastError());

    ret = SetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd);
    ok(ret, "SetKernelObjectSecurity failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    sacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorSacl(sd2, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "SACL not present\n");
    ok(sacl != (void *)0xdeadbeef, "SACL not set\n");
    ok(sacl->AclRevision == ACL_REVISION3, "Expected revision 3, got %d\n", sacl->AclRevision);
    ok(!defaulted, "SACL defaulted\n");

    free(sd2);

    ret = InitializeAcl(acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with error %lu\n", GetLastError());

    ret = AllocateAndInitializeSid(&sia_world, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void **)&everyone);
    ok(ret, "AllocateAndInitializeSid failed with error %lu\n", GetLastError());

    ret = AddAccessAllowedAce(acl, ACL_REVISION, KEY_READ, everyone);
    ok(ret, "AddAccessAllowedAce failed with error %lu\n", GetLastError());

    ret = SetSecurityDescriptorSacl(sd, TRUE, acl, FALSE);
    ok(ret, "SetSecurityDescriptorSacl failed with error %lu\n", GetLastError());

    ret = SetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd);
    ok(ret, "SetKernelObjectSecurity failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(handle, LABEL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    sacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorSacl(sd2, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "SACL not present\n");
    ok(sacl && sacl != (void *)0xdeadbeef, "SACL not set\n");
    ok(!defaulted, "SACL defaulted\n");
    ok(!sacl->AceCount, "SACL contains an unexpected ACE count %u\n", sacl->AceCount);

    FreeSid(everyone);
    free(sd2);
    CloseHandle(handle);
}

static void test_system_security_access(void)
{
    static const WCHAR testkeyW[] =
        {'S','O','F','T','W','A','R','E','\\','W','i','n','e','\\','S','A','C','L','t','e','s','t',0};
    LONG res;
    HKEY hkey;
    PSECURITY_DESCRIPTOR sd;
    ACL *sacl;
    DWORD err, len = 128;
    TOKEN_PRIVILEGES priv, *priv_prev;
    HANDLE token;
    LUID luid;
    BOOL ret;

    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &token )) return;
    if (!LookupPrivilegeValueA( NULL, SE_SECURITY_NAME, &luid ))
    {
        CloseHandle( token );
        return;
    }

    /* ACCESS_SYSTEM_SECURITY requires special privilege */
    res = RegCreateKeyExW( HKEY_LOCAL_MACHINE, testkeyW, 0, NULL, 0, KEY_READ|ACCESS_SYSTEM_SECURITY, NULL, &hkey, NULL );
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "unprivileged user\n" );
        CloseHandle( token );
        return;
    }
    todo_wine ok( res == ERROR_PRIVILEGE_NOT_HELD, "got %ld\n", res );

    priv.PrivilegeCount = 1;
    priv.Privileges[0].Luid = luid;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    priv_prev = malloc( len );
    ret = AdjustTokenPrivileges( token, FALSE, &priv, len, priv_prev, &len );
    ok( ret, "got %lu\n", GetLastError());

    res = RegCreateKeyExW( HKEY_LOCAL_MACHINE, testkeyW, 0, NULL, 0, KEY_READ|ACCESS_SYSTEM_SECURITY, NULL, &hkey, NULL );
    if (res == ERROR_PRIVILEGE_NOT_HELD)
    {
        win_skip( "privilege not held\n" );
        free( priv_prev );
        CloseHandle( token );
        return;
    }
    ok( !res, "got %ld\n", res );

    /* restore privileges */
    ret = AdjustTokenPrivileges( token, FALSE, priv_prev, 0, NULL, NULL );
    ok( ret, "got %lu\n", GetLastError() );
    free( priv_prev );

    /* privilege is checked on access */
    err = GetSecurityInfo( hkey, SE_REGISTRY_KEY, SACL_SECURITY_INFORMATION, NULL, NULL, NULL, &sacl, &sd );
    todo_wine ok( err == ERROR_PRIVILEGE_NOT_HELD || err == ERROR_ACCESS_DENIED, "got %lu\n", err );
    if (err == ERROR_SUCCESS)
        LocalFree( sd );

    priv.PrivilegeCount = 1;
    priv.Privileges[0].Luid = luid;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    priv_prev = malloc( len );
    ret = AdjustTokenPrivileges( token, FALSE, &priv, len, priv_prev, &len );
    ok( ret, "got %lu\n", GetLastError());

    err = GetSecurityInfo( hkey, SE_REGISTRY_KEY, SACL_SECURITY_INFORMATION, NULL, NULL, NULL, &sacl, &sd );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    RegCloseKey( hkey );
    LocalFree( sd );

    /* handle created without ACCESS_SYSTEM_SECURITY, privilege held */
    res = RegCreateKeyExW( HKEY_LOCAL_MACHINE, testkeyW, 0, NULL, 0, KEY_READ, NULL, &hkey, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    sd = NULL;
    err = GetSecurityInfo( hkey, SE_REGISTRY_KEY, SACL_SECURITY_INFORMATION, NULL, NULL, NULL, &sacl, &sd );
    todo_wine ok( err == ERROR_SUCCESS, "got %lu\n", err );
    RegCloseKey( hkey );
    LocalFree( sd );

    /* restore privileges */
    ret = AdjustTokenPrivileges( token, FALSE, priv_prev, 0, NULL, NULL );
    ok( ret, "got %lu\n", GetLastError() );
    free( priv_prev );

    /* handle created without ACCESS_SYSTEM_SECURITY, privilege not held */
    res = RegCreateKeyExW( HKEY_LOCAL_MACHINE, testkeyW, 0, NULL, 0, KEY_READ, NULL, &hkey, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    err = GetSecurityInfo( hkey, SE_REGISTRY_KEY, SACL_SECURITY_INFORMATION, NULL, NULL, NULL, &sacl, &sd );
    ok( err == ERROR_PRIVILEGE_NOT_HELD || err == ERROR_ACCESS_DENIED, "got %lu\n", err );
    RegCloseKey( hkey );

    res = RegDeleteKeyW( HKEY_LOCAL_MACHINE, testkeyW );
    ok( !res, "got %ld\n", res );
    CloseHandle( token );
}

static void test_GetWindowsAccountDomainSid(void)
{
#ifdef __REACTOS__
    char *user, buffer1[SECURITY_MAX_SID_SIZE];
    PSID domain_sid = (PSID *)&buffer1;
#else
    char *user, buffer1[SECURITY_MAX_SID_SIZE], buffer2[SECURITY_MAX_SID_SIZE];
    SID_IDENTIFIER_AUTHORITY domain_ident = { SECURITY_NT_AUTHORITY };
    PSID domain_sid = (PSID *)&buffer1;
    PSID domain_sid2 = (PSID *)&buffer2;
#endif
    DWORD sid_size;
    PSID user_sid;
    HANDLE token;
    BOOL bret = TRUE;
#ifndef __REACTOS__
    int i;
#endif

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }

    bret = GetTokenInformation(token, TokenUser, NULL, 0, &sid_size);
    ok(!bret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    user = malloc(sid_size);
    bret = GetTokenInformation(token, TokenUser, user, sid_size, &sid_size);
    ok(bret, "GetTokenInformation(TokenUser) failed with error %ld\n", GetLastError());
    CloseHandle(token);
    user_sid = ((TOKEN_USER *)user)->User.Sid;

    SetLastError(0xdeadbeef);
    bret = GetWindowsAccountDomainSid(0, 0, 0);
    ok(!bret, "GetWindowsAccountDomainSid succeeded\n");
    ok(GetLastError() == ERROR_INVALID_SID, "expected ERROR_INVALID_SID, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    bret = GetWindowsAccountDomainSid(user_sid, 0, 0);
    ok(!bret, "GetWindowsAccountDomainSid succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    sid_size = SECURITY_MAX_SID_SIZE;
    SetLastError(0xdeadbeef);
    bret = GetWindowsAccountDomainSid(user_sid, 0, &sid_size);
    ok(!bret, "GetWindowsAccountDomainSid succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    ok(sid_size == GetSidLengthRequired(4), "expected size %ld, got %ld\n", GetSidLengthRequired(4), sid_size);

    SetLastError(0xdeadbeef);
    bret = GetWindowsAccountDomainSid(user_sid, domain_sid, 0);
    ok(!bret, "GetWindowsAccountDomainSid succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    sid_size = 1;
    SetLastError(0xdeadbeef);
    bret = GetWindowsAccountDomainSid(user_sid, domain_sid, &sid_size);
    ok(!bret, "GetWindowsAccountDomainSid succeeded\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(sid_size == GetSidLengthRequired(4), "expected size %ld, got %ld\n", GetSidLengthRequired(4), sid_size);

    sid_size = SECURITY_MAX_SID_SIZE;
    bret = GetWindowsAccountDomainSid(user_sid, domain_sid, &sid_size);
    ok(bret, "GetWindowsAccountDomainSid failed with error %ld\n", GetLastError());
    ok(sid_size == GetSidLengthRequired(4), "expected size %ld, got %ld\n", GetSidLengthRequired(4), sid_size);
#ifndef __REACTOS__ // This crashes on WS03, Vista, Win7, and Win8.1.
    InitializeSid(domain_sid2, &domain_ident, 4);
    for (i = 0; i < 4; i++)
        *GetSidSubAuthority(domain_sid2, i) = *GetSidSubAuthority(user_sid, i);
    ok(EqualSid(domain_sid, domain_sid2), "unexpected domain sid %s != %s\n",
       debugstr_sid(domain_sid), debugstr_sid(domain_sid2));
#endif

    free(user);
}

static void test_GetSidIdentifierAuthority(void)
{
    char buffer[SECURITY_MAX_SID_SIZE];
    PSID authority_sid = (PSID *)buffer;
    PSID_IDENTIFIER_AUTHORITY id;
    BOOL ret;

    memset(buffer, 0xcc, sizeof(buffer));
    ret = IsValidSid(authority_sid);
    ok(!ret, "expected FALSE, got %u\n", ret);

    SetLastError(0xdeadbeef);
    id = GetSidIdentifierAuthority(authority_sid);
    ok(id != NULL, "got NULL pointer as identifier authority\n");
    ok(GetLastError() == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    id = GetSidIdentifierAuthority(NULL);
    ok(id != NULL, "got NULL pointer as identifier authority\n");
    ok(GetLastError() == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", GetLastError());
}

static void test_pseudo_tokens(void)
{
    TOKEN_STATISTICS statistics1, statistics2;
    HANDLE token;
    DWORD retlen;
    BOOL ret;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    ok(ret, "OpenProcessToken failed with error %lu\n", GetLastError());
    memset(&statistics1, 0x11, sizeof(statistics1));
    ret = GetTokenInformation(token, TokenStatistics, &statistics1, sizeof(statistics1), &retlen);
    ok(ret, "GetTokenInformation failed with %lu\n", GetLastError());
    CloseHandle(token);

    /* test GetCurrentProcessToken() */
    SetLastError(0xdeadbeef);
    memset(&statistics2, 0x22, sizeof(statistics2));
    ret = GetTokenInformation(GetCurrentProcessToken(), TokenStatistics,
                              &statistics2, sizeof(statistics2), &retlen);
    ok(ret || broken(GetLastError() == ERROR_INVALID_HANDLE),
       "GetTokenInformation failed with %lu\n", GetLastError());
    if (ret)
        ok(!memcmp(&statistics1, &statistics2, sizeof(statistics1)), "Token statistics do not match\n");
    else
        win_skip("CurrentProcessToken not supported, skipping test\n");

    /* test GetCurrentThreadEffectiveToken() */
    SetLastError(0xdeadbeef);
    memset(&statistics2, 0x22, sizeof(statistics2));
    ret = GetTokenInformation(GetCurrentThreadEffectiveToken(), TokenStatistics,
                              &statistics2, sizeof(statistics2), &retlen);
    ok(ret || broken(GetLastError() == ERROR_INVALID_HANDLE),
       "GetTokenInformation failed with %lu\n", GetLastError());
    if (ret)
        ok(!memcmp(&statistics1, &statistics2, sizeof(statistics1)), "Token statistics do not match\n");
    else
        win_skip("CurrentThreadEffectiveToken not supported, skipping test\n");

    SetLastError(0xdeadbeef);
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token);
    ok(!ret, "OpenThreadToken should have failed\n");
    ok(GetLastError() == ERROR_NO_TOKEN, "Expected ERROR_NO_TOKEN, got %lu\n", GetLastError());

    /* test GetCurrentThreadToken() */
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(GetCurrentThreadToken(), TokenStatistics,
                              &statistics2, sizeof(statistics2), &retlen);
    todo_wine ok(GetLastError() == ERROR_NO_TOKEN || broken(GetLastError() == ERROR_INVALID_HANDLE),
                 "Expected ERROR_NO_TOKEN, got %lu\n", GetLastError());
}

static void test_maximum_allowed(void)
{
    HANDLE (WINAPI *pCreateEventExA)(SECURITY_ATTRIBUTES *, LPCSTR, DWORD, DWORD);
    char buffer_sd[SECURITY_DESCRIPTOR_MIN_LENGTH], buffer_acl[256];
    SECURITY_DESCRIPTOR *sd = (SECURITY_DESCRIPTOR *)&buffer_sd;
    SECURITY_ATTRIBUTES sa;
    ACL *acl = (ACL *)&buffer_acl;
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    ACCESS_MASK mask;
    HANDLE handle;
    BOOL ret;

    pCreateEventExA = (void *)GetProcAddress(hkernel32, "CreateEventExA");
    if (!pCreateEventExA)
    {
        win_skip("CreateEventExA is not available\n");
        return;
    }

    ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(ret, "InitializeSecurityDescriptor failed with %lu\n", GetLastError());
    memset(buffer_acl, 0, sizeof(buffer_acl));
    ret = InitializeAcl(acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with %lu\n", GetLastError());
    ret = SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with %lu\n", GetLastError());

    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle       = FALSE;

    handle = pCreateEventExA(&sa, NULL, 0, MAXIMUM_ALLOWED | 0x4);
    ok(handle != NULL, "CreateEventExA failed with error %lu\n", GetLastError());
    mask = get_obj_access(handle);
    ok(mask == EVENT_ALL_ACCESS, "Expected %x, got %lx\n", EVENT_ALL_ACCESS, mask);
    CloseHandle(handle);
}

static void check_token_label(HANDLE token, DWORD *level, BOOL sacl_inherited)
{
    static SID medium_sid = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                             {SECURITY_MANDATORY_MEDIUM_RID}};
    static SID high_sid = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                           {SECURITY_MANDATORY_HIGH_RID}};
    SECURITY_DESCRIPTOR_CONTROL control;
    SYSTEM_MANDATORY_LABEL_ACE *ace;
    BOOL ret, present, defaulted;
    SECURITY_DESCRIPTOR *sd;
    ACL *sacl = NULL, *dacl;
    DWORD size, revision;
    char *str;
    SID *sid;

    ret = GetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd = malloc(size);
    ret = GetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, sd, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    ret = GetSecurityDescriptorControl(sd, &control, &revision);
    ok(ret, "GetSecurityDescriptorControl failed with error %lu\n", GetLastError());
    if (sacl_inherited)
        todo_wine ok(control == (SE_SELF_RELATIVE | SE_SACL_AUTO_INHERITED | SE_SACL_PRESENT),
                     "Unexpected security descriptor control %#x\n", control);
    else
        todo_wine ok(control == (SE_SELF_RELATIVE | SE_SACL_PRESENT),
                     "Unexpected security descriptor control %#x\n", control);
    ok(revision == 1, "Unexpected security descriptor revision %lu\n", revision);

    sid = (void *)0xdeadbeef;
    defaulted = TRUE;
    ret = GetSecurityDescriptorOwner(sd, (void **)&sid, &defaulted);
    ok(ret, "GetSecurityDescriptorOwner failed with error %lu\n", GetLastError());
    ok(!sid, "Owner present\n");
    ok(!defaulted, "Owner defaulted\n");

    sid = (void *)0xdeadbeef;
    defaulted = TRUE;
    ret = GetSecurityDescriptorGroup(sd, (void **)&sid, &defaulted);
    ok(ret, "GetSecurityDescriptorGroup failed with error %lu\n", GetLastError());
    ok(!sid, "Group present\n");
    ok(!defaulted, "Group defaulted\n");

    ret = GetSecurityDescriptorSacl(sd, &present, &sacl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "No SACL in the security descriptor\n");
    ok(!!sacl, "NULL SACL in the security descriptor\n");
    ok(!defaulted, "SACL defaulted\n");
    ok(sacl->AceCount == 1, "SACL contains an unexpected ACE count %u\n", sacl->AceCount);

    ret = GetAce(sacl, 0, (void **)&ace);
    ok(ret, "GetAce failed with error %lu\n", GetLastError());

    ok(ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE,
       "Unexpected ACE type %#x\n", ace->Header.AceType);
    ok(!ace->Header.AceFlags, "Unexpected ACE flags %#x\n", ace->Header.AceFlags);
    ok(ace->Header.AceSize, "Unexpected ACE size %u\n", ace->Header.AceSize);
    ok(ace->Mask == SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, "Unexpected ACE mask %#lx\n", ace->Mask);

    sid = (SID *)&ace->SidStart;
    ConvertSidToStringSidA(sid, &str);
    ok(EqualSid(sid, &medium_sid) || EqualSid(sid, &high_sid), "Got unexpected SID %s\n", str);
    *level = sid->SubAuthority[0];
    LocalFree(str);

    ret = GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl failed with error %lu\n", GetLastError());
    todo_wine ok(!present, "DACL present\n");

    free(sd);
}

static void test_token_label(void)
{
    SID low_sid = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                   {SECURITY_MANDATORY_LOW_RID}};
    char sacl_buffer[50];
    SECURITY_ATTRIBUTES attr = {.nLength = sizeof(SECURITY_ATTRIBUTES)};
    ACL *sacl = (ACL *)sacl_buffer;
    TOKEN_LINKED_TOKEN linked;
    DWORD level, level2, size;
    PSECURITY_DESCRIPTOR sd;
    HANDLE token, token2;
    BOOL ret;

    if (!pAddMandatoryAce)
    {
        win_skip("Mandatory integrity control is not supported.\n");
        return;
    }

    ret = OpenProcessToken(GetCurrentProcess(), READ_CONTROL | TOKEN_QUERY | TOKEN_DUPLICATE, &token);
    ok(ret, "OpenProcessToken failed with error %lu\n", GetLastError());

    check_token_label(token, &level, TRUE);

    ret = DuplicateTokenEx(token, READ_CONTROL, NULL, SecurityAnonymous, TokenPrimary, &token2);
    ok(ret, "Failed to duplicate token, error %lu\n", GetLastError());

    check_token_label(token2, &level2, TRUE);
    ok(level2 == level, "Expected level %#lx, got %#lx.\n", level, level2);

    CloseHandle(token2);

    ret = DuplicateTokenEx(token, READ_CONTROL, NULL, SecurityImpersonation, TokenImpersonation, &token2);
    ok(ret, "Failed to duplicate token, error %lu\n", GetLastError());

    check_token_label(token2, &level2, TRUE);
    ok(level2 == level, "Expected level %#lx, got %#lx.\n", level, level2);

    CloseHandle(token2);

    /* Any label set in the SD when calling DuplicateTokenEx() is ignored. */

    ret = GetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());

    sd = malloc(size);
    ret = GetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, sd, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    InitializeAcl(sacl, sizeof(sacl_buffer), ACL_REVISION);
#ifdef __REACTOS__
    pAddMandatoryAce(sacl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, &low_sid);
#else
    AddMandatoryAce(sacl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, &low_sid);
#endif
    SetSecurityDescriptorSacl(sd, TRUE, sacl, FALSE);

    attr.lpSecurityDescriptor = sd;
    ret = DuplicateTokenEx(token, TOKEN_ALL_ACCESS, &attr, SecurityImpersonation, TokenImpersonation, &token2);
    ok(ret, "Failed to duplicate token, error %lu\n", GetLastError());

    check_token_label(token2, &level2, TRUE);
    ok(level2 == level, "Expected level %#lx, got %#lx.\n", level, level2);

    /* Trying to set a SD on the token also claims success but has no effect. */

    ret = SetKernelObjectSecurity(token2, LABEL_SECURITY_INFORMATION, sd);
    ok(ret, "Failed to set SD, error %lu\n", GetLastError());

    check_token_label(token2, &level2, FALSE);
    ok(level2 == level, "Expected level %#lx, got %#lx.\n", level, level2);

    free(sd);

    /* Test the linked token. */

#ifdef __REACTOS__
    /* This test crashes on Vista and Win7 */
    if (GetNTVersion() == _WIN32_WINNT_VISTA || GetNTVersion() == _WIN32_WINNT_WIN7) {
        skip("Linked token tests crash on Vista and Win7.\n");
    }
    else {
#endif
    ret = GetTokenInformation(token, TokenLinkedToken, &linked, sizeof(linked), &size);
    ok(ret, "Failed to get linked token, error %lu\n", GetLastError());

    check_token_label(linked.LinkedToken, &level2, TRUE);
    ok(level2 == level, "Expected level %#lx, got %#lx.\n", level, level2);

    CloseHandle(linked.LinkedToken);
#ifdef __REACTOS__
    }
#endif

    CloseHandle(token);
}

static void test_token_security_descriptor(void)
{
    static SID low_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                            {SECURITY_MANDATORY_LOW_RID}};
    char buffer_sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_DESCRIPTOR *sd = (SECURITY_DESCRIPTOR *)&buffer_sd, *sd2;
    char buffer_acl[256], buffer[MAX_PATH];
    ACL *acl = (ACL *)&buffer_acl, *acl2, *acl_child;
    BOOL defaulted, present, ret, found;
    HANDLE token, token2, token3;
    EXPLICIT_ACCESSW exp_access;
    PROCESS_INFORMATION info;
    DWORD size, index, retd;
    ACCESS_ALLOWED_ACE *ace;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA startup;
    PSID psid;

    /* Test whether we can create tokens with security descriptors */
    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &token);
    ok(ret, "OpenProcessToken failed with error %lu\n", GetLastError());

    ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(ret, "InitializeSecurityDescriptor failed with error %lu\n", GetLastError());

    memset(buffer_acl, 0, sizeof(buffer_acl));
    ret = InitializeAcl(acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with error %lu\n", GetLastError());

    ret = ConvertStringSidToSidA("S-1-5-6", &psid);
    ok(ret, "ConvertStringSidToSidA failed with error %lu\n", GetLastError());

    ret = AddAccessAllowedAceEx(acl, ACL_REVISION, NO_PROPAGATE_INHERIT_ACE, GENERIC_ALL, psid);
    ok(ret, "AddAccessAllowedAceEx failed with error %lu\n", GetLastError());

    ret = SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %lu\n", GetLastError());

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle = FALSE;

    ret = DuplicateTokenEx(token, MAXIMUM_ALLOWED, &sa, SecurityImpersonation, TokenImpersonation, &token2);
    ok(ret, "DuplicateTokenEx failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(token2, DACL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(token2, DACL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    acl2 = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorDacl(sd2, &present, &acl2, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl failed with error %lu\n", GetLastError());
    ok(present, "acl2 not present\n");
    ok(acl2 != (void *)0xdeadbeef, "acl2 not set\n");
    ok(acl2->AceCount == 1, "Expected 1 ACE, got %d\n", acl2->AceCount);
    ok(!defaulted, "acl2 defaulted\n");

    ret = GetAce(acl2, 0, (void **)&ace);
    ok(ret, "GetAce failed with error %lu\n", GetLastError());
    ok(ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE, "Unexpected ACE type %#x\n", ace->Header.AceType);
    ok(EqualSid(&ace->SidStart, psid), "Expected access allowed ACE\n");
    ok(ace->Header.AceFlags == NO_PROPAGATE_INHERIT_ACE,
       "Expected NO_PROPAGATE_INHERIT_ACE as flags, got %x\n", ace->Header.AceFlags);

    free(sd2);

    /* Duplicate token without security attributes.
     * Tokens do not inherit the security descriptor in DuplicateToken. */
    ret = DuplicateTokenEx(token2, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenImpersonation, &token3);
    ok(ret, "DuplicateTokenEx failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(token3, DACL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(token3, DACL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    acl2 = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorDacl(sd2, &present, &acl2, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl failed with error %lu\n", GetLastError());
    ok(present, "DACL not present\n");

    ok(acl2 != (void *)0xdeadbeef, "DACL not set\n");
    ok(!defaulted, "DACL defaulted\n");

    index = 0;
    found = FALSE;
    while (GetAce(acl2, index++, (void **)&ace))
    {
        if (ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE && EqualSid(&ace->SidStart, psid))
            found = TRUE;
    }
    ok(!found, "Access allowed ACE was inherited\n");

    free(sd2);

    /* When creating a child process, the process does inherit the token of
     * the parent but not the DACL of the token */
    ret = GetKernelObjectSecurity(token, DACL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd2 = malloc(size);
    ret = GetKernelObjectSecurity(token, DACL_SECURITY_INFORMATION, sd2, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    acl2 = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorDacl(sd2, &present, &acl2, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl failed with error %lu\n", GetLastError());
    ok(present, "DACL not present\n");
    ok(acl2 != (void *)0xdeadbeef, "DACL not set\n");
    ok(!defaulted, "DACL defaulted\n");

    exp_access.grfAccessPermissions = GENERIC_ALL;
    exp_access.grfAccessMode = GRANT_ACCESS;
    exp_access.grfInheritance = NO_PROPAGATE_INHERIT_ACE;
    exp_access.Trustee.pMultipleTrustee = NULL;
    exp_access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    exp_access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    exp_access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    exp_access.Trustee.ptstrName = (void*)psid;

    retd = SetEntriesInAclW(1, &exp_access, acl2, &acl_child);
    ok(retd == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", retd);

    memset(sd, 0, sizeof(buffer_sd));
    ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(ret, "InitializeSecurityDescriptor failed with error %lu\n", GetLastError());

    ret = SetSecurityDescriptorDacl(sd, TRUE, acl_child, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %lu\n", GetLastError());

    ret = SetKernelObjectSecurity(token, DACL_SECURITY_INFORMATION, sd);
    ok(ret, "SetKernelObjectSecurity failed with error %lu\n", GetLastError());

    /* The security label is also not inherited */
    if (pAddMandatoryAce)
    {
        ret = InitializeAcl(acl, 256, ACL_REVISION);
        ok(ret, "InitializeAcl failed with error %lu\n", GetLastError());

        ret = pAddMandatoryAce(acl, ACL_REVISION, 0, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP, &low_level);
        ok(ret, "AddMandatoryAce failed with error %lu\n", GetLastError());

        memset(sd, 0, sizeof(buffer_sd));
        ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
        ok(ret, "InitializeSecurityDescriptor failed with error %lu\n", GetLastError());

        ret = SetSecurityDescriptorSacl(sd, TRUE, acl, FALSE);
        ok(ret, "SetSecurityDescriptorSacl failed with error %lu\n", GetLastError());

        ret = SetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, sd);
        ok(ret, "SetKernelObjectSecurity failed with error %lu\n", GetLastError());
    }
    else
        win_skip("SYSTEM_MANDATORY_LABEL not supported\n");

    /* Start child process with our modified token */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(buffer, "%s security test_token_sd", myARGV[0]);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess failed with error %lu\n", GetLastError());
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    LocalFree(acl_child);
    free(sd2);
    LocalFree(psid);

    CloseHandle(token3);
    CloseHandle(token2);
    CloseHandle(token);
}

static void test_child_token_sd(void)
{
    static SID low_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                            {SECURITY_MANDATORY_LOW_RID}};
    SYSTEM_MANDATORY_LABEL_ACE *ace_label;
    BOOL ret, present, defaulted;
    ACCESS_ALLOWED_ACE *acc_ace;
    SECURITY_DESCRIPTOR *sd;
    DWORD size, i;
    HANDLE token;
    PSID psid;
    ACL *acl;

    ret = ConvertStringSidToSidA("S-1-5-6", &psid);
    ok(ret, "ConvertStringSidToSidA failed with error %lu\n", GetLastError());

    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &token);
    ok(ret, "OpenProcessToken failed with error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(token, DACL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd = malloc(size);
    ret = GetKernelObjectSecurity(token, DACL_SECURITY_INFORMATION, sd, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    acl = NULL;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorDacl(sd, &present, &acl, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl failed with error %lu\n", GetLastError());
    ok(present, "DACL not present\n");
    ok(acl && acl != (void *)0xdeadbeef, "Got invalid DACL\n");
    ok(!defaulted, "DACL defaulted\n");

    ok(acl->AceCount, "Expected at least one ACE\n");
    for (i = 0; i < acl->AceCount; i++)
    {
        ret = GetAce(acl, i, (void **)&acc_ace);
        ok(ret, "GetAce failed with error %lu\n", GetLastError());
        ok(acc_ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE || !EqualSid(&acc_ace->SidStart, psid),
           "ACE inherited from the parent\n");
    }

    LocalFree(psid);
    free(sd);

    if (!pAddMandatoryAce)
    {
        win_skip("SYSTEM_MANDATORY_LABEL not supported\n");
        return;
    }

    ret = GetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Unexpected GetKernelObjectSecurity return value %d, error %lu\n", ret, GetLastError());

    sd = malloc(size);
    ret = GetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION, sd, size, &size);
    ok(ret, "GetKernelObjectSecurity failed with error %lu\n", GetLastError());

    acl = NULL;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorSacl(sd, &present, &acl, &defaulted);
    ok(ret, "GetSecurityDescriptorSacl failed with error %lu\n", GetLastError());
    ok(present, "SACL not present\n");
    ok(acl && acl != (void *)0xdeadbeef, "Got invalid SACL\n");
    ok(!defaulted, "SACL defaulted\n");
    ok(acl->AceCount == 1, "Expected exactly one ACE\n");
    ret = GetAce(acl, 0, (void **)&ace_label);
    ok(ret, "GetAce failed with error %lu\n", GetLastError());
    ok(ace_label->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE,
       "Unexpected ACE type %#x\n", ace_label->Header.AceType);
    ok(!EqualSid(&ace_label->SidStart, &low_level),
       "Low integrity level should not have been inherited\n");

    free(sd);
}

static void test_GetExplicitEntriesFromAclW(void)
{
    static const WCHAR wszCurrentUser[] = { 'C','U','R','R','E','N','T','_','U','S','E','R','\0'};
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    PSID everyone_sid = NULL, users_sid = NULL;
    EXPLICIT_ACCESSW access;
    EXPLICIT_ACCESSW *access2;
    PACL new_acl, old_acl = NULL;
    ULONG count;
    DWORD res;

    old_acl = malloc(256);
    res = InitializeAcl(old_acl, 256, ACL_REVISION);
    ok(res, "InitializeAcl failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone_sid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &users_sid);
    ok(res, "AllocateAndInitializeSid failed with error %ld\n", GetLastError());

    res = AddAccessAllowedAce(old_acl, ACL_REVISION, KEY_READ, users_sid);
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());

    access2 = NULL;
    res = GetExplicitEntriesFromAclW(old_acl, &count, &access2);
    ok(res == ERROR_SUCCESS, "GetExplicitEntriesFromAclW failed with error %ld\n", GetLastError());
    ok(count == 1, "Expected count == 1, got %ld\n", count);
    ok(access2[0].grfAccessMode == GRANT_ACCESS, "Expected GRANT_ACCESS, got %d\n", access2[0].grfAccessMode);
    ok(access2[0].grfAccessPermissions == KEY_READ, "Expected KEY_READ, got %ld\n", access2[0].grfAccessPermissions);
    ok(access2[0].Trustee.TrusteeForm == TRUSTEE_IS_SID, "Expected SID trustee, got %d\n", access2[0].Trustee.TrusteeForm);
    ok(access2[0].grfInheritance == NO_INHERITANCE, "Expected NO_INHERITANCE, got %lx\n", access2[0].grfInheritance);
    ok(EqualSid(access2[0].Trustee.ptstrName, users_sid), "Expected equal SIDs\n");
    LocalFree(access2);

    access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    access.Trustee.pMultipleTrustee = NULL;

    access.grfAccessPermissions = KEY_WRITE;
    access.grfAccessMode = GRANT_ACCESS;
    access.grfInheritance = NO_INHERITANCE;
    access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    access.Trustee.ptstrName = everyone_sid;
    res = SetEntriesInAclW(1, &access, old_acl, &new_acl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(new_acl != NULL, "returned acl was NULL\n");

    access2 = NULL;
    res = GetExplicitEntriesFromAclW(new_acl, &count, &access2);
    ok(res == ERROR_SUCCESS, "GetExplicitEntriesFromAclW failed with error %ld\n", GetLastError());
    ok(count == 2, "Expected count == 2, got %ld\n", count);
    ok(access2[0].grfAccessMode == GRANT_ACCESS, "Expected GRANT_ACCESS, got %d\n", access2[0].grfAccessMode);
    ok(access2[0].grfAccessPermissions == KEY_WRITE, "Expected KEY_WRITE, got %ld\n", access2[0].grfAccessPermissions);
    ok(access2[0].Trustee.TrusteeType == TRUSTEE_IS_UNKNOWN,
       "Expected TRUSTEE_IS_UNKNOWN trustee type, got %d\n", access2[0].Trustee.TrusteeType);
    ok(access2[0].Trustee.TrusteeForm == TRUSTEE_IS_SID, "Expected SID trustee, got %d\n", access2[0].Trustee.TrusteeForm);
    ok(access2[0].grfInheritance == NO_INHERITANCE, "Expected NO_INHERITANCE, got %lx\n", access2[0].grfInheritance);
    ok(EqualSid(access2[0].Trustee.ptstrName, everyone_sid), "Expected equal SIDs\n");
    LocalFree(access2);
    LocalFree(new_acl);

    access.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    res = SetEntriesInAclW(1, &access, old_acl, &new_acl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(new_acl != NULL, "returned acl was NULL\n");

    access2 = NULL;
    res = GetExplicitEntriesFromAclW(new_acl, &count, &access2);
    ok(res == ERROR_SUCCESS, "GetExplicitEntriesFromAclW failed with error %ld\n", GetLastError());
    ok(count == 2, "Expected count == 2, got %ld\n", count);
    ok(access2[0].grfAccessMode == GRANT_ACCESS, "Expected GRANT_ACCESS, got %d\n", access2[0].grfAccessMode);
    ok(access2[0].grfAccessPermissions == KEY_WRITE, "Expected KEY_WRITE, got %ld\n", access2[0].grfAccessPermissions);
    ok(access2[0].Trustee.TrusteeType == TRUSTEE_IS_UNKNOWN,
       "Expected TRUSTEE_IS_UNKNOWN trustee type, got %d\n", access2[0].Trustee.TrusteeType);
    ok(access2[0].Trustee.TrusteeForm == TRUSTEE_IS_SID, "Expected SID trustee, got %d\n", access2[0].Trustee.TrusteeForm);
    ok(access2[0].grfInheritance == NO_INHERITANCE, "Expected NO_INHERITANCE, got %lx\n", access2[0].grfInheritance);
    ok(EqualSid(access2[0].Trustee.ptstrName, everyone_sid), "Expected equal SIDs\n");
    LocalFree(access2);
    LocalFree(new_acl);

    access.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    access.Trustee.ptstrName = (LPWSTR)wszCurrentUser;
    res = SetEntriesInAclW(1, &access, old_acl, &new_acl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(new_acl != NULL, "returned acl was NULL\n");

    access2 = NULL;
    res = GetExplicitEntriesFromAclW(new_acl, &count, &access2);
    ok(res == ERROR_SUCCESS, "GetExplicitEntriesFromAclW failed with error %ld\n", GetLastError());
    ok(count == 2, "Expected count == 2, got %ld\n", count);
    ok(access2[0].grfAccessMode == GRANT_ACCESS, "Expected GRANT_ACCESS, got %d\n", access2[0].grfAccessMode);
    ok(access2[0].grfAccessPermissions == KEY_WRITE, "Expected KEY_WRITE, got %ld\n", access2[0].grfAccessPermissions);
    ok(access2[0].Trustee.TrusteeType == TRUSTEE_IS_UNKNOWN,
       "Expected TRUSTEE_IS_UNKNOWN trustee type, got %d\n", access2[0].Trustee.TrusteeType);
    ok(access2[0].Trustee.TrusteeForm == TRUSTEE_IS_SID, "Expected SID trustee, got %d\n", access2[0].Trustee.TrusteeForm);
    ok(access2[0].grfInheritance == NO_INHERITANCE, "Expected NO_INHERITANCE, got %lx\n", access2[0].grfInheritance);
    LocalFree(access2);
    LocalFree(new_acl);

    access.grfAccessMode = REVOKE_ACCESS;
    access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    access.Trustee.ptstrName = users_sid;
    res = SetEntriesInAclW(1, &access, old_acl, &new_acl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(new_acl != NULL, "returned acl was NULL\n");

    access2 = (void *)0xdeadbeef;
    res = GetExplicitEntriesFromAclW(new_acl, &count, &access2);
    ok(res == ERROR_SUCCESS, "GetExplicitEntriesFromAclW failed with error %ld\n", GetLastError());
    ok(count == 0, "Expected count == 0, got %ld\n", count);
    ok(access2 == NULL, "access2 was not NULL\n");
    LocalFree(new_acl);

    /* Make the ACL both Allow and Deny Everyone. */
    res = AddAccessAllowedAce(old_acl, ACL_REVISION, KEY_READ, everyone_sid);
    ok(res, "AddAccessAllowedAce failed with error %ld\n", GetLastError());
    res = AddAccessDeniedAce(old_acl, ACL_REVISION, KEY_WRITE, everyone_sid);
    ok(res, "AddAccessDeniedAce failed with error %ld\n", GetLastError());
    /* Revoke Everyone. */
    access.Trustee.ptstrName = everyone_sid;
    access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    access.grfAccessPermissions = 0;
    new_acl = NULL;
    res = SetEntriesInAclW(1, &access, old_acl, &new_acl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %lu\n", res);
    ok(new_acl != NULL, "returned acl was NULL\n");
    /* Deny Everyone should remain (along with Grant Users from earlier). */
    access2 = NULL;
    res = GetExplicitEntriesFromAclW(new_acl, &count, &access2);
    ok(res == ERROR_SUCCESS, "GetExplicitEntriesFromAclW failed with error %ld\n", GetLastError());
    ok(count == 2, "Expected count == 2, got %ld\n", count);
#ifdef __REACTOS__
    if (!access2) {
        ok(FALSE, "FIXME: access2 should not be null!\n"); // Happens on ReactOS currently
    } else {
#endif
    ok(access2[0].grfAccessMode == GRANT_ACCESS, "Expected GRANT_ACCESS, got %d\n", access2[0].grfAccessMode);
    ok(access2[0].grfAccessPermissions == KEY_READ , "Expected KEY_READ, got %ld\n", access2[0].grfAccessPermissions);
    ok(EqualSid(access2[0].Trustee.ptstrName, users_sid), "Expected equal SIDs\n");
    ok(access2[1].grfAccessMode == DENY_ACCESS, "Expected DENY_ACCESS, got %d\n", access2[1].grfAccessMode);
    ok(access2[1].grfAccessPermissions == KEY_WRITE, "Expected KEY_WRITE, got %ld\n", access2[1].grfAccessPermissions);
    ok(EqualSid(access2[1].Trustee.ptstrName, everyone_sid), "Expected equal SIDs\n");
    LocalFree(access2);
#ifdef __REACTOS__
    }
#endif

    FreeSid(users_sid);
    FreeSid(everyone_sid);
    free(old_acl);
}

static void test_BuildSecurityDescriptorW(void)
{
    SECURITY_DESCRIPTOR old_sd, *new_sd, *rel_sd;
    ULONG new_sd_size;
    DWORD buf_size;
    char buf[1024];
    BOOL success;
    DWORD ret;

    InitializeSecurityDescriptor(&old_sd, SECURITY_DESCRIPTOR_REVISION);

    buf_size = sizeof(buf);
    rel_sd = (SECURITY_DESCRIPTOR *)buf;
    success = MakeSelfRelativeSD(&old_sd, rel_sd, &buf_size);
    ok(success, "MakeSelfRelativeSD failed with %lu\n", GetLastError());

    new_sd = NULL;
    new_sd_size = 0;
    ret = BuildSecurityDescriptorW(NULL, NULL, 0, NULL, 0, NULL, NULL, &new_sd_size, (void **)&new_sd);
    ok(ret == ERROR_SUCCESS, "BuildSecurityDescriptor failed with %lu\n", ret);
    ok(new_sd != NULL, "expected new_sd != NULL\n");
    LocalFree(new_sd);

    new_sd = (void *)0xdeadbeef;
    ret = BuildSecurityDescriptorW(NULL, NULL, 0, NULL, 0, NULL, &old_sd, &new_sd_size, (void **)&new_sd);
    ok(ret == ERROR_INVALID_SECURITY_DESCR, "expected ERROR_INVALID_SECURITY_DESCR, got %lu\n", ret);
    ok(new_sd == (void *)0xdeadbeef, "expected new_sd == 0xdeadbeef, got %p\n", new_sd);

    new_sd = NULL;
    new_sd_size = 0;
    ret = BuildSecurityDescriptorW(NULL, NULL, 0, NULL, 0, NULL, rel_sd, &new_sd_size, (void **)&new_sd);
    ok(ret == ERROR_SUCCESS, "BuildSecurityDescriptor failed with %lu\n", ret);
    ok(new_sd != NULL, "expected new_sd != NULL\n");
    LocalFree(new_sd);
}

static void test_EqualDomainSid(void)
{
    SID_IDENTIFIER_AUTHORITY ident = { SECURITY_NT_AUTHORITY };
    char sid_buffer[SECURITY_MAX_SID_SIZE], sid_buffer2[SECURITY_MAX_SID_SIZE];
    PSID domainsid, sid = sid_buffer, sid2 = sid_buffer2;
    DWORD size;
    BOOL ret, equal;
    unsigned int i;

    ret = AllocateAndInitializeSid(&ident, 6, SECURITY_NT_NON_UNIQUE, 12, 23, 34, 45, 56, 0, 0, &domainsid);
    ok(ret, "AllocateAndInitializeSid error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EqualDomainSid(NULL, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_SID, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EqualDomainSid(domainsid, domainsid, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(well_known_sid_values); i++)
    {
        SID *pisid = sid;

        size = sizeof(sid_buffer);
        if (!CreateWellKnownSid(i, NULL, sid, &size))
        {
            trace("Well known SID %u not supported\n", i);
            continue;
        }

        equal = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = EqualDomainSid(sid, domainsid, &equal);
        if (pisid->SubAuthority[0] != SECURITY_BUILTIN_DOMAIN_RID)
        {
            ok(!ret, "%u: got %d\n", i, ret);
            ok(GetLastError() == ERROR_NON_DOMAIN_SID, "%u: got %lu\n", i, GetLastError());
            ok(equal == 0xdeadbeef, "%u: got %d\n", i, equal);
            continue;
        }

        ok(ret, "%u: got %d\n", i, ret);
        ok(GetLastError() == 0, "%u: got %lu\n", i, GetLastError());
        ok(equal == 0, "%u: got %d\n", i, equal);

        size = sizeof(sid_buffer2);
        ret = CreateWellKnownSid(i, well_known_sid_values[i].without_domain ? NULL : domainsid, sid2, &size);
        ok(ret, "%u: CreateWellKnownSid error %lu\n", i, GetLastError());

        equal = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = EqualDomainSid(sid, sid2, &equal);
        ok(ret, "%u: got %d\n", i, ret);
        ok(GetLastError() == 0, "%u: got %lu\n", i, GetLastError());
        ok(equal == 1, "%u: got %d\n", i, equal);
    }

    FreeSid(domainsid);
}

static DWORD WINAPI duplicate_handle_access_thread(void *arg)
{
    HANDLE event = arg, event2;
    BOOL ret;

    event2 = OpenEventA(SYNCHRONIZE, FALSE, "test_dup");
    ok(!!event2, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    event2 = OpenEventA(EVENT_MODIFY_STATE, FALSE, "test_dup");
    ok(!!event2, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    ret = DuplicateHandle(GetCurrentProcess(), event, GetCurrentProcess(),
            &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    return 0;
}

static void test_duplicate_handle_access(void)
{
    char acl_buffer[200], everyone_sid_buffer[100], local_sid_buffer[100], cmdline[300];
    HANDLE token, restricted, impersonation, all_event, sync_event, event2, thread;
    SECURITY_ATTRIBUTES sa = {.nLength = sizeof(sa)};
    SID *everyone_sid = (SID *)everyone_sid_buffer;
    SID *local_sid = (SID *)local_sid_buffer;
    ACL *acl = (ACL *)acl_buffer;
    SID_AND_ATTRIBUTES sid_attr;
    SECURITY_DESCRIPTOR sd;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    DWORD size;
    BOOL ret;

    /* DuplicateHandle() validates access against the calling thread's token and
     * the target process's token. It does *not* validate access against the
     * calling process's token, even if the calling thread is not impersonating.
     */

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &token);
    ok(ret, "got error %lu\n", GetLastError());

    size = sizeof(everyone_sid_buffer);
    ret = CreateWellKnownSid(WinWorldSid, NULL, everyone_sid, &size);
    ok(ret, "got error %lu\n", GetLastError());
    size = sizeof(local_sid_buffer);
    ret = CreateWellKnownSid(WinLocalSid, NULL, local_sid, &size);
    ok(ret, "got error %lu\n", GetLastError());

    InitializeAcl(acl, sizeof(acl_buffer), ACL_REVISION);
    ret = AddAccessAllowedAce(acl, ACL_REVISION, SYNCHRONIZE, everyone_sid);
    ok(ret, "got error %lu\n", GetLastError());
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    ret = AddAccessAllowedAce(acl, ACL_REVISION, EVENT_MODIFY_STATE, local_sid);
    ok(ret, "got error %lu\n", GetLastError());
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    ret = SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    sa.lpSecurityDescriptor = &sd;

    sid_attr.Sid = local_sid;
    sid_attr.Attributes = 0;
    ret = CreateRestrictedToken(token, 0, 1, &sid_attr, 0, NULL, 0, NULL, &restricted);
    ok(ret, "got error %lu\n", GetLastError());
    ret = DuplicateTokenEx(restricted, TOKEN_IMPERSONATE, NULL,
            SecurityImpersonation, TokenImpersonation, &impersonation);
    ok(ret, "got error %lu\n", GetLastError());

    all_event = CreateEventA(&sa, TRUE, TRUE, "test_dup");
    ok(!!all_event, "got error %lu\n", GetLastError());
    sync_event = OpenEventA(SYNCHRONIZE, FALSE, "test_dup");
    ok(!!sync_event, "got error %lu\n", GetLastError());

    event2 = OpenEventA(SYNCHRONIZE, FALSE, "test_dup");
    ok(!!event2, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    event2 = OpenEventA(EVENT_MODIFY_STATE, FALSE, "test_dup");
    ok(!!event2, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    ret = DuplicateHandle(GetCurrentProcess(), all_event, GetCurrentProcess(), &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    ret = DuplicateHandle(GetCurrentProcess(), sync_event, GetCurrentProcess(), &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    ret = SetThreadToken(NULL, impersonation);
    ok(ret, "got error %lu\n", GetLastError());

    thread = CreateThread(NULL, 0, duplicate_handle_access_thread, sync_event, 0, NULL);
    ret = WaitForSingleObject(thread, 1000);
    ok(!ret, "wait failed\n");

    event2 = OpenEventA(SYNCHRONIZE, FALSE, "test_dup");
    ok(!!event2, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    SetLastError(0xdeadbeef);
    event2 = OpenEventA(EVENT_MODIFY_STATE, FALSE, "test_dup");
    ok(!event2, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    ret = DuplicateHandle(GetCurrentProcess(), all_event, GetCurrentProcess(), &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(ret, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), sync_event, GetCurrentProcess(), &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    ret = RevertToSelf();
    ok(ret, "got error %lu\n", GetLastError());

    sprintf(cmdline, "%s security duplicate %Iu %lu %Iu", myARGV[0],
            (ULONG_PTR)sync_event, GetCurrentProcessId(), (ULONG_PTR)impersonation );
    ret = CreateProcessAsUserA(restricted, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "got error %lu\n", GetLastError());

    ret = DuplicateHandle(GetCurrentProcess(), all_event, pi.hProcess, &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), sync_event, pi.hProcess, &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(!ret, "wait failed\n");

    CloseHandle(impersonation);
    CloseHandle(restricted);
    CloseHandle(token);
    CloseHandle(sync_event);
    CloseHandle(all_event);
}

static void test_duplicate_handle_access_child(void)
{
    HANDLE event, event2, process, token;
    BOOL ret;

    event = (HANDLE)(ULONG_PTR)_atoi64(myARGV[3]);
    process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, atoi(myARGV[4]));
    ok(!!process, "failed to open process, error %lu\n", GetLastError());

    event2 = OpenEventA(SYNCHRONIZE, FALSE, "test_dup");
    ok(!!event2, "got error %lu\n", GetLastError());
    CloseHandle(event2);

    SetLastError(0xdeadbeef);
    event2 = OpenEventA(EVENT_MODIFY_STATE, FALSE, "test_dup");
    ok(!event2, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    ret = DuplicateHandle(process, event, process, &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(process, event, GetCurrentProcess(), &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    ret = DuplicateHandle(process, (HANDLE)(ULONG_PTR)_atoi64(myARGV[5]),
            GetCurrentProcess(), &token, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "failed to retrieve token, error %lu\n", GetLastError());
    ret = SetThreadToken(NULL, token);
    ok(ret, "failed to set thread token, error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(process, event, process, &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(process, event, GetCurrentProcess(), &event2, EVENT_MODIFY_STATE, FALSE, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());

    ret = RevertToSelf();
    ok(ret, "failed to revert, error %lu\n", GetLastError());
    CloseHandle(token);
    CloseHandle(process);
}

#define join_process(a) join_process_(__LINE__, a)
static void join_process_(int line, const PROCESS_INFORMATION *pi)
{
    DWORD ret = WaitForSingleObject(pi->hProcess, 1000);
    ok_(__FILE__, line)(!ret, "wait failed\n");
    CloseHandle(pi->hProcess);
    CloseHandle(pi->hThread);
}

static void test_create_process_token(void)
{
    char cmdline[300], acl_buffer[200], sid_buffer[100];
    SECURITY_ATTRIBUTES sa = {.nLength = sizeof(sa)};
    ACL *acl = (ACL *)acl_buffer;
    SID *sid = (SID *)sid_buffer;
    SID_AND_ATTRIBUTES sid_attr;
    HANDLE event, token, token2;
    PROCESS_INFORMATION pi;
    SECURITY_DESCRIPTOR sd;
    STARTUPINFOA si = {0};
    DWORD size;
    BOOL ret;

    size = sizeof(sid_buffer);
    ret = CreateWellKnownSid(WinLocalSid, NULL, sid, &size);
    ok(ret, "got error %lu\n", GetLastError());
    ret = InitializeAcl(acl, sizeof(acl_buffer), ACL_REVISION);
    ok(ret, "got error %lu\n", GetLastError());
    ret = AddAccessAllowedAce(acl, ACL_REVISION, EVENT_MODIFY_STATE, sid);
    ok(ret, "got error %lu\n", GetLastError());
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    ret = SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    sa.lpSecurityDescriptor = &sd;
    event = CreateEventA(&sa, TRUE, TRUE, "test_event");
    ok(!!event, "got error %lu\n", GetLastError());

    sprintf(cmdline, "%s security restricted 0", myARGV[0]);

#ifdef __REACTOS__
    /* This block creates test failures on WS03 */
    if (GetNTVersion() > _WIN32_WINNT_WS03) {
#endif
    ret = CreateProcessAsUserA(NULL, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "got error %lu\n", GetLastError());
    join_process(&pi);
#ifdef __REACTOS__
    }
#endif

    ret = CreateProcessAsUserA(GetCurrentProcessToken(), NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    todo_wine ok(!ret, "expected failure\n");
    todo_wine ok(GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError());
    if (ret) join_process(&pi);

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &token);
    ok(ret, "got error %lu\n", GetLastError());
    ret = CreateProcessAsUserA(token, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret || broken(GetLastError() == ERROR_ACCESS_DENIED) /* < 7 */, "got error %lu\n", GetLastError());
    if (ret) join_process(&pi);
    CloseHandle(token);

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    ok(ret, "got error %lu\n", GetLastError());
    ret = CreateProcessAsUserA(token, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());
    CloseHandle(token);

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ASSIGN_PRIMARY, &token);
    ok(ret, "got error %lu\n", GetLastError());
    ret = CreateProcessAsUserA(token, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());
    CloseHandle(token);

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE, &token);
    ok(ret, "got error %lu\n", GetLastError());

    ret = DuplicateTokenEx(token, TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, NULL,
            SecurityImpersonation, TokenImpersonation, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    ret = CreateProcessAsUserA(token2, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret || broken(GetLastError() == ERROR_BAD_TOKEN_TYPE) /* < 7 */, "got error %lu\n", GetLastError());
    if (ret) join_process(&pi);
    CloseHandle(token2);

    sprintf(cmdline, "%s security restricted 1", myARGV[0]);
    sid_attr.Sid = sid;
    sid_attr.Attributes = 0;
    ret = CreateRestrictedToken(token, 0, 1, &sid_attr, 0, NULL, 0, NULL, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    ret = CreateProcessAsUserA(token2, NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "got error %lu\n", GetLastError());
    join_process(&pi);
    CloseHandle(token2);

    CloseHandle(token);

    CloseHandle(event);
}

static void test_create_process_token_child(void)
{
    HANDLE event;

    SetLastError(0xdeadbeef);
    event = OpenEventA(EVENT_MODIFY_STATE, FALSE, "test_event");
    if (!atoi(myARGV[3]))
    {
        ok(!!event, "got error %lu\n", GetLastError());
        CloseHandle(event);
    }
    else
    {
        ok(!event, "expected failure\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());
    }
}

static void test_pseudo_handle_security(void)
{
    char buffer[200];
    PSECURITY_DESCRIPTOR sd = buffer, sd_ptr;
    unsigned int i;
    DWORD size;
    BOOL ret;

    static const HKEY keys[] =
    {
        HKEY_CLASSES_ROOT,
        HKEY_CURRENT_USER,
        HKEY_LOCAL_MACHINE,
        HKEY_USERS,
        HKEY_PERFORMANCE_DATA,
        HKEY_CURRENT_CONFIG,
        HKEY_DYN_DATA,
    };

#ifdef __REACTOS__
    ret = GetKernelObjectSecurity(GetCurrentProcess(), OWNER_SECURITY_INFORMATION, sd, sizeof(buffer), &size);
#else
    ret = GetKernelObjectSecurity(GetCurrentProcess(), OWNER_SECURITY_INFORMATION, &sd, sizeof(buffer), &size);
#endif
    ok(ret, "got error %lu\n", GetLastError());

#ifdef __REACTOS__
    ret = GetKernelObjectSecurity(GetCurrentThread(), OWNER_SECURITY_INFORMATION, sd, sizeof(buffer), &size);
#else
    ret = GetKernelObjectSecurity(GetCurrentThread(), OWNER_SECURITY_INFORMATION, &sd, sizeof(buffer), &size);
#endif
    ok(ret, "got error %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(keys); ++i)
    {
        SetLastError(0xdeadbeef);
#ifdef __REACTOS__
        ret = GetKernelObjectSecurity(keys[i], OWNER_SECURITY_INFORMATION, sd, sizeof(buffer), &size);
#else
        ret = GetKernelObjectSecurity(keys[i], OWNER_SECURITY_INFORMATION, &sd, sizeof(buffer), &size);
#endif
        ok(!ret, "key %p: expected failure\n", keys[i]);
        ok(GetLastError() == ERROR_INVALID_HANDLE, "key %p: got error %lu\n", keys[i], GetLastError());

        ret = GetSecurityInfo(keys[i], SE_REGISTRY_KEY,
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &sd_ptr);
        if (keys[i] == HKEY_PERFORMANCE_DATA)
            ok(ret == ERROR_INVALID_HANDLE, "key %p: got error %u\n", keys[i], ret);
        else if (keys[i] == HKEY_DYN_DATA)
            todo_wine ok(ret == ERROR_CALL_NOT_IMPLEMENTED || broken(ret == ERROR_INVALID_HANDLE) /* <7 */,
                    "key %p: got error %u\n", keys[i], ret);
        else
            ok(!ret, "key %p: got error %u\n", keys[i], ret);
        if (!ret) LocalFree(sd_ptr);

        ret = GetSecurityInfo(keys[i], SE_KERNEL_OBJECT,
                DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &sd_ptr);
        ok(ret == ERROR_INVALID_HANDLE, "key %p: got error %u\n", keys[i], ret);
    }
}

static const LUID_AND_ATTRIBUTES *find_privilege(const TOKEN_PRIVILEGES *privs, const LUID *luid)
{
    DWORD i;

    for (i = 0; i < privs->PrivilegeCount; ++i)
    {
        if (!memcmp(luid, &privs->Privileges[i].Luid, sizeof(LUID)))
            return &privs->Privileges[i];
    }

    return NULL;
}

static void test_duplicate_token(void)
{
    const DWORD orig_access = TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_PRIVILEGES;
    char prev_privs_buffer[128], ret_privs_buffer[1024];
    TOKEN_PRIVILEGES *prev_privs = (void *)prev_privs_buffer;
    TOKEN_PRIVILEGES *ret_privs = (void *)ret_privs_buffer;
    const LUID_AND_ATTRIBUTES *priv;
    TOKEN_PRIVILEGES privs;
    SECURITY_QUALITY_OF_SERVICE qos = {.Length = sizeof(qos)};
    OBJECT_ATTRIBUTES attr = {.Length = sizeof(attr)};
    SECURITY_IMPERSONATION_LEVEL level;
    HANDLE token, token2;
    DWORD size;
    BOOL ret;

    ret = OpenProcessToken(GetCurrentProcess(), orig_access, &token);
    ok(ret, "got error %lu\n", GetLastError());

    /* Disable a privilege, to see if that privilege modification is preserved
     * in the duplicated tokens. */
    privs.PrivilegeCount = 1;
    ret = LookupPrivilegeValueA(NULL, "SeChangeNotifyPrivilege", &privs.Privileges[0].Luid);
    ok(ret, "got error %lu\n", GetLastError());
    privs.Privileges[0].Attributes = 0;
    ret = AdjustTokenPrivileges(token, FALSE, &privs, sizeof(prev_privs_buffer), prev_privs, &size);
    ok(ret, "got error %lu\n", GetLastError());

    ret = DuplicateToken(token, SecurityAnonymous, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    TEST_GRANTED_ACCESS(token2, TOKEN_QUERY | TOKEN_IMPERSONATE);
    ret = GetTokenInformation(token2, TokenImpersonationLevel, &level, sizeof(level), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(level == SecurityAnonymous, "got impersonation level %#x\n", level);
    ret = GetTokenInformation(token2, TokenPrivileges, ret_privs, sizeof(ret_privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());
    priv = find_privilege(ret_privs, &privs.Privileges[0].Luid);
    ok(!!priv, "Privilege should exist\n");
    todo_wine ok(priv->Attributes == SE_GROUP_MANDATORY, "Got attributes %#lx\n", priv->Attributes);
    CloseHandle(token2);

    ret = DuplicateTokenEx(token, 0, NULL, SecurityAnonymous, TokenPrimary, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    TEST_GRANTED_ACCESS(token2, orig_access);
    ret = GetTokenInformation(token2, TokenImpersonationLevel, &level, sizeof(level), &size);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got error %lu.\n", GetLastError());
    ret = GetTokenInformation(token2, TokenPrivileges, ret_privs, sizeof(ret_privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());
    priv = find_privilege(ret_privs, &privs.Privileges[0].Luid);
    ok(!!priv, "Privilege should exist\n");
    todo_wine ok(priv->Attributes == SE_GROUP_MANDATORY, "Got attributes %#lx\n", priv->Attributes);
    CloseHandle(token2);

    ret = DuplicateTokenEx(token, MAXIMUM_ALLOWED, NULL, SecurityAnonymous, TokenPrimary, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    TEST_GRANTED_ACCESS(token2, TOKEN_ALL_ACCESS);
    CloseHandle(token2);

    ret = DuplicateTokenEx(token, TOKEN_QUERY_SOURCE, NULL, SecurityAnonymous, TokenPrimary, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    TEST_GRANTED_ACCESS(token2, TOKEN_QUERY_SOURCE);
    CloseHandle(token2);

    ret = DuplicateTokenEx(token, 0, NULL, SecurityIdentification, TokenImpersonation, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    TEST_GRANTED_ACCESS(token2, orig_access);
    ret = GetTokenInformation(token2, TokenImpersonationLevel, &level, sizeof(level), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(level == SecurityIdentification, "got impersonation level %#x\n", level);
    ret = GetTokenInformation(token2, TokenPrivileges, ret_privs, sizeof(ret_privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());
    priv = find_privilege(ret_privs, &privs.Privileges[0].Luid);
    ok(!!priv, "Privilege should exist\n");
    todo_wine ok(priv->Attributes == SE_GROUP_MANDATORY, "Got attributes %#lx\n", priv->Attributes);
    CloseHandle(token2);

    ret = NtDuplicateToken(token, 0, &attr, FALSE, TokenImpersonation, &token2);
    ok(ret == STATUS_SUCCESS, "Got status %#x.\n", ret);
    TEST_GRANTED_ACCESS(token2, orig_access);
    ret = GetTokenInformation(token2, TokenImpersonationLevel, &level, sizeof(level), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(level == SecurityAnonymous, "got impersonation level %#x\n", level);
    ret = GetTokenInformation(token2, TokenPrivileges, ret_privs, sizeof(ret_privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());
    priv = find_privilege(ret_privs, &privs.Privileges[0].Luid);
    ok(!!priv, "Privilege should exist\n");
    todo_wine ok(priv->Attributes == SE_GROUP_MANDATORY, "Got attributes %#lx\n", priv->Attributes);
    CloseHandle(token2);

    ret = NtDuplicateToken(token, 0, &attr, TRUE, TokenImpersonation, &token2);
    ok(ret == STATUS_SUCCESS, "Got status %#x.\n", ret);
    TEST_GRANTED_ACCESS(token2, orig_access);
    ret = GetTokenInformation(token2, TokenPrivileges, ret_privs, sizeof(ret_privs_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());
    priv = find_privilege(ret_privs, &privs.Privileges[0].Luid);
    todo_wine ok(!priv, "Privilege shouldn't exist\n");
    CloseHandle(token2);

    qos.ImpersonationLevel = SecurityIdentification;
    qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    qos.EffectiveOnly = FALSE;
    attr.SecurityQualityOfService = &qos;
    ret = NtDuplicateToken(token, 0, &attr, FALSE, TokenImpersonation, &token2);
    ok(ret == STATUS_SUCCESS, "Got status %#x.\n", ret);
    TEST_GRANTED_ACCESS(token2, orig_access);
    ret = GetTokenInformation(token2, TokenImpersonationLevel, &level, sizeof(level), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(level == SecurityIdentification, "got impersonation level %#x\n", level);
    CloseHandle(token2);

    privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    ret = AdjustTokenPrivileges(token, FALSE, &privs, sizeof(prev_privs_buffer), prev_privs, &size);
    ok(ret, "got error %lu\n", GetLastError());

    CloseHandle(token);
}

static void test_GetKernelObjectSecurity(void)
{
    /* Basic tests for parameter validation. */

    SECURITY_DESCRIPTOR_CONTROL control;
    DWORD size, ret_size, revision;
    BOOL ret, present, defaulted;
    PSECURITY_DESCRIPTOR sd;
    PSID sid;
    ACL *acl;

    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetKernelObjectSecurity(NULL, OWNER_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(GetCurrentProcess(), OWNER_SECURITY_INFORMATION, NULL, 0, NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetKernelObjectSecurity(GetCurrentProcess(), OWNER_SECURITY_INFORMATION, NULL, 0, &size);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    ok(size > 0 && size != 0xdeadbeef, "got size 0\n");

    sd = malloc(size + 1);

    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(GetCurrentProcess(), OWNER_SECURITY_INFORMATION, sd, size - 1, &ret_size);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    ok(ret_size == size, "expected size %lu, got %lu\n", size, ret_size);

    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(GetCurrentProcess(), OWNER_SECURITY_INFORMATION, sd, size + 1, &ret_size);
    ok(ret, "expected success\n");
    ok(GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError());
    ok(ret_size == size, "expected size %lu, got %lu\n", size, ret_size);

    free(sd);

    /* Calling the function with flags not defined succeeds and yields an empty
     * descriptor. */

    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(GetCurrentProcess(), 0x100000, NULL, 0, &size);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());

    sd = malloc(size);
    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(GetCurrentProcess(), 0x100000, sd, size, &ret_size);
    ok(ret, "expected success\n");
    ok(GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError());
    ok(ret_size == size, "expected size %lu, got %lu\n", size, ret_size);

    ret = GetSecurityDescriptorControl(sd, &control, &revision);
    ok(ret, "got error %lu\n", GetLastError());
    todo_wine ok(control == SE_SELF_RELATIVE, "got control %#x\n", control);
    ok(revision == SECURITY_DESCRIPTOR_REVISION1, "got revision %lu\n", revision);

    ret = GetSecurityDescriptorOwner(sd, &sid, &defaulted);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!sid, "expected no owner SID\n");
    ok(!defaulted, "expected owner not defaulted\n");

    ret = GetSecurityDescriptorGroup(sd, &sid, &defaulted);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!sid, "expected no group SID\n");
    ok(!defaulted, "expected group not defaulted\n");

    ret = GetSecurityDescriptorDacl(sd, &present, &acl, &defaulted);
    ok(ret, "got error %lu\n", GetLastError());
    todo_wine ok(!present, "expected no DACL present\n");
    /* the descriptor is defaulted only on Windows >= 7 */

    ret = GetSecurityDescriptorSacl(sd, &present, &acl, &defaulted);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!present, "expected no SACL present\n");
    /* the descriptor is defaulted only on Windows >= 7 */

    free(sd);
}

static void check_different_token(HANDLE token1, HANDLE token2)
{
    TOKEN_STATISTICS stats1, stats2;
    DWORD size;
    BOOL ret;

    ret = GetTokenInformation(token1, TokenStatistics, &stats1, sizeof(stats1), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ret = GetTokenInformation(token2, TokenStatistics, &stats2, sizeof(stats2), &size);
    ok(ret, "got error %lu\n", GetLastError());

    ok(memcmp(&stats1.TokenId, &stats2.TokenId, sizeof(LUID)), "expected different IDs\n");
}

static void test_elevation(void)
{
    TOKEN_LINKED_TOKEN linked, linked2;
    DWORD orig_type, type, size;
    TOKEN_ELEVATION elevation;
    HANDLE token, token2;
    BOOL ret;

#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03) {
        skip("test_elevation() is invalid for WS03\n");
        return;
    }
#endif
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | READ_CONTROL | TOKEN_DUPLICATE
            | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_DEFAULT, &token);
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetTokenInformation(token, TokenElevationType, &type, sizeof(type), &size);
    ok(ret, "got error %lu\n", GetLastError());
    orig_type = type;
    ret = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ret = GetTokenInformation(token, TokenLinkedToken, &linked, sizeof(linked), &size);
    if (!ret && GetLastError() == ERROR_NO_SUCH_LOGON_SESSION) /* fails on w2008s64 */
    {
        win_skip("Failed to get linked token.\n");
        CloseHandle(token);
        return;
    }
    ok(ret, "got error %lu\n", GetLastError());

    if (type == TokenElevationTypeDefault)
    {
        ok(elevation.TokenIsElevated == FALSE, "got elevation %#lx\n", elevation.TokenIsElevated);
        ok(!linked.LinkedToken, "expected no linked token\n");
    }
    else if (type == TokenElevationTypeLimited)
    {
        ok(elevation.TokenIsElevated == FALSE, "got elevation %#lx\n", elevation.TokenIsElevated);
        ok(!!linked.LinkedToken, "expected a linked token\n");

        TEST_GRANTED_ACCESS(linked.LinkedToken, TOKEN_ALL_ACCESS);
        ret = GetTokenInformation(linked.LinkedToken, TokenElevationType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenElevationTypeFull, "got type %#lx\n", type);
        ret = GetTokenInformation(linked.LinkedToken, TokenElevation, &elevation, sizeof(elevation), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(elevation.TokenIsElevated == TRUE, "got elevation %#lx\n", elevation.TokenIsElevated);
        ret = GetTokenInformation(linked.LinkedToken, TokenType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenImpersonation, "got type %#lx\n", type);
        ret = GetTokenInformation(linked.LinkedToken, TokenImpersonationLevel, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == SecurityIdentification, "got impersonation level %#lx\n", type);

        /* Asking for the linked token again gives us a different token. */
        ret = GetTokenInformation(token, TokenLinkedToken, &linked2, sizeof(linked2), &size);
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetTokenInformation(linked2.LinkedToken, TokenElevationType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenElevationTypeFull, "got type %#lx\n", type);
        ret = GetTokenInformation(linked2.LinkedToken, TokenElevation, &elevation, sizeof(elevation), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(elevation.TokenIsElevated == TRUE, "got elevation %#lx\n", elevation.TokenIsElevated);

        check_different_token(linked.LinkedToken, linked2.LinkedToken);

        CloseHandle(linked2.LinkedToken);

        /* Asking for the linked token's linked token gives us a new limited token. */
        ret = GetTokenInformation(linked.LinkedToken, TokenLinkedToken, &linked2, sizeof(linked2), &size);
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetTokenInformation(linked2.LinkedToken, TokenElevationType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenElevationTypeLimited, "got type %#lx\n", type);
        ret = GetTokenInformation(linked2.LinkedToken, TokenElevation, &elevation, sizeof(elevation), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(elevation.TokenIsElevated == FALSE, "got elevation %#lx\n", elevation.TokenIsElevated);

        check_different_token(token, linked2.LinkedToken);

        CloseHandle(linked2.LinkedToken);

        CloseHandle(linked.LinkedToken);

        type = TokenElevationTypeLimited;
        ret = SetTokenInformation(token, TokenElevationType, &type, sizeof(type));
        ok(!ret, "expected failure\n");
        todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

        elevation.TokenIsElevated = FALSE;
        ret = SetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation));
        ok(!ret, "expected failure\n");
        todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());
    }
    else
    {
        ok(elevation.TokenIsElevated == TRUE, "got elevation %#lx\n", elevation.TokenIsElevated);
        ok(!!linked.LinkedToken, "expected a linked token\n");

        TEST_GRANTED_ACCESS(linked.LinkedToken, TOKEN_ALL_ACCESS);
        ret = GetTokenInformation(linked.LinkedToken, TokenElevationType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenElevationTypeLimited, "got type %#lx\n", type);
        ret = GetTokenInformation(linked.LinkedToken, TokenElevation, &elevation, sizeof(elevation), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(elevation.TokenIsElevated == FALSE, "got elevation %#lx\n", elevation.TokenIsElevated);
        ret = GetTokenInformation(linked.LinkedToken, TokenType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenImpersonation, "got type %#lx\n", type);
        ret = GetTokenInformation(linked.LinkedToken, TokenImpersonationLevel, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == SecurityIdentification, "got impersonation level %#lx\n", type);

        /* Asking for the linked token again gives us a different token. */
        ret = GetTokenInformation(token, TokenLinkedToken, &linked2, sizeof(linked2), &size);
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetTokenInformation(linked2.LinkedToken, TokenElevationType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenElevationTypeLimited, "got type %#lx\n", type);
        ret = GetTokenInformation(linked2.LinkedToken, TokenElevation, &elevation, sizeof(elevation), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(elevation.TokenIsElevated == FALSE, "got elevation %#lx\n", elevation.TokenIsElevated);

        check_different_token(linked.LinkedToken, linked2.LinkedToken);

        CloseHandle(linked2.LinkedToken);

        /* Asking for the linked token's linked token gives us a new elevated token. */
        ret = GetTokenInformation(linked.LinkedToken, TokenLinkedToken, &linked2, sizeof(linked2), &size);
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetTokenInformation(linked2.LinkedToken, TokenElevationType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenElevationTypeFull, "got type %#lx\n", type);
        ret = GetTokenInformation(linked2.LinkedToken, TokenElevation, &elevation, sizeof(elevation), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(elevation.TokenIsElevated == TRUE, "got elevation %#lx\n", elevation.TokenIsElevated);

        check_different_token(token, linked2.LinkedToken);

        CloseHandle(linked2.LinkedToken);

        CloseHandle(linked.LinkedToken);

        type = TokenElevationTypeLimited;
        ret = SetTokenInformation(token, TokenElevationType, &type, sizeof(type));
        ok(!ret, "expected failure\n");
        todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

        elevation.TokenIsElevated = FALSE;
        ret = SetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation));
        ok(!ret, "expected failure\n");
        todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());
    }

    ret = DuplicateTokenEx(token, TOKEN_ALL_ACCESS, NULL, SecurityAnonymous, TokenPrimary, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    ret = GetTokenInformation(token2, TokenElevationType, &type, sizeof(type), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(type == orig_type, "expected same type\n");
    ret = GetTokenInformation(token2, TokenElevation, &elevation, sizeof(elevation), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(elevation.TokenIsElevated == (type == TokenElevationTypeFull), "got elevation %#lx\n", elevation.TokenIsElevated);
    ret = GetTokenInformation(token2, TokenLinkedToken, &linked, sizeof(linked), &size);
    ok(ret, "got error %lu\n", GetLastError());
    if (type == TokenElevationTypeDefault)
    {
        ok(!linked.LinkedToken, "expected no linked token\n");
        ret = GetTokenInformation(linked.LinkedToken, TokenType, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == TokenImpersonation, "got type %#lx\n", type);
        ret = GetTokenInformation(linked.LinkedToken, TokenImpersonationLevel, &type, sizeof(type), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(type == SecurityIdentification, "got impersonation level %#lx\n", type);
        CloseHandle(linked.LinkedToken);
    }
    else
        ok(!!linked.LinkedToken, "expected a linked token\n");
    CloseHandle(token2);

    ret = CreateRestrictedToken(token, 0, 0, NULL, 0, NULL, 0, NULL, &token2);
    ok(ret, "got error %lu\n", GetLastError());
    ret = GetTokenInformation(token2, TokenElevationType, &type, sizeof(type), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(type == orig_type, "expected same type\n");
    ret = GetTokenInformation(token2, TokenElevation, &elevation, sizeof(elevation), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ok(elevation.TokenIsElevated == (type == TokenElevationTypeFull), "got elevation %#lx\n", elevation.TokenIsElevated);
    ret = GetTokenInformation(token2, TokenLinkedToken, &linked, sizeof(linked), &size);
    ok(ret, "got error %lu\n", GetLastError());
    if (type == TokenElevationTypeDefault)
        ok(!linked.LinkedToken, "expected no linked token\n");
    else
        ok(!!linked.LinkedToken, "expected a linked token\n");
    CloseHandle(linked.LinkedToken);
    CloseHandle(token2);

    if (type != TokenElevationTypeDefault)
    {
        char prev_privs_buffer[128], acl_buffer[256], prev_acl_buffer[256];
        TOKEN_PRIVILEGES privs, *prev_privs = (TOKEN_PRIVILEGES *)prev_privs_buffer;
        TOKEN_DEFAULT_DACL *prev_acl = (TOKEN_DEFAULT_DACL *)prev_acl_buffer;
        TOKEN_DEFAULT_DACL *ret_acl = (TOKEN_DEFAULT_DACL *)acl_buffer;
        TOKEN_DEFAULT_DACL default_acl;
        PRIVILEGE_SET priv_set;
        BOOL ret, is_member;
        DWORD size;
        ACL acl;

        /* Linked tokens do not preserve privilege modifications. */

        privs.PrivilegeCount = 1;
        ret = LookupPrivilegeValueA(NULL, "SeChangeNotifyPrivilege", &privs.Privileges[0].Luid);
        ok(ret, "got error %lu\n", GetLastError());
        privs.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;
        ret = AdjustTokenPrivileges(token, FALSE, &privs, sizeof(prev_privs_buffer), prev_privs, &size);
        ok(ret, "got error %lu\n", GetLastError());

        priv_set.PrivilegeCount = 1;
        priv_set.Control = 0;
        priv_set.Privilege[0] = privs.Privileges[0];
        ret = PrivilegeCheck(token, &priv_set, &is_member);
        ok(ret, "got error %lu\n", GetLastError());
        ok(!is_member, "not a member\n");

        ret = GetTokenInformation(token, TokenLinkedToken, &linked, sizeof(linked), &size);
        ok(ret, "got error %lu\n", GetLastError());

        ret = PrivilegeCheck(linked.LinkedToken, &priv_set, &is_member);
        ok(ret, "got error %lu\n", GetLastError());
        ok(is_member, "not a member\n");

        CloseHandle(linked.LinkedToken);

        ret = AdjustTokenPrivileges(token, FALSE, prev_privs, 0, NULL, NULL);
        ok(ret, "got error %lu\n", GetLastError());

        /* Linked tokens do not preserve default DACL modifications. */

        ret = GetTokenInformation(token, TokenDefaultDacl, prev_acl, sizeof(prev_acl_buffer), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(prev_acl->DefaultDacl->AceCount, "expected non-empty default DACL\n");

        InitializeAcl(&acl, sizeof(acl), ACL_REVISION);
        default_acl.DefaultDacl = &acl;
        ret = SetTokenInformation(token, TokenDefaultDacl, &default_acl, sizeof(default_acl));
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetTokenInformation(token, TokenDefaultDacl, ret_acl, sizeof(acl_buffer), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(!ret_acl->DefaultDacl->AceCount, "expected empty default DACL\n");

        ret = GetTokenInformation(token, TokenLinkedToken, &linked, sizeof(linked), &size);
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetTokenInformation(linked.LinkedToken, TokenDefaultDacl, ret_acl, sizeof(acl_buffer), &size);
        ok(ret, "got error %lu\n", GetLastError());
        ok(ret_acl->DefaultDacl->AceCount, "expected non-empty default DACL\n");

        CloseHandle(linked.LinkedToken);

        ret = SetTokenInformation(token, TokenDefaultDacl, prev_acl, sizeof(*prev_acl));
        ok(ret, "got error %lu\n", GetLastError());
    }

    CloseHandle(token);
}

static void test_group_as_file_owner(void)
{
    char sd_buffer[200], sid_buffer[100];
    SECURITY_DESCRIPTOR *sd = (SECURITY_DESCRIPTOR *)sd_buffer;
    char temp_path[MAX_PATH], path[MAX_PATH];
    SID *admin_sid = (SID *)sid_buffer;
    BOOL ret, present, defaulted;
    SECURITY_DESCRIPTOR new_sd;
    HANDLE file;
    DWORD size;
    ACL *dacl;

    /* The EA Origin client sets the SD owner of a directory to Administrators,
     * while using the default DACL, and subsequently tries to create
     * subdirectories. */

    size = sizeof(sid_buffer);
    CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &size);

    ret = CheckTokenMembership(NULL, admin_sid, &present);
    ok(ret, "got error %lu\n", GetLastError());
    if (!present)
    {
        skip("user is not an administrator\n");
        return;
    }

    GetTempPathA(ARRAY_SIZE(temp_path), temp_path);
    sprintf(path, "%s\\testdir", temp_path);

    ret = CreateDirectoryA(path, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    file = CreateFileA(path, FILE_ALL_ACCESS, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    ret = GetKernelObjectSecurity(file, DACL_SECURITY_INFORMATION, sd_buffer, sizeof(sd_buffer), &size);
    ok(ret, "got error %lu\n", GetLastError());
    ret = GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted);
    ok(ret, "got error %lu\n", GetLastError());

    InitializeSecurityDescriptor(&new_sd, SECURITY_DESCRIPTOR_REVISION);

    ret = SetSecurityDescriptorOwner(&new_sd, admin_sid, FALSE);
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted);
    ok(ret, "got error %lu\n", GetLastError());

    ret = SetSecurityDescriptorDacl(&new_sd, present, dacl, defaulted);
    ok(ret, "got error %lu\n", GetLastError());

    ret = SetKernelObjectSecurity(file, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, &new_sd);
    ok(ret, "got error %lu\n", GetLastError());

    CloseHandle(file);

    sprintf(path, "%s\\testdir\\subdir", temp_path);
    ret = CreateDirectoryA(path, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    ret = RemoveDirectoryA(path);
    ok(ret, "got error %lu\n", GetLastError());
    sprintf(path, "%s\\testdir", temp_path);
    ret = RemoveDirectoryA(path);
    ok(ret, "got error %lu\n", GetLastError());
}

static void test_IsValidSecurityDescriptor(void)
{
    SECURITY_DESCRIPTOR *sd;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = IsValidSecurityDescriptor(NULL);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_INVALID_SECURITY_DESCR, "Unexpected error %ld.\n", GetLastError());

    sd = calloc(1, SECURITY_DESCRIPTOR_MIN_LENGTH);

    SetLastError(0xdeadbeef);
    ret = IsValidSecurityDescriptor(sd);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_INVALID_SECURITY_DESCR, "Unexpected error %ld.\n", GetLastError());

    ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(ret, "Unexpected return value %d, error %ld.\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = IsValidSecurityDescriptor(sd);
    ok(ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Unexpected error %ld.\n", GetLastError());

    free(sd);
}

static void test_window_security(void)
{
    PSECURITY_DESCRIPTOR sd;
    BOOL present, defaulted;
    HDESK desktop;
    DWORD ret;
    ACL *dacl;

    desktop = GetThreadDesktop(GetCurrentThreadId());

    ret = GetSecurityInfo(desktop, SE_WINDOW_OBJECT,
            DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &sd);
    ok(!ret, "got error %lu\n", ret);

    ret = GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted);
    ok(ret == TRUE, "got error %lu\n", GetLastError());
    todo_wine ok(present == TRUE, "got present %d\n", present);
    ok(defaulted == FALSE, "got defaulted %d\n", defaulted);

    LocalFree(sd);
}

START_TEST(security)
{
    init();
    if (!hmod) return;

    if (myARGC >= 3)
    {
        if (!strcmp(myARGV[2], "test_token_sd"))
            test_child_token_sd();
        else if (!strcmp(myARGV[2], "test"))
            test_process_security_child();
        else if (!strcmp(myARGV[2], "duplicate"))
            test_duplicate_handle_access_child();
        else if (!strcmp(myARGV[2], "restricted"))
            test_create_process_token_child();
        return;
    }
    test_kernel_objects_security();
    test_ConvertStringSidToSid();
    test_trustee();
    test_allocateLuid();
    test_lookupPrivilegeName();
    test_lookupPrivilegeValue();
    test_CreateWellKnownSid();
    test_FileSecurity();
    test_AccessCheck();
    test_token_attr();
    test_GetTokenInformation();
    test_LookupAccountSid();
    test_LookupAccountName();
    test_security_descriptor();
    test_process_security();
    test_impersonation_level();
    test_SetEntriesInAclW();
    test_SetEntriesInAclA();
    test_CreateDirectoryA();
    test_GetNamedSecurityInfoA();
    test_ConvertStringSecurityDescriptor();
    test_ConvertSecurityDescriptorToString();
    test_PrivateObjectSecurity();
    test_InitializeAcl();
    test_GetWindowsAccountDomainSid();
    test_EqualDomainSid();
    test_GetSecurityInfo();
    test_GetSidSubAuthority();
    test_CheckTokenMembership();
    test_EqualSid();
    test_GetUserNameA();
    test_GetUserNameW();
    test_CreateRestrictedToken();
    test_TokenIntegrityLevel();
    test_default_dacl_owner_group_sid();
    test_AdjustTokenPrivileges();
    test_AddAce();
    test_AddMandatoryAce();
    test_system_security_access();
    test_GetSidIdentifierAuthority();
    test_pseudo_tokens();
    test_maximum_allowed();
    test_token_label();
    test_GetExplicitEntriesFromAclW();
    test_BuildSecurityDescriptorW();
    test_duplicate_handle_access();
    test_create_process_token();
    test_pseudo_handle_security();
    test_duplicate_token();
    test_GetKernelObjectSecurity();
    test_elevation();
    test_group_as_file_owner();
    test_IsValidSecurityDescriptor();
    test_window_security();

    /* Must be the last test, modifies process token */
    test_token_security_descriptor();
}
