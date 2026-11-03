/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IQueryAssociations / IAssociationArray (element-based, CAssocArray style)
 * COPYRIGHT:   Copyright 2002 Jon Griffiths (Wine)
 *              Copyright 2024-2026 Whindmar Saksit <whindsaks@proton.me>
 *              Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "CQueryAssocCommon.h"

/*****************************************************************************
 * AEINFO - element entry stored in the DSA
 *
 * Layout reconstructed from the static data in the assembly listing:
 *   offset  0: DWORD  dwFlags
 *   offset  4: GUID*  pClsid
 *   offset  8: DWORD  reserved
 *   offset 12: PFUNC  pfnCreate
 *   offset 16: IAssociationElementOld* pElement
 */
typedef struct AEINFO
{
    DWORD                   dwFlags;
    const GUID             *pClsid;
    DWORD                   dwReserved;
    HRESULT (CALLBACK *pfnCreate)(const GUID*, PCWSTR, IAssociationElementOld**);
    IAssociationElementOld *pElement;
} AEINFO, *PAEINFO;

enum GETELEMENT
{
    GETELEMENT_SUCCESS        = 1,
    GETELEMENT_FLAGSMISMATCH  = 0,
    GETELEMENT_OUTOFRANGE     = -1,
    GETELEMENT_CREATIONFAILED = -2,
};

class CEnumAssocElems;

class CQueryAssociations :
    public CComCoClass<CQueryAssociations, &CLSID_QueryAssociations>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IAssociationArrayOld,
    public IAssociationArrayInitialize,
    public IQueryAssociations
{
public:
    CQueryAssociations();
    ~CQueryAssociations();

    DECLARE_REGISTRY_RESOURCEID(IDR_QUERYASSOCIATIONS)
    DECLARE_NOT_AGGREGATABLE(CQueryAssociations)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CQueryAssociations)
        COM_INTERFACE_ENTRY_IID(IID_IAssociationArrayOld, IAssociationArrayOld)
        COM_INTERFACE_ENTRY_IID(IID_IAssociationArrayInitialize, IAssociationArrayInitialize)
        COM_INTERFACE_ENTRY_IID(IID_IQueryAssociations, IQueryAssociations)
    END_COM_MAP()

    // IAssociationArrayOld
    STDMETHODIMP EnumElements(ULONG flags, IEnumAssociationElementsOld **ppEnum) override;
    STDMETHODIMP QueryString(ULONG flags, ASSOCQUERY query, PCWSTR key, PWSTR *ppszValue) override;
    STDMETHODIMP QueryDword(ULONG flags, ASSOCQUERY query, PCWSTR key, DWORD *pdwValue) override;
    STDMETHODIMP QueryExists(ULONG flags, ASSOCQUERY query, PCWSTR key) override;
    STDMETHODIMP QueryDirect(ULONG flags, ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB **ppBlob) override;
    STDMETHODIMP QueryObject(ULONG flags, ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID *ppvObj) override;

    // IAssociationArrayInitialize
    STDMETHODIMP InitClassElements(ULONG flags, PCWSTR pszClass) override;
    STDMETHODIMP InsertElements(ULONG flags, IEnumAssociationElementsOld *pElements) override;
    STDMETHODIMP FilterElements(ULONG dwFilter) override;

    // IQueryAssociations
    STDMETHOD(Init)(ASSOCF flags, LPCWSTR pwszAssoc, HKEY hkProgid, HWND hwnd) override;
    STDMETHOD(GetString)(ASSOCF flags, ASSOCSTR str, LPCWSTR pwszExtra, LPWSTR pwszOut, DWORD *pcchOut) override;
    STDMETHOD(GetKey)(ASSOCF flags, ASSOCKEY key, LPCWSTR pwszExtra, HKEY *phkeyOut) override;
    STDMETHOD(GetData)(ASSOCF flags, ASSOCDATA data, LPCWSTR pwszExtra, void *pvOut, DWORD *pcbOut) override;
    STDMETHOD(GetEnum)(ASSOCF cfFlags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut) override;

