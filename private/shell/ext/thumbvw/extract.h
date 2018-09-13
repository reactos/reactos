#ifndef _EXTRACT_H
#define _EXTRACT_H

void CalculateAspectRatio( const SIZE * prgSize, RECT * pRect );
BOOL FactorAspectRatio( LPBITMAPINFO pbiScaled,
                        LPVOID pScaledBits,
                        const SIZE * prgSize,
                        RECT rect,
                        DWORD dwClrDepth,
                        HPALETTE hPal,
                        BOOL fOrigSize,
                        HBITMAP * phBmpThumbnail );

HRESULT RegisterHandler( LPCSTR pszExts, UINT cExts, UINT cEntrySize, LPCSTR pszIID, LPCSTR pszCLSID );
HRESULT UnregisterHandler( LPCSTR pszExts, UINT cExts, UINT cEntrySize, LPCSTR pszIID, LPCSTR pszCLSID );

extern "C" BOOL ConvertDIBSECTIONToThumbnail( BITMAPINFO * pbi,
                                              LPVOID pBits,
                                              HBITMAP * phBmpThumbnail,
                                              const SIZE * prgSize,
                                              DWORD dwClrDepth,
                                              HPALETTE hpal,
                                              UINT uiSharpPct,
                                              BOOL fOrigImage );

BOOL CreateSizedDIBSECTION( const SIZE * prgSize,
                            DWORD dwClrDepth,
                            HPALETTE hpal,
                            const BITMAPINFO * pCurInfo,
                            HBITMAP * phbmp,
                            BITMAPINFO ** pBMI,
                            LPVOID * ppBits );

LPVOID CalcBitsOffsetInDIB( LPBITMAPINFO pBMI );

#define THUMBNAIL_BACKGROUND_BRUSH    WHITE_BRUSH
#define THUMBNAIL_BACKGROUND_PEN      WHITE_PEN

#define THUMBNAIL_BORDER_PEN          BLACK_PEN
#define THUMBNAIL_BORDERSHADOW_BRUSH  GRAY_BRUSH

class CThumbnailView;
HRESULT CExtractImageTask_Create( CThumbnailView * pView,
                                  LPEXTRACTIMAGE pExtract,
                                  LPCWSTR pszCache,
                                  LPCWSTR pszFullPath,
                                  LPCITEMIDLIST pidl,
                                  const FILETIME * pfNewTimeStamp,
                                  int iItem,
                                  DWORD dwFlags,
                                  LPRUNNABLETASK * ppTask );

class CExtractImageTask : public IRunnableTask,
                          public CComObjectRoot
{
    public:
        BEGIN_COM_MAP( CExtractImageTask )
            COM_INTERFACE_ENTRY( IRunnableTask )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CExtractImageTask )
        
        STDMETHOD (Run)( void );
        STDMETHOD (Kill)( BOOL fWait );
        STDMETHOD (Suspend)( );
        STDMETHOD (Resume)( );
        STDMETHOD_( ULONG, IsRunning )( void );

    protected:
        CExtractImageTask();
        ~CExtractImageTask();
        HRESULT InternalResume();

        friend HRESULT CExtractImageTask_Create( CThumbnailView * pView,
                                                 LPEXTRACTIMAGE pExtract,
                                                 LPCWSTR pszCache,
                                                 LPCWSTR pszFullPath,
                                                 LPCITEMIDLIST pidl,
                                                 const FILETIME * pfNewDateStamp,
                                                 int iItem,
                                                 DWORD dwFlags,
                                                 LPRUNNABLETASK * ppTask );

        LONG m_lState;
        LPEXTRACTIMAGE m_pExtract;
        LPRUNNABLETASK m_pTask;
        WCHAR m_szCache[MAX_PATH];
        WCHAR m_szFullPath[MAX_PATH];
        LPCITEMIDLIST m_pidl;
        CThumbnailView * m_pView;
        DWORD m_dwMask;
        DWORD m_dwFlags;
        int m_iItem;
        HBITMAP m_hBmp;
        FILETIME m_ftDateStamp;
        BOOL m_fNoDateStamp;
};

class CThumbnailShrinker : public IScaleAndSharpenImage2,
                           public CComObjectRoot,
                           public CComCoClass< CThumbnailShrinker,&CLSID_ThumbnailScaler >

{
    public:
        CThumbnailShrinker();
        ~CThumbnailShrinker();
    
        BEGIN_COM_MAP( CThumbnailShrinker )
            COM_INTERFACE_ENTRY( IScaleAndSharpenImage2 )
        END_COM_MAP( )

        DECLARE_REGISTRY( CThumbnailShrinker,
                          _T("Shell.ThumbnailExtract.BMP.1"),
                          _T("Shell.ThumbnailExtract.BMP.1"),
                          IDS_BMPTHUMBEXTRACT_DESC,
                          THREADFLAGS_APARTMENT);

        DECLARE_NOT_AGGREGATABLE( CThumbnailShrinker );

        STDMETHOD( ScaleSharpen2) ( BITMAPINFO * pbi,
                                    LPVOID pBits,
                                    HBITMAP * phBmpThumbnail,
                                    const SIZE * prgSize,
                                    DWORD dwRecClrDepth,
                                    HPALETTE hpal,
                                    UINT uiSharpPct,
                                    BOOL fOrigSize );
};

#endif

