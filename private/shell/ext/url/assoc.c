/*
 * assoc.c - Type association routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include <mluisupp.h>

#define _INTSHCUT_               /* for intshcut.h */
#include <intshcut.h>
#ifdef WINNT_ENV
#include <intshctp.h>  /* ALL_???_FLAGS */
#endif

#include "assoc.h"
#include "extricon.h"
#include "openas.h"
#pragma warning(disable:4001) /* "single line comment" warning */
#include "filetype.h"
#include "resource.h"
#pragma warning(default:4001) /* "single line comment" warning */
#include "shlstock.h"
#include "shlvalid.h"


/* Global Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PUBLIC_DATA const HKEY g_hkeyURLProtocols          = HKEY_CLASSES_ROOT;
PUBLIC_DATA const HKEY g_hkeyMIMESettings          = HKEY_CLASSES_ROOT;

PUBLIC_DATA CCHAR g_cszURLProtocol[]               = "URL Protocol";

PUBLIC_DATA CCHAR g_cszContentType[]               = "Content Type";
PUBLIC_DATA CCHAR g_cszExtension[]                 = "Extension";

#pragma data_seg()


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA CCHAR s_cszShellOpenCmdSubKeyFmt[]    = "%s\\shell\\open\\command";
PRIVATE_DATA CCHAR s_cszAppOpenCmdFmt[]            = "%s %%1";
PRIVATE_DATA CCHAR s_cszDefaultIconSubKeyFmt[]     = "%s\\DefaultIcon";
PRIVATE_DATA CCHAR s_cszDefaultIcon[]              = "url.dll,0";

#pragma data_seg()


/***************************** Private Functions *****************************/


/*
** RegisterAppAsURLProtocolHandler()
**
** Under HKEY_CLASSES_ROOT\url-protocol\shell\open\command, add default value =
** "c:\foo\bar.exe %1".
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL RegisterAppAsURLProtocolHandler(PCSTR pcszProtocol,
                                                  PCSTR pcszApp)
{
   BOOL bResult = FALSE;
   DWORD dwcbShellOpenCmdSubKeyLen;
   PSTR pszShellOpenCmdSubKey;

   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszApp, CSTR));

   /* (+ 1) for null terminator. */
   dwcbShellOpenCmdSubKeyLen = sizeof(s_cszShellOpenCmdSubKeyFmt) + 1
                               + lstrlen(pcszProtocol);

   if (AllocateMemory(dwcbShellOpenCmdSubKeyLen, &pszShellOpenCmdSubKey))
   {
      DWORD dwcbAppOpenCmdLen;
      PSTR pszAppOpenCmd;

      /* BUGBUG: We should quote pcszApp here only if it contains spaces. */

      /* (+ 1) for null terminator. */
      dwcbAppOpenCmdLen = sizeof(s_cszAppOpenCmdFmt) + 1 + lstrlen(pcszApp);

      if (AllocateMemory(dwcbAppOpenCmdLen, &pszAppOpenCmd))
      {
         EVAL((DWORD)wsprintf(pszShellOpenCmdSubKey, s_cszShellOpenCmdSubKeyFmt,
                              pcszProtocol) < dwcbShellOpenCmdSubKeyLen);

         EVAL((DWORD)wsprintf(pszAppOpenCmd, s_cszAppOpenCmdFmt, pcszApp)
              < dwcbAppOpenCmdLen);

         /* (+ 1) for null terminator. */
         bResult = (SetRegKeyValue(g_hkeyURLProtocols, pszShellOpenCmdSubKey,
                                   NULL, REG_SZ, (PCBYTE)pszAppOpenCmd,
                                   lstrlen(pszAppOpenCmd) + 1)
                    == ERROR_SUCCESS);

         FreeMemory(pszShellOpenCmdSubKey);
         pszShellOpenCmdSubKey = NULL;
      }

      FreeMemory(pszAppOpenCmd);
      pszAppOpenCmd = NULL;
   }

   return(bResult);
}


