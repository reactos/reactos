#include <crtdll/stdlib.h>


void  _initterm (
        void (* fStart[])(void),
        void (* fEnd[])(void)
        )
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
