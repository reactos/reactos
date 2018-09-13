//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1995.
//
//  File:       acl.cxx
//
//  Contents:   Implementation for the Shares Acl Editor in the "Sharing"
//              property page.  It is just a front end for the Generic
//              ACL Editor that is specific to Shares.
//
//  History:    5-Apr-95 BruceFo  Stole from net\ui\shellui\share\shareacl.cxx
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <lmerr.h>

extern "C"
{
#include <sedapi.h>
}

#include "resource.h"
#include "helpids.h"
#include "acl.hxx"
#include "util.hxx"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef
DWORD
(*SedDiscretionaryAclEditorType)(
        HWND                         Owner,
        HANDLE                       Instance,
        LPWSTR                       Server,
        PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
        PSED_APPLICATION_ACCESSES    ApplicationAccesses,
        LPWSTR                       ObjectName,
        PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
        ULONG                        CallbackContext,
        PSECURITY_DESCRIPTOR         SecurityDescriptor,
        BOOLEAN                      CouldntReadDacl,
        BOOLEAN                      CantWriteDacl,
        LPDWORD                      SEDStatusReturn,
        DWORD                        Flags
        );

// NOTE: the SedDiscretionaryAclEditor string is used in GetProcAddress to
// get the correct entrypoint. Since GetProcAddress is not UNICODE, this string
// must be ANSI.
#define ACLEDIT_DLL_STRING                 TEXT("acledit.dll")
#define SEDDISCRETIONARYACLEDITOR_STRING   ("SedDiscretionaryAclEditor")

//
// Declare the callback routine based on typedef in sedapi.h.
//

DWORD
SedCallback(
    HWND                   hwndParent,
    HANDLE                 hInstance,
    ULONG                  ulCallbackContext,
    PSECURITY_DESCRIPTOR   pSecDesc,
    PSECURITY_DESCRIPTOR   pSecDescNewObjects,
    BOOLEAN                fApplyToSubContainers,
    BOOLEAN                fApplyToSubObjects,
    LPDWORD                StatusReturn
    );

//
// Structure for callback function's usage. A pointer to this is passed as
// ulCallbackContext. The callback functions sets bSecDescModified to TRUE
// and makes a copy of the security descriptor. The caller of EditShareAcl
// is responsible for deleting the memory in pSecDesc if bSecDescModified is
// TRUE. This flag will be FALSE if the user hit CANCEL in the ACL editor.
//
struct SHARE_CALLBACK_INFO
{
    BOOL                 bSecDescModified;
    PSECURITY_DESCRIPTOR pSecDesc;
};


//
// Local function prototypes
//

VOID
InitializeShareGenericMapping(
    IN OUT PGENERIC_MAPPING pSHAREGenericMapping
    );

LONG
CreateDefaultSecDesc(
    OUT PSECURITY_DESCRIPTOR* ppSecDesc
    );

VOID
DeleteDefaultSecDesc(
    IN PSECURITY_DESCRIPTOR pSecDesc
    );

//
// The following two arrays define the permission names for NT Files.  Note
// that each index in one array corresponds to the index in the other array.
// The second array will be modifed to contain a string pointer pointing to
// a loaded string corresponding to the IDS_* in the first array.
//
DWORD g_dwSharePermNames[] =
{
    IDS_ACLEDIT_PERM_GEN_NO_ACCESS,
    IDS_ACLEDIT_PERM_GEN_READ,
    IDS_ACLEDIT_PERM_GEN_MODIFY,
    IDS_ACLEDIT_PERM_GEN_ALL
};

