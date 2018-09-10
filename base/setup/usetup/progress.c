/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/progress.c
 * PURPOSE:         Partition list functions
 * PROGRAMMER:
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static
BOOLEAN NTAPI
UpdateProgressPercentage(
    IN PPROGRESSBAR Bar,
    IN BOOLEAN AlwaysUpdate,
    OUT PSTR Buffer,
    IN SIZE_T cchBufferSize)
{
    // static PCSTR ProgressFormatText;
    ULONG OldProgress = Bar->Progress;

    /* Calculate the new percentage */
    if (Bar->StepCount == 0)
        Bar->Progress = 0;
    else
        Bar->Progress = ((100 * Bar->CurrentStep + (Bar->StepCount / 2)) / Bar->StepCount);

    /* Build the progress string if it has changed */
    if ( Bar->ProgressFormatText &&
        (AlwaysUpdate || (Bar->Progress != OldProgress)) )
    {
        RtlStringCchPrintfA(Buffer, cchBufferSize,
                            Bar->ProgressFormatText, Bar->Progress);
        return TRUE;
    }
    return FALSE;
}

static
VOID
DrawBorder(
    IN PPROGRESSBAR Bar)
{
    COORD coPos;
    DWORD Written;
    SHORT i;

    /* draw upper left corner */
    coPos.X = Bar->Left;
    coPos.Y = Bar->Top + 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xDA, // '+',
                                1,
                                coPos,
                                &Written);

    /* draw upper edge */
    coPos.X = Bar->Left + 1;
    coPos.Y = Bar->Top + 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC4, // '-',
                                Bar->Right - Bar->Left - 1,
                                coPos,
                                &Written);

    /* draw upper right corner */
    coPos.X = Bar->Right;
    coPos.Y = Bar->Top + 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xBF, // '+',
                                1,
                                coPos,
                                &Written);

    /* draw left and right edge */
    for (i = Bar->Top + 2; i < Bar->Bottom; i++)
    {
        coPos.X = Bar->Left;
        coPos.Y = i;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB3, // '|',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = Bar->Right;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB3, //'|',
                                    1,
                                    coPos,
                                    &Written);
    }

    /* draw lower left corner */
    coPos.X = Bar->Left;
    coPos.Y = Bar->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC0, // '+',
                                1,
                                coPos,
                                &Written);

    /* draw lower edge */
    coPos.X = Bar->Left + 1;
    coPos.Y = Bar->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC4, // '-',
                                Bar->Right - Bar->Left - 1,
                                coPos,
                                &Written);

    /* draw lower right corner */
    coPos.X = Bar->Right;
    coPos.Y = Bar->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xD9, // '+',
                                1,
                                coPos,
                                &Written);
}

static
VOID
DrawThickBorder(
    IN PPROGRESSBAR Bar)
{
    COORD coPos;
    DWORD Written;
    SHORT i;

    /* draw upper left corner */
    coPos.X = Bar->Left;
    coPos.Y = Bar->Top + 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC9, // '+',
                                1,
                                coPos,
                                &Written);

    /* draw upper edge */
    coPos.X = Bar->Left + 1;
    coPos.Y = Bar->Top + 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xCD, // '-',
                                Bar->Right - Bar->Left - 1,
                                coPos,
                                &Written);

    /* draw upper right corner */
    coPos.X = Bar->Right;
    coPos.Y = Bar->Top + 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xBB, // '+',
                                1,
                                coPos,
                                &Written);

    /* draw left and right edge */
    for (i = Bar->Top + 2; i < Bar->Bottom; i++)
    {
        coPos.X = Bar->Left;
        coPos.Y = i;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xBA, // '|',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = Bar->Right;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xBA, //'|',
                                    1,
                                    coPos,
                                    &Written);
    }

    /* draw lower left corner */
    coPos.X = Bar->Left;
    coPos.Y = Bar->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC8, // '+',
                                1,
                                coPos,
                                &Written);

    /* draw lower edge */
    coPos.X = Bar->Left + 1;
    coPos.Y = Bar->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xCD, // '-',
                                Bar->Right - Bar->Left - 1,
                                coPos,
                                &Written);

    /* draw lower right corner */
    coPos.X = Bar->Right;
    coPos.Y = Bar->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xBC, // '+',
                                1,
                                coPos,
                                &Written);
}

