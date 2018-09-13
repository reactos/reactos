#include "private.h"
#include "convobj.h"

#include "detcbase.h"
#include "codepage.h"
#include "detcjpn.h"
#include "detckrn.h"


CMLangConvertCharset::CMLangConvertCharset(void)
{
    lpCharConverter = NULL ;

    m_dwSrcEncoding = 0 ;
    m_dwDetectSrcEncoding = 0 ;
    m_dwDstEncoding = 0 ;
    m_dwMode = 0 ;

    return ;
}

CMLangConvertCharset::~CMLangConvertCharset(void)
{
    if (lpCharConverter)
        delete lpCharConverter ;
    return ;
}

//
//  CMLangConvertCharset implementation
//
STDAPI CMLangConvertCharset::Initialize(UINT uiSrcCodePage, UINT uiDstCodePage, DWORD dwProperty)
{
    HRESULT hr = S_OK ;

    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::Initialize called."));

    if ( m_dwSrcEncoding != uiSrcCodePage ||
            m_dwDstEncoding != uiDstCodePage )
    {
        m_dwSrcEncoding = uiSrcCodePage ;
        m_dwDstEncoding = uiDstCodePage ;

        if (lpCharConverter)
            delete lpCharConverter ;

        lpCharConverter = new CICharConverter ;

        if (!lpCharConverter)
            return E_FAIL ;

        hr = lpCharConverter->ConvertSetup(m_dwSrcEncoding, m_dwDstEncoding);
    }

    m_dwMode = 0 ;
    m_dwProperty = dwProperty ;

    return hr ;
}

STDAPI CMLangConvertCharset::GetSourceCodePage(UINT *puiSrcCodePage)
{
    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::GetSourceCodePage called."));

    if (puiSrcCodePage)
    {
        *puiSrcCodePage = m_dwSrcEncoding ;
        return S_OK ;
    }
    else
        return E_INVALIDARG ;
}

STDAPI CMLangConvertCharset::GetDestinationCodePage(UINT *puiDstCodePage)
{
    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::GetDestinationCodePage called."));

    if (puiDstCodePage)
    {
        *puiDstCodePage = m_dwDstEncoding ;
        return S_OK ;
    }
    else
        return E_INVALIDARG ;
}

STDAPI CMLangConvertCharset::GetDeterminedSrcCodePage(UINT *puiCodePage)
{
    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::GetDeterminedSrcCodePage called."));

    if (m_dwDetectSrcEncoding)
    {
        if (puiCodePage)
        {
            *puiCodePage = m_dwDetectSrcEncoding;
            return S_OK ;
        }
        else
            return E_INVALIDARG ;
    }
    else
        return S_FALSE ;
}

STDAPI CMLangConvertCharset::GetProperty(DWORD *pdwProperty)
{
    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::GetProperty called."));

    if (pdwProperty)
    {
        *pdwProperty = m_dwProperty;
        return S_OK ;
    }
    else
        return E_INVALIDARG ;
}

