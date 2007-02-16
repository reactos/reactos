#include <headers.h>
#include <datatypes.h>

int maccpy( u8b dest[16], u8b source[16] )
{
  int i;
  for( i = 0; i < 16; i++ )
    dest[i] = source[i];
  return 0;
}

int maccmp( u8b dest[16], u8b source[16] )
{
  int i;
  char equal = 0;

  for( i = 0; i < 16; i++ )
    if( dest[i] != source[i] )
      equal = -1;

  return equal;
}

int str2mac( u8b from[17], u8b to[16] )
{
  int i, j, length;
  u8b buffer;
  u8b dif;

  length = (int)strlen( from );

  for( i = 0, j = 0; i < length; )
    {
      buffer = 0;
      while( (from[i] != ':' ) && (from[i]!= '\0' ))
	{
	  buffer = buffer << 4;
	  (from[i]>57)?(dif=55):(dif=48);
	  buffer += (from[i] - dif);
	  i++;
	}
      to[j++] = buffer;
      i++;
    }
  return 0;
}
