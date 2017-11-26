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

#pragma once

extern LONG objs_loaded;

typedef enum {
    FactoryUnknown,
    FactoryIconHandler,
    FactoryContextMenu,
    FactoryPropSheet,
    FactoryVolPropSheet
} factory_type;

class Factory : public IClassFactory {
public:
    Factory() {
        refcount = 0;
        type = FactoryUnknown;
        InterlockedIncrement(&objs_loaded);
    }

    virtual ~Factory() {
        InterlockedDecrement(&objs_loaded);
    }

    // IUnknown

    HRESULT __stdcall QueryInterface(REFIID riid, void **ppObj);

    ULONG __stdcall AddRef() {
        return InterlockedIncrement(&refcount);
    }

    ULONG __stdcall Release() {
        LONG rc = InterlockedDecrement(&refcount);

        if (rc == 0)
            delete this;

        return rc;
    }

    // IClassFactory

    virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual HRESULT __stdcall LockServer(BOOL bLock);

    factory_type type;

private:
    LONG refcount;
};
