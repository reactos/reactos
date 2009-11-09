/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        dib.h
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

HBITMAP CreateDIBWithProperties(int width, int height);

int GetDIBWidth(HBITMAP hbm);

int GetDIBHeight(HBITMAP hbm);

void SaveDIBToFile(HBITMAP hbm, LPTSTR name, HDC hdc);

HBITMAP LoadDIBFromFile(LPTSTR name);
