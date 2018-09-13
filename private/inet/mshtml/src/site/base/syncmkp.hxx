//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       synctree.hxx
//
//  Contents:   Markup Syncronization Opcode/Operand Buffer stuff
//
//----------------------------------------------------------------------------

#ifndef I_SYNCMKP_HXX_
#define I_SYNCMKP_HXX_
#pragma INCMSG("--- Beg 'syncmkp.hxx'")

MtExtern(CMkpSyncLogger_aryLogSinks_pv)

// forwards
interface IMarkupSyncLogSink;
interface IHTMLElement;
interface IMarkupPointer;
class CMarkupPointer;

// main markup logging object
class CMkpSyncLogger
{
public:

    CMkpSyncLogger();
    ~CMkpSyncLogger();

    inline bool IsLogging()
    {
        return (0 != _aryLogSinks.Size());
    }

    DECLARE_CPtrAry(CAryLogSinks, IMarkupSyncLogSink *, Mt(Mem), Mt(CMkpSyncLogger_aryLogSinks_pv))
    CAryLogSinks        _aryLogSinks;

    HRESULT SetAttributeHandler(CElement * pElement, BSTR strPropertyName, VARIANT * pvarPropertyValue, LONG lFlags);
    HRESULT InsertElementHandler(IMarkupPointer * pIPointerStart, IMarkupPointer * pIPointerFinish, IHTMLElement * pIElementInsert);
    HRESULT RemoveElementHandler(CElement * pElementRemove);
    HRESULT InsertTextHandler(OLECHAR * pchText, long cch, CMarkupPointer * pPointerTarget);
    HRESULT CutCopyMoveHandler(CMarkupPointer * pPointerStart, CMarkupPointer * pPointerFinish, CMarkupPointer * pPointerTarget, BOOL fRemove);
    HRESULT CutCopyMoveFinalizeHandler();

    // static so it's accessible to CBase (which can't include the necessary headers due to conflicts)
    static HRESULT StaticSetAttributeHandler(CBase * pbase, BSTR strPropertyName, VARIANT * pvarPropertyValue, LONG lFlags);

}; // CMkpSyncLogger

#endif
