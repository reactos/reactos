/*
 * Infparse.c - Setup.inf parsing code.
 * Clark Cyr, Mike Colee, Todd Laney
 * Copyright (C) Microsoft, 1989
 * March 15, 1989
 *
 *  Modification History:
 *
 *  3/15/89  CC  Clark wrote this code for control Panel. This is windows
 *               code.
 *
 *  3/20/89  MC  Decided this code would work for Dos and windows portion
 *               of setup. take out windows specifc stuff like local alloc's
 *               and dialog stuff. Replace it with standard C run time calls.
 *
 *  3/24/89  Toddla TOTAL rewrite! nothing is the same any more.
 *
 *  6/29/89  MC fixed getprofilestring func to not strip quotes if more
 *              than one field exists.
 */

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>
#include "drivers.h"
#include "sulib.h"

/*** hack.  to avoid realloc problems we make READ_BUFSIZE
            as big as the inf file, thus avoiding any reallocs */

#define READ_BUFSIZE    27000   /* size of inf buffer */
#define TMP_BUFSIZE     1024    /* size of temp reads */

#define EOF        0x1A
#define ISEOL(c)     ((c) == '\n' || (c) == '\r' || (c) == '\0' || (c) == EOF)
#define ISSEP(c)   ((c) == '='  || (c) == ',')
#define ISWHITE(c) ((c) == ' '  || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define ISNOISE(c) ((c) == '"')

#define QUOTE   '"'
#define EQUAL   '='

PINF   pinfDefault = NULL;

static PSTR    pBuf;
static PINF    pInf;
static UINT    iBuf;
static UINT    iInf;

/* Globaly used pointers to non-translatable text strings. */

extern char *pszPATH;

/* Local prototypes */

BOOL multifields(PINF);


static char GETC(int fh)
{
    register UINT n;

    if (!pBuf)
        return EOF;

    n = iBuf % TMP_BUFSIZE;

    if (n == 0)
    {
       _lread(fh,pBuf,TMP_BUFSIZE);
    }
    iBuf++;
    return pBuf[n];
}

static void PUTC(char c)
{
    if (!pInf)
        return;

    pInf[iInf++] = c;
}

static void MODIFYC(char c)
{
    if (!pInf)
        return;

    pInf[iInf++ - 1] = c;
}

static char LASTC(void) {
    if (!pInf) return ' ';

    if (iInf == 0) {
        return ' ';
    }
    return pInf[iInf - 1];
}

/* int infLoadFile()      Load a entire INF file into memory
 *                        comments are removed, each line is terminated
 *                        by a \0 each section is terminated by a \0\0
 *                        ONLY spaces inside of " " are preserved
 *                        the end of file is marked with a ^Z
 *
 *   RETURNS:  A pointer to a block of memory containg file, NULL if failure
 *
 */
