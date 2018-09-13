//  File:       zonepch.h
//
//  Contents:   Standard header files included by every file in this directory
//
//  
//  Functions://
//  History: 
//
//----------------------------------------------------------------------------

#ifndef _ZONEPCH_H_
#define _ZONEPCH_H_

// NOTE: This directory only supports Unicode currently

// Include this first since urlmon.hxx redefines malloc, free etc.
#include <malloc.h> // required for alloca prototype. 

#include "urlmon.hxx"
#include "shlwapi.h"
#include "shlwapip.h"
#include "winineti.h"

#include "zoneutil.h"
#include "urlenum.h"
#include "regzone.h"
#include "zonemgr.h"
#include "secmgr.h"

#include "resource.h"
// Unix: may not need to include unaligned.hpp if building with Apogee
//       Need it for Solaris CC compiler
#include <unaligned.hpp>

#endif
 

