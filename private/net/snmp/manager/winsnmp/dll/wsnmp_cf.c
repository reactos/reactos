// wsnmp_cf.c
//
// WinSNMP Communications Functions and helpers
// Copyright 1995-1998 ACE*COMM Corp
// Rleased to Microsoft under Contract
//
// Bob Natale (bnatale@acecomm.com)
//
// 19980625 - Modified SnmpStartup() to allow for NULL
//            output args and to check for IsBadWritePtr()
//            when non-NULL
//
#include "winsnmp.inc"

#define SNMP_MAJOR_VERSION 2
#define SNMP_MINOR_VERSION 0
#define SNMP_SUPPORT_LEVEL SNMPAPI_V2_SUPPORT

#ifdef SOLARIS
BOOL DllMain (HINSTANCE, DWORD, LPVOID);
#endif // SOLARIS

LPPDUS MapV2TrapV1 (HSNMP_PDU hPdu);
THR_TYPE WINAPI thrManager (LPVOID);
THR_TYPE WINAPI thrTrap (LPVOID);
THR_TYPE WINAPI thrTimer (LPVOID);
THR_TYPE WINAPI thrAgent (LPVOID);
THR_TYPE WINAPI thrNotify (LPVOID);

void FreeRegister (DWORD nTrap)
{
LPTRAPNOTICE pTrap;
EnterCriticalSection (&cs_TRAP);
pTrap = snmpGetTableEntry(&TrapDescr, nTrap);
if (pTrap->ourEntity)
   SnmpFreeEntity (pTrap->ourEntity);
if (pTrap->agentEntity)
   SnmpFreeEntity (pTrap->agentEntity);
if (pTrap->Context)
   SnmpFreeContext (pTrap->Context);
snmpFreeTableEntry(&TrapDescr, nTrap);
LeaveCriticalSection (&cs_TRAP);
return;
} // end_FreeRegister

// Exported Functions
// SnmpStartup
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpStartup (OUT smiLPUINT32 nMajorVersion,
                OUT smiLPUINT32 nMinorVersion,
                OUT smiLPUINT32 nLevel,
                OUT smiLPUINT32 nTranslateMode,
                OUT smiLPUINT32 nRetransmitMode)
{
WSADATA wsaData;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION hTask = (HSNMP_SESSION)GetCurrentProcessId();
//
#ifdef SOLARIS
if (TaskData.hTask != hTask)
   DllMain (NULL, DLL_PROCESS_ATTACH, NULL);
#endif
//
if (nMajorVersion)
   {
   if (IsBadWritePtr (nMajorVersion, sizeof(smiUINT32)))
      goto ARG_ERROR;
   *nMajorVersion = SNMP_MAJOR_VERSION;
   }
if (nMinorVersion)
   {
   if (IsBadWritePtr (nMinorVersion, sizeof(smiUINT32)))
      goto ARG_ERROR;
   *nMinorVersion = SNMP_MINOR_VERSION;
   }
if (nLevel)
   {
   if (IsBadWritePtr (nLevel, sizeof(smiUINT32)))
      goto ARG_ERROR;
   *nLevel = SNMP_SUPPORT_LEVEL;
   }
if (nTranslateMode)
   {
   if (IsBadWritePtr (nTranslateMode, sizeof(smiUINT32)))
      goto ARG_ERROR;
   *nTranslateMode  = SNMPAPI_UNTRANSLATED_V1;
   }
if (nRetransmitMode)
   {
   if (IsBadWritePtr (nRetransmitMode, sizeof(smiUINT32)))
      goto ARG_ERROR;
   *nRetransmitMode = SNMPAPI_ON;
   }
goto ARGS_OK;
ARG_ERROR:
lError = SNMPAPI_ALLOC_ERROR;
goto ERROR_OUT;
ARGS_OK:
EnterCriticalSection (&cs_TASK);
TaskData.nRetransmitMode = SNMPAPI_ON;
TaskData.nTranslateMode  = SNMPAPI_UNTRANSLATED_V1;
// SnmpStartup is idempotent...
if (TaskData.hTask == hTask)
   goto DONE;  // ...already called
// New task starting up...get OS info
TaskData.sEnv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
GetVersionEx (&TaskData.sEnv);
// Start WinSock connection...should return 0
if (WSAStartup ((WORD)0x0101, &wsaData))
   {
   lError = SNMPAPI_TL_NOT_INITIALIZED;
   goto ERROR_PRECHECK;
   }
// Set trapPipe (used in NT case only)
TaskData.trapPipe = INVALID_HANDLE_VALUE;
// Set trapSock (used in Win95 case only)
TaskData.trapSock = INVALID_SOCKET;
// Set "manager" sockets (used at SnmpSendMsg() time)
TaskData.ipSock = TaskData.ipxSock = INVALID_SOCKET;
// Start timer thread
#ifdef SOLARIS
thr_create (NULL, 0, thrTimer, NULL, THR_FLAGS, &TaskData.timerThread);
#else
{
DWORD thrId;
TaskData.timerThread = (HANDLE)_beginthreadex (NULL, 0, thrTimer, NULL, 0, &thrId);
}
#endif // SOLARIS
//
DONE:
TaskData.hTask = hTask;
TaskData.nLastError = SNMPAPI_SUCCESS;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_TASK);
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpStartup

