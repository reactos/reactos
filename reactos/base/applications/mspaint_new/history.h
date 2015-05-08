/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/history.h
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 */

void newReversible(void);

void undo(void);

void redo(void);

void resetToU1(void);

void clearHistory(void);

void insertReversible(HBITMAP hbm);

void cropReversible(int width, int height, int xOffset, int yOffset);
