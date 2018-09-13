//---------------------------------------------------------------------------
// Handle dde conversations.
//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop
//#include "cabdde.h"
//#include "vdate.h"
//#include "hnfblock.h"
//#include "resource.h"
//#include "cmdfile.h"

#include <mluisupp.h>

// CHEEBUGBUG: should this be done native for each platform?
#ifdef UNICODE
#define CP_WINNATURAL   CP_WINUNICODE
#else
#define CP_WINNATURAL   CP_WINANSI
#endif

// BUGBUG: this data is duplicated in all instances of the
// cabinet but is only used by the first instance!

DWORD g_dwDDEInst = 0L;
HSZ   g_hszTopic = 0;
HSZ   g_hszService = 0;
HSZ   g_hszStar = 0;
HSZ   g_hszShell = 0;
HSZ   g_hszAppProps = 0;
HSZ   g_hszFolders = 0;
BOOL  g_LFNGroups = FALSE;
HWND  g_hwndDde = NULL;
UINT  g_nTimer = 0;
HWND  g_hwndDDEML = NULL;
HWND  g_hwndClient = NULL;
DWORD g_dwAppFlags = DDECONV_NONE;

// From shell32\nothunk.c
extern "C" void SHGlobalDefect(DWORD dwHnd32);

// From Shell32\shlobjs.c
extern "C" void WINAPI SHAbortInvokeCommand();

#define IDT_REPEAT_ACKS             10

//
// Lets define a simple structure that handles the different converstations
// that might be happening concurently, I don't expect many conversations
// to happen at the same time, so this can be rather simple
//
struct _DDECONV;
typedef struct _DDECONV  DDECONV, * PDDECONV;
struct _DDECONV
{
    DWORD       dwFlags;                // Flags.
    PDDECONV    pddecNext;
    HCONV       hconv;                  // Handle to the conversation;
    BOOL        fDirty;                 // Has any changes been made;
    IShellLink  *psl;                   // temp link to work with
    TCHAR        szGroup[MAX_PATH];     // Group pathname
    TCHAR        szShare[MAX_PATH];      // Used to override UNC connections.
    TCHAR        chDrive;                // Used to override UNC connections.
};

PDDECONV    g_pddecHead = NULL;         // List of current conversations.
LPTSTR       g_pszLastGroupName = NULL;  // Last group name used for items
                                        // that are created by programs
                                        // that do not setup a context


typedef BOOL (*DDECOMMAND)(LPTSTR lpszBuf, UINT * lpwCmd, PDDECONV pddec);
typedef struct _DDECOMMANDINFO
{
    LPCTSTR     pszCommand;
    DDECOMMAND lpfnCommand;
} DDECOMMANDINFO;

DWORD GetDDEAppFlagsFromWindow(HWND hwnd);
UINT* GetDDECommands(LPTSTR lpCmd, const DDECOMMANDINFO *lpsCommands, BOOL fLFN);

void DDE_AddShellServices(void);
void DDE_RemoveShellServices(void);

BOOL DDE_CreateGroup(LPTSTR, UINT *, PDDECONV);
BOOL DDE_ShowGroup(LPTSTR, UINT *, PDDECONV);
BOOL DDE_AddItem(LPTSTR, UINT *, PDDECONV);
BOOL DDE_ExitProgman(LPTSTR, UINT *, PDDECONV);
BOOL DDE_DeleteGroup(LPTSTR, UINT *, PDDECONV);
BOOL DDE_DeleteItem(LPTSTR, UINT *, PDDECONV);
// BOOL NEAR PASCAL DDE_ReplaceItem(LPSTR, UINT *, PDDECONV);
#define DDE_ReplaceItem DDE_DeleteItem
BOOL DDE_Reload(LPTSTR, UINT *, PDDECONV);
BOOL DDE_ViewFolder(LPTSTR, UINT *, PDDECONV);
BOOL DDE_ExploreFolder(LPTSTR, UINT *, PDDECONV);
BOOL DDE_FindFolder(LPTSTR, UINT *, PDDECONV);
BOOL DDE_OpenFindFile(LPTSTR, UINT *, PDDECONV);
BOOL DDE_ConfirmID(LPTSTR lpszBuf, UINT * lpwCmd, PDDECONV pddec);
BOOL DDE_ShellFile(LPTSTR lpszBuf, UINT * lpwCmd, PDDECONV pddec);
#ifdef DEBUG
BOOL DDE_Beep(LPTSTR, UINT *, PDDECONV);
#endif
void MapGroupName(LPCTSTR lpszOld, LPTSTR lpszNew, ULONG cbNew);

TCHAR const c_szGroupGroup[] = TEXT("groups");
#define c_szStarDotStar TEXT("*.*")
 CHAR const c_szCRLF[] = "\r\n";

TCHAR const c_szCreateGroup[]   = TEXT("CreateGroup");
TCHAR const c_szShowGroup[]     = TEXT("ShowGroup");
TCHAR const c_szAddItem[]       = TEXT("AddItem");
TCHAR const c_szExitProgman[]   = TEXT("ExitProgman");
TCHAR const c_szDeleteGroup[]   = TEXT("DeleteGroup");
TCHAR const c_szDeleteItem[]    = TEXT("DeleteItem");
TCHAR const c_szReplaceItem[]   = TEXT("ReplaceItem");
TCHAR const c_szReload[]        = TEXT("Reload");
TCHAR const c_szFindFolder[]    = TEXT("FindFolder");
TCHAR const c_szOpenFindFile[]  = TEXT("OpenFindFile");
#define c_szDotPif TEXT(".pif")
TCHAR const c_szTrioDataFax[]   = TEXT("DDEClient");
TCHAR const c_szTalkToPlus[]    = TEXT("ddeClass");
TCHAR const c_szStartUp[]       = TEXT("StartUp");
TCHAR const c_szCCMail[]        = TEXT("ccInsDDE");
TCHAR const c_szBodyWorks[]     = TEXT("BWWFrame");
TCHAR const c_szMediaRecorder[] = TEXT("DDEClientWndClass");
TCHAR const c_szDiscis[]        = TEXT("BACKSCAPE");
TCHAR const c_szMediaRecOld[]   = TEXT("MediaRecorder");
TCHAR const c_szMediaRecNew[]   = TEXT("Media Recorder");
TCHAR const c_szDialog[]        = TEXT("#32770");
TCHAR const c_szJourneyMan[]    = TEXT("Sender");
TCHAR const c_szCADDE[]         = TEXT("CA_DDECLASS");
TCHAR const c_szFaxServe[]      = TEXT("Install");
TCHAR const c_szMakePMG[]       = TEXT("Make Program Manager Group");
TCHAR const c_szViewFolder[]    = TEXT("ViewFolder");
TCHAR const c_szExploreFolder[] = TEXT("ExploreFolder");
TCHAR const c_szRUCabinet[]     = TEXT("ConfirmCabinetID");
 CHAR const c_szNULLA[] = "";
TCHAR const c_szGetIcon[] = TEXT("GetIcon");
TCHAR const c_szGetDescription[] = TEXT("GetDescription");
TCHAR const c_szGetWorkingDir[] = TEXT("GetWorkingDir");

TCHAR const c_szService[] = TEXT("Progman");
TCHAR const c_szTopic[] = TEXT("Progman");
#define c_szShell TEXT("Shell")
TCHAR const c_szFolders[] = TEXT("Folders");
TCHAR const c_szMapGroups[] = TEXT("MapGroups");
#define c_szStar TEXT("*")
TCHAR const c_szAppProps[] = TEXT("AppProperties");
#define c_szDotLnk TEXT(".lnk")
 CHAR const c_szDesktopIniA[] = STR_DESKTOPINIA;
 CHAR const c_szGroupsA[] = "Groups";
 
TCHAR const c_szShellFile[]     = TEXT("ShellFile");

TCHAR const c_szMrPostman[]     = TEXT("setupPmFrame");

#ifdef DEBUG
TCHAR const c_szBeep[]          = TEXT("Beep");
#endif


DDECOMMANDINFO const c_sDDECommands[] =
{
    { c_szCreateGroup  , DDE_CreateGroup   },
    { c_szShowGroup    , DDE_ShowGroup     },
    { c_szAddItem      , DDE_AddItem       },
    { c_szExitProgman  , DDE_ExitProgman   },
    { c_szDeleteGroup  , DDE_DeleteGroup   },
    { c_szDeleteItem   , DDE_DeleteItem    },
    { c_szReplaceItem  , DDE_ReplaceItem   },
    { c_szReload       , DDE_Reload        },
    { c_szViewFolder   , DDE_ViewFolder    },
    { c_szExploreFolder, DDE_ExploreFolder },
    { c_szFindFolder,    DDE_FindFolder    },
    { c_szOpenFindFile,  DDE_OpenFindFile  },
    { c_szRUCabinet,     DDE_ConfirmID},
    { c_szShellFile,     DDE_ShellFile},
#ifdef DEBUG
    { c_szBeep         , DDE_Beep          },
#endif
    { 0, 0 },
} ;

#define HDDENULL        ((HDDEDATA)NULL)
#define HSZNULL         ((HSZ)NULL)
#define _DdeCreateStringHandle(dwInst, lpsz, nCP)       DdeCreateStringHandle(dwInst, (LPTSTR)lpsz, nCP)
#define _LocalReAlloc(h, cb, flags)      (h ? LocalReAlloc(h, cb, flags) : LocalAlloc(LPTR, cb))

//-------------------------------------------------------------------------
#define ITEMSPERROW 7

typedef struct
{
    LPTSTR pszDesc;
    LPTSTR pszCL;
    LPTSTR pszWD;
    LPTSTR pszIconPath;
    int iIcon;
    BOOL fMin;
    WORD wHotkey;
} GROUPITEM, *PGROUPITEM;


//--------------------------------------------------------------------------
//
// A couple helper functions only this code uses:
//

STDAPI_(void) OpenGroup(LPCTSTR pszGroup, int nCmdShow)
{
    IETHREADPARAM *piei = SHCreateIETHREADPARAM(NULL, 0, NULL, NULL);
    if (piei) 
    {
        ASSERT(*pszGroup);
        piei->pidl = ILCreateFromPath(pszGroup);
        piei->uFlags = COF_NORMAL | COF_WAITFORPENDING;
        piei->nCmdShow = SW_NORMAL;

        SHOpenFolderWindow(piei);
    }
}


//--------------------------------------------------------------------------
// Returns a pointer to the first non-whitespace character in a string.
LPTSTR SkipWhite(LPTSTR lpsz)
    {
    /* prevent sign extension in case of DBCS */
    while (*lpsz && (TUCHAR)*lpsz <= TEXT(' '))
        lpsz++;

    return(lpsz);
    }

//--------------------------------------------------------------------------
// Reads a parameter out of a string removing leading and trailing whitespace.
// Terminated by , or ).  ] [ and ( are not allowed.  Exception: quoted
// strings are treated as a whole parameter and may contain []() and ,.
// Places the offset of the first character of the parameter into some place
// and NULL terminates the parameter.
// If fIncludeQuotes is false it is assumed that quoted strings will contain single
// commands (the quotes will be removed and anything following the quotes will
// be ignored until the next comma). If fIncludeQuotes is TRUE, the contents of
// the quoted string will be ignored as before but the quotes won't be
// removed and anything following the quotes will remain.
LPTSTR GetOneParameter(LPCTSTR lpCmdStart, LPTSTR lpCmd,
    UINT *lpW, BOOL fIncludeQuotes)
    {
    LPTSTR     lpT;

    switch (*lpCmd)
        {
        case TEXT(','):
            *lpW = lpCmd - lpCmdStart;  // compute offset
            *lpCmd++ = 0;                /* comma: becomes a NULL string */
            break;

        case TEXT('"'):
            if (fIncludeQuotes)
            {
                TraceMsg(TF_DDE, "GetOneParameter: Keeping quotes.");

                // quoted string... don't trim off "
                *lpW = lpCmd - lpCmdStart;  // compute offset
                ++lpCmd;
                while (*lpCmd && *lpCmd != TEXT('"'))
                    lpCmd = CharNext(lpCmd);
                if (!*lpCmd)
                    return(NULL);
                lpT = lpCmd;
                ++lpCmd;

                goto skiptocomma;
            }
            else
            {
                // quoted string... trim off "
                ++lpCmd;
                *lpW = lpCmd - lpCmdStart;  // compute offset
                while (*lpCmd && *lpCmd != TEXT('"'))
                    lpCmd = CharNext(lpCmd);
                if (!*lpCmd)
                    return(NULL);
                *lpCmd++ = 0;
                lpCmd = SkipWhite(lpCmd);

                // If there's a comma next then skip over it, else just go on as
                // normal.
                if (*lpCmd == TEXT(','))
                    lpCmd++;
            }
            break;

        case TEXT(')'):
            return(lpCmd);                /* we ought not to hit this */

        case TEXT('('):
        case TEXT('['):
        case TEXT(']'):
            return(NULL);                 /* these are illegal */

        default:
            lpT = lpCmd;
            *lpW = lpCmd - lpCmdStart;  // compute offset
skiptocomma:
            while (*lpCmd && *lpCmd != TEXT(',') && *lpCmd != TEXT(')'))
            {
                /* Check for illegal characters. */
                if (*lpCmd == TEXT(']') || *lpCmd == TEXT('[') || *lpCmd == TEXT('(') )
                    return(NULL);

                /* Remove trailing whitespace */
                /* prevent sign extension */
                if ((TUCHAR)*lpCmd > TEXT(' '))
                    lpT = lpCmd;

                lpCmd = CharNext(lpCmd);
            }

            /* Eat any trailing comma. */
            if (*lpCmd == TEXT(','))
                lpCmd++;

            /* NULL terminator after last nonblank character -- may write over
             * terminating ')' but the caller checks for that because this is
             * a hack.
             */

#ifdef UNICODE
            lpT[1] = 0;
#else
            lpT[IsDBCSLeadByte(*lpT) ? 2 : 1] = 0;
#endif
            break;
        }

    // Return next unused character.
    return(lpCmd);
    }

//---------------------------------------------------------------------------
// Extracts an alphabetic string and looks it up in a list of possible
// commands, returning a pointer to the character after the command and
// sticking the command index somewhere.


LPTSTR GetCommandName(LPTSTR lpCmd, const DDECOMMANDINFO * lpsCommands, UINT *lpW)
    {
    TCHAR chT;
    UINT iCmd = 0;
    LPTSTR lpT;

    /* Eat any white space. */
    lpT = lpCmd = SkipWhite(lpCmd);

    /* Find the end of the token. */
    while (IsCharAlpha(*lpCmd))
        lpCmd = CharNext(lpCmd);

    /* Temporarily NULL terminate it. */
    chT = *lpCmd;
    *lpCmd = 0;

    /* Look up the token in a list of commands. */
    *lpW = (UINT)-1;
    while (lpsCommands->pszCommand)
        {
        if (!lstrcmpi(lpsCommands->pszCommand, lpT))
            {
            *lpW = iCmd;
            break;
            } 
        iCmd++;
        ++lpsCommands;
        }

    *lpCmd = chT;

    return(lpCmd);
    }

/*  Called with: pointer to a string to parse and a pointer to a
 *  list of sz's containing the allowed function names.
 *  The function returns a global handle to an array of words containing
 *  one or more command definitions.  A command definition consists of
 *  a command index, a parameter count, and that number of offsets.  Each
 *  offset is an offset to a parameter in lpCmd which is now zero terminated.
 *  The list of command is terminated with -1.
 *  If there was a syntax error the return value is NULL.
 *  Caller must free block.
 */

