/*
 *  RPCRT4
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <storage32.h>

#include <debug.h>

/***********************************************************************
 * RPCRT4_LibMain
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

BOOL WINAPI
RPCRT4_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	break;

    case DLL_PROCESS_DETACH:
	break;	    
    }

    return TRUE;
}

/*************************************************************************
 *           UuidCreate   [RPCRT4]
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
#if 1
  UNIMPLEMENTED;
  return S_OK;
#else
   static char has_init = 0;
   unsigned char a[6];
   static int                      adjustment = 0;
   static struct timeval           last = {0, 0};
   static UINT16                   clock_seq;
   struct timeval                  tv;
   unsigned long long              clock_reg;
   UINT clock_high, clock_low;
   UINT16 temp_clock_seq, temp_clock_mid, temp_clock_hi_and_version;
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
#ifdef HAVE_SA_LEN
#  ifndef max
#   define max(a,b) ((a) > (b) ? (a) : (b))
#  endif
#  define ifreq_size(i) max(sizeof(struct ifreq),\
sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
# else
#  define ifreq_size(i) sizeof(struct ifreq)
# endif /* HAVE_SA_LEN */
      
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
	    for (i = 0; i < n; i+= ifreq_size(*ifr) ) {
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
   temp_clock_mid = (UINT16)clock_high;
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
   
   Print(MAX_TRACE, ("%s\n", PRINT_GUID(Uuid)));
   
   return S_OK;
#endif
}

/*************************************************************************
 *           RpcStringFreeA   [RPCRT4.436]
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

  return S_OK;
}

/*************************************************************************
 *           UuidToStringA   [RPCRT4.450]
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


  /* FIXME: this should be RPC_S_OUT_OF_MEMORY */
  if(!(*StringUuid))
    return ERROR_OUTOFMEMORY;

       sprintf(*StringUuid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 Uuid->Data1, Uuid->Data2, Uuid->Data3,
                 Uuid->Data4[0], Uuid->Data4[1], Uuid->Data4[2],
                 Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                 Uuid->Data4[6], Uuid->Data4[7] );

  return S_OK; /*FIXME: this should be RPC_S_OK */
}

/***********************************************************************
 *		NdrDllRegisterProxy (RPCRT4.@)
 */
HRESULT WINAPI NdrDllRegisterProxy(
  HMODULE hDll,          /* [in] */
  void **pProxyFileList, /* [???] FIXME: const ProxyFileInfo ** */
  const CLSID *pclsid    /* [in] */
) {
  Print(MIN_TRACE, ("(%x,%p,%s), stub!\n",hDll,pProxyFileList,PRINT_GUID(pclsid)));
  return S_OK;
}
