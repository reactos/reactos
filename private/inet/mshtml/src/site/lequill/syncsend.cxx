#include "headers.hxx"

#define __mshtml_h__
#ifndef X_ITREESYNC_H_
#define X_ITREESYNC_H_
#include "itreesync.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_SYNCBUF_HXX_
#define X_SYNCBUF_HXX_
#include "syncbuf.hxx"
#endif

#ifndef X_QDOCGLUE_HXX_
#define X_QDOCGLUE_HXX_
#include "qdocglue.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif




// turn off the crud
#define MapDocCpToViewCp(a,b,cpDoc) (cpDoc)
#define AdjustDocCp(a,b,c,d) (S_OK)




#if DBG==1
    BOOL g_fEnableTreeSyncDebugOutput = 0;

    void DbgStr(const char * sz)
    {
        if (g_fEnableTreeSyncDebugOutput)
        {
            OutputDebugStringA(sz);
        }
    }

    void DbgStrW(const WCHAR * wz)
    {
        if (g_fEnableTreeSyncDebugOutput)
        {
            OutputDebugStringW(wz);
        }
    }

    void DbgInt(const char * sz, long l)
    {
        if (g_fEnableTreeSyncDebugOutput)
        {
            OutputDebugStringA(sz);
            if (l == 0x80000000)
            {
                OutputDebugStringA("-2147483648");
            }
            else
            {
                char b[30];
                b[29] = 0;
                int i = 29;
                if (l < 0)
                {
                    OutputDebugStringA("-");
                    l = - l;
                }
                do
                {
                    i--;
                    b[i] = '0' + (l % 10);
                    l /= 10;
                } while (l != 0);
                OutputDebugStringA(b + i);
            }
        }
    }

    void DbgDumpTree(char * szLabel, CElement * pElemRoot)
    {
        if (g_fEnableTreeSyncDebugOutput)
        {
            CTreePos * ptpDocWalker;
            CTreePos * ptpDocEnd;
            long cpDocBase;
            long cchDocCountdown;

            DbgStr(szLabel);

            // set up the tree-walking pointers
            pElemRoot->GetTreeExtent(&ptpDocWalker,&ptpDocEnd);

            // get the base cps
            cpDocBase = ptpDocWalker->GetCp();

            DbgInt("base=",cpDocBase);
            DbgStr(" ");

            cchDocCountdown = 0;

            // begin walking
            bool fStop = false;
            while (!fStop)
            {
                if (ptpDocWalker == ptpDocEnd)
                {
                    fStop = true;
                }

                DbgInt(" ",ptpDocWalker->GetCp() - cpDocBase + (ptpDocWalker->IsText() ? ptpDocWalker->GetCch() - cchDocCountdown : 0));
                DbgStr(":");
                if (ptpDocWalker->IsNode())
                {
                    CTreeNode * pDocNode = ptpDocWalker->Branch();
                    CElement * pDocElem = pDocNode->Element();
                    if (ptpDocWalker->IsEndNode())
                    {
                        DbgStr("/");
                    }
                    DbgStrW(NameFromEtag(pDocElem->Tag()));
                }
                else if (ptpDocWalker->IsText())
                {
                    CTxtPtr tpDoc(ptpDocWalker->GetMarkup(),ptpDocWalker->GetCp() + ptpDocWalker->GetCch() - cchDocCountdown);
                    WCHAR wchDoc[2];
                    wchDoc[0] = tpDoc.GetChar();
                    wchDoc[1] = 0;
                    DbgStrW(wchDoc);
                }
                else
                {
                    switch (ptpDocWalker->Type())
                    {
                    default:
                    case CTreePos::Uninit:
                        DbgInt("***???***",ptpDocWalker->Type());
                        break;
                    case CTreePos::NodeBeg:
                    case CTreePos::NodeEnd:
                    case CTreePos::Text:
                        Assert(false);
                        break;
                    case CTreePos::Pointer:
                        DbgInt("ptr",ptpDocWalker->SN());
                        break;
                    }
                }

                // stop loop if end is reached
                if (fStop)
                {
                    break;
                }

                // advance both pointers (advance once, then skip all markup pointers)
                if (cchDocCountdown > 0)
                {
                    cchDocCountdown --;
                }
                if (cchDocCountdown == 0)
                {
                    ptpDocWalker = ptpDocWalker->NextTreePos();
                    Assert(ptpDocWalker);

                    cchDocCountdown = 0;
                    if (ptpDocWalker->Type() == CTreePos::EType::Text)
                    {
                        cchDocCountdown = ptpDocWalker->GetCch();
                    }
                }
            }

            DbgStr("\n");
        }
    }

    void DumpTwoTrees(char * szLabel, CElement * pElementSyncRoot)
    {
        if (g_fEnableTreeSyncDebugOutput)
        {
            HRESULT hr;
            ITreeSyncBehavior * pBinding = NULL;
            IHTMLElement * pIElementSyncRoot = NULL;
            IHTMLElement * pIElementSyncRootInDoc = NULL;
            CElement * pElementSyncRootInDoc;

            // find the data tree
            hr = pElementSyncRoot->QueryInterface(IID_IHTMLElement,(void**)&pIElementSyncRoot);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = pElementSyncRoot->Doc()->_pqdocGlue->GetBindBehavior(pIElementSyncRoot,&pBinding);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = pBinding->GetDataHtmlElement(&pIElementSyncRootInDoc);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = pIElementSyncRootInDoc->QueryInterface(CLSID_CElement,(void**)&pElementSyncRootInDoc);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            if (szLabel)
            {
                DbgStr(szLabel);
            }

            DbgDumpTree(" --- doc:   ",pElementSyncRootInDoc);
            DbgDumpTree("     view:  ",pElementSyncRoot);

        Cleanup:
            Assert(hr == S_OK);
            ReleaseInterface(pBinding);
            ReleaseInterface(pIElementSyncRoot);
            ReleaseInterface(pIElementSyncRootInDoc);
        }
    }
#else
    #define DbgStr(x) ((void)0)
    #define DbgStrW(x) ((void)0)
    #define DbgInt(x,y) ((void)0)
    #define DbgDumpTree(x,y) ((void)0)
    #define DumpTwoTrees(x,y) ((void)0)
#endif


