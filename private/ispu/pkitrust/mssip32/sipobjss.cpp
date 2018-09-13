//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjSS.cpp
//
//  Contents:   Microsoft SIP Provider - Structured Storage
//
//  History:    07-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "sipobjss.hxx"

#include    <objidl.h>
#include    <objbase.h>

#define     SIG_AUTHOR          0
#define     SIG_NOTARY          1
#define     SIG_MAX             1

typedef struct StreamIds_
{
    DWORD       dwSigIndex;
    WCHAR       *pwszName;
} StreamIds;

static StreamIds Ids[] =
{
    SIG_AUTHOR, L"\001MSDigSig(Author)",
    SIG_NOTARY, L"\001MSDigSig(Notary)",
    0xffffffff, NULL
};

////////////////////////////////////////////////////////////////////////////
//
// construct/destruct:
//

SIPObjectSS_::SIPObjectSS_(DWORD id) : SIPObject_(id)
{
    GUID        gSS = CRYPT_SUBJTYPE_SS_IMAGE;

    memset(&SpcSigInfo,0x00,sizeof(SPC_SIGINFO));

    SpcSigInfo.dwSipVersion = MSSIP_CURRENT_VERSION;

    memcpy(&SpcSigInfo.gSIPGuid, &gSS, sizeof(GUID));

    pTopStg                 = NULL;

    this->fUseFileMap       = FALSE;
}

SIPObjectSS_::~SIPObjectSS_(void)
{
    if (pTopStg)
    {
        pTopStg->Commit(STGC_DEFAULT);
        pTopStg->Release();
    }
}

////////////////////////////////////////////////////////////////////////////
//
// public:
//