PINF infLoadFile(int fh)
{
    UINT    len;
    char    c;
    BOOL    fQuote = FALSE;
    BOOL    inSectionName = FALSE;

    if (fh == -1)
      return NULL;

    len = (UINT)_llseek(fh,0L,SEEK_END);

    _llseek(fh,0L,SEEK_SET);

    iBuf = 0;
    iInf = 0;
    pBuf = ALLOC(TMP_BUFSIZE);          // temp buffer
    if (!pBuf)
        return NULL;
    pInf = FALLOC(len);                 // destination, at least as big as file
    if (!pInf) {
        FREE((HANDLE)pBuf);
        return NULL;
    }

    while (iBuf < len)
    {
        c = GETC(fh);
loop:
        if (iBuf >= len)
            break;

        switch (c)
        {
            case '[':
                inSectionName = TRUE;
                PUTC(c);
                break;

            case ']':
                if (inSectionName) {
                    if (LASTC() == ' ') {
                        MODIFYC(c);
                    } else {
                        PUTC(c);
                    }
                    inSectionName = FALSE;
                } else {
                    PUTC(c);
                }
                break;

            case '\r':      /* ignore '\r' */
                break;

            case '\n':
                for (; ISWHITE(c); c = GETC(fh))
                    ;
                if (c != ';')
                    PUTC(0);    /* all lines end in a \0 */

                if (c == '[') {
                    PUTC(0);    /* all sections end with \0\0 */
                }

                fQuote = FALSE;
                goto loop;
                break;

            case '\t':
            case ' ':
                if (inSectionName) {
                    if (LASTC() != ' ' && LASTC() != ']')
                        PUTC(' ');
                } else {
                    if (fQuote)
                        PUTC(c);
                }
                break;

            case '"':
                fQuote = !fQuote;
                PUTC(c);
                break;

            case ';':
                for (; !ISEOL(c); c = GETC(fh))
                    ;
                goto loop;
                break;

            default:
                PUTC(c);
                break;
        }
    }

    PUTC(0);
    PUTC(0);
    PUTC(EOF);
    FREE((HANDLE)pBuf);

    // try to shrink this block


    // just leave pInf it's original size.  don't bother shrinking it

    return pInf;
}

/* PINF FAR PASCAL infOpen()
 *   PARAMETERS
 *           szInf - path to inf file to open and load
 *
 *   RETURNS:  A pointer to the parsed inf file if successful,
 *             Null pointer in the case of failure.
 *
 *   ENTER:
 *   EXIT:   To caller
 */

PINF infOpen(LPSTR szInf)
{
    char    szBuf[MAX_PATH];
    int     fh;
    PINF    pinf;

    fh = -1;

    if (szInf == NULL)
        szInf = szSetupInf;

    /*
     * Next try to open passed parameter as is. For Dos half.
     */
    if (fh == -1)
    {
        fh = _lopen(szInf, OF_READ);
    }
    /*
     * Next try destination path\system32. for win half.
     */
    if (fh == -1) {
        lstrcpy(szBuf, szSetupPath);
      catpath(szBuf, "system32");
      catpath(szBuf, szInf);
      fh = _lopen(szBuf, OF_READ);
    }
    /*
     * Next try destination path. for initial setup.
     */
    if (fh == -1) {
        lstrcpy(szBuf, szSetupPath);
      catpath(szBuf, szInf);
      fh = _lopen(szBuf, OF_READ);
    }
    if (fh != -1)
    {
        pinf = infLoadFile(fh);
        _lclose(fh);

        if (pinf && !pinfDefault)
            pinfDefault = pinf;

        return pinf;
    }
    return NULL;
}

/* void FAR PASCAL infClose(PINF pinf)
 *
 *   ENTER:
 *   EXIT:   To caller
 */
void infClose(PINF pinf)
{
    if (pinf == NULL)
        pinf = pinfDefault;

    if (pinf != NULL)
    {
        FFREE(pinf);

        if (pinf == pinfDefault)
            pinfDefault = NULL;
    }
}


/* FindSection  locates a section in Setup.Inf.  Sections are
 *               assumed to be delimited by a '[' as the first
 *               character on a line.
 *
 * Arguments:   pInf     Pointer to SETUP.INF buffer
 *              pszSect  LPSTR to section name
 *
 * Return:      UINT file position of the first line in the section
 *               0 if section not found
 */

UINT FindSection(PINF pInf, LPSTR pszSect)
{
    BOOL        fFound = FALSE;
    int         nLen = lstrlen(pszSect);
    PINF        pch;

    if (!pInf)
        return 0;

    pch = pInf;
    while (!fFound && *pch != EOF)
    {
        if (*pch++ == '[')
        {
            fFound = !strnicmp(pszSect, pch, nLen) && pch[nLen] == ']';
        }

        /*
         * go to the next line, dont forget to skip over \0 and \0\0
         */
        while (*pch != EOF && *pch != '\0')
            pch++;

        while (*pch == 0)
            pch++;
    }
    return((fFound && *pch != '[' && *pch != EOF) ? pch - pInf : 0);
}

