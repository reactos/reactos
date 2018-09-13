
/*****************************************************************************

                                S H A R E S

    Name:       shares.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This file contains functions for manipulating NetDDE shares.

    History:
        21-Jan-1994 John Fu     Reformat and cleanup.
        19-Apr-1994 John Fu     Fix shares dialog help topic.
        03-Nov-1997 DrewM       Revised dialogs to use context sensitive help
*****************************************************************************/


#define    NOAUTOUPDATE 1

#include <windows.h>
#include <windowsx.h>
#include <nddeapi.h>
#include <nddesec.h>
#include <sedapi.h>

#include "common.h"
#include "clipbook.h"
#include "clipbrd.h"
#include "auditchk.h"
#include "clipdsp.h"
#include "dialogs.h"
#include "helpids.h"
#include "shares.h"
#include "clpbkdlg.h"
#include "cvutil.h"
#include "debugout.h"
#include "security.h"
#include "initmenu.h"





#define MAX_PERMNAMELEN     64



// Typedefs used to dynamically load and call the permission editors.

typedef DWORD (WINAPI *LPFNSACLEDIT)(HWND,
                                     HANDLE,
                                     LPWSTR,
                                     PSED_OBJECT_TYPE_DESCRIPTOR,
                                     PSED_APPLICATION_ACCESSES,
                                     LPWSTR,
                                     PSED_FUNC_APPLY_SEC_CALLBACK,
                                     ULONG_PTR,
                                     PSECURITY_DESCRIPTOR,
                                     BOOLEAN,
                                     LPDWORD,
                                     DWORD);

typedef DWORD (WINAPI *LPFNDACLEDIT)(HWND,
                                     HANDLE,
                                     LPWSTR,
                                     PSED_OBJECT_TYPE_DESCRIPTOR,
                                     PSED_APPLICATION_ACCESSES,
                                     LPWSTR,
                                     PSED_FUNC_APPLY_SEC_CALLBACK,
                                     ULONG_PTR,
                                     PSECURITY_DESCRIPTOR,
                                     BOOLEAN,
                                     BOOLEAN,
                                     LPDWORD,
                                     DWORD);


// Typedef for dynamically loading the Edit Owner dialog.
typedef DWORD (WINAPI *LPFNOWNER)(HWND,
                                  HANDLE,
                                  LPWSTR,
                                  LPWSTR,
                                  LPWSTR,
                                  UINT,
                                  PSED_FUNC_APPLY_SEC_CALLBACK,
                                  ULONG_PTR,
                                  PSECURITY_DESCRIPTOR,
                                  BOOLEAN,
                                  BOOLEAN,
                                  LPDWORD,
                                  PSED_HELP_INFO,
                                  DWORD);









static TCHAR    szDirName[256] = {'\0',};
static WCHAR    ShareObjectName[80];


static SED_APPLICATION_ACCESS KeyPerms[] =
   {
   SED_DESC_TYPE_RESOURCE,          0,                          0, NULL,
   SED_DESC_TYPE_RESOURCE,          NDDE_GUI_READ,              0, NULL,
   SED_DESC_TYPE_RESOURCE,          NDDE_GUI_READ_LINK,         0, NULL,
   SED_DESC_TYPE_RESOURCE,          NDDE_GUI_CHANGE,            0, NULL,
   SED_DESC_TYPE_RESOURCE,          GENERIC_ALL,                0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_READ,            0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_WRITE,           0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_INITIATE_STATIC, 0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_INITIATE_LINK,   0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_REQUEST,         0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_ADVISE,          0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_POKE,            0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_EXECUTE,         0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_ADD_ITEMS,       0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  NDDE_SHARE_LIST_ITEMS,      0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  DELETE,                     0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  READ_CONTROL,               0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  WRITE_DAC,                  0, NULL,
   SED_DESC_TYPE_RESOURCE_SPECIAL,  WRITE_OWNER,                0, NULL,
   };

static SED_APPLICATION_ACCESS KeyAudits[] =
   {
   SED_DESC_TYPE_AUDIT, NDDE_GUI_READ,   0, NULL,
   SED_DESC_TYPE_AUDIT, NDDE_GUI_CHANGE, 0, NULL,
   SED_DESC_TYPE_AUDIT, WRITE_DAC,       0, NULL,
   SED_DESC_TYPE_AUDIT, WRITE_OWNER,     0, NULL
   };







// Callback function gets called by the permission editor