// SnmpCleanup
SNMPAPI_STATUS SNMPAPI_CALL SnmpCleanup (void)
{
DWORD nSession;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
// Variables for threads not associated with a specific session
DWORD nHandles = 0;
HANDLE hTemp[4] = {NULL, NULL, NULL, NULL};
CONST HANDLE *hObjects = &hTemp[0];
//--------------------------------------------------------------
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
EnterCriticalSection (&cs_SESSION);
// Do all Forgotten Closes
if (SessDescr.Used)
   {
   for (nSession = 0; nSession < SessDescr.Allocated; nSession++)
      if (((LPSESSION)snmpGetTableEntry(&SessDescr, nSession))->nTask)
         SnmpClose ((HSNMP_SESSION)(nSession + 1));
   }
LeaveCriticalSection (&cs_SESSION);
EnterCriticalSection (&cs_TASK);
// Terminate thrTimer
if (TaskData.timerThread)
   {
   hTemp[nHandles++] = TaskData.timerThread;
   // NULL signals the timer thread to terminate itself
   TaskData.timerThread = NULL;
   }
// Close "Mgr" sockets and threads
if (TaskData.ipSock != INVALID_SOCKET)
   {// UDP channel
   // check thrManager code to understand the lines below:
   SOCKET ipSock = TaskData.ipSock;
   TaskData.ipSock = INVALID_SOCKET;
   closesocket (ipSock);
   if (TaskData.ipThread)
      hTemp[nHandles++] = TaskData.ipThread;
   }
if (TaskData.ipxSock != INVALID_SOCKET)
   {// IPX channel
   // check thrManager code to understand the lines below:
   SOCKET ipxSock = TaskData.ipxSock;
   TaskData.ipxSock = INVALID_SOCKET;
   closesocket (ipxSock);
   if (TaskData.ipxThread)
      hTemp[nHandles++] = TaskData.ipxThread;
   }
// Terminate thrTrap
if (TaskData.trapThread)
   {
   if (TaskData.sEnv.dwPlatformId == VER_PLATFORM_WIN32_NT)
      { // NT-specific stuff
      TerminateThread (TaskData.trapThread, 0);
      if (TaskData.trapPipe != INVALID_HANDLE_VALUE)
         CloseHandle (TaskData.trapPipe);
      }
   if (TaskData.sEnv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
      { // Win95-specific stuff
      if (IsWindow (TaskData.trapWnd))
         PostMessage (TaskData.trapWnd, WSNMP_TRAPS_OFF, (DWORD_PTR)TaskData.hTask, 0L);
      if (TaskData.trapSock != INVALID_SOCKET)
         closesocket (TaskData.trapSock);
      }
   hTemp[nHandles++] = TaskData.trapThread;
   }
WaitForMultipleObjects (nHandles, hObjects, TRUE, 5000);
while (nHandles > 0)
   {
   nHandles--;
   CloseHandle (hTemp[nHandles]);
   }
// Do the main thing
ZeroMemory (&TaskData, sizeof(TASK));
LeaveCriticalSection (&cs_TASK);
// Close down WinSock connection
WSACleanup ();
//
#ifdef SOLARIS
DllMain (NULL, DLL_PROCESS_DETACH, NULL);
#endif // SOLARIS
//
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpCleanup

// Open a session (v1 and v2)
HSNMP_SESSION SNMPAPI_CALL SnmpOpen (IN HWND hWnd, IN UINT wMsg)
{
return (SnmpCreateSession (hWnd, wMsg, NULL, NULL));
} // end_SnmpOpen

// Open a session, w/callback option (v2)
HSNMP_SESSION SNMPAPI_CALL
   SnmpCreateSession (IN HWND hWnd, IN UINT wMsg,
                      IN SNMPAPI_CALLBACK fCallBack,
                      IN LPVOID lpClientData)
{
DWORD nSession;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
LPSESSION pSession;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
// Check for window/message notification mode argument validity
if (fCallBack == NULL)
   if (!IsWindow(hWnd))
      {
      lError = SNMPAPI_HWND_INVALID;
      goto ERROR_OUT;
      }
//
EnterCriticalSection (&cs_SESSION);
lError = snmpAllocTableEntry(&SessDescr, &nSession);
if (lError != SNMPAPI_SUCCESS)
	goto ERROR_PRECHECK;
pSession = snmpGetTableEntry(&SessDescr, nSession);

pSession->nTask        = TaskData.hTask;
pSession->hWnd         = hWnd;
pSession->wMsg         = wMsg;
pSession->fCallBack    = fCallBack;
pSession->lpClientData = lpClientData;
if (fCallBack)
   {
   DWORD thrId;
   pSession->thrEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
   pSession->thrCount = 0;
   pSession->thrHandle = (HANDLE)_beginthreadex
      (NULL, 0, thrNotify, (LPVOID)nSession, 0, &thrId);
   }
pSession->nLastError = SNMPAPI_SUCCESS;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_SESSION);
if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_SESSION)(nSession+1));
ERROR_OUT:
return ((HSNMP_SESSION)SaveError (0, lError));
} // end_SnmpOpen

// SnmpClose
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpClose (IN HSNMP_SESSION hSession)
{
HANDLE thrTemp;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
DWORD nSes = HandleToUlong(hSession) - 1;
DWORD i;
LPSESSION pSession;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&SessDescr, nSes))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
pSession = snmpGetTableEntry(&SessDescr, nSes);

// Strategy:
// 1st:  Stop notifications to session
// 2nd:  Stop accepting new messages
//       Traps
//       Agents
// 3rd:  Clear out pending messages
// 4th:  Free up all other resources
//
// PART_1:  Stop notifications to the closing Session
// Block window/message notification (in all cases!)
pSession->hWnd = NULL;
// Block callback notification (if required)
if (pSession->fCallBack != NULL)
   {
   // Save thrHandle for WaitForSingleObject call
   EnterCriticalSection (&cs_SESSION);
   thrTemp = pSession->thrHandle;
   // If this is a callback session, must stop thrNotify instance
   pSession->thrHandle = NULL;
   // 0xFFFFFFFF signals thrNotify instance to terminate itself
   pSession->thrCount = 0xFFFFFFFF;
   // SetEvent signals thrNotify instance to run
   SetEvent (pSession->thrEvent);
   LeaveCriticalSection (&cs_SESSION);

   // Wait for termination signal from thread handle
   WaitForSingleObject (thrTemp, 30000);
   // Close thrNotify instance handle
   CloseHandle (thrTemp);
   // Close thrNotify event handle
   CloseHandle (pSession->thrEvent);
   }

