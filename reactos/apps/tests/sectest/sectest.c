/* $Id: sectest.c,v 1.2 2002/08/14 20:58:31 dwelch Exp $ */
#define UNICODE
#define _UNICODE
#include <windows.h>

int main(int argc, char* argv[])
{
  HANDLE hFile;
  HANDLE Section;
  PVOID BaseAddress;
  
  printf("Section Test\n");
  
  hFile = CreateFile(_T("sectest.txt"),
		     GENERIC_READ | GENERIC_WRITE, 
		     0, 
		     NULL, 
		     CREATE_ALWAYS, 
		     0, 
		     0);
  if (hFile == INVALID_HANDLE_VALUE)
    {
      printf("Failed to create file (err=%d)", GetLastError());
      return 1;
    }
  
  Section = CreateFileMapping(hFile,
			      NULL,
			      PAGE_READWRITE, 
			      0,
			      4096,
			      NULL);
  if (Section == NULL)
    {
      printf("Failed to create section (err=%d)", GetLastError());
      return 1;
    }
  
  printf("Mapping view of section\n");
  BaseAddress = MapViewOfFile(Section,
			      FILE_MAP_ALL_ACCESS,
			      0,
			      0,
			      4096);
  printf("BaseAddress %x\n", (UINT) BaseAddress);
  if (BaseAddress == NULL)
    {
      printf("Failed to map section (%d)\n", GetLastError());
      return 1;
    }
  
  printf("Clearing section\n");
  FillMemory(BaseAddress, 4096, ' ');
  printf("Copying test data to section\n");
  strcpy(BaseAddress, "test data");
  
  if (!UnmapViewOfFile(BaseAddress))
    {	
      printf("Failed to unmap view of file (%d)\n", GetLastError());
      return 1;
    }
  
  if (!CloseHandle(hFile))
    {	
      printf("Failed to close file (%d)\n", GetLastError());
      return 1;
    }
  
  return 0;
}

