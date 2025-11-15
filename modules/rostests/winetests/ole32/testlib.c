/*
 * Test library
 *
 * Copyright 2019 Nikolay Sivov
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

#if 0
#pragma makedep testdll
#endif

#include <stdio.h>
#include <windows.h>
#include <initguid.h>

DEFINE_GUID(CLSID_WineOOPTest, 0x5201163f, 0x8164, 0x4fd0, 0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd);
DEFINE_GUID(IID_Testiface7, 0x82222222, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj)
{
    if (IsEqualGUID(clsid, &CLSID_WineOOPTest))
        return 0x80001234;

    if (IsEqualGUID(clsid, &IID_Testiface7))
        return 0x80001235;

    return E_NOTIMPL;
}