static
VOID
DrawProgressBar(
    IN PPROGRESSBAR Bar)
{
    COORD coPos;
    DWORD Written;
    PROGRESSBAR BarBorder = *Bar;
    CHAR TextBuffer[256];

    /* Draw the progress bar "border" border */
    if (Bar->DoubleEdge)
    {
        BarBorder.Top -= 5;
        BarBorder.Bottom += 2;
        BarBorder.Right += 5;
        BarBorder.Left -= 5;
        DrawThickBorder(&BarBorder);
    }

    /* Draw the progress bar border */
    DrawBorder(Bar);

    /* Display the description text */
    if (Bar->DescriptionText)
        CONSOLE_SetTextXY(Bar->TextTop, Bar->TextRight, Bar->DescriptionText);

    /* Always update and display the progress */
    if (Bar->UpdateProgressProc &&
        Bar->UpdateProgressProc(Bar, TRUE, TextBuffer, ARRAYSIZE(TextBuffer)))
    {
        coPos.X = Bar->Left + (Bar->Width - strlen(TextBuffer) + 1) / 2;
        coPos.Y = Bar->Top;
        WriteConsoleOutputCharacterA(StdOutput,
                                     TextBuffer,
                                     strlen(TextBuffer),
                                     coPos,
                                     &Written);
    }

    /* Draw the empty bar */
    coPos.X = Bar->Left + 1;
    for (coPos.Y = Bar->Top + 2; coPos.Y <= Bar->Bottom - 1; coPos.Y++)
    {
        FillConsoleOutputAttribute(StdOutput,
                                   Bar->ProgressColour,
                                   Bar->Width - 2,
                                   coPos,
                                   &Written);

        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Bar->Width - 2,
                                    coPos,
                                    &Written);
    }
}


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
    IN PUPDATE_PROGRESS UpdateProgressProc OPTIONAL)
{
    PPROGRESSBAR Bar;

    Bar = (PPROGRESSBAR)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        sizeof(PROGRESSBAR));
    if (Bar == NULL)
        return NULL;

    Bar->Left = Left;
    Bar->Top = Top;
    Bar->Right = Right;
    Bar->Bottom = Bottom;
    Bar->TextTop = TextTop;
    Bar->TextRight = TextRight;

    Bar->Width = Bar->Right - Bar->Left + 1;

    Bar->DoubleEdge = DoubleEdge;
    Bar->ProgressColour = ProgressColour;
    Bar->DescriptionText = DescriptionText;
    Bar->ProgressFormatText = ProgressFormatText;

    Bar->UpdateProgressProc = UpdateProgressProc;

    /* Reset the progress bar counts and initially draw it */
    ProgressSetStepCount(Bar, StepCount);

    return Bar;
}

PPROGRESSBAR
CreateProgressBar(
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom,
    IN SHORT TextTop,
    IN SHORT TextRight,
    IN BOOLEAN DoubleEdge,
    IN PCSTR DescriptionText OPTIONAL)
{
    /* Call the Ex variant of the function */
    return CreateProgressBarEx(Left, Top, Right, Bottom,
                               TextTop, TextRight,
                               DoubleEdge,
                               FOREGROUND_YELLOW | BACKGROUND_BLUE,
                               0,
                               DescriptionText,
                               "%-3lu%%",
                               UpdateProgressPercentage);
}

VOID
DestroyProgressBar(
    IN OUT PPROGRESSBAR Bar)
{
    RtlFreeHeap(ProcessHeap, 0, Bar);
}


VOID
ProgressSetStepCount(
    IN PPROGRESSBAR Bar,
    IN ULONG StepCount)
{
    Bar->CurrentStep = 0;
    Bar->StepCount = StepCount;

    Bar->Progress = 0;
    Bar->Pos = 0;

    DrawProgressBar(Bar);
}

VOID
ProgressNextStep(
    IN PPROGRESSBAR Bar)
{
    ProgressSetStep(Bar, Bar->CurrentStep + 1);
}

VOID
ProgressSetStep(
    IN PPROGRESSBAR Bar,
    IN ULONG Step)
{
    COORD coPos;
    DWORD Written;
    ULONG NewPos;
    CHAR TextBuffer[256];

    if (Step > Bar->StepCount)
        return;

    Bar->CurrentStep = Step;

    /* Update the progress and redraw it if it has changed */
    if (Bar->UpdateProgressProc &&
        Bar->UpdateProgressProc(Bar, FALSE, TextBuffer, ARRAYSIZE(TextBuffer)))
    {
        coPos.X = Bar->Left + (Bar->Width - strlen(TextBuffer) + 1) / 2;
        coPos.Y = Bar->Top;
        WriteConsoleOutputCharacterA(StdOutput,
                                     TextBuffer,
                                     strlen(TextBuffer),
                                     coPos,
                                     &Written);
    }

    /* Calculate the bar position */
    NewPos = (((Bar->Width - 2) * 2 * Bar->CurrentStep + (Bar->StepCount / 2)) / Bar->StepCount);

    /* Redraw the bar if it has changed */
    if (Bar->Pos != NewPos)
    {
        Bar->Pos = NewPos;

        for (coPos.Y = Bar->Top + 2; coPos.Y <= Bar->Bottom - 1; coPos.Y++)
        {
            coPos.X = Bar->Left + 1;
            FillConsoleOutputCharacterA(StdOutput,
                                        0xDB,
                                        Bar->Pos / 2,
                                        coPos,
                                        &Written);
            coPos.X += Bar->Pos / 2;

            if (NewPos & 1)
            {
                FillConsoleOutputCharacterA(StdOutput,
                                            0xDD,
                                            1,
                                            coPos,
                                            &Written);
                coPos.X++;
            }

            if (coPos.X <= Bar->Right - 1)
            {
                FillConsoleOutputCharacterA(StdOutput,
                                            ' ',
                                            Bar->Right - coPos.X,
                                            coPos,
                                            &Written);
            }
        }
    }
}

/* EOF */