/* char* fnGetDataString(npszData,szDataStr)
 *
 * Called by functions that read sections of information from setup.inf
 * to obtain strings that are set equal to keywords. Example:
 *
 * welcome=("Hello There")
 *
 * This function will return a pointer to the null terminated string
 * "Hello There".
 *
 * ENTRY:
 *
 * npszData    : pointer to entire section taken from setup.inf
 * npszDataStr : pointer to key word to look for (welcome in example above.)
 *
 * EXIT: returns pointer to string if successful, NULL if failure.
 *
 */
BOOL fnGetDataString(PINF npszData, LPSTR szDataStr, LPSTR szBuf)
{
    int len = lstrlen(szDataStr);

    while (npszData)
    {
            if (!strnicmp(npszData,szDataStr,len))  // looking for correct prof.
            {
               npszData += len;            // found !, look past prof str.
               while (ISWHITE(*npszData))  // suck out the crap.
                       npszData++;
          if (*npszData == EQUAL)     // Now we have what were looking for !
               {
                       npszData++;

             if (!multifields(npszData) )
             {
                while (ISWHITE(*npszData) || ISNOISE(*npszData))
                             npszData++;

                          while (*npszData)
                             *szBuf++ = *npszData++;

                       /*
                        * remove trailing spaces, and those pesky ()'s
                           */

                while (ISWHITE(szBuf[-1]) || ISNOISE(szBuf[-1]))
                             szBuf--;

                          *szBuf = 0;
                          return TRUE;
             }
             else
             {
                while (*npszData)
                   *szBuf++ = *npszData++;
                *szBuf = '\0';
                return TRUE;
             }
               }
       }
       npszData = infNextLine(npszData);
    }
    *szBuf = 0;
    return FALSE;
}

/*  PINF FAR PASCAL infSetDefault(pinf)
 *
 *  Sets the default INF file
 *
 * ENTRY:
 *      pinf            : inf file to be new default
 *
 * EXIT: returns old default
 *
 */
PINF infSetDefault(PINF pinf)
{
    PINF pinfT;

    pinfT = pinfDefault;
    pinfDefault = pinf;
    return pinfT;
}

/*  PINF FAR PASCAL infFindSection(pinf,szSection)
 *
 *  Reads a entire section into memory and returns a pointer to it
 *
 * ENTRY:
 *      pinf            : inf file to search for section
 *      szSection       : section name to read
 *
 * EXIT: returns pointer to section, NULL if error
 *
 */
PINF infFindSection(PINF pinf, LPSTR szSection)
{
    UINT   pos;

    if (pinf == NULL)
        pinf = pinfDefault;

    pos = FindSection(pinf, szSection);
    return pos ? pinf + pos : NULL;
}

/*  BOOL FAR PASCAL infGetProfileString(szSection,szItem,szBuf)
 *
 *  Reads a single string from a section in SETUP.INF
 *
 *  [section]
 *      item = string
 *
 * ENTRY:
 *      szSection       : pointer to section name to read.
 *      szItem          : pointer to item name to read
 *      szBuf           : pointer to a buffer to hold result
 *
 * EXIT: returns TRUE if successful, FALSE if failure.
 *
 */
BOOL infGetProfileString(PINF pinf, LPSTR szSection,LPSTR szItem,LPSTR szBuf)
{
    PINF    pSection;

    pSection = infFindSection(pinf,szSection);
    if (pSection )
        return fnGetDataString(pSection,szItem,szBuf);
    else
        *szBuf = 0;
    return FALSE;
}

