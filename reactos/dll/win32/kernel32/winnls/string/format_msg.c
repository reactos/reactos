/*
 * FormatMessage implementation
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 2009 Alexandre Julliard
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

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(resource);

extern HMODULE kernel32_handle;

struct format_args
{
    ULONG_PTR    *args;
    __ms_va_list *list;
    int           last;
};

/* Messages used by FormatMessage
 *
 * They can be specified either directly or using a message ID and
 * loading them from the resource.
 *
 * The resourcedata has following format:
 * start:
 * 0: DWORD nrofentries
 * nrofentries * subentry:
 *	0: DWORD firstentry
 *	4: DWORD lastentry
 *      8: DWORD offset from start to the stringentries
 *
 * (lastentry-firstentry) * stringentry:
 * 0: WORD len (0 marks end)	[ includes the 4 byte header length ]
 * 2: WORD flags
 * 4: CHAR[len-4]
 * 	(stringentry i of a subentry refers to the ID 'firstentry+i')
 *
 * Yes, ANSI strings in win32 resources. Go figure.
 */

/**********************************************************************
 *	load_message    (internal)
 */
static LPWSTR load_message( HMODULE module, UINT id, WORD lang )
{
    MESSAGE_RESOURCE_ENTRY *mre;
    WCHAR *buffer;
    NTSTATUS status;

    TRACE("module = %p, id = %08x\n", module, id );

    if (!module) module = GetModuleHandleW( NULL );
    if ((status = RtlFindMessage( module, (ULONG)RT_MESSAGETABLE, lang, id, &mre )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    if (mre->Flags & MESSAGE_RESOURCE_UNICODE)
    {
        int len = (strlenW( (const WCHAR *)mre->Text ) + 1) * sizeof(WCHAR);
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len ))) return NULL;
        memcpy( buffer, mre->Text, len );
    }
    else
    {
        int len = MultiByteToWideChar( CP_ACP, 0, (const char *)mre->Text, -1, NULL, 0 );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
        MultiByteToWideChar( CP_ACP, 0, (const char*)mre->Text, -1, buffer, len );
    }
    TRACE("returning %s\n", wine_dbgstr_w(buffer));
    return buffer;
}

static LPWSTR search_message( DWORD flags, HMODULE module, UINT id, WORD lang )
{
    LPWSTR from = NULL;
    if (flags & FORMAT_MESSAGE_FROM_HMODULE)
        from = load_message( module, id, lang );
    if (!from && (flags & FORMAT_MESSAGE_FROM_SYSTEM))
    {
        /* Fold win32 hresult to its embedded error code. */
        if (HRESULT_SEVERITY(id) == SEVERITY_ERROR &&
            HRESULT_FACILITY(id) == FACILITY_WIN32)
        {
            id = HRESULT_CODE(id);
        }
        from = load_message( kernel32_handle, id, lang );
    }
    return from;
}

/**********************************************************************
 *	get_arg    (internal)
 */
static ULONG_PTR get_arg( int nr, DWORD flags, struct format_args *args )
{
    if (nr == -1) nr = args->last + 1;
    if (args->list)
    {
        if (!args->args) args->args = HeapAlloc( GetProcessHeap(), 0, 99 * sizeof(ULONG_PTR) );
        while (nr > args->last)
            args->args[args->last++] = va_arg( *args->list, ULONG_PTR );
    }
    if (nr > args->last) args->last = nr;
    return args->args[nr - 1];
}

/**********************************************************************
 *	format_insert    (internal)
 */
