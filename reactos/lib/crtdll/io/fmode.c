/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <fcntl.h>
#include <io.h>

#undef _fmode
unsigned int _fmode = O_TEXT;

unsigned  int *_fmode_dll = &_fmode;       
