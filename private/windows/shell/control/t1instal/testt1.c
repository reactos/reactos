#include <stdio.h>
#include <stdlib.h>
#include "t1instal.h"

#define DEFAULT_STR  (void*)"Converting with pfb: "
#define PFM_STR      (void*)"Converting with pfb+pfm: "
#define FULL_STR     (void*)"Converting with pfb+pfm+ttf: "

#ifdef _MSC_VER
#  define CDECL  __cdecl
#else
#  define CDECL
#endif


#ifdef PROGRESS
const void STDCALL PrintProgress(short percent, void *arg)
{
   char *str = arg;
   
   printf("\r%s%d%%  ", str, percent);
}
#else
#define PrintProgress 0L
#endif

void CDECL main(int argc, char **argv)
{
   short res;
   char buf[128];

   if (argc==2)
      res = ConvertTypeface(argv[1], 0L, 0L,
                            PrintProgress, DEFAULT_STR);
   else if (argc==3) {
      if (IsType1(argv[2], sizeof(buf), buf)) {
         printf("Converting typeface: %s\n", buf);
         res = ConvertTypeface(argv[1], argv[2], 0L,
                               PrintProgress, PFM_STR);
      } else {
         printf("Not a valid Adobe Type 1 typeface.\n");
      }
         
   } else if (argc==4) {
      if (IsType1(argv[2], sizeof(buf), buf)) {
         printf("Converting typeface: %s\n", buf);
         res = ConvertTypeface(argv[1], argv[2], argv[3],
                               PrintProgress, PFM_STR);
      } else {
         printf("Not a valid Adobe Type 1 typeface.\n");
      }
   }

   if (res!=SUCCESS)
      puts("\nConversion failed!");

   exit((int)res);
}
