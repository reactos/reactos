//
//  (C) Copyright MICROSOFT Corp., 1994
//
//  TITLE:       FILETYPE.C
//  VERSION:     1.0
//  DATE:        5/10/94
//  AUTHOR:      Vince Roggero (vincentr)
//
//================================================================
//
//  CHANGE LOG:
//
//  DATE         REV DESCRIPTION
//  -----------  --- ---------------------------------------------
//               VMR Original version
//================================================================

//
// Note: this file has been moved from explorer to shell32, then
//       to shdocvw.  It works like this:
//
//          Win95           - implemented in explorer
//          IE 1.0 - 3.0    - implemented in URL.DLL with MIME type support
//          Old Nashville   - moved to shell32, w/ MIME type support
//          NT 4            - ditto
//          IE4             - moved to shdocvw
//          NT5             - moved to shdoc401 and back to shell32
//
// Since there are some UNICODE issues with this file (that is,
// there are a bunch of unicode/ansi bugs we'd have to address
// if we try to run the ANSI version on NT), we just build this
// file twice and package both code bases into shdocvw.
// 


//================================================================
//  View.Options.File Types
//================================================================

// BUGBUG - need to be able to handle case where user enters exe without an ext during EditCommand Dialog

#include "priv.h"
#include "filetype.h"
#include "resource.h"
#include <help.h>       // Help IDs
#include <shellids.h>  // more ids
#include <limits.h>

#include <mluisupp.h>

#define TF_FILETYPE     0

#define FILETYPESHEETS          1
#define MAXTEXTLEN              32
#define NUMITEMS                7
#define NUMCOLUMNS              2
#define MAXCOLUMNHDG            64
#define MAXEXTSIZE              (64+2)
#define FTD_EDIT                1
#define FTD_EXT                 2
#define FTD_COMMAND             3
#define FTD_DOCICON             4
#define FTD_MIME                5
#define SHELLEXEICONINDEX       2
#define SHELLDEFICONINDEX       44

#define WM_CTRL_SETFOCUS        WM_USER + 1

/* File Type */
#define IDH_FILETYPE_CONTENT_TYPE                   0x1025
#define IDH_FILETYPE_OPENS_WITH                     0x1026
#define IDH_NEW_FILETYPE_CONTENT_TYPE               0x1027
#define IDH_NEWFILETYPE_DEFAULT_EXT                 0x1028
#define IDH_FILETYPE_EXTENSION                      0x1029
#define IDH_FILETYPE_CONFIRM_OPEN                   0x104b


#ifdef UNICODE
#define FT_DlgProc          FT_DlgProcW
#define FTCmd_DlgProc       FTCmd_DlgProcW
#define FTEdit_DlgProc      FTEdit_DlgProcW
#define CreateFileTypePage  CreateFileTypePageW
#define FileType_Callback   FileType_CallbackW
#define IDS_PROGRAMSFILTER  IDS_PROGRAMSFILTER_NT
#else
#define FT_DlgProc          FT_DlgProcA
#define FTCmd_DlgProc       FTCmd_DlgProcA
#define FTEdit_DlgProc      FTEdit_DlgProcA
#define CreateFileTypePage  CreateFileTypePageA
#define FileType_Callback   FileType_CallbackA
#define IDS_PROGRAMSFILTER  IDS_PROGRAMSFILTER_WIN95
#endif

//
// NOTE:
//      we can call these api's even though they were TCHAR
//      exports because we only run filetypa.c on win95, and 
//      filetypw.c on WINNT, so everything matches.
//
#undef GetFileNameFromBrowse
#undef PickIconDlg


//
// shdoc401 has some wrappers that mess us up since we are
// compiled both unicode and ansi. #undef them here
//
#undef SendMessage
#ifdef UNICODE
#define SendMessage SendMessageW
#else
#define SendMessage SendMessageA
#endif


//
// HACKHACK (reinerf)
//
// we have to do this since the shell32.w95 lib file exports ShellMessageBox
// and the shell32.nt4 lib file exports ShellMessageBoxA/W and we link to the 
// win95 lib on x86 and the nt4 lib on alpha. sigh. We never-ever call ShellMessageBox with
// strings so we can get away with calling the xxxA version.
#if !defined(_X86_)
// alpha exports A/W so call A
#undef ShellMessageBox
#define ShellMessageBox ShellMessageBoxA
#endif

// HACKHACK (reinerf)
//
// we call PathProcessCommand which was a tchar export on NT4/Win95. But, since this code is
// compiled twice we can call right through to the old ordinal export. Didnt want to add this
// to dllload.c since it dosent thunk, it just calls the tchar export.
#undef PathProcessCommand
#ifdef UNICODE
#define PathProcessCommand FileTypeThunk_PathProcessCommandW
#else
#define PathProcessCommand FileTypeThunk_PathProcessCommandA
#endif

typedef LONG (__stdcall * PFNPATHPROCESSCOMMANDA)(LPCSTR lpSrc, LPSTR lpDest, int iDestMax, DWORD dwFlags);
typedef LONG (__stdcall * PFNPATHPROCESSCOMMANDW)(LPCWSTR lpSrc, LPWSTR lpDest, int iDestMax, DWORD dwFlags);
#define PFN_FIRSTTIME   ((void *)0xFFFFFFFF)

#ifdef UNICODE
LONG WINAPI FileTypeThunk_PathProcessCommandW(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags)
#else
LONG WINAPI FileTypeThunk_PathProcessCommandA(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags)
#endif
{
#ifdef UNICODE
    static PFNPATHPROCESSCOMMANDW s_pfn = PFN_FIRSTTIME;
#else
    static PFNPATHPROCESSCOMMANDA s_pfn = PFN_FIRSTTIME;
#endif

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));  // this is fast

        if (NULL == hinst)
            hinst = LoadLibrary(TEXT("SHELL32.DLL"));

        if (hinst)
        {
            // PathProcessCommand = Ordinal653
#ifdef UNICODE
            s_pfn = (PFNPATHPROCESSCOMMANDW)GetProcAddress(hinst, (LPCSTR) MAKEINTRESOURCE(653));
#else
            s_pfn = (PFNPATHPROCESSCOMMANDA)GetProcAddress(hinst, (LPCSTR) MAKEINTRESOURCE(653));
#endif
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
        return s_pfn(lpSrc, lpDest, iDestMax, dwFlags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return -1;
}

//
// HACKHACK (reinerf)
//
// since we link to the win95 shell32.lib, we need thunks for things in the shell32.nt4 that win95
// didnt have.
//
#ifdef UNICODE
#define SHGetFileInfoW FileTypeThunk_SHGetFileInfoW

typedef DWORD (__stdcall * PFNSHGETFILEINFOW)(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags);

LONG WINAPI FileTypeThunk_SHGetFileInfoW(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags)
{
    static PFNSHGETFILEINFOW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));  // this is fast

        if (NULL == hinst)
            hinst = LoadLibrary(TEXT("SHELL32.DLL"));

        if (hinst)
        {
            // PathProcessCommand = Ordinal653
            s_pfn = (PFNSHGETFILEINFOW)GetProcAddress(hinst, "SHGetFileInfoW");
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
        return s_pfn(pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return -1;
}
#endif // UNICODE, HACKHACK


#define SetDefaultDialogFont SHSetDefaultDialogFont
#define RemoveDefaultDialogFont SHRemoveDefaultDialogFont


HRESULT CreateFileTypePageW(HPROPSHEETPAGE * phpsp, LPVOID pvReserved);
HRESULT CreateFileTypePageA(HPROPSHEETPAGE * phpsp, LPVOID pvReserved);

#define ACP_US      1252

/* stuff a point value packed in an LPARAM into a POINT */

#define LPARAM_TO_POINT(lparam, pt)       ((pt).x = LOWORD(lparam), \
                                           (pt).y = HIWORD(lparam))


#define c_szShowExt             TEXT("AlwaysShowExt")
#define c_szSpaceFile           TEXT(" File")
#define c_szShell2              TEXT("shell32.dll")
#define c_szExefileOpenCommand  TEXT("\"%1\"")
#define c_szSpPercentOne        TEXT(" %1")
#define c_szDDEExec             TEXT("ddeexec")
#define c_szShellOpenCommand    TEXT("shell\\open\\command")
#define c_szShellexIconHandler  TEXT("shellex\\IconHandler")
#define c_szDDEApp              TEXT("Application")
#define c_szDDEAppNot           TEXT("ifexec")
#define c_szDDETopic            TEXT("topic")
#define c_szNew                 TEXT("New")
#define c_szFile                TEXT("file")

#define c_szTemplateSS          TEXT("%s\\%s")
#define c_szTemplateSSS         TEXT("%s\\%s\\%s")

#define c_szShellOpenCmdSubKey  TEXT("Shell\\Open\\Command")

#define c_szShell               STRREG_SHELL        // "shell"
#define c_szDefaultIcon         TEXT("DefaultIcon")
#define c_szEditFlags           TEXT("EditFlags")
#define c_szExtension           TEXT("Extension")
#define c_szContentType         TEXT("Content Type")


static HIMAGELIST  g_himlSysSmall = NULL;
static HIMAGELIST  g_himlSysLarge = NULL;


typedef struct tagFILETYPESINFO
{
    TCHAR szId[MAX_PATH];                   // file type identifier         mmmfile
    TCHAR szDesc[MAX_PATH];                 // file type description        Animation
    TCHAR szDefaultAction[MAX_PATH];                // default verb                         mmmfile\shell = (szDefaultAction=open)
    DWORD dwAttributes;                     // Optional attributes for type;        mmmfile\Attributes
    HICON hIconDoc;                         // Large icon for doc
    HICON hIconOpen;                        // Large icon for exe
    HKEY hkeyFT;                            // HKEY for this file type
    HDPA hDPAExt;                           // dynamic pointer array for list of extensions
    DWORD dwMIMEFlags;                      // flags from MIMEFLAGS
    TCHAR szOriginalMIMEType[MAX_PATH];     // original MIME type
} FILETYPESINFO, *PFILETYPESINFO;


typedef struct tagFILETYPESCOMMANDINFO
{
    TCHAR szId[MAX_PATH];                   // file type identifier mmmfile
    TCHAR szActionKey[MAX_PATH];            // action verb                  mmmfile\shell\open (szAction=open)
    TCHAR szActionValue[MAX_PATH];          // action verb                  mmmfile\shell\open=Open (szAction=open)
    TCHAR szCommand[MAX_PATH];              // app to execute               mmmfile\shell\open\command = szCommand (C:\FOO.EXE %1)
    TCHAR szDDEMsg[MAX_PATH];               // DDE message                  mmmfile\shell\open\ddeexec = szDDEMsg
    TCHAR szDDEApp[MAX_PATH];               // DDE application              mmmfile\shell\open\ddeexec\application = szDDEApp
    TCHAR szDDEAppNot[MAX_PATH];            // DDE application !run mmmfile\shell\open\ddeexec\ifexec = szDDEAppNot
    TCHAR szDDETopic[MAX_PATH];             // DDE topic                    mmmfile\shell\open\ddeexec\topic = szDDETopic
    DWORD dwVerbAttributes;                 // Optional attributes for verb;        mmmfile\shell\verb -> Attributes=dwVerbAttributes
} FILETYPESCOMMANDINFO, *PFILETYPESCOMMANDINFO;


typedef struct tagFILETYPESDIALOGINFO
{
    HWND hPropDialog;
    HWND hEditDialog;
    HWND hCmdDialog;
    HWND hwndDocIcon;
    HWND hwndOpenIcon;
    HWND hwndEditDocIcon;
    HWND hwndDocExt;
    HWND hwndContentTypeComboBox;
    HWND hwndDefExtensionComboBox;
    HWND hwndLVFT;
    HWND hwndLVFTEdit;
    HIMAGELIST himlFT;
    HIMAGELIST himlLarge;                   // System image list large icons
    HIMAGELIST himlSmall;                   // System image list small icons
    HANDLE hThread;
    DWORD dwCommand;                                // Edit or New
    DWORD dwCommandEdit;                    // Edit or New
    INT iItem;                                              // File Type ListView's item
    INT iEditItem;                                  // Edit ListView's item
    TCHAR szId[MAX_PATH];                   // file type identifier mmmfile
    TCHAR szIconPath[MAX_PATH];
    INT iIconIndex;
    HFONT hfReg;
    HFONT hfBold;
    PFILETYPESINFO pFTInfo;
    PFILETYPESCOMMANDINFO pFTCInfo;
} FILETYPESDIALOGINFO, *PFILETYPESDIALOGINFO;



BOOL    FTEdit_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, LPTSTR szActionKey, LPTSTR szActionValue, LPTSTR szId, HKEY hk);
LPCTSTR FT_GetHelpFileFromControl(HWND hwndControl);



static VOID FT_CleanupOne(PFILETYPESDIALOGINFO pFTDInfo, PFILETYPESINFO pFTInfo)
{
    TraceMsg(TF_FILETYPE, "FT_CleanupOne");

    if (pFTDInfo->pFTInfo == pFTInfo)
        pFTDInfo->pFTInfo = NULL;

    if(pFTInfo->hIconDoc)
        DestroyIcon(pFTInfo->hIconDoc);
    if(pFTInfo->hIconOpen)
        DestroyIcon(pFTInfo->hIconOpen);
    if(pFTInfo->hkeyFT)
        RegCloseKey(pFTInfo->hkeyFT);
    if(pFTInfo->hDPAExt)
    {
        int iCnt;
        int i;

        iCnt = DPA_GetPtrCount(pFTInfo->hDPAExt);
        for(i = 0; i < iCnt; i++)
            LocalFree((HANDLE)DPA_FastGetPtr(pFTInfo->hDPAExt, i));
        DPA_Destroy(pFTInfo->hDPAExt);
    }
    LocalFree((HANDLE)pFTInfo);
}

// ================================================================
// ================================================================



#define c_szMIMETypeFmt     TEXT("MIME\\Database\\Content Type\\%s")

/* MIME Types
 *************/

typedef BOOL (CALLBACK *ENUMPROC)(LPCTSTR pcsz, LPARAM lparam);

typedef struct enumdata
{
   ENUMPROC enumproc;

   LPARAM lparam;
}
ENUMDATA;
typedef ENUMDATA * PENUMDATA;
typedef const ENUMDATA CENUMDATA;
typedef const ENUMDATA * PCENUMDATA;

typedef struct mimeenumdata
{
   ENUMPROC enumproc;

   LPARAM lparam;

   LPCTSTR pcszMIMEType;
}
MIMEENUMDATA;
typedef MIMEENUMDATA * PMIMEENUMDATA;
typedef const MIMEENUMDATA CMIMEENUMDATA;
typedef const MIMEENUMDATA * PCMIMEENUMDATA;

typedef struct bufferdata
{
   LPTSTR pszBuf;

   UINT ucBufLen;
}
BUFFERDATA;
typedef BUFFERDATA * PBUFFERDATA;
typedef const BUFFERDATA CBUFFERDATA;
typedef const BUFFERDATA * PCBUFFERDATA;


/*************************** Private MIME Functions **************************/


/* Debug Validation Functions
 *****************************/


#ifdef DEBUG
#ifndef UNICODE
// This file is compiled twice (filetypa.c and filetypw.c).  But this
// code below should be instantiated only once.

BOOL IsValidPCENUMDATA(PCENUMDATA pcenumdata)
{
   /* lparam may be any value. */
   return(IS_VALID_READ_PTR(pcenumdata, CENUMDATA) &&
          IS_VALID_CODE_PTR(pcenumdata->enumproc, ENUMPROC));
}


BOOL IsValidPCMIMEENUMDATA(PCMIMEENUMDATA pcmimeenumdata)
{
   /* lparam may be any value. */
   return(IS_VALID_READ_PTR(pcmimeenumdata, CMIMEENUMDATA) &&
          IS_VALID_CODE_PTR(pcmimeenumdata->enumproc, ENUMPROC) &&
          IS_VALID_STRING_PTR(pcmimeenumdata->pcszMIMEType, -1));
}


BOOL IsValidPCBUFFERDATA(PCBUFFERDATA pcbufdata)
{
   return(IS_VALID_READ_PTR(pcbufdata, CBUFFERDATA) &&
          EVAL(pcbufdata->ucBufLen > 0) &&
          IS_VALID_WRITE_BUFFER(pcbufdata->pszBuf, TCHAR, pcbufdata->ucBufLen));
}

#else

BOOL    IsValidPCENUMDATA(PCENUMDATA pcenumdata);
BOOL    IsValidPCMIMEENUMDATA(PCMIMEENUMDATA pcmimeenumdata);
BOOL    IsValidPCBUFFERDATA(PCBUFFERDATA pcbufdata);

#endif  // UNICODE
#endif   /* DEBUG */


/*
** ClassIsSafeToOpen()
**
** Determines whether or not a file class is known safe to open.
**
*/
static BOOL ClassIsSafeToOpen(LPCTSTR pcszClass)
{
   BOOL bSafe;
   DWORD dwValueType;
   DWORD dwEditFlags;
   DWORD dwcbLen = SIZEOF(dwEditFlags);

   ASSERT(IS_VALID_STRING_PTR(pcszClass, -1));

   bSafe = (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pcszClass, c_szEditFlags,
                                   &dwValueType, &dwEditFlags, &dwcbLen) &&
            (dwValueType == REG_BINARY ||
             dwValueType == REG_DWORD) &&
            IsFlagSet(dwEditFlags, FTA_OpenIsSafe));

   return(bSafe);
}


/*
** SetClassEditFlags()
**
** Sets or clears EditFlags for a file class.
**
*/
static BOOL SetClassEditFlags(LPCTSTR pcszClass, DWORD dwFlags, BOOL bSet)
{
   BOOL bResult = FALSE;
   DWORD dwValueType;
   DWORD dwEditFlags;
   DWORD dwcbLen = SIZEOF(dwEditFlags);

   ASSERT(IS_VALID_STRING_PTR(pcszClass, -1));

   /* Get current file class flags. */

   if (NO_ERROR != SHGetValue(HKEY_CLASSES_ROOT, pcszClass, c_szEditFlags,
                              &dwValueType, &dwEditFlags, &dwcbLen) ||
       (dwValueType != REG_BINARY &&
        dwValueType != REG_DWORD))
      dwEditFlags = 0;

   /* Set or clear SafeOpen flag for file class. */

   if (bSet)
      SetFlag(dwEditFlags, dwFlags);
   else
      ClearFlag(dwEditFlags, dwFlags);

   /*
    * N.b., we must set this as REG_BINARY because the base Win95 shell32.dll
    * only accepts REG_BINARY EditFlags.
    */

   bResult = (NO_ERROR == SHSetValue(HKEY_CLASSES_ROOT, pcszClass, c_szEditFlags,
                                     REG_BINARY, &dwEditFlags, SIZEOF(dwEditFlags)));

   return(bResult);
}


/*
** GetMIMEValue()
**
** Retrieves the data for a value of a MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL GetMIMEValue(LPCTSTR pcszMIMEType, LPCTSTR pcszValue,
                              PDWORD pdwValueType, PBYTE pbyteValueBuf,
                              PDWORD pdwcbValueBufLen)
{
   BOOL bResult;
   TCHAR szMIMETypeSubKey[MAX_PATH];

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, -1));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   bResult = (GetMIMETypeSubKey(pcszMIMEType, szMIMETypeSubKey,
                                       SIZECHARS(szMIMETypeSubKey)) &&
              NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, szMIMETypeSubKey,
                                     pcszValue, pdwValueType, pbyteValueBuf,
                                     pdwcbValueBufLen));

   return(bResult);
}


/*
** GetMIMETypeStringValue()
**
** Retrieves the string for a registered MIME type's value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL GetMIMETypeStringValue(LPCTSTR pcszMIMEType, LPCTSTR pcszValue,
                                         LPTSTR pszBuf, UINT ucBufLen)
{
   BOOL bResult;
   DWORD dwValueType;
   DWORD dwcbLen = CbFromCch(ucBufLen);

   /* GetMIMEValue() will verify parameters. */

   bResult = (GetMIMEValue(pcszMIMEType, pcszValue, &dwValueType,
                           (PBYTE)pszBuf, &dwcbLen) &&
              dwValueType == REG_SZ);

   if (! bResult)
   {
      if (ucBufLen > 0)
         *pszBuf = '\0';
   }

   ASSERT(! ucBufLen ||
          IS_VALID_STRING_PTR(pszBuf, -1));

   return(bResult);
}


/*
** GetFileTypeValue()
**
** Retrieves the data for a value of the file class associated with an
** extension.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL GetFileTypeValue(LPCTSTR pcszExtension, LPCTSTR pcszSubKey,
                                  LPCTSTR pcszValue, PDWORD pdwValueType,
                                  PBYTE pbyteValueBuf, PDWORD pdwcbValueBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IsValidExtension(pcszExtension));
   ASSERT(! pcszSubKey ||
          IS_VALID_STRING_PTR(pcszSubKey, -1));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, -1));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   if (EVAL(*pcszExtension))
   {
      TCHAR szSubKey[MAX_PATH];
      DWORD dwcbLen = SIZEOF(szSubKey);
      DWORD dwType;

      /* Get extension's file type. */

      if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pcszExtension, NULL, &dwType,
                                 szSubKey, &dwcbLen) &&
          REG_SZ == dwType && *szSubKey)
      {
         /* Any sub key to append? */

         if (pcszSubKey)
         {
            /* Yes. */

            /* (+ 1) for possible key separator. */

            bResult = EVAL(lstrlen(szSubKey) + 1 + lstrlen(pcszSubKey)
                           < SIZECHARS(szSubKey));

            if (bResult)
            {
               PathAppend(szSubKey, pcszSubKey);
               ASSERT(lstrlen(szSubKey) < SIZECHARS(szSubKey));
            }
         }
         else
            /* No. */
            bResult = TRUE;

         if (bResult)
            /* Get file type's value string. */
            bResult = (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, szSubKey, pcszValue,
                                              pdwValueType, pbyteValueBuf,
                                              pdwcbValueBufLen));
      }
      else
         TraceMsg(TF_FILETYPE, "GetFileTypeValue(): No file type registered for extension %s.",
                  pcszExtension);
   }
   else
      TraceMsg(TF_WARNING, "GetFileTypeValue(): No extension given.");

   return(bResult);
}


/*
** MIME_GetExtension()
**
** Determines the file name extension to be used when writing a file of a MIME
** type to the file system.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL MIME_GetExtension(LPCTSTR pcszMIMEType, LPTSTR pszExtensionBuf,
                                   UINT ucExtensionBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));
   ASSERT(IS_VALID_WRITE_BUFFER(pszExtensionBuf, TCHAR, ucExtensionBufLen));

   if (EVAL(ucExtensionBufLen > 2))
   {
      /* Leave room for possible leading period. */

      if (GetMIMETypeStringValue(pcszMIMEType, c_szExtension,
                                 pszExtensionBuf + 1, ucExtensionBufLen - 1))
      {
         if (pszExtensionBuf[1])
         {
            /* Prepend period if necessary. */

            if (pszExtensionBuf[1] == TEXT('.'))
               /* (+ 1) for null terminator. */
               MoveMemory(pszExtensionBuf, pszExtensionBuf + 1,
                          CbFromCch(lstrlen(pszExtensionBuf + 1) + 1));
            else
               pszExtensionBuf[0] = TEXT('.');

            bResult = TRUE;
         }
      }
   }

   if (! bResult)
   {
      if (ucExtensionBufLen > 0)
         *pszExtensionBuf = '\0';
   }

   if (bResult)
      TraceMsg(TF_FILETYPE, "MIME_GetExtension(): Extension %s registered as default extension for MIME type %s.",
                 pszExtensionBuf, pcszMIMEType);

   ASSERT((bResult &&
           IsValidExtension(pszExtensionBuf)) ||
          (! bResult &&
           (! ucExtensionBufLen ||
            ! *pszExtensionBuf)));
   ASSERT(! ucExtensionBufLen ||
          (UINT)lstrlen(pszExtensionBuf) < ucExtensionBufLen);

   return(bResult);
}


/*
** GetMIMEFileTypeValue()
**
** Retrieves the data for a value of the file class associated with a MIME
** type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL GetMIMEFileTypeValue(LPCTSTR pcszMIMEType, LPCTSTR pcszSubKey,
                                      LPCTSTR pcszValue, PDWORD pdwValueType,
                                      PBYTE pbyteValueBuf,
                                      PDWORD pdwcbValueBufLen)
{
   BOOL bResult;
   TCHAR szExtension[MAX_PATH];

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));
   ASSERT(! pcszSubKey ||
          IS_VALID_STRING_PTR(pcszSubKey, -1));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTR(pcszValue, -1));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   /* Get file name extension associated with MIME type. */

   if (MIME_GetExtension(pcszMIMEType, szExtension, SIZECHARS(szExtension)))
      bResult = GetFileTypeValue(szExtension, pcszSubKey, pcszValue, pdwValueType,
                                 pbyteValueBuf, pdwcbValueBufLen);
   else
      bResult = FALSE;

   return(bResult);
}


