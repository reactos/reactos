/*
 * ReactOS ATL
 *
 * Copyright 2005 Jacek Caban
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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

#pragma once

class IRegistrarBase : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE AddReplacement(LPCOLESTR key, LPCOLESTR item) = 0;
    virtual HRESULT STDMETHODCALLTYPE ClearReplacements() = 0;
};

namespace ATL
{

class CRegObject : public IRegistrarBase
{
public:
    typedef struct rep_list_str
    {
        LPOLESTR key;
        LPOLESTR item;
        int key_len;
        struct rep_list_str *next;
    } rep_list;

    typedef struct
    {
        LPOLESTR str;
        DWORD alloc;
        DWORD len;
    } strbuf;

    rep_list *m_rep;

public:
    CRegObject()
    {
        m_rep = NULL;
    }

    ~CRegObject()
    {
        HRESULT hResult;

        hResult = ClearReplacements();
        ATLASSERT(SUCCEEDED(hResult));
        (void)hResult;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID & /* riid */, void ** /* ppvObject */ )
    {
        ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
        return E_NOTIMPL;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
        return 1;
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
        return 0;
    }

    HRESULT STDMETHODCALLTYPE AddReplacement(LPCOLESTR key, LPCOLESTR item)
    {
        int len;
        rep_list *new_rep;

        new_rep = reinterpret_cast<rep_list *>(HeapAlloc(GetProcessHeap(), 0, sizeof(rep_list)));
        if (new_rep == NULL)
            return E_OUTOFMEMORY;

        new_rep->key_len  = lstrlenW(key);
        new_rep->key = reinterpret_cast<OLECHAR *>(HeapAlloc(GetProcessHeap(), 0, (new_rep->key_len + 1) * sizeof(OLECHAR)));
        if (new_rep->key == NULL)
        {
            HeapFree(GetProcessHeap(), 0, new_rep);
            return E_OUTOFMEMORY;
        }
        memcpy(new_rep->key, key, (new_rep->key_len + 1) * sizeof(OLECHAR));

        len = lstrlenW(item) + 1;
        new_rep->item = reinterpret_cast<OLECHAR *>(HeapAlloc(GetProcessHeap(), 0, len * sizeof(OLECHAR)));
        if (new_rep->item == NULL)
        {
            HeapFree(GetProcessHeap(), 0, new_rep->key);
            HeapFree(GetProcessHeap(), 0, new_rep);
            return E_OUTOFMEMORY;
        }
        memcpy(new_rep->item, item, len * sizeof(OLECHAR));

        new_rep->next = m_rep;
        m_rep = new_rep;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ClearReplacements()
    {
        rep_list *iter;
        rep_list *iter2;

        iter = m_rep;
        while (iter)
        {
            iter2 = iter->next;
            HeapFree(GetProcessHeap(), 0, iter->key);
            HeapFree(GetProcessHeap(), 0, iter->item);
            HeapFree(GetProcessHeap(), 0, iter);
            iter = iter2;
        }

        m_rep = NULL;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ResourceRegisterSz(LPCOLESTR resFileName, LPCOLESTR szID, LPCOLESTR szType)
    {
        return RegisterWithResource(resFileName, szID, szType, TRUE);
    }

    HRESULT STDMETHODCALLTYPE ResourceUnregisterSz(LPCOLESTR resFileName, LPCOLESTR szID, LPCOLESTR szType)
    {
        return RegisterWithResource(resFileName, szID, szType, FALSE);
    }

    HRESULT STDMETHODCALLTYPE FileRegister(LPCOLESTR fileName)
    {
        return RegisterWithFile(fileName, TRUE);
    }

    HRESULT STDMETHODCALLTYPE FileUnregister(LPCOLESTR fileName)
    {
        return RegisterWithFile(fileName, FALSE);
    }

    HRESULT STDMETHODCALLTYPE StringRegister(LPCOLESTR data)
    {
        return RegisterWithString(data, TRUE);
    }

    HRESULT STDMETHODCALLTYPE StringUnregister(LPCOLESTR data)
    {
        return RegisterWithString(data, FALSE);
    }

    HRESULT STDMETHODCALLTYPE ResourceRegister(LPCOLESTR resFileName, UINT nID, LPCOLESTR szType)
    {
        return ResourceRegisterSz(resFileName, MAKEINTRESOURCEW(nID), szType);
    }

    HRESULT STDMETHODCALLTYPE ResourceUnregister(LPCOLESTR resFileName, UINT nID, LPCOLESTR szType)
    {
        return ResourceUnregisterSz(resFileName, MAKEINTRESOURCEW(nID), szType);
    }

protected:
    HRESULT STDMETHODCALLTYPE RegisterWithResource(LPCOLESTR resFileName, LPCOLESTR szID, LPCOLESTR szType, BOOL doRegister)
    {
        return resource_register(resFileName, szID, szType, doRegister);
    }

    HRESULT STDMETHODCALLTYPE RegisterWithFile(LPCOLESTR fileName, BOOL doRegister)
    {
        return file_register(fileName, doRegister);
    }

    HRESULT STDMETHODCALLTYPE RegisterWithString(LPCOLESTR data, BOOL doRegister)
    {
        return string_register(data, doRegister);
    }

private:
    inline LONG RegDeleteTreeX(HKEY parentKey, LPCTSTR subKeyName)
    {
        wchar_t szBuffer[256];
        DWORD dwSize;
        FILETIME time;
        HKEY childKey;
        LONG lRes;

        ATLASSERT(parentKey != NULL);
        lRes = RegOpenKeyEx(parentKey, subKeyName, 0, KEY_READ | KEY_WRITE, &childKey);
        if (lRes != ERROR_SUCCESS)
            return lRes;

        dwSize = sizeof(szBuffer) / sizeof(szBuffer[0]);
        while (RegEnumKeyExW(parentKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time) == ERROR_SUCCESS)
        {
            lRes = RegDeleteTreeX(childKey, szBuffer);
            if (lRes != ERROR_SUCCESS)
                return lRes;
            dwSize = sizeof(szBuffer) / sizeof(szBuffer[0]);
        }
        RegCloseKey(childKey);
        return RegDeleteKey(parentKey, subKeyName);
    }

    HRESULT strbuf_init(strbuf *buf)
    {
        buf->str = reinterpret_cast<LPOLESTR>(HeapAlloc(GetProcessHeap(), 0, 128 * sizeof(WCHAR)));
        if (buf->str == NULL)
            return E_OUTOFMEMORY;
        buf->alloc = 128;
        buf->len = 0;
        return S_OK;
    }

    HRESULT strbuf_write(LPCOLESTR str, strbuf *buf, int len)
    {
        LPOLESTR newBuffer;

        if (len == -1)
            len = lstrlenW(str);
        if (buf->len + len + 1 >= buf->alloc)
        {
            buf->alloc = (buf->len + len) * 2;
            newBuffer = reinterpret_cast<LPOLESTR>(HeapReAlloc(GetProcessHeap(), 0, buf->str, buf->alloc * sizeof(WCHAR)));
            if (newBuffer == NULL)
                return E_OUTOFMEMORY;
            buf->str = newBuffer;
        }
        memcpy(buf->str + buf->len, str, len * sizeof(OLECHAR));
        buf->len += len;
        buf->str[buf->len] = '\0';
        return S_OK;
    }


    HRESULT file_register(LPCOLESTR fileName, BOOL do_register)
    {
        HANDLE file;
        DWORD filelen;
        DWORD len;
        LPWSTR regstrw;
        LPSTR regstra;
        LRESULT lres;
        HRESULT hResult;

        file = CreateFileW(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (file != INVALID_HANDLE_VALUE)
        {
            filelen = GetFileSize(file, NULL);
            regstra = reinterpret_cast<LPSTR>(HeapAlloc(GetProcessHeap(), 0, filelen));
            if (regstra == NULL)
                return E_OUTOFMEMORY;
            lres = ReadFile(file, regstra, filelen, NULL, NULL);
            if (lres == ERROR_SUCCESS)
            {
                len = MultiByteToWideChar(CP_ACP, 0, regstra, filelen, NULL, 0) + 1;
                regstrw = reinterpret_cast<LPWSTR>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR)));
                if (regstrw == NULL)
                {
                    HeapFree(GetProcessHeap(), 0, regstra);
                    return E_OUTOFMEMORY;
                }
                MultiByteToWideChar(CP_ACP, 0, regstra, filelen, regstrw, len);
                regstrw[len - 1] = '\0';

                hResult = string_register(regstrw, do_register);

                HeapFree(GetProcessHeap(), 0, regstrw);
            }
            else
            {
                hResult = HRESULT_FROM_WIN32(lres);
            }
            HeapFree(GetProcessHeap(), 0, regstra);
            CloseHandle(file);
        }
        else
        {
            hResult = HRESULT_FROM_WIN32(GetLastError());
        }

        return hResult;
    }

    HRESULT resource_register(LPCOLESTR resFileName, LPCOLESTR szID, LPCOLESTR szType, BOOL do_register)
    {
        HINSTANCE hins;
        HRSRC src;
        HGLOBAL regstra;
        LPWSTR regstrw;
        DWORD len;
        DWORD reslen;
        HRESULT hResult;

        hins = LoadLibraryExW(resFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hins)
        {
            src = FindResourceW(hins, szID, szType);
            if (src)
            {
                regstra = LoadResource(hins, src);
                reslen = SizeofResource(hins, src);
                if (regstra)
                {
                    len = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<LPCSTR>(regstra), reslen, NULL, 0) + 1;
                    regstrw = reinterpret_cast<LPWSTR>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR)));
                    if (regstrw == NULL)
                        return E_OUTOFMEMORY;
                    MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<LPCSTR>(regstra), reslen, regstrw, len);
                    regstrw[len - 1] = '\0';

                    hResult = string_register(regstrw, do_register);

                    HeapFree(GetProcessHeap(), 0, regstrw);
                }
                else
                    hResult = HRESULT_FROM_WIN32(GetLastError());
            }
            else
                hResult = HRESULT_FROM_WIN32(GetLastError());
            FreeLibrary(hins);
        }
        else
            hResult = HRESULT_FROM_WIN32(GetLastError());

        return hResult;
    }

    HRESULT string_register(LPCOLESTR data, BOOL do_register)
    {
        strbuf buf;
        HRESULT hResult;

        hResult = strbuf_init(&buf);
        if (FAILED(hResult))
            return hResult;
        hResult = do_preprocess(data, &buf);
        if (SUCCEEDED(hResult))
        {
            hResult = do_process_root_key(buf.str, do_register);
            if (FAILED(hResult) && do_register)
                do_process_root_key(buf.str, FALSE);
        }

        HeapFree(GetProcessHeap(), 0, buf.str);
        return hResult;
    }

    HRESULT do_preprocess(LPCOLESTR data, strbuf *buf)
    {
        LPCOLESTR iter;
        LPCOLESTR iter2;
        rep_list *rep_iter;
        HRESULT hResult;

        iter2 = data;
        iter = wcschr(data, '%');
        while (iter)
        {
            hResult = strbuf_write(iter2, buf, static_cast<int>(iter - iter2));
            if (FAILED(hResult))
                return hResult;

            iter2 = ++iter;
            if (!*iter2)
                return DISP_E_EXCEPTION;
            iter = wcschr(iter2, '%');
            if (!iter)
                return DISP_E_EXCEPTION;

            if (iter == iter2)
            {
                hResult = strbuf_write(_T("%"), buf, 1);
                if (FAILED(hResult))
                    return hResult;
            }
            else
            {
                for (rep_iter = m_rep; rep_iter; rep_iter = rep_iter->next)
                {
                    if (rep_iter->key_len == iter - iter2 && !_memicmp(iter2, rep_iter->key, rep_iter->key_len * sizeof(wchar_t)))
                        break;
                }
                if (!rep_iter)
                    return DISP_E_EXCEPTION;

                hResult = strbuf_write(rep_iter->item, buf, -1);
                if (FAILED(hResult))
                    return hResult;
            }

            iter2 = ++iter;
            iter = wcschr(iter, '%');
        }

        hResult = strbuf_write(iter2, buf, -1);
        if (FAILED(hResult))
            return hResult;

        return S_OK;
    }

    HRESULT get_word(LPCOLESTR *str, strbuf *buf)
    {
        LPCOLESTR iter;
        LPCOLESTR iter2;
        HRESULT hResult;

        iter2 = *str;
        buf->len = 0;
        buf->str[0] = '\0';

        while (iswspace (*iter2))
            iter2++;
        iter = iter2;
        if (!*iter)
        {
            *str = iter;
            return S_OK;
        }

        if (*iter == '}' || *iter == '=')
        {
            hResult = strbuf_write(iter++, buf, 1);
            if (FAILED(hResult))
                return hResult;
        }
        else if (*iter == '\'')
        {
            iter2 = ++iter;
            iter = wcschr(iter, '\'');
            if (!iter)
            {
                *str = iter;
                return DISP_E_EXCEPTION;
            }
            hResult = strbuf_write(iter2, buf, static_cast<int>(iter - iter2));
            if (FAILED(hResult))
                return hResult;
            iter++;
        }
        else
        {
            while (*iter && !iswspace(*iter))
                iter++;
            hResult = strbuf_write(iter2, buf, static_cast<int>(iter - iter2));
            if (FAILED(hResult))
                return hResult;
        }

        while (iswspace(*iter))
            iter++;
        *str = iter;
        return S_OK;
    }

    inline unsigned int HexToBin(OLECHAR a)
    {
        if (a >= '0' && a <= '9')
            return a - '0';
        if (a >= 'A' && a <= 'F')
            return a - 'A' + 10;
        if (a >= 'a' && a <= 'f')
            return a - 'a' + 10;
        ATLASSERT(false);
        return 0;
    }

    HRESULT do_process_key(LPCOLESTR *pstr, HKEY parent_key, strbuf *buf, BOOL do_register)
    {
        LPCOLESTR iter;
        HRESULT hres;
        LONG lres;
        HKEY hkey;
        strbuf name;

        enum {
            NORMAL,
            NO_REMOVE,
            IS_VAL,
            FORCE_REMOVE,
            DO_DELETE
        } key_type = NORMAL;

        static const wchar_t *wstrNoRemove = _T("NoRemove");
        static const wchar_t *wstrForceRemove = _T("ForceRemove");
        static const wchar_t *wstrDelete = _T("Delete");
        static const wchar_t *wstrval = _T("val");

        iter = *pstr;
        hkey = NULL;
        hres = get_word(&iter, buf);
        if (FAILED(hres))
            return hres;
        hres = strbuf_init(&name);
        if (FAILED(hres))
            return hres;

        while(buf->str[1] || buf->str[0] != '}')
        {
            key_type = NORMAL;
            if (!lstrcmpiW(buf->str, wstrNoRemove))
                key_type = NO_REMOVE;
            else if (!lstrcmpiW(buf->str, wstrForceRemove))
                key_type = FORCE_REMOVE;
            else if (!lstrcmpiW(buf->str, wstrval))
                key_type = IS_VAL;
            else if (!lstrcmpiW(buf->str, wstrDelete))
                key_type = DO_DELETE;

            if (key_type != NORMAL)
            {
                hres = get_word(&iter, buf);
                if (FAILED(hres))
                    break;
            }

            if (do_register)
            {
                if (key_type == IS_VAL)
                {
                    hkey = parent_key;
                    hres = strbuf_write(buf->str, &name, -1);
                    if (FAILED(hres))
                        return hres;
                }
                else if (key_type == DO_DELETE)
                {
                    RegDeleteTreeX(parent_key, buf->str);
                }
                else
                {
                    if (key_type == FORCE_REMOVE)
                        RegDeleteTreeX(parent_key, buf->str);
                    lres = RegCreateKey(parent_key, buf->str, &hkey);
                    if (lres != ERROR_SUCCESS)
                    {
                        hres = HRESULT_FROM_WIN32(lres);
                        break;
                    }
                }
            }
            else if (key_type != IS_VAL && key_type != DO_DELETE)
            {
                hres = strbuf_write(buf->str, &name, -1);
                if (FAILED(hres))
                    return hres;
                lres = RegOpenKey(parent_key, buf->str, &hkey);
                if (lres != ERROR_SUCCESS)
                {
                }
            }

            if (key_type != DO_DELETE && *iter == '=')
            {
                iter++;
                hres = get_word(&iter, buf);
                if (FAILED(hres))
                    break;
                if (buf->len != 1)
                {
                    hres = DISP_E_EXCEPTION;
                    break;
                }
                if (do_register)
                {
                    switch(buf->str[0])
                    {
                        case 's':
                            hres = get_word(&iter, buf);
                            if (FAILED(hres))
                                break;
                            lres = RegSetValueEx(hkey, name.len ? name.str :  NULL, 0, REG_SZ, (PBYTE)buf->str,
                                    (lstrlenW(buf->str) + 1) * sizeof(WCHAR));
                            if (lres != ERROR_SUCCESS)
                                hres = HRESULT_FROM_WIN32(lres);
                            break;
                        case 'e':
                            hres = get_word(&iter, buf);
                            if (FAILED(hres))
                                break;
                            lres = RegSetValueEx(hkey, name.len ? name.str :  NULL, 0, REG_EXPAND_SZ, (PBYTE)buf->str,
                                    (lstrlenW(buf->str) + 1) * sizeof(WCHAR));
                            if (lres != ERROR_SUCCESS)
                                hres = HRESULT_FROM_WIN32(lres);
                            break;
                        case 'd':
                            {
                                hres = get_word(&iter, buf);
                                if (FAILED(hres))
                                    break;
                                WCHAR *end;
                                DWORD dw;
                                if ((buf->str[0] == '0' && buf->str[1] == 'x') || (buf->str[0] == '&' && buf->str[1] == 'H'))
                                    dw = wcstoul(&buf->str[2], &end, 16);
                                else
                                    dw = wcstol(&buf->str[0], &end, 10);
                                lres = RegSetValueEx(hkey, name.len ? name.str :  NULL, 0, REG_DWORD, (PBYTE)&dw, sizeof(dw));
                                if (lres != ERROR_SUCCESS)
                                    hres = HRESULT_FROM_WIN32(lres);
                                break;
                            }
                        case 'b':
                            {
                                DWORD            count;
                                DWORD            curIndex;

                                hres = get_word(&iter, buf);
                                if (FAILED(hres))
                                    break;
                                count = buf->len;
                                if ((count & 1) != 0)
                                    return DISP_E_EXCEPTION;
                                count = count / 2;
                                for (curIndex = 0; curIndex < count; curIndex++)
                                    ((BYTE*)buf->str)[curIndex] = (HexToBin(buf->str[curIndex * 2]) << 4) | HexToBin(buf->str[curIndex * 2 + 1]);
                                lres = RegSetValueEx(hkey, name.len ? name.str :  NULL, 0, REG_BINARY, (PBYTE)buf->str, count);
                                if (lres != ERROR_SUCCESS)
                                    hres = HRESULT_FROM_WIN32(lres);
                                break;
                            }
                        default:
                            hres = DISP_E_EXCEPTION;
                    }
                    if (FAILED(hres))
                        break;
                }
                else
                {
                    if (*iter == '-')
                        iter++;
                    hres = get_word(&iter, buf);
                    if (FAILED(hres))
                        break;
                }
            }
            else if(key_type == IS_VAL)
            {
                hres = DISP_E_EXCEPTION;
                break;
            }

            if (key_type != IS_VAL && key_type != DO_DELETE && *iter == '{' && iswspace(iter[1]))
            {
                hres = get_word(&iter, buf);
                if (FAILED(hres))
                    break;
                hres = do_process_key(&iter, hkey, buf, do_register);
                if (FAILED(hres))
                    break;
            }

            if (!do_register && (key_type == NORMAL || key_type == FORCE_REMOVE))
            {
                RegDeleteKey(parent_key, name.str);
            }

            if (hkey && key_type != IS_VAL)
                RegCloseKey(hkey);
            hkey = 0;
            name.len = 0;

            hres = get_word(&iter, buf);
            if (FAILED(hres))
                break;
        }

        HeapFree(GetProcessHeap(), 0, name.str);
        if (hkey && key_type != IS_VAL)
            RegCloseKey(hkey);
        *pstr = iter;
        return hres;
    }

    HRESULT do_process_root_key(LPCOLESTR data, BOOL do_register)
    {
        LPCOLESTR iter;
        strbuf buf;
        unsigned int i;
        HRESULT hResult;
        static const struct {
            const wchar_t *name;
            HKEY key;
        } root_keys[] = {
            {_T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
            {_T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
            {_T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
            {_T("HKEY_USERS"), HKEY_USERS},
            {_T("HKEY_PERFORMANCE_DATA"), HKEY_PERFORMANCE_DATA},
            {_T("HKEY_DYN_DATA"), HKEY_DYN_DATA},
            {_T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG},
            {_T("HKCR"), HKEY_CLASSES_ROOT},
            {_T("HKCU"), HKEY_CURRENT_USER},
            {_T("HKLM"), HKEY_LOCAL_MACHINE},
            {_T("HKU"), HKEY_USERS},
            {_T("HKPD"), HKEY_PERFORMANCE_DATA},
            {_T("HKDD"), HKEY_DYN_DATA},
            {_T("HKCC"), HKEY_CURRENT_CONFIG},
        };

        iter = data;

        hResult = strbuf_init(&buf);
        if (FAILED(hResult))
            return hResult;
        hResult = get_word(&iter, &buf);
        if (FAILED(hResult))
            return hResult;

        while (*iter)
        {
            if (!buf.len)
            {
                hResult = DISP_E_EXCEPTION;
                break;
            }
            for (i = 0; i < sizeof(root_keys) / sizeof(root_keys[0]); i++)
            {
                if (!lstrcmpiW(buf.str, root_keys[i].name))
                    break;
            }
            if (i == sizeof(root_keys) / sizeof(root_keys[0]))
            {
                hResult = DISP_E_EXCEPTION;
                break;
            }
            hResult = get_word(&iter, &buf);
            if (FAILED(hResult))
                break;
            if (buf.str[1] || buf.str[0] != '{')
            {
                hResult = DISP_E_EXCEPTION;
                break;
            }
            hResult = do_process_key(&iter, root_keys[i].key, &buf, do_register);
            if (FAILED(hResult))
                break;
            hResult = get_word(&iter, &buf);
            if (FAILED(hResult))
                break;
        }
        HeapFree(GetProcessHeap(), 0, buf.str);
        return hResult;
    }

};

}; //namespace ATL
