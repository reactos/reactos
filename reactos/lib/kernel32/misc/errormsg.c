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

/* $Id: errormsg.c,v 1.9 2003/11/05 20:32:07 sedwards Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Kernel32
 * PURPOSE:          Format Message
 * FILE:             lib/user32/controls/static.c
 * PROGRAMER:        Steven Edwards lib/kernel32/misc/errormsg.c
 * REVISION HISTORY: 2003/11/05 Created
 * NOTES:            Adapted from Wine
 */

#include <ddk/ntddk.h>
#include <kernel32/kernel32.h>
#include <kernel32/error.h>
#include <stdio.h>
#include <string.h>
#include <wine/unicode.h>

/* Wine porting stuff */

#define TRACE DPRINT
#define FIXME DPRINT
typedef struct tagMESSAGE_RESOURCE_ENTRY {
        WORD    Length;
        WORD    Flags;
        BYTE    Text[1];
} MESSAGE_RESOURCE_ENTRY,*PMESSAGE_RESOURCE_ENTRY;
#define MESSAGE_RESOURCE_UNICODE        0x0001

typedef struct tagMESSAGE_RESOURCE_BLOCK {
        DWORD   LowId;
        DWORD   HighId;
        DWORD   OffsetToEntries;
} MESSAGE_RESOURCE_BLOCK,*PMESSAGE_RESOURCE_BLOCK;

typedef struct tagMESSAGE_RESOURCE_DATA {
        DWORD                   NumberOfBlocks;
        MESSAGE_RESOURCE_BLOCK  Blocks[ 1 ];
} MESSAGE_RESOURCE_DATA,*PMESSAGE_RESOURCE_DATA;

/* These are needed so that we can call the functions from inside kernel itself */

LPVOID WINAPI HeapAlloc( HANDLE heap, DWORD flags, SIZE_T size )
{
    return RtlAllocateHeap( heap, flags, size );
}

BOOL WINAPI HeapFree( HANDLE heap, DWORD flags, LPVOID ptr )
{
    return RtlFreeHeap( heap, flags, ptr );
}

LPVOID WINAPI HeapReAlloc( HANDLE heap, DWORD flags, LPVOID ptr, SIZE_T size )
{
    return RtlReAllocateHeap( heap, flags, ptr, size );
}

/* strdup macros */
/* DO NOT USE IT!!  it will go away soon */

inline static LPSTR HEAP_strdupWtoA( HANDLE heap, DWORD flags, LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = HeapAlloc( heap, flags, len );
    if(ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

/* End of WINE porting Code */

/* Messages...used by FormatMessage32* (KERNEL32.something)
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
 *	load_messageA		(internal)
 */
static INT load_messageA( HMODULE instance, UINT id, WORD lang,
                          LPSTR buffer, INT buflen )
{
    const MESSAGE_RESOURCE_ENTRY *mre;
    int		i,slen;

    TRACE("instance = %08lx, id = %08lx, buffer = %p, length = %ld\n", (DWORD)instance, (DWORD)id, buffer, (DWORD)buflen);

    //if (RtlFindMessage( instance, RT_MESSAGETABLE, lang, id, &mre ) != STATUS_SUCCESS) return 0;

    slen=mre->Length;
    TRACE("	- strlen=%d\n",slen);
    i = min(buflen - 1, slen);
    if (buffer == NULL)
	return slen;
    if (i>0) {
	if (mre->Flags & MESSAGE_RESOURCE_UNICODE)
            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)mre->Text, -1, buffer, i, NULL, NULL );
	else
	    lstrcpynA(buffer, (LPSTR)mre->Text, i);
	buffer[i]=0;
    } else {
	if (buflen>1) {
	    buffer[0]=0;
	    return 0;
	}
    }
    if (buffer)
	    TRACE("'%s' copied !\n", buffer);
    return i;
}

#if 0  /* FIXME */
/**********************************************************************
 *	load_messageW   (internal)
 */
static INT load_messageW( HMODULE instance, UINT id, WORD lang,
                          LPWSTR buffer, INT buflen )
{
    INT retval;
    LPSTR buffer2 = NULL;
    if (buffer && buflen)
	buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen );
    retval = load_messageA(instance,id,lang,buffer2,buflen);
    if (buffer)
    {
	if (retval) {
	    lstrcpynAtoW( buffer, buffer2, buflen );
	    retval = strlenW( buffer );
	}
	HeapFree( GetProcessHeap(), 0, buffer2 );
    }
    return retval;
}
#endif


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
	va_list* _args )
{
    LPDWORD args=(LPDWORD)_args;
#if defined(__i386__) || defined(__sparc__)
/* This implementation is completely dependent on the format of the va_list on x86 CPUs */
    LPSTR	target,t;
    DWORD	talloced;
    LPSTR	from,f;
    DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL    eos = FALSE;
    INT	bufsize;
    HMODULE	hmodule = (HMODULE)lpSource;
    CHAR	ch;

    TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping (%lu) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        from = HeapAlloc( GetProcessHeap(), 0, strlen((LPSTR)lpSource)+1 );
        strcpy( from, (LPSTR)lpSource );
    }
    else {
        bufsize = 0;

        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
        {
            if (!hmodule)
                hmodule = GetModuleHandleW(NULL);
            bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
        }
        if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) && (!bufsize))
        {
            hmodule = GetModuleHandleA("kernel32");
            bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
        }

        if (!bufsize) {
            SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
            return 0;
        }
 
        from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
        load_messageA(hmodule,dwMessageId,dwLanguageId,from,bufsize+1);
    }
    target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    t	= target;
    talloced= 100;

