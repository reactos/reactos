/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

    NTFSAcl.cxx

    This file contains the implementation for the NT File System ACL
    Editor.  It is just a front end for the Generic ACL Editor that is
    specific to the NTFS.

    FILE HISTORY:
        Johnl   30-Dec-1991     Created

*/

#include "rshx32.h"
#include <msgpopup.h>
#include <getpriv.h>
#include <dbg.h>

#define INCL_NETCONS
#include <lmui.hxx>

#define INCL_BLT_CONTROL
#include <blt.hxx>
#include <security.hxx>     // OS_SECURITY_DESCRIPTOR, OS_ACL, etc.
#include <errmap.hxx>       // MapNTStatus

#include "ntfsacl.h"


UINT CALLBACK
NTFSPSPCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    ASSERT(ppsp != NULL && ppsp->dwSize >= sizeof(PROPSHEETPAGE));

    PNT_PAGE_CONTEXT pContext = (PNT_PAGE_CONTEXT)ppsp->lParam;
    ASSERT(pContext != NULL && !IsBadReadPtr(pContext, sizeof(*pContext)));

    switch (uMsg)
    {
    case PSPCB_CREATE:
        if (pContext->idsPromptString)
        {
            if (IDYES != MsgPopup(hwnd,
                                  pContext->idsPromptString,
                                  pContext->pszPromptFile1,
                                  pContext->pszPromptFile2,
                                  g_hInstance,
                                  MB_YESNO | MB_ICONWARNING))
                return 0;   // abort
        }
        break;

    case PSPCB_RELEASE:
        delete pContext;
        break;
    }

    return 1;
}


/*******************************************************************

    NAME:       SedCallback

    SYNOPSIS:   Security Editor callback for the NTFS ACL Editor

    ENTRY:      See sedapi.h

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   17-Mar-1992     Filled out
        jeffreys 3-May-1996     Changed from FMX to CF_HDROP

********************************************************************/

