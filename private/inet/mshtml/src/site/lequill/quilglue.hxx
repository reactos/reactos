//+----------------------------------------------------------------------------
//
// File:        quilglue.HXX
//
// Contents:    CQuillGlue and related classes
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef _QUILGLUE_HXX_
#define _QUILGLUE_HXX_

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_QUILLSITE_H_
#define X_QUILLSITE_H_
#include "quillsite.h"
#endif

class CFlowLayout;

MtExtern(CQuillGlue)

// global pointer to free-threaded Quill Layout Manager
extern ITextLayoutManager *g_pQLM;

HRESULT InitQuillLayout();
void DeinitQuillLayout();

class CQuillGlue  : public ITextStory, public ITextLayoutSite, public ITextInPlacePaint,
                    public ILineServicesHost
{
public:

    friend class CFlowLayout;
    friend class CQuillNotificationWrapper;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CQuillGlue))

    //
    // IUnknown declarations
    //

    DECLARE_FORMS_STANDARD_IUNKNOWN(CQuillGlue)

//// Internal helper functions
protected:
    CLayout *GetOwnerLayoutAtCp(long cpQuill);
    long CpAbsFromCpRel(long cpQuill);
    long CpRelFromCpAbs(long cpTrident);
    void GetViewTopLeft(long *pLeft, long *pTop);
    CElement *GetFlowLayoutElement() const;
    void SyncTreePointerToCp(CTreePos **pptp, long cpAbs, long *pcchOffsetOfCpAbsFromTPStart);
    CLayout *GetNestedLayoutFromTreePointer(CTreePos **pptp);

//// CFlowLayout helper functions
public:
    HRESULT Init(CFlowLayout *pfl);
    void Detach();
    void Draw(CFormDrawInfo *pDI, RECT *prcView);
    void DrawRelativeElement(CElement * pElement, CFormDrawInfo * pDI);
    LONG Render(CFormDrawInfo * pDI,
                const RECT &rcView,
                const RECT &rcRender,
                LONG iliStop = -1);
    HRESULT ExecCommand(long qcmd);
    HRESULT Notify(CNotification *pnf);
    void NukeLayout();
#ifndef DISPLAY_TREE
    DWORD SetPosition(CPositionInfo * ppi, RECT * prcPosition);
#endif
    inline ITextLayoutElement *GetTextLayout() {return m_ptle;}
    BOOL RecalcView(CCalcInfo * pci, BOOL fFullRecalc);
    void RecalcLineShift(CCalcInfo * pci, DWORD grfLayout);
    BOOL HasLines();
    HRESULT GetCharacterMetrics(long cp, long Type, RECT *prc, long *px, long *py);
    HRESULT HandleMessage(CMessage * pMessage, CElement * pElem, CTreeNode *pNodeContext);
    LONG    CpFromPoint(POINT pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout ** ppLayout,
                        BOOL fAllowEOL,
                        BOOL fIgnoreBeforeAfterSpace = FALSE,
                        BOOL fExactFit = FALSE,
                        BOOL *pfRightOfCp = NULL,
                        BOOL *pfPsuedoHit = NULL,
                        CCalcInfo * pci = NULL, 
                        BOOL *pfEOL = NULL); 
    void RegionFromElement(CElement * pElement,
                           CDataAry<RECT> * paryRects,
                           LONG cpStart,
                           LONG cpFinish,
                           LONG cpClipStart,
                           LONG cpClipFinish,
                           BOOL fRelativeToView,
                           BOOL fRestrictToVisible,
                           BOOL fBlockElement,
                           RECT * prcBoundingRect);
    LONG GetMaxXScrollFromDisplay();
    LONG GetMaxYScrollFromDisplay();
#ifndef DISPLAY_TREE
    HRESULT PositionAndLayoutObjectsForScroll(CPositionInfo * ppi, LONG dx, LONG dy);
