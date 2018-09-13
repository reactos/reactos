/*-----------------------------------------------------------------------------
Name:   mkdep.c

Description:
Determine file dependencies

To Build:
    cl /Ox /W3 mkdep.c

Revision History:
brendand (8/3/94) - Taken from GaryBu, merged files into a single unit
brendand (8/4/94) - Added .PCH and wild-card support
-----------------------------------------------------------------------------*/

// Includes -------------------------------------------------------------------
#define LINT_ARGS
#include    <assert.h>
#include    <ctype.h>
#include    <io.h>
#include    <malloc.h>
#include    <process.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>

// Types and Constants --------------------------------------------------------
#ifndef CDECL
#define CDECL
#endif
#ifndef CONST
#define CONST
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef Assert
#define Assert(f)       assert(f)
#endif

#define TRUE 1
#define FALSE 0

#define FOREVER while(1)
#define BLOCK
#define VOID    void

#ifdef D86
#define szROText        "rt"
#define szRWText        "r+t"
#define szWOText        "wt"
#define szROBin         "rb"
#define szRWBin         "r+b"
#define szWOBin         "wb"
#endif

typedef int                             BOOL;
typedef char*                   SZ;
typedef unsigned char   BYTE;
typedef BYTE*                   PB;
typedef unsigned short  WORD;
typedef WORD*                   PW;
typedef unsigned long   LONG;

#define lpbNull ((PB) NULL)

#define LOWORD(l)       ((WORD)l)
#define HIWORD(l)       ((WORD)(((LONG)l >> 16) & 0xffff))
#define LOBYTE(w)       ((BYTE)w)
#define HIBYTE(w)       (((WORD)w >> 8) & 0xff)
#define MAKEWORD(l,h)   ((WORD)(l)|((WORD)(h)<<8))
#define MAKELONG(l,h)   ((long)(((unsigned)l)|((unsigned long)((unsigned)h))<<16))

/* Args Record - MarkArgs, UnmarkArgs */
typedef struct
    {
    int cargArr;
    SZ *pszArr;
    } ARR;

/* drive usage types - getdt */
#define dtNil           0
#define dtLocal         1
#define dtUserNet       2

/* File attributes  - getatr, setatr */
#define atrError                0xffff
#define atrReadOnly             FILE_READONLY
#define atrHidden               FILE_HIDDEN
#define atrSystem               FILE_SYSTEM
#define atrVolume               0x08
#define atrDirectory    FILE_DIRECTORY
#define atrArchive              FILE_ARCHIVED

/* Macro for defining Linked list inertion */
#define AddToList(new,head,tail,link,null) { if(head==null) head=new; else tail->link = new; tail=new; new->link=null; }

/* & deletion */
#define DeleteFromList(item,head,tail,link,null,prev) { if(prev==null) head=item->link; else prev->link = item->link; \
if (tail==item) tail = prev; }

/* for MtimeOfFile() */
typedef long MTIME;
#define mtimeError ((MTIME) -1L)

typedef enum
    {
    langUnknown,
    langC,
    langAsm,
    langRC
    } LANG;

typedef struct _di
    {
    struct _di      *pdiNext;       /* next in list */
    char            *szPath;        /* path name */
    char            *szName;        /* full name */
    BOOL            fPathIsStd; /* name from standard includes (-I) */
    } DI;   /* dir info */

typedef struct _lk
    {
    struct _lk      *plkNext;       /* next in list */
    struct _fi      *pfi;           /* file info for link */
    } LK;   /* File link */

typedef struct _fi
    {
    struct _fi      *pfiNext;       /* single link */
    char            *szPath;        /* path name */
    char            *szName;        /* full name */
    LANG            lang;           /* language */
    struct _lk      *plkHead;       /* included list */
    struct _lk      *plkTail;       /* included list */
    unsigned        fIgnore:1;      /* ignore: either -X and std include or -x <file> */
    unsigned        cout:15;        /* output count */
    } FI;   /* file info */

typedef VOID (*PFN_ENUM)(char *, char *);

#define iszIncMax 40
char*   szPrefix = "";
char*   szSuffix = ".$O";

#define rmj                     1
#define rmm                     1
#define rup                     0
#define szVerName       "Forms3 Version"

// Globals --------------------------------------------------------------------
DI*     pdiHead = NULL; /* stack of directories of files included */
FI*     pfiHead = NULL;
FI*     pfiTail = NULL;
WORD    coutCur = 0;
int     cchLine;

int     iszIncMac = 0;
char*   rgszIncPath[iszIncMax];     // actual path
char*   rgszIncName[iszIncMax];     // name to output

BOOL    fVerbose       = FALSE;
BOOL    fReplacePrefix = FALSE;
BOOL    fNoGenHeaders  = FALSE;      // True if all header files must be present
BOOL    fIgnoreStd = FALSE;          // True if std include files should be ignored
BOOL    fUseCurDir = FALSE;          // When True: if a file doesn't exist and
                                     //   we are going to print a dependency for
                                     //   it, use the current directory rather
                                     //   than the directory of the source file.
