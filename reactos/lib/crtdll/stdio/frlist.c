/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
//#include <crtdll/local.h>
#include <crtdll/internal/file.h>

extern FILE _crtdll_iob[5];

static __file_rec __initial_file_rec = {
  0,
  5,
{ &_crtdll_iob[0], &_crtdll_iob[1], &_crtdll_iob[2], 
   &_crtdll_iob[3], &_crtdll_iob[4] }
};

__file_rec *__file_rec_list = &__initial_file_rec;
