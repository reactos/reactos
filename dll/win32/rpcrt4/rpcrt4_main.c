/*
 *  RPCRT4
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 * 
 * WINE RPC TODO's (and a few TODONT's)
 *
 * - Statistics: we are supposed to be keeping various counters.  we aren't.
 *
 * - Async RPC: Unimplemented.
 *
 * - The NT "ports" API, aka LPC.  Greg claims this is on his radar.  Might (or
 *   might not) enable users to get some kind of meaningful result out of
 *   NT-based native rpcrt4's.  Commonly-used transport for self-to-self RPC's.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "winuser.h"
#include "winnt.h"
#include "winternl.h"
#include "iptypes.h"
#include "iphlpapi.h"
#include "wine/unicode.h"
#include "rpc.h"

#include "ole2.h"
#include "rpcndr.h"
#include "rpcproxy.h"

#include "rpc_binding.h"
#include "rpcss_np_client.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

static UUID uuid_nil;
static HANDLE master_mutex;

HANDLE RPCRT4_GetMasterMutex(void)
{
    return master_mutex;
}

static CRITICAL_SECTION uuid_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &uuid_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": uuid_cs") }
};
static CRITICAL_SECTION uuid_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static CRITICAL_SECTION threaddata_cs;
static CRITICAL_SECTION_DEBUG threaddata_cs_debug =
{
    0, 0, &threaddata_cs,
    { &threaddata_cs_debug.ProcessLocksList, &threaddata_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": threaddata_cs") }
};
static CRITICAL_SECTION threaddata_cs = { &threaddata_cs_debug, -1, 0, 0, 0, 0 };

struct list threaddata_list = LIST_INIT(threaddata_list);

struct context_handle_list
{
    struct context_handle_list *next;
    NDR_SCONTEXT context_handle;
};

struct threaddata
{
    struct list entry;
    CRITICAL_SECTION cs;
    DWORD thread_id;
    RpcConnection *connection;
    RpcBinding *server_binding;
    struct context_handle_list *context_handle_list;
};

/***********************************************************************
 * DllMain
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    struct threaddata *tdata;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        master_mutex = CreateMutexA( NULL, FALSE, RPCSS_MASTER_MUTEX_NAME);
        if (!master_mutex)
          ERR("Failed to create master mutex\n");
        break;

    case DLL_THREAD_DETACH:
        tdata = NtCurrentTeb()->ReservedForNtRpc;
        if (tdata)
        {
            EnterCriticalSection(&threaddata_cs);
            list_remove(&tdata->entry);
            LeaveCriticalSection(&threaddata_cs);

            DeleteCriticalSection(&tdata->cs);
            if (tdata->connection)
                ERR("tdata->connection should be NULL but is still set to %p\n", tdata->connection);
            if (tdata->server_binding)
                ERR("tdata->server_binding should be NULL but is still set to %p\n", tdata->server_binding);
            HeapFree(GetProcessHeap(), 0, tdata);
        }
        break;

    case DLL_PROCESS_DETACH:
        CloseHandle(master_mutex);
        master_mutex = NULL;
        break;
    }

    return TRUE;
}

/*************************************************************************
 *           RpcStringFreeA   [RPCRT4.@]
 *
 * Frees a character string allocated by the RPC run-time library.
 *
 * RETURNS
 *
 *  S_OK if successful.
 */
RPC_STATUS WINAPI RpcStringFreeA(RPC_CSTR* String)
{
  HeapFree( GetProcessHeap(), 0, *String);

  return RPC_S_OK;
}

/*************************************************************************
 *           RpcStringFreeW   [RPCRT4.@]
 *
 * Frees a character string allocated by the RPC run-time library.
 *
 * RETURNS
 *
 *  S_OK if successful.
 */
RPC_STATUS WINAPI RpcStringFreeW(RPC_WSTR* String)
{
  HeapFree( GetProcessHeap(), 0, *String);

  return RPC_S_OK;
}

/*************************************************************************
 *           RpcRaiseException   [RPCRT4.@]
 *
 * Raises an exception.
 */
void DECLSPEC_NORETURN WINAPI RpcRaiseException(RPC_STATUS exception)
{
  /* shouldn't return */
  RaiseException(exception, 0, 0, NULL);
  ERR("handler continued execution\n");
  ExitProcess(1);
}

/*************************************************************************
 * UuidCompare [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid1        [I] Uuid to compare
 *     UUID *Uuid2        [I] Uuid to compare
 *     RPC_STATUS *Status [O] returns RPC_S_OK
 * 
 * RETURNS
 *    -1  if Uuid1 is less than Uuid2
 *     0  if Uuid1 and Uuid2 are equal
 *     1  if Uuid1 is greater than Uuid2
 */
int WINAPI UuidCompare(UUID *Uuid1, UUID *Uuid2, RPC_STATUS *Status)
{
  int i;

  TRACE("(%s,%s)\n", debugstr_guid(Uuid1), debugstr_guid(Uuid2));

  *Status = RPC_S_OK;

  if (!Uuid1) Uuid1 = &uuid_nil;
  if (!Uuid2) Uuid2 = &uuid_nil;

  if (Uuid1 == Uuid2) return 0;

  if (Uuid1->Data1 != Uuid2->Data1)
    return Uuid1->Data1 < Uuid2->Data1 ? -1 : 1;

  if (Uuid1->Data2 != Uuid2->Data2)
    return Uuid1->Data2 < Uuid2->Data2 ? -1 : 1;

  if (Uuid1->Data3 != Uuid2->Data3)
    return Uuid1->Data3 < Uuid2->Data3 ? -1 : 1;

  for (i = 0; i < 8; i++) {
    if (Uuid1->Data4[i] < Uuid2->Data4[i])
      return -1;
    if (Uuid1->Data4[i] > Uuid2->Data4[i])
      return 1;
  }

  return 0;
}