char*   szPrintDir = NULL;           // If set, only print files in this dir.
char*   szPCHFile = NULL;            // .H which marks end of .PCH


// Prototypes -----------------------------------------------------------------

int     main(int, char**);
VOID    Usage(void);

char*   SzIncludesC(char *, BOOL *), *SzIncludesAsm(char *), *SzIncludesRC(char *, BOOL *);
FI*     PfiDependFn(char *, char *, BOOL, LANG, BOOL);
FI*     PfiLookup(char *, char *, LANG);
FI*     PfiAlloc(char *, char *, BOOL, LANG);
VOID    FreeFi(FI *);
VOID    AllocLk(FI *, FI *);
VOID    FreeAllLk(FI *);
VOID    StartReport(void);
VOID    ContinueReport(void);
VOID    EndReport(void);
VOID    EndLine(void);
VOID    Indent(void);
VOID    Report(char *, char *);
VOID    PrReverse(char *, char *);
BOOL    FPrintFi(FI *);
VOID    EnumChildren(FI *, PFN_ENUM, char *);
VOID    Process(char *, BOOL);
VOID    Fatal(char *);
SZ      SzTransEnv(SZ);
VOID    NormalizePath(SZ);
VOID    MakeName(SZ, SZ, SZ);
VOID    CopyPath(SZ, SZ);
VOID    PushDir(char *, char *, BOOL);
VOID    PopDir(void);
DI*     PdiFromIdi(int);
int     AddIncludeDir(char *);

VOID
Fatal(sz)
char *sz;
    {
    fprintf(stderr, "mkdep: error: %s\n", sz);
    exit(1);
    }


VOID
Usage()
    {
    if (rup == 0)
        fprintf(stderr, "Mkdep V%d.%02d\n", rmj, rmm);
    else
        fprintf(stderr, "Mkdep V%d.%02d.%02d\n", rmj, rmm, rup);

    fprintf(stderr,
        "usage: mkdep [-v] [-r] [-n] [-X] [-C] [-I includeDir]*\n"
         "\t[-p prefix] [-P replace_prefix] [-s suffix] \n"
         "\t[-d file]* [-D printDir] files\n\n"
         "\t-v  Verbose\n"
         "\t-r  Reverse the dependencies that are output\n"
         "\t-n  Don't emit dependencies on files that don't now exist\n"
         "\t-X  Search, but don't print standard includes\n"
         "\t-C  If file doesn't exist, use .\\ not the directory of including file\n"
         "\t-I  Include directory to search for <> includes\n"
         // "\t-J  Search include directories from the INCLUDE environment variable\n"
         "\t-p  Prefix for all target-file names\n"
         "\t-P  Ditto, but first remove existing prefix from name\n"
         "\t-s  Suffix for all target-file names (default %s)\n"
         "\t-d  Search, but don't print named file\n"
         "\t-D  Only print files which are in named dir\n"
         "\t-h  Header which marks the end of the .PCH\n\n"
         "A response file can be used by specifying '@filename' as an option.\n"
             , szSuffix);
    exit(1);
    }


char **CmdArgs;
int    cArgs;
int    CurArg = 1;
FILE  *pfileResponse = NULL;
char   achBuf[256];
char * pBuf = NULL;

char *
GetNextArg()
{
    char *pszTokens = " \t\n";

    if (pfileResponse)
    {
        char * psz;

        if (pBuf)
        {
            pBuf = strtok(NULL, pszTokens);

            if (pBuf)
                return pBuf;
        }

        do
        {
            psz = fgets(achBuf, 256, pfileResponse);
            if (psz == NULL)
            {
                fclose(pfileResponse);
                pfileResponse = NULL;
            }
            else if (achBuf[strlen(achBuf)-1] != '\n')
            {
                fclose(pfileResponse);
                Fatal("Line too long in response file. Must be less "
                        "than 256 characters.");
            }
            else
            {
                pBuf = strtok(achBuf, pszTokens);

                if (pBuf)
                    return pBuf;
            }
        } while (psz && !pBuf);
    }

    if (CurArg >= cArgs)
        return NULL;

    return CmdArgs[CurArg++];
}

#define FSwitchCh(ch)   ((ch)=='-' || (ch) == '/' || (ch) == '@')

