/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>
#include <msvcrt/string.h>

//int fputws(const wchar_t* wsOutput, FILE* fileWrite)
//int fputs(const char *s, FILE *f)

int
fputws(const wchar_t* s, FILE* f)
{
  int r = 0;
  int c;
  int unbuffered;
  wchar_t localbuf[BUFSIZ];

  unbuffered = f->_flag & _IONBF;
  if (unbuffered)
  {
    f->_flag &= ~_IONBF;
    f->_ptr = f->_base = localbuf;
    f->_bufsiz = BUFSIZ;
  }

  while ((c = *s++))
    r = putwc(c, f);

  if (unbuffered)
  {
    fflush(f);
    f->_flag |= _IONBF;
    f->_base = NULL;
    f->_bufsiz = 0;
    f->_cnt = 0;
  }
  return(r);
}
