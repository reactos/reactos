/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Marcus Meissner
 */

#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#include <ddk/ntddk.h>

/* Funny to divide them between user and kernel. */

/* be careful: always use functions from wctype.h if character > 255 */

/*
 * Unicode case conversion routines ... these should be used where
 * toupper/tolower are used for ASCII.
 */

// bugfix: mutual exclusiveness of 
// FORMAT_MESSAGE_FROM_STRING, FORMAT_MESSAGE_FROM_SYSTEM and FORMAT_MESSAGE_FROM_HMODULE
// in FormatMessage

#define IsDBCSLeadByte(c) FALSE
#define IsDBCSLeadByteEx(c,n) FALSE
#define CopyMemory memcpy
#define lstrlenA strlen
#define lstrlenW wcslen

DWORD LoadMessageA(HMODULE hModule,DWORD dwMessageId,
	 DWORD dwLanguageId,LPSTR Buffer,UINT BufSize);
DWORD LoadMessageW(HMODULE hModule,DWORD dwMessageId,
	 DWORD dwLanguageId,LPWSTR Buffer,UINT BufSize);


LPWSTR lstrchrW(LPCWSTR s, INT c);
LPSTR lstrchrA(LPCSTR s, INT c);



LPSTR STDCALL CharNextA(LPCSTR lpsz)
{
    LPSTR next = (LPSTR)lpsz;
    if (!*lpsz) 
	return next;
    if (IsDBCSLeadByte( *next )) 
	next++;
    next++;
    return next;
}


/***********************************************************************
 *           CharNextExA   (USER.30)
 */
LPSTR STDCALL CharNextExA( WORD codepage, LPCSTR ptr, DWORD flags )
{
    LPSTR next = (LPSTR)ptr;
    if (!*ptr) 
	return next;
    if (IsDBCSLeadByteEx( codepage, *next )) 
	return next++;
    next++;
    return next;
}



LPWSTR
STDCALL
CharNextW(LPCWSTR lpsz)
{
   LPWSTR next = (LPWSTR)lpsz;
   if (!*lpsz) 
	return next;
   next++;
   return next;
}


LPSTR STDCALL CharPrevA( LPCSTR start, LPCSTR ptr )
{
    LPCSTR next;
    while (*start && (start < ptr))
    {
        next = CharNextA( start );
        if (next >= ptr) 
		break;
        start = next;
    }
    return (LPSTR)start;
}



LPSTR STDCALL CharPrevExA( WORD codepage, LPCSTR start, LPCSTR ptr, DWORD flags )
{
    
    LPCSTR next;
    while (*start && (start < ptr))
    {
        next = CharNextExA( codepage, start, flags );
        if (next > ptr) 
		break;
        start = next;
    }
    return (LPSTR)start;
}






LPWSTR STDCALL CharPrevW(LPCWSTR start,LPCWSTR x)
{
    LPWSTR prev = (LPWSTR)x;
    if (x<=start) 
	return prev;
    prev--;
    return prev;
}


LPSTR STDCALL CharLowerA(LPSTR x)
{
    LPSTR	s;
    UINT s2;
    if (!HIWORD(x)) {	    
	    s2 = (UINT)x;
		
	    if (!IsDBCSLeadByte( s2 )) {
		if (!IsCharLowerA(s2))
			s2 = s2 - ( 'A' - 'a' );
		return (LPSTR)s2;
	    }
	    else {
		// should do a multibyte toupper
		if (s2 >= 'A' && s2 <= 'Z')   
			s2 = s2 - ( 'A' - 'a' );
		return (LPSTR)s2;
	    }
            return (LPSTR)x;
    }

    s=x;
    while (*s)
    {
	    if (!IsDBCSLeadByte( *s )) {
		if (!IsCharLowerA(*s))  
			*s = *s - ( 'A' - 'a' );
	    }
	    else {
		// should do a multibyte toupper
		s++;
	    }
            s++;
    }
    return x;
    
}


