//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         MFmWrap.h
//
// Contents     Interface definition for Mac Unicode-friendly Forms Wrapper 
//              Interfaces
//
// Interfaces   CControlMac
//              
//
// Note:        These class definitions are required to convert internal
//              UNICODE strings to ANSI strings before passing them on to
//              the appropriate Mac Forms superclass method. By defining
//              the interface name as our subclass wrapper, the main body of
//              code does not need to concern itself with UNICODE vs ANSI -
//              the code will call the correct method.
//
//	History:	02/07/96    Created by kfl / black diamond.
//
//-----------------------------------------------------------------------------

#ifndef I_MFMWRAP_HXX_
#define I_MFMWRAP_HXX_
#pragma INCMSG("--- Beg 'mfmwrap.hxx'")

// Note:        The following typedefs are required by the Mac 
//              Unicode wrapper classes.  MFmWrap #defines
//              some of the forms interfaces so that the main body 
//              of code will use the wrapper classes instead of the
//              original forms interface.  However, in order to not 
//              have to wrap any method that refers to a pointer
//              to the wrapped interface, we need a way to reference
//              the original interface - thus the need for the pointer
//              typedef.
//

interface IControls;
typedef IControls *             LPCONTROLS;

#  if defined(_MACUNICODE) && !defined(_MAC)
// the rest of the code will only be used for Mac UNICODE  implementations
STDAPI FormsCreatePropertyFrameW(
            HWND        hwndOwner,
            UINT        x,
            UINT        y,
      const LPWSTR      lpszCaption,
            ULONG       cObjects,
            IUnknown**  ppunk,
            ULONG       cPages,
      const CLSID *     pPageClsID,
            LCID        lcid);
#define   FormsCreatePropertyFrame   FormsCreatePropertyFrameW

/*
STDAPI FormsOpenReadOnlyStorageOnResourceW(
    HINSTANCE hInst, LPCWSTR lpstrID, LPSTORAGE * ppStg);
#define   FormsOpenReadOnlyStorageOnResource   FormsOpenReadOnlyStorageOnResourceW
*/
#ifdef PRODUCT_97
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
interface ITextBox95Mac : public ITextBox95
{
public:
    operator ITextBox95* () { return this; }

    virtual HRESULT __stdcall SetPasswordChar( 
        /* [in] */ OLECHAR wchar);
    virtual HRESULT __stdcall SetPasswordChar( 
        /* [in] */ WCHAR wchar) = 0;
    
    
    virtual HRESULT __stdcall GetPasswordChar( 
        /* [out] */ OLECHAR *wchar);
    virtual HRESULT __stdcall GetPasswordChar( 
        /* [out] */ WCHAR *wchar) = 0;
};
#define ITextBox95                    ITextBox95Mac
#endif  // PRODUCT_97

//------------------------------------------------------------------------------ 
//
//------------------------------------------------------------------------------
interface IControlSelectorEventsMac : public IControlSelectorEvents
{
public:
    operator IControlSelectorEvents* () { return this; }

    virtual HRESULT __stdcall SelectionChange( 
        /* [in] */ REFCLSID clsid,
        /* [in] */ OLECHAR *szTooltip);
    
    virtual HRESULT __stdcall SelectionChange( 
        /* [in] */ REFCLSID clsid,
        /* [in] */ WCHAR *szTooltip) = 0;
};
#define IControlSelectorEvents                    IControlSelectorEventsMac


//------------------------------------------------------------------------------ 
//
//------------------------------------------------------------------------------
interface IControlsMac : public IControls
{
public:
    operator IControls* () { return this; }

        virtual HRESULT __stdcall GetItemByName( 
            /* [in] */ LPCOLESTR pstr,
            /* [out] */ IControl **Control);
        
        virtual HRESULT __stdcall GetItemByName( 
            /* [in] */ LPCWSTR pstr,
            /* [out] */ IControl **Control) = 0;
        
};
#define IControls                    IControlsMac

//------------------------------------------------------------------------------ 
//
//------------------------------------------------------------------------------
interface IControlPaletteEventsMac : public IControlPaletteEvents
{
public:
    operator IControlPaletteEvents* () { return this; }

