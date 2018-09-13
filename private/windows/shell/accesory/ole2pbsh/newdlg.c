/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   NEWDLG.c                                    *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  New common dialog interface                 *
*   date:   12/23/90 @ 15:12                            *
*                                                       *
********************************************************/

#include <windows.h>
#include <port1632.h>

#include "dlgs.h"

#include "oleglue.h"
#include "pbrush.h"


/* variables for the new File Open,File SaveAs and Find Text dialogs */
OPENFILENAME OFN;

static TCHAR szTemplate [CAPTIONMAX];
static TCHAR szTitle    [CAPTIONMAX];
static TCHAR szFileName [MAX_PATH];
static TCHAR szLastDir  [MAX_PATH];
static TCHAR szSaveFilter[FILTERMAX * 6];    /* default filter spec. for above    */
static TCHAR szOpenFilter[FILTERMAX * 4];
static TCHAR szColorFilter[FILTERMAX * 2];
static TCHAR szCustFilterSpec[FILTERMAX];     /* buffer for custom filters created */

/* captions for dialog boxes with DS_MODALFRAME ATTRIBUTES */
static UINT DialogCaption[] = { IDSSaveAs, IDSFileOpen, IDSPasteFrom,
                                IDSCopyTo, IDSGetColors, IDSSaveColors };
static int Dialog[]         = { FileSave, FileOpen, FileOpen,
                                FileSave, FileOpen, FileSave };
static TCHAR *Filter[]      = { szSaveFilter, szOpenFilter, szOpenFilter,
                                szSaveFilter, szColorFilter, szColorFilter };

static TCHAR *OpenExt[] = { TEXT("*.BMP;*.DIB"), TEXT("*.MSP"), TEXT("*.PCX"), TEXT("*.*") };
static TCHAR OpenExtDef[4] = { TEXT('\0') };
static TCHAR *SaveExt[] = { TEXT("*.PCX"), TEXT("*.BMP"), TEXT("*.BMP"), TEXT("*.BMP"), TEXT("*.BMP") };
static TCHAR *ColorExt[] = { TEXT("*.PAL") };

extern WORD wFileType;
extern int fileMode;
extern TCHAR *namePtr, *pathPtr;
extern int DlgCaptionNo;
extern BITMAPFILEHEADER_VER1 BitmapHeader;
extern RECT pickRect;
extern int imagePlanes, imagePixels;
extern int imageWid, imageHgt;

#ifndef WIN32
typedef unsigned int * PUINT;
#endif

static void LoadFilters(LPTSTR pszFilter, PUINT Id, LPTSTR Extension[]);
static void ParseFileName(LPTSTR szTemp);

/* This is also in FileDlg.C */
BOOL PRIVATE MyStrChr(TCHAR *s, TCHAR *t)
{
   TCHAR *pt;

   for( ; *s; ++s)
      for(pt = t; *pt; ++pt)
         if(*s == *pt)
            return TRUE;

   return FALSE;
}

BOOL PUBLIC InitNewDialogs(HINSTANCE hInst) {
    /* construct default filter string in the required format for
     * the new FileOpen and FileSaveAs dialogs.  Pretty brutish...
     */
    static UINT OpenIds[] = { IDS_BMPFILTER, IDS_MSPFILTER, IDS_PCXFILTER, IDS_ALLFILTER, 0 };
    static UINT ColorIds[] = { IDS_COLORFILTER, 0 };
    static UINT SaveIds[] = { IDS_PCXFILTER, IDS_MONOBMPFILTER,
          IDS_16COLORFILTER, IDS_256COLORFILTER, IDS_24BITFILTER, 0 };

    LoadFilters(szOpenFilter, OpenIds, OpenExt);
    LoadFilters(szColorFilter, ColorIds, ColorExt);
    LoadFilters(szSaveFilter, SaveIds, SaveExt);

    *szCustFilterSpec = TEXT('\0');

    /* init. some fields of the OPENFILENAME struct used by fileopen and
     * filesaveas
     */
    OFN.lStructSize       = sizeof(OPENFILENAME);
    OFN.hInstance         = hInst;
    OFN.nMaxCustFilter    = FILTERMAX;
    OFN.nFilterIndex      = 1;
    OFN.nMaxFile          = MAX_PATH;
#ifndef WIN32
    OFN.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance((FARPROC)FileDialog, hInst);
#else
    OFN.lpfnHook = (LPOFNHOOKPROC)FileDialog;
#endif
    OFN.lCustData         = 0L;
    OFN.lpstrDefExt       = OpenExtDef;

    return TRUE;
}


