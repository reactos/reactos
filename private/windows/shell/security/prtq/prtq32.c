/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    prtq32.c

Abstract:

    Print queue administration.

Author:

    Don Ryan (donryan) 14-Jun-1995

Environment:

    User Mode - Win32

Revision History:

    14-Jun-1995 donryan     Munged from windows\printman\security.c.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "prtq32.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HINSTANCE g_hInstance;
LPTSTR    g_pszPrinter = NULL;
TCHAR     g_szHlpFile[] = TEXT("prtman.hlp");

GENERIC_MAPPING g_GenericMappingPrinters =
{                                  
    PRINTER_READ,                  
    PRINTER_WRITE,                 
    PRINTER_EXECUTE,               
    PRINTER_ALL_ACCESS             
};

GENERIC_MAPPING g_GenericMappingDocuments =
{                                  
    JOB_READ,                      
    JOB_WRITE,                     
    JOB_EXECUTE,                   
    JOB_ALL_ACCESS                 
};

SED_HELP_INFO g_PermissionsHelpInfo =
{                                  
    g_szHlpFile,
    ID_HELP_PERMISSIONS_MAIN_DLG,
    0,
    0,
    ID_HELP_PERMISSIONS_ADD_USER_DLG,
    ID_HELP_PERMISSIONS_LOCAL_GROUP,
    ID_HELP_PERMISSIONS_GLOBAL_GROUP,
    ID_HELP_PERMISSIONS_FIND_ACCOUNT
};

SED_HELP_INFO g_AuditingHelpInfo =
{                                  
    g_szHlpFile,
    ID_HELP_AUDITING_MAIN_DLG,
    0,
    0,
    ID_HELP_AUDITING_ADD_USER_DLG,
    ID_HELP_AUDITING_LOCAL_GROUP,
    ID_HELP_AUDITING_GLOBAL_GROUP,
    ID_HELP_AUDITING_FIND_ACCOUNT
};

SED_HELP_INFO g_TakeOwnershipHelpInfo =
{
    g_szHlpFile,
    ID_HELP_TAKE_OWNERSHIP
};

SED_OBJECT_TYPE_DESCRIPTOR g_ObjectTypeDescriptor =
{
    SED_REVISION1,                 // Revision                    
    TRUE,                          // IsContainer                 
    TRUE,                          // AllowNewObjectPerms         
    TRUE,                          // MapSpecificPermsToGeneric   
    &g_GenericMappingPrinters,     // GenericMapping              
    &g_GenericMappingDocuments,    // GenericMappingNewObjects    
    NULL,                          // ObjectTypeName              
    NULL,                          // HelpInfo                    
    NULL,                          // ApplyToSubContainerTitle    
    NULL,                          // ApplyToObjectsTitle         
    NULL,                          // ApplyToSubContainerConfirmation
    NULL,                          // SpecialObjectAccessTitle    
    NULL                           // SpecialNewObjectAccessTitle 
};

// Application accesses passed to the discretionary ACL editor
// as well as the Take Ownership dialog.
SED_APPLICATION_ACCESS g_pDiscretionaryAccessGroup[PERMS_COUNT] =
{
    // No Access:
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  // Type                   
        0,                                  // AccessMask1            
        0,                                  // AccessMask2            
        NULL                                // PermissionTitle        
    },

    // Print permission:
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,
        GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL                      
    },

    // Document Administer permission:
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,
        STANDARD_RIGHTS_READ,      
        GENERIC_ALL,
        NULL                       
    },

    // Administer permission:
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,
        GENERIC_ALL,
        GENERIC_ALL,
        NULL                       
    }
};

