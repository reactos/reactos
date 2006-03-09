/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>


static void fcloseall_helper(FILE *f)
{
  fflush(f);
  if (fileno(f) > 2)
    fclose(f);
}

void __stdio_cleanup_proc(void);
void __stdio_cleanup_proc(void)
{
  _fwalk(fcloseall_helper);
}

void (*__stdio_cleanup_hook)(void) = __stdio_cleanup_proc;
