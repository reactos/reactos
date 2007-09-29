//========================================================================
//
// GooTimer.cc
//
// Copyright 2005 Jonathan Blandford <jrb@redhat.com>
// Inspired by gtimer.c in glib, which is Copyright 2000 by the GLib Team
//
//========================================================================

#include <config.h>

#ifdef HAVE_GETTIMEOFDAY

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "GooTimer.h"

//------------------------------------------------------------------------
// GooTimer
//------------------------------------------------------------------------


GooTimer::GooTimer() {
  gettimeofday (&start, NULL);
  active = true;
}


void
GooTimer::stop() {
  gettimeofday (&end, NULL);
  active = false;
}

#define USEC_PER_SEC 1000000
double
GooTimer::getElapsed ()
{
  double total;
  struct timeval elapsed;

  if (active)
    gettimeofday (&end, NULL);

  if (start.tv_usec > end.tv_usec)
    {
      end.tv_usec += USEC_PER_SEC;
      end.tv_sec--;
    }

  elapsed.tv_usec = end.tv_usec - start.tv_usec;
  elapsed.tv_sec = end.tv_sec - start.tv_sec;

  total = elapsed.tv_sec + ((double) elapsed.tv_usec / 1e6);
  if (total < 0)
    {
      total = 0;
    }

  return total;
}

#endif
