/****************************************************************************/
/*                                                                          */
/*  PMDDE.C -                                                               */
/*                                                                          */
/*        Windows Program Starter DDE Routines                              */
/*                                                                          */
/****************************************************************************/
#include "progman.h"
#include "dde.h"
#include "uniconv.h"

#define PMPrint(s)  KdPrint(("PROGMAN: ")); \
                         KdPrint(s);            \
                         KdPrint(("\n"));


//
// Define this if you want to know everything about Progman DDE
//

//#define VERBOSE_PROGMANDDE

#ifdef VERBOSE_PROGMANDDE
#define VerbosePrint(s) PMPrint(s)
#else
#define VerbosePrint(s)
#endif



/* DDE window classes */
TCHAR szProgmanDDE[] = TEXT("ProgmanDDE");
TCHAR szAppIconDDE[] = TEXT("AppIconDDE");
TCHAR szAppDescDDE[] = TEXT("AppDescDDE");
TCHAR szAppWDirDDE[] = TEXT("AppWDirDDE");
//
// For compatibility reasons, allow the old WIn3.1 Shell - AppProperties
// DDE connection.
//
TCHAR szAppProperties[] = TEXT("AppProperties");

BOOL bProgmanDDE = TRUE;
BOOL bAppIconDDE = TRUE;
BOOL bAppDescDDE = TRUE;
BOOL bAppWDirDDE = TRUE;

/* application names*/
TCHAR szShell[] = TEXT("Shell");

/* topics*/
TCHAR szAppIcon[] = TEXT("AppIcon");
TCHAR szAppDesc[] = TEXT("AppDescription");
TCHAR szAppWDir[] = TEXT("AppWorkingDir");
TCHAR szSystem[]  = TEXT("System");

/* items*/
TCHAR szGroupList[] = TEXT("Groups");

#define DDE_PROGMAN     0
#define APP_ICON        1
#define APP_DESC        2
#define APP_WDIR        3

#define WCHAR_QUOTE     L'"'

BOOL fForcePoint = FALSE;                /* used for replacement*/
POINT ptForce;

typedef struct _datadde
  {
    unsigned short unused:12,
             fResponse:1,
             fRelease:1,
             reserved:1,
             fAckReq:1;
    WORD  cfFormat;
  } DATADDE;

typedef struct _newicondata
  {
    DATADDE dd;
    DWORD dwResSize;
    DWORD dwVer;
    BYTE iResource;
  } NEWICONDATA;


int NEAR PASCAL myatoi(LPTSTR lp);
LPTSTR NEAR PASCAL SkipWhite(LPTSTR lpsz);
LPTSTR APIENTRY GetOneParameter(LPTSTR lpCmd, WPARAM *lpW, BOOL bSaveQuotes);
LPTSTR NEAR PASCAL GetCommandName(LPTSTR lpCmd, LPTSTR lpFormat, WPARAM *lpW);
HANDLE NEAR PASCAL GetDDECommands(LPTSTR lpCmd, LPTSTR lpFormat);

typedef struct _ddeconversation {
    HWND hwndClient;
    HWND hwndServer;
    DWORD dwType;
    struct _ddeconversation *Next;
} DDECONVERSATION, *PDDECONVERSATION;

