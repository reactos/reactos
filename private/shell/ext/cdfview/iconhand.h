//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// iconhand.h 
//
//   The class definition for the cdf icon handler.
//
//   History:
//
//       4/23/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _ICONHAND_H_

#define _ICONHAND_H_

//
// Defines
//

#define WSTR_DEFAULT    L"DefaultLogo"

#define LOGO_WIDTH      80
#define LOGO_WIDTH_WIDE 194

#define GLEAM_OFFSET    53


//
// The class definition for the icon handler.
//

class CIconHandler : public IExtractIcon,
#ifdef UNICODE
                     public IExtractIconA,
#endif
                     public IExtractImage,
                     public IRunnableTask,
                     public CPersist
{
//
// Methods.
//

public:

    // Constructor
    CIconHandler(void);

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

    // IExtractImage
    STDMETHODIMP GetLocation(LPWSTR pszPathBuffer,
                             DWORD cch,
                             DWORD *pdwPriority,
                             const SIZE* pSize,
                             DWORD dwRecClrDepth,
                             DWORD *pdwFlags);

    STDMETHODIMP Extract(HBITMAP* phBmp);

    // IRunnable task
    STDMETHODIMP         Run(void);
    STDMETHODIMP         Kill(BOOL fWait);
    STDMETHODIMP         Suspend(void);
    STDMETHODIMP         Resume(void);
    STDMETHODIMP_(ULONG) IsRunning(void);

private:

    //Destructor
    ~CIconHandler(void);

    // check for default installed channel
    BOOL IsDefaultChannel(void);
    
    //Helper functions
    HRESULT ParseCdfIcon(void);
    HRESULT ParseCdfImage(BSTR* pbstrURL, BSTR* pbstrWURL);
    HRESULT ParseCdfShellLink();
    HRESULT ParseCdfInfoTip(void** ppv);
    HRESULT ExtractCustomImage(const SIZE* pSize,HBITMAP* phBmp);
    HRESULT ExtractDefaultImage(const SIZE* pSize,HBITMAP* phBmp);
    HRESULT GetBitmap(IImgCtx* pIImgCtx, const SIZE* pSize, HBITMAP* phBmp);

    HRESULT StretchBltCustomImage(IImgCtx* pIImgCtx,
                                  const SIZE* pSize,
                                  HDC hdcDst); 

    HRESULT StretchBltDefaultImage(const SIZE* pSize, HDC hdcDest);
    HRESULT DrawGleam(HDC hdcDst);
    HRESULT SynchronousDownload(IImgCtx* pIImgCtx, LPCWSTR pwszURL);

    inline BOOL UseWideLogo(int cx) {return cx >
                                         ((LOGO_WIDTH + LOGO_WIDTH_WIDE) >> 1);}

    //HRESULT QueryInternetShortcut(PCDFITEMIDLIST pcdfidl,
    //                              REFIID riid,
    //                              void** ppvOut);

//
// Member variables.
//

private:

    ULONG           m_cRef;
    IExtractIcon*   m_pIExtractIcon;
    BSTR            m_bstrImageURL;
    BSTR            m_bstrImageWideURL;
    PCDFITEMIDLIST  m_pcdfidl;
    BOOL            m_fDone;
    BOOL            m_fDrawGleam;
    DWORD           m_dwClrDepth;
    SIZE            m_rgSize;
    LPTSTR			m_pszErrURL;   // this is the res: URL that shows errors messages
};

#endif // _ICONHAND_H_
