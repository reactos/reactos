#include <windows.h>
#include <tlhelp32.h>

static char* title = "     PID   PARENT  TIME NAME\n";
char buf[256];

void main()
{
  HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);

  DWORD r;
  WriteFile(stdout,title,lstrlen(title),&r,NULL);

  HANDLE pl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

  PROCESSENTRY32 pe;
  pe.dwSize = sizeof(PROCESSENTRY32);
  pe.th32ParentProcessID = 0;

  if(Process32First(pl,&pe)) do
  {
    HANDLE p =OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,pe.th32ProcessID);
    FILETIME cr;
    FILETIME ex;
    FILETIME kt;
    FILETIME ut;
    GetProcessTimes(p,&cr,&ex,&kt,&ut);
    WORD fatdate;
    WORD fattime;
    FileTimeToDosDateTime(&cr,&fatdate,&fattime);
    int hour = (fattime & 0xf800) >> 11;
    int minute = (fattime & 0x07e0) >> 5;

    wsprintf(buf,"%08X %08X %2d:%02d %s\n",pe.th32ProcessID,pe.th32ParentProcessID,hour,minute,pe.szExeFile);
    WriteFile(stdout,buf,lstrlen(buf),&r,NULL);
    CloseHandle(p);
    pe.th32ParentProcessID = 0;

  } while( Process32Next(pl,&pe));

  CloseHandle(pl);
}