void ParseFileName(LPTSTR pszFullName) {
    LPTSTR pszFileName;
    LPTSTR pszBackslash;

    /* Locate the last backslash.  "C:<filename>" isn't possible. */
    pszBackslash = NULL;
    pszFileName = pszFullName;
    while (*pszFileName) {
        if (*pszFileName == TEXT('\\'))
            pszBackslash = pszFileName;
        pszFileName = CharNext(pszFileName);
    }

    /* Save away the file name and path */
    if (pszBackslash) {
        *pszBackslash = (TCHAR) 0;
        lstrcpy(pathPtr, pszFullName);

        DB_OUT(pathPtr);
        lstrcpy(namePtr, pszBackslash + 1);

        DB_OUT(namePtr);
        *pszBackslash = TEXT('\\');

        /* Special case for the root path specified */
        if( pathPtr[ 1 ] == TEXT(':') && (pathPtr[ 2 ] == TEXT('\0'))) {
            pathPtr[ 2 ] = TEXT('\\');
            pathPtr[ 3 ] = TEXT('\0');
        }
    } else {
        *pathPtr = (TCHAR) 0;
        lstrcpy(namePtr, pszFullName);
    }
}


BOOL NEAR PASCAL GetExtFromInd(int i)
{
  LPTSTR lpstrFilter;

  /* Skip to the appropriate filter */
  lpstrFilter = (LPTSTR)OFN.lpstrFilter;
  goto CheckFilter;

  do {
      lpstrFilter += lstrlen(lpstrFilter)+1;
      lpstrFilter += lstrlen(lpstrFilter)+1;

CheckFilter:
      if (!*lpstrFilter)
          return(FALSE);
  } while (i--) ;

  /* If we got to the filter, retrieve the extension */
  /* Skip past the "*." */
  lpstrFilter += lstrlen(lpstrFilter)+3;

  /* Copy the extension, if not wildcard. */
  if (*lpstrFilter == TEXT('*'))
      return(FALSE);

  for (i=0; i<3; ++i, ++lpstrFilter)
    {
      if (!*lpstrFilter || *lpstrFilter==TEXT(';'))
          break;
      OpenExtDef[i] = *lpstrFilter;
    }
  OpenExtDef[i] = TEXT('\0');

  return(TRUE);
}


static void NEAR PASCAL DisplayFileInfo(HWND hDlg)
{
  TCHAR s[MAX_PATH];
  int i;

  if (!GetDlgItemText(hDlg, edt1, s, CharSizeOf(s)))
      return;
  CharUpper(s);

  switch (DlgCaptionNo)
    {
      case COPYTO:
        BitmapHeader.wid = abs(pickRect.right - pickRect.left + 1);
        BitmapHeader.hgt = abs(pickRect.bottom - pickRect.top + 1);
        BitmapHeader.planes = (BYTE) imagePlanes;
        BitmapHeader.bitcount = (BYTE) imagePixels;
        i = 1;
        break;

      case FILESAVE:
        BitmapHeader.wid = imageWid;
        BitmapHeader.hgt = imageHgt;
        BitmapHeader.planes = (BYTE)imagePlanes;
        BitmapHeader.bitcount = (BYTE)imagePixels;
        i = 1;
        break;

      default:
        if (MyStrChr(s, TEXT("*?")))       /* Wild card... */
            return;

        switch (PBGetFileType(s))
          {
            case BITMAPFILE:
            case BITMAPFILE4:
            case BITMAPFILE8:
            case BITMAPFILE24:
                i = GetBitmapInfo(hDlg);
                break;

            case MSPFILE:
                i = GetMSPInfo(hDlg);
                break;

            default:
                i = GetInfo(hDlg);
                break;
          }
        break;
    }

  if (i)
    {
      i = wFileType;
      wFileType = ((DlgCaptionNo == COPYTO) || (DlgCaptionNo == FILESAVE))
                                           ? BITMAPFILE
                                           : PBGetFileType(s);
      DoDialog(INFOBOX, hDlg, (WNDPROC)InfoDlg);
      wFileType = i;
    }
}