SED_APPLICATION_ACCESS g_SedAppAccessSharePerms[] =
{
    { SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_NO_ACCESS, 0, NULL },
    { SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_READ,      0, NULL },
    { SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_MODIFY,    0, NULL },
    { SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_ALL,       0, NULL }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Function:   EditShareAcl
//
//  Synopsis:   Invokes the generic ACL editor, specifically for NT shares
//
//  Arguments:  [hwndParent] - Parent window handle
//              [pszShareName] - Fully qualified name of resource we will
//                  edit, basically a share name.
//              [pSecDesc] - The initial security descriptor. If NULL, we will
//                  create a default that is "World all" access.
//              [pbSecDescModified] - Set to TRUE if the security descriptor
//                  was modified (i.e., the user hit "OK"), or FALSE if not
//                  (i.e., the user hit "Cancel")
//              [ppSecDesc] - *ppSecDesc points to a new security descriptor
//                  if *pbSecDescModified is TRUE. This memory must be freed
//                  by the caller.
//
//  History:
//        ChuckC   10-Aug-1992  Created. Culled from NTFS ACL code.
//        Yi-HsinS 09-Oct-1992  Added ulHelpContextBase
//        BruceFo  4-Apr-95     Stole and used in ntshrui.dll
//
//--------------------------------------------------------------------------

LONG
EditShareAcl(
    IN HWND                      hwndParent,
    IN LPWSTR                    pszServerName,
    IN TCHAR *                   pszShareName,
    IN PSECURITY_DESCRIPTOR      pSecDesc,
    OUT BOOL*                    pbSecDescModified,
    OUT PSECURITY_DESCRIPTOR*    ppSecDesc
    )
{
    appAssert(NULL != pszShareName);
    appDebugOut((DEB_TRACE, "EditShareAcl, share %ws\n", pszShareName));

    appAssert(NULL == pSecDesc || IsValidSecurityDescriptor(pSecDesc));
    appAssert(pbSecDescModified);
    appAssert(ppSecDesc);

    *pbSecDescModified = FALSE;

    LONG err ;
    PWSTR pszPermName;
    BOOL bCreatedDefaultSecDesc = FALSE;

    do // error breakout
    {
        /*
         * if pSecDesc is NULL, this is new share or a share with no
         * security descriptor.
         * we go and create a new (default) security descriptor.
         */
        if (NULL == pSecDesc)
        {
            LONG err = CreateDefaultSecDesc(&pSecDesc);
            if (err != NERR_Success)
            {
                appDebugOut((DEB_ERROR, "CreateDefaultSecDesc failed, 0x%08lx\n", err));
                break ;
            }

            appDebugOut((DEB_TRACE, "CreateDefaultSecDesc descriptor = 0x%08lx\n", pSecDesc));
            bCreatedDefaultSecDesc = TRUE;
        }

        appAssert(IsValidSecurityDescriptor(pSecDesc));

        /* Retrieve the resource strings appropriate for the type of object we
         * are looking at
         */

        WCHAR szTypeName[50];
        LoadString(g_hInstance, IDS_ACLEDIT_TITLE, szTypeName, ARRAYLEN(szTypeName));

        WCHAR szDefaultPermName[50];
        LoadString(g_hInstance, IDS_ACLEDIT_PERM_GEN_READ, szDefaultPermName, ARRAYLEN(szDefaultPermName));

        /*
         * other misc stuff we need pass to security editor
         */
        SED_OBJECT_TYPE_DESCRIPTOR sedObjDesc ;
        SED_HELP_INFO sedHelpInfo ;
        GENERIC_MAPPING SHAREGenericMapping ;

        // setup mappings
        InitializeShareGenericMapping( &SHAREGenericMapping ) ;

        // setup help: BUGBUG
        WCHAR szHelpFile[50];
        LoadString(g_hInstance, IDS_HELPFILENAME, szHelpFile, ARRAYLEN(szHelpFile));

        sedHelpInfo.pszHelpFileName = szHelpFile;
        sedHelpInfo.aulHelpContext[HC_MAIN_DLG]                 = ACL_HC_MAIN_DLG;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_DLG]             = ACL_HC_ADD_USER_DLG;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG]  = ACL_HC_ADD_USER_MEMBERS_LG_DLG;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG]  = ACL_HC_ADD_USER_MEMBERS_GG_DLG;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG]      = ACL_HC_ADD_USER_SEARCH_DLG;

        // These are not used, set to zero
        sedHelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = 0 ;
        sedHelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0 ;

        // setup the object description
        sedObjDesc.Revision                    = SED_REVISION1 ;
        sedObjDesc.IsContainer                 = FALSE ;
        sedObjDesc.AllowNewObjectPerms         = FALSE ;
        sedObjDesc.MapSpecificPermsToGeneric   = TRUE ;
        sedObjDesc.GenericMapping              = &SHAREGenericMapping ;
        sedObjDesc.GenericMappingNewObjects    = &SHAREGenericMapping ;
        sedObjDesc.HelpInfo                    = &sedHelpInfo ;
        sedObjDesc.ObjectTypeName              = szTypeName ;
        sedObjDesc.SpecialObjectAccessTitle    = NULL ;

        /* Now we need to load the global arrays with the permission names
         * from the resource file.
         */
        UINT cArrayItems  = ARRAYLEN(g_SedAppAccessSharePerms);
        PSED_APPLICATION_ACCESS aSedAppAccess = g_SedAppAccessSharePerms ;

        /* Loop through each permission title retrieving the text from the
         * resource file and setting the pointer in the array.
         */

        for ( UINT i = 0 ; i < cArrayItems ; i++ )
        {
            pszPermName = GetResourceString(g_dwSharePermNames[i]) ;
            if (NULL == pszPermName)
            {
                appDebugOut((DEB_ERROR, "GetResourceString failed\n"));
                break ;
            }
            aSedAppAccess[i].PermissionTitle = pszPermName;
        }
        if (i < cArrayItems)
        {
            appDebugOut((DEB_ERROR, "failed to get all share permission names\n"));
            break ;
        }

        SED_APPLICATION_ACCESSES sedAppAccesses ;
        sedAppAccesses.Count           = cArrayItems ;
        sedAppAccesses.AccessGroup     = aSedAppAccess ;
        sedAppAccesses.DefaultPermName = szDefaultPermName;

        /*
         * pass this along so when the call back function is called,
         * we can set it.
         */
        SHARE_CALLBACK_INFO callbackinfo ;
        callbackinfo.pSecDesc         = NULL;
        callbackinfo.bSecDescModified = FALSE;

        //
        // Now, load up the ACL editor and invoke it. We don't keep it around
        // because our DLL is loaded whenever the system is, so we don't want
        // the netui*.dll's hanging around as well...
        //

        HINSTANCE hInstanceAclEditor = NULL;
        SedDiscretionaryAclEditorType pSedDiscretionaryAclEditor = NULL;

        hInstanceAclEditor = LoadLibrary(ACLEDIT_DLL_STRING);
        if (NULL == hInstanceAclEditor)
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "LoadLibrary of acledit.dll failed, 0x%08lx\n", err));
            break;
        }

        pSedDiscretionaryAclEditor = (SedDiscretionaryAclEditorType)
            GetProcAddress(hInstanceAclEditor,SEDDISCRETIONARYACLEDITOR_STRING);
        if ( pSedDiscretionaryAclEditor == NULL )
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "GetProcAddress of SedDiscretionaryAclEditor failed, 0x%08lx\n", err));
            break;
        }

        DWORD dwSedReturnStatus ;

        appAssert(pSedDiscretionaryAclEditor != NULL);
        err = (*pSedDiscretionaryAclEditor)(
                                hwndParent,
                                g_hInstance,
                                pszServerName,
                                &sedObjDesc,
                                &sedAppAccesses,
                                pszShareName,
                                SedCallback,
                                (ULONG) &callbackinfo,
                                pSecDesc,
                                FALSE,  // always can read
                                FALSE,  // If we can read, we can write
                                &dwSedReturnStatus,
                                0 ) ;

        if (!FreeLibrary(hInstanceAclEditor))
        {
            LONG err2 = GetLastError();
            appDebugOut((DEB_ERROR, "FreeLibrary of acledit.dll failed, 0x%08lx\n", err2));
            // not fatal: continue...
        }

        if (0 != err)
        {
            appDebugOut((DEB_ERROR, "SedDiscretionaryAclEditor failed, 0x%08lx\n", err));
            break ;
        }

        *pbSecDescModified = callbackinfo.bSecDescModified ;

        if (*pbSecDescModified)
        {
            *ppSecDesc = callbackinfo.pSecDesc;

            appDebugOut((DEB_TRACE, "After calling acl editor, *ppSecDesc = 0x%08lx\n", *ppSecDesc));
            appAssert(IsValidSecurityDescriptor(*ppSecDesc));
        }

    } while (FALSE) ;

    //
    // Free memory...
    //

    UINT cArrayItems  = ARRAYLEN(g_SedAppAccessSharePerms);
    PSED_APPLICATION_ACCESS aSedAppAccess = g_SedAppAccessSharePerms ;
    for ( UINT i = 0 ; i < cArrayItems ; i++ )
    {
        pszPermName = aSedAppAccess[i].PermissionTitle;
        if (NULL == pszPermName)
        {
            // if we hit a NULL, that's it!
            break ;
        }

        delete[] pszPermName;
    }

    if (bCreatedDefaultSecDesc)
    {
        DeleteDefaultSecDesc(pSecDesc);
    }

    appAssert(!*pbSecDescModified || IsValidSecurityDescriptor(*ppSecDesc));

    if (0 != err)
    {
        MyErrorDialog(hwndParent, IERR_NOACLEDITOR);
    }

    return err;
}


