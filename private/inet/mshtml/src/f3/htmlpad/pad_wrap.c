//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:   Wrap the midl generated pad_i.c so we don't get lots of 
//          level 4 warnings from the system headers
//
//-------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#pragma warning(disable:4201)   // nameless struct/union

#include <pad_i.c>
