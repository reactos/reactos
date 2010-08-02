/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        history.h
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 */

void newReversible();

void undo();

void redo();

void resetToU1();

void clearHistory();

void insertReversible();

void cropReversible(int width, int height, int xOffset, int yOffset);
