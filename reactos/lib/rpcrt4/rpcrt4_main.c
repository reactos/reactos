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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * WINE RPC TODO's (and a few TODONT's)
 *
 * - Ove's decreasingly incomplete widl is an IDL compiler for wine.  For widl
 *   to be wine's only IDL compiler, a fair bit of work remains to be done.
 *   until then we have used some midl-generated stuff.  (What?)
 *   widl currently doesn't generate stub/proxy files required by wine's (O)RPC
 *   capabilities -- nor does it make those lovely format strings :(
 *   The MS MIDL compiler does some really esoteric stuff.  Of course Ove has
 *   started with the less esoteric stuff.  There are also lots of nice
 *   comments in there if you want to flex your bison and help build this monster.
 *
 * - RPC has a quite featureful error handling mechanism; basically none of this is
 *   implemented right now.  We also have deficiencies on the compiler side, where
 *   wine's __TRY / __EXCEPT / __FINALLY macros are not even used for RpcTryExcept & co,
 *   due to syntactic differences! (we can fix it with widl by using __TRY)
 *
 * - There are several different memory allocation schemes for MSRPC.
 *   I don't even understand what they all are yet, much less have them
 *   properly implemented.  Surely we are supposed to be doing something with
 *   the user-provided allocation/deallocation functions, but so far,
 *   I don't think we are doing this...
 *
 * - MSRPC provides impersonation capabilities which currently are not possible
 *   to implement in wine.  At the very least we should implement the authorization
 *   API's & gracefully ignore the irrelevant stuff (to an extent we already do).
 *
 * - Some transports are not yet implemented.  The existing transport implementations
 *   are incomplete and may be bug-infested.
 * 
 * - The various transports that we do support ought to be supported in a more
 *   object-oriented manner, as in DCE's RPC implementation, instead of cluttering
 *   up the code with conditionals like we do now.
 * 
 * - Data marshalling: So far, only the beginnings of a full implementation
 *   exist in wine.  NDR protocol itself is documented, but the MS API's to
 *   convert data-types in memory into NDR are not.  This is challenging work,
 *   and has supposedly been "at the top of Greg's queue" for several months now.
 *
 * - ORPC is RPC for OLE; once we have a working RPC framework, we can
 *   use it to implement out-of-process OLE client/server communications.
 *   ATM there is maybe a disconnect between the marshalling in the OLE DLL's
 *   and the marshalling going on here [TODO: well, is there or not?]
 * 
 * - In-source API Documentation, at least for those functions which we have
 *   implemented, but preferably for everything we can document, would be nice,
 *   since some of this stuff is quite obscure.
 *
 * - Name services... [TODO: what about them]
 *
 * - Protocol Towers: Totally unimplemented.... I think.
 *
 * - Context Handle Rundown: whatever that is.
 *
 * - Nested RPC's: Totally unimplemented.
 *
 * - Statistics: we are supposed to be keeping various counters.  we aren't.
 *
 * - Async RPC: Unimplemented.
 *
 * - XML/http RPC: Somewhere there's an XML fiend that wants to do this! Betcha
 *   we could use these as a transport for RPC's across computers without a
 *   permissions and/or licensing crisis.
 *
 * - The NT "ports" API, aka LPC.  Greg claims this is on his radar.  Might (or
 *   might not) enable users to get some kind of meaningful result out of
 *   NT-based native rpcrt4's.  Commonly-used transport for self-to-self RPC's.
 *
 * - ...?  More stuff I haven't thought of.  If you think of more RPC todo's
 *   drop me an e-mail <gmturner007@ameritech.net> or send a patch to the
 *   wine-patches mailing list.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "winuser.h"
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

WINE_DEFAULT_DEBUG_CHANNEL(ole);

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
      0, 0, { 0, (DWORD)(__FILE__ ": uuid_cs") }
};
static CRITICAL_SECTION uuid_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

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
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        master_mutex = CreateMutexA( NULL, FALSE, RPCSS_MASTER_MUTEX_NAME);
        if (!master_mutex)
          ERR("Failed to create master mutex\n");
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
RPC_STATUS WINAPI RpcStringFreeA(unsigned char** String)
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
RPC_STATUS WINAPI RpcStringFreeW(unsigned short** String)
{
  HeapFree( GetProcessHeap(), 0, *String);

  return RPC_S_OK;
}

/*************************************************************************
 *           RpcRaiseException   [RPCRT4.@]
 *
 * Raises an exception.
 */
void WINAPI RpcRaiseException(RPC_STATUS exception)
{
  /* FIXME: translate exception? */
  RaiseException(exception, 0, 0, NULL);
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
    PIP_ADAPTER_INFO adapter = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), 0, buflen);

    if (GetAdaptersInfo(adapter, &buflen) == ERROR_BUFFER_OVERFLOW) {
        HeapFree(GetProcessHeap(), 0, adapter);
        adapter = (IP_ADAPTER_INFO *)HeapAlloc(GetProcessHeap(), 0, buflen);
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

        address[0] |= 0x80;
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
  int i;

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
 *  S_OUT_OF_MEMORY if unsucessful.
 */
RPC_STATUS WINAPI UuidToStringA(UUID *Uuid, unsigned char** StringUuid)
{
  *StringUuid = HeapAlloc( GetProcessHeap(), 0, sizeof(char) * 37);

  if(!(*StringUuid))
    return RPC_S_OUT_OF_MEMORY;

  if (!Uuid) Uuid = &uuid_nil;

  sprintf(*StringUuid, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
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
 *  S_OUT_OF_MEMORY if unsucessful.
 */
RPC_STATUS WINAPI UuidToStringW(UUID *Uuid, unsigned short** StringUuid)
{
  char buf[37];

  if (!Uuid) Uuid = &uuid_nil;

  sprintf(buf, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
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
RPC_STATUS WINAPI UuidFromStringA(unsigned char* str, UUID *uuid)
{
    BYTE *s = (BYTE *)str;
    int i;

    if (!s) return UuidCreateNil( uuid );

    if (strlen(s) != 36) return RPC_S_INVALID_STRING_UUID;

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
RPC_STATUS WINAPI UuidFromStringW(unsigned short* s, UUID *uuid)
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

HRESULT WINAPI RPCRT4_DllRegisterServer( void )
{
    FIXME( "(): stub\n" );
    return S_OK;
}

BOOL RPCRT4_StartRPCSS(void)
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
    int i, j = 0;

    TRACE("(msg == %p, vardata_payload == %p, reply == %p)\n", msg, vardata_payload, reply);

    client_handle = RPCRT4_RpcssNPConnect();

    while (!client_handle) {
        /* start the RPCSS process */
	if (!RPCRT4_StartRPCSS()) {
	    ERR("Unable to start RPCSS process.\n");
	    return FALSE;
	}
	/* wait for a connection (w/ periodic polling) */
        for (i = 0; i < 60; i++) {
            Sleep(200);
            client_handle = RPCRT4_RpcssNPConnect();
            if (client_handle) break;
        } 
        /* we are only willing to try twice */
	if (j++ >= 1) break;
    }

    if (!client_handle) {
        /* no dice! */
        ERR("Unable to connect to RPCSS process!\n");
	SetLastError(RPC_E_SERVER_DIED_DNE);
	return FALSE;
    }

    /* great, we're connected.  now send the message */
    if (!RPCRT4_SendReceiveNPMsg(client_handle, msg, vardata_payload, reply)) {
        ERR("Something is amiss: RPC_SendReceive failed.\n");
	return FALSE;
    }

    return TRUE;
}