UINT* GetDDECommands(LPTSTR lpCmd, const DDECOMMANDINFO * lpsCommands, BOOL fLFN)
{
  UINT cParm, cCmd = 0;
  UINT *lpW;
  UINT *lpRet;
  LPCTSTR lpCmdStart = lpCmd;
  BOOL fIncludeQuotes = FALSE;

  lpRet = lpW = (UINT*)GlobalAlloc(GPTR, 512L);
  if (!lpRet)
      return 0;

  while (*lpCmd)
    {
      /* Skip leading whitespace. */
      lpCmd = SkipWhite(lpCmd);

      /* Are we at a NULL? */
      if (!*lpCmd)
        {
          /* Did we find any commands yet? */
          if (cCmd)
              goto GDEExit;
          else
              goto GDEErrExit;
        }

      /* Each command should be inside square brackets. */
      if (*lpCmd != TEXT('['))
          goto GDEErrExit;
      lpCmd++;

      /* Get the command name. */
      lpCmd = GetCommandName(lpCmd, lpsCommands, lpW);
      if (*lpW == (UINT)-1)
          goto GDEErrExit;

      // We need to leave quotes in for the first param of an AddItem.
      if (fLFN && *lpW == 2)
      {
          TraceMsg(TF_DDE, "GetDDECommands: Potential LFN AddItem command...");
          fIncludeQuotes = TRUE;
      }

      lpW++;

      /* Start with zero parms. */
      cParm = 0;
      lpCmd = SkipWhite(lpCmd);

      /* Check for opening '(' */
      if (*lpCmd == TEXT('('))
        {
          lpCmd++;

          /* Skip white space and then find some parameters (may be none). */
          lpCmd = SkipWhite(lpCmd);

          while (*lpCmd != TEXT(')'))
            {
              if (!*lpCmd)
                  goto GDEErrExit;

              // Only the first param of the AddItem command needs to
              // handle quotes from LFN guys.
              if (fIncludeQuotes && (cParm != 0))
                  fIncludeQuotes = FALSE;

              /* Get the parameter. */
              if (!(lpCmd = GetOneParameter(lpCmdStart, lpCmd, lpW + (++cParm), fIncludeQuotes)))
                  goto GDEErrExit;

              /* HACK: Did GOP replace a ')' with a NULL? */
              if (!*lpCmd)
                  break;

              /* Find the next one or ')' */
              lpCmd = SkipWhite(lpCmd);
            }

          // Skip closing bracket.
          lpCmd++;

          /* Skip the terminating stuff. */
          lpCmd = SkipWhite(lpCmd);
        }

      /* Set the count of parameters and then skip the parameters. */
      *lpW++ = cParm;
      lpW += cParm;

      /* We found one more command. */
      cCmd++;

      /* Commands must be in square brackets. */
      if (*lpCmd != TEXT(']'))
          goto GDEErrExit;
      lpCmd++;
    }

GDEExit:
  /* Terminate the command list with -1. */
  *lpW = (UINT)-1;

  return lpRet;

GDEErrExit:
  GlobalFree(lpW);
  return(0);
}

//---------------------------------------------------------------------------
// lpszBuf is the dde command with NULLs between the commands and the
// arguments.
// *lpwCmd is the number of paramaters.
// *(lpwCmd+n) are offsets to those paramters in lpszBuf.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Make sure the path name only uses valid characters
// BUGBUG:: there is a similar function in shelldll\path.c
BOOL _IsValidFileNameChar(TUCHAR ch)
{
    switch (ch) {
    case TEXT(';'):       // terminator
    case TEXT(','):       // terminator
    case TEXT('|'):       // pipe
    case TEXT('>'):       // redir
    case TEXT('<'):       // redir
    case TEXT('"'):       // quote
    case TEXT('?'):       // wc           we only do wilds here because they're
    case TEXT('*'):       // wc           legal for qualifypath
    case TEXT('\\'):      // path separator
    case TEXT(':'):       // drive colon
    case TEXT('/'):       // path sep
        return FALSE;
    }

    // Can not be a control char...
    return ch >= TEXT(' ');
}

//---------------------------------------------------------------------------
// Make a long group name valid on an 8.3 machine.
// This assumes the name is already a valid LFN.
void _ShortenGroupName(LPTSTR lpName)
{
    LPTSTR pCh = lpName;

    ASSERT(lpName);

    while (*pCh)
    {
        // Spaces?
        if (*pCh == TEXT(' '))
            *pCh = TEXT('_');
        // Next
        pCh = CharNext(pCh);
        // Limit to 8 chars.
        if (pCh-lpName >= 8)
            break;
    }
    // Null term.
    *pCh = TEXT('\0');
}

//---------------------------------------------------------------------------
// This function will convert the name into a valid file name
void FileName_MakeLegal(LPTSTR lpName)
{
    LPTSTR lpT;

    ASSERT(lpName);

    for (lpT = lpName; *lpT != TEXT('\0'); lpT = CharNext(lpT))
    {
        if (!_IsValidFileNameChar(*lpT))
            *lpT = TEXT('_');        // Don't Allow invalid chars in names
    }

    // Quick check to see if we support long group names.
    if (!g_LFNGroups)
    {
        // Nope, shorten it.
        _ShortenGroupName(lpName);
    }
}

//---------------------------------------------------------------------------
// Given a ptr to a path and a ptr to the start of its filename componenent
// make the filename legal and tack it on the end of the path.
void GenerateGroupName(LPTSTR lpszPath, LPTSTR lpszName)
{
    ASSERT(lpszPath);
    ASSERT(lpszName);

    // Deal with ":" and "\" in the group name before trying to
    // qualify it.
    FileName_MakeLegal(lpszName);
    PathAppend(lpszPath, lpszName);
    PathQualify(lpszPath);
}

//---------------------------------------------------------------------------
// Simple function used by AddItem, DeleteItem, ReplaceItem to make sure
// that our group name has been setup properly.
void _CheckForCurrentGroup(PDDECONV pddec)
{
    // Need a group - if nothing is specified then we default to using
    // the last group name that someone either created or viewed.
    //
    if (!pddec->szGroup[0])
    {
        // We will use the last context that was set...
        // Note: after that point, we will not track the new create
        // groups and the like of other contexts.
        ENTERCRITICAL;
        if (g_pszLastGroupName != NULL) {
            lstrcpy(pddec->szGroup, g_pszLastGroupName);
        } else {
            CABINETSTATE cs;
            if (IsUserAnAdmin() &&
                    (ReadCabinetState(&cs, SIZEOF(cs)), cs.fAdminsCreateCommonGroups)) {
                SHGetSpecialFolderPath(NULL, pddec->szGroup, CSIDL_COMMON_PROGRAMS, TRUE);
            } else {
                SHGetSpecialFolderPath(NULL, pddec->szGroup, CSIDL_PROGRAMS, TRUE);
            }
        }
        LEAVECRITICAL;
    }
}

//---------------------------------------------------------------------------
// For those apps that do not setup their context for where to
// add items during their processing we need to keep the path
// of the last group that was created (in g_pszLastGroupName).
void _KeepLastGroup(LPCTSTR lpszGroup)
{
    LPTSTR lpGroup;

    ENTERCRITICAL;

    lpGroup = (LPTSTR)_LocalReAlloc(g_pszLastGroupName, (lstrlen(lpszGroup) + 1) * SIZEOF(TCHAR), LMEM_MOVEABLE|LMEM_ZEROINIT);
    if (lpGroup != NULL) {
        g_pszLastGroupName = lpGroup;
        lstrcpy(g_pszLastGroupName, lpszGroup);
    }

    LEAVECRITICAL;
}

//---------------------------------------------------------------------------
// NB HACK - Lots of setup apps dot lots of Create/Groups and as we're
// more async now we end up showing lots of identical group windows.
// Also, even the time delay in determining that the group is already
// open can cause some setup apps to get confused.
// So, to stop this happening we keep track of the last group created
// or shown and skip the Cabinet_OpenFolder if it's the same guy and we're
// within a X second timeout limit.
BOOL _SameLastGroup(LPCTSTR lpszGroup)
{
    static DWORD dwTimeOut = 0;
    BOOL fRet = FALSE;
    
    if (lpszGroup && g_pszLastGroupName)
    {
        // Too soon?
        if (GetTickCount() - dwTimeOut < 30*1000)
        {
            LPTSTR pszName1 = PathFindFileName(lpszGroup);
            LPTSTR pszName2 = PathFindFileName(g_pszLastGroupName);
            
            // Yep, same group as last time?
            ENTERCRITICAL;
            if (lstrcmpi(pszName1, pszName2) == 0)
            {
                // Yep.
                fRet = TRUE;
            }
            LEAVECRITICAL;
        }
    }
    
    dwTimeOut = GetTickCount();
    return fRet;
}

//---------------------------------------------------------------------------
// Map the group name to a proper path taking care of the startup group and
// app hacks on the way.
void GetGroupPath(LPCTSTR pszName, LPTSTR pszPath, DWORD dwFlags, INT iCommonGroup)
{
    TCHAR  szGroup[MAX_PATH];
    BOOL   bCommonGroup;
    BOOL   bFindPersonalGroup = FALSE;

    if (!pszName)
        return;

    //
    // Determine which type of group to create.
    //
    if (IsUserAnAdmin()) {
        if (iCommonGroup == 0) {
            bCommonGroup = FALSE;

        } else if (iCommonGroup == 1) {
            bCommonGroup = TRUE;

        } else {
            //
            // Administrators get common groups created by default
            // when the setup application doesn't specificly state
            // what kind of group to create.  This feature can be
            // turned off in the cabinet state flags.
            //
            CABINETSTATE cs;
            ReadCabinetState(&cs, SIZEOF(cs));
            if (cs.fAdminsCreateCommonGroups) {
                bFindPersonalGroup = TRUE;
                bCommonGroup = FALSE;   // This might get turned on later
                                        // if find is unsuccessful
            } else {
                bCommonGroup = FALSE;
            }
        }
    } else {
        //
        // Regular users can't create common group items.
        //
        bCommonGroup = FALSE;
    }

    // Handle NULL groups for certain apps and map Startup (non-localised)
    // to the startup group.
    if (((dwFlags & DDECONV_NULL_FOR_STARTUP) && !*pszName)
        || (lstrcmpi(pszName, c_szStartUp) == 0))
    {
        if (bCommonGroup) {
            SHGetSpecialFolderPath(NULL, pszPath, CSIDL_COMMON_STARTUP, TRUE);
        } else {
            SHGetSpecialFolderPath(NULL, pszPath, CSIDL_STARTUP, TRUE);
        }
    }
    else
    {
        // Hack for Media Recorder.
        if (dwFlags & DDECONV_MAP_MEDIA_RECORDER)
        {
            if (lstrcmpi(pszName, c_szMediaRecOld) == 0)
                lstrcpy(szGroup, c_szMediaRecNew);
            else
                lstrcpy(szGroup, pszName);
        }
        else
        {
            // Map group name for FE characters which have identical
            // twins in both DBCS/SBCS. Stolen from grpconv's similar
            // function.

            MapGroupName(pszName, szGroup, ARRAYSIZE(szGroup));
        }

        // Possibly find existing group
        if (bFindPersonalGroup)
        {
            SHGetSpecialFolderPath(NULL, pszPath, CSIDL_PROGRAMS, TRUE);
            GenerateGroupName(pszPath, szGroup);
            if (PathFileExists(pszPath))
                return;
            bCommonGroup = TRUE;
        }

        // Get the first bit of the path for this group.
        if (bCommonGroup) {
            SHGetSpecialFolderPath(NULL, pszPath, CSIDL_COMMON_PROGRAMS, TRUE);
        } else {
            SHGetSpecialFolderPath(NULL, pszPath, CSIDL_PROGRAMS, TRUE);
        }

        GenerateGroupName(pszPath, szGroup);
    }
}

BOOL IsParameterANumber(LPTSTR lp)
{
  while (*lp) {
      if (*lp < TEXT('0') || *lp > TEXT('9'))
          return(FALSE);
      lp++;
  }
  return(TRUE);
}

//---------------------------------------------------------------------------
// [ CreateGroup ( Group Name [, Group File] [,Common Flag] ) ]
// REVIEW UNDONE Allow the use of a group file to be specified.
BOOL DDE_CreateGroup(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    BOOL bRet;
    INT iCommonGroup = -1;
    TCHAR szGroup[MAX_PATH];     // Group pathname

    DBG_ENTER(FTF_DDE, DDE_CreateGroup);

    if ((*lpwCmd > 3) || (*lpwCmd == 0))
    {
        bRet = FALSE;
        goto Leave;
    }

    if (*lpwCmd >= 2) {

        //
        // Need to check for common group flag
        //
        if (*lpwCmd == 3) {
            if (lpszBuf[*(lpwCmd + 3)] == TEXT('1')) {
                iCommonGroup = 1;
            } else {
                iCommonGroup = 0;
            }
        } else if (*lpwCmd == 2 && IsParameterANumber(lpszBuf + *(lpwCmd+2))) {
            if (lpszBuf[*(lpwCmd + 2)] == TEXT('1')) {
                iCommonGroup = 1;
            } else {
                iCommonGroup = 0;
            }
        }
    }

    lpwCmd++;

    GetGroupPath(&lpszBuf[*lpwCmd], szGroup, pddec->dwFlags, iCommonGroup);

    TraceMsg(TF_DDE, "Create Group %s", (LPTSTR) szGroup);

    // Stop creating lots of identical folders.
    if (!_SameLastGroup(szGroup))
    {
        lstrcpy(pddec->szGroup,szGroup);    // Now working on this group...

        // If it doesn't exist then create it.
        if (!PathFileExists(pddec->szGroup))
        {
            if (CreateDirectory(pddec->szGroup, NULL))
            {
                SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, pddec->szGroup, NULL);
            }
            else
            {
                bRet = FALSE;
                goto Leave;
            }
        }

        // Show it.
        OpenGroup(pddec->szGroup, SW_NORMAL);
        _KeepLastGroup(pddec->szGroup);
    }
    else
    {
        TraceMsg(TF_DDE, "Ignoring duplicate CreateGroup");
    }

    bRet = TRUE;

Leave:
    DBG_EXIT_BOOL(FTF_DDE, DDE_CreateGroup, bRet);

    return bRet;
}

//---------------------------------------------------------------------------
// REVIEW HACK - Don't just caste, call GetConvInfo() to get this.
#define _GetDDEWindow(hconv)    ((HWND)hconv)

//---------------------------------------------------------------------------
// Return the hwnd of the guy we're talking too.
HWND _GetDDEPartnerWindow(HCONV hconv)
{
        CONVINFO ci;

        ci.hwndPartner = NULL;
        ci.cb = SIZEOF(ci);
        DdeQueryConvInfo(hconv, QID_SYNC, &ci);
        return ci.hwndPartner;
}