/*************************************************************************
 * UuidEqual [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid1        [I] Uuid to compare
 *     UUID *Uuid2        [I] Uuid to compare
 *     RPC_STATUS *Status [O] returns RPC_S_OK
 *
 * RETURNS
 *     TRUE/FALSE
 */
int WINAPI UuidEqual(UUID *Uuid1, UUID *Uuid2, RPC_STATUS *Status)
{
  TRACE("(%s,%s)\n", debugstr_guid(Uuid1), debugstr_guid(Uuid2));
  return !UuidCompare(Uuid1, Uuid2, Status);
}

/*************************************************************************
 * UuidIsNil [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid         [I] Uuid to compare
 *     RPC_STATUS *Status [O] retuns RPC_S_OK
 *
 * RETURNS
 *     TRUE/FALSE
 */
int WINAPI UuidIsNil(UUID *Uuid, RPC_STATUS *Status)
{
  TRACE("(%s)\n", debugstr_guid(Uuid));
  if (!Uuid) return TRUE;
  return !UuidCompare(Uuid, &uuid_nil, Status);
}

 /*************************************************************************
 * UuidCreateNil [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid [O] returns a nil UUID
 *
 * RETURNS
 *     RPC_S_OK
 */
RPC_STATUS WINAPI UuidCreateNil(UUID *Uuid)
{
  *Uuid = uuid_nil;
  return RPC_S_OK;
}

/* Number of 100ns ticks per clock tick. To be safe, assume that the clock
   resolution is at least 1000 * 100 * (1/1000000) = 1/10 of a second */
#define TICKS_PER_CLOCK_TICK 1000
#define SECSPERDAY  86400
#define TICKSPERSEC 10000000
/* UUID system time starts at October 15, 1582 */
#define SECS_15_OCT_1582_TO_1601  ((17 + 30 + 31 + 365 * 18 + 5) * SECSPERDAY)
#define TICKS_15_OCT_1582_TO_1601 ((ULONGLONG)SECS_15_OCT_1582_TO_1601 * TICKSPERSEC)

static void RPC_UuidGetSystemTime(ULONGLONG *time)
{
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);

    *time = ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    *time += TICKS_15_OCT_1582_TO_1601;
}

/* Assume that a hardware address is at least 6 bytes long */ 
#define ADDRESS_BYTES_NEEDED 6

static RPC_STATUS RPC_UuidGetNodeAddress(BYTE *address)
{
    int i;
    DWORD status = RPC_S_OK;

    ULONG buflen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO adapter = HeapAlloc(GetProcessHeap(), 0, buflen);

    if (GetAdaptersInfo(adapter, &buflen) == ERROR_BUFFER_OVERFLOW) {
        HeapFree(GetProcessHeap(), 0, adapter);
        adapter = HeapAlloc(GetProcessHeap(), 0, buflen);
    }

    if (GetAdaptersInfo(adapter, &buflen) == NO_ERROR) {
        for (i = 0; i < ADDRESS_BYTES_NEEDED; i++) {
            address[i] = adapter->Address[i];
        }
    }
    /* We can't get a hardware address, just use random numbers.
       Set the multicast bit to prevent conflicts with real cards. */
    else {
        for (i = 0; i < ADDRESS_BYTES_NEEDED; i++) {
            address[i] = rand() & 0xff;
        }

        address[0] |= 0x01;
        status = RPC_S_UUID_LOCAL_ONLY;
    }

    HeapFree(GetProcessHeap(), 0, adapter);
    return status;
}

/*************************************************************************
 *           UuidCreate   [RPCRT4.@]
 *
 * Creates a 128bit UUID.
 *
 * RETURNS
 *
 *  RPC_S_OK if successful.
 *  RPC_S_UUID_LOCAL_ONLY if UUID is only locally unique.
 *
 *  FIXME: No compensation for changes across reloading
 *         this dll or across reboots (e.g. clock going 
 *         backwards and swapped network cards). The RFC
 *         suggests using NVRAM for storing persistent 
 *         values.
 */
RPC_STATUS WINAPI UuidCreate(UUID *Uuid)
{
    static int initialised, count;

    ULONGLONG time;
    static ULONGLONG timelast;
    static WORD sequence;

    static DWORD status;
    static BYTE address[MAX_ADAPTER_ADDRESS_LENGTH];

    EnterCriticalSection(&uuid_cs);

    if (!initialised) {
        RPC_UuidGetSystemTime(&timelast);
        count = TICKS_PER_CLOCK_TICK;

        sequence = ((rand() & 0xff) << 8) + (rand() & 0xff);
        sequence &= 0x1fff;

        status = RPC_UuidGetNodeAddress(address);
        initialised = 1;
    }

    /* Generate time element of the UUID. Account for going faster
       than our clock as well as the clock going backwards. */
    while (1) {
        RPC_UuidGetSystemTime(&time);
        if (time > timelast) {
            count = 0;
            break;
        }
        if (time < timelast) {
            sequence = (sequence + 1) & 0x1fff;
            count = 0;
            break;
        }
        if (count < TICKS_PER_CLOCK_TICK) {
            count++;
            break;
        }
    }

    timelast = time;
    time += count;

    /* Pack the information into the UUID structure. */

    Uuid->Data1  = (unsigned long)(time & 0xffffffff);
    Uuid->Data2  = (unsigned short)((time >> 32) & 0xffff);
    Uuid->Data3  = (unsigned short)((time >> 48) & 0x0fff);

    /* This is a version 1 UUID */
    Uuid->Data3 |= (1 << 12);

    Uuid->Data4[0]  = sequence & 0xff;
    Uuid->Data4[1]  = (sequence & 0x3f00) >> 8;
    Uuid->Data4[1] |= 0x80;

    Uuid->Data4[2] = address[0];
    Uuid->Data4[3] = address[1];
    Uuid->Data4[4] = address[2];
    Uuid->Data4[5] = address[3];
    Uuid->Data4[6] = address[4];
    Uuid->Data4[7] = address[5];

    LeaveCriticalSection(&uuid_cs);

    TRACE("%s\n", debugstr_guid(Uuid));

    return status;
}

