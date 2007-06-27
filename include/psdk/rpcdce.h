/*
 * Copyright (C) 2000 Francois Gouget
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

#ifndef __WINE_RPCDCE_H
#define __WINE_RPCDCE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

typedef unsigned char* RPC_CSTR;
typedef unsigned short* RPC_WSTR;
typedef void* RPC_AUTH_IDENTITY_HANDLE;
typedef void* RPC_AUTHZ_HANDLE;
typedef void* RPC_IF_HANDLE;
typedef I_RPC_HANDLE RPC_BINDING_HANDLE;
typedef RPC_BINDING_HANDLE handle_t;
#define rpc_binding_handle_t RPC_BINDING_HANDLE
#define RPC_MGR_EPV void

typedef struct _RPC_BINDING_VECTOR
{
  unsigned long Count;
  RPC_BINDING_HANDLE BindingH[1];
} RPC_BINDING_VECTOR;
#define rpc_binding_vector_t RPC_BINDING_VECTOR

typedef struct _UUID_VECTOR
{
  unsigned long Count;
  UUID *Uuid[1];
} UUID_VECTOR;
#define uuid_vector_t UUID_VECTOR

typedef struct _RPC_IF_ID
{
  UUID Uuid;
  unsigned short VersMajor;
  unsigned short VersMinor;
} RPC_IF_ID;

typedef struct
{
  unsigned long Count;
  RPC_IF_ID *IfId[1];
} RPC_IF_ID_VECTOR;

typedef I_RPC_HANDLE *RPC_EP_INQ_HANDLE;

#define RPC_C_EP_ALL_ELTS 0
#define RPC_C_EP_MATCH_BY_IF 1
#define RPC_C_EP_MATCH_BY_OBJ 2
#define RPC_C_EP_MATCH_BY_BOTH 3

#define RPC_C_VERS_ALL 1
#define RPC_C_VERS_COMPATIBLE 2
#define RPC_C_VERS_EXACT 3
#define RPC_C_VERS_MAJOR_ONLY 4
#define RPC_C_VERS_UPTO 5

#define RPC_C_BINDING_INFINITE_TIMEOUT 10
#define RPC_C_BINDING_MIN_TIMEOUT 0
#define RPC_C_BINDING_DEFAULT_TIMEOUT 5
#define RPC_C_BINDING_MAX_TIMEOUT 9

#define RPC_C_CANCEL_INFINITE_TIMEOUT -1

#define RPC_C_LISTEN_MAX_CALLS_DEFAULT 1234
#define RPC_C_PROTSEQ_MAX_REQS_DEFAULT 10

/* RPC_POLICY EndpointFlags */
#define RPC_C_BIND_TO_ALL_NICS          0x1
#define RPC_C_USE_INTERNET_PORT         0x1
#define RPC_C_USE_INTRANET_PORT         0x2
#define RPC_C_DONT_FAIL                 0x4

/* RPC_POLICY EndpointFlags specific to the Falcon/RPC transport */
#define RPC_C_MQ_TEMPORARY                  0x0000
#define RPC_C_MQ_PERMANENT                  0x0001
#define RPC_C_MQ_CLEAR_ON_OPEN              0x0002
#define RPC_C_MQ_USE_EXISTING_SECURITY      0x0004
#define RPC_C_MQ_AUTHN_LEVEL_NONE           0x0000
#define RPC_C_MQ_AUTHN_LEVEL_PKT_INTEGRITY  0x0008
#define RPC_C_MQ_AUTHN_LEVEL_PKT_PRIVACY    0x0010

#define RPC_C_AUTHN_LEVEL_DEFAULT 0
#define RPC_C_AUTHN_LEVEL_NONE 1
#define RPC_C_AUTHN_LEVEL_CONNECT 2
#define RPC_C_AUTHN_LEVEL_CALL 3
#define RPC_C_AUTHN_LEVEL_PKT 4
#define RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5
#define RPC_C_AUTHN_LEVEL_PKT_PRIVACY 6

