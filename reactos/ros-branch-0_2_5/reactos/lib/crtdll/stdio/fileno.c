#include <msvcrt/stdio.h>

#undef fileno
int fileno(FILE *f) 
{
	return f->_file;
}


/*
 * @implemented
 */
int _fileno(FILE *f) 
{
	return f->_file;
}