#endif
    HRESULT GetLineInfoFromPoint(LONG xPos, LONG yPos, BOOL fSkipFrame, LONG xExposedWidth,
                        BOOL fIgnoreRelLines, LONG *pyLine, LONG *pcpFirst);
	HRESULT GetLineInfo(CTreePos *pTP, CFlowLayout *pFlowLayout, BOOL fAtEndOfLine, HTMLPtrDispInfoRec *pInfo);

    LONG GetMaxCpCalced();

	HRESULT MoveMarkupPointer(CMarkupPointer *pMarkupPointer, LAYOUT_MOVE_UNIT eUnit, LONG lXCurReally, BOOL fEOL);

	VOID ShowSelected(CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected);

//// CFlowLayout data access functions
public:
    LONG GetMMWIDTHMinWidth() { return m_xMinWidth; }
    LONG GetMMWIDTHMaxWidth() { return m_xMaxWidth; }
    BOOL FMinMaxValid() { return m_fMinMaxValid; }
    BOOL NoContent() { return m_fNoContent; }
    void ResetMinMax() { m_fMinMaxValid = FALSE; }
    LONG GetNATURALBestHeight() { return m_yBestHeight; }
    LONG GetNATURALBaseline() { return m_yBaseline; }
    LONG GetLongestLineWidth();
    void SetMinWidth(long xWidth) { m_xMinWidth = xWidth; }
    void SetMaxWidth(long xWidth) { m_xMaxWidth = xWidth; }
    void SetLongestLineWidth(long xWidth) { m_xWidthLongestLine = xWidth; }

//// ITextStory implementation
protected:
	STDMETHODIMP GetLength(long *pcpMac);
	STDMETHODIMP GetPapLimits(long cp, long *pcpFirst, long *pcpLim);
	STDMETHODIMP GetRunLimits(long cp, long *pcpFirst, long *pcpLim);
	STDMETHODIMP FetchText(long cp, wchar_t *pchBuffer, int cchBuffer, int *pcchFetch);
	STDMETHODIMP FetchTextProperties(long cp, long propertyMode, ITextPropertyList **pptpl);
	STDMETHODIMP IsNestedLayoutAtCp(DWORD dwCookieTP, long cp);
	STDMETHODIMP GetNestedObjectAttributes(DWORD dwCookieTP, long cp, long *pcwchRun, long *palign, long *pstylePos, DWORD *pdwFlags);
	STDMETHODIMP GetNestedObjectMargins(DWORD dwCookieTP, long cp, RECT *rcMargins);
	STDMETHODIMP GetHtmlDoc(IHTMLDocument2** ppDoc);
	STDMETHODIMP GetCurrentElement(IHTMLElement** ppElement);

