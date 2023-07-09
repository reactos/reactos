#include "pch.h"
#include "dlshell.h"
#pragma hdrstop

STDAPI DataObj_SetDWORD(IDataObject *pdtobj, UINT cf, DWORD dw)
{
    HRESULT hres = E_OUTOFMEMORY;
    DWORD *pdw = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD));
    if (pdw)
    {
        *pdw = dw;
        hres = DataObj_SetGlobal(pdtobj, cf, pdw);

        if (FAILED(hres))
            GlobalFree((HGLOBAL)pdw);
    }
    return hres;
}

STDAPI DataObj_GetDWORD(IDataObject *pdtobj, UINT cf, DWORD *pdwOut)
{
    STGMEDIUM medium;
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hres = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hres))
    {
        DWORD *pdw = (DWORD *)GlobalLock(medium.hGlobal);
        if (pdw)
        {
            *pdwOut = *pdw;
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hres = E_UNEXPECTED;
        }

        ReleaseStgMedium(&medium);
    }

    return hres;
}

STDAPI DataObj_SetPasteSucceeded(IDataObject *pdtobj, DWORD dwEffect)
{
    return DataObj_SetDWORD(pdtobj, g_cfPasteSucceeded, dwEffect);
}

STDAPI_(DWORD) DataObj_GetPerformedEffect(IDataObject *pdtobj)
{
    DWORD dw = 0;

    DataObj_GetDWORD(pdtobj, g_cfPerformedDropEffect, &dw);
    return dw;
}

STDAPI DataObj_SetPerformedEffect(IDataObject *pdtobj, DWORD dwEffect)
{
    return DataObj_SetDWORD(pdtobj, g_cfPerformedDropEffect, dwEffect);
}

STDAPI DataObj_SetPreferredEffect(IDataObject *pdtobj, DWORD dwEffect)
{
    return DataObj_SetDWORD(pdtobj, g_cfPreferredDropEffect, dwEffect);
}

STDAPI_(DWORD) DataObj_GetPreferredEffect(IDataObject *pdtobj, DWORD dwDefault)
{
    DWORD dw;

    if (SUCCEEDED(DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, &dw)))
        return dw;

    return dwDefault;
}

STDAPI DataObj_SetGlobal(IDataObject *pdtobj, UINT cf, HGLOBAL hGlobal)
{
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hGlobal;
    medium.pUnkForRelease = NULL;

    // give the data object ownership of ths
    return pdtobj->SetData(&fmte, &medium, TRUE);
}

STDAPI DataObj_SetDropTarget(IDataObject *pdtobj, const CLSID *pclsid)
{
    CLSID *pdata = (CLSID *)GlobalAlloc(GPTR, SIZEOF(*pclsid));
    if (pdata)
    {
        *pdata = *pclsid;
        HRESULT hres = DataObj_SetGlobal(pdtobj, g_cfTargetCLSID, pdata);
        if (FAILED(hres))
            GlobalFree(pdata);
        return hres;
    }
    return E_OUTOFMEMORY;
}


STDAPI DataObj_SetPoints(IDataObject *pdtobj, LPFNCIDLPOINTS pfnPts, LPARAM lParam, SCALEINFO *psi)
{
#if 1
    return E_FAIL;
#else
    HRESULT hres = E_OUTOFMEMORY;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        POINT *ppt = (POINT *)GlobalAlloc(GPTR, SIZEOF(POINT) * (1 + pida->cidl));
        if (ppt)
        {
            UINT i;
            POINT ptOrigin;
            // Grab the anchor point
            pfnPts(NULL, &ptOrigin, lParam);
            ppt[0] = ptOrigin;

            for (i = 1; i <= pida->cidl; i++)
            {
                LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, i - 1);

                pfnPts(pidl, &ppt[i], lParam);
                ppt[i].x -= ptOrigin.x;
                ppt[i].y -= ptOrigin.y;
                ppt[i].x = (ppt[i].x * psi->xMul) / psi->xDiv;
                ppt[i].y = (ppt[i].y * psi->yMul) / psi->yDiv;
            }

            hres = DataObj_SetGlobal(pdtobj, g_cfOFFSETS, ppt);
            if (FAILED(hres))
                GlobalFree((HGLOBAL)ppt);
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return hres;
#endif
}



STDAPI_(LPIDA) DataObj_GetHIDA(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (pmedium)
    {
        pmedium->pUnkForRelease = NULL;
        pmedium->hGlobal = NULL;
    }

    if (!pmedium)
    {
        if (SUCCEEDED(pdtobj->QueryGetData(&fmte)))
            return (LPIDA)TRUE;
        else
            return (LPIDA)FALSE;
    }
    else if (SUCCEEDED(pdtobj->GetData(&fmte, pmedium)))
    {
        return (LPIDA)GlobalLock(pmedium->hGlobal);
    }

    return NULL;
}