int
main(iszMax, rgsz)
int iszMax;
char *rgsz[];
    {
    BOOL    fReverse = FALSE;
    char   *pszArg;
    int i = 0;

    if (iszMax == 1)
        Usage();

    CmdArgs = rgsz;
    cArgs   = iszMax;

    /* Parse command line switches.
     */
    while (((pszArg = GetNextArg()) != NULL) && FSwitchCh(pszArg[0]))
        {
        char chSwitch = pszArg[1];

        if (pszArg[0] == '@')
        {
            if (pszArg[1] == '\0')
                Usage();

            pfileResponse = fopen(&pszArg[1], "rt");
            if (!pfileResponse)
            {
                fprintf(stderr, "mkdep: error: Could not open response file "
                        "'%s'.\n", &pszArg[1]);
                return(1);
            }

            continue;
        }

        // fprintf(stderr, "Arg %d: '%s' ", i++, pszArg);

        switch (chSwitch)
            {
        case 'v':
            fVerbose = TRUE;
            break;
        case 'r':
            fReverse = TRUE;
            break;
        case 'n':
            fNoGenHeaders = TRUE;
            break;
        case 'x':
        case 'X':
            fIgnoreStd = TRUE;
            break;
        case 'C':
            fUseCurDir = TRUE;
            break;

#if 0
        case 'J':
            {
            SZ szInc = getenv("INCLUDE");
            if (szInc)
                {
                char    rgszDir[iszIncMax][_MAX_FNAME];
                int     nDirs,i;
                char*   psz;

                // Convert embedded semicolons to blanks
                for (psz=szInc; *psz; psz++)
                    if (*psz == ';')
                        *psz = ' ';

                /* This is very bogus! a dynamic way of reading the dirs
                   should be done so up to iszIncMax dirs can be read. Also,
                   AddIncludeDir does not copy the strings and rgszDir is
                   an automatic variable!
                */
                fprintf(stderr, "-J option: only first 16 include dirs parsed.\n");
                nDirs =
                     sscanf(szInc,
                       "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                       rgszDir[0],rgszDir[1],rgszDir[2],rgszDir[3],rgszDir[4],
                       rgszDir[5],rgszDir[6],rgszDir[7],rgszDir[8],rgszDir[9],
                       rgszDir[10],rgszDir[11],rgszDir[12],rgszDir[13],
                       rgszDir[14],rgszDir[15]);
                for (i = 0; i < nDirs; i++)
                    AddIncludeDir(rgszDir[i]);
                }
            else
                fprintf(stderr,"-J option: INCLUDE variable not set.\n");
            }
            break;
#endif

        case 's':
        case 'P':
        case 'p':
        case 'I':
        case 'd':
        case 'D':
        case 'h':
            {
            char *sz = &pszArg[2];

            if (sz[0] == '\0')
                {
                /* Allow "-I includefile"
                 * and   "-IincludeFile"
                 */
                pszArg = GetNextArg();
                if (!pszArg)
                    Usage();

                sz = pszArg;
                }

            // fprintf(stderr, "File: '%s'.", sz);

            sz = strdup(sz);

            switch (chSwitch)
                {
            case 's':
                szSuffix = sz;
                break;
            case 'P':
                fReplacePrefix = TRUE;
                // Drop through
            case 'p':
                szPrefix = sz;
                break;
            case 'I':
                AddIncludeDir(sz);
                break;
            case 'd':
                {
                FI *pfi;

                // exlude file given
                // NOTE: the -C option if given, must appear before now

                NormalizePath(sz);

                if ((pfi = PfiDependFn(SzTransEnv(sz), sz, FALSE, langUnknown, FALSE)) != NULL)
                    // file existed; ignore it
                    pfi->fIgnore = TRUE;
                else
                    // file doesn't exist, create FI
                    (void)PfiAlloc(SzTransEnv(sz), sz, TRUE, langUnknown);
                break;
                }
            case 'D':
                /* only print files from given directory */
                NormalizePath(sz);
                szPrintDir = sz;
                break;
            case 'h':
                szPCHFile = sz;
                break;
                }
            }
            break;

        default:
            Usage();
            break;
            }

        // fprintf(stderr, "\n");
        }

    while (pszArg)
        {
        long                hf;
        char                szPath[_MAX_DIR];
        char                szName[_MAX_PATH];
        struct _finddata_t  fd;

        // fprintf(stderr, "Reading path '%s' - ", pszArg);

        NormalizePath(pszArg);
        CopyPath(szPath, pszArg);

        // fprintf(stderr, "'%s\\%s'\n", szPath, pszArg);

        hf = _findfirst(pszArg, &fd);

        if (hf > -1)
            {
            do
                {
                MakeName(szName, szPath, fd.name);
                // fprintf(stderr, "     -- '%s'\n", szName);
                Process(szName, fReverse);
                }
            while (!_findnext(hf, &fd));
            _findclose(hf);
            }
//      else
//          fprintf(stderr, "Unable to find source file: %s\n", pszArg);

        pszArg = GetNextArg();
        }
    return( 0 );
    }

/*****************************************************************************/
/* standard dependency report */

VOID
StartReport()
/*
  -- prepare for a new line
*/
    {
    cchLine = 77;
    }

VOID
EndLine()
/*
  -- Make it so that the next Report starts on a new line.
 */
    {
    cchLine = 0;
    }


