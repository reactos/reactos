/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "winnls.h"
#include "objbase.h"
#include "oleauto.h"

#include "msipriv.h"
#include "msiserver.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/* types arranged by precedence */
#define FORMAT_NULL         0x0001
#define FORMAT_LITERAL      0x0002
#define FORMAT_NUMBER       0x0004
#define FORMAT_LBRACK       0x0010
#define FORMAT_LBRACE       0x0020
#define FORMAT_RBRACK       0x0011
#define FORMAT_RBRACE       0x0021
#define FORMAT_ESCAPE       0x0040
#define FORMAT_PROPNULL     0x0080
#define FORMAT_ERROR        0x1000
#define FORMAT_FAIL         0x2000

#define left_type(x) (x & 0xF0)

typedef struct _tagFORMAT
{
    MSIPACKAGE *package;
    MSIRECORD *record;
    LPWSTR deformatted;
    int len;
    int n;
    BOOL propfailed;
    BOOL groupfailed;
    int groups;
} FORMAT;

typedef struct _tagFORMSTR
{
    struct list entry;
    int n;
    int len;
    int type;
    BOOL propfound;
    BOOL nonprop;
} FORMSTR;

typedef struct _tagSTACK
{
    struct list items;
} STACK;

static STACK *create_stack(void)
{
    STACK *stack = msi_alloc(sizeof(STACK));
    list_init(&stack->items);
    return stack;
}

static void free_stack(STACK *stack)
{
    while (!list_empty(&stack->items))
    {
        FORMSTR *str = LIST_ENTRY(list_head(&stack->items), FORMSTR, entry);
        list_remove(&str->entry);
        msi_free(str);
    }

    msi_free(stack);
}

static void stack_push(STACK *stack, FORMSTR *str)
{
    list_add_head(&stack->items, &str->entry);
}

static FORMSTR *stack_pop(STACK *stack)
{
    FORMSTR *ret;

    if (list_empty(&stack->items))
        return NULL;

    ret = LIST_ENTRY(list_head(&stack->items), FORMSTR, entry);
    list_remove(&ret->entry);
    return ret;
}

static FORMSTR *stack_find(STACK *stack, int type)
{
    FORMSTR *str;

    LIST_FOR_EACH_ENTRY(str, &stack->items, FORMSTR, entry)
    {
        if (str->type == type)
            return str;
    }

    return NULL;
}

static FORMSTR *stack_peek(STACK *stack)
{
    return LIST_ENTRY(list_head(&stack->items), FORMSTR, entry);
}

static LPCWSTR get_formstr_data(FORMAT *format, FORMSTR *str)
{
    return &format->deformatted[str->n];
}

static LPWSTR dup_formstr(FORMAT *format, FORMSTR *str)
{
    LPWSTR val;
    LPCWSTR data;

    if (str->len == 0)
        return NULL;

    val = msi_alloc((str->len + 1) * sizeof(WCHAR));
    data = get_formstr_data(format, str);
    lstrcpynW(val, data, str->len + 1);

    return val;
}

static LPWSTR deformat_index(FORMAT *format, FORMSTR *str)
{
    LPWSTR val, ret;

    val = msi_alloc((str->len + 1) * sizeof(WCHAR));
    lstrcpynW(val, get_formstr_data(format, str), str->len + 1);

    ret = msi_dup_record_field(format->record, atoiW(val));

    msi_free(val);
    return ret;
}

static LPWSTR deformat_property(FORMAT *format, FORMSTR *str)
{
    LPWSTR val, ret;

    val = msi_alloc((str->len + 1) * sizeof(WCHAR));
    lstrcpynW(val, get_formstr_data(format, str), str->len + 1);

    ret = msi_dup_property(format->package, val);

    msi_free(val);
    return ret;
}

