/* Definitions for the VERsion info library (VER.DLL)
 *
 * Copyright 1996 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_VERRSRC_H
#define __WINE_VERRSRC_H

/* Macro to deal with LP64 <=> LLP64 differences in numeric constants with 'l' modifier */
#ifndef __MSABI_LONG
# if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#  define __MSABI_LONG(x)         x ## l
# else
#  define __MSABI_LONG(x)         x
# endif
#endif

/* resource ids for different version infos */
#define	VS_FILE_INFO		RT_VERSION
#define	VS_VERSION_INFO		1
#define	VS_USER_DEFINED		100

#define VS_FFI_SIGNATURE        __MSABI_LONG(0xfeef04bd)   /* FileInfo Magic */
#define VS_FFI_STRUCVERSION     __MSABI_LONG(0x00010000)   /* struc version 1.0 */
#define VS_FFI_FILEFLAGSMASK    __MSABI_LONG(0x0000003f)   /* valid flags */

/* VS_VERSION.dwFileFlags */
#define VS_FF_DEBUG             __MSABI_LONG(0x01)
#define VS_FF_PRERELEASE        __MSABI_LONG(0x02)
#define VS_FF_PATCHED           __MSABI_LONG(0x04)
#define VS_FF_PRIVATEBUILD      __MSABI_LONG(0x08)
#define VS_FF_INFOINFERRED      __MSABI_LONG(0x10)
#define VS_FF_SPECIALBUILD      __MSABI_LONG(0x20)

/* VS_VERSION.dwFileOS */

/* major os version */
#define VOS_UNKNOWN             __MSABI_LONG(0x00000000)
#define VOS_DOS                 __MSABI_LONG(0x00010000)
#define VOS_OS216               __MSABI_LONG(0x00020000)
#define VOS_OS232               __MSABI_LONG(0x00030000)
#define VOS_NT                  __MSABI_LONG(0x00040000)
#define VOS_WINCE               __MSABI_LONG(0x00050000)

/* minor os version */
#define VOS__BASE               __MSABI_LONG(0x00000000)
#define VOS__WINDOWS16          __MSABI_LONG(0x00000001)
#define VOS__PM16               __MSABI_LONG(0x00000002)
#define VOS__PM32               __MSABI_LONG(0x00000003)
#define VOS__WINDOWS32          __MSABI_LONG(0x00000004)

/* possible versions */
#define	VOS_DOS_WINDOWS16	(VOS_DOS|VOS__WINDOWS16)
#define	VOS_DOS_WINDOWS32	(VOS_DOS|VOS__WINDOWS32)
#define	VOS_OS216_PM16		(VOS_OS216|VOS__PM16)
#define	VOS_OS232_PM32		(VOS_OS232|VOS__PM32)
#define	VOS_NT_WINDOWS32	(VOS_NT|VOS__WINDOWS32)

/* VS_VERSION.dwFileType */
#define VFT_UNKNOWN             __MSABI_LONG(0x00000000)
#define VFT_APP                 __MSABI_LONG(0x00000001)
#define VFT_DLL                 __MSABI_LONG(0x00000002)
#define VFT_DRV                 __MSABI_LONG(0x00000003)
#define VFT_FONT                __MSABI_LONG(0x00000004)
#define VFT_VXD                 __MSABI_LONG(0x00000005)
/* ??one type missing??         __MSABI_LONG(0x00000006) -Marcus */
#define VFT_STATIC_LIB          __MSABI_LONG(0x00000007)

