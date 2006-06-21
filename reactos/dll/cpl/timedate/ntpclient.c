#include <timedate.h>

SOCKET Sock;
SOCKADDR_IN myAddr, ntpAddr;

BOOL InitialiseConnection(CHAR *szIpAddr)
{
    WSADATA wsaData;
    INT Ret;

    Ret = WSAStartup(MAKEWORD(2, 2),
                     &wsaData);
    if (Ret != 0)
        return FALSE;

    Sock = WSASocket(AF_INET,
                     SOCK_DGRAM,
                     IPPROTO_UDP,
                     NULL,
                     0,
                     WSA_FLAG_OVERLAPPED);
    if (Sock == INVALID_SOCKET )
        return FALSE;

    /* setup client socket */
    ZeroMemory(&myAddr, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(IPPORT_TIMESERVER);
    myAddr.sin_addr.s_addr = inet_addr(szIpAddr);

    Ret = bind(Sock,
               (SOCKADDR *)&myAddr,
               sizeof(SOCKADDR));
    if (Ret == SOCKET_ERROR)
        return FALSE;

    /* setup server socket */
    ZeroMemory(&ntpAddr, sizeof(ntpAddr));
    ntpAddr.sin_family = AF_INET;
    ntpAddr.sin_port = htons(IPPORT_TIMESERVER);
    ntpAddr.sin_addr.s_addr = INADDR_ANY;

    return TRUE;
}

VOID DestroyConnection()
{
    WSACleanup();
}

/* send some data to wake the server up */
BOOL SendData()
{
    CHAR Packet[] = "";
    INT Ret;

    Ret = sendto(Sock,
                 Packet,
                 sizeof(Packet),
                 0,
                 (SOCKADDR *)&myAddr,
                 sizeof(SOCKADDR_IN));

    if (Ret == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}


ULONG RecieveData(ULONG ulTime)
{
    INT Ret;
    INT Size = sizeof(SOCKADDR_IN);

    Ret = recvfrom(Sock,
                   (char *)&ulTime,
                   4,
                   0,
                   (SOCKADDR *)&ntpAddr,
                   &Size);
    if (Ret != SOCKET_ERROR)
        ulTime = ntohl(ulTime);
    else
        ulTime = 0;


    return ulTime;
}
