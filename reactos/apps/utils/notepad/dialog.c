/*
 *  Notepad (dialog.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  To be distributed under the Wine License
 */

#include <assert.h>
#include <stdio.h>
#include <windows.h>

/* temp for ROS */
#include "ros.h"

#include "main.h"
#include "license.h"
#include "language.h"
#include "dialog.h"

#define LCC_HASASSERT
#include "lcc.h"

static LRESULT WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);



int AlertIDS(UINT ids_message, UINT ids_caption, WORD type) {
  /*
   * Given some ids strings, this acts as a language-aware wrapper for 
   * "MessageBox"
   */
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szCaption[MAX_STRING_LEN];
   
   LoadString(Globals.hInstance, ids_message, szMessage, sizeof(szMessage));
   LoadString(Globals.hInstance, ids_caption, szCaption, sizeof(szCaption));
   
   return (MessageBox(Globals.hMainWnd, szMessage, szCaption, type));
}

void AlertFileNotFound(LPSTR szFileName) {

   int nResult;
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szRessource[MAX_STRING_LEN];

   /* Load and format szMessage */
   LoadString(Globals.hInstance, IDS_NOTFOUND, szRessource, sizeof(szRessource));
   wsprintf(szMessage, szRessource, szFileName);
   
   /* Load szCaption */
   LoadString(Globals.hInstance, IDS_ERROR,  szRessource, sizeof(szRessource));

   /* Display Modal Dialog */
   nResult = MessageBox(Globals.hMainWnd, szMessage, szRessource, MB_ICONEXCLAMATION);

}

int AlertFileNotSaved(LPSTR szFileName) {

   int nResult;
   CHAR szMessage[MAX_STRING_LEN];
   CHAR szRessource[MAX_STRING_LEN];

   /* Load and format Message */

   LoadString(Globals.hInstance, IDS_NOTSAVED, szRessource, sizeof(szRessource));
   wsprintf(szMessage, szRessource, szFileName);
   
   /* Load Caption */

   LoadString(Globals.hInstance, IDS_ERROR,  szRessource, sizeof(szRessource));

   /* Display modal */
   nResult = MessageBox(Globals.hMainWnd, szMessage, szRessource, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
   return(nResult);
}


VOID AlertOutOfMemory(void) {
   int nResult;
   
   nResult = AlertIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_ICONEXCLAMATION);
   PostQuitMessage(1);
}


BOOL ExistFile(LPCSTR szFilename) {
/*
 *  Returns: TRUE  - if "szFileName" exists
 *           FALSE - if it does not
 */
   WIN32_FIND_DATA entry;
   HANDLE hFile;
   
   hFile = FindFirstFile(szFilename, &entry);
   
   return (hFile!=INVALID_HANDLE_VALUE);
}

VOID DoSaveFile(VOID) {

    /* FIXME: Really Save the file */
    /* ... (Globals.szFileName); */
}


BOOL DoCloseFile(void) {
/* Return value: TRUE  - User agreed to close (both save/don't save) */
/*               FALSE - User cancelled close by selecting "Cancel" */

    int nResult;
   
    if (strlen(Globals.szFileName)>0) {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult) {
            case IDYES:     DoSaveFile();
                            break;

            case IDNO:      break;

            case IDCANCEL:  return(FALSE);
                            break;
                      
            default:        return(FALSE);
                            break;
        } /* switch */
    } /* if */
  
    /* Forget file name  */
    lstrcpy(Globals.szFileName, "");
    LANGUAGE_UpdateWindowCaption();
    return(TRUE);
}


void DoOpenFile(LPCSTR szFileName) {

    /* Close any files and prompt to save changes */
    if (DoCloseFile()) {
        GetFileTitle(szFileName, Globals.szFileName, sizeof(Globals.szFileName));
        LANGUAGE_UpdateWindowCaption();

        LoadBufferFromFile(szFileName);
    }
}


VOID DIALOG_FileNew(VOID)
{
    /* Close any files and promt to save changes */
    if (DoCloseFile()) {
        TrashBuffer();
    }
}

