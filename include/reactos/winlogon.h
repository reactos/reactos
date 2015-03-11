/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/reactos/winlogon.h
 * PURPOSE:     Private interface between Win32 and Winlogon
 * PROGRAMMER:  Ge van Geldorp (gvg@reactos.com)
 */

#ifndef REACTOS_WINLOGON_H_INCLUDED
#define REACTOS_WINLOGON_H_INCLUDED

#define EWX_SHUTDOWN_CANCELED       0x0080

#define EWX_CALLER_SYSTEM           0x0100
#define EWX_CALLER_WINLOGON         0x0200

#define EWX_CALLER_WINLOGON_LOGOFF  0x1000 // WARNING!! Broken flag.

// All the range 0x0400 to 0x1000 is reserved for Winlogon.

// Flag 0x2000 appears to be a flag set when we call InitiateSystemShutdown* APIs (Winlogon shutdown APIs).

// 0x4000 is also reserved.

#define EWX_NOTIFY      0x8000
#define EWX_NONOTIFY    0x10000

#endif /* REACTOS_WINLOGON_H_INCLUDED */

/* EOF */
