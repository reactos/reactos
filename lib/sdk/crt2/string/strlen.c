/* strlen.c
(c) 2010 Jose Catena
*/

_NOINTRINSIC(strlen)

size_t _CDECL strlen(const char *str)
{
	size_t i;

	for(i=0; str[i]; i++);
	return(i);
}