static LPCWSTR format_insert( BOOL unicode_caller, int insert, LPCWSTR format,
                              DWORD flags, struct format_args *args,
                              LPWSTR *result )
{
    static const WCHAR fmt_u[] = {'%','u',0};
    WCHAR *wstring = NULL, *p, fmt[256];
    ULONG_PTR arg;
    int size;

    if (*format != '!')  /* simple string */
    {
        arg = get_arg( insert, flags, args );
        if (unicode_caller || !arg)
        {
            static const WCHAR nullW[] = {'(','n','u','l','l',')',0};
            const WCHAR *str = (const WCHAR *)arg;

            if (!str) str = nullW;
            *result = HeapAlloc( GetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(WCHAR) );
            strcpyW( *result, str );
        }
        else
        {
            const char *str = (const char *)arg;
            DWORD length = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
            *result = HeapAlloc( GetProcessHeap(), 0, length * sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, str, -1, *result, length );
        }
        return format;
    }

    format++;
    p = fmt;
    *p++ = '%';

    while (*format == '0' ||
           *format == '+' ||
           *format == '-' ||
           *format == ' ' ||
           *format == '*' ||
           *format == '#')
    {
        if (*format == '*')
        {
            p += sprintfW( p, fmt_u, get_arg( insert, flags, args ));
            insert = -1;
            format++;
        }
        else *p++ = *format++;
    }
    while (isdigitW(*format)) *p++ = *format++;

    if (*format == '.')
    {
        *p++ = *format++;
        if (*format == '*')
        {
            p += sprintfW( p, fmt_u, get_arg( insert, flags, args ));
            insert = -1;
            format++;
        }
        else
            while (isdigitW(*format)) *p++ = *format++;
    }

    /* replicate MS bug: drop an argument when using va_list with width/precision */
    if (insert == -1 && args->list) args->last--;
    arg = get_arg( insert, flags, args );

    /* check for ascii string format */
    if ((format[0] == 'h' && format[1] == 's') ||
        (format[0] == 'h' && format[1] == 'S') ||
        (unicode_caller && format[0] == 'S') ||
        (!unicode_caller && format[0] == 's'))
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, (char *)arg, -1, NULL, 0 );
        wstring = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, (char *)arg, -1, wstring, len );
        arg = (ULONG_PTR)wstring;
        *p++ = 's';
    }
    /* check for ascii character format */
    else if ((format[0] == 'h' && format[1] == 'c') ||
             (format[0] == 'h' && format[1] == 'C') ||
             (unicode_caller && format[0] == 'C') ||
             (!unicode_caller && format[0] == 'c'))
    {
        char ch = arg;
        wstring = HeapAlloc( GetProcessHeap(), 0, 2 * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, &ch, 1, wstring, 1 );
        wstring[1] = 0;
        arg = (ULONG_PTR)wstring;
        *p++ = 's';
    }
    /* check for wide string format */
    else if ((format[0] == 'l' && format[1] == 's') ||
             (format[0] == 'l' && format[1] == 'S') ||
             (format[0] == 'w' && format[1] == 's') ||
             (!unicode_caller && format[0] == 'S'))
    {
        *p++ = 's';
    }
    /* check for wide character format */
    else if ((format[0] == 'l' && format[1] == 'c') ||
             (format[0] == 'l' && format[1] == 'C') ||
             (format[0] == 'w' && format[1] == 'c') ||
             (!unicode_caller && format[0] == 'C'))
    {
        *p++ = 'c';
    }
    /* FIXME: handle I64 etc. */
    else while (*format && *format != '!') *p++ = *format++;

    *p = 0;
    size = 256;
    for (;;)
    {
        WCHAR *ret = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );
        int needed = snprintfW( ret, size, fmt, arg );
        if (needed == -1 || needed >= size)
        {
            HeapFree( GetProcessHeap(), 0, ret );
            size = max( needed + 1, size * 2 );
        }
        else
        {
            *result = ret;
            break;
        }
    }

    while (*format && *format != '!') format++;
    if (*format == '!') format++;

    HeapFree( GetProcessHeap(), 0, wstring );
    return format;
}

struct _format_message_data
{
    LPWSTR formatted;
    DWORD size;
    LPWSTR t;
    LPWSTR space;
    BOOL inspace;
    DWORD width, w;
};

