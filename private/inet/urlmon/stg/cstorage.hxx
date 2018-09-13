//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CSTORAGE.HXX
//
//  Contents:
//
//  Classes:    Declaration for IStorage class
//
//  Functions:
//
//  History:    12-20-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR CStorage : public IStorage
{
public:

        CStorage(IStorage *pStorage);

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** IStorage methods ***
        STDMETHOD(CreateStream) (THIS_ const OLECHAR *pwcsName, DWORD grfMode,
            DWORD dwReserved1, DWORD dwReserved2, LPSTREAM FAR *ppStm);
        STDMETHOD(OpenStream) (THIS_ const OLECHAR *pwcsName,
            void FAR *pReserved1, DWORD grfMode, DWORD dwReserved2,
            LPSTREAM FAR *ppStm);
        STDMETHOD(CreateStorage) (THIS_ const OLECHAR *pwcsName, DWORD grfMode,
            DWORD dwReserved1, DWORD dwReserved2, LPSTORAGE FAR *ppStg);
        STDMETHOD(OpenStorage) (THIS_ const OLECHAR *pwcsName,
            LPSTORAGE pstgPriority, DWORD grfMode, SNB snbExclude,
            DWORD dwReserved, LPSTORAGE FAR *ppStg);
        STDMETHOD(CopyTo) (THIS_ DWORD dwCiidExclude,
            IID const FAR *rgiidExclude, SNB snbExclude, LPSTORAGE pStgDest);
        STDMETHOD(MoveElementTo) (THIS_ const OLECHAR *lpszName,
            LPSTORAGE pStgDest, const OLECHAR *lpszNewName, DWORD grfFlags);
        STDMETHOD(Commit) (THIS_ DWORD grfCommitFlags);
        STDMETHOD(Revert) (THIS);
        STDMETHOD(EnumElements) (THIS_ DWORD dwReserved1, void FAR *pReserved2,
            DWORD dwReserved3, LPENUMSTATSTG FAR *ppenumStatStg);
        STDMETHOD(DestroyElement) (THIS_ const OLECHAR *pwcsName);
        STDMETHOD(RenameElement) (THIS_ const OLECHAR *pwcsOldName,
            const OLECHAR *pwcsNewName);
        STDMETHOD(SetElementTimes) (THIS_ const OLECHAR *lpszName,
            FILETIME const FAR *pctime, FILETIME const FAR *patime,
            FILETIME const FAR *pmtime);
        STDMETHOD(SetClass) (THIS_ REFCLSID rclsid);
        STDMETHOD(SetStateBits) (THIS_ DWORD grfStateBits, DWORD grfMask);
        STDMETHOD(Stat) (THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag);

private:
        CRefCount   _CRefs;
        IStorage    *_pStg;
};

