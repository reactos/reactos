/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/lsass/lsass.h
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
    LSASS_REQUEST_REGISTER_LOGON_PROCESS,
    LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE,
    LSASS_REQUEST_DEREGISTER_LOGON_PROCESS,
    LSASS_REQUEST_LOGON_USER,
    LSASS_REQUEST_LOOKUP_AUTHENTICATION_PACKAGE,
    LSASS_REQUEST_MAXIMUM
} LSA_API_NUMBER, *PLSA_API_NUMBER;

#if 0
typedef struct _LSASS_LOOKUP_AUTHENTICATION_PACKAGE_REQUEST
{
   ULONG PackageNameLength;
   WCHAR PackageName[0];
} LSASS_LOOKUP_AUTHENTICATION_PACKAGE_REQUEST,
 *PLSASS_LOOKUP_AUTHENTICATION_PACKAGE_REQUEST;

typedef struct _LSASS_LOOKUP_AUTHENTICATION_PACKAGE_REPLY
{
   ULONG Package;
} LSASS_LOOKUP_AUTHENTICATION_PACKAGE_REPLY,
 *PLSASS_LOOKUP_AUTHENTICATION_PACKAGE_REPLY;

typedef struct _LSASS_DEREGISTER_LOGON_PROCESS_REQUEST
{
    ULONG Dummy;
} LSASS_DEREGISTER_LOGON_PROCESS_REQUEST,
 *PLSASS_DEREGISTER_LOGON_PROCES_REQUEST;

typedef struct _LSASS_DEREGISTER_LOGON_PROCESS_REPLY
{
    ULONG Dummy;
} LSASS_DEREGISTER_LOGON_PROCESS_REPLY,
 *PLSASS_DEREGISTER_LOGON_PROCESS_REPLY;
#endif

typedef struct _LSASS_CALL_AUTHENTICATION_PACKAGE_REQUEST
{
   ULONG AuthenticationPackage;
   ULONG InBufferLength;
   UCHAR InBuffer[0];
} LSASS_CALL_AUTHENTICATION_PACKAGE_REQUEST,
*PLSASS_CALL_AUTHENTICATION_PACKAGE_REQUEST;

typedef struct _LSASS_CALL_AUTHENTICATION_PACKAGE_REPLY
{
   ULONG OutBufferLength;
   UCHAR OutBuffer[0];
} LSASS_CALL_AUTHENTICATION_PACKAGE_REPLY,
*PLSASS_CALL_AUTHENTICATION_PACKAGE_REPLY;

typedef struct _LSASS_LOGON_USER_REQUEST
{
   ULONG OriginNameLength;
   PWSTR OriginName;
   SECURITY_LOGON_TYPE LogonType;
   ULONG AuthenticationPackage;
   PVOID AuthenticationInformation;
   ULONG AuthenticationInformationLength;
   ULONG LocalGroupsCount;
   PSID_AND_ATTRIBUTES LocalGroups;
   TOKEN_SOURCE SourceContext;
   UCHAR Data[1];
} LSASS_LOGON_USER_REQUEST, *PLSASS_LOGON_USER_REQUEST;

typedef struct _LSASS_LOGON_USER_REPLY
{
   PVOID ProfileBuffer;
   ULONG ProfileBufferLength;
   LUID LogonId;
   HANDLE Token;
   QUOTA_LIMITS Quotas;
   NTSTATUS SubStatus;
   UCHAR Data[1];
} LSASS_LOGON_USER_REPLY, *PLSASS_LOGON_USER_REPLY;

#if 0
typedef struct _LSASS_REGISTER_LOGON_PROCESS_REQUEST
{
   ULONG Length;
   WCHAR LogonProcessNameBuffer[127];
} LSASS_REGISTER_LOGON_PROCESS_REQUEST, *PLSASS_REGISTER_LOGON_PROCESS_REQUEST;