/* Change the extension on a file name in the save dialog box
 */
void NEAR PASCAL NewExtension(HWND hDlg, int i)
{
  TCHAR szName[MAX_PATH];

  GetDlgItemText(hDlg, edt1, szName, sizeof(szName));
  ReplaceExtension(szName, i);
  SetDlgItemText(hDlg, edt1, szName);
}


/* This is now a subclass of the standard file dialog routine */
BOOL FAR PASCAL FileDialog(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
  int i;
#ifdef JAPAN        // added by Hiraisi (BUGFIX #1856 WIN31)
  static TCHAR s[_MAX_PATH];
  BOOL PUBLIC bFileExists(LPTSTR lpFilename);
#endif

  switch (message)
    {
      case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam,lParam))
          {
            case cmb1:
              if (GET_WM_COMMAND_CMD(wParam,lParam) == CBN_SELCHANGE)
                {
                  GetExtFromInd(i=(int)SendDlgItemMessage(hDlg, cmb1,
                        CB_GETCURSEL, 0, 0L));

                  if (DlgCaptionNo==COPYTO || DlgCaptionNo==FILESAVE)
                      NewExtension(hDlg, i);
                }
              break;

            case IDINFO:
              DisplayFileInfo(hDlg);
              break;

#ifdef JAPAN        // added by Hiraisi (BUGFIX #1856 WIN31)
            case edt1:
               if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_CHANGE) {
                   if ((DlgCaptionNo != COLORLOAD)
                       && (DlgCaptionNo != COLORSAVE)
                       && (DlgCaptionNo != FILESAVE)
                       && (DlgCaptionNo != COPYTO)
                       && (GetDlgItemText(hDlg,edt1,s,FILENAMElen - 1))
                       &&  bFileExists(s))
                              EnableWindow(GetDlgItem(hDlg, IDINFO), TRUE);
                   else
                              EnableWindow(GetDlgItem(hDlg, IDINFO), FALSE);
               }
               break;
#endif
          }
        break;

      case WM_INITDIALOG:
#ifdef JAPAN        // added by Hiraisi (BUGFIX #1856 WIN31)
        if ((DlgCaptionNo != COLORLOAD)
            && (DlgCaptionNo != COLORSAVE)
            && (DlgCaptionNo != FILESAVE)
            && (DlgCaptionNo != COPYTO)
            && (GetDlgItemText(hDlg,edt1,s,FILENAMElen - 1)
            && bFileExists(s)))
                EnableWindow(GetDlgItem(hDlg, IDINFO), TRUE);
        else
                EnableWindow(GetDlgItem(hDlg, IDINFO), FALSE);
#else
        switch (DlgCaptionNo)
          {
            case COLORLOAD:
            case COLORSAVE:
              EnableWindow(GetDlgItem(hDlg, IDINFO), FALSE);
              break;

            default:
              break;
          }
#endif
        return TRUE;
        break;

#ifdef JAPAN        // added by Hiraisi (BUGFIX #1856 WIN31)
      case WM_CTLCOLOR:
        if ((DlgCaptionNo != COLORLOAD)
            && (DlgCaptionNo != COLORSAVE)
            && (DlgCaptionNo != FILESAVE)
            && (DlgCaptionNo != COPYTO))
        {
            if (HIWORD(lParam) == CTLCOLOR_STATIC)
            {
               if (GetDlgItemText(hDlg,edt1,s,FILENAMElen - 1)
                   && bFileExists(s))
                      EnableWindow(GetDlgItem(hDlg, IDINFO), TRUE);
               else
                      EnableWindow(GetDlgItem(hDlg, IDINFO), FALSE);
            }
        }
        break;
#endif
      default:
        return FALSE;
    }

  return FALSE;
}