/*
** RegisterURLProtocolDescription()
**
** Under g_hkeyURLSettings\url-protocol, add default value =
** URL:Url-protocol Protocol.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL RegisterURLProtocolDescription(PCSTR pcszProtocol)
{
   BOOL bResult = FALSE;
   PSTR pszProtocolCopy;

   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));

   if (StringCopy(pcszProtocol, &pszProtocolCopy))
   {
      char szDescriptionFmt[MAX_PATH_LEN];

      /*
       * Convert first character of protocol to upper case for description
       * string.
       */

      *pszProtocolCopy = (CHAR)PtrToUlong(CharUpper((LPSTR)(DWORD)*pszProtocolCopy));

      if (MLLoadStringA(IDS_URL_DESC_FORMAT,
                     szDescriptionFmt, sizeof(szDescriptionFmt)))
      {
         char szDescription[MAX_PATH_LEN];

         if ((UINT)lstrlen(szDescriptionFmt) + (UINT)lstrlen(pszProtocolCopy)
             < sizeof(szDescription))
         {
            EVAL(wsprintf(szDescription, szDescriptionFmt, pszProtocolCopy)
                 < sizeof(szDescription));

            /* (+ 1) for null terminator. */
            bResult = (SetRegKeyValue(g_hkeyURLProtocols, pcszProtocol,
                                      NULL, REG_SZ, (PCBYTE)szDescription,
                                      lstrlen(szDescription) + 1)
                       == ERROR_SUCCESS);
         }
      }

      FreeMemory(pszProtocolCopy);
      pszProtocolCopy = NULL;
   }

   return(bResult);
}


/*
** RegisterURLProtocolFlags()
**
** Under g_hkeyURLSettings\url-protocol, add EditFlags = FTA_Show.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL RegisterURLProtocolFlags(PCSTR pcszProtocol)
{
   DWORD dwEditFlags = FTA_Show;

   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));

   /* BUGBUG: What about preserving any existing EditFlags here? */

   /* (+ 1) for null terminator. */
   return(SetRegKeyValue(g_hkeyURLProtocols, pcszProtocol, g_cszEditFlags,
                         REG_BINARY, (PCBYTE)&dwEditFlags, sizeof(dwEditFlags))
          == ERROR_SUCCESS);
}


/*
** RegisterURLProtocol()
**
** Under g_hkeyURLSettings\url-protocol, add URL Protocol = "".
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL RegisterURLProtocol(PCSTR pcszProtocol)
{
   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));

   /* (+ 1) for null terminator. */
   return(SetRegKeyValue(g_hkeyURLProtocols, pcszProtocol, g_cszURLProtocol,
                         REG_SZ, (PCBYTE)EMPTY_STRING,
                         lstrlen(EMPTY_STRING) + 1) == ERROR_SUCCESS);
}


/*
** RegisterURLProtocolDefaultIcon()
**
** Under g_hkeyURLSettings\url-protocol\DefaultIcon, add default value =
** app.exe,0.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL RegisterURLProtocolDefaultIcon(PCSTR pcszProtocol)
{
   BOOL bResult = FALSE;
   DWORD dwcbDefaultIconSubKeyLen;
   PSTR pszDefaultIconSubKey;

   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));

   /* (+ 1) for null terminator. */
   dwcbDefaultIconSubKeyLen = sizeof(s_cszDefaultIconSubKeyFmt) + 1
                              + lstrlen(pcszProtocol);

   if (AllocateMemory(dwcbDefaultIconSubKeyLen, &pszDefaultIconSubKey))
   {
      EVAL((DWORD)wsprintf(pszDefaultIconSubKey, s_cszDefaultIconSubKeyFmt,
                           pcszProtocol) < dwcbDefaultIconSubKeyLen);

      bResult = (SetRegKeyValue(g_hkeyURLProtocols, pszDefaultIconSubKey,
                                NULL, REG_SZ, (PCBYTE)s_cszDefaultIcon,
                                sizeof(s_cszDefaultIcon))
                 == ERROR_SUCCESS);

      FreeMemory(pszDefaultIconSubKey);
      pszDefaultIconSubKey = NULL;
   }

   return(bResult);
}


