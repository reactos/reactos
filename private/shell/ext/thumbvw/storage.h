#ifndef _STORAGE_H
#define _STORAGE_H

#include "tngen\ctngen.h"

class CThumbStore : public IShellImageStore,
                    public IPersistFolder,
                    public IPersistFile,
                    public CComObjectRoot,
                    public CComCoClass< CThumbStore,&CLSID_ShellThumbnailDiskCache >
{
    struct PrivCatalogEntry
    {
        DWORD     cbSize;
        DWORD     dwIndex;
        FILETIME  ftTimeStamp;
        WCHAR     szName[1];
    };

    struct CatalogHeader
    {
        WORD      cbSize;
        WORD      wVersion;
        DWORD     dwEntryCount;
    };

    public:
        BEGIN_COM_MAP( CThumbStore )
            COM_INTERFACE_ENTRY( IShellImageStore )
            COM_INTERFACE_ENTRY( IPersistFolder )
            COM_INTERFACE_ENTRY( IPersistFile )
        END_COM_MAP( )

        DECLARE_REGISTRY( CThumbStore,
                          _T("Shell.ThumbnailDiskCache.1"),
                          _T("Shell.ThumbnailDiskCache.1"),
                          IDS_THUMBNAILVIEW_DESC,
                          THREADFLAGS_APARTMENT)

        DECLARE_NOT_AGGREGATABLE( CThumbStore )

        CThumbStore();
        ~CThumbStore();

        HRESULT LoadCatalog( void );
        HRESULT SaveCatalog( void );
        
        ////////////////////////////////////////
        // *** IPersist methods ***
        STDMETHOD(GetClassID)(THIS_ LPCLSID lpClassID);

        // *** IPersistFolder methods ***
        STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidl);

        // *** IPersistFile methods ***
        STDMETHOD (IsDirty) ( void);
        STDMETHOD (Load) ( LPCWSTR pszFileName, DWORD dwMode );
        STDMETHOD (Save) ( LPCWSTR pszFileName, BOOL fRemember );
        STDMETHOD (SaveCompleted) ( LPCWSTR pszFileName );
        STDMETHOD (GetCurFile) ( LPWSTR *ppszFileName );

        // *** IImageCache methods ****
        STDMETHOD ( Open ) ( DWORD dwMode, DWORD * pdwLock );
        STDMETHOD ( Create ) ( DWORD dwMode, DWORD * pdwLock );
        STDMETHOD ( Close ) ( DWORD const * pdwLock );
        STDMETHOD ( Commit ) ( DWORD const * pdwLock );
        STDMETHOD ( ReleaseLock ) ( DWORD const * pdwLock );
        STDMETHOD ( IsLocked ) ( THIS );
        
        STDMETHOD ( GetMode ) ( DWORD * pdwMode );
        STDMETHOD ( GetCapabilities ) ( DWORD * pdwCapMask );

        STDMETHOD ( AddEntry ) ( LPCWSTR pszName, const FILETIME * pftTimeStamp, DWORD dwMode, HBITMAP hImage );
        STDMETHOD ( GetEntry ) ( LPCWSTR pszName, DWORD dwMode, HBITMAP * phImage );
        STDMETHOD ( DeleteEntry ) ( LPCWSTR pszName );
        STDMETHOD ( IsEntryInStore ) ( LPCWSTR pszName, FILETIME * pftTimeStamp );

        STDMETHOD ( Enum ) ( LPENUMSHELLIMAGESTORE * ppEnum );
       
   protected:
        friend class CEnumThumbStore;
        
        HRESULT FindStreamID( LPCWSTR pszName, DWORD & dwStream, PrivCatalogEntry ** ppEntry );
        HRESULT GetEntryStream( DWORD dwStream, DWORD dwMode, LPSTREAM *ppStream );
        DWORD GetAccessMode( DWORD dwMode, BOOL fStream );

        DWORD AquireLock( void );
        void ReleaseLock( DWORD dwLock );

        BOOL InitCodec( void );

        HRESULT PrepImage( HBITMAP *phBmp, SIZE * prgSize, LPVOID * ppBits );
        BOOL DecompressImage( LPVOID pvInBuffer, ULONG ulBufferSize, HBITMAP * phBmp );
        BOOL CompressImage( HBITMAP hBmp, LPVOID * ppvOutBuffer, ULONG * plBufSize );

        HRESULT WriteImage( LPSTREAM pStream, HBITMAP hBmp );
        HRESULT ReadImage( LPSTREAM pStream, HBITMAP * phBmp );

        BOOL SupportsStreams( void );
        void CheckSupportsStreams( LPCWSTR pszPath );

        HRESULT WriteToStream( LPCWSTR pszPath, const FILETIME * pftFileTimeStamp, HBITMAP hBmp );
        HRESULT ReadFromStream( LPCWSTR pszPath, HBITMAP * phBmp );
        HRESULT DeleteFromStream( LPCWSTR pszPath );
        HRESULT IsEntryInStream( LPCWSTR pszPath, FILETIME * pftTimeStamp );
        
        
        CatalogHeader m_rgHeader;
        CList<PrivCatalogEntry> m_rgCatalog;
        LPSTORAGE m_pStorage;
        DWORD m_dwFlags;
        WCHAR m_szPath[MAX_PATH];
        DWORD m_dwMaxIndex;

        DWORD m_dwCatalogChange;

        // Crit section used to protect the internals
        CRITICAL_SECTION m_csInternals;
        
        // needed for this object to be free-threaded... so that 
        // we can query the catalog from the main thread whilst icons are
        // being read and written from the main thread.
        CRITICAL_SECTION m_csLock;

        DWORD m_dwLock;
        int m_fLocked;
        BITBOOL m_fSupportsStreams : 1;

        CThumbnailFCNContainer * m_pJPEGCodec;
};

HRESULT CEnumThumbStore_Create( CThumbStore * pThis, LPENUMSHELLIMAGESTORE * ppEnum );

class CEnumThumbStore : public IEnumShellImageStore,
                         public CComObjectRoot
{
    public:
        BEGIN_COM_MAP( CEnumThumbStore )
            COM_INTERFACE_ENTRY( IEnumShellImageStore )
        END_COM_MAP( )

        CEnumThumbStore();
        ~CEnumThumbStore();

        STDMETHOD ( Reset ) ( void );
        STDMETHOD ( Next ) ( ULONG celt, PENUMSHELLIMAGESTOREDATA * prgElt, ULONG * pceltFetched );
        STDMETHOD ( Skip ) ( ULONG celt );
        STDMETHOD ( Clone ) ( IEnumShellImageStore ** pEnum );
        
    protected:
        friend HRESULT CEnumThumbStore_Create( CThumbStore * pThis, LPENUMSHELLIMAGESTORE * ppEnum );

        CThumbStore * m_pStore;
        CLISTPOS m_pPos;
        DWORD m_dwCatalogChange;
};

#endif

