/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Fallback implementation of sincos
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>

// This is a very simplistic implementation to make GCC 11 happy
void sincos(double x, double *s, double *c)
{
    *s = sin(x);
    *c = cos(x);
}
