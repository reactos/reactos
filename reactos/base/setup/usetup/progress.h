/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/progress.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

typedef struct _PROGRESS
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
    SHORT TextTop;
    SHORT TextRight;

    SHORT Width;

    ULONG Percent;
    SHORT Pos;

    ULONG StepCount;
    ULONG CurrentStep;

    BOOLEAN Double;
    CHAR *Text;
} PROGRESSBAR, *PPROGRESSBAR;

/* FUNCTIONS ****************************************************************/

PPROGRESSBAR
CreateProgressBar(
    SHORT Left,
    SHORT Top,
    SHORT Right,
    SHORT Bottom,
    SHORT TextTop,
    SHORT TextRight,
    BOOLEAN DoubleEdge,
    CHAR *Text);

VOID
DestroyProgressBar(
    PPROGRESSBAR Bar);

VOID
ProgressSetStepCount(
    PPROGRESSBAR Bar,
    ULONG StepCount);

VOID
ProgressNextStep(
    PPROGRESSBAR Bar);

VOID
ProgressSetStep(
    PPROGRESSBAR Bar,
    ULONG Step);

/* EOF */