PRIVATE_CODE BOOL AllowedToRegisterMIMEType(PCSTR pcszMIMEContentType)
{
   BOOL bResult;

#pragma data_seg(DATA_SEG_READ_ONLY)

   bResult = (lstrcmpi(pcszMIMEContentType, "application/octet-stream") != 0 &&
              lstrcmpi(pcszMIMEContentType, "application/octet-string") != 0);

#pragma data_seg()

   if (bResult)
      TRACE_OUT(("AllowedToRegisterMIMEType(): MIME type %s may be registered.",
                 pcszMIMEContentType));
   else
      WARNING_OUT(("AllowedToRegisterMIMEType(): MIME type %s may not be registered.",
                   pcszMIMEContentType));

   return(bResult);
}


/****************************** Public Functions *****************************/


/*
** RegisterMIMETypeForExtension()
**
** Under HKEY_CLASSES_ROOT\.ext, add Content Type = mime/type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL RegisterMIMETypeForExtension(PCSTR pcszExtension,
                                               PCSTR pcszMIMEContentType)
{
   ASSERT(IS_VALID_STRING_PTR(pcszExtension, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, CSTR));

   ASSERT(IsValidExtension(pcszExtension));

   /* (+ 1) for null terminator. */
   return(SetRegKeyValue(HKEY_CLASSES_ROOT, pcszExtension, g_cszContentType, REG_SZ,
                         (PCBYTE)pcszMIMEContentType,
                         lstrlen(pcszMIMEContentType) + 1) == ERROR_SUCCESS);
}


/*
** UnregisterMIMETypeForExtension()
**
** Deletes Content Type under HKEY_CLASSES_ROOT\.ext.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL UnregisterMIMETypeForExtension(PCSTR pcszExtension)
{
   ASSERT(IS_VALID_STRING_PTR(pcszExtension, CSTR));

   ASSERT(IsValidExtension(pcszExtension));

   return(NO_ERROR == SHDeleteValue(HKEY_CLASSES_ROOT, pcszExtension, g_cszContentType));
}


/*
** RegisterExtensionForMIMEType()
**
** Under g_hkeyMIMESettings\MIME\Database\Content Type\mime/type, add
** Content Type = mime/type and Extension = .ext.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL RegisterExtensionForMIMEType(PCSTR pcszExtension,
                                               PCSTR pcszMIMEContentType)
{
   BOOL bResult;
   char szMIMEContentTypeSubKey[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcszExtension, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, CSTR));

   ASSERT(IsValidExtension(pcszExtension));

   bResult = GetMIMETypeSubKey(pcszMIMEContentType, szMIMEContentTypeSubKey,
                               sizeof(szMIMEContentTypeSubKey));

   if (bResult)
      /* (+ 1) for null terminator. */
      bResult = (SetRegKeyValue(g_hkeyMIMESettings, szMIMEContentTypeSubKey,
                                g_cszExtension, REG_SZ, (PCBYTE)pcszExtension,
                                lstrlen(pcszExtension) + 1) == ERROR_SUCCESS);

   if (bResult)
      TRACE_OUT(("RegisterExtensionForMIMEType(): Registered extension %s as default extension for MIME type %s.",
                 pcszExtension,
                 pcszMIMEContentType));
   else
      WARNING_OUT(("RegisterExtensionForMIMEType(): Failed to register extension %s as default extension for MIME type %s.",
                   pcszExtension,
                   pcszMIMEContentType));

   return(bResult);
}


