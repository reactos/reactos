/*
** SND.C
**
** Sound applet for the NT Control Panel.
**
*/
/* Revision history:
 * Laurie Griffiths 19/12/91  Ported the windows 3.1 code to NT
 */

/*=============================================================================
| The whole thing is kicked off by Init.c
| When the DLL is loaded DllInitialize in init.c is called (as a result of
| the DLLENTRY line in SOURCES).  This just loads some static strings
| with values dug out of resources.
|
| The control panel then calls CPlApplet in init.c with a series of
| messages which are explained in control\h\cpl.h
|
| The CPL_INQUIRE message is different in 3.1 (CPL_NEWINQUIRE).
|
| The big one is CPL_DBLCLK which is the go signal.
| A dialog box is created in RunApplet in init.c with window procedure
| SoundDlg (end of this file).
|
| WM_INITDIALOG is the trigger to fill the list boxes with text which is
| taken from the INI file (InitDialog).  If there is no wave device present
| then it just allows the user to set the message beep flag (displays
| original state and allows alteration).  If there is a wave device then
| the Test button is enabled.  Whether or not there is a device to play the
| files, you can still assign them to events.  This could be useful
| to someone installing the system.  There's no real reason why they should
| be forced to configure the sound board first.
|
| Currently it does a message box to say that there's no sound board.
| Is this too intrusive?
|
| The calling tree looks like:
|
| DllInitialize  loads resources
|
| CPlApplet             main entry point from control panel (in init.c)
|     RunApplet             brings up dialog box (in init.c)
|         DialogBox
|             SoundDlg              dialog procedure
|                 InitDialog
|                     FindDescription        find description in <name=file,descr> string
|                     ShowSound              show filename associated with a sound
|                     NewSound               update list box with new <name,file,descr>
|                     catPath                concatenate path onto file
|                 DrawItem               draw item in list box
|                     DrawItemText           draw text of item
|                         GetSoundEntry          interrogate list box
|                 ControlMessage         process messages from controls in dialogs
|                     EnablePlay             enable/disable the TEST button
|                     FillDirBox             fill list box from files in current dir
|                     ShowSound              show filename associated with a sound
|                     PlayTheSound           play the i-th sound entry
|                         GetSoundEntry          interrogate list box
|                     WriteSounds            write out a new [sounds] section
|                    ChangeSound            change the filename associated with a sound
|                         QualifyFileName        get full path name of file
|                         NewSound               update list box with new <name,file,descr>
|                         GetSoundEntry          interrogate list box
|
| The following are called from several places in the above
|
| GetSoundEntry
|     FindDescription        find description in <name=file,descr> string
|
| EnablePlay            enable/disable the TEST button
|     GetSoundEntry         interrogate list box
|
| ShowSound             show filename associated with a sound
|     GetSoundEntry         interrogate list box
|     QualifyFileName       get full path name of file
|     FileName              extract file name from full path name
|     StripPathName         split path into path and file by inserting null
|         FileName              extract file name from full path name
|     FillDirBox            fill list box from files in current dir
|     EnablePlay            enable/disable the TEST button
 ============================================================================*/

#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>
#include <cphelp.h>

#include <cpl.h>
#include <shellapi.h>
#include "snd.h"

#define     SetWindowRedraw(hwnd, fRedraw)  \
            ((VOID)SendMessage(hwnd, WM_SETREDRAW, (UINT)(BOOL)(fRedraw), 0L))


/*---------------------------------------------------------------------------*/

#define STRSIZ  MAX_PATH    // maximum size of a string or filename
#define MAXINI  4096       // max size of all ini keys in [sounds] section
#define SLASH(c)        ((c) == '/' || (c) == '\\')

/*
**  DLGOPEN private definitions
**/

#define ATTRFILELIST    0x0000  // include files only
#define ATTRDIRLIST     0xC010  // directories and drives ONLY

/*---------------------------------------------------------------------------*/

