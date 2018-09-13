#ifndef _CONVOBJ_H_
#define _CONVOBJ_H_

#ifdef  __cplusplus

#include "mlatl.h"
#include "fechrcnv.h"
#include "convbase.h"
#include "ichrcnv.h"

//
//  CMLangConvertCharset declaration with IMLangConvertCharset Interface
//
class ATL_NO_VTABLE CMLangConvertCharset :
    public CComObjectRoot,
    public CComCoClass<CMLangConvertCharset, &CLSID_CMLangConvertCharset>,
    public IMLangConvertCharset
{
public:
    CMLangConvertCharset(void);
    ~CMLangConvertCharset(void);

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLangConvertCharset)
        COM_INTERFACE_ENTRY(IMLangConvertCharset)
    END_COM_MAP()

public:
    // IMLangConvertCharset
    STDMETHOD(Initialize)(UINT uiSrcCodePage, UINT uiDstCodePage, DWORD dwProperty);
    STDMETHOD(GetSourceCodePage)(UINT *puiSrcCodePage);
    STDMETHOD(GetDestinationCodePage)(UINT *puiDstCodePage);
    STDMETHOD(GetDeterminedSrcCodePage)(UINT *puiCodePage);
    STDMETHOD(GetProperty)(DWORD *pdwProperty);
    STDMETHOD(DoConversion)(BYTE *pSrcStr, UINT *pcSrcSize, BYTE *pDstStr, UINT *pcDstSize);
    STDMETHOD(DoConversionToUnicode)(CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize);
    STDMETHOD(DoConversionFromUnicode)(WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize);

private:

    DWORD m_dwSrcEncoding;
    DWORD m_dwDetectSrcEncoding;
    DWORD m_dwDstEncoding;
    DWORD m_dwMode;
    DWORD m_dwProperty;

    CICharConverter* lpCharConverter;

};

#endif  // __cplusplus

#endif  // _CONVOBJ_H_
