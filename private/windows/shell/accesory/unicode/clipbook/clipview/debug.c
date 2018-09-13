//
// Purpose: This module contains functions that are being debugged or
//   are of interest during debugging. Generally, a function will be
//   placed in this file temporarily during debugging to make it easier
//   to find when running NTSD, 'cause the linker doesn't seem to include
//   symbols for functions that are only called from within their
//   own .c file.
//

// #include <nt.h>
// #include <ntrtl.h>
// #include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <sedapi.h>
#include "clipbook.h"
#include "clipdsp.h"
#include "common.h"
#include <commctrl.h>
#include <nddeapi.h>
#include <nddesec.h>
#include "helpids.h"
#include "dialogs.h"
#include "uniconv.h"

// Typedef for dynamically loading the Edit Owner dialog.
typedef DWORD (WINAPI *LPFNOWNER)( HWND, HANDLE, LPWSTR, LPWSTR,
      LPWSTR, UINT, PSED_FUNC_APPLY_SEC_CALLBACK, ULONG,
      PSECURITY_DESCRIPTOR, BOOLEAN, BOOLEAN, LPDWORD, PSED_HELP_INFO,
      DWORD);


extern BOOL GetTokenHandle(HANDLE *ph);
extern LRESULT Properties(HWND, PLISTENTRY);

extern DWORD dwCurrentHelpId;

// #define NOOLEITEMSPERMIT if netdde does not have
// support for allowing certain default OLE items in item-level
// conversations
#define NOOLEITEMSPERMIT
#ifdef NOOLEITEMSPERMIT
#define NOLEITEMS    5
static TCHAR *OleShareItems[NOLEITEMS] =
   {
   TEXT("StdDocumentName"),
   TEXT("EditEnvItems"),
   TEXT("StdHostNames"),
   TEXT("StdTargetDevice"),
   TEXT("StdDocDimensions")
   };
#endif // NOOLEITEMSPERMIT


#include "dumpsd.c"

TCHAR atchStatusBar[128];

