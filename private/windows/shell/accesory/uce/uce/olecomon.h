//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#ifndef OLECOMON_H
#define OLECOMON_H

typedef LPVOID* PPVOID;
int DoDragAndDrop(HWND hWnd, LPDROPSOURCE pDropSource);

#ifdef _DEBUG
#define TRACE    OutputDebugString

#else // _DEBUG

#define TRACE    NOP_FUNCTION

#endif // _DEBUG

#endif // OLECOMON_H
