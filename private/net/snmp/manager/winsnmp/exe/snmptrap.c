// snmptrap.c
//
// Original Microsoft code modified by ACE*COMM
// for use with WSNMP32.DLL and other trap receiving
// clients, per contract.
//
// Bob Natale, ACE*COMM (bnatale@acecomm.com)
// For NT v5 Beta, v970228
// Additional enhancements planned.
//
// This version of SNMPTRAP has no dependencies
// on either MGMTAPI.DLL or WSNMP32.DLL.
//
// WinSNMP clients use the SnmpRegister() function.
//
// Other clients will need to match the following
// values and structures:
//
//    SNMP_TRAP structure
//    SNMPTRAPPIPE name
//    TRAPBUFSIZE value
//
// Change log:
// ------------------------------------------------
// 4.0.1381.3  Apr 8, 1998 Bob Natale
//
// 1.  Re-worked the trap port monitoring thread into
//     two threads...one for IP and one for IPX, to
//     comply with WinSock v2's restrictions against
//     multi-protocol select().
//
// 2.  General clean-up/streamlining wrt "legacy" code
//     from original MS version...more to do here, esp.
//     wrt error handling code that does not do anything.
// ------------------------------------------------
// 4.0.1381.4  Apr. 10, 1998 Bob Natale
//
// 1.  Replaced mutex calls with critical_sectin calls.
//
// 2.  Cleaned out some dead code (removed commented out code)
// ------------------------------------------------
#include <windows.h>
#include <winsock.h>
#include <wsipx.h>
#include <process.h>
//--------------------------- PRIVATE VARIABLES -----------------------------
#define SNMPMGRTRAPPIPE "\\\\.\\PIPE\\MGMTAPI"
#define MAX_OUT_BUFS    16
#define TRAPBUFSIZE     4096
#define IP_TRAP_PORT    162
#define IPX_TRAP_PORT   36880
// ******** INITIALIZE A LIST HEAD ********
#define ll_init(head) (head)->next = (head)->prev = (head);
// ******** TEST A LIST FOR EMPTY ********
#define ll_empt(head) ( ((head)->next) == (head) )
// ******** Get ptr to next entry ********
#define ll_next(item,head)\
( (ll_node *)(item)->next == (head) ? 0 : \
(ll_node *)(item)->next )
// ******** ADD AN ITEM TO THE END OF A LIST ********
#define ll_adde(item,head)\
   {\
   ll_node *pred = (head)->prev;\
   ((ll_node *)(item))->next = (head);\
   ((ll_node *)(item))->prev = pred;\
   (pred)->next = ((ll_node *)(item));\
   (head)->prev = ((ll_node *)(item));\
   }
// ******** REMOVE AN ITEM FROM A LIST ********
#define ll_rmv(item)\
   {\
   ll_node *pred = ((ll_node *)(item))->prev;\
   ll_node *succ = ((ll_node *)(item))->next;\
   pred->next = succ;\
   succ->prev = pred;\
   }
// ******** List head/node ********
typedef struct ll_s
   { // linked list structure
   struct  ll_s *next;  // next node
   struct  ll_s *prev;  // prev. node
   } ll_node;           // linked list node
typedef struct
   {// shared by server trap thread and pipe thread
   ll_node  links;
   HANDLE   hPipe;
   } svrPipeListEntry;
typedef struct
   {
   SOCKADDR Addr;              
   int      AddrLen;           
   UINT     TrapBufSz;
   char     TrapBuf[TRAPBUFSIZE];   // the size of this array should match the size of the structure
                                    // defined in wsnmp_no.c!!!
   }        SNMP_TRAP, *PSNMP_TRAP;

HANDLE hExitEvent = NULL;
LPCTSTR svcName = "SNMPTRAP";
SERVICE_STATUS_HANDLE hService = 0;
SERVICE_STATUS status =
  {SERVICE_WIN32, SERVICE_STOPPED, SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0};
SOCKET ipSock = INVALID_SOCKET;
SOCKET ipxSock = INVALID_SOCKET;
HANDLE ipThread = NULL;
HANDLE ipxThread = NULL;
CRITICAL_SECTION cs_PIPELIST;
ll_node *pSvrPipeListHead = NULL;

//--------------------------- PRIVATE PROTOTYPES ----------------------------
DWORD WINAPI svrTrapThread (IN OUT LPVOID threadParam);
DWORD WINAPI svrPipeThread (IN LPVOID threadParam);
VOID WINAPI svcHandlerFunction (IN DWORD dwControl);
VOID WINAPI svcMainFunction (IN DWORD dwNumServicesArgs,
                             IN LPSTR *lpServiceArgVectors);