VOID
ContinueReport()
/*
  -- Output continuation character, new line, then indent.
 */
    {
    printf(" \\\n");
    StartReport();
    Indent();
    }

VOID
EndReport()
/*
  -- Finish off this line.
 */
    {
    printf("\n\n");
    }

VOID
Indent()
/*
  -- Indent a tab at the beginning of a line.
 */
    {
    printf("\t");
    cchLine -= 8;           /* for tab */
    }


VOID
Report(sz, szParm)
/*
  -- report string
  -- if too many characters extend line
*/
register char * sz;
char *  szParm;         /* ignored */
    {
    int cch = strlen(sz);

    if (cch > cchLine)
        {
        ContinueReport();
        while (isspace(sz[0]))
            {
            sz++;
            cch--;
            }
        }

    while (*sz != '\0')
        {
        if (*sz == '#')
            {
            putchar('\\');          /* escape any # in path */
            cch++;
            }
        putchar(*sz);
        sz++;
        }
    cchLine -= cch;
    }


/*****************************************************************************/
/* Reverse dependency printing */

VOID
PrReverse(szHdr, szSource)
/*
  -- report reverse dependency
*/
char *  szHdr;
char *  szSource;
    {
    printf("%s: %s\n", szHdr, szSource);
    }


/*****************************************************************************/

BOOL FPrintFi(pfi)
/*
  -- returns true if we should print this file; false if ignore; false if
     szPrintDir is != 0 and it is a prefix of szName.  The current directory
     is a zero length string and is handled specially
*/
FI *pfi;
    {
    if (pfi->fIgnore)
        return FALSE;

    if (szPrintDir == NULL)
        return TRUE;

    if (*szPrintDir == '\0')
        // only print current directory (check for / in name)
        return strchr(pfi->szName, '/') == 0;
    else
        // print if szPrintDir is prefix of name
        return strncmp(szPrintDir, pfi->szName, strlen(szPrintDir)) == 0;
    }


VOID
EnumChildren(pfi, pfnDo, szParm)
/*
  -- enumerate children, call *pfnDo for each element
*/
FI *    pfi;
PFN_ENUM pfnDo;
char *  szParm;
    {
    LK *plk;

    for (plk = pfi->plkHead; plk != NULL; plk = plk->plkNext)
        {
        FI *pfi = plk->pfi;

        if (pfi->cout < coutCur)
            {
            /* Mark that we've visited this node, to prevent
             * infinite recursion should we have a self referential
             * dependency graph.
             */
            pfi->cout = coutCur;

            if (FPrintFi(pfi))
                {
                if (szParm == NULL)
                    (*pfnDo)(" ", szParm);
                (*pfnDo)(pfi->szName, szParm);
                }

            // recurse on nested includes; may include a non-standard includes
            EnumChildren(pfi, pfnDo, szParm);
            }
        }
    }



VOID
Process(szPath, fReverse)
/*
  -- process a file
  -- reverse => show headers as depending on files
*/
char *  szPath;                 // path name to file
BOOL    fReverse;
    {
    FI *    pfi;

    strlwr(szPath);

    /* Build a list of all dependencies. */
    pfi = PfiDependFn(szPath, szPath, FALSE, langUnknown, FALSE);

    if (pfi == NULL)
        {
        if (fVerbose)
            fprintf(stderr, "mkdep: warning: file %s ignored\n", szPath);
        }
    else if (pfi->plkHead != NULL)
        {
        /* file depends on something */

        if (!fReverse)
            {
            /* normal dependencies */
            char *  pch;

            /* truncate any suffix */
            pch = strrchr(szPath, '.');
            if (pch)
                {
                if (strchr(pch, '/') || strchr(pch, '\\'))
                    pch = NULL;
                }
            if (pch != NULL)
                *pch = '\0';

            StartReport();

            Report(szPrefix, NULL);
            if (fReplacePrefix)
                {
                /* prefix replaces any name prefix */
                char *  szName = szPath;

                while (*szPath != '\0')
                    {
                    if (*szPath == '\\' || *szPath == '/')
                        szName = szPath+1;
                    szPath++;
                    }
                Report(szName, NULL);
                }
            else
                {
                Report(szPath, NULL);
                }
            Report(szSuffix, NULL);
            Report(" :", NULL);

            EndLine();

            coutCur++;
            EnumChildren(pfi, Report, NULL);

            EndReport();
            }
        else
            {
            /* reverse dependencies */
            coutCur++;
            EnumChildren(pfi, PrReverse, szPath);
            }
        }

    if (pfi != NULL)
        // free top level FI (presumably for .c/.asm file which won't be needed)
        FreeFi(pfi);
    }



