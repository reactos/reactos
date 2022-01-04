/*
 * IQueryAssociations object and helper functions
 *
 * Copyright 2002 Jon Griffiths
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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 *  IQueryAssociations
 *
 * DESCRIPTION
 *  This object provides a layer of abstraction over the system registry in
 *  order to simplify the process of parsing associations between files.
 *  Associations in this context means the registry entries that link (for
 *  example) the extension of a file with its description, list of
 *  applications to open the file with, and actions that can be performed on it
 *  (the shell displays such information in the context menu of explorer
 *  when you right-click on a file).
 *
 * HELPERS
 * You can use this object transparently by calling the helper functions
 * AssocQueryKeyA(), AssocQueryStringA() and AssocQueryStringByKeyA(). These
 * create an IQueryAssociations object, perform the requested actions
 * and then dispose of the object. Alternatively, you can create an instance
 * of the object using AssocCreate() and call the following methods on it:
 *
 * METHODS
 */

CQueryAssociations::CQueryAssociations() : hkeySource(0), hkeyProgID(0)
{
}

CQueryAssociations::~CQueryAssociations()
{
}

/**************************************************************************
 *  IQueryAssociations_Init
 *
 * Initialise an IQueryAssociations object.
 *
 * PARAMS
 *  cfFlags    [I] ASSOCF_ flags from "shlwapi.h"
 *  pszAssoc   [I] String for the root key name, or NULL if hkeyProgid is given
 *  hkeyProgid [I] Handle for the root key, or NULL if pszAssoc is given
 *  hWnd       [I] Reserved, must be NULL.
 *
 * RETURNS
 *  Success: S_OK. iface is initialised with the parameters given.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::Init(
    ASSOCF cfFlags,
    LPCWSTR pszAssoc,
    HKEY hkeyProgid,
    HWND hWnd)
{
    static const WCHAR szProgID[] = L"ProgID";

    TRACE("(%p)->(%d,%s,%p,%p)\n", this,
                                    cfFlags,
                                    debugstr_w(pszAssoc),
                                    hkeyProgid,
                                    hWnd);

    if (hWnd != NULL)
    {
        FIXME("hwnd != NULL not supported\n");
    }

    if (cfFlags != 0)
    {
        FIXME("unsupported flags: %x\n", cfFlags);
    }

    RegCloseKey(this->hkeySource);
    RegCloseKey(this->hkeyProgID);
    this->hkeySource = this->hkeyProgID = NULL;
    if (pszAssoc != NULL)
    {
        WCHAR *progId;
        HRESULT hr;

        LONG ret = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                            pszAssoc,
                            0,
                            KEY_READ,
                            &this->hkeySource);
        if (ret)
        {
            return S_OK;
        }

        /* if this is a progid */
        if (*pszAssoc != '.' && *pszAssoc != '{')
        {
            this->hkeyProgID = this->hkeySource;
            return S_OK;
        }

        /* if it's not a progid, it's a file extension or clsid */
        if (*pszAssoc == '.')
        {
            /* for a file extension, the progid is the default value */
            hr = this->GetValue(this->hkeySource, NULL, (void**)&progId, NULL);
            if (FAILED(hr))
                return S_OK;
        }
        else /* if (*pszAssoc == '{') */
        {
            HKEY progIdKey;
            /* for a clsid, the progid is the default value of the ProgID subkey */
            ret = RegOpenKeyExW(this->hkeySource,
                                szProgID,
                                0,
                                KEY_READ,
                                &progIdKey);
            if (ret != ERROR_SUCCESS)
                return S_OK;
            hr = this->GetValue(progIdKey, NULL, (void**)&progId, NULL);
            if (FAILED(hr))
                return S_OK;
            RegCloseKey(progIdKey);
        }

        /* open the actual progid key, the one with the shell subkey */
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                            progId,
                            0,
                            KEY_READ,
                            &this->hkeyProgID);
        HeapFree(GetProcessHeap(), 0, progId);

        return S_OK;
    }
    else if (hkeyProgid != NULL)
    {
        /* reopen the key so we don't end up closing a key owned by the caller */
        RegOpenKeyExW(hkeyProgid, NULL, 0, KEY_READ, &this->hkeyProgID);
        this->hkeySource = this->hkeyProgID;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

/**************************************************************************
 *  IQueryAssociations_GetString
 *
 * Get a file association string from the registry.
 *
 * PARAMS
 *  cfFlags  [I]   ASSOCF_ flags from "shlwapi.h"
 *  str      [I]   Type of string to get (ASSOCSTR enum from "shlwapi.h")
 *  pszExtra [I]   Extra information about the string location
 *  pszOut   [O]   Destination for the association string
 *  pcchOut  [I/O] Length of pszOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the string, pcchOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetString(
    ASSOCF flags,
    ASSOCSTR str,
    LPCWSTR pszExtra,
    LPWSTR pszOut,
    DWORD *pcchOut)
{
    const ASSOCF unimplemented_flags = ~ASSOCF_NOTRUNCATE;
    DWORD len = 0;
    HRESULT hr;
    WCHAR path[MAX_PATH];

    TRACE("(%p)->(0x%08x, %u, %s, %p, %p)\n", this, flags, str, debugstr_w(pszExtra), pszOut, pcchOut);
    if (flags & unimplemented_flags)
    {
        FIXME("%08x: unimplemented flags\n", flags & unimplemented_flags);
    }

    if (!pcchOut)
    {
        return E_UNEXPECTED;
    }

    if (!this->hkeySource && !this->hkeyProgID)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    }

    switch (str)
    {
        case ASSOCSTR_COMMAND:
        {
            WCHAR *command;
            hr = this->GetCommand(pszExtra, &command);
            if (SUCCEEDED(hr))
            {
                hr = this->ReturnString(flags, pszOut, pcchOut, command, strlenW(command) + 1);
                HeapFree(GetProcessHeap(), 0, command);
            }
            return hr;
        }
        case ASSOCSTR_EXECUTABLE:
        {
            hr = this->GetExecutable(pszExtra, path, MAX_PATH, &len);
            if (FAILED(hr))
            {
                return hr;
            }
            len++;
            return this->ReturnString(flags, pszOut, pcchOut, path, len);
        }
        case ASSOCSTR_FRIENDLYDOCNAME:
        {
            WCHAR *pszFileType;

            hr = this->GetValue(this->hkeySource, NULL, (void**)&pszFileType, NULL);
            if (FAILED(hr))
            {
                return hr;
            }
            DWORD size = 0;
            DWORD ret = RegGetValueW(HKEY_CLASSES_ROOT, pszFileType, NULL, RRF_RT_REG_SZ, NULL, NULL, &size);
            if (ret == ERROR_SUCCESS)
            {
                WCHAR *docName = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, size));
                if (docName)
                {
                    ret = RegGetValueW(HKEY_CLASSES_ROOT, pszFileType, NULL, RRF_RT_REG_SZ, NULL, docName, &size);
                    if (ret == ERROR_SUCCESS)
                    {
                        hr = this->ReturnString(flags, pszOut, pcchOut, docName, strlenW(docName) + 1);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ret);
                    }
                    HeapFree(GetProcessHeap(), 0, docName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ret);
            }
            HeapFree(GetProcessHeap(), 0, pszFileType);
            return hr;
        }
        case ASSOCSTR_FRIENDLYAPPNAME:
        {
            PVOID verinfoW = NULL;
            DWORD size, retval = 0;
            UINT flen;
            WCHAR *bufW;
            static const WCHAR translationW[] = L"\\VarFileInfo\\Translation";
            static const WCHAR fileDescFmtW[] = L"\\StringFileInfo\\%04x%04x\\FileDescription";
            WCHAR fileDescW[41];

            hr = this->GetExecutable(pszExtra, path, MAX_PATH, &len);
            if (FAILED(hr))
            {
                return hr;
            }
            retval = GetFileVersionInfoSizeW(path, &size);
            if (!retval)
            {
                goto get_friendly_name_fail;
            }
            verinfoW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retval);
            if (!verinfoW)
            {
                return E_OUTOFMEMORY;
            }
            if (!GetFileVersionInfoW(path, 0, retval, verinfoW))
            {
                goto get_friendly_name_fail;
            }
            if (VerQueryValueW(verinfoW, translationW, (LPVOID *)&bufW, &flen))
            {
                UINT i;
                DWORD *langCodeDesc = (DWORD *)bufW;
                for (i = 0; i < flen / sizeof(DWORD); i++)
                {
                    sprintfW(fileDescW, fileDescFmtW, LOWORD(langCodeDesc[i]), HIWORD(langCodeDesc[i]));
                    if (VerQueryValueW(verinfoW, fileDescW, (LPVOID *)&bufW, &flen))
                    {
                        /* Does strlenW(bufW) == 0 mean we use the filename? */
                        len = strlenW(bufW) + 1;
                        TRACE("found FileDescription: %s\n", debugstr_w(bufW));
                        hr = this->ReturnString(flags, pszOut, pcchOut, bufW, len);
                        HeapFree(GetProcessHeap(), 0, verinfoW);
                        return hr;
                    }
                }
            }
        get_friendly_name_fail:
            PathRemoveExtensionW(path);
            PathStripPathW(path);
            TRACE("using filename: %s\n", debugstr_w(path));
            hr = this->ReturnString(flags, pszOut, pcchOut, path, strlenW(path) + 1);
            HeapFree(GetProcessHeap(), 0, verinfoW);
            return hr;
        }
        case ASSOCSTR_CONTENTTYPE:
        {
            static const WCHAR Content_TypeW[] = L"Content Type";

            DWORD size = 0;
            DWORD ret = RegGetValueW(this->hkeySource, NULL, Content_TypeW, RRF_RT_REG_SZ, NULL, NULL, &size);
            if (ret != ERROR_SUCCESS)
            {
                return HRESULT_FROM_WIN32(ret);
            }
            WCHAR *contentType = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, size));
            if (contentType != NULL)
            {
                ret = RegGetValueW(this->hkeySource, NULL, Content_TypeW, RRF_RT_REG_SZ, NULL, contentType, &size);
                if (ret == ERROR_SUCCESS)
                {
                    hr = this->ReturnString(flags, pszOut, pcchOut, contentType, strlenW(contentType) + 1);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ret);
                }
                HeapFree(GetProcessHeap(), 0, contentType);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            return hr;
        }
        case ASSOCSTR_DEFAULTICON:
        {
            static const WCHAR DefaultIconW[] = L"DefaultIcon";
            DWORD ret;
            DWORD size = 0;
            ret = RegGetValueW(this->hkeyProgID, DefaultIconW, NULL, RRF_RT_REG_SZ, NULL, NULL, &size);
            if (ret == ERROR_SUCCESS)
            {
                WCHAR *icon = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, size));
                if (icon)
                {
                    ret = RegGetValueW(this->hkeyProgID, DefaultIconW, NULL, RRF_RT_REG_SZ, NULL, icon, &size);
                    if (ret == ERROR_SUCCESS)
                    {
                        hr = this->ReturnString(flags, pszOut, pcchOut, icon, strlenW(icon) + 1);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ret);
                    }
                    HeapFree(GetProcessHeap(), 0, icon);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ret);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ret);
            }
            return hr;
        }
        case ASSOCSTR_SHELLEXTENSION:
        {
            static const WCHAR shellexW[] = L"ShellEx\\";
            WCHAR keypath[sizeof(shellexW) / sizeof(shellexW[0]) + 39], guid[39];
            CLSID clsid;
            HKEY hkey;

            hr = CLSIDFromString(pszExtra, &clsid);
            if (FAILED(hr))
            {
                return hr;
            }
            strcpyW(keypath, shellexW);
            strcatW(keypath, pszExtra);
            LONG ret = RegOpenKeyExW(this->hkeySource, keypath, 0, KEY_READ, &hkey);
            if (ret)
            {
                return HRESULT_FROM_WIN32(ret);
            }
            DWORD size = sizeof(guid);
            ret = RegGetValueW(hkey, NULL, NULL, RRF_RT_REG_SZ, NULL, guid, &size);
            RegCloseKey(hkey);
            if (ret)
            {
                return HRESULT_FROM_WIN32(ret);
            }
            return this->ReturnString(flags, pszOut, pcchOut, guid, size / sizeof(WCHAR));
        }

        default:
        {
            FIXME("assocstr %d unimplemented!\n", str);
            return E_NOTIMPL;
        }
    }
}

