#include "priv.h"
#include "ishcut.h"

STDMETHODIMP Intshcut::GetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    ZeroMemory(pstgmed, SIZEOF(*pstgmed));

    if (pfmtetc->cfFormat == g_cfURL)
        hr = TransferUniformResourceLocator(pfmtetc, pstgmed);
    else if (pfmtetc->cfFormat == CF_TEXT)
        hr = TransferText(pfmtetc, pstgmed);
    else if (pfmtetc->cfFormat == g_cfFileDescA)
        hr = TransferFileGroupDescriptorA(pfmtetc, pstgmed);
    else if (pfmtetc->cfFormat == g_cfFileDescW)
        hr = TransferFileGroupDescriptorW(pfmtetc, pstgmed);
    else if (pfmtetc->cfFormat == g_cfFileContents)
        hr = TransferFileContents(pfmtetc, pstgmed);
    else
        hr = DV_E_FORMATETC;

    return hr;
}

STDMETHODIMP Intshcut::GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed)
{
    return E_NOTIMPL;
}

STDMETHODIMP Intshcut::QueryGetData(FORMATETC *pfmtetc)
{
    HRESULT hr;

    if ((pfmtetc->cfFormat == g_cfURL) ||
        (pfmtetc->cfFormat == CF_TEXT) ||
        (pfmtetc->cfFormat == g_cfFileDescA) ||
        (pfmtetc->cfFormat == g_cfFileDescW))
    {
        hr = (pfmtetc->tymed & TYMED_HGLOBAL) ? S_OK : DV_E_TYMED;
    }
    else if (pfmtetc->cfFormat == g_cfFileContents)
    {
        if (pfmtetc->dwAspect == DVASPECT_COPY)
            hr = (pfmtetc->tymed & TYMED_ISTREAM) ? S_OK : DV_E_TYMED;
        else if ((pfmtetc->dwAspect == DVASPECT_CONTENT) || 
                 (pfmtetc->dwAspect == DVASPECT_LINK))
            hr = (pfmtetc->tymed & TYMED_HGLOBAL) ? S_OK : DV_E_TYMED;
        else
            hr = DV_E_FORMATETC;
    }
    else
        hr = DV_E_FORMATETC;

    return hr;
}

STDMETHODIMP Intshcut::GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut)
{
    HRESULT hr = QueryGetData(pfmtetcIn);
    if (hr == S_OK)
    {
        *pfmtetcOut = *pfmtetcIn;

        if (pfmtetcIn->ptd == NULL)
            hr = DATA_S_SAMEFORMATETC;
        else
        {
            pfmtetcIn->ptd = NULL;
            ASSERT(hr == S_OK);
        }
    }
    else
        ZeroMemory(pfmtetcOut, SIZEOF(*pfmtetcOut));

    return hr;
}

STDMETHODIMP Intshcut::SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease)
{
    return DV_E_FORMATETC;
}

STDMETHODIMP Intshcut::EnumFormatEtc(DWORD dwDirFlags, IEnumFORMATETC **ppiefe)
{
    if (dwDirFlags == DATADIR_GET)
    {
        FORMATETC rgfmtetc[] =
        {
            // the order here defines the accuarcy of rendering
            { g_cfFileContents, NULL, DVASPECT_LINK,  0, TYMED_HGLOBAL },
            { g_cfFileDescW,    NULL, DVASPECT_LINK, -1, TYMED_HGLOBAL },
            { g_cfFileDescA,    NULL, DVASPECT_LINK, -1, TYMED_HGLOBAL },
            { g_cfFileContents, NULL, DVASPECT_COPY,  0, TYMED_ISTREAM },
            { g_cfFileDescW,    NULL, DVASPECT_COPY, -1, TYMED_HGLOBAL },
            { g_cfFileDescA,    NULL, DVASPECT_COPY, -1, TYMED_HGLOBAL },
            { g_cfFileContents, NULL, DVASPECT_CONTENT,  0, TYMED_HGLOBAL },
            { g_cfFileDescW,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { g_cfFileDescA,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { g_cfURL,          NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { CF_TEXT,          NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };
        return SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppiefe);
    }
    *ppiefe = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP Intshcut::DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink *piadvsink, DWORD *pdwConnection)
{
    *pdwConnection = 0;
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP Intshcut::DUnadvise( DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP Intshcut::EnumDAdvise(IEnumSTATDATA **ppiesd)
{
    *ppiesd = NULL;
    return OLE_E_ADVISENOTSUPPORTED;
}

