#ifndef _RPCDCE2_H
#define _RPCDCE2_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <rpcdce.h>

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
#define RPC_C_MGMT_INQ_IF_IDS 0
#define RPC_C_MGMT_INQ_PRINC_NAME 1
#define RPC_C_MGMT_INQ_STATS 2
#define RPC_C_MGMT_IS_SERVER_LISTEN 3
#define RPC_C_MGMT_STOP_SERVER_LISTEN 4

int RPC_ENTRY UuidCompare(UUID*,UUID*,RPC_STATUS*);
RPC_STATUS RPC_ENTRY UuidCreateNil(UUID*);
int RPC_ENTRY UuidEqual(UUID*,UUID*,RPC_STATUS*);
unsigned short RPC_ENTRY UuidHash(UUID*,RPC_STATUS*);
int RPC_ENTRY UuidIsNil(UUID*,RPC_STATUS*);
#ifdef RPC_UNICODE_SUPPORTED
RPC_STATUS RPC_ENTRY DceErrorInqTextA(RPC_STATUS,unsigned char*);
RPC_STATUS RPC_ENTRY DceErrorInqTextW(RPC_STATUS,unsigned short*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqNextA(RPC_EP_INQ_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE*,UUID*,unsigned char**);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqNextW(RPC_EP_INQ_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE*,UUID*,unsigned short**);
#ifdef UNICODE
#define RpcMgmtEpEltInqNext RpcMgmtEpEltInqNextW
#define DceErrorInqText DceErrorInqTextW
#else
#define RpcMgmtEpEltInqNext RpcMgmtEpEltInqNextA
#define DceErrorInqText DceErrorInqTextA
#endif /* UNICODE */
#else /* RPC_UNICODE_SUPPORTED */
RPC_STATUS RPC_ENTRY DceErrorInqText(RPC_STATUS,unsigned char*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqNext(RPC_EP_INQ_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE*,UUID*,unsigned char**);
#endif
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqBegin(RPC_BINDING_HANDLE,unsigned long,RPC_IF_ID*,unsigned long,UUID*,RPC_EP_INQ_HANDLE*);
RPC_STATUS RPC_ENTRY RpcMgmtEpEltInqDone(RPC_EP_INQ_HANDLE*);
RPC_STATUS RPC_ENTRY RpcMgmtEpUnregister(RPC_BINDING_HANDLE,RPC_IF_ID*,RPC_BINDING_HANDLE,UUID*);
RPC_STATUS RPC_ENTRY RpcMgmtSetAuthorizationFn(RPC_MGMT_AUTHORIZATION_FN);
#ifdef __cplusplus
}
#endif
#endif
