//=================================================================
//
//   File:      rulescol.hxx
//
//  Contents:   Style Sheet Rules Array class
//
//  Classes:    CStyleSheetRuleArray
//
//=================================================================

#ifndef I_RULESCOL_HXX_
#define I_RULESCOL_HXX_
#pragma INCMSG("--- Beg 'rulescol.hxx'")

#define _hxx_
#include "rulescol.hdl"

class CStyleSheet;
class CStyleRule;
class CRuleStyle;

MtExtern(CStyleSheetRule)
MtExtern(CStyleSheetRuleArray)

//+----------------------------------------------------------------------------
//
//  Class:      CStyleSheetRule
//
//   Note:      Stylesheet rule 
//
//-----------------------------------------------------------------------------
class CStyleSheetRule : public CBase
{
    friend class CElement;
    DECLARE_CLASS_TYPES(CStyleSheetRule, CBase)
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSheetRule))

    CStyleSheetRule( CStyleSheet *pStyleSheet, DWORD dwSID, ELEMENT_TAG eTag );
    ~CStyleSheetRule();
    int StyleSheetRelease();
    CStyleRule *GetRule();
    inline CStyleSheet *GetStyleSheet() { return _pStyleSheet; };

#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void) { return _pStyleSheet->UndoManager(); }
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE) 
        { return _pStyleSheet->QueryCreateUndo( fRequiresParent, fDirtyChange ); }
#endif // NO_EDIT

    #define _CStyleSheetRule_
    #include "rulescol.hdl"

    DECLARE_PLAIN_IUNKNOWN(CStyleSheetRule);
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    DWORD           _dwID;      // Equivalent to the CStyleID.
protected:
    DECLARE_CLASSDESC_MEMBERS;
    CStyleSheet    *_pStyleSheet;
    ELEMENT_TAG     _eTag;
    CRuleStyle     *_pStyle;
};




//+------------------------------------------------------------
//
//  Class : CStyleRuleArray
//
//-------------------------------------------------------------

class CStyleSheetRuleArray : public CCollectionBase
{
    DECLARE_CLASS_TYPES(CStyleSheetRuleArray, CCollectionBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSheetRuleArray))

    CStyleSheetRuleArray( CStyleSheet *pStyleSheet );
    ~CStyleSheetRuleArray();
    int StyleSheetRelease();

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CStyleSheetRuleArray);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);


    // Necessary to support expandos on a collection.
    CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { return ((CBase *)_pStyleSheet)->GetAtomTable(pfExpando); }


    #define _CStyleSheetRuleArray_
    #include "rulescol.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    // CCollectionBase overrides
    long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE ) {return -1;}
    LPCTSTR GetName(long lIdx) {return NULL;}
    HRESULT GetItem( long lIndex, VARIANT *pvar );

private:
    CStyleSheet                    *_pStyleSheet;
};

#pragma INCMSG("--- End 'rulescol.hxx'")
#else
#pragma INCMSG("*** Dup 'rulescol.hxx'")
#endif