static LPWSTR deformat_component(FORMAT *format, FORMSTR *str)
{
    LPWSTR key, ret = NULL;
    MSICOMPONENT *comp;
    BOOL source;

    key = msi_alloc((str->len + 1) * sizeof(WCHAR));
    lstrcpynW(key, get_formstr_data(format, str), str->len + 1);

    comp = get_loaded_component(format->package, key);
    if (!comp)
        goto done;

    source = (comp->Action == INSTALLSTATE_SOURCE) ? TRUE : FALSE;
    ret = resolve_folder(format->package, comp->Directory, source, FALSE, TRUE, NULL);

done:
    msi_free(key);
    return ret;
}

static LPWSTR deformat_file(FORMAT *format, FORMSTR *str, BOOL shortname)
{
    LPWSTR key, ret = NULL;
    MSIFILE *file;
    DWORD size;

    key = msi_alloc((str->len + 1) * sizeof(WCHAR));
    lstrcpynW(key, get_formstr_data(format, str), str->len + 1);

    file = get_loaded_file(format->package, key);
    if (!file)
        goto done;

    if (!shortname)
    {
        ret = strdupW(file->TargetPath);
        goto done;
    }

    size = GetShortPathNameW(file->TargetPath, NULL, 0);
    if (size <= 0)
    {
        ret = strdupW(file->TargetPath);
        goto done;
    }

    size++;
    ret = msi_alloc(size * sizeof(WCHAR));
    GetShortPathNameW(file->TargetPath, ret, size);

done:
    msi_free(key);
    return ret;
}

static LPWSTR deformat_environment(FORMAT *format, FORMSTR *str)
{
    LPWSTR key, ret = NULL;
    DWORD sz;

    key = msi_alloc((str->len + 1) * sizeof(WCHAR));
    lstrcpynW(key, get_formstr_data(format, str), str->len + 1);

    sz  = GetEnvironmentVariableW(key, NULL ,0);
    if (sz <= 0)
        goto done;

    sz++;
    ret = msi_alloc(sz * sizeof(WCHAR));
    GetEnvironmentVariableW(key, ret, sz);

done:
    msi_free(key);
    return ret;
}

static LPWSTR deformat_literal(FORMAT *format, FORMSTR *str, BOOL *propfound,
                               BOOL *nonprop, int *type)
{
    LPCWSTR data = get_formstr_data(format, str);
    LPWSTR replaced = NULL;
    char ch = data[0];

    if (ch == '\\')
    {
        str->n++;
        if (str->len == 1)
        {
            str->len = 0;
            replaced = NULL;
        }
        else
        {
            str->len = 1;
            replaced = dup_formstr(format, str);
        }
    }
    else if (ch == '~')
    {
        if (str->len != 1)
            replaced = NULL;
        else
        {
            replaced = msi_alloc(sizeof(WCHAR));
            *replaced = '\0';
        }
    }
    else if (ch == '%' || ch == '#' || ch == '!' || ch == '$')
    {
        str->n++;
        str->len--;

        switch (ch)
        {
        case '%':
            replaced = deformat_environment(format, str); break;
        case '#':
            replaced = deformat_file(format, str, FALSE); break;
        case '!':
            replaced = deformat_file(format, str, TRUE); break;
        case '$':
            replaced = deformat_component(format, str); break;
        }

        *type = FORMAT_LITERAL;
    }
    else
    {
        replaced = deformat_property(format, str);
        *type = FORMAT_LITERAL;

        if (replaced)
            *propfound = TRUE;
        else
            format->propfailed = TRUE;
    }

    return replaced;
}

