#include <msvcrti.h>


int * __fpecode(void)
{
  return(&(GetThreadData()->fpecode));
}
