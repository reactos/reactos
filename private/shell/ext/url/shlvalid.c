/*
 * shlvalid.c - Shell validation functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "olestock.h"
#include "olevalid.h"
#include "shlstock.h"
#include "shlvalid.h"

#ifdef DEBUG


/****************************** Public Functions *****************************/


PUBLIC_CODE BOOL IsValidPCIExtractIcon(PCIExtractIcon pciei)
{
   return(IS_VALID_READ_PTR(pciei, CIExtractIcon) &&
          IS_VALID_INTERFACE_PTR((PCIUnknown)pciei, IUnknown) &&
          IS_VALID_METHOD(pciei, GetIconLocation) &&
          IS_VALID_METHOD(pciei, Extract));
}


PUBLIC_CODE BOOL IsValidPCINewShortcutHook(PCINewShortcutHook pcinshhk)
{
   return(IS_VALID_READ_PTR(pcinshhk, CINewShortcutHook) &&
          IS_VALID_INTERFACE_PTR((PCIUnknown)pcinshhk, IUnknown) &&
          IS_VALID_METHOD(pcinshhk, SetReferent) &&
          IS_VALID_METHOD(pcinshhk, GetReferent) &&
          IS_VALID_METHOD(pcinshhk, SetFolder) &&
          IS_VALID_METHOD(pcinshhk, GetFolder) &&
          IS_VALID_METHOD(pcinshhk, GetName) &&
          IS_VALID_METHOD(pcinshhk, GetExtension));
}


PUBLIC_CODE BOOL IsValidPCIShellExecuteHook(PCIShellExecuteHook pciseh)
{
   return(IS_VALID_READ_PTR(pciseh, CIShellExecuteHook) &&
          IS_VALID_INTERFACE_PTR((PCIUnknown)pciseh, IUnknown) &&
          IS_VALID_METHOD(pciseh, Execute));
}


PUBLIC_CODE BOOL IsValidPCIShellExtInit(PCIShellExtInit pcisei)
{
   return(IS_VALID_READ_PTR(pcisei, CIShellExtInit) &&
          IS_VALID_INTERFACE_PTR((PCIUnknown)pcisei, IUnknown) &&
          IS_VALID_METHOD(pcisei, Initialize));
}


PUBLIC_CODE BOOL IsValidPCIShellLink(PCIShellLink pcisl)
{
   return(IS_VALID_READ_PTR(pcisl, CIShellLink) &&
          IS_VALID_READ_PTR(pcisl->lpVtbl, sizeof(*(pcisl->lpVtbl))) &&
          IS_VALID_INTERFACE_PTR((PCIUnknown)pcisl, IUnknown) &&
          IS_VALID_METHOD(pcisl, SetPath) &&
          IS_VALID_METHOD(pcisl, GetPath) &&
          IS_VALID_METHOD(pcisl, SetRelativePath) &&
          IS_VALID_METHOD(pcisl, SetIDList) &&
          IS_VALID_METHOD(pcisl, GetIDList) &&
          IS_VALID_METHOD(pcisl, SetDescription) &&
          IS_VALID_METHOD(pcisl, GetDescription) &&
          IS_VALID_METHOD(pcisl, SetArguments) &&
          IS_VALID_METHOD(pcisl, GetArguments) &&
          IS_VALID_METHOD(pcisl, SetWorkingDirectory) &&
          IS_VALID_METHOD(pcisl, GetWorkingDirectory) &&
          IS_VALID_METHOD(pcisl, SetHotkey) &&
          IS_VALID_METHOD(pcisl, GetHotkey) &&
          IS_VALID_METHOD(pcisl, SetShowCmd) &&
          IS_VALID_METHOD(pcisl, GetShowCmd) &&
          IS_VALID_METHOD(pcisl, SetIconLocation) &&
          IS_VALID_METHOD(pcisl, GetIconLocation) &&
          IS_VALID_METHOD(pcisl, Resolve));
}


PUBLIC_CODE BOOL IsValidPCIShellPropSheetExt(PCIShellPropSheetExt pcispse)
{
   return(IS_VALID_READ_PTR(pcispse, CIShellPropSheetExt) &&
          IS_VALID_INTERFACE_PTR((PCIUnknown)pcispse, IUnknown) &&
          IS_VALID_METHOD(pcispse, AddPages) &&
          IS_VALID_METHOD(pcispse, ReplacePage));
}


PUBLIC_CODE BOOL IsValidPCITEMIDLIST(PCITEMIDLIST pcidl)
{
   /* BUGBUG: Fill me in. */

   return(IS_VALID_READ_PTR(pcidl, CITEMIDLIST));
}


PUBLIC_CODE BOOL IsValidPCPROPSHEETPAGE(PCPROPSHEETPAGE pcpsp)
{
   /* BUGBUG: Fill me in. */

   return(IS_VALID_READ_PTR(pcpsp, CPROPSHEETPAGE));
}


PUBLIC_CODE BOOL IsValidPCSHELLEXECUTEINFO(PCSHELLEXECUTEINFO pcei)
{
   /* hInstApp may be any value. */
   /* dwHotKey may be any value. */

   return(IS_VALID_READ_PTR(pcei, CSHELLEXECUTEINFO) &&
          pcei->cbSize >= sizeof(*pcei) &&
          FLAGS_ARE_VALID(pcei->fMask, ALL_SHELLEXECUTE_MASK_FLAGS) &&
          (! pcei->hwnd ||
           IS_VALID_HANDLE(pcei->hwnd, WND)) &&
          (! pcei->lpVerb ||
           IS_VALID_STRING_PTR(pcei->lpVerb, CSTR)) &&
          (! pcei->lpFile ||
           IS_VALID_STRING_PTR(pcei->lpFile, CSTR)) &&
          (! pcei->lpParameters ||
           IS_VALID_STRING_PTR(pcei->lpParameters, CSTR)) &&
          (! pcei->lpDirectory ||
           IS_VALID_STRING_PTR(pcei->lpDirectory, CSTR)) &&
          EVAL(IsValidShowCmd(pcei->nShow)) &&
          (IS_FLAG_CLEAR(pcei->fMask, SEE_MASK_IDLIST) ||
           IS_VALID_STRUCT_PTR(pcei->lpIDList, CITEMIDLIST)) &&
          ((pcei->fMask & SEE_MASK_CLASSKEY) == SEE_MASK_CLASSKEY ||
           IS_FLAG_CLEAR(pcei->fMask, SEE_MASK_CLASSNAME) ||
           IS_VALID_STRING_PTR(pcei->lpClass, CSTR)) &&
          (IS_FLAG_CLEAR(pcei->fMask, SEE_MASK_CLASSKEY) ||
           IS_VALID_HANDLE(pcei->hkeyClass, KEY)) &&
          (! pcei->hIcon ||
           IS_VALID_HANDLE(pcei->hIcon, ICON)) &&
          (! pcei->hProcess ||
           IS_VALID_HANDLE(pcei->hProcess, PROCESS)));
}

#endif   /* DEBUG */