/*
** UnregisterExtensionForMIMEType()
**
** Deletes Extension under
** g_hkeyMIMESettings\MIME\Database\Content Type\mime/type.  If no other values
** or sub keys are left, deletes
** g_hkeyMIMESettings\MIME\Database\Content Type\mime/type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  May also delete MIME key.
*/
PUBLIC_CODE BOOL UnregisterExtensionForMIMEType(PCSTR pcszMIMEContentType)
{
   BOOL bResult;
   char szMIMEContentTypeSubKey[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, CSTR));

   bResult = (GetMIMETypeSubKey(pcszMIMEContentType, szMIMEContentTypeSubKey,
                                sizeof(szMIMEContentTypeSubKey)) &&
              SHDeleteValue(g_hkeyMIMESettings, szMIMEContentTypeSubKey,
                                g_cszExtension) == ERROR_SUCCESS &&
              SHDeleteOrphanKey(g_hkeyMIMESettings, szMIMEContentTypeSubKey) == ERROR_SUCCESS);

   if (bResult)
      TRACE_OUT(("UnregisterExtensionForMIMEType(): Unregistered default extension for MIME type %s.",
                 pcszMIMEContentType));
   else
      WARNING_OUT(("UnregisterExtensionForMIMEType(): Failed to unregister default extension for MIME type %s.",
                   pcszMIMEContentType));

   return(bResult);
}


PUBLIC_CODE BOOL RegisterMIMEAssociation(PCSTR pcszFile,
                                         PCSTR pcszMIMEContentType)
{
   BOOL bResult;
   PCSTR pcszExtension;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, CSTR));

   pcszExtension = ExtractExtension(pcszFile);

    /*
     * Don't allow association of flag unknown MIME types
     * application/octet-stream and application/octet-string.
     */

   if (EVAL(*pcszExtension) &&
       AllowedToRegisterMIMEType(pcszMIMEContentType))
      bResult = (RegisterMIMETypeForExtension(pcszExtension, pcszMIMEContentType) &&
                 RegisterExtensionForMIMEType(pcszExtension, pcszMIMEContentType));
   else
      bResult = FALSE;

   return(bResult);
}


PUBLIC_CODE BOOL RegisterURLAssociation(PCSTR pcszProtocol, PCSTR pcszApp)
{
   ASSERT(IS_VALID_STRING_PTR(pcszProtocol, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszApp, CSTR));

   return(RegisterAppAsURLProtocolHandler(pcszProtocol, pcszApp) &&
          RegisterURLProtocolDescription(pcszProtocol) &&
          RegisterURLProtocol(pcszProtocol) &&
          RegisterURLProtocolFlags(pcszProtocol) &&
          RegisterURLProtocolDefaultIcon(pcszProtocol));
}


PUBLIC_CODE HRESULT MyMIMEAssociationDialog(HWND hwndParent, DWORD dwInFlags,
                                            PCSTR pcszFile,
                                            PCSTR pcszMIMEContentType,
                                            PSTR pszAppBuf, UINT ucAppBufLen)
{
   HRESULT hr;
   OPENASINFO oainfo;

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_MIMEASSOCDLG_FLAGS));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszAppBuf, STR, ucAppBufLen));

   /* Use default file name if not supplied by caller. */

   if (ucAppBufLen > 0)
      *pszAppBuf = '\0';

   oainfo.pcszFile = pcszFile;
   oainfo.pcszClass = pcszMIMEContentType;
   oainfo.dwInFlags = 0;

   if (IS_FLAG_SET(dwInFlags, MIMEASSOCDLG_FL_REGISTER_ASSOC))
      SET_FLAG(oainfo.dwInFlags, (OPENASINFO_FL_ALLOW_REGISTRATION |
                                  OPENASINFO_FL_REGISTER_EXT));

   hr = MyOpenAsDialog(hwndParent, &oainfo);

   if (hr == S_OK &&
       IS_FLAG_SET(dwInFlags, MIMEASSOCDLG_FL_REGISTER_ASSOC))
      hr = RegisterMIMEAssociation(pcszFile, pcszMIMEContentType) ? S_OK
                                                                  : E_OUTOFMEMORY;

   if (SUCCEEDED(hr))
      lstrcpyn(pszAppBuf, oainfo.szApp, ucAppBufLen);

   ASSERT(! ucAppBufLen ||
          (IS_VALID_STRING_PTR(pszAppBuf, STR) &&
           EVAL((UINT)lstrlen(pszAppBuf) < ucAppBufLen)));
   ASSERT(SUCCEEDED(hr) ||
          (! ucAppBufLen ||
           EVAL(! *pszAppBuf)));

   return(hr);
}


