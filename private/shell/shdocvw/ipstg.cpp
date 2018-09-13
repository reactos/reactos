#include "priv.h"
#include "ipstg.h"

HRESULT CImpIPersistStorage::InitNew(IStorage *pStg)
{
    return InitNew();
}

HRESULT CImpIPersistStorage::Load(IStorage *pStg)
{
    HRESULT hres = E_INVALIDARG;

    if (EVAL(pStg))
    {
        IStream* pstm = NULL;
        hres = pStg->OpenStream(L"CONTENTS",0,STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pstm);
        if (EVAL(SUCCEEDED(hres)))
        {
            pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);
            hres = Load(pstm);
            pstm->Release();
        }
    }

    return hres;
}

HRESULT CImpIPersistStorage::Save(IStorage *pStgSave, BOOL fSameAsLoad)
{
    HRESULT hres = E_INVALIDARG;

    if (EVAL(pStgSave))
    {
        IStream* pstm = NULL;
        hres = pStgSave->CreateStream(L"CONTENTS",STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &pstm);
        if (EVAL(SUCCEEDED(hres)))
        {
            pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);
            hres = Save(pstm, TRUE);
            pstm->Release();
        }
    }

    return hres;
}

HRESULT CImpIPersistStorage::SaveCompleted(IStorage *pStgNew)
{
    return S_OK;
}

HRESULT CImpIPersistStorage::HandsOffStorage(void)
{
    return S_OK;
}

