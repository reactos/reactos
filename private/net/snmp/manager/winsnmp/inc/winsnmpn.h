#ifndef _INC_WINSNMPN
#define _INC_WINSNMPN
//
// winsnmpn.h
//
// Private include file for ACE*COMM's
// NetPlus Win32 WinSNMP implementation
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 970310 - Removed NPWSNMPSTATUSREPORT structure
//        - Added localAddress to TaskData
//        - Removed unused #defines
//
// 970417 - Added OSVERSIONINFO to TaskData
//
#include <winsock.h>
#include <wsipx.h>
//
#ifdef SOLARIS
#define THR_FLAGS THR_BOUND|THR_DETACHED
#define THR_TYPE LPVOID
#else // Win32
#define THR_TYPE DWORD
#endif // SOLARIS
//
#define DEFSESSIONS           10 // just for testing -- should be rolled back to 10
#define DEFVBLS               100 // just for testing -- should be rolled back to 100
#define DEFPDUS               100 // just for testing -- should be rolled back to 100
#define DEFENTITIES           100 // just for testing -- should be rolled back to 100
#define DEFCONTEXTS           100 // just for testing -- should be rolled back to 100
#define DEFMSGS               100 // just for testing -- should be rolled back to 100
#define DEFTRAPS              10 // just for testing -- should be rolled back to 10
#define DEFAGENTS             5 // just for testing -- should be rolled back to 5

#define MAXCENTISECONDS       429496729U
#define MAXMILLISECONDS       4294967295U
#define DEFTIMEOUT            300      // centisecs = 3 seconds
#define DEFRETRY              3
#define IP_SNMP_PORT          161
#define IP_TRAP_PORT          162
#define IPX_SNMP_PORT         36879
#define IPX_TRAP_PORT         36880
#define SNMPTRAPPIPE          "\\\\.\\PIPE\\MGMTAPI"
#define TRAPSERVERTIMEOUT     30000     // millisecs = 30 seconds
#define NP_WSX95              "NP_WSX95"
#define NP_WSX95_EXE          "NP_WSX95.EXE"
#define WSNMP_TRAPS_ON        WM_USER + 12
#define WSNMP_TRAPS_OFF       WM_USER + 13
#define LOOPBACK_ADDR         "127.0.0.1"
#define MAXTRAPIDS            20    // Max sub-ids in trap matches
#define MAX_FRIEND_NAME_LEN   31
#define MAX_CONTEXT_LEN       256
#define MAX_HOSTNAME          64
#define AF_INET_ADDR_SIZE     4
#define AF_IPX_ADDR_SIZE      10
#define NP_SEND               1 // "To send" msg status
#define NP_SENT               2 // "Sent" msg status
#define NP_RCVD               3 // "Received" msg status
#define NP_READY              4 // "Ready" for pickup by app
#define NP_EXPIRED            5 // "Timed out" for thrNotify
#define NP_REQUEST            1 // Agent request/response msg type
#define NP_RESPONSE           2 // Mgr request/response msg type
#define NP_TRAP               3 // Trap msg type
#define MAX_PENDING_WAIT      1000 // Trap service status period

typedef union
   {
   SOCKADDR_IN  inet;
   SOCKADDR_IPX ipx;
   } SAS, *LPSAS;

typedef struct _VB
   {
   smiOID      name;
   smiVALUE    value;
   smiINT32    data_length;
   struct _VB  *next_var;
   } VARBIND, *LPVARBIND;

typedef struct
   {
   smiOID         enterprise;
   smiIPADDR      agent_addr;
   smiINT32       generic_trap;
   smiINT32       specific_trap;
   smiTIMETICKS   time_ticks;
   } V1TRAP, *LPV1TRAP;

typedef struct _tdBuffer       // SNMP Buffer Descriptor
   {
   struct _tdBuffer *next;     // Link to the next buffer of the table
   struct _tdBuffer *prev;     // Link to the previous buffer of the table
   DWORD  Used;                // Number of entries in use in this buffer
   } SNMPBD, FAR *LPSNMPBD;

typedef struct                 // SNMP Table Descriptor
{
   DWORD    Used;              // Number currently ulilized
   DWORD    Allocated;         // Number currently allocated
   DWORD    BlocksToAdd;       // Incremented in chunks of this many records
   DWORD    BlockSize;         // Record size (bytes)
   LPSNMPBD HeadBuffer;        // circular list to the buffers of the table
                               // 'HeadBuffer' points to a SNMPBD structure suffixed with
                               // 'BlocksToAdd' blocks of size 'BlockSize' each
   } SNMPTD, FAR *LPSNMPTD;