/**************************************************************************
 *  IQueryAssociations_GetKey
 *
 * Get a file association key from the registry.
 *
 * PARAMS
 *  cfFlags  [I] ASSOCF_ flags from "shlwapi.h"
 *  assockey [I] Type of key to get (ASSOCKEY enum from "shlwapi.h")
 *  pszExtra [I] Extra information about the key location
 *  phkeyOut [O] Destination for the association key
 *
 * RETURNS
 *  Success: S_OK. phkeyOut contains a handle to the key.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetKey(
    ASSOCF cfFlags,
    ASSOCKEY assockey,
    LPCWSTR pszExtra,
    HKEY *phkeyOut)
{
    FIXME("(%p,0x%8x,0x%8x,%s,%p)-stub!\n", this, cfFlags, assockey,
            debugstr_w(pszExtra), phkeyOut);
    return E_NOTIMPL;
}

/**************************************************************************
 *  IQueryAssociations_GetData
 *
 * Get the data for a file association key from the registry.
 *
 * PARAMS
 *  cfFlags   [I]   ASSOCF_ flags from "shlwapi.h"
 *  assocdata [I]   Type of data to get (ASSOCDATA enum from "shlwapi.h")
 *  pszExtra  [I]   Extra information about the data location
 *  pvOut     [O]   Destination for the association key
 *  pcbOut    [I/O] Size of pvOut
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the data, pcbOut contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetData(ASSOCF cfFlags, ASSOCDATA assocdata, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut)
{
    static const WCHAR edit_flags[] = L"EditFlags";

    TRACE("(%p,0x%8x,0x%8x,%s,%p,%p)\n", this, cfFlags, assocdata,
            debugstr_w(pszExtra), pvOut, pcbOut);

    if(cfFlags)
    {
        FIXME("Unsupported flags: %x\n", cfFlags);
    }

    switch(assocdata)
    {
        case ASSOCDATA_EDITFLAGS:
        {
            if(!this->hkeyProgID)
            {
                return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
            }

            void *data;
            DWORD size;
            HRESULT hres = this->GetValue(this->hkeyProgID, edit_flags, &data, &size);
            if(FAILED(hres))
            {
                return hres;
            }

            if (!pcbOut)
            {
                HeapFree(GetProcessHeap(), 0, data);
                return hres;
            }

            hres = this->ReturnData(pvOut, pcbOut, data, size);
            HeapFree(GetProcessHeap(), 0, data);
            return hres;
        }
        default:
        {
            FIXME("Unsupported ASSOCDATA value: %d\n", assocdata);
            return E_NOTIMPL;
        }
    }
}

/**************************************************************************
 *  IQueryAssociations_GetEnum
 *
 * Not yet implemented in native Win32.
 *
 * PARAMS
 *  cfFlags   [I] ASSOCF_ flags from "shlwapi.h"
 *  assocenum [I] Type of enum to get (ASSOCENUM enum from "shlwapi.h")
 *  pszExtra  [I] Extra information about the enum location
 *  riid      [I] REFIID to look for
 *  ppvOut    [O] Destination for the interface.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Presumably this function returns an enumerator object.
 */