static SZCODE aszNull[] = "";
static SZCODE aszDirSeparator[] = "\\";
static SZCODE aszResourceDir[]= "resource";
static SZCODE aszSounds[] = "sounds";
static SZCODE aszNewSoundFindFormat[] = "%s=";
static SZCODE aszNewSoundFormat[] = "%s=%s,%s";
static SZCODE aszCurrentDir[] = ".";
static SZCODE aszFileFilter[] = "*.wav";
static char   aszAllFiles[] = "*.*";    // this is written on by DlgDirList()
static BOOL   fSndPlaySound;            // sound board working
static BOOL   fIniChanged;
static BOOL   fEnabled= FALSE;          // True iff [Sounds] Enabled=1 in Win.ini

#if PARANOIA
/*---------------------------------------------------------------------------
| put <title><str>"\n" out onto debug screen
 ----------------------------------------------------------------------------*/
void Trace(LPSTR lpstr, LPSTR lpstrTitle)
{  char msg[255];
   if (!lpstr) lpstr = "";
   if (!lpstrTitle) lpstrTitle = "SOUND: SND.C";
   wsprintf(msg, "%s %s\n", lpstrTitle, lpstr);
   OutputDebugString(msg);
} /* Trace */
#endif

/*---------------------------------------------------------------------------
| ?
 ----------------------------------------------------------------------------*/
static VOID PASCAL NEAR CPHelp( HWND hwnd, DWORD dContext)
{  WinHelp(hwnd, aszSoundHlp, HELP_CONTEXT, dContext);
} /* CPHelp */

/*---------------------------------------------------------------------------
| lszEntry points to a string from WIN.INI
| split it at the start of the description by inserting 0 and return
| pointer to the description
|
| CASE1 (nice -- delimiter present )
|       input string :  text<delim>text
|       return ptr   :             ^
|       output string:  text0      text
|    where <delim> ::= ","[{<tab>|<space>|","}...]
|           0 represents the end-of-string delimiter
|
| CASE2 (nasty -- no delimiter present )
|       input string :  text0
|       return ptr   :      ^
|       output string:  text0
| (Since Fangled And Tangled file systems allow spaces in names, the delimiter
| rules were changed from DOS require the delimiter to start with comma).
 ----------------------------------------------------------------------------*/
static LPSTR FindDescription( LPSTR lszEntry)
{
   /* skip to next end-of-string or comma (space or tab no longer count) */
   for (
       ; *lszEntry && *lszEntry!=','
       ; lszEntry++
       )
   ;

   if (*lszEntry)
   {  BOOL fComma;

      fComma = (*lszEntry == ',');
      *lszEntry = (char)0;
      /* skip the inserted null and any following blanks and tabs */
      for (lszEntry++; *lszEntry == ' ' || *lszEntry == '\t'; lszEntry++)
      ;
      /* if now found a comma and hadn't before, skip tabs and spaces again */
      if (!fComma && (*lszEntry == ','))
      {  for (lszEntry++; *lszEntry == ' ' || *lszEntry == '\t'; lszEntry++)
         ;
      }
   }
   return lszEntry;
} /* FindDescription */

/*---------------------------------------------------------------------------
| Get (Event, File, Description) from the iListEntry-th entry in the NAMES list box
| iListEntry==-1 means use the current selection
| Supplying NULL for any of these PLPSTRs is safe and means don't bother
| to return this item.
 ----------------------------------------------------------------------------*/
static VOID NEAR GetSoundEntry( HWND    hwnd           /* dialog box */
                              , int iListEntry         /* number of event */
                              , LPSTR lszEvent         /* event name returned */
                              , LPSTR lszFile          /* file name returned */
                              , LPSTR lszDescription   /* description returned */
                              )
{  char aszBuffer[STRSIZ];
   LPSTR lszCur;
   LPSTR lszStart;

   /* aszBuffer = text of iListEntry-th entry in list box, -1 => use current */
   hwnd = GetDlgItem(hwnd, LB_NAMES);
   if (iListEntry == -1)
      iListEntry = (int)(LONG)
                      SendMessage(hwnd, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
   lstrcpy(aszBuffer, (PSTR)(UINT)(DWORD)SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iListEntry, (LPARAM)0));

   /* replace "=" in aszBuffer with NULL (there'd better be one there!) */
   for (lszCur = aszBuffer; *lszCur != '='; lszCur++)
   ;
   *lszCur = (char)0;

   /* lszEvent = <Event> from list entry */
   if (lszEvent)
   {  lstrcpy(lszEvent, aszBuffer);
   }

   lszStart = ++lszCur;
   lszCur = FindDescription(lszCur);
   if (lszFile)
   {  if (!lstrcmpi(lszStart, aszNoSound))
          *lszFile = (char)0;
      else
         lstrcpy(lszFile, lszStart);
   }
   if (lszDescription)
   {  if (!*lszCur)
         lszCur = aszBuffer;
      lstrcpy(lszDescription, lszCur);
   }
}/* GetSoundEntry */


