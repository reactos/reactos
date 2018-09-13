#include "shellprv.h"
#include "datautil.h"

#include "idlcomm.h"

STDAPI DataObj_SetBlob(IDataObject *pdtobj, UINT cf, LPCVOID pvBlob, UINT cbBlob)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPVOID pv = GlobalAlloc(GPTR, cbBlob);
    if (pv)
    {
        CopyMemory(pv, pvBlob, cbBlob);
        hres = DataObj_SetGlobal(pdtobj, cf, pv);

        if (FAILED(hres))
            GlobalFree((HGLOBAL)pv);
    }
    return hres;
}

STDAPI DataObj_GetBlob(IDataObject *pdtobj, UINT cf, LPVOID pvBlob, UINT cbBlob)
{
    STGMEDIUM medium = {0};
    FORMATETC fmte = {(CLIPFORMAT) cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hres = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hres))
    {
        LPVOID pv = GlobalLock(medium.hGlobal);
        if (pv)
        {
            CopyMemory(pvBlob, pv, cbBlob);
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

STDAPI DataObj_SetDWORD(IDataObject *pdtobj, UINT cf, DWORD dw)
{
    return DataObj_SetBlob(pdtobj, cf, &dw, sizeof(DWORD));
}

STDAPI_(DWORD) DataObj_GetDWORD(IDataObject *pdtobj, UINT cf, DWORD dwDefault)
{
    DWORD dwRet;
    if (FAILED(DataObj_GetBlob(pdtobj, cf, &dwRet, sizeof(DWORD))))
        dwRet = dwDefault;
    return dwRet;
}


STDAPI DataObj_SetGlobal(IDataObject *pdtobj, UINT cf, HGLOBAL hGlobal)
{
    FORMATETC fmte = {(CLIPFORMAT) cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {0};

    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hGlobal;
    medium.pUnkForRelease = NULL;

    // give the data object ownership of ths
    return pdtobj->SetData(&fmte, &medium, TRUE);
}

STDAPI DataObj_SetDropTarget(IDataObject *pdtobj, const CLSID *pclsid)
{
    return DataObj_SetBlob(pdtobj, g_cfTargetCLSID, pclsid, sizeof(CLSID));
}

STDAPI DataObj_GetDropTarget(IDataObject *pdtobj, CLSID *pclsid)
{
    return DataObj_GetBlob(pdtobj, g_cfTargetCLSID, pclsid, sizeof(CLSID));
}

STDAPI DataObj_SetPoints(IDataObject *pdtobj, LPFNCIDLPOINTS pfnPts, LPARAM lParam, SCALEINFO *psi)
{
    HRESULT hres = E_OUTOFMEMORY;
    STGMEDIUM medium = {0};
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
    STGMEDIUM medium = {0};
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

// PERFPERF 
// This routine used to copy 512 bytes at a time, but that had a major negative perf impact.
// I have measured a 2-3x speedup in copy times by increasing this buffer size to 16k.
// Yes, its a lot of stack, but it is memory well spent.                    -saml
#define STREAM_COPY_BUF_SIZE        16384
#define STREAM_PROGRESS_INTERVAL    (100*1024/STREAM_COPY_BUF_SIZE) // display progress after this many blocks

HRESULT StreamCopyWithProgress(IStream *pstmFrom, IStream *pstmTo, ULARGE_INTEGER cb, PROGRESSINFO * ppi)
{
    BYTE buf[STREAM_COPY_BUF_SIZE];
    ULONG cbRead;
    HRESULT hres = NOERROR;
    int nSection = 0;         // How many buffer sizes have we copied?
    ULARGE_INTEGER uliNewCompleted;

    if (ppi)
    {
        uliNewCompleted.QuadPart = ppi->uliBytesCompleted.QuadPart;
    }

    while (cb.QuadPart)
    {
        if (ppi && ppi->ppd)
        {
            if (0 == (nSection % STREAM_PROGRESS_INTERVAL))
            {
                EVAL(SUCCEEDED(ppi->ppd->SetProgress64(uliNewCompleted.QuadPart, ppi->uliBytesTotal.QuadPart)));

                if (ppi->ppd->HasUserCancelled())
                {
                    hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    break;
                }
            }
        }

        hres = pstmFrom->Read(buf, min(cb.LowPart, SIZEOF(buf)), &cbRead);
        if (FAILED(hres) || (cbRead == 0))
        {
            //  sometimes we are just done.
            if (SUCCEEDED(hres))
                hres = S_OK;
            break;
        }


        if (ppi)
        {
            uliNewCompleted.QuadPart += (ULONGLONG) cbRead;
        }

        cb.QuadPart -= cbRead;

        hres = pstmTo->Write(buf, cbRead, &cbRead);
        if (FAILED(hres) || (cbRead == 0))
            break;

        nSection++;
    }

    return hres;
}


STDAPI DataObj_SaveToFile(IDataObject *pdtobj, UINT cf, LONG lindex, LPCTSTR pszFile, DWORD dwFileSize, PROGRESSINFO * ppi)
{
    STGMEDIUM medium = {0};
    FORMATETC fmte;
    HRESULT hres;

    fmte.cfFormat = (CLIPFORMAT) cf;
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
                // BUGBUG raymondc: what about writes greater than 4 GB?
                if (!WriteFile(hfile, GlobalLock(medium.hGlobal), dwFileSize ? dwFileSize : (DWORD) GlobalSize(medium.hGlobal), &dwWrite, NULL))
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

                hres = StreamCopyWithProgress(medium.pstm, pstm, ul, ppi);
                pstm->Release();

                if (FAILED(hres))
                    EVAL(DeleteFile(pszFile));

                DebugMsg(TF_FSTREE, TEXT("IStream::CopyTo() -> %x"), hres);
            }
            break;
        }

        case TYMED_ISTORAGE:
        {
            WCHAR wszNewFile[MAX_PATH];
            IStorage *pstg;

            DebugMsg(TF_FSTREE, TEXT("got IStorage"));

            SHTCharToUnicode(pszFile, wszNewFile, ARRAYSIZE(wszNewFile));
            hres = StgCreateDocfile(wszNewFile,
                            STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                            0, &pstg);
            if (SUCCEEDED(hres))
            {
                hres = medium.pstg->CopyTo(0, NULL, NULL, pstg);

                DebugMsg(TF_FSTREE, TEXT("IStorage::CopyTo() -> %x"), hres);

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
    STGMEDIUM medium = {0};
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


STDAPI_(BOOL) DataObj_CanGoAsync(IDataObject *pdtobj)
{
    BOOL fDoOpAsynch = FALSE;
    IAsyncOperation * pao;

    if (SUCCEEDED(pdtobj->QueryInterface(IID_IAsyncOperation, (void **)&pao)))
    {
        BOOL fIsOpAsync;
        if (SUCCEEDED(pao->GetAsyncMode(&fIsOpAsync)) && (TRUE == fIsOpAsync))
        {
            fDoOpAsynch = SUCCEEDED(pao->StartOperation(NULL));
        }
        pao->Release();
    }
    return fDoOpAsynch;
}


//
// HACKHACK: (reinerf) - We used to always do async drag/drop operations on NT4 by cloning the
// dataobject. Some apps (WS_FTP 6.0) rely on the async nature in order for drag/drop to work since
// they stash the return value from DoDragDrop and look at it later when their copy hook is invoked 
// by SHFileOperation(). So, we sniff the HDROP and if it has one path that contains "WS_FTPE\Notify"
// in it, then we do the operation async. 
//
STDAPI_(BOOL) DataObj_GoAsyncForCompat(IDataObject *pdtobj)
{
    BOOL bRet = FALSE;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        // is there only one path in the hdrop?
        if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0) == 1)
        {
            TCHAR szPath[MAX_PATH];

            // is it the magical WS_FTP path ("%temp%\WS_FTPE\Notify") that WS_FTP sniffs
            // for in their copy hook?
            if (DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)) &&
                StrStrI(szPath, TEXT("WS_FTPE\\Notify")))
            {
                // yes, we have to do an async operation for app compat
                TraceMsg(TF_WARNING, "DataObj_GoAsyncForCompat: found WS_FTP HDROP, doing async drag-drop");
                bRet = TRUE;
            }
        }

        ReleaseStgMedium(&medium);
    }

    return bRet;
}
