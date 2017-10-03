/*
 *    file system folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2009              Andrew Hill
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IDropTargetHelper implementation
*/

CDropTargetHelper::CDropTargetHelper()
{
}

CDropTargetHelper::~CDropTargetHelper()
{
}

HRESULT WINAPI CDropTargetHelper::InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject *pDataObject)
{
    FIXME ("(%p)->()\n", this);
    return E_NOTIMPL;
}
HRESULT WINAPI CDropTargetHelper::InitializeFromWindow(HWND hwnd, POINT *ppt, IDataObject *pDataObject)
{
    FIXME ("(%p)->()\n", this);
    return E_NOTIMPL;
}


HRESULT WINAPI CDropTargetHelper::DragEnter (HWND hwndTarget, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    FIXME ("(%p)->(%p %p %p 0x%08x)\n", this, hwndTarget, pDataObject, ppt, dwEffect);
    return E_NOTIMPL;
}

HRESULT WINAPI CDropTargetHelper::DragLeave()
{
    FIXME ("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDropTargetHelper::DragOver(POINT *ppt, DWORD dwEffect)
{
    FIXME ("(%p)->(%p 0x%08x)\n", this, ppt, dwEffect);
    return E_NOTIMPL;
}

HRESULT WINAPI CDropTargetHelper::Drop(IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    FIXME ("(%p)->(%p %p 0x%08x)\n", this, pDataObject, ppt, dwEffect);
    return E_NOTIMPL;
}

HRESULT WINAPI CDropTargetHelper::Show(BOOL fShow)
{
    FIXME ("(%p)->(%u)\n", this, fShow);
    return E_NOTIMPL;
}
