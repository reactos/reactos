/*
 * PROJECT:     ReactOS Registry Editor
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Exporting registry data to a text file
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "regedit.h"

static HKEY reg_class_keys[] =
{
    HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
    HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_DYN_DATA
};

static LPWSTR load_str(INT id)
{
    /* Use rotation buffer */
    static WCHAR s_asz[3][MAX_PATH];
    static INT s_index = 0;
    LPWSTR psz;
    LoadStringW(hInst, id, s_asz[s_index], MAX_PATH);
    psz = s_asz[s_index];
    s_index = (s_index + 1) % _countof(s_asz);
    return psz;
}

static void txt_fputs(FILE *fp, LPCWSTR str)
{
    fwrite(str, lstrlenW(str) * sizeof(WCHAR), 1, fp);
}

static void txt_newline(FILE *fp)
{
    txt_fputs(fp, L"\r\n");
}

static void txt_fprintf(FILE *fp, LPCWSTR format, ...)
{
    WCHAR line[1024];
    va_list va;
    va_start(va, format);
    StringCchVPrintfW(line, _countof(line), format, va);
    txt_fputs(fp, line);
    va_end(va);
}

static HKEY txt_parse_key_name(LPCWSTR key_name, WCHAR **key_path)
{
    unsigned int i;

    if (!key_name) return 0;

    *key_path = wcschr(key_name, '\\');
    if (*key_path)
        (*key_path)++;

    for (i = 0; i < _countof(reg_class_keys); i++)
    {
        int len = lstrlenW(reg_class_namesW[i]);
        if (!_wcsnicmp(key_name, reg_class_namesW[i], len) &&
            (key_name[len] == 0 || key_name[len] == '\\'))
        {
            return reg_class_keys[i];
        }
    }

    return 0;
}

static void txt_export_binary(FILE *fp, const void *data, size_t size)
{
    const BYTE *pb = data;
    for (DWORD addr = 0; addr < size; addr += 0x10)
    {
        txt_fprintf(fp, L"%08X  ", addr);
        for (size_t column = 0; column < 16; ++column)
        {
            if (addr + column >= size)
            {
                if (column == 8)
                    txt_fputs(fp, L"  ");
                txt_fputs(fp, L"   ");
            }
            else
            {
                if (column == 8)
                    txt_fputs(fp, L" -");
                txt_fprintf(fp, L" %02x", (pb[addr + column] & 0xFF));
            }
        }
        txt_fputs(fp, L"  ");
        for (size_t column = 0; column < 16; ++column)
        {
            if (addr + column >= size)
            {
                break;
            }
            else
            {
                BYTE b = pb[addr + column];
                if (isprint(b) || IsCharAlphaNumericW(b))
                    txt_fprintf(fp, L"%c", b);
                else
                    txt_fputs(fp, L".");
            }
        }
        txt_newline(fp);
    }
}

static void txt_export_field(FILE *fp, LPCWSTR label, LPCWSTR value)
{
    txt_fprintf(fp, L"%-19s%s\r\n", label, value);
}

static void txt_export_multi_sz(FILE *fp, const void *data, size_t size)
{
    LPCWSTR pch;
    for (pch = data; *pch; pch += lstrlenW(pch) + 1)
    {
        if (pch == data)
            txt_export_field(fp, load_str(IDS_FIELD_DATA), pch);
        else
            txt_export_field(fp, L"", pch);
    }
}

static void txt_export_type(FILE *fp, LPCWSTR type)
{
    txt_export_field(fp, load_str(IDS_FIELD_TYPE), type);
}

static void txt_export_name(FILE *fp, LPCWSTR name)
{
    txt_export_field(fp, load_str(IDS_FIELD_NAME), name);
}

