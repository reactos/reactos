#ifndef _RPCDCE_H
#define _RPCDCE_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <basetyps.h>

#define IN
#define OUT
#ifndef OPTIONAL
#define OPTIONAL
#endif
#define uuid_t UUID
#define rpc_binding_handle_t RPC_BINDING_HANDLE
#define rpc_binding_vector_t RPC_BINDING_VECTOR
#define uuid_vector_t UUID_VECTOR
#define RPC_C_BINDING_INFINITE_TIMEOUT 10
#define RPC_C_BINDING_MIN_TIMEOUT 0
#define RPC_C_BINDING_DEFAULT_TIMEOUT 5
#define RPC_C_BINDING_MAX_TIMEOUT 9
#define RPC_C_CANCEL_INFINITE_TIMEOUT (-1)
#define RPC_C_LISTEN_MAX_CALLS_DEFAULT 1234
#define RPC_C_PROTSEQ_MAX_REQS_DEFAULT 10
#define RPC_C_BIND_TO_ALL_NICS 1
#define RPC_C_USE_INTERNET_PORT 1
#define RPC_C_USE_INTRANET_PORT 2
#define RPC_MGR_EPV void
#define RPC_C_STATS_CALLS_IN 0
#define RPC_C_STATS_CALLS_OUT 1
#define RPC_C_STATS_PKTS_IN 2
#define RPC_C_STATS_PKTS_OUT 3
#define RPC_IF_AUTOLISTEN 0x0001
#define RPC_IF_OLE 2
#define RPC_C_MGMT_INQ_IF_IDS 0
#define RPC_C_MGMT_INQ_PRINC_NAME 1
#define RPC_C_MGMT_INQ_STATS 2
#define RPC_C_MGMT_IS_SERVER_LISTEN 3
#define RPC_C_MGMT_STOP_SERVER_LISTEN 4
#define RPC_C_EP_ALL_ELTS 0
#define RPC_C_EP_MATCH_BY_IF 1
#define RPC_C_EP_MATCH_BY_OBJ 2
#define RPC_C_EP_MATCH_BY_BOTH 3
#define RPC_C_VERS_ALL 1
#define RPC_C_VERS_COMPATIBLE 2
#define RPC_C_VERS_EXACT 3
#define RPC_C_VERS_MAJOR_ONLY 4
#define RPC_C_VERS_UPTO 5
#define DCE_C_ERROR_STRING_LEN 256
#define RPC_C_PARM_MAX_PACKET_LENGTH 1
#define RPC_C_PARM_BUFFER_LENGTH 2
#define RPC_C_AUTHN_LEVEL_DEFAULT 0
#define RPC_C_AUTHN_LEVEL_NONE 1
#define RPC_C_AUTHN_LEVEL_CONNECT 2
#define RPC_C_AUTHN_LEVEL_CALL 3
#define RPC_C_AUTHN_LEVEL_PKT 4
#define RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5
#define RPC_C_AUTHN_LEVEL_PKT_PRIVACY 6
#define RPC_C_IMP_LEVEL_ANONYMOUS 1
#define RPC_C_IMP_LEVEL_IDENTIFY 2
#define RPC_C_IMP_LEVEL_IMPERSONATE 3
#define RPC_C_IMP_LEVEL_DELEGATE 4
#define RPC_C_QOS_IDENTITY_STATIC 0
#define RPC_C_QOS_IDENTITY_DYNAMIC 1
#define RPC_C_QOS_CAPABILITIES_DEFAULT 0
#define RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH 1
#define RPC_C_PROTECT_LEVEL_DEFAULT(RPC_C_AUTHN_LEVEL_DEFAULT)
#define RPC_C_PROTECT_LEVEL_NONE(RPC_C_AUTHN_LEVEL_NONE)
#define RPC_C_PROTECT_LEVEL_CONNECT(RPC_C_AUTHN_LEVEL_CONNECT)
#define RPC_C_PROTECT_LEVEL_CALL(RPC_C_AUTHN_LEVEL_CALL)
#define RPC_C_PROTECT_LEVEL_PKT(RPC_C_AUTHN_LEVEL_PKT)
#define RPC_C_PROTECT_LEVEL_PKT_INTEGRITY(RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
#define RPC_C_PROTECT_LEVEL_PKT_PRIVACY(RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
#define RPC_C_AUTHN_NONE 0
#define RPC_C_AUTHN_DCE_PRIVATE 1
#define RPC_C_AUTHN_DCE_PUBLIC 2
#define RPC_C_AUTHN_DEC_PUBLIC 4
#define RPC_C_AUTHN_WINNT 10
#define RPC_C_AUTHN_DEFAULT 0xFFFFFFFF
#define RPC_C_SECURITY_QOS_VERSION L
#define SEC_WINNT_AUTH_IDENTITY_ANSI 0x1
#define SEC_WINNT_AUTH_IDENTITY_UNICODE 0x2
#define RPC_C_AUTHZ_NONE 0
#define RPC_C_AUTHZ_NAME 1
#define RPC_C_AUTHZ_DCE 2
#define RPC_C_AUTHZ_DEFAULT 0xFFFFFFFF

