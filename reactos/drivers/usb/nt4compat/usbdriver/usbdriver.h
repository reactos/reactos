#define NTDDI_VERSION   NTDDI_WIN2K
#define _WIN32_WINNT    _WIN32_WINNT_WIN2K

// hacks for WDK
#define __field_bcount_part_opt(x,y)
#define __field_bcount_part(x,y)
#define __field_bcount_opt(x)
#define __in_range(a,b)
#define __volatile
#define __struct_bcount(x)
#define __inexpressible_readableTo(x)
#define __deref_volatile
#ifndef __field_ecount
#    define __field_ecount(x)
#endif


#include <ntddk.h>
#include <stdio.h>
#include "debug.h"
#include "usb.h"
#include "hcd.h"
#include "td.h"
#include "irplist.h"
#include "events.h"
#include "devmgr.h"
#include "hub.h"
#include "umss.h"
#include "mouse.h"
#include "uhciver.h"
