/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    apdetect.h

Abstract:

    Some extra stuff to allow registry configuration for
    detect type modes..
    
Author:

    Josh Cohen (joshco)		10-Oct-1998

Environment:

    User Mode - Win32

Revision History:

    Josh Cohen (joshco)		07-Oct-1998
       Created

these are defines for autodetection flags
this allows an admin or tester to easily verify
correct operation of the detection system.
You can control which detection methods are used,
wether or not to force netbios name resolution,
or wether or not to cache the flag.

The default is DNS_A, DHCP, cacheable

--*/


#ifndef PROXY_AUTO_DETECT_TYPE_SAFETY_H
	#define PROXY_AUTO_DETECT_TYPE_SAFETY_H

	#define PROXY_AUTO_DETECT_TYPE_DEFAULT 	67
	// do dns_a, dhcp and cache this flag.
	
	#define PROXY_AUTO_DETECT_TYPE_DHCP 	1
	#define PROXY_AUTO_DETECT_TYPE_DNS_A    2
	#define PROXY_AUTO_DETECT_TYPE_DNS_SRV	4
	#define PROXY_AUTO_DETECT_TYPE_DNS_TXT  8
	#define PROXY_AUTO_DETECT_TYPE_SLP		16

// assume no real domain, netbios
	#define PROXY_AUTO_DETECT_TYPE_NO_DOMAIN 32

// just read this once for performance..
	#define PROXY_AUTO_DETECT_CACHE_ME		64
	
/* this is the default path that we append when
	creating a CURL from a DNS resolve.
	http://wpad/wpad
*/

	#define PROXY_AUTO_DETECT_PATH "wpad.dat"
	
	DWORD
	WINAPI
  		GetProxyDetectType( VOID) ;

#endif
  