static HRESULT
RebuildCpMap(CDoc * pDocView, CElement * pElementSyncRoot, ITreeSyncRemapHack *pIHack)
{
    HRESULT hr = S_OK;
    ITreeSyncBehavior * pBinding = NULL;
    IHTMLElement * pIElementSyncRoot = NULL;
    IHTMLElement * pIElementSyncRootInDoc = NULL;
    CElement * pElementSyncRootInDoc;
    CTreePos * ptpDocWalker;
    CTreePos * ptpDocEnd;
    CTreePos * ptpViewWalker;
    CTreePos * ptpViewEnd;
    long cpDocBase;
    long cpViewBase;
    long cchDocCountdown;
    long cchViewCountdown;
    VARIANT var;
    VariantInit(&var);
    BSTR bstrName = SysAllocString(L"viewonlyelement");
    IHTMLDocument2 * pIDoc = NULL;

    hr = THR(pDocView->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // find the data tree
    hr = THR(pElementSyncRoot->QueryInterface(IID_IHTMLElement,(void**)&pIElementSyncRoot));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = THR(pDocView->_pqdocGlue->GetBindBehavior(pIElementSyncRoot,&pBinding));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = THR(pBinding->GetDataHtmlElement(&pIElementSyncRootInDoc));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = THR(pIElementSyncRootInDoc->QueryInterface(CLSID_CElement,(void**)&pElementSyncRootInDoc));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    DbgDumpTree(" --- doc:   ",pElementSyncRootInDoc);
    DbgDumpTree("     view:  ",pElementSyncRoot);

    // set up the tree-walking pointers
    pElementSyncRootInDoc->GetTreeExtent(&ptpDocWalker,&ptpDocEnd);
    pElementSyncRoot->GetTreeExtent(&ptpViewWalker,&ptpViewEnd);

    // get the base cps
    cpDocBase = ptpDocWalker->GetCp();
    cpViewBase = ptpViewWalker->GetCp();

    cchDocCountdown = 0;
    cchViewCountdown = 0;

    // begin walking
    while (ptpDocWalker != ptpDocEnd)
    {
        // debug things
        long cpDocLeftCp = ptpDocWalker->GetCp();
        long cpViewLeftCp = ptpViewWalker->GetCp();
        long cpDocLeftCpRel = cpDocLeftCp - cpDocBase;
        long cpViewLeftCpRel = cpViewLeftCp - cpViewBase;

        bool fDontAdvanceData = false;

        // whenever we see something wacked, assume it is crap added to the view and skip it, plus
        // adjust the view offsets in the map.
        bool fAreTheyDifferent = false;

        // check for whole chunks of view-exempt area
        if (!fAreTheyDifferent)
        {
            long cpViewAdjust = 0;
            // detection of skip over subtree stuff
            if (ptpViewWalker->IsBeginNode())
            {
                CTreeNode *ptn = ptpViewWalker->Branch();
                CElement *pel = ptn->Element();
                VariantClear(&var);
                if (SUCCEEDED(pel->getAttribute(bstrName,0,&var)))
                {
                    if (var.vt == VT_BSTR)
                    {
                        if (0 == StrCmpIC(var.bstrVal,L"subtree"))
                        {
                            // skip over everything!
                            pel->GetTreeExtent(NULL,&ptpViewWalker);
                            cpViewAdjust = (ptpViewWalker->GetCp() - cpViewLeftCp) + 1;
                        }
                    }
                }
            }

            // do the adjustment
            if (cpViewAdjust != 0)
            {
                fAreTheyDifferent = true; // don't do the rest of it
                // temporary hack to get the mapper out of trident, until we can figure out the right way
                hr = pIHack->AdjustViewCpOneRoot(pIDoc,pIElementSyncRoot,cpViewLeftCpRel,cpViewAdjust);
                if (S_OK != hr)
                {
                    goto Cleanup;
                }

                // skip ahead the view walker, but don't skip ahead the doc walker
                fDontAdvanceData = true;
            }
        }

        if (!fAreTheyDifferent)
        {
            // check the node types
            if (!fAreTheyDifferent)
            {
                CTreePos::EType eDocType = ptpDocWalker->Type();
                CTreePos::EType eViewType = ptpViewWalker->Type();
                if (eDocType != eViewType)
                {
                    fAreTheyDifferent = true;
                }
            }

            // if same, should check to see that they are for corresponding elements
            if (!fAreTheyDifferent)
            {
                if (ptpDocWalker->IsNode())
                {
                    CTreeNode * pDocNode = ptpDocWalker->Branch();
                    CTreeNode * pViewNode = ptpViewWalker->Branch();
                    while ((pElementSyncRoot != pViewNode->Element())
                        && (pElementSyncRootInDoc != pDocNode->Element()))
                    {
                        CElement * pDocElem = pDocNode->Element();
                        CElement * pViewElem = pViewNode->Element();
                        VariantClear(&var);
                        if (SUCCEEDED(pViewElem->getAttribute(bstrName,0,&var)))
                        {
                            if (var.vt != VT_NULL)
                            {
                                // view only element, so jump it and try again
                                pViewNode = pViewNode->Parent();
                                continue;
                            }
                        }
                        // check tags
                        if (pDocElem->Tag() != pViewElem->Tag())
                        {
                            fAreTheyDifferent = true;
                            break;
                        }
                        // next
                        pDocNode = pDocNode->Parent();
                        pViewNode = pViewNode->Parent();
                    }
                    while (pElementSyncRoot != pViewNode->Element())
                    {
                        bool fCanSkipThisOne = false;
                        CElement * pViewElem = pViewNode->Element();
                        VariantClear(&var);
                        if (SUCCEEDED(pViewElem->getAttribute(bstrName,0,&var)))
                        {
                            if (var.vt != VT_NULL)
                            {
                                // view only element, so jump it and try again
                                fCanSkipThisOne = true;
                            }
                        }
                        if (!fCanSkipThisOne)
                        {
                            break;
                        }
                        // try next parent
                        pViewNode = pViewNode->Parent();
                    }
                    // both should be at root
                    if ((pElementSyncRoot != pViewNode->Element())
                        != (pElementSyncRootInDoc != pDocNode->Element()))
                    {
                        fAreTheyDifferent = true;
                    }
                }
            }

            // if text, should check to see that text is the same
            if (!fAreTheyDifferent)
            {
                if (ptpDocWalker->Type() == CTreePos::EType::Text)
                {
                    CTxtPtr tpDoc(ptpDocWalker->GetMarkup(),ptpDocWalker->GetCp() + ptpDocWalker->GetCch() - cchDocCountdown);
                    CTxtPtr tpView(ptpViewWalker->GetMarkup(),ptpViewWalker->GetCp() + ptpViewWalker->GetCch() - cchViewCountdown);
                    WCHAR wchDoc = tpDoc.GetChar();
                    WCHAR wchView = tpView.GetChar();
                    if (wchDoc != wchView)
                    {
                        fAreTheyDifferent = true;
                    }
                }
            }

            // do the skippage
            if (fAreTheyDifferent)
            {
                // they are different

                // insert a view thingie
                long cpViewTextAdjust = 0;
                if (ptpViewWalker->Type() == CTreePos::EType::Text)
                {
                    cpViewTextAdjust = ptpViewWalker->GetCch() - cchViewCountdown;
                }

                // temporary hack to get the mapper out of trident, until we can figure out the right way
                hr = pIHack->AdjustViewCpOneRoot(pIDoc,pIElementSyncRoot,ptpViewWalker->GetCp() - cpViewBase + cpViewTextAdjust,1);
                if (S_OK != hr)
                {
                    goto Cleanup;
                }

                // skip ahead the view walker, but don't skip ahead the doc walker
                fDontAdvanceData = true;
            }
        }

        // advance both pointers (advance once, then skip all markup pointers)
        if (!fDontAdvanceData)
        {
            if (cchDocCountdown > 0)
            {
                cchDocCountdown --;
            }
            if (cchDocCountdown == 0)
            {
                do
                {
                    ptpDocWalker = ptpDocWalker->NextTreePos();
                    Assert(ptpDocWalker);
                } while (ptpDocWalker->IsPointer());

                cchDocCountdown = 0;
                if (ptpDocWalker->Type() == CTreePos::EType::Text)
                {
                    cchDocCountdown = ptpDocWalker->GetCch();
                }
            }
        }
        if (cchViewCountdown > 0)
        {
            cchViewCountdown --;
        }
        if (cchViewCountdown == 0)
        {
            do
            {
                ptpViewWalker = ptpViewWalker->NextTreePos();
                Assert(ptpViewWalker);
            } while (ptpViewWalker->IsPointer());

            cchViewCountdown = 0;
            if (ptpViewWalker->Type() == CTreePos::EType::Text)
            {
                cchViewCountdown = ptpViewWalker->GetCch();
            }
        }
    }

Cleanup:

    ReleaseInterface(pBinding);
    ReleaseInterface(pIElementSyncRoot);
    ReleaseInterface(pIElementSyncRootInDoc);
    VariantClear(&var);
    SysFreeString(bstrName);
    ReleaseInterface(pIDoc);

    RRETURN(hr);
}


HRESULT
CQDocGlue::InsertElementViewOnly(
	IHTMLElement * pIElem,
	IMarkupPointer * pIStart,
	IMarkupPointer * pIEnd)
{
    HRESULT hr;
    CMarkupPointer * pStart;
    CMarkupPointer * pEnd;
    CTreePos * ptpStart;
    CTreePos * ptpEnd;
    long cpStart;
    long cpEnd;
    CElement * pElem;
    bool fMustRemoveElement = false;
    long cpStartDummy;
    long cpEndDummy;
    CElement * pElemSyncRoot = NULL;
    VARIANT var;
    VariantInit(&var);
    BSTR bstrName = SysAllocString(L"viewonlyelement");
#if 0
    CTreeNode * pFirstBranch;
    long cpFirstBase;
#endif

    hr = THR(pIStart->QueryInterface(CLSID_CMarkupPointer,(void**)&pStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIEnd->QueryInterface(CLSID_CMarkupPointer,(void**)&pEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIElem->QueryInterface(CLSID_CElement,(void**)&pElem));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    ptpStart = pStart->TreePos();
    if (ptpStart == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ptpEnd = pEnd->TreePos();
    if (ptpEnd == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    cpStart = ptpStart->GetCp();
    cpEnd = ptpEnd->GetCp();

    var.vt = VT_I4;
    var.lVal = 1;
    hr = THR(pIElem->setAttribute(bstrName,var,0));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(_pDoc->InsertElement(pIElem,pIStart,pIEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    fMustRemoveElement = true;

#if 0
    pFirstBranch = pElem->GetFirstBranch();
    if (pFirstBranch == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (!pFirstBranch->IsFirstBranch())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (!pFirstBranch->IsLastBranch())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
#endif

    hr = THR(GetSyncInfoFromElement(pIElem,&pElemSyncRoot,&cpStartDummy,&cpEndDummy));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

#if 0
    hr = THR(GetSyncBaseIndex(pElemSyncRoot, &cpFirstBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(AdjustViewCp(_pDoc,pElemSyncRoot,cpEnd - cpFirstBase,1));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(AdjustViewCp(_pDoc,pElemSyncRoot,cpStart - cpFirstBase,1));
    if (S_OK != hr)
    {
        (void)AdjustViewCp(_pDoc,pElemSyncRoot,cpEnd - cpFirstBase,-1); // fixup, but if we failed, we're probably permanently screwed
        goto Cleanup;
    }
#endif

    fMustRemoveElement = false;

Cleanup:

    if (fMustRemoveElement)
    {
        _pDoc->RemoveElement(pIElem);
    }
    VariantClear(&var);
    SysFreeString(bstrName);

    RRETURN(hr);
}


HRESULT
CQDocGlue::RemoveElementViewOnly( // experimental
	IHTMLElement * pIElem)
{
    HRESULT hr;
    CElement * pElem;
    long cpStartDummy;
    long cpEndDummy;
    CElement * pElemSyncRoot = NULL;

    hr = THR(pIElem->QueryInterface(CLSID_CElement,(void**)&pElem));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(GetSyncInfoFromElement(pIElem,&pElemSyncRoot,&cpStartDummy,&cpEndDummy));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(_pDoc->RemoveElement(pIElem));

Cleanup:
    RRETURN(hr);
}




// remove this hack
const IID IID_ITreeSyncLogSource = {0x704bc5e4,0x2e3d,0x11d2,0xbf,0xd9,0x00,0x80,0x5f,0x85,0x23,0x36};


bool
CTreeSyncLogger::IsLogging(CDoc *pDoc)
{
    bool f = (0 != _aryLogSinks.Size());
    return f;
}


HRESULT
CTreeSyncLogger::ApplyInsertText(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertTextLogRecord *pLogRecord)
{
    HRESULT hr;
    IMarkupPointer * pIPointer = NULL;
    CMarkupPointer * pPointer;
    CTreePos * ptpPointer;
    CMarkup * pMarkup;
    long cpDoc;
    long cpView;
    long cch;
    OLECHAR * wz;

    cpDoc = pLogRecord->GetCp();
    cch = pLogRecord->GetCch();
    wz = pLogRecord->GetText();
    Assert((cch == 0) || (wz != NULL));
    if (!((cch == 0) || (wz != NULL)))
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    DbgStr(" --- InsertText");
    DbgInt("  cpDoc=",cpDoc);
    DbgInt("  cch=",cch);
    DbgStr("  wz='");
    DbgStrW(wz);
    DbgStr("'\n");

    // mapper
    cpView = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDoc) + cpViewBase;

    DbgStr("     ");
    DbgInt("  cpView=",cpView - cpViewBase);
    DbgStr("\n");

    // map adjust for the change we make
    hr = AdjustDocCp(pDocView,pElementViewRoot,cpDoc,cch);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointer));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointer,cpView));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointer->SetGravity(POINTER_GRAVITY_Right));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointer->QueryInterface(CLSID_CMarkupPointer,(void**)&pPointer));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    ptpPointer = pPointer->TreePos();
    if (ptpPointer == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pMarkup = ptpPointer->GetMarkup();
    if (pMarkup == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(pDocView->InsertText(wz,cch,pIPointer));


Cleanup:

    if (pIPointer != NULL)
    {
        pIPointer->Unposition();
        ReleaseInterface(pIPointer);
    }

    DumpTwoTrees("After ApplyInsertText:\n",pElementViewRoot);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyReverseInsertText(long cpBase, CDoc *pDoc, CElement * pElementRoot, CInsertTextLogRecord *pLogRecord)
{
    HRESULT hr;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerFinish = NULL;
    IMarkupPointer * pIPointerTarget = NULL;
	long cpStart;
	long cpFinish;

	cpStart = pLogRecord->GetCp();
	cpFinish = cpStart + pLogRecord->GetCch();

    DbgStr(" --- ReverseInsertText");
    DbgInt("  cpStart=",cpStart);
    DbgInt("  cpFinish=",cpFinish);
    DbgStr("'\n");

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerStart)); 
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerFinish)); 
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerStart, cpBase + cpStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerFinish, cpBase + cpFinish));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->CutCopyMove(pIPointerStart,pIPointerFinish,NULL,TRUE));
    if (S_OK != hr)
    {
        goto Cleanup;
    }


Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }

    if (pIPointerFinish != NULL)
    {
        pIPointerFinish->Unposition();
        ReleaseInterface(pIPointerFinish);
    }

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyInsertElement(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertElementLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpDocStart;
    long cpDocFinish;
    long cpViewStart;
    long cpViewFinish;
    long tag;
    long cchAttrs;
    OLECHAR * wzAttrs;
    IMarkupPointer *pIPointerStart = NULL;
    IMarkupPointer *pIPointerEnd = NULL;
    IHTMLElement * pIElementNew = NULL;

    cpDocStart = pLogRecord->GetCpStart();
    cpDocFinish = pLogRecord->GetCpFinish();
    tag = pLogRecord->GetTagId();
    cchAttrs = pLogRecord->GetCchAttrs();
    wzAttrs = pLogRecord->GetAttrs();
    Assert((cchAttrs == 0) || (wzAttrs != NULL));
    if (!((cchAttrs == 0) || (wzAttrs != NULL)))
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    DbgStr(" --- InsertElement");
    DbgInt("  cpDocStart=",cpDocStart);
    DbgInt("  cpDocFinish=",cpDocFinish);
    DbgInt("  tag=",tag);
    DbgInt("  cchAttrs=",cchAttrs);
    DbgStr("  wzAttrs='");
    DbgStrW(wzAttrs);
    DbgStr("'\n");

    // mapper
    cpViewStart = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocStart) + cpViewBase;
    cpViewFinish = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocFinish) + cpViewBase;

    DbgStr("     ");
    DbgInt("  cpViewStart=",cpViewStart - cpViewBase);
    DbgInt("  cpViewFinish=",cpViewFinish - cpViewBase);
    DbgStr("\n");

    // map adjust for the change we make
    hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocFinish,1);
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocStart,1);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerStart,cpViewStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerEnd,cpViewFinish));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    wzAttrs = NULL; // HACK: they don't handle attrs right now (!) so we'll null this out
    hr = THR(pDocView->CreateElement((ELEMENT_TAG_ID)tag,wzAttrs,&pIElementNew));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->InsertElement(pIElementNew,pIPointerStart,pIPointerEnd));


Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }
    if (pIPointerEnd != NULL)
    {
        pIPointerEnd->Unposition();
        ReleaseInterface(pIPointerEnd);
    }
    ReleaseInterface(pIElementNew);

    DumpTwoTrees("After ApplyInsertElement:\n",pElementViewRoot);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyReverseInsertElement(long cpBase, CDoc *pDoc, CElement * pElementRoot, CInsertElementLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpStart;
    IMarkupPointer	*pIPointerStart = NULL;
	CMarkupPointer	*pPointerStart;
	CElement		*pElement;
    IHTMLElement	*pIElement = NULL;
	CTreePos		*ptpStart;
	CTreeNode		*pTreeNode;

    cpStart = pLogRecord->GetCpStart();

    DbgStr(" --- ReverseInsertElement");
    DbgInt("  cpStart=",cpStart);
    DbgStr("'\n");

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerStart, cpStart + cpBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pIPointerStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	ptpStart = pPointerStart->TreePos();
	if (ptpStart == NULL)
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}


    // at this point, ptpStart is the position right before the element's open tag

    // scan until an node-begin position is found (skipping other tree pointers)
	while(!ptpStart->IsBeginNode())
	{
		ptpStart = ptpStart->NextTreePos();
		if (ptpStart == NULL)
		{
			hr = E_UNEXPECTED;
			goto Cleanup;
		}
	}

	pTreeNode = ptpStart->Branch();
	if (pTreeNode == NULL)
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}

	pElement = pTreeNode->Element();
	if (pElement == NULL)
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}

	hr = THR(pElement->QueryInterface(IID_IHTMLElement, (void **)&pIElement));
	if (hr != S_OK)
	{
		goto Cleanup;
	}

	hr = THR(pDoc->RemoveElement(pIElement));

Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }
	ReleaseInterface(pIElement);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyDeleteElement(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CDeleteElementLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpDocStart;
    long cpDocFinish;
    long cpViewStart;
    IMarkupPointer * pIPointerStart = NULL;
    IHTMLElement * pIElementDoomed = NULL;
    CMarkupPointer * pPointerStart;
    CTreePos * ptpStart;
    CTreeNode * pTreeNode;
    CElement * pElementDoomed;

    cpDocStart = pLogRecord->GetCpStart();
    cpDocFinish = pLogRecord->GetCpFinish();

    DbgStr(" --- DeleteElement");
    DbgInt("  cpDocStart=",cpDocStart);
    DbgInt("  cpDocFinish=",cpDocFinish);
    DbgStr("\n");

    // mapper
    cpViewStart = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocStart) + cpViewBase;

    DbgStr("     ");
    DbgInt("  cpViewStart=",cpViewStart - cpViewBase);
    DbgStr("\n");

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerStart,cpViewStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerStart->QueryInterface(CLSID_CMarkupPointer,(void**)&pPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    ptpStart = pPointerStart->TreePos();
    if (ptpStart == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // at this point, ptpStart is the position right before the element's open tag

    // scan until an node-begin position is found (skipping other tree pointers)
    while (!ptpStart->IsBeginNode())
    {
        ptpStart = ptpStart->NextTreePos();
        if (ptpStart == NULL)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }

    // now, ptpStart should be the actual open tag position

    pTreeNode = ptpStart->Branch();
    if (pTreeNode == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pElementDoomed = pTreeNode->Element();
    if (pElementDoomed == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(pElementDoomed->QueryInterface(IID_IHTMLElement,(void**)&pIElementDoomed));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

#if 0 // hack
    // map adjust for the change we make
    hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocFinish,-1);
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocStart,-1);
    if (S_OK != hr)
    {
        goto Cleanup;
    }
#endif

    hr = THR(pDocView->RemoveElement(pIElementDoomed));


Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }
    ReleaseInterface(pIElementDoomed);

    DumpTwoTrees("After ApplyDeleteElement:\n",pElementViewRoot);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyReverseDeleteElement(long cpBase, CDoc *pDoc, CElement * pElementRoot, CDeleteElementLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpStart;
    long cpFinish;
    long tag;
	long cchAttrs;
	OLECHAR *wzAttrs;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerFinish = NULL;
    IHTMLElement * pIElement = NULL;

    cpStart = pLogRecord->GetCpStart();
    cpFinish = pLogRecord->GetCpFinish();
	tag = pLogRecord->GetTagId();
	cchAttrs = pLogRecord->GetCchAttrs();
	wzAttrs = pLogRecord->GetAttrs();

	Assert ((cchAttrs == 0) || (wzAttrs != NULL));
	if (!((cchAttrs == 0) || (wzAttrs != NULL)))
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}

    DbgStr(" --- ReversDeleteElement");
    DbgInt("  cpStart=",cpStart);
    DbgInt("  cpFinish=",cpFinish);
    DbgInt("  tag=",tag);
    DbgInt("  cchAttrs=",cchAttrs);
    DbgStr("  wzAttrs='");
    DbgStrW(wzAttrs);
    DbgStr("'\n");

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerStart));
	if (S_OK != hr)
	{	
		goto Cleanup;
	}
	
	hr = THR(pDoc->CreateMarkupPointer(&pIPointerFinish));
	if (S_OK != hr)
	{	
		goto Cleanup;
	}
	
	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerStart, cpStart + cpBase));
	if (S_OK != hr)
	{	
		goto Cleanup;
	}
	
	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerFinish, cpFinish + cpBase));
	if (S_OK != hr)
	{	
		goto Cleanup;
	}
	
	wzAttrs = NULL; // HACK: they don't handle attrs right now (!) so we'll null this out
	hr = THR(pDoc->CreateElement((ELEMENT_TAG_ID)tag, wzAttrs, &pIElement));
	if (S_OK != hr)
	{	
		goto Cleanup;
	}
	
	
	hr = THR(pDoc->InsertElement(pIElement, pIPointerStart, pIPointerFinish));


Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }

    if (pIPointerFinish != NULL)
    {
        pIPointerFinish->Unposition();
        ReleaseInterface(pIPointerFinish);
    }
    ReleaseInterface(pIElement);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyChangeAttr(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CChangeAttrLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpDocStart;
    long cpViewStart;
    long cchName;
    long cchValNew;
    DWORD dwBits;
    BSTR bstrName = NULL;
    BSTR bstrValNew = NULL;
    IMarkupPointer * pIPointerStart = NULL;
    CMarkupPointer * pPointerStart;
    CTreePos * ptpStart;
    CTreeNode * pTreeNode;
    CElement *pElement;
    VARIANT var; // not cleaned up, since bstrValNew is
    VARIANT_BOOL fSuccess;

    cpDocStart = pLogRecord->GetCpElementStart();
    cchName = pLogRecord->GetCchName();
    bstrName = SysAllocStringLen(pLogRecord->GetName(),cchName);
    cchValNew = pLogRecord->GetCchValNew();
    bstrValNew = SysAllocStringLen(pLogRecord->GetSzValNew(),cchValNew);
    dwBits = pLogRecord->GetDWBits();
    if ((bstrName == NULL) || (bstrValNew == NULL))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    DbgStr(" --- ChangeAttr\n");

    // mapper
    cpViewStart = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocStart) + cpViewBase;

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerStart,cpViewStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // get element from cp

    hr = THR( pIPointerStart->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointerStart ) );
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    ptpStart = pPointerStart->TreePos();
    if (ptpStart == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // at this point, ptpStart is the position right before the element's open tag

    // scan until an node-begin position is found (skipping other tree pointers)
    while (!ptpStart->IsBeginNode())
    {
        ptpStart = ptpStart->NextTreePos();
        if (ptpStart == NULL)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }

    // now, ptpStart should be the actual open tag position

    pTreeNode = ptpStart->Branch();
    if (pTreeNode == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pElement = pTreeNode->Element();
    if (pElement == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (0 != (dwBits & SYNCLOG_CHANGEATTR_NEWNOTNULL))
    {
        var.vt = VT_BSTR;
        var.bstrVal = bstrValNew;
        hr = THR(pElement->setAttribute(bstrName,var,0)); // review: what about "flags" field
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
    else
    {
        hr = THR(pElement->removeAttribute(bstrName,0,&fSuccess)); // review: what about "flags" field
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }


Cleanup:

    SysFreeString(bstrName);
    SysFreeString(bstrValNew);
    ReleaseInterface(pIPointerStart);

    DumpTwoTrees("After ApplyChangeAttr:\n",pElementViewRoot);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyReverseChangeAttr(long cpBase, CDoc *pDoc, CElement * pElementRoot, CChangeAttrLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpStart;
    long cchName;
    long cchValOld;
    DWORD dwBits;
    BSTR bstrName = NULL;
    BSTR bstrValOld = NULL;

    IMarkupPointer * pIPointerStart = NULL;
    CMarkupPointer * pPointerStart;
    CTreePos * ptpStart;
    CTreeNode * pTreeNode;
    CElement *pElement;
    VARIANT var; // not cleaned up
    VARIANT_BOOL fSuccess;

    cpStart		= pLogRecord->GetCpElementStart();
    cchName		= pLogRecord->GetCchName();
    bstrName	= SysAllocStringLen(pLogRecord->GetName(),cchName);
    cchValOld	= pLogRecord->GetCchValOld();
    bstrValOld	= SysAllocStringLen(pLogRecord->GetSzValOld(),cchValOld);
    dwBits		= pLogRecord->GetDWBits();

    if ((bstrName == NULL) || (bstrValOld == NULL))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    DbgStr(" --- ReversChangeAttr\n");

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerStart));
	if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerStart,cpStart + cpBase));
	if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pIPointerStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerStart));
	if (S_OK != hr)
    {
        goto Cleanup;
    }

	ptpStart = pPointerStart->TreePos();
	if (ptpStart == NULL)
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}
	
	while(!ptpStart->IsBeginNode())
	{
		ptpStart = ptpStart->NextTreePos();
		if (ptpStart == NULL)
		{
			hr = E_UNEXPECTED;
			goto Cleanup;
		}
	}

	pTreeNode = ptpStart->Branch();
	if (pTreeNode == NULL)
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}

	pElement = pTreeNode->Element();
	if (pElement == NULL)
	{
		hr = E_UNEXPECTED;
		goto Cleanup;
	}

	if ((dwBits & SYNCLOG_CHANGEATTR_OLDNOTNULL) != 0)
	{
		var.vt = VT_BSTR;
		var.bstrVal = bstrValOld;
		pElement->setAttribute(bstrName, var, 0); // review: what about "flags" field
	}
	else
	{
		pElement->removeAttribute(bstrName,0,&fSuccess); // review: what about "flags" field
	}

