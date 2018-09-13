//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       _txtsave.h
//
//  Contents:   Objects used for saving forms to the stream
//
//  Classes:    CTextSaver
//              CRangeSaver
//
//----------------------------------------------------------------------------

#ifndef I__TXTSAVE_H_
#define I__TXTSAVE_H_
#pragma INCMSG("--- Beg '_txtsave.h'")

#ifndef X_SAVER_HXX_
#define X_SAVER_HXX_
#include "saver.hxx"
#endif

class CStreamWriteBuff;
class CElement;

//
//  Flags for use with the range saver
//
enum
{
    RSF_CFHTML_HEADER =         0x1,    // include CF-HTML header
    RSF_FRAGMENT =              0x2,    // include the fragment
    RSF_CONTEXT =               0x4,    // include the context
    RSF_FOR_RTF_CONV =          0x8,    // mode friendly to the RTF converter
    RSF_SELECTION =             0x10,   // include the selection
    RSF_NO_ENTITIZE_UNKNOWN =   0x20,   // do not entitize unknown characters
    RSF_NO_IE4_COMPAT_SEL =     0x40,   // don't compute selection according to IE4 rules
    RSF_NO_IE4_COMPAT_FRAG =    0x80    // don't compute fragment according to IE4 rules
};

#define RSF_CFHTML (RSF_CFHTML_HEADER | RSF_FRAGMENT | RSF_SELECTION | RSF_CONTEXT)
#define RSF_HTML   (RSF_FRAGMENT | RSF_SELECTION | RSF_CONTEXT)

//+---------------------------------------------------------------------------
//
//  Class:      CRangeSaver
//
//  Synopsis:   This class is designed to write a given range to a stream
//              with various formatting options.
//
//----------------------------------------------------------------------------

class CRangeSaver : public CTreeSaver
{
public:

    CRangeSaver(
        CMarkupPointer *    pLeft,
        CMarkupPointer *    pRight,
        DWORD               dwFlags,
        CStreamWriteBuff *  pswb,
        CMarkup *           pMarkup,
        CElement *          pelContainer = NULL );

    HRESULT Save();

private:

    void Initialize(
        CMarkupPointer *    pLeft,
        CMarkupPointer *    pRight,
        DWORD               dwFlags,
        CStreamWriteBuff *  pswb,
        CMarkup *           pMarkup,
        CElement *          pelContainer );

protected:
    //
    // CF-HTML header offset information
    //
    struct
    {
        LONG iHTMLStart, iHTMLEnd;
        LONG iFragmentStart, iFragmentEnd;
        LONG iSelectionStart, iSelectionEnd;
    }
    _header;

    HRESULT SaveSelection( BOOL fEnd );

    //
    // Internal helpers
    //
    HRESULT GetStmOffset(LONG * plOffset);
    HRESULT SetStmOffset(LONG lOffset);
    HRESULT WriteCFHTMLHeader();
    HRESULT WriteOpenContext();
    HRESULT WriteCloseContext();

    void    DoIE4SelectionCollapse();
    void    ComputeIE4Fragment();
    void    ComputeIE4Selection();

    DWORD       _dwFlags;
};

#pragma INCMSG("--- End '_txtsave.h'")
#else
#pragma INCMSG("*** Dup '_txtsave.h'")
#endif
