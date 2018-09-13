/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    hwprof.c

Abstract:

    This module contains the dialog box procedure for the Hardware Profiles
    Dialog Box in the System Applet.

Author:

    Paula Tomlinson (paulat) 8-22-1995

Environment:

    User mode only.

Revision History:

    22-Aug-1995     paulat

        Creation and initial implementation.

    21-Jan-1999     jamesca

        Added handling for hardware profile aliases and hardware-detected
        profile attributes.

--*/


//
// include files
//

#include "sysdm.h"
#include <stdlib.h>


//
// private types and definitions
//

#define MAX_PROFILES             9999
#define MAX_ALIASES              9999
#define MAX_FRIENDLYNAME_LEN     80
#define MAX_PROFILEID_LEN        5
#define MAX_DOCKID_LEN           128
#define MAX_SERIALNUM_LEN        128

#define MAX_USER_WAIT            500
#define MIN_USER_WAIT            0
#define DEFAULT_USER_WAIT        30

#define MAX_GUID_STRING_LEN      39   // 38 chars + terminating null

typedef struct HWPROFILE_s {
   HWND     hParent;
   ULONG    ulFromProfileID;
   TCHAR    szFromFriendlyName[MAX_FRIENDLYNAME_LEN];
   ULONG    ulToProfileID;
   TCHAR    szToFriendlyName[MAX_FRIENDLYNAME_LEN];
} HWPROFILE, *PHWPROFILE;


typedef struct HWPROF_VALUES_s {
   ULONG    ulAction;
   ULONG    ulProfile;
   ULONG    ulPreferenceOrder;
   ULONG    ulDockState;
   BOOL     bAliasable;
   BOOL     bCloned;
   BOOL     bPortable;
   ULONG    ulCreatedFrom;
   WCHAR    szDockID[MAX_DOCKID_LEN];
   WCHAR    szSerialNumber[MAX_SERIALNUM_LEN];
   WCHAR    szFriendlyName[MAX_FRIENDLYNAME_LEN];
} HWPROF_VALUES, *PHWPROF_VALUES;


typedef struct HWPROF_INFO_s {
   ULONG             ulNumProfiles;
   ULONG             ulActiveProfiles;
   PHWPROF_VALUES    pHwProfValues;
   ULONG             ulSelectedProfile;
   ULONG             ulSelectedProfileIndex;
   BOOL              bPortable;
   BOOL              bHwDetectedPortable;
   ULONG             ulUndockedProfileNameCount;
   ULONG             ulDockedProfileNameCount;
   ULONG             ulUnknownProfileNameCount;
} HWPROF_INFO, *PHWPROF_INFO;


#define HWP_NO_ACTION   0x00000000
#define HWP_DELETE      0x00000001
#define HWP_CREATE      0x00000002
#define HWP_RENAME      0x00000004
#define HWP_REORDER     0x00000008
#define HWP_PROPERTIES  0x00000010
#define HWP_NEWPROFILE  0x00001000

//
// private prototypes
//

BOOL
GetCurrentProfile(
      PULONG  pulProfile
      );

BOOL
GetRegProfileCount(
      PULONG   pulProfiles
      );

BOOL
FillProfileList(
      HWND  hDlg
      );

BOOL
IsProfileNameInUse(
      HWND     hDlg,
      LPTSTR   pszFriendlyName
      );

BOOL
CopyHardwareProfile(
      HWND   hDlg,
      ULONG_PTR ulIndex,
      ULONG  ulProfile,
      LPTSTR szNewFriendlyName
      );

BOOL
RenameHardwareProfile(
      HWND   hDlg,
      ULONG_PTR ulIndex,
      ULONG  ulProfile,
      LPTSTR szNewFriendlyName
      );

BOOL
DeleteHardwareProfile(
      HWND   hDlg,
      ULONG_PTR ulIndex
      );

BOOL
GetUserWaitInterval(
      PULONG   pulWait
      );

BOOL
SetUserWaitInterval(
      ULONG   ulWait
      );

BOOL
GetFreeProfileID(
      PHWPROF_INFO   pInfo,
      PULONG         pulProfile
      );

ULONG
GetOriginalProfile(
      PHWPROF_INFO  pInfo,
      ULONG         ulProfile,
      ULONG         ulBufferIndex
      );

BOOL
InsertRank(
      PHWPROF_INFO   pInfo,
      ULONG          ulRank
      );

BOOL
DeleteRank(
      PHWPROF_INFO   pInfo,
      ULONG          ulRank
      );

BOOL
FlushProfileChanges(
      HWND hDlg,
      HWND hList
      );

BOOL
WriteProfileInfo(
      PHWPROF_VALUES pProfValues
      );

BOOL
RemoveNewProfiles(
      PHWPROF_INFO   pInfo
      );

BOOL
SwapPreferenceOrder(
      HWND  hDlg,
      HWND  hList,
      ULONG_PTR ulIndex1,
      ULONG_PTR ulIndex2
      );

BOOL
DeleteProfileDependentTree(
      ULONG ulProfile
      );

BOOL
CopyAliasEntries(
      ULONG ulSrcProfile,
      ULONG ulDestProfile
      );

BOOL
DeleteAliasEntries(
      ULONG ulProfile
      );

BOOL
CopyAliasEntryType(
      ULONG  ulSrcProfile,
      ULONG  ulDestProfile,
      LPWSTR szSubKeyName
      );

BOOL
DeleteAliasEntryType(
      ULONG  ulProfile,
      LPWSTR szSubKeyName
      );

BOOL
CopyRegistryNode(
      HKEY     hSrcKey,
      HKEY     hDestKey
      );

BOOL
DeleteRegistryNode(
      HKEY     hNodeKey,
      LPTSTR   szSubKey
      );
VOID
AdjustProfileTypeCounter(
      PHWPROF_INFO   pInfo,
      ULONG          ulDockState,
      BOOL           bIncrement
      );

BOOL
StripCurrentTag(
      LPTSTR   szOriginalName,
      ULONG    ulProfile,
      ULONG    ulCurrentProfile
      );

BOOL
AppendCurrentTag(
      LPTSTR   szTaggedName,
      LPCTSTR  szOriginalName,
      ULONG    ulProfile,
      ULONG    ulCurrentProfile
      );

VOID
CreateHwProfileFriendlyName(
      IN  HWND    hDlg,
      IN  ULONG   ulDockState,
      OUT LPTSTR  szFriendlyName
   );

VOID
DisplayPrivateMessage(
      HWND  hDlg,
      UINT  uiPrivateError
      );

VOID
DisplaySystemMessage(
      HWND  hWnd,
      UINT  uiSystemError
      );

BOOL
ValidateAsciiString(
      IN LPTSTR  pszString
      );

BOOL
UpdateOrderButtonState(
      HWND  hDlg
      );

BOOL
DisplayProperties(
      IN HWND           hOwnerDlg,
      IN PHWPROF_INFO   pProfValues
      );

typedef BOOL (CALLBACK FAR * LPFNADDPROPSHEETPAGE)(HPROPSHEETPAGE, LPARAM);


//
// global strings
//
WCHAR pszErrorCaption[MAX_PATH];
WCHAR pszRegDefaultFriendlyName[MAX_FRIENDLYNAME_LEN];
WCHAR pszCurrentTag[64];
WCHAR pszUnavailable[64];
WCHAR pszDocked[64];
WCHAR pszUndocked[64];
WCHAR pszUnknown[64];

WCHAR pszRegIDConfigDB[] = TEXT("System\\CurrentControlSet\\Control\\IDConfigDB");
WCHAR pszRegHwProfiles[] = TEXT("System\\CurrentControlSet\\Hardware Profiles");
WCHAR pszRegKnownDockingStates[] =  TEXT("Hardware Profiles");
WCHAR pszRegCurrentDockInfo[] =     TEXT("CurrentDockInfo");
WCHAR pszRegDockingState[] =        TEXT("DockingState");
WCHAR pszRegAlias[] =               TEXT("Alias");
WCHAR pszRegAcpiAlias[] =           TEXT("AcpiAlias");
WCHAR pszRegProfileNumber[] =       TEXT("ProfileNumber");
WCHAR pszRegCurrentConfig[] =       TEXT("CurrentConfig");
WCHAR pszRegUserWaitInterval[] =    TEXT("UserWaitInterval");
WCHAR pszRegFriendlyName[] =        TEXT("FriendlyName");
WCHAR pszRegPristine[] =            TEXT("Pristine");
WCHAR pszRegHwProfileGuid[] =       TEXT("HwProfileGuid");
WCHAR pszRegPreferenceOrder[] =     TEXT("PreferenceOrder");
WCHAR pszRegDockState[] =           TEXT("DockState");
WCHAR pszRegAliasable[] =           TEXT("Aliasable");
WCHAR pszRegCloned[] =              TEXT("Cloned");
WCHAR pszRegIsPortable[] =          TEXT("IsPortable");
WCHAR pszRegDockID[] =              TEXT("DockID");
WCHAR pszRegSerialNumber[] =        TEXT("SerialNumber");
WCHAR pszRegAcpiSerialNumber[] =    TEXT("AcpiSerialNumber");
WCHAR pszRegPropertyProviders[] =   TEXT("PropertyProviders");
WCHAR pszRegDocked[] =              TEXT("Docked");
WCHAR pszRegUndocked[] =            TEXT("Undocked");
WCHAR pszRegUnknown[] =             TEXT("Unknown");

//
// global mutex for synchronization
//
WCHAR  pszNamedMutex[] =            TEXT("System-HardwareProfiles-PLT");
HANDLE g_hMutex = NULL;

//
// global info for property sheet extensions
//
#define MAX_EXTENSION_PROVIDERS  32
HMODULE        hLibs[MAX_EXTENSION_PROVIDERS];
HPROPSHEETPAGE hPages[MAX_EXTENSION_PROVIDERS];
ULONG          ulNumPages = 0;
BOOL           bAdmin = FALSE;


static int HwProfileHelpIds[] = {
   IDD_HWP_PROFILES,        (IDH_HWPROFILE + IDD_HWP_PROFILES),
   IDD_HWP_PROPERTIES,      (IDH_HWPROFILE + IDD_HWP_PROPERTIES),
   IDD_HWP_COPY,            (IDH_HWPROFILE + IDD_HWP_COPY),
   IDD_HWP_RENAME,          (IDH_HWPROFILE + IDD_HWP_RENAME),
   IDD_HWP_DELETE,          (IDH_HWPROFILE + IDD_HWP_DELETE),
   IDD_HWP_ST_MULTIPLE,     (IDH_HWPROFILE + IDD_HWP_ST_MULTIPLE),
   IDD_HWP_WAITFOREVER,     (IDH_HWPROFILE + IDD_HWP_WAITFOREVER),
   IDD_HWP_WAITUSER,        (IDH_HWPROFILE + IDD_HWP_WAITUSER),
   IDD_HWP_SECONDS,         (IDH_HWPROFILE + IDD_HWP_SECONDS),
   IDD_HWP_SECSCROLL,       (IDH_HWPROFILE + IDD_HWP_SECSCROLL),
   IDD_HWP_COPYTO,          (IDH_HWPROFILE + IDD_HWP_COPYTO),
   IDD_HWP_COPYFROM,        (IDH_HWPROFILE + IDD_HWP_COPYFROM),
   IDD_HWP_ST_DOCKID,       (IDH_HWPROFILE + IDD_HWP_ST_DOCKID),
   IDD_HWP_ST_SERIALNUM,    (IDH_HWPROFILE + IDD_HWP_ST_SERIALNUM),
   IDD_HWP_DOCKID,          (IDH_HWPROFILE + IDD_HWP_DOCKID),
   IDD_HWP_SERIALNUM,       (IDH_HWPROFILE + IDD_HWP_SERIALNUM),
   IDD_HWP_PORTABLE,        (IDH_HWPROFILE + IDD_HWP_PORTABLE),
   IDD_HWP_ALIASABLE,       IDH_HWP_PROPERTIES_SELECTION_CHECKBOX,
   IDD_HWP_UNKNOWN,         (IDH_HWPROFILE + IDD_HWP_UNKNOWN),
   IDD_HWP_DOCKED,          (IDH_HWPROFILE + IDD_HWP_DOCKED),
   IDD_HWP_UNDOCKED,        (IDH_HWPROFILE + IDD_HWP_UNDOCKED),
   IDD_HWP_ST_PROFILE,      (IDH_HWPROFILE + IDD_HWP_ST_PROFILE),
   IDD_HWP_ORDERUP,         (IDH_HWPROFILE + IDD_HWP_ORDERUP),
   IDD_HWP_ORDERDOWN,       (IDH_HWPROFILE + IDD_HWP_ORDERDOWN),
   IDD_HWP_RENAMEFROM,      (IDH_HWPROFILE + IDD_HWP_RENAMEFROM),
   IDD_HWP_RENAMETO,        (IDH_HWPROFILE + IDD_HWP_RENAMETO),
   IDD_HWP_WAITUSER_TEXT_1, (IDH_HWPROFILE + IDD_HWP_WAITUSER),
   IDD_HWP_UNUSED_1,        -1,
   IDD_HWP_UNUSED_2,        -1,
   IDD_HWP_UNUSED_3,        -1,
   IDD_HWP_UNUSED_4,        -1,
   IDD_HWP_UNUSED_5,        -1,
   IDD_HWP_UNUSED_6,        -1,
   0, 0
};


/**-------------------------------------------------------------------------**/
INT_PTR
APIENTRY
HardwareProfilesDlg(
      HWND    hDlg,
      UINT    uMessage,
      WPARAM  wParam,
      LPARAM  lParam
      )