// Application accesses passed to the system ACL editor:
SED_APPLICATION_ACCESS g_pSystemAccessGroup[PERMS_AUDIT_COUNT] =
{
    // Print permission:
    {
        SED_DESC_TYPE_AUDIT,
        PRINTER_ACCESS_USE,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        PRINTER_ACCESS_ADMINISTER | ACCESS_SYSTEM_SECURITY,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        DELETE,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        WRITE_DAC,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        WRITE_OWNER,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    }
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
CallDiscretionaryAclEditor(
    HWND   hWnd, 
    PQUEUE pPrinterContext
    );

VOID
CallSystemAclEditor(
    HWND   hWnd, 
    PQUEUE pPrinterContext
    );

VOID
CallTakeOwnershipDialog(
    HWND   hWnd, 
    PQUEUE pPrinterContext
    );

DWORD 
SedCallback2(
    HWND                 hwndParent,
    HANDLE               hInstance,
    ULONG_PTR            CallBackContext,
    PSECURITY_DESCRIPTOR pUpdatedSecurityDescriptor,
    PSECURITY_DESCRIPTOR pSecDescNewObjects,
    BOOLEAN              ApplyToSubContainers,
    BOOLEAN              ApplyToSubObjects,
    LPDWORD              StatusReturn 
    );

BOOL
BuildNewSecurityDescriptor(
    PSECURITY_DESCRIPTOR pNewSecurityDescriptor,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pUpdatedSecurityDescriptor
    );

PSECURITY_DESCRIPTOR
AllocCopySecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    PDWORD               pLength
    );

BOOL 
OpenPrinterForSpecifiedAccess(
    LPTSTR   pName,
    LPHANDLE pHandle,
    DWORD    AccessRequested,
    LPTSTR*  pServerName,
    PDWORD   pAccessGranted 
    );

BOOL 
CheckPrinterAccessForAuditing( 
    LPTSTR pPrinterName, 
    PHANDLE phPrinter 
    );

BOOL
GetGeneric(
    IN  PROC    fnGet,
    IN  DWORD   Level,
    IN  PBYTE   *ppGetData,
    IN  DWORD   cbBuf,
    OUT LPDWORD pcbReturned,
    IN  PVOID   Arg1,
    IN  PVOID   Arg2 
    );

VOID
InitializeStrings(
    );

LPTSTR 
GetStr(
    int id
    );

LPVOID
AllocSplMem(
    DWORD cb
    );

LPVOID
ReallocSplMem(
    LPVOID lpOldMem,
    DWORD cbNew
    );

BOOL
FreeSplMem(
    LPVOID pMem
    );

LPTSTR
AllocSplStr(
    LPTSTR lpStr
    );

BOOL
FreeSplStr(
   LPTSTR lpStr
    );

PQUEUE
AllocQueue(
   LPTSTR pPrinterName
    );

