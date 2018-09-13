/****************************** Module Header ******************************\
* Module Name: userkdx.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Common include files for kd and ntsd.
* A preprocessed version of this file is passed to structo.exe to build
*  the struct field name-offset tables.
*
* History:
* 04-16-1996 GerardoB Created
\***************************************************************************/
#ifndef _USERKDX_
#define _USERKDX_

#include "precomp.h"
#pragma hdrstop

#ifdef KERNEL
#include <stddef.h>
#include <windef.h>
#define _USERK_
#include "heap.h"
#undef _USERK_
#include <wingdi.h>
#include <w32gdip.h>
#include <kbd.h>
#include <ntgdistr.h>
#include <winddi.h>
#include <gre.h>
#include <ddeml.h>
#include <ddetrack.h>
#include <w32err.h>
#include "immstruc.h"
#include <winuserk.h>
#include <usergdi.h>
#include <zwapi.h>
#include <userk.h>
#include <access.h>
#include <hmgshare.h>

// CNTFS [!dso]
// note: cannot do LCB because it contains a definition which confuses structo:
//                  union { FILE_NAME; OVERLAY_LCB; };
#undef CREATE_NEW
#undef OPEN_EXISTING
#include <..\cntfs\Ntfsproc.h>  // For !dso

// FSRTL [!dso]
#undef ZERO
#include <fsrtl.h>  // For !dso
// these are definitions extracted from ntos\fsrtl\   oplock.c and filelock.c
#include "fsrtllks.h"  // For !dso

// CACHE [!dso]
#undef DebugTrace
#define _BITMAP_RANGE _CCBITMAP_RANGE
#define  BITMAP_RANGE  CCBITMAP_RANGE
#define PBITMAP_RANGE PCCBITMAP_RANGE
#define PBCB CCPBCB
#define Noop CCNoop
#include <..\cache\cc.h>  // For !dso
#undef _BITMAP_RANGE
#undef  BITMAP_RANGE
#undef PBITMAP_RANGE
#undef  PBCB
#undef  Noop

// CNSS [!dso]
#include <..\dd\CNSS\Common.h>  // For !dso

// end

#else // KERNEL

#include "usercli.h"

#include "usersrv.h"
#include <ntcsrdll.h>
#include "csrmsg.h"

#endif

// Rtl heap stuff
#include <..\inc\heap.h>

#include "conapi.h"

// NTCON stuff for DSO
#include "conmsg.h"
#include "server.h"

#include <imagehlp.h>
#include <wdbgexts.h>
#include <ntsdexts.h>
#define NOEXTAPI

// IMM stuff
#include "immuser.h"

#endif /* _USERKDX_ */