#define ADD_TO_T(c) do { \
        *t++=c;\
        if (t-target == talloced) {\
            target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
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
                                sprintf(fmtstr,"%%%s",f);
                                f=x+1;
                            } else {
                                fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                                sprintf(fmtstr,"%%%s",f);
                                f+=strlen(f); /*at \0*/
                            }
                        } else {
                            if(!args) break;
                            fmtstr = HeapAlloc(GetProcessHeap(),0,3);
                            strcpy( fmtstr, "%s" );
                        }
                        if (args) {
                            int sz;
                            LPSTR b;

                            if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                                argliststart=args+insertnr-1;
                            else
                                argliststart=(*(DWORD**)args)+insertnr-1;

                                /* FIXME: precision and width components are not handled correctly */
                            if ( (strcmp(fmtstr, "%ls") == 0) || (strcmp(fmtstr,"%S") == 0) ) {
                                sz = WideCharToMultiByte( CP_ACP, 0, *(WCHAR**)argliststart, -1, NULL, 0, NULL, NULL);
                                b = HeapAlloc(GetProcessHeap(), 0, sz);
                                WideCharToMultiByte( CP_ACP, 0, *(WCHAR**)argliststart, -1, b, sz, NULL, NULL);
                            } else {
                                b = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz = 1000);
                                /* CMF - This makes a BIG assumption about va_list */
                                TRACE("A BIG assumption\n");
                                _vsnprintf(b, sz, fmtstr, (va_list) argliststart);
                            }
                            for (x=b; *x; x++) ADD_TO_T(*x);

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
        target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    }
    //TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        *((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(GMEM_ZEROINIT,max(nSize, talloced));
        memcpy(*(LPSTR*)lpBuffer,target,talloced);
    } else {
        lstrcpynA(lpBuffer,target,nSize);
    }
    HeapFree(GetProcessHeap(),0,target);
    if (from) HeapFree(GetProcessHeap(),0,from);
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
    LPDWORD args=(LPDWORD)_args;
#if defined(__i386__) || defined(__sparc__)
/* This implementation is completely dependent on the format of the va_list on x86 CPUs */
    LPSTR target,t;
    DWORD talloced;
    LPSTR from,f;
    DWORD width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL eos = FALSE;
    INT bufsize;
    HMODULE hmodule = (HMODULE)lpSource;
    CHAR ch;

    TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping not supported.\n");
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        from = HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)lpSource);
    }
    else {
        bufsize = 0;

        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
        {
            if (!hmodule)
                hmodule = GetModuleHandleW(NULL);
            bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
        }
        if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) && (!bufsize))
        {
            hmodule = GetModuleHandleA("kernel32");
            bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
        }

        if (!bufsize) {
            SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
            return 0;
        }
 
        from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
        load_messageA(hmodule,dwMessageId,dwLanguageId,from,bufsize+1);
    }
    target = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100 );
    t = target;
    talloced= 100;

#define ADD_TO_T(c)  do {\
    *t++=c;\
    if (t-target == talloced) {\
        target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
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
                    char *fmtstr,*sprintfbuf,*x;
                    DWORD *argliststart;

                    fmtstr = NULL;
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
                                fmtstr=HeapAlloc( GetProcessHeap(), 0, strlen(f)+2);
                                sprintf(fmtstr,"%%%s",f);
                                f=x+1;
                            } else {
                                fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                                sprintf(fmtstr,"%%%s",f);
                                f+=strlen(f); /*at \0*/
                            }
                        } else {
                            if(!args) break;
                            fmtstr = HeapAlloc( GetProcessHeap(),0,3);
                            strcpy( fmtstr, "%s" );
                        }
                        if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                            argliststart=args+insertnr-1;
                        else
                            argliststart=(*(DWORD**)args)+insertnr-1;

                        if (fmtstr[strlen(fmtstr)-1]=='s' && argliststart[0]) {
                            DWORD xarr[3];

                            xarr[0]=(DWORD)HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)(*(argliststart+0)));
                            /* possible invalid pointers */
                            xarr[1]=*(argliststart+1);
                            xarr[2]=*(argliststart+2);
                            sprintfbuf=HeapAlloc(GetProcessHeap(),0,strlenW((LPWSTR)argliststart[0])*2+1);

                            /* CMF - This makes a BIG assumption about va_list */
                            vsprintf(sprintfbuf, fmtstr, (va_list) xarr);
                            HeapFree(GetProcessHeap(), 0, (LPVOID) xarr[0]);
                        } else {
                            sprintfbuf=HeapAlloc(GetProcessHeap(),0,100);

                            /* CMF - This makes a BIG assumption about va_list */
                            vsprintf(sprintfbuf, fmtstr, (va_list) argliststart);
                        }
                        x=sprintfbuf;
                        while (*x) {
                            ADD_TO_T(*x++);
                        }
                        HeapFree(GetProcessHeap(),0,sprintfbuf);
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
    if (nSize && talloced<nSize)
        target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        DWORD len = MultiByteToWideChar( CP_ACP, 0, target, -1, NULL, 0 );
        *((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(GMEM_ZEROINIT,len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, target, -1, *(LPWSTR*)lpBuffer, len );
    }
    else
    {
        if (nSize > 0 && !MultiByteToWideChar( CP_ACP, 0, target, -1, lpBuffer, nSize ))
            lpBuffer[nSize-1] = 0;
    }
    HeapFree(GetProcessHeap(),0,target);
    if (from) HeapFree(GetProcessHeap(),0,from);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlenW(*(LPWSTR*)lpBuffer):
            strlenW(lpBuffer);
#else
    return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T