/*************************************************************************
 *           UuidCreateSequential   [RPCRT4.@]
 *
 * Creates a 128bit UUID.
 *
 * RETURNS
 *
 *  RPC_S_OK if successful.
 *  RPC_S_UUID_LOCAL_ONLY if UUID is only locally unique.
 *
 */
RPC_STATUS WINAPI UuidCreateSequential(UUID *Uuid)
{
   return UuidCreate(Uuid);
}


/*************************************************************************
 *           UuidHash   [RPCRT4.@]
 *
 * Generates a hash value for a given UUID
 *
 * Code based on FreeDCE implementation
 *
 */
unsigned short WINAPI UuidHash(UUID *uuid, RPC_STATUS *Status)
{
  BYTE *data = (BYTE*)uuid;
  short c0 = 0, c1 = 0, x, y;
  unsigned int i;

  if (!uuid) data = (BYTE*)(uuid = &uuid_nil);

  TRACE("(%s)\n", debugstr_guid(uuid));

  for (i=0; i<sizeof(UUID); i++) {
    c0 += data[i];
    c1 += c0;
  }

  x = -c1 % 255;
  if (x < 0) x += 255;

  y = (c1 - c0) % 255;
  if (y < 0) y += 255;

  *Status = RPC_S_OK;
  return y*256 + x;
}

/*************************************************************************
 *           UuidToStringA   [RPCRT4.@]
 *
 * Converts a UUID to a string.
 *
 * UUID format is 8 hex digits, followed by a hyphen then three groups of
 * 4 hex digits each followed by a hyphen and then 12 hex digits
 *
 * RETURNS
 *
 *  S_OK if successful.
 *  S_OUT_OF_MEMORY if unsuccessful.
 */
RPC_STATUS WINAPI UuidToStringA(UUID *Uuid, RPC_CSTR* StringUuid)
{
  *StringUuid = HeapAlloc( GetProcessHeap(), 0, sizeof(char) * 37);

  if(!(*StringUuid))
    return RPC_S_OUT_OF_MEMORY;

  if (!Uuid) Uuid = &uuid_nil;

  sprintf( (char*)*StringUuid, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 Uuid->Data1, Uuid->Data2, Uuid->Data3,
                 Uuid->Data4[0], Uuid->Data4[1], Uuid->Data4[2],
                 Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                 Uuid->Data4[6], Uuid->Data4[7] );

  return RPC_S_OK;
}

/*************************************************************************
 *           UuidToStringW   [RPCRT4.@]
 *
 * Converts a UUID to a string.
 *
 *  S_OK if successful.
 *  S_OUT_OF_MEMORY if unsuccessful.
 */
RPC_STATUS WINAPI UuidToStringW(UUID *Uuid, RPC_WSTR* StringUuid)
{
  char buf[37];

  if (!Uuid) Uuid = &uuid_nil;

  sprintf(buf, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
               Uuid->Data1, Uuid->Data2, Uuid->Data3,
               Uuid->Data4[0], Uuid->Data4[1], Uuid->Data4[2],
               Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
               Uuid->Data4[6], Uuid->Data4[7] );

  *StringUuid = RPCRT4_strdupAtoW(buf);

  if(!(*StringUuid))
    return RPC_S_OUT_OF_MEMORY;

  return RPC_S_OK;
}

static const BYTE hex2bin[] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x00 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x10 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x20 */
    0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,        /* 0x30 */
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,  /* 0x40 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x50 */
    0,10,11,12,13,14,15                     /* 0x60 */
};

/***********************************************************************
 *		UuidFromStringA (RPCRT4.@)
 */
