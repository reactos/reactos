#ifndef _LOGO_H
#define _LOGO_H

typedef HRESULT (* LPUPDATEFN)( LPVOID pData, DWORD dwItem, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache );

#define LOGO_HEIGHT 32
#define LOGO_WIDE_WIDTH 194
#define LOGO_WIDTH  80

class CLogoBase
{
    public:
        CLogoBase( BOOL fWide = FALSE );
        ~CLogoBase();

        virtual STDMETHODIMP_(ULONG) AddRef(void)  PURE;
        virtual STDMETHODIMP_(ULONG) Release(void) PURE;

        static void _Initialize( void );
        static void _Cleanup( void );
        
        virtual IShellFolder * GetSF() PURE;
        virtual HWND GetHWND() PURE;

        inline HIMAGELIST GetLogoHIML( void );
        
        // intialisation functions
        HRESULT InitLogoView( void );
        HRESULT ExitLogoView( void );

        int GetLogoIndex( DWORD dwItem, LPCITEMIDLIST pidl, LPRUNNABLETASK *ppTask, DWORD * pdwPriority, DWORD * pdwFlags );
        int GetDefaultLogo( LPCITEMIDLIST pidl, BOOL fQuick );

        HRESULT AddTaskToQueue( LPRUNNABLETASK pTask, DWORD dwPriority, DWORD lParam );
        
        // create the default logo for an item....
        HRESULT CreateDefaultLogo(int iIcon, int cxLogo, int cyLogo, LPCTSTR pszText, HBITMAP * phBmpLogo);

        HRESULT FlushLogoCache( void );

        HRESULT AddRefLogoCache( void );
        HRESULT ReleaseLogoCache( void );

        // get the task ID used with the task scheduler
        virtual REFTASKOWNERID GetTOID( void ) PURE;

        virtual HRESULT UpdateLogoCallback( DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache ) PURE;

        HRESULT DitherBitmap( HBITMAP hBmp, HBITMAP * phBmpNew );

        int AddIndicesToLogoList( int iIcon, UINT uIndex );

        int FindLogoFromIcon( int iIcon, int * piLastLogo );
        
    protected:
        
        int GetCachedLogoIndex(DWORD dwItem, LPCITEMIDLIST pidl, LPRUNNABLETASK *ppTask, DWORD * pdwPriority, DWORD * pdwFlags );
        
        IImageCache * _pLogoCache;              // My be NULL in low memory conditions.
        IShellTaskScheduler * _pTaskScheduler;
        HIMAGELIST _himlLogos;
        SIZEL _rgLogoSize;
        DWORD _dwClrDepth;
        HDSA  _hdsaLogoIndices;

        static CRITICAL_SECTION s_csSharedLogos;
        static long             s_lSharedWideLogosRef;
        static IImageCache *    s_pSharedWideLogoCache;
        static HDSA             s_hdsaWideLogoIndices;

        HPALETTE _hpalHalftone;
        BOOL     _fWide;
};

inline HIMAGELIST CLogoBase::GetLogoHIML( )
{
    return _himlLogos;
}

struct LogoIndex
{
    int iIcon;
    int iLogo;
};


#endif

