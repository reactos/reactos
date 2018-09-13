#ifndef __DSKQUOTA_PRIVATE_H
#define __DSKQUOTA_PRIVATE_H
///////////////////////////////////////////////////////////////////////////////
/*  File: private.h

    Description: Private stuff used in the quota management library.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#define USEQUICKSORT  // Tell comctl32 to use QuickSort for sorting DPA's.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NT_INCLUDED
#   include <nt.h>
#endif

#ifndef _NTRTL_
#   include <ntrtl.h>
#endif

#ifndef _NTURTL_
#   include <nturtl.h>
#endif

#include <ntdddfs.h>

#ifndef _NTSEAPI_
#   include <ntseapi.h>
#endif

#ifdef __cplusplus
}  // end of extern "C"
#endif


#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_WINDOWSX
#   include <windowsx.h>
#endif

#ifndef _OLE2_H_
#   include <ole2.h>
#endif

#ifndef _OLECTL_H_
#   include <olectl.h>     // Standard OLE interfaces.
#endif

#ifndef _INC_SHELLAPI
#   include <shellapi.h>
#endif

#ifndef _SHLGUID_H_
#   include <shlguid.h>
#endif

#ifndef _INC_SHLWAPI
#   include <shlwapi.h>
#endif

#ifndef _SHLOBJ_H_
#   include <shlobj.h>
#endif

//
// Disable silly warnings.
//
#pragma warning( disable : 4100 )  // Unreferenced formal parameter
#pragma warning( disable : 4710 )  // Inline was not expanded


//
// Disable "inline" for DEBUG builds so we can set breakpoints
// on inlined methods.
//
#if DBG
#   define INLINE inline
#else
#   define INLINE
#endif

//
// Define PROFILE to activate IceCAP profiler.
//
#ifdef PROFILE
#   include "icapexp.h"
#   define ICAP_START       StartCAP()
#   define ICAP_START_ALL   StartCAPAll()
#   define ICAP_STOP        StopCAP()
#   define ICAP_STOP_ALL    StopCAPAll()
#   define ICAP_SUSPEND     SuspendCAP()
#   define ICAP_SUSPEND_ALL SuspendCAPAll()
#   define ICAP_RESUME      ResumeCAP()
#   define ICAP_RESUME_ALL  SuspendCAPAll()
#else
#   define ICAP_START       0
#   define ICAP_START_ALL   0
#   define ICAP_STOP        0
#   define ICAP_STOP_ALL    0
#   define ICAP_SUSPEND     0
#   define ICAP_SUSPEND_ALL 0
#   define ICAP_RESUME      0
#   define ICAP_RESUME_ALL  0
#endif

typedef unsigned __int64  UINT64;
typedef __int64           INT64;

#ifndef _INC_DSKQUOTA_DEBUG_H
#   include "debug.h"
#endif
#ifndef _INC_DSKQUOTA_DEBUGP_H
#   include "debugp.h"
#endif
#ifndef _INC_DSKQUOTA_EXCEPT_H
#   include "except.h"
#endif
#ifndef _INC_DSKQUOTA_THDSYNC_H
#   include "thdsync.h"
#endif
#ifndef _INC_DSKQUOTA_AUTOPTR_H
#   include "autoptr.h"
#endif
#ifndef _INC_DSKQUOTA_CARRAY_H
#   include "carray.h"
#endif
#ifndef _INC_DSKQUOTA_ALLOC_H
#   include "alloc.h"
#endif
#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif
#ifndef _INC_DSKQUOTA_PATHSTR_H
#   include "pathstr.h"
#endif
#ifndef _INC_DSKQUOTA_UTILS_H
#   include "utils.h"
#endif
#ifndef _INC_DSKQUOTA_DBLNUL_H
#   include "dblnul.h"
#endif
#ifndef _INC_DSKQUOTA_STRRET_H
#   include "strret.h"
#endif
#ifndef _INC_DSKQUOTA_XBYTES_H
#   include "xbytes.h"
#endif
#ifndef _INC_DSKQUOTA_REGSTR_H
#   include "regstr.h"
#endif


extern HINSTANCE g_hInstDll;        // Global module instance handle.
extern LONG      g_cRefThisDll;     // Global module reference count.

//
// Global data mutex and macros.
//
extern  HANDLE g_hMutex;

#define LOCK_GLOBAL_DATA    WaitForSingleObject(g_hMutex, INFINITE)
#define RELEASE_GLOBAL_DATA ReleaseMutex(g_hMutex)

//
// Unlimited quota threshold and limit are indicated by a value of -1.
// A limit of -2 marks a record for deletion.
// This is the way NTFS wants it.
//
const LONGLONG NOLIMIT  = (LONGLONG)-1;
const LONGLONG MARK4DEL = (LONGLONG)-2;

//
// Convenience macro for calculating number of elements in an array.
//
#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif
#define ARRAYSIZE(a)  (sizeof(a)/sizeof((a)[0]))

//
// Per-volume quota information.
//
typedef struct DiskQuotaFSObjectInformation {
    LONGLONG DefaultQuotaThreshold;
    LONGLONG DefaultQuotaLimit;
    ULONG    FileSystemControlFlags;
} DISKQUOTA_FSOBJECT_INFORMATION, *PDISKQUOTA_FSOBJECT_INFORMATION;


//
// SIDLIST is a synonym for FILE_GET_QUOTA_INFORMATION.
//
#define SIDLIST  FILE_GET_QUOTA_INFORMATION
#define PSIDLIST PFILE_GET_QUOTA_INFORMATION

//
// Private stuff for twiddling bits in quota state DWORD.
// Public clients don't need these.
// Note that the LOG_VOLUME_XXXX flags are not included.
// This feature is not exposed through the quota APIs.
//
#define DISKQUOTA_LOGFLAG_MASK              0x00000030
#define DISKQUOTA_LOGFLAG_SHIFT                      4
#define DISKQUOTA_FLAGS_MASK                0x00000337
#define DISKQUOTA_FILEFLAG_MASK             0x00000300


//
// Maximum length of a SID.
//
const UINT MAX_SID_LEN = (FIELD_OFFSET(SID, SubAuthority) + 
                          sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES);
//
// SID is a variable length structure.
// This defines how large the FILE_QUOTA_INFORMATION structure can be if the SID
// is maxed out.
//
const UINT FILE_QUOTA_INFORMATION_MAX_LEN = sizeof(FILE_QUOTA_INFORMATION) -
                                            sizeof(SID) +
                                            MAX_SID_LEN;

//
// BUGBUG: These may actually be shorter than MAX_PATH.
//         Need to find out what actual max is.
//
const UINT MAX_USERNAME      = MAX_PATH;  // i.e. BrianAu
const UINT MAX_DOMAIN        = MAX_PATH;  // i.e. REDMOND
const UINT MAX_FULL_USERNAME = MAX_PATH;  // i.e. Brian Aust

const UINT MAX_VOL_LABEL = 33;  // Includes term NUL.

const UINT MAX_GUIDSTR_LEN = 40;


#endif // __DSKQUOTA_PRIVATE_H
