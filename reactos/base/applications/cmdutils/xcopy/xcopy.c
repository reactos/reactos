/*
 * XCOPY - Wine-compatible xcopy program
 *
 * Copyright (C) 2007 J. Edmeades
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

/*
 * FIXME:
 * This should now support all options listed in the xcopy help from
 * windows XP except:
 *  /Z - Copy from network drives in restartable mode
 *  /X - Copy file audit settings (sets /O)
 *  /O - Copy file ownership + ACL info
 *  /G - Copy encrypted files to unencrypted destination
 *  /V - Verifies files
 */

/*
 * Notes:
 * Apparently, valid return codes are:
 *   0 - OK
 *   1 - No files found to copy
 *   2 - CTRL+C during copy
 *   4 - Initialization error, or invalid source specification
 *   5 - Disk write error
 */


#include <stdio.h>
#include <windows.h>
#include <wine/debug.h>
#include <wine/unicode.h>
#include "xcopy.h"

WINE_DEFAULT_DEBUG_CHANNEL(xcopy);

/* Prototypes */
static int XCOPY_ProcessSourceParm(WCHAR *suppliedsource, WCHAR *stem,
                                   WCHAR *spec, DWORD flags);
static int XCOPY_ProcessDestParm(WCHAR *supplieddestination, WCHAR *stem,
                                 WCHAR *spec, WCHAR *srcspec, DWORD flags);
static int XCOPY_DoCopy(WCHAR *srcstem, WCHAR *srcspec,
                        WCHAR *deststem, WCHAR *destspec,
                        DWORD flags);
static BOOL XCOPY_CreateDirectory(const WCHAR* path);
static BOOL XCOPY_ProcessExcludeList(WCHAR* parms);
static BOOL XCOPY_ProcessExcludeFile(WCHAR* filename, WCHAR* endOfName);
static WCHAR *XCOPY_LoadMessage(UINT id);
static void XCOPY_FailMessage(DWORD err);
static int XCOPY_wprintf(const WCHAR *format, ...);

/* Typedefs */
typedef struct _EXCLUDELIST
{
  struct _EXCLUDELIST *next;
  WCHAR               *name;
} EXCLUDELIST;


/* Global variables */
static ULONG filesCopied           = 0;              /* Number of files copied  */
static EXCLUDELIST *excludeList    = NULL;           /* Excluded strings list   */
static FILETIME dateRange;                           /* Date range to copy after*/
static const WCHAR wchr_slash[]   = {'\\', 0};
static const WCHAR wchr_star[]    = {'*', 0};
static const WCHAR wchr_dot[]     = {'.', 0};
static const WCHAR wchr_dotdot[]  = {'.', '.', 0};

/* Constants (Mostly for widechars) */


/* To minimize stack usage during recursion, some temporary variables
   made global                                                        */
static WCHAR copyFrom[MAX_PATH];
static WCHAR copyTo[MAX_PATH];


/* =========================================================================
   main - Main entrypoint for the xcopy command

     Processes the args, and drives the actual copying
   ========================================================================= */
