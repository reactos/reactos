/*
 * Copyright 2005 Jacek Caban
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

#include "atlbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

/**************************************************************
 * ATLRegistrar implementation
 */

static const struct {
    WCHAR name[22];
    HKEY  key;
} root_keys[] = {
    {L"HKEY_CLASSES_ROOT",     HKEY_CLASSES_ROOT},
    {L"HKEY_CURRENT_USER",     HKEY_CURRENT_USER},
    {L"HKEY_LOCAL_MACHINE",    HKEY_LOCAL_MACHINE},
    {L"HKEY_USERS",            HKEY_USERS},
    {L"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA},
    {L"HKEY_DYN_DATA",         HKEY_DYN_DATA},
    {L"HKEY_CURRENT_CONFIG",   HKEY_CURRENT_CONFIG},
    {L"HKCR",                  HKEY_CLASSES_ROOT},
    {L"HKCU",                  HKEY_CURRENT_USER},
    {L"HKLM",                  HKEY_LOCAL_MACHINE},
    {L"HKU",                   HKEY_USERS},
    {L"HKPD",                  HKEY_PERFORMANCE_DATA},
    {L"HKDD",                  HKEY_DYN_DATA},
    {L"HKCC",                  HKEY_CURRENT_CONFIG}
};

typedef struct rep_list_str {
    LPOLESTR key;
    LPOLESTR item;
    int key_len;
    struct rep_list_str *next;
} rep_list;

typedef struct {
    IRegistrar IRegistrar_iface;
    LONG ref;
    rep_list *rep;
} Registrar;

typedef struct {
    LPOLESTR str;
    DWORD alloc;
    DWORD len;
} strbuf;

static inline Registrar *impl_from_IRegistrar(IRegistrar *iface)
{
    return CONTAINING_RECORD(iface, Registrar, IRegistrar_iface);
}

static void strbuf_init(strbuf *buf)
{
    buf->str = malloc(128*sizeof(WCHAR));
    buf->alloc = 128;
    buf->len = 0;
}

static void strbuf_write(LPCOLESTR str, strbuf *buf, int len)
{
    if(len == -1)
        len = lstrlenW(str);
    if(buf->len+len+1 >= buf->alloc) {
        buf->alloc = (buf->len+len)<<1;
        buf->str = realloc(buf->str, buf->alloc*sizeof(WCHAR));
    }
    memcpy(buf->str+buf->len, str, len*sizeof(OLECHAR));
    buf->len += len;
    buf->str[buf->len] = '\0';
}

static int xdigit_to_int(WCHAR c)
{
    if('0' <= c && c <= '9') return c - '0';
    if('a' <= c && c <= 'f') return c - 'a' + 10;
    if('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

static HRESULT get_word(LPCOLESTR *str, strbuf *buf)
{
    LPCOLESTR iter, iter2 = *str;

    buf->len = 0;
    buf->str[0] = '\0';

    while(iswspace(*iter2))
        iter2++;
    iter = iter2;
    if(!*iter) {
        *str = iter;
        return S_OK;
    }

    if(*iter == '}' || *iter == '=') {
        strbuf_write(iter++, buf, 1);
    }else if(*iter == '\'') {
        for (;;)
        {
            iter2 = ++iter;
            iter = wcschr(iter, '\'');
            if(!iter) {
                WARN("Unexpected end of script\n");
                *str = iter;
                return DISP_E_EXCEPTION;
            }
            if (iter[1] != '\'') break;
            iter++;
            strbuf_write(iter2, buf, iter-iter2);
        }
        strbuf_write(iter2, buf, iter-iter2);
        iter++;
    }else {
        while(*iter && !iswspace(*iter))
            iter++;
        strbuf_write(iter2, buf, iter-iter2);
    }

    while(iswspace(*iter))
        iter++;
    *str = iter;
    return S_OK;
}

static HRESULT do_preprocess(const Registrar *This, LPCOLESTR data, strbuf *buf)
{
    LPCOLESTR iter, iter2 = data;
    rep_list *rep_iter;

    iter = wcschr(data, '%');
    while(iter) {
        strbuf_write(iter2, buf, iter-iter2);

        iter2 = ++iter;
        if(!*iter2)
            return DISP_E_EXCEPTION;
        iter = wcschr(iter2, '%');
        if(!iter)
            return DISP_E_EXCEPTION;

        if(iter == iter2) {
            strbuf_write(L"%", buf, 1);
        }else {
            for(rep_iter = This->rep; rep_iter; rep_iter = rep_iter->next) {
                if(rep_iter->key_len == iter-iter2
                        && !wcsnicmp(iter2, rep_iter->key, rep_iter->key_len))
                    break;
            }
            if(!rep_iter) {
                WARN("Could not find replacement: %s\n", debugstr_wn(iter2, iter-iter2));
                return DISP_E_EXCEPTION;
            }

            strbuf_write(rep_iter->item, buf, -1);
        }

        iter2 = ++iter;
        iter = wcschr(iter, '%');
    }

    strbuf_write(iter2, buf, -1);
    TRACE("%s\n", debugstr_w(buf->str));

    return S_OK;
}

static HRESULT do_process_key(LPCOLESTR *pstr, HKEY parent_key, strbuf *buf, BOOL do_register)
{
    LPCOLESTR iter;
    HRESULT hres;
    LONG lres;
    HKEY hkey = 0;
    strbuf name;

    enum {
        NORMAL,
        NO_REMOVE,
        IS_VAL,
        FORCE_REMOVE,
        DO_DELETE
    } key_type = NORMAL;

    iter = *pstr;
    hres = get_word(&iter, buf);
    if(FAILED(hres))
        return hres;
    strbuf_init(&name);

    while(buf->str[1] || buf->str[0] != '}') {
        key_type = NORMAL;
        if(!lstrcmpiW(buf->str, L"NoRemove"))
            key_type = NO_REMOVE;
        else if(!lstrcmpiW(buf->str, L"ForceRemove"))
            key_type = FORCE_REMOVE;
        else if(!lstrcmpiW(buf->str, L"val"))
            key_type = IS_VAL;
        else if(!lstrcmpiW(buf->str, L"Delete"))
            key_type = DO_DELETE;

        if(key_type != NORMAL) {
            hres = get_word(&iter, buf);
            if(FAILED(hres))
                break;
        }
        TRACE("name = %s\n", debugstr_w(buf->str));

        if(do_register) {
            if(key_type == IS_VAL) {
                hkey = parent_key;
                strbuf_write(buf->str, &name, -1);
            }else if(key_type == DO_DELETE) {
                TRACE("Deleting %s\n", debugstr_w(buf->str));
                RegDeleteTreeW(parent_key, buf->str);
            }else {
                if(key_type == FORCE_REMOVE)
                    RegDeleteTreeW(parent_key, buf->str);
                lres = RegCreateKeyW(parent_key, buf->str, &hkey);
                if(lres != ERROR_SUCCESS) {
                    WARN("Could not create(open) key: %08lx\n", lres);
                    hres = HRESULT_FROM_WIN32(lres);
                    break;
                }
            }
        }else if(key_type != IS_VAL && key_type != DO_DELETE) {
            strbuf_write(buf->str, &name, -1);
            lres = RegOpenKeyW(parent_key, buf->str, &hkey);
              if(lres != ERROR_SUCCESS)
                WARN("Could not open key %s: %08lx\n", debugstr_w(name.str), lres);
        }

        if(key_type != DO_DELETE && *iter == '=') {
            iter++;
            hres = get_word(&iter, buf);
            if(FAILED(hres))
                break;
            if(buf->len != 1) {
                WARN("Wrong registry type: %s\n", debugstr_w(buf->str));
                hres = DISP_E_EXCEPTION;
                break;
            }
            if(do_register) {
                switch(buf->str[0]) {
                case 's':
                    hres = get_word(&iter, buf);
                    if(FAILED(hres))
                        break;
                    lres = RegSetValueExW(hkey, name.len ? name.str :  NULL, 0, REG_SZ, (PBYTE)buf->str,
                            (lstrlenW(buf->str)+1)*sizeof(WCHAR));
                    if(lres != ERROR_SUCCESS) {
                        WARN("Could set value of key: %08lx\n", lres);
                        hres = HRESULT_FROM_WIN32(lres);
                        break;
                    }
                    break;
                case 'd': {
                    DWORD dw;
                    hres = get_word(&iter, buf);
                    if(FAILED(hres))
                        break;
                    dw = wcstoul(buf->str, NULL, 10);
                    lres = RegSetValueExW(hkey, name.len ? name.str :  NULL, 0, REG_DWORD,
                            (PBYTE)&dw, sizeof(dw));
                    if(lres != ERROR_SUCCESS) {
                        WARN("Could set value of key: %08lx\n", lres);
                        hres = HRESULT_FROM_WIN32(lres);
                        break;
                    }
                    break;
                }
                case 'b': {
                    BYTE *bytes;
                    DWORD count;
                    DWORD i;
                    hres = get_word(&iter, buf);
                    if(FAILED(hres))
                        break;
                    count = (lstrlenW(buf->str) + 1) / 2;
                    bytes = malloc(count);
                    if(bytes == NULL) {
                        hres = E_OUTOFMEMORY;
                        break;
                    }
                    for(i = 0; i < count && buf->str[2*i]; i++) {
                        int d1, d2;
                        if((d1 = xdigit_to_int(buf->str[2*i])) == -1 || (d2 = xdigit_to_int(buf->str[2*i + 1])) == -1) {
                            hres = E_FAIL;
                            break;
                        }
                        bytes[i] = (d1 << 4) | d2;
                    }
                    if(SUCCEEDED(hres)) {
                        lres = RegSetValueExW(hkey, name.len ? name.str :  NULL, 0, REG_BINARY,
                            bytes, count);
                        if(lres != ERROR_SUCCESS) {
                            WARN("Could not set value of key: 0x%08lx\n", lres);
                            hres = HRESULT_FROM_WIN32(lres);
                        }
                    }
                    free(bytes);
                    break;
                }
                default:
                    WARN("Wrong resource type: %s\n", debugstr_w(buf->str));
                    hres = DISP_E_EXCEPTION;
                };
                if(FAILED(hres))
                    break;
            }else {
                if(*iter == '-')
                    iter++;
                hres = get_word(&iter, buf);
                if(FAILED(hres))
                    break;
            }
        }else if(key_type == IS_VAL) {
            WARN("value not set!\n");
            hres = DISP_E_EXCEPTION;
            break;
        }

        if(key_type != IS_VAL && key_type != DO_DELETE && *iter == '{' && iswspace(iter[1])) {
            hres = get_word(&iter, buf);
            if(FAILED(hres))
                break;
            hres = do_process_key(&iter, hkey, buf, do_register);
            if(FAILED(hres))
                break;
        }

        TRACE("%x %x\n", do_register, key_type);
        if(!do_register && (key_type == NORMAL || key_type == FORCE_REMOVE)) {
            TRACE("Deleting %s\n", debugstr_w(name.str));
            RegDeleteKeyW(parent_key, name.str);
        }

        if(hkey && key_type != IS_VAL)
            RegCloseKey(hkey);
        hkey = 0;
        name.len = 0;

        hres = get_word(&iter, buf);
        if(FAILED(hres))
            break;
    }

    free(name.str);
    if(hkey && key_type != IS_VAL)
        RegCloseKey(hkey);
    *pstr = iter;
    return hres;
}

static HRESULT do_process_root_key(LPCOLESTR data, BOOL do_register)
{
    LPCOLESTR iter = data;
    strbuf buf;
    HRESULT hres;
    unsigned int i;

    strbuf_init(&buf);
    hres = get_word(&iter, &buf);
    if(FAILED(hres))
        goto done;

    while(*iter) {
        if(!buf.len) {
            WARN("ward.len == 0, failed\n");
            hres = DISP_E_EXCEPTION;
            break;
        }
        for(i=0; i<ARRAY_SIZE(root_keys); i++) {
            if(!lstrcmpiW(buf.str, root_keys[i].name))
                break;
        }
        if(i == ARRAY_SIZE(root_keys)) {
            WARN("Wrong root key name: %s\n", debugstr_w(buf.str));
            hres = DISP_E_EXCEPTION;
            break;
        }
        hres = get_word(&iter, &buf);
        if(FAILED(hres))
            break;
        if(buf.str[1] || buf.str[0] != '{') {
            WARN("Failed, expected '{', got %s\n", debugstr_w(buf.str));
            hres = DISP_E_EXCEPTION;
            break;
        }
        hres = do_process_key(&iter, root_keys[i].key, &buf, do_register);
        if(FAILED(hres)) {
            WARN("Processing key failed: %08lx\n", hres);
            break;
        }
        hres = get_word(&iter, &buf);
        if(FAILED(hres))
            break;
    }

done:
    free(buf.str);
    return hres;
}

static HRESULT string_register(Registrar *This, LPCOLESTR data, BOOL do_register)
{
    strbuf buf;
    HRESULT hres;

    TRACE("(%p %s %x)\n", This, debugstr_w(data), do_register);

    strbuf_init(&buf);
    hres = do_preprocess(This, data, &buf);
    if(FAILED(hres)) {
        WARN("preprocessing failed!\n");
        free(buf.str);
        return hres;
    }

    hres = do_process_root_key(buf.str, do_register);
    if(FAILED(hres) && do_register)
        do_process_root_key(buf.str, FALSE);

    free(buf.str);
    return hres;
}

static HRESULT resource_register(Registrar *This, LPCOLESTR resFileName,
                        LPCOLESTR szID, LPCOLESTR szType, BOOL do_register)
{
    HINSTANCE hins;
    HRSRC src;
    LPSTR regstra;
    LPWSTR regstrw;
    DWORD len, reslen;
    HRESULT hres;

    hins = LoadLibraryExW(resFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(hins) {
        src = FindResourceW(hins, szID, szType);
        if(src) {
            regstra = LoadResource(hins, src);
            reslen = SizeofResource(hins, src);
            if(regstra) {
                len = MultiByteToWideChar(CP_ACP, 0, regstra, reslen, NULL, 0)+1;
                regstrw = calloc(len, sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, regstra, reslen, regstrw, len);
                regstrw[len-1] = '\0';

                hres = string_register(This, regstrw, do_register);

                free(regstrw);
            }else {
                WARN("could not load resource\n");
                hres = HRESULT_FROM_WIN32(GetLastError());
            }
        }else {
            WARN("Could not find source\n");
            hres = HRESULT_FROM_WIN32(GetLastError());
        }
        FreeLibrary(hins);
    }else {
        WARN("Could not load resource file\n");
        hres = HRESULT_FROM_WIN32(GetLastError());
    }

    return hres;
}

static HRESULT file_register(Registrar *This, LPCOLESTR fileName, BOOL do_register)
{
    HANDLE file;
    DWORD filelen, len;
    LPWSTR regstrw;
    LPSTR regstra;
    HRESULT hres;

    file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(file != INVALID_HANDLE_VALUE) {
        filelen = GetFileSize(file, NULL);
        regstra = malloc(filelen);
        if(ReadFile(file, regstra, filelen, NULL, NULL)) {
            len = MultiByteToWideChar(CP_ACP, 0, regstra, filelen, NULL, 0)+1;
            regstrw = calloc(len, sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, regstra, filelen, regstrw, len);
            regstrw[len-1] = '\0';

            hres = string_register(This, regstrw, do_register);

            free(regstrw);
        }else {
            WARN("Failed to read file %s\n", debugstr_w(fileName));
            hres = HRESULT_FROM_WIN32(GetLastError());
        }
        free(regstra);
        CloseHandle(file);
    }else {
        WARN("Could not open file %s\n", debugstr_w(fileName));
        hres = HRESULT_FROM_WIN32(GetLastError());
    }

    return hres;
}

static HRESULT WINAPI Registrar_QueryInterface(IRegistrar *iface, REFIID riid, void **ppvObject)
{
    TRACE("(%p)->(%s %p\n", iface, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(&IID_IUnknown, riid)
       || IsEqualGUID(&IID_IRegistrar, riid)
       || IsEqualGUID(&IID_IRegistrarBase, riid)) {
        IRegistrar_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI Registrar_AddRef(IRegistrar *iface)
{
    Registrar *This = impl_from_IRegistrar(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ->%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI Registrar_Release(IRegistrar *iface)
{
    Registrar *This = impl_from_IRegistrar(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ->%ld\n", This, ref);
    if(!ref) {
        IRegistrar_ClearReplacements(iface);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI Registrar_AddReplacement(IRegistrar *iface, LPCOLESTR Key, LPCOLESTR item)
{
    Registrar *This = impl_from_IRegistrar(iface);
    int len;
    rep_list *new_rep;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(Key), debugstr_w(item));

    new_rep = malloc(sizeof(*new_rep));

    new_rep->key_len  = lstrlenW(Key);
    new_rep->key = malloc((new_rep->key_len + 1) * sizeof(OLECHAR));
    memcpy(new_rep->key, Key, (new_rep->key_len+1)*sizeof(OLECHAR));

    len = lstrlenW(item)+1;
    new_rep->item = malloc(len*sizeof(OLECHAR));
    memcpy(new_rep->item, item, len*sizeof(OLECHAR));

    new_rep->next = This->rep;
    This->rep = new_rep;

    return S_OK;
}

static HRESULT WINAPI Registrar_ClearReplacements(IRegistrar *iface)
{
    Registrar *This = impl_from_IRegistrar(iface);
    rep_list *iter, *iter2;

    TRACE("(%p)\n", This);

    if(!This->rep)
        return S_OK;

    iter = This->rep;
    while(iter) {
        iter2 = iter->next;
        free(iter->key);
        free(iter->item);
        free(iter);
        iter = iter2;
    }

    This->rep = NULL;
    return S_OK;
}

static HRESULT WINAPI Registrar_ResourceRegisterSz(IRegistrar* iface, LPCOLESTR resFileName,
                LPCOLESTR szID, LPCOLESTR szType)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s %s %s)\n", This, debugstr_w(resFileName), debugstr_w(szID), debugstr_w(szType));
    return resource_register(This, resFileName, szID, szType, TRUE);
}

static HRESULT WINAPI Registrar_ResourceUnregisterSz(IRegistrar* iface, LPCOLESTR resFileName,
                LPCOLESTR szID, LPCOLESTR szType)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s %s %s)\n", This, debugstr_w(resFileName), debugstr_w(szID), debugstr_w(szType));
    return resource_register(This, resFileName, szID, szType, FALSE);
}

static HRESULT WINAPI Registrar_FileRegister(IRegistrar* iface, LPCOLESTR fileName)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(fileName));
    return file_register(This, fileName, TRUE);
}

static HRESULT WINAPI Registrar_FileUnregister(IRegistrar* iface, LPCOLESTR fileName)
{
    Registrar *This = impl_from_IRegistrar(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(fileName));
    return file_register(This, fileName, FALSE);
}

static HRESULT WINAPI Registrar_StringRegister(IRegistrar* iface, LPCOLESTR data)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(data));
    return string_register(This, data, TRUE);
}

static HRESULT WINAPI Registrar_StringUnregister(IRegistrar* iface, LPCOLESTR data)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(data));
    return string_register(This, data, FALSE);
}