// PART_2:  Stop accepting new messages for the closing Session
// Free Notifications registered by the closing Session
EnterCriticalSection (&cs_TRAP);
for (i = 0; i < TrapDescr.Allocated && TrapDescr.Used != 0; i++)
   {
   LPTRAPNOTICE pTrap = snmpGetTableEntry(&TrapDescr, i);
   if (pTrap->Session == hSession)
      FreeRegister (i);
   } // end_for (Traps)
LeaveCriticalSection (&cs_TRAP);
// Free Agents registered by the closing Session
EnterCriticalSection (&cs_AGENT);
for (i = 0; i < AgentDescr.Allocated && AgentDescr.Used != 0; i++)
   {
   LPAGENT pAgent = snmpGetTableEntry(&AgentDescr, i);
   if (pAgent->Session == hSession)
      SnmpListen (pAgent->Entity, SNMPAPI_OFF);
   }
LeaveCriticalSection (&cs_AGENT);
// PART_3:  Free all pending messages for the closing Session
EnterCriticalSection (&cs_MSG);
for (i = 0; i < MsgDescr.Allocated && MsgDescr.Used != 0; i++)
   {
   LPSNMPMSG pMsg = snmpGetTableEntry(&MsgDescr, i);
   if (pMsg->Session == hSession)
      FreeMsg (i);
   }
LeaveCriticalSection (&cs_MSG);
// PART_4:  Free all other resources
// Free Entities allocated by the closing Session
EnterCriticalSection (&cs_ENTITY);
for (i = 0; i < EntsDescr.Allocated && EntsDescr.Used != 0; i++)
   {
   LPENTITY pEntity = snmpGetTableEntry(&EntsDescr, i);
   if (pEntity->Session == hSession)
      SnmpFreeEntity ((HSNMP_ENTITY)(i+1));
   }
LeaveCriticalSection (&cs_ENTITY);
// Free Contexts allocated by the closing Session
EnterCriticalSection (&cs_CONTEXT);
for (i = 0; i < CntxDescr.Allocated && CntxDescr.Used != 0; i++)
   {
   LPCTXT pCtxt = snmpGetTableEntry(&CntxDescr, i);
   if (pCtxt->Session == hSession)
      SnmpFreeContext ((HSNMP_CONTEXT)(i+1));
   }
LeaveCriticalSection (&cs_CONTEXT);
// Free VBLs allocated by the closing Session
EnterCriticalSection (&cs_VBL);
for (i = 0; i < VBLsDescr.Allocated && VBLsDescr.Used != 0; i++)
   {
   LPVBLS pVbl = snmpGetTableEntry(&VBLsDescr, i);
   if (pVbl->Session == hSession)
      SnmpFreeVbl ((HSNMP_VBL)(i+1));
   }
LeaveCriticalSection (&cs_VBL);
// Free PDUs allocated by the closing Session
EnterCriticalSection (&cs_PDU);

for (i = 0; i < PDUsDescr.Allocated && PDUsDescr.Used != 0; i++)
   {
   LPPDUS pPDU = snmpGetTableEntry(&PDUsDescr, i);
   if (pPDU->Session == hSession)
      SnmpFreePdu ((HSNMP_PDU)(i+1));
   }

LeaveCriticalSection (&cs_PDU);
// Free the Session table entry used by the closing Session
EnterCriticalSection (&cs_SESSION);
snmpFreeTableEntry(&SessDescr, nSes);
LeaveCriticalSection (&cs_SESSION);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
// As of 19980808 there are no error cases with a valid session
return (SaveError (0, lError));
} // end_SnmpClose

// SnmpSendMsg
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSendMsg (IN HSNMP_SESSION hSession,
                IN HSNMP_ENTITY hSrc,
                IN HSNMP_ENTITY hDst,
                IN HSNMP_CONTEXT hCtx,
                IN HSNMP_PDU hPdu)
{
LPPDUS sendPdu;
BOOL fMsg;
DWORD nMsg;
DWORD pduType;
smiINT32 dllReqId;
smiOCTETS tmpContext;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
//
DWORD thrId;
SOCKET *pSock;
int tFamily;
SOCKADDR tAddr;
HANDLE *pThread;
//
DWORD nSrc;
DWORD nDst;
DWORD nCtx;
DWORD nPdu;
//
BOOL  fBroadcast;
//
LPPDUS pPdu;
LPENTITY pEntSrc, pEntDst;
LPCTXT pCtxt;
LPSNMPMSG pMsg;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&SessDescr, HandleToUlong(hSession)-1))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }

// Save valid session for later error returns
lSession = hSession;
if (hSrc)  // Allowed to be NULL
   {
   nSrc = HandleToUlong(hSrc) - 1;
   if (!snmpValidTableEntry(&EntsDescr, nSrc))
      {
      lError = SNMPAPI_ENTITY_INVALID;
      goto ERROR_OUT;
      }
   pEntSrc = snmpGetTableEntry(&EntsDescr, nSrc);
   }
nDst = HandleToUlong(hDst) - 1;
if (!snmpValidTableEntry(&EntsDescr, nDst))
   {
   lError = SNMPAPI_ENTITY_INVALID;
   goto ERROR_OUT;
   }
pEntDst = snmpGetTableEntry(&EntsDescr, nDst);

nCtx = HandleToUlong(hCtx) - 1;
if (!snmpValidTableEntry(&CntxDescr, nCtx))
   {
   lError = SNMPAPI_CONTEXT_INVALID;
   goto ERROR_OUT;
   }
pCtxt = snmpGetTableEntry(&CntxDescr, nCtx);