{
   BOOL           Status;
   ULONG          ulCurrentProfile, ulSelectedProfile,
                  ulWait, ulBufferIndex = 0;
   ULONG_PTR      ulIndex;
   LONG           lValue;
   TCHAR          szProfileName[MAX_PATH], szName[MAX_PATH];
   HWND           hList;
   int            nValue;
   HWPROFILE      HwSelectedProfile;
   LPNM_UPDOWN    pUDData;
   PHWPROF_INFO   pInfo;
   HICON          hIcon;


   switch (uMessage)
   {
      case WM_INITDIALOG:
         bAdmin = IsUserAdmin();

         //
         // attempt to claim the named mutex and lock other instances of
         // this dialog box out
         //
         g_hMutex = CreateMutex(NULL, TRUE, pszNamedMutex);

         if (g_hMutex == NULL) {

            if (GetLastError() == ERROR_ALREADY_EXISTS) {
               DisplayPrivateMessage(hDlg, HWP_ERROR_IN_USE);
            } else {
               DisplaySystemMessage(hDlg, GetLastError());
            }

            EndDialog(hDlg, FALSE);
            return FALSE;
         }

         //
         // load some global strings
         //
         LoadString(hInstance, HWP_CURRENT_TAG, pszCurrentTag, 64);
         LoadString(hInstance, HWP_UNAVAILABLE, pszUnavailable, 64);
         LoadString(hInstance, HWP_UNKNOWN_PROFILE,  pszUnknown, 64);
         LoadString(hInstance, HWP_DOCKED_PROFILE,   pszDocked, 64);
         LoadString(hInstance, HWP_UNDOCKED_PROFILE, pszUndocked, 64);
         LoadString(hInstance, HWP_ERROR_CAPTION, pszErrorCaption, MAX_PATH);
         LoadString(hInstance, HWP_DEF_FRIENDLYNAME, pszRegDefaultFriendlyName,
                  MAX_FRIENDLYNAME_LEN);

         //
         // fill the profiles listbox with all installed profiles,
         // this will also select the current profile
         //
         if (!FillProfileList(hDlg)) {
            EndDialog(hDlg, FALSE);
            return FALSE;
         }

         pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

         //
         // place the icons on the up and down selection buttons
         //
         SendDlgItemMessage(
               hDlg, IDD_HWP_ORDERUP, BM_SETIMAGE, (WPARAM)IMAGE_ICON,
               (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(UP_ICON)));

         SendDlgItemMessage(
               hDlg, IDD_HWP_ORDERDOWN, BM_SETIMAGE, (WPARAM)IMAGE_ICON,
               (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(DOWN_ICON)));

         //
         // update button enable/disable states
         //
         UpdateOrderButtonState(hDlg);

         //
         // disable Delete for the current profile
         //
         EnableWindow(GetDlgItem(hDlg, IDD_HWP_DELETE), FALSE);

         //
         // disable copy if we're already at the max number of profiles
         // (including the pristine profile)
         //
         if ((pInfo->ulNumProfiles+1) > MAX_PROFILES) {
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_COPY), FALSE);
         }

         //
         // initialize the user wait setting
         //
         SendDlgItemMessage(hDlg, IDD_HWP_SECSCROLL, UDM_SETBASE, 10, 0);
         SendDlgItemMessage(hDlg, IDD_HWP_SECSCROLL, UDM_SETRANGE, 0,
                  MAKELONG((SHORT)MAX_USER_WAIT, (SHORT)MIN_USER_WAIT));
         SendDlgItemMessage(hDlg, IDD_HWP_SECONDS, EM_LIMITTEXT, 3, 0L);

         GetUserWaitInterval(&ulWait);

         if (ulWait == 0xFFFFFFFF) {
             CheckRadioButton(hDlg, IDD_HWP_WAITFOREVER, IDD_HWP_WAITUSER,
                              IDD_HWP_WAITFOREVER);
            SendDlgItemMessage(hDlg, IDD_HWP_SECSCROLL, UDM_SETPOS, 0,
                               DEFAULT_USER_WAIT);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECONDS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECSCROLL), FALSE);
         }
         else {
            CheckRadioButton(hDlg, IDD_HWP_WAITFOREVER, IDD_HWP_WAITUSER,
                             IDD_HWP_WAITUSER);
            SendDlgItemMessage(hDlg, IDD_HWP_SECSCROLL, UDM_SETPOS, 0, ulWait);
         }

         //
         // Disable all actions if user not part of administrators local group
         //
         if (!bAdmin) {
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_ORDERUP),    FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_PROPERTIES), FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_COPY),       FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_RENAME),     FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_DELETE),     FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_ORDERDOWN),  FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_WAITFOREVER),FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_WAITUSER),   FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_WAITUSER_TEXT_1), FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECONDS),    FALSE);
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECSCROLL),  FALSE);
         }
         return 0;


      case WM_HELP:
         WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, HELP_FILE,
               HELP_WM_HELP, (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;

      case WM_CONTEXTMENU:
         WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
               (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;

      case WM_DESTROY:
          //
          // only free the buffer if we've already initialized
          //
          pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

          if (pInfo) {
              LocalUnlock(LocalHandle(pInfo->pHwProfValues));
              LocalFree(LocalHandle(pInfo->pHwProfValues));
              LocalUnlock(LocalHandle(pInfo));
              LocalFree(LocalHandle(pInfo));

              hIcon = (HICON)SendDlgItemMessage(
                    hDlg, IDD_HWP_ORDERUP, BM_GETIMAGE, 0, 0);
              if (hIcon) {
                 DeleteObject(hIcon);
              }

              hIcon = (HICON)SendDlgItemMessage(
                    hDlg, IDD_HWP_ORDERDOWN, BM_GETIMAGE, 0, 0);
              if (hIcon) {
                 DeleteObject(hIcon);
              }
          }
          break;

      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
                if (bAdmin) {
                    //
                    // save the user wait interval in the registry
                    //
                    if (IsDlgButtonChecked(hDlg, IDD_HWP_WAITFOREVER)) {
                        ulWait = 0xFFFFFFFF;
                    }
                    else {
                        ulWait = GetDlgItemInt(hDlg, IDD_HWP_SECONDS,
                            &Status, FALSE);
                        if (!Status  ||  ulWait > MAX_USER_WAIT) {
                            TCHAR szCaption[MAX_PATH];
                            TCHAR szMsg[MAX_PATH];

                            LoadString(hInstance, HWP_ERROR_CAPTION, szCaption, MAX_PATH);
                            LoadString(hInstance, HWP_INVALID_WAIT, szMsg, MAX_PATH);
        
                            MessageBox(hDlg, szMsg, szCaption,
                                       MB_OK | MB_ICONEXCLAMATION);
        
                            SetFocus(GetDlgItem(hDlg, IDD_HWP_SECONDS));
        
                            return(TRUE);
                        }
                    }
                    SetUserWaitInterval(ulWait);
    
                    //
                    // flush the pending changes in profile buffer
                    //
                    hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);
                    FlushProfileChanges(hDlg, hList);
                }
                EndDialog(hDlg, 0);
                break;

            case IDCANCEL:
                pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

                if (pInfo) {
                    //
                    // If profile modifications have already been commited from
                    // within the Property Sheet, that's okay. But if accessing
                    // Properties caused any profiles to be created then they
                    // should be removed now since the user is effectively
                    // cancelling that creation now by cancelling from the main
                    // Hardware Profiles dialog.
                    //
                    if (bAdmin) {
                        RemoveNewProfiles(pInfo);
                    }
                }
                SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                EndDialog(hDlg, 0);
                break;

            case IDD_HWP_ORDERUP:
               //
               // move selected profile "up" in preference order
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

               ulIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
               if (ulIndex == LB_ERR) {
                  break;
               }

               pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

               //
               // if we're not already at the top, swap preferences
               //
               if (ulIndex > 0) {
                  SwapPreferenceOrder(hDlg, hList, ulIndex, ulIndex-1);
                  UpdateOrderButtonState(hDlg);
                  PropSheet_Changed(GetParent(hDlg), hDlg);
               }
               break;


            case IDD_HWP_ORDERDOWN:
               //
               // move selected profile "down" in preference order
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

               ulIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
               if (ulIndex == LB_ERR) {
                  break;
               }

               pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

               //
               // if we're not already at the bottom, swap preferences
               //
               if (ulIndex < pInfo->ulNumProfiles-1) {
                  SwapPreferenceOrder(hDlg, hList, ulIndex, ulIndex+1);
                  UpdateOrderButtonState(hDlg);
                  PropSheet_Changed(GetParent(hDlg), hDlg);
               }
               break;


            case IDD_HWP_PROFILES:
               //
               // selection changed, enable/disable Delete button based
               // on whether it's the current config that is selected
               //

               if (bAdmin) {

                   if (HIWORD(wParam) == LBN_DBLCLK) {
                      SendMessage(hDlg, WM_COMMAND, MAKELONG(IDD_HWP_PROPERTIES,0), 0);
                   }
                   else if (HIWORD(wParam) == LBN_SELCHANGE) {

                      if (!GetCurrentProfile(&ulCurrentProfile)) {
                         break;
                      }

                      if ((ulIndex = SendMessage((HWND)lParam,
                            LB_GETCURSEL, 0, 0)) == LB_ERR) {
                         break;
                      }

                      if ((lValue = (LONG)SendMessage((HWND)lParam, LB_GETITEMDATA,
                            ulIndex, 0)) == LB_ERR) {
                         break;
                      }

                      if ((ULONG)lValue == ulCurrentProfile) {
                         EnableWindow(GetDlgItem(hDlg, IDD_HWP_DELETE), FALSE);
                      }
                      else {
                         EnableWindow(GetDlgItem(hDlg, IDD_HWP_DELETE), TRUE);
                      }


                      //
                      // update button enable/disable states
                      //
                      UpdateOrderButtonState(hDlg);
                   }
               }
               break;


            case IDD_HWP_WAITFOREVER:
               //
               // if user chooses wait forever, disable the seconds control
               //
               if (HIWORD(wParam) == BN_CLICKED) {
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECONDS), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECSCROLL), FALSE);
                  PropSheet_Changed(GetParent(hDlg), hDlg);
               }
               break;


            case IDD_HWP_WAITUSER:
               //
               // if user chooses a wait interval, reenable seconds control
               //
               if (HIWORD(wParam) == BN_CLICKED) {
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECONDS), TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_SECSCROLL), TRUE);
                  PropSheet_Changed(GetParent(hDlg), hDlg);
               }
               break;


            case IDD_HWP_PROPERTIES:
               //
               // retrieve the profile buffer
               //
               pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

               //
               // get the selected profile
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

               ulIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
               if (ulIndex == LB_ERR) {
                  break;
               }

               //
               // find the profile entry in the buffer that matches the selection
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);
               ulSelectedProfile = (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex, 0);

               while (ulBufferIndex < pInfo->ulNumProfiles) {
                  if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == ulSelectedProfile) {
                     break;
                  }
                  ulBufferIndex++;
               }

               //
               // commit the changes for this profile before calling Properties
               //
               WriteProfileInfo(&pInfo->pHwProfValues[ulBufferIndex]);

               //
               // pass the HWPROF_VALUES struct for the selected profile
               // to the property sheet page when it's created. The
               // property sheet may update some of these fields and
               // may also commit changes for this profile to the registry.
               //
               pInfo->ulSelectedProfileIndex = ulBufferIndex;
               pInfo->ulSelectedProfile = (ULONG)SendMessage(hList, LB_GETITEMDATA,
                                                      ulIndex, 0);

               DisplayProperties(hDlg, pInfo);
               //DisplayProperties(hDlg, &pInfo->pHwProfValues[ulBufferIndex]);
               break;


            case IDD_HWP_COPY:               
               //
               // retrieve the profile buffer
               //
               pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

               //
               // get the selected profile, this is the "From" selection
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

               ulIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
               if (ulIndex == LB_ERR) {
                  break;
               }

               SendMessage(hList, LB_GETTEXT, ulIndex,
                     (LPARAM)(LPCTSTR)HwSelectedProfile.szFromFriendlyName);
               HwSelectedProfile.ulFromProfileID =
                     (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex, 0);

               //
               // find the profile entry in the buffer that matches the selection
               //
               ulBufferIndex = 0;
               while (ulBufferIndex < pInfo->ulNumProfiles) {
                   if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == HwSelectedProfile.ulFromProfileID) {
                       break;
                   }
                   ulBufferIndex++;
               }


               //
               // determine the next suggested name for the given profile
               //
               CreateHwProfileFriendlyName(hDlg,
                                           pInfo->pHwProfValues[ulBufferIndex].ulDockState,
                                           HwSelectedProfile.szToFriendlyName);

               //
               // pass selected profile info to copy profile dialog box
               //

               HwSelectedProfile.hParent = hDlg;

               if (!DialogBoxParam(hInstance,
                        MAKEINTRESOURCE(DLG_HWP_COPY), hDlg,
                        CopyProfileDlg,
                        (LPARAM)&HwSelectedProfile)) {
                  //
                  // if returns FALSE, either user canceled or no work
                  // required
                  //
                  break;
               }

               //
               // clone the profile in the in-memory profile buffer
               // and update the display
               //
               CopyHardwareProfile(
                        hDlg,
                        ulIndex,
                        HwSelectedProfile.ulFromProfileID,
                        HwSelectedProfile.szToFriendlyName);

               UpdateOrderButtonState(hDlg);
               PropSheet_Changed(GetParent(hDlg), hDlg);
               break;


            case IDD_HWP_RENAME:
               //
               // get the selected profile
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

               ulIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
               if (ulIndex == LB_ERR) {
                  break;
               }

               SendMessage(hList, LB_GETTEXT, ulIndex,
                     (LPARAM)(LPCTSTR)HwSelectedProfile.szFromFriendlyName);
               HwSelectedProfile.ulFromProfileID =
                     (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex, 0);

               //
               // strip off the current tag if it exists "(Current)"
               //
               GetCurrentProfile(&ulCurrentProfile);

               StripCurrentTag(
                     HwSelectedProfile.szFromFriendlyName,
                     HwSelectedProfile.ulFromProfileID,
                     ulCurrentProfile);

               // pass selected profile info to rename profile dialog box
               //

               HwSelectedProfile.hParent = hDlg;

               if (!DialogBoxParam(hInstance,
                        MAKEINTRESOURCE(DLG_HWP_RENAME), hDlg,
                        RenameProfileDlg,
                        (LPARAM)&HwSelectedProfile)) {
                  //
                  // if returns FASLE, either user canceled or no work
                  // required (i.e., user chose same name or zero-length
                  // name)
                  //
                  break;
               }

               //
               // rename the profile in the in-memory profile buffer
               // and update the display
               //
               RenameHardwareProfile(
                        hDlg,
                        ulIndex,
                        HwSelectedProfile.ulFromProfileID,
                        HwSelectedProfile.szToFriendlyName);

               PropSheet_Changed(GetParent(hDlg), hDlg);
               break;


            case IDD_HWP_DELETE: {

               TCHAR szCaption[MAX_PATH];
               TCHAR szMsg[MAX_PATH];
               TCHAR szMsg1[MAX_PATH];

               //
               // get the selected profile
               //
               hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

               ulIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
               if (ulIndex == LB_ERR) {
                  break;
               }

               //
               // confirm that user really wants to delete the profile
               // (the confirm message has a substitute symbol for
               // profile name)
               //
               SendMessage(hList, LB_GETTEXT, ulIndex,
                     (LPARAM)(LPCTSTR)szProfileName);

               LoadString(hInstance, HWP_CONFIRM_DELETE_CAP, szCaption, MAX_PATH);
               LoadString(hInstance, HWP_CONFIRM_DELETE, szMsg1, MAX_PATH);

               wsprintf(szMsg, szMsg1, szProfileName);

               if (MessageBox(hDlg, szMsg, szCaption,
                              MB_YESNO | MB_ICONQUESTION) == IDNO) {
                   break;
               }

               //
               // mark the profile as deleted in the in-memory buffer
               // and update the display
               //
               DeleteHardwareProfile(
                        hDlg,
                        ulIndex);

               UpdateOrderButtonState(hDlg);
               PropSheet_Changed(GetParent(hDlg), hDlg);
               break;
             }

             case IDD_HWP_SECONDS:

                if (HIWORD(wParam) == EN_UPDATE) {
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
                break;

            default:
               return FALSE;
          }
          break;

       } // case WM_COMMAND...

       default:
          return FALSE;
          break;
    }

    return TRUE;

} // HardwareProfilesDlg



/**-------------------------------------------------------------------------**/
INT_PTR
APIENTRY
CopyProfileDlg(
      HWND    hDlg,
      UINT    uMessage,
      WPARAM  wParam,
      LPARAM  lParam
      )

