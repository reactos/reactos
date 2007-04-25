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

#ifndef MAX_AVISTREAMS
#define MAX_AVISTREAMS 8
#endif

#ifndef comptypeDIB
#define comptypeDIB  mmioFOURCC('D','I','B',' ')
#endif

#ifndef DIBWIDTHBYTES
#define WIDTHBYTES(i)     (((i+31)&(~31))/8)
#define DIBWIDTHBYTES(bi) WIDTHBYTES((bi).biWidth * (bi).biBitCount)
#endif

#ifndef DIBPTR
#define DIBPTR(lp)      ((LPBYTE)(lp) + (lp)->biSize + \
                         (lp)->biClrUsed * sizeof(RGBQUAD))
#endif

#define IDD_SAVEOPTIONS      0x0100
#define IDC_INTERLEAVE       0x0110
#define IDC_INTERLEAVEEVERY  0x0111
#define IDC_STREAM           0x0112
#define IDC_OPTIONS          0x0113
#define IDC_FORMATTEXT       0x0114

#define IDS_WAVESTREAMFORMAT 0x0100
#define IDS_WAVEFILETYPE     0x0101
#define IDS_ALLMULTIMEDIA    0x0184
#define IDS_ALLFILES         0x0185
#define IDS_VIDEO            0x0189
#define IDS_AUDIO            0x0190
#define IDS_AVISTREAMFORMAT  0x0191
#define IDS_AVIFILETYPE      0x0192
#define IDS_UNCOMPRESSED     0x0193

DEFINE_AVIGUID(CLSID_ICMStream, 0x00020001, 0, 0);
DEFINE_AVIGUID(CLSID_WAVFile,   0x00020003, 0, 0);
DEFINE_AVIGUID(CLSID_ACMStream, 0x0002000F, 0, 0);
DEFINE_AVIGUID(IID_IEditStreamInternal, 0x0002000A,0,0);

extern HMODULE AVIFILE_hModule;

extern HRESULT AVIFILE_CreateAVIFile(REFIID riid, LPVOID *ppobj);
extern HRESULT AVIFILE_CreateWAVFile(REFIID riid, LPVOID *ppobj);
extern HRESULT AVIFILE_CreateACMStream(REFIID riid, LPVOID *ppobj);
extern HRESULT AVIFILE_CreateICMStream(REFIID riid, LPVOID *ppobj);
extern PAVIEDITSTREAM AVIFILE_CreateEditStream(PAVISTREAM pstream);
extern PGETFRAME AVIFILE_CreateGetFrame(PAVISTREAM pstream);
extern PAVIFILE  AVIFILE_CreateAVITempFile(int nStreams, const PAVISTREAM *ppStreams);

extern LPCWSTR  AVIFILE_BasenameW(LPCWSTR szFileName);

#endif
