#include "private.h"
#include "subitem.h"
#include "subsmgrp.h"

#include "helper.h"
#include "offl_cpp.h"   // Yech. Pidl stuff.

const TCHAR c_szSubscriptionInfoValue[] = TEXT("~SubsInfo");

#ifdef UNICODE
#define c_wszSubscriptionInfoValue c_szSubscriptionInfoValue
#else
const WCHAR c_wszSubscriptionInfoValue[] = L"~SubsInfo";
#endif

//  pragmas are for compatibility with the notification manager
//  registry data structures which were not pack 8

#pragma pack(push, RegVariantBlob, 1)

struct SimpleVariantBlob
{
    VARIANT var;
};

struct BSTRVariantBlob : public SimpleVariantBlob
{
    DWORD   dwSize;
//    WCHAR   wsz[];    //  Variable length string
};

struct SignatureSimpleBlob
{
    DWORD               dwSignature;
    SimpleVariantBlob   svb;
};

struct SignatureBSTRBlob
{
    DWORD               dwSignature;
    BSTRVariantBlob     bvb;
};

#pragma pack(pop, RegVariantBlob)


#define BLOB_SIGNATURE 0x4b434f4d

HRESULT BlobToVariant(BYTE *pData, DWORD cbData, VARIANT *pVar, DWORD *pcbUsed)
{
    HRESULT hr = S_OK;
    SimpleVariantBlob *pBlob = (SimpleVariantBlob *)pData;

    ASSERT(NULL != pBlob);
    ASSERT(cbData >= sizeof(SimpleVariantBlob));
    ASSERT(NULL != pVar);

    if ((NULL != pBlob) &&
        (cbData >= sizeof(SimpleVariantBlob)) &&
        (NULL != pVar))
    {
        memcpy(pVar, &pBlob->var, sizeof(VARIANT));

        switch (pVar->vt)
        {
            case VT_I4:                 // LONG           
            case VT_UI1:                // BYTE           
            case VT_I2:                 // SHORT          
            case VT_R4:                 // FLOAT          
            case VT_R8:                 // DOUBLE         
            case VT_BOOL:               // VARIANT_BOOL   
            case VT_ERROR:              // SCODE          
            case VT_CY:                 // CY             
            case VT_DATE:               // DATE
            case VT_I1:                 // CHAR           
            case VT_UI2:                // USHORT         
            case VT_UI4:                // ULONG          
            case VT_INT:                // INT            
            case VT_UINT:               // UINT           
                if (pcbUsed)
                {
                    *pcbUsed = sizeof(SimpleVariantBlob);
                }
                break;                

            case VT_BSTR:               // BSTR
                hr = E_UNEXPECTED;

                ASSERT(cbData >= sizeof(BSTRVariantBlob));

                if (cbData >= sizeof(BSTRVariantBlob))
                {
                    BSTRVariantBlob *pbstrBlob = (BSTRVariantBlob *)pData;
                    DWORD dwSize = pbstrBlob->dwSize;

                    ASSERT(cbData >= (sizeof(BSTRVariantBlob) + dwSize));

                    if (cbData >= (sizeof(BSTRVariantBlob) + dwSize))
                    {
                        pVar->bstrVal = SysAllocStringByteLen(NULL, dwSize);

                        if (NULL != pVar->bstrVal)
                        {
                            if (pcbUsed)
                            {
                                *pcbUsed = sizeof(BSTRVariantBlob) + pbstrBlob->dwSize;
                            }
                            memcpy(pVar->bstrVal, 
                                   ((BYTE *)pbstrBlob) + 
                                        (FIELD_OFFSET(BSTRVariantBlob, dwSize) + 
                                        sizeof(dwSize)),
                                   dwSize);
                            hr = S_OK;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
                break;                

            default:
                hr = E_NOTIMPL;
                break;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT SignatureBlobToVariant(BYTE *pData, DWORD cbData, VARIANT *pVar)
{
    HRESULT hr;
    
    SignatureSimpleBlob *pBlob = (SignatureSimpleBlob *)pData;

    ASSERT(NULL != pBlob);
    ASSERT(cbData >= sizeof(SignatureSimpleBlob));
    ASSERT(NULL != pVar);
    ASSERT(BLOB_SIGNATURE == pBlob->dwSignature);

    if ((NULL != pBlob) &&
        (cbData >= sizeof(SignatureSimpleBlob)) &&
        (NULL != pVar) &&
        (BLOB_SIGNATURE == pBlob->dwSignature))
    {
        hr = BlobToVariant((BYTE *)&pBlob->svb, 
                           cbData - (FIELD_OFFSET(SignatureSimpleBlob, svb)),
                           pVar,
                           NULL);
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT VariantToSignatureBlob(const VARIANT *pVar, BYTE **ppData, DWORD *pdwSize)
{
    HRESULT hr;

    ASSERT(NULL != pVar);
    ASSERT(NULL != ppData);
    ASSERT(NULL != pdwSize);

    if ((NULL != pVar) && (NULL != ppData) && (NULL != pdwSize))
    {
        DWORD dwSize;
        DWORD dwBstrLen = 0;

        hr = S_OK;
     
        switch (pVar->vt)
        {
            case VT_I4:                 // LONG           
            case VT_UI1:                // BYTE           
            case VT_I2:                 // SHORT          
            case VT_R4:                 // FLOAT          
            case VT_R8:                 // DOUBLE         
            case VT_BOOL:               // VARIANT_BOOL   
            case VT_ERROR:              // SCODE          
            case VT_CY:                 // CY             
            case VT_DATE:               // DATE
            case VT_I1:                 // CHAR           
            case VT_UI2:                // USHORT         
            case VT_UI4:                // ULONG          
            case VT_INT:                // INT            
            case VT_UINT:               // UINT           
                dwSize = sizeof(SignatureSimpleBlob);
                break;                

            case VT_BSTR:               // BSTR
                if (NULL != pVar->bstrVal) 
                    dwBstrLen = SysStringByteLen(pVar->bstrVal);
                dwSize = sizeof(SignatureBSTRBlob) + dwBstrLen;
                break;
                        
            default:
                hr = E_NOTIMPL;
                dwSize = 0;
                break;
        }

        if (SUCCEEDED(hr))
        {
            SignatureSimpleBlob *pSignatureBlob = (SignatureSimpleBlob *)new BYTE[dwSize];

            if (NULL != pSignatureBlob)
            {
                *ppData = (BYTE *)pSignatureBlob;
                *pdwSize = dwSize;
                
                pSignatureBlob->dwSignature = BLOB_SIGNATURE;

                switch (pVar->vt)
                {
                    case VT_I4:                 // LONG           
                    case VT_UI1:                // BYTE           
                    case VT_I2:                 // SHORT          
                    case VT_R4:                 // FLOAT          
                    case VT_R8:                 // DOUBLE         
                    case VT_BOOL:               // VARIANT_BOOL   
                    case VT_ERROR:              // SCODE          
                    case VT_CY:                 // CY             
                    case VT_DATE:               // DATE
                    case VT_I1:                 // CHAR           
                    case VT_UI2:                // USHORT         
                    case VT_UI4:                // ULONG          
                    case VT_INT:                // INT            
                    case VT_UINT:               // UINT
                    {
                        SimpleVariantBlob *pBlob = &pSignatureBlob->svb;
                        memcpy(&pBlob->var, pVar, sizeof(VARIANT));
                        break;
                    }

                    case VT_BSTR:               // BSTR
                    {
                        BSTRVariantBlob *pbstrBlob = 
                            &((SignatureBSTRBlob *)pSignatureBlob)->bvb;
                        
                        memcpy(&pbstrBlob->var, pVar, sizeof(VARIANT));
                        pbstrBlob->dwSize = dwBstrLen;
                        memcpy(((BYTE *)pbstrBlob) + 
                                  (FIELD_OFFSET(BSTRVariantBlob, dwSize) + 
                                  sizeof(dwSize)),
                               pVar->bstrVal, 
                               dwBstrLen);
                        break;
                    }
                                
                    default:
                        ASSERT(0);  // Default case should have been eliminated!
                        break;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else        
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

CEnumItemProperties::CEnumItemProperties()
{
    ASSERT(0 == m_nCurrent);
    ASSERT(0 == m_nCount);
    ASSERT(NULL == m_pItemProps);

    m_cRef = 1;

    DllAddRef();    
}

CEnumItemProperties::~CEnumItemProperties()
{
    if (NULL != m_pItemProps)
    {
        for (ULONG i = 0; i < m_nCount; i++)
        {
            VariantClear(&m_pItemProps[i].variantValue);
            if (NULL != m_pItemProps[i].pwszName)
            {
                CoTaskMemFree(m_pItemProps[i].pwszName);
            }
        }
        delete [] m_pItemProps;
    }
    DllRelease();
}

HRESULT CEnumItemProperties::Initialize(const SUBSCRIPTIONCOOKIE *pCookie, ISubscriptionItem *psi)
{
    HRESULT hr = E_FAIL;
    HKEY hkey;

    ASSERT(NULL != pCookie);

    if (OpenItemKey(pCookie, FALSE, KEY_READ, &hkey))
    {
        DWORD dwMaxValNameSize;
        DWORD dwMaxDataSize;
        DWORD dwCount;

        if (RegQueryInfoKey(hkey,
            NULL,   // address of buffer for class string 
            NULL,   // address of size of class string buffer 
            NULL,   // reserved 
            NULL,   // address of buffer for number of subkeys 
            NULL,   // address of buffer for longest subkey name length  
            NULL,   // address of buffer for longest class string length 
            &dwCount,   // address of buffer for number of value entries 
            &dwMaxValNameSize,  // address of buffer for longest value name length 
            &dwMaxDataSize, // address of buffer for longest value data length 
            NULL,   // address of buffer for security descriptor length 
            NULL    // address of buffer for last write time
            ) == ERROR_SUCCESS)
        {
            //  This allocates enough for Password as well
            m_pItemProps = new ITEMPROP[dwCount];

            dwMaxValNameSize++; //  Need room for NULL

            //  BUGBUG alloca candidates:

            TCHAR *pszValName = new TCHAR[dwMaxValNameSize];
#ifndef UNICODE
            WCHAR *pwszValName = new WCHAR[dwMaxValNameSize];
#endif
            BYTE *pData = new BYTE[dwMaxDataSize];  
            
            if ((NULL != m_pItemProps) && (NULL != pData) && 
#ifndef UNICODE
                (NULL != pwszValName) &&
#endif
                (NULL != pszValName) 
               )
            {
                hr = S_OK;

                for (ULONG i = 0; i < dwCount; i++)
                {
                    DWORD dwType;
                    DWORD dwSize = dwMaxDataSize;
                    DWORD dwNameSize = dwMaxValNameSize;

                    if (SHEnumValue(hkey, i, pszValName, &dwNameSize, 
                                    &dwType, pData, &dwSize) != ERROR_SUCCESS)
                    {
                        hr = E_UNEXPECTED;
                        break;
                    }

                    //  Skip the default value and our subscription info structure
                    if ((NULL == *pszValName) ||
                        (0 == StrCmp(pszValName, c_szSubscriptionInfoValue))
                       )
                    {
                        continue;
                    }

                    if (dwType != REG_BINARY)
                    {
                        hr = E_UNEXPECTED;
                        break;
                    }

                    hr = SignatureBlobToVariant(pData, dwSize, &m_pItemProps[m_nCount].variantValue);
                    if (SUCCEEDED(hr))
                    {
                        WCHAR *pwszName;
                    #ifdef UNICODE
                        pwszName = pszValName;
                    #else
                        MultiByteToWideChar(CP_ACP, 0, pszValName, -1, 
                                    pwszValName, dwMaxValNameSize);
                        pwszName = pwszValName;
                    #endif                           
                        ULONG ulSize = (lstrlenW(pwszName) + 1) * sizeof(WCHAR);
                        m_pItemProps[m_nCount].pwszName = (WCHAR *)CoTaskMemAlloc(ulSize);
                        if (NULL != m_pItemProps[m_nCount].pwszName)
                        {
                            StrCpyW(m_pItemProps[m_nCount].pwszName, pwszName);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                        m_nCount++;
                    }
                    else
                    {
                        break;
                    }
                }

                if (SUCCEEDED(ReadPassword(psi, &m_pItemProps[m_nCount].variantValue.bstrVal)))
                {
                    m_pItemProps[m_nCount].pwszName = (WCHAR *)CoTaskMemAlloc(sizeof(L"Password"));

                    if (NULL != m_pItemProps[m_nCount].pwszName)
                    {
                        StrCpyW(m_pItemProps[m_nCount].pwszName, L"Password");
                        m_pItemProps[m_nCount].variantValue.vt = VT_BSTR;
                        m_nCount++;
                    }
                    else
                    {
                        SysFreeString(m_pItemProps[m_nCount].variantValue.bstrVal);
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        #ifndef UNICODE
            SAFEDELETE(pwszValName);
        #endif
            SAFEDELETE(pszValName);
            SAFEDELETE(pData);
        }
        RegCloseKey(hkey);
    }
    return hr;
}

// IUnknown members
STDMETHODIMP CEnumItemProperties::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;

    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }

    if ((IID_IUnknown == riid) || (IID_IEnumItemProperties == riid))
    {
        *ppv = (IEnumItemProperties *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    
    return hr;
}

STDMETHODIMP_(ULONG) CEnumItemProperties::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CEnumItemProperties::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CEnumItemProperties::CopyItem(ITEMPROP *pip, WCHAR *pwszName, VARIANT *pVar)
{
    HRESULT hr;

    ASSERT(NULL != pwszName);
    
    if (NULL != pwszName)
    {
        
        ULONG cb = (lstrlenW(pwszName) + 1) * sizeof(WCHAR);

        pip->pwszName = (WCHAR *)CoTaskMemAlloc(cb);
        if (NULL != pip->pwszName)
        {
            StrCpyW(pip->pwszName, pwszName);
            pip->variantValue.vt = VT_EMPTY;    //BUGBUG is this a good idea?
            hr = VariantCopy(&pip->variantValue, pVar);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT CEnumItemProperties::CopyRange(ULONG nStart, ULONG nCount, 
                                       ITEMPROP *ppip, ULONG *pnCopied)
{
    HRESULT hr = S_OK;
    ULONG n = 0;
    ULONG i;

    ASSERT((NULL != ppip) && (NULL != pnCopied));
    
    for (i = nStart; (S_OK == hr) && (i < m_nCount) && (n < nCount); i++, n++)
    {
        hr = CopyItem(&ppip[n], m_pItemProps[i].pwszName, &m_pItemProps[i].variantValue);
    }

    *pnCopied = n;

    if (SUCCEEDED(hr))
    {
        hr = (n == nCount) ? S_OK : S_FALSE;
    }
    
    return hr;
}

// IEnumItemProperties
STDMETHODIMP CEnumItemProperties::Next( 
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ITEMPROP *rgelt,
    /* [out] */ ULONG *pceltFetched)
{
    HRESULT hr;

    if ((0 == celt) || 
        ((celt > 1) && (NULL == pceltFetched)) ||
        (NULL == rgelt))
    {
        return E_INVALIDARG;
    }

    DWORD nFetched;

    hr = CopyRange(m_nCurrent, celt, rgelt, &nFetched);

    m_nCurrent += nFetched;

    if (pceltFetched)
    {
        *pceltFetched = nFetched;
    }

    return hr;
}

STDMETHODIMP CEnumItemProperties::Skip( 
    /* [in] */ ULONG celt)
{
    HRESULT hr;
    
    m_nCurrent += celt;

    if (m_nCurrent > (m_nCount - 1))
    {
        m_nCurrent = m_nCount;  //  Passed the last one
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }
    
    return hr;
}

STDMETHODIMP CEnumItemProperties::Reset()
{
    m_nCurrent = 0;

    return S_OK;
}

STDMETHODIMP CEnumItemProperties::Clone( 
    /* [out] */ IEnumItemProperties **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppenum = NULL;

    CEnumItemProperties *peip = new CEnumItemProperties;

    if (NULL != peip)
    {
        peip->m_pItemProps = new ITEMPROP[m_nCount];

        if (NULL != peip->m_pItemProps)
        {
            ULONG nFetched;

            hr = E_FAIL;

            peip->m_nCount = m_nCount;
            hr = CopyRange(0, m_nCount, peip->m_pItemProps, &nFetched);

            if (SUCCEEDED(hr))
            {
                ASSERT(m_nCount == nFetched);

                if (m_nCount == nFetched)
                {
                    hr = peip->QueryInterface(IID_IEnumItemProperties, (void **)ppenum);
                }
            }
        }
        peip->Release();
    }    
    return hr;
}

STDMETHODIMP CEnumItemProperties::GetCount( 
    /* [out] */ ULONG *pnCount)
{
    if (NULL == pnCount)
    {
        return E_INVALIDARG;
    }

    *pnCount = m_nCount;

    return S_OK;
}


CSubscriptionItem::CSubscriptionItem(const SUBSCRIPTIONCOOKIE *pCookie, HKEY hkey)
{
    ASSERT(NULL != pCookie);
    ASSERT(0 == m_dwFlags);

    m_cRef = 1;

    if (NULL != pCookie)
    {
        m_Cookie = *pCookie;
    }

    SUBSCRIPTIONITEMINFO sii;

    sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

    if ((hkey != NULL) && 
        SUCCEEDED(Read(hkey, c_wszSubscriptionInfoValue, (BYTE *)&sii, sizeof(SUBSCRIPTIONITEMINFO))))

    {
        m_dwFlags = sii.dwFlags;
    }

    DllAddRef();
}

CSubscriptionItem::~CSubscriptionItem()
{
    if (m_dwFlags & SI_TEMPORARY)
    {
        TCHAR szKey[MAX_PATH];

        if (ItemKeyNameFromCookie(&m_Cookie, szKey, ARRAYSIZE(szKey)))
        {
             SHDeleteKey(HKEY_CURRENT_USER, szKey);
        }
    }
    DllRelease();
}

HRESULT CSubscriptionItem::Read(HKEY hkeyIn, const WCHAR *pwszValueName, 
                                BYTE *pData, DWORD dwDataSize)
{
    HRESULT hr = E_FAIL;
    HKEY hkey = hkeyIn;

    ASSERT((NULL != pwszValueName) && (NULL != pData) && (0 != dwDataSize));

    if ((NULL != hkey) || OpenItemKey(&m_Cookie, FALSE, KEY_READ, &hkey))
    {
        DWORD dwType;
        DWORD dwSize = dwDataSize;

#ifdef UNICODE
        if ((RegQueryValueExW(hkey, pwszValueName, NULL, &dwType, pData, &dwSize) == ERROR_SUCCESS) &&
#else
        TCHAR szValueName[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, pwszValueName, -1, szValueName, ARRAYSIZE(szValueName), NULL, NULL);
        if ((RegQueryValueExA(hkey, szValueName, NULL, &dwType, pData, &dwSize) == ERROR_SUCCESS) &&
#endif
            (dwSize == dwDataSize) && (REG_BINARY == dwType))
        {
            hr = S_OK;
        }
        if (NULL == hkeyIn)
        {
            RegCloseKey(hkey);
        }
    }
    return hr;
}

HRESULT CSubscriptionItem::ReadWithAlloc(HKEY hkeyIn, const WCHAR *pwszValueName, 
                                         BYTE **ppData, DWORD *pdwDataSize)
{
    HRESULT hr = E_FAIL;
    HKEY hkey = hkeyIn;

    ASSERT((NULL != pwszValueName) && (NULL != ppData) && (NULL != pdwDataSize));

    if ((NULL != hkey) || OpenItemKey(&m_Cookie, FALSE, KEY_READ, &hkey))
    {
        DWORD dwType;
        DWORD dwSize = 0;

#ifdef UNICODE
        if (RegQueryValueExW(hkey, pwszValueName, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
#else
        TCHAR szValueName[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, pwszValueName, -1, szValueName, ARRAYSIZE(szValueName), NULL, NULL);
        if (RegQueryValueExA(hkey, szValueName, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
#endif
        {
            BYTE *pData = new BYTE[dwSize];
            *pdwDataSize = dwSize;

            if (NULL != pData)
            {
#ifdef UNICODE
                if ((RegQueryValueExW(hkey, pwszValueName, NULL, &dwType, pData, pdwDataSize) == ERROR_SUCCESS) &&
#else
                if ((RegQueryValueExA(hkey, szValueName, NULL, &dwType, pData, pdwDataSize) == ERROR_SUCCESS) &&
#endif
                    (dwSize == *pdwDataSize) && (REG_BINARY == dwType))
                {
                    *ppData = pData;
                    hr = S_OK;
                }
                else
                {
                    delete [] pData;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        if (NULL == hkeyIn)
        {
            RegCloseKey(hkey);
        }
    }
    return hr;
}

HRESULT CSubscriptionItem::Write(HKEY hkeyIn, const WCHAR *pwszValueName, 
                                 BYTE *pData, DWORD dwDataSize)
{
    HRESULT hr = E_FAIL;
    HKEY hkey = hkeyIn;

    ASSERT((NULL != pwszValueName) && (NULL != pData) && (0 != dwDataSize));

    if ((NULL != hkey) || OpenItemKey(&m_Cookie, FALSE, KEY_WRITE, &hkey))
    {
#ifdef UNICODE
        if (RegSetValueExW(hkey, pwszValueName, 0, REG_BINARY, pData, dwDataSize) == ERROR_SUCCESS)
#else
        TCHAR szValueName[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, pwszValueName, -1, szValueName, ARRAYSIZE(szValueName), NULL, NULL);
        if (RegSetValueExA(hkey, szValueName, 0, REG_BINARY, pData, dwDataSize) == ERROR_SUCCESS)
#endif
        {
            hr = S_OK;
        }
        if (NULL == hkeyIn)
        {
            RegCloseKey(hkey);
        }
    }
    return hr;
}


STDMETHODIMP CSubscriptionItem::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;

    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }

    if ((IID_IUnknown == riid) || (IID_ISubscriptionItem == riid))
    {
        *ppv = (ISubscriptionItem *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    
    return hr;
}


STDMETHODIMP_(ULONG) CSubscriptionItem::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSubscriptionItem::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CSubscriptionItem::GetCookie(SUBSCRIPTIONCOOKIE *pCookie)
{
    if (NULL == pCookie)
    {
        return E_INVALIDARG;
    }

    *pCookie = m_Cookie;

    return S_OK;
}

STDMETHODIMP CSubscriptionItem::GetSubscriptionItemInfo( 
    /* [out] */ SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo)
{
    HRESULT hr;

    if ((NULL == pSubscriptionItemInfo) ||
        (pSubscriptionItemInfo->cbSize < sizeof(SUBSCRIPTIONITEMINFO)))
    {
        return E_INVALIDARG;
    }

    hr = Read(NULL, c_wszSubscriptionInfoValue, (BYTE *)pSubscriptionItemInfo, sizeof(SUBSCRIPTIONITEMINFO));

    ASSERT(sizeof(SUBSCRIPTIONITEMINFO) == pSubscriptionItemInfo->cbSize);
    
    if (SUCCEEDED(hr) &&
        (sizeof(SUBSCRIPTIONITEMINFO) != pSubscriptionItemInfo->cbSize))
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

STDMETHODIMP CSubscriptionItem::SetSubscriptionItemInfo( 
    /* [in] */ const SUBSCRIPTIONITEMINFO *pSubscriptionItemInfo)
{
    if ((NULL == pSubscriptionItemInfo) ||
        (pSubscriptionItemInfo->cbSize < sizeof(SUBSCRIPTIONITEMINFO)))
    {
        return E_INVALIDARG;
    }

    m_dwFlags = pSubscriptionItemInfo->dwFlags;
    
    return Write(NULL, c_wszSubscriptionInfoValue, (BYTE *)pSubscriptionItemInfo, sizeof(SUBSCRIPTIONITEMINFO));
}

STDMETHODIMP CSubscriptionItem::ReadProperties( 
    ULONG nCount,
    /* [size_is][in] */ const LPCWSTR rgwszName[],
    /* [size_is][out] */ VARIANT rgValue[])
{
    HRESULT hr = S_OK;
    
    if ((0 == nCount) || (NULL == rgwszName) || (NULL == rgValue))
    {
        return E_INVALIDARG;
    }

    HKEY hkey;

    if (OpenItemKey(&m_Cookie, FALSE, KEY_READ, &hkey))
    {
        for (ULONG i = 0; (i < nCount) && (S_OK == hr); i++)
        {
            BYTE *pData;
            DWORD dwDataSize;

            if (StrCmpIW(rgwszName[i], c_szPropPassword) == 0)
            {
                if (SUCCEEDED(ReadPassword(this, &rgValue[i].bstrVal)))
                {
                    rgValue[i].vt = VT_BSTR;
                }
                else
                {
                    rgValue[i].vt = VT_EMPTY;
                }
            }
            else
            {
                HRESULT hrRead = ReadWithAlloc(hkey, rgwszName[i], &pData, &dwDataSize);

                if (SUCCEEDED(hrRead))
                {
                    hr = SignatureBlobToVariant(pData, dwDataSize, &rgValue[i]);
                    delete [] pData;
                }
                else
                {
                    rgValue[i].vt = VT_EMPTY;
                }
            }
        }
        RegCloseKey(hkey);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CSubscriptionItem::WriteProperties( 
    ULONG nCount,
    /* [size_is][in] */ const LPCWSTR rgwszName[],
    /* [size_is][in] */ const VARIANT rgValue[])
{
    HRESULT hr = S_OK;
    
    if ((0 == nCount) || (NULL == rgwszName) || (NULL == rgValue))
    {
        return E_INVALIDARG;
    }

    HKEY hkey;

    if (OpenItemKey(&m_Cookie, FALSE, KEY_WRITE, &hkey))
    {
        for (ULONG i = 0; (i < nCount) && (S_OK == hr); i++)
        {
            if (rgValue[i].vt == VT_EMPTY)
            {
                //  We don't actually care if this fails since it is
                //  meant to delete the property anyhow
#ifdef UNICODE
                RegDeleteValueW(hkey, rgwszName[i]);
#else
                TCHAR szValueName[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, rgwszName[i], -1, szValueName, 
                                    ARRAYSIZE(szValueName), NULL, NULL);
                RegDeleteValueA(hkey, szValueName);
#endif
            }
            else
            {
                BYTE *pData;
                DWORD dwDataSize;

                //  Special case the name property for easy viewing
                if ((VT_BSTR == rgValue[i].vt) && 
                    (StrCmpIW(rgwszName[i], c_szPropName) == 0))
                {   
                #ifdef UNICODE
                    RegSetValueExW(hkey, NULL, 0, REG_SZ, (LPBYTE)rgValue[i].bstrVal, 
                                   (lstrlenW(rgValue[i].bstrVal) + 1) * sizeof(WCHAR));
                #else
                    TCHAR szValueName[MAX_PATH];
                    WideCharToMultiByte(CP_ACP, 0, rgValue[i].bstrVal, -1, szValueName, 
                                        ARRAYSIZE(szValueName), NULL, NULL);
                    RegSetValueExA(hkey, NULL, 0, REG_SZ, (LPBYTE)szValueName, lstrlenA(szValueName) + 1);
                #endif
                }

                if (StrCmpIW(rgwszName[i], c_szPropPassword) == 0)
                {
                    if (VT_BSTR == rgValue[i].vt)
                    {
                        hr = WritePassword(this, rgValue[i].bstrVal);
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    hr = VariantToSignatureBlob(&rgValue[i], &pData, &dwDataSize);

                    if (SUCCEEDED(hr))
                    {
                        hr = Write(hkey, rgwszName[i], pData, dwDataSize);
                        delete [] pData;
                    }
                }
            }
        }
        RegCloseKey(hkey);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CSubscriptionItem::EnumProperties( 
    /* [out] */ IEnumItemProperties **ppEnumItemProperties)
{
    HRESULT hr;

    if (NULL == ppEnumItemProperties)
    {
        return E_INVALIDARG;
    }
    CEnumItemProperties *peip = new CEnumItemProperties;

    *ppEnumItemProperties = NULL;

    if (NULL != peip)
    {
        hr = peip->Initialize(&m_Cookie, this);
        if (SUCCEEDED(hr))
        {
            hr = peip->QueryInterface(IID_IEnumItemProperties, (void **)ppEnumItemProperties);
        }
        peip->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CSubscriptionItem::NotifyChanged()
{
    HRESULT hr;

    // Notify the shell folder of the updated item
    // This is SOMEWHAT inefficient... why do we need 1000 properties for a pidl?
    // Why do we copy them around? Why why why?

    OOEBuf      ooeBuf;
    LPMYPIDL    newPidl = NULL;
    DWORD       dwSize = 0;

    memset(&ooeBuf, 0, sizeof(ooeBuf));

    hr = LoadWithCookie(NULL, &ooeBuf, &dwSize, &m_Cookie);

    if (SUCCEEDED(hr))
    {
        newPidl = COfflineFolderEnum::NewPidl(dwSize);
        if (newPidl)
        {
            CopyToMyPooe(&ooeBuf, &(newPidl->ooe));
            _GenerateEvent(SHCNE_UPDATEITEM, (LPITEMIDLIST)newPidl, NULL);
            COfflineFolderEnum::FreePidl(newPidl);
        }
    }

    return hr;
}
