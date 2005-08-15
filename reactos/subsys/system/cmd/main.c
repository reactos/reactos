#include <precomp.h>
#include "resource.h"

#ifdef _UNICODE
extern int _main (void);
#else
extern int _main (int argc, char *argv[]);
#endif

/*
 * main function
 */
#ifdef _UNICODE
int main(void)
#else
int main (int argc, char *argv[])
#endif
{
#ifdef _UNICODE
  return _main();
#else
  return _main(argc, argv);
#endif
}

/* EOF */