//---------------------------------------------------------------------------
// [ ShowGroup (group_name, wShowParm) ]
// REVIEW This sets the default group - not neccessarily what progman
// used to do but probably close enough.
BOOL DDE_ShowGroup(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    BOOL bRet;
    int nShowCmd;
    BOOL fUseStartup = FALSE;
    TCHAR szGroup[MAX_PATH];
    INT iCommonGroup = -1;

    DBG_ENTER(FTF_DDE, DDE_ShowGroup);

    if (*lpwCmd < 2 || *lpwCmd > 3)
    {
        bRet = FALSE;
        goto Leave;
    }

    if (*lpwCmd == 3) {

        //
        // Need to check for common group flag
        //

        if (lpszBuf[*(lpwCmd + 3)] == TEXT('1')) {
            iCommonGroup = 1;
        } else {
            iCommonGroup = 0;
        }
    }

    lpwCmd++;

    GetGroupPath(&lpszBuf[*lpwCmd], szGroup, pddec->dwFlags, iCommonGroup);

    // NB VJE-r setup passes an invalid group name to ShowGroup command.
    // Use szGroup and check it before copying it to pddec->szGroup.
    if (!PathFileExists(szGroup))
    {
        bRet = FALSE;
        goto Leave;
    }

    // Get the show cmd.
    lpwCmd++;
    nShowCmd = StrToInt(&lpszBuf[*lpwCmd]);
    TraceMsg(TF_DDE, "Showing %s (%d)", (LPTSTR)szGroup, nShowCmd);

    // Stop lots of cabinet windows from appearing without slowing down the dde
    // conversation if we're just doing a ShowNormal/ShowNA of a group we probably
    // just created.
    switch (nShowCmd)
    {
        case SW_SHOWNORMAL:
        case SW_SHOWNOACTIVATE:
        case SW_SHOW:
        case SW_SHOWNA:
        {
            if (_SameLastGroup(szGroup))
            {
                TraceMsg(TF_DDE, "Ignoring duplicate ShowGroup.");
                bRet = TRUE;
                goto Leave;
            }
            break;
        }
        case SW_SHOWMINNOACTIVE:
        {
                nShowCmd = SW_SHOWMINIMIZED;
                break;
        }
    }

    // It's OK to use the new group.
    lstrcpy(pddec->szGroup, szGroup);

    // Else
    _KeepLastGroup(pddec->szGroup);

    OpenGroup(pddec->szGroup, nShowCmd);

    bRet = TRUE;

Leave:
    DBG_EXIT_BOOL(FTF_DDE, DDE_ShowGroup, bRet);

    return bRet;
}


//---------------------------------------------------------------------------
// [ DeleteGroup (group_name) ]
BOOL DDE_DeleteGroup(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    BOOL bRet;
    TCHAR  szGroupName[MAX_PATH];
    INT iCommonGroup = -1;

    DBG_ENTER(FTF_DDE, DDE_DeleteGroup);

    if (*lpwCmd < 1 || *lpwCmd > 3)
    {
        bRet = FALSE;
        goto Leave;
    }

    if (*lpwCmd == 2) {
        //
        // Need to check for common group flag
        //

        if (lpszBuf[*(lpwCmd + 2)] == TEXT('1')) {
            iCommonGroup = 1;
        } else {
            iCommonGroup = 0;
        }
    }

    lpwCmd++;

    GetGroupPath(&lpszBuf[*lpwCmd], szGroupName, pddec->dwFlags, iCommonGroup);

    if (!PathFileExists(szGroupName))
    {
        bRet = FALSE;
        goto Leave;
    }

    szGroupName[lstrlen(szGroupName) + 1] = TEXT('\0');     // double NULL terminate

    // Now simply try to delete the group!
    // Use copy engine that will actually move to trash can...
    {
        SHFILEOPSTRUCT sFileOp =
        {
            NULL,
            FO_DELETE,
            szGroupName,
            NULL,
            FOF_RENAMEONCOLLISION | FOF_NOCONFIRMATION | FOF_SILENT,
        } ;

        TraceMsg(TF_DDE, "Deleting group %s.", szGroupName);

        SHFileOperation(&sFileOp);

        TraceMsg(TF_DDE, "Finished deleting");

    }

    // Clear the last group flag so that Create+Delete+Create
    // does the right thing.
    _KeepLastGroup(c_szNULL);
    bRet = TRUE;

Leave:
    DBG_EXIT_BOOL(FTF_DDE, DDE_DeleteGroup, bRet);

    return bRet;
}

//---------------------------------------------------------------------------
// Take the filename part of a path, copy it into lpszName and the pretty it
// up so it can be used as a link name.
void BuildDefaultName(LPTSTR lpszName, LPCTSTR lpszPath)
{
    LPTSTR lpszFilename;

    lpszFilename = PathFindFileName(lpszPath);
    lstrcpy(lpszName, lpszFilename);
    // NB Path remove extension can only remove extensions from filenames
    // not paths.
    PathRemoveExtension(lpszName);
    CharLower(lpszName);
    CharUpperBuff(lpszName, 1);
}

//----------------------------------------------------------------------------
// Return TRUE if the window belongs to a 32bit or a Win4.0 app.
// NB We can't just check if it's a 32bit window
// since many apps use 16bit ddeml windows to communicate with the shell
BOOL Window_IsWin32OrWin4(HWND hwnd)
{
    if (g_fRunningOnNT)
    {
        // BUGBUG - BobDay - Shouldn't there be a compat. way to do this?
        if ( LOWORD(GetWindowLong(hwnd,GWL_HINSTANCE)) == 0 ) {
            // 32-bit window
            return TRUE;
        }
        // BUGBUG - BobDay - Don't know about whether Win31 or Win40 yet?
        return FALSE;
    }
    else
    {
        DWORD idProcess;
        GetWindowThreadProcessId(hwnd, &idProcess);
        if (!(GetProcessDword(idProcess, GPD_FLAGS) & GPF_WIN16_PROCESS) ||
            (GetProcessDword(idProcess, GPD_EXP_WINVER) >= 0x0400))
        {
            TraceMsg(TF_DDE, "Win32 app (hwnd %x) handling DDE cmd.", hwnd);
            return TRUE;
        }

        TraceMsg(TF_DDE, "Win16 app (hwnd %x) handling DDE cmd.", hwnd);
        return FALSE;
    }
}


//---------------------------------------------------------------------------
BOOL HConv_PartnerIsLFNAware(HCONV hconv)
{
    HWND hwndPartner = _GetDDEPartnerWindow(hconv);

    // If this is being forwared by the desktop then assume the app isn't
    // LFN aware.
    if (IsDesktopWindow(hwndPartner))
        return FALSE;
    else
        return Window_IsWin32OrWin4(hwndPartner);
}

//---------------------------------------------------------------------------
BOOL PrivatePathStripToRoot(LPTSTR szRoot)
{
    while(!PathIsRoot(szRoot))
    {
        if (!PathRemoveFileSpec(szRoot))
        {
            // If we didn't strip anything off,
            // must be current drive
            return(FALSE);
        }
    }

    return(TRUE);
}