RPC_STATUS WINAPI UuidFromStringA(RPC_CSTR s, UUID *uuid)
{
    int i;

    if (!s) return UuidCreateNil( uuid );

    if (strlen((char*)s) != 36) return RPC_S_INVALID_STRING_UUID;

    if ((s[8]!='-') || (s[13]!='-') || (s[18]!='-') || (s[23]!='-'))
        return RPC_S_INVALID_STRING_UUID;

    for (i=0; i<36; i++)
    {
        if ((i == 8)||(i == 13)||(i == 18)||(i == 23)) continue;
        if (s[i] > 'f' || (!hex2bin[s[i]] && s[i] != '0')) return RPC_S_INVALID_STRING_UUID;
    }

    /* in form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX */

    uuid->Data1 = (hex2bin[s[0]] << 28 | hex2bin[s[1]] << 24 | hex2bin[s[2]] << 20 | hex2bin[s[3]] << 16 |
                   hex2bin[s[4]] << 12 | hex2bin[s[5]]  << 8 | hex2bin[s[6]]  << 4 | hex2bin[s[7]]);
    uuid->Data2 =  hex2bin[s[9]] << 12 | hex2bin[s[10]] << 8 | hex2bin[s[11]] << 4 | hex2bin[s[12]];
    uuid->Data3 = hex2bin[s[14]] << 12 | hex2bin[s[15]] << 8 | hex2bin[s[16]] << 4 | hex2bin[s[17]];

    /* these are just sequential bytes */
    uuid->Data4[0] = hex2bin[s[19]] << 4 | hex2bin[s[20]];
    uuid->Data4[1] = hex2bin[s[21]] << 4 | hex2bin[s[22]];
    uuid->Data4[2] = hex2bin[s[24]] << 4 | hex2bin[s[25]];
    uuid->Data4[3] = hex2bin[s[26]] << 4 | hex2bin[s[27]];
    uuid->Data4[4] = hex2bin[s[28]] << 4 | hex2bin[s[29]];
    uuid->Data4[5] = hex2bin[s[30]] << 4 | hex2bin[s[31]];
    uuid->Data4[6] = hex2bin[s[32]] << 4 | hex2bin[s[33]];
    uuid->Data4[7] = hex2bin[s[34]] << 4 | hex2bin[s[35]];
    return RPC_S_OK;
}


/***********************************************************************
 *		UuidFromStringW (RPCRT4.@)
 */
RPC_STATUS WINAPI UuidFromStringW(RPC_WSTR s, UUID *uuid)
{
    int i;

    if (!s) return UuidCreateNil( uuid );

    if (strlenW(s) != 36) return RPC_S_INVALID_STRING_UUID;

    if ((s[8]!='-') || (s[13]!='-') || (s[18]!='-') || (s[23]!='-'))
        return RPC_S_INVALID_STRING_UUID;

    for (i=0; i<36; i++)
    {
        if ((i == 8)||(i == 13)||(i == 18)||(i == 23)) continue;
        if (s[i] > 'f' || (!hex2bin[s[i]] && s[i] != '0')) return RPC_S_INVALID_STRING_UUID;
    }

    /* in form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX */

    uuid->Data1 = (hex2bin[s[0]] << 28 | hex2bin[s[1]] << 24 | hex2bin[s[2]] << 20 | hex2bin[s[3]] << 16 |
                   hex2bin[s[4]] << 12 | hex2bin[s[5]]  << 8 | hex2bin[s[6]]  << 4 | hex2bin[s[7]]);
    uuid->Data2 =  hex2bin[s[9]] << 12 | hex2bin[s[10]] << 8 | hex2bin[s[11]] << 4 | hex2bin[s[12]];
    uuid->Data3 = hex2bin[s[14]] << 12 | hex2bin[s[15]] << 8 | hex2bin[s[16]] << 4 | hex2bin[s[17]];

    /* these are just sequential bytes */
    uuid->Data4[0] = hex2bin[s[19]] << 4 | hex2bin[s[20]];
    uuid->Data4[1] = hex2bin[s[21]] << 4 | hex2bin[s[22]];
    uuid->Data4[2] = hex2bin[s[24]] << 4 | hex2bin[s[25]];
    uuid->Data4[3] = hex2bin[s[26]] << 4 | hex2bin[s[27]];
    uuid->Data4[4] = hex2bin[s[28]] << 4 | hex2bin[s[29]];
    uuid->Data4[5] = hex2bin[s[30]] << 4 | hex2bin[s[31]];
    uuid->Data4[6] = hex2bin[s[32]] << 4 | hex2bin[s[33]];
    uuid->Data4[7] = hex2bin[s[34]] << 4 | hex2bin[s[35]];
    return RPC_S_OK;
}

/***********************************************************************
 *              DllRegisterServer (RPCRT4.@)
 */

HRESULT WINAPI DllRegisterServer( void )
{
    FIXME( "(): stub\n" );
    return S_OK;
}

static BOOL RPCRT4_StartRPCSS(void)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    static char cmd[6];
    BOOL rslt;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);

    /* apparently it's not OK to use a constant string below */
    CopyMemory(cmd, "rpcss", 6);

    /* FIXME: will this do the right thing when run as a test? */
    rslt = CreateProcessA(
        NULL,           /* executable */
        cmd,            /* command line */
        NULL,           /* process security attributes */
        NULL,           /* primary thread security attributes */
        FALSE,          /* inherit handles */
        0,              /* creation flags */
        NULL,           /* use parent's environment */
        NULL,           /* use parent's current directory */
        &si,            /* STARTUPINFO pointer */
        &pi             /* PROCESS_INFORMATION */
    );

    if (rslt) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }

    return rslt;
}

/***********************************************************************
 *           RPCRT4_RPCSSOnDemandCall (internal)
 * 
 * Attempts to send a message to the RPCSS process
 * on the local machine, invoking it if necessary.
 * For remote RPCSS calls, use.... your imagination.
 * 
 * PARAMS
 *     msg             [I] pointer to the RPCSS message
 *     vardata_payload [I] pointer vardata portion of the RPCSS message
 *     reply           [O] pointer to reply structure
 *
 * RETURNS
 *     TRUE if successful
 *     FALSE otherwise
 */