#define RPC_C_AUTHN_NONE 0
#define RPC_C_AUTHN_DCE_PRIVATE 1
#define RPC_C_AUTHN_DCE_PUBLIC 2
#define RPC_C_AUTHN_DEC_PUBLIC 4
#define RPC_C_AUTHN_GSS_NEGOTIATE 9
#define RPC_C_AUTHN_WINNT 10
#define RPC_C_AUTHN_GSS_SCHANNEL 14
#define RPC_C_AUTHN_GSS_KERBEROS 16
#define RPC_C_AUTHN_DPA 17
#define RPC_C_AUTHN_MSN 18
#define RPC_C_AUTHN_DIGEST 21
#define RPC_C_AUTHN_MQ 100
#define RPC_C_AUTHN_DEFAULT 0xffffffff

/* values for RPC_SECURITY_QOS*::ImpersonationType */
#define RPC_C_IMP_LEVEL_DEFAULT     0
#define RPC_C_IMP_LEVEL_ANONYMOUS   1
#define RPC_C_IMP_LEVEL_IDENTIFY    2
#define RPC_C_IMP_LEVEL_IMPERSONATE 3
#define RPC_C_IMP_LEVEL_DELEGATE    4

/* values for RPC_SECURITY_QOS*::IdentityTracking */
#define RPC_C_QOS_IDENTIFY_STATIC   0
#define RPC_C_QOS_IDENTIFY_DYNAMIC  1

/* flags for RPC_SECURITY_QOS*::Capabilities */
#define RPC_C_QOS_CAPABILITIES_DEFAULT          0x0
#define RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH      0x1
#define RPC_C_QOS_CAPABILITIES_MAKE_FULLSIC     0x2
#define RPC_C_QOS_CAPABILITIES_ANY_AUTHORITY    0x4

/* values for RPC_SECURITY_QOS*::Version */
#define RPC_C_SECURITY_QOS_VERSION      1
#define RPC_C_SECURITY_QOS_VERSION_1    1
#define RPC_C_SECURITY_QOS_VERSION_2    2

/* flags for RPC_SECURITY_QOS_V2::AdditionalSecurityInfoType */
#define RPC_C_AUTHN_INFO_TYPE_HTTP  1

/* flags for RPC_HTTP_TRANSPORT_CREDENTIALS::Flags */
#define RPC_C_HTTP_FLAG_USE_SSL                 0x1
#define RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME   0x2

/* values for RPC_HTTP_TRANSPORT_CREDENTIALS::AuthenticationTarget */
#define RPC_C_HTTP_AUTHN_TARGET_SERVER  1
#define RPC_C_HTTP_AUTHN_TARGET_PROXY   2

#define RPC_C_HTTP_AUTHN_SCHEME_BASIC       0x01
#define RPC_C_HTTP_AUTHN_SCHEME_NTLM        0x02
#define RPC_C_HTTP_AUTHN_SCHEME_PASSPORT    0x04
#define RPC_C_HTTP_AUTHN_SCHEME_DIGEST      0x08
#define RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE   0x10

typedef RPC_STATUS RPC_ENTRY RPC_IF_CALLBACK_FN( RPC_IF_HANDLE InterfaceUuid, void *Context );
typedef void (__RPC_USER *RPC_AUTH_KEY_RETRIEVAL_FN)();

typedef struct _RPC_POLICY
{
  unsigned int  Length;
  unsigned long EndpointFlags;
  unsigned long NICFlags;
} RPC_POLICY,  *PRPC_POLICY;

typedef struct _SEC_WINNT_AUTH_IDENTITY_W
{
    unsigned short* User;
    unsigned long UserLength;
    unsigned short* Domain;
    unsigned long DomainLength;
    unsigned short* Password;
    unsigned long PasswordLength;
    unsigned long Flags;
} SEC_WINNT_AUTH_IDENTITY_W, *PSEC_WINNT_AUTH_IDENTITY_W;