BOOL PUBLIC DoFileDialog(int id, HWND hwnd) {
    REGISTER BOOL result;
    TCHAR szTemp[MAX_PATH];
    DWORD rc;

    /* Set up dialog specific items (title, default filter, ...) */
    LoadString(hInst, DialogCaption[DlgCaptionNo], szTitle, CharSizeOf(szTitle));
    OFN.hwndOwner      = hwnd;
    OFN.lpTemplateName = (LPTSTR) MAKEINTRESOURCE(Dialog[DlgCaptionNo]);
    OFN.lpstrFilter    = Filter[DlgCaptionNo];
    OFN.lpstrTitle        = szTitle;
    OFN.lpstrFileTitle    = NULL;
    OFN.lpstrCustomFilter = szCustFilterSpec;
    OFN.lpstrInitialDir   = szLastDir;
    OFN.Flags             = OFN_ENABLETEMPLATE | OFN_ENABLEHOOK |
                            OFN_HIDEREADONLY;

    OFN.nFilterIndex = 1;

    switch (DlgCaptionNo) {
        case FILELOAD:
        case PASTEFROM:
            switch (wFileType) {
                case MSPFILE:
                  OFN.nFilterIndex = 2;
                  break;

                case PCXFILE:
                  OFN.nFilterIndex = 3;
                  break;
            }

            /* Fall through */
        case COLORLOAD:
            OFN.Flags   |= OFN_FILEMUSTEXIST;
            break;

        case COPYTO:
        case FILESAVE:
            {
              HDC hDC;
              WORD twFileType;

              if (hDC = GetWindowDC(pbrushWnd[PAINTid])) {
                  twFileType = GetImageFileType(GetDeviceCaps(hDC, BITSPIXEL)
                        * GetDeviceCaps(hDC, PLANES));
                  ReleaseDC(pbrushWnd[PAINTid], hDC);
              } else
                  twFileType = 1;

              if (wFileType > twFileType
                    && BITMAPFILE <= wFileType && wFileType <= BITMAPFILE24)
                  wFileType = twFileType;
            }

            /* Set the correct filter index */
            OFN.nFilterIndex = wFileType + 1;

            /* Fall through */
        case COLORSAVE:
            OFN.Flags   |= OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
            break;
    }

    GetExtFromInd(LOWORD(OFN.nFilterIndex)-1);

    /* Retrieve the starting file name */
    if (gfLinked)
        lstrcpy(szTemp, namePtr);
    else
        *szTemp = TEXT('\0');

    OFN.lpstrFile      = szTemp;

DoFileDialogAgain:
    result = ((Dialog[DlgCaptionNo] == FileOpen)
                ? GetOpenFileName((LPOPENFILENAME)&OFN)
                : GetSaveFileName((LPOPENFILENAME)&OFN));

        rc = CommDlgExtendedError();
        if(rc)
                SimpleMessage(IDSNotEnufMem, NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);

    if (result) {
        ParseFileName(szTemp);

        switch (id) {
            case LOADBOX:
                switch (OFN.nFilterIndex) {
                    case 2: wFileType = MSPFILE; break;
                    case 3: wFileType = PCXFILE; break;
                    default: wFileType = (WORD)PBGetFileType(szTemp); break;
                }
                break;

            case SAVEBOX:
                /* Don't allow saving of MSP files */
                if (LOWORD(OFN.nFilterIndex) >= 1
                      && LOWORD(OFN.nFilterIndex) <= 5)
                    wFileType = (WORD)OFN.nFilterIndex - 1;
                else
                    wFileType = PBGetFileType(szTemp);

                /* Fall through */
            case CLRSVBOX:
                if (!SaveFileNameOK(pathPtr, namePtr))
                  {
                    lstrcpy(szTemp, namePtr);
                    goto DoFileDialogAgain;
                  }
                break;

            default:
                break;
        }
    }

    return result;
}

static void LoadFilters(LPTSTR pszFilter, PUINT Id, LPTSTR Extension[])
{
    while (*Id) {
        LoadString(hInst, *Id, pszFilter, FILTERMAX);
        pszFilter += lstrlen (pszFilter);
        *pszFilter++ = TEXT(' ');
        *pszFilter++ = TEXT('(');
        lstrcpy(pszFilter, *Extension);
        pszFilter += lstrlen (pszFilter);
        *pszFilter++ = TEXT(')');
        *pszFilter++ = 0;
        lstrcpy(pszFilter, *Extension);
        pszFilter += lstrlen(pszFilter) + 1;

        Id++; Extension++;
    }

    *pszFilter = TEXT('\0');
}