/*
** MIME_IsExternalHandlerRegistered()
**
** Determines whether or not an external handler is registered for a MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL MIME_IsExternalHandlerRegistered(LPCTSTR pcszMIMEType)
{
   BOOL bResult;
   DWORD dwValueType;
   TCHAR szOpenCmd[MAX_PATH];
   DWORD dwcbOpenCmdLen = SIZEOF(szOpenCmd);

   /* GetMIMEFileTypeValue() will verify parameters. */

   /* Look up the open command of the MIME type's associated file type. */

   bResult = (GetMIMEFileTypeValue(pcszMIMEType, c_szShellOpenCmdSubKey,
                                   NULL, &dwValueType, (PBYTE)szOpenCmd,
                                   &dwcbOpenCmdLen) &&
              dwValueType == REG_SZ);

   TraceMsg(TF_FILETYPE, "MIME_IsExternalHandlerRegistered(): %s external handler is registered for MIME type %s.",
              bResult ? TEXT("An") : TEXT("No"), pcszMIMEType);

   return(bResult);
}


/*
** MIME_GetMIMETypeFromExtension()
**
** Determines the MIME type associated with a file extension.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL MIME_GetMIMETypeFromExtension(LPCTSTR pcszPath,
                                                      LPTSTR pszMIMETypeBuf,
                                                      UINT ucMIMETypeBufLen)
{
   BOOL bResult = FALSE;
   LPCTSTR pcszExtension;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, -1));
   ASSERT(IS_VALID_WRITE_BUFFER(pszMIMETypeBuf, TCHAR, ucMIMETypeBufLen));

   pcszExtension = PathFindExtension(pcszPath);

   if (*pcszExtension)
   {
      DWORD dwcLen = CbFromCch(ucMIMETypeBufLen);
      DWORD dwType;

      bResult = (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pcszExtension,
                                        c_szContentType, &dwType, pszMIMETypeBuf,
                                        &dwcLen) &&
                 REG_SZ == dwType);
   }

   if (! bResult)
   {
      if (ucMIMETypeBufLen > 0)
         *pszMIMETypeBuf = '\0';
   }

   ASSERT(! ucMIMETypeBufLen ||
          (IS_VALID_STRING_PTR(pszMIMETypeBuf, -1) &&
           (UINT)lstrlen(pszMIMETypeBuf) < ucMIMETypeBufLen));
   ASSERT(bResult ||
          ! ucMIMETypeBufLen ||
          ! *pszMIMETypeBuf);

   return(bResult);
}


/* Windows Control Functions
 ****************************/


/*
** AddStringToComboBox()
**
** Adds a string to a combo box.  Does not check to see if the string has
** already been added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL AddStringToComboBox(HWND hwndComboBox, LPCTSTR pcsz)
{
   BOOL bResult;
   LONG lAddStringResult;

   ASSERT(IS_VALID_HANDLE(hwndComboBox, WND));
   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

   lAddStringResult = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)pcsz);

   bResult = (lAddStringResult != CB_ERR &&
              lAddStringResult != CB_ERRSPACE);

   if (bResult)
      TraceMsg(TF_FILETYPE, "AddStringToComboBox(): Added string %s to combo box.",
               pcsz);
   else
      TraceMsg(TF_WARNING, "AddStringToComboBox(): Failed to add string %s to combo box.",
                   pcsz);

   return(bResult);
}


/*
** SafeAddStringToComboBox()
**
** Adds a string to a combo box.  Checks to see if the string has already been
** added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL SafeAddStringToComboBox(HWND hwndComboBox, LPCTSTR pcsz)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hwndComboBox, WND));
   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

   if (SendMessage(hwndComboBox, CB_FINDSTRINGEXACT, 0, (LPARAM)pcsz) == CB_ERR)
      bResult = AddStringToComboBox(hwndComboBox, pcsz);
   else
   {
      bResult = TRUE;

      TraceMsg(TF_FILETYPE, "SafeAddStringToComboBox(): String %s already added to combo box.",
               pcsz);
   }

   return(bResult);
}


/*
** SafeAddStringsToComboBox()
**
** Adds a list of strings to a combo box.  Does not check to see if the strings
** have already been added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL SafeAddStringsToComboBox(HDPA hdpa, HWND hwndComboBox)
{
   BOOL bResult;
   int ncStrings;
   int i;

   ASSERT(IS_VALID_HANDLE(hwndComboBox, WND));

   ncStrings = DPA_GetPtrCount(hdpa);

   for (i = 0; i < ncStrings; i++)
   {
      LPCTSTR pcsz;

      pcsz = DPA_FastGetPtr(hdpa, i);
      ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

      bResult = SafeAddStringToComboBox(hwndComboBox, pcsz);

      if (! bResult)
         break;
   }

   return(bResult);
}


/*
** AddAndSetComboBoxCurrentSelection()
**
** Adds a string to a combo box, and sets it as the current selection.  Does
** not check to see if the string has already been added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL AddAndSetComboBoxCurrentSelection(HWND hwndComboBox,
                                                    LPCTSTR pcszText)
{
   BOOL bResult;
   LONG liSel;

   liSel = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)pcszText);

   bResult = (liSel != CB_ERR &&
              liSel != CB_ERRSPACE &&
              SendMessage(hwndComboBox, CB_SETCURSEL, liSel, 0) != CB_ERR);

   if (bResult)
      TraceMsg(TF_FILETYPE, "AddAndSetComboBoxCurrentSelection(): Current combo box selection set to %s.",
               pcszText);

   return(bResult);
}


/* Enumeration Callback Functions
 *********************************/


/*
** ExtensionEnumerator()
**
** Enumeration callback function to enumerate extensions.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL CALLBACK ExtensionEnumerator(LPCTSTR pcsz, LPARAM lparam)
{
   BOOL bContinue;

   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));
   ASSERT(IS_VALID_STRUCT_PTR((PCENUMDATA)lparam, CENUMDATA));

   if (*pcsz == TEXT('.'))
   {
      PCENUMDATA pcenumdata = (PCENUMDATA)lparam;

      bContinue = (*(pcenumdata->enumproc))(pcsz, pcenumdata->lparam);
   }
   else
      bContinue = TRUE;

   return(bContinue);
}


/*
** MIMETypeEnumerator()
**
** Enumeration callback function to enumerate MIME types.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL CALLBACK MIMETypeEnumerator(LPCTSTR pcsz, LPARAM lparam)
{
   BOOL bContinue;
   TCHAR szMIMEType[MAX_PATH];
   DWORD dwcbContentTypeLen = SIZEOF(szMIMEType);
   DWORD dwType;

   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));
   ASSERT(IS_VALID_STRUCT_PTR((PCENUMDATA)lparam, CENUMDATA));

   if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pcsz, c_szContentType, &dwType,
                              szMIMEType, &dwcbContentTypeLen) &&
       REG_SZ == dwType)
   {
      PCENUMDATA pcenumdata = (PCENUMDATA)lparam;

      TraceMsg(TF_FILETYPE, "MIMETypeEnumerator(): MIME type %s registered for extension %s.",
               szMIMEType, pcsz);

      bContinue = (*(pcenumdata->enumproc))(szMIMEType, pcenumdata->lparam);
   }
   else
      bContinue = TRUE;

   return(bContinue);
}


/*
** MIMETypeExtensionEnumerator()
**
** Enumeration callback function to enumerate extensions registered as a given
** MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL CALLBACK MIMETypeExtensionEnumerator(LPCTSTR pcsz,
                                                       LPARAM lparam)
{
   BOOL bContinue = TRUE;
   TCHAR szMIMEType[MAX_PATH];
   DWORD dwcbContentTypeLen = SIZEOF(szMIMEType);
   DWORD dwType;

   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));
   ASSERT(IS_VALID_STRUCT_PTR((PCMIMEENUMDATA)lparam, CMIMEENUMDATA));

   if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pcsz, c_szContentType, &dwType,
                              szMIMEType, &dwcbContentTypeLen) &&
       REG_SZ == dwType)
   {
      PCMIMEENUMDATA pcmimeenumdata = (PCMIMEENUMDATA)lparam;

      if (! lstrcmpi(szMIMEType, pcmimeenumdata->pcszMIMEType))
      {
         TraceMsg(TF_FILETYPE, "MIMETypeEnumerator(): Found extension %s registered for MIME type %s.",
                  pcsz, szMIMEType);

         bContinue = (*(pcmimeenumdata->enumproc))(pcsz, pcmimeenumdata->lparam);
      }
   }

   return(bContinue);
}


/*
** AddStringToComboBoxEnumerator()
**
** Enumeration callback function to add strings to a combo box.  Does not check
** to see if the strings have already been added.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL CALLBACK AddStringToComboBoxEnumerator(LPCTSTR pcsz,
                                                         LPARAM lparam)
{
   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));
   ASSERT(IS_VALID_HANDLE((HWND)lparam, WND));

   return(AddStringToComboBox((HWND)lparam, pcsz));
}


/*
** AddHandledMIMETypeEnumerator()
**
** Enumeration callback function to enumerate MIME types with registered open
** verb application handlers.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL CALLBACK AddHandledMIMETypeEnumerator(LPCTSTR pcsz,
                                                        LPARAM lparam)
{
   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));
   ASSERT(IS_VALID_HANDLE((HWND)lparam, WND));

   /* Only add MIME types with registered applications. */

   return(MIME_IsExternalHandlerRegistered(pcsz)
          ? SafeAddStringToComboBox((HWND)lparam, pcsz)
          : TRUE);
}


/*
** ReplacementDefExtensionEnumerator()
**
** Enumeration callback function to find a default extension for a MIME type.
** Returns the first extension in alphabetical order.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL CALLBACK ReplacementDefExtensionEnumerator(LPCTSTR pcsz,
                                                             LPARAM lparam)
{
   PBUFFERDATA pbufdata = (PBUFFERDATA)lparam;

   ASSERT(IS_VALID_STRING_PTR(pcsz, -1));
   ASSERT(IS_VALID_STRUCT_PTR((PCBUFFERDATA)lparam, CBUFFERDATA));

   if (! *(pbufdata->pszBuf) ||
       lstrcmpi(pcsz, pbufdata->pszBuf) < 0)
   {
      if ((UINT)lstrlen(pcsz) < pbufdata->ucBufLen)
         lstrcpy(pbufdata->pszBuf, pcsz);
      else
         TraceMsg(TF_WARNING, "ReplacementDefExtensionEnumerator(): %u byte buffer too small to hold %d byte extension %s.",
                  pcsz, lstrlen(pcsz), pbufdata->ucBufLen);
   }

   return(TRUE);
}


/* Registry Enumeration Functions
 *********************************/


/*
** EnumSubKeys()
**
** Enumerates direct child sub keys of a given registry key.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static LRESULT EnumSubKeys(HKEY hkeyParent, ENUMPROC enumproc,
                                 LPARAM lparam)
{
   LONG lResult;
   DWORD dwi;
   TCHAR szSubKey[MAX_PATH];
   DWORD cchSubkey = SIZECHARS(szSubKey);

   /* lparam may be any value. */
   ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));

   TraceMsg(TF_FILETYPE, "EnumSubKeys(): Enumerating sub keys.");

   for (dwi = 0;
        (lResult = RegEnumKeyEx(hkeyParent, dwi, szSubKey, &cchSubkey,
                                NULL, NULL, NULL, NULL)) == ERROR_SUCCESS;
        dwi++)
   {
      if (! (*enumproc)(szSubKey, lparam))
      {
         TraceMsg(TF_FILETYPE, "EnumSubKeys(): Callback aborted at sub key %s, index %lu.",
                  szSubKey, dwi);
         lResult = ERROR_CANCELLED;
         break;
      }

      cchSubkey = SIZECHARS(szSubKey);
   }

   if (lResult == ERROR_NO_MORE_ITEMS)
   {
      TraceMsg(TF_FILETYPE, "EnumSubKeys(): Enumerated %lu sub keys.", dwi);

      lResult = ERROR_SUCCESS;
   }
   else
      TraceMsg(TF_FILETYPE, "EnumSubKeys(): Stopped after enumerating %lu sub keys.",
               dwi);

   return(lResult);
}


/*
** EnumExtensions()
**
** Enumerates extensions listed in the registry under HKEY_CLASSES_ROOT.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static LRESULT EnumExtensions(ENUMPROC enumproc, LPARAM lparam)
{
   ENUMDATA enumdata;

   /* lparam may be any value. */
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));

   TraceMsg(TF_FILETYPE, "EnumExtensions(): Enumerating extensions.");

   enumdata.enumproc = enumproc;
   enumdata.lparam = lparam;

   return(EnumSubKeys(HKEY_CLASSES_ROOT, &ExtensionEnumerator, (LPARAM)&enumdata));
}


/*
** EnumMIMETypes()
**
** Enumerates MIME types registered to extensions under HKEY_CLASSES_ROOT.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static LRESULT EnumMIMETypes(ENUMPROC enumproc, LPARAM lparam)
{
   ENUMDATA enumdata;

   /* lparam may be any value. */
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));

   TraceMsg(TF_FILETYPE, "EnumMIMETypes(): Enumerating MIME types.");

   enumdata.enumproc = enumproc;
   enumdata.lparam = lparam;

   return(EnumExtensions(&MIMETypeEnumerator, (LPARAM)&enumdata));
}


/*
** EnumExtensionsOfMIMEType()
**
** Enumerates extensions registered as a given MIME type under
** HKEY_CLASSES_ROOT.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static LRESULT EnumExtensionsOfMIMEType(ENUMPROC enumproc, LPARAM lparam,
                                              LPCTSTR pcszMIMEType)
{
   MIMEENUMDATA mimeenumdata;

   /* lparam may be any value. */
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));

   ASSERT(*pcszMIMEType);

   TraceMsg(TF_FILETYPE, "EnumExtensionsOfMIMEType(): Enumerating extensions registered as MIME type %s.",
            pcszMIMEType);

   mimeenumdata.enumproc = enumproc;
   mimeenumdata.lparam = lparam;
   mimeenumdata.pcszMIMEType = pcszMIMEType;

   return(EnumExtensions(&MIMETypeExtensionEnumerator, (LPARAM)&mimeenumdata));
}


/* MIME Registry Functions
 **************************/


/*
** FindMIMETypeOfExtensionList()
**
** Finds the MIME type of the first extension in a list of extensions that has
** a registered MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL FindMIMETypeOfExtensionList(HDPA hdpaExtensions,
                                              LPTSTR pszMIMETypeBuf,
                                              UINT ucMIMETypeBufLen)
{
   BOOL bFound = FALSE;
   int ncExtensions;
   int i;

   ASSERT(IS_VALID_WRITE_BUFFER(pszMIMETypeBuf, TCHAR, ucMIMETypeBufLen));

   ncExtensions = DPA_GetPtrCount(hdpaExtensions);

   for (i = 0; i < ncExtensions; i++)
   {
      LPCTSTR pcszExtension;

      pcszExtension = DPA_FastGetPtr(hdpaExtensions, i);
      ASSERT(IsValidExtension(pcszExtension));

      bFound = MIME_GetMIMETypeFromExtension(pcszExtension, pszMIMETypeBuf,
                                             ucMIMETypeBufLen);

      if (bFound)
         break;
   }

   if (! bFound)
   {
      if (ucMIMETypeBufLen > 0)
         *pszMIMETypeBuf = '\0';
   }

   ASSERT(! ucMIMETypeBufLen ||
          (IS_VALID_STRING_PTR(pszMIMETypeBuf, -1) &&
           (UINT)lstrlen(pszMIMETypeBuf) < ucMIMETypeBufLen));
   ASSERT(bFound ||
          ! ucMIMETypeBufLen ||
          ! *pszMIMETypeBuf);

   return(bFound);
}


/*
** RegisterContentTypeForArrayOfExtensions()
**
** Registers the given MIME type for each extension in a list of extensions.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL RegisterContentTypeForArrayOfExtensions(LPCTSTR pcszMIMEType,
                                                          HDPA hdpaExtensions)
{
   BOOL bResult = TRUE;
   int ncExtensions;
   int i;

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));

   ncExtensions = DPA_GetPtrCount(hdpaExtensions);

   for (i = 0; i < ncExtensions; i++)
   {
      LPCTSTR pcszExtension;

      pcszExtension = DPA_FastGetPtr(hdpaExtensions, i);
      ASSERT(IsValidExtension(pcszExtension));

      bResult = RegisterMIMETypeForExtension(pcszExtension, pcszMIMEType);

      if (bResult)
         TraceMsg(TF_FILETYPE, "RegisterContentTypeForArrayOfExtensions(): Registered MIME type %s for extension %s.",
                  pcszMIMEType, pcszExtension);
      else
      {
         TraceMsg(TF_WARNING, "RegisterContentTypeForArrayOfExtensions(): Failed to register MIME type %s for extension %s.",
                  pcszMIMEType, pcszExtension);
         break;
      }
   }

   return(bResult);
}


/*
** UnregisterContentTypeForArrayOfExtensions()
**
** Unregisters the MIME type association of each extension in a list of
** extensions.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL UnregisterContentTypeForArrayOfExtensions(HDPA hdpaExtensions)
{
   BOOL bResult = TRUE;
   int ncExtensions;
   int i;

   ncExtensions = DPA_GetPtrCount(hdpaExtensions);

   for (i = 0; i < ncExtensions; i++)
   {
      LPCTSTR pcszExtension;

      pcszExtension = DPA_FastGetPtr(hdpaExtensions, i);
      ASSERT(IsValidExtension(pcszExtension));

      if (! UnregisterMIMETypeForExtension(pcszExtension))
         bResult = FALSE;

      if (bResult)
         TraceMsg(TF_FILETYPE, "UnregisterContentTypeForArrayOfExtensions(): Unregistered MIME type for extension %s.",
                  pcszExtension);
      else
         TraceMsg(TF_WARNING, "UnregisterContentTypeForArrayOfExtensions(): Failed to unregister MIME type for extension %s.",
                  pcszExtension);
   }

   return(bResult);
}


/*
** ExtensionSearchCmp()
**
** Callback function to perform case-insensitive string comparison.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static int CALLBACK ExtensionSearchCmp(PVOID pvFirst, PVOID pvSecond,
                                             LPARAM lparam)
{
   ASSERT(IS_VALID_STRING_PTR(pvFirst, -1));
   ASSERT(IS_VALID_STRING_PTR(pvSecond, -1));
   ASSERT(! lparam);

   return(lstrcmpi(pvFirst, pvSecond));
}


/*
** NeedReplacementDefExtension()
**
** Determines whether or not the default extension of the given MIME type is in
** the given list of extensions.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL NeedReplacementDefExtension(HDPA hdpaExtension,
                                              LPCTSTR pcszOriginalMIMEType)
{
   BOOL bNeedReplacementDefExtension;
   TCHAR szDefExtension[MAX_PATH];

   ASSERT(IS_VALID_STRING_PTR(pcszOriginalMIMEType, -1));

   ASSERT(*pcszOriginalMIMEType);

   /* Is there a default extension specified for the MIME type? */

   if (MIME_GetExtension(pcszOriginalMIMEType, szDefExtension, SIZECHARS(szDefExtension)))
   {
      /* Yes.  Is that default extension in the list of extensions? */

      bNeedReplacementDefExtension = (DPA_Search(hdpaExtension, szDefExtension,
                                                 0, &ExtensionSearchCmp, 0, 0)
                                      != -1);

      if (bNeedReplacementDefExtension)
         TraceMsg(TF_FILETYPE, "NeedReplacementDefExtension(): Previous default extension %s for MIME type %s removed.",
                  szDefExtension, pcszOriginalMIMEType);
      else
         TraceMsg(TF_FILETYPE, "NeedReplacementDefExtension(): Previous default extension %s for MIME type %s remains registered.",
                  szDefExtension, pcszOriginalMIMEType);
   }
   else
   {
      /* No.  Choose a new default extension. */

      bNeedReplacementDefExtension = TRUE;

      TraceMsg(TF_WARNING, "NeedReplacementDefExtension(): No default extension registered for MIME type %s.  Choosing default extension.",
               pcszOriginalMIMEType);
   }

   return(bNeedReplacementDefExtension);
}


/*
** FindReplacementDefExtension()
**
** Finds a suitable default extension for the given MIME type.  Selects the
** first extension in alphabetical order that is registered as the given MIME
** type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL FindReplacementDefExtension(HDPA hdpaExtension,
                                        LPCTSTR pcszOriginalMIMEType,
                                        LPTSTR pszDefExtensionBuf, UINT ucDefExtensionBufLen)
{
   BOOL bFound;
   BUFFERDATA bufdata;

   ASSERT(IS_VALID_STRING_PTR(pcszOriginalMIMEType, -1));
   ASSERT(IS_VALID_WRITE_BUFFER(pszDefExtensionBuf, TCHAR, ucDefExtensionBufLen));

   ASSERT(*pcszOriginalMIMEType);

   if (EVAL(*pcszOriginalMIMEType) &&
       EVAL(ucDefExtensionBufLen > 1))
   {
      /* Any extensions still registered as the given MIME type? */

      *pszDefExtensionBuf = '\0';

      bufdata.pszBuf = pszDefExtensionBuf;
      bufdata.ucBufLen = ucDefExtensionBufLen;

      EVAL(EnumExtensionsOfMIMEType(&ReplacementDefExtensionEnumerator,
                                    (LPARAM)&bufdata, pcszOriginalMIMEType)
           == ERROR_SUCCESS);

      bFound = (*pszDefExtensionBuf != '\0');
   }
   else
      bFound = FALSE;

   if (bFound)
      TraceMsg(TF_FILETYPE, "FindReplacementDefExtension(): Extension %s remains registered as MIME type %s.",
               pszDefExtensionBuf, pcszOriginalMIMEType);
   else
      TraceMsg(TF_FILETYPE, "FindReplacementDefExtension(): No extensions remain registered as MIME type %s.",
               pcszOriginalMIMEType);

   ASSERT(! bFound ||
          (IsValidExtension(pszDefExtensionBuf) &&
           (UINT)lstrlen(pszDefExtensionBuf) < ucDefExtensionBufLen));
   ASSERT(bFound ||
          ! ucDefExtensionBufLen ||
          ! *pszDefExtensionBuf);

   return(bFound);
}


/*
** RegisterNewDefExtension()
**
** Registers a new default extension for the given MIME type, assuming that the
** MIME types associated with the given list of extensions has changed.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL RegisterNewDefExtension(HDPA hdpaExtension,
                                              LPCTSTR pcszOriginalMIMEType)
{
   BOOL bResult = TRUE;

   ASSERT(IS_VALID_STRING_PTR(pcszOriginalMIMEType, -1));

   /* Was there originally a MIME type specified? */

   if (*pcszOriginalMIMEType)
   {
      /*
       * Yes.  Was the previous default extension for the MIME type just
       * removed?
       */

      if (NeedReplacementDefExtension(hdpaExtension, pcszOriginalMIMEType))
      {
         TCHAR szDefExtension[MAX_PATH];

         /*
          * Yes.  Are there any remaining extensions registered as the MIME
          * type?
          */

         if (FindReplacementDefExtension(hdpaExtension, pcszOriginalMIMEType,
                                         szDefExtension,
                                         SIZECHARS(szDefExtension)))
            /*
             * Yes.  Set one of them as the new default extension for the MIME
             * type.
             */
            bResult = RegisterExtensionForMIMEType(szDefExtension,
                                                   pcszOriginalMIMEType);
         else
            /* No.  Remove the MIME type. */
            bResult = UnregisterExtensionForMIMEType(pcszOriginalMIMEType);
      }
   }
   else
      /* No.  Nothing to clean up. */
      TraceMsg(TF_FILETYPE, "RegisterNewDefExtension(): No original MIME type, no new default extension");

   return(bResult);
}


/*
** AddMIMETypeInfo()
**
** Registers the given MIME type for the given list of extensions.  Registers
** the given default extension for the given MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL AddMIMETypeInfo(PFILETYPESDIALOGINFO pFTDInfo,
                                  LPCTSTR pcszOldMIMEType, LPCTSTR pcszNewMIMEType,
                                  LPCTSTR pcszDefExtension)
{
   BOOL bMIMEResult;
   BOOL bOldDefExtensionResult;
   BOOL bNewDefExtensionResult;

   ASSERT(IS_VALID_STRING_PTR(pcszOldMIMEType, -1));
   ASSERT(IS_VALID_STRING_PTR(pcszNewMIMEType, -1));
   ASSERT(IS_VALID_STRING_PTR(pcszDefExtension, -1));

   /* Register MIME type for extensions. */

   bMIMEResult = RegisterContentTypeForArrayOfExtensions(pcszNewMIMEType,
                                                         pFTDInfo->pFTInfo->hDPAExt);

   /* Pick new default extension for old MIME type. */

   if (*pcszOldMIMEType &&
       lstrcmpi(pcszOldMIMEType, pcszNewMIMEType) != 0)
      bOldDefExtensionResult = RegisterNewDefExtension(pFTDInfo->pFTInfo->hDPAExt,
                                                       pcszOldMIMEType);
   else
      bOldDefExtensionResult = TRUE;

   /* Register default extension for MIME type. */

   bNewDefExtensionResult = RegisterExtensionForMIMEType(pcszDefExtension,
                                                         pcszNewMIMEType);

   return(bMIMEResult &&
          bOldDefExtensionResult &&
          bNewDefExtensionResult);
}


/*
** RemoveMIMETypeInfo()
**
** Removes the MIME type association for the given list of extensions.
** Registers a new default extension for the given MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL RemoveMIMETypeInfo(PFILETYPESDIALOGINFO pFTDInfo,
                                     LPCTSTR pcszOldMIMEType)
{
   BOOL bMIMEResult;
   BOOL bDefExtensionResult;

   ASSERT(IS_VALID_STRING_PTR(pcszOldMIMEType, -1));

   /* Unregister MIME type for extensions. */

   bMIMEResult = UnregisterContentTypeForArrayOfExtensions(pFTDInfo->pFTInfo->hDPAExt);

   /* Pick new default extension for old MIME type. */

   bDefExtensionResult = RegisterNewDefExtension(pFTDInfo->pFTInfo->hDPAExt,
                                                 pcszOldMIMEType);

   return(bMIMEResult &&
          bDefExtensionResult);
}