PUBLIC_CODE HRESULT MyURLAssociationDialog(HWND hwndParent, DWORD dwInFlags,
                                           PCSTR pcszFile, PCSTR pcszURL,
                                           PSTR pszAppBuf, UINT ucAppBufLen)
{
   HRESULT hr;
   PSTR pszProtocol;

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_URLASSOCDLG_FLAGS));
   ASSERT(IS_FLAG_SET(dwInFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME) ||
          IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszURL, CSTR));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszAppBuf, STR, ucAppBufLen));

   /* Use URL protocol as class name. */

   if (ucAppBufLen > 0)
      *pszAppBuf = '\0';

   hr = CopyURLProtocol(pcszURL, &pszProtocol);

   if (hr == S_OK)
   {
      char szInternetShortcut[MAX_PATH_LEN];
      OPENASINFO oainfo;

      /* Use default file name if not supplied by caller. */

      if (IS_FLAG_SET(dwInFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME) &&
          EVAL(MLLoadStringA(IDS_INTERNET_SHORTCUT,
                          szInternetShortcut, sizeof(szInternetShortcut))))
         pcszFile = szInternetShortcut;

      oainfo.pcszFile = pcszFile;
      oainfo.pcszClass = pszProtocol;
      oainfo.dwInFlags = 0;

      if (IS_FLAG_SET(dwInFlags, URLASSOCDLG_FL_REGISTER_ASSOC))
         SET_FLAG(oainfo.dwInFlags, OPENASINFO_FL_ALLOW_REGISTRATION);

      hr = MyOpenAsDialog(hwndParent, &oainfo);

      if (hr == S_OK &&
          IS_FLAG_SET(dwInFlags, URLASSOCDLG_FL_REGISTER_ASSOC))
         hr = RegisterURLAssociation(pszProtocol, oainfo.szApp) ? S_OK
                                                                : E_OUTOFMEMORY;

      if (SUCCEEDED(hr))
         lstrcpyn(pszAppBuf, oainfo.szApp, ucAppBufLen);

      FreeMemory(pszProtocol);
      pszProtocol = NULL;
   }

   ASSERT(! ucAppBufLen ||
          (IS_VALID_STRING_PTR(pszAppBuf, STR) &&
           EVAL((UINT)lstrlen(pszAppBuf) < ucAppBufLen)));
   ASSERT(SUCCEEDED(hr) ||
          (! ucAppBufLen ||
           EVAL(! *pszAppBuf)));

   return(hr);
}


#ifdef DEBUG

PUBLIC_CODE BOOL IsValidPCOPENASINFO(PCOPENASINFO pcoainfo)
{
   return(IS_VALID_READ_PTR(pcoainfo, COPENASINFO) &&
          IS_VALID_STRING_PTR(pcoainfo->pcszFile, CSTR) &&
          (! pcoainfo->pcszClass ||
           IS_VALID_STRING_PTR(pcoainfo->pcszClass, CSTR)) &&
          FLAGS_ARE_VALID(pcoainfo->dwInFlags, ALL_OPENASINFO_FLAGS) &&
          (! *pcoainfo->szApp ||
           IS_VALID_STRING_PTR(pcoainfo->szApp, STR)));
}

#endif   /* DEBUG */


