/*****************************************************************************\
    FILE: encoding.cpp
    
    DESCRIPTION:
        Handle taking internet strings by detecting if they are UTF-8 encoded
    or DBCS and finding out what code page was used.
\*****************************************************************************/

#include "priv.h"
#include "util.h"
#include "ftpurl.h"
#include "statusbr.h"
#include <commctrl.h>
#include <shdocvw.h>


/*****************************************************************************\
    CLASS: CMultiLanguageCache
\*****************************************************************************/


HRESULT CMultiLanguageCache::_Init(void)
{
    if (m_pml2)
        return S_OK;

    return CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void **) &m_pml2);
}


/*****************************************************************************\
    CLASS: CWireEncoding
\*****************************************************************************/
CWireEncoding::CWireEncoding(void)
{
    // We can go on the stack, so we may not be zero inited.
    m_nConfidence = 0;
    m_uiCodePage = CP_ACP;     // 
    m_dwMode = 0;

    m_fUseUTF8 = FALSE;
}


CWireEncoding::~CWireEncoding(void)
{
}


void CWireEncoding::_ImproveAccuracy(CMultiLanguageCache * pmlc, LPCWIRESTR pwStr, BOOL fUpdateCP, UINT * puiCodePath)
{
    DetectEncodingInfo dei = {0};
    INT nStructs = 1;
    INT cchSize = lstrlenA(pwStr);
    IMultiLanguage2 * pml2 = pmlc->GetIMultiLanguage2();

    // Assume we will use the normal code page.
    *puiCodePath = m_uiCodePage;
    if (S_OK == pml2->DetectInputCodepage(MLDETECTCP_8BIT, CP_AUTO, (LPWIRESTR)pwStr, &cchSize, &dei, (INT *)&nStructs))
    {
        // Is it UTF8 or just plain ansi(CP_20127)?
        if (((CP_UTF_8 == dei.nCodePage) || (CP_20127 == dei.nCodePage)) &&
            (dei.nConfidence > 70))
        {
            // Yes, so make sure the caller uses UTF8 to decode but don't update
            // the codepage.
            *puiCodePath = CP_UTF_8;
        }
        else
        {
            if (fUpdateCP && (dei.nConfidence > m_nConfidence))
            {
                m_uiCodePage = dei.nCodePage;
                m_nConfidence = dei.nConfidence;
            }
        }
    }
}


HRESULT CWireEncoding::WireBytesToUnicode(CMultiLanguageCache * pmlc, LPCWIRESTR pwStr, DWORD dwFlags, LPWSTR pwzDest, DWORD cchSize)
{
    HRESULT hr;

    // Optimize for the fast common case.
    if (Is7BitAnsi(pwStr))
    {
        pwzDest[0] = 0;
        SHAnsiToUnicodeCP(CP_UTF_8, pwStr, pwzDest, cchSize);
        hr = S_OK;
    }
    else
    {
#ifdef FEATURE_CP_AUTODETECT
        if (this)
        {
            CMultiLanguageCache mlcTemp;
            UINT cchSizeTemp = cchSize;
            UINT uiCodePageToUse;

            if (!pmlc)
                pmlc = &mlcTemp;

            if (!pmlc || !pmlc->GetIMultiLanguage2())
                return E_FAIL;

            IMultiLanguage2 * pml2 = pmlc->GetIMultiLanguage2();
            _ImproveAccuracy(pmlc, pwStr, (WIREENC_IMPROVE_ACCURACY & dwFlags), &uiCodePageToUse);
            if (CP_ACP == uiCodePageToUse)
                uiCodePageToUse = GetACP();

            UINT cchSrcSize = lstrlenA(pwStr) + 1; // The need to do the terminator also.
            hr = pml2->ConvertStringToUnicode(&m_dwMode, uiCodePageToUse, (LPWIRESTR)pwStr, &cchSrcSize, pwzDest, &cchSizeTemp);
            if (!(EVAL(S_OK == hr)))
                SHAnsiToUnicode(pwStr, pwzDest, cchSize);

        }
        else
#endif // FEATURE_CP_AUTODETECT
        {
            UINT uiCodePage = ((WIREENC_USE_UTF8 & dwFlags) ? CP_UTF_8 : CP_ACP);

            SHAnsiToUnicodeCP(uiCodePage, pwStr, pwzDest, cchSize);
        }
    }

    return hr;
}


HRESULT CWireEncoding::UnicodeToWireBytes(CMultiLanguageCache * pmlc, LPCWSTR pwzStr, DWORD dwFlags, LPWIRESTR pwDest, DWORD cchSize)
{
    HRESULT hr = S_OK;

#ifdef FEATURE_CP_AUTODETECT
    CMultiLanguageCache mlcTemp;
    DWORD dwCodePage = CP_UTF_8;
    DWORD dwModeTemp = 0;
    DWORD * pdwMode = &dwModeTemp;
    UINT cchSizeTemp = cchSize;

    // In some cases, we don't know the site, so we use this.
    // BUGBUG: Come back and force this to be set.
    if (this)
    {
        dwCodePage = m_uiCodePage;
        pdwMode = &m_dwMode;
    }

    if (!pmlc)
        pmlc = &mlcTemp;

    if (!pmlc)
        return E_FAIL;

    IMultiLanguage2 * pml2 = pmlc->GetIMultiLanguage2();
//    if (WIREENC_USE_UTF8 & dwFlags)
//        dwCodePage = CP_UTF_8;

    UINT cchSrcSize = lstrlenW(pwzStr) + 1; // The need to do the terminator also.
    if (CP_ACP == dwCodePage)
        dwCodePage = GetACP();

    hr = pml2->ConvertStringFromUnicode(pdwMode, dwCodePage, (LPWSTR) pwzStr, &cchSrcSize, pwDest, &cchSizeTemp);
    if (!(EVAL(S_OK == hr)))
        SHUnicodeToAnsi(pwzStr, pwDest, cchSize);

#else // FEATURE_CP_AUTODETECT
    UINT nCodePage = ((WIREENC_USE_UTF8 & dwFlags) ? CP_UTF_8 : CP_ACP);

    SHUnicodeToAnsiCP(nCodePage, pwzStr, pwDest, cchSize);
#endif // FEATURE_CP_AUTODETECT

    return hr;
}



