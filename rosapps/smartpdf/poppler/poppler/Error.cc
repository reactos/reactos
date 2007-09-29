//========================================================================
//
// Error.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include "GlobalParams.h"
#include "Error.h"

static void CDECL defaultErrorFunction(int pos, char *msg, va_list args)
{
  if (pos >= 0) {
    fprintf(stderr, "Error (%d): ", pos);
  } else {
    fprintf(stderr, "Error: ");
  }
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");
  fflush(stderr);
}

static void CDECL (*errorFunction)(int , char *, va_list args) = defaultErrorFunction;

void setErrorFunction(void CDECL (* f)(int , char *, va_list args))
{
    errorFunction = f;
}

void CDECL error(int pos, char *msg, ...) {
  va_list args;
  // NB: this can be called before the globalParams object is created
  if (globalParams && globalParams->getErrQuiet()) {
    return;
  }
  va_start(args, msg);
  (*errorFunction)(pos, msg, args);
  va_end(args);
}
