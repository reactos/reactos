#include <stdio.h>

int _utime(const char* filename, struct utimbuf* buf)
{
   printf("utime(filename %s, buf %x)\n",filename,buf);
   return(-1);
}