nPdu = HandleToUlong(hPdu) - 1;
if (!snmpValidTableEntry(&PDUsDescr, nPdu))
   {
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

if (!snmpValidTableEntry(&VBLsDescr, HandleToUlong(pPdu->VBL)-1))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
//--------------
tFamily = pEntDst->addr.inet.sin_family;

// enter the critical section for the TaskData structure to insure
// the atomicity of the Test&Set operation of the TaskData.[ip|ipx]Thread
EnterCriticalSection (&cs_TASK);

pThread = (tFamily==AF_IPX) ? &TaskData.ipxThread : &TaskData.ipThread;
pSock = (tFamily==AF_IPX) ? &TaskData.ipxSock : &TaskData.ipSock;

if (*pThread)   // ASSERT(*pSock != INVALID_SOCKET)
   {
   LeaveCriticalSection(&cs_TASK);
   goto CHANNEL_OPEN;
   }
*pSock = socket (tFamily, SOCK_DGRAM, (tFamily==AF_IPX)?NSPROTO_IPX:0);

if (*pSock == INVALID_SOCKET)
   {
   LeaveCriticalSection(&cs_TASK);
   lError = SNMPAPI_TL_NOT_SUPPORTED;
   goto ERROR_OUT;
   }

// try to set the socket for broadcasts. No matter the result
// a possible error will be caught later
fBroadcast = TRUE;
setsockopt (*pSock,
            SOL_SOCKET,
			SO_BROADCAST,
			(CHAR *) &fBroadcast,
			sizeof ( BOOL )
		   );

// Kludge for Win95 WinSock/IPX bug...have to "bind"
ZeroMemory (&tAddr, sizeof(SOCKADDR));
tAddr.sa_family = (USHORT)tFamily;
bind (*pSock, &tAddr, (tFamily==AF_IPX)?sizeof(SOCKADDR_IPX):sizeof(SOCKADDR_IN));
// Start "listener" and timer threads
#ifdef SOLARIS
thr_create (NULL, 0, thrManager, (LPVOID)tSock, THR_FLAGS, pThread);
#else
*pThread = (HANDLE)_beginthreadex (NULL, 0, thrManager, (LPVOID)pSock, 0, &thrId);
#endif // SOLARIS
if (*pThread == NULL)
   {
   LeaveCriticalSection (&cs_TASK);
   closesocket (*pSock);
   *pSock = INVALID_SOCKET;
   lError = SNMPAPI_TL_RESOURCE_ERROR;
   goto ERROR_OUT;
   }
LeaveCriticalSection (&cs_TASK);
//---------------
CHANNEL_OPEN:
pduType = pPdu->type;
sendPdu = pPdu;
if (pEntDst->version == 1)
	{ // Test for special v2 msg -> v1 dst operations
   if (pduType == SNMP_PDU_TRAP)
      { // RFC 2089 v2 to v1 trap conversion
      sendPdu =  MapV2TrapV1 (hPdu);
      if (sendPdu == NULL)
         {
         lError = SNMPAPI_OTHER_ERROR;
         goto ERROR_OUT;
         }
      pduType = SNMP_PDU_V1TRAP;
      }
   else if (pduType == SNMP_PDU_INFORM)
      {
      lError = SNMPAPI_OPERATION_INVALID;
      goto ERROR_OUT;
      }
   }
// Space check
EnterCriticalSection (&cs_MSG);
lError = snmpAllocTableEntry(&MsgDescr, &nMsg);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pMsg = snmpGetTableEntry(&MsgDescr, nMsg);

// Now Build it
if (pduType == SNMP_PDU_RESPONSE || pduType == SNMP_PDU_TRAP)
   dllReqId = pPdu->appReqId;
else
   dllReqId = ++(TaskData.nLastReqId);
tmpContext.len = pCtxt->commLen;
tmpContext.ptr = pCtxt->commStr;
// Save BuildMessage status for later check
fMsg = BuildMessage (pEntDst->version-1, &tmpContext, sendPdu,
       dllReqId, &(pMsg->Addr), &(pMsg->Size));
// If v2 to v1 trap conversion was required, then cleanup...
if (pduType == SNMP_PDU_V1TRAP)
   {
   FreeVarBindList (sendPdu->VBL_addr);   // Checks for NULL
   FreeV1Trap (sendPdu->v1Trap);          // Checks for NULL
   GlobalFree (sendPdu);
   }
// If BuildMessage failed, that's all folks!
if (!fMsg)
   {
   snmpFreeTableEntry(&MsgDescr, nMsg);
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_PRECHECK;
   }
pMsg->Session = hSession;
pMsg->Status = NP_SEND;  // "send"
pMsg->Type = pduType;
pMsg->nRetransmitMode = TaskData.nRetransmitMode;
pMsg->dllReqId = dllReqId;
pMsg->appReqId = pPdu->appReqId;
pMsg->agentEntity = hDst;
pMsg->ourEntity   = hSrc;
pMsg->Context     = hCtx;
LeaveCriticalSection (&cs_MSG);
// Update reference counts for entities and contexts,
EnterCriticalSection (&cs_ENTITY);
if (hSrc)
   pEntSrc->refCount++;
pEntDst->refCount++;
LeaveCriticalSection (&cs_ENTITY);
EnterCriticalSection (&cs_CONTEXT);
pCtxt->refCount++;
LeaveCriticalSection (&cs_CONTEXT);
// Prepare addressing info for traps
EnterCriticalSection (&cs_MSG);
CopyMemory (&(pMsg->Host), &pEntDst->addr, sizeof(SAS));
if (pduType == SNMP_PDU_V1TRAP ||
    pduType == SNMP_PDU_TRAP ||
    pduType == SNMP_PDU_INFORM)
   {
   if (tFamily == AF_IPX)
      {
      if (pMsg->Host.ipx.sa_socket == ntohs (IPX_SNMP_PORT))
         pMsg->Host.ipx.sa_socket = htons (IPX_TRAP_PORT);
      }
   else // Assume AF_INET
      {
      if (pMsg->Host.inet.sin_port == ntohs (IP_SNMP_PORT))
         pMsg->Host.inet.sin_port = htons(IP_TRAP_PORT);
      }
   }
// Send the packet
thrId = sendto (*pSock, pMsg->Addr, pMsg->Size,
                0, (LPSOCKADDR)&(pMsg->Host), sizeof(SAS));
if (thrId == SOCKET_ERROR)
   {
   FreeMsg (nMsg);
   lError = SNMPAPI_TL_OTHER;
   goto ERROR_PRECHECK;
   }
// Need to check for SOCKET_ERROR!
if (pduType == SNMP_PDU_TRAP ||
    pduType == SNMP_PDU_V1TRAP ||
    pduType == SNMP_PDU_RESPONSE)
   {
   FreeMsg (nMsg);
   }
else
   {
   pMsg->Status = NP_SENT;
   // Time entity's timeout value is stored as centiseconds in 32 bits
   pMsg->Wait   = pEntDst->nPolicyTimeout;
   // Converting to milliseconds for timer operations could overflow
   if (pMsg->Wait <= MAXCENTISECONDS)  // So check first...if ok
      pMsg->Wait *= 10;                // Convert to milliseconds
   else                                         // eles...
      pMsg->Wait = MAXMILLISECONDS;    // Set to max milliseconds
   pMsg->Tries  = pEntDst->nPolicyRetry;
   pMsg->Ticks  = GetTickCount();
   }
ERROR_PRECHECK:
LeaveCriticalSection (&cs_MSG);
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpSendMsg

// SnmpRecvMsg
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpRecvMsg (IN HSNMP_SESSION hSession,
                OUT LPHSNMP_ENTITY srcEntity,
                OUT LPHSNMP_ENTITY dstEntity,
                OUT LPHSNMP_CONTEXT context,
                OUT LPHSNMP_PDU pdu)
{
DWORD nMsg;
DWORD nPdu;
int pduType;
smiLPOCTETS community;
smiUINT32 version;
smiUINT32 nMode;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
DWORD nSes = HandleToUlong(hSession) - 1;
LPPDUS pPdu;
LPENTITY pEntity;
LPSNMPMSG pMsg;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&SessDescr, nSes))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
// Valid session...save for possible error return
lSession = hSession;
EnterCriticalSection (&cs_MSG);
// Find a message for the calling session
for (nMsg = 0; nMsg < MsgDescr.Allocated; nMsg++)
   {
   pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
   if (pMsg->Session == hSession &&
       pMsg->Status == NP_READY)
      break;
   }
