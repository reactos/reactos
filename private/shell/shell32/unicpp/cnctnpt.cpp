#include "stdafx.h"
#pragma hdrstop

//
//  Route the CoTaskMemAlloc stuff to SHAlloc stuff (which will in turn
//  call the OLE task allocator as necessary).
//
#undef CoTaskMemAlloc
#undef CoTaskMemFree
#undef CoTaskMemRealloc

#define CoTaskMemAlloc      SHAlloc
#define CoTaskMemFree       SHFree
#define CoTaskMemRealloc    SHRealloc

#include "..\..\lib\cnctnpt.cpp"