/*---------------------------------------------------------------------------
| get full path name of file.  Return TRUE iff it worked.
| updates lszFile, assumes there is enough room.
 ----------------------------------------------------------------------------*/
static BOOL NEAR PASCAL QualifyFileName( LPSTR  lszFile )
{  OFSTRUCT of;
   UINT     fErrMode;
   BOOL     fReturn;

   fErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);  /* return errors to us */
   if ( OpenFile( lszFile
                , &of
                , OF_EXIST | OF_SHARE_DENY_NONE | OF_READ
                )
      != HFILE_ERROR
      )
   {  OemToAnsi(of.szPathName, lszFile);
      fReturn = TRUE;
   } else
      fReturn = FALSE;
   SetErrorMode(fErrMode);
   return fReturn;
}/* QualifyFileName */

/*---------------------------------------------------------------------------
| update lszPath to point to file-name at end of path-and-file-name
 ----------------------------------------------------------------------------*/
static LPSTR NEAR PASCAL FileName( LPCSTR lszPath)
{  LPCSTR lszCur;
   /* move forward to first null */
   for (lszCur = lszPath; *lszCur; lszCur++)
   ;
   /* move backwards until find SLASH or colon */
   for (; lszCur >= lszPath && !SLASH(*lszCur) && *lszCur != ':'; lszCur--)
   ;
   /* move forwards one char */
   return (LPSTR)++lszCur;
}/* FileName */

/*---------------------------------------------------------------------------
| update lszPath to insert a null at end of path (possibly hitting the "\"
| possibly hitting the start of the file name)
 ----------------------------------------------------------------------------*/
static VOID NEAR PASCAL StripPathName( LPSTR lszPath)
{  LPSTR lszCur;
   /* move on to first char of filename at end of path */
   lszCur = FileName(lszPath);
   /* if path at least two chars long and ends in \ and not in :\
   |  then move back one char
   */
   if (lszCur > lszPath+1 && SLASH(lszCur[-1]) && lszCur[-2] != ':')
      lszCur--;
   /* insert null */
   *lszCur = (char)0;
}/* StripPathName */

/*---------------------------------------------------------------------------
| show the filenames that can be associated with sounds
 ----------------------------------------------------------------------------*/
static VOID NEAR FillDirBox( HWND hwnd
                           , LPSTR lszDir  // the path
                           )
{  char aszCWD[STRSIZ];
   char aszNewCWD[STRSIZ];
   int iDirLen;
   UINT fErrMode;

   // remove any stupid trailing '/'
   iDirLen = lstrlen(lszDir) - 1;
   if (SLASH(lszDir[iDirLen]) && lszDir[iDirLen - 1] != ':')
      lszDir[iDirLen] = (char)0;

   GetCurrentDirectory(STRSIZ, aszCWD);
   fErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);  /* return errors to us */
   if (  !SetCurrentDirectory(lszDir)              // set from string
      || !GetCurrentDirectory(STRSIZ, aszNewCWD)   // get canonical form?
      )
   {  /* failed!  back out and beep */
      SetCurrentDirectory(aszCWD);
      SetErrorMode(fErrMode);
      MessageBeep(0);
   } else
   {  HWND hwndFiles;

      SetErrorMode(fErrMode);
      hwndFiles = GetDlgItem(hwnd, LB_FILES);
      if ( !(int)(LONG)
                   SendMessage( hwndFiles, LB_GETCOUNT, (WPARAM)0, (LPARAM)0)
         || lstrcmpi(aszNewCWD, aszCWD)
         )
      {  /* Fill directory listbox from current directory */
         SetWindowRedraw(hwndFiles, FALSE);
         DlgDirList(hwnd, aszAllFiles, LB_FILES, ID_DIR, ATTRDIRLIST);
         SendMessage( hwndFiles
                    , LB_DIR
                    , (WPARAM)ATTRFILELIST
                    , (LPARAM)(LPCSTR)aszFileFilter
                    );
         SetWindowRedraw(hwndFiles, TRUE);
         SendMessage( hwndFiles
                    , LB_ADDSTRING
                    , (WPARAM)0
                    , (LPARAM)(LPCSTR)aszNoSound
                    );
      }
   }
}/* FillDirBox */

