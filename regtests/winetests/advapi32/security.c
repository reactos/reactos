/*
 * Unit tests for security functions
 *
 * Copyright (c) 2004 Mike McCormack
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

#define _WIN32_WINNT 0x0501

#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "aclapi.h"
#include "winnt.h"

typedef BOOL (WINAPI *fnBuildTrusteeWithSidA)( TRUSTEE *trustee, PSID psid );
typedef BOOL (WINAPI *fnBuildTrusteeWithNameA)( TRUSTEE *trustee, LPSTR str );
typedef BOOL (WINAPI *fnConvertSidToStringSidA)( PSID pSid, LPSTR *str );
typedef BOOL (WINAPI *fnConvertStringSidToSidA)( LPCSTR str, PSID pSid );
typedef BOOL (WINAPI *fnGetFileSecurityA)(LPCSTR, SECURITY_INFORMATION,
                                          PSECURITY_DESCRIPTOR, DWORD, LPDWORD);

static HMODULE hmod;

fnBuildTrusteeWithSidA   pBuildTrusteeWithSidA;
fnBuildTrusteeWithNameA  pBuildTrusteeWithNameA;
fnConvertSidToStringSidA pConvertSidToStringSidA;
fnConvertStringSidToSidA pConvertStringSidToSidA;
fnGetFileSecurityA pGetFileSecurityA;

struct sidRef
{
    SID_IDENTIFIER_AUTHORITY auth;
    const char *refStr;
};

static void init(void)
{
    hmod = GetModuleHandle("advapi32.dll");
}

void test_sid()
{
    struct sidRef refs[] = {
     { { {0x00,0x00,0x33,0x44,0x55,0x66} }, "S-1-860116326-1" },
     { { {0x00,0x00,0x01,0x02,0x03,0x04} }, "S-1-16909060-1"  },
     { { {0x00,0x00,0x00,0x01,0x02,0x03} }, "S-1-66051-1"     },
     { { {0x00,0x00,0x00,0x00,0x01,0x02} }, "S-1-258-1"       },
     { { {0x00,0x00,0x00,0x00,0x00,0x02} }, "S-1-2-1"         },
     { { {0x00,0x00,0x00,0x00,0x00,0x0c} }, "S-1-12-1"        },
    };
    const char noSubAuthStr[] = "S-1-5";
    unsigned int i;
    PSID psid = NULL;
    BOOL r;
    LPSTR str = NULL;

    pConvertSidToStringSidA = (fnConvertSidToStringSidA)
                    GetProcAddress( hmod, "ConvertSidToStringSidA" );
    if( !pConvertSidToStringSidA )
        return;
    pConvertStringSidToSidA = (fnConvertStringSidToSidA)
                    GetProcAddress( hmod, "ConvertStringSidToSidA" );
    if( !pConvertStringSidToSidA )
        return;

    r = pConvertStringSidToSidA( NULL, NULL );
    ok( !r, "expected failure with NULL parameters\n" );
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED )
        return;
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = pConvertStringSidToSidA( refs[0].refStr, NULL );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = pConvertStringSidToSidA( NULL, &str );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %ld\n",
     GetLastError() );

    r = pConvertStringSidToSidA( noSubAuthStr, &psid );
    ok( !r,
     "expected failure with no sub authorities\n" );
    ok( GetLastError() == ERROR_INVALID_SID,
     "expected GetLastError() is ERROR_INVALID_SID, got %ld\n",
     GetLastError() );

    for( i = 0; i < sizeof(refs) / sizeof(refs[0]); i++ )
    {
        PISID pisid;

        r = AllocateAndInitializeSid( &refs[i].auth, 1,1,0,0,0,0,0,0,0,
         &psid );
        ok( r, "failed to allocate sid\n" );
        r = pConvertSidToStringSidA( psid, &str );
        ok( r, "failed to convert sid\n" );
        if (r)
        {
            ok( !strcmp( str, refs[i].refStr ),
                "incorrect sid, expected %s, got %s\n", refs[i].refStr, str );
            LocalFree( str );
        }
        if( psid )
            FreeSid( psid );

        r = pConvertStringSidToSidA( refs[i].refStr, &psid );
        ok( r, "failed to parse sid string\n" );
        pisid = (PISID)psid;
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
}

void test_trustee()
{
    TRUSTEE trustee;
    PSID psid;
    LPSTR str = "2jjj";

    SID_IDENTIFIER_AUTHORITY auth = { {0x11,0x22,0,0,0, 0} };

    pBuildTrusteeWithSidA = (fnBuildTrusteeWithSidA)
                    GetProcAddress( hmod, "BuildTrusteeWithSidA" );
    pBuildTrusteeWithNameA = (fnBuildTrusteeWithNameA)
                    GetProcAddress( hmod, "BuildTrusteeWithNameA" );
    if( !pBuildTrusteeWithSidA || !pBuildTrusteeWithNameA)
        return;

    if ( ! AllocateAndInitializeSid( &auth, 1, 42, 0,0,0,0,0,0,0,&psid ) )
    {
        trace( "failed to init SID\n" );
       return;
    }

    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithSidA( &trustee, psid );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE,
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_SID, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == (LPSTR) psid, "ptstrName wrong\n" );
    FreeSid( psid );

    /* test BuildTrusteeWithNameA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithNameA( &trustee, str );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE,
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_NAME, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == str, "ptstrName wrong\n" );
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
#define SE_CHANGE_NOTIFY_PRIVILLEGE      23L
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
    BOOL (WINAPI *pLookupPrivilegeNameA)(LPSTR, PLUID, LPSTR, LPDWORD);
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
     strlen("SeCreateTokenPrivilege") + 1);
    /* check a known value and its returned length on success */
    cchName = sizeof(buf);
    ok(pLookupPrivilegeNameA(NULL, &luid, buf, &cchName) &&
     cchName == strlen("SeCreateTokenPrivilege"),
     "LookupPrivilegeNameA returned an incorrect output length for\n"
     "SeCreateTokenPrivilege (got %ld, expected %d)\n", cchName,
     (int)strlen("SeCreateTokenPrivilege"));
    /* check known values */
    for (i = SE_MIN_WELL_KNOWN_PRIVILEGE; i < SE_MAX_WELL_KNOWN_PRIVILEGE; i++)
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
    ok( !ret && GetLastError() == RPC_S_SERVER_UNAVAILABLE,
     "LookupPrivilegeNameA didn't fail with RPC_S_SERVER_UNAVAILABLE: %ld\n",
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
     { "SeChangeNotifyPrivilege", SE_CHANGE_NOTIFY_PRIVILLEGE },
     { "SeRemoteShutdownPrivilege", SE_REMOTE_SHUTDOWN_PRIVILEGE },
     { "SeUndockPrivilege", SE_UNDOCK_PRIVILEGE },
     { "SeSyncAgentPrivilege", SE_SYNC_AGENT_PRIVILEGE },
     { "SeEnableDelegationPrivilege", SE_ENABLE_DELEGATION_PRIVILEGE },
     { "SeManageVolumePrivilege", SE_MANAGE_VOLUME_PRIVILEGE },
     { "SeImpersonatePrivilege", SE_IMPERSONATE_PRIVILEGE },
     { "SeCreateGlobalPrivilege", SE_CREATE_GLOBAL_PRIVILEGE },
    };
    BOOL (WINAPI *pLookupPrivilegeValueA)(LPCSTR, LPCSTR, PLUID);
    int i;
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
    ok( !ret && GetLastError() == RPC_S_SERVER_UNAVAILABLE,
     "LookupPrivilegeValueA didn't fail with RPC_S_SERVER_UNAVAILABLE: %ld\n",
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
    for (i = 0; i < sizeof(privs) / sizeof(privs[0]); i++)
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

static void test_luid(void)
{
    test_allocateLuid();
    test_lookupPrivilegeName();
    test_lookupPrivilegeValue();
}

static void test_FileSecurity(void)
{
    char directory[MAX_PATH];
    DWORD retval, outSize;
    BOOL result;
    BYTE buffer[0x40];

    pGetFileSecurityA = (fnGetFileSecurityA)
                    GetProcAddress( hmod, "GetFileSecurityA" );
    if( !pGetFileSecurityA )
        return;

    retval = GetTempPathA(sizeof(directory), directory);
    if (!retval) {
        trace("GetTempPathA failed\n");
        return;
    }

    strcpy(directory, "\\Should not exist");

    SetLastError(NO_ERROR);
    result = GetFileSecurityA( directory,OWNER_SECURITY_INFORMATION,
                              (PSECURITY_DESCRIPTOR)buffer,0x40,&outSize);
    ok(!result, "GetFileSecurityA should fail for not existing directories/files\n");
    ok( (GetLastError() == ERROR_FILE_NOT_FOUND ) ||
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) ,
        "last error ERROR_FILE_NOT_FOUND / ERROR_CALL_NOT_IMPLEMENTED (98) "
        "expected, got %ld\n", GetLastError());
}

START_TEST(security)
{
    init();
    if (!hmod) return;
    test_sid();
    test_trustee();
    test_luid();
    test_FileSecurity();
}
