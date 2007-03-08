#include <options.h>
#include <wutils.h>

u8b try_lock( u8b *key )
{
  if( *key == TRUE )
    return FALSE;
  return TRUE;
}