DWORD STDCALL CharLowerBuffA(LPSTR s,DWORD buflen)
{
    DWORD done=0;

    if (!s) 
	return 0; /* YES */
    while (*s && (buflen--))
    {
	    if (!IsDBCSLeadByte( *s )) {
            	if (!IsCharLowerA(*s))   
			*s = *s - ( 'A' - 'a' );
	    }
	    else {
		// should do a multibyte toupper
		s++;
	    }
            s++;
            done++;
    }
    return done;
}


DWORD STDCALL CharLowerBuffW(LPWSTR s,DWORD buflen)
{
    DWORD done=0;

    if (!s) 
	return 0; /* YES */
    while (*s && (buflen--))
    {
        if (!IsCharLowerW(*s))   
		*s = *s - ( L'A' - L'a' );
        s++;
        done++;
    }
    return done;
}


LPWSTR STDCALL CharLowerW(LPWSTR x)
{
    LPWSTR s;
    UINT s2;
    if (!HIWORD(x)) {
	s2 = (UINT)x;
	if (!IsCharLowerW(s2))   
		s2 = s2 - ( L'A' - L'a' );
    	return (LPWSTR)s2;
    }
    s = x;
    while (*s)
    {
	if (!IsCharLowerW(*s))   
		*s = *s - ( L'A' - L'a' );
        s++;
    }
    return x;
}



LPSTR STDCALL CharUpperA(LPSTR x)
{
    LPSTR	s;
    UINT s2;
    if (!HIWORD(x)) {	    
	    s2 = (UINT)x;

		
	    if (!IsDBCSLeadByte( s2 )) {
		if (!IsCharUpperW(s2))   
			s2 = s2 + ( 'A' - 'a' );
		return (LPSTR)s2;
	    }
	    else {
		// should do a multibyte toupper
		if (s2 >= 'a' && s2 <= 'z')   
			s2 = s2 + ( 'A' - 'a' );
		return (LPSTR)s2;
	    }
            return x;
    }

   

    s=x;
    while (*s)
    {
	    if (!IsDBCSLeadByte( *s )) {
		if (!IsCharUpperW(*s))   
			*s = *s + ( 'A' - 'a' );
	    }
	    else {
		// should do a multibyte toupper
		s++;
	    }
            s++;
    }
    return x;
    
}


DWORD STDCALL CharUpperBuffA(LPSTR s,DWORD buflen)
{
    DWORD done=0;

    if (!s) 
	return 0; /* YES */
    while (*s && (buflen--))
    {
	    if (!IsDBCSLeadByte( *s )) {
            	if (!IsCharUpperW(*s))    
			*s = *s + ( 'A' - 'a' );
	    }
	    else {
		// should do a multibyte toupper
		s++;
	    }
            s++;
            done++;
    }
    return done;
}


DWORD STDCALL CharUpperBuffW(LPWSTR s,DWORD buflen)
{
    DWORD done=0;

    if (!s) 
	return 0; /* YES */
    while (*s && (buflen--))
    {
        if (!IsCharUpperW(*s)) 
		*s = *s + ( L'A' - L'a' );
        s++;
        done++;
    }
    return done;
}


LPWSTR STDCALL CharUpperW(LPWSTR x)
{
    LPWSTR s;
    UINT s2;
    if (!HIWORD(x)) {
	s2 = (UINT)x;
	if (!IsCharUpperW(s2))   
		s2 = s2 + ( L'A' - L'a' );
    	return (LPWSTR)s2;
    }
    s = x;
    while (*s)
    {
	if (!IsCharUpperW(*s))   
		*s = *s + ( L'A' - L'a' );
        s++;
    }
    return x;
}



WINBOOL STDCALL IsCharAlphaNumericA(CHAR c)
{
   return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')  || (c >= '0' && c <= '9'));
}


