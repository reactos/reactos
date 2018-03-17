/*
 * Copyright 2016 Michael Müller
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "inseng_private.h"

#include "wine/list.h"

struct inf_value
{
    struct list entry;
    char *key;
    char *value;

    struct inf_section *section;
};

struct inf_section
{
    struct list entry;
    char *name;
    struct list values;

    struct inf_file *file;
};

struct inf_file
{
    char *content;
    DWORD size;
    struct list sections;
};

static void inf_value_free(struct inf_value *value)
{
    heap_free(value);
}

static void inf_section_free(struct inf_section *section)
{
    struct inf_value *val, *val_next;
    LIST_FOR_EACH_ENTRY_SAFE(val, val_next, &section->values, struct inf_value, entry)
    {
        list_remove(&val->entry);
        inf_value_free(val);
    }

    heap_free(section);
}

static const char *get_substitution(struct inf_file *inf, const char *name, int len)
{
    struct inf_section *sec;
    struct inf_value *value = NULL;

    sec = inf_get_section(inf, "Strings");
    if (!sec) return NULL;

    while (inf_section_next_value(sec, &value))
    {
        if (strlen(value->key) == len && !strncasecmp(value->key, name, len))
            return value->value;
    }

    return NULL;
}

static int expand_variables_buffer(struct inf_file *inf, const char *str, char *output)
{
    const char *p, *var_start = NULL;
    int var_len = 0, len = 0;
    const char *substitution;

    for (p = str; *p; p++)
    {
        if (*p != '%')
        {
            if (var_start)
                var_len++;
            else
            {
                if (output)
                    *output++ = *p;
                len++;
            }

            continue;
        }

        if (!var_start)
        {
            var_start = p;
            var_len = 0;

            continue;
        }

        if (!var_len)
        {
            /* just an escaped % */
            if (output)
                *output++ = '%';
            len += 1;

            var_start = NULL;
            continue;
        }

        substitution = get_substitution(inf, var_start + 1, var_len);
        if (!substitution)
        {
            if (output)
            {
                memcpy(output, var_start, var_len + 2);
                output += var_len + 2;
            }
            len += var_len + 2;
        }
        else
        {
            int sub_len = strlen(substitution);

            if (output)
            {
                memcpy(output, substitution, sub_len);
                output += sub_len;
            }
            len += sub_len;
        }

         var_start = NULL;
    }

    if (output) *output = 0;
    return len + 1;
}

static char *expand_variables(struct inf_file *inf, const char *str)
{
    char *buffer;
    int len;

    len = expand_variables_buffer(inf, str, NULL);
    buffer = heap_alloc(len);
    if (!len) return NULL;

    expand_variables_buffer(inf, str, buffer);
    return buffer;
}

void inf_free(struct inf_file *inf)
{
    struct inf_section *sec, *sec_next;
    LIST_FOR_EACH_ENTRY_SAFE(sec, sec_next, &inf->sections, struct inf_section, entry)
    {
        list_remove(&sec->entry);
        inf_section_free(sec);
    }

    heap_free(inf->content);
    heap_free(inf);
}

BOOL inf_next_section(struct inf_file *inf, struct inf_section **sec)
{
    struct list *next_entry, *cur_position;

    if (*sec)
        cur_position = &(*sec)->entry;
    else
        cur_position = &inf->sections;

    next_entry = list_next(&inf->sections, cur_position);
    if (!next_entry) return FALSE;

    *sec = CONTAINING_RECORD(next_entry, struct inf_section, entry);
    return TRUE;
}

struct inf_section *inf_get_section(struct inf_file *inf, const char *name)
{
    struct inf_section *sec = NULL;

    while (inf_next_section(inf, &sec))
    {
        if (!strcasecmp(sec->name, name))
            return sec;
    }

    return NULL;
}

char *inf_section_get_name(struct inf_section *section)
{
    return strdupA(section->name);
}

BOOL inf_section_next_value(struct inf_section *sec, struct inf_value **value)
{
    struct list *next_entry, *cur_position;

    if (*value)
        cur_position = &(*value)->entry;
    else
        cur_position = &sec->values;

    next_entry = list_next(&sec->values, cur_position);
    if (!next_entry) return FALSE;

    *value = CONTAINING_RECORD(next_entry, struct inf_value, entry);
    return TRUE;
}

struct inf_value *inf_get_value(struct inf_section *sec, const char *key)
{
    struct inf_value *value = NULL;