static HRESULT WINAPI Registrar_ResourceRegister(IRegistrar* iface, LPCOLESTR resFileName,
                UINT nID, LPCOLESTR szType)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s %d %s)\n", iface, debugstr_w(resFileName), nID, debugstr_w(szType));
    return resource_register(This, resFileName, MAKEINTRESOURCEW(nID), szType, TRUE);
}

static HRESULT WINAPI Registrar_ResourceUnregister(IRegistrar* iface, LPCOLESTR resFileName,
                UINT nID, LPCOLESTR szType)
{
    Registrar *This = impl_from_IRegistrar(iface);
    TRACE("(%p)->(%s %d %s)\n", This, debugstr_w(resFileName), nID, debugstr_w(szType));
    return resource_register(This, resFileName, MAKEINTRESOURCEW(nID), szType, FALSE);
}

static const IRegistrarVtbl RegistrarVtbl = {
    Registrar_QueryInterface,
    Registrar_AddRef,
    Registrar_Release,
    Registrar_AddReplacement,
    Registrar_ClearReplacements,
    Registrar_ResourceRegisterSz,
    Registrar_ResourceUnregisterSz,
    Registrar_FileRegister,
    Registrar_FileUnregister,
    Registrar_StringRegister,
    Registrar_StringUnregister,
    Registrar_ResourceRegister,
    Registrar_ResourceUnregister,
};

