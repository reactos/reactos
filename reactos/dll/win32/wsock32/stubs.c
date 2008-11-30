/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock DLL
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */
#include <winsock2.h>
#include <windows.h>
//#include <stdlib.h>

/*
 * @unimplemented
 */
int
WINAPI
getsockopt(SOCKET Sock,
           int Level,
           int OptName,
           char *OptVal,
           int *OptLen)
{
  OutputDebugStringW(L"w32sock getsockopt stub called\n");

  return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
int
WINAPI
setsockopt(SOCKET Sock,
           int Level,
           int OptName,
           const char *OptVal,
           int OptLen)
{
  OutputDebugStringW(L"w32sock setsockopt stub called\n");

  return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
int
WINAPI
recv(SOCKET Sock,
     char *Buf,
     int Len,
     int Flags)
{
  OutputDebugStringW(L"w32sock recv stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
WINAPI
recvfrom(SOCKET Sock,
         char *Buf,
         int Len,
         int Flags,
         struct sockaddr *From,
         int *FromLen)
{
  OutputDebugStringW(L"w32sock recvfrom stub called\n");

  return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DllMain(HINSTANCE InstDLL,
        DWORD Reason,
        LPVOID Reserved)
{
  return TRUE;
}
