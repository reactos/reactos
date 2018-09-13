//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1995.
//
//  File:       acl.hxx
//
//  Contents:   Declarations for the Shares Acl Editor in the "Sharing"
//              property page.
//
//  History:    5-Apr-95 BruceFo  Stole from net\ui\shellui\share\shareacl.cxx
//
//--------------------------------------------------------------------------

#ifndef __ACL_HXX__
#define __ACL_HXX__

LONG
EditShareAcl(
    IN HWND                      hwndParent,
    IN LPWSTR                    pszServerName,
    IN TCHAR *                   pszShareName,
    IN PSECURITY_DESCRIPTOR      pSecDesc,
    OUT BOOL*                    pfSecDescModified,
    OUT PSECURITY_DESCRIPTOR*    ppSecDesc
    );

//
// Share General Permissions
//

#define FILE_PERM_GEN_NO_ACCESS          (0)
#define FILE_PERM_GEN_READ               (GENERIC_READ    |\
                                          GENERIC_EXECUTE)
#define FILE_PERM_GEN_MODIFY             (GENERIC_READ    |\
                                          GENERIC_EXECUTE |\
                                          GENERIC_WRITE   |\
                                          DELETE )
#define FILE_PERM_GEN_ALL                (GENERIC_ALL)


#endif // __ACL_HXX__