int wmain (int argc, WCHAR *argvW[])
{
    int     rc = 0;
    WCHAR   suppliedsource[MAX_PATH] = {0};   /* As supplied on the cmd line */
    WCHAR   supplieddestination[MAX_PATH] = {0};
    WCHAR   sourcestem[MAX_PATH] = {0};       /* Stem of source          */
    WCHAR   sourcespec[MAX_PATH] = {0};       /* Filespec of source      */
    WCHAR   destinationstem[MAX_PATH] = {0};  /* Stem of destination     */
    WCHAR   destinationspec[MAX_PATH] = {0};  /* Filespec of destination */
    WCHAR   copyCmd[MAXSTRING];               /* COPYCMD env var         */
    DWORD   flags = 0;                        /* Option flags            */
    const WCHAR PROMPTSTR1[]  = {'/', 'Y', 0};
    const WCHAR PROMPTSTR2[]  = {'/', 'y', 0};
    const WCHAR COPYCMD[]  = {'C', 'O', 'P', 'Y', 'C', 'M', 'D', 0};
    const WCHAR EXCLUDE[]  = {'E', 'X', 'C', 'L', 'U', 'D', 'E', ':', 0};

    /*
     * Parse the command line
     */

    /* Confirm at least one parameter */
    if (argc < 2) {
        XCOPY_wprintf(XCOPY_LoadMessage(STRING_INVPARMS));
        return RC_INITERROR;
    }

    /* Preinitialize flags based on COPYCMD */
    if (GetEnvironmentVariableW(COPYCMD, copyCmd, MAXSTRING)) {
        if (wcsstr(copyCmd, PROMPTSTR1) != NULL ||
            wcsstr(copyCmd, PROMPTSTR2) != NULL) {
            flags |= OPT_NOPROMPT;
        }
    }

    /* FIXME: On UNIX, files starting with a '.' are treated as hidden under
       wine, but on windows these can be normal files. At least one installer
       uses files such as .packlist and (validly) expects them to be copied.
       Under wine, if we do not copy hidden files by default then they get
       lose                                                                   */
    flags |= OPT_COPYHIDSYS;

    /* Skip first arg, which is the program name */
    argvW++;

    while (argc > 1)
    {
        argc--;
        WINE_TRACE("Processing Arg: '%s'\n", wine_dbgstr_w(*argvW));

        /* First non-switch parameter is source, second is destination */
        if (*argvW[0] != '/') {
            if (suppliedsource[0] == 0x00) {
                lstrcpyW(suppliedsource, *argvW);
            } else if (supplieddestination[0] == 0x00) {
                lstrcpyW(supplieddestination, *argvW);
            } else {
                XCOPY_wprintf(XCOPY_LoadMessage(STRING_INVPARMS));
                return RC_INITERROR;
            }
        } else {
            /* Process all the switch options
                 Note: Windows docs say /P prompts when dest is created
                       but tests show it is done for each src file
                       regardless of the destination                   */
            switch (toupper(argvW[0][1])) {
            case 'I': flags |= OPT_ASSUMEDIR;     break;
            case 'S': flags |= OPT_RECURSIVE;     break;
            case 'Q': flags |= OPT_QUIET;         break;
            case 'F': flags |= OPT_FULL;          break;
            case 'L': flags |= OPT_SIMULATE;      break;
            case 'W': flags |= OPT_PAUSE;         break;
            case 'T': flags |= OPT_NOCOPY | OPT_RECURSIVE; break;
            case 'Y': flags |= OPT_NOPROMPT;      break;
            case 'N': flags |= OPT_SHORTNAME;     break;
            case 'U': flags |= OPT_MUSTEXIST;     break;
            case 'R': flags |= OPT_REPLACEREAD;   break;
            case 'H': flags |= OPT_COPYHIDSYS;    break;
            case 'C': flags |= OPT_IGNOREERRORS;  break;
            case 'P': flags |= OPT_SRCPROMPT;     break;
            case 'A': flags |= OPT_ARCHIVEONLY;   break;
            case 'M': flags |= OPT_ARCHIVEONLY |
                               OPT_REMOVEARCH;    break;

            /* E can be /E or /EXCLUDE */
            case 'E': if (CompareStringW(LOCALE_USER_DEFAULT,
                                         NORM_IGNORECASE | SORT_STRINGSORT,
                                         &argvW[0][1], 8,
                                         EXCLUDE, -1) == 2) {
                        if (XCOPY_ProcessExcludeList(&argvW[0][9])) {
                          XCOPY_FailMessage(ERROR_INVALID_PARAMETER);
                          return RC_INITERROR;
                        } else flags |= OPT_EXCLUDELIST;
                      } else flags |= OPT_EMPTYDIR | OPT_RECURSIVE;
                      break;

            /* D can be /D or /D: */
            case 'D': if ((argvW[0][2])==':' && isdigit(argvW[0][3])) {
                          SYSTEMTIME st;
                          WCHAR     *pos = &argvW[0][3];
                          BOOL       isError = FALSE;
                          memset(&st, 0x00, sizeof(st));

                          /* Parse the arg : Month */
                          st.wMonth = _wtol(pos);
                          while (*pos && isdigit(*pos)) pos++;
                          if (*pos++ != '-') isError = TRUE;

                          /* Parse the arg : Day */
                          if (!isError) {
                              st.wDay = _wtol(pos);
                              while (*pos && isdigit(*pos)) pos++;
                              if (*pos++ != '-') isError = TRUE;
                          }

                          /* Parse the arg : Day */
                          if (!isError) {
                              st.wYear = _wtol(pos);
                              if (st.wYear < 100) st.wYear+=2000;
                          }

                          if (!isError && SystemTimeToFileTime(&st, &dateRange)) {
                              SYSTEMTIME st;
                              WCHAR datestring[32], timestring[32];

                              flags |= OPT_DATERANGE;

                              /* Debug info: */
                              FileTimeToSystemTime (&dateRange, &st);
                              GetDateFormatW(0, DATE_SHORTDATE, &st, NULL, datestring,
                                             sizeof(datestring)/sizeof(WCHAR));
                              GetTimeFormatW(0, TIME_NOSECONDS, &st,
                                             NULL, timestring, sizeof(timestring)/sizeof(WCHAR));

                              WINE_TRACE("Date being used is: %s %s\n",
                                         wine_dbgstr_w(datestring), wine_dbgstr_w(timestring));
                          } else {
                              XCOPY_FailMessage(ERROR_INVALID_PARAMETER);
                              return RC_INITERROR;
                          }
                      } else {
                          flags |= OPT_DATENEWER;
                      }
                      break;

            case '-': if (toupper(argvW[0][2])=='Y')
                          flags &= ~OPT_NOPROMPT; break;
            case '?': XCOPY_wprintf(XCOPY_LoadMessage(STRING_HELP));
                      return RC_OK;
            default:
                WINE_TRACE("Unhandled parameter '%s'\n", wine_dbgstr_w(*argvW));
                XCOPY_wprintf(XCOPY_LoadMessage(STRING_INVPARM), *argvW);
                return RC_INITERROR;
            }
        }
        argvW++;
    }

    /* Default the destination if not supplied */
    if (supplieddestination[0] == 0x00)
        lstrcpyW(supplieddestination, wchr_dot);

    /* Trace out the supplied information */
    WINE_TRACE("Supplied parameters:\n");
    WINE_TRACE("Source      : '%s'\n", wine_dbgstr_w(suppliedsource));
    WINE_TRACE("Destination : '%s'\n", wine_dbgstr_w(supplieddestination));

    /* Extract required information from source specification */
    rc = XCOPY_ProcessSourceParm(suppliedsource, sourcestem, sourcespec, flags);

    /* Extract required information from destination specification */
    rc = XCOPY_ProcessDestParm(supplieddestination, destinationstem,
                               destinationspec, sourcespec, flags);

    /* Trace out the resulting information */
    WINE_TRACE("Resolved parameters:\n");
    WINE_TRACE("Source Stem : '%s'\n", wine_dbgstr_w(sourcestem));
    WINE_TRACE("Source Spec : '%s'\n", wine_dbgstr_w(sourcespec));
    WINE_TRACE("Dest   Stem : '%s'\n", wine_dbgstr_w(destinationstem));
    WINE_TRACE("Dest   Spec : '%s'\n", wine_dbgstr_w(destinationspec));

    /* Pause if necessary */
    if (flags & OPT_PAUSE) {
        DWORD count;
        char pausestr[10];

        XCOPY_wprintf(XCOPY_LoadMessage(STRING_PAUSE));
        ReadFile (GetStdHandle(STD_INPUT_HANDLE), pausestr, sizeof(pausestr),
                  &count, NULL);
    }

    /* Now do the hard work... */
    rc = XCOPY_DoCopy(sourcestem, sourcespec,
                destinationstem, destinationspec,
                flags);

    /* Clear up exclude list allocated memory */
    while (excludeList) {
        EXCLUDELIST *pos = excludeList;
        excludeList = excludeList -> next;
        HeapFree(GetProcessHeap(), 0, pos->name);
        HeapFree(GetProcessHeap(), 0, pos);
    }

    /* Finished - print trailer and exit */
    if (flags & OPT_SIMULATE) {
        XCOPY_wprintf(XCOPY_LoadMessage(STRING_SIMCOPY), filesCopied);
    } else if (!(flags & OPT_NOCOPY)) {
        XCOPY_wprintf(XCOPY_LoadMessage(STRING_COPY), filesCopied);
    }
    if (rc == RC_OK && filesCopied == 0) rc = RC_NOFILES;
    return rc;

}


