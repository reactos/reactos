/*
 * ReactOS - RPCRT4
 * Copyright 2000 Huw D M Davies for Codeweavers
 *
 * Gettimeofday - Copyright Wu Yongwei
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
 * NOTES: (sedwards) 
 * Dont use this for anything major in ReactOS. Its only here
 * to make dll dependancys happy and for the UUIDCreate Function.
 * 
 * If you wish to work on rpcrt4 in ReactOS please look at the current winehq
 * sources and figure out a way to implement a IPC/LPC bridge so we dont have
 * for this code from WINE.
 *
 * throw the code out and start over. =)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "rpc.h"

#include "ole2.h"
#include "rpcndr.h"
#include "rpcproxy.h"

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "wine/config.h"
#include "wine/port.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static UUID uuid_nil;
static HANDLE master_mutex;

HANDLE RPCRT4_GetMasterMutex(void)
{
    return master_mutex;
}

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
    break;

    case DLL_PROCESS_DETACH:
    break;
    }

    return TRUE;
}

/* From WINE RPC_Binding.c */
LPWSTR RPCRT4_strdupAtoW(LPSTR src)
{
  DWORD len;
  LPWSTR s;
  if (!src) return NULL;
  len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
  s = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
  MultiByteToWideChar(CP_ACP, 0, src, -1, s, len);
  return s;
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
 * (an educated-guess implementation)
 *
 * PARAMS
 *     UUID *Uuid1        [I] Uuid to compare
 *     UUID *Uuid2        [I] Uuid to compare
 *     RPC_STATUS *Status [O] returns RPC_S_OK
 * 
 * RETURNS
 *     -1 if Uuid1 is less than Uuid2
 *     0  if Uuid1 and Uuid2 are equal
 *     1  if Uuid1 is greater than Uuid2
 */
int WINAPI UuidCompare(UUID *Uuid1, UUID *Uuid2, RPC_STATUS *Status)
{
  TRACE("(%s,%s)\n", debugstr_guid(Uuid1), debugstr_guid(Uuid2));
  *Status = RPC_S_OK;
  if (!Uuid1) Uuid1 = &uuid_nil;
  if (!Uuid2) Uuid2 = &uuid_nil;
  if (Uuid1 == Uuid2) return 0;
  return memcmp(Uuid1, Uuid2, sizeof(UUID));
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
int WINAPI UuidIsNil(UUID *uuid, RPC_STATUS *Status)
{
  TRACE("(%s)\n", debugstr_guid(uuid));
  *Status = RPC_S_OK;
  if (!uuid) return TRUE;
  return !memcmp(uuid, &uuid_nil, sizeof(UUID));
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

 /*************************************************************************
 * gettimeofday for systems that lack it
 */

#ifndef __GNUC__
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif

struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;
    static int      tzflag;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    if (tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

/*************************************************************************
 *           UuidCreate   [RPCRT4.@]
 *
 * Creates a 128bit UUID.
 * Implemented according the DCE specification for UUID generation.
 * Code is based upon uuid library in e2fsprogs by Theodore Ts'o.
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * RETURNS
 *
 *  S_OK if successful.
 */
RPC_STATUS WINAPI UuidCreate(UUID *Uuid)
{
   static char has_init = 0;
   static unsigned char a[6];
   static int                      adjustment = 0;
   static struct timeval           last = {0, 0};
   static WORD                     clock_seq;
   struct timeval                  tv;
   unsigned long long              clock_reg;
   DWORD clock_high, clock_low;
   WORD temp_clock_seq, temp_clock_mid, temp_clock_hi_and_version;
#ifdef HAVE_NET_IF_H
   int             sd;
   struct ifreq    ifr, *ifrp;
   struct ifconf   ifc;
   char buf[1024];
   int             n, i;
#endif

   /* Have we already tried to get the MAC address? */
   if (!has_init) {
#ifdef HAVE_NET_IF_H
      /* BSD 4.4 defines the size of an ifreq to be
       * max(sizeof(ifreq), sizeof(ifreq.ifr_name)+ifreq.ifr_addr.sa_len
       * However, under earlier systems, sa_len isn't present, so
       *  the size is just sizeof(struct ifreq)
       */
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
#  ifndef max
#   define max(a,b) ((a) > (b) ? (a) : (b))
#  endif
#  define ifreq_size(i) max(sizeof(struct ifreq),\
sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
# else
#  define ifreq_size(i) sizeof(struct ifreq)
# endif /* HAVE_STRUCT_SOCKADDR_SA_LEN */

      sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
      if (sd < 0) {
	 /* if we can't open a socket, just use random numbers */
	 /* set the multicast bit to prevent conflicts with real cards */
	 a[0] = (rand() & 0xff) | 0x80;
	 a[1] = rand() & 0xff;
	 a[2] = rand() & 0xff;
	 a[3] = rand() & 0xff;
	 a[4] = rand() & 0xff;
	 a[5] = rand() & 0xff;
      } else {
	 memset(buf, 0, sizeof(buf));
	 ifc.ifc_len = sizeof(buf);
	 ifc.ifc_buf = buf;
	 /* get the ifconf interface */
	 if (ioctl (sd, SIOCGIFCONF, (char *)&ifc) < 0) {
	    close(sd);
	    /* no ifconf, so just use random numbers */
	    /* set the multicast bit to prevent conflicts with real cards */
	    a[0] = (rand() & 0xff) | 0x80;
	    a[1] = rand() & 0xff;
	    a[2] = rand() & 0xff;
	    a[3] = rand() & 0xff;
	    a[4] = rand() & 0xff;
	    a[5] = rand() & 0xff;
	 } else {
	    /* loop through the interfaces, looking for a valid one */
	    n = ifc.ifc_len;
	    for (i = 0; i < n; i+= ifreq_size(ifr) ) {
	       ifrp = (struct ifreq *)((char *) ifc.ifc_buf+i);
	       strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
	       /* try to get the address for this interface */
# ifdef SIOCGIFHWADDR
	       if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
		   continue;
	       memcpy(a, (unsigned char *)&ifr.ifr_hwaddr.sa_data, 6);
# else
#  ifdef SIOCGENADDR
	       if (ioctl(sd, SIOCGENADDR, &ifr) < 0)
		   continue;
	       memcpy(a, (unsigned char *) ifr.ifr_enaddr, 6);
#  else
	       /* XXX we don't have a way of getting the hardware address */
	       close(sd);
	       a[0] = 0;
	       break;
#  endif /* SIOCGENADDR */
# endif /* SIOCGIFHWADDR */
	       /* make sure it's not blank */
	       if (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5])
		   continue;

	       goto valid_address;
	    }
	    /* if we didn't find a valid address, make a random one */
	    /* once again, set multicast bit to avoid conflicts */
	    a[0] = (rand() & 0xff) | 0x80;
	    a[1] = rand() & 0xff;
	    a[2] = rand() & 0xff;
	    a[3] = rand() & 0xff;
	    a[4] = rand() & 0xff;
	    a[5] = rand() & 0xff;

	    valid_address:
	    close(sd);
	 }
      }
#else
      /* no networking info, so generate a random address */
      a[0] = (rand() & 0xff) | 0x80;
      a[1] = rand() & 0xff;
      a[2] = rand() & 0xff;
      a[3] = rand() & 0xff;
      a[4] = rand() & 0xff;
      a[5] = rand() & 0xff;
#endif /* HAVE_NET_IF_H */
      has_init = 1;
   }

   /* generate time element of GUID */

   /* Assume that the gettimeofday() has microsecond granularity */
#define MAX_ADJUSTMENT 10

   try_again:
   gettimeofday(&tv, 0);
   if ((last.tv_sec == 0) && (last.tv_usec == 0)) {
      clock_seq = ((rand() & 0xff) << 8) + (rand() & 0xff);
      clock_seq &= 0x1FFF;
      last = tv;
      last.tv_sec--;
   }
   if ((tv.tv_sec < last.tv_sec) ||
       ((tv.tv_sec == last.tv_sec) &&
	(tv.tv_usec < last.tv_usec))) {
      clock_seq = (clock_seq+1) & 0x1FFF;
      adjustment = 0;
   } else if ((tv.tv_sec == last.tv_sec) &&
	      (tv.tv_usec == last.tv_usec)) {
      if (adjustment >= MAX_ADJUSTMENT)
	  goto try_again;
      adjustment++;
   } else
       adjustment = 0;

   clock_reg = tv.tv_usec*10 + adjustment;
   clock_reg += ((unsigned long long) tv.tv_sec)*10000000;
   clock_reg += (((unsigned long long) 0x01B21DD2) << 32) + 0x13814000;

   clock_high = clock_reg >> 32;
   clock_low = clock_reg;
   temp_clock_seq = clock_seq | 0x8000;
   temp_clock_mid = (WORD)clock_high;
   temp_clock_hi_and_version = (clock_high >> 16) | 0x1000;

   /* pack the information into the GUID structure */

   ((unsigned char*)&Uuid->Data1)[3] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&Uuid->Data1)[2] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&Uuid->Data1)[1] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&Uuid->Data1)[0] = (unsigned char)clock_low;

   ((unsigned char*)&Uuid->Data2)[1] = (unsigned char)temp_clock_mid;
   temp_clock_mid >>= 8;
   ((unsigned char*)&Uuid->Data2)[0] = (unsigned char)temp_clock_mid;

   ((unsigned char*)&Uuid->Data3)[1] = (unsigned char)temp_clock_hi_and_version;
   temp_clock_hi_and_version >>= 8;
   ((unsigned char*)&Uuid->Data3)[0] = (unsigned char)temp_clock_hi_and_version;

   ((unsigned char*)Uuid->Data4)[1] = (unsigned char)temp_clock_seq;
   temp_clock_seq >>= 8;
   ((unsigned char*)Uuid->Data4)[0] = (unsigned char)temp_clock_seq;

   ((unsigned char*)Uuid->Data4)[2] = a[0];
   ((unsigned char*)Uuid->Data4)[3] = a[1];
   ((unsigned char*)Uuid->Data4)[4] = a[2];
   ((unsigned char*)Uuid->Data4)[5] = a[3];
   ((unsigned char*)Uuid->Data4)[6] = a[4];
   ((unsigned char*)Uuid->Data4)[7] = a[5];

   TRACE("%s\n", debugstr_guid(Uuid));

   return RPC_S_OK;
}


/*************************************************************************
 *           UuidCreateSequential   [RPCRT4.@]
 *
 * Creates a 128bit UUID by calling UuidCreate.
 * New API in Win 2000
 */
RPC_STATUS WINAPI UuidCreateSequential(UUID *Uuid)
{
   return UuidCreate (Uuid);
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
