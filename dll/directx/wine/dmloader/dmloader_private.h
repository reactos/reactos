/* DirectMusicLoader Private Include
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DMLOADER_PRIVATE_H
#define __WINE_DMLOADER_PRIVATE_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "winreg.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"
#include "dmobject.h"

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicLoaderCF             IDirectMusicLoaderCF;
typedef struct IDirectMusicContainerCF          IDirectMusicContainerCF;

typedef struct IDirectMusicLoaderFileStream     IDirectMusicLoaderFileStream;
typedef struct IDirectMusicLoaderResourceStream IDirectMusicLoaderResourceStream;
typedef struct IDirectMusicLoaderGenericStream  IDirectMusicLoaderGenericStream;

/*****************************************************************************
 * Creation helpers
 */
extern HRESULT create_dmloader(REFIID riid, void **ret_iface);
extern HRESULT create_dmcontainer(REFIID riid, void **ret_iface);
extern HRESULT DMUSIC_CreateDirectMusicLoaderResourceStream(void **ppobj);

/*****************************************************************************
 * IDirectMusicLoaderResourceStream implementation structure
 */
struct IDirectMusicLoaderResourceStream {
	/* IUnknown fields */
	const IStreamVtbl *StreamVtbl;
	/* reference counter */
	LONG dwRef;
	/* data */
	LPBYTE pbMemData;
	LONGLONG llMemLength;
	/* current position */
	LONGLONG llPos;	
};

/* Custom: */
extern HRESULT WINAPI IDirectMusicLoaderResourceStream_Attach(LPSTREAM iface, LPBYTE pbMemData,
        LONGLONG llMemLength, LONGLONG llPos);

extern HRESULT loader_stream_create(IDirectMusicLoader *loader, IStream *stream, IStream **ret_iface);
extern HRESULT file_stream_create(const WCHAR *path, IStream **ret_iface);

#include "debug.h"

#endif	/* __WINE_DMLOADER_PRIVATE_H */
