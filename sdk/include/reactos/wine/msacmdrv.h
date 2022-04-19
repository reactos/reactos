/*
 * Declarations for MSACM driver
 *
 * Copyright 1998 Patrik Stridvall
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

#ifndef __WINE_MSACMDRV_H
#define __WINE_MSACMDRV_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

/***********************************************************************
 * Types
 */

/***********************************************************************
 * Defines/Enums
 */

#define MAKE_ACM_VERSION(mjr, mnr, bld) \
  (((LONG)(mjr)<<24) | ((LONG)(mnr)<<16) | ((LONG)bld))

#define ACMDRVOPENDESC_SECTIONNAME_CHARS

#define ACMDM_DRIVER_NOTIFY             (ACMDM_BASE + 1)
#define ACMDM_DRIVER_DETAILS            (ACMDM_BASE + 10)
#define ACMDM_DRIVER_ABOUT              (ACMDM_BASE + 11)

#define ACMDM_HARDWARE_WAVE_CAPS_INPUT  (ACMDM_BASE + 20)
#define ACMDM_HARDWARE_WAVE_CAPS_OUTPUT (ACMDM_BASE + 21)

#define ACMDM_FORMATTAG_DETAILS         (ACMDM_BASE + 25)
#define ACMDM_FORMAT_DETAILS            (ACMDM_BASE + 26)
#define ACMDM_FORMAT_SUGGEST            (ACMDM_BASE + 27)

#define ACMDM_FILTERTAG_DETAILS         (ACMDM_BASE + 50)
#define ACMDM_FILTER_DETAILS            (ACMDM_BASE + 51)

#define ACMDM_STREAM_OPEN               (ACMDM_BASE + 76)
#define ACMDM_STREAM_CLOSE              (ACMDM_BASE + 77)
#define ACMDM_STREAM_SIZE               (ACMDM_BASE + 78)
#define ACMDM_STREAM_CONVERT            (ACMDM_BASE + 79)
#define ACMDM_STREAM_RESET              (ACMDM_BASE + 80)
#define ACMDM_STREAM_PREPARE            (ACMDM_BASE + 81)
#define ACMDM_STREAM_UNPREPARE          (ACMDM_BASE + 82)
#define ACMDM_STREAM_UPDATE             (ACMDM_BASE + 83)

/***********************************************************************
 * Structures
 */

typedef struct _ACMDRVOPENDESCA
{
  DWORD  cbStruct;
  FOURCC fccType;
  FOURCC fccComp;
  DWORD  dwVersion;
  DWORD  dwFlags;
  DWORD  dwError;
  LPCSTR pszSectionName;
  LPCSTR pszAliasName;
  DWORD  dnDevNode;
} ACMDRVOPENDESCA, *PACMDRVOPENDESCA;

typedef struct _ACMDRVOPENDESCW
{
  DWORD   cbStruct;
  FOURCC  fccType;
  FOURCC  fccComp;
  DWORD   dwVersion;
  DWORD   dwFlags;
  DWORD   dwError;
  LPCWSTR pszSectionName;
  LPCWSTR pszAliasName;
  DWORD   dnDevNode;
} ACMDRVOPENDESCW, *PACMDRVOPENDESCW;

typedef struct _ACMDRVSTREAMINSTANCE
{
  DWORD           cbStruct;
  PWAVEFORMATEX   pwfxSrc;
  PWAVEFORMATEX   pwfxDst;
  PWAVEFILTER     pwfltr;
  DWORD_PTR       dwCallback;
  DWORD_PTR       dwInstance;
  DWORD           fdwOpen;
  DWORD           fdwDriver;
  DWORD_PTR       dwDriver;
  HACMSTREAM    has;
} ACMDRVSTREAMINSTANCE, *PACMDRVSTREAMINSTANCE;

typedef struct _ACMDRVSTREAMHEADER *PACMDRVSTREAMHEADER;
#include <pshpack1.h>
typedef struct _ACMDRVSTREAMHEADER {
  DWORD                cbStruct;
  DWORD                fdwStatus;
  DWORD_PTR            dwUser;
  LPBYTE               pbSrc;
  DWORD                cbSrcLength;
  DWORD                cbSrcLengthUsed;
  DWORD_PTR            dwSrcUser;
  LPBYTE               pbDst;
  DWORD                cbDstLength;
  DWORD                cbDstLengthUsed;
  DWORD_PTR            dwDstUser;

  DWORD                fdwConvert;
  PACMDRVSTREAMHEADER *padshNext;
  DWORD                fdwDriver;
  DWORD_PTR            dwDriver;

  /* Internal fields for ACM */
  DWORD                fdwPrepared;
  DWORD_PTR            dwPrepared;
  LPBYTE               pbPreparedSrc;
  DWORD                cbPreparedSrcLength;
  LPBYTE               pbPreparedDst;
  DWORD                cbPreparedDstLength;
} ACMDRVSTREAMHEADER;
#include <poppack.h>

typedef struct _ACMDRVSTREAMSIZE
{
  DWORD cbStruct;
  DWORD fdwSize;
  DWORD cbSrcLength;
  DWORD cbDstLength;
} ACMDRVSTREAMSIZE, *PACMDRVSTREAMSIZE;

typedef struct _ACMDRVFORMATSUGGEST
{
  DWORD           cbStruct;
  DWORD           fdwSuggest;
  PWAVEFORMATEX   pwfxSrc;
  DWORD           cbwfxSrc;
  PWAVEFORMATEX   pwfxDst;
  DWORD           cbwfxDst;
} ACMDRVFORMATSUGGEST, *PACMDRVFORMATSUGGEST;

#endif  /* __WINE_MSACMDRV_H */
