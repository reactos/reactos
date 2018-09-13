/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

    NTFSAudt.cpp

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


// The following array defines the auditting names for NT files.
SED_APPLICATION_ACCESS accessNTFileAudits[] =
    {
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_READ,        0, MAKEINTRESOURCE(IDS_FILE_AUDIT_READ) },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_WRITE,       0, MAKEINTRESOURCE(IDS_FILE_AUDIT_WRITE) },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_EXECUTE,     0, MAKEINTRESOURCE(IDS_FILE_AUDIT_EXECUTE) },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_DELETE,      0, MAKEINTRESOURCE(IDS_FILE_AUDIT_DELETE) },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_CHANGE_PERM, 0, MAKEINTRESOURCE(IDS_FILE_AUDIT_CHANGE_PERM) },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_CHANGE_OWNER,0, MAKEINTRESOURCE(IDS_FILE_AUDIT_CHANGE_OWNER) }
    };

#define COUNT_NTFILEAUDITS_ARRAY    ARRAYSIZE(accessNTFileAudits)

// The following array defines the auditting names for NT directories.
SED_APPLICATION_ACCESS accessNTDirAudits[] =
    {
      { SED_DESC_TYPE_AUDIT, DIR_AUDIT_READ,        0, MAKEINTRESOURCE(IDS_DIR_AUDIT_READ) },
      { SED_DESC_TYPE_AUDIT, DIR_AUDIT_WRITE,       0, MAKEINTRESOURCE(IDS_DIR_AUDIT_WRITE) },
      { SED_DESC_TYPE_AUDIT, DIR_AUDIT_EXECUTE,     0, MAKEINTRESOURCE(IDS_DIR_AUDIT_EXECUTE) },
      { SED_DESC_TYPE_AUDIT, DIR_AUDIT_DELETE,      0, MAKEINTRESOURCE(IDS_DIR_AUDIT_DELETE) },
      { SED_DESC_TYPE_AUDIT, DIR_AUDIT_CHANGE_PERM, 0, MAKEINTRESOURCE(IDS_DIR_AUDIT_CHANGE_PERM) },
      { SED_DESC_TYPE_AUDIT, DIR_AUDIT_CHANGE_OWNER,0, MAKEINTRESOURCE(IDS_DIR_AUDIT_CHANGE_OWNER) }
    };

#define COUNT_NTDIRAUDITS_ARRAY     ARRAYSIZE(accessNTDirAudits)


