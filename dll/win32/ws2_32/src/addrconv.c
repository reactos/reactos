/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32/src/addrconv.c
 * PURPOSE:     Address and Port Conversion Support
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *******************************************************************/

#ifndef BE

/* DWORD network to host byte order conversion for little endian machines */
#define DN2H(dw) \
  ((((dw) & 0xFF000000L) >> 24) | \
   (((dw) & 0x00FF0000L) >> 8) | \
     (((dw) & 0x0000FF00L) << 8) | \
     (((dw) & 0x000000FFL) << 24))

/* DWORD host to network byte order conversion for little endian machines */
#define DH2N(dw) \
    ((((dw) & 0xFF000000L) >> 24) | \
     (((dw) & 0x00FF0000L) >> 8) | \
     (((dw) & 0x0000FF00L) << 8) | \
     (((dw) & 0x000000FFL) << 24))

/* WORD network to host order conversion for little endian machines */
#define WN2H(w) \
    ((((w) & 0xFF00) >> 8) | \
     (((w) & 0x00FF) << 8))

/* WORD host to network byte order conversion for little endian machines */
#define WH2N(w) \
    ((((w) & 0xFF00) >> 8) | \
     (((w) & 0x00FF) << 8))

#else /* BE */

/* DWORD network to host byte order conversion for big endian machines */
#define DN2H(dw) \
    (dw)

/* DWORD host to network byte order conversion big endian machines */
#define DH2N(dw) \
    (dw)

/* WORD network to host order conversion for big endian machines */
#define WN2H(w) \
    (w)

/* WORD host to network byte order conversion for big endian machines */
#define WH2N(w) \
    (w)

#endif /* BE */

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
ULONG
WSAAPI
inet_addr(IN  CONST CHAR FAR* cp)
{
    register u_long val, base, n;
    register unsigned char c;
    u_long parts[4], *pp = parts;
    if (!cp) return INADDR_ANY;
    if (!isdigit(*cp)) return INADDR_NONE;

again:
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, other=decimal.
     */
    val = 0; base = 10;
    if (*cp == '0') {
        if (*++cp == 'x' || *cp == 'X')
            base = 16, cp++;
        else
            base = 8;
    }
    while ((c = *cp)) {
        if (isdigit(c)) {
            val = (val * base) + (c - '0');
            cp++;
            continue;
        }
        if (base == 16 && isxdigit(c)) {
            val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
            cp++;
            continue;
        }
        break;
    }
    if (*cp == '.') {
        /*
         * Internet format:
         *    a.b.c.d
         *    a.b.c    (with c treated as 16-bits)
         *    a.b    (with b treated as 24 bits)
         */
        if (pp >= parts + 4) return (INADDR_NONE);
        *pp++ = val;
        cp++;
        goto again;
    }
    /*
     * Check for trailing characters.
     */
    if (*cp && !isspace((UCHAR)*cp)) return (INADDR_NONE);

    *pp++ = val;
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    n = (u_long)(pp - parts);
    switch (n) {

    case 1:                /* a -- 32 bits */
        val = parts[0];
        break;

    case 2:                /* a.b -- 8.24 bits */
        val = (parts[0] << 24) | (parts[1] & 0xffffff);
        break;

    case 3:                /* a.b.c -- 8.8.16 bits */
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
            (parts[2] & 0xffff);
        break;

    case 4:                /* a.b.c.d -- 8.8.8.8 bits */
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
              ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
        break;

    default:
        return (INADDR_NONE);
    }
    val = htonl(val);
    return (val);
}

/*
 * @implemented
 */
CHAR FAR*
WSAAPI
inet_ntoa(IN IN_ADDR in)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    WSADATA WsaData;
    BOOL ManualLoad = FALSE;
    CHAR b[10];
    PCHAR p;
    DPRINT("inet_ntoa: %lx\n", in);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        DPRINT("MANUAL LOAD\n");

        /* Only fail if the error wasn't related to a missing WSAStartup */
        if (ErrorCode != WSANOTINITIALISED)
        {
            /* Fail */
            SetLastError(ErrorCode);
            return NULL;
        }

        /* Apps aren't expected to call WSAStartup for this API, so we will */
        if ((ErrorCode = WSAStartup(MAKEWORD(2,2), &WsaData)) != ERROR_SUCCESS)
        {
            /* We failed */
            SetLastError(ErrorCode);
            return NULL;
        }

        /* Try the prolog again */
        ManualLoad = TRUE;
        if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
        {
            /* Failed again... */
            WSACleanup();
            SetLastError(ErrorCode);
            return NULL;
        }
    }

    p = Thread->Buffer;
    _itoa(in.S_un.S_addr & 0xFF, b, 10);
    strcpy(p, b);
    _itoa((in.S_un.S_addr >> 8) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 16) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 24) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);

    /* Cleanup the manual load */
    if (ManualLoad) WSACleanup();

    /* Return the buffer */
    return p;
}