/* BOOL FAR PASCAL infParseField(szData,n,szBuf)
 *
 * Given a line from SETUP.INF, will extract the nth field from the string
 * fields are assumed separated by comma's.  Leading and trailing spaces
 * are removed.
 *
 * ENTRY:
 *
 * szData    : pointer to line from SETUP.INF
 * n         : field to extract. ( 1 based )
 *             0 is field before a '=' sign
 * szBuf     : pointer to buffer to hold extracted field
 *
 * EXIT: returns TRUE if successful, FALSE if failure.
 *
 */
BOOL infParseField(PINF szData, int n, LPSTR szBuf)
{
    BOOL    fQuote = FALSE;
    PINF    pch;
    LPSTR   ptr;

    if (!szData || !szBuf)
        return FALSE;

    /*
     * find the first separator
     */
    for (pch=szData; *pch && !ISSEP(*pch); pch++) {
      if ( *pch == QUOTE )
         fQuote = !fQuote;
    }

    if (n == 0 && *pch != '=')
        return FALSE;

    if (n > 0 && *pch == '=' && !fQuote)
        szData = ++pch;

    /*
     *  locate the nth comma, that is not inside of quotes
     */
    fQuote = FALSE;
    while (n > 1)
    {
            while (*szData)
            {
          if (!fQuote && ISSEP(*szData))
                   break;

          if (*szData == QUOTE)
                   fQuote = !fQuote;

               szData++;
            }

            if (!*szData) {
               szBuf[0] = 0;            // make szBuf empty
               return FALSE;
            }

            szData++;
            n--;
    }
    /*
     * now copy the field to szBuf
     */
    while (ISWHITE(*szData))
            szData++;

    fQuote = FALSE;
    ptr = szBuf;                // fill output buffer with this
    while (*szData)
    {
       if (*szData == QUOTE)
               fQuote = !fQuote;
       else if (!fQuote && ISSEP(*szData))
               break;
            else
               *ptr++ = *szData;
            szData++;
    }
    /*
     * remove trailing spaces, and those pesky ()'s
     */
    while ((ptr > szBuf) && (ISWHITE(ptr[-1]) || ISNOISE(ptr[-1])))
            ptr--;

    *ptr = 0;
    return TRUE;
}

/* BOOL multifields(LPSTR npszData);
 *
 * Given a line line from mmdriver.inf that was after a profile
 * string this function will determine if that line has more than one
 * field. ie. Fields are seperated by commas that are NOT contained between
 * quotes.
 *
 * ENYRY:
 *
 * npszData : a line from setup.inf Example "xyz adapter",1:foobar.drv
 *
 * EXIT: This function will return TRUE if the line containes more than
 *       one field, ie the function would return TRUE for the example line
 *       shown above.
 *
 */
BOOL multifields(PINF npszData)
{
   BOOL    fQuote = FALSE;

        while (*npszData)
        {
      if (!fQuote && ISSEP(*npszData))
                   return TRUE;

      if (*npszData == QUOTE)
                   fQuote = !fQuote;

           npszData++;
        }
   return FALSE;
}

/* LPSTR FAR PASCAL infNextLine(sz)
 *
 * Given a line from SETUP.INF, advance to the next line.  will skip past the
 * ending NULL character checking for end of buffer \0\0
 *
 * ENTRY:
 *
 * sz        : pointer to line from a SETUP.INF section
 *
 * EXIT: returns pointer to next line if successful, NULL if failure.
 *
 */
PINF infNextLine(PINF pinf)
{
    if (!pinf)
        return NULL;

    while (*pinf != 0 || *(pinf + 1) == ' ')
        pinf++;

    return *++pinf ? pinf : NULL;
}

/* int FAR PASCAL infLineCount(pinf)
 *
 * Given a section from SETUP.INF, returns the number of lines in the section
 *
 * ENTRY:
 *
 * pinf      : pointer to a section from SETUP.INF
 *
 * EXIT: returns line count
 *
 */
int infLineCount(PINF pinf)
{
    int n = 0;

    for (n=0; pinf; pinf = infNextLine(pinf))
        n++;

    return n;
}