/* Utility Functions
 ********************/


/*
** GetFirstString()
**
** Finds the first string in alphabetical order in the given list of strings.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL GetFirstString(HDPA hdpa, LPTSTR pszBuf, UINT ucBufLen)
{
   BOOL bFound = FALSE;
   int ncStrings;
   int iFirst = 0;
   int i;

   ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, ucBufLen));

   ncStrings = DPA_GetPtrCount(hdpa);

   for (i = 0; i < ncStrings; i++)
   {
      LPCTSTR pcsz;

      pcsz = DPA_FastGetPtr(hdpa, i);
      ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

      if (EVAL((UINT)lstrlen(pcsz) < ucBufLen) &&
          (! bFound ||
           lstrcmpi(pcsz, DPA_FastGetPtr(hdpa, iFirst)) < 0))
      {
         iFirst = i;
         bFound = TRUE;
      }
   }

   if (bFound)
      lstrcpy(pszBuf, DPA_FastGetPtr(hdpa, iFirst));
   else
   {
      if (ucBufLen > 0)
         *pszBuf = '\0';
   }

   ASSERT(! ucBufLen ||
          IS_VALID_STRING_PTR(pszBuf, -1));
   ASSERT(bFound ||
          ! ucBufLen ||
          ! *pszBuf);

   return(bFound);
}


/*
** IsListOfExtensions()
**
** Determines whether or not a list of strings contains any strings that are
** not extensions.  Returns FALSE for empty list.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL IsListOfExtensions(HDPA hdpa)
{
   BOOL bNoRogues = TRUE;
   BOOL bAnyExtensions = FALSE;
   int ncStrings;
   int i;

   ncStrings = DPA_GetPtrCount(hdpa);

   for (i = 0; i < ncStrings; i++)
   {
      LPCTSTR pcsz;

      pcsz = DPA_FastGetPtr(hdpa, i);
      ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

      if (*pcsz == TEXT('.'))
      {
         bAnyExtensions = TRUE;

         TraceMsg(TF_FILETYPE, "IsListOfExtensions(): Found extension %s.",
                  pcsz);
      }
      else
      {
         bNoRogues = FALSE;

         TraceMsg(TF_FILETYPE, "IsListOfExtensions(): Found non-extension %s.",
                  pcsz);
      }
   }

   TraceMsg(TF_FILETYPE, "IsListOfExtensions(): This %s a list of extensions.",
            (bAnyExtensions && bNoRogues) ? TEXT("is") : TEXT("is not"));

   return(bAnyExtensions && bNoRogues);
}


/* New/Edit Dialog MIME Control Functions
 *****************************************/


/*
** InitContentTypeEditControl()
**
** Fills the edit control of the Content Type combo box with the MIME Type of
** the list of extensions.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL InitContentTypeEditControl(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;
   TCHAR szMIMEType[MAX_PATH];

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   EVAL(SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_RESETCONTENT, 0, 0));

   if (FindMIMETypeOfExtensionList(pFTDInfo->pFTInfo->hDPAExt, szMIMEType,
                                   SIZECHARS(szMIMEType)) &&
       *szMIMEType)
      bResult = AddAndSetComboBoxCurrentSelection(pFTDInfo->hwndContentTypeComboBox,
                                                  szMIMEType);
   else
      bResult = TRUE;

   return(bResult);
}


/*
** FillContentTypeListBox()
**
** Fills the list box of the Content Type combo box with MIME types with
** registered handlers.  Only fills list box once.  Does not reset content of
** Content Type combo box before filling.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL FillContentTypeListBox(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   if (IsFlagClear(pFTDInfo->pFTInfo->dwMIMEFlags, MIME_FL_CONTENT_TYPES_ADDED))
   {
      SetFlag(pFTDInfo->pFTInfo->dwMIMEFlags, MIME_FL_CONTENT_TYPES_ADDED);

      bResult = (EnumMIMETypes(&AddHandledMIMETypeEnumerator,
                               (LPARAM)(pFTDInfo->hwndContentTypeComboBox))
                 == ERROR_SUCCESS);
   }
   else
   {
      TraceMsg(TF_FILETYPE, "FillContentTypeListBox(): Content Type combo box already filled.");

      bResult = TRUE;
   }

   return(bResult);
}


/*
** GetAssociatedExtension()
**
** Returns the contents of the Associated Extension edit control as a valid
** extension.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL GetAssociatedExtension(HWND hdlg, LPTSTR pszAssocExtensionBuf,
                                         UINT ucAssocExtensionBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_WRITE_BUFFER(pszAssocExtensionBuf, TCHAR, ucAssocExtensionBufLen));

   if (EVAL(ucAssocExtensionBufLen > 2))
   {
      PFILETYPESDIALOGINFO pFTDInfo;

      pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

      /* Leave room for possible leading period. */

      GetWindowText(pFTDInfo->hwndDocExt, pszAssocExtensionBuf + 1,
                    ucAssocExtensionBufLen - 1);

      if (pszAssocExtensionBuf[1])
      {
         /* Prepend period if necessary. */

         if (pszAssocExtensionBuf[1] == TEXT('.'))
            /* (+ 1) for null terminator. */
            MoveMemory(pszAssocExtensionBuf, pszAssocExtensionBuf + 1,
                       CbFromCch(lstrlen(pszAssocExtensionBuf + 1) + 1));
         else
            pszAssocExtensionBuf[0] = TEXT('.');

         bResult = TRUE;
      }
   }

   if (! bResult)
   {
      if (ucAssocExtensionBufLen > 0)
         *pszAssocExtensionBuf = '\0';
   }

   if (bResult)
      TraceMsg(TF_FILETYPE, "GetAssociatedExtension(): Associated Extension is %s.",
               pszAssocExtensionBuf);
   else
      TraceMsg(TF_FILETYPE, "GetAssociatedExtension(): No Associated Extension.");

   ASSERT(! bResult ||
          (IsValidExtension(pszAssocExtensionBuf) &&
           (UINT)lstrlen(pszAssocExtensionBuf) < ucAssocExtensionBufLen));
   ASSERT(bResult ||
          ! ucAssocExtensionBufLen ||
          ! *pszAssocExtensionBuf);

   return(bResult);
}


/*
** FillDefExtensionListBox()
**
** Fills the list box of the Default Extension combo box with extensions
** registered as the current MIME type.  Also adds either the Associated
** Extension extension (New File Type dialog), or the list of extensions being
** edited (Edit File Type dialog).  Fills list box every time.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL FillDefExtensionListBox(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;
   TCHAR szMIMEType[MAX_PATH];
   LONG liSel;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   EVAL(SendMessage(pFTDInfo->hwndDefExtensionComboBox, CB_RESETCONTENT, 0, 0));

   GetWindowText(pFTDInfo->hwndContentTypeComboBox, szMIMEType,
                 SIZECHARS(szMIMEType));

   if (*szMIMEType)
   {
      TCHAR szDefExtension[MAX_PATH];

      /* Add extensions registered as given MIME type, if any. */

      bResult = (EnumExtensionsOfMIMEType(&AddStringToComboBoxEnumerator,
                                          (LPARAM)(pFTDInfo->hwndDefExtensionComboBox),
                                          szMIMEType)
                 == ERROR_SUCCESS);

      /*
       * Add the extension from the Associated Extension edit control in the
       * New File Type Dialog, or the list of extensions in the Edit File Type
       * dialog.
       */

   	if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
         /* New File Type dialog. */
         bResult = (GetAssociatedExtension(hdlg, szDefExtension,
                                           SIZECHARS(szDefExtension)) &&
                    SafeAddStringToComboBox(pFTDInfo->hwndDefExtensionComboBox,
                                            szDefExtension) &&
                    bResult);
      else
         /* Edit File Type dialog. */
         bResult = (SafeAddStringsToComboBox(pFTDInfo->pFTInfo->hDPAExt,
                                             pFTDInfo->hwndDefExtensionComboBox) &&
                    bResult);

      /*
       * Set default extension as registered default extension, or first
       * extension in list.
       */

      if (*szMIMEType &&
          MIME_GetExtension(szMIMEType, szDefExtension, SIZECHARS(szDefExtension)))
      {
         liSel = SendMessage(pFTDInfo->hwndDefExtensionComboBox,
                             CB_FINDSTRINGEXACT, 0, (LPARAM)szDefExtension);

         if (liSel == CB_ERR)
            liSel = 0;
      }
      else
         liSel = 0;

      /* There may be no entries in the combo box here. */

      SendMessage(pFTDInfo->hwndDefExtensionComboBox, CB_SETCURSEL, liSel, 0);
   }

   return(bResult);
}


/*
** SetDefExtensionComboBoxState()
**
** Enables or disables the Default Extension text and edit control based upon:
**    1) FTA_NoEditMIME setting
**    2) contents of Content Type combo box edit control
**    3) contents of Associated Extension edit control (New File Type dialog
**       only)
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static void SetDefExtensionComboBoxState(HWND hdlg, LPCTSTR pcszMIMEType)
{
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   if (IsFlagClear(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
   {
      TCHAR szAssocExtension[MAX_PATH];
      BOOL bEnable;

      bEnable = (*pcszMIMEType != '\0');

   	if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
         /* New File Type dialog. */
         bEnable = (bEnable &&
                    GetAssociatedExtension(hdlg, szAssocExtension,
                                           SIZECHARS(szAssocExtension)));

      EnableWindow(pFTDInfo->hwndDefExtensionComboBox, bEnable);

      TraceMsg(TF_FILETYPE, "EnableDefExtensionComboBox(): Default extension combo box %s.",
               bEnable ? TEXT("enabled") : TEXT("disabled"));
   }
}


/*
** SetDefExtension()
**
** Fills the read-only edit control of the Default Extension combo box with the
** default extension of the given MIME type.  If there is no default extension
** for the given MIME type, falls back to:
**    - the contents of the Associated Extension edit control in the New File
**      Type dialog
**    - the first extension in alphabetical order in the list of extensions
**      being edited in the Edit File Type dialog
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL SetDefExtension(HWND hdlg, LPCTSTR pcszMIMEType)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, -1));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   EVAL(SendMessage(pFTDInfo->hwndDefExtensionComboBox, CB_RESETCONTENT, 0, 0));

   /* Any MIME type? */

   if (*pcszMIMEType)
   {
      TCHAR szDefExtension[MAX_PATH];

      /*
       * Yes.  Use the registered default extension, or the first extension in
       * the list of extensions.
       */

      bResult = MIME_GetExtension(pcszMIMEType, szDefExtension, SIZECHARS(szDefExtension));

      if (! bResult)
      {
      	if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
            /* New File Type dialog. */
            bResult = GetAssociatedExtension(hdlg, szDefExtension,
                                             SIZECHARS(szDefExtension));
         else
            /* Edit File Type dialog. */
            bResult = GetFirstString(pFTDInfo->pFTInfo->hDPAExt, szDefExtension,
                                     SIZECHARS(szDefExtension));
      }

      if (bResult)
         bResult = AddAndSetComboBoxCurrentSelection(pFTDInfo->hwndDefExtensionComboBox,
                                                     szDefExtension);
   }
   else
      /* No.  No default extension. */
      bResult = TRUE;

   return(bResult);
}


/*
** FillDefExtensionEditControlFromSelection()
**
** Fills the read-only edit control of the Default Extension combo box based
** upon the current selection of list box of the Content Type combo box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL FillDefExtensionEditControlFromSelection(HWND hdlg)
{
   BOOL bResult = TRUE;
   PFILETYPESDIALOGINFO pFTDInfo;
   TCHAR szMIMEType[MAX_PATH];
   LONG liSel;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   liSel = SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_GETCURSEL, 0, 0);

   if (liSel != CB_ERR)
   {
      bResult = (SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_GETLBTEXT,
                             liSel, (LPARAM)szMIMEType) != CB_ERR &&
                 SetDefExtension(hdlg, szMIMEType));

      SetDefExtensionComboBoxState(hdlg, szMIMEType);
   }
   else
      TraceMsg(TF_FILETYPE, "FillDefExtensionEditControlFromSelection(): No MIME type selection.");

   return(bResult);
}


/*
** FillDefExtensionEditControlFromEditControl()
**
** Fills the read-only edit control of the Default Extension combo box based
** upon the contents of the edit control of the Content Type combo box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL FillDefExtensionEditControlFromEditControl(HWND hdlg)
{
   PFILETYPESDIALOGINFO pFTDInfo;
   TCHAR szMIMEType[MAX_PATH];

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   GetWindowText(pFTDInfo->hwndContentTypeComboBox, szMIMEType,
                 SIZECHARS(szMIMEType));

   SetDefExtensionComboBoxState(hdlg, szMIMEType);

   return(SetDefExtension(hdlg, szMIMEType));
}


/*
** SetMIMEControlState()
**
** Enables or disables the MIME controls:
**    1) the Content Type combo box
**    2) the Default Extension combo box
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static void SetMIMEControlState(HWND hdlg, BOOL bEnable)
{
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   EnableWindow(pFTDInfo->hwndContentTypeComboBox, bEnable);
   EnableWindow(pFTDInfo->hwndDefExtensionComboBox, bEnable);
}


/*************************** Public MIME Functions ***************************/


/*
** InitMIMEControls()
**
** Initializes contents of Content Type combo box and Default Extension combo
** box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL InitMIMEControls(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hdlg, DWL_USER);

   SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_LIMITTEXT, MAX_PATH - 1, 0);

   if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
   {
      /* New File Type dialog. */

      ClearFlag(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME);

      *(pFTDInfo->pFTInfo->szOriginalMIMEType) = '\0';

      bResult = TRUE;

      TraceMsg(TF_FILETYPE, "InitMIMEControls(): Cleared MIME controls for New File Type dialog box.");
   }
   else
   {
      /* Edit File Type dialog. */

      /*
       * Disable MIME controls if requested, and when editing non-extension
       * types.
       */

      if (IsFlagClear(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
      {
         if (! IsListOfExtensions(pFTDInfo->pFTInfo->hDPAExt))
         {
            SetFlag(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME);

            TraceMsg(TF_FILETYPE, "InitMIMEControls(): Disabling MIME controls for non-extension type.");
         }
      }
      else
         TraceMsg(TF_FILETYPE, "InitMIMEControls(): Disabling MIME controls, as requested.");

      if (IsFlagClear(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
      {
         /* Initialize contents of MIME controls for extensions. */

         bResult = InitContentTypeEditControl(hdlg);
         bResult = (FillDefExtensionEditControlFromSelection(hdlg) &&
                    bResult);

         /*
          * Don't call FillContentTypeListBox() and
          * FillDefExtensionListBox() here.  Wait until user drops them
          * down.
          */

         TraceMsg(TF_FILETYPE, "InitMIMEControls(): Initialized MIME controls for Edit File Type dialog box.");
      }
      else
      {
         /* Disable MIME controls. */

         SetMIMEControlState(hdlg, FALSE);

         bResult = TRUE;
      }
   }

   SetDefExtensionComboBoxState(hdlg, pFTDInfo->pFTInfo->szOriginalMIMEType);

   return(bResult);
}


/*
** OnContentTypeSelectionChange()
**
** Updates MIME controls after selection change in the list box of the Content
** Type combo box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL OnContentTypeSelectionChange(HWND hdlg)
{
   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   return(FillDefExtensionEditControlFromSelection(hdlg));
}


/*
** OnContentTypeEditChange()
**
** Updates MIME controls after edit change in the edit control of the Content
** Type combo box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL OnContentTypeEditChange(HWND hdlg)
{
   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   return(FillDefExtensionEditControlFromEditControl(hdlg));
}


/*
** OnContentTypeDropDown()
**
** Updates MIME controls after drop down of the list box of the Content Type
** combo box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL OnContentTypeDropDown(HWND hdlg)
{
   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   return(FillContentTypeListBox(hdlg));
}


/*
** OnDefExtensionDropDown()
**
** Updates MIME controls after drop down of the list box of the Default
** Extension combo box.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL OnDefExtensionDropDown(HWND hdlg)
{
   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   return(FillDefExtensionListBox(hdlg));
}


/*
** RegisterMIMEInformation()
**
** Registers current MIME information at close of New File Type dialog or Edit
** File Type dialog.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
static BOOL RegisterMIMEInformation(PFILETYPESDIALOGINFO pFTDInfo)
{
    BOOL bResult;

    if (IsFlagClear(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
    {
        TCHAR szMIMEType[MAX_PATH];
        TCHAR szDefExtension[MAX_PATH];

        GetWindowText(pFTDInfo->hwndContentTypeComboBox, szMIMEType,
                     SIZECHARS(szMIMEType));

        GetWindowText(pFTDInfo->hwndDefExtensionComboBox, szDefExtension,
                     SIZECHARS(szDefExtension));

        if (*szMIMEType)
        {
            /*
             * Yes.  Register the MIME type for each extension in the list,
             * and set the default extension for the MIME type.
             */

            ASSERT(lstrlen(szDefExtension) > 0);

            bResult = AddMIMETypeInfo(pFTDInfo,
                                      pFTDInfo->pFTInfo->szOriginalMIMEType,
                                      szMIMEType, szDefExtension);
        }
        else
        {
            /*
             * No.  Clear the MIME type for each extension in the list, and
             * choose a new default extension for the MIME type.
             */

            ASSERT(! lstrlen(szDefExtension));

            bResult = RemoveMIMETypeInfo(pFTDInfo,
                                         pFTDInfo->pFTInfo->szOriginalMIMEType);
        }
    }
    else
    {
        bResult = TRUE;

        TraceMsg(TF_FILETYPE, "RegisterMIMEInformation(): Not registering MIME information, as requested.");
    }

    return(bResult);
}



static HKEY GetHkeyFT(LPTSTR pszId)
{
    HKEY hkeyFT;

    if(RegCreateKey(HKEY_CLASSES_ROOT, pszId, &hkeyFT) != ERROR_SUCCESS)
        hkeyFT = NULL;

    return(hkeyFT);
}


//================================================================
//  This function returns non-zero value only if the specified type
// has an associated icon specified by "DefaultIcon=" key.
//================================================================
static HICON GetDefaultIcon(HKEY *hkeyFT, LPTSTR pszId, DWORD dwIconType)
{
    HICON hicon = (HICON)NULL;
    LONG err;
    TCHAR szDefaultIcon[MAX_PATH];
    int iIconIndex;
    int iImage;

    ASSERT(hkeyFT != NULL);

    if(*hkeyFT == NULL)
        *hkeyFT = GetHkeyFT(pszId);

    if(*hkeyFT != (HKEY)NULL)
    {
        err = SIZEOF(szDefaultIcon);
        err = RegQueryValue(*hkeyFT, c_szDefaultIcon, szDefaultIcon, &err);
        if (err == ERROR_SUCCESS && *szDefaultIcon)
        {
            iIconIndex = PathParseIconLocation(szDefaultIcon);
            PathRemoveArgs(szDefaultIcon);
            iImage = Shell_GetCachedImageIndex(szDefaultIcon, iIconIndex, 0);
            hicon = ImageList_ExtractIcon(g_hinst, (dwIconType == SHGFI_LARGEICON ? g_himlSysLarge : g_himlSysSmall), iImage);
        }
    }

    TraceMsg(TF_FILETYPE, "FT GetDefaultIcon szIcon=%s iIndex=%d hIcon=0x%x hkeyFT=0x%x",
            szDefaultIcon, iIconIndex, hicon, *hkeyFT);

    return(hicon);
}


static HICON GetDocIcon(PFILETYPESDIALOGINFO pFTDInfo, LPTSTR lpszStr)
{
    SHFILEINFO sfi;
    HICON hIcon = (HICON)NULL;
    int iImageIndex;

    if(pFTDInfo->pFTInfo->dwAttributes & FTA_HasExtension)
    {
        if(*lpszStr == TEXT('.'))  // is an extension
        {
            if(SHGetFileInfo((LPTSTR)lpszStr, FILE_ATTRIBUTE_NORMAL, &sfi, SIZEOF(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES))
                hIcon = sfi.hIcon;
        }
        else
        {
            // get simulated doc icon if from exe name
            iImageIndex = Shell_GetCachedImageIndex(lpszStr, 0, GIL_SIMULATEDOC);
            hIcon = ImageList_ExtractIcon(g_hinst, g_himlSysLarge, iImageIndex);
        }
    }
    else    // special case for folder, drive etc.
    {
        if((hIcon = GetDefaultIcon(&pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->szId, SHGFI_LARGEICON)) == (HICON)NULL)
            // use default shell icon in case above calls fail to find an icon
            hIcon = ImageList_ExtractIcon(g_hinst, g_himlSysLarge, II_DOCNOASSOC);
    }

    return(hIcon);
}


static VOID DisplayDocObjects(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    LV_ITEM LVItem;
    TCHAR szFile[MAX_PATH];
    int iCnt;
    int i;

    // Display extensions in read-only edit control
    iCnt = DPA_GetPtrCount(pFTDInfo->pFTInfo->hDPAExt);
    *szFile = TEXT('\0');
    for(i = 0; (i < iCnt) && (lstrlen(szFile) < MAX_PATH); i++)
    {
        if(*(LPTSTR)(DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i)))
        {
            // Make sure we have enough room left in our string...
            if ((lstrlen(szFile) +
                lstrlen((LPTSTR)DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i)))
                >= (MAX_PATH - 2))
                break;
            lstrcat(szFile, (LPTSTR)DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i) + 1);
            lstrcat(szFile, TEXT(" "));
        }
    }
    SetDlgItemText(hDialog, IDC_FT_PROP_DOCEXTRO, CharUpper(szFile));

    // Display MIME type in read-only edit control.
    if (IsListOfExtensions(pFTDInfo->pFTInfo->hDPAExt))
        FindMIMETypeOfExtensionList(pFTDInfo->pFTInfo->hDPAExt, szFile, SIZECHARS(szFile));
    else
        *szFile = '\0';
    SetDlgItemText(hDialog, IDC_FT_PROP_CONTTYPERO, szFile);
    lstrcpy(pFTDInfo->pFTInfo->szOriginalMIMEType, szFile);

    // Get doc icon if not already gotten
    if(pFTDInfo->pFTInfo->hIconDoc == (HICON)NULL)
    {
        pFTDInfo->pFTInfo->hIconDoc = GetDocIcon(pFTDInfo, DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0));

        // Get the image index from the list view item
        LVItem.mask        = LVIF_IMAGE;
        LVItem.iItem       = pFTDInfo->iItem;
        LVItem.iSubItem    = 0;
        ListView_GetItem(pFTDInfo->hwndLVFT, &LVItem);
        if(ImageList_ReplaceIcon(pFTDInfo->himlFT, LVItem.iImage, pFTDInfo->pFTInfo->hIconDoc) != (-1))
            ListView_SetItem(pFTDInfo->hwndLVFT, &LVItem);
    }

    // Display document object icon
    if(pFTDInfo->pFTInfo->hIconDoc != (HICON)NULL)
    {
        PostMessage(pFTDInfo->hwndDocIcon, STM_SETIMAGE, IMAGE_ICON,
                (LPARAM)pFTDInfo->pFTInfo->hIconDoc);
    }
}


