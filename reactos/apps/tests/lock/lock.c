
#include <windows.h>
#include <stdio.h>

HANDLE	hFile;

BOOL Slock(DWORD start,DWORD len)
{

  OVERLAPPED	overl;
  BOOL	stat;
  overl.Offset	= start;
  overl.OffsetHigh	= 0;
  overl.hEvent = 0;


  stat = LockFileEx(hFile,LOCKFILE_FAIL_IMMEDIATELY,0,len,0,&overl) ;
  printf("Slock %i-%i %s\n",start,start+len,stat!=0 ? "OK" : "FAILED");
		return stat;

}

BOOL Xlock(DWORD start,DWORD len)
{

  OVERLAPPED	overl;
  BOOL	stat;

  overl.Offset	= start;
  overl.OffsetHigh	= 0;
  overl.hEvent = 0;

  stat =  LockFileEx(hFile,LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,0,len,0,&overl);

  printf("Xlock %i-%i %s\n",start,start+len,stat!=0 ? "OK" : "FAILED");
  return stat;


}

BOOL unlock(DWORD start,DWORD len)
{

  OVERLAPPED	overl;
  BOOL	stat;
  overl.Offset	= start;
  overl.OffsetHigh	= 0;
  overl.hEvent = 0;

  stat = UnlockFileEx(hFile,0,len,0,&overl) ;
  printf("unlock %i-%i %s\n",start,start+len,stat!=0 ? "OK" : "FAILED");
  return stat;

}


BOOL mkfile()
{
   hFile = CreateFile("C:\\lock.test",
                      GENERIC_READ|GENERIC_WRITE,
		      FILE_SHARE_READ|FILE_SHARE_WRITE,
		      NULL,
		      OPEN_ALWAYS,
		      FILE_ATTRIBUTE_NORMAL,
		      NULL);

   printf("mkfile %s\n",(hFile == INVALID_HANDLE_VALUE) ? "FAILED" : "OK");
   return !(hFile == INVALID_HANDLE_VALUE);


}

void main(void)
{
  DWORD ass;

  printf("enter main\n");

  mkfile();

  Slock(8,10);
  Slock(10,5);
  Slock(10,5);
  Slock(15,5);
  Slock(5,10);
  Slock(0,100);
  Xlock(30,10);
  Xlock(30,1);
  unlock(30,1);
  unlock(30,10);
  Xlock(30,5);
  Slock(35,5);
  unlock(50,5);
  unlock(0,100);
  unlock(10,5);

  if (WriteFile(hFile,"ass",4,&ass,NULL) == 0) printf("write 1 failed\n");
  else printf("write 1 success\n");
	
  CloseHandle(hFile);
  mkfile();
  Slock(0,100);

  if (WriteFile(hFile,"ass",4,&ass,NULL) == 0) printf("write 2 failed\n");
  else printf("write 2 success\n");


  CloseHandle(hFile);
  Sleep(10000);


}