//---------------------------------------------------------------------------
BOOL Net_ConnectDrive(LPCTSTR pszShare, TCHAR *pchDrive)
{
    DWORD err;
    NETRESOURCE nr;
    TCHAR szAccessName[MAX_PATH];
    ULONG cbAccessName = SIZEOF(szAccessName);
    DWORD dwResult;

    // Connect to the given share and return the drive that it's on.
    nr.lpRemoteName = (LPTSTR)pszShare;
    nr.lpLocalName = NULL;
    nr.lpProvider = NULL;
    nr.dwType = RESOURCETYPE_DISK;
    err = WNetUseConnection(NULL, &nr, NULL, NULL, CONNECT_TEMPORARY | CONNECT_REDIRECT,
        szAccessName, &cbAccessName, &dwResult);
    if (err == WN_SUCCESS)
    {
        TraceMsg(TF_DDE, "Net_ConnextDrive: %s %s %x", pszShare, szAccessName, dwResult);
        if (pchDrive)
            *pchDrive = szAccessName[0];
        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
BOOL Net_DisconnectDrive(TCHAR chDrive)
{
    TCHAR szDrive[3];

    // Disconnect the given drive from it's share.
    szDrive[0] = chDrive;
    szDrive[1] = TEXT(':');
    szDrive[2] = TEXT('\0');
    return WNetCancelConnection2(szDrive, 0, FALSE) == WN_SUCCESS;
}

//---------------------------------------------------------------------------
// Convert (\\foo\bar\some\path, X) to (X:\some\path)
void Path_ChangeUNCToDrive(LPTSTR pszPath, TCHAR chDrive)
{
    TCHAR szPath[MAX_PATH];
    LPTSTR pszSpec;

    lstrcpy(szPath, pszPath);
    PrivatePathStripToRoot(szPath);
    pszPath[0] = chDrive;
    pszPath[1] = TEXT(':');
    pszPath[2] = TEXT('\\');
    pszSpec = pszPath + lstrlen(szPath) + 1;
    if (*pszSpec)
        lstrcpy(&pszPath[3],pszSpec);
}

//---------------------------------------------------------------------------
LPITEMIDLIST Pidl_CreateUsingAppPaths(LPCTSTR pszApp)
{
    TCHAR sz[MAX_PATH];
    long cb = SIZEOF(sz);

    TraceMsg(TF_DDE, "Trying app paths...");

    lstrcpy(sz, REGSTR_PATH_APPPATHS);
    PathAppend(sz, pszApp);
    if (RegQueryValue(HKEY_LOCAL_MACHINE, sz, sz, &cb) == ERROR_SUCCESS)
    {
        return ILCreateFromPath(sz);
    }
    return NULL;
}

//---------------------------------------------------------------------------
// [ AddItem (command,name,icopath,index,pointx,pointy, defdir,hotkey,fminimize,fsepvdm) ]
// This adds things to the current group ie what ever's currently in
// the conversations szGroup string
BOOL DDE_AddItem(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    BOOL bRet;
    UINT nParams;

    TCHAR szTmp[MAX_PATH];
    TCHAR szName[MAX_PATH];
    TCHAR szCL[MAX_PATH];
    TCHAR szShare[MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    LPTSTR lpszArgs;
    UINT iIcon;
    int nShowCmd;
    BOOL fIconPath = FALSE;
    LPITEMIDLIST pidl;
    IPersistFile *ppf;
    LPTSTR dirs[2];
    TCHAR chDrive;

    DBG_ENTER(FTF_DDE, DDE_AddItem);

    // Make sure group name is setup
    _CheckForCurrentGroup(pddec);

    // Only certain param combinations are allowed.
    nParams = *lpwCmd;
    if (nParams < 1 || nParams == 5 || nParams > 10)
    {
        bRet = FALSE;
        goto Leave;
    }

    // There must at least be a command.
    lpwCmd++;
    lstrcpy(szCL, &lpszBuf[*lpwCmd]);
    if (!*szCL)
    {
        bRet = FALSE;
        goto Leave;
    }

#ifdef DEBUG
    // Separate the args.
    if (HConv_PartnerIsLFNAware(pddec->hconv))
    {
        // Quotes will have been left in the string.
        TraceMsg(TF_DDE, "Partner is LFN aware.");
    }
    else
    {
        // Quotes will have been removed from the string.
        TraceMsg(TF_DDE, "Partner is not LFN aware.");
    }
#endif

    // We initialize the IDLIst of this shell link to NULL, such that
    // when we set it later it won't screw around with the working directory
    // we may have set.
    pddec->psl->SetIDList(NULL);

    // NB - This can deal with quoted spaces.
    PathRemoveBlanks(szCL);
    lpszArgs = PathGetArgs(szCL);
    if (*lpszArgs)
        *(lpszArgs-1) = TEXT('\0');

    // Win32/Win4.0 setup apps are allowed to use paths with (quoted)
    // spaces in them so we may need to remove them now.
    PathUnquoteSpaces(szCL);

    pddec->psl->SetArguments(lpszArgs);

    // Special case UNC paths.
    if ((pddec->dwFlags & DDECONV_NO_UNC) && PathIsUNC(szCL))
    {
        // CL is a UNC but we know this app can't handle UNC's, we'll need to
        // fake up a drive for it.
        TraceMsg(TF_DDE, "Mapping UNC to drive.");

        // Get the server/share name.
        lstrcpy(szShare, szCL);
        PrivatePathStripToRoot(szShare);
        // Do we already have a cached connection to this server share?
        if (lstrcmpi(szShare, pddec->szShare) == 0)
        {
            // Yes
            TraceMsg(TF_DDE, "Using cached connection.");
            // Mangle the path to use the drive instead of the UNC.
            Path_ChangeUNCToDrive(szCL, pddec->chDrive);
        }
        else
        {
            // No
            TraceMsg(TF_DDE, "Creating new connection.");
            // Make a connection.
            if (Net_ConnectDrive(szShare, &chDrive))
            {
                // Store the server/share.
                lstrcpy(pddec->szShare, szShare);
                // Store the drive.
                pddec->chDrive = chDrive;
                // Set the DDECONV_FORCED_CONNECTION flag so we can cleanup later.
                pddec->dwFlags |= DDECONV_FORCED_CONNECTION;
                // Mangle the path to use the drive instead of the UNC.
                Path_ChangeUNCToDrive(szCL, pddec->chDrive);
            }
            else
            {
                TraceMsg(TF_DDE, "Can't create connection.");
            }
        }
        TraceMsg(TF_DDE, "CL changed to %s.", szCL);
    }

    // Is there a name?
    szName[0] = TEXT('\0');
    if (nParams > 1)
    {
        // Yep,
        lpwCmd++;
        lstrcpy(szName, &lpszBuf[*lpwCmd]);
    }

    // Make absolutely sure we have a name.
    if (!szName[0])
        BuildDefaultName(szName, szCL);

    // Make it legal.
    FileName_MakeLegal(szName);

    // NB Skip setting the CL until we get the WD, we may need
    // it.

    // Deal with the icon path.
    if (nParams > 2)
    {
        lpwCmd++;
        lstrcpy(szTmp, &lpszBuf[*lpwCmd]);
        if (*szTmp)
        {
            // Some idiots try to put arguments on the icon path line.
            lpszArgs = PathGetArgs(szTmp);
            if (*lpszArgs)
                *(lpszArgs-1) = TEXT('\0');
            // Save it.
            fIconPath = TRUE;
        }
    }
    else
    {
        szTmp[0] = TEXT('\0');
    }

    iIcon = 0;
    // Icon index
    if (nParams > 3)
    {
        lpwCmd++;
        // They must have had an icon path for this to make sense.
        if (fIconPath)
        {
            iIcon = StrToInt(&lpszBuf[*lpwCmd]);
            // REVIEW Don't support icon indexs > 666 hack anymore.
            // It used to mark this item as the selected one. This
            // won't work in the new shell.
            if (iIcon >= 666)
            {
                iIcon -= 666;
            }
        }
    }

    pddec->psl->SetIconLocation(szTmp, iIcon);

    // Get the point :-)
    // REVIEW UNDONE ForcePt stuff for ReplaceItem.
    if (nParams > 4)
    {
        POINT ptIcon;
        lpwCmd++;
        ptIcon.x = StrToInt(&lpszBuf[*lpwCmd]);
        lpwCmd++;
        ptIcon.y = StrToInt(&lpszBuf[*lpwCmd]);
    }

    // The working dir. Do we need a default one?
    if (nParams > 6)
    {
        lpwCmd++;
        lstrcpy(szTmp, &lpszBuf[*lpwCmd]);
    }
    else
    {
        szTmp[0] = TEXT('\0');
    }

    // If we don't have a default directory, try to derive one from the
    // given CL (unless it's a UNC).
    if (!szTmp[0])
    {
        // Use the command for this.
        // REVIEW UNDONE It would be better fo the WD and the IP to be
        // moveable like the CL.
        lstrcpy(szTmp, szCL);
        // Remove the last component.
        PathRemoveFileSpec(szTmp);
    }

    // Don't use UNC paths.
    if (PathIsUNC(szTmp))
        pddec->psl->SetWorkingDirectory(c_szNULL);
    else
        pddec->psl->SetWorkingDirectory(szTmp);

    // Now we have a WD we can deal with the command line better.
    dirs[0] = szTmp;
    dirs[1] = NULL;
    PathResolve(szCL, (LPCTSTR*)dirs, PRF_TRYPROGRAMEXTENSIONS | PRF_VERIFYEXISTS);

    pidl = ILCreateFromPath(szCL);
    if (!pidl)
    {
        TraceMsg(TF_DDE, "Can't create IL from path. Using simple idlist.");
        // REVIEW UNDONE Check that the file doesn't exist.
        pidl = SHSimpleIDListFromPath(szCL);
        // The Family Circle Cookbook tries to create a shortcut
        // to wordpad.exe but since that's now not on the path
        // we can't find it. The fix is to do what ShellExec does
        // and check the App Paths section of the registry.
        if (!pidl)
        {
            pidl = Pidl_CreateUsingAppPaths(szCL);
        }
    }

    if (pidl)
    {
        pddec->psl->SetIDList(pidl);
        ILFree(pidl);
    }
    else
    {
        TraceMsg(TF_DDE, "Can't create idlist for %s", szCL);

        if (pddec->dwFlags & DDECONV_ALLOW_INVALID_CL)
            bRet = TRUE;
        else
            bRet = FALSE;

        goto Leave;
    }

    // Hotkey.
    if (nParams > 7)
    {
        WORD wHotkey;
        lpwCmd++;
        wHotkey = (WORD)StrToInt(&lpszBuf[*lpwCmd]);
        pddec->psl->SetHotkey(wHotkey);
    }
    else
    {
        pddec->psl->SetHotkey(0);
    }

    // Show command
    if (nParams > 8)
    {
        lpwCmd++;
        if (StrToInt(&lpszBuf[*lpwCmd]))
            nShowCmd = SW_SHOWMINNOACTIVE;
        else
            nShowCmd = SW_SHOWNORMAL;
        pddec->psl->SetShowCmd(nShowCmd);
    }
    else
    {
        pddec->psl->SetShowCmd(SW_SHOWNORMAL);
    }
    if (nParams > 9)
    {
        lpwCmd++;
        if (StrToInt(&lpszBuf[*lpwCmd]))
        {
            // BUGBUG - BobDay - Handle Setup of Seperate VDM flag!
            // pddec->psl->SetSeperateVDM(pddec->psl, wHotkey);
        }
    }

    pddec->fDirty = TRUE;

    PathCombine(szTmp, pddec->szGroup, szName);
    lstrcat(szTmp, c_szDotLnk);
    PathQualify(szTmp);

    // We need to handle link duplication problems on SFN drives.
    if (!IsLFNDrive(szTmp) && PathFileExists(szTmp))
        PathYetAnotherMakeUniqueName(szTmp, szTmp, NULL, NULL);

    pddec->psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

    StrToOleStrN(wszPath, ARRAYSIZE(wszPath), szTmp, -1);
    ppf->Save(wszPath, TRUE);
    ppf->Release();
    // REVIEW - Sometimes links don't get the right icons. The theory is that
    // a folder in the process of opening (due to a CreateGroup) will pick
    // up a partially written .lnk file. When the link is finally complete
    // we send a SHCNE_CREATE but this will get ignored if defview already has
    // the incomplete item. To hack around this we generate an update item
    // event to force an incomplete link to be re-read.
    TraceMsg(TF_DDE, "Generating events.");

    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szTmp, NULL);
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, szTmp, NULL);

    bRet = TRUE;

Leave:
    DBG_EXIT_BOOL(FTF_DDE, DDE_AddItem, bRet);

    return bRet;
}



//---------------------------------------------------------------------------
// [ DeleteItem (ItemName)]
// This deletes the specified item from a group
BOOL DDE_DeleteItem(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    BOOL bRet;
    TCHAR szPath[MAX_PATH];

    DBG_ENTER(FTF_DDE, DDE_DeleteItem);

    if (*lpwCmd != 1)
    {
        bRet = FALSE;
    }
    else
    {
        lpwCmd++;

        // Make sure group name is setup
        _CheckForCurrentGroup(pddec);

        pddec->fDirty = TRUE;

        // REVIEW IANEL Hardcoded .lnk and .pif
        PathCombine(szPath, pddec->szGroup, &lpszBuf[*lpwCmd]);
        lstrcat(szPath, c_szDotLnk);
        bRet = Win32DeleteFile(szPath);

        PathCombine(szPath, pddec->szGroup, &lpszBuf[*lpwCmd]);
        lstrcat(szPath, c_szDotPif);
        bRet |= DeleteFile(szPath);
    }

    DBG_EXIT_BOOL(FTF_DDE, DDE_DeleteItem, bRet);

    return bRet;
}

//---------------------------------------------------------------------------
// [ ExitProgman (bSaveGroups) ]
// REVIEW This doesn't do anything in the new shell. It's supported to stop
// old installations from barfing.
// REVIEW UNDONE - We should keep track of the groups we've shown
// and maybe hide them now.
BOOL DDE_ExitProgman(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    return TRUE;
}

//---------------------------------------------------------------------------
// [ Reload (???) ]
// REVIEW Just return FALSE
BOOL DDE_Reload(LPTSTR lpszBuf, UINT *lpwCmd, PDDECONV pddec)
{
    return FALSE;
}

//---------------------------------------------------------------------------
PDDECONV DDE_MapHConv(HCONV hconv)
{
    PDDECONV pddec;

    ENTERCRITICAL;
    for (pddec = g_pddecHead; pddec != NULL; pddec = pddec->pddecNext)
    {
        if (pddec->hconv == hconv)
            break;
    }
    LEAVECRITICAL;

    TraceMsg(TF_DDE, "Mapping %lx -> %lx", (DWORD)hconv, (DWORD)(LPVOID)pddec);
    return(pddec);
}

//
//  This data structure is used to return the error information from
// _GetPIDLFromDDEArgs to its caller. The caller may pop up a message
// box using this information. idMsg==0 indicates there is no such
// information.
//
typedef struct _SHDDEERR {      // sde (Software Design Engineer, Not!)
    UINT idMsg;
    TCHAR szParam[MAX_PATH];
} SHDDEERR, *PSHDDEERR;

//---------------------------------------------------------------------------
// Helper function to convert passed in command parameters into the
// appropriate Id list
LPITEMIDLIST _GetPIDLFromDDEArgs(UINT nArg, LPTSTR lpszBuf, UINT * lpwCmd, PSHDDEERR psde, LPCITEMIDLIST *ppidlGlobal)
{
    LPTSTR lpsz;
    LPITEMIDLIST pidl = NULL;
    
    // Switch from 0-based to 1-based 
    ++nArg;
    if (*lpwCmd < nArg)
    {
        TraceMsg(TF_DDE, "Invalid parameter count of %d", *lpwCmd);
        return NULL;
    }

    // Skip to the right argument
    lpwCmd += nArg;
    lpsz = &lpszBuf[*lpwCmd];

    TraceMsg(TF_DDE, "Converting \"%s\" to pidl", lpsz);
    // REVIEW: all associations will go through here.  this
    // is probably not what we want for normal cmd line type operations

    // A colon at the begining of the path means that this is a pointer
    // to an idl otherwise it's a regular path.
    if (lpsz[0] == TEXT(':'))
    {
        HANDLE hMem;
        DWORD  dwProcId;
        LPTSTR pszNextColon;

        // Convert the string into a pointer.

        hMem = (HANDLE)StrToLong((LPTSTR)(lpsz+1));
        pszNextColon = StrChr(lpsz+1,TEXT(':'));
        if (pszNextColon)
        {
            LPITEMIDLIST pidlShared;

            dwProcId = (DWORD)StrToLong(pszNextColon+1);
            pidlShared = (LPITEMIDLIST)SHLockShared(hMem,dwProcId);
            if (pidlShared && !IsBadReadPtr(pidlShared,1))
            {
                pidl = ILClone(pidlShared);
                SHUnlockShared(pidlShared);
            }
            SHFreeShared(hMem,dwProcId);
        }
        else if ( hMem && !IsBadReadPtr( hMem, sizeof(WORD)))
        {
            // this is likely to be browser only mode on win95 with the old pidl arguments which is
            // going to be in shared memory.... (must be cloned into local memory)...
            pidl = ILClone((LPITEMIDLIST) hMem);

            // this will get freed if we succeed.
            ASSERT( ppidlGlobal );
            *ppidlGlobal = (LPITEMIDLIST) hMem;
        }

        return pidl;
    }
    else
    {
        TCHAR tszQual[MAX_PATH];

        // We must copy to a temp buffer because the PathQualify may
        // result in a string longer than our input buffer and faulting
        // seems like a bad way of handling that situation.
        lstrcpyn(tszQual, lpsz, ARRAYSIZE(tszQual));
        lpsz = tszQual;

        // Is this a URL?
        if ( !PathIsURL(lpsz) )
            {
            // No; qualify it
            PathQualify(lpsz);
            }
        pidl = ILCreateFromPath(lpsz);
        if (pidl==NULL && psde) {
            psde->idMsg = IDS_CANTFINDDIR;
            lstrcpyn(psde->szParam, lpsz, ARRAYSIZE(psde->szParam));
        }
        return pidl;
    }
}


LPITEMIDLIST GetPIDLFromDDEArgs(LPTSTR lpszBuf, UINT * lpwCmd, PSHDDEERR psde, LPCITEMIDLIST * ppidlGlobal)
{
    LPITEMIDLIST pidl = _GetPIDLFromDDEArgs(1, lpszBuf, lpwCmd, psde, ppidlGlobal);
    if (!pidl)
    {
        pidl = _GetPIDLFromDDEArgs(0, lpszBuf, lpwCmd, psde, ppidlGlobal);
    }
    
    return pidl;
}

void _FlagsToParams(UINT uFlags, LPTSTR pszParams)
{
    if (uFlags & COF_EXPLORE)
        lstrcat(pszParams, TEXT(",/E"));

    if (uFlags & COF_SELECT)
        lstrcat(pszParams, TEXT(",/SELECT"));

    if (uFlags & COF_CREATENEWWINDOW)
        lstrcat(pszParams, TEXT(",/N"));

    if (uFlags & COF_USEOPENSETTINGS)
        lstrcat(pszParams, TEXT(",/S"));
}


// lpszBuf is a multi-string containing the various parameters.  

// lpwCmd is an array of indexes, where the first 
// element is the count of parameters, and each element
// after that is the starting offset into lpszBuf
// for the respective parameter.

BOOL DoDDE_ViewFolder(IShellBrowser* psb, HWND hwndParent, LPTSTR pszBuf, UINT *puCmd, BOOL fExplore, DWORD dwHotKey)
{
    // used to support the older win95 (browser only mode) Global passing of pidl pointers..
    LPITEMIDLIST pidlGlobal = NULL;
    LPITEMIDLIST pidl;
    int nCmdShow;
    SHDDEERR sde;
    BOOL fSuccess = TRUE;

    if (*puCmd != 3)
        return FALSE;   // Wrong number of arguments

    // The ShowWindow parameter is the third 
    nCmdShow = StrToLong(&pszBuf[*(puCmd+3)]);

    pidl = GetPIDLFromDDEArgs(pszBuf, puCmd, &sde, (LPCITEMIDLIST*)&pidlGlobal);
    if (pidl)
    {
        IETHREADPARAM *pfi = SHCreateIETHREADPARAM(NULL, nCmdShow, NULL, NULL);
        if (pfi)
        {
            pfi->hwndCaller = hwndParent;
            pfi->pidl = pidl;
            pfi->wHotkey = (UINT)dwHotKey;
            pfi->uFlags = COF_NORMAL;
            pfi->psbCaller = psb;
            if (psb)
                psb->AddRef();

            // Check for a :0 thing. Probably came from the command line.
            if (lstrcmpi(&pszBuf[*(puCmd+2)], TEXT(":0")) != 0)
            {
                // we need to use COF_USEOPENSETTINGS here.  this is where the open
                // from within cabinets happen.  if it's done via the command line
                // then it will esentially turn to COF_NORMAL because the a cabinet
                // window won't be the foreground window.

                pfi->uFlags = COF_USEOPENSETTINGS;
            }

            if (fExplore)
                pfi->uFlags |= COF_EXPLORE;

            // Whoever added REST_SEPARATEDESKTOPPROCESS to IE4 code FORGOT to add it to
            // IE4 shell32, so we need to manually check this one ourselves:
            //
            if (SHGetRestriction(NULL, L"Explorer", L"DesktopProcess"))
            {
                TCHAR szExplorer[MAX_PATH];
                TCHAR szCmdLine[MAX_PATH];
                SHELLEXECUTEINFO ei = { SIZEOF(ei), 0, NULL, NULL, szExplorer, szCmdLine, NULL, SW_SHOWNORMAL};

                DWORD dwProcess = GetCurrentProcessId();
                HANDLE hIdList = NULL;

                GetModuleFileName(NULL, szExplorer, ARRAYSIZE(szExplorer));

                fSuccess = TRUE;
                if (pfi->pidl)
                {
                    hIdList = SHAllocShared(pfi->pidl, ILGetSize(pfi->pidl), dwProcess);
                    wsprintf(szCmdLine, TEXT("/IDLIST,:%ld:%ld"), hIdList, dwProcess);
                    if (!hIdList)
                        fSuccess = FALSE;
                }
                else
                {
                    lstrcpy(szCmdLine, TEXT("/IDLIST,:0"));
                }

                _FlagsToParams(pfi->uFlags, szCmdLine + lstrlen(szCmdLine));

                if (fSuccess)
                {
                    fSuccess = ShellExecuteEx(&ei);
                }
                if (!fSuccess && hIdList)
                    SHFreeShared(hIdList, dwProcess);

                SHDestroyIETHREADPARAM(pfi);
            }
            else
            {
                //
                // Check if this is a folder or not. If not, we always create
                // a new window (even though we can browse in-place). If you
                // don't like it, please talk to ChristoB. (SatoNa)
                //
                //  I don't like it...  not for the explore case.
                // 
                if (!(pfi->uFlags & COF_EXPLORE))
                {
                    ULONG dwAttr = SFGAO_FOLDER;
                    if (SUCCEEDED(SHGetAttributesOf(pidl, &dwAttr)) && !(dwAttr & SFGAO_FOLDER))
                    {
                        pfi->uFlags |= COF_CREATENEWWINDOW;
                    }
                }
                fSuccess = SHOpenFolderWindow(pfi);
            }

            if (!fSuccess && (GetLastError() == ERROR_OUTOFMEMORY))
                SHAbortInvokeCommand();

            fSuccess = TRUE;    // If we fail we don't want people to try
                                // to create process as this will blow up...
        }
    }
    else
    {
        if (sde.idMsg) 
        {
            ShellMessageBox(MLGetHinst(), hwndParent,
                MAKEINTRESOURCE(sde.idMsg), MAKEINTRESOURCE(IDS_CABINET),
                MB_OK|MB_ICONHAND|MB_SETFOREGROUND, sde.szParam);
        }
        fSuccess = FALSE;
    }

    if (fSuccess && pidlGlobal)
    {
        // WIN95 Browser only mode, only free it if we succeeded
        ILGlobalFree(pidlGlobal);
    }
    
    return fSuccess;
}

//---------------------------------------------------------------------------
BOOL DDE_ViewFolder(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
    return DoDDE_ViewFolder(NULL, NULL, lpszBuf, puCmd, FALSE, 0);
}

//---------------------------------------------------------------------------
// BUGBUG ExploreFolder and ViewFolder do the same thing right now.
BOOL DDE_ExploreFolder(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
    return DoDDE_ViewFolder(NULL, NULL, lpszBuf, puCmd, TRUE, 0);
}


BOOL DDE_FindFolder(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlNetwork;
    LPITEMIDLIST pidlGlobal = NULL;

    pidl = GetPIDLFromDDEArgs(lpszBuf, puCmd, NULL, (LPCITEMIDLIST*)&pidlGlobal);

    if (pidl != NULL)
    {
        // A very large hack.  If the pidl is to the network neighborhood,
        // we do a FindComputer instead!
        pidlNetwork = SHCloneSpecialIDList(NULL, CSIDL_NETWORK, FALSE);
        if (pidlNetwork && ILIsEqual(pidlNetwork, pidl))
            SHFindComputer(pidl, NULL);
        else
            SHFindFiles(pidl, NULL);
        ILFree(pidlNetwork);

        ILFree(pidl);

        // WIN95 browser only mode, backwards compatibility
        if ( pidlGlobal )
            ILGlobalFree( pidlGlobal );
            
        return TRUE;
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// This processes the Find Folder command.  It is used for both for selecting
// Find on a folders context menu as well as opening a find file.
BOOL DDE_OpenFindFile(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlGlobal = NULL;
    
    pidl = GetPIDLFromDDEArgs(lpszBuf, puCmd, NULL, (LPCITEMIDLIST*)&pidlGlobal);

    if (pidl != NULL)
    {
        SHFindFiles(NULL, pidl);

        // WIN95 browser only mode, backwards compatibility
        if ( pidlGlobal )
            ILGlobalFree( pidlGlobal );
            
        return(TRUE);
    }
    else
        return(FALSE);
}

//---------------------------------------------------------------------------
BOOL DDE_ConfirmID(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
    BOOL bRet;

    DBG_ENTER(FTF_DDE, DDE_ConfirmID);

    bRet = (*puCmd == 0);

    DBG_EXIT_BOOL(FTF_DDE, DDE_ConfirmID, bRet);
    return bRet;
}


#ifdef DEBUG
//---------------------------------------------------------------------------
BOOL DDE_Beep(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
#if 0
    int i;

    for (i=*puCmd; i>=0; --i)
    {
        MessageBeep(0);
    }
    return(TRUE);
#else
    DWORD dwTime;

    dwTime = GetTickCount();
    TraceMsg(TF_DDE, "Spin...");
    // Spin. Spin. Spin. Huh Huh. Cool.
    while ((GetTickCount()-dwTime) < 4000)
    {
        // Spin.
    }
    TraceMsg(TF_DDE, "Spinning done.");
    return TRUE;
#endif
}
#endif

//---------------------------------------------------------------------------
BOOL DDE_ShellFile(LPTSTR lpszBuf, UINT * puCmd, PDDECONV pddec)
{
    LPITEMIDLIST pidlGlobal = NULL;
    LPITEMIDLIST pidl = GetPIDLFromDDEArgs(lpszBuf, puCmd, NULL, (LPCITEMIDLIST*)&pidlGlobal);
    if (pidl) {
        
        ShellExecCommandFile(pidl);
        
        ILFree(pidl);

        // WIN95 browser only mode, backwards compatibility
        if ( pidlGlobal )
            ILGlobalFree( pidlGlobal );
        
        return TRUE;
    }
    return FALSE;
}
//---------------------------------------------------------------------------
VOID CALLBACK TimerProc_RepeatAcks(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    HWND hwndPartner;

    if (g_hwndDde)
    {
        hwndPartner = _GetDDEPartnerWindow((HCONV)g_hwndDde);
        if (hwndPartner)
        {
            TraceMsg(TF_DDE, "DDE partner (%x) appears to be stuck - repeating Ack.", hwndPartner);
            PostMessage(hwndPartner, WM_DDE_ACK, (WPARAM)g_hwndDde, 0);
        }
    }
}

//---------------------------------------------------------------------------
HDDEDATA HandleDDEExecute(HDDEDATA hData, HCONV hconv)
{
    UINT *lpwCmd;
    UINT *lpwCmdTemp;
    UINT wCmd;
    PDDECONV pddec;
    HDDEDATA hddeRet = (HDDEDATA) DDE_FACK;
    UINT nErr;
    LPTSTR pszBuf;
    int cbData;

    DBG_ENTER(FTF_DDE, HandleDDEExecute);

    pddec = DDE_MapHConv(hconv);
    if (pddec == NULL)
    {
        // Could not find conversation
        hddeRet = HDDENULL;
        goto Leave;
    }

    if ((pddec->dwFlags & DDECONV_REPEAT_ACKS) && g_nTimer)
    {
        KillTimer(NULL, g_nTimer);
        g_nTimer = 0;
    }

    // NB Living Books Installer cats all their commands together
    // which requires about 300bytes - better just allocate it on
    // the fly.
    cbData = DdeGetData(hData, NULL, 0, 0L);
    if (cbData == 0)
    {
        // No data?
        hddeRet = HDDENULL;
        goto Leave;
    }

    pszBuf = (LPTSTR)LocalAlloc(LPTR, cbData);
    if (!pszBuf)
    {
        TraceMsg(TF_ERROR, "HandleDDEExecute: Can't allocate buffer (%d)", cbData);
        ASSERT(0);
        hddeRet = HDDENULL;
        goto Leave;
    }

    cbData = DdeGetData(hData, (LPBYTE)pszBuf, cbData, 0L);
    if (cbData == 0)
    {
        nErr = DdeGetLastError(g_dwDDEInst);
        TraceMsg(TF_ERROR, "HandleDDEExecute: Data invalid (%d).", nErr);
        LocalFree(pszBuf);
        ASSERT(0);
        hddeRet = HDDENULL;
        goto Leave;
    }

#ifdef UNICODE
    //
    // At this point, we may have ANSI data in pszBuf, but we need UNICODE!
    // !!!HACK alert!!! We're going to poke around in the string to see if it is
    // ansi or unicode.  We know that DDE execute commands should only
    // start with " " or "[", so we use that information...
    //
    // By the way, this only really happens when we get an out of order
    // WM_DDE_EXECUTE (app didn't send WM_DDE_INITIATE -- Computer Associate
    // apps like to do this when they setup).  Most of the time DDEML will
    // properly translate the data for us because they correctly determine
    // ANSI/UNICODE conversions from the WM_DDE_INITIATE message.

    if ((cbData>2) &&
        ((*((LPBYTE)pszBuf)==(BYTE)' ') || (*((LPBYTE)pszBuf)==(BYTE)'[')) &&
        (*((LPBYTE)pszBuf+1)!=0 ))
    {
        // We think that pszBuf is an ANSI string, so convert it
        LPTSTR pszUBuf;

        pszUBuf = (LPTSTR)LocalAlloc( LPTR, cbData * SIZEOF(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, (LPCSTR)pszBuf, -1, pszUBuf, cbData );
        LocalFree( pszBuf );
        pszBuf = pszUBuf;
    }
#endif // UNICODE

    if (pszBuf[0] == TEXT('\0'))
    {
        TraceMsg(TF_ERROR, "HandleDDEExecute: Empty execute command.");
        LocalFree(pszBuf);
        ASSERT(0);

        hddeRet = HDDENULL;
        goto Leave;
    }

    TraceMsg(TF_DDE, "Executing %s", pszBuf);

    lpwCmd = GetDDECommands(pszBuf, c_sDDECommands, HConv_PartnerIsLFNAware(hconv));
    if (!lpwCmd)
    {
        TraceMsg(TF_ERROR, "HandleDDEExecute: Invalid command.");
        LocalFree(pszBuf);
        ASSERT(0);

        // Make sure Discis installers get the Ack they're waiting for.
        if ((pddec->dwFlags & DDECONV_REPEAT_ACKS) && !g_nTimer)
        {
            // DebugBreak();
            g_nTimer = SetTimer(NULL, IDT_REPEAT_ACKS, 1000, (TIMERPROC)TimerProc_RepeatAcks);
        }

        hddeRet = HDDENULL;
        goto Leave;
    }

    // Store off lpwCmd so we can free the correect addr later
    lpwCmdTemp = lpwCmd;

    // Execute a command.
    while (*lpwCmd != (UINT)-1)
    {
        wCmd = *lpwCmd++;
        // Subtract 1 to account for the terminating NULL
        if (wCmd < ARRAYSIZE(c_sDDECommands)-1)
        {
            if (!c_sDDECommands[wCmd].lpfnCommand(pszBuf, lpwCmd, pddec))
            {
                hddeRet = HDDENULL;
            }
        }

        // Next command.
        lpwCmd += *lpwCmd + 1;
    }

    // Tidyup...
    GlobalFree(lpwCmdTemp);
    LocalFree(pszBuf);

    // Make sure Discis installers get the Ack they're waiting for.
    if ((pddec->dwFlags & DDECONV_REPEAT_ACKS) && !g_nTimer)
    {
        // DebugBreak();
        g_nTimer = SetTimer(NULL, IDT_REPEAT_ACKS, 1000, (TIMERPROC)TimerProc_RepeatAcks);
    }

Leave:
    DBG_EXIT_DWORD(FTF_DDE, HandleDDEExecute, hddeRet);

    return hddeRet;
}

//---------------------------------------------------------------------------
// NOTE: ANSI ONLY

// Used for filtering out hidden, . and .. stuff.

BOOL FindData_FileIsNormalA(WIN32_FIND_DATAA *lpfd)
{
    if ((lpfd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
        lstrcmpiA(lpfd->cFileName, c_szDesktopIniA) == 0)
    {
        return FALSE;
    }
    else if (lpfd->cFileName[0] == '.')
    {
        if ((lpfd->cFileName[1] == '\0') ||
            ((lpfd->cFileName[1] == '.') && (lpfd->cFileName[2] == '\0')))
        {
            return FALSE;
        }
    }
    return TRUE;
}

//---------------------------------------------------------------------------
HDDEDATA EnumGroups(HSZ hszItem)
{
    TCHAR szGroup[MAX_PATH];
#ifdef UNICODE        
    CHAR  szAGroup[MAX_PATH];
#endif        
    WIN32_FIND_DATAA fd;
    HANDLE hff;
    LPSTR lpszBuf = NULL;
    UINT cbBuf = 0;
    UINT cch;
    HDDEDATA hData;

    // Enumerate all the top level folders in the programs folder.
    SHGetSpecialFolderPath(NULL, szGroup, CSIDL_PROGRAMS, TRUE);
    PathAppend(szGroup, c_szStarDotStar);

    // We do a bunch of DDE work below, all of which is ANSI only.  This is
    // the cleanest point to break over from UNICODE to ANSI, so the conversion
    // is done here.
    // BUGBUG - BobDay - Is this right? Can't we do all in unicode?

#ifdef UNICODE
    if (0 == WideCharToMultiByte(CP_ACP, 0, szGroup, -1, szAGroup, MAX_PATH, NULL, NULL))
    {
        return NULL;
    }
    hff = FindFirstFileA(szAGroup, &fd);
#else
    hff = FindFirstFile(szGroup, &fd);
#endif

    if (hff != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (FindData_FileIsNormalA(&fd)))
            {
                // Data is seperated by \r\n.
                cch = lstrlenA(fd.cFileName) + 2;
                lpszBuf = (LPSTR)_LocalReAlloc(lpszBuf, cbBuf + (cch + 1) * SIZEOF(TCHAR), LMEM_MOVEABLE|LMEM_ZEROINIT);
                if (lpszBuf)
                {
                    // Copy it over.
                    lstrcpyA(lpszBuf + cbBuf, fd.cFileName);
                    lstrcatA(lpszBuf + cbBuf, c_szCRLF);
                    cbBuf = cbBuf + cch ;
                }
                else
                {
                    cbBuf = 0;
                }
            }
        } while (FindNextFileA(hff, &fd));
        FindClose(hff);

        //
        // If the user is an admin, then we need to enumerate
        // the common groups also.
        //

        if (IsUserAnAdmin()) {

            SHGetSpecialFolderPath(NULL, szGroup, CSIDL_COMMON_PROGRAMS, TRUE);
            PathAppend(szGroup, c_szStarDotStar);

#ifdef UNICODE
            if (0 == WideCharToMultiByte(CP_ACP, 0, szGroup, -1, szAGroup, MAX_PATH, NULL, NULL))
            {
                return NULL;
            }
            hff = FindFirstFileA(szAGroup, &fd);
#else
            hff = FindFirstFile(szGroup, &fd);
#endif


            if (hff != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                            (FindData_FileIsNormalA(&fd)))
                    {
                         // Data is seperated by \r\n.
                         cch = lstrlenA(fd.cFileName) + 2;
                         lpszBuf = (LPSTR)_LocalReAlloc(lpszBuf, cbBuf + (cch + 1) * SIZEOF(TCHAR), LMEM_MOVEABLE|LMEM_ZEROINIT);
                         if (lpszBuf)
                         {
                             // Copy it over.
                             lstrcpyA(lpszBuf + cbBuf, fd.cFileName);
                             lstrcatA(lpszBuf + cbBuf, c_szCRLF);
                             cbBuf = cbBuf + cch ;
                         }
                         else
                         {
                             cbBuf = 0;
                         }
                    }
                } while (FindNextFileA(hff, &fd));
                FindClose(hff);
            }
        }

        // Now package up the data and return.
        if (lpszBuf)
        {
            // Don't stomp on the last crlf, Word hangs while setting up
            // if this isn't present, just stick a null on the end.
            lpszBuf[cbBuf] = TEXT('\0');
            if (hszItem)
            {
                hData = DdeCreateDataHandle(g_dwDDEInst, (LPBYTE)lpszBuf, cbBuf+1, 0, hszItem, CF_TEXT, 0);
            }
            else
            {
                // Handle NULL hszItems (Logitech Fotomans installer does this). We need to create
                // a new hszItem otherwise DDEML gets confused (Null hszItems are only supposed to
                // be for DDE_EXECUTE data handles).
                TraceMsg(TF_WARNING, "EnumGroups: Invalid (NULL) hszItem used in request, creating new valid one.");
                hszItem = _DdeCreateStringHandle(g_dwDDEInst, c_szGroupsA, CP_WINANSI);
                hData = DdeCreateDataHandle(g_dwDDEInst, (LPBYTE)lpszBuf, cbBuf+1, 0, hszItem, CF_TEXT, 0);
                DdeFreeStringHandle(g_dwDDEInst, hszItem);
            }
            LocalFree(lpszBuf);
            return hData;
        }
    }

    // Empty list - Progman returned a single null.

    // BUGBUG (Davepl) I need to cast to LPBYTE since c_szNULLA is const.  If this
    // function doesn't really need to write to the buffer, it should be declared
    // as const

    hData = DdeCreateDataHandle(g_dwDDEInst, (LPBYTE)c_szNULLA, 1, 0, hszItem, CF_TEXT, 0);
    return hData;
}

//----------------------------------------------------------------------------
// Crossties 1.0 doesn't like an empty icon path (which couldn't happen in 3.1)
// so we make one here.
void ConstructIconPath(LPTSTR pszIP, LPCTSTR pszCL, LPCTSTR pszWD)
{
    TCHAR sz[MAX_PATH];

    lstrcpy(sz, pszCL);
    PathRemoveArgs(sz);
    PathUnquoteSpaces(sz);
    FindExecutable(sz, pszWD, pszIP);
}

//----------------------------------------------------------------------------
BOOL GroupItem_GetLinkInfo(LPCTSTR lpszGroupPath, PGROUPITEM pgi, LPCITEMIDLIST pidlLink,
    LPSHELLFOLDER psf, IShellLink *psl, IPersistFile *ppf)
{
    BOOL fRet = FALSE;
    STRRET str;
    TCHAR szName[MAX_PATH];
    DWORD dwAttribs;
    WCHAR wszPath[MAX_PATH];
    TCHAR sz[MAX_PATH];
    TCHAR szCL[MAX_PATH];
    TCHAR szArgs[MAX_PATH];
    int nShowCmd;
    LPTSTR pszData = NULL;

    ASSERT(pgi);
    ASSERT(pidlLink);
    ASSERT(psf);

    dwAttribs = SFGAO_LINK;
    if (SUCCEEDED(psf->GetAttributesOf(1, &pidlLink, &dwAttribs)))
    {
        if (dwAttribs & SFGAO_LINK)
        {
            // Get the relevant data.
            // Copy it.
            // Stick pointers in pgi.
            if (SUCCEEDED(psf->GetDisplayNameOf(pidlLink, SHGDN_NORMAL, &str)))
            {
                StrRetToStrN(szName, ARRAYSIZE(szName), &str, pidlLink);

                TraceMsg(TF_DDE, "Link %s", szName);

                pgi->pszDesc = StrDup(szName);
                PathCombine(sz, lpszGroupPath, szName);
                lstrcat(sz, c_szDotLnk);
                StrToOleStrN(wszPath, ARRAYSIZE(wszPath), sz, -1);
                // Read the link.
                // "name","CL",def dir,icon path,x,y,icon index,hotkey,minflag.
                ppf->Load(wszPath, 0);
                // Copy all the data.
                szCL[0] = TEXT('\0');
                if (SUCCEEDED(psl->GetPath(szCL, ARRAYSIZE(szCL), NULL, SLGP_SHORTPATH)))
                {
                    // Valid CL?
                    if (szCL[0])
                    {
                        // Yep, Uses LFN's?
                        // PathGetShortPath(sz);
                        szArgs[0] = TEXT('\0');
                        psl->GetArguments(szArgs, ARRAYSIZE(szArgs));
                        lstrcpy(sz, szCL);
                        if (szArgs[0])
                        {
                            lstrcat(sz, TEXT(" "));
                            StrNCat(sz, szArgs, ARRAYSIZE(sz));
                        }
                        pgi->pszCL = StrDup(sz);
                        TraceMsg(TF_DDE, "GroupItem_GetLinkInfo: CL %s", sz);
                        // WD
                        sz[0] = TEXT('\0');
                        psl->GetWorkingDirectory(sz, ARRAYSIZE(sz));
                        TraceMsg(TF_DDE, "GroupItem_GetLinkInfo: WD %s", sz);
                        if (sz[0])
                        {
                            TCHAR szShortPath[MAX_PATH];
                            if (GetShortPathName(sz, szShortPath, ARRAYSIZE(szShortPath)))
                                lstrcpy(sz, szShortPath);
                        }

                        pgi->pszWD = StrDup(sz);
                        // Now setup the Show Command - Need to map to index numbers...
                        psl->GetShowCmd(&nShowCmd);
                        if (nShowCmd == SW_SHOWMINNOACTIVE)
                        {
                            TraceMsg(TF_DDE, "GroupItem_GetLinkInfo: Show min.");
                            pgi->fMin = TRUE;
                        }
                        else
                        {
                            TraceMsg(TF_DDE, "GroupItem_GetLinkInfo: Show normal.");
                            pgi->fMin = FALSE;
                        }
                        // Icon path.
                        sz[0] = TEXT('\0');
                        pgi->iIcon = 0;
                        psl->GetIconLocation(sz, ARRAYSIZE(sz), &pgi->iIcon);
                        if (pgi->iIcon < 0)
                            pgi->iIcon = 0;
                        if (sz[0])
                            PathGetShortPath(sz);
                        else
                            ConstructIconPath(sz, pgi->pszCL, pgi->pszWD);
                        TraceMsg(TF_DDE, "GroupItem_GetLinkInfo: IL %s %d", sz, pgi->iIcon);
                        pgi->pszIconPath = StrDup(sz);
                        // Hotkey
                        pgi->wHotkey = 0;
                        psl->GetHotkey(&pgi->wHotkey);
                        // Success.
                        fRet = TRUE;
                    }
                    else
                    {
                        // Deal with links to weird things.
                        TraceMsg(TF_DDE, "GroupItem_GetLinkInfo: Invalid command line.");
                    }
                }
            }
        }
    }

    return fRet;
}


int DSA_DestroyGroupCallback(LPVOID p, LPVOID d)
{
    PGROUPITEM pgi = (PGROUPITEM)p;
    LocalFree(pgi->pszDesc);
    LocalFree(pgi->pszCL);
    LocalFree(pgi->pszWD);
    LocalFree(pgi->pszIconPath);
    return 1;
}

//---------------------------------------------------------------------------
// Return the links in a group.
HDDEDATA EnumItemsInGroup(HSZ hszItem, LPCTSTR lpszGroup)
{
    HRESULT hres;
    LPITEMIDLIST pidl, pidlGroup;
    LPSHELLFOLDER psf;
    TCHAR sz[MAX_PATH];
    TCHAR szLine[MAX_PATH*4];
    HDDEDATA hddedata = HDDENULL;
    ULONG celt;
    GROUPITEM gi;
    int cItems = 0;
    IPersistFile *ppf;
    IShellLink *psl;
    HDSA hdsaGroup;
    UINT cbDDE;
    UINT cchDDE;
    int x, y;
    LPTSTR pszDDE = NULL;
    PGROUPITEM pgi;
    BOOL fOK = FALSE;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    BOOL bCommon = FALSE;

    TraceMsg(TF_DDE, "c.eiig: Enumerating %s.", (LPTSTR)lpszGroup);


    //
    // Get personal group location
    //

    if (!SHGetSpecialFolderPath(NULL, sz, CSIDL_PROGRAMS, FALSE)) {
        return NULL;
    }

    PathAddBackslash(sz);
    lstrcat(sz, lpszGroup);

    //
    // Test if the group exists.
    //

    hFile = FindFirstFile (sz, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

       if (SHRestricted(REST_NOCOMMONGROUPS)) {
           return NULL;
       }

       //
       // Personal group doesn't exist.  Try a common group.
       //

       if (!SHGetSpecialFolderPath(NULL, sz, CSIDL_COMMON_PROGRAMS, FALSE)) {
           return NULL;
       }

       PathAddBackslash(sz);
       lstrcat(sz, lpszGroup);
       bCommon = TRUE;

    } else {
        FindClose (hFile);
    }



    hdsaGroup = DSA_Create(SIZEOF(GROUPITEM), 0);
    if (hdsaGroup)
    {
        // Get the group info.
        pidlGroup = ILCreateFromPath(sz);
        if (pidlGroup)
        {
            IShellFolder* psfDesktop;

            hres = SHGetDesktopFolder(&psfDesktop);
            if (SUCCEEDED(hres))
            {
                hres = psfDesktop->BindToObject(pidlGroup, NULL, IID_IShellFolder, (LPVOID*)&psf);
                if (SUCCEEDED(hres))
                {
                    LPENUMIDLIST penum;
                    hres = psf->EnumObjects(NULL, SHCONTF_NONFOLDERS, &penum);
                    if (SUCCEEDED(hres))
                    {
                        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
                        if (SUCCEEDED(hres))
                        {
                            psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
                            ASSERT(ppf); // nobody checks it below
                            while ((penum->Next(1, &pidl, &celt) == NOERROR) && (celt == 1))
                            {
                                if (GroupItem_GetLinkInfo(sz, &gi, pidl, psf, psl, ppf))
                                {
                                    // Add it to the list
                                    DSA_InsertItem(hdsaGroup, cItems, &gi);
                                    cItems++;
                                }
                                ILFree(pidl);
                            }
                            fOK = TRUE;
                            ppf->Release();
                            psl->Release();
                        }
                        penum->Release();
                    }
                    psf->Release();
                }
                psfDesktop->Release();
            }
            ILFree(pidlGroup);
        }
        else
        {
            TraceMsg(DM_ERROR, "c.eiig: Can't create IDList for path..");
        }

        if (fOK)
        {
            // Create dde data.
            TraceMsg(TF_DDE, "c.eiig: %d links", cItems);

            // "Group Name",path,#items,showcmd
            PathGetShortPath(sz);
            wsprintf(szLine, TEXT("\"%s\",%s,%d,%d,%d\r\n"), lpszGroup, sz, cItems, SW_SHOWNORMAL, bCommon);
            cchDDE = lstrlen(szLine)+1;
            cbDDE = cchDDE * SIZEOF(TCHAR);
            pszDDE = (LPTSTR)LocalAlloc(LPTR, cbDDE);
            if (pszDDE)
            {
                lstrcpy(pszDDE, szLine);
                cItems--;
                while (cItems >= 0)
                {
                    pgi = (GROUPITEM*)DSA_GetItemPtr(hdsaGroup, cItems);
                    ASSERT(pgi);
                    // Fake up reasonable coords.
                    x = ((cItems%ITEMSPERROW)*64)+32;
                    y = ((cItems/ITEMSPERROW)*64)+32;
                    // "name","CL",def dir,icon path,x,y,icon index,hotkey,minflag.
                    wsprintf(szLine, TEXT("\"%s\",\"%s\",%s,%s,%d,%d,%d,%d,%d\r\n"), pgi->pszDesc, pgi->pszCL,
                        pgi->pszWD, pgi->pszIconPath, x, y, pgi->iIcon, pgi->wHotkey, pgi->fMin);
                    cchDDE += lstrlen(szLine);
                    cbDDE = cchDDE * SIZEOF(TCHAR);
                    pszDDE = (LPTSTR)_LocalReAlloc((HLOCAL)pszDDE, cbDDE + SIZEOF(TCHAR), LMEM_MOVEABLE|LMEM_ZEROINIT);
                    if (pszDDE)
                    {
                        lstrcat(pszDDE, szLine);
                        cItems--;
                    }
                    else
                    {
                        TraceMsg(DM_ERROR, "c.eiig: Unable to realocate DDE line.");
                        break;
                    }
                }

#ifdef UNICODE
                // Multiply by two, for worst case, where every char was a multibyte char
                int cbADDE = lstrlen(pszDDE) * 2;       // Trying to make an ANSI string!!!
                LPSTR pszADDE = (LPSTR)LocalAlloc(LPTR, cbADDE + 2);
                if (pszADDE)
                {
                    WideCharToMultiByte(CP_ACP, 0, pszDDE, -1, pszADDE, cbADDE, NULL, NULL);

                    hddedata = DdeCreateDataHandle(g_dwDDEInst, (LPBYTE)pszADDE, cbADDE, 0, hszItem, CF_TEXT, 0);
                    LocalFree(pszADDE);
                }
                else
                {
                    TraceMsg(DM_ERROR, "c.eiig: Can't allocate ANSI buffer.");
                }
#else
                hddedata = DdeCreateDataHandle(g_dwDDEInst, (LPBYTE)pszDDE, cbDDE+1, 0, hszItem, CF_TEXT, 0);
#endif
                LocalFree(pszDDE);
            }
        }
        else
        {
            TraceMsg(DM_ERROR, "c.eiig: Can't create group list.");
        }
        
        DSA_DestroyCallback(hdsaGroup, DSA_DestroyGroupCallback, 0);
    }

    return hddedata;
}

//---------------------------------------------------------------------------
HDDEDATA DDE_HandleRequest(HSZ hszItem, HCONV hconv)
{
    TCHAR szGroup[MAX_PATH];
    PDDECONV pddec;

    TraceMsg(TF_DDE, "DDEML Request(%lx) - OK.", (DWORD)hconv);

    pddec = DDE_MapHConv(hconv);
    if (pddec == NULL)
        return HDDENULL;

    DdeQueryString(g_dwDDEInst, hszItem, szGroup, ARRAYSIZE(szGroup), CP_WINNATURAL);

    TraceMsg(TF_DDE, "Request for item %s.", (LPTSTR) szGroup);
    // There's a bug in Progman where null data returns the list of groups.
    // Logitech relies on this behaviour.
    if (szGroup[0] == TEXT('\0'))
    {
        return EnumGroups(hszItem);
    }
    // Special case group names of "Groups" or "Progman" and return the list
    // of groups instead.
    else if (lstrcmpi(szGroup, c_szGroupGroup) == 0 || lstrcmpi(szGroup, c_szTopic) == 0)
    {
        return EnumGroups(hszItem);
    }
    // Special case winoldapp properties.
    else if (lstrcmpi(szGroup, c_szGetIcon) == 0 ||
        lstrcmpi(szGroup, c_szGetDescription) == 0 ||
        lstrcmpi(szGroup, c_szGetWorkingDir) == 0)
    {
        return HDDENULL;
    }
    // Assume it's a group name.
    else
    {
        return EnumItemsInGroup(hszItem, szGroup);
    }
}

//---------------------------------------------------------------------------
// Support Disconnect
void DDE_HandleDisconnect(HCONV hconv)
{
    PDDECONV pddecPrev = NULL;
    PDDECONV pddec;

    TraceMsg(TF_DDE, "DDEML Disconnect(%lx) - OK.", (DWORD)hconv);

    // Find the conversation in the list of them and free it.
    ENTERCRITICAL;
    for (pddec = g_pddecHead; pddec != NULL; pddec = pddec->pddecNext)
    {
        if (pddec->hconv == hconv)
        {
            // Found it, so first unlink it
            if (pddecPrev == NULL)
                g_pddecHead = pddec->pddecNext;
            else
                pddecPrev->pddecNext = pddec->pddecNext;
            break;
        }
        pddecPrev = pddec;
    }
    LEAVECRITICAL;

    // Now Free it outside of critical section
    if (pddec)
    {
        pddec->psl->Release();

        // Were we forced to create a redirected drive?
        if (pddec->dwFlags & DDECONV_FORCED_CONNECTION)
        {
            // Yep. Clean it up now.
            Net_DisconnectDrive(pddec->chDrive);
        }

        if ((pddec->dwFlags & DDECONV_REPEAT_ACKS) && g_nTimer)
        {
            KillTimer(NULL, g_nTimer);
            g_nTimer = 0;
        }

        LocalFree(pddec);
    }

    g_hwndDde = NULL;
}

//---------------------------------------------------------------------------
// Support wildcard topics.
HDDEDATA DDE_HandleWildConnects(void)
{
    HSZPAIR hszpair[4];

    TraceMsg(TF_DDE, "DDEML wild connect.");

    hszpair[0].hszSvc = g_hszService;
    hszpair[0].hszTopic = g_hszTopic;
    hszpair[1].hszSvc = g_hszShell;
    hszpair[1].hszTopic = g_hszAppProps;
    hszpair[2].hszSvc = g_hszFolders;
    hszpair[2].hszTopic = g_hszAppProps;
    hszpair[3].hszSvc = HSZNULL;
    hszpair[3].hszTopic = HSZNULL;

    return DdeCreateDataHandle(g_dwDDEInst, (LPBYTE)&hszpair, SIZEOF(hszpair), 0, HSZNULL, CF_TEXT, 0);
}

//---------------------------------------------------------------------------
// App hack flags for DDE.
// REVIEW UNDONE - Read these from the registry so we can app hack on the fly.

// Bodyworks.
// Uses PostMessage(-1,...) to talk to the shell and DDEML
// can't handle that level of abuse. By having DDEML ignore the command
// it'll get forwarded through to the desktop which can handle it. Sigh.

// CCMail.
// Can't handle being installed via a UNC but unlike most app that have
// problems with UNC's they appear to set up fine - you'll just have
// lots of problems trying to run the app. We handle this by faking
// up a drive connection for them. We don't want to do this generally
// since the user could easily run out of drive letters.

// Discis. [There are dozens of Discis apps that use the same setup.]
// Can't handle getting activate messages out of order with DDE (which
// happens easily now). They end up spinning in a loop looking for an
// ACK they've already got. We hack around this by detecting them being
// hung and post them another ack. We keep doing that until they wake
// up and start talking to us again.

// Media Recorder.
// Their app wants to be single instance so at init they search for
// windows with the TITLE (!!!) of "MediaRecorder". If you launch
// them from their own folder (which has the title "MediaRecorder" then
// they refuse to run. We fix this by mapping their group name at
// setup time.

// Trio DataFax.
// This app wants to add something to the startup group but doesn't
// know what it's called so it tries to load the Startup string out
// of Progman. If Progman isn't running they try to create a group
// with a NULL title. We detect this case and map them to the new
// startup group name.

// TalkToPlus.
// They try to make a link to Terminal.exe and abort their setup
// if the AddItem fails. We fix this my forcing the AddItem to
// return success.

// Winfax Pro 4.0.
// They use the shell= line in win.ini for the service/topic so
// they end up talking to the shell using Explorer/Explorer!
// They also talk to the LAST responder to the init broadcast
// instead of the first AND they use SendMsg/Free instead of waiting for
// Acks. We fix this by allowing their service/topic to work, and have
// the desktop copy the data before sending it through to DDEML.
// REVIEW We key off the fact that their dde window is a dialog with no
// title - seems a bit broad to me.

// The Journeyman Project.
// This app causes damage to space-time. We fix it by generating a
// small HS-field around their installer.

// CA apps in general.
// Don't bother sending DDE_INIT's before sending the execute commands.
// We fix it by doing the init on the fly if needed.

// Faxserve.
// Broadcasts their EXEC commands. Their class name is "install" which
// is a bit too generic for my liking but since we handle this problem
// by forcing everything to go through the desktop it's not very risky.

struct {
    LPCTSTR pszClass;
    LPCTSTR pszTitle;
    DWORD id;
} const c_DDEApps[] = {
    c_szMrPostman,          NULL,           DDECONV_NO_INIT,
    c_szBodyWorks,          NULL,           DDECONV_FAIL_CONNECTS,
    c_szCCMail,             NULL,           DDECONV_NO_UNC,
    c_szDiscis,             NULL,           DDECONV_REPEAT_ACKS,
    c_szMediaRecorder,      NULL,           DDECONV_MAP_MEDIA_RECORDER,
    c_szTrioDataFax,        NULL,           DDECONV_NULL_FOR_STARTUP,
    c_szTalkToPlus,         NULL,           DDECONV_ALLOW_INVALID_CL,
    c_szDialog,             c_szMakePMG,    DDECONV_REPEAT_ACKS,
    c_szDialog,             c_szNULL,       DDECONV_EXPLORER_SERVICE_AND_TOPIC|DDECONV_USING_SENDMSG,
    c_szJourneyMan,         NULL,           DDECONV_EXPLORER_SERVICE_AND_TOPIC,
    c_szCADDE,              NULL,           DDECONV_NO_INIT,
    c_szFaxServe,           NULL,           DDECONV_FAIL_CONNECTS
};

//---------------------------------------------------------------------------
DWORD GetDDEAppFlagsFromWindow(HWND hwnd)
{
    int i;
    TCHAR szClass[MAX_PATH];

    if (hwnd && !Window_IsWin32OrWin4(hwnd))
    {
        GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
        for (i=0; i<ARRAYSIZE(c_DDEApps); i++)
        {
            // NB Keep this case sensative to narrow the scope a bit.
            if (lstrcmp(szClass, c_DDEApps[i].pszClass) == 0)
            {
                // Do we care about the title?
                if (c_DDEApps[i].pszTitle)
                {
                    TCHAR szTitle[MAX_PATH];

                    GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle));
                    if (lstrcmp(szTitle, c_DDEApps[i].pszTitle) == 0)
                    {
                        TraceMsg(TF_DDE, "App flags 0x%x for %s %s.", c_DDEApps[i].id, c_DDEApps[i].pszClass, c_DDEApps[i].pszTitle);
                        return c_DDEApps[i].id;
                    }
                }
                else
                {
                    // Nope.
                    TraceMsg(TF_DDE, "App flags 0x%x for %s.", c_DDEApps[i].id, c_DDEApps[i].pszClass);
                    return c_DDEApps[i].id;
                }
            }
        }
    }

    return DDECONV_NONE;
}