static void
txt_export_data(FILE *fp, INT i, LPCWSTR value_name, DWORD value_len, DWORD type,
                const void *data, size_t size)
{
    LPCWSTR pszType;

    txt_fprintf(fp, load_str(IDS_VALUE_INDEX), i);
    txt_newline(fp);
    txt_export_name(fp, value_name);

    switch (type)
    {
        case REG_SZ:
            txt_export_type(fp, L"REG_SZ");
            txt_export_field(fp, load_str(IDS_FIELD_DATA), data);
            break;

        case REG_DWORD:
            txt_export_type(fp, L"REG_DWORD");
            txt_fprintf(fp, L"%-19s0x%lx\r\n", load_str(IDS_FIELD_DATA), *(DWORD*)data);
            break;

        case REG_EXPAND_SZ:
            txt_export_type(fp, L"REG_EXPAND_SZ");
            txt_export_field(fp, load_str(IDS_FIELD_DATA), data);
            break;

        case REG_MULTI_SZ:
            txt_export_type(fp, L"REG_MULTI_SZ");
            txt_export_multi_sz(fp, data, size);
            break;

        case REG_BINARY:
        case REG_QWORD:
        case REG_NONE:
        default:
            if (type == REG_BINARY)
                pszType = L"REG_BINARY";
            else if (type == REG_QWORD)
                pszType = L"REG_QWORD";
            else if (type == REG_NONE)
                pszType = L"REG_NONE";
            else
                pszType = load_str(IDS_UNKNOWN);

            txt_export_type(fp, pszType);
            txt_export_field(fp, load_str(IDS_FIELD_DATA), L"");
            txt_export_binary(fp, data, size);
            break;
    }

    txt_newline(fp);
}

static WCHAR *
txt_build_subkey_path(LPCWSTR path, DWORD path_len, LPCWSTR subkey_name, DWORD subkey_len)
{
    WCHAR *subkey_path;
    SIZE_T cb_subkey_path = (path_len + subkey_len + 2) * sizeof(WCHAR);
    subkey_path = malloc(cb_subkey_path);
    StringCbPrintfW(subkey_path, cb_subkey_path, L"%s\\%s", path, subkey_name);
    return subkey_path;
}

static void txt_export_key_name(FILE *fp, LPCWSTR name)
{
    txt_export_field(fp, load_str(IDS_FIELD_KEY_NAME), name);
}

static void txt_export_class_and_last_write(FILE *fp, HKEY key)
{
    WCHAR szClassName[MAX_PATH];
    DWORD cchClassName = _countof(szClassName);
    FILETIME ftLastWrite, ftLocal, ftNull = { 0 };
    SYSTEMTIME stLastWrite;
    WCHAR sz1[64], sz2[64];
    LONG error;

    error = RegQueryInfoKeyW(key, szClassName, &cchClassName, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, &ftLastWrite);
    if (error != ERROR_SUCCESS)
    {
        cchClassName = 0;
        ftLastWrite = ftNull;
    }

    szClassName[_countof(szClassName) - 1] = UNICODE_NULL;

    if (cchClassName > 0)
        txt_export_field(fp, load_str(IDS_FIELD_CLASS_NAME), szClassName);
    else
        txt_export_field(fp, load_str(IDS_FIELD_CLASS_NAME), load_str(IDS_NO_CLASS_NAME));

    if (memcmp(&ftLastWrite, &ftNull, sizeof(ftNull)) == 0)
    {
        txt_export_field(fp, load_str(IDS_FIELD_LASTWRITE), load_str(IDS_NULL_TIMESTAMP));
        return;
    }

    FileTimeToLocalFileTime(&ftLastWrite, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &stLastWrite);
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLastWrite, NULL, sz1, _countof(sz1));
    GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &stLastWrite, NULL, sz2, _countof(sz2));
    txt_fprintf(fp, L"%-19s%s - %s\r\n", load_str(IDS_FIELD_LASTWRITE), sz1, sz2);
}