/*
 * @implemented
 */
ULONG
WSAAPI
htonl(IN ULONG hostlong)
{
    return DH2N(hostlong);
}

/*
 * @implemented
 */
USHORT
WSAAPI
htons(IN USHORT hostshort)
{
    return WH2N(hostshort);
}

/*
 * @implemented
 */
ULONG
WSAAPI
ntohl(IN ULONG netlong)
{
    return DN2H(netlong);
}

/*
 * @implemented
 */
USHORT
WSAAPI
ntohs(IN USHORT netshort)
{
    return WN2H(netshort);
}

/*
 * @implemented
 */
INT
WSAAPI
WSAHtonl(IN SOCKET s,
         IN ULONG hostlong,
         OUT ULONG FAR* lpnetlong)
{
    INT ErrorCode;
    PWSSOCKET Socket;
    DPRINT("WSAHtonl: %p, %lx, %p\n", s, hostlong, lpnetlong);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Make sure we got a parameter */
        if (!lpnetlong)
        {
            /* Fail */
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Check which byte order to use */
            if (Socket->CatalogEntry->ProtocolInfo.iNetworkByteOrder ==
                LITTLEENDIAN)
            {
                /* No conversion needed */
                *lpnetlong = hostlong;
            }
            else
            {
                /* Use a swap */
                *lpnetlong = DN2H(hostlong);
            }

            /* Dereference the socket */
            WsSockDereference(Socket);

            /* Return success */
            return ERROR_SUCCESS;
        }
        else
        {
            /* Set the error code */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAHtons(IN SOCKET s,
         IN USHORT hostshort,
         OUT USHORT FAR* lpnetshort)
{
    INT ErrorCode;
    PWSSOCKET Socket;
    DPRINT("WSAHtons: %p, %lx, %p\n", s, hostshort, lpnetshort);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Make sure we got a parameter */
        if (!lpnetshort)
        {
            /* Fail */
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Check which byte order to use */
            if (Socket->CatalogEntry->ProtocolInfo.iNetworkByteOrder ==
                LITTLEENDIAN)
            {
                /* No conversion needed */
                *lpnetshort = hostshort;
            }
            else
            {
                /* Use a swap */
                *lpnetshort = WN2H(hostshort);
            }

            /* Dereference the socket */
            WsSockDereference(Socket);

            /* Return success */
            return ERROR_SUCCESS;
        }
        else
        {
            /* Set the error code */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSANtohl(IN SOCKET s,
         IN ULONG netlong,
         OUT ULONG FAR* lphostlong)
{
    INT ErrorCode;
    PWSSOCKET Socket;
    DPRINT("WSANtohl: %p, %lx, %p\n", s, netlong, lphostlong);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Make sure we got a parameter */
        if (!lphostlong)
        {
            /* Fail */
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Check which byte order to use */
            if (Socket->CatalogEntry->ProtocolInfo.iNetworkByteOrder ==
                LITTLEENDIAN)
            {
                /* No conversion needed */
                *lphostlong = netlong;
            }
            else
            {
                /* Use a swap */
                *lphostlong = DN2H(netlong);
            }

            /* Dereference the socket */
            WsSockDereference(Socket);

            /* Return success */
            return ERROR_SUCCESS;
        }
        else
        {
            /* Set the error code */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSANtohs(IN SOCKET s,
         IN USHORT netshort,
         OUT USHORT FAR* lphostshort)
{
    INT ErrorCode;
    PWSSOCKET Socket;
    DPRINT("WSANtohs: %p, %lx, %p\n", s, netshort, lphostshort);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Make sure we got a parameter */
        if (!lphostshort)
        {
            /* Fail */
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Check which byte order to use */
            if (Socket->CatalogEntry->ProtocolInfo.iNetworkByteOrder ==
                LITTLEENDIAN)
            {
                /* No conversion needed */
                *lphostshort = netshort;
            }
            else
            {
                /* Use a swap */
                *lphostshort = WN2H(netshort);
            }

            /* Dereference the socket */
            WsSockDereference(Socket);

            /* Return success */
            return ERROR_SUCCESS;
        }
        else
        {
            /* Set the error code */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

PCSTR
WSAAPI
inet_ntop(
    _In_ INT Family,
    _In_ const VOID *pAddr,
    _Out_writes_(StringBufSize) PSTR pStringBuf,
    _In_ size_t StringBufSize)
{
    NTSTATUS Status;
    ULONG BufSize = StringBufSize;

    switch (Family)
    {
        case AF_INET:
            Status = RtlIpv4AddressToStringExA(pAddr, 0, pStringBuf, &BufSize);
            break;
        case AF_INET6:
            Status = RtlIpv6AddressToStringExA(pAddr, 0, 0, pStringBuf, &BufSize);
            break;
        default:
            SetLastError(WSAEAFNOSUPPORT);
            return NULL;
    }

    if (!NT_SUCCESS(Status)) 
    {
        SetLastError(WSAEINVAL);
        return NULL;
    }

    return pStringBuf;
}

PCWSTR
WSAAPI
InetNtopW(
    _In_ INT Family,
    _In_ const VOID *pAddr,
    _Out_writes_(StringBufSize) PWSTR pStringBuf,
    _In_ size_t StringBufSize)
{
    NTSTATUS Status;
    ULONG BufSize = StringBufSize;

    switch (Family)
    {
        case AF_INET:
            Status = RtlIpv4AddressToStringExW(pAddr, 0, pStringBuf, &BufSize);
            break;
        case AF_INET6:
            Status = RtlIpv6AddressToStringExW(pAddr, 0, 0, pStringBuf, &BufSize);
            break;
        default:
            SetLastError(WSAEAFNOSUPPORT);
            return NULL;
    }

    if (!NT_SUCCESS(Status)) 
    {
        SetLastError(WSAEINVAL);
        return NULL;
    }

    return pStringBuf;
}

INT
WSAAPI
inet_pton(
    _In_ INT Family,
    _In_ PCSTR pszAddrString,
    _Out_writes_bytes_(sizeof(IN_ADDR6)) PVOID pAddrBuf)
{
    NTSTATUS Status;
    PCSTR ch;

    if (!pszAddrString || !pAddrBuf)
    {
        SetLastError(WSAEFAULT);
        return -1;
    }

    switch (Family)
    {
        case AF_INET:
            Status = RtlIpv4StringToAddressA(pszAddrString, TRUE, &ch, pAddrBuf);
            break;
        case AF_INET6:
            Status = RtlIpv6StringToAddressA(pszAddrString, &ch, pAddrBuf);
            break;
        default:
            SetLastError(WSAEAFNOSUPPORT);
            return -1;
    }

    if (!NT_SUCCESS(Status) || (*ch != 0)) 
    {
        return 0;
    }

    return 1;
}

INT
WSAAPI
InetPtonW(
    _In_ INT Family,
    _In_ PCWSTR pszAddrString,
    _Out_writes_bytes_(sizeof(IN_ADDR6)) PVOID pAddrBuf)
{
    NTSTATUS Status;
    PCWSTR ch;

    if (!pszAddrString || !pAddrBuf)
    {
        SetLastError(WSAEFAULT);
        return -1;
    }

    switch (Family)
    {
        case AF_INET:
            Status = RtlIpv4StringToAddressW(pszAddrString, TRUE, &ch, pAddrBuf);
            break;
        case AF_INET6:
            Status = RtlIpv6StringToAddressW(pszAddrString, &ch, pAddrBuf);
            break;
        default:
            SetLastError(WSAEAFNOSUPPORT);
            return -1;
    }

    if (!NT_SUCCESS(Status) || (*ch != 0)) 
    {
        SetLastError(WSAEINVAL); /* Only unicode version sets this error */
        return 0;
    }

    return 1;
}