typedef struct _SEC_WINNT_AUTH_IDENTITY_A
{
    unsigned char* User;
    unsigned long UserLength;
    unsigned char* Domain;
    unsigned long DomainLength;
    unsigned char* Password;
    unsigned long PasswordLength;
    unsigned long Flags;
} SEC_WINNT_AUTH_IDENTITY_A, *PSEC_WINNT_AUTH_IDENTITY_A;

typedef struct _RPC_HTTP_TRANSPORT_CREDENTIALS_W
{
    SEC_WINNT_AUTH_IDENTITY_W *TransportCredentials;
    unsigned long Flags;
    unsigned long AuthenticationTarget;
    unsigned long NumberOfAuthnSchemes;
    unsigned long *AuthnSchemes;
    unsigned short *ServerCertificateSubject;
} RPC_HTTP_TRANSPORT_CREDENTIALS_W, *PRPC_HTTP_TRANSPORT_CREDENTIALS_W;

typedef struct _RPC_HTTP_TRANSPORT_CREDENTIALS_A
{
    SEC_WINNT_AUTH_IDENTITY_A *TransportCredentials;
    unsigned long Flags;
    unsigned long AuthenticationTarget;
    unsigned long NumberOfAuthnSchemes;
    unsigned long *AuthnSchemes;
    unsigned char *ServerCertificateSubject;
} RPC_HTTP_TRANSPORT_CREDENTIALS_A, *PRPC_HTTP_TRANSPORT_CREDENTIALS_A;

typedef struct _RPC_SECURITY_QOS {
    unsigned long Version;
    unsigned long Capabilities;
    unsigned long IdentityTracking;
    unsigned long ImpersonationType;
} RPC_SECURITY_QOS, *PRPC_SECURITY_QOS;

typedef struct _RPC_SECURITY_QOS_V2_W
{
    unsigned long Version;
    unsigned long Capabilities;
    unsigned long IdentityTracking;
    unsigned long ImpersonationType;
    unsigned long AdditionalSecurityInfoType;
    union
    {
        RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;
    } u;
} RPC_SECURITY_QOS_V2_W, *PRPC_SECURITY_QOS_V2_W;

typedef struct _RPC_SECURITY_QOS_V2_A
{
    unsigned long Version;
    unsigned long Capabilities;
    unsigned long IdentityTracking;
    unsigned long ImpersonationType;
    unsigned long AdditionalSecurityInfoType;
    union
    {
        RPC_HTTP_TRANSPORT_CREDENTIALS_A *HttpCredentials;
    } u;
} RPC_SECURITY_QOS_V2_A, *PRPC_SECURITY_QOS_V2_A;

#define _SEC_WINNT_AUTH_IDENTITY WINELIB_NAME_AW(_SEC_WINNT_AUTH_IDENTITY_)
#define  SEC_WINNT_AUTH_IDENTITY WINELIB_NAME_AW(SEC_WINNT_AUTH_IDENTITY_)
#define PSEC_WINNT_AUTH_IDENTITY WINELIB_NAME_AW(PSEC_WINNT_AUTH_IDENTITY_)

#define RPC_HTTP_TRANSPORT_CREDENTIALS_  WINELIB_NAME_AW(RPC_HTTP_TRANSPORT_CREDENTIALS_)
#define PRPC_HTTP_TRANSPORT_CREDENTIALS_ WINELIB_NAME_AW(PRPC_HTTP_TRANSPORT_CREDENTIALS_)
#define _RPC_HTTP_TRANSPORT_CREDENTIALS_ WINELIB_NAME_AW(_RPC_HTTP_TRANSPORT_CREDENTIALS_)