/* =========================================================================
   XCOPY_ProcessSourceParm - Takes the supplied source parameter, and
     converts it into a stem and a filespec
   ========================================================================= */
static int XCOPY_ProcessSourceParm(WCHAR *suppliedsource, WCHAR *stem,
                                   WCHAR *spec, DWORD flags)
{
    WCHAR             actualsource[MAX_PATH];
    WCHAR            *starPos;
    WCHAR            *questPos;
    DWORD             attribs;

    /*
     * Validate the source, expanding to full path ensuring it exists
     */
    if (GetFullPathNameW(suppliedsource, MAX_PATH, actualsource, NULL) == 0) {
        WINE_FIXME("Unexpected failure expanding source path (%d)\n", GetLastError());
        return RC_INITERROR;
    }

    /* If full names required, convert to using the full path */
    if (flags & OPT_FULL) {
        lstrcpyW(suppliedsource, actualsource);
    }

    /*
     * Work out the stem of the source
     */

    /* If a directory is supplied, use that as-is (either fully or
          partially qualified)
       If a filename is supplied + a directory or drive path, use that
          as-is
       Otherwise
          If no directory or path specified, add eg. C:
          stem is Drive/Directory is bit up to last \ (or first :)
          spec is bit after that                                         */

    starPos = wcschr(suppliedsource, '*');
    questPos = wcschr(suppliedsource, '?');
    if (starPos || questPos) {
        attribs = 0x00;  /* Ensures skips invalid or directory check below */
    } else {
        attribs = GetFileAttributesW(actualsource);
    }

    if (attribs == INVALID_FILE_ATTRIBUTES) {
        XCOPY_FailMessage(GetLastError());
        return RC_INITERROR;

    /* Directory:
         stem should be exactly as supplied plus a '\', unless it was
          eg. C: in which case no slash required */
    } else if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
        WCHAR lastChar;

        WINE_TRACE("Directory supplied\n");
        lstrcpyW(stem, suppliedsource);
        lastChar = stem[lstrlenW(stem)-1];
        if (lastChar != '\\' && lastChar != ':') {
            lstrcatW(stem, wchr_slash);
        }
        lstrcpyW(spec, wchr_star);

    /* File or wildcard search:
         stem should be:
           Up to and including last slash if directory path supplied
           If c:filename supplied, just the c:
           Otherwise stem should be the current drive letter + ':' */
    } else {
        WCHAR *lastDir;

        WINE_TRACE("Filename supplied\n");
        lastDir   = wcsrchr(suppliedsource, '\\');

        if (lastDir) {
            lstrcpyW(stem, suppliedsource);
            stem[(lastDir-suppliedsource) + 1] = 0x00;
            lstrcpyW(spec, (lastDir+1));
        } else if (suppliedsource[1] == ':') {
            lstrcpyW(stem, suppliedsource);
            stem[2] = 0x00;
            lstrcpyW(spec, suppliedsource+2);
        } else {
            WCHAR curdir[MAXSTRING];
            GetCurrentDirectoryW(sizeof(curdir)/sizeof(WCHAR), curdir);
            stem[0] = curdir[0];
            stem[1] = curdir[1];
            stem[2] = 0x00;
            lstrcpyW(spec, suppliedsource);
        }
    }

    return RC_OK;
}

