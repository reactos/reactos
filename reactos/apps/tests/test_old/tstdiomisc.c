#include <stdio.h>

int
t1 (void)
{
  int n = -1;
  sscanf ("abc  ", "abc %n", &n);
  printf ("t1: count=%d\n", n);

  return n != 5;
}

int
t2 (void)
{
  int result = 0;
  int n;
  long N;
  int retval;
#define SCAN(INPUT, FORMAT, VAR, EXP_RES, EXP_VAL) \
  VAR = -1; \
  retval = sscanf (INPUT, FORMAT, &VAR); \
  printf ("sscanf (\"%s\", \"%s\", &x) => %d, x = %ld\n", \
	  INPUT, FORMAT, retval, (long int) VAR); \
  result |= retval != EXP_RES || VAR != EXP_VAL

  SCAN ("12345", "%ld", N, 1, 12345);
  SCAN ("12345", "%llllld", N, 0, -1);
  SCAN ("12345", "%LLLLLd", N, 0, -1);
  SCAN ("test ", "%*s%n",  n, 0, 4);
  SCAN ("test ", "%2*s%n",  n, 0, -1);
  SCAN ("12 ",   "%l2d",  n, 0, -1);
  SCAN ("12 ",   "%2ld",  N, 1, 12);

  n = -1;
  N = -1;
  retval = sscanf ("1 1", "%d %Z", &n, &N);
  printf ("sscanf (\"1 1\", \"%%d %%Z\", &n, &N) => %d, n = %d, N = %ld\n", \
	  retval, n, N); \
  result |= retval != 1 || n != 1 || N != -1;

  return result;
}

int
main (int argc, char *argv[])
{
  int result = 0;

  result |= t1 ();
  result |= t2 ();

  result |= fflush (stdout) == EOF;

  return result;
}
