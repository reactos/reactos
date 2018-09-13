/**********************************************************************
 * FontFldr.h - Definition for the CFontFolder, our implementation for
 *       the IShellFolder in our browser.
 *
 **********************************************************************/

#if !defined(__FONTFLDR_H__)
#define __FONTFLDR_H__

// Forward declarations --------------------------------------------------
//
#if defined(__FCN__)
#include "fsnotify.h"      // for NOTIFYWATCH
#endif

#if !defined(__VIEWVECT_H__)
#include "viewvect.h"
#endif

class CFontList;
class CFontView;

// ********************************************************************
class CFontFolder : public IShellFolder, public IPersistFolder
{
public:
    CFontFolder();
    ~CFontFolder();
    int Init();
    
    // Utility functions.
    //
     CFontList * poFontList ( );
    BOOL bRefresh( );
    BOOL bRefView( CFontView * poView );
    BOOL bReleaseView( CFontView * poView );

#if defined(__FCN__)
    VOID vReconcileFolder( );    // Launch a thread to call vDoReconcileFolder
    VOID vDoReconcileFolder( );
#endif

    // *** IUnknown methods ***

    STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj );
    STDMETHODIMP_(ULONG) AddRef( void );
    STDMETHODIMP_(ULONG) Release( void );
    
    // *** IShellFolder methods ***

    STDMETHODIMP ParseDisplayName ( HWND hwndOwner,
                                    LPBC pbc, 
                                    LPOLESTR lpszDisplayName,
                                    ULONG * pchEaten, 
                                    LPITEMIDLIST * ppidl,
                                    ULONG *pdwAttributes) ;
    
    STDMETHODIMP EnumObjects( HWND hwndOwner,
                              DWORD grfFlags,
                              LPENUMIDLIST * ppenumIDList) ;
    
    STDMETHODIMP BindToObject( LPCITEMIDLIST pidl,
                               LPBC pbcReserved,
                               REFIID riid,
                               LPVOID * ppvOut) ;

    STDMETHODIMP BindToStorage( LPCITEMIDLIST pidl,
                                LPBC pbcReserved,
                                REFIID riid,
                                LPVOID * ppvObj) ;

    STDMETHODIMP CompareIDs( LPARAM lParam,
                             LPCITEMIDLIST pidl1,
                             LPCITEMIDLIST pidl2) ;

    STDMETHODIMP CreateViewObject( HWND hwndOwner,
                                   REFIID riid,
                                   LPVOID * ppvOut) ;

    STDMETHODIMP GetAttributesOf( UINT cidl,
                                  LPCITEMIDLIST * apidl,
                                  ULONG * rgfInOut) ;

    STDMETHODIMP GetUIObjectOf( HWND hwndOwner,
                                UINT cidl,
                                LPCITEMIDLIST * apidl,
                                REFIID riid,
                                UINT * prgfInOut,
                                LPVOID * ppvOut) ;

    STDMETHODIMP GetDisplayNameOf( LPCITEMIDLIST pidl,
                                   DWORD uFlags,
                                   LPSTRRET lpName) ;

    STDMETHODIMP SetNameOf( HWND hwndOwner,
                            LPCITEMIDLIST pidl,
                            LPCOLESTR lpszName,
                            DWORD uFlags,
                            LPITEMIDLIST * ppidlOut) ;

    
    // *** IPersist methods ***

    STDMETHODIMP GetClassID( LPCLSID lpClassID );
    
    // *** IPersistFolder methods ***

    STDMETHODIMP Initialize( LPCITEMIDLIST pidl );
    
    
private:
    static int sm_id;
    int m_id;
    int m_cRef;

#if defined(__FCN__)
    NOTIFYWATCH m_Notify;
    HANDLE      m_hNotifyThread;
#endif

    CViewVector *  m_poViewList;

};

#endif   // __FONTFLDR_H__