Cleanup:

    SysFreeString(bstrName);
    SysFreeString(bstrValOld);
    ReleaseInterface(pIPointerStart);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyInsertTree(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CInsertTreeLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpStart;
    long cpFinish;
    long cpDocTarget;
    long cpViewTarget;
    long cchHTML;
    OLECHAR * wzHTML;
    IMarkupContainer * pIContainer = NULL;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerEnd = NULL;
    IMarkupPointer * pIPointerTarget = NULL;
    IMarkupPointer * pIPointerTargetTail = NULL;
    CMarkupPointer * pStart;
    CMarkupPointer * pEnd;
    CMarkupPointer * pTarget;
    CMarkupPointer * pTargetTail;
    CTreePos * ptpStart;
    CMarkup * pMarkupAux;
    long cchNewTargetSize;

    cpStart = pLogRecord->GetCpStart(); // aux tree, don't add cpViewBase!
    cpFinish = pLogRecord->GetCpFinish(); // aux tree, don't add cpViewBase!
    cpDocTarget = pLogRecord->GetCpTarget();
    cchHTML = pLogRecord->GetCchHTML();
    wzHTML = pLogRecord->GetHTML();
    Assert((cchHTML == 0) || (wzHTML != NULL));
    if (!((cchHTML == 0) || (wzHTML != NULL)))
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    Assert((wzHTML == NULL) || (cchHTML == (long)_tcslen(wzHTML))); // must be null terminated

    DbgStr(" --- InsertTree\n");

    // mapper
    cpViewTarget = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocTarget) + cpViewBase;

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->ParseString(wzHTML,0,&pIContainer,pIPointerStart,pIPointerEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerStart->QueryInterface(CLSID_CMarkupPointer,(void**)&pStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerEnd->QueryInterface(CLSID_CMarkupPointer,(void**)&pEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    ptpStart = pStart->TreePos();
    if (ptpStart == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pMarkupAux = ptpStart->GetMarkup();
    if (pMarkupAux == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(pStart->MoveToCp(cpStart,pMarkupAux));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pEnd->MoveToCp(cpFinish,pMarkupAux));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerTarget));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerTarget,cpViewTarget));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerTargetTail));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerTargetTail->MoveToPointer(pIPointerTarget));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerTarget->SetGravity(POINTER_GRAVITY_Left));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerTargetTail->SetGravity(POINTER_GRAVITY_Right));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerTarget->QueryInterface(CLSID_CMarkupPointer,(void**)&pTarget));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointerTargetTail->QueryInterface(CLSID_CMarkupPointer,(void**)&pTargetTail));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CutCopyMove(pIPointerStart,pIPointerEnd,pIPointerTarget,true/*fRemove -- hopefully makes splice faster*/));

    // map adjust for the change we make
    cchNewTargetSize = pTargetTail->TreePos()->GetCp() - pTarget->TreePos()->GetCp();
    hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocTarget,cchNewTargetSize);
    if (S_OK != hr)
    {
        goto Cleanup;
    }


Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }
    if (pIPointerEnd != NULL)
    {
        pIPointerEnd->Unposition();
        ReleaseInterface(pIPointerEnd);
    }
    if (pIPointerTarget != NULL)
    {
        pIPointerTarget->Unposition();
        ReleaseInterface(pIPointerTarget);
    }
    if (pIPointerTargetTail != NULL)
    {
        pIPointerTargetTail->Unposition();
        ReleaseInterface(pIPointerTargetTail);
    }
    ReleaseInterface(pIContainer);

    DumpTwoTrees("After ApplyInsertTree:\n",pElementViewRoot);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyReverseInsertTree(long cpBase, CDoc *pDoc, CElement * pElementRoot, CInsertTreeLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpStart;
    long cpFinish;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerFinish = NULL;

    cpStart = pLogRecord->GetCpTarget(); 
    cpFinish = cpStart + pLogRecord->GetCpFinish() - pLogRecord->GetCpStart();

    DbgStr(" --- ReversInsertTree\n");

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerStart)); 
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerFinish)); 
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerStart, cpStart + cpBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerFinish, cpFinish + cpBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

	hr = THR(pDoc->CutCopyMove(pIPointerStart,pIPointerFinish,NULL,TRUE));	

Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }

    if (pIPointerFinish != NULL)
    {
        pIPointerFinish->Unposition();
        ReleaseInterface(pIPointerFinish);
    }

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyCutCopyMove(long cpViewBase, CDoc *pDocView, CElement * pElementViewRoot, CCutCopyMoveTreeLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpDocStart;
    long cpDocFinish;
    long cpDocTarget;
    long cpViewStart;
    long cpViewFinish;
    long cpViewTarget;
    BOOL fRemove;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerFinish = NULL;
    IMarkupPointer * pIPointerTarget = NULL;
    IMarkupPointer * pIPointerTargetTail = NULL;
    CMarkupPointer * pStart;
    CMarkupPointer * pFinish;
    CMarkupPointer * pTarget;
    CMarkupPointer * pTargetTail;
    long cpStartAdjust;
    long cpTargetAdjust;
#if 0
    long cpStartGarbageAddIn;
#endif

    cpDocStart = pLogRecord->GetCpStart();
    cpDocFinish = pLogRecord->GetCpFinish();
    cpDocTarget = pLogRecord->GetCpTarget();
    fRemove = pLogRecord->GetFRemove();

    DbgStr(" --- CutCopyMove");
    DbgInt("  cpDocStart=",cpDocStart);
    DbgInt("  cpDocFinish=",cpDocFinish);
    DbgInt("  cpDocTarget=",cpDocTarget);
    DbgInt("  fRemove=",fRemove);
    DbgStr("\n");

#if 0
    // map validation -- can't delete if junk is in the view-only area
    // review(tomlaw): since we can't fail it, lets just fix it up so it's robust
    cpStartGarbageAddIn = 0;
    if (fRemove)
    {
        long cpStartDelta;
        long cpFinishDelta;

        cpStartDelta = cpDocStart - MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocStart);
        cpFinishDelta = cpDocFinish - MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocFinish);
        cpStartGarbageAddIn = cpFinishDelta - cpStartDelta;
    }