static VOID ExtToShellCommand(HKEY hkeyFT, LPTSTR pszName, UINT uName)
{
    TCHAR szIdValue[MAX_PATH];
    TCHAR szShellCommand[MAX_PATH];
    TCHAR szShellCommandValue[MAX_PATH];
    TCHAR szClass[MAX_PATH];
    TCHAR szShellKey[MAX_PATH];
    DWORD dwClass;
    DWORD dwShellKey;
    DWORD dwSubKey;
    FILETIME ftLastWrite;
    LONG err = E_INVALIDARG;    // For proper handling of no hkey

    if(hkeyFT)
    {
        // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell value
        err = SIZEOF(szIdValue);
        err = RegQueryValue(hkeyFT, c_szShell, szIdValue, &err);
        if (err == ERROR_SUCCESS && *szIdValue)
        {
            // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell\[szIdValue]\Command value
            wsprintf(szShellCommand, c_szTemplateSSS, c_szShell, szIdValue, TEXT("command"));
            err = SIZEOF(szShellCommand);
            err = RegQueryValue(hkeyFT, szShellCommand, szShellCommandValue, &err);
        }
        else
        {
            // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell\Open\Command value
            err = SIZEOF(szShellCommandValue);
            err = RegQueryValue(hkeyFT, c_szShellOpenCommand, szShellCommandValue, &err);
            if (err != ERROR_SUCCESS || !*szShellCommandValue)
            {
                HKEY hk;

                // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell\[1st Key]\Command value
                if(RegOpenKeyEx(hkeyFT, c_szShell, (DWORD)NULL, KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS)
                {
                    dwClass = ARRAYSIZE(szClass);
                    dwShellKey = ARRAYSIZE(szShellKey);
                    dwSubKey = 0;
                    if(RegEnumKeyEx(hk, dwSubKey, szShellKey, &dwShellKey, NULL, szClass, &dwClass, &ftLastWrite) == ERROR_SUCCESS)
                    {
                        wsprintf(szShellCommand, c_szTemplateSS, szShellKey, TEXT("command"));
                        err = SIZEOF(szShellCommandValue);
                        err = RegQueryValue(hk, szShellCommand, szShellCommandValue, &err);
                    }

                    RegCloseKey(hk);
                }
            }
        }
    }

    if(err == ERROR_SUCCESS)
        lstrcpyn(pszName, szShellCommandValue, uName);
    else
        *pszName =TEXT('\0');
}


static VOID DisplayOpensWithObjects(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    TCHAR szFile[MAX_PATH];
    TCHAR szFullPath[MAX_PATH];
    SHFILEINFO sfi;

    // Get default action's executable
    //    Search order:
    //    1. FTID\[value-of-FTID\shell-key]\command
    //    2. FTID\open\command
    //    3. FTID\[1st-FTID\shell-subkey]\command
    ExtToShellCommand(pFTDInfo->pFTInfo->hkeyFT, szFile, ARRAYSIZE(szFile));

    if(*szFile)
    {
        int cchT = lstrlen(c_szExefileOpenCommand);
        if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
            szFile, cchT, c_szExefileOpenCommand, cchT) == 2)
        {
            // handle types like exefile, comfile, & batfile that don't have exe

            // Get open icon if not already gotten
            pFTDInfo->pFTInfo->hIconOpen = (HICON)NULL;
            PostMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)NULL);
            MLLoadString(IDS_FT_EXEFILE, szFullPath, ARRAYSIZE(szFullPath));
        }
        else
        {
            // Attempt to fix up the filename, if that fails then just strip the name
            // enough so that we can display something sensible.

            if ( PathProcessCommand( szFile, szFile, MAX_PATH, PPCF_NODIRECTORIES ) == -1 )
            {
                PathRemoveArgs(szFile);
                PathRemoveBlanks(szFile);
                PathUnquoteSpaces(szFile);
            }

            lstrcpy(szFullPath, szFile);

            // Get open icon if not already gotten
            if(pFTDInfo->pFTInfo->hIconOpen == (HICON)NULL)
            {
                sfi.hIcon = 0;
                if(SHGetFileInfo(szFullPath, FILE_ATTRIBUTE_NORMAL, &sfi, SIZEOF(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES))
                {
                    if(sfi.hIcon != (HICON)NULL)
                        pFTDInfo->pFTInfo->hIconOpen = sfi.hIcon;
                    else
                        pFTDInfo->pFTInfo->hIconOpen = ImageList_ExtractIcon(g_hinst, g_himlSysLarge, II_DOCNOASSOC);
                }
            }
        }

        // Display open icon
        if(pFTDInfo->pFTInfo->hIconOpen != (HICON)NULL)
            PostMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pFTDInfo->pFTInfo->hIconOpen);

        // Display exe name
        lstrcpy(szFile, PathFindFileName(szFullPath));  // Strip off the path
        if(*szFile)
        {
            PathRemoveExtension(szFile);    // Strip off the extension
            SetDlgItemText(hDialog, IDC_FT_PROP_OPENEXE, CharUpper(szFile));
        }
    }
    else
    {
        PostMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)NULL);
        SetDlgItemText(hDialog, IDC_FT_PROP_OPENEXE, c_szNULL);
    }
}


static LONG RemoveFileType(PFILETYPESDIALOGINFO pFTDInfo)
{
    LONG lRC = ERROR_SUCCESS;
    LV_ITEM LVItem;
    int i;
    int iCnt;
    LPTSTR pszExt;
    TCHAR szKey[MAX_PATH];
    TCHAR szBuf[2];
    HKEY hk;

    // Remove filetype and keys from the registry
    if(*pFTDInfo->pFTInfo->szId)
            if(SHDeleteKey(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId) != ERROR_SUCCESS)
                    lRC = !ERROR_SUCCESS;

    // Free allocated memory &
    // Remove extension(s) and their keys from the registry
    LVItem.mask = LVIF_PARAM;
    LVItem.iItem = pFTDInfo->iItem;
    LVItem.iSubItem = 0;

    ListView_GetItem(pFTDInfo->hwndLVFT, &LVItem);
    if(pFTDInfo->pFTInfo = (PFILETYPESINFO)LVItem.lParam)
    {
        if(pFTDInfo->pFTInfo->hDPAExt != (HDPA)NULL)
        {
            if (! RemoveMIMETypeInfo(pFTDInfo, pFTDInfo->pFTInfo->szOriginalMIMEType))
                lRC = !ERROR_SUCCESS;
            iCnt = DPA_GetPtrCount(pFTDInfo->pFTInfo->hDPAExt);
            for(i = 0; i < iCnt; i++)
            {
                if(pszExt = DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i))
                {
                    if(*pszExt)
                    {
                        // Don't delete extension if it has a ShellNew key, just remove filetype
                        wsprintf(szKey, TEXT("%s\\%s%s"), pszExt, c_szShell, c_szNew);
                        if(RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hk) == ERROR_SUCCESS)
                        {
                            RegCloseKey(hk);
                            *szBuf = TEXT('\0');  // remove the filetype assoc
                            RegSetValue(HKEY_CLASSES_ROOT, pszExt,
                                            REG_SZ, szBuf, ARRAYSIZE(szBuf));
                        }
                        else
                        {
                            if(SHDeleteKey(HKEY_CLASSES_ROOT, pszExt) != ERROR_SUCCESS)
                                lRC = !ERROR_SUCCESS;
                        }
                    }
                }
            }
        }
        pFTDInfo->pFTInfo = NULL;   // don't attempt to go through deleted pointer
    }

    // Remove item from list view
    ListView_DeleteItem(pFTDInfo->hwndLVFT, pFTDInfo->iItem);

    pFTDInfo->iItem = ListView_GetNextItem(pFTDInfo->hwndLVFT, -1, LVNI_FOCUSED);
    ListView_SetItemState(pFTDInfo->hwndLVFT, pFTDInfo->iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    SetFocus(pFTDInfo->hwndLVFT);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return(lRC);
}


static DWORD GetVerbAttributes(HKEY hkeyFT, LPTSTR pszVerb)
{
    HKEY hk;
    LONG err;
    TCHAR szVerbKey[MAX_PATH];
    DWORD dwType;
    DWORD dwAttributes = 1;
    DWORD dwSize;

    wsprintf(szVerbKey, c_szTemplateSS, c_szShell, pszVerb);
    if((hkeyFT != NULL) && (RegOpenKeyEx(hkeyFT, szVerbKey, (DWORD)NULL, KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS))
    {
        dwSize = SIZEOF(dwAttributes);
        err = RegQueryValueEx(hk, (LPTSTR)c_szEditFlags, NULL, &dwType, (LPBYTE)&dwAttributes, &dwSize);
        if (err != ERROR_SUCCESS ||
            (dwType != REG_BINARY && dwType != REG_DWORD) ||
            dwSize != SIZEOF(dwAttributes))
        {
            dwAttributes = 0;
        }
        RegCloseKey(hk);
    }

    return(dwAttributes);
}


static BOOL IsDefaultAction(PFILETYPESDIALOGINFO pFTDInfo, LPTSTR pszAction)
{
    return((lstrcmpi(pFTDInfo->pFTInfo->szDefaultAction, pszAction) == 0) ||
       (!(*pFTDInfo->pFTInfo->szDefaultAction) && (lstrcmpi(pszAction, TEXT("open")) == 0)));
}


static DWORD SetVerbAttributes(HKEY hkeyFT, LPTSTR pszVerb, DWORD dwAttributes)
{
    HKEY hk;
    LONG err;
    TCHAR szVerbKey[MAX_PATH];
    DWORD dwSize;

    wsprintf(szVerbKey, c_szTemplateSS, c_szShell, pszVerb);
    if((hkeyFT != NULL) && (RegOpenKeyEx(hkeyFT, szVerbKey, (DWORD)NULL, KEY_SET_VALUE, &hk) == ERROR_SUCCESS))
    {
        dwSize = SIZEOF(dwAttributes);
        err = RegSetValueEx(hk, (LPTSTR)c_szEditFlags, 0, REG_BINARY, (LPBYTE)&dwAttributes, dwSize);
        if (err != ERROR_SUCCESS)
            dwAttributes= 0;
        RegCloseKey(hk);
    }

    return(dwAttributes);
}


static LONG DeleteDDEKeys(LPCTSTR pszKey)
{
    TCHAR szBuf[MAX_PATH];

    // Delete DDEApp keys
    wsprintf(szBuf, c_szTemplateSS, pszKey, c_szDDEExec);
    return(SHDeleteKey(HKEY_CLASSES_ROOT, szBuf));
}


static LONG SaveFileTypeData(DWORD dwName, PFILETYPESDIALOGINFO pFTDInfo)
{
    LONG lRC = ERROR_SUCCESS;
    HKEY hk;
    HKEY hk2;
    TCHAR szBuf[MAX_PATH];
    TCHAR szAction[MAX_PATH];

    switch(dwName)
    {
    case FTD_EDIT:
        // Save file type id and description
        if(RegSetValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId, REG_SZ, pFTDInfo->pFTInfo->szDesc,
                        ARRAYSIZE(pFTDInfo->pFTInfo->szDesc)) != ERROR_SUCCESS)
            lRC = !ERROR_SUCCESS;

        // Save default action key and value
        wsprintf(szBuf, c_szTemplateSS, pFTDInfo->pFTInfo->szId, c_szShell);
        if(RegSetValue(HKEY_CLASSES_ROOT, szBuf, REG_SZ, pFTDInfo->pFTInfo->szDefaultAction,
                        ARRAYSIZE(szAction)) != ERROR_SUCCESS)
            lRC = !ERROR_SUCCESS;
        break;

    case FTD_DOCICON:
        wsprintf(szBuf, c_szTemplateSS, pFTDInfo->pFTInfo->szId, c_szDefaultIcon);
        wsprintf(szAction, TEXT("%s,%d"), pFTDInfo->szIconPath, pFTDInfo->iIconIndex);
        if(RegSetValue(HKEY_CLASSES_ROOT, szBuf, REG_SZ, szAction,
                        ARRAYSIZE(szAction)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;
        break;

    case FTD_EXT:
        // Save extension and file type id
        if(RegSetValue(HKEY_CLASSES_ROOT,
                        DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0),
                        REG_SZ, pFTDInfo->pFTInfo->szId,
                        ARRAYSIZE(pFTDInfo->pFTInfo->szId)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;
        break;

    case FTD_MIME:
        // Save MIME type.
        if (! RegisterMIMEInformation(pFTDInfo))
            lRC = !ERROR_SUCCESS;
        break;

    case FTD_COMMAND:
        // Create/Open HKEY_CLASSES_ROOT\filetype\shell\action key
        wsprintf(szBuf, c_szTemplateSSS,
                pFTDInfo->pFTCInfo->szId, c_szShell, pFTDInfo->pFTCInfo->szActionKey);
        if(RegCreateKey(HKEY_CLASSES_ROOT, szBuf, &hk) == ERROR_SUCCESS)
        {
            // Tag as user defined verb
            if(pFTDInfo->pFTCInfo->dwVerbAttributes)
                SetVerbAttributes(pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->pFTCInfo->szActionKey, pFTDInfo->pFTCInfo->dwVerbAttributes);

            // Save action verb key and value if string has accelerator
            if(lstrcmp(pFTDInfo->pFTCInfo->szActionKey,
                    pFTDInfo->pFTCInfo->szActionValue) != 0)
            {
                if(RegSetValue(hk, NULL, REG_SZ, pFTDInfo->pFTCInfo->szActionValue,
                                ARRAYSIZE(pFTDInfo->pFTCInfo->szActionValue)) != ERROR_SUCCESS)
                    lRC = !ERROR_SUCCESS;
            }

            // Save action command key and value
            if(RegSetValue(hk, TEXT("command"), REG_SZ, pFTDInfo->pFTCInfo->szCommand,
                            ARRAYSIZE(pFTDInfo->pFTCInfo->szCommand)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;

            if(IsDlgButtonChecked(pFTDInfo->hCmdDialog, IDC_FT_CMD_USEDDE))
            {
                // Save DDE Message key and value
                if(*pFTDInfo->pFTCInfo->szDDEMsg)
                {
                    if(RegSetValue(hk, c_szDDEExec, REG_SZ, pFTDInfo->pFTCInfo->szDDEMsg,
                                    ARRAYSIZE(pFTDInfo->pFTCInfo->szDDEMsg)) != ERROR_SUCCESS)
                        lRC = !ERROR_SUCCESS;
                }

                if(RegCreateKey(hk, c_szDDEExec, &hk2) == ERROR_SUCCESS)
                {
                    // Save DDEApp key and value
                    if(*pFTDInfo->pFTCInfo->szDDEApp)
                    {
                        if(RegSetValue(hk2, c_szDDEApp, REG_SZ, pFTDInfo->pFTCInfo->szDDEApp,
                                        ARRAYSIZE(pFTDInfo->pFTCInfo->szDDEApp)) != ERROR_SUCCESS)
                            lRC = !ERROR_SUCCESS;
                    }

                    // Save DDEAppNot key and value
                    if(*pFTDInfo->pFTCInfo->szDDEAppNot)
                    {
                        if(RegSetValue(hk2, c_szDDEAppNot, REG_SZ, pFTDInfo->pFTCInfo->szDDEAppNot,
                                        ARRAYSIZE(pFTDInfo->pFTCInfo->szDDEAppNot)) != ERROR_SUCCESS)
                            lRC = !ERROR_SUCCESS;
                    }

                    // Save DDETopic key and value
                    if(*pFTDInfo->pFTCInfo->szDDETopic)
                    {
                        if(RegSetValue(hk2, c_szDDETopic, REG_SZ, pFTDInfo->pFTCInfo->szDDETopic,
                                        ARRAYSIZE(pFTDInfo->pFTCInfo->szDDETopic)) != ERROR_SUCCESS)
                            lRC = !ERROR_SUCCESS;
                    }
                    RegCloseKey(hk2);
                }
            }
            else
                DeleteDDEKeys(szBuf);

            RegCloseKey(hk);
        }
        else
            lRC = !ERROR_SUCCESS;
        break;
    }

    return(lRC);
}


static BOOL SetDefaultAction(PFILETYPESDIALOGINFO pFTDInfo)
{
    TCHAR szFile[MAX_PATH];
    LV_ITEM LVItem;

    if(IsDefaultAction(pFTDInfo, pFTDInfo->pFTCInfo->szActionKey))
        *pFTDInfo->pFTInfo->szDefaultAction = TEXT('\0');
    else
        lstrcpy(pFTDInfo->pFTInfo->szDefaultAction, pFTDInfo->pFTCInfo->szActionKey);

    // This will cause the new icon and exe to be reretreived and displayed when select in prop sheet
    if(pFTDInfo->pFTInfo->hIconOpen != (HICON)NULL)
    {
        DestroyIcon(pFTDInfo->pFTInfo->hIconOpen);
        pFTDInfo->pFTInfo->hIconOpen = (HICON)NULL;
        SendMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)0);
    }

    if(pFTDInfo->pFTInfo->hIconDoc != (HICON)NULL)
    {
        DestroyIcon(pFTDInfo->pFTInfo->hIconDoc);
        pFTDInfo->pFTInfo->hIconDoc = (HICON)NULL;
        SendMessage(pFTDInfo->hwndDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)0);
    }

    // Save default action
    SaveFileTypeData(FTD_EDIT, pFTDInfo);
///     if(IsDefaultAction(pFTDInfo, szAction))
///     {
    ExtToShellCommand(pFTDInfo->pFTInfo->hkeyFT, szFile, ARRAYSIZE(szFile));
    PathRemoveArgs(szFile);
    PathRemoveBlanks(szFile);
    if(PathIsRelative(szFile))
        PathFindOnPath(szFile, NULL);   // search for exe

    //
    //  First, try to get the icon based of "DefaultIcon=" key.
    // If it fails, then we'll get the document icon from the
    // newly specified exe file.
    //
    pFTDInfo->pFTInfo->hIconDoc = GetDefaultIcon(&pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->szId, SHGFI_LARGEICON);
    if (pFTDInfo->pFTInfo->hIconDoc==NULL) {
        pFTDInfo->pFTInfo->hIconDoc = GetDocIcon(pFTDInfo, szFile);
    }

    SendMessage(pFTDInfo->hwndEditDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pFTDInfo->pFTInfo->hIconDoc);

    // Get the image index from the list view item
    LVItem.mask        = LVIF_IMAGE;
    LVItem.iItem       = pFTDInfo->iItem;
    LVItem.iSubItem    = 0;
    ListView_GetItem(pFTDInfo->hwndLVFT, &LVItem);

    // replace the icon in the image list
    if(pFTDInfo->himlFT && (LVItem.iImage >= 0) && pFTDInfo->pFTInfo->hIconDoc)
        if(ImageList_ReplaceIcon(pFTDInfo->himlFT, LVItem.iImage, pFTDInfo->pFTInfo->hIconDoc) != (-1))
            ListView_SetItem(pFTDInfo->hwndLVFT, &LVItem);
///     }

    return(TRUE);
}


static VOID StrRemoveChar(LPTSTR pszSrc, LPTSTR pszDest, TCHAR ch)
{
    LPTSTR pSrc = pszSrc;
    LPTSTR pDest = (pszDest ?pszDest :pszSrc);

    ASSERT(pSrc);
    ASSERT(pDest);

    if(pSrc && pDest)
    {
        while(*pSrc)
        {
            if(*pSrc != ch)
                *(pDest++) = *pSrc;
            pSrc++;
        }
        *pDest = TEXT('\0');
    }
}


static VOID AddExtDot(LPTSTR pszExt, int iExt)
{
    PathRemoveBlanks(pszExt);       // remove 1st and last blank
    StrRemoveChar(pszExt, NULL, TEXT('.')); // remove all dots
    hmemcpy(pszExt+1, pszExt, (lstrlen(pszExt)+1)*SIZEOF(TCHAR)); // make room for dot
    *pszExt = TEXT('.');    // insert dot
}


static BOOL ValidExtension(HWND hDialog, PFILETYPESDIALOGINFO pFTDInfo)
{
    BOOL bRC = TRUE;
    TCHAR szExt[MAX_PATH];
    TCHAR szId[MAX_PATH];
    TCHAR szBuf[MAX_PATH];
    TCHAR szStr1[256];
    TCHAR szStr2[256];
    DWORD dwId;
    HWND hwndButton;

    // On new types verify that the extension is not already in use.
    GetDlgItemText(hDialog, IDC_FT_EDIT_EXT, szExt, ARRAYSIZE(szExt));

    if(*szExt)
    {
        AddExtDot(szExt, ARRAYSIZE(szExt));

        dwId = SIZEOF(szId);
        *szId = TEXT('\0');
        if((RegQueryValue(HKEY_CLASSES_ROOT, szExt, szId, &dwId) == ERROR_SUCCESS) && *szId)
        {
            // Disable OK button
            hwndButton = GetDlgItem(hDialog, IDOK);
            EnableWindow(hwndButton, FALSE);

            // Tell user that this extension is already in use
            *szBuf = TEXT('\0');
            *szStr2 = TEXT('\0');
            if(MLLoadString(IDS_FT_MB_EXTTEXT, szStr1, ARRAYSIZE(szStr1)))
            {
                if(MLLoadString(IDS_FT, szStr2, ARRAYSIZE(szStr2)))
                {
                    if(lstrlen(szStr1) + lstrlen(szExt) + lstrlen(szId) < ARRAYSIZE(szBuf))
                        wsprintf(szBuf, szStr1, szExt, szId);
                }
            }
            MessageBox(hDialog, szBuf, szStr2, MB_OK | MB_ICONSTOP);
            PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0, (LPARAM)GetDlgItem(hDialog, IDC_FT_EDIT_EXT));
            EnableWindow(hwndButton, TRUE);  // Enable OK
            bRC = FALSE;
        }
        else if(!(*pFTDInfo->szId))
        {
            HKEY hk;
            int iCnt = 1;
            LPTSTR pszExt = szExt;

            if(*pszExt == TEXT('.'))
                pszExt++;   // remove dot

            // Create unique file type id
            lstrcpy(pFTDInfo->szId, pszExt);
            lstrcat(pFTDInfo->szId, c_szFile);

            while(RegOpenKey(HKEY_CLASSES_ROOT, pFTDInfo->szId, &hk) == ERROR_SUCCESS)
            {
                RegCloseKey(hk);
                wsprintf(pFTDInfo->szId, TEXT("%s%s%02d"),
                        pszExt, c_szFile, iCnt);
                iCnt++;
            }
            EnableWindow(GetDlgItem(pFTDInfo->hEditDialog, IDC_FT_EDIT_NEW), TRUE);
        }
    }
    else
    {
        // Tell the use that an extension is required
        *szBuf = TEXT('\0');
        *szStr2 = TEXT('\0');
        if(MLLoadString(IDS_FT_MB_NOEXT, szStr1, ARRAYSIZE(szStr1)))
        {
            if(MLLoadString(IDS_FT, szStr2, ARRAYSIZE(szStr2)))
            {
                if(lstrlen(szStr1) + lstrlen(szExt) + lstrlen(szId) < ARRAYSIZE(szBuf))
                    wsprintf(szBuf, szStr1, szExt, szId);
            }
        }
        MessageBox(hDialog, szBuf, szStr2, MB_OK | MB_ICONSTOP);
        PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0, (LPARAM)GetDlgItem(hDialog, IDC_FT_EDIT_EXT));
        bRC = FALSE;
    }

    return(bRC);
}


static DWORD GetFileTypeAttributes(HKEY hkeyFT)
{
    LONG err;
    DWORD dwType;
    DWORD dwAttributeValue = 0;
    DWORD dwAttributeSize;

    if (hkeyFT == NULL)
        return 0;

    dwAttributeSize = SIZEOF(dwAttributeValue);
    err = RegQueryValueEx(hkeyFT, (LPTSTR)c_szEditFlags, NULL, &dwType, (LPBYTE)&dwAttributeValue, &dwAttributeSize);

    if (err != ERROR_SUCCESS ||
        (dwType != REG_BINARY && dwType != REG_DWORD) ||
        dwAttributeSize != SIZEOF(dwAttributeValue))
    {
        dwAttributeValue = 0;
    }

    return(dwAttributeValue);
}


static BOOL DefaultAction(HKEY hkeyFT, LPTSTR pszDefaultAction, DWORD *dwDefaultAction)
{
    LONG err;

    err = RegQueryValue(hkeyFT, c_szShell, pszDefaultAction, dwDefaultAction);
    if (err == ERROR_SUCCESS && *pszDefaultAction)
        return(TRUE);

    return(FALSE);
}


static VOID VerbToExe(HKEY hkeyFT, LPTSTR pszVerb, LPTSTR pszExe, DWORD *pdwExe)
{
    // caller is responsible to setting pdwExe
    TCHAR ach[MAX_PATH];
    LONG err;

    wsprintf(ach, c_szTemplateSSS, c_szShell, pszVerb, TEXT("command"));
    err = RegQueryValue(hkeyFT, ach, pszExe, pdwExe);
    if (err != ERROR_SUCCESS || !*pszExe)
    {
        *pdwExe = 0;
    }
}


static VOID OkToClose_NoCancel(HINSTANCE hinst, HWND hDialog)
{
    TCHAR szStr1[256];

    if(MLLoadString(IDS_FT_CLOSE, szStr1, ARRAYSIZE(szStr1)))
    {
        SetWindowText(GetDlgItem(hDialog, IDOK), szStr1);
        EnableWindow(GetDlgItem(hDialog, IDCANCEL), FALSE);
    }
}


static BOOL ActionIsEntered(HWND hDialog, BOOL bMBoxFlag)
{
    BOOL bRC = TRUE;
    TCHAR szAction[MAX_PATH];

    // Check for value
    if(!GetDlgItemText(hDialog, IDC_FT_CMD_ACTION, szAction, ARRAYSIZE(szAction)))
    {
        if(bMBoxFlag)
        {
            // Tell user that this exe is invalid
            ShellMessageBox(MLGetHinst(), hDialog, MAKEINTRESOURCEA(IDS_FT_MB_NOACTION), MAKEINTRESOURCEA(IDS_FT), MB_OK | MB_ICONSTOP);
            PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0, (LPARAM)GetDlgItem(hDialog, IDC_FT_CMD_ACTION));
        }
        bRC = FALSE;
    }
    return(bRC);
}


static BOOL ActionExeIsValid(HWND hDialog, BOOL bMBoxFlag)
{
    BOOL bRC = TRUE;
    TCHAR szPath[MAX_PATH];
    TCHAR szFileName[MAX_PATH];

    // Check for valid exe
    GetDlgItemText(hDialog, IDC_FT_CMD_EXE, szPath, ARRAYSIZE(szPath));
    PathRemoveArgs(szPath);
    PathUnquoteSpaces(szPath);
    lstrcpy(szFileName, PathFindFileName(szPath));
    if(!(*szPath) || (!(PathIsExe(szPath))) || ((!(PathFileExists(szPath))) && (!(PathFindOnPath(szFileName, NULL)))))
    {
        if(bMBoxFlag)
        {
            // Tell user that this exe is invalid
            ShellMessageBox(MLGetHinst(), hDialog, MAKEINTRESOURCEA(IDS_FT_MB_EXETEXT), MAKEINTRESOURCEA(IDS_FT),
                            MB_OK | MB_ICONSTOP);
            PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0, (LPARAM)GetDlgItem(hDialog, IDC_FT_CMD_EXE));
        }
        bRC = FALSE;
    }
    return(bRC);
}