VOID DIALOG_FileOpen(VOID)
{
        OPENFILENAME openfilename;
        CHAR szPath[MAX_PATHNAME_LEN];
        CHAR szDir[MAX_PATHNAME_LEN];
        CHAR szzFilter[2 * MAX_STRING_LEN + 100];
        CHAR szDefaultExt[4];
        LPSTR p = szzFilter;

        lstrcpy(szDefaultExt, "txt");

        LoadString(Globals.hInstance, IDS_TEXT_FILES_TXT, p, MAX_STRING_LEN);
        p += strlen(p) + 1;
        lstrcpy(p, "*.txt");
        p += strlen(p) + 1;
        LoadString(Globals.hInstance, IDS_ALL_FILES, p, MAX_STRING_LEN);
        p += strlen(p) + 1;
        lstrcpy(p, "*.*");
        p += strlen(p) + 1;
        *p = '\0';

        GetCurrentDirectory(sizeof(szDir), szDir);
        lstrcpy(szPath,"*.txt");

        openfilename.lStructSize       = sizeof(OPENFILENAME);
        openfilename.hwndOwner         = Globals.hMainWnd;
        openfilename.hInstance         = Globals.hInstance;
        openfilename.lpstrFilter       = szzFilter;
        openfilename.lpstrCustomFilter = 0;
        openfilename.nMaxCustFilter    = 0;
        openfilename.nFilterIndex      = 0;
        openfilename.lpstrFile         = szPath;
        openfilename.nMaxFile          = sizeof(szPath);
        openfilename.lpstrFileTitle    = 0;
        openfilename.nMaxFileTitle     = 0;
        openfilename.lpstrInitialDir   = szDir;
        openfilename.lpstrTitle        = 0;
        openfilename.Flags             = OFN_FILEMUSTEXIST + OFN_PATHMUSTEXIST;
        openfilename.nFileOffset       = 0;
        openfilename.nFileExtension    = 0;
        openfilename.lpstrDefExt       = szDefaultExt;
        openfilename.lCustData         = 0;
        openfilename.lpfnHook          = 0;
        openfilename.lpTemplateName    = 0;

        if (GetOpenFileName(&openfilename)) {
                
                if (ExistFile(openfilename.lpstrFile))
                    DoOpenFile(openfilename.lpstrFile);
                else 
                    AlertFileNotFound(openfilename.lpstrFile);
                    
        }
}

VOID DIALOG_FileSave(VOID)
{
        /* FIXME: Save File */

        DIALOG_FileSaveAs();
}

VOID DIALOG_FileSaveAs(VOID)
{
        OPENFILENAME saveas;
        CHAR szPath[MAX_PATHNAME_LEN];
        CHAR szDir[MAX_PATHNAME_LEN];
        CHAR szDefaultExt[4];
        CHAR szzFilter[2 * MAX_STRING_LEN + 100];
       
        LPSTR p = szzFilter;

        lstrcpy(szDefaultExt, "txt");

        LoadString(Globals.hInstance, IDS_TEXT_FILES_TXT, p, MAX_STRING_LEN);
        p += strlen(p) + 1;
        lstrcpy(p, "*.txt");
        p += strlen(p) + 1;
        LoadString(Globals.hInstance, IDS_ALL_FILES, p, MAX_STRING_LEN);
        p += strlen(p) + 1;
        lstrcpy(p, "*.*");
        p += strlen(p) + 1;
        *p = '\0';

        lstrcpy(szPath,"*.*");

        GetCurrentDirectory(sizeof(szDir), szDir);

        saveas.lStructSize       = sizeof(OPENFILENAME);
        saveas.hwndOwner         = Globals.hMainWnd;
        saveas.hInstance         = Globals.hInstance;
        saveas.lpstrFilter       = szzFilter;
        saveas.lpstrCustomFilter = 0;
        saveas.nMaxCustFilter    = 0;
        saveas.nFilterIndex      = 0;
        saveas.lpstrFile         = szPath;
        saveas.nMaxFile          = sizeof(szPath);
        saveas.lpstrFileTitle    = 0;
        saveas.nMaxFileTitle     = 0;
        saveas.lpstrInitialDir   = szDir;
        saveas.lpstrTitle        = 0;
        saveas.Flags             = OFN_PATHMUSTEXIST + OFN_OVERWRITEPROMPT + OFN_HIDEREADONLY;
        saveas.nFileOffset       = 0;
        saveas.nFileExtension    = 0;
        saveas.lpstrDefExt       = szDefaultExt;
        saveas.lCustData         = 0;
        saveas.lpfnHook          = 0;
        saveas.lpTemplateName    = 0;

        if (GetSaveFileName(&saveas)) {
            lstrcpy(Globals.szFileName, saveas.lpstrFile);
            LANGUAGE_UpdateWindowCaption();
            DIALOG_FileSave();
        }
}

