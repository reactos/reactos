/* Kill for Win32 by chris@dbn.lia.net
 * http://users.lia.net/chris/win32/
 * Public domain 
 */


#include <windows.h>

inline int h2i(char x)
{
  return (x + ((x & 0x40) >> 6) | ((x & 0x40) >> 3)) & 0x0f;
}

void msg(HANDLE out, LPCTSTR fmt...)
{
  char tmp[256];
  wvsprintf(tmp,fmt,(char*)&fmt+sizeof(fmt));
  DWORD w;
  WriteFile(out,tmp,lstrlen(tmp),&w,NULL);
}

void main(void)
{
  HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE stderr = GetStdHandle(STD_ERROR_HANDLE);

  LPTSTR cmdline = GetCommandLine();

  //Scan command line to end of program name
  while(*cmdline && *cmdline != ' ')
    if(*cmdline++ == '"')
      while(*cmdline && *cmdline++ != '"');

  // Loop while we have parameters.
  while(*cmdline)
  {
    //scan past spaces..
    while(*cmdline == ' ')
      cmdline++;

    //read in pid
    int pid = 0;
    while(*cmdline)
      pid = (pid << 4) + h2i(*cmdline++); 

    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pid);
    if(p)
    {
      if(TerminateProcess(p,(unsigned int)-9))
        msg(stdout,"Terminated PID: %08X\n",pid);
      else
        msg(stdout,"Couldn't kill PID: %08X, error=%d\n",pid,GetLastError());
      CloseHandle(p);
    }
    else
      msg(stderr,"No such PID: %08X, error=%d\n",pid,GetLastError());
  }
}