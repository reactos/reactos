include <stdlib.h>

int  _aexit_rtn_dll(int exitcode)
{
	_exit(exitcode);
}

void _amsg_exit (int errnum)
{
	fprintf(stdout,strerror(errnum));
        _aexit_rtn_dll(-1);      
}

