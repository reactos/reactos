/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
//#include <crtdll/local.h>
#include <crtdll/internal/file.h>


static __file_rec __initial_file_rec = {
  0,
  5,
  { &_iob[0], &_iob[1], &_iob[2], &_iob[3], &_iob[4] }
};

__file_rec *__file_rec_list = &__initial_file_rec;