#endif

    // mapper
    cpViewStart = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocStart) + cpViewBase;
    cpViewFinish = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocFinish) + cpViewBase;
    if ((DWORD)cpDocTarget != (DWORD)-1)
    {
        cpViewTarget = MapDocCpToViewCp(pDocView,pElementViewRoot,cpDocTarget) + cpViewBase;
    }
    else
    {
        cpViewTarget = -1;
    }

    DbgStr("     ");
    DbgInt("  cpViewStart=",cpViewStart - cpViewBase);
    DbgInt("  cpViewFinish=",cpViewFinish - cpViewBase);
    DbgInt("  cpViewTarget=",(DWORD)cpViewTarget != (DWORD)-1 ? cpViewTarget - cpViewBase : -1);
    DbgStr("\n");

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->CreateMarkupPointer(&pIPointerFinish));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerStart,cpViewStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerFinish,cpViewFinish));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    if ((DWORD)cpDocTarget != (DWORD)-1)
    {
        hr = THR(pDocView->CreateMarkupPointer(&pIPointerTarget));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(pDocView->_pqdocGlue->MovePointerToCPos(pIPointerTarget,cpViewTarget));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(pIPointerTarget->SetGravity(POINTER_GRAVITY_Left));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        //

        hr = THR(pDocView->CreateMarkupPointer(&pIPointerTargetTail));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(pIPointerTargetTail->MoveToPointer(pIPointerTarget));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(pIPointerTargetTail->SetGravity(POINTER_GRAVITY_Right));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }

    hr = THR(pDocView->CutCopyMove(pIPointerStart,pIPointerFinish,pIPointerTarget,fRemove));

    // compute start adjustment
    hr = THR(pIPointerStart->QueryInterface(CLSID_CMarkupPointer,(void**)&pStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = THR(pIPointerFinish->QueryInterface(CLSID_CMarkupPointer,(void**)&pFinish));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    cpStartAdjust = (cpDocFinish - cpDocStart) - (pFinish->TreePos()->GetCp() - pStart->TreePos()->GetCp());

    // compute target adjustment
    cpTargetAdjust = 0;
    if ((DWORD)cpDocTarget != (DWORD)-1)
    {
        hr = THR(pIPointerTarget->QueryInterface(CLSID_CMarkupPointer,(void**)&pTarget));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        hr = THR(pIPointerTargetTail->QueryInterface(CLSID_CMarkupPointer,(void**)&pTargetTail));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        cpTargetAdjust = pTargetTail->TreePos()->GetCp() - pTarget->TreePos()->GetCp();
    }

    // adjust
    if ((cpTargetAdjust != 0) && (cpDocStart > cpDocTarget))
    {
        // do source first, since it's after target
        if (fRemove)
        {
            hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocStart,- cpStartAdjust/* - cpStartGarbageAddIn*/);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
        hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocTarget,cpTargetAdjust);
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
    else
    {
        // target first
        if ((DWORD)cpDocTarget != (DWORD)-1)
        {
            hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocTarget,cpTargetAdjust);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
        if (fRemove)
        {
            hr = AdjustDocCp(pDocView,pElementViewRoot,cpDocStart,- cpStartAdjust/* - cpStartGarbageAddIn*/);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
    }


Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }
    if (pIPointerFinish != NULL)
    {
        pIPointerFinish->Unposition();
        ReleaseInterface(pIPointerFinish);
    }
    if (pIPointerTarget != NULL)
    {
        pIPointerTarget->Unposition();
        ReleaseInterface(pIPointerTarget);
    }
    if (pIPointerTargetTail != NULL)
    {
        pIPointerTargetTail->Unposition();
        ReleaseInterface(pIPointerTargetTail);
    }

    DumpTwoTrees("After ApplyCutCopyMove:\n",pElementViewRoot);

    RRETURN(hr);
}


HRESULT
CTreeSyncLogger::ApplyReverseCutCopyMove(long cpBase, CDoc *pDoc, CElement * pElementRoot, CCutCopyMoveTreeLogRecord *pLogRecord)
{
    HRESULT hr;
    long cpStart;
    long cpFinish;
    long cpTarget;
    BOOL fRemove;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerFinish = NULL;
    IMarkupPointer * pIPointerTarget = NULL;
    CMarkupPointer * pStart;
    CMarkupPointer * pFinish;
	IMarkupContainer * pIContainer = NULL;

	// There are four cases here:
	//
	// cpStart cpFinish cpTarget fRemove |      |      | cpStart cpFinish cpTarget fRemove 
	// -----------------------------------------------------------------------------------
	//    a      a + b     -1       T    | cut  | copy |    -1    -1 + b     a        F
	//    a      a + b     -1       F    | ?    | ?    |    -1    -1 + b     a        F
	//    a      a + b     c        T    | move | move |    c      c + b     a        T
	//    a      a + b     c        F    | copy | cut  |    c      c + b     0        T
	 
		
    cpTarget  = pLogRecord->GetCpStart();
    cpStart   = pLogRecord->GetCpTarget();
    cpFinish  = cpStart + pLogRecord->GetCpFinish() - cpTarget;
    fRemove   = pLogRecord->GetFRemove();

    DbgStr(" --- ReverseCutCopyMove");
    DbgInt("  cpStart=", cpStart);
    DbgInt("  cpFinish=",cpFinish);
    DbgInt("  cpTarget=",cpTarget);
    DbgInt("  fRemove=",fRemove);
    DbgStr("\n");

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerStart)); 
	if (S_OK != hr)
	{
		goto Cleanup;
	}

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerFinish));
	if (S_OK != hr)
	{
		goto Cleanup;
	}

	hr = THR(pDoc->CreateMarkupPointer(&pIPointerTarget));
	if (S_OK != hr)
	{
		goto Cleanup;
	}

	if (cpStart == -1)
	{
		if (fRemove)
		{
			//  there was a Cut operation and we should Copy from enclosed HTML
			OLECHAR				*wzOldHTML;
			long				cchOldHTML;
			long				cpOldHTMLStart;
			long				cpOldHTMLFinish;
			CMarkup				*pMarkupAux;
			CTreePos			*ptpStart;
			long				cpAuxParseStart;

			wzOldHTML = pLogRecord->GetOldHTML();
			cchOldHTML = pLogRecord->GetCchOldHTML();
			cpOldHTMLStart = pLogRecord->GetCpOldHTMLStart();
			cpOldHTMLFinish = pLogRecord->GetCpOldHTMLFinish();

            hr = THR(pDoc->ParseString(wzOldHTML, 0, &pIContainer, pIPointerStart, pIPointerFinish));
			if (S_OK != hr)
			{
				goto Cleanup;
			}

			// We should move the IPointerStart and IPointerFinishn.
			hr = THR(pIPointerStart->QueryInterface(CLSID_CMarkupPointer,(void**)&pStart));
			if (S_OK != hr)
			{
				goto Cleanup;
			}

			hr = THR(pIPointerFinish->QueryInterface(CLSID_CMarkupPointer,(void**)&pFinish));
			if (S_OK != hr)
			{
				goto Cleanup;
			}

			ptpStart = pStart->TreePos();
			if (ptpStart == NULL)
			{
				hr = E_UNEXPECTED;
				goto Cleanup;
			}

			pMarkupAux = ptpStart->GetMarkup();
			if (pMarkupAux == NULL)
			{
				hr = E_UNEXPECTED;
				goto Cleanup;
			}

			cpAuxParseStart = ptpStart->GetCp();

			hr = THR(pStart->MoveToCp(cpOldHTMLStart + cpAuxParseStart,pMarkupAux));
			if (S_OK != hr)
			{
				goto Cleanup;
			}

			hr = THR(pFinish->MoveToCp(cpOldHTMLFinish + cpAuxParseStart,pMarkupAux));
			if (S_OK != hr)
			{
				goto Cleanup;
			}

			hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerTarget,cpTarget + cpBase));
			if (S_OK != hr)
			{
				goto Cleanup;
			}

			fRemove = FALSE;
		}
		else 
		{
			// unexpected combination
			hr = E_UNEXPECTED;
			goto Cleanup;
		}
	}			
	else
	{
		hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerStart, cpStart));
		if (S_OK != hr)
		{
			goto Cleanup;
		}
		hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerFinish, cpFinish));			
		if (S_OK != hr)
		{
			goto Cleanup;
		}

		if (fRemove)
		{
			// there was a Move operation. We should move it back
			hr = THR(pDoc->_pqdocGlue->MovePointerToCPos(pIPointerTarget, cpTarget + cpBase));
			if (S_OK != hr)
			{
				goto Cleanup;
			}
		}
		else 
		{
			// there was a Copy operation, using Cut
			pIPointerTarget = NULL;
			fRemove = TRUE;
		}
	}

	hr = THR(pDoc->CutCopyMove(pIPointerStart, pIPointerFinish, pIPointerTarget, fRemove));