/*---------------------------------------------------------------------------
| Enable or Disable the test button, file and sound displays
 ----------------------------------------------------------------------------*/
static VOID NEAR EnablePlay( HWND hwnd)
{ char    aszFile[STRSIZ];
  int     iSelection;
  BOOL    fSelection;
  HWND    hwndFiles;

  hwndFiles = GetDlgItem(hwnd, LB_FILES);
  iSelection = (int)(LONG)
                    SendMessage(hwndFiles, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
  fSelection = DlgDirSelectEx(hwnd, aszFile, STRSIZ, LB_FILES);
  GetSoundEntry(hwnd, -1, NULL, aszFile, NULL);
  EnableWindow( GetDlgItem(hwnd, ID_PLAY)               // the TEST button
              , fSndPlaySound
                && !fSelection
                && *aszFile
                && (iSelection != LB_ERR)
              );
  // enable disable the listboxes -- actually always enable (LKG)
  EnableWindow(GetDlgItem(hwnd, LB_NAMES), TRUE || fSndPlaySound);
  EnableWindow(hwndFiles, TRUE || fSndPlaySound);
}/* EnablePlay */


/*---------------------------------------------------------------------------
| Select the i-th element in the event NAMES list box
| Look up the file from the registry.
| Refill the FILES list box based on the dir containing the file
| Highlight (select) the file associated with it in the FILES list box
| if possible - the filename from the registry might be crap
| If there is no file, set the selection to no sound
 ----------------------------------------------------------------------------*/
static VOID NEAR ShowSound( HWND hwnd, int i)
{  char aszFile[STRSIZ];
   char aszPath[STRSIZ];


   if (i != -1)
      SendDlgItemMessage(hwnd, LB_NAMES, LB_SETCURSEL, (WPARAM)i, (LPARAM)0);
   GetSoundEntry(hwnd, i, NULL, aszFile, NULL);
   lstrcpy(aszPath, aszFile);
   if (QualifyFileName(aszPath))
   {
      LONG lRc;
      StripPathName(aszPath);                /* aszPath is now just the path */
      FillDirBox(hwnd, aszPath);
      lRc = SendDlgItemMessage( hwnd
                              , LB_FILES
                              , LB_SELECTSTRING
                              , (WPARAM)-1
                              , (LPARAM)FileName(aszFile)
                              );
      if (lRc==-1) {
          /* We couldn't find the file name in the list box.
          ** The most likely reason is that the file name contains a comma
          ** and we have stored the short file name in the registry.
          ** (storing file names with commas in the registry breaks the
          ** parsing scheme which is inherited from DOS.  Some apps go
          ** looking there so we need to keep compatibility.
          ** FindFirstFile is how you get the long name back
          */

          WIN32_FIND_DATA fd;
          FindFirstFile(aszFile, &fd);

          lRc = SendDlgItemMessage( hwnd
                                  , LB_FILES
                                  , LB_SELECTSTRING
                                  , (WPARAM)-1
                                  , (LPARAM)FileName(fd.cFileName)
                                  );
          if (lRc==-1) {
             /* Can't even find the long name - maybe there's crap in the registry
             ** or the file has been deleted.
             */

             SendDlgItemMessage(hwnd, LB_FILES, LB_SETCURSEL, (WPARAM)(-1), (LPARAM)0);
          }
      }
   } else
   {

      FillDirBox(hwnd, aszCurrentDir);
      SendDlgItemMessage( hwnd
                        , LB_FILES
                        , LB_SELECTSTRING
                        , (WPARAM)-1
                        , (LPARAM)(LPCSTR)aszNoSound
                        );
   }
   EnablePlay(hwnd);
}/* ShowSound */

/*---------------------------------------------------------------------------
| play the filename associated with a sound
 ----------------------------------------------------------------------------*/
static VOID NEAR PlayTheSound( HWND hwnd, int i)
{  char aszFile[STRSIZ];

   GetSoundEntry(hwnd, i, NULL, aszFile, NULL);
   // play the sound if it is not associated with <none>
   if (fSndPlaySound && *aszFile)
   {  BOOL fPlayed;
      HCURSOR hcur;

      hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
      fPlayed = sndPlaySound(aszFile, SND_ASYNC | SND_FILENAME);
#if PARANOIA
                                       Trace("Sound should have played", NULL);
#endif //DBG
      SetCursor(hcur);
      // The sound did not play, tell the user.
      if (!fPlayed)
         MessageBox(hwnd, aszErrorPlayMessage, aszErrorPlayTitle, MB_OK);
   }
}/* PlayTheSound */

/*---------------------------------------------------------------------------
| update list box.
| add/repl any <lszEvent>=... entry with <lszEvent>=<lszFile>,<lszDescription>
 ----------------------------------------------------------------------------*/
static VOID NEAR NewSound( HWND hwnd         /* dialog box window handle */
                         , LPCSTR lszEvent
                         , LPCSTR lszFile
                         , LPCSTR lszDescription
                         )
{
   PSTR szEntry;
   int  iSound;
   int  iNewSound;

   hwnd = GetDlgItem(hwnd, LB_NAMES);

   /*
   ** The original Win 31 did not check the return value from LocalAlloc
   ** i'll just return if we could not allocate the storage.
   */
   szEntry = (PSTR)LocalAlloc(LMEM_FIXED, lstrlen(lszEvent)
                                + lstrlen(lszFile)
                                + lstrlen(lszDescription) + 3);
   if ( szEntry == NULL ) {
       return;
   }

   iSound = (int)(LONG)SendMessage(hwnd, LB_FINDSTRING, (WPARAM)-1, (LPARAM)lszDescription);
   if (iSound == LB_ERR)
        iNewSound = (int)(DWORD)SendMessage(hwnd, LB_ADDSTRING,
                                            (WPARAM)0, (LPARAM)lszDescription);
   else {
        LocalFree((HLOCAL)(UINT)(DWORD)SendMessage(hwnd, LB_GETITEMDATA,
                                                   (WPARAM)iSound, (LPARAM)0));
        iNewSound = iSound;
   }


   wsprintf(szEntry, aszNewSoundFormat, lszEvent, lszFile, lszDescription);
   SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)iNewSound, (LPARAM)szEntry );
   if (iSound == LB_ERR)
   	SendMessage(hwnd, LB_SETCURSEL, (WPARAM)iNewSound, (LPARAM)0);

   if (iSound != LB_ERR)
      SetWindowRedraw(hwnd, TRUE);

}/* NewSound */

