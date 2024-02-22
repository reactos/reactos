/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Miscellaneous of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

HRESULT
GetCompartment(
    IUnknown *pUnknown,
    REFGUID rguid,
    ITfCompartment **ppComp,
    BOOL bThread);

HRESULT
SetCompartmentDWORD(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    DWORD dwValue,
    BOOL bThread);

HRESULT
GetCompartmentDWORD(
    IUnknown *pUnknown,
    REFGUID rguid,
    LPDWORD pdwValue,
    BOOL bThread);

HRESULT
SetCompartmentUnknown(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    IUnknown *punkValue);

HRESULT
ClearCompartment(
    TfClientId tid,
    IUnknown *pUnknown,
    REFGUID rguid,
    BOOL bThread);

/***********************************************************************/

class CModeBias
{
public:
    GUID m_guid;

    GUID ConvertModeBias(LONG bias);
    LONG ConvertModeBias(REFGUID guid);
    void SetModeBias(REFGUID rguid);
};
