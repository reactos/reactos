/////////////////////////////////////////////////////////////////////////////
//  FILE          : autoenrl.h                                             //
//  DESCRIPTION   : Auto Enrollment functions                              //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//          Jan 25 1995 jeffspel  Created                                      //
//      Apr  9 1995 larrys  Removed some APIs                              //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __AUTOENRL_H__
#define __AUTOENRL_H__

#define SECURITY_WIN32
#include <security.h>
#include <winldap.h>

VOID InitializeAutoEnrollmentSupport (VOID);

HANDLE RegisterAutoEnrollmentProcessing(
                               IN BOOL fMachineEnrollment,
                               IN HANDLE hToken
                               );

BOOL DeRegisterAutoEnrollment(HANDLE hAuto);

typedef struct _AE_CERT_TEST
{
    DWORD       idTest;
    LPWSTR      pwszReason;
} AE_CERT_TEST, *PAE_CERT_TEST;


typedef struct _AE_CERT_TEST_ARRAY
{
    DWORD dwVersion;
    BOOL  fRenewalOK;
    DWORD cTests;
    DWORD cMaxTests;
    AE_CERT_TEST Test[ANYSIZE_ARRAY];
} AE_CERT_TEST_ARRAY, *PAE_CERT_TEST_ARRAY;
#define AE_CERT_TEST_ARRAY_VERSION 0

#define AE_CERT_TEST_SIZE_INCREMENT  10

#define AE_CERT_TEST_ARRAY_PROPID CERT_FIRST_USER_PROP_ID+0x2e0b // 0xae0b (auto-enrollment object)

void AELogTestResult(PAE_CERT_TEST_ARRAY    *ppAEData,
                     DWORD                  idTest,
                     ...);


typedef ULONG (LDAPAPI * PFNLDAP_BIND_SW)(
    LDAP *ld, 
    PWCHAR dn, 
    PWCHAR cred, 
    ULONG method 
    );

typedef ULONG (LDAPAPI * PFNLDAP_BIND_SA)( 
    LDAP *ld, 
    PCHAR dn, 
    PCHAR cred, 
    ULONG method );


typedef BOOLEAN (*PFNGETUSERNAMEEXA)(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    );

typedef BOOLEAN (*PFNGETUSERNAMEEXW)(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    );

typedef LDAP * (LDAPAPI * PFNLDAP_INITW)( PWCHAR HostName, ULONG PortNumber );
typedef LDAP * (LDAPAPI * PFNLDAP_INITA)( PCHAR HostName, ULONG PortNumber );


typedef ULONG (LDAPAPI * PFNLDAP_SET_OPTION) (LDAP *ld, int option, void *invalue);


typedef ULONG (LDAPAPI * PFNLDAP_SEARCH_EXT_SW)(
        LDAP            *ld,
        PWCHAR          base,
        ULONG           scope,
        PWCHAR          filter,
        PWCHAR          attrs[],
        ULONG           attrsonly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        struct l_timeval *timeout,
        ULONG            SizeLimit,
        LDAPMessage     **res
    );
typedef ULONG (LDAPAPI * PFNLDAP_SEARCH_EXT_SA)(
        LDAP            *ld,
        PCHAR           base,
        ULONG           scope,
        PCHAR           filter,
        PCHAR           attrs[],
        ULONG           attrsonly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        struct l_timeval *timeout,
        ULONG            SizeLimit,
        LDAPMessage     **res
    );

typedef LDAPMessage * (LDAPAPI * PFNLDAP_FIRST_ENTRY)(LDAP *ld, LDAPMessage *res);


typedef PWCHAR * (LDAPAPI * PFNLDAP_EXPLODE_DNW)( PWCHAR dn, ULONG notypes );
typedef PCHAR *  (LDAPAPI * PFNLDAP_EXPLODE_DNA)( PCHAR dn, ULONG notypes );


typedef PWCHAR * (LDAPAPI * PFNLDAP_GET_VALUESW)(
        LDAP            *ld,
        LDAPMessage     *entry,
        PWCHAR          attr
        );
typedef PCHAR * (LDAPAPI * PFNLDAP_GET_VALUESA)(
        LDAP            *ld,
        LDAPMessage     *entry,
        PCHAR           attr
        );

typedef ULONG (LDAPAPI * PFNLDAP_VALUE_FREEW)( PWCHAR *vals );
typedef ULONG (LDAPAPI * PFNLDAP_VALUE_FREEA)( PCHAR *vals );

typedef ULONG (LDAPAPI * PFNLDAP_MSGFREE)( LDAPMessage *res );

typedef ULONG (LDAPAPI * PFNLDAP_UNBIND)( LDAP *ld );

typedef ULONG (LDAPAPI * PFNLDAPGETLASTERROR)(VOID);
typedef ULONG (LDAPAPI * PFNLDAPMAPERRORTOWIN32)(ULONG LdapError);

#ifdef UNICODE
#define PFNLDAP_BIND_S            PFNLDAP_BIND_SW
#define PFNGETUSERNAMEEX          PFNGETUSERNAMEEXW
#define PFNLDAP_INIT              PFNLDAP_INITW
#define PFNLDAP_SEARCH_EXT_S      PFNLDAP_SEARCH_EXT_SW
#define PFNLDAP_EXPLODE_DN        PFNLDAP_EXPLODE_DNW
#define PFNLDAP_GET_VALUES        PFNLDAP_GET_VALUESW
#define PFNLDAP_VALUE_FREE        PFNLDAP_VALUE_FREEW
#else
#define PFNLDAP_BIND_S            PFNLDAP_BIND_SA
#define PFNGETUSERNAMEEX          PFNGETUSERNAMEEXA
#define PFNLDAP_INIT              PFNLDAP_INITA
#define PFNLDAP_SEARCH_EXT_S      PFNLDAP_SEARCH_EXT_SA
#define PFNLDAP_EXPLODE_DN        PFNLDAP_EXPLODE_DNA
#define PFNLDAP_GET_VALUES        PFNLDAP_GET_VALUESA
#define PFNLDAP_VALUE_FREE        PFNLDAP_VALUE_FREEA
#endif

#if DBG
#define AE_ERROR                0x0001
#define AE_WARNING              0x0002
#define AE_INFO                 0x0004
#define AE_TRACE                0x0008
#define AE_ALLOC                0x0010
#define AE_RES                  0x0020

#define AE_DEBUG(x) AEDebugLog x
#define AE_BEGIN(x) AEDebugLog(DEB_TRACE, L"BEGIN:" x L"\n");
#define AE_RETURN(x) { AEDebugLog(AE_TRACE, L"RETURN (%lx) Line %d\n",(x), __LINE__); return (x); }
#define AE_END()    { AEDebugLog(AE_TRACE, L"END:Line %d\n",  __LINE__); }
#define AE_BREAK()  { AEDebugLog(AE_TRACE, L"BREAK  Line %d\n",  __LINE__); }
void    AEDebugLog(long Mask,  LPCWSTR Format, ...);

#define MAX_DEBUG_BUFFER 256

#else
#define AE_DEBUG(x) 
#define AE_BEGIN(x) 
#define AE_RETURN(x) return (x)
#define AE_END() 
#define AE_BREAK() 

#endif

#endif // __AUTOENRL_H__