/*---------------------------------------------------------------------------
| change the file associated with a sound
 ----------------------------------------------------------------------------*/
static VOID NEAR ChangeSound(HWND hwnd,int iSound, LPSTR lszFile)
{  char aszName[STRSIZ];
   char aszDescription[STRSIZ];
   char aszShortName[STRSIZ];

   if (!QualifyFileName(lszFile))
      lszFile = aszNull;
   GetSoundEntry(hwnd, iSound, aszName, NULL, aszDescription);
   /* because long file names can contain commas which totally screw the parsing
      of the directory entries (basically <event>=<file>,<descr>), if there is
      a comma in the long file name, we put the short one in instead.
      If the short name also has a comma, then we're screwed.  Such file
      names are not allowed!  In this case we set the sound to <none>
   */
   if (0 != strchr(lszFile,',')) {
       GetShortPathName(lszFile, aszShortName, STRSIZ);
       lszFile = aszShortName;
       if (0 != strchr(lszFile,','))
          strcpy(lszFile, "<none>");
   }
   NewSound(hwnd, aszName, lszFile, aszDescription);
}/* ChangeSound */

/*---------------------------------------------------------------------------
| write out a new [sounds] section
 ----------------------------------------------------------------------------*/
static VOID NEAR WriteSounds(HWND hwnd)
{  int iSound;
   int iTotalSounds;
   BOOL bAllOK = TRUE; /* all sounds written OK */
   DWORD dwLastError = 0;

   /* The idea is to capture the error code of the first error that we see,
      to put up an error box in the event of any error (so that the user has
      some idea of what has happened), but to continue to the end anyway
      as it may be that it's still going to write all the data anyway
   */

   hwnd = GetDlgItem(hwnd, LB_NAMES);
   iTotalSounds = (int)(LONG)SendMessage( hwnd
                                        , LB_GETCOUNT
                                        , (WPARAM)0
                                        , (LPARAM)0
                                        );
   // delete the whole section;
   bAllOK = WriteProfileString(aszSounds, NULL, NULL);
   if (!bAllOK)
   {  dwLastError = GetLastError();
      if (dwLastError==259)   /* "No more items".  This is normal */
      {  bAllOK = TRUE;
         dwLastError = 0;
      }
   }

   //Write Enabled flag
   if(fEnabled)
   {  BOOL bOK = WriteProfileString(aszSounds, "Enable", "1");
      if (bAllOK && !bOK)
      {  dwLastError = GetLastError();
      }
      bAllOK =  bAllOK && bOK;

   }
   else
   {  BOOL bOK = WriteProfileString(aszSounds, "Enable", "0");
      if (bAllOK && !bOK)
      {  dwLastError = GetLastError();
      }
      bAllOK =  bAllOK && bOK;
   }

   for (iSound = 0; iSound < iTotalSounds; iSound++)
   {  char aszBuffer[STRSIZ];
      LPSTR   lszCur;
      BOOL    bOK;

      lstrcpy(aszBuffer, (PSTR)(UINT)(DWORD)SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iSound, (LPARAM)0));
      for (lszCur = aszBuffer; *lszCur != '='; lszCur++)
      ;
      *lszCur = (char)0;
      bOK = WriteProfileString(aszSounds, aszBuffer, lszCur + 1);
      if (bAllOK && !bOK)
      {  dwLastError = GetLastError();
      }
      bAllOK =  bAllOK && bOK;
   }
   if (!bAllOK)
   {   char szErr[STRSIZ];
       wsprintf(szErr, aszWriteErr, GetLastError());
       MessageBox(hwnd, szErr, aszAppName, MB_OK);
   }

   SendMessage( (HWND)-1
              , WM_WININICHANGE
              , (WPARAM)0
              , (LPARAM)(LPCSTR)aszSounds
              );
} /* WriteSounds */

