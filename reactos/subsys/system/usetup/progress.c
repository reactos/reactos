
/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "progress.h"

/* FUNCTIONS ****************************************************************/


static VOID
DrawBorder(PPROGRESS Bar)
{
  COORD coPos;
  ULONG Written;
  SHORT i;

  /* draw upper left corner */
  coPos.X = Bar->Left;
  coPos.Y = Bar->Top + 1;
  FillConsoleOutputCharacter(0xDA, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw upper edge */
  coPos.X = Bar->Left + 1;
  coPos.Y = Bar->Top + 1;
  FillConsoleOutputCharacter(0xC4, // '-',
			     Bar->Right - Bar->Left - 1,
			     coPos,
			     &Written);

  /* draw upper right corner */
  coPos.X = Bar->Right;
  coPos.Y = Bar->Top + 1;
  FillConsoleOutputCharacter(0xBF, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw left and right edge */
  for (i = Bar->Top + 2; i < Bar->Bottom; i++)
    {
      coPos.X = Bar->Left;
      coPos.Y = i;
      FillConsoleOutputCharacter(0xB3, // '|',
				 1,
				 coPos,
				 &Written);

      coPos.X = Bar->Right;
      FillConsoleOutputCharacter(0xB3, //'|',
				 1,
				 coPos,
				 &Written);
    }

  /* draw lower left corner */
  coPos.X = Bar->Left;
  coPos.Y = Bar->Bottom;
  FillConsoleOutputCharacter(0xC0, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw lower edge */
  coPos.X = Bar->Left + 1;
  coPos.Y = Bar->Bottom;
  FillConsoleOutputCharacter(0xC4, // '-',
			     Bar->Right - Bar->Left - 1,
			     coPos,
			     &Written);

  /* draw lower right corner */
  coPos.X = Bar->Right;
  coPos.Y = Bar->Bottom;
  FillConsoleOutputCharacter(0xD9, // '+',
			     1,
			     coPos,
			     &Written);
}


static VOID
DrawProgressBar(PPROGRESS Bar)
{
  CHAR TextBuffer[8];
  COORD coPos;
  ULONG Written;
  SHORT i;

  /* Print percentage */
  sprintf(TextBuffer, "%-3lu%%", Bar->Percent);

  coPos.X = Bar->Left + (Bar->Width - 2) / 2;
  coPos.Y = Bar->Top;
  WriteConsoleOutputCharacters(TextBuffer,
			       4,
			       coPos);

  DrawBorder(Bar);

  /* Draw the bar */
  coPos.X = Bar->Left + 1;
  for (coPos.Y = Bar->Top + 2; coPos.Y <= Bar->Bottom - 1; coPos.Y++)
    {
      FillConsoleOutputAttribute(0x1E, /* Yellow on blue */
				 Bar->Width - 2,
				 coPos,
				 &Written);

      FillConsoleOutputCharacter(' ',
				 Bar->Width - 2,
				 coPos,
				 &Written);
    }

}



PPROGRESS
CreateProgressBar(SHORT Left,
		  SHORT Top,
		  SHORT Right,
		  SHORT Bottom)
{
  PPROGRESS Bar;

  Bar = (PPROGRESS)RtlAllocateHeap(ProcessHeap,
				   0,
				   sizeof(PROGRESS));
  if (Bar == NULL)
    return(NULL);

  Bar->Left = Left;
  Bar->Top = Top;
  Bar->Right = Right;
  Bar->Bottom = Bottom;

  Bar->Width = Bar->Right - Bar->Left + 1;

  Bar->Percent = 0;
  Bar->Pos = 0;

  Bar->StepCount = 0;
  Bar->CurrentStep = 0;

  DrawProgressBar(Bar);

  return(Bar);
}


VOID
DestroyProgressBar(PPROGRESS Bar)
{
  RtlFreeHeap(ProcessHeap,
	      0,
	      Bar);
}

VOID
ProgressSetStepCount(PPROGRESS Bar,
		     ULONG StepCount)
{
  Bar->CurrentStep = 0;
  Bar->StepCount = StepCount;

  DrawProgressBar(Bar);
}


VOID
ProgressNextStep(PPROGRESS Bar)
{
  CHAR TextBuffer[8];
  COORD coPos;
  ULONG Written;
  ULONG NewPercent;
  ULONG NewPos;

  if ((Bar->StepCount == 0) ||
      (Bar->CurrentStep == Bar->StepCount))
    return;

  Bar->CurrentStep++;

  /* Calculate new percentage */
  NewPercent = (ULONG)(((100.0 * (float)Bar->CurrentStep) / (float)Bar->StepCount) + 0.5);

  /* Redraw precentage if changed */
  if (Bar->Percent != NewPercent)
    {
      Bar->Percent = NewPercent;

      sprintf(TextBuffer, "%-3lu%%", Bar->Percent);

      coPos.X = Bar->Left + (Bar->Width - 2) / 2;
      coPos.Y = Bar->Top;
      WriteConsoleOutputCharacters(TextBuffer,
				   4,
				   coPos);
    }

  /* Calculate bar position */
  NewPos = (ULONG)((((float)(Bar->Width - 2) * 2.0 * (float)Bar->CurrentStep) / (float)Bar->StepCount) + 0.5);

  /* Redraw bar if changed */
  if (Bar->Pos != NewPos)
    {
      Bar->Pos = NewPos;

      for (coPos.Y = Bar->Top + 2; coPos.Y <= Bar->Bottom - 1; coPos.Y++)
	{
	  coPos.X = Bar->Left + 1;
	  FillConsoleOutputCharacter(0xDB,
				     Bar->Pos / 2,
				     coPos,
				     &Written);
	  coPos.X += Bar->Pos/2;

	  if (NewPos & 1)
	    {
	      FillConsoleOutputCharacter(0xDD,
					 1,
					 coPos,
					 &Written);
	      coPos.X++;
	    }

	  if (coPos.X <= Bar->Right - 1)
	    {
	      FillConsoleOutputCharacter(' ',
					 Bar->Right - coPos.X,
					 coPos,
					 &Written);
	    }
	}
    }
}

/* EOF */