if (nMsg == MsgDescr.Allocated)
   {
   lError = SNMPAPI_NOOP;
   goto ERROR_PRECHECK1;
   }
if (!pMsg->Addr)
   {
   lError = SNMPAPI_MESSAGE_INVALID;
   goto ERROR_PRECHECK1;
   }
ERROR_PRECHECK1:
LeaveCriticalSection (&cs_MSG);
if (lError != SNMPAPI_SUCCESS)
   goto ERROR_OUT;
// Allocate a slot in PDU table
EnterCriticalSection (&cs_PDU);

lError = snmpAllocTableEntry(&PDUsDescr, &nPdu);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK2;
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

nMode = ParseMessage (pMsg->Addr, pMsg->Size,
                      &version, &community, pPdu);
if (nMode != 0) // non-zero = error code
   {
   snmpFreeTableEntry(&PDUsDescr, nPdu);
   FreeMsg (nMsg);
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_PRECHECK2;
   }
pPdu->Session  = hSession;
pPdu->appReqId = pMsg->appReqId;
ERROR_PRECHECK2:
LeaveCriticalSection (&cs_PDU);
if (lError != SNMPAPI_SUCCESS)
   goto ERROR_OUT;
pduType = pPdu->type;
EnterCriticalSection (&cs_ENTITY);

if (dstEntity)
   { // Deliberate assignment...
   if (*dstEntity = pMsg->ourEntity)
      {
      pEntity = snmpGetTableEntry(&EntsDescr, HandleToUlong(pMsg->ourEntity)-1);
      pEntity->refCount++;
      }
   }
if (srcEntity)
   {
    if (pduType == SNMP_PDU_TRAP ||
       pduType == SNMP_PDU_INFORM ||
       pduType != SNMP_PDU_RESPONSE)
      {
      int afType = pMsg->Host.ipx.sa_family;
      char afHost[24];
      EnterCriticalSection (&cs_XMODE);
      SnmpGetTranslateMode (&nMode);
      SnmpSetTranslateMode (SNMPAPI_UNTRANSLATED_V1);
      if (afType == AF_IPX)
         SnmpIpxAddressToStr (pMsg->Host.ipx.sa_netnum,
                              pMsg->Host.ipx.sa_nodenum,
                              afHost);
      else // AF_INET
         lstrcpy (afHost, inet_ntoa (pMsg->Host.inet.sin_addr));
      pMsg->agentEntity = SnmpStrToEntity (hSession, afHost);
      pEntity = snmpGetTableEntry(&EntsDescr, HandleToUlong(pMsg->agentEntity)-1);
      if (afType == AF_IPX)
         pEntity->addr.ipx.sa_socket = pMsg->Host.ipx.sa_socket;
      else // AF_INET
         pEntity->addr.inet.sin_port = pMsg->Host.inet.sin_port;
      SnmpSetTranslateMode (nMode);
      LeaveCriticalSection (&cs_XMODE);
      }
   // Deliberate assignment...
   if (*srcEntity = pMsg->agentEntity)
      {
      pEntity = snmpGetTableEntry(&EntsDescr, HandleToUlong(pMsg->agentEntity)-1);
      pEntity->refCount++;
      }
   }
LeaveCriticalSection (&cs_ENTITY);
EnterCriticalSection (&cs_CONTEXT);
if (context)
   {
   if (pduType == SNMP_PDU_TRAP ||
       pduType == SNMP_PDU_INFORM ||
       pduType != SNMP_PDU_RESPONSE)
      {
      EnterCriticalSection (&cs_XMODE);
      SnmpGetTranslateMode (&nMode);
      SnmpSetTranslateMode (SNMPAPI_UNTRANSLATED_V1);
      pMsg->Context = SnmpStrToContext (hSession, community);
      SnmpSetTranslateMode (nMode);
      LeaveCriticalSection (&cs_XMODE);
      }
   // Deliberate assignment...
   if (*context = pMsg->Context)
      ((LPCTXT)snmpGetTableEntry(&CntxDescr, HandleToUlong(pMsg->Context)-1))->refCount++;
   }