/*---------------------------------------------------------------------------
|  Process control message from dialog box
 ----------------------------------------------------------------------------*/
static VOID PASCAL NEAR ControlMessage(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   switch ((WORD)wParam)
   {  HWND hwndFocus;

      case IDH_CHILD_SND:
         CPHelp(hwnd, dwContext);
         break;
      case IDOK:
         hwndFocus = GetFocus();
         if (hwndFocus && (GetDlgCtrlID(hwndFocus) == LB_FILES))
#if defined(WIN16)
            PostMessage( hwnd
                       , WM_COMMAND
                       , (WPARAM)LB_FILES
                       , MAKELPARAM(hwndFocus, LBN_DBLCLK)
                       );
#else
            PostMessage( hwnd
                       , WM_COMMAND
                       , (WPARAM)MAKELONG(LB_FILES,LBN_DBLCLK)
                       , (LPARAM)hwndFocus
                       );
#endif //WIN16
         else
            {  HCURSOR hcur;
               BOOL    fBeep;

               if (fIniChanged)
               {  if ( MessageBox( hwnd
                                 , aszWarningMessage
                                 , aszWarningTitle
                                 , MB_ICONEXCLAMATION | MB_OKCANCEL
                                 )
                     == IDCANCEL
                     )
                     {  //  PostMessage(hwnd, WM_INITDIALOG, 0, 0L);
                        //Bug #3298 -jyg-
                        break;
                     }
               }
               /* hourglass while we do things */
               hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

               /* write any sounds away. */
               if (fSndPlaySound)
                  sndPlaySound(NULL, 0);            // flush any cached sounds
#if PARANOIA
                                 Trace("Sound should have been flushed", NULL);
#endif //DBG
               WriteSounds(hwnd);

               /* Write the new beep setting */
               SystemParametersInfo(SPI_GETBEEP, 0, &fBeep, 0);
               if (fBeep != (BOOL)IsDlgButtonChecked(hwnd, ID_BEEP))
                  SystemParametersInfo( SPI_SETBEEP
                                      , !fBeep
                                      , NULL
                                      , SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE
                                      );
               /* restore pointer */
               SetCursor(hcur);
               EndDialog(hwnd, TRUE);
            }
            break;
      case IDCANCEL:
         if (fSndPlaySound)
            sndPlaySound(NULL, 0); // Shut up when the dialog exits
                                   // meaningful when we go async
#if PARANOIA
                                  Trace("Sound should have been killed", NULL);
#endif // DBG
         EndDialog(hwnd, FALSE);
         break;
      case ID_PLAY:
         PlayTheSound(hwnd, -1);
         break;
      case ID_BEEP:
         break;
      case LB_NAMES:
         switch(HIWORD(wParam))            /* NT change, was HIWORD(lParam) */
         {  case LBN_SELCHANGE:
               ShowSound(hwnd, -1);
               break;
            case LBN_DBLCLK:
               PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PLAY, (LPARAM)0);
               break;
         }
         break;
      case LB_FILES:
         switch(HIWORD(wParam))
         {  char aszSelection[STRSIZ];

            case LBN_SELCHANGE:
               if (!DlgDirSelectEx(hwnd, aszSelection, STRSIZ, LB_FILES))
                  ChangeSound(hwnd, -1, aszSelection);
               EnablePlay(hwnd);
               break;
            case LBN_DBLCLK:
               if (DlgDirSelectEx(hwnd, aszSelection, STRSIZ, LB_FILES))
                  FillDirBox(hwnd, aszSelection);
               else
                  PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PLAY, (LPARAM)0);
               break;
         }
         break;
   }
}/* ControlMessage */

