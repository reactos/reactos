//+---------------------------------------------------------------------
//
//   File:      cutil.hxx
//
//  Contents:   Utility functions for CSite, move out from csite.cxx
//
//------------------------------------------------------------------------

#ifndef I_CUTIL_HXX_
#define I_CUTIL_HXX_
#pragma INCMSG("--- Beg 'cutil.hxx'")

class CGenDataObject;

HRESULT ClsidParamStrFromClsid (CLSID * pClsid, LPTSTR ptszParam, int cbParam);

HRESULT ClsidParamStrFromClsidStr (LPTSTR ptszClsid, LPTSTR ptszParam, int cbParam);

HRESULT HtmlTagStrFromParam (LPTSTR lptszParam, int *pnFound);

STDAPI_(void) OleUIMetafilePictIconFree(HGLOBAL hMetaPict);

HRESULT DoInsertObjectUI (CElement * pElement, DWORD * pdwResult, LPTSTR * plptszResult);

HRESULT CreateHtmlDOFromIDM (UINT cmd, LPTSTR lptszParam, IDataObject ** ppHtmlDO);

HRESULT ObjectParamStrFromDO (IDataObject * pDO, RECT * prc, LPTSTR lptszParam, int cbParam);

HRESULT CreateLinkDataObject(const TCHAR *              pchUrl,
                             const TCHAR *              pchName,
                             IUniformResourceLocator ** ppLink);

HRESULT CopyFileToClipboard(const TCHAR * pchPath, CGenDataObject * pDO);

BOOL IsTabKey(CMessage * pMessage);
BOOL IsFrameTabKey(CMessage * pMessage);

#pragma INCMSG("--- End 'cutil.hxx'")
#else
#pragma INCMSG("*** Dup 'cutil.hxx'")
#endif