//--------------------------- PRIVATE PROCEDURES ----------------------------
VOID WINAPI svcHandlerFunction (IN DWORD dwControl)
{
if (dwControl == SERVICE_CONTROL_STOP)
   {
   status.dwCurrentState = SERVICE_STOP_PENDING;
   status.dwCheckPoint++;
   status.dwWaitHint = 45000;
   if (!SetServiceStatus(hService, &status))
      exit(1);
   // set event causing trap thread to terminate
   if (!SetEvent(hExitEvent))
      {
      status.dwCurrentState = SERVICE_STOPPED;
      status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
      status.dwServiceSpecificExitCode = 1; // OPENISSUE - svc err code
      status.dwCheckPoint = 0;
      status.dwWaitHint = 0;
      // We are exiting in any case, so ignore any error...
      SetServiceStatus (hService, &status);
      exit(1);
      }
   }
else
   //   dwControl == SERVICE_CONTROL_INTERROGATE
   //   dwControl == SERVICE_CONTROL_PAUSE
   //   dwControl == SERVICE_CONTROL_CONTINUE
   //   dwControl == <anything else>
   {
   if (status.dwCurrentState == SERVICE_STOP_PENDING ||
       status.dwCurrentState == SERVICE_START_PENDING)
      status.dwCheckPoint++;
   if (!SetServiceStatus (hService, &status))
      exit(1);
   }
} // end_svcHandlerFunction()

VOID WINAPI svcMainFunction (IN DWORD dwNumServicesArgs,
                             IN LPSTR *lpServiceArgVectors)
{
WSADATA WinSockData;
HANDLE hPipeThread = NULL;
DWORD  dwThreadId;
//---------------------------------------------------------------------
hService = RegisterServiceCtrlHandler (svcName, svcHandlerFunction);
if (hService == 0)
   {
   status.dwCurrentState = SERVICE_STOPPED;
   status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
   status.dwServiceSpecificExitCode = 2; // OPENISSUE - svc err code
   status.dwCheckPoint = 0;
   status.dwWaitHint = 0;
   // We are exiting in any case, so ignore any error...
   SetServiceStatus (hService, &status);
   exit(1);
   }
status.dwCurrentState = SERVICE_START_PENDING;
status.dwWaitHint = 20000;

if (!SetServiceStatus(hService, &status))
   exit(1);

__try
{
    InitializeCriticalSection (&cs_PIPELIST);
}
__except(EXCEPTION_EXECUTE_HANDLER)
{
   exit(1);
}

if (WSAStartup ((WORD)0x0101, &WinSockData))
   goto CLOSE_OUT; // WinSock startup failure


// allocate linked-list header for client received traps
if ((pSvrPipeListHead = (ll_node *)GlobalAlloc (GHND, sizeof(ll_node))) == NULL)
   goto CLOSE_OUT;
ll_init(pSvrPipeListHead);
if ((hPipeThread = (HANDLE)_beginthreadex
                   (NULL, 0, svrPipeThread, NULL, 0, &dwThreadId)) == 0)
   goto CLOSE_OUT;
//-----------------------------------------------------------------------------------
//CHECK_IP:
ipSock = socket (AF_INET, SOCK_DGRAM, 0);
if (ipSock != INVALID_SOCKET)
   {
   struct sockaddr_in localAddress_in;
   struct servent *serv;
   ZeroMemory (&localAddress_in, sizeof(localAddress_in));
   localAddress_in.sin_family = AF_INET;
   if ((serv = getservbyname ("snmp-trap", "udp")) == NULL)
      localAddress_in.sin_port = htons (IP_TRAP_PORT);
   else
      localAddress_in.sin_port = (SHORT)serv->s_port;
   localAddress_in.sin_addr.s_addr = htonl (INADDR_ANY);
   if (bind (ipSock, (LPSOCKADDR)&localAddress_in, sizeof(localAddress_in)) != SOCKET_ERROR)
      ipThread = (HANDLE)_beginthreadex
                 (NULL, 0, svrTrapThread, (LPVOID)ipSock, 0, &dwThreadId);
   }
//-----------------------------------------------------------------------------------
//CHECK_IPX:
ipxSock = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
if (ipxSock != INVALID_SOCKET)
   {
   struct sockaddr_ipx localAddress_ipx;
   ZeroMemory (&localAddress_ipx, sizeof(localAddress_ipx));
   localAddress_ipx.sa_family = AF_IPX;
   localAddress_ipx.sa_socket = htons (IPX_TRAP_PORT);
   if (bind (ipxSock, (LPSOCKADDR)&localAddress_ipx, sizeof(localAddress_ipx)) != SOCKET_ERROR)
      ipxThread = (HANDLE)_beginthreadex
                  (NULL, 0, svrTrapThread, (LPVOID)ipxSock, 0, &dwThreadId);
   }
//-----------------------------------------------------------------------------------
// We are ready to listen for traps...
status.dwCurrentState = SERVICE_RUNNING;
status.dwCheckPoint   = 0;
status.dwWaitHint     = 0;
if (!SetServiceStatus(hService, &status))
   goto CLOSE_OUT;
WaitForSingleObject (hExitEvent, INFINITE);
//-----------------------------------------------------------------------------------
CLOSE_OUT:
EnterCriticalSection (&cs_PIPELIST);
if (hPipeThread != NULL)
   {
   TerminateThread (hPipeThread, 0);
   CloseHandle (hPipeThread);
   }
if (ipSock != INVALID_SOCKET)
   closesocket (ipSock);
if (ipThread != NULL)
   {
   WaitForSingleObject (ipThread, INFINITE);
   CloseHandle (ipThread);
   }
if (ipxSock != INVALID_SOCKET)
   closesocket (ipxSock);
if (ipxThread != NULL)
   {
   WaitForSingleObject (ipxThread, INFINITE);
   CloseHandle (ipxThread);
   }
if (pSvrPipeListHead != NULL)
   GlobalFree (pSvrPipeListHead);
LeaveCriticalSection (&cs_PIPELIST);
DeleteCriticalSection (&cs_PIPELIST);
WSACleanup();
status.dwCurrentState = SERVICE_STOPPED;
status.dwCheckPoint = 0;
status.dwWaitHint = 0;
if (!SetServiceStatus(hService, &status))
   exit(1);
} // end_svcMainFunction()