typedef struct _LSASS_REGISTER_LOGON_PROCESS_REPLY
{
   LSA_OPERATIONAL_MODE OperationalMode;
} LSASS_REGISTER_LOGON_PROCESS_REPLY, *PLSASS_REGISTER_LOGON_PROCESS_REPLY;
#endif

typedef struct _LSA_CONNECTION_INFO
{
    NTSTATUS Status;
    LSA_OPERATIONAL_MODE OperationalMode;
    ULONG Length;
    CHAR LogonProcessNameBuffer[LSASS_MAX_LOGON_PROCESS_NAME_LENGTH + 1];
} LSA_CONNECTION_INFO, *PLSA_CONNECTION_INFO;

#if 0
typedef union _LSASS_REQUEST
{
   PORT_MESSAGE Header;
   struct {
      UCHAR LpcHeader[sizeof(PORT_MESSAGE)];
      ULONG Type;
      union
        {
           LSASS_REGISTER_LOGON_PROCESS_REQUEST RegisterLogonProcessRequest;
           LSASS_LOGON_USER_REQUEST LogonUserRequest;
           LSASS_CALL_AUTHENTICATION_PACKAGE_REQUEST
             CallAuthenticationPackageRequest;
           LSASS_DEREGISTER_LOGON_PROCESS_REPLY DeregisterLogonProcessRequest;
           LSASS_LOOKUP_AUTHENTICATION_PACKAGE_REQUEST
             LookupAuthenticationPackageRequest;
        } d;
   };
} LSASS_REQUEST, *PLSASS_REQUEST;

typedef struct _LSASS_REPLY
{
   PORT_MESSAGE Header;
   NTSTATUS Status;
   union
     {
	LSASS_REGISTER_LOGON_PROCESS_REPLY RegisterLogonProcessReply;
	LSASS_LOGON_USER_REPLY LogonUserReply;
	LSASS_CALL_AUTHENTICATION_PACKAGE_REPLY CallAuthenticationPackageReply;
	LSASS_DEREGISTER_LOGON_PROCESS_REPLY DeregisterLogonProcessReply;
	LSASS_LOOKUP_AUTHENTICATION_PACKAGE_REPLY
	  LookupAuthenticationPackageReply;
     } d;
} LSASS_REPLY, *PLSASS_REPLY;
#endif


typedef struct _LSA_REGISTER_LOGON_PROCESS_MSG
{
    union
    {
        struct
        {
            ULONG Length;
            CHAR LogonProcessNameBuffer[LSASS_MAX_LOGON_PROCESS_NAME_LENGTH + 1];
        } Request;
        struct
        {
            LSA_OPERATIONAL_MODE OperationalMode;
        } Reply;
    };
} LSA_REGISTER_LOGON_PROCESS_MSG, *PLSA_REGISTER_LOGON_PROCESS_MSG;


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

typedef struct _LSA_API_MSG
{
    PORT_MESSAGE h;
    struct
    {
        LSA_API_NUMBER ApiNumber;
        NTSTATUS Status;
        union
        {
            LSA_REGISTER_LOGON_PROCESS_MSG RegisterLogonProcess;
//            LSA_LOGON_USER_MSG LogonUser;
//            LSA_CALL_AUTHENTICATION_PACKAGE_MSG CallAuthenticationPackage;
            LSA_DEREGISTER_LOGON_PROCESS_MSG DeregisterLogonProcess;
            LSA_LOOKUP_AUTHENTICATION_PACKAGE_MSG LookupAuthenticationPackage;
        };
    };
} LSA_API_MSG, *PLSA_API_MSG;

#define LSA_PORT_DATA_SIZE(c)     (sizeof(ULONG)+sizeof(NTSTATUS)+sizeof(c))
#define LSA_PORT_MESSAGE_SIZE     (sizeof(LSA_API_MSG))

#endif /* __INCLUDE_LSASS_LSASS_H */