typedef struct
   {
   HSNMP_SESSION  hTask;
   smiUINT32      nTranslateMode;
   smiUINT32      nRetransmitMode;
   DWORD          localAddress;
   SNMPAPI_STATUS conveyAddress; // SNMPAPI_ON/OFF
   HANDLE         timerThread;
   SOCKET         ipSock;
   HANDLE         ipThread;
   SOCKET         ipxSock;
   HANDLE         ipxThread;
   SOCKET         trapSock;   // Win95-only
   HANDLE         trapThread;
   HWND           trapWnd;    // Win95-only
   HANDLE         trapPipe;   // NT-only
   smiUINT32      nLastReqId;
   SNMPAPI_STATUS nLastError;
   OSVERSIONINFO  sEnv;       // Operating System
   } TASK, FAR *LPTASK;

typedef struct
   {
   HSNMP_SESSION    nTask;
   HWND             hWnd;
   DWORD            wMsg;
   SNMPAPI_CALLBACK fCallBack;
   LPVOID           lpClientData;
   HANDLE           thrEvent;
   HANDLE           thrHandle;
   DWORD            thrCount;
   SNMPAPI_STATUS   nLastError;
   } SESSION, FAR *LPSESSION;                      
typedef struct
   {
   HSNMP_SESSION  Session;
   smiINT32       type;
   smiINT32       appReqId;
   smiINT32       errStatus;
   smiINT32       errIndex;
   HSNMP_VBL      VBL;
   LPVARBIND      VBL_addr;
   LPV1TRAP       v1Trap;
   } PDUS, FAR *LPPDUS;
typedef struct
   {
   HSNMP_SESSION  Session;
   LPVARBIND      vbList;
   } VBLS, FAR *LPVBLS;
typedef struct
   {
   HSNMP_SESSION  Session;
   smiUINT32      refCount;
   smiUINT32      version;
   smiBYTE        name[MAX_FRIEND_NAME_LEN+1];  // friendly name
   smiTIMETICKS   nPolicyTimeout;               // centiseconds
   smiTIMETICKS   nActualTimeout;               // centiseconds
   smiUINT32      nPolicyRetry;
   smiUINT32      nActualRetry;
   smiUINT32      Agent;
   SAS            addr;
   } ENTITY, FAR *LPENTITY;
typedef struct
   {
   HSNMP_SESSION  Session;
   smiUINT32      refCount;
   smiUINT32      version;
   smiBYTE        name[MAX_FRIEND_NAME_LEN+1];  // friendly name
   smiUINT32      commLen;                      // len of commStr
   smiBYTE        commStr[MAX_CONTEXT_LEN];     // raw value
   } CTXT, FAR *LPCTXT;
typedef struct
   {
   HSNMP_SESSION  Session;
   DWORD          Status;        // NP_SEND|SENT|RCVD|READY
   DWORD          Type;          // PDU type
   HSNMP_ENTITY   agentEntity;
   HSNMP_ENTITY   ourEntity;
   HSNMP_CONTEXT  Context;
   smiUINT32      dllReqId;
   smiUINT32      appReqId;
   smiUINT32      nRetransmitMode;
   smiUINT32      Ticks;   // Msg sent time
   smiUINT32      Wait;     // Msg timeout interval (millisecs)
   smiUINT32      Tries;   // Msg retry count
   smiLPBYTE      Addr;
   smiUINT32      Size;
   SAS            Host;
   } SNMPMSG, FAR *LPSNMPMSG;
typedef struct
   {
   HSNMP_SESSION  Session;
   HSNMP_ENTITY   ourEntity;
   HSNMP_ENTITY   agentEntity;
   HSNMP_CONTEXT  Context;
   smiOID         notification;
   smiUINT32      notificationValue[MAXTRAPIDS];
   } TRAPNOTICE, FAR *LPTRAPNOTICE;
typedef struct
   {
   HSNMP_SESSION  Session;
   HSNMP_ENTITY   Entity;
   SOCKET         Socket;
   HANDLE         Thread;
   } AGENT, FAR *LPAGENT;

#endif // _INC_WINSNMPN