//+-------------------------------------------------------------------------
//
//  Function:   SedCallback
//
//  Synopsis:   Security Editor callback for the SHARE ACL Editor
//
//  Arguments:  See sedapi.h
//
//  History:
//        ChuckC   10-Aug-1992  Created
//        BruceFo  4-Apr-95     Stole and used in ntshrui.dll
//
//--------------------------------------------------------------------------

DWORD
SedCallback(
    HWND                   hwndParent,
    HANDLE                 hInstance,
    ULONG                  ulCallbackContext,
    PSECURITY_DESCRIPTOR   pSecDesc,
    PSECURITY_DESCRIPTOR   pSecDescNewObjects,
    BOOLEAN                fApplyToSubContainers,
    BOOLEAN                fApplyToSubObjects,
    LPDWORD                StatusReturn
    )
{
    appDebugOut((DEB_TRACE, "SedCallback, got pSecDesc = 0x%08lx\n", pSecDesc));
    appAssert(IsValidSecurityDescriptor(pSecDesc));

    SHARE_CALLBACK_INFO* pCallbackInfo = (SHARE_CALLBACK_INFO *)ulCallbackContext;
    appAssert(NULL != pCallbackInfo);

    delete[] (BYTE*)pCallbackInfo->pSecDesc;
    pCallbackInfo->pSecDesc         = CopySecurityDescriptor(pSecDesc);
    pCallbackInfo->bSecDescModified = TRUE;

    appAssert(IsValidSecurityDescriptor(pCallbackInfo->pSecDesc));
    appDebugOut((DEB_TRACE, "SedCallback, return pSecDesc = 0x%08lx\n", pCallbackInfo->pSecDesc));

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   InitializeShareGenericMapping
//
//  Synopsis:   Initializes the passed generic mapping structure for shares.
//
//  Arguments:  [pSHAREGenericMapping] - Pointer to GENERIC_MAPPING to be init.
//
//  History:
//        ChuckC   10-Aug-1992  Created. Culled from NTFS ACL code.
//        BruceFo  4-Apr-95     Stole and used in ntshrui.dll
//
//--------------------------------------------------------------------------

VOID
InitializeShareGenericMapping(
    IN OUT PGENERIC_MAPPING pSHAREGenericMapping
    )
{
    appDebugOut((DEB_ITRACE, "InitializeShareGenericMapping\n"));

    pSHAREGenericMapping->GenericRead    = FILE_GENERIC_READ ;
    pSHAREGenericMapping->GenericWrite   = FILE_GENERIC_WRITE ;
    pSHAREGenericMapping->GenericExecute = FILE_GENERIC_EXECUTE ;
    pSHAREGenericMapping->GenericAll     = FILE_ALL_ACCESS ;
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateDefaultSecDesc
//
//  Synopsis:   Create a default ACL for either a new share or for
//              a share that dont exist.
//
//  Arguments:  [ppSecDesc] - *ppSecDesc points to a "world all" access
//                  security descriptor on exit. Caller is responsible for
//                  freeing it.
//
//  Returns:    NERR_Success if OK, api error otherwise.
//
//  History:
//        ChuckC   10-Aug-1992  Created. Culled from NTFS ACL code.
//        BruceFo  4-Apr-95     Stole and used in ntshrui.dll
//
//--------------------------------------------------------------------------

LONG
CreateDefaultSecDesc(
    OUT PSECURITY_DESCRIPTOR* ppSecDesc
    )
{
    appDebugOut((DEB_ITRACE, "CreateDefaultSecDesc\n"));

    appAssert(NULL != ppSecDesc) ;
    appAssert(NULL == *ppSecDesc) ;

    LONG                    err = NERR_Success;
    PSECURITY_DESCRIPTOR    pSecDesc = NULL;
    PACL                    pAcl = NULL;
    DWORD                   cbAcl;
    PSID                    pSid = NULL;

    *ppSecDesc = NULL;

    do        // error breakout
    {
        // First, create a world SID. Next, create an access allowed
        // ACE with "Generic All" access with the world SID. Put the ACE in
        // the ACL and the ACL in the security descriptor.

        SID_IDENTIFIER_AUTHORITY IDAuthorityWorld = SECURITY_WORLD_SID_AUTHORITY;

        if (!AllocateAndInitializeSid(
                    &IDAuthorityWorld,
                    1,
                    SECURITY_WORLD_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pSid))
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "AllocateAndInitializeSid failed, 0x%08lx\n", err));
            break;
        }

        appAssert(IsValidSid(pSid));

        cbAcl = sizeof(ACL)
              + (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))
              + GetLengthSid(pSid)
              ;

        pAcl = (PACL) new BYTE[cbAcl];
        if (NULL == pAcl)
        {
            err = ERROR_OUTOFMEMORY;
            appDebugOut((DEB_ERROR, "new ACL failed\n"));
            break;
        }

        if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION2))
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "InitializeAcl failed, 0x%08lx\n", err));
            break;
        }

        if (!AddAccessAllowedAce(
                    pAcl,
                    ACL_REVISION2,
                    GENERIC_ALL,
                    pSid))
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "AddAccessAllowedAce failed, 0x%08lx\n", err));
            break;
        }

        appAssert(IsValidAcl(pAcl));

        pSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[SECURITY_DESCRIPTOR_MIN_LENGTH];
        if (NULL == pSecDesc)
        {
            err = ERROR_OUTOFMEMORY;
            appDebugOut((DEB_ERROR, "new SECURITY_DESCRIPTOR failed\n"));
            break;
        }

        if (!InitializeSecurityDescriptor(
                    pSecDesc,
                    SECURITY_DESCRIPTOR_REVISION1))
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "InitializeSecurityDescriptor failed, 0x%08lx\n", err));
            break;
        }

        if (!SetSecurityDescriptorDacl(
                    pSecDesc,
                    TRUE,
                    pAcl,
                    FALSE))
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "SetSecurityDescriptorDacl failed, 0x%08lx\n", err));
            break;
        }

        appAssert(IsValidSecurityDescriptor(pSecDesc));

        // Make the security descriptor self-relative

        DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
        appDebugOut((DEB_ITRACE, "SECURITY_DESCRIPTOR length = %d\n", dwLen));

        PSECURITY_DESCRIPTOR pSelfSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[dwLen];
        if (NULL == pSelfSecDesc)
        {
            err = ERROR_OUTOFMEMORY;
            appDebugOut((DEB_ERROR, "new SECURITY_DESCRIPTOR (2) failed\n"));
            break;
        }

        DWORD cbSelfSecDesc = dwLen;
        if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &cbSelfSecDesc))
        {
            err = GetLastError();
            appDebugOut((DEB_ERROR, "MakeSelfRelativeSD failed, 0x%08lx\n", err));
            break;
        }

        appAssert(IsValidSecurityDescriptor(pSelfSecDesc));

        //
        // all done: set the security descriptor
        //

        *ppSecDesc = pSelfSecDesc;

    } while (FALSE) ;

    if (NULL != pSid)
    {
        FreeSid(pSid);
    }
    delete[] (BYTE*)pAcl;
    delete[] (BYTE*)pSecDesc;

    appAssert(IsValidSecurityDescriptor(*ppSecDesc));

    return err;
}


//+-------------------------------------------------------------------------
//
//  Function:   DeleteDefaultSecDesc
//
//  Synopsis:   Delete a security descriptor that was created by
//              CreateDefaultSecDesc
//
//  Arguments:  [pSecDesc] - security descriptor to delete
//
//  Returns:    nothing
//
//  History:
//        BruceFo  4-Apr-95     Created
//
//--------------------------------------------------------------------------

VOID
DeleteDefaultSecDesc(
    IN PSECURITY_DESCRIPTOR pSecDesc
    )
{
    appDebugOut((DEB_ITRACE, "DeleteDefaultSecDesc\n"));

    delete[] (BYTE*)pSecDesc;
}