DWORD CALLBACK SedCallback(HWND,
                  HANDLE,
                  ULONG_PTR,
                  PSECURITY_DESCRIPTOR,
                  PSECURITY_DESCRIPTOR,
                  BOOLEAN,
                  BOOLEAN,
                  LPDWORD);










#if DEBUG


/*
 *      DumpDdeInfo
 */

void DumpDdeInfo(
    PNDDESHAREINFO  pDdeI,
    LPTSTR          lpszServer)
{
LPTSTR      lpszT;
unsigned    i;



    PINFO(TEXT("Dde block:\r\n\r\n"));
    PINFO(TEXT("Server: <%s> Share: <%s>\r\n"),
          lpszServer ? lpszServer : "NULL",
          pDdeI->lpszShareName);

    lpszT = pDdeI->lpszAppTopicList;

    for (i = 0;i < 3;i++)
        {
        PINFO(TEXT("App|Topic %d: <%s>\r\n"),i, lpszT);
        lpszT += lstrlen(lpszT) + 1;
        }

    PINFO(TEXT("Rev: %ld Shared: %ld Service: %ld Start: %ld\r\n"),
          pDdeI->lRevision,
          pDdeI->fSharedFlag,
          pDdeI->fService,
          pDdeI->fStartAppFlag);

    PINFO(TEXT("Type: %ld Show: %ld Mod1: %lx Mod2: %lx\r\n"),
          pDdeI->lShareType,
          pDdeI->nCmdShow,
          pDdeI->qModifyId[0],
          pDdeI->qModifyId[1]);

    PINFO(TEXT("Items: %ld ItemList:"),
          pDdeI->cNumItems);


    lpszT = pDdeI->lpszItemList;

    if (lpszT)
        {
        for (i = 0;i < (unsigned)pDdeI->cNumItems;i++)
            {
            if ((i - 1)% 4 == 0)
                {
                PINFO(TEXT("\r\n"));
                }

            PINFO(TEXT("%s\t"),lpszT);
            lpszT += lstrlen(lpszT) + 1;
            }
        PINFO(TEXT("\r\n"));
        }
    else
        {
        PINFO(TEXT("NULL\r\n"));
        }

}


#endif // DEBUG











/*
 *      SedCallback
 *
 *  Purpose: Callback function called by ACLEDIT.DLL. See SEDAPI.H for
 *     details on its parameters and return value.
 *
 *  Notes: The CallbackContext of this callback should be a string in
 *     this format: Computername\0Sharename\0SECURITY_INFORMATION struct.
 */