WINBOOL STDCALL IsCharAlphaNumericW(WCHAR c)
{
    return ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')  || (c >= L'0' && c <= L'9'));
}
WINBOOL STDCALL IsCharAlphaA(CHAR c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

WINBOOL STDCALL IsCharAlphaW(WCHAR c)
{
	return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');
}


WINBOOL STDCALL IsCharLowerA(CHAR c)
{
    return (c >= 'a' && c <= 'z');
}


WINBOOL STDCALL IsCharLowerW(WCHAR c)
{
    return (c >= L'a' && c <= L'z');
}


WINBOOL STDCALL IsCharUpperA(CHAR c)
{
    return (c >= 'A' && c <= 'Z' );
}


WINBOOL STDCALL IsCharUpperW(WCHAR c)
{
    return (c >= L'A' && c <= L'Z' );
}


DWORD STDCALL FormatMessageA(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	va_list *Arguments
) {
	LPSTR	target,t;
	DWORD	talloced;
	LPSTR	from,f;
	//DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	DWORD	nolinefeed = 0;
	DWORD   len;

	////TRACE(resource, "(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
	//	     dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,Arguments);
	//if (width)  
	//	FIXME(resource,"line wrapping not supported.\n");
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
		len = lstrlenA((LPSTR)lpSource);
		from = HeapAlloc( GetProcessHeap(),0, len+1 );
		CopyMemory(from,lpSource,len+1);
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
		len = 200;
		from = HeapAlloc( GetProcessHeap(),0,200 );
		wsprintfA(from,"Systemmessage, messageid = 0x%08lx\n",dwMessageId);
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
		INT	bufsize;

		dwMessageId &= 0xFFFF;
		bufsize=LoadMessageA((HMODULE)lpSource,dwMessageId,dwLanguageId,NULL,100);
		if (bufsize) {
			from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
			LoadMessageA((HMODULE)lpSource,dwMessageId,dwLanguageId,from,bufsize+1);
		}
	}
	target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
	t	= target;
	talloced= 100;

#define ADD_TO_T(c) \
	*t++=c;\
	if (t-target == talloced) {\
		target	= (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
		t	= target+talloced;\
		talloced*=2;\
	}

	if (from) {
		f=from;
		while (*f && !nolinefeed) {
			if (*f=='%') {
				int	insertnr;
				char	*fmtstr,*sprintfbuf,*x,*lastf;
				DWORD	*argliststart;

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
						if (NULL!=(x=lstrchrA(f,'!'))) {
							*x='\0';
							fmtstr=HeapAlloc(GetProcessHeap(),0,lstrlenA(f)+2);
							wsprintfA(fmtstr,"%%%s",f);
							f=x+1;
						} else {
							len = lstrlenA(f);
							fmtstr=HeapAlloc(GetProcessHeap(),0,len + 1);
							wsprintfA(fmtstr,"%%%s",f);
							f+=lstrlenA(f); /*at \0*/
						}
					} else
					        if(!Arguments) 
						  break;
					else {
						fmtstr = HeapAlloc( GetProcessHeap(),0, 3 );
						fmtstr[0] = '%';
						fmtstr[1] = 's';
						fmtstr[2] = 0;
					}
					if (Arguments) {
						if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
							argliststart=Arguments+insertnr-1;
						else
                                                    argliststart=(*(DWORD**)Arguments)+insertnr-1;

						if (fmtstr[lstrlenA(fmtstr)-1]=='s')
							sprintfbuf=HeapAlloc(GetProcessHeap(),0,lstrlenA((LPSTR)argliststart[0])+1);
						else
							sprintfbuf=HeapAlloc(GetProcessHeap(),0,100);

						/* CMF - This makes a BIG assumption about va_list */
						wvsprintfA(sprintfbuf, fmtstr, (va_list) argliststart);
						x=sprintfbuf;
						while (*x) {
							ADD_TO_T(*x++);
						}
						HeapFree(GetProcessHeap(),0,sprintfbuf);
					} else {
						/* NULL Arguments - copy formatstr 
						 * (probably wrong)
						 */
						while ((lastf<f)&&(*lastf)) {
							ADD_TO_T(*lastf++);
						}
					}
					HeapFree(GetProcessHeap(),0,fmtstr);
					break;
				case 'n':
					/* FIXME: perhaps add \r too? */
					ADD_TO_T('\n');
					f++;
					break;
				case '0':
					nolinefeed=1;
					f++;
					break;
				default:ADD_TO_T(*f++)
					break;

				}
			} else {
				ADD_TO_T(*f++)
			}
		}
		*t='\0';
	}
	if (!nolinefeed) {
	    /* add linefeed */
	    if(t==target || t[-1]!='\n')
		ADD_TO_T('\n'); /* FIXME: perhaps add \r too? */
	}
	talloced = lstrlenA(target)+1;
	if (nSize && talloced<nSize) {
		target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
	}
   // //TRACE(resource,"-- %s\n",debugstr_a(target));
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		/* nSize is the MINIMUM size */
		*((LPVOID*)lpBuffer) = (LPVOID)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,talloced);
		CopyMemory(*(LPSTR*)lpBuffer,target,talloced);
	} else
		strncpy(lpBuffer,target,nSize);
	HeapFree(GetProcessHeap(),0,target);
	if (from) HeapFree(GetProcessHeap(),0,from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			lstrlenA(*(LPSTR*)lpBuffer):
			lstrlenA(lpBuffer);
}
#undef ADD_TO_T