STDAPI CMLangConvertCharset::DoConversion(BYTE *pSrcStr, UINT *pcSrcSize, BYTE *pDstStr, UINT *pcDstSize)
{
    HRESULT hr ;
    DWORD dwMode = m_dwMode ;
    int nSrcSize = -1 ;
    int nDstSize = 0 ;

    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::DoConversion called."));

    // no converter was set up 
    if (!lpCharConverter)
        return E_FAIL ;

    if (pcSrcSize)
        nSrcSize = *pcSrcSize ;

    if ( pSrcStr && nSrcSize == -1 ) // Get length of lpSrcStr if not given, assuming lpSrcStr is a zero terminate string.
    {
        if ( m_dwSrcEncoding == CP_UCS_2 )
    		nSrcSize = (lstrlenW( (WCHAR*) pSrcStr) << 1 ) ;
        else
    		nSrcSize = lstrlenA( (CHAR*) pSrcStr) ;
    }

    if (pcDstSize)
        nDstSize = *pcDstSize ;

    if ( m_dwSrcEncoding == CP_JP_AUTO ) // Auto Detection for Japan
    {
        CIncdJapanese DetectJapan;
        DWORD dwSrcEncoding ;

        dwSrcEncoding = DetectJapan.DetectStringA((LPSTR)pSrcStr, nSrcSize);
        // if dwSrcEncoding is zero means there is an ambiguity, we don't return
        // the detected codepage to caller, instead we defaut its codepage internally
        // to SJIS
        if (dwSrcEncoding)
        {
            m_dwDetectSrcEncoding = m_dwSrcEncoding = dwSrcEncoding ;
            m_dwProperty |= MLCONVCHARF_AUTODETECT ;
        }
        else
            dwSrcEncoding = CP_JPN_SJ;
        hr = lpCharConverter->ConvertSetup(dwSrcEncoding, m_dwDstEncoding);
        if ( hr != S_OK )
            return hr ;
    }
    else if ( m_dwSrcEncoding == CP_KR_AUTO ) // Auto Detection for Korean
    {
        CIncdKorean DetectKorean;

        m_dwDetectSrcEncoding = m_dwSrcEncoding = DetectKorean.DetectStringA((LPSTR)pSrcStr, nSrcSize);
        hr = lpCharConverter->ConvertSetup(m_dwSrcEncoding, m_dwDstEncoding);
        if ( hr != S_OK )
            return hr ;
        m_dwProperty |= MLCONVCHARF_AUTODETECT ;
    }
    else if ( m_dwSrcEncoding == CP_AUTO ) // General Auto Detection for all code pages
    {
        nSrcSize = DETECTION_MAX_LEN < *pcSrcSize ?  DETECTION_MAX_LEN : *pcSrcSize;
        INT nScores = 1;
        DWORD dwSrcEncoding ;
        DetectEncodingInfo Encoding;

        if ( S_OK == _DetectInputCodepage(MLDETECTCP_HTML, 1252, (char *)pSrcStr, &nSrcSize, &Encoding, &nScores))
        {
            m_dwDetectSrcEncoding = m_dwSrcEncoding = dwSrcEncoding = Encoding.nCodePage;
            m_dwProperty |= MLCONVCHARF_AUTODETECT ;
        }
        else
        {
            dwSrcEncoding = 1252;
        }

        hr = lpCharConverter->ConvertSetup(dwSrcEncoding, m_dwDstEncoding);
        if ( hr != S_OK )
        {
            return hr ;
        }
    }

    hr = lpCharConverter->DoCodeConvert(&dwMode, (LPCSTR) pSrcStr, &nSrcSize, (LPSTR) pDstStr, &nDstSize, m_dwProperty, NULL);

    // return the number of bytes processed for the source. 
    if (pcSrcSize)
        *pcSrcSize = lpCharConverter->_nSrcSize ;

    if (pcDstSize)
        *pcDstSize = nDstSize;

    if (pDstStr)
        m_dwMode = dwMode ;

    lpCharConverter->ConvertCleanUp();
    return hr ;
}

STDAPI CMLangConvertCharset::DoConversionToUnicode(CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize)
{

    HRESULT hr ;
    UINT nByteCountSize = (pcDstSize ? *pcDstSize * sizeof(WCHAR) : 0 ) ;

    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::DoConversionToUnicode called."));

    hr = DoConversion((BYTE*)pSrcStr,pcSrcSize,(BYTE*)pDstStr,&nByteCountSize);

    if (pcDstSize)
        *pcDstSize = nByteCountSize / sizeof(WCHAR);

    return hr;
}

STDAPI CMLangConvertCharset::DoConversionFromUnicode(WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize)
{
    HRESULT hr ;
    UINT nByteCountSize = (pcSrcSize ? *pcSrcSize * sizeof(WCHAR) : 0 ) ;

    DebugMsg(DM_TRACE, TEXT("CMLangConvertCharset::DoConversionFromUnicode called."));

    hr = DoConversion((BYTE*)pSrcStr,&nByteCountSize,(BYTE*)pDstStr,pcDstSize);

    if (pcSrcSize)
        *pcSrcSize = nByteCountSize / sizeof(WCHAR);

    return hr ;
}