private:
    // Internal state (CAssocArray style)
    HRESULT                      m_hr;
    LPWSTR                       m_pszClass;
    DWORD                        m_dwFilter;
    BOOL                         m_bOpenByExeName;
    HDSA                         m_hDSA;

    IEnumAssociationElementsOld *m_pElement1;
    DWORD                        m_dwFlags1;
    IEnumAssociationElementsOld *m_pElement2;
    DWORD                        m_dwFlags2;

    // Verb cache
    IAssociationElementOld      *m_pElement3;
    LPWSTR                       m_pszVerb;
    DWORD                        m_dwFlags3;
    IAssociationElementOld      *m_pParent;

public:
    // Helpers
    void        _Reset();
    BOOL        _FirstElement(DWORD dwFlags, IAssociationElementOld **ppElem);
    GETELEMENT  _GetElement(INT iItem, DWORD dwFlags, IAssociationElementOld **ppElem);
    HRESULT     _GetCachedVerbElement(DWORD dwFlags, PCWSTR pszVerb,
                                      IAssociationElementOld **ppElem,
                                      IAssociationElementOld **ppParent);
    void        _SetCachedVerbElement(DWORD dwFlags, PCWSTR pszVerb,
                                      IAssociationElementOld *pElem,
                                      IAssociationElementOld *pParent);
    HRESULT     _GetVerbElement(DWORD dwFlags, PCWSTR pszVerb,
                                IAssociationElementOld **ppElem,
                                IAssociationElementOld **ppParent);
    void        _InitDelayedElements(INT iInsertPos, DWORD dwFlags);
    HRESULT     _InsertSingleElement(IUnknown *pUnk);

    template <typename T_OUT>
    HRESULT _QueryElementAny(
        HRESULT (CALLBACK *pfnQuery)(IAssociationElementOld*, ASSOCQUERY, PCWSTR, PVOID, PVOID*),
        DWORD      dwFlags,
        ASSOCQUERY query,
        PCWSTR     pszKey,
        T_OUT      pOut,
        PVOID     *ppvObj);

    // Legacy registry state (kept for GetString/GetKey/GetData compatibility)
    HKEY                            hkeySource;
    HKEY                            hkeyProgID;
    ASSOCF                          m_InitFlags;
    BYTE                            m_InitType;
    CComPtr<CQueryAssociations>     m_pBaseClass;
    bool                            m_SkipBaseClass;

    void Reset();
    HKEY GetClassHandle() const { return hkeyProgID ? hkeyProgID : hkeySource; }
    CQueryAssociations* TryBaseClass(ASSOCF flags);

    // Original private helpers (kept)
    HRESULT DuplicateHKey(HKEY hKey, HKEY *phKey);
    HRESULT OpenShellVerbKey(ASSOCF flags, LPCWSTR pszVerb, LPCWSTR pszSubKey, CRegKeyRef &KeyRef);
    HRESULT GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size);
    HRESULT GetCommand(const WCHAR *extra, WCHAR **command);
    HRESULT GetExecutable(LPCWSTR pszExtra, LPWSTR path, DWORD pathlen, DWORD *len);
    HRESULT ReturnData(void *out, DWORD *outlen, const void *data, DWORD datalen);
    HRESULT ReturnString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPCWSTR data, DWORD datalen);
    HRESULT ReturnAndFreeString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPWSTR data);
    HRESULT ReturnRegData(ASSOCF flags, HKEY hKey, PCWSTR pszName, void *pvOut, DWORD *pcbOut);
    bool EmulateWin2000() const;
    bool IsNT5() const;
};

// Enumerator
class CEnumAssocElems : public IEnumAssociationElementsOld
{
    LONG                    m_cRefs;
    INT                     m_iItem;
    DWORD                   m_dwFlags;
    CQueryAssociations     *m_pAssocArray;

public:
    CEnumAssocElems(CQueryAssociations *pAssocArray, DWORD dwFlags);
    virtual ~CEnumAssocElems();

    STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP Next(ULONG celt, IAssociationElementOld **pElements, ULONG *pceltFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumAssociationElementsOld **ppNew) override;

private:
    HRESULT _Next(IAssociationElementOld **ppElem);
};
