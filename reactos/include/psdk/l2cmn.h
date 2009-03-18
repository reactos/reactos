#ifndef _L2CMN_H
#define _L2CMN_H

typedef struct _L2_NOTIFICATION_DATA {
    DWORD NotificationSource;
    DWORD NotificationCode;
    GUID InterfaceGuid;
    DWORD dwDataSize;
#if defined(__midl) || defined(__WIDL__)
    [unique, size_is(dwDataSize)] PBYTE pData;
#else
    PVOID pData;
#endif
} L2_NOTIFICATION_DATA, *PL2_NOTIFICATION_DATA;

#endif
