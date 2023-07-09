/*
 * Core NT headers
 */
#include <ntos.h>
#include <zwapi.h>
//#include <ntdbg.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddstor.h>
#include <wdmguid.h>
#include <ntcsrmsg.h>

/*
 * Standard C runtime headers
 */
#include <limits.h>
#include <stddef.h>
#include <stdio.h>


/*
 * Win32 headers
 */
#include <windef.h>
#include <wingdi.h>
#include <w32gdip.h>
#include <winerror.h>
#include <ntgdistr.h>
#include <winddi.h>
#include <w32p.h>
#include <w32err.h>
#include <gre.h>
#include <usergdi.h>
#include <heap.h>
#include <ddeml.h>
#include <ddemlp.h>
#include <winuserk.h>
#include <dde.h>
#include <ddetrack.h>


/*
 * Far East specific headers
 */
#ifdef FE_IME
#include <immstruc.h>
#endif

/*
 * NtUser global headers
 */

#include <mountmgr.h>
#include <ioevent.h>

/*
 * NtUser Kernel specific headers
 */
#include <kbd.h>
#include "userk.h"
#include "access.h"
#include <conapi.h>

#include "winstaw.h"
#include <icadd.h>
#include <regapi.h>