Cleanup:

    if (pIPointerStart != NULL)
    {
        pIPointerStart->Unposition();
        ReleaseInterface(pIPointerStart);
    }
    if (pIPointerFinish != NULL)
    {
        pIPointerFinish->Unposition();
        ReleaseInterface(pIPointerFinish);
    }
    if (pIPointerTarget != NULL)
    {
        pIPointerTarget->Unposition();
        ReleaseInterface(pIPointerTarget);
    }
	ReleaseInterface(pIContainer);

    RRETURN(hr);
}


// send the log
HRESULT
CTreeSyncLogger::SyncLoggerSend(CElement *pElementSyncRoot)
{
    ITreeSyncBehavior *pTreeSyncBehavior = NULL;
    HRESULT hr = E_UNEXPECTED;
    long cpLogAdjust = 0;
    long cpFirstBase;
    long cpThisBase;
    CDoc *pDocData;
    CTreeNode *pNode;
    IHTMLElement * pIElementSyncRoot = NULL;
    int i;

    // arg check

    if (NULL == pElementSyncRoot)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    hr = THR(pElementSyncRoot->QueryInterface(IID_IHTMLElement,(void**)&pIElementSyncRoot));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = THR(pElementSyncRoot->Doc()->_pqdocGlue->GetBindBehavior(pIElementSyncRoot,&pTreeSyncBehavior));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    //  propagate to all mirrors from pElementSyncRoot to Root

    pDocData = pElementSyncRoot->Doc();

    hr = THR(pDocData->_pqdocGlue->GetSyncBaseIndex(pElementSyncRoot, &cpFirstBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    while (pElementSyncRoot)
    {
        // send to all registered clients

        for (i = 0; i < _aryLogSinks.Size(); i ++)
        {
            CTreeSyncLogger::SINKREC    rec = _aryLogSinks.Item(i);

            (void)rec._pSink->ReceiveStreamData(pIElementSyncRoot,(BYTE*)_bufOut.GetBuffer(),_bufOut.GetIndex(),cpLogAdjust);
        }

        //  Now, walk up to the next tree sync root (usually just the one!)
        for (pNode = pElementSyncRoot->GetFirstBranch()->Parent(); pNode; pNode = pNode->Parent())
        {
            pElementSyncRoot = pNode->Element();
            ClearInterface(&pIElementSyncRoot);
            hr = THR(pElementSyncRoot->QueryInterface(IID_IHTMLElement,(void**)&pIElementSyncRoot));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            ClearInterface(&pTreeSyncBehavior);
            if (pElementSyncRoot->Tag() == ETAG_ROOT ||
                S_OK == pElementSyncRoot->Doc()->_pqdocGlue->GetBindBehavior(pIElementSyncRoot,&pTreeSyncBehavior))
            {
                break;
            }
        }
        if (NULL == pTreeSyncBehavior)
        {
            break;
        }
        hr = THR(pDocData->_pqdocGlue->GetSyncBaseIndex(pElementSyncRoot, &cpThisBase));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        cpLogAdjust = cpFirstBase - cpThisBase;
    }

    // reset the log buffer so we can get new ones
    _bufOut.Shutdown();
    _bufOut.Init(&_bufpool); // reinit as write (output) buffer


Cleanup:

    ReleaseInterface(pTreeSyncBehavior);
    ReleaseInterface(pIElementSyncRoot);

    return S_OK;
}


HRESULT
CQDocGlue::ApplyForward(
		IHTMLElement * pIElemTreeSyncRoot,
		BYTE * rgbData,
		DWORD cbData,
        long cpRootBaseAdjust)
{
    return _SyncLogger.ApplyLog(pIElemTreeSyncRoot,rgbData,cbData,cpRootBaseAdjust,true/*forward*/);
}


HRESULT
CQDocGlue::ApplyForward1(
	IHTMLElement * pElemTreeSyncRoot,
	DWORD opcode,
	BYTE *rgbStruct,
	long cpRootBaseAdjust)
{
    return _SyncLogger.ApplyForward1(pElemTreeSyncRoot,opcode,rgbStruct,cpRootBaseAdjust);
}


HRESULT
CQDocGlue::ApplyReverse(
		IHTMLElement * pIElemTreeSyncRoot,
		BYTE * rgbData,
		DWORD cbData,
        long cpRootBaseAdjust)
{
    return _SyncLogger.ApplyLog(pIElemTreeSyncRoot,rgbData,cbData,cpRootBaseAdjust,false/*reverse*/);
}


HRESULT
CTreeSyncLogger::ApplyLog(
		IHTMLElement * pIElementSyncRoot,
		BYTE * rgbData,
		DWORD cbData,
        long cpRootBaseAdjust,
        bool fForward)
{
    HRESULT hr = E_UNEXPECTED;
    CDoc *pDoc;
    CElement *pElementSyncRoot;
    long cpViewBase;
    CLogBuffer Buffer;

    // arg check

    Buffer.Init(rgbData,cbData); // read buffer

    if (NULL == pIElementSyncRoot)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    hr = THR(pIElementSyncRoot->QueryInterface(CLSID_CElement,(void**)&pElementSyncRoot));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // get the internal objects corresponding to the arguments

    pDoc = pElementSyncRoot->Doc();
    if (NULL == pDoc)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(pDoc->_pqdocGlue->GetSyncBaseIndex(pElementSyncRoot, &cpViewBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    cpViewBase += cpRootBaseAdjust;

    // sync the trees

    if (fForward)
    {
        // FORWARD application

        while (!Buffer.IsEndOfBuffer())
        {
            TreeOps op;

            hr = THR(Buffer.SkipDWORD()); // skip leading length delimiter

            hr = THR(Buffer.ReadOp(&op));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            switch (op)
            {
            default:
                AssertSz(false,"unhandled opcode in tree-sync while applying to target tree");
                hr = E_UNEXPECTED;
                break;

            case synclogInsertText:
                {
                    CInsertTextLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyInsertText(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogInsertElement:
                {
                    CInsertElementLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyInsertElement(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogDeleteElement:
                {
                    CDeleteElementLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyDeleteElement(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogChangeAttr:
                {
                    CChangeAttrLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyChangeAttr(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogInsertTree:
                {
                    CInsertTreeLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyInsertTree(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogCutCopyMove:
                {
                    CCutCopyMoveTreeLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyCutCopyMove(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;
            } // switch
        } // loop
    }
    else
    {
        // REVERSE application

        Buffer.SetIndex(cbData);

        while (Buffer.GetIndex() > 0)
        {
            TreeOps op;
            DWORD dwRecSize;

            hr = THR(Buffer.BackReadPrevDWORD(&dwRecSize));

            Buffer.SetIndex(Buffer.GetIndex() - (dwRecSize - sizeof(DWORD)));

            hr = THR(Buffer.SkipDWORD()); // skip leading length delimiter

            hr = THR(Buffer.ReadOp(&op));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            switch (op)
            {
            default:
                AssertSz(false,"unhandled opcode in tree-sync while applying to target tree");
                hr = E_UNEXPECTED;
                break;

            case synclogInsertText:
                {
                    CInsertTextLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyReverseInsertText(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogInsertElement:
                {
                    CInsertElementLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyReverseInsertElement(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogDeleteElement:
                {
                    CDeleteElementLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyReverseDeleteElement(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogChangeAttr:
                {
                    CChangeAttrLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyReverseChangeAttr(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogInsertTree:
                {
                    CInsertTreeLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyReverseInsertTree(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;

            case synclogCutCopyMove:
                {
                    CCutCopyMoveTreeLogRecord LogRec(&Buffer,&hr);
                    THR(hr);
                    if (hr == S_OK)
                    {
                        hr = THR(ApplyReverseCutCopyMove(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
                    }
                }
                break;
            } // switch

            Buffer.SetIndex(Buffer.GetIndex() - dwRecSize);
        } // loop
    }

Cleanup:
    RRETURN(S_OK);
}


HRESULT
CTreeSyncLogger::ApplyForward1(
	IHTMLElement * pIElementSyncRoot,
	DWORD opcode,
	BYTE *rgbStruct,
	long cpRootBaseAdjust)
{
    HRESULT hr = E_UNEXPECTED;
    CDoc *pDoc;
    CElement *pElementSyncRoot;
    long cpViewBase;

    // arg check

    if (NULL == pIElementSyncRoot)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    hr = THR(pIElementSyncRoot->QueryInterface(CLSID_CElement,(void**)&pElementSyncRoot));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // get the internal objects corresponding to the arguments

    pDoc = pElementSyncRoot->Doc();
    if (NULL == pDoc)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(pDoc->_pqdocGlue->GetSyncBaseIndex(pElementSyncRoot, &cpViewBase));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    cpViewBase += cpRootBaseAdjust;

    // sync the trees

    switch (opcode)
    {
    default:
        AssertSz(false,"unhandled opcode in tree-sync while applying to target tree");
        hr = E_UNEXPECTED;
        break;

    case synclogInsertText:
        {
            CInsertTextLogRecord LogRec((INSERTTEXTREC*)rgbStruct);
            hr = THR(ApplyInsertText(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
        }
        break;

    case synclogInsertElement:
        {
            CInsertElementLogRecord LogRec((INSDELELEMENTREC*)rgbStruct);
            hr = THR(ApplyInsertElement(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
        }
        break;

    case synclogDeleteElement:
        {
            CDeleteElementLogRecord LogRec((INSDELELEMENTREC*)rgbStruct);
            hr = THR(ApplyDeleteElement(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
        }
        break;

    case synclogChangeAttr:
        {
            CChangeAttrLogRecord LogRec((CHANGEATTRREC*)rgbStruct);
            hr = THR(ApplyChangeAttr(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
        }
        break;

    case synclogInsertTree:
        {
            CInsertTreeLogRecord LogRec((INSERTTREEREC*)rgbStruct);
            hr = THR(ApplyInsertTree(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
        }
        break;

    case synclogCutCopyMove:
        {
            CCutCopyMoveTreeLogRecord LogRec((CUTCOPYMOVEREC*)rgbStruct);
            hr = THR(ApplyCutCopyMove(cpViewBase,pDoc,pElementSyncRoot,&LogRec));
        }
        break;
    } // switch

Cleanup:
    RRETURN(S_OK);
}


HRESULT
CTreeSyncLogger::SetAttributeHandler(CBase *pbase, BSTR strPropertyName, VARIANT * pvarPropertyValue, LONG lFlags, bool *pfDoItOut, CElement **ppElementSyncRoot)
{
    HRESULT hr;
    CChangeAttrLogRecord LogRecord;
    long cpPointerStart;
    long cpPointerFinish;
    IHTMLElement * pIElement = NULL;
    CElement * pElementSyncRoot = NULL;
    VARIANT varOld;
    VariantInit(&varOld);
    VARIANT varNew;
    VariantInit(&varNew);
    VARIANT varTemp;
    VariantInit(&varTemp);
    CDoc * pDoc;
    CElement * pEl;
    DWORD dwBits = 0;

    *pfDoItOut = false;
    *ppElementSyncRoot = NULL;

    hr = pbase->PrivateQueryInterface(IID_IHTMLElement,(void**)&pIElement);
    if (SUCCEEDED(hr))
    {
        hr = pIElement->QueryInterface(CLSID_CElement,(void**)&pEl);
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        pDoc = pEl->Doc();
        if (pDoc == NULL)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        if (pDoc->_pqdocGlue->_SyncLogger.IsLogging(pDoc))
        {
            // get element cp's
            hr = THR(pDoc->_pqdocGlue->GetSyncInfoFromElement(pIElement, 
                                                  &pElementSyncRoot, 
                                                  &cpPointerStart,
                                                  &cpPointerFinish));
            //  Don't log if there is no mapping (not in mapable
            //  part of tree
            if (S_OK != hr)
            {
                hr = S_OK;
                goto Cleanup;
            }

            hr = pEl->getAttribute(strPropertyName,0,&varOld);
            if (SUCCEEDED(hr))
            {
                if (varOld.vt != VT_NULL)
                {
                    dwBits |= SYNCLOG_CHANGEATTR_OLDNOTNULL;
                }
                hr = THR(VariantChangeTypeSpecial ( &varTemp, &varOld, VT_BSTR, NULL, VARIANT_NOVALUEPROP ));
                Assert(SUCCEEDED(hr));
                VariantClear(&varOld);
                varOld = varTemp;
            }

            if (pvarPropertyValue != NULL)
            {
                dwBits |= SYNCLOG_CHANGEATTR_NEWNOTNULL;
            }

            if (pvarPropertyValue != NULL)
            {
                hr = THR(VariantChangeTypeSpecial ( &varNew, pvarPropertyValue, VT_BSTR, NULL, VARIANT_NOVALUEPROP ));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }
            else
            {
                varNew.vt = VT_VOID;
            }

            LogRecord.SetCpElementStart(cpPointerStart);

            hr = THR(LogRecord.SetName(strPropertyName,SysStringLen(strPropertyName)));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            if (varOld.vt == VT_BSTR)
            {
                hr = THR(LogRecord.SetSzValOld(varOld.bstrVal,SysStringLen(varOld.bstrVal)));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }

            if (varNew.vt == VT_BSTR)
            {
                hr = THR(LogRecord.SetSzValNew(varNew.bstrVal,SysStringLen(varNew.bstrVal)));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }

            LogRecord.SetDWBits(dwBits);

            hr = THR(LogRecord.Write(&(pDoc->_pqdocGlue->_SyncLogger._bufOut)));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            *pfDoItOut = true;
        }
    }

    *ppElementSyncRoot = pElementSyncRoot;

Cleanup:
    ReleaseInterface(pIElement);
    VariantClear(&varOld);
    VariantClear(&varNew);
    return S_OK;
}




HRESULT
CQDocGlue::RegisterLogSink(ITreeSyncLogSink * pLogSink)
{
    HRESULT                     hr;
    CTreeSyncLogger::SINKREC    rec;

    rec._pSink = pLogSink;

    hr = THR(_SyncLogger._aryLogSinks.AppendIndirect(&rec));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    pLogSink->AddRef();

Cleanup:

    RRETURN(hr);
}


HRESULT
CQDocGlue::UnregisterLogSink(ITreeSyncLogSink * pLogSink)
{
    HRESULT                     hr = S_OK;
    int                         i;
    CTreeSyncLogger::SINKREC    rec;

    rec._pSink = pLogSink;

    i = _SyncLogger._aryLogSinks.FindIndirect(&rec);
    if (i < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    _SyncLogger._aryLogSinks.Delete(i);

    pLogSink->Release();

Cleanup:

    RRETURN(hr);
}


// temporary method, until we have the real stuff going
HRESULT
CQDocGlue::RebuildCpMap(IHTMLElement * pIElementSyncRoot, ITreeSyncRemapHack * pSyncUpdateCallback)
{
    HRESULT hr;
    CElement * pElem;

    hr = THR(pIElementSyncRoot->QueryInterface(CLSID_CElement,(void**)&pElem));
    if (S_OK != hr)
    {
        return hr;
    }

    if (pElem->Doc() != _pDoc)
    {
        return E_INVALIDARG;
    }

    return ::RebuildCpMap(pElem->Doc(),pElem,pSyncUpdateCallback);
}

HRESULT
CQDocGlue::GetSyncBaseIndexI(IHTMLElement * pElementSyncRoot, long * pcpRelative)
{
    HRESULT hr;
    CElement * pElem;

    hr = THR(pElementSyncRoot->QueryInterface(CLSID_CElement,(void**)&pElem));
    if (S_OK != hr)
    {
        return hr;
    }

    if (pElem->Doc() != _pDoc)
    {
        return E_INVALIDARG;
    }

    return GetSyncBaseIndex(pElem,pcpRelative);
}
