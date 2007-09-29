/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#ifndef _DISPLAY_MODEL_FITZ_H_
#define _DISPLAY_MODEL_FITZ_H_

#include "DisplayModel.h"

class DisplayModelFitz : public DisplayModel
{
public:
    DisplayModelFitz(DisplayMode displayMode);
    virtual ~DisplayModelFitz();

    PdfEngineFitz * pdfEngineFitz(void) { return (PdfEngineFitz*)_pdfEngine; }
    virtual void    handleLink(PdfLink *pdfLink);

    virtual int     getTextInRegion(int pageNo, RectD *region, unsigned short *buf, int buflen);

    virtual void    cvtUserToScreen(int pageNo, double *x, double *y);
    virtual void    cvtScreenToUser(int *pageNo ,double *x, double *y);
};

DisplayModelFitz *DisplayModelFitz_CreateFromFileName(
  const char *fileName,
  SizeD totalDrawAreaSize,
  int scrollbarXDy, int scrollbarYDx,
  DisplayMode displayMode, int startPage,
  WindowInfo *win);

#endif