VOID DIALOG_FilePrint(VOID)
{
        LONG bFlags, nBase;
        WORD nOffset;
        DOCINFO di;
        int nResult;
        HDC hContext;
        PRINTDLG printer;

        CHAR szDocumentName[MAX_STRING_LEN]; /* Name of document */
        CHAR szPrinterName[MAX_STRING_LEN];  /* Name of the printer */
        CHAR szDeviceName[MAX_STRING_LEN];   /* Name of the printer device */
        CHAR szOutput[MAX_STRING_LEN];       /* in which file/device to print */

/*        LPDEVMODE  hDevMode;   */
/*        LPDEVNAMES hDevNames; */

/*        hDevMode  = GlobalAlloc(GMEM_MOVEABLE + GMEM_ZEROINIT, sizeof(DEVMODE)); */
/*        hDevNames = GlobalAlloc(GMEM_MOVEABLE + GMEM_ZEROINIT, sizeof(DEVNAMES)); */

        /* Get Current Settings */

        printer.lStructSize           = sizeof(PRINTDLG);
        printer.hwndOwner             = Globals.hMainWnd;
        printer.hInstance             = Globals.hInstance;

        /* Let PrintDlg create a DEVMODE structure */
        printer.hDevMode              = 0;
        printer.hDevNames             = 0;
        printer.hDC                   = 0;
        printer.Flags                 = PD_RETURNDEFAULT;
        printer.nFromPage             = 0;
        printer.nToPage               = 0;
        printer.nMinPage              = 0;
        printer.nMaxPage              = 0;
        printer.nCopies               = 0;
        printer.lCustData             = 0;
        printer.lpfnPrintHook         = 0;
        printer.lpfnSetupHook         = 0;
        printer.lpPrintTemplateName   = 0;
        printer.lpSetupTemplateName   = 0;
        printer.hPrintTemplate        = 0;
        printer.hSetupTemplate        = 0;

        nResult = PrintDlg(&printer);

/*        hContext = CreateDC(, szDeviceName, "TEST.TXT", 0); */

        /* Congratulations to those Microsoft Engineers responsable */
        /* for the following pointer acrobatics */

        assert(printer.hDevNames!=0);

        nBase = (LONG)(printer.hDevNames);

        nOffset = (WORD)((LPDEVNAMES) printer.hDevNames)->wDriverOffset;
        lstrcpy(szPrinterName, (LPCSTR) (nBase + nOffset));

        nOffset = (WORD)((LPDEVNAMES) printer.hDevNames)->wDeviceOffset;
        lstrcpy(szDeviceName, (LPCSTR) (nBase + nOffset));

        nOffset = (WORD)((LPDEVNAMES) printer.hDevNames)->wOutputOffset;
        lstrcpy(szOutput, (LPCSTR) (nBase + nOffset));

        MessageBox(Globals.hMainWnd, szPrinterName, "Printer Name", MB_ICONEXCLAMATION);
        MessageBox(Globals.hMainWnd, szDeviceName,  "Device Name",  MB_ICONEXCLAMATION);
        MessageBox(Globals.hMainWnd, szOutput,      "Output",       MB_ICONEXCLAMATION);

        /* Set some default flags */

        bFlags = PD_RETURNDC + PD_SHOWHELP;

        if (TRUE) {
             /* Remove "Print Selection" if there is no selection */
             bFlags = bFlags + PD_NOSELECTION;
        } 

        printer.Flags                 = bFlags;
/*
        printer.nFromPage             = 0;
        printer.nToPage               = 0;
        printer.nMinPage              = 0;
        printer.nMaxPage              = 0;
*/

        /* Let commdlg manage copy settings */
        printer.nCopies               = (WORD)PD_USEDEVMODECOPIES;

        if (PrintDlg(&printer)) {

            /* initialize DOCINFO */
            di.cbSize = sizeof(DOCINFO);
            lstrcpy((LPSTR)di.lpszDocName, szDocumentName);
            lstrcpy((LPSTR)di.lpszOutput,  szOutput);

            hContext = printer.hDC;
            assert(hContext!=0);
            assert( (int) hContext!=PD_RETURNDC);

            SetMapMode(hContext, MM_LOMETRIC);
/*          SetViewPortExExt(hContext, 10, 10, 0); */
            SetBkMode(hContext, OPAQUE);

            nResult = TextOut(hContext, 0, 0, " ", 1);
            assert(nResult != 0);

            nResult = StartDoc(hContext, &di);
            assert(nResult != SP_ERROR);

            nResult = StartPage(hContext);
            assert(nResult >0);

            /* FIXME: actually print */

            nResult = EndPage(hContext);

            switch (nResult) {
               case SP_ERROR:
                       MessageBox(Globals.hMainWnd, "Generic Error", "Print Engine Error", MB_ICONEXCLAMATION);
                       break;
               case SP_APPABORT:
                       MessageBox(Globals.hMainWnd, "The print job was aborted.", "Print Engine Error", MB_ICONEXCLAMATION);
                       break;
               case SP_USERABORT:
                       MessageBox(Globals.hMainWnd, "The print job was aborted using the Print Manager ", "Print Engine Error", MB_ICONEXCLAMATION);
                       break;
               case SP_OUTOFDISK:
                       MessageBox(Globals.hMainWnd, "Out of disk space", "Print Engine Error", MB_ICONEXCLAMATION);
                       break;
               case SP_OUTOFMEMORY:
                       AlertOutOfMemory();
                       break;
               default:
                       MessageBox(Globals.hMainWnd, "Default", "Print", MB_ICONEXCLAMATION); 
            } /* switch */
            nResult = EndDoc(hContext);
            assert(nResult>=0);
            nResult = DeleteDC(hContext);
            assert(nResult!=0);
        } /* if */

/*       GlobalFree(hDevNames); */
/*       GlobalFree(hDevMode); */
}

