/* ---------------- video.h ----------------- */

#ifndef VIDEO_H
#define VIDEO_H

#include "rect.h"

void GetVideo(DFRECT, PCHAR_INFO);
void StoreVideo(DFRECT, PCHAR_INFO);
void wputch(DFWINDOW, int, int, int);
char GetVideoChar(int, int);
void PutVideoChar(int, int, int);
void wputs(DFWINDOW, void *, int, int);
void scroll_window(DFWINDOW, DFRECT, int);

#define videochar(x,y) (GetVideoChar(x,y) & 0xFF)

#endif