BOOL RPCRT4_RPCSSOnDemandCall(PRPCSS_NP_MESSAGE msg, char *vardata_payload, PRPCSS_NP_REPLY reply)
{
    HANDLE client_handle;
    BOOL ret;
    int i, j = 0;

    TRACE("(msg == %p, vardata_payload == %p, reply == %p)\n", msg, vardata_payload, reply);

    client_handle = RPCRT4_RpcssNPConnect();

    while (INVALID_HANDLE_VALUE == client_handle) {
        /* start the RPCSS process */
	if (!RPCRT4_StartRPCSS()) {
	    ERR("Unable to start RPCSS process.\n");
	    return FALSE;
	}
	/* wait for a connection (w/ periodic polling) */
        for (i = 0; i < 60; i++) {
            Sleep(200);
            client_handle = RPCRT4_RpcssNPConnect();
            if (INVALID_HANDLE_VALUE != client_handle) break;
        } 
        /* we are only willing to try twice */
	if (j++ >= 1) break;
    }

    if (INVALID_HANDLE_VALUE == client_handle) {
        /* no dice! */
        ERR("Unable to connect to RPCSS process!\n");
	SetLastError(RPC_E_SERVER_DIED_DNE);
	return FALSE;
    }

    /* great, we're connected.  now send the message */
    ret = TRUE;
    if (!RPCRT4_SendReceiveNPMsg(client_handle, msg, vardata_payload, reply)) {
        ERR("Something is amiss: RPC_SendReceive failed.\n");
	ret = FALSE;
    }
    CloseHandle(client_handle);

    return ret;
}

#define MAX_RPC_ERROR_TEXT 256

/******************************************************************************
 * DceErrorInqTextW   (rpcrt4.@)
 *
 * Notes
 * 1. On passing a NULL pointer the code does bomb out.
 * 2. The size of the required buffer is not defined in the documentation.
 *    It appears to be 256.
 * 3. The function is defined to return RPC_S_INVALID_ARG but I don't know
 *    of any value for which it does.
 * 4. The MSDN documentation currently declares that the second argument is
 *    unsigned char *, even for the W version.  I don't believe it.
 */
RPC_STATUS RPC_ENTRY DceErrorInqTextW (RPC_STATUS e, RPC_WSTR buffer)
{
    DWORD count;
    count = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, e, 0, buffer, MAX_RPC_ERROR_TEXT, NULL);
    if (!count)
    {
        count = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, RPC_S_NOT_RPC_ERROR, 0, buffer, MAX_RPC_ERROR_TEXT, NULL);
        if (!count)
        {
            ERR ("Failed to translate error\n");
            return RPC_S_INVALID_ARG;
        }
    }
    return RPC_S_OK;
}

/******************************************************************************
 * DceErrorInqTextA   (rpcrt4.@)
 */
RPC_STATUS RPC_ENTRY DceErrorInqTextA (RPC_STATUS e, RPC_CSTR buffer)
{
    RPC_STATUS status;
    WCHAR bufferW [MAX_RPC_ERROR_TEXT];
    if ((status = DceErrorInqTextW (e, bufferW)) == RPC_S_OK)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, bufferW, -1, (LPSTR)buffer, MAX_RPC_ERROR_TEXT,
                NULL, NULL))
        {
            ERR ("Failed to translate error\n");
            status = RPC_S_INVALID_ARG;
        }
    }
    return status;
}

/******************************************************************************
 * I_RpcAllocate   (rpcrt4.@)
 */
void * WINAPI I_RpcAllocate(unsigned int Size)
{
    return HeapAlloc(GetProcessHeap(), 0, Size);
}

/******************************************************************************
 * I_RpcFree   (rpcrt4.@)
 */
void WINAPI I_RpcFree(void *Object)
{
    HeapFree(GetProcessHeap(), 0, Object);
}

/******************************************************************************
 * I_RpcMapWin32Status   (rpcrt4.@)
 *
 * Maps Win32 RPC error codes to NT statuses.
 *
 * PARAMS
 *  status [I] Win32 RPC error code.
 *
 * RETURNS
 *  Appropriate translation into an NT status code.
 */
