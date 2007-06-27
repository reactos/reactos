
/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/


static VOID
DrawBorder(PPROGRESSBAR Bar)
{
  COORD coPos;
  ULONG Written;
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

static VOID
DrawThickBorder(PPROGRESSBAR Bar)
{
  COORD coPos;
  ULONG Written;
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

static VOID
DrawProgressBar(PPROGRESSBAR Bar)
{
  CHAR TextBuffer[8];
  COORD coPos;
  ULONG Written;
  PROGRESSBAR BarBorder = *Bar;

  /* Print percentage */
  sprintf(TextBuffer, "%-3lu%%", Bar->Percent);

  coPos.X = Bar->Left + (Bar->Width - 2) / 2;
  coPos.Y = Bar->Top;
  WriteConsoleOutputCharacterA(StdOutput,
			       TextBuffer,
			       4,
			       coPos,
			       &Written);

  /* Draw the progress bar border */
  DrawBorder(Bar);
  
  /* Write Text Associated with Bar */
  CONSOLE_SetTextXY(Bar->TextTop, Bar->TextRight, Bar->Text);
  
  /* Draw the progress bar "border" border */
  if (Bar->Double)
  {
      BarBorder.Top -= 5;
      BarBorder.Bottom += 2;
      BarBorder.Right += 5;
      BarBorder.Left -= 5;
      DrawThickBorder(&BarBorder);
  }

  /* Draw the bar */
  coPos.X = Bar->Left + 1;
  for (coPos.Y = Bar->Top + 2; coPos.Y <= Bar->Bottom - 1; coPos.Y++)
    {
      FillConsoleOutputAttribute(StdOutput,
				 FOREGROUND_YELLOW | BACKGROUND_BLUE,
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
CreateProgressBar(SHORT Left,
		  SHORT Top,
		  SHORT Right,
		  SHORT Bottom,
          SHORT TextTop,
          SHORT TextRight,
          IN BOOLEAN DoubleEdge,
          char* Text)
{
  PPROGRESSBAR Bar;

  Bar = (PPROGRESSBAR)RtlAllocateHeap(ProcessHeap,
				      0,
				      sizeof(PROGRESSBAR));
  if (Bar == NULL)
    return(NULL);

  Bar->Left = Left;
  Bar->Top = Top;
  Bar->Right = Right;
  Bar->Bottom = Bottom;
  Bar->TextTop = TextTop;
  Bar->TextRight = TextRight;
  Bar->Double = DoubleEdge;
  Bar->Text = Text;

  Bar->Width = Bar->Right - Bar->Left + 1;

  Bar->Percent = 0;
  Bar->Pos = 0;

  Bar->StepCount = 0;
  Bar->CurrentStep = 0;

  DrawProgressBar(Bar);

  return(Bar);
}


VOID
DestroyProgressBar(PPROGRESSBAR Bar)
{
  RtlFreeHeap(ProcessHeap,
	      0,
	      Bar);
}

VOID
ProgressSetStepCount(PPROGRESSBAR Bar,
		     ULONG StepCount)
{
  Bar->CurrentStep = 0;
  Bar->StepCount = StepCount;

  DrawProgressBar(Bar);
}


VOID
ProgressNextStep(PPROGRESSBAR Bar)
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
      WriteConsoleOutputCharacterA(StdOutput,
				   TextBuffer,
				   4,
				   coPos,
				   &Written);
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
	  FillConsoleOutputCharacterA(StdOutput,
				     0xDB,
				     Bar->Pos / 2,
				     coPos,
				     &Written);
	  coPos.X += Bar->Pos/2;

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


VOID
ProgressSetStep (PPROGRESSBAR Bar,
		 ULONG Step)
{
  CHAR TextBuffer[8];
  COORD coPos;
  ULONG Written;
  ULONG NewPercent;
  ULONG NewPos;

  if (Step > Bar->StepCount)
    return;

  Bar->CurrentStep = Step;

  /* Calculate new percentage */
  NewPercent = (ULONG)(((100.0 * (float)Bar->CurrentStep) / (float)Bar->StepCount) + 0.5);

  /* Redraw precentage if changed */
  if (Bar->Percent != NewPercent)
    {
      Bar->Percent = NewPercent;

      sprintf(TextBuffer, "%-3lu%%", Bar->Percent);

      coPos.X = Bar->Left + (Bar->Width - 2) / 2;
      coPos.Y = Bar->Top;
      WriteConsoleOutputCharacterA(StdOutput,
				   TextBuffer,
				   4,
				   coPos,
				   &Written);
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
	  FillConsoleOutputCharacterA(StdOutput,
				     0xDB,
				     Bar->Pos / 2,
				     coPos,
				     &Written);
	  coPos.X += Bar->Pos/2;

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
