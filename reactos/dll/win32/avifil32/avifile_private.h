/*
 * Copyright 2002 Michael Günnewig
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

#ifndef __AVIFILE_PRIVATE_H
#define __AVIFILE_PRIVATE_H

#include <assert.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <vfw.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "extrachunk.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

#ifndef MAX_AVISTREAMS
#define MAX_AVISTREAMS 8
#endif

#ifndef DIBWIDTHBYTES
#define WIDTHBYTES(i)     (((i+31)&(~31))/8)
#define DIBWIDTHBYTES(bi) WIDTHBYTES((bi).biWidth * (bi).biBitCount)
#endif

#ifndef DIBPTR
#define DIBPTR(lp)      ((LPBYTE)(lp) + (lp)->biSize + \
                         (lp)->biClrUsed * sizeof(RGBQUAD))
#endif

DEFINE_AVIGUID(CLSID_ICMStream, 0x00020001, 0, 0);
DEFINE_AVIGUID(CLSID_WAVFile,   0x00020003, 0, 0);
DEFINE_AVIGUID(CLSID_ACMStream, 0x0002000F, 0, 0);

extern HMODULE AVIFILE_hModule DECLSPEC_HIDDEN;

extern HRESULT AVIFILE_CreateAVIFile(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT AVIFILE_CreateWAVFile(IUnknown *outer_unk, REFIID riid, void **ret_iface) DECLSPEC_HIDDEN;
extern HRESULT AVIFILE_CreateACMStream(REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern HRESULT AVIFILE_CreateICMStream(REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern PAVIEDITSTREAM AVIFILE_CreateEditStream(PAVISTREAM pstream) DECLSPEC_HIDDEN;
extern PGETFRAME AVIFILE_CreateGetFrame(PAVISTREAM pstream) DECLSPEC_HIDDEN;
extern PAVIFILE  AVIFILE_CreateAVITempFile(int nStreams, const PAVISTREAM *ppStreams) DECLSPEC_HIDDEN;

extern LPCWSTR  AVIFILE_BasenameW(LPCWSTR szFileName) DECLSPEC_HIDDEN;

#endif /* __AVIFILE_PRIVATE_H */
