//+------------------------------------------------------------------------
//
//  File:       operators.cxx
//
//  Contents:   non-inline operators
//
//  History:    06-Aug-97  DavidD
//
//-------------------------------------------------------------------------

#include "headers.hxx"

void * __cdecl operator new(size_t cb)
{ return _MemAllocClear(cb); }

void * __cdecl operator new(size_t cb, int notused)
{ return _MemAllocClear(cb); }

void * __cdecl operator new(size_t cb, void * pv)
{ return pv; }

void __cdecl operator delete(void *pv)
{ _MemFree(pv); }

