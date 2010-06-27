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
#include "wine/unicode.h"

struct format_args
{
    ULONG_PTR    *args;
    __ms_va_list *list;
    int           last;
};

static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};

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

static const WCHAR PCNTFMTWSTR[] = { '%','%','%','s',0 };
static const WCHAR FMTWSTR[] = { '%','s',0 };

/**********************************************************************
 *	load_messageW		(internal)
 */
static LPWSTR load_messageW( HMODULE module, UINT id, WORD lang )
{
    PMESSAGE_RESOURCE_ENTRY mre;
    WCHAR *buffer;
    NTSTATUS status;

    DPRINT("module = %p, id = %08x\n", module, id );

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
    DPRINT("returning %S\n", buffer);
    return buffer;
}


/**********************************************************************
 *	load_messageA		(internal)
 */
static LPSTR load_messageA( HMODULE module, UINT id, WORD lang )
{
    PMESSAGE_RESOURCE_ENTRY mre;
    char *buffer;
    NTSTATUS status;

    DPRINT("module = %p, id = %08x\n", module, id );

    if (!module) module = GetModuleHandleW( NULL );
    if ((status = RtlFindMessage( module, (ULONG)RT_MESSAGETABLE, lang, id, &mre )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    if (mre->Flags & MESSAGE_RESOURCE_UNICODE)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, (const WCHAR *)mre->Text, -1, NULL, 0, NULL, NULL );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len ))) return NULL;
        WideCharToMultiByte( CP_ACP, 0, (const WCHAR *)mre->Text, -1, buffer, len, NULL, NULL );
    }
    else
    {
        int len = strlen((const char*)mre->Text) + 1;
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len ))) return NULL;
        memcpy( buffer, mre->Text, len );
    }
    DPRINT("returning %s\n", buffer);
    return buffer;
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
 *	format_insertA    (internal)
 */
static LPCSTR format_insertA( int insert, LPCSTR format, DWORD flags,
                              struct format_args *args, LPSTR *result )
{
    char *astring = NULL, *p, fmt[256];
    ULONG_PTR arg;
    int size;

    if (*format != '!')  /* simple string */
    {
        char *str = (char *)get_arg( insert, flags, args );
        *result = HeapAlloc( GetProcessHeap(), 0, strlen(str) + 1 );
        strcpy( *result, str );
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
            p += sprintf( p, "%lu", get_arg( insert, flags, args ));
            insert = -1;
            format++;
        }
        else *p++ = *format++;
    }
    while (isdigit(*format)) *p++ = *format++;

    if (*format == '.')
    {
        *p++ = *format++;
        if (*format == '*')
        {
            p += sprintf( p, "%lu", get_arg( insert, flags, args ));
            insert = -1;
            format++;
        }
        else
            while (isdigit(*format)) *p++ = *format++;
    }

    /* replicate MS bug: drop an argument when using va_list with width/precision */
    if (insert == -1 && args->list) args->last--;
    arg = get_arg( insert, flags, args );

    /* check for wide string format */
    if ((format[0] == 'l' && format[1] == 's') ||
        (format[0] == 'l' && format[1] == 'S') ||
        (format[0] == 'w' && format[1] == 's') ||
        (format[0] == 'S'))
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, (WCHAR *)arg, -1, /*FIXME*/
                                         NULL, 0, NULL, NULL );
        astring = HeapAlloc( GetProcessHeap(), 0, len );
        WideCharToMultiByte( CP_ACP, 0, (WCHAR *)arg, -1, astring, len, NULL, NULL );
        arg = (ULONG_PTR)astring;
        *p++ = 's';
    }
    /* check for wide character format */
    else if ((format[0] == 'l' && format[1] == 'c') ||
             (format[0] == 'l' && format[1] == 'C') ||
             (format[0] == 'w' && format[1] == 'c') ||
             (format[0] == 'C'))
    {
        WCHAR ch = arg;
        DWORD len = WideCharToMultiByte( CP_ACP, 0, &ch, 1, NULL, 0, NULL, NULL );
        astring = HeapAlloc( GetProcessHeap(), 0, len + 1 );
        WideCharToMultiByte( CP_ACP, 0, &ch, 1, astring, len, NULL, NULL );
        astring[len] = 0;
        arg = (ULONG_PTR)astring;
        *p++ = 's';
    }
    /* check for ascii string format */
    else if ((format[0] == 'h' && format[1] == 's') ||
             (format[0] == 'h' && format[1] == 'S'))
    {
        *p++ = 's';
    }
    /* check for ascii character format */
    else if ((format[0] == 'h' && format[1] == 'c') ||
             (format[0] == 'h' && format[1] == 'C'))
    {
        *p++ = 'c';
    }
    /* FIXME: handle I64 etc. */
    else while (*format && *format != '!') *p++ = *format++;

    *p = 0;
    size = 256;
    for (;;)
    {
        char *ret = HeapAlloc( GetProcessHeap(), 0, size );
        int needed = snprintf( ret, size, fmt, arg );
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

    HeapFree( GetProcessHeap(), 0, astring );
    return format;
}


