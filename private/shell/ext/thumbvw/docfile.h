#ifndef _DOCFILE_H
#define _DOCFILE_H

class CDocFileHandler : public IExtractImage2,
                        public IPersistFile,
                        public CComObjectRoot,
                        public CComCoClass< CDocFileHandler,&CLSID_DocfileThumbnailHandler >
{
    public:
        CDocFileHandler();
        ~CDocFileHandler();
        
        BEGIN_COM_MAP( CDocFileHandler )
            COM_INTERFACE_ENTRY( IExtractImage )
            COM_INTERFACE_ENTRY( IExtractImage2 )
            COM_INTERFACE_ENTRY( IPersistFile )
        END_COM_MAP( )

        DECLARE_REGISTRY( CDocFileHandler,
                          _T("Shell.ThumbnailExtract.Docfile.1"),
                          _T("Shell.ThumbnailExtract.DocFile.1"),
                          IDS_DOCTHUMBEXTRACT_DESC,
                          THREADFLAGS_APARTMENT);

        DECLARE_NOT_AGGREGATABLE( CDocFileHandler );

        // IExtractThumbnail
        STDMETHOD (GetLocation) ( LPWSTR pszPathBuffer,
                                  DWORD cch,
                                  DWORD * pdwPriority,
                                  const SIZE * prgSize,
                                  DWORD dwRecClrDepth,
                                  DWORD *pdwFlags );
     
        STDMETHOD (Extract)( HBITMAP * phBmpThumbnail);

        STDMETHOD ( GetDateStamp ) ( FILETIME * pftDateStamp );
        
        // IPersistFile
        STDMETHOD (GetClassID)(CLSID * pCLSID );
        STDMETHOD (IsDirty)(void);
        STDMETHOD (Load)(LPCOLESTR pszFileName, DWORD dwMode);
        STDMETHOD (Save)(LPCOLESTR pszFileName, BOOL fRemember);
        STDMETHOD (SaveCompleted)(LPCOLESTR pszFileName);
        STDMETHOD (GetCurFile)(LPOLESTR * ppszFileName);
    protected:
        LPWSTR m_pszPath;
        SIZE m_rgSize;
        DWORD m_dwRecClrDepth;
        BITBOOL m_fOrigSize : 1;
};

#endif

