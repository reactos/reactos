//========================================================================
//
// GooList.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#ifdef HAVE_GETTIMEOFDAY

#ifndef GOOTIMER_H
#define GOOTIMER_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "gtypes.h"
#include <sys/time.h>

//------------------------------------------------------------------------
// GooList
//------------------------------------------------------------------------

class GooTimer {
public:

  // Create a new timer.
  GooTimer();

  void stop ();
  double getElapsed();


private:

	struct timeval start;
	struct timeval end;
	GBool active;
};

#endif

#endif