static void txt_export_registry_data(FILE *fp, HKEY key, LPCWSTR path)
{
    LONG rc;
    DWORD max_value_len = MAX_PATH, value_len;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD i, type, path_len;
    WCHAR *value_name, *subkey_name, *subkey_path;
    BYTE *data;
    HKEY subkey;

    txt_export_key_name(fp, path);
    txt_export_class_and_last_write(fp, key);

    value_name = malloc(max_value_len * sizeof(WCHAR));
    data = malloc(max_data_bytes);

    i = 0;
    for (;;)
    {
        value_len = max_value_len;
        data_size = max_data_bytes;
        rc = RegEnumValueW(key, i, value_name, &value_len, NULL, &type, data, &data_size);
        if (rc == ERROR_SUCCESS)
        {
            txt_export_data(fp, i, value_name, value_len, type, data, data_size);
            i++;
        }
        else if (rc == ERROR_MORE_DATA)
        {
            if (data_size > max_data_bytes)
            {
                max_data_bytes = data_size;
                data = realloc(data, max_data_bytes);
            }
            else
            {
                max_value_len *= 2;
                value_name = realloc(value_name, max_value_len * sizeof(WCHAR));
            }
        }
        else break;
    }

    free(data);
    free(value_name);

    subkey_name = malloc(MAX_PATH * sizeof(WCHAR));

    path_len = lstrlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_PATH;
        rc = RegEnumKeyExW(key, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            if (i == 0)
                txt_newline(fp);

            subkey_path = txt_build_subkey_path(path, path_len, subkey_name, subkey_len);
            if (!RegOpenKeyExW(key, subkey_name, 0, KEY_READ, &subkey))
            {
                txt_newline(fp);
                txt_export_registry_data(fp, subkey, subkey_path);
                RegCloseKey(subkey);
            }
            free(subkey_path);
            i++;
        }
        else break;
    }

    free(subkey_name);
}

static FILE *txt_open_export_file(LPCWSTR file_name)
{
    FILE *file = _wfopen(file_name, L"wb");
    if (file)
        fwrite("\xFF\xFE", 2, 1, file);
    return file;
}

static HKEY txt_open_export_key(HKEY key_class, LPCWSTR subkey, LPCWSTR path)
{
    HKEY key;

    if (RegOpenKeyExW(key_class, subkey, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return NULL;

    return key;
}

static BOOL txt_export_key(LPCWSTR file_name, LPCWSTR path)
{
    HKEY key_class, key;
    WCHAR *subkey;
    FILE *fp;

    if (!(key_class = txt_parse_key_name(path, &subkey)))
    {
        if (subkey) *(subkey - 1) = 0;
        return FALSE;
    }

    if (!(key = txt_open_export_key(key_class, subkey, path)))
        return FALSE;

    fp = txt_open_export_file(file_name);
    if (fp)
    {
        txt_export_registry_data(fp, key, path);
        txt_newline(fp);
        fclose(fp);
    }

    RegCloseKey(key);
    return fp != NULL;
}

static BOOL txt_export_all(LPCWSTR file_name, LPCWSTR path)
{
    FILE *fp;
    int i;
    HKEY classes[] = {HKEY_LOCAL_MACHINE, HKEY_USERS}, key;
    WCHAR *class_name;

    fp = txt_open_export_file(file_name);
    if (!fp)
        return FALSE;

    for (i = 0; i < _countof(classes); i++)
    {
        if (!(key = txt_open_export_key(classes[i], NULL, path)))
        {
            fclose(fp);
            return FALSE;
        }

        class_name = malloc((lstrlenW(reg_class_namesW[i]) + 1) * sizeof(WCHAR));
        lstrcpyW(class_name, reg_class_namesW[i]);

        txt_export_registry_data(fp, classes[i], class_name);

        free(class_name);
        RegCloseKey(key);
    }

    txt_newline(fp);
    fclose(fp);

    return TRUE;
}

BOOL txt_export_registry_key(LPCWSTR file_name, LPCWSTR path)
{
    if (path && *path)
        return txt_export_key(file_name, path);
    else
        return txt_export_all(file_name, path);
}