FI *
PfiDependFn(szPath, szName, fPathIsStd, lang, fIsPCHFile)
/*
  -- given a file name & language, return a filled in FI
  -- return NULL if error
*/
char *  szPath;                 // path name to file
char *  szName;                 // official name of file
BOOL    fPathIsStd;             // path portion of szPath is from standard includes (-I)
LANG    lang;                   // propagate parent language
BOOL    fIsPCHFile;             // Is .PCH marker file
    {
    FILE *  pfile;
    char    rgch[256];
    char *  sz;
    char *  szSuffix;
    FI *    pfi;

    /* first check to see if already in list */
    if ((pfi = PfiLookup(szPath, szName, lang)) != NULL)
        return pfi;

    szSuffix = strrchr(szPath, '.');

    if (lang != langUnknown)
        {
            /* do nothing -- keep old language */
        }
    else if (szSuffix == NULL)
        return NULL;
    else if (strcmp(szSuffix, ".asm") == 0 || strcmp(szSuffix, ".inc") == 0)
        lang = langAsm;
    else if (strcmp(szSuffix, ".rc") == 0)
        lang = langRC;
    else
        lang = langC;

    if ((pfile = fopen(szPath, "rt")) == NULL)
    {
        // fprintf(stderr, "Could not open file '%s'.\n", szPath);
        return NULL;
    }

    pfi = PfiAlloc(szPath, szName, fPathIsStd && fIgnoreStd, lang);

    if (lang == langRC)
    {
        //
        // Make sure we don't try to parse binary files - major waste of time!
        //
        static char *aszBinary[] = { ".ico", ".sqz", ".bmp", ".tlb", ".cur",
                                     ".odg", ".ppg", ".otb" };
        static int cBinary = sizeof(aszBinary)/sizeof(aszBinary[0]);
        int    i;

        if (!szSuffix)
        {
            if ((szSuffix = strrchr(szPath, '.')) == NULL)
                goto Cleanup;
        }

        for (i = cBinary; i && stricmp(szSuffix, aszBinary[i-1]); i--)
            ;

        if (i != 0)
            goto Cleanup;
    }

    // Don't search inside of the .PCH marker file
    if (!fIsPCHFile)
        {

        BLOCK
            {
            /* Push the directory of this file on the list of directories for
             * include searches.  Save an indication as to whether this include is
             * from a standard place.
             */
            char    szPathT[256];
            char    szNameT[256];

            CopyPath(szPathT, szPath);
            CopyPath(szNameT, szName);

            PushDir(szPathT, szNameT, fPathIsStd);
            }

        while ((sz = fgets(rgch, 256, pfile)) != NULL)
            {
            char *  szInc;
            BOOL    fThisDirNew = FALSE;    /* must be in this directory */
            int     cch = strlen(sz);

            if (cch < 2)
                continue;
            if (sz[cch-1] == '\n')
                sz[cch-1] = '\0';  /* note : will truncate long lines */

            if ((lang == langC && (szInc = SzIncludesC(sz, &fThisDirNew)) != NULL) ||
                (lang == langAsm && (szInc = SzIncludesAsm(sz)) != NULL) ||
                (lang == langRC && (szInc = SzIncludesRC(sz, &fThisDirNew)) != NULL))
                {
                FI *    pfiNew = NULL;
                char    szPathNew[256];
                char    szNameNew[256];
                BOOL    fIsPCH;

                fIsPCH = (szPCHFile && !_stricmp(szInc, szPCHFile));

                /* if file can be found in current directory, cycle
                 * through all current directories possible.
                 */
                if (fThisDirNew)
                    {
                    int     idi;
                    DI *    pdi;

                    for (idi = 0; (pdi = PdiFromIdi(idi)) != NULL; idi++)
                        {
                        MakeName(szPathNew, pdi->szPath, szInc);
                        MakeName(szNameNew, pdi->szName, szInc);

                        /* Do recursive call to include file */
                        pfiNew = PfiDependFn(szPathNew, szNameNew, pdi->fPathIsStd, lang, fIsPCH);

                        /* If we found it, get out of loop */
                        if (pfiNew != NULL)
                            break;
                        }
                    }

                /* If the file hasn't been found yet, look for it
                 * in the standard include directories.
                 */
                if (pfiNew == NULL)
                    {
                    int     isz;

                    for (isz = 0; isz < iszIncMac; isz++)
                        {
                        MakeName(szPathNew, rgszIncPath[isz], szInc);
                        MakeName(szNameNew, rgszIncName[isz], szInc);

                        /* Do recursive call to include file */
                        pfiNew = PfiDependFn(szPathNew, szNameNew, TRUE, lang, fIsPCH);

                        /* If we found it, mark it and get out of loop */
                        if (pfiNew != NULL)
                            break;
                        }
                    }

                /* The file doesn't exist anywhere.  If it was included
                 * with quote marks and the user didn't specify -n, we
                 * will pretend the file is in the same directory as
                 * the file that's including it.
                 */
                if (pfiNew == NULL && fThisDirNew && !fNoGenHeaders)
                    {
                    BOOL fPathIsStd;

                    if (fUseCurDir)
                        {
                        MakeName(szPathNew, ".\\", szInc);
                        MakeName(szNameNew, ".\\", szInc);
                        fPathIsStd = FALSE;

                        /* Look for -d names */
                        if ((pfiNew = PfiLookup(szPathNew, szNameNew,lang)) == NULL)
                            pfiNew = PfiAlloc(szPathNew, szNameNew, FALSE, lang);
                        }
                    else
                        {
                        DI *    pdi;

                        pdi = PdiFromIdi(0);
                        if (pdi == NULL)
                            Fatal("mkdep: internal error");
                        MakeName(szPathNew, pdi->szPath, szInc);
                        MakeName(szNameNew, pdi->szName, szInc);

                        // in this case we already look through existing FI list

                        pfiNew = PfiAlloc(szPathNew, szNameNew,
                            pdi->fPathIsStd && fIgnoreStd, lang);
                        }
                    }

                // If the .PCH marker file has been found, truncate all preceeding .H files
                if (pfiNew && fIsPCH)
                    {
                    FreeAllLk(pfi);
                    FreeFi(pfi->pfiNext);
                    pfi->pfiNext = NULL;
                    }

                /* If we found the file, add it to the list of files */
                if (pfiNew != NULL)
                    {
                    /* add if not already in list */
                    LK *    plk;
                    BOOL    fRedundant = FALSE;

                    for (plk = pfi->plkHead; plk != NULL;
                        plk = plk->plkNext)
                        {
                        if (plk->pfi == pfiNew)
                            {
                            fRedundant = TRUE;
                            break;
                            }
                        }
                    if (!fRedundant)
                        AllocLk(pfi, pfiNew);
                    }
                }
            }

        PopDir();
    }

Cleanup:
    fclose(pfile);
    return pfi;
    }



char *
SzIncludesC(sz, pfThisDir)
/*
  -- return file name of include file or NULL
  -- if returning non-NULL, set *pfThisDir if file should exist in this
    directory (i.e. #include "...").
*/
char *sz;
BOOL *pfThisDir;
    {
    char *szLine = sz;

    while (isspace(*sz))
        sz++;

    if (sz[0] == '#')
        {
        /* Allow space after '#' but before directive.
         */
        sz++;
        while (isspace(sz[0]))
            sz++;

        if (strncmp(sz, "include", 7) == 0)
            {
            /* found it */
            char *  pchEnd;

            sz += 7;
            while (isspace(*sz))
                sz++;
            if ((*sz == '<' && (pchEnd =strchr(sz+1,'>')) !=NULL) ||
                (*sz == '"' && (pchEnd =strchr(sz+1, '"')) !=NULL))
                {
                *pfThisDir = *sz == '"';
                *pchEnd = '\0';
                return sz+1;
                }
            else
                {
                fprintf(stderr, "mkdep: warning: ignoring line : %s\n", szLine);
                return NULL;
                }
            }
        }
    return NULL;
    }



char *
SzIncludesAsm(sz)
/*
  -- return file name of include file or NULL
*/
char *sz;
    {
    char *szLine = sz;

    strlwr(szLine);

    while (isspace(*sz))
        sz++;

    if (strncmp(sz, "include", 7) == 0)
        {
        /* found it */
        char *pchEnd;

        sz += 7;
        while (isspace(*sz))
            sz++;
        pchEnd = sz;
        while (*pchEnd && !isspace(*pchEnd) && *pchEnd != ';')
            pchEnd++;
        if (pchEnd == sz)
            {
            fprintf(stderr, "mkdep: warning: ignoring line : %s\n", szLine);
            return NULL;
            }
        *pchEnd = '\0';
        return sz;
        }
    return NULL;
    }

char *
SzIncludesRC(sz, pfThisDir)
/*
  -- return name of include file or resource file for an RC file
  -- if returning non-NULL, set *pfThisDir if file should exist in this
    directory (i.e. #include "...").
*/
char *sz;
BOOL *pfThisDir;
    {

    static char *aszValidTypes[] =
    {
        "CURSOR", "ICON", "RT_DOCFILE", "TYPELIB", "BITMAP"
    };
    static int cValidTypes = sizeof(aszValidTypes)/sizeof(aszValidTypes[0]);

    char   achIdent[255] = { 0 };
    char   achType[255]  = { 0 };
    char   achFile[255]  = { 0 };
    char * pch;
    int    cch;
    char * szC;
    int    n, i;

    szC = SzIncludesC(sz, pfThisDir);
    if (szC)
        return szC;

    *pfThisDir = TRUE;

    n = sscanf(sz, "%[a-zA-Z0-9_] %[a-zA-Z0-9_] %n%[a-zA-Z0-9.\"]",
               achIdent, achType, &cch, achFile);

    if (n < 3)
        return NULL;

    for (i = cValidTypes; i && stricmp(achType, aszValidTypes[i-1]); i--)
        ;

    if (i == 0)
        return NULL;

    sz += cch;

    while (isspace(*sz))
        sz++;

    sz[strlen(achFile)] = '\0';

    if (*sz == '\"')
        sz++;

    if ((pch = strrchr(sz, '\"')) != NULL)
        *pch = '\0';

    return sz;

    }



FI *
PfiLookup(szPath, szName, lang)
/*
  -- lookup name in current list of FI; if file is of an unknown language and
     lang is not, set the language of this file.
*/
char *  szPath;                 // path name to file
char *  szName;                 // official name of file
LANG    lang;                   // lang desired; langUnknown means any acceptible
    {
    FI *pfi;

    for (pfi = pfiHead; pfi != NULL; pfi = pfi->pfiNext)
        {
        if (strcmp(szPath, pfi->szPath) == 0)
            {
            /* got one */
            if (lang != langUnknown && lang != pfi->lang)
                {
                // want a specific language and that is not what the file is
                if (pfi->lang != langUnknown)
                    fprintf(stderr,
                        "mkdep: warning: language conflict for file %s\n",
                        pfi->szPath);
                else
                    pfi->lang = lang;               // was unknown, set to known
                }

            return pfi;
            }
        }

    return NULL;
    }



FI *
PfiAlloc(szPath, szName, fIgnore, lang)
/*
  -- allocate an FI
*/
char *  szPath;                 // path name to file
char *  szName;                 // official name of file
BOOL    fIgnore;                // true -> don't print this file
LANG    lang;                   // lang for file; can be langUnknown
    {
    FI *pfi;

    if ((pfi = (FI *) malloc(sizeof(FI))) == NULL ||
        (pfi->szName = strdup(szName)) == NULL ||
        (pfi->szPath = strdup(szPath)) == NULL)
        Fatal("out of memory");
    pfi->lang = lang;
    pfi->fIgnore = fIgnore;
    pfi->plkHead = pfi->plkTail = NULL;
    AddToList(pfi, pfiHead, pfiTail, pfiNext, NULL);
    pfi->cout = coutCur;
    return pfi;
    }


VOID
FreeFi(pfiFree)
/*
  -- free an FI and all associated LK and remove from FI list
*/
FI *pfiFree;
    {
    FI *pfiT, *pfiPrev;

    FreeAllLk(pfiFree);

    for (pfiT = pfiHead, pfiPrev = 0; pfiT != pfiFree; pfiPrev = pfiT, pfiT = pfiT->pfiNext)
        {
        // should find it on list
        // Assert(pfiT != NULL);
        }

    DeleteFromList(pfiFree, pfiHead, pfiTail, pfiNext, NULL, pfiPrev);

    //free(pfiFree);
    }



VOID
AllocLk(pfiOwner, pfiNew)
/*
  -- allocate a LK - add to owner list - point to pfiNew
*/
FI *pfiOwner;
FI *pfiNew;
    {
    LK *plk;

    if ((plk = (LK *) malloc(sizeof(LK))) == NULL)
        Fatal("out of memory");
    plk->plkNext = NULL;
    AddToList(plk, pfiOwner->plkHead, pfiOwner->plkTail, plkNext, NULL);
    plk->pfi = pfiNew;
    }


VOID
FreeAllLk(pfi)
/*
  -- free all lk attached to FI
*/
FI *pfi;
    {
    LK *    plk;
    LK *    plkNext;

    for (plk = pfi->plkHead; plk != NULL; plk = plkNext)
        {
        plkNext = plk->plkNext;
        //free(plk);
        }

    pfi->plkHead = NULL;
    pfi->plkTail = NULL;
    }



SZ
SzTransEnv(sz)
/*
  -- return a path string with optional $(...) in it
*/
SZ      sz;
    {
    SZ      szEnv;
    char *  pch;
    char    szT[256];

    if (sz[0] != '$' || sz[1] != '(')
        return sz;
    sz += 2;

    if ((pch = strchr(sz, ')')) == NULL)
        return sz;              // something wrong

    *pch = '\0';
    if ((szEnv = getenv(sz)) == NULL)
        {
        fprintf(stderr,
           "mkdep: warning: environment variable %s not defined\n");
        Fatal("incomplete path");
        }
    *pch = ')';             // restore string

    /* copy the environment variable into buffer */
    strcpy(szT, szEnv);
    strcat(szT, pch+1);             // and rest of string
    NormalizePath(szT);             // normalize again with new prefix
    return strdup(szT);
    }


VOID NormalizePath(sz)
/*
  -- convert path to a normal form in place: forward slashes, no ../, etc.
*/
char *sz;
    {
    char *pch, *pch2;

    /* change all backslashes to forward slashes */
    for (pch=sz; *pch; ++pch)
        if (*pch == '\\')
            *pch = '/';

    /* Remove ".." entries.  (The algorithm below doesn't find all
     * possible cases, but it's good enuff.)
     */
    while ((pch=strstr(sz, "/../")) != NULL)
        {
        *pch = '\0';
        pch2 = strrchr(sz, '/');
        if (pch2 != NULL && pch2[1] != '$' && pch2[1] != '.')
            memmove(pch2+1, pch+4, strlen(pch+1)+1);
        else
            {
            *pch = '/';
            break;
            }
        }

    // remove single . and leading ./
    if (sz[0] == '.')
        {
        if (sz[1] == '\0')
            sz[0] = '\0';

        else if (sz[1] == '/')
            memmove(sz, sz+2, strlen(sz)-2+1);
        }
    }


VOID
MakeName(szDest, szSrcPath, szSrcFile)
/*
  -- copy a path plus filename into a complete filename
  -- normalizes when done
*/
char *  szDest;                 // where to store complete filename
char *  szSrcPath;              // path
char *  szSrcFile;              // filename
    {
    if (szSrcFile[0] && szSrcFile[1]==':')
        {
        if (!(szSrcPath[0] && szSrcPath[1]==':') ||
            tolower(szSrcPath[0]) != tolower(szSrcFile[0]))
            {
            strcpy(szDest, szSrcFile);
            NormalizePath(szDest);
            return;
            }
        *szDest++ = *szSrcFile++;  *szDest++ = *szSrcFile++;
        }
    if (szSrcFile[0] == '/' || szSrcFile[0] == '\\')
        {
        strcpy(szDest, szSrcFile);
        NormalizePath(szDest);
        return;
        }

    strcpy(szDest, szSrcPath);
    if (szDest[0] != '\0')
        {
        char ch = szDest[strlen(szDest)-1];

        if (ch != ':' && ch != '/' && ch != '\\')
            strcat(szDest, "/");
        }
    strcat(szDest, szSrcFile);

    NormalizePath(szDest);
    }




VOID
CopyPath(szDestPath, szSrcFullName)
/*
  -- copy the path part of szSrcFullName into szDestPath
*/
char *  szDestPath;
char *  szSrcFullName;
    {
    int     ich;
    int     ichPathEnd;     // index to end of path part of szSrcFullName
    char    ch;

    /* Figure out where the path part of szSrcFullName ends and the
     * name part begins.
     */
    for (ich = ichPathEnd = 0; (ch=szSrcFullName[ich]) != 0; ++ich)
        if (ch == ':' || ch == '/' || ch == '\\')
            ichPathEnd = ich+1;

    /* Copy the path */
    for (ich = 0; ich < ichPathEnd; ++ich)
        szDestPath[ich] = szSrcFullName[ich];
    szDestPath[ich] = 0;
    }



VOID
PushDir(szPath, szName, fPathIsStd)
/*
  -- push a directory name on the stack of directories for all nested
     includes
*/
char *  szPath;                 // path name of file (e.g. "c:\foo\bar")
char *  szName;                 // official name of file (e.g. "$(INCL)")
BOOL    fPathIsStd;             // path portion of szPath is from standard includes (-I)
    {
    DI *    pdi;

    if ((pdi = malloc(sizeof(DI))) == NULL)
        Fatal("out of memory");
    pdi->szPath = strdup(szPath);
    pdi->szName = strdup(szName);
    pdi->fPathIsStd = fPathIsStd;
    /* Insert at head of list */
    pdi->pdiNext = pdiHead;
    pdiHead = pdi;
    }



VOID
PopDir(void)
/*
  -- pop a directory name from the stack of directories for all nested
     includes
*/
    {
    DI *pdiFree;

    if (pdiHead == NULL)
        Fatal("mkdep: internal error");

    pdiFree = pdiHead;
    pdiHead = pdiHead->pdiNext;

    //free(pdiFree->szPath);
    //free(pdiFree->szName);
    //free(pdiFree);
    }



DI *
PdiFromIdi(idi)
/*
  -- return a pointer to one element from the stack, or NULL
*/
int     idi;                    // index of element to get (0 = top of stack)
    {
    DI *    pdi;

    for (pdi = pdiHead; pdi && idi; idi--)
        pdi = pdi->pdiNext;
    return pdi;
    }

int
AddIncludeDir(szFile)
char * szFile;
{
    if (iszIncMac+1 >= iszIncMax)
        {
        fprintf(stderr,
                "mkdep: warning"
                ": too many include directories"
                "; ignoring %s\n", szFile);
        return 0;
        }
    else
        {
        /* normal include */
        NormalizePath(szFile);
        rgszIncPath[iszIncMac] = SzTransEnv(szFile);
        rgszIncName[iszIncMac] = szFile;
        // fprintf(stderr,"Added include: %s\n",szFile);
        iszIncMac++;
        return 1;
        }
}