static LPWSTR build_default_format(const MSIRECORD* record)
{
    int i;  
    int count;
    LPWSTR rc, buf;
    static const WCHAR fmt[] = {'%','i',':',' ','%','s',' ',0};
    static const WCHAR fmt_null[] = {'%','i',':',' ',' ',0};
    static const WCHAR fmt_index[] = {'%','i',0};
    LPCWSTR str;
    WCHAR index[10];
    DWORD size, max_len, len;

    count = MSI_RecordGetFieldCount(record);

    max_len = MAX_PATH;
    buf = msi_alloc((max_len + 1) * sizeof(WCHAR));

    rc = NULL;
    size = 1;
    for (i = 1; i <= count; i++)
    {
        sprintfW(index, fmt_index, i);
        str = MSI_RecordGetString(record, i);
        len = (str) ? lstrlenW(str) : 0;
        len += (sizeof(fmt_null)/sizeof(fmt_null[0]) - 3) + lstrlenW(index);
        size += len;

        if (len > max_len)
        {
            max_len = len;
            buf = msi_realloc(buf, (max_len + 1) * sizeof(WCHAR));
            if (!buf) return NULL;
        }

        if (str)
            sprintfW(buf, fmt, i, str);
        else
            sprintfW(buf, fmt_null, i);

        if (!rc)
        {
            rc = msi_alloc(size * sizeof(WCHAR));
            lstrcpyW(rc, buf);
        }
        else
        {
            rc = msi_realloc(rc, size * sizeof(WCHAR));
            lstrcatW(rc, buf);
        }
    }

    msi_free(buf);
    return rc;
}

static BOOL format_is_number(WCHAR x)
{
    return ((x >= '0') && (x <= '9'));
}

static BOOL format_str_is_number(LPWSTR str)
{
    LPWSTR ptr;

    for (ptr = str; *ptr; ptr++)
        if (!format_is_number(*ptr))
            return FALSE;

    return TRUE;
}

static BOOL format_is_alpha(WCHAR x)
{
    return (!format_is_number(x) && x != '\0' &&
            x != '[' && x != ']' && x != '{' && x != '}');
}

static BOOL format_is_literal(WCHAR x)
{
    return (format_is_alpha(x) || format_is_number(x));
}

static int format_lex(FORMAT *format, FORMSTR **out)
{
    int type, len = 1;
    FORMSTR *str;
    LPCWSTR data;
    WCHAR ch;

    *out = NULL;

    if (!format->deformatted)
        return FORMAT_NULL;

    *out = msi_alloc_zero(sizeof(FORMSTR));
    if (!*out)
        return FORMAT_FAIL;

    str = *out;
    str->n = format->n;
    str->len = 1;
    data = get_formstr_data(format, str);

    ch = data[0];
    switch (ch)
    {
        case '{': type = FORMAT_LBRACE; break;
        case '}': type = FORMAT_RBRACE; break;
        case '[': type = FORMAT_LBRACK; break;
        case ']': type = FORMAT_RBRACK; break;
        case '~': type = FORMAT_PROPNULL; break;
        case '\0': type = FORMAT_NULL; break;

        default:
            type = 0;
    }

    if (type)
    {
        str->type = type;
        format->n++;
        return type;
    }

    if (ch == '\\')
    {
        while (data[len] && data[len] != ']')
            len++;

        type = FORMAT_ESCAPE;
    }
    else if (format_is_alpha(ch))
    {
        while (format_is_literal(data[len]))
            len++;

        type = FORMAT_LITERAL;
    }
    else if (format_is_number(ch))
    {
        while (format_is_number(data[len]))
            len++;

        type = FORMAT_NUMBER;

        if (data[len] != ']')
        {
            while (format_is_literal(data[len]))
                len++;

            type = FORMAT_LITERAL;
        }
    }
    else
    {
        ERR("Got unknown character %c(%x)\n", ch, ch);
        return FORMAT_ERROR;
    }

    format->n += len;
    str->len = len;
    str->type = type;

    return type;
}