VOID DIALOG_FilePageSetup(VOID)
{
        DIALOG_PageSetup();
}

VOID DIALOG_FilePrinterSetup(VOID)
{
        PRINTDLG printer;

        printer.lStructSize          = sizeof(PRINTDLG);
        printer.hwndOwner            = Globals.hMainWnd;
        printer.hInstance            = Globals.hInstance;
        printer.hDevMode             = 0;
        printer.hDevNames            = 0;
        printer.hDC                  = 0;
        printer.Flags                = PD_PRINTSETUP;
        printer.nFromPage            = 0;
        printer.nToPage              = 0;
        printer.nMinPage             = 0;
        printer.nMaxPage             = 0;
        printer.nCopies              = 1;
        printer.lCustData            = 0;
        printer.lpfnPrintHook        = 0;
        printer.lpfnSetupHook        = 0;
        printer.lpPrintTemplateName  = 0;
        printer.lpSetupTemplateName  = 0;
        printer.hPrintTemplate       = 0;
        printer.hSetupTemplate       = 0;
        
        if (PrintDlg(&printer)) {
            /* do nothing */
        };

}

VOID DIALOG_FileExit(VOID)
{
        if (DoCloseFile()) {
               PostQuitMessage(0);
        }
}

VOID DIALOG_EditUndo(VOID)
{
   MessageBox(Globals.hMainWnd, "Undo", "Debug", MB_ICONEXCLAMATION);
        /* undo */
}

VOID DIALOG_EditCut(VOID)
{
    HANDLE hMem;

    hMem = GlobalAlloc(GMEM_ZEROINIT, 99);

    OpenClipboard(Globals.hMainWnd);
    EmptyClipboard();

    /* FIXME: Get text */
    lstrcpy((CHAR *)hMem, "Hello World");

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    GlobalFree(hMem);
}

VOID DIALOG_EditCopy(VOID)
{
    HANDLE hMem;

    hMem = GlobalAlloc(GMEM_ZEROINIT, 99);

    OpenClipboard(Globals.hMainWnd);
    EmptyClipboard();

    /* FIXME: Get text */
    lstrcpy((CHAR *)hMem, "Hello World");

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    GlobalFree(hMem);
}

VOID DIALOG_EditPaste(VOID)
{
    HANDLE hClipText;

    if (IsClipboardFormatAvailable(CF_TEXT)) {
        OpenClipboard(Globals.hMainWnd);
        hClipText = GetClipboardData(CF_TEXT);
        CloseClipboard();
        MessageBox(Globals.hMainWnd, (CHAR *)hClipText, "PASTE", MB_ICONEXCLAMATION);
    }
}

VOID DIALOG_EditDelete(VOID)
{
        /* Delete */
}

VOID DIALOG_EditSelectAll(VOID)
{
        /* Select all */
}

VOID DIALOG_EditTimeDate(VOID)
{
    SYSTEMTIME   st;
    LPSYSTEMTIME lpst = &st;
    CHAR         szDate[MAX_STRING_LEN];
    LPSTR        date = szDate;
    
    GetLocalTime(&st);
    GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, lpst, NULL, date, MAX_STRING_LEN);
    GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, lpst, NULL, date, MAX_STRING_LEN);

}

