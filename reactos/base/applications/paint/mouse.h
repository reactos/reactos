/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        mouse.h
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 */

void placeSelWin();

void startPaintingL(HDC hdc, short x, short y, int fg, int bg);

void whilePaintingL(HDC hdc, short x, short y, int fg, int bg);

void endPaintingL(HDC hdc, short x, short y, int fg, int bg);

void startPaintingR(HDC hdc, short x, short y, int fg, int bg);

void whilePaintingR(HDC hdc, short x, short y, int fg, int bg);

void endPaintingR(HDC hdc, short x, short y, int fg, int bg);