//---------------------------------------------------------------------------
DWORD GetDDEAppFlags(HCONV hconv)
{
    return GetDDEAppFlagsFromWindow(_GetDDEPartnerWindow(hconv));
}

//---------------------------------------------------------------------------
HDDEDATA DDE_HandleConnect(HSZ hsz1, HSZ hsz2)
{
    if ((hsz1 == g_hszTopic && hsz2 == g_hszService) ||
        (hsz1 == g_hszAppProps && hsz2 == g_hszShell) ||
        (hsz1 == g_hszAppProps && hsz2 == g_hszFolders))
    {
        TraceMsg(TF_DDE, "DDEML Connect.");
        return (HDDEDATA)DDE_FACK;
    }
    else
    {
        // Unknown topic/service.
        TraceMsg(TF_DDE, "DDEML Connect - unknown service/topic.");
        return (HDDEDATA)NULL;
    }
}

//---------------------------------------------------------------------------
// Returns TRUE if the drive where the Programs folder is supports LFNs.
BOOL _SupportLFNGroups(void)
{
    TCHAR szPrograms[MAX_PATH];
    DWORD dwMaxCompLen = 0;
    
    SHGetSpecialFolderPath(NULL, szPrograms, CSIDL_PROGRAMS, TRUE);
    return IsLFNDrive(szPrograms);
}