typedef I_RPC_HANDLE RPC_BINDING_HANDLE;
typedef RPC_BINDING_HANDLE handle_t;
typedef struct _RPC_BINDING_VECTOR {
	unsigned long Count;
	RPC_BINDING_HANDLE BindingH[1];
} RPC_BINDING_VECTOR;
typedef struct _UUID_VECTOR {
	unsigned long Count;
	UUID *Uuid[1];
} UUID_VECTOR;
typedef void *RPC_IF_HANDLE;
typedef struct _RPC_IF_ID {
	UUID Uuid;
	unsigned short VersMajor;
	unsigned short VersMinor;
} RPC_IF_ID;
typedef struct _RPC_POLICY {
	unsigned int Length ;
	unsigned long EndpointFlags ;
	unsigned long NICFlags ;
} RPC_POLICY,*PRPC_POLICY ;
typedef void __RPC_USER RPC_OBJECT_INQ_FN(UUID*,UUID*,RPC_STATUS*);
typedef RPC_STATUS RPC_IF_CALLBACK_FN(RPC_IF_HANDLE,void*);
typedef struct {
	unsigned int Count;
	unsigned long Stats[1];
} RPC_STATS_VECTOR;
typedef struct {
	unsigned long Count;
	RPC_IF_ID*IfId[1];
} RPC_IF_ID_VECTOR;
typedef void *RPC_AUTH_IDENTITY_HANDLE;
typedef void *RPC_AUTHZ_HANDLE;
typedef struct _RPC_SECURITY_QOS {
	unsigned long Version;
	unsigned long Capabilities;
	unsigned long IdentityTracking;
	unsigned long ImpersonationType;
} RPC_SECURITY_QOS,*PRPC_SECURITY_QOS;
typedef struct _SEC_WINNT_AUTH_IDENTITY_W {
	unsigned short *User;
	unsigned long UserLength;
	unsigned short *Domain;
	unsigned long DomainLength;
	unsigned short *Password;
	unsigned long PasswordLength;
	unsigned long Flags;
} SEC_WINNT_AUTH_IDENTITY_W,*PSEC_WINNT_AUTH_IDENTITY_W;
typedef struct _SEC_WINNT_AUTH_IDENTITY_A {
	unsigned char *User;
	unsigned long UserLength;
	unsigned char *Domain;
	unsigned long DomainLength;
	unsigned char *Password;
	unsigned long PasswordLength;
	unsigned long Flags;
} SEC_WINNT_AUTH_IDENTITY_A,*PSEC_WINNT_AUTH_IDENTITY_A;
typedef struct {
	unsigned char *UserName;
	unsigned char *ComputerName;
	unsigned short Privilege;
	unsigned long AuthFlags;
} RPC_CLIENT_INFORMATION1,* PRPC_CLIENT_INFORMATION1;
typedef I_RPC_HANDLE *RPC_EP_INQ_HANDLE;
typedef int(__RPC_API *RPC_MGMT_AUTHORIZATION_FN)(RPC_BINDING_HANDLE,unsigned long,RPC_STATUS*);

