/*
 *      MultiMedia Streams private interfaces (AMSTREAM.DLL)
 *
 * Copyright 2004 Christian Costa
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

#ifndef __AMSTREAM_PRIVATE_INCLUDED__
#define __AMSTREAM_PRIVATE_INCLUDED__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "mmstream.h"
#include "austream.h"
#include "amstream.h"

HRESULT multimedia_stream_create(IUnknown *outer, void **out);
HRESULT AMAudioData_create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT filter_create(IUnknown *outer, void **out);
HRESULT ddraw_stream_create(IUnknown *outer, void **out);
HRESULT audio_stream_create(IUnknown *outer, void **out);

#endif /* __AMSTREAM_PRIVATE_INCLUDED__ */
