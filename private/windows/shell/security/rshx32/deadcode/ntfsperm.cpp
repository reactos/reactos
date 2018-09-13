/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

    NTFSPerm.cpp

    This file contains the implementation for the NT File System ACL
    Editor.  It is just a front end for the Generic ACL Editor that is
    specific to the NTFS.

    FILE HISTORY:
        Johnl   30-Dec-1991     Created

*/

#include "rshx32.h"
#include <getpriv.h>
#include <dbg.h>

#define INCL_NETCONS
#include <lmui.hxx>

#include <string.hxx>       // required for security.hxx
#include <security.hxx>     // OS_SECURITY_DESCRIPTOR, OS_ACL, etc.

#include "ntfsacl.h"
#include "helpnums.h"


// The following array defines the permission names for NT files.
SED_APPLICATION_ACCESS accessNTFilePerms[] =
    {
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_READ,        0, MAKEINTRESOURCE(IDS_FILE_PERM_SPEC_READ) },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_WRITE,       0, MAKEINTRESOURCE(IDS_FILE_PERM_SPEC_WRITE) },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_EXECUTE,     0, MAKEINTRESOURCE(IDS_FILE_PERM_SPEC_EXECUTE) },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_DELETE,      0, MAKEINTRESOURCE(IDS_FILE_PERM_SPEC_DELETE) },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_CHANGE_PERM, 0, MAKEINTRESOURCE(IDS_FILE_PERM_SPEC_CHANGE_PERM) },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_CHANGE_OWNER,0, MAKEINTRESOURCE(IDS_FILE_PERM_SPEC_CHANGE_OWNER) },

      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_NO_ACCESS,    0, MAKEINTRESOURCE(IDS_FILE_PERM_GEN_NO_ACCESS) },
      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_READ,         0, MAKEINTRESOURCE(IDS_FILE_PERM_GEN_READ) },
      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_MODIFY,       0, MAKEINTRESOURCE(IDS_FILE_PERM_GEN_MODIFY) },
      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_ALL,          0, MAKEINTRESOURCE(IDS_FILE_PERM_GEN_ALL) }
    };

#define COUNT_NTFILEPERMS_ARRAY     ARRAYSIZE(accessNTFilePerms)

// The following array defines the permission names for NT directories.
SED_APPLICATION_ACCESS accessNTDirPerms[] =
{
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_READ,        0, MAKEINTRESOURCE(IDS_DIR_PERM_SPEC_READ) },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_WRITE,       0, MAKEINTRESOURCE(IDS_DIR_PERM_SPEC_WRITE) },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_EXECUTE,     0, MAKEINTRESOURCE(IDS_DIR_PERM_SPEC_EXECUTE) },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_DELETE,      0, MAKEINTRESOURCE(IDS_DIR_PERM_SPEC_DELETE) },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_CHANGE_PERM, 0, MAKEINTRESOURCE(IDS_DIR_PERM_SPEC_CHANGE_PERM) },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_CHANGE_OWNER,0, MAKEINTRESOURCE(IDS_DIR_PERM_SPEC_CHANGE_OWNER) },

  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_NO_ACCESS,NEWFILE_PERM_GEN_NO_ACCESS, MAKEINTRESOURCE(IDS_DIR_PERM_GEN_NO_ACCESS) },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_LIST,     NEWFILE_PERM_GEN_LIST,      MAKEINTRESOURCE(IDS_DIR_PERM_GEN_LIST) },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_READ,     NEWFILE_PERM_GEN_READ,      MAKEINTRESOURCE(IDS_DIR_PERM_GEN_READ) },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_DEPOSIT,  NEWFILE_PERM_GEN_DEPOSIT,   MAKEINTRESOURCE(IDS_DIR_PERM_GEN_DEPOSIT) },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_PUBLISH,  NEWFILE_PERM_GEN_PUBLISH,   MAKEINTRESOURCE(IDS_DIR_PERM_GEN_PUBLISH) },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_MODIFY,   NEWFILE_PERM_GEN_MODIFY,    MAKEINTRESOURCE(IDS_DIR_PERM_GEN_MODIFY) },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_ALL,      NEWFILE_PERM_GEN_ALL,       MAKEINTRESOURCE(IDS_DIR_PERM_GEN_ALL) },

  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_READ,         0, MAKEINTRESOURCE(IDS_NEWFILE_PERM_SPEC_READ) },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_WRITE,        0, MAKEINTRESOURCE(IDS_NEWFILE_PERM_SPEC_WRITE) },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_EXECUTE,      0, MAKEINTRESOURCE(IDS_NEWFILE_PERM_SPEC_EXECUTE) },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_DELETE,       0, MAKEINTRESOURCE(IDS_NEWFILE_PERM_SPEC_DELETE) },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_CHANGE_PERM,  0, MAKEINTRESOURCE(IDS_NEWFILE_PERM_SPEC_CHANGE_PERM) },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_CHANGE_OWNER, 0, MAKEINTRESOURCE(IDS_NEWFILE_PERM_SPEC_CHANGE_OWNER) }
};