LONG WINAPI I_RpcMapWin32Status(RPC_STATUS status)
{
    TRACE("(%ld)\n", status);
    switch (status)
    {
    case ERROR_ACCESS_DENIED: return STATUS_ACCESS_DENIED;
    case ERROR_INVALID_HANDLE: return RPC_NT_SS_CONTEXT_MISMATCH;
    case ERROR_OUTOFMEMORY: return STATUS_NO_MEMORY;
    case ERROR_INVALID_PARAMETER: return STATUS_INVALID_PARAMETER;
    case ERROR_INSUFFICIENT_BUFFER: return STATUS_BUFFER_TOO_SMALL;
    case ERROR_MAX_THRDS_REACHED: return STATUS_NO_MEMORY;
    case ERROR_NOACCESS: return STATUS_ACCESS_VIOLATION;
    case ERROR_NOT_ENOUGH_SERVER_MEMORY: return STATUS_INSUFF_SERVER_RESOURCES;
    case ERROR_WRONG_PASSWORD: return STATUS_WRONG_PASSWORD;
    case ERROR_INVALID_LOGON_HOURS: return STATUS_INVALID_LOGON_HOURS;
    case ERROR_PASSWORD_EXPIRED: return STATUS_PASSWORD_EXPIRED;
    case ERROR_ACCOUNT_DISABLED: return STATUS_ACCOUNT_DISABLED;
    case ERROR_INVALID_SECURITY_DESCR: return STATUS_INVALID_SECURITY_DESCR;
    case RPC_S_INVALID_STRING_BINDING: return RPC_NT_INVALID_STRING_BINDING;
    case RPC_S_WRONG_KIND_OF_BINDING: return RPC_NT_WRONG_KIND_OF_BINDING;
    case RPC_S_INVALID_BINDING: return RPC_NT_INVALID_BINDING;
    case RPC_S_PROTSEQ_NOT_SUPPORTED: return RPC_NT_PROTSEQ_NOT_SUPPORTED;
    case RPC_S_INVALID_RPC_PROTSEQ: return RPC_NT_INVALID_RPC_PROTSEQ;
    case RPC_S_INVALID_STRING_UUID: return RPC_NT_INVALID_STRING_UUID;
    case RPC_S_INVALID_ENDPOINT_FORMAT: return RPC_NT_INVALID_ENDPOINT_FORMAT;
    case RPC_S_INVALID_NET_ADDR: return RPC_NT_INVALID_NET_ADDR;
    case RPC_S_NO_ENDPOINT_FOUND: return RPC_NT_NO_ENDPOINT_FOUND;
    case RPC_S_INVALID_TIMEOUT: return RPC_NT_INVALID_TIMEOUT;
    case RPC_S_OBJECT_NOT_FOUND: return RPC_NT_OBJECT_NOT_FOUND;
    case RPC_S_ALREADY_REGISTERED: return RPC_NT_ALREADY_REGISTERED;
    case RPC_S_TYPE_ALREADY_REGISTERED: return RPC_NT_TYPE_ALREADY_REGISTERED;
    case RPC_S_ALREADY_LISTENING: return RPC_NT_ALREADY_LISTENING;
    case RPC_S_NO_PROTSEQS_REGISTERED: return RPC_NT_NO_PROTSEQS_REGISTERED;
    case RPC_S_NOT_LISTENING: return RPC_NT_NOT_LISTENING;
    case RPC_S_UNKNOWN_MGR_TYPE: return RPC_NT_UNKNOWN_MGR_TYPE;
    case RPC_S_UNKNOWN_IF: return RPC_NT_UNKNOWN_IF;
    case RPC_S_NO_BINDINGS: return RPC_NT_NO_BINDINGS;
    case RPC_S_NO_PROTSEQS: return RPC_NT_NO_PROTSEQS;
    case RPC_S_CANT_CREATE_ENDPOINT: return RPC_NT_CANT_CREATE_ENDPOINT;
    case RPC_S_OUT_OF_RESOURCES: return RPC_NT_OUT_OF_RESOURCES;
    case RPC_S_SERVER_UNAVAILABLE: return RPC_NT_SERVER_UNAVAILABLE;
    case RPC_S_SERVER_TOO_BUSY: return RPC_NT_SERVER_TOO_BUSY;
    case RPC_S_INVALID_NETWORK_OPTIONS: return RPC_NT_INVALID_NETWORK_OPTIONS;
    case RPC_S_NO_CALL_ACTIVE: return RPC_NT_NO_CALL_ACTIVE;
    case RPC_S_CALL_FAILED: return RPC_NT_CALL_FAILED;
    case RPC_S_CALL_FAILED_DNE: return RPC_NT_CALL_FAILED_DNE;
    case RPC_S_PROTOCOL_ERROR: return RPC_NT_PROTOCOL_ERROR;
    case RPC_S_UNSUPPORTED_TRANS_SYN: return RPC_NT_UNSUPPORTED_TRANS_SYN;
    case RPC_S_UNSUPPORTED_TYPE: return RPC_NT_UNSUPPORTED_TYPE;
    case RPC_S_INVALID_TAG: return RPC_NT_INVALID_TAG;
    case RPC_S_INVALID_BOUND: return RPC_NT_INVALID_BOUND;
    case RPC_S_NO_ENTRY_NAME: return RPC_NT_NO_ENTRY_NAME;
    case RPC_S_INVALID_NAME_SYNTAX: return RPC_NT_INVALID_NAME_SYNTAX;
    case RPC_S_UNSUPPORTED_NAME_SYNTAX: return RPC_NT_UNSUPPORTED_NAME_SYNTAX;
    case RPC_S_UUID_NO_ADDRESS: return RPC_NT_UUID_NO_ADDRESS;
    case RPC_S_DUPLICATE_ENDPOINT: return RPC_NT_DUPLICATE_ENDPOINT;
    case RPC_S_UNKNOWN_AUTHN_TYPE: return RPC_NT_UNKNOWN_AUTHN_TYPE;
    case RPC_S_MAX_CALLS_TOO_SMALL: return RPC_NT_MAX_CALLS_TOO_SMALL;
    case RPC_S_STRING_TOO_LONG: return RPC_NT_STRING_TOO_LONG;
    case RPC_S_PROTSEQ_NOT_FOUND: return RPC_NT_PROTSEQ_NOT_FOUND;
    case RPC_S_PROCNUM_OUT_OF_RANGE: return RPC_NT_PROCNUM_OUT_OF_RANGE;
    case RPC_S_BINDING_HAS_NO_AUTH: return RPC_NT_BINDING_HAS_NO_AUTH;
    case RPC_S_UNKNOWN_AUTHN_SERVICE: return RPC_NT_UNKNOWN_AUTHN_SERVICE;
    case RPC_S_UNKNOWN_AUTHN_LEVEL: return RPC_NT_UNKNOWN_AUTHN_LEVEL;
    case RPC_S_INVALID_AUTH_IDENTITY: return RPC_NT_INVALID_AUTH_IDENTITY;
    case RPC_S_UNKNOWN_AUTHZ_SERVICE: return RPC_NT_UNKNOWN_AUTHZ_SERVICE;
    case EPT_S_INVALID_ENTRY: return EPT_NT_INVALID_ENTRY;
    case EPT_S_CANT_PERFORM_OP: return EPT_NT_CANT_PERFORM_OP;
    case EPT_S_NOT_REGISTERED: return EPT_NT_NOT_REGISTERED;
    case EPT_S_CANT_CREATE: return EPT_NT_CANT_CREATE;
    case RPC_S_NOTHING_TO_EXPORT: return RPC_NT_NOTHING_TO_EXPORT;
    case RPC_S_INCOMPLETE_NAME: return RPC_NT_INCOMPLETE_NAME;
    case RPC_S_INVALID_VERS_OPTION: return RPC_NT_INVALID_VERS_OPTION;
    case RPC_S_NO_MORE_MEMBERS: return RPC_NT_NO_MORE_MEMBERS;
    case RPC_S_NOT_ALL_OBJS_UNEXPORTED: return RPC_NT_NOT_ALL_OBJS_UNEXPORTED;
    case RPC_S_INTERFACE_NOT_FOUND: return RPC_NT_INTERFACE_NOT_FOUND;
    case RPC_S_ENTRY_ALREADY_EXISTS: return RPC_NT_ENTRY_ALREADY_EXISTS;
    case RPC_S_ENTRY_NOT_FOUND: return RPC_NT_ENTRY_NOT_FOUND;
    case RPC_S_NAME_SERVICE_UNAVAILABLE: return RPC_NT_NAME_SERVICE_UNAVAILABLE;
    case RPC_S_INVALID_NAF_ID: return RPC_NT_INVALID_NAF_ID;
    case RPC_S_CANNOT_SUPPORT: return RPC_NT_CANNOT_SUPPORT;
    case RPC_S_NO_CONTEXT_AVAILABLE: return RPC_NT_NO_CONTEXT_AVAILABLE;
    case RPC_S_INTERNAL_ERROR: return RPC_NT_INTERNAL_ERROR;
    case RPC_S_ZERO_DIVIDE: return RPC_NT_ZERO_DIVIDE;
    case RPC_S_ADDRESS_ERROR: return RPC_NT_ADDRESS_ERROR;
    case RPC_S_FP_DIV_ZERO: return RPC_NT_FP_DIV_ZERO;
    case RPC_S_FP_UNDERFLOW: return RPC_NT_FP_UNDERFLOW;
    case RPC_S_FP_OVERFLOW: return RPC_NT_FP_OVERFLOW;
    case RPC_S_CALL_IN_PROGRESS: return RPC_NT_CALL_IN_PROGRESS;
    case RPC_S_NO_MORE_BINDINGS: return RPC_NT_NO_MORE_BINDINGS;
    case RPC_S_CALL_CANCELLED: return RPC_NT_CALL_CANCELLED;
    case RPC_S_INVALID_OBJECT: return RPC_NT_INVALID_OBJECT;
    case RPC_S_INVALID_ASYNC_HANDLE: return RPC_NT_INVALID_ASYNC_HANDLE;
    case RPC_S_INVALID_ASYNC_CALL: return RPC_NT_INVALID_ASYNC_CALL;
    case RPC_S_GROUP_MEMBER_NOT_FOUND: return RPC_NT_GROUP_MEMBER_NOT_FOUND;
    case RPC_X_NO_MORE_ENTRIES: return RPC_NT_NO_MORE_ENTRIES;
    case RPC_X_SS_CHAR_TRANS_OPEN_FAIL: return RPC_NT_SS_CHAR_TRANS_OPEN_FAIL;
    case RPC_X_SS_CHAR_TRANS_SHORT_FILE: return RPC_NT_SS_CHAR_TRANS_SHORT_FILE;
    case RPC_X_SS_IN_NULL_CONTEXT: return RPC_NT_SS_IN_NULL_CONTEXT;
    case RPC_X_SS_CONTEXT_DAMAGED: return RPC_NT_SS_CONTEXT_DAMAGED;
    case RPC_X_SS_HANDLES_MISMATCH: return RPC_NT_SS_HANDLES_MISMATCH;
    case RPC_X_SS_CANNOT_GET_CALL_HANDLE: return RPC_NT_SS_CANNOT_GET_CALL_HANDLE;
    case RPC_X_NULL_REF_POINTER: return RPC_NT_NULL_REF_POINTER;
    case RPC_X_ENUM_VALUE_OUT_OF_RANGE: return RPC_NT_ENUM_VALUE_OUT_OF_RANGE;
    case RPC_X_BYTE_COUNT_TOO_SMALL: return RPC_NT_BYTE_COUNT_TOO_SMALL;
    case RPC_X_BAD_STUB_DATA: return RPC_NT_BAD_STUB_DATA;
    case RPC_X_PIPE_CLOSED: return RPC_NT_PIPE_CLOSED;
    case RPC_X_PIPE_DISCIPLINE_ERROR: return RPC_NT_PIPE_DISCIPLINE_ERROR;
    case RPC_X_PIPE_EMPTY: return RPC_NT_PIPE_EMPTY;
    case ERROR_PASSWORD_MUST_CHANGE: return STATUS_PASSWORD_MUST_CHANGE;
    case ERROR_ACCOUNT_LOCKED_OUT: return STATUS_ACCOUNT_LOCKED_OUT;
    default: return status;
    }
}

