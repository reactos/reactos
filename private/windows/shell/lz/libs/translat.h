/*
** translat.h - Translation macros for common DOS / Windows functions.
**
** Author:  DavidDi (stolen from ToddLa)
*/


#ifndef WM_USER

/********************************* DOS code ********************************/

// Globals
///////////

INT _ret;
INT _error;


// Types
/////////

typedef  unsigned CHAR   BYTE;
typedef unsigned SHORT  WORD;
typedef unsigned LONG   DWORD;
typedef INT             BOOL;
typedef CHAR *          PSTR;
typedef CHAR NEAR *     NPSTR;
typedef CHAR FAR *      LPSTR;
typedef INT  FAR *      LPINT;


// Constants
/////////////

// NULL

#ifndef NULL
   #if (_MSC_VER >= 600)
      #define NULL ((void *)0)
   #elif (defined(M_I86SM) || defined(M_I86MM))
      #define NULL 0
   #else
      #define NULL 0L
   #endif
#endif

// modifiers

#define FAR    FAR
#define NEAR   near
#define LONG   long
#define VOID   void
#define PASCAL PASCAL

// Boolean values

#define FALSE  0
#define TRUE   1


// Macros
//////////

// byte manipulation

#define LOWORD(l)       ((WORD)(l))
#define HIWORD(l)       ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)       ((BYTE)(w))
#define HIBYTE(w)       (((WORD)(w) >> 8) & 0xFF)
#define MAKELONG(a, b)  ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))

// file i/o

//-protect-
#define FOPEN(psz)               (                                              \
                                    (_ret = -1),                                \
                                    (_error = _dos_open(psz, O_RDONLY, &_ret)), \
                                    _ret                                        \
                                 )

//-protect-
#define FCREATE(psz)             (                                                \
                                    (_ret = -1),                                  \
                                    (_error = _dos_creat(psz, _A_NORMAL, &_ret)), \
                                    _ret                                          \
                                 )

#define FCLOSE(dosh)             (_error = _dos_close(dosh))

//-protect-
#define FREAD(dosh, buf, len)    (                                               \
                                    (_error = _dos_read(dosh, buf, len, &_ret)), \
                                    _ret                                         \
                                 )

//-protect-
#define FWRITE(dosh, buf, len)   (                                                \
                                    (_error = _dos_write(dosh, buf, len, &_ret)), \
                                    _ret                                          \
                                 )

#define FSEEK(dosh, off, i)      lseek(dosh, (long)(off), i)

#define FERROR()                 _error

// near heap memory management

#define ALLOC(n)                 malloc(n)
#define FREE(p)                  free(p)
#define SIZE(p)                  _msize(p)
#define REALLOC(p, n)            realloc(p,n)

// FAR heap memory management

#define FALLOC(n)                _fmalloc(n)
#define FFREE(n)                 _ffree(n)

// string manipulation

#define STRCAT(psz1, psz2)       strcat(psz1, psz2)
#define STRCMP(psz1, psz2)       strcmp(psz1, psz2)
#define STRCMPI(psz1, psz2)      strcmpi(psz1, psz2)
#define STRCPY(psz1, psz2)       strcpy(psz1, psz2)
#define STRLEN(psz)              strlen(psz)
#define STRLWR(psz)              strlwr(psz)
#define STRUPR(psz)              strupr(psz)

// character classification

#define ISALPHA(c)               isalpha(c)
#define ISALPHANUMERIC(c)        isalnum(c)
#define ISLOWER(c)               islower(c)
#define ISUPPER(c)               isupper(c)

#else

/******************************* Windows code ******************************/

// file i/o

#ifdef ORGCODE
#define FOPEN(psz)               _lopen(psz, READ)
#else
#define FOPEN(psz)               _lopen(psz, OF_READ)
#endif
#define FCREATE(psz)             _lcreat(psz, 0)
#define FCLOSE(dosh)             _lclose(dosh)
#define FREAD(dosh, buf, len)    _lread(dosh, buf, len)
#define FWRITE(dosh, buf, len)   _lwrite(dosh, buf, len)
#define FSEEK(dosh, off, i)      _llseek(dosh, (DWORD)off, i)
#define FERROR()                 0

// near heap memory management

#define ALLOC(n)                 (VOID *)LocalAlloc(LPTR, n)
#define FREE(p)                  LocalFree(p)
#define SIZE(p)                  LocalSize(p)
#define REALLOC(p, n)            LocalRealloc(p, n, LMEM_MOVEABLE)

// FAR heap memory management

#ifdef ORGCODE
#define FALLOC(n)                (VOID FAR *)MAKELONG(0, GlobalAlloc(GPTR, (DWORD)n))
#define FFREE(n)                 GlobalFree((HANDLE)HIWORD((LONG)n))
#else
#define FALLOC(n)                GlobalAlloc(GPTR, (DWORD)n)
#define FFREE(n)                 GlobalFree((HANDLE)n)
#endif	
// string manipulation

#define STRCAT(psz1, psz2)       lstrcat(psz1, psz2)
#define STRCMP(psz1, psz2)       lstrcmp(psz1, psz2)
#define STRCMPI(psz1, psz2)      lstrcmpi(psz1, psz2)
#define STRCPY(psz1, psz2)       lstrcpy(psz1, psz2)
#define STRLEN(psz)              lstrlen(psz)
#define STRLWR(psz)              AnsiLower(psz)
#define STRUPR(psz)              AnsiUpper(psz)

// character classification

#define ISALPHA(c)               IsCharAlpha(c)
#define ISALPHANUMERIC(c)        IsCharAlphaNumeric(c)
#define ISLOWER(c)               IsCharLower(c)
#define ISUPPER(c)               IsCharUpper(c)

#endif

/******************************* common code *******************************/


// Constants
/////////////

#define SEP_STR   "\\"

#define EQUAL     '='
#define SPACE     ' '
#define COLON     ':'
#define PERIOD    '.'

#define LF        0x0a
#define CR        0x0d
#define CTRL_Z    0x1a

// flags for _lseek

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2


// Macros
//////////

// character classification

#define ISWHITE(c)      ((c) == ' '  || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define ISFILL(c)       ((c) == ' '  || (c) == '\t')
#define ISEOL(c)        ((c) == '\n' || (c) == '\r' || (c) == '\0' || (c) == CTRL_Z)
#define ISCRLF(c)       ((c) == '\n' || (c) == '\r')
#define ISDIGIT(c)      ((c) >= '0'  && (c) <= '9')
#define ISLETTER(c)     (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define ISSWITCH(c)     ((c) == '/' || (c) == '-')
#define ISSLASH(c)      ((c) == '/' || (c) == '\\')

// character manipulation

#define TOUPPERCASE(c)  ((c) >= 'a' && (c) <= 'z' ? (c) - 'a' + 'A' : (c))
#define TOLOWERCASE(c)  ((c) >= 'A' && (c) <= 'Z' ? (c) - 'A' + 'a' : (c))
#define HEXVALUE(c)     (ISDIGIT(c) ? (c) - '0' : TOUPPERCASE(c) - 'A' + 10)