    while (inf_section_next_value(sec, &value))
    {
        if (!strcasecmp(value->key, key))
            return value;
    }

    return NULL;
}

char *inf_value_get_key(struct inf_value *value)
{
    return strdupA(value->key);
}

char *inf_value_get_value(struct inf_value *value)
{
    return expand_variables(value->section->file, value->value);
}

char *trim(char *str, char **last_chr, BOOL strip_quotes)
{
    char *last;

    for (; *str; str++)
    {
        if (*str != '\t' && *str != ' ')
            break;
    }

    if (!*str)
    {
        if (last_chr) *last_chr = str;
        return str;
    }

    last = str + strlen(str) - 1;

    for (; last > str; last--)
    {
        if (*last != '\t' && *last != ' ')
            break;
        *last = 0;
    }

    if (strip_quotes && last != str)
    {
        if (*last == '"' && *str == '"')
        {
            str++;
            *last = 0;
        }
    }

    if (last_chr) *last_chr = last;
    return str;
}

static char *get_next_line(char **str, char **last_chr)
{
    BOOL in_next_line = FALSE;
    char *start, *next;

    start = *str;
    if (!start || !*start) return NULL;

    for (next = start; *next; next++)
    {
        if (*next == '\n' || *next == '\r')
        {
            *next = 0;
            in_next_line = TRUE;
        }
        else if (in_next_line)
        {
            break;
        }
    }

    *str = next;
    return trim(start, last_chr, FALSE);
}

/* This function only fails in case of an memory allocation error
 * and does not touch section in case the parsing failed. */
static HRESULT inf_section_parse(struct inf_file *inf, char *line, char *last_chr, struct inf_section **section)
{
    struct inf_section *sec;
    char *comment;
    char *name;

    if (*line != '[')
        return S_OK;

    line++;

    comment = strchr(line, ';');
    if (comment)
    {
        *comment = 0;
        line = trim(line, &last_chr, FALSE);
    }

    if (*last_chr != ']')
        return S_OK;

    *last_chr = 0;
    name = trim(line, NULL, FALSE);
    if (!name) return S_OK;

    sec = heap_alloc_zero(sizeof(*sec));
    if (!sec) return E_OUTOFMEMORY;

    sec->name = name;
    sec->file = inf;
    list_init(&sec->values);

    list_add_tail(&inf->sections, &sec->entry);

    *section = sec;
    return S_OK;
}

static HRESULT inf_value_parse(struct inf_section *sec, char *line)
{
    struct inf_value *key_val;
    char *key, *value, *del;

    del = strchr(line, '=');
    if (!del) return S_OK;

    *del = 0;
    key = line;
    value = del + 1;

    key = trim(key, NULL, FALSE);
    value = trim(value, NULL, TRUE);

    key_val = heap_alloc_zero(sizeof(*key_val));
    if (!key_val) return E_OUTOFMEMORY;

    key_val->key = key;
    key_val->value = value;
    key_val->section = sec;

    list_add_tail(&sec->values, &key_val->entry);
    return S_OK;
}

static HRESULT inf_process_content(struct inf_file *inf)
{
    struct inf_section *section = NULL;
    char *content = inf->content;
    char *line, *last_chr;
    HRESULT hr = S_OK;

    while (SUCCEEDED(hr) && (line = get_next_line(&content, &last_chr)))
    {
        if (*line == '[')
            hr = inf_section_parse(inf, line, last_chr, &section);
        else if (strchr(line, '=') && section)
            hr = inf_value_parse(section, line);
    }

    return hr;
}

HRESULT inf_load(const char *path, struct inf_file **inf_file)
{
    LARGE_INTEGER file_size;
    struct inf_file *inf;
    HRESULT hr = E_FAIL;
    HANDLE file;
    DWORD read;

    file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return E_FAIL;

    inf = heap_alloc_zero(sizeof(*inf));
    if (!inf) goto error;

    if (!GetFileSizeEx(file, &file_size))
        goto error;

    inf->size = file_size.QuadPart;

    inf->content = heap_alloc_zero(inf->size);
    if (!inf->content) goto error;

    list_init(&inf->sections);

    if (!ReadFile(file, inf->content, inf->size, &read, NULL) || read != inf->size)
        goto error;

    hr = inf_process_content(inf);
    if (FAILED(hr)) goto error;

    CloseHandle(file);
    *inf_file = inf;
    return S_OK;

error:
    if (inf) inf_free(inf);
    CloseHandle(file);
    return hr;
}
