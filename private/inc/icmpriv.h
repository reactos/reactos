/****************************Module*Header******************************\
* Module Name: ICMPRIV.H
*
* Module Descripton: Internal data structures and constants for ICM
*
* Warnings:
*
* Issues:
*
* Created:  8 January 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#ifndef _ICMPRIV_H_
#define _ICMPRIV_H_

#include "icm.h"          // include external stuff first

#ifdef __cplusplus
extern "C" {
#endif

//
// External (but OS internal) functional declarations
//

BOOL    InternalGetPS2ColorSpaceArray (PBYTE, DWORD, DWORD, PBYTE, PDWORD, PBOOL);
BOOL    InternalGetPS2ColorRenderingIntent(PBYTE, DWORD, PBYTE, PDWORD);
BOOL    InternalGetPS2ColorRenderingDictionary(PBYTE, DWORD, PBYTE, PDWORD, PBOOL);
BOOL    InternalGetPS2PreviewCRD(PBYTE, PBYTE, DWORD, PBYTE, PDWORD, PBOOL);
BOOL    InternalGetPS2CSAFromLCS(LPLOGCOLORSPACE, PBYTE, PDWORD, PBOOL);
BOOL    InternalGetDeviceConfig(LPCTSTR,DWORD,DWORD,PVOID,PDWORD);
BOOL    InternalSetDeviceConfig(LPCTSTR,DWORD,DWORD,PVOID,DWORD);

//
// Function ID for InternalGet/SetDeviceConfig
//

#define MSCMS_PROFILE_ENUM_MODE     1

#if !defined(_GDI32_)  // not include from here if gdi32.

//
// Useful macros
//

#define ABS(x)                      ((x) > 0 ? (x) : -(x))
#define DWORD_ALIGN(x)              (((x) + 3) & ~3)

#ifdef LITTLE_ENDIAN
#define FIX_ENDIAN(x)               (((x) & 0xff000000) >> 24 | \
                                     ((x) & 0xff0000)   >> 8  | \
                                     ((x) & 0xff00)     << 8  | \
                                     ((x) & 0xff)       << 24 )

#define FIX_ENDIAN16(x)             (((x) & 0xff00) >> 8 | ((x) & 0xff) << 8)
#else
#define FIX_ENDIAN(x)               (x)
#define FIX_ENDIAN16(x)             (x)
#endif

#if !defined(FROM_PS) // not include from here if postscript driver.

//
// MSCMS Internal definition
//

typedef struct tagTAGDATA {
    TAGTYPE tagType;
    DWORD   dwOffset;
    DWORD   cbSize;
} TAGDATA;
typedef TAGDATA *PTAGDATA;

//
// ICM supports the following  objects:
// 1. Profile object: This is created when an application requsts a handle
//      to a profile.
// 2. Color transform object: This is created when an application creates
//      a color transform.
// 3. CMM object: This is created when ICM loads a CMM into memory to
//      perform color matching.
//

typedef enum {
    OBJ_PROFILE             = 'PRFL',
    OBJ_TRANSFORM           = 'XFRM',
    OBJ_CMM                 = ' CMM',
} OBJECTTYPE;

typedef struct tagOBJHEAD {
    OBJECTTYPE  objType;
    DWORD       dwUseCount;
} OBJHEAD;
typedef OBJHEAD *POBJHEAD;

//
// Profile object:
// Memory for profile objects is allocated from ICM's per process heap.
// These objects use handles from ICM's per process handle table.
//

typedef struct tagPROFOBJ {
    OBJHEAD   objHdr;           // common object header info
    DWORD     dwType;           // type (from profile structure)
    PVOID     pProfileData;     // data (from profile structure)
    DWORD     cbDataSize;       // size of data (from profile structure)
    DWORD     dwFlags;          // miscellaneous flags
    HANDLE    hFile;            // handle to open profile
    HANDLE    hMap;             // handle to profile mapping
    DWORD     dwMapSize;        // size of the file mapping object
    PBYTE     pView;            // pointer to mapped view of profile
} PROFOBJ;
typedef PROFOBJ *PPROFOBJ;

//
// Flags for ((PPROFOBJ)0)->dwFlags
//

#define MEMORY_MAPPED       1   // memory mapped profile
#define PROFILE_TEMP        2   // temporary profile has been created
#define READWRITE_ACCESS    4   // if this bit is set, app has read & write
                                // access to profile, else it has only read
                                // read access.

//
// Transform returned by CMM
//

typedef HANDLE  HCMTRANSFORM;

//
// For internal use, compiler doesn't accept PBYTE* below
//

typedef PBYTE*  PPBYTE;

//
// CMM function calltable
//

typedef struct tagCMMFNS {

    //
    // Required functions
    //

    DWORD          (WINAPI *pCMGetInfo)(DWORD);
    HCMTRANSFORM   (WINAPI *pCMCreateTransform)(LPLOGCOLORSPACE, PVOID, PVOID);
    HCMTRANSFORM   (WINAPI *pCMCreateTransformExt)(LPLOGCOLORSPACE, PVOID, PVOID, DWORD);
    BOOL           (WINAPI *pCMDeleteTransform)(HCMTRANSFORM);
    BOOL           (WINAPI *pCMTranslateRGBs)(HCMTRANSFORM, PVOID, BMFORMAT,
                       DWORD, DWORD, DWORD, PVOID, BMFORMAT, DWORD);
    BOOL           (WINAPI *pCMTranslateRGBsExt)(HCMTRANSFORM, PVOID, BMFORMAT,
                       DWORD, DWORD, DWORD, PVOID, BMFORMAT, DWORD, PBMCALLBACKFN, LPARAM);
    BOOL           (WINAPI *pCMCheckRGBs)(HCMTRANSFORM, PVOID, BMFORMAT,
                       DWORD, DWORD, DWORD, PBYTE, PBMCALLBACKFN, LPARAM);
    HCMTRANSFORM   (WINAPI *pCMCreateMultiProfileTransform)(PHPROFILE, DWORD, PDWORD, DWORD, DWORD);
    BOOL           (WINAPI *pCMTranslateColors)(HCMTRANSFORM, PCOLOR, DWORD,
                       COLORTYPE, PCOLOR, COLORTYPE);
    BOOL           (WINAPI *pCMCheckColors)(HCMTRANSFORM, PCOLOR, DWORD,
                       COLORTYPE, PBYTE);
    //
    // Optional functions
    //

    BOOL           (WINAPI *pCMCreateProfile)(LPLOGCOLORSPACE, PPBYTE);
    BOOL           (WINAPI *pCMGetNamedProfileInfo)(HPROFILE, PNAMED_PROFILE_INFO);
    BOOL           (WINAPI *pCMConvertColorNameToIndex)(HPROFILE, LPCOLOR_NAME, LPDWORD, DWORD);
    BOOL           (WINAPI *pCMConvertIndexToColorName)(HPROFILE, LPDWORD, LPCOLOR_NAME, DWORD);
    BOOL           (WINAPI *pCMCreateDeviceLinkProfile)(PHPROFILE, DWORD, PDWORD, DWORD, DWORD, PPBYTE);
    BOOL           (WINAPI *pCMIsProfileValid)(HPROFILE, PBOOL);
    BOOL           (WINAPI *pCMGetPS2ColorSpaceArray)(HPROFILE, DWORD, DWORD, PBYTE, PDWORD, PBOOL);
    BOOL           (WINAPI *pCMGetPS2ColorRenderingIntent)(HPROFILE, DWORD, PBYTE, PDWORD);
    BOOL           (WINAPI *pCMGetPS2ColorRenderingDictionary)(HPROFILE, DWORD,
                       PBYTE, PDWORD, PBOOL);
} CMMFNS;
typedef CMMFNS *PCMMFNS;

//
// CMM object:
// Memory for CMM objects is allocated from ICM's per process heap.
// They are maintained in a linked list.
//

typedef struct tagCMMOBJ {
    OBJHEAD           objHdr;
    DWORD             dwFlags;  // miscellaneous flags
    DWORD             dwCMMID;  // ICC identifier
    DWORD             dwTaskID; // process ID of current task
    HINSTANCE         hCMM;     // handle to instance of CMM dll
    CMMFNS            fns;      // function calltable
    struct tagCMMOBJ* pNext;    // pointer to next object
} CMMOBJ;
typedef CMMOBJ *PCMMOBJ;

//
//  dwFlags for CMMOBJ
//

#define CMM_DONT_USE_PS2_FNS        0x00001

//
// Color transform object
//

typedef struct tagTRANSFORMOBJ {
    OBJHEAD      objHdr;
    PCMMOBJ      pCMMObj;       // pointer to CMM object
    HCMTRANSFORM hcmxform;      // transform returned by CMM
} TRANSFORMOBJ;
typedef TRANSFORMOBJ *PTRANSFORMOBJ;

//
// Parameter to InternalHandleColorProfile
//

typedef enum {
    ADDPROFILES,
    REMOVEPROFILES,
    ENUMPROFILES,
} PROFILEOP;

//
// CMM returned transform should be larger than this value
//

#define TRANSFORM_ERROR    (HTRANSFORM)255

#define PROFILE_SIGNATURE          'psca'

#define HEADER(pProfObj)           ((PPROFILEHEADER)pProfObj->pView)
#define VIEW(pProfObj)             (pProfObj->pView)
#define PROFILE_SIZE(pProfObj)     (FIX_ENDIAN(HEADER(pProfObj)->phSize))
#define TAG_COUNT(pProfObj)        (*((DWORD *)(VIEW(pProfObj) + \
                                   sizeof(PROFILEHEADER))))
#define TAG_DATA(pProfObj)         ((PTAGDATA)(VIEW(pProfObj) + \
                                   sizeof(PROFILEHEADER) + sizeof(DWORD)))

#define MAGIC                      'ICM '
#define PTRTOHDL(x)                ((HANDLE)((ULONG_PTR)(x) ^ MAGIC))
#define HDLTOPTR(x)                ((ULONG_PTR)(x) ^ MAGIC)

PVOID   MemAlloc(DWORD);
PVOID   MemReAlloc(PVOID, DWORD);
VOID    MemFree(PVOID);
VOID    MyCopyMemory(PBYTE, PBYTE, DWORD);
PVOID   AllocateHeapObject(OBJECTTYPE);
VOID    FreeHeapObject(HANDLE);
BOOL    ValidHandle(HANDLE, OBJECTTYPE);
PCMMOBJ GetColorMatchingModule(DWORD);
BOOL    ValidColorMatchingModule(DWORD,PTSTR);
PCMMOBJ GetPreferredCMM();
VOID    ReleaseColorMatchingModule(PCMMOBJ);
BOOL    ValidProfile(PPROFOBJ);
BOOL    ConvertToAnsi(PCWSTR, PSTR*, BOOL);
BOOL    ConvertToUnicode(PCSTR, PWSTR*, BOOL);
PTSTR   GetFilenameFromPath(PTSTR);

//
// For use with the new Device Settings tag
//

typedef struct _SETTINGS {
    DWORD         dwSettingType;     // 'rsln', 'mdia' etc.
    DWORD         dwSizePerValue;    // number of bytes per value
    DWORD         nValues;           // number of values
    DWORD         Value[1];          // array of value entries
} SETTINGS, *PSETTINGS;

typedef struct _SETTINGCOMBOS {
    DWORD         dwSize;           // size of this structure, including sub structures
    DWORD         nSettings;        // number of setting structures
    SETTINGS      Settings[1];      // array of setting entries
} SETTINGCOMBOS, *PSETTINGCOMBOS;

typedef struct _PERPLATFORMENTRY {
    DWORD         PlatformID;        // platform signature ('msft', 'appl' etc.)
    DWORD         dwSize;            // size of this structure, including sub structures
    DWORD         nSettingCombos;    // number of setting combo structures
    SETTINGCOMBOS SettingCombos[1];  // array of setting combos entries
} PLATFORMENTRY, *PPLATFORMENTRY;

typedef struct _DEVICESETTINGS {
    DWORD         dwTagID;           // 'devs'
    DWORD         dwReserved;        // must be 0
    DWORD         nPlatforms;        // number of platform structures
    PLATFORMENTRY PlatformEntry[1];  // array of platform entries
} DEVICESETTINGS, *PDEVICESETTINGS;

#endif  // ifndef FROM_PS

#endif  // ifndef _GDI32_

#ifdef __cplusplus
}
#endif

#endif  // ifndef _ICMPRIV_H_