DWORD CALLBACK SedCallback(
    HWND                 hwndParent,
    HANDLE               hInstance,
    ULONG_PTR            penvstr,
    PSECURITY_DESCRIPTOR SecDesc,
    PSECURITY_DESCRIPTOR SecDescNewObjects,
    BOOLEAN              ApplyToSubContainers,
    BOOLEAN              ApplyToSubObjects,
    LPDWORD              StatusReturn)
{
PSECURITY_DESCRIPTOR    psdSet;
SEDCALLBACKCONTEXT      *pcbcontext;
DWORD                   ret = NDDE_NO_ERROR + 37;
DWORD                   dwMyRet = ERROR_INVALID_PARAMETER;
DWORD                   dwLen;
DWORD                   dwErr;


    pcbcontext = (SEDCALLBACKCONTEXT *)penvstr;

    PINFO(TEXT("SedCallback: machine  %ls share %ls SI %ld\r\n"),
          pcbcontext->awchCName, pcbcontext->awchSName, pcbcontext->si);


    // BUGBUG Need to give this capability to remote shares somehow!!!
    if (!IsValidSecurityDescriptor(SecDesc))
        {
        PERROR(TEXT("Bad security descriptor created, can't set security."));
        *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;
        dwMyRet = ERROR_INVALID_SECURITY_DESCR;
        }
    else
        {
        PINFO(TEXT("Setting security to "));
        PrintSD(SecDesc);

        SetLastError(0);
        dwLen = GetSecurityDescriptorLength (SecDesc);

        if (dwErr = GetLastError())
            {
            PERROR(TEXT("GetSecurityDescriptorLength -> %u\r\n"), dwErr);
            dwMyRet = ERROR_INVALID_SECURITY_DESCR;
            }
        else
            {
            // Try to make sure that the SD is self-relative, 'cause the
            // NetDDE functions vomit when given absolute SDs.

            if (psdSet = LocalAlloc (LPTR, dwLen))
                {
                if (FALSE == MakeSelfRelativeSD (SecDesc, psdSet, &dwLen))
                    {
                    LocalFree(psdSet);

                    if (psdSet = LocalAlloc (LPTR, dwLen))
                        {
                        if (FALSE == MakeSelfRelativeSD (SecDesc, psdSet, &dwLen))
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

                    NDdeGetTrustedShareW (pcbcontext->awchCName,
                                          pcbcontext->awchSName,
                                          dwTrust,
                                          dwTrust + 1,
                                          dwTrust + 2);

                    ret = NDdeSetShareSecurityW (pcbcontext->awchCName,
                                                 pcbcontext->awchSName,
                                                 pcbcontext->si,
                                                 psdSet);

                    PINFO(TEXT("Set share info. %d\r\n"),ret);

                    if (ret != NDDE_NO_ERROR)
                        {
                        //MessageBoxID (hInst,
                        //              hwndParent,
                        //              IDS_INTERNALERR,
                        //              IDS_APPNAME,
                        //              MB_OK | MB_ICONSTOP);
                        NDdeMessageBox (hInst,
                                        hwndParent,
                                        ret,
                                        IDS_APPNAME,
                                        MB_OK|MB_ICONSTOP);

                        *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;
                        dwMyRet =  ERROR_ACCESS_DENIED;
                        }
                    else
                        {
                        NDdeSetTrustedShareW (pcbcontext->awchCName,
                                              pcbcontext->awchSName,
                                              0);

                        NDdeSetTrustedShareW (pcbcontext->awchCName,
                                              pcbcontext->awchSName,
                                              dwTrust[0]);

                        *StatusReturn = SED_STATUS_MODIFIED;
                        dwMyRet =  ERROR_SUCCESS;
                        }
                    LocalFree(psdSet);
                    }
                }
            }
        }


    return(dwMyRet);


}








/*
 *      EditPermissions
 *
 *  Purpose: Call the Acl Editor for the selected page.
 *
 *  Parameters:
 *     fSacl - TRUE to call the SACL editor (auditing); FALSE to call
 *        the DACL editor (permissions).
 *
 *  Returns: current selected item in list box or LB_ERR.
 */

LRESULT EditPermissions (
    BOOL    fSacl)
{
LPLISTENTRY     lpLE;
TCHAR           rgtchCName[MAX_COMPUTERNAME_LENGTH + 3];
TCHAR           rgtchShareName[MAX_NDDESHARENAME + 1];
DWORD           dwBAvail;
WORD            wItems;
unsigned        iListIndex;
TCHAR           szBuf[MAX_PAGENAME_LENGTH + 32];




    iListIndex = (int)SendMessage(pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);

    if (iListIndex != LB_ERR)
       {
       // Set status bar text, and make the cursor an arrow.
       // LoadString(hInst, IDS_GETPERMS, atchStatusBar, sizeof(atchStatusBar));
       // LockApp(TRUE, atchStatusBar);

       if (SendMessage (pActiveMDI->hWndListbox,
                        LB_GETTEXT, iListIndex, (LPARAM)(LPCSTR)&lpLE)
           == LB_ERR)
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
             GetComputerName (rgtchCName + 2, &dwBAvail);
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
          EditPermissions2 (hwndApp, rgtchShareName, fSacl);



          ///////////////////////////////////////////////
          // do the execute to change the security on the file.
          lstrcat(lstrcpy(szBuf, IsShared(lpLE) ? SZCMD_SHARE : SZCMD_UNSHARE),
                      lpLE->name);
          PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

          MySyncXact ( (LPBYTE)szBuf,
              lstrlen(szBuf) +1, GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
              XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL);
          }
       // LockApp(FALSE, szNull);
       }


    return iListIndex;

}












/*
 *      EditPermissions2
 *
 *  Purpose: Put up the standard "permission editor" dialog.
 *
 *  Parameters:
 *     hWnd - Parent window for the dialog.
 *     pShareName - Name of the DDE share.
 *     lpDdeI - Pointer to an NDDESHAREINFO describing the share.
 *     fSacl - TRUE if you're editing the SACL, FALSE to edit the DACL
 *
 *  Returns:
 *     TRUE on success, FALSE on failure.
 */