DWORD STDCALL FormatMessageW(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	va_list *Arguments
) {
	LPWSTR	target,t;
	DWORD	talloced;
	LPWSTR	from,f;
	//DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	DWORD	nolinefeed = 0;
	DWORD   len;
	////TRACE(resource, "(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
	//	     dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,Arguments);
	//if (width) 
	//	FIXME(resource,"line wrapping not supported.\n");
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
		len = lstrlenW((LPWSTR)lpSource);
		from = HeapAlloc( GetProcessHeap(),0, (len+1)*2 );
		CopyMemory(from,lpSource,(len+1)*2);
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
		from = HeapAlloc( GetProcessHeap(),0,200 );
		wsprintfW(from,L"Systemmessage, messageid = 0x%08lx\n",dwMessageId);
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
		INT	bufsize;

		dwMessageId &= 0xFFFF;
		bufsize=LoadMessageW((HMODULE)lpSource,dwMessageId,dwLanguageId,NULL,100);
		if (bufsize) {
			from = HeapAlloc( GetProcessHeap(), 0, (bufsize + 1)*sizeof(WCHAR) );
			LoadMessageW((HMODULE)lpSource,dwMessageId,dwLanguageId,from,(bufsize + 1)*sizeof(WCHAR));
		}
	}
	target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
	t	= target;
	talloced= 100;

