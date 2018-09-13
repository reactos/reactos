// wsnmp_no.c
//
// WinSNMP Notification Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 970310 - Added callback session support
//        - Added v2 to v1 trap mapping
//        - Refined v1 to v2 trap mapping
//
#include "winsnmp.inc"
//
BOOL DispatchTrap (LPSAS host, smiLPOCTETS community, LPPDUS pdu);
BOOL MapV1TrapV2 (LPPDUS pdu);
smiUINT32 ParsePduHdr (smiLPBYTE msgPtr, smiUINT32 msgLen, smiLPUINT32 version, smiLPINT type, smiLPUINT32 reqID);
BOOL SetPduType (smiLPBYTE msgPtr, smiUINT32 msgLen, int pduType);
THR_TYPE WINAPI thrNotify (LPVOID);

smiUINT32 sysUpTimeValue[9]    = {1,3,6,1,2,1,1,3,0};
smiOID sysUpTimeName   = {9, sysUpTimeValue};
smiUINT32 snmpTrapOidValue[11] = {1,3,6,1,6,3,1,1,4,1,0};
smiOID snmpTrapOidName = {11, snmpTrapOidValue};
smiUINT32 snmpTrapsValue[10]   = {1,3,6,1,6,3,1,1,5,999};
smiOID snmpTrapsName   = {10, snmpTrapsValue};
smiUINT32 snmpTrapEntValue[11] = {1,3,6,1,6,3,1,1,4,3,0};
smiOID snmpTrapEntName = {11, snmpTrapEntValue};
smiUINT32 snmpTrapAddrValue[7] = {1,3,6,1,3,1057,1};
smiOID snmpTrapAddrName = {7, snmpTrapAddrValue};

void MsgNotify (smiUINT32 msgType, smiLPBYTE msgAddr, smiUINT32 msgLen, smiUINT32 nAgent, LPSAS host)
{
DWORD nFound;
DWORD nMsg;
smiUINT32 version;
smiINT pduType;
smiUINT32 reqID;
LPSESSION pSession;
LPSNMPMSG pMsg;

nMsg = ParsePduHdr (msgAddr, msgLen, &version, &pduType, &reqID);
if (nMsg != 0) // ParsePduHdr returns 0 == no_error
   {
   if (msgAddr)
   GlobalFree (msgAddr);
   return;
   }
switch (msgType)
   {
   case NP_TRAP:
   if (TrapDescr.Used && // Discard traps if no registrations
         (pduType == SNMP_PDU_INFORM ||
          pduType == SNMP_PDU_TRAP ||
          pduType == SNMP_PDU_V1TRAP))
      {
      smiLPOCTETS community;
      LPPDUS pdu;
      BOOL bConvert;
      pdu = GlobalAlloc (GPTR, sizeof(PDUS));
      if (pdu == NULL)
         goto DONE_TRAP;
      nMsg = ParseMessage (msgAddr, msgLen, &version, &community, pdu);
      if (nMsg != 0) // ParseMessage returns 0 == no_error
         goto DONE_PDU;
      if (pduType == SNMP_PDU_INFORM)
         { // Send the Inform acknowledgment response
         SOCKET s;
         SetPduType (msgAddr, msgLen, SNMP_PDU_RESPONSE);
         if (host->ipx.sa_family == AF_IPX)
            s = TaskData.ipxSock;
         else
            s = TaskData.ipSock;
         sendto (s, msgAddr, msgLen, 0, (LPSOCKADDR)host, sizeof(SOCKADDR));
         SetPduType (msgAddr, msgLen, SNMP_PDU_INFORM);
         }
      bConvert = TRUE;
      if (pduType == SNMP_PDU_V1TRAP)           // If v1 trap...
         bConvert = MapV1TrapV2 (pdu);          // convert to v2 trap
      if (bConvert)
         DispatchTrap (host, community, pdu);   // always v2 here
      // Cleanup is the same regardless of success or failure
      FreeVarBindList (pdu->VBL_addr);  // Checks for NULL
      FreeV1Trap (pdu->v1Trap);         // ditto
      FreeOctetString (community);      // ditto
      DONE_PDU:
      GlobalFree (pdu);
      } // end_if Trap_or_Inform PDU
   DONE_TRAP:
   GlobalFree (msgAddr);
   return; // end_case NP_TRAP

   case NP_RESPONSE:
   if (pduType != SNMP_PDU_RESPONSE)
      {
      GlobalFree (msgAddr);
      return;
      }
   EnterCriticalSection (&cs_MSG);

   for (nFound=0, nMsg=0; nFound<MsgDescr.Used && nMsg<MsgDescr.Allocated; nMsg++)
      {
      pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
      if (!pMsg->Session)
         continue;
	  nFound++;
      if ((pMsg->Status == NP_SENT) && // Must have been sent!
          (pMsg->dllReqId == reqID))   // Must match up!
         {
         pMsg->Status = NP_RCVD;       // ResponsePDU!
         // Release sent packet message
         if (pMsg->Addr)
            GlobalFree (pMsg->Addr);
         // Point to received packet message
         pMsg->Addr = msgAddr;
         pMsg->Size = msgLen;
         LeaveCriticalSection (&cs_MSG);
		 pSession = snmpGetTableEntry(&SessDescr, HandleToUlong(pMsg->Session) - 1);
         if (pSession->fCallBack)
            { // callback session notification mode
            EnterCriticalSection (&cs_SESSION);
            if (pSession->thrHandle)
               {
               if (pSession->thrCount != 0xFFFFFFFF)
                  pSession->thrCount++;
               SetEvent (pSession->thrEvent);
               }
            else
               FreeMsg (nMsg);
            LeaveCriticalSection (&cs_SESSION);
            }
         else
            { // window/message session notification mode
            if (IsWindow(pSession->hWnd))
               {
               pMsg->Status = NP_READY;
               PostMessage (pSession->hWnd,
                            pSession->wMsg,
                            0, pMsg->appReqId);
               }
            else
               FreeMsg (nMsg);
            }
         return;  // Matched response with request
         } // end_if
      } // end_for
   // If we fall through the for loop without finding a match,
   //  this must be a spurious message from agent...discard
   GlobalFree (msgAddr);
   LeaveCriticalSection (&cs_MSG);
   return; // end_case NP_RESPONSE

   case NP_REQUEST:
   // To allow for AgentX Master Agents and Mid-Level-Managers
   // any type of PDU may be accepted on this channel - BobN 4/8/97
   // Get a msg slot
   EnterCriticalSection (&cs_MSG);
   if (snmpAllocTableEntry(&MsgDescr, &nMsg) != SNMPAPI_SUCCESS)
   {
       LeaveCriticalSection(&cs_MSG);
       return;
   }
   pMsg = snmpGetTableEntry(&MsgDescr, nMsg);

   pMsg->Session = ((LPAGENT)snmpGetTableEntry(&AgentDescr, nAgent))->Session;
   pMsg->Status = NP_RCVD;       // In-bound request
   pMsg->Type = pduType;
   pMsg->Addr = msgAddr;
   pMsg->Size = msgLen;
   pMsg->appReqId = pMsg->dllReqId = reqID;
   CopyMemory (&(pMsg->Host), host, sizeof(SAS));
   LeaveCriticalSection (&cs_MSG);
   pSession = snmpGetTableEntry(&SessDescr, HandleToUlong(pMsg->Session) - 1);
   if (pSession->fCallBack)
      { // callback session notification mode
     EnterCriticalSection (&cs_SESSION);
      if (pSession->thrHandle)
         {
         if (pSession->thrCount != 0xFFFFFFFF)
            pSession->thrCount++;
         SetEvent (pSession->thrEvent);
         }
      else
         {
         FreeMsg (nMsg);
         }
      LeaveCriticalSection (&cs_SESSION);
      }
   else
      {
      if (IsWindow(pSession->hWnd))
         {
         pMsg->Status = NP_READY;
         PostMessage (pSession->hWnd,
                      pSession->wMsg,
                      0, pMsg->appReqId);
         }
      else
         FreeMsg (nMsg);
      }
   break;

   default:
   GlobalFree (msgAddr);
   break;
   } // end_switch msgType
return;
} // end_MsgNotify