/***************************** Exported Functions ****************************/


INTSHCUTAPI HRESULT WINAPI MIMEAssociationDialogA(HWND hwndParent,
                                                  DWORD dwInFlags,
                                                  PCSTR pcszFile,
                                                  PCSTR pcszMIMEContentType,
                                                  PSTR pszAppBuf,
                                                  UINT ucAppBufLen)
{
   HRESULT hr;

   DebugEntry(MIMEAssociationDialogA);

#ifdef EXPV
   /* Verify parameters. */

   if (IS_VALID_HANDLE(hwndParent, WND) &&
       IS_VALID_STRING_PTR(pcszFile, CSTR) &&
       IS_VALID_STRING_PTR(pcszMIMEContentType, CSTR) &&
       IS_VALID_WRITE_BUFFER_PTR(pszAppBuf, STR, ucAppBufLen))
   {
      if (FLAGS_ARE_VALID(dwInFlags, ALL_MIMEASSOCDLG_FLAGS))
#endif
      {
         hr = MyMIMEAssociationDialog(hwndParent, dwInFlags, pcszFile,
                                      pcszMIMEContentType, pszAppBuf,
                                      ucAppBufLen);
      }
#ifdef EXPV
      else
         hr = E_FLAGS;
   }
   else
      hr = E_POINTER;
#endif

   DebugExitHRESULT(MIMEAssociationDialogA, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

INTSHCUTAPI HRESULT WINAPI MIMEAssociationDialogW(HWND hwndParent,
                                                  DWORD dwInFlags,
                                                  PCWSTR pcszFile,
                                                  PCWSTR pcszMIMEContentType,
                                                  PWSTR pszAppBuf,
                                                  UINT ucAppBufLen)
{
   HRESULT hr;

   DebugEntry(MIMEAssociationDialogW);

   SetLastError(ERROR_NOT_SUPPORTED);
   hr = E_NOTIMPL;

   DebugExitHRESULT(MIMEAssociationDialogW, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


INTSHCUTAPI HRESULT WINAPI URLAssociationDialogA(HWND hwndParent,
                                                 DWORD dwInFlags,
                                                 PCSTR pcszFile, PCSTR pcszURL,
                                                 PSTR pszAppBuf,
                                                 UINT ucAppBufLen)
{
   HRESULT hr;

   DebugEntry(URLAssociationDialogA);

#ifdef EXPV
   /* Verify parameters. */

   if (IS_VALID_HANDLE(hwndParent, WND) &&
       (IS_FLAG_SET(dwInFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME) ||
        IS_VALID_STRING_PTR(pcszFile, CSTR)) &&
       IS_VALID_STRING_PTR(pcszURL, CSTR) &&
       IS_VALID_WRITE_BUFFER_PTR(pszAppBuf, STR, ucAppBufLen))
   {
      if (FLAGS_ARE_VALID(dwInFlags, ALL_URLASSOCDLG_FLAGS))
#endif
      {
         hr = MyURLAssociationDialog(hwndParent, dwInFlags, pcszFile, pcszURL,
                                     pszAppBuf, ucAppBufLen);
      }
#ifdef EXPV
      else
         hr = E_FLAGS;
   }
   else
      hr = E_POINTER;
#endif

   DebugExitHRESULT(URLAssociationDialogA, hr);

   return(hr);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

INTSHCUTAPI HRESULT WINAPI URLAssociationDialogW(HWND hwndParent,
                                                 DWORD dwInFlags,
                                                 PCWSTR pcszFile,
                                                 PCWSTR pcszURL,
                                                 PWSTR pszAppBuf,
                                                 UINT ucAppBufLen)
{
   HRESULT hr;

   DebugEntry(URLAssociationDialogW);

   SetLastError(ERROR_NOT_SUPPORTED);
   hr = E_NOTIMPL;

   DebugExitHRESULT(URLAssociationDialogW, hr);

   return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */
