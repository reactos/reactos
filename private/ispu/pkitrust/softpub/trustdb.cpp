//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       trustdb.cpp
//
//--------------------------------------------------------------------------

//
// PersonalTrustDb.cpp
//
// Code that maintains a list of trusted publishers, agencies, and so on.
// The list is stored in the registry under the root
//
//      HKEY_CURRENT_USER\Software\Microsoft\Secure\WinTrust\
//          Trust Providers\Software Publishing\Trust Database
//
// (Note the fact that this is user-sensitive.)
//
// Under this key are the keys "0", "1", "2", and so on. Each of these represents
// the (exact) distance from the leaf of the certification hierarchy for which a
// given certificate is trusted.
//
// Under each of these keys is a list of values, where
//      value name  == stringized form of a cert's (issuer name, serial number)
//      value value == display name of cert to show in UI
//
// In addition, there is a value under the root key with the name
//      Trust Commercial Publishers
// If this value is absent, or if it is present and has a non "0" value
// then commercial publishers are trusted. Otherwise they are not

#include    "global.hxx"
#include    "cryptreg.h"
#include    "trustdb.h"

/////////////////////////////////////////////////////////

DECLARE_INTERFACE (IUnkInner)
    {
    STDMETHOD(InnerQueryInterface) (THIS_ REFIID iid, LPVOID* ppv) PURE;
    STDMETHOD_ (ULONG, InnerAddRef) (THIS) PURE;
    STDMETHOD_ (ULONG, InnerRelease) (THIS) PURE;
    };

/////////////////////////////////////////////////////////

const WCHAR szTrustDB[]     = REGPATH_WINTRUST_POLICY_FLAGS L"\\Trust Database";
const WCHAR szCommercial[]  = L"Trust Commercial Publishers";

extern "C" const GUID IID_IPersonalTrustDB = IID_IPersonalTrustDB_Data;

/////////////////////////////////////////////////////////

HRESULT WINAPI OpenTrustDB(IUnknown* punkOuter, REFIID iid, void** ppv);

class CTrustDB : IPersonalTrustDB, IUnkInner
    {
        LONG        m_refs;             // our reference count
        IUnknown*   m_punkOuter;        // our controlling unknown (may be us ourselves)

        HKEY        m_hkeyCurUser;      // the "real" current user!
        HKEY        m_hkeyTrustDB;      // the root of our trust database
        HKEY        m_hkeyZero;         // cached leaf key
        HKEY        m_hkeyOne;          // cached agency key

        HCRYPTPROV  m_hprov;            // cryptographic provider for name hashing

public:
    static HRESULT CreateInstance(IUnknown* punkOuter, REFIID iid, void** ppv);

private:
    STDMETHODIMP         QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(THIS);
    STDMETHODIMP_(ULONG) Release(THIS);

    STDMETHODIMP         InnerQueryInterface(REFIID iid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) InnerAddRef();
    STDMETHODIMP_(ULONG) InnerRelease();

    STDMETHODIMP         IsTrustedCert(DWORD dwEncodingType, PCCERT_CONTEXT pCert, LONG iLevel, BOOL fCommercial);
    STDMETHODIMP         AddTrustCert(PCCERT_CONTEXT pCert,       LONG iLevel, BOOL fLowerLevelsToo);

    STDMETHODIMP         RemoveTrustCert(PCCERT_CONTEXT pCert,       LONG iLevel, BOOL fLowerLevelsToo);
    STDMETHODIMP         RemoveTrustToken(LPWSTR,           LONG iLevel, BOOL fLowerLevelsToo);

    STDMETHODIMP         AreCommercialPublishersTrusted();
    STDMETHODIMP         SetCommercialPublishersTrust(BOOL fTrust);

    STDMETHODIMP         GetTrustList(
                            LONG                iLevel,             // the cert chain level to get
                            BOOL                fLowerLevelsToo,    // included lower levels, remove duplicates
                            TRUSTLISTENTRY**    prgTrustList,       // place to return the trust list
                            ULONG*              pcTrustList         // place to return the size of the returned trust list
                            );
private:
                        CTrustDB(IUnknown* punkOuter);
                        ~CTrustDB();
    HRESULT             Init();
    void                BytesToString(ULONG cb, void* pv, LPWSTR sz);
    HRESULT             X500NAMEToString(ULONG cb, void*pv, LPWSTR szDest);
    HKEY                KeyOfLevel(LONG iLevel);
    BOOL                ShouldClose(LONG iLevel) { return iLevel > 1; }

    HRESULT             GetIssuerSerialString(PCCERT_CONTEXT pCert, LPWSTR *ppsz);

    };