/******************************************************************************
 * I_RpcExceptionFilter   (rpcrt4.@)
 */
int WINAPI I_RpcExceptionFilter(ULONG ExceptionCode)
{
    TRACE("0x%x\n", ExceptionCode);
    switch (ExceptionCode)
    {
    case STATUS_DATATYPE_MISALIGNMENT:
    case STATUS_BREAKPOINT:
    case STATUS_ACCESS_VIOLATION:
    case STATUS_ILLEGAL_INSTRUCTION:
    case STATUS_PRIVILEGED_INSTRUCTION:
    case STATUS_INSTRUCTION_MISALIGNMENT:
    case STATUS_STACK_OVERFLOW:
    case STATUS_POSSIBLE_DEADLOCK:
        return EXCEPTION_CONTINUE_SEARCH;
    default:
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

/******************************************************************************
 * RpcErrorStartEnumeration   (rpcrt4.@)
 */
RPC_STATUS RPC_ENTRY RpcErrorStartEnumeration(RPC_ERROR_ENUM_HANDLE* EnumHandle)
{
    FIXME("(%p): stub\n", EnumHandle);
    return RPC_S_ENTRY_NOT_FOUND;
}

/******************************************************************************
 * RpcMgmtSetCancelTimeout   (rpcrt4.@)
 */
RPC_STATUS RPC_ENTRY RpcMgmtSetCancelTimeout(LONG Timeout)
{
    FIXME("(%d): stub\n", Timeout);
    return RPC_S_OK;
}

static struct threaddata *get_or_create_threaddata(void)
{
    struct threaddata *tdata = NtCurrentTeb()->ReservedForNtRpc;
    if (!tdata)
    {
        tdata = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*tdata));
        if (!tdata) return NULL;

        InitializeCriticalSection(&tdata->cs);
        tdata->thread_id = GetCurrentThreadId();

        EnterCriticalSection(&threaddata_cs);
        list_add_tail(&threaddata_list, &tdata->entry);
        LeaveCriticalSection(&threaddata_cs);

        NtCurrentTeb()->ReservedForNtRpc = tdata;
        return tdata;
    }
    return tdata;
}

