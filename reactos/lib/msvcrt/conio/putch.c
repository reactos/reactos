/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/conio/putch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/conio.h>

/*
 * @implemented
 */
int _putch(int c)
{
  DWORD NumberOfCharsWritten;

  if (WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),&c,1,&NumberOfCharsWritten,NULL)) {
    return -1;
  }
  return NumberOfCharsWritten;
}