BOOL WINAPI EditPermissions2 (
    HWND    hWnd,
    LPTSTR  pShareName,
    BOOL    fSacl)
{
SED_OBJECT_TYPE_DESCRIPTOR  ObjectTypeDescriptor;
SED_APPLICATION_ACCESSES    ApplicationAccesses;

PSECURITY_DESCRIPTOR        pSD = NULL;
GENERIC_MAPPING             GmDdeShare;
SED_HELP_INFO               HelpInfo;
SEDCALLBACKCONTEXT          cbcontext;

DWORD       Status;
DWORD       dwRtn;
unsigned    i, iFirst;
BOOL        fRet = FALSE;
DWORD       dwSize;
BOOL        fCouldntRead;
HMODULE     hMod;
LPWSTR      szPermNames = NULL;

WCHAR	szSpecial[256];


    PINFO(TEXT("EditPermissions2: %s"), fSacl ? "SACL\r\n" : "DACL\r\n");

    if (fSacl && !AuditPrivilege (AUDIT_PRIVILEGE_ON))
        return fRet;




    SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Set up the callback context for the SedCallback function.
    cbcontext.awchCName[0] = cbcontext.awchCName[1] = L'\\';
    if (pActiveMDI->flags & (F_LOCAL | F_CLPBRD))
        {
        dwSize = MAX_COMPUTERNAME_LENGTH + 1;
        GetComputerNameW(cbcontext.awchCName + 2, &dwSize);
        }
    else
        {
        #ifdef REMOTE_ADMIN_OK
            #ifdef UNICODE
                lstrcpy(awchEnvStr + 2, pActiveMDI->szBaseName);
            #else
                MultiByteToWideChar (CP_ACP,
                                     0,
                                     pActiveMDI->szBaseName, -1,
                                     cbcontext.awchCName + 2,
                                     MAX_COMPUTERNAME_LENGTH + 1);
            #endif
        #else
            PERROR(TEXT("EditPermissions2() on remote window!!!\r\n"));
            MessageBoxID (hInst,
                          hwndApp,
                          IDS_INTERNALERR,
                          IDS_APPNAME,
                          MB_OK | MB_ICONHAND);
        #endif
        }


    #ifdef UNICODE
        lstrcpyW(cbcontext.awchSName, pShareName);
    #else
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pShareName, -1,
                            cbcontext.awchSName, MAX_NDDESHARENAME);
    #endif

    cbcontext.si = (fSacl? SACL_SECURITY_INFORMATION: DACL_SECURITY_INFORMATION);





    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, 30);
    if (!pSD)
        {
        PERROR(TEXT("LocalAlloc fail\r\n"));
        }
    else
        {
        // Get the security descriptor off of the share
        dwRtn = NDdeGetShareSecurityW (cbcontext.awchCName,
                                       cbcontext.awchSName,
                                       cbcontext.si |
                                       OWNER_SECURITY_INFORMATION,
                                       pSD,
                                       30,
                                       &dwSize);
        switch (dwRtn)
            {
            case NDDE_NO_ERROR:
                fCouldntRead = FALSE;
                PrintSD(pSD);
                break;

            case NDDE_BUF_TOO_SMALL:
                {
                PINFO(TEXT("GetShareSec sez SD is %ld bytes long, ret %ld\r\n"),
                      dwSize, dwRtn);

                LocalFree(pSD);
                pSD = NULL;

                if (dwSize < 65535 && (pSD = LocalAlloc(LPTR, dwSize)))
                    {
                    dwRtn = NDdeGetShareSecurityW (cbcontext.awchCName,
                                                   cbcontext.awchSName,
                                                   cbcontext.si |
                                                   OWNER_SECURITY_INFORMATION,
                                                   pSD,
                                                   dwSize,
                                                   &dwSize);

                    if (NDDE_NO_ERROR == dwRtn)
                        {
                        fCouldntRead = FALSE;
                        PINFO(TEXT("Got security!\r\n"));
                        PrintSD(pSD);
                        }
                    else
                        {
                        PERROR(TEXT("NDdeGetSecurity fail %ld!\r\n"), dwRtn);
                        fCouldntRead = TRUE;
                        LocalFree(pSD);
                        pSD = NULL;
                        break;
                        }
                    }
                else
                    {
                    PERROR(TEXT("LocalReAlloc fail (%ld bytes)\r\n"), dwSize);
                    }
                }
                break;

            case NDDE_ACCESS_DENIED:
            default:
                fCouldntRead = TRUE;
                LocalFree(pSD);
                pSD = NULL;
                break;
            }
        }



    if (!pSD && !fCouldntRead)
        {
        MessageBoxID(hInst, hWnd, IDS_INTERNALERR, IDS_APPNAME, MB_OK | MB_ICONHAND);
        goto done;
        }




    LoadStringW(hInst, IDS_SHROBJNAME, ShareObjectName,
          sizeof(ShareObjectName));

    // Set up help contexts for all of the dialogs, so the Help
    // buttons will work.
    HelpInfo.pszHelpFileName = L"clipbrd.hlp";
    HelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = 0;
    HelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0;
    HelpInfo.aulHelpContext[HC_ADD_USER_DLG]                = IDH_ADD_USER_DLG;
    HelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG]     = IDH_ADD_MEM_LG_DLG;
    HelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG]     = IDH_ADD_MEM_GG_DLG;
    HelpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG]         = IDH_FIND_ACCT_DLG;
    HelpInfo.aulHelpContext[HC_MAIN_DLG]                    = fSacl ?
                                                               IDH_AUDITDLG :
                                                               IDH_PERMSDLG;

    // Set up a GENERIC_MAPPING struct-- we don't use generic
    // rights, but the struct has to be there.
    GmDdeShare.GenericRead    = NDDE_GUI_READ;
    GmDdeShare.GenericWrite   = NDDE_GUI_CHANGE;
    GmDdeShare.GenericExecute = NDDE_GUI_READ_LINK;
    GmDdeShare.GenericAll     = NDDE_GUI_FULL_CONTROL;

    ObjectTypeDescriptor.Revision                        = SED_REVISION1;
    ObjectTypeDescriptor.IsContainer                     = FALSE;
    ObjectTypeDescriptor.AllowNewObjectPerms             = FALSE;
    ObjectTypeDescriptor.MapSpecificPermsToGeneric       = FALSE;
    ObjectTypeDescriptor.GenericMapping                  = &GmDdeShare;
    ObjectTypeDescriptor.GenericMappingNewObjects        = &GmDdeShare;
    ObjectTypeDescriptor.ObjectTypeName                  = ShareObjectName;
    ObjectTypeDescriptor.HelpInfo                        = &HelpInfo;
    ObjectTypeDescriptor.ApplyToSubContainerTitle        = NULL;
    ObjectTypeDescriptor.ApplyToSubContainerConfirmation = NULL;

    LoadStringW (hInst, IDS_SPECIAL, szSpecial, 256 );
    ObjectTypeDescriptor.SpecialObjectAccessTitle = szSpecial;

    ObjectTypeDescriptor.SpecialNewObjectAccessTitle     = NULL;

    if (fSacl)
        {
        PINFO(TEXT("Editing SACL..\r\n"));
        ApplicationAccesses.Count           = sizeof(KeyAudits)/sizeof(KeyAudits[0]);
        ApplicationAccesses.AccessGroup     = KeyAudits;
        }
    else
        {
        ApplicationAccesses.Count           = sizeof(KeyPerms)/sizeof(KeyPerms[0]);
        ApplicationAccesses.AccessGroup     = KeyPerms;
        // This corresponds to "Read and Link"
        ApplicationAccesses.DefaultPermName = KeyPerms[2].PermissionTitle;
        }


    // Load the permission names-- note ternary operator to give us
    // the AUDIT names if we're editing the SACL
    iFirst = fSacl ? IDS_AUDITNAMEFIRST : IDS_PERMNAMEFIRST;


    szPermNames = GlobalAlloc (LPTR,
                               ApplicationAccesses.Count
                               * MAX_PERMNAMELEN
                               * sizeof(WCHAR));

    if (!szPermNames)
        goto done;


    for (i=0; i<ApplicationAccesses.Count; i++)
        {
        ApplicationAccesses.AccessGroup[i].PermissionTitle
            = szPermNames + i * MAX_PERMNAMELEN;
        LoadStringW (hInst,
                     iFirst + i,
                     ApplicationAccesses.AccessGroup[i].PermissionTitle,
                     MAX_PERMNAMELEN - 1);
        }



    if (fSacl)
        {
        LPFNSACLEDIT lpfn;

        PINFO(TEXT("Finding SACL editor..\r\n"));

        if (hMod = LoadLibrary("ACLEDIT.DLL"))
            {
            if (lpfn = (LPFNSACLEDIT)GetProcAddress(hMod, "SedSystemAclEditor"))
                {
                SetCursor(LoadCursor(NULL, IDC_ARROW));

                PINFO(TEXT("Calling SACL editor..\r\n"));

                dwRtn = (*lpfn) (hWnd,                    // owner wnd
                                 hInst,                   // hinstance
                                 NULL,                    // Server (NULL means local)
                                 &ObjectTypeDescriptor,   // Object type
                                 &ApplicationAccesses,    // Access types.
                                 cbcontext.awchSName + 1, // Object name
                                 SedCallback,             // Apply security callback
                                 (ULONG_PTR)&cbcontext,   // Callback context
                                 pSD,                     // Points to current ACL
                                 (BOOLEAN)fCouldntRead,   // true if user can't read ACL list.
                                 &Status,                 // Status return code
                                 (DWORD)0);
                }
            else
                {
                MessageBoxID(hInst, hWnd, IDS_INTERNALERR, IDS_APPNAME, MB_OK | MB_ICONHAND);
                }
            FreeLibrary(hMod);
            }
        else
            {
            MessageBoxID(hInst, hWnd, IDS_INTERNALERR, IDS_APPNAME, MB_OK | MB_ICONHAND);
            }
        }
    else
        {
        LPFNDACLEDIT lpfn;

        PINFO(TEXT("Getting DACL edit \r\n"));

        if (hMod = LoadLibrary("ACLEDIT.DLL"))
            {
            if (lpfn = (LPFNDACLEDIT)GetProcAddress(hMod,
                  "SedDiscretionaryAclEditor"))
                {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                dwRtn = (*lpfn) (hWnd,                    // owner wnd
                                 hInst,                   // hinstance
                                 NULL,                    // Server (NULL means local)
                                 &ObjectTypeDescriptor,   // Object type
                                 &ApplicationAccesses,    // Access types.
                                 cbcontext.awchSName + 1, // Object name
                                 SedCallback,             // Apply security callback
                                 (ULONG_PTR)&cbcontext,   // Callback context
                                 pSD,                     // Points to current ACL
                                 (BOOLEAN)fCouldntRead,   // true if user can't read ACL list.
                                 FALSE,                   // true if user can't write ACL list
                                 &Status,                 // Status return code
                                 0L);
                }
            FreeLibrary(hMod);
            }
        }



    PINFO(TEXT("ACL Editor returned status %ld, ret value %ld\r\n"), Status, dwRtn);

    fRet = TRUE;


    SendMessage (hWnd, WM_COMMAND, IDM_REFRESH, 0);