BOOL
FreeQueue(
   PQUEUE pQueue
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// General routines                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

int APIENTRY 
InitializeDLL(
    HINSTANCE hInstance, 
    DWORD     dwReason,
    LPVOID    lpReserved
    )

/*++

Routine Description:

    Dll's entry point.

Arguments:

    Same as DllEntryPoint.

Return Values:

    Same as DllEntryPoint.

--*/

{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance; 

        DisableThreadLibraryCalls(hInstance);
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Exported routines                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

void APIENTRY 
EditQACL2(
    HWND   hWnd,
    LPTSTR pPrinterName,
    UINT   SedId
    )

/*++

Routine Description:

    Edit NT print queue permissions.

Arguments:

    hWnd         - parent window.
    pPrinterName - unc print queue path.
    SedId        - procedure identifier.

Return Values:

    None.

--*/

{
    PQUEUE pQueue;

    if (pQueue = AllocQueue(pPrinterName))
    {
        if (OpenPrinterForSpecifiedAccess(
                pQueue->pPrinterName,
                &pQueue->hPrinter,
                PRINTER_ACCESS_HIGHEST_PERMITTED,
                &pQueue->pServerName,
                &pQueue->AccessGranted
                ))
        {
            InitializeStrings(); 

            switch (SedId)
            {
            case SED_ID_PERMS:        
                CallDiscretionaryAclEditor(hWnd, pQueue);
                break;
            case SED_ID_AUDIT:        
                CallSystemAclEditor(hWnd, pQueue);
                break;
            case SED_ID_OWNER:        
                CallTakeOwnershipDialog(hWnd, pQueue);
                break;
            }

            ClosePrinter(pQueue->hPrinter);
        }                                
        
        FreeQueue(pQueue);
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Dispatch routines                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
CallDiscretionaryAclEditor(
    HWND   hWnd, 
    PQUEUE pPrinterContext
    )

/*++

Routine Description:

    Edit access privileges of print queue.

Arguments:

    hWnd            - parent window.
    pPrinterContext - printer context.

Return Values:

    None.

--*/

{
    SECURITY_CONTEXT           SecurityContext;
    HANDLE                     hPrinterWriteDac;
    BOOL                       CantWriteDacl;
    SED_APPLICATION_ACCESSES   ApplicationAccesses;
    PPRINTER_INFO_3            pPrinterInfo3 = NULL;
    DWORD                      cbPrinterInfo3;
    PSECURITY_DESCRIPTOR       pSecurityDescriptor;
    DWORD                      Status;
    DWORD                      Error;
    DWORD                      i;

    SecurityContext.SecurityInformation = DACL_SECURITY_INFORMATION;
    SecurityContext.pPrinterContext     = pPrinterContext;

    if( GetGeneric( (PROC)GetPrinter, 3, (PBYTE *)&pPrinterInfo3, 0,
            &cbPrinterInfo3, (PVOID)pPrinterContext->hPrinter, NULL ) )
    {
        SecurityContext.hPrinter = pPrinterContext->hPrinter;

        pSecurityDescriptor = pPrinterInfo3->pSecurityDescriptor;
        SecurityContext.pSecurityDescriptor = pSecurityDescriptor;

        /* Pass all the permissions to the ACL editor,
         * and set up the type required:
         */
        ApplicationAccesses.Count = PERMS_COUNT;
        ApplicationAccesses.AccessGroup = g_pDiscretionaryAccessGroup;
        ApplicationAccesses.DefaultPermName =
            g_pDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle;

        for( i = 0; i < PERMS_COUNT; i++ )
            ApplicationAccesses.AccessGroup[i].Type =
                SED_DESC_TYPE_CONT_AND_NEW_OBJECT;

        g_ObjectTypeDescriptor.AllowNewObjectPerms = TRUE;
        g_ObjectTypeDescriptor.HelpInfo = &g_PermissionsHelpInfo;

        Error = SedDiscretionaryAclEditor(
                    hWnd, 
                    g_hInstance, 
                    pPrinterContext->pServerName, 
                    &g_ObjectTypeDescriptor,
                    &ApplicationAccesses, 
                    pPrinterContext->pPrinterName, 
                    SedCallback2,
                    (ULONG_PTR)&SecurityContext, 
                    pSecurityDescriptor, 
                    FALSE,
                    (BOOLEAN)(!(pPrinterContext->AccessGranted & WRITE_DAC)),
                    &Status, 
                    0
                    );

        if( Error != NO_ERROR )
        {
            // CODEWORK...
        }    

        FreeSplMem( pPrinterInfo3 );
    }
    else
    {
        // CODEWORK...
    }
}


VOID
CallSystemAclEditor(
    HWND   hWnd, 
    PQUEUE pPrinterContext
    )

/*++

Routine Description:

    Edit auditing properties of print queue.

Arguments:

    hWnd            - parent window.
    pPrinterContext - printer context.

Return Values:

    None.

--*/

{
    SECURITY_CONTEXT           SecurityContext;
    HANDLE                     hPrinterSystemAccess;
    SED_APPLICATION_ACCESSES   ApplicationAccesses;
    PPRINTER_INFO_3            pPrinterInfo3 = NULL;
    DWORD                      cbPrinterInfo3;
    PSECURITY_DESCRIPTOR       pSecurityDescriptor;
    DWORD                      Status;
    DWORD                      Error;

    if( !CheckPrinterAccessForAuditing( pPrinterContext->pPrinterName,
                                        &hPrinterSystemAccess ) )
    {
        //
        // CODEWORK...
        //
        return;
    }

    SecurityContext.SecurityInformation = SACL_SECURITY_INFORMATION;
    SecurityContext.pPrinterContext     = pPrinterContext;
    SecurityContext.hPrinter            = hPrinterSystemAccess;

    if( GetGeneric( (PROC)GetPrinter, 3, (PBYTE *)&pPrinterInfo3, 0,
                     &cbPrinterInfo3, (PVOID)hPrinterSystemAccess, NULL ) )
    {
        pSecurityDescriptor = pPrinterInfo3->pSecurityDescriptor;
        SecurityContext.pSecurityDescriptor = pSecurityDescriptor;

        /* Pass only the Print and Administer permissions to the ACL editor,
         * and set up the type required:
         */
        ApplicationAccesses.Count = PERMS_AUDIT_COUNT;
        ApplicationAccesses.AccessGroup = g_pSystemAccessGroup;
        ApplicationAccesses.DefaultPermName =
            g_pDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle;

        g_ObjectTypeDescriptor.AllowNewObjectPerms = FALSE;
        g_ObjectTypeDescriptor.HelpInfo = &g_AuditingHelpInfo;

        Error = SedSystemAclEditor(
                    hWnd, 
                    g_hInstance, 
                    pPrinterContext->pServerName, 
                    &g_ObjectTypeDescriptor,
                    &ApplicationAccesses, 
                    pPrinterContext->pPrinterName, 
                    SedCallback2,
                    (ULONG_PTR)&SecurityContext, 
                    pSecurityDescriptor, 
                    FALSE,
                    &Status, 
                    0
                    );

        if( Error != NO_ERROR )
        {
            //
            // CODEWORK...
            //
        }

        FreeSplMem( pPrinterInfo3 );
    }
    else
    {
        //
        // CODEWORK...
        //
    }

    ClosePrinter( hPrinterSystemAccess );
}


VOID
CallTakeOwnershipDialog(
    HWND   hWnd, 
    PQUEUE pPrinterContext
    )

/*++

Routine Description:

    Edit ownership of print queue.

Arguments:

    hWnd   - parent window.
    pPrinterContext - printer context.

Return Values:

    None.

--*/

{
    SECURITY_CONTEXT           SecurityContext;
    HANDLE                     hPrinterWriteOwner;
    BOOL                       CantWriteOwner;
    BOOL                       CantReadOwner;
    SED_APPLICATION_ACCESSES   ApplicationAccesses;
    PPRINTER_INFO_3            pPrinterInfo3 = NULL;
    DWORD                      cbPrinterInfo3;
    PSECURITY_DESCRIPTOR        pSecurityDescriptor;
    DWORD                      Status;
    DWORD                      Error;
    DWORD                      i;

    SecurityContext.SecurityInformation = OWNER_SECURITY_INFORMATION;
    SecurityContext.pPrinterContext     = pPrinterContext;
    
    if( GetGeneric( (PROC)GetPrinter, 3, (PBYTE *)&pPrinterInfo3, 0,
                     &cbPrinterInfo3, (PVOID)pPrinterContext->hPrinter, NULL ) )
    {
        SecurityContext.hPrinter = pPrinterContext->hPrinter;

        pSecurityDescriptor = pPrinterInfo3->pSecurityDescriptor;
        SecurityContext.pSecurityDescriptor = pSecurityDescriptor;

        ApplicationAccesses.Count = PERMS_COUNT;
        ApplicationAccesses.AccessGroup = g_pDiscretionaryAccessGroup;
        ApplicationAccesses.DefaultPermName =
            g_pDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle;

        for( i = 0; i < PERMS_COUNT; i++ )
            ApplicationAccesses.AccessGroup[i].Type =
                SED_DESC_TYPE_AUDIT;

        Error = SedTakeOwnership(
                    hWnd, 
                    g_hInstance, 
                    pPrinterContext->pServerName, 
                    g_pszPrinter,
                    pPrinterContext->pPrinterName, 
                    1, 
                    SedCallback2,
                    (ULONG_PTR)&SecurityContext, 
                    pSecurityDescriptor,
                    FALSE,
                    (BOOLEAN)(!(pPrinterContext->AccessGranted & WRITE_OWNER)),
                    &Status,
                    &g_TakeOwnershipHelpInfo, 
                    0 
                    );

        if( Error != NO_ERROR )
        {
            //
            // CODEWORK...
            //
        }

        FreeSplMem( pPrinterInfo3 );
    }
    else
    {
        //
        // CODEWORK...
        //
    }    
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Security callback routine                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD 
SedCallback2(
    HWND                 hwndParent,
    HANDLE               hInstance,
    ULONG_PTR            CallBackContext,
    PSECURITY_DESCRIPTOR pUpdatedSecurityDescriptor,
    PSECURITY_DESCRIPTOR pSecDescNewObjects,
    BOOLEAN              ApplyToSubContainers,
    BOOLEAN              ApplyToSubObjects,
    LPDWORD              StatusReturn 
    )

/*++

Routine Description:

    Called by acledit to process writes.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    PSECURITY_CONTEXT    pSecurityContext;
    SECURITY_DESCRIPTOR  NewSecurityDescriptor;
    PRINTER_INFO_3       PrinterInfo3;
    PSECURITY_DESCRIPTOR pSelfRelativeSD = NULL;
    DWORD                cbSelfRelativeSD;
    BOOL                 OK = FALSE;

    pSecurityContext = (PSECURITY_CONTEXT)CallBackContext;

    if( InitializeSecurityDescriptor( &NewSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION1 )
     && BuildNewSecurityDescriptor( &NewSecurityDescriptor,
                                    pSecurityContext->SecurityInformation,
                                    pUpdatedSecurityDescriptor ) )

        pSelfRelativeSD = AllocCopySecurityDescriptor( &NewSecurityDescriptor,
                                                       &cbSelfRelativeSD );

    if( pSelfRelativeSD )
    {
        PrinterInfo3.pSecurityDescriptor = pSelfRelativeSD;

        OK = SetPrinter( pSecurityContext->hPrinter, 3, (PBYTE)&PrinterInfo3, 0 );

        FreeSplMem( pSelfRelativeSD );
    }

    return ( OK ? 0 : 1 );

    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(CallBackContext);
    UNREFERENCED_PARAMETER(pSecDescNewObjects);
    UNREFERENCED_PARAMETER(ApplyToSubContainers);
    UNREFERENCED_PARAMETER(ApplyToSubObjects);
    UNREFERENCED_PARAMETER(StatusReturn);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Support routines                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
BuildNewSecurityDescriptor(
    PSECURITY_DESCRIPTOR pNewSecurityDescriptor,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pUpdatedSecurityDescriptor
    )

/*++

Routine Description:

    Builds new security desriptor.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    BOOL Defaulted = FALSE;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    BOOL DaclPresent = FALSE;
    PACL pDacl = NULL;
    BOOL SaclPresent = FALSE;
    PACL pSacl = NULL;
    BOOL OK = TRUE;

    if( ( SecurityInformation == OWNER_SECURITY_INFORMATION )
     && GetSecurityDescriptorOwner( pUpdatedSecurityDescriptor,
                                    &pOwnerSid, &Defaulted ) )
    {
        OK = SetSecurityDescriptorOwner( pNewSecurityDescriptor,
                                         pOwnerSid, Defaulted );
    }

    if( ( SecurityInformation == DACL_SECURITY_INFORMATION )
     && GetSecurityDescriptorDacl( pUpdatedSecurityDescriptor,
                                   &DaclPresent, &pDacl, &Defaulted ) )
    {
        OK = SetSecurityDescriptorDacl( pNewSecurityDescriptor,
                                        DaclPresent, pDacl, Defaulted );
    }

    if( ( SecurityInformation == SACL_SECURITY_INFORMATION )
     && GetSecurityDescriptorSacl( pUpdatedSecurityDescriptor,
                                   &SaclPresent, &pSacl, &Defaulted ) )
    {
        OK = SetSecurityDescriptorSacl( pNewSecurityDescriptor,
                                        SaclPresent, pSacl, Defaulted );
    }

    return OK;
}


PSECURITY_DESCRIPTOR
AllocCopySecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    PDWORD               pLength
    )

/*++

Routine Description:

    Alloc copy of security descriptor.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    PSECURITY_DESCRIPTOR pSecurityDescriptorCopy;
    DWORD                Length;

    Length = GetSecurityDescriptorLength(pSecurityDescriptor);

    if(pSecurityDescriptorCopy = AllocSplMem(Length))
    {
        MakeSelfRelativeSD(pSecurityDescriptor,
                           pSecurityDescriptorCopy,
                           &Length);

        *pLength = Length;
    }

    return pSecurityDescriptorCopy;
}


BOOL 
OpenPrinterForSpecifiedAccess(
    LPTSTR   pName,
    LPHANDLE pHandle,
    DWORD    AccessRequested,
    LPTSTR*  pServerName,
    PDWORD   pAccessGranted 
    )

/*++

Routine Description:

    Obtains printer handle w/access.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    PRINTER_DEFAULTS PrinterDefaults;
    BOOL             rc = FALSE;
    BOOL             TryAll = FALSE;

    PrinterDefaults.pDatatype = NULL;
    PrinterDefaults.pDevMode  = NULL;

    switch( AccessRequested )
    {
    case PRINTER_ACCESS_HIGHEST_PERMITTED:
        TryAll = TRUE;
        /* fall through ... */

    case PRINTER_ALL_ACCESS:
        PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;
        rc = OpenPrinter( pName, pHandle, &PrinterDefaults );
        if( rc || !TryAll || (( GetLastError( ) != ERROR_ACCESS_DENIED ) &&
                              ( GetLastError( ) != ERROR_PRIVILEGE_NOT_HELD)) )
            break;

    case PRINTER_READ:
        PrinterDefaults.DesiredAccess = PRINTER_READ;
        rc = OpenPrinter( pName, pHandle, &PrinterDefaults );
        if( rc || !TryAll || (( GetLastError( ) != ERROR_ACCESS_DENIED ) &&
                              ( GetLastError( ) != ERROR_PRIVILEGE_NOT_HELD)) )
            break;

    case READ_CONTROL:
        PrinterDefaults.DesiredAccess = READ_CONTROL;
        rc = OpenPrinter( pName, pHandle, &PrinterDefaults );
    }

    if( pAccessGranted )
    {
        if( rc )
            *pAccessGranted = PrinterDefaults.DesiredAccess;
        else
            *pAccessGranted = PRINTER_ACCESS_DENIED;
    }

    if (pServerName)
    {
        if( rc )
        {
            TCHAR szServerName[MAX_PATH];
            LPTSTR pSlash;

            lstrcpy(szServerName, pName);

            pSlash = StrChr( &szServerName[2], TEXT('\\'));
            if (pSlash)
                *pSlash = TEXT('\0');       // Terminate server at \\foo

            *pServerName = AllocSplStr(szServerName);
        }
        else
            *pServerName = NULL;
    }

    return rc;
}


BOOL 
CheckPrinterAccessForAuditing( 
    LPTSTR pPrinterName, 
    PHANDLE phPrinter 
    )

/*++

Routine Description:

    Grants system access if possible.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    PRINTER_DEFAULTS PrinterDefaults;
    BOOL             rc = FALSE;

    PrinterDefaults.pDatatype = NULL;
    PrinterDefaults.pDevMode  = NULL;

    PrinterDefaults.DesiredAccess = ACCESS_SYSTEM_SECURITY;

    rc = OpenPrinter( pPrinterName, phPrinter, &PrinterDefaults );

    return rc;
}


#define GET_ARGS Level, (LPBYTE)*ppGetData, cbBuf, pcbReturned

BOOL
GetGeneric(
    IN  PROC    fnGet,
    IN  DWORD   Level,
    IN  PBYTE   *ppGetData,
    IN  DWORD   cbBuf,
    OUT LPDWORD pcbReturned,
    IN  PVOID   Arg1,
    IN  PVOID   Arg2 
    )

/*++

Routine Description:

    Wrapper around GetInfo calls.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    BOOL   rc = FALSE;
    BOOL   UnknownFunction = FALSE;
    DWORD  cbRealloc;

    if( fnGet == (PROC)GetPrinter )
        rc = GetPrinter( (HANDLE)Arg1, GET_ARGS );
                         // hPrinter
    else
    {
        *ppGetData = NULL;
        UnknownFunction = TRUE;
    }

    if( ( rc == FALSE ) && ( UnknownFunction == FALSE ) )
    {
        if( GetLastError( ) == ERROR_INSUFFICIENT_BUFFER )
        {
            cbRealloc = *pcbReturned;
            *ppGetData = (PBYTE)ReallocSplMem( *ppGetData, cbRealloc );
            cbBuf = cbRealloc;

            if( *ppGetData )
            {
                if( fnGet == (PROC)GetPrinter )
                    rc = GetPrinter( (HANDLE)Arg1, GET_ARGS );
                                     // hPrinter

                /* If things haven't worked out, free up the buffer.
                 * We do this because otherwise the caller will not know
                 * whether the pointer is valid any more,
                 * since ReallocSplMem might have failed.
                 */
                if( rc == FALSE )
                {
                    if( *ppGetData )
                        FreeSplMem( *ppGetData );
                    *ppGetData = NULL;
                    *pcbReturned = 0;
                }
            }
        }

        else
        {
            if( *ppGetData )
                FreeSplMem( *ppGetData );
            *ppGetData = NULL;
            *pcbReturned = 0;
            rc = FALSE;
        }
    }

    else
        *pcbReturned = cbBuf;

    return rc;
}


