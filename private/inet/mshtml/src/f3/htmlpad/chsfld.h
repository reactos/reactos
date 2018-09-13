////////////////////////////////////////////////////////////////////////////
//
//      CHSFLD.H
//
//      Prototype for HrPickFolder.
//      Displays a dialog box allowing user to choose a folder from message
//      stores in the current profile.
//
//  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#ifndef _CHSFLD_H_
#define _CHSFLD_H_

// Parameters:
// 
// required:
//
// hInst    - [in] instance of the module containing resources for the dialog
//
// hWnd     - [in] handle of the parent window for the dialog 
//
// pses     - [in] pointer to MAPI session object
//
// ppfld    - [out] on success points to the variable where a pointer to the
//                  selected folder is stored
//
// ppmdb    - [out] on success points to the variable where a pointer to a
//                  message store object containing the selected folder is stored
//
// optional:
//
// pcb      - [in/out] size of the buffer pointed to by *ppb
//
// ppb      - [in/out] *ppb is a pointer to the buffer where expand/collapse
//                      state of the dialog is stored. (don't mess with it).
//                      The state is valid only within the same MAPI session.
//
// Return Values:
//
// S_OK     - The call succeeded and has returned the expected values
//
// E_INVALIDARG - One or more of the parameters passed into the function
//                  were not valid
//
// MAPI_E_USER_CANCEL   - User canceled the dialog

STDAPI HrPickFolder(HINSTANCE hInst, HWND hWnd, LPMAPISESSION pses,
                    LPMAPIFOLDER * ppfld, LPMDB * ppmdb,
                    ULONG *pcb, LPBYTE *ppb);
                        
typedef   HRESULT (STDAPICALLTYPE * HRPICKFOLDER)(HINSTANCE hInst, HWND hWnd,
                    LPMAPISESSION pses, LPMAPIFOLDER * ppfld, LPMDB * ppmdb,
                    ULONG *pcb, LPBYTE *ppb);

#endif /* _CHSFLD_H_ */
#endif /* _WIN32 */