/* =========================================================================
   XCOPY_ProcessDestParm - Takes the supplied destination parameter, and
     converts it into a stem
   ========================================================================= */
static int XCOPY_ProcessDestParm(WCHAR *supplieddestination, WCHAR *stem, WCHAR *spec,
                                 WCHAR *srcspec, DWORD flags)
{
    WCHAR  actualdestination[MAX_PATH];
    DWORD attribs;
    BOOL isDir = FALSE;

    /*
     * Validate the source, expanding to full path ensuring it exists
     */
    if (GetFullPathNameW(supplieddestination, MAX_PATH, actualdestination, NULL) == 0) {
        WINE_FIXME("Unexpected failure expanding source path (%d)\n", GetLastError());
        return RC_INITERROR;
    }

    /* Destination is either a directory or a file */
    attribs = GetFileAttributesW(actualdestination);

    if (attribs == INVALID_FILE_ATTRIBUTES) {

        /* If /I supplied and wildcard copy, assume directory */
        if (flags & OPT_ASSUMEDIR &&
            (wcschr(srcspec, '?') || wcschr(srcspec, '*'))) {

            isDir = TRUE;

        } else {
            DWORD count;
            char  answer[10] = "";
            WCHAR fileChar[2];
            WCHAR dirChar[2];

            /* Read the F and D characters from the resource file */
            wcscpy(fileChar, XCOPY_LoadMessage(STRING_FILE_CHAR));
            wcscpy(dirChar, XCOPY_LoadMessage(STRING_DIR_CHAR));

            while (answer[0] != fileChar[0] && answer[0] != dirChar[0]) {
                XCOPY_wprintf(XCOPY_LoadMessage(STRING_QISDIR), supplieddestination);

                ReadFile(GetStdHandle(STD_INPUT_HANDLE), answer, sizeof(answer), &count, NULL);
                WINE_TRACE("User answer %c\n", answer[0]);

                answer[0] = toupper(answer[0]);
            }

            if (answer[0] == dirChar[0]) {
                isDir = TRUE;
            } else {
                isDir = FALSE;
            }
        }
    } else {
        isDir = (attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    if (isDir) {
        lstrcpyW(stem, actualdestination);
        *spec = 0x00;

        /* Ensure ends with a '\' */
        if (stem[lstrlenW(stem)-1] != '\\') {
            lstrcatW(stem, wchr_slash);
        }

    } else {
        WCHAR drive[MAX_PATH];
        WCHAR dir[MAX_PATH];
        WCHAR fname[MAX_PATH];
        WCHAR ext[MAX_PATH];
        _wsplitpath(actualdestination, drive, dir, fname, ext);
        lstrcpyW(stem, drive);
        lstrcatW(stem, dir);
        lstrcpyW(spec, fname);
        lstrcatW(spec, ext);
    }
    return RC_OK;
}

/* =========================================================================
   XCOPY_DoCopy - Recursive function to copy files based on input parms
     of a stem and a spec

      This works by using FindFirstFile supplying the source stem and spec.
      If results are found, any non-directory ones are processed
      Then, if /S or /E is supplied, another search is made just for
      directories, and this function is called again for that directory

   ========================================================================= */
static int XCOPY_DoCopy(WCHAR *srcstem, WCHAR *srcspec,
                        WCHAR *deststem, WCHAR *destspec,
                        DWORD flags)
{
    WIN32_FIND_DATAW *finddata;
    HANDLE          h;
    BOOL            findres = TRUE;
    WCHAR           *inputpath, *outputpath;
    BOOL            copiedFile = FALSE;
    DWORD           destAttribs, srcAttribs;
    BOOL            skipFile;
    int             ret = 0;

    /* Allocate some working memory on heap to minimize footprint */
    finddata = HeapAlloc(GetProcessHeap(), 0, sizeof(WIN32_FIND_DATAW));
    inputpath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    outputpath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));

    /* Build the search info into a single parm */
    lstrcpyW(inputpath, srcstem);
    lstrcatW(inputpath, srcspec);

    /* Search 1 - Look for matching files */
    h = FindFirstFileW(inputpath, finddata);
    while (h != INVALID_HANDLE_VALUE && findres) {

        skipFile = FALSE;

        /* Ignore . and .. */
        if (lstrcmpW(finddata->cFileName, wchr_dot)==0 ||
            lstrcmpW(finddata->cFileName, wchr_dotdot)==0 ||
            finddata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            WINE_TRACE("Skipping directory, . or .. (%s)\n", wine_dbgstr_w(finddata->cFileName));
        } else {

            /* Get the filename information */
            lstrcpyW(copyFrom, srcstem);
            if (flags & OPT_SHORTNAME) {
              lstrcatW(copyFrom, finddata->cAlternateFileName);
            } else {
              lstrcatW(copyFrom, finddata->cFileName);
            }

            lstrcpyW(copyTo, deststem);
            if (*destspec == 0x00) {
                if (flags & OPT_SHORTNAME) {
                    lstrcatW(copyTo, finddata->cAlternateFileName);
                } else {
                    lstrcatW(copyTo, finddata->cFileName);
                }
            } else {
                lstrcatW(copyTo, destspec);
            }

            /* Do the copy */
            WINE_TRACE("ACTION: Copy '%s' -> '%s'\n", wine_dbgstr_w(copyFrom),
                                                      wine_dbgstr_w(copyTo));
            if (!copiedFile && !(flags & OPT_SIMULATE)) XCOPY_CreateDirectory(deststem);

            /* See if allowed to copy it */
            srcAttribs = GetFileAttributesW(copyFrom);
            WINE_TRACE("Source attribs: %d\n", srcAttribs);

            if ((srcAttribs & FILE_ATTRIBUTE_HIDDEN) ||
                (srcAttribs & FILE_ATTRIBUTE_SYSTEM)) {

                if (!(flags & OPT_COPYHIDSYS)) {
                    skipFile = TRUE;
                }
            }

            if (!(srcAttribs & FILE_ATTRIBUTE_ARCHIVE) &&
                (flags & OPT_ARCHIVEONLY)) {
                skipFile = TRUE;
            }

            /* See if file exists */
            destAttribs = GetFileAttributesW(copyTo);
            WINE_TRACE("Dest attribs: %d\n", srcAttribs);

            /* Check date ranges if a destination file already exists */
            if (!skipFile && (flags & OPT_DATERANGE) &&
                (CompareFileTime(&finddata->ftLastWriteTime, &dateRange) < 0)) {
                WINE_TRACE("Skipping file as modified date too old\n");
                skipFile = TRUE;
            }

            /* If just /D supplied, only overwrite if src newer than dest */
            if (!skipFile && (flags & OPT_DATENEWER) &&
               (destAttribs != INVALID_FILE_ATTRIBUTES)) {
                HANDLE h = CreateFileW(copyTo, GENERIC_READ, FILE_SHARE_READ,
                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                      NULL);
                if (h != INVALID_HANDLE_VALUE) {
                    FILETIME writeTime;
                    GetFileTime(h, NULL, NULL, &writeTime);

                    if (CompareFileTime(&finddata->ftLastWriteTime, &writeTime) <= 0) {
                        WINE_TRACE("Skipping file as dest newer or same date\n");
                        skipFile = TRUE;
                    }
                    CloseHandle(h);
                }
            }

            /* See if exclude list provided. Note since filenames are case
               insensitive, need to uppercase the filename before doing
               strstr                                                     */
            if (!skipFile && (flags & OPT_EXCLUDELIST)) {
                EXCLUDELIST *pos = excludeList;
                WCHAR copyFromUpper[MAX_PATH];

                /* Uppercase source filename */
                lstrcpyW(copyFromUpper, copyFrom);
                CharUpperBuffW(copyFromUpper, lstrlenW(copyFromUpper));

                /* Loop through testing each exclude line */
                while (pos) {
                    if (wcsstr(copyFromUpper, pos->name) != NULL) {
                        WINE_TRACE("Skipping file as matches exclude '%s'\n",
                                   wine_dbgstr_w(pos->name));
                        skipFile = TRUE;
                        pos = NULL;
                    } else {
                        pos = pos->next;
                    }
                }
            }

            /* Prompt each file if necessary */
            if (!skipFile && (flags & OPT_SRCPROMPT)) {
                DWORD count;
                char  answer[10];
                BOOL  answered = FALSE;
                WCHAR yesChar[2];
                WCHAR noChar[2];

                /* Read the Y and N characters from the resource file */
                wcscpy(yesChar, XCOPY_LoadMessage(STRING_YES_CHAR));
                wcscpy(noChar, XCOPY_LoadMessage(STRING_NO_CHAR));

                while (!answered) {
                    XCOPY_wprintf(XCOPY_LoadMessage(STRING_SRCPROMPT), copyFrom);
                    ReadFile (GetStdHandle(STD_INPUT_HANDLE), answer, sizeof(answer),
                              &count, NULL);

                    answered = TRUE;
                    if (toupper(answer[0]) == noChar[0])
                        skipFile = TRUE;
                    else if (toupper(answer[0]) != yesChar[0])
                        answered = FALSE;
                }
            }

            if (!skipFile &&
                destAttribs != INVALID_FILE_ATTRIBUTES && !(flags & OPT_NOPROMPT)) {
                DWORD count;
                char  answer[10];
                BOOL  answered = FALSE;
                WCHAR yesChar[2];
                WCHAR allChar[2];
                WCHAR noChar[2];

                /* Read the A,Y and N characters from the resource file */
                wcscpy(yesChar, XCOPY_LoadMessage(STRING_YES_CHAR));
                wcscpy(allChar, XCOPY_LoadMessage(STRING_ALL_CHAR));
                wcscpy(noChar, XCOPY_LoadMessage(STRING_NO_CHAR));

                while (!answered) {
                    XCOPY_wprintf(XCOPY_LoadMessage(STRING_OVERWRITE), copyTo);
                    ReadFile (GetStdHandle(STD_INPUT_HANDLE), answer, sizeof(answer),
                              &count, NULL);

                    answered = TRUE;
                    if (toupper(answer[0]) == allChar[0])
                        flags |= OPT_NOPROMPT;
                    else if (toupper(answer[0]) == noChar[0])
                        skipFile = TRUE;
                    else if (toupper(answer[0]) != yesChar[0])
                        answered = FALSE;
                }
            }

            /* See if it has to exist! */
            if (destAttribs == INVALID_FILE_ATTRIBUTES && (flags & OPT_MUSTEXIST)) {
                skipFile = TRUE;
            }

            /* Output a status message */
            if (!skipFile) {
                if (flags & OPT_QUIET) {
                    /* Skip message */
                } else if (flags & OPT_FULL) {
                    const WCHAR infostr[]   = {'%', 's', ' ', '-', '>', ' ',
                                               '%', 's', '\n', 0};

                    XCOPY_wprintf(infostr, copyFrom, copyTo);
                } else {
                    const WCHAR infostr[] = {'%', 's', '\n', 0};
                    XCOPY_wprintf(infostr, copyFrom);
                }

                /* If allowing overwriting of read only files, remove any
                   write protection                                       */
                if ((destAttribs & FILE_ATTRIBUTE_READONLY) &&
                    (flags & OPT_REPLACEREAD)) {
                    SetFileAttributesW(copyTo, destAttribs & ~FILE_ATTRIBUTE_READONLY);
                }

                copiedFile = TRUE;
                if (flags & OPT_SIMULATE || flags & OPT_NOCOPY) {
                    /* Skip copy */
                } else if (CopyFileW(copyFrom, copyTo, FALSE) == 0) {

                    DWORD error = GetLastError();
                    XCOPY_wprintf(XCOPY_LoadMessage(STRING_COPYFAIL),
                           copyFrom, copyTo, error);
                    XCOPY_FailMessage(error);

                    if (flags & OPT_IGNOREERRORS) {
                        skipFile = TRUE;
                    } else {
                        ret = RC_WRITEERROR;
                        goto cleanup;
                    }
                }

                /* If /M supplied, remove the archive bit after successful copy */
                if (!skipFile) {
                    if ((srcAttribs & FILE_ATTRIBUTE_ARCHIVE) &&
                        (flags & OPT_REMOVEARCH)) {
                        SetFileAttributesW(copyFrom, (srcAttribs & ~FILE_ATTRIBUTE_ARCHIVE));
                    }
                    filesCopied++;
                }
            }
        }

        /* Find next file */
        findres = FindNextFileW(h, finddata);
    }
    FindClose(h);

    /* Search 2 - do subdirs */
    if (flags & OPT_RECURSIVE) {
        lstrcpyW(inputpath, srcstem);
        lstrcatW(inputpath, wchr_star);
        findres = TRUE;
        WINE_TRACE("Processing subdirs with spec: %s\n", wine_dbgstr_w(inputpath));

        h = FindFirstFileW(inputpath, finddata);
        while (h != INVALID_HANDLE_VALUE && findres) {

            /* Only looking for dirs */
            if ((finddata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (lstrcmpW(finddata->cFileName, wchr_dot) != 0) &&
                (lstrcmpW(finddata->cFileName, wchr_dotdot) != 0)) {

                WINE_TRACE("Handling subdir: %s\n", wine_dbgstr_w(finddata->cFileName));

                /* Make up recursive information */
                lstrcpyW(inputpath, srcstem);
                lstrcatW(inputpath, finddata->cFileName);
                lstrcatW(inputpath, wchr_slash);

                lstrcpyW(outputpath, deststem);
                if (*destspec == 0x00) {
                    lstrcatW(outputpath, finddata->cFileName);

                    /* If /E is supplied, create the directory now */
                    if ((flags & OPT_EMPTYDIR) &&
                        !(flags & OPT_SIMULATE))
                        XCOPY_CreateDirectory(outputpath);

                    lstrcatW(outputpath, wchr_slash);
                }

                XCOPY_DoCopy(inputpath, srcspec, outputpath, destspec, flags);
            }

            /* Find next one */
            findres = FindNextFileW(h, finddata);
        }
    }

cleanup:

    /* free up memory */
    HeapFree(GetProcessHeap(), 0, finddata);
    HeapFree(GetProcessHeap(), 0, inputpath);
    HeapFree(GetProcessHeap(), 0, outputpath);

    return ret;
}

/* =========================================================================
 * Routine copied from cmd.exe md command -
 * This works recursively. so creating dir1\dir2\dir3 will create dir1 and
 * dir2 if they do not already exist.
 * ========================================================================= */
static BOOL XCOPY_CreateDirectory(const WCHAR* path)
{
    int len;
    WCHAR *new_path;
    BOOL ret = TRUE;

    new_path = HeapAlloc(GetProcessHeap(),0, sizeof(WCHAR) * (lstrlenW(path)+1));
    lstrcpyW(new_path,path);

    while ((len = lstrlenW(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectoryW(new_path,NULL))
    {
        WCHAR *slash;
        DWORD last_error = GetLastError();
        if (last_error == ERROR_ALREADY_EXISTS)
            break;

        if (last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if (!(slash = wcsrchr(new_path,'\\')) && ! (slash = wcsrchr(new_path,'/')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if (!XCOPY_CreateDirectory(new_path))
        {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }
    HeapFree(GetProcessHeap(),0,new_path);
    return ret;
}

/* =========================================================================
 * Process the /EXCLUDE: file list, building up a list of substrings to
 * avoid copying
 * Returns TRUE on any failure
 * ========================================================================= */
static BOOL XCOPY_ProcessExcludeList(WCHAR* parms) {

    WCHAR *filenameStart = parms;

    WINE_TRACE("/EXCLUDE parms: '%s'\n", wine_dbgstr_w(parms));
    excludeList = NULL;

    while (*parms && *parms != ' ' && *parms != '/') {

        /* If found '+' then process the file found so far */
        if (*parms == '+') {
            if (XCOPY_ProcessExcludeFile(filenameStart, parms)) {
                return TRUE;
            }
            filenameStart = parms+1;
        }
        parms++;
    }

    if (filenameStart != parms) {
        if (XCOPY_ProcessExcludeFile(filenameStart, parms)) {
            return TRUE;
        }
    }

    return FALSE;
}

/* =========================================================================
 * Process a single file from the /EXCLUDE: file list, building up a list
 * of substrings to avoid copying
 * Returns TRUE on any failure
 * ========================================================================= */
static BOOL XCOPY_ProcessExcludeFile(WCHAR* filename, WCHAR* endOfName) {

    WCHAR   endChar = *endOfName;
    WCHAR   buffer[MAXSTRING];
    FILE   *inFile  = NULL;
    const WCHAR readTextMode[]  = {'r', 't', 0};

    /* Null terminate the filename (temporarily updates the filename hence
         parms not const)                                                 */
    *endOfName = 0x00;

    /* Open the file */
    inFile = _wfopen(filename, readTextMode);
    if (inFile == NULL) {
        XCOPY_wprintf(XCOPY_LoadMessage(STRING_OPENFAIL), filename);
        *endOfName = endChar;
        return TRUE;
    }

    /* Process line by line */
    while (fgetws(buffer, sizeof(buffer)/sizeof(WCHAR), inFile) != NULL) {
        EXCLUDELIST *thisEntry;
        int length = lstrlenW(buffer);

        /* Strip CRLF */
        buffer[length-1] = 0x00;

        /* If more than CRLF */
        if (length > 1) {
          thisEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(EXCLUDELIST));
          thisEntry->next = excludeList;
          excludeList = thisEntry;
          thisEntry->name = HeapAlloc(GetProcessHeap(), 0,
                                      (length * sizeof(WCHAR))+1);
          lstrcpyW(thisEntry->name, buffer);
          CharUpperBuffW(thisEntry->name, length);
          WINE_TRACE("Read line : '%s'\n", wine_dbgstr_w(thisEntry->name));
        }
    }

    /* See if EOF or error occurred */
    if (!feof(inFile)) {
        XCOPY_wprintf(XCOPY_LoadMessage(STRING_READFAIL), filename);
        *endOfName = endChar;
        return TRUE;
    }

    /* Revert the input string to original form, and cleanup + return */
    *endOfName = endChar;
    fclose(inFile);
    return FALSE;
}

/* =========================================================================
 * Load a string from the resource file, handling any error
 * Returns string retrieved from resource file
 * ========================================================================= */
static WCHAR *XCOPY_LoadMessage(UINT id) {
    static WCHAR msg[MAXSTRING];
    const WCHAR failedMsg[]  = {'F', 'a', 'i', 'l', 'e', 'd', '!', 0};

    if (!LoadStringW(GetModuleHandleW(NULL), id, msg, sizeof(msg)/sizeof(WCHAR))) {
       WINE_FIXME("LoadString failed with %d\n", GetLastError());
       lstrcpyW(msg, failedMsg);
    }
    return msg;
}

/* =========================================================================
 * Load a string for a system error and writes it to the screen
 * Returns string retrieved from resource file
 * ========================================================================= */
static void XCOPY_FailMessage(DWORD err) {
    LPWSTR lpMsgBuf;
    int status;

    status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL, err, 0,
                            (LPWSTR) &lpMsgBuf, 0, NULL);
    if (!status) {
      WINE_FIXME("FIXME: Cannot display message for error %d, status %d\n",
                 err, GetLastError());
    } else {
      const WCHAR infostr[] = {'%', 's', '\n', 0};
      XCOPY_wprintf(infostr, lpMsgBuf);
      LocalFree ((HLOCAL)lpMsgBuf);
    }
}

/* =========================================================================
 * Output a formatted unicode string. Ideally this will go to the console
 *  and hence required WriteConsoleW to output it, however if file i/o is
 *  redirected, it needs to be WriteFile'd using OEM (not ANSI) format
 * ========================================================================= */
int XCOPY_wprintf(const WCHAR *format, ...) {

    static WCHAR *output_bufW = NULL;
    static char  *output_bufA = NULL;
    static BOOL  toConsole    = TRUE;
    static BOOL  traceOutput  = FALSE;
#define MAX_WRITECONSOLE_SIZE 65535

    va_list parms;
    DWORD   len, nOut;
    DWORD   res = 0;

    /*
     * Allocate buffer to use when writing to console
     * Note: Not freed - memory will be allocated once and released when
     *         xcopy ends
     */

    if (!output_bufW) output_bufW = HeapAlloc(GetProcessHeap(), 0,
                                              MAX_WRITECONSOLE_SIZE);
    if (!output_bufW) {
      WINE_FIXME("Out of memory - could not allocate 2 x 64K buffers\n");
      return 0;
    }

    va_start(parms, format);
    len = vsnprintfW(output_bufW, MAX_WRITECONSOLE_SIZE/sizeof(WCHAR), format, parms);
    va_end(parms);

    /* Try to write as unicode all the time we think its a console */
    if (toConsole) {
      res = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                          output_bufW, len, &nOut, NULL);
    }

    /* If writing to console has failed (ever) we assume its file
       i/o so convert to OEM codepage and output                  */
    if (!res) {
      BOOL usedDefaultChar = FALSE;
      DWORD convertedChars;

      toConsole = FALSE;

      /*
       * Allocate buffer to use when writing to file. Not freed, as above
       */
      if (!output_bufA) output_bufA = HeapAlloc(GetProcessHeap(), 0,
                                                MAX_WRITECONSOLE_SIZE);
      if (!output_bufA) {
        WINE_FIXME("Out of memory - could not allocate 2 x 64K buffers\n");
        return 0;
      }

      /* Convert to OEM, then output */
      convertedChars = WideCharToMultiByte(GetConsoleOutputCP(), 0, output_bufW,
                          len, output_bufA, MAX_WRITECONSOLE_SIZE,
                          "?", &usedDefaultChar);
      WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), output_bufA, convertedChars,
                &nOut, FALSE);
    }

    /* Trace whether screen or console */
    if (!traceOutput) {
      WINE_TRACE("Writing to console? (%d)\n", toConsole);
      traceOutput = TRUE;
    }
    return nOut;
}