/**********************************************************************
 *	format_insertW    (internal)
 */
static LPCWSTR format_insertW( int insert, LPCWSTR format, DWORD flags,
                               struct format_args *args, LPWSTR *result )
{
    static const WCHAR fmt_lu[] = {'%','l','u',0};
    WCHAR *wstring = NULL, *p, fmt[256];
    ULONG_PTR arg;
    int size;

    if (*format != '!')  /* simple string */
    {
        WCHAR *str = (WCHAR *)get_arg( insert, flags, args );
        *result = HeapAlloc( GetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(WCHAR) );
        strcpyW( *result, str );
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
            p += sprintfW( p, fmt_lu, get_arg( insert, flags, args ));
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
            p += sprintfW( p, fmt_lu, get_arg( insert, flags, args ));
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
        (format[0] == 'S'))
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, (char *)arg, -1, /*FIXME*/ NULL, 0 );
        wstring = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, (char *)arg, -1, wstring, len );
        arg = (ULONG_PTR)wstring;
        *p++ = 's';
    }
    /* check for ascii character format */
    else if ((format[0] == 'h' && format[1] == 'c') ||
             (format[0] == 'h' && format[1] == 'C') ||
             (format[0] == 'C'))
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
             (format[0] == 'w' && format[1] == 's'))
    {
        *p++ = 's';
    }
    /* check for wide character format */
    else if ((format[0] == 'l' && format[1] == 'c') ||
             (format[0] == 'l' && format[1] == 'C') ||
             (format[0] == 'w' && format[1] == 'c'))
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