//--------------------------- PUBLIC PROCEDURES -----------------------------
int __cdecl main ()
{
BOOL fOk;
OSVERSIONINFO osInfo;
SERVICE_TABLE_ENTRY svcStartTable[2] =
   {
   {(LPTSTR)svcName, svcMainFunction},
   {NULL, NULL}
   };
osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
fOk = GetVersionEx (&osInfo);
if (fOk && (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))
   { // create event to synchronize trap server shutdown
   hExitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
   // this call will not return until service stopped
   fOk = StartServiceCtrlDispatcher (svcStartTable);
   CloseHandle (hExitEvent);
   }
return fOk;
} // end_main()

//
DWORD WINAPI svrTrapThread (LPVOID threadParam)
// This thread takes a SOCKET parameter, loops on select()
// for data in-coming over that socket, writing it back
// out to clients over all pipes currently on the list of
// trap notification pipes.shared by this thread and the
// pipe thread
{
PSNMP_TRAP pRecvTrap = NULL;
struct fd_set readfds;
SOCKET fd = (SOCKET)threadParam;
int len;
//
while (TRUE)
   {

   ULONG ulTrapSize = 0;
   DWORD dwError = 0;
   // construct readfds which gets destroyed by select()
   FD_ZERO(&readfds);
   FD_SET(fd, &readfds);
   if (select (0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR)
      break; // terminate thread
   if (!(FD_ISSET(fd, &readfds)))
      continue;

   if (ioctlsocket(
            fd,              // socket to query
            FIONREAD,        // query for the size of the incoming datagram
            &ulTrapSize      // unsigned long to store the size of the datagram
            ) != 0)
      continue;              // continue if we could not determine the size of the
                             // incoming datagram

   if (pRecvTrap == NULL ||
       pRecvTrap->TrapBufSz < ulTrapSize)
   {
       if (pRecvTrap != NULL)
       {
           GlobalFree(pRecvTrap);
           pRecvTrap = NULL;
       }
       pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, sizeof(SNMP_TRAP) - sizeof(pRecvTrap->TrapBufSz) + ulTrapSize);
       if (pRecvTrap == NULL) // if there is so few memory that we can't allocate a bit ..
           break;             // bail out and stop the SNMPTRAP service (bug? - other option => 100% CPU which is worst)
   }

   pRecvTrap->TrapBufSz = ulTrapSize;
   pRecvTrap->AddrLen = sizeof(pRecvTrap->Addr);
   len = recvfrom (
           fd,
           pRecvTrap->TrapBuf,
           pRecvTrap->TrapBufSz,
           0, 
           &(pRecvTrap->Addr),
           &(pRecvTrap->AddrLen));
   if (len == SOCKET_ERROR)
      continue;
   EnterCriticalSection (&cs_PIPELIST);
   // add header to length
   len += sizeof(SNMP_TRAP) - sizeof(pRecvTrap->TrapBuf);

   if (!ll_empt(pSvrPipeListHead))
      {
      DWORD written;
      ll_node *item = pSvrPipeListHead;
      while (item = ll_next(item, pSvrPipeListHead))
         {
         if (!WriteFile(
				((svrPipeListEntry *)item)->hPipe,
				(LPBYTE)pRecvTrap,
				len,
				&written,
				NULL)
			)
            {
            DWORD dwError = GetLastError();
            // OPENISSUE - what errors could result from pipe break
            if (dwError != ERROR_NO_DATA)
               {
               ; // Placeholder for error handling
               }
            if (!DisconnectNamedPipe(((svrPipeListEntry *)item)->hPipe))
               {
               ; // Placeholder for error handling
               }
            else if (!CloseHandle(((svrPipeListEntry *)item)->hPipe))
               {
               ; // Placeholder for error handling
               }
            ll_rmv(item);
            GlobalFree(item); // check for errors?
            item = pSvrPipeListHead;
            } // end_if !WriteFile
         else if (written != (DWORD)len)
            {
            ; // Placeholder for error handling
            }
         } // end_while item = ll_next
      } // end_if !ll_empt
   LeaveCriticalSection (&cs_PIPELIST);
   } // end while TRUE

   if (pRecvTrap != NULL)
       GlobalFree(pRecvTrap);

   return 0;
} // end svrTrapThread()

