/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_common.h
 * PURPOSE:         d3d9.dll common functions, defines and macros
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#ifndef _D3D9_COMMON_H_
#define _D3D9_COMMON_H_

#define COBJMACROS
#include <windows.h>

#define DLLAPI __declspec(dllexport)
#define DX_D3D9_DEBUG 0x80000000

extern struct IDirect3D9Vtbl Direct3D9_Vtbl;

#endif // _D3D9_COMMON_H_
