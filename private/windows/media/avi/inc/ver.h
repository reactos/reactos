/*****************************************************************************\
*                                                                             *
* ver.h -       Version management functions, types, and definitions          *
*                                                                             *
*               Include file for VER.DLL and VER.LIB.  These libraries are    *
*               designed to allow version stamping of Windows executable files*
*               and of special .VER files for DOS executable files.           *
*                                                                             *
*               The API is unchanged for LIB and DLL versions.                *
*                                                                             *
*               Copyright (c) 1992, Microsoft Corp.  All rights reserved      *
*                                                                             *
*******************************************************************************
*
* #define LIB   - To be used with VER.LIB (default is for VER.DLL)
*
\*****************************************************************************/

#ifndef _INC_VER
#define _INC_VER

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/*
 * If .lib version is being used, declare types used in this file.
 */
#ifdef LIB

#ifndef WINAPI                      /* don't declare if they're already declared */
#define WINAPI      _far _pascal
#define NEAR        _near
#define FAR         _far
#define PASCAL      _pascal
typedef int             BOOL;
#define TRUE        1
#define FALSE       0
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    UINT;
typedef signed long     LONG;
typedef unsigned long   DWORD;
typedef char far*       LPSTR;
typedef const char far* LPCSTR;
typedef int             HFILE;
#define OFSTRUCT    void            /* Not used by the .lib version */
#define LOWORD(l)		((WORD)(l))
#define HIWORD(l)		((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define MAKEINTRESOURCE(i)	(LPSTR)((DWORD)((WORD)(i)))
#endif  /* WINAPI */

#else   /* LIB */

/* If .dll version is being used and we're being included with
 * the 3.0 windows.h, #define compatible type aliases.
 * If included with the 3.0 windows.h, #define compatible aliases
 */
#ifndef _INC_WINDOWS
#define UINT        WORD
#define LPCSTR      LPSTR
#define HFILE       int
#endif  /* !_INC_WINDOWS */

#endif  /* !LIB */

/* ----- RC defines ----- */
#ifdef RC_INVOKED
#define ID(id)			id
#else
#define ID(id)			MAKEINTRESOURCE(id)
#endif

/* ----- Symbols ----- */
#define VS_FILE_INFO		ID(16)		/* Version stamp res type */
#define VS_VERSION_INFO		ID(1)  		/* Version stamp res ID */
#define VS_USER_DEFINED		ID(100)		/* User-defined res IDs */

/* ----- VS_VERSION.dwFileFlags ----- */
#define	VS_FFI_SIGNATURE	0xFEEF04BDL
#define	VS_FFI_STRUCVERSION	0x00010000L
#define	VS_FFI_FILEFLAGSMASK	0x0000003FL

/* ----- VS_VERSION.dwFileFlags ----- */
#define	VS_FF_DEBUG		0x00000001L
#define	VS_FF_PRERELEASE	0x00000002L
#define	VS_FF_PATCHED		0x00000004L
#define	VS_FF_PRIVATEBUILD	0x00000008L
#define	VS_FF_INFOINFERRED	0x00000010L
#define	VS_FF_SPECIALBUILD	0x00000020L

/* ----- VS_VERSION.dwFileOS ----- */
#define	VOS_UNKNOWN		0x00000000L
#define	VOS_DOS			0x00010000L
#define	VOS_OS216		0x00020000L
#define	VOS_OS232		0x00030000L
#define	VOS_NT			0x00040000L

#define	VOS__BASE		0x00000000L
#define	VOS__WINDOWS16		0x00000001L
#define	VOS__PM16		0x00000002L
#define	VOS__PM32		0x00000003L
#define	VOS__WINDOWS32		0x00000004L

#define	VOS_DOS_WINDOWS16	0x00010001L
#define	VOS_DOS_WINDOWS32	0x00010004L
#define	VOS_OS216_PM16		0x00020002L
#define	VOS_OS232_PM32		0x00030003L
#define	VOS_NT_WINDOWS32	0x00040004L

/* ----- VS_VERSION.dwFileType ----- */
#define	VFT_UNKNOWN		0x00000000L
#define	VFT_APP			0x00000001L
#define	VFT_DLL			0x00000002L
#define	VFT_DRV			0x00000003L
#define	VFT_FONT		0x00000004L
#define	VFT_VXD			0x00000005L
#define	VFT_STATIC_LIB		0x00000007L

/* ----- VS_VERSION.dwFileSubtype for VFT_WINDOWS_DRV ----- */
#define	VFT2_UNKNOWN		0x00000000L
#define VFT2_DRV_PRINTER	0x00000001L
#define	VFT2_DRV_KEYBOARD	0x00000002L
#define	VFT2_DRV_LANGUAGE	0x00000003L
#define	VFT2_DRV_DISPLAY	0x00000004L
#define	VFT2_DRV_MOUSE		0x00000005L
#define	VFT2_DRV_NETWORK	0x00000006L
#define	VFT2_DRV_SYSTEM		0x00000007L
#define	VFT2_DRV_INSTALLABLE	0x00000008L
#define	VFT2_DRV_SOUND		0x00000009L
#define	VFT2_DRV_COMM		0x0000000AL

