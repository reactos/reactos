/*
 * COPYRIGHT:       BSD - See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS CRT library
 * PURPOSE:         Portable implementation of round
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <math.h>

double round(double arg)
{
    if (arg < 0.0)
        return ceil(arg - 0.5);
    else
        return floor(arg + 0.5);
}