THR_TYPE WINAPI thrNotify (LPVOID cbSessionSlot)
{
    DWORD           nSes = (DWORD)((DWORD_PTR)cbSessionSlot);
    HSNMP_SESSION   hSession = (HSNMP_SESSION)(nSes + 1);
    DWORD           nUsed, nMsg;
    WPARAM          wParam;
    LPARAM          lParam;
    BOOL            bFound, bWillBlock;
	LPSESSION		pSession;
    LPSNMPMSG       pMsg;

    // pSession->thrCount counts the number of requests. External threads increment it
    // each time they know something has changed in the message table (message expired or received)
    // thrNotify decrements it each time it scans the message table.
    do
    {
        EnterCriticalSection (&cs_SESSION);
		pSession = snmpGetTableEntry(&SessDescr, nSes);
        if (pSession->thrCount != 0xFFFFFFFF &&
            pSession->thrCount != 0)
            pSession->thrCount-- ;
        bWillBlock = pSession->thrCount == 0;
        LeaveCriticalSection (&cs_SESSION);

        // The thread will block only if the pSession->thrCount was 0 (tested in critical
        // section). It will be unblocked by external threads, from inside the same critical section.
        if (bWillBlock)
            WaitForSingleObject (pSession->thrEvent, INFINITE);

        // termination is requested, just break the loop
        if (pSession->thrCount == 0xFFFFFFFF)
            break;

        bFound = FALSE;
        // Find a waiting Msg for this session to process
        EnterCriticalSection (&cs_MSG);
        for (nUsed=0, nMsg=0;
             nUsed<MsgDescr.Used && nMsg<MsgDescr.Allocated;
             nMsg++)
        {
            pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
            if (pMsg->Session == hSession &&
                (pMsg->Status == NP_RCVD || pMsg->Status == NP_EXPIRED))
            {
                // the message was found. It might be already received, or it might
                // be timed out. Either case, the notification function has to be called.
                wParam = pMsg->Status == NP_RCVD ? 0 : SNMPAPI_TL_TIMEOUT ;
                lParam = pMsg->appReqId;

                if (wParam == SNMPAPI_TL_TIMEOUT)
                    FreeMsg(nMsg); // no more need for this expired bugger
                else
                    pMsg->Status = NP_READY; // mark it for SnmpRecvMsg()

                bFound = TRUE;
                // as the message was found, no reason to loop further
                break;
            }

            // update nFound to avoid searching more than the messages available
            nUsed += (pMsg->Session != 0);
        }
        LeaveCriticalSection (&cs_MSG);

        if (bFound)
        {
            //if a message was found for this session, call the notification function
            (*(pSession->fCallBack)) (hSession,
                                      pSession->hWnd,
                                      pSession->wMsg,
                                      wParam,
                                      lParam,
                                      pSession->lpClientData);
        }

    } while (TRUE);

    _endthreadex(0);

    return (0);
} // end_thrNotify

THR_TYPE WINAPI thrManager (LPVOID xSock)
{
DWORD  iBytes;
int    nSock;
fd_set readFDS;
SOCKET *pSock = (SOCKET*)xSock;
SOCKET tSock = *pSock;
SAS    host;
smiLPBYTE rMsgPtr;
int    fLen;
while (TRUE)
   {
   FD_ZERO (&readFDS);
   // Note:  strategy used in this block to assign a value
   // to "fLen" is important for Solaris and benign for Win32
   FD_SET (tSock, &readFDS);
   fLen = (int)tSock;
   fLen++;
   // Must preserve value of fLen across loops
   nSock = select (fLen, &readFDS, NULL, NULL, NULL);
   if (nSock == SOCKET_ERROR || *pSock == INVALID_SOCKET)
      goto DONE;
   // Only one socket monitored per thread, hence
   // FD_ISSET can be safely assumed at this point
   nSock = ioctlsocket (tSock, FIONREAD, &iBytes);
   if (nSock == SOCKET_ERROR || *pSock == INVALID_SOCKET)
      goto DONE;
   // Find the message buffer address...
   rMsgPtr = GlobalAlloc (GPTR, iBytes);
   if (rMsgPtr == NULL)
      { // No space error...throw away the message...
      recvfrom (tSock, (LPSTR)&nSock, 1, 0, NULL, NULL);
      if (*pSock == INVALID_SOCKET)
          goto DONE;
      // ...and call it quits.
      continue;
      }
   nSock = sizeof(SAS);
   // get the datagram and the address of the host that sent it
   iBytes = recvfrom (tSock, rMsgPtr, iBytes, 0, (LPSOCKADDR)&host, &nSock);
   if (iBytes != SOCKET_ERROR && *pSock != INVALID_SOCKET)
      MsgNotify (NP_RESPONSE, rMsgPtr, iBytes, 0, &host);
   } // end_while
DONE:
return (0);
} // end_thrManager

