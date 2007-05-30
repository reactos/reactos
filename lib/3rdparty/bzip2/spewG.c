
/* spew out a thoroughly gigantic file designed so that bzip2
   can compress it reasonably rapidly.  This is to help test
   support for large files (> 2GB) in a reasonable amount of time.
   I suggest you use the undocumented --exponential option to
   bzip2 when compressing the resulting file; this saves a bit of
   time.  Note: *don't* bother with --exponential when compressing
   Real Files; it'll just waste a lot of CPU time :-)
   (but is otherwise harmless).
*/

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>

/* The number of megabytes of junk to spew out (roughly) */
#define MEGABYTES 5000

#define N_BUF 1000000
char buf[N_BUF];

int main ( int argc, char** argv )
{
   int ii, kk, p;
   srandom(1);
   setbuffer ( stdout, buf, N_BUF );
   for (kk = 0; kk < MEGABYTES * 515; kk+=3) {
      p = 25+random()%50;
      for (ii = 0; ii < p; ii++)
         printf ( "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" );
      for (ii = 0; ii < p-1; ii++)
         printf ( "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" );
      for (ii = 0; ii < p+1; ii++)
         printf ( "ccccccccccccccccccccccccccccccccccccc" );
   }
   fflush(stdout);
   return 0;
}
