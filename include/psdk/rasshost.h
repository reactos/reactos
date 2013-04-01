#ifndef _RASSHOST_H_
#define _RASSHOST_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <ras.h>
#include <mprapi.h>

#define SECURITYMSG_SUCCESS       1
#define SECURITYMSG_FAILURE       2
#define SECURITYMSG_ERROR         3

typedef DWORD HPORT;

typedef struct _SECURITY_MESSAGE
{
    DWORD  dwMsgId;
    HPORT  hPort;
    DWORD  dwError;
    CHAR   UserName[UNLEN + 1];
    CHAR   Domain[DNLEN + 1];
} SECURITY_MESSAGE, *PSECURITY_MESSAGE;

typedef struct _RAS_SECURITY_INFO
{
    DWORD  LastError;
    DWORD  BytesReceived;
    CHAR   DeviceName[MAX_DEVICE_NAME + 1];
} RAS_SECURITY_INFO, *PRAS_SECURITY_INFO;

typedef DWORD (WINAPI *RASSECURITYPROC)();

VOID WINAPI RasSecurityDialogComplete(IN SECURITY_MESSAGE* pSecMsg);
DWORD WINAPI RasSecurityDialogBegin(IN HPORT hPort, IN PBYTE pSendBuf, IN DWORD SendBufSize, IN PBYTE pRecvBuf, IN DWORD RecvBufSize, IN VOID (WINAPI* RasSecurityDialogComplete)(SECURITY_MESSAGE*));
DWORD WINAPI RasSecurityDialogEnd(IN HPORT hPort);
DWORD WINAPI RasSecurityDialogSend(IN HPORT hPort, IN PBYTE pBuffer, IN WORD BufferLength);
DWORD WINAPI RasSecurityDialogReceive(IN HPORT hPort, IN PBYTE pBuffer, IN PWORD pBufferLength, IN DWORD Timeout, IN HANDLE hEvent);
DWORD WINAPI RasSecurityDialogGetInfo(IN HPORT hPort, IN RAS_SECURITY_INFO* pBuffer);

#ifdef __cplusplus
}
#endif
#endif