static FORMSTR *format_replace(FORMAT *format, BOOL propfound, BOOL nonprop,
                               int oldsize, int type, LPWSTR replace)
{
    FORMSTR *ret;
    LPWSTR str, ptr;
    DWORD size = 0;
    int n;

    if (replace)
    {
        if (!*replace)
            size = 1;
        else
            size = lstrlenW(replace);
    }

    size -= oldsize;
    size = format->len + size + 1;

    if (size <= 1)
    {
        msi_free(format->deformatted);
        format->deformatted = NULL;
        format->len = 0;
        return NULL;
    }

    str = msi_alloc(size * sizeof(WCHAR));
    if (!str)
        return NULL;

    str[0] = '\0';
    memcpy(str, format->deformatted, format->n * sizeof(WCHAR));
    n = format->n;

    if (replace)
    {
        if (!*replace)
        {
            str[n] = '\0';
            n++;
        }
        else
        {
            lstrcpyW(&str[n], replace);
            n += lstrlenW(replace);
        }
    }

    ptr = &format->deformatted[format->n + oldsize];
    memcpy(&str[n], ptr, (lstrlenW(ptr) + 1) * sizeof(WCHAR));

    msi_free(format->deformatted);
    format->deformatted = str;
    format->len = size - 1;

    /* don't reformat the NULL */
    if (replace && !*replace)
        format->n++;

    if (!replace)
        return NULL;

    ret = msi_alloc_zero(sizeof(FORMSTR));
    if (!ret)
        return NULL;

    ret->len = lstrlenW(replace);
    ret->type = type;
    ret->n = format->n;
    ret->propfound = propfound;
    ret->nonprop = nonprop;

    return ret;
}

static LPWSTR replace_stack_group(FORMAT *format, STACK *values,
                                  BOOL *propfound, BOOL *nonprop,
                                  int *oldsize, int *type)
{
    LPWSTR replaced = NULL;
    FORMSTR *content;
    FORMSTR *node;
    int n;

    *nonprop = FALSE;
    *propfound = FALSE;

    node = stack_pop(values);
    n = node->n;
    *oldsize = node->len;
    msi_free(node);

    while ((node = stack_pop(values)))
    {
        *oldsize += node->len;

        if (node->nonprop)
            *nonprop = TRUE;

        if (node->propfound)
            *propfound = TRUE;

        msi_free(node);
    }

    content = msi_alloc_zero(sizeof(FORMSTR));
    content->n = n;
    content->len = *oldsize;
    content->type = FORMAT_LITERAL;

    if (!format->groupfailed && (*oldsize == 2 ||
        (format->propfailed && !*nonprop)))
    {
        msi_free(content);
        return NULL;
    }
    else if (format->deformatted[content->n + 1] == '{' &&
             format->deformatted[content->n + content->len - 2] == '}')
    {
        format->groupfailed = FALSE;
        content->len = 0;
    }
    else if (*propfound && !*nonprop &&
             !format->groupfailed && format->groups == 0)
    {
        content->n++;
        content->len -= 2;
    }
    else
    {
        if (format->groups != 0)
            format->groupfailed = TRUE;

        *nonprop = TRUE;
    }

    replaced = dup_formstr(format, content);
    *type = content->type;
    msi_free(content);

    if (format->groups == 0)
        format->propfailed = FALSE;

    return replaced;
}

static LPWSTR replace_stack_prop(FORMAT *format, STACK *values,
                                 BOOL *propfound, BOOL *nonprop,
                                 int *oldsize, int *type)
{
    LPWSTR replaced = NULL;
    FORMSTR *content;
    FORMSTR *node;
    int n;

    *propfound = FALSE;
    *nonprop = FALSE;

    node = stack_pop(values);
    n = node->n;
    *oldsize = node->len;
    *type = stack_peek(values)->type;
    msi_free(node);

    while ((node = stack_pop(values)))
    {
        *oldsize += node->len;

        if (*type != FORMAT_ESCAPE &&
            stack_peek(values) && node->type != *type)
            *type = FORMAT_LITERAL;

        msi_free(node);
    }

    content = msi_alloc_zero(sizeof(FORMSTR));
    content->n = n + 1;
    content->len = *oldsize - 2;
    content->type = *type;

    if (*type == FORMAT_NUMBER)
    {
        replaced = deformat_index(format, content);
        if (replaced)
            *propfound = TRUE;
        else
            format->propfailed = TRUE;

        if (replaced)
            *type = format_str_is_number(replaced) ?
                FORMAT_NUMBER : FORMAT_LITERAL;
    }
    else if (format->package)
    {
        replaced = deformat_literal(format, content, propfound, nonprop, type);
    }
    else
    {
        *nonprop = TRUE;
        content->n--;
        content->len += 2;
        replaced = dup_formstr(format, content);
    }

    msi_free(content);
    return replaced;
}

