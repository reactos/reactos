//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       printsi.h
//
//  This file contains the definition of the CPrintSecurity object.
//
//--------------------------------------------------------------------------

#ifndef _PRINTSI_H_
#define _PRINTSI_H_

#include "si.h"

STDMETHODIMP
CheckPrinterAccess(LPCTSTR pszObjectName,
                   LPDWORD pdwAccessGranted,
                   LPTSTR  pszServer,
                   ULONG   cchServer);

DWORD LoadWinSpool();

class CPrintSecurity : public CSecurityInformation
{
public:
    CPrintSecurity(SE_OBJECT_TYPE seType) : CSecurityInformation(seType) {}

    STDMETHOD(Initialize)(HDPA   hItemList,
                          DWORD  dwFlags,
                          LPTSTR pszServer,
                          LPTSTR pszObject);

    // ISecurityInformation methods
    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,     // override
                           PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(GetAccessRights)(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess);
    STDMETHOD(MapGeneric)(const GUID *pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask);
    STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes);

protected:
    // Override these
    STDMETHOD(ReadObjectSecurity)(LPCTSTR pszObject,
                                  SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD);
    STDMETHOD(WriteObjectSecurity)(LPCTSTR pszObject,
                                   SECURITY_INFORMATION si,
                                   PSECURITY_DESCRIPTOR pSD);
};

#endif  /* _PRINTSI_H_ */
