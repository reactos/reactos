/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/setmode.c
 * PURPOSE:     Sets the file translation mode
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <io.h>
#include <stdio.h>
#include <internal/file.h>

#define NDEBUG
#include <internal/debug.h>


