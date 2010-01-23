/*
 *        self-registerable dll functions for qedit.dll
 *
 * Copyright (C) 2008 Google (Lei Zhang)
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

#include "qedit_private.h"
#include "winreg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

struct regsvr_coclass
{
    CLSID const *clsid;   /* NULL for end of list */
    LPCSTR name;          /* can be NULL to omit */
    LPCSTR ips;           /* can be NULL to omit */
    LPCSTR ips32;         /* can be NULL to omit */
    LPCSTR ips32_tmodel;  /* can be NULL to omit */
    LPCSTR progid;        /* can be NULL to omit */
    LPCSTR viprogid;      /* can be NULL to omit */
    LPCSTR progid_extra;  /* can be NULL to omit */
};

static HRESULT register_coclasses(struct regsvr_coclass const *list);
static HRESULT unregister_coclasses(struct regsvr_coclass const *list);

/***********************************************************************
 *              static string constants
 */
static WCHAR const clsid_keyname[6] = {
  'C', 'L', 'S', 'I', 'D', 0 };
static WCHAR const curver_keyname[7] = {
    'C', 'u', 'r', 'V', 'e', 'r', 0 };
static WCHAR const ips_keyname[13] = {
  'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r', 0 };
static WCHAR const ips32_keyname[15] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r', '3', '2', 0 };
static WCHAR const progid_keyname[7] = {
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const viprogid_keyname[25] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 'I', 'n', 'd', 'e', 'p',
    'e', 'n', 'd', 'e', 'n', 't', 'P', 'r', 'o', 'g', 'I', 'D',
    0 };
static char const tmodel_valuename[] = "ThreadingModel";

/***********************************************************************
 *              static helper functions
 */
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
                                   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
                                   char const *value);
static LONG register_progid(WCHAR const *clsid,
                            char const *progid, char const *curver_progid,
                            char const *name, char const *extra);



/***********************************************************************
 *              register_coclasses
 */
static HRESULT register_coclasses(struct regsvr_coclass const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
        WCHAR buf[39];
        HKEY clsid_key;

        StringFromGUID2(list->clsid, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
                              KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
        if (res != ERROR_SUCCESS) goto error_close_coclass_key;

        if (list->name) {
            res = RegSetValueExA(clsid_key, NULL, 0, REG_SZ,
                                 (CONST BYTE*)(list->name),
                                 strlen(list->name) + 1);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->ips) {
            res = register_key_defvalueA(clsid_key, ips_keyname, list->ips);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->ips32) {
            HKEY ips32_key;

            res = RegCreateKeyExW(clsid_key, ips32_keyname, 0, NULL, 0,
                                  KEY_READ | KEY_WRITE, NULL,
                                  &ips32_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;

            res = RegSetValueExA(ips32_key, NULL, 0, REG_SZ,
                                 (CONST BYTE*)list->ips32,
                                 lstrlenA(list->ips32) + 1);
            if (res == ERROR_SUCCESS && list->ips32_tmodel)
                res = RegSetValueExA(ips32_key, tmodel_valuename, 0, REG_SZ,
                                     (CONST BYTE*)list->ips32_tmodel,
                                     strlen(list->ips32_tmodel) + 1);
            RegCloseKey(ips32_key);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->progid) {
            res = register_key_defvalueA(clsid_key, progid_keyname,
                                         list->progid);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;

            res = register_progid(buf, list->progid, NULL,
                                  list->name, list->progid_extra);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->viprogid) {
            res = register_key_defvalueA(clsid_key, viprogid_keyname,
                                         list->viprogid);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;

            res = register_progid(buf, list->viprogid, list->progid,
                                  list->name, list->progid_extra);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

error_close_clsid_key:
        RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *              unregister_coclasses
 */
static HRESULT unregister_coclasses(struct regsvr_coclass const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
                        KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
        WCHAR buf[39];

        StringFromGUID2(list->clsid, buf, 39);
        res = RegDeleteTreeW(coclass_key, buf);
        if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
        if (res != ERROR_SUCCESS) goto error_close_coclass_key;

        if (list->progid) {
            res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->progid);
            if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
            if (res != ERROR_SUCCESS) goto error_close_coclass_key;
        }

        if (list->viprogid) {
            res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->viprogid);
            if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
            if (res != ERROR_SUCCESS) goto error_close_coclass_key;
        }
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *              regsvr_key_defvalueW
 */
static LONG register_key_defvalueW(
    HKEY base,
    WCHAR const *name,
    WCHAR const *value)
{
    LONG res;
    HKEY key;

    res = RegCreateKeyExW(base, name, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &key, NULL);
    if (res != ERROR_SUCCESS) return res;
    res = RegSetValueExW(key, NULL, 0, REG_SZ, (CONST BYTE*)value,
                         (lstrlenW(value) + 1) * sizeof(WCHAR));
    RegCloseKey(key);
    return res;
}

/***********************************************************************
 *              regsvr_key_defvalueA
 */
static LONG register_key_defvalueA(
    HKEY base,
    WCHAR const *name,
    char const *value)
{
  LONG res;
  HKEY key;

  res = RegCreateKeyExW(base, name, 0, NULL, 0,
                        KEY_READ | KEY_WRITE, NULL, &key, NULL);
  if (res != ERROR_SUCCESS) return res;
  res = RegSetValueExA(key, NULL, 0, REG_SZ, (CONST BYTE*)value,
                       lstrlenA(value) + 1);
  RegCloseKey(key);
  return res;
}

/***********************************************************************
 *              regsvr_progid
 */
static LONG register_progid(
    WCHAR const *clsid,
    char const *progid,
    char const *curver_progid,
    char const *name,
    char const *extra)
{
    LONG res;
    HKEY progid_key;

    res = RegCreateKeyExA(HKEY_CLASSES_ROOT, progid, 0,
                          NULL, 0, KEY_READ | KEY_WRITE, NULL,
                          &progid_key, NULL);
    if (res != ERROR_SUCCESS) return res;

    if (name) {
        res = RegSetValueExA(progid_key, NULL, 0, REG_SZ,
                             (CONST BYTE*)name, strlen(name) + 1);
        if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (clsid) {
        res = register_key_defvalueW(progid_key, clsid_keyname, clsid);
        if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (curver_progid) {
        res = register_key_defvalueA(progid_key, curver_keyname,
                                     curver_progid);
        if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (extra) {
        HKEY extra_key;

        res = RegCreateKeyExA(progid_key, extra, 0,
                              NULL, 0, KEY_READ | KEY_WRITE, NULL,
                              &extra_key, NULL);
        if (res == ERROR_SUCCESS)
            RegCloseKey(extra_key);
    }

error_close_progid_key:
    RegCloseKey(progid_key);
    return res;
}

/***********************************************************************
 *              coclass list
 */
static struct regsvr_coclass const coclass_list[] = {
    {   &CLSID_MediaDet,
        "MediaDet",
        NULL,
        "qedit.dll",
        "Both"
    },
    { NULL }                    /* list terminator */
};

/***********************************************************************
 *                DllRegisterServer (QEDIT.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = register_coclasses(coclass_list);
    return hr;
}

/***********************************************************************
 *                DllUnregisterServer (QEDIT.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_coclasses(coclass_list);
    return S_OK;
}
