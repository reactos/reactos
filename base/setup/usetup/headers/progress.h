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
 * PROGRAMMER:
 */

#pragma once

struct _PROGRESSBAR;

typedef BOOLEAN
(NTAPI *PUPDATE_PROGRESS)(
    IN struct _PROGRESSBAR* Bar,
    IN BOOLEAN AlwaysUpdate,
    OUT PSTR Buffer,
    IN SIZE_T cchBufferSize);

typedef struct _PROGRESSBAR
{
    /* Border and text positions */
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
    SHORT TextTop;
    SHORT TextRight;

    SHORT Width;

    /* Maximum and current step counts */
    ULONG StepCount;
    ULONG CurrentStep;

    /* User-specific displayed bar progress/position */
    PUPDATE_PROGRESS UpdateProgressProc;
    ULONG Progress;
    SHORT Pos;

    /* Static progress bar cues */
    BOOLEAN DoubleEdge;
    SHORT ProgressColour;
    PCSTR DescriptionText;
    PCSTR ProgressFormatText;
} PROGRESSBAR, *PPROGRESSBAR;


/* FUNCTIONS ****************************************************************/

PPROGRESSBAR
CreateProgressBarEx(
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom,
    IN SHORT TextTop,
    IN SHORT TextRight,
    IN BOOLEAN DoubleEdge,
    IN SHORT ProgressColour,
    IN ULONG StepCount,
    IN PCSTR DescriptionText OPTIONAL,
    IN PCSTR ProgressFormatText OPTIONAL,
    IN PUPDATE_PROGRESS UpdateProgressProc OPTIONAL);

PPROGRESSBAR
CreateProgressBar(
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom,
    IN SHORT TextTop,
    IN SHORT TextRight,
    IN BOOLEAN DoubleEdge,
    IN PCSTR DescriptionText OPTIONAL);

VOID
DestroyProgressBar(
    IN OUT PPROGRESSBAR Bar);

VOID
ProgressSetStepCount(
    IN PPROGRESSBAR Bar,
    IN ULONG StepCount);

VOID
ProgressNextStep(
    IN PPROGRESSBAR Bar);

VOID
ProgressSetStep(
    IN PPROGRESSBAR Bar,
    IN ULONG Step);

/* EOF */
