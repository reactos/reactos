/*
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         PIT rollover table
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* GLOBALS *******************************************************************/

HALP_ROLLOVER HalpRolloverTable[15] =
{
    {1197, 10032}, /* 1 ms */
    {2394, 20064},
    {3591, 30096},
    {4767, 39952},
    {5964, 49984},
    {7161, 60016},
    {8358, 70048},
    {9555, 80080},
    {10731, 89936},
    {11949, 100144},
    {13125, 110000},
    {14322, 120032},
    {15519, 130064},
    {16695, 139920},
    {17892, 149952} /* 15 ms */
};