PDDECONVERSATION pDdeConversation = NULL;
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitDDEConverstionStruct() -                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID InitDdeConversationStruct()
{
    pDdeConversation = (PDDECONVERSATION)LocalAlloc(LPTR, sizeof(DDECONVERSATION));
    pDdeConversation->Next = NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddDdeConverstion() -                                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID AddDdeConversation(HWND hwndServer, HWND hwndClient, DWORD dwType)
{
    PDDECONVERSATION pT = NULL;

    if (!pDdeConversation) {
        return;
    }
    if (pT = (PDDECONVERSATION)LocalAlloc(LPTR, sizeof(DDECONVERSATION))) {
        pT->hwndServer = hwndServer;
        pT->hwndClient = hwndClient;
        pT->dwType = dwType;
        pT->Next = pDdeConversation->Next;
        pDdeConversation->Next = pT;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RemoveDdeConverstion() -                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID RemoveDdeConversation(HWND hwndServer, HWND hwndClient, DWORD dwType)
{
    PDDECONVERSATION pT;
    PDDECONVERSATION pFree;

    if (!pDdeConversation) {
        return;
    }
    for (pT = pDdeConversation; pT->Next; pT = pT->Next) {
        if ((pT->Next->hwndClient == hwndClient) &&
                      (pT->Next->hwndServer == hwndServer) &&
                      (pT->Next->dwType == dwType)) {
            pFree = pT->Next;
            pT->Next = pT->Next->Next;
            LocalFree(pFree);
            return;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsDDEConverstion() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL IsDdeConversation(HWND hwndServer, HWND hwndClient, DWORD dwType)
{
    PDDECONVERSATION pT;

    if (!pDdeConversation) {
        return(FALSE);
    }
    for (pT = pDdeConversation; pT->Next; pT = pT->Next) {
        if ((pT->Next->hwndClient == hwndClient) &&
                      (pT->Next->hwndServer == hwndServer) &&
                      (pT->Next->dwType == dwType)) {
            return(TRUE);
        }
    }
    return(FALSE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DDEFail() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID APIENTRY DDEFail(HWND hWnd, HWND hwndTo, ATOM aItem)
{
    MPostWM_DDE_ACK(hwndTo, hWnd, ACK_NEG, aItem);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SkipWhite() -                                                           */
/*                                                                          */
/* Returns a pointer to the first non-whitespace character in a string.     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LPTSTR APIENTRY SkipWhite(LPTSTR lpsz)
{
  /* prevent sign extension */
  while (*lpsz && (TUCHAR)*lpsz <= (TUCHAR)TEXT(' '))
      lpsz++;

  return(lpsz);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetCommandName() -                                                      */
/*                                                                          */
/* Extracts an alphabetic string and looks it up in a list of possible
/* commands, returning a pointer to the character after the command and
/* sticking the command index somewhere.
/*
/* lpFormat string syntax:
/*     cmd\0cmd\0...\0\0
/*
/*--------------------------------------------------------------------------*/

LPTSTR APIENTRY GetCommandName(LPTSTR lpCmd, LPTSTR lpFormat, WPARAM *lpW)
{
  register TCHAR chT;
  register WORD iCmd = 0;
  LPTSTR         lpT;

  /* Eat any white space. */
  lpT = lpCmd = SkipWhite(lpCmd);

  /* Find the end of the token. */
  while (IsCharAlpha(*lpCmd))
      lpCmd++;

  /* Temporarily NULL terminate it. */
  chT = *lpCmd;
  *lpCmd = TEXT('\0');

  /* Look up the token in a list of commands. */
  *lpW = (DWORD_PTR)0xFFFF;
  while (*lpFormat) {
      if (!lstrcmpi(lpFormat, lpT)) {
          *lpW = iCmd;
          break;
      }
      iCmd++;
      while (*lpFormat++)
          ;
  }

  *lpCmd = chT;

  return(lpCmd);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ValidateFileName() -                                                    */
/*                                                                          */
/*  Checks if the given filename really exists.  The filename passed        */
/*  in will have quotes around it, so this routine removes the quotes       */
/*  first.  Returns TRUE if it is a valid file.                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL APIENTRY ValidateFileName (LPTSTR lpFileName)
{
    TCHAR           chT;
    WORD            wLen;
    LPTSTR          lpEnd;
    BOOL            bResult = FALSE;
    HANDLE          hFile;
    WIN32_FIND_DATA fd;

    // Save the last character (better be a quote), and move
    // the terminating NULL one character forward.
    wLen = (WORD)lstrlen (lpFileName);
    lpEnd = lpFileName + wLen - 1;
    chT = *lpEnd;

    // MarkTa fix for spaces at the end of a filename
    // Remove the spaces by moving the quote forward.
    while (*(lpEnd-1) == TEXT(' '))
        lpEnd--;

    *lpEnd = TEXT('\0');

    // Test if this is a file.
    hFile = FindFirstFile(lpFileName+1, &fd);

    if (hFile != INVALID_HANDLE_VALUE)
       {
       FindClose (hFile);
       bResult = TRUE;
       }


    // Put back the character we removed eariler, and NULL terminate
    *lpEnd = chT;
    *(lpEnd+1) = TEXT('\0');

    return (bResult);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetOneParameter() -                                                     */
/*                                                                          */
/*  Reads a parameter out of a string removing leading and trailing whitespace.
/*  Terminated by , or ).  ] [ and ( are not allowed.  Exception: quoted
/*  strings are treated as a whole parameter and may contain []() and ,.
/*  Places the offset of the first character of the parameter into some place
/*  and NULL terminates the parameter.
/*
/*--------------------------------------------------------------------------*/

LPTSTR APIENTRY GetOneParameter(LPTSTR lpCmd, WPARAM *lpW, BOOL bSaveQuotes)
{
    LPTSTR          lpT;
    LPTSTR          lpTemp;
    TCHAR           chT;
    WORD            wLen;
    LPTSTR          lpEnd;

    switch (*lpCmd) {
    case TEXT(','):
        *lpW = (DWORD_PTR)lpCmd;
        *lpCmd++ = 0;                /* comma: becomes a NULL string */
        break;

    case TEXT('"'):                         /* quoted string... trim off " */

        VerbosePrint (("Quoted parameter before parsing: %S", lpCmd));
        VerbosePrint (("bSaveQuotes = %d", bSaveQuotes));

        if (bSaveQuotes)
           {
           // Set the beginning marker at the quote then increment.
           *lpW = (DWORD_PTR)lpCmd;
           ++lpCmd;
           }
        else
           {
           // Increment first to skip the quote
           ++lpCmd;
           *lpW = (DWORD_PTR)lpCmd;
           }

        while (*lpCmd && *lpCmd != TEXT('"'))
            lpCmd++;
        if (!*lpCmd)
            return(NULL);

        lpTemp = lpCmd;  // lpTemp should point at the quote
        lpCmd++;

        if (bSaveQuotes)
           {
           chT = *lpCmd;
           *lpCmd = TEXT('\0');

           VerbosePrint (("Checking %S to confirm that it really is a file.", *lpW));
           if (!ValidateFileName ((LPTSTR) *lpW))
              {
              // file doesn't exist.  Remove the quotes.
              VerbosePrint (("No, this isn't a file.  Removing the quotes."));
              *lpW = *lpW + sizeof (TCHAR);
              lpTemp = (LPTSTR)(*lpW) + lstrlen((LPTSTR) (*lpW)) - 1;
              *lpTemp = TEXT('\0');
              VerbosePrint (("New string after removing the quotes: %S", *lpW));
              }
            else
              {

              //
              // The quoted filename is valid, so now we want to test if
              // the quotes are really necessary.  To do this, remove
              // the quotes, and then call CheckEscapes to look for funny
              // characters.
              //

              VerbosePrint (("Yes, %S is a file.  Checking if we really need these quotes", *lpW));
              SheRemoveQuotes ((LPTSTR) *lpW);
              CheckEscapes ((LPTSTR) *lpW, lstrlen ((LPTSTR) *lpW) + 2);
              VerbosePrint (("After checking quotes we have %S", *lpW));
              }

           *lpCmd = chT;
           }
        else
           *lpTemp = TEXT(' ');


        while (*lpCmd && *lpCmd != TEXT(')') && *lpCmd != TEXT(','))
            lpCmd++;
        if (!*lpCmd)
            return(NULL);

        if (*lpCmd == TEXT(','))
           {
           *lpCmd = TEXT('\0');
           lpCmd++;
           }
        else
           *lpCmd = TEXT('\0');


        // Remove the space at the end of the string if the parser
        // added it.
        wLen = (WORD)lstrlen ((LPTSTR)(*lpW));
        lpEnd = (LPTSTR)(*lpW) + wLen - 1;
        if (*lpEnd == TEXT (' '))
           *lpEnd = TEXT('\0');

        VerbosePrint (("Quoted parameter after parsing: %S", *lpW));
        break;

    case TEXT(')'):
        return(lpCmd);                /* we ought not to hit this */

    case TEXT('('):
    case TEXT('['):
    case TEXT(']'):
        return(NULL);                 /* these are illegal */

    default:
        lpT = lpCmd;
        *lpW = (DWORD_PTR)lpT;

        while (*lpCmd && *lpCmd != TEXT(',') && *lpCmd != TEXT(')')) {
            /* Check for illegal characters. */
            if (*lpCmd == TEXT(']') || *lpCmd == TEXT('[') || *lpCmd == TEXT('(') )
                return(NULL);

            /* Remove trailing whitespace */
            /* prevent sign extension */
            if ((TUCHAR)*lpCmd > (TUCHAR)TEXT(' '))
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
        lpT = CharNext(lpT);
        *lpT = TEXT('\0');

        break;
    }

    /* Return next unused character. */
    return(lpCmd);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetDDECommands() -                                                      */
/*                                                                          */
/*  Called with: far pointer to a string to parse and a far pointer to a
/*  list of sz's containing the allowed function names.
/*  The function returns a global handle to an array of words containing
/*  one or more command definitions.  A command definition consists of
/*  a command index, a parameter count, and that number of offsets.  Each
/*  offset is an offset to a parameter in lpCmd which is now zero terminated.
/*  The list of command is terminated with -1.
/*  If there was a syntax error the return value is NULL.
/*  Caller must free block.
/*
/*--------------------------------------------------------------------------*/

HANDLE NEAR PASCAL GetDDECommands(LPTSTR lpCmd, LPTSTR lpFormat)
{
  register WORD     cParm;
  WORD              cCmd = 0;
  register HANDLE   hDDECmds;
  DWORD_PTR *       lpW = NULL;
  BOOL              bAddItem = FALSE;

  /* Will allow up to 128 words, 64 single command (less with parms). */
  /*
   * Now these are 32bit, since the offset is now replaced by the
   * full pointer which is 32 bit.
   *
   * (And if they are 64bit, they get even bigger.)
   */
  hDDECmds = GlobalAlloc(GHND, 128 * sizeof(DWORD_PTR));
  if (!hDDECmds)
      return(NULL);

  /* Get pointer to array. */
  lpW = (DWORD_PTR *)GlobalLock(hDDECmds);
  while (*lpCmd) {
      /* Skip leading whitespace. */
      lpCmd = SkipWhite(lpCmd);

      /* Are we at a NULL? */
      if (!*lpCmd) {
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
      lpCmd = GetCommandName(lpCmd, lpFormat, lpW);
      if (*lpW == (DWORD_PTR)0xFFFF)
          goto GDEErrExit;

      if (*lpW == 1)
         bAddItem = TRUE;

      lpW++;

      /* Start with zero parms. */
      cParm = 0;
      lpCmd = SkipWhite(lpCmd);

      /* Check for opening '(' */
      if (*lpCmd == TEXT('(')) {
          lpCmd++;

          /* Skip white space and then find some parameters (may be none). */
          lpCmd = SkipWhite(lpCmd);

          while (*lpCmd != TEXT(')')) {
              if (!*lpCmd)
                  goto GDEErrExit;

              /* Get the parameter. */
              if (bAddItem && (cParm == 0 || cParm == 2 || cParm == 6))
                 {
                 // In this case, we are working with filenames of
                 // the command line, icon path, or default directory of
                 // the AddItem command.
                 // We don't want to strip the quotes if they exist.
                 if (!(lpCmd = GetOneParameter(lpCmd, lpW + (++cParm), TRUE)))
                     goto GDEErrExit;
                 }
              else
                 {
                 // This is for every other parameter.  The quotes will be
                 // stripped.
                 if (!(lpCmd = GetOneParameter(lpCmd, lpW + (++cParm), FALSE)))
                     goto GDEErrExit;
                 }

              /* HACK: Did GOP replace a ')' with a NULL? */
              if (!*lpCmd)
                  break;

              /* Find the next one or ')' */
              lpCmd = SkipWhite(lpCmd);
          }

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
  *lpW = (DWORD_PTR)0xFFFF;

  GlobalUnlock(hDDECmds);
  return(hDDECmds);

GDEErrExit:
  GlobalUnlock(hDDECmds);
  GlobalFree(hDDECmds);
  return(NULL);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsParameterANumber() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL APIENTRY IsParameterANumber(LPTSTR lp)
{
  while (*lp) {
      if (*lp < TEXT('0') || *lp > TEXT('9'))
          return(FALSE);
      lp++;
  }
  return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  myatoi() -                                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int APIENTRY myatoi(LPTSTR lp)
{
  register int        i = 0;

  while (*lp >= TEXT('0') && *lp <= TEXT('9')) {
      i *= 10;
      i += (int)(*lp-TEXT('0'));
      lp++;
  }
  return(i);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ExecuteHandler() -                                                      */
/*                                                                          */
/* Handles WM_DDE_EXECUTE messages...                                       */
/*                                                                          */
/*  return 0 if it fails                                                    */
/*         1 if it succeeds                                                 */
/*         2 if it succeeds and the command was ExitProgman                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD APIENTRY ExecuteHandler(HANDLE hString)
{
  register HWND     hwndT;
  LPTSTR             lpT;
  LPTSTR             lpString;
  register HANDLE   hCmd;
  DWORD_PTR *       lpwCmd;
  DWORD             dwRet = 0;
  PGROUP            pGroup;
  LPGROUPDEF        lpgd;
  WCHAR		    lpFmtinit[] = TEXT("CreateGroup#AddItem#DeleteGroup#ExitProgman#ShowGroup#DeleteItem#ReplaceItem#Reload#ChangeINIFile#");
  LPTSTR	    lpFmt ;

  lpFmt = lpFmtinit;
  /* Lock the command string. */
  lpString = (LPTSTR)GlobalLock(hString);
  if(!lpString)
      return(0);

  VerbosePrint(("Execute Handler received: %S", lpString));

#ifdef DEBUG_PROGMAN_DDE
  {
  TCHAR szDebug[300];

  wsprintf (szDebug, TEXT("%d   PROGMAN:   Execute Handler recived:  %s\r\n"),
            GetTickCount(), lpString);
  OutputDebugString(szDebug);
  }
#endif

  bInDDE = TRUE;

  /* Parse the commands. */

// the following line does not work on build 363! TEXT string is truncated
// after "CreateGroup".
  //hCmd = GetDDECommands(lpString, (LPTSTR)TEXT("CreateGroup\0AddItem\0DeleteGroup\0ExitProgman\0ShowGroup\0DeleteItem\0ReplaceItem\0Reload\0ChangeINIFile\0"));


  // substitute nulls for '#'
  while (*lpFmt) {
      if (*lpFmt == TEXT('#'))
	 *lpFmt = (TCHAR) 0;
      lpFmt++ ;
  }
  lpFmt = lpFmtinit; // reset the pointer back to the begining
  hCmd = GetDDECommands(lpString, lpFmt) ;
  if (!hCmd)
      goto DEHErrExit1;

  /* Lock the list of commands and parameter offsets. */
  lpwCmd = (DWORD_PTR *)GlobalLock(hCmd);

  /* Execute each command. */
  while (*lpwCmd != (DWORD_PTR)0xFFFF) {

      switch (*lpwCmd++) {
      case 0:
      {
          INT   cParm;
          INT   nCommonGrp = -1;
          LPTSTR lpCommonGrp = NULL;
          LPTSTR lpGroupName;


          /* [ CreateGroup ( groupname [, groupfile] [, common_group_flag] ) ] */
          /*
           * The groups are now in the registry thus no more group files
           * and therefore this is now replaced by
           *   [ CreateGroup ( groupname ) ]
           * The groupfile is specified is ignored. This will cause an error
           * for compatability reasons.
           */

          /*
           * A new optional parameter is added to specify whether to create
           * a Common group  or a Personal group.
           *     1 for Common Group
           *     0 for Personal Group
           * Only users with administrative rights can create/delete Common
           * groups. The default if this parameter is not specified is:
           *     Common group if user has admin rights
           *     Personal group if not
           */

          /* Make sure that we have 1, 2 or 3 parameters, ignore the 2nd one
           * if it represents a groupfile name.
           */
          cParm = (INT)*lpwCmd++;
          if ((cParm < 1) || (cParm > 3))
              goto DEHErrExit;

          /* Get a pointer to the group name. */
          lpT = (LPTSTR) *lpwCmd++;

          VerbosePrint (("CreateGroup received: %S", lpT));

          if (cParm == 3) {
              // skip group file parameter
              lpwCmd++;
          }


          if (cParm > 1) {
              //
              // Test if the 2nd parameter is a groupfile name or the
              // common group flag
              //

              if (IsParameterANumber((LPTSTR)*lpwCmd)) {

                  // get the common group flag
                  if ((nCommonGrp = myatoi((LPTSTR) *lpwCmd++)) &&
                                              !AccessToCommonGroups)
                      goto DEHErrExit;

              }
              else {
                  lpwCmd++;
              }
          }

          /* Search for the group... if it already exists, activate it. */
          hwndT = GetWindow(hwndMDIClient, GW_CHILD);
          while (hwndT) {
              /* Skip icon titles. */
              if (!GetWindow(hwndT, GW_OWNER)) {

                  /* Compare the group title with the request. */
                  pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
                  if (lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup)) {

                      lpGroupName = (LPTSTR) PTR(lpgd, lpgd->pName);
                      GlobalUnlock(pGroup->hGroup);
                      if (!lstrcmpi(lpT, lpGroupName)) {
                          BOOL bContinueSearch = TRUE;

                          //
                          // First case is the app didn't request
                          // a specific type of group.
                          //

                          if (nCommonGrp == -1) {

                              //
                              // If the user has access to
                              // common groups (thus also has
                              // access to personal groups), or
                              // the existing group is personal,
                              // then we are finished.
                              //

                              if (AccessToCommonGroups || !pGroup->fCommon) {
                                  bContinueSearch = FALSE;
                              }
                          }

                          //
                          // Second case the app requested
                          // a common group match.
                          //

                          else if (nCommonGrp == 1) {

                              //
                              // If user has access to the common groups
                              // and the group found is common, then we
                              // are finished.
                              //

                              if (AccessToCommonGroups && pGroup->fCommon) {
                                  bContinueSearch = FALSE;
                              }
                          }

                          //
                          // Third case is the app requested
                          // a personal group match.
                          //

                          else if (nCommonGrp == 0) {

                              //
                              // Check to see if the group is also
                              // personal.
                              //

                              if (!pGroup->fCommon) {
                                 bContinueSearch = FALSE;
                              }
                          }

                          if (bContinueSearch) {
                              hwndT = GetWindow(hwndT, GW_HWNDNEXT);
                              continue;
                          } else {

                              VerbosePrint (("CreateGroup: Activing group"));
                              BringWindowToTop(hwndT);
                              break;
                          }
                      }
                  }
              }
              hwndT = GetWindow(hwndT, GW_HWNDNEXT);
          }

          /* If we didn't find it, add it. */
          if (!hwndT) {
              TCHAR szTemp[MAXITEMPATHLEN+1];         // Group name.

              //
              // If the app does care what type of group to create,
              // the default is to create a common group if the
              // user has admin privilages.  Otherwise they get a
              // personal group.
              //

              if (nCommonGrp == -1) {

                  if (AccessToCommonGroups) {
                      nCommonGrp = 1;
                  } else {
                      nCommonGrp = 0;
                  }
              }

              lstrcpy(szTemp, lpT);
              VerbosePrint (("CreateGroup: Creating new group"));
              CreateNewGroup(szTemp, (nCommonGrp == 1));
          }

          break;
      }

      case 1:
      {
          INT      cParm;
          WORD      iIconIndex;
          POINT     pt;
          LPPOINT   lppt;
          DWORD     dwFlags = CI_SET_DOS_FULLSCRN;
          BOOL      fMinimize;
          WORD      wHotKey;
          TCHAR      szExpPath[MAXITEMPATHLEN+1];
          TCHAR      szExpDir[MAXITEMPATHLEN+1];
          HICON     hIcon;
    	  TCHAR     szT[MAX_PATH];
	      WORD      id;

          /* [ AddItem (command,name,icopath,index,pointx,pointy,
                            defdir,hotkey,fminimize) ] */
          //
          // pActiveGroup is non NULL when the user
          // has an item or group properties dialog up in
          // progman i.e. the user is working in progman
          // while some other app is doing DDE.
          // We can't have both play on the same group at
          // the same time.
          // johannec 5-13-93 bug 9513
          //
          if (pCurrentGroup == pActiveGroup) {
              PMPrint (("AddItem:  DDE converstation started with the group you have open! Exiting."));
              goto DEHErrExit;
          }

          /* There must be at least a command string. */
          if ((cParm = (INT)*lpwCmd++) < 1) {
              PMPrint (("AddItem:  No command string!"));
              goto DEHErrExit;
          }

          /* Make sure we have a reasonable number of parameters. */
          if (cParm == 5 || cParm > 10) {
              PMPrint (("AddItem:  Not enough or too many parameters!"));
              goto DEHErrExit;
          }

          /* If all else fails, there must be a command string! */
          lpT = (LPTSTR) *lpwCmd++;
          if (!*lpT) {
              PMPrint (("AddItem:  Null pointer for command string!"));
              goto DEHErrExit;
          }

          VerbosePrint (("AddItem:  szPathField = %S", lpT));
          lstrcpy(szPathField, lpT);
          lstrcpy(szExpPath, szPathField);
          DoEnvironmentSubst(szExpPath, CharSizeOf(szExpPath));

          VerbosePrint (("AddItem:  Expanded path = %S", szExpPath));

          StripArgs(szExpPath);

          VerbosePrint (("AddItem:  Path after StripArgs call = %S", szExpPath));

          if (*szExpPath != WCHAR_QUOTE)
            CheckEscapes(szExpPath, CharSizeOf(szExpPath));

          VerbosePrint (("AddItem:  Path after CheckEscapes call = %S", szExpPath));

          /* Look for the name field. */
          szNameField[0] = TEXT('\0');
          if (cParm > 1) {
              /* Get the next parameter. */
              lpT = (LPTSTR)*lpwCmd++;

              if (lstrlen (lpT) > MAXITEMNAMELEN)
                  lpT[MAXITEMNAMELEN] = TEXT('\0');

              lstrcpy(szNameField, lpT);
          }

          /* If none given, generate one from the command. */
          if (szNameField[0] == TEXT('\0')) {
              // NB Use the unexpanded path.
              BuildDescription(szNameField, szPathField);
          }

          VerbosePrint (("AddItem:  Name field will be: %S", szNameField));

          /* Look for the icon's path. */
          szIconPath[0] = TEXT('\0');
          if (cParm > 2) {
              lpT = (LPTSTR)*lpwCmd++;
              lstrcpy(szIconPath,lpT);

              VerbosePrint(("AddItem:  An icon path was given of: %S", szIconPath));
              StripArgs(szIconPath);
              // I am removing this call to CheckEscapes because
              // the filenames could now have quotes around them.
              // This call will automaticly add another set of quotes
              // thus causing the wrong icon to be displayed.
              // ericflo 2/25/94
              //CheckEscapes(szIconPath, CharSizeOf(szIconPath));
              VerbosePrint (("AddItem:  After stripping args the icon path = %S", szIconPath));
          }
          else
              szIconPath[0] = TEXT('\0');

          /* Get the icon index. */
          if (cParm > 3) {
              lpT = (LPTSTR)*lpwCmd++;
              iIconIndex = (WORD)myatoi(lpT);
          }
          else
              iIconIndex = 0;

          if (iIconIndex >= 666) {
              iIconIndex -= 666;
          }
	  else {
	      dwFlags |= CI_ACTIVATE;
	  }

          //
          // If there is no icon path, check if we have an executable associated
          // with the command path.
          //
	  if (!*szIconPath) {
    	      FindExecutable(szExpPath, szExpDir, szT);
    	      if (!*szT) {
        	  dwFlags |= CI_NO_ASSOCIATION;
    	      }
	  }
	  else {
    	      //
    	      // convert the icon index to the icon id which is what progman
    	      // uses.
    	      //
    	      lstrcpy(szT, szIconPath);
    	      id = iIconIndex;
    	      hIcon = ExtractAssociatedIcon(hAppInstance, szT, &id);
    	      if (lstrcmpi(szT, szIconPath)) {
        	  id = iIconIndex;
    	      }
	  }

          VerbosePrint (("AddItem:  Icon index = %d", id));

          /* Get the point :)  Note x cannot be specified alone. */
          if (cParm > 4) {
              lpT = (LPTSTR)*lpwCmd++;
              if (*lpT) {
                  pt.x = myatoi(lpT);
              }
              else {
                  pt.x = -1;
              }
              lpT = (LPTSTR)*lpwCmd++;
              if (*lpT) {
                  pt.y = myatoi(lpT);
              }
              else {
                  pt.x = -1;
              }
              lppt = (LPPOINT)&pt;
          }
          else
              lppt = (LPPOINT)NULL;

          if (fForcePoint) {
              lppt = &ptForce;
              fForcePoint = FALSE;
          }

          /* look to see if there is a default directory
           */
          if (cParm > 6) {
              lpT = (LPTSTR)*lpwCmd++;
              VerbosePrint (("AddItem:  Given this default direcotry: %S", lpT));
              lstrcpy(szDirField, lpT);
              lstrcpy(szExpDir, lpT);
              DoEnvironmentSubst(szExpDir, CharSizeOf(szExpDir));
              StripArgs(szExpDir);
              VerbosePrint(("AddItem:  After expanding and strip args, we have: %S", szExpDir));
          }
          else {
              szDirField[0] = TEXT('\0');
          }

          // If the directory is null then use the path bit
          // of the command line.  (Unexpanded)
          if (!*szDirField) {
              GetDirectoryFromPath(szPathField, szDirField);
              if (*szDirField) {
                 CheckEscapes (szDirField, MAXITEMPATHLEN+1);
              }
          }

          VerbosePrint (("AddItem:  Default directory is: %S", szDirField));

          /* hotkey
           */
          if (cParm > 7) {
              lpT = (LPTSTR)*lpwCmd++;
              wHotKey = (WORD)myatoi(lpT);
          }
          else
              wHotKey = 0;

          /* fminimize
           */
          if (cParm > 8) {
              lpT = (LPTSTR)*lpwCmd++;
              fMinimize = myatoi(lpT);
          }
          else
              fMinimize = FALSE;

          /* fseparateVDM
           */
          if (cParm > 9) {
              lpT = (LPTSTR)*lpwCmd++;
              if (myatoi(lpT)) {
                  dwFlags |= CI_SEPARATE_VDM;
                  VerbosePrint (("AddItem:  Separate VDM flag specified"));
              }
          }

          VerbosePrint (("AddItem: Results passed to CreateNewItem are:"));
          VerbosePrint (("         Name Field = %S", szNameField));
          VerbosePrint (("         Path Field = %S", szPathField));
          VerbosePrint (("         Icon Path  = %S", szIconPath));
          VerbosePrint (("         Dir Field  = %S", szDirField));
          VerbosePrint (("         Hot Key    = %d", wHotKey));
          VerbosePrint (("         Minimize   = %d", fMinimize));
          VerbosePrint (("         id         = %d", id));
          VerbosePrint (("         Icon Index = %d", iIconIndex));
          VerbosePrint (("         Flags      = %lx", dwFlags));


          /* Now add the new item!!! */
          if (!CreateNewItem(pCurrentGroup->hwnd,
                      szNameField, szPathField,
                      szIconPath, szDirField, wHotKey, fMinimize,
                      id, iIconIndex, NULL, lppt, dwFlags))
              goto DEHErrExit;

          // Update scrollbars.
          if ((bAutoArrange) && (!bAutoArranging))
              ArrangeItems(pCurrentGroup->hwnd);
          else if (!bAutoArranging)
              CalcGroupScrolls(pCurrentGroup->hwnd);

          break;
      }

      case 2:
      {
          int cParm;
          BOOL fCommonGrp = FALSE;
          BOOL fCommonDefaulted = FALSE;
          LPTSTR lpGroupName;
          HWND hwndPersGrp = NULL;

          /* [ DeleteGroup (group_name [, common_group_flag] ) ] */

          /*
           * A new optional parameter is added to specify whether to delete
           * a Common group  or a Personal group.
           *     1 for Common Group
           *     0 for Personal Group
           * Only users with administrative rights can create/delete Common
           * groups. The default if this parameter is not specified is:
           *     Common group if user has admin rights
           *     Personal group if not
           */

          /* Make sure that we have 1 or 2 parameter. */
          cParm = (int)*lpwCmd++;

          if ((cParm < 1) || (cParm > 2))
              goto DEHErrExit;

          /* Get a pointer to the group name. */
          lpT = (LPTSTR) *lpwCmd++;

          if (cParm == 2) {
              //
              // Get the common group flag. The User must have Write and
              // Delete access to the common groups.
              //
              if ((fCommonGrp = myatoi((LPTSTR) *lpwCmd++)) &&
                                              !AccessToCommonGroups)
                   goto DEHErrExit;
          }
          else if (AccessToCommonGroups) {
              //
              // The default for a user with Write access rights to the Common
              // Groups is deleting a common group.
              //
              fCommonGrp = TRUE;
              fCommonDefaulted = TRUE;
          }

          /* Search for the group... */
          hwndT = GetWindow(hwndMDIClient, GW_CHILD);
          while (hwndT) {
              /* Skip icon titles. */
              if (!GetWindow(hwndT, GW_OWNER)) {

                  /* Compare the group title with the request. */
                  pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
                  if (lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup)) {

                      lpGroupName = (LPTSTR) PTR(lpgd, lpgd->pName);
                      GlobalUnlock(pGroup->hGroup);
                      if (!lstrcmpi(lpT, lpGroupName)) {
                          if ((fCommonGrp && !pGroup->fCommon) ||
                              (!fCommonGrp && pGroup->fCommon) ) {

                              //
                              // If the app did not specify common nor personal
                              // group and we defaulted to common group (because
                              // the user is an admin), then don't ignore the
                              // personal group that was found. If no common group
                              // is found, we'll default to the personal group.
                              //  5-7-93 johannec bug ????
                              //
                              if (fCommonGrp && fCommonDefaulted) {
                                  hwndPersGrp = hwndT;
                              }

                              hwndT = GetWindow(hwndT, GW_HWNDNEXT);
                              continue;
                          }
                          //
                          // pActiveGroup is non NULL when the user
                          // has an item or group properties dialog up in
                          // progman i.e. the user is working in progman
                          // while some other app is doing DDE.
                          // We can't have both play on the same group at
                          // the same time.
                          // johannec 5-13-93 bug 9513
                          //
                          if (pGroup == pActiveGroup) {
                              goto DEHErrExit;
                          }
                          DeleteGroup(hwndT);
                          break;
                      }
                  }
              }
              hwndT = GetWindow(hwndT, GW_HWNDNEXT);
          }

          /* If we didn't find it, report the error. */
          if (!hwndT) {
              if (hwndPersGrp) {
                  //
                  // If a personal group was found instead of the common group
                  // delete it.
                  //

                  pGroup = (PGROUP)GetWindowLongPtr(hwndPersGrp, GWLP_PGROUP);

                  //
                  // pActiveGroup is non NULL when the user
                  // has an item or group properties dialog up in
                  // progman i.e. the user is working in progman
                  // while some other app is doing DDE.
                  // We can't have both play on the same group at
                  // the same time.
                  // johannec 5-13-93 bug 9513
                  //
                  if (pGroup == pActiveGroup) {
                      goto DEHErrExit;
                  }
                  DeleteGroup(hwndPersGrp);

              } else {
                  goto DEHErrExit;
              }
          }
          break;
      }

      case 3:
          /* [ ExitProgman (bSaveGroups) ] */

          if (bExitWindows)
              goto DEHErrExit;

          /* Make sure that we have 1 parameter. */
          if (*lpwCmd++ != 1)
              goto DEHErrExit;

          /* Get a pointer to the parm. */
          lpT = (LPTSTR) *lpwCmd++;

          bSaveSettings = FALSE;
          if (*lpT == TEXT('1'))
              WriteINIFile();

          //
          // The 2 is a magic return value inside of the
          // DDEMsgProc routine.
          //

          dwRet = 2;
          goto DEHErrExit;
          break;

      case 4:
      {
          INT cParm;
          int iShowCmd;
          BOOL fCommonGrp = FALSE;
          BOOL fCommonDefaulted = FALSE;
          HWND hwndPersGrp = NULL;
          TCHAR szT[MAXKEYLEN + 1];
          TCHAR szCommonGroupSuffix[MAXKEYLEN + 1];
          WINDOWPLACEMENT wp;

          /* [ ShowGroup (group_name, wShowParm [, fCommonGroup] ) ] */

          /*
           * A new optional parameter is added to specify whether to show
           * a Common group  or a Personal group.
           *     1 for Common Group
           *     0 for Personal Group
           * Only users with administrative rights can create/delete Common
           * groups. The default if this parameter is not specified is:
           *     Common group if user has admin rights
           *     Personal group if not
           */

          /* Make sure that we have 2 or 3 parameters. */
          cParm = (INT)*lpwCmd++;
          if ((cParm < 2) || (cParm > 3))
              goto DEHErrExit;

          /* Get a pointer to the group name. */
          lpT = (LPTSTR) *lpwCmd++;

          VerbosePrint (("ShowGroup:  Called with %S", lpT));

          iShowCmd = myatoi((LPTSTR) *lpwCmd++);

          if (cParm == 3) {
              //
              // get the common group flag
              //
              fCommonGrp = myatoi((LPTSTR) *lpwCmd++);
          }
          else if (AccessToCommonGroups) {
              //
              // The default for a user with administrative rights is Common
              // Groups.
              //
              fCommonGrp = TRUE;
              fCommonDefaulted = TRUE;
          }

          /* Search for the group... */
          hwndT = GetWindow(hwndMDIClient, GW_CHILD);
          while (hwndT) {
              //
              // Skip icon titles.
              //
              if (GetWindow(hwndT, GW_OWNER)) {
                   hwndT = GetWindow(hwndT, GW_HWNDNEXT);
                   continue;
              }

              /* Compare the group title with the request. */
              pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
              if (lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup)) {

                  lstrcpy(szT, (LPTSTR) PTR(lpgd, lpgd->pName));
                  GlobalUnlock(pGroup->hGroup);
                  if (!lstrcmpi(lpT, szT)) {

                      if ((fCommonGrp && !pGroup->fCommon) ||
                          (!fCommonGrp && pGroup->fCommon) ) {
                          //
                          // If the app did not specify common nor personal
                          // group and we defaulted to common group (because
                          // the user is an admin), then don't ignore the
                          // personal group that was found. If no common group
                          // is found, we'll default to the personal group.
                          //  5-7-93 johannec bug ????
                          //
                          if (fCommonGrp && fCommonDefaulted) {
                              hwndPersGrp = hwndT;
                          }

                          hwndT = GetWindow(hwndT, GW_HWNDNEXT);
                          continue;
                      }
                      ShowWindow(hwndT, iShowCmd);
                      //
                      // if the group is common and not being minimized
                      // then must add the common suffix to the group
                      // window title. If the group is being minimized
                      // then make sure the common suffix is not there.
                      //
                      if (fCommonGrp) {
                          wp.length = sizeof(WINDOWPLACEMENT);
                          GetWindowPlacement(hwndT, &wp);
                          if (wp.showCmd != SW_SHOWMINIMIZED &&
                              wp.showCmd != SW_MINIMIZE &&
                              wp.showCmd != SW_SHOWMINNOACTIVE ) {
                              LoadString(hAppInstance, IDS_COMMONGRPSUFFIX,
                                             szCommonGroupSuffix,
                                             CharSizeOf(szCommonGroupSuffix));
                              lstrcat(szT, szCommonGroupSuffix);
                          }
                          SetWindowText(hwndT, szT);
                      }
                      SendMessage(hwndT, WM_ACTIVATE, 1, 0);
                      break;
                  }
              }
              hwndT = GetWindow(hwndT, GW_HWNDNEXT);
          }

          /* If we didn't find it, report the error. */
          if (!hwndT) {
              if (hwndPersGrp) {
                  //
                  // If a personal group was found instead of the common group
                  // show it.
                  //
                  ShowWindow(hwndPersGrp, iShowCmd);
                  SendMessage(hwndPersGrp, WM_ACTIVATE, 1, 0);
              }
              else {
                  goto DEHErrExit;
              }
          }
          break;
      }
      case 6:

          /* [ ReplaceItem (item_name) ] */
          fForcePoint = TRUE;
          ptForce.x = -1;     // in case we don't really find the item
          ptForce.y = -1;

          /* fall thru */

      case 5:
      {
          PITEM pItem;
          LPITEMDEF lpid;

          /* [ DeleteItem (item_name) ] */

          //
          // pActiveGroup is non NULL when the user
          // has an item or group properties dialog up in
          // progman i.e. the user is working in progman
          // while some other app is doing DDE.
          // We can't have both play on the same group at
          // the same time.
          // johannec 5-13-93 bug 9513
          //
          if (pCurrentGroup == pActiveGroup) {
              goto DEHErrExit;
          }

          /* exactly one parameter
           */
          if (*lpwCmd++ != 1)
              goto DEHErrExit;

          lpT = (LPTSTR) *lpwCmd++;

          lpgd = LockGroup(pCurrentGroup->hwnd);
          if (!lpgd)
              goto DEHErrExit;

          for (pItem = pCurrentGroup->pItems; pItem; pItem = pItem->pNext) {
              lpid = ITEM(lpgd,pItem->iItem);
              if (!lstrcmpi((LPTSTR) PTR(lpgd, lpid->pName),lpT)) {
                  ptForce.x = pItem->rcIcon.left;
                  ptForce.y = pItem->rcIcon.top;
                  UnlockGroup(pCurrentGroup->hwnd);
                  DeleteItem(pCurrentGroup,pItem);
                  break;
              }
          }
          if (!pItem) {
              UnlockGroup(pCurrentGroup->hwnd);
              goto DEHErrExit;
          }

          break;
      }

      case 7:
      {
          int cParm;
          BOOL fAll;
          BOOL fCommonGrp = FALSE;

          /* [ Reload [(groupname [, common_group_flag] )] ] */

          /*
           * A new optional parameter is added to specify whether to reload
           * a Common group  or a Personal group.
           *     1 for Common Group
           *     0 for Personal Group
           * Only users with administrative rights can create/delete Common
           * groups. The default if this parameter is not specified is:
           *     Common group if user has admin rights
           *     Personal group if not
           */

          cParm = (int)*lpwCmd++;

          if (!cParm)
              fAll = TRUE;
          else if ((cParm == 1) || (cParm == 2))
              fAll = FALSE;
          else
              goto DEHErrExit;

          if (fAll) {
              HWND hwndT;

              ShowWindow(hwndMDIClient, SW_HIDE);
              ValidateRect(hwndProgman,NULL);

              /* unload all the groups!
               */
              for (hwndT = GetWindow(hwndMDIClient, GW_CHILD);
                   hwndT;
                   hwndT = GetWindow(hwndMDIClient, GW_CHILD)) {

                  /* Skip icon titles. */
                  while (GetWindow(hwndT, GW_OWNER)) {
                      hwndT = GetWindow(hwndT,GW_HWNDNEXT);
                      if (!hwndT)
                          break;
                  }

                  if (hwndT)
                      UnloadGroupWindow(hwndT);
              }

              LoadAllGroups();
              ShowWindow(hwndMDIClient,SW_SHOW);
          }
          else {
              TCHAR szT[120];
              WORD idGroup;
              HWND hwndT;

              /* get the name to reload
               */
              lpT = (LPTSTR) *lpwCmd++;

              if (cParm == 2) {
                  //
                  // Get the common group flag. The User must have Write
                  // access to the reload common groups.
                  //
                  if ((fCommonGrp = myatoi((LPTSTR) *lpwCmd++)) &&
                                              !AccessToCommonGroups)
                      goto DEHErrExit;
              }
              else if (AccessToCommonGroups) {
                  //
                  // The default for a user with administrative rights is Common
                  // Groups.
                  //
                  fCommonGrp = TRUE;
              }

              /* search for it
               */
              for (hwndT = GetWindow(hwndMDIClient, GW_CHILD);
                   hwndT;
                   hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {

                  /* Skip icon titles. */
                  if (GetWindow(hwndT, GW_OWNER))
                      continue;

                  /* Compare the group title with the request. */
                  pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
                  if (lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup)) {

                      lstrcpy(szT, (LPTSTR) PTR(lpgd, lpgd->pName));
                      GlobalUnlock(pGroup->hGroup);

                      if (lstrcmpi(lpT, szT))
                          continue;

                      if ((fCommonGrp && !pGroup->fCommon) ||
                          (!fCommonGrp && pGroup->fCommon) )
                          continue;

                      /* we found the group.  Unload and reload it.
                       */
                      lstrcpy(szT,pGroup->lpKey);
                      idGroup = pGroup->wIndex;
                      UnloadGroupWindow(hwndT);
                      LoadGroupWindow(szT, idGroup, fCommonGrp);
                      break;
                  }
              }
              if (!hwndT)
                  goto DEHErrExit;
          }
          break;
      }

      default:
          goto DEHErrExit;
      }
  }

  /* 't all woiked! */
  dwRet = 1;

DEHErrExit:
  GlobalUnlock(hCmd);
  GlobalFree(hCmd);

DEHErrExit1:
  GlobalUnlock(hString);

  bInDDE = FALSE;

  return dwRet;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitRespond() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL APIENTRY InitRespond( HANDLE hWnd, WPARAM wParam, LPARAM lParam,
                           LPTSTR szApp, LPTSTR szTopic,
                           BOOL fBCReply       // Whether or not to reply to a broadcast message.
                                               // ie a null app string.
                           )
{
    HWND hwndDDE = NULL;
    ATOM atom1, atom2;
    DWORD dwType;

    atom1 = GlobalAddAtom(szApp);
    atom2 = GlobalAddAtom(szTopic);

    if ((!LOWORD(lParam) && fBCReply) || LOWORD(lParam) == atom1) {
        if (!HIWORD(lParam) || HIWORD(lParam) == atom2) {

            if (!lstrcmp(szApp, szProgman)) {  // use Progman's main hwnd
                dwType = DDE_PROGMAN;
                if (IsDdeConversation(hWnd, (HWND)wParam, DDE_PROGMAN)) {
                    MPostWM_DDE_TERMINATE((HWND)wParam, hWnd);
                    hwndDDE = CreateWindow(szProgmanDDE, NULL, WS_CHILD, 0, 0, 0, 0,
                                   hwndProgman, NULL, hAppInstance, NULL);
                }
                else {
                    // use Progman's hwnd for the first DDE conversation
                    hwndDDE = hWnd;
                }
            } else if (!lstrcmp(szApp, szShell)) {
                if (!lstrcmp(szTopic, szAppIcon)) {
                    if (IsDdeConversation(hWnd, (HWND)wParam, APP_ICON)) {
                        return(TRUE);
                    }
                    dwType = APP_ICON;
                    hwndDDE = CreateWindow(szAppIconDDE, NULL, WS_CHILD, 0, 0, 0, 0,
                                   hwndProgman, NULL, hAppInstance, NULL);
                }
                else if (!lstrcmp(szTopic, szAppDesc)) {
                    if (IsDdeConversation(hWnd, (HWND)wParam, APP_DESC)) {
                        return(TRUE);
                    }
                    dwType = APP_DESC;
                    hwndDDE = CreateWindow(szAppDescDDE, NULL, WS_CHILD, 0, 0, 0, 0,
                                   hwndProgman, NULL, hAppInstance, NULL);
                }
                else if (!lstrcmp(szTopic, szAppWDir)) {
                    if (IsDdeConversation(hWnd, (HWND)wParam, APP_WDIR)) {
                        return(TRUE);
                    }
                    dwType = APP_WDIR;
                    hwndDDE = CreateWindow(szAppWDirDDE, NULL, WS_CHILD, 0, 0, 0, 0,
                                   hwndProgman, NULL, hAppInstance, NULL);
                }
            }

            //
            // For compatibility reasons, allow Shell - AppProperties DDE
            // connection.
            //
            if (!lstrcmp(szApp, szShell) &&
                          !lstrcmp(szTopic, szAppProperties) ) {  // use Progman's main hwnd
                dwType = DDE_PROGMAN;
                if (IsDdeConversation(hWnd, (HWND)wParam, DDE_PROGMAN)) {
                    MPostWM_DDE_TERMINATE((HWND)wParam, hWnd);
                    hwndDDE = CreateWindow(szProgmanDDE, NULL, WS_CHILD, 0, 0, 0, 0,
                                   hwndProgman, NULL, hAppInstance, NULL);
                }
                else {
                    // use Progman's hwnd for the first DDE conversation
                    hwndDDE = hWnd;
                }
            }

            if (hwndDDE) {
                SendMessage((HWND)wParam, WM_DDE_ACK, (WPARAM)hwndDDE,
                                                 MAKELONG(atom1, atom2));
                AddDdeConversation(hwndDDE, (HWND)wParam, dwType);
                return(TRUE);
            }
        }
    }
    /*
     * No message sent or
     * Destination won't accept the ACK and so didn't delete
     * the atoms we provided - so we must do it.
     */
//    GlobalDeleteAtom(atom1);
//    GlobalDeleteAtom(atom2);
    return(FALSE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GroupRequest() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID APIENTRY GroupRequest(HWND hWnd, HWND hwndClient, ATOM fmt, ATOM aItem)
{
    DWORD     cb;
    LPTSTR     lpT;
    register HANDLE   hT;
    register PGROUP   pGroup;
    HANDLE hReAlloc;
    LPGROUPDEF lpgd;

    if (fmt != CF_TEXT && fmt != CF_UNICODETEXT) {
        DDEFail(hWnd, hwndClient, aItem);
        return;
    }

    /*sizeof (WORD) ok, as hT becomes lpT which is LPSTR*/
    cb = 2 * sizeof(WORD) + 2;
    hT = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (!hT) {
        DDEFail(hWnd,hwndClient,aItem);
        return;
    }

    /* Ask the client to release the data and inform him that this
     * is in response to a request message.  Clipboard format is
     * plain text.
     */
    lpT = (LPTSTR)GlobalLock(hT);
    ((WORD FAR *)lpT)[0] = 3 << 12;
    ((WORD FAR *)lpT)[1] = CF_TEXT;
    ((WORD FAR *)lpT)[2] = 0;
    GlobalUnlock(hT);

    /* Go through the list of groups appending the name of each
     * group as a line in the shared memory item.
     */
    for (pGroup=pFirstGroup; pGroup; pGroup = pGroup->pNext) {
        lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

        cb += sizeof(TCHAR) * (2 + lstrlen( (LPTSTR)PTR(lpgd, lpgd->pName) ));
        if (!(hReAlloc = GlobalReAlloc(hT, cb, GMEM_MOVEABLE))) {
            GlobalFree(hT);
            DDEFail(hWnd,hwndClient,aItem);
            return;
        }
        hT = hReAlloc;

        /*sizeof (WORD) ok, as hT becomes lpT which is LPSTR*/
        lpT = (LPTSTR)((LPSTR)GlobalLock(hT) + 2 * sizeof(WORD));
        lpT += lstrlen(lpT);
        /* we've already allocated it to be large enough...
         */
        //
        // The title may contain ' (Common)' at the end if the group is a
        // common group. So get the group title from the group itself not
        // from the window title.
        //
        lstrcpy(lpT, (LPTSTR)PTR(lpgd, lpgd->pName));
        lstrcat(lpT, TEXT("\r\n"));
        GlobalUnlock(pGroup->hGroup);
        GlobalUnlock(hT);
    }

    if (fmt == CF_TEXT) {
        LPSTR lpMultiByteStr = NULL;
        int cchMultiByte = 0;
        HANDLE hMultiByte;

        // convert the string to Ansi
        lpT = GlobalLock(hT) ;
        cchMultiByte = WideCharToMultiByte(CP_ACP, 0,
                    (LPTSTR)((LPBYTE)lpT+ 2*sizeof(WORD)), -1,
                    lpMultiByteStr, cchMultiByte, NULL, NULL);

        hMultiByte = GlobalAlloc(GMEM_MOVEABLE,(++cchMultiByte) + 2 * sizeof(WORD));
        lpMultiByteStr = GlobalLock(hMultiByte);

        ((WORD FAR *)lpMultiByteStr)[0] = 3 << 12;
        ((WORD FAR *)lpMultiByteStr)[1] = CF_TEXT;
        WideCharToMultiByte(CP_ACP, 0,
                            (LPTSTR)((LPBYTE)lpT+ 2*sizeof(WORD)), -1,
                             (LPSTR)(lpMultiByteStr + 2 * sizeof(WORD)),
                             cchMultiByte, NULL, NULL);

        GlobalUnlock(hMultiByte);
        GlobalUnlock(hT);
        GlobalFree(hT);
        hT = hMultiByte;

    }
    MPostWM_DDE_DATA(hwndClient, hWnd, hT, aItem);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindIconProp() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

extern ULONG Color16Palette[];
extern ULONG Color256Palette[];

VOID APIENTRY FindIconProp(HWND hWnd, WPARAM wParam, LPARAM lParam, WORD iProp)
{
    PGROUP pGroup;
    PITEM pItem;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    UINT uiMsg = WM_DDE_ACK;
    HANDLE hData;
    DDEDATA FAR * lpdd;
    WORD cb;
    NEWICONDATA FAR * lpIconData;
    LPBYTE lpS;
    LPBYTE lpD;
    HWND hwndT;
    TCHAR szCommand[MAXITEMPATHLEN+1];
    TCHAR szDefDir[2 * (MAXITEMPATHLEN+1)];
    ATOM aItem;    // the app.'s id for which the info. is requested.
    TCHAR szId[16]; //to extract the id from the atom.
    DWORD dwId;
    PBITMAPINFOHEADER pbih, pbihNew;
    DWORD colors;
    LPVOID palette;

    if (fInExec) {
        /* we are inside the exec call!  it must have come from the
         * current icon!
         */
        pGroup = pCurrentGroup;
        pItem = pGroup->pItems;
        goto GotIt;
    }

    /* use the mdi window list to get the z-order */
    aItem = HIWORD(lParam);
    if (!GlobalGetAtomName(aItem, (LPTSTR)szId, 16))
        goto Fail;
    dwId = MyAtoi((LPTSTR)szId);
    if (!dwId) {
        goto Fail;
    }

    for (hwndT=GetWindow(hwndMDIClient, GW_CHILD); hwndT; hwndT=GetWindow(hwndT, GW_HWNDNEXT)) {
        if (GetWindow(hwndT, GW_OWNER))
    	  continue;

      	pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);

        for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {
            if (pItem->dwDDEId == dwId) {
                goto GotIt;
            }
        }
    }

Fail:
    /* didn't find it; fail
     */
    MPostDDEMsg((HWND)wParam, uiMsg, hWnd, (UINT)0, (UINT)aItem);
    return;

GotIt:
    /* from now on, we say use default instead of not me
     */
    uiMsg = WM_DDE_DATA;

    lpgd = LockGroup(pGroup->hwnd);
    if (!lpgd)
        goto Fail;

    lpid = ITEM(lpgd,pItem->iItem);

    switch (iProp) {

    case APP_ICON:
        cb = (WORD)(sizeof(NEWICONDATA) + lpid->cbIconRes);
        pbih = (PBITMAPINFOHEADER)PTR(lpgd, lpid->pIconRes);
        if (pbih->biClrUsed == -1) {
            colors = (1 << (pbih ->biPlanes * pbih->biBitCount));
            if (colors == 16 || colors == 256) {
                cb += (WORD)(colors * sizeof(RGBQUAD));
            }
        }
        break;

    case APP_DESC:
        cb = (WORD)(sizeof(DDEDATA) + sizeof(TCHAR)*lstrlen((LPTSTR) PTR(lpgd, lpid->pName)));
        break;

    case APP_WDIR:
        GetItemCommand(pGroup, pItem, szCommand, szDefDir);
        cb = (WORD)(sizeof(DDEDATA) + sizeof(TCHAR)*lstrlen(szDefDir));
        break;

    default:
        goto Fail;
    }

    hData = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT,
            (DWORD)cb);
    if (!hData) {
        UnlockGroup(pGroup->hwnd);
        goto Fail;
    }

    lpdd = (DDEDATA FAR *)GlobalLock(hData);
    if (!lpdd) {
        GlobalFree(hData);
        UnlockGroup(pGroup->hwnd);
        goto Fail;
    }
    lpdd->fResponse = TRUE;
    lpdd->fRelease = TRUE;
    lpdd->cfFormat = CF_TEXT;

    switch (iProp) {
    case APP_ICON:
    	if ((short)lpid->cbIconRes <= 0) {
            // This icon is toast.
            GlobalUnlock(hData);
            UnlockGroup(pGroup->hwnd);
            goto Fail;
        }

        lpIconData = (NEWICONDATA FAR *)lpdd;

        lpIconData->dwResSize = (DWORD)lpid->cbIconRes;
        //lpIconData->dwVer = lpid->dwIconVer;
        lpIconData->dwVer = (lpid->wIconVer == 2) ? 0x00020000 : 0x00030000;

        lpD = (LPBYTE)&(lpIconData->iResource);
        lpS = (LPBYTE)PTR(lpgd, lpid->pIconRes);
        cb = lpid->cbIconRes;
        if ((pbih->biClrUsed == -1) && (colors == 16 || colors == 32)) {
            if (colors == 16) {
                palette = Color16Palette;
            } else if (colors == 256) {
                palette = Color256Palette;
            }

            pbihNew = (PBITMAPINFOHEADER)lpD;
            RtlCopyMemory(pbihNew, pbih, sizeof( *pbih ));
            pbihNew->biClrUsed = 0;
            RtlCopyMemory((pbihNew+1), palette, colors * sizeof(RGBQUAD));
            RtlCopyMemory((PCHAR)(pbihNew+1) + (colors * sizeof(RGBQUAD)),
                          (pbih+1),
                          lpid->cbIconRes - sizeof(*pbih)
                         );
        } else {
            while (cb--) {
                *lpD++ = *lpS++;
            }
        }

        break;

    case APP_DESC:
        lstrcpy((LPTSTR)lpdd->Value,(LPTSTR) PTR(lpgd, lpid->pName));
        break;

    case APP_WDIR:
        lstrcpy((LPTSTR)lpdd->Value,szDefDir);
        break;
    }

    GlobalUnlock(hData);
    UnlockGroup(pGroup->hwnd);

    if (!MPostWM_DDE_DATA((HWND)wParam, hWnd, hData, (ATOM)aItem)){
        GlobalFree(hData);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindIconPath() -                                                        */
/*     In NT groups the icon path is not stored when it is not specified by */
/*     the user when the item is created. For DDE requests on groups, the   */
/*     icon path needs to be returned. This function will determine the     */
/*     icon path the way it first find the icon.                            */
/*  9/17/93 JOhannec
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FindIconPath(
    LPTSTR szPathField,
    LPTSTR szDirField,
    LPTSTR szIconPath
    )
{
    TCHAR szIconExe[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    HICON hIcon;
    WORD wIconIndex;
    WORD wIconId;

    lstrcpy(szIconExe, szPathField);
    DoEnvironmentSubst(szIconExe, CharSizeOf(szIconExe));
    StripArgs(szIconExe);
    TagExtension(szIconExe, sizeof(szIconExe));
    if (*szIconExe == TEXT('"') && *(szIconExe + lstrlen(szIconExe)-1) == TEXT('"')) {
        SheRemoveQuotes(szIconExe);
    }

        //
        // if it's a relative path, extractassociatedicon and LoadLibrary don't
        // handle that so find the executable first
        //
        SetCurrentDirectory(szOriginalDirectory);
        FindExecutable(szIconExe, szDirField, szTemp);
        if (*szTemp) {
            lstrcpy(szIconExe, szTemp);
            TagExtension(szIconExe, sizeof(szIconExe));
            if (*szIconExe == TEXT('"') && *(szIconExe + lstrlen(szIconExe)-1) == TEXT('"')) {
		       SheRemoveQuotes(szIconExe);
	        }
        }
        else {
            *szIconExe = 0;    // Use a dummy value so no icons will be found
                               // and progman's item icon will be used instead
                               // This is to make moricons.dll item icon be the
                               // right one.  -johannec 6/4/93
        }
        //
        // reset the current directory to progman's working directory i.e. Windows directory
        //
        SetCurrentDirectory(szWindowsDirectory);

        wIconIndex = 0;
        hIcon = ExtractAssociatedIconEx(hAppInstance, szIconExe, &wIconIndex, &wIconId);
        if (hIcon)
            DestroyIcon(hIcon);

        lstrcpy(szIconPath, szIconExe);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddStringToSeg() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL APIENTRY AddStringToSeg(LPHANDLE lphT, LPINT lpcb, LPTSTR lpsz, WORD wT, BOOL fCR)
{
    TCHAR szT[10];
    INT cb;
    LPTSTR lp;
    HANDLE hReAlloc;

    if (!lpsz) {
        wsprintf(szT,TEXT("%d"),wT);
        lpsz = szT;
        wT = (WORD)0;
    }

    cb = sizeof(TCHAR) * (lstrlen(lpsz) + (wT ? 2 : 0) + (fCR ? 2 : 1));
    if (!(hReAlloc = GlobalReAlloc(*lphT,*lpcb+cb,GMEM_MOVEABLE|GMEM_ZEROINIT))) {
        GlobalFree(*lphT);
        return FALSE;
    }
    else {
        *lphT = hReAlloc;
    }

    lp = (LPTSTR)((LPSTR)GlobalLock(*lphT) + *lpcb - 2);   // this is to go before the null byte
    if (wT)
        *lp++ = TEXT('"');

    lstrcpy(lp,lpsz);
    lp += lstrlen(lp);
    if (wT)
        *lp++ = TEXT('"');

    if (fCR) {
        *lp++ = TEXT('\r');
        *lp++ = TEXT('\n');
    }
    else
        *lp++ = TEXT(',');
    *lp = 0;
    GlobalUnlock(*lphT);

    *lpcb += cb;

    return TRUE;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DumpGroup() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/
VOID APIENTRY DumpGroup(HWND hwnd, ATOM aName, HWND hwndConv, WORD cfFormat)
{
    HWND hwndGroup;
    PGROUP pGroup;
    PITEM pItem;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    WORD i;
    INT cb;
    HANDLE hT;
    LPTSTR lpT;
    INT state;
    BOOL fActivated;

    if (cfFormat != CF_TEXT && cfFormat != CF_UNICODETEXT)
        goto Fail;

    for (hwndGroup = GetWindow(hwndMDIClient,GW_CHILD);
         hwndGroup;
         hwndGroup = GetWindow(hwndGroup,GW_HWNDNEXT)) {
        if (GetWindow(hwndGroup,GW_OWNER))
            continue;

        lpgd = LockGroup(hwndGroup);
        if (!lpgd)
            goto Fail;

        if (aName == GlobalFindAtom((LPTSTR) PTR(lpgd, lpgd->pName)))
            goto FoundGroup;
        UnlockGroup(hwndGroup);
    }

Fail:
#ifdef ORGCODE
    DDEFail(hwnd,hwndConv,MAKELONG(cfFormat,aName));
#else
    DDEFail(hwnd, hwndConv, aName);
#endif
    return;

FoundGroup:
    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup,GWLP_PGROUP);

        /*sizeof (WORD) ok, as hT becomes lpT which is LPSTR*/
    cb = 2 * sizeof(WORD) + 2;
    hT = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (!hT)
        goto Fail;

    /* Ask the client to release the data and inform him that this
     * is in response to a request message.  Clipboard format is
     * plain text.
     */
    lpT = (LPTSTR)GlobalLock(hT);
    ((WORD FAR *)lpT)[0] = 3 << 12;
    ((WORD FAR *)lpT)[1] = CF_TEXT;
    ((WORD FAR *)lpT)[2] = 0;
    GlobalUnlock(hT);

    /* the first line is group properties
     */
    if (!AddStringToSeg(&hT,&cb,(LPTSTR) PTR(lpgd, lpgd->pName),TRUE,FALSE))
        goto Fail;

#if 1
// don't allow apps to know the group key.

//
// change 2-21-93 johannec
// for compatibilty reasons we must privide the group filename which
// doesn't mean anything in NT so we provide the key name instad.
    if (!AddStringToSeg(&hT,&cb,pGroup->lpKey, FALSE, FALSE))
        goto Fail;
#endif

    /* put the number of items in
     */
    for (i = 0, pItem = pGroup->pItems; pItem; pItem = pItem->pNext)
        i++;

#if 1
    if (!AddStringToSeg(&hT,&cb,NULL,i,FALSE))
    	goto Fail;

    // Return the window state as a SW_ value.
    // REVIEW not all SW_ values are supported.
    // It would be nice if there was some way to query a SW_ value
    // but I guess it would too much to ask for windows to be even remotely
    // orthogonal.  I don't know who "designed" the Windows API but it
    // really is the worst windowing system I have ever used.
    // Luckily orthogonality doesn't affect stock prices :-)
    state = SW_SHOWNORMAL;

    if (pGroup == pCurrentGroup) {
        fActivated = TRUE;
    }
    else {
        fActivated = FALSE;
    }

    if (IsZoomed(hwndGroup)) {
        // Maxed.
        state = SW_SHOWMAXIMIZED;
    }
    else if (IsIconic(hwndGroup)) {
        // Minned.
        if(fActivated)
            state = SW_SHOWMINIMIZED;
        else
            state = SW_SHOWMINNOACTIVE;
    }
    else {
        // It's normal.
        if(fActivated)
            state = SW_SHOWNORMAL;
        else
            state = SW_SHOWNOACTIVATE;
    }

    // Give info on the state.
    if (!AddStringToSeg(&hT,&cb,NULL,(WORD)state, FALSE))
    	goto Fail;
#else
    if (!AddStringToSeg(&hT,&cb,NULL,i,FALSE))
	    goto Fail;
#endif

    if (!AddStringToSeg(&hT,&cb,NULL,(WORD)pGroup->fCommon,TRUE))
        goto Fail;


    /* each additional line is an item
     */
    for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {

        lpid = ITEM(lpgd,pItem->iItem);

        /* name
         */
        if (!AddStringToSeg(&hT, &cb, (LPTSTR) PTR(lpgd, lpid->pName), TRUE, FALSE))
            goto Fail;

        /* command line and default directory
         */
        GetItemCommand(pGroup, pItem, szPathField, szDirField);
        if (!AddStringToSeg(&hT, &cb, szPathField, TRUE, FALSE))
            goto Fail;
        if (!AddStringToSeg(&hT, &cb, szDirField, FALSE, FALSE))
            goto Fail;

        /* icon path
         */
        if (!*(LPTSTR)PTR(lpgd, lpid->pIconPath)) {
            FindIconPath(szPathField, szDirField, szIconPath);
        }
        else {
            lstrcpy(szIconPath, (LPTSTR) PTR(lpgd, lpid->pIconPath));
        }
        if (!AddStringToSeg(&hT, &cb, szIconPath, FALSE, FALSE))
            goto Fail;

        /* x-y coordinates
         */
        if (!AddStringToSeg(&hT, &cb, NULL, (WORD)pItem->rcIcon.left, FALSE))
            goto Fail;

        if (!AddStringToSeg(&hT, &cb, NULL, (WORD)pItem->rcIcon.top, FALSE))
            goto Fail;

        /* icon, hotkey, fminimize
         */
        if ((SHORT)lpid->wIconIndex >= 0) {
            //
            // apps requesting group info are expecting icon index not icon id.
            //
            if (!AddStringToSeg(&hT, &cb, NULL, lpid->wIconIndex,FALSE))
                goto Fail;
        }
        else {
            if (!AddStringToSeg(&hT, &cb, NULL, lpid->iIcon, FALSE))
                goto Fail;
        }

        if (!AddStringToSeg(&hT,&cb,NULL,GroupFlag(pGroup,pItem,(WORD)ID_HOTKEY),FALSE))
            goto Fail;

        if (!AddStringToSeg(&hT,&cb,NULL,GroupFlag(pGroup,pItem,(WORD)ID_MINIMIZE),FALSE))
            goto Fail;

        if (!AddStringToSeg(&hT,&cb,NULL,GroupFlag(pGroup,pItem,(WORD)ID_NEWVDM),TRUE))
            goto Fail;
    }

#ifdef ORGCODE
    PostMessage(hwndConv, WM_DDE_DATA, hwnd, MAKELONG(hT,cfFormat));
#else
    if (cfFormat == CF_TEXT) {
        LPSTR lpMultiByteStr = NULL;
        int cchMultiByte = 0;
        HANDLE hMultiByte;

	    // convert the string to Ansi
        lpT = GlobalLock(hT) ;
        cchMultiByte = WideCharToMultiByte(CP_ACP, 0,
                    (LPTSTR)((LPBYTE)lpT+ 2*sizeof(WORD)), -1,
                    lpMultiByteStr, cchMultiByte, NULL, NULL);

        hMultiByte = GlobalAlloc(GMEM_MOVEABLE,(++cchMultiByte) + 2 * sizeof(WORD));
        lpMultiByteStr = GlobalLock(hMultiByte);

        ((WORD FAR *)lpMultiByteStr)[0] = 3 << 12;
        ((WORD FAR *)lpMultiByteStr)[1] = CF_TEXT;
        WideCharToMultiByte(CP_ACP, 0,
                            (LPTSTR)((LPBYTE)lpT+ 2*sizeof(WORD)), -1,
                             (LPSTR)(lpMultiByteStr + 2 * sizeof(WORD)),
                             cchMultiByte, NULL, NULL);

	    GlobalUnlock(hMultiByte);
        GlobalUnlock(hT);
        GlobalFree(hT);
        hT = hMultiByte;


    }
    MPostWM_DDE_DATA(hwndConv, hwnd, hT, (ATOM)aName);
#endif

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DDEMsgProc() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LRESULT APIENTRY DDEMsgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    // should go in ProgmanWndProc
    case WM_DDE_INITIATE:
        //
        // HACK: returning 1 if the WM_DDE_ACK was sent successfully in
        // InitRespond is NOT part of the DDE Protocol BUT for backward
        // compatability with WIndows3.0 and 3.1 this solves
        // some problems with WOW apps' setup.
        //

#ifdef DEBUG_PROGMAN_DDE
        {
        TCHAR szDebug[300];

        wsprintf (szDebug, TEXT("%d   PROGMAN:   Received WM_DDE_INITIATE\r\n"),
                  GetTickCount());
        OutputDebugString(szDebug);
        }
#endif

        if (InitRespond(hWnd,wParam,lParam,szShell,szAppIcon, TRUE))
            return(1L);
        if (InitRespond(hWnd,wParam,lParam,szShell,szAppDesc, TRUE))
            return(1L);
        if (InitRespond(hWnd,wParam,lParam,szShell,szAppWDir, TRUE))
            return(1L);
//      InitRespond(hWnd,wParam,lParam,szShell,szSystem, TRUE);
        if (InitRespond(hWnd,wParam,lParam,szProgman,szProgman, FALSE)) {
#ifdef DEBUG_PROGMAN_DDE
            {
            TCHAR szDebug[300];

            wsprintf (szDebug, TEXT("%d   PROGMAN:   Received WM_DDE_INITIATE.  return 1\r\n"),
                      GetTickCount());
            OutputDebugString(szDebug);
            }
#endif
            return(1L);
        }
        //
        // For compatibility reasons, allow Shell - AppProperties DDE
        // connection
        //
        if (InitRespond(hWnd,wParam,lParam,szShell,szAppProperties, TRUE))
            return(1L);

#ifdef DEBUG_PROGMAN_DDE
        {
        TCHAR szDebug[300];

        wsprintf (szDebug, TEXT("%d   PROGMAN:   Received WM_DDE_INITIATE.  FAILED\r\n"),
                  GetTickCount());
        OutputDebugString(szDebug);
        }
#endif
        break;

    case WM_DDE_REQUEST:
    {
        ATOM fmt;
        ATOM aItem;

        fmt = GET_WM_DDE_REQUEST_FORMAT(wParam, lParam);
        aItem = GET_WM_DDE_REQUEST_ITEM(wParam, lParam);
        if (aItem == GlobalFindAtom(szProgman)
            || aItem == GlobalFindAtom(szGroupList)) {
            GroupRequest(hWnd, (HWND)wParam, fmt, aItem);
        }
        else
            DumpGroup(hWnd, aItem, (HWND)wParam, fmt);
        DDEFREE(WM_DDE_REQUEST, lParam);
        break;
    }

    case WM_DDE_EXECUTE:
    {
        HANDLE hCommands;
        WORD wStatus;
        DWORD ret;
    LPSTR lpCommands ;
    HLOCAL hloc ;
    HLOCAL hlocTemp ;
    int cchMultiByte ;
    LPWSTR lpWideCharStr = NULL ;
    int cchWideChar = 0  ;
    BOOL bIsWindowUnicode ;
        UnpackDDElParam(WM_DDE_EXECUTE, lParam, NULL, (PUINT_PTR)&hCommands);

	// was the sending window a unicode app?
        bIsWindowUnicode=IsWindowUnicode((HWND)wParam) ;
	if (!bIsWindowUnicode) {
	    // convert the string to unicode
            lpCommands = GlobalLock(hCommands) ;
            cchMultiByte=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpCommands,
                    -1,lpWideCharStr,cchWideChar) ;

            hloc = GlobalAlloc(GMEM_MOVEABLE,(++cchMultiByte)*sizeof(TCHAR)) ;
            lpWideCharStr = GlobalLock(hloc) ;

            MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpCommands,
                            -1,lpWideCharStr,cchMultiByte) ;

	    GlobalUnlock(hloc) ;
            GlobalUnlock(hCommands) ;
            hlocTemp  = hCommands;
            hCommands = hloc ;
	}

        if (ret = ExecuteHandler(hCommands)) {
            wStatus = 0x8000;
        } else {
            wStatus = 0x0000;
        }
	if (!bIsWindowUnicode) {
            hCommands = hlocTemp;
	    GlobalFree(hloc) ;
        }

        MPostWM_DDE_EXECACK((HWND)wParam, hWnd, wStatus, hCommands);
        if (ret == 2) {         // Exit command was executed
            MPostWM_DDE_TERMINATE((HWND)wParam, hWnd);
            PostMessage(hwndProgman, WM_CLOSE, 0, 0L);
        }
//      DDEFREE(WM_DDE_EXECUTE, lParam);    // executes arn't really packed.
        break;
    }

    case WM_DDE_TERMINATE:
#ifdef ORGCODE
        SendMessage(wParam, WM_DDE_TERMINATE, (WPARAM)hWnd, lParam);
#else
        RemoveDdeConversation(hWnd, (HWND)wParam, DDE_PROGMAN);
        MPostWM_DDE_TERMINATE((HWND)wParam, hWnd);
//      DDEFREE(WM_DDE_TERMINATE, lParam);  // terminates arn't packed
#endif
        if (hWnd != hwndProgman) {
            DestroyWindow (hWnd);
        }
        break;

    case WM_DDE_ACK:
        DDEFREE(WM_DDE_ACK, lParam);
        break;

    /* All other DDE messages are unsupported. */
    case WM_DDE_DATA:
    case WM_DDE_ADVISE:
    case WM_DDE_UNADVISE:
    case WM_DDE_POKE:
#ifdef ORGCODE
        DDEFail(hWnd,wParam,lParam);
#else
        {
            UINT_PTR uiHi;

            UnpackDDElParam(wMsg, lParam, NULL, &uiHi);
            DDEFail(hWnd, (HWND)wParam, (ATOM)uiHi);
            DDEFREE(wMsg, lParam);
        }
#endif
        break;

    default:
        return DefWindowProc(hWnd,wMsg,wParam,lParam);
    }
    return(0L);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AppIconDDEMsgProc() -                                                   */
/*                                                                          */
/*        Application = "Shell"                                             */
/*        Topic = "AppIcon"                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LRESULT APIENTRY AppIconDDEMsgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    case WM_DDE_REQUEST:
        FindIconProp(hWnd, wParam, lParam, APP_ICON);
        DDEFREE(WM_DDE_REQUEST, lParam);
        break;

    case WM_DDE_TERMINATE:
        RemoveDdeConversation(hWnd, (HWND)wParam, APP_ICON);
        MPostWM_DDE_TERMINATE((HWND)wParam, hWnd);
        DDEFREE(WM_DDE_TERMINATE, lParam);
        DestroyWindow(hWnd);
        break;

    default:
        return DDEMsgProc(hWnd, wMsg, wParam, lParam);
    }
    return 0L;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AppDescriptionDDEMsgProc() -                                            */
/*                                                                          */
/*        Application = "Shell"                                             */
/*        Topic = "AppDescription"                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LRESULT APIENTRY AppDescriptionDDEMsgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    case WM_DDE_REQUEST:
        FindIconProp(hWnd, wParam, lParam, APP_DESC);
        DDEFREE(WM_DDE_REQUEST, lParam);
        break;

    case WM_DDE_TERMINATE:
        RemoveDdeConversation(hWnd, (HWND)wParam, APP_DESC);
        PostMessage((HWND)wParam, WM_DDE_TERMINATE, (WPARAM)hWnd, 0L);
        DestroyWindow(hWnd);
        break;

    default:
        return DDEMsgProc(hWnd, wMsg, wParam, lParam);
    }
    return 0L;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AppWorkingDirDDEMsgProc() -                                             */
/*                                                                          */
/*        Application = "Shell"                                             */
/*        Topic = "AppWorkingDir"                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LRESULT APIENTRY AppWorkingDirDDEMsgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    case WM_DDE_REQUEST:
        FindIconProp(hWnd, wParam, lParam, APP_WDIR);
        DDEFREE(WM_DDE_REQUEST, lParam);
        break;

    case WM_DDE_TERMINATE:
        RemoveDdeConversation(hWnd, (HWND)wParam, APP_WDIR);
        MPostWM_DDE_TERMINATE((HWND)wParam, hWnd);
        DDEFREE(WM_DDE_TERMINATE, lParam);
        DestroyWindow(hWnd);
        break;

    default:
        return DDEMsgProc(hWnd, wMsg, wParam, lParam);
    }
    return 0L;
}


VOID APIENTRY RegisterDDEClasses(HANDLE hInstance)
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = AppIconDDEMsgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szAppIconDDE;

    if (!RegisterClass(&wc))
        bAppIconDDE = FALSE;

    wc.lpfnWndProc = AppDescriptionDDEMsgProc;
    wc.lpszClassName = szAppDescDDE;

    if (!RegisterClass(&wc))
        bAppDescDDE = FALSE;

    wc.lpfnWndProc = AppWorkingDirDDEMsgProc;
    wc.lpszClassName = szAppWDirDDE;

    if (!RegisterClass(&wc))
        bAppWDirDDE = FALSE;

    wc.lpfnWndProc = DDEMsgProc;
    wc.lpszClassName = szProgmanDDE;

    if (!RegisterClass(&wc))
        bProgmanDDE = FALSE;

    InitDdeConversationStruct();

}
