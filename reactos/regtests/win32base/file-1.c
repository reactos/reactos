#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  char buffer[4096];
  HANDLE file;
  DWORD wrote;
  int c;

  file = CreateFile("test.dat", 
   GENERIC_READ | GENERIC_WRITE, 
   0, 
   NULL, 
   CREATE_ALWAYS, 
   0, 
   0);

  if (file == INVALID_HANDLE_VALUE)
    {
      sprintf(Buffer, "Error opening file (Status %x)", GetLastError());
      return TS_FAILED;
    }

  for (c = 0; c < sizeof(buffer); c++)
    buffer[c] = (char)c;

  if (WriteFile( file, buffer, 4096, &wrote, NULL) == FALSE)
    {
      sprintf(Buffer, "Error writing file (Status %x)", GetLastError());
      return TS_FAILED;
    }

  SetFilePointer(file, 0, 0, FILE_BEGIN);

  if (ReadFile( file, buffer, 4096, &wrote, NULL) == FALSE)
    {
      sprintf(Buffer, "Error reading file (Status %x)", GetLastError());
      return TS_FAILED;
    }
  for (c = 0; c < sizeof(buffer); c++)
    {
      if (buffer[c] != (char)c)
        {
          strcpy(Buffer, "Error: data read back is not what was written");
          CloseHandle(file);
          return TS_FAILED;
        }
    }

  CloseHandle(file);
  return TS_OK;
}

int
File_1Test(int Command, char *Buffer)
{
  switch (Command)
    {
      case TESTCMD_RUN:
        return RunTest(Buffer);
      case TESTCMD_TESTNAME:
        strcpy(Buffer, "File read/write");
        return TS_OK;
      default:
        break;
    }
  return TS_FAILED;
}