HRESULT CWireEncoding::ReSetCodePages(CMultiLanguageCache * pmlc, CFtpPidlList * pFtpPidlList)
{
    CMultiLanguageCache mlcTemp;
    
    if (!pmlc)
        pmlc = &mlcTemp;
    
    if (!pmlc)
        return E_FAIL;

    // BUGBUG/TODO:
    return S_OK;
}


HRESULT CWireEncoding::CreateFtpItemID(CMultiLanguageCache * pmlc, LPFTP_FIND_DATA pwfd, LPITEMIDLIST * ppidl)
{
    CMultiLanguageCache mlcTemp;
    WCHAR wzDisplayName[MAX_PATH];
    
    if (!pmlc)
        pmlc = &mlcTemp;

    WireBytesToUnicode(pmlc, pwfd->cFileName, (m_fUseUTF8 ? WIREENC_USE_UTF8 : WIREENC_NONE), wzDisplayName, ARRAYSIZE(wzDisplayName));
    return FtpItemID_CreateReal(pwfd, wzDisplayName, ppidl);
}


HRESULT CWireEncoding::ChangeFtpItemIDName(CMultiLanguageCache * pmlc, LPCITEMIDLIST pidlBefore, LPCWSTR pwzNewName, BOOL fUTF8, LPITEMIDLIST * ppidlAfter)
{
    CMultiLanguageCache mlcTemp;
    WIRECHAR wWireName[MAX_PATH];
    HRESULT hr;

    if (!pmlc)
        pmlc = &mlcTemp;

    hr = UnicodeToWireBytes(pmlc, pwzNewName, (fUTF8 ? WIREENC_USE_UTF8 : WIREENC_NONE), wWireName, ARRAYSIZE(wWireName));
    if (EVAL(SUCCEEDED(hr)))
        hr = FtpItemID_CreateWithNewName(pidlBefore, pwzNewName, wWireName, ppidlAfter);

    return hr;
}




BOOL SHIsUTF8Encoded(LPCWIRESTR pszIsUTF8)
{
    unsigned int len = lstrlenA(pszIsUTF8);
    LPCWIRESTR endbuf = pszIsUTF8 + len;
    unsigned char byte2mask = 0x00;
    unsigned char c;
    int trailing = 0;               // trailing (continuation) bytes to follow
                             
     while (pszIsUTF8 != endbuf)
     {
         c = *pszIsUTF8++;
         if (trailing)
         {
             if ((c & 0xC0) == 0x80)    // Does trailing byte follow UTF-8 format?
             {
                 if (byte2mask)     // Need to check 2nd byte for proper range?
                 {
                     if (c & byte2mask)      // Are appropriate bits set?
                         byte2mask=0x00;
                     else
                         return 0;

                     trailing--;
                 }
             }
             else
                 return FALSE;
         }
         else
         {
             if ((c & 0x80) == 0x00)
                 continue;         // valid 1 byte UTF-8
             else
             {
                 if ((c & 0xE0) == 0xC0) // valid 2 byte UTF-8
                 {
                    if (c & 0x1E)      // Is UTF-8 byte in proper range?
                    {
                        trailing =1;
                    }
                    else
                        return FALSE;
                 }
                 else
                 {
                     if ((c & 0xF0) == 0xE0)               // valid 3 byte UTF-8
                     {
                         if (!(c & 0x0F))                          // Is UTF-8 byte in proper range?
                            byte2mask=0x20;                    // If not set mask to check next byte
                         trailing = 2;
                     }
                     else
                     {
                         if ((c & 0xF8) == 0xF0)               // valid 4 byte UTF-8
                         {
                             if (!(c & 0x07))                          // Is UTF-8 byte in proper range?
                                byte2mask=0x30;                    // If not set mask to check next byte
                             trailing = 3;
                         }
                         else
                         {
                             if ((c & 0xFC) == 0xF8)               // valid 5 byte UTF-8
                             {
                                 if (!(c & 0x03))                          // Is UTF-8 byte in proper range?
                                    byte2mask=0x38;                    // If not set mask to check next byte

                                 trailing = 4;
                             }
                             else
                             {
                                 if ((c & 0xFE) == 0xFC)               // valid 6 byte UTF-8
                                 {
                                     if (!(c & 0x01))                          // Is UTF-8 byte in proper range?
                                        byte2mask=0x3C;                    // If not set mask to check next byte

                                     trailing = 5;
                                 }
                                 else
                                     return FALSE;
                             }
                         }
                     }
                 }
             }
         }
     }

     return (trailing == 0);
 }