#if 0
// Purpose: Create an SD with a DACL saying "Current user- All". And
//    nothing else. Note: This implementation is guaranteed to create
//    a self-relative SD.
//
// Returns: Pointer to the SD, or NULL on failure.
//
//////////////////////
PSECURITY_DESCRIPTOR CurrentUserOnlySD(
void)
{
SECURITY_DESCRIPTOR sd;
PSECURITY_DESCRIPTOR pSD = NULL;
HANDLE htok;
PSID   sid;
DWORD  dwSidSize = 100;
DWORD  dwSDSize;
PACL   pacl;
TOKEN_USER *pusr;
PSID psidWorld;
SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

PINFO(TEXT("CUOnlySD "));

AllocateAndInitializeSid(&WorldAuthority, 1, SECURITY_WORLD_RID,
      0, 0, 0, 0, 0, 0, 0, &psidWorld);

if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
   {
   if (GetTokenHandle(&htok))
      {
      pusr = (TOKEN_USER *)GlobalAllocPtr(GHND, dwSidSize);

      if (!GetTokenInformation(htok, TokenUser, pusr, dwSidSize, &dwSidSize))
         {
         GlobalFree(pusr);
         pusr = (TOKEN_USER *)GlobalAlloc(GPTR, dwSidSize);
         }

      if (pusr)
         {
         if (GetTokenInformation(htok, TokenUser, pusr, dwSidSize, &dwSidSize))
            {
            PrintSid(pusr->User.Sid);

            if (SetSecurityDescriptorOwner(&sd, pusr->User.Sid, TRUE) &&
                SetSecurityDescriptorGroup(&sd, pusr->User.Sid, TRUE))
               {
               if (pacl = (PACL)GlobalAllocPtr(GHND, sizeof(ACL) +
                     (2*sizeof(ACCESS_ALLOWED_ACE)) +
                     GetLengthSid(pusr->User.Sid) +
                     GetLengthSid(psidWorld)))
                  {
                  if (InitializeAcl(pacl, sizeof(ACL) +
                        (2*sizeof(ACCESS_ALLOWED_ACE)) +
                        GetLengthSid(pusr->User.Sid) +
                        GetLengthSid(psidWorld),
                        ACL_REVISION))
                     {
                     if (AddAccessAllowedAce(pacl, ACL_REVISION,
                            NDDE_GUI_FULL_CONTROL,
                            pusr->User.Sid))
                         // 6/25/93 - a-mgates Pages should have "userFC"
                         //   access only when first created
                         // &&
                         //
                         // AddAccessAllowedAce(pacl, ACL_REVISION,
                         //    NDDE_GUI_READ,
                         //    psidWorld))
                        {
                        if (SetSecurityDescriptorDacl(&sd, TRUE, pacl, TRUE))
                           {
                           dwSDSize = GetSecurityDescriptorLength(&sd);
                           if (pSD = GlobalAllocPtr(GHND, dwSDSize))
                              {
                              if (MakeSelfRelativeSD(&sd, pSD, &dwSDSize))
                                 {
                                 PINFO(TEXT("CUOnlySD OK\r\n"));
                                 PrintSD(pSD);
                                 }
                              else
                                 {
                                 PERROR(TEXT("MSelfRelSD fail %ld\r\n"),
                                       GetLastError());
                                 GlobalFreePtr(pSD);
                                 pSD = NULL;
                                 }
                              }
                           else
                              {
                              PERROR(TEXT("GAPtr for self-rel SD fail\r\n"));
                              }
                           }
                        else
                           {
                           PERROR(TEXT("SetSDDacl fail%ld\r\n"), GetLastError());
                           }
                        }
                     else
                        {
                        PERROR(TEXT("AddAccAllAce fail %ld\r\n"), GetLastError());
                        }
                     }
                  else
                     {
                     PERROR(TEXT("InitAcl fail%ld\r\n"), GetLastError());
                     }
                  GlobalFreePtr(pacl);
                  }
               else
                  {
                  PERROR(TEXT("GAPtr (PACL) fail%ld\r\n"), GetLastError());
                  }
               }
            else
               {
               PERROR(TEXT("Couldn't set owner/group %ld\r\n"),GetLastError());
               }
            }
         else
            {
            PERROR(TEXT("GetTokinfo fail%ld\r\n"), GetLastError());
            }
         GlobalFreePtr(pusr);
         }
      else
         {
         PERROR(TEXT("GAPtr fail%ld\r\n"), GetLastError());
         }
      }
   else
      {
      PERROR(TEXT("GTokHnd fail%ld\r\n"), GetLastError());
      }
   }
else
   {
   PERROR(TEXT("InitSD fail%ld\r\n"), GetLastError());
   }

FreeSid(psidWorld);

PERROR(TEXT("Returning %p\r\n"), pSD);
return(pSD);
}
#endif // 0

