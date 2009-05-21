/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        mouse.h
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 */

void placeSelWin();

void startPainting(HDC hdc, short x, short y, int fg, int bg);

void whilePainting(HDC hdc, short x, short y, int fg, int bg);

void endPainting(HDC hdc, short x, short y, int fg, int bg);