static void format_add_char(struct _format_message_data *fmd, WCHAR c)
{
    *fmd->t++ = c;
    if (fmd->width && fmd->width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
    {
        switch (c) {
        case '\r':
        case '\n':
            fmd->space = NULL;
            fmd->inspace = FALSE;
            fmd->w = 0;
            break;
        case ' ':
            if (!fmd->inspace)
                fmd->space = fmd->t - 1;
            fmd->inspace = TRUE;
            fmd->w++;
            break;
        default:
            fmd->inspace = FALSE;
            fmd->w++;
        }
        if (fmd->w == fmd->width) {
            LPWSTR notspace;
            if (fmd->space) {
                notspace = fmd->space;
                while (notspace != fmd->t && *notspace == ' ')
                    notspace++;
            } else
                notspace = fmd->space = fmd->t;
            fmd->w = fmd->t - notspace;
            memmove(fmd->space+2, notspace, fmd->w * sizeof(*fmd->t));
            *fmd->space++ = '\r';
            *fmd->space++ = '\n';
            fmd->t = fmd->space + fmd->w;
            fmd->space = NULL;
            fmd->inspace = FALSE;
        }
    }
    if ((DWORD)(fmd->t - fmd->formatted) == fmd->size) {
        DWORD_PTR ispace = fmd->space - fmd->formatted;
        /* Allocate two extra characters so we can insert a '\r\n' in
         * the middle of a word.
         */
        fmd->formatted = HeapReAlloc(GetProcessHeap(), 0, fmd->formatted, (fmd->size * 2 + 2) * sizeof(WCHAR));
        fmd->t = fmd->formatted + fmd->size;
        if (fmd->space)
            fmd->space = fmd->formatted + ispace;
        fmd->size *= 2;
    }
}

/**********************************************************************
 *	format_message    (internal)
 */
static LPWSTR format_message( BOOL unicode_caller, DWORD dwFlags, LPCWSTR fmtstr,
                              struct format_args *format_args )
{
    struct _format_message_data fmd;
    LPCWSTR f;
    BOOL eos = FALSE;

    fmd.size = 100;
    fmd.formatted = fmd.t = HeapAlloc( GetProcessHeap(), 0, (fmd.size + 2) * sizeof(WCHAR) );

    fmd.width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    fmd.w = 0;
    fmd.inspace = FALSE;
    fmd.space = NULL;
    f = fmtstr;
    while (*f && !eos) {
        if (*f=='%') {
            int insertnr;
            WCHAR *str,*x;

            f++;
            switch (*f) {
            case '1':case '2':case '3':case '4':case '5':
            case '6':case '7':case '8':case '9':
                if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS)
                    goto ignore_inserts;
                else if (((dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY) && !format_args->args) ||
                        (!(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY) && !format_args->list))
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    HeapFree(GetProcessHeap(), 0, fmd.formatted);
                    return NULL;
                }
                insertnr = *f-'0';
                switch (f[1]) {
                case '0':case '1':case '2':case '3':
                case '4':case '5':case '6':case '7':
                case '8':case '9':
                    f++;
                    insertnr = insertnr*10 + *f-'0';
                    f++;
                    break;
                default:
                    f++;
                    break;
                }
                f = format_insert( unicode_caller, insertnr, f, dwFlags, format_args, &str );
                for (x = str; *x; x++) format_add_char(&fmd, *x);
                HeapFree( GetProcessHeap(), 0, str );
                break;
            case 'n':
                format_add_char(&fmd, '\r');
                format_add_char(&fmd, '\n');
                f++;
                break;
            case 'r':
                format_add_char(&fmd, '\r');
                f++;
                break;
            case 't':
                format_add_char(&fmd, '\t');
                f++;
                break;
            case '0':
                eos = TRUE;
                f++;
                break;
            case '\0':
                SetLastError(ERROR_INVALID_PARAMETER);
                HeapFree(GetProcessHeap(), 0, fmd.formatted);
                return NULL;
            ignore_inserts:
            default:
                if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS)
                    format_add_char(&fmd, '%');
                format_add_char(&fmd, *f++);
                break;
            }
        } else {
            WCHAR ch = *f;
            f++;
            if (ch == '\r') {
                if (*f == '\n')
                    f++;
                if(fmd.width)
                    format_add_char(&fmd, ' ');
                else
                {
                    format_add_char(&fmd, '\r');
                    format_add_char(&fmd, '\n');
                }
            } else {
                if (ch == '\n')
                {
                    if(fmd.width)
                        format_add_char(&fmd, ' ');
                    else
                    {
                        format_add_char(&fmd, '\r');
                        format_add_char(&fmd, '\n');
                    }
                }
                else
                    format_add_char(&fmd, ch);
            }
        }
    }
    *fmd.t = '\0';

    return fmd.formatted;
}

/***********************************************************************
 *           FormatMessageA   (KERNEL32.@)
 */
