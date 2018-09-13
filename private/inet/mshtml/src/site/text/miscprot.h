#ifndef I_MISCPROT_H_
#define I_MISCPROT_H_
#pragma INCMSG("--- Beg 'miscprot.h'")

// This function is defined in treenode.cxx
styleBorderStyle ConvertFmToCSSBorderStyle(BYTE bFmBorderStyle);

// This function is defined in disp.cxx
void BoundingRectForAnArrayOfRectsWithEmptyOnes(RECT *prcBound, CDataAry<RECT> * paryRects);

void BoundingRectForAnArrayOfRects(RECT *prcBound, CDataAry<RECT> * paryRects);

#pragma INCMSG("--- End 'miscprot.h'")
#else
#pragma INCMSG("*** Dup 'miscprot.h'")
#endif
