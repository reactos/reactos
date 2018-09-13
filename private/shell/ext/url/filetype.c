/* Look for BUILDBUILD to find build hacks. */

/* BUILDBUILD: My hacks to get this to build. */

#include "project.h"
#pragma hdrstop

#include "urlshell.h"           /* BUILDBUILD */

#include <mluisupp.h>

#pragma warning(disable:4001) /* "single line comment" warning */
#include <windowsx.h>
#include <contxids.h>
#include "resource.h"
#pragma warning(default:4001) /* "single line comment" warning */
#include "assoc.h"
#include "clsfact.h"

#define DATASEG_READONLY   DATA_SEG_READ_ONLY
#define hinstCabinet       GetThisModulesHandle()
#define IDI_SYSFILE        154   /* from cabinet\rcids.h: Icon index in shell32 for default icon; used by filetypes */

PRIVATE_DATA HIMAGELIST s_himlSysSmall = NULL;
PRIVATE_DATA HIMAGELIST s_himlSysLarge = NULL;

extern BOOL  lPathIsExeA(LPCSTR);
extern LONG  lRegDeleteKeyA(HKEY, LPCSTR);
extern int   lShell_GetCachedImageIndexA(LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags);

#define Shell_GetCachedImageIndex   lShell_GetCachedImageIndexA
#define PathIsExe                   lPathIsExeA
#undef RegDeleteKey
#define RegDeleteKey                lRegDeleteKeyA

#pragma data_seg(DATA_SEG_READ_ONLY)

CCHAR c_szCommand[]              = "command";
CCHAR c_szDDEAppNot[]            = "ifexec";
CCHAR c_szDDEApp[]               = "Application";
CCHAR c_szDDEExec[]              = "ddeexec";
CCHAR c_szDDETopic[]             = "topic";
CCHAR c_szDefaultIcon[]          = "DefaultIcon";
CCHAR c_szEditFlags[]            = "EditFlags";
CCHAR c_szExefileOpenCommand[]   = "\"%1\"";
CCHAR c_szFile[]                 = "file";
CCHAR c_szNULL[]                 = "";
CCHAR c_szNew[]                  = "New";
CCHAR c_szOpen[]                 = "open";
CCHAR c_szShell2[]               = "shell32.dll";
CCHAR c_szShellOpenCommand[]     = "shell\\open\\command";
CCHAR c_szShell[]                = "Shell";
CCHAR c_szShellexIconHandler[]   = "shellex\\IconHandler";
CCHAR c_szShowExt[]              = "AlwaysShowExt";
CCHAR c_szSpPercentOne[]         = " %1";
CCHAR c_szSpaceFile[]            = " File";
CCHAR c_szSpace[]                = " ";
CCHAR c_szCLSID[]                = "CLSID";

#pragma data_seg()

#pragma warning(disable:4001) /* "single line comment" warning */

/******************************************************************************
                     Original Shell32.dll code starts here.
******************************************************************************/

//================================================================
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

//================================================================
//  View.Options.File Types
//================================================================

// BUGBUG - need to be able to handle case where user enters exe without an ext during EditCommand Dialog

#if 0 /* BUILDBUILD */
#include "cabinet.h"
#endif
#include "filetype.h"
#if 0 /* BUILDBUILD */
#include "rcids.h"
#endif
#if 0 /* BUILDBUILD */
#include "..\..\inc\help.h" // Help IDs
#else
#undef NO_HELP  /* BUILDBUILD */
#include <help.h>
#endif

#if DEBUG
#define FT_DEBUG 0
#else
#define FT_DEBUG 0        // allways leave as 0 for retail
#endif  /* DEBUG */

#pragma data_seg(".text", "CODE")
const char c_szTemplateSS[] = "%s\\%s";
const char c_szTemplateSSS[] = "%s\\%s\\%s";
#pragma data_seg()


// PathProcessCommand is present in SUR (and probably Nashville), it is required to
// fix the use of LFN in registry entries, as it may not be present we must late
// bind to it.

LONG UrlPathProcessCommand( LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags );
LONG UrlPathProcessCommand2( LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags );

typedef LONG (*P_PathProcessCommand)(LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags);

P_PathProcessCommand lpPathProcessCommand = NULL;

// Fix the registry functions for WinNT so that we do the right thing
// and expand REG_STRING_SZ variables.

#ifdef RegQueryValue
#undef RegQueryValue
#endif

#define RegQueryValue(hkey, pszSubkey, pvValue, pcbSize)    SHGetValue(hkey, pszSubkey, NULL, NULL, pvValue, pcbSize)