HRESULT STDMETHODCALLTYPE CQueryAssociations::GetEnum(
    ASSOCF cfFlags,
    ASSOCENUM assocenum,
    LPCWSTR pszExtra,
    REFIID riid,
    LPVOID *ppvOut)
{
    return E_NOTIMPL;
}

HRESULT CQueryAssociations::GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size)
{
    DWORD size;
    LONG ret;

    ret = RegQueryValueExW(hkey, name, 0, NULL, NULL, &size);
    if (ret != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(ret);
    }
    if (!size)
    {
        return E_FAIL;
    }
    *data = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*data)
    {
        return E_OUTOFMEMORY;
    }
    ret = RegQueryValueExW(hkey, name, 0, NULL, (LPBYTE)*data, &size);
    if (ret != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, *data);
        return HRESULT_FROM_WIN32(ret);
    }
    if(data_size)
    {
        *data_size = size;
    }
    return S_OK;
}

HRESULT CQueryAssociations::GetCommand(const WCHAR *extra, WCHAR **command)
{
    HKEY hkeyCommand;
    HKEY hkeyShell;
    HKEY hkeyVerb;
    HRESULT hr;
    LONG ret;
    WCHAR *extra_from_reg = NULL;
    WCHAR *filetype;
    static const WCHAR commandW[] = L"command";
    static const WCHAR shellW[] = L"shell";

    /* When looking for file extension it's possible to have a default value
     that points to another key that contains 'shell/<verb>/command' subtree. */
    hr = this->GetValue(this->hkeySource, NULL, (void**)&filetype, NULL);
    if (hr == S_OK)
    {
        HKEY hkeyFile;

        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, filetype, 0, KEY_READ, &hkeyFile);
        HeapFree(GetProcessHeap(), 0, filetype);

        if (ret == ERROR_SUCCESS)
        {
            ret = RegOpenKeyExW(hkeyFile, shellW, 0, KEY_READ, &hkeyShell);
            RegCloseKey(hkeyFile);
        }
        else
        {
            ret = RegOpenKeyExW(this->hkeySource, shellW, 0, KEY_READ, &hkeyShell);
        }
    }
    else
    {
        ret = RegOpenKeyExW(this->hkeySource, shellW, 0, KEY_READ, &hkeyShell);
    }

    if (ret)
    {
        return HRESULT_FROM_WIN32(ret);
    }

    if (!extra)
    {
        /* check for default verb */
        hr = this->GetValue(hkeyShell, NULL, (void**)&extra_from_reg, NULL);
        if (FAILED(hr))
        {
            /* no default verb, try first subkey */
            DWORD max_subkey_len;

            ret = RegQueryInfoKeyW(hkeyShell, NULL, NULL, NULL, NULL, &max_subkey_len, NULL, NULL, NULL, NULL, NULL, NULL);
            if (ret)
            {
                RegCloseKey(hkeyShell);
                return HRESULT_FROM_WIN32(ret);
            }

            max_subkey_len++;
            extra_from_reg = static_cast<WCHAR*>(HeapAlloc(GetProcessHeap(), 0, max_subkey_len * sizeof(WCHAR)));
            if (!extra_from_reg)
            {
                RegCloseKey(hkeyShell);
                return E_OUTOFMEMORY;
            }

            ret = RegEnumKeyExW(hkeyShell, 0, extra_from_reg, &max_subkey_len, NULL, NULL, NULL, NULL);
            if (ret)
            {
                HeapFree(GetProcessHeap(), 0, extra_from_reg);
                RegCloseKey(hkeyShell);
                return HRESULT_FROM_WIN32(ret);
            }
        }
        extra = extra_from_reg;
    }

    /* open verb subkey */
    ret = RegOpenKeyExW(hkeyShell, extra, 0, KEY_READ, &hkeyVerb);
    HeapFree(GetProcessHeap(), 0, extra_from_reg);
    RegCloseKey(hkeyShell);
    if (ret)
    {
        return HRESULT_FROM_WIN32(ret);
    }
    /* open command subkey */
    ret = RegOpenKeyExW(hkeyVerb, commandW, 0, KEY_READ, &hkeyCommand);
    RegCloseKey(hkeyVerb);
    if (ret)
    {
        return HRESULT_FROM_WIN32(ret);
    }
    hr = this->GetValue(hkeyCommand, NULL, (void**)command, NULL);
    RegCloseKey(hkeyCommand);
    return hr;
}

