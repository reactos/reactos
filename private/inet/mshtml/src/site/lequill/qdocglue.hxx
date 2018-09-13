//+----------------------------------------------------------------------------
//
// File:        qdocglue.HXX
//
// Contents:    CQDocGlue and related classes
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _QDOCGLUE_HXX_
#define _QDOCGLUE_HXX_

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_QUILLSITE_H_
#define X_QUILLSITE_H_
#include "quillsite.h"
#endif

#ifndef X_ITREESYNC_H_
#define X_ITREESYNC_H_
#include "itreesync.h"
#endif

#ifndef X_SYNCBUF_HXX_
#define X_SYNCBUF_HXX_
#include "syncbuf.hxx"
#endif

MtExtern(CQIdleTask)
MtExtern(CQDocGlue)

class CQIdleTask : public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CQIdleTask))

    CQIdleTask(ITextLayoutGroup* pTextLayoutGroup);

    void OnRun(DWORD dwTimeOut);
    void OnTerminate();
    
private:
    ITextLayoutGroup    *m_pITextLayoutGroup;
};

class CQDocGlue : public ITextLayoutAccess
#if TREE_SYNC
        , public ITreeSyncServices
        , public ITreeSyncLogSource
#endif
{
//// Internal helper functions
protected:
    HRESULT InitTextLayoutGroup();

//// Helper API functions
public:
    HRESULT Init();
    void Passivate();
    BOOL FExternalLayout();
    inline ITextLayoutGroup *GetTextLayoutGroup() { return m_pITextLayoutGroup; }

//// CQDocGlue implementation
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CQDocGlue))

    CQDocGlue(IUnknown *pUnkOuter);
    ~CQDocGlue();

    //// IUnknown implementation
    DECLARE_AGGREGATED_IUNKNOWN(CQDocGlue)
    STDMETHODIMP PrivateQueryInterfaceNonIUnknown(REFIID, void **);

    IUnknown * PunkInner() { return (IUnknown *)(ITextLayoutAccess *)this; }

#if TREE_SYNC
//// ITreeSyncServices methods
public:

    STDMETHODIMP GetBindBehavior(
        IHTMLElement * pElemTreeSyncRoot,
        ITreeSyncBehavior ** pTreeSyncRoot);

    STDMETHODIMP MoveSyncInfoToPointer(
        IMarkupPointer * pPointer,
        IHTMLElement ** pElemTreeSyncRoot,
        long * pcpRelative);

    STDMETHODIMP MovePointerToSyncInfo(
        IMarkupPointer * pPointer,
        IHTMLElement * pElemTreeSyncRoot,
        long cpRelative);

    STDMETHODIMP MoveSyncInfoToElement(
        IHTMLElement * pElement,
        IHTMLElement ** pElemTreeSyncRoot,
		long * pcpRelative);

    STDMETHODIMP GetElementFromSyncInfo(
        IHTMLElement ** pElement,
        IHTMLElement * pElemTreeSyncRoot,
		long cpRelative);

	STDMETHODIMP ApplyForward(
		IHTMLElement * pElemTreeSyncRoot,
		BYTE * rgbData,
		DWORD cbData,
        long cpRootBaseAdjust);

	STDMETHODIMP ApplyReverse(
		IHTMLElement * pElemTreeSyncRoot,
		BYTE * rgbData,
		DWORD cbData,
        long cpRootBaseAdjust);

	STDMETHODIMP InsertElementViewOnly( // experimental
		IHTMLElement * pIElem,
		IMarkupPointer * pIStart,
		IMarkupPointer * pIEnd);

	STDMETHODIMP RemoveElementViewOnly( // experimental
		IHTMLElement * pIElem);

	// temporary method, until we have the real stuff going
	STDMETHODIMP RebuildCpMap(
		IHTMLElement * pIElementSyncRoot,
        ITreeSyncRemapHack * pSyncUpdateCallback);

	STDMETHODIMP ApplyForward1(
		IHTMLElement * pElemTreeSyncRoot,
		DWORD opcode,
		BYTE *rgbStruct,
		long cpRootBaseAdjust);

//// ITreeSyncServices helpers
public:
	STDMETHODIMP MovePointerToCPos(IMarkupPointer *pIPointer, long cp);
	STDMETHODIMP GetSyncInfoFromTreePos(CTreePos * ptp,  CElement ** pElementSyncRoot,  long *pcpRelative);
	STDMETHODIMP GetSyncInfoFromElement(IHTMLElement * pIElement,
                                        CElement ** pElementSyncRoot, 
                                        long * pcpRelativeStart,
                                        long * pcpRelativeEnd);
	STDMETHODIMP GetSyncInfoFromPointer(IMarkupPointer * pPointer, 
                                        CElement ** pElementSyncRoot, 
                                        long * pcpRelative);
	STDMETHODIMP GetSyncBaseIndex(CElement * pElementSyncRoot,long * pcpRelative);
	STDMETHODIMP GetSyncBaseIndexI(
        IHTMLElement * pElementSyncRoot,
		long * pcpRelative);

//// ITreeSyncLogSource methods
	STDMETHODIMP RegisterLogSink(
		ITreeSyncLogSink * pLogSink);

	STDMETHODIMP UnregisterLogSink(
		ITreeSyncLogSink * pLogSink);

//// Tree Sync Instance Variables
public:
    CTreeSyncLogger _SyncLogger;
#endif // TREE_SYNC

//// ITextLayoutAccess implementation
protected:
	STDMETHODIMP GetTextLayoutGroup(ITextLayoutGroup **ppgrp);
    STDMETHODIMP GetLayoutOptions(DWORD *pdwOptions);
    STDMETHODIMP SetLayoutOptions(DWORD dwOptions);

//// member variables
private:
    IUnknown            *_pUnkOuter;            // Controlling IUnknown
    ITextLayoutGroup    *m_pITextLayoutGroup;   // Pointer to ITextLayoutGroup interface
    DWORD               m_dwOptions;            // Layout options
    CQIdleTask          *m_pIdleTask;           // Pointer to CTask to call Quill's idle
    CDoc                *_pDoc;                 // Pointer to the CDoc (from _pUnkOuter)
};

#endif  // _QDOCGLUE_HXX_
