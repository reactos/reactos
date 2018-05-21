/*              DirectShow Editing Services (qedit.dll)
 *
 * Copyright 2008 Google (Lei Zhang)
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

#ifndef __QEDIT_PRIVATE_INCLUDED__
#define __QEDIT_PRIVATE_INCLUDED__

#include <stdarg.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <dshow.h>
#define __WINE_DDRAW_H /* ROS HACK */
#include <qedit.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(qedit);

HRESULT AMTimeline_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT MediaDet_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT SampleGrabber_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;

#endif /* __QEDIT_PRIVATE_INCLUDED__ */