static VOID FT_MergeDuplicates(HWND hwndLV)
{
    int i;
    int iCnt;
    LV_ITEM LVItem;
    PFILETYPESINFO pFTInfo1;
    PFILETYPESINFO pFTInfo2;

    LVItem.mask = LVIF_PARAM;
    LVItem.iItem = 0;
    LVItem.iSubItem = 0;
    ListView_GetItem(hwndLV, &LVItem);  // Get item 0

    iCnt = ListView_GetItemCount(hwndLV);
    pFTInfo1 = (PFILETYPESINFO)LVItem.lParam;
    for(i = 1; i < iCnt; i++)
    {
        LVItem.iItem = i;
        ListView_GetItem(hwndLV, &LVItem);  // LVItem.lParam points to file type info
        pFTInfo2 = (PFILETYPESINFO)LVItem.lParam;

        if(lstrcmpi(pFTInfo1->szId, pFTInfo2->szId) == 0)       // we have a match
        {
            // add extension in pFTInfo1's hDPAExt
            DPA_AppendPtr(pFTInfo1->hDPAExt, (LPVOID)DPA_FastGetPtr(pFTInfo2->hDPAExt,0));
            DPA_DeletePtr(pFTInfo2->hDPAExt, 0);
            ListView_DeleteItem(hwndLV, i);
            i--;
            iCnt--;
        }
        else
        {
            pFTInfo1 = pFTInfo2;
        }
    }
}


static BOOL FindDDEOptions(PFILETYPESDIALOGINFO pFTDInfo)
{
    BOOL bRC = FALSE;
    TCHAR ach[MAX_PATH];
    LONG err;
    HKEY hkDDE;

    // see if we have a DDE Message key and value
    if(pFTDInfo->pFTInfo->hkeyFT)
    {
        wsprintf(ach, c_szTemplateSSS, c_szShell, pFTDInfo->pFTCInfo->szActionKey, c_szDDEExec);
        err = SIZEOF(pFTDInfo->pFTCInfo->szDDEMsg);
        err = RegQueryValue(pFTDInfo->pFTInfo->hkeyFT, ach, pFTDInfo->pFTCInfo->szDDEMsg, &err);
        if(err == ERROR_SUCCESS && *pFTDInfo->pFTCInfo->szDDEMsg)
        {
            bRC = TRUE;
            if(RegOpenKey(pFTDInfo->pFTInfo->hkeyFT, ach, &hkDDE) == ERROR_SUCCESS)
            {
                // see if we have a DDE Application key and value
                err = SIZEOF(pFTDInfo->pFTCInfo->szDDEApp);
                RegQueryValue(hkDDE, c_szDDEApp, pFTDInfo->pFTCInfo->szDDEApp, &err);

                // see if we have a DDE Application Not Running key and value
                err = SIZEOF(pFTDInfo->pFTCInfo->szDDEAppNot);
                RegQueryValue(hkDDE, c_szDDEAppNot, pFTDInfo->pFTCInfo->szDDEAppNot, &err);

                // see if we have a DDE Topic key and value
                err = SIZEOF(pFTDInfo->pFTCInfo->szDDETopic);
                RegQueryValue(hkDDE, c_szDDETopic, pFTDInfo->pFTCInfo->szDDETopic, &err);

                RegCloseKey(hkDDE);
            }
        }
    }
    return(bRC);
}


static VOID ResizeCommandDlg(HWND hDialog, BOOL bFlag)
{
    RECT rcDialog;
    RECT rcControl;

    GetWindowRect(hDialog, &rcDialog);

    if(bFlag)       // resize to show dde group
            GetWindowRect(GetDlgItem(hDialog, IDC_FT_CMD_DDEGROUP), &rcControl);
    else            // resize to hide dde group
            GetWindowRect(GetDlgItem(hDialog, IDC_FT_CMD_USEDDE), &rcControl);

    ShowWindow(GetDlgItem(hDialog, IDC_FT_CMD_DDEMSG), bFlag);
    ShowWindow(GetDlgItem(hDialog, IDC_FT_CMD_DDEAPP), bFlag);
    ShowWindow(GetDlgItem(hDialog, IDC_FT_CMD_DDEAPPNOT), bFlag);
    ShowWindow(GetDlgItem(hDialog, IDC_FT_CMD_DDETOPIC), bFlag);
    ShowWindow(GetDlgItem(hDialog, IDC_FT_CMD_DDEGROUP), bFlag);
    SetWindowPos(GetDlgItem(hDialog, IDC_FT_CMD_USEDDE), HWND_TOPMOST, 0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);

    MoveWindow(hDialog, rcDialog.left, rcDialog.top, rcDialog.right - rcDialog.left,
            (rcControl.bottom - rcDialog.top) + 10, TRUE);

    SetFocus(GetDlgItem(hDialog, IDC_FT_CMD_USEDDE));
}


static LONG RemoveAction(PFILETYPESDIALOGINFO pFTDInfo, HKEY hk, LPCTSTR pszKey, LPTSTR szAction)
{
    LONG lRC = ERROR_SUCCESS;
    HKEY hk1;

    // Remove keys from the registry
    if(RegOpenKeyEx(hk, pszKey, (DWORD)NULL, KEY_READ|KEY_WRITE, &hk1) == ERROR_SUCCESS)
    {
        if(SHDeleteKey(hk1, szAction) != ERROR_SUCCESS)
            lRC = !ERROR_SUCCESS;
        RegCloseKey(hk1);
    }

    // Remove item from list view - which will delete the allocated
    // structure...
    ListView_DeleteItem(pFTDInfo->hwndLVFTEdit, pFTDInfo->iEditItem);
    ListView_SetItemState(pFTDInfo->hwndLVFTEdit, pFTDInfo->iEditItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    PostMessage(pFTDInfo->hwndLVFTEdit, WM_SETFOCUS, (WPARAM)0, (LPARAM)0);

    if(pFTDInfo->pFTInfo->hIconOpen != (HICON)NULL)
    {
        DestroyIcon(pFTDInfo->pFTInfo->hIconOpen);
        pFTDInfo->pFTInfo->hIconOpen = (HICON)NULL;
        SendMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pFTDInfo->pFTInfo->hIconOpen);
    }

    return(lRC);
}


// taken from fstreex.c
static BOOL ExtToTypeNameAndId(LPTSTR pszExt, LPTSTR pszDesc, DWORD *pdwDesc, LPTSTR pszId, DWORD *pdwId)
{
    LONG err;
    DWORD dwNm;
    BOOL bRC = TRUE;

    // NOTE pdwDesc is count of BYTES

    err = RegQueryValue(HKEY_CLASSES_ROOT, pszExt, pszId, pdwId);
    if (err == ERROR_SUCCESS && *pszId)
    {
        dwNm = *pdwDesc;        // if we fail we will still have name size to use
        err = RegQueryValue(HKEY_CLASSES_ROOT, pszId, pszDesc, &dwNm);
        if (err != ERROR_SUCCESS || !*pszDesc)
                goto Error;
        *pdwDesc = dwNm;
    }
    else
    {
        TCHAR szExt[MAX_PATH];  // "TXT"
        TCHAR szTemplate[128];   // "%s File"
        TCHAR szRet[MAX_PATH+20];        // "TXT File"
Error:
        bRC = FALSE;

        lstrcpy(pszId, pszExt);

        pszExt++;
        lstrcpy(szExt, pszExt);
        CharUpper(szExt);
        MLLoadString(IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
        wsprintf(szRet, szTemplate, szExt);
        lstrcpyn(pszDesc, szRet, (*pdwDesc) / SIZEOF(TCHAR));
        *pdwDesc = lstrlen(pszDesc) * SIZEOF(TCHAR);
    }
    return(bRC);
}





static VOID FTCmd_OnInitDialog(HWND hDialog, WPARAM wParam, LPARAM lParam)
{
        TCHAR szPath[MAX_PATH];
        TCHAR szAction[MAX_PATH];
        DWORD dwAction;
        DWORD dwPath;
        TCHAR szBuf[256];
        int iLen;
        LONG err;
        PFILETYPESDIALOGINFO pFTDInfo;

        TraceMsg(TF_FILETYPE, "FT Cmd: WM_INITDIALOG wParam=0x%x lParam=0x%x ", wParam, lParam);

        // The followings are controls that need platform
        // native character-set font set.
        SetDefaultDialogFont(hDialog, IDC_FT_CMD_ACTION);
        SetDefaultDialogFont(hDialog, IDC_FT_CMD_EXETEXT);
        SetDefaultDialogFont(hDialog, IDC_FT_CMD_DDEMSG);
        SetDefaultDialogFont(hDialog, IDC_FT_CMD_DDEAPP);
        SetDefaultDialogFont(hDialog, IDC_FT_CMD_DDEAPPNOT);
        SetDefaultDialogFont(hDialog, IDC_FT_CMD_DDETOPIC);

        pFTDInfo = (PFILETYPESDIALOGINFO)lParam;
        SetWindowLong(hDialog, DWL_USER, (LPARAM)pFTDInfo);
        pFTDInfo->hCmdDialog = hDialog;

        if(pFTDInfo->dwCommandEdit == IDC_FT_EDIT_EDIT)
        {
                // Set window title to show file type description we are editing
                if(MLLoadString(IDS_FT_EDITTITLE, szBuf, ARRAYSIZE(szBuf)))
                {
                        lstrcpy(szPath, szBuf);
                        lstrcat(szPath, pFTDInfo->pFTInfo->szDesc);
                        SetWindowText(hDialog, szPath);
                }

                // Set application field to executable used to perform action shown above
                dwPath = ARRAYSIZE(pFTDInfo->pFTCInfo->szCommand);
                VerbToExe(pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->pFTCInfo->szActionKey,
                        pFTDInfo->pFTCInfo->szCommand, &dwPath);

                // Remove %1 if at end of string
                lstrcpy(szBuf, c_szSpPercentOne);       // BUGBUG - StrCmpN modifies LPCSTR's even though that is how its params are declared
                iLen = lstrlen(c_szSpPercentOne);
                if(StrCmpN(&pFTDInfo->pFTCInfo->szCommand[lstrlen(pFTDInfo->pFTCInfo->szCommand)-iLen],
                                szBuf, iLen) == 0)
                        pFTDInfo->pFTCInfo->szCommand[lstrlen(pFTDInfo->pFTCInfo->szCommand)-iLen] = TEXT('\0');

                SetDlgItemText(hDialog, IDC_FT_CMD_EXE, pFTDInfo->pFTCInfo->szCommand);

                // Set command field to action verb keys value
                wsprintf(szPath, c_szTemplateSS, c_szShell, pFTDInfo->pFTCInfo->szActionKey);
                ASSERT(pFTDInfo->pFTInfo->hkeyFT);
                dwAction = SIZEOF(szAction);
                err = RegQueryValue(pFTDInfo->pFTInfo->hkeyFT, szPath, szAction, &dwAction);
                if(err == ERROR_SUCCESS && *szAction)
                        lstrcpy(pFTDInfo->pFTCInfo->szActionValue, szAction);
                else
                        lstrcpy(pFTDInfo->pFTCInfo->szActionValue, pFTDInfo->pFTCInfo->szActionKey);

                SetDlgItemText(hDialog, IDC_FT_CMD_ACTION, pFTDInfo->pFTCInfo->szActionValue);

                if(FindDDEOptions(((PFILETYPESDIALOGINFO)lParam)))
                {
                // Check the Use DDE checkbox
                        CheckDlgButton(hDialog, IDC_FT_CMD_USEDDE, TRUE);

                        // Set DDE field values
                        SetDlgItemText(hDialog, IDC_FT_CMD_DDEMSG,
                                pFTDInfo->pFTCInfo->szDDEMsg);
                        SetDlgItemText(hDialog, IDC_FT_CMD_DDEAPP,
                                pFTDInfo->pFTCInfo->szDDEApp);
                        SetDlgItemText(hDialog, IDC_FT_CMD_DDEAPPNOT,
                                pFTDInfo->pFTCInfo->szDDEAppNot);
                        SetDlgItemText(hDialog, IDC_FT_CMD_DDETOPIC,
                                pFTDInfo->pFTCInfo->szDDETopic);
                }

        }

        if(pFTDInfo->dwCommandEdit == IDC_FT_EDIT_NEW)  // enable all controls on New
        {
                pFTDInfo->pFTInfo->dwAttributes &= ((FTA_NoEditVerbCmd|FTA_NoEditVerbExe|FTA_NoDDE) ^ 0xffffffff);
        }

//      // Add items to action combo box
//      SendMessage(GetDlgItem(hDialog, IDC_FT_CMD_ACTION), CB_ADDSTRING, 0, (LPARAM)c_szOpenVerb);
//      SendMessage(GetDlgItem(hDialog, IDC_FT_CMD_ACTION), CB_ADDSTRING, 0, (LPARAM)c_szPrintVerb);

        // Don't allow actions to be edited if not new - bug#9553
        EnableWindow(GetDlgItem(hDialog, IDC_FT_CMD_ACTION),
                !(pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditVerbCmd)&&
                (pFTDInfo->dwCommandEdit != IDC_FT_EDIT_EDIT));

        EnableWindow(GetDlgItem(hDialog, IDC_FT_CMD_EXE),
                !((pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditVerbExe) &&
                  (!(pFTDInfo->pFTCInfo->dwVerbAttributes & FTAV_UserDefVerb))));
        EnableWindow(GetDlgItem(hDialog, IDC_FT_CMD_BROWSE),
                !((pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditVerbExe) &&
                  (!(pFTDInfo->pFTCInfo->dwVerbAttributes & FTAV_UserDefVerb))));
        ShowWindow(GetDlgItem(hDialog, IDC_FT_CMD_USEDDE),
                (((pFTDInfo->pFTInfo->dwAttributes & FTA_NoDDE) &&
                  (!(pFTDInfo->pFTCInfo->dwVerbAttributes & FTAV_UserDefVerb))) ?SW_HIDE :SW_SHOW));

        // Resize Dialog to see/hide DDE controls
        ResizeCommandDlg(hDialog, (pFTDInfo->pFTInfo->dwAttributes & FTA_NoDDE ?0 :IsDlgButtonChecked(hDialog, IDC_FT_CMD_USEDDE)));
}


static BOOL FTCmd_OnOK(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
        TCHAR szPath[MAX_PATH];
        TCHAR szKey[MAX_PATH];
        TCHAR szAction[MAX_PATH];

        // Validate fields
        if(!ActionIsEntered(hDialog, TRUE))
                return(FALSE);
        if(!ActionExeIsValid(hDialog, TRUE))
                return(FALSE);

        // Get and save edit command dialog text
        GetDlgItemText(hDialog, IDC_FT_CMD_ACTION, szAction, ARRAYSIZE(szAction));
        if(!(*szAction))  // Must have a value
                return(FALSE);

        if(pFTDInfo->dwCommandEdit == IDC_FT_EDIT_NEW)
        {
                if(!FTEdit_AddInfoToLV(pFTDInfo, NULL, szAction, pFTDInfo->szId, (HKEY)NULL))
                        return(FALSE);
                ListView_RedrawItems(pFTDInfo->hwndLVFTEdit, 0, ListView_GetItemCount(pFTDInfo->hwndLVFTEdit));
        }

        if(pFTDInfo->pFTCInfo)
        {
                lstrcpy(pFTDInfo->pFTCInfo->szActionValue, szAction);
                // Get executable field value for this verb
                lstrcpy(szPath, pFTDInfo->pFTCInfo->szCommand); // save prev val for check below
                GetDlgItemText(hDialog, IDC_FT_CMD_EXE,
                        pFTDInfo->pFTCInfo->szCommand, ARRAYSIZE(pFTDInfo->pFTCInfo->szCommand));

                // Add %1 to end if not already part of command
                lstrcpy(szAction, &c_szSpPercentOne[1]);   // borrow szAction; StrStr mods param 2
                if(StrStr(pFTDInfo->pFTCInfo->szCommand, szAction) == (LPTSTR)NULL)
                        lstrcat(pFTDInfo->pFTCInfo->szCommand, c_szSpPercentOne);

                // Get DDE field values
                if(IsDlgButtonChecked(hDialog, IDC_FT_CMD_USEDDE))
                {
                        GetDlgItemText(hDialog, IDC_FT_CMD_DDEMSG,
                                pFTDInfo->pFTCInfo->szDDEMsg, ARRAYSIZE(pFTDInfo->pFTCInfo->szDDEMsg));
                        GetDlgItemText(hDialog, IDC_FT_CMD_DDEAPP,
                                pFTDInfo->pFTCInfo->szDDEApp, ARRAYSIZE(pFTDInfo->pFTCInfo->szDDEApp));
                        GetDlgItemText(hDialog, IDC_FT_CMD_DDEAPPNOT,
                                pFTDInfo->pFTCInfo->szDDEAppNot, ARRAYSIZE(pFTDInfo->pFTCInfo->szDDEAppNot));
                        GetDlgItemText(hDialog, IDC_FT_CMD_DDETOPIC,
                                pFTDInfo->pFTCInfo->szDDETopic, ARRAYSIZE(pFTDInfo->pFTCInfo->szDDETopic));
                }
                else
                {
                        // HKEY_CLASSES_ROOT\filetype\shell\action key
                        wsprintf(szKey, c_szTemplateSSS, pFTDInfo->pFTCInfo->szId, c_szShell,
                                pFTDInfo->pFTCInfo->szActionKey);
                        DeleteDDEKeys(szKey);
                        *pFTDInfo->pFTCInfo->szDDEMsg = 0;
                        *pFTDInfo->pFTCInfo->szDDEApp = 0;
                        *pFTDInfo->pFTCInfo->szDDEAppNot = 0;
                        *pFTDInfo->pFTCInfo->szDDETopic = 0;
                }
                pFTDInfo->pFTCInfo->dwVerbAttributes = FTAV_UserDefVerb;
                SaveFileTypeData(FTD_COMMAND, pFTDInfo);

                // If exe has changed cause redraw of icon and exe name in prop sheet
                if(lstrcmpi(szPath, pFTDInfo->pFTCInfo->szCommand) != 0)
                {
                        HICON hIcon = NULL;


                        if(IsDefaultAction(pFTDInfo, pFTDInfo->pFTCInfo->szActionKey))
                        {
                                if(pFTDInfo->dwCommand == IDC_FT_PROP_EDIT)
                                {
                                        // Cause refind/redraw of Doc and Open icons in main dialog
                                        if(pFTDInfo->pFTInfo->hIconDoc != (HICON)NULL)
                                        {
                                                DestroyIcon(pFTDInfo->pFTInfo->hIconDoc);
                                                pFTDInfo->pFTInfo->hIconDoc = (HICON)NULL;
                                                SendMessage(pFTDInfo->hwndDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)0);
                                        }
                                        if(pFTDInfo->pFTInfo->hIconOpen != (HICON)NULL)
                                        {
                                                DestroyIcon(pFTDInfo->pFTInfo->hIconOpen);
                                                pFTDInfo->pFTInfo->hIconOpen = (HICON)NULL;
                                                SendMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)0);
                                        }
                                }

                                lstrcpy(szPath, pFTDInfo->pFTCInfo->szCommand);
                                PathRemoveArgs(szPath);
                                PathFindOnPath(szPath, NULL);

                                if((pFTDInfo->pFTInfo->dwAttributes & FTA_HasExtension) ||
                                   (pFTDInfo->dwCommand == IDC_FT_PROP_NEW))
                                {
                                        int iImageIndex;

                                        // get simulated doc icon
                                        iImageIndex = Shell_GetCachedImageIndex(szPath, 0, GIL_SIMULATEDOC);
                                        hIcon = ImageList_ExtractIcon(g_hinst, g_himlSysLarge, iImageIndex);
                                }
                                else
                                {
                                    // special cases like folder and drive
                                        if((hIcon = GetDefaultIcon(&pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->szId, SHGFI_LARGEICON)) == (HICON)NULL)
                                                // use default shell icon in case above calls fail to find an icon
                                                hIcon = ImageList_ExtractIcon(g_hinst, g_himlSysLarge, II_DOCNOASSOC);
                                }
                                SendMessage(pFTDInfo->hwndEditDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
                        }
                }
        }

        return(TRUE);
}


static VOID FTCmd_OnBrowse(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szTitle[MAX_PATH];

    // Warning can not pass MAKEINTRESOURCE strings to shell32 for GetFileNameFromBrowse
    // as it assuming shell32 for the resources...
    TCHAR szEXE[20];    // Make sure lots of room
    TCHAR szFilters[MAX_PATH];
    LPTSTR psz;

    szPath[0] = 0;

    MLLoadString(IDS_CAP_OPENAS, szTitle, ARRAYSIZE(szTitle));
    MLLoadString(IDS_EXE, szEXE, ARRAYSIZE(szEXE));

    // And we need to convert #'s to \0's...
    MLLoadString(IDS_PROGRAMSFILTER, szFilters, ARRAYSIZE(szFilters));
    psz = szFilters;
    while (*psz)
    {
        if (*psz == TEXT('#'))
        {
            LPTSTR pszT = psz;
            psz = CharNext(psz);
            *pszT = TEXT('\0');
        }
        else
            psz = CharNext(psz);
    }

    if (GetFileNameFromBrowse(hDialog, szPath, ARRAYSIZE(szPath), NULL, szEXE, szFilters, szTitle))
    {
        PathQuoteSpaces(szPath);
        SetDlgItemText(hDialog, IDC_FT_CMD_EXE, szPath);
    }
}


static VOID FTCmd_OnCommand(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
        TraceMsg(TF_FILETYPE, "FT Cmd: WM_COMMAND wParam 0x%04x 0x%04x", HIWORD(wParam), LOWORD(wParam));

        switch(LOWORD(wParam))
        {
                case IDOK:
                        if(!FTCmd_OnOK(pFTDInfo, hDialog, wParam, lParam))
                                break;
                        // Fall through...
                case IDCANCEL:
                        EndDialog(hDialog, (LOWORD(wParam) == IDOK));
                        break;

                case IDC_FT_CMD_BROWSE:
                        FTCmd_OnBrowse(pFTDInfo, hDialog);
                        break;

                case IDC_FT_CMD_USEDDE:
                        // Resize Dialog to see/hide DDE controls
                        ResizeCommandDlg(hDialog, IsDlgButtonChecked(hDialog, IDC_FT_CMD_USEDDE));
                        break;
        }
}

//================================================================
//================================================================
const static DWORD aEditCommandHelpIDs[] = {  // Context Help IDs
    IDC_FT_PROP_LV_FILETYPES,  IDH_FCAB_FT_PROP_LV_FILETYPES,
    IDC_FT_PROP_NEW,           IDH_FCAB_FT_PROP_NEW,
    IDC_FT_PROP_REMOVE,        IDH_FCAB_FT_PROP_REMOVE,
    IDC_FT_PROP_EDIT,          IDH_FCAB_FT_PROP_EDIT,
    IDC_FT_PROP_DOCICON,       IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_DOCEXTRO,      IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_OPENICON,      IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_OPENEXE,       IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_CMD_ACTION,         IDH_FCAB_FT_CMD_ACTION,
    IDC_FT_CMD_EXETEXT,        IDH_FCAB_FT_CMD_EXE,
    IDC_FT_CMD_EXE,            IDH_FCAB_FT_CMD_EXE,
    IDC_FT_CMD_BROWSE,         IDH_FCAB_FT_CMD_BROWSE,
    IDC_FT_CMD_DDEGROUP,       IDH_FCAB_FT_CMD_USEDDE,
    IDC_FT_CMD_USEDDE,         IDH_FCAB_FT_CMD_USEDDE,
    IDC_FT_CMD_DDEMSG,         IDH_FCAB_FT_CMD_DDEMSG,
    IDC_FT_CMD_DDEAPP,         IDH_FCAB_FT_CMD_DDEAPP,
    IDC_FT_CMD_DDEAPPNOT,      IDH_FCAB_FT_CMD_DDEAPPNOT,
    IDC_FT_CMD_DDETOPIC,       IDH_FCAB_FT_CMD_DDETOPIC,
    0, 0
};



BOOL CALLBACK FTCmd_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
        PFILETYPESDIALOGINFO pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hDialog, DWL_USER);

        INSTRUMENT_WNDPROC(SHCNFI_FTCmd_DLGPROC, hDialog, message, wParam, lParam);

        switch(message)
        {
                case WM_INITDIALOG:
                        FTCmd_OnInitDialog(hDialog, wParam, lParam);
                        return(TRUE);

                case WM_HELP:
                        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                                HELP_WM_HELP, (DWORD)(LPTSTR) aEditCommandHelpIDs);
                        return TRUE;

                case WM_CONTEXTMENU:
                        if ((int)SendMessage(hDialog, WM_NCHITTEST, 0, lParam) != HTCLIENT)
                            return FALSE;   // don't process it
                        SHWinHelpOnDemandWrap((HWND) wParam, NULL, HELP_CONTEXTMENU,
                                (DWORD)(LPVOID) aEditCommandHelpIDs);
                        return TRUE;

                case WM_COMMAND:
                        FTCmd_OnCommand(pFTDInfo, hDialog, wParam, lParam);
                        break;

                case WM_CTRL_SETFOCUS:
                        SetFocus((HWND)lParam);
                        SendMessage((HWND)lParam, EM_SETSEL, (WPARAM)0, (LPARAM)MAKELPARAM(0, -1));
                        break;

                case WM_DESTROY:
                        RemoveDefaultDialogFont(hDialog);
        }
        return(FALSE);
}





