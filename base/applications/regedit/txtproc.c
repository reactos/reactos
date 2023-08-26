/*
 * PROJECT:     ReactOS Registry Editor
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Exporting registry data to a text file
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "regedit.h"

#define MAX_SUBKEY_LEN   257

static HKEY reg_class_keys[] =
{
    HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
    HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_DYN_DATA
};

static INT text_length(const WCHAR *text, size_t byte_size)
{
    size_t cch;
    StringCchLengthW(text, byte_size / sizeof(WCHAR), &cch);
    return (INT)cch;
}

static LPWSTR load_str(INT id)
{
    static WCHAR s_asz[3][MAX_PATH];
    static INT s_index = 0;
    LPWSTR psz;
    LoadStringW(hInst, id, s_asz[s_index], MAX_PATH);
    psz = s_asz[s_index];
    s_index = (s_index + 1) % _countof(s_asz);
    return psz;
}

static void TXTPROC_write_line(FILE *fp, const WCHAR *str)
{
    fwrite(str, lstrlenW(str) * sizeof(WCHAR), 1, fp);
}

static void export_newline(FILE *fp)
{
    TXTPROC_write_line(fp, L"\r\n");
}

static void TXTPROC_fprintf(FILE *fp, const WCHAR *format, ...)
{
    WCHAR line[1024];
    va_list va;
    va_start(va, format);
    StringCchVPrintfW(line, _countof(line), format, va);
    TXTPROC_write_line(fp, line);
    va_end(va);
}

static HKEY parse_key_name(WCHAR *key_name, WCHAR **key_path)
{
    unsigned int i;

    if (!key_name) return 0;

    *key_path = wcschr(key_name, '\\');
    if (*key_path) (*key_path)++;

    for (i = 0; i < ARRAY_SIZE(reg_class_keys); i++)
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

static void export_binary(FILE *fp, const void *data, size_t size)
{
    const BYTE *pb = data;
    for (DWORD addr = 0; addr < size; addr += 0x10)
    {
        TXTPROC_fprintf(fp, L"%08X  ", addr);
        for (size_t column = 0; column < 16; ++column)
        {
            if (addr + column >= size)
            {
                if (column == 8)
                    TXTPROC_fprintf(fp, L"  ");
                TXTPROC_fprintf(fp, L"   ");
            }
            else
            {
                if (column == 8)
                    TXTPROC_fprintf(fp, L" -");
                TXTPROC_fprintf(fp, L" %02x", (pb[addr + column] & 0xFF));
            }
        }
        TXTPROC_fprintf(fp, L"  ");
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
                    TXTPROC_fprintf(fp, L"%c", b);
                else
                    TXTPROC_fprintf(fp, L".");
            }
        }
        export_newline(fp);
    }
}

static void export_multi_string(FILE *fp, const void *data, size_t size)
{
    const WCHAR *pch;
    for (pch = data; *pch; pch += lstrlenW(pch) + 1)
    {
        if (pch == data)
            TXTPROC_fprintf(fp, L"%-20s%-*s\r\n", load_str(IDS_FIELD_DATA), lstrlenW(pch), pch);
        else
            TXTPROC_fprintf(fp, L"%-20s%-*s\r\n", L"", lstrlenW(pch), pch);
    }
}

static void export_data(FILE *fp, INT i, WCHAR *value_name, DWORD value_len, DWORD type,
                        const void *data, size_t size)
{
    TXTPROC_fprintf(fp, load_str(IDS_VALUE_INDEX), i);
    TXTPROC_fprintf(fp, L"\r\n%-20s%s\r\n", load_str(IDS_FIELD_NAME), value_name);

    switch (type)
    {
    case REG_SZ:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), L"REG_SZ");
        TXTPROC_fprintf(fp, L"%-20s%-*s\r\n", load_str(IDS_FIELD_DATA), text_length(data, size), data);
        break;
    case REG_DWORD:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), L"REG_DWORD");
        TXTPROC_fprintf(fp, L"%-20s0x%08lX\r\n", load_str(IDS_FIELD_DATA), (DWORD*)data);
        break;
    case REG_NONE:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), L"REG_NONE");
        break;
    case REG_EXPAND_SZ:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), L"REG_EXPAND_SZ");
        TXTPROC_fprintf(fp, L"%-20s%-*s\r\n", load_str(IDS_FIELD_DATA), text_length(data, size), data);
        break;
    case REG_BINARY:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), L"REG_BINARY");
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_DATA), L"");
        export_binary(fp, data, size);
        break;
    case REG_MULTI_SZ:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), L"REG_MULTI_SZ");
        export_multi_string(fp, data, size);
        break;
    default:
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_TYPE), load_str(IDS_UNKNOWN));
        export_binary(fp, data, size);
        break;
    }

    export_newline(fp);
}

static WCHAR *build_subkey_path(WCHAR *path, DWORD path_len, WCHAR *subkey_name, DWORD subkey_len)
{
    WCHAR *subkey_path;
    subkey_path = malloc((path_len + subkey_len + 2) * sizeof(WCHAR));
    swprintf(subkey_path, L"%s\\%s", path, subkey_name);
    return subkey_path;
}

static void export_key_name(FILE *fp, const WCHAR *name)
{
    TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_KEY_NAME), name);
}

static void export_class_and_last_write(FILE *fp, HKEY key)
{
    WCHAR szClassName[MAX_PATH];
    DWORD cchClassName = _countof(szClassName);
    FILETIME ftLastWrite, ftLocal, ftNull = { 0 };
    SYSTEMTIME stLastWrite;
    WCHAR sz1[64], sz2[64];

    RegQueryInfoKey(key, szClassName, &cchClassName, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                    NULL, &ftLastWrite);

    if (cchClassName > 0)
        TXTPROC_fprintf(fp, L"%-20s%-*s\r\n", load_str(IDS_FIELD_CLASS_NAME),
                        text_length(szClassName, cchClassName * sizeof(WCHAR)), szClassName);
    else
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_CLASS_NAME), load_str(IDS_NO_CLASS_NAME));

    if (memcmp(&ftLastWrite, &ftNull, sizeof(ftNull)) == 0)
    {
        TXTPROC_fprintf(fp, L"%-20s%s\r\n", load_str(IDS_FIELD_LASTWRITE), load_str(IDS_NULL_TIMESTAMP));
    }
    else
    {
        FileTimeToLocalFileTime(&ftLastWrite, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &stLastWrite);
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLastWrite, NULL, sz1, _countof(sz1));
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &stLastWrite, NULL, sz2, _countof(sz2));
        TXTPROC_fprintf(fp, L"%-20s%s - %s\r\n", load_str(IDS_FIELD_LASTWRITE), sz1, sz2);
    }
}

static void export_registry_data(FILE *fp, HKEY key, WCHAR *path)
{
    LONG rc;
    DWORD max_value_len = 256, value_len;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD i, type, path_len;
    WCHAR *value_name, *subkey_name, *subkey_path;
    BYTE *data;
    HKEY subkey;

    export_key_name(fp, path);
    export_class_and_last_write(fp, key);

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
            export_data(fp, i, value_name, value_len, type, data, data_size);
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

    subkey_name = malloc(MAX_SUBKEY_LEN * sizeof(WCHAR));

    path_len = lstrlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_SUBKEY_LEN;
        rc = RegEnumKeyExW(key, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            if (i == 0)
                export_newline(fp);

            subkey_path = build_subkey_path(path, path_len, subkey_name, subkey_len);
            if (!RegOpenKeyExW(key, subkey_name, 0, KEY_READ, &subkey))
            {
                export_newline(fp);
                export_registry_data(fp, subkey, subkey_path);
                RegCloseKey(subkey);
            }
            free(subkey_path);
            i++;
        }
        else break;
    }

    free(subkey_name);
}

static FILE *TXTPROC_open_export_file(WCHAR *file_name)
{
    FILE *file = _wfopen(file_name, L"wb");
    if (!file)
    {
        _wperror(L"regedit");
        error_exit(STRING_CANNOT_OPEN_FILE, file_name);
    }

    fwrite("\xFF\xFE", 2, 1, file);
    return file;
}

static HKEY open_export_key(HKEY key_class, WCHAR *subkey, WCHAR *path)
{
    HKEY key;

    if (!RegOpenKeyExW(key_class, subkey, 0, KEY_READ, &key))
        return key;

    output_message(STRING_OPEN_KEY_FAILED, path);
    return NULL;
}

static BOOL export_key(WCHAR *file_name, WCHAR *path)
{
    HKEY key_class, key;
    WCHAR *subkey;
    FILE *fp;

    if (!(key_class = parse_key_name(path, &subkey)))
    {
        if (subkey) *(subkey - 1) = 0;
        output_message(STRING_INVALID_SYSTEM_KEY, path);
        return FALSE;
    }

    if (!(key = open_export_key(key_class, subkey, path)))
        return FALSE;

    fp = TXTPROC_open_export_file(file_name);
    export_registry_data(fp, key, path);
    export_newline(fp);
    fclose(fp);

    RegCloseKey(key);
    return TRUE;
}

static BOOL export_all(WCHAR *file_name, WCHAR *path)
{
    FILE *fp;
    int i;
    HKEY classes[] = {HKEY_LOCAL_MACHINE, HKEY_USERS}, key;
    WCHAR *class_name;

    fp = TXTPROC_open_export_file(file_name);

    for (i = 0; i < ARRAY_SIZE(classes); i++)
    {
        if (!(key = open_export_key(classes[i], NULL, path)))
        {
            fclose(fp);
            return FALSE;
        }

        class_name = malloc((lstrlenW(reg_class_namesW[i]) + 1) * sizeof(WCHAR));
        lstrcpyW(class_name, reg_class_namesW[i]);

        export_registry_data(fp, classes[i], class_name);

        free(class_name);
        RegCloseKey(key);
    }

    export_newline(fp);
    fclose(fp);

    return TRUE;
}

BOOL export_registry_key_in_text(WCHAR *file_name, WCHAR *path)
{
    if (path && *path)
        return export_key(file_name, path);
    else
        return export_all(file_name, path);
}