{
   PHWPROFILE  pHwProfile;
   static HIMC himcOrg;


   switch (uMessage)
   {
      case WM_INITDIALOG:
         //
         // the profile info struct is passed in lparam, save in
         // Window word for thread-safe use in later messages
         //
         SetWindowLongPtr(hDlg, DWLP_USER, lParam);
         pHwProfile = (PHWPROFILE)lParam;

         //
         // initialize "To" and "From" fields
         //
         SendDlgItemMessage(hDlg, IDD_HWP_COPYTO, EM_LIMITTEXT,
               MAX_FRIENDLYNAME_LEN-1, 0L);
         SetDlgItemText(hDlg, IDD_HWP_COPYFROM, pHwProfile->szFromFriendlyName);
         SetDlgItemText(hDlg, IDD_HWP_COPYTO, pHwProfile->szToFriendlyName);
         SendDlgItemMessage(hDlg, IDD_HWP_COPYTO, EM_SETSEL, 0, -1);
         SetFocus(GetDlgItem(hDlg, IDD_HWP_COPYTO));
         //
         // Remove any association the window may have with an input context,
         // because we don't allow to use DBCS in H/W profile name.
         //
         himcOrg = ImmAssociateContext(GetDlgItem(hDlg, IDD_HWP_COPYTO), (HIMC)NULL);
         return FALSE;


      case WM_DESTROY:
         if (himcOrg)
           ImmAssociateContext(GetDlgItem(hDlg, IDD_HWP_COPYTO), himcOrg);
         return FALSE;

      case WM_HELP:
         WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, HELP_FILE,
               HELP_WM_HELP, (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;

      case WM_CONTEXTMENU:
         WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
                  (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;


      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
               pHwProfile = (PHWPROFILE)GetWindowLongPtr(hDlg, DWLP_USER);

               GetDlgItemText(hDlg, IDD_HWP_COPYTO,
                     pHwProfile->szToFriendlyName, MAX_FRIENDLYNAME_LEN);


               if (pHwProfile->szToFriendlyName == NULL ||
                        *pHwProfile->szToFriendlyName == '\0') {
                  //
                  // accept request to copy to zero-length string but
                  // do nothing (return FALSE from DialogBox call)
                  //
                  EndDialog(hDlg, FALSE);
                  return TRUE;
               }

               //
               // validate that string contains only ASCII characters
               //
               if (!ValidateAsciiString(pHwProfile->szToFriendlyName)) {
                  DisplayPrivateMessage(hDlg, HWP_ERROR_INVALID_CHAR);
                  break;
               }

               //
               // Check for duplicates
               //

               if (IsProfileNameInUse(pHwProfile->hParent,
                                      pHwProfile->szToFriendlyName)) {
                  //
                  // if name already used by a different profile (including
                  // the name of this profile), deny the request, but don't
                  // end the dialog box
                  //
                  DisplayPrivateMessage(hDlg, HWP_ERROR_PROFILE_IN_USE);
                  break;
               }

               //
               // otherwise, we'll accept the name
               //
               EndDialog(hDlg,TRUE);
               break;

            case IDCANCEL:
               EndDialog(hDlg,FALSE);
               break;

            default:
               return FALSE;
          }
          break;

       } // case WM_COMMAND...

       default:
          return FALSE;
          break;
    }

    return TRUE;

} // CopyProfileDlg



/**-------------------------------------------------------------------------**/
INT_PTR
APIENTRY
RenameProfileDlg(
      HWND    hDlg,
      UINT    uMessage,
      WPARAM  wParam,
      LPARAM  lParam
      )

{
   PHWPROFILE  pHwProfile;
   ULONG       ulReturn;
   static HIMC himcOrg;


   switch (uMessage)
   {
      case WM_INITDIALOG:
         //
         // the profile info struct is passed in lparam, save in
         // Window word for thread-safe use in later messages
         //
         SetWindowLongPtr(hDlg, DWLP_USER, lParam);
         pHwProfile = (PHWPROFILE)lParam;

         //
         // initialize "To" and "From" fields
         //
         SendDlgItemMessage(hDlg, IDD_HWP_RENAMETO, EM_LIMITTEXT,
               MAX_FRIENDLYNAME_LEN-1, 0L);
         SetDlgItemText(hDlg, IDD_HWP_RENAMEFROM, pHwProfile->szFromFriendlyName);
         SetDlgItemText(hDlg, IDD_HWP_RENAMETO, pHwProfile->szFromFriendlyName);
         SendDlgItemMessage(hDlg, IDD_HWP_RENAMETO, EM_SETSEL, 0, -1);
         SetFocus(GetDlgItem(hDlg, IDD_HWP_RENAMETO));
         //
         // Remove any association the window may have with an input context,
         // because we don't allow to use DBCS in H/W profile name.
         //
         himcOrg = ImmAssociateContext(GetDlgItem(hDlg, IDD_HWP_RENAMETO), (HIMC)NULL);
         return FALSE;


      case WM_DESTROY:
         if (himcOrg)
           ImmAssociateContext(GetDlgItem(hDlg, IDD_HWP_RENAMETO), himcOrg);
         return FALSE;

      case WM_HELP:
         WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, HELP_FILE,
               HELP_WM_HELP, (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;

      case WM_CONTEXTMENU:
         WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
                  (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;


      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
               pHwProfile = (PHWPROFILE)GetWindowLongPtr(hDlg, DWLP_USER);

               ulReturn = GetDlgItemText(hDlg, IDD_HWP_RENAMETO,
                     pHwProfile->szToFriendlyName, MAX_FRIENDLYNAME_LEN);


               if (pHwProfile->szToFriendlyName == NULL ||
                        *pHwProfile->szToFriendlyName == '\0') {
                  //
                  // accept request to copy to zero-length string but
                  // do nothing (return FALSE from DialogBox call)
                  //
                  EndDialog(hDlg, FALSE);
                  return TRUE;
               }

               if (lstrcmpi(pHwProfile->szToFriendlyName,
                        pHwProfile->szFromFriendlyName) == 0) {
                  //
                  // accept request to rename to same name but do
                  // nothing (return FALSE from DialogBox call)
                  //
                  EndDialog(hDlg, FALSE);
                  return TRUE;
               }

               //
               // validate that string contains only ASCII characters
               //
               if (!ValidateAsciiString(pHwProfile->szToFriendlyName)) {
                  DisplayPrivateMessage(hDlg, HWP_ERROR_INVALID_CHAR);
                  break;
               }

               //
               // Check for duplicates
               //

               if (IsProfileNameInUse(pHwProfile->hParent,
                                      pHwProfile->szToFriendlyName)) {
                  //
                  // if name already used by a different profile, deny
                  // the request, but don't end the dialog box
                  //
                  DisplayPrivateMessage(hDlg, HWP_ERROR_PROFILE_IN_USE);
                  break;
               }

               //
               // otherwise, we'll accept the name
               //
               EndDialog(hDlg,TRUE);
               break;

            case IDCANCEL:
               EndDialog(hDlg,FALSE);
               break;

            default:
               return FALSE;
          }
          break;

       } // case WM_COMMAND...

       default:
          return FALSE;
          break;
    }

    return TRUE;

} // RenameProfileDlg




/**-------------------------------------------------------------------------**/
BOOL
GetCurrentProfile(
      PULONG  pulProfile
      )
{
   WCHAR    RegStr[MAX_PATH];
   ULONG    ulSize;
   HKEY     hKey;


   //
   // open the IDConfigDB key
   //
   if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
            KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {

      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }

   //
   // retrieve the CurrentConfig value
   //
   ulSize = sizeof(ULONG);
   if (RegQueryValueEx(
            hKey, pszRegCurrentConfig, NULL, NULL,
            (LPBYTE)pulProfile, &ulSize) != ERROR_SUCCESS) {

      RegCloseKey(hKey);
      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }

   RegCloseKey(hKey);
   return TRUE;

} // GetCurrentProfile