#define COUNT_NTDIRPERMS_ARRAY      ARRAYSIZE(accessNTDirPerms)


DWORD WINAPI
CreateNTFSPermPage(DWORD            dwFlags,
                   LPDATAOBJECT     pDataObj,
                   HPROPSHEETPAGE  *phPage)
{
    ASSERT(IsResDisk(dwFlags));
    ASSERT(IsServerNT(dwFlags));
    ASSERT(IsVolumeNTACLS(dwFlags));
    ASSERT(pDataObj);
    ASSERT(phPage);
    ASSERT(g_pfnSHDragQueryFile);

    APIERR err = NERR_Success;
    PNT_PAGE_CONTEXT pContext = NULL;

    *phPage = NULL;

    // BUGBUG
    // jms - need to ensure we have ownership of the HGLOBAL returned by GetData.
    // i.e. we need medium.punkForRelease to be NULL or else copy the buffer.
    STGMEDIUM medium;
    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    if (FAILED(pDataObj->GetData(&fe, &medium)))
        return ERROR_INVALID_DATA;

    do { // error breakout

        HDROP hDrop = (HDROP)medium.hGlobal;

        // Get the number of files/dirs in the selection
        UINT cItems = (*g_pfnSHDragQueryFile)(hDrop, (UINT)-1, NULL, 0);
        ASSERT(cItems > 0);

        pContext = new NT_PAGE_CONTEXT(hDrop);
        if (!pContext)
        {
            ReleaseStgMedium(&medium);
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Assume everything is OK to start
        BOOL fCanRead = TRUE;
        BOOL fCanWriteDACL = TRUE;
        BOOL fCanWriteOwner = TRUE;
        BOOL fCheckSecurity = TRUE; // Check write access unless bad DACL intersection

        PSECURITY_DESCRIPTOR pSecurityDesc = NULL;
        BUFFER bufSD(1024);

        // Get the name of the first file
        TCHAR szObjectName[MAX_PATH];
        (*g_pfnSHDragQueryFile)(hDrop, 0, szObjectName, ARRAYSIZE(szObjectName));

        err = GetSecurity(szObjectName,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION,
                          &bufSD,
                          NULL /* pfPrivAdjusted */);
        switch (err)
        {
        case ERROR_ACCESS_DENIED:
            fCanRead = FALSE;
            err = NERR_Success ;
            break;

        case NO_ERROR:
            pSecurityDesc = (PSECURITY_DESCRIPTOR)bufSD.QueryPtr();
            break;

        default:
            break;
        }
        if (err)
            break;

        GENERIC_MAPPING NTFSGenericMapping;
        InitializeNTFSGenericMapping(&NTFSGenericMapping);

        //
        // Make a copy of the security descriptor so we can modify
        // it if necessary, depending on the intersection.
        //
        OS_SECURITY_DESCRIPTOR osSecurityDesc(pSecurityDesc, TRUE /* fCopy */);
        if (err = osSecurityDesc.QueryError())
            break;

        if (cItems > 1) // multiple selection
        {
            BOOL fOwnerEqual = FALSE;
            BOOL fACLEqual = FALSE;
	    TCHAR szFailingFile[MAX_PATH];
            MSGID idsPromptToContinue = IDS_BAD_INTERSECTION;
            LPCTSTR pszPrompt = szObjectName;

            if (!fCanRead ||
                (err = osSecurityDesc.QueryError()) ||
                (err = CompareNTFSSecurityIntersection(hDrop,
                                                       pSecurityDesc, //osSecurityDesc,
                                                       &fOwnerEqual,
                                                       NULL /* pfGroupEqual */,
                                                       &fACLEqual /* pfDACLEqual */,
                                                       NULL /* pfSACLEqual */,
                                                       &NTFSGenericMapping,
                                                       &NTFSGenericMapping,
                                                       TRUE /* fMapGenAll */,
                                                       IsResContainer(dwFlags),
                                                       szFailingFile,
                                                       ARRAYSIZE(szFailingFile))))
	    {
                //
                //  If we didn't have access to read one of the security
                //  descriptors, then give the user the option of continuing
                //
                if (!fCanRead || err == ERROR_ACCESS_DENIED)
                {
                    if (fCanRead)
                    {
                        fCanRead = FALSE;
                        pszPrompt = szFailingFile;
                    }
                    idsPromptToContinue = IERR_MULTI_SELECT_AND_CANT_READ;
                    err = NERR_Success;
                }
                else
                    break;
            }

            if (!fACLEqual || !fCanRead)
            {
                // Clear the DACL
                OS_ACL osacl(NULL);
                if (err = osSecurityDesc.SetDACL(TRUE, &osacl))
                    break;

                // Point to the copy of the security descriptor
                pSecurityDesc = osSecurityDesc;

                // Set the context info to prompt the user (happens later)
                pContext->idsPromptString = idsPromptToContinue;
                pContext->pszPromptFile1 = CopyString(pszPrompt);
                pContext->pszPromptFile2 = CopyString(szFailingFile);

                // Don't check write access... we already know the DACLs aren't
                // equal so we wouldn't gain anything by checking. If the user
                // makes changes later, we will try to write them and see what
                // happens.
                fCheckSecurity = FALSE;
            }
            if (err)
                break;

            if (!fOwnerEqual)
            {
                // REVIEW - Should we just clear the group here or check
                // the group for equality above?

                // Clear the owner/group
                if ((err = osSecurityDesc.SetOwner(FALSE, NULL)) ||
                    (err = osSecurityDesc.SetGroup(FALSE, NULL)))
                    break;

                // Point to the copy of the security descriptor
                pSecurityDesc = osSecurityDesc;
            }

        } // if IsMultiSelect


        // Based on what we are doing, choose the correct set of masks
        // and resource strings.

        SED_HELP_INFO sedHelpInfo;
        sedHelpInfo.pszHelpFileName                             = MAKEINTRESOURCE(IDS_FILE_PERM_HELP_FILE);
        sedHelpInfo.aulHelpContext[HC_ADD_USER_DLG]             = HC_SED_USER_BROWSER_DIALOG;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG]  = HC_SED_USER_BROWSER_LOCALGROUP;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG]  = HC_SED_USER_BROWSER_GLOBALGROUP;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG]      = HC_SED_USER_BROWSER_FINDUSER;

        SED_OBJECT_TYPE_DESCRIPTOR sedObjectType;
        sedObjectType.Revision                  = SED_REVISION;
        sedObjectType.IsContainer               = (BOOLEAN)IsResContainer(dwFlags);
        sedObjectType.MapSpecificPermsToGeneric = TRUE;
        sedObjectType.GenericMapping            = &NTFSGenericMapping,
        sedObjectType.GenericMappingNewObjects  = &NTFSGenericMapping,
        sedObjectType.HelpInfo                  = &sedHelpInfo;

        SED_APPLICATION_ACCESSES sedAccesses;

        if (IsResContainer(dwFlags))
        {
            sedAccesses.Count           = COUNT_NTDIRPERMS_ARRAY;
            sedAccesses.AccessGroup     = accessNTDirPerms;
            sedAccesses.DefaultPermName = MAKEINTRESOURCE(IDS_DIR_PERM_GEN_READ);

            sedObjectType.AllowNewObjectPerms             = TRUE;
            sedObjectType.ObjectTypeName                  = MAKEINTRESOURCE(IDS_DIRECTORY);
            sedObjectType.ApplyToSubContainerTitle        = MAKEINTRESOURCE(IDS_NT_ASSIGN_PERM_TITLE);
            sedObjectType.ApplyToObjectsTitle             = MAKEINTRESOURCE(IDS_NT_ASSIGN_FILE_PERM_TITLE);
            sedObjectType.ApplyToSubContainerConfirmation = MAKEINTRESOURCE(IDS_TREE_APPLY_WARNING);
            sedObjectType.SpecialObjectAccessTitle        = MAKEINTRESOURCE(IDS_NT_DIR_SPECIAL_ACCESS);
            sedObjectType.SpecialNewObjectAccessTitle     = MAKEINTRESOURCE(IDS_NT_NEWOBJ_SPECIAL_ACCESS);

            sedHelpInfo.aulHelpContext[HC_MAIN_DLG]                    = HC_SED_NT_DIR_PERMS_DLG;
            sedHelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = HC_SED_NT_SPECIAL_DIRS_FM;
            sedHelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = HC_SED_NT_SPECIAL_NEW_FILES_FM;
        }
        else
        {
            sedAccesses.Count           = COUNT_NTFILEPERMS_ARRAY;
            sedAccesses.AccessGroup     = accessNTFilePerms;
            sedAccesses.DefaultPermName = MAKEINTRESOURCE(IDS_FILE_PERM_GEN_READ);

            sedObjectType.AllowNewObjectPerms             = FALSE;
            sedObjectType.ObjectTypeName                  = MAKEINTRESOURCE(IDS_FILE);
            sedObjectType.ApplyToSubContainerTitle        = NULL;
            sedObjectType.ApplyToObjectsTitle             = NULL;
            sedObjectType.ApplyToSubContainerConfirmation = NULL;
            sedObjectType.SpecialObjectAccessTitle        = MAKEINTRESOURCE(IDS_NT_FILE_SPECIAL_ACCESS);
            sedObjectType.SpecialNewObjectAccessTitle     = NULL;

            sedHelpInfo.aulHelpContext[HC_MAIN_DLG]                    = HC_SED_NT_FILE_PERMS_DLG;
            sedHelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = HC_SED_NT_SPECIAL_FILES_FM;
            sedHelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0;
        }

        // See if we can write the DACL
        if (fCheckSecurity &&
            (err = ::CheckFileSecurity(szObjectName,
                                       WRITE_DAC,
                                       &fCanWriteDACL)))
        {
            break;
        }

        // See if we can write the Owner
        if (EnablePrivilege(SE_TAKE_OWNERSHIP_PRIVILEGE))
        {
            // The user has the take ownership privilege
            ReleasePrivileges();
            fCanWriteOwner = TRUE;
        }
        else
        {
            err = GetLastError();

            if (err == ERROR_PRIVILEGE_NOT_HELD)
            {
                // The user doesn't have the privilege, but may have explicit
                // permission to take ownership.  Check here.

                if (fCheckSecurity &&
                    (err = ::CheckFileSecurity(szObjectName,
                                               WRITE_OWNER,
                                               &fCanWriteOwner)))
                {
                    break;
                }
            }
            else
            {
                // some other error from EnablePrivilege()...
                break;
            }
        }

        if (cItems > 1)
        {
            // Replace the object name with the "X files selected" string.
            TCHAR szTemp[MAX_PATH];
            szTemp[0] = TEXT('\0');

            LoadString(g_hInstance,
                       IsResContainer(dwFlags) ? IDS_DIRECTORY_MULTI_SEL
                                               : IDS_FILE_MULTI_SEL,
                       szTemp,
                       ARRAYSIZE(szTemp));

            wsprintf(szObjectName, szTemp, cItems);
        }

        DWORD dwFlags = SED_USEREFPARENT | SED_USEPSPCALLBACK;

        if (!fCanRead)
            dwFlags |= SED_COULDNT_READ_DACL;
        if (!fCanWriteDACL)
            dwFlags |= SED_CANT_WRITE_DACL;
        if (!fCanWriteOwner)
            dwFlags |= SED_CANT_WRITE_OWNER;

        if (!g_hAclEditDll)
        {
            g_hAclEditDll = LoadLibrary(DLL_ACLEDIT);
            if (!g_hAclEditDll)
                break;
        }

        static CREATEACLEDITPAGE g_pfnCreateDACLPage = NULL;
        if (!g_pfnCreateDACLPage)
        {
            g_pfnCreateDACLPage = (CREATEACLEDITPAGE)GetProcAddress(
                                        g_hAclEditDll,
                                        EXP_SEDCREATEPERMPAGE);
            if (!g_pfnCreateDACLPage)
                break;
        }
        err = (*g_pfnCreateDACLPage)(g_hInstance,
                                     NULL,//pszServerName,
                                     &sedObjectType,
                                     &sedAccesses,
                                     szObjectName,
                                     NTFSApplySecurity2,
                                     (ULONG)pContext,
                                     pSecurityDesc,
                                     dwFlags,
                                     (LPUINT)&g_cRefThisDll,
                                     NTFSPSPCallback,
                                     phPage);
    } while (FALSE);

    if ( *phPage == NULL )
    {
        // clean up
//        ReleaseStgMedium(&medium); // context destructor takes care of this
        delete pContext;

        ODS(TEXT("::CreateNTFSSecurityPages - CreateNTFSPermPage failed\n\r"));
    }
    // else don't call ReleaseStgMedium or free hDrop since it will be freed
    // when the page is destroyed

    return err;
}