// Purpose: Call the Acl Editor for the selected page.
//
// Parameters:
//    fSacl - TRUE to call the SACL editor (auditing); FALSE to call
//       the DACL editor (permissions).
//
// Returns: 0L always-- this function handles its own errors.
//
//////////////////////////////////////////////////////////////////
LRESULT EditPermissions(
BOOL fSacl)
{
LPLISTENTRY lpLE;
TCHAR rgtchCName[MAX_COMPUTERNAME_LENGTH + 3];
TCHAR rgtchShareName[MAX_NDDESHARENAME + 1];
DWORD dwBAvail;
WORD wItems;
PNDDESHAREINFO lpddei;
unsigned iListIndex;
int ret;
TCHAR   szBuf[MAX_PAGENAME_LENGTH + 32];

iListIndex = (int)SendMessage(pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);

if ( iListIndex != LB_ERR )
   {
   // Set status bar text, and make the cursor an arrow.
   // LoadString(hInst, IDS_GETPERMS, atchStatusBar, CharSizeOf(atchStatusBar));
   // LockApp(TRUE, atchStatusBar);

   if ( SendMessage ( pActiveMDI->hWndListbox,
           LB_GETTEXT, iListIndex, (LPARAM)(LPCSTR)&lpLE) == LB_ERR)
      {
      PERROR(TEXT("PermsEdit No text: %d\n\r"), iListIndex );
      }
   else
      {
      // NDdeShareGetInfo wants a wItems containing 0. Fine.
      wItems = 0;

      // Get computer name containing share
      rgtchCName[0] = rgtchCName[1] = TEXT('\\');
      if (pActiveMDI->flags & F_LOCAL)
         {
         dwBAvail = MAX_COMPUTERNAME_LENGTH + 1;
         GetComputerName(rgtchCName + 2, &dwBAvail);
         }
      else
         {
         lstrcpy(rgtchCName + 2, pActiveMDI->szBaseName);
         }

      PINFO(TEXT("Getting page %s from server %s\r\n"),
           lpLE->name, rgtchCName);

      // Set up sharename string ("$<pagename>")
      lstrcpy(rgtchShareName, lpLE->name);
      rgtchShareName[0] = SHR_CHAR;

      // Edit the permissions
      PINFO(TEXT("Editing permissions for share %s\r\n"), rgtchShareName);
      PermissionsEdit(hwndApp, rgtchShareName, fSacl);

      ///////////////////////////////////////////////
      // do the execute to change the security on the file.
      lstrcat(lstrcpy(szBuf, IsShared(lpLE) ? SZCMD_SHARE : SZCMD_UNSHARE),
                  lpLE->name);
      PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

      MySyncXact ( (LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) +1),
          GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
          XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL);
      }
   // LockApp(FALSE, szNull);
   }
return 0L;
}

