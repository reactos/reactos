/****************************************************************************/
/*                                                                          */
/*  rcutil.C -                                                              */
/*                                                                          */
/*    Windows 3.0 Resource Compiler - Utility Functions                     */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "rc.h"


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyAlloc() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

//  HACK Alert.  Allocate an extra longlong and return past it (to allow for PREVCH()
//  to store a byte before the allocation block and to maintain 8 byte alignment).

VOID *
MyAlloc(
    UINT nbytes
    )
{
    PVOID s;

    if ((s = HeapAlloc(hHeap, HEAP_NO_SERIALIZE|HEAP_ZERO_MEMORY, nbytes+8)) != NULL) {
        return(((PCHAR)s)+8);
    } else {
        SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(1120), nbytes);
        quit(Msg_Text);
    }
    return NULL;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyFree() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID *
MyFree(
    VOID *p
    )
{
    if (p) {
        HeapFree(hHeap, HEAP_NO_SERIALIZE, ((PCHAR)p)-8);
    }

    return(NULL);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyMakeStr() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

WCHAR *
MyMakeStr(
    WCHAR *s
    )
{
    WCHAR * s1;

    if (s) {
        s1 = (WCHAR *) MyAlloc((wcslen(s) + 1) * sizeof(WCHAR));  /* allocate buffer */
        wcscpy(s1, s);                          /* copy string */
    } else {
        s1 = s;
    }

    return(s1);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyRead() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

UINT
MyRead(
    FILE *fh,
    VOID *p,
    UINT n
    )
{
    UINT n1;

    n1 = fread(p, 1, (size_t)n, fh);
    if (ferror (fh)) {
        quit(GET_MSG(1121));
        return 0;
    } else {
        return(n1);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyWrite() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

UINT
MyWrite(
    FILE *fh,
    VOID *p,
    UINT n
    )
{
    UINT n1;

    if ((n1 = fwrite(p, 1, n, fh)) != n) {
        quit("RC : fatal error RW1022: I/O error writing file.");
        return (0);
    } else {
        return(n1);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyAlign() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

UINT
MyAlign(
    PFILE fh
    )
{
    DWORD   t0 = 0;
    DWORD   ib;

    /* align file to dword */
    ib = MySeek(fh, 0L, SEEK_CUR);
    if (ib % 4) {
        ib = 4 - ib % 4;
        MyWrite(fh, (PVOID)&t0, (UINT)ib);
        return(ib);
    }
    return(0);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MySeek() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

LONG
MySeek(
    FILE *fh,
    LONG pos,
    int cmd
    )
{
    if (fseek(fh, pos, cmd))
        quit("RC : fatal error RW1023: I/O error seeking in file");
    if ((pos = ftell (fh)) == -1L)
        quit("RC : fatal error RW1023: I/O error seeking in file");
    return(pos);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyCopy() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
MyCopy(
    FILE *srcfh,
    PFILE dstfh,
    ULONG nbytes
    )
{
    PCHAR  buffer = (PCHAR) MyAlloc(BUFSIZE);

    UINT n;

    while (nbytes) {
        if (nbytes <= BUFSIZE)
            n = (UINT)nbytes;
        else
            n = BUFSIZE;
        nbytes -= n;

        MyRead(srcfh, buffer, n);
        MyWrite( dstfh, buffer, n);
    }

    MyFree(buffer);

    return(n);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MyCopyAll() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
MyCopyAll(
    FILE *srcfh,
    PFILE dstfh
    )
{
    PCHAR  buffer = (PCHAR) MyAlloc(BUFSIZE);

    UINT n;

    while ((n = fread(buffer, 1, BUFSIZE, srcfh)) != 0)
        MyWrite(dstfh, buffer, n);

    MyFree(buffer);

    return TRUE;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  strpre() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* strpre: return -1 if pch1 is a prefix of pch2, 0 otherwise.
 * compare is case insensitive.
 */

int
strpre(
    PWCHAR  pch1,
    PWCHAR  pch2
    )
{
    while (*pch1) {
        if (!*pch2)
            return 0;
        else if (towupper(*pch1) == towupper(*pch2))
            pch1++, pch2++;
        else
            return 0;
    }
    return - 1;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  iswhite() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
iswhite (
    WCHAR c
    )
{
    /* returns true for whitespace and linebreak characters */
    switch (c) {
        case L' ':
        case L'\t':
        case L'\r':
        case L'\n':
        case EOF:
            return(-1);
            break;
        default:
            return(0);
            break;
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  IsSwitchChar() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

BOOL
IsSwitchChar(
    CHAR c
    )
{
    /* true for switch characters */
    return (c == '/' || c == '-');
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  searchenv() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* the _searchenv() function in the C5.0 RTL doesn't work.  In particular,
 * it fails to check for \ characters and can end up returning paths like
 * D:\\FILE.EXT.  szActual is assumed to be at least MAX_PATH and will
 * always get walked all over
 */

VOID
searchenv(
    PCHAR szFile,
    PCHAR szVar,
    PCHAR szActual
    )
{
    PCHAR pchVar;
    PCHAR pch;
    int ich;
    PFILE fhTemp;

    if (!strcmp(szVar, "INCLUDE"))
        pchVar = pchInclude;
    else
        pchVar = getenv(szVar);

    /* we don't do absolute paths */
    if (szFile[0] == '\\' || szFile[0] == '/' || szFile[1] == ':') {
        for (ich = 0; szActual[ich] = szFile[ich]; ich++)
            ;
        return;
    }

    do {
        /* copy the next environment string... */
        for (ich = 0; (szActual[ich] = pchVar[ich]) != '\000'; ich++)
            if (pchVar[ich] == ';')
                break;
        szActual[ich] = '\000';
        pchVar += ich;
        if (*pchVar)
            pchVar++;

        /* HARD LOOP -- find end of path string */
        for (pch = szActual; *pch; pch++)
            ;

        /* check first!  this is what _searchenv() messed up! */
        if (pch[-1] != '\\' && pch[-1] != '/')
            *pch++ = '\\';

        /* HARD LOOP -- we already know szFile does start with a drive or abs. dir */
        for (ich = 0; pch[ich] = szFile[ich]; ich++)
            ;

        /* is the file here? szActual already contains name */
        if ((fhTemp = fopen(szActual, "rb")) != NULL) {
            fclose (fhTemp);
            return;
        }
    } while (szActual[0] && *pchVar);

    /* if we reach here, we know szActual is empty */
    return;

}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  ExtractFileName(szFullName, szFileName) -                                */
/*                                                                           */
/*      This routine is used to extract just the file name from a string     */
/*  that may or may not contain a full or partial path name.                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
ExtractFileName(
    PWCHAR szFullName,
    PWCHAR szFileName
    )
{
    int iLen;
    PWCHAR pCh;

    iLen = wcslen(szFullName);

    /* Goto the last character of the full name; */
    pCh = (PWCHAR)(szFullName + iLen);
    pCh--;

    /* Look for '/', '\\' or ':' character */
    while (iLen--) {
        if ((*pCh == L'\\') || (*pCh == L'/') || (*pCh == L':'))
            break;
        pCh--;
    }

    wcscpy(szFileName, ++pCh);
}


DWORD
wcsatoi(
    WCHAR *s
    )
{
    DWORD       t = 0;

    while (*s) {
        t = 10 * t + (DWORD)((CHAR)*s - '0');
        s++;
    }
    return t;
}


WCHAR *
wcsitow(
    LONG   v,
    WCHAR *s,
    DWORD  r
    )
{
    DWORD       cb = 0;
    DWORD       t;
    DWORD       tt = v;

    while (tt) {
        t = tt % r;
        cb++;
        tt /= r;
    }

    s += cb;
    *s-- = 0;
    while (v) {
        t = v % r;
        *s-- = (WCHAR)((CHAR)t + '0');
        v /= r;
    }
    return ++s;
}


// ----------------------------------------------------------------------------
//
//  PreBeginParse
//
// ----------------------------------------------------------------------------

VOID
PreBeginParse(
    PRESINFO pRes,
    int id
    )
{
    while (token.type != BEGIN) {
        switch (token.type) {
            case TKLANGUAGE:
                pRes->language = GetLanguage();
                break;

            case TKVERSION:
                GetToken(FALSE);
                if (token.type != NUMLIT)
                    ParseError1(2139);
                pRes->version = token.longval;
                break;

            case TKCHARACTERISTICS:
                GetToken(FALSE);
                if (token.type != NUMLIT)
                    ParseError1(2140);
                pRes->characteristics = token.longval;
                break;

            default:
                ParseError1(id);
                break;
        }
        GetToken(FALSE);
    }

    if (token.type != BEGIN)
        ParseError1(id);

    GetToken(TRUE);
}