//// ITextLayoutSite implementation
protected:
	STDMETHODIMP GetLayoutSize(DWORD dwSiteCookie, int nUnit, long *pdxpWidth, long *pdypHeight);
	STDMETHODIMP GetPadding(DWORD dwSiteCookie, int nUnit, 
				long *pdxpLeft, long *pdxpRight,
				long *pdypTop, long *pdypBottom);
	STDMETHODIMP GetColumns(DWORD dwSiteCookie, int *pcColumns);
	STDMETHODIMP GetConstraintPoints(DWORD dwSiteCookie, ITextConstraintPoints **ppPoints, DWORD *pdwConsCookie);
	STDMETHODIMP GetDisplayRect(DWORD dwSiteCookie, RECT *prect);
	STDMETHODIMP GetScale(DWORD dwSiteCookie, long *pdxpInch, long *pdypInch);
	STDMETHODIMP GetBkgndMode(DWORD dwSiteCookie, long *pBkMode, COLORREF *pcrBack);
	STDMETHODIMP GetRotation(DWORD dwSiteCookie, float *pdegs);
	STDMETHODIMP GetTextFlow(DWORD dwSiteCookie, long *pqtf);
	STDMETHODIMP GetVerticalAlign(DWORD dwSiteCookie, long *pVA);
	STDMETHODIMP GetAllowSplitWordsAcrossConstraints(DWORD dwSiteCookie, BOOL *pfAllow);
	STDMETHODIMP GetHyphenation(DWORD dwSiteCookie, long *pfHyphenate, float *pptsHyphenationZone);
    STDMETHODIMP GetScrollPosition(long *pdxpScroll, long *pdypScroll);
    STDMETHODIMP GetVisible(long *pfVisible);

	STDMETHODIMP_(void) OnSelectionChanged(DWORD dwSiteCookie, long qselchg);
	STDMETHODIMP SetCrs(DWORD dwSiteCookie, long dgc, float degs);
	STDMETHODIMP GetVisibleRect(DWORD dwSiteCookie, RECT *prect);
	STDMETHODIMP EnsureRectVisible(DWORD dwSiteCookie,
							  RECT *prcBound, RECT *prcFrame,
							  long scrHorz, long scrVert);
	STDMETHODIMP ScrollForMouseDrag(DWORD dwSiteCookie, POINT pt);

	STDMETHODIMP_(BOOL) HasFocus(DWORD dwSiteCookie);
	STDMETHODIMP RequestResize(DWORD dwSiteCookie, float ptsNewWidth, float ptsNewHeight);
	
        // this one is not in the interface, but could be there if needed...
        STDMETHODIMP RequestLayout(DWORD grfLayout);
	
	STDMETHODIMP GetHdcRef(DWORD dwSiteCookie, HDC *phdcRef);
	STDMETHODIMP FreeHdcRef(DWORD dwSiteCookie, HDC hdcRef);
	STDMETHODIMP_(BOOL) UpdateWindow(DWORD dwSiteCookie);
	STDMETHODIMP CursorOut(DWORD dwSiteCookie, POINT pt, long qcmdPhysical, long qcmdLogical);

	STDMETHODIMP EventNotify(DWORD dwSiteCookie, long qevt, long evtLong1, long evtLong2);

	STDMETHODIMP CalcNestedSizeAtCp(DWORD dwSiteCookie, DWORD dwCookieTP, long cp, long fUnderLayout, long xWidthMax, SIZE *psize);
	STDMETHODIMP GetAbsoluteRenderPosition(DWORD dwSiteCookie, DWORD dwCookieTP, long cp, long *pdxpLeft, long *pdypTop);
	STDMETHODIMP DrawNestedLayoutAtCp(DWORD dwSiteCookie, DWORD dwCookieTP, long cp);
    STDMETHODIMP PositionNestedLayout(DWORD dwSiteCookie, DWORD dwCookieTP, long cp, RECT *prc);

    STDMETHODIMP AppendRectHelper(DWORD dwSiteCookie, void *pvaryRects, RECT * prcBound,
                             RECT * prcLine, LONG  xOffset, LONG yOffset,
                             LONG cp, LONG cpClipStart, LONG cpClipFinish);

    STDMETHODIMP InitDisplayTreeForPositioning(DWORD dwSiteCookie, long cpStart);
    STDMETHODIMP PositionLayoutDispNode(DWORD dwSiteCookie, long cpObject, long dx, long dy);
    STDMETHODIMP DrawBackgroundAndBorder(DWORD dwSiteCookie, RECT *prcClip, long cpStart, long cpLim);

//// ITextInPlacePaint implementation
protected:
	STDMETHODIMP BeginIncrementalPaint(DWORD dwSiteCookie, long tc, RECT *prcMax, DWORD dwFlags,
                                        HDC *phdcRef, POINT *rgptRotate);
	STDMETHODIMP EndIncrementalPaint(DWORD dwSiteCookie, long tc);
	STDMETHODIMP BeginRectPaint(DWORD dwSiteCookie, long tc, RECT *prc, DWORD dwFlags, long SurfaceKind, HDC *phdc,
							    POINT *rgptRotate);
	STDMETHODIMP EndRectPaint(DWORD dwSiteCookie, long tc, HDC *phdc);
	STDMETHODIMP_(BOOL) IsRectObscured(DWORD dwSiteCookie, long tc, RECT *prc, POINT *rgptRotate);
	STDMETHODIMP InvalidateRect(DWORD dwSiteCookie, long tc, RECT *prc, DWORD dwFlags, POINT *rgptRotate);