done:


    if (pSD)         LocalFree((HLOCAL)pSD);
    if (szPermNames) GlobalFree (szPermNames);


    SetCursor(LoadCursor(NULL, IDC_ARROW));

    AuditPrivilege(AUDIT_PRIVILEGE_OFF);


    return fRet;

}










/*
 *      EditOwner
 *
 *  Purpose: Edit ownership on the selected page.
 */

LRESULT EditOwner(void)
{
LPLISTENTRY             lpLE;
DWORD                   dwBAvail;
unsigned                iListIndex;
DWORD                   Status;
DWORD                   ret;
WCHAR                   ShareObjectName[100];
BOOL                    fCouldntRead;
BOOL                    fCouldntWrite;
DWORD                   dwSize;
HMODULE                 hMod;
SED_HELP_INFO           HelPINFO;
SEDCALLBACKCONTEXT      cbcontext;
PSECURITY_DESCRIPTOR    pSD = NULL;;




    iListIndex = (int)SendMessage(pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);

    if (iListIndex == LB_ERR)
        {
        PERROR(TEXT("Attempt to modify ownership with no item sel'ed\r\n"));
        goto done;
        }




    if (SendMessage ( pActiveMDI->hWndListbox, LB_GETTEXT, iListIndex, (LPARAM)(LPCSTR)&lpLE)
        == LB_ERR)
        {
        PERROR(TEXT("PermsEdit No text: %d\n\r"), iListIndex );
        goto done;
        }



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
            lstrcpy (cbcontext.awchCName, pActiveMDI->szBaseName);
        #else
            MultiByteToWideChar (CP_ACP, 0, pActiveMDI->szBaseName, -1,
                                 cbcontext.awchCName, MAX_COMPUTERNAME_LENGTH + 1);
        #endif
        }



    // Get page name
    SendMessage(pActiveMDI->hWndListbox, LB_GETTEXT, iListIndex, (LPARAM)&lpLE);

    PINFO(TEXT("Getting page %s from server %ws\r\n"),
         lpLE->name, cbcontext.awchCName);

    #ifdef UNICODE
        lstrcpyW (cbcontext.awchSName, lpLE->name);
    #else
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, lpLE->name,
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

    NDdeGetShareSecurityW (cbcontext.awchCName,
                           cbcontext.awchSName,
                           OWNER_SECURITY_INFORMATION,
                           pSD,
                           0L,
                           &dwSize);


    if (!(pSD = LocalAlloc(LPTR, min(dwSize, 65535L))))
        {
        PERROR(TEXT("Couldn't get current owner (%ld bytes)!\r\n"), dwSize);
        }




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
            NDdeSetTrustedShareW (cbcontext.awchCName,
                                  cbcontext.awchSName,
                                  adwTrust[0]);

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

    HelPINFO.pszHelpFileName = L"CLIPBRD.HLP";
    HelPINFO.aulHelpContext[ HC_MAIN_DLG ] = IDH_OWNER;

    if (hMod = LoadLibrary("ACLEDIT.DLL"))
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
               (ULONG_PTR)&cbcontext,
               fCouldntRead ? NULL : pSD,
               (BOOLEAN)fCouldntRead,
               (BOOLEAN)fCouldntWrite,
               &Status,
               &HelPINFO,
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


    PINFO(TEXT("Ownership edited. Ret code %d, status %d\r\n"), ret, Status);

    LocalFree((HLOCAL)pSD);