/*---------------------------------------------------------------------------
| lszpath = lszpath [\] xlsz
| where xlsz is lsz with any leading drive: and .\ removed
 ----------------------------------------------------------------------------*/
static VOID NEAR PASCAL catpath( LPSTR  lszpath, LPCSTR lsz)
{
   // Remove any drive letters from the directory to append
   if (lsz[1] == ':')
      lsz += 2;
   // Remove any current directories ".\" from directory to append
   while (*lsz == '.' && SLASH(lsz[1]))
      lsz += 2;
   // Dont append a NULL string or a single "."
   if (*lsz && !(*lsz == '.' && !lsz[1]))
   {  /* add a "\" unless it already ends in "\" or ":" */
      if (  (!SLASH(lszpath[lstrlen(lszpath)-1]))
         && ((lszpath[lstrlen(lszpath)-1]) != ':')
         )
      lstrcat(lszpath, aszDirSeparator);
      lstrcat(lszpath, lsz);
   }
}/* catpath */

/*---------------------------------------------------------------------------
| Initialse the dialog
| fIniChanged = FALSE
 ----------------------------------------------------------------------------*/
static BOOL PASCAL NEAR InitDialog( HWND hwnd          /* dlg box window */
                                  )
{  char        aszBuffer[STRSIZ];
   WAVEOUTCAPS woCaps;
   LPSTR       lszSection;
   HANDLE      hgSection;
   BOOL        fBeep;
   BOOL        fSoundsExist = TRUE;


   fIniChanged = FALSE;

   // Determine if there is a wave device;
   fSndPlaySound = waveOutGetNumDevs()
                   && !waveOutGetDevCaps(0, &woCaps, sizeof(woCaps))
                   && woCaps.dwFormats != 0L;
   /*********************************************
   * fSndPlaySound
   * = (IDYES==MessageBox(NULL,"Pretend sound device available?","???",MB_YESNO));
   **********************************************/

   if (!fSndPlaySound)
      MessageBox(hwnd, aszNoDevice, aszErrorPlayTitle, MB_OK);

   // Set the Windows Enabled flag
   fEnabled = (1==GetProfileInt(aszSounds, "Enable", 0));

   /* fill the list box with the [sounds] section of WIN.INI; */
   if ((hgSection = GlobalAlloc(GMEM_MOVEABLE, (DWORD)MAXINI)) == NULL)
      return FALSE;
   lszSection = (LPVOID)GlobalLock(hgSection);
   if (0<(int)GetProfileString(aszSounds, NULL, aszNull, lszSection, MAXINI))
   {
       for (; *lszSection; lszSection += lstrlen(lszSection) + 1)
       {
          LPSTR  lszDescription;

          if (0>=(int)GetProfileString( aszSounds
                                      , lszSection
                                      , aszNull
                                      , aszBuffer
                                      , sizeof(aszBuffer)
                                      )
             ) break;
          /* split string by inserting null and get pointer to second part */
          lszDescription = FindDescription(aszBuffer);
          if (!*lszDescription)
             lszDescription = lszSection;
          if (0!=lstrcmpi(lszSection,"Enable"))
          {
              NewSound(hwnd, lszSection, aszBuffer, lszDescription);
          }
       }
   }
   else fSoundsExist = FALSE;
   GlobalUnlock(hgSection);
   GlobalFree(hgSection);

   //  Try to change to the windows "resource" directory;
   //  if that fails hit up the windows "system" directory;
   GetWindowsDirectory(aszBuffer, sizeof(aszBuffer));
   catpath(aszBuffer, aszResourceDir);
   if (!SetCurrentDirectory(aszBuffer))
   {  GetSystemDirectory(aszBuffer, sizeof(aszBuffer));
      SetCurrentDirectory(aszBuffer);
   }

   // select the first sound;  There might not be any courtesy SteveWo.
   if (fSoundsExist) ShowSound(hwnd, 0);

   //  Get the beep enabled flag from USER;
   SystemParametersInfo(SPI_GETBEEP, 0, (&fBeep), 0);
   CheckDlgButton(hwnd, ID_BEEP, fBeep);

}/* InitDialog */