DWORD WINAPI
CreateNTFSAuditPage(DWORD           dwFlags,
                    LPDATAOBJECT    pDataObj,
                    HPROPSHEETPAGE *phPage)
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

        BOOL fPrivAdjusted = FALSE;

        PSECURITY_DESCRIPTOR pSecurityDesc = NULL;
        BUFFER bufSD(1024);

        // Get the name of the first file
        TCHAR szObjectName[MAX_PATH];
        (*g_pfnSHDragQueryFile)(hDrop, 0, szObjectName, ARRAYSIZE(szObjectName));

        err = GetSecurity(szObjectName,
                          SACL_SECURITY_INFORMATION,
                          &bufSD,
                          &fPrivAdjusted);
        switch (err)
        {
        case ERROR_ACCESS_DENIED:
            err = ERROR_PRIVILEGE_NOT_HELD;
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
            BOOL fACLEqual = FALSE;
	    TCHAR szFailingFile[MAX_PATH];

            if ((err = osSecurityDesc.QueryError()) ||
                (err = CompareNTFSSecurityIntersection(hDrop,
                                                       pSecurityDesc, //osSecurityDesc,
                                                       NULL /* pfOwnerEqual */,
                                                       NULL /* pfGroupEqual */,
                                                       NULL /* pfDACLEqual */,
                                                       &fACLEqual /* pfSACLEqual */,
                                                       &NTFSGenericMapping,
                                                       &NTFSGenericMapping,
                                                       TRUE /* fMapGenAll */,
                                                       IsResContainer(dwFlags),
                                                       szFailingFile,
                                                       ARRAYSIZE(szFailingFile))))
	    {
                break;
            }

            if (!fACLEqual)
            {
                // Clear the SACL
                OS_ACL osacl(NULL);
                if (err = osSecurityDesc.SetSACL(TRUE, &osacl))
                    break;

                // Point to the copy of the security descriptor
                pSecurityDesc = osSecurityDesc;

                // Set the context info to prompt the user
                pContext->idsPromptString = IDS_BAD_INTERSECTION;
                pContext->pszPromptFile1 = CopyString(szObjectName);
                pContext->pszPromptFile2 = CopyString(szFailingFile);
            }

        } // if IsMultiSelect

        if (fPrivAdjusted)
            ReleasePrivileges();


        // Based on what we are doing, choose the correct set of masks
        // and resource strings.

        SED_HELP_INFO sedHelpInfo;
        sedHelpInfo.pszHelpFileName                                 = MAKEINTRESOURCE(IDS_FILE_PERM_HELP_FILE);
        sedHelpInfo.aulHelpContext[HC_ADD_USER_DLG]                 = HC_SED_USER_BROWSER_AUDIT_DLG;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG]      = HC_SED_USER_BR_AUDIT_LOCALGROUP;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG]      = HC_SED_USER_BR_AUDIT_GLOBALGROUP;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG]          = HC_SED_USER_BR_AUDIT_FINDUSER;
        sedHelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]           = 0;
        sedHelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG]  = 0;

        SED_OBJECT_TYPE_DESCRIPTOR sedObjectType;
        sedObjectType.Revision                      = SED_REVISION;
        sedObjectType.IsContainer                   = (BOOLEAN)IsResContainer(dwFlags);
        sedObjectType.MapSpecificPermsToGeneric     = TRUE;
        sedObjectType.GenericMapping                = &NTFSGenericMapping,
        sedObjectType.GenericMappingNewObjects      = &NTFSGenericMapping,
        sedObjectType.HelpInfo                      = &sedHelpInfo;
        sedObjectType.AllowNewObjectPerms           = FALSE;
        sedObjectType.SpecialObjectAccessTitle      = NULL;
        sedObjectType.SpecialNewObjectAccessTitle   = NULL;

        SED_APPLICATION_ACCESSES sedAccesses;

        if (IsResContainer(dwFlags))
        {
            sedAccesses.Count           = COUNT_NTDIRAUDITS_ARRAY;
            sedAccesses.AccessGroup     = accessNTDirAudits;
            sedAccesses.DefaultPermName = MAKEINTRESOURCE(IDS_DIR_PERM_GEN_READ);

            sedObjectType.ObjectTypeName                  = MAKEINTRESOURCE(IDS_DIRECTORY);
            sedObjectType.ApplyToSubContainerTitle        = MAKEINTRESOURCE(IDS_NT_ASSIGN_AUDITS_TITLE);
            sedObjectType.ApplyToObjectsTitle             = MAKEINTRESOURCE(IDS_NT_ASSIGN_FILE_AUDITS_TITLE);
            sedObjectType.ApplyToSubContainerConfirmation = MAKEINTRESOURCE(IDS_TREE_APPLY_WARNING);

            sedHelpInfo.aulHelpContext[HC_MAIN_DLG] = HC_SED_NT_DIR_AUDITS_DLG;
        }
        else
        {
            sedAccesses.Count           = COUNT_NTFILEAUDITS_ARRAY;
            sedAccesses.AccessGroup     = accessNTFileAudits;
            sedAccesses.DefaultPermName = MAKEINTRESOURCE(IDS_FILE_PERM_GEN_READ);

            sedObjectType.ObjectTypeName                  = MAKEINTRESOURCE(IDS_FILE);
            sedObjectType.ApplyToSubContainerTitle        = NULL;
            sedObjectType.ApplyToObjectsTitle             = NULL;
            sedObjectType.ApplyToSubContainerConfirmation = NULL;

            sedHelpInfo.aulHelpContext[HC_MAIN_DLG] = HC_SED_NT_FILE_AUDITS_DLG;
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

        if (!g_hAclEditDll)
        {
            g_hAclEditDll = LoadLibrary(DLL_ACLEDIT);
            if (!g_hAclEditDll)
                break;
        }

        static CREATEACLEDITPAGE g_pfnCreateSACLPage = NULL;
        if (!g_pfnCreateSACLPage)
        {
            g_pfnCreateSACLPage = (CREATEACLEDITPAGE)GetProcAddress(
                                        g_hAclEditDll,
                                        EXP_SEDCREATEAUDITPAGE);
            if (!g_pfnCreateSACLPage)
                break;
        }
        err = (*g_pfnCreateSACLPage)(g_hInstance,
                                     NULL,//pszServerName,
                                     &sedObjectType,
                                     &sedAccesses,
                                     szObjectName,
                                     NTFSApplySecurity2,
                                     (ULONG)pContext,
                                     pSecurityDesc,
                                     SED_USEREFPARENT | SED_USEPSPCALLBACK,
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