/***********************************************************************
 *           AtlCreateRegistrar              [atl100.@]
 */
HRESULT WINAPI AtlCreateRegistrar(IRegistrar **ret)
{
    Registrar *registrar;

    registrar = calloc(1, sizeof(*registrar));
    if(!registrar)
        return E_OUTOFMEMORY;

    registrar->IRegistrar_iface.lpVtbl = &RegistrarVtbl;
    registrar->ref = 1;

    *ret = &registrar->IRegistrar_iface;
    return S_OK;
}

/***********************************************************************
 *           AtlUpdateRegistryFromResourceD         [atl100.@]
 */
HRESULT WINAPI AtlUpdateRegistryFromResourceD(HINSTANCE inst, LPCOLESTR res,
        BOOL bRegister, struct _ATL_REGMAP_ENTRY *pMapEntries, IRegistrar *pReg)
{
    const struct _ATL_REGMAP_ENTRY *iter;
    WCHAR module_name[MAX_PATH];
    IRegistrar *registrar;
    HRESULT hres;

    if(!GetModuleFileNameW(inst, module_name, MAX_PATH)) {
        FIXME("hinst %p: did not get module name\n", inst);
        return E_FAIL;
    }

    TRACE("%p (%s), %s, %d, %p, %p\n", inst, debugstr_w(module_name),
	debugstr_w(res), bRegister, pMapEntries, pReg);

    if(pReg) {
        registrar = pReg;
    }else {
        hres = AtlCreateRegistrar(&registrar);
        if(FAILED(hres))
            return hres;
    }

    IRegistrar_AddReplacement(registrar, L"MODULE", module_name);

    for (iter = pMapEntries; iter && iter->szKey; iter++)
        IRegistrar_AddReplacement(registrar, iter->szKey, iter->szData);

    if(bRegister)
        hres = IRegistrar_ResourceRegisterSz(registrar, module_name, res, L"REGISTRY");
    else
        hres = IRegistrar_ResourceUnregisterSz(registrar, module_name, res, L"REGISTRY");

    if(registrar != pReg)
        IRegistrar_Release(registrar);
    return hres;
}
