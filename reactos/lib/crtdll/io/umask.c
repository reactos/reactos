/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <sys/stat.h>

mode_t
_umask(mode_t newmask)
{
  static mode_t the_mask = 022;
  mode_t old_mask = the_mask;
  the_mask = newmask;
  return old_mask;
}
