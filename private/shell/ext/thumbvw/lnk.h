#ifndef _LNK_H
#define _LNK_H

class CLnkThumb :  public IPersistFile,
                   public IRunnableTask,
                   public IExtractImage2,
                   public CComObjectRoot,
                   public CComCoClass< CLnkThumb,&CLSID_LnkThumbnailDelegator >
{
    public:
        CLnkThumb();
        ~CLnkThumb();

        // we provide our own QI ....
        HRESULT _InternalQueryInterface( REFIID riid, LPVOID * ppvObj );

        DECLARE_REGISTRY( CLnkThumb,
                          _T("Shell.ThumbnailExtract.Lnk.1"),
                          _T("Shell.ThumbnailExtract.Lnk.1"),
                          IDS_LNKTHUMBEXTRACT_DESC,
                          THREADFLAGS_APARTMENT);

        DECLARE_NOT_AGGREGATABLE( CLnkThumb );

        // IRunnableTask
        STDMETHOD (Run)();
        STDMETHOD (Kill)( BOOL fWait );
        STDMETHOD (Suspend)( );
        STDMETHOD (Resume)( );
        STDMETHOD_( ULONG, IsRunning )();

        // IExtractThumbnail
        STDMETHOD (GetLocation) ( LPWSTR pszPathBuffer,
                                  DWORD cch,
                                  DWORD * pdwPriority,
                                  const SIZE * prgSize,
                                  DWORD dwRecClrDepth,
                                  DWORD *pdwFlags );
     
        STDMETHOD (Extract)( HBITMAP * phBmpThumbnail);

        STDMETHOD (GetDateStamp) ( FILETIME * pftDateStamp );
        
        // IPersistFile
        STDMETHOD (GetClassID )(CLSID *pClassID);
        STDMETHOD (IsDirty )();
        STDMETHOD (Load )( LPCOLESTR pszFileName, DWORD dwMode);
        STDMETHOD (Save )( LPCOLESTR pszFileName, BOOL fRemember);
        STDMETHOD (SaveCompleted )( LPCOLESTR pszFileName);
        STDMETHOD (GetCurFile )( LPOLESTR *ppszFileName);

   protected:
        HRESULT BindToFolder();

        LPITEMIDLIST m_pidl;
        LPITEMIDLIST m_pidlLast;
        LPSHELLFOLDER m_pFolder;
        LPEXTRACTIMAGE m_pExtract;
        LPEXTRACTIMAGE2 m_pExtract2;
        LPRUNNABLETASK m_pRunnable;
};

#endif
