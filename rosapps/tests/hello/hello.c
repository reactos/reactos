#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
   WCHAR str[255];
   
   wsprintfW(str, L"%04X", 0xab34);
   printf("%S\n", str);
   return(0);
}