void RPCRT4_SetThreadCurrentConnection(RpcConnection *Connection)
{
    struct threaddata *tdata = get_or_create_threaddata();
    if (!tdata) return;

    EnterCriticalSection(&tdata->cs);
    tdata->connection = Connection;
    LeaveCriticalSection(&tdata->cs);
}

void RPCRT4_SetThreadCurrentCallHandle(RpcBinding *Binding)
{
    struct threaddata *tdata = get_or_create_threaddata();
    if (!tdata) return;

    tdata->server_binding = Binding;
}

RpcBinding *RPCRT4_GetThreadCurrentCallHandle(void)
{
    struct threaddata *tdata = get_or_create_threaddata();
    if (!tdata) return NULL;

    return tdata->server_binding;
}

void RPCRT4_PushThreadContextHandle(NDR_SCONTEXT SContext)
{
    struct threaddata *tdata = get_or_create_threaddata();
    struct context_handle_list *context_handle_list;

    if (!tdata) return;

    context_handle_list = HeapAlloc(GetProcessHeap(), 0, sizeof(*context_handle_list));
    if (!context_handle_list) return;

    context_handle_list->context_handle = SContext;
    context_handle_list->next = tdata->context_handle_list;
    tdata->context_handle_list = context_handle_list;
}

void RPCRT4_RemoveThreadContextHandle(NDR_SCONTEXT SContext)
{
    struct threaddata *tdata = get_or_create_threaddata();
    struct context_handle_list *current, *prev;

    if (!tdata) return;

    for (current = tdata->context_handle_list, prev = NULL; current; prev = current, current = current->next)
    {
        if (current->context_handle == SContext)
        {
            if (prev)
                prev->next = current->next;
            else
                tdata->context_handle_list = current->next;
            HeapFree(GetProcessHeap(), 0, current);
            return;
        }
    }
}

NDR_SCONTEXT RPCRT4_PopThreadContextHandle(void)
{
    struct threaddata *tdata = get_or_create_threaddata();
    struct context_handle_list *context_handle_list;
    NDR_SCONTEXT context_handle;

    if (!tdata) return NULL;

    context_handle_list = tdata->context_handle_list;
    if (!context_handle_list) return NULL;
    tdata->context_handle_list = context_handle_list->next;

    context_handle = context_handle_list->context_handle;
    HeapFree(GetProcessHeap(), 0, context_handle_list);
    return context_handle;
}

/******************************************************************************
 * RpcCancelThread   (rpcrt4.@)
 */
RPC_STATUS RPC_ENTRY RpcCancelThread(void* ThreadHandle)
{
    DWORD target_tid;
    struct threaddata *tdata;

    TRACE("(%p)\n", ThreadHandle);

    target_tid = GetThreadId(ThreadHandle);
    if (!target_tid)
        return RPC_S_INVALID_ARG;

    EnterCriticalSection(&threaddata_cs);
    LIST_FOR_EACH_ENTRY(tdata, &threaddata_list, struct threaddata, entry)
        if (tdata->thread_id == target_tid)
        {
            EnterCriticalSection(&tdata->cs);
            if (tdata->connection) rpcrt4_conn_cancel_call(tdata->connection);
            LeaveCriticalSection(&tdata->cs);
            break;
        }
    LeaveCriticalSection(&threaddata_cs);

    return RPC_S_OK;
}