// Purpose: Callback function called by ACLEDIT.DLL. See SEDAPI.H for
//    details on its parameters and return value.
//
// Notes: The CallbackContext of this callback should be a string in
//    this format: Computername\0Sharename\0SECURITY_INFORMATION struct.
///////////////////////////////////////////////////////////////////////
DWORD CALLBACK SedCallback(
HWND                 hwndParent,
HANDLE               hInstance,
ULONG                penvstr,
PSECURITY_DESCRIPTOR SecDesc,
PSECURITY_DESCRIPTOR SecDescNewObjects,
BOOLEAN              ApplyToSubContainers,
BOOLEAN              ApplyToSubObjects,
LPDWORD              StatusReturn)
{
DWORD ret = NDDE_NO_ERROR + 37;
DWORD dwMyRet = ERROR_INVALID_PARAMETER;
TCHAR rgtch[MAX_COMPUTERNAME_LENGTH + 3];
DWORD dwLen;
PSECURITY_DESCRIPTOR psdSet;
SEDCALLBACKCONTEXT *pcbcontext;

pcbcontext = (SEDCALLBACKCONTEXT *)penvstr;

PINFO(TEXT("SedCallback: machine  %ls share %ls SI %ld\r\n"),
      pcbcontext->awchCName, pcbcontext->awchSName, pcbcontext->si);

// BUGBUG Need to give this capability to remote shares somehow!!!
if (IsValidSecurityDescriptor(SecDesc))
   {
   PINFO(TEXT("Setting security to "));
   PrintSD(SecDesc);

   SetLastError(0);
   dwLen = GetSecurityDescriptorLength(SecDesc);
   if (GetLastError())
      {
      PERROR(TEXT("Bad SecDesc!\r\n"));
      dwMyRet = ERROR_INVALID_SECURITY_DESCR;
      }
   else
      {
      // Try to make sure that the SD is self-relative, 'cause the
      // NetDDE functions vomit when given absolute SDs.
      if (psdSet = LocalAlloc(LPTR, dwLen))
         {
         if (FALSE == MakeSelfRelativeSD(SecDesc, psdSet, &dwLen))
            {
            LocalFree(psdSet);

            if (psdSet = LocalAlloc(LPTR, dwLen))
               {
               if (FALSE == MakeSelfRelativeSD(SecDesc, psdSet, &dwLen))
                  {
                  LocalFree(psdSet);
                  psdSet = NULL;
                  dwMyRet = ERROR_INVALID_SECURITY_DESCR;
                  }
               }
            else
               {
               dwMyRet = ERROR_NOT_ENOUGH_MEMORY;
               }
            }

         if (psdSet)
            {
            DWORD dwTrust[3];

            NDdeGetTrustedShareW(pcbcontext->awchCName, pcbcontext->awchSName,
                  dwTrust, dwTrust + 1, dwTrust + 2);
            ret = NDdeSetShareSecurityW(pcbcontext->awchCName,
                  pcbcontext->awchSName, pcbcontext->si, psdSet);
            PINFO(TEXT("Set share info. %d\r\n"),ret);

            if (ret != NDDE_NO_ERROR)
               {
               MessageBoxID(hInst, hwndParent, IDS_INTERNALERR, IDS_APPNAME,
                     MB_OK | MB_ICONSTOP);

               *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;
               dwMyRet =  ERROR_ACCESS_DENIED;
               }
            else
               {
               NDdeSetTrustedShareW(pcbcontext->awchCName,
                     pcbcontext->awchSName, 0);
               NDdeSetTrustedShareW(pcbcontext->awchCName,
                     pcbcontext->awchSName, dwTrust[0]);
               *StatusReturn = SED_STATUS_MODIFIED;
               dwMyRet =  ERROR_SUCCESS;
               }
            LocalFree(psdSet);
            }
         }
      }
   }
else
   {
   PERROR(TEXT("Bad security descriptor created, can't set security."));
   *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;
   dwMyRet = ERROR_INVALID_SECURITY_DESCR;
   }
return(dwMyRet);
}

