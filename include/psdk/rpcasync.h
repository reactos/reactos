
#ifndef __RPCASYNC_H__
#define __RPCASYNC_H__

#if _MSC_VER > 1000
#pragma once
#endif

#if defined(__RPC_WIN64__)
#include <pshpack8.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _RPC_NOTIFICATION_TYPES
{
    RpcNotificationTypeNone,
    RpcNotificationTypeEvent,
    RpcNotificationTypeApc,
    RpcNotificationTypeIoc,
    RpcNotificationTypeHwnd,
    RpcNotificationTypeCallback
} RPC_NOTIFICATION_TYPES;

typedef enum _RPC_ASYNC_EVENT
{
    RpcCallComplete,
    RpcSendComplete,
    RpcReceiveComplete
} RPC_ASYNC_EVENT;

typedef enum tagExtendedErrorParamTypes
{
    eeptAnsiString = 1,
    eeptUnicodeString,
    eeptLongVal,
    eeptShortVal,
    eeptPointerVal,
    eeptNone,
    eeptBinary
} ExtendedErrorParamTypes;

typedef struct tagBinaryParam
{
    void *Buffer;
    short Size;
} BinaryParam;

typedef struct tagRPC_EE_INFO_PARAM
{
    ExtendedErrorParamTypes ParameterType;
    union
        {
        LPSTR AnsiString;
        LPWSTR UnicodeString;
        long LVal;
        short SVal;
        ULONGLONG PVal;
        BinaryParam BVal;
        } u;
} RPC_EE_INFO_PARAM;

#define MaxNumberOfEEInfoParams 4
#define RPC_EEINFO_VERSION 1
#define EEInfoPreviousRecordsMissing 1
#define EEInfoNextRecordsMissing 2 
#define EEInfoUseFileTime 4
#define EEInfoGCCOM 11
#define EEInfoGCFRS 12
#define RPC_CALL_ATTRIBUTES_VERSION (1)
#define RPC_QUERY_SERVER_PRINCIPAL_NAME (2)
#define RPC_QUERY_CLIENT_PRINCIPAL_NAME (4)


struct _RPC_ASYNC_STATE;

typedef void RPC_ENTRY
    RPCNOTIFICATION_ROUTINE (struct _RPC_ASYNC_STATE *pAsync, void *Context, RPC_ASYNC_EVENT Event);

typedef RPCNOTIFICATION_ROUTINE *PFN_RPCNOTIFICATION_ROUTINE;

typedef struct _RPC_ASYNC_STATE 
{
    unsigned intSize;
    unsigned long Signature;
    long Lock;
    unsigned long Flags;
    void *StubInfo;
    void *UserInfo;
    void *RuntimeInfo;
    RPC_ASYNC_EVENT Event;

    RPC_NOTIFICATION_TYPES NotificationType;
    union
    {
        struct
        {
            PFN_RPCNOTIFICATION_ROUTINE NotificationRoutine;
            HANDLE hThread;
        } APC;
        struct
        {
            HANDLE hIOPort;
            DWORD dwNumberOfBytesTransferred;
            DWORD_PTR dwCompletionKey;
            LPOVERLAPPED lpOverlapped;
        } IOC;
        struct
        {
            HWND hWnd;
            UINT Msg;
        } HWND;
        HANDLE hEvent;
        PFN_RPCNOTIFICATION_ROUTINE NotificationRoutine;
    } u;

    LONG_PTR Reserved[4];
    } RPC_ASYNC_STATE, *PRPC_ASYNC_STATE;

typedef struct tagRPC_EXTENDED_ERROR_INFO
{
    ULONG Version;
    LPWSTR ComputerName;
    ULONG ProcessID;
    union
    {
        SYSTEMTIME SystemTime;
        FILETIME FileTime;
    } u;
    ULONG GeneratingComponent;
    ULONG Status;
    USHORT DetectionLocation;
    USHORT Flags;
    int NumberOfParameters;
    RPC_EE_INFO_PARAM Parameters[MaxNumberOfEEInfoParams];
} RPC_EXTENDED_ERROR_INFO;

typedef struct tagRPC_ERROR_ENUM_HANDLE
{
    ULONG Signature;
    void *CurrentPos;
    void *Head;
} RPC_ERROR_ENUM_HANDLE;

typedef struct tagRPC_CALL_ATTRIBUTES_V1_W
{
    unsigned int Version;
    unsigned long Flags;
    unsigned long ServerPrincipalNameBufferLength;
    unsigned short *ServerPrincipalName;
    unsigned long ClientPrincipalNameBufferLength;
    unsigned short *ClientPrincipalName;
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    BOOL NullSession;
} RPC_CALL_ATTRIBUTES_V1_W;

