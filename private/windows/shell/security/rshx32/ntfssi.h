//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ntfssi.h
//
//  This file contains the definition of the CNTFSSecurity object.
//
//--------------------------------------------------------------------------

#ifndef _NTFSSI_H_
#define _NTFSSI_H_

#include "si.h"

STDMETHODIMP
CheckFileAccess(LPCTSTR pszObjectName, LPDWORD pdwAccessGranted);

struct NTFS_COMPARE_DATA
{
    HDPA hItemList;
    DWORD dwSIFlags;
    SECURITY_INFORMATION siConflict;
    UINT idsSaclPrompt;
    UINT idsDaclPrompt;
    LPTSTR pszSaclConflict;
    LPTSTR pszDaclConflict;
    HRESULT hrResult;
    BOOL bAbortThread;

    NTFS_COMPARE_DATA(HDPA h, DWORD dw) : hItemList(h), dwSIFlags(dw) {}
    ~NTFS_COMPARE_DATA();
};
typedef NTFS_COMPARE_DATA *PNTFS_COMPARE_DATA;


class CNTFSSecurity : public CSecurityInformation
{
private:
    HANDLE              m_hCompareThread;
    PNTFS_COMPARE_DATA  m_pCompareData;
    IProgressDialog    *m_pProgressDlg;
    HWND                m_hwndPopupOwner;
    PSECURITY_DESCRIPTOR m_psdOwnerFullControl;

public:
    CNTFSSecurity(SE_OBJECT_TYPE seType);
    virtual ~CNTFSSecurity();

    STDMETHOD(Initialize)(HDPA   hItemList,
                          DWORD  dwFlags,
                          LPTSTR pszServer,
                          LPTSTR pszObject);

    // ISecurityInformation methods not handled by CSecurityInformation
    // or overridden here.
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
    STDMETHOD(GetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault);
    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage);
protected:

    HRESULT SetSecurityLocal(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR pSD,
                             LPBOOL pbNotAllApplied = NULL);
    HRESULT SetSecurityDeep(LPTSTR *ppszBuffer,
                            LPUINT pcchBuffer,
                            DWORD dwAttributes,
                            SECURITY_INFORMATION si,
                            PSECURITY_DESCRIPTOR pSD,
                            LPBOOL pbNotAllApplied = NULL);
    void CreateProgressDialog(SECURITY_INFORMATION si);
    void CloseProgressDialog(void);
    HRESULT SetProgress(LPTSTR pszFile);
    HRESULT BuildOwnerFullControlSD(PSECURITY_DESCRIPTOR pSD);

#ifdef SUPPORT_NTFS_DEFAULT
    HRESULT GetParentSecurity(SECURITY_INFORMATION si,
                              PSECURITY_DESCRIPTOR *ppSD);
    HRESULT GetDefaultSecurity(SECURITY_INFORMATION si,
                               PSECURITY_DESCRIPTOR *ppSD);
#endif

#ifdef DONT_USE_ACLAPI
    STDMETHOD(ReadObjectSecurity)(LPCTSTR pszObject,
                                  SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD);
#endif  // DONT_USE_ACLAPI
    STDMETHOD(WriteObjectSecurity)(LPCTSTR pszObject,
                                   SECURITY_INFORMATION si,
                                   PSECURITY_DESCRIPTOR pSD);

    void WaitForComparison();
    static DWORD WINAPI NTFSReadSD(LPCTSTR pszObject,
                                   SECURITY_INFORMATION si,
                                   PSECURITY_DESCRIPTOR* ppSD);
    static DWORD WINAPI NTFSCompareThreadProc(LPVOID pvData);
};

#endif  /* _NTFSSI_H_ */
