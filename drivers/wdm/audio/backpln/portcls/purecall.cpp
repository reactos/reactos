/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/connection.c
 * PURPOSE:         purecall stub
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

extern "C" {
  void
	  __cxa_pure_virtual()
  {
    // put error handling here

    DbgBreakPoint();

  }
}