/**-------------------------------------------------------------------------**/
BOOL
GetRegProfileCount(
      PULONG   pulProfiles
      )
{
   WCHAR    RegStr[MAX_PATH];
   HKEY     hKey;


   //
   // open the Known Docking States key
   //
   wsprintf(RegStr, TEXT("%s\\%s"),
            pszRegIDConfigDB,
            pszRegKnownDockingStates);

   if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0, KEY_READ,
            &hKey) != ERROR_SUCCESS) {

      *pulProfiles = 0;
      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }

   //
   // find out the total number of profiles
   //
   if (RegQueryInfoKey(
            hKey, NULL, NULL, NULL, pulProfiles, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {

      *pulProfiles = 0;
      RegCloseKey(hKey);
      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }

   ASSERT(*pulProfiles > 0);  // The key for the pristine profile should be there, at least.
   *pulProfiles-= 1;          // Don't count the pristine in the number or working profiles.

   RegCloseKey(hKey);
   return TRUE;

} // GetRegProfileCount


/**-------------------------------------------------------------------------**/
BOOL
GetSelectedProfile(
      HWND     hCtl,
      PULONG   pulSelectedProfile
      )
{
    LONG lSelect;

    // THIS ISN"T BEING CALLED RIGHT NOW, IF IT STARTS GETTING CALLED,
    // THERE SHOULD BE LOGIC FOR REENABLING OR DISABLING THE DELETE
    // BUTTON BASED ON WHETHER THE FIRST INDEX IS THE CURRENT PROFILE
    // (ONLY APPLIES TO THE ERROR CASE WHERE THERE ARE NO CURRENT
    // SELECTIONS)

    lSelect = (LONG)SendMessage(hCtl, LB_GETCURSEL, 0, 0);

    if (lSelect != LB_ERR) {

      *pulSelectedProfile =
         (ULONG)SendMessage(hCtl, LB_GETITEMDATA, lSelect, 0);
    }
    else {
       //
       // no selections, assume first one
       //
       SendMessage(hCtl, LB_SETCURSEL, 0, 0);

       *pulSelectedProfile = (ULONG)SendMessage(hCtl, LB_GETITEMDATA, 0, 0);
    }

    return TRUE;

} // GetSelectedProfile



/**-------------------------------------------------------------------------**/
BOOL
FillProfileList(
      HWND  hDlg
      )

{
   HWND           hList;
   ULONG          ulCurrentProfile, ulCurrentIndex;
   ULONG          ulIndex=0, ulSize=0;
   ULONG          enumIndex = 0, ulProfileID=0;
   ULONG          ulCurrentDockingState = 0;
   ULONG          ulDockID = 0, ulSerialNumber=0;
   HKEY           hKey = NULL, hCfgKey = NULL;
   HKEY           hCurrent = NULL;
   WCHAR          RegStr[MAX_PATH], szName[MAX_PATH];
   ULONG          RegStatus = ERROR_SUCCESS;
   WCHAR          szFriendlyName[MAX_FRIENDLYNAME_LEN];
   WCHAR          szProfile[MAX_PROFILEID_LEN];
   HLOCAL         hMem;
   PHWPROF_INFO   pInfo;
   LRESULT        lReturn;
   REGSAM         sam;


   //
   // retrieve a handle to the listbox window
   //
   hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

   //
   // retrieve the id of the current profile
   //
   if (!GetCurrentProfile(&ulCurrentProfile)) {
      DisplaySystemMessage(hDlg, ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   //
   // allocate a buffer for the main profile info struct
   //
   hMem = LocalAlloc(LHND, sizeof(HWPROF_INFO));

   if (hMem == NULL) {
      DisplaySystemMessage(hDlg, ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   pInfo = (PHWPROF_INFO)LocalLock(hMem);

   //
   // save the number of profiles currently in the registry
   //
   if (!GetRegProfileCount(&(pInfo->ulNumProfiles))) {
      LocalFree(hMem);
      return FALSE;
   }

   pInfo->ulActiveProfiles = pInfo->ulNumProfiles;

   //
   // Initialize the hardware detected portable flag
   //
   pInfo->bHwDetectedPortable = FALSE;

   //
   // allocate a buffer to hold all the profile values
   //
   hMem = LocalAlloc(LHND, sizeof(HWPROF_VALUES) * pInfo->ulNumProfiles);

   if (hMem == NULL) {
      LocalUnlock(LocalHandle(pInfo));
      LocalFree(LocalHandle(pInfo));
      return FALSE;
   }

   pInfo->pHwProfValues = (PHWPROF_VALUES)LocalLock(hMem);

   SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pInfo);

   //
   // clear the listbox and turn redraw off
   //
   SendMessage(hList, LB_RESETCONTENT, 0, 0);
   SendMessage(hList, WM_SETREDRAW, (WPARAM)FALSE, 0);

   //
   // open the Hardware Profiles key
   //
   wsprintf(RegStr, TEXT("%s\\%s"),
            pszRegIDConfigDB,
            pszRegKnownDockingStates);

   if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0,
            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
            &hKey) != ERROR_SUCCESS) {

      DisplaySystemMessage(hDlg, ERROR_REGISTRY_CORRUPT);
      LocalUnlock(LocalHandle(pInfo->pHwProfValues));
      LocalFree(LocalHandle(pInfo->pHwProfValues));
      LocalUnlock(LocalHandle(pInfo));
      LocalFree(LocalHandle(pInfo));
      return FALSE;
   }

   //
   // read the values for the name generating counters
   //
   ulSize = sizeof(DWORD);

   wsprintf(szName, pszRegUndocked);
   if (RegQueryValueEx(hKey, szName, NULL, NULL,
                       (LPBYTE)&pInfo->ulUndockedProfileNameCount, &ulSize)) {
       pInfo->ulUnknownProfileNameCount = 0;
   }

   wsprintf(szName, pszRegDocked);
   if (RegQueryValueEx(hKey, szName, NULL, NULL,
                       (LPBYTE)&pInfo->ulDockedProfileNameCount, &ulSize)) {
       pInfo->ulUnknownProfileNameCount = 0;
   }

   wsprintf(szName, pszRegUnknown);
   if (RegQueryValueEx(hKey, szName, NULL, NULL,
                       (LPBYTE)&pInfo->ulUnknownProfileNameCount, &ulSize)) {
       pInfo->ulUnknownProfileNameCount = 0;
   }

   //
   // pad the list box with a blank entry for each profile
   // (this facilitates adding the profiles in rank order
   //
   for (ulIndex = 0; ulIndex < pInfo->ulNumProfiles; ulIndex++) {
      SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)TEXT(" "));
   }

   //
   // enumerate each of the existing hardware profiles
   //
   ulIndex = 0;
   enumIndex = 0;
   while (RegStatus != ERROR_NO_MORE_ITEMS) {

      //
      // enumerate the profile key
      //
      ulSize = MAX_PROFILEID_LEN;
      RegStatus = RegEnumKeyEx(
               hKey, enumIndex, szProfile, &ulSize, NULL, NULL, NULL, NULL);

      if (RegStatus == ERROR_SUCCESS) {
         //
         // open the enumerated profile key
         //
         if (bAdmin) {
             sam = KEY_QUERY_VALUE | KEY_SET_VALUE;
         } else {
             sam = KEY_QUERY_VALUE;
         }

         if (RegOpenKeyEx(
                  hKey, szProfile, 0, sam, &hCfgKey) != ERROR_SUCCESS) {

            RegCloseKey(hKey);
            LocalUnlock(LocalHandle(pInfo->pHwProfValues));
            LocalFree(LocalHandle(pInfo->pHwProfValues));
            LocalUnlock(LocalHandle(pInfo));
            LocalFree(LocalHandle(pInfo));
            if (bAdmin) {
                DisplaySystemMessage(hDlg, ERROR_REGISTRY_CORRUPT);
                return FALSE;
            }
            else {
                return TRUE;
            }
         }

         //
         // if this is the Pristine profile, ignore it, and move on to the next.
         //

         ulProfileID = _wtoi(szProfile);

         if (!ulProfileID) {
             enumIndex++;
             RegCloseKey(hCfgKey);
             continue;
         }

         //----------------------------------------------------------
         // retrieve the profile registry info, save in buffer
         //----------------------------------------------------------

         //
         // aliasable
         //
         ulSize = sizeof(DWORD);
         if (RegQueryValueEx(
             hCfgKey, pszRegAliasable, NULL, NULL,
             (LPBYTE)&pInfo->pHwProfValues[ulIndex].bAliasable,
             &ulSize) != ERROR_SUCCESS) {
             pInfo->pHwProfValues[ulIndex].bAliasable = TRUE;
         }

         //
         // cloned
         //
         ulSize = sizeof(DWORD);
         if (RegQueryValueEx(
             hCfgKey, pszRegCloned, NULL, NULL,
             (LPBYTE)&pInfo->pHwProfValues[ulIndex].bCloned,
             &ulSize) != ERROR_SUCCESS) {
             pInfo->pHwProfValues[ulIndex].bCloned = FALSE;
         }

         //
         // friendly name
         //
         ulSize = MAX_FRIENDLYNAME_LEN * sizeof(TCHAR);
         if (RegQueryValueEx(
                  hCfgKey, pszRegFriendlyName, NULL, NULL,
                  (LPBYTE)&pInfo->pHwProfValues[ulIndex].szFriendlyName,
                  &ulSize) != ERROR_SUCCESS) {

            //
            // if no FriendlyName then write out and use a default
            // value name (for compatibility with Win95)
            //
            if (bAdmin) {

                lstrcpy(pInfo->pHwProfValues[ulIndex].szFriendlyName,
                         pszRegDefaultFriendlyName);

                RegSetValueEx(
                         hCfgKey, pszRegFriendlyName, 0, REG_SZ,
                         (LPBYTE)pszRegDefaultFriendlyName,
                         (lstrlen(pszRegDefaultFriendlyName)+1) * sizeof(TCHAR));
            }
         }

         //
         // preference order ranking
         //
         ulSize = sizeof(ULONG);
         if (RegQueryValueEx(
                  hCfgKey, pszRegPreferenceOrder, NULL, NULL,
                  (LPBYTE)&pInfo->pHwProfValues[ulIndex].ulPreferenceOrder,
                  &ulSize) != ERROR_SUCCESS) {

            // BUGBUG - rerank all profiles if this happens
         }

         //
         // dock state
         //
         ulSize = sizeof(ULONG);
         if (RegQueryValueEx(
                  hCfgKey, pszRegDockState, NULL, NULL,
                  (LPBYTE)&pInfo->pHwProfValues[ulIndex].ulDockState,
                  &ulSize) != ERROR_SUCCESS) {

            pInfo->pHwProfValues[ulIndex].ulDockState =
                     DOCKINFO_USER_SUPPLIED | DOCKINFO_DOCKED | DOCKINFO_UNDOCKED;
         }

         //
         // portable computer flag  - this is obsolete info, just save the current
         // setting if it exists and then delete it (might need the original
         // setting later)
         //
         ulSize = sizeof(ULONG);
         if (RegQueryValueEx(
                  hCfgKey, pszRegIsPortable, NULL, NULL,
                  (LPBYTE)&pInfo->pHwProfValues[ulIndex].bPortable,
                  &ulSize) != ERROR_SUCCESS) {

            pInfo->pHwProfValues[ulIndex].bPortable = FALSE;
         }

         RegDeleteValue(hCfgKey, pszRegIsPortable);

         pInfo->pHwProfValues[ulIndex].ulProfile = _wtoi(szProfile);
         pInfo->pHwProfValues[ulIndex].ulAction = HWP_NO_ACTION;
         RegCloseKey(hCfgKey);

         //
         // If this is the current profile, open the CurrentDockInfo key
         //
         if (pInfo->pHwProfValues[ulIndex].ulProfile == ulCurrentProfile) {

             wsprintf(RegStr, TEXT("%s\\%s"),
                      pszRegIDConfigDB,
                      pszRegCurrentDockInfo);
             
             if (RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE, RegStr, 0,
                 KEY_QUERY_VALUE, &hCurrent) != ERROR_SUCCESS) {
                 
                 //
                 // Could not open the CurrentDockInfo key;
                 // Dock ID and Serial Number are unavailable
                 //
                 
                 pInfo->pHwProfValues[ulIndex].szDockID[0] = TEXT('\0');
                 pInfo->pHwProfValues[ulIndex].szSerialNumber[0] = TEXT('\0');

             } else {
                 
                 //
                 // Retrieve the hardware-detected dock state for the current profile
                 //
                 ulSize = sizeof(DWORD);
                 if ((RegQueryValueEx(
                     hCurrent, pszRegDockingState, NULL, NULL,
                     (LPBYTE)&ulCurrentDockingState,
                     &ulSize) == ERROR_SUCCESS) && 
                     ulCurrentDockingState &&
                     !((ulCurrentDockingState & DOCKINFO_DOCKED) &&
                       (ulCurrentDockingState & DOCKINFO_UNDOCKED))) {
                     //
                     // if the hardware-detected dockstate is present and known,
                     // override the user-supplied dockstate
                     //
                     pInfo->bHwDetectedPortable = TRUE;
                     pInfo->pHwProfValues[ulIndex].ulDockState = ulCurrentDockingState;
                 } else {
                     //
                     // keep the user-supplied dockstate
                     //
                     pInfo->bHwDetectedPortable = FALSE;
                     ulCurrentDockingState = pInfo->pHwProfValues[ulIndex].ulDockState;
                 }

                 if ((ulCurrentDockingState & DOCKINFO_UNDOCKED) &&
                     !(ulCurrentDockingState & DOCKINFO_DOCKED) &&
                     !(ulCurrentDockingState & DOCKINFO_USER_SUPPLIED)) {
                     //
                     // The hardware has detected an undocked state; set the global IsPortable flag
                     //
                     pInfo->bPortable = TRUE;

                 } else if ((ulCurrentDockingState & DOCKINFO_DOCKED) &&
                     !(ulCurrentDockingState & DOCKINFO_UNDOCKED) &&
                     !(ulCurrentDockingState & DOCKINFO_USER_SUPPLIED)) {                     
                     //
                     // The hardware has detected a docked state; set the global IsPortable flag
                     //
                     pInfo->bPortable = TRUE;
                     
                     //
                     // Serial Number and DockID are only valid for docked Profiles
                     //

                     //
                     // Retrieve the Serial Number for the current Profile.
                     //
                     // ("AcpiSerialNumber" is preferred because it is updated
                     // on Acpi dock events; "SerialNumber" is set only at boot
                     // time, from the BIOS, and may be inaccurate across Acpi
                     // dock transitions)
                     //
                     ulSize = MAX_SERIALNUM_LEN * sizeof(TCHAR);
                     if (RegQueryValueEx(
                         hCurrent, pszRegAcpiSerialNumber, NULL, NULL,
                         (LPBYTE)&pInfo->pHwProfValues[ulIndex].szSerialNumber,
                         &ulSize) != ERROR_SUCCESS) {
                         //
                         // No Acpi Serial Number, but we know the machine is
                         // docked (based on the DockState from the PnP BIOS
                         // info) so check for a PnP BIOS SerialNumber
                         //
                         ulSize = sizeof(DWORD);
                         if (RegQueryValueEx(
                             hCurrent, pszRegSerialNumber, NULL, NULL,
                             (LPBYTE)&ulSerialNumber,
                             &ulSize) != ERROR_SUCCESS) {                             
                             pInfo->pHwProfValues[ulIndex].szSerialNumber[0] = TEXT('\0');
                         } else {
                             wsprintf(pInfo->pHwProfValues[ulIndex].szSerialNumber,
                                      L"%X", 
                                      (ULONG)ulSerialNumber);
                         }
                     }
                     
                     //
                     // Retrieve the DockID for the current Profile, if available.
                     //
                     ulSize = sizeof(DWORD);
                     if (RegQueryValueEx(
                         hCurrent, pszRegDockID, NULL, NULL,
                         (LPBYTE)&ulDockID,
                         &ulSize) != ERROR_SUCCESS) {                 
                         pInfo->pHwProfValues[ulIndex].szDockID[0] = TEXT('\0');
                     } else {
                         wsprintf(pInfo->pHwProfValues[ulIndex].szDockID, 
                                  L"%X", 
                                  (ULONG)ulDockID);
                     }
                     
                 }
                 
             }
             
         } else {
             //
             // Serial Number and DockID are only valid for the current Profile
             //         
             pInfo->pHwProfValues[ulIndex].szSerialNumber[0] = TEXT('\0');
             pInfo->pHwProfValues[ulIndex].szDockID[0] = TEXT('\0');
         }
         
         //
         // delete the blank string in this spot, add the friendly name
         // (append current tag if necessary)
         //
         SendMessage(hList, LB_DELETESTRING,
                  pInfo->pHwProfValues[ulIndex].ulPreferenceOrder, 0);

         AppendCurrentTag(
                  szName,        // new fixed up name
                  pInfo->pHwProfValues[ulIndex].szFriendlyName,
                  pInfo->pHwProfValues[ulIndex].ulProfile,
                  ulCurrentProfile);

         lReturn = SendMessage(hList, LB_INSERTSTRING,
                  pInfo->pHwProfValues[ulIndex].ulPreferenceOrder,
                  (LPARAM)(LPCTSTR)szName);


         //
         // store the profile id along with the entry so we
         // can associate the string and the profile id later
         //
         SendMessage(hList, LB_SETITEMDATA,
            (WPARAM)lReturn, pInfo->pHwProfValues[ulIndex].ulProfile);

         //
         // if this is the current profile, save the index
         //
         if (pInfo->pHwProfValues[ulIndex].ulProfile == ulCurrentProfile) {
            ulCurrentIndex = pInfo->pHwProfValues[ulIndex].ulPreferenceOrder;
         }
      }

      ulIndex++;
      enumIndex++;

   } // while

   RegCloseKey(hKey);

   //---------------------------------------------------------------------
   // migrate portable information
   //---------------------------------------------------------------------
   
   if (bAdmin) {
       sam = KEY_READ | KEY_WRITE;
   } else {
       sam = KEY_READ;
   }

   if (RegOpenKeyEx(
       HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
       sam, &hKey) == ERROR_SUCCESS) {

       if (pInfo->bHwDetectedPortable) {
           if (bAdmin) {
               //
               // Set the global IsPortable setting as identified by the Hardware
               //
               RegSetValueEx(hKey, pszRegIsPortable, 0, REG_DWORD,
                             (LPBYTE)&pInfo->bPortable, sizeof(DWORD));
           }
           
       } else {
           //
           // Is there a global IsPortable setting?
           //
           ulSize = sizeof(DWORD);
           if (RegQueryValueEx(hKey, pszRegIsPortable, NULL, NULL,
                               (LPBYTE)&pInfo->bPortable, &ulSize) != ERROR_SUCCESS) {
               //
               // the global IsPortable flag isn't there, and the hardware did
               // not detect this as being a portable machine, so by default, it
               // is not.
               //
               pInfo->bPortable = FALSE;
           }
       }
       
       RegCloseKey(hKey);
   }

   SendMessage(hList, WM_SETREDRAW, (WPARAM)TRUE, 0);
   SendMessage(hList, LB_SETCURSEL, ulCurrentIndex, 0);

   return TRUE;

} // FillProfileList



/**-------------------------------------------------------------------------**/
BOOL
IsProfileNameInUse(
      HWND     hDlg,
      LPTSTR   pszFriendlyName
      )
{
   ULONG          ulBufferIndex=0;
   PHWPROF_INFO   pInfo=NULL;


   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   //
   // check each friendly name (that hasn't been deleted) for a
   // match (case-insensitive)
   //
   while (ulBufferIndex < pInfo->ulNumProfiles) {

      if (!(pInfo->pHwProfValues[ulBufferIndex].ulAction & HWP_DELETE)) {

         if (lstrcmpi(pInfo->pHwProfValues[ulBufferIndex].szFriendlyName,
                  pszFriendlyName) == 0) {
            return TRUE;      // match, name is in use
         }
      }
      ulBufferIndex++;
   }

   return FALSE;  // no match found, name not in use

} // IsProfileNameInUse



/**-------------------------------------------------------------------------**/
VOID
UnicodeToUL(LPWSTR pString, PULONG pulValue)
{
    ULONG i;

    *pulValue = 0;
    for (i = 0; i < 4; i++) {
        *pulValue *= 10;
        *pulValue += (pString[i] <= '9') ?
                     (pString[i] - '0') :
                     (pString[i]-'A'+10);
    }
} // UnicodeToUL



