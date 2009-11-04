/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <fcntl.h>

/* Set default file mode to text */

/* Is this correct?  Default value of  _fmode in msvcrt.dll is 0. */

int _fmode = _O_TEXT; 
