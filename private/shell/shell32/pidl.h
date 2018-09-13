/***************************************************************************
 * common stuff for pidls, and for shell type pidls                        *
 * broken out so that it can be accessed from other places such as the     *
 * debugger extension code                                                 *
 **************************************************************************/
 
#ifndef _PIDL_H_
#define _PIDL_H_

// declaration of Basic regitem pidl structure.
#pragma pack(1)
typedef struct _IDREGITEM
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bOrder;
    CLSID   clsid;
} IDREGITEM;
typedef UNALIGNED IDREGITEM *LPIDREGITEM;
typedef const UNALIGNED IDREGITEM *LPCIDREGITEM;
#pragma pack()

// FILE SYSTEM PIDLS from fstreex.h
typedef struct _IDFOLDER_FSA
{
    DWORD   dwSize;
    WORD    dateModified;
    WORD    timeModified;
    WORD    wAttrs;
    CHAR    cFileName[MAX_PATH];
    CHAR    cAltFileName[8+1+3+1];  // short name (may be empty)
} IDFOLDER_FSA;

// only used if the file name can't be round triped to ansi and back
typedef struct _IDFOLDER_FSW
{
    DWORD   dwSize;
    WORD    dateModified;
    WORD    timeModified;
    WORD    wAttrs;
    WCHAR   cFileName[MAX_PATH];
    CHAR    cAltFileName[8+1+3+1];  // ANSI version of cFileName (some chars not converted)
} IDFOLDER_FSW;

typedef struct _IDFOLDERA
{
    WORD            cb;
    BYTE            bFlags;
    IDFOLDER_FSA    fs;
} IDFOLDERA;
typedef UNALIGNED IDFOLDERA *LPIDFOLDERA;
typedef const UNALIGNED IDFOLDERA *LPCIDFOLDERA;

typedef struct _IDFOLDERW
{
    WORD            cb;
    BYTE            bFlags;
    IDFOLDER_FSW    fs;
} IDFOLDERW;
typedef UNALIGNED IDFOLDERW *LPIDFOLDERW;
typedef const UNALIGNED IDFOLDERW *LPCIDFOLDERW;

#ifdef UNICODE
#define IDFOLDER    IDFOLDERW
#define LPIDFOLDER  LPIDFOLDERW
#define LPCIDFOLDER LPCIDFOLDERW
#else
#define IDFOLDER    IDFOLDERA
#define LPIDFOLDER  LPIDFOLDERA
#define LPCIDFOLDER LPCIDFOLDERA
#endif

#pragma pack(1)
typedef struct _IDDRIVE
{
    WORD    cb;
    BYTE    bFlags;
    CHAR    cName[4];
    ULONGLONG qwSize;  // this is a "guess" at the disk size and free space
    ULONGLONG qwFree;
    WORD    wChecksum;
} IDDRIVE;
typedef const UNALIGNED IDDRIVE *LPCIDDRIVE;
typedef UNALIGNED IDDRIVE *LPIDDRIVE;
#pragma pack()


#endif