/**-------------------------------------------------------------------------**/
BOOL
CopyHardwareProfile(
      HWND   hDlg,
      ULONG_PTR ulIndex,
      ULONG  ulProfile,
      LPTSTR szNewFriendlyName
      )
{
   HWND           hList;
   ULONG          ulBufferIndex=0, ulNewBufferIndex=0;
   ULONG          ulNewProfile=0;
   ULONG_PTR      ulNewIndex=0;
   PHWPROF_INFO   pInfo=NULL;
   HLOCAL         hMem=NULL;
   WCHAR          szTemp[MAX_PATH];
   HKEY           hKey;
   LONG           RegStatus;


   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   //
   // retrieve a handle to the listbox window
   //
   hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

   //
   // find which entry in the buffer list matches this profile
   //
   while (ulBufferIndex < pInfo->ulNumProfiles) {
      if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == ulProfile) {
         break;
      }
      ulBufferIndex++;
   }

   //
   // reallocate the profile buffer to hold another entry
   //
   pInfo->ulActiveProfiles++;
   pInfo->ulNumProfiles++;

   LocalUnlock(LocalHandle(pInfo->pHwProfValues));

   hMem = (PHWPROF_VALUES)LocalReAlloc(
               LocalHandle(pInfo->pHwProfValues),
               pInfo->ulNumProfiles * sizeof(HWPROF_VALUES),
               LMEM_MOVEABLE | LMEM_ZEROINIT);

   if (hMem == NULL) {
      DisplaySystemMessage(hDlg, ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   pInfo->pHwProfValues = (PHWPROF_VALUES)LocalLock(hMem);
   ulNewBufferIndex = pInfo->ulNumProfiles-1;

   //
   // find a free profile id to use
   //
   if (!GetFreeProfileID(pInfo, &ulNewProfile)) {
      return FALSE;
   }

   pInfo->pHwProfValues[ulNewBufferIndex].ulProfile = ulNewProfile;

   //
   // save the friendly name retrieved from the copy dialog box
   //
   lstrcpy(pInfo->pHwProfValues[ulNewBufferIndex].szFriendlyName,
            szNewFriendlyName);

   //
   // assume it's the last in the preference order (zero-based)
   //
   pInfo->pHwProfValues[ulNewBufferIndex].ulPreferenceOrder =
               pInfo->ulActiveProfiles - 1;

   //
   // copy the profile info from the selected profile to the new profile
   //
   pInfo->pHwProfValues[ulNewBufferIndex].ulDockState =
               pInfo->pHwProfValues[ulBufferIndex].ulDockState;

   //
   // new hardware profiles are (by default) not aliasable, unless no alias can
   // be copied for this profile.
   //
   pInfo->pHwProfValues[ulNewBufferIndex].bAliasable = FALSE;

   //
   // copied profiles are not clones of the pristine profile.
   //
   pInfo->pHwProfValues[ulNewBufferIndex].bCloned = FALSE;

   //pInfo->pHwProfValues[ulNewBufferIndex].bPortable =
   //            pInfo->pHwProfValues[ulBufferIndex].bPortable;

   lstrcpy(pInfo->pHwProfValues[ulNewBufferIndex].szDockID,
           pInfo->pHwProfValues[ulBufferIndex].szDockID);

   lstrcpy(pInfo->pHwProfValues[ulNewBufferIndex].szSerialNumber,
           pInfo->pHwProfValues[ulBufferIndex].szSerialNumber);

   //
   // save the original profile id this was copied from
   //
   pInfo->pHwProfValues[ulNewBufferIndex].ulCreatedFrom =
               GetOriginalProfile(pInfo, ulProfile, ulBufferIndex);

   //
   // increment the appropriate name-generating counter
   // (note that the counter is NOT symmetrically decreased whenever a profile
   // is deleted -- it increases over the lifetime of the system as new profiles
   // are created; this prevents us from assigning names that incrementally less
   // than names we have already assigned.)
   //
   AdjustProfileTypeCounter(pInfo,
                            pInfo->pHwProfValues[ulNewBufferIndex].ulDockState, 
                            TRUE);

   //
   // set the new profile in the listbox (at the end)
   //
   ulNewIndex = SendMessage(hList, LB_ADDSTRING, 0,
               (LPARAM)(LPTSTR)szNewFriendlyName);

   SendMessage(hList, LB_SETITEMDATA,
               (WPARAM)ulNewIndex,
               pInfo->pHwProfValues[ulNewBufferIndex].ulProfile);

   //
   // select the new profile
   //
   SendMessage(hList, LB_SETCURSEL, ulNewIndex, 0);

   //
   // mark the change
   //
   pInfo->pHwProfValues[ulNewBufferIndex].ulAction |= HWP_CREATE;

   //
   // disable copy if we're now at the max number of profiles
   //
   if ((pInfo->ulNumProfiles+1) >= MAX_PROFILES) {
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_COPY), FALSE);
   }
   
   //
   // reenable delete since by definition the selection is not on the
   // current profile (whether it was before or not)
   //
   EnableWindow(GetDlgItem(hDlg, IDD_HWP_DELETE), TRUE);

   return TRUE;

} // CopyHardwareProfile



/**-------------------------------------------------------------------------**/
BOOL
RenameHardwareProfile(
      HWND   hDlg,
      ULONG_PTR ulIndex,
      ULONG  ulProfile,
      LPTSTR szNewFriendlyName
      )
{
   HWND           hList;
   ULONG          ulBufferIndex=0, ulCurrentProfile=0;
   PHWPROF_INFO   pInfo=NULL;
   WCHAR          szName[MAX_PATH];


   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   //
   // retrieve a handle to the listbox window
   //
   hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

   //
   // find the profile entry in the buffer that matches the selection
   //
   while (ulBufferIndex < pInfo->ulNumProfiles) {
      if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == ulProfile) {
         break;
      }
      ulBufferIndex++;
   }

   //
   // set the new friendly name in the listbox
   //
   GetCurrentProfile(&ulCurrentProfile);
   AppendCurrentTag(szName, szNewFriendlyName, ulProfile, ulCurrentProfile);

   SendMessage(hList, LB_DELETESTRING, ulIndex, 0);
   SendMessage(hList, LB_INSERTSTRING, ulIndex, (LPARAM)(LPTSTR)szName);
   SendMessage(hList, LB_SETITEMDATA, ulIndex,
               pInfo->pHwProfValues[ulIndex].ulProfile);

   //
   // re-select the index (is this necessary?)
   //
   SendMessage(hList, LB_SETCURSEL, ulIndex, 0);

   //
   // mark the change
   //
   pInfo->pHwProfValues[ulBufferIndex].ulAction |= HWP_RENAME;
   lstrcpy(pInfo->pHwProfValues[ulBufferIndex].szFriendlyName,
            szNewFriendlyName);

   return TRUE;

} // RenameHardwareProfile



/**-------------------------------------------------------------------------**/
BOOL
DeleteHardwareProfile(
      HWND   hDlg,
      ULONG_PTR ulIndex
      )
{
   HWND           hList;
   ULONG          ulBufferIndex=0, ulProfile=0, ulCurrentProfile=0;
   PHWPROF_INFO   pInfo=NULL;


   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   //
   // retrieve a handle to the listbox window
   //
   hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

   //
   // find the profile entry in the buffer that matches the selection
   //
   ulProfile = (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex, 0);

   while (ulBufferIndex < pInfo->ulNumProfiles) {
      if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == ulProfile) {
         break;
      }
      ulBufferIndex++;
   }

   //
   // readjust all the rankings to be consecutive
   //
   DeleteRank(pInfo, pInfo->pHwProfValues[ulBufferIndex].ulPreferenceOrder);

   //
   // decrement the count of active profiles
   //
   pInfo->ulActiveProfiles--;

   //
   // delete the friendly name in the listbox
   //
   SendMessage(hList, LB_DELETESTRING, ulIndex, 0);

   //
   // re-select the following index (same position)
   //
   if (ulIndex >= pInfo->ulActiveProfiles) {
      ulIndex = pInfo->ulActiveProfiles-1;
   }

   SendMessage(hList, LB_SETCURSEL, ulIndex, 0);

   //
   // mark the change
   //
   pInfo->pHwProfValues[ulBufferIndex].ulAction |= HWP_DELETE;

   //
   // enable copy if less than max number of profiles
   //
   if (pInfo->ulNumProfiles < MAX_PROFILES) {
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_COPY), TRUE);
   }

   //
   // find the profile entry in the buffer that matches the new selection
   //
   ulProfile = (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex, 0);
   ulBufferIndex = 0;

   while (ulBufferIndex < pInfo->ulNumProfiles) {
      if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == ulProfile) {
         break;
      }
      ulBufferIndex++;
   }

   GetCurrentProfile(&ulCurrentProfile);

   //
   // if the newly selected entry is the current profile, disable delete
   //
   if (pInfo->pHwProfValues[ulBufferIndex].ulProfile == ulCurrentProfile) {
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_DELETE), FALSE);
   }


   return TRUE;

} // DeleteHardwareProfiles



/**-------------------------------------------------------------------------**/
BOOL
GetUserWaitInterval(
      PULONG   pulWait
      )
{
   ULONG    ulSize;
   HKEY     hKey;


   //
   // open the IDConfigDB key
   //
   if(RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
            KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {

      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }

   //
   // retrieve the UserWaitInterval value
   //
   ulSize = sizeof(ULONG);
   if (RegQueryValueEx(
            hKey, pszRegUserWaitInterval, NULL, NULL,
            (LPBYTE)pulWait, &ulSize) != ERROR_SUCCESS) {
      *pulWait = DEFAULT_USER_WAIT;
   }

   RegCloseKey(hKey);
   return TRUE;

} // GetUserWaitInterval



/**-------------------------------------------------------------------------**/
BOOL
SetUserWaitInterval(
      ULONG   ulWait
      )
{
   HKEY     hKey;


   if (bAdmin) {
       //
       // open the IDConfigDB key
       //
       if(RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
                KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {

          DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
          return FALSE;
       }

       //
       // set the UserWaitInterval value
       //
       if (RegSetValueEx(
                hKey, pszRegUserWaitInterval, 0, REG_DWORD,
                (LPBYTE)&ulWait, sizeof(ULONG)) != ERROR_SUCCESS) {
          RegCloseKey(hKey);
          return FALSE;
       }

       RegCloseKey(hKey);
   }

   return TRUE;

} // SetUserWaitInterval


/**-------------------------------------------------------------------------**/
BOOL
GetFreeProfileID(
      PHWPROF_INFO   pInfo,
      PULONG         pulProfile
      )

{
   ULONG    ulProfileID = 0, ulBufferIndex = 0;
   BOOL     bHit;


   //
   // find a profile id that isn't used
   //
   while (ulProfileID < MAX_PROFILES) {

      ulBufferIndex = 0;
      bHit = FALSE;

      while (ulBufferIndex < pInfo->ulNumProfiles) {

         if (ulProfileID == pInfo->pHwProfValues[ulBufferIndex].ulProfile) {
            bHit = TRUE;
            break;
         }

         ulBufferIndex++;
      }

      //
      // if I got all the way through the list without a hit, then this
      // profile id is free
      //
      if (!bHit) {
         *pulProfile = ulProfileID;
         return TRUE;
      }

      ulProfileID++;
   }

   *pulProfile = 0xFFFFFFFF;
   return FALSE;

} // GetFreeProfileID


/**-------------------------------------------------------------------------**/
ULONG
GetOriginalProfile(
      PHWPROF_INFO  pInfo,
      ULONG         ulProfile,
      ULONG         ulBufferIndex
      )
{
    ULONG   ulIndex, ulIndexCreatedFrom;

    //
    // if the specified profile is a newly created profile, then it is
    // by definition the first in the copy chain
    //
    if (!(pInfo->pHwProfValues[ulBufferIndex].ulAction & HWP_CREATE)) {
        return ulProfile;
    }

    ulIndex = ulBufferIndex;

    while (pInfo->pHwProfValues[ulIndex].ulAction & HWP_CREATE) {
        //
        // find which entry in the buffer list matches the "CopiedFrom" profile
        //
        ulIndexCreatedFrom = 0;

        while (ulIndexCreatedFrom < pInfo->ulNumProfiles) {
           if (pInfo->pHwProfValues[ulIndexCreatedFrom].ulProfile ==
               pInfo->pHwProfValues[ulIndex].ulCreatedFrom) {
              break;
           }
           ulIndexCreatedFrom++;
        }
        ulIndex = ulIndexCreatedFrom;
    }

    return pInfo->pHwProfValues[ulIndex].ulProfile;

} // GetOriginalProfile


/**-------------------------------------------------------------------------**/
BOOL
InsertRank(
      PHWPROF_INFO   pInfo,
      ULONG          ulRank
      )

{
   ULONG ulIndex;

   //
   // for inserting a rank and readjusting the other ranks, just
   // scan through the list and for any rank that is greater than
   // or equal to the inserted rank, increment the rank value
   //
   for (ulIndex = 0; ulIndex < pInfo->ulNumProfiles; ulIndex++) {
      //
      // if it's marked for delete, don't bother with it
      //
      if (!(pInfo->pHwProfValues[ulIndex].ulAction & HWP_DELETE)) {

         if (pInfo->pHwProfValues[ulIndex].ulPreferenceOrder >= ulRank) {
            pInfo->pHwProfValues[ulIndex].ulPreferenceOrder++;
            pInfo->pHwProfValues[ulIndex].ulAction |= HWP_REORDER;
         }
      }

   }

   return TRUE;

} // InsertRank



/**-------------------------------------------------------------------------**/
BOOL
DeleteRank(
      PHWPROF_INFO   pInfo,
      ULONG          ulRank
      )

{
   ULONG ulIndex;

   //
   // for deleting a rank and readjusting the other ranks, just
   // scan through the list and for any rank that is greater than
   // the deleted rank, subtract one from the rank value
   //
   for (ulIndex = 0; ulIndex < pInfo->ulNumProfiles; ulIndex++) {
      //
      // if it's marked for delete, don't bother with it
      //
      if (!(pInfo->pHwProfValues[ulIndex].ulAction & HWP_DELETE)) {

         if (pInfo->pHwProfValues[ulIndex].ulPreferenceOrder > ulRank) {
            pInfo->pHwProfValues[ulIndex].ulPreferenceOrder--;
            pInfo->pHwProfValues[ulIndex].ulAction |= HWP_REORDER;
         }
      }

   }

   return TRUE;

} // DeleteRank



/**-------------------------------------------------------------------------**/
BOOL
FlushProfileChanges(
      HWND hDlg,
      HWND hList
      )
{
   ULONG    ulIndex=0;
   HKEY     hKey = NULL, hDestKey = NULL, hSrcKey = NULL, hHwProf = NULL;
   WCHAR    RegStr[MAX_PATH];
   PHWPROF_INFO   pInfo=NULL;


   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);


   //
   // First pass, process the changes for each profile (except delete)
   //
   while (ulIndex < pInfo->ulNumProfiles) {

      //
      // were any changes made to this profile?
      //
      if (pInfo->pHwProfValues[ulIndex].ulAction == HWP_NO_ACTION) {
         goto NextProfile;
      }

      //
      // save deleting for the second pass
      //
      if (pInfo->pHwProfValues[ulIndex].ulAction & HWP_DELETE) {
         goto NextProfile;
      }

      //
      // commit the changes for this profile
      //
      WriteProfileInfo(&pInfo->pHwProfValues[ulIndex]);

      NextProfile:
         ulIndex++;
   }



   //
   // Second pass, process the delete requests
   //
   ulIndex = 0;
   while (ulIndex < pInfo->ulNumProfiles) {

      if (pInfo->pHwProfValues[ulIndex].ulAction & HWP_DELETE) {
         //
         // we only need to delete the key if it exists (if this
         // isn't a delete of a profile that was also just created)
         //
         if (!(pInfo->pHwProfValues[ulIndex].ulAction & HWP_CREATE)) {

            wsprintf(RegStr, TEXT("%s\\%s"),
                     pszRegIDConfigDB,
                     pszRegKnownDockingStates);

            if (RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE, RegStr, 0, KEY_ALL_ACCESS,
                     &hKey) != ERROR_SUCCESS) {

               DisplaySystemMessage(hDlg, ERROR_REGISTRY_CORRUPT);
               return FALSE;
            }

            wsprintf(RegStr, TEXT("%04u"),
                     pInfo->pHwProfValues[ulIndex].ulProfile);

            RegDeleteKey(hKey, RegStr);
            RegCloseKey(hKey);

            //
            // also delete the profile specific enum tree
            //
            DeleteProfileDependentTree(pInfo->pHwProfValues[ulIndex].ulProfile);

            //
            // also delete the aliases to this profile
            //
            DeleteAliasEntries(pInfo->pHwProfValues[ulIndex].ulProfile);
            
         } else {
             //
             // decrement the name-generating counter for profiles we are
             // deleting that we created this session (only if registry keys
             // were never created for this new profile)
             //
             AdjustProfileTypeCounter(pInfo,
                                      pInfo->pHwProfValues[ulIndex].ulDockState, 
                                      FALSE);
         }
         
      }
      ulIndex++;
   }

   //
   // commit global settings
   //
   if(RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
            KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {

       RegSetValueEx(hKey, pszRegIsPortable, 0, REG_DWORD,
                (LPBYTE)&pInfo->bPortable, sizeof(DWORD));

       if (RegOpenKeyEx(
                 hKey, pszRegKnownDockingStates, 0,
                 KEY_SET_VALUE, &hHwProf) == ERROR_SUCCESS) {

           if (pInfo->ulUndockedProfileNameCount > 0) {
               RegSetValueEx(hHwProf, pszRegUndocked, 0, REG_DWORD,
                             (LPBYTE)&pInfo->ulUndockedProfileNameCount, sizeof(ULONG));
           } else {
               RegDeleteValue(hHwProf, pszRegUndocked);
           }
           
           if (pInfo->ulDockedProfileNameCount > 0) {
               RegSetValueEx(hHwProf, pszRegDocked, 0, REG_DWORD,
                             (LPBYTE)&pInfo->ulDockedProfileNameCount, sizeof(ULONG));
           } else {
               RegDeleteValue(hHwProf, pszRegDocked);
           }
           
           if (pInfo->ulUnknownProfileNameCount > 0) {
               RegSetValueEx(hHwProf, pszRegUnknown, 0, REG_DWORD,
                             (LPBYTE)&pInfo->ulUnknownProfileNameCount, sizeof(ULONG));
           } else {
               RegDeleteValue(hHwProf, pszRegUnknown);
           }
           
           RegCloseKey(hHwProf);
       }
       RegCloseKey(hKey);
   }

   return TRUE;

} // FlushProfileChanges



/**-------------------------------------------------------------------------**/
BOOL
WriteProfileInfo(
   PHWPROF_VALUES pProfValues
   )
{
   HKEY     hKey = NULL, hDestKey = NULL, hSrcKey = NULL;
   WCHAR    RegStr[MAX_PATH];
   UUID     NewGuid;
   LPTSTR   UuidString;
   TCHAR    szGuid[MAX_GUID_STRING_LEN];

   if (pProfValues->ulAction & HWP_DELETE) {
      return TRUE;      // skip it
   }

   //
   // form the registry key string
   //
   wsprintf(RegStr, TEXT("%s\\%s\\%04u"),
            pszRegIDConfigDB,
            pszRegKnownDockingStates,
            pProfValues->ulProfile);

   //
   // create the profile key if it's a new profile. Don't
   // worry about security because the profile subkey always
   // inherits the security of the parent Hardware Profiles key.
   //
   if (pProfValues->ulAction & HWP_CREATE) {

      if (RegCreateKeyEx(
               HKEY_LOCAL_MACHINE, RegStr, 0, NULL,
               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
               NULL, &hKey, NULL) != ERROR_SUCCESS) {
         return FALSE;
      }

      //
      // create a HwProfileGuid if its a new profile
      //
      if ((UuidCreate(&NewGuid) == RPC_S_OK) &&    
          (UuidToString(&NewGuid, &UuidString) == RPC_S_OK)) {                    

          //
          // put curly braces around the guid
          //
          lstrcpy(szGuid, TEXT("{"));
          lstrcat(szGuid, UuidString);
          lstrcat(szGuid, TEXT("}"));
          RpcStringFree(&UuidString);
          
          //
          // save the new HwProfileGuid in the registry
          //
          RegSetValueEx(
              hKey, pszRegHwProfileGuid, 0, REG_SZ,
              (LPBYTE)szGuid,
              (lstrlen(szGuid)+1) * sizeof(TCHAR) );         
      }
      
   } else {
      //
      // if not a create, just open the existing key
      //
      if (RegOpenKeyEx(
               HKEY_LOCAL_MACHINE, RegStr, 0, KEY_SET_VALUE,
               &hKey) != ERROR_SUCCESS) {
         return FALSE;
      }
   }

   //
   // update preference order if modified
   //
   if ((pProfValues->ulAction & HWP_REORDER) ||
            (pProfValues->ulAction & HWP_CREATE)) {

      RegSetValueEx(
            hKey, pszRegPreferenceOrder, 0, REG_DWORD,
            (LPBYTE)&pProfValues->ulPreferenceOrder, sizeof(ULONG));

      pProfValues->ulAction &= ~HWP_REORDER;    // clear action
   }

   //
   // update friendly name if modified
   //
   if ((pProfValues->ulAction & HWP_RENAME) ||
            (pProfValues->ulAction & HWP_CREATE)) {

      RegSetValueEx(
            hKey, pszRegFriendlyName, 0, REG_SZ,
            (LPBYTE)pProfValues->szFriendlyName,
            (lstrlen(pProfValues->szFriendlyName)+1) * sizeof(TCHAR));

      pProfValues->ulAction &= ~HWP_RENAME;     // clear action
   }

   //
   // update property values if modified
   //
   if ((pProfValues->ulAction & HWP_PROPERTIES) ||
            (pProfValues->ulAction & HWP_CREATE)) {

      RegSetValueEx(
            hKey, pszRegDockState, 0, REG_DWORD,
            (LPBYTE)&pProfValues->ulDockState, sizeof(ULONG));

      RegSetValueEx(
            hKey, pszRegAliasable, 0, REG_DWORD,
            (LPBYTE)&pProfValues->bAliasable, sizeof(DWORD));

      pProfValues->ulAction &= ~HWP_PROPERTIES;    // clear action
   }


   if (pProfValues->ulAction & HWP_CREATE) {
      //
      // copy the profile enum info. Don't worry about security on
      // this createkey because the profile key always inherits the
      // security of the parent Hardware Profiles key.
      //
      wsprintf(RegStr, TEXT("%s\\%04u"),
            pszRegHwProfiles,
            pProfValues->ulProfile);

      RegCreateKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hDestKey, NULL);

      wsprintf(RegStr, TEXT("%s\\%04u"),
            pszRegHwProfiles,
            pProfValues->ulCreatedFrom);

      RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0, KEY_ALL_ACCESS, &hSrcKey);

      CopyRegistryNode(hSrcKey, hDestKey);

      //
      // copy all aliases for the source profile to the destination as well.
      //
      if (CopyAliasEntries(pProfValues->ulCreatedFrom, 
                           pProfValues->ulProfile) == FALSE) {

          //
          // We didn't actually copy any aliases, so make sure this profile gets
          // marked as "Aliasable" so that it shows up as a profile option at
          // boot time.
          //
          pProfValues->bAliasable = TRUE;
          RegSetValueEx(
              hKey, pszRegAliasable, 0, REG_DWORD,
              (LPBYTE)&pProfValues->bAliasable, sizeof(DWORD));
      }

      if (hDestKey != NULL) RegCloseKey(hDestKey);
      if (hSrcKey != NULL) RegCloseKey(hSrcKey);

      pProfValues->ulAction &= ~HWP_CREATE;     // clear action
      pProfValues->ulAction |= HWP_NEWPROFILE;  // created during this session
   }

   RegCloseKey(hKey);

   return TRUE;

} // WriteProfileInfo