/* VS_VERSION.dwFileSubtype for VFT_DRV */
#define VFT2_UNKNOWN            __MSABI_LONG(0x00000000)
#define VFT2_DRV_PRINTER        __MSABI_LONG(0x00000001)
#define VFT2_DRV_KEYBOARD       __MSABI_LONG(0x00000002)
#define VFT2_DRV_LANGUAGE       __MSABI_LONG(0x00000003)
#define VFT2_DRV_DISPLAY        __MSABI_LONG(0x00000004)
#define VFT2_DRV_MOUSE          __MSABI_LONG(0x00000005)
#define VFT2_DRV_NETWORK        __MSABI_LONG(0x00000006)
#define VFT2_DRV_SYSTEM         __MSABI_LONG(0x00000007)
#define VFT2_DRV_INSTALLABLE    __MSABI_LONG(0x00000008)
#define VFT2_DRV_SOUND          __MSABI_LONG(0x00000009)
#define VFT2_DRV_COMM           __MSABI_LONG(0x0000000a)
#define VFT2_DRV_INPUTMETHOD    __MSABI_LONG(0x0000000b)
#define VFT2_DRV_VERSIONED_PRINTER __MSABI_LONG(0x0000000c)

/* VS_VERSION.dwFileSubtype for VFT_FONT */
#define VFT2_FONT_RASTER        __MSABI_LONG(0x00000001)
#define VFT2_FONT_VECTOR        __MSABI_LONG(0x00000002)
#define VFT2_FONT_TRUETYPE      __MSABI_LONG(0x00000003)

/* VerFindFile Flags */
	/* input */
#define	VFFF_ISSHAREDFILE	0x0001

	/* output (returned) */
#define	VFF_CURNEDEST		0x0001
#define	VFF_FILEINUSE		0x0002
#define	VFF_BUFFTOOSMALL	0x0004

/* VerInstallFile Flags */
	/* input */
#define	VIFF_FORCEINSTALL	0x0001
#define	VIFF_DONTDELETEOLD	0x0002

	/* output (return) */
#define VIF_TEMPFILE            __MSABI_LONG(0x00000001)
#define VIF_MISMATCH            __MSABI_LONG(0x00000002)
#define VIF_SRCOLD              __MSABI_LONG(0x00000004)
#define VIF_DIFFLANG            __MSABI_LONG(0x00000008)
#define VIF_DIFFCODEPG          __MSABI_LONG(0x00000010)
#define VIF_DIFFTYPE            __MSABI_LONG(0x00000020)
#define VIF_WRITEPROT           __MSABI_LONG(0x00000040)
#define VIF_FILEINUSE           __MSABI_LONG(0x00000080)
#define VIF_OUTOFSPACE          __MSABI_LONG(0x00000100)
#define VIF_ACCESSVIOLATION     __MSABI_LONG(0x00000200)
#define VIF_SHARINGVIOLATION    __MSABI_LONG(0x00000400)
#define VIF_CANNOTCREATE        __MSABI_LONG(0x00000800)
#define VIF_CANNOTDELETE        __MSABI_LONG(0x00001000)
#define VIF_CANNOTRENAME        __MSABI_LONG(0x00002000)
#define VIF_CANNOTDELETECUR     __MSABI_LONG(0x00004000)
#define VIF_OUTOFMEMORY         __MSABI_LONG(0x00008000)
#define VIF_CANNOTREADSRC       __MSABI_LONG(0x00010000)
#define VIF_CANNOTREADDST       __MSABI_LONG(0x00020000)
#define VIF_BUFFTOOSMALL        __MSABI_LONG(0x00040000)
#define VIF_CANNOTLOADLZ32      __MSABI_LONG(0x00080000)
#define VIF_CANNOTLOADCABINET   __MSABI_LONG(0x00100000)


#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define FILE_VER_GET_LOCALISED  0x01
#define FILE_VER_GET_NEUTRAL    0x02
#define FILE_VER_GET_PREFETCHED 0x04

typedef struct tagVS_FIXEDFILEINFO {
	DWORD   dwSignature;
	DWORD   dwStrucVersion;
	DWORD   dwFileVersionMS;
	DWORD   dwFileVersionLS;
	DWORD   dwProductVersionMS;
	DWORD   dwProductVersionLS;
	DWORD   dwFileFlagsMask;
	DWORD   dwFileFlags;
	DWORD   dwFileOS;
	DWORD   dwFileType;
	DWORD   dwFileSubtype;
	DWORD   dwFileDateMS;
	DWORD   dwFileDateLS;
} VS_FIXEDFILEINFO;

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* RC_INVOKED */

#endif /* __WINE_VERRSRC_H */