//// ILineServicesHost implementation
protected:
	STDMETHODIMP GetTreePointer(long cpRel, DWORD *pdwCookieTP, long *pcchOffsetOfCpRelFromTPStart);
	STDMETHODIMP FreeTreePointer(DWORD dwCookieTP);
	STDMETHODIMP SetTreePointerCp(DWORD dwCookieTP, long cpRel, long *pcchOffsetOfCpRelFromTPStart);
	STDMETHODIMP GetTreePointerCch(DWORD dwCookieTP, long *pcch);
	STDMETHODIMP MoveToNextTreePos(DWORD dwCookieTP);
	STDMETHODIMP IsNestedLayoutAtTreePos(DWORD dwCookieTP);
	STDMETHODIMP AdvanceBeforeLine(DWORD dwCookieTP, long cpRelOrigLineStart, long *pcchSkip);
	STDMETHODIMP AdvanceAfterLine(DWORD dwCookieTP, DWORD dwCookieLC, long cpRelOrigLineLim, long *pcchSkip);
	STDMETHODIMP HiddenBeforeAlignedObj(DWORD dwCookieTP, long cpRelOrigLineLim, long *pcchSkip);

	STDMETHODIMP GetLineContext(DWORD *pdwCookie);
	STDMETHODIMP FreeLineContext(DWORD dwCookie);
	STDMETHODIMP SetContext(DWORD dwCookie);
	STDMETHODIMP ClearContext(DWORD dwCookie);
	STDMETHODIMP Setup(DWORD dwCookie, long dxtMaxWidth, long cpStart, BOOL fMinMaxPass);
	STDMETHODIMP DiscardLine(DWORD dwCookie);
	STDMETHODIMP CpRelFromCpLs(DWORD dwCookie, long cpLs, long *pcpRel);
	STDMETHODIMP CpLsFromCpRel(DWORD dwCookie, long cpRel, long *pcpLs);
	STDMETHODIMP FetchHostRun(DWORD dwCookie, long cpRelLs,
                    LPCWSTR *ppwchRun, DWORD *pcchRun, BOOL *pfHidden, void *plsChp,
                    long propertyGroup, ITextPropertyList **pptpl, void **ppvHostRun, DWORD *pdwFlags);
	STDMETHODIMP TerminateLineAfterRun(DWORD dwCookie, void *pvHostRun);
	STDMETHODIMP GetLineSummaryInfo(DWORD dwCookie, long cpRelLineLim, DWORD *pdwFlags);

//// CQuillGlue implementation - please document all member variables!
protected:
    IUnknown *              _pUnkOuter;         // controlling unknown
    CFlowLayout *           m_pFlowLayout;      // parent flow layout
    HWND                    m_hwndRef;
    HWND                    m_hwndPaint;
    HDC                     m_hdcPaint;
    BOOL                    m_fScreenAccess;
    RECT                    m_rectPaint;
    BOOL                    m_fMinMaxValid;     // has SIZEMODE_MMWIDTH been calculated & cached?
    LONG                    m_xMinWidth;        // results of SIZEMODE_MMWIDTH
    LONG                    m_xMaxWidth;
    LONG                    m_yBestHeight;      // height for SIZEMODE_NATURAL
    LONG                    m_yBaseline;        // baseline position for SIZEMODE_NATURAL
    LONG                    m_xWidthLongestLine;    // width of longest calculated line
    BOOL                    m_fNoContent;       // empty content
    BOOL                    _fIsEditable;       // matches CRecalcLinePtr::_fIsEditable

    CCalcInfo *             _pci;               // set while under CalcSize, used by nested calls
#ifndef DISPLAY_TREE
    CPositionInfo *         _ppi;               // set while under SetPosition, used by nested calls
#endif
    CFormDrawInfo *         _pdi;               // set while under Draw, used by nested calls

    BOOL                    _fLayoutRequested;  // layout request sent, don't send again

//// Member variables
protected:
    ITextLayoutElement *    m_ptle;             // pointer to Quill view
    BOOL                    m_fCalcBestHeight;
    CDispNode *             _pDispNodePrev;     // previous display tree node used during
                                                // <om ITextLayoutSite.PositionLayoutDispNode> callback.
};

#endif  // _QUILGLUE_HXX_
