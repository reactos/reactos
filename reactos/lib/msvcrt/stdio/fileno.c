#include <crtdll/stdio.h>

#if 0
#undef fileno
int fileno(FILE *f) 
{
	return f->_file;
}
#endif

int _fileno(FILE *f) 
{
	return f->_file;
}
