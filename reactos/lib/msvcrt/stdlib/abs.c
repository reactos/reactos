/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int
abs(int j)
{
  return j<0 ? -j : j;
}