// ================================================================
VOID FT_CleanupOne(PFILETYPESDIALOGINFO pFTDInfo, PFILETYPESINFO pFTInfo)
{
#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT_CleanupOne");
#endif
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

#ifdef MIME

/* MIME Types
 *************/

typedef BOOL (CALLBACK *ENUMPROC)(PCSTR pcsz, LPARAM lparam);

typedef struct enumdata
{
   ENUMPROC enumproc;

   LPARAM lparam;
}
ENUMDATA;
DECLARE_STANDARD_TYPES(ENUMDATA);

typedef struct mimeenumdata
{
   ENUMPROC enumproc;

   LPARAM lparam;

   PCSTR pcszMIMEType;
}
MIMEENUMDATA;
DECLARE_STANDARD_TYPES(MIMEENUMDATA);

typedef struct bufferdata
{
   PSTR pszBuf;

   UINT ucBufLen;
}
BUFFERDATA;
DECLARE_STANDARD_TYPES(BUFFERDATA);


/*************************** Private MIME Functions **************************/


/* Debug Validation Functions
 *****************************/


#ifdef DEBUG

PRIVATE_CODE IsValidPCENUMDATA(PCENUMDATA pcenumdata)
{
   /* lparam may be any value. */
   return(IS_VALID_READ_PTR(pcenumdata, CENUMDATA) &&
          IS_VALID_CODE_PTR(pcenumdata->enumproc, ENUMPROC));
}


PRIVATE_CODE IsValidPCMIMEENUMDATA(PCMIMEENUMDATA pcmimeenumdata)
{
   /* lparam may be any value. */
   return(IS_VALID_READ_PTR(pcmimeenumdata, CMIMEENUMDATA) &&
          IS_VALID_CODE_PTR(pcmimeenumdata->enumproc, ENUMPROC) &&
          IS_VALID_STRING_PTR(pcmimeenumdata->pcszMIMEType, CSTR));
}


PRIVATE_CODE IsValidPCBUFFERDATA(PCBUFFERDATA pcbufdata)
{
   return(IS_VALID_READ_PTR(pcbufdata, CBUFFERDATA) &&
          EVAL(pcbufdata->ucBufLen > 0) &&
          IS_VALID_WRITE_BUFFER_PTR(pcbufdata->pszBuf, STR, pcbufdata->ucBufLen));
}

#endif   /* DEBUG */


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
PRIVATE_CODE BOOL AddStringToComboBox(HWND hwndComboBox, PCSTR pcsz)
{
   BOOL bResult;
   LRESULT lAddStringResult;

   ASSERT(IS_VALID_HANDLE(hwndComboBox, WND));
   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   lAddStringResult = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)pcsz);

   bResult = (lAddStringResult != CB_ERR &&
              lAddStringResult != CB_ERRSPACE);

   if (bResult)
      TRACE_OUT(("AddStringToComboBox(): Added string %s to combo box.",
                 pcsz));
   else
      WARNING_OUT(("AddStringToComboBox(): Failed to add string %s to combo box.",
                   pcsz));

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
PRIVATE_CODE BOOL SafeAddStringToComboBox(HWND hwndComboBox, PCSTR pcsz)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hwndComboBox, WND));
   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   if (SendMessage(hwndComboBox, CB_FINDSTRINGEXACT, 0, (LPARAM)pcsz) == CB_ERR)
      bResult = AddStringToComboBox(hwndComboBox, pcsz);
   else
   {
      bResult = TRUE;

      TRACE_OUT(("SafeAddStringToComboBox(): String %s already added to combo box.",
                 pcsz));
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
PRIVATE_CODE BOOL SafeAddStringsToComboBox(HDPA hdpa, HWND hwndComboBox)
{
   BOOL bResult;
   int ncStrings;
   int i;

   ASSERT(IS_VALID_HANDLE(hwndComboBox, WND));

   ncStrings = DPA_GetPtrCount(hdpa);

   for (i = 0; i < ncStrings; i++)
   {
      PCSTR pcsz;

      pcsz = DPA_FastGetPtr(hdpa, i);
      ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

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
PRIVATE_CODE BOOL AddAndSetComboBoxCurrentSelection(HWND hwndComboBox,
                                                    PCSTR pcszText)
{
   BOOL bResult;
   LRESULT liSel;

   liSel = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)pcszText);

   bResult = (liSel != CB_ERR &&
              liSel != CB_ERRSPACE &&
              SendMessage(hwndComboBox, CB_SETCURSEL, liSel, 0) != CB_ERR);

   if (bResult)
      TRACE_OUT(("AddAndSetComboBoxCurrentSelection(): Current combo box selection set to %s.",
                 pcszText));

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
PRIVATE_CODE BOOL CALLBACK ExtensionEnumerator(PCSTR pcsz, LPARAM lparam)
{
   BOOL bContinue;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR((PCENUMDATA)lparam, CENUMDATA));

   if (*pcsz == PERIOD)
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
PRIVATE_CODE BOOL CALLBACK MIMETypeEnumerator(PCSTR pcsz, LPARAM lparam)
{
   BOOL bContinue;
   char szMIMEType[MAX_PATH];
   DWORD dwcbContentTypeLen = sizeof(szMIMEType);

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR((PCENUMDATA)lparam, CENUMDATA));

   if (GetRegKeyStringValue(HKEY_CLASSES_ROOT, pcsz, g_cszContentType,
                            szMIMEType, &dwcbContentTypeLen)
       == ERROR_SUCCESS)
   {
      PCENUMDATA pcenumdata = (PCENUMDATA)lparam;

      TRACE_OUT(("MIMETypeEnumerator(): MIME type %s registered for extension %s.",
                 szMIMEType,
                 pcsz));

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
PRIVATE_CODE BOOL CALLBACK MIMETypeExtensionEnumerator(PCSTR pcsz,
                                                       LPARAM lparam)
{
   BOOL bContinue = TRUE;
   char szMIMEType[MAX_PATH];
   DWORD dwcbContentTypeLen = sizeof(szMIMEType);

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR((PCMIMEENUMDATA)lparam, CMIMEENUMDATA));

   if (GetRegKeyStringValue(HKEY_CLASSES_ROOT, pcsz, g_cszContentType,
                            szMIMEType, &dwcbContentTypeLen)
       == ERROR_SUCCESS)
   {
      PCMIMEENUMDATA pcmimeenumdata = (PCMIMEENUMDATA)lparam;

      if (! lstrcmpi(szMIMEType, pcmimeenumdata->pcszMIMEType))
      {
         TRACE_OUT(("MIMETypeEnumerator(): Found extension %s registered for MIME type %s.",
                    pcsz,
                    szMIMEType));

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
PRIVATE_CODE BOOL CALLBACK AddStringToComboBoxEnumerator(PCSTR pcsz,
                                                         LPARAM lparam)
{
   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
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
PRIVATE_CODE BOOL CALLBACK AddHandledMIMETypeEnumerator(PCSTR pcsz,
                                                        LPARAM lparam)
{
   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
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
PRIVATE_CODE BOOL CALLBACK ReplacementDefExtensionEnumerator(PCSTR pcsz,
                                                             LPARAM lparam)
{
   PBUFFERDATA pbufdata = (PBUFFERDATA)lparam;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR((PCBUFFERDATA)lparam, CBUFFERDATA));

   if (! *(pbufdata->pszBuf) ||
       lstrcmpi(pcsz, pbufdata->pszBuf) < 0)
   {
      if ((UINT)lstrlen(pcsz) < pbufdata->ucBufLen)
         lstrcpy(pbufdata->pszBuf, pcsz);
      else
         WARNING_OUT(("ReplacementDefExtensionEnumerator(): %u byte buffer too small to hold %d byte extension %s.",
                      pcsz,
                      lstrlen(pcsz),
                      pbufdata->ucBufLen));
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
PRIVATE_CODE LRESULT EnumSubKeys(HKEY hkeyParent, ENUMPROC enumproc,
                                 LPARAM lparam)
{
   LONG lResult;
   DWORD dwi;
   char szSubKey[MAX_PATH];
   DWORD dwcbSubKeyLen = sizeof(szSubKey);

   /* lparam may be any value. */
   ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));

   TRACE_OUT(("EnumSubKeys(): Enumerating sub keys."));

   for (dwi = 0;
        (lResult = RegEnumKeyEx(hkeyParent, dwi, szSubKey, &dwcbSubKeyLen,
                                NULL, NULL, NULL, NULL)) == ERROR_SUCCESS;
        dwi++)
   {
      if (! (*enumproc)(szSubKey, lparam))
      {
         TRACE_OUT(("EnumSubKeys(): Callback aborted at sub key %s, index %lu.",
                    szSubKey,
                    dwi));
         lResult = ERROR_CANCELLED;
         break;
      }

      dwcbSubKeyLen = sizeof(szSubKey);
   }

   if (lResult == ERROR_NO_MORE_ITEMS)
   {
      TRACE_OUT(("EnumSubKeys(): Enumerated %lu sub keys.",
                 dwi));

      lResult = ERROR_SUCCESS;
   }
   else
      TRACE_OUT(("EnumSubKeys(): Stopped after enumerating %lu sub keys.",
                 dwi));

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
PRIVATE_CODE LRESULT EnumExtensions(ENUMPROC enumproc, LPARAM lparam)
{
   ENUMDATA enumdata;

   /* lparam may be any value. */
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));

   TRACE_OUT(("EnumExtensions(): Enumerating extensions."));

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
PRIVATE_CODE LRESULT EnumMIMETypes(ENUMPROC enumproc, LPARAM lparam)
{
   ENUMDATA enumdata;

   /* lparam may be any value. */
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));

   TRACE_OUT(("EnumMIMETypes(): Enumerating MIME types."));

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
PRIVATE_CODE LRESULT EnumExtensionsOfMIMEType(ENUMPROC enumproc, LPARAM lparam,
                                              PCSTR pcszMIMEType)
{
   MIMEENUMDATA mimeenumdata;

   /* lparam may be any value. */
   ASSERT(IS_VALID_CODE_PTR(enumproc, ENUMPROC));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));

   ASSERT(*pcszMIMEType);

   TRACE_OUT(("EnumExtensionsOfMIMEType(): Enumerating extensions registered as MIME type %s.",
              pcszMIMEType));

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
PRIVATE_CODE BOOL FindMIMETypeOfExtensionList(HDPA hdpaExtensions,
                                              PSTR pszMIMETypeBuf,
                                              UINT ucMIMETypeBufLen)
{
   BOOL bFound = FALSE;
   int ncExtensions;
   int i;

   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszMIMETypeBuf, STR, ucMIMETypeBufLen));

   ncExtensions = DPA_GetPtrCount(hdpaExtensions);

   for (i = 0; i < ncExtensions; i++)
   {
      PCSTR pcszExtension;

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
          (IS_VALID_STRING_PTR(pszMIMETypeBuf, STR) &&
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
PRIVATE_CODE BOOL RegisterContentTypeForArrayOfExtensions(PCSTR pcszMIMEType,
                                                          HDPA hdpaExtensions)
{
   BOOL bResult = TRUE;
   int ncExtensions;
   int i;

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));

   ncExtensions = DPA_GetPtrCount(hdpaExtensions);

   for (i = 0; i < ncExtensions; i++)
   {
      PCSTR pcszExtension;

      pcszExtension = DPA_FastGetPtr(hdpaExtensions, i);
      ASSERT(IsValidExtension(pcszExtension));

      bResult = RegisterMIMETypeForExtension(pcszExtension, pcszMIMEType);

      if (bResult)
         TRACE_OUT(("RegisterContentTypeForArrayOfExtensions(): Registered MIME type %s for extension %s.",
                    pcszMIMEType,
                    pcszExtension));
      else
      {
         WARNING_OUT(("RegisterContentTypeForArrayOfExtensions(): Failed to register MIME type %s for extension %s.",
                      pcszMIMEType,
                      pcszExtension));
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
PRIVATE_CODE BOOL UnregisterContentTypeForArrayOfExtensions(HDPA hdpaExtensions)
{
   BOOL bResult = TRUE;
   int ncExtensions;
   int i;

   ncExtensions = DPA_GetPtrCount(hdpaExtensions);

   for (i = 0; i < ncExtensions; i++)
   {
      PCSTR pcszExtension;

      pcszExtension = DPA_FastGetPtr(hdpaExtensions, i);
      ASSERT(IsValidExtension(pcszExtension));

      if (! UnregisterMIMETypeForExtension(pcszExtension))
         bResult = FALSE;

      if (bResult)
         TRACE_OUT(("UnregisterContentTypeForArrayOfExtensions(): Unregistered MIME type for extension %s.",
                    pcszExtension));
      else
         WARNING_OUT(("UnregisterContentTypeForArrayOfExtensions(): Failed to unregister MIME type for extension %s.",
                      pcszExtension));
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
PRIVATE_CODE int CALLBACK ExtensionSearchCmp(PVOID pvFirst, PVOID pvSecond,
                                             LPARAM lparam)
{
   ASSERT(IS_VALID_STRING_PTR(pvFirst, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pvSecond, CSTR));
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
PRIVATE_CODE BOOL NeedReplacementDefExtension(HDPA hdpaExtension,
                                              PCSTR pcszOriginalMIMEType)
{
   BOOL bNeedReplacementDefExtension;
   char szDefExtension[MAX_PATH];

   ASSERT(IS_VALID_STRING_PTR(pcszOriginalMIMEType, CSTR));

   ASSERT(*pcszOriginalMIMEType);

   /* Is there a default extension specified for the MIME type? */

   if (MIME_GetExtension(pcszOriginalMIMEType, szDefExtension, sizeof(szDefExtension)))
   {
      /* Yes.  Is that default extension in the list of extensions? */

      bNeedReplacementDefExtension = (DPA_Search(hdpaExtension, szDefExtension,
                                                 0, &ExtensionSearchCmp, 0, 0)
                                      != -1);

      if (bNeedReplacementDefExtension)
         TRACE_OUT(("NeedReplacementDefExtension(): Previous default extension %s for MIME type %s removed.",
                    szDefExtension,
                    pcszOriginalMIMEType));
      else
         TRACE_OUT(("NeedReplacementDefExtension(): Previous default extension %s for MIME type %s remains registered.",
                    szDefExtension,
                    pcszOriginalMIMEType));
   }
   else
   {
      /* No.  Choose a new default extension. */

      bNeedReplacementDefExtension = TRUE;

      WARNING_OUT(("NeedReplacementDefExtension(): No default extension registered for MIME type %s.  Choosing default extension.",
                   pcszOriginalMIMEType));
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
PRIVATE_CODE BOOL FindReplacementDefExtension(HDPA hdpaExtension,
                                        PCSTR pcszOriginalMIMEType,
                                        PSTR pszDefExtensionBuf, UINT ucDefExtensionBufLen)
{
   BOOL bFound;
   BUFFERDATA bufdata;

   ASSERT(IS_VALID_STRING_PTR(pcszOriginalMIMEType, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszDefExtensionBuf, STR, ucDefExtensionBufLen));

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
      TRACE_OUT(("FindReplacementDefExtension(): Extension %s remains registered as MIME type %s.",
                 pszDefExtensionBuf,
                 pcszOriginalMIMEType));
   else
      TRACE_OUT(("FindReplacementDefExtension(): No extensions remain registered as MIME type %s.",
                 pcszOriginalMIMEType));

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
PRIVATE_CODE BOOL RegisterNewDefExtension(HDPA hdpaExtension,
                                              PCSTR pcszOriginalMIMEType)
{
   BOOL bResult = TRUE;

   ASSERT(IS_VALID_STRING_PTR(pcszOriginalMIMEType, CSTR));

   /* Was there originally a MIME type specified? */

   if (*pcszOriginalMIMEType)
   {
      /*
       * Yes.  Was the previous default extension for the MIME type just
       * removed?
       */

      if (NeedReplacementDefExtension(hdpaExtension, pcszOriginalMIMEType))
      {
         char szDefExtension[MAX_PATH];

         /*
          * Yes.  Are there any remaining extensions registered as the MIME
          * type?
          */

         if (FindReplacementDefExtension(hdpaExtension, pcszOriginalMIMEType,
                                         szDefExtension,
                                         sizeof(szDefExtension)))
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
      TRACE_OUT(("RegisterNewDefExtension(): No original MIME type, no new default extension"));

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
PRIVATE_CODE BOOL AddMIMETypeInfo(PFILETYPESDIALOGINFO pFTDInfo,
                                  PCSTR pcszOldMIMEType, PCSTR pcszNewMIMEType,
                                  PCSTR pcszDefExtension)
{
   BOOL bMIMEResult;
   BOOL bOldDefExtensionResult;
   BOOL bNewDefExtensionResult;

   ASSERT(IS_VALID_STRING_PTR(pcszOldMIMEType, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszNewMIMEType, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszDefExtension, CSTR));

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
PRIVATE_CODE BOOL RemoveMIMETypeInfo(PFILETYPESDIALOGINFO pFTDInfo,
                                     PCSTR pcszOldMIMEType)
{
   BOOL bMIMEResult;
   BOOL bDefExtensionResult;

   ASSERT(IS_VALID_STRING_PTR(pcszOldMIMEType, CSTR));

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
PRIVATE_CODE BOOL GetFirstString(HDPA hdpa, PSTR pszBuf, UINT ucBufLen)
{
   BOOL bFound = FALSE;
   int ncStrings;
   int iFirst = 0;
   int i;

   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszBuf, STR, ucBufLen));

   ncStrings = DPA_GetPtrCount(hdpa);

   for (i = 0; i < ncStrings; i++)
   {
      PCSTR pcsz;

      pcsz = DPA_FastGetPtr(hdpa, i);
      ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

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
          IS_VALID_STRING_PTR(pszBuf, STR));
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
PRIVATE_CODE BOOL IsListOfExtensions(HDPA hdpa)
{
   BOOL bNoRogues = TRUE;
   BOOL bAnyExtensions = FALSE;
   int ncStrings;
   int i;

   ncStrings = DPA_GetPtrCount(hdpa);

   for (i = 0; i < ncStrings; i++)
   {
      PCSTR pcsz;

      pcsz = DPA_FastGetPtr(hdpa, i);
      ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

      if (*pcsz == PERIOD)
      {
         bAnyExtensions = TRUE;

         TRACE_OUT(("IsListOfExtensions(): Found extension %s.",
                    pcsz));
      }
      else
      {
         bNoRogues = FALSE;

         TRACE_OUT(("IsListOfExtensions(): Found non-extension %s.",
                    pcsz));
      }
   }

   TRACE_OUT(("IsListOfExtensions(): This %s a list of extensions.",
              (bAnyExtensions && bNoRogues) ? "is" : "is not"));

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
PRIVATE_CODE BOOL InitContentTypeEditControl(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;
   char szMIMEType[MAX_PATH];

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   EVAL(SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_RESETCONTENT, 0, 0));

   if (FindMIMETypeOfExtensionList(pFTDInfo->pFTInfo->hDPAExt, szMIMEType,
                                   sizeof(szMIMEType)) &&
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
PRIVATE_CODE BOOL FillContentTypeListBox(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   if (IS_FLAG_CLEAR(pFTDInfo->pFTInfo->dwMIMEFlags, MIME_FL_CONTENT_TYPES_ADDED))
   {
      SET_FLAG(pFTDInfo->pFTInfo->dwMIMEFlags, MIME_FL_CONTENT_TYPES_ADDED);

      bResult = (EnumMIMETypes(&AddHandledMIMETypeEnumerator,
                               (LPARAM)(pFTDInfo->hwndContentTypeComboBox))
                 == ERROR_SUCCESS);
   }
   else
   {
      TRACE_OUT(("FillContentTypeListBox(): Content Type combo box already filled."));

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
PRIVATE_CODE BOOL GetAssociatedExtension(HWND hdlg, PSTR pszAssocExtensionBuf,
                                         UINT ucAssocExtensionBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszAssocExtensionBuf, STR, ucAssocExtensionBufLen));

   if (EVAL(ucAssocExtensionBufLen > 2))
   {
      PFILETYPESDIALOGINFO pFTDInfo;

      pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

      /* Leave room for possible leading period. */

      GetWindowText(pFTDInfo->hwndDocExt, pszAssocExtensionBuf + 1,
                    ucAssocExtensionBufLen - 1);

      if (pszAssocExtensionBuf[1])
      {
         /* Prepend period if necessary. */

         if (pszAssocExtensionBuf[1] == PERIOD)
            /* (+ 1) for null terminator. */
            MoveMemory(pszAssocExtensionBuf, pszAssocExtensionBuf + 1,
                       lstrlen(pszAssocExtensionBuf + 1) + 1);
         else
            pszAssocExtensionBuf[0] = PERIOD;

         bResult = TRUE;
      }
   }

   if (! bResult)
   {
      if (ucAssocExtensionBufLen > 0)
         *pszAssocExtensionBuf = '\0';
   }

   if (bResult)
      TRACE_OUT(("GetAssociatedExtension(): Associated Extension is %s.",
                 pszAssocExtensionBuf));
   else
      TRACE_OUT(("GetAssociatedExtension(): No Associated Extension."));

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
PRIVATE_CODE BOOL FillDefExtensionListBox(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;
   char szMIMEType[MAX_PATH];
   LRESULT liSel;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   EVAL(SendMessage(pFTDInfo->hwndDefExtensionComboBox, CB_RESETCONTENT, 0, 0));

   GetWindowText(pFTDInfo->hwndContentTypeComboBox, szMIMEType,
                 sizeof(szMIMEType));

   if (*szMIMEType)
   {
      char szDefExtension[MAX_PATH];

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
                                           sizeof(szDefExtension)) &&
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
          MIME_GetExtension(szMIMEType, szDefExtension, sizeof(szDefExtension)))
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
PRIVATE_CODE void SetDefExtensionComboBoxState(HWND hdlg, PCSTR pcszMIMEType)
{
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   if (IS_FLAG_CLEAR(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
   {
      char szAssocExtension[MAX_PATH];
      BOOL bEnable;

      bEnable = (*pcszMIMEType != '\0');

       if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
         /* New File Type dialog. */
         bEnable = (bEnable &&
                    GetAssociatedExtension(hdlg, szAssocExtension,
                                           sizeof(szAssocExtension)));

      EnableWindow(pFTDInfo->hwndDefExtensionComboBox, bEnable);

      TRACE_OUT(("EnableDefExtensionComboBox(): Default extension combo box %s.",
                 bEnable ? "enabled" : "disabled"));
   }

   return;
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
PRIVATE_CODE BOOL SetDefExtension(HWND hdlg, PCSTR pcszMIMEType)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEType, CSTR));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   EVAL(SendMessage(pFTDInfo->hwndDefExtensionComboBox, CB_RESETCONTENT, 0, 0));

   /* Any MIME type? */

   if (*pcszMIMEType)
   {
      char szDefExtension[MAX_PATH];

      /*
       * Yes.  Use the registered default extension, or the first extension in
       * the list of extensions.
       */

      bResult = MIME_GetExtension(pcszMIMEType, szDefExtension, sizeof(szDefExtension));

      if (! bResult)
      {
          if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
            /* New File Type dialog. */
            bResult = GetAssociatedExtension(hdlg, szDefExtension,
                                             sizeof(szDefExtension));
         else
            /* Edit File Type dialog. */
            bResult = GetFirstString(pFTDInfo->pFTInfo->hDPAExt, szDefExtension,
                                     sizeof(szDefExtension));
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
PRIVATE_CODE BOOL FillDefExtensionEditControlFromSelection(HWND hdlg)
{
   BOOL bResult = TRUE;
   PFILETYPESDIALOGINFO pFTDInfo;
   char szMIMEType[MAX_PATH];
   LRESULT liSel;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   liSel = SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_GETCURSEL, 0, 0);

   if (liSel != CB_ERR)
   {
      bResult = (SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_GETLBTEXT,
                             liSel, (LPARAM)szMIMEType) != CB_ERR &&
                 SetDefExtension(hdlg, szMIMEType));

      SetDefExtensionComboBoxState(hdlg, szMIMEType);
   }
   else
      TRACE_OUT(("FillDefExtensionEditControlFromSelection(): No MIME type selection."));

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
PRIVATE_CODE BOOL FillDefExtensionEditControlFromEditControl(HWND hdlg)
{
   PFILETYPESDIALOGINFO pFTDInfo;
   char szMIMEType[MAX_PATH];

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   GetWindowText(pFTDInfo->hwndContentTypeComboBox, szMIMEType,
                 sizeof(szMIMEType));

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
PRIVATE_CODE void SetMIMEControlState(HWND hdlg, BOOL bEnable)
{
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

    EnableWindow(pFTDInfo->hwndContentTypeComboBox, bEnable);
   EnableWindow(pFTDInfo->hwndDefExtensionComboBox, bEnable);

   return;
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
PRIVATE_CODE BOOL InitMIMEControls(HWND hdlg)
{
   BOOL bResult;
   PFILETYPESDIALOGINFO pFTDInfo;

   ASSERT(IS_VALID_HANDLE(hdlg, WND));

   pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hdlg, DWLP_USER);

   EVAL(SendMessage(pFTDInfo->hwndContentTypeComboBox, CB_LIMITTEXT, MAX_PATH, 0));
   EVAL(SendMessage(pFTDInfo->hwndDefExtensionComboBox, CB_LIMITTEXT, MAX_PATH, 0));

    if (pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
   {
      /* New File Type dialog. */

      CLEAR_FLAG(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME);

      *(pFTDInfo->pFTInfo->szOriginalMIMEType) = '\0';

      bResult = TRUE;

      TRACE_OUT(("InitMIMEControls(): Cleared MIME controls for New File Type dialog box."));
   }
   else
   {
      /* Edit File Type dialog. */

      /*
       * Disable MIME controls if requested, and when editing non-extension
       * types.
       */

      if (IS_FLAG_CLEAR(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
      {
         if (! IsListOfExtensions(pFTDInfo->pFTInfo->hDPAExt))
         {
            SET_FLAG(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME);

            TRACE_OUT(("InitMIMEControls(): Disabling MIME controls for non-extension type."));
         }
      }
      else
         TRACE_OUT(("InitMIMEControls(): Disabling MIME controls, as requested."));

      if (IS_FLAG_CLEAR(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
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

         TRACE_OUT(("InitMIMEControls(): Initialized MIME controls for Edit File Type dialog box."));
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
PRIVATE_CODE BOOL OnContentTypeSelectionChange(HWND hdlg)
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
PRIVATE_CODE BOOL OnContentTypeEditChange(HWND hdlg)
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
PRIVATE_CODE BOOL OnContentTypeDropDown(HWND hdlg)
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
PRIVATE_CODE BOOL OnDefExtensionDropDown(HWND hdlg)
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
PRIVATE_CODE BOOL RegisterMIMEInformation(PFILETYPESDIALOGINFO pFTDInfo)
{
   BOOL bResult;

   if (IS_FLAG_CLEAR(pFTDInfo->pFTInfo->dwAttributes, FTA_NoEditMIME))
   {
       char szMIMEType[MAX_PATH];
      char szDefExtension[MAX_PATH];

       GetWindowText(pFTDInfo->hwndContentTypeComboBox, szMIMEType,
                    sizeof(szMIMEType));

       GetWindowText(pFTDInfo->hwndDefExtensionComboBox, szDefExtension,
                    sizeof(szDefExtension));

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

      TRACE_OUT(("RegisterMIMEInformation(): Not registering MIME information, as requested."));
   }

   return(bResult);
}

#endif   /* MIME */

//================================================================
//================================================================
BOOL FT_OnInitDialog(HWND hDialog, LPARAM lParam)
{
    HMODULE hShellDll = NULL;

    PFILETYPESDIALOGINFO pFTDInfo;
    BOOL bRC = FALSE;
    DECLAREWAITCURSOR;
#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT: WM_INITDIALOG");
#endif

    SetWaitCursor();

   Shell_GetImageLists(&s_himlSysLarge, &s_himlSysSmall);

    pFTDInfo = (PFILETYPESDIALOGINFO)((LPPROPSHEETPAGE)lParam)->lParam;
    SetWindowLongPtr(hDialog, DWLP_USER, (LPARAM)pFTDInfo);
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
            {    // macro needs brackets
                ListView_SetItemState(pFTDInfo->hwndLVFT, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED); // Set listview to item 0
                bRC = TRUE;
            }
        }
            SendMessage(pFTDInfo->hwndLVFT, WM_SETREDRAW, TRUE, 0);
    }

   // Attempt to bind to the ordinal for shell32!PathProcessCommand

    hShellDll = GetModuleHandle( c_szShell2 );

    if ( hShellDll )
    {
        lpPathProcessCommand = (P_PathProcessCommand)GetProcAddress(hShellDll, (LPCSTR)653);
    }

    ResetWaitCursor();

    return(bRC);
}

//================================================================
//================================================================
VOID FT_OnLVN_ItemChanged(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, LPARAM lParam)
{
    LV_ITEM LVItem;

    if((((NM_LISTVIEW *)lParam)->uChanged & LVIF_STATE) &&
        ((NM_LISTVIEW *)lParam)->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))
    {
#ifdef FT_DEBUG
        DebugMsg(DM_TRACE, "FT: WM_NOTIFY - LVN_ITEMCHANGED");
#endif

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

//================================================================
//================================================================
VOID FT_OnLVN_GetDispInfo(PFILETYPESDIALOGINFO pFTDInfo, LPARAM lParam)
{
    LV_ITEM LVItem;
    int iImageIndex;
    HICON hIcon;
    SHFILEINFO sfi;
    LV_DISPINFO *pnmv;

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT: WM_NOTIFY - LVN_GETDISPINFO");
#endif
    pnmv = (LV_DISPINFO *)lParam;

    if(pnmv->item.mask & LVIF_IMAGE)
    {
        if(((PFILETYPESINFO)(pnmv->item.lParam))->dwAttributes & FTA_HasExtension)
        {
            if(SHGetFileInfo(DPA_FastGetPtr(((PFILETYPESINFO)(pnmv->item.lParam))->hDPAExt,0),
                    FILE_ATTRIBUTE_NORMAL,
                    &sfi, sizeof(SHFILEINFO),
                    SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES))
                hIcon = sfi.hIcon;
         else
                hIcon = NULL;     /* BUILDBUILD: Added. */
#ifdef FT_DEBUG
            DebugMsg(DM_TRACE, "LVN_GETDISPINFO SHGetFileInfo szFile=%s hIcon=0x%x",
                DPA_FastGetPtr(((PFILETYPESINFO)(pnmv->item.lParam))->hDPAExt,0), hIcon);
#endif
        }
        else
            hIcon = GetDefaultIcon(&(((PFILETYPESINFO)(pnmv->item.lParam))->hkeyFT), ((PFILETYPESINFO)(pnmv->item.lParam))->szId, SHGFI_SMALLICON);

        if(hIcon == (HICON)NULL)  // use default shell icon in case above calls fail to find an icon
        {
            UINT iIndex = Shell_GetCachedImageIndex(c_szShell2, II_DOCNOASSOC, 0);
            hIcon = ImageList_ExtractIcon(hinstCabinet, s_himlSysSmall, iIndex);
        }

        if(hIcon != (HICON)NULL)
        {
#if FT_DEBUG
            HDC hdc = GetDC(NULL);
            DrawIcon(hdc,200, 0, hIcon);
            ReleaseDC(NULL, hdc);
#endif

            if((iImageIndex = ImageList_AddIcon(pFTDInfo->himlFT, hIcon)) != (-1))
            {
                ZeroMemory(&LVItem, sizeof(LV_ITEM));
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

//================================================================
//================================================================
BOOL FT_OnNotify(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, LPARAM lParam)
{
    LPNMHDR pnm = (NMHDR*)lParam;
#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT: WM_NOTIFY code=0x%x =%d", ((NMHDR *) lParam)->code, ((NMHDR *) lParam)->code);
#endif

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

//================================================================
//================================================================
VOID FT_OnCommand(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    char   szMsg[MAX_PATH];
    char   szTitle[MAX_PATH];
            

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT: WM_COMMAND");
#endif

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

            MLLoadStringA(IDS_FT_MB_REMOVETYPE, szMsg, ARRAYSIZE(szMsg));
            MLLoadStringA(IDS_FT, szTitle, ARRAYSIZE(szTitle));
            
            if(ShellMessageBox(MLGetHinst(), hDialog,
                               szMsg,
                               szTitle,
                               MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
            {
                RemoveFileType(pFTDInfo);
                PropSheet_CancelToClose(GetParent(hDialog));
            }
            break;
    }
}

//================================================================
//================================================================
#pragma data_seg(DATASEG_READONLY)
const static DWORD aFileTypeOptionsHelpIDs[] = {  // Context Help IDs
    IDC_GROUPBOX,                 IDH_COMM_GROUPBOX,
    IDC_FT_PROP_LV_FILETYPES,     IDH_FCAB_FT_PROP_LV_FILETYPES,
    IDC_FT_PROP_NEW,              IDH_FCAB_FT_PROP_NEW,
    IDC_FT_PROP_REMOVE,           IDH_FCAB_FT_PROP_REMOVE,
    IDC_FT_PROP_EDIT,             IDH_FCAB_FT_PROP_EDIT,
    IDC_FT_PROP_EDIT,             IDH_FCAB_FT_PROP_EDIT,
    IDC_FT_PROP_DOCICON,          IDH_FILETYPE_EXTENSION,
    IDC_FT_PROP_DOCEXTRO_TXT,     IDH_FILETYPE_EXTENSION,
    IDC_FT_PROP_DOCEXTRO,         IDH_FILETYPE_EXTENSION,
#ifdef MIME
    IDC_FT_PROP_CONTTYPERO_TXT,   IDH_FILETYPE_CONTENT_TYPE,
    IDC_FT_PROP_CONTTYPERO,       IDH_FILETYPE_CONTENT_TYPE,
#endif   /* MIME */
    IDC_FT_PROP_OPENICON,         IDH_FILETYPE_OPENS_WITH,
    IDC_FT_PROP_OPENEXE_TXT,      IDH_FILETYPE_OPENS_WITH,
    IDC_FT_PROP_OPENEXE,          IDH_FILETYPE_OPENS_WITH,
    0, 0
};
#ifdef MIME
CONST char s_cszMIMEHelpFile[] = "IExplore.hlp";
#endif   /* MIME */
#pragma data_seg()
//================================================================
//================================================================

#ifdef MIME

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
PRIVATE_CODE PCSTR FT_GetHelpFileFromControl(HWND hwndControl)
{
   PCSTR pcszHelpFile = NULL;
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
         case IDC_BROWSEINPLACE:

            /* MIME help comes from the MIME help file. */
            pcszHelpFile = s_cszMIMEHelpFile;
            break;

         default:
            /* Other help is taken from the default Win95 help file. */
            break;
      }
   }

   TRACE_OUT(("FT_GetHelpFileFromControl(): Using %s for control %d (HWND %#lx).",
              pcszHelpFile ? pcszHelpFile : "default Win95 help file",
              nControlID,
              hwndControl));

   ASSERT(! pcszHelpFile ||
          IS_VALID_STRING_PTR(pcszHelpFile, CSTR));

   return(pcszHelpFile);
}

#endif   /* MIME */

INT_PTR CALLBACK FT_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    PFILETYPESDIALOGINFO pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hDialog, DWLP_USER);

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FileTypesDialogProc wParam=0x%x lParam=0x%x", wParam, lParam);
#endif

    switch(message)
    {
        case WM_INITDIALOG:
            return(FT_OnInitDialog(hDialog, lParam));

        case WM_NOTIFY:
            return(FT_OnNotify(pFTDInfo, hDialog, lParam));

        case WM_DESTROY:
            if (pFTDInfo->hThread) {
                HANDLE hThread = pFTDInfo->hThread;
                pFTDInfo->hThread = 0;        // signal thread that we are done if still running
                if(WaitForSingleObject(hThread, 2000) == WAIT_TIMEOUT)
                    TerminateThread(hThread, 0);
            }

            // Tidy up the function pointers we aquired into shelldll.

            lpPathProcessCommand = NULL;
            break;

        case WM_COMMAND:
            FT_OnCommand(pFTDInfo, hDialog, wParam, lParam);
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle,
#ifdef MIME
                    FT_GetHelpFileFromControl((HWND)(((LPHELPINFO)lParam)->hItemHandle)),
#else
                    NULL,
#endif   /* MIME */
                    HELP_WM_HELP, (DWORD_PTR)(LPSTR)aFileTypeOptionsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
        {
            POINT pt;

            if ((int)SendMessage(hDialog, WM_NCHITTEST, 0, lParam) != HTCLIENT)
               return FALSE;   // don't process it

            LPARAM_TO_POINT(lParam, pt);
            EVAL(ScreenToClient(hDialog, &pt));

               SHWinHelpOnDemandWrap((HWND)wParam,
#ifdef MIME
                    FT_GetHelpFileFromControl(ChildWindowFromPoint(hDialog, pt)),
#else
                    NULL,
#endif   /* MIME */
                    HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)aFileTypeOptionsHelpIDs);
            return TRUE;
        }
    }

    return(FALSE);
}

//================================================================
//================================================================

#ifdef MIME

BOOL FTEdit_ConfirmOpenAfterDownload(PFILETYPESDIALOGINFO pFTDInfo)
{
   return(! ClassIsSafeToOpen(pFTDInfo->pFTInfo->szId));
}

BOOL FTEdit_SetConfirmOpenAfterDownload(PFILETYPESDIALOGINFO pFTDInfo,
                                        BOOL bConfirm)
{
   return(SetClassEditFlags(pFTDInfo->pFTInfo->szId, FTA_OpenIsSafe,
                            ! bConfirm));
}

// Defined originally in shdocvw
#define BROWSEHACK_DONTINPLACENAVIGATE     0x00000008

void 
FTEdit_InitBrowseInPlace(
    HWND hdlg,
    PFILETYPESDIALOGINFO pFTDInfo)
{
    HWND hwndCtl = GetDlgItem(hdlg, IDC_BROWSEINPLACE);
    TCHAR sz[MAX_PATH];

    wsprintf(sz, TEXT("%s\\DocObject"), pFTDInfo->pFTInfo->szId);

    // Does the class\DocObject key exist?
    if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, sz, NULL, NULL, NULL, NULL))
    {
        // Yes
        DWORD dwValue;
        DWORD cbSize;

        EnableWindow(hwndCtl, TRUE);

        cbSize = sizeof(dwValue);
        if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId, 
                                   TEXT("BrowserFlags"), NULL, &dwValue,
                                   &cbSize))
        {
            Button_SetCheck(hwndCtl, IS_FLAG_CLEAR(dwValue, BROWSEHACK_DONTINPLACENAVIGATE));
        }
        else
        {
            Button_SetCheck(hwndCtl, TRUE);
        }
    }
}


void
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
            CLEAR_FLAG(dwValue, BROWSEHACK_DONTINPLACENAVIGATE);
        else
            SET_FLAG(dwValue, BROWSEHACK_DONTINPLACENAVIGATE);
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

#endif   /* MIME */

//================================================================
VOID FTEdit_EnableButtonsPerAction(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, int iItem)
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

//================================================================
//================================================================
BOOL FTEdit_IsExtShowable(PFILETYPESDIALOGINFO pFTDInfo)
{
    char szShowExt[MAX_PATH];
    DWORD dwType;
    DWORD dwShowExt;
    // First see if the type has a FileViews Sub key.
    dwShowExt = sizeof(szShowExt);
    return(RegQueryValueEx(pFTDInfo->pFTInfo->hkeyFT, c_szShowExt, NULL, &dwType, (LPSTR)szShowExt, &dwShowExt)
        == ERROR_SUCCESS);
}

//================================================================
//================================================================
void FTEdit_SetShowExt(PFILETYPESDIALOGINFO pFTDInfo, BOOL bShowExt)
{
    if (bShowExt)
        RegSetValueEx(pFTDInfo->pFTInfo->hkeyFT, (LPSTR)c_szShowExt, 0, REG_SZ, c_szNULL, 0);
    else
        RegDeleteValue(pFTDInfo->pFTInfo->hkeyFT, (LPSTR)c_szShowExt);
}

//================================================================

#ifdef FEATURE_INTL
// International support

typedef struct
{
    HFONT hfontNew;
    HFONT hfontOld;
    BOOL  bStock;
}
DLGFONTDATA, *LPDLGFONTDATA;

#define DLGATOM_FONTDATA 0x100       // for window prop.

BOOL CALLBACK SetfontChildProc(HWND hwnd, LPARAM lparam)
{
    char szClass[32];
    if (GetClassName(hwnd, szClass, sizeof(szClass)))
    {
        if ( !lstrcmpi(szClass, "Edit")
    || !lstrcmpi(szClass, "ComboBox")
    || !lstrcmpi(szClass, "Button")
    || !lstrcmpi(szClass, "ListBox") )
        {
            // the first one is enough to store the font handle.
            if(((DLGFONTDATA *)lparam)->bStock == FALSE
              && ((DLGFONTDATA *)lparam)->hfontOld == NULL)
                ((DLGFONTDATA *)lparam)->hfontOld = GetWindowFont(hwnd);

            SetWindowFont(hwnd, ((DLGFONTDATA *)lparam)->hfontNew, FALSE);
        }
    }
    return TRUE;
}

int CALLBACK MyEnumFontFamProc(
    ENUMLOGFONTEX FAR*  lpelf,    // address of logical-font data
    NEWTEXTMETRIC FAR*  lpntm,    // address of physical-font data
    int  FontType,    // type of font
    LPARAM  lParam     // address of application-defined data
   )
{
    LOGFONT *pLogfont = (LOGFONT*)lParam;

    *pLogfont = lpelf->elfLogFont;

    return;

}

void SetShellFont(HWND hwnd)
{
    LOGFONT    lfGui, lfDlg;
    HFONT   hfont=NULL;
    LPDLGFONTDATA pDfd;

    if ( GetProp(hwnd, MAKEINTATOM(DLGATOM_FONTDATA)) )
        return; // don't do this twice otherwise leaks resource.


    if (!(hfont=GetWindowFont(hwnd)))
    {
        hfont=GetStockObject(SYSTEM_FONT);
    }
    GetObject(hfont, sizeof(LOGFONT), &lfDlg);

    if ( (hfont = GetStockObject(DEFAULT_GUI_FONT)) == FALSE )
    {
        // NT 3.51 case
        LOGFONT *pLogfont;
        HDC hDC = GetDC(hwnd);

        if( !(pLogfont = LocalAlloc(GPTR, sizeof(LOGFONT))) )
        {
            ReleaseDC(hwnd, hDC);
            return;
        }
        GetProfileString("FontSubstitutes","MS Shell Dlg",pLogfont->lfFaceName,
                                   lfGui.lfFaceName,sizeof(lfGui.lfFaceName) );
        EnumFontFamilies(hDC,lfGui.lfFaceName,(FONTENUMPROC)MyEnumFontFamProc,
                                                             (LPARAM)pLogfont);
        lfGui = *pLogfont;
        LocalFree(pLogfont);
        ReleaseDC(hwnd, hDC);

    }
    else
        GetObject(hfont, sizeof(LOGFONT), &lfGui);

    if (!(pDfd = (LPDLGFONTDATA)LocalAlloc(GPTR, sizeof(DLGFONTDATA))))
    {
        return;
    }


    if ( (lfGui.lfHeight != lfDlg.lfHeight) || (hfont == FALSE) )
    {
        lfGui.lfHeight = lfDlg.lfHeight;
        lfGui.lfWidth = lfDlg.lfWidth;
        hfont=CreateFontIndirect(&lfGui);
        pDfd->bStock = FALSE;
    }
    else
        pDfd->bStock = TRUE;

    pDfd->hfontNew = hfont;
    pDfd->hfontOld = NULL;


    EnumChildWindows(hwnd, SetfontChildProc, (LPARAM)pDfd);

    // Set this so that we can delete it at termination.
    if (pDfd->bStock == FALSE)
        SetProp(hwnd, MAKEINTATOM(DLGATOM_FONTDATA), (HANDLE)pDfd);
    else
        // We won't use this if we were able to use stock font.
        LocalFree((HANDLE)pDfd);
}

void DeleteShellFont(HWND hwnd)
{
    DLGFONTDATA *pDfd;

    if (pDfd=(DLGFONTDATA *)GetProp(hwnd, MAKEINTATOM(DLGATOM_FONTDATA)))
    {
        pDfd->hfontNew = pDfd->hfontOld;
        pDfd->hfontOld = NULL;

        EnumChildWindows(hwnd, SetfontChildProc, (LPARAM)pDfd);
        RemoveProp(hwnd, MAKEINTATOM(DLGATOM_FONTDATA));

        // SetfontChildProc should return the original font handle.
        if (pDfd->hfontOld && !pDfd->bStock)
            DeleteObject(pDfd->hfontOld);

        LocalFree((HANDLE)pDfd);
    }
}

#endif


//================================================================
BOOL FTEdit_OnInitDialog(HWND hDialog, WPARAM wParam, LPARAM lParam)
{
    DWORD dwItemCnt;
    LOGFONT lf;
    PFILETYPESDIALOGINFO pFTDInfo;

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Edit: WM_INITDIALOG wParam=0x%x lParam=0x%x ", wParam, lParam);
#endif
#ifdef FEATURE_INTL
    if (GetACP() != 1252)
            SetShellFont(hDialog);
#endif

    pFTDInfo = (PFILETYPESDIALOGINFO)lParam;
    SetWindowLongPtr(hDialog, DWLP_USER, (LPARAM)pFTDInfo);
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
        if (pFTDInfo->pFTInfo->hkeyFT)
            // Get rid of old handle
            RegCloseKey(pFTDInfo->pFTInfo->hkeyFT);
        pFTDInfo->pFTInfo->hkeyFT = GetHkeyFT(pFTDInfo->szId);

#ifdef MIME
        CheckDlgButton(hDialog, IDC_FT_EDIT_CONFIRM_OPEN, FTEdit_ConfirmOpenAfterDownload(pFTDInfo));

        FTEdit_InitBrowseInPlace(hDialog, pFTDInfo);
#endif   /* MIME */

        if (FTEdit_IsExtShowable(pFTDInfo))
            CheckDlgButton(hDialog, IDC_FT_EDIT_SHOWEXT, TRUE);
        break;

    case IDC_FT_PROP_NEW:
        {
        char szTitle[256];
        if(!FTEdit_InitListViewCols(pFTDInfo->hwndLVFTEdit))
            return(FALSE);

        MLLoadStringA(IDS_ADDNEWFILETYPE, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hDialog, szTitle);

        // Make extension text and edit control visible
        pFTDInfo->hwndDocExt = GetDlgItem(hDialog, IDC_FT_EDIT_EXTTEXT);
        ShowWindow(pFTDInfo->hwndDocExt, SW_SHOW);
        pFTDInfo->hwndDocExt = GetDlgItem(hDialog, IDC_FT_EDIT_EXT);
        ShowWindow(pFTDInfo->hwndDocExt, SW_SHOW);
        SetFocus(pFTDInfo->hwndDocExt);
        *pFTDInfo->szId = '\0';
        *pFTDInfo->szIconPath = '\0';


        pFTDInfo->hwndEditDocIcon = GetDlgItem(hDialog, IDC_FT_EDIT_DOCICON);

        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_EDIT), FALSE);
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT), FALSE);
#ifdef MIME
        CheckDlgButton(hDialog, IDC_FT_EDIT_CONFIRM_OPEN, TRUE);
#endif   /* MIME */
        }
        break;
    }

    // Get the font used in the dialogue box
    pFTDInfo->hfReg = (HFONT)SendMessage( hDialog, WM_GETFONT, 0, 0L );

    if ( NULL == pFTDInfo ->hfReg )
        pFTDInfo->hfReg = GetStockObject( SYSTEM_FONT );

    // Create a bold version of if for default verbs
    GetObject( pFTDInfo->hfReg, SIZEOF(lf), &lf );
    lf.lfWeight = FW_BOLD;
    pFTDInfo->hfBold = CreateFontIndirect(&lf);
#ifdef MIME
   pFTDInfo->pFTInfo->dwMIMEFlags = 0;
   pFTDInfo->hwndContentTypeComboBox = GetDlgItem(hDialog, IDC_FT_COMBO_CONTTYPE);
   pFTDInfo->hwndDefExtensionComboBox = GetDlgItem(hDialog, IDC_FT_COMBO_DEFEXT);
   InitMIMEControls(hDialog);
#endif   /* MIME */
    return(TRUE);    // Successful initdialog
}

//================================================================
//================================================================
#if 0
VOID ChangeDefaultButtonText(HWND hDialog, DWORD dwIDS)
{
    char szStr[256];

    if(MLLoadStringA(dwIDS, szStr, sizeof(szStr)))
        SetWindowText(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT), szStr);
}
#endif
//================================================================
//================================================================
#define lpdis ((LPDRAWITEMSTRUCT)lParam)
BOOL FTEdit_OnDrawItem(PFILETYPESDIALOGINFO pFTDInfo, WPARAM wParam, LPARAM lParam)
{
    LV_ITEM LVItem;
        char szActionValue[MAX_PATH];

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Edit: WM_DRAWITEM wParam=0x%x lParam=0x%x", wParam, lParam);
#endif

    if (lpdis->CtlType == ODT_LISTVIEW)
    {
        DRAWITEMSTRUCT *lpdi = (LPDRAWITEMSTRUCT)lParam;
        PFILETYPESCOMMANDINFO pFTCInfo;

        LVItem.mask = LVIF_PARAM;
        LVItem.iItem = lpdi->itemID;
        LVItem.iSubItem = 0;
        ListView_GetItem(pFTDInfo->hwndLVFTEdit, &LVItem);  // lParam points to file type info                    pFTInfo = (PFILETYPESINFO)LVItem.lParam;
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
//            ChangeDefaultButtonText(pFTDInfo->hEditDialog, IDS_CLEARDEFAULT);
        }
        else
        {
            SelectObject(lpdi->hDC, pFTDInfo->hfReg);
//            ChangeDefaultButtonText(pFTDInfo->hEditDialog, IDS_SETDEFAULT);
        }

                StrRemoveChar(pFTCInfo->szActionValue, szActionValue, '&');
        ExtTextOut(lpdi->hDC,
            lpdi->rcItem.left,lpdi->rcItem.top,
            ETO_OPAQUE, &lpdi->rcItem,
            szActionValue, lstrlen(szActionValue),
            NULL);

        // Draw the focus rect if this is the selected or it has focus!
        if ( ( lpdi->itemState & ODS_FOCUS ) || ( lpdi->itemState & ODS_SELECTED ) )
            DrawFocusRect(lpdi->hDC, &lpdi->rcItem);

        return(TRUE);
    }
    return(FALSE);
}
//================================================================
//================================================================
#define lpmis ((LPMEASUREITEMSTRUCT)lParam)
BOOL FTEdit_OnMeasureItem(WPARAM wParam, LPARAM lParam)
{
    LOGFONT lf;

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Edit: WM_MEASUREITEM wParam=0x%x lParam=0x%x", wParam, lParam);
#endif

    if (lpmis->CtlType == ODT_LISTVIEW)
    {
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
//        hfReg = CreateFontIndirect(&lf);
//        lf.lfWeight = FW_BOLD;
//        hfBold = CreateFontIndirect(&lf);
        ((MEASUREITEMSTRUCT *)lParam)->itemHeight = lf.lfHeight;
        ((MEASUREITEMSTRUCT *)lParam)->itemHeight = 14;
        return(TRUE);
    }
    return(FALSE);
}

//================================================================
VOID FTEdit_OnNotify(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Edit: WM_NOTIFY wParam=0x%x lParam=0x%x", wParam, lParam);
#endif
    // Process ListView notifications
    switch(((LV_DISPINFO *)lParam)->hdr.code)
    {
        case NM_DBLCLK:
#ifdef FT_DEBUG
            DebugMsg(DM_TRACE, "FT Edit: WM_NOTIFY - NM_DBLCLK");
#endif
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
#ifdef FT_DEBUG
            DebugMsg(DM_TRACE, "FT Edit: WM_NOTIFY - LVN_ITEMCHANGED");
#endif

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

    }    // switch(((LV_DISPINFO *)lParam)->hdr.code)
}

//================================================================
//================================================================
BOOL FTEdit_OnOK(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    LV_ITEM LVItem;
    char szExt[MAX_PATH];
    char szDesc[MAX_PATH];

    if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
        {
        GetDlgItemText(hDialog, IDC_FT_EDIT_EXT, szExt, sizeof(szExt));

                // We need to do some cleanup here to make it work properly
                // in the cases where ther user types in something like
                // *.foo or .foo
                // This is real crude
                StrRemoveChar(szExt, NULL, '*');
        }
    else
        lstrcpy(szExt, DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0));

    // Validate file type description
    GetDlgItemText(hDialog, IDC_FT_EDIT_DESC, szDesc, sizeof(szDesc));
    if(!(*szDesc))
    {
        lstrcpy(szDesc, CharUpper((szExt[0] == '.' ? &szExt[1] : szExt)));
        if (lstrlen(szDesc)+lstrlen(c_szSpaceFile) < ARRAYSIZE(szDesc))
        {
            lstrcat(szDesc, c_szSpaceFile);
        }
    }

    // Save extension when new type is selected
    if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
    {
        if(!ValidExtension(hDialog, pFTDInfo))
            return(FALSE);
        AddExtDot(CharLower(szExt), sizeof(szExt));
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

#ifdef MIME
        FTEdit_SetConfirmOpenAfterDownload(pFTDInfo, IsDlgButtonChecked(hDialog, IDC_FT_EDIT_CONFIRM_OPEN));

        if (IsWindowEnabled(GetDlgItem(hDialog, IDC_BROWSEINPLACE)))
            FTEdit_SetBrowseInPlace(pFTDInfo, IsDlgButtonChecked(hDialog, IDC_BROWSEINPLACE));
#endif   /* MIME */

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

#ifdef MIME
    SaveFileTypeData(FTD_MIME, pFTDInfo);
#endif   /* MIME */

    // This may be overkill but for now, have it refresh the
    // windows...
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return(TRUE);
}

//================================================================
//================================================================
VOID FTEdit_OnRemove(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    char    szMsg[MAX_PATH];
    char    szTitle[MAX_PATH];
    
    // HKEY_CLASSES_ROOT\filetype\shell\action key

    MLLoadStringA(IDS_FT_MB_REMOVEACTION, szMsg, ARRAYSIZE(szMsg));
    MLLoadStringA(IDS_FT, szTitle, ARRAYSIZE(szTitle));
    
    if(ShellMessageBox(MLGetHinst(), hDialog,
        szMsg,
        szTitle,MB_YESNO | MB_ICONQUESTION) == IDNO)
        return;

    Assert(pFTDInfo->pFTInfo->hkeyFT);
    RemoveAction(pFTDInfo, pFTDInfo->pFTInfo->hkeyFT, c_szShell,
                pFTDInfo->pFTCInfo->szActionKey);

    if(ListView_GetItemCount(pFTDInfo->hwndLVFTEdit) == 0)
    {
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_EDIT), FALSE);
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hDialog, IDC_FT_EDIT_DEFAULT), FALSE);
    }

    OkToClose_NoCancel(hDialog);
    PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
}
//================================================================
//================================================================


// Helper function to query a key value, returning if the key has been queried
// and the result was non-zero.
// BUGBUG (scotth): we already have helper functions that do this.
//                  this needs cleaning

static BOOL do_query_value( HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpValue, LONG lSize )
{
    LONG err = lSize;

    err = RegQueryValue( hKey, lpSubKey, lpValue, &err );

    return ( err == ERROR_SUCCESS && *lpValue );
}


// Given a sub key attempt to find it in in the classes section of the registry:

static BOOL find_sub_key( LPCTSTR lpExt, LPCTSTR lpSubKey, LPTSTR lpValue, LONG iSize )
{
    BOOL bResult = FALSE;
    TCHAR szProgId[MAX_PATH];
    TCHAR szTemp[MAX_PATH * 3 ];
    TCHAR szCLSID[ MAX_PATH ];
    HKEY hKey = NULL;

    // Attempt to get our root key, check for a ProgId, otherwise leave it at the extension
    if ( do_query_value( HKEY_CLASSES_ROOT, lpExt, szProgId, SIZEOF(szProgId) ) )
        RegOpenKey( HKEY_CLASSES_ROOT, szProgId, &hKey );
    else
        RegOpenKey( HKEY_CLASSES_ROOT, lpExt, &hKey );

    // If we aquired the key then check for the sub-key
    if ( hKey )
    {
        bResult = do_query_value( hKey, lpSubKey, lpValue, iSize );

        // Didn't find it, so check for the CLSID and look under that
        if ( !bResult && do_query_value( hKey, c_szCLSID, szCLSID, SIZEOF(szCLSID) ) )
        {
            wsprintf( szTemp, c_szTemplateSSS, c_szCLSID, szCLSID, lpSubKey );
            bResult = do_query_value( HKEY_CLASSES_ROOT, szTemp, lpValue, iSize );
        }

        RegCloseKey( hKey );
    }

    return bResult;
}


// Handle displaying the icon picker for the givne extension, follow the same icon
// rules as the Explorer.

VOID FTEdit_OnChangeIcon(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    SHFILEINFO sfi;
    HICON hIcon;
    char szBuf[MAX_PATH-2];

    if(pFTDInfo->dwCommand == IDC_FT_PROP_NEW)
    {
        lstrcpy(sfi.szDisplayName, c_szShell2);
        sfi.iIcon = -(IDI_SYSFILE);
    }
    else
    {
        LPTSTR pszExt = (LPTSTR)DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0);

        sfi.szDisplayName[0]='\0';              // no initial string
        sfi.iIcon = 0;                          // default to first icon

        // If we have an extension then attempt to look up a suitable icon for it.
        if ( *pszExt )
        {
            // check for a DefaultIcon, if found the convert into something useful
            if ( find_sub_key( pszExt, c_szDefaultIcon, sfi.szDisplayName, SIZEOF(sfi.szDisplayName) ) )
            {
                sfi.iIcon = ParseIconLocation( sfi.szDisplayName );
            }
            else
            {
                // Otherwise take the ShellOpen command and work wit that
                find_sub_key( pszExt, c_szShellOpenCommand, sfi.szDisplayName, SIZEOF(sfi.szDisplayName) );
            }
        }
        else
        {
            // Some objects don't have extensions, therefore we must attempt to use the key
            // we were given when invoked.

            if ( do_query_value( pFTDInfo->pFTInfo->hkeyFT, c_szDefaultIcon, sfi.szDisplayName, SIZEOF(sfi.szDisplayName) ) )
                                 sfi.iIcon = ParseIconLocation( sfi.szDisplayName );

        }
    }

    if( *sfi.szDisplayName )
    {
        // Fix up the name we have so that we can display the PickIcon dlg, this includes
        // resolve the relative item, and striping arguments.  Should this fail then we
        // strip the string of arguments and pass it in, letting PickIcon do its worst!

        if ( UrlPathProcessCommand( sfi.szDisplayName, sfi.szDisplayName, SIZEOF(sfi.szDisplayName), PPCF_NODIRECTORIES ) == -1 )
        {
            PathQuoteSpaces( sfi.szDisplayName );
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

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FTEdit_OnChangeIcon RegQueryValue szIcon=%s iIndex=%d hkeyFT=0x%x",
            sfi.szDisplayName, sfi.iIcon, pFTDInfo->pFTInfo->hkeyFT);
#endif

    if (RUNNING_NT)
        MultiByteToWideChar(CP_ACP, 0, sfi.szDisplayName, -1,
                    (LPWSTR)szBuf, sizeof(szBuf));
    else
        lstrcpy(szBuf, sfi.szDisplayName);

    if(PickIconDlg(hDialog, szBuf, sizeof(szBuf), &sfi.iIcon))
    {
        if (RUNNING_NT)
            WideCharToMultiByte(CP_ACP, 0, (LPWSTR)szBuf, -1,
                    pFTDInfo->szIconPath,sizeof(szBuf), NULL, NULL);
        else
            lstrcpy(pFTDInfo->szIconPath, szBuf);

        pFTDInfo->iIconIndex = sfi.iIcon;
        hIcon = ExtractIcon(hinstCabinet, pFTDInfo->szIconPath, pFTDInfo->iIconIndex);
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
            OkToClose_NoCancel(hDialog);
            PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
        }
    }
}

//================================================================
//================================================================
VOID FTEdit_OnCommand(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Edit: WM_COMMAND wParam hi=0x%04x lo=0x%04x lParam=0x%x", HIWORD(wParam), LOWORD(wParam), lParam);
#endif

    switch(idCmd)
    {
        case IDOK:
            if(!FTEdit_OnOK(pFTDInfo, hDialog))
                break;
            // Fall through...
        case IDCANCEL:
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
                OkToClose_NoCancel(hDialog);
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
                OkToClose_NoCancel(hDialog);
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
            SetDefaultAction(pFTDInfo);    // set default action for current item

            // update list view
            ListView_RedrawItems(pFTDInfo->hwndLVFTEdit, 0, ListView_GetItemCount(pFTDInfo->hwndLVFTEdit));
            UpdateWindow(pFTDInfo->hwndLVFTEdit);

            OkToClose_NoCancel(hDialog);
            PropSheet_CancelToClose(GetParent(pFTDInfo->hPropDialog));
            break;

#ifdef MIME
      case IDC_FT_COMBO_CONTTYPE:
         switch (GET_WM_COMMAND_CMD(wParam, lParam))
         {
            case CBN_SELCHANGE:
               TRACE_OUT(("FTEdit_OnCommand(): MIME Type selection change."));
               OnContentTypeSelectionChange(hDialog);
               break;
            case CBN_EDITCHANGE:
               TRACE_OUT(("FTEdit_OnCommand(): MIME Type edit change."));
               OnContentTypeEditChange(hDialog);
               break;
            case CBN_DROPDOWN:
               TRACE_OUT(("FTEdit_OnCommand(): MIME Type drop down."));
               OnContentTypeDropDown(hDialog);
               break;
         }
         break;
      case IDC_FT_COMBO_DEFEXT:
         switch (GET_WM_COMMAND_CMD(wParam, lParam))
         {
            case CBN_DROPDOWN:
               TRACE_OUT(("FTEdit_OnCommand(): Default Ext drop down."));
               OnDefExtensionDropDown(hDialog);
               break;
         }
         break;
      case IDC_FT_EDIT_EXT:
         switch (GET_WM_COMMAND_CMD(wParam, lParam))
         {
            case EN_CHANGE:
               TRACE_OUT(("FTEdit_OnCommand(): Associated Ext change."));
               OnContentTypeEditChange(hDialog);
               break;
         }
         break;
#endif   /* MIME */
    }
}

//================================================================
//================================================================
#pragma data_seg(DATASEG_READONLY)
const static DWORD aEditFileTypesHelpIDs[] = {  // Context Help IDs
    IDC_NO_HELP_1,              NO_HELP,
    IDC_FT_EDIT_DOCICON,        IDH_FCAB_FT_EDIT_DOCICON,
    IDC_FT_EDIT_CHANGEICON,     IDH_FCAB_FT_EDIT_CHANGEICON,
    IDC_FT_EDIT_DESCTEXT,       IDH_FCAB_FT_EDIT_DESC,
    IDC_FT_EDIT_DESC,           IDH_FCAB_FT_EDIT_DESC,
    IDC_FT_EDIT_EXTTEXT,        IDH_FCAB_FT_EDIT_EXT,
    IDC_FT_EDIT_EXT,            IDH_FCAB_FT_EDIT_EXT,
#ifdef MIME
    IDC_FT_COMBO_CONTTYPETEXT,  IDH_NEW_FILETYPE_CONTENT_TYPE,
    IDC_FT_COMBO_CONTTYPE,      IDH_NEW_FILETYPE_CONTENT_TYPE,
    IDC_FT_COMBO_DEFEXTTEXT,    IDH_NEWFILETYPE_DEFAULT_EXT,
    IDC_FT_COMBO_DEFEXT,        IDH_NEWFILETYPE_DEFAULT_EXT,
    IDC_FT_EDIT_CONFIRM_OPEN,   IDH_FILETYPE_CONFIRM_OPEN,
    IDC_BROWSEINPLACE,          NO_HELP,
#endif   /* MIME */
    IDC_FT_EDIT_LV_CMDSTEXT,    IDH_FCAB_FT_EDIT_LV_CMDS,
    IDC_FT_EDIT_LV_CMDS,        IDH_FCAB_FT_EDIT_LV_CMDS,
    IDC_FT_EDIT_DEFAULT,        IDH_FCAB_FT_EDIT_DEFAULT,
    IDC_FT_EDIT_NEW,            IDH_FCAB_FT_EDIT_NEW,
    IDC_FT_EDIT_EDIT,           IDH_FCAB_FT_EDIT_EDIT,
    IDC_FT_EDIT_REMOVE,         IDH_FCAB_FT_EDIT_REMOVE,
    IDC_FT_EDIT_SHOWEXT,        IDH_FCAB_FT_EDIT_SHOWEXT,
    0,                          0
};
#pragma data_seg()
//================================================================
//================================================================

INT_PTR CALLBACK FTEdit_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    PFILETYPESDIALOGINFO pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hDialog, DWLP_USER);

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
#ifdef MIME
                    FT_GetHelpFileFromControl((HWND)(((LPHELPINFO)lParam)->hItemHandle)),
#else
                    NULL,
#endif   /* MIME */
                     HELP_WM_HELP, (DWORD_PTR)(LPSTR)aEditFileTypesHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
        {
            POINT pt;

            if ((int)SendMessage(hDialog, WM_NCHITTEST, 0, lParam) != HTCLIENT)
               return FALSE;   // don't process it

            LPARAM_TO_POINT(lParam, pt);
            EVAL(ScreenToClient(hDialog, &pt));

            SHWinHelpOnDemandWrap((HWND)wParam,
#ifdef MIME
                    FT_GetHelpFileFromControl(ChildWindowFromPoint(hDialog, pt)),
#else
                    NULL,
#endif   /* MIME */
                    HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)aEditFileTypesHelpIDs);

            return TRUE;
        }

        case WM_COMMAND:
            FTEdit_OnCommand(pFTDInfo, hDialog, wParam, lParam);
            break;

        case WM_CTRL_SETFOCUS:
            SetFocus((HWND)lParam);
            SendMessage((HWND)lParam, EM_SETSEL, (WPARAM)0, (LPARAM)MAKELPARAM(0, -1));
            break;
#ifdef FEATURE_INTL

        case WM_DESTROY:
            if (GetACP() != 1252)
                DeleteShellFont(hDialog);
            break;
#endif
    }
    return(FALSE);
}

//================================================================
//================================================================
VOID FTCmd_OnInitDialog(HWND hDialog, WPARAM wParam, LPARAM lParam)
{
    char szPath[MAX_PATH+6];    // 6 = "\shell"
    char szAction[MAX_PATH];
    DWORD dwAction;
    DWORD dwPath;
    char szBuf[256];
    int iLen;
    LONG err;
    PFILETYPESDIALOGINFO pFTDInfo;

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Cmd: WM_INITDIALOG wParam=0x%x lParam=0x%x ", wParam, lParam);
#endif

    pFTDInfo = (PFILETYPESDIALOGINFO)lParam;
    SetWindowLongPtr(hDialog, DWLP_USER, (LPARAM)pFTDInfo);
    pFTDInfo->hCmdDialog = hDialog;

    // Limit the extension length
    SendDlgItemMessage(hDialog, IDC_FT_EDIT_EXT, EM_LIMITTEXT, (WPARAM)PATH_CCH_EXT, 0);

    if(pFTDInfo->dwCommandEdit == IDC_FT_EDIT_EDIT)
    {
        // Set window title to show file type description we are editing
        if(MLLoadStringA(IDS_FT_EDITTITLE, szBuf, sizeof(szBuf)))
        {
            lstrcpy(szPath, szBuf);
            GetDlgItemText( GetParent(hDialog), IDC_FT_EDIT_DESC, szBuf, SIZEOF(szBuf) );       // ensures correct title, even if edited!
            lstrcat(szPath, szBuf);
            SetWindowText(hDialog, szPath);
        }

        // Set application field to executable used to perform action shown above
        dwPath = sizeof(pFTDInfo->pFTCInfo->szCommand);
        VerbToExe(pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->pFTCInfo->szActionKey,
            pFTDInfo->pFTCInfo->szCommand, &dwPath);

        // Remove %1 if at end of string
        lstrcpy(szBuf, c_szSpPercentOne);    // BUGBUG - StrCmpN modifies LPCSTR's even though that is how its params are declared
        iLen = lstrlen(c_szSpPercentOne);
        if(StrCmpN(&pFTDInfo->pFTCInfo->szCommand[lstrlen(pFTDInfo->pFTCInfo->szCommand)-iLen],
                szBuf, iLen) == 0)
            pFTDInfo->pFTCInfo->szCommand[lstrlen(pFTDInfo->pFTCInfo->szCommand)-iLen] = '\0';

        SetDlgItemText(hDialog, IDC_FT_CMD_EXE, pFTDInfo->pFTCInfo->szCommand);

        // Set command field to action verb keys value
        wsprintf(szPath, c_szTemplateSS, c_szShell, pFTDInfo->pFTCInfo->szActionKey);
        Assert(pFTDInfo->pFTInfo->hkeyFT);
        dwAction = sizeof(szAction);
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

//    // Add items to action combo box
//    SendMessage(GetDlgItem(hDialog, IDC_FT_CMD_ACTION), CB_ADDSTRING, 0, (LPARAM)c_szOpenVerb);
//    SendMessage(GetDlgItem(hDialog, IDC_FT_CMD_ACTION), CB_ADDSTRING, 0, (LPARAM)c_szPrintVerb);

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

//====================================================================
//====================================================================
LONG DeleteDDEKeys(LPCSTR pszKey)
{
    char szBuf[MAX_PATH+MAXEXTSIZE+6+8]; // 6 = "\shell", 8 = "\ddeexec"

    // Delete DDEApp keys
    wsprintf(szBuf, c_szTemplateSS, pszKey, c_szDDEExec);
    return(RegDeleteKey(HKEY_CLASSES_ROOT, szBuf));
}

//================================================================
//================================================================
BOOL FTCmd_OnOK(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{
    char szPath[MAX_PATH];
    char szKey[MAX_PATH+MAXEXTSIZE+7]; // 7 = "\shell\"
    char szAction[MAX_PATH];

    // Validate fields
    if(!ActionIsEntered(hDialog, TRUE))
        return(FALSE);
    if(!ActionExeIsValid(hDialog, TRUE))
        return(FALSE);

    // Get and save edit command dialog text
    GetDlgItemText(hDialog, IDC_FT_CMD_ACTION, szAction, sizeof(szAction));
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
        lstrcpy(szPath, pFTDInfo->pFTCInfo->szCommand);    // save prev val for check below
        GetDlgItemText(hDialog, IDC_FT_CMD_EXE,
            pFTDInfo->pFTCInfo->szCommand, sizeof(pFTDInfo->pFTCInfo->szCommand));

        // Add %1 to end if not already part of command
        lstrcpy(szAction, &c_szSpPercentOne[1]);   // borrow szAction; StrStr mods param 2
        if(StrStr(pFTDInfo->pFTCInfo->szCommand, szAction) == NULL
             && lstrlen(pFTDInfo->pFTCInfo->szCommand) + lstrlen(c_szSpPercentOne)
                      < ARRAYSIZE(pFTDInfo->pFTCInfo->szCommand) )
        {
            lstrcat(pFTDInfo->pFTCInfo->szCommand, c_szSpPercentOne);
        }

        // Get DDE field values
        if(IsDlgButtonChecked(hDialog, IDC_FT_CMD_USEDDE))
        {
            GetDlgItemText(hDialog, IDC_FT_CMD_DDEMSG,
                pFTDInfo->pFTCInfo->szDDEMsg, sizeof(pFTDInfo->pFTCInfo->szDDEMsg));
            GetDlgItemText(hDialog, IDC_FT_CMD_DDEAPP,
                pFTDInfo->pFTCInfo->szDDEApp, sizeof(pFTDInfo->pFTCInfo->szDDEApp));
            GetDlgItemText(hDialog, IDC_FT_CMD_DDEAPPNOT,
                pFTDInfo->pFTCInfo->szDDEAppNot, sizeof(pFTDInfo->pFTCInfo->szDDEAppNot));
            GetDlgItemText(hDialog, IDC_FT_CMD_DDETOPIC,
                pFTDInfo->pFTCInfo->szDDETopic, sizeof(pFTDInfo->pFTCInfo->szDDETopic));
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
                    hIcon = ImageList_ExtractIcon(hinstCabinet, s_himlSysLarge, iImageIndex);
                }
                else
                {
                    // special cases like folder and drive
                    if((hIcon = GetDefaultIcon(&pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->szId, SHGFI_LARGEICON)) == (HICON)NULL)
                    {
                        // use default shell icon in case above calls fail to find an icon
                        UINT iIndex = Shell_GetCachedImageIndex(c_szShell2, II_DOCNOASSOC, 0);
                        hIcon = ImageList_ExtractIcon(hinstCabinet, s_himlSysLarge, iIndex);
                    }
                }
                SendMessage(pFTDInfo->hwndEditDocIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
            }
        }
    }

    return(TRUE);
}

//================================================================
//================================================================
VOID FTCmd_OnBrowse(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    /*
     * The problem with NT is that this function is UNICODE-ONLY (see below),
     * and so we must convert all our strings to Unicode before calling, then
     * convert the szPath back to ANSI upon return.
     *
     * BOOL WINAPI GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cchFilePath,
     *   LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle)
     *
     */
    if (RUNNING_NT)
    {
        WCHAR szuPath[MAX_PATH];
        WCHAR szuTitle[80];
        WCHAR szuExe[16];
        WCHAR szuFilters[MAX_PATH];
        LPWSTR psz;

        szuPath[0] = szuTitle[0] = szuExe[0] = szuFilters[0]= 0;
        MLLoadStringW(IDS_OPENAS, szuTitle, 80);
        MLLoadStringW(IDS_EXE, szuExe, 16);
        MLLoadStringW(IDS_PROGRAMSFILTER, szuFilters, MAX_PATH);
        /* hack up the array... */
        psz = szuFilters;
        while (*psz)
        {
            if (*psz == (WCHAR)('#'))
                *psz = (WCHAR)('\0');
            psz++;
        }

        if (GetFileNameFromBrowse(hDialog, (char *)szuPath, MAX_PATH, NULL,
            (LPCSTR)szuExe, (LPCSTR)szuFilters, (LPCSTR)szuTitle))
            {
                /* Specifically UNICODE */
                PathQuoteSpaces((LPSTR)szuPath);
                SetDlgItemTextW(hDialog, IDC_FT_CMD_EXE, szuPath);
            }
    }
    else
    {
        /* leave this here for running on Win95 */
        char szPath[MAX_PATH];
        char szTitle[MAX_PATH];
        char szExe[16];
        char szFilters[MAX_PATH];
        char *psz;

        szPath[0] = szTitle[0] = szExe[0] = szFilters[0]= 0;
        MLLoadStringA(IDS_OPENAS, szTitle, sizeof(szTitle));
        MLLoadStringA(IDS_EXE, szExe, sizeof(szExe));
        MLLoadStringA(IDS_PROGRAMSFILTER, szFilters, sizeof(szFilters));
        psz = szFilters;
        while (*psz)
        {
            if (*psz == (char)('#'))
                *psz = (char)('\0');
            psz++;
        }

        if (GetFileNameFromBrowse(hDialog, szPath, sizeof(szPath), NULL,
            szExe, szFilters, szTitle))
            {
                PathQuoteSpaces(szPath);
                SetDlgItemText(hDialog, IDC_FT_CMD_EXE, szPath);
            }
    }
}

//================================================================
//================================================================
VOID FTCmd_OnCommand(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog, WPARAM wParam, LPARAM lParam)
{

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT Cmd: WM_COMMAND wParam 0x%04x 0x%04x", HIWORD(wParam), LOWORD(wParam));
#endif

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
#pragma data_seg(DATASEG_READONLY)
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
#pragma data_seg()

//================================================================
//================================================================
INT_PTR CALLBACK FTCmd_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    PFILETYPESDIALOGINFO pFTDInfo = (PFILETYPESDIALOGINFO)GetWindowLongPtr(hDialog, DWLP_USER);

    switch(message)
    {
        case WM_INITDIALOG:
            FTCmd_OnInitDialog(hDialog, wParam, lParam);
            return(TRUE);

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR) aEditCommandHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
                        if ((int)SendMessage(hDialog, WM_NCHITTEST, 0, lParam) != HTCLIENT)
                            return FALSE;   // don't process it
            SHWinHelpOnDemandWrap((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) aEditCommandHelpIDs);
            return TRUE;

        case WM_COMMAND:
            FTCmd_OnCommand(pFTDInfo, hDialog, wParam, lParam);
            break;

        case WM_CTRL_SETFOCUS:
            SetFocus((HWND)lParam);
            SendMessage((HWND)lParam, EM_SETSEL, (WPARAM)0, (LPARAM)MAKELPARAM(0, -1));
            break;
    }
    return(FALSE);
}

//================================================================
//================================================================
VOID OkToClose_NoCancel(HWND hDialog)
{
    char szStr1[256];

    if(MLLoadStringA(IDS_FT_CLOSE, szStr1, sizeof(szStr1)))
    {
        SetWindowText(GetDlgItem(hDialog, IDOK), szStr1);
        EnableWindow(GetDlgItem(hDialog, IDCANCEL), FALSE);
    }
}

//================================================================
//================================================================
HICON GetDocIcon(PFILETYPESDIALOGINFO pFTDInfo, LPSTR lpszStr)
{
    SHFILEINFO sfi;
    HICON hIcon = (HICON)NULL;
    int iImageIndex;

    if(pFTDInfo->pFTInfo->dwAttributes & FTA_HasExtension)
    {
        if(*lpszStr == '.')  // is an extension
        {
            if(SHGetFileInfo((LPSTR)lpszStr, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES))
                hIcon = sfi.hIcon;
        }
        else
        {
            // get simulated doc icon if from exe name
            iImageIndex = Shell_GetCachedImageIndex(lpszStr, 0, GIL_SIMULATEDOC);
            hIcon = ImageList_ExtractIcon(hinstCabinet, s_himlSysLarge, iImageIndex);
        }
    }
    else    // special case for folder, drive etc.
    {
        if((hIcon = GetDefaultIcon(&pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->szId, SHGFI_LARGEICON)) == (HICON)NULL)
        {
            // use default shell icon in case above calls fail to find an icon
            UINT iIndex = Shell_GetCachedImageIndex(c_szShell2, II_DOCNOASSOC, 0);
            hIcon = ImageList_ExtractIcon(hinstCabinet, s_himlSysLarge, iIndex);
        }
    }

    return(hIcon);
}

//================================================================
//================================================================
VOID DisplayDocObjects(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    LV_ITEM LVItem;
    char szFile[MAX_PATH];
    int iCnt;
    int i;

    // Display extensions in read-only edit control
    iCnt = DPA_GetPtrCount(pFTDInfo->pFTInfo->hDPAExt);
    *szFile = '\0';
    for(i = 0; (i < iCnt) && (lstrlen(szFile) < MAX_PATH); i++)
    {
        if(*(LPSTR)(DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i)))
        {
                        // Make sure we have enough room left in our string...
                        if ((lstrlen(szFile) +
                                lstrlen((LPSTR)DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i)))
                                >= (MAX_PATH - 2))
                            break;
            lstrcat(szFile, (LPSTR)DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i) + 1);
            lstrcat(szFile, c_szSpace);
        }
    }
    SetDlgItemText(hDialog, IDC_FT_PROP_DOCEXTRO, CharUpper(szFile));

#ifdef MIME
   // Display MIME type in read-only edit control.
   if (IsListOfExtensions(pFTDInfo->pFTInfo->hDPAExt))
      FindMIMETypeOfExtensionList(pFTDInfo->pFTInfo->hDPAExt, szFile, sizeof(szFile));
   else
      *szFile = '\0';
   SetDlgItemText(hDialog, IDC_FT_PROP_CONTTYPERO, szFile);
   lstrcpy(pFTDInfo->pFTInfo->szOriginalMIMEType, szFile);
#endif   /* MIME */

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
        PostMessage(pFTDInfo->hwndDocIcon, STM_SETIMAGE, IMAGE_ICON,
                (LPARAM)pFTDInfo->pFTInfo->hIconDoc);
}

//================================================================
//================================================================
VOID DisplayOpensWithObjects(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog)
{
    char szFile[MAX_PATH];
    char szFullPath[MAX_PATH];
    SHFILEINFO sfi;

    // Get default action's executable
    //    Search order:
    //    1. FTID\[value-of-FTID\shell-key]\command
    //    2. FTID\open\command
    //    3. FTID\[1st-FTID\shell-subkey]\command
    ExtToShellCommand(pFTDInfo->pFTInfo->hkeyFT, szFile, sizeof(szFile));

    if(*szFile)
    {
        int cbT = lstrlen(c_szExefileOpenCommand);
        if (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                szFile, cbT, c_szExefileOpenCommand, cbT) == 2)
        {
            // handle types like exefile, comfile, & batfile that don't have exe

            // Get open icon if not already gotten
            pFTDInfo->pFTInfo->hIconOpen = (HICON)NULL;
            PostMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)NULL);
            MLLoadStringA(IDS_FT_EXEFILE, szFullPath, sizeof(szFullPath));
        }
        else
        {
            if ( UrlPathProcessCommand( szFile, szFile, MAX_PATH, PPCF_NODIRECTORIES ) == -1 )
            {
                PathRemoveArgs(szFile);
                PathRemoveBlanks(szFile);
                PathUnquoteSpaces(szFile);

                if (PathIsRelative(szFile))
                  PathFindOnPath(szFile, NULL);
            }

            lstrcpy(szFullPath, szFile);

            // Get open icon if not already gotten
            if(pFTDInfo->pFTInfo->hIconOpen == (HICON)NULL)
            {
                sfi.hIcon = 0;
#ifdef MIME
            TRACE_OUT(("DisplayOpensWithObjects(): Retrieving icon for %s.",
                       szFullPath));
#endif   /* MIME */
                if(SHGetFileInfo(szFullPath, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES))
                {
                    if(sfi.hIcon != (HICON)NULL)
                        pFTDInfo->pFTInfo->hIconOpen = sfi.hIcon;
                    else
                    {
                        UINT iIndex = Shell_GetCachedImageIndex(c_szShell2, II_DOCNOASSOC, 0);
                        pFTDInfo->pFTInfo->hIconOpen = ImageList_ExtractIcon(hinstCabinet, s_himlSysLarge, iIndex);
                    }
                }
            }
        }

        // Display open icon
        if(pFTDInfo->pFTInfo->hIconOpen != (HICON)NULL)
            PostMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pFTDInfo->pFTInfo->hIconOpen);

        // Display exe name
        lstrcpy(szFile, PathFindFileName(szFullPath));    // Strip off the path
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

//================================================================
//================================================================
BOOL ValidExtension(HWND hDialog, PFILETYPESDIALOGINFO pFTDInfo)
{
    BOOL bRC = TRUE;
    char szExt[MAX_PATH];
    char szId[MAX_PATH];
    char szBuf[MAX_PATH];
    char szStr1[256];
    char szStr2[256];
    DWORD dwId;
    HWND hwndButton;
    int iLength;

    // On new types verify that the extension is not already in use.
    GetDlgItemText(hDialog, IDC_FT_EDIT_EXT, szExt, sizeof(szExt));

    iLength = lstrlen(szExt);

    if(iLength != 0 && iLength < MAXEXTSIZE)
    {
        AddExtDot(szExt, sizeof(szExt));

        dwId = sizeof(szId);
        *szId = '\0';
        if((RegQueryValue(HKEY_CLASSES_ROOT, szExt, szId, &dwId) == ERROR_SUCCESS) && *szId)
        {
            // Disable OK button
            hwndButton = GetDlgItem(hDialog, IDOK);
            EnableWindow(hwndButton, FALSE);

            // Tell user that this extension is already in use
            *szBuf = '\0';
            *szStr2 = '\0';
            if(MLLoadStringA(IDS_FT_MB_EXTTEXT, szStr1, sizeof(szStr1)))
            {
                if(MLLoadStringA(IDS_FT, szStr2, sizeof(szStr2)))
                {
                    if(lstrlen(szStr1) + lstrlen(szExt) + lstrlen(szId) < sizeof(szBuf))
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
            LPSTR pszExt = szExt;

            if(*pszExt == '.')
                pszExt++;   // remove dot

            // Create unique file type id
            lstrcpy(pFTDInfo->szId, pszExt);
            lstrcat(pFTDInfo->szId, c_szFile);

            while(RegOpenKey(HKEY_CLASSES_ROOT, pFTDInfo->szId, &hk) == ERROR_SUCCESS)
            {
                RegCloseKey(hk);
#pragma data_seg(".text", "CODE")
                wsprintf(pFTDInfo->szId, "%s%s%02d",
                    pszExt, c_szFile, iCnt);
#pragma data_seg()
                iCnt++;
            }
            EnableWindow(GetDlgItem(pFTDInfo->hEditDialog, IDC_FT_EDIT_NEW), TRUE);
        }
    }
    else
    {
        // Tell the use that an extension is required
        *szBuf = '\0';
        *szStr2 = '\0';
        if(MLLoadStringA(IDS_FT_MB_NOEXT, szStr1, sizeof(szStr1)))
        {
            if(MLLoadStringA(IDS_FT, szStr2, sizeof(szStr2)))
            {
                if(lstrlen(szStr1) + lstrlen(szExt) + lstrlen(szId) < sizeof(szBuf))
                    wsprintf(szBuf, szStr1, szExt, szId);
            }
        }
        MessageBox(hDialog, szBuf, szStr2, MB_OK | MB_ICONSTOP);
        PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0, (LPARAM)GetDlgItem(hDialog, IDC_FT_EDIT_EXT));
        bRC = FALSE;
    }

    return(bRC);
}

//================================================================
//================================================================
BOOL ActionIsEntered(HWND hDialog, BOOL bMBoxFlag)
{
    BOOL bRC = TRUE;
    char szAction[MAX_PATH];

    // Check for value
    if(!GetDlgItemText(hDialog, IDC_FT_CMD_ACTION, szAction, sizeof(szAction)))
    {
        if(bMBoxFlag)
        {
            // Tell user that this exe is invalid
            ShellMessageBox(hinstCabinet, hDialog,
                MAKEINTRESOURCE(IDS_FT_MB_NOACTION),
                MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP);
            PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0,
                (LPARAM)GetDlgItem(hDialog, IDC_FT_CMD_ACTION));
        }
        bRC = FALSE;
    }
    return(bRC);
}

//================================================================
//================================================================
BOOL ActionExeIsValid(HWND hDialog, BOOL bMBoxFlag)
{
    BOOL bRC = TRUE;
    char szPath[MAX_PATH];
        char szFileName[MAX_PATH];

    // Check for valid exe
    GetDlgItemText(hDialog, IDC_FT_CMD_EXE,    szPath, sizeof(szPath));
    PathRemoveArgs(szPath);
        PathUnquoteSpaces(szPath);
        lstrcpy(szFileName, PathFindFileName(szPath));
    if(!(*szPath) || (!(PathIsExe(szPath))) ||
        ((!(PathFileExists(szPath))) && (!(PathFindOnPath(szFileName, NULL)))))
    {
        if(bMBoxFlag)
        {
            // Tell user that this exe is invalid
            ShellMessageBox(hinstCabinet, hDialog, MAKEINTRESOURCE(IDS_FT_MB_EXETEXT),
                MAKEINTRESOURCE(IDS_FT),MB_OK | MB_ICONSTOP);
            PostMessage(hDialog, WM_CTRL_SETFOCUS, (WPARAM)0,
                (LPARAM)GetDlgItem(hDialog, IDC_FT_CMD_EXE));
        }
        bRC = FALSE;
    }
    return(bRC);
}

//================================================================
//================================================================
BOOL FT_InitListViewCols(HWND hwndLV)
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

//================================================================
//================================================================
BOOL FTEdit_InitListViewCols(HWND hwndLV)
{
    LV_COLUMN col;
    RECT rc;

    SetWindowLong(hwndLV, GWL_EXSTYLE,
        GetWindowLong(hwndLV, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

    // Insert one column
    GetClientRect(hwndLV, &rc);
    ZeroMemory(&col, sizeof(LV_COLUMN));
    col.mask = LVCF_FMT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = rc.right - GetSystemMetrics(SM_CXBORDER);
    if(ListView_InsertColumn(hwndLV, 0, &col) == (-1))
        return(FALSE);

    return(TRUE);
}

//================================================================
//================================================================
void CALLBACK FTListViewEnumItems(PFILETYPESDIALOGINFO pFTDInfo, int i, int iEnd, HANDLE *pfShouldLive)
{
    LV_ITEM item;

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FTListViewThread created.");
#endif

    item.iSubItem = 0;
    item.mask = LVIF_IMAGE;        // This should be the only slow part

    if (iEnd == -1 ) {
        iEnd = ListView_GetItemCount(pFTDInfo->hwndLVFT);
    }

    for (; (!pfShouldLive || *pfShouldLive) && i < iEnd; i++)
    {
        item.iItem = i;
        ListView_GetItem(pFTDInfo->hwndLVFT, &item);
    }
}

//================================================================
//================================================================
DWORD CALLBACK FTListViewThread(PFILETYPESDIALOGINFO pFTDInfo)
{
    HANDLE hThread = pFTDInfo->hThread;
    FTListViewEnumItems(pFTDInfo, 10, -1, &pFTDInfo->hThread);
    CloseHandle(hThread);
    pFTDInfo->hThread = 0;
    return 0;
}

//================================================================
//================================================================
VOID CreateListViewThread(PFILETYPESDIALOGINFO pFTDInfo)
{
    // Create background thread to force list view to draw items
    DWORD idThread;

    if (pFTDInfo->hThread)
        return;

    pFTDInfo->hThread = CreateThread(NULL, 0, FTListViewThread, pFTDInfo, 0, &idThread);
    if(pFTDInfo->hThread)
        SetThreadPriority(pFTDInfo->hThread, THREAD_PRIORITY_BELOW_NORMAL);
}

//================================================================
//================================================================
BOOL FT_InitListView(PFILETYPESDIALOGINFO pFTDInfo)
{
    DWORD dwSubKey = 0;
    char szDesc[MAX_PATH];
    char szClass[MAX_PATH];
    char szClassesKey[MAX_PATH];    // string containing the classes key
    char szId[MAX_PATH];
    char szShellCommandValue[MAX_PATH];
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

    if((pFTDInfo->himlFT = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), TRUE, 0, 8)) == (HIMAGELIST)NULL)
        return(FALSE);
    ListView_SetImageList(pFTDInfo->hwndLVFT, pFTDInfo->himlFT, LVSIL_SMALL);

    // Enumerate extensions from registry to get file types
    dwClassesKey = sizeof(szClassesKey);
    dwClass = sizeof(szClass);
    while(RegEnumKeyEx(HKEY_CLASSES_ROOT, dwSubKey, szClassesKey, &dwClassesKey,
            NULL, szClass, &dwClass, &ftLastWrite) != ERROR_NO_MORE_ITEMS)
    {
        *szId = '\0';
        dwAttributes = 0;

        // find the file type identifier and description from the extension
        if(*szClassesKey == '.')
        {
            dwName = sizeof(szDesc);
            dwId = sizeof(szId);
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
                    err = sizeof(szShellCommandValue);
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
                dwName = sizeof(szDesc);
                err = RegQueryValue(hkeyFT, NULL, szDesc, &dwName);
                if(err != ERROR_SUCCESS || !*szDesc)
                    lstrcpy(szDesc, szClassesKey);
                *szClassesKey = '\0';
            }
        }

#ifdef FT_DEBUG
        DebugMsg(DM_TRACE, "FT RegEnum HKCR szClassKey=%s szId=%s dwAttributes=%d",    szClassesKey, szId, dwAttributes);
#endif

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
        dwClassesKey = sizeof(szClassesKey);
        dwClass = sizeof(szClass);
    }

    ListView_SortItems(pFTDInfo->hwndLVFT, NULL, 0);
    FT_MergeDuplicates(pFTDInfo->hwndLVFT);

    FTListViewEnumItems(pFTDInfo, 0, 10, NULL);
    CreateListViewThread(pFTDInfo);

    return(bRC);
}

//================================================================
//================================================================
int FTEdit_InitListView(PFILETYPESDIALOGINFO pFTDInfo)
{
    char szClass[MAX_PATH];
    char szAction[MAX_PATH];
    DWORD dwClass;
    DWORD dwAction;
    int iSubKey;
    FILETIME ftLastWrite;
    HKEY hk;

    // See if we have a default action verb
    iSubKey = sizeof(pFTDInfo->pFTInfo->szDefaultAction);
    DefaultAction(pFTDInfo->pFTInfo->hkeyFT, pFTDInfo->pFTInfo->szDefaultAction, &iSubKey);

    // Enumerate action verbs
    iSubKey = 0;
    if(RegOpenKeyEx(pFTDInfo->pFTInfo->hkeyFT, c_szShell, 0,
                    KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS)
    {
        dwClass = sizeof(szClass);
        dwAction = sizeof(szAction);
        // add verbs to list view
        while(RegEnumKeyEx(hk, iSubKey, szAction, &dwAction, NULL, szClass,
                           &dwClass, &ftLastWrite) == ERROR_SUCCESS)
        {
            if(!FTEdit_AddInfoToLV(pFTDInfo, szAction, NULL, pFTDInfo->pFTInfo->szId, hk))
            {
                iSubKey = (-1);
                break;
            }

            dwClass = sizeof(szClass);
            dwAction = sizeof(szAction);
            iSubKey++;
        }
        RegCloseKey(hk);
    }

    return(iSubKey);
}

//================================================================
//================================================================
BOOL IsIconPerInstance(HKEY hkeyFT)
{
    LONG err;
    char szDefaultIcon[MAX_PATH];
    BOOL bRC = FALSE;

    Assert(hkeyFT != NULL);

    err = sizeof(szDefaultIcon);
    err = RegQueryValue(hkeyFT, c_szDefaultIcon, szDefaultIcon, &err);
    if (err == ERROR_SUCCESS && *szDefaultIcon)
    {
        int cbT = lstrlen(c_szSpPercentOne) - 1;
        if (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                szDefaultIcon, cbT, &c_szSpPercentOne[1], cbT) == 2)
            bRC = TRUE;
    }
    return(bRC);
}

//================================================================
//================================================================
BOOL HasIconHandler(HKEY hkeyFT)
{
    char szBuf[MAX_PATH];
    DWORD dwBuf;

    Assert(hkeyFT != NULL);

    // Don't allow icon to be changed if type has an icon handler
    dwBuf = sizeof(szBuf);
    return(RegQueryValue(hkeyFT, c_szShellexIconHandler, szBuf, &dwBuf) == ERROR_SUCCESS);
}

//================================================================
//================================================================
BOOL FT_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, HKEY hkeyFT, LPSTR szExt,
                    LPSTR szDesc, LPSTR szId, DWORD dwAttributes)
{
    BOOL bRC = FALSE;
    LV_ITEM LVItem;
    LPSTR pszExt;

    Assert(hkeyFT != (HKEY)NULL);

    if((pFTDInfo->pFTInfo = LocalAlloc(GPTR, sizeof(FILETYPESINFO))) != NULL)
    {
        // create dynamic pointer array for FILETYPESINFO dpaExt member
        if((pFTDInfo->pFTInfo->hDPAExt = DPA_Create(4)) != (HDPA)NULL)    
        {
            if((pszExt = LocalAlloc(GPTR, lstrlen(szExt)+1)) != NULL)
            {
                lstrcpy(pszExt, szExt);
                if(DPA_InsertPtr(pFTDInfo->pFTInfo->hDPAExt, 0x7FFF, (LPVOID)pszExt) == 0)
                {
                    lstrcpy(pFTDInfo->pFTInfo->szDesc, szDesc);
                    lstrcpy(pFTDInfo->pFTInfo->szId, szId);
                    pFTDInfo->pFTInfo->dwAttributes = dwAttributes;
                    if(HasIconHandler(hkeyFT) || IsIconPerInstance(hkeyFT))
                        pFTDInfo->pFTInfo->dwAttributes |= FTA_NoEditIcon;
                    pFTDInfo->pFTInfo->hkeyFT = hkeyFT;

                    LVItem.mask        = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                    LVItem.iItem       = 0x7FFF;
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

//================================================================
//================================================================
BOOL FTEdit_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, LPSTR szActionKey,
        LPSTR szActionValue, LPSTR szId, HKEY hk)
{
    BOOL bRC = FALSE;
    int iIndex = 0;
    LV_ITEM LVItem;

    if((pFTDInfo->pFTCInfo = LocalAlloc(LPTR, sizeof(FILETYPESCOMMANDINFO))) != NULL)
    {
        lstrcpy(pFTDInfo->pFTCInfo->szId, szId);

                if (szActionKey)
                {
            lstrcpy(pFTDInfo->pFTCInfo->szActionKey, szActionKey);
            lstrcpy(pFTDInfo->pFTCInfo->szActionValue, szActionKey);

                    if (hk != NULL)
                    {
                        DWORD dwSize;
                        char szTemp[MAX_PATH];

                        // See if there is nice text for the action...
                        dwSize = sizeof(szTemp);
                        if ((RegQueryValue(hk, szActionKey, szTemp, &dwSize) == ERROR_SUCCESS)
                                && (dwSize > 1))
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
                    LPSTR pszT = StrChr(szActionValue, '=');

                    if (pszT)
                    {
                        *pszT++ = '\0';
                  StrRemoveChar(szActionValue, pFTDInfo->pFTCInfo->szActionKey, '&');
                 lstrcpy(szActionValue, pszT);
                    }
                    else
                    {
                        // We want to remove the & of the command as well as convert blanks into _s
                        // as default command processing has problems with processing of blanks
                 StrRemoveChar(szActionValue, pFTDInfo->pFTCInfo->szActionKey, '&');
                        for (pszT = pFTDInfo->pFTCInfo->szActionKey; *pszT; pszT = CharNext(pszT))
                        {
                            if (*pszT == ' ')
                                *pszT = '_';
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

//================================================================
//================================================================
VOID AddExtDot(LPSTR pszExt, UINT iExt)
{
    UINT iLength;

    PathRemoveBlanks(pszExt);       // remove 1st and last blank
    StrRemoveChar(pszExt, NULL, TEXT('.')); // remove all dots

    iLength = lstrlen(pszExt);      // How much left?
    if (iLength < iExt-1)
    {
        hmemcpy(pszExt+1, pszExt, (lstrlen(pszExt)+1)*SIZEOF(TCHAR)); // make room for dot
        *pszExt = TEXT('.');    // insert dot
    }
}

//================================================================
//================================================================
DWORD GetFileTypeAttributes(HKEY hkeyFT)
{
    LONG err;
    DWORD dwType;
    DWORD dwAttributeValue = 0;
    DWORD dwAttributeSize;

    if (hkeyFT == NULL)
        return 0;

    dwAttributeSize = sizeof(dwAttributeValue);
    err = RegQueryValueEx(hkeyFT, (LPTSTR)c_szEditFlags, NULL, &dwType, (LPBYTE)&dwAttributeValue, &dwAttributeSize);

    if (err != ERROR_SUCCESS ||
        dwType != REG_BINARY ||
        dwAttributeSize != sizeof(dwAttributeValue))
    {
        dwAttributeValue = 0;
    }

    return(dwAttributeValue);
}

//================================================================
//================================================================
DWORD SetVerbAttributes(HKEY hkeyFT, LPSTR pszVerb, DWORD dwAttributes)
{
    HKEY hk;
    LONG err;
    char szVerbKey[MAX_PATH+6];    // 6 = "\shell"
    DWORD dwSize;

    wsprintf(szVerbKey, c_szTemplateSS, c_szShell, pszVerb);
    if((hkeyFT != NULL) &&
        (RegOpenKeyEx(hkeyFT, szVerbKey, 0, KEY_SET_VALUE, &hk) == ERROR_SUCCESS))
    {
        dwSize = sizeof(dwAttributes);
        err = RegSetValueEx(hk, c_szEditFlags, 0, REG_BINARY,
                            (LPBYTE)&dwAttributes, dwSize);
        if (err != ERROR_SUCCESS)
            dwAttributes= 0;
        RegCloseKey(hk);
    }

    return(dwAttributes);
}

//================================================================
//================================================================
DWORD GetVerbAttributes(HKEY hkeyFT, LPSTR pszVerb)
{
    HKEY hk;
    LONG err;
    char szVerbKey[MAX_PATH+6];    // 6 = "\shell"
    DWORD dwType;
    DWORD dwAttributes = 1;
    DWORD dwSize;

    wsprintf(szVerbKey, c_szTemplateSS, c_szShell, pszVerb);
    if((hkeyFT != NULL) && (RegOpenKeyEx(hkeyFT, szVerbKey, 0, KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS))
    {
        dwSize = sizeof(dwAttributes);
                err = RegQueryValueEx(hk, (LPTSTR)c_szEditFlags, NULL, &dwType, (LPBYTE)&dwAttributes, &dwSize);
                if (err != ERROR_SUCCESS ||
                    dwType != REG_BINARY ||
                    dwSize != sizeof(dwAttributes))
                {
                        dwAttributes = 0;
                }
        RegCloseKey(hk);
    }

    return(dwAttributes);
}

//================================================================
//================================================================
VOID FT_MergeDuplicates(HWND hwndLV)
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
        if(lstrcmpi(pFTInfo1->szId, pFTInfo2->szId) == 0)    // we have a match
        {
            // add extension in pFTInfo1's hDPAExt
            DPA_InsertPtr(pFTInfo1->hDPAExt, 0x7fff, (LPVOID)DPA_FastGetPtr(pFTInfo2->hDPAExt,0));
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

//================================================================
//================================================================
VOID ExtToShellCommand(HKEY hkeyFT, LPSTR pszName, UINT uName)
{
    char szIdValue[MAX_PATH];
    char szShellCommand[MAX_PATH+MAX_PATH+7];  // 7 = "\shell\"
    char szShellCommandValue[MAX_PATH];
    char szClass[MAX_PATH];
    char szShellKey[MAX_PATH];
    DWORD dwClass;
    DWORD dwShellKey;
    DWORD dwSubKey;
    FILETIME ftLastWrite;
    LONG err = E_INVALIDARG;    // For proper handling of no hkey
    HKEY hk = (HKEY)NULL;

    if(hkeyFT)
    {
        // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell value
        err = sizeof(szIdValue);
        err = RegQueryValue(hkeyFT, c_szShell, szIdValue, &err);
        if (err == ERROR_SUCCESS && *szIdValue)
        {
            // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell\[szIdValue]\Command value
            wsprintf(szShellCommand, c_szTemplateSSS, c_szShell, szIdValue, c_szCommand);
            err = sizeof(szShellCommand);
            err = RegQueryValue(hkeyFT, szShellCommand, szShellCommandValue, &err);
        }
        else
        {
            // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell\Open\Command value
            err = sizeof(szShellCommandValue);
            err = RegQueryValue(hkeyFT, c_szShellOpenCommand, szShellCommandValue, &err);
            if (err != ERROR_SUCCESS || !*szShellCommandValue)
            {
                // see if there is a HKEY_CLASSES_ROOT\[szId]\Shell\[1st Key]\Command value
                if(RegOpenKeyEx(hkeyFT, c_szShell, 0, KEY_READ, &hk) == ERROR_SUCCESS)
                {
                    dwClass = sizeof(szClass);
                    dwShellKey = sizeof(szShellKey);
                    dwSubKey = 0;
                    if(RegEnumKeyEx(hk, dwSubKey, szShellKey, &dwShellKey, NULL, szClass, &dwClass, &ftLastWrite) == ERROR_SUCCESS)
                    {
                        wsprintf(szShellCommand, c_szTemplateSS, szShellKey, c_szCommand);
                        err = sizeof(szShellCommandValue);
                        err = RegQueryValue(hk, szShellCommand, szShellCommandValue, &err);
                    }
                }
            }
        }
    }

    if(hk != (HKEY)NULL)
        RegCloseKey(hk);

    if(err == ERROR_SUCCESS)
        lstrcpyn(pszName, szShellCommandValue, uName);
    else
        *pszName ='\0';
}

//================================================================
//  This function returns non-zero value only if the specified type
// has an associated icon specified by "DefaultIcon=" key.
//================================================================
HICON GetDefaultIcon(HKEY *hkeyFT, LPSTR pszId, DWORD dwIconType)
{
    HICON hicon = (HICON)NULL;
    LONG err;
    char szDefaultIcon[MAX_PATH];
    int iIconIndex;
    int iImage;

    Assert(hkeyFT != NULL);

    if(*hkeyFT == NULL)
        *hkeyFT = GetHkeyFT(pszId);

    if(*hkeyFT != (HKEY)NULL)
    {
        err = sizeof(szDefaultIcon);
        err = RegQueryValue(*hkeyFT, c_szDefaultIcon, szDefaultIcon, &err);
        if (err == ERROR_SUCCESS && *szDefaultIcon)
        {
            iIconIndex = ParseIconLocation(szDefaultIcon);
            PathRemoveArgs(szDefaultIcon);
            iImage = Shell_GetCachedImageIndex(szDefaultIcon, iIconIndex, 0);
            hicon = ImageList_ExtractIcon(hinstCabinet, (dwIconType == SHGFI_LARGEICON ? s_himlSysLarge : s_himlSysSmall), iImage);
        }
    }

#ifdef FT_DEBUG
    DebugMsg(DM_TRACE, "FT GetDefaultIcon szIcon=%s iIndex=%d hIcon=0x%x hkeyFT=0x%x",
        szDefaultIcon, iIconIndex, hicon, *hkeyFT);
#endif

    return(hicon);
}

//================================================================
//================================================================
BOOL FindDDEOptions(PFILETYPESDIALOGINFO pFTDInfo)
{
    BOOL bRC = FALSE;
    char ach[MAX_PATH+8];  // 8 = "\ddeexec"
    LONG err;
    HKEY hkDDE;

    // see if we have a DDE Message key and value
    if(pFTDInfo->pFTInfo->hkeyFT)
    {
        wsprintf(ach, c_szTemplateSSS, c_szShell, pFTDInfo->pFTCInfo->szActionKey, c_szDDEExec);
        err = sizeof(pFTDInfo->pFTCInfo->szDDEMsg);
        err = RegQueryValue(pFTDInfo->pFTInfo->hkeyFT, ach, pFTDInfo->pFTCInfo->szDDEMsg, &err);
        if(err == ERROR_SUCCESS && *pFTDInfo->pFTCInfo->szDDEMsg)
        {
            bRC = TRUE;
            if(RegOpenKey(pFTDInfo->pFTInfo->hkeyFT, ach, &hkDDE) == ERROR_SUCCESS)
            {
                    // see if we have a DDE Application key and value
                err = sizeof(pFTDInfo->pFTCInfo->szDDEApp);
                RegQueryValue(hkDDE, c_szDDEApp, pFTDInfo->pFTCInfo->szDDEApp, &err);

                // see if we have a DDE Application Not Running key and value
                err = sizeof(pFTDInfo->pFTCInfo->szDDEAppNot);
                RegQueryValue(hkDDE, c_szDDEAppNot, pFTDInfo->pFTCInfo->szDDEAppNot, &err);

                // see if we have a DDE Topic key and value
                err = sizeof(pFTDInfo->pFTCInfo->szDDETopic);
                RegQueryValue(hkDDE, c_szDDETopic, pFTDInfo->pFTCInfo->szDDETopic, &err);

                RegCloseKey(hkDDE);
            }
        }
    }
    return(bRC);
}

//====================================================================
//====================================================================
BOOL DefaultAction(HKEY hkeyFT, LPSTR pszDefaultAction, DWORD *dwDefaultAction)
{
    LONG err;

    err = RegQueryValue(hkeyFT, c_szShell, pszDefaultAction, dwDefaultAction);
    if (err == ERROR_SUCCESS && *pszDefaultAction)
        return(TRUE);

    return(FALSE);
}

//====================================================================
//====================================================================
VOID VerbToExe(HKEY hkeyFT, LPSTR pszVerb, LPSTR pszExe, DWORD *pdwExe)
{
    // caller is responsible to setting pdwExe
    char ach[MAX_PATH+MAX_PATH+7]; // 7 = "\shell\"
    LONG err;

    wsprintf(ach, c_szTemplateSSS, c_szShell, pszVerb, c_szCommand);
    err = RegQueryValue(hkeyFT, ach, pszExe, pdwExe);
    if (err != ERROR_SUCCESS || !*pszExe)
        {
        *pdwExe = 0;
        }
}

//====================================================================
//====================================================================
LONG SaveFileTypeData(DWORD dwName, PFILETYPESDIALOGINFO pFTDInfo)
{
    LONG lRC = ERROR_SUCCESS;
    HKEY hk;
    HKEY hk2;
    char szBuf[MAX_PATH+MAX_PATH+12];    // 12 = "\defaulticon"
    char szAction[MAX_PATH];

    switch(dwName)
    {
        case FTD_EDIT:
            // Save file type id and description
            if(RegSetValue(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId, REG_SZ, pFTDInfo->pFTInfo->szDesc,
                    sizeof(pFTDInfo->pFTInfo->szDesc)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;

            // Save default action key and value
            wsprintf(szBuf, c_szTemplateSS, pFTDInfo->pFTInfo->szId, c_szShell);
            if(RegSetValue(HKEY_CLASSES_ROOT, szBuf, REG_SZ, pFTDInfo->pFTInfo->szDefaultAction,
                    sizeof(szAction)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;
            break;
        case FTD_DOCICON:
            wsprintf(szBuf, c_szTemplateSS, pFTDInfo->pFTInfo->szId, c_szDefaultIcon);
#pragma data_seg(".text", "CODE")
            wsprintf(szAction, "%s,%d", pFTDInfo->szIconPath, pFTDInfo->iIconIndex);
#pragma data_seg()
            if(RegSetValue(HKEY_CLASSES_ROOT, szBuf, REG_SZ, szAction,
                    sizeof(szAction)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;
            break;
        case FTD_EXT:
            // Save extension and file type id
            if(RegSetValue(HKEY_CLASSES_ROOT,
                    DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt,0),
                    REG_SZ, pFTDInfo->pFTInfo->szId,
                    sizeof(pFTDInfo->pFTInfo->szId)) != ERROR_SUCCESS)
                lRC = !ERROR_SUCCESS;
            break;
#ifdef MIME
        case FTD_MIME:
            // Save MIME type.
                        if (! RegisterMIMEInformation(pFTDInfo))
                lRC = !ERROR_SUCCESS;
            break;
#endif   /* MIME */
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
                            sizeof(pFTDInfo->pFTCInfo->szActionValue)) != ERROR_SUCCESS)
                        lRC = !ERROR_SUCCESS;
                }

                // Save action command key and value
                if(RegSetValue(hk, c_szCommand, REG_SZ, pFTDInfo->pFTCInfo->szCommand,
                        sizeof(pFTDInfo->pFTCInfo->szCommand)) != ERROR_SUCCESS)
                    lRC = !ERROR_SUCCESS;

                if(IsDlgButtonChecked(pFTDInfo->hCmdDialog, IDC_FT_CMD_USEDDE))
                {
                    // Save DDE Message key and value
                    if(*pFTDInfo->pFTCInfo->szDDEMsg)
                    {
                        if(RegSetValue(hk, c_szDDEExec, REG_SZ, pFTDInfo->pFTCInfo->szDDEMsg,
                                sizeof(pFTDInfo->pFTCInfo->szDDEMsg)) != ERROR_SUCCESS)
                            lRC = !ERROR_SUCCESS;
                    }

                           if(RegCreateKey(hk, c_szDDEExec, &hk2) == ERROR_SUCCESS)
                    {
                            // Save DDEApp key and value
                            if(*pFTDInfo->pFTCInfo->szDDEApp)
                            {
                                if(RegSetValue(hk2, c_szDDEApp, REG_SZ, pFTDInfo->pFTCInfo->szDDEApp,
                                        sizeof(pFTDInfo->pFTCInfo->szDDEApp)) != ERROR_SUCCESS)
                                    lRC = !ERROR_SUCCESS;
                            }

                            // Save DDEAppNot key and value
                            if(*pFTDInfo->pFTCInfo->szDDEAppNot)
                            {
                                if(RegSetValue(hk2, c_szDDEAppNot, REG_SZ, pFTDInfo->pFTCInfo->szDDEAppNot,
                                        sizeof(pFTDInfo->pFTCInfo->szDDEAppNot)) != ERROR_SUCCESS)
                                    lRC = !ERROR_SUCCESS;
                            }

                            // Save DDETopic key and value
                            if(*pFTDInfo->pFTCInfo->szDDETopic)
                            {
                                if(RegSetValue(hk2, c_szDDETopic, REG_SZ, pFTDInfo->pFTCInfo->szDDETopic,
                                        sizeof(pFTDInfo->pFTCInfo->szDDETopic)) != ERROR_SUCCESS)
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

//====================================================================
//====================================================================
VOID ResizeCommandDlg(HWND hDialog, BOOL bFlag)
{
    RECT rcDialog;
    RECT rcControl;

    GetWindowRect(hDialog, &rcDialog);

    if(bFlag)    // resize to show dde group
        GetWindowRect(GetDlgItem(hDialog, IDC_FT_CMD_DDEGROUP), &rcControl);
    else        // resize to hide dde group
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

//====================================================================
//====================================================================
LONG RemoveAction(PFILETYPESDIALOGINFO pFTDInfo, HKEY hk, LPCSTR pszKey, LPSTR szAction)
{
    LONG lRC = ERROR_SUCCESS;
    HKEY hk1;
    int iNext;

    // Remove keys from the registry
    if(RegOpenKeyEx(hk, pszKey, 0, KEY_ALL_ACCESS, &hk1) == ERROR_SUCCESS)
    {
        if(RegDeleteKey(hk1, szAction) != ERROR_SUCCESS)
            lRC = !ERROR_SUCCESS;
        RegCloseKey(hk1);
    }

    // Remove the item from the list view, and ensure that the relative item
    // is selected, eg. if we deleted the last item then the last item remains
    // selected - this used to be completely bogus.

    iNext = ListView_GetNextItem(pFTDInfo->hwndLVFTEdit, pFTDInfo->iEditItem, LVNI_BELOW );
    if ( iNext == -1 )
        iNext = ListView_GetNextItem(pFTDInfo->hwndLVFTEdit, pFTDInfo->iEditItem, LVNI_ABOVE );

    ListView_DeleteItem(pFTDInfo->hwndLVFTEdit, pFTDInfo->iEditItem);
    ListView_SetItemState(pFTDInfo->hwndLVFTEdit, iNext, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    SetFocus(pFTDInfo->hwndLVFTEdit);

    if(pFTDInfo->pFTInfo->hIconOpen != (HICON)NULL)
    {
        DestroyIcon(pFTDInfo->pFTInfo->hIconOpen);
        pFTDInfo->pFTInfo->hIconOpen = (HICON)NULL;
        SendMessage(pFTDInfo->hwndOpenIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pFTDInfo->pFTInfo->hIconOpen);
    }

    return(lRC);
}

//====================================================================
//====================================================================
LONG RemoveFileType(PFILETYPESDIALOGINFO pFTDInfo)
{
    LONG lRC = ERROR_SUCCESS;
    LV_ITEM LVItem;
    int i;
    int iCnt;
    LPSTR pszExt;
    char szKey[MAX_PATH];
    char szBuf[2];
    HKEY hk;

    // Remove filetype and keys from the registry
    if(*pFTDInfo->pFTInfo->szId)
        if(RegDeleteKey(HKEY_CLASSES_ROOT, pFTDInfo->pFTInfo->szId) != ERROR_SUCCESS)
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
#ifdef MIME
         if (! RemoveMIMETypeInfo(pFTDInfo, pFTDInfo->pFTInfo->szOriginalMIMEType))
              lRC = !ERROR_SUCCESS;
#endif   /* MIME */
         iCnt = DPA_GetPtrCount(pFTDInfo->pFTInfo->hDPAExt);
            for(i = 0; i < iCnt; i++)
            {
                if(pszExt = DPA_FastGetPtr(pFTDInfo->pFTInfo->hDPAExt, i))
                {
                    if(*pszExt)
                    {
                        // Don't delete extension if it has a ShellNew key, just remove filetype
#pragma data_seg(".text", "CODE")
                        wsprintf(szKey, "%s\\%s%s", pszExt, c_szShell, c_szNew);
#pragma data_seg()
                        if(RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hk) == ERROR_SUCCESS)
                        {
                            RegCloseKey(hk);
                            *szBuf = '\0';  // remove the filetype assoc
                            RegSetValue(HKEY_CLASSES_ROOT, pszExt,
                                    REG_SZ, szBuf, sizeof(szBuf));
                        }
                        else
                        {
                            if(RegDeleteKey(HKEY_CLASSES_ROOT, pszExt) != ERROR_SUCCESS)
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

    // We need to case if we delete the last item as than we must
    // select the previous item not the same number...
    iCnt = ListView_GetItemCount(pFTDInfo->hwndLVFT);
    if (pFTDInfo->iItem >= iCnt)
        pFTDInfo->iItem--;
    ListView_SetItemState(pFTDInfo->hwndLVFT, pFTDInfo->iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    ListView_RedrawItems(pFTDInfo->hwndLVFT, 0, iCnt);
    ListView_EnsureVisible(pFTDInfo->hwndLVFT, pFTDInfo->iItem, FALSE);
    PostMessage(pFTDInfo->hwndLVFT, WM_SETFOCUS, (WPARAM)0, (LPARAM)0);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return(lRC);
}

//====================================================================
//====================================================================
// taken from fstreex.c
BOOL ExtToTypeNameAndId(LPSTR pszExt, LPSTR pszDesc, DWORD *pdwDesc, LPSTR pszId, DWORD *pdwId)
{
    LONG err;
    DWORD dwNm;
    BOOL bRC = TRUE;

    err = RegQueryValue(HKEY_CLASSES_ROOT, pszExt, pszId, pdwId);
    if (err == ERROR_SUCCESS && *pszId)
    {
        dwNm = *pdwDesc;    // if we fail we will still have name size to use
        err = RegQueryValue(HKEY_CLASSES_ROOT, pszId, pszDesc, &dwNm);
        if (err != ERROR_SUCCESS || !*pszDesc)
            goto Error;
        *pdwDesc = dwNm;
    }
    else
    {
           char szExt[MAX_PATH];    // "TXT"
           char szTemplate[128];   // "%s File"
        char szRet[MAX_PATH+20];        // "TXT File"
Error:
        bRC = FALSE;

        lstrcpy(pszId, pszExt);

        pszExt++;
        lstrcpy(szExt, pszExt);
        AnsiUpper(szExt);
        MLLoadStringA(IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
        wsprintf(szRet, szTemplate, szExt);
        lstrcpyn(pszDesc, szRet, *pdwDesc);
        *pdwDesc = lstrlen(pszDesc);
    }
    return(bRC);
}

//================================================================
//================================================================
VOID StrRemoveChar(LPSTR pszSrc, LPSTR pszDest, char ch)
{
    LPSTR pSrc = pszSrc;
    LPSTR pDest = (pszDest ?pszDest :pszSrc);

    Assert(pSrc);
    Assert(pDest);

    if(pSrc && pDest)
    {
        while(*pSrc)
        {
            if(*pSrc != ch)
                *(pDest++) = *pSrc;
            pSrc++;
        }
        *pDest = '\0';
    }
}

//================================================================
//================================================================
// This code should be in PATH.C
// in/out:
//    pszIconFile    location string
//            "progman.exe,3" -> "progman.exe"
//
// returns:
//    icon index (zero based) ready for ExtractIcon
//
int ParseIconLocation(LPSTR pszIconFile)
{
    int iIndex = 0;
    LPSTR pszComma = StrChr(pszIconFile, ',');

    if (pszComma) {
        *pszComma++ = 0;        // terminate the icon file name.

        while(*pszComma==' ')   // remove any white space
            pszComma++;

        iIndex = StrToInt(pszComma);
    }
    PathRemoveBlanks(pszIconFile);
    return iIndex;
}

//================================================================
//================================================================
BOOL IsDefaultAction(PFILETYPESDIALOGINFO pFTDInfo, LPSTR pszAction)
{
    return((lstrcmpi(pFTDInfo->pFTInfo->szDefaultAction, pszAction) == 0) ||
       (!(*pFTDInfo->pFTInfo->szDefaultAction) && (lstrcmpi(pszAction, c_szOpen) == 0)));
}

//================================================================
//================================================================
BOOL SetDefaultAction(PFILETYPESDIALOGINFO pFTDInfo)
{
    char szFile[MAX_PATH];
    LV_ITEM LVItem;

    if(IsDefaultAction(pFTDInfo, pFTDInfo->pFTCInfo->szActionKey))
        *pFTDInfo->pFTInfo->szDefaultAction = '\0';
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
///    if(IsDefaultAction(pFTDInfo, szAction))
///    {
        ExtToShellCommand(pFTDInfo->pFTInfo->hkeyFT, szFile, sizeof(szFile));
        PathRemoveArgs(szFile);
        PathRemoveBlanks(szFile);
        if(PathIsRelative(szFile))
            PathFindOnPath(szFile, NULL);    // search for exe

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
///    }

    return(TRUE);
}

//================================================================
//================================================================
HKEY GetHkeyFT(LPSTR pszId)
{
    HKEY hkeyFT;

    if(RegCreateKey(HKEY_CLASSES_ROOT, pszId, &hkeyFT) != ERROR_SUCCESS)
        hkeyFT = NULL;

    return(hkeyFT);
}



//================================================================================================//

//
// We make use of a newer API in shelldll.  However this is a private API introduced in SUR.
//
// Therefore this code attempts to do the right thing, first we check to see if we have
// found the exported entry from SHELLDLL.  If we have then we call that, but, this however may
// required thunking.
//
// If we didn't bind to the export, then we call our own version of the call.
//

LONG UrlPathProcessCommand( LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags )
{
    LONG lResult = 0;
    LPWSTR lpwSrc=NULL;
    LPWSTR lpwDest=NULL;
    int iLen;

    if ( lpPathProcessCommand )
    {
        if (RUNNING_NT)
        {
            iLen =     MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, NULL, 0);

            lpwSrc = (LPWSTR)LocalAlloc(LPTR, SIZEOF(WCHAR)*iLen);
            lpwDest = (LPWSTR)LocalAlloc(LPTR, SIZEOF(WCHAR)*iMax);

            if ( lpwSrc && lpwDest )
            {
                MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, lpwSrc, iLen);

                lResult = lpPathProcessCommand( (LPTSTR)lpwSrc, (LPTSTR)lpwDest, iMax, dwFlags );

                if ( lResult != -1 )
                    WideCharToMultiByte(CP_ACP, 0, lpwDest, -1, lpDest, iMax, NULL, NULL);
            }
        }
        else
            lResult = lpPathProcessCommand( lpSrc, lpDest, iMax, dwFlags );
    }
    else
    {
        lResult = UrlPathProcessCommand2( lpSrc, lpDest, iMax, dwFlags );
    }

    if ( lpwSrc )
        LocalFree(lpwSrc);
    if ( lpwDest )
        LocalFree(lpwDest);

    return lResult;
}

//
// PathProcessCommand implementation as found in path.c (shelldll).
//

LONG UrlPathProcessCommand2( LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags )
{
    TCHAR szName[MAX_PATH];
    LPTSTR lpBuffer, lpBuffer2;
    LPCTSTR lpArgs = NULL;
    DWORD dwAttrib;
    LONG i, iTotal;
    LONG iResult = -1;
    BOOL bAddQuotes = FALSE;
    BOOL bQualify = FALSE;
    BOOL bFound = FALSE;
    BOOL bHitSpace = FALSE;

    // Process the given source string, attempting to find what is that path, and what is its
    // arguments.

    if ( lpSrc )
    {
        // Extract the sub string, if its is realative then resolve (if required).

        if ( *lpSrc == TEXT('\"') )
        {
            for ( lpSrc++, i=0 ; i<MAX_PATH && *lpSrc && *lpSrc!=TEXT('\"') ; i++, lpSrc++ )
                szName[i] = *lpSrc;

            szName[i] = TEXT('\0');

            if ( *lpSrc )
                lpArgs = lpSrc+1;

            if ( ((dwFlags & PPCF_FORCEQUALIFY) || PathIsRelative( szName ))
                    && !( dwFlags & PPCF_NORELATIVEOBJECTQUALIFY ) )
            {
                if ( !PathResolve( szName, NULL, PRF_TRYPROGRAMEXTENSIONS ) )
                    goto exit_gracefully;
            }

            bFound = TRUE;
        }
        else
        {
            // Is this a relative object, and then take each element upto a seperator
            // and see if we hit an file system object.  If not then we can

            bQualify = PathIsRelative( lpSrc ) || ((dwFlags & PPCF_FORCEQUALIFY) != 0);

            for ( i=0; i < MAX_PATH ; i++ )
            {
                szName[i] = lpSrc[i];

                // If we hit a space then the string either contains a LFN or we have
                // some arguments.  Therefore attempt to get the attributes for the string
                // we have so far, if we are unable to then we can continue
                // checking, if we hit then we know that the object exists and the
                // trailing string are its arguments.

                if ( !szName[i] || szName[i] == TEXT(' ') )
                {
                    szName[i] = TEXT('\0');

                    while ( TRUE )
                    {
                        if ( bQualify && !PathResolve( szName, NULL, PRF_TRYPROGRAMEXTENSIONS ) )
                            break;

                        dwAttrib = GetFileAttributes( szName );

                        if ( dwAttrib == -1 || ( ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) && dwFlags & PPCF_NODIRECTORIES ) )
                            break;

                        if ( bQualify && ( dwFlags & PPCF_NORELATIVEOBJECTQUALIFY ) )
                            *lstrcpyn( szName, lpSrc, i ) = TEXT(' ');

                        bFound = TRUE;                  // success
                        lpArgs = &lpSrc[i];

                        goto exit_gracefully;
                    }

                    if ( bQualify )
                        hmemcpy( szName, lpSrc, (i+1)*SIZEOF(TCHAR) );
                    else
                        szName[i]=lpSrc[i];

                    bHitSpace = TRUE;
                }

                if ( !szName[i] )
                    break;
            }
        }
    }

exit_gracefully:

    // Work out how big the temporary buffer should be, allocate it and
    // build the returning string into it.  Then compose the string
    // to be returned.

    if ( bFound )
    {
        if ( StrChr( szName, TEXT(' ') ) )
            bAddQuotes = dwFlags & PPCF_ADDQUOTES;

        iTotal  = lstrlen(szName) + 1;                // for terminator
        iTotal += bAddQuotes ? 2 : 0;
        iTotal += (dwFlags & PPCF_ADDARGUMENTS) && lpArgs ? lstrlen(lpArgs) : 0;

        if ( lpDest )
        {
            if ( iTotal <= iDestMax )
            {
                lpBuffer = lpBuffer2 = (LPTSTR)LocalAlloc( LMEM_FIXED, SIZEOF(TCHAR)*iTotal );

                if ( lpBuffer )
                {
                    // First quote if required
                    if ( bAddQuotes )
                        *lpBuffer2++ = TEXT('\"');

                    // Matching name
                    lstrcpy( lpBuffer2, szName );

                    // Closing quote if required
                    if ( bAddQuotes )
                        lstrcat( lpBuffer2, TEXT("\"") );

                    // Arguments (if requested)
                    if ( (dwFlags & PPCF_ADDARGUMENTS) && lpArgs )
                        lstrcat( lpBuffer2, lpArgs );

                    // Then copy into callers buffer, and free out temporary buffer
                    lstrcpy( lpDest, lpBuffer );
                    LocalFree( (HGLOBAL)lpBuffer );

                    // Return the length of the resulting string
                    iResult = iTotal;
                }
            }
        }
        else
        {
            // Resulting string is this big, although nothing returned (allows them to allocate a buffer)
            iResult = iTotal;
        }
    }

    return iResult;
}
