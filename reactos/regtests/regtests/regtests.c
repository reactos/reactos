/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/regtests/regtests.c
 * PURPOSE:         Regression testing host
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#define NTOS_MODE_USER
#include <stdio.h>
#include <ntos.h>
#include "regtests.h"

#define OUPUT_MODE_DbgPrint 0
#define OUPUT_MODE_OutputDebugString 1
#define OUPUT_MODE_printf 2

static int OutputMode = 0;

static void OutputRoutine(char *Buffer)
{
  if (OutputMode == OUPUT_MODE_DbgPrint)
    {
      DbgPrint(Buffer);
    }
  else if (OutputMode == OUPUT_MODE_OutputDebugString)
    {
      OutputDebugString(Buffer);
    }
  else if (OutputMode == OUPUT_MODE_printf)
    {
      printf(Buffer);
    }
}

static VOID
RunTestDriver(LPSTR FileName, LPSTR TestName)
{
  TestDriverMain Main;
  HMODULE hModule;

  hModule = LoadLibrary(FileName);
  if (hModule != NULL) 
    { 
        Main = (TestDriverMain) GetProcAddress(hModule, "RegTestMain");
        if (Main != NULL)
          {
            (Main)(OutputRoutine, TestName);
          }
        FreeLibrary(hModule); 
    }
}

int
main(int argc, char* argv[])
{
  LPSTR testname = NULL;
  int i;

  if (argc > 1)
    {
      i = 1;
      if (argv[i][0] == '-')
        {
          switch (argv[i][1])
            {
              case 'd':
                OutputMode = OUPUT_MODE_DbgPrint;
                break;
              case 'o':
                OutputMode = OUPUT_MODE_OutputDebugString;
                break;
              case 'p':
                OutputMode = OUPUT_MODE_printf;
                break;
              default:
                printf("Usage: regtests [-dop] [testname]");
                return 0;
            }
          i++;
        }

      testname = argv[i];
    }

  RunTestDriver("win32base.dll", testname);
  RunTestDriver("kmrtint.dll", testname);
  return 0;
}
