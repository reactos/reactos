#include <msvcrt/stdlib.h>


/*
 * @implemented
 */
void _initterm(void (*fStart[])(void), void (*fEnd[])(void))
{
	int i = 0;

	if ( fStart == NULL || fEnd == NULL )
		return;

	while ( &fStart[i] < fEnd )
	{
		if ( fStart[i] != NULL )
			(*fStart[i])();
		i++;
	}
}


typedef int (* _onexit_t)(void);

/*
 * @unimplemented
 */
_onexit_t __dllonexit(_onexit_t func, void (** fStart[])(void),	void (** fEnd[])(void))
{
}

/*
 * @unimplemented
 */
_onexit_t _onexit(_onexit_t x)
{
	return x;
}
