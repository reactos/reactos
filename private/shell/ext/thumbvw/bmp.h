#ifndef _BMP_H
#define _BMP_H

////////////////////////////////////////////////////////////////////////////////////
//  structures for dealing with import filters.
#pragma pack(2)                     // Switch on 2-byte packing.
typedef struct 
{
    unsigned short  slippery: 1;    // True if file may disappear.
    unsigned short  write : 1;      // True if open for write.
    unsigned short  unnamed: 1;     // True if unnamed.
    unsigned short  linked : 1;     // Linked to an FS FCB.
    unsigned short  mark : 1;       // Generic mark bit.
    union 
    {
        CHAR        ext[4];         // File extension.
        HFILE       hfEmbed;        // handle to file containing graphic (for import).
    };
    unsigned short  handle;         // not used.
    CHAR     fullName[260];         // Full path name and file name.
    DWORD    filePos;               // Position in file of...
} FILESPEC;

typedef struct 
{
    HANDLE  h;
    RECT    bbox;
    int     inch;
} GRPI;
#pragma pack()

BOOL HasGraphicsFilter( LPCWSTR pszExt, LPSTR szHandler, DWORD * pcbSize );

class COfficeThumb :  public IExtractImage,
                      public IPersistFile,
                      public CComObjectRoot,
                      public CComCoClass< COfficeThumb,&CLSID_OfficeGrfxFilterThumbnailExtractor >
{
    public:
        COfficeThumb();
        ~COfficeThumb();

        BEGIN_COM_MAP( COfficeThumb )
            COM_INTERFACE_ENTRY( IExtractImage )
            COM_INTERFACE_ENTRY( IPersistFile )
        END_COM_MAP( )

        DECLARE_REGISTRY( COfficeThumb,
                          _T("Shell.ThumbnailExtract.Office.1"),
                          _T("Shell.ThumbnailExtract.Office.1"),
                          IDS_OFCTHUMBEXTRACT_DESC,
                          THREADFLAGS_APARTMENT);

        DECLARE_NOT_AGGREGATABLE( COfficeThumb );

        // IExtractImage
        STDMETHOD (GetLocation) ( LPWSTR pszPathBuffer,
                                  DWORD cch,
                                  DWORD * pdwPriority,
                                  const SIZE * prgSize,
                                  DWORD dwRecClrDepth,
                                  DWORD *pdwFlags );
 
        STDMETHOD (Extract)( HBITMAP * phBmpThumbnail);

        // IPersistFile
        STDMETHOD (GetClassID )(CLSID *pClassID);
        STDMETHOD (IsDirty )();
        STDMETHOD (Load )( LPCOLESTR pszFileName, DWORD dwMode);
        STDMETHOD (Save )( LPCOLESTR pszFileName, BOOL fRemember);
        STDMETHOD (SaveCompleted )( LPCOLESTR pszFileName);
        STDMETHOD (GetCurFile )( LPOLESTR *ppszFileName);

        LPBITMAPINFOHEADER MetaHeaderToBitmapInfo( LPMETAHEADER pmh );
        LPBITMAPINFOHEADER HMetafileToBitmapInfo( GRPI pict );

    protected:
        WCHAR m_szPath[MAX_PATH];
        SIZE m_rgSize;
        DWORD m_dwRecClrDepth;
        BITBOOL m_fOrigSize;
};


#endif