#define ADD_TO_T(c) \
	*t++=c;\
	if (t-target == talloced) {\
		target	= (WCHAR *)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
		t	= target+talloced;\
		talloced*=2;\
	}

	if (from) {
		f=from;
		while (*f && !nolinefeed) {
			if (*f=='%') {
				int	insertnr;
				wchar_t	*fmtstr,*sprintfbuf,*x,*lastf;
				DWORD	*argliststart;

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
						if (NULL!=(x=lstrchrW(f,'!'))) {
							*x='\0';
							len = lstrlenW(f);
							fmtstr=HeapAlloc(GetProcessHeap(),0,(len+1)*2);
							wsprintfW(fmtstr,L"%%%s",f);
							f=x+1;
						} else {
							len = lstrlenW(f);
							fmtstr=HeapAlloc(GetProcessHeap(),0,(len+1)*2);
							wsprintfW(fmtstr,L"%%%s",f);
							f+=len; /*at \0*/
						}
					} else
					        if(!Arguments) 
						  break;
					else {
						fmtstr = HeapAlloc( GetProcessHeap(),0, 6 );
						fmtstr[0] = '%';
						fmtstr[1] = 's';
						fmtstr[2] = 0;
					}
					if (Arguments) {
						if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
							argliststart=Arguments+insertnr-1;
						else
                                                    argliststart=(*(DWORD**)Arguments)+insertnr-1;

						if (fmtstr[lstrlenW(fmtstr)-1]=='s')
							sprintfbuf=HeapAlloc(GetProcessHeap(),0,lstrlenW((LPWSTR)argliststart[0])+1);
						else
							sprintfbuf=HeapAlloc(GetProcessHeap(),0,100);

						/* CMF - This makes a BIG assumption about va_list */
						wvsprintfW(sprintfbuf, fmtstr, (va_list) argliststart);
						x=sprintfbuf;
						while (*x) {
							ADD_TO_T(*x++);
						}
						HeapFree(GetProcessHeap(),0,sprintfbuf);
					} else {
						/* NULL Arguments - copy formatstr 
						 * (probably wrong)
						 */
						while ((lastf<f)&&(*lastf)) {
							ADD_TO_T(*lastf++);
						}
					}
					HeapFree(GetProcessHeap(),0,fmtstr);
					break;
				case 'n':
					/* FIXME: perhaps add \r too? */
					ADD_TO_T('\n');
					f++;
					break;
				case '0':
					nolinefeed=1;
					f++;
					break;
				default:ADD_TO_T(*f++)
					break;

				}
			} else {
				ADD_TO_T(*f++)
			}
		}
		*t='\0';
	}
	if (!nolinefeed) {
	    /* add linefeed */
	    if(t==target || t[-1]!='\n')
		ADD_TO_T('\n'); /* FIXME: perhaps add \r too? */
	}
	talloced = (lstrlenW(target)+1)*sizeof(WCHAR);
	if (nSize && talloced<nSize) {
		target = (LPWSTR)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
	}
    ////TRACE(resource,"-- %s\n",debugstr_a(target));
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		/* nSize is the MINIMUM size */
		*((LPVOID*)lpBuffer) = (LPVOID)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,talloced);
		CopyMemory(*(LPSTR*)lpBuffer,target,talloced);
	} else
		wcsncpy(lpBuffer,target,nSize);
	HeapFree(GetProcessHeap(),0,target);
	if (from) HeapFree(GetProcessHeap(),0,from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			lstrlenW(*(LPWSTR*)lpBuffer):
			lstrlenW(lpBuffer);
}
#undef ADD_TO_T

DWORD LoadMessageA(HMODULE hModule,DWORD dwMessageId,DWORD dwLanguageId,LPSTR Buffer,UINT BufSize)
{
	return 0;
}

DWORD LoadMessageW(HMODULE hModule,DWORD dwMessageId,DWORD dwLanguageId,LPWSTR Buffer,UINT BufSize)
{
	return 0;
}



LPSTR lstrchrA(LPCSTR s, INT c)
{
  char cc = c;
  while (*s)
  {
    if (*s == cc)
      return (char *)s;
    s++;
  }
  if (cc == 0)
    return (char *)s;
  return 0;
}

LPWSTR lstrchrW(LPCWSTR s, int c)
{
  WCHAR cc = c;
  while (*s)
  {
    if (*s == cc)
      return (LPWSTR)s;
    s++;
  }
  if (cc == 0)
    return (LPWSTR)s;
  return 0;
}

#if 0

VOID STDCALL OutputDebugStringA(LPCSTR lpOutputString)
{
  
	WCHAR DebugStringW[161];
	int i,j;
	i = 0;
	j = 0;
	while ( lpOutputString[i] != 0 )
	{
		while ( j < 160 && lpOutputString[i] != 0 )
		{
			DebugStringW[j] = (WCHAR)lpOutputString[i];		
			i++;
			j++;
		}
		DebugStringW[j] = 0;
		OutputDebugStringW(DebugStringW);
		j = 0;
	}
   
	return;

}

VOID
STDCALL
OutputDebugStringW(
    LPCWSTR lpOutputString
    )
{
   UNICODE_STRING UnicodeOutput;

   UnicodeOutput.Buffer = (WCHAR *)lpOutputString;
   UnicodeOutput.Length = lstrlenW(lpOutputString)*sizeof(WCHAR);
   UnicodeOutput.MaximumLength = UnicodeOutput.Length;
	
   NtDisplayString(&UnicodeOutput);
}

#endif
