#include <stdio.h>
#include <time.h>
void main()
{
  char curdate[9];
  time_t now;
  struct tm *tm_now;

  time(&now);
  tm_now=localtime(&now);
  strftime(curdate, sizeof(curdate), "%Y%m%d", tm_now);
  printf("%s", curdate);
}