LeaveCriticalSection (&cs_CONTEXT);
FreeOctetString (community);
if (pdu)
   *pdu = (HSNMP_PDU)(nPdu+1);
else
   SnmpFreePdu ((HSNMP_PDU)(nPdu+1));
// Mark SendRecv slot as free
FreeMsg (nMsg);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpRecvMsg

// Allocates a generic ACL to be used for the security descriptor of the SNMPTRAP service
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
                                   GENERIC_ALL,
                                   pSidAdmins ) || 
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   GENERIC_READ | GENERIC_EXECUTE,
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

// frees a generic ACL
void FreeGenericACL( PACL pAcl)
{
    if (pAcl != NULL)
        GlobalFree(pAcl);
}

// SnmpRegister
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpRegister (IN HSNMP_SESSION hSession,
                 IN HSNMP_ENTITY hSrc,
                 IN HSNMP_ENTITY hDst,
                 IN HSNMP_CONTEXT hCtx,
                 IN smiLPCOID notification,
                 IN smiUINT32 status)
{
DWORD nNotice, nFound;
smiINT32 nCmp;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
DWORD nSes = HandleToUlong(hSession) - 1;
DWORD nSrc;
DWORD nDst;
DWORD nCtx;
LPENTITY pEntSrc, pEntDst;
LPCTXT pCtxt;
LPTRAPNOTICE pTrap;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (status != SNMPAPI_OFF) status = SNMPAPI_ON;
if (status == SNMPAPI_ON)
   {
   if (!snmpValidTableEntry(&SessDescr, nSes))
      {
      lError = SNMPAPI_SESSION_INVALID;
      goto ERROR_OUT;
      }
   else // Got a valid session...save for possible error return
      lSession = hSession;
   }
if (hSrc)
   {
   nSrc = HandleToUlong(hSrc) - 1;
   if (!snmpValidTableEntry(&EntsDescr, nSrc))
      {
      lError = SNMPAPI_ENTITY_INVALID;
      goto ERROR_OUT;
      }
   pEntSrc = snmpGetTableEntry(&EntsDescr, nSrc);
   }
if (hDst)
   {
   nDst = HandleToUlong(hDst) - 1;
   if (!snmpValidTableEntry(&EntsDescr, nDst))
      {
      lError = SNMPAPI_ENTITY_INVALID;
      goto ERROR_OUT;
      }
   pEntDst = snmpGetTableEntry(&EntsDescr, nDst);
   }
if (hCtx)
   {
   nCtx = HandleToUlong(hCtx) - 1;
   if (!snmpValidTableEntry(&CntxDescr, nCtx))
      {
      lError = SNMPAPI_CONTEXT_INVALID;
      goto ERROR_OUT;
      }
   pCtxt = snmpGetTableEntry(&CntxDescr, nCtx);
   }
if (notification)
   {
   if ((!notification->len) || notification->len > MAXOBJIDSIZE)
      {
      lError = SNMPAPI_SIZE_INVALID;
      goto ERROR_OUT;
      }
   if (!notification->ptr)
      {
      lError = SNMPAPI_OID_INVALID;
      goto ERROR_OUT;
      }
   }
EnterCriticalSection (&cs_TRAP);
for (nNotice = 0, nFound = 0; nNotice < TrapDescr.Allocated,
                              nFound < TrapDescr.Used; nNotice++)
   { // First, count now many we've tested
   pTrap = snmpGetTableEntry(&TrapDescr, nNotice);
   if (pTrap->Session) nFound++;
   // then search for a parameter matches
   if ((pTrap->Session == hSession) &&
       (pTrap->ourEntity == hSrc) &&
       (pTrap->agentEntity == hDst) &&
       (pTrap->Context == hCtx))
      { // Ok, we found one
      if (!notification)
         // if the notification parameter is null, then we
         // want to either turn on or turn off all notifications
         // from this match...so clear any entries already in
         // the table and we'll add this wildcard entry if the
         // operation is SNMPAPI_ON at the end.
         {
         FreeRegister (nNotice);
         continue;
         }
      else // notification specified
         {
         if (!pTrap->notification.len)
            {
            // Redundant request (already wildcarded)
            // Skip it and return!
            goto ERROR_PRECHECK;
            }
         else // pTrap->notification
            {
            // compare OIDs
            SnmpOidCompare (notification, &(pTrap->notification),
                            0, &nCmp);
            if (nCmp)      // no match
               continue;   // ...try the next one
            else // !nCcmp
               { // got a match...
               // if SNMPAPI_ON, redundant request...skip it and return
               // if SNMPAPI_OFF, free the entry first
               if (status != SNMPAPI_ON)
                  FreeRegister (nNotice); // SNMPAPI_OFF
               goto ERROR_PRECHECK;
               } // end_else_!nCmp
            } // end_else_TrapTable[nNotice].notificatin
         } // end_else_notification_specified
      } // end_if_we_found_one
   } // end_for
if (status == SNMPAPI_OFF)
   { // Found nothing to turn off...that's ok.
   goto ERROR_PRECHECK;
   }
//
#ifndef SOLARIS
// Special check for NT...is SNMPTRAP service running?
if (TaskData.trapThread == NULL &&
    TaskData.sEnv.dwPlatformId == VER_PLATFORM_WIN32_NT)
   {
   DWORD   dwReturn  = SNMPAPI_TL_NOT_INITIALIZED;
   DWORD   pMode     = PIPE_WAIT | PIPE_READMODE_MESSAGE;
   LPCTSTR svcName   = "SNMPTRAP";
   LPCTSTR svcDesc   = "SNMP Trap Service";
   LPCTSTR svcPath   = "%SystemRoot%\\system32\\snmptrap.exe";
   SC_HANDLE scmHandle = NULL;
   SC_HANDLE svcHandle = NULL;
   SERVICE_STATUS svcStatus;
   BOOL fStatus;
   // Minimal SCM connection, for case when SNMPTRAP is running
   scmHandle = OpenSCManager (NULL, NULL, SC_MANAGER_CONNECT);
   if (scmHandle == NULL)
      goto DONE_SC;
   svcHandle = OpenService (scmHandle, svcName, SERVICE_QUERY_STATUS);
   if (svcHandle == NULL)
      {
      if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
         goto DONE_SC;
      else
         { // Must attempt to create service
         PACL pAcl;
         SECURITY_DESCRIPTOR S_Desc;
         // Need new scmHandle with admin priv
         CloseServiceHandle (scmHandle);
         scmHandle = OpenSCManager (NULL, NULL, SC_MANAGER_CREATE_SERVICE);
         if (scmHandle == NULL)
            goto DONE_SC; // Could not open SCM with admin priv
         svcHandle = CreateService (scmHandle, svcName, svcDesc,
                                    WRITE_DAC|SERVICE_QUERY_STATUS,
                                    SERVICE_WIN32_OWN_PROCESS,
                                    SERVICE_DEMAND_START,
                                    SERVICE_ERROR_NORMAL,
                                    svcPath,
                                    NULL, NULL,
                                    "TCPIP\0EventLog\0\0",
                                    NULL, NULL);
         if (svcHandle == NULL)
            goto DONE_SC; // Could not create service
         InitializeSecurityDescriptor (&S_Desc, SECURITY_DESCRIPTOR_REVISION);
         if ((pAcl = AllocGenericACL()) == NULL ||
             !SetSecurityDescriptorDacl (&S_Desc, TRUE, pAcl, FALSE))
         {
             FreeGenericACL(pAcl);
             goto DONE_SC;
         }
         SetServiceObjectSecurity (svcHandle, DACL_SECURITY_INFORMATION, &S_Desc);
         FreeGenericACL(pAcl);
         }
      }
   fStatus = QueryServiceStatus (svcHandle, &svcStatus);
   while (fStatus)
      {
      switch (svcStatus.dwCurrentState)
         {
         case SERVICE_RUNNING:
         dwReturn = SNMPAPI_SUCCESS;
         goto DONE_SC;

         case SERVICE_STOPPED:
         // Start SNMPTRAP service if necessary
         CloseServiceHandle (svcHandle);
         svcHandle = OpenService (scmHandle, svcName, SERVICE_START|SERVICE_QUERY_STATUS);
         if (svcHandle == NULL)
            goto DONE_SC; // Could not start service
         svcStatus.dwCurrentState = SERVICE_START_PENDING;
         fStatus = StartService (svcHandle, 0, NULL);
         break;

         case SERVICE_STOP_PENDING:
         case SERVICE_START_PENDING:
         Sleep (MAX_PENDING_WAIT);
         fStatus = QueryServiceStatus (svcHandle, &svcStatus);
         break;

         case SERVICE_PAUSED:
         case SERVICE_PAUSE_PENDING:
         case SERVICE_CONTINUE_PENDING:
         default:
         fStatus = FALSE;  // Nothing to do about these
         break;
         }
      }
DONE_SC:
   if (scmHandle)
      CloseServiceHandle (scmHandle);
   if (svcHandle)
      CloseServiceHandle (svcHandle);
   if (dwReturn != SNMPAPI_SUCCESS)
      {
ERROR_PRECHECK1:
      lError = dwReturn;
      goto ERROR_PRECHECK;
      }
   // Setup for pipe-oriented operations
   dwReturn = SNMPAPI_TL_RESOURCE_ERROR;
   // block on instance of server pipe becoming available
   if (!WaitNamedPipe (SNMPTRAPPIPE, TRAPSERVERTIMEOUT))
      goto ERROR_PRECHECK1;
   TaskData.trapPipe =
      CreateFile (SNMPTRAPPIPE, GENERIC_READ|GENERIC_WRITE,
                  FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (TaskData.trapPipe == INVALID_HANDLE_VALUE)
      goto ERROR_PRECHECK1;
   if (!SetNamedPipeHandleState (TaskData.trapPipe, &pMode, NULL, NULL))
      {
      CloseHandle (TaskData.trapPipe);
      TaskData.trapPipe = INVALID_HANDLE_VALUE;
      goto ERROR_PRECHECK1;
      }
   } // end_NT check for SNMPTRAP service
#endif // !SOLARIS
//If we got this far, add it
lError = snmpAllocTableEntry(&TrapDescr, &nNotice);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pTrap = snmpGetTableEntry(&TrapDescr, nNotice);

// add it
pTrap->Session = hSession;
// Deliberate assignments in next three if statements
if (pTrap->ourEntity = hSrc)
   //EntityTable[nSrc-1].refCount++; -- was this a bug??? nSrc is already 0 based
    pEntSrc->refCount++;
if (pTrap->agentEntity = hDst)
   //EntityTable[nDst-1].refCount++; -- was this a bug??? nDst is already 0 based
    pEntDst->refCount++;
if (pTrap->Context = hCtx)
   //ContextTable[nCtx-1].refCount++; -- was this a bug?? nCtx is already 0 based
   pCtxt->refCount++;
if (notification)
   { // Reproduce the OID
   pTrap->notification.ptr = NULL;
   // Deliberate assignment in next statement
   if (pTrap->notification.len = notification->len)
      {
      if (pTrap->notification.len > MAXTRAPIDS)
         pTrap->notification.len = MAXTRAPIDS;
      if (notification->ptr)
         {
         // Deliberate assignment in next statement
         pTrap->notification.ptr = &(pTrap->notificationValue[0]);
         CopyMemory (pTrap->notification.ptr, notification->ptr,
                     pTrap->notification.len * sizeof(smiUINT32));
         }
      }
   }
if (TaskData.trapThread == NULL)
   {
// Leave # lines at margin (for Solaris cc compiler)
#ifdef SOLARIS
   thr_create (NULL, 0, thrTrap, NULL, THR_FLAGS, &TaskData.trapThread);
#else // Win32
   DWORD thrId;
   TaskData.trapThread = (HANDLE)_beginthreadex (NULL, 0, thrTrap, NULL, 0, &thrId);
#endif
   }
ERROR_PRECHECK:
LeaveCriticalSection (&cs_TRAP);
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpRegister

void FreeMsg (DWORD nMsg)
{
LPSNMPMSG pMsg;
EnterCriticalSection (&cs_MSG);
// Decrement reference counts
pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
SnmpFreeEntity (pMsg->agentEntity);
SnmpFreeEntity (pMsg->ourEntity);
SnmpFreeContext (pMsg->Context);
if (pMsg->Addr)
   GlobalFree (pMsg->Addr);
snmpFreeTableEntry(&MsgDescr, nMsg);
LeaveCriticalSection (&cs_MSG);
return;
} // end_FreeMsg

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpListen (IN HSNMP_ENTITY hEntity,
               IN smiUINT32 status)
{
smiUINT32 nAgent = 0;
DWORD thrId;
DWORD nEntity = HandleToUlong(hEntity) - 1;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPENTITY pEntity;
LPAGENT pAgent;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&EntsDescr, nEntity))
   {
   lError = SNMPAPI_ENTITY_INVALID;
   goto ERROR_OUT;
   }
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);
lSession = pEntity->Session;
if (status != SNMPAPI_ON && status != SNMPAPI_OFF)
   {
   lError = SNMPAPI_MODE_INVALID;
   goto ERROR_OUT;
   }