//
// Purpose: Edit ownership on the selected page.
LRESULT EditOwner(
void)
{
LPLISTENTRY        lpLE;
DWORD              dwBAvail;
unsigned           iListIndex;
DWORD              Status;
DWORD              ret;
WCHAR              ShareObjectName[100];
SEDCALLBACKCONTEXT cbcontext;
BOOL               fCouldntRead;
BOOL               fCouldntWrite;
SED_HELP_INFO      HelpInfo;
DWORD              dwSize;
HMODULE            hMod;
PSECURITY_DESCRIPTOR pSD = NULL;;


iListIndex = (int)SendMessage(pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);
if ( iListIndex != LB_ERR )
   {
   if ( SendMessage ( pActiveMDI->hWndListbox, LB_GETTEXT,
            iListIndex, (LPARAM)(LPCSTR)&lpLE) != LB_ERR)
      {
      // Set up the callback context
      if (pActiveMDI->flags & F_LOCAL)
         {
         cbcontext.awchCName[0] = cbcontext.awchCName[1] = L'\\';
         dwBAvail = MAX_COMPUTERNAME_LENGTH + 1;
         GetComputerNameW(cbcontext.awchCName + 2, &dwBAvail);
         }
      else
         {
      #ifdef UNICODE
         lstrcpyW(cbcontext.awchCName, pActiveMDI->szBaseName);
      #else
         MultiByteToWideChar(CP_ACP, 0, pActiveMDI->szBaseName, -1,
            cbcontext.awchCName, MAX_COMPUTERNAME_LENGTH + 1);
      #endif
         }

      // Get page name
      SendMessage(pActiveMDI->hWndListbox, LB_GETTEXT, iListIndex, (LPARAM)&lpLE);

      PINFO(TEXT("Getting page %s from server %ws\r\n"),
           lpLE->name, cbcontext.awchCName);
   #ifdef UNICODE
      lstrcpyW( cbcontext.awchSName, lpLE->name);
   #else
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpLE->name,
            -1, cbcontext.awchSName, 100);
   #endif

   #ifndef USETWOSHARESPERPAGE
      cbcontext.awchSName[0] = L'$';
   #endif
      cbcontext.si = OWNER_SECURITY_INFORMATION;

      // Get object name
      LoadStringW(hInst, IDS_CB_PAGE, ShareObjectName, 99);

      // Get owner
      dwSize = 0L;
      PINFO(TEXT("Getting secinfo for %ls ! %ls\r\n"),
            cbcontext.awchCName,
            cbcontext.awchSName);
      NDdeGetShareSecurityW(
            cbcontext.awchCName,
            cbcontext.awchSName,
            OWNER_SECURITY_INFORMATION,
            pSD,
            0L,
            &dwSize);
      if (pSD = LocalAlloc(LPTR, min(dwSize, 65535L)))
         {
         PINFO(TEXT("Getting owner on %ls ! %ls..\r\n"),
               cbcontext.awchCName, cbcontext.awchSName);

         ret = NDdeGetShareSecurityW(
               cbcontext.awchCName,
               cbcontext.awchSName,
               OWNER_SECURITY_INFORMATION,
               pSD,
               dwSize,
               &dwSize);

         if (NDDE_NO_ERROR == ret)
            {
            DWORD adwTrust[3];

            fCouldntRead = FALSE;

            NDdeGetTrustedShareW(
                  cbcontext.awchCName,
                  cbcontext.awchSName,
                  adwTrust, adwTrust + 1, adwTrust + 2);

            ret = NDdeSetShareSecurityW(
                  cbcontext.awchCName,
                  cbcontext.awchSName,
                  OWNER_SECURITY_INFORMATION,
                  pSD);

            if (NDDE_NO_ERROR == ret)
               {
               NDdeSetTrustedShareW(
                  cbcontext.awchCName,
                  cbcontext.awchSName, adwTrust[0]);

               fCouldntWrite = FALSE;
               }
            }
         else
            {
            PERROR(TEXT("Couldn't get owner (err %d)!\r\n"), ret);
            fCouldntRead = TRUE;
            // We just set fCouldntWrite to FALSE if we couldn't read,
            // 'cause the only way to find out if we could would be
            // to overwrite the current ownership info (and we DON'T
            // KNOW WHAT IT IS!!)
            fCouldntWrite = FALSE;
            }

         HelpInfo.pszHelpFileName = TEXT("CLIPBRD.HLP");
         HelpInfo.aulHelpContext[ HC_MAIN_DLG ] = IDH_OWNER;

         if (hMod = LoadLibrary(TEXT("ACLEDIT.DLL")))
            {
            LPFNOWNER lpfn;

            if (lpfn = (LPFNOWNER)GetProcAddress(hMod, "SedTakeOwnership"))
               {
               ret = (*lpfn)(
                  hwndApp,
                  hInst,
                  cbcontext.awchCName,
                  ShareObjectName,
                  cbcontext.awchSName + 1,
                  1,
                  SedCallback,
                  (ULONG)&cbcontext,
                  fCouldntRead ? NULL : pSD,
                  fCouldntRead,
                  fCouldntWrite,
                  &Status,
                  &HelpInfo,
                  0L);
               }
            else
               {
               PERROR(TEXT("Couldn't get proc!\r\n"));
               }
            FreeLibrary(hMod);
            }
         else
            {
            PERROR(TEXT("Couldn't loadlib!\r\n"));
            }

         PINFO(TEXT("Ownership edited. Ret code %d, status %d\r\n"),
               ret, Status);

         LocalFree((HLOCAL)pSD);
         }
      else
         {
         PERROR(TEXT("Couldn't get current owner (%ld bytes)!\r\n"),
               dwSize);
         }
      }
   else
      {
      PERROR(TEXT("PermsEdit No text: %d\n\r"), iListIndex );
      }
   }