//================================================================
//================================================================
const static DWORD aEditFileTypesHelpIDs[] = {  // Context Help IDs
    IDC_NO_HELP_1,           NO_HELP,
    IDC_FT_EDIT_DOCICON,     IDH_FCAB_FT_EDIT_DOCICON,
    IDC_FT_EDIT_CHANGEICON,  IDH_FCAB_FT_EDIT_CHANGEICON,
    IDC_FT_EDIT_DESCTEXT,    IDH_FCAB_FT_EDIT_DESC,
    IDC_FT_EDIT_DESC,        IDH_FCAB_FT_EDIT_DESC,
    IDC_FT_EDIT_EXTTEXT,     IDH_FCAB_FT_EDIT_EXT,
    IDC_FT_EDIT_EXT,         IDH_FCAB_FT_EDIT_EXT,
    IDC_FT_COMBO_CONTTYPETEXT,  IDH_MIME_TYPE,
    IDC_FT_COMBO_CONTTYPE,      IDH_MIME_TYPE,
    IDC_FT_COMBO_DEFEXTTEXT,    IDH_DEFAULT_EXT,
    IDC_FT_COMBO_DEFEXT,        IDH_DEFAULT_EXT,
    IDC_FT_EDIT_CONFIRM_OPEN,   IDH_CONFIRM_OPEN,
    IDC_FT_EDIT_BROWSEINPLACE,  IDH_SAME_WINDOW,
    IDC_FT_EDIT_LV_CMDSTEXT, IDH_FCAB_FT_EDIT_LV_CMDS,
    IDC_FT_EDIT_LV_CMDS,     IDH_FCAB_FT_EDIT_LV_CMDS,
    IDC_FT_EDIT_DEFAULT,     IDH_FCAB_FT_EDIT_DEFAULT,
    IDC_FT_EDIT_NEW,         IDH_FCAB_FT_EDIT_NEW,
    IDC_FT_EDIT_EDIT,        IDH_FCAB_FT_EDIT_EDIT,
    IDC_FT_EDIT_REMOVE,      IDH_FCAB_FT_EDIT_REMOVE,
    IDC_FT_EDIT_QUICKVIEW,   IDH_FCAB_FT_EDIT_QUICKVIEW,
    IDC_FT_EDIT_SHOWEXT,     IDH_FCAB_FT_EDIT_SHOWEXT,
    0, 0
};



static BOOL FTEdit_ConfirmOpenAfterDownload(PFILETYPESDIALOGINFO pFTDInfo)
{
   return(! ClassIsSafeToOpen(pFTDInfo->pFTInfo->szId));
}


static BOOL FTEdit_SetConfirmOpenAfterDownload(PFILETYPESDIALOGINFO pFTDInfo,
                                        BOOL bConfirm)
{
   return(SetClassEditFlags(pFTDInfo->pFTInfo->szId, FTA_OpenIsSafe,
                            ! bConfirm));
}

// Defined originally in shdocvw
#define BROWSEHACK_DONTINPLACENAVIGATE     0x00000008


static void
FTEdit_InitBrowseInPlace(
    HWND hdlg,
    PFILETYPESDIALOGINFO pFTDInfo)
{
    HWND hwndCtl = GetDlgItem(hdlg, IDC_FT_EDIT_BROWSEINPLACE);
    TCHAR sz[MAX_PATH];
    HKEY hkey;

    wsprintf(sz, TEXT("%s\\DocObject"), pFTDInfo->pFTInfo->szId);

    // Does the class\DocObject key exist?
    if (NO_ERROR == RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, KEY_QUERY_VALUE, &hkey))
    {
        // Yes
        DWORD dwValue;
        DWORD cbSize;

        RegCloseKey(hkey);

        EnableWindow(hwndCtl, TRUE);

        cbSize = sizeof(dwValue);
        if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId,
                                   TEXT("BrowserFlags"), NULL, &dwValue,
                                   &cbSize))
        {
            Button_SetCheck(hwndCtl, IsFlagClear(dwValue, BROWSEHACK_DONTINPLACENAVIGATE));
        }
        else
        {
            Button_SetCheck(hwndCtl, TRUE);
        }
    }
}


static void
FTEdit_SetBrowseInPlace(
    PFILETYPESDIALOGINFO pFTDInfo,
    BOOL bSet)
{
    DWORD dwValue;
    DWORD cbSize;

    cbSize = sizeof(dwValue);
    if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId,
                               TEXT("BrowserFlags"), NULL, &dwValue,
                               &cbSize))
    {
        // We store the "boolean not" of bSet
        if (bSet)
            ClearFlag(dwValue, BROWSEHACK_DONTINPLACENAVIGATE);
        else
            SetFlag(dwValue, BROWSEHACK_DONTINPLACENAVIGATE);
    }
    else
    {
        dwValue = bSet ? 0 : BROWSEHACK_DONTINPLACENAVIGATE;
    }

    // If the value of BrowserFlags is 0, just delete the value altogether.
    if (0 == dwValue)
        SHDeleteValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId,
                      TEXT("BrowserFlags"));
    else
        SHSetValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId,
                   TEXT("BrowserFlags"), REG_DWORD, &dwValue, sizeof(dwValue));
}



static const TCHAR c_szFileViewer[] = TEXT("QuickView");
static const TCHAR c_szDefViewerKeyName[] = TEXT("QuickView\\*");
static const TCHAR c_szDefViewer[] = TEXT("*");


static BOOL FTEdit_ISViewable (PFILETYPESDIALOGINFO pFTDInfo)
{
    TCHAR szViewers[MAX_PATH+40];       // BUGBUG - Why 40? s/b SIZEOF(x)
    LONG cbValue;


    // First see if the type has a FileViews Sub key.
    cbValue = SIZEOF(szViewers);
    if(RegQueryValue(pFTDInfo->pFTInfo->hkeyFT, c_szFileViewer, szViewers,
            &cbValue) == ERROR_SUCCESS)
    {
        return(TRUE);

    }

    // Now see what extensions are associated with this one.
    if(pFTDInfo->pFTInfo->hDPAExt != (HDPA)NULL)
    {
        int iExt;
        int cExt;
        HKEY hkeyExt;


        cExt = DPA_GetPtrCount(pFTDInfo->pFTInfo->hDPAExt);
        for (iExt = 0; iExt < cExt; iExt++)
        {
            lstrcpy(szViewers, c_szFileViewer);
            lstrcat(szViewers, TEXT("\\"));
            lstrcat(szViewers, (LPTSTR)DPA_FastGetPtr(
                    pFTDInfo->pFTInfo->hDPAExt, iExt));

            if (RegOpenKey(HKEY_CLASSES_ROOT, szViewers, &hkeyExt)==ERROR_SUCCESS)
            {
                TraceMsg(TF_FILETYPE, "FTEDIT_IsViewable: by Ext %s", szViewers);

                RegCloseKey(hkeyExt);
                return(TRUE);
            }
        }
    }

    return(FALSE);
}


static void FTEdit_SetViewable (PFILETYPESDIALOGINFO pFTDInfo, BOOL fViewable)
{
    // First see if the type has a FileViews Sub key.
    if (fViewable)
    {
       RegSetValue(pFTDInfo->pFTInfo->hkeyFT, c_szFileViewer, REG_SZ,
               c_szDefViewer, SIZEOF(c_szDefViewer));
    }
    else
    {
        // Make sure that we don't have our new force item set.
        // Also go through the extensions and remove it if necessary.
        int iExt;
        int cExt;
        LONG lRet;
        TCHAR szViewers[MAX_PATH+40];   // BUGBUG - Why 40? s/b SIZEOF(x)

        RegDeleteKey (pFTDInfo->pFTInfo->hkeyFT, c_szFileViewer);

        cExt = DPA_GetPtrCount(pFTDInfo->pFTInfo->hDPAExt);
        for (iExt = 0; iExt < cExt; iExt++)
        {
            lstrcpy(szViewers, c_szFileViewer);
            lstrcat(szViewers, TEXT("\\"));
            lstrcat(szViewers, (LPTSTR)DPA_FastGetPtr(
                    pFTDInfo->pFTInfo->hDPAExt, iExt));

            lRet = SHDeleteKey(HKEY_CLASSES_ROOT, szViewers);

            TraceMsg(TF_FILETYPE, "FTEDIT_SetViewable: Delete key %s ret=%x",
                    szViewers, lRet);
        }
    }
}


static VOID FTEdit_EnableButtonsPerAction(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, int iItem)
{
        LV_ITEM LVItem;

        // Get FILETYPESINFO from LVItem's lParam
        LVItem.mask = LVIF_PARAM;
        LVItem.iItem = iItem;
        LVItem.iSubItem = 0;
        LVItem.lParam = 0;
        ListView_GetItem(pFTDInfo->hwndLVFTEdit, &LVItem);

        if(LVItem.lParam == 0)
        {
            // If this fails to get information, we will assume
            // that there are no commands, so disable Edit and remove...
            EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_EDIT),FALSE);
            EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_REMOVE), FALSE);
            EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT), FALSE);
            return;
        }

        pFTDInfo->pFTCInfo = (PFILETYPESCOMMANDINFO)LVItem.lParam;

        pFTDInfo->pFTCInfo->dwVerbAttributes = GetVerbAttributes(pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->pFTCInfo->szActionKey);
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_EDIT),
                !((pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditVerb) &&
                (!(pFTDInfo->pFTCInfo->dwVerbAttributes & FTAV_UserDefVerb))));
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_REMOVE),
                !((pFTDInfo->pFTInfo->dwAttributes & FTA_NoRemoveVerb) &&
                (!(pFTDInfo->pFTCInfo->dwVerbAttributes & FTAV_UserDefVerb))));
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT),
                !((pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditDflt)));
}


static BOOL FTEdit_IsExtShowable(PFILETYPESDIALOGINFO pFTDInfo)
{
    // First see if the type has a FileViews Sub key.

    return (NO_ERROR == RegQueryValueEx(pFTDInfo->pFTInfo->hkeyFT,
                                        (LPTSTR)c_szShowExt, NULL, NULL,
                                        NULL, NULL));
}


static void FTEdit_SetShowExt(PFILETYPESDIALOGINFO pFTDInfo, BOOL bShowExt)
{
        if (bShowExt)
                RegSetValueEx(pFTDInfo->pFTInfo->hkeyFT, (LPTSTR)c_szShowExt, 0, REG_SZ, (LPBYTE)c_szNULL, 0);
        else
                RegDeleteValue(pFTDInfo->pFTInfo->hkeyFT, (LPTSTR)c_szShowExt);
}


static BOOL FTEdit_AreDefaultViewersInstalled()
{
    TCHAR szValue[MAX_PATH];
    HKEY hkey;
    BOOL fRet = FALSE;

    if (RegOpenKey(HKEY_CLASSES_ROOT, c_szDefViewerKeyName, &hkey) == ERROR_SUCCESS)
    {
        // (Yes, the fourth parameter really is "count of characters")
        if (RegEnumKey(hkey, 0, szValue, SIZECHARS(szValue)) == ERROR_SUCCESS)
            fRet = TRUE;
        RegCloseKey(hkey);
    }

     return fRet;
}


static BOOL FTEdit_InitListViewCols(HWND hwndLV)
{
    LV_COLUMN col;
    RECT rc;

    SetWindowLong(hwndLV, GWL_EXSTYLE,
            GetWindowLong(hwndLV, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

    // Insert one column
    GetClientRect(hwndLV, &rc);
    ZeroMemory(&col, SIZEOF(LV_COLUMN));
    col.mask = LVCF_FMT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = rc.right - GetSystemMetrics(SM_CXBORDER);
    if(ListView_InsertColumn(hwndLV, 0, &col) == (-1))
        return(FALSE);

    return(TRUE);
}


static BOOL FTEdit_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, LPTSTR szActionKey,
        LPTSTR szActionValue, LPTSTR szId, HKEY hk)
{
        BOOL bRC = FALSE;
        int iIndex = 0;
        LV_ITEM LVItem;

        if((pFTDInfo->pFTCInfo = LocalAlloc(LPTR, SIZEOF(FILETYPESCOMMANDINFO))) != NULL)
        {
                lstrcpy(pFTDInfo->pFTCInfo->szId, szId);

                if (szActionKey)
                {
                    lstrcpy(pFTDInfo->pFTCInfo->szActionKey, szActionKey);
                    lstrcpy(pFTDInfo->pFTCInfo->szActionValue, szActionKey);

                    if (hk != NULL)
                    {
                        DWORD dwSize;
                        TCHAR szTemp[MAX_PATH];

                        // See if there is nice text for the action...
                        dwSize = SIZEOF(szTemp);
                        if ((RegQueryValue(hk, szActionKey, szTemp, &dwSize) == ERROR_SUCCESS)
                                && (dwSize > SIZEOF(TCHAR)))
                        {
                            lstrcpy(pFTDInfo->pFTCInfo->szActionValue, szTemp);
                        }
                    }
                }

                else
                {
                    // Special case if user typed in something like:
                    // print=My Print to take the print off to be
                    // its own special char...
                    LPTSTR pszT = StrChr(szActionValue, TEXT('='));

                    if (pszT)
                    {
                        *pszT++ = TEXT('\0');
                        StrRemoveChar(szActionValue, pFTDInfo->pFTCInfo->szActionKey, TEXT('&'));
                        lstrcpy(szActionValue, pszT);
                    }
                    else
                    {
                        // We want to remove the & of the command as well as convert blanks into _s
                        // as default command processing has problems with processing of blanks
                        StrRemoveChar(szActionValue, pFTDInfo->pFTCInfo->szActionKey, TEXT('&'));
                        for (pszT = pFTDInfo->pFTCInfo->szActionKey; *pszT; pszT = CharNext(pszT))
                        {
                            if (*pszT == TEXT(' '))
                                *pszT = TEXT('_');
                        }
                    }
                    lstrcpy(pFTDInfo->pFTCInfo->szActionValue, szActionValue);


                }


                LVItem.mask        = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                LVItem.iItem       = iIndex++;
                LVItem.iSubItem    = 0;
                LVItem.pszText     = pFTDInfo->pFTCInfo->szActionValue;
                LVItem.lParam      = (LPARAM)pFTDInfo->pFTCInfo;

                if(ListView_InsertItem(pFTDInfo->hwndLVFTEdit, &LVItem) != (-1))
                {
                        // Enable the remove button
                        EnableWindow(GetDlgItem(pFTDInfo->hEditDialog, IDC_FT_EDIT_REMOVE), TRUE);
                        bRC = TRUE;
                }
        }
        return(bRC);
}


static int FTEdit_InitListView(PFILETYPESDIALOGINFO pFTDInfo)
{
    TCHAR szClass[MAX_PATH];
    TCHAR szAction[MAX_PATH];
    DWORD dwClass;
    DWORD dwAction;
    int iSubKey;
    FILETIME ftLastWrite;
    HKEY hk;

    // See if we have a default action verb
    iSubKey = SIZEOF(pFTDInfo->pFTInfo->szDefaultAction);
    DefaultAction(pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->pFTInfo->szDefaultAction, &iSubKey);

    // Enumerate action verbs
    iSubKey = 0;
    if(RegOpenKeyEx(pFTDInfo->pFTInfo->hkeyFT, c_szShell, (DWORD)NULL, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS)
    {
        dwClass = ARRAYSIZE(szClass);
        dwAction = ARRAYSIZE(szAction);
        // add verbs to list view
        while(RegEnumKeyEx(hk, iSubKey, szAction, &dwAction, NULL, szClass, &dwClass, &ftLastWrite) == ERROR_SUCCESS)
        {
            if(!FTEdit_AddInfoToLV(pFTDInfo, szAction, NULL, pFTDInfo->pFTInfo->szId, hk))
            {
                iSubKey = (-1);
                break;
            }

            dwClass = ARRAYSIZE(szClass);
            dwAction = ARRAYSIZE(szAction);
            iSubKey++;
        }
        RegCloseKey(hk);
    }

    return(iSubKey);
}


static BOOL FTEdit_OnInitDialog(HWND hDialog, WPARAM wParam, LPARAM lParam)
{
        DWORD dwItemCnt;
        LOGFONT lf;
        PFILETYPESDIALOGINFO pFTDInfo;

        TraceMsg(TF_FILETYPE, "FT Edit: WM_INITDIALOG wParam=0x%x lParam=0x%x ", wParam, lParam);

        SetDefaultDialogFont(hDialog, IDC_FT_EDIT_DESC);
        SetDefaultDialogFont(hDialog, IDC_FT_EDIT_LV_CMDS);

        pFTDInfo = (PFILETYPESDIALOGINFO)lParam;
        SetWindowLong(hDialog, DWL_USER, (LPARAM)pFTDInfo);
        pFTDInfo->hEditDialog = hDialog;
        pFTDInfo->pFTCInfo = (PFILETYPESCOMMANDINFO)NULL;
        pFTDInfo->szIconPath[0] = 0;

        pFTDInfo->hwndLVFTEdit = GetDlgItem(hDialog, IDC_FT_EDIT_LV_CMDS);

        switch (pFTDInfo->dwCommand)
        {
        case IDC_FT_PROP_EDIT:
                // these guys are already hidden
                // IDC_FT_EDIT_EXTTEXT, IDC_FT_EDIT_EXT

                // Display DOC Icon
                pFTDInfo->hwndEditDocIcon = GetDlgItem(hDialog, IDC_FT_EDIT_DOCICON);
                SendMessage(pFTDInfo->hwndEditDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pFTDInfo->pFTInfo->hIconDoc);

                // Set edit control with file type description
                SetDlgItemText(hDialog, IDC_FT_EDIT_DESC, pFTDInfo->pFTInfo->szDesc);

                // Init and fill list view with action verbs
                if(pFTDInfo->hwndLVFTEdit != (HWND)NULL)
                {
                        if(FTEdit_InitListViewCols(pFTDInfo->hwndLVFTEdit))
                        {
                                if((dwItemCnt = FTEdit_InitListView((PFILETYPESDIALOGINFO)lParam)) == (-1))
                                {
                                        return(FALSE);
                                }
                        }
                        else
                                return(FALSE);
                }

                // Set listview to item 0
                ListView_SetItemState(pFTDInfo->hwndLVFTEdit, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

                lstrcpy(pFTDInfo->szId, pFTDInfo->pFTInfo->szId);  // used for when we are adding a verb to an existing filetype
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_NEW), !(pFTDInfo->pFTInfo->dwAttributes & FTA_NoNewVerb));
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DESC), !(pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditDesc));
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_CHANGEICON), !(pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditIcon));

                FTEdit_EnableButtonsPerAction(pFTDInfo, hDialog, 0);

                if (pFTDInfo->pFTInfo->hkeyFT) {        // Get rid of old handle
                    RegCloseKey(pFTDInfo->pFTInfo->hkeyFT);
                }

                pFTDInfo->pFTInfo->hkeyFT = GetHkeyFT(pFTDInfo->szId);

                CheckDlgButton(hDialog, IDC_FT_EDIT_CONFIRM_OPEN, FTEdit_ConfirmOpenAfterDownload(pFTDInfo));

                FTEdit_InitBrowseInPlace(hDialog, pFTDInfo);

                if (FTEdit_AreDefaultViewersInstalled())
                {
                    if (FTEdit_ISViewable(pFTDInfo))
                            CheckDlgButton(hDialog, IDC_FT_EDIT_QUICKVIEW, TRUE);
                }
                else
                    EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_QUICKVIEW), FALSE);

                if (FTEdit_IsExtShowable(pFTDInfo))
                        CheckDlgButton(hDialog, IDC_FT_EDIT_SHOWEXT, TRUE);
            break;

        case IDC_FT_PROP_NEW:
            {
                TCHAR szTitle[256];
                if(!FTEdit_InitListViewCols(pFTDInfo->hwndLVFTEdit))
                        return(FALSE);

                MLLoadString(IDS_ADDNEWFILETYPE, szTitle, ARRAYSIZE(szTitle));
                SetWindowText(hDialog, szTitle);

                // Make extension text and edit control visible
                pFTDInfo->hwndDocExt = GetDlgItem(hDialog, IDC_FT_EDIT_EXTTEXT);
                ShowWindow(pFTDInfo->hwndDocExt, SW_SHOW);
                pFTDInfo->hwndDocExt = GetDlgItem(hDialog, IDC_FT_EDIT_EXT);
                ShowWindow(pFTDInfo->hwndDocExt, SW_SHOW);
                SetFocus(pFTDInfo->hwndDocExt);
                *pFTDInfo->szId = TEXT('\0');
                *pFTDInfo->szIconPath = TEXT('\0');


                pFTDInfo->hwndEditDocIcon = GetDlgItem(hDialog, IDC_FT_EDIT_DOCICON);

                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_EDIT), FALSE);
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_REMOVE), FALSE);
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT), FALSE);
                CheckDlgButton(hDialog, IDC_FT_EDIT_CONFIRM_OPEN, FTEdit_ConfirmOpenAfterDownload(pFTDInfo));
            }
            break;
        }

        SystemParametersInfo(SPI_GETICONTITLELOGFONT, SIZEOF(lf), &lf, FALSE);
        pFTDInfo->hfReg = CreateFontIndirect(&lf);
        lf.lfWeight = FW_BOLD;
        pFTDInfo->hfBold = CreateFontIndirect(&lf);
        pFTDInfo->pFTInfo->dwMIMEFlags = 0;
        pFTDInfo->hwndContentTypeComboBox = GetDlgItem(hDialog, IDC_FT_COMBO_CONTTYPE);
        pFTDInfo->hwndDefExtensionComboBox = GetDlgItem(hDialog, IDC_FT_COMBO_DEFEXT);
        InitMIMEControls(hDialog);
        return(TRUE);   // Successful initdialog
}


#define lpdis ((LPDRAWITEMSTRUCT)lParam)

static BOOL FTEdit_OnDrawItem(PFILETYPESDIALOGINFO pFTDInfo, WPARAM wParam, LPARAM lParam)
{
        LV_ITEM LVItem;
        TCHAR szActionValue[MAX_PATH];

        TraceMsg(TF_FILETYPE, "FT Edit: WM_DRAWITEM wParam=0x%x lParam=0x%x", wParam, lParam);

        if (lpdis->CtlType == ODT_LISTVIEW)
        {
                DRAWITEMSTRUCT *lpdi = (LPDRAWITEMSTRUCT)lParam;
                PFILETYPESCOMMANDINFO pFTCInfo;

                LVItem.mask = LVIF_PARAM;
                LVItem.iItem = lpdi->itemID;
                LVItem.iSubItem = 0;
                ListView_GetItem(pFTDInfo->hwndLVFTEdit, &LVItem);  // lParam points to file type info                                  pFTInfo = (PFILETYPESINFO)LVItem.lParam;
                pFTCInfo = (PFILETYPESCOMMANDINFO)LVItem.lParam;

                if((pFTDInfo->hwndLVFTEdit == GetFocus()) && ((lpdi->itemState & ODS_FOCUS) && (lpdi->itemState & ODS_SELECTED)))
                {
                        SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
                        SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                }
                else
                {
                        SetBkColor(lpdi->hDC, GetSysColor(COLOR_WINDOW));
                        SetTextColor(lpdi->hDC, GetSysColor(COLOR_WINDOWTEXT));
                }

                // Use Bold font for default action
                if(IsDefaultAction(pFTDInfo, pFTCInfo->szActionKey))
                {
                        SelectObject(lpdi->hDC, pFTDInfo->hfBold);
//                      ChangeDefaultButtonText(pFTDInfo->hEditDialog, IDS_CLEARDEFAULT);
                }
                else
                {
                        SelectObject(lpdi->hDC, pFTDInfo->hfReg);
//                      ChangeDefaultButtonText(pFTDInfo->hEditDialog, IDS_SETDEFAULT);
                }

                StrRemoveChar(pFTCInfo->szActionValue, szActionValue, TEXT('&'));
                ExtTextOut(lpdi->hDC,
                        lpdi->rcItem.left,lpdi->rcItem.top,
                        ETO_OPAQUE, &lpdi->rcItem,
                        szActionValue, lstrlen(szActionValue),
                        NULL);

                if((pFTDInfo->hwndLVFTEdit == GetFocus()) && ((lpdi->itemState & ODS_FOCUS) && (lpdi->itemState & ODS_SELECTED)))
                        DrawFocusRect(lpdi->hDC, &lpdi->rcItem);

                return(TRUE);
        }
        return(FALSE);
}


#define lpmis ((LPMEASUREITEMSTRUCT)lParam)

static BOOL FTEdit_OnMeasureItem(WPARAM wParam, LPARAM lParam)
{
        LOGFONT lf;

        TraceMsg(TF_FILETYPE, "FT Edit: WM_MEASUREITEM wParam=0x%x lParam=0x%x", wParam, lParam);

        if (lpmis->CtlType == ODT_LISTVIEW)
        {
                SystemParametersInfo(SPI_GETICONTITLELOGFONT, SIZEOF(lf), &lf, FALSE);
//              hfReg = CreateFontIndirect(&lf);
//              lf.lfWeight = FW_BOLD;
//              hfBold = CreateFontIndirect(&lf);
                ((MEASUREITEMSTRUCT *)lParam)->itemHeight = lf.lfHeight;
                ((MEASUREITEMSTRUCT *)lParam)->itemHeight = 14;
                return(TRUE);
        }
        return(FALSE);
}