EnterCriticalSection (&cs_ENTITY);
EnterCriticalSection (&cs_AGENT);
if (status)
   { // status == SNMPAPI_ON
   int nProto = IPPROTO_UDP;
   int nSize = sizeof(SOCKADDR_IN);
   int nFamily = pEntity->addr.inet.sin_family;
   if (pEntity->Agent)
      { // Entity already running as agent
      lError = SNMPAPI_NOOP;
      goto ERROR_PRECHECK;
      }
   // Allocate a slot in AGENT table
   lError = snmpAllocTableEntry(&AgentDescr, &nAgent);
   if (lError != SNMPAPI_SUCCESS)
       goto ERROR_PRECHECK;
   pAgent = snmpGetTableEntry(&AgentDescr, nAgent);

// Agent table entry allocated...setup for agent thread
   if (nFamily == AF_IPX)
      {
      nProto = NSPROTO_IPX;
      nSize = sizeof(SOCKADDR_IPX);
      }
   pAgent->Socket = socket (nFamily, SOCK_DGRAM, nProto);
   if (pAgent->Socket == INVALID_SOCKET)
      {
      snmpFreeTableEntry(&AgentDescr, nAgent);
      lError = SNMPAPI_TL_RESOURCE_ERROR;
      goto ERROR_PRECHECK;
      }
   if (bind (pAgent->Socket,
            (LPSOCKADDR)&pEntity->addr, nSize)
      == SOCKET_ERROR)
      {
      snmpFreeTableEntry(&AgentDescr, nAgent);
      closesocket (pAgent->Socket);
      lError = SNMPAPI_TL_OTHER;
      goto ERROR_PRECHECK;
      }
   // Make Entity and Agent point to each other
   pEntity->Agent = nAgent + 1;
   pAgent->Entity = hEntity;
   pAgent->Session = lSession;
   // Create agent thread...needs error checking
#ifdef SOLARIS // Leave at left margin for Solaris compiler
   thr_create (NULL, 0, thrAgent, (LPVOID)nAgent, THR_FLAGS, &(pAgent->Thread));
#else  // Win32
   pAgent->Thread = (HANDLE)_beginthreadex (NULL, 0, thrAgent, (LPVOID)nAgent, 0, &thrId);
#endif // SOLARIS
   } // end_if status == SNMPAPI_ON
