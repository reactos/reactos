//-------------------------------------------------------------------
//
// File: SelRange.h
//
// Contents:
//    This file contians Selection Range handling definitions.
//
// History:
//    14-Oct-94   MikeMi   Created
//
//-------------------------------------------------------------------

#ifndef __SELRANGE_H__
#define __SELRANGE_H__

#include <windows.h>
#include <limits.h>

#define SELRANGE_MINVALUE  0
#define SELRANGE_MAXVALUE  LONG_MAX - 2
#define SELRANGE_ERROR      LONG_MAX



typedef HANDLE HSELRANGE;

#ifdef __cplusplus
extern "C"
{
#endif

ILVRange *LVRange_Create( );

#ifdef __cplusplus
}
#endif

#endif
