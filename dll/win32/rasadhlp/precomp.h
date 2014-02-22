#ifndef _RASADHLP_H
#define _RASADHLP_H

#define _WIN32_WINNT 0x502
#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdarg.h>

/* PSDK Headers */
#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <wsnetbs.h>
#include <wininet.h>

/* These should go in rasadhlp.h */
#define FILE_DEVICE_ACD                         0x000000F1
#define _ACD_CTL_CODE(function, method, access) \
    CTL_CODE(FILE_DEVICE_ACD, function, method, access)

#define IOCTL_ACD_RESET \
    _ACD_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_ACD_ENABLE \
    _ACD_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_ACD_NOTIFICATION \
    _ACD_CTL_CODE(2, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_ACD_KEEPALIVE \
    _ACD_CTL_CODE(3, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_ACD_COMPLETION \
    _ACD_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_ACD_CONNECT_ADDRESS \
    _ACD_CTL_CODE(5, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef enum
{
    AutoDialIp,
    AutoDialIpx,
    AutoDialNetBios,
    AutoDialIpHost
} AUTODIAL_FAMILY;

typedef enum
{
    ConnectionIpxLana,
    ConnectionIp,
    ConnectionIpHost,
    ConnectionNetBiosMac,
} CONNECTION_FAMILY;

typedef struct _AUTODIAL_ADDR
{
    AUTODIAL_FAMILY Family;
    union
    {
        IN_ADDR Ip4Address;
        CHAR IpxNode[6];
        CHAR NetBiosAddress[NETBIOS_NAME_LENGTH];
        CHAR HostName[INTERNET_MAX_PATH_LENGTH];
    };
} AUTODIAL_ADDR, *PAUTODIAL_ADDR;

typedef struct _AUTODIAL_CONN
{
    CONNECTION_FAMILY Family;
    union
    {
        UCHAR IpxLana;
        ULONG Ip4Address;
        WCHAR ConnectionName[32];
        CHAR NetBiosMac[6];
    };
} AUTODIAL_CONN, *PAUTODIAL_CONN;

typedef struct _AUTODIAL_COMMAND
{
    AUTODIAL_ADDR Address;
    BOOL NewConnection;
    AUTODIAL_CONN Connection;
} AUTODIAL_COMMAND, *PAUTODIAL_COMMAND;

BOOLEAN
WINAPI
AcsHlpNoteNewConnection(
    IN PAUTODIAL_ADDR ConnectionAddress,
    IN PAUTODIAL_CONN Connection
);

BOOLEAN
WINAPI
AcsHlpAttemptConnection(
    IN PAUTODIAL_ADDR ConnectionAddress
);

#endif /* _RASADHLP_H */
