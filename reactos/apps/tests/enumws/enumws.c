#include <windows.h>
#include <stdio.h>

BOOL CALLBACK
EnumDesktopProc(LPWSTR lpszWindowStation, LPARAM lParam)
{
   printf("\t%S\n", lpszWindowStation);

   return TRUE;
}

BOOL CALLBACK
EnumWindowStationProc(LPWSTR lpszWindowStation, LPARAM lParam)
{
   HWINSTA hWinSta;

   printf("%S\n", lpszWindowStation);
   hWinSta = OpenWindowStationW(lpszWindowStation, FALSE,
      WINSTA_ENUMDESKTOPS);
   if (hWinSta == NULL)
   {
      printf("\tCan't open window station.\n");
      return TRUE;
   }
   EnumDesktopsW(hWinSta, EnumDesktopProc, 0xdede);

   return TRUE;
}

int main()
{
   EnumWindowStationsW(EnumWindowStationProc, 0xbadbed);

   return 0;
}