//---------------------------------------------------------------------------
// REVIEW HACK - Don't just caste, call GetConvInfo() to get this. We can't
// do this as yet because of a bug in the thunk layer.
#define _GetDDEWindow(hconv)    ((HWND)hconv)

//---------------------------------------------------------------------------
HDDEDATA DDE_HandleConnectConfirm(HCONV hconv)
{
    DWORD dwAppFlags = GetDDEAppFlags(hconv);
    PDDECONV pddec;

    if (dwAppFlags & DDECONV_FAIL_CONNECTS)
    {
        DdeDisconnect(hconv);
        return FALSE;
    }

    pddec = (PDDECONV)LocalAlloc(LPTR, SIZEOF(DDECONV));
    if (pddec)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&pddec->psl)))
        {
            pddec->hconv = hconv;
            // pddec->szGroup[0] = '\0';   // implicit
            // pddec->fDirty = FALSE;      // implicit
            // protect access to global list
            ENTERCRITICAL;
            pddec->pddecNext = g_pddecHead;
            g_pddecHead = pddec;
            LEAVECRITICAL;

            TraceMsg(TF_DDE, "DDEML Connect_CONFIRM(%lx) - OK.", (DWORD)hconv);

            // Do we support LFN groups?
            g_LFNGroups = _SupportLFNGroups();
            // Tell the desktops DDE code we're handling things from here.
            g_hwndDde = _GetDDEWindow(hconv);
            // No conversation yet (wild connect?) - signal it with a hwnd -1.
            if (!g_hwndDde)
                g_hwndDde = (HWND)-1;
            // Keep track of the app hacks.
            pddec->dwFlags = dwAppFlags;

            // Success.
            return (HDDEDATA)DDE_FACK;
        }
        TraceMsg(TF_DDE, "Unable to create IShellLink interface.");
        LocalFree(pddec);
    }
    else
    {
        TraceMsg(TF_ERROR, "Unable to allocate memory for tracking dde conversations.");
    }
    return (HDDEDATA)NULL;
}

