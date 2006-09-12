/* rsym_common.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rsym.h"

char*
convert_path ( const char* origpath )
{
	char* newpath;
	int i;

	newpath = strdup(origpath);

	i = 0;
	while (newpath[i] != 0)
	{
#ifdef UNIX_PATHS
		if (newpath[i] == '\\')
		{
			newpath[i] = '/';
		}
#else
#ifdef DOS_PATHS
		if (newpath[i] == '/')
		{
			newpath[i] = '\\';
		}
#endif
#endif
		i++;
	}
	return(newpath);
}

void*
load_file ( const char* file_name, size_t* file_size )
{
	FILE* f;
	void* FileData = NULL;

	f = fopen ( file_name, "rb" );
	if (f != NULL)
	{
		fseek(f, 0L, SEEK_END);
		*file_size = ftell(f);
		fseek(f, 0L, SEEK_SET);
		FileData = malloc(*file_size);
		if (FileData != NULL)
		{
			if ( *file_size != fread(FileData, 1, *file_size, f) )
			{
				free(FileData);
				FileData = NULL;
			}
		}
		fclose(f);
	}
	return FileData;
}
