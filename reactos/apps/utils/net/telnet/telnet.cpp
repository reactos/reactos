/* $Id: telnet.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : telnet.cpp
 * AUTHOR     : unknown (sources found on www.telnet.org)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: telnet client for the W32 subsystem
 * DATE       : 2001-01-21
 * REVISIONS
 *	2001-02-21 ea	Modified to compile under 0.0.16 src tree
 */
#include <winsock.h>
#include <windows.h>

#include "telnet.h"
#include "console.h"


//
// sock_loop is the thread dedicatd to reading socket input.
// It waits for data from the socket, and then gives it one byte at a time
// to the telnet vm to process.
//

DWORD sock_loop(SOCKET server)
{
  char buf[256];
  unsigned long read;
  char* scan;

  while( (read = recv(server,buf,sizeof(buf),0)) && read != SOCKET_ERROR )
  {
    scan = buf;
    while(read--)
      vm(server,*scan++);
  }
  int x = WSAGetLastError();
  return 0;
}

DWORD input_loop(SOCKET server)
{
  char buf[256];
  DWORD read;

  do
  {
    WaitForSingleObject(StandardInput, INFINITE);
    ReadFile(StandardInput, buf, sizeof buf, & read, NULL);
  }
  while(SOCKET_ERROR != send(server, buf, read, 0));

  return 0;

}

void telnet(SOCKET server)
{
  DWORD dwThreadIdsock;
  DWORD dwThreadIdinput;
  HANDLE threads[2];


  threads[0] = CreateThread( 
    NULL,                        /* no security attributes        */ 
    0,                           /* use default stack size        */ 
    (LPTHREAD_START_ROUTINE) sock_loop,  /* thread function       */ 
    (LPVOID)server,              /* argument to thread function   */ 
    0,                           /* use default creation flags    */ 
    &dwThreadIdsock);            /* returns the thread identifier */ 

  //wait for the other thread to complete any setup negotiation...
  //Sleep(500); //- this is not the problem - its just bloody stuffing up!

  threads[1] = CreateThread( 
    NULL,                        /* no security attributes        */ 
    0,                           /* use default stack size        */ 
    (LPTHREAD_START_ROUTINE) input_loop, /* thread function       */ 
    (LPVOID)server,              /* argument to thread function   */ 
    0,                           /* use default creation flags    */ 
    &dwThreadIdinput);           /* returns the thread identifier */ 


  WaitForMultipleObjects(2,threads,FALSE,INFINITE);
}


//
// connect to the hostname,port
// 
void telnet(
  char const* pszHostName,
  const short nPort)
{
  unsigned long ip;
  if((*pszHostName <= '9') && (*pszHostName >= '0'))
  {
     if((ip = inet_addr(pszHostName)) == INADDR_NONE)
       err("invalid host IP address given");
  }
  else
  {
    hostent* ent = gethostbyname(pszHostName);
    if(!ent)
      err(sockmsg(WSAGetLastError()));
    ip = *(unsigned long*)(ent->h_addr);
  }

  sockaddr_in name;
  name.sin_family = AF_INET;
  name.sin_port = htons(nPort);
  name.sin_addr = *(in_addr*)&ip;

  console_title_connecting (pszHostName, nPort);
  
  SOCKET server;

  if((server = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))==INVALID_SOCKET)
    err(sockmsg(WSAGetLastError()));

  if(SOCKET_ERROR == connect(server,(sockaddr*)&name,sizeof(sockaddr)))
    err(sockmsg(WSAGetLastError()));

  console_title_connected (pszHostName, nPort);
  
  telnet(server);

  closesocket(server);
}

/* EOF */