//---------------------------------------------------------------------------
HDDEDATA CALLBACK DDECallback(UINT type, UINT fmt, HCONV hconv,
        HSZ hsz1, HSZ hsz2,HDDEDATA  hData, DWORD dwData1, DWORD dwData2)
{
    switch (type)
    {
    case XTYP_CONNECT:
        return DDE_HandleConnect(hsz1, hsz2);

    case XTYP_WILDCONNECT:
        return DDE_HandleWildConnects();

    case XTYP_CONNECT_CONFIRM:
        return DDE_HandleConnectConfirm(hconv);

    case XTYP_REGISTER:
    case XTYP_UNREGISTER:
        return (HDDEDATA) NULL;

    case XTYP_ADVDATA:
        return (HDDEDATA) DDE_FACK;

    case XTYP_XACT_COMPLETE:
        return (HDDEDATA) NULL;

    case XTYP_DISCONNECT:
        DDE_HandleDisconnect(hconv);
        return (HDDEDATA) NULL;

    case XTYP_EXECUTE:
        return HandleDDEExecute(hData, hconv);

    case XTYP_REQUEST:
        if (hsz1 == g_hszTopic || hsz1 == g_hszAppProps)
        {
            return DDE_HandleRequest(hsz2, hconv);
        }
        else
        {
            TraceMsg(TF_DDE, "DDEML Request - Invalid Topic.");
            return (HDDEDATA) NULL;
        }

    default:
        return (HDDEDATA) NULL;

    }
}

static BOOL s_bDDEInited = FALSE;

ATOM g_aProgman = 0;

//---------------------------------------------------------------------------
void InitialiseDDE(void)
{
    DBG_ENTER(FTF_DDE, InitialiseDDE);

    // No need to do this twice
    if ( !s_bDDEInited )
    {
        // Hack for Alone In the Dark 2.
        // They do a case sensative comparison of the progman atom and they
        // need it to be uppercase.
        g_aProgman = GlobalAddAtom(TEXT("PROGMAN"));

        if (DdeInitialize(&g_dwDDEInst, DDECallback, CBF_FAIL_POKES | CBF_FAIL_ADVISES, 0L))
        {
            TraceMsg(TF_DDE, "DDE Initialization failure.");
        }

        g_hszTopic = _DdeCreateStringHandle(g_dwDDEInst, c_szTopic, CP_WINNATURAL);
        g_hszService = _DdeCreateStringHandle(g_dwDDEInst, c_szService, CP_WINNATURAL);
        g_hszStar = _DdeCreateStringHandle(g_dwDDEInst, c_szStar, CP_WINNATURAL);
        g_hszShell = _DdeCreateStringHandle(g_dwDDEInst, c_szShell, CP_WINNATURAL);
        g_hszAppProps = _DdeCreateStringHandle(g_dwDDEInst, c_szAppProps, CP_WINNATURAL);
        g_hszFolders = _DdeCreateStringHandle(g_dwDDEInst, c_szFolders, CP_WINNATURAL);

        DdeNameService(g_dwDDEInst, g_hszFolders,  HSZNULL, DNS_REGISTER);

        DDE_AddShellServices();

        s_bDDEInited = TRUE;
    }

    DBG_EXIT(FTF_DDE, InitialiseDDE);
}

//---------------------------------------------------------------------------
void UnInitialiseDDE(void)
{
    if (!s_bDDEInited)
    {
        return;
    }

    DDE_RemoveShellServices();

    DdeNameService(g_dwDDEInst, g_hszFolders,  HSZNULL, DNS_UNREGISTER);

    DdeFreeStringHandle(g_dwDDEInst, g_hszTopic);
    DdeFreeStringHandle(g_dwDDEInst, g_hszService);
    DdeFreeStringHandle(g_dwDDEInst, g_hszStar);
    DdeFreeStringHandle(g_dwDDEInst, g_hszShell);
    DdeFreeStringHandle(g_dwDDEInst, g_hszAppProps);
    DdeFreeStringHandle(g_dwDDEInst, g_hszFolders);

    if (!DdeUninitialize(g_dwDDEInst))
    {
        TraceMsg(TF_DDE, "DDE Un-Initialization failure.");
    }

    g_dwDDEInst = 0;

    if (g_aProgman)
        g_aProgman = GlobalDeleteAtom(g_aProgman);

    s_bDDEInited = FALSE;

}

//---------------------------------------------------------------------------
void DDE_AddShellServices(void)
{
    ASSERT(g_dwDDEInst);
    ASSERT(g_hszService);
    ASSERT(g_hszShell);

    // Only register these if we are the shell...
    DdeNameService(g_dwDDEInst, g_hszService,  HSZNULL, DNS_REGISTER);
    DdeNameService(g_dwDDEInst, g_hszShell,  HSZNULL, DNS_REGISTER);
}

//---------------------------------------------------------------------------
void DDE_RemoveShellServices(void)
{
    // If dde is not installed blow out of here
    if (!s_bDDEInited)
        return;

    ASSERT(g_dwDDEInst);

    DdeNameService(g_dwDDEInst, g_hszService,  HSZNULL, DNS_UNREGISTER);
    DdeNameService(g_dwDDEInst, g_hszShell,  HSZNULL, DNS_UNREGISTER);
}



BOOL GetGroupName(LPCTSTR lpszOld, LPTSTR lpszNew, ULONG cbNew)
{
    DWORD dwType;

    return ERROR_SUCCESS == SHGetValue(g_hkcuExplorer, c_szMapGroups, lpszOld, &dwType, (LPVOID)lpszNew, &cbNew);
}

void MapGroupName(LPCTSTR lpszOld, LPTSTR lpszNew, ULONG cbNew)
{
    if (!GetGroupName(lpszOld, lpszNew, cbNew))
    {
        lstrcpyn(lpszNew, lpszOld, cbNew);
    }
}

STDAPI_(BOOL) DDEHandleViewFolderNotify(IShellBrowser* psb, HWND hwnd, LPNMVIEWFOLDER lpnm)
{
    BOOL fRet = FALSE;
    TCHAR szCmd[MAX_PATH + 100];
    UINT *lpwCmd;
    
    if (g_fRunningOnNT) 
        OleStrToStrN(szCmd, ARRAYSIZE(szCmd), (LPWSTR)lpnm->szCmd, (UINT)-1);
    else
        lstrcpyn(szCmd, lpnm->szCmd, ARRAYSIZE(szCmd));

    lpwCmd = GetDDECommands(szCmd, c_sDDECommands, FALSE);
    if (lpwCmd) 
    {
        UINT *lpwCmdSave = lpwCmd;
        UINT c = *lpwCmd++;

        LPCTSTR pszCommand = c_sDDECommands[c].pszCommand;

        ASSERT(c < ARRAYSIZE(c_sDDECommands));

        if (pszCommand == c_szViewFolder || 
            pszCommand == c_szExploreFolder)
        {
            fRet = DoDDE_ViewFolder(psb, hwnd, szCmd, lpwCmd,
                    pszCommand == c_szExploreFolder, lpnm->dwHotKey);
        }
        else if (pszCommand == c_szShellFile)
        {
            fRet = DDE_ShellFile(szCmd, lpwCmd, 0);
        }

        GlobalFree(lpwCmdSave);
    }
    return fRet;
} 

