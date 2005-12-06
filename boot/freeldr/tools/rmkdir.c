#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#define DOS_PATH_CHAR	'\\'
#define UNIX_PATH_CHAR	'/'

#if defined (__DJGPP__) || defined (__WIN32__)
#define DOS_PATHS
#define PATH_CHAR		'\\'
#define PATH_CHAR_STR	"\\"
#else
#define UNIX_PATHS
#define PATH_CHAR		'/'
#define PATH_CHAR_STR	"/"
#endif

void ConvertPathCharacters(char *Path)
{
	int		i;
   
	i = 0;
	while (Path[i] != 0)
	{
		if (Path[i] == DOS_PATH_CHAR || Path[i] == UNIX_PATH_CHAR)
		{
			Path[i] = PATH_CHAR;
		}

		i++;
	}
}

int MakeDirectory(char *Directory)
{
	char	CurrentDirectory[1024];

	getcwd(CurrentDirectory, 1024);

	if (chdir(Directory) == 0)
	{
		chdir(CurrentDirectory);
		return 0;
	}

#if defined (UNIX_PATHS) || defined (__DJGPP__)
	if (mkdir(Directory, 0755) != 0)
	{
		perror("Failed to create directory");
		return 1;
	}
#else
	if (mkdir(Directory) != 0)
	{
		perror("Failed to create directory");
		return 1;
	}
#endif

	if (chdir(Directory) != 0)
	{
		perror("Failed to change directory");
		return 1;
	}

	chdir(CurrentDirectory);
	
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Wrong number of arguments\n");
		exit(1);
	}

	ConvertPathCharacters(argv[1]);

	return MakeDirectory(argv[1]);
}
