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

#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS

#include <dshow.h>
#include <amstream.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(amstream);

HRESULT AM_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT AMAudioData_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT MediaStreamFilter_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT ddrawmediastream_create(IMultiMediaStream *Parent, const MSPID *pPurposeId,
        STREAM_TYPE StreamType, IAMMediaStream **ppMediaStream) DECLSPEC_HIDDEN;
HRESULT audiomediastream_create(IMultiMediaStream *parent, const MSPID *purpose_id,
        STREAM_TYPE stream_type, IAMMediaStream **media_stream) DECLSPEC_HIDDEN;

#endif /* __AMSTREAM_PRIVATE_INCLUDED__ */