typedef struct tagRPC_CALL_ATTRIBUTES_V1_A
{
    unsigned int Version;
    unsigned long Flags;
    unsigned long ServerPrincipalNameBufferLength;
    unsigned char *ServerPrincipalName;
    unsigned long ClientPrincipalNameBufferLength;
    unsigned char *ClientPrincipalName;
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    BOOL NullSession;
} RPC_CALL_ATTRIBUTES_V1_A;

#define RPC_ASYNC_VERSION_1_0 sizeof(RPC_ASYNC_STATE)
#define RPC_C_NOTIFY_ON_SEND_COMPLETE 0x1
#define RPC_C_INFINITE_TIMEOUT INFINITE
#define RpcAsyncGetCallHandle(pAsync) (((PRPC_ASYNC_STATE) pAsync)->RuntimeInfo)



RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncInitializeHandle (
    PRPC_ASYNC_STATE pAsync,
    unsigned int Size);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncRegisterInfo ( PRPC_ASYNC_STATE pAsync);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncGetCallStatus (PRPC_ASYNC_STATE pAsync);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncCompleteCall (
    PRPC_ASYNC_STATE pAsync,
    void *Reply);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncAbortCall (
    PRPC_ASYNC_STATE pAsync,
    unsigned long ExceptionCode) ;

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncCancelCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN BOOL fAbort) ;

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcAsyncCleanupThread ( IN DWORD dwTimeout);

RPCRTAPI RPC_STATUS  RPC_ENTRY
RpcErrorStartEnumeration (IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorGetNextRecord (
    IN RPC_ERROR_ENUM_HANDLE *EnumHandle, 
    IN BOOL CopyStrings,
    OUT RPC_EXTENDED_ERROR_INFO *ErrorInfo);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorEndEnumeration (IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorResetEnumeration (IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorGetNumberOfRecords (
    IN RPC_ERROR_ENUM_HANDLE *EnumHandle, 
    OUT int *Records);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorSaveErrorInfo (
    IN RPC_ERROR_ENUM_HANDLE *EnumHandle, 
    OUT PVOID *ErrorBlob,
    OUT size_t *BlobSize);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorLoadErrorInfo (
    IN PVOID ErrorBlob,
    IN size_t BlobSize,
    OUT RPC_ERROR_ENUM_HANDLE *EnumHandle);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcErrorAddRecord (IN RPC_EXTENDED_ERROR_INFO *ErrorInfo);

RPCRTAPI void RPC_ENTRY
RpcErrorClearInformation (void);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcGetAuthorizationContextForClient (
    IN RPC_BINDING_HANDLE ClientBinding OPTIONAL,
    IN BOOL ImpersonateOnReturn,
    IN PVOID Reserved1,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Reserved2,
    IN DWORD Reserved3,
    IN PVOID Reserved4,
    OUT PVOID *pAuthzClientContext);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcFreeAuthorizationContext (IN OUT PVOID *pAuthzClientContext);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcSsContextLockExclusive (IN RPC_BINDING_HANDLE ServerBindingHandle, IN PVOID UserContext);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcSsContextLockShared (
    IN RPC_BINDING_HANDLE ServerBindingHandle,
    IN PVOID UserContext);


RPCRTAPI RPC_STATUS RPC_ENTRY
RpcServerInqCallAttributesW (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    IN OUT void *RpcCallAttributes);

RPCRTAPI RPC_STATUS RPC_ENTRY
RpcServerInqCallAttributesA (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    IN OUT void *RpcCallAttributes);





RPC_STATUS RPC_ENTRY
I_RpcAsyncSetHandle (
    IN  PRPC_MESSAGE Message,
    IN  PRPC_ASYNC_STATE pAsync);

RPC_STATUS RPC_ENTRY
I_RpcAsyncAbortCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode) ;

int RPC_ENTRY
I_RpcExceptionFilter (unsigned long ExceptionCode);


#ifdef UNICODE
    #define RPC_CALL_ATTRIBUTES_V1 RPC_CALL_ATTRIBUTES_V1_W
    #define RpcServerInqCallAttributes RpcServerInqCallAttributesW
#else
    #define RPC_CALL_ATTRIBUTES_V1 RPC_CALL_ATTRIBUTES_V1_A
    #define RpcServerInqCallAttributes RpcServerInqCallAttributesA
#endif 

typedef RPC_CALL_ATTRIBUTES_V1 RPC_CALL_ATTRIBUTES;

#ifdef __cplusplus
}
#endif

#if defined(__RPC_WIN64__)
#include <poppack.h>
#endif

#endif