/* ----- VS_VERSION.dwFileSubtype for VFT_WINDOWS_FONT ----- */
#define VFT2_FONT_RASTER	0x00000001L
#define	VFT2_FONT_VECTOR	0x00000002L
#define	VFT2_FONT_TRUETYPE	0x00000003L

/* ----- VerFindFile() flags ----- */
#define VFFF_ISSHAREDFILE	0x0001

#define VFF_CURNEDEST		0x0001
#define VFF_FILEINUSE		0x0002
#define VFF_BUFFTOOSMALL	0x0004

/* ----- VerInstallFile() flags ----- */
#define VIFF_FORCEINSTALL	0x0001
#define VIFF_DONTDELETEOLD	0x0002

#define VIF_TEMPFILE		0x00000001L
#define VIF_MISMATCH		0x00000002L
#define VIF_SRCOLD		0x00000004L

#define VIF_DIFFLANG		0x00000008L
#define VIF_DIFFCODEPG		0x00000010L
#define VIF_DIFFTYPE		0x00000020L

#define VIF_WRITEPROT		0x00000040L
#define VIF_FILEINUSE		0x00000080L
#define VIF_OUTOFSPACE		0x00000100L
#define VIF_ACCESSVIOLATION	0x00000200L
#define VIF_SHARINGVIOLATION	0x00000400L
#define VIF_CANNOTCREATE	0x00000800L
#define VIF_CANNOTDELETE	0x00001000L
#define VIF_CANNOTRENAME	0x00002000L
#define VIF_CANNOTDELETECUR	0x00004000L
#define VIF_OUTOFMEMORY		0x00008000L

#define VIF_CANNOTREADSRC	0x00010000L
#define VIF_CANNOTREADDST	0x00020000L

#define VIF_BUFFTOOSMALL	0x00040000L

#ifndef RC_INVOKED              /* RC doesn't need to see the rest of this */

/* ----- Types and structures ----- */

typedef signed short int SHORT;

typedef struct tagVS_FIXEDFILEINFO
{
    DWORD   dwSignature;            /* e.g. 0xfeef04bd */
    DWORD   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
    DWORD   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
    DWORD   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
    DWORD   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
    DWORD   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
    DWORD   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
    DWORD   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    DWORD   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
    DWORD   dwFileType;             /* e.g. VFT_DRIVER */
    DWORD   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
    DWORD   dwFileDateMS;           /* e.g. 0 */
    DWORD   dwFileDateLS;           /* e.g. 0 */
} VS_FIXEDFILEINFO;

/* ----- Function prototypes ----- */

UINT WINAPI VerFindFile(UINT uFlags, LPCSTR szFileName,
      LPCSTR szWinDir, LPCSTR szAppDir,
      LPSTR szCurDir, UINT FAR* lpuCurDirLen,
      LPSTR szDestDir, UINT FAR* lpuDestDirLen);

DWORD WINAPI VerInstallFile(UINT uFlags,
      LPCSTR szSrcFileName, LPCSTR szDestFileName, LPCSTR szSrcDir,
      LPCSTR szDestDir, LPCSTR szCurDir, LPSTR szTmpFile, UINT FAR* lpuTmpFileLen);

/* Returns size of version info in bytes */
DWORD WINAPI GetFileVersionInfoSize(
      LPCSTR lpstrFilename,     /* Filename of version stamped file */
      DWORD FAR *lpdwHandle);   /* Information for use by GetFileVersionInfo */

/* Read version info into buffer */
BOOL WINAPI GetFileVersionInfo(
      LPCSTR lpstrFilename,     /* Filename of version stamped file */
      DWORD dwHandle,           /* Information from GetFileVersionSize */
      DWORD dwLen,              /* Length of buffer for info */
      void FAR* lpData);        /* Buffer to place the data structure */

/* Returns size of resource in bytes */
DWORD WINAPI GetFileResourceSize(
      LPCSTR lpstrFilename,     /* Filename of version stamped file */
      LPCSTR lpstrResType,      /* Type:  normally VS_FILE_INFO */
      LPCSTR lpstrResID,        /* ID:  normally VS_VERSION_INFO */
      DWORD FAR *lpdwFileOffset); /* Returns file offset of resource */

/* Reads file resource into buffer */
BOOL WINAPI GetFileResource(
      LPCSTR lpstrFilename,     /* Filename of version stamped file */
      LPCSTR lpstrResType,      /* Type:  normally VS_FILE_INFO */
      LPCSTR lpstrResID,        /* ID:  normally VS_VERSION_INFO */
      DWORD dwFileOffset,       /* File offset or NULL */
      DWORD dwResLen,           /* Length of resource to read or NULL */
      void FAR* lpData);        /* Pointer to data buffer */

UINT WINAPI VerLanguageName(UINT wLang, LPSTR szLang, UINT nSize);

#ifdef LIB

UINT WINAPI GetWindowsDir(LPCSTR szAppDir, LPSTR lpBuffer, int nSize);

UINT WINAPI GetSystemDir(LPCSTR szAppDir, LPSTR lpBuffer, int nSize);

#endif /* LIB */

BOOL WINAPI VerQueryValue(const void FAR* pBlock, LPCSTR lpSubBlock,
      void FAR* FAR* lplpBuffer, UINT FAR* lpuLen);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#pragma pack()

#endif  /* !RC_INVOKED */
#endif  /* !_INC_VER */