#define RPC_SECURITY_QOS_V2  WINELIB_NAME_AW(RPC_SECURITY_QOS_V2_)
#define PRPC_SECURITY_QOS_V2 WINELIB_NAME_AW(PRPC_SECURITY_QOS_V2_)
#define _RPC_SECURITY_QOS_V2 WINELIB_NAME_AW(_RPC_SECURITY_QOS_V2_)

/* SEC_WINNT_AUTH Flags */
#define SEC_WINNT_AUTH_IDENTITY_ANSI    0x1
#define SEC_WINNT_AUTH_IDENTITY_UNICODE 0x2

/* RpcServerRegisterIfEx Flags */
#define RPC_IF_AUTOLISTEN               0x1
#define RPC_IF_OLE                      0x2
#define RPC_IF_ALLOW_UNKNOWN_AUTHORITY  0x4
#define RPC_IF_ALLOW_SECURE_ONLY        0x8

RPC_STATUS RPC_ENTRY DceErrorInqTextA(RPC_STATUS e, RPC_CSTR buffer);
RPC_STATUS RPC_ENTRY DceErrorInqTextW(RPC_STATUS e, RPC_WSTR buffer);
#define              DceErrorInqText WINELIB_NAME_AW(DceErrorInqText)

RPCRTAPI void RPC_ENTRY
  RpcRaiseException( RPC_STATUS exception );
        
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingCopy( RPC_BINDING_HANDLE SourceBinding, RPC_BINDING_HANDLE* DestinationBinding );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingFree( RPC_BINDING_HANDLE* Binding );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingInqObject( RPC_BINDING_HANDLE Binding, UUID* ObjectUuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingReset( RPC_BINDING_HANDLE Binding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingSetObject( RPC_BINDING_HANDLE Binding, UUID* ObjectUuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcObjectSetType( UUID* ObjUuid, UUID* TypeUuid );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingFromStringBindingA( RPC_CSTR StringBinding, RPC_BINDING_HANDLE* Binding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingFromStringBindingW( RPC_WSTR StringBinding, RPC_BINDING_HANDLE* Binding );
#define RpcBindingFromStringBinding WINELIB_NAME_AW(RpcBindingFromStringBinding)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingToStringBindingA( RPC_BINDING_HANDLE Binding, RPC_CSTR *StringBinding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingToStringBindingW( RPC_BINDING_HANDLE Binding, RPC_WSTR *StringBinding );
#define RpcBindingFromStringBinding WINELIB_NAME_AW(RpcBindingFromStringBinding)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingVectorFree( RPC_BINDING_VECTOR** BindingVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingComposeA( RPC_CSTR ObjUuid, RPC_CSTR Protseq, RPC_CSTR NetworkAddr,
                            RPC_CSTR Endpoint, RPC_CSTR Options, RPC_CSTR *StringBinding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingComposeW( RPC_WSTR ObjUuid, RPC_WSTR Protseq, RPC_WSTR NetworkAddr,
                            RPC_WSTR Endpoint, RPC_WSTR Options, RPC_WSTR *StringBinding );
#define RpcStringBindingCompose WINELIB_NAME_AW(RpcStringBindingCompose)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingParseA( RPC_CSTR StringBinding, RPC_CSTR *ObjUuid, RPC_CSTR *Protseq,
                          RPC_CSTR *NetworkAddr, RPC_CSTR *Endpoint, RPC_CSTR *NetworkOptions );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingParseW( RPC_WSTR StringBinding, RPC_WSTR *ObjUuid, RPC_WSTR *Protseq,
                          RPC_WSTR *NetworkAddr, RPC_WSTR *Endpoint, RPC_WSTR *NetworkOptions );
#define RpcStringBindingParse WINELIB_NAME_AW(RpcStringBindingParse)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpResolveBinding( RPC_BINDING_HANDLE Binding, RPC_IF_HANDLE IfSpec );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                  UUID_VECTOR* UuidVector, RPC_CSTR Annotation );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterW( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                  UUID_VECTOR* UuidVector, RPC_WSTR Annotation );
#define RpcEpRegister WINELIB_NAME_AW(RpcEpRegister)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterNoReplaceA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                           UUID_VECTOR* UuidVector, RPC_CSTR Annotation );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterNoReplaceW( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                           UUID_VECTOR* UuidVector, RPC_WSTR Annotation );
#define RpcEpRegisterNoReplace WINELIB_NAME_AW(RpcEpRegisterNoReplace)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpUnregister( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                   UUID_VECTOR* UuidVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerInqBindings( RPC_BINDING_VECTOR** BindingVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerListen( unsigned int MinimumCallThreads, unsigned int MaxCalls, unsigned int DontWait );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcMgmtWaitServerListen( void );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcMgmtStopServerListening( RPC_BINDING_HANDLE Binding );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcMgmtInqIfIds( RPC_BINDING_HANDLE Binding, RPC_IF_ID_VECTOR** IfIdVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcMgmtEpEltInqBegin( RPC_BINDING_HANDLE EpBinding, ULONG InquiryType, RPC_IF_ID *IfId,
                        ULONG VersOption, UUID *ObjectUuid, RPC_EP_INQ_HANDLE *InquiryContext);

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                         unsigned int Flags, unsigned int MaxCalls, RPC_IF_CALLBACK_FN* IfCallbackFn );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterIf2( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                        unsigned int Flags, unsigned int MaxCalls, unsigned int MaxRpcSize, RPC_IF_CALLBACK_FN* IfCallbackFn );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUnregisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, unsigned int WaitForCallsToComplete );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUnregisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, int RundownContextHandles );


RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqA(RPC_CSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor);
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqW(RPC_WSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor);
#define RpcServerUseProtseq WINELIB_NAME_AW(RpcServerUseProtseq)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpA( RPC_CSTR Protseq, unsigned int MaxCalls, RPC_CSTR Endpoint, void *SecurityDescriptor );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpW( RPC_WSTR Protseq, unsigned int MaxCalls, RPC_WSTR Endpoint, void *SecurityDescriptor );
#define RpcServerUseProtseqEp WINELIB_NAME_AW(RpcServerUseProtseqEp)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpExA( RPC_CSTR Protseq, unsigned int MaxCalls, RPC_CSTR Endpoint, void *SecurityDescriptor,
                            PRPC_POLICY Policy );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpExW( RPC_WSTR Protseq, unsigned int MaxCalls, RPC_WSTR Endpoint, void *SecurityDescriptor,
                            PRPC_POLICY Policy );
#define RpcServerUseProtseqEpEx WINELIB_NAME_AW(RpcServerUseProtseqEpEx)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterAuthInfoA( RPC_CSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                              void *Arg );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterAuthInfoW( RPC_WSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                              void *Arg );
#define RpcServerRegisterAuthInfo WINELIB_NAME_AW(RpcServerRegisterAuthInfo)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingSetAuthInfoExA( RPC_BINDING_HANDLE Binding, RPC_CSTR ServerPrincName, ULONG AuthnLevel,
                            ULONG AuthnSvc, RPC_AUTH_IDENTITY_HANDLE AuthIdentity, ULONG AuthzSvr,
                            RPC_SECURITY_QOS *SecurityQos );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingSetAuthInfoExW( RPC_BINDING_HANDLE Binding, RPC_WSTR ServerPrincName, ULONG AuthnLevel,
                            ULONG AuthnSvc, RPC_AUTH_IDENTITY_HANDLE AuthIdentity, ULONG AuthzSvr,
                            RPC_SECURITY_QOS *SecurityQos );
