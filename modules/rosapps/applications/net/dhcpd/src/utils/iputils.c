#include <headers.h>
#include <datatypes.h>
#include <options.h>
#include <utils.h>

int split_ip( char *text, u8b *dest, int place )
{
  int dotcount;

  /* Don't touch this, unless you like pointer aritmethic */

  *dest = 0;

  if( !text )
    return -1;

  for( dotcount = 0; (dotcount < place) && ( text ); text++ )
      if( *text == '.' )
	dotcount++;

  if( !text )
    return -2;

  while(( *text != '.' ) && ( *text != '\0' ))
    {
      *dest *= 10;
      *dest += (u8b)(*text-48);
      /* 48 is not a hack, is just the code of 0 */
      text++;
    }

  return 0;
}

int get_ip( char *text, u32b *dest )
{
  /* Don't touch this, unless you like pointer aritmethic */

  *dest = 0;

  if( !text )
    return -1;

  while( *text != '\0' )
    {
      if( *text == '.' )
	{
	  *dest = *dest << 8;
	  text++;
	  continue;
	}
      *dest *= 10;
      *dest += (u8b)(*text-48);
      /* 48 is not a hack, is just the code of 0 */
      text++;
    }

  return 0;
}