HRESULT CQueryAssociations::GetExecutable(LPCWSTR pszExtra, LPWSTR path, DWORD pathlen, DWORD *len)
{
    WCHAR *pszCommand;
    WCHAR *pszStart;
    WCHAR *pszEnd;

    HRESULT hr = this->GetCommand(pszExtra, &pszCommand);
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD expLen = ExpandEnvironmentStringsW(pszCommand, NULL, 0);
    if (expLen > 0)
    {
        expLen++;
        WCHAR *buf = static_cast<WCHAR *>(HeapAlloc(GetProcessHeap(), 0, expLen * sizeof(WCHAR)));
        ExpandEnvironmentStringsW(pszCommand, buf, expLen);
        HeapFree(GetProcessHeap(), 0, pszCommand);
        pszCommand = buf;
    }

    /* cleanup pszCommand */
    if (pszCommand[0] == '"')
    {
        pszStart = pszCommand + 1;
        pszEnd = strchrW(pszStart, '"');
        if (pszEnd)
        {
            *pszEnd = 0;
        }
        *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
    }
    else
    {
        pszStart = pszCommand;
        for (pszEnd = pszStart; (pszEnd = strchrW(pszEnd, ' ')); pszEnd++)
        {
            WCHAR c = *pszEnd;
            *pszEnd = 0;
            if ((*len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL)))
            {
                break;
            }
            *pszEnd = c;
        }
        if (!pszEnd)
        {
            *len = SearchPathW(NULL, pszStart, NULL, pathlen, path, NULL);
        }
    }

    HeapFree(GetProcessHeap(), 0, pszCommand);
    if (!*len)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    return S_OK;
}

HRESULT CQueryAssociations::ReturnData(void *out, DWORD *outlen, const void *data, DWORD datalen)
{
    if (out)
    {
        if (*outlen < datalen)
        {
            *outlen = datalen;
            return E_POINTER;
        }
        *outlen = datalen;
        memcpy(out, data, datalen);
        return S_OK;
    }
    else
    {
        *outlen = datalen;
        return S_FALSE;
    }
}

HRESULT CQueryAssociations::ReturnString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPCWSTR data, DWORD datalen)
{
    HRESULT hr = S_OK;
    DWORD len;

    TRACE("flags=0x%08x, data=%s\n", flags, debugstr_w(data));

    if (!out)
    {
        *outlen = datalen;
        return S_FALSE;
    }

    if (*outlen < datalen)
    {
        if (flags & ASSOCF_NOTRUNCATE)
        {
            len = 0;
            if (*outlen > 0) out[0] = 0;
            hr = E_POINTER;
        }
        else
        {
            len = min(*outlen, datalen);
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        *outlen = datalen;
    }
    else
    {
        len = datalen;
    }

    if (len)
    {
        memcpy(out, data, len*sizeof(WCHAR));
    }

    return hr;
}