done:

    return 0L;

}











/*
 *      Properties
 *
 *  Purpose: Change the properties of a share by displaying the Properties
 *     dialog and applying the changes the user makes to the share.
 *
 *  Parameters:
 *     hwnd - Parent window for the properties dialog
 *     lpLE - The entry we're messing with.
 *
 *  Returns:
 *     0L always. We don't return an error code because we handle informing
 *     the user of errors inside the routine.
 */

LRESULT Properties(
    HWND        hwnd,
    PLISTENTRY  lpLE)
{
PNDDESHAREINFO  lpDdeI;
LRESULT         ret;
WORD            wAddlItems;
DWORD           dwRet;
TCHAR           szBuf[MAX_PAGENAME_LENGTH + 32];
BOOL            fAlreadyShared;
DWORD           adwTrust[3];





    PINFO(TEXT("Props "));

    lpDdeI = GlobalAllocPtr(GHND, 2048 * sizeof(TCHAR));

    if (!lpDdeI)
        {
        PERROR(TEXT("GlobalAllocPtr failed\n\r"));
        return 0L;
        }




    // Use "shared" version of name, 'cause that's the way the DDE
    // share is named.
    fAlreadyShared = IsShared(lpLE);
    SetShared (lpLE, TRUE);

    PINFO(TEXT("for share [%s]"), lpLE->name);
    wAddlItems = 0;
    ret = NDdeShareGetInfo (NULL,
                            lpLE->name,
                            2,
                            (LPBYTE)lpDdeI,
                            2048 * sizeof(TCHAR),
                            &dwRet,
                            &wAddlItems );




    if (!fAlreadyShared)
        {
        SetShared(lpLE, FALSE);
        }




    PINFO(TEXT(" GetInfo ret %ld\r\n"), ret);

    if (NDDE_ACCESS_DENIED == ret)
        {
        MessageBoxID(hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME, MB_OK | MB_ICONHAND);
        }
    else if (ret != NDDE_NO_ERROR)
        {
        PERROR(TEXT("Error from NDdeShareGetInfo %d\n\r"), ret );
        //MessageBoxID ( hInst, hwndApp, IDS_SHARINGERROR, IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK);
        NDdeMessageBox ( hInst,
                         hwndApp,
                         (UINT)ret,
                         IDS_SHAREDLGTITLE,
                         MB_ICONHAND | MB_OK);
        }
    else if (ret == NDDE_NO_ERROR)
        {
        PINFO(TEXT("Dialog "));

        // Put up the properties dialog
        dwCurrentHelpId = 0;            //  F1 will be context sensitive
        ret = DialogBoxParam (hInst,
                              fAlreadyShared?
                               MAKEINTRESOURCE(IDD_PROPERTYDLG):
                               MAKEINTRESOURCE(IDD_SHAREDLG),
                              hwnd,
                              ShareDlgProc,
                              (LPARAM)lpDdeI );

        dwCurrentHelpId = 0;



        // If the user hit OK, try to apply the changes asked for.
        if (ret)
            {
            PINFO(TEXT("OK "));

            // Change static app/topic to $<pagename> form
            if (!fAlreadyShared)
                {
                register LPTSTR lpOog;

                lpOog = lpDdeI->lpszAppTopicList;

                // Jump over the first two NULL chars you find-- these
                // are the old- and new-style app/topic pairs, we don't
                // mess with them. Then jump over the next BAR_CHAR you find.
                // The first character after that is the first char of the
                // static topic-- change that to a SHR_CHAR.

                while (*lpOog++) ;
                while (*lpOog++) ;


                // BUGBUG: TEXT('|') should == BAR_CHAR. If not, this needs to
                // be adjusted.

                while (*lpOog++ != TEXT('|')) ;


                *lpOog = SHR_CHAR;
                }


            lpDdeI->fSharedFlag = 1L;

            // Get current trusted status
            if (NDDE_NO_ERROR != NDdeGetTrustedShare (NULL,
                                                      lpDdeI->lpszShareName,
                                                      adwTrust,
                                                      adwTrust + 1,
                                                      adwTrust + 2))
                {
                adwTrust[0] = 0;
                }



            DumpDdeInfo(lpDdeI, NULL);
            ret = NDdeShareSetInfo (NULL,
                                    lpDdeI->lpszShareName,
                                    2,
                                    (LPBYTE)lpDdeI,
                                    2048 * sizeof(TCHAR),
                                    0);


            if (NDDE_ACCESS_DENIED == ret)
                {
                MessageBoxID(hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
                      MB_OK | MB_ICONHAND);
                }
            else if (NDDE_NO_ERROR != ret)
                {
                PERROR(TEXT("Error from NDdeShareSetInfo %d\n\r"), ret );
                //MessageBoxID ( hInst, hwndApp, IDS_SHARINGERROR,
                //    IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK );
                NDdeMessageBox (hInst, hwndApp, (UINT)ret,
                                IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK );
                }
            else
                {
                NDdeSetTrustedShare(NULL, lpDdeI->lpszShareName, adwTrust[0]);

                #if 0
                // If the page wasn't already shared, and the user hasn't
                // changed the permissions, then we should update the permissions
                // from 'CreatorAll' to 'CreatorAll WorldRL'. (Of course,
                // if the user DID change permissions, we need to respect that
                // and NOT change them.)
                if (!fAlreadyShared)
                    {
                    PSECURITY_DESCRIPTOR pSD;
                    DWORD dwSize;
                    TCHAR atch[2048];
                    BOOL  fDacl, fDefault;
                    PACL  pacl;

                    SetShared(lpLE, TRUE);

                    pSD = atch;
                    if (NDDE_NO_ERROR == NDdeGetShareSecurity(NULL,
                          lpDdeI->lpszShareName,
                          DACL_SECURITY_INFORMATION, pSD, 2048 * sizeof(TCHAR),
                          &dwSize))
                        {
                        if (GetSecurityDescriptorDacl(pSD, &fDacl, &pacl, &fDefault))
                            {
                            if (fDefault || !fDacl)
                                {
                                if (InitializeShareSD(&pSD))
                                    {
                                    DWORD dwTrust[3];

                                    NDdeGetTrustedShare (NULL,
                                                         lpDdeI->lpszShareName,
                                                         dwTrust,
                                                         dwTrust + 1,
                                                         dwTrust + 2);

                                    NDdeSetShareSecurity (NULL,
                                                          lpDdeI->lpszShareName,
                                                          DACL_SECURITY_INFORMATION,
                                                          pSD);

                                    NDdeSetTrustedShare (NULL,
                                                         lpDdeI->lpszShareName,
                                                         dwTrust[0]);

                                    LocalFree(pSD);
                                    }
                                }
                            }
                        }
                    }
                #endif



                ///////////////////////////////////////////////
                // do the execute to change the server state
                lstrcat(lstrcpy(szBuf,SZCMD_SHARE), lpLE->name);
                PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

                if (MySyncXact ((LPBYTE)szBuf,
                                lstrlen(szBuf) +1,
                                GETMDIINFO(hwndLocal)->hExeConv,
                                0L,
                                CF_TEXT,
                                XTYP_EXECUTE,
                                SHORT_SYNC_TIMEOUT,
                                NULL))
                    {
                    SetShared(lpLE, TRUE);
                    // if (pMDI->DisplayMode == DSP_PREV)
                    //    {
                    //    InvalidateRect(pMDI->hWndListbox, NULL, FALSE);
                    //    }
                    // else
                    //    {
                    //    SendMessage(pMDI->hWndListbox,LB_SETCURSEL, tmp, 0L);
                    //    UpdateNofMStatus ( hwndLocal );
                    //    }
                    InitializeMenu(GetMenu(hwndApp));
                    }
                else
                    {
                    XactMessageBox (hInst, hwnd, IDS_APPNAME, MB_OK | MB_ICONSTOP);
                    }
                }
            }
        else if (!fAlreadyShared)  // User hit cancel on the dialog, restore the original shared state
            {
            SetShared(lpLE, FALSE);
            }
        }


    GlobalFreePtr(lpDdeI);



    return 0L;

}