static VOID FTEdit_OnNotify(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
        TraceMsg(TF_FILETYPE, "FT Edit: WM_NOTIFY wParam=0x%x lParam=0x%x", wParam, lParam);

        // Process ListView notifications
        switch(((LV_DISPINFO *)lParam)->hdr.code)
        {
                case NM_DBLCLK:
                        TraceMsg(TF_FILETYPE, "FT Edit: WM_NOTIFY - NM_DBLCLK");

                        if(ListView_GetItemCount(pFTDInfo->hwndLVFTEdit))
                        {
                                if(!(pFTDInfo->pFTInfo->dwAttributes & FTA_NoEditVerb))
                                        PostMessage(hDialog, WM_COMMAND, (WPARAM)IDC_FT_EDIT_EDIT, 0);
                        }
                        break;

                case NM_SETFOCUS:
                case NM_KILLFOCUS:
                        // update list view
                        ListView_RedrawItems(pFTDInfo->hwndLVFTEdit, 0, ListView_GetItemCount(pFTDInfo->hwndLVFTEdit));
                        UpdateWindow(pFTDInfo->hwndLVFTEdit);
                        break;

                case LVN_ITEMCHANGED:
                        TraceMsg(TF_FILETYPE, "FT Edit: WM_NOTIFY - LVN_ITEMCHANGED");

                        if((((NM_LISTVIEW *)lParam)->uChanged & LVIF_STATE) &&
                        ((NM_LISTVIEW *)lParam)->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))
                        {
                            FTEdit_EnableButtonsPerAction(pFTDInfo, hDialog,
                           pFTDInfo->iEditItem = ((NM_LISTVIEW *)lParam)->iItem);
                        }
                        break;
                case LVN_DELETEITEM:
                        // We were notified that an item was deleted.
                        // so delete the underlying data that it is pointing
                        // to.
                        if (((NM_LISTVIEW*)lParam)->lParam)
                                LocalFree((HANDLE)((NM_LISTVIEW*)lParam)->lParam);
                        break;

        }       // switch(((LV_DISPINFO *)lParam)->hdr.code)
}


static BOOL IsIconPerInstance(HKEY hkeyFT)
{
        LONG err;
        TCHAR szDefaultIcon[MAX_PATH];
        BOOL bRC = FALSE;

        ASSERT(hkeyFT != NULL);

        err = SIZEOF(szDefaultIcon);
        err = RegQueryValue(hkeyFT, c_szDefaultIcon, szDefaultIcon, &err);
        if (err == ERROR_SUCCESS && *szDefaultIcon)
        {
                int cchT = lstrlen(c_szSpPercentOne) - 1;
                if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        szDefaultIcon, cchT, &c_szSpPercentOne[1], cchT) == 2)
                        bRC = TRUE;
        }
        return(bRC);
}


static BOOL HasIconHandler(HKEY hkeyFT)
{
        TCHAR szBuf[MAX_PATH];
        DWORD dwBuf;

        ASSERT(hkeyFT != NULL);

        // Don't allow icon to be changed if type has an icon handler
        dwBuf = SIZEOF(szBuf);
        return(RegQueryValue(hkeyFT, c_szShellexIconHandler, szBuf, &dwBuf) == ERROR_SUCCESS);
}


static BOOL FT_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, HKEY hkeyFT, LPTSTR szExt, LPTSTR szDesc, LPTSTR szId, DWORD dwAttributes)
{
        BOOL bRC = FALSE;
        LV_ITEM LVItem;
        LPTSTR pszExt;

        ASSERT(hkeyFT != (HKEY)NULL);

        if((pFTDInfo->pFTInfo = LocalAlloc(LPTR, SIZEOF(FILETYPESINFO))) != NULL)
        {
                if((pFTDInfo->pFTInfo->hDPAExt = DPA_Create(4)) != (HDPA)NULL)  // create dynamic pointer array for FILETYPESINFO dpaExt member
                {
                        if((pszExt = LocalAlloc(LPTR, (lstrlen(szExt)+1)*SIZEOF(TCHAR))) != NULL)
                        {
                                lstrcpy(pszExt, szExt);
                                if(DPA_AppendPtr(pFTDInfo->pFTInfo->hDPAExt, (LPVOID)pszExt) == 0)
                                {
                                        lstrcpy(pFTDInfo->pFTInfo->szDesc, szDesc);
                                        lstrcpy(pFTDInfo->pFTInfo->szId, szId);
                                        pFTDInfo->pFTInfo->dwAttributes = dwAttributes;
                                        if(HasIconHandler(hkeyFT) || IsIconPerInstance(hkeyFT))
                                                pFTDInfo->pFTInfo->dwAttributes |= FTA_NoEditIcon;
                                        pFTDInfo->pFTInfo->hkeyFT = hkeyFT;

                                        LVItem.mask        = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                                        LVItem.iItem       = INT_MAX;
                                        LVItem.iSubItem    = 0;
                                        LVItem.pszText     = szDesc;
                                        LVItem.iImage      = I_IMAGECALLBACK;
                                        LVItem.lParam      = (LPARAM)pFTDInfo->pFTInfo;

                                        if ((pFTDInfo->iItem = ListView_InsertItem(pFTDInfo->hwndLVFT, &LVItem)) != (-1))
                                                bRC = TRUE;
                                }
                        }
                }
        }
        return(bRC);
}


static BOOL FTEdit_OnOK(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
        LV_ITEM LVItem;
        TCHAR szExt[MAX_PATH];
        TCHAR szDesc[MAX_PATH];

        if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
        {
                GetDlgItemText(hDialog, IDC_FT_EDIT_EXT, szExt, ARRAYSIZE(szExt));

                // We need to do some cleanup here to make it work properly
                // in the cases where ther user types in something like
                // *.foo or .foo
                // This is real crude
                StrRemoveChar(szExt, NULL, TEXT('*'));
        }
        else
                lstrcpy(szExt, DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0));

        // Validate file type description
        GetDlgItemText(hDialog, IDC_FT_EDIT_DESC, szDesc, ARRAYSIZE(szDesc));
        if(!(*szDesc))
        {
                lstrcpy(szDesc, CharUpper((szExt[0] == TEXT('.') ? &szExt[1] : szExt)));
                lstrcat(szDesc, c_szSpaceFile);
        }

        // Save extension when new type is selected
        if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
        {
                if(!ValidExtension(hDialog, pFTDInfo))
                        return(FALSE);
                AddExtDot(CharLower(szExt), ARRAYSIZE(szExt));
                if(pFTDInfo->hwndLVFT != (HWND)NULL)
                {
                        HKEY hkeyFT = GetHkeyFT(pFTDInfo->szId);
                        FT_AddInfoToLV(pFTDInfo, hkeyFT, szExt, szDesc, pFTDInfo->szId, 0);
                        pFTDInfo->pFTInfo->dwAttributes = FTA_HasExtension;
                }
                SaveFileTypeData(FTD_EXT, pFTDInfo);
        }

        lstrcpy(pFTDInfo->pFTInfo->szDesc, szDesc);
        SetDlgItemText(hDialog, IDC_FT_EDIT_DESC, szDesc);

        // Save file type id, description, and default action
        SaveFileTypeData(FTD_EDIT, pFTDInfo);

        // Save Doc icon if a change was made
        if(*pFTDInfo->szIconPath)
        {
                SaveFileTypeData(FTD_DOCICON, pFTDInfo);

                // Get the image index from the list view item
                LVItem.mask        = LVIF_IMAGE;
                LVItem.iItem       = pFTDInfo->iItem;
                LVItem.iSubItem    = 0;
                ListView_GetItem(pFTDInfo->hwndLVFT, &LVItem);

                // replace the icon in the image list
                if(pFTDInfo->himlFT && (LVItem.iImage >= 0) && pFTDInfo->pFTInfo->hIconDoc)
                        if(ImageList_ReplaceIcon(pFTDInfo->himlFT, LVItem.iImage, pFTDInfo->pFTInfo->hIconDoc) != (-1))
                                ListView_SetItem(pFTDInfo->hwndLVFT, &LVItem);
        }
        if(pFTDInfo->dwCommand == IDC_FT_PROP_EDIT)
        {
                // Tell prev dialog to update new values
                LVItem.mask       = LVIF_TEXT;
                LVItem.iItem      = pFTDInfo->iItem;
                LVItem.iSubItem   = 0;
                LVItem.pszText    = pFTDInfo->pFTInfo->szDesc;
                ListView_SetItem(pFTDInfo->hwndLVFT, &LVItem);
        }

        FTEdit_SetConfirmOpenAfterDownload(pFTDInfo, IsDlgButtonChecked(hDialog, IDC_FT_EDIT_CONFIRM_OPEN));

        if (IsWindowEnabled(GetDlgItem(hDialog, IDC_FT_EDIT_BROWSEINPLACE)))
            FTEdit_SetBrowseInPlace(pFTDInfo, IsDlgButtonChecked(hDialog, IDC_FT_EDIT_BROWSEINPLACE));

        if (FTEdit_AreDefaultViewersInstalled())
            FTEdit_SetViewable(pFTDInfo, IsDlgButtonChecked(hDialog,
                    IDC_FT_EDIT_QUICKVIEW));

        FTEdit_SetShowExt(pFTDInfo, IsDlgButtonChecked(hDialog,
                        IDC_FT_EDIT_SHOWEXT));

        if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
        {
                if(pFTDInfo->hwndLVFT != (HWND)NULL)
                {
                        int iItem;
                        LV_FINDINFO LV_FindInfo;

                        ListView_SetItemState(pFTDInfo->hwndLVFT, pFTDInfo->iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        ListView_SortItems(pFTDInfo->hwndLVFT, NULL, 0);
                        LV_FindInfo.flags = LVFI_PARAM;
                        LV_FindInfo.lParam = (LPARAM)pFTDInfo->pFTInfo;
                        if((iItem = ListView_FindItem(pFTDInfo->hwndLVFT, -1, &LV_FindInfo)) != -1)
                                pFTDInfo->iItem = iItem;
                        else
                                pFTDInfo->iItem = 0;
                        ListView_EnsureVisible(pFTDInfo->hwndLVFT, pFTDInfo->iItem, FALSE);
                        PostMessage(pFTDInfo->hwndLVFT, WM_SETFOCUS, (WPARAM)0, (LPARAM)0);
                }
        }

    SaveFileTypeData(FTD_MIME, pFTDInfo);

    // This may be overkill but for now, have it refresh the
    // windows...
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

        return(TRUE);
}


static VOID FTEdit_OnRemove(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
        // HKEY_CLASSES_ROOT\filetype\shell\action key
        if(ShellMessageBox(MLGetHinst(), hDialog, MAKEINTRESOURCEA(IDS_FT_MB_REMOVEACTION), MAKEINTRESOURCEA(IDS_FT), MB_YESNO | MB_ICONQUESTION) == IDNO)
                return;

        ASSERT(pFTDInfo->pFTInfo->hkeyFT);
        RemoveAction(pFTDInfo, pFTDInfo->pFTInfo->hkeyFT, c_szShell,
                pFTDInfo->pFTCInfo->szActionKey);

        if(ListView_GetItemCount(pFTDInfo->hwndLVFTEdit) == 0)
        {
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_EDIT), FALSE);
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_REMOVE), FALSE);
                EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT), FALSE);
        }

        OkToClose_NoCancel(g_hinst, hDialog);
        PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
}


static VOID FTEdit_OnChangeIcon(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
        SHFILEINFO sfi;
        HICON hIcon;
        TCHAR szBuf[MAX_PATH];

        if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
        {
                lstrcpy(sfi.szDisplayName, c_szShell2);
                sfi.iIcon = -(IDI_SYSFILE);
        }
        else
        {
                LPTSTR pszExt = (LPTSTR)DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0);

                if(*pszExt)
                {
                        // Get (1)default cmd (2)open cmd (3)first cmd for PickIcon
                        ExtToShellCommand(pFTDInfo->pFTInfo->hkeyFT, sfi.szDisplayName, ARRAYSIZE(sfi.szDisplayName));
                }
                else    // see if we have a DefaultIcon key
                {
                        LONG err;

                        err = SIZEOF(sfi.szDisplayName);
                        err = RegQueryValue(pFTDInfo->pFTInfo->hkeyFT, c_szDefaultIcon, sfi.szDisplayName, &err);
                        if (err == ERROR_SUCCESS && *sfi.szDisplayName)
                        {
                                sfi.iIcon = PathParseIconLocation(sfi.szDisplayName);
                                PathRemoveArgs(sfi.szDisplayName);
                        }
                        else
                                *(sfi.szDisplayName) = TEXT('\0');

                        TraceMsg(TF_FILETYPE, "FTEdit_OnChangeIcon RegQueryValue szIcon=%s iIndex=%d hkeyFT=0x%x",
                                sfi.szDisplayName, sfi.iIcon, pFTDInfo->pFTInfo->hkeyFT);
                }
        }

        if(*sfi.szDisplayName)
        {
            // Fix up the name we have so that we can display the PickIcon dlg, this includes
            // resolve the relative item, and striping arguments.  Should this fail then we
            // strip the string of arguments and pass it in, letting PickIcon do its worst!

            if ( PathProcessCommand( sfi.szDisplayName, sfi.szDisplayName, SIZEOF(sfi.szDisplayName), PPCF_NODIRECTORIES ) == -1 )
            {
                PathRemoveArgs( sfi.szDisplayName );
                PathRemoveBlanks( sfi.szDisplayName );
                PathUnquoteSpaces( sfi.szDisplayName );
            }

            if(lstrcmp(sfi.szDisplayName, c_szExefileOpenCommand) == 0)
                *sfi.szDisplayName = TEXT('\0');
            else
            {
                if(PathIsRelative(sfi.szDisplayName))
                    PathFindOnPath(sfi.szDisplayName, NULL);        // search for exe
            }
        }

        lstrcpy(szBuf, sfi.szDisplayName);
        if(PickIconDlg(hDialog, szBuf, ARRAYSIZE(szBuf), &sfi.iIcon))
        {
                lstrcpy(pFTDInfo->szIconPath, szBuf);
                pFTDInfo->iIconIndex = sfi.iIcon;
                hIcon = ExtractIcon(g_hinst, pFTDInfo->szIconPath, pFTDInfo->iIconIndex);
                if(hIcon != (HICON)NULL)
                {
                        if((pFTDInfo->hwndEditDocIcon = GetDlgItem(hDialog, IDC_FT_EDIT_DOCICON)) != (HWND)NULL)
                                SendMessage(pFTDInfo->hwndEditDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
                        if(pFTDInfo->pFTInfo->hIconDoc != (HICON)NULL)
                                DestroyIcon(pFTDInfo->pFTInfo->hIconDoc);
                        pFTDInfo->pFTInfo->hIconDoc = hIcon;
                }

                // OK -> Close and Disable Cancel
                if(pFTDInfo->dwCommand == IDC_FT_PROP_EDIT)
                {
                        OkToClose_NoCancel(g_hinst, hDialog);
                        PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
                }
        }
}


static VOID FTEdit_OnCommand(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    TraceMsg(TF_FILETYPE, "FT Edit: WM_COMMAND wParam hi=0x%04x lo=0x%04x lParam=0x%x", HIWORD(wParam), LOWORD(wParam), lParam);

    switch(idCmd)
    {
    case IDOK:
        if(!FTEdit_OnOK(pFTDInfo, hDialog))
            break;
        // Fall through...

    case IDCANCEL:
        DeleteObject(pFTDInfo->hfReg);
        DeleteObject(pFTDInfo->hfBold);
        EndDialog(hDialog, (idCmd == IDOK));
        break;

    case IDC_FT_EDIT_NEW:
    {
        int iPrevEditItem = pFTDInfo->iEditItem;

        if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
        {
            if(!ValidExtension(hDialog, pFTDInfo))
                break;
        }
        pFTDInfo->dwCommandEdit = idCmd;
        pFTDInfo->iEditItem = ListView_GetItemCount(pFTDInfo->hwndLVFTEdit) + 1;
        if(DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(DLG_FILETYPEOPTIONSCMD), hDialog, FTCmd_DlgProc, (LPARAM)pFTDInfo))
        {
            OkToClose_NoCancel(g_hinst, hDialog);
            PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
        }
        else
            pFTDInfo->iEditItem = iPrevEditItem;

        // We need to again see if the commands for edit
        // and remove should be enabled...
        //
        FTEdit_EnableButtonsPerAction(pFTDInfo, hDialog, pFTDInfo->iEditItem);

        break;
    }

    case IDC_FT_EDIT_EDIT:
        pFTDInfo->dwCommandEdit = idCmd;
        if(DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(DLG_FILETYPEOPTIONSCMD), hDialog, FTCmd_DlgProc, (LPARAM)pFTDInfo))
        {
            OkToClose_NoCancel(g_hinst, hDialog);
            PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
        }
        break;

    case IDC_FT_EDIT_REMOVE:
        FTEdit_OnRemove(pFTDInfo, hDialog);
        break;

    case IDC_FT_EDIT_CHANGEICON:
        FTEdit_OnChangeIcon(pFTDInfo, hDialog);
        break;

    case IDC_FT_EDIT_DEFAULT:
        SetDefaultAction(pFTDInfo);     // set default action for current item

        // update list view
        ListView_RedrawItems(pFTDInfo->hwndLVFTEdit, 0, ListView_GetItemCount(pFTDInfo->hwndLVFTEdit));
        UpdateWindow(pFTDInfo->hwndLVFTEdit);

        OkToClose_NoCancel(g_hinst, hDialog);
        PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
        break;

    case IDC_FT_COMBO_CONTTYPE:
        switch (GET_WM_COMMAND_CMD(wParam, lParam))
        {
           case CBN_SELCHANGE:
              TraceMsg(TF_FILETYPE, "FTEdit_OnCommand(): MIME Type selection change.");
              OnContentTypeSelectionChange(hDialog);
              break;
           case CBN_EDITCHANGE:
              TraceMsg(TF_FILETYPE, "FTEdit_OnCommand(): MIME Type edit change.");
              OnContentTypeEditChange(hDialog);
              break;
           case CBN_DROPDOWN:
              TraceMsg(TF_FILETYPE, "FTEdit_OnCommand(): MIME Type drop down.");
              OnContentTypeDropDown(hDialog);
              break;
        }
        break;

    case IDC_FT_COMBO_DEFEXT:
        switch (GET_WM_COMMAND_CMD(wParam, lParam))
        {
        case CBN_DROPDOWN:
            TraceMsg(TF_FILETYPE, "FTEdit_OnCommand(): Default Ext drop down.");
            OnDefExtensionDropDown(hDialog);
            break;
        }
        break;

    case IDC_FT_EDIT_EXT:
        switch (GET_WM_COMMAND_CMD(wParam, lParam))
        {
        case EN_CHANGE:
            TraceMsg(TF_FILETYPE, "FTEdit_OnCommand(): Associated Ext change.");
            OnContentTypeEditChange(hDialog);
            break;
        }
        break;
    }
}


BOOL CALLBACK FTEdit_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    PFILETYPESDIALOGINFO pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hDialog, DWL_USER);

    INSTRUMENT_WNDPROC(SHCNFI_FTEdit_DLGPROC, hDialog, message, wParam, lParam);

    switch(message)
    {
        case WM_INITDIALOG:
            if(!FTEdit_OnInitDialog(hDialog, wParam, lParam))
            {
                EndDialog(hDialog, FALSE);
                break;
            }
            else
                return(TRUE);

        case WM_DRAWITEM:
            FTEdit_OnDrawItem(pFTDInfo, wParam, lParam);
            break;

        case WM_MEASUREITEM:
            return(FTEdit_OnMeasureItem(wParam, lParam));

        case WM_NOTIFY:
            FTEdit_OnNotify(pFTDInfo, hDialog, wParam, lParam);
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle,
                    FT_GetHelpFileFromControl((HWND)(((LPHELPINFO)lParam)->hItemHandle)),
                    HELP_WM_HELP, (DWORD)(LPTSTR)aEditFileTypesHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            {
            POINT pt;

            if ((int)SendMessage(hDialog, WM_NCHITTEST, 0, lParam) != HTCLIENT)
               return FALSE;   // don't process it

            LPARAM_TO_POINT(lParam, pt);
            EVAL(ScreenToClient(hDialog, &pt));

            SHWinHelpOnDemandWrap((HWND)wParam,
                    FT_GetHelpFileFromControl(ChildWindowFromPoint(hDialog, pt)),
                    HELP_CONTEXTMENU, (DWORD)(LPVOID)aEditFileTypesHelpIDs);

            return TRUE;
            }

        case WM_COMMAND:
            FTEdit_OnCommand(pFTDInfo, hDialog, wParam, lParam);
            break;

        case WM_CTRL_SETFOCUS:
            SetFocus((HWND)lParam);
            SendMessage((HWND)lParam, EM_SETSEL, (WPARAM)0, (LPARAM)MAKELPARAM(0, -1));
            break;

        case WM_DESTROY:
            RemoveDefaultDialogFont(hDialog);
            break;
    }
    return(FALSE);
}




static void CALLBACK FTListViewEnumItems(PFILETYPESDIALOGINFO pFTDInfo, int i, int iEnd, HANDLE *pfShouldLive)
{
    LV_ITEM item;

    TraceMsg(TF_FILETYPE, "FTListViewThread created.");

    item.iSubItem = 0;
    item.mask = LVIF_IMAGE;         // This should be the only slow part

    if (iEnd == -1 ) {
        iEnd = ListView_GetItemCount(pFTDInfo->hwndLVFT);
    }

    for (; (!pfShouldLive || *pfShouldLive) && i < iEnd; i++)
    {
        item.iItem = i;
        ListView_GetItem(pFTDInfo->hwndLVFT, &item);
    }
}


static DWORD CALLBACK FTListViewThread(PFILETYPESDIALOGINFO pFTDInfo)
{
    HANDLE hThread = pFTDInfo->hThread;
    FTListViewEnumItems(pFTDInfo, 10, -1, &pFTDInfo->hThread);
    CloseHandle(hThread);
    pFTDInfo->hThread = 0;
    return 0;
}


static VOID CreateListViewThread(PFILETYPESDIALOGINFO pFTDInfo)
{
    // Create background thread to force list view to draw items
    DWORD idThread;

    if (pFTDInfo->hThread)
        return;

    pFTDInfo->hThread = CreateThread(NULL, 0, FTListViewThread, pFTDInfo, 0, &idThread);
    if(pFTDInfo->hThread)
            SetThreadPriority(pFTDInfo->hThread, THREAD_PRIORITY_BELOW_NORMAL);
}


static BOOL FT_InitListViewCols(HWND hwndLV)
{
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT rc;

    SetWindowLong(hwndLV, GWL_EXSTYLE,
            GetWindowLong(hwndLV, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

    // Insert one column
    GetClientRect(hwndLV, &rc);
    col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
            - 2 * GetSystemMetrics(SM_CXEDGE);
    if(ListView_InsertColumn(hwndLV, 0, &col) == (-1))
        return(FALSE);

    return(TRUE);
}


static BOOL FT_InitListView(PFILETYPESDIALOGINFO pFTDInfo)
{
    DWORD dwSubKey = 0;
    TCHAR szDesc[MAX_PATH];
    TCHAR szClass[MAX_PATH];
    TCHAR szClassesKey[MAX_PATH];   // string containing the classes key
    TCHAR szId[MAX_PATH];
    TCHAR szShellCommandValue[MAX_PATH];
    DWORD dwName;
    DWORD dwClass;
    DWORD dwId;
    DWORD dwClassesKey;
    DWORD dwAttributes;
    FILETIME ftLastWrite;
    BOOL bRC = TRUE;
    BOOL bRC1;
    LONG err;
    HKEY hkeyFT = NULL;
    UINT flags = ILC_MASK;
    if (IS_WINDOW_RTL_MIRRORED(pFTDInfo->hwndLVFT))
    {
        flags |= ILC_MIRROR;
    }
    if((pFTDInfo->himlFT = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON), flags, 0, 8)) == (HIMAGELIST)NULL)
            return(FALSE);
    ListView_SetImageList(pFTDInfo->hwndLVFT, pFTDInfo->himlFT, LVSIL_SMALL);

    // Enumerate extensions from registry to get file types
    dwClassesKey = ARRAYSIZE(szClassesKey);
    dwClass = ARRAYSIZE(szClass);
    while(RegEnumKeyEx(HKEY_CLASSES_ROOT, dwSubKey, szClassesKey, &dwClassesKey, NULL, szClass, &dwClass, &ftLastWrite) != ERROR_NO_MORE_ITEMS)
    {
        *szId = TEXT('\0');
        dwAttributes = 0;

        if(*szClassesKey == TEXT('.'))  // find the file type identifier and description from the extension
        {
            dwName = SIZEOF(szDesc);
            dwId = SIZEOF(szId);
            bRC1 = ExtToTypeNameAndId(szClassesKey, szDesc, &dwName, szId, &dwId);

            if(RegOpenKey(HKEY_CLASSES_ROOT, szId, &hkeyFT) != ERROR_SUCCESS)
                hkeyFT = NULL;
            else
            {
                dwAttributes = GetFileTypeAttributes(hkeyFT);
                if(!(dwAttributes & FTA_Exclude))
                    dwAttributes |= FTA_HasExtension;

                if(!bRC1)
                {
                    // see if there is a HKEY_CLASSES_ROOT\[.Ext]\Shell\Open\Command value
                    err = SIZEOF(szShellCommandValue);
                    err = RegQueryValue(hkeyFT, c_szShellOpenCommand, szShellCommandValue, &err);
                    if (err != ERROR_SUCCESS || !(*szShellCommandValue))
                    {
                        dwAttributes = FTA_Exclude;
                        RegCloseKey(hkeyFT);
                        hkeyFT = NULL;
                    }
                    else
                    {
                        dwAttributes |= FTA_ExtShellOpen;
                    }
                }
            }
        }
        else
        {
            if(RegOpenKey(HKEY_CLASSES_ROOT, szClassesKey, &hkeyFT) != ERROR_SUCCESS)
                    hkeyFT = NULL;
            if((dwAttributes = GetFileTypeAttributes(hkeyFT)) & FTA_Show)
            {
                lstrcpy(szId, szClassesKey);
                dwName = SIZEOF(szDesc);
                err = RegQueryValue(hkeyFT, NULL, szDesc, &dwName);
                if(err != ERROR_SUCCESS || !*szDesc)
                    lstrcpy(szDesc, szClassesKey);
                *szClassesKey = TEXT('\0');
            }
        }

        TraceMsg(TF_FILETYPE, "FT RegEnum HKCR szClassKey=%s szId=%s dwAttributes=%d",       szClassesKey, szId, dwAttributes);

        if((!(dwAttributes & FTA_Exclude)) && ((dwAttributes & FTA_Show) || (dwAttributes & FTA_HasExtension) || (dwAttributes & FTA_ExtShellOpen)))
        {
            if(!FT_AddInfoToLV(pFTDInfo, hkeyFT, szClassesKey, szDesc, szId, dwAttributes))
            {
                RegCloseKey(hkeyFT);
                bRC = FALSE;
                break;
            }
        }
        else
            RegCloseKey(hkeyFT);

        dwSubKey++;
        dwClassesKey = ARRAYSIZE(szClassesKey);
        dwClass = ARRAYSIZE(szClass);
    }

    ListView_SortItems(pFTDInfo->hwndLVFT, NULL, 0);
    FT_MergeDuplicates(pFTDInfo->hwndLVFT);

    FTListViewEnumItems(pFTDInfo, 0, 10, NULL);
    CreateListViewThread(pFTDInfo);

    return(bRC);
}


