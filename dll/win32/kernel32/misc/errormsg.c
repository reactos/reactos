/* $Id$
 *
 * reactos/lib/kernel32/misc/errormsg.c
 * Wine calls this file now as kernel/format_msg.c
 *
 */
/*
 * FormatMessage implementation
 *
 * Copyright 1996 Marcus Meissner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <k32.h>

#define NDEBUG

#include <debug.h>
#include "wine/unicode.h"

#define TRACE DPRINT
#define FIXME DPRINT

static const WCHAR PCNTFMTWSTR[] = { '%','%','%','s',0 };
static const WCHAR FMTWSTR[] = { '%','s',0 };
static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};

/* strdup macros */
/* DO NOT USE IT!!  it will go away soon */

__inline static LPSTR HEAP_strdupWtoA( HANDLE heap, DWORD flags, LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = RtlAllocateHeap(RtlGetProcessHeap(), flags, len );
    if(ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

/* Messages...used by FormatMessage32* (KERNEL32.something)
 *
 * They can be specified either directly or using a message ID and
 * loading them from the resource.
 *
 * The resourcedata has following format:
 * start:
 * 0: DWORD nrofentries
 * nrofentries * subentry:
 *      0: DWORD firstentry
 *      4: DWORD lastentry
 *      8: DWORD offset from start to the stringentries
 *
 * (lastentry-firstentry) * stringentry:
 * 0: WORD len (0 marks end)    [ includes the 4 byte header length ]
 * 2: WORD flags
 * 4: CHAR[len-4]
 *      (stringentry i of a subentry refers to the ID 'firstentry+i')
 *
 * Yes, ANSI strings in win32 resources. Go figure.
 */

/**********************************************************************
 *      load_messageA           (internal)
 */

static LPSTR load_messageA( HMODULE module, UINT id, WORD lang )
{
    PRTL_MESSAGE_RESOURCE_ENTRY mre;
    char *buffer;
    NTSTATUS Status;

    TRACE("module = %p, id = %08x\n", module, id );

    if (!module) module = GetModuleHandleW( NULL );
    Status = RtlFindMessage( module, (ULONG) RT_MESSAGETABLE, lang, id, &mre );
    if (!NT_SUCCESS(Status))
        return NULL;

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
    //TRACE("returning %s\n", wine_dbgstr_a(buffer));
    return buffer;
}



static LPWSTR load_messageW( HMODULE module, UINT id, WORD lang )
{
    PRTL_MESSAGE_RESOURCE_ENTRY mre;
    WCHAR *buffer;
    NTSTATUS Status;

    TRACE("module = %p, id = %08x\n", module, id );

    if (!module) module = GetModuleHandleW( NULL );
    Status = RtlFindMessage( module, (ULONG) RT_MESSAGETABLE, lang, id, &mre );
    if (!NT_SUCCESS(Status))
        return NULL;

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
    //TRACE("returning %s\n", wine_dbgstr_w(buffer));
    return buffer;
}

/***********************************************************************
 *           FormatMessageA   (KERNEL32.@)
 * FIXME: missing wrap,
 *
 * @implemented
 */
DWORD WINAPI FormatMessageA(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	va_list* _args )
{
    LPDWORD args=(LPDWORD)_args;
    HMODULE kernel32_handle = GetModuleHandleW(kernel32W);

#if defined(__i386__) || defined(__sparc__)
/* This implementation is completely dependent on the format of the va_list on x86 CPUs */
    LPSTR	target,t;
    DWORD	talloced;
    LPSTR	from,f;
    DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL    eos = FALSE;
    CHAR	ch;

    TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (!lpBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping (%lu) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        from = HeapAlloc( GetProcessHeap(), 0, strlen((LPCSTR)lpSource)+1 );
        if (from == NULL)
        {
            return 0;
        }
        strcpy( from, (LPCSTR)lpSource );
    }
    else {
        from = NULL;
        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
            from = load_messageA( (HMODULE)lpSource, dwMessageId, (WORD)dwLanguageId );
        if (!from && (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM))
            from = load_messageA( kernel32_handle, dwMessageId, (WORD)dwLanguageId );

        if (!from)
        {
            SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
            return 0;
        }
    }
    target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    if(target == NULL)
    {
        HeapFree(GetProcessHeap(),0,from);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    t	= target;
    talloced= 100;

#define ADD_TO_T(c) do { \
        *t++=c;\
        if (t-target == talloced) {\
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
                    char *fmtstr,*x,*lastf;
                    DWORD *argliststart;

                    fmtstr = NULL;
                    lastf = f;
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
                        if (*f=='!') {
                            f++;
                            if (NULL!=(x=strchr(f,'!'))) {
                                *x='\0';
                                fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                                if(fmtstr == NULL)
                                {
                                    HeapFree(GetProcessHeap(),0,from);
                                    HeapFree(GetProcessHeap(),0,target);
                                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                    return 0;
                                }
                                sprintf(fmtstr,"%%%s",f);
                                f=x+1;
                            } else {
                                fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                                if(fmtstr == NULL)
                                {
                                    HeapFree(GetProcessHeap(),0,from);
                                    HeapFree(GetProcessHeap(),0,target);
                                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                    return 0;
                                }
                                sprintf(fmtstr,"%%%s",f);
                                f+=strlen(f); /*at \0*/
                            }
                        } else {
                            if(!args) break;
                            fmtstr = HeapAlloc(GetProcessHeap(),0,3);
                            if(fmtstr == NULL)
                            {
                                HeapFree(GetProcessHeap(),0,from);
                                HeapFree(GetProcessHeap(),0,target);
                                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                return 0;
                            }
                            strcpy( fmtstr, "%s" );
                        }
                        if (args) {
                            int sz;
                            LPSTR b;

                            if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                                argliststart=args+insertnr-1;
                            else
                                argliststart=(*(DWORD**)args)+insertnr-1;

                            b = NULL;
                            sz = 0;
                            do {
                                if (b) {
                                    HeapFree(GetProcessHeap(), 0, b);
                                }
                                sz += 256;
                                b = HeapAlloc(GetProcessHeap(), 0, sz);
                                /* CMF - This makes a BIG assumption about va_list */
                            } while (0 > _vsnprintf(b, sz, fmtstr, (va_list) argliststart));
                            x=b;
                            while(*x)
                                ADD_TO_T(*x++);

                            HeapFree(GetProcessHeap(),0,b);
                        } else {
                                /* NULL args - copy formatstr
                                 * (probably wrong)
                                 */
                            while ((lastf<f)&&(*lastf)) {
                                ADD_TO_T(*lastf++);
                            }
                        }
                        HeapFree(GetProcessHeap(),0,fmtstr);
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
    //TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        *((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(LMEM_ZEROINIT,max(nSize, talloced));
        memcpy(*(LPSTR*)lpBuffer,target,talloced);
    } else {
        lstrcpynA(lpBuffer,target,nSize);
    }
    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    TRACE("-- returning %d\n", (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?  strlen(*(LPSTR*)lpBuffer):strlen(lpBuffer));
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlen(*(LPSTR*)lpBuffer):
            strlen(lpBuffer);
#else
    return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T

/***********************************************************************
 *           FormatMessageW   (KERNEL32.@)
 *
 * @implemented
 */
DWORD WINAPI FormatMessageW(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	va_list* _args )
{
    HMODULE kernel32_handle = GetModuleHandleW(kernel32W);
    LPDWORD args=(LPDWORD)_args;
#if defined(__i386__) || defined(__sparc__)
/* This implementation is completely dependent on the format of the va_list on x86 CPUs */
    LPWSTR target,t;
    DWORD talloced,len;
    LPWSTR from,f;
    DWORD width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL eos = FALSE;
    WCHAR ch;

    TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (!lpBuffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping not supported.\n");
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        from = HeapAlloc( GetProcessHeap(), 0, (strlenW((LPCWSTR)lpSource) + 1) *
            sizeof(WCHAR) );
        if(from == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        strcpyW( from, (LPCWSTR)lpSource );
    }
    else {
        from = NULL;
        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
            from = load_messageW( (HMODULE)lpSource, dwMessageId, (WORD)dwLanguageId );
        if (!from && (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM))
            from = load_messageW( kernel32_handle, dwMessageId,(WORD)dwLanguageId );

        if (!from)
        {
            SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
            return 0;
        }
    }

    target = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100 * sizeof(WCHAR) );
    if(target == NULL)
    {
        HeapFree(GetProcessHeap(),0,from);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    t = target;
    talloced= 100;

#define ADD_TO_T(c)  do {\
    *t++=c;\
    if (t-target == talloced) {\
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
                    WCHAR *fmtstr,*sprintfbuf,*x,*lastf;
                    DWORD *argliststart;

                    fmtstr = NULL;
                    lastf = f;
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
                        if (*f=='!') {
                            f++;
                            if (NULL!=(x=strchrW(f,'!'))) {
                                *x='\0';
                                fmtstr=HeapAlloc( GetProcessHeap(), 0,(strlenW(f)+2)*sizeof(WCHAR));
                                if(fmtstr == NULL)
                                {
                                    HeapFree(GetProcessHeap(),0,from);
                                    HeapFree(GetProcessHeap(),0,target);
                                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                    return 0;
                                }
                                sprintfW(fmtstr,PCNTFMTWSTR,f);
                                f=x+1;
                            } else {
                                fmtstr=HeapAlloc(GetProcessHeap(),0,(strlenW(f)+2)*sizeof(WCHAR));
                                if(fmtstr == NULL)
                                {
                                    HeapFree(GetProcessHeap(),0,from);
                                    HeapFree(GetProcessHeap(),0,target);
                                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                    return 0;
                                }
                                sprintfW(fmtstr,PCNTFMTWSTR,f);
                                f+=strlenW(f); /*at \0*/
                            }
                        } else {
                            if(!args) break;
                            fmtstr = HeapAlloc( GetProcessHeap(),0,3*sizeof(WCHAR));
                            if(fmtstr == NULL)
                            {
                                HeapFree(GetProcessHeap(),0,from);
                                HeapFree(GetProcessHeap(),0,target);
                                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                return 0;
                            }
                            strcpyW( fmtstr, FMTWSTR );
                        }

                        if (args) {
                            if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                                argliststart=args+insertnr-1;
                            else
                                argliststart=(*(DWORD**)args)+insertnr-1;

                            len = 0;
                            sprintfbuf = NULL;
                            do {
                                if (sprintfbuf) {
                                    HeapFree(GetProcessHeap(),0,sprintfbuf);
                                }
                                len += 256;
                                sprintfbuf=HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
                                /* CMF - This makes a BIG assumption about va_list */
                            } while (0 > _vsnwprintf(sprintfbuf, len, fmtstr, (va_list) argliststart));
                            x=sprintfbuf;
                            while (*x) {
                                ADD_TO_T(*x++);
                            }
                            HeapFree(GetProcessHeap(),0,sprintfbuf);

                        } else {
                                /* NULL args - copy formatstr
                                 * (probably wrong)
                                 */
                            while ((lastf<f)&&(*lastf)) {
                                ADD_TO_T(*lastf++);
                            }
                        }

                        HeapFree(GetProcessHeap(),0,fmtstr);
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
        *((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(LMEM_ZEROINIT,len*sizeof(WCHAR));
        strcpyW(*(LPWSTR*)lpBuffer, target);
    }
    else lstrcpynW(lpBuffer, target, nSize);

    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    //TRACE("ret=%s\n", wine_dbgstr_w((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
      //  *(LPWSTR*)lpBuffer : lpBuffer));
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlenW(*(LPWSTR*)lpBuffer):
            strlenW(lpBuffer);
#else
    return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T