/**-------------------------------------------------------------------------**/
BOOL
RemoveNewProfiles(
      PHWPROF_INFO   pInfo
      )
{
   ULONG    ulIndex=0;
   HKEY     hKey = NULL;
   WCHAR    RegStr[MAX_PATH];


   //
   // check each profile for any HWP_NEWPROFILE flags
   //
   while (ulIndex < pInfo->ulNumProfiles) {

      if (pInfo->pHwProfValues[ulIndex].ulAction & HWP_NEWPROFILE) {

         wsprintf(RegStr, TEXT("%s\\%s"),
                  pszRegIDConfigDB,
                  pszRegKnownDockingStates);

         if (RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE, RegStr, 0, KEY_ALL_ACCESS,
                  &hKey) == ERROR_SUCCESS) {

            wsprintf(RegStr, TEXT("%04u"),
                     pInfo->pHwProfValues[ulIndex].ulProfile);

            RegDeleteKey(hKey, RegStr);
            RegCloseKey(hKey);
         }

         //
         // also delete the profile specific enum tree
         //
         DeleteProfileDependentTree(pInfo->pHwProfValues[ulIndex].ulProfile);

         //
         // also delete aliases to this profile
         //
         DeleteAliasEntries(pInfo->pHwProfValues[ulIndex].ulProfile);

         //
         // decrement the appropriate name-generating counter
         // (note that the counter is only deleted for new profiles that are
         // never fully commited; we don't make an attempt to decerement the
         // counter when any profile is deleted, else we may get into a case
         // where we assign a name to a new profile that in incrementally less
         // than a name we had previously assigned.)
         //
         AdjustProfileTypeCounter(pInfo,
                                  pInfo->pHwProfValues[ulIndex].ulDockState, 
                                  FALSE);
      }
      ulIndex++;
   }

   return TRUE;

} // RemoveNewProfiles



/**-------------------------------------------------------------------------**/
BOOL
SwapPreferenceOrder(
      HWND  hDlg,
      HWND  hList,
      ULONG_PTR ulIndex1,
      ULONG_PTR ulIndex2
      )

{
   ULONG    ulProfile1=0, ulProfile2=0;
   ULONG    ulBufferIndex1=0, ulBufferIndex2=0;
   WCHAR    szFriendlyName1[MAX_FRIENDLYNAME_LEN];
   WCHAR    szFriendlyName2[MAX_FRIENDLYNAME_LEN];
   ULONG    ulTemp=0;
   PHWPROF_INFO   pInfo=NULL;


   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   //
   // retrieve the profile id for the two selected profile entries
   //
   ulProfile1 = (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex1, 0);
   ulProfile2 = (ULONG)SendMessage(hList, LB_GETITEMDATA, ulIndex2, 0);

   //
   // find the profile entry in the buffer that matches these selections
   //
   while (ulBufferIndex1 < pInfo->ulNumProfiles) {
      if (pInfo->pHwProfValues[ulBufferIndex1].ulProfile == ulProfile1) {
         break;
      }
      ulBufferIndex1++;
   }

   while (ulBufferIndex2 < pInfo->ulNumProfiles) {
      if (pInfo->pHwProfValues[ulBufferIndex2].ulProfile == ulProfile2) {
         break;
      }
      ulBufferIndex2++;
   }

   //
   // swap the order values of the profiles in the in-memory buffer
   //
   ulTemp = pInfo->pHwProfValues[ulBufferIndex1].ulPreferenceOrder;
   pInfo->pHwProfValues[ulBufferIndex1].ulPreferenceOrder =
            pInfo->pHwProfValues[ulBufferIndex2].ulPreferenceOrder;
   pInfo->pHwProfValues[ulBufferIndex2].ulPreferenceOrder = ulTemp;

   //
   // mark both profiles as having been reordered
   //
   pInfo->pHwProfValues[ulBufferIndex1].ulAction |= HWP_REORDER;
   pInfo->pHwProfValues[ulBufferIndex2].ulAction |= HWP_REORDER;

   //
   // swap the positions in the list box
   //
   SendMessage(hList, LB_GETTEXT, ulIndex1, (LPARAM)(LPTSTR)szFriendlyName1);
   SendMessage(hList, LB_GETTEXT, ulIndex2, (LPARAM)(LPTSTR)szFriendlyName2);

   SendMessage(hList, LB_DELETESTRING, ulIndex1, 0);
   SendMessage(hList, LB_INSERTSTRING, ulIndex1,
         (LPARAM)(LPTSTR)szFriendlyName2);

   SendMessage(hList, LB_DELETESTRING, ulIndex2, 0);
   SendMessage(hList, LB_INSERTSTRING, ulIndex2,
         (LPARAM)(LPTSTR)szFriendlyName1);

   SendMessage(hList, LB_SETITEMDATA, ulIndex1, ulProfile2);
   SendMessage(hList, LB_SETITEMDATA, ulIndex2, ulProfile1);

   //
   // finally, select the second index (the second index is the rank
   // position we're moving to)
   //
   SendMessage(hList, LB_SETCURSEL, ulIndex2, 0);

   return TRUE;

} // SwapPreferenceOrder


/**-------------------------------------------------------------------------**/
BOOL
DeleteProfileDependentTree(
      ULONG ulProfile
      )
{
   TCHAR szProfile[5], szKey[MAX_PATH];
   LONG  RegStatus = ERROR_SUCCESS;
   HKEY  hHwProfKey, hCfgKey;
   ULONG ulIndex = 0, ulSize = 0;


   //
   // form the registry key string
   //
   if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, pszRegHwProfiles, 0, KEY_ALL_ACCESS,
            &hHwProfKey) != ERROR_SUCCESS) {

      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }

   wsprintf(szProfile, TEXT("%04u"),
            ulProfile);

   DeleteRegistryNode(hHwProfKey, szProfile);
   RegCloseKey(hHwProfKey);

   return TRUE;

} // DeleteProfileDependentTree

/**-------------------------------------------------------------------------**/

BOOL
CopyAliasEntries(
    ULONG ulSrcProfile,
    ULONG ulDestProfile
    )
{
   BOOL bNewAliasCreated = FALSE;

   bNewAliasCreated |= CopyAliasEntryType(ulSrcProfile, ulDestProfile, pszRegAlias);
   bNewAliasCreated |= CopyAliasEntryType(ulSrcProfile, ulDestProfile, pszRegAcpiAlias);    

   return bNewAliasCreated;

} // CopyAliasEntries

/**-------------------------------------------------------------------------**/

BOOL
DeleteAliasEntries(
    ULONG ulProfile    
    )
{
   BOOL bResult = TRUE;

   bResult |= DeleteAliasEntryType(ulProfile, pszRegAlias);
   bResult |= DeleteAliasEntryType(ulProfile, pszRegAcpiAlias);    

   return bResult;

} // CopyAliasEntries

/**-------------------------------------------------------------------------**/

