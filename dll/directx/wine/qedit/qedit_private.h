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

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "qedit.h"
#include "wine/strmbase.h"

HRESULT timeline_create(IUnknown *outer, IUnknown **out);
HRESULT media_detector_create(IUnknown *outer, IUnknown **out);
HRESULT null_renderer_create(IUnknown *outer, IUnknown **out);
HRESULT sample_grabber_create(IUnknown *outer, IUnknown **out);

#endif /* __QEDIT_PRIVATE_INCLUDED__ */