static BOOL FT_OnInitDialog(HWND hDialog, LPARAM lParam)
{
        PFILETYPESDIALOGINFO pFTDInfo;
        BOOL bRC = FALSE;
        DECLAREWAITCURSOR;

        TraceMsg(TF_FILETYPE, "FT: WM_INITDIALOG");

        SetWaitCursor();

        SetDefaultDialogFont(hDialog, IDC_FT_PROP_LV_FILETYPES);
        SetDefaultDialogFont(hDialog, IDC_FT_PROP_OPENEXE);

        pFTDInfo = (PFILETYPESDIALOGINFO)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLong(hDialog, DWL_USER, (LPARAM)pFTDInfo);
        pFTDInfo->hPropDialog = hDialog;
        pFTDInfo->pFTInfo = (PFILETYPESINFO)NULL;

        if((pFTDInfo->hwndLVFT = GetDlgItem(hDialog, IDC_FT_PROP_LV_FILETYPES)) != (HWND)NULL)
        {
                SendMessage(pFTDInfo->hwndLVFT, WM_SETREDRAW, FALSE, 0);
                pFTDInfo->hwndDocIcon = GetDlgItem(hDialog, IDC_FT_PROP_DOCICON);
                pFTDInfo->hwndOpenIcon = GetDlgItem(hDialog, IDC_FT_PROP_OPENICON);
                if(FT_InitListViewCols(pFTDInfo->hwndLVFT))
                {
                        if(FT_InitListView(pFTDInfo))
                        {       // macro needs brackets
                                ListView_SetItemState(pFTDInfo->hwndLVFT, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED); // Set listview to item 0
                                bRC = TRUE;
                        }
                }
                SendMessage(pFTDInfo->hwndLVFT, WM_SETREDRAW, TRUE, 0);
        }

        ResetWaitCursor();

        return(bRC);
}


static VOID FT_OnLVN_ItemChanged(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, LPARAM lParam)
{
        LV_ITEM LVItem;

        if((((NM_LISTVIEW *)lParam)->uChanged & LVIF_STATE) &&
                ((NM_LISTVIEW *)lParam)->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))
        {
                TraceMsg(TF_FILETYPE, "FT: WM_NOTIFY - LVN_ITEMCHANGED");

                // Get FILETYPESINFO from LVItem's lParam
                LVItem.mask = LVIF_PARAM;
                LVItem.iItem = pFTDInfo->iItem = ((NM_LISTVIEW *)lParam)->iItem;
                LVItem.iSubItem = 0;
                ListView_GetItem(pFTDInfo->hwndLVFT, &LVItem);  // lParam points to file type info
                pFTDInfo->pFTInfo = (PFILETYPESINFO)LVItem.lParam;

                DisplayDocObjects(pFTDInfo, hDialog);

                DisplayOpensWithObjects(pFTDInfo, hDialog);

                EnableWindow(GetDlgItem(hDialog, IDC_FT_PROP_EDIT), !(pFTDInfo->pFTInfo->dwAttributes & FTA_NoEdit));
                EnableWindow(GetDlgItem(hDialog, IDC_FT_PROP_REMOVE), !(pFTDInfo->pFTInfo->dwAttributes & FTA_NoRemove));
        }
}


static VOID FT_OnLVN_GetDispInfo(PFILETYPESDIALOGINFO pFTDInfo, LPARAM lParam)
{
        LV_ITEM LVItem;
        int iImageIndex;
        HICON hIcon;
        SHFILEINFO sfi;
        LV_DISPINFO *pnmv;

        TraceMsg(TF_FILETYPE, "FT: WM_NOTIFY - LVN_GETDISPINFO");

        pnmv = (LV_DISPINFO *)lParam;

        if(pnmv->item.mask & LVIF_IMAGE)
        {
                if(((PFILETYPESINFO)(pnmv->item.lParam))->dwAttributes & FTA_HasExtension)
                {
                        if(SHGetFileInfo(DPA_FastGetPtr(((PFILETYPESINFO)(pnmv->item.lParam))->hDPAExt,0),
                                        FILE_ATTRIBUTE_NORMAL,
                                        &sfi, SIZEOF(SHFILEINFO),
                                        SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES))
                                hIcon = sfi.hIcon;
                        else
                                hIcon = NULL;

                        TraceMsg(TF_FILETYPE, "LVN_GETDISPINFO SHGetFileInfo szFile=%s hIcon=0x%x",
                                DPA_FastGetPtr(((PFILETYPESINFO)(pnmv->item.lParam))->hDPAExt,0), hIcon);
                }
                else
                        hIcon = GetDefaultIcon(&(((PFILETYPESINFO)(pnmv->item.lParam))->hkeyFT), ((PFILETYPESINFO)(pnmv->item.lParam))->szId, SHGFI_SMALLICON);

                if(hIcon == (HICON)NULL)  // use default shell icon in case above calls fail to find an icon
                        hIcon = ImageList_ExtractIcon(g_hinst, g_himlSysSmall, II_DOCNOASSOC);

                if(hIcon != (HICON)NULL)
                {
                        if((iImageIndex = ImageList_AddIcon(pFTDInfo->himlFT, hIcon)) != (-1))
                        {
                                ZeroMemory(&LVItem, SIZEOF(LV_ITEM));
                                LVItem.mask        = LVIF_IMAGE;
                                LVItem.iItem       = pnmv->item.iItem;
                                LVItem.iImage      = iImageIndex;
                                ListView_SetItem(pFTDInfo->hwndLVFT, &LVItem);

                                pnmv->item.iImage = iImageIndex;

                        }
                        DestroyIcon(hIcon);
                }
        }

}


static BOOL FT_OnNotify(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, LPARAM lParam)
{
    LPNMHDR pnm = (NMHDR*)lParam;

    TraceMsg(TF_FILETYPE, "FT: WM_NOTIFY code=0x%x =%d", ((NMHDR *) lParam)->code, ((NMHDR *) lParam)->code);

    // Process ListView notifications
    if (IDC_FT_PROP_LV_FILETYPES == pnm->idFrom) {
        PFILETYPESINFO pFTInfo;

        switch(((LV_DISPINFO *)lParam)->hdr.code)
        {
        case NM_DBLCLK:
            if(!(pFTDInfo->pFTInfo->dwAttributes & FTA_NoEdit))
                PostMessage(hDialog, WM_COMMAND, (WPARAM)IDC_FT_PROP_EDIT, 0);
            break;

        case LVN_ITEMCHANGED:
            FT_OnLVN_ItemChanged(pFTDInfo, hDialog, lParam);
            break;

        case LVN_GETDISPINFO:
            FT_OnLVN_GetDispInfo(pFTDInfo, lParam);
            break;

        case LVN_DELETEITEM:
            pFTInfo = (PFILETYPESINFO)(((NM_LISTVIEW*)lParam)->lParam);

            if (pFTInfo) {
                FT_CleanupOne(pFTDInfo, pFTInfo);
            }
            break;
        }
    }
    return(FALSE);
}


static VOID FT_OnCommand(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
        UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

        TraceMsg(TF_FILETYPE, "FT: WM_COMMAND");

        switch(idCmd)
        {
                case IDC_FT_PROP_NEW:
                case IDC_FT_PROP_EDIT:
                        pFTDInfo->dwCommand = idCmd;
                        DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(DLG_FILETYPEOPTIONSEDIT), hDialog, FTEdit_DlgProc, (LPARAM)pFTDInfo);
                        DisplayOpensWithObjects(pFTDInfo, hDialog);
                        DisplayDocObjects(pFTDInfo, hDialog);
                        break;

                case IDC_FT_PROP_REMOVE:
                        // Tell user that this extension is already in use
                        if(ShellMessageBox(MLGetHinst(), hDialog, MAKEINTRESOURCEA(IDS_FT_MB_REMOVETYPE), MAKEINTRESOURCEA(IDS_FT), MB_YESNO | MB_ICONQUESTION) == IDYES) {
                            RemoveFileType(pFTDInfo);
                            PropSheet_CancelToClose(GetParent(hDialog));
                        }
                        break;
        }
}



//================================================================
//================================================================
const static DWORD aFileTypeOptionsHelpIDs[] = {  // Context Help IDs
    IDC_GROUPBOX,                 IDH_COMM_GROUPBOX,
    IDC_FT_PROP_LV_FILETYPES,     IDH_FCAB_FT_PROP_LV_FILETYPES,
    IDC_FT_PROP_NEW,              IDH_FCAB_FT_PROP_NEW,
    IDC_FT_PROP_REMOVE,           IDH_FCAB_FT_PROP_REMOVE,
    IDC_FT_PROP_EDIT,             IDH_FCAB_FT_PROP_EDIT,
    IDC_FT_PROP_EDIT,             IDH_FCAB_FT_PROP_EDIT,
    IDC_FT_PROP_DOCICON,          IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_DOCEXTRO_TXT,     IDH_EXTENSION,
    IDC_FT_PROP_DOCEXTRO,         IDH_EXTENSION,
    IDC_FT_PROP_CONTTYPERO_TXT,   IDH_MIME_TYPE,
    IDC_FT_PROP_CONTTYPERO,       IDH_MIME_TYPE,
    IDC_FT_PROP_OPENICON,         IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_OPENEXE_TXT,      IDH_OPENS_WITH,
    IDC_FT_PROP_OPENEXE,          IDH_OPENS_WITH,
    0, 0
};

#define c_szMIMEHelpFile        TEXT("Update.hlp")


//================================================================
//================================================================


/*
** FT_GetHelpFileFromControl()
**
** Determines whether to use the MIME help file or the default Win95 help file
** for context-sensitive help for a given control.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
/*
 * HACKHACK: We depend upon the group box in the File Types property sheet
 * occurring after the controls inside it in the parent dialog's child window
 * order.  This ordering is set up under Win95 by declaring the group box
 * after the other controls in the dialog's resource definition.
 */
static LPCTSTR FT_GetHelpFileFromControl(HWND hwndControl)
{
   LPCTSTR pcszHelpFile = NULL;
   int nControlID = 0;

   ASSERT(! hwndControl ||
          IS_VALID_HANDLE(hwndControl, WND));

   if (hwndControl)
   {
      nControlID = GetDlgCtrlID(hwndControl);

      switch (nControlID)
      {
         case IDC_FT_PROP_DOCICON:
         case IDC_FT_PROP_DOCEXTRO_TXT:
         case IDC_FT_PROP_DOCEXTRO:

         case IDC_FT_PROP_CONTTYPERO_TXT:
         case IDC_FT_PROP_CONTTYPERO:

         case IDC_FT_PROP_OPENICON:
         case IDC_FT_PROP_OPENEXE_TXT:
         case IDC_FT_PROP_OPENEXE:

         case IDC_FT_COMBO_CONTTYPETEXT:
         case IDC_FT_COMBO_CONTTYPE:

         case IDC_FT_COMBO_DEFEXTTEXT:
         case IDC_FT_COMBO_DEFEXT:

         case IDC_FT_EDIT_CONFIRM_OPEN:
         case IDC_FT_EDIT_BROWSEINPLACE:

            /* MIME help comes from the MIME help file. */
            pcszHelpFile = c_szMIMEHelpFile;
            break;

         default:
            /* Other help is taken from the default Win95 help file. */
            break;
      }
   }

   TraceMsg(TF_FILETYPE, "FT_GetHelpFileFromControl(): Using %s for control %d (HWND %#lx).",
              pcszHelpFile ? pcszHelpFile : TEXT("default Win95 help file"),
              nControlID,
              hwndControl);

   ASSERT(! pcszHelpFile ||
          IS_VALID_STRING_PTR(pcszHelpFile, -1));

   return(pcszHelpFile);
}


BOOL CALLBACK FT_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
        PFILETYPESDIALOGINFO pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLong(hDialog, DWL_USER);

        TraceMsg(TF_FILETYPE, "FileTypesDialogProc wParam=0x%x lParam=0x%x", wParam, lParam);

        INSTRUMENT_WNDPROC(SHCNFI_FT_DLGPROC, hDialog, message, wParam, lParam);

        switch(message)
        {
                case WM_INITDIALOG:
                        DllAddRef();
                        return(FT_OnInitDialog(hDialog, lParam));


                case WM_NOTIFY:
                        return(FT_OnNotify(pFTDInfo, hDialog, lParam));

                case WM_DESTROY:
                    RemoveDefaultDialogFont(hDialog);
                    if (pFTDInfo->hThread) {
                        HANDLE hThread = pFTDInfo->hThread;
                        pFTDInfo->hThread = 0;          // signal thread that we are done if still running
                        if(WaitForSingleObject(hThread, 2000) == WAIT_TIMEOUT)
                            TerminateThread(hThread, 0);
                    }
                break;

                case WM_COMMAND:
                        FT_OnCommand(pFTDInfo, hDialog, wParam, lParam);
                        break;

                case WM_HELP:
                    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle,
                            FT_GetHelpFileFromControl((HWND)(((LPHELPINFO)lParam)->hItemHandle)),
                            HELP_WM_HELP, (DWORD)(LPTSTR)aFileTypeOptionsHelpIDs);
                    return TRUE;

                case WM_CONTEXTMENU:
                    {
                        POINT pt;

                        if ((int)SendMessage(hDialog, WM_NCHITTEST, 0, lParam) != HTCLIENT)
                            return FALSE;   // don't process it

                        LPARAM_TO_POINT(lParam, pt);
                        EVAL(ScreenToClient(hDialog, &pt));

                        SHWinHelpOnDemandWrap((HWND)wParam,
                               FT_GetHelpFileFromControl(ChildWindowFromPoint(hDialog, pt)),
                               HELP_CONTEXTMENU, (DWORD)(LPVOID)aFileTypeOptionsHelpIDs);
                        return TRUE;
                    }

                case WM_NCDESTROY:
                    DllRelease();
                    break;

        }

        return(FALSE);
}


/*----------------------------------------------------------
Purpose: Callback for the File Type property page.

Returns:
Cond:    --
*/
UINT
CALLBACK
FileType_Callback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
    {
    UINT uResult = TRUE;
    PFILETYPESDIALOGINFO pinfo = (PFILETYPESDIALOGINFO)ppsp->lParam;

    // uMsg may be any value.

    ASSERT(! hwnd ||
           IS_VALID_HANDLE(hwnd, WND));

    switch (uMsg)
        {
    case PSPCB_CREATE:
        break;

    case PSPCB_RELEASE:
        TraceMsg(TF_FILETYPE, "FileType_Callback received PSPCB_RELEASE.");

        if (pinfo)
            {
            LocalFree(pinfo);
            }
        break;

    default:
        break;
        }

    return uResult;
    }


/*----------------------------------------------------------
Purpose: Create the File Type property page and return a handle
         to the page.

         The instance data related to this page is freed in the
         page's callback, as specified

Returns: NO_ERROR if the page is created

Cond:    --
*/
HRESULT
CreateFileTypePage(
    OUT HPROPSHEETPAGE * phpsp,
    IN  LPVOID           pvReserved)    // Must be NULL
    {
    HRESULT hres;

    ASSERT(phpsp);
    ASSERT( !pvReserved );      // right now this must be NULL

    if ( !phpsp || pvReserved )
        {
        hres = E_INVALIDARG;
        }
    else
        {
        PFILETYPESDIALOGINFO pinfo = NULL;
        PROPSHEETPAGE psp;

        *phpsp = NULL;

        Shell_GetImageLists(&g_himlSysLarge, &g_himlSysSmall);

        psp.dwSize      = SIZEOF(psp);
        psp.dwFlags     = PSP_USECALLBACK;
        psp.hInstance   = MLGetHinst();
        psp.pfnCallback = FileType_Callback;
        psp.pszTemplate = MAKEINTRESOURCE(DLG_FILETYPEOPTIONS);
        psp.pfnDlgProc  = FT_DlgProc;

        pinfo = LocalAlloc(LPTR, SIZEOF(*pinfo));

        if ( !pinfo )
            {
            hres = E_OUTOFMEMORY;
            }
        else
            {
            psp.lParam = (LPARAM)pinfo;

            *phpsp = CreatePropertySheetPage(&psp);

            if (*phpsp)
                {
                hres = NO_ERROR;
                }
            else
                {
                hres = E_OUTOFMEMORY;
                LocalFree(pinfo);
                }
            }
        }

    return hres;
    }


#ifndef UNICODE
// This file is compiled twice (filetypa.c and filetypw.c).  But this
// code below should be instantiated only once.

//========================================================================
// CFileTypes Class definition
//========================================================================

typedef struct _CFileTypes
    {
    IShellPropSheetExt  spse;
    UINT                cRef;
    } CFileTypes;


/*----------------------------------------------------------
Purpose: QueryInterface method

Returns:
Cond:    --
*/
STDMETHODIMP
CFileTypes_QueryInterface(
    IN  LPSHELLPROPSHEETEXT pspse,
    IN  REFIID              riid,
    OUT LPVOID FAR *        ppvObj)
{
    HRESULT hres;
    CFileTypes * this = IToClass(CFileTypes, spse, pspse);

    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));

    if (IsEqualIID(riid, &IID_IShellPropSheetExt) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = pspse;
        this->cRef++;

        hres = NOERROR;
    }
    else
    {
        *ppvObj = NULL;
        hres = E_NOINTERFACE;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: AddRef method

Returns:
Cond:    --
*/
STDMETHODIMP_(ULONG)
CFileTypes_AddRef(
    IN LPSHELLPROPSHEETEXT pspse)
{
    CFileTypes * this = IToClass(CFileTypes, spse, pspse);

    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));

    this->cRef++;
    return this->cRef;
}


/*----------------------------------------------------------
Purpose: Release method

Returns:
Cond:    --
*/
STDMETHODIMP_(ULONG)
CFileTypes_Release(
    IN LPSHELLPROPSHEETEXT pspse)
{
    CFileTypes * this = IToClass(CFileTypes, spse, pspse);

    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));

    this->cRef--;
    if (this->cRef > 0)
    {
        return this->cRef;
    }

    LocalFree((HLOCAL)this);
    DllRelease();
    return 0;
}


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::AddPages for CFileTypes

         This is called for full-shell installations, via CLSID
         {B091E540-83E3-11CF-A713-0020AFD79762}.

*/
STDMETHODIMP
CFileTypes_AddPages(
    IN LPSHELLPROPSHEETEXT  pspse,
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam)
{
    HRESULT hres;
    CFileTypes * this = IToClass(CFileTypes, spse, pspse);
    HPROPSHEETPAGE hpsp;

    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));

    // Make sure the FileIconTable is init properly.  If brought in
    // by inetcpl we need to set this true...
    FileIconInit(TRUE);

    // We need to run the unicode version on NT, to avoid all bugs
    // that occur with the ANSI version (due to unicode-to-ansi 
    // conversions of file names).

    if (g_fRunningOnNT)
        hres = CreateFileTypePageW(&hpsp, NULL);
    else
        hres = CreateFileTypePageA(&hpsp, NULL);

    if (SUCCEEDED(hres) && !pfnAddPage(hpsp, lParam) )
    {
        hres = E_FAIL;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::ReplacePage for CFileTypes

         This is called on browser-only installations, via 
         the FileTypes Hook.  Our hook CLSID is 
         {FBF23B41-E3F0-101B-8488-00AA003E56F8}.

*/
STDMETHODIMP
CFileTypes_ReplacePage(
    IN LPSHELLPROPSHEETEXT  pspse,
    IN UINT                 uPageID,
    IN LPFNADDPROPSHEETPAGE pfnReplaceWith,
    IN LPARAM               lParam)
{
    HRESULT hres = E_NOTIMPL;
    CFileTypes * this = IToClass(CFileTypes, spse, pspse);

    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));

    if (EXPPS_FILETYPES == uPageID)
    {
        HPROPSHEETPAGE hpsp;

        // We need to run the unicode version on NT, to avoid all bugs
        // that occur with the ANSI version (due to unicode-to-ansi 
        // conversions of file names).

        if (g_fRunningOnNT)
            hres = CreateFileTypePageW(&hpsp, NULL);
        else
            hres = CreateFileTypePageA(&hpsp, NULL);

        if (SUCCEEDED(hres) && !pfnReplaceWith(hpsp, lParam) )
        {
            hres = E_FAIL;
        }
    }

    return hres;
}


// VTable
const IShellPropSheetExtVtbl c_CFileTypesVtbl =
    {
    CFileTypes_QueryInterface, CFileTypes_AddRef, CFileTypes_Release,
    CFileTypes_AddPages,
    CFileTypes_ReplacePage,
    };



/*----------------------------------------------------------
Purpose: Create a FileTypes object that provides an IShellPropSheet

         The file types object implements the file types page.
         This page is accessed in a couple different ways, depending
         on the platform and version.

         Win95/NT4  
         ---------

         The shell will bind to the CLSID registered in "HKLM\Software\
         Microsoft\CurrentVersion\Explorer\FileTypesPropertySheetHook", 
         if it exists.  This object replaces whatever old implementation
         the shell has.  This was added originally to Win95 close to 
         shipping so IE 1.0 could override it with a MIME-enabled dialog.

         URL.DLL implemented the MIME-enabled dialog originally 
         for IE 1.0, using CLSID {FBF23B41-E3F0-101B-8488-00AA003E56F8}
         (the "FileTypes Hook" CLSID).

         The NT shell followed in the footsteps of the original Nashville 
         shell code, which made the shell-implemented page accessible via 
         the CLSID {B091E540-83E3-11CF-A713-0020AFD79762}, (the 
         "FileTypes" CLSID) and included the MIME-enabled changes that 
         were in URL.DLL.  This allowed us to move the File Types 
         dialog from explorer.exe to shell32, and each view is 
         responsible to add it to the View/Options page (notice file-
         oriented folders include it, while the Control Panel does not
         now).

         Since URL.DLL is an ansi DLL, and there are bugs wrt ansi-unicode 
         conversion in it, the IE 3.0 version (when running on NT) of 
         URL.DLL's FileTypes Hook CLSID simply bound to the FileTypes 
         CLSID implemented in shell32.  On Win95, URL.DLL used its 
         own implementaion.

         IE4
         ---------

         The filetype dialog code is moved to shdocvw, so there is one 
         binary base for this dialog in both browser-only and 
         integrated-shell installations.  The same code is accessed 
         thru two different CLSIDs (the FileTypes Hook CLSID and the 
         FileTypes cLSID).

         The FileTypes hook mechanism is removed from explorer/shell32
         on the full-shell install.  So in this case, the page is 
         accessed via the FileTypes CLSID.

         On the browser-only install, the page is accessed via the 
         FileTypes Hook CLSID.  The NT version no longer forwards to
         NT shell's implementation because we build a unicode version
         of this page as well.

*/
STDAPI CFileTypes_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    // aggregation checking is handled in class factory

    HRESULT hres;
    CFileTypes * pft;

    pft = LocalAlloc(LPTR, SIZEOF(*pft));
    if (pft)
    {
        DllAddRef(); // DllRelease is called when object is destroyed

        pft->spse.lpVtbl = (IShellPropSheetExtVtbl*)&c_CFileTypesVtbl;
        pft->cRef = 1;

        hres = CFileTypes_QueryInterface(&pft->spse, riid, ppvOut);

        CFileTypes_Release(&pft->spse);
        return hres;
    }

    *ppvOut = NULL;

    return E_OUTOFMEMORY;
}
#endif  // !UNICODE