STDAPI_(LPNMVIEWFOLDER) DDECreatePostNotify(LPNMVIEWFOLDER pnm)
{
    LPNMVIEWFOLDER pnmPost = NULL;
    TCHAR szCmd[MAX_PATH * 2];

    if (g_fRunningOnNT) 
        OleStrToStrN(szCmd, ARRAYSIZE(szCmd), (LPWSTR)pnm->szCmd, (UINT)-1);
    else
        lstrcpyn(szCmd, pnm->szCmd, ARRAYSIZE(szCmd));

    UINT *pwCmd = GetDDECommands(szCmd, c_sDDECommands, FALSE);
    if (pwCmd) 
    {
        LPCTSTR pszCommand = c_sDDECommands[*pwCmd].pszCommand;

        ASSERT(*pwCmd < ARRAYSIZE(c_sDDECommands));

        //
        //  these are the only commands handled by a PostNotify
        if (pszCommand == c_szViewFolder 
        ||  pszCommand == c_szExploreFolder
        ||  pszCommand == c_szShellFile)
        {
            DWORD cbSize = g_fRunningOnNT ? SIZEOF(NMVIEWFOLDERW) : SIZEOF(NMVIEWFOLDERA);
            pnmPost = (LPNMVIEWFOLDER)LocalAlloc(LPTR, cbSize);

            if (pnmPost)
            {
                if(!IsBadReadPtr(pnm,cbSize))
                    memcpy(pnmPost, pnm, cbSize);
                else 
                {
                    LocalFree(pnmPost);
                    pnmPost = NULL;
                }
            }
     
        }

        GlobalFree(pwCmd);
    }

    return pnmPost;
}

//---------------------------------------------------------------------------
LRESULT _ForwardDDEMsgs(HWND hwnd, HWND hwndForward, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend)
{
    TraceMsg(TF_DDE, "c.fdm: Forwarding DDE to %x", hwndForward);

    if (hwndForward && IsWindow(hwndForward))
    {
        TraceMsg(TF_DDE, "c.fdm: %lx %lx %lx", uMsg, (WPARAM)hwnd, lParam);
        if (fSend)
            return SendMessage(hwndForward, uMsg, (WPARAM)hwnd, lParam);
        else
            return PostMessage(hwndForward, uMsg, (WPARAM)hwnd, lParam);
    }
    else
    {
        TraceMsg(TF_DDE, "c.fdm: Invalid DDEML window, Can't forward DDE messages.");
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

//---------------------------------------------------------------------------
// Set/cleared by dde connect/disconnect.
const TCHAR c_szExplorerTopic[] = TEXT("Explorer");
const TCHAR c_szDMGFrame[] = TEXT("DMGFrame");  // This is the 16-bit/Win95 window class name

//---------------------------------------------------------------------------
// Broadcast to all ddeml server windows.

void DDEML_Broadcast(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (g_fRunningOnNT)
    {
        HWND hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
        while (hwnd)
        {
            TCHAR szClass[32];
            GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
            if ((lstrcmp(szClass, c_szDMGFrame) == 0) ||
                (lstrcmp(szClass, TEXT("DDEMLMom")) == 0))    // this is the 32-bit NT window class name
                SendMessage(hwnd, uMsg, wParam, lParam);
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        }
    }
    else
    {
        // Win95 Version
        HWND hwnd = NULL;
        while ((hwnd = FindWindowEx(NULL, hwnd, c_szDMGFrame, NULL)) != NULL)
        {
            //
            // give the window 20 seconds before continuing
            // (this is the same wait USER uses before nuking hung apps)
            // BUGBUG: make this smaller
            //
            SendMessageTimeout(hwnd, uMsg, wParam, lParam, SMTO_ABORTIFHUNG, 20000, NULL);
        }
    }
}


//---------------------------------------------------------------------------
LRESULT _HandleDDEInitiateAndAck(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL g_fInInit = FALSE;
    ATOM aProgman;
    TCHAR szService[32];
    TCHAR szTopic[32];
    TCHAR szClass[32];
    UINT uHigh, uLow;
    BOOL fForceAccept = FALSE;

    if (uMsg == WM_DDE_INITIATE)
    {
        TraceMsg(TF_DDE, "c.hdi: Init.");

        // Don't handle DDE messages if we're already using DDEML. This happens when apps
        // broadcast DDE_INIT and don't stop on the first reply. Both our DDEML window and
        // the desktop end up replying. Most apps don't care and just talk to the first or
        // the last one but Ventura gets confused and thinks it's finished doing DDE when it
        // gets the second ACK and destroys it's internal DDE window.
        if (g_hwndDde)
        {
            TraceMsg(TF_DDE, "c.fpwp: Not forwarding DDE, DDEML is handing it.");
            KillTimer(hwnd, IDT_DDETIMEOUT);
        }
        // Are we re-cursing?
        else if (!g_fInInit)
        {
            // Nope, Is this for Progman, Progman or Shell, AppProperties?
            if (lParam)
            {
                GlobalGetAtomName(LOWORD(lParam), szService, ARRAYSIZE(szService));
                GlobalGetAtomName(HIWORD(lParam), szTopic, ARRAYSIZE(szTopic));
            }
            else
            {
                // Progman allowed a null Service & a null Topic to imply Progman, Progman.
                szService[0] = TEXT('\0');
                szTopic[0] = TEXT('\0');
                fForceAccept = TRUE;
            }

            // Keep track of hacks, we reset this on the disconnect.
            g_dwAppFlags = GetDDEAppFlagsFromWindow((HWND)wParam);

            // Hacks for WinFax and Journeyman Project.
            if ((g_dwAppFlags & DDECONV_EXPLORER_SERVICE_AND_TOPIC)
                && (lstrcmpi(szTopic, c_szExplorerTopic) == 0)
                && (lstrcmpi(szService, c_szExplorerTopic) == 0))
            {
                fForceAccept = TRUE;
            }

            if (((lstrcmpi(szTopic, c_szTopic) == 0) && (lstrcmpi(szService, c_szService) == 0)) ||
                fForceAccept)
            {
                TraceMsg(TF_DDE, "c.hdi: Init on [Progman,Progman] - needs forwarding.");
                // Nope go find it.
                // NB This will cause an echo on every DDE_INIT for Progman, Progman after booting.
                // It shouldn't be a problem :-)
                // Keep track of who to send Acks back to.
                g_hwndClient = (HWND)wParam;
                // Now find the real shell.
                aProgman = GlobalAddAtom(c_szService);
                TraceMsg(TF_DDE, "c.d_hdm: Finding shell dde handler...");
                g_fInInit = TRUE;
                // SendMessage(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwnd, MAKELPARAM(aProgman, aProgman));
                DDEML_Broadcast(WM_DDE_INITIATE, (WPARAM)hwnd, MAKELPARAM(aProgman, aProgman));
                g_fInInit = FALSE;
                TraceMsg(TF_DDE, "c.d_hdm: ...Done");
                GlobalDeleteAtom(aProgman);
            }
            else
            {
                TraceMsg(TF_DDE, "c.hdi: Init on something other than [Progman,Progman] - Ignoring");
                KillTimer(hwnd, IDT_DDETIMEOUT);
            }
        }
        else
        {
            TraceMsg(TF_DDE, "c.hdi: Recursing - Init ignored.");
        }
        return 0;
    }
    else if (uMsg == WM_DDE_ACK)
    {
        TraceMsg(TF_DDE, "c.hdi: Ack.");
        // Is this in response to the DDE_Init above?
        if (g_fInInit)
        {
            // Yep, keep track of who we're talking too.
            GetClassName((HWND)wParam, szClass, ARRAYSIZE(szClass));
            TraceMsg(TF_DDE, "c.d_hdm: Init-Ack from %x (%s).", wParam, szClass);
            g_hwndDDEML = (HWND)wParam;
            // The forward it back (send it, don't post it - Breaks Prodogy).
            return _ForwardDDEMsgs(hwnd, g_hwndClient, uMsg, (WPARAM)hwnd, lParam, TRUE);
        }
        else
        {
            // Nope, just forward it back.

            // Hack for WinFaxPro.
            if (g_dwAppFlags & DDECONV_USING_SENDMSG)
            {
                // We copied the data before sending it on so we can free it here.
                // WinFax ignores the reply so don't bother sending it.
                UnpackDDElParam(uMsg, lParam, &uLow, &uHigh);
                if (uHigh)
                    GlobalFree((HGLOBAL)uHigh);
                return 0;
            }

            return _ForwardDDEMsgs(hwnd, g_hwndClient, uMsg, (WPARAM)hwnd, lParam, FALSE);
        }
    }
    return 0;
}

//---------------------------------------------------------------------------
LRESULT _HandleDDEForwardBiDi(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ((HWND)wParam == g_hwndDDEML)
        return _ForwardDDEMsgs(hwnd, g_hwndClient, uMsg, wParam, lParam, FALSE);
    else if ((HWND)wParam == g_hwndClient)
        return _ForwardDDEMsgs(hwnd, g_hwndDDEML, uMsg, wParam, lParam, FALSE);
    else
        return 0;
}

//---------------------------------------------------------------------------
LRESULT _HandleDDETerminate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndClient;

    TraceMsg(DM_TRACE, "c.hddet: Terminate.");

    if ((HWND)wParam == g_hwndDDEML)
    {
        // This should be the last message (a terminate from ddeml to the client).
        // Cleanup now.
        KillTimer(hwnd, IDT_DDETIMEOUT);
        TraceMsg(DM_TRACE, "c.hddet: Cleanup.");
        hwndClient = g_hwndClient;
        g_hwndClient = NULL;
        g_hwndDDEML = NULL;
        g_dwAppFlags = DDECONV_NONE;
        return _ForwardDDEMsgs(hwnd, hwndClient, uMsg, wParam, lParam, FALSE);
    }
    else if ((HWND)wParam == g_hwndClient)
    {
        return _ForwardDDEMsgs(hwnd, g_hwndDDEML, uMsg, wParam, lParam, FALSE);
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------
LRESULT _HandleDDEExecute(HWND hwnd, HWND hwndForward, UINT uMsg,
        WPARAM wParam, LPARAM lParam, BOOL fSend)
{
    ATOM aApp, aTopic;
    HANDLE hNew;
    LPTSTR pNew, pOld;

    // NB WinFaxPro does a Send/Free which avoids Users DDE hack
    // and means they get to delete the data while we're in
    // the middle of using it so we must copy it here. We'll
    // clean it up on the Ack.
    // NB WinFaxPro re-uses the same 16bit selector for all their
    // messages which the thunk layer can't handle it. We need to
    // defect the 32bit side (and free it) so the next time they
    // send the 16bit handle through the thunk layer they get a
    // new 32bit version.
    if (g_dwAppFlags & DDECONV_USING_SENDMSG)
    {
        UINT cb = GlobalSize((HGLOBAL)lParam);
        hNew = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
        if (hNew)
        {
            // Copy the old data.
            pNew = (LPTSTR)GlobalLock(hNew);
            pOld = (LPTSTR)GlobalLock((HGLOBAL)lParam);
            memcpy(pNew, pOld, cb);
            GlobalUnlock((HGLOBAL)lParam);
            GlobalUnlock(hNew);
            // Tell kernel we're done with 32bit side
            // of the old handle and then free it.
            SHGlobalDefect((DWORD)lParam);
            GlobalFree((HGLOBAL)lParam);
            // Use our copy.
            lParam = (LPARAM)hNew;
        }
    }

    // NB CA neglect to send a DDE_INIT, they just start
    // throwing DDE_EXEC's at us so we fake up an init
    // from them to DDEML to get things rolling.
    if (!hwndForward)
    {
        if (!(g_dwAppFlags & DDECONV_NO_INIT))
           g_dwAppFlags = GetDDEAppFlagsFromWindow((HWND)wParam);

        if (g_dwAppFlags & DDECONV_NO_INIT)
        {
            aApp = GlobalAddAtom(c_szService);
            aTopic = GlobalAddAtom(c_szTopic);
            SendMessage(hwnd, WM_DDE_INITIATE, wParam, MAKELPARAM(aApp, aTopic));
            GlobalDeleteAtom(aApp);
            GlobalDeleteAtom(aTopic);
            hwndForward = g_hwndDDEML;
        }
    }

    return _ForwardDDEMsgs(hwnd, hwndForward, uMsg, wParam, lParam, fSend);
}

//---------------------------------------------------------------------------
// Stupid hacks to get various apps installed (read: ATM). These are the people
// who do a FindWindow for Progman and then do dde to it directly.
// These people should not be allowed to write code.
STDAPI_(LRESULT) DDEHandleMsgs(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TraceMsg(TF_DDE, "c.fpwp: Forwarding DDE.");

    SetTimer(hwnd, IDT_DDETIMEOUT, 30*1000, NULL);

    switch (uMsg)
    {
    case WM_DDE_INITIATE:
    case WM_DDE_ACK:
        return _HandleDDEInitiateAndAck(hwnd, uMsg, wParam, lParam);
        
    case WM_DDE_TERMINATE:
        return _HandleDDETerminate(hwnd, uMsg, wParam, lParam);
        
    case WM_DDE_DATA:
        return _HandleDDEForwardBiDi(hwnd, uMsg, wParam, lParam);
        
    case WM_DDE_ADVISE:
    case WM_DDE_UNADVISE:
    case WM_DDE_REQUEST:
    case WM_DDE_POKE:
        return _ForwardDDEMsgs(hwnd, g_hwndDDEML, uMsg, wParam, lParam, FALSE);
        
    case WM_DDE_EXECUTE:
        return _HandleDDEExecute(hwnd, g_hwndDDEML, uMsg, wParam, lParam, FALSE);
    }
    return 0;
}

//---------------------------------------------------------------------------
// Some installers (Wep2) forget to Terminate a conversation so we timeout
// after not getting any dde-messages for a while. If we don't, and you run
// a Wep2 install a second time we think the installer is already talking via
// ddeml so we don't reply from the desktop. Wep2 then thinks Progman isn't
// running, does a WinExec of Progman and hangs waiting to talk to it. Progman
// never replies since it is not the shell. Nasty Nasty Nasty.
STDAPI_(void) DDEHandleTimeout(HWND hwnd)
{
    HWND hwndClient, hwndDDEML;

    TraceMsg(DM_TRACE, "c.hdt: DDE Timeout.");

    KillTimer(hwnd, IDT_DDETIMEOUT);

    // Has everything gone away yet?
    if (g_hwndDDEML && g_hwndClient)
    {
        // Nope. Don't want to forward anymore.
        hwndClient = g_hwndClient;
        hwndDDEML = g_hwndDDEML;
        g_hwndClient = NULL;
        g_hwndDDEML = NULL;
        g_dwAppFlags = DDECONV_NONE;
        // Shutdown our ddeml alter-ego.
        // NB If the client window has already gone away (very likely) it's not a
        // problem, ddeml will skip posting the reply but will still do the
        // disconnect callback.
        PostMessage(hwndDDEML, WM_DDE_TERMINATE, (WPARAM)hwnd, 0);
    }
}





// INTERNAL EXPORT FUNCTION:
// This is for explorer to call to initialize and uninitialize SHELL DDE
// services.
void WINAPI ShellDDEInit(BOOL fInit)
{
    if (fInit)
        InitialiseDDE();
    else
        UnInitialiseDDE();
}
