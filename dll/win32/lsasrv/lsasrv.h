/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsasrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>


#include <ntsecapi.h>

#include <string.h>

#include "lsass.h"
#include "lsa_s.h"

#include <wine/debug.h>


typedef enum _LSA_DB_OBJECT_TYPE
{
    LsaDbIgnoreObject,
    LsaDbContainerObject,
    LsaDbPolicyObject,
    LsaDbAccountObject,
    LsaDbDomainObject,
    LsaDbSecretObject
} LSA_DB_OBJECT_TYPE, *PLSA_DB_OBJECT_TYPE;

typedef struct _LSA_DB_OBJECT
{
    ULONG Signature;
    LSA_DB_OBJECT_TYPE ObjectType;
    ULONG RefCount;
    ACCESS_MASK Access;
    HANDLE KeyHandle;
    struct _LSA_DB_OBJECT *ParentObject;
} LSA_DB_OBJECT, *PLSA_DB_OBJECT;

#define LSAP_DB_SIGNATURE 0x12345678


/* authport.c */
NTSTATUS
StartAuthenticationPort(VOID);

/* database.c */
NTSTATUS
LsapInitDatabase(VOID);

LSAPR_HANDLE
LsapCreateDbObject(LSAPR_HANDLE ParentHandle,
                   LPWSTR ObjectName,
                   BOOLEAN Open,
                   LSA_DB_OBJECT_TYPE HandleType,
                   ACCESS_MASK DesiredAccess);

NTSTATUS
LsapValidateDbObject(LSAPR_HANDLE Handle,
                     LSA_DB_OBJECT_TYPE HandleType,
                     ACCESS_MASK GrantedAccess);

NTSTATUS
LsapCloseDbObject(LSAPR_HANDLE Handle);

NTSTATUS
LsapGetObjectAttribute(PLSA_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       LPVOID AttributeData,
                       PULONG AttributeSize);

NTSTATUS
LsapSetObjectAttribute(PLSA_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       LPVOID AttributeData,
                       ULONG AttributeSize);

/* lsarpc.c */
VOID
LsarStartRpcServer(VOID);

/* policy.c */
NTSTATUS
LsarQueryAuditEvents(LSAPR_HANDLE PolicyHandle,
                     PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryPrimaryDomain(LSAPR_HANDLE PolicyHandle,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryAccountDomain(LSAPR_HANDLE PolicyHandle,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryDnsDomain(LSAPR_HANDLE PolicyHandle,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarSetPrimaryDomain(LSAPR_HANDLE PolicyObject,
                     PLSAPR_POLICY_PRIMARY_DOM_INFO Info);

NTSTATUS
LsarSetAccountDomain(LSAPR_HANDLE PolicyObject,
                     PLSAPR_POLICY_ACCOUNT_DOM_INFO Info);

NTSTATUS
LsarSetDnsDomain(LSAPR_HANDLE PolicyObject,
                 PLSAPR_POLICY_DNS_DOMAIN_INFO Info);

/* privileges.c */
NTSTATUS
LsarpLookupPrivilegeName(PLUID Value,
                         PUNICODE_STRING *Name);

NTSTATUS
LsarpLookupPrivilegeValue(PUNICODE_STRING Name,
                          PLUID Value);

/* sids.h */
NTSTATUS
LsapInitSids(VOID);

NTSTATUS
LsapLookupSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
               PLSAPR_TRANSLATED_NAME OutputNames);