#ifdef RPC_UNICODE_SUPPORTED
typedef struct _RPC_PROTSEQ_VECTORA {
	unsigned int Count;
	unsigned char*Protseq[1];
} RPC_PROTSEQ_VECTORA;
typedef struct _RPC_PROTSEQ_VECTORW {
	unsigned int Count;
	unsigned short*Protseq[1];
} RPC_PROTSEQ_VECTORW;
RPC_STATUS RPC_ENTRY RpcBindingFromStringBindingA(unsigned char *,RPC_BINDING_HANDLE *);
RPC_STATUS RPC_ENTRY RpcBindingFromStringBindingW(unsigned short *,RPC_BINDING_HANDLE *);
RPC_STATUS RPC_ENTRY RpcBindingToStringBindingA(RPC_BINDING_HANDLE,unsigned char**);
RPC_STATUS RPC_ENTRY RpcBindingToStringBindingW(RPC_BINDING_HANDLE,unsigned short**);
RPC_STATUS RPC_ENTRY RpcStringBindingComposeA(unsigned char *,unsigned char *,unsigned char *,unsigned char *,unsigned char *,unsigned char **);
RPC_STATUS RPC_ENTRY RpcStringBindingComposeW(unsigned short *,unsigned short *,unsigned short *,unsigned short *,unsigned short *,unsigned short **);
RPC_STATUS RPC_ENTRY RpcStringBindingParseA(unsigned char *,unsigned char **,unsigned char **,unsigned char **,unsigned char **,unsigned char **);
RPC_STATUS RPC_ENTRY RpcStringBindingParseW(unsigned short *,unsigned short **,unsigned short **,unsigned short **,unsigned short **,unsigned short **);
RPC_STATUS RPC_ENTRY RpcStringFreeA(unsigned char**);
RPC_STATUS RPC_ENTRY RpcStringFreeW(unsigned short**);
RPC_STATUS RPC_ENTRY RpcNetworkIsProtseqValidA(unsigned char*);
RPC_STATUS RPC_ENTRY RpcNetworkIsProtseqValidW(unsigned short*);
RPC_STATUS RPC_ENTRY RpcNetworkInqProtseqsA(RPC_PROTSEQ_VECTORA**);
RPC_STATUS RPC_ENTRY RpcNetworkInqProtseqsW(RPC_PROTSEQ_VECTORW**);
RPC_STATUS RPC_ENTRY RpcProtseqVectorFreeA(RPC_PROTSEQ_VECTORA**);
RPC_STATUS RPC_ENTRY RpcProtseqVectorFreeW(RPC_PROTSEQ_VECTORW**);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqA(unsigned char*,unsigned int,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqW(unsigned short*,unsigned int,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqExA(unsigned char*,unsigned int MaxCalls,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqExW(unsigned short*,unsigned int,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEpA(unsigned char*,unsigned int,unsigned char*,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEpExA(unsigned char*,unsigned int,unsigned char*,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEpW(unsigned short*,unsigned int,unsigned short*,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEpExW(unsigned short*,unsigned int,unsigned short*,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqIfA(unsigned char*,unsigned int,RPC_IF_HANDLE,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqIfExA(unsigned char*,unsigned int,RPC_IF_HANDLE,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqIfW(unsigned short*,unsigned int,RPC_IF_HANDLE,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqIfExW(unsigned short*,unsigned int,RPC_IF_HANDLE,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcMgmtInqServerPrincNameA(RPC_BINDING_HANDLE,unsigned long,unsigned char**);
RPC_STATUS RPC_ENTRY RpcMgmtInqServerPrincNameW(RPC_BINDING_HANDLE,unsigned long,unsigned short**);
RPC_STATUS RPC_ENTRY RpcServerInqDefaultPrincNameA(unsigned long,unsigned char**);
RPC_STATUS RPC_ENTRY RpcServerInqDefaultPrincNameW(unsigned long,unsigned short**);
RPC_STATUS RPC_ENTRY RpcNsBindingInqEntryNameA(RPC_BINDING_HANDLE,unsigned long,unsigned char**);
RPC_STATUS RPC_ENTRY RpcNsBindingInqEntryNameW(RPC_BINDING_HANDLE,unsigned long,unsigned short**);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthClientA(RPC_BINDING_HANDLE,RPC_AUTHZ_HANDLE *,unsigned char**,unsigned long*,unsigned long*,unsigned long*);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthClientW(RPC_BINDING_HANDLE,RPC_AUTHZ_HANDLE *,unsigned short**,unsigned long*,unsigned long*,unsigned long*);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthInfoA(RPC_BINDING_HANDLE,unsigned char**,unsigned long*,unsigned long*,RPC_AUTH_IDENTITY_HANDLE*,unsigned long*);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthInfoW(RPC_BINDING_HANDLE,unsigned short**,unsigned long*,unsigned long*,RPC_AUTH_IDENTITY_HANDLE*,unsigned long*);
RPC_STATUS RPC_ENTRY RpcBindingSetAuthInfoA(RPC_BINDING_HANDLE,unsigned char*,unsigned long,unsigned long,RPC_AUTH_IDENTITY_HANDLE,unsigned long);
RPC_STATUS RPC_ENTRY RpcBindingSetAuthInfoExA(RPC_BINDING_HANDLE,unsigned char*,unsigned long,unsigned long,RPC_AUTH_IDENTITY_HANDLE,unsigned long,RPC_SECURITY_QOS*);
RPC_STATUS RPC_ENTRY RpcBindingSetAuthInfoW(RPC_BINDING_HANDLE,unsigned short*,unsigned long,unsigned long,RPC_AUTH_IDENTITY_HANDLE,unsigned long);
RPC_STATUS RPC_ENTRY RpcBindingSetAuthInfoExW(RPC_BINDING_HANDLE,unsigned short*,unsigned long,unsigned long,RPC_AUTH_IDENTITY_HANDLE,unsigned long,RPC_SECURITY_QOS*);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthInfoExA(RPC_BINDING_HANDLE,unsigned char**,unsigned long*,unsigned long*,RPC_AUTH_IDENTITY_HANDLE*,unsigned long*,unsigned long,RPC_SECURITY_QOS*);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthInfoExW(RPC_BINDING_HANDLE,unsigned short ** , unsigned long *, unsigned long *, RPC_AUTH_IDENTITY_HANDLE *, unsigned long *, unsigned long , RPC_SECURITY_QOS *);
typedef void(__RPC_USER *RPC_AUTH_KEY_RETRIEVAL_FN)(void*,unsigned short*,unsigned long,void**,RPC_STATUS*);
RPC_STATUS RPC_ENTRY RpcServerRegisterAuthInfoA(unsigned char*,unsigned long,RPC_AUTH_KEY_RETRIEVAL_FN,void*);
RPC_STATUS RPC_ENTRY RpcServerRegisterAuthInfoW(unsigned short*,unsigned long,RPC_AUTH_KEY_RETRIEVAL_FN,void*);
RPC_STATUS RPC_ENTRY UuidToStringA(UUID*,unsigned char**);
RPC_STATUS RPC_ENTRY UuidFromStringA(unsigned char*,UUID*);
RPC_STATUS RPC_ENTRY UuidToStringW(UUID*,unsigned short**);
RPC_STATUS RPC_ENTRY UuidFromStringW(unsigned short*,UUID*);
RPC_STATUS RPC_ENTRY RpcEpRegisterNoReplaceA(RPC_IF_HANDLE,RPC_BINDING_VECTOR*,UUID_VECTOR*,unsigned char*);
RPC_STATUS RPC_ENTRY RpcEpRegisterNoReplaceW(RPC_IF_HANDLE,RPC_BINDING_VECTOR*, UUID_VECTOR*,unsigned short*);
RPC_STATUS RPC_ENTRY RpcEpRegisterA(RPC_IF_HANDLE,RPC_BINDING_VECTOR*,UUID_VECTOR*,unsigned char*);
RPC_STATUS RPC_ENTRY RpcEpRegisterW(RPC_IF_HANDLE,RPC_BINDING_VECTOR*,UUID_VECTOR*,unsigned short*);
RPC_STATUS RPC_ENTRY DceErrorInqTextA(RPC_STATUS,unsigned char*);
RPC_STATUS RPC_ENTRY DceErrorInqTextW(RPC_STATUS,unsigned short*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqNextA(RPC_EP_INQ_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE*,UUID*,unsigned char**);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqNextW(RPC_EP_INQ_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE*,UUID*,unsigned short**);
#ifdef UNICODE
#define RPC_PROTSEQ_VECTOR RPC_PROTSEQ_VECTORW
#define SEC_WINNT_AUTH_IDENTITY SEC_WINNT_AUTH_IDENTITY_W
#define PSEC_WINNT_AUTH_IDENTITY PSEC_WINNT_AUTH_IDENTITY_W
#define _SEC_WINNT_AUTH_IDENTITY _SEC_WINNT_AUTH_IDENTITY_W
#define RpcMgmtEpEltInqNext RpcMgmtEpEltInqNextW
#define RpcBindingFromStringBinding RpcBindingFromStringBindingW
#define RpcBindingToStringBinding RpcBindingToStringBindingW
#define RpcStringBindingCompose RpcStringBindingComposeW
#define RpcStringBindingParse RpcStringBindingParseW
#define RpcStringFree RpcStringFreeW
#define RpcNetworkIsProtseqValid RpcNetworkIsProtseqValidW
#define RpcNetworkInqProtseqs RpcNetworkInqProtseqsW
#define RpcProtseqVectorFree RpcProtseqVectorFreeW
#define RpcServerUseProtseq RpcServerUseProtseqW
#define RpcServerUseProtseqEx RpcServerUseProtseqExW
#define RpcServerUseProtseqEp RpcServerUseProtseqEpW
#define RpcServerUseProtseqEpEx RpcServerUseProtseqEpExW
#define RpcServerUseProtseqIf RpcServerUseProtseqIfW
#define RpcServerUseProtseqIfEx RpcServerUseProtseqIfExW
#define RpcMgmtInqServerPrincName RpcMgmtInqServerPrincNameW
#define RpcServerInqDefaultPrincName RpcServerInqDefaultPrincNameW
#define RpcNsBindingInqEntryName RpcNsBindingInqEntryNameW
#define RpcBindingInqAuthClient RpcBindingInqAuthClientW
#define RpcBindingInqAuthInfo RpcBindingInqAuthInfoW
#define RpcBindingSetAuthInfo RpcBindingSetAuthInfoW
#define RpcServerRegisterAuthInfo RpcServerRegisterAuthInfoW
#define RpcBindingInqAuthInfoEx RpcBindingInqAuthInfoExW
#define RpcBindingSetAuthInfoEx RpcBindingSetAuthInfoExW
#define UuidFromString UuidFromStringW
#define UuidToString UuidToStringW
#define RpcEpRegisterNoReplace RpcEpRegisterNoReplaceW
#define RpcEpRegister RpcEpRegisterW
#define DceErrorInqText DceErrorInqTextW
#else /* UNICODE */
#define RPC_PROTSEQ_VECTOR RPC_PROTSEQ_VECTORA
#define SEC_WINNT_AUTH_IDENTITY SEC_WINNT_AUTH_IDENTITY_A
#define PSEC_WINNT_AUTH_IDENTITY PSEC_WINNT_AUTH_IDENTITY_A
#define _SEC_WINNT_AUTH_IDENTITY _SEC_WINNT_AUTH_IDENTITY_A
#define RpcMgmtEpEltInqNext RpcMgmtEpEltInqNextA
#define RpcBindingFromStringBinding RpcBindingFromStringBindingA
#define RpcBindingToStringBinding RpcBindingToStringBindingA
#define RpcStringBindingCompose RpcStringBindingComposeA
#define RpcStringBindingParse RpcStringBindingParseA
#define RpcStringFree RpcStringFreeA
#define RpcNetworkIsProtseqValid RpcNetworkIsProtseqValidA
#define RpcNetworkInqProtseqs RpcNetworkInqProtseqsA
#define RpcProtseqVectorFree RpcProtseqVectorFreeA
#define RpcServerUseProtseq RpcServerUseProtseqA
#define RpcServerUseProtseqEx RpcServerUseProtseqExA
#define RpcServerUseProtseqEp RpcServerUseProtseqEpA
#define RpcServerUseProtseqEpEx RpcServerUseProtseqEpExA
#define RpcServerUseProtseqIf RpcServerUseProtseqIfA
#define RpcServerUseProtseqIfEx RpcServerUseProtseqIfExA
#define RpcMgmtInqServerPrincName RpcMgmtInqServerPrincNameA
#define RpcServerInqDefaultPrincName RpcServerInqDefaultPrincNameA
#define RpcNsBindingInqEntryName RpcNsBindingInqEntryNameA
#define RpcBindingInqAuthClient RpcBindingInqAuthClientA
#define RpcBindingInqAuthInfo RpcBindingInqAuthInfoA
#define RpcBindingSetAuthInfo RpcBindingSetAuthInfoA
#define RpcServerRegisterAuthInfo RpcServerRegisterAuthInfoA
#define RpcBindingInqAuthInfoEx RpcBindingInqAuthInfoExA
#define RpcBindingSetAuthInfoEx RpcBindingSetAuthInfoExA
#define UuidFromString UuidFromStringA
#define UuidToString UuidToStringA
#define RpcEpRegisterNoReplace RpcEpRegisterNoReplaceA
#define RpcEpRegister RpcEpRegisterA
#define DceErrorInqText DceErrorInqTextA
#endif /* UNICODE */
#else /* RPC_UNICODE_SUPPORTED */
typedef struct _RPC_PROTSEQ_VECTOR {
	unsigned int Count;
	unsigned char* Protseq[1];
} RPC_PROTSEQ_VECTOR;
RPC_STATUS RPC_ENTRY RpcBindingFromStringBinding(unsigned char *,RPC_BINDING_HANDLE *);
RPC_STATUS RPC_ENTRY RpcBindingToStringBinding(RPC_BINDING_HANDLE,unsigned char **);
RPC_STATUS RPC_ENTRY RpcStringBindingCompose(unsigned char *,unsigned char *,unsigned char *,unsigned char *,unsigned char *,unsigned char **);
RPC_STATUS RPC_ENTRY RpcStringBindingParse(unsigned char *,unsigned char **,unsigned char **,unsigned char **,unsigned char **,unsigned char **);
RPC_STATUS RPC_ENTRY RpcStringFree(unsigned char**);
RPC_STATUS RPC_ENTRY RpcNetworkIsProtseqValid(unsigned char*);
RPC_STATUS RPC_ENTRY RpcNetworkInqProtseqs(RPC_PROTSEQ_VECTOR **);
RPC_STATUS RPC_ENTRY RpcServerInqBindings(RPC_BINDING_VECTOR **);
RPC_STATUS RPC_ENTRY RpcServerUseProtseq(unsigned char*,unsigned int,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEx(unsigned char*,unsigned int,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEp(unsigned char*,unsigned int,unsigned char*,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqEpEx(unsigned char*,unsigned int,unsigned char*,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqIf(unsigned char*,unsigned int,RPC_IF_HANDLE,void*);
RPC_STATUS RPC_ENTRY RpcServerUseProtseqIfEx(unsigned char*,unsigned int,RPC_IF_HANDLE,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcMgmtInqServerPrincName(RPC_BINDING_HANDLE,unsigned long,unsigned char**);
RPC_STATUS RPC_ENTRY RpcServerInqDefaultPrincName(unsigned long,unsigned char**);
RPC_STATUS RPC_ENTRY RpcNsBindingInqEntryName(RPC_BINDING_HANDLE,unsigned long,unsigned char**);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthClient(RPC_BINDING_HANDLE,RPC_AUTHZ_HANDLE*,unsigned char**,unsigned long*,unsigned long*,unsigned long*);
RPC_STATUS RPC_ENTRY RpcBindingInqAuthInfo(RPC_BINDING_HANDLE,unsigned char **,unsigned long *,unsigned long *,RPC_AUTH_IDENTITY_HANDLE *,unsigned long *);
RPC_STATUS RPC_ENTRY RpcBindingSetAuthInfo(RPC_BINDING_HANDLE,unsigned char *,unsigned long,unsigned long,RPC_AUTH_IDENTITY_HANDLE,unsigned long);
typedef void(__RPC_USER *RPC_AUTH_KEY_RETRIEVAL_FN)(void*,unsigned char*,unsigned long,void**,RPC_STATUS*);
RPC_STATUS RPC_ENTRY RpcServerRegisterAuthInfo(unsigned char*,unsigned long,RPC_AUTH_KEY_RETRIEVAL_FN,void*);
RPC_STATUS RPC_ENTRY UuidToString(UUID*,unsigned char**);
RPC_STATUS RPC_ENTRY UuidFromString(unsigned char*,UUID*);
RPC_STATUS RPC_ENTRY RpcEpRegisterNoReplace(RPC_IF_HANDLE,RPC_BINDING_VECTOR*,UUID_VECTOR*,unsigned char*);
RPC_STATUS RPC_ENTRY RpcEpRegister(RPC_IF_HANDLE,RPC_BINDING_VECTOR*,UUID_VECTOR*,unsigned char*);
RPC_STATUS RPC_ENTRY DceErrorInqText(RPC_STATUS,unsigned char*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqNext(RPC_EP_INQ_HANDLE,RPC_IF_ID *,RPC_BINDING_HANDLE *,unsigned char **);
#endif /* RPC_UNICODE_SUPPORTED */

RPC_STATUS RPC_ENTRY RpcBindingCopy(RPC_BINDING_HANDLE,RPC_BINDING_HANDLE*);
RPC_STATUS RPC_ENTRY RpcBindingFree(RPC_BINDING_HANDLE*);
RPC_STATUS RPC_ENTRY RpcBindingInqObject(RPC_BINDING_HANDLE,UUID *);
RPC_STATUS RPC_ENTRY RpcBindingReset(RPC_BINDING_HANDLE);
RPC_STATUS RPC_ENTRY RpcBindingSetObject(RPC_BINDING_HANDLE,UUID *);
RPC_STATUS RPC_ENTRY RpcMgmtInqDefaultProtectLevel(unsigned long,unsigned long *);
RPC_STATUS RPC_ENTRY RpcBindingVectorFree(RPC_BINDING_VECTOR **);
RPC_STATUS RPC_ENTRY RpcIfInqId(RPC_IF_HANDLE,RPC_IF_ID *);
RPC_STATUS RPC_ENTRY RpcMgmtInqComTimeout(RPC_BINDING_HANDLE,unsigned int*);
RPC_STATUS RPC_ENTRY RpcMgmtSetComTimeout(RPC_BINDING_HANDLE,unsigned int);
RPC_STATUS RPC_ENTRY RpcMgmtSetCancelTimeout(long Timeout);
RPC_STATUS RPC_ENTRY RpcObjectInqType(UUID *,UUID *);
RPC_STATUS RPC_ENTRY RpcObjectSetInqFn(RPC_OBJECT_INQ_FN *);
RPC_STATUS RPC_ENTRY RpcObjectSetType(UUID *,UUID *);
RPC_STATUS RPC_ENTRY RpcProtseqVectorFree(RPC_PROTSEQ_VECTOR **);
RPC_STATUS RPC_ENTRY RpcServerInqIf(RPC_IF_HANDLE,UUID*,RPC_MGR_EPV**);
RPC_STATUS RPC_ENTRY RpcServerListen(unsigned int,unsigned int,unsigned int);
RPC_STATUS RPC_ENTRY RpcServerRegisterIf(RPC_IF_HANDLE,UUID*,RPC_MGR_EPV*);
RPC_STATUS RPC_ENTRY RpcServerRegisterIfEx(RPC_IF_HANDLE,UUID*,RPC_MGR_EPV*,unsigned int,unsigned int,RPC_IF_CALLBACK_FN*);
RPC_STATUS RPC_ENTRY RpcServerRegisterIf2(RPC_IF_HANDLE,UUID*,RPC_MGR_EPV*,unsigned int,unsigned int,unsigned int,RPC_IF_CALLBACK_FN*);
RPC_STATUS RPC_ENTRY RpcServerUnregisterIf(RPC_IF_HANDLE,UUID*,unsigned int);
RPC_STATUS RPC_ENTRY RpcServerUseAllProtseqs(unsigned int,void*);
RPC_STATUS RPC_ENTRY RpcServerUseAllProtseqsEx(unsigned int,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcServerUseAllProtseqsIf(unsigned int,RPC_IF_HANDLE,void*);
RPC_STATUS RPC_ENTRY RpcServerUseAllProtseqsIfEx(unsigned int,RPC_IF_HANDLE,void*,PRPC_POLICY);
RPC_STATUS RPC_ENTRY RpcMgmtStatsVectorFree(RPC_STATS_VECTOR**);
RPC_STATUS RPC_ENTRY RpcMgmtInqStats(RPC_BINDING_HANDLE,RPC_STATS_VECTOR**);
RPC_STATUS RPC_ENTRY RpcMgmtIsServerListening(RPC_BINDING_HANDLE);
RPC_STATUS RPC_ENTRY RpcMgmtStopServerListening(RPC_BINDING_HANDLE);
RPC_STATUS RPC_ENTRY RpcMgmtWaitServerListen(void);
RPC_STATUS RPC_ENTRY RpcMgmtSetServerStackSize(unsigned long);
void RPC_ENTRY RpcSsDontSerializeContext(void);
RPC_STATUS RPC_ENTRY RpcMgmtEnableIdleCleanup(void);
RPC_STATUS RPC_ENTRY RpcMgmtInqIfIds(RPC_BINDING_HANDLE,RPC_IF_ID_VECTOR**);
RPC_STATUS RPC_ENTRY RpcIfIdVectorFree(RPC_IF_ID_VECTOR**);
RPC_STATUS RPC_ENTRY RpcEpResolveBinding(RPC_BINDING_HANDLE,RPC_IF_HANDLE);
RPC_STATUS RPC_ENTRY RpcBindingServerFromClient(RPC_BINDING_HANDLE,RPC_BINDING_HANDLE*);
DECLSPEC_NORETURN void  RPC_ENTRY RpcRaiseException(RPC_STATUS);
RPC_STATUS RPC_ENTRY RpcTestCancel(void);
RPC_STATUS RPC_ENTRY RpcCancelThread(void*);
RPC_STATUS RPC_ENTRY UuidCreate(UUID*);
signed int RPC_ENTRY UuidCompare(UUID*,UUID*, RPC_STATUS*);
RPC_STATUS RPC_ENTRY UuidCreateNil(UUID*);
int RPC_ENTRY UuidEqual(UUID*,UUID*, RPC_STATUS*);
unsigned short RPC_ENTRY UuidHash(UUID*,RPC_STATUS*);
int RPC_ENTRY UuidIsNil(UUID*,RPC_STATUS*);
RPC_STATUS RPC_ENTRY RpcEpUnregister(RPC_IF_HANDLE,RPC_BINDING_VECTOR*,UUID_VECTOR*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqBegin(RPC_BINDING_HANDLE,unsigned long,RPC_IF_ID*,unsigned long,UUID*,RPC_EP_INQ_HANDLE*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqDone(RPC_EP_INQ_HANDLE*);
RPC_STATUS RPC_ENTRY RpcMgmtEpUnregister(RPC_BINDING_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE,UUID*);
RPC_STATUS RPC_ENTRY RpcMgmtSetAuthorizationFn(RPC_MGMT_AUTHORIZATION_FN);
RPC_STATUS RPC_ENTRY RpcMgmtInqParameter(unsigned int,unsigned long*);
RPC_STATUS RPC_ENTRY RpcMgmtSetParameter(unsigned int,unsigned long);
RPC_STATUS RPC_ENTRY RpcMgmtBindingInqParameter(RPC_BINDING_HANDLE,unsigned int,unsigned long*);
RPC_STATUS RPC_ENTRY RpcMgmtBindingSetParameter(RPC_BINDING_HANDLE,unsigned int,unsigned long);

#if _WIN32_WINNT >= 0x0500
RPC_STATUS RPC_ENTRY UuidCreateSequential(UUID*);
#endif

#include <rpcdcep.h>
#ifdef __cplusplus
}
#endif
#endif