THR_TYPE WINAPI thrTimer (LPVOID nTask)
{ // Clean-up any timed-out messages
BOOL bFree;
DWORD lTicks, nMsg;
DWORD nFound;
SOCKET tSock;
LPSNMPMSG pMsg;

// This thread won't be needed immediately upon creation.
// It sleeps/suspends itself as appropriate.
// SnmpSendMsg() resumes it for each message sent
// SnmpCleanup() resumes it to signal termination.
while (TRUE)
   {// Once per second granularity
#ifdef SOLARIS
   sleep (1);
#else
   Sleep (1000);
#endif // SOLARIS
   // Check for termination request
   if (TaskData.timerThread == NULL)
      goto DONE;
   // If no msgs, go back to sleep
   if (MsgDescr.Used == 0)
      continue;
   EnterCriticalSection (&cs_MSG);
   for (nMsg=0, nFound=0; nFound<MsgDescr.Used && nMsg<MsgDescr.Allocated; nMsg++)
      {
      pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
      if (!pMsg->Session)           // Skip unused slots
         continue;
      nFound++; // Signal break when last used slot is processed
      if (pMsg->Status != NP_SENT)  // Skip pending and
         continue;                           // received messages
      lTicks = GetTickCount();               // update current time
      // Following test revised on 10/18/96 by BobN
      // Message "tick-time" rather than "TTL" now stored in MSG
      // to enable test for Windows timer wrap (49.7 days)
      if (pMsg->Wait + pMsg->Ticks > lTicks  &&  // MSG TTL test
          pMsg->Ticks <= lTicks)                 // Timer wrap test
         continue;                            // Retain the message
      bFree = TRUE; // Prepare to free the message slot
      if (pMsg->nRetransmitMode)
         {
         if (pMsg->Tries)
            {
            //WriteSocket (nMsg);
            // Determine which socket to use
            if (pMsg->Host.ipx.sa_family == AF_IPX)
               tSock = TaskData.ipxSock;
            else
               tSock = TaskData.ipSock;
            // Send the data
            sendto (tSock, pMsg->Addr, pMsg->Size,
                    0, (LPSOCKADDR)&(pMsg->Host), sizeof(SAS));
            // Need to check for SOCKET_ERROR!
            // end_WriteSocket
            pMsg->Ticks = lTicks;
            pMsg->Tries--;    // Record the attempt
            if (!pMsg->Tries) // No further retries?
               {                       // Release buffer space
               GlobalFree (pMsg->Addr);
               pMsg->Addr = NULL;
               pMsg->Size = 0;
               }
            bFree = FALSE;             // Retain the message slot
            }
         else
            {
			LPSESSION pSession;

			pSession = snmpGetTableEntry(&SessDescr, HandleToUlong(pMsg->Session) - 1);
            if (pSession->fCallBack)
               { // callback session notification mode
               EnterCriticalSection (&cs_SESSION);
               if (pSession->thrHandle)
                  {
                  bFree = FALSE; // thrNotify will free it
                  pMsg->Status = NP_EXPIRED;
                  if (pSession->thrCount != 0xFFFFFFFF)
                     pSession->thrCount++;
                  SetEvent (pSession->thrEvent);
                  }
               LeaveCriticalSection (&cs_SESSION);
               }
            else
               { // windows/message session notification mode
               if (IsWindow(pSession->hWnd))
                  {
                  PostMessage (pSession->hWnd,
                               pSession->wMsg,
                               SNMPAPI_TL_TIMEOUT,
                               pMsg->appReqId);
                  }
               }
            } // end_else (no retry left)
         } // end_if (retransmitMode)
      if (bFree) FreeMsg (nMsg);
      } // end_for
   LeaveCriticalSection (&cs_MSG);
   } // end_while
DONE:
return (0);
} // end_thrTimer

