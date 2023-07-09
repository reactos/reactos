/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    NTFSAcl.hxx

    This file contains the manifests for the NTFS front to the Generic
    ACL Editor.


    FILE HISTORY:
        Johnl   03-Jan-1992     Created
        beng    06-Apr-1992     Unicode fix

*/

#ifndef _NTFSACL_HXX_
#define _NTFSACL_HXX_

#ifndef RC_INVOKED

LPTSTR CopyString(LPCTSTR psz);
inline void FreeString(LPTSTR psz)
{
    if (!ID(psz))
        LocalFree((HLOCAL)psz);
}

void
InitializeNTFSGenericMapping(PGENERIC_MAPPING pGenericMapping);

APIERR
GetSecurity(LPCTSTR              pszFileName,
            SECURITY_INFORMATION si,
            BUFFER              *pbuffSecDescData,
            BOOL                *pfAuditPrivAdjusted);

APIERR
CheckFileSecurity(LPCTSTR        pszFileName,
                  ACCESS_MASK    DesiredAccess,
                  BOOL          *pfAccessGranted);

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
                                UINT                 cchFailingFile);

DWORD WINAPI
NTFSApplySecurity2(HWND      hwndParent,
                   HINSTANCE hInstance,
                   ULONG     ulCallbackContext,
                   PSECURITY_DESCRIPTOR pSecDesc,
                   PSECURITY_DESCRIPTOR pSecDescNewObjects,
                   BOOLEAN   fApplyToSubContainers,
                   BOOLEAN   fApplyToSubObjects,
                   LPDWORD   pdwStatusReturn,
                   DWORD     dwReason);

UINT CALLBACK
NTFSPSPCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

#include <pshpack4.h>
struct NT_PAGE_CONTEXT
{
    HDROP   hDrop;
    UINT    idsPromptString;
    LPTSTR  pszPromptFile1;
    LPTSTR  pszPromptFile2;

    NT_PAGE_CONTEXT(HDROP h = NULL) :
        hDrop(h),
        idsPromptString(0),
        pszPromptFile1(NULL),
        pszPromptFile2(NULL) {}

    ~NT_PAGE_CONTEXT()
    {
        if (hDrop)
            GlobalFree(hDrop);
        FreeString(pszPromptFile1);
        FreeString(pszPromptFile2);
    }
};
typedef NT_PAGE_CONTEXT *PNT_PAGE_CONTEXT;
#include <poppack.h>

#endif // RC_INVOKED
#endif // _NTFSACL_HXX_
