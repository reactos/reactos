#include <stdio.h>
#include <io.h>

#undef _fileno

/*
 * @implemented
 */
int _fileno(FILE *f)
{
  return f->_file;
}
