#include <stdlib.h>
#include <stdio.h>
#include <io.h>

static char msg[] = "Abort!\r\n";

void
abort()
{
  _write(stderr->_file, msg, sizeof(msg)-1);
  _exit(1);
}
