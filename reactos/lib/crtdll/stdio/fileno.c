#include <stdio.h>

#undef fileno
int fileno(FILE *f) 
{
	return f->_file;
}


int _fileno(FILE *f) 
{
	return f->_file;
}