DWORD WINAPI FormatMessageA(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	__ms_va_list* args )
{
    struct format_args format_args;
    DWORD ret = 0;
    LPWSTR	target;
    DWORD	destlength;
    LPWSTR	from;

    TRACE("(0x%x,%p,%d,0x%x,%p,%d,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);

    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        if (!lpBuffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        else
            *(LPSTR *)lpBuffer = NULL;
    }

    if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
    {
        format_args.args = (ULONG_PTR *)args;
        format_args.list = NULL;
        format_args.last = 0;
    }
    else
    {
        format_args.args = NULL;
        format_args.list = args;
        format_args.last = 0;
    }

    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        DWORD length = MultiByteToWideChar(CP_ACP, 0, lpSource, -1, NULL, 0);
        from = HeapAlloc( GetProcessHeap(), 0, length * sizeof(WCHAR) );
        MultiByteToWideChar(CP_ACP, 0, lpSource, -1, from, length);
    }
    else if (dwFlags & (FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM))
    {
        from = search_message( dwFlags, (HMODULE)lpSource, dwMessageId, dwLanguageId );
        if (!from) return 0;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    target = format_message( FALSE, dwFlags, from, &format_args );
    if (!target)
        goto failure;

    TRACE("-- %s\n", debugstr_w(target));

    /* Only try writing to an output buffer if there are processed characters
     * in the temporary output buffer. */
    if (*target)
    {
        destlength = WideCharToMultiByte(CP_ACP, 0, target, -1, NULL, 0, NULL, NULL);
        if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
        {
            LPSTR buf = LocalAlloc(LMEM_ZEROINIT, max(nSize, destlength));
            WideCharToMultiByte(CP_ACP, 0, target, -1, buf, destlength, NULL, NULL);
            *((LPSTR*)lpBuffer) = buf;
        }
        else
        {
            if (nSize < destlength)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                goto failure;
            }
            WideCharToMultiByte(CP_ACP, 0, target, -1, lpBuffer, destlength, NULL, NULL);
        }
        ret = destlength - 1; /* null terminator */
    }

failure:
    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    if (!(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)) HeapFree( GetProcessHeap(), 0, format_args.args );
    TRACE("-- returning %u\n", ret);
    return ret;
}

/***********************************************************************
 *           FormatMessageW   (KERNEL32.@)
 */
DWORD WINAPI FormatMessageW(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	__ms_va_list* args )
{
    struct format_args format_args;
    DWORD ret = 0;
    LPWSTR target;
    DWORD talloced;
    LPWSTR from;

    TRACE("(0x%x,%p,%d,0x%x,%p,%d,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);

    if (!lpBuffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
        *(LPWSTR *)lpBuffer = NULL;

    if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
    {
        format_args.args = (ULONG_PTR *)args;
        format_args.list = NULL;
        format_args.last = 0;
    }
    else
    {
        format_args.args = NULL;
        format_args.list = args;
        format_args.last = 0;
    }

    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        from = HeapAlloc( GetProcessHeap(), 0, (strlenW(lpSource) + 1) *
            sizeof(WCHAR) );
        strcpyW( from, lpSource );
    }
    else if (dwFlags & (FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM))
    {
        from = search_message( dwFlags, (HMODULE)lpSource, dwMessageId, dwLanguageId );
        if (!from) return 0;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    target = format_message( TRUE, dwFlags, from, &format_args );
    if (!target)
        goto failure;

    talloced = strlenW(target)+1;
    TRACE("-- %s\n",debugstr_w(target));

    /* Only allocate a buffer if there are processed characters in the
     * temporary output buffer. If a caller supplies the buffer, then
     * a null terminator will be written to it. */
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        if (*target)
        {
            /* nSize is the MINIMUM size */
            *((LPVOID*)lpBuffer) = LocalAlloc(LMEM_ZEROINIT, max(nSize, talloced)*sizeof(WCHAR));
            strcpyW(*(LPWSTR*)lpBuffer, target);
        }
    }
    else
    {
        if (nSize < talloced)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto failure;
        }
        strcpyW(lpBuffer, target);
    }

    ret = talloced - 1; /* null terminator */
failure:
    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    if (!(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)) HeapFree( GetProcessHeap(), 0, format_args.args );
    TRACE("-- returning %u\n", ret);
    return ret;
}