VOID
InitializeStrings(
    )

/*++

Routine Description:

    Wrapper around LocalAlloc.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    if( !g_pszPrinter )
    {
        g_pszPrinter = GetStr( IDS_PRINTER );
        g_ObjectTypeDescriptor.ObjectTypeName = g_pszPrinter;

        g_pDiscretionaryAccessGroup[PERMS_NOACC].PermissionTitle =
            GetStr( IDS_NOACCESS );

        g_pDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle =
            GetStr( IDS_PRINT );

        g_pDiscretionaryAccessGroup[PERMS_DOCAD].PermissionTitle =
            GetStr( IDS_ADMINISTERDOCUMENTS );

        g_pDiscretionaryAccessGroup[PERMS_ADMIN].PermissionTitle =
            GetStr( IDS_ADMINISTER );

        g_pSystemAccessGroup[PERMS_AUDIT_PRINT].PermissionTitle =
            GetStr( IDS_AUDIT_PRINT );

        g_pSystemAccessGroup[PERMS_AUDIT_ADMINISTER].PermissionTitle =
            GetStr( IDS_AUDIT_ADMINISTER );

        g_pSystemAccessGroup[PERMS_AUDIT_DELETE].PermissionTitle =
            GetStr( IDS_AUDIT_DELETE );

        g_pSystemAccessGroup[PERMS_AUDIT_CHANGE_PERMISSIONS].PermissionTitle =
            GetStr( IDS_CHANGE_PERMISSIONS );

        g_pSystemAccessGroup[PERMS_AUDIT_TAKE_OWNERSHIP].PermissionTitle =
            GetStr( IDS_TAKE_OWNERSHIP );
    }
}


LPTSTR 
GetStr(
    int id
    )

/*++

Routine Description:

    Load resource string.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    TCHAR ResString[MAX_PATH];
    DWORD length = 0;
    LPTSTR pStr;
    DWORD  cbStr;

    length = LoadString(g_hInstance, id, ResString, sizeof(ResString)/sizeof(TCHAR));

    cbStr = ( length * sizeof ( TCHAR ) + sizeof ( TCHAR ) );
    pStr = (LPTSTR)AllocSplMem( cbStr );

    if( pStr )
        memcpy( pStr, ResString, cbStr );

    return pStr;
}


LPVOID
AllocSplMem(
    DWORD cb
    )

/*++

Routine Description:

    Wrapper around LocalAlloc.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    LPDWORD pMem;

    if (!cb)
        return(NULL);

    pMem=(LPDWORD)LocalAlloc(LPTR, cb);

    if (!pMem)
        return NULL;

    return (LPVOID)pMem;
}


LPVOID
ReallocSplMem(
    LPVOID lpOldMem,
    DWORD cbNew
    )

/*++

Routine Description:

    Wrapper around LocalReAlloc.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    if (lpOldMem)
        return LocalReAlloc(lpOldMem, cbNew, LMEM_MOVEABLE);
    else
        return AllocSplMem(cbNew);
}


BOOL
FreeSplMem(
    LPVOID pMem
    )

/*++

Routine Description:

    Wrapper around LocalFree.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
    return LocalFree((HLOCAL)pMem) == NULL;
}


LPTSTR
AllocSplStr(
    LPTSTR lpStr
    )

/*++

Routine Description:

    Alloc copy of string.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
   LPTSTR lpMem;

   if (!lpStr)
      return 0;

   if (lpMem = (LPTSTR)AllocSplMem( (lstrlen(lpStr) + 1 )*sizeof(TCHAR)))
      lstrcpy(lpMem, lpStr);

   return lpMem;
}


BOOL
FreeSplStr(
   LPTSTR lpStr
    )

/*++

Routine Description:

    Free copy of string.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
   return lpStr ? FreeSplMem(lpStr) : FALSE;
}


PQUEUE
AllocQueue(
   LPTSTR pPrinterName
    )

/*++

Routine Description:

    Alloc queue context.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
   PQUEUE pQueue;

   if( ( pQueue = (PQUEUE)AllocSplMem( sizeof( QUEUE ) ) )
     &&( pQueue->pPrinterName = AllocSplStr( pPrinterName ) ) )
   {
       pQueue->pServerName = NULL;
   }
   else
   if( pQueue )
   {
       FreeSplMem( pQueue );
       pQueue = NULL;
   }
   return pQueue;
}


BOOL
FreeQueue(
   PQUEUE pQueue
    )

/*++

Routine Description:

    Free queue context.

Arguments:
    
    <insert>.

Return Values:

    <insert>.

--*/

{
   if(pQueue->pServerName)
       FreeSplStr(pQueue->pServerName);
   FreeSplStr(pQueue->pPrinterName);
   FreeSplMem(pQueue);

   return TRUE;
}
