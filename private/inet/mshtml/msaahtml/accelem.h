//================================================================================
//      File:   ACCELEM.H
//      Date:   5/30/97
//      Desc:   contains definition of CAccElement class.
//              CAccElement is the abstract base class for all 
//              accessible  elements.
//
//      Author: Arunj
//
//================================================================================

#ifndef __ACCELEM__
#define __ACCELEM__

#if defined(_DEBUG)
extern ULONG g_uObjCount;       // Global object count for all derived objects
#endif

extern LONG g_cLocks; // global dll lock, for memory management and dll unloading

//================================================================================
// includes
//================================================================================

#include "aomtypes.h"

//================================================================================
// CAccElement class definition 
//================================================================================

class CAccElement : public IUnknown
{

public:
        //--------------------------------------------------
    // IUnknown 
    //--------------------------------------------------
    
    virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv)     =0;
    virtual STDMETHODIMP_(ULONG)    AddRef(void)    { return ++m_cRef; }
    virtual STDMETHODIMP_(ULONG)    Release(void)   
    { 
        if ( m_cRef > 0 )
            m_cRef--;
        else
        {
            assert(FALSE);
            return 0;
        }

        // if cRef ==0 we are outta here
        if (!m_cRef)
        {
            delete this;
            return 0;
        }

        return m_cRef;
    }


    //--------------------------------------------------
    // Internal IAccessible support 
    //--------------------------------------------------
    

    virtual HRESULT GetAccParent(IDispatch ** ppdispParent)                 =0;

    virtual HRESULT GetAccName(long lChild, BSTR * pszName)                 =0;

    virtual HRESULT GetAccValue(long lChild, BSTR * pszValue)               =0;

    virtual HRESULT GetAccDescription(long lChild, BSTR * pszDescription)   =0;
    
    virtual HRESULT GetAccRole(long lChild, long *plRole) 
    {
        *plRole = m_lRole;
        return(S_OK);
    }
        
    virtual HRESULT GetAccState(long lChild, long *plState)                 =0;


    virtual HRESULT GetAccHelp(long lChild, BSTR * pszHelp)                 =0;

    virtual HRESULT GetAccHelpTopic(BSTR * pszHelpFile, long lChild,long * pidTopic)    =0;
    
    virtual HRESULT GetAccKeyboardShortcut(long lChild, BSTR * pszKeyboardShortcut)     =0;

    
    virtual HRESULT GetAccDefaultAction(long lChild, BSTR * pszDefAction)   =0;

    virtual HRESULT AccSelect(long flagsSel, long lChild)                   =0;

    virtual HRESULT AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild) =0;
    
    
    virtual HRESULT AccDoDefaultAction(long lChild)                         =0;

    virtual HRESULT SetAccName(long lChild, BSTR szName)                    =0;

    virtual HRESULT SetAccValue(long lChild, BSTR szValue)                  =0;
    
    virtual void Detach() =0;
    virtual BOOL IsDetached() { return FALSE; } // default for AE case 
    virtual void ReleaseTridentInterfaces () {};
    virtual void Zombify() {};

    //--------------------------------------------------
    // Accessor methods
    //--------------------------------------------------

    UINT GetChildID(void)           { return(m_nChildID); }
    HWND GetWindowHandle( void )    { return(m_hWnd); }
    
    long GetAOMType() { return(m_lAOMType); }

    //--------------------------------------------------
    // stub constructor/destructor
    //--------------------------------------------------

    CAccElement() {
#ifdef _DEBUG
        g_uObjCount++;
#endif
        InterlockedIncrement( &g_cLocks); 
    }

    CAccElement(UINT nChildID,HWND hWnd,BOOL bUnSupportedTag = FALSE)
    {
        
#ifdef _DEBUG
        g_uObjCount++;
#endif
        InterlockedIncrement( &g_cLocks); 

        //--------------------------------------------------
        // these variables are common to all elements and 
        // objects.
        //--------------------------------------------------
        m_nChildID      = nChildID;
        m_hWnd          = hWnd;
        m_cRef          = 1;                // Init to 1, since we're alive
        m_pIUnknown     = this;
        m_bSupportedTag = !bUnSupportedTag;

    }

    
    virtual ~CAccElement() {
#ifdef _DEBUG
        g_uObjCount--;
#endif  
        InterlockedDecrement( &g_cLocks);

    }

    //--------------------------------------------------
    // use this method to distinguish between supported and
    // unsupported objects.  This functionality needs 
    // to be implemented here so that it can be accessed
    // at the most abstract level.
    //--------------------------------------------------

    BOOL    IsSupported(void)
    {
        return(m_bSupportedTag);
    }

protected:

    //--------------------------------------------------
    // Data : specific to COM object implementation
    //--------------------------------------------------
    
    ULONG                   m_cRef;         // reference counter
    IUnknown *              m_pIUnknown;    // pointer to delegating IUnknown   

    //--------------------------------------------------
    // Data : specific to generic Accessible object/
    // element heirarchy
    //--------------------------------------------------

    HWND                    m_hWnd;
    UINT                    m_nChildID;         // always CHILDID_SELF for AOs
    long                    m_lRole;            // set during derived object construction.
    BOOL                    m_bSupportedTag;    // flagged TRUE for supported AOM objects.
    long                    m_lAOMType;         // type of AOM data : set in derived class 
                                                // constructor.
};



#endif  // __ACCELEM__