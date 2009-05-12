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

void SaveDIBToFile(HBITMAP hbm, char name[], HDC hdc);

HBITMAP LoadDIBFromFile(char name[]);
