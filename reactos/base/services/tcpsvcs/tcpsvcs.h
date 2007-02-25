/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/tcpsvcs.h
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>

/* default port numbers */
#define ECHO_PORT 7
#define DISCARD_PORT 9
#define DAYTIME_PORT 13
#define QOTD_PORT 17
#define CHARGEN_PORT 19

#define NUM_SERVICES 5
#define BUF_SIZE 255
#define BUF 1024
#define CS_TIMEOUT 1000

/* RFC865 states no more than 512 chars per line */
#define MAX_QUOTE_BUF 512

/* printable ASCII's characters for chargen */
#define START 32
#define END 126

/* number of chars to put on a line */
#define LINESIZ 74 // 72 + /r and /n

/* data structure to pass to threads */
typedef struct _Services {
    USHORT Port;
    TCHAR *Name;
    LPTHREAD_START_ROUTINE Service;
} SERVICES, *PSERVICES;

/* tcpsvcs functions */
//static VOID WINAPI ServiceMain(DWORD argc, LPTSTR argv[]);
VOID WINAPI ServerCtrlHandler(DWORD control);
INT CreateServers(VOID);
VOID LogEvent (LPCTSTR UserMessage, INT ExitCode, BOOL PrintErrorMsg);
void UpdateStatus (int NewStatus, int Check);


/* skelserver functions */
DWORD WINAPI StartServer(LPVOID lpParam);
SOCKET SetUpListener(USHORT Port);
VOID AcceptConnections(SOCKET ListeningSocket,
    LPTHREAD_START_ROUTINE Service, TCHAR *Name);
BOOL EchoIncomingPackets(SOCKET sd);
BOOL ShutdownConnection(SOCKET Sock, BOOL bRec);

/* chargen functions */
DWORD WINAPI ChargenHandler(VOID* Sock_);
BOOL GenerateChars(SOCKET Sock);
BOOL SendLine(SOCKET Sock, char* Line);

/* daytime functions */
DWORD WINAPI DaytimeHandler(VOID* Sock_);
BOOL SendTime(SOCKET Sock, char *time);

/* echo functions */
DWORD WINAPI EchoHandler(VOID* Sock_);
BOOL EchoIncomingPackets(SOCKET Sock);

/* discard functions */
DWORD WINAPI DiscardHandler(VOID* Sock_);
BOOL RecieveIncomingPackets(SOCKET Sock);

/* qotd functions */
DWORD WINAPI QotdHandler(VOID* Sock_);
BOOL SendQuote(SOCKET Sock, char* Quote);