/***********************************************************************
 *           FormatMessageA   (KERNEL32.@)
 * FIXME: missing wrap,
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
    LPSTR	target,t;
    DWORD	talloced;
    LPSTR	from;
    LPCSTR f;
    DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL    eos = FALSE;
    CHAR	ch;
    HMODULE kernel32_handle = GetModuleHandleW(kernel32W);

    DPRINT("(0x%x,%p,%d,0x%x,%p,%d,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (!lpBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
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

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        DPRINT1("FIXME: line wrapping (%u) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        from = HeapAlloc( GetProcessHeap(), 0, strlen(lpSource) + 1 );
        strcpy( from, lpSource );
    }
    else {
        from = NULL;
        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
            from = load_messageA( (HMODULE)lpSource, dwMessageId, dwLanguageId );
        if (!from && (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM))
            from = load_messageA( kernel32_handle, dwMessageId, dwLanguageId );
        if (!from) return 0;
    }
    target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    t	= target;
    talloced= 100;

#define ADD_TO_T(c) do { \
        *t++=c;\
        if ((DWORD)(t-target) == talloced) {\
            target = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
            t = target+talloced;\
            talloced*=2;\
      }\
} while (0)

    if (from) {
        f=from;
        if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) {
            while (*f && !eos)
                ADD_TO_T(*f++);
        }
        else {
            while (*f && !eos) {
                if (*f=='%') {
                    int insertnr;
                    char *str,*x;

                    f++;
                    if (!*f) {
                        ADD_TO_T('%');
                        continue;
                    }
                    switch (*f) {
                    case '1':case '2':case '3':case '4':case '5':
                    case '6':case '7':case '8':case '9':
                        insertnr=*f-'0';
                        switch (f[1]) {
                        case '0':case '1':case '2':case '3':
                        case '4':case '5':case '6':case '7':
                        case '8':case '9':
                            f++;
                            insertnr=insertnr*10+*f-'0';
                            f++;
                            break;
                        default:
                            f++;
                            break;
                        }
                        f = format_insertA( insertnr, f, dwFlags, &format_args, &str );
                        for (x = str; *x; x++) ADD_TO_T(*x);
                        HeapFree( GetProcessHeap(), 0, str );
                        break;
                    case 'n':
                        ADD_TO_T('\r');
                        ADD_TO_T('\n');
                        f++;
                        break;
                    case '0':
                        eos = TRUE;
                        f++;
                        break;
                    default:
                        ADD_TO_T(*f++);
                        break;
                    }
                } else {
                    ch = *f;
                    f++;
                    if (ch == '\r') {
                        if (*f == '\n')
                            f++;
                        if(width)
                            ADD_TO_T(' ');
                        else
                        {
                            ADD_TO_T('\r');
                            ADD_TO_T('\n');
                        }
                    } else {
                        if (ch == '\n')
                        {
                            if(width)
                                ADD_TO_T(' ');
                            else
                            {
                                ADD_TO_T('\r');
                                ADD_TO_T('\n');
                            }
                        }
                        else
                            ADD_TO_T(ch);
                    }
                }
            }
        }
        *t='\0';
    }
    talloced = strlen(target)+1;
    if (nSize && talloced<nSize) {
        target = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    }
    DPRINT("-- %S\n", target);
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        *((LPVOID*)lpBuffer) = LocalAlloc(LMEM_ZEROINIT,max(nSize, talloced));
        memcpy(*(LPSTR*)lpBuffer,target,talloced);
    } else {
        lstrcpynA(lpBuffer,target,nSize);
    }
    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    if (!(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)) HeapFree( GetProcessHeap(), 0, format_args.args );
    ret = (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? strlen(*(LPSTR*)lpBuffer) : strlen(lpBuffer);
    DPRINT("-- returning %d\n", ret);
    return ret;
}
#undef ADD_TO_T


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
    LPWSTR target,t;
    DWORD talloced;
    LPWSTR from;
    LPCWSTR f;
    DWORD width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL eos = FALSE;
    WCHAR ch;
    HMODULE kernel32_handle = GetModuleHandleW(kernel32W);

    DPRINT("(0x%x,%p,%d,0x%x,%p,%d,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (!lpBuffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
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

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        DPRINT1("FIXME: line wrapping not supported.\n");
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        from = HeapAlloc( GetProcessHeap(), 0, (strlenW(lpSource) + 1) *
            sizeof(WCHAR) );
        strcpyW( from, lpSource );
    }
    else {
        from = NULL;
        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
            from = load_messageW( (HMODULE)lpSource, dwMessageId, dwLanguageId );
        if (!from && (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM))
            from = load_messageW( kernel32_handle, dwMessageId, dwLanguageId );
        if (!from) return 0;
    }
    target = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100 * sizeof(WCHAR) );
    t = target;
    talloced= 100;

#define ADD_TO_T(c)  do {\
    *t++=c;\
    if ((DWORD)(t-target) == talloced) {\
        target = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2*sizeof(WCHAR));\
        t = target+talloced;\
        talloced*=2;\
    } \
} while (0)

    if (from) {
        f=from;
        if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) {
            while (*f && !eos)
                ADD_TO_T(*f++);
        }
        else {
            while (*f && !eos) {
                if (*f=='%') {
                    int insertnr;
                    WCHAR *str,*x;

                    f++;
                    if (!*f) {
                        ADD_TO_T('%');
                        continue;
                    }

                    switch (*f) {
                    case '1':case '2':case '3':case '4':case '5':
                    case '6':case '7':case '8':case '9':
                        insertnr=*f-'0';
                        switch (f[1]) {
                        case '0':case '1':case '2':case '3':
                        case '4':case '5':case '6':case '7':
                        case '8':case '9':
                            f++;
                            insertnr=insertnr*10+*f-'0';
                            f++;
                            break;
                        default:
                            f++;
                            break;
                        }
                        f = format_insertW( insertnr, f, dwFlags, &format_args, &str );
                        for (x = str; *x; x++) ADD_TO_T(*x);
                        HeapFree( GetProcessHeap(), 0, str );
                        break;
                    case 'n':
                        ADD_TO_T('\r');
                        ADD_TO_T('\n');
                        f++;
                        break;
                    case '0':
                        eos = TRUE;
                        f++;
                        break;
                    default:
                        ADD_TO_T(*f++);
                        break;
                    }
                } else {
                    ch = *f;
                    f++;
                    if (ch == '\r') {
                        if (*f == '\n')
                            f++;
                        if(width)
                            ADD_TO_T(' ');
                        else
                        {
                            ADD_TO_T('\r');
                            ADD_TO_T('\n');
                        }
                    } else {
                        if (ch == '\n')
                        {
                            if(width)
                                ADD_TO_T(' ');
                            else
                            {
                                ADD_TO_T('\r');
                                ADD_TO_T('\n');
                            }
                        }
                        else
                            ADD_TO_T(ch);
                    }
                }
            }
        }
        *t='\0';
    }
    talloced = strlenW(target)+1;
    if (nSize && talloced<nSize)
        target = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize*sizeof(WCHAR));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        DWORD len = strlenW(target) + 1;
        *((LPVOID*)lpBuffer) = LocalAlloc(LMEM_ZEROINIT,len*sizeof(WCHAR));
        strcpyW(*(LPWSTR*)lpBuffer, target);
    }
    else lstrcpynW(lpBuffer, target, nSize);

    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    if (!(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)) HeapFree( GetProcessHeap(), 0, format_args.args );
    DPRINT("ret=%S\n", (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        *(LPWSTR*)lpBuffer : lpBuffer);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlenW(*(LPWSTR*)lpBuffer):
            strlenW(lpBuffer);
}
#undef ADD_TO_T