else
   { // status == SNMPAPI_OFF
   if (!pEntity->Agent)
      { // Entity not running as agent
      lError = SNMPAPI_NOOP;
      goto ERROR_PRECHECK;
      }
   // Entity is running as agent
   nAgent = pEntity->Agent - 1;
   pAgent = snmpGetTableEntry(&AgentDescr, nAgent);
   closesocket (pAgent->Socket);
   WaitForSingleObject (pAgent->Thread, INFINITE);
   CloseHandle (pAgent->Thread);
   snmpFreeTableEntry(&AgentDescr, nAgent);
   // Must terminate entity's agent status
   pEntity->Agent = 0;
   // Must terminate entity if nothing else was using it
   if (pEntity->refCount == 0)
      SnmpFreeEntity (hEntity);
   } // end_else status == SNMPAPI_OFF
ERROR_PRECHECK:
LeaveCriticalSection (&cs_AGENT);
LeaveCriticalSection (&cs_ENTITY);
ERROR_OUT:
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
else
   return (SaveError (lSession, lError));
} // end_SnmpListen()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpCancelMsg (HSNMP_SESSION hSession, smiINT32 nReqID)
{
DWORD nMsg = 0;
DWORD nFound = 0;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPSNMPMSG pMsg;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&SessDescr, HandleToUlong(hSession)-1))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
lSession = hSession;
EnterCriticalSection (&cs_MSG);

while (nFound < MsgDescr.Used && nMsg < MsgDescr.Allocated)
   {
   pMsg = snmpGetTableEntry(&MsgDescr, nMsg);
   // Deliberate assignement in next conditional
   if (pMsg->Session)
      {
      nFound++;
      if (pMsg->Session == hSession)
         {
         if (pMsg->Status == NP_SENT &&
             pMsg->appReqId == (smiUINT32)nReqID)
            {
            FreeMsg (nMsg);
            goto ERROR_PRECHECK;
            }
         }
      }
   nMsg++;
   }
// Falied to find a MSG that matched the request
lError = SNMPAPI_PDU_INVALID;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_MSG);
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
// else...failure case
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpCancelMsg
