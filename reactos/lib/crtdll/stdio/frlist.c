/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
//#include <crtdll/local.h>

static __file_rec __initial_file_rec = {
  0,
  5,
{ stdin, stdout, stderr, stdprn, stdaux }
};

__file_rec *__file_rec_list = &__initial_file_rec;
