//=================================================================
//
//   File:      optshold.hxx
//
//  Contents:   COptionsHolder class
//
//  Classes:    COptionsHolder
//              CFontNameOptions
//              CFontSizeOptions
//
//=================================================================

#ifndef I_OPTSHOLD_HXX_
#define I_OPTSHOLD_HXX_
#pragma INCMSG("--- Beg 'optshold.hxx'")

#define _hxx_
#include "optshold.hdl"

MtExtern(CFontNameOptions)
MtExtern(CFontNameOptions_aryFontNames_pv)
MtExtern(CFontSizeOptions)
MtExtern(CFontSizeOptions_aryFontSizes_pv)
MtExtern(COptionsHolder)
MtExtern(COptionsHolder_aryFontSizeObjects_pv)

//+------------------------------------------------------------
//
//  Class : CFontNamesOptions
//
//-------------------------------------------------------------

class CFontNameOptions : public CBase,
                         public IHTMLFontNamesCollection
{
    typedef CBase super;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontNameOptions))

    CFontNameOptions() {};
    ~CFontNameOptions();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CFontNameOptions);
    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IHTMLOptionsHolder methods
    #define _CFontNameOptions_
    #include "optshold.hdl"

    // helper and builder functions
    HRESULT   AddName (TCHAR * strFontNmae);
    void      SetSize(long lSize) { _aryFontNames.SetSize(lSize); };

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    DECLARE_CDataAry(CAryFontNames, CStr, Mt(Mem), Mt(CFontNameOptions_aryFontNames_pv))
    CAryFontNames _aryFontNames;
};


//+------------------------------------------------------------
//
//  Class : CFontSizeOptions
//
//-------------------------------------------------------------

class CFontSizeOptions : public CBase,
                         public IHTMLFontSizesCollection
{
    typedef CBase  super;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontSizeOptions))

    CFontSizeOptions() {};
    ~CFontSizeOptions() { _aryFontSizes.DeleteAll(); };

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CFontSizeOptions);
    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)


    // IHTMLOptionsHolder methods
    #define _CFontSizeOptions_
    #include "optshold.hdl"

    // helper and builer functions
    HRESULT     AddSize(long lFsize);
    void      SetSize(long lSize) { _aryFontSizes.SetSize(lSize); };


    CStr            _cstrFont;

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    DECLARE_CPtrAry(CAryFontSizes, long, Mt(Mem), Mt(CFontSizeOptions_aryFontSizes_pv))
    CAryFontSizes  _aryFontSizes;

};




//+------------------------------------------------------------
//
//  Class : COptionsHolder
//
//-------------------------------------------------------------

class COptionsHolder : public CBase,
                       public IHTMLOptionsHolder
{
    typedef CBase  super;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COptionsHolder))

    COptionsHolder(CDoc *pDoc);
    ~COptionsHolder();

    virtual void    Passivate();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(COptionsHolder);
    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    HRESULT OpenSaveFileDlg( VARIANTARG initFile, VARIANTARG initDir, VARIANTARG filter, VARIANTARG title, BSTR *pathName, BOOL fSaveFile);
    // helper methods
    long   GetObjectLocation(BSTR strTargetFontName);

    // IHTMLOptionsHolder methods
    #define _COptionsHolder_
    #include "optshold.hdl"

    // helper and builder functions
    void    SetParentHWnd(HWND hParentWnd) {_hParentWnd = hParentWnd;}

protected:
    DECLARE_CLASSDESC_MEMBERS;


private:
    DECLARE_CPtrAry(CAryFontSizeObjects, CFontSizeOptions *, Mt(Mem), Mt(COptionsHolder_aryFontSizeObjects_pv))

    CDoc                      * _pDoc;           // pointer back to the document
    CFontNameOptions          * _pFontNameObj;   // pointer to font name obj
    CAryFontSizeObjects         _aryFontSizeObjects; // pointer to font Size obj
    VARIANT                     _execArg;       // Argument of ExecCommand
    HWND                        _hParentWnd;
};

#pragma INCMSG("--- End 'optshold.hxx'")
#else
#pragma INCMSG("*** Dup 'optshold.hxx'")
#endif
