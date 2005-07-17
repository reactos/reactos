#include <windows.h>
#include "regtests.h"

int
mainCRTStartup(int argc, char *argv[])
{
  return WinMain(NULL, NULL, NULL, 0);
}

_SetupOnce()
{
}