BOOL
CopyAliasEntryType(
    ULONG  ulSrcProfile,
    ULONG  ulDestProfile,
    LPWSTR szSubKeyName
    )
{

   LONG   RegStatus = ERROR_SUCCESS;
   HKEY   hAliasRoot, hSrcKey, hDestKey;
   WCHAR  RegStr[MAX_PATH];
   WCHAR  szString[MAX_PATH];
   ULONG  ulAliasProfileNumber, ulNewAlias, ulSize, i;
   BOOL   bNewAliasCreated=FALSE;

   //
   // check all aliases under the "Alias" key
   //
   wsprintf(RegStr, TEXT("%s\\%s"),
            pszRegIDConfigDB,
            szSubKeyName);

   RegStatus = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0, KEY_ALL_ACCESS,
            &hAliasRoot);
   if (RegStatus == ERROR_FILE_NOT_FOUND) {
      //
      // No Alias subkey, this could be ok if we have the other type.
      //
      RegStatus = ERROR_SUCCESS;
      return bNewAliasCreated;
   } else if (RegStatus != ERROR_SUCCESS) {
      //
      // Some other registry error occurred.
      //
      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   } else {
       //
       // enumerate all Alias subkeys
       //
       for (i=0; RegStatus == ERROR_SUCCESS; i++) {

           ulSize = MAX_PATH;
           
           RegStatus = RegEnumKey(hAliasRoot, i, szString, ulSize);

           if (RegStatus == ERROR_SUCCESS) {

               if (RegOpenKey(hAliasRoot, szString, &hSrcKey) == ERROR_SUCCESS) {
             
                   ulSize = sizeof(ULONG);
                   
                   if (RegQueryValueEx(
                       hSrcKey, pszRegProfileNumber, NULL, NULL,
                       (LPBYTE)&ulAliasProfileNumber, 
                       &ulSize) == ERROR_SUCCESS) {
                 
                       //
                       // Check if we need to copy this one
                       //
                       if (ulSrcProfile == ulAliasProfileNumber) {

                           //
                           // Find an unused Alias subkey name
                           //          
                           ulNewAlias = 0;
                           while (ulNewAlias < MAX_ALIASES) {
                               ulNewAlias++;
                               wsprintf(RegStr, TEXT("%s\\%s\\%04u"),
                                        pszRegIDConfigDB,
                                        szSubKeyName,
                                        ulNewAlias);              
                               RegStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                                        RegStr, 0, KEY_ALL_ACCESS, 
                                                        &hDestKey);
                               
                               if (RegStatus == ERROR_SUCCESS) {
                                   RegCloseKey(hDestKey);
                                   continue;
                               } else if (RegStatus == ERROR_FILE_NOT_FOUND) {
                                   RegStatus = ERROR_SUCCESS;
                                   break;
                               } else {
                                   break;
                               }
                           }
                     
                           if ((RegStatus != ERROR_SUCCESS) || (ulNewAlias >= MAX_ALIASES)) {
                               RegCloseKey(hSrcKey);
                               break;
                           }

                           wsprintf(RegStr, TEXT("%s\\%s\\%04u"),
                                    pszRegIDConfigDB,
                                    szSubKeyName,
                                    ulNewAlias);
                           
                           RegCreateKeyEx(
                               HKEY_LOCAL_MACHINE, RegStr, 0, NULL, REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS, NULL, &hDestKey, NULL);
                        
                           //
                           // copy this alias entry 
                           //   
                           CopyRegistryNode(hSrcKey, hDestKey);
                           
                           RegSetValueEx(
                               hDestKey, pszRegProfileNumber, 0, REG_DWORD,
                               (LPBYTE)&ulDestProfile, sizeof(ULONG));
                           
                           bNewAliasCreated = TRUE;
                           if (ulNewAlias < (ULONG)_wtoi(szString)) {
                               // kick the enumeration index up one, else we'll
                               // find the key we just copied again.
                               i++;
                           }
                           
                           if (hDestKey != NULL) {
                               RegCloseKey(hDestKey);
                               hDestKey = NULL;
                           }                           
                       }
                   }
                   if (hSrcKey != NULL) {
                       RegCloseKey(hSrcKey);
                       hSrcKey = NULL;
                   }
               }
           }
       }
       RegCloseKey(hAliasRoot);
       hAliasRoot = NULL;
   }

   return bNewAliasCreated;

} // CopyAliasEntryType

/**-------------------------------------------------------------------------**/

BOOL
DeleteAliasEntryType(
      ULONG  ulProfile,
      LPWSTR szSubKeyName
      )

{

   LONG   RegStatus = ERROR_SUCCESS;
   HKEY   hAliasRoot;
   HKEY   hAliasSubKey;
   WCHAR  RegStr[MAX_PATH];
   WCHAR  szString[MAX_PATH];
   ULONG  ulAliasProfileNumber, ulSize, i;
   BOOL   bDeleted = FALSE;

   //
   // check all aliases under the "Alias" key
   //
   wsprintf(RegStr, TEXT("%s\\%s"),
            pszRegIDConfigDB,
            szSubKeyName);

   RegStatus = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, RegStr, 0, KEY_ALL_ACCESS,
            &hAliasRoot);
   if (RegStatus == ERROR_FILE_NOT_FOUND) {
       return TRUE;
   } else if (RegStatus != ERROR_SUCCESS) {
      DisplaySystemMessage(NULL, ERROR_REGISTRY_CORRUPT);
      return FALSE;
   }
   
   //
   // enumerate all subkeys
   //
   RegStatus = ERROR_SUCCESS;

   for (i=0; RegStatus == ERROR_SUCCESS; ) {

      ulSize = MAX_PATH;

      RegStatus = RegEnumKey(hAliasRoot, i, szString, ulSize);
      bDeleted = FALSE;

      if (RegStatus == ERROR_SUCCESS) {
         if (RegOpenKey(hAliasRoot, szString, &hAliasSubKey) == ERROR_SUCCESS) {
             
             ulSize = sizeof(ULONG);
             if (RegQueryValueEx(
                 hAliasSubKey, pszRegProfileNumber, NULL, NULL,
                 (LPBYTE)&ulAliasProfileNumber, 
                 &ulSize) == ERROR_SUCCESS) {
                 
                 if (ulProfile == ulAliasProfileNumber) {
                     //
                     // delete this alias entry 
                     //
                     RegCloseKey(hAliasSubKey);
                     hAliasSubKey = NULL;
                     RegDeleteKey(hAliasRoot, szString);
                     bDeleted = TRUE;
                 }
             }
             if (hAliasSubKey) RegCloseKey(hAliasSubKey);
         }
      }
      if (!bDeleted) i++;
   }   
   RegCloseKey(hAliasRoot);
      
   return TRUE;

} // DeleteAliasEntryType

/**-------------------------------------------------------------------------**/

BOOL
CopyRegistryNode(
   HKEY     hSrcKey,
   HKEY     hDestKey
   )
{

   LONG  RegStatus = ERROR_SUCCESS;
   HKEY  hSrcSubKey, hDestSubKey;
   WCHAR szString[MAX_PATH];
   ULONG ulDataSize, ulLength, ulType, i;
   BYTE  Data[MAX_PATH * 2];          // biggest value data??
   PSECURITY_DESCRIPTOR pSecDesc;


   //
   // copy all values for this key
   //
   for (i=0; RegStatus == ERROR_SUCCESS; i++) {

      ulLength = MAX_PATH;
      ulDataSize = 1024;

      RegStatus = RegEnumValue(hSrcKey, i, szString, &ulLength, NULL,
            &ulType, Data, &ulDataSize);

        if (RegStatus == ERROR_SUCCESS) {
           RegSetValueEx(hDestKey, szString, 0, ulType, Data, ulDataSize);
        }
    }

    //
    // recursively call CopyRegistryNode to copy all subkeys
    //
    RegStatus = ERROR_SUCCESS;

    for (i=0; RegStatus == ERROR_SUCCESS; i++) {

      ulLength = MAX_PATH;

      RegStatus = RegEnumKey(hSrcKey, i, szString, ulLength);

      if (RegStatus == ERROR_SUCCESS) {
         if (RegOpenKey(hSrcKey, szString, &hSrcSubKey) == ERROR_SUCCESS) {
            if (RegCreateKey(hDestKey, szString, &hDestSubKey) == ERROR_SUCCESS) {

               RegGetKeySecurity(hSrcSubKey, DACL_SECURITY_INFORMATION,
                     NULL, &ulDataSize);
               pSecDesc = LocalAlloc(LPTR, ulDataSize);
               RegGetKeySecurity(hSrcSubKey, DACL_SECURITY_INFORMATION,
                     pSecDesc, &ulDataSize);

               CopyRegistryNode(hSrcSubKey, hDestSubKey);

               RegSetKeySecurity(hDestSubKey, DACL_SECURITY_INFORMATION, pSecDesc);
               LocalFree(pSecDesc);
               RegCloseKey(hDestSubKey);
            }
            RegCloseKey(hSrcSubKey);
         }
      }
   }

   return TRUE;

} // CopyRegistryNode


/**-------------------------------------------------------------------------**/
BOOL
DeleteRegistryNode(
   HKEY     hParentKey,
   LPTSTR   szKey
   )
{
   ULONG ulSize = 0;
   LONG  RegStatus = ERROR_SUCCESS;
   HKEY  hKey = NULL;
   WCHAR szSubKey[MAX_PATH];


   //
   // attempt to delete the key
   //
   if (RegDeleteKey(hParentKey, szKey) != ERROR_SUCCESS) {

      if (RegOpenKeyEx(
               hParentKey, szKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WRITE,
               &hKey) != ERROR_SUCCESS) {
         return FALSE;
      }

      //
      // enumerate subkeys and delete those nodes
      //
      while (RegStatus == ERROR_SUCCESS) {
         //
         // enumerate the first level children under the profile key
         //
         ulSize = MAX_PATH;
         RegStatus = RegEnumKeyEx(
                  hKey, 0, szSubKey, &ulSize,
                  NULL, NULL, NULL, NULL);

         if (RegStatus == ERROR_SUCCESS) {

            if (!DeleteRegistryNode(hKey, szSubKey)) {
               RegCloseKey(hKey);
               return FALSE;
            }
         }
      }

      //
      // subkeys have been deleted, try deleting this key again
      //
      RegCloseKey(hKey);
      RegDeleteKey(hParentKey, szKey);
   }

   return TRUE;

}  // DeleteRegistryNode


/**-------------------------------------------------------------------------**/
VOID
AdjustProfileTypeCounter(
    PHWPROF_INFO   pInfo,
    ULONG          ulDockState,
    BOOL           bIncrement
    )
{
    PULONG pCounter;

    //
    // use the counter corresponding to the given DockState.
    //
    if ((ulDockState & DOCKINFO_DOCKED) &&
        (ulDockState & DOCKINFO_UNDOCKED)) {       
        pCounter = &pInfo->ulUnknownProfileNameCount;
    } else if (ulDockState & DOCKINFO_DOCKED) {
        pCounter = &pInfo->ulDockedProfileNameCount;
    } else if (ulDockState & DOCKINFO_UNDOCKED) {
        pCounter = &pInfo->ulUndockedProfileNameCount;
    } else {
        pCounter = &pInfo->ulUnknownProfileNameCount;
    }

    //
    // increment or decrement the counter, as requested.
    //
    if (bIncrement && (*pCounter >= 0)) {
        *pCounter += 1;
    } else if (!bIncrement && (*pCounter > 0)) {
        *pCounter -= 1;        
    } else {
        *pCounter = 0;
    }
}

/**-------------------------------------------------------------------------**/
BOOL
StripCurrentTag(
   LPTSTR   szFriendlyName,
   ULONG    ulProfile,
   ULONG    ulCurrentProfile
   )
{
   ULONG ulTagLen, ulNameLen;


   if (ulProfile == ulCurrentProfile) {

      ulTagLen = lstrlen(pszCurrentTag);
      ulNameLen = lstrlen(szFriendlyName);

      if (ulNameLen < ulTagLen) {
         return TRUE;   // nothing to do
      }

      if (lstrcmpi(&szFriendlyName[ulNameLen - ulTagLen], pszCurrentTag) == 0) {
         //
         // truncate the string before the current tag
         //
         szFriendlyName[ulNameLen - ulTagLen - 1] = '\0';
      }
   }

   return TRUE;

} // StripCurrentTag


/**-------------------------------------------------------------------------**/
BOOL
AppendCurrentTag(
   LPTSTR   szTaggedName,
   LPCTSTR  szOriginalName,
   ULONG    ulProfile,
   ULONG    ulCurrentProfile
   )
{

   lstrcpy(szTaggedName, szOriginalName);

   //
   // if the profile is the current profile, then append the tag
   // (let's user easily identify it as current)
   //
   if (ulProfile == ulCurrentProfile) {
      lstrcat(szTaggedName, TEXT(" "));
      lstrcat(szTaggedName, pszCurrentTag);
   }

   return TRUE;

} // AppendCurrentTag


/**-------------------------------------------------------------------------**/
VOID
CreateHwProfileFriendlyName(
   IN  HWND       hDlg,
   IN  ULONG      ulDockState,
   OUT LPTSTR     szFriendlyName
   )
{

   PHWPROF_INFO   pInfo;
   LPTSTR         szPrefix;
   PULONG         pulIndex;
   BOOL           bUnknown=FALSE, bUniqueFriendlyName=FALSE;

   //
   // retrieve the profile buffer
   //
   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   //
   // based on the DockState, determine the appropriate label to use and get a
   // pointer to its counter index
   //
   if ((ulDockState & DOCKINFO_DOCKED) &&
       (ulDockState & DOCKINFO_UNDOCKED)) {
       szPrefix = pszUnknown;
       pulIndex = &pInfo->ulUnknownProfileNameCount;
       bUnknown = TRUE;
   }
   else if (ulDockState & DOCKINFO_DOCKED) {
       szPrefix = pszDocked;
       pulIndex = &pInfo->ulDockedProfileNameCount;
   }
   else if (ulDockState & DOCKINFO_UNDOCKED) {
       szPrefix = pszUndocked;
       pulIndex = &pInfo->ulUndockedProfileNameCount;
   }
   else {
       szPrefix = pszUnknown;
       pulIndex = &pInfo->ulUnknownProfileNameCount;
       bUnknown = TRUE;
   }

   while ((!bUniqueFriendlyName) && (*pulIndex < MAX_PROFILES)) {
       //
       // as long as we don't have a unique name, build a FriendlyName based on
       // the DockState and counter index
       //
       if (bUnknown || (*pulIndex > 0)) {
           wsprintf(szFriendlyName, TEXT("%s %u"),
                    szPrefix,
                    *pulIndex + 1);
       } else {
           lstrcpy(szFriendlyName, szPrefix);
       }

       if (IsProfileNameInUse(hDlg,szFriendlyName)) {
           //
           // if this friendly name is in use, increment the counter to reflect this
           //
           *pulIndex += 1;
       } else {
           //
           // a unique friendly name has been generated
           //
           bUniqueFriendlyName = TRUE;
       }
   }

   if (!bUniqueFriendlyName) {
       //
       // if unable to generate a unique friendly name, just use some default.
       // the user may have to deal with any errors arising from duplicate names
       // is this name is already taken
       //
       lstrcpy(szFriendlyName, pszRegDefaultFriendlyName);
   }

   return;

} // CreateHwProfileFriendlyName


/**-------------------------------------------------------------------------**/
VOID
DisplayPrivateMessage(
      HWND  hWnd,
      UINT  uiPrivateError
      )
{
   WCHAR szMessage[MAX_PATH];


   LoadString(hInstance, uiPrivateError, szMessage, MAX_PATH);
   MessageBox(hWnd, szMessage, pszErrorCaption, MB_OK | MB_ICONSTOP);

   return;
}


/**-------------------------------------------------------------------------**/
VOID
DisplaySystemMessage(
      HWND  hWnd,
      UINT  uiSystemError
      )
{
   WCHAR szMessage[MAX_PATH];

   //
   // retrieve the string matching the Win32 system error
   //
   FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            uiSystemError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            szMessage,
            MAX_PATH,
            NULL);

   //
   // display a message box with this error
   //
   MessageBox(
            hWnd,
            szMessage,
            pszErrorCaption,
            MB_OK | MB_ICONSTOP);

   return;

} // DisplaySystemMessage


/**--------------------------------------------------------------------------**/
BOOL
ValidateAsciiString(
   IN LPTSTR  pszString
   )
{
   LPTSTR p = NULL;

   for (p = pszString; *p; p++) {
      if (*p > 128) {
         return FALSE;
      }
   }

   return TRUE;

} // ValidateAsciiString