static UINT replace_stack(FORMAT *format, STACK *stack, STACK *values)
{
    LPWSTR replaced = NULL;
    FORMSTR *beg;
    FORMSTR *top;
    FORMSTR *node;
    BOOL propfound = FALSE;
    BOOL nonprop = FALSE;
    BOOL group = FALSE;
    int oldsize = 0;
    int type, n;

    node = stack_peek(values);
    type = node->type;
    n = node->n;

    if (type == FORMAT_LBRACK)
        replaced = replace_stack_prop(format, values, &propfound,
                                      &nonprop, &oldsize, &type);
    else if (type == FORMAT_LBRACE)
    {
        replaced = replace_stack_group(format, values, &propfound,
                                       &nonprop, &oldsize, &type);
        group = TRUE;
    }

    format->n = n;
    beg = format_replace(format, propfound, nonprop, oldsize, type, replaced);
    if (!beg)
        return ERROR_SUCCESS;

    msi_free(replaced);
    format->n = beg->n + beg->len;

    top = stack_peek(stack);
    if (top)
    {
        type = top->type;

        if ((type == FORMAT_LITERAL || type == FORMAT_NUMBER) &&
            type == beg->type)
        {
            top->len += beg->len;

            if (group)
                top->nonprop = FALSE;

            if (type == FORMAT_LITERAL)
                top->nonprop = beg->nonprop;

            if (beg->propfound)
                top->propfound = TRUE;

            msi_free(beg);
            return ERROR_SUCCESS;
        }
    }

    stack_push(stack, beg);
    return ERROR_SUCCESS;
}

static BOOL verify_format(LPWSTR data)
{
    int count = 0;

    while (*data)
    {
        if (*data == '[' && *(data - 1) != '\\')
            count++;
        else if (*data == ']')
            count--;

        data++;
    }

    if (count > 0)
        return FALSE;

    return TRUE;
}

static DWORD deformat_string_internal(MSIPACKAGE *package, LPCWSTR ptr, 
                                      WCHAR** data, DWORD *len,
                                      MSIRECORD* record, INT* failcount)
{
    FORMAT format;
    FORMSTR *str = NULL;
    STACK *stack, *temp;
    FORMSTR *node;
    int type;

    if (!ptr)
    {
        *data = NULL;
        *len = 0;
        return ERROR_SUCCESS;
    }

    *data = strdupW(ptr);
    *len = lstrlenW(ptr);

    ZeroMemory(&format, sizeof(FORMAT));
    format.package = package;
    format.record = record;
    format.deformatted = *data;
    format.len = *len;

    stack = create_stack();
    temp = create_stack();

    if (!verify_format(*data))
        return ERROR_SUCCESS;

    while ((type = format_lex(&format, &str)) != FORMAT_NULL)
    {
        if (type == FORMAT_LBRACK || type == FORMAT_LBRACE ||
            type == FORMAT_LITERAL || type == FORMAT_NUMBER ||
            type == FORMAT_ESCAPE || type == FORMAT_PROPNULL)
        {
            if (type == FORMAT_LBRACE)
            {
                format.propfailed = FALSE;
                format.groups++;
            }
            else if (type == FORMAT_ESCAPE &&
                     !stack_find(stack, FORMAT_LBRACK))
            {
                format.n -= str->len - 1;
                str->len = 1;
            }

            stack_push(stack, str);
        }
        else if (type == FORMAT_RBRACK || type == FORMAT_RBRACE)
        {
            if (type == FORMAT_RBRACE)
                format.groups--;

            stack_push(stack, str);

            if (stack_find(stack, left_type(type)))
            {
                do
                {
                    node = stack_pop(stack);
                    stack_push(temp, node);
                } while (node->type != left_type(type));

                replace_stack(&format, stack, temp);
            }
        }
    }

    *data = format.deformatted;
    *len = format.len;

    msi_free(str);
    free_stack(stack);
    free_stack(temp);

    return ERROR_SUCCESS;
}