PACL AllocGenericACL()
{
    PACL                        pAcl;
    PSID                        pSidAdmins, pSidUsers;
    SID_IDENTIFIER_AUTHORITY    Authority = SECURITY_NT_AUTHORITY;
    DWORD                       dwAclLength;

    pSidAdmins = pSidUsers = NULL;

    if ( !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidAdmins ) ||
         !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_USERS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidUsers ))
    {
        return NULL;
    }

    dwAclLength = sizeof(ACL) + 
                  sizeof(ACCESS_ALLOWED_ACE) -
                  sizeof(ULONG) +
                  GetLengthSid(pSidAdmins) +
                  sizeof(ACCESS_ALLOWED_ACE) - 
                  sizeof(ULONG) +
                  GetLengthSid(pSidUsers);

    pAcl = GlobalAlloc (GPTR, dwAclLength);
    if (pAcl != NULL)
    {
        if (!InitializeAcl( pAcl, dwAclLength, ACL_REVISION) ||
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   GENERIC_READ | GENERIC_WRITE,
                                   pSidAdmins ) || 
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   (GENERIC_READ | (FILE_GENERIC_WRITE & ~FILE_CREATE_PIPE_INSTANCE)),
                                   pSidUsers ))
        {
            GlobalFree(pAcl);
            pAcl = NULL;
        }
    }

    FreeSid(pSidAdmins);
    FreeSid(pSidUsers);

    return pAcl;
}

void FreeGenericACL( PACL pAcl)
{
    if (pAcl != NULL)
        GlobalFree(pAcl);
}

DWORD WINAPI svrPipeThread (LPVOID threadParam)
{
// This thread creates a named pipe instance and
// blocks waiting for a client connection.  When
// client connects, the pipe handle is added to the
// list of trap notification pipes.
// It then waits for another connection.
DWORD  nInBufLen = sizeof(SNMP_TRAP);
DWORD  nOutBufLen = sizeof(SNMP_TRAP) * MAX_OUT_BUFS;
SECURITY_ATTRIBUTES S_Attrib;
SECURITY_DESCRIPTOR S_Desc;
PACL   pAcl;
// construct security decsriptor
InitializeSecurityDescriptor (&S_Desc, SECURITY_DESCRIPTOR_REVISION);

if ((pAcl = AllocGenericACL()) == NULL ||
    !SetSecurityDescriptorDacl (&S_Desc, TRUE, pAcl, FALSE))
{
    FreeGenericACL(pAcl);
    return (0);
}

S_Attrib.nLength = sizeof(SECURITY_ATTRIBUTES);
S_Attrib.lpSecurityDescriptor = &S_Desc;
S_Attrib.bInheritHandle = TRUE;
//
while (TRUE)
   {
   HANDLE hPipe;
   svrPipeListEntry *item;
   hPipe = CreateNamedPipe (SNMPMGRTRAPPIPE,
            PIPE_ACCESS_DUPLEX,
            (PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE),
            PIPE_UNLIMITED_INSTANCES,
            nOutBufLen, nInBufLen, 0, &S_Attrib);

   if (hPipe == INVALID_HANDLE_VALUE)
      {
      break;
      }
   else if (!ConnectNamedPipe(hPipe, NULL) && 
             (GetLastError() != ERROR_PIPE_CONNECTED))
      {
      break;
      }
   else if (!(item = (svrPipeListEntry *)
             GlobalAlloc (GPTR, sizeof(svrPipeListEntry))))
      {
      break;;
      }
   else
      {
      item->hPipe = hPipe;
      EnterCriticalSection (&cs_PIPELIST);
      ll_adde(item, pSvrPipeListHead);
      LeaveCriticalSection (&cs_PIPELIST);
      } // end_else
   } // end_while TRUE

FreeGenericACL(pAcl);
return(0);

} // end_svrPipeThread()