/**--------------------------------------------------------------------------**/
BOOL
UpdateOrderButtonState(
   HWND  hDlg
   )
{
   PHWPROF_INFO   pInfo;
   ULONG_PTR      ulIndex = 0;
   HWND           hList;


   pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

   hList = GetDlgItem(hDlg, IDD_HWP_PROFILES);

   if ((ulIndex = SendMessage(hList,
        LB_GETCURSEL, 0, 0)) == LB_ERR) {
      return FALSE;
   }

   if (ulIndex == 0) {
      //
      // if focus currently on the button we're about to disable,
      // change focus first or focus will be lost.
      //
      if (GetFocus() == GetDlgItem(hDlg, IDD_HWP_ORDERUP)) {
          SendMessage(hDlg, DM_SETDEFID, IDD_HWP_ORDERDOWN, 0L);
          SetFocus(GetDlgItem(hDlg, IDD_HWP_ORDERDOWN));
      }
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_ORDERUP), FALSE);
   } else {
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_ORDERUP), TRUE);
   }

   if (ulIndex < pInfo->ulActiveProfiles-1) {
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_ORDERDOWN), TRUE);
   } else {
      //
      // if focus currently on the button we're about to disable,
      // change focus first or focus will be lost.
      //
      if (GetFocus() == GetDlgItem(hDlg, IDD_HWP_ORDERDOWN)) {
          SendMessage(hDlg, DM_SETDEFID, IDD_HWP_PROPERTIES, 0L);
          SetFocus(GetDlgItem(hDlg, IDD_HWP_PROPERTIES));
      }
      EnableWindow(GetDlgItem(hDlg, IDD_HWP_ORDERDOWN), FALSE);
   }




   return TRUE;
}


/**-------------------------------------------------------------------------**/
BOOL CALLBACK AddPropSheetPageProc(
    HPROPSHEETPAGE  hpage,
    LPARAM  lParam
   )
{
    hPages[ulNumPages] = hpage;

    return TRUE;

} // AddPropSheetPageProc


/**--------------------------------------------------------------------------**/
BOOL
DisplayProperties(
    IN HWND           hOwnerDlg,
    IN PHWPROF_INFO   pInfo
    )
{
    BOOL              bStatus;
    LPTSTR            pszProviderList = NULL, pszProvider = NULL;
    PROPSHEETPAGE     PropPage;
    PROPSHEETHEADER   PropHeader;
    FARPROC           lpProc;
    ULONG             i, ulSize;
    HKEY              hKey = NULL;


    //
    // create the first page (General)
    //
    ulNumPages = 0;

    PropPage.dwSize        = sizeof(PROPSHEETPAGE);
    PropPage.dwFlags       = PSP_DEFAULT;
    PropPage.hInstance     = hInstance;
    PropPage.pszTemplate   = MAKEINTRESOURCE(DLG_HWP_GENERAL);
    PropPage.pszIcon       = NULL;
    PropPage.pszTitle      = NULL;
    PropPage.pfnDlgProc    = GeneralProfileDlg;
    PropPage.lParam        = (LPARAM)pInfo;
    PropPage.pfnCallback   = NULL;

    hPages[0] = CreatePropertySheetPage(&PropPage);
    if (hPages[0] == NULL) {
        return FALSE;
    }

    ulNumPages++;

    //
    // open the IDConfigDB key
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegIDConfigDB, 0,
                     KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }


    //---------------------------------------------------------------
    // Are there any other property pages?
    //---------------------------------------------------------------

    if (RegQueryValueEx(hKey, pszRegPropertyProviders, NULL, NULL,
                        NULL, &ulSize) == ERROR_SUCCESS) {

        pszProviderList = LocalAlloc(LPTR, ulSize);

        if (pszProviderList != NULL) {
            //
            // read list of providers
            //
            if (RegQueryValueEx(hKey, pszRegPropertyProviders, NULL, NULL,
                    (LPBYTE)pszProviderList, &ulSize) == ERROR_SUCCESS) {
                //
                // Ask each provider to create and register it's property page
                //
                for (pszProvider = pszProviderList;
                     *pszProvider;
                     pszProvider += lstrlen(pszProvider) + 1) {

                    if (ulNumPages >= MAX_EXTENSION_PROVIDERS) {
                        break;      // stop at max number of pages
                    }

                    //
                    // load the provider DLL
                    //
                    hLibs[ulNumPages] = LoadLibrary(pszProvider);
                    if (hLibs[ulNumPages] != NULL) {

                        lpProc = GetProcAddress(hLibs[ulNumPages],
                                                "ExtensionPropSheetPageProc");
                        if (lpProc != NULL) {
                            //
                            // pass the profile ID to the provider as the lParam value
                            //
                            if ((lpProc)(NULL,
                                         &AddPropSheetPageProc,
                                         pInfo->ulSelectedProfile)) {
                                ulNumPages++;
                            }
                        }
                    }
                }
            }
            LocalFree(pszProviderList);
        }
    }

    RegCloseKey(hKey);


    //
    // create the property sheet
    //
    PropHeader.dwSize      = sizeof(PROPSHEETHEADER);
    PropHeader.dwFlags     = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    PropHeader.hwndParent  = hOwnerDlg;
    PropHeader.hInstance   = hInstance;
    PropHeader.pszIcon     = NULL;   //MAKEINTRESOURCE(DOCK_ICON);
    PropHeader.pszCaption  =
            pInfo->pHwProfValues[pInfo->ulSelectedProfileIndex].szFriendlyName;
    PropHeader.nPages      = ulNumPages;
    PropHeader.phpage      = hPages;
    PropHeader.nStartPage  = 0;
    PropHeader.pfnCallback = NULL;

    if (PropertySheet(&PropHeader) == 1) {
        bStatus = FALSE;
    } else {
        bStatus = TRUE;
    }

    //
    // cleanup extension page info
    //
    for (i = 1; i < ulNumPages; i++) {
        FreeLibrary(hLibs[i]);
    }

    return bStatus;

} // DisplayProperties



/**-------------------------------------------------------------------------**/
INT_PTR
APIENTRY
GeneralProfileDlg(
      HWND    hDlg,
      UINT    uMessage,
      WPARAM  wParam,
      LPARAM  lParam
      )

{
   PHWPROF_INFO      pInfo = NULL;
   PHWPROF_VALUES    pProfInfo = NULL;
   ULONG             ulReturn, ulIndex;

   switch (uMessage)
   {
      case WM_INITDIALOG:

         if (!lParam) {
            break;
         }

         //
         // on WM_INITDIALOG call, lParam points to the property sheet page.
         // The lParam field in the property sheet page struct is set by,
         // caller. When I created the property sheet, I passed in a pointer
         // to a HWPROF_INFO struct. Save this in the user window long so I
         // can access it on later messages.
         //
         pInfo = (PHWPROF_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
         SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pInfo);

         pProfInfo = (PHWPROF_VALUES)(&(pInfo->pHwProfValues[pInfo->ulSelectedProfileIndex]));

         SetDlgItemText(hDlg, IDD_HWP_ST_PROFILE, pProfInfo->szFriendlyName);

         //
         // for pre-beta hwprofile code, the dockstate might have originally
         // been set to zero which is invalid, use 0x111 instead
         //
         if (pProfInfo->ulDockState == 0) {
            pProfInfo->ulDockState =
                     DOCKINFO_USER_SUPPLIED | DOCKINFO_DOCKED | DOCKINFO_UNDOCKED;
         }

         //
         // initialize the dock state radio buttons
         //
         if ((pProfInfo->ulDockState & DOCKINFO_DOCKED) &&
             (pProfInfo->ulDockState & DOCKINFO_UNDOCKED)) {

            CheckRadioButton(hDlg, IDD_HWP_UNKNOWN, IDD_HWP_UNDOCKED, IDD_HWP_UNKNOWN);
         }
         else if (pProfInfo->ulDockState & DOCKINFO_DOCKED) {
            CheckRadioButton(hDlg, IDD_HWP_UNKNOWN, IDD_HWP_UNDOCKED, IDD_HWP_DOCKED);
         }
         else if (pProfInfo->ulDockState & DOCKINFO_UNDOCKED) {
            CheckRadioButton(hDlg, IDD_HWP_UNKNOWN, IDD_HWP_UNDOCKED, IDD_HWP_UNDOCKED);
         }
         else {
            CheckRadioButton(hDlg, IDD_HWP_UNKNOWN, IDD_HWP_UNDOCKED, IDD_HWP_UNKNOWN);
         }

         //
         // if the user-specified bit is not set then the dock state
         // was determined from the hardware so don't allow changing it
         //
         if (pProfInfo->ulDockState & DOCKINFO_USER_SUPPLIED) {
         }
         else {
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_PORTABLE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_DOCKED), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNDOCKED), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNKNOWN), FALSE);
         }

         //
         // initialize the dock id and serial # static control
         //
         if (pProfInfo->szSerialNumber[0] &&
             (pProfInfo->szSerialNumber[0] != TEXT('0'))) {
            SetDlgItemText(hDlg, IDD_HWP_SERIALNUM, pProfInfo->szSerialNumber);
         }
         else {
            SetDlgItemText(hDlg, IDD_HWP_SERIALNUM, pszUnavailable);
         }

         if (pProfInfo->szDockID[0] &&
             (pProfInfo->szDockID[0] != TEXT('0'))) {
            SetDlgItemText(hDlg, IDD_HWP_DOCKID, pProfInfo->szDockID);
            //
            // if dock id is available then docking state is known
            // and cannot be over-ridden (this is a redundant check,
            // the dock state should be accurate)
            //
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_PORTABLE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_DOCKED), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNDOCKED), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNKNOWN), FALSE);
         }
         else {
            SetDlgItemText(hDlg, IDD_HWP_DOCKID, pszUnavailable);
         }

         //
         // initialize the portable checkbox-groupbox
         //
         if (pInfo->bPortable) {
            CheckDlgButton(hDlg, IDD_HWP_PORTABLE, BST_CHECKED);
         }
         else {
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_DOCKED), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNDOCKED), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNKNOWN), FALSE);
         }

         //
         // Don't allow changing the global IsPortable Flag if it was determined
         // from the hardware
         //
         if (pInfo->bHwDetectedPortable) {
             EnableWindow(GetDlgItem(hDlg, IDD_HWP_PORTABLE), FALSE);
         }

         //
         // initialize the aliasable checkbox
         //
         if (pProfInfo->bAliasable) {
            CheckDlgButton(hDlg, IDD_HWP_ALIASABLE, BST_CHECKED);
         }
         else {
             CheckDlgButton(hDlg, IDD_HWP_ALIASABLE, BST_UNCHECKED);
         }

         SetFocus(GetDlgItem(hDlg, IDD_HWP_PORTABLE));
         return FALSE;


      case WM_NOTIFY:

         if (!lParam) {
            break;
         }

         switch (((NMHDR *)lParam)->code) {

             case PSN_APPLY:

               pInfo = (PHWPROF_INFO)GetWindowLongPtr(hDlg, DWLP_USER);
               pProfInfo = (PHWPROF_VALUES)(&(pInfo->pHwProfValues[pInfo->ulSelectedProfileIndex]));

               //
               // unchecked --> checked case
               //
               if (!pInfo->bPortable && IsDlgButtonChecked(hDlg, IDD_HWP_PORTABLE)) {
                  pInfo->bPortable = TRUE;
               }
               //
               // checked --> unchecked case
               //
               else if (pInfo->bPortable && !IsDlgButtonChecked(hDlg, IDD_HWP_PORTABLE)) {

                   TCHAR szCaption[MAX_PATH];
                   TCHAR szMsg[MAX_PATH];

                   LoadString(hInstance, HWP_ERROR_CAPTION, szCaption, MAX_PATH);
                   LoadString(hInstance, HWP_CONFIRM_NOT_PORTABLE, szMsg, MAX_PATH);

                   //
                   // confirm with user that other profiles will be set to unknown
                   //
                   if (MessageBox(hDlg, szMsg, szCaption,
                                  MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {

                       SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);   // don't apply
                       return TRUE;
                   }

                   for (ulIndex = 0; ulIndex < pInfo->ulNumProfiles; ulIndex++) {

                       if (pInfo->pHwProfValues[ulIndex].ulDockState & 
                           DOCKINFO_USER_SUPPLIED) {
                           //
                           // only user-specified dock states will be changed
                           //
                           pInfo->pHwProfValues[ulIndex].ulDockState =
                               DOCKINFO_USER_SUPPLIED | DOCKINFO_DOCKED | DOCKINFO_UNDOCKED;
                           pInfo->pHwProfValues[ulIndex].ulAction = HWP_PROPERTIES;                           
                       }

                   }
                   pInfo->bPortable = FALSE;
               }

               //
               // if user-specified dock state, then update the profile values
               // with current ui settings
               //
               if (pProfInfo->ulDockState & DOCKINFO_USER_SUPPLIED) {

                  if (IsDlgButtonChecked(hDlg, IDD_HWP_DOCKED)) {
                     pProfInfo->ulDockState |= DOCKINFO_DOCKED;
                     pProfInfo->ulDockState &= ~DOCKINFO_UNDOCKED;
                  }
                  else if (IsDlgButtonChecked(hDlg, IDD_HWP_UNDOCKED)) {
                     pProfInfo->ulDockState |= DOCKINFO_UNDOCKED;
                     pProfInfo->ulDockState &= ~DOCKINFO_DOCKED;
                  }
                  else {
                     pProfInfo->ulDockState |= (DOCKINFO_UNDOCKED | DOCKINFO_DOCKED);
                  }
               }

               //
               // aliasable unchecked --> checked case
               //
               if (!pProfInfo->bAliasable && IsDlgButtonChecked(hDlg, IDD_HWP_ALIASABLE)) {
                  pProfInfo->bAliasable = TRUE;
               } 
               //
               // aliasable checked --> unchecked case
               //
               else if (pProfInfo->bAliasable && !IsDlgButtonChecked(hDlg, IDD_HWP_ALIASABLE)) {
                  pProfInfo->bAliasable = FALSE;
               } 

               //
               // commit the changes for this profile
               //
               pProfInfo->ulAction |= HWP_PROPERTIES;
               WriteProfileInfo(pProfInfo);

               SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);   // TRUE if error
               break;

            case PSN_RESET:
               //
               // user canceled the property sheet
               //
               break;
         }
         break;


      case WM_HELP:
         WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, HELP_FILE,
               HELP_WM_HELP, (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;

      case WM_CONTEXTMENU:
         WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
                  (DWORD_PTR)(LPTSTR)HwProfileHelpIds);
         break;


      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDD_HWP_PORTABLE:
               //
               // if user chooses portable
               //
               if (!IsDlgButtonChecked(hDlg, IDD_HWP_PORTABLE)) {
                  CheckRadioButton(hDlg, IDD_HWP_UNKNOWN, IDD_HWP_UNDOCKED, IDD_HWP_UNKNOWN);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_DOCKED), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNDOCKED), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNKNOWN), FALSE);
               }
               else {
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_DOCKED), TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNDOCKED), TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDD_HWP_UNKNOWN), TRUE);
               }
               break;

            default:
               return FALSE;
          }
          break;

       } // case WM_COMMAND...

       default:
          return FALSE;
          break;
    }

    return TRUE;

} // GeneralProfileDlg
