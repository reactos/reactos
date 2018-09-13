//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       si.h
//
//  This file contains the definition of the CSecurityInformation
//  base class.
//
//--------------------------------------------------------------------------

#ifndef _SI_H_
#define _SI_H_

class CSecurityInformation : public ISecurityInformation
{
protected:
    ULONG           m_cRef;
    SE_OBJECT_TYPE  m_seType;
    HDPA            m_hItemList;
    DWORD           m_dwSIFlags;
    LPTSTR          m_pszServerName;
    LPTSTR          m_pszObjectName;
    HWND            m_hwndOwner;

public:
    CSecurityInformation(SE_OBJECT_TYPE seType);
    virtual ~CSecurityInformation();

    STDMETHOD(Initialize)(HDPA   hItemList,
                          DWORD  dwFlags,
                          LPTSTR pszServer,
                          LPTSTR pszObject);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    // ISecurityInformation methods
    STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo);
    STDMETHOD(GetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault);
    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(GetAccessRights)(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess) PURE;
    STDMETHOD(MapGeneric)(const GUID *pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask) PURE;
    STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes) PURE;
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage);

protected:
    STDMETHOD(ReadObjectSecurity)(LPCTSTR pszObject,
                                  SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD);
    STDMETHOD(WriteObjectSecurity)(LPCTSTR pszObject,
                                   SECURITY_INFORMATION si,
                                   PSECURITY_DESCRIPTOR pSD);
};

#endif  /* _SI_H_ */
