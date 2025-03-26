/*
 * COPYRIGHT:       BSD - See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS CRT library
 * PURPOSE:         Portable implementation of roundf
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <math.h>

float roundf(float arg)
{
    if (arg < 0.0)
        return ceilf(arg - 0.5);
    else
        return floorf(arg + 0.5);
}