STDAPI_(void) ReleaseStgMediumHGLOBAL(void *pv, STGMEDIUM *pmedium)
{
    if (pmedium->hGlobal && (pmedium->tymed == TYMED_HGLOBAL))
    {
#ifdef DEBUG
        if (pv)
        {
            void *pvT = (void *)GlobalLock(pmedium->hGlobal);
            ASSERT(pvT == pv);
            GlobalUnlock(pmedium->hGlobal);
        }
#endif
        GlobalUnlock(pmedium->hGlobal);
    }
    else
    {
        ASSERT(0);
    }

    ReleaseStgMedium(pmedium);
}

STDAPI_(void) HIDA_ReleaseStgMedium(LPIDA pida, STGMEDIUM * pmedium)
{
    ReleaseStgMediumHGLOBAL((void *)pida, pmedium);
}

STDAPI_(UINT) DataObj_GetHIDACount(IDataObject *pdtobj)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        UINT count = pida->cidl;

        ASSERT(pida->cidl == HIDA_GetCount(medium.hGlobal));

        HIDA_ReleaseStgMedium(pida, &medium);
        return count;
    }
    return 0;
}


STDAPI DataObj_SaveToFile(IDataObject *pdtobj, UINT cf, LONG lindex, LPCTSTR pszFile, DWORD dwFileSize)
{
    STGMEDIUM medium;
    FORMATETC fmte;
    HRESULT hres;

    fmte.cfFormat = cf;
    fmte.ptd = NULL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex = lindex;
    fmte.tymed = TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE;

    hres = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hres))
    {
        switch (medium.tymed) {
        case TYMED_HGLOBAL:
        {
            HANDLE hfile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
            if (hfile != INVALID_HANDLE_VALUE)
            {
                DWORD dwWrite;
                if (!WriteFile(hfile, GlobalLock(medium.hGlobal), dwFileSize ? dwFileSize : GlobalSize(medium.hGlobal), &dwWrite, NULL))
                    hres = HRESULT_FROM_WIN32(GetLastError());

                GlobalUnlock(medium.hGlobal);
                CloseHandle(hfile);

                if (FAILED(hres))
                    EVAL(DeleteFile(pszFile));
            }
            else
            {
                hres = HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }

        case TYMED_ISTREAM:
        {
            IStream *pstm;
            hres = SHCreateStreamOnFile(pszFile, OF_CREATE | OF_WRITE | OF_SHARE_DENY_WRITE, &pstm);
            if (SUCCEEDED(hres))
            {
                const ULARGE_INTEGER ul = {(UINT)-1, (UINT)-1};    // the whole thing

                hres = medium.pstm->CopyTo(pstm, ul, NULL, NULL);
                pstm->Release();

                if (FAILED(hres))
                    EVAL(DeleteFile(pszFile));
            }
            break;
        }

        case TYMED_ISTORAGE:
        {
            WCHAR wszNewFile[MAX_PATH];
            IStorage *pstg;

            SHTCharToUnicode(pszFile, wszNewFile, ARRAYSIZE(wszNewFile));
            hres = StgCreateDocfile(wszNewFile,
                            STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                            0, &pstg);
            if (SUCCEEDED(hres))
            {
                hres = medium.pstg->CopyTo(0, NULL, NULL, pstg);

                pstg->Commit(STGC_OVERWRITE);
                pstg->Release();

                if (FAILED(hres))
                    EVAL(DeleteFile(pszFile));
            }
        }
            break;

        default:
            AssertMsg(FALSE, TEXT("got typed that I didn't ask for %d"), medium.tymed);
        }

        if (SUCCEEDED(hres))
        {
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszFile, NULL);
            SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, pszFile, NULL);
        }

        ReleaseStgMedium(&medium);
    }
    return hres;
}

STDAPI DataObj_GetShellURL(IDataObject *pdtobj, STGMEDIUM *pmedium, LPCSTR *ppszURL)
{
    FORMATETC fmte = {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hres;

    if (pmedium)
    {
        hres = pdtobj->GetData(&fmte, pmedium);
        if (SUCCEEDED(hres))
            *ppszURL = (LPCSTR)GlobalLock(pmedium->hGlobal);
    }
    else
        hres = pdtobj->QueryGetData(&fmte); // query only

    return hres;
}


STDAPI DataObj_GetOFFSETs(IDataObject *pdtobj, POINT *ppt)
{
    STGMEDIUM medium;
    FORMATETC fmt = {g_cfOFFSETS, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    ASSERT(g_cfOFFSETS);
    ASSERT(ppt);
    ppt->x = ppt->y = 0;
    HRESULT hres = pdtobj->GetData(&fmt, &medium);
    if (SUCCEEDED(hres))
    {
        POINT * pptTemp = (POINT *)GlobalLock(medium.hGlobal);
        if (pptTemp)
        {
            *ppt = *pptTemp;
            GlobalUnlock(medium.hGlobal);
        }
        else
            hres = E_UNEXPECTED;
        ReleaseStgMedium(&medium);
    }
    return hres;
}