/*---------------------------------------------------------------------------*/
static	VOID PASCAL NEAR Destroy(
	HWND	hwnd)
{
	int	iEvents;

	hwnd = GetDlgItem(hwnd, LB_NAMES);
	iEvents = (int)(DWORD)SendMessage(hwnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
	for (; iEvents--;)
		LocalFree((HLOCAL)(UINT)(DWORD)SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iEvents, (LPARAM)0));
}

/*---------------------------------------------------------------------------
| Dialog procedure for the dialog box
 ----------------------------------------------------------------------------*/
BOOL SoundDlg( HWND     hwnd
             , UINT     uMsg
             , WPARAM   wParam
             , LPARAM   lParam
             )
{
   switch (uMsg)
   {
      case WM_WININICHANGE:
         // See if someone has modified the sounds section or can't tell;
         if (  !lstrcmpi((LPCSTR)lParam, aszSounds)
            || !*((LPCSTR)lParam)
            )
            fIniChanged = TRUE;
         break;

      case WM_COMMAND:
         ControlMessage(hwnd, wParam, lParam);
         return TRUE;

      case WM_INITDIALOG:
         return InitDialog(hwnd);

      case WM_DESTROY:
         Destroy(hwnd);
         break;

      default:
         if (uMsg == uHelpMessage)
         {  CPHelp(hwnd, dwContext);
            return TRUE;
         }
         break;
   }
   return FALSE;
}/* SoundDlg */