BOOL SIPObjectSS_::RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx)
{
    if (dwIdx > SIG_MAX)
    {
        SetLastError((DWORD)TRUST_E_NOSIGNATURE);
        return(FALSE);
    }

    if (this->FileHandleFromSubject(pSI, GENERIC_READ | GENERIC_WRITE))
    {
        if (pTopStg->DestroyElement(Ids[dwIdx].pwszName) != S_OK)
        {
            SetLastError((DWORD)TRUST_E_NOSIGNATURE);
            return(FALSE);
        }

        return(TRUE);
    }

    return(FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
// protected:
//

BOOL SIPObjectSS_::GetMessageFromFile(SIP_SUBJECTINFO *pSI,
                                      WIN_CERTIFICATE *pWinCert,
                                      DWORD dwIndex,DWORD *pcbCert)
{
    if (!(pTopStg))
    {
        return(FALSE);
    }

    if (dwIndex > SIG_MAX)
    {
        SetLastError((DWORD)TRUST_E_NOSIGNATURE);
        return(FALSE);
    }

    STATSTG     sStatStg;
    IStream     *pStream;
    DWORD       cbCert;

    pStream     = NULL;

    if ((pTopStg->OpenStream(Ids[dwIndex].pwszName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                             0, &pStream) != S_OK) ||
        !(pStream))
    {
        SetLastError(TRUST_E_NOSIGNATURE);

        return(FALSE);
    }

    if (pStream->Stat(&sStatStg, STATFLAG_NONAME) != S_OK)
    {
        pStream->Release();

        SetLastError(ERROR_BAD_FORMAT);

        return(FALSE);
    }

    cbCert = sStatStg.cbSize.LowPart;

    if (cbCert == 0)
    {
        pStream->Release();

        SetLastError(TRUST_E_NOSIGNATURE);

        return(FALSE);
    }

    cbCert += WVT_OFFSETOF(WIN_CERTIFICATE, bCertificate);

    if (*pcbCert < cbCert)
    {
        pStream->Release();

        *pcbCert = cbCert;

        SetLastError(ERROR_INSUFFICIENT_BUFFER);

        return(FALSE);
    }

    if (pWinCert)
    {
        DWORD   cbRead;

        pWinCert->dwLength          = cbCert;
        pWinCert->wRevision         = WIN_CERT_REVISION_2_0;
        pWinCert->wCertificateType  = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
    
        cbRead = 0;

        cbCert -= WVT_OFFSETOF(WIN_CERTIFICATE, bCertificate);

        if ((pStream->Read(pWinCert->bCertificate, cbCert, &cbRead) != S_OK) ||
            (cbRead != cbCert))
        {
            SetLastError(ERROR_BAD_FORMAT);
    
            pStream->Release();

            return(FALSE);
        }
    }

    pStream->Release();

    return(TRUE);
}

BOOL SIPObjectSS_::PutMessageInFile(SIP_SUBJECTINFO *pSI,
                                    WIN_CERTIFICATE *pWinCert,DWORD *pdwIndex)
{
    if ((pWinCert->dwLength <= OFFSETOF(WIN_CERTIFICATE,bCertificate))  ||
        (pWinCert->wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (*pdwIndex > SIG_MAX)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    IStream     *pStream;

    pStream     = NULL;

    if ((pTopStg->CreateStream(Ids[*pdwIndex].pwszName, 
                               STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                               0, 0, &pStream) != S_OK) ||
        !(pStream))
    {
        SetLastError(ERROR_BAD_FORMAT);

        return(FALSE);
    }

    if (pStream->Write(pWinCert->bCertificate, 
                        pWinCert->dwLength - WVT_OFFSETOF(WIN_CERTIFICATE, bCertificate), 
                        NULL) != S_OK)
    {
        pStream->Release();

        SetLastError(ERROR_BAD_FORMAT);

        return(FALSE);
    }

    pStream->Release();

    return(TRUE);
}


BOOL SIPObjectSS_::GetDigestStream(DIGEST_DATA *pDigestData, 
                                   DIGEST_FUNCTION pfnCallBack, DWORD dwFlags)
{
    return(this->IStorageDigest(pTopStg, pDigestData, pfnCallBack));
}


BOOL SIPObjectSS_::IStorageDigest(IStorage *pStg, DIGEST_DATA *pDigestData, DIGEST_FUNCTION pfnCallBack)
{
    STATSTG     *pSortStg;
    STATSTG     sStatStg;
    DWORD       cSortStg;
    BOOL        fRet;


    cSortStg    = 0;
    pSortStg    = NULL;

    if (!(this->SortElements(pStg, &cSortStg, &pSortStg)))
    {
        return(FALSE);
    }

    if (cSortStg == 0)
    {
        return(TRUE);
    }

    if (!(pSortStg))
    {
        return(FALSE);
    }

    for (int i = 0; i < (int)cSortStg; i++)
    {
        switch (pSortStg[i].type)
        {
            case STGTY_STORAGE:
                
                IStorage        *pInnerStg;

                pInnerStg = NULL;

                if ((pStg->OpenStorage(pSortStg[i].pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                       0, 0, &pInnerStg) != S_OK) ||
                    !(pInnerStg))
                {
                    this->FreeElements(&cSortStg, &pSortStg);

                    SetLastError(ERROR_BAD_FORMAT);

                    return(FALSE);
                }

                //
                // WARNING: recursive!
                //
                fRet = this->IStorageDigest(pInnerStg, pDigestData, pfnCallBack);

                pInnerStg->Release();

                if (!(fRet))
                {
                    this->FreeElements(&cSortStg, &pSortStg);

                    return(FALSE);
                }

                break;

            case STGTY_STREAM:

                IStream     *pInnerStm;
                StreamIds   *pIds;
                BOOL        fSigEntry;
                BYTE        rgb[512];
                ULONG       cbRead;

                fSigEntry   = FALSE;
                pIds        = &Ids[0];

                while (pIds->dwSigIndex != 0xffffffff)
                {
                    if (_wcsicmp(pSortStg[i].pwcsName, pIds->pwszName) == 0)
                    {
                        fSigEntry = TRUE;
                        break;
                    }

                    pIds++;
                }

                if (fSigEntry)
                {
                    break;
                }

                pInnerStm = NULL;

                if ((pStg->OpenStream(pSortStg[i].pwcsName, 
                                     NULL,
                                     STGM_READ | STGM_SHARE_EXCLUSIVE,
                                     0,
                                     &pInnerStm) != S_OK) ||
                    !(pInnerStm))
                {
                    this->FreeElements(&cSortStg, &pSortStg);

                    SetLastError(ERROR_BAD_FORMAT);

                    return(FALSE);
                }

                for EVER
                {
                    cbRead = 0;
                    if (pInnerStm->Read(rgb, 512, &cbRead) != S_OK)
                    {
                        break;
                    }

                    if (cbRead == 0)
                    {
                        break;
                    }

                    if (!(pfnCallBack(pDigestData, rgb, cbRead)))
                    {
                        this->FreeElements(&cSortStg, &pSortStg);

                        pInnerStm->Release();

                        return(FALSE);
                    }

                }

                pInnerStm->Release();
                break;

            case STGTY_LOCKBYTES:
                break;

            case STGTY_PROPERTY:
                break;

            default:
                break;
        }
    }

    memset(&sStatStg, 0x00, sizeof(STATSTG));

    if (pStg->Stat(&sStatStg, STATFLAG_NONAME) != S_OK)
    {
        this->FreeElements(&cSortStg, &pSortStg);

        SetLastError(ERROR_BAD_FORMAT);

        return(FALSE);
    }

    //              the ctime member is changed if the file is copied....
    //        !(pfnCallBack(pDigestData, (BYTE *)&sStatStg.ctime, sizeof(FILETIME))) ||
    //
    if (!(pfnCallBack(pDigestData, (BYTE *)&sStatStg.type, sizeof(DWORD))) ||
        !(pfnCallBack(pDigestData, (BYTE *)&sStatStg.cbSize, sizeof(ULARGE_INTEGER))) ||
        !(pfnCallBack(pDigestData, (BYTE *)&sStatStg.clsid, sizeof(CLSID))) ||
        !(pfnCallBack(pDigestData, (BYTE *)&sStatStg.grfStateBits, sizeof(DWORD))))
    {
        this->FreeElements(&cSortStg, &pSortStg);
        
        return(FALSE);
    }
    

    this->FreeElements(&cSortStg, &pSortStg);

    return(TRUE);
}

BOOL SIPObjectSS_::FileHandleFromSubject(SIP_SUBJECTINFO *pSubject, DWORD dwAccess, DWORD dwShared)
{
  /*  if ((dwAccess & GENERIC_WRITE) &&
        (pSubject->hFile != NULL) &&
        (pSubject->hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(pSubject->hFile);
        pSubject->hFile = NULL;
    }  */

    
    HRESULT hr;

    pTopStg = NULL;

  /*  if ((hr = StgOpenStorage((const WCHAR *)pSubject->pwsFileName, 
                        NULL, 
                        (dwAccess & GENERIC_WRITE) ? 
                                        (STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT) : 
                                        (STGM_READ | STGM_SHARE_DENY_NONE | STGM_TRANSACTED),
                        NULL,
                        0,
                        &pTopStg)) != S_OK)
    {
        pTopStg = NULL;
        return(FALSE);
    }  */

    if ((hr = StgOpenStorage((const WCHAR *)pSubject->pwsFileName, 
                        NULL, 
                        (dwAccess & GENERIC_WRITE) ? 
                                        (STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT) : 
                                        (STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT),
                        NULL,
                        0,
                        &pTopStg)) != S_OK)
    {
        pTopStg = NULL;
        
        return(FALSE);
    }  

    return(TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
// private:
//

BOOL WINAPI IsStructuredStorageFile(WCHAR *pwszFileName, GUID *pgSubject)
{
    GUID        gSS = CRYPT_SUBJTYPE_SS_IMAGE;

    if (!(pwszFileName) ||
        !(pgSubject))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (StgIsStorageFile(pwszFileName) == S_OK)
    {
        memcpy(pgSubject, &gSS, sizeof(GUID));
        return(TRUE);
    }

    return(FALSE);
}

static int __cdecl CompareSTATSTG(const void *p1, const void *p2)
{
    return(wcscmp(((STATSTG *)p1)->pwcsName, ((STATSTG *)p2)->pwcsName));
}

void SIPObjectSS_::FreeElements(DWORD *pcStg, STATSTG **ppStg)
{
    if (*ppStg) 
    {
        STATSTG *pStg;

        pStg = *ppStg;

        for (int i = 0; i < (int)*pcStg; i++)
        {
            if (pStg[i].pwcsName)
            {
                CoTaskMemFree(pStg[i].pwcsName);
            }
        }

        DELETE_OBJECT(*ppStg);
    }

    *pcStg = 0;
}

BOOL SIPObjectSS_::SortElements(IStorage *pStg, DWORD *pcSortStg, STATSTG **ppSortStg)
{
    DWORD           cb;
    IEnumSTATSTG    *pEnum;

    pEnum       = NULL;

    *pcSortStg  = 0;
    *ppSortStg  = NULL;

    if (pStg->EnumElements(0, NULL, 0, &pEnum) != S_OK)
    {
        return(FALSE);
    }

    DWORD   celtFetched;
    STATSTG rgCntStatStg[10];

    for EVER
    {
        celtFetched = 0;

        pEnum->Next(10, rgCntStatStg, &celtFetched);

        if (celtFetched == 0)
        {
            break;
        }

        *pcSortStg += celtFetched;

        while (celtFetched--)
        {
            CoTaskMemFree(rgCntStatStg[celtFetched].pwcsName);
        }
    }

    if (*pcSortStg > 0) 
    {
        cb = sizeof(STATSTG) * *pcSortStg;

        if (!(*ppSortStg = (STATSTG *)this->SIPNew(cb)))
        {
            pEnum->Release();
            return(FALSE);
        }

        memset(*ppSortStg, 0x00, cb);

        pEnum->Reset();

        celtFetched = 0;

        if ((pEnum->Next(*pcSortStg, *ppSortStg, &celtFetched) != S_OK) ||
            (celtFetched != *pcSortStg))
        {
            this->FreeElements(pcSortStg, ppSortStg);
            pEnum->Release();

            SetLastError(ERROR_BAD_FORMAT);

            return(FALSE);
        }

        qsort(*ppSortStg, *pcSortStg, sizeof(STATSTG), CompareSTATSTG);
    }

    pEnum->Release();

    return(TRUE);
}