DWORD WINAPI
NTFSApplySecurity2(HWND      hwndParent,
                   HINSTANCE /*hInstance*/,
                   ULONG     ulCallbackContext,
                   PSECURITY_DESCRIPTOR psecdesc,
                   PSECURITY_DESCRIPTOR /*psecdescNewObjects*/,
                   BOOLEAN   /*fApplyToSubContainers*/,
                   BOOLEAN   /*fApplyToSubObjects*/,
                   LPDWORD   pdwStatusReturn,
                   DWORD     dwReason)
{
    APIERR err = NO_ERROR;
    PNT_PAGE_CONTEXT pContext = (PNT_PAGE_CONTEXT)ulCallbackContext;
    ASSERT(pContext != NULL);
    HDROP hDrop = pContext->hDrop;
    ASSERT(hDrop != NULL);

    SECURITY_INFORMATION si = 0;
    ULONG ulPrivilege = 0;
    BOOL fPrivAdjusted = FALSE;

    switch (dwReason)
    {
    case SED_ACCESSES:
        si |= DACL_SECURITY_INFORMATION;
        break;

    case SED_AUDITS:
        si |= SACL_SECURITY_INFORMATION;
        ulPrivilege = SE_SECURITY_PRIVILEGE;
        break;

    case SED_OWNER:
        si |= OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION;
        ulPrivilege = SE_TAKE_OWNERSHIP_PRIVILEGE;
        break;

    default:
        ASSERT(FALSE);
        *pdwStatusReturn = SED_STATUS_FAILED_TO_MODIFY;
        return ERROR_GEN_FAILURE;
    }

    //  BUGBUG put up hourglass cursor here

    if (ulPrivilege && EnablePrivilege(ulPrivilege))
        fPrivAdjusted = TRUE;

    UINT uiCount = (*g_pfnSHDragQueryFile)(hDrop, (UINT)-1, NULL, 0);

    //
    //  Apply the new permissions
    //

    TCHAR szSelItem[MAX_PATH];
    for (UINT i = 0; i < uiCount; i++)
    {
        if (0 == (*g_pfnSHDragQueryFile)(hDrop, 0, szSelItem, ARRAYSIZE(szSelItem)))
        {
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        if (!SetFileSecurity(szSelItem, si, psecdesc))
        {
            err = GetLastError();

            ODS2(TEXT("NTFS SedCallback - Error %lu applying security to %s\r\n"),
                 err, szSelItem);
            break;
        }
    }

    if (err)
    {
        *pdwStatusReturn = (i > 0 ? SED_STATUS_NOT_ALL_MODIFIED : SED_STATUS_FAILED_TO_MODIFY);
        ::MsgPopup(hwndParent, err);     // BUGBUG: this is too cryptic
    }
    else
        *pdwStatusReturn = SED_STATUS_MODIFIED;

    if (fPrivAdjusted)
        ReleasePrivileges();

    //  BUGBUG restore cursor here

    return err;
}


/*******************************************************************

    NAME:       CompareNTFSSecurityIntersection

    SYNOPSIS:   Determines if the files/dirs currently selected have
                equivalent security descriptors

    ENTRY:      hDrop - CF_HDROP handle used for getting selection
                psecdesc - Baseline security descriptor to compare against
                pfOwnerEqual - Set to TRUE if all the owners are equal
                pfACLEqual, etc. similar to pfOwnerEqual

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The first non-equal ACL causes the function to exit.

                On a 20e with 499 files selected locally, it took 35.2 minutes
                to read the security descriptors from the disk and 14 seconds
                to determine the intersection.  So even though the Compare
                method uses an n^2 algorithm, it only takes up 0.6% of the
                wait time.

    HISTORY:
        Johnl   05-Nov-1992      Created
        JeffreyS 11-Jun-1996     Changed from FMX to HDROP format
                                 and made pfOwnerEqual reliable even
                                 when pfAclEqual is set to FALSE

********************************************************************/

APIERR
CompareNTFSSecurityIntersection(HDROP                hDrop,
                                PSECURITY_DESCRIPTOR psecdesc,
                                BOOL                *pfOwnerEqual,
                                BOOL                *pfGroupEqual,
                                BOOL                *pfDACLEqual,
                                BOOL                *pfSACLEqual,
                                PGENERIC_MAPPING     pGenericMapping,
                                PGENERIC_MAPPING     pGenericMappingObjects,
                                BOOL                 fMapGenAll,
                                BOOL                 fIsContainer,
                                LPTSTR               pszFailingFile,
                                UINT                 cchFailingFile)
{
    ODS1(TEXT("::CompareNTFSSecurityIntersection - Entered @ %d\r\n"),
         ::GetTickCount()/100);

    APIERR err;
    OS_SECURITY_DESCRIPTOR ossecdesc1(psecdesc);
    UINT cSel = (*g_pfnSHDragQueryFile)(hDrop, (UINT)-1, NULL, 0);
    ASSERT(cSel > 1);

    BUFFER  buffSecDescData(1024);
    if (err = buffSecDescData.QueryError())
    {
        return err;
    }

    SECURITY_INFORMATION si = 0;

    if (pfOwnerEqual)
    {
        *pfOwnerEqual = TRUE;
        si |= OWNER_SECURITY_INFORMATION;
    }
    if (pfGroupEqual)
    {
        *pfGroupEqual = TRUE;
        si |= GROUP_SECURITY_INFORMATION;
    }
    if (pfDACLEqual)
    {
        *pfDACLEqual = TRUE;
        si |= DACL_SECURITY_INFORMATION;
    }
    if (pfSACLEqual)
    {
        *pfSACLEqual = TRUE;
        si |= SACL_SECURITY_INFORMATION;
    }

    TCHAR szSel[MAX_PATH];

    for (UINT i = 1 ; i < cSel && si != 0 ; i++)
    {
        if (0 == (*g_pfnSHDragQueryFile)(hDrop, i, szSel, ARRAYSIZE(szSel)))
            err = ERROR_INVALID_PARAMETER;
        if (err ||
            (err = ::GetSecurity(szSel,
                                 si,
                                 &buffSecDescData,
                                 NULL)))
        {
            break;
        }

        PSECURITY_DESCRIPTOR psecdesc2 = (PSECURITY_DESCRIPTOR)
                                                  buffSecDescData.QueryPtr();

        OS_SECURITY_DESCRIPTOR ossecdesc2(psecdesc2);
        if ((err = ossecdesc2.QueryError()) ||
            (err = ossecdesc1.Compare(&ossecdesc2,
                                      pfOwnerEqual,
                                      pfGroupEqual,
                                      pfDACLEqual,
                                      pfSACLEqual,
                                      pGenericMapping,
                                      pGenericMappingObjects,
                                      fMapGenAll,
                                      fIsContainer)))

        {
            break;
        }

        // If we find an owner that doesn't match, we can stop checking owners
        if (pfOwnerEqual && !*pfOwnerEqual)
        {
            pfOwnerEqual = NULL;
            si &= ~OWNER_SECURITY_INFORMATION;
        }

        // Ditto for the group
        if (pfGroupEqual && !*pfGroupEqual)
        {
            pfGroupEqual = NULL;
            si &= ~GROUP_SECURITY_INFORMATION;
        }

        // Same for DACLs
        if (pfDACLEqual && !*pfDACLEqual)
        {
            pfDACLEqual = NULL;
            si &= ~DACL_SECURITY_INFORMATION;

            if (pszFailingFile)
                lstrcpyn(pszFailingFile, szSel, cchFailingFile);
        }

        // And for SACLs
        if (pfSACLEqual && !*pfSACLEqual)
        {
            pfSACLEqual = NULL;
            si &= ~SACL_SECURITY_INFORMATION;

            if (pszFailingFile)
                lstrcpyn(pszFailingFile, szSel, cchFailingFile);
            // Note that if both pfDACLEqual and pfSACLEqual start
            // out as non-NULL, then the LAST one to become NULL
            // will set pszFailingFile.  These 2 should rarely
            // (if ever) both be non-NULL, and even if this
            // happens it doesn't really matter which file name
            // is reported as the failing file (either would be
            // correct in that situation).
        }
    }

    ODS1(TEXT("::CompareNTFSSecurityIntersection - Left    @ %d\r\n"),
         ::GetTickCount()/100);
    return err;
}


/*******************************************************************

    NAME:       ::GetSecurity

    SYNOPSIS:   Retrieves a security descriptor from an NTFS file/directory

    ENTRY:      pszFileName - Name of file/dir to get security desc. for
                pbuffSecDescData - Buffer to store the data into
                sedpermtype - Are we getting audit/access info
                pfAuditPrivAdjusted - Set to TRUE if audit priv was enabled.
                    Set this to NULL if the privilege has already been adjusted

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   05-Nov-1992     Broke out
        JeffreyS 11-Jun-1996    Changed to take SECURITY_INFORMATION
                                parameter instead of SED_PERM_TYPE

********************************************************************/

APIERR GetSecurity(LPCTSTR              pszFileName,
                   SECURITY_INFORMATION si,
                   BUFFER              *pbuffSecDescData,
                   BOOL                *pfAuditPrivAdjusted)
{
    ASSERT(pszFileName);
    ASSERT(pbuffSecDescData);
    APIERR err;

    if (pfAuditPrivAdjusted)
        *pfAuditPrivAdjusted = FALSE;

    if (si == 0)
    {
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if (err = pbuffSecDescData->QueryError())
    {
        return err;
    }

    if (si & SACL_SECURITY_INFORMATION)
    {
        /* We will need to enable the SeAuditPrivilege to read/write the
         * SACL for NT.
         */
        if (!EnablePrivilege(SE_SECURITY_PRIVILEGE))
            return GetLastError();

        if (pfAuditPrivAdjusted)
            *pfAuditPrivAdjusted = TRUE;
    }

    //
    //  Try once with the given buffer, if it doesn't fit then we will try
    //  again with the known required size which should succeed unless
    //  another error occurs, in which case we bail.
    //
    PSECURITY_DESCRIPTOR pSecurityDesc = NULL;
    DWORD dwLengthNeeded;
    if (!::GetFileSecurity(pszFileName,
                           si,
                           (PSECURITY_DESCRIPTOR)pbuffSecDescData->QueryPtr(),
                           pbuffSecDescData->QuerySize(),
                           &dwLengthNeeded))
    {
        err = ::GetLastError();
        if(err == ERROR_INSUFFICIENT_BUFFER)
        {
            err = pbuffSecDescData->Resize((UINT)dwLengthNeeded);

            /* If this guy fails then we bail
             */
            if (!err &&
                !::GetFileSecurity(pszFileName,
                                   si,
                                   (PSECURITY_DESCRIPTOR)pbuffSecDescData->QueryPtr(),
                                   pbuffSecDescData->QuerySize(),
                                   &dwLengthNeeded))
            {
                err = ::GetLastError();
            }
        }
    }

    // Release the SeAuditPrivilege only if the caller didn't
    // provide pfAuditPrivAdjusted
    if ((si & SACL_SECURITY_INFORMATION) && !pfAuditPrivAdjusted)
        ReleasePrivileges();

    return err;
}

/*******************************************************************

    NAME:       InitializeNTFSGenericMapping

    SYNOPSIS:   Initializes the passed generic mapping structure
                appropriately depending on whether this is a file
                or a directory.

    ENTRY:      pNTFSGenericMapping - Pointer to GENERIC_MAPPING to be init.
                fIsDirectory - TRUE if directory, FALSE if file

    EXIT:

    RETURNS:

    NOTES:      Note that Delete Child was removed from Generic Write.

    HISTORY:
        Johnl   27-Feb-1992     Created

********************************************************************/

void InitializeNTFSGenericMapping(PGENERIC_MAPPING pNTFSGenericMapping)
{
    pNTFSGenericMapping->GenericRead    = FILE_GENERIC_READ;
    pNTFSGenericMapping->GenericWrite   = FILE_GENERIC_WRITE;
    pNTFSGenericMapping->GenericExecute = FILE_GENERIC_EXECUTE;
    pNTFSGenericMapping->GenericAll     = FILE_ALL_ACCESS;
}


/*******************************************************************

    NAME:       CheckFileSecurity

    SYNOPSIS:   Checks to see if the current user has access to the file or
                directory

    ENTRY:      pszFileName - File or directory name
                DesiredAccess - Access to check for
                pfAccessGranted - Set to TRUE if access was granted

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If the check requires enabled privileges, they must be enabled
                before this call.

    HISTORY:
        Johnl   15-Jan-1993     Created

********************************************************************/

APIERR CheckFileSecurity(LPCTSTR        pszFileName,
                         ACCESS_MASK    DesiredAccess,
                         BOOL          *pfAccessGranted)
{
    APIERR err = NERR_Success;
    *pfAccessGranted = TRUE;

    do { // error breakout

        //
        // Convert the DOS device name ("X:\") to an NT device name
        // (looks something like "\dosdevices\X:\")
        //
        int cbFileName = lstrlen(pszFileName) * sizeof(TCHAR);
        UNICODE_STRING  UniStrNtFileName;
        BUFFER          buffNtFileName(cbFileName + 80);

        if (err = buffNtFileName.QueryError())
        {
            break;
        }

        UniStrNtFileName.Buffer = (PWSTR)buffNtFileName.QueryPtr();
        UniStrNtFileName.Length = 0;
        UniStrNtFileName.MaximumLength = buffNtFileName.QuerySize();

        if (!RtlDosPathNameToNtPathName_U(pszFileName,
                                          &UniStrNtFileName,
                                          NULL,
                                          NULL))
        {
            ASSERT(FALSE);
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        OBJECT_ATTRIBUTES oa;
        IO_STATUS_BLOCK StatusBlock;
        InitializeObjectAttributes(&oa,
                                   &UniStrNtFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   0,
                                   0);

        //
        //  Check to see if we have permission/privilege to read the security
        //
        HANDLE hFile;
        if (err = ERRMAP::MapNTStatus(::NtOpenFile(&hFile,
                                                   DesiredAccess,
                                                   &oa,
                                                   &StatusBlock,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                   0)))
        {
            ODS2(TEXT("CheckFileSecurity - check failed with error %lu on desired access 0x%08x\r\n"),
                 err,
                 DesiredAccess);

            if (err == ERROR_ACCESS_DENIED)
            {
                *pfAccessGranted = FALSE;
                err = NERR_Success;
            }
        }
        else
            ::NtClose(hFile);


    } while (FALSE);

    return err;
}


LPTSTR CopyString(LPCTSTR psz)
{
    LPTSTR pszNew;

    if (ID(psz))
        pszNew = (LPTSTR)psz;
    else
    {
        pszNew = (LPTSTR)LocalAlloc(LPTR, (lstrlen(psz) + 1) * sizeof(TCHAR));

        if (pszNew)
            lstrcpy(pszNew, psz);
    }

    return pszNew;
}