#define RpcBindingSetAuthInfoEx WINELIB_NAME_AW(RpcBindingSetAuthInfoEx)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingSetAuthInfoA( RPC_BINDING_HANDLE Binding, RPC_CSTR ServerPrincName, ULONG AuthnLevel,
                          ULONG AuthnSvc, RPC_AUTH_IDENTITY_HANDLE AuthIdentity, ULONG AuthzSvr );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingSetAuthInfoW( RPC_BINDING_HANDLE Binding, RPC_WSTR ServerPrincName, ULONG AuthnLevel,
                          ULONG AuthnSvc, RPC_AUTH_IDENTITY_HANDLE AuthIdentity, ULONG AuthzSvr );
#define RpcBindingSetAuthInfo WINELIB_NAME_AW(RpcBindingSetAuthInfo)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingInqAuthInfoExA( RPC_BINDING_HANDLE Binding, RPC_CSTR * ServerPrincName, ULONG *AuthnLevel,
                            ULONG *AuthnSvc, RPC_AUTH_IDENTITY_HANDLE *AuthIdentity, ULONG *AuthzSvc,
                            ULONG RpcQosVersion, RPC_SECURITY_QOS *SecurityQOS );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingInqAuthInfoExW( RPC_BINDING_HANDLE Binding, RPC_WSTR *ServerPrincName, ULONG *AuthnLevel,
                            ULONG *AuthnSvc, RPC_AUTH_IDENTITY_HANDLE *AuthIdentity, ULONG *AuthzSvc,
                            ULONG RpcQosVersion, RPC_SECURITY_QOS *SecurityQOS );
#define RpcBindingInqAuthInfoEx WINELIB_NAME_AW(RpcBindingInqAuthInfoEx)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingInqAuthInfoA( RPC_BINDING_HANDLE Binding, RPC_CSTR * ServerPrincName, ULONG *AuthnLevel,
                          ULONG *AuthnSvc, RPC_AUTH_IDENTITY_HANDLE *AuthIdentity, ULONG *AuthzSvc );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingInqAuthInfoW( RPC_BINDING_HANDLE Binding, RPC_WSTR *ServerPrincName, ULONG *AuthnLevel,
                          ULONG *AuthnSvc, RPC_AUTH_IDENTITY_HANDLE *AuthIdentity, ULONG *AuthzSvc );
#define RpcBindingInqAuthInfo WINELIB_NAME_AW(RpcBindingInqAuthInfo)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcNetworkIsProtseqValidA( RPC_CSTR protseq );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcNetworkIsProtseqValidW( RPC_WSTR protseq );
#define RpcNetworkIsProtseqValid WINELIB_NAME_AW(RpcNetworkIsProtseqValid)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringFreeA(RPC_CSTR* String);
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringFreeW(RPC_WSTR* String);
#define RpcStringFree WINELIB_NAME_AW(RpcStringFree)

RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidToStringA( UUID* Uuid, RPC_CSTR* StringUuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidToStringW( UUID* Uuid, RPC_WSTR* StringUuid );
#define UuidToString WINELIB_NAME_AW(UuidToString)

RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidFromStringA( RPC_CSTR StringUuid, UUID* Uuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidFromStringW( RPC_WSTR StringUuid, UUID* Uuid );
#define UuidFromString WINELIB_NAME_AW(UuidFromString)

RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidCreate( UUID* Uuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidCreateSequential( UUID* Uuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidCreateNil( UUID* Uuid );
RPCRTAPI signed int RPC_ENTRY
  UuidCompare( UUID* Uuid1, UUID* Uuid2, RPC_STATUS* Status_ );
RPCRTAPI int RPC_ENTRY
  UuidEqual( UUID* Uuid1, UUID* Uuid2, RPC_STATUS* Status_ );
RPCRTAPI unsigned short RPC_ENTRY
  UuidHash(UUID* Uuid, RPC_STATUS* Status_ );
RPCRTAPI int RPC_ENTRY
  UuidIsNil( UUID* Uuid, RPC_STATUS* Status_ );

#ifdef __cplusplus
}
#endif

#include <rpcdcep.h>

#endif /*__WINE_RPCDCE_H */