else
   {
   PERROR(TEXT("Attempt to modify ownership with no item sel'ed\r\n"));
   }
return 0L;
}


//
// Purpose: Create a Clipbook page.
//
///////////////////////////////////////////////////////////////////
LRESULT OnIDMKeep(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
int    tmp;
DWORD  ret;
LPTSTR lpItem;
HANDLE hData;
PNDDESHAREINFO lpDdeI;
// PSECURITY_DESCRIPTOR pSD;
TCHAR  atchItem[256];
unsigned i;

if (CountClipboardFormats())
   {
   if (hwndLocal && IsWindow(hwndLocal))
      {
      tmp = (int)SendMessage (  pActiveMDI->hWndListbox,
          LB_GETCOUNT, 0, 0L );

      if ( tmp < MAX_ALLOWED_PAGES )
         {
         szKeepAs[0] = TEXT('\0');
         dwCurrentHelpId = IDH_DLG_PASTEDATA;
         ret = DialogBoxParam(hInst,
                 (LPTSTR) MAKEINTRESOURCE(IDD_KEEPASDLG),
                 hwnd,
                 (DLGPROC)KeepAsDlgProc ,
                 0L);
         PINFO(TEXT("DialogBox returning %d\n\r"), ret );
         dwCurrentHelpId = 0L;

         // refresh main window
         UpdateWindow ( hwndApp );

         if ( ret && lstrlen(szKeepAs) )
            {
            // Set up NetDDE share for the page
            lpDdeI = GlobalAllocPtr(GHND, ByteCountOf(2048));
            if (lpDdeI)
               {
               LPTSTR lpLen; // Pointer to the end of the current data block
               TCHAR rgtchCName[MAX_COMPUTERNAME_LENGTH + 3];
               DWORD dwLen = MAX_COMPUTERNAME_LENGTH;

               // Set up computer name with \\ in front
               rgtchCName[1] = rgtchCName[0] = TEXT('\\');
               GetComputerName(rgtchCName + 2, &dwLen);

               lpLen = (LPBYTE)lpDdeI + sizeof(NDDESHAREINFO);

               // Set up the constant members of the struct
               lpDdeI->lRevision           = 1L;
               lpDdeI->fSharedFlag         = 0;
               lpDdeI->fService            = 0;
               lpDdeI->fStartAppFlag       = 0;
               lpDdeI->qModifyId[0]        = 0;
               lpDdeI->qModifyId[1]        = 0;
               lpDdeI->nCmdShow            = SW_SHOWMINNOACTIVE;
               lpDdeI->lShareType          = SHARE_TYPE_STATIC;

               // Enter the share name... must be == $<PAGENAME>.
               // (WFW used the dollar sign, we use the dollar sign. I
               //  hate backwards compatibility.)
               lpDdeI->lpszShareName = lpLen;

            #ifdef USETWOSHARESPERPAGE
               if (fSharePreference)
                  {
                  *lpLen = SHR_CHAR;
                  }
               else
                  {
                  *lpLen = UNSHR_CHAR;
                  }
            #else
               *lpLen = SHR_CHAR;
            #endif

               lstrcpy(lpDdeI->lpszShareName + 1, szKeepAs + 1);
               lpLen += lstrlen(lpDdeI->lpszShareName) + 1;


               // Start work on the app|topic list
               lpDdeI->lpszAppTopicList = lpLen;

               // By default, there are no items.
               atchItem[0] = TEXT('\0');

               // Set up old-style and OLE name if cf_objectlink is
               // available, else set '\0'.
               if (OpenClipboard(hwnd))
                  {
                  unsigned cb;
                  LPTSTR lpData;

                  if ((hData = VGetClipboardData(NULL, cf_link)) &&
                      (lpData = GlobalLock(hData)))
                     {
                     PINFO(TEXT("Link found\r\n"));
                     lstrcpy(lpLen, lpData);
                     lpLen += cb = lstrlen(lpLen);
                     *lpLen++ = TEXT('|');
                     lstrcpy(lpLen, lpData + cb + 1);
                     cb += lstrlen(lpLen) + 2;
                     lpLen += lstrlen(lpLen) + 1;
                     lstrcpy(atchItem, lpData + cb);
                     GlobalUnlock(lpData);
                     lpDdeI->lShareType |= SHARE_TYPE_OLD;
                     }
                  else
                     {
                     *lpLen++ = TEXT('\0');
                     }

                  if ((hData = VGetClipboardData(NULL, cf_objectlink)) &&
                      (lpData = GlobalLock(hData)))
                     {
                     PINFO(TEXT("ObjectLink found\r\n"));
                     lstrcpy(lpLen, lpData);
                     lpLen += cb = lstrlen(lpLen);
                     *lpLen++ = TEXT('|');
                     lstrcpy(lpLen, lpData + cb + 1);
                     cb += lstrlen(lpLen) + 2;
                     lpLen += lstrlen(lpLen) + 1;
                     lstrcpy(atchItem, lpData + cb);
                     GlobalUnlock(lpData);
                     lpDdeI->lShareType |= SHARE_TYPE_NEW;
                     }
                  else
                     {
                     *lpLen++ = TEXT('\0');
                     }
                  CloseClipboard();
                  }
               else // We couldn't open, we can't get objectlink.
                  {
                  *lpLen++ = TEXT('\0');
                  *lpLen++ = TEXT('\0');
                  }

               // Set up "CLIPSRV|*<pagename>" for a static app/topic
               // We use the *<pagename> form because when the page
               // is first created, it's ALWAYS unshared, and the server's
               // expecting us to be on the "unshared" topic name.
               lstrcpy(lpLen, SZ_SRV_NAME);
               lstrcat(lpLen, TEXT(BAR_CHAR));
               lpLen += lstrlen(lpLen);
               *lpLen = UNSHR_CHAR;


               lstrcpy(lpLen + 1, szKeepAs + 1);
               lpLen += lstrlen(lpLen) + 1;
               // NetDDE requires a fourth NULL at the end of the app/topic
               // list - dumb, but easier to do this than fix that.
               *lpLen++ = TEXT('\0');

               lpDdeI->lpszItemList = lpLen;
               // If there's an item listed, we need to set the item.
               // Otherwise, set no items-- this is an OLE link to the entire
               // document. ANY item, but there's nothing but the static
               // share anyway.
               if (lstrlen(atchItem))
                  {
                  lstrcpy(lpLen, atchItem);
                  lpLen += lstrlen(lpLen) + 1;
                  lpDdeI->cNumItems = 1;
           #ifdef NOOLEITEMSPERMIT
                  for (i = 0; i < NOLEITEMS; i++)
                     {
                     lstrcpy(lpLen, OleShareItems[i]);
                     lpLen += lstrlen(lpLen) + 1;
                     }
                  lpDdeI->cNumItems = NOLEITEMS + 1;
           #endif
                  }
               else
                  {
                  lpDdeI->cNumItems = 0;
                  *lpLen++ = TEXT('\0');
                  }
               // Finish off item list with an extra null.
               *lpLen++ = TEXT('\0');

               // Get an SD -- if this fails, we'll get a NULL pSD back,
               // and we'll end up creating the share with a default SD.
               // This is a tolerable condition.
               // pSD = CurrentUserOnlySD();

               // Create the share
               DumpDdeInfo(lpDdeI, rgtchCName);
               ret = NDdeShareAdd(rgtchCName, 2, NULL, (LPBYTE)lpDdeI,
                     sizeof(NDDESHAREINFO) );

               // We have to set security in a separate step, because
               // we set up a "default" DACL, and if we pass it in to
               // NDdeShareAdd(), it'll get overwritten by the "inherited"
               // DACL
               // The security people are scum. Use default
               // ([CreatorAll WorldRL]) share security.
               // if (pSD && NDDE_NO_ERROR == ret)
               //    {
               //    PrintSD(pSD);
               //
               //    ret = NDdeSetShareSecurity(rgtchCName,
               //                lpDdeI->lpszShareName,
               //                DACL_SECURITY_INFORMATION, pSD);
               //    }
               //
               PINFO(TEXT("NDdeShareAdd ret %ld\r\n"), ret);
               // if (pSD)
               //    {
               //    GlobalFree(pSD);
               //    }

               if (NDDE_NO_ERROR == ret)
                  {
                  // Need to trust the share so that we can init through it!
                  NDdeSetTrustedShare(rgtchCName, lpDdeI->lpszShareName,
                        NDDE_TRUST_SHARE_INIT);

                  // Send DDEExecute to tell clipsrv that we've created this page,
                  // and will it please make an actual file for it?
                  // NOTE must force all formats rendered to prevent deadlock
                  // on the clipboard.
                  ForceRenderAll(hwnd, NULL);
                  AssertConnection(hwndLocal);
                  lstrcat(lstrcpy(szBuf, SZCMD_PASTE),szKeepAs);
                  if ( MySyncXact( (LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) +1),
                      GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
                      XTYP_EXECUTE, 300L * 1000L , NULL ))
                     {
         #ifdef NOAUTOUPDATE
                     UpdateListBox( hwndLocal, GETMDIINFO(hwndLocal)->hExeConv );
         #endif
                     if ( fSharePreference )
                        {
                        UpdateListBox( hwndLocal, GETMDIINFO(hwndLocal)->hExeConv );
                        tmp = (int)SendMessage ( GETMDIINFO(hwndLocal)->hWndListbox,
                            LB_FINDSTRING, (WPARAM)-1, (LPARAM)szKeepAs );
                        if ( tmp != LB_ERR )
                           {
                           PLISTENTRY lpLE;

                           SendMessage(GETMDIINFO(hwndLocal)->hWndListbox,
                              LB_GETTEXT, tmp, (LPARAM)&lpLE);
                           Properties(hwnd, lpLE);
                           }
                        }
                     }
                  else
                     {
                     MessageBoxID(hInst, hwnd, IDS_NOCONNECTION, IDS_APPNAME,
                         MB_OK | MB_ICONSTOP );
                     NDdeShareDel(rgtchCName, lpDdeI->lpszShareName, 0);
                     }
                  }
               else if (NDDE_ACCESS_DENIED == ret)
                  {
                  MessageBoxID(hInst, hwnd, IDS_PRIVILEGEERROR, IDS_APPNAME,
                        MB_OK | MB_ICONSTOP);
                  }
               else
                  {
                  PERROR(TEXT("NDDE Error %d\r\n"), ret);
                  MessageBoxID(hInst, hwnd, IDS_INTERNALERR, IDS_APPNAME,
                        MB_OK | MB_ICONSTOP);
                  }

               GlobalFreePtr(lpDdeI);
               }
            else
               {
               MessageBoxID(hInst, hwnd, IDS_INTERNALERR, IDS_APPNAME,
                     MB_OK | MB_ICONSTOP);
               }
            }
         // Else user hit cancel on dialog
         }
      else
         {
         MessageBoxID ( hInst, hwnd, IDS_MAXPAGESERROR, IDS_PASTEDLGTITLE,
             MB_OK | MB_ICONEXCLAMATION );
         }
      }
   else
      {
      MessageBoxID(hInst, hwnd, IDS_NOCLPBOOK, IDS_APPNAME,
            MB_OK | MB_ICONSTOP);
      }
   }
else
   {
   PERROR(TEXT("Paste entered with no data on the clipboard!\r\n"));
   };

return 0L;
}
