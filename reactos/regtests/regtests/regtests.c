/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/regtests/regtests.c
 * PURPOSE:         Regression testing host
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"

VOID
RunTestDriver(LPTSTR FileName)
{
  TestDriverMain Main;
  HMODULE hModule;

  hModule = LoadLibrary(FileName);
  if (hModule != NULL) 
    { 
        Main = (TestDriverMain) GetProcAddress(hModule, "RegTestMain");
        if (Main != NULL) 
          {
            (Main)(); 
          }
        FreeLibrary(hModule); 
    }
}

int
main(int argc, char* argv[])
{
  RunTestDriver("win32base.dll");
  RunTestDriver("kmrtint.dll");
  return 0;
}