#ifndef SOLARIS
// Client side trap processing for Win32
THR_TYPE WINAPI thrTrap (LPVOID lpTask)
{
#define TRAPBUFSIZE 4096
typedef struct
   {
   SOCKADDR Addr;
   int      AddrLen;
   UINT     TrapBufSz;
   char     TrapBuf[TRAPBUFSIZE];
   } SNMP_TRAP, *PSNMP_TRAP;
SNMP_TRAP recvTrap;
DWORD iBytes;
smiLPBYTE rMsgPtr;
//
// Approach differs for NT (SNMPTRAP) vs '95
//
if (TaskData.sEnv.dwPlatformId == VER_PLATFORM_WIN32_NT)
{
	// IMPORTANT NOTE FOR NT:
	// This code must be consistent with the SNMPTRAP code
	// wrt TRAPBUFSIZE, SNMPTRAPPIPE, and the SNMP_TRAP struct

	while (TRUE)
    {
        if (ReadFile (
		        TaskData.trapPipe,
		        (LPBYTE)&recvTrap,
		        sizeof(SNMP_TRAP) - sizeof(recvTrap.TrapBuf),
		        &iBytes,
		        NULL) ||
            GetLastError() != ERROR_MORE_DATA)
            break;

        // Find the message buffer address...
        rMsgPtr = GlobalAlloc (GPTR, 2*recvTrap.TrapBufSz);

        if (rMsgPtr == NULL ||
			!ReadFile(
                TaskData.trapPipe,
                (LPBYTE)rMsgPtr,
                2*recvTrap.TrapBufSz,
                &iBytes,
                NULL)) // No space error or wrong data on pipe...drop the message...
        {
            GetLastError();
            continue;
        }
        // get the datagram and the address of the host that sent it
        MsgNotify (NP_TRAP, rMsgPtr, iBytes, 0, (LPSAS)&recvTrap.Addr);
    } // end while()

	goto DONE;
} // end_NT_thrTrap
//
// Handle Win95 traps
//
else if (TaskData.sEnv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
{
#define BASE_PORT 26025
#define MAX_PORTS 10
SOCKADDR_IN trapAddr;
int trapPort;
int nSock;
fd_set readFDS;
int i, j;
DWORD iHeader = sizeof(SOCKADDR) + sizeof(int);
TaskData.trapWnd = FindWindow (NP_WSX95, NP_WSX95);
if (TaskData.trapWnd == NULL)
   {
   STARTUPINFO su;
   PROCESS_INFORMATION pi;
   ZeroMemory (&su, sizeof(STARTUPINFO));
   su.cb = sizeof(STARTUPINFO);
   if (CreateProcess (NULL, NP_WSX95_EXE, NULL, NULL, FALSE,
                      0, NULL, NULL, &su, &pi))
      {
      WaitForInputIdle (pi.hProcess, TRAPSERVERTIMEOUT);
      TaskData.trapWnd = FindWindow (NP_WSX95, NP_WSX95);
      }
   if (TaskData.trapWnd == NULL)
      return (0); // ERROR: Cannot connect to NP_WSX95
   }
// create UDP socket to receive traps from NP_WSX95
TaskData.trapSock = socket (AF_INET, SOCK_DGRAM, 0);
if (TaskData.trapSock == INVALID_SOCKET)
   return (0); // ERROR: Cannot create socket
ZeroMemory (&trapAddr, sizeof(trapAddr));
trapAddr.sin_family = AF_INET;
trapAddr.sin_addr.s_addr = inet_addr (LOOPBACK_ADDR);
trapPort = BASE_PORT;
for (i = 0; i < MAX_PORTS; i++)
   {
   trapPort++;
   trapAddr.sin_port = htons ((WORD)trapPort);
   j = bind (TaskData.trapSock, (LPSOCKADDR)&trapAddr, sizeof(trapAddr));
   if (j == 0)
      break; // 0 = bind success
   }
if (j == SOCKET_ERROR)
   {
   closesocket (TaskData.trapSock);
   TaskData.trapSock = INVALID_SOCKET;
   goto DONE; // ERROR: Cannot bind a port
   }
// Notify NP_WSX95 to forward traps
PostMessage (TaskData.trapWnd, WSNMP_TRAPS_ON, (DWORD_PTR)TaskData.hTask, trapPort);
//
while (TRUE)
   {
   FD_ZERO (&readFDS);
   FD_SET (TaskData.trapSock, &readFDS);
   nSock = select (0, &readFDS, NULL, NULL, NULL);
   if (nSock == SOCKET_ERROR)
      goto DONE;
   // Thread is listening on only one socket, so
   // FD_ISSET can be safely assumed here
   // get the datagram and the address of the host that sent it
   iBytes = recv (TaskData.trapSock, (LPBYTE)&recvTrap, sizeof(SNMP_TRAP), 0);
   if (iBytes != SOCKET_ERROR && iBytes > iHeader)
      {
      iBytes -= iHeader;
      // Find the message buffer address...
      rMsgPtr = GlobalAlloc (GPTR, iBytes);
      if (rMsgPtr == NULL) // No space error!
         continue;
      CopyMemory (rMsgPtr, &recvTrap.TrapBuf, iBytes);
      MsgNotify (NP_TRAP, rMsgPtr, iBytes, 0, (LPSAS)&recvTrap.Addr);
      } // end_if
   } // end_while
} // end_95_thrTrap
DONE:
return (0);
} // end_Win32_thrTrap
#endif // ! SOLARIS

#ifdef SOLARIS
THR_TYPE WINAPI thrTrap (LPVOID lpTask)
// This thread sets up a trap listen socket,
// receives traps from the socket,
// then notifies the DLL for processing.
{
SOCKADDR localAddress;
SOCKADDR_IN localAddress_in;
SOCKADDR_IPX localAddress_ipx;
struct servent *serv;
struct fd_set readfds;
int    fdSockMax = 0;
int    fLen = 0;
int    nSock;
SOCKET fd;
SOCKET fdSock[2];
BOOL   fSuccess = FALSE;
SAS    host;
smiLPBYTE rMsgPtr;
DWORD  iBytes;
int    i;
//
fd = socket (AF_INET, SOCK_DGRAM, 0);
if (fd != INVALID_SOCKET)
   {
   ZeroMemory (&localAddress_in, sizeof(localAddress_in));
   localAddress_in.sin_family = AF_INET;
   if ((serv = getservbyname ("snmp-trap", "udp")) == NULL)
      localAddress_in.sin_port = htons (IP_TRAP_PORT);
   else
      localAddress_in.sin_port = (short)serv->s_port;
   localAddress_in.sin_addr.s_addr = htonl (INADDR_ANY);
   CopyMemory (&localAddress, &localAddress_in, sizeof(localAddress_in));
   if (bind (fd, &localAddress, sizeof(localAddress)) != SOCKET_ERROR)
      {
      fLen = fd;
      fdSock[fdSockMax++] = fd;
      fSuccess = TRUE;
      }
   }
fd = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
if (fd != INVALID_SOCKET)
   {
   ZeroMemory (&localAddress_ipx, sizeof(localAddress_ipx));
   localAddress_ipx.sa_family = AF_IPX;
   localAddress_ipx.sa_socket = htons (IPX_TRAP_PORT);
   CopyMemory (&localAddress, &localAddress_ipx, sizeof(localAddress_ipx));
   if (bind (fd, &localAddress, sizeof(localAddress)) != SOCKET_ERROR)
      {
      if (fd > fLen) fLen = fd;
      fdSock[fdSockMax++] = fd;
      fSuccess = TRUE;
      }
   }
if (!fSuccess)
   return (0); // can't open either socket
fLen++;  // increment select counter
while (TRUE)
   {
   FD_ZERO(&readfds);
   // construct readfds which gets destroyed by select()
   for (i=0; i < fdSockMax; i++)
      FD_SET(fdSock[i], &readfds);
   // Must preserve value of fLen across loops
   nSock = select (fLen, &readfds, NULL, NULL, NULL);
   if (nSock == SOCKET_ERROR)
      goto DONE;
   for (i=0; i<fdSockMax; i++)
      {
      if (FD_ISSET(fdSock[i], &readfds))
         {
         ioctlsocket (fdSock[i], FIONREAD, &iBytes);
         rMsgPtr = GlobalAlloc (GPTR, iBytes);
         if (rMsgPtr == NULL)
            {
            recvfrom (fdSock[i], (LPSTR)&nSock, 1, 0, NULL, NULL);
            continue;
            }
         nSock = sizeof(SAS);
         iBytes = recvfrom (fdSock[i], rMsgPtr, iBytes, 0, (LPSOCKADDR)&host, &nSock);
         if (iBytes != SOCKET_ERROR)
            MsgNotify (NP_TRAP, rMsgPtr, iBytes, 0, &host);
         } // end_if FD_ISSET
      } // end_for
   } // end while TRUE
DONE:
return (NULL);
} // end_thrTrap (SOLARIS)
#endif // SOLARIS

THR_TYPE WINAPI thrAgent (LPVOID newAgent)
{
DWORD  iBytes;
int    iLen;
fd_set readFDS;
SAS    host;
smiLPBYTE rMsgPtr;
DWORD  nAgent = (DWORD)((DWORD_PTR)newAgent);
SOCKET sAgent = ((LPAGENT)snmpGetTableEntry(&AgentDescr, nAgent))->Socket;
// fLen logic needed for Solaris; ignored in Win32
int    fLen = (int)sAgent + 1;
while (TRUE)
   {
   FD_ZERO (&readFDS);
   FD_SET (sAgent, &readFDS);
   // Must preserve value of fLen acroos loops
   iLen = select (fLen, &readFDS, NULL, NULL, NULL);
   if (iLen == SOCKET_ERROR)
      goto DONE;
   // Only one socket per thread, therefore
   // we can safely assume FD_ISSET here
   iLen = ioctlsocket (sAgent, FIONREAD, &iBytes);
   if (iLen == SOCKET_ERROR)
      goto DONE;
   if (iBytes == 0)
      continue;
   // Find the message buffer address...
   rMsgPtr = GlobalAlloc (GPTR, iBytes);
   if (rMsgPtr == NULL)
      { // No space error...throw away the message...
      recvfrom (sAgent, (LPSTR)&iLen, 1, 0, NULL, NULL);
      // ...and call it quits.
      continue;
      }
   iLen = sizeof(SAS);
   // get the datagram and the address of the host that sent it
   iBytes = recvfrom (sAgent, rMsgPtr, iBytes, 0, (LPSOCKADDR)&host, &iLen);
   if (iBytes != SOCKET_ERROR)
      MsgNotify (NP_REQUEST, rMsgPtr, iBytes, nAgent, &host);
   } // end_while
DONE:
return (0);
} // end_thrAgent

BOOL MapV1TrapV2 (LPPDUS pdu)
{
LPVARBIND VbTicks = NULL;
LPVARBIND VbTrap = NULL;
LPVARBIND VbAddress = NULL;
LPVARBIND VbEnterprise = NULL;
LPVARBIND endPtr;
smiLPUINT32 ptrTrap;
smiUINT32 lenTrap;
if (!pdu)
   return (FALSE);
// Adjust "generic" for v2 values
pdu->v1Trap->generic_trap++;        // as oid in v2 is v1 # +1
if (pdu->v1Trap->generic_trap == 7) // specific?
   pdu->v1Trap->generic_trap = 0;   // v2
// rfc1908:(2)  If a Trap-PDU is received, then it is mapped into a
// SNMPv2-Trap-PDU.  This is done by prepending onto the variable-bindings
// field two new bindings: sysUpTime.0 [12], which takes its value from the
// timestamp field of the Trap-PDU; and.......
if (!(VbTicks = (LPVARBIND)GlobalAlloc (GPTR, sizeof(VARBIND))))
   return (FALSE);
if (SnmpOidCopy (&sysUpTimeName, &VbTicks->name) == SNMPAPI_FAILURE)
   goto ERROROUT;
VbTicks->value.syntax = SNMP_SYNTAX_TIMETICKS;
VbTicks->value.value.uNumber = pdu->v1Trap->time_ticks;
// ..... snmpTrapOID.0 [13], which is calculated thusly: if the value of
// generic-trap field is `enterpriseSpecific', then the value used is the
// concatenation of the enterprise field from the Trap-PDU with two additional
// sub-identifiers, `0', and the value of the specific-trap field; otherwise,
// the value of the corresponding trap defined in [13] is used.  (For example,
// if the value of the generic-trap field is `coldStart', then the coldStart
// trap [13] is used.)
if (!(VbTrap = (LPVARBIND)GlobalAlloc (GPTR, sizeof(VARBIND))))
   goto ERROROUT;
if (SnmpOidCopy (&snmpTrapOidName, &VbTrap->name) == SNMPAPI_FAILURE)
   goto ERROROUT;
VbTrap->value.syntax = SNMP_SYNTAX_OID;
if (snmpTrapsValue[9] = pdu->v1Trap->generic_trap) // Deliberate assignment
   { // SNMP_TRAP_GENERIC
   lenTrap = sizeof(snmpTrapsValue);
   VbTrap->value.value.oid.len = lenTrap / sizeof(smiUINT32);
   ptrTrap = snmpTrapsValue;
   }
else
   { // SNMP_TRAP_ENTERPRISE
   lenTrap = pdu->v1Trap->enterprise.len * sizeof(smiUINT32);
   VbTrap->value.value.oid.len = pdu->v1Trap->enterprise.len + 2;
   ptrTrap = pdu->v1Trap->enterprise.ptr;
   }
if (!(VbTrap->value.value.oid.ptr = (smiLPUINT32)GlobalAlloc
      (GPTR, VbTrap->value.value.oid.len * sizeof(smiUINT32))))
   goto ERROROUT;
CopyMemory (VbTrap->value.value.oid.ptr, ptrTrap, lenTrap);
if (!pdu->v1Trap->generic_trap)
   { // SNMP_TRAP_ENTERPRISE
   VbTrap->value.value.oid.ptr[pdu->v1Trap->enterprise.len+1] =
      pdu->v1Trap->specific_trap;
   }
// Special code to retain v1Trap AgentAddress info in an experimental object
// This is *not* part of the WinSNMP v2.0 standard at this time (6/25/98)
if (TaskData.conveyAddress != SNMPAPI_ON)
   goto DO_ENTERPRISE;
if (!(VbAddress = (LPVARBIND)GlobalAlloc (GPTR, sizeof(VARBIND))))
   goto ERROROUT;
if (SnmpOidCopy (&snmpTrapAddrName, &VbAddress->name) == SNMPAPI_FAILURE)
   goto ERROROUT;
VbAddress->value.syntax = SNMP_SYNTAX_IPADDR;
// *Re-use* this OID parsed in WSNMP_BN code
VbAddress->value.value.string.len = pdu->v1Trap->agent_addr.len;
VbAddress->value.value.string.ptr = pdu->v1Trap->agent_addr.ptr;
pdu->v1Trap->agent_addr.len = 0;    // Setting .ptr to NULL required
pdu->v1Trap->agent_addr.ptr = NULL; // for later call to FreeV1Trap()
DO_ENTERPRISE:
// Then,......one new binding is appended onto the variable-bindings field:
// snmpTrapEnterpriseOID.0 [13], which takes its value from the enterprise field
// of the Trap-PDU.
//
// WINSNMP specs in SnmpRecvMsg specifies this append for both the generic and
// specific traps and not only for specific as RFC 1452 does.
if (!(VbEnterprise = (LPVARBIND)GlobalAlloc (GPTR, sizeof(VARBIND))))
   goto ERROROUT;
if (SnmpOidCopy (&snmpTrapEntName, &VbEnterprise->name) == SNMPAPI_FAILURE)
   goto ERROROUT;
VbEnterprise->value.syntax = SNMP_SYNTAX_OID;
// *Re-use* this OID parsed in WSNMP_BN code
VbEnterprise->value.value.oid.len = pdu->v1Trap->enterprise.len;
VbEnterprise->value.value.oid.ptr = pdu->v1Trap->enterprise.ptr;
pdu->v1Trap->enterprise.len = 0;    // Setting .ptr to NULL required
pdu->v1Trap->enterprise.ptr = NULL; // for later call to FreeV1Trap()

//We have all the variables set, just need to link them together
//backup the head of the original vbs
endPtr = pdu->VBL_addr;
// setup the new head of the list
pdu->VBL_addr = VbTicks;
VbTicks->next_var = VbTrap;
VbTrap->next_var = endPtr;
// position endPtr on the last varbind from the list
if (endPtr != NULL)
{
    // set it on the last varbind from the original V1 trap
    while (endPtr->next_var != NULL)
        endPtr = endPtr->next_var;
}
else
{
    // set it on VbTrap if no varbinds were passed in the V1 trap
    endPtr = VbTrap;
}
// append VbAddress if any and set endPtr on the new ending
if (VbAddress != NULL)
{
    endPtr->next_var = VbAddress;
    endPtr = VbAddress;
}
// append VbEnterprise
endPtr->next_var = VbEnterprise;
VbEnterprise->next_var = NULL;
//
// make it say it's an SNMPv2 Trap PDU
pdu->type = SNMP_PDU_TRAP;
// Assign a RequestID (not in SNMPv1 Trap PDUs (no need to lock)
pdu->appReqId = ++(TaskData.nLastReqId);
return (TRUE);
//
ERROROUT:
// Free only those resources created in this function
// FreeVarBind is a noop on NULLs, so no need to check first
FreeVarBind (VbEnterprise);
FreeVarBind (VbAddress);
FreeVarBind (VbTrap);
FreeVarBind (VbTicks);
return (FALSE);
} // end_MapV1TrapV2

BOOL DispatchTrap (LPSAS host, smiLPOCTETS community, LPPDUS pdu)
{
#define MAXSLOTS 10 // maximum active trap receivers
DWORD nCmp;
DWORD nFound = 0;
DWORD nTrap = 0;
DWORD nTraps[MAXSLOTS];
DWORD nMsg = 0;
LPSNMPMSG pMsg;
LPTRAPNOTICE pTrap;

EnterCriticalSection (&cs_TRAP);
for (nTrap = 0; nTrap < TrapDescr.Allocated; nTrap++)
   {
   pTrap = snmpGetTableEntry(&TrapDescr, nTrap);
   if (!pTrap->Session) continue;  // Active trap registration and
   if (pTrap->notification.len)    // all Traps?
      {                            // Nope, specific test

	  SNMPAPI_STATUS lError;
      // Next line is critical...do not remove...BN 3/8/96
      pTrap->notification.ptr = &(pTrap->notificationValue[0]);
      // 2nd param below assumes well-formed trap/inform...BN 1/21/97
      lError = SnmpOidCompare (&(pTrap->notification),
         &pdu->VBL_addr->next_var->value.value.oid,
         pTrap->notification.len, &nCmp);
      if (lError != SNMPAPI_SUCCESS || nCmp) continue;  // not equal...
      }
   if (pTrap->agentEntity)         // Specific agent?
      {
      int nResult;
      LPENTITY pEntity = snmpGetTableEntry(&EntsDescr, HandleToUlong(pTrap->agentEntity) - 1);
      if (host->ipx.sa_family == AF_IPX)
         nResult = memcmp (&host->ipx.sa_netnum,
                   &(pEntity->addr.ipx.sa_netnum), AF_IPX_ADDR_SIZE);
      else // AF_IPX
         nResult = memcmp (&host->inet.sin_addr,
                   &(pEntity->addr.inet.sin_addr), AF_INET_ADDR_SIZE);
      if (nResult)
         continue;                           // not equal...
      }
   if (pTrap->Context)             // Specific context?
      {
      LPCTXT pCtxt = snmpGetTableEntry(&CntxDescr, HandleToUlong(pTrap->Context) - 1);
      if (community->len != pCtxt->commLen)
         continue;                           // not equal...lengths
      if (memcmp (community->ptr, pCtxt->commStr,
                  (size_t)community->len))
         continue;                           // not equal...values
      }
   nTraps[nFound] = nTrap;                   // Got a match!
   nFound++; // Count the number found and check it against maximums
   if ((nFound == (MAXSLOTS)) || (nFound == TrapDescr.Used))
      break;
   } // end_for
LeaveCriticalSection (&cs_TRAP);
if (nFound == 0)  // Nothing to do
   return (SNMPAPI_FAILURE);
//
nCmp = nFound;    // Save count for later user
EnterCriticalSection (&cs_MSG);
while (nFound)
   {
   DWORD lError;

   lError = snmpAllocTableEntry(&MsgDescr, &nMsg);
   if (lError != SNMPAPI_SUCCESS)
   {
       LeaveCriticalSection(&cs_MSG);
       return lError;
   }
   pMsg = snmpGetTableEntry(&MsgDescr, nMsg);

   --nFound;
   nTrap = nTraps[nFound];
   nTraps[nFound] = nMsg;   // Need for later use
   pTrap = snmpGetTableEntry(&TrapDescr, nTrap);

   pMsg->Session = pTrap->Session;
   pMsg->Status = NP_RCVD;
   pMsg->Type = pdu->type;
   // 960522 - BN...
   // Need to increment the eventual "dstEntity" if
   // one was specified on the SnmpRegister() filter (unusual).
   // Deliberate assignment...
   if (pMsg->ourEntity = pTrap->ourEntity)
      {
      LPENTITY pEntity = snmpGetTableEntry(&EntsDescr, HandleToUlong(pTrap->ourEntity)-1);
      pEntity->refCount++;
      }

   // end_960522 - BN
   pMsg->dllReqId = pMsg->appReqId = pdu->appReqId;
   pMsg->Ticks = pMsg->Tries = 0;
   CopyMemory (&(pMsg->Host), host, sizeof(SAS));
   if (!(BuildMessage (1, community, pdu, pdu->appReqId,
         &(pMsg->Addr), &(pMsg->Size))))
      {
      snmpFreeTableEntry(&MsgDescr, nMsg);
      LeaveCriticalSection (&cs_MSG);
      return (SNMPAPI_PDU_INVALID);
      }
   } // end_while (nFound)
LeaveCriticalSection (&cs_MSG);
//
// The next while loop actually sends the one or more trap messages
// to the application(s)...
// This is due to the fact that we "clone" the incoming trap msg
// if there are multiple registrations for it.  BobN 2/20/95
while (nCmp) // Saved message count
   { // Now actually send the message(s)
   LPSESSION pSession;

   nMsg = nTraps[--nCmp];
   pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
   pSession = snmpGetTableEntry(&SessDescr, HandleToUlong(pMsg->Session) - 1);
   if (pSession->fCallBack)
      { // callback session notification mode
      EnterCriticalSection (&cs_SESSION);
      if (pSession->thrHandle)
         {
         if (pSession->thrCount != 0xFFFFFFFF)
            pSession->thrCount++;
         SetEvent (pSession->thrEvent);
         }
      else
         FreeMsg (nMsg);
      LeaveCriticalSection (&cs_SESSION);
      }
   else
      { // window/message session notification mode
      if (IsWindow(pSession->hWnd))
         {
         pMsg->Status = NP_READY;
         PostMessage (pSession->hWnd,
                      pSession->wMsg, 0, 0L);
         }
      else
         FreeMsg (nMsg);
      }
   } // end_while (nCmp)
return (SNMPAPI_SUCCESS);
} // end_DispatchTrap

LPPDUS MapV2TrapV1 (HSNMP_PDU hPdu)
{
// Convert SNMPv2 trap to SNMPv1 trap, for sending only
HSNMP_VBL hNewVbl = NULL;
LPPDUS oldPdu = NULL;
LPPDUS newPdu = NULL;
smiUINT32 lCount;
smiUINT32 lCmp;
smiUINT32 i;
smiLPBYTE tmpPtr = NULL;
smiOID sName;
smiVALUE sValue;
//
if (hPdu == NULL)
   return (NULL);
oldPdu = snmpGetTableEntry(&PDUsDescr, HandleToUlong(hPdu)-1);
if (oldPdu->type != SNMP_PDU_TRAP)
   return (NULL);
SnmpGetPduData (hPdu, NULL, NULL, NULL, NULL, &hNewVbl);
if (hNewVbl == NULL)
   return (NULL);
newPdu = GlobalAlloc (GPTR, sizeof(PDUS));
if (newPdu == NULL)
   goto ERR_OUT;
// From RFC 2089
// 3.3  Processing an outgoing SNMPv2 TRAP
//
// If SNMPv2 compliant instrumentation presents an SNMPv2 trap to the
// SNMP engine and such a trap passes all regular checking and then is
// to be sent to an SNMPv1 destination, then the following steps must be
// followed to convert such a trap to an SNMPv1 trap.  This is basically
// the reverse of the SNMPv1 to SNMPv2 mapping as described in RFC1908
// [3].
newPdu->type = SNMP_PDU_V1TRAP;
newPdu->v1Trap = GlobalAlloc (GPTR, sizeof(V1TRAP));
if (newPdu->v1Trap == NULL)
   goto ERR_OUT;
//
//   1.  If any of the varBinds in the varBindList has an SNMPv2 syntax
//       of Counter64, then such varBinds are implicitly considered to
//       be not in view, and so they are removed from the varBindList to
//       be sent with the SNMPv1 trap.
//
// We will do that step later, but check the VB count for now:
lCount = SnmpCountVbl (hNewVbl);
// Need at least 2 for sysUptime and snmpTrapOID!
if (lCount < 2)
   goto ERR_OUT;
//
//   2.  The 3 special varBinds in the varBindList of an SNMPv2 trap
//       (sysUpTime.0 (TimeTicks), snmpTrapOID.0 (OBJECT IDENTIFIER) and
//       optionally snmpTrapEnterprise.0 (OBJECT IDENTIFIER)) are
//       removed from the varBindList to be sent with the SNMPv1 trap.
//       These 2 (or 3) varBinds are used to decide how to set other
//       fields in the SNMPv1 trap PDU as follows:
//
//       a.  The value of sysUpTime.0 is copied into the timestamp field
//           of the SNMPv1 trap.
//
SnmpGetVb (hNewVbl, 1, &sName, &sValue);
SnmpOidCompare (&sysUpTimeName, &sName, 0, &lCmp);
SnmpFreeDescriptor (SNMP_SYNTAX_OID, (smiLPOPAQUE)&sName);
   if (lCmp != 0)
      goto ERR_OUT;
newPdu->v1Trap->time_ticks = sValue.value.uNumber;
SnmpDeleteVb (hNewVbl, 1);
lCount--;
//
//       b.  If the snmpTrapOID.0 value is one of the standard traps
//           the specific-trap field is set to zero and the generic
//           trap field is set according to this mapping:
//
//              value of snmpTrapOID.0                generic-trap
//              ===============================       ============
//              1.3.6.1.6.3.1.1.5.1 (coldStart)                  0
//              1.3.6.1.6.3.1.1.5.2 (warmStart)                  1
//              1.3.6.1.6.3.1.1.5.3 (linkDown)                   2
//              1.3.6.1.6.3.1.1.5.4 (linkUp)                     3
//              1.3.6.1.6.3.1.1.5.5 (authenticationFailure)      4
//              1.3.6.1.6.3.1.1.5.6 (egpNeighborLoss)            5
//
SnmpGetVb (hNewVbl, 1, &sName, &sValue);
SnmpOidCompare (&snmpTrapOidName, &sName, 0, &lCmp);
SnmpFreeDescriptor (SNMP_SYNTAX_OID, (smiLPOPAQUE)&sName);
   if (lCmp != 0)
      goto ERR_OUT;
SnmpOidCompare (&snmpTrapsName, &sValue.value.oid, 9, &lCmp);
if (!lCmp)
   {
   newPdu->v1Trap->generic_trap = sValue.value.oid.ptr[9] - 1;
   newPdu->v1Trap->specific_trap = 0;
//           The enterprise field is set to the value of
//           snmpTrapEnterprise.0 if this varBind is present, otherwise
//           it is set to the value snmpTraps as defined in RFC1907 [4].
   i = snmpTrapsName.len - 1;
   tmpPtr = (smiLPBYTE)snmpTrapsValue;
   }
//
//       c.  If the snmpTrapOID.0 value is not one of the standard
//           traps, then the generic-trap field is set to 6 and the
//           specific-trap field is set to the last subid of the
//           snmpTrapOID.0 value.
else
   {
   newPdu->v1Trap->generic_trap = 6;
   i = sValue.value.oid.len;
   newPdu->v1Trap->specific_trap = sValue.value.oid.ptr[i-1];
   tmpPtr = (smiLPBYTE)sValue.value.oid.ptr;
//
//           o   If the next to last subid of snmpTrapOID.0 is zero,
//               then the enterprise field is set to snmpTrapOID.0 value
//               and the last 2 subids are truncated from that value.
   if (sValue.value.oid.ptr[i-2] == 0)
      i -= 2;
//           o   If the next to last subid of snmpTrapOID.0 is not zero,
//               then the enterprise field is set to snmpTrapOID.0 value
//               and the last 1 subid is truncated from that value.
   else
      i -= 1;
//           In any event, the snmpTrapEnterprise.0 varBind (if present)
//           is ignored in this case.
   }
//
newPdu->v1Trap->enterprise.len = i;
i *= sizeof(smiUINT32);
// This allocation might have to be freed later,
// if generic trap and SnmpTrapEnterprise.0 is present in the varbindlist.
newPdu->v1Trap->enterprise.ptr = GlobalAlloc (GPTR, i);
if (newPdu->v1Trap->enterprise.ptr == NULL)
   {
   SnmpFreeDescriptor (SNMP_SYNTAX_OID, (smiLPOPAQUE)&sValue.value.oid);
   goto ERR_OUT;
   }
CopyMemory (newPdu->v1Trap->enterprise.ptr, tmpPtr, i);
SnmpFreeDescriptor (SNMP_SYNTAX_OID, (smiLPOPAQUE)&sValue.value.oid);
SnmpDeleteVb (hNewVbl, 1);
lCount--;
//
i = 1;
while (i <= lCount)
   {
   SnmpGetVb (hNewVbl, i, &sName, &sValue);
   if (sValue.syntax == SNMP_SYNTAX_CNTR64)
      {
      SnmpDeleteVb (hNewVbl, i);
      lCount--;
      goto LOOP;
      }
   SnmpOidCompare (&snmpTrapEntName, &sName, 0, &lCmp);
   if (lCmp == 0)
      {
      if (newPdu->v1Trap->specific_trap == 0)
         {
         if  (newPdu->v1Trap->enterprise.ptr)
            GlobalFree (newPdu->v1Trap->enterprise.ptr);
         lCmp = sValue.value.oid.len * sizeof(smiUINT32);
         newPdu->v1Trap->enterprise.ptr = GlobalAlloc (GPTR, lCmp);
         if (newPdu->v1Trap->enterprise.ptr == NULL)
            goto ERR_OUT;
         newPdu->v1Trap->enterprise.len = sValue.value.oid.len;
         CopyMemory (newPdu->v1Trap->enterprise.ptr,
                     sValue.value.oid.ptr, lCmp);
         }
      SnmpDeleteVb (hNewVbl, i);
      lCount--;
      goto LOOP;
      }
   i++;
LOOP:
   SnmpFreeDescriptor (SNMP_SYNTAX_OID, (smiLPOPAQUE)&sName);
   SnmpFreeDescriptor (sValue.syntax, (smiLPOPAQUE)&sValue.value.oid);
   }
if (lCount > 0)
   {
   LPVBLS pVbl = snmpGetTableEntry(&VBLsDescr, HandleToUlong(hNewVbl)-1);
   // Retain existing varbindlist remainder
   newPdu->VBL_addr = pVbl->vbList;
   // Flag it as gone for subsequent call to SnmpFreeVbl
   pVbl->vbList = NULL;
   }
SnmpFreeVbl (hNewVbl);
//
//   3.  The agent-addr field is set with the appropriate address of the
//       the sending SNMP entity, which is the IP address of the sending
//       entity of the trap goes out over UDP; otherwise the agent-addr
//       field is set to address 0.0.0.0.
newPdu->v1Trap->agent_addr.len = sizeof(DWORD);
newPdu->v1Trap->agent_addr.ptr = GlobalAlloc (GPTR, sizeof(DWORD));
if (newPdu->v1Trap->agent_addr.ptr == NULL)
   goto ERR_OUT;
if (TaskData.localAddress == 0)
   { // Get the local machine address (for outgoing v1 traps)
   char szLclHost [MAX_HOSTNAME];
   LPHOSTENT lpstHostent;
   if (gethostname (szLclHost, MAX_HOSTNAME) != SOCKET_ERROR)
      {
      lpstHostent = gethostbyname ((LPSTR)szLclHost);
      if (lpstHostent)
         TaskData.localAddress = *((LPDWORD)(lpstHostent->h_addr));
      }
   }
*(LPDWORD)newPdu->v1Trap->agent_addr.ptr = TaskData.localAddress;
// end_RFC2089
return (newPdu);
//
ERR_OUT:
SnmpFreeVbl (hNewVbl);
if (newPdu)
   {
   FreeVarBindList (newPdu->VBL_addr); // Checks for NULL
   FreeV1Trap (newPdu->v1Trap);        // Checks for NULL
   GlobalFree (newPdu);
   }
return (NULL);
} // end_MapV2TrapV1
