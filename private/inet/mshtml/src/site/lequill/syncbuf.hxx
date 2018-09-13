//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       syncbuf.hxx
//
//  Contents:   Tree Syncronization Opcode/Operand Buffer stuff
//
//----------------------------------------------------------------------------

#ifndef I_SYNCBUF_HXX_
#define I_SYNCBUF_HXX_
#pragma INCMSG("--- Beg 'syncbuf.hxx'")

#include "logtool.h"

MtExtern(CTreeSyncLogger)
MtExtern(CTreeSyncLogger_aryLogSinks_pv)

// forwards
interface ITreeSyncLogSink;

// main tree logging object
class CTreeSyncLogger
{
public:

	DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreeSyncLogger))

    CTreeSyncLogger();
    ~CTreeSyncLogger();

    HRESULT SyncLoggerSend(CElement *pElementSyncRoot);

    bool IsLogging(CDoc *pDoc);

    HRESULT ApplyLog(
        IHTMLElement * pIElemTreeSyncRoot,
        BYTE * rgbData,
        DWORD cbData,
        long cpRootBaseAdjust,
        bool fForward);

	HRESULT ApplyForward1(
		IHTMLElement * pElemTreeSyncRoot,
		DWORD opcode,
		BYTE *rgbStruct,
		long cpRootBaseAdjust);

public: // bug (tomlaw)

    struct SINKREC
    {
        ITreeSyncLogSink * _pSink;
    };
    DECLARE_CDataAry(CAryLogSinks, SINKREC, Mt(Mem), Mt(CTreeSyncLogger_aryLogSinks_pv))
    CAryLogSinks        _aryLogSinks;

public:

    HRESULT ApplyInsertText(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertTextLogRecord *pLogRecord);
    HRESULT ApplyInsertElement(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertElementLogRecord *pLogRecord);
    HRESULT ApplyDeleteElement(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CDeleteElementLogRecord *pLogRecord);
    HRESULT ApplyChangeAttr(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CChangeAttrLogRecord *pLogRecord);
    HRESULT ApplyInsertTree(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertTreeLogRecord *pLogRecord);
    HRESULT ApplyCutCopyMove(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CCutCopyMoveTreeLogRecord *pLogRecord);

    HRESULT ApplyReverseInsertText(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertTextLogRecord *pLogRecord);
    HRESULT ApplyReverseInsertElement(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertElementLogRecord *pLogRecord);
    HRESULT ApplyReverseDeleteElement(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CDeleteElementLogRecord *pLogRecord);
    HRESULT ApplyReverseChangeAttr(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CChangeAttrLogRecord *pLogRecord);
    HRESULT ApplyReverseInsertTree(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertTreeLogRecord *pLogRecord);
    HRESULT ApplyReverseCutCopyMove(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CCutCopyMoveTreeLogRecord *pLogRecord);

    HRESULT SetAttributeHandler(CBase *pbase, BSTR strPropertyName, VARIANT * pvarPropertyValue, LONG lFlags, bool *pfDoItOut, CElement **ppElementSyncRoot);

public: // bug (tomlaw)

    CLogBufferPoolManager _bufpool;
    CLogBuffer _bufOut;
};

#endif
