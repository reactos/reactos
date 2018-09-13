#include "private.h"

//
//  Globals
//


#if 0
STDAPI EnumRfc1766Info(DWORD dwIndex, PRFC1766INFOA pRfc1766Info)
{
    EnsureRfc1766Table();

    if (NULL != pRfc1766Info)
    {
        if (dwIndex < g_cRfc1766)
        {
            *pRfc1766Info = g_pRfc1766[dwIndex];
            return S_OK;    
        }
        else
            return S_FALSE;
    }
    return E_INVALIDARG;
}    
#endif

STDAPI LcidToRfc1766A(LCID Locale, LPSTR pszRfc1766, int iMaxLength)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    if (0 < iMaxLength)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (MimeRfc1766[i].LcId == Locale)
                break;
        }
        if (i < g_cRfc1766)
        {
            if (WideCharToMultiByte(1252, 0, MimeRfc1766[i].szRfc1766, -1, pszRfc1766, iMaxLength, NULL, NULL))
                hr = S_OK;
        }
        else
        {
            CHAR sz[MAX_RFC1766_NAME];

            if (GetLocaleInfoA(Locale, LOCALE_SABBREVLANGNAME, sz, ARRAYSIZE(sz)))
            {
                CharLowerA(sz);
                if (!lstrcmpA(sz, "cht"))
                {
                    lstrcpynA(pszRfc1766, "zh-tw", iMaxLength);        
                }
                else if (!lstrcmpA(sz, "chs"))
                {
                    lstrcpynA(pszRfc1766, "zh-cn", iMaxLength);
                }
                else if (!lstrcmpA(sz, "jpn"))
                {
                    lstrcpynA(pszRfc1766, "ja", iMaxLength);
                }
                else
                {
                    sz[2] = '\0';
                    lstrcpynA(pszRfc1766, sz, iMaxLength);
                }
                hr = S_OK;
            }
            else
                hr = E_FAIL;
        }
    }
    return hr;
}    



STDAPI LcidToRfc1766W(LCID Locale, LPWSTR pwszRfc1766, int nChar)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    if (0 < nChar)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (MimeRfc1766[i].LcId == Locale)
                break;
        }
        if (i < g_cRfc1766)
        {
            MLStrCpyNW(pwszRfc1766, MimeRfc1766[i].szRfc1766, nChar);
            hr = S_OK;
        }
        else
        {
            CHAR sz[MAX_RFC1766_NAME];
            CHAR sz1[MAX_RFC1766_NAME];

            if (GetLocaleInfoA(Locale, LOCALE_SABBREVLANGNAME, sz, ARRAYSIZE(sz)))
            {
                CharLowerA(sz);
                if (!lstrcmpA(sz, "cht"))
                {
                    lstrcpynA(sz1, "zh-tw", nChar);
                }
                else if (!lstrcmpA(sz, "chs"))
                {
                    lstrcpynA(sz1, "zh-cn", nChar);
                }
                else if (!lstrcmpA(sz, "jpn"))
                {
                    lstrcpynA(sz1, "ja", nChar);
                }
                else
                {
                    sz[2] = '\0';
                    lstrcpynA(sz1, sz, nChar);
                }
                if (MultiByteToWideChar(1252, 0, sz1, lstrlen(sz1)+1, pwszRfc1766, nChar))
                    hr = S_OK;
            }
            else
                hr = E_FAIL;
        }
    }
    return hr;
}    

STDAPI Rfc1766ToLcidW(PLCID pLocale, LPCWSTR pwszRfc1766)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;
    
    if (NULL != pLocale && NULL != pwszRfc1766)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (!MLStrCmpIW(MimeRfc1766[i].szRfc1766, pwszRfc1766))
                break;
        }
        if (i < g_cRfc1766)
        {
            *pLocale = MimeRfc1766[i].LcId;
            hr = S_OK;
        }
        else
        {
            if (2 < lstrlenW(pwszRfc1766))
            {
                WCHAR sz[3];

                sz[0] = pwszRfc1766[0];
                sz[1] = pwszRfc1766[1];
                sz[2] = 0;
                for (i = 0; i < g_cRfc1766; i++)
                {
                    if (!MLStrCmpIW(MimeRfc1766[i].szRfc1766, sz))
                        break;                
                }
                if (i < g_cRfc1766)
                {
                    *pLocale = MimeRfc1766[i].LcId;
                    hr = S_FALSE;
                }
                else
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
    }
    return hr;
}

STDAPI Rfc1766ToLcidA(PLCID pLocale, LPCSTR pszRfc1766)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != pLocale && NULL != pszRfc1766)
    {
        int i;
        WCHAR sz[MAX_RFC1766_NAME];

        for (i = 0; i < MAX_RFC1766_NAME - 1; i++)
        {
            sz[i] = (WCHAR)pszRfc1766[i];
            if (0 == sz[i])
                break;
        }
        if (i == MAX_RFC1766_NAME -1)
            sz[i] = 0;

        hr = Rfc1766ToLcidW(pLocale, (LPCWSTR)sz);
    }
    return hr;
}