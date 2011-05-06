/*
 * Default Handler Tests
 *
 * Copyright 2008 Huw Davies
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/test.h"


static HRESULT create_storage(IStorage **stg)
{
    HRESULT hr;
    ILockBytes *lock_bytes;

    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &lock_bytes);
    if(SUCCEEDED(hr))
    {
        hr = StgCreateDocfileOnILockBytes(lock_bytes,
                  STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, stg);
        ILockBytes_Release(lock_bytes);
    }
    return hr;
}

typedef struct
{
    DWORD version;
    DWORD flags;
    DWORD link_update_opt;
    DWORD res;
    DWORD moniker_size;
} ole_stream_header_t;

static void test_olestream(void)
{
    HRESULT hr;
    const CLSID non_existent_class = {0xa5f1772f, 0x3772, 0x490f, {0x9e, 0xc6, 0x77, 0x13, 0xe8, 0xb3, 0x4b, 0x5d}};
    IOleObject *ole_obj;
    IPersistStorage *persist;
    IStorage *stg;
    IStream *stm;
    static const WCHAR olestream[] = {1,'O','l','e',0};
    ULONG read;
    ole_stream_header_t header;

    hr = create_storage(&stg);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IStorage_OpenStream(stg, olestream, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stm);
    ok(hr == STG_E_FILENOTFOUND, "got %08x\n", hr);

    hr = OleCreateDefaultHandler(&non_existent_class, 0, &IID_IOleObject, (void**)&ole_obj);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IOleObject_QueryInterface(ole_obj, &IID_IPersistStorage, (void**)&persist);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IPersistStorage_InitNew(persist, stg);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IStorage_OpenStream(stg, olestream, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stm);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IStream_Read(stm, &header, sizeof(header), &read);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(read == sizeof(header), "read %d\n", read);
    ok(header.version == 0x02000001, "got version %08x\n", header.version);
    ok(header.flags == 0x0, "got flags %08x\n", header.flags);
    ok(header.link_update_opt == 0x0, "got link update option %08x\n", header.link_update_opt);
    ok(header.res == 0x0, "got reserved %08x\n", header.res);
    ok(header.moniker_size == 0x0, "got moniker size %08x\n", header.moniker_size);

    IStream_Release(stm);

    IPersistStorage_Release(persist);
    IOleObject_Release(ole_obj);

    IStorage_Release(stg);
}

START_TEST(defaulthandler)
{
    OleInitialize(NULL);

    test_olestream();

    OleUninitialize();
}
