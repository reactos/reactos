//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// exticon.h 
//
//   Extract icon com object.
//
//   History:
//
//       3/21/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _EXTICON_H_

#define _EXTICON_H_


//
// Defines
//

#define     TSTR_ICO_EXT        TEXT(".ico")

#define     INDEX_IMAGE         -1

#define     COLOR1              (RGB(0,0,255))
#define     COLOR2              (RGB(0,255,0))


//
// Function prototypes.
//

void CALLBACK ImgCtx_Callback(void* pIImgCtx, void* phEvent);

void MungePath(LPTSTR pszPath);
void DemungePath(LPTSTR pszPath);


//
// Class definition for the extract icon class.
//

class CExtractIcon : public IExtractIcon
#ifdef UNICODE
                    ,public IExtractIconA
#endif
{
//
// Methods
//

public:

    // Constructors
    CExtractIcon(PCDFITEMIDLIST pcdfidl,
                 IXMLElementCollection* pIXMLElementCollection);

    CExtractIcon (
        PCDFITEMIDLIST pcdfidl,
        IXMLElement *pElem);

    CExtractIcon( BSTR pszPath );
    
    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IExtractIcon methods.
    STDMETHODIMP GetIconLocation(UINT uFlags,
                                 LPTSTR szIconFile,
                                 UINT cchMax,
                                 int *piIndex,
                                 UINT *pwFlags);

    STDMETHODIMP Extract(LPCTSTR pszFile,
                         UINT nIconIndex,
                         HICON *phiconLarge,
                         HICON *phiconSmall,
                         UINT nIconSize);

#ifdef UNICODE
    // IExtractIconA methods.
    STDMETHODIMP GetIconLocation(UINT uFlags,
                                 LPSTR szIconFile,
                                 UINT cchMax,
                                 int *piIndex,
                                 UINT *pwFlags);

    STDMETHODIMP Extract(LPCSTR pszFile,
                         UINT nIconIndex,
                         HICON *phiconLarge,
                         HICON *phiconSmall,
                         UINT nIconSize);
#endif

    // Public helper(s) to talk between implementations of IExtractIcon
    // KENSY: We should probably update the other helpers to look at this
    //        variable instead of taking a gleam parameter.
    
    void    SetGleam(BOOL fGleam) { m_fGleam = fGleam; }
    
private:

    // Destructor.
    ~CExtractIcon(void);

    // Helper functions.
    STDMETHODIMP GetCustomIconLocation(UINT uFlags,
                                       LPTSTR szIconFile,
                                       UINT cchMax,
                                       int *piIndex,
                                       UINT *pwFlags);

    STDMETHODIMP GetDefaultIconLocation(UINT uFlags,
                                        LPTSTR szIconFile,
                                        UINT cchMax,
                                        int *piIndex,
                                        UINT *pwFlags);

    HRESULT SynchronousDownload(LPCTSTR pszFile,
                                IImgCtx** ppIImgCtx,
                                HANDLE hExitThreadEvent);

    HICON   ExtractImageIcon(WORD wSize, IImgCtx* pIImgCtx, BOOL fDrawGleam);

    HRESULT CreateImageAndMask(IImgCtx* pIImgCtx,
                               HDC hdcScreen,
                               SIZE* pSize,
                               HBITMAP* phbmImage,
                               HBITMAP* phbmMask,
                               BOOL fDrawGleam);

    HRESULT StretchBltImage(IImgCtx* pIImgCtx,
                            const SIZE* pSize,
                            HDC hdcDst,
                            BOOL fDrawGleam);

    HRESULT CreateMask(IImgCtx* pIImgCtx,
                       HDC hdcScreen,
                       HDC hdc1,
                       const SIZE* pSize,
                       HBITMAP* phbmMask,
                       BOOL fDrawGleam);

    BOOL ColorFill(HDC hdc, const SIZE* pSize, COLORREF clr);

    HRESULT ExtractGleamedIcon(LPCTSTR pszIconFile, 
                               int iIndex, 
                               UINT uFlags,
                               HICON *phiconLarge, 
                               HICON *phiconSmall, 
                               UINT nIconSize);

    HRESULT ApplyGleamToIcon(HICON hIcon, ULONG nSize, HICON *phGleamedIcon);

    BOOL GetBitmapSize(HBITMAP hbmp, int* pcx, int* pcy);
    
//
// Member variables.
//

private:

    ULONG       m_cRef;
    int         m_iconType;
    BSTR        m_bstrIconURL;
    BOOL        m_fGleam;
};


#endif _EXTICON_H_
