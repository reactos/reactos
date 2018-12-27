/* Copyright (c) Mark Harmstone 2016-17
 *
 * This file is part of WinBtrfs.
 *
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include "shellext.h"
#include <windows.h>
#include "factory.h"
#include "iconoverlay.h"
#include "contextmenu.h"
#include "propsheet.h"
#include "volpropsheet.h"

HRESULT __stdcall Factory::QueryInterface(const IID& iid, void** ppv) {
    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();

    return S_OK;
}

HRESULT __stdcall Factory::LockServer(BOOL bLock) {
    return E_NOTIMPL;
}

HRESULT __stdcall Factory::CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv) {
    if (pUnknownOuter)
        return CLASS_E_NOAGGREGATION;

    switch (type) {
        case FactoryIconHandler:
            if (iid == IID_IUnknown || iid == IID_IShellIconOverlayIdentifier) {
                BtrfsIconOverlay* bio = new BtrfsIconOverlay;
                if (!bio)
                    return E_OUTOFMEMORY;

                return bio->QueryInterface(iid, ppv);
            }
            break;

        case FactoryContextMenu:
            if (iid == IID_IUnknown || iid == IID_IContextMenu || iid == IID_IShellExtInit) {
                BtrfsContextMenu* bcm = new BtrfsContextMenu;
                if (!bcm)
                    return E_OUTOFMEMORY;

                return bcm->QueryInterface(iid, ppv);
            }
            break;

        case FactoryPropSheet:
            if (iid == IID_IUnknown || iid == IID_IShellPropSheetExt || iid == IID_IShellExtInit) {
                BtrfsPropSheet* bps = new BtrfsPropSheet;
                if (!bps)
                    return E_OUTOFMEMORY;

                return bps->QueryInterface(iid, ppv);
            }
            break;

        case FactoryVolPropSheet:
            if (iid == IID_IUnknown || iid == IID_IShellPropSheetExt || iid == IID_IShellExtInit) {
                BtrfsVolPropSheet* bps = new BtrfsVolPropSheet;
                if (!bps)
                    return E_OUTOFMEMORY;

                return bps->QueryInterface(iid, ppv);
            }
            break;

        default:
            break;
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}
