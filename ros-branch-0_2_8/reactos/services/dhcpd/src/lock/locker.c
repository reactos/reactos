#include <headers.h>
#include <datatypes.h>
#include <options.h>
#include <leases.h>
#include <lock.h>

int check_leased_list()
{
  DHCPLIST *temp, *ntemp;
  int count = 1, i;

  /*  fprintf( stdout, "checking list! \n" ); */

  return 0;
}

static int test_and_set()
{
  /* Test lock, if it's locked return FALSE */

  return TRUE;
}

int lock_list()
{
  int count = 1;

  while( !test_and_set() )
    {
      sleep( 1 );
      count++;
    }

  return count;
}

int unlock_list()
{
  return TRUE;
}
