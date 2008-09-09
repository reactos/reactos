/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_cursor.h
 * PURPOSE:         d3d9.dll internal cursor methods
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_CURSOR_H
#define _D3D9_CURSOR_H

#include "d3d9_common.h"

typedef struct _D3D9Cursor
{
/* 0x0000 */    DWORD dwUnknown0000;
/* 0x0004 */    DWORD dwUnknown0004;
/* 0x0008 */    DWORD dwUnknown0008;
/* 0x000c */    DWORD dwUnknown000c;
/* 0x0010 */    DWORD dwUnknown0010;
/* 0x0014 */    DWORD dwUnknown0014;
/* 0x0018 */    DWORD dwWidth;
/* 0x001c */    DWORD dwHeight;
/* 0x0020 */    DWORD dwUnknown0020[18];
/* 0x0070 */    struct _Direct3DDevice9_INT* pBaseDevice;
/* 0x0074 */    struct _Direct3DSwapChain9_INT* pSwapChain;
/* 0x0078 */    DWORD dwUnknown0078;
/* 0x007c */    DWORD dwMonitorVirtualX;
/* 0x0080 */    DWORD dwMonitorVirtualY;
/* 0x0084 */    DWORD dwUnknown0084;
} D3D9Cursor;

D3D9Cursor* CreateD3D9Cursor(struct _Direct3DDevice9_INT* pBaseDevice, struct _Direct3DSwapChain9_INT* pSwapChain);

#endif // _D3D9_CURSOR_H