/////////////////////////////////////////////////////////////////////////////

HRESULT CTrustDB::IsTrustedCert(DWORD dwEncodingType,
                                PCCERT_CONTEXT pCert,
                                LONG iLevel,
                                BOOL fCommercial)
    {
    HRESULT hr;
    LPWSTR pszValueName;
    hr = GetIssuerSerialString(pCert, &pszValueName);
    if (FAILED(hr))
        return hr;

    //
    // Get the key to query under
    //
    HKEY hkey = KeyOfLevel(iLevel);
    if (hkey)
        {
        //
        // Do the query. If present, it's trusted.
        //
        DWORD dwType;
        if (RegQueryValueExU(hkey, pszValueName, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
            {
            hr = S_OK;      // trusted
            }
        else
            {
            hr = S_FALSE;   // not trusted
            }
        if (ShouldClose(iLevel))
            RegCloseKey(hkey);
        }
    else
        hr = E_UNEXPECTED;

    CoTaskMemFree(pszValueName);
    return hr;
    }

HRESULT CTrustDB::AddTrustCert(PCCERT_CONTEXT pCert, LONG iLevel, BOOL fLowerLevelsToo)


// Add trust in the indicated certificate at the indicated level
    {
    HRESULT hr;
    LPWSTR pszValueName;
    hr = GetIssuerSerialString(pCert, &pszValueName);
    if (FAILED(hr))
        return hr;
    //
    // Get the key to query under
    //
    HKEY hkey = KeyOfLevel(iLevel);
    if (hkey)
        {
        //
        // Get the value value to set.
        //
        LPWSTR pwszName;

        if (iLevel >= 1)
            pwszName = spGetAgencyNameOfCert(pCert);
        else
            pwszName = spGetPublisherNameOfCert(pCert);

        if (NULL == pwszName )
            hr = E_UNEXPECTED;
        else
            {
            //
            // Set the value
            //
            if (RegSetValueExU(hkey, pszValueName, NULL, REG_SZ, (BYTE*)pwszName, (wcslen(pwszName)+1) * sizeof(WCHAR)) == ERROR_SUCCESS)
                {
                // Success!
                }
            else
                hr = E_UNEXPECTED;

            }

            delete pwszName;
        if (ShouldClose(iLevel))
            RegCloseKey(hkey);
        }
    else
        hr = E_UNEXPECTED;

//     #ifdef 0
//     if (hr==S_OK)
//         {
//         ASSERT(IsTrustedCert(pCert, iLevel, FALSE) == S_OK);
//         }
//     #endif

    //
    // If we are succesful, then recurse to lower levels if necessary
    // REVIEW: this can be made zippier, but that's probably not worth
    // it given how infrequently this is called.
    //
    if (hr==S_OK && fLowerLevelsToo && iLevel > 0)
        {
        hr = AddTrustCert(pCert, iLevel-1, fLowerLevelsToo);
        }

    CoTaskMemFree(pszValueName);
    return hr;
    }

HRESULT CTrustDB::RemoveTrustCert(PCCERT_CONTEXT pCert, LONG iLevel, BOOL fLowerLevelsToo)
    {
    HRESULT hr;
    LPWSTR pszValueName;
    hr = GetIssuerSerialString(pCert, &pszValueName);
    if (FAILED(hr))
        return hr;

    //
    // Remove the value
    //
    RemoveTrustToken(pszValueName, iLevel, fLowerLevelsToo);

//     #ifdef 0
//     if (hr==S_OK)
//         {
//         ASSERT(IsTrustedCert(pCert, iLevel, FALSE) == S_FALSE);
//         }
//     #endif

    CoTaskMemFree(pszValueName);
    return hr;
    }

HRESULT CTrustDB::RemoveTrustToken(LPWSTR szToken, LONG iLevel, BOOL fLowerLevelsToo)
    {
    HRESULT hr = S_OK;
    //
    // Get the key to query under
    //
    HKEY hkey = KeyOfLevel(iLevel);
    if (hkey)
        {
        //
        // Remove the value
        //
        RegDeleteValueU(hkey, szToken);
        //
        // Clean up
        //
        if (ShouldClose(iLevel))
            RegCloseKey(hkey);
        }
    else
        hr = E_UNEXPECTED;

    //
    // If we are succesful, then recurse to lower levels if necessary
    //
    if (hr==S_OK && fLowerLevelsToo && iLevel > 0)
        {
        hr = RemoveTrustToken(szToken, iLevel-1, fLowerLevelsToo);
        }

    return hr;
    }

HRESULT CTrustDB::AreCommercialPublishersTrusted()
// Answer whether commercial publishers are trusted.
//      S_OK == yes
//      S_FALSE == no
//      other == can't tell
    {
        return( S_FALSE );
    }

HRESULT CTrustDB::SetCommercialPublishersTrust(BOOL fTrust)
// Set the commercial trust setting
    {
        return( S_OK );
    }

/////////////////////////////////////////////////////////////////////////////

HRESULT CTrustDB::GetTrustList(
// Return the (unsorted) list of trusted certificate names and their
// corresponding display names
//
    LONG                iLevel,             // the cert chain level to get
    BOOL                fLowerLevelsToo,    // included lower levels, remove duplicates
    TRUSTLISTENTRY**    prgTrustList,       // place to return the trust list
    ULONG*              pcTrustList         // place to return the size of the returned trust list
    ) {
    HRESULT hr = S_OK;
    *prgTrustList = NULL;
    *pcTrustList  = 0;

    DWORD cbMaxValue = 0;
    BYTE *pbValue = NULL;

    // We just enumerate all the subkeys of the database root. The sum of the
    // number of values contained under each is an upper bound on the number
    // of trusted entries that we have.
    //
    ULONG       cTrust = 0;
    DWORD       dwRootIndex;
    WCHAR       szSubkey[MAX_PATH+1];
    DWORD       dwSubkeyLen = MAX_PATH+1;
    FILETIME    ft;
    for (dwRootIndex = 0;
         RegEnumKeyExU(
                m_hkeyTrustDB, 
                dwRootIndex, 
                (LPWSTR)szSubkey, 
                &dwSubkeyLen,
                0,
                NULL,
                NULL,
                &ft)!=ERROR_NO_MORE_ITEMS;
         dwRootIndex++
         )
        {
        HKEY hkey;
        if (RegOpenKeyExU(m_hkeyTrustDB, (LPWSTR)szSubkey, 0, MAXIMUM_ALLOWED, &hkey) == ERROR_SUCCESS)
            {
            // We've found a subkey. Count the values thereunder.
            //
            WCHAR    achClass[MAX_PATH];       /* buffer for class name   */
            DWORD    cchClassName = MAX_PATH;  /* length of class string  */
            DWORD    cSubKeys;                 /* number of subkeys       */
            DWORD    cbMaxSubKey;              /* longest subkey size     */
            DWORD    cchMaxClass;              /* longest class string    */
            DWORD    cValues;              /* number of values for key    */
            DWORD    cchMaxValue;          /* longest value name          */
            DWORD    cbMaxValueData;       /* longest value data          */
            DWORD    cbSecurityDescriptor; /* size of security descriptor */
            FILETIME ftLastWriteTime;      /* last write time             */

            // Get the value count.
            if (ERROR_SUCCESS==
                    RegQueryInfoKeyU(hkey,        /* key handle                    */
                        achClass,                /* buffer for class name         */
                        &cchClassName,           /* length of class string        */
                        NULL,                    /* reserved                      */
                        &cSubKeys,               /* number of subkeys             */
                        &cbMaxSubKey,            /* longest subkey size           */
                        &cchMaxClass,            /* longest class string          */
                        &cValues,                /* number of values for this key */
                        &cchMaxValue,            /* longest value name            */
                        &cbMaxValueData,         /* longest value data            */
                        &cbSecurityDescriptor,   /* security descriptor           */
                        &ftLastWriteTime))       /* last write time               */
                {
                cTrust += cValues;
                if (cbMaxValueData > cbMaxValue)
                    cbMaxValue = cbMaxValueData;
                }
            else
                {
                hr = E_UNEXPECTED;
                break;
                }

            RegCloseKey(hkey);
            }
        else
            {
            hr = E_UNEXPECTED;
            break;
            }
        //
        // reset the buffer size for next call
        //
        dwSubkeyLen = MAX_PATH+1;
        }

    if (hr==S_OK)
        {
        // At this point, cTrust has an upper bound on the number of
        // trust entries that we'll find. Assume that we need them all
        // and, accordingly, allocate the output buffer.
        //
        pbValue = (BYTE *) CoTaskMemAlloc(cbMaxValue);
        if (pbValue)
            {
            if (cTrust == 0)
            {
                CoTaskMemFree(pbValue);
                return S_OK;
            }
            *prgTrustList = (TRUSTLISTENTRY*) CoTaskMemAlloc(cTrust * sizeof(TRUSTLISTENTRY));
            }
        if (*prgTrustList && pbValue)
            {
            // Once again, iterate through each of the subkeys of the root key
            //
            DWORD       dwRootIndex;
            WCHAR       szSubkey[MAX_PATH+1];
            DWORD       dwSubkeyLen = MAX_PATH+1;
            FILETIME    ft;
            for (dwRootIndex = 0;
                 RegEnumKeyExU(
                        m_hkeyTrustDB, 
                        dwRootIndex, 
                        (LPWSTR)szSubkey, 
                        &dwSubkeyLen, 
                        0, 
                        NULL, 
                        NULL, 
                        &ft)!=ERROR_NO_MORE_ITEMS;
                 dwRootIndex++
                 )
                {
                HKEY hkey;
                LONG iLevel = _wtol(szSubkey);
                if (RegOpenKeyExU(m_hkeyTrustDB, (LPWSTR)szSubkey, 0, MAXIMUM_ALLOWED, &hkey) == ERROR_SUCCESS)
                    {
                    // We've found a subkey. Enumerate the values thereunder.
                    //
                    DWORD dwIndex, cbToken, dwType, cbValue, dw;
                    TRUSTLISTENTRY* pEntry;
                    for (dwIndex = 0;
                        // ---------
                         pEntry = &(*prgTrustList)[*pcTrustList],
                         cbToken = MAX_PATH * sizeof(WCHAR),
                         cbValue = cbMaxValue,
                         *pcTrustList < cTrust
                            && (dw = RegEnumValueU(hkey,
                                dwIndex,
                                (LPWSTR)(&pEntry->szToken),
                                &cbToken,
                                NULL,
                                &dwType,
                                pbValue,
                                &cbValue
                                )) != ERROR_NO_MORE_ITEMS
                            && (dw != ERROR_INVALID_PARAMETER)     // Win95 hack
                            ;
                        // ---------
                         *pcTrustList += 1,  // nb: only bump this on success
                         dwIndex++
                        ) {

                        // Update the display name
                        if (cbValue > (sizeof(pEntry->szDisplayName) - 1))
                            cbValue = sizeof(pEntry->szDisplayName) - 1;
                        if (cbValue)
                            memcpy(pEntry->szDisplayName, pbValue, cbValue);
                        pEntry->szDisplayName[cbValue] = L'\0';
                        //
                        // Remember the level at which we found this
                        //
                        pEntry->iLevel = iLevel;
                        //
                        // If this was in fact a duplicate, then forget about it
                        //
                        ULONG i;
                        for (i=0; i < *pcTrustList; i++)
                            {
                            TRUSTLISTENTRY* pHim = &(*prgTrustList)[i];
                            if (lstrcmpW(pHim->szToken, pEntry->szToken) == 0)
                                {
                                //
                                // REVIEW: put in a better display name in the
                                // same-name-but-different-level case
                                //
                                if (pHim->iLevel == iLevel)
                                    {
                                    // If it's a complete duplicate, omit it
                                    *pcTrustList -= 1;
                                    }
                                break;
                                }
                            } // end loop looking for duplicates
                        } // end loop over values

                    RegCloseKey(hkey);
                    }
                }
            }
        else
            hr = E_OUTOFMEMORY;
        CoTaskMemFree(pbValue);
        }


    return hr;
    }

/////////////////////////////////////////////////////////////////////////////

HKEY CTrustDB::KeyOfLevel(LONG iLevel)
// Answer the key to use for accessing the given level of the cert chain
    {
    #ifdef _UNICODE
        #error NYI
    #endif
    HKEY hkey = NULL;

    switch (iLevel)
        {
    case 0: hkey = m_hkeyZero;  break;
    case 1: hkey = m_hkeyOne;   break;
    default:
        {
        DWORD dwDisposition;
        WCHAR sz[10];
        _ltow(iLevel, (WCHAR*)sz, 10);  // nb: won't work in Unicode
        if (RegCreateKeyExU(m_hkeyTrustDB, sz, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hkey, &dwDisposition) != ERROR_SUCCESS)
            hkey = NULL;
        } // end default
        } // end switch
    return hkey;
    }

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CTrustDB::QueryInterface(REFIID iid, LPVOID* ppv)
    {
    return (m_punkOuter->QueryInterface(iid, ppv));
    }
STDMETHODIMP_(ULONG) CTrustDB::AddRef(void)
    {
    return (m_punkOuter->AddRef());
    }
STDMETHODIMP_(ULONG) CTrustDB::Release(void)
    {
    return (m_punkOuter->Release());
    }

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CTrustDB::InnerQueryInterface(REFIID iid, LPVOID* ppv)
    {
    *ppv = NULL;
    while (TRUE)
        {
        if (iid == IID_IUnknown)
            {
            *ppv = (LPVOID)((IUnkInner*)this);
            break;
            }
        if (iid == IID_IPersonalTrustDB)
            {
            *ppv = (LPVOID) ((IPersonalTrustDB *) this);
            break;
            }
        return E_NOINTERFACE;
        }
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
    }
STDMETHODIMP_(ULONG) CTrustDB::InnerAddRef(void)
    {
    return ++m_refs;
    }
STDMETHODIMP_(ULONG) CTrustDB::InnerRelease(void)
    {
    ULONG refs = --m_refs;
    if (refs == 0)
        {
        m_refs = 1;
        delete this;
        }
    return refs;
    }

/////////////////////////////////////////////////////////////////////////////

HRESULT OpenTrustDB(IUnknown* punkOuter, REFIID iid, void** ppv)
    {
    return CTrustDB::CreateInstance(punkOuter, iid, ppv);
    }

HRESULT CTrustDB::CreateInstance(IUnknown* punkOuter, REFIID iid, void** ppv)
    {
    HRESULT hr;

    *ppv = NULL;
    CTrustDB* pnew = new CTrustDB(punkOuter);
    if (pnew == NULL) return E_OUTOFMEMORY;
    if ((hr = pnew->Init()) != S_OK)
        {
        delete pnew;
        return hr;
        }
    IUnkInner* pme = (IUnkInner*)pnew;
    hr = pme->InnerQueryInterface(iid, ppv);
    pme->InnerRelease();                // balance starting ref cnt of one
    return hr;
    }

CTrustDB::CTrustDB(IUnknown* punkOuter) :
        m_refs(1),
        m_hkeyCurUser(NULL),
        m_hkeyTrustDB(NULL),
        m_hkeyZero(NULL),
        m_hkeyOne(NULL),
        m_hprov(NULL)
    {
    if (punkOuter == NULL)
        m_punkOuter = (IUnknown *) ((LPVOID) ((IUnkInner *) this));
    else
        m_punkOuter = punkOuter;
    }

CTrustDB::~CTrustDB()
    {
    if (m_hkeyOne)
        RegCloseKey(m_hkeyOne);
    if (m_hkeyZero)
        RegCloseKey(m_hkeyZero);
    if (m_hkeyTrustDB)
        RegCloseKey(m_hkeyTrustDB);

    if (m_hkeyCurUser)
        RegCloseHKCU(m_hkeyCurUser);

    if (m_hprov)
        CryptReleaseContext(m_hprov, 0);
    }

HRESULT CTrustDB::Init()
    {
    HRESULT hr = S_OK;

    DWORD dwDisposition;

    if (( hr = RegOpenHKCU(&m_hkeyCurUser) ) != ERROR_SUCCESS)
    {
        m_hkeyCurUser = NULL;
        return(hr);
    }

    if (( hr = RegCreateKeyExU(m_hkeyCurUser, szTrustDB,    0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &m_hkeyTrustDB, &dwDisposition) ) != ERROR_SUCCESS
    ||  ( hr = RegCreateKeyExU(m_hkeyTrustDB, L"0",        0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &m_hkeyZero, &dwDisposition) ) != ERROR_SUCCESS
    ||  ( hr = RegCreateKeyExU(m_hkeyTrustDB, L"1",        0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &m_hkeyOne, &dwDisposition) ) != ERROR_SUCCESS)
        return(hr);
    if (!CryptAcquireContext(&m_hprov, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return(GetLastError());

    return S_OK;
    }


/////////////////////////////////////////////////////////////////////////////

void CTrustDB::BytesToString(ULONG cb, void* pv, LPWSTR sz)
// Convert the bytes into some string form.
// Needs (cb * 2 + 1) * sizeof(WCHAR) bytes of space in sz
    {
    BYTE* pb = (BYTE*)pv;
    for (ULONG i = 0; i<cb; i++)
        {
        int b = *pb;
        *sz++ = (((b & 0xF0)>>4) + L'a');
        *sz++ =  ((b & 0x0F)     + L'a');
        pb++;
        }
    *sz++ = 0;
    }

HRESULT CTrustDB::X500NAMEToString(ULONG cb, void*pv, LPWSTR szDest)
//
// X500 names can have VERY long encodings, so we can't just
// do a literal vanilla encoding
//
// There must be CBX500NAME characters of space in the destination
//
// NOTE: We rely on the lack of collision in the hash values.
// Chance of a collision for a set of 'p' names is approx:
//
//         p^2 / n
//
// (if p<<n) where n (with MD5) is 2^128. An amazingly small chance.
//
    {
    #define CBHASH      16                  // MD5
    #define CBX500NAME  (2*CBHASH + 1)
    HRESULT hr = S_OK;
    HCRYPTHASH hash;
    if (CryptCreateHash(m_hprov, CALG_MD5, NULL, NULL, &hash))
        {
        if (CryptHashData(hash, (BYTE*)pv, cb, 0))
            {
            BYTE rgb[CBHASH];
            ULONG cb = CBHASH;
            if (CryptGetHashParam(hash, HP_HASHVAL, &rgb[0], &cb, 0))
                {
                BytesToString(cb, &rgb[0], szDest);
                }
            else
                hr = GetLastError();
            }
        else
            hr = GetLastError();
        CryptDestroyHash(hash);
        }
    else
        hr = GetLastError();
    return hr;
    }

/////////////////////////////////////////////////////////////////////////////
HRESULT CTrustDB::GetIssuerSerialString(PCCERT_CONTEXT pCert, LPWSTR *ppsz)
// Conver the issuer and serial number to some reasonable string form.
    {
    HRESULT hr = S_OK;
    PCERT_INFO pCertInfo = pCert->pCertInfo;
    ULONG cbIssuer = CBX500NAME * sizeof(WCHAR);
    ULONG cbSerial = (pCertInfo->SerialNumber.cbData*2+1) * sizeof(WCHAR);
    WCHAR* sz      = (WCHAR*)CoTaskMemAlloc(cbSerial + sizeof(WCHAR) + cbIssuer);
    if (sz)
        {
        if (S_OK == (hr = X500NAMEToString(
                pCertInfo->Issuer.cbData,
                pCertInfo->Issuer.pbData,
                sz
                )))
            {
            WCHAR* szNext = &sz[CBX500NAME-1];

            *szNext++ = L' ';
            BytesToString(
                pCertInfo->SerialNumber.cbData,
                pCertInfo->SerialNumber.pbData,
                szNext
                );
            }
        else
            {
            CoTaskMemFree(sz);
            sz = NULL;
            }
        }
    *ppsz = sz;
    return hr;
    }

