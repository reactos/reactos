/*
 * PROJECT:   .inf file parser
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"

#define NDEBUG
#include <debug.h>

void
DbgPrint(const char *Fmt, ...)
{
  va_list Args;

  va_start(Args, Fmt);
  vfprintf(stderr, Fmt, Args);
  va_end(Args);
}

/* EOF */
