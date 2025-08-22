#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <strsafe.h>

#define STATUS_PENDING ((DWORD)0x00000103)

/*
** macro definitions
*/
#define TELNET_PORT      (23)

#define BUFSIZE        (4096)  
#define USERID_SIZE      (64)
#define CTRLC             (3)
#define BS                (8)
#define CR               (13)
#define LF               (10)
#define DEL             (127)

#define IAC "\xff"
#define DONT "\xfe"
#define WONT "\xfc"
#define WILL "\xfb"
#define DO "\xfd"
#define SB "\xfa"
#define SE "\xf0"
#define ECHO "\x01"
#define SUPPRESS_GO_AHEAD "\x03"
#define TERMINAL_TYPE "\x18"
#define NAWS "\x1f"
#define LINEMODE "\x22"
#define NEWENVIRON "\x27"
#define MODE "\x01"

#define HANDSHAKE_TIMEOUT (3)

/*
** types
*/

typedef struct client_s
{
  char     userID[USERID_SIZE];
  int      socket;
  BOOLEAN  bTerminate;
  BOOLEAN  bReadFromPipe;
  BOOLEAN  bWriteToPipe;
  HANDLE   hProcess;
  DWORD    dwProcessId;
  HANDLE   hChildStdinWr;   
  HANDLE   hChildStdoutRd;
} client_t;

typedef enum
{
  NoEcho = 0,
  Echo = 1,
  Password = 2
} EchoMode;

/*
** Forward function declarations
*/
static BOOL WINAPI Cleanup(DWORD dwControlType);
static void WaitForConnect(void);
static BOOLEAN StartSocketInterface(void);
static void CreateSocket(void);
static void UserLogin(int client_socket);
static DWORD WINAPI UserLoginThread(LPVOID);
static int DoTelnetHandshake(int sock);
static int ReceiveLine(int sock, char *buffer, int len, EchoMode echo);
static void RunShell(client_t *client); 
//static BOOL CreateChildProcess(const char *); 
static DWORD WINAPI MonitorChildThread(LPVOID);
static DWORD WINAPI WriteToPipeThread(LPVOID); 
static DWORD WINAPI ReadFromPipeThread(LPVOID); 
static void TerminateShell(client_t *client);
static VOID ErrorExit(LPTSTR);
int kickoff_telnetd(void);
