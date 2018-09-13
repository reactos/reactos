/*
 * valid.c - Validation functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/****************************** Public Functions *****************************/


PUBLIC_CODE BOOL IsValidHWND(HWND hwnd)
{
   /* Ask User if this is a valid window. */

   return(IsWindow(hwnd));
}


#ifdef DEBUG

PUBLIC_CODE BOOL IsValidHANDLE(HANDLE hnd)
{
   return(EVAL(hnd != INVALID_HANDLE_VALUE));
}


PUBLIC_CODE BOOL IsValidHEVENT(HANDLE hevent)
{
   return(IsValidHANDLE(hevent));
}


PUBLIC_CODE BOOL IsValidHFILE(HANDLE hf)
{
   return(IsValidHANDLE(hf));
}


PUBLIC_CODE BOOL IsValidHGLOBAL(HGLOBAL hg)
{
   return(IsValidHANDLE(hg));
}


PUBLIC_CODE BOOL IsValidHMENU(HMENU hmenu)
{
   return(IsValidHANDLE(hmenu));
}


PUBLIC_CODE BOOL IsValidHINSTANCE(HINSTANCE hinst)
{
   return(IsValidHANDLE(hinst));
}


PUBLIC_CODE BOOL IsValidHICON(HICON hicon)
{
   return(IsValidHANDLE(hicon));
}


PUBLIC_CODE BOOL IsValidHKEY(HKEY hkey)
{
   return(IsValidHANDLE(hkey));
}


PUBLIC_CODE BOOL IsValidHMODULE(HMODULE hmod)
{
   return(IsValidHANDLE(hmod));
}


PUBLIC_CODE BOOL IsValidHPROCESS(HANDLE hprocess)
{
   return(IsValidHANDLE(hprocess));
}


PUBLIC_CODE BOOL IsValidPCSECURITY_ATTRIBUTES(PCSECURITY_ATTRIBUTES pcsa)
{
   /* BUGBUG: Fill me in. */

   return(IS_VALID_READ_PTR(pcsa, CSECURITY_ATTRIBUTES));
}


PUBLIC_CODE BOOL IsValidFileCreationMode(DWORD dwMode)
{
   BOOL bResult;

   switch (dwMode)
   {
      case CREATE_NEW:
      case CREATE_ALWAYS:
      case OPEN_EXISTING:
      case OPEN_ALWAYS:
      case TRUNCATE_EXISTING:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT(("IsValidFileCreationMode(): Invalid file creation mode %#lx.",
                    dwMode));
         break;
   }

   return(bResult);
}


PUBLIC_CODE BOOL IsValidHTEMPLATEFILE(HANDLE htf)
{
   return(IsValidHANDLE(htf));
}


PUBLIC_CODE BOOL IsValidPCFILETIME(PCFILETIME pcft)
{
   /* dwLowDateTime may be any value. */
   /* dwHighDateTime may be any value. */

   return(IS_VALID_READ_PTR(pcft, CFILETIME));
}


PUBLIC_CODE BOOL IsValidPCPOINT(PCPOINT pcpt)
{

   /* x may be any value. */
   /* y may be any value. */

   return(IS_VALID_READ_PTR(pcpt, CPOINT));
}


PUBLIC_CODE BOOL IsValidPCPOINTL(PCPOINTL pcptl)
{

   /* x may be any value. */
   /* y may be any value. */

   return(IS_VALID_READ_PTR(pcptl, CPOINTL));
}


PUBLIC_CODE BOOL IsValidPCWIN32_FIND_DATA(PCWIN32_FIND_DATA pcwfd)
{
   /* BUGBUG: Fill me in. */

   return(IS_VALID_READ_PTR(pcwfd, CWIN32_FIND_DATA));
}


PUBLIC_CODE BOOL IsValidShowCmd(int nShow)
{
   BOOL bResult;

   switch (nShow)
   {
      case SW_HIDE:
      case SW_SHOWNORMAL:
      case SW_SHOWMINIMIZED:
      case SW_SHOWMAXIMIZED:
      case SW_SHOWNOACTIVATE:
      case SW_SHOW:
      case SW_MINIMIZE:
      case SW_SHOWMINNOACTIVE:
      case SW_SHOWNA:
      case SW_RESTORE:
      case SW_SHOWDEFAULT:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT(("IsValidShowCmd(): Invalid show command %d.",
                    nShow));
         break;
   }

   return(bResult);
}


PUBLIC_CODE BOOL IsValidPath(PCSTR pcszPath)
{
   /* BUGBUG: Beef up path validation. */

   return(IS_VALID_STRING_PTR(pcszPath, CSTR) &&
          EVAL((UINT)lstrlen(pcszPath) < MAX_PATH_LEN));
}


PUBLIC_CODE BOOL IsValidPathResult(HRESULT hr, PCSTR pcszPath,
                                   UINT ucbPathBufLen)
{
   return((hr == S_OK &&
           EVAL(IsValidPath(pcszPath)) &&
           EVAL((UINT)lstrlen(pcszPath) < ucbPathBufLen)) ||
          (hr != S_OK &&
           EVAL(! ucbPathBufLen ||
                ! pcszPath ||
                ! *pcszPath)));
}


PUBLIC_CODE BOOL IsValidExtension(PCSTR pcszExt)
{
   return(IS_VALID_STRING_PTR(pcszExt, CSTR) &&
          EVAL(lstrlen(pcszExt) < MAX_PATH_LEN) &&
          EVAL(*pcszExt == PERIOD));
}


PUBLIC_CODE BOOL IsValidIconIndex(HRESULT hr, PCSTR pcszIconFile,
                                  UINT ucbIconFileBufLen, int niIcon)
{
   return(EVAL(IsValidPathResult(hr, pcszIconFile, ucbIconFileBufLen)) &&
          EVAL(hr == S_OK ||
               ! niIcon));
}


PUBLIC_CODE BOOL IsValidRegistryValueType(DWORD dwType)
{
   BOOL bResult;

   switch (dwType)
   {
      case REG_NONE:
      case REG_SZ:
      case REG_EXPAND_SZ:
      case REG_BINARY:
      case REG_DWORD:
      case REG_DWORD_BIG_ENDIAN:
      case REG_LINK:
      case REG_MULTI_SZ:
      case REG_RESOURCE_LIST:
      case REG_FULL_RESOURCE_DESCRIPTOR:
      case REG_RESOURCE_REQUIREMENTS_LIST:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT(("IsValidRegistryValueType(): Invalid registry value type %lu.",
                    dwType));
         break;
   }

   return(bResult);
}


PUBLIC_CODE BOOL IsValidHotkey(WORD wHotkey)
{
   /* BUGBUG: Beef up hotkey validation. */

   return(wHotkey != 0);
}


#ifdef _COMPARISONRESULT_DEFINED_

PUBLIC_CODE BOOL IsValidCOMPARISONRESULT(COMPARISONRESULT cr)
{
   BOOL bResult;

   switch (cr)
   {
      case CR_FIRST_SMALLER:
      case CR_EQUAL:
      case CR_FIRST_LARGER:
         bResult = TRUE;
         break;

      default:
         WARNING_OUT(("IsValidCOMPARISONRESULT(): Unknown COMPARISONRESULT %d.",
                      cr));
         bResult = FALSE;
         break;
   }

   return(bResult);
}

#endif   /* _COMPARISONRESULT_DEFINED_ */

#endif   /* DEBUG */

