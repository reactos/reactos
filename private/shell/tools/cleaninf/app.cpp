
#include "priv.h"       

HINSTANCE g_hinst;

#define APP_VERSION         "Version 0.3"


// Don't link to shlwapi.dll so this is a stand-alone tool

/*----------------------------------------------------------
Purpose: If a path is contained in quotes then remove them.

*/
void
PathUnquoteSpaces(
    LPTSTR lpsz)
{
    int cch;

    cch = lstrlen(lpsz);

    // Are the first and last chars quotes?
    if (lpsz[0] == TEXT('"') && lpsz[cch-1] == TEXT('"'))
    {
        // Yep, remove them.
        lpsz[cch-1] = TEXT('\0');
        hmemcpy(lpsz, lpsz+1, (cch-1) * SIZEOF(TCHAR));
    }
}


#define CH_WHACK TEXT(FILENAME_SEPARATOR)

LPTSTR
PathFindExtension(
    LPCTSTR pszPath)
{
    LPCTSTR pszDot = NULL;

    if (pszPath)
    {
        for (; *pszPath; pszPath = CharNext(pszPath))
        {
            switch (*pszPath) {
            case TEXT('.'):
                pszDot = pszPath;         // remember the last dot
                break;
            case CH_WHACK:
            case TEXT(' '):         // extensions can't have spaces
                pszDot = NULL;       // forget last dot, it was in a directory
                break;
            }
        }
    }

    // if we found the extension, return ptr to the dot, else
    // ptr to end of the string (NULL extension) (cast->non const)
    return pszDot ? (LPTSTR)pszDot : (LPTSTR)pszPath;
}



__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}


LPSTR FAR PASCAL StrChrA(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}

BOOL
StrTrim(
    IN OUT LPSTR  pszTrimMe,
    IN     LPCSTR pszTrimChars)
    {
    BOOL bRet = FALSE;
    LPSTR psz;
    LPSTR pszStartMeat;
    LPSTR pszMark = NULL;

    ASSERT(IS_VALID_STRING_PTRA(pszTrimMe, -1));
    ASSERT(IS_VALID_STRING_PTRA(pszTrimChars, -1));

    if (pszTrimMe)
    {
        /* Trim leading characters. */

        psz = pszTrimMe;

        while (*psz && StrChrA(pszTrimChars, *psz))
            psz = CharNextA(psz);

        pszStartMeat = psz;

        /* Trim trailing characters. */

        // (The old algorithm used to start from the end and go
        // backwards, but that is piggy because DBCS version of
        // CharPrev iterates from the beginning of the string
        // on every call.)

        while (*psz)
            {
            if (StrChrA(pszTrimChars, *psz))
                {
                pszMark = psz;
                }
            else
                {
                pszMark = NULL;
                }
            psz = CharNextA(psz);
            }

        // Any trailing characters to clip?
        if (pszMark)
            {
            // Yes
            *pszMark = '\0';
            bRet = TRUE;
            }

        /* Relocate stripped string. */

        if (pszStartMeat > pszTrimMe)
        {
            /* (+ 1) for null terminator. */
            MoveMemory(pszTrimMe, pszStartMeat, CbFromCchA(lstrlenA(pszStartMeat) + 1));
            bRet = TRUE;
        }
        else
            ASSERT(pszStartMeat == pszTrimMe);

        ASSERT(IS_VALID_STRING_PTRA(pszTrimMe, -1));
    }

    return bRet;
    }



void PrintSyntax(void)
{
    fprintf(stderr, "cleaninf.exe  " APP_VERSION "\n\n"
                    "Cleans up an inf, html, or script file for public distribution or for packing\n"
                    "into a resource.  Without any options, this removes all comments.  This\n"
                    "tool recognizes .inf, .htm, .hta, .js and .htc files by default.\n\n"
                    "Syntax:  cleaninf [-w] [-inf | -htm | -js | -htc] sourceFile destFile\n\n"
                    "          -w     Strip whitespace\n\n"
                    "         These flags are mutually exclusive, and will treat the file\n"
                    "         accordingly, regardless of extension:\n"
                    "          -inf   Treat file as a .inf file\n"
                    "          -htm   Treat file as a .htm file\n"
                    "          -js    Treat file as a .js file\n"
                    "          -htc   Treat file as a .htc file\n");
}    


/*----------------------------------------------------------
Purpose: Worker function to do the work

*/
int
DoWork(int cArgs, char * rgszArgs[])
{
    LPSTR psz;
    LPSTR pszSrc = NULL;
    LPSTR pszDest = NULL;
    DWORD dwFlags = 0;
    int i;
    int nRet = 0;

    // (The first arg is actually the exe.  Skip that.)

    for (i = 1; i < cArgs; i++)
    {
        psz = rgszArgs[i];

        // Check for options
        if ('/' == *psz || '-' == *psz)
        {
            psz++;
            switch (*psz)
            {
            case '?':
                // Help
                PrintSyntax();
                return 0;

            case 'w':
                dwFlags |= PFF_WHITESPACE;
                break;

            default:
                if (0 == strncmp(psz, "inf", 3))
                {
                    dwFlags |= PFF_INF;
                }
                else if (0 == strncmp(psz, "htm", 3))
                {
                    dwFlags |= PFF_HTML;
                }
                else if (0 == strncmp(psz, "js", 2))
                {
                    dwFlags |= PFF_JS;
                }
                else if (0 == strncmp(psz, "htc", 3))
                {
                    dwFlags |= PFF_HTC;
                }
                else
                {
                    // unknown
                    fprintf(stderr, "Invalid option -%c\n", *psz);
                    return -1;
                }
                break;
            }
        }
        else if (!pszSrc)
            pszSrc = rgszArgs[i];
        else if (!pszDest)
            pszDest = rgszArgs[i];
        else
        {
            fprintf(stderr, "Ignoring invalid parameter - %s\n", rgszArgs[i]);
        }
    }

    if (!pszSrc || !pszDest)
    {
        PrintSyntax();
        return -2;
    }

    // Has the file type already been explicitly specified?
    if ( !(dwFlags & (PFF_INF | PFF_HTML | PFF_JS | PFF_HTC)) )
    {
        // No; determine it based on the extension
        LPTSTR pszExt = PathFindExtension(pszSrc);

        if (pszExt)
        {
            if (0 == lstrcmpi(pszExt, ".htm") || 0 == lstrcmpi(pszExt, ".hta"))
                dwFlags |= PFF_HTML;
            else if (0 == lstrcmpi(pszExt, ".js"))
                dwFlags |= PFF_JS;
            else if (0 == lstrcmpi(pszExt, ".htc"))
                dwFlags |= PFF_HTC;
        }
    }
    
    // Open the files
    PathUnquoteSpaces(pszSrc);
    PathUnquoteSpaces(pszDest);

    FILE * pfileSrc = fopen(pszSrc, "r");

    if (NULL == pfileSrc)
    {
        fprintf(stderr, "\"%s\" could not be opened", pszSrc);
        nRet = -3;
    }
    else
    {
        FILE * pfileDest = fopen(pszDest, "w");

        if (NULL == pfileDest)
        {
            fprintf(stderr, "\"%s\" could not be created", pszDest);
            nRet = -4;
        }
        else
        {
            CParseFile parsefile;

            parsefile.Parse(pfileSrc, pfileDest, dwFlags);
            
            fclose(pfileDest);
        }
        
        fclose(pfileSrc);
    }
    return nRet;
}


#ifdef UNIX

EXTERN_C
HINSTANCE MwMainwinInitLite(int argc, char *argv[], void* lParam);

EXTERN_C
HINSTANCE mainwin_init(int argc, char *argv[])
{
          return MwMainwinInitLite(argc, argv, NULL);
}

#endif

int __cdecl main(int argc, char * argv[])
{
    return DoWork(argc, argv);
}    

