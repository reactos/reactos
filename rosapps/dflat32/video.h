/* ---------------- video.h ----------------- */

#ifndef VIDEO_H
#define VIDEO_H

#include "rect.h"

void DfGetVideo(DFRECT, PCHAR_INFO);
void DfStoreVideo(DFRECT, PCHAR_INFO);
void DfWPutch(DFWINDOW, int, int, int);
char DfGetVideoChar(int, int);
void DfPutVideoChar(int, int, int);
void DfWPuts(DFWINDOW, void *, int, int);
void DfScrollWindow(DFWINDOW, DFRECT, int);

#define DfVideoChar(x,y) (DfGetVideoChar(x,y) & 0xFF)

#endif