UINT MSI_FormatRecordW( MSIPACKAGE* package, MSIRECORD* record, LPWSTR buffer,
                        LPDWORD size )
{
    LPWSTR deformated;
    LPWSTR rec;
    DWORD len;
    UINT rc = ERROR_INVALID_PARAMETER;

    TRACE("%p %p %p %p\n", package, record, buffer, size);

    rec = msi_dup_record_field(record,0);
    if (!rec)
        rec = build_default_format(record);

    TRACE("(%s)\n",debugstr_w(rec));

    deformat_string_internal(package, rec, &deformated, &len, record, NULL);
    if (buffer)
    {
        if (*size>len)
        {
            memcpy(buffer,deformated,len*sizeof(WCHAR));
            rc = ERROR_SUCCESS;
            buffer[len] = 0;
        }
        else
        {
            if (*size > 0)
            {
                memcpy(buffer,deformated,(*size)*sizeof(WCHAR));
                buffer[(*size)-1] = 0;
            }
            rc = ERROR_MORE_DATA;
        }
    }
    else
        rc = ERROR_SUCCESS;

    *size = len;

    msi_free(rec);
    msi_free(deformated);
    return rc;
}

UINT WINAPI MsiFormatRecordW( MSIHANDLE hInstall, MSIHANDLE hRecord, 
                              LPWSTR szResult, LPDWORD sz )
{
    UINT r = ERROR_INVALID_HANDLE;
    MSIPACKAGE *package;
    MSIRECORD *record;

    TRACE("%d %d %p %p\n", hInstall, hRecord, szResult, sz);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;
        BSTR value = NULL;
        awstring wstr;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (remote_package)
        {
            hr = IWineMsiRemotePackage_FormatRecord( remote_package, hRecord,
                                                     &value );
            if (FAILED(hr))
                goto done;

            wstr.unicode = TRUE;
            wstr.str.w = szResult;
            r = msi_strcpy_to_awstring( value, &wstr, sz );

done:
            IWineMsiRemotePackage_Release( remote_package );
            SysFreeString( value );

            if (FAILED(hr))
            {
                if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                    return HRESULT_CODE(hr);

                return ERROR_FUNCTION_FAILED;
            }

            return r;
        }
    }

    record = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );

    if (!record)
        return ERROR_INVALID_HANDLE;
    if (!sz)
    {
        msiobj_release( &record->hdr );
        if (szResult)
            return ERROR_INVALID_PARAMETER;
        else
            return ERROR_SUCCESS;
    }

    r = MSI_FormatRecordW( package, record, szResult, sz );
    msiobj_release( &record->hdr );
    if (package)
        msiobj_release( &package->hdr );
    return r;
}

UINT WINAPI MsiFormatRecordA( MSIHANDLE hInstall, MSIHANDLE hRecord,
                              LPSTR szResult, LPDWORD sz )
{
    UINT r;
    DWORD len, save;
    LPWSTR value;

    TRACE("%d %d %p %p\n", hInstall, hRecord, szResult, sz);

    if (!hRecord)
        return ERROR_INVALID_HANDLE;

    if (!sz)
    {
        if (szResult)
            return ERROR_INVALID_PARAMETER;
        else
            return ERROR_SUCCESS;
    }

    r = MsiFormatRecordW( hInstall, hRecord, NULL, &len );
    if (r != ERROR_SUCCESS)
        return r;

    value = msi_alloc(++len * sizeof(WCHAR));
    if (!value)
        return ERROR_OUTOFMEMORY;

    r = MsiFormatRecordW( hInstall, hRecord, value, &len );
    if (r != ERROR_SUCCESS)
        goto done;

    save = len + 1;
    len = WideCharToMultiByte(CP_ACP, 0, value, len + 1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, value, len, szResult, *sz, NULL, NULL);

    if (szResult && len > *sz)
    {
        if (*sz) szResult[*sz - 1] = '\0';
        r = ERROR_MORE_DATA;
    }

    *sz = save - 1;

done:
    msi_free(value);
    return r;
}