        virtual HRESULT __stdcall SelectionChange( 
            /* [in] */ REFCLSID clsid,
            /* [in] */ LPOLESTR szTooltip);
        virtual HRESULT __stdcall SelectionChange( 
            /* [in] */ REFCLSID clsid,
            /* [in] */ LPWSTR szTooltip) = 0;
};
#define IControlPaletteEvents        IControlPaletteEventsMac

//------------------------------------------------------------------------------ 
//
//------------------------------------------------------------------------------
interface IControlPaletteMac : public IControlPalette
{
public:
    operator IControlPalette* () { return this; }

        virtual HRESULT __stdcall AddPage( 
            /* [in] */ IStorage *pStg,
            /* [in] */ LPOLESTR szName,
            /* [out] */ long *plIndex);
        virtual HRESULT __stdcall AddPage( 
            /* [in] */ IStorage * pStg,
            /* [in] */ LPWSTR szName,
            /* [out] */ long *plIndex) = 0;
        
        
        virtual HRESULT __stdcall InsertPage( 
            /* [in] */ IStorage *pStg,
            /* [in] */ LPOLESTR szName,
            /* [in] */ long lIndex);
        
        virtual HRESULT __stdcall InsertPage( 
            /* [in] */ IStorage * pStg,
            /* [in] */ LPWSTR szName,
            /* [in] */ long lIndex) = 0;
        
};
#define IControlPalette        IControlPaletteMac

//------------------------------------------------------------------------------ 
//
//------------------------------------------------------------------------------
interface IGetUniqueIDMac : public IGetUniqueID
{
public:
    operator IGetUniqueID* () { return this; }

        virtual HRESULT __stdcall GetUniqueName( 
            /* [in] */ LPWSTR pstrPrefix,
            /* [in] */ LPWSTR pstrSuggestedName,
            /* [out][in] */ ULONG *pulSuffix,
            /* [in] */ BOOL fAllowDupeCheck) = 0;
        virtual HRESULT __stdcall GetUniqueName( 
            /* [in] */ LPOLESTR pstrPrefix,
            /* [in] */ LPOLESTR pstrSuggestedName,
            /* [out][in] */ ULONG *pulSuffix,
            /* [in] */ BOOL fAllowDupeCheck);
};
#define IGetUniqueID                IGetUniqueIDMac

//------------------------------------------------------------------------------ 
//
//------------------------------------------------------------------------------
interface ITabStripExpertEventsMac : public ITabStripExpertEvents
{
public:
    operator ITabStripExpertEvents* () { return this; }

        virtual HRESULT __stdcall DoRenameItem( 
            /* [in] */ long lIndex,
            /* [in] */ LPOLESTR bstr,
            /* [in] */ LPOLESTR bstrTip,
            /* [in] */ LPOLESTR bstrAccel,
            /* [out][in] */ VARIANT_BOOL *EnableDefault);
        virtual HRESULT __stdcall DoRenameItem( 
            /* [in] */ long lIndex,
            /* [in] */ LPWSTR bstr,
            /* [in] */ LPWSTR bstrTip,
            /* [in] */ LPWSTR bstrAccel,
            /* [out][in] */ VARIANT_BOOL *EnableDefault) = 0;
};

interface ITabStripExpertMac : public ITabStripExpert
{
public:
    operator ITabStripExpert* () { return this; }

        virtual HRESULT __stdcall SetTabStripExpertEvents( 
            ITabStripExpertEventsMac *pTabStripExpertEvents)  = 0;
        virtual HRESULT __stdcall SetTabStripExpertEvents( 
            ITabStripExpertEvents *pTabStripExpertEvents) 
        { return  SetTabStripExpertEvents ((ITabStripExpertEventsMac*)pTabStripExpertEvents);  }
};
#define ITabStripExpert                ITabStripExpertMac
#define ITabStripExpertEvents          ITabStripExpertEventsMac

#endif // _MACUNICODE

#pragma INCMSG("--- End 'mfmwrap.hxx'")
#else
#pragma INCMSG("*** Dup 'mfmwrap.hxx'")
#endif
