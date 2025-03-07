/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/reactos/subsys/lsass/lsass.h
 * PURPOSE:         LSASS API declarations
 * UPDATE HISTORY:
 *                  Created 05/08/00
 */

#ifndef __INCLUDE_LSASS_LSASS_H
#define __INCLUDE_LSASS_LSASS_H

#include <ntsecapi.h>

#define LSASS_MAX_LOGON_PROCESS_NAME_LENGTH 127
#define LSASS_MAX_PACKAGE_NAME_LENGTH 127

typedef enum _LSA_API_NUMBER
{
    LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE,
    LSASS_REQUEST_DEREGISTER_LOGON_PROCESS,
    LSASS_REQUEST_LOGON_USER,
    LSASS_REQUEST_LOOKUP_AUTHENTICATION_PACKAGE,
    LSASS_REQUEST_ENUM_LOGON_SESSIONS,
    LSASS_REQUEST_GET_LOGON_SESSION_DATA,
    LSASS_REQUEST_POLICY_CHANGE_NOTIFY,
    LSASS_REQUEST_MAXIMUM
} LSA_API_NUMBER, *PLSA_API_NUMBER;

typedef enum _LSA_TRUSTED_CALLER
{
    NO,
    YES,
    CHECK
} LSA_TRUSTED_CALLER;

typedef struct _LSA_CONNECTION_INFO
{
    NTSTATUS Status;
    LSA_OPERATIONAL_MODE OperationalMode;
    ULONG Length;
    CHAR LogonProcessNameBuffer[LSASS_MAX_LOGON_PROCESS_NAME_LENGTH + 1];
    BOOL CreateContext;
    LSA_TRUSTED_CALLER TrustedCaller;
} LSA_CONNECTION_INFO, *PLSA_CONNECTION_INFO;


typedef struct _LSA_LOGON_USER_MSG
{
    union
    {
        struct
        {
            LSA_STRING OriginName;
            SECURITY_LOGON_TYPE LogonType;
            ULONG AuthenticationPackage;
            PVOID AuthenticationInformation;
            ULONG AuthenticationInformationLength;
            PTOKEN_GROUPS LocalGroups;
            ULONG LocalGroupsCount;
            TOKEN_SOURCE SourceContext;
        } Request;

        struct
        {
            PVOID ProfileBuffer;
            ULONG ProfileBufferLength;
            LUID LogonId;
            HANDLE Token;
            QUOTA_LIMITS Quotas;
            NTSTATUS SubStatus;
        } Reply;
    };
} LSA_LOGON_USER_MSG, *PLSA_LOGON_USER_MSG;


typedef struct _LSA_CALL_AUTHENTICATION_PACKAGE_MSG
{
    union
    {
        struct
        {
            ULONG AuthenticationPackage;
            PVOID ProtocolSubmitBuffer;
            ULONG SubmitBufferLength;
        } Request;
        struct
        {
            PVOID ProtocolReturnBuffer;
            ULONG ReturnBufferLength;
            NTSTATUS ProtocolStatus;
        } Reply;
    };
} LSA_CALL_AUTHENTICATION_PACKAGE_MSG, *PLSA_CALL_AUTHENTICATION_PACKAGE_MSG;


typedef struct _LSA_DEREGISTER_LOGON_PROCESS_MSG
{
    union
    {
        struct
        {
            ULONG Dummy;
        } Request;
        struct
        {
            ULONG Dummy;
        } Reply;
    };
} LSA_DEREGISTER_LOGON_PROCESS_MSG, *PLSA_DEREGISTER_LOGON_PROCESS_MSG;


typedef struct _LSA_LOOKUP_AUTHENTICATION_PACKAGE_MSG
{
    union
    {
        struct
        {
            ULONG PackageNameLength;
            CHAR PackageName[LSASS_MAX_PACKAGE_NAME_LENGTH + 1];
        } Request;
        struct
        {
            ULONG Package;
        } Reply;
    };
} LSA_LOOKUP_AUTHENTICATION_PACKAGE_MSG, *PLSA_LOOKUP_AUTHENTICATION_PACKAGE_MSG;


typedef struct _LSA_ENUM_LOGON_SESSIONS_MSG
{
    union
    {
        struct
        {
            ULONG Dummy;
        } Request;
        struct
        {
            ULONG LogonSessionCount;
            PVOID LogonSessionBuffer;
        } Reply;
    };
} LSA_ENUM_LOGON_SESSIONS_MSG, *PLSA_ENUM_LOGON_SESSIONS_MSG;


typedef struct _LSA_GET_LOGON_SESSION_DATA_MSG
{
    union
    {
        struct
        {
            LUID LogonId;
        } Request;
        struct
        {
            PVOID SessionDataBuffer;
        } Reply;
    };
} LSA_GET_LOGON_SESSION_DATA_MSG, *PLSA_GET_LOGON_SESSION_DATA_MSG;


typedef struct _LSA_POLICY_CHANGE_NOTIFY_MSG
{
    union
    {
        struct
        {
            POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass;
            HANDLE NotificationEventHandle;
            BOOLEAN Register;
        } Request;
        struct
        {
            ULONG Dummy;
        } Reply;
    };
} LSA_POLICY_CHANGE_NOTIFY_MSG, *PLSA_POLICY_CHANGE_NOTIFY_MSG;


typedef struct _LSA_API_MSG
{
    PORT_MESSAGE h;
    union
    {
        LSA_CONNECTION_INFO ConnectInfo;
        struct
        {
            LSA_API_NUMBER ApiNumber;
            NTSTATUS Status;
            union
            {
                LSA_LOGON_USER_MSG LogonUser;
                LSA_CALL_AUTHENTICATION_PACKAGE_MSG CallAuthenticationPackage;
                LSA_DEREGISTER_LOGON_PROCESS_MSG DeregisterLogonProcess;
                LSA_LOOKUP_AUTHENTICATION_PACKAGE_MSG LookupAuthenticationPackage;
                LSA_ENUM_LOGON_SESSIONS_MSG EnumLogonSessions;
                LSA_GET_LOGON_SESSION_DATA_MSG GetLogonSessionData;
                LSA_POLICY_CHANGE_NOTIFY_MSG PolicyChangeNotify;
            };
        };
    };
} LSA_API_MSG, *PLSA_API_MSG;

#define LSA_PORT_DATA_SIZE(c)     (sizeof(ULONG)+sizeof(NTSTATUS)+sizeof(c))
#define LSA_PORT_MESSAGE_SIZE     (sizeof(LSA_API_MSG))

#endif /* __INCLUDE_LSASS_LSASS_H */