VOID DIALOG_EditWrap(VOID)
{
        Globals.bWrapLongLines = !Globals.bWrapLongLines;
        CheckMenuItem(Globals.hEditMenu, NP_EDIT_WRAP, MF_BYCOMMAND | 
        (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
}

VOID DIALOG_Search(VOID)
{
        Globals.find.lStructSize      = sizeof(Globals.find);
        Globals.find.hwndOwner        = Globals.hMainWnd;
        Globals.find.hInstance        = Globals.hInstance;
        Globals.find.lpstrFindWhat    = (CHAR *) &Globals.szFindText;
        Globals.find.wFindWhatLen     = sizeof(Globals.szFindText);
        Globals.find.lpstrReplaceWith = 0;
        Globals.find.wReplaceWithLen  = 0;
        Globals.find.Flags            = FR_DOWN;
        Globals.find.lCustData        = 0;
        Globals.find.lpfnHook         = 0;
        Globals.find.lpTemplateName   = 0;

        /* We only need to create the modal FindReplace dialog which will */
        /* notify us of incoming events using hMainWnd Window Messages    */

        Globals.hFindReplaceDlg = FindText(&Globals.find);
        assert(Globals.hFindReplaceDlg !=0);
}

VOID DIALOG_SearchNext(VOID)
{
        /* Search Next */
}

VOID DIALOG_HelpContents(VOID)
{
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
        /* Search Help */
}

VOID DIALOG_HelpHelp(VOID)
{
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_HELPONHELP, 0);
}

VOID DIALOG_HelpLicense(VOID)
{
        WineLicense(Globals.hMainWnd);
}

VOID DIALOG_HelpNoWarranty(VOID)
{
        WineWarranty(Globals.hMainWnd);
}

VOID DIALOG_HelpAboutWine(VOID)
{
        CHAR szNotepad[MAX_STRING_LEN];

        LoadString(Globals.hInstance, IDS_NOTEPAD, szNotepad, sizeof(szNotepad));
        ShellAbout(Globals.hMainWnd, szNotepad, "Notepad\n" /* WINE_RELEASE_INFO */, 0);
}

/***********************************************************************
 *
 *           DIALOG_PageSetup
 */

VOID DIALOG_PageSetup(VOID)
{
  WNDPROC lpfnDlg;

  lpfnDlg = MakeProcInstance(DIALOG_PAGESETUP_DlgProc, Globals.hInstance);
  DialogBox(Globals.hInstance, STRING_PAGESETUP_Xx, Globals.hMainWnd, (DLGPROC)lpfnDlg);
  FreeProcInstance(lpfnDlg);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_DlgProc
 */

static LRESULT WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

   switch (msg)
    {
    case WM_COMMAND:
      switch (wParam)
        {
        case IDOK:
          /* save user input and close dialog */
          GetDlgItemText(hDlg, NP_PAGESETUP_HEAD,   Globals.szHeader,       sizeof(Globals.szHeader));
          GetDlgItemText(hDlg, NP_PAGESETUP_TAIL,   Globals.szFooter,       sizeof(Globals.szFooter));
          GetDlgItemText(hDlg, NP_PAGESETUP_TOP,    Globals.szMarginTop,    sizeof(Globals.szMarginTop));
          GetDlgItemText(hDlg, NP_PAGESETUP_BOTTOM, Globals.szMarginBottom, sizeof(Globals.szMarginBottom));
          GetDlgItemText(hDlg, NP_PAGESETUP_LEFT,   Globals.szMarginLeft,   sizeof(Globals.szMarginLeft));
          GetDlgItemText(hDlg, NP_PAGESETUP_RIGHT,  Globals.szMarginRight,  sizeof(Globals.szMarginRight));
          EndDialog(hDlg, IDOK);
          return TRUE;

        case IDCANCEL:
          /* discard user input and close dialog */
          EndDialog(hDlg, IDCANCEL);
          return TRUE;

        case IDHELP:
          /* FIXME: Bring this to work */
          MessageBox(Globals.hMainWnd, "Sorry, no help available", "Help", MB_ICONEXCLAMATION);
          return TRUE;
        }
      break;

    case WM_INITDIALOG:
       /* fetch last user input prior to display dialog */
       SetDlgItemText(hDlg, NP_PAGESETUP_HEAD,   Globals.szHeader);
       SetDlgItemText(hDlg, NP_PAGESETUP_TAIL,   Globals.szFooter);
       SetDlgItemText(hDlg, NP_PAGESETUP_TOP,    Globals.szMarginTop);
       SetDlgItemText(hDlg, NP_PAGESETUP_BOTTOM, Globals.szMarginBottom);
       SetDlgItemText(hDlg, NP_PAGESETUP_LEFT,   Globals.szMarginLeft);
       SetDlgItemText(hDlg, NP_PAGESETUP_RIGHT,  Globals.szMarginRight);
       break;
    }

  return FALSE;
}
