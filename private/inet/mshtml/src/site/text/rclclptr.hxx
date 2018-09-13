#ifndef I_RCLCLPTR_HXX_
#define I_RCLCLPTR_HXX_
#pragma INCMSG("--- Beg 'rclcptr.hxx'")

#ifndef X_MARGIN_HXX_
#define X_MARGIN_HXX_
#include "margin.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

class CLine;
class CLineArray;

MtExtern(CRecalcLinePtr);

//+------------------------------------------------------------------------
//
//  Class:      CRecalcLinePtr
//
//  Synopsis:   Special line pointer. Encapsulate the use of a temporary
//              line array when building lines. This pointer automatically
//              switches between the main and the temporary new line array
//              depending on the line index.
//
//-------------------------------------------------------------------------

class CRecalcLinePtr
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRecalcLinePtr))

    CRecalcLinePtr(CDisplay *pdp, CCalcInfo *pci);

    void Init(CLineArray * prgliOld,
                int iNewFirst, CLineArray * prgliNew);
    void Reset(int iNewFirst);

    CLine * operator[] (int iLine);

    CLine * First(int iLine);

    int At() { return _iLine; }

    CLine * AddLine();
    CLine * InsertLine(int iLine);

    LONG    Count () { return _cAll; }

    CLine * Next();
    CLine * Prev();

    CStackDataAry < CAlignedLine, 32 > _aryLeftAlignedImgs;
    CStackDataAry < CAlignedLine, 32 > _aryRightAlignedImgs;

    CDisplay  * _pdp;
    CCalcInfo * _pci;

    CMarginInfo _marginInfo;    // Margin info

    LONG    _xLeadAdjust;
    LONG    _xMarqueeWidth;
    LONG    _xLayoutLeftIndent;
    LONG    _xLayoutRightIndent;
    LONG    _xMaxRightAlign;

    long    _cLeftAlignedLayouts;
    long    _cRightAlignedLayouts;

    // For calculating interline spacing.
    void    InitPrevAfter(BOOL *pfLastWasBreak, CLinePtr & rpOld);

    CTreeNode * CalcInterParaSpace(CLSMeasurer * pMe, LONG iPrevLine, BOOL fFirstLineInLayout);
    
    void    RecalcMargins(
                    int iLineFirst,
                    int iLineLast,
                    LONG yHeight,
                    LONG yBeforeSpace);

    // Unfortunate Netscape compatibility function.
    LONG   NetscapeBottomMargin(CLSMeasurer *pMe);

    int    AlignObjects( CLSMeasurer * pme,
                         CLine      *pLineMeasured,
                         LONG        cch,
                         BOOL        fMeasuringSitesAtBOL,
                         BOOL        fBreakAtWord,
                         BOOL        fMinMaxPass,
                         int         iLineFirst,
                         int         iLineLast,
                         LONG       *pyHeight,
                         int         xWidhtMax,
                         LONG       *pyAlignDescent,
                         LONG       *pxMaxLineWidth = NULL);

    BOOL    ClearObjects(CLine *pLineMeasured,
                    int iLineFirst,
                    int & iLineLast,
                    LONG * pyHeight);

    void    ApplyLineIndents(CTreeNode * pNode, CLine *pLineMeasured, UINT uiFlags);

    BOOL    IsValidMargins(int yHeight)
    {
        return (yHeight < _marginInfo._yLeftMargin) &&
               (yHeight < _marginInfo._yRightMargin);
    }

    BOOL    MeasureLine(CLSMeasurer &me,
                        UINT uiFlags,
                        INT *   piLine,
                        LONG *  pyHeight,
                        LONG *  pyAlignDescent,
                        LONG *  pxMinLineWidth,
                        LONG *  pxMaxLineWidth);

    LONG    CalcAlignedSitesAtBOL(CLSMeasurer * pme,
                                  CLine      * pLineMeasured,
                                  UINT         uiFlags,
                                  INT        * piLine,
                                  LONG       * pyHeight,
                                  LONG       * xWidth,
                                  LONG       * pyAlignDescent,
                                  LONG       * pxMaxLineWidth,
                                  BOOL       * pfClearMarginsRequired);
    LONG    CalcAlignedSitesAtBOLCore(CLSMeasurer * pme,
                                      CLine      * pLineMeasured,
                                      UINT         uiFlags,
                                      INT        * piLine,
                                      LONG       * pyHeight,
                                      LONG       * xWidth,
                                      LONG       * pyAlignDescent,
                                      LONG       * pxMaxLineWidth,
                                      BOOL       * pfClearMarginsRequired);

    void    FixupChunks(CLSMeasurer & me, INT * piLine);
            
    CTreeNode * CalcParagraphSpacing(CLSMeasurer *pMe, BOOL fFirstLineInLayout);
    
    LONG    GetAvailableWidth()
    {
        return max(0L, (2 * _xMarqueeWidth)    +
                   _pdp->GetFlowLayout()->GetMaxLineWidth() -
                   _marginInfo._xLeftMargin            -
                   _marginInfo._xRightMargin);
    }

    BOOL CheckForClear(CTreeNode * pNode = NULL);
                                                    
    void SetupMeasurerForBeforeSpace(CLSMeasurer *pMe);
    void CalcAfterSpace(CLSMeasurer *pMe, LONG cpMax);
    CTreePos * CalcBeforeSpace(CLSMeasurer *pMe, BOOL fFirstLineInLayout);
    BOOL CollectSpaceInfoFromEndNode(CTreeNode *pNode, BOOL fFirstLineInLayout, BOOL fPadBordForEmptyBlock = FALSE);
    void ResetPosAndNegSpace()
            {
                _lTopPadding = _lPosSpace = _lNegSpace = _lPosSpaceNoP = _lNegSpaceNoP = 0;
            }

    long        _xBordLeftPerLine;
    long        _xBordLeft;
    long        _xBordRightPerLine;
    long        _xBordRight;
    long        _yBordTop;
    long        _yBordBottom;

    long        _xPadLeftPerLine;
    long        _xPadLeft;
    long        _xPadRightPerLine;
    long        _xPadRight;
    long        _yPadTop;
    long        _yPadBottom;

    CTreeNode * _pNodeLeftTop;
    CTreeNode * _pNodeRightTop;

private:

    int         _iLine;                 // current line
    int         _iNewFirst;             // first line to get from temporary new line array
    int         _iNewPast;              // last line + 1 to get from temporary new line array
    int         _cAll;                  // total number of lines
    CLineArray *_prgliOld;     // pointer to main line array
    CLineArray *_prgliNew;     // pointer to temporary new line array

    BOOL        _fMarginFromStyle;          // Used to determine whether to autoclear aligned things.
    BOOL        _fNoMarginAtBottom;         // no margin at bottom if explicit end P tag
    BOOL        _fIsEditable;
    
    long        _iPF;
    BOOL        _fInnerPF;
    long        _xLeft;
    long        _xRight;

    long        _lTopPadding;
    long        _lPosSpace;
    long        _lNegSpace;
    long        _lPosSpaceNoP;
    long        _lNegSpaceNoP;
    
    CTreeNode * _pPrevBlockNode;
    CTreeNode * _pPrevRunOwnerBranch;
};

#pragma INCMSG("--- End 'rclcptr.hxx'")
#else
#pragma INCMSG("*** Dup 'rclcptr.hxx'")
#endif
