/*
* ReactOS Explorer
*
* Copyright 2014 - David Quintana
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "precomp.h"

extern HRESULT InitShellServices(HDPA * phdpa);
extern HRESULT ShutdownShellServices(HDPA hdpa);

static int CALLBACK InitializeAllCallback(void* pItem, void* pData)
{
    IOleCommandTarget * pOct = reinterpret_cast<IOleCommandTarget *>(pItem);
    HRESULT * phr = reinterpret_cast<HRESULT *>(pData);
    TRACE("Initializing SSO %p\n", pOct);
    *phr = pOct->Exec(&CGID_ShellServiceObject, OLECMDID_NEW, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    return SUCCEEDED(*phr);
}

static int CALLBACK ShutdownAllCallback(void* pItem, void* pData)
{
    IOleCommandTarget * pOct = reinterpret_cast<IOleCommandTarget *>(pItem);
    TRACE("Shutting down SSO %p\n", pOct);
    pOct->Exec(&CGID_ShellServiceObject, OLECMDID_SAVE, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    return TRUE;
}

static int CALLBACK DeleteAllEnumCallback(void* pItem, void* pData)
{
    IOleCommandTarget * pOct = reinterpret_cast<IOleCommandTarget *>(pItem);
    TRACE("Releasing SSO %p\n", pOct);
    pOct->Release();
    return TRUE;
}

HRESULT InitShellServices(HDPA * phdpa)
{
    IOleCommandTarget * pOct;
    HKEY    hkey;
    CLSID   clsid;
    WCHAR   name[MAX_PATH];
    WCHAR   value[MAX_PATH];
    DWORD   type;
    LONG    ret;
    HDPA    hdpa;
    HRESULT hr = S_OK;
    int     count = 0;

    *phdpa = NULL;

    TRACE("Enumerating Shell Service Ojbect GUIDs...\n");

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad",
        &hkey))
    {
        ERR("RegOpenKey failed.\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hdpa = DPA_Create(5);

    /* Enumerate */
    do
    {
        DWORD   name_len = MAX_PATH;
        DWORD   value_len = sizeof(value); /* byte count! */

        ret = RegEnumValueW(hkey, count, name, &name_len, 0, &type, (LPBYTE) &value, &value_len);
        if (ret)
            break;

        if (type != REG_SZ)
        {
            WARN("Value type was not REG_SZ.\n");
            continue;
        }

        hr = CLSIDFromString(value, &clsid);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            ERR("CLSIDFromString failed %08x.\n", hr);
            goto cleanup;
        }

        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IOleCommandTarget, &pOct));
        if (FAILED_UNEXPECTEDLY(hr))
        {
            ERR("CoCreateInstance failed %08x.\n", hr);
            goto cleanup;
        }

        DPA_AppendPtr(hdpa, pOct);

        count++;
    }
    while (1);

    if (ret != ERROR_NO_MORE_ITEMS)
    {
        ERR("RegEnumValueW failed %08x.\n", ret);
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    RegCloseKey(hkey);

    /* Initialize */
    DPA_EnumCallback(hdpa, InitializeAllCallback, &hr);
    if (FAILED_UNEXPECTEDLY(hr))
        goto cleanup;

    *phdpa = hdpa;
    return count > 0 ? S_OK : S_FALSE;

cleanup:
    *phdpa = NULL;
    ShutdownShellServices(hdpa);
    return hr;
}

HRESULT ShutdownShellServices(HDPA hdpa)
{
    DPA_EnumCallback(hdpa, ShutdownAllCallback, NULL);
    DPA_EnumCallback(hdpa, DeleteAllEnumCallback, NULL);
    DPA_Destroy(hdpa);
    return S_OK;
}
