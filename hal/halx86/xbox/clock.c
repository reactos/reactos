/*
 * PROJECT:     Xbox HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PIT rollover table
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* GLOBALS *******************************************************************/

HALP_ROLLOVER HalpRolloverTable[15] =
{
    {1125, 10000}, /* 1 ms */
    {2250, 20000},
    {3375, 30000},
    {4500, 40000},
    {5625, 50000},
    {6750, 60000},
    {7875, 70000},
    {9000, 80000},
    {10125, 90000},
    {11250, 100000},
    {12375, 110000},
    {13500, 120000},
    {14625, 130000},
    {15750, 140000},
    {16875, 150000} /* 15 ms